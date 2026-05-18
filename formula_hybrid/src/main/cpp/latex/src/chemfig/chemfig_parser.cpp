#include "chemfig_parser.h"
#include "chemfig_constants.h"
#include "common.h"
#include <cmath>
#include <cstdlib>
#include <cwctype>

namespace tex {

namespace {
    using namespace chemfig;

    constexpr float ANGLE_INCREMENT = ANGLE_INCREMENT_DEG * CHEM_PI / 180.0f;
}

wchar_t ChemfigParser::peek(const wchar_t* p) {
    return (!p || *p == L'\0') ? L'\0' : *p;
}

wchar_t ChemfigParser::peekNext(const wchar_t* p) {
    return (!p || *p == L'\0' || *(p + 1) == L'\0') ? L'\0' : *(p + 1);
}

void ChemfigParser::skipWhitespace(const wchar_t*& p) {
    while (p && *p != L'\0' && iswspace(*p)) p++;
}

bool ChemfigParser::match(const wchar_t*& p, wchar_t expected) {
    if (peek(p) == expected) {
        p++;
        return true;
    }
    return false;
}

bool ChemfigParser::parse(const std::wstring& input, Molecule& mol) {
    mol.atoms.clear();
    mol.bonds.clear();
    mol.rings.clear();
    mol.hooks.clear();
    mol.anchors.clear();
    mol.maxAtomWidth = 0.0f;

    const wchar_t* p = input.c_str();
    skipWhitespace(p);
    int atomIndex = 0;
    bool result = (peek(p) == L'*') ? parseRing(p, mol, atomIndex) : parseChain(p, mol, atomIndex, -1, 0);
    if (!result) return false;
    skipWhitespace(p);
    while (peek(p) != L'\0') {
        if (peek(p) == L'*') {
            if (mol.rings.empty()) return false;
            size_t atomCountBefore = mol.atoms.size();
            size_t bondCountBefore = mol.bonds.size();
            size_t ringCountBefore = mol.rings.size();
            int savedAtomIndex = atomIndex;
            result = parseRing(p, mol, atomIndex);
            if (!result || mol.rings.size() <= ringCountBefore) return false;
            size_t firstRingIdx = ringCountBefore - 1;
            size_t secondRingIdx = mol.rings.size() - 1;
            int rightmostAtom = -1;
            float maxX = -1e30f;
            for (int idx : mol.rings[firstRingIdx].atomIndices) {
                if (idx >= 0 && idx < static_cast<int>(mol.atoms.size())) {
                    if (mol.atoms[idx].position.x > maxX) {
                        maxX = mol.atoms[idx].position.x;
                        rightmostAtom = idx;
                    }
                }
            }
            int leftmostNewAtom = -1;
            float minX = 1e30f;
            for (size_t i = atomCountBefore; i < mol.atoms.size(); i++) {
                if (mol.atoms[i].position.x < minX) {
                    minX = mol.atoms[i].position.x;
                    leftmostNewAtom = static_cast<int>(i);
                }
            }
            if (rightmostAtom >= 0 && leftmostNewAtom >= 0) {
                ChemPoint fromPos = mol.atoms[rightmostAtom].position;
                ChemPoint toPos = mol.atoms[leftmostNewAtom].position;
                float bondLen = DEFAULT_BOND_LENGTH;
                ChemPoint offset(fromPos.x + bondLen - toPos.x, fromPos.y - toPos.y);
                for (size_t i = atomCountBefore; i < mol.atoms.size(); i++) {
                    mol.atoms[i].position.x += offset.x;
                    mol.atoms[i].position.y += offset.y;
                }
                mol.rings[secondRingIdx].center.x += offset.x;
                mol.rings[secondRingIdx].center.y += offset.y;
                if (mol.addBond(rightmostAtom, leftmostNewAtom, BOND_SINGLE, BondParams()) < 0) return false;
            }
        } else {
            int lastAtomId = -1;
            if (!mol.atoms.empty()) lastAtomId = static_cast<int>(mol.atoms.size()) - 1;
            float lastAngle = 0.0f;
            if (!mol.rings.empty()) {
                const Ring& lastRing = mol.rings.back();
                if (!lastRing.atomIndices.empty()) {
                    lastAtomId = lastRing.atomIndices.back();
                    if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                        ChemPoint lp = mol.atoms[lastAtomId].position;
                        lastAngle = std::atan2(-(lp.y - lastRing.center.y), lp.x - lastRing.center.x);
                    }
                }
            }
            result = parseChain(p, mol, atomIndex, lastAtomId, lastAngle);
            if (!result) return false;
        }
        skipWhitespace(p);
    }
    if (result) {
        resolveHooks(mol);
        mol.normalizeBondLengths();
        for (const auto& bond : mol.bonds) {
            if (bond.fromAtom < 0 || bond.fromAtom >= static_cast<int>(mol.atoms.size()) ||
                bond.toAtom < 0 || bond.toAtom >= static_cast<int>(mol.atoms.size())) {
                return false;
            }
        }
        for (const auto& ring : mol.rings) {
            for (int idx : ring.atomIndices) {
                if (idx < 0 || idx >= static_cast<int>(mol.atoms.size())) {
                    return false;
                }
            }
        }
    }
    return result;
}

bool ChemfigParser::parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex) {
    return parseRing(p, mol, atomIndex, -1, -1, nullptr);
}

bool ChemfigParser::parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex,
                               int sharedFromAtom, int sharedToAtom, BondType* sharedBondTypeOut) {
    if (!match(p, L'*')) return false;

    bool hasInnerCircle = match(p, L'*');
    float innerStartAngle = 0;
    float innerEndAngle = 360;
    if (hasInnerCircle && match(p, L'[')) {
        innerStartAngle = parseNumber(p);
        if (match(p, L',')) {
            innerEndAngle = parseNumber(p);
        }
        while (peek(p) != L']' && peek(p) != L'\0') p++;
        match(p, L']');
    }

    int ringSize = 0;
    while (peek(p) >= L'0' && peek(p) <= L'9') {
        ringSize = ringSize * 10 + (*p - L'0');
        p++;
    }
    if (ringSize < 3 || ringSize > 12) ringSize = 6;
    if (!match(p, L'(')) return false;

    Ring ring;
    ring.sides = ringSize;
    ring.hasInnerCircle = hasInnerCircle;
    ring.innerStartAngle = innerStartAngle;
    ring.innerEndAngle = innerEndAngle;
    ring.radius = calculateRingRadius(ringSize);

    bool isChained = (sharedFromAtom >= 0 && sharedToAtom < 0);
    bool isFused = (sharedFromAtom >= 0 && sharedToAtom >= 0);

    if (isFused) {
        if (sharedFromAtom >= static_cast<int>(mol.atoms.size()) ||
            sharedToAtom >= static_cast<int>(mol.atoms.size())) {
            return false;
        }
        ChemPoint fromPos = mol.atoms[sharedFromAtom].position;
        ChemPoint toPos = mol.atoms[sharedToAtom].position;
        ChemPoint midPoint((fromPos.x + toPos.x) / 2, (fromPos.y + toPos.y) / 2);
        ChemPoint edgeDir = toPos - fromPos;
        float edgeLen = edgeDir.length();
        if (edgeLen < EPSILON) return false;

        float angleStep = CHEM_TWO_PI / ringSize;
        float halfStep = angleStep / 2.0f;
        float subRadius = edgeLen / (2.0f * std::sin(halfStep));
        ChemPoint outNormal = ChemPoint(-edgeDir.y, edgeDir.x).normalized();
        ChemPoint subCenter = midPoint + outNormal * (subRadius * std::cos(halfStep));

        ring.center = subCenter;
        ring.radius = subRadius;

        float fromAngle = std::atan2(-(fromPos.y - subCenter.y), fromPos.x - subCenter.x);
        ring.atomIndices.push_back(sharedFromAtom);

        for (int i = 1; i < ringSize - 1; i++) {
            float angle = fromAngle + i * angleStep;
            ChemPoint pos(subCenter.x + subRadius * std::cos(angle),
                         subCenter.y - subRadius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
        }
        ring.atomIndices.push_back(sharedToAtom);
    } else if (isChained) {
        if (sharedFromAtom >= static_cast<int>(mol.atoms.size())) {
            return false;
        }
        ChemPoint firstPos = mol.atoms[sharedFromAtom].position;
        float angleStep = CHEM_TWO_PI / ringSize;
        float startAngle;
        if (ringSize == 3) {
            startAngle = CHEM_PI;
        } else {
            startAngle = -(CHEM_PI - CHEM_PI / ringSize);
        }

        ChemPoint centerOffset(ring.radius * std::cos(startAngle),
                              -ring.radius * std::sin(startAngle));
        ChemPoint center(firstPos.x - centerOffset.x, firstPos.y - centerOffset.y);
        ring.center = center;

        ring.atomIndices.push_back(sharedFromAtom);
        for (int i = 1; i < ringSize; i++) {
            float angle = startAngle + i * angleStep;
            ChemPoint pos(center.x + ring.radius * std::cos(angle),
                         center.y - ring.radius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
        }
    } else {
        float angleStep = CHEM_TWO_PI / ringSize;
        float startAngle = -(CHEM_PI - CHEM_PI / ringSize);

        float cx = 0, cy = 0;
        for (int i = 0; i < ringSize; i++) {
            float angle = startAngle + i * angleStep;
            ChemPoint pos(ring.radius * std::cos(angle), -ring.radius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
            cx += pos.x;
            cy += pos.y;
        }
        ring.center = ChemPoint(cx / ringSize, cy / ringSize);
    }

    int currentRingIndex = mol.rings.size();
    mol.rings.push_back(ring);

    int bondCount = 0;
    while (bondCount < ringSize && peek(p) != L')' && peek(p) != L'\0') {
        int i = bondCount;
        if (isFused && i == ringSize - 1) {
            skipWhitespace(p);
            BondType sharedBondType = parseBondType(p);
            skipWhitespace(p);
            if (peek(p) == L'_' || peek(p) == L'^') p++;
            skipWhitespace(p);
            parseBondParams(p, 0);
            skipWhitespace(p);
            if (peek(p) == L'*') {
                mol.rings[currentRingIndex].bondTypes.push_back(sharedBondType);
                if (sharedBondTypeOut) {
                    *sharedBondTypeOut = sharedBondType;
                }
                bool hasSharedBond = false;
                for (const auto& b : mol.bonds) {
                    if ((b.fromAtom == sharedFromAtom && b.toAtom == sharedToAtom) ||
                        (b.fromAtom == sharedToAtom && b.toAtom == sharedFromAtom)) {
                        hasSharedBond = true;
                        break;
                    }
                }
                if (!hasSharedBond) {
                    mol.addBond(sharedToAtom, sharedFromAtom, sharedBondType, BondParams(), currentRingIndex);
                }
                BondType nestedSharedBondType = BOND_SINGLE;
                if (!parseRing(p, mol, atomIndex, sharedToAtom, sharedFromAtom, &nestedSharedBondType)) return false;
            } else {
                parseAtomGroup(p);
                mol.rings[currentRingIndex].bondTypes.push_back(sharedBondType);
                bool hasSharedBond = false;
                for (const auto& b : mol.bonds) {
                    if ((b.fromAtom == sharedFromAtom && b.toAtom == sharedToAtom) ||
                        (b.fromAtom == sharedToAtom && b.toAtom == sharedFromAtom)) {
                        hasSharedBond = true;
                        break;
                    }
                }
                if (!hasSharedBond) {
                    if (mol.addBond(sharedToAtom, sharedFromAtom, sharedBondType, BondParams(), currentRingIndex) < 0) return false;
                }
                if (sharedBondTypeOut) {
                    *sharedBondTypeOut = sharedBondType;
                }
            }
            bondCount++;
            continue;
        }

        skipWhitespace(p);

        if (peek(p) == L'*') {
            if (i >= static_cast<int>(mol.rings[currentRingIndex].atomIndices.size())) return false;
            int fromIdx = mol.rings[currentRingIndex].atomIndices[i];
            int toIdx = mol.rings[currentRingIndex].atomIndices[(i + 1) % ringSize];
            BondType nestedSharedBondType = BOND_SINGLE;
            if (!parseRing(p, mol, atomIndex, fromIdx, toIdx, &nestedSharedBondType)) return false;
            mol.rings[currentRingIndex].bondTypes.push_back(nestedSharedBondType);
            bondCount++;
            continue;
        }

        if (peek(p) == L'(') {
            p++;
            if (i >= static_cast<int>(mol.rings[currentRingIndex].atomIndices.size())) return false;
            int branchAtomIdx = mol.rings[currentRingIndex].atomIndices[i];
            if (branchAtomIdx < 0 || branchAtomIdx >= static_cast<int>(mol.atoms.size())) return false;
            ChemPoint branchPos = mol.atoms[branchAtomIdx].position;
            float branchAngle;
            if (ringSize == 3) {
                float angleStep = CHEM_TWO_PI / ringSize;
                float startAngle = CHEM_PI;
                float atomAngle = startAngle + i * angleStep;
                branchAngle = atomAngle + CHEM_PI / 2;
            } else {
                branchAngle = std::atan2(-(branchPos.y - mol.rings[currentRingIndex].center.y), branchPos.x - mol.rings[currentRingIndex].center.x);
            }
            if (!parseBranch(p, mol, atomIndex, branchAtomIdx, branchAngle)) return false;
            continue;
        }

        BondType bt = parseBondType(p);
        skipWhitespace(p);
        if (peek(p) == L'_' || peek(p) == L'^') p++;
        skipWhitespace(p);
        BondParams params = parseBondParams(p, 0);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        if (i >= static_cast<int>(mol.rings[currentRingIndex].atomIndices.size())) return false;
        mol.rings[currentRingIndex].bondTypes.push_back(bt);
        if (mol.addBond(mol.rings[currentRingIndex].atomIndices[i], mol.rings[currentRingIndex].atomIndices[(i + 1) % ringSize], bt, params, currentRingIndex) < 0) return false;

        int targetAtom = mol.rings[currentRingIndex].atomIndices[(i + 1) % ringSize];
        if (!label.empty() && targetAtom >= 0 && targetAtom < static_cast<int>(mol.atoms.size())) {
            mol.atoms[targetAtom].text = label;
            parseAndApplyChargeToAtom(mol, targetAtom, mol.atoms[targetAtom].text);
        }

        skipWhitespace(p);
        if (peek(p) == L'?' && peekNext(p) == L'[') {
            std::wstring hookName = parseHookName(p);
            if (!hookName.empty()) {
                mol.hooks.push_back(Hook(hookName, targetAtom));
            }
        }

        skipWhitespace(p);
        if (peek(p) == L'@' && peekNext(p) == L'{') {
            std::wstring anchorName = parseAnchorName(p);
            if (!anchorName.empty()) {
                mol.anchors.push_back(Anchor(anchorName, targetAtom));
            }
        }

        bondCount++;
    }

    {
        const wchar_t* checkP = p;
        while (peek(checkP) != L')' && peek(checkP) != L'\0') {
            if (peek(checkP) == L'*') {
                return false;
            }
            checkP++;
        }
    }
    while (peek(p) != L')' && peek(p) != L'\0') p++;
    match(p, L')');
    atomIndex += ringSize - (isFused ? 2 : (isChained ? 1 : 0));
    mol.calculateBounds();
    return true;
}

static float resolveBondAngle(const BondParams& params, float lastAngle) {
    if (!params.hasAngle) return lastAngle;
    float newAngle = params.isRelativeAngle ? lastAngle + params.angle : params.angle;
    while (newAngle >= chemfig::CHEM_TWO_PI) newAngle -= chemfig::CHEM_TWO_PI;
    while (newAngle < 0) newAngle += chemfig::CHEM_TWO_PI;
    return newAngle;
}

static int addAtomAndBond(Molecule& mol, int fromAtomId, const std::wstring& label,
                           BondType bondType, const BondParams& params,
                           float bondAngle, int& atomIndex) {
    if (fromAtomId < 0 || fromAtomId >= static_cast<int>(mol.atoms.size())) {
        return -1;
    }

    mol.updateMaxAtomWidth(mol.atoms[fromAtomId].text);
    mol.updateMaxAtomWidth(label);

    float bondLength = chemfig::DEFAULT_BOND_LENGTH * params.lengthCoeff;

    float fromHalfW = Molecule::calculateAtomWidth(mol.atoms[fromAtomId].text) / 2.0f;
    float toHalfW = Molecule::calculateAtomWidth(label) / 2.0f;
    float minVisibleBond = chemfig::DEFAULT_BOND_LENGTH * 0.3f;
    float requiredDist = fromHalfW + toHalfW + 2.0f * chemfig::TEXT_BOND_GAP + minVisibleBond;
    if (requiredDist > bondLength) {
        bondLength = requiredDist;
    }

    ChemPoint lastPos = mol.atoms[fromAtomId].position;
    ChemPoint newPos(lastPos.x + bondLength * std::cos(bondAngle),
                     lastPos.y - bondLength * std::sin(bondAngle));
    int newAtomId = mol.addAtom(newPos, label);
    atomIndex++;
    if (mol.addBond(fromAtomId, newAtomId, bondType, params) < 0) return -1;
    return newAtomId;
}

bool ChemfigParser::parseChain(const wchar_t*& p, Molecule& mol, int& atomIndex,
                                int prevAtom, float currentAngle) {
    skipWhitespace(p);
    std::wstring firstLabel = parseAtomGroup(p);

    ChemPoint startPos(0, 0);
    if (prevAtom >= 0 && prevAtom < static_cast<int>(mol.atoms.size())) {
        startPos = mol.atoms[prevAtom].position;
    }

    int lastAtomId;
    if (atomIndex == 0 && prevAtom < 0) {
        lastAtomId = mol.addAtom(startPos, firstLabel);
        atomIndex = 1;
    } else {
        lastAtomId = prevAtom;
        if (!firstLabel.empty() && lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
            mol.atoms[lastAtomId].text = firstLabel;
        }
    }

    parseAndApplyChargeToAtom(mol, lastAtomId, mol.atoms[lastAtomId].text);

    skipWhitespace(p);
    if (peek(p) == L'@' && peekNext(p) == L'{') {
        std::wstring anchorName = parseAnchorName(p);
        if (!anchorName.empty()) {
            mol.anchors.push_back(Anchor(anchorName, lastAtomId));
        }
    }

    skipWhitespace(p);
    if (peek(p) == L'?' && peekNext(p) == L'[') {
        std::wstring hookName = parseHookName(p);
        if (!hookName.empty()) {
            mol.hooks.push_back(Hook(hookName, lastAtomId));
        }
    }

    float lastAngle = currentAngle;
    int lastBranchAtomId = -1;

    while (peek(p) != L'\0') {
        skipWhitespace(p);
        wchar_t ch = peek(p);
        if (ch == L'\0' || ch == L')' || ch == L'}') break;

        if (ch == L'(') {
            p++;
            size_t prevAtomCount = mol.atoms.size();
            if (!parseBranch(p, mol, atomIndex, lastAtomId, lastAngle)) return false;
            if (mol.atoms.size() > prevAtomCount) {
                lastBranchAtomId = static_cast<int>(mol.atoms.size()) - 1;
            }
            continue;
        }

        if (ch == L'*') {
            if (!parseRing(p, mol, atomIndex, lastAtomId, -1)) return false;
            if (!mol.rings.empty()) {
                const Ring& lastRing = mol.rings.back();
                if (!lastRing.atomIndices.empty()) {
                    lastAtomId = lastRing.atomIndices.back();
                }
            }
            lastBranchAtomId = -1;
            continue;
        }

        bool hasExplicitBond = (ch == L'-' || ch == L'=' || ch == L'~' || ch == L'>' || ch == L'<');
        BondType bondType = BOND_SINGLE;
        if (hasExplicitBond) {
            bondType = parseBondType(p);
        }

        skipWhitespace(p);
        if (hasExplicitBond && (peek(p) == L'_' || peek(p) == L'^')) {
            p++;
        }

        skipWhitespace(p);
        BondParams params = parseBondParams(p, lastAngle);
        float bondAngle = resolveBondAngle(params, lastAngle);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        if (!hasExplicitBond && !label.empty()) {
            if (lastBranchAtomId >= 0 && lastBranchAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastBranchAtomId].text += label;
                parseAndApplyChargeToAtom(mol, lastBranchAtomId, mol.atoms[lastBranchAtomId].text);
            } else if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastAtomId].text += label;
                parseAndApplyChargeToAtom(mol, lastAtomId, mol.atoms[lastAtomId].text);
            }
        } else {
            lastAtomId = addAtomAndBond(mol, lastAtomId, label, bondType, params, bondAngle, atomIndex);
            if (lastAtomId < 0) return false;
            parseAndApplyChargeToAtom(mol, lastAtomId, mol.atoms[lastAtomId].text);
            if (!params.anchorName.empty()) {
                mol.anchors.push_back(Anchor(params.anchorName, lastAtomId));
            }
            lastAngle = bondAngle;
        }
        lastBranchAtomId = -1;

        skipWhitespace(p);
        if (peek(p) == L'@' && peekNext(p) == L'{') {
            std::wstring anchorName = parseAnchorName(p);
            if (!anchorName.empty()) {
                mol.anchors.push_back(Anchor(anchorName, lastAtomId));
            }
        }

        skipWhitespace(p);
        if (peek(p) == L'?' && peekNext(p) == L'[') {
            std::wstring hookName = parseHookName(p);
            if (!hookName.empty()) {
                mol.hooks.push_back(Hook(hookName, lastAtomId));
            }
        }
    }

    mol.calculateBounds();
    return true;
}

bool ChemfigParser::parseBranch(const wchar_t*& p, Molecule& mol, int& atomIndex,
                                 int branchAtom, float branchAngle) {
    int savedLastAtomId = branchAtom;
    float savedLastAngle = branchAngle;
    int lastBranchAtomId = -1;

    while (peek(p) != L'\0' && peek(p) != L')') {
        skipWhitespace(p);
        wchar_t ch = peek(p);
        if (ch == L'\0' || ch == L')' || ch == L'}') break;

        if (ch == L'(') {
            p++;
            size_t prevAtomCount = mol.atoms.size();
            if (!parseBranch(p, mol, atomIndex, savedLastAtomId, savedLastAngle)) return false;
            if (mol.atoms.size() > prevAtomCount) {
                lastBranchAtomId = static_cast<int>(mol.atoms.size()) - 1;
            }
            continue;
        }

        if (ch == L'*') {
            if (!parseRing(p, mol, atomIndex, savedLastAtomId, -1)) return false;
            if (!mol.rings.empty()) {
                const Ring& lastRing = mol.rings.back();
                if (!lastRing.atomIndices.empty()) {
                    savedLastAtomId = lastRing.atomIndices.back();
                }
            }
            lastBranchAtomId = -1;
            continue;
        }

        bool hasExplicitBond = (ch == L'-' || ch == L'=' || ch == L'~' || ch == L'>' || ch == L'<');
        BondType bondType = BOND_SINGLE;
        if (hasExplicitBond) {
            bondType = parseBondType(p);
        }

        skipWhitespace(p);
        if (hasExplicitBond && (peek(p) == L'_' || peek(p) == L'^')) {
            p++;
        }

        skipWhitespace(p);
        BondParams params = parseBondParams(p, savedLastAngle);
        float bondAngle = resolveBondAngle(params, savedLastAngle);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        if (!hasExplicitBond && !label.empty()) {
            if (lastBranchAtomId >= 0 && lastBranchAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastBranchAtomId].text += label;
                parseAndApplyChargeToAtom(mol, lastBranchAtomId, mol.atoms[lastBranchAtomId].text);
            } else if (savedLastAtomId >= 0 && savedLastAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[savedLastAtomId].text += label;
                parseAndApplyChargeToAtom(mol, savedLastAtomId, mol.atoms[savedLastAtomId].text);
            }
        } else {
            savedLastAtomId = addAtomAndBond(mol, savedLastAtomId, label, bondType, params, bondAngle, atomIndex);
            if (savedLastAtomId < 0) return false;
            parseAndApplyChargeToAtom(mol, savedLastAtomId, mol.atoms[savedLastAtomId].text);
            if (!params.anchorName.empty()) {
                mol.anchors.push_back(Anchor(params.anchorName, savedLastAtomId));
            }
            savedLastAngle = bondAngle;
        }
        lastBranchAtomId = -1;

        skipWhitespace(p);
        if (peek(p) == L'@' && peekNext(p) == L'{') {
            std::wstring anchorName = parseAnchorName(p);
            if (!anchorName.empty()) {
                mol.anchors.push_back(Anchor(anchorName, savedLastAtomId));
            }
        }

        skipWhitespace(p);
        if (peek(p) == L'?' && peekNext(p) == L'[') {
            std::wstring hookName = parseHookName(p);
            if (!hookName.empty()) {
                mol.hooks.push_back(Hook(hookName, savedLastAtomId));
            }
        }
    }

    match(p, L')');
    return true;
}

BondType ChemfigParser::parseBondType(const wchar_t*& p) {
    wchar_t ch = peek(p);

    if (ch == L'-') { p++; return BOND_SINGLE; }
    if (ch == L'=') { p++; return BOND_DOUBLE; }
    if (ch == L'~') { p++; return BOND_TRIPLE; }
    if (ch == L'>') {
        p++;
        wchar_t next = peek(p);
        if (next == L':') { p++; return BOND_WEDGE_DOTTED_UP; }
        if (next == L'|') { p++; return BOND_WEDGE_HOLLOW_UP; }
        return BOND_WEDGE_SOLID_UP;
    }
    if (ch == L'<') {
        p++;
        wchar_t next = peek(p);
        if (next == L':') { p++; return BOND_WEDGE_DOTTED_DOWN; }
        if (next == L'|') { p++; return BOND_WEDGE_HOLLOW_DOWN; }
        return BOND_WEDGE_SOLID_DOWN;
    }
    return BOND_SINGLE;
}

BondParams ChemfigParser::parseBondParams(const wchar_t*& p, float currentAngle) {
    BondParams params;

    if (peek(p) == L'#') {
        p++;
        if (match(p, L'(')) {
            float offset1 = parseNumber(p);
            while (peek(p) != L',' && peek(p) != L')' && peek(p) != L'\0') p++;
            float offset2 = offset1;
            if (match(p, L',')) {
                offset2 = parseNumber(p);
                while (peek(p) != L')' && peek(p) != L'\0') p++;
            }
            params.offsetStart = offset1;
            params.offsetEnd = offset2;
            params.hasOffset = true;
            match(p, L')');
        }
    }

    if (!match(p, L'[')) return params;

    std::wstring fields[5];
    int fieldIndex = 0;

    while (peek(p) != L']' && peek(p) != L'\0') {
        wchar_t ch = peek(p);
        if (ch == L'@' && peekNext(p) == L'{') {
            p++;
            std::wstring anchorName;
            if (match(p, L'{')) {
                while (peek(p) != L'\0' && peek(p) != L'}') {
                    anchorName += *p++;
                }
                match(p, L'}');
            }
            params.anchorName = anchorName;
            continue;
        }
        if (ch == L',' && fieldIndex < 4) {
            p++;
            fieldIndex++;
        } else {
            fields[fieldIndex] += ch;
            p++;
        }
    }
    match(p, L']');

    if (!fields[0].empty()) {
        const std::wstring& af = fields[0];
        if (af[0] == L':') {
            params.hasAngle = true;
            if (af.length() > 1 && af[1] == L':') {
                params.isRelativeAngle = true;
                params.angle = safeStof(af.substr(2)) * CHEM_PI / 180.0f;
            } else {
                params.isRelativeAngle = false;
                std::wstring numStr = af.substr(1);
                if (numStr.empty()) { params.hasAngle = false; }
                else {
                    float sign = (numStr[0] == L'-') ? -1.0f : 1.0f;
                    if (numStr[0] == L'-') numStr = numStr.substr(1);
                    if (!numStr.empty()) params.angle = sign * safeStof(numStr) * CHEM_PI / 180.0f;
                    else params.hasAngle = false;
                }
            }
        } else {
            params.hasAngle = true;
            params.isRelativeAngle = false;
            params.angle = safeStof(af) * ANGLE_INCREMENT;
        }
    }

    if (!fields[1].empty()) {
        params.lengthCoeff = safeStof(fields[1]);
        if (params.lengthCoeff <= 0.0f) params.lengthCoeff = 1.0f;
    }
    if (!fields[2].empty()) { params.fromAtomNum = safeStoi(fields[2]); }
    if (!fields[3].empty()) { params.toAtomNum = safeStoi(fields[3]); }
    if (!fields[4].empty()) { params.tikzStyle = fields[4]; }

    return params;
}

std::wstring ChemfigParser::parseAtomGroup(const wchar_t*& p) {
    std::wstring label;
    bool inBraces = match(p, L'{');

    while (peek(p) != L'\0') {
        wchar_t ch = peek(p);
        if (inBraces) {
            if (ch == L'}') {
                p++;
                skipWhitespace(p);
                if (peek(p) == L'{') {
                    p++;
                    label += L'|';
                    continue;
                }
                break;
            }
            if (ch == L'_' || ch == L'^') {
                label += ch;
                p++;
                if (peek(p) == L'{') {
                    p++;
                    int depth = 1;
                    while (peek(p) != L'\0' && depth > 0) {
                        if (peek(p) == L'{') depth++;
                        else if (peek(p) == L'}') {
                            depth--;
                            if (depth == 0) break;
                        }
                        label += *p;
                        p++;
                    }
                    if (peek(p) == L'}') p++;
                } else if (peek(p) != L'\0') {
                    label += *p;
                    p++;
                }
                label += L'|';
                continue;
            }
            if (ch == L'\\') {
                const wchar_t* cmdStart = p;
                p++;
                std::wstring cmdName;
                while (peek(p) != L'\0' && peek(p) >= L'a' && peek(p) <= L'z') {
                    cmdName += *p;
                    p++;
                }
                if (cmdName == L"chemabove" || cmdName == L"chembelow") {
                    label += L'\\';
                    label += cmdName;
                    for (int argIdx = 0; argIdx < 2; argIdx++) {
                        if (peek(p) == L'{') {
                            p++;
                            label += L'{';
                            int depth = 1;
                            while (peek(p) != L'\0' && depth > 0) {
                                if (peek(p) == L'{') depth++;
                                else if (peek(p) == L'}') {
                                    depth--;
                                    if (depth == 0) break;
                                }
                                label += *p;
                                p++;
                            }
                            if (peek(p) == L'}') { p++; }
                            label += L'}';
                        }
                    }
                    continue;
                } else if (cmdName == L"vphantom" || cmdName == L"hphantom" || cmdName == L"phantom") {
                    if (peek(p) == L'{') {
                        p++;
                        int depth = 1;
                        while (peek(p) != L'\0' && depth > 0) {
                            if (peek(p) == L'{') depth++;
                            else if (peek(p) == L'}') {
                                depth--;
                                if (depth == 0) break;
                            }
                            p++;
                        }
                        if (peek(p) == L'}') p++;
                    }
                    continue;
                } else if (cmdName == L"charge") {
                    label += L'\\';
                    label += cmdName;
                    for (int argIdx = 0; argIdx < 2; argIdx++) {
                        if (peek(p) == L'{') {
                            p++;
                            label += L'{';
                            int depth = 1;
                            while (peek(p) != L'\0' && depth > 0) {
                                if (peek(p) == L'{') depth++;
                                else if (peek(p) == L'}') {
                                    depth--;
                                    if (depth == 0) break;
                                }
                                label += *p;
                                p++;
                            }
                            if (peek(p) == L'}') { p++; }
                            label += L'}';
                        }
                    }
                    continue;
                } else if (cmdName == L"scriptstyle" || cmdName == L"scriptscriptstyle" ||
                           cmdName == L"displaystyle" || cmdName == L"textstyle") {
                    label += L'\\';
                    label += cmdName;
                    continue;
                } else if (cmdName == L"ominus" || cmdName == L"oplus" ||
                           cmdName == L"cdot" || cmdName == L"circ" ||
                           cmdName == L"bullet" || cmdName == L"times") {
                    label += L'\\';
                    label += cmdName;
                    continue;
                } else {
                    p = cmdStart;
                    label += ch;
                    p++;
                }
                continue;
            }
            label += ch;
            p++;
        } else {
            if (ch == L'(') {
                const wchar_t* scan = p + 1;
                if (!scan) { p++; continue; }
                int depth = 1;
                while (scan && *scan != L'\0' && depth > 0) {
                    if (*scan == L'(') depth++;
                    else if (*scan == L')') depth--;
                    if (depth > 0) scan++;
                }
                if (scan && *scan == L')' && *(scan + 1) != L'\0' && (*(scan + 1) == L'_' || *(scan + 1) == L'^') && *(scan + 2) == L'{') {
                    label += L'(';
                    p++;
                    depth = 1;
                    while (peek(p) != L'\0' && depth > 0) {
                        if (peek(p) == L'(') depth++;
                        else if (peek(p) == L')') {
                            depth--;
                            if (depth == 0) break;
                        }
                        wchar_t c = *p;
                        label += c;
                        p++;
                        if ((c == L'_' || c == L'^') && peek(p) != L'{') {
                            while (peek(p) != L'\0' &&
                                   ((peek(p) >= L'0' && peek(p) <= L'9') ||
                                    (peek(p) >= L'a' && peek(p) <= L'z') ||
                                    (peek(p) >= L'A' && peek(p) <= L'Z'))) {
                                label += *p;
                                p++;
                            }
                            label += L'|';
                        }
                    }
                    if (peek(p) == L')') {
                        label += L')';
                        p++;
                    }
                    if ((peek(p) == L'_' || peek(p) == L'^') && peekNext(p) == L'{') {
                        label += *p;
                        p++;
                        p++;
                        while (peek(p) != L'\0' && peek(p) != L'}') {
                            label += *p;
                            p++;
                        }
                        if (peek(p) == L'}') {
                            p++;
                        }
                        label += L'|';
                    }
                    continue;
                }
                break;
            }
            if (ch == L'^' && peekNext(p) == L'{') {
                label += ch;
                p++;
                p++;
                while (peek(p) != L'\0' && peek(p) != L'}') {
                    label += *p;
                    p++;
                }
                if (peek(p) == L'}') {
                    p++;
                }
                label += L'|';
                continue;
            }
            if (ch == L'_' && peekNext(p) == L'{') {
                label += ch;
                p++;
                p++;
                while (peek(p) != L'\0' && peek(p) != L'}') {
                    label += *p;
                    p++;
                }
                if (peek(p) == L'}') {
                    p++;
                }
                label += L'|';
                continue;
            }
            if ((ch == L'_' || ch == L'^') && peekNext(p) != L'{') {
                label += ch;
                p++;
                if (peek(p) != L'\0' && peek(p) != L'_' && peek(p) != L'^') {
                    label += *p;
                    p++;
                }
                label += L'|';
                continue;
            }
            if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') ||
                (ch >= L'0' && ch <= L'9') || ch == L'+' || ch == L' ' || ch == L'|' || ch == L'#') {
                label += ch;
                p++;
            } else if (ch == L'\\') {
                const wchar_t* temp = p + 1;
                bool isCharge = false;
                if (peek(temp) == L'c') {
                    temp++;
                    isCharge = true;
                    const wchar_t* chargeStr = L"harge";
                    while (*chargeStr != L'\0') {
                        if (peek(temp) != *chargeStr) {
                            isCharge = false;
                            break;
                        }
                        temp++;
                        chargeStr++;
                    }
                }
                if (isCharge && peek(temp) == L'{') {
                    label += L'\\';
                    p++;
                    while (peek(p) != L'\0' && peek(p) != L'{') {
                        label += *p;
                        p++;
                    }
                    label += L'{';
                    if (peek(p) == L'{') p++;
                    int depth = 1;
                    while (peek(p) != L'\0' && depth > 0) {
                        if (peek(p) == L'{') depth++;
                        else if (peek(p) == L'}') {
                            depth--;
                            if (depth == 0) break;
                        }
                        label += *p;
                        p++;
                    }
                    if (peek(p) == L'}') p++;
                    label += L'}';
                    label += L'{';
                    if (peek(p) == L'{') p++;
                    depth = 1;
                    while (peek(p) != L'\0' && depth > 0) {
                        if (peek(p) == L'{') depth++;
                        else if (peek(p) == L'}') {
                            depth--;
                            if (depth == 0) break;
                        }
                        label += *p;
                        p++;
                    }
                    if (peek(p) == L'}') p++;
                    label += L'}';
                    continue;
                } else {
                    const wchar_t* cmdStart = p;
                    p++;
                    std::wstring cmdName;
                    while (peek(p) != L'\0' && peek(p) >= L'a' && peek(p) <= L'z') {
                        cmdName += *p;
                        p++;
                    }
                    if (cmdName == L"chemabove" || cmdName == L"chembelow") {
                        label += L'\\';
                        label += cmdName;
                        for (int argIdx = 0; argIdx < 2; argIdx++) {
                            if (peek(p) == L'{') {
                                p++;
                                label += L'{';
                                int depth = 1;
                                while (peek(p) != L'\0' && depth > 0) {
                                    if (peek(p) == L'{') depth++;
                                    else if (peek(p) == L'}') {
                                        depth--;
                                        if (depth == 0) break;
                                    }
                                    label += *p;
                                    p++;
                                }
                                if (peek(p) == L'}') { p++; }
                                label += L'}';
                            }
                        }
                        continue;
                    } else if (cmdName == L"vphantom" || cmdName == L"hphantom" || cmdName == L"phantom") {
                        if (peek(p) == L'{') {
                            p++;
                            int depth = 1;
                            while (peek(p) != L'\0' && depth > 0) {
                                if (peek(p) == L'{') depth++;
                                else if (peek(p) == L'}') {
                                    depth--;
                                    if (depth == 0) break;
                                }
                                p++;
                            }
                            if (peek(p) == L'}') p++;
                        }
                        continue;
                    } else if (cmdName == L"scriptstyle" || cmdName == L"scriptscriptstyle" ||
                               cmdName == L"displaystyle" || cmdName == L"textstyle") {
                        label += L'\\';
                        label += cmdName;
                        continue;
                    } else if (cmdName == L"ominus" || cmdName == L"oplus" ||
                               cmdName == L"cdot" || cmdName == L"circ" ||
                               cmdName == L"bullet" || cmdName == L"times") {
                        label += L'\\';
                        label += cmdName;
                        continue;
                    } else {
                        p = cmdStart;
                        label += ch;
                        p++;
                    }
                }
            } else if (ch == L'\'') {
                label += ch;
                p++;
            } else {
                break;
            }
        }
    }

    while (!label.empty() && label.back() == L' ') {
        label.pop_back();
    }

    std::wstring result;
    result.reserve(label.size());
    for (wchar_t c : label) {
        if (c == L' ') result += L'|';
        else result += c;
    }

    return result;
}

float ChemfigParser::parseNumber(const wchar_t*& p) {
    std::wstring numStr;
    bool negative = match(p, L'-');

    while (peek(p) >= L'0' && peek(p) <= L'9') { numStr += *p; p++; }
    if (match(p, L'.')) {
        numStr += L'.';
        while (peek(p) >= L'0' && peek(p) <= L'9') { numStr += *p; p++; }
    }

    if (numStr.empty()) return 0;
    float val = safeStof(numStr);
    return negative ? -val : val;
}

float ChemfigParser::parseAngleSpec(const wchar_t*& p, float currentAngle) {
    bool isRelative = false;
    if (match(p, L':')) { if (match(p, L':')) isRelative = true; }

    bool negative = match(p, L'-');
    float angle = parseNumber(p);
    if (negative) angle = -angle;

    angle = angle * CHEM_PI / 180.0f;
    return isRelative ? currentAngle + angle : angle;
}

std::wstring ChemfigParser::parseHookName(const wchar_t*& p) {
    if (!match(p, L'?')) return L"";
    if (!match(p, L'[')) return L"";

    std::wstring name;
    while (peek(p) != L'\0' && peek(p) != L']') {
        name += *p;
        p++;
    }
    match(p, L']');
    return name;
}

std::wstring ChemfigParser::parseAnchorName(const wchar_t*& p) {
    if (!match(p, L'@')) return L"";
    if (!match(p, L'{')) return L"";

    std::wstring name;
    while (peek(p) != L'\0' && peek(p) != L'}') {
        name += *p;
        p++;
    }
    match(p, L'}');
    return name;
}

void ChemfigParser::resolveHooks(Molecule& mol) {
    std::map<std::wstring, std::vector<int>> hookMap;

    for (const auto& hook : mol.hooks) {
        hookMap[hook.name].push_back(hook.atomIndex);
    }

    for (const auto& pair : hookMap) {
        const std::vector<int>& atoms = pair.second;
        if (atoms.size() >= 2) {
            for (size_t i = 0; i < atoms.size() - 1; i++) {
                for (size_t j = i + 1; j < atoms.size(); j++) {
                    int from = atoms[i];
                    int to = atoms[j];

                    if (from >= 0 && to >= 0 &&
                        from < static_cast<int>(mol.atoms.size()) &&
                        to < static_cast<int>(mol.atoms.size())) {
                        mol.addBond(from, to, BOND_SINGLE, BondParams(), -1, true);
                    }
                }
            }
        }
    }
}

bool ChemfigParser::parseChargeSpec(const wchar_t*& p, std::vector<Charge>& charges) {
    if (!match(p, L'{')) return false;

    std::wstring chargeSpec;
    int depth = 1;
    while (peek(p) != L'\0' && depth > 0) {
        if (peek(p) == L'{') depth++;
        else if (peek(p) == L'}') {
            depth--;
            if (depth == 0) break;
        }
        chargeSpec += *p;
        p++;
    }
    match(p, L'}');

    size_t start = 0;
    size_t end = chargeSpec.find(L',');

    while (start < chargeSpec.length()) {
        std::wstring pair = chargeSpec.substr(start, end - start);

        size_t equalsPos = pair.find(L'=');
        if (equalsPos == std::wstring::npos) {
            return false;
        }

        std::wstring angleStr = pair.substr(0, equalsPos);
        std::wstring mark = pair.substr(equalsPos + 1);

        bool isScriptStyle = false;
        if (mark.size() >= 2 && mark[0] == L'$') {
            size_t endDollar = mark.find(L'$', 1);
            if (endDollar != std::wstring::npos) {
                std::wstring content = mark.substr(1, endDollar - 1);
                size_t scriptPos = content.find(L"\\scriptstyle");
                if (scriptPos != std::wstring::npos) {
                    isScriptStyle = true;
                    mark = content.substr(scriptPos + 12);
                } else {
                    mark = content;
                }
            }
        }

        size_t firstNonSpace = angleStr.find_first_not_of(L" \t");
        if (firstNonSpace != std::wstring::npos) {
            angleStr = angleStr.substr(firstNonSpace);
        } else {
            angleStr.clear();
        }
        float angle = 0;
        float distance = 0;
        if (!angleStr.empty()) {
            try {
                size_t colonPos = angleStr.find(L':');
                if (colonPos != std::wstring::npos) {
                    std::wstring distStr = angleStr.substr(colonPos + 1);
                    size_t ptPos = distStr.find(L"pt");
                    if (ptPos != std::wstring::npos) {
                        distStr = distStr.substr(0, ptPos);
                    }
                    distance = safeStof(distStr);
                    angleStr = angleStr.substr(0, colonPos);
                }
                angle = safeStof(angleStr);
            } catch (...) {
                return false;
            }
        }

        charges.emplace_back(angle, distance, mark, isScriptStyle);

        if (end == std::wstring::npos) break;
        start = end + 1;
        while (start < chargeSpec.length() && chargeSpec[start] == L' ') start++;
        end = chargeSpec.find(L',', start);
    }

    return true;
}

void ChemfigParser::parseAndApplyChargeToAtom(Molecule& mol, int atomIndex, std::wstring& label) {
    if (atomIndex < 0 || atomIndex >= static_cast<int>(mol.atoms.size())) {
        return;
    }

    const wchar_t* p = label.c_str();
    std::wstring newLabel;
    
    while (peek(p) != L'\0') {
        if (peek(p) == L'\\' && peekNext(p) == L'c') {
            const wchar_t* temp = p + 1;
            std::wstring cmd;
            while (peek(temp) != L'\0' && (peek(temp) >= L'a' && peek(temp) <= L'z')) {
                cmd += *temp;
                temp++;
            }
            if (cmd == L"charge") {
                p = temp;
                std::vector<Charge> charges;
                if (parseChargeSpec(p, charges)) {
                    if (!mol.atoms[atomIndex].charges.empty()) {
                        mol.atoms[atomIndex].charges.insert(
                            mol.atoms[atomIndex].charges.end(), 
                            charges.begin(), charges.end());
                    } else {
                        mol.atoms[atomIndex].charges = charges;
                    }
                    
                    if (!match(p, L'{')) continue;
                    int depth = 1;
                    std::wstring innerLabel;
                    while (peek(p) != L'\0' && depth > 0) {
                        if (peek(p) == L'{') depth++;
                        else if (peek(p) == L'}') {
                            depth--;
                            if (depth == 0) break;
                        }
                        innerLabel += *p;
                        p++;
                    }
                    match(p, L'}');
                    newLabel += innerLabel;
                    continue;
                }
            }
        }
        newLabel += *p;
        p++;
    }
    
    mol.atoms[atomIndex].text = newLabel;
}

} // namespace tex

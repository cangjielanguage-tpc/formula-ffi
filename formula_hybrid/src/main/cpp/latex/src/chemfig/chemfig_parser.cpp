#include "chemfig_parser.h"
#include "common.h"
#include <cmath>

namespace tex {

namespace {
    constexpr float CHEMFIG_PI = 3.14159265358979f;
    constexpr float DEFAULT_BOND_LENGTH = 2.5f;
    constexpr float DEFAULT_RING_RADIUS = 2.5f;
    constexpr float ANGLE_INCREMENT = 45.0f * CHEMFIG_PI / 180.0f;
    constexpr float EPSILON = 0.0001f;
}

wchar_t ChemfigParser::peek(const wchar_t* p) {
    return (!p || *p == L'\0') ? L'\0' : *p;
}

wchar_t ChemfigParser::peekNext(const wchar_t* p) {
    return (!p || *p == L'\0' || *(p + 1) == L'\0') ? L'\0' : *(p + 1);
}

void ChemfigParser::skipWhitespace(const wchar_t*& p) {
    while (p && *p == L' ') p++;
}

bool ChemfigParser::match(const wchar_t*& p, wchar_t expected) {
    if (peek(p) == expected) {
        p++;
        return true;
    }
    return false;
}

bool ChemfigParser::parse(const std::wstring& input, Molecule& mol) {
    const wchar_t* p = input.c_str();
    skipWhitespace(p);
    int atomIndex = 0;
    bool result = (peek(p) == L'*') ? parseRing(p, mol, atomIndex) : parseChain(p, mol, atomIndex, -1, 0);
    if (result) {
        resolveHooks(mol);
    }
    return result;
}

bool ChemfigParser::parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex) {
    return parseRing(p, mol, atomIndex, -1, -1);
}

bool ChemfigParser::parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex,
                               int sharedFromAtom, int sharedToAtom) {
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
    ring.radius = DEFAULT_RING_RADIUS;

    bool isChained = (sharedFromAtom >= 0 && sharedToAtom < 0);
    bool isFused = (sharedFromAtom >= 0 && sharedToAtom >= 0);

    if (isFused) {
        if (sharedFromAtom >= static_cast<int>(mol.atoms.size()) ||
            sharedToAtom >= static_cast<int>(mol.atoms.size())) {
            throw ex_parse("Chemfig ring fusion: invalid atom index");
        }
        ChemPoint fromPos = mol.atoms[sharedFromAtom].position;
        ChemPoint toPos = mol.atoms[sharedToAtom].position;
        ChemPoint midPoint((fromPos.x + toPos.x) / 2, (fromPos.y + toPos.y) / 2);
        ChemPoint edgeDir = toPos - fromPos;
        float edgeLen = edgeDir.length();

        float angleStep = 2.0f * CHEMFIG_PI / ringSize;
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
            throw ex_parse("Chemfig ring chain: invalid atom index");
        }
        ChemPoint firstPos = mol.atoms[sharedFromAtom].position;
        float angleStep = 2.0f * CHEMFIG_PI / ringSize;
        float startAngle = -(CHEMFIG_PI - CHEMFIG_PI / ringSize);

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
        float angleStep = 2.0f * CHEMFIG_PI / ringSize;
        float startAngle = -(CHEMFIG_PI - CHEMFIG_PI / ringSize);

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

    int bondCount = 0;
    while (bondCount < ringSize && peek(p) != L')' && peek(p) != L'\0') {
        int i = bondCount;
        if (isFused && i == ringSize - 1) {
            skipWhitespace(p);
            parseBondType(p);
            skipWhitespace(p);
            parseBondParams(p, 0);
            skipWhitespace(p);
            parseAtomGroup(p);
            ring.bondTypes.push_back(BOND_SINGLE);
            bondCount++;
            continue;
        }

        skipWhitespace(p);

        if (peek(p) == L'*') {
            int fromIdx = ring.atomIndices[i];
            int toIdx = ring.atomIndices[(i + 1) % ringSize];
            parseRing(p, mol, atomIndex, fromIdx, toIdx);
            ring.bondTypes.push_back(BOND_SINGLE);
            mol.addBond(fromIdx, toIdx, BOND_SINGLE);
            bondCount++;
            continue;
        }

        if (peek(p) == L'(') {
            p++;
            int branchAtomIdx = ring.atomIndices[i];
            ChemPoint branchPos = mol.atoms[branchAtomIdx].position;
            float branchAngle = std::atan2(-(branchPos.y - ring.center.y), branchPos.x - ring.center.x);
            parseBranch(p, mol, atomIndex, branchAtomIdx, branchAngle);
            continue;
        }

        BondType bt = parseBondType(p);
        skipWhitespace(p);
        BondParams params = parseBondParams(p, 0);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        ring.bondTypes.push_back(bt);
        mol.addBond(ring.atomIndices[i], ring.atomIndices[(i + 1) % ringSize], bt, params);

        int targetAtom = ring.atomIndices[(i + 1) % ringSize];
        if (!label.empty()) {
            mol.atoms[targetAtom].text = label;
        }

        skipWhitespace(p);
        if (peek(p) == L'?' && peekNext(p) == L'[') {
            std::wstring hookName = parseHookName(p);
            if (!hookName.empty()) {
                mol.hooks.push_back(Hook(hookName, targetAtom));
            }
        }

        bondCount++;
    }

    while (peek(p) != L')' && peek(p) != L'\0') p++;
    match(p, L')');
    mol.rings.push_back(ring);
    atomIndex += ringSize - (isFused ? 2 : (isChained ? 1 : 0));
    mol.calculateBounds();
    return true;
}

static float resolveBondAngle(const BondParams& params, float lastAngle) {
    if (!params.hasAngle) return lastAngle;
    return params.isRelativeAngle ? lastAngle + params.angle : params.angle;
}

static int addAtomAndBond(Molecule& mol, int fromAtomId, const std::wstring& label,
                           BondType bondType, const BondParams& params,
                           float bondAngle, int& atomIndex) {
    if (fromAtomId < 0 || fromAtomId >= static_cast<int>(mol.atoms.size())) {
        throw ex_parse("Chemfig: invalid from-atom index in bond");
    }
    float bondLength = DEFAULT_BOND_LENGTH * params.lengthCoeff;
    ChemPoint lastPos = mol.atoms[fromAtomId].position;
    ChemPoint newPos(lastPos.x + bondLength * std::cos(bondAngle),
                     lastPos.y - bondLength * std::sin(bondAngle));
    int newAtomId = mol.addAtom(newPos, label);
    atomIndex++;
    mol.addBond(fromAtomId, newAtomId, bondType, params);
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

    while (peek(p) != L'\0') {
        skipWhitespace(p);
        wchar_t ch = peek(p);
        if (ch == L'\0' || ch == L')') break;

        if (ch == L'(') {
            p++;
            parseBranch(p, mol, atomIndex, lastAtomId, lastAngle);
            continue;
        }

        if (ch == L'*') {
            parseRing(p, mol, atomIndex, lastAtomId, -1);
            if (!mol.rings.empty()) {
                auto& lastRing = mol.rings.back();
                lastAtomId = lastRing.atomIndices.back();
            }
            continue;
        }

        bool hasExplicitBond = (ch == L'-' || ch == L'=' || ch == L'~' || ch == L'>' || ch == L'<');
        BondType bondType = BOND_SINGLE;
        if (hasExplicitBond) {
            bondType = parseBondType(p);
        }

        skipWhitespace(p);
        BondParams params = parseBondParams(p, lastAngle);
        float bondAngle = resolveBondAngle(params, lastAngle);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        if (!hasExplicitBond && !label.empty()) {
            if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastAtomId].text += label;
            }
        } else {
            lastAtomId = addAtomAndBond(mol, lastAtomId, label, bondType, params, bondAngle, atomIndex);
            lastAngle = bondAngle;
        }

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

    while (peek(p) != L'\0' && peek(p) != L')') {
        skipWhitespace(p);
        wchar_t ch = peek(p);
        if (ch == L')') break;

        if (ch == L'(') {
            p++;
            parseBranch(p, mol, atomIndex, savedLastAtomId, savedLastAngle);
            continue;
        }

        if (ch == L'*') {
            parseRing(p, mol, atomIndex, savedLastAtomId, -1);
            if (!mol.rings.empty()) {
                auto& lastRing = mol.rings.back();
                savedLastAtomId = lastRing.atomIndices.back();
            }
            continue;
        }

        bool hasExplicitBond = (ch == L'-' || ch == L'=' || ch == L'~' || ch == L'>' || ch == L'<');
        BondType bondType = BOND_SINGLE;
        if (hasExplicitBond) {
            bondType = parseBondType(p);
        }

        skipWhitespace(p);
        BondParams params = parseBondParams(p, savedLastAngle);
        float bondAngle = resolveBondAngle(params, savedLastAngle);
        skipWhitespace(p);
        std::wstring label = parseAtomGroup(p);

        savedLastAtomId = addAtomAndBond(mol, savedLastAtomId, label, bondType, params, bondAngle, atomIndex);
        savedLastAngle = bondAngle;

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
                try { params.angle = std::stof(af.substr(2)) * CHEMFIG_PI / 180.0f; } catch (...) {}
            } else {
                params.isRelativeAngle = false;
                try {
                    std::wstring numStr = af.substr(1);
                    float sign = (!numStr.empty() && numStr[0] == L'-') ? -1.0f : 1.0f;
                    if (numStr[0] == L'-') numStr = numStr.substr(1);
                    params.angle = sign * std::stof(numStr) * CHEMFIG_PI / 180.0f;
                } catch (...) {}
            }
        } else {
            try {
                params.hasAngle = true;
                params.isRelativeAngle = false;
                params.angle = std::stof(af) * ANGLE_INCREMENT;
            } catch (...) {}
        }
    }

    if (!fields[1].empty()) { try { params.lengthCoeff = std::stof(fields[1]); } catch (...) {} }
    if (!fields[2].empty()) { try { params.fromAtomNum = std::stoi(fields[2]); } catch (...) {} }
    if (!fields[3].empty()) { try { params.toAtomNum = std::stoi(fields[3]); } catch (...) {} }
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
            label += ch;
            p++;
        } else {
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
                continue;
            }
            if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') ||
                (ch >= L'0' && ch <= L'9') || ch == L'_' || ch == L'^' || ch == L'+' || ch == L' ' || ch == L'|') {
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
    try { return negative ? -std::stof(numStr) : std::stof(numStr); } catch (...) { return 0; }
}

float ChemfigParser::parseAngleSpec(const wchar_t*& p, float currentAngle) {
    bool isRelative = false;
    if (match(p, L':')) { if (match(p, L':')) isRelative = true; }

    bool negative = match(p, L'-');
    float angle = parseNumber(p);
    if (negative) angle = -angle;

    angle = angle * CHEMFIG_PI / 180.0f;
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
                        mol.addBond(from, to, BOND_SINGLE);
                    }
                }
            }
        }
    }
}

} // namespace tex

#include "chemfig_builder.h"
#include "chemfig_parser.h"
#include "chemfig_constants.h"
#include "common.h"
#include <cmath>
#include <map>

namespace tex {

using namespace chemfig;

static std::wstring builderExtractPureAtomText(const std::wstring& label) {
    std::wstring result;
    const wchar_t* p = label.c_str();

    while (*p != L'\0') {
        if (*p == L'\\') {
            const wchar_t* temp = p + 1;
            std::wstring cmd;
            while (*temp != L'\0' && (*temp >= L'a' && *temp <= L'z')) {
                cmd += *temp;
                temp++;
            }
            if (cmd == L"charge") {
                p = temp;
                if (ChemfigParser::skipBraceArg(p)) {
                    result += ChemfigParser::parseBraceArg(p);
                    continue;
                }
            } else if (cmd == L"chemabove" || cmd == L"chembelow") {
                p = temp;
                std::wstring arg1 = ChemfigParser::parseBraceArg(p);
                std::wstring arg2 = ChemfigParser::parseBraceArg(p);
                bool firstIsVphantom = (arg1.size() > 10 && arg1.substr(0, 10) == L"\\vphantom");
                if (firstIsVphantom) {
                    result += arg2;
                } else {
                    result += arg1;
                }
                continue;
            } else if (cmd == L"vphantom") {
                p = temp;
                ChemfigParser::skipBraceArg(p);
                continue;
            }
        }
        result += *p;
        p++;
    }

    return result;
}

static void builderParseAndApplyCharge(Molecule& mol, int atomIndex, std::wstring& label) {
    if (atomIndex < 0 || atomIndex >= static_cast<int>(mol.atoms.size())) {
        return;
    }

    const wchar_t* p = label.c_str();
    std::wstring newLabel;

    while (ChemfigParser::peek(p) != L'\0') {
        if (ChemfigParser::peek(p) == L'\\' && ChemfigParser::peekNext(p) == L'c') {
            const wchar_t* temp = p + 1;
            std::wstring cmd;
            while (ChemfigParser::peek(temp) != L'\0' && (ChemfigParser::peek(temp) >= L'a' && ChemfigParser::peek(temp) <= L'z')) {
                cmd += *temp;
                temp++;
            }
            if (cmd == L"charge") {
                p = temp;
                std::vector<Charge> charges;
                ChemfigParser::parseChargeSpec(p, charges);
                if (!charges.empty()) {
                    if (!mol.atoms[atomIndex].charges.empty()) {
                        mol.atoms[atomIndex].charges.insert(
                            mol.atoms[atomIndex].charges.end(),
                            charges.begin(), charges.end());
                    } else {
                        mol.atoms[atomIndex].charges = charges;
                    }

                    if (ChemfigParser::match(p, L'{')) {
                        newLabel += ChemfigParser::collectBalanced(p, L'{', L'}');
                    }
                    continue;
                }
            }
        }
        newLabel += *p;
        p++;
    }

    mol.atoms[atomIndex].text = newLabel;
}

static float builderResolveBondAngle(const BondParams& params, float lastAngle) {
    if (!params.hasAngle) return lastAngle;
    float newAngle = params.isRelativeAngle ? lastAngle + params.angle : params.angle;
    while (newAngle >= CHEM_TWO_PI) newAngle -= CHEM_TWO_PI;
    while (newAngle < 0) newAngle += CHEM_TWO_PI;
    return newAngle;
}

static int builderAddAtomAndBond(Molecule& mol, int fromAtomId, const AstAtomSpec& atomSpec,
                                  const AstBondSpec& bondSpec, float bondAngle, int& atomIndex, int* outBondId = nullptr) {
    if (fromAtomId < 0 || fromAtomId >= static_cast<int>(mol.atoms.size())) {
        return -1;
    }

    std::wstring label = atomSpec.label;
    std::wstring pureLabel = builderExtractPureAtomText(label);
    std::wstring pureFromLabel = builderExtractPureAtomText(mol.atoms[fromAtomId].text);
    mol.updateMaxAtomWidth(pureFromLabel);
    mol.updateMaxAtomWidth(pureLabel);

    float fromWidth = Molecule::calculateAtomWidth(pureFromLabel);
    float toWidth = Molecule::calculateAtomWidth(pureLabel);
    float minBondForText = (fromWidth + toWidth) * BOND_TEXT_CENTER_RATIO + BOND_PARAMS_DEFAULT_COEFF_VAL;
    float bondLength = std::max(DEFAULT_BOND_LENGTH, minBondForText) * bondSpec.params.lengthCoeff;

    ChemPoint lastPos = mol.atoms[fromAtomId].position;
    ChemPoint newPos(lastPos.x + bondLength * std::cos(bondAngle),
                     lastPos.y - bondLength * std::sin(bondAngle));
    int newAtomId = mol.addAtom(newPos, label);
    atomIndex++;
    int bondId = mol.addBond(fromAtomId, newAtomId, bondSpec.type, bondSpec.params);
    if (bondId < 0) return -1;
    if (outBondId) *outBondId = bondId;

    if (!bondSpec.params.anchorName.empty()) {
        mol.anchors.push_back(Anchor(bondSpec.params.anchorName, -1, bondId, bondSpec.params.anchorPosition));
    }

    builderParseAndApplyCharge(mol, newAtomId, mol.atoms[newAtomId].text);

    return newAtomId;
}

static void builderProcessBranchChain(Molecule& mol, const AstChainNode& chain, int& lastAtomId, int& atomIndex, float currentAngle);
static void builderProcessChain(Molecule& mol, const AstChainNode& chain, int& lastAtomId, int& atomIndex, float currentAngle);
static void builderProcessRing(Molecule& mol, const AstRingNode& ringNode, int& atomIndex);

static void builderProcessBranchChain(Molecule& mol, const AstChainNode& chain, int& lastAtomId, int& atomIndex, float currentAngle) {
    float lastAngle = currentAngle;
    int lastBranchAtomId = -1;

    for (const auto& elem : chain.elements) {
        if (elem.isRing) {
            if (elem.ringNode) {
                builderProcessRing(mol, *elem.ringNode, atomIndex);
                if (!mol.rings.empty()) {
                    const Ring& lastRing = mol.rings.back();
                    if (!lastRing.atomIndices.empty()) {
                        lastAtomId = lastRing.atomIndices.back();
                    }
                }
            }
            lastBranchAtomId = -1;
            continue;
        }

        if (elem.isBranch) {
            if (elem.branchChain) {
                int savedLastAtom = lastAtomId;
                float savedAngle = lastAngle;
                builderProcessBranchChain(mol, *elem.branchChain, savedLastAtom, atomIndex, savedAngle);
                if (savedLastAtom >= 0 && savedLastAtom < static_cast<int>(mol.atoms.size())) {
                    lastAtomId = savedLastAtom;
                }
            }
            continue;
        }

        bool hasExplicitBond = elem.bondSpec.hasExplicitBond;

        if (!hasExplicitBond && !elem.atomLabel.empty()) {
            if (lastBranchAtomId >= 0 && lastBranchAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastBranchAtomId].text += elem.atomLabel.label;
                builderParseAndApplyCharge(mol, lastBranchAtomId, mol.atoms[lastBranchAtomId].text);
            } else if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastAtomId].text += elem.atomLabel.label;
                builderParseAndApplyCharge(mol, lastAtomId, mol.atoms[lastAtomId].text);
            }
        } else if (hasExplicitBond) {
            float bondAngle = builderResolveBondAngle(elem.bondSpec.params, lastAngle);
            int newAtomId = builderAddAtomAndBond(mol, lastAtomId, elem.atomLabel, elem.bondSpec, bondAngle, atomIndex);
            if (newAtomId >= 0) {
                lastAtomId = newAtomId;
                lastAngle = bondAngle;
            }
            lastBranchAtomId = -1;
        }

        if (!elem.anchorName.empty()) {
            mol.anchors.push_back(Anchor(elem.anchorName, lastAtomId));
        }

        if (!elem.hookName.empty()) {
            mol.hooks.push_back(Hook(elem.hookName, lastAtomId));
        }
    }
}

static void builderProcessChain(Molecule& mol, const AstChainNode& chain, int& lastAtomId, int& atomIndex, float currentAngle) {
    ChemPoint startPos(0, 0);
    if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
        startPos = mol.atoms[lastAtomId].position;
    }

    if (atomIndex == 0 && lastAtomId < 0) {
        lastAtomId = mol.addAtom(startPos, chain.initialAtom.label);
        atomIndex = 1;
    } else {
        if (!chain.initialAtom.label.empty() && lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
            mol.atoms[lastAtomId].text = chain.initialAtom.label;
        }
    }

    builderParseAndApplyCharge(mol, lastAtomId, mol.atoms[lastAtomId].text);

    if (!chain.initialAnchorName.empty()) {
        mol.anchors.push_back(Anchor(chain.initialAnchorName, lastAtomId));
    }

    if (!chain.initialHookName.empty()) {
        mol.hooks.push_back(Hook(chain.initialHookName, lastAtomId));
    }

    float lastAngle = currentAngle;
    int lastBranchAtomId = -1;

    for (const auto& elem : chain.elements) {
        if (elem.isRing) {
            if (elem.ringNode) {
                builderProcessRing(mol, *elem.ringNode, atomIndex);
                if (!mol.rings.empty()) {
                    const Ring& lastRing = mol.rings.back();
                    if (!lastRing.atomIndices.empty()) {
                        lastAtomId = lastRing.atomIndices.back();
                    }
                }
            }
            lastBranchAtomId = -1;
            continue;
        }

        if (elem.isBranch) {
            if (elem.branchChain) {
                int savedLastAtom = lastAtomId;
                float savedAngle = lastAngle;
                int branchAtomIndex = atomIndex;
                builderProcessBranchChain(mol, *elem.branchChain, savedLastAtom, branchAtomIndex, savedAngle);
                if (savedLastAtom >= 0 && savedLastAtom < static_cast<int>(mol.atoms.size())) {
                    lastBranchAtomId = savedLastAtom;
                    atomIndex = branchAtomIndex;
                }
            }
            continue;
        }

        bool hasExplicitBond = elem.bondSpec.hasExplicitBond;

        if (!hasExplicitBond && !elem.atomLabel.empty()) {
            if (lastBranchAtomId >= 0 && lastBranchAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastBranchAtomId].text += elem.atomLabel.label;
                builderParseAndApplyCharge(mol, lastBranchAtomId, mol.atoms[lastBranchAtomId].text);
            } else if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                mol.atoms[lastAtomId].text += elem.atomLabel.label;
                builderParseAndApplyCharge(mol, lastAtomId, mol.atoms[lastAtomId].text);
            }
        } else if (hasExplicitBond) {
            float bondAngle = builderResolveBondAngle(elem.bondSpec.params, lastAngle);
            int newAtomId = builderAddAtomAndBond(mol, lastAtomId, elem.atomLabel, elem.bondSpec, bondAngle, atomIndex);
            if (newAtomId >= 0) {
                lastAtomId = newAtomId;
                lastAngle = bondAngle;
            }
            lastBranchAtomId = -1;
        }

        if (!elem.anchorName.empty()) {
            mol.anchors.push_back(Anchor(elem.anchorName, lastAtomId));
        }

        if (!elem.hookName.empty()) {
            mol.hooks.push_back(Hook(elem.hookName, lastAtomId));
        }
    }

    mol.calculateBounds();
}

static void builderProcessRing(Molecule& mol, const AstRingNode& ringNode, int& atomIndex) {
    Ring ring;
    ring.sides = ringNode.sides;
    ring.hasInnerCircle = ringNode.hasInnerCircle;
    ring.innerStartAngle = ringNode.innerStartAngle;
    ring.innerEndAngle = ringNode.innerEndAngle;
    ring.radius = calculateRingRadius(ringNode.sides);

    bool isChained = (ringNode.sharedFromAtomIdx >= 0 && ringNode.sharedToAtomIdx < 0);
    bool isFused = (ringNode.sharedFromAtomIdx >= 0 && ringNode.sharedToAtomIdx >= 0);

    if (isFused) {
        if (ringNode.sharedFromAtomIdx >= static_cast<int>(mol.atoms.size()) ||
            ringNode.sharedToAtomIdx >= static_cast<int>(mol.atoms.size())) {
            return;
        }
        ChemPoint fromPos = mol.atoms[ringNode.sharedFromAtomIdx].position;
        ChemPoint toPos = mol.atoms[ringNode.sharedToAtomIdx].position;
        ChemPoint midPoint((fromPos.x + toPos.x) / 2, (fromPos.y + toPos.y) / 2);
        ChemPoint edgeDir = toPos - fromPos;
        float edgeLen = edgeDir.length();
        if (edgeLen < EPSILON) return;

        float angleStep = CHEM_TWO_PI / ringNode.sides;
        float halfStep = angleStep / 2.0f;
        float subRadius = edgeLen / (2.0f * std::sin(halfStep));
        ChemPoint outNormal = ChemPoint(-edgeDir.y, edgeDir.x).normalized();
        ChemPoint subCenter = midPoint + outNormal * (subRadius * std::cos(halfStep));

        ring.center = subCenter;
        ring.radius = subRadius;

        float fromAngle = std::atan2(-(fromPos.y - subCenter.y), fromPos.x - subCenter.x);
        ring.atomIndices.push_back(ringNode.sharedFromAtomIdx);

        for (int i = 1; i < ringNode.sides - 1; i++) {
            float angle = fromAngle + i * angleStep;
            ChemPoint pos(subCenter.x + subRadius * std::cos(angle),
                         subCenter.y - subRadius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
        }
        ring.atomIndices.push_back(ringNode.sharedToAtomIdx);
    } else if (isChained) {
        if (ringNode.sharedFromAtomIdx >= static_cast<int>(mol.atoms.size())) {
            return;
        }
        ChemPoint firstPos = mol.atoms[ringNode.sharedFromAtomIdx].position;
        float angleStep = CHEM_TWO_PI / ringNode.sides;
        float startAngle;
        if (ringNode.sides == 3) {
            startAngle = CHEM_PI;
        } else {
            startAngle = -(CHEM_PI - CHEM_PI / ringNode.sides);
        }

        ChemPoint centerOffset(ring.radius * std::cos(startAngle),
                              -ring.radius * std::sin(startAngle));
        ChemPoint center(firstPos.x - centerOffset.x, firstPos.y - centerOffset.y);
        ring.center = center;

        ring.atomIndices.push_back(ringNode.sharedFromAtomIdx);
        for (int i = 1; i < ringNode.sides; i++) {
            float angle = startAngle + i * angleStep;
            ChemPoint pos(center.x + ring.radius * std::cos(angle),
                         center.y - ring.radius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
        }
    } else {
        float angleStep = CHEM_TWO_PI / ringNode.sides;
        float startAngle = -(CHEM_PI - CHEM_PI / ringNode.sides);

        float cx = 0, cy = 0;
        for (int i = 0; i < ringNode.sides; i++) {
            float angle = startAngle + i * angleStep;
            ChemPoint pos(ring.radius * std::cos(angle), -ring.radius * std::sin(angle));
            ring.atomIndices.push_back(mol.addAtom(pos));
            cx += pos.x;
            cy += pos.y;
        }
        ring.center = ChemPoint(cx / ringNode.sides, cy / ringNode.sides);
    }

    int currentRingIndex = mol.rings.size();
    mol.rings.push_back(ring);

    for (int i = 0; i < static_cast<int>(ringNode.bondSpecs.size()) && i < ringNode.sides; i++) {
        const AstRingBondSpec& ringBond = ringNode.bondSpecs[i];

        if (isFused && i == ringNode.sides - 1) {
            if (ringBond.isFusedRing && ringBond.fusedRing) {
                mol.rings[currentRingIndex].bondTypes.push_back(ringBond.fusedBondType);
                bool hasSharedBond = false;
                for (const auto& b : mol.bonds) {
                    if ((b.fromAtom == ringNode.sharedFromAtomIdx && b.toAtom == ringNode.sharedToAtomIdx) ||
                        (b.fromAtom == ringNode.sharedToAtomIdx && b.toAtom == ringNode.sharedFromAtomIdx)) {
                        hasSharedBond = true;
                        break;
                    }
                }
                if (!hasSharedBond) {
                    mol.addBond(ringNode.sharedToAtomIdx, ringNode.sharedFromAtomIdx, ringBond.fusedBondType, BondParams(), currentRingIndex);
                }
                builderProcessRing(mol, *ringBond.fusedRing, atomIndex);
            }
            continue;
        }

        if (i < static_cast<int>(ringNode.bondSpecs.size()) - 1 ||
            (!isFused && i == ringNode.sides - 1)) {
            int fromAtomIdx = ring.atomIndices[i];
            int toAtomIdx = ring.atomIndices[(i + 1) % ringNode.sides];

            if (fromAtomIdx >= 0 && fromAtomIdx < static_cast<int>(mol.atoms.size()) &&
                toAtomIdx >= 0 && toAtomIdx < static_cast<int>(mol.atoms.size())) {
                mol.rings[currentRingIndex].bondTypes.push_back(ringBond.bond.type);
                mol.addBond(fromAtomIdx, toAtomIdx, ringBond.bond.type, ringBond.bond.params, currentRingIndex);
            }

            int targetAtom = toAtomIdx;
            if (!ringBond.atomLabel.empty() && targetAtom >= 0 && targetAtom < static_cast<int>(mol.atoms.size())) {
                mol.atoms[targetAtom].text = ringBond.atomLabel.label;
                builderParseAndApplyCharge(mol, targetAtom, mol.atoms[targetAtom].text);
            }

            if (!ringBond.hookName.empty() && targetAtom >= 0) {
                mol.hooks.push_back(Hook(ringBond.hookName, targetAtom));
            }

            if (!ringBond.anchorName.empty() && targetAtom >= 0) {
                mol.anchors.push_back(Anchor(ringBond.anchorName, targetAtom));
            }

            if (!ringBond.branches.empty()) {
                for (const auto& branch : ringBond.branches) {
                    if (branch.chain && targetAtom >= 0 && targetAtom < static_cast<int>(mol.atoms.size())) {
                        int fromIdx = targetAtom;
                        float branchAngle;
                        if (ringNode.sides == 3) {
                            float angleStep = CHEM_TWO_PI / ringNode.sides;
                            float startAngle = CHEM_PI;
                            float atomAngle = startAngle + i * angleStep;
                            branchAngle = atomAngle + CHEM_PI / 2;
                        } else {
                            branchAngle = std::atan2(-(mol.atoms[fromIdx].position.y - ring.center.y),
                                                    mol.atoms[fromIdx].position.x - ring.center.x);
                        }
                        int branchAtomIdx = targetAtom;
                        int branchAtomIndex = atomIndex;
                        builderProcessBranchChain(mol, *branch.chain, branchAtomIdx, branchAtomIndex, branchAngle);
                    }
                }
            }
        }
    }

    for (int i = 0; i < static_cast<int>(ringNode.bondSpecs.size()); i++) {
        const AstRingBondSpec& ringBond = ringNode.bondSpecs[i];
        if (i < static_cast<int>(ringNode.bondSpecs.size()) - 1 || !isFused) {
            if (ringBond.atomLabel.empty()) {
                int targetAtom = ring.atomIndices[(i + 1) % ringNode.sides];
                if (targetAtom >= 0 && targetAtom < static_cast<int>(mol.atoms.size())) {
                    if (!ringBond.hookName.empty()) {
                        mol.hooks.push_back(Hook(ringBond.hookName, targetAtom));
                    }
                    if (!ringBond.anchorName.empty()) {
                        mol.anchors.push_back(Anchor(ringBond.anchorName, targetAtom));
                    }
                    if (!ringBond.branches.empty()) {
                        for (const auto& branch : ringBond.branches) {
                            if (branch.chain && targetAtom >= 0 && targetAtom < static_cast<int>(mol.atoms.size())) {
                                int fromIdx = targetAtom;
                                float branchAngle;
                                if (ringNode.sides == 3) {
                                    float angleStep = CHEM_TWO_PI / ringNode.sides;
                                    float startAngle = CHEM_PI;
                                    float atomAngle = startAngle + i * angleStep;
                                    branchAngle = atomAngle + CHEM_PI / 2;
                                } else {
                                    branchAngle = std::atan2(-(mol.atoms[fromIdx].position.y - ring.center.y),
                                                            mol.atoms[fromIdx].position.x - ring.center.x);
                                }
                                int branchAtomIdx = targetAtom;
                                int branchAtomIndex = atomIndex;
                                builderProcessBranchChain(mol, *branch.chain, branchAtomIdx, branchAtomIndex, branchAngle);
                            }
                        }
                    }
                }
            }
        }
    }

    atomIndex += ringNode.sides - (isFused ? 2 : (isChained ? 1 : 0));
    mol.calculateBounds();
}

static void builderProcessMultiRingConnections(Molecule& mol, const ParseAST& ast) {
    for (const auto& conn : ast.multiRingConnections) {
        if (conn.rightmostAtomInFirst >= 0 && conn.rightmostAtomInFirst < static_cast<int>(mol.atoms.size()) &&
            conn.leftmostAtomInSecond >= 0 && conn.leftmostAtomInSecond < static_cast<int>(mol.atoms.size())) {
            ChemPoint fromPos = mol.atoms[conn.rightmostAtomInFirst].position;
            ChemPoint toPos = mol.atoms[conn.leftmostAtomInSecond].position;
            float bondLen = DEFAULT_BOND_LENGTH;
            ChemPoint offset(fromPos.x + bondLen - toPos.x, fromPos.y - toPos.y);

            size_t secondRingIdx = static_cast<size_t>(conn.secondRingIdx);
            if (secondRingIdx < mol.rings.size()) {
                for (int idx : mol.rings[secondRingIdx].atomIndices) {
                    if (idx >= 0 && idx < static_cast<int>(mol.atoms.size())) {
                        mol.atoms[idx].position.x += offset.x;
                        mol.atoms[idx].position.y += offset.y;
                    }
                }
                mol.rings[secondRingIdx].center.x += offset.x;
                mol.rings[secondRingIdx].center.y += offset.y;
            }

            mol.addBond(conn.rightmostAtomInFirst, conn.leftmostAtomInSecond, BOND_SINGLE, BondParams());
        }
    }
}

static void builderResolveHooks(Molecule& mol, const ParseAST& ast) {
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

static void builderResolveAnchors(Molecule& mol, const ParseAST& ast) {
    for (const auto& anchor : ast.anchors) {
        bool exists = false;
        for (const auto& existing : mol.anchors) {
            if (existing.name == anchor.name) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            mol.anchors.push_back(Anchor(anchor.name, anchor.atomIndexRef));
        }
    }
}

static void builderResolveCurves(Molecule& mol, const ParseAST& ast) {
    for (const auto& curve : ast.curves) {
        CurvePath cp;
        cp.fromName = curve.fromName;
        cp.toName = curve.toName;
        cp.controlPoints = curve.controlPoints;
        cp.hasArrow = curve.hasArrow;
        cp.lineWidth = curve.lineWidth;
        cp.shortenStart = curve.shortenStart;
        cp.shortenEnd = curve.shortenEnd;

        for (const auto& anchor : mol.anchors) {
            if (anchor.name == curve.fromName) {
                cp.fromAtom = anchor.atomIndex;
            }
            if (anchor.name == curve.toName) {
                cp.toAtom = anchor.atomIndex;
            }
        }

        mol.curves.push_back(cp);
    }
}

Molecule MoleculeBuilder::build(const ParseAST& ast) {
    Molecule mol;
    mol.atoms.clear();
    mol.bonds.clear();
    mol.rings.clear();
    mol.hooks.clear();
    mol.anchors.clear();
    mol.curves.clear();
    mol.maxAtomWidth = 0.0f;

    int atomIndex = 0;
    int lastAtomId = -1;
    float currentAngle = 0.0f;

    for (const auto& chain : ast.topLevelChains) {
        if (chain) {
            builderProcessChain(mol, *chain, lastAtomId, atomIndex, currentAngle);
            if (!mol.rings.empty()) {
                const Ring& lastRing = mol.rings.back();
                if (!lastRing.atomIndices.empty()) {
                    lastAtomId = lastRing.atomIndices.back();
                    if (lastAtomId >= 0 && lastAtomId < static_cast<int>(mol.atoms.size())) {
                        currentAngle = std::atan2(-(mol.atoms[lastAtomId].position.y - lastRing.center.y),
                                                 mol.atoms[lastAtomId].position.x - lastRing.center.x);
                    }
                }
            }
        }
    }

    for (const auto& ringNode : ast.rings) {
        builderProcessRing(mol, ringNode, atomIndex);
    }

    builderProcessMultiRingConnections(mol, ast);

    builderResolveHooks(mol, ast);

    mol.calculateBounds();

    for (const auto& bond : mol.bonds) {
        if (bond.fromAtom < 0 || bond.fromAtom >= static_cast<int>(mol.atoms.size()) ||
            bond.toAtom < 0 || bond.toAtom >= static_cast<int>(mol.atoms.size())) {
            return mol;
        }
    }
    for (const auto& ring : mol.rings) {
        for (int idx : ring.atomIndices) {
            if (idx < 0 || idx >= static_cast<int>(mol.atoms.size())) {
                return mol;
            }
        }
    }

    builderResolveAnchors(mol, ast);
    builderResolveCurves(mol, ast);

    mol.calculateBounds();

    return mol;
}

std::wstring MoleculeBuilder::extractPureAtomText(const std::wstring& label) {
    return builderExtractPureAtomText(label);
}

void MoleculeBuilder::parseAndApplyChargeToAtom(Molecule& mol, int atomIndex, std::wstring& label) {
    builderParseAndApplyCharge(mol, atomIndex, label);
}

} // namespace tex

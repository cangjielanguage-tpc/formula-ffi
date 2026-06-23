#ifndef CHEMFIG_AST_H_INCLUDED
#define CHEMFIG_AST_H_INCLUDED

#include "chemfig_types.h"
#include <vector>
#include <string>
#include <memory>

namespace tex {

struct AstBondSpec {
    BondType type;
    BondParams params;
    bool hasExplicitBond;

    AstBondSpec() : type(BOND_SINGLE), hasExplicitBond(false) {}
};

struct AstAtomSpec {
    std::wstring label;
    std::vector<Charge> charges;

    bool empty() const { return label.empty() && charges.empty(); }
};

struct AstRingBondSpec {
    AstBondSpec bond;
    AstAtomSpec atomLabel;
    std::wstring anchorName;
    std::wstring hookName;

    struct BranchSpec {
        std::shared_ptr<struct AstChainNode> chain;
    };
    std::vector<BranchSpec> branches;
    bool isFusedRing;
    std::shared_ptr<struct AstRingNode> fusedRing;
    BondType fusedBondType;

    AstRingBondSpec() : isFusedRing(false), fusedBondType(BOND_SINGLE) {}
};

struct AstRingNode {
    int sides;
    bool hasInnerCircle;
    float innerStartAngle;
    float innerEndAngle;
    int sharedFromAtomIdx;
    int sharedToAtomIdx;
    std::vector<AstRingBondSpec> bondSpecs;

    AstRingNode() : sides(6), hasInnerCircle(false), innerStartAngle(0), innerEndAngle(360),
                     sharedFromAtomIdx(-1), sharedToAtomIdx(-1) {}
};

struct AstChainNode;

struct AstChainElement {
    bool isRing;
    bool isBranch;
    AstBondSpec bondSpec;
    AstAtomSpec atomLabel;
    std::wstring anchorName;
    std::wstring hookName;
    std::shared_ptr<AstRingNode> ringNode;
    std::shared_ptr<AstChainNode> branchChain;

    AstChainElement() : isRing(false), isBranch(false) {}
};

struct AstChainNode {
    AstAtomSpec initialAtom;
    std::wstring initialAnchorName;
    std::wstring initialHookName;
    std::vector<AstChainElement> elements;
};

struct AstHookSpec {
    std::wstring name;
    int atomIndexRef;
};

struct AstAnchorSpec {
    std::wstring name;
    int atomIndexRef;
};

struct AstCurveNode {
    std::wstring fromName;
    std::wstring toName;
    std::vector<CurveControlPoint> controlPoints;
    bool hasArrow;
    float lineWidth;
    float shortenStart;
    float shortenEnd;

    AstCurveNode() : hasArrow(true), lineWidth(1.0f), shortenStart(0.0f), shortenEnd(0.0f) {}
};

struct MultiRingConnection {
    int firstRingIdx;
    int secondRingIdx;
    int rightmostAtomInFirst;
    int leftmostAtomInSecond;
};

struct ParseAST {
    std::vector<AstAtomSpec> atomSpecs;
    std::vector<AstRingNode> rings;
    std::vector<AstHookSpec> hooks;
    std::vector<AstAnchorSpec> anchors;
    std::vector<AstCurveNode> curves;
    std::vector<std::shared_ptr<AstChainNode>> topLevelChains;
    std::vector<MultiRingConnection> multiRingConnections;

    bool isValid;

    ParseAST() : isValid(true) {}
};

} // namespace tex

#endif // CHEMFIG_AST_H_INCLUDED

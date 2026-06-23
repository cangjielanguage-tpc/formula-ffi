#ifndef SCHEME_TYPES_H_INCLUDED
#define SCHEME_TYPES_H_INCLUDED

#include "chemfig_types.h"
#include "scheme_config.h"
#include <vector>
#include <string>
#include <map>

namespace tex {

enum ArrowType {
    ARROW_FORWARD = 0,
    ARROW_BACKWARD = 1,
    ARROW_BIDIRECTIONAL = 2,
    ARROW_EQUILIBRIUM = 3,
    ARROW_LONG_EQUILIB = 4,
    ARROW_ALT_EQUILIB = 5,
    ARROW_HARP_RIGHT = 6,
    ARROW_HARP_LEFT = 7,
    ARROW_FISHHOOK = 8,
    ARROW_INVISIBLE = 9,
    ARROW_CURVED_FORWARD = 10,
    ARROW_CURVED_BACKWARD = 11,
    ARROW_CURVED_BIDIR = 12,
    ARROW_HARPOON_RIGHT = 13,
    ARROW_HARPOON_LEFT = 14,
    ARROW_DASHED_FORWARD = 15,
    ARROW_DASHED_EQUILIBRIUM = 16,
    ARROW_ARC_FORWARD = 17,
    ARROW_ARC_BACKWARD = 18,
    ARROW_ARC_BIDIR = 19
};

enum MergeDirection {
    MERGE_RIGHT = 0,
    MERGE_LEFT = 1,
    MERGE_UP = 2,
    MERGE_DOWN = 3
};

struct ArrowAnchor {
    std::wstring compoundRef;
    std::wstring anchorName;

    bool isDefault() const {
        return compoundRef.empty() && anchorName.empty();
    }
};

struct ArrowRef {
    std::wstring compoundRef;
    std::wstring atomRef;
    std::wstring anchorName;

    bool isDefault() const {
        return compoundRef.empty() && atomRef.empty() && anchorName.empty();
    }
};

enum DashPattern {
    DASH_NONE,
    DASH_DASHED,
    DASH_DOTTED,
    DASH_DENSELY_DASHED,
    DASH_LOOSELY_DASHED
};

constexpr float ARROW_PARAM_UNSET = -999.0f;

struct ArrowParams {
    ArrowType type;
    float angle;
    float lengthCoeff;
    float curveHeight;
    float yShift;
    std::wstring yShiftRaw;
    std::wstring labelAbove;
    std::wstring labelBelow;
    std::wstring tikzStyle;
    std::wstring color;
    bool dashed;
    DashPattern dashPattern;
    ArrowRef fromRef;
    ArrowRef toRef;
    ArrowAnchor fromAnchor;
    ArrowAnchor toAnchor;

    ArrowParams()
        : type(ARROW_FORWARD),
          angle(ARROW_PARAM_UNSET),
          lengthCoeff(ARROW_PARAM_UNSET),
          curveHeight(0.0f),
          yShift(0.0f),
          dashed(false),
          dashPattern(DASH_NONE) {}

    bool isCurved() const {
        return type == ARROW_CURVED_FORWARD ||
               type == ARROW_CURVED_BACKWARD ||
               type == ARROW_CURVED_BIDIR ||
               type == ARROW_ARC_FORWARD ||
               type == ARROW_ARC_BACKWARD ||
               type == ARROW_ARC_BIDIR ||
               curveHeight != 0.0f;
    }

    float effectiveCurveHeight() const {
        if (curveHeight != 0.0f) return curveHeight;
        if (type == ARROW_ARC_FORWARD || type == ARROW_ARC_BACKWARD || type == ARROW_ARC_BIDIR) {
            return SchemeConfig::instance().defaultCurveHeight;
        }
        return curveHeight;
    }

    bool isDashed() const {
        return dashed || type == ARROW_DASHED_FORWARD || type == ARROW_DASHED_EQUILIBRIUM;
    }
};

struct CompoundInfo {
    Molecule molecule;
    std::wstring name;
    std::wstring refName;
    int number;
    std::map<std::wstring, int> atomAnchors;

    CompoundInfo() : number(0) {}
};

struct ArrowElement {
    ArrowParams params;
    int fromCompound;
    int toCompound;
    int fromSubschemeIdx;
    int toSubschemeIdx;

    ArrowElement() : fromCompound(-1), toCompound(-1), fromSubschemeIdx(-1), toSubschemeIdx(-1) {}
};

struct PlusElement {
    int afterCompound;
    std::wstring sepLeftRaw;
    std::wstring sepRightRaw;
    std::wstring vshiftRaw;
    float sepLeft;
    float sepRight;
    float vshift;
    bool hasCustomSep;

    PlusElement() : afterCompound(-1), sepLeft(0), sepRight(0), vshift(0), hasCustomSep(false) {}
};

struct MergeSource {
    int compoundIndex;
    std::wstring anchorName;
    std::wstring compoundRef;

    MergeSource() : compoundIndex(-1) {}
};

struct MergeTarget {
    int compoundIndex;
    std::wstring anchorName;
    std::wstring compoundRef;
    std::wstring style;

    MergeTarget() : compoundIndex(-1) {}
};

struct MergeGeometry {
    float segmentCoeff;
    float arrowCoeff;
    float startPosition;
    std::wstring style;

    MergeGeometry()
        : segmentCoeff(0.5f),
          arrowCoeff(0.5f),
          startPosition(0.5f) {}
};

struct MergeElement {
    MergeDirection direction;
    std::vector<MergeSource> sources;
    MergeTarget target;
    MergeGeometry geometry;
    std::wstring labelAbove;
    std::wstring labelBelow;
    int afterCompound;

    MergeElement() : direction(MERGE_RIGHT), afterCompound(-1) {}
};

struct CompoundAnchorPos {
    ChemPoint center;
    ChemPoint north;
    ChemPoint south;
    ChemPoint east;
    ChemPoint west;
    ChemPoint northEast;
    ChemPoint northWest;
    ChemPoint southEast;
    ChemPoint southWest;
    ChemPoint midEast;
    ChemPoint midWest;
    ChemPoint midNorth;
    ChemPoint midSouth;
    ChemPoint base;
    ChemPoint baseEast;
    ChemPoint baseWest;
    float width;
    float height;

    ChemPoint getAnchor(const std::wstring& name) const;
};

enum ElementKind {
    ELEM_COMPOUND,
    ELEM_ARROW,
    ELEM_PLUS,
    ELEM_MERGE,
    ELEM_LINEBREAK,
    ELEM_SUBSCHEME
};

struct SubschemeInfo {
    int startCompound;
    int endCompound;
    std::wstring refName;
    int firstCompoundIndex;
    std::vector<int> internalArrows;
    int startArrow;
    int endArrow;
    std::wstring leftDelim;
    std::wstring rightDelim;

    SubschemeInfo() : startCompound(-1), endCompound(-1), firstCompoundIndex(-1), startArrow(-1), endArrow(-1) {}
};

struct ReactionScheme {
    std::vector<CompoundInfo> compounds;
    std::vector<ArrowElement> arrows;
    std::vector<PlusElement> pluses;
    std::vector<MergeElement> merges;
    std::vector<SubschemeInfo> subschemes;
    std::vector<std::pair<ElementKind, int>> elementOrder;
    std::vector<CurvePath> curves;

    std::map<std::wstring, int> compoundRefs;

    float minX, maxX, minY, maxY;

    ReactionScheme() : minX(0), maxX(0), minY(0), maxY(0) {}

    void calculateBounds() {
        if (compounds.empty()) {
            minX = maxX = minY = maxY = 0;
            return;
        }
        const auto& first = compounds[0].molecule;
        minX = first.minX;
        maxX = first.maxX;
        minY = first.minY;
        maxY = first.maxY;
        for (size_t i = 1; i < compounds.size(); i++) {
            const auto& c = compounds[i].molecule;
            if (c.minX < minX) minX = c.minX;
            if (c.maxX > maxX) maxX = c.maxX;
            if (c.minY < minY) minY = c.minY;
            if (c.maxY > maxY) maxY = c.maxY;
        }
    }

    int compoundCount() const { return static_cast<int>(compounds.size()); }
    int arrowCount() const { return static_cast<int>(arrows.size()); }

    int findCompound(const std::wstring& ref) const {
        auto it = compoundRefs.find(ref);
        return it != compoundRefs.end() ? it->second : -1;
    }

    ChemPoint getAtomPosition(int compoundIdx, const std::wstring& atomRef) const {
        if (compoundIdx < 0 || compoundIdx >= compoundCount()) return ChemPoint();
        const auto& info = compounds[compoundIdx];
        auto it = info.atomAnchors.find(atomRef);
        if (it != info.atomAnchors.end() && it->second >= 0 &&
            it->second < static_cast<int>(info.molecule.atoms.size())) {
            return info.molecule.atoms[it->second].position;
        }
        return info.molecule.center();
    }

    static CompoundAnchorPos calculateCompoundAnchors(
        float x, float y, float width, float height, float boxHeight, float boxDepth);
};

inline bool parseNumericAnchor(const std::wstring& name, float& outAngle) {
    if (name.empty()) return false;
    bool numeric = true;
    bool hasDot = false;
    size_t start = 0;
    if (name[0] == L'-' || name[0] == L'+') { start = 1; }
    if (start >= name.size()) return false;
    for (size_t i = start; i < name.size() && numeric; i++) {
        if (name[i] == L'.') {
            if (hasDot) numeric = false;
            hasDot = true;
        } else if (name[i] < L'0' || name[i] > L'9') {
            numeric = false;
        }
    }
    if (numeric && name.size() > start) {
        outAngle = chemfig::safeStof(name);
        return true;
    }
    return false;
}

inline ChemPoint CompoundAnchorPos::getAnchor(const std::wstring& name) const {
    if (name.empty()) return center;
    switch (name[0]) {
        case L'n':
            if (name == L"n" || name == L"north") return north;
            if (name == L"ne" || name == L"north east") return northEast;
            if (name == L"nw" || name == L"north west") return northWest;
            break;
        case L's':
            if (name == L"s" || name == L"south") return south;
            if (name == L"se" || name == L"south east") return southEast;
            if (name == L"sw" || name == L"south west") return southWest;
            break;
        case L'e':
            if (name == L"e" || name == L"east") return east;
            break;
        case L'w':
            if (name == L"w" || name == L"west") return west;
            break;
        case L'c':
            if (name == L"c" || name == L"center") return center;
            break;
        case L'm':
            if (name == L"me" || name == L"mid east") return midEast;
            if (name == L"mw" || name == L"mid west") return midWest;
            if (name == L"mn" || name == L"mid north") return midNorth;
            if (name == L"ms" || name == L"mid south") return midSouth;
            break;
        case L'b':
            if (name == L"b" || name == L"base") {
                return base;
            }
            if (name == L"be" || name == L"base east") {
                return baseEast;
            }
            if (name == L"bw" || name == L"base west") {
                return baseWest;
            }
            break;
        case L'1':
            if (name == L"180" || name == L"180.0") return west;
            break;
        case L'0':
            if (name == L"0" || name == L"0.0") return east;
            break;
        case L'9':
            if (name == L"90" || name == L"90.0") return north;
            break;
        case L'2':
            if (name == L"270" || name == L"270.0") return south;
            if (name == L"225" || name == L"225.0") return southWest;
            break;
        case L'4':
            if (name == L"45" || name == L"45.0") return northEast;
            break;
        case L'3':
            if (name == L"315" || name == L"315.0") return southEast;
            if (name == L"135" || name == L"135.0") return northWest;
            break;
        case L'-':
            if (name == L"-45" || name == L"-45.0") return southEast;
            if (name == L"-135" || name == L"-135.0") return southWest;
            break;
    }

    float angle = 0.0f;
    if (parseNumericAnchor(name, angle)) {
        float rad = chemfig::degToRad(angle);
        float halfW = width * 0.5f;
        float halfH = height * 0.5f;
        float dx = std::cos(rad);
        float dy = -std::sin(rad);
        float tX = (std::abs(dx) > chemfig::EPSILON) ? halfW / std::abs(dx) : 1e8f;
        float tY = (std::abs(dy) > chemfig::EPSILON) ? halfH / std::abs(dy) : 1e8f;
        float t = std::min(tX, tY);
        return ChemPoint(center.x + dx * t, center.y + dy * t);
    }
    return center;
}

inline CompoundAnchorPos ReactionScheme::calculateCompoundAnchors(
    float x, float y, float width, float height, float boxHeight, float boxDepth) {
    CompoundAnchorPos pos;
    pos.width = width;
    pos.height = height;
    pos.center = ChemPoint(x + width * 0.5f, y + (boxDepth - boxHeight) * 0.5f);
    pos.north = ChemPoint(pos.center.x, y - boxHeight);
    pos.south = ChemPoint(pos.center.x, y + boxDepth);
    pos.east = ChemPoint(x + width, pos.center.y);
    pos.west = ChemPoint(x, pos.center.y);
    pos.northEast = ChemPoint(x + width, y - boxHeight);
    pos.northWest = ChemPoint(x, y - boxHeight);
    pos.southEast = ChemPoint(x + width, y + boxDepth);
    pos.southWest = ChemPoint(x, y + boxDepth);
    pos.midEast = ChemPoint(x + width, pos.center.y);
    pos.midWest = ChemPoint(x, pos.center.y);
    pos.midNorth = ChemPoint(pos.center.x, y - boxHeight * 0.5f);
    pos.midSouth = ChemPoint(pos.center.x, y + boxDepth * 0.5f);
    pos.base = ChemPoint(pos.center.x, y + boxDepth);
    pos.baseEast = ChemPoint(x + width, y + boxDepth);
    pos.baseWest = ChemPoint(x, y + boxDepth);
    return pos;
}

} // namespace tex

#endif // SCHEME_TYPES_H_INCLUDED

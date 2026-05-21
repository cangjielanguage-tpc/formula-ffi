#ifndef CHEMFIG_TYPES_H_INCLUDED
#define CHEMFIG_TYPES_H_INCLUDED

#include "chemfig_constants.h"
#include "common.h"
#include <vector>
#include <string>
#include <algorithm>

namespace tex {

enum BondType {
    BOND_SINGLE = 1,
    BOND_DOUBLE = 2,
    BOND_TRIPLE = 3,
    BOND_AROMATIC = 4,
    BOND_WEDGE_SOLID_UP = 5,
    BOND_WEDGE_SOLID_DOWN = 6,
    BOND_WEDGE_DOTTED_UP = 7,
    BOND_WEDGE_DOTTED_DOWN = 8,
    BOND_WEDGE_HOLLOW_UP = 9,
    BOND_WEDGE_HOLLOW_DOWN = 10
};

struct ChemPoint {
    float x;
    float y;

    ChemPoint() : x(0), y(0) {}
    ChemPoint(float px, float py) : x(px), y(py) {}

    ChemPoint operator+(const ChemPoint& other) const {
        return ChemPoint(x + other.x, y + other.y);
    }
    ChemPoint operator-(const ChemPoint& other) const {
        return ChemPoint(x - other.x, y - other.y);
    }
    ChemPoint operator*(float s) const {
        return ChemPoint(x * s, y * s);
    }
    ChemPoint operator/(float s) const {
        if (std::abs(s) < chemfig::EPSILON) return ChemPoint(0, 0);
        return ChemPoint(x / s, y / s);
    }
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    ChemPoint normalized() const {
        float len = length();
        if (len < chemfig::EPSILON) return ChemPoint(0, 0);
        return ChemPoint(x / len, y / len);
    }
    ChemPoint perpendicular() const {
        return ChemPoint(-y, x);
    }
    float dot(const ChemPoint& other) const {
        return x * other.x + y * other.y;
    }
};

struct BondParams {
    float angle;
    float lengthCoeff;
    int fromAtomNum;
    int toAtomNum;
    std::wstring tikzStyle;
    bool hasAngle;
    bool isRelativeAngle;
    bool hasOffset;
    float offsetStart;
    float offsetEnd;
    std::wstring anchorName;
    float anchorPosition;

    BondParams() : angle(0), lengthCoeff(1.0f), fromAtomNum(-1), toAtomNum(-1),
                   hasAngle(false), isRelativeAngle(false),
                   hasOffset(false), offsetStart(0), offsetEnd(0), anchorPosition(-1.0f) {}
};

struct Charge {
    float angle;
    float distance;
    std::wstring mark;
    bool isScriptStyle;

    Charge() : angle(0), distance(0), isScriptStyle(false) {}
    Charge(float ang, float dist, const std::wstring& m, bool script = false) : angle(ang), distance(dist), mark(m), isScriptStyle(script) {}
};

struct AtomNode {
    ChemPoint position;
    std::wstring text;
    int number;
    std::vector<Charge> charges;

    AtomNode() : number(-1) {}
    AtomNode(const ChemPoint& pos, const std::wstring& txt, int num = -1)
        : position(pos), text(txt), number(num) {}
};

struct Bond {
    BondType type;
    BondParams params;
    int fromAtom;
    int toAtom;
    int ringIndex;
    bool isHook;

    Bond() : type(BOND_SINGLE), fromAtom(-1), toAtom(-1), ringIndex(-1), isHook(false) {}
};

struct Ring {
    int sides;
    std::vector<int> atomIndices;
    std::vector<BondType> bondTypes;
    bool hasInnerCircle;
    float innerStartAngle;
    float innerEndAngle;
    ChemPoint center;
    float radius;

    Ring() : sides(6), hasInnerCircle(false), innerStartAngle(0), innerEndAngle(360), radius(1.0f) {}
};

struct Hook {
    std::wstring name;
    int atomIndex;
    BondType bondType;
    BondParams params;

    Hook() : atomIndex(-1), bondType(BOND_SINGLE) {}
    Hook(const std::wstring& n, int idx, BondType bt = BOND_SINGLE, const BondParams& bp = BondParams())
        : name(n), atomIndex(idx), bondType(bt), params(bp) {}
};

struct Anchor {
    std::wstring name;
    int atomIndex;
    int bondIndex;
    float bondPosition;

    Anchor() : atomIndex(-1), bondIndex(-1), bondPosition(-1.0f) {}
    Anchor(const std::wstring& n, int idx, int bondIdx = -1, float pos = -1.0f)
        : name(n), atomIndex(idx), bondIndex(bondIdx), bondPosition(pos) {}
};

struct CurvePoint {
    float angle;
    float distance;

    CurvePoint() : angle(0), distance(0) {}
    CurvePoint(float ang, float dist) : angle(ang), distance(dist) {}
};

struct CurveControlPoint {
    CurvePoint point;
    bool relative;
    bool toEnd;

    CurveControlPoint() : relative(true), toEnd(false) {}
    CurveControlPoint(float angle, float dist, bool rel, bool toEndPt)
        : point(angle, dist), relative(rel), toEnd(toEndPt) {}
};

struct CurvePath {
    std::wstring fromName;
    std::wstring toName;
    int fromAtom;
    int toAtom;
    std::vector<CurveControlPoint> controlPoints;
    bool hasArrow;
    float lineWidth;
    float shortenStart;
    float shortenEnd;

    CurvePath() : fromAtom(-1), toAtom(-1), hasArrow(true), lineWidth(1.0f),
                  shortenStart(0.0f), shortenEnd(0.0f) {}
};

struct Molecule {
    std::vector<AtomNode> atoms;
    std::vector<Bond> bonds;
    std::vector<Ring> rings;
    std::vector<Hook> hooks;
    std::vector<Anchor> anchors;
    std::vector<CurvePath> curves;

    float minX, maxX, minY, maxY;
    float maxAtomWidth;

    Molecule() : minX(0), maxX(0), minY(0), maxY(0), maxAtomWidth(0) {}

    void calculateBounds() {
        if (atoms.empty()) {
            minX = maxX = minY = maxY = 0;
            return;
        }
        minX = maxX = atoms[0].position.x;
        minY = maxY = atoms[0].position.y;
        for (const auto& a : atoms) {
            if (a.position.x < minX) minX = a.position.x;
            if (a.position.x > maxX) maxX = a.position.x;
            if (a.position.y < minY) minY = a.position.y;
            if (a.position.y > maxY) maxY = a.position.y;
        }
    }

    float width() const { return maxX - minX; }
    float height() const { return maxY - minY; }

    ChemPoint center() const {
        return ChemPoint((minX + maxX) / 2, (minY + maxY) / 2);
    }

    int addAtom(const ChemPoint& pos, const std::wstring& text = L"", int number = -1) {
        int id = static_cast<int>(atoms.size());
        atoms.push_back(AtomNode(pos, text, number));
        return id;
    }

    int addBond(int from, int to, BondType type, const BondParams& params = BondParams(), int ringIdx = -1, bool isHook = false) {
        if (from < 0 || from >= static_cast<int>(atoms.size()) ||
            to < 0 || to >= static_cast<int>(atoms.size())) {
            return -1;
        }
        int id = static_cast<int>(bonds.size());
        Bond bond;
        bond.fromAtom = from;
        bond.toAtom = to;
        bond.type = type;
        bond.params = params;
        bond.ringIndex = ringIdx;
        bond.isHook = isHook;
        bonds.push_back(bond);
        return id;
    }

    static float calculateAtomWidth(const std::wstring& atomLabel) {
        if (atomLabel.empty()) return 1.0f;

        float width = 0.0f;
        bool inSubscript = false;
        bool inSuperscript = false;

        for (wchar_t ch : atomLabel) {
            if (ch == L'_') {
                inSubscript = true;
                continue;
            } else if (ch == L'^') {
                inSuperscript = true;
                continue;
            } else if (ch == L'{' || ch == L'}' || ch == L'|') {
                if (ch == L'|') {
                    inSubscript = false;
                    inSuperscript = false;
                }
                continue;
            }

            if (inSubscript || inSuperscript) {
                width += chemfig::SUBSCRIPT_SCALE;
            } else {
                if (ch >= L'A' && ch <= L'Z') width += 1.0f;
                else if (ch >= L'a' && ch <= L'z') width += 0.8f;
                else if (ch >= L'0' && ch <= L'9') width += 0.6f;
                else width += 0.8f;
            }
        }

        return std::max(1.0f, width);
    }

    void updateMaxAtomWidth(const std::wstring& atomLabel) {
        float w = calculateAtomWidth(atomLabel);
        if (w > maxAtomWidth) maxAtomWidth = w;
    }

    void normalizeBondLengths() {
        calculateBounds();
    }

    bool isValidAtomIndex(int idx) const {
        return idx >= 0 && idx < static_cast<int>(atoms.size());
    }
};

} // namespace tex

#endif // CHEMFIG_TYPES_H_INCLUDED

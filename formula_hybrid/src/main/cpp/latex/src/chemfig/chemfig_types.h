#ifndef CHEMFIG_TYPES_H_INCLUDED
#define CHEMFIG_TYPES_H_INCLUDED

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
    float length() const {
        return std::sqrt(x * x + y * y);
    }
    ChemPoint normalized() const {
        float len = length();
        if (len < 0.0001f) return ChemPoint(0, 0);
        return ChemPoint(x / len, y / len);
    }
    ChemPoint perpendicular() const {
        return ChemPoint(-y, x);
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

    BondParams() : angle(0), lengthCoeff(1.0f), fromAtomNum(-1), toAtomNum(-1),
                   hasAngle(false), isRelativeAngle(false),
                   hasOffset(false), offsetStart(0), offsetEnd(0) {}
};

struct AtomNode {
    ChemPoint position;
    std::wstring text;
    int number;

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

    Anchor() : atomIndex(-1) {}
    Anchor(const std::wstring& n, int idx) : name(n), atomIndex(idx) {}
};

struct Molecule {
    std::vector<AtomNode> atoms;
    std::vector<Bond> bonds;
    std::vector<Ring> rings;
    std::vector<Hook> hooks;
    std::vector<Anchor> anchors;
    
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
        int id = atoms.size();
        atoms.push_back(AtomNode(pos, text, number));
        return id;
    }
    
    int addBond(int from, int to, BondType type, const BondParams& params = BondParams(), int ringIdx = -1, bool isHook = false) {
        if (from < 0 || from >= static_cast<int>(atoms.size()) ||
            to < 0 || to >= static_cast<int>(atoms.size())) {
            return -1;
        }
        int id = bonds.size();
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
            } else if (ch == L'{' || ch == L'}') {
                continue;
            }
            
            // 更精确的字符宽度计算，接近TeX Live chemfig的行为
            if (inSubscript || inSuperscript) {
                // 下标/上标字符宽度约为正常字符的0.7倍
                width += 0.7f;
            } else {
                // 正常字符宽度，考虑不同字符的实际宽度差异
                if (ch >= L'A' && ch <= L'Z') width += 1.0f;  // 大写字母
                else if (ch >= L'a' && ch <= L'z') width += 0.8f; // 小写字母
                else if (ch >= L'0' && ch <= L'9') width += 0.6f; // 数字
                else width += 0.8f; // 其他字符
            }
        }
        
        // 最小原子宽度为1.0，与TeX Live chemfig一致
        return std::max(1.0f, width);
    }

    void updateMaxAtomWidth(const std::wstring& atomLabel) {
        float width = calculateAtomWidth(atomLabel);
        if (width > maxAtomWidth) {
            maxAtomWidth = width;
        }
    }

    void normalizeBondLengths() {
        // 与TeX Live chemfig一致的键长统一逻辑
        // 即使maxAtomWidth <= 1.0f，也要确保所有键长一致
        float scaleFactor = 1.0f;
        
        // 更精确的缩放因子计算，接近TeX Live chemfig的行为
        if (maxAtomWidth > 1.0f) {
            // 使用更平滑的缩放曲线，避免过度放大
            scaleFactor = 1.0f + (maxAtomWidth - 1.0f) * 0.25f;
        }
        
        // 统一所有键的lengthCoeff
        for (auto& bond : bonds) {
            bond.params.lengthCoeff = scaleFactor;
        }
        
        calculateBounds();
    }
};

} // namespace tex

#endif // CHEMFIG_TYPES_H_INCLUDED

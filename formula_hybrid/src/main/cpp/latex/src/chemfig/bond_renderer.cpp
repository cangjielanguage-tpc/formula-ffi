#include "bond_renderer.h"
#include <cmath>
#include <algorithm>

namespace tex {

namespace {
    constexpr float DOUBLE_BOND_OFFSET = 0.1f;
    constexpr float TRIPLE_BOND_OFFSET = 0.4f;
    constexpr float WEDGE_WIDTH = 0.18f;
    constexpr float DEFAULT_BOND_WIDTH_PT = 0.8f;
    constexpr float EPSILON = 0.001f;

    struct TikzStyle {
        float lineWidth;
        bool hasLineWidth;
        color lineColor;
        bool hasColor;
        bool isDashed;

        TikzStyle() : lineWidth(0), hasLineWidth(false), lineColor(black), hasColor(false), isDashed(false) {}
    };

    bool isWordBoundary(const std::wstring& s, size_t pos, size_t len) {
        return pos + len >= s.length() || s[pos + len] == L',' || s[pos + len] == L' ';
    }

    TikzStyle parseTikzStyle(const std::wstring& style) {
        TikzStyle result;
        if (style.empty()) return result;

        size_t pos = 0;
        while (pos < style.length()) {
            while (pos < style.length() && (style[pos] == L' ' || style[pos] == L',')) pos++;
            if (pos >= style.length()) break;

            if (style.compare(pos, 10, L"line width") == 0) {
                pos += 10;
                while (pos < style.length() && style[pos] == L' ') pos++;
                if (pos < style.length() && style[pos] == L'=') pos++;
                while (pos < style.length() && style[pos] == L' ') pos++;
                std::wstring numStr;
                while (pos < style.length() && ((style[pos] >= L'0' && style[pos] <= L'9') || style[pos] == L'.')) {
                    numStr += style[pos++];
                }
                if (!numStr.empty()) {
                    try {
                        result.lineWidth = std::stof(std::string(numStr.begin(), numStr.end())) / DEFAULT_BOND_WIDTH_PT;
                        result.hasLineWidth = true;
                    } catch (...) {}
                }
                while (pos < style.length() && style[pos] != L',') pos++;
            } else if (style.compare(pos, 6, L"dashed") == 0 && isWordBoundary(style, pos, 6)) {
                result.isDashed = true;
                pos += 6;
            } else if (style.compare(pos, 5, L"green") == 0 && isWordBoundary(style, pos, 5)) {
                result.lineColor = 0xFF00FF00;
                result.hasColor = true;
                pos += 5;
            } else if (style.compare(pos, 4, L"dash") == 0 && isWordBoundary(style, pos, 4)) {
                result.isDashed = true;
                pos += 4;
            } else if (style.compare(pos, 4, L"blue") == 0 && isWordBoundary(style, pos, 4)) {
                result.lineColor = 0xFF0000FF;
                result.hasColor = true;
                pos += 4;
            } else if (style.compare(pos, 4, L"thin") == 0 && isWordBoundary(style, pos, 4)) {
                result.lineWidth = 0.5f / DEFAULT_BOND_WIDTH_PT;
                result.hasLineWidth = true;
                pos += 4;
            } else if (style.compare(pos, 4, L"bold") == 0 && isWordBoundary(style, pos, 4)) {
                result.lineWidth = 2.0f / DEFAULT_BOND_WIDTH_PT;
                result.hasLineWidth = true;
                pos += 4;
            } else if (style.compare(pos, 5, L"thick") == 0 && isWordBoundary(style, pos, 5)) {
                result.lineWidth = 1.5f / DEFAULT_BOND_WIDTH_PT;
                result.hasLineWidth = true;
                pos += 5;
            } else if (style.compare(pos, 3, L"red") == 0 && isWordBoundary(style, pos, 3)) {
                result.lineColor = 0xFFFF0000;
                result.hasColor = true;
                pos += 3;
            } else if (style.compare(pos, 6, L"hollow") == 0 && isWordBoundary(style, pos, 6)) {
                pos += 6;
            } else {
                while (pos < style.length() && style[pos] != L',') pos++;
            }
        }
        return result;
    }

    struct BondGeometry {
        ChemPoint dir;
        ChemPoint perp;
        float len;

        BondGeometry(const ChemPoint& from, const ChemPoint& to)
            : dir((to - from).normalized()), perp(dir.perpendicular()), len((to - from).length()) {}
    };
}

void BondRenderer::drawDashedLine(Graphics2D& g2, float x1, float y1, float x2, float y2, float scale) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < EPSILON) return;

    float dashLen = 0.12f * scale;
    float gapLen = 0.08f * scale;
    float segmentLen = dashLen + gapLen;

    int numSegments = std::max(1, static_cast<int>(len / segmentLen));
    float dirX = dx / len;
    float dirY = dy / len;

    for (int i = 0; i < numSegments; i++) {
        float startT = i * segmentLen;
        float endT = std::min(startT + dashLen, len);
        g2.drawLine(x1 + dirX * startT, y1 + dirY * startT,
                    x1 + dirX * endT, y1 + dirY * endT);
    }
}

void BondRenderer::drawSingle(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    g2.drawLine(from.x, from.y, to.x, to.y);
}

void BondRenderer::drawDouble(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale,
                              const ChemPoint& ringCenter, bool inRing) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float offset = DOUBLE_BOND_OFFSET * scale;

    if (inRing) {
        ChemPoint mid = (from + to) * 0.5f;
        ChemPoint toCenter = ringCenter - mid;
        if (bg.perp.x * toCenter.x + bg.perp.y * toCenter.y < 0) bg.perp = bg.perp * (-1);

        g2.drawLine(from.x, from.y, to.x, to.y);

        float perpDotCenter = bg.perp.x * toCenter.x + bg.perp.y * toCenter.y;
        bool isTriangle = (perpDotCenter < 0);
        float shortenRatio = isTriangle ? 0.618f : 0.75f;
        float actualShorten = bg.len * (1.0f - shortenRatio) * 0.5f;

        ChemPoint lineOffset = bg.perp * offset * 2.0f;
        ChemPoint innerFrom = from + lineOffset + bg.dir * actualShorten;
        ChemPoint innerTo = to + lineOffset - bg.dir * actualShorten;
        g2.drawLine(innerFrom.x, innerFrom.y, innerTo.x, innerTo.y);
    } else {
        ChemPoint off1 = bg.perp * offset;
        ChemPoint off2 = bg.perp * (-offset);
        ChemPoint f1 = from + off1, t1 = to + off1;
        ChemPoint f2 = from + off2, t2 = to + off2;
        g2.drawLine(f1.x, f1.y, t1.x, t1.y);
        g2.drawLine(f2.x, f2.y, t2.x, t2.y);
    }
}

void BondRenderer::drawTriple(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float offset = TRIPLE_BOND_OFFSET * scale;
    float halfOffset = offset * 0.5f;
    g2.drawLine(from.x, from.y, to.x, to.y);

    ChemPoint off1 = bg.perp * halfOffset;
    ChemPoint off2 = bg.perp * (-halfOffset);
    ChemPoint f1 = from + off1, t1 = to + off1;
    ChemPoint f2 = from + off2, t2 = to + off2;
    g2.drawLine(f1.x, f1.y, t1.x, t1.y);
    g2.drawLine(f2.x, f2.y, t2.x, t2.y);
}

void BondRenderer::drawWedgeHollow(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up) {
    if (up) {
        drawWedgeHollowUp(g2, from, to, scale);
    } else {
        drawWedgeHollowDown(g2, from, to, scale);
    }
}

void BondRenderer::drawWedgeHollowUp(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float wideW = WEDGE_WIDTH * scale;
    float narrowW = WEDGE_WIDTH * scale * 0.2f;

    ChemPoint base1 = from + bg.perp * wideW;
    ChemPoint base2 = from - bg.perp * wideW;
    ChemPoint tip1 = to + bg.perp * narrowW;
    ChemPoint tip2 = to - bg.perp * narrowW;

    g2.drawLine(base1.x, base1.y, base2.x, base2.y);
    g2.drawLine(base2.x, base2.y, tip2.x, tip2.y);
    g2.drawLine(tip2.x, tip2.y, tip1.x, tip1.y);
    g2.drawLine(tip1.x, tip1.y, base1.x, base1.y);
}

void BondRenderer::drawWedgeHollowDown(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float wideW = WEDGE_WIDTH * scale;
    float narrowW = WEDGE_WIDTH * scale * 0.2f;

    ChemPoint base1 = from + bg.perp * narrowW;
    ChemPoint base2 = from - bg.perp * narrowW;
    ChemPoint tip1 = to + bg.perp * wideW;
    ChemPoint tip2 = to - bg.perp * wideW;

    g2.drawLine(base1.x, base1.y, base2.x, base2.y);
    g2.drawLine(base2.x, base2.y, tip2.x, tip2.y);
    g2.drawLine(tip2.x, tip2.y, tip1.x, tip1.y);
    g2.drawLine(tip1.x, tip1.y, base1.x, base1.y);
}

void BondRenderer::drawWedgeDotted(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    ChemPoint delta = to - from;
    constexpr int NUM_DASHES = 5;
    for (int i = 0; i <= NUM_DASHES; i++) {
        float t = static_cast<float>(i) / NUM_DASHES;
        ChemPoint p = from + delta * t;
        float w = WEDGE_WIDTH * scale * (up ? (1 - t) : t );
        ChemPoint p1 = p + bg.perp * w;
        ChemPoint p2 = p - bg.perp * w;
        g2.drawLine(p1.x, p1.y, p2.x, p2.y);
    }
}

void BondRenderer::drawWedgeSolid(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up) {
    if (up) {
        drawWedgeSolidUp(g2, from, to, scale);
    } else {
        drawWedgeSolidDown(g2, from, to, scale);
    }
}

void BondRenderer::drawWedgeSolidUp(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float wideW = WEDGE_WIDTH * scale;
    float narrowW = WEDGE_WIDTH * scale * 0.2f;

    ChemPoint base1 = from + bg.perp * wideW;
    ChemPoint base2 = from - bg.perp * wideW;
    ChemPoint tip1 = to + bg.perp * narrowW;
    ChemPoint tip2 = to - bg.perp * narrowW;

    ChemPoint delta = to - from;
    int steps = std::max(2, static_cast<int>(bg.len / (0.02f * scale)));
    for (int i = 0; i <= steps; i++) {
        float t = static_cast<float>(i) / steps;
        float w = wideW + (narrowW - wideW) * t;
        ChemPoint p = from + delta * t;
        ChemPoint p1 = p + bg.perp * w;
        ChemPoint p2 = p - bg.perp * w;
        g2.drawLine(p1.x, p1.y, p2.x, p2.y);
    }
}

void BondRenderer::drawWedgeSolidDown(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale) {
    BondGeometry bg(from, to);
    if (bg.len < EPSILON) return;

    float wideW = WEDGE_WIDTH * scale;
    float narrowW = WEDGE_WIDTH * scale * 0.2f;

    ChemPoint base1 = from + bg.perp * narrowW;
    ChemPoint base2 = from - bg.perp * narrowW;
    ChemPoint tip1 = to + bg.perp * wideW;
    ChemPoint tip2 = to - bg.perp * wideW;

    ChemPoint delta = to - from;
    int steps = std::max(2, static_cast<int>(bg.len / (0.02f * scale)));
    for (int i = 0; i <= steps; i++) {
        float t = static_cast<float>(i) / steps;
        float w = narrowW + (wideW - narrowW) * t;
        ChemPoint p = from + delta * t;
        ChemPoint p1 = p + bg.perp * w;
        ChemPoint p2 = p - bg.perp * w;
        g2.drawLine(p1.x, p1.y, p2.x, p2.y);
    }
}

void BondRenderer::drawBond(Graphics2D& g2, BondType type,
                            const ChemPoint& from, const ChemPoint& to,
                            float scale, const BondParams& params,
                            const ChemPoint& ringCenter, bool inRing) {
    TikzStyle ts = parseTikzStyle(params.tikzStyle);

    const Stroke& oldStroke = g2.getStroke();
    color oldColor = g2.getColor();

    if (ts.hasLineWidth) {
        g2.setStroke(Stroke(ts.lineWidth * oldStroke.lineWidth, oldStroke.cap, oldStroke.join));
    }
    if (ts.hasColor) {
        g2.setColor(ts.lineColor);
    }

    switch (type) {
        case BOND_SINGLE:
            if (ts.isDashed) {
                drawDashedLine(g2, from.x, from.y, to.x, to.y, scale);
            } else {
                drawSingle(g2, from, to, scale);
            }
            break;
        case BOND_DOUBLE:
            drawDouble(g2, from, to, scale, ringCenter, inRing);
            break;
        case BOND_TRIPLE:
            drawTriple(g2, from, to, scale);
            break;
        case BOND_WEDGE_HOLLOW_UP:
            drawWedgeHollowUp(g2, from, to, scale);
            break;
        case BOND_WEDGE_HOLLOW_DOWN:
            drawWedgeHollowDown(g2, from, to, scale);
            break;
        case BOND_WEDGE_DOTTED_UP:
            drawWedgeDotted(g2, from, to, scale, true);
            break;
        case BOND_WEDGE_DOTTED_DOWN:
            drawWedgeDotted(g2, from, to, scale, false);
            break;
        case BOND_WEDGE_SOLID_UP:
            drawWedgeSolid(g2, from, to, scale, true);
            break;
        case BOND_WEDGE_SOLID_DOWN:
            drawWedgeSolid(g2, from, to, scale, false);
            break;
        case BOND_AROMATIC:
        default:
            drawSingle(g2, from, to, scale);
            break;
    }

    g2.setStroke(oldStroke);
    if (ts.hasColor) {
        g2.setColor(oldColor);
    }
}

} // namespace tex

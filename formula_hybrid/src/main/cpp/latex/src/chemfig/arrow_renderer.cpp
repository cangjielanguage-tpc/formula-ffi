#include "arrow_renderer.h"
#include "chemfig_constants.h"
#include "scheme_config.h"
#include <cmath>
#include <algorithm>
#include <map>

namespace tex {

namespace {
    using namespace chemfig;

    struct DirInfo {
        float dx, dy, len, dirX, dirY;
        ChemPoint perp;

        DirInfo(const ChemPoint& from, const ChemPoint& to)
            : dx(to.x - from.x), dy(to.y - from.y),
              len(std::sqrt(dx * dx + dy * dy)),
              dirX(len > EPSILON ? dx / len : 0.0f),
              dirY(len > EPSILON ? dy / len : 0.0f),
              perp(-dirY, dirX) {}

        bool valid() const { return len > EPSILON; }
    };

    color parseColorName(const std::wstring& name) {
        static std::map<std::wstring, color> colorMap = {
            {L"red", red},
            {L"blue", blue},
            {L"green", green},
            {L"yellow", yellow},
            {L"black", black},
            {L"white", white},
            {L"cyan", cyan},
            {L"magenta", magenta},
            {L"orange", rgb(255, 165, 0)},
            {L"purple", rgb(128, 0, 128)},
            {L"brown", rgb(165, 42, 42)},
            {L"gray", rgb(128, 128, 128)},
            {L"grey", rgb(128, 128, 128)},
            {L"pink", rgb(255, 192, 203)},
            {L"violet", rgb(238, 130, 238)},
            {L"olive", rgb(128, 128, 0)},
            {L"teal", rgb(0, 128, 128)},
            {L"lime", rgb(0, 255, 0)},
            {L"darkgray", rgb(64, 64, 64)},
            {L"lightgray", rgb(192, 192, 192)},
            {L"darkblue", rgb(0, 0, 139)},
            {L"darkgreen", rgb(0, 100, 0)},
            {L"darkred", rgb(139, 0, 0)}
        };
        
        auto it = colorMap.find(name);
        if (it != colorMap.end()) {
            return it->second;
        }
        return trans;
    }
}

ChemPoint ArrowRenderer::computeCurveControlPoint(const ChemPoint& from,
                                                   const ChemPoint& to,
                                                   float curveHeight) {
    DirInfo d(from, to);
    if (!d.valid()) return ChemPoint((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);
    ChemPoint mid((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);
    return ChemPoint(mid.x + d.perp.x * curveHeight * d.len,
                     mid.y + d.perp.y * curveHeight * d.len);
}

void ArrowRenderer::drawArrowHead(Graphics2D& g2, const ChemPoint& tip,
                                  const ChemPoint& ref, float scale,
                                  const ArrowStyle& style) {
    DirInfo d(ref, tip);
    if (!d.valid()) return;

    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;

    ChemPoint base(tip.x - d.dirX * hl, tip.y - d.dirY * hl);
    g2.drawLine(tip.x, tip.y, base.x + d.perp.x * hw, base.y + d.perp.y * hw);
    g2.drawLine(tip.x, tip.y, base.x - d.perp.x * hw, base.y - d.perp.y * hw);
}

void ArrowRenderer::drawHarpoonHead(Graphics2D& g2, const ChemPoint& tip,
                                    const ChemPoint& ref, float scale,
                                    const ArrowStyle& style, bool upper) {
    DirInfo d(ref, tip);
    if (!d.valid()) return;

    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;
    float sign = upper ? 1.0f : -1.0f;

    ChemPoint base(tip.x - d.dirX * hl, tip.y - d.dirY * hl);
    g2.drawLine(tip.x, tip.y, base.x + d.perp.x * hw * sign, base.y + d.perp.y * hw * sign);
}

void ArrowRenderer::drawMergeArrowHead(Graphics2D& g2, const ChemPoint& tip,
                                       const ChemPoint& from, float scale,
                                       float headLength, float headWidth) {
    DirInfo d(from, tip);
    if (!d.valid()) return;

    float hl = headLength * scale;
    float hw = headWidth * scale;

    ChemPoint base(tip.x - d.dirX * hl, tip.y - d.dirY * hl);
    g2.drawLine(tip.x, tip.y, base.x + d.perp.x * hw, base.y + d.perp.y * hw);
    g2.drawLine(tip.x, tip.y, base.x - d.perp.x * hw, base.y - d.perp.y * hw);
}

void ArrowRenderer::drawForward(Graphics2D& g2, const ChemPoint& from,
                                const ChemPoint& to, float scale,
                                const ArrowStyle& style) {
    g2.drawLine(from.x, from.y, to.x, to.y);
    drawArrowHead(g2, to, from, scale, style);
}

void ArrowRenderer::drawBackward(Graphics2D& g2, const ChemPoint& from,
                                 const ChemPoint& to, float scale,
                                 const ArrowStyle& style) {
    g2.drawLine(from.x, from.y, to.x, to.y);
    drawArrowHead(g2, from, to, scale, style);
}

void ArrowRenderer::drawBidirectional(Graphics2D& g2, const ChemPoint& from,
                                      const ChemPoint& to, float scale,
                                      const ArrowStyle& style) {
    g2.drawLine(from.x, from.y, to.x, to.y);
    drawArrowHead(g2, to, from, scale, style);
    drawArrowHead(g2, from, to, scale, style);
}

void ArrowRenderer::drawEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                    const ChemPoint& to, float scale,
                                    const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 3.0f;
    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;
    float margin = hl * 0.8f;

    ChemPoint innerFrom(from.x + d.dirX * margin, from.y + d.dirY * margin);
    ChemPoint innerTo(to.x - d.dirX * margin, to.y - d.dirY * margin);

    ChemPoint upperFrom(innerFrom.x - d.perp.x * offset, innerFrom.y - d.perp.y * offset);
    ChemPoint upperTo(innerTo.x - d.perp.x * offset, innerTo.y - d.perp.y * offset);
    ChemPoint lowerFrom(innerFrom.x + d.perp.x * offset, innerFrom.y + d.perp.y * offset);
    ChemPoint lowerTo(innerTo.x + d.perp.x * offset, innerTo.y + d.perp.y * offset);

    g2.drawLine(upperFrom.x, upperFrom.y, upperTo.x, upperTo.y);
    g2.drawLine(lowerFrom.x, lowerFrom.y, lowerTo.x, lowerTo.y);

    ChemPoint base1(upperTo.x - d.dirX * hl, upperTo.y - d.dirY * hl);
    g2.drawLine(upperTo.x, upperTo.y, base1.x - d.perp.x * hw, base1.y - d.perp.y * hw);

    ChemPoint base2(lowerFrom.x + d.dirX * hl, lowerFrom.y + d.dirY * hl);
    g2.drawLine(lowerFrom.x, lowerFrom.y, base2.x + d.perp.x * hw, base2.y + d.perp.y * hw);
}

void ArrowRenderer::drawLongEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                        const ChemPoint& to, float scale,
                                        const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 3.0f;
    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;
    float margin = hl * 0.8f;

    ChemPoint lowerFrom(from.x + d.perp.x * offset, from.y + d.perp.y * offset);
    ChemPoint lowerTo(to.x + d.perp.x * offset, to.y + d.perp.y * offset);

    float longLen = d.len - 2.0f * margin;
    float shortLen = longLen / 2.0f;
    float shortStart = (longLen - shortLen) * 0.5f;
    ChemPoint upperFrom(from.x + d.dirX * (margin + shortStart) - d.perp.x * offset,
                        from.y + d.dirY * (margin + shortStart) - d.perp.y * offset);
    ChemPoint upperTo(upperFrom.x + d.dirX * shortLen, upperFrom.y + d.dirY * shortLen);

    g2.drawLine(upperFrom.x, upperFrom.y, upperTo.x, upperTo.y);
    g2.drawLine(lowerFrom.x + d.dirX * margin, lowerFrom.y + d.dirY * margin,
                lowerTo.x - d.dirX * margin, lowerTo.y - d.dirY * margin);

    ChemPoint base1(upperTo.x - d.dirX * hl, upperTo.y - d.dirY * hl);
    g2.drawLine(upperTo.x, upperTo.y, base1.x - d.perp.x * hw, base1.y - d.perp.y * hw);

    ChemPoint lowerMarginFrom(lowerFrom.x + d.dirX * margin, lowerFrom.y + d.dirY * margin);
    ChemPoint base2(lowerMarginFrom.x + d.dirX * hl, lowerMarginFrom.y + d.dirY * hl);
    g2.drawLine(lowerMarginFrom.x, lowerMarginFrom.y, base2.x + d.perp.x * hw, base2.y + d.perp.y * hw);
}

void ArrowRenderer::drawAltEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                       const ChemPoint& to, float scale,
                                       const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 3.0f;
    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;
    float margin = hl * 0.8f;

    ChemPoint upperFrom(from.x - d.perp.x * offset, from.y - d.perp.y * offset);
    ChemPoint upperTo(to.x - d.perp.x * offset, to.y - d.perp.y * offset);

    float longLen = d.len - 2.0f * margin;
    float shortLen = longLen / 2.0f;
    float shortStart = (longLen - shortLen) * 0.5f;
    ChemPoint lowerFrom(from.x + d.dirX * (margin + shortStart) + d.perp.x * offset,
                        from.y + d.dirY * (margin + shortStart) + d.perp.y * offset);
    ChemPoint lowerTo(lowerFrom.x + d.dirX * shortLen, lowerFrom.y + d.dirY * shortLen);

    g2.drawLine(upperFrom.x + d.dirX * margin, upperFrom.y + d.dirY * margin,
                upperTo.x - d.dirX * margin, upperTo.y - d.dirY * margin);
    g2.drawLine(lowerFrom.x, lowerFrom.y, lowerTo.x, lowerTo.y);

    ChemPoint upperMarginTo(upperTo.x - d.dirX * margin, upperTo.y - d.dirY * margin);
    ChemPoint base1(upperMarginTo.x - d.dirX * hl, upperMarginTo.y - d.dirY * hl);
    g2.drawLine(upperMarginTo.x, upperMarginTo.y, base1.x - d.perp.x * hw, base1.y - d.perp.y * hw);

    ChemPoint base2(lowerFrom.x + d.dirX * hl, lowerFrom.y + d.dirY * hl);
    g2.drawLine(lowerFrom.x, lowerFrom.y, base2.x + d.perp.x * hw, base2.y + d.perp.y * hw);
}

void ArrowRenderer::drawHarpRight(Graphics2D& g2, const ChemPoint& from,
                                  const ChemPoint& to, float scale,
                                  const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float hl = style.headLength * scale;
    float margin = hl * 1.2f;

    g2.drawLine(from.x, from.y, to.x, to.y);

    float slashLen = style.doubleBondOffset * scale * 3.0f;
    float slashSpacing = slashLen * 0.24f;
    float slashAngle = 45.0f * CHEM_PI / 180.0f;
    float cosA = std::cos(slashAngle);
    float sinA = std::sin(slashAngle);

    ChemPoint innerFrom(from.x + d.dirX * margin, from.y + d.dirY * margin);
    ChemPoint innerTo(to.x - d.dirX * margin, to.y - d.dirY * margin);
    ChemPoint mid((innerFrom.x + innerTo.x) * 0.5f, (innerFrom.y + innerTo.y) * 0.5f);

    float sx = d.perp.x * cosA - d.dirX * sinA;
    float sy = d.perp.y * cosA - d.dirY * sinA;
    float slen = std::sqrt(sx * sx + sy * sy);
    if (slen > EPSILON) { sx /= slen; sy /= slen; }

    float offset = slashSpacing * 0.5f;
    for (int i = -1; i <= 1; i += 2) {
        ChemPoint sCenter(mid.x + d.dirX * offset * i, mid.y + d.dirY * offset * i);
        g2.drawLine(sCenter.x - sx * slashLen * 0.5f, sCenter.y - sy * slashLen * 0.5f,
                    sCenter.x + sx * slashLen * 0.5f, sCenter.y + sy * slashLen * 0.5f);
    }

    drawArrowHead(g2, to, from, scale, style);
}

void ArrowRenderer::drawHarpLeft(Graphics2D& g2, const ChemPoint& from,
                                 const ChemPoint& to, float scale,
                                 const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float hl = style.headLength * scale;
    float margin = hl * 1.2f;

    g2.drawLine(from.x, from.y, to.x, to.y);

    float slashLen = style.doubleBondOffset * scale * 3.0f;
    float slashSpacing = slashLen * 0.24f;
    float slashAngle = 45.0f * CHEM_PI / 180.0f;
    float cosA = std::cos(slashAngle);
    float sinA = std::sin(slashAngle);

    ChemPoint innerFrom(from.x + d.dirX * margin, from.y + d.dirY * margin);
    ChemPoint innerTo(to.x - d.dirX * margin, to.y - d.dirY * margin);
    ChemPoint mid((innerFrom.x + innerTo.x) * 0.5f, (innerFrom.y + innerTo.y) * 0.5f);

    float sx = d.perp.x * cosA - d.dirX * sinA;
    float sy = d.perp.y * cosA - d.dirY * sinA;
    float slen = std::sqrt(sx * sx + sy * sy);
    if (slen > EPSILON) { sx /= slen; sy /= slen; }

    float offset = slashSpacing * 0.5f;
    for (int i = -1; i <= 1; i += 2) {
        ChemPoint sCenter(mid.x + d.dirX * offset * i, mid.y + d.dirY * offset * i);
        g2.drawLine(sCenter.x - sx * slashLen * 0.5f, sCenter.y - sy * slashLen * 0.5f,
                    sCenter.x + sx * slashLen * 0.5f, sCenter.y + sy * slashLen * 0.5f);
    }

    drawArrowHead(g2, from, to, scale, style);
}

void ArrowRenderer::drawFishhook(Graphics2D& g2, const ChemPoint& from,
                                 const ChemPoint& to, float scale,
                                 const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float r = style.harpRadius * scale;
    ChemPoint mid((from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f);
    ChemPoint bend1(mid.x + d.perp.x * r, mid.y + d.perp.y * r);
    ChemPoint bend2(mid.x - d.perp.x * r, mid.y - d.perp.y * r);

    g2.drawLine(from.x, from.y, bend1.x, bend1.y);
    g2.drawLine(bend1.x, bend1.y, to.x, to.y);
    g2.drawLine(from.x, from.y, bend2.x, bend2.y);
    g2.drawLine(bend2.x, bend2.y, to.x, to.y);
}

void ArrowRenderer::drawInvisible(Graphics2D& g2, const ChemPoint&,
                                  const ChemPoint&, float,
                                  const ArrowStyle&) {}

void ArrowRenderer::drawHarpoonRight(Graphics2D& g2, const ChemPoint& from,
                                     const ChemPoint& to, float scale,
                                     const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 6.0f;
    ChemPoint upperFrom(from.x + d.perp.x * offset, from.y + d.perp.y * offset);
    ChemPoint upperTo(to.x + d.perp.x * offset, to.y + d.perp.y * offset);

    g2.drawLine(upperFrom.x, upperFrom.y, upperTo.x, upperTo.y);
    drawHarpoonHead(g2, upperTo, upperFrom, scale, style, true);
}

void ArrowRenderer::drawHarpoonLeft(Graphics2D& g2, const ChemPoint& from,
                                    const ChemPoint& to, float scale,
                                    const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 6.0f;
    ChemPoint lowerFrom(from.x - d.perp.x * offset, from.y - d.perp.y * offset);
    ChemPoint lowerTo(to.x - d.perp.x * offset, to.y - d.perp.y * offset);

    g2.drawLine(lowerFrom.x, lowerFrom.y, lowerTo.x, lowerTo.y);
    drawHarpoonHead(g2, lowerTo, lowerFrom, scale, style, false);
}

void ArrowRenderer::drawDashedLine(Graphics2D& g2, const ChemPoint& from,
                                   const ChemPoint& to, float dashLen, float gapLen) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float pos = 0.0f;
    bool drawing = true;

    while (pos < d.len) {
        float segLen = drawing ? dashLen : gapLen;
        float endPos = std::min(pos + segLen, d.len);

        if (drawing) {
            g2.drawLine(from.x + d.dirX * pos, from.y + d.dirY * pos,
                        from.x + d.dirX * endPos, from.y + d.dirY * endPos);
        }

        pos = endPos;
        drawing = !drawing;
    }
}

void ArrowRenderer::drawDashedForward(Graphics2D& g2, const ChemPoint& from,
                                      const ChemPoint& to, float scale,
                                      const ArrowStyle& style) {
    drawDashedLine(g2, from, to, style.dashLength * scale, style.dashGap * scale);
    drawArrowHead(g2, to, from, scale, style);
}

void ArrowRenderer::drawDashedEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                          const ChemPoint& to, float scale,
                                          const ArrowStyle& style) {
    DirInfo d(from, to);
    if (!d.valid()) return;

    float offset = style.doubleBondOffset * scale / 3.0f;
    float hl = style.headLength * scale;
    float hw = style.headWidth * scale;
    float margin = hl * 0.8f;

    ChemPoint innerFrom(from.x + d.dirX * margin, from.y + d.dirY * margin);
    ChemPoint innerTo(to.x - d.dirX * margin, to.y - d.dirY * margin);

    ChemPoint upperFrom(innerFrom.x - d.perp.x * offset, innerFrom.y - d.perp.y * offset);
    ChemPoint upperTo(innerTo.x - d.perp.x * offset, innerTo.y - d.perp.y * offset);
    ChemPoint lowerFrom(innerFrom.x + d.perp.x * offset, innerFrom.y + d.perp.y * offset);
    ChemPoint lowerTo(innerTo.x + d.perp.x * offset, innerTo.y + d.perp.y * offset);

    float dashLen = style.dashLength * scale;
    float gapLen = style.dashGap * scale;
    drawDashedLine(g2, upperFrom, upperTo, dashLen, gapLen);
    drawDashedLine(g2, lowerFrom, lowerTo, dashLen, gapLen);

    ChemPoint base1(upperTo.x - d.dirX * hl, upperTo.y - d.dirY * hl);
    g2.drawLine(upperTo.x, upperTo.y, base1.x - d.perp.x * hw, base1.y - d.perp.y * hw);

    ChemPoint base2(lowerFrom.x + d.dirX * hl, lowerFrom.y + d.dirY * hl);
    g2.drawLine(lowerFrom.x, lowerFrom.y, base2.x + d.perp.x * hw, base2.y + d.perp.y * hw);
}

void ArrowRenderer::drawCurvedPath(Graphics2D& g2, const ChemPoint& from,
                                   const ChemPoint& ctrl, const ChemPoint& to,
                                   float /*scale*/, int segments) {
    float prevX = from.x;
    float prevY = from.y;
    for (int i = 1; i <= segments; i++) {
        float t = static_cast<float>(i) / segments;
        float t1 = 1.0f - t;
        float x = t1 * t1 * from.x + 2.0f * t1 * t * ctrl.x + t * t * to.x;
        float y = t1 * t1 * from.y + 2.0f * t1 * t * ctrl.y + t * t * to.y;
        g2.drawLine(prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
}

void ArrowRenderer::drawCurvedForward(Graphics2D& g2, const ChemPoint& from,
                                      const ChemPoint& to, float scale,
                                      const ArrowStyle& style, float curveHeight) {
    ChemPoint ctrl = computeCurveControlPoint(from, to, curveHeight);
    drawCurvedPath(g2, from, ctrl, to, scale, CURVE_SEGMENTS);

    float t = 1.0f / CURVE_SEGMENTS;
    float t1 = 1.0f - t;
    ChemPoint nearTo(t1 * t1 * from.x + 2.0f * t1 * t * ctrl.x + t * t * to.x,
                     t1 * t1 * from.y + 2.0f * t1 * t * ctrl.y + t * t * to.y);
    drawArrowHead(g2, to, nearTo, scale, style);
}

void ArrowRenderer::drawCurvedBackward(Graphics2D& g2, const ChemPoint& from,
                                       const ChemPoint& to, float scale,
                                       const ArrowStyle& style, float curveHeight) {
    ChemPoint ctrl = computeCurveControlPoint(from, to, curveHeight);
    drawCurvedPath(g2, from, ctrl, to, scale, CURVE_SEGMENTS);

    float t = 1.0f - 1.0f / CURVE_SEGMENTS;
    float t1 = 1.0f - t;
    ChemPoint nearFrom(t1 * t1 * from.x + 2.0f * t1 * t * ctrl.x + t * t * to.x,
                       t1 * t1 * from.y + 2.0f * t1 * t * ctrl.y + t * t * to.y);
    drawArrowHead(g2, from, nearFrom, scale, style);
}

void ArrowRenderer::drawCurvedBidirectional(Graphics2D& g2, const ChemPoint& from,
                                            const ChemPoint& to, float scale,
                                            const ArrowStyle& style, float curveHeight) {
    ChemPoint ctrl = computeCurveControlPoint(from, to, curveHeight);
    drawCurvedPath(g2, from, ctrl, to, scale, CURVE_SEGMENTS);

    {
        float t = 1.0f / CURVE_SEGMENTS;
        float t1 = 1.0f - t;
        ChemPoint nearTo(t1 * t1 * from.x + 2.0f * t1 * t * ctrl.x + t * t * to.x,
                         t1 * t1 * from.y + 2.0f * t1 * t * ctrl.y + t * t * to.y);
        drawArrowHead(g2, to, nearTo, scale, style);
    }

    {
        float t = 1.0f - 1.0f / CURVE_SEGMENTS;
        float t1 = 1.0f - t;
        ChemPoint nearFrom(t1 * t1 * from.x + 2.0f * t1 * t * ctrl.x + t * t * to.x,
                           t1 * t1 * from.y + 2.0f * t1 * t * ctrl.y + t * t * to.y);
        drawArrowHead(g2, from, nearFrom, scale, style);
    }
}

void ArrowRenderer::drawArrow(Graphics2D& g2, ArrowType type,
                              const ChemPoint& from, const ChemPoint& to,
                              float scale, const ArrowParams& params,
                              const ArrowStyle& style) {
    const Stroke& oldStroke = g2.getStroke();
    color oldColor = g2.getColor();
    
    g2.setStroke(Stroke(style.lineWidth * scale, CAP_BUTT, JOIN_MITER));
    
    if (!params.color.empty()) {
        color arrowColor = parseColorName(params.color);
        if (arrowColor != trans) {
            g2.setColor(arrowColor);
        }
    }

    bool useDashed = params.dashed || type == ARROW_DASHED_FORWARD || type == ARROW_DASHED_EQUILIBRIUM;
    
    if (useDashed && type != ARROW_DASHED_FORWARD && type != ARROW_DASHED_EQUILIBRIUM) {
        drawDashedLine(g2, from, to, style.dashLength * scale, style.dashGap * scale);
        drawArrowHead(g2, to, from, scale, style);
    } else {
        switch (type) {
            case ARROW_FORWARD:
                drawForward(g2, from, to, scale, style);
                break;
            case ARROW_BACKWARD:
                drawBackward(g2, from, to, scale, style);
                break;
            case ARROW_BIDIRECTIONAL:
                drawBidirectional(g2, from, to, scale, style);
                break;
            case ARROW_EQUILIBRIUM:
                drawEquilibrium(g2, from, to, scale, style);
                break;
            case ARROW_LONG_EQUILIB:
                drawLongEquilibrium(g2, from, to, scale, style);
                break;
            case ARROW_ALT_EQUILIB:
                drawAltEquilibrium(g2, from, to, scale, style);
                break;
            case ARROW_HARP_RIGHT:
                drawHarpRight(g2, from, to, scale, style);
                break;
            case ARROW_HARP_LEFT:
                drawHarpLeft(g2, from, to, scale, style);
                break;
            case ARROW_FISHHOOK:
                drawFishhook(g2, from, to, scale, style);
                break;
            case ARROW_INVISIBLE:
                drawInvisible(g2, from, to, scale, style);
                break;
            case ARROW_CURVED_FORWARD:
                drawCurvedForward(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_ARC_FORWARD:
                drawCurvedForward(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_CURVED_BACKWARD:
                drawCurvedBackward(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_ARC_BACKWARD:
                drawCurvedBackward(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_CURVED_BIDIR:
                drawCurvedBidirectional(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_ARC_BIDIR:
                drawCurvedBidirectional(g2, from, to, scale, style, params.effectiveCurveHeight());
                break;
            case ARROW_HARPOON_RIGHT:
                drawHarpoonRight(g2, from, to, scale, style);
                break;
            case ARROW_HARPOON_LEFT:
                drawHarpoonLeft(g2, from, to, scale, style);
                break;
            case ARROW_DASHED_FORWARD:
                drawDashedForward(g2, from, to, scale, style);
                break;
            case ARROW_DASHED_EQUILIBRIUM:
                drawDashedEquilibrium(g2, from, to, scale, style);
                break;
            default:
                drawForward(g2, from, to, scale, style);
                break;
        }
    }

    g2.setStroke(oldStroke);
    g2.setColor(oldColor);
}

} // namespace tex

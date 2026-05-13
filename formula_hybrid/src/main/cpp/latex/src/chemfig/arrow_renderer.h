#ifndef ARROW_RENDERER_H_INCLUDED
#define ARROW_RENDERER_H_INCLUDED

#include "scheme_types.h"
#include "graphic/graphic.h"

namespace tex {

struct ArrowStyle {
    float headLength;
    float headWidth;
    float lineWidth;
    float doubleBondOffset;
    float harpRadius;
    float dashLength;
    float dashGap;
};

class ArrowRenderer {
public:
    static void drawArrow(Graphics2D& g2, ArrowType type,
                          const ChemPoint& from, const ChemPoint& to,
                          float scale, const ArrowParams& params,
                          const ArrowStyle& style);

    static ChemPoint computeCurveControlPoint(const ChemPoint& from,
                                               const ChemPoint& to,
                                               float curveHeight);

    static void drawMergeArrowHead(Graphics2D& g2, const ChemPoint& tip,
                                   const ChemPoint& from, float scale,
                                   float headLength, float headWidth);

private:
    static void drawForward(Graphics2D& g2, const ChemPoint& from,
                            const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawBackward(Graphics2D& g2, const ChemPoint& from,
                             const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawBidirectional(Graphics2D& g2, const ChemPoint& from,
                                  const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawLongEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                    const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawAltEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                   const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawHarpRight(Graphics2D& g2, const ChemPoint& from,
                              const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawHarpLeft(Graphics2D& g2, const ChemPoint& from,
                             const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawFishhook(Graphics2D& g2, const ChemPoint& from,
                             const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawInvisible(Graphics2D& g2, const ChemPoint& from,
                              const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawHarpoonRight(Graphics2D& g2, const ChemPoint& from,
                                 const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawHarpoonLeft(Graphics2D& g2, const ChemPoint& from,
                                const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawDashedForward(Graphics2D& g2, const ChemPoint& from,
                                  const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawDashedEquilibrium(Graphics2D& g2, const ChemPoint& from,
                                      const ChemPoint& to, float scale, const ArrowStyle& style);
    static void drawCurvedForward(Graphics2D& g2, const ChemPoint& from,
                                  const ChemPoint& to, float scale,
                                  const ArrowStyle& style, float curveHeight);
    static void drawCurvedBackward(Graphics2D& g2, const ChemPoint& from,
                                   const ChemPoint& to, float scale,
                                   const ArrowStyle& style, float curveHeight);
    static void drawCurvedBidirectional(Graphics2D& g2, const ChemPoint& from,
                                        const ChemPoint& to, float scale,
                                        const ArrowStyle& style, float curveHeight);

    static void drawArrowHead(Graphics2D& g2, const ChemPoint& tip,
                              const ChemPoint& ref, float scale, const ArrowStyle& style);
    static void drawHarpoonHead(Graphics2D& g2, const ChemPoint& tip,
                                const ChemPoint& ref, float scale,
                                const ArrowStyle& style, bool upper);
    static void drawCurvedPath(Graphics2D& g2, const ChemPoint& from,
                               const ChemPoint& ctrl, const ChemPoint& to,
                               float scale, int segments);
    static void drawDashedLine(Graphics2D& g2, const ChemPoint& from,
                               const ChemPoint& to, float dashLen, float gapLen);
};

} // namespace tex

#endif // ARROW_RENDERER_H_INCLUDED

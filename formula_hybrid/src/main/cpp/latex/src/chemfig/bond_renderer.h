#ifndef BOND_RENDERER_H_INCLUDED
#define BOND_RENDERER_H_INCLUDED

#include "chemfig_types.h"
#include "graphic/graphic.h"

namespace tex {

class BondRenderer {
public:
    static void drawBond(Graphics2D& g2, BondType type,
                         const ChemPoint& from, const ChemPoint& to, 
                         float scale, const BondParams& params,
                         const ChemPoint& ringCenter, bool inRing);

private:
    static void drawSingle(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawDouble(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale,
                           const ChemPoint& ringCenter, bool inRing);
    static void drawTriple(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeUp(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeDown(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeDashed(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up);
    static void drawWedgeHollow(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up);
    
    static void drawLine(Graphics2D& g2, float x1, float y1, float x2, float y2, float scale);
    static void drawDashedLine(Graphics2D& g2, float x1, float y1, float x2, float y2, float scale);
};

} // namespace tex

#endif // BOND_RENDERER_H_INCLUDED

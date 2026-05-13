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
    static void drawWedgeHollowUp(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeHollowDown(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeDotted(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up);
    static void drawWedgeSolid(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale, bool up);
    static void drawWedgeSolidUp(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    static void drawWedgeSolidDown(Graphics2D& g2, const ChemPoint& from, const ChemPoint& to, float scale);
    
    static void drawDashedLine(Graphics2D& g2, float x1, float y1, float x2, float y2, float scale);
};

} // namespace tex

#endif // BOND_RENDERER_H_INCLUDED

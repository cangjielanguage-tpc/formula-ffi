#ifndef CHEMFIG_BOX_H_INCLUDED
#define CHEMFIG_BOX_H_INCLUDED

#include "chemfig_types.h"
#include "bond_renderer.h"
#include "atom/box.h"
#include <unordered_map>

namespace tex {

enum TextSegmentType {
    SEG_NORMAL,
    SEG_SUBSCRIPT,
    SEG_SUPERSCRIPT
};

struct TextSegment {
    std::wstring text;
    TextSegmentType type;
    sptr<TextLayout> layout;
    float width;
    float height;
    float ascent;
};

struct AtomLayout {
    std::wstring text;
    float x;
    float y;
    float textOffsetX;
    float textOffsetY;
    sptr<TextLayout> layout;
    std::vector<TextSegment> segments;
    float normalAscent;
    float normalHeight;
};

struct AtomTextBounds {
    float halfW;
    float halfH;
};

class ChemfigBox : public Box {
private:
    Molecule _molecule;
    color _color;
    sptr<Font> _font;
    float _sizeFactor;
    float _textBoundsMinX;
    float _textBoundsMinY;
    std::vector<AtomLayout> _atomLayouts;
    std::unordered_map<int, AtomTextBounds> _atomTextBounds;
    
    void calculateLayout();
    void buildAtomLayouts(float offsetX, float offsetY, float scale);
    void drawMolecule(Graphics2D& g2, float x, float y);

public:
    ChemfigBox(const Molecule& mol, color c, const sptr<Font>& font, float sizeFactor);
    
    void draw(Graphics2D& g2, float x, float y) override;
    
    int getLastFontId() override;
};

} // namespace tex

#endif // CHEMFIG_BOX_H_INCLUDED

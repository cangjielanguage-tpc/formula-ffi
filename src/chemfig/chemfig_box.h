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

constexpr float BOX_TEXT_SCALE_BASE = 0.1f;
constexpr float BOX_BOND_OFFSET_SCALE = 0.06f;
constexpr float BOX_INNER_CIRCLE_RADIUS_RATIO = 0.75f;
constexpr float BOX_CHARGE_RADIUS_FACTOR = 0.55f;
constexpr float BOX_CHARGE_DISPLAY_BASE = 10.0f;
constexpr float BOX_CHARGE_RECT_WIDTH_FACTOR = 0.45f;
constexpr float BOX_CHARGE_RECT_HEIGHT_FACTOR = 0.12f;
constexpr float BOX_CHARGE_LINE_LENGTH_FACTOR = 0.5f;
constexpr float BOX_CHARGE_DOT_SIZE_FACTOR = 0.12f;
constexpr float BOX_CHARGE_DOT_SPACING_FACTOR = 0.18f;
constexpr float BOX_CHARGE_CIRCLE_RADIUS_FACTOR = 0.20f;
constexpr float BOX_CHARGE_CIRCLE_LINE_RATIO = 0.8f;
constexpr float BOX_CURVE_FROM_ATOM_EXTRA = 6.0f;
constexpr float BOX_CURVE_TO_ATOM_EXTRA = 6.0f;
constexpr float BOX_CURVE_SHORTEN_SCALE = 0.1f;
constexpr float BOX_CURVE_HEAD_LENGTH = 0.25f;
constexpr float BOX_CURVE_HEAD_WIDTH = 0.13f;
constexpr float BOX_CURVE_ARROW_ANGLE = 0.4f;
constexpr float BOX_TEXT_WIDTH_EXTRA = 0.6f;

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
    float _textBoundsMaxX;
    float _textBoundsMaxY;
    std::vector<AtomLayout> _atomLayouts;
    std::unordered_map<int, AtomTextBounds> _atomTextBounds;
    bool _layoutsBuilt;

    void calculateLayout();
    void buildAtomLayouts(float offsetX, float offsetY, float scale);
    float computeAnchorYOffset(int atomIndex) const;
    void drawMolecule(Graphics2D& g2, float x, float y);

public:
    ChemfigBox(const Molecule& mol, color c, const sptr<Font>& font, float sizeFactor);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    float getTextBoundsMinX() const { return _textBoundsMinX; }
    float getTextBoundsMinY() const { return _textBoundsMinY; }
    float getTextBoundsMaxY() const { return _textBoundsMaxY; }
    float getSizeFactor() const { return _sizeFactor; }
};

} // namespace tex

#endif // CHEMFIG_BOX_H_INCLUDED

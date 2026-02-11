#ifndef BOX_H_INCLUDED
#define BOX_H_INCLUDED

#include "atom/atom.h"
#include <stack>

using namespace tex;

namespace tex {

class Char;
class CharFont;
class TeXFont;
class SymbolAtom;

/***************************************************************************************************
 *                              factories to create boxes                                          *
 ***************************************************************************************************/

/**
 * Responsible for creating a box containing a delimiter symbol that exists in
 * different sizes.
 */
class DelimiterFactory {
public:
    static sptr<Box> create(_in_ SymbolAtom& symbol, _out_ TeXEnvironment& env, int size);

    /**
     * Create a delimiter with specified symbol name and min height
     *
     * @param symbol
     *      the name of the delimiter symbol
     * @param env
     *      the TeXEnvironment in which to create the delimiter box
     * @param minHeight
     *      the minimum required total height of the box (height + depth).
     * @return the box representing the delimiter variant that fits best
     *      according to the required minimum size.
     */
    static sptr<Box> create(const string& symbol, _out_ TeXEnvironment& env, float minHeight);
};

/**
 * Responsible for creating a box containing a delimiter symbol that exists in
 * different sizes.
 */
class XLeftRightArrowFactory {
private:
    static sptr<Atom> MINUS;
    static sptr<Atom> LEFT;
    static sptr<Atom> RIGHT;

public:
    static sptr<Box> create(bool left, _out_ TeXEnvironment& env, float width);

    static sptr<Box> create(_out_ TeXEnvironment& env, float width);
};

/***************************************************************************************************
 *                                        rule boxes                                               *
 ***************************************************************************************************/

/**
 * A box composed of a horizontal row of child boxes
 */
class HorizontalBox : public Box {
private:
    void recalculate(const Box& b);

    pair<sptr<HorizontalBox>, sptr<HorizontalBox>> split(int pos, int shift);

public:
    vector<int> _breakPositions;

    HorizontalBox() {}

    HorizontalBox(color fg, color bg) : Box(fg, bg) {}

    HorizontalBox(const sptr<Box>& b, float w, int alignment);

    HorizontalBox(const sptr<Box>& b);

    sptr<HorizontalBox> cloneBox();

    void add(const sptr<Box>& b) override;

    void add(int pos, const sptr<Box>& b) override;

    inline void addBreakPosition(int pos) {
        _breakPositions.push_back(pos);
    }

    pair<sptr<HorizontalBox>, sptr<HorizontalBox>> split(int pos) {
        return split(pos, 1);
    }

    pair<sptr<HorizontalBox>, sptr<HorizontalBox>> splitRemove(int pos) {
        return split(pos, 2);
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;
};

/**
 * A box composed of other boxes, put one above the other
 */
class VerticalBox : public Box {
private:
    float _leftMostPos, _rightMostPos;

    void recalculateWidth(const Box& b);

public:
    VerticalBox() : _leftMostPos(F_MAX), _rightMostPos(F_MIN) {}

    VerticalBox(const sptr<Box>& b, float rest, int alignment);

    void add(const sptr<Box>& b) override;

    void add(const sptr<Box>& b, float interline);

    void add(int pos, const sptr<Box>& b) override;

    inline int getSize() const {
        return _children.size();
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;
};

/**
 * A box representing another box with a horizontal rule above it, with
 * appropriate kerning.
 */
class OverBar : public VerticalBox {
public:
    OverBar() = delete;

    OverBar(const sptr<Box>& b, float kern, float thickness);
};

/**
 * A box representing another box with a delimiter box and a script box above or
 * under it, with script and delimiter separated by a kerning.
 */
class OverUnderBox : public Box {
private:
    // base, delimiter and script
    sptr<Box> _base, _del, _script;
    // kerning amount between the delimiter and the script
    float _kern;
    // whether the delimiter should be drawn over (<->under) the base box
    bool _over;

public:
    OverUnderBox() = delete;

    /**
     * The parameter boxes must have an equal width!!
     *
     * @param base
     *      base box to be drawn on the baseline
     * @param del
     *      delimiter box
     * @param script
     *      subscript or superscript box
     * @param kern
     *      the kerning amount to draw
     * @param over
     *      true : draws delimiter and script box above the base box,
     *      false : under the base box
     */
    OverUnderBox(
        const sptr<Box>& base, const sptr<Box>& del,
        const sptr<Box>& script, float kern, bool over);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * A box representing a horizontal line.
 */
class HorizontalRule : public Box {
private:
    color _color;
    float _speShift;

public:
    HorizontalRule() = delete;

    HorizontalRule(float thickness, float width, float shift);

    HorizontalRule(float thickness, float width, float shift, bool trueShift);

    HorizontalRule(float thickness, float width, float shift, color c, bool trueshift);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;
};

/***************************************************************************************************
 *                                   operation boxes                                               *
 ***************************************************************************************************/

/**
 * A box representing a scale operation
 */
class ScaleBox : public Box {
private:
    sptr<Box> _box;
    float _sx, _sy;
    float _factor;

    void init(const sptr<Box>& b, float sx, float sy);

public:
    ScaleBox() = delete;

    ScaleBox(const sptr<Box>& b, float sx, float sy) {
        init(b, sx, sy);
    }

    ScaleBox(const sptr<Box>& b, float factor) {
        init(b, factor, factor);
        _factor = factor;
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * A box representing a reflected box
 */
class ReflectBox : public Box {
private:
    sptr<Box> _box;

public:
    ReflectBox() = delete;

    ReflectBox(const sptr<Box>& b);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * Enumeration representing rotation origin
 */
enum Rotation {
    // Bottom Left
    BL,
    // Bottom Center
    BC,
    // Bottom Right
    BR,
    // Top Left
    TL,
    // Top Center
    TC,
    // Top Right
    TR,
    // Bottom Bottom Left
    BBL,
    // Bottom Bottom Right
    BBR,
    // Bottom Bottom Center
    BBC,
    // Center Left
    CL,
    // Center Center
    CC,
    // Center Right
    CR
};

/**
 * A box representing a rotate operation
 */
class RotateBox : public Box {
private:
    sptr<Box> _box;
    float _angle;
    float _xmax, _xmin, _ymax, _ymin;
    int _option;
    float _shiftX, _shiftY;

    void init(const sptr<Box>& b, float angle, float x, float y);

    static Point calculateShift(const Box& b, int option);

public:
    RotateBox() = delete;

    RotateBox(const sptr<Box>& b, float angle, float x, float y) {
        init(b, angle, x, y);
    }

    RotateBox(const sptr<Box>& b, float angle, const Point& origin) {
        init(b, angle, origin.x, origin.y);
    }

    RotateBox(const sptr<Box>& b, float angle, int option) {
        const Point& p = calculateShift(*b, option);
        init(b, angle, p.x, p.y);
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;

    static int getOrigin(string option);
};

/***************************************************************************************************
 *                                  wrapped boxes                                                  *
 ***************************************************************************************************/

/**
 * A box representing a wrapped box by square frame
 */
class FramedBox : public Box {
public:
    sptr<Box> _box;
    float _thickness;
    float _space;
    color _line;
    color _bg;

    void init(const sptr<Box>& box, float thickness, float space);

public:
    FramedBox() = delete;

    FramedBox(const sptr<Box>& box, float thickness, float space) {
        init(box, thickness, space);
    }

    FramedBox(const sptr<Box>& box, float thickness, float space, color line, color bg) {
        init(box, thickness, space);
        _line = line;
        _bg = bg;
    }

    virtual void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * A box representing a wrapped box by oval frame
 */
class OvalBox : public FramedBox {
private:
    float _multiplier, _diameter;

public:
    OvalBox() = delete;

    OvalBox(
        const sptr<FramedBox>& fbox,
        float multiplier = 0.5f,
        float diameter = 0.f)
        : FramedBox(fbox->_box, fbox->_thickness, fbox->_space),
          _multiplier(multiplier),
          _diameter(diameter) {}

    void draw(Graphics2D& g2, float x, float y) override;
};

/**
 * A box representing a wrapped box by shadowed frame
 */
class ShadowBox : public FramedBox {
private:
    float _shadowRule;

public:
    ShadowBox() = delete;

    ShadowBox(const sptr<FramedBox>& fbox, float shadowRule)
        : FramedBox(fbox->_box, fbox->_thickness, fbox->_space) {
        _shadowRule = shadowRule;
        _depth += shadowRule;
        _width += shadowRule;
    }

    void draw(Graphics2D& g2, float x, float y) override;
};

/***************************************************************************************************
 *                                      basic boxes                                                *
 ***************************************************************************************************/

/**
 * A box representing whitespace
 */
class StrutBox : public Box {
public:
    StrutBox() = delete;

    StrutBox(float w, float h, float d, float s) {
        _width = w;
        _height = h;
        _depth = d;
        _shift = s;
    }

    void draw(Graphics2D& g2, float x, float y) override {
        // no visual effect
    }

    int getLastFontId() override;
};

/**
 * A box representing glue
 */
class GlueBox : public Box {
public:
    float _stretch, _shrink;

    GlueBox() = delete;

    GlueBox(float space, float stretch, float shrink) {
        _width = space;
        _stretch = stretch;
        _shrink = shrink;
    }

    void draw(Graphics2D& g2, float x, float y) override {
        // no visual effect
    }

    int getLastFontId() override;
};

/**
 * A box representing a single character
 */
class CharBox : public Box {
private:
    sptr<CharFont> _cf;
    float _size;
    float _italic;

public:
    CharBox() = delete;

    /**
     * Create a new CharBox that will represent the character defined by the
     * given Char-object.
     *
     * @param c
     *      a Char-object containing the character's font information.
     */
    CharBox(const Char& c);

    void addItalicCorrectionToWidth();

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;
};

/**
 * A box representing a text rendering box
 */
class TextRenderingBox : public Box {
private:
    static sptr<Font> _font;
    sptr<TextLayout> _layout;
    float _size;

    void init(const wstring& str, int type, float size, const sptr<Font>& f, bool kerning);

public:
    TextRenderingBox() = delete;

    TextRenderingBox(const wstring& str, int type, float size, const sptr<Font>& f, bool kerning) {
        init(str, type, size, f, kerning);
    }

    TextRenderingBox(const wstring& str, int type, float size) {
        init(str, type, size, sptr<Font>(_font), true);
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    static void setFont(const string& name);

    static void _init_();

    static void _free_();
};

/**
 * A box representing 'wrapper' that with insets in left, top, right and bottom
 */
class WrapperBox : public Box {
private:
    sptr<Box> _base;
    float _l;

public:
    WrapperBox() = delete;

    WrapperBox(const sptr<Box>& base) : _base(base), _l(0) {
        _height = _base->_height;
        _depth = _base->_depth;
        _width = _base->_width;
    }

    WrapperBox(const sptr<Box>& base, float width, float rowheight, float rowdepth, float align)
        : _base(base), _l(0) {
        _height = rowheight;
        _depth = rowdepth;
        _width = width;
        if (base->_width < 0) _width += base->_width;
        if (align == ALIGN_RIGHT) {
            _l = width - _base->_width;
        } else if (align == ALIGN_CENTER) {
            _l = (width - _base->_width) / 2.f;
        }
    }

    void setInsets(float l, float t, float r, float b);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * Class representing a box that shifted up or down (when shift is negative)
 */
class ShiftBox : public Box {
private:
    float _sf;
    sptr<Box> _base;

public:
    ShiftBox() = delete;

    ShiftBox(const sptr<Box>& base, float shift) : _base(base), _sf(shift) {}

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * Class represents several lines
 */
class LineBox : public Box {
private:
    // Every 4 elements represent a line, thus (x1, y1, x2, y2)
    vector<float> _lines;
    float _thickness;
    int _lineCount;

public:
    LineBox() = delete;

    LineBox(const vector<float> lines, float thickness);

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;
};

/**
 * Class representing a box covered by another box
 */
class OverlappedBox : public Box {
private:
    sptr<Box> _base;
    sptr<Box> _overlap;

public:
    OverlappedBox() = delete;

    OverlappedBox(const sptr<Box> base, const sptr<Box> overlap)
        : _base(base), _overlap(overlap) {
        _width = base->_width;
        _height = base->_height;
        _depth = base->_depth;
        _shift = base->_shift;
        _type = base->_type;
    }

    void draw(Graphics2D& g2, float x, float y) override;

    int getLastFontId() override;

    vector<sptr<Box>> getChildren() const override;
};

/**
 * OverlayBox: 将两个 Box 叠加绘制，底层是 _base，上层是 _overlay
 */
class OverlayBox : public Box {
private:
    sptr<Box> _base;      // 底层 Box
    sptr<Box> _overlay;   // 叠加 Box

public:
    // 接收底层和叠加 Box，计算最大尺寸
    OverlayBox(const sptr<Box>& base, const sptr<Box>& overlay)
        : _base(base), _overlay(overlay) {
        _width  = max(_base->_width, _overlay->_width);
        _height = max(_base->_height, _overlay->_height);
        _depth  = max(_base->_depth, _overlay->_depth);
    }

    // 绘制方法：先绘制底层 Box，再绘制居中叠加 Box
    void draw(Graphics2D& g2, float x, float y) override {
        float bx = (_width - _base->_width) / 2;      // 底层 Box 居中偏移
        _base->draw(g2, x + bx, y);

        float ox = (_width - _overlay->_width) / 2;   // 叠加 Box 居中偏移
        _overlay->draw(g2, x + ox, y);
    }

    int getLastFontId() override {
        return _base->getLastFontId();
    }
};

/**
 * EllipseBox: 绘制椭圆形 Box，用于叠加
 */
class EllipseBox : public Box {
private:
    static constexpr float lineWidth = 0.04f;     // 描边线宽
    static constexpr float ellipseXRatio = 0.75f; // x 方向缩放比例
    static constexpr float ellipseYRatio = 0.28f; // y 方向缩放比例

public:
    // 初始化椭圆的宽、高、深度
    EllipseBox(float width, float height, float depth) {
        _width  = width  * ellipseXRatio;
        _height = height;
        _depth  = depth;
    }

void draw(Graphics2D& g2, float x, float y) override {
    float rx = _width / 2;
    float ry = (_height + _depth) / 2 * ellipseYRatio;
    g2.drawEllipse(x, y,_width,_height, rx, ry, lineWidth,_depth);
}

    // 椭圆没有字体
    int getLastFontId() override {
        return -1;
    }
};

/**
 * OiintAtom: 表示语义上的“双积分”符号，内部由2个 ∫ 和一个椭圆叠加构成
 */
class OiintAtom : public Atom {
private:
    sptr<Atom> _base; // 原子内容，用于生成底层 Box

public:
    // 接收一个 Atom 作为底层
    OiintAtom(const sptr<Atom>& base)
        : _base(base) {
        _type       = TYPE_BIG_OPERATOR;   // 类型为大运算符
        _typelimits = SCRIPT_NOLIMITS;     // 上下标不受限制
    }

    // 生成 Box，用于渲染
    inline sptr<Box> createBox(TeXEnvironment& env) {
        sptr<Box> baseBox = _base->createBox(env);  // 底层 Box
        sptr<EllipseBox> ellipse(
            new EllipseBox(
                baseBox->_width,
                baseBox->_height,
                baseBox->_depth
            )
        );

        // 返回底层 Box 与椭圆叠加的 OverlayBox
        return sptr<Box>(new OverlayBox(baseBox, ellipse));
    }

    __decl_clone(OiintAtom)
};

}  // namespace tex

#endif  // BOX_H_INCLUDED

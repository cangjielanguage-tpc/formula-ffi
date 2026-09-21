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
    color _lineColor;  // Line color

public:
    LineBox() = delete;

    LineBox(const vector<float> lines, float thickness, color lineColor = black);

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
 * CircleBox: 绘制正圆形 Box，用于叠加（如 \textcircled）
 * 圆心位于盒子的垂直中心（相对基线偏移 (height - depth) / 2），
 * 直径 = _width，且 height + depth == 2 * r（圆恰好内切于盒子边界）
 */
class CircleBox : public Box {
private:
    float _r;         // 圆半径
    float _lineWidth; // 描边线宽（与盒子坐标同单位，随渲染 scale 等比缩放）

public:
    CircleBox() = delete;

    /**
     * r             圆半径
     * centerOffset  圆心相对基线的偏移（基线以上为正），
     *               内部推导 height = r + centerOffset、depth = r - centerOffset，
     *               恒满足 height + depth == 2 * r（圆恰好内切于盒子边界）
     * lineWidth     描边线宽
     */
    CircleBox(float r, float centerOffset, float lineWidth)
        : _r(r), _lineWidth(lineWidth) {
        _width = r * 2;
        _height = r + centerOffset;
        _depth = r - centerOffset;
    }

    void draw(Graphics2D& g2, float x, float y) override {
        startDraw(g2, x, y);
        // drawEllipse 语义: 圆心 cy = y - h/2 + depth/2，传入
        // (_width, _height, _depth) 后圆心恰为盒子垂直中心
        g2.drawEllipse(x, y, _width, _height, _r, _r, _lineWidth, _depth);
        endDraw(g2);
    }

    // 圆没有字体
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

/**
 * CancelToBox: 用于实现 \cancelto{target}{expr}
 * 在表达式上画一条带箭头的斜线，并在箭头末端显示目标值
 * 
 * 安全特性：
 * - 尺寸计算时检查 INF/NaN
 * - 箭头绘制时检查除零
 * - 线宽有效性验证
 */
class CancelToBox : public Box {
private:
    sptr<Box> _base;       // 基础表达式 box
    sptr<Box> _target;     // 目标值 box
    float _lineThickness;  // 线宽
    float _arrowExtend;    // 箭头延伸长度
    color _lineColor;      // 线条颜色（包括箭头）

    // 安全地计算箭头延伸长度，避免 INF/NaN
    static float safeCalcExtend(float height, float depth) {
        float totalHeight = height + depth;
        if (!isfinite(totalHeight) || totalHeight <= 0) {
            return 10.0f;  // 默认延伸长度
        }
        return totalHeight * 0.13f;
    }

    // 安全地计算间隙，避免 INF/NaN
    static float safeCalcGap(float lineThickness) {
        if (!isfinite(lineThickness) || lineThickness <= 0) {
            return 4.0f;  // 默认间隙
        }
        return lineThickness * 2;
    }

    // 安全地限制尺寸在合理范围内
    static float clampSize(float size, float minVal, float maxVal) {
        if (!isfinite(size)) return minVal;
        return max(minVal, min(size, maxVal));
    }

    // 绘制箭头（从起点到终点，箭头在终点处，实心箭头效果）
    void drawArrow(Graphics2D& g2, float x1, float y1, float x2, float y2) {
        // 设置线条宽度（加粗箭头）
        float safeThickness = _lineThickness;
        if (!isfinite(safeThickness) || safeThickness <= 0) {
            safeThickness = 1.2f;  // 提高默认线宽（加粗）
        }
        
        // 保存当前线宽和颜色
        const Stroke& oldStroke = g2.getStroke();
        float oldLineWidth = oldStroke.lineWidth;
        int oldColor = g2.getColor();
        
        g2.setStrokeWidth(safeThickness);
        g2.setColor(_lineColor);  // 设置线条颜色为 CancelColor
        
        // 绘制主线（实线）
        g2.drawLine(x1, y1, x2, y2);

        // 箭头大小
        float arrowSize = safeThickness * 5;  // 箭头长度
        float arrowAngle = PI / 8;  // 30 度，箭头角度

        // 计算从起点到终点的向量（用于确定箭头方向）
        float dx = x2 - x1;
        float dy = y2 - y1;
        float len = sqrt(dx * dx + dy * dy);

        // 防止除零：如果长度为 0 或无效，不绘制箭头
        if (!isfinite(len) || len < PREC) {
            g2.setStrokeWidth(oldLineWidth);
            return;
        }

        // 归一化向量（使用乘法避免除法）
        float invLen = 1.0f / len;
        float nx = dx * invLen;
        float ny = dy * invLen;

        // 计算箭头两翼的端点（使用向量旋转）
        // 左翼：逆时针旋转 arrowAngle
        float cosA = cos(arrowAngle);
        float sinA = sin(arrowAngle);
        float lx = nx * cosA - ny * sinA;
        float ly = nx * sinA + ny * cosA;

        // 右翼：顺时针旋转 arrowAngle
        float rx = nx * cosA + ny * sinA;
        float ry = -nx * sinA + ny * cosA;

        // 计算箭头三角形的三个顶点
        // 左翼端点
        float leftX = x2 - lx * arrowSize;
        float leftY = y2 - ly * arrowSize;
        // 右翼端点
        float rightX = x2 - rx * arrowSize;
        float rightY = y2 - ry * arrowSize;
        
        // 绘制实心三角形箭头：使用三条线段组成封闭三角形并黑色填充
        // 使用黑色填充三角形内部
        g2.setColor(0x000000);  // 黑色
        
        // 使用多条平行线填充三角形内部（模拟实心填充）
        int fillLines = 15;  // 填充线条数
        for (int i = 0; i < fillLines; i++) {
            float t = (float)(i + 1) / (float)fillLines;
            // 当前填充线的起点（在底边上）
            float baseX = leftX + (rightX - leftX) * t;
            float baseY = leftY + (rightY - leftY) * t;
            // 当前填充线的终点（在顶点方向）
            float tipX = baseX + (x2 - baseX) * 0.3f;
            float tipY = baseY + (y2 - baseY) * 0.3f;
            // 绘制填充线
            g2.drawLine(baseX, baseY, tipX, tipY);
        }
        
        // 恢复原来的颜色
        g2.setColor(oldColor);
        
        // 绘制三角形三条边（封闭三角形轮廓）
        g2.setStrokeWidth(safeThickness);
        g2.drawLine(leftX, leftY, rightX, rightY);      // 底边
        g2.drawLine(rightX, rightY, x2, y2);            // 右边
        g2.drawLine(x2, y2, leftX, leftY);              // 左边
        
        // 恢复原来的线宽和颜色
        g2.setStrokeWidth(oldLineWidth);
        g2.setColor(oldColor);
    }

public:
    CancelToBox() = delete;

    CancelToBox(const sptr<Box>& base, const sptr<Box>& target, float lineThickness, color lineColor = black)
        : _base(base), _target(target), _lineThickness(lineThickness), _lineColor(lineColor) {

        // 检查 base 是否有效（尺寸不能太小）
        float baseHeight = base->_height + base->_depth;
        float baseWidth = base->_width;
        
        // 如果 base 尺寸过小（接近空），限制箭头延伸长度
        // 这防止了 \cancelto{target}{} 产生超长箭头
        constexpr float MIN_BASE_SIZE = 0.5f;  // 最小基础尺寸
        if (baseHeight < MIN_BASE_SIZE && baseWidth < MIN_BASE_SIZE) {
            // base 太小，不延伸箭头
            _arrowExtend = 0;
        } else {
            // 安全计算箭头延伸长度
            _arrowExtend = safeCalcExtend(base->_height, base->_depth);
        }

        // 安全计算目标值间隙
        float targetGap = safeCalcGap(lineThickness);

        // 安全计算整体尺寸，限制在合理范围内
        constexpr float MAX_DIM = 100000.0f;  // 最大尺寸限制
        constexpr float MIN_DIM = 0.1f;       // 最小尺寸限制
        
        _width = clampSize(
            base->_width + _arrowExtend + target->_width + targetGap,
            MIN_DIM, MAX_DIM
        );
        _height = clampSize(
            base->_height + _arrowExtend + target->_height + target->_depth + targetGap,
            MIN_DIM, MAX_DIM
        );
        _depth = clampSize(base->_depth, MIN_DIM, MAX_DIM);
    }

    void draw(Graphics2D& g2, float x, float y) override {
        // 整体向右偏移量
        float offsetX = _lineThickness * 8;  // 向右移动 2 倍线宽
        
        // 绘制基础表达式（向右偏移）
        _base->draw(g2, x + offsetX, y);

        // 安全计算各部分尺寸
        float safeDepth = isfinite(_base->_depth) ? _base->_depth : 0.0f;
        float safeHeight = isfinite(_base->_height) ? _base->_height : 0.0f;
        float safeWidth = isfinite(_base->_width) ? _base->_width : 0.0f;
        
        // 计算目标值的位置（在基础表达式右上角的右上方）
        float targetGap = safeCalcGap(_lineThickness);
        float targetOffsetX = _lineThickness * 3;  // 目标值向右偏移 3 倍线宽
        float targetX = x + offsetX + safeWidth + targetGap + targetOffsetX;
        float safeTargetDepth = isfinite(_target->_depth) ? _target->_depth : 0.0f;
        float safeTargetHeight = isfinite(_target->_height) ? _target->_height : 0.0f;
        // 目标值的底部与基础表达式的顶部对齐
        float targetY = y - safeHeight + safeTargetDepth;
        
        // 绘制目标值
        _target->draw(g2, targetX, targetY);
        
        // 箭头起点：基础表达式（x^0）的左下角（向下偏移）
        float offsetY = _lineThickness * 8;  // 向下偏移 2 倍线宽
        float x1 = x;
        float y1 = y + safeDepth + offsetY;
        // 箭头终点：目标值（1）的左上角（箭头指向 1，向上移）
        float offsetTargetY = _lineThickness * 3;  // 向上偏移 2 倍线宽
        float x2 = targetX * 1.0f;
        float y2 = targetY - safeTargetHeight * 0.15f - offsetTargetY;

        // 绘制带箭头的斜线（从 x^0 指向 1）
        drawArrow(g2, x1, y1, x2, y2);
    }

    int getLastFontId() override {
        return _base->getLastFontId();
    }
};

class FilledRectBox : public Box {
public:
    FilledRectBox(float width, float height, float depth) {
        _width = width;
        _height = height;
        _depth = depth;
    }

    void draw(Graphics2D& g2, float x, float y) override {
        g2.fillRect(x, y - _height, _width, _height + _depth);
    }

    int getLastFontId() override {
        return 0;
    }
};

class RectBox : public Box {
public:
    RectBox(float width, float height, float depth) {
        _width = width;
        _height = height;
        _depth = depth;
    }

    void draw(Graphics2D& g2, float x, float y) override {
        g2.drawRect(x, y - _height, _width, _height + _depth);
    }

    int getLastFontId() override {
        return 0;
    }
};

}  // namespace tex

#endif  // BOX_H_INCLUDED

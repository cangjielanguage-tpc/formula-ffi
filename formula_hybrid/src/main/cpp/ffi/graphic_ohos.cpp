#include "config.h"

#if defined(__OS_ohos__) && !defined(MEM_CHECK)

#include "graphic_ohos.h"
#include "utils.h"

using namespace tex;
using namespace std;

/******************************************* ohos Font *****************************************/

map<string, string> Font_ohos::_file_name_map;

Font_ohos::Font_ohos(const string& family, int style, float size)
    : _family(family), _style(style), _size(size), _file(""){
    }

Font_ohos::Font_ohos(const string& file, float size) : Font_ohos("", PLAIN, size) {
    loadFont(file);
    _file = file;
}

void Font_ohos::loadFont(const string& file) {
    auto it = _file_name_map.find(file);
    if (it != _file_name_map.end()) {
        // already loaded
        _family = _file_name_map[file];
        return;
    }

    int index = file.rfind("/");
    string name = file.substr(index + 1, file.rfind(".") - index - 1);
    _family = name;
    _file_name_map[file] = _family;
}

string Font_ohos::getFile() const {
    return _file;
}

string Font_ohos::getFamily() const {
    return _family;
}

int Font_ohos::getStyle() const {
    return _style;
}

float Font_ohos::getSize() const {
    return _size;
}

sptr<Font> Font_ohos::deriveFont(int style) const {
    return sptr<Font>(new Font_ohos(_family, style, _size));
}

bool Font_ohos::operator==(const Font& ft) const {
    const Font_ohos& f = static_cast<const Font_ohos&>(ft);
    return _size == f._size && _style == f._style && _family == f._family;
}

bool Font_ohos::operator!=(const Font& f) const {
    return !(*this == f);
}

Font* Font::create(const string& file, float size) {
    return new Font_ohos(file, size);
}

sptr<Font> Font::_create(const string& name, int style, float size) {
    return sptr<Font>(new Font_ohos(name, style, size));
}

/******************************************* Text layout ******************************************/

TextLayout_ohos::TextLayout_ohos(const wstring& src, const sptr<Font_ohos>& font): _txt(src), _font(font) {}

void TextLayout_ohos::getBounds(_out_ Rect& r) {
    OH_Drawing_TypographyStyle* typoStyle = OH_Drawing_CreateTypographyStyle();
    OH_Drawing_TextStyle* txtStyle = OH_Drawing_CreateTextStyle();
    OH_Drawing_TypographyCreate* handler = OH_Drawing_CreateTypographyHandler(typoStyle, OH_Drawing_CreateFontCollection());
    double fontSize = _font->getSize();
    OH_Drawing_SetTextStyleFontSize(txtStyle, fontSize);
    OH_Drawing_SetTextStyleFontWeight(txtStyle, FONT_WEIGHT_400);
    bool halfLeading = true;
    OH_Drawing_SetTextStyleHalfLeading(txtStyle, halfLeading);
    const char *fontFamilies[] = {_font->getFamily().c_str()};
    OH_Drawing_SetTextStyleFontFamilies(txtStyle, 1, fontFamilies);
    OH_Drawing_TypographyHandlerPushTextStyle(handler, txtStyle);
    OH_Drawing_TypographyHandlerAddText(handler, wide2utf8(_txt.c_str()).c_str());
    OH_Drawing_PlaceholderSpan placeholderSpan = {(double)1000, (double)1000, ALIGNMENT_OFFSET_AT_BASELINE, TEXT_BASELINE_ALPHABETIC, 10};
    OH_Drawing_TypographyHandlerAddPlaceholder(handler, &placeholderSpan);
    OH_Drawing_TypographyHandlerPopTextStyle(handler);
    OH_Drawing_Typography* typography = OH_Drawing_CreateTypography(handler);
    OH_Drawing_TypographyLayout(typography, 1000);
    OH_Drawing_RectHeightStyle heightStyle = RECT_HEIGHT_STYLE_TIGHT;
    OH_Drawing_RectWidthStyle widthStyle = RECT_WIDTH_STYLE_TIGHT;
    OH_Drawing_TextBox *textbox = OH_Drawing_TypographyGetRectsForRange(typography, 0, _txt.length(), heightStyle, widthStyle);
    float right =  OH_Drawing_GetRightFromTextBox (textbox, 0);
    float bottom = OH_Drawing_GetBottomFromTextBox(textbox, 0);

    r.x = 0;
    r.y = -9;
    r.w = right;
    r.h = bottom;

    OH_Drawing_DestroyTypography(typography);
    OH_Drawing_DestroyTypographyHandler(handler);
    OH_Drawing_DestroyTextStyle(txtStyle);
    OH_Drawing_DestroyTypographyStyle(typoStyle);
}

void TextLayout_ohos::draw(Graphics2D& g2, float x, float y) {
    const Font* oldFont = g2.getFont();
    g2.setFont(_font.get());
    g2.drawText(_txt, x, y);
    g2.setFont(oldFont);
}

sptr<TextLayout> TextLayout::create(const wstring& txt, const sptr<Font>& font) {
    sptr<Font_ohos> f = static_pointer_cast<Font_ohos>(font);
    return sptr<TextLayout>(new TextLayout_ohos(txt, f));
}

/******************************************* Graphics 2D ******************************************/

Graphics2D_ohos::Graphics2D_ohos(OH_Drawing_Bitmap *bitmap, uint32_t background): _stroke() {
    _color = black;
    _bitmap = bitmap;
    _font = new Font_ohos("sans-serif", PLAIN, 14.f);

    _canvas = OH_Drawing_CanvasCreate();
    OH_Drawing_CanvasBind(_canvas, _bitmap);
    OH_Drawing_CanvasClear(_canvas, background);

    _pen = OH_Drawing_PenCreate();
    OH_Drawing_PenSetAntiAlias(_pen, true);

    _brush = OH_Drawing_BrushCreate();

    _txtStyle = OH_Drawing_CreateTextStyle();

    // 选择从左到右/左对齐等排版属性
    _typoStyle = OH_Drawing_CreateTypographyStyle();
    OH_Drawing_SetTypographyTextDirection(_typoStyle, TEXT_DIRECTION_LTR);
    OH_Drawing_SetTypographyTextAlign(_typoStyle, TEXT_ALIGN_LEFT);
}

Graphics2D_ohos::~Graphics2D_ohos() {
    OH_Drawing_PenDestroy(_pen);
    OH_Drawing_BrushDestroy(_brush);
    OH_Drawing_DestroyTextStyle(_txtStyle);
    OH_Drawing_DestroyTypographyStyle(_typoStyle);
    OH_Drawing_CanvasDestroy(_canvas);
}

void Graphics2D_ohos::setColor(color c) {
    _color = c;
    OH_Drawing_PenSetColor(_pen, _color);
    OH_Drawing_BrushSetColor(_brush, _color);
    OH_Drawing_SetTextStyleColor(_txtStyle, c);
}

color Graphics2D_ohos::getColor() const {
    return _color;
}

void Graphics2D_ohos::setStroke(const Stroke& s) {
    _stroke = s;
    OH_Drawing_PenSetWidth(_pen, s.lineWidth);
    OH_Drawing_PenSetMiterLimit(_pen, s.miterLimit);
    OH_Drawing_PenLineCapStyle c;
    switch (s.cap) {
    case CAP_BUTT:
        c = LINE_FLAT_CAP;
        break;
    case CAP_ROUND:
        c = LINE_ROUND_CAP;
        break;
    case CAP_SQUARE:
        c = LINE_SQUARE_CAP;
        break;
    }
    OH_Drawing_PenSetCap(_pen, c);
    OH_Drawing_PenLineJoinStyle j;
    switch (s.join) {
    case JOIN_BEVEL:
        j = LINE_BEVEL_JOIN;
        break;
    case JOIN_ROUND:
        j = LINE_ROUND_JOIN;
        break;
    case JOIN_MITER:
        j = LINE_MITER_JOIN;
        break;
    }
    OH_Drawing_PenSetJoin(_pen, j);
}

const Stroke& Graphics2D_ohos::getStroke() const {
    return _stroke;
}

void Graphics2D_ohos::setStrokeWidth(float w) {
    _stroke.lineWidth = w;
   OH_Drawing_PenSetWidth(_pen, w);
}

const Font* Graphics2D_ohos::getFont() const {
    return _font;
}

// 修改
void Graphics2D_ohos::setFont(const Font* font) {
    _font = static_cast<const Font_ohos*>(font);
}

void Graphics2D_ohos::translate(float dx, float dy) {
    OH_Drawing_CanvasTranslate(_canvas, dx, dy);
}

void Graphics2D_ohos::scale(float sx, float sy) {
    T[SX] *= sx;
    T[SY] *= sy;
    OH_Drawing_CanvasScale(_canvas, sx, sy);
}

void Graphics2D_ohos::rotate(float angle) {
    rotate(angle, 0, 0);
}

void Graphics2D_ohos::rotate(float angle, float px, float py) {
    float r = angle / PI * 180;
    T[R] += r;
    T[PX] = px;
    T[PY] = py;
    OH_Drawing_CanvasRotate(_canvas, r, px, py);
}

void Graphics2D_ohos::reset() {
    OH_Drawing_CanvasRotate(_canvas, -T[R], T[PX], T[PY]);
    memset_s(T, sizeof(T), 0, sizeof(T));
    T[SX] = T[SY] = 1.f;
}

float Graphics2D_ohos::sx() const {
    return T[SX];
}

float Graphics2D_ohos::sy() const {
    return T[SY];
}

void Graphics2D_ohos::drawChar(wchar_t c, float x, float y) {
    wstring str = {c, L'\0'};
    drawText(str, x, y);
}

void Graphics2D_ohos::setTextStyle(int style) {
    switch (style) {
    case PLAIN:
        OH_Drawing_SetTextStyleFontWeight(_txtStyle, FONT_WEIGHT_400);
        break;
    case BOLD:
        OH_Drawing_SetTextStyleFontWeight(_txtStyle, FONT_WEIGHT_700);
        break;
    case ITALIC:
        OH_Drawing_SetTextStyleFontStyle(_txtStyle, FONT_STYLE_ITALIC);
        break;
    case BOLDITALIC:
        OH_Drawing_SetTextStyleFontWeight(_txtStyle, FONT_WEIGHT_700);
        OH_Drawing_SetTextStyleFontStyle(_txtStyle, FONT_STYLE_ITALIC);
        break;
    default:
        OH_Drawing_SetTextStyleFontWeight(_txtStyle, FONT_WEIGHT_400);
        break;
    }
}

void Graphics2D_ohos::drawText(const wstring& t, float x, float y) {
    string tmp = wide2utf8(t.c_str());
    int len = tmp.length();
    char *str = (char *)malloc(len + 1);
    tmp.copy(str, len, 0);
    str[len] = '\0';
    tmp = _font->getFile();
    len = tmp.length();
    char *file = (char *)malloc(len + 1);
    tmp.copy(file, len, 0);
    file[len] = '\0';
    tmp = _font->getFamily();
    len = tmp.length();
    char *family = (char *)malloc(len + 1);
    tmp.copy(family, len, 0);
    family[len] = '\0';
    const char *fontFamilies[] = {family};
    float s = _font->getSize();
    OH_Drawing_SetTextStyleFontSize(_txtStyle, s);
    OH_Drawing_SetTextStyleBaseLine(_txtStyle, TEXT_BASELINE_ALPHABETIC);
    OH_Drawing_SetTextStyleFontHeight(_txtStyle, 0.1);
    setTextStyle(_font->getStyle());
    OH_Drawing_FontCollection *fontCollection = OH_Drawing_CreateFontCollection();
    if (file[0] != '\0') {
        OH_Drawing_RegisterFont(fontCollection, fontFamilies[0], file);
    }
    OH_Drawing_SetTextStyleFontFamilies(_txtStyle, 1, fontFamilies);
    OH_Drawing_SetTextStyleLocale(_txtStyle, "en");

    OH_Drawing_TypographyCreate *handler = OH_Drawing_CreateTypographyHandler(_typoStyle, fontCollection);
    OH_Drawing_TypographyHandlerPushTextStyle(handler, _txtStyle);
    OH_Drawing_TypographyHandlerAddText(handler, str);
    OH_Drawing_TypographyHandlerPopTextStyle(handler);
    OH_Drawing_Typography *typography = OH_Drawing_CreateTypography(handler);
    double maxWidth = (double)OH_Drawing_BitmapGetWidth(_bitmap);
    OH_Drawing_TypographyLayout(typography, maxWidth);

    OH_Drawing_TypographyPaint(typography, _canvas, (double)x, (double)y);

    OH_Drawing_DestroyTypography(typography);
    OH_Drawing_DestroyTypographyHandler(handler);
    OH_Drawing_DestroyFontCollection(fontCollection);

    free(str);
    free(file);
    free(family);
}

void Graphics2D_ohos::drawLine(float x1, float y1, float x2, float y2) {
    OH_Drawing_CanvasAttachPen(_canvas, _pen);
    OH_Drawing_CanvasDrawLine(_canvas, x1, y1, x2, y2);
    OH_Drawing_CanvasDetachPen(_canvas);
}

void Graphics2D_ohos::renderRect(float x, float y, float w, float h) {
    OH_Drawing_CanvasAttachPen(_canvas, _pen);
    OH_Drawing_Rect *rect = OH_Drawing_RectCreate(x, y, x + w, y + h);
    OH_Drawing_CanvasDrawRect(_canvas, rect);
    OH_Drawing_RectDestroy(rect);
    OH_Drawing_CanvasDetachPen(_canvas);
}

void Graphics2D_ohos::drawRect(float x, float y, float w, float h) {
    renderRect(x, y, w, h);
}

void Graphics2D_ohos::fillRect(float x, float y, float w, float h) {
    float th = _stroke.lineWidth;
    setStrokeWidth(0.f);
    OH_Drawing_CanvasAttachBrush(_canvas, _brush);
    renderRect(x, y, w, h);
    OH_Drawing_CanvasDetachBrush(_canvas);
    setStrokeWidth(th);
}

void Graphics2D_ohos::renderRoundRect(float x, float y, float w, float h, float rx, float ry) {
    OH_Drawing_Rect *rect = OH_Drawing_RectCreate(x, y, x + w, y + h);
    OH_Drawing_RoundRect *roundRect = OH_Drawing_RoundRectCreate(rect, rx, ry);
    OH_Drawing_CanvasDrawRoundRect(_canvas, roundRect);
    OH_Drawing_RoundRectDestroy(roundRect);
    OH_Drawing_RectDestroy(rect);
}

void Graphics2D_ohos::drawRoundRect(float x, float y, float w, float h, float rx, float ry) {
    renderRoundRect(x, y, w, h, rx, ry);
}

void Graphics2D_ohos::fillRoundRect(float x, float y, float w, float h, float rx, float ry) {
    float th = _stroke.lineWidth;
    setStrokeWidth(0.f);
    OH_Drawing_CanvasAttachBrush(_canvas, _brush);
    renderRoundRect(x, y, w, h, rx, ry);
    OH_Drawing_CanvasDetachBrush(_canvas);
    setStrokeWidth(th);
}

void Graphics2D_ohos::drawEllipse(
    float x, float y,
    float w, float h,
    float rx, float ry,
    float lineWidth,
    float depth
) {
    // 计算椭圆中心
    float cx = x + w / 2;
    float cy = y - h / 2 + depth / 2;

    // 计算包围椭圆的矩形
    float left   = cx - rx;
    float right  = cx + rx;
    float top    = cy - ry;
    float bottom = cy + ry;

    // 保存原先的线宽
    float th = _stroke.lineWidth;

    // 设置新的线宽
    setStrokeWidth(lineWidth);

    // 绘制椭圆
    OH_Drawing_CanvasAttachPen(_canvas, _pen);
    OH_Drawing_Rect* rect = OH_Drawing_RectCreate(left, top, right, bottom);
    OH_Drawing_CanvasDrawOval(_canvas, rect);
    OH_Drawing_RectDestroy(rect);
    OH_Drawing_CanvasDetachPen(_canvas);

    // 恢复原先的线宽
    setStrokeWidth(th);
}

#endif  // __OS_OHOS__

#include "config.h"

#if defined(__OS_ohos__) && !defined(MEM_CHECK)

#ifndef GRAPHIC_OHOS_H_INCLUDED
#define GRAPHIC_OHOS_H_INCLUDED

#include "graphic/graphic.h"
#include <native_drawing/drawing_bitmap.h>
#include <native_drawing/drawing_text_typography.h>
#include <native_drawing/drawing_pen.h>
#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_canvas.h>
#include <native_drawing/drawing_font_collection.h>
#include <native_drawing/drawing_rect.h>
#include <native_drawing/drawing_round_rect.h>
#include <native_drawing/drawing_register_font.h>
#include <native_drawing/drawing_text_typography.h>

using namespace std;

namespace tex {

class Font_ohos : public Font {
private:
    static map<string, string> _file_name_map;

    int _style;
    float _size;
    string _family;
    string _file;

    void loadFont(const string &file);

public:
    Font_ohos(const string& family = "", int style = PLAIN, float size = 1.f);

    Font_ohos(const string& file, float size);

    string getFile() const;

    string getFamily() const;

    int getStyle() const;

    virtual float getSize() const override;

    virtual sptr<Font> deriveFont(int style) const override;

    virtual bool operator==(const Font& f) const override;

    virtual bool operator!=(const Font& f) const override;
};

/**************************************************************************************************/

class TextLayout_ohos : public TextLayout {
private:
    sptr<Font_ohos> _font;
    const wstring _txt;

public:
    TextLayout_ohos(const wstring& src, const sptr<Font_ohos>& font);

    virtual void getBounds(_out_ Rect& r) override;

    virtual void draw(Graphics2D& g2, float x, float y) override;
};

/**************************************************************************************************/

class Graphics2D_ohos : public Graphics2D {
private:
    color _color;
    Stroke _stroke;
    const Font_ohos* _font;
    float T[9] = {1, 0, 0, 0, 1, 0, 0, 0, 0};
    int SX = 0;
    int SY = 4;
    int TX = 2;
    int TY = 5;
    int R = 6;
    int PX = 7;
    int PY = 8;

    OH_Drawing_Bitmap *_bitmap;
    OH_Drawing_Canvas *_canvas;
    OH_Drawing_Pen *_pen;
    OH_Drawing_Brush *_brush;
    OH_Drawing_TextStyle *_txtStyle;
    OH_Drawing_TypographyStyle *_typoStyle;

    void renderRect(float x, float y, float w, float h);
    void renderRoundRect(float x, float y, float w, float h, float rx, float ry);
    void setTextStyle(int style);

public:
    Graphics2D_ohos(OH_Drawing_Bitmap *bitmap, uint32_t foreground);

    ~Graphics2D_ohos();

    virtual void setColor(color c) override;

    virtual color getColor() const override;

    virtual void setStroke(const Stroke& s) override;

    virtual const Stroke& getStroke() const override;

    virtual void setStrokeWidth(float w) override;

    virtual const Font* getFont() const override;

    virtual void setFont(const Font* font) override;

    virtual void translate(float dx, float dy) override;

    virtual void scale(float sx, float sy) override;

    virtual void rotate(float angle) override;

    virtual void rotate(float angle, float px, float py) override;

    virtual void reset() override;

    virtual float sx() const override;

    virtual float sy() const override;

    virtual void drawChar(wchar_t c, float x, float y) override;

    virtual void drawText(const wstring& t, float x, float y) override;

    virtual void drawLine(float x, float y1, float x2, float y2) override;

    virtual void drawRect(float x, float y, float w, float h) override;

    virtual void fillRect(float x, float y, float w, float h) override;

    virtual void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override;

    virtual void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override;

    virtual void drawEllipse(float x, float y, float w, float h, float rx, float ry, float lineWidth, float depth) override;
};

}  // namespace tex

#endif  // GRAPHIC_OHOS_H_INCLUDED
#endif  // __OS_ohos__ && !MEM_CHECK

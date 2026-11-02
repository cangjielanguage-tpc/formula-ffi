#include "config.h"

#ifdef MEM_CHECK

#include "graphic/graphic.h"
#include "latex.h"

namespace tex {

class Font_none : public Font {
public:
    Font_none() {}

    float getSize() const override {
        return 1.f;
    }

    sptr<Font> deriveFont(int style) const override {
        return sptr<Font>(new Font_none());
    }

    bool operator==(const Font& f) const override {
        return false;
    }

    bool operator!=(const Font& f) const override {
        return !(*this == f);
    }
};

Font* Font::create(const string& file, float size) {
    return new Font_none();
}

sptr<Font> Font::_create(const string& name, int style, float size) {
    return sptr<Font>(new Font_none());
}

/**************************************************************************************************/

class TextLayout_none : public TextLayout {
public:
    TextLayout_none() {}

    void getBounds(_out_ Rect& bounds) override {
        bounds.x = bounds.y = bounds.w = bounds.h = 0.f;
    }

    void draw(Graphics2D& g2, float x, float y) override {
    }
};

sptr<TextLayout> TextLayout::create(const wstring& src, const sptr<Font>& font) {
    return sptr<TextLayout>(new TextLayout_none());
}

/**************************************************************************************************/

class Graphics2D_none : public Graphics2D {
private:
    static Font* _default_font;
    const Font* _font;
    Stroke _stroke;

public:
    Graphics2D_none() : _font(_default_font), _stroke() {}

    static void release() {
        delete _default_font;
    }

    void setColor(color c) override {
    }

    color getColor() const override {
        return 0;
    }

    void setStroke(const Stroke& s) override {
        _stroke = s;
    }

    const Stroke& getStroke() const override {
        return _stroke;
    }

    void setStrokeWidth(float w) override {
    }

    const Font* getFont() const override {
        return _font;
    }

    void setFont(const Font* font) override {
        _font = font;
    }

    void translate(float dx, float dy) override {
    }

    void scale(float sx, float sy) override {
    }

    void rotate(float angle) override {
    }

    void rotate(float angle, float px, float py) override {
    }

    void reset() override {
    }

    float sx() const override {
        return 1.f;
    }

    float sy() const override {
        return 1.f;
    }

    void drawChar(wchar_t c, float x, float y) override {
    }

    void drawText(const wstring& c, float x, float y) override {
    }

    void drawLine(float x1, float y1, float x2, float y2) override {
    }

    void drawRect(float x, float y, float w, float h) override {
    }

    void fillRect(float x, float y, float w, float h) override {
    }

    void drawRoundRect(float x, float y, float w, float h, float rx, float ry) override {
    }

    void fillRoundRect(float x, float y, float w, float h, float rx, float ry) override {
    }

    void drawEllipse(float x, float y, float w, float h, float rx, float ry, float lineWidth, float depth) override {
    }
};

Font* Graphics2D_none::_default_font = new Font_none();

}  // namespace tex

#include "samples/samples.h"

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static std::wstring readUtf8ToWide(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return L"";
    std::string content;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) content.append(buf, n);
    fclose(f);
#ifdef _WIN32
    int wlen = MultiByteToWideChar(CP_UTF8, 0, content.data(), (int)content.size(), nullptr, 0);
    std::wstring w((size_t)wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, content.data(), (int)content.size(), &w[0], wlen);
    return w;
#else
    // UTF-8 -> UTF-32 (Linux wchar_t)
    std::wstring w;
    size_t i = 0;
    while (i < content.size()) {
        unsigned char c = (unsigned char)content[i];
        unsigned int cp = 0;
        int extra = 0;
        if (c < 0x80) {
            cp = c;
        } else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F;
            extra = 1;
        } else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F;
            extra = 2;
        } else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07;
            extra = 3;
        } else {
            cp = 0xFFFD;
        }
        for (int k = 0; k < extra && i + 1 < content.size(); k++) {
            i++;
            cp = (cp << 6) | ((unsigned char)content[i] & 0x3F);
        }
        w.push_back((wchar_t)cp);
        i++;
    }
    return w;
#endif
}

int main(int argc, char* argv[]) {
    LaTeX::init();

    if (argc > 1) {
        std::wstring code = readUtf8ToWide(argv[1]);
        printf("input len=%zu, start parse...\n", code.size());
        fflush(stdout);
        try {
            auto r = LaTeX::parse(code, 2000, 14, 14 / 3.f, black);
            printf("parse ok, drawing...\n");
            fflush(stdout);
            Graphics2D_none g2;
            r->draw(g2, 0, 0);
            delete r;
            printf("draw ok\n");
        } catch (std::exception& e) {
            printf("exception: %s\n", e.what());
        }
        fflush(stdout);
        LaTeX::release();
        Graphics2D_none::release();
        return 0;
    }

    tex::Samples samples;
    for (int i = 0; i < samples.count(); i++) {
        auto r = LaTeX::parse(samples.next(), 720, 20, 20 / 3.f, black);
        Graphics2D_none g2;
        r->draw(g2, 0, 0);
        delete r;
    }

    LaTeX::release();
    Graphics2D_none::release();
    return 0;
}

#endif  // MEM_CHECK

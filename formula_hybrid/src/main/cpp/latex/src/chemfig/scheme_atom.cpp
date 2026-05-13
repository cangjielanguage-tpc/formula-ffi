#include "scheme_atom.h"
#include "chemfig_constants.h"
#include "core/core.h"
#include "fonts/fonts.h"
#include "common.h"

namespace tex {

SchemeAtom::SchemeAtom(const std::wstring& code)
    : _code(code) {
    if (!SchemeParser::parse(_code, _scheme)) {
        throw ex_parse("Failed to parse reaction scheme: " + chemfig::wstringToUtf8(code));
    }
}

sptr<Box> SchemeAtom::createBox(TeXEnvironment& env) {
    color c = env.getColor();

    sptr<TeXFont> tf = env.getTeXFont();
    DefaultTeXFont* dtf = dynamic_cast<DefaultTeXFont*>(tf.get());
    int type = PLAIN;
    if (dtf != nullptr) {
        type = (dtf->_isIt ? ITALIC : PLAIN) | (dtf->_isBold ? BOLD : 0);
    }
    float fontSize = 10.f;
    sptr<Font> font = Font::_create("serif", type, fontSize);
    if (dtf != nullptr && dtf->_isSs) {
        font = Font::_create("sans-serif", type, fontSize);
    }
    if (font == nullptr) {
        throw ex_parse("Scheme: failed to create font");
    }

    float sizeFactor = DefaultTeXFont::getSizeFactor(env.getStyle());

    return sptr<Box>(new SchemeBox(_scheme, c, font, sizeFactor, env));
}

} // namespace tex

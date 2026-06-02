#include "chemfig_atom.h"
#include "chemfig_constants.h"
#include "core/core.h"
#include "fonts/fonts.h"
#include "common.h"

namespace tex {

ChemfigAtom::ChemfigAtom(const std::wstring& code)
    : _code(code) {
    if (!ChemfigParser::parse(_code, _molecule)) {
        throw ex_parse("Failed to parse chemfig code: " + chemfig::wstringToUtf8(code));
    }
}

sptr<Box> ChemfigAtom::createBox(TeXEnvironment& env) {
    color c = env.getColor();
    
    if (_molecule.atoms.empty()) {
        throw ex_parse("Chemfig molecule has no atoms");
    }
    
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
        throw ex_parse("Chemfig: failed to create font");
    }
    
    float sizeFactor = DefaultTeXFont::getSizeFactor(env.getStyle());
    
    return sptr<Box>(new ChemfigBox(_molecule, c, font, sizeFactor));
}

} // namespace tex

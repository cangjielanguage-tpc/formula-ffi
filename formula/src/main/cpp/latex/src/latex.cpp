#include "latex.h"
#include "core/core.h"
#include "core/formula.h"
#include "core/macro.h"
#include "fonts/fonts.h"

using namespace std;
using namespace tex;

string tex::RES_BASE = "res";

TeXFormula* LaTeX::_formula = nullptr;
TeXRenderBuilder* LaTeX::_builder = nullptr;

void LaTeX::init(const string& res_root_path) {
    RES_BASE = res_root_path;
    if (_formula != nullptr) return;

    NewCommandMacro::_init_();
    DefaultTeXFont::_init_();
    SymbolAtom::_init_();
    Glue::_init_();
    TeXFormula::_init_();
    TextRenderingBox::_init_();

    _formula = new TeXFormula();
    _builder = new TeXRenderBuilder();
}

void LaTeX::release() {
    DefaultTeXFont::_free_();
    TeXFormula::_free_();
    NewCommandMacro::_free_();
    TextRenderingBox::_free_();

    if (_formula != nullptr) {
        delete _formula;
        _formula = nullptr;
    }
    if (_builder != nullptr) {
        delete _builder;
        _builder = nullptr;
    }
}

void LaTeX::releaseGlueAndMacroInfo() {
    Glue::_free_();
    MacroInfo::_free_();
}

const string& LaTeX::getResRootPath() {
    return RES_BASE;
}

void LaTeX::setDebug(bool debug) {
    TeXFormula::setDEBUG(debug);
}

TeXRender* LaTeX::parse(const wstring& latex, int width, float textSize, float lineSpace, color fg) {
    if (_formula == nullptr) {
        throw ex_parse("LaTeX formula object is not initialized. Call LaTeX::init() first.");
    }
    if (_builder == nullptr) {
        throw ex_parse("LaTeX builder object is not initialized. Call LaTeX::init() first.");
    }

    bool lined = true;
    if (startswith(latex, L"$$") || startswith(latex, L"\\[")) {
        lined = false;
    }
    int align = lined ? ALIGN_LEFT : ALIGN_CENTER;
    _formula->setLaTeX(latex);
    TeXRender* render =
        _builder->setStyle(STYLE_DISPLAY)
            .setTextSize(textSize)
            .setWidth(UNIT_PIXEL, width, align)
            .setIsMaxWidth(lined)
            .setLineSpace(UNIT_PIXEL, lineSpace)
            .setForeground(fg)
            .build(*_formula);
    if (render == nullptr) {
        throw ex_parse("Failed to build TeXRender from formula");
    }
    return render;
}

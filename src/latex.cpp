#include "latex.h"
#include "core/core.h"
#include "core/formula.h"
#include "core/macro.h"
#include "fonts/fonts.h"
#include <unordered_set>

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

TeXRender* LaTeX::parse(const wstring& latex, int width, float textSize, float lineSpace, color fg, bool isPartial) {
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
    _formula->setIsPartial(isPartial);
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

// 跳过花括号内容，返回跳过后的位置，括号不匹配返回npos
static inline size_t skipBraces(const wstring& s, size_t p, size_t n) {
    int b = 1;
    while (p < n && b > 0) b += (s[p] == L'{') - (s[p++] == L'}');
    return b ? wstring::npos : p;
}

// 跳过方括号内容，返回跳过后的位置，括号不匹配返回npos
static inline size_t skipBrackets(const wstring& s, size_t p, size_t n) {
    int b = 1;
    while (p < n && b > 0) b += (s[p] == L'[') - (s[p++] == L']');
    return b ? wstring::npos : p;
}

// ============================================================================
// LaTeX 公式验证模块
// ============================================================================

namespace {

// 跳过空白字符
// 参数: s - 输入字符串, p - 当前位置引用, n - 字符串长度
static inline void skipWS(const wstring& s, size_t& p, size_t n) {
    while (p < n && (s[p] == L' ' || s[p] == L'\t')) p++;
}

// 数组/矩阵类环境名称集合
// 规则: 这些环境支持 & 分隔符和 \\ 换行符
static const unordered_set<wstring> ARRAY_ENVS = {
    L"matrix", L"pmatrix", L"bmatrix", L"Bmatrix",
    L"vmatrix", L"Vmatrix", L"array", L"cases",
    L"aligned", L"gathered", L"split", L"smallmatrix",
    L"align", L"flalign", L"alignat", L"gather", L"multline"
};

static const unordered_set<wstring> ENV_WITH_ARG = {
    L"array", L"alignat", L"alignedat"
};

// 重音命令名称集合
// 规则: 这些命令需要一个参数
static const unordered_set<wstring> ACCENT_COMMANDS = {
    L"hat", L"widehat", L"tilde", L"widetilde", L"acute", L"grave", L"ddot", L"ddddot", L"dot", L"bar",
    L"breve", L"check", L"vec", L"mathring", L"overline", L"underline", L"overbrace", L"underbrace",
    L"overrightarrow", L"overleftarrow", L"overleftrightarrow", L"underrightarrow", L"underleftarrow",
    L"underleftrightarrow", L"overbrack", L"underbrack", L"overparen", L"underparen", L"undertilde"
};

// 堆叠命令名称集合
// 规则: 这些命令需要2-3个参数
static const unordered_set<wstring> STACK_COMMANDS = {
    L"stackrel", L"stackbin", L"overset", L"underset", L"sideset", L"prescript",
    L"accentset", L"underaccent"
};

// 盒子命令名称集合
// 规则: 这些命令需要1-3个参数
static const unordered_set<wstring> BOX_COMMANDS = {
    L"fbox", L"boxed", L"colorbox", L"fcolorbox", L"shadowbox", L"ovalbox", L"doublebox", L"smash"
};

// 图形变换命令名称集合
// 规则: 这些命令需要1-3个参数
static const unordered_set<wstring> TRANSFORM_COMMANDS = {
    L"rotatebox", L"scalebox", L"resizebox", L"reflectbox"
};

// 文本命令名称集合
// 规则: 这些命令需要1个参数
static const unordered_set<wstring> TEXT_COMMANDS = {
    L"text", L"mbox", L"intertext", L"textit", L"textbf", L"textsf", L"texttt",
    L"textrm", L"textsc", L"textsuperscript", L"textsubscript", L"textcircled"
};

// 字体命令名称集合
// 规则: 这些命令需要1个参数
static const unordered_set<wstring> FONT_COMMANDS = {
    L"mathbf", L"mathit", L"mathrm", L"mathsf", L"mathtt", L"mathbb", L"mathcal",
    L"mathscr", L"mathfrak", L"mathds", L"boldsymbol", L"bold", L"cal", L"frak",
    L"Bbb", L"oldstylenums"
};

// 数学类型声明命令名称集合
// 规则: 这些命令需要1个参数
static const unordered_set<wstring> MATH_TYPE_COMMANDS = {
    L"mathop", L"mathbin", L"mathrel", L"mathopen", L"mathclose", L"mathpunct",
    L"mathord", L"mathinner"
};

// 其他需要参数的命令名称集合
// 规则: 这些命令需要1个参数
static const unordered_set<wstring> OTHER_ARG_COMMANDS = {
    L"operatorname", L"phantom", L"hphantom", L"vphantom", L"llap", L"rlap", L"clap",
    L"mathllap", L"mathrlap", L"mathclap", L"cancel", L"bcancel", L"xcancel", L"st",
    L"includegraphics", L"Braket", L"Set", L"Bra", L"Ket", L"char",
    L"roman", L"Roman", L"XML", L"dynamic", L"externalFont", L"cornersize"
};

// \left/\right 支持的分隔符命令集合
static const unordered_set<wstring> DELIMITER_COMMANDS = {
    L"{", L"}", L"|", L"langle", L"rangle", L"lfloor", L"rfloor",
    L"lceil", L"rceil", L"uparrow", L"downarrow", L"updownarrow",
    L"Uparrow", L"Downarrow", L"Updownarrow"
};

// 判断是否为数组/矩阵类环境
static inline bool isArrayEnv(const wstring& e) {
    return ARRAY_ENVS.find(e) != ARRAY_ENVS.end();
}

// 判断是否为重音命令
static inline bool isAccentCommand(const wstring& cmd) {
    return ACCENT_COMMANDS.find(cmd) != ACCENT_COMMANDS.end();
}

// 判断是否为有效的分隔符命令
static inline bool isValidDelimiterCommand(const wstring& cmd) {
    return DELIMITER_COMMANDS.find(cmd) != DELIMITER_COMMANDS.end();
}

// 判断是否为有效的单字符分隔符
// 支持的单字符分隔符: ( ) [ ] . | < >
static inline bool isValidDelimiterChar(wchar_t c) {
    return c == L'(' || c == L')' || c == L'[' || c == L']' || 
           c == L'.' || c == L'|' || c == L'<' || c == L'>';
}

// 判断是否为矩阵环境
static inline bool isMatrixEnv(const wstring& e) {
    return e == L"matrix" || e == L"pmatrix" || e == L"bmatrix" || e == L"Bmatrix" ||
           e == L"vmatrix" || e == L"Vmatrix";
}

// 构建带位置信息的错误消息
static string buildErrorMsg(const string& msg, size_t pos, const wstring& formula) {
    return msg + " at position " + to_string(pos);
}

// 验证 \frac 命令
// 规则: \frac{分子}{分母} 必须完整，分子和分母不能为空
// 示例: \frac{a}{b} 合法, \frac{}{b} 非法, \frac{a}{} 非法
// 异常: 解析错误时抛出 ex_parse
static size_t validateFrac(const wstring& latex, size_t p, size_t n) {
    skipWS(latex, p, n);
    if (p >= n || latex[p] != L'{') 
        throw ex_parse(buildErrorMsg("\\frac requires numerator in braces", p, latex));
    
    size_t startNumerator = p + 1;
    if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
        throw ex_parse(buildErrorMsg("\\frac numerator not closed", startNumerator, latex));
    
    skipWS(latex, p, n);
    if (p >= n || latex[p] != L'{') 
        throw ex_parse(buildErrorMsg("\\frac requires denominator in braces", p, latex));
    
    size_t startDenominator = p + 1;
    if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
        throw ex_parse(buildErrorMsg("\\frac denominator not closed", startDenominator, latex));
    
    return p;
}

static size_t validateSqrt(const wstring& latex, size_t p, size_t n) {
    skipWS(latex, p, n);
    
    if (p < n && latex[p] == L'[') {
        size_t startBracket = p + 1;
        if ((p = skipBrackets(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("\\sqrt brackets not closed", startBracket, latex));
    }
    
    skipWS(latex, p, n);
    if (p >= n || latex[p] != L'{') 
        throw ex_parse(buildErrorMsg("\\sqrt requires radicand in braces", p, latex));
    
    size_t startBrace = p + 1;
    if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
        throw ex_parse(buildErrorMsg("\\sqrt radicand not closed", startBrace, latex));
    
    return p;
}

static size_t validateAccentCommand(const wstring& latex, size_t p, size_t n, const wstring& cmd) {
    skipWS(latex, p, n);
    if (p >= n) 
        throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires an argument", p, latex));
    
    wchar_t c = latex[p];
    if (c == L'{') {
        size_t startBrace = p + 1;
        if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " argument not closed", startBrace, latex));
    } else if (c == L'\\') {
        p++;
        while (p < n && isalpha(latex[p])) p++;
    } else {
        p++;
    }
    
    return p;
}

static size_t validateSingleArgCommand(const wstring& latex, size_t p, size_t n, const wstring& cmd) {
    skipWS(latex, p, n);
    if (p >= n) 
        throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires an argument", p, latex));
    
    if (latex[p] == L'{') {
        size_t startBrace = p + 1;
        if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " argument not closed", startBrace, latex));
    } else {
        p++;
    }
    
    return p;
}

static size_t validateBoxCommand(const wstring& latex, size_t p, size_t n, const wstring& cmd) {
    skipWS(latex, p, n);
    if (p >= n) 
        throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires arguments", p, latex));
    
    if (latex[p] == L'{') {
        size_t startBrace = p + 1;
        if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " first argument not closed", startBrace, latex));
        
        skipWS(latex, p, n);
        if (cmd == L"colorbox" || cmd == L"fcolorbox" || cmd == L"scalebox" || 
            cmd == L"resizebox" || cmd == L"rotatebox") {
            if (p >= n || latex[p] != L'{') 
                throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires second argument in braces", p, latex));
            
            startBrace = p + 1;
            if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
                throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " second argument not closed", startBrace, latex));
            
            if (cmd == L"fcolorbox" || cmd == L"resizebox") {
                skipWS(latex, p, n);
                if (p >= n || latex[p] != L'{') 
                    throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires third argument in braces", p, latex));
                
                startBrace = p + 1;
                if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
                    throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " third argument not closed", startBrace, latex));
            }
        }
    } else {
        p++;
    }
    
    return p;
}

static size_t validateStackCommand(const wstring& latex, size_t p, size_t n, const wstring& cmd) {
    skipWS(latex, p, n);
    if (p >= n) 
        throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires arguments", p, latex));
    
    if (latex[p] == L'{') {
        size_t startBrace = p + 1;
        if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " first argument not closed", startBrace, latex));
        
        skipWS(latex, p, n);
        if (cmd == L"stackrel" || cmd == L"stackbin" || cmd == L"overset" || cmd == L"underset" || 
            cmd == L"accentset" || cmd == L"underaccent" || cmd == L"sideset" || cmd == L"prescript") {
            if (p >= n || latex[p] != L'{') 
                throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires second argument in braces", p, latex));
            
            startBrace = p + 1;
            if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
                throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " second argument not closed", startBrace, latex));
        }
        
        if (cmd == L"sideset" || cmd == L"prescript") {
            skipWS(latex, p, n);
            if (p >= n || (latex[p] != L'{' && latex[p] != L'[')) 
                throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " requires third argument in braces or brackets", p, latex));
            
            startBrace = p + 1;
            if (latex[p] == L'{') {
                if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
                    throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " third argument not closed", startBrace, latex));
            } else {
                if ((p = skipBrackets(latex, p + 1, n)) == wstring::npos) 
                    throw ex_parse(buildErrorMsg("\\" + wide2utf8(cmd.c_str()) + " third argument not closed", startBrace, latex));
            }
        }
    } else {
        p++;
    }
    
    return p;
}

static void validateMatrixColumns(const wstring& content, const wstring& env) {
    if (env == L"gathered" || env == L"aligned" || env == L"split" || 
        env == L"align" || env == L"flalign" || env == L"alignat" || 
        env == L"gather" || env == L"multline") {
        return;
    }
    
    vector<int> cols;
    int c = 0, depth = 0;
    
    for (size_t i = 0; i < content.length(); i++) {
        wchar_t ch = content[i];
        if (ch == L'{') depth++;
        else if (ch == L'}') depth--;
        else if (ch == L'&' && depth == 0) c++;
        else if (ch == L'\\' && i + 1 < content.length() && content[i + 1] == L'\\') {
            cols.push_back(c + 1);
            c = 0;
            i++;
        }
    }
    cols.push_back(c + 1);
    
    for (size_t i = 1; i < cols.size(); i++) {
        if (cols[i] != cols[0]) 
            throw ex_parse("Matrix columns inconsistent: row 1 has " + to_string(cols[0]) + 
                          " columns, but row " + to_string(i + 1) + " has " + to_string(cols[i]) + " columns");
    }
    
    if (env == L"array") {
        size_t fp = content.find(L'{');
        size_t fe = content.find(L'}', fp);
        if (fp != wstring::npos && fe != wstring::npos) {
            int formatCols = 0;
            for (wchar_t ch : content.substr(fp + 1, fe - fp - 1)) {
                if (ch == L'c' || ch == L'l' || ch == L'r') formatCols++;
            }
            if (formatCols > 0 && !cols.empty() && cols[0] != formatCols) 
                throw ex_parse("Array columns mismatch: format specifies " + to_string(formatCols) + 
                              " columns, but content has " + to_string(cols[0]) + " columns");
        }
    }
    
    if (env == L"cases") {
        for (size_t i = 0; i < cols.size(); i++) {
            if (cols[i] != 2) 
                throw ex_parse("cases environment requires exactly 2 columns per row, but row " + 
                              to_string(i + 1) + " has " + to_string(cols[i]) + " columns");
        }
    }
}

static size_t findMatchingEnd(const wstring& latex, size_t p, size_t n, const wstring& env) {
    int depth = 1;
    wstring beginCmd = L"\\begin{" + env + L"}";
    wstring endCmd = L"\\end{" + env + L"}";
    
    while (p < n && depth > 0) {
        if (latex[p] == L'\\') {
            size_t cmdStart = p;
            p++;
            wstring cmd;
            while (p < n && isalpha(latex[p])) cmd += latex[p++];
            
            if (cmd == L"begin") {
                skipWS(latex, p, n);
                if (p < n && latex[p] == L'{') {
                    p++;
                    wstring innerEnv;
                    while (p < n && latex[p] != L'}') innerEnv += latex[p++];
                    if (p < n) p++;
                    if (innerEnv == env) depth++;
                }
            } else if (cmd == L"end") {
                skipWS(latex, p, n);
                if (p < n && latex[p] == L'{') {
                    p++;
                    wstring innerEnv;
                    while (p < n && latex[p] != L'}') innerEnv += latex[p++];
                    if (p < n) p++;
                    if (innerEnv == env) {
                        depth--;
                        if (depth == 0) return cmdStart;
                    }
                }
            }
        } else {
            p++;
        }
    }
    
    return wstring::npos;
}

static size_t validateBegin(const wstring& latex, size_t p, size_t n, int& arrayDepth) {
    skipWS(latex, p, n);
    if (p >= n || latex[p] != L'{') 
        throw ex_parse(buildErrorMsg("\\begin requires environment name in braces", p, latex));
    
    wstring env;
    p++;
    while (p < n && latex[p] != L'}') env += latex[p++];
    if (p >= n) 
        throw ex_parse(buildErrorMsg("\\begin environment name not closed", p, latex));
    p++;
    
    if (ENV_WITH_ARG.find(env) != ENV_WITH_ARG.end()) {
        skipWS(latex, p, n);
        if (p < n && latex[p] == L'{') {
            p++;
            int depth = 1;
            while (p < n && depth > 0) {
                if (latex[p] == L'{') depth++;
                else if (latex[p] == L'}') depth--;
                p++;
            }
        }
    }
    
    size_t endPos = findMatchingEnd(latex, p, n, env);
    if (endPos == wstring::npos) 
        throw ex_parse(buildErrorMsg("\\begin{" + wide2utf8(env.c_str()) + "} has no matching \\end", p, latex));
    
    if (isArrayEnv(env)) {
        arrayDepth++;
        validateMatrixColumns(latex.substr(p, endPos - p), env);
    }
    
    return p;
}

// 验证 \end 命令
static size_t validateEnd(const wstring& latex, size_t p, size_t n, int& arrayDepth) {
    skipWS(latex, p, n);
    if (p < n && latex[p] == L'{') {
        wstring env;
        p++;
        while (p < n && latex[p] != L'}') env += latex[p++];
        if (p < n) p++;
        if (isArrayEnv(env) && arrayDepth > 0) arrayDepth--;
    }
    return p;
}

// 验证分隔符命令 (\left 或 \right 后的分隔符)
static size_t validateDelimiter(const wstring& latex, size_t p, size_t n, const string& context) {
    if (p >= n) 
        throw ex_parse(buildErrorMsg(context + " requires delimiter", p, latex));
    
    if (latex[p] == L'\\') {
        p++;
        wstring dc;
        while (p < n && isalpha(latex[p])) dc += latex[p++];
        
        if (!dc.empty() && !isValidDelimiterCommand(dc)) 
            throw ex_parse(buildErrorMsg(context + " invalid delimiter: \\" + wide2utf8(dc.c_str()), p, latex));
        
        if (dc.empty() && p < n && latex[p] != L'{' && latex[p] != L'}' && latex[p] != L'|') 
            throw ex_parse(buildErrorMsg(context + " invalid delimiter", p, latex));
        if (dc.empty() && p < n) p++;
    } else if (isValidDelimiterChar(latex[p])) {
        p++;
    } else {
        throw ex_parse(buildErrorMsg(context + " invalid delimiter: " + string(1, (char)latex[p]), p, latex));
    }
    
    return p;
}

// 验证 \left 命令并查找匹配的 \right
static size_t validateLeft(const wstring& latex, size_t p, size_t n) {
    skipWS(latex, p, n);
    p = validateDelimiter(latex, p, n, "\\left");
    
    // 查找匹配的 \right
    int leftCount = 1;
    for (size_t i = p; i < n && leftCount > 0; ) {
        if (latex[i] == L'\\') {
            i++;
            wstring nc;
            while (i < n && isalpha(latex[i])) nc += latex[i++];
            if (nc == L"left") leftCount++;
            else if (nc == L"right") leftCount--;
        } else {
            i++;
        }
    }
    
    if (leftCount > 0) 
        throw ex_parse(buildErrorMsg("\\left has no matching \\right", p, latex));
    
    return p;
}

// 验证 \right 命令
static size_t validateRight(const wstring& latex, size_t p, size_t n) {
    skipWS(latex, p, n);
    return validateDelimiter(latex, p, n, "\\right");
}

static size_t validateSubscriptSuperscript(const wstring& latex, size_t p, size_t n, wchar_t& lastSubSup) {
    wchar_t c = latex[p];
    
    if (lastSubSup == c) 
        throw ex_parse(buildErrorMsg("Double subscript/superscript", p, latex));
    
    lastSubSup = c;
    p++;
    
    if (p >= n) 
        throw ex_parse(buildErrorMsg("Subscript/superscript has no content", p, latex));
    
    skipWS(latex, p, n);
    
    if (p >= n) 
        throw ex_parse(buildErrorMsg("Subscript/superscript has no content", p, latex));
    
    wchar_t next = latex[p];
    
    if (next == L'_' || next == L'^') 
        throw ex_parse(buildErrorMsg("Subscript/superscript has no content", p, latex));
    
    if (next == L'{') {
        lastSubSup = 0;
        size_t startBrace = p + 1;
        if ((p = skipBraces(latex, p + 1, n)) == wstring::npos) 
            throw ex_parse(buildErrorMsg("Subscript/superscript braces not closed", startBrace, latex));
    } else {
        p++;
    }
    
    return p;
}

} // anonymous namespace

/**
 * @brief 校验LaTeX公式语法（遵循标准LaTeX规则）
 * 
 * 本函数采用状态机模式逐字符扫描LaTeX公式，对常见语法错误进行预检测，
 * 在实际渲染前捕获问题，提供精确的错误位置信息。
 * 
 * @section algorithm 算法说明
 * - 时间复杂度: O(n)，n为公式长度
 * - 空间复杂度: O(1)，仅使用常量状态变量
 * - 采用单遍扫描，维护花括号平衡、数组深度、上下标状态
 * 
 * @section rules 校验规则
 * 
 * **1. 分数命令 \frac**
 * - 格式: \frac{分子}{分母}
 * - 合法: \frac{a}{b}, \frac{x+1}{x-1}, \frac{}{b}, \frac{a}{}
 * - 非法: \frac{a}{, \frac a b, \frac
 * 
 * **2. 根号命令 \sqrt**
 * - 格式: \sqrt{内容} 或 \sqrt[n]{内容}
 * - 合法: \sqrt{x}, \sqrt[3]{x}, \sqrt{}, \sqrt[]{x}
 * - 非法: \sqrt, \sqrt{x
 * 
 * **3. 环境匹配 \begin/\end**
 * - 要求: \begin{xxx} 必须有对应的 \end{xxx}
 * - 支持嵌套匹配
 * - 合法: \begin{matrix}a\end{matrix}
 * - 非法: \begin{matrix}a, \begin{matrix}a\end{pmatrix}
 * 
 * **4. 分隔符配对 \left/\right**
 * - 要求: \left 必须配对 \right
 * - 支持混合分隔符: \left( x \right]
 * - 合法: \left( x \right), \left. x \right|
 * - 非法: \left( x, \right)
 * 
 * **5. 上下标 _ 和 ^**
 * - 要求: 后必须有内容，禁止双重相同类型
 * - 合法: x_1, x^2, x_1^2, x_{n+1}^{2k}
 * - 非法: x_, x^, x_1_2, x^2^3
 * 
 * **6. 花括号匹配**
 * - 要求: {} 必须成对
 * - 合法: {a}, {a{b}c}
 * - 非法: {a, a}, {a{b}
 * 
 * **7. 对齐符 &**
 * - 要求: 只能在矩阵/数组环境中使用
 * - 合法: \begin{matrix}a & b\end{matrix}
 * - 非法: a & b, x_1 & x_2
 * 
 * **8. 矩阵列数一致性**
 * - 要求: 每行列数相同（gathered/aligned/split除外）
 * - 合法: \begin{matrix}a & b\\c & d\end{matrix}
 * - 非法: \begin{matrix}a & b\\c\end{matrix}
 * 
 * **9. 重音命令**
 * - 命令: \hat, \tilde, \vec, \bar, \dot, \overline, \underline 等
 * - 要求: 必须有一个参数
 * - 合法: \hat{x}, \overline{a+b}, \vec{v}
 * - 非法: \hat, \vec
 * 
 * **10. cases 环境列数**
 * - 要求: 每行必须恰好 2 列
 * - 合法: \begin{cases}x & y\\a & b\end{cases}
 * - 非法: \begin{cases}x & y & z\end{cases}
 * 
 * **11. 堆叠命令**
 * - 命令: \stackrel, \overset, \underset, \sideset, \prescript 等
 * - 要求: 必须有相应数量的参数
 * 
 * **12. 盒子命令**
 * - 命令: \boxed, \fbox, \colorbox, \scalebox, \rotatebox 等
 * - 要求: 必须有相应数量的参数
 * 
 * **13. 字体/文本命令**
 * - 命令: \mathbf, \text, \mathrm, \mathbb 等
 * - 要求: 必须有一个参数
 * 
 * @section internal 内部状态变量
 * - braceBalance: 花括号平衡计数器，用于检测未闭合的花括号
 * - arrayDepth: 数组环境嵌套深度，用于控制 & 分隔符合法性
 * - lastSubSup: 上一个上下标类型，用于检测双重上下标错误
 * 
 * @param latex 待校验的LaTeX公式（宽字符串）
 * @throws ex_parse 当检测到语法错误时抛出异常，包含错误位置信息
 * 
 * @note 本函数仅进行语法预检测，不保证公式在渲染时一定成功
 * @see validateFrac, validateSqrt, validateBegin, validateLeft
 */
static void validateLatexFormula(const wstring& latex) {
    if (latex.empty()) 
        throw ex_parse("Empty LaTeX formula");
    
    const size_t n = latex.length();
    size_t p = 0;
    int braceBalance = 0;
    int arrayDepth = 0;
    wchar_t lastSubSup = 0;
    
    while (p < n) {
        wchar_t c = latex[p];
        
        if (c == L'\\') {
            lastSubSup = 0;
            
            if (++p >= n) 
                throw ex_parse(buildErrorMsg("Incomplete command at end of formula", p - 1, latex));
            
            wstring cmd;
            while (p < n && isalpha(latex[p])) cmd += latex[p++];
            
            if (cmd.empty()) {
                if (p < n && (latex[p] == L'\\' || latex[p] == L'{' || latex[p] == L'}' || 
                              latex[p] == L'_' || latex[p] == L'^' || latex[p] == L'&')) {
                    p++;
                }
                continue;
            }
            
            if (cmd == L"frac") {
                p = validateFrac(latex, p, n);
            } 
            else if (cmd == L"sqrt") {
                p = validateSqrt(latex, p, n);
            } 
            else if (isAccentCommand(cmd)) {
                p = validateAccentCommand(latex, p, n, cmd);
            }
            else if (STACK_COMMANDS.find(cmd) != STACK_COMMANDS.end()) {
                p = validateStackCommand(latex, p, n, cmd);
            }
            else if (BOX_COMMANDS.find(cmd) != BOX_COMMANDS.end()) {
                p = validateBoxCommand(latex, p, n, cmd);
            }
            else if (TRANSFORM_COMMANDS.find(cmd) != TRANSFORM_COMMANDS.end()) {
                p = validateBoxCommand(latex, p, n, cmd);
            }
            else if (TEXT_COMMANDS.find(cmd) != TEXT_COMMANDS.end()) {
                p = validateSingleArgCommand(latex, p, n, cmd);
            }
            else if (FONT_COMMANDS.find(cmd) != FONT_COMMANDS.end()) {
                p = validateSingleArgCommand(latex, p, n, cmd);
            }
            else if (MATH_TYPE_COMMANDS.find(cmd) != MATH_TYPE_COMMANDS.end()) {
                p = validateSingleArgCommand(latex, p, n, cmd);
            }
            else if (OTHER_ARG_COMMANDS.find(cmd) != OTHER_ARG_COMMANDS.end()) {
                p = validateSingleArgCommand(latex, p, n, cmd);
            }
            else if (cmd == L"rule") {
                p = validateSingleArgCommand(latex, p, n, cmd);
                p = validateSingleArgCommand(latex, p, n, cmd);
            }
            else if (cmd == L"begin") {
                p = validateBegin(latex, p, n, arrayDepth);
            } 
            else if (cmd == L"end") {
                p = validateEnd(latex, p, n, arrayDepth);
            } 
            else if (cmd == L"left") {
                p = validateLeft(latex, p, n);
            } 
            else if (cmd == L"right") {
                p = validateRight(latex, p, n);
            }
        }
        else if (c == L'_' || c == L'^') {
            p = validateSubscriptSuperscript(latex, p, n, lastSubSup);
        }
        else if (c == L'{') {
            braceBalance++;
            p++;
            lastSubSup = 0;
        }
        else if (c == L'}') {
            if (--braceBalance < 0) 
                throw ex_parse(buildErrorMsg("Unmatched closing brace '}'", p, latex));
            p++;
            lastSubSup = 0;
        }
        else if (c == L'&') {
            if (arrayDepth == 0) 
                throw ex_parse(buildErrorMsg("'&' only available in array mode", p, latex));
            p++;
            lastSubSup = 0;
        }
        else {
            p++;
            lastSubSup = 0;
        }
    }
    
    if (braceBalance > 0) 
        throw ex_parse(buildErrorMsg("Unmatched opening brace '{'", n, latex));
}

LaTeXParseResult LaTeX::parseWithError(const wstring& latex, int width, float textSize, float lineSpace, color fg) {
    LaTeXParseResult result;
    result.formula = latex;
    
    try {
        validateLatexFormula(latex);
        
        TeXRender* render = parse(latex, width, textSize, lineSpace, fg, false);
        result.success = true;
        result.render = render;
        result.errorMessage = "";
    } catch (const ex_parse& e) {
        result.success = false;
        result.render = nullptr;
        result.errorMessage = string(e.what());
    } catch (const exception& e) {
        result.success = false;
        result.render = nullptr;
        result.errorMessage = string(e.what());
    } catch (...) {
        result.success = false;
        result.render = nullptr;
        result.errorMessage = "Unknown error occurred while parsing formula";
    }
    
    return result;
}


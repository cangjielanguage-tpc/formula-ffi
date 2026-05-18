#include "scheme_parser.h"
#include "chemfig_constants.h"
#include "chemfig_parser.h"
#include "scheme_config.h"
#include <cwctype>
#include <algorithm>

namespace tex {

namespace {

using namespace chemfig;

wchar_t peekNext(const wchar_t* p) {
    return (!p || *p == L'\0' || p[1] == L'\0') ? L'\0' : p[1];
}

std::wstring stripOuterBraces(const std::wstring& s) {
    if (s.size() < 2 || s.front() != L'{' || s.back() != L'}') {
        return s;
    }
    int depth = 0;
    bool matched = true;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == L'{') depth++;
        else if (s[i] == L'}') depth--;
        if (depth == 0 && i < s.size() - 1) { matched = false; break; }
    }
    if (matched) return s.substr(1, s.size() - 2);
    return s;
}

bool isNumericString(const std::wstring& s) {
    if (s.empty()) return false;
    std::wstring cleaned = stripOuterBraces(s);
    if (cleaned.empty()) return false;
    size_t start = 0;
    if (cleaned[0] == L'-' || cleaned[0] == L'+') start = 1;
    if (start >= cleaned.size()) return false;
    bool hasDot = false;
    for (size_t i = start; i < cleaned.size(); i++) {
        if (cleaned[i] == L'.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!iswdigit(cleaned[i])) {
            return false;
        }
    }
    return true;
}

bool isLengthString(const std::wstring& s) {
    if (s.empty()) return false;
    std::wstring cleaned = stripOuterBraces(s);
    if (cleaned.empty()) return false;
    size_t start = 0;
    if (cleaned[0] == L'-' || cleaned[0] == L'+') start = 1;
    if (start >= cleaned.size()) return false;
    bool hasDot = false;
    size_t i = start;
    for (; i < cleaned.size(); i++) {
        if (cleaned[i] == L'.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!iswdigit(cleaned[i])) {
            break;
        }
    }
    if (i == start) return false;
    if (i < cleaned.size()) {
        for (; i < cleaned.size(); i++) {
            if (!iswalpha(cleaned[i])) return false;
        }
        return true;
    }
    return true;
}

float safeStofWithBraces(const std::wstring& s) {
    std::wstring cleaned = stripOuterBraces(s);
    return safeStof(cleaned);
}

std::wstring trim(const std::wstring& s) {
    size_t first = s.find_first_not_of(L" \t\n\r");
    if (first == std::wstring::npos) return L"";
    size_t last = s.find_last_not_of(L" \t\n\r");
    return s.substr(first, last - first + 1);
}

std::vector<std::wstring> splitByComma(const std::wstring& s) {
    std::vector<std::wstring> result;
    int depth = 0;
    std::wstring current;
    for (wchar_t c : s) {
        if (c == L'{') depth++;
        else if (c == L'}') depth--;
        if (c == L',' && depth == 0) {
            result.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    if (!current.empty() || !result.empty()) {
        result.push_back(trim(current));
    }
    return result;
}

}

std::wstring SchemeParser::preprocessSchemeSyntax(const std::wstring& input) {
    std::wstring result;
    size_t i = 0;
    size_t n = input.size();

    while (i < n) {
        if (input[i] == L'\\' && i + 1 < n) {
            size_t cmdStart = i;
            i++;
            std::wstring cmd;
            while (i < n && iswalpha(input[i])) {
                cmd += input[i];
                i++;
            }

            if (cmd == L"schemestart") {
                result += L"\\scheme{";
                size_t depth = 1;
                while (i < n && depth > 0) {
                    if (input[i] == L'\\' && i + 1 < n) {
                        size_t peekStart = i;
                        i++;
                        std::wstring peekCmd;
                        while (i < n && iswalpha(input[i])) {
                            peekCmd += input[i];
                            i++;
                        }
                        if (peekCmd == L"schemestop") {
                            depth--;
                            if (depth == 0) break;
                            result += L"\\schemestop";
                        } else if (peekCmd == L"schemestart") {
                            depth++;
                            result += L"\\schemestart";
                        } else if (peekCmd == L"chemnameinit") {
                            while (i < n && iswspace(input[i])) i++;
                            if (i < n && input[i] == L'{') {
                                i++;
                                int bd = 1;
                                while (i < n && bd > 0) {
                                    if (input[i] == L'\\' && i + 1 < n) {
                                        i += 2;
                                    } else if (input[i] == L'{') {
                                        bd++;
                                        i++;
                                    } else if (input[i] == L'}') {
                                        bd--;
                                        if (bd == 0) i++;
                                        else i++;
                                    } else {
                                        i++;
                                    }
                                }
                            }
                        } else {
                            result += input.substr(peekStart, i - peekStart);
                        }
                    } else {
                        if (input[i] == L'\\' && i + 1 < n) {
                            result += input[i];
                            result += input[i + 1];
                            i += 2;
                        } else if (input[i] == L'{') {
                            depth++;
                            result += input[i];
                            i++;
                        } else if (input[i] == L'}') {
                            depth--;
                            if (depth == 0) { i++; break; }
                            result += input[i];
                            i++;
                        } else {
                            result += input[i];
                            i++;
                        }
                    }
                }
                result += L"}";
                continue;
            }

            if (cmd == L"setchemfig") {
                while (i < n && iswspace(input[i])) i++;
                if (i < n && input[i] == L'{') {
                    size_t braceStart = i;
                    i++;
                    int bd = 1;
                    std::wstring kvContent;
                    while (i < n && bd > 0) {
                        if (input[i] == L'\\' && i + 1 < n) {
                            kvContent += input[i];
                            kvContent += input[i + 1];
                            i += 2;
                        } else if (input[i] == L'{') {
                            bd++;
                            kvContent += input[i];
                            i++;
                        } else if (input[i] == L'}') {
                            bd--;
                            if (bd == 0) { i++; break; }
                            kvContent += input[i];
                            i++;
                        } else {
                            kvContent += input[i];
                            i++;
                        }
                    }
                    size_t eqPos = kvContent.find(L'=');
                    if (eqPos != std::wstring::npos) {
                        std::wstring key = kvContent.substr(0, eqPos);
                        std::wstring value = kvContent.substr(eqPos + 1);
                        SchemeConfig::instance().set(key, value);
                    }
                }
                continue;
            }

            if (cmd == L"resetchemfig") {
                SchemeConfig::instance().reset();
                continue;
            }

            if (cmd == L"chemnameinit") {
                while (i < n && iswspace(input[i])) i++;
                if (i < n && input[i] == L'{') {
                    i++;
                    int bd = 1;
                    while (i < n && bd > 0) {
                        if (input[i] == L'\\' && i + 1 < n) {
                            i += 2;
                        } else if (input[i] == L'{') {
                            bd++;
                            i++;
                        } else if (input[i] == L'}') {
                            bd--;
                            if (bd == 0) i++;
                            else i++;
                        } else {
                            i++;
                        }
                    }
                }
                continue;
            }

            result += input.substr(cmdStart, i - cmdStart);
            continue;
        }

        result += input[i];
        i++;
    }

    return result;
}

wchar_t SchemeParser::peek(const wchar_t* p) {
    return (!p || *p == L'\0') ? L'\0' : *p;
}

void SchemeParser::skipWhitespace(const wchar_t*& p) {
    while (p && *p != L'\0' && iswspace(*p)) p++;
}

bool SchemeParser::match(const wchar_t*& p, wchar_t expected) {
    if (peek(p) == expected) {
        p++;
        return true;
    }
    return false;
}

std::wstring SchemeParser::parseBraceContent(const wchar_t*& p) {
    std::wstring result;
    if (!p || peek(p) != L'{') return result;
    p++;
    int depth = 1;
    while (p && *p != L'\0' && depth > 0) {
        if (*p == L'\\' && *(p + 1) != L'\0') {
            result += *p++;
            result += *p++;
        } else if (*p == L'{') {
            depth++;
            result += *p++;
        } else if (*p == L'}') {
            depth--;
            if (depth == 0) { p++; return result; }
            result += *p++;
        } else {
            result += *p++;
        }
    }
    return result;
}

std::wstring SchemeParser::parseBracketContent(const wchar_t*& p) {
    std::wstring result;
    if (!p || peek(p) != L'[') return result;
    p++;
    int depth = 1;
    while (p && *p != L'\0' && depth > 0) {
        if (*p == L'\\' && *(p + 1) != L'\0') {
            result += *p++;
            result += *p++;
        } else if (*p == L'[') {
            depth++;
            result += *p++;
        } else if (*p == L']') {
            depth--;
            if (depth == 0) { p++; return result; }
            result += *p++;
        } else {
            result += *p++;
        }
    }
    return result;
}

std::wstring SchemeParser::parseAtRef(const wchar_t*& p) {
    std::wstring result;
    if (!p || peek(p) != L'@') return result;
    p++;
    if (peek(p) == L'{') {
        p++;
        while (p && *p != L'\0' && *p != L'}') {
            result += *p++;
        }
        if (peek(p) == L'}') p++;
    } else {
        while (p && *p != L'\0' && (iswalnum(*p) || *p == L'_' || *p == L'.')) {
            result += *p++;
        }
    }
    return result;
}

ArrowType SchemeParser::parseArrowCode(const wchar_t*& p, std::wstring& labelAbove, std::wstring& labelBelow) {
    skipWhitespace(p);

    if (peek(p) == L'0') { p++; return ARROW_INVISIBLE; }

    std::wstring code;

    while (*p != L'\0' && *p != L'[' && *p != L'}' && *p != L')') {
        wchar_t c = *p;
        if (c == L'-' || c == L'<' || c == L'>' || c == L'/' ||
            c == L'\\' || c == L'~' || c == L'=' || c == L'.' || c == L'U') {
            code += c;
            p++;
        } else {
            break;
        }
    }

    while (peek(p) == L'[') {
        std::wstring content = parseBracketContent(p);
        if (labelAbove.empty()) {
            labelAbove = content;
        } else if (labelBelow.empty()) {
            labelBelow = content;
        }
    }

    if (code.empty()) return ARROW_FORWARD;

    if (code == L"->") return ARROW_FORWARD;
    if (code == L"<-") return ARROW_BACKWARD;
    if (code == L"<->") return ARROW_BIDIRECTIONAL;
    if (code == L"<=>") return ARROW_EQUILIBRIUM;
    if (code == L"<<->") return ARROW_LONG_EQUILIB;
    if (code == L"<->>") return ARROW_ALT_EQUILIB;
    if (code == L"-/>") return ARROW_HARP_RIGHT;
    if (code == L"</-") return ARROW_HARP_LEFT;
    if (code == L"-..>") return ARROW_HARPOON_RIGHT;
    if (code == L"<..-") return ARROW_HARPOON_LEFT;
    if (code == L"-->") return ARROW_DASHED_FORWARD;
    if (code == L"<=>>") return ARROW_DASHED_EQUILIBRIUM;
    if (code == L"->>") return ARROW_CURVED_FORWARD;
    if (code == L"<-<") return ARROW_CURVED_BACKWARD;
    if (code == L"~>") return ARROW_CURVED_FORWARD;
    if (code == L"<~") return ARROW_CURVED_BACKWARD;
    if (code == L"-U>") return ARROW_ARC_FORWARD;
    if (code == L"<U-") return ARROW_ARC_BACKWARD;
    if (code == L"<U>") return ARROW_ARC_BIDIR;

    if (code.find(L"\\<>") != std::wstring::npos) return ARROW_FISHHOOK;

    int leftCount = 0, rightCount = 0, eqCount = 0;
    bool hasDot = false, hasForwardSlash = false, hasBackslash = false;
    for (wchar_t c : code) {
        switch (c) {
            case L'<': leftCount++; break;
            case L'>': rightCount++; break;
            case L'=': eqCount++; break;
            case L'/': hasForwardSlash = true; break;
            case L'\\': hasBackslash = true; break;
            case L'.': hasDot = true; break;
        }
    }

    if (hasDot && hasBackslash) return ARROW_FISHHOOK;
    if (hasDot && rightCount > 0 && leftCount == 0) return ARROW_HARPOON_RIGHT;
    if (hasDot && leftCount > 0 && rightCount == 0) return ARROW_HARPOON_LEFT;
    if (hasBackslash) return ARROW_FISHHOOK;
    if (hasForwardSlash) return (leftCount > 0 && rightCount == 0) ? ARROW_HARP_LEFT : ARROW_HARP_RIGHT;

    if (eqCount > 0) {
        if (leftCount > 0 && rightCount > 0) return ARROW_EQUILIBRIUM;
        if (leftCount >= 2 && rightCount == 1) return ARROW_LONG_EQUILIB;
        if (leftCount == 1 && rightCount >= 2) return ARROW_ALT_EQUILIB;
        if (rightCount > 0) return ARROW_DASHED_EQUILIBRIUM;
        return ARROW_DASHED_FORWARD;
    }

    if (leftCount > 0 && rightCount > 0) {
        return (leftCount >= 2 && rightCount >= 2) ? ARROW_CURVED_BIDIR : ARROW_BIDIRECTIONAL;
    }
    if (rightCount > 0 && leftCount == 0) {
        return (rightCount >= 2) ? ARROW_CURVED_FORWARD : ARROW_FORWARD;
    }
    if (leftCount > 0 && rightCount == 0) {
        return (leftCount >= 2) ? ARROW_CURVED_BACKWARD : ARROW_BACKWARD;
    }

    return ARROW_FORWARD;
}

ArrowParams SchemeParser::parseArrowArgs(const wchar_t*& p) {
    ArrowParams params;
    int argIndex = 0;
    bool labelAboveSet = false;
    bool labelBelowSet = false;

    while (peek(p) == L'[') {
        std::wstring content = parseBracketContent(p);

        auto parts = splitByComma(content);
        
        bool firstTwoNumericOrEmpty = (parts.size() >= 2 && 
                                       (parts[0].empty() || isNumericString(parts[0])) && 
                                       (parts[1].empty() || isNumericString(parts[1])));
        
        if (firstTwoNumericOrEmpty && parts.size() >= 2) {
            if (!parts[0].empty()) params.angle = safeStofWithBraces(parts[0]);
            if (parts.size() > 1 && !parts[1].empty()) {
                params.lengthCoeff = safeStofWithBraces(parts[1]);
                if (params.lengthCoeff <= 0.0f) params.lengthCoeff = 1.0f;
            }
            if (parts.size() > 2 && !parts[2].empty()) {
                params.tikzStyle = parts[2];
            }
            argIndex = (argIndex <= 1) ? 4 : (argIndex + 2);
        } else if ((argIndex == 0 || argIndex == 1) && isNumericString(content) && !labelAboveSet) {
            params.angle = safeStofWithBraces(content);
            argIndex = 3;
        } else {
            switch (argIndex) {
                case 0:
                    params.labelAbove = content;
                    labelAboveSet = true;
                    break;
                case 1:
                    params.labelBelow = content;
                    labelBelowSet = true;
                    break;
                case 2:
                    if (!content.empty()) {
                        if (isLengthString(content) && !isNumericString(content)) {
                            params.yShiftRaw = content;
                        } else {
                            params.angle = safeStofWithBraces(content);
                        }
                    }
                    break;
                case 3:
                    if (!content.empty()) {
                        params.lengthCoeff = safeStofWithBraces(content);
                        if (params.lengthCoeff <= 0.0f) params.lengthCoeff = 1.0f;
                    }
                    break;
                case 4:
                    if (!content.empty()) {
                        params.curveHeight = safeStofWithBraces(content);
                    }
                    break;
                case 5: params.tikzStyle = content; break;
                default: break;
            }
        }
        argIndex++;
    }

    if (!params.tikzStyle.empty()) {
        std::wstring style = params.tikzStyle;
        if (style.find(L"dashed") != std::wstring::npos) {
            params.dashed = true;
        }
        
        static const wchar_t* colorNames[] = {
            L"red", L"blue", L"green", L"yellow", L"black", L"white",
            L"cyan", L"magenta", L"orange", L"purple", L"brown", L"gray",
            L"pink", L"violet", L"olive", L"teal", L"lime", L"darkgray",
            L"lightgray", L"darkblue", L"darkgreen", L"darkred"
        };
        
        for (const auto& colorName : colorNames) {
            if (style.find(colorName) != std::wstring::npos) {
                params.color = colorName;
                break;
            }
        }
    }

    return params;
}

bool SchemeParser::parseArrowAnchor(const wchar_t*& p, ArrowAnchor& anchor) {
    skipWhitespace(p);
    if (peek(p) == L'@') {
        std::wstring name = parseAtRef(p);
        if (name.empty()) return false;
        size_t dotPos = name.find(L'.');
        if (dotPos != std::wstring::npos) {
            anchor.compoundRef = name.substr(0, dotPos);
            anchor.anchorName = name.substr(dotPos + 1);
        } else {
            anchor.compoundRef = name;
            anchor.anchorName = name;
        }
    }
    return true;
}

void SchemeParser::parseArrowEndpointRef(const wchar_t*& p, ArrowRef& ref, ArrowAnchor& anchor) {
    skipWhitespace(p);

    if (peek(p) == L'@') {
        parseArrowAnchor(p, anchor);
        ref.compoundRef = anchor.compoundRef;
        ref.anchorName = anchor.anchorName;
        return;
    }

    if (peek(p) == L'.') {
        p++;
        std::wstring anc;
        while (*p != L'\0') {
            if (*p == L'-' && peekNext(p) == L'-') break;
            if (*p == L')') break;
            anc += *p++;
        }
        while (!anc.empty() && iswspace(anc.back())) anc.pop_back();
        if (!anc.empty()) {
            ref.anchorName = anc;
            anchor.anchorName = anc;
        }
        return;
    }

    std::wstring token;
    while (*p != L'\0' && *p != L'-' && *p != L')' && *p != L'.' && !iswspace(*p)) {
        token += *p++;
    }
    skipWhitespace(p);

    if (peek(p) == L'.') {
        p++;
        ref.compoundRef = token;
        anchor.compoundRef = token;
        std::wstring anc;
        while (*p != L'\0') {
            if (*p == L'-' && peekNext(p) == L'-') break;
            if (*p == L')') break;
            anc += *p++;
        }
        while (!anc.empty() && iswspace(anc.back())) anc.pop_back();
        if (!anc.empty()) {
            ref.anchorName = anc;
            anchor.anchorName = anc;
        }
    } else if (!token.empty()) {
        ref.anchorName = token;
        anchor.anchorName = token;
    }
}

bool SchemeParser::parseCompound(const wchar_t*& p, ReactionScheme& scheme) {
    skipWhitespace(p);

    std::wstring refName;
    if (peek(p) == L'@') {
        refName = parseAtRef(p);
    }

    std::wstring molCode = parseBraceContent(p);
    if (molCode.empty()) return false;

    CompoundInfo info;
    if (!ChemfigParser::parse(molCode, info.molecule)) {
        return false;
    }
    info.molecule.calculateBounds();
    info.refName = refName;

    for (const auto& anchor : info.molecule.anchors) {
        info.atomAnchors[anchor.name] = anchor.atomIndex;
    }

    int idx = static_cast<int>(scheme.compounds.size());
    scheme.compounds.push_back(info);
    scheme.elementOrder.push_back(std::make_pair(ELEM_COMPOUND, idx));

    if (!refName.empty()) {
        scheme.compoundRefs[refName] = idx;
    }

    return true;
}

bool SchemeParser::parseArrow(const wchar_t*& p, ReactionScheme& scheme) {
    skipWhitespace(p);

    ArrowRef fromRef, toRef;
    ArrowAnchor fromAnchor, toAnchor;

    if (peek(p) == L'(') {
        p++;
        parseArrowEndpointRef(p, fromRef, fromAnchor);

        skipWhitespace(p);
        if (peek(p) == L'-' && peekNext(p) == L'-') { p += 2; }
        else if (peek(p) == L'-') { p++; }
        skipWhitespace(p);

        parseArrowEndpointRef(p, toRef, toAnchor);

        skipWhitespace(p);
        if (peek(p) == L')') p++;
    }

    skipWhitespace(p);

    ArrowType type = ARROW_FORWARD;
    std::wstring labelAbove, labelBelow;
    if (peek(p) == L'{') {
        p++;
        type = parseArrowCode(p, labelAbove, labelBelow);
        skipWhitespace(p);
        if (peek(p) == L'}') p++;
    }

    ArrowParams params = parseArrowArgs(p);

    params.type = type;
    params.fromRef = fromRef;
    params.toRef = toRef;
    params.fromAnchor = fromAnchor;
    params.toAnchor = toAnchor;
    
    if (params.labelAbove.empty() && !labelAbove.empty()) {
        params.labelAbove = labelAbove;
    }
    if (params.labelBelow.empty() && !labelBelow.empty()) {
        params.labelBelow = labelBelow;
    }

    if (params.isCurved() && params.curveHeight == 0.0f) {
        params.curveHeight = SchemeConfig::instance().defaultCurveHeight;
    }

    int idx = static_cast<int>(scheme.arrows.size());
    ArrowElement elem;
    elem.params = params;
    scheme.arrows.push_back(elem);
    scheme.elementOrder.push_back(std::make_pair(ELEM_ARROW, idx));

    return true;
}

bool SchemeParser::parsePlus(const wchar_t*& p, ReactionScheme& scheme) {
    PlusElement elem;
    elem.afterCompound = static_cast<int>(scheme.compounds.size()) - 1;

    if (peek(p) == L'{') {
        std::wstring content = parseBraceContent(p);
        auto parts = splitByComma(content);
        
        if (parts.size() >= 1 && !parts[0].empty()) {
            elem.sepLeftRaw = parts[0];
            elem.hasCustomSep = true;
        }
        if (parts.size() >= 2 && !parts[1].empty()) {
            elem.sepRightRaw = parts[1];
            elem.hasCustomSep = true;
        }
        if (parts.size() >= 3 && !parts[2].empty()) {
            elem.vshiftRaw = parts[2];
            elem.hasCustomSep = true;
        }
    }

    int idx = static_cast<int>(scheme.pluses.size());
    scheme.pluses.push_back(elem);
    scheme.elementOrder.push_back(std::make_pair(ELEM_PLUS, idx));
    return true;
}

void SchemeParser::parseMergeCompoundRef(const std::wstring& content,
                                          std::wstring& compoundRef,
                                          std::wstring& anchorName,
                                          int& compoundIndex,
                                          ReactionScheme& scheme) {
    if (content.empty()) return;

    if (content[0] == L'@') {
        std::wstring ref = content.substr(1);
        size_t dotPos = ref.find(L'.');
        if (dotPos != std::wstring::npos) {
            compoundRef = ref.substr(0, dotPos);
            anchorName = ref.substr(dotPos + 1);
        } else {
            compoundRef = ref;
        }
    } else if (content.size() > 8 && content.substr(0, 8) == L"\\chemfig") {
        size_t bracePos = content.find(L'{');
        if (bracePos != std::wstring::npos) {
            std::wstring molCode = content.substr(bracePos + 1);
            if (!molCode.empty() && molCode.back() == L'}') {
                molCode.pop_back();
            }
            CompoundInfo info;
            if (ChemfigParser::parse(molCode, info.molecule)) {
                info.molecule.calculateBounds();
                int idx = static_cast<int>(scheme.compounds.size());
                scheme.compounds.push_back(info);
                scheme.elementOrder.push_back(std::make_pair(ELEM_COMPOUND, idx));
                compoundIndex = idx;
            }
        }
    } else {
        CompoundInfo info;
        if (ChemfigParser::parse(content, info.molecule)) {
            info.molecule.calculateBounds();
            int idx = static_cast<int>(scheme.compounds.size());
            scheme.compounds.push_back(info);
            scheme.elementOrder.push_back(std::make_pair(ELEM_COMPOUND, idx));
            compoundIndex = idx;
        }
    }
}

bool SchemeParser::parseMerge(const wchar_t*& p, ReactionScheme& scheme) {
    MergeElement elem;
    elem.afterCompound = static_cast<int>(scheme.compounds.size()) - 1;

    auto parseMergeDirectionAndGeometry = [](const std::vector<std::wstring>& parts, MergeDirection& dir, MergeGeometry& geo) {
        for (const auto& part : parts) {
            if (part == L"->" || part == L"right" || part == L"r") dir = MERGE_RIGHT;
            else if (part == L"<-" || part == L"left" || part == L"l") dir = MERGE_LEFT;
            else if (part == L"up" || part == L"u") dir = MERGE_UP;
            else if (part == L"down" || part == L"d") dir = MERGE_DOWN;
            else if (isNumericString(part)) geo.segmentCoeff = safeStofWithBraces(part);
            else geo.style = part;
        }
    };

    skipWhitespace(p);

    if (peek(p) == L'[') {
        std::wstring content = parseBracketContent(p);
        auto parts = splitByComma(content);
        parseMergeDirectionAndGeometry(parts, elem.direction, elem.geometry);
    }

    skipWhitespace(p);

    while (peek(p) == L'{') {
        std::wstring sourceContent = parseBraceContent(p);
        MergeSource src;
        if (!sourceContent.empty()) {
            parseMergeCompoundRef(sourceContent, src.compoundRef, src.anchorName, src.compoundIndex, scheme);
        }
        elem.sources.push_back(src);

        skipWhitespace(p);
        if (peek(p) == L'\\') {
            if (peekNext(p) == L'+') {
                p += 2;
                skipWhitespace(p);
                continue;
            }
        }
        break;
    }

    skipWhitespace(p);

    if (peek(p) == L'{') {
        std::wstring targetContent = parseBraceContent(p);
        if (!targetContent.empty()) {
            parseMergeCompoundRef(targetContent, elem.target.compoundRef, elem.target.anchorName, elem.target.compoundIndex, scheme);
        }
    }

    if (peek(p) == L'[') {
        std::wstring content = parseBracketContent(p);
        auto parts = splitByComma(content);
        parseMergeDirectionAndGeometry(parts, elem.direction, elem.geometry);
    }

    int idx = static_cast<int>(scheme.merges.size());
    scheme.merges.push_back(elem);
    scheme.elementOrder.push_back(std::make_pair(ELEM_MERGE, idx));

    return true;
}

bool SchemeParser::parseChemname(const wchar_t*& p, ReactionScheme& scheme) {
    skipWhitespace(p);
    std::wstring molContent = parseBraceContent(p);
    skipWhitespace(p);
    std::wstring name = parseBraceContent(p);

    if (molContent.empty()) return false;

    if (molContent.size() > 8 && molContent.substr(0, 8) == L"\\chemfig") {
        size_t braceStart = molContent.find(L'{');
        if (braceStart != std::wstring::npos) {
            int depth = 1;
            size_t j = braceStart + 1;
            while (j < molContent.size() && depth > 0) {
                if (molContent[j] == L'{') depth++;
                else if (molContent[j] == L'}') { depth--; if (depth == 0) break; }
                j++;
            }
            molContent = molContent.substr(braceStart + 1, j - braceStart - 1);
        }
    }

    CompoundInfo info;
    if (!ChemfigParser::parse(molContent, info.molecule)) {
        return false;
    }
    info.molecule.calculateBounds();
    info.name = name;

    for (const auto& anchor : info.molecule.anchors) {
        info.atomAnchors[anchor.name] = anchor.atomIndex;
    }

    int idx = static_cast<int>(scheme.compounds.size());
    scheme.compounds.push_back(info);
    scheme.elementOrder.push_back(std::make_pair(ELEM_COMPOUND, idx));

    return true;
}

void SchemeParser::resolveReferences(ReactionScheme& scheme) {
    for (auto& elem : scheme.elementOrder) {
        if (elem.first == ELEM_COMPOUND) {
            if (elem.second < 0 || elem.second >= static_cast<int>(scheme.compounds.size())) continue;
            CompoundInfo& info = scheme.compounds[elem.second];
            if (!info.refName.empty() && scheme.compoundRefs.find(info.refName) == scheme.compoundRefs.end()) {
                scheme.compoundRefs[info.refName] = elem.second;
            }
            for (const auto& anchor : info.atomAnchors) {
                if (scheme.compoundRefs.find(anchor.first) == scheme.compoundRefs.end()) {
                    scheme.compoundRefs[anchor.first] = elem.second;
                }
            }
        }
    }

    int lastCompoundIdx = -1;

    for (size_t i = 0; i < scheme.elementOrder.size(); i++) {
        if (scheme.elementOrder[i].first == ELEM_ARROW) {
            int arrowIdx = scheme.elementOrder[i].second;
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(scheme.arrows.size())) continue;
            ArrowElement& arrow = scheme.arrows[arrowIdx];

            int fromIdx = -1;
            int toIdx = -1;
            int fromSubIdx = -1;
            int toSubIdx = -1;

            for (int j = static_cast<int>(i) - 1; j >= 0; j--) {
                if (scheme.elementOrder[j].first == ELEM_COMPOUND) {
                    fromIdx = scheme.elementOrder[j].second;
                    break;
                } else if (scheme.elementOrder[j].first == ELEM_SUBSCHEME) {
                    int subIdx = scheme.elementOrder[j].second;
                    if (subIdx >= 0 && subIdx < static_cast<int>(scheme.subschemes.size())) {
                        const auto& subInfo = scheme.subschemes[subIdx];
                        fromIdx = subInfo.firstCompoundIndex;
                        fromSubIdx = subIdx;
                        break;
                    }
                }
            }
            for (size_t j = i + 1; j < scheme.elementOrder.size(); j++) {
                if (scheme.elementOrder[j].first == ELEM_COMPOUND) {
                    toIdx = scheme.elementOrder[j].second;
                    break;
                } else if (scheme.elementOrder[j].first == ELEM_SUBSCHEME) {
                    int subIdx = scheme.elementOrder[j].second;
                    if (subIdx >= 0 && subIdx < static_cast<int>(scheme.subschemes.size())) {
                        const auto& subInfo = scheme.subschemes[subIdx];
                        toIdx = subInfo.firstCompoundIndex;
                        toSubIdx = subIdx;
                        break;
                    }
                }
            }

            auto resolveCompoundIdx = [&](const ArrowRef& ref, const ArrowAnchor& anchor, int defaultIdx) -> int {
                int idx = -1;
                const std::wstring& refName = !ref.compoundRef.empty() ? ref.compoundRef : anchor.compoundRef;
                if (!refName.empty()) {
                    idx = scheme.findCompound(refName);
                    if (idx < 0 && refName.size() > 1 &&
                        (refName[0] == L'c' || refName[0] == L'C')) {
                        std::wstring numStr = refName.substr(1);
                        if (!numStr.empty()) {
                            bool allDigits = true;
                            for (wchar_t c : numStr) {
                                if (!iswdigit(c)) { allDigits = false; break; }
                            }
                            if (allDigits) {
                                int cIdx = std::stoi(numStr) - 1;
                                if (cIdx >= 0 && cIdx < scheme.compoundCount()) {
                                    idx = cIdx;
                                    scheme.compoundRefs[refName] = idx;
                                }
                            }
                        }
                    }
                }
                return (idx >= 0) ? idx : defaultIdx;
            };

            arrow.fromCompound = resolveCompoundIdx(arrow.params.fromRef, arrow.params.fromAnchor, fromIdx);
            arrow.toCompound = resolveCompoundIdx(arrow.params.toRef, arrow.params.toAnchor, toIdx);

            if (arrow.params.fromRef.compoundRef.empty() && arrow.params.fromAnchor.compoundRef.empty()) {
                arrow.fromSubschemeIdx = fromSubIdx;
            }
            if (arrow.params.toRef.compoundRef.empty() && arrow.params.toAnchor.compoundRef.empty()) {
                arrow.toSubschemeIdx = toSubIdx;
            }

            if (!arrow.params.fromAnchor.anchorName.empty() && arrow.fromCompound >= 0) {
                scheme.compoundRefs[arrow.params.fromAnchor.anchorName] = arrow.fromCompound;
            }
            if (!arrow.params.toAnchor.anchorName.empty() && arrow.toCompound >= 0) {
                scheme.compoundRefs[arrow.params.toAnchor.anchorName] = arrow.toCompound;
            }
        }

        if (scheme.elementOrder[i].first == ELEM_COMPOUND) {
            lastCompoundIdx = scheme.elementOrder[i].second;
        }
    }

    for (auto& subInfo : scheme.subschemes) {
        subInfo.internalArrows.clear();
        if (subInfo.startArrow >= 0 && subInfo.endArrow >= subInfo.startArrow) {
            for (int ai = subInfo.startArrow; ai <= subInfo.endArrow; ai++) {
                subInfo.internalArrows.push_back(ai);
            }
        }
    }

    for (auto& subInfo : scheme.subschemes) {
        int lastCompoundInSub = subInfo.firstCompoundIndex;
        
        for (int arrowIdx : subInfo.internalArrows) {
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(scheme.arrows.size())) continue;
            ArrowElement& arrow = scheme.arrows[arrowIdx];
            
            int fromIdx = -1;
            int toIdx = -1;
            
            auto resolveCompoundIdx = [&](const ArrowRef& ref, const ArrowAnchor& anchor, int defaultIdx) -> int {
                int idx = -1;
                const std::wstring& refName = !ref.compoundRef.empty() ? ref.compoundRef : anchor.compoundRef;
                if (!refName.empty()) {
                    idx = scheme.findCompound(refName);
                }
                return (idx >= 0) ? idx : defaultIdx;
            };
            
            if (arrow.params.fromRef.compoundRef.empty() && arrow.params.fromAnchor.compoundRef.empty()) {
                fromIdx = lastCompoundInSub;
            }
            
            if (arrow.params.toRef.compoundRef.empty() && arrow.params.toAnchor.compoundRef.empty()) {
                toIdx = lastCompoundInSub + 1;
                if (toIdx > subInfo.endCompound) toIdx = subInfo.endCompound;
            }
            
            arrow.fromCompound = resolveCompoundIdx(arrow.params.fromRef, arrow.params.fromAnchor, fromIdx);
            arrow.toCompound = resolveCompoundIdx(arrow.params.toRef, arrow.params.toAnchor, toIdx);
            
            if (arrow.toCompound >= subInfo.startCompound && arrow.toCompound <= subInfo.endCompound) {
                lastCompoundInSub = arrow.toCompound;
            }
        }
    }

    for (auto& merge : scheme.merges) {
        for (auto& src : merge.sources) {
            if (src.compoundIndex < 0 && !src.compoundRef.empty()) {
                src.compoundIndex = scheme.findCompound(src.compoundRef);
            }
            if (src.compoundIndex >= scheme.compoundCount()) src.compoundIndex = -1;
        }
        if (merge.target.compoundIndex < 0 && !merge.target.compoundRef.empty()) {
            merge.target.compoundIndex = scheme.findCompound(merge.target.compoundRef);
        }
        if (merge.target.compoundIndex >= scheme.compoundCount()) merge.target.compoundIndex = -1;
    }

    if (SchemeConfig::instance().autoNumber) {
        int num = 1;
        for (auto& info : scheme.compounds) {
            info.number = num++;
        }
    }
}

bool SchemeParser::parseContent(const wchar_t* p, ReactionScheme& scheme) {
    while (*p != L'\0') {
        skipWhitespace(p);
        if (*p == L'\0') break;

        if (*p == L'\\') {
            if (peekNext(p) == L'+') {
                p += 2;
                if (!parsePlus(p, scheme)) return false;
                continue;
            }
            if (peekNext(p) == L'\\') {
                p += 2;
                scheme.elementOrder.push_back(std::make_pair(ELEM_LINEBREAK, 0));
                continue;
            }

            p++;
            std::wstring cmd;
            while (*p != L'\0' && (iswalpha(*p) || *p == L'@')) {
                cmd += *p++;
            }

            if (cmd == L"chemfig") {
                if (!parseCompound(p, scheme)) return false;
            } else if (cmd == L"arrow") {
                if (!parseArrow(p, scheme)) return false;
            } else if (cmd == L"merge") {
                if (!parseMerge(p, scheme)) return false;
            } else if (cmd == L"chemname") {
                if (!parseChemname(p, scheme)) return false;
            } else if (cmd == L"chemnameinit") {
                skipWhitespace(p);
                std::wstring offset = parseBraceContent(p);
                if (!offset.empty()) {
                    SchemeConfig::instance().nameOffset = safeStofWithBraces(offset);
                }
            } else if (cmd == L"schemestart" || cmd == L"schemestop") {
                continue;
            } else if (cmd == L"subscheme") {
                std::wstring content = parseBraceContent(p);
                int startIdx = static_cast<int>(scheme.compounds.size());
                int arrowStartIdx = static_cast<int>(scheme.arrows.size());
                size_t elemStartIdx = scheme.elementOrder.size();
                
                SubschemeInfo subInfo;
                subInfo.startCompound = startIdx;
                subInfo.startArrow = arrowStartIdx;
                const wchar_t* subP = content.c_str();
                if (!parseContent(subP, scheme)) return false;
                int endIdx = static_cast<int>(scheme.compounds.size()) - 1;
                int arrowEndIdx = static_cast<int>(scheme.arrows.size()) - 1;
                subInfo.endCompound = endIdx;
                subInfo.endArrow = arrowEndIdx;
                
                if (startIdx <= endIdx) {
                    subInfo.firstCompoundIndex = startIdx;
                    
                    int subIdx = static_cast<int>(scheme.subschemes.size());
                    scheme.subschemes.push_back(subInfo);
                    
                    scheme.elementOrder.erase(
                        scheme.elementOrder.begin() + elemStartIdx,
                        scheme.elementOrder.end());
                    
                    scheme.elementOrder.push_back(std::make_pair(ELEM_SUBSCHEME, subIdx));
                }
            } else if (cmd == L"compnam") {
                skipWhitespace(p);
                std::wstring name = parseBraceContent(p);
                if (!scheme.compounds.empty()) {
                    scheme.compounds.back().name = name;
                }
            } else if (cmd == L"chemabove") {
                skipWhitespace(p);
                std::wstring label = parseBraceContent(p);
                if (!scheme.arrows.empty()) {
                    scheme.arrows.back().params.labelAbove = label;
                }
            } else if (cmd == L"chembelow") {
                skipWhitespace(p);
                std::wstring label = parseBraceContent(p);
                if (!scheme.arrows.empty()) {
                    scheme.arrows.back().params.labelBelow = label;
                }
            } else if (cmd == L"chemsign") {
                skipWhitespace(p);
                std::wstring sign = parseBraceContent(p);
                if (!scheme.compounds.empty()) {
                    scheme.compounds.back().name = sign;
                }
            } else if (cmd == L"compref") {
                skipWhitespace(p);
                std::wstring ref = parseBraceContent(p);
                if (!scheme.compounds.empty()) {
                    scheme.compounds.back().refName = ref;
                    scheme.compoundRefs[ref] = static_cast<int>(scheme.compounds.size()) - 1;
                }
            } else {
                while (*p != L'\0' && *p != L'\\' && !iswspace(*p)) p++;
            }
        } else {
            std::wstring bareText;
            while (*p != L'\0' && *p != L'\\' && !iswspace(*p)) {
                bareText += *p++;
            }
            if (!bareText.empty()) {
                CompoundInfo info;
                if (ChemfigParser::parse(bareText, info.molecule)) {
                    info.molecule.calculateBounds();
                    int idx = static_cast<int>(scheme.compounds.size());
                    scheme.compounds.push_back(info);
                    scheme.elementOrder.push_back(std::make_pair(ELEM_COMPOUND, idx));
                }
            }
        }
    }

    return true;
}

bool SchemeParser::parse(const std::wstring& input, ReactionScheme& scheme) {
    SchemeConfig::instance().reset();

    scheme.compounds.clear();
    scheme.arrows.clear();
    scheme.pluses.clear();
    scheme.merges.clear();
    scheme.subschemes.clear();
    scheme.elementOrder.clear();
    scheme.compoundRefs.clear();

    const wchar_t* p = input.c_str();
    if (!parseContent(p, scheme)) return false;

    resolveReferences(scheme);
    return !scheme.compounds.empty();
}

} // namespace tex

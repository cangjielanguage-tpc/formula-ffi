#ifndef SCHEME_PARSER_H_INCLUDED
#define SCHEME_PARSER_H_INCLUDED

#include "scheme_types.h"
#include <string>

namespace tex {

class SchemeParser {
public:
    static bool parse(const std::wstring& input, ReactionScheme& scheme);

    static std::wstring preprocessSchemeSyntax(const std::wstring& input);

private:
    static bool parseCompound(const wchar_t*& p, ReactionScheme& scheme);
    static bool parseArrow(const wchar_t*& p, ReactionScheme& scheme);
    static bool parseArrowAnchor(const wchar_t*& p, ArrowAnchor& anchor);
    static void parseArrowEndpointRef(const wchar_t*& p, ArrowRef& ref, ArrowAnchor& anchor);
    static bool parsePlus(const wchar_t*& p, ReactionScheme& scheme);
    static bool parseMerge(const wchar_t*& p, ReactionScheme& scheme);
    static void parseMergeCompoundRef(const std::wstring& content,
                                      std::wstring& compoundRef,
                                      std::wstring& anchorName,
                                      int& compoundIndex,
                                      ReactionScheme& scheme);
    static bool parseChemname(const wchar_t*& p, ReactionScheme& scheme);

    static ArrowType parseArrowCode(const wchar_t*& p, std::wstring& labelAbove, std::wstring& labelBelow);
    static ArrowParams parseArrowArgs(const wchar_t*& p);

    static std::wstring parseBraceContent(const wchar_t*& p);
    static std::wstring parseBracketContent(const wchar_t*& p);
    static std::wstring parseAtRef(const wchar_t*& p);

    static void resolveReferences(ReactionScheme& scheme);

    static bool parseContent(const wchar_t* p, ReactionScheme& scheme);

    static wchar_t peek(const wchar_t* p);
    static void skipWhitespace(const wchar_t*& p);
    static bool match(const wchar_t*& p, wchar_t expected);
};

} // namespace tex

#endif // SCHEME_PARSER_H_INCLUDED

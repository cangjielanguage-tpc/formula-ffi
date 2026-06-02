#ifndef CHEMFIG_PARSER_H_INCLUDED
#define CHEMFIG_PARSER_H_INCLUDED

#include "chemfig_types.h"
#include "chemfig_ast.h"
#include <string>
#include <map>
#include <memory>

namespace tex {

class ChemfigParser {
public:
    static bool parse(const std::wstring& input, Molecule& mol);
    static std::shared_ptr<ParseAST> parseToAST(const std::wstring& input);
    static bool parseDrawCommand(const wchar_t*& p, std::vector<CurvePath>& curves);

    static std::wstring collectBalanced(const wchar_t*& p, wchar_t open, wchar_t close);
    static void skipBalanced(const wchar_t*& p, wchar_t open, wchar_t close);
    static std::wstring parseBraceArg(const wchar_t*& p);
    static bool skipBraceArg(const wchar_t*& p);

    static bool parseChargeSpec(const wchar_t*& p, std::vector<Charge>& charges);

    static wchar_t peek(const wchar_t* p);
    static wchar_t peekNext(const wchar_t* p);
    static void skipWhitespace(const wchar_t*& p);
    static bool match(const wchar_t*& p, wchar_t expected);

private:
    static bool parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex);
    static bool parseRing(const wchar_t*& p, Molecule& mol, int& atomIndex,
                           int sharedFromAtom, int sharedToAtom, BondType* sharedBondType = nullptr);
    static bool parseChain(const wchar_t*& p, Molecule& mol, int& atomIndex, int prevAtom, float currentAngle);
    static bool parseBranch(const wchar_t*& p, Molecule& mol, int& atomIndex, int branchAtom, float branchAngle);
    static BondType parseBondType(const wchar_t*& p);
    static BondParams parseBondParams(const wchar_t*& p, float currentAngle);
    static std::wstring parseAtomGroup(const wchar_t*& p);
    static bool tryProcessLatexCommand(const wchar_t*& p, std::wstring& label);
    static std::wstring parseHookName(const wchar_t*& p);
    static std::wstring parseAnchorName(const wchar_t*& p);
    static float parseAngleSpec(const wchar_t*& p, float currentAngle);
    static float parseNumber(const wchar_t*& p);
    static void resolveHooks(Molecule& mol);
    static void parseAndApplyChargeToAtom(Molecule& mol, int atomIndex, std::wstring& label);
    static bool parseChemmove(const wchar_t*& p, Molecule& mol);

    static std::shared_ptr<AstChainNode> parseChainToAST(const wchar_t*& p, int& atomIndex, bool inBranch);
    static std::shared_ptr<AstRingNode> parseRingToAST(const wchar_t*& p, int& atomIndex,
                                                        int sharedFromAtom, int sharedToAtom);
    static AstAtomSpec parseAtomGroupToAST(const wchar_t*& p);
    static bool parseChainElementsToAST(const wchar_t*& p, std::vector<AstChainElement>& elements,
                                         int& atomIndex, bool inBranch);
};

} // namespace tex

#endif // CHEMFIG_PARSER_H_INCLUDED

#ifndef CHEMFIG_BUILDER_H_INCLUDED
#define CHEMFIG_BUILDER_H_INCLUDED

#include "chemfig_types.h"
#include "chemfig_ast.h"
#include <vector>
#include <string>

namespace tex {

class MoleculeBuilder {
public:
    static Molecule build(const ParseAST& ast);

    static std::wstring extractPureAtomText(const std::wstring& label);
    static void parseAndApplyChargeToAtom(Molecule& mol, int atomIndex, std::wstring& label);
};

} // namespace tex

#endif // CHEMFIG_BUILDER_H_INCLUDED

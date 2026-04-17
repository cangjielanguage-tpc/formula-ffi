#ifndef CHEMFIG_ATOM_H_INCLUDED
#define CHEMFIG_ATOM_H_INCLUDED

#include "chemfig_parser.h"
#include "chemfig_box.h"
#include "atom/atom.h"

namespace tex {

class ChemfigAtom : public Atom {
private:
    std::wstring _code;
    Molecule _molecule;

public:
    ChemfigAtom(const std::wstring& code);
    
    sptr<Box> createBox(TeXEnvironment& env) override;
    
    const Molecule& getMolecule() const { return _molecule; }

    __decl_clone(ChemfigAtom)
};

} // namespace tex

#endif // CHEMFIG_ATOM_H_INCLUDED

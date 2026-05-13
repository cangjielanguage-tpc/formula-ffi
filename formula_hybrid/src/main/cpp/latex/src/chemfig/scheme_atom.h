#ifndef SCHEME_ATOM_H_INCLUDED
#define SCHEME_ATOM_H_INCLUDED

#include "scheme_parser.h"
#include "scheme_box.h"
#include "atom/atom.h"

namespace tex {

class SchemeAtom : public Atom {
private:
    std::wstring _code;
    ReactionScheme _scheme;

public:
    SchemeAtom(const std::wstring& code);

    sptr<Box> createBox(TeXEnvironment& env) override;

    __decl_clone(SchemeAtom)
};

} // namespace tex

#endif // SCHEME_ATOM_H_INCLUDED

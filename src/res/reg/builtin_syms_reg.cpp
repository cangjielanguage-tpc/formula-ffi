#include "res/reg/builtin_syms_reg.h"

DEF_SYMBOLS_SET(Builtin)

REG_SYMBOLS(base)
REG_SYMBOLS(amssymb)
REG_SYMBOLS(amsfonts)
REG_SYMBOLS(stmaryrd)
REG_SYMBOLS(special)
#ifdef CHEMICAL_SYMBOLS
REG_SYMBOLS(mhchem)
#endif

END_DEF_SYMBOLS_SET

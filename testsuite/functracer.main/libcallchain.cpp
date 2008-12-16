#include <stdlib.h>

#include "libcallchain_cpp.h"

static void lib_do_backtrace(void)
{
        free(malloc(222));
}

// This is the "terminal" class, usually inlined
class libX {
public:
        void lib_x(int i, ...) { lib_do_backtrace(); }
};

#define def_class(cls1, cls2, meth1, meth2) \
void cls1::meth1(int i, ...) { cls2().meth2(i + 1, this); }

def_class(libC, libX, lib_h, lib_x)
def_class(libB, libC, lib_g, lib_h)
def_class(libA, libB, lib_f, lib_g)

#undef def_class

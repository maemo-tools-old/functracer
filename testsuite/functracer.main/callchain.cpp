#include <stdlib.h>

#include "libcallchain_cpp.h"

static void do_backtrace(void)
{
        free(malloc(111));
}

// This is the "terminal" class, usually inlined
class X {
public:
        void x(int i, ...) { do_backtrace(); libA().lib_f(1, this); }
};

#define def_class(cls1, cls2, meth1, meth2) \
class cls1 { \
public: \
        void meth1(int i, ...) { cls2().meth2(i + 1, this); } \
};

def_class(C, X, h, x)
def_class(B, C, g, h)
def_class(A, B, f, g)

int main(void)
{
        A().f(1, &main);
        return 0;
}

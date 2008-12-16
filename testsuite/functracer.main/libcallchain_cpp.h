#define dec_class(cls, meth) \
class cls { \
public: \
        void meth(int i, ...); \
};

dec_class(libC, lib_h)
dec_class(libB, lib_g)
dec_class(libA, lib_f)

#undef dec_class

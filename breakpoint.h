#ifndef TT_BREAKPOINT_H
#define TT_BREAKPOINT_H

#ifdef __i386__

#define BREAKPOINT_VALUE {0xcc}
#define BREAKPOINT_LENGTH 1
#define DECR_PC_AFTER_BREAK 1

#endif

struct breakpoint {
        void *addr;
        unsigned char orig_value[BREAKPOINT_LENGTH];
        int enabled;
        struct library_symbol *libsym;
};

#endif /* TT_BREAKPOINT_H */

#ifndef TT_LIBRARY_H
#define TT_LIBRARY_H

enum toplt {
	LS_TOPLT_NONE = 0,	/* PLT not used for this symbol. */
	LS_TOPLT_EXEC,		/* PLT for this symbol is executable. */
	LS_TOPLT_POINT		/* PLT for this symbol is a non-executable. */
};

struct library_symbol {
	char *name;
	void *enter_addr;
	char needs_init;
	enum toplt plt_type;
	char is_weak;
	struct library_symbol *next;
};

#endif /* TT_LIBRARY_H */

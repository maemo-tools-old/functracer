
extern void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));
#define msg_dbg(level, expr...) debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, expr)

extern void msg_warn(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void msg_err(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void error_file(char *filename, char *msg);

/* XXX deprecated */
#define error_exit(fmt...)	msg_err(fmt)
#define debug(level, fmt...)	msg_dbg(level, fmt)


extern void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));
#define msg_dbg(level, ...) debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

extern void msg_warn(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void msg_err(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));
extern void error_file(const char *filename, const char *msg);

/* XXX deprecated */
#define error_exit(...)		msg_err(__VA_ARGS__)
#define debug(level, ...)	msg_dbg(level, __VA_ARGS__)

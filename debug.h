
void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));

#define debug(level, expr...) debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, expr)

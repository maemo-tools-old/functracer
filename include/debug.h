
void debug_(int level, const char *file, int line, const char *func,
	    const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));

#define debug(level, expr...) debug_(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, expr)

void error_exit(const char *fmt, ...) __attribute__ ((format(printf, 1, 2)));

#define error_msg(expr...) debug_(0, __FILE__, __LINE__, __PRETTY_FUNCTION__, expr)

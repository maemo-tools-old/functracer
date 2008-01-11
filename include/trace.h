#ifndef FTK_TRACE_H
#define FTK_TRACE_H

#include <sys/types.h>

extern int trace_main_loop(void);
extern int trace_attach(pid_t pid);
extern int trace_execute(char *filename, char *argv[]);

#endif /* !FTK_TRACE_H */

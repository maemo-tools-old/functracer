#ifndef FTK_TRACE_H
#define FTK_TRACE_H

#include <sys/types.h>

extern int exiting;

extern int trace_main_loop(void);
extern void trace_attach(pid_t pid);
extern int trace_execute(char *filename, char *argv[]);

#endif /* !FTK_TRACE_H */

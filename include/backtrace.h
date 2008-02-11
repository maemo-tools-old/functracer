#ifndef FTK_BACKTRACE_H
#define FTK_BACKTRACE_H

#include <sys/types.h>

struct bt_data;

extern struct bt_data *bt_init(pid_t pid);
extern int bt_backtrace(struct bt_data *btd, char **buffer, int size);
extern void bt_finish(struct bt_data *btd);

#endif /* FTK_BACKTRACE_H */

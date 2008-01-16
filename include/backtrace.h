#ifndef FTK_BACKTRACE_H
#define FTK_BACKTRACE_H

#include <sys/types.h>
#include <libunwind.h>

struct bt_data {
	unw_addr_space_t as;
	struct UPT_info *ui;
};

extern struct bt_data *bt_init(pid_t pid);
extern int bt_backtrace(struct bt_data *btd, char **buffer, int size);
extern void bt_finish(struct bt_data *btd);

#endif /* FTK_BACKTRACE_H */

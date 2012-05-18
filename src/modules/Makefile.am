AM_CFLAGS = -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes \
	-Wsign-compare -Wformat-security \
	-Wbad-function-cast -Wpointer-arith \
	 -fPIC
#-Wwrite-strings -Wcast-qual

pkglib_LTLIBRARIES = memory.la file.la
memory_la_SOURCES = memory.c
memory_la_LDFLAGS = -no-undefined -module -avoid-version
file_la_SOURCES = file.c
file_la_LDFLAGS = -no-undefined -module -avoid-version

pkglib_LTLIBRARIES += gobject.la
gobject_la_SOURCES = gobject.c
gobject_la_LDFLAGS = -no-undefined -module -avoid-version 

pkglib_LTLIBRARIES += memtransfer.la
memtransfer_la_SOURCES = memtransfer.c
memtransfer_la_LDFLAGS = -no-undefined -module -avoid-version

pkglib_LTLIBRARIES += thread.la
thread_la_SOURCES = thread.c
thread_la_LDFLAGS = -no-undefined -module -avoid-version -lpthread

pkglib_LTLIBRARIES += shmsysv.la
shmsysv_la_SOURCES = shmsysv.c
shmsysv_la_LDFLAGS = -no-undefined -module -avoid-version

pkglib_LTLIBRARIES += audit.la
audit_la_SOURCES = audit.c
audit_la_LDFLAGS = -no-undefined -module -avoid-version

pkglib_LTLIBRARIES += shmposix.la
shmposix_la_SOURCES = shmposix.c
shmposix_la_LDFLAGS = -no-undefined -module -avoid-version

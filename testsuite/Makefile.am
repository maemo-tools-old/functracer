AUTOMAKE_OPTIONS = dejagnu

SUBDIRS = functracer.file functracer.main functracer.memory functracer.memtransfer \
	functracer.shmsysv functracer.audit functracer.gobject functracer.thread

# Reset compiler/linker variables so that tests do not link with Functracer
# dependencies
CFLAGS =
LDFLAGS =
LIBS =

CLEANFILES = *.log *.sum site.bak setval.tmp site.exp

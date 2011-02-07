SUFFIXES:      
clean-local:
	-rm -f creat fdopen fopen freopen open dup dup2 fcntl \
		inotify_init open64 pipe pie2 socket
	-rm -f *.o *.so 
	-rm -f *.rtrace.txt
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

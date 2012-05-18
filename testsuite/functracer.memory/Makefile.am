SUFFIXES:      
clean-local:
	-rm -f calloc malloc_recursive malloc_simple memalign posix_memalign realloc valloc
	-rm -f *.o *.so
	-rm -f *.rtrace.txt
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

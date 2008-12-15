SUFFIXES:      
clean-local:
	-rm -f malloc_recursive malloc_simple
	-rm -f *.o *.so 
	-rm -f *.trace
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

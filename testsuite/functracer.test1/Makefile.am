SUFFIXES:      
clean-local:
	-rm -f malloc_simple malloc_recursive 
	-rm -f *.o *.so 
	-rm -f *.trace
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

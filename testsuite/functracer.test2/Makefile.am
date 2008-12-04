SUFFIXES:      
clean-local:
	-rm -f fork clone gthreads 
	-rm -f *.o 
	-rm -f *.trace
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

SUFFIXES:      
clean-local:
	-rm -f callchain callchain_cpp clone fork gthreads
	-rm -f *.o *.so 
	-rm -f *.trace
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

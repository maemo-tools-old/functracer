SUFFIXES:      
clean-local:
	-rm -f audit
	-rm -f *.o *.so 
	-rm -f *.rtrace.txt
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

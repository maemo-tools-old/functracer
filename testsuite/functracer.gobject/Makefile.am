SUFFIXES:      
clean-local:
	-rm -f gobject
	-rm -f *.o *.so 
	-rm -f *.rtrace.txt
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

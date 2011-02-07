SUFFIXES:      
clean-local:
	-rm -f shm_open mmap mmap64
	-rm -f *.o *.so 
	-rm -f *.rtrace.txt
	-rm -f $(CLEANFILES)

distclean-local: clean
	-rm -f Makefile

.PHONY: $(RECURSIVE_TARGETS) check  clean distclean realclean

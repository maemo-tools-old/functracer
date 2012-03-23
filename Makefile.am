SUBDIRS = src testsuite

DISTCLEANFILES = configure Makefile.in ltmain.sh install-sh depcomp aclocal.m4 config.log man/functracer.1

# Man page support
man1_MANS = man/*.1

man/*.1: man-local

man-local:
	help2man src/functracer -N -o man/functracer.1 -i man/functracer.1.inc
	
distclean-local:
	$(RM) man/*.1

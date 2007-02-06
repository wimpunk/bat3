#bin_PROGRAMS=bat3
#bat3_SOURCES=bat3.c xmalloc.c system.h aclocal.m4 bat3-opt.c bat3-opt.h
#
##  add " -lm" to the following if bat3 requires the math library
#bat3_LDADD=-lopts
#
#EXTRA_DIST=bat3.lsm.in bat3.spec.in bat3-opt.def bat3.1 manpage.tpl
#
##  if you write a self-test script named `chk', uncomment the
##  following and add `chk' to the EXTRA_DIST list
##TESTS=chk
#
#bat3.o: bat3-opt.h
#bat3-opt.o: bat3-opt.c
#bat3-opt.c bat3-opt.h: $(srcdir)/bat3-opt.def
#	autogen -L/usr/share/autogen $(srcdir)/bat3-opt.def
#
#bat3.1: $(srcdir)/bat3-opt.def
#	autogen -T manpage.tpl  $(srcdir)/bat3-opt.def
#	awk -f updater bat3.1 > tmp$$$$ && mv tmp$$$$ bat3.1
##	rm updater
#
## info_TEXINFOS = bat3.texinfo
## bat3_TEXINFOS = gpl.texinfo
#
##  install the man page
#man_MANS=bat3.1
SUBDIRS=src

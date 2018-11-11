include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS +=  -DGEGL_OP_BUNDLE
AM_CXXFLAGS +=  -DGEGL_OP_BUNDLE
LIBS = $(op_libs)


opdir = $(ext_dir)
op_LTLIBRARIES = \
	gegl-common-cxx.la

gegl_common_cxx_la_SOURCES =\
	module.c \
	warp.cc

module.c: Makefile $(gegl_common_cxx_la_SOURCES)
	$(AM_V_GEN) \
	$(top_srcdir)/tools/gen-loader.sh `echo $^ | cut -d' ' -f2-` > module.c

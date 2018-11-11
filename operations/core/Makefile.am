include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS += -DGEGL_OP_BUNDLE

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
    gegl-core.la

gegl_core_la_SOURCES =\
    module.c \
	cache.c \
	load.c \
	clone.c \
	cast-format.c \
	cast-space.c \
	convert-format.c \
	convert-space.c \
	crop.c \
	json.c \
	nop.c

module.c: Makefile $(gegl_core_la_SOURCES)
	$(AM_V_GEN) \
	$(top_srcdir)/tools/gen-loader.sh `echo $^ | cut -d' ' -f2-` > module.c


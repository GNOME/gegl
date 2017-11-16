SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES =    \
	bayer-matrix.la \
	bilateral-filter-fast.la \
	color-to-alpha-plus.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	gradient-map.la \
	hstack.la \
	integral-image.la \
	linear-sinusoid.la \
	rawbayer-load.la \
	segment-kmeans.la \
	spherize.la

SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
	bayer-matrix.la \
	bilateral-filter-fast.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	domain-transform.la \
	gradient-map.la \
	hstack.la \
	enlarge.la \
	enlarge2.la \
	inpaint.la \
	integral-image.la \
	linear-sinusoid.la \
	rawbayer-load.la  \
	segment-kmeans.la

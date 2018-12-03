SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

ops = \
	bayer-matrix.la \
	bilateral-filter-fast.la \
	connected-components.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	domain-transform.la \
	gradient-map.la \
	gcr.la \
	hstack.la \
	enlarge.la \
	aces-rrt.la \
	inpaint.la \
	integral-image.la \
	linear-sinusoid.la \
	rawbayer-load.la  \
	segment-kmeans.la \
	selective-hue-saturation.la

if HAVE_CXX14

voronoi_diagram_la_SOURCES = voronoi-diagram.cc

ops += \
	voronoi-diagram.la

endif

opdir = $(ext_dir)
op_LTLIBRARIES = $(ops)

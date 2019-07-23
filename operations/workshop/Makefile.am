SUBDIRS = generated external

include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)

LIBS = $(op_libs)

ops = \
	bilateral-filter-fast.la \
	boxblur.la \
	boxblur-1d.la \
	connected-components.la \
	demosaic-bimedian.la \
	demosaic-simple.la \
	ditto.la \
	gradient-map.la \
	gcr.la \
	hstack.la \
	aces-rrt.la \
	alpha-inpaint.la \
	integral-image.la \
	munker-tartan.la \
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

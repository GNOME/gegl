include $(top_srcdir)/operations/Makefile-common.am

EXTRA_DIST = $(wildcard $(srcdir)/*.h)

AM_CPPFLAGS += -I$(srcdir)
AM_CFLAGS +=  -DGEGL_OP_BUNDLE
LIBS = $(op_libs)

opdir = $(ext_dir)
op_LTLIBRARIES = \
	gegl-common-gpl3.la 

gegl_common_gpl3_la_SOURCES =\
    	module.c \
	antialias.c \
	apply-lens.c \
	bump-map.c \
	cartoon.c \
	channel-mixer.c \
	color-exchange.c \
	color-to-alpha.c \
	cubism.c \
	deinterlace.c \
	diffraction-patterns.c \
	displace.c \
	edge-laplace.c \
	edge.c \
	emboss.c \
	engrave.c \
	fractal-explorer.c \
	fractal-trace.c \
	gaussian-blur-selective.c \
	illusion.c \
	lens-distortion.c \
	lens-flare.c \
	maze.c \
	mosaic.c \
	motion-blur-circular.c \
	motion-blur-zoom.c \
	noise-slur.c \
	noise-solid.c \
	oilify.c \
	photocopy.c \
	plasma.c \
	polar-coordinates.c \
	red-eye-removal.c \
	ripple.c \
	shadows-highlights.c \
	shadows-highlights-correction.c \
	shift.c \
	sinus.c \
	softglow.c \
	spiral.c \
	supernova.c \
	texturize-canvas.c \
	tile-glass.c \
	tile-paper.c \
	value-propagate.c \
	video-degradation.c \
	waves.c \
	whirl-pinch.c \
	wind.c

module.c: $(gegl_common_gpl3_la_SOURCES)
	$(top_srcdir)/tools/gen-loader.sh $^ > module.c

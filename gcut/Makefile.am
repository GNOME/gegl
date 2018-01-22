if OS_WIN32
no_undefined = -no-undefined
endif

EXTRA_DIST = default.edl default.edl.inc

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/graph \
	-I$(top_srcdir)/gegl/graph \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl \
	-I$(top_builddir)/gegl/process \
	-I$(top_srcdir)/gegl/process \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-Wno-missing-prototypes

AM_CFLAGS = \
	$(DEP_CFLAGS) $(BABL_CFLAGS) $(PNG_CFLAGS) \
        $(MRG_CFLAGS) $(GEXIV2_CFLAGS)

AM_LDFLAGS =  \
	$(no_undefined) ../gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(BABL_LIBS) $(PNG_LIBS) $(LIBSPIRO) $(MATH_LIB) \
        $(MRG_LIBS) $(GEXIV2_LIBS)

bin_PROGRAMS = gcut

default.edl.inc: default.edl
	cat $< | \
	sed 's/\\/\\\\/g' | \
	sed 's/\r/a/' | \
	sed 's/"/\\"/g' | \
	sed 's/^/"/' | \
	sed 's/$$/\\n"/' > $@

gcut_SOURCES =			\
	gcut.c			\
	gcut-util.c		\
        default.edl.inc		\
	gcut.h			\
	iconographer.c		\
	clip.c

if HAVE_MRG
gcut_SOURCES += gcut-ui.c renderer.c
AM_CFLAGS += $(SDL_CFLAGS)
AM_LDFLAGS += $(SDL_LIBS)
endif

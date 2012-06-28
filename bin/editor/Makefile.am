if OS_WIN32
no_undefined = -no-undefined
endif

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir)/gegl \
	-I$(top_srcdir)/gegl \
	-I$(top_builddir)/gegl/buffer \
	-I$(top_srcdir)/gegl/buffer \
	-I$(top_builddir)/gegl/property-types \
	-I$(top_srcdir)/gegl/property-types \
	-I$(top_builddir)/gegl/operation \
	-I$(top_srcdir)/gegl/operation \
	-I$(top_builddir)/gegl/module \
	-I$(top_srcdir)/gegl/module \
	-I$(top_builddir)/gegl/opencl \
	-I$(top_srcdir)/gegl/opencl

AM_CFLAGS = \
	$(DEP_CFLAGS) $(BABL_CFLAGS) $(GTK_CFLAGS)


AM_LDFLAGS =  \
	$(no_undefined) ../../gegl/libgegl-$(GEGL_API_VERSION).la \
	$(DEP_LIBS) $(BABL_LIBS) $(GTK_LIBS) $(LIBSPIRO)

bin_PROGRAMS = editor

editor_SOURCES = 		            \
			gegl-editor.c       \
			gegl-editor-layer.c \
			gegl-node-widget.c


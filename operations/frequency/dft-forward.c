/* This file is a part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2008 Zhang Junbo  <zhangjb@svn.gnome.org>
 */

#ifdef GEGL_CHANT_PROPERTIES

/* no properties */

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "dft-forward.c"

#include <gegl-chant.h>
#include "tools/fourier.c"
#include "tools/component.c"

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static GeglRectangle
get_required_for_output(GeglOperation *operation,
                        const gchar *input_pad,
                        const GeglRectangle *roi)
{
  return  *gegl_operation_source_get_bounding_box(operation, "input");
}

static GeglRectangle
get_cached_region(GeglOperation *operation,
                  const GeglRectangle *roi)
{
  return get_bounding_box(operation);
}

static void
prepare(GeglOperation *operation)
{
  gegl_operation_set_format(operation, "input", babl_format ("RGBA double"));
  gegl_operation_set_format(operation, "output",
                            babl_format_n ("double", 8));
}

static gboolean
process(GeglOperation *operation,
        GeglBuffer *input,
        GeglBuffer *output,
        const GeglRectangle *result)
{
  gint width = gegl_buffer_get_width(input);
  gint height = gegl_buffer_get_height(input);
  gdouble *src_buf;
  gdouble *dst_buf;
  gdouble *tmp_src_buf;
  gdouble *tmp_dst_buf;
  gint i;

  src_buf = g_new0(gdouble, 4*width*height);
  tmp_src_buf = g_new0(gdouble, width*height);
  dst_buf = g_new0(gdouble, 8*height*(width));
  tmp_dst_buf = g_new0(gdouble, 2*height*(width));

  gegl_buffer_get(input, 1.0, NULL, babl_format ("RGBA double"), src_buf,
                  GEGL_AUTO_ROWSTRIDE);

  for (i=0; i<4; i++)
    {
      get_rgba_component(src_buf, tmp_src_buf, i, width*height);
      dft(tmp_src_buf, (fftw_complex *)tmp_dst_buf, width, height);
      set_complex_component(tmp_dst_buf, dst_buf, i, FFT_HALF(width)*height);
    }
  
  gegl_buffer_set(output, NULL, babl_format_n ("double", 8), dst_buf,
                  GEGL_AUTO_ROWSTRIDE);

  g_free(src_buf);
  g_free(dst_buf);
  g_free(tmp_src_buf);
  g_free(tmp_dst_buf);
  return TRUE;
}

static void
gegl_chant_class_init(GeglChantClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS(klass);
  filter_class = GEGL_OPERATION_FILTER_CLASS(klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output= get_required_for_output;
  operation_class->get_cached_region = get_cached_region;

  operation_class->name = "dft-forward";
  operation_class->categories = "frequency";
  operation_class->description
    = "Perform 2-D Discrete Fourier Transform for a RGBA image.";
}

#endif

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

gegl_chant_pointer(filter_real, "Matrix", "The transfer function matrix(real part).")
gegl_chant_pointer(filter_imag, "Matrix", "The transfer function matrix(imag part).")
gegl_chant_int(flag, "Flag", 0, 15, 14,
               "Decide which componet need to process. Example: if flag=14, "
               "14==0b1110, so we filter on componets RGB, do not filter on A.")

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "freq-general-filter.c"

#include "gegl-chant.h"
#include "tools/component.c"
#include "tools/filters.c"

static void
prepare(GeglOperation *operation)
{
  Babl *format = babl_format_n ("double", 8);
  gegl_operation_set_format(operation, "input", format);
  gegl_operation_set_format(operation, "output", format);
}

static gboolean
process(GeglOperation *operation,
        GeglBuffer *input,
        GeglBuffer *output,
        const GeglRectangle *result)
{
  gint width = gegl_buffer_get_width(input);
  gint height = gegl_buffer_get_height(input);
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gdouble *src_buf;
  gdouble *dst_buf;
  gdouble *comp_real;
  gdouble *comp_imag;
  gdouble *Hr_buf = o->filter_real;
  gdouble *Hi_buf = o->filter_imag;
  gint flag = o->flag;
  gint i;

  src_buf = g_new0(gdouble, 8*width*height);
  dst_buf = g_new0(gdouble, 8*width*height);    
  comp_real = g_new0(gdouble, FFT_HALF(width)*height);
  comp_imag = g_new0(gdouble,FFT_HALF(width)*height);  
  gegl_buffer_get(input, 1.0, NULL, babl_format_n ("double", 8), src_buf,
                  GEGL_AUTO_ROWSTRIDE);  
  for (i=0; i<4; i++)
    {
      get_freq_component(src_buf, comp_real, i, FFT_HALF(width)*height);
      get_freq_component(src_buf, comp_imag, 4+i, FFT_HALF(width)*height);

      if ((8>>i)&flag)
        {
          freq_multiply(comp_real, comp_imag, Hr_buf, Hi_buf, width, height);
        }

      set_freq_component(comp_real, dst_buf, i, FFT_HALF(width)*height);
      set_freq_component(comp_imag, dst_buf, 4+i, FFT_HALF(width)*height);
    }    
  gegl_buffer_set(output, NULL, babl_format_n ("double", 8), dst_buf,
                  GEGL_AUTO_ROWSTRIDE);

  g_free(src_buf);
  g_free(dst_buf);
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

  operation_class->name = "freq-general-filter";
  operation_class->categories = "frequency";
  operation_class->description
    = "The most general filer in frequency domain. What it does is just "
    "multiplying a matrix on the freqeuncy image.";
}

#endif

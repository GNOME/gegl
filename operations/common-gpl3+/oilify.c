/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 1995 Spencer Kimball and Peter Mattis
 * Copyright 1996 Torsten Martinsen
 * Copyright 2007 Daniel Richard G.
 * Copyright 2011 Hans Lo <hansshulo@gmail.com>
 * Copyright 2019 Øyvind Kolås
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int    (mask_radius, _("Mask Radius"), 4)
    description (_("Radius of circle around pixel, can also be scaled per "
                   "pixel by a buffer on the aux pad."))
    value_range (1, 100)
    ui_range (1, 25)
    ui_meta     ("unit", "pixel-distance")

property_int    (exponent, _("Exponent"), 8)
    description (_("Exponent for processing; controls smoothness - can be "
                   "scaled per pixel with a buffer on the aux2 pad."))
    value_range (1, 20)

property_int (intensities, _("Number of intensities"), 128)
    description(_("Histogram size"))
    value_range (8, 256)

property_boolean (use_inten, _("Intensity Mode"), TRUE)
    description(_("Use pixel luminance values"))

#else

#define GEGL_OP_COMPOSER3
#define GEGL_OP_NAME     oilify
#define GEGL_OP_C_SOURCE oilify.c

#include "gegl-op.h"

#define NUM_INTENSITIES       256

static void
clamp_buffer_values (gfloat  *buf,
                     gint     n_components,
                     gint     n_pixels)
{
  gint  i;

  for (i = 0; i < n_pixels * n_components; i++)
    buf[i] = CLAMP (buf[i], 0.f, 1.f);
}

/* Get the pixel from x, y offset from the center pixel src_pix */

static void
get_pixel (gint    x,
           gint    y,
           gint    buf_width,
           gfloat *src_begin,
           gfloat *dst)
{
  gint    b;
  gfloat *src = src_begin + 4 * (x + buf_width * y);

  for (b = 0; b < 4; b++)
    {
      dst[b] = src[b];
    }
}

static void
get_pixel_inten (gint    x,
                 gint    y,
                 gint    buf_width,
                 gfloat *inten_begin,
                 gfloat *dst)
{
  *dst = *(inten_begin + (x + buf_width*y));
}

static void
oilify_pixel_inten (gint     x,
                    gint     y,
                    gdouble  radius,
                    gint     exponent,
                    gint     intensities,
                    gint     buf_width,
                    gfloat  *src_buf,
                    gfloat  *inten_buf,
                    gfloat  *dst_pixel)
{
  gfloat cumulative_rgb[4][NUM_INTENSITIES];
  gint   hist_inten[NUM_INTENSITIES];
  gfloat mult_inten;
  gfloat temp_pixel[4];
  gint   ceil_radius = ceil (radius);
  gdouble radius_sq = radius * radius;
  gint i, j, b;
  gint inten_max;
  gint intensity;
  gfloat ratio, temp_inten_pixel;
  gfloat weight;
  gfloat color[4];
  gfloat div;

  for (i = 0; i < intensities; i++)
    {
      hist_inten[i] = 0;

      for (b = 0; b < 4; b++)
          cumulative_rgb[b][i] = 0.f;
    }

  /* calculate histograms */
  for (i = -ceil_radius; i <= ceil_radius; i++)
    {
      for (j = -ceil_radius; j <= ceil_radius; j++)
        {
          if (i*i + j*j <= radius_sq)
            {
              get_pixel (x + i,
                         y + j,
                         buf_width,
                         src_buf,
                         temp_pixel);
              get_pixel_inten (x + i,
                               y + j,
                               buf_width,
                               inten_buf,
                               &temp_inten_pixel);
              intensity = temp_inten_pixel * (intensities - 1);
              hist_inten[intensity]++;

              for (b = 0; b < 4; b++)
                {
                  cumulative_rgb[b][intensity] += temp_pixel[b];
                }
            }
        }
    }

  inten_max = 1;

  /* calculated maximums */

  for (i = 0; i < intensities; i++)
    inten_max = MAX (inten_max, hist_inten[i]);

  /* calculate weight and use it to set the pixel */

  div = 0.f;

  for (b = 0; b < 4; b++)
    color[b] = 0.f;

  for (i = 0; i < intensities; i++)
    {
      if (hist_inten[i] > 0)
        {
          ratio = (gfloat) hist_inten[i] / (gfloat) inten_max;

          /* using this instead of pow function gives HUGE performance
             improvement but we cannot use floating point exponent... */

          weight = 1.;

          for(j = 0; j < exponent; j++)
            weight *= ratio;

          /* weight = powf(ratio, exponent); */
          mult_inten = weight / (gfloat) hist_inten[i];

          div += weight;

          for (b = 0; b < 4; b++)
            color[b] += mult_inten * cumulative_rgb[b][i];
        }
    }

  for (b = 0; b < 4; b++)
    dst_pixel[b] = color[b] / div;
}

static void
oilify_pixel (gint     x,
              gint     y,
              gdouble  radius,
              gint     exponent,
              gint     intensities,
              gint     buf_width,
              gfloat  *src_buf,
              gfloat  *dst_pixel)
{
  gint   hist[4][NUM_INTENSITIES];
  gfloat temp_pixel[4];
  gint   ceil_radius = ceil (radius);
  gdouble radius_sq = radius * radius;
  gint i, j, b;
  gint hist_max[4];
  gint intensity;
  gfloat sum[4];
  gfloat ratio;
  gfloat weight;
  gfloat result[4];
  gfloat div[4];

  for (i = 0; i < intensities; i++)
    {
      for (b = 0; b < 4; b++)
        {
          hist[b][i] = 0;
        }
    }

  /* calculate histograms */

  for (i = -ceil_radius; i <= ceil_radius; i++)
    {
      for (j = -ceil_radius; j <= ceil_radius; j++)
        {
          if (i*i + j*j <= radius_sq)
            {
              get_pixel (x + i,
                         y + j,
                         buf_width,
                         src_buf,
                         temp_pixel);

              for (b = 0; b < 4; b++)
                {
                  intensity = temp_pixel[b] * (intensities - 1);
                  hist[b][intensity]++;
                }
            }
        }
    }

  for (b = 0; b < 4; b++)
    hist_max[b] = 1;

  for (i = 0; i < intensities; i++)
    {
      for (b = 0; b < 4; b++)
        {
          if (hist_max[b] < hist[b][i]) /* MAX macros too slow here */
            hist_max[b] = hist[b][i];
        }
    }

  /* calculate weight and use it to set the pixel */

  for (b = 0; b < 4; b++)
    {
      sum[b] = 0.f;
      div[b] = 0.f;
    }

  for (i = 0; i < intensities; i++)
    {
      /* UNROLL this bottleneck loop, up to 50% faster */
      #define DO_HIST_STEP(b) if(hist[b][i] > 0)                          \
        {                                                                 \
          ratio = (gfloat) hist[b][i] / (gfloat) hist_max[b];             \
          weight = 1.f;                                                   \
                                                                          \
          for (j = 0; j < exponent; j++)                                  \
            weight *= ratio;                                              \
                                                                          \
          sum[b] += weight * (gfloat) i;                                  \
          div[b] += weight;                                               \
        }

      DO_HIST_STEP(0)
      DO_HIST_STEP(1)
      DO_HIST_STEP(2)
      DO_HIST_STEP(3)
      #undef DO_HIST_STEP
    }

  for (b = 0; b < 4; b++)
    {
      result[b] = sum[b] / (gfloat) (intensities - 1);
      dst_pixel[b] = result[b] / div[b];
    }
}

static void
prepare (GeglOperation *operation)
{
  const Babl  *space = gegl_operation_get_source_space (operation, "input");

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("RGBA float", space));
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle rect = *region;

  rect.x -= o->mask_radius;
  rect.y -= o->mask_radius;
  rect.width  += o->mask_radius * 2;
  rect.height += o->mask_radius * 2;

  return rect;
}

#include "opencl/gegl-cl.h"
#include "gegl-buffer-cl-iterator.h"

#include "opencl/oilify.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_oilify (cl_mem               in_tex,
           cl_mem               out_tex,
           cl_mem               inten_tex,
           size_t               global_worksize,
           const GeglRectangle *roi,
           gint                 mask_radius,
           gint                 number_of_intensities,
           gint                 exponent,
           gboolean             use_inten)
{

  const size_t gbl_size[2] = {roi->width, roi->height};
  cl_int radius      = ceil (mask_radius);
  cl_int intensities = number_of_intensities;
  cl_float exp       = (gfloat) exponent;
  cl_int cl_err      = 0;
  gint arg = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"kernel_oilify", "kernel_oilify_inten", NULL};
      cl_data = gegl_cl_compile_and_build (oilify_cl_source, kernel_name);
    }

  if (!cl_data)
    return TRUE;

  /* simple hack: select suitable kernel using boolean:
   * 0 - no intensity mode
   * 1 - intensity mode
   */

  cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                               sizeof(cl_mem), (void *) &in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                                sizeof(cl_mem), (void *) &out_tex);
  CL_CHECK;
  if (use_inten)
  {
    cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                                  sizeof (cl_mem), (void *) &inten_tex);
    CL_CHECK;
  }
  cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                                sizeof(cl_int), (void *) &radius);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                                sizeof(cl_int), (void *) &intensities);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg (cl_data->kernel[use_inten], arg++,
                                sizeof(cl_float), (void *) &exp);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel (gegl_cl_get_command_queue(),
                                        cl_data->kernel[use_inten], 2,
                                        NULL, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

static gboolean
cl_process (GeglOperation       *operation,
            GeglBuffer          *input,
            GeglBuffer          *output,
            const Babl          *inten_format,
            const GeglRectangle *result)
{
  const Babl *in_format  = gegl_operation_get_format (operation, "input");
  const Babl *out_format = gegl_operation_get_format (operation, "output");
  gint  inten_buf        = 0;
  gint  err;

  GeglProperties *o = GEGL_PROPERTIES (operation);

  GeglBufferClIterator *i = gegl_buffer_cl_iterator_new (output,
                                                         result,
                                                         out_format,
                                                         GEGL_CL_BUFFER_WRITE);

  gint read = gegl_buffer_cl_iterator_add_2 (i,
                                             input,
                                             result,
                                             in_format,
                                             GEGL_CL_BUFFER_READ,
                                             o->mask_radius,
                                             o->mask_radius,
                                             o->mask_radius,
                                             o->mask_radius,
                                             GEGL_ABYSS_CLAMP);

  if (o->use_inten)
  {
    inten_buf = gegl_buffer_cl_iterator_add_2 (i,
                                               input,
                                               result,
                                               inten_format,
                                               GEGL_CL_BUFFER_READ,
                                               o->mask_radius,
                                               o->mask_radius,
                                               o->mask_radius,
                                               o->mask_radius,
                                               GEGL_ABYSS_CLAMP);
  }

  while (gegl_buffer_cl_iterator_next (i, &err))
    {
      if (err)
        return FALSE;

      err = cl_oilify (i->tex[read],
                       i->tex[0],
/* inten_buf sets to output buffer if use_inten is set to FALSE and won't be used */
                       i->tex[inten_buf],
                       i->size[0],
                       &i->roi[0],
                       o->mask_radius,
                       o->intensities,
                       o->exponent,
                       o->use_inten);

      if (err)
        return FALSE;
    }

  return TRUE;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *aux2,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o    = GEGL_PROPERTIES (operation);
  const Babl *format   = gegl_operation_get_format (operation, "output");
  const Babl *y_format = babl_format_with_space ("Y float", format);

  gint x = o->mask_radius; /* initial x                   */
  gint y = o->mask_radius; /*           and y coordinates */
  gfloat *src_buf;
  gfloat *dst_buf;
  gfloat *out_pixel;

  gfloat *mask_radius_pixel = NULL;
  gfloat *exponent_pixel    = NULL;
  gfloat *mask_radius_buf   = NULL;
  gfloat *exponent_buf      = NULL;
  gfloat *inten_buf         = NULL;

  gint n_pixels = result->width * result->height;
  GeglRectangle src_rect;
  gint total_pixels;

  /* the opencl implementation doesn't (yet) support the parameter buffers */

  if (aux == NULL && aux2 == NULL && gegl_operation_use_opencl (operation))
    if (cl_process (operation, input, output, y_format, result))
      return TRUE;

  src_rect         = *result;
  src_rect.x      -= o->mask_radius;
  src_rect.y      -= o->mask_radius;
  src_rect.width  += o->mask_radius*2;
  src_rect.height += o->mask_radius*2;

  total_pixels = src_rect.width * src_rect.height;

  src_buf = gegl_malloc (4 * total_pixels * sizeof (gfloat));
  dst_buf = gegl_malloc (4 * n_pixels * sizeof (gfloat));

  gegl_buffer_get (input, &src_rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
  clamp_buffer_values (src_buf, 4, total_pixels);

  if (o->use_inten)
    {
      inten_buf = gegl_malloc (total_pixels * sizeof (gfloat));

      gegl_buffer_get (input, &src_rect, 1.0, y_format, inten_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
      clamp_buffer_values (inten_buf, 1, total_pixels);
    }

  if (aux)
    {
      mask_radius_buf   =
      mask_radius_pixel = gegl_malloc (n_pixels * sizeof (gfloat));

      gegl_buffer_get (aux, result, 1.0, y_format, mask_radius_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    }

  if (aux2)
    {
      exponent_buf   =
      exponent_pixel = gegl_malloc (n_pixels * sizeof (gfloat));

      gegl_buffer_get (aux2, result, 1.0, y_format, exponent_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
    }

  out_pixel = dst_buf;

  while (n_pixels--)
    {
      gfloat mask_radius = o->mask_radius;
      gfloat exponent    = o->exponent;

      if (exponent_pixel)
        {
          exponent *= CLAMP(*exponent_pixel, 0.0, 1.0);
          exponent_pixel++;
        }

      if (mask_radius_pixel)
        {
          mask_radius *= CLAMP(*mask_radius_pixel, 0.0, 1.0);
          mask_radius_pixel++;
        }

      if (inten_buf)
        oilify_pixel_inten (x, y, mask_radius, exponent, o->intensities,
                            src_rect.width, src_buf, inten_buf, out_pixel);
      else
        oilify_pixel (x, y, mask_radius, exponent, o->intensities,
                      src_rect.width, src_buf, out_pixel);
      out_pixel += 4;

      /* update x and y coordinates */
      x++;
      if (x >= result->width + o->mask_radius)
        {
          x = o->mask_radius;
          y++;
        }
    }

  gegl_buffer_set (output, result, 0,
                   babl_format_with_space ("RGBA float", format),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  gegl_free (src_buf);
  gegl_free (dst_buf);

  if (inten_buf)
    gegl_free (inten_buf);

  if (mask_radius_buf)
    gegl_free (mask_radius_buf);

  if (exponent_buf)
    gegl_free (exponent_buf);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass          *operation_class;
  GeglOperationComposer3Class *composer3_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer3_class = GEGL_OPERATION_COMPOSER3_CLASS (klass);

  composer3_class->process          = process;
  composer3_class->aux_label        = _("Mask radius buffer");
  composer3_class->aux_description  = _("Per pixel buffer for modulating the "
                                        "mask radius, expecting a scaling "
                                        "factor in range 0.0-1.0");
  composer3_class->aux2_label       = _("Exponent buffer");
  composer3_class->aux2_description = _("Per pixel buffer for modulating the "
                                        "exponent parameter, expecting a "
                                        "scaling factor in range 0.0-1.0");

  operation_class->get_required_for_output = get_required_for_output;
  operation_class->prepare                 = prepare;
  operation_class->threaded                = FALSE;

  gegl_operation_class_set_keys (operation_class,
   "categories" , "artistic",
   "name"       , "gegl:oilify",
   "title",      _("Oilify"),
   "license",     "GPL3+",
   "reference-hash", "8cdf7cedd9f56deb8d09c491ec750527",
   "description",_("Emulate an oil painting"),
   NULL);
}
#endif

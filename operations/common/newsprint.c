/* This file is an image processing operation for GEGL
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
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2017,2019 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_newsprint_pattern)
  enum_value (GEGL_NEWSPRINT_PATTERN_LINE,     "line",     N_("Line"))
  enum_value (GEGL_NEWSPRINT_PATTERN_CIRCLE,   "circle",   N_("Circle"))
  enum_value (GEGL_NEWSPRINT_PATTERN_DIAMOND,  "diamond",  N_("Diamond"))
  enum_value (GEGL_NEWSPRINT_PATTERN_PSCIRCLE, "pssquare", N_("PSSquare (or Euclidian) dot"))
  enum_value (GEGL_NEWSPRINT_PATTERN_CROSS,    "cross",    N_("Crossing Lines"))
enum_end (GeglNewsprintPattern)

enum_start (gegl_newsprint_color_model)
  enum_value (GEGL_NEWSPRINT_COLOR_MODEL_WHITE_ON_BLACK, "white-on-black", N_("White on Black"))
  enum_value (GEGL_NEWSPRINT_COLOR_MODEL_BLACK_ON_WHITE, "black-on-white", N_("Black on White"))
  enum_value (GEGL_NEWSPRINT_COLOR_MODEL_RGB,  "rgb",      N_("RGB"))
  enum_value (GEGL_NEWSPRINT_COLOR_MODEL_CMYK, "cmyk",     N_("CMYK"))
enum_end (GeglNewsprintColorModel)

property_enum (color_model, _("Color Model"),
              GeglNewsprintColorModel, gegl_newsprint_color_model, GEGL_NEWSPRINT_COLOR_MODEL_BLACK_ON_WHITE)
              description (_("How many inks to use just black, rg, rgb (additive), or cmyk"))


property_enum (pattern2, _("Red and cyan pattern"),
               GeglNewsprintPattern, gegl_newsprint_pattern, GEGL_NEWSPRINT_PATTERN_LINE)
               description (_("Halftoning/dot pattern to use"))
               ui_meta ("visible", "color-model {rgb, cmyk}")
               ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                 " color-model {cmyk} : cmyk-label]")
               ui_meta ("rgb-label",  _("Red pattern"))
               ui_meta ("cmyk-label", _("Cyan pattern"))

property_double (period2, _("Red and cyan period"), 12.0)
                 value_range (0.0, 200.0)
                 description (_("The number of pixels across one repetition of a base pattern at base resolution."))
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Red period"))
                 ui_meta ("cmyk-label", _("Cyan period"))

property_double (angle2, _("Red and cyan angle"), 15.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")
                 ui_meta ("direction", "ccw")
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Red angle"))
                 ui_meta ("cmyk-label", _("Cyan angle"))

property_enum (pattern3, _("Green and magenta pattern"),
               GeglNewsprintPattern, gegl_newsprint_pattern, GEGL_NEWSPRINT_PATTERN_LINE)
               description (_("Halftoning/dot pattern to use"))
               ui_meta ("visible", "color-model {rgb, cmyk}")
               ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                 " color-model {cmyk} : cmyk-label]")
               ui_meta ("rgb-label",  _("Green pattern"))
               ui_meta ("cmyk-label", _("Magenta pattern"))

property_double (period3, _("Green and magenta period"), 12.0)
                 value_range (0.0, 200.0)
                 description (_("The number of pixels across one repetition of a base pattern at base resolution."))
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Green period"))
                 ui_meta ("cmyk-label", _("Magenta period"))

property_double (angle3, _("Green and magenta angle"), 45.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")
                 ui_meta ("direction", "ccw")
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Green angle"))
                 ui_meta ("cmyk-label", _("Magenta angle"))

property_enum (pattern4, _("Blue and Yellow pattern"),
               GeglNewsprintPattern, gegl_newsprint_pattern, GEGL_NEWSPRINT_PATTERN_LINE)
               description (_("Halftoning/dot pattern to use"))
               ui_meta ("visible", "color-model {rgb, cmyk}")
               ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                 " color-model {cmyk} : cmyk-label]")
               ui_meta ("rgb-label",  _("Blue pattern"))
               ui_meta ("cmyk-label", _("Yellow pattern"))

property_double (period4, _("Blue and Yellow period"), 12.0)
                 value_range (0.0, 200.0)
                 description (_("The number of pixels across one repetition of a base pattern at base resolution."))
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Blue period"))
                 ui_meta ("cmyk-label", _("Yellow period"))

property_double (angle4, _("Blue and Yellow angle"), 0.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")
                 ui_meta ("direction", "ccw")
                 ui_meta ("visible", "color-model {rgb, cmyk}")
                 ui_meta ("label", "[color-model {rgb}  : rgb-label,"
                                   " color-model {cmyk} : cmyk-label]")
                 ui_meta ("rgb-label",  _("Blue angle"))
                 ui_meta ("cmyk-label", _("Yellow angle"))

property_enum (pattern, _("Black pattern"),
               GeglNewsprintPattern, gegl_newsprint_pattern, GEGL_NEWSPRINT_PATTERN_LINE)
               description (_("Halftoning/dot pattern to use"))
               ui_meta ("visible", "color-model {white-on-black, black-on-white, cmyk}")
               ui_meta ("label", "[color-model {white-on-black,"
                                 "              black-on-white} : bw-label,"
                                 " color-model {cmyk}           : cmyk-label]")
               ui_meta ("bw-label",   _("Pattern"))
               ui_meta ("cmyk-label", _("Black pattern"))


property_double (period, _("Black period"), 12.0)
                 value_range (0.0, 200.0)
                 description (_("The number of pixels across one repetition of a base pattern at base resolution."))
                 ui_meta ("visible", "color-model {white-on-black, black-on-white, cmyk}")
                 ui_meta ("label", "[color-model {white-on-black,"
                                   "              black-on-white} : bw-label,"
                                   " color-model {cmyk}           : cmyk-label]")
                 ui_meta ("bw-label",   _("Period"))
                 ui_meta ("cmyk-label", _("Black period"))

property_double (angle, _("Black angle"), 75.0)
                 value_range (-180.0, 180.0)
                 ui_meta ("unit", "degree")
                 ui_meta ("direction", "ccw")
                 description (_("Angle offset for patterns"))
                 ui_meta ("visible", "color-model {white-on-black, black-on-white, cmyk}")
                 ui_meta ("label", "[color-model {white-on-black,"
                                   "              black-on-white} : bw-label,"
                                   " color-model {cmyk}           : cmyk-label]")
                 ui_meta ("bw-label",   _("Angle"))
                 ui_meta ("cmyk-label", _("Black angle"))

property_double (black_pullout, _("Black pullout"), 1.0)
                 value_range (0.0, 1.0)
                 description (_("How much of common gray to pull out of CMY"))
                 ui_meta ("visible", "color-model {cmyk}")

property_int    (aa_samples, _("Anti-alias oversampling factor"), 16)
                 value_range (1, 128)
                 description (_("Number of samples that are averaged for antialiasing the result."))

property_double (turbulence, _("Turbulence"), 0.0)
                 value_range (0.0, 1.0)  // rename to wave-pinch or period-pinch?
                 description (_("Color saturation dependent compression of period"))

property_double (blocksize, _("Blocksize"), -1.0)
                 value_range (-1.0, 64.0)
                 description (_("Number of periods per tile, this tiling avoids high frequency anomaly that angle boost causes"))

property_double (angleboost, _("Angle Boost"), 0.0)
                 value_range (0.0, 4.0)
                 description (_("Multiplication factor for desired rotation of the local space for texture, the way this is computed makes it weak for desaturated colors and possibly stronger where there is color."))


#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     newsprint
#define GEGL_OP_C_SOURCE newsprint.c

#include "gegl-op.h"

/* for details and more liberal licensing of the following function, see
   https://pippin.gimp.org/spachrotyzer/ */

static
float spachrotyze (
    float x,
    float y,
    float part_white,
    float offset,
    float hue,
    int   pattern,
    float period,
    float turbulence,
    float blocksize,
    float angleboost,
    float angle,
    int   max_aa_samples)
{
  float acc = 0.0f;

  float twist = 3.1415f / 2.f - ((hue * angleboost) + angle);

  float width  = (period * (1.0f - turbulence) +
                 (period * offset) * turbulence);

  float vec0 = cosf (twist);
  float vec1 = sinf (twist);

  float xi = 0.5f;
  float yi = 0.2f;
  int count = 0;
  int in = 0;
  float old_acc = 0.0f;

  x += period * 2.f;
  y += period * 2.f;

  for (int i = 0; i < max_aa_samples ; i++)
  {
    xi = fmodf (xi + 0.618033988749854f, 1.0f);
    yi = fmodf (yi + (0.618033988749854f/1.61235f), 1.0f);

    old_acc = acc;
    {
      float u = fmodf (x + xi - 0.5f * width, blocksize * width);
      float v = fmodf (y + yi - 0.5f * width, blocksize * width);

      float w = vec0 * u + vec1 * v;
      float q = vec1 * u - vec0 * v;

      float wperiod = fmodf (w, width);
      float wphase  = (wperiod / width) * 2.0f - 1.0f;

      float qperiod = fmodf (q, width);
      float qphase  = (qperiod / width) * 2.0f - 1.0f;

      if (pattern == 0)       /* line */
      {
        if (fabsf (wphase) < part_white)
          in ++;
      }
      else if (pattern == 1) /* dot */
      {
        if (qphase * qphase + wphase * wphase <
          part_white * part_white * 2.0f)
          in ++;
      }
      else if (pattern == 2) /* diamond */
      {
        if ((fabsf (wphase) + fabsf (qphase))/2.0f < part_white )
          in++;
      }
      else if (pattern == 3) /* dot-to-diamond-to-dot */
      {
          float ax = fabsf (wphase ) ;
          float ay = fabsf (qphase ) ;
          float v = 0.0;

          if  (ax + ay > 1.0f)
          {
            ay = 1.0f - ay;
            ax = 1.0f - ax;
            v = 2.0f - hypotf (ax, ay);
          }
          else
          {
            v = hypotf (ax, ay);
          }
          v/=2.0f;
          if (v < part_white)
            in++;
      }
      else if (pattern == 4) /* cross */
      {
        float part_white2 = part_white * part_white;
        if (fabsf (wphase) < part_white2 ||
            fabsf (qphase) < part_white2)
          in++;
      }
      count ++;

      acc = in * 1.0f / count;
      if (i > 3 &&
          fabsf (acc - old_acc) < 0.23f)
        break;
      old_acc = acc;
    }
  }
  return acc;
}

static inline double degrees_to_radians (double degrees)
{
  return degrees * (2 * G_PI / 360.0);
}

#if 0
-- threshold filter

level = 0.5

for y=0, height-1 do
	for x=0, width-1 do
		ax = (x / width) * 2 - 1
		ay = (y / height) * 2 - 1
		if ax < 0 then ax = -ax end
		if ay < 0 then ay = -ay end
		v = ax * ay

		if false then
		if ax + ay > 1.0 then
		  ax = 1.0 - ax
		  ay = 1.0 - ay
	  --  v = (2.0 - math.sqrt((1.0-ay) * (1.0-ay) + (1.0-ax) * (1.0-ax)))/2
          v = (math.sqrt(ax * ax + ay * ay)/2)

		else
          v = 1.0-math.sqrt(ax * ax + ay * ay)/2
		end
		end
		set_value (x,y,v)
	end
	progress (y/height)
end

#endif


static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gfloat     *in_pixel = in_buf;
  gfloat     *out_pixel = out_buf;

  gint        x = roi->x; /* initial x         */
  gint        y = roi->y; /* and y coordinates */
  gfloat blocksize = o->blocksize;
  if (blocksize < 0.0)
    blocksize = 819200.0;

  switch (o->color_model)
  {
    case GEGL_NEWSPRINT_COLOR_MODEL_WHITE_ON_BLACK:
       while (n_pixels--)
         {
           float luminance  = in_pixel[1];
           float chroma     = fabsf (in_pixel[0]-in_pixel[1]);
           float angle      = fabsf (in_pixel[2]-in_pixel[1]);

           float gray = spachrotyze (x, y,
                                    luminance, chroma, angle,
                                    o->pattern,
                                    o->period / (1.0*(1<<level)),
                                    o->turbulence,
                                    blocksize,
                                    o->angleboost,
                                    degrees_to_radians (o->angle),
                                    o->aa_samples);

           for (int c = 0; c < 3; c++)
             out_pixel[c] = gray;
           out_pixel[3] = in_pixel[3];

           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
    case GEGL_NEWSPRINT_COLOR_MODEL_BLACK_ON_WHITE:
       while (n_pixels--)
         {
           float luminance  = in_pixel[1];
           float chroma     = fabsf (in_pixel[0]-in_pixel[1]);
           float angle      = fabsf (in_pixel[2]-in_pixel[1]);

           float gray = 1.0-spachrotyze (x, y,
                                    1.0-luminance, chroma, angle,
                                    o->pattern,
                                    o->period / (1.0*(1<<level)),
                                    o->turbulence,
                                    blocksize,
                                    o->angleboost,
                                    degrees_to_radians (o->angle),
                                    o->aa_samples);

           for (int c = 0; c < 3; c++)
             out_pixel[c] = gray;
           out_pixel[3] = in_pixel[3];

           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
    case GEGL_NEWSPRINT_COLOR_MODEL_RGB:
       while (n_pixels--)
         {
           float pinch = fabsf (in_pixel[0]-in_pixel[1]);
           float angle = fabsf (in_pixel[2]-in_pixel[1]);

           out_pixel[0] = spachrotyze (x, y,
                                       in_pixel[0], pinch, angle,
                                       o->pattern2,
                                       o->period2 / (1.0*(1<<level)),
                                       o->turbulence,
                                       blocksize,
                                       o->angleboost,
                                       degrees_to_radians (o->angle2),
                                       o->aa_samples);

           out_pixel[1] = spachrotyze (x, y,
                                       in_pixel[1], pinch, angle,
                                       o->pattern3,
                                       o->period3 / (1.0*(1<<level)),
                                       o->turbulence,
                                       blocksize,
                                       o->angleboost,
                                       degrees_to_radians (o->angle3),
                                       o->aa_samples);

           out_pixel[2] = spachrotyze (x, y,
                                   in_pixel[2], pinch, angle,
                                   o->pattern4,
                                   o->period4 / (1.0*(1<<level)),
                                   o->turbulence,
                                   blocksize,
                                   o->angleboost,
                                   degrees_to_radians (o->angle4),
                                   o->aa_samples);
           out_pixel[3] = in_pixel[3];

           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
    case GEGL_NEWSPRINT_COLOR_MODEL_CMYK:
       while (n_pixels--)
         {
           float pinch = fabsf (in_pixel[0]-in_pixel[1]);
           float angle = fabsf (in_pixel[2]-in_pixel[1]);
           float c = 1.0  - in_pixel[0];
           float m = 1.0  - in_pixel[1];
           float iy = 1.0 - in_pixel[2];
           float k = 1.0;

           if (c < k) k = c;
           if (m < k) k = m;
           if (y < k) k = y;

           k = k * o->black_pullout; /* black pull out */

           if (k < 1.0)
           {
             c = (c - k) / (1.0 - k);
             m = (m - k) / (1.0 - k);
             iy = (iy - k) / (1.0 - k);
           }
           else /* wont happen with 0.40 pullout */
           {
             c = m = iy = 1.0;
           }

           c = spachrotyze (x, y,
                            c, pinch, angle,
                            o->pattern2,
                            o->period2 / (1.0*(1<<level)),
                            o->turbulence,
                            blocksize,
                            o->angleboost,
                            degrees_to_radians (o->angle2),
                            o->aa_samples);

           m = spachrotyze (x, y,
                            m, pinch, angle,
                            o->pattern3,
                            o->period3 / (1.0*(1<<level)),
                            o->turbulence,
                            blocksize,
                            o->angleboost,
                            degrees_to_radians (o->angle3),
                            o->aa_samples);

           iy = spachrotyze (x, y,
                             iy, pinch, angle,
                             o->pattern4,
                             o->period4 / (1.0*(1<<level)),
                             o->turbulence,
                             blocksize,
                             o->angleboost,
                             degrees_to_radians (o->angle4),
                             o->aa_samples);

           k = spachrotyze (x, y,
                            k, pinch, angle,
                            o->pattern,
                            o->period / (1.0*(1<<level)),
                            o->turbulence,
                            blocksize,
                            o->angleboost,
                            degrees_to_radians (o->angle),
                            o->aa_samples);

           if (k < 1.0) {
             c = c * (1.0 - k) + k;
             m = m * (1.0 - k) + k;
             iy= iy* (1.0 - k) + k;
           } else {
             c = 1.0; iy = 1.0; m = 1.0;
           }
           out_pixel[0] = 1.0 - c;
           out_pixel[1] = 1.0 - m;
           out_pixel[2] = 1.0 - iy;
           out_pixel[3] = in_pixel[3];
           out_pixel += 4;
           in_pixel  += 4;

           /* update x and y coordinates */
           x++; if (x>=roi->x + roi->width) { x=roi->x; y++; }
         }
       break;
  }

  return  TRUE;
}

#if 0
#include "opencl/gegl-cl.h"
#include "opencl/spachrotyze.cl.h"

static GeglClRunData *cl_data = NULL;

/* OpenCL processing function */
static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            int                  level)
{
  const size_t  gbl_size[2] = {roi->width, roi->height};
  const size_t  gbl_offs[2] = {roi->x, roi->y};

  GeglProperties   *o  = GEGL_PROPERTIES (op);
  cl_int cl_err = 0;

  if (!cl_data)
    {
      const char *kernel_name[] = {"gegl_spachrotyzer", NULL};
      cl_data = gegl_cl_compile_and_build (spachrotyzer_cl_source, kernel_name);
    }

  if (!cl_data)
  {
    g_warning ("cl fail\n");
    CL_CHECK;
    return 1;
  }

  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 1, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 2, sizeof(cl_int),   (void*)&o->pattern);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 3, sizeof(cl_float), (void*)&o->period);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 4, sizeof(cl_float), (void*)&o->turbulence);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 5, sizeof(cl_float), (void*)&o->blocksize);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 6, sizeof(cl_float), (void*)&o->angleboost);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[0], 7, sizeof(cl_float), (void*)&o->angle);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[0], 2,
                                        gbl_offs, gbl_size, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}
#endif

static void prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "input", babl_format_with_space ("R~G~B~A float", space));
  gegl_operation_set_format (operation, "output", babl_format_with_space ("RGBA float", space));
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;
  gchar                         *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:newsprint'>"
    "      <params>"
    "        <param name='period'>6.0</param>"
    "        <param name='pattern'>pssquare</param>"
    "        <param name='color-model'>cmyk</param>"
    "      </params>"
    "    </node>"
    "    <node operation='gegl:load' path='standard-input.png'/>"
    "  </node>"
    "  <node operation='gegl:checkerboard'>"
    "    <params>"
    "      <param name='color1'>rgb(0.25,0.25,0.25)</param>"
    "      <param name='color2'>rgb(0.75,0.75,0.75)</param>"
    "    </params>"
    "  </node>"    
    "</gegl>";


  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  point_filter_class->process = process;
  operation_class->threaded = TRUE;
#if 0
  point_filter_class->cl_process = cl_process;
  operation_class->opencl_support = TRUE;
#endif

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:newsprint",
    "title",              _("Newsprint"),
    "categories" ,        "render",
    "position-dependent", "true",
    "reference-hash",     "2792e6d5aec5c2f80e45aea3a610d1f6",
    "reference-composition", composition,
    "description",        _("Digital halftoning with optional modulations. "),
    NULL);
}

#endif

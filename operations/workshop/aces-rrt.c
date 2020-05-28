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
 * Copyright 2018 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

   /* no properties */

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     aces_rrt
#define GEGL_OP_C_SOURCE aces-rrt.c

#include "gegl-op.h"

static inline float aces_rrt (float x)
{
 /* source of approximation:

    https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
    https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
  */
 float a = x * (x + 0.0245786f) - 0.000090537f;
 float b = x * (0.983729f * x + 0.4329510f) + 0.238081f;
 return a / b;
}

#define lerp(a,b,d)   ((a) * (1.0-d) + (b) * ((d)))

static inline void aces_rrt_rgb (float rin, float gin, float bin, float *rout, float *gout, float *bout)
{
  float r, g, b;

  r = aces_rrt (rin);
  g = aces_rrt (gin);
  b = aces_rrt (bin);
  r = (rin);
  g = (gin);
  b = (bin);

#if 0
   /* this is not the proper glow + desaturate used by ACES RRT,
      but does something similar 
    */

#define THRESHOLD     0.05
#define THRESHOLD2    0.4


  {
  float gray;

  gray = (r+g+b) / 3.0;
  if (gray > 1.0f) gray = 1.0f;

  if (gray < THRESHOLD)
  {
    float saturation = 1.0-(gray)/THRESHOLD;
    r = lerp (r, gray, saturation);
    g = lerp (g, gray, saturation);
    b = lerp (b, gray, saturation);
  }
  else if (gray > THRESHOLD2)
  {
    float saturation = (gray - THRESHOLD2)/(1.0-THRESHOLD2);
    r = lerp (r, gray, saturation);
    g = lerp (g, gray, saturation);
    b = lerp (b, gray, saturation);
  }
  }
#endif

  *rout = r;
  *gout = g;
  *bout = b;
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  gfloat *in  = in_buf;
  gfloat *out = out_buf;

  while (samples--)
    {
      aces_rrt_rgb (in[0], in[1], in[2],
                  &out[0], &out[1], &out[2]);
      out[3] = in[3];

      in += 4;
      out+= 4;
    }
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process  = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:aces-rrt",
    "title",       _("ACES RRT"),
    "categories" , "color:tonemapping",
    "description",
       _("HDR to SDR proofing filter/mapping curve that is an approximation of the ACES RRT (Reference Rendering Transform). When feeding scene-refereed imagery into this op, the result is suitable for display referred transform to sRGB or output display using regular ICC matric profiles as the ODT. Note that for the time being, this is a luminance only approximation of the ACES RRT; without desaturation of highlights and shadows nor red hue modifications."),
    NULL);
}

#endif

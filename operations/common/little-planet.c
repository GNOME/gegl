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
 * Copyright 2014, 2018 Øyvind Kolås <pippin@gimp.org>
 */

#ifdef GEGL_PROPERTIES

property_double (pan, _("Pan"), 0.0)
  description   (_("Horizontal camera panning"))
  value_range   (-360.0, 360.0)
  ui_meta       ("unit", "degree")
  ui_meta       ("direction", "cw")

property_double (tilt, _("Tilt"), 90.0)
  description   (_("Vertical camera panning"))
  value_range   (-180.0, 180.0)
  ui_range      (-180.0, 180.0)
  ui_meta       ("unit", "degree")
  ui_meta       ("direction", "cw")

property_double (spin, _("Spin"), 0.0)
  description   (_("Spin angle around camera axis"))
  value_range   (-360.0, 360.0)
  ui_meta       ("direction", "cw")

property_double (zoom, _("Zoom"), 100.0)
  description   (_("Zoom level"))
  value_range   (0.01, 1000.0)

property_int    (width, _("Width"), -1)
  description   (_("output/rendering width in pixels, -1 for input width"))
  value_range   (-1, 10000)
  ui_meta       ("role", "output-extent")
  ui_meta       ("axis", "x")

property_int    (height, _("Height"), -1)
  description   (_("output/rendering height in pixels, -1 for input height"))
  value_range   (-1, 10000)
  ui_meta       ("role", "output-extent")
  ui_meta       ("axis", "y")

property_boolean(inverse, _("Inverse transform"), FALSE)
  description   (_("Do the inverse mapping, useful for touching up zenith, nadir or other parts of panorama."))

property_enum   (sampler_type, _("Resampling method"),
                  GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_NEAREST)
  description   (_("Image resampling method to use, for good results with double resampling when retouching panoramas, use nearest to generate the view and cubic or better for the inverse transform back to panorama."))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     little_planet
#define GEGL_OP_C_SOURCE little-planet.c

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-op.h"

typedef struct _Transform Transform;
struct _Transform
{
  float pan;
  float tilt;
  float sin_tilt;
  float cos_tilt;
  float sin_spin;
  float cos_spin;
  float sin_negspin;
  float cos_negspin;
  float zoom;
  float spin;
  float xoffset;
  float width;
  float height;
  float in_width;
  float in_height;
  void (*mapfun) (Transform *transform, float x, float  y, float *lon, float *lat);
  int   reverse;
  int   do_spin;
  int   do_zoom;
};

static void inline
stereographic_ll2xy (Transform *transform,
                     float lon, float lat,
                     float *x,  float *y)
{
  float k, sin_lat, cos_lat, cos_lon_minus_pan;

  lat = lat * M_PI - M_PI/2;
  lon = lon * (M_PI * 2);

  sin_lat = sinf (lat);
  cos_lat = cosf (lat);
  cos_lon_minus_pan = cosf (lon - transform->pan);

  k = 2/(1 + transform->sin_tilt * sin_lat +
             transform->cos_tilt * cos_lat *
             cos_lon_minus_pan);

  *x = k * ((cos_lat * sin (lon - transform->pan)));
  *y = k * ((transform->cos_tilt * sin_lat -
        transform->sin_tilt * cos_lat * cos_lon_minus_pan));

  if (transform->do_zoom)
  {
    *x *= transform->zoom;
    *y *= transform->zoom;
  }

  if (transform->do_spin)
  {
    float tx = *x, ty = *y;
    *x = tx * transform->cos_negspin - ty * transform->sin_negspin;
    *y = ty * transform->cos_negspin + tx * transform->sin_negspin;
  }

  *x += transform->xoffset;
  *y += 0.5f;
}

static void inline
stereographic_xy2ll (Transform *transform,
                     float x, float y,
                     float *lon, float *lat)
{
  float p, c;
  float longtitude, latitude;
  float sin_c, cos_c;

  y -= 0.5f;
  x -= transform->xoffset;

  if (transform->do_spin)
  {
    float tx = x, ty = y;
    x = tx * transform->cos_spin - ty * transform->sin_spin;
    y = ty * transform->cos_spin + tx * transform->sin_spin;
  }

  if (transform->do_zoom)
  {
    x /= transform->zoom;
    y /= transform->zoom;
  }

  p = sqrtf (x*x+y*y);
  c = 2 * atan2f (p / 2, 1);

  sin_c = sinf (c);
  cos_c = cosf (c);

  latitude = asinf (cos_c * transform->sin_tilt + ( y * sin_c * transform->cos_tilt) / p);
  longtitude = transform->pan + atan2f ( x * sin_c, p * transform->cos_tilt * cos_c - y * transform->sin_tilt * sin_c);

  if (longtitude < 0)
    longtitude += M_PI * 2;

  *lon = (longtitude / (M_PI * 2));
  *lat = ((latitude + M_PI/2) / M_PI);
}

static void prepare_transform (Transform *transform,
                               float pan, float spin, float zoom, float tilt,
                               float width, float height,
                               float input_width, float input_height,
                               int inverse)
{
  float xoffset = 0.5f;
  transform->reverse = inverse;

  if (inverse)
    transform->mapfun = stereographic_ll2xy;
  else
    transform->mapfun = stereographic_xy2ll;

  pan  = pan / 360 * M_PI * 2;
  spin = spin / 360 * M_PI * 2;
  zoom = zoom / 1000.0f;
  tilt = tilt / 360 * M_PI * 2;

  while (pan > M_PI)
    pan -= 2 * M_PI;

  if (width <= 0 || height <= 0)
  {
    width = input_height;
    height = width;
    xoffset = ((input_width - height)/height) / 2 + 0.5f;
  }
  else
  {
    float orig_width = width;
    width = height;
    xoffset = ((orig_width - height)/height) / 2 + 0.5f;
  }

  transform->do_spin = fabsf (spin) > 0.000001 ? 1 : 0;
  transform->do_zoom = fabsf (zoom-1.0f) > 0.000001 ? 1 : 0;

  transform->pan         = pan;
  transform->tilt        = tilt;
  transform->spin        = spin;
  transform->zoom        = zoom;
  transform->xoffset     = xoffset;
  transform->sin_tilt    = sinf (tilt);
  transform->cos_tilt    = cosf (tilt);
  transform->sin_spin    = sinf (spin);
  transform->cos_spin    = cosf (spin);
  transform->sin_negspin = sinf (-spin);
  transform->cos_negspin = cosf (-spin);
  transform->width       = width;
  transform->height      = height;
  transform->in_width    = input_width;
  transform->in_height   = input_height;

  if (inverse)
  {
    float tmp;
#define swap(a,b) tmp = a; a = b; b= tmp;
    swap(transform->width, transform->in_width);
    swap(transform->height, transform->in_height);
#undef swap
  }
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format;
  if (o->sampler_type == GEGL_SAMPLER_NEAREST)
    format = babl_format_with_space ("RGBA float", space);
  else
    format = babl_format_with_space ("RaGaBaA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = {0,0,0,0};

  if (o->width <= 0 || o->height <= 0)
  {
     GeglRectangle *in_rect;
     in_rect = gegl_operation_source_get_bounding_box (operation, "input");
     if (in_rect)
       {
          result = *in_rect;
       }
     else
     {
       result.width = 320;
       result.height = 200;
     }
  }
  else
  {
    result.width = o->width;
    result.height = o->height;
  }
  return result;
}

static void prepare_transform2 (Transform *transform,
                                GeglOperation *operation,
                                gint level)
{
  gint factor = 1 << level;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle in_rect = *gegl_operation_source_get_bounding_box (operation, "input");

  prepare_transform (transform,
                     o->pan, o->spin, o->zoom, o->tilt,
                     o->width / factor, o->height / factor,
                     in_rect.width, in_rect.height,
                     o->inverse);

}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result = *region;

  const GeglRectangle *in_rect =
      gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      result = *in_rect;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Transform           transform;
  GeglSampler        *sampler;
  gint                factor       = 1 << level;
  GeglBufferIterator *it;
  GeglBufferMatrix2  scale_matrix;
  GeglBufferMatrix2  *scale        = NULL;
  gint                sampler_type = o->sampler_type;
  const Babl         *format_io    = gegl_operation_get_format (operation, "output");
  GeglSamplerGetFun   getfun;

  level = 0;
  factor = 1;
  prepare_transform2 (&transform, operation, level);

  if (level)
    sampler_type = GEGL_SAMPLER_NEAREST;

  if (transform.reverse)
  {
    /* hack to avoid artifacts have been observed with these samplers */
    sampler_type = GEGL_SAMPLER_NEAREST;
  }

  if (sampler_type != GEGL_SAMPLER_NEAREST && abs(o->tilt < 33))
    /* skip the computation of sampler neighborhood scale matrix in cases where
     * we are unlikely to be scaling down */
    scale = &scale_matrix;

  sampler = gegl_buffer_sampler_new_at_level (input, format_io, sampler_type, 0);
  getfun = gegl_sampler_get_fun (sampler);

  {
    float   ud = ((1.0f/transform.width)*factor);
    float   vd = ((1.0f/transform.height)*factor);
    int abyss_mode = transform.reverse ? GEGL_ABYSS_NONE : GEGL_ABYSS_LOOP;

    it = gegl_buffer_iterator_new (output, result, level, format_io,
                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 1);

    while (gegl_buffer_iterator_next (it))
      {
        gint i;
        gint n_pixels = it->length;
        gint x = it->items[0].roi.width; /* initial x                   */

        float   u0 = (((it->items[0].roi.x*factor * 1.0f)/transform.width));
        float   u, v;

        float *out = it->items[0].data;

        u = u0;
        v = ((it->items[0].roi.y*factor * 1.0/transform.height));

        if (scale)
          {
            for (i=0; i<n_pixels; i++)
              {
                float cx, cy;
/* we need our own jacobian matrix approximator,
 * since we do not operate on pixel values
 */
#define gegl_sampler_compute_scale2(matrix, x, y)        \
{                                                        \
  float ax, ay, bx, by;                                  \
  gegl_unmap(x + 0.5 * ud, y, ax, ay);                   \
  gegl_unmap(x - 0.5 * ud, y, bx, by);                   \
  matrix.coeff[0][0] = (ax - bx);   \
  matrix.coeff[1][0] = (ay - by);  \
  gegl_unmap(x, y + 0.5 * ud, ax, ay);                   \
  gegl_unmap(x, y - 0.5 * ud, bx, by);                   \
  matrix.coeff[0][1] = (ax - bx);   \
  matrix.coeff[1][1] = (ay - by);  \
}

#define gegl_unmap(xx,yy,ud,vd) {                                  \
                  float rx, ry;                                    \
                  transform.mapfun (&transform, xx, yy, &rx, &ry); \
                  ud = rx;vd = ry;}
                gegl_sampler_compute_scale2 (scale_matrix, u, v);
                gegl_unmap(u,v, cx, cy);
#undef gegl_unmap

#if 1
                if (scale_matrix.coeff[0][0] > 0.5f)
                  scale_matrix.coeff[0][0] = (scale_matrix.coeff[0][0]-1.0) * transform.in_width;
                else if (scale_matrix.coeff[0][0] < -0.5f) 
                  scale_matrix.coeff[0][0] = (scale_matrix.coeff[0][0]+1.0) * transform.in_width;
                else
                  scale_matrix.coeff[0][0] *= transform.in_width;

                if (scale_matrix.coeff[0][1] > 0.5f)
                  scale_matrix.coeff[0][1] = (scale_matrix.coeff[0][1]-1.0) * transform.in_width;
                else if (scale_matrix.coeff[0][1] < -0.5f)
                  scale_matrix.coeff[0][1] = (scale_matrix.coeff[0][1]+1.0) * transform.in_width;
                else
                  scale_matrix.coeff[0][1] *= transform.in_width;

                scale_matrix.coeff[1][1] *= transform.in_height;
                scale_matrix.coeff[1][0] *= transform.in_height;
#endif
                getfun (sampler,
                        cx * transform.in_width + 0.5f,
                        cy * transform.in_height + 0.5f,
                        scale, out, abyss_mode);

                out += 4;

                /* update x, y and u,v coordinates */
                x--;
                u+=ud;
                if (x == 0)
                  {
                    x = it->items[0].roi.width;
                    u = u0;
                    v += vd;
                  }
              }
          }
        else
          {
            for (i=0; i<n_pixels; i++)
              {
                float cx, cy;
                transform.mapfun (&transform, u, v, &cx, &cy);
                getfun (sampler,
                        cx * transform.in_width + 0.5f,
                        cy * transform.in_height + 0.5f,
                        scale, out, abyss_mode);
                out += 4;

                /* update x, y and u,v coordinates */
                x--;
                u+=ud;
                if (x <= 0)
                  {
                    x = it->items[0].roi.width;
                    u = u0;
                    v += vd;
                  }
              }
          }
      }
  }

  g_object_unref (sampler);
  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static gchar *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:stereographic-projection' width='200' height='200'/>"
    "<node operation='gegl:load'>"
    "  <params>"
    "    <param name='path'>standard-panorama.png</param>"
    "  </params>"
    "</node>"
    "</gegl>";

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationFilterClass *filter_class;
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class  = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->process                 = operation_process;
  operation_class->threaded                = TRUE;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
    "name",                  "gegl:stereographic-projection",
    "compat-name",           "gegl:little-planet",
    "title",                 _("Little Planet"),
    "position-dependent",    "true",
    "categories" ,           "map",
    "reference-hash",        "43e6da04bdcebdbb9270f3d798444d08",
    "reference-composition", composition,
    "description", _("Do a stereographic/little planet transform of an equirectangular image."),
    NULL);
}
#endif

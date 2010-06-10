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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES


gegl_chant_path   (d,        _("Vector"),
                             _("A GeglVector representing the path of the stroke"))
gegl_chant_string (transform,_("Transform"), "",
                             _("svg style description of transform."))

/* Brush shape parameters - FIXME : those should not be in the stroke operation
 * stroking is about where to put dabs of paint, not modifying the brush
 * use a brush dab op instead
 */
gegl_chant_double (scale,    _("Scale Factor"),  0.0, 10.0, 2.0,
                             _("Brush Scale Factor"))
gegl_chant_double (hardness, _("Hardness"),   0.0, 1.0, 0.6,
                             _("Brush Hardness, 0.0 for soft 1.0 for hard."))
gegl_chant_double (angle,    _("Angle"),   0.0, 360.0, 0.0,
                             _("Brush Angle."))
gegl_chant_double (aspect,   _("Aspect Ratio"),   0.1, 10.0, 1.0,
                             _("Brush Aspect, 0.1 for pancake 10.0 for spike."))
gegl_chant_double (force,    _("Force"),   0.0, 1.0, 0.6,
                             _("Brush Force."))

/* Stroke shape parameters */
gegl_chant_double (spacing,  _("Spacing"),   0.01, 200, 0.6,
                             _("Stroke Spacing."))
gegl_chant_double (jitter,   _("Jitter"),   0.0, 1.0, 0.6,
                             _("Stroke Jitter"))

gegl_chant_color  (color,    _("Color"),      "rgba(0.0,0.0,0.0,0.0)",
                             _("Color of paint to use for stroking."))
gegl_chant_double (opacity,  _("Opacity"),  -2.0, 2.0, 1.0,
                             _("Stroke Opacity."))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE "brush-stroke.c"

#include "gegl-plugin.h"
#include "gegl-buffer-private.h"

/* the path api isn't public yet */
#include "property-types/gegl-path.h"
static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata);

#include "gegl-chant.h"
#include <stdio.h>


typedef struct StampStatic {
  gboolean  valid;
  Babl     *format;
  gfloat   *buf;
  gdouble   radius;
}StampStatic;

static void gegl_path_stamp (GeglBuffer *buffer,
                             const GeglRectangle *clip_rect,
                             gdouble     x,
                             gdouble     y,
                             gdouble     radius,
                             gdouble     hardness,
                             GeglColor  *color,
                             gdouble     opacity)
{
  gfloat col[4];
  static StampStatic s = {FALSE,}; /* XXX: 
                                      we will ultimately leak the last valid
                                      cached brush. */

  GeglRectangle temp;
  GeglRectangle roi;

  roi.x = floor(x-radius);
  roi.y = floor(y-radius);
  roi.width = ceil (x+radius) - floor (x-radius);
  roi.height = ceil (y+radius) - floor (y-radius);

  gegl_color_get_rgba4f (color, col);

  /* bail out if we wouldn't leave a mark on the buffer */
  if (!gegl_rectangle_intersect (&temp, &roi, clip_rect))
    {
      return;
    }

  if (s.format == NULL)
    s.format = babl_format ("RaGaBaA float");

  if (s.buf == NULL ||
      s.radius != radius)
    {
      if (s.buf != NULL)
        g_free (s.buf);
      /* allocate a little bit more, just in case due to rounding errors and
       * such */
      s.buf = g_malloc (4*4* (roi.width + 2 ) * (roi.height + 2));
      s.radius = radius;
      s.valid = TRUE;  
    }
  g_assert (s.buf);

  gegl_buffer_get_unlocked (buffer, 1.0, &roi, s.format, s.buf, 0);

  {
    gint u, v;
    gint i=0;

    gfloat radius_squared = radius * radius;
    gfloat inner_radius_squared = (radius * hardness)*(radius * hardness);
    gfloat soft_range = radius_squared - inner_radius_squared;

    for (v= roi.y; v < roi.y + roi.height ; v++)
    {
      gfloat vy2 = (v-y)*(v-y);
      for (u= roi.x; u < roi.x + roi.width; u++)
        {
          gfloat o = (u-x) * (u-x) + vy2;

          if (o < inner_radius_squared)
             o = col[3];
          else if (o < radius_squared)
            {
              o = (1.0 - (o-inner_radius_squared) / (soft_range)) * col[3];
            }
          else
            {
              o=0.0;
            }
         if (o!=0.0)
           {
             gint c;
             o = o*opacity;
             for (c=0;c<4;c++)
               s.buf[i*4+c] = (s.buf[i*4+c] * (1.0-o) + col[c] * o);
           }
         i++;
        }
    }
  }
  gegl_buffer_set_unlocked (buffer, &roi, s.format, s.buf, 0);
}

static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata)
{
  GeglRectangle rect = *roi;
  GeglChantO    *o   = GEGL_CHANT_PROPERTIES (userdata);
  gfloat stroke_width = o->scale*5.0*2.0;
  /* invalidate the incoming rectangle */
  
  rect.x -= stroke_width/2.;
  rect.y -= stroke_width/2;
  rect.width += stroke_width;
  rect.height += stroke_width;

  gegl_operation_invalidate (userdata, &rect, FALSE);
};

static void
prepare (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "input", babl_format ("RaGaBaA float"));
  /*
  gegl_operation_set_format (operation, "aux", babl_format ("Y float"));
  */
  if (o->transform && o->transform[0] != '\0')
    {
      GeglMatrix3 matrix;
      gegl_matrix3_parse_string (matrix, o->transform);
      gegl_path_set_matrix (o->d, matrix);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;
  gdouble        x0, x1, y0, y1;
  gfloat stroke_width = o->scale*5.0;

  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");

  gegl_path_get_bounds (o->d, &x0, &x1, &y0, &y1);
  defined.x      = x0 - stroke_width/2;
  defined.y      = y0 - stroke_width/2;
  defined.width  = x1 - x0 + stroke_width;
  defined.height = y1 - y0 + stroke_width;

  if (in_rect)
    {
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}

#if 0
static gboolean gegl_path_is_closed (GeglPath *path)
{
  const GeglPathItem *knot;

  if (!path)
    return FALSE;
  knot = gegl_path_get_node (path, -1);
  if (!knot)
    return FALSE;
  if (knot->type == 'z')
    {
      return TRUE;
    }
  return FALSE;
}
#endif


#if 0
static GeglRectangle
get_cached_region (GeglOperation *operation)
{
  return get_bounding_box (operation);
}
#endif

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o  = GEGL_CHANT_PROPERTIES (operation);
  GeglPath *path = o->d;

  gfloat spacing, length, radius, hardness, opacity;
  gdouble pos = 0;
  gboolean incremental = TRUE;

  if (!o->d)
    return TRUE;

  if (input)
    {
      gegl_buffer_copy (input, result, output, result);
    }
  else
    {
      gegl_buffer_clear (output, result);
    }

    radius = 5 * o->scale;
    hardness = o->hardness;

    length  = gegl_path_get_length(path);

    
    if (gegl_buffer_is_shared (output))
      while (!gegl_buffer_try_lock (output));

    do
    {
      gdouble x, y;

      /* Set the brush tip : set brush worker's pos */
      if (! gegl_path_calc (path,pos,&x,&y)) break;
      /* Compute coordinates : jitter ? */

      if (incremental)
      {
          /* Blend into output : set blend worker pos and process */
          gegl_path_stamp (output, result,
                x, y, radius, hardness, o->color, o->opacity);
      }
      else
      {
          /* Blend into a mask */
      }

      /* Calculate next position */
      spacing = o->spacing * radius;
      pos = pos + spacing;
    }
    while (pos < length);

    if (! incremental)
    {
      /* Blend the mask into output */
    }

    if (gegl_buffer_is_shared (output))
      gegl_buffer_unlock (output);

  return  TRUE;
}

static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
#if 0
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  cairo_t *cr;
  cairo_surface_t *surface;
  gchar *data = "     ";
  gboolean result = FALSE;

  surface = cairo_image_surface_create_for_data ((guchar*)data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 1,1,4);
  cr = cairo_create (surface);
  gegl_path_cairo_play (o->d, cr);
  cairo_set_line_width (cr, o->stroke_width);


  if (o->stroke_width > 0.1 && o->stroke_opacity > 0.0001)
    result = cairo_in_stroke (cr, x, y);


  cairo_destroy (cr);

  if (result)
    return operation->node;

#endif
  return NULL;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->detect = detect;
  /*operation_class->no_cache = TRUE;*/

  operation_class->name        = "gegl:brush-stroke";
  operation_class->categories  = "render";
  operation_class->description = _("Renders a brush stroke");
#if 0
  operation_class->get_cached_region = (void*)get_cached_region;
#endif
}


#endif

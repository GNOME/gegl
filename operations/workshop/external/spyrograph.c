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

enum_start (gegl_curve_type)
  enum_value (GEGL_CURVE_TYPE_SPYROGRAPH,   "spyrograph",      N_("Spyrograph"))
  enum_value (GEGL_CURVE_TYPE_EPITROCHOID, "epitrochoid", N_("EPITROCHOID"))
enum_end (GeglCurveType)

property_enum (curve_type, _("Curve Type"),
               GeglCurveType, gegl_curve_type,
               GEGL_CURVE_TYPE_SPYROGRAPH)
  description (_("Curve type"))

property_int   (fixed_gear_teeth, _("Fixed Gear Teeth"), 96)
   description (_("Number of teeth in fixed gear."))
   value_range (10, 180)

property_int   (moving_gear_teeth, _("Moving Gear Teeth"), 36)
   description (_("Number of teeth in moving gear. Radius of moving gear,"
                  " relative to radius of fixed gear, is determined by the "
                  "proportion between the number of teeth in gears."))
   value_range (10, 100)

property_double (hole_percent, _("Hole Percent"), 100.0)
  description (_("How far the hole is from the center of the moving gear. "
                 "100 means that the hole is at the gear's edge."))
  value_range (0.0, 100.0)

property_double (x, _("X"), 0.5)
  description (_("X coordinate of pattern center"))
  ui_range    (0.0, 1.0)
  ui_meta     ("unit", "relative-coordinate")
  ui_meta     ("axis", "x")

property_double (y, _("Y"), 0.5)
  description (_("Y coordinate of pattern center"))
  ui_range    (0.0, 1.0)
  ui_meta     ("unit", "relative-coordinate")
  ui_meta     ("axis", "y")

property_double (radius, _("Radius"), 100.0)
  description (_("Radius of fixed gear"))
  value_range (1.0, G_MAXDOUBLE)
  ui_range    (1.0, 1000.0)
  ui_meta     ("unit", "pixel-distance")

property_double (rotation, _("Rotation"), 0.0)
  description (_("Pattern rotation"))
  value_range (0.0, 360.0)
  ui_meta     ("unit", "degree")

property_color  (stroke, _("Stroke Color"), "rgba(0.0,0.0,0.0,0.0)")
  description(_("Color of paint to use for stroking"))

property_double (stroke_width,_("Stroke width"), 2.0)
  description (_("The width of the brush used to stroke the path"))
  value_range (0.0, 200.0)

property_double (stroke_opacity, _("Stroke opacity"), 1.0)
  description (_("Opacity of stroke, note, does not behave like SVG since at the moment stroking is done using an airbrush tool"))
  value_range (-2.0, 2.0)

property_double (stroke_hardness, _("Hardness"), 0.6)
  description (_("Hardness of the brush, 0.0 for a soft brush, 1.0 for a hard brush"))
  value_range (0.0, 1.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     spyrograph
#define GEGL_OP_C_SOURCE spyrograph.c

#include "gegl-plugin.h"
#include "gegl-path.h"
#include "gegl-op.h"
#include <cairo.h>

typedef struct StampStatic {
  gboolean    valid;
  const Babl *format;
  gfloat     *buf;
  gdouble     radius;
}StampStatic;

static void gegl_path_stroke  (GeglBuffer *buffer,
                               const GeglRectangle *clip_rect,
                               GeglPath *vector,
                               GeglColor  *color,
                               gdouble     linewidth,
                               gdouble     hardness,
                               gdouble     opacity);

static void gegl_path_stamp   (GeglBuffer *buffer,
                               const GeglRectangle *clip_rect,
                               gdouble     x,
                               gdouble     y,
                               gdouble     radius,
                               gdouble     hardness,
                               GeglColor  *color,
                               gdouble     opacity);

static void
gegl_path_stroke (GeglBuffer *buffer,
                  const GeglRectangle *clip_rect,
                  GeglPath *vector,
                  GeglColor  *color,
                  gdouble     linewidth,
                  gdouble     hardness,
                  gdouble     opacity)
{
  gfloat traveled_length = 0;
  gfloat need_to_travel = 0;
  gfloat x = 0,y = 0;
  GeglPathList *iter;
  gdouble       xmin, xmax, ymin, ymax;
  GeglRectangle extent;

  if (!vector)
    return;

  if (!clip_rect)
    {
      g_print ("using buffer extent\n");
      clip_rect = gegl_buffer_get_extent (buffer);
    }

  iter = gegl_path_get_flat_path (vector);
  gegl_path_get_bounds (vector, &xmin, &xmax, &ymin, &ymax);
  extent.x = floor (xmin);
  extent.y = floor (ymin);
  extent.width = ceil (xmax) - extent.x;
  extent.height = ceil (ymax) - extent.y;

  if (!gegl_rectangle_intersect (&extent, &extent, clip_rect))
   {
     return;
   }

  while (iter)
    {
      /*fprintf (stderr, "%c, %i %i\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);*/
      switch (iter->d.type)
        {
          case 'M':
            x = iter->d.point[0].x;
            y = iter->d.point[0].y;
            need_to_travel = 0;
            traveled_length = 0;
            break;
          case 'L':
            {
              GeglPathPoint a,b;

              gfloat spacing;
              gfloat local_pos;
              gfloat distance;
              gfloat offset;
              gfloat leftover;
              gfloat radius = linewidth / 2.0;

              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              spacing = 0.2 * radius;

              distance = gegl_path_point_dist (&a, &b);

              leftover = need_to_travel - traveled_length;
              offset = spacing - leftover;

              local_pos = offset;

              if (distance > 0)
                for (;
                     local_pos <= distance;
                     local_pos += spacing)
                  {
                    GeglPathPoint spot;
                    gfloat ratio = local_pos / distance;
                    gfloat radius = linewidth/2;

                    gegl_path_point_lerp (&spot, &a, &b, ratio);

                    gegl_path_stamp (buffer, clip_rect,
                      spot.x, spot.y, radius, hardness, color, opacity);

                    traveled_length += spacing;
                  }

              need_to_travel += distance;

              x = b.x;
              y = b.y;
            }

            break;
          case 'u':
            g_error ("stroking uninitialized path\n");
            break;
          case 's':
            break;
          default:
            g_error ("can't stroke for instruction: %i\n", iter->d.type);
            break;
        }
      iter=iter->next;
    }
}

static void
gegl_path_stamp (GeglBuffer *buffer,
                 const GeglRectangle *clip_rect,
                 gdouble     x,
                 gdouble     y,
                 gdouble     radius,
                 gdouble     hardness,
                 GeglColor  *color,
                 gdouble     opacity)
{
  gfloat col[4];
  StampStatic s = {FALSE,}; /* there should be a cache of stamps,
                               note that stamps are accessed in multiple threads
                             */

  GeglRectangle temp;
  GeglRectangle roi;

  roi.x = floor(x-radius);
  roi.y = floor(y-radius);
  roi.width = ceil (x+radius) - floor (x-radius);
  roi.height = ceil (y+radius) - floor (y-radius);

  gegl_color_get_pixel (color, babl_format ("RGBA float"), col);

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
      g_free (s.buf);
      /* allocate a little bit more, just in case due to rounding errors and
       * such */
      s.buf = g_malloc (4*4* (roi.width + 2 ) * (roi.height + 2));
      s.radius = radius;
      s.valid = TRUE;
    }
  g_assert (s.buf);

  gegl_buffer_get (buffer, &roi, 1.0, s.format, s.buf, 0, GEGL_ABYSS_NONE);

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
  gegl_buffer_set (buffer, &roi, 0, s.format, s.buf, 0);
  g_free (s.buf);
}

static int 
findGCD(int num1, int num2) {
    if (num2 == 0) {
        return num1;
    }
    if (num1 % num2 == 0) {
        return num2;
    }
    return findGCD(num2, num1 % num2);
}


static GeglPath *
gegl_path_curve(GeglProperties *o)
{
  GeglPath *path = gegl_path_new();
  GeglPathItem gegl_path_item;

  double TWO_PI = 2 * G_PI; 

  int least_common_mult = o->fixed_gear_teeth * o->moving_gear_teeth /
    findGCD(o->fixed_gear_teeth, o-> moving_gear_teeth);
  int steps = least_common_mult + 1;

  // Extract paramters.
  gfloat hole_percent = o->hole_percent;
  gfloat x_center = o->x;
  gfloat y_center = o->y;
  gfloat fixed_gear_radius = o->radius;
  gfloat pattern_rotation = o->rotation * G_PI / 180.0;

  // Variables used in loop.
  int i; 
  gfloat x, y;
  gfloat fixed_gear_angle, moving_gear_angle;

  // Computations.
  gfloat fixed_angle_factor = TWO_PI / o-> fixed_gear_teeth;
  gfloat moving_gear_radius = fixed_gear_radius * o->moving_gear_teeth / o->fixed_gear_teeth ;
  gfloat hole_dist_from_center = hole_percent / 100.0 * moving_gear_radius;

  gfloat moving_angle_factor; 
  if (o->curve_type == GEGL_CURVE_TYPE_SPYROGRAPH) {
    moving_angle_factor = fixed_angle_factor * -1 * 
      (o->fixed_gear_teeth - o->moving_gear_teeth) / o->moving_gear_teeth;
  }
  else {
    moving_angle_factor = fixed_angle_factor *  
      (o->fixed_gear_teeth + o->moving_gear_teeth) / o->moving_gear_teeth;
  }

  // Compute points of pattern.
  for (i=0; i<steps; i++) {
    moving_gear_angle = i * moving_angle_factor;
    fixed_gear_angle = fmod(i * fixed_angle_factor + pattern_rotation, TWO_PI);

    x = x_center + (fixed_gear_radius - moving_gear_radius) * cos(fixed_gear_angle) +
        hole_dist_from_center * cos(moving_gear_angle);
    y = y_center + ( fixed_gear_radius - moving_gear_radius) * sin(fixed_gear_angle) +
        hole_dist_from_center * sin(moving_gear_angle);

    gegl_path_append (path, 'L', x, y);
  }

  // Replace first node from an 'L' type to an 'M' type
  gegl_path_get_node(path, 0, &gegl_path_item);
  gegl_path_item.type = 'M';
  gegl_path_replace_node(path, 0, &gegl_path_item);

  return path;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  // Compute vector of curve, and store it on the operation.
  GeglPath *d = gegl_path_curve(o);
  g_object_set_data (G_OBJECT (operation), "d", d);

  gegl_operation_set_format (operation, "output", babl_format ("R'aG'aB'aA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties    *o       = GEGL_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;
  gdouble        x0, x1, y0, y1;
  GeglPath *d = g_object_get_data (G_OBJECT (operation), "d");

  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");

  gegl_path_get_bounds (d, &x0, &x1, &y0, &y1);
  defined.x      = x0 - o->stroke_width/2;
  defined.y      = y0 - o->stroke_width/2;
  defined.width  = x1 - x0 + o->stroke_width;
  defined.height = y1 - y0 + o->stroke_width;

  if (in_rect)
    {
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}

static void gegl_path_cairo_play (GeglPath *path,
                                    cairo_t *cr);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglPath *d = g_object_get_data (G_OBJECT (operation), "d");

  if (input)
    {
      gegl_buffer_copy (input, result, GEGL_ABYSS_NONE,
                        output, result);
    }
  else
    {
      gegl_buffer_clear (output, result);
    }

  // For small stroke size, use Cairo to draw.
  if (o->stroke_width <= 1.0)
    {
      gdouble color[4] = {0, 0, 0, 0};
      gegl_color_get_pixel (o->stroke, babl_format ("R'G'B'A double"), color);
      color[3] *= o->stroke_opacity;

      if (color[3] > 0.001)
        {
          static GMutex mutex = { 0, };
          cairo_t *cr;
          cairo_surface_t *surface;
          guchar *data;

          g_mutex_lock (&mutex);
          data = gegl_buffer_linear_open (output, result, NULL, babl_format ("cairo-ARGB32"));
          surface = cairo_image_surface_create_for_data (data,
                                                         CAIRO_FORMAT_ARGB32,
                                                         result->width,
                                                         result->height,
                                                         result->width * 4);
          cr = cairo_create (surface);
          cairo_translate (cr, -result->x, -result->y);
          gegl_path_cairo_play (d, cr);
          cairo_set_source_rgba (cr, color[0], color[1], color[2], color[3]);
          cairo_set_line_width (cr, o->stroke_width);
          cairo_stroke (cr);

          g_mutex_unlock (&mutex);
          gegl_buffer_linear_close (output, data);
        }

      return TRUE;
    }


  g_object_set_data (G_OBJECT (operation), "path-radius", GINT_TO_POINTER((gint)(o->stroke_width+1)/2));

  if (o->stroke_width > 0.1 && o->stroke_opacity > 0.0001)
    {
      gegl_path_stroke (output, result,
                                d,
                                o->stroke,
                                o->stroke_width,
                                o->stroke_hardness,
                                o->stroke_opacity);
    }

  return  TRUE;
}


static void foreach_cairo (const GeglPathItem *knot,
                           gpointer              cr)
{
  switch (knot->type)
    {
      case 'M':
        cairo_move_to (cr, knot->point[0].x, knot->point[0].y);
        break;
      case 'L':
        cairo_line_to (cr, knot->point[0].x, knot->point[0].y);
        break;
      case 'z':
        cairo_close_path (cr);
        break;
      default:
        g_print ("%s uh?:%c\n", G_STRLOC, knot->type);
    }
}

static void gegl_path_cairo_play (GeglPath *path,
                                    cairo_t *cr)
{
  gegl_path_foreach_flat (path, foreach_cairo, cr);
}

static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  cairo_t *cr;
  cairo_surface_t *surface;
  gchar *data = "     ";
  gboolean result = FALSE;
  GeglPath *d = g_object_get_data (G_OBJECT (operation), "d");

  surface = cairo_image_surface_create_for_data ((guchar*)data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 1,1,4);
  cr = cairo_create (surface);
  gegl_path_cairo_play (d, cr);
  cairo_set_line_width (cr, o->stroke_width);

  if (o->stroke_width > 0.1 && o->stroke_opacity > 0.0001)
    result = cairo_in_stroke (cr, x, y);

  cairo_destroy (cr);

  if (result)
    return operation->node;

  return NULL;
}

static void
finalize (GObject *object)
{
  GeglPath *d = g_object_get_data (object, "d");
  if (d) {
     gegl_path_clear(d);
  }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  const gchar *composition =
    "<gegl>"
    "<node operation='gegl:crop' width='200' height='200'/>"
    "<node operation='gegl:over'>"
    "<node operation='gegl:spyrograph'>"
    "  <params>"
    "    <param name='fixed-gear-teeth'>96</param>"
    "    <param name='moving-gear-teeth'>36</param>"
    "    <param name='x'>100</param>"
    "    <param name='y'>100</param>"
    "    <param name='radius'>90</param>"
    "    <param name='stroke'>rgba(0,0,1,0.9)</param>"
    "    <param name='stroke-hardness'>1.0</param>"
    "    <param name='stroke-width'>8.0</param>"
    "  </params>"
    "</node>"
    "</node>"
    "<node operation='gegl:checkerboard' color1='rgb(0.25,0.25,0.25)' color2='rgb(0.75,0.75,0.75)'/>"
    "</gegl>";

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  object_class->finalize = finalize;
  filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->detect = detect;
  /*operation_class->cache_policy = GEGL_CACHE_POLICY_NEVER;*/

  gegl_operation_class_set_keys (operation_class,
    "name",           "gegl:spyrograph",
    "title",          _("Render Spyrograph"),
    "categories",     "render",
    "reference-hash", "73276d276ac18bc1f32404e258f7b9ee",
    "reference-composition", composition,
    "description" , _("Renders a Spyrograph pattern"),
    NULL);
}


#endif

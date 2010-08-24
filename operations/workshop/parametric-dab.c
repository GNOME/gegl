
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


/* Brush shape parameters */
gegl_chant_double (scale,    _("Scale Factor"),  0.0, 10.0, 2.0,
                             _("Brush Scale Factor"))
gegl_chant_curve  (scale_curve, _("Scale Curve"), _("Brush Scale curve"))

gegl_chant_double (hardness, _("Hardness"),   0.0, 1.0, 0.6,
                             _("Brush Hardness, 0.0 for soft 1.0 for hard."))
gegl_chant_curve  (hardness_curve, _("Hardness Curve"), _("Brush hardness curve"))

gegl_chant_double (angle,    _("Angle"),   0.0, 360.0, 0.0,
                             _("Brush Angle."))
gegl_chant_curve  (angle_curve, _("Angle Curve"), _("Brush Angle curve"))

gegl_chant_double (aspect,   _("Aspect Ratio"),   0.1, 10.0, 1.0,
                             _("Brush Aspect, 0.1 for pancake 10.0 for spike."))
gegl_chant_curve  (aspect_curve, _("Aspect Curve"), _("Brush Aspect Curve"))

gegl_chant_double (force,    _("Force"),   0.0, 1.0, 0.6,
                             _("Brush Force."))
gegl_chant_curve  (force_curve, _("Force Curve"), _("Brush Force Curve"))
gegl_chant_double (flow,    _("Flow"),   0.0, 1.0, 1.0,
                             _("Brush opacity."))
gegl_chant_curve  (flow_curve, _("Flow Curve"), _("Brush Flow Curve"))

/* dab position parameters */
gegl_chant_double (pos, _("Position"), 0.0, G_MAXDOUBLE, 0.0,
                   _("Position along the curves"))

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE "parametric-dab.c"
#define BASE_RADIUS 50.0

#include "gegl-plugin.h"
#include "gegl-buffer-private.h"

#include "gegl-chant.h"
#include <stdio.h>

/*------- STAMPS --------*/

typedef struct StampStatic {
  gboolean  valid;
  Babl     *format;
  gfloat   *buf;
  gdouble   radius;
}StampStatic;

/* Paint round dab mask centered on 0
 * TODO : use clip rect
 */
static void stamp_round (GeglBuffer *buffer,
                         const GeglRectangle *result,
                         gdouble     scale,
                         gdouble     angle,
                         gdouble     aspect,
                         gdouble     hardness,
                         gdouble     flow)
{
    gdouble radius= BASE_RADIUS * scale;
    static StampStatic s = {FALSE,}; /* XXX: 
                                        we will ultimately leak the last valid
                                        cached brush. */

    GeglRectangle temp;
    GeglRectangle roi;

    roi.x = floor(-radius);
    roi.y = floor(-radius);
    roi.width = ceil (radius) - roi.x;
    roi.height = ceil (radius) - roi.y;

    /* bail out if we wouldn't leave a mark on the buffer */
    if (flow == 0.0) 
      return;
    if (radius < 0.1) 
      return;
    if (hardness == 0.0) 
      return;
    if (!gegl_rectangle_intersect (&temp, &roi, result))
      return;

    /* Set up the stamp */
    if (s.format == NULL)
      s.format = babl_format ("Y float");

    if (s.buf == NULL ||
        s.radius != radius)
      {
        if (s.buf != NULL)
          g_free (s.buf);
        /* allocate a little bit more, just in case due to rounding errors and
         * such */
        s.buf = g_malloc (4* (roi.width + 2 ) * (roi.height + 2));
        s.radius = radius;
        s.valid = TRUE;  
      }
    g_assert (s.buf);



    gegl_buffer_get_unlocked (buffer, 1.0, &roi, s.format, s.buf, 0);
    /* Dab painting, shamelessly borrowed from mypaint */
    {
      gint x, y;
      gint i=0;

      gfloat radius2 = radius * radius;
      gfloat inv_radius2 = 1.0/radius2;

      gfloat angle_rad=angle/360.0*2.0*M_PI;
      gfloat cs=cos(angle_rad);
      gfloat sn=sin(angle_rad);

      for (y= roi.y; y < roi.y + roi.height ; y++)
      {
        for (x= roi.x; x < roi.x + roi.width; x++)
          {
            /* Computes the position of the point from the center
             * 0 is at center, 1 is at border, >1 is out
             */
            gfloat yy = (y*cs-x*sn);
            gfloat xx = (y*sn+x*cs);
            if (aspect>1.0)
              yy*=aspect;
            else
              xx/=aspect;
            gfloat rr = sqrt((yy*yy+xx*xx)*inv_radius2);

            /*
            fprintf(stderr,"rr = %f, xx=%f, yy=%f\n\
                     ar= %f \n",rr,xx,yy, aspect_ratio );
            */
            if (rr<=1.0)
              {
                gfloat opa = flow;
                if (hardness < 1.0)
                  if (rr<hardness)
                    opa *= rr+1.0-(rr/hardness);
                  else
                    opa *= hardness/(1.0-hardness)*(1.0-rr);

                s.buf[i] = opa;
              }
           i++;
          }
      }
    }
    gegl_buffer_set_unlocked (buffer, &roi, s.format, s.buf, 0);
  return;
}

/*------- OPERATION METHODS -------*/

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO    *o       = GEGL_CHANT_PROPERTIES (operation);
  gfloat         r = o->scale * BASE_RADIUS;
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  //if (in_rect)
    {
      result.x = - r;
      result.y = - r;
      result.width  = r*2;
      result.height = r*2;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o  = GEGL_CHANT_PROPERTIES (operation);
  
  /* Computing properties */

  gdouble scale    = o->scale;
  gdouble hardness = o->hardness;
  gdouble flow     = o->flow;
  gdouble angle    = o->angle;
  gdouble aspect   = o->aspect;

  if (o->scale_curve)
    scale = scale * gegl_curve_calc_value(o->scale_curve, o->pos);

  if (o->flow_curve)
    flow = flow * gegl_curve_calc_value(o->flow_curve, o->pos);

  if (o->hardness_curve)
    hardness = hardness * gegl_curve_calc_value(o->hardness_curve, o->pos);

  if (o->angle_curve)
    angle = angle * gegl_curve_calc_value(o->angle_curve, o->pos);

  if (o->aspect_curve)
    aspect = aspect * gegl_curve_calc_value(o->aspect_curve, o->pos);

  gegl_buffer_clear (output, result);

  if (gegl_buffer_is_shared (output))
    while (!gegl_buffer_try_lock (output));
    
  stamp_round (output,result,scale,angle,aspect,hardness,flow);

  if (gegl_buffer_is_shared (output))
    gegl_buffer_unlock (output);

  return  TRUE;
}

#if 0
static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
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
  cairo_set_line_width (cr, o->radius);


  if (o->radius > 0.1 && o->stroke_opacity > 0.0001)
    result = cairo_in_stroke (cr, x, y);


  cairo_destroy (cr);

  if (result)
    return operation->node;

  return NULL;
}
#endif

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  /*operation_class->detect = detect;*/
  /*operation_class->no_cache = TRUE;*/

  operation_class->name        = "gegl:parametric-dab";
  operation_class->categories  = "render";
  operation_class->description = _("Renders a brush dab");
#if 0
  operation_class->get_cached_region = (void*)get_cached_region;
#endif
}
#endif


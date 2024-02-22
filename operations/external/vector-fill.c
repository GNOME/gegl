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
 * Copyright 2006, 2018 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_color  (color, _("Color"),  "rgba(0.0,0.0,0.0,1.0)")
  description(_("Color of paint to use for filling."))

property_double (opacity,  _("Opacity"), 1.0)
  description(_("The fill opacity to use."))
  value_range (-2.0, 2.0)

  /* XXX: replace with enum? */
property_string (fill_rule,_("Fill rule."), "nonzero")
  description(_("how to determine what to fill (nonzero|evenodd)"))

property_string (transform,_("Transform"), "")
  description (_("svg style description of transform."))

property_path (d, _("Vector"), NULL)
  description (_("A GeglVector representing the path of the stroke"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     vector_fill
#define GEGL_OP_C_SOURCE vector-fill.c

#include "gegl-plugin.h"

/* the path api isn't public yet */
#include "property-types/gegl-path.h"
static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata);

#include "gegl-op.h"
#include "ctx.h"

static void path_changed (GeglPath *path,
                          const GeglRectangle *roi,
                          gpointer userdata)
{
  GeglProperties    *o   = GEGL_PROPERTIES (userdata);
  GeglRectangle rect;
  gdouble        x0, x1, y0, y1;

  gegl_path_get_bounds(o->d, &x0, &x1, &y0, &y1);
  rect.x = x0 - 1;
  rect.y = y0 - 1;
  rect.width = x1 - x0 + 2;
  rect.height = y1 - y0 + 2;

  gegl_operation_invalidate (userdata, &rect, TRUE);
};

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *input_format  = gegl_operation_get_source_format (operation, "input");
  const Babl *input_space   = input_format?babl_format_get_space (input_format):NULL;
  const Babl *color_format  = gegl_color_get_format (o->color);
  BablModelFlag model_flags = input_format?babl_get_model_flags (input_format):0;

  if (input_space == NULL){
    input_space = babl_format_get_space (color_format);
    model_flags = babl_get_model_flags (color_format);
  }

  if (model_flags & BABL_MODEL_FLAG_CMYK)
  {
    gegl_operation_set_format (operation, "output", babl_format_with_space ("camayakaA float", input_space));
  }
  else
  {
    gegl_operation_set_format (operation, "output", babl_format_with_space ("RaGaBaA float", input_space));
  }

  if (o->transform && o->transform[0] != '\0')
    {
      GeglMatrix3 matrix;
      gegl_matrix3_parse_string (&matrix, o->transform);
      gegl_path_set_matrix (o->d, &matrix);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties    *o       = GEGL_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;
  gdouble        x0, x1, y0, y1;

  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");

  gegl_path_get_bounds (o->d, &x0, &x1, &y0, &y1);
  defined.x      = x0;
  defined.y      = y0;
  defined.width  = x1 - x0;
  defined.height = y1 - y0;

  if (in_rect)
    {
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}

static void gegl_path_ctx_play (GeglPath *path,
                                Ctx      *ctx);

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gboolean need_fill = FALSE;
  const Babl *format =  gegl_operation_get_format (operation, "output");
  const Babl *device_space  =  babl_format_get_space (format);
  gdouble color[5] = {0, 0, 0, 0, 0};
  int is_cmyk = babl_get_model_flags (format) & BABL_MODEL_FLAG_CMYK ? 1 : 0;
  const Babl *color_format  = gegl_color_get_format (o->color);
  const Babl *color_space   = babl_format_get_space (color_format);

  char device_space_ascii[64]="";
  char color_space_ascii[64]="";

  if (device_space)
    sprintf (device_space_ascii, "%p", device_space);
  if (color_space)
    sprintf (color_space_ascii, "%p", color_space);

  if (input)
    {
      gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, output, result);
    }
  else
    {
      gegl_buffer_clear (output, result);
    }

  if (o->opacity > 0.0001 && o->color)
    {
      if (is_cmyk)
      {
        gegl_color_get_pixel (o->color, babl_format_with_space ("CMYKA double", color_space), color);
        color[4] *= o->opacity;
        if (color[4] > 0.001)
          need_fill=TRUE;
      }
      else
      {
        gegl_color_get_pixel (o->color, babl_format_with_space ("R'G'B'A double", color_space), color);
        color[3] *= o->opacity;
        if (color[3] > 0.001)
          need_fill=TRUE;
      }
    }

  if (need_fill)
    {
      static GMutex mutex = { 0, };

      g_mutex_lock (&mutex);

      {
        guchar *data = gegl_buffer_linear_open (output, result, NULL,
                                                format);
        Ctx *ctx;
        if (is_cmyk)
          ctx = ctx_new_for_framebuffer (data, result->width, result->height,
                                         result->width * 5 * 4, CTX_FORMAT_CMYKAF);
        else
          ctx = ctx_new_for_framebuffer (data, result->width, result->height,
                                         result->width * 4 * 4, CTX_FORMAT_RGBAF);

        if (!is_cmyk)
        {
          if (device_space)
            ctx_colorspace (ctx, CTX_COLOR_SPACE_DEVICE_RGB, (guint8*)device_space_ascii, strlen (device_space_ascii)+1);
          if (color_space)
            ctx_colorspace (ctx, CTX_COLOR_SPACE_USER_RGB, (guint8*)color_space_ascii, strlen (color_space_ascii)+1);
        }

        ctx_translate (ctx, -result->x, -result->y);
        if (g_str_equal (o->fill_rule, "evenodd"))
          ctx_fill_rule (ctx, CTX_FILL_RULE_EVEN_ODD);

        gegl_path_ctx_play (o->d, ctx);

        if (is_cmyk)
            ctx_cmyka (ctx, color[0], color[1], color[2], color[3], color[4]);
        else
            ctx_rgba (ctx, color[0], color[1], color[2], color[3]);
        ctx_fill (ctx);
        ctx_free (ctx);

        gegl_buffer_linear_close (output, data);
      }
      g_mutex_unlock (&mutex);
    }

  return  TRUE;
}

static void foreach_ctx (const GeglPathItem *knot,
                          gpointer           ctx)
{
  switch (knot->type)
    {
      case 'M':
        ctx_move_to (ctx, knot->point[0].x, knot->point[0].y);
        break;
      case 'L':
        ctx_line_to (ctx, knot->point[0].x, knot->point[0].y);
        break;
      case 'C':
        ctx_curve_to (ctx, knot->point[0].x, knot->point[0].y,
                           knot->point[1].x, knot->point[1].y,
                           knot->point[2].x, knot->point[2].y);
        break;
      case 'z':
        ctx_close_path (ctx);
        break;
      default:
        g_print ("%s uh?:%c\n", G_STRLOC, knot->type);
    }
}

static void gegl_path_ctx_play (GeglPath *path,
                                Ctx       *ctx)
{
  gegl_path_foreach_flat (path, foreach_ctx, ctx);
}

static GeglNode *detect (GeglOperation *operation,
                         gint           x,
                         gint           y)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Ctx     *ctx = ctx_new_drawlist (-1, -1);
  gboolean result = FALSE;

  gegl_path_ctx_play (o->d, ctx);

  if (!result)
    {
      if (o->d)
        {
          result = ctx_in_fill (ctx, x, y);
        }
    }
  ctx_free (ctx);

  if (result)
    return operation->node;

  return NULL;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;
  gchar                    *composition = "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop' width='200' height='200'/>"
    "<node operation='gegl:over'>"
    "<node operation='gegl:translate' x='40' y='40'/>"
    "<node operation='gegl:fill-path'>"
    "  <params>"
    "    <param name='color'>rgb(0.0, 0.6, 1.0)</param>"
    "    <param name='d'>"
    "M0,50 C0,78 24,100 50,100 C77,100 100,78 100,50 C100,45 99,40 98,35 C82,35 66,35 50,35 C42,35 35,42 35,50 C35,58 42,65 50,65 C56,65 61,61 64,56 C67,51 75,55 73,60 C69,  69 60,75 50,75 C36,75 25,64 25,50 C25,36 36,25 50,25 L93,25 C83,9 67,0 49,0 C25,0 0,20 0,50   z"
    "                    </param>"
    "  </params>"
    "</node>"
    "</node>"
    "<node operation='gegl:checkerboard' color1='rgb(0.25,0.25,0.25)' color2='rgb(0.75,0.75,0.75)'/>"
    "</gegl>";


  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->detect = detect;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:fill-path",
    "title",       _("Fill Path"),
    "categories",  "render:vector",
    "reference-hash", "f76db1e12141c49e0f117a9dcde5d4e5",
    "description", _("Renders a filled region"),
    "reference-composition", composition,
    NULL);
}

#endif

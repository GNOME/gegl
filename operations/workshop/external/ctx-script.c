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
#include "ctx.h"

#ifdef GEGL_PROPERTIES

property_string (d, _("Vector"), "rgb 1 1 1 rectangle 0 0 400 400 fill rgb 1 0 0 rectangle 10 10 10 10 fill ")
  description (_("A GeglVector representing the path of the stroke"))
  ui_meta ("multiline", "true")


#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     ctx_script
#define GEGL_OP_C_SOURCE ctx-script.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *input_format  = gegl_operation_get_source_format (operation, "input");
  const Babl *input_space   = input_format?babl_format_get_space (input_format):NULL;

  gegl_operation_set_format (operation, "output", babl_format_with_space ("RaGaBaA float", input_space));

}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  //GeglProperties    *o       = GEGL_PROPERTIES (operation);
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;

  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");

#if 0
  // TODO : parse script and get pos
  gdouble        x0, x1, y0, y1;
  gegl_path_get_bounds (o->d, &x0, &x1, &y0, &y1);
  defined.x      = x0;
  defined.y      = y0;
  defined.width  = x1 - x0;
  defined.height = y1 - y0;
#endif

  if (in_rect)
    { 
      return *in_rect;
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }

  return defined;
}

#include <stdio.h>

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *format =  gegl_operation_get_format (operation, "output");

  gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, output, result);
  guchar *data = gegl_malloc(result->width*result->height*4*4);
  gegl_buffer_get (input, result, 1.0, format, data, result->width*4*4, GEGL_ABYSS_NONE);
  Ctx *ctx;
  ctx = ctx_new_for_framebuffer (data, result->width, result->height,
                                 result->width * 4 * 4, CTX_FORMAT_RGBAF);

  ctx_translate (ctx, -result->x, -result->y);

  ctx_save(ctx);
  ctx_parse (ctx, o->d);
  ctx_restore(ctx);

  ctx_free (ctx);

  gegl_buffer_set (output, result, 0, format, data, result->width*4*4);
  gegl_free (data);

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;


  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:ctx-script",
    "title",       _("Ctx script"),
    "categories",  "render:vector",
    "description", _("Renders a ctx vector graphics"),
    NULL);
}

#endif

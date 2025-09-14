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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 *           2023 Øyvind Kolås <pippin@gimp.org>
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_boolean (clip_low, _("Clip low pixel values"), TRUE)
     description (_("Clip low pixel values"))

property_double (low_limit, _("Low limit"), 0.0)
    value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
    ui_range    (-1.0, 1.0)
    description (_("Pixels values lower than this limit will be set to it"))
    ui_meta     ("sensitive", "clip-low")

property_boolean (clip_high, _("Clip high pixel values"), TRUE)
     description (_("Clip high pixel values"))

property_double (high_limit, _("High limit"), 1.0)
    value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
    ui_range    (0.0, 2.0)
    description (_("Pixels values higher than this limit will be set to it"))
    ui_meta     ("sensitive", "clip-high")

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     alpha_clip
#define GEGL_OP_C_SOURCE alpha-clip.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *src_format = gegl_operation_get_source_format (operation, "input");
  const char *format     = "RGBA float";

  if (src_format)
    {
      const Babl *model = babl_format_get_model (src_format);

      if (babl_model_is (model, "RGB"))
        format = "RGBA float";
      else if (babl_model_is (model, "RGBA"))
        format = "RGBA float";
      else if (babl_model_is (model, "R'G'B'"))
        format = "R'G'B'A float";
      else if (babl_model_is (model, "R'G'B'A"))
        format = "R'G'B'A float";
    }

  gegl_operation_set_format (operation, "input",  babl_format_with_space (format, space));
  gegl_operation_set_format (operation, "output", babl_format_with_space (format, space));
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  gint            n_components = 4;
  gfloat *input  = in_buf;
  gfloat *output = out_buf;

  float low_limit = o->low_limit;
  float high_limit = o->high_limit;

  if (o->clip_low && o->clip_high)
    {
      while (n_pixels--)
        {
          for (int i = 0; i < 3; i++) output[i] = input[i];
          output[3] = CLAMP (input[3], low_limit, high_limit);

          input  += n_components;
          output += n_components;
        }
    }

  else if (o->clip_high)
    {
      while (n_pixels--)
          {
            for (int i = 0; i < 3; i++) output[i] = input[i];
            output[3] = input[3] > high_limit ? high_limit : input[3];

            input  += n_components;
            output += n_components;
          }
    }

  else if (o->clip_low)
    {
      while (n_pixels--)
        {
          for (int i = 0; i < 3; i++) output[i] = input[i];
          output[3] = input[3] < low_limit ? low_limit : input[3];

          input  += n_components;
          output += n_components;
        }
    }

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
  GeglProperties      *o = GEGL_PROPERTIES (operation);

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  if (!o->clip_high && !o->clip_low)
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                              g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;
  gchar                         *composition = 
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "  <node operation='gegl:crop' width='200' height='200'/>"
    "  <node operation='gegl:over'>"
    "    <node operation='gegl:alpha-clip'>"
    "      <params>"
    "        <param name='low_limit'>0.2</param>"
    "        <param name='high_limit'>0.8</param>"
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
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  operation_class->opencl_support = FALSE;

  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:alpha-clip",
    "title",       _("Clip Alpha"),
    "categories",  "color",
    "reference-composition", composition,
    "reference-hash", "4f82a070d379eab44c88d3c68ecadb22",
    "description", _("Keep alpha values inside a specific range"),
    NULL);
}

#endif

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
 * Copyright 2018 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#define MAX_ITERATIONS 20
#define MAX_TRANSFORMS 10
#define EPSILON        1e-6

#ifdef GEGL_PROPERTIES

property_string  (transform, _("Transform"), "matrix (1, 0, 0, 0, 1, 0, 0, 0, 1)")
    description  (_("Transformation matrix, using SVG syntax "
                    "(or multiple matrices, separated by semicolons)"))

property_int     (first_iteration, _("First iteration"), 0)
    description  (_("First iteration"))
    value_range  (0, MAX_ITERATIONS)

property_int     (iterations, _("Iterations"), 3)
    description  (_("Number of iterations"))
    value_range  (0, MAX_ITERATIONS)

property_color (fade_color, _("Fade color"), "transparent")
    description  (_("Color to fade transformed images towards, "
                    "with a rate depending on its alpha"))

property_double  (fade_opacity, _("Fade opacity"), 1.0)
    description  (_("Amount by which to scale the opacity of "
                    "each transformed image"))
    value_range  (0.0, 1.0)

property_boolean (paste_below, _("Paste below"), FALSE)
    description  (_("Paste transformed images below each other"))

property_enum    (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_LINEAR)
    description  (_("Mathematical method for reconstructing pixel values"))

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     recursive_transform
#define GEGL_OP_C_SOURCE recursive-transform.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *transform_nodes[MAX_TRANSFORMS];
  GeglNode *color_overlay_node;
  GeglNode *opacity_node;
  GeglNode *over_nodes[MAX_TRANSFORMS];
} Iteration;

static void
update_graph (GeglOperation *operation)
{
  GeglProperties  *o     = GEGL_PROPERTIES (operation);
  Iteration       *iters = o->user_data;
  GeglNode        *node  = operation->node;
  GeglNode        *input;
  GeglNode        *output;
  gchar          **matrix_strs;
  gdouble          fade_color[4];
  gint             i;
  gint             j;

  if (! iters)
    return;

  input  = gegl_node_get_input_proxy  (node, "input");
  output = gegl_node_get_output_proxy (node, "output");

  gegl_node_link (input, output);

  for (i = 0; i <= MAX_ITERATIONS; i++)
    {
      for (j = MAX_TRANSFORMS - 1; j >= 0; j--)
        {
          g_object_set (iters[i].over_nodes[j],
                        "cache-policy", GEGL_CACHE_POLICY_AUTO,
                        NULL);

          gegl_node_disconnect (iters[i].over_nodes[j],    "input");
          gegl_node_disconnect (iters[i].over_nodes[j],    "aux");
        }

      gegl_node_disconnect (iters[i].opacity_node,         "input");
      gegl_node_disconnect (iters[i].color_overlay_node,   "input");

      for (j = 0; j < MAX_TRANSFORMS; j++)
        gegl_node_disconnect (iters[i].transform_nodes[j], "input");
    }

  if (o->first_iteration == 0 && o->iterations == 0)
    return;

  matrix_strs = g_strsplit (o->transform, ";", MAX_TRANSFORMS + 1);

  if (! matrix_strs[0])
    {
      g_strfreev (matrix_strs);

      return;
    }

  gegl_color_get_rgba (o->fade_color,
                       &fade_color[0],
                       &fade_color[1],
                       &fade_color[2],
                       &fade_color[3]);

  if (! matrix_strs[1])
    {
      GeglMatrix3 transform;

      gegl_matrix3_parse_string (&transform, matrix_strs[0]);

      for (i = o->iterations; i >= 0; i--)
        {
          GeglNode    *source_node;
          GeglMatrix3  matrix;
          gchar       *matrix_str;
          gint         n = o->first_iteration + i;

          gegl_matrix3_identity (&matrix);

          for (j = 0; j < n; j++)
            gegl_matrix3_multiply (&matrix, &transform, &matrix);

          matrix_str = gegl_matrix3_to_string (&matrix);

          gegl_node_set (iters[i].transform_nodes[0],
                         "transform", matrix_str,
                         "sampler",   o->sampler_type,
                         NULL);

          g_free (matrix_str);

          gegl_node_link (input, iters[i].transform_nodes[0]);
          source_node = iters[i].transform_nodes[0];

          if (n > 0 && fabs (fade_color[3]) > EPSILON)
            {
              GeglColor *color = gegl_color_new (NULL);
              gdouble    a = 1.0 - pow (1.0 - fade_color[3], n);

              gegl_color_set_rgba (color,
                                   fade_color[0],
                                   fade_color[1],
                                   fade_color[2],
                                   a);

              gegl_node_set (iters[i].color_overlay_node,
                             "value", color,
                             "srgb",  TRUE,
                             NULL);

              g_object_unref (color);

              gegl_node_link (source_node, iters[i].color_overlay_node);
              source_node = iters[i].color_overlay_node;
            }

          if (n > 0 && fabs (o->fade_opacity - 1.0) > EPSILON)
            {
              gegl_node_set (iters[i].opacity_node,
                             "value", pow (o->fade_opacity, n),
                             NULL);

              gegl_node_link (source_node, iters[i].opacity_node);
              source_node = iters[i].opacity_node;
            }

          gegl_node_connect (source_node, "output",
                             iters[i].over_nodes[0],
                                ! o->paste_below ? "input" : "aux");

          if (i == 0)
            {
              gegl_node_link (iters[i].over_nodes[0], output);
            }
          else
            {
              gegl_node_connect (iters[i].over_nodes[0], "output",
                                 iters[i - 1].over_nodes[0],
                                    ! o->paste_below ? "aux" :
                                                                                   "input");
            }
        }
    }
  else
    {
      gint n_iterations = MIN (o->first_iteration + o->iterations, MAX_ITERATIONS);
      gint n_transforms;

      for (n_transforms = 0;
           n_transforms < MAX_TRANSFORMS && matrix_strs[n_transforms];
           n_transforms++);

      for (i = n_iterations; i >= 0; i--)
        {
          if (i < n_iterations)
            {
              GeglNode *source_node = NULL;

              for (j = 0; j < n_transforms; j++)
                {
                  gegl_node_set (iters[i].transform_nodes[j],
                                 "transform", matrix_strs[j],
                                 "sampler",   o->sampler_type,
                                 NULL);

                  gegl_node_link (iters[i + 1].over_nodes[n_transforms - 1],
                                  iters[i].transform_nodes[j]);

                  if (j == 0)
                    {
                      source_node = iters[i].transform_nodes[j];
                    }
                  else
                    {
                      if (! o->paste_below)
                        {
                          gegl_node_connect (
                                      source_node,                 "output",
                                      iters[i].over_nodes[j - 1],  "input");
                          gegl_node_connect (
                                      iters[i].transform_nodes[j], "output",
                                      iters[i].over_nodes[j - 1],  "aux");
                        }
                      else
                        {
                          gegl_node_connect (source_node,                 "output",
                                             iters[i].over_nodes[j - 1],  "aux");
                          gegl_node_connect (iters[i].transform_nodes[j], "output",
                                             iters[i].over_nodes[j - 1],  "input");
                        }

                      source_node = iters[i].over_nodes[j - 1];
                    }
                }

              if (fabs (fade_color[3]) > EPSILON)
                {
                  gegl_node_set (iters[i].color_overlay_node,
                                 "value", o->fade_color,
                                 "srgb",  TRUE,
                                 NULL);

                  gegl_node_link (source_node, iters[i].color_overlay_node);
                  source_node = iters[i].color_overlay_node;
                }

              if (fabs (o->fade_opacity - 1.0) > EPSILON)
                {
                  gegl_node_set (iters[i].opacity_node,
                                 "value", o->fade_opacity,
                                 NULL);

                  gegl_node_link (source_node, iters[i].opacity_node);
                  source_node = iters[i].opacity_node;
                }

              gegl_node_connect (source_node, "output",
                                 iters[i].over_nodes[n_transforms - 1], ! o->paste_below ? "aux" :
                                                                                              "input");

              if (i > 0)
                {
                  g_object_set (iters[i].over_nodes[n_transforms - 1],
                                "cache-policy", GEGL_CACHE_POLICY_ALWAYS,
                                NULL);
                }
            }

          if (i >= o->first_iteration)
            {
              gegl_node_connect (input, "output",
                                 iters[i].over_nodes[n_transforms - 1],
                                   ! o->paste_below ? "input" :
                                                                                              "aux");
            }
        }

      gegl_node_link (iters[0].over_nodes[n_transforms - 1], output);
    }

  g_strfreev (matrix_strs);
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o     = GEGL_PROPERTIES (operation);
  Iteration      *iters = o->user_data;
  GeglNode       *node  = operation->node;
  gint            i;
  gint            j;

  if (! iters)
    iters = o->user_data = g_new (Iteration, MAX_ITERATIONS + 1);

  for (i = 0; i <= MAX_ITERATIONS; i++)
    {
      for (j = 0; j < MAX_TRANSFORMS; j++)
        {
          iters[i].transform_nodes[j] =
            gegl_node_new_child (node,
                                 "operation", "gegl:transform",
                                 NULL);
        }

      iters[i].color_overlay_node =
        gegl_node_new_child (node,
                             "operation", "gegl:color-overlay",
                             NULL);

      iters[i].opacity_node =
        gegl_node_new_child (node,
                             "operation", "gegl:opacity",
                             NULL);

      for (j = 0; j < MAX_TRANSFORMS; j++)
        {
          iters[i].over_nodes[j] =
            gegl_node_new_child (node,
                                 "operation", "gegl:over",
                                 NULL);

        }
    }
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  g_clear_pointer (&o->user_data, g_free);

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass           *object_class;
  GeglOperationClass     *operation_class;
  GeglOperationMetaClass *operation_meta_class;

  object_class         = G_OBJECT_CLASS (klass);
  operation_class      = GEGL_OPERATION_CLASS (klass);
  operation_meta_class = GEGL_OPERATION_META_CLASS (klass);

  object_class->dispose        = dispose;
  operation_class->attach      = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:recursive-transform",
    "title",       _("Recursive Transform"),
    "categories",  "map",
    "description", _("Apply a transformation recursively."),
    NULL);
}

#endif

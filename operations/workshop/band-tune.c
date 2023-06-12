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
 * Copyright 2020 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (radius1, _("Detail band"), 1.1)
    description(_("Features size for detail band, used for noise removal."))
    value_range (0.0, 100.0)
    ui_range    (0.5, 2.0)

property_double (scale1, _("Detail scale, negative values diminish signal in detail band, positive values increase signal."), -1.6)
    description(_("Scaling factor for image features at radius, -1 cancels them out 1.0 edge enhances"))
    value_range (-100.0, 100.0)
    ui_range    (-3.0, 3.0)

property_double (bw1, _("Detail bandwidth"), 0.375)
    description("lower values narrower band, higher values wider band - default value presumed to provide good band separation.")
    value_range (0.01, 0.6)



property_double (radius2, _("Edge band"), 10.0)
    description(_("Features size for edge band, used to compensate for loss of edges in detail pass."))
    value_range (0.0, 100.0)
    ui_range    (0.5, 2.0)

property_double (scale2, _("Edge scale, negative values diminish signal in detail band, positive values increase signal."), 0.0)
    description(_("Scaling factor for image features at radius, -1 cancels them out 1.0 edge enhances"))
    value_range (-100.0, 100.0)
    ui_range    (-3.0, 3.0)

property_double (bw2, _("Edge bandwidth"), 0.375)
    description("lower values narrower band, higher values wider band - default value presumed to provide good band separation.")
    value_range (0.01, 0.6)

property_boolean (show_mask, _("Visualize Adjustment Mask"), FALSE)

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     band_tune
#define GEGL_OP_C_SOURCE band-tune.c

#include "gegl-op.h"

#define N_BANDS 2

typedef struct
{
  GeglNode *input;
  GeglNode *add[N_BANDS];
  GeglNode *sub[N_BANDS];
  GeglNode *blur1[N_BANDS];
  GeglNode *blur2[N_BANDS];
  GeglNode *mul[N_BANDS];

  GeglNode *mask_sub;
  GeglNode *mask_add;
  GeglNode *mask_mul;

  GeglNode *output;
} State;

static float compute_radius2 (float center_radius, float bw)
{
  return center_radius / (bw + 1);
}
static float compute_radius1 (float center_radius, float bw)
{
  return compute_radius2(center_radius, bw) * bw;
}

static void
update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  float bw = 0.625;
  GeglNode *iter = state->input;

  for (int band = 0; band < N_BANDS; band++)
  {
    float scale = 0.0;
    float radius = 0.0;
    switch (band)
    {
      case 0:
        scale = o->scale1;
        radius = o->radius1;
        bw = 1.0 - o->bw1;
        break;
#if 1
      case 1:
        scale = o->scale2;
        radius = o->radius2;
        bw = 1.0 - o->bw2;
        break;
#endif
    }

    if (fabs(scale) > 0.01)
    {
      float radius1 = compute_radius1(radius, bw);
      float radius2 = compute_radius2(radius, bw);
      gegl_node_set (state->blur1[band], "std-dev-x", radius1,
                                         "std-dev-y", radius1,
                                         NULL);
      gegl_node_set (state->blur2[band], "std-dev-x", radius2,
                                         "std-dev-y", radius2,
                                          NULL);
      gegl_node_set (state->mul[band], "value", scale, NULL);
      gegl_node_connect (state->add[band],   "input",
                         iter,               "output");
      gegl_node_connect (state->blur1[band], "input",
                         iter,               "output");
      gegl_node_connect (state->blur2[band], "input",
                         iter,               "output");
      gegl_node_connect (state->sub[band],   "input",
                         state->blur1[band], "output");
      gegl_node_connect (state->sub[band],   "aux",
                         state->blur2[band], "output");
      gegl_node_connect (state->mul[band],   "input",
                         state->sub[band],   "output");
      gegl_node_connect (state->add[band],   "aux",
                         state->mul[band],   "output");
      iter = state->add[band];
    }
  }

  if (o->show_mask)
  {
    gegl_node_connect (state->mask_sub, "input",
                       state->input,    "output");
    gegl_node_connect (state->mask_sub, "aux",
                       iter,            "output");

    gegl_node_connect (state->mask_mul, "input",
                       state->mask_sub, "output");

    gegl_node_connect (state->mask_add, "input",
                       state->mask_mul, "output");
    iter = state->mask_add;
  }

  gegl_node_connect (state->output, "input",
                     iter,          "output");
}

static void
attach (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl = operation->node;
  State *state = g_malloc0 (sizeof (State));
  o->user_data = state;

  state->input    = gegl_node_get_input_proxy (gegl, "input");
  state->output   = gegl_node_get_output_proxy (gegl, "output");

  for (int band = 0; band < N_BANDS; band++)
  {
    state->add[band]   = gegl_node_new_child (gegl, "operation", "gegl:add", NULL);
    state->mul[band]   = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 0.0, NULL);
    state->sub[band]   = gegl_node_new_child (gegl, "operation", "gegl:subtract", NULL);
    state->blur1[band] = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
    state->blur2[band] = gegl_node_new_child (gegl, "operation", "gegl:gaussian-blur", NULL);
  }

  state->mask_add = gegl_node_new_child (gegl, "operation", "gegl:add", "value", 0.2, NULL);
  state->mask_sub = gegl_node_new_child (gegl, "operation", "gegl:subtract", NULL);
  state->mask_mul = gegl_node_new_child (gegl, "operation", "gegl:multiply", "value", 4.0, NULL);
}

static void
dispose (GObject *object)
{
   GeglProperties  *o     = GEGL_PROPERTIES (object);
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
    "name",        "gegl:band-tune",
    "title",       _("Band tune"),
    "categories",  "enhance:sharpen:denoise",
    "description", _("Parametric band equalizer for tuning frequency bands of image, the op provides abstracted input parameters that control two difference of gaussians driven band pass filters used as adjustments of the image signal."),
    NULL);
}

#endif

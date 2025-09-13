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
 * Copyright 2023 2025 Øyvind Kolås <pippin@gimp.org>
 * 2022-2024 Sam Lester (Official Bevel - a new bevel filter candidate for Gimp
 * 3 that combines my bevel algorithms - featuring the algorithm of my  bevel,
 * custom bevel, 
   and sharp bevel all in one filter) 

Bump Bevel Graph inspired by my plugin Custom Bevel from 2022

median-blur radius=1 alpha-percentile=80
gaussian-blur std-dev-x=4 std-dev-y=4
id=2 overlay aux=[ ref=2
emboss ]
id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ] opacity value=2 median-blur radius=0


Chamfer Bevel Graph inspired by my plugin Sharp Bevel from 2023

median-blur radius=1 alpha-percentile=80
id=1 src-in aux=[ ref=1 
median-blur radius=0 
id=2 overlay aux=[ ref=2
distance-transform
emboss depth=15 azimuth=33 ]

 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gbevel_type)
  enum_value (GEGL_BEVEL_CHAMFER,  "chamfer",
              N_("Chamfer"))
  enum_value (GEGL_BEVEL_BUMP,     "bump",
              N_("Bump"))
enum_end (gbeveltype)

property_enum (type, _("Bevel Type"),
    gbeveltype, gbevel_type,
    GEGL_BEVEL_CHAMFER)
    description (_("The family of bevel to use"))

enum_start (gchamfer_blend_mode)
  enum_value (CHAMFER_BLEND_GIMPBLEND, "gimpblend",
              N_("None (for use with GIMPs blending options)"))
  enum_value (CHAMFER_BLEND_HARDLIGHT, "hardlight",
              N_("Hard Light"))
  enum_value (CHAMFER_BLEND_MULTIPLY,  "multiply",
              N_("Multiply"))
  enum_value (CHAMFER_BLEND_COLORDODGE,  "colordodge",
              N_("Color Dodge"))
  enum_value (CHAMFER_BLEND_DARKEN,    "darken",
              N_("Darken"))
  enum_value (CHAMFER_BLEND_LIGHTEN,   "lighten",
              N_("Lighten"))
  enum_value (CHAMFER_BLEND_ADD,   "add",
              N_("Add"))
enum_end (gChamferBlendMode)

property_enum (blendmode, _("Blend Mode"),
    gChamferBlendMode, gchamfer_blend_mode,
    CHAMFER_BLEND_HARDLIGHT)
  description (_("What blending mode the bevel's emboss will be. Light Map is a special blend mode that allows users to extract the filters output as a light map which should be put on a layer above or be used with Gimp's blending options."))

property_enum (metric, _("Distance Map Setting"),
               GeglDistanceMetric, gegl_distance_metric, GEGL_DISTANCE_METRIC_CHEBYSHEV)
    description (_("Distance Map is unique to chamfer bevel and has three settings that alter the structure of the chamfer."))
ui_meta ("visible", "!type {bump}" )

property_double (radius, _("Radius"), 3.0)
  value_range (1.0, 8.0)
  ui_range (1.0, 8.0)
  ui_gamma (1.5)
ui_meta ("visible", "!type {chamfer}" )
    description (_("Radius of softening for making bump of the shape."))
  ui_steps      (0.01, 0.50)

property_double (elevation, _("Elevation"), 25.0)
    description (_("Elevation angle of the Bevel."))
    value_range (0.0, 180.0)
    ui_meta ("unit", "degree")
  ui_steps      (0.01, 0.50)


/*An issue with this filter is that emboss depth= above 15 does nothing on chamfer bevel , but works great on bump bevel.
so I made an extra node just for it that only updates emboss depth=, as a result depth and sharpness' are co-existing properties.
*/

property_int (depth, _("Depth"), 40)
    description (_("Emboss depth - Brings out depth and detail of the bump bevel."))
    value_range (1, 100)
    ui_range (1, 80)

property_double (azimuth, _("Light Angle"), 68.0)
    description (_("Direction of a light source illuminating and shading the bevel."))
    value_range (0, 360)
  ui_steps      (0.01, 0.50)
    ui_meta ("unit", "degree")
    ui_meta ("direction", "ccw")


#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gegl_bevel
#define GEGL_OP_C_SOURCE bevel.c

#include "gegl-op.h"

typedef struct
{
  GeglNode *input;
  GeglNode *blur;
  GeglNode *emb;
  GeglNode *emb2;
  GeglNode *dt;
  GeglNode *blend;
  GeglNode *opacity;
  GeglNode *nop;
  GeglNode *nop2;
  GeglNode *median;
  GeglNode *thresholdalpha;
  GeglNode *replaceontop;
  GeglNode *fixbump;
  GeglNode *smoothchamfer;
  GeglNode *output;
} State; 


static void attach (GeglOperation *operation)
{
  GeglNode *gegl = operation->node;
  GeglProperties *o = GEGL_PROPERTIES (operation);

  State *state = o->user_data = g_malloc0 (sizeof (State));

  state->input    = gegl_node_get_input_proxy (gegl, "input");
  state->output   = gegl_node_get_output_proxy (gegl, "output");

  state->blur = gegl_node_new_child (gegl,
                                  "operation", "gegl:gaussian-blur", "clip-extent", FALSE,   "abyss-policy", 0,                
                                  NULL);

  state->emb   = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss",
                                  NULL);

  state->emb2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:emboss", "depth", 15,
                                  NULL);

  state->opacity   = gegl_node_new_child (gegl,
                                  "operation", "gegl:opacity", "value", 0.8,
                                  NULL);

  state->replaceontop   = gegl_node_new_child (gegl, /*This blend mode is replace + alpha lock*/
                                  "operation", "gegl:src-in",
                                  NULL);
                                
  state->nop   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->nop2   = gegl_node_new_child (gegl,
                                  "operation", "gegl:nop",
                                  NULL);

  state->dt   = gegl_node_new_child (gegl,
                                  "operation", "gegl:distance-transform", "metric", 2,
                                  NULL);

  state->median   = gegl_node_new_child (gegl,
                                  "operation", "gegl:median-blur", "radius", 1, "alpha-percentile", 80.0,
                                  NULL);

                                    #define EMBEDDEDGRAPH \
" opacity value=1.7 median-blur abyss-policy=none radius=0 id=0 dst-out aux=[ ref=0  component-extract component=alpha   levels in-low=0.15  color-to-alpha opacity-threshold=0.4  ]  median-blur abyss-policy=none radius=0 "\
                              
  state->thresholdalpha   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", EMBEDDEDGRAPH,
                                  NULL);

                                    #define EMBEDDEDGRAPH2 \
" opacity value=2.2 median-blur abyss-policy=none radius=0 "\

  state->fixbump   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", EMBEDDEDGRAPH2, /* I prefer using median-blur radius=0 over gegl:alpha-clip*/
                                  NULL);

                                    #define EMBEDDEDGRAPH3 \
" id=1 src-atop aux=[ ref=1 bilateral-filter blur-radius=4 edge-preservation=6 mean-curvature-blur iterations=1 ] "  /* This hidden graph smooths the bevel */

  state->smoothchamfer   = gegl_node_new_child (gegl,
                                  "operation", "gegl:gegl", "string", EMBEDDEDGRAPH3, 
                                  NULL);

  state->blend      = gegl_node_new_child (gegl, "operation", "gegl:hard-light", /* This blend mode can be anything, but in default its hardlight */
                                           NULL);

  gegl_operation_meta_redirect (operation, "radius", state->blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "radius", state->blur, "std-dev-y");
  gegl_operation_meta_redirect (operation, "elevation", state->emb, "elevation");
  gegl_operation_meta_redirect (operation, "azimuth", state->emb, "azimuth");
  gegl_operation_meta_redirect (operation, "elevation", state->emb2, "elevation");
  gegl_operation_meta_redirect (operation, "azimuth", state->emb2, "azimuth");
  gegl_operation_meta_redirect (operation, "metric", state->dt, "metric");
}

static void update_graph (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  if (!state)
    return;

  const char *blend_op = "gegl:nop";
  switch (o->blendmode) {
    case CHAMFER_BLEND_GIMPBLEND:  blend_op = "gegl:src"; break;
    case CHAMFER_BLEND_HARDLIGHT:  blend_op = "gegl:hard-light"; break;
    case CHAMFER_BLEND_MULTIPLY:   blend_op = "gegl:multiply"; break;
    case CHAMFER_BLEND_COLORDODGE: blend_op = "gegl:color-dodge"; break; 
    case CHAMFER_BLEND_DARKEN:     blend_op = "gegl:darken"; break;
    case CHAMFER_BLEND_LIGHTEN:    blend_op = "gegl:lighten"; break;
    case CHAMFER_BLEND_ADD:        blend_op = "gegl:add"; break;
  }
  gegl_node_set (state->blend, "operation", blend_op, NULL);

  if (o->type == GEGL_BEVEL_CHAMFER)
  {
    int level = o->depth / 100.0 * 15;
    if (level == 0)
      level = 1;
    gegl_node_set (state->emb2, "depth", level, NULL);

    gegl_node_link_many (state->input, state->median, state->nop, state->replaceontop, state->smoothchamfer, state->output, NULL);
    gegl_node_connect (state->replaceontop, "aux", state->blend, "output");
    gegl_node_link_many (state->nop, state->nop2, state->blend, NULL);
    gegl_node_connect (state->blend, "aux", state->opacity, "output");
    gegl_node_link_many (state->nop2, state->dt, state->emb2, state->opacity, NULL);
  }
  else
  {
    gegl_node_set (state->emb, "depth", o->depth, NULL);

    if (o->blendmode > CHAMFER_BLEND_GIMPBLEND)
    {
      gegl_node_link_many (state->input, state->median, state->blur, state->nop, state->blend, state->thresholdalpha, state->output, NULL);
      gegl_node_link_many (state->nop, state->emb,  NULL);
      gegl_node_connect (state->blend, "aux", state->emb, "output");
    }
    else
    {
      gegl_node_link_many (state->input, state->median, state->blur, state->emb, state->fixbump, state->output, NULL);
    }
  }
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
  GeglOperationClass *operation_class;
  GeglOperationMetaClass *operation_meta_class = GEGL_OPERATION_META_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  G_OBJECT_CLASS(klass)->dispose = dispose;
  operation_class->attach = attach;
  operation_meta_class->update = update_graph;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:bevel",
    "title",       _("Bevel"),
    "reference-hash", "44143870affcfdba0bbb8b7247ca14fb",
    "description", _("Two bevel effects in one place, Chamfer - which simulates lighting of chamfered 3D-edges, and Bump - the second make a 3D inflation effect by an emboss covering a blur. Both bevels benefit from color filled alpha defined shapes."),
    "gimp:menu-path", "<Image>/Filters/Light and Shadow",
    "gimp:menu-label", _("Bevel..."),
    NULL);
}

#endif

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
 * Copyright 2006 ?yvind Kolås <pippin@gimp.org>
 */
#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int(cutoff, "cutoff", 0, G_MAXINT, 0, "The cutoff frequency of gaussian lowpass.")
gegl_chant_int(flag, "Flag", 0, 15, 14,
               "Decide which componet need to process. Example: if flag=14, "
               "14==0b1110, so we filter on componets RGB, do not filter on A.")

#else

#define GEGL_CHANT_TYPE_META
#define GEGL_CHANT_C_FILE       "gaussian-lowpass.c"

#include "gegl-chant.h"

typedef struct _Priv Priv;
struct _Priv
{
  GeglNode *self;
  GeglNode *input;
  GeglNode *output;

  GeglNode *dft_forward;
  GeglNode *glpf_filter;
  GeglNode *dft_backward;

};

static void attach (GeglOperation *operation)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  Priv       *priv = g_malloc0 (sizeof (Priv));

  o->chant_data = (void*) priv;

    {
      GeglNode *gegl;
      gegl = operation->node;

      priv->input = gegl_node_get_input_proxy (gegl, "input");
      priv->output = gegl_node_get_output_proxy (gegl, "output");
      priv->dft_forward = gegl_node_new_child (gegl,
                                            "operation", "dft-forward",
                                            NULL);
      priv->glpf_filter = gegl_node_new_child (gegl,
					    "operation","gaussian-lowpass-filter",
					    NULL);
      priv->dft_backward = gegl_node_new_child (gegl,
                                            "operation", "dft-backward",
                                            NULL);

      gegl_node_link_many (priv->input, priv->dft_forward, priv->glpf_filter, 
			   priv->dft_backward, priv->output,NULL);
      gegl_operation_meta_redirect (operation, "cutoff", priv->glpf_filter, "cutoff");
      gegl_operation_meta_redirect (operation, "flag", priv->glpf_filter, "flag");
    }
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GObjectClass       *object_class;
  GeglOperationClass *operation_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->attach = attach;

  operation_class->name        = "gaussian-lowpass";
  operation_class->categories  = "meta:enhance";
  operation_class->description =
        "Performs an gaussian lowpass filter.";
}

#endif

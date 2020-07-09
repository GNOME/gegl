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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 *
 */

#ifdef GEGL_PROPERTIES

property_double (scaling, _("Scaling"), 1.0)
  description   (_("scaling factor of displacement, indicates how large spatial"
              " displacement a relative mapping value of 1.0 corresponds to."))
  value_range (0.0, 5000.0)

property_enum (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type, GEGL_SAMPLER_CUBIC)

property_enum (abyss_policy, _("Abyss policy"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_NONE)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     map_relative
#define GEGL_OP_C_SOURCE map-relative.c

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-op.h"


#define MAP_RELATIVE
#include "map-common.h"


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;
  gchar                      *composition = 
    "<gegl>"
    "<node operation='gegl:crop' width='200' height='200'/>"
    "<node operation='gegl:over'>"
      "<node operation='gegl:map-relative'>"
      "  <params>"
      "    <param name='scaling'>30</param>"
      "  </params>"
      "  <node operation='gegl:perlin-noise' />"
      "</node>"
      "<node operation='gegl:load' path='standard-input.png'/>"
    "</node>"
    "<node operation='gegl:checkerboard' color1='rgb(0.25,0.25,0.25)' color2='rgb(0.75,0.75,0.75)'/>"
    "</gegl>";


  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:map-relative",
    "title",       _("Map Relative"),
    "categories" , "map",
    "reference-hash", "c662bb6323771333ee49f7a30638eb22",
    "reference-hashB", "f2a0b3c8485ce7b8867dca7d1f567d58",
    "description", _("sample input with an auxiliary buffer that contain relative source coordinates"),
    "reference-composition", composition,
    NULL);
}
#endif

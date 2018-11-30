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
 * Copyright 2010 Michael Muré <batolettre@gmail.com>
 *
 */

#ifdef GEGL_PROPERTIES

property_enum (sampler_type, _("Resampling method"),
    GeglSamplerType, gegl_sampler_type,
    GEGL_SAMPLER_CUBIC)

property_enum (abyss_policy, _("Abyss policy"),
               GeglAbyssPolicy, gegl_abyss_policy,
               GEGL_ABYSS_NONE)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     map_absolute
#define GEGL_OP_C_SOURCE map-absolute.c

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-op.h"


#define MAP_ABSOLUTE
#include "map-common.h"


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;

  gegl_operation_class_set_keys (operation_class,
    "name",              "gegl:map-absolute",
    "title",              _("Map Absolute"),
    "categories",        "map",
    "position-dependent", "true",
    "description", _("sample input with an auxiliary buffer that contain absolute source coordinates"),
    NULL);
}
#endif

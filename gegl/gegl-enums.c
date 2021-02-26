/* This file is part of GEGL
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
 */

#include "config.h"

#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "gegl-enums.h"


GType
gegl_dither_method_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_DITHER_NONE,             N_("None"),             "none"             },
        { GEGL_DITHER_FLOYD_STEINBERG,  N_("Floyd-Steinberg"),  "floyd-steinberg"  },
        { GEGL_DITHER_BAYER,            N_("Bayer"),            "bayer"            },
        { GEGL_DITHER_RANDOM,           N_("Random"),           "random"           },
        { GEGL_DITHER_RANDOM_COVARIANT, N_("Random Covariant"), "random-covariant" },
        { GEGL_DITHER_ARITHMETIC_ADD,   N_("Arithmetic add"),   "add"  },
        { GEGL_DITHER_ARITHMETIC_ADD_COVARIANT,   N_("Arithmetic add covariant"),  "add-covariant"  },
        { GEGL_DITHER_ARITHMETIC_XOR,   N_("Arithmetic xor"),   "xor"  },
        { GEGL_DITHER_ARITHMETIC_XOR_COVARIANT,   N_("Arithmetic xor covariant"),  "xor-covariant"  },
        { GEGL_DITHER_BLUE_NOISE,   N_("Blue Noise"),  "blue-noise"  },
        { GEGL_DITHER_BLUE_NOISE_COVARIANT,   N_("Blue Noise Covariant"),  "blue-noise-covariant"  },

        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglDitherMethod", values);
    }

  return etype;
}

GType
gegl_distance_metric_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_DISTANCE_METRIC_EUCLIDEAN, N_("Euclidean"), "euclidean"  },
        { GEGL_DISTANCE_METRIC_MANHATTAN, N_("Manhattan"), "manhattan"  },
        { GEGL_DISTANCE_METRIC_CHEBYSHEV, N_("Chebyshev"), "chebyshev" },

        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglDistanceMetric", values);
    }

  return etype;
}

GType
gegl_orientation_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_ORIENTATION_HORIZONTAL, N_("Horizontal"), "horizontal" },
        { GEGL_ORIENTATION_VERTICAL,   N_("Vertical"),   "vertical"   },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglOrientation", values);
    }

  return etype;
}

GType
gegl_babl_variant_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_BABL_VARIANT_FLOAT, N_("Float"), "float"},
        { GEGL_BABL_VARIANT_LINEAR, N_("Linear"), "linear"},
        { GEGL_BABL_VARIANT_NONLINEAR, N_("Non-linear"), "non-linear"},
        { GEGL_BABL_VARIANT_PERCEPTUAL, N_("Perceptual"), "perceptual"},
        { GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED, N_("Linear-premultiplied"), "linear-premultiplied"},
        { GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED, N_("Perceptual-premultiplied"), "perceptual-premultiplied"},
        { GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED_IF_ALPHA, N_("Linear-premultiplied-if-alpha"), "linear-premultiplied-if-alpha"},
        { GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED_IF_ALPHA, N_("Perceptual-premultiplied-if-alpha"), "perceptual-premultiplied-if-alpha"},
        { GEGL_BABL_VARIANT_ALPHA, N_("add-alpha"), "add-alpha"},
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglBablVariant", values);
    }

  return etype;
}

GType
gegl_cache_policy_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_CACHE_POLICY_AUTO, N_("Auto"), "auto" },
        { GEGL_CACHE_POLICY_NEVER, N_("Never"), "never" },
        { GEGL_CACHE_POLICY_ALWAYS, N_("Always"), "always" },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglCachePolicy", values);
    }

  return etype;
}

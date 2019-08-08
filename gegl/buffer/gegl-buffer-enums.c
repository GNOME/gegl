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

#include "gegl-buffer-enums.h"

GType
gegl_abyss_policy_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_ABYSS_NONE,  N_("None"),  "none"  },
        { GEGL_ABYSS_CLAMP, N_("Clamp"), "clamp" },
        { GEGL_ABYSS_LOOP,  N_("Loop"),  "loop"  },
        { GEGL_ABYSS_BLACK, N_("Black"), "black" },
        { GEGL_ABYSS_WHITE, N_("White"), "white" },
        // we do not really want this one introspected/translated,
        // will this bite us?
        //{ GEGL_BUFFER_NEAREST, N_("Nearest"), "nearest" },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglAbyssPolicy", values);
    }

  return etype;
}

GType
gegl_access_mode_get_type (void)
{
  static GType ftype = 0;

  if (ftype == 0)
    {
      static GFlagsValue values[] = {
        { GEGL_ACCESS_READ,      N_("Read"),        "read"      },
        { GEGL_ACCESS_WRITE,     N_("Write"),       "write"     },
        { GEGL_ACCESS_READWRITE, N_("Read/Write"), "readwrite" },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      ftype = g_flags_register_static ("GeglAccessMode", values);
    }

  return ftype;
}

GType
gegl_sampler_type_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_SAMPLER_NEAREST, N_("Nearest"), "nearest" },
        { GEGL_SAMPLER_LINEAR,  N_("Linear"),  "linear"  },
        { GEGL_SAMPLER_CUBIC,   N_("Cubic"),   "cubic"   },
        { GEGL_SAMPLER_NOHALO,  N_("NoHalo"),  "nohalo"  },
        { GEGL_SAMPLER_LOHALO,  N_("LoHalo"),  "lohalo"  },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglSamplerType", values);
    }

  return etype;
}

GType
gegl_rectangle_alignment_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
    {
      static GEnumValue values[] = {
        { GEGL_RECTANGLE_ALIGNMENT_SUBSET,   N_("Subset"),   "subset"   },
        { GEGL_RECTANGLE_ALIGNMENT_SUPERSET, N_("Superset"), "superset" },
        { GEGL_RECTANGLE_ALIGNMENT_NEAREST,  N_("Nearest"),  "nearest"  },
        { 0, NULL, NULL }
      };
      gint i;

      for (i = 0; i < G_N_ELEMENTS (values); i++)
        if (values[i].value_name)
          values[i].value_name =
            dgettext (GETTEXT_PACKAGE, values[i].value_name);

      etype = g_enum_register_static ("GeglRectangleAlignment", values);
    }

  return etype;
}

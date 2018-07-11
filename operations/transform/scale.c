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
 *
 * Copyright 2018 Ell
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include <gegl.h>
#include <gegl-plugin.h>

#include "gegl-config.h"

#include "scale.h"
#include "module.h"

enum
{
  PROP_ABYSS_POLICY = 1
};

static void              gegl_scale_get_property     (GObject      *object,
                                                      guint         prop_id,
                                                      GValue       *value,
                                                      GParamSpec   *pspec);
static void              gegl_scale_set_property     (GObject      *object,
                                                      guint         prop_id,
                                                      const GValue *value,
                                                      GParamSpec   *pspec);

static GeglAbyssPolicy   gegl_scale_get_abyss_policy (OpTransform  *transform);

/* ************************* */

static void              op_scale_init               (OpScale      *self);
static void              op_scale_class_init         (OpScaleClass *klass);
static gpointer          op_scale_parent_class = NULL;

static void
op_scale_class_intern_init (gpointer klass)
{
  op_scale_parent_class = g_type_class_peek_parent (klass);
  op_scale_class_init ((OpScaleClass *) klass);
}

GType
op_scale_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpScaleClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_scale_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpScale),
          0,      /* n_preallocs */
          (GInstanceInitFunc) op_scale_init,
          NULL    /* value_table */
        };

      g_define_type_id =
        gegl_module_register_type (transform_module_get_module (),
                                   TYPE_OP_TRANSFORM,
                                   "GeglOpPlugIn-scale-core",
                                   &g_define_type_info, 0);
    }
  return g_define_type_id;
}

static void
op_scale_class_init (OpScaleClass *klass)
{
  GObjectClass     *gobject_class   = G_OBJECT_CLASS (klass);
  OpTransformClass *transform_class = OP_TRANSFORM_CLASS (klass);

  gobject_class->set_property       = gegl_scale_set_property;
  gobject_class->get_property       = gegl_scale_get_property;

  transform_class->get_abyss_policy = gegl_scale_get_abyss_policy;

  g_object_class_install_property (gobject_class, PROP_ABYSS_POLICY,
                                   g_param_spec_enum (
                                     "abyss-policy",
                                     _("Abyss policy"),
                                     _("How image edges are handled"),
                                     GEGL_TYPE_ABYSS_POLICY,
                                     GEGL_ABYSS_NONE,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
op_scale_init (OpScale *self)
{
}

static void
gegl_scale_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  OpScale *self = OP_SCALE (object);

  switch (prop_id)
    {
    case PROP_ABYSS_POLICY:
      g_value_set_enum (value, self->abyss_policy);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_scale_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  OpScale *self = OP_SCALE (object);

  switch (prop_id)
    {
    case PROP_ABYSS_POLICY:
      self->abyss_policy = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GeglAbyssPolicy
gegl_scale_get_abyss_policy (OpTransform *transform)
{
  return OP_SCALE (transform)->abyss_policy;
}

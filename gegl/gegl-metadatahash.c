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
 * Copyright 2020 Brian Stafford
 */

#include "gegl-metadatahash.h"

struct _GeglMetadataHash
  {
    GeglMetadataStore parent_instance;

    GHashTable *store;
  };

G_DEFINE_TYPE (GeglMetadataHash, gegl_metadata_hash, GEGL_TYPE_METADATA_STORE)

typedef struct _GeglMetadataValue GeglMetadataValue;
struct _GeglMetadataValue
  {
    GValue value;
    GParamSpec *pspec;
    gboolean shadow;
  };

/* GObject {{{1 */

GeglMetadataStore *
gegl_metadata_hash_new (void)
{
  return g_object_new (GEGL_TYPE_METADATA_HASH, NULL);
}

static void gegl_metadata_hash_finalize (GObject *object);

/* GObject {{{1 */

static void     gegl_metadata_hash_declare (GeglMetadataStore *store, GParamSpec *pspec, gboolean shadow);
static gboolean gegl_metadata_hash_has_value (GeglMetadataStore *self, const gchar *name);
static GParamSpec *gegl_metadata_hash_pspec (GeglMetadataStore *self, const gchar *name);
static void     gegl_metadata_hash_set_value (GeglMetadataStore *self, const gchar *name, const GValue *value);
static const GValue *gegl_metadata_hash_get_value (GeglMetadataStore *self, const gchar *name);

static void
gegl_metadata_hash_class_init (GeglMetadataHashClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GeglMetadataStoreClass *store_class = GEGL_METADATA_STORE_CLASS (class);

  gobject_class->finalize = gegl_metadata_hash_finalize;

  store_class->_declare = gegl_metadata_hash_declare;
  store_class->has_value = gegl_metadata_hash_has_value;
  store_class->pspec = gegl_metadata_hash_pspec;
  store_class->set_value = gegl_metadata_hash_set_value;
  store_class->_get_value = gegl_metadata_hash_get_value;
}

static void
metadata_value_free (gpointer data)
{
  GeglMetadataValue *meta = data;

  g_param_spec_unref (meta->pspec);
  g_value_unset (&meta->value);
}

static void
gegl_metadata_hash_init (GeglMetadataHash *self)
{
  self->store = g_hash_table_new_full (g_str_hash, g_str_equal,
                                       g_free, metadata_value_free);
}

static void
gegl_metadata_hash_finalize (GObject *object)
{
  GeglMetadataHash *self = (GeglMetadataHash *) object;

  g_hash_table_unref (self->store);
  G_OBJECT_CLASS (gegl_metadata_hash_parent_class)->finalize (object);
}

/* Declare metadata {{{1 */

static void
gegl_metadata_hash_declare (GeglMetadataStore *store,
                                      GParamSpec *pspec, gboolean shadow)
{
  GeglMetadataHash *self = (GeglMetadataHash *) store;
  GeglMetadataValue *meta;
  const gchar *name;

  meta = g_slice_new0 (GeglMetadataValue);
  meta->shadow = shadow;
  meta->pspec = pspec;

  name = g_param_spec_get_name (meta->pspec);
  g_hash_table_replace (self->store, g_strdup (name), meta);
}

/* Metadata accessors {{{1 */

static gboolean
gegl_metadata_hash_has_value (GeglMetadataStore *store, const gchar *name)
{
  GeglMetadataHash *self = (GeglMetadataHash *) store;
  GeglMetadataValue *meta;

  g_return_val_if_fail (GEGL_IS_METADATA_HASH (self), FALSE);

  meta = g_hash_table_lookup (self->store, name);
  return meta != NULL && G_IS_VALUE (&meta->value);
}

static GParamSpec *
gegl_metadata_hash_pspec (GeglMetadataStore *store, const gchar *name)
{
  GeglMetadataHash *self = (GeglMetadataHash *) store;
  GeglMetadataValue *meta;

  g_return_val_if_fail (GEGL_IS_METADATA_HASH (self), FALSE);

  meta = g_hash_table_lookup (self->store, name);
  return meta != NULL ? meta->pspec : NULL;
}

/* set/get by GValue */

static void
gegl_metadata_hash_set_value (GeglMetadataStore *store, const gchar *name,
                              const GValue *value)
{
  GeglMetadataHash *self = (GeglMetadataHash *) store;
  GeglMetadataValue *meta;
  gboolean success;

  g_return_if_fail (GEGL_IS_METADATA_HASH (self));

  meta = g_hash_table_lookup (self->store, name);
  g_return_if_fail (meta != NULL);

  if (!G_IS_VALUE (&meta->value))
    g_value_init (&meta->value, G_PARAM_SPEC_VALUE_TYPE (meta->pspec));

  if (value != NULL)
    success = g_param_value_convert (meta->pspec, value, &meta->value, FALSE);
  else
    {
      g_param_value_set_default (meta->pspec, &meta->value);
      success = TRUE;
    }
  if (success)
    gegl_metadata_store_notify (store, meta->pspec, meta->shadow);
}

static const GValue *
gegl_metadata_hash_get_value (GeglMetadataStore *store, const gchar *name)
{
  GeglMetadataHash *self = (GeglMetadataHash *) store;
  GeglMetadataValue *meta;

  g_return_val_if_fail (GEGL_IS_METADATA_HASH (self), NULL);

  meta = g_hash_table_lookup (self->store, name);
  return meta != NULL && G_IS_VALUE (&meta->value) ? &meta->value : NULL;
}

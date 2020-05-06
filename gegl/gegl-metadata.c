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

#include "gegl-metadata.h"

G_DEFINE_INTERFACE (GeglMetadata, gegl_metadata, G_TYPE_OBJECT)

static void
gegl_metadata_default_init (G_GNUC_UNUSED GeglMetadataInterface *iface)
{
}

void
gegl_metadata_register_map (GeglMetadata *metadata, const gchar *file_module,
                            guint flags, const GeglMetadataMap *map, gsize n_map)
{
  GeglMetadataInterface *iface;

  g_return_if_fail (GEGL_IS_METADATA (metadata));

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_if_fail (iface->register_map != NULL);
  return (*iface->register_map) (metadata, file_module, flags, map, n_map);
}

void
gegl_metadata_unregister_map (GeglMetadata *metadata)
{
  GeglMetadataInterface *iface;

  g_return_if_fail (GEGL_IS_METADATA (metadata));

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_if_fail (iface->register_map != NULL);
  return (*iface->register_map) (metadata, NULL, 0, NULL, 0);
}

gboolean
gegl_metadata_set_resolution (GeglMetadata *metadata, GeglResolutionUnit unit,
                              gfloat x, gfloat y)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->set_resolution != NULL, FALSE);
  return (*iface->set_resolution) (metadata, unit, x, y);
}

gboolean
gegl_metadata_get_resolution (GeglMetadata *metadata, GeglResolutionUnit *unit,
                              gfloat *x, gfloat *y)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->get_resolution != NULL, FALSE);
  return (*iface->get_resolution) (metadata, unit, x, y);
}

gboolean
gegl_metadata_iter_lookup (GeglMetadata *metadata, GeglMetadataIter *iter,
                           const gchar *key)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->iter_lookup != NULL, FALSE);
  return (*iface->iter_lookup) (metadata, iter, key);
}

void
gegl_metadata_iter_init (GeglMetadata *metadata, GeglMetadataIter *iter)
{
  GeglMetadataInterface *iface;

  g_return_if_fail (GEGL_IS_METADATA (metadata));

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_if_fail (iface->iter_init != NULL);
  (*iface->iter_init) (metadata, iter);
}

const gchar *
gegl_metadata_iter_next (GeglMetadata *metadata, GeglMetadataIter *iter)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->iter_next != NULL, FALSE);
  return (*iface->iter_next) (metadata, iter);
}

gboolean
gegl_metadata_iter_set_value (GeglMetadata *metadata, GeglMetadataIter *iter,
                              const GValue *value)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->iter_set_value != NULL, FALSE);
  return (*iface->iter_set_value) (metadata, iter, value);
}

gboolean
gegl_metadata_iter_get_value (GeglMetadata *metadata, GeglMetadataIter *iter,
                              GValue *value)
{
  GeglMetadataInterface *iface;

  g_return_val_if_fail (GEGL_IS_METADATA (metadata), FALSE);

  iface = GEGL_METADATA_GET_IFACE (metadata);
  g_return_val_if_fail (iface->iter_get_value != NULL, FALSE);
  return (*iface->iter_get_value) (metadata, iter, value);
}

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

#ifndef _geglmetadatahash_h
#define _geglmetadatahash_h

#include <glib.h>
#include "gegl-metadatastore.h"

G_BEGIN_DECLS

#define GEGL_TYPE_METADATA_HASH   gegl_metadata_hash_get_type ()
G_DECLARE_FINAL_TYPE (
        GeglMetadataHash,
        gegl_metadata_hash,
        GEGL, METADATA_HASH,
        GeglMetadataStore
)

/**
 * SECTION:gegl-metadatahash
 * @title: GeglMetadataHash
 * @short_description: A metadata store object for use with file modules.
 * @see_also: #GeglMetadata #GeglMetadataStore
 *
 * #GeglMetadataHash is a #GeglMetadataStore implementing the data store using
 * a hash table. It adds no new methods or properties to #GeglMetadataStore.
 */

/**
 * gegl_metadata_hash_new:
 *
 * Create a new #GeglMetadataHash
 *
 * Returns: (transfer full): New #GeglMetadataHash cast to #GeglMetadataStore
 */
GeglMetadataStore *       gegl_metadata_hash_new        (void);

G_END_DECLS

#endif

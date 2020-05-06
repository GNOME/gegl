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

#ifndef __GEGL_METADATA_H__
#define __GEGL_METADATA_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * SECTION:gegl-metadata
 * @title: GeglMetadata
 * @short_description: A metadata interface for use with file modules.
 * @see_also: #GeglMetadataStore #GeglMetadataHash
 *
 * Objects which need to store or retrieve image metadata when saving and
 * loading image files should implement GeglMetadata. The object should be cast
 * with GEGL_METADATA() and passed to the file load or save module via the
 * `metadata` property. Image file modules should not implement the metadata
 * property if either the module or file format does not support metadata.
 *
 * Gegl understands (but is not limited to) the following well-known metadata
 * variables:
 *
 * - artist: Name of image creator.
 * - comment: Miscellaneous comment; conversion from GIF comment.
 * - copyright: Copyright notice.
 * - description: Description of image (possibly long).
 * - disclaimer: Legal disclaimer.
 * - software: Software used to create the image.
 * - source: Device used to create the image.
 * - timestamp: Time of original image creation.
 * - title: Short (one line) title or caption for image.
 * - warning: Warning of nature of content.
 *
 * The Gegl Metadata subsystem can be used in one of three ways described
 * below in order of increasing complexity:
 *
 * 1. Recommended: Create a #GeglMetadataHash and pass it to a file loader
 *    or saver via its `metadata` property. #GeglMetadataHash is a subclass of
 *    #GeglMetadataStore which saves metadata in a hash table but which adds no
 *    new properties or methods.  Image file metadata to be retrieved or saved
 *    is accessed via #GeglMetadataStore properties or methods. Metadata values
 *    not directly supported by Gegl may be declared using a #GParamSpec.
 * 2. Subclass #GeglMetadataStore. This may be useful if an application stores
 *    metadata in internal structures which may be accessed via the subclass.
 *    The subclass is used identically to #GeglMetadataHash.
 *    #GeglMetadataStore aims to be sufficiently flexible to cover the majority
 *    of application requirements.
 * 3. Implement the #GeglMetadata interface. This option should only be used if
 *    #GeglMetadataStore cannot adequately satisfy application requirements.
 *    Particular attention should be paid to semantics of the interface methods
 *    as the file modules interact directly with these.
 *
 * For more complex requirements than provided by the metadata subsystem it is
 * probably better to use a library such as `exiv2` or similar.
 */

/**
 * GeglResolutionUnit:
 * @GEGL_RESOLUTION_UNIT_NONE: Unknown or resolution not applicable.
 * @GEGL_RESOLUTION_UNIT_DPI: Dots or pixels per inch.
 * @GEGL_RESOLUTION_UNIT_DPM: Dots or pixels per metre.
 *
 * An enumerated type specifying resolution (density) units.  If resolution
 * units are unknown, X and Y resolution specify the pixel aspect ratio.
 */
typedef enum
  {
    GEGL_RESOLUTION_UNIT_NONE,
    GEGL_RESOLUTION_UNIT_DPI,
    GEGL_RESOLUTION_UNIT_DPM
  }
GeglResolutionUnit;

/**
 * GeglMapFlags:
 * @GEGL_MAP_EXCLUDE_UNMAPPED: Prevent further mapping from being registered.
 *
 * Flags controlling the mapping strategy.
 */
typedef enum _GeglMapFlags
  {
    GEGL_MAP_EXCLUDE_UNMAPPED = 1
  }
GeglMapFlags;


#define GEGL_TYPE_METADATA          (gegl_metadata_get_type ())
G_DECLARE_INTERFACE (GeglMetadata, gegl_metadata, GEGL, METADATA, GObject)

/**
 * GeglMetadataMap:
 * @local_name: Name of metadata variable used in the file module.
 * @name: Standard metadata variable name used by Gegl.
 * @transform: Optional #GValue transform function.
 *
 * Struct to describe how a metadata variable is mapped from the name used by
 * the image file module to the name used by Gegl.  An optional transform
 * function may be specified, e.g. to transform from a #GDatetime to a string.
 */
typedef struct _GeglMetadataMap {
  const gchar *local_name;
  const gchar *name;
  GValueTransform transform;
} GeglMetadataMap;

/**
 * GeglMetadataIter:
 *
 * An opaque type representing a metadata iterator.
 */
typedef struct {
  /*< private >*/
  guint       stamp;
  gpointer    user_data;
  gpointer    user_data2;
  gpointer    user_data3;
} GeglMetadataIter;

/**
 * GeglMetadataInterface:
 * @register_map: See gegl_metadata_register_map().  If called with a NULL map,
 * the registration is deleted.
 * @set_resolution: See gegl_metadata_set_resolution().
 * @get_resolution: See gegl_metadata_get_resolution().
 * @iter_lookup: See gegl_metadata_iter_lookup().
 * @iter_init: See gegl_metadata_iter_init().
 * @iter_next: See gegl_metadata_iter_next().
 * @iter_set_value: See gegl_metadata_iter_set_value().
 * @iter_get_value: See gegl_metadata_iter_get_value().
 *
 * The #GeglMetadata interface structure.
 */
struct _GeglMetadataInterface
{
  /*< private >*/
  GTypeInterface base_iface;

  void        (*register_map)         (GeglMetadata *metadata,
                                       const gchar *file_module,
                                       guint flags,
                                       const GeglMetadataMap *map,
                                       gsize n_map);

  /*< public >*/
  gboolean    (*set_resolution)       (GeglMetadata *metadata,
                                       GeglResolutionUnit unit,
                                       gfloat x, gfloat y);
  gboolean    (*get_resolution)       (GeglMetadata *metadata,
                                       GeglResolutionUnit *unit,
                                       gfloat *x, gfloat *y);

  gboolean    (*iter_lookup)          (GeglMetadata *metadata,
                                       GeglMetadataIter *iter,
                                       const gchar *key);
  void        (*iter_init)            (GeglMetadata *metadata,
                                       GeglMetadataIter *iter);
  const gchar *(*iter_next)           (GeglMetadata *metadata,
                                       GeglMetadataIter *iter);
  gboolean    (*iter_set_value)       (GeglMetadata *metadata,
                                       GeglMetadataIter *iter,
                                       const GValue *value);
  gboolean    (*iter_get_value)       (GeglMetadata *metadata,
                                       GeglMetadataIter *iter,
                                       GValue *value);
};

/**
 * gegl_metadata_register_map:
 * @metadata:   The #GeglMetadata interface
 * @file_module: String identifying the file module, e.g, `"gegl:png-save"`
 * @flags:      Flags specifying capabilities of underlying file format
 * @map: (array length=n_map): Array of mappings from file module metadata
 *              names to Gegl well-known names.
 * @n_map:      Number of entries in @map
 *
 * Set the name of the file module and pass an array of mappings from
 * file-format specific metadata names to those used by Gegl. A GValue
 * transformation function may be supplied, e.g. to parse or format timestamps.
 */
void            gegl_metadata_register_map      (GeglMetadata *metadata,
                                                 const gchar *file_module,
                                                 guint flags,
                                                 const GeglMetadataMap *map,
                                                 gsize n_map);

/**
 * gegl_metadata_unregister_map:
 * @metadata:   The #GeglMetadata interface
 *
 * Unregister the file module mappings and any further mappings added or
 * modified by the application.  This should be called after the file module
 * completes operations.
 */
void            gegl_metadata_unregister_map    (GeglMetadata *metadata);

/**
 * gegl_metadata_set_resolution:
 * @metadata:   The #GeglMetadata interface
 * @unit:       Specify #GeglResolutionUnit
 * @x:          X resolution
 * @y:          Y resolution
 *
 * Set resolution retrieved from image file's metadata.  Intended for use by
 * the image file reader.  If resolution is not supported by the application or
 * if the operation fails %FALSE is returned and the values are ignored.
 *
 * Returns:     %TRUE if successful.
 */
gboolean        gegl_metadata_set_resolution    (GeglMetadata *metadata,
                                                 GeglResolutionUnit unit,
                                                 gfloat x, gfloat y);

/**
 * gegl_metadata_get_resolution:
 * @metadata:   The #GeglMetadata interface
 * @unit:       #GeglResolutionUnit return location
 * @x:          X resolution return location
 * @y:          Y resolution return location
 *
 * Retrieve resolution from the application image metadata.  Intended for use
 * by the image file writer.  If resolution is not supported by the application
 * or if the operation fails %FALSE is returned and the resolution values are
 * not updated.
 *
 * Returns:     %TRUE if successful.
 */
gboolean        gegl_metadata_get_resolution    (GeglMetadata *metadata,
                                                 GeglResolutionUnit *unit,
                                                 gfloat *x, gfloat *y);

/**
 * gegl_metadata_iter_lookup:
 * @metadata:   The #GeglMetadata interface
 * @iter:       #GeglMetadataIter to be initialised
 * @key:        Name of the value look up
 *
 * Look up the specified key and initialise an iterator to reference the
 * associated metadata. The iterator is used in conjunction with
 * gegl_metadata_set_value() and gegl_metadata_get_value(). Note that this
 * iterator is not valid for gegl_metadata_iter_next().
 *
 * Returns:     %TRUE if key is found.
 */
gboolean        gegl_metadata_iter_lookup       (GeglMetadata *metadata,
                                                 GeglMetadataIter *iter,
                                                 const gchar *key);

/**
 * gegl_metadata_iter_init:
 * @metadata:   The #GeglMetadata interface
 * @iter:       #GeglMetadataIter to be initialised
 *
 * Initialise an iterator to find all supported metadata keys.
 */
void            gegl_metadata_iter_init         (GeglMetadata *metadata,
                                                 GeglMetadataIter *iter);

/**
 * gegl_metadata_iter_next:
 * @metadata:   The #GeglMetadata interface
 * @iter:       #GeglMetadataIter to be updated
 *
 * Move the iterator to the next metadata item
 *
 * Returns:     key name if found, else %NULL
 */
const gchar    *gegl_metadata_iter_next         (GeglMetadata *metadata,
                                                 GeglMetadataIter *iter);

/**
 * gegl_metadata_iter_set_value:
 * @metadata:   The #GeglMetadata interface
 * @iter:       #GeglMetadataIter referencing the value to set
 * @value:      Value to set in the interface
 *
 * Set application data retrieved from image file's metadata.  Intended for use
 * by the image file reader.  If the operation fails it returns %FALSE and
 * @value is ignored.
 *
 * Returns:     %TRUE if successful.
 */
gboolean        gegl_metadata_iter_set_value    (GeglMetadata *metadata,
                                                 GeglMetadataIter *iter,
                                                 const GValue *value);

/**
 * gegl_metadata_iter_get_value:
 * @metadata:   The #GeglMetadata interface
 * @iter:       #GeglMetadataIter referencing the value to get
 * @value:      Value to set in the interface
 *
 * Retrieve image file metadata from the application.  Intended for use by the
 * image file writer. If the operation fails it returns %FALSE and @value is
 * not updated.
 *
 * Returns:     %TRUE if successful.
 */
gboolean        gegl_metadata_iter_get_value    (GeglMetadata *metadata,
                                                 GeglMetadataIter *iter,
                                                 GValue *value);

G_END_DECLS

#endif

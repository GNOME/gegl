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

#ifndef __GEGL_METADATA_STORE_H__
#define __GEGL_METADATA_STORE_H__

#include <glib.h>
#include "gegl-metadata.h"

G_BEGIN_DECLS

#define GEGL_TYPE_METADATA_STORE   gegl_metadata_store_get_type ()
G_DECLARE_DERIVABLE_TYPE (
        GeglMetadataStore,
        gegl_metadata_store,
        GEGL, METADATA_STORE,
        GObject
)

/**
 * SECTION:gegl-metadatastore
 * @title: GeglMetadataStore
 * @short_description: A metadata store base class for use with file modules.
 * @see_also: #GeglMetadata #GeglMetadataHash
 *
 * #GeglMetadataStore is a non-instantiable base class implementing the
 * #GeglMetadata interface and provides methods for metadata access using
 * well-known names.  For consistency with other #GObject features, the naming
 * convention for metadata variables is the same as for GObject properties.
 *
 * Methods are provided allowing the application to test whether a particular
 * metadata item has a value and to set or get the values. If a metadata value
 * does not exist, a GLib warning is printed. The
 * gegl_metadata_store_has_value() method can be used to test silently for
 * unset variables.
 *
 * Signals are provided to allow an application to intercept metadata values
 * from file modules, for example a Jpeg comment block might be parsed to set
 * multiple metadata values, or multiple values may be formatted into the
 * comment block.
 *
 * Image resolution and resolution units are accessible only as properties.
 * Well-known metatdata values are shadowed by properties to allow applications
 * to take advantage of features such as introspection and property binding.
 *
 * #GeglMetadataStore does not itself implement the storage mechanism, it must
 * be subclassed to provide this. #GeglMetadataHash implements a store using a
 * hash table.  For convenience gegl_metadata_hash_new() casts its return value
 * to #GeglMetadataStore as it does not add any new methods or properties.
 */

/**
 * GeglMetadataStoreClass:
 * @_declare: The _declare virtual method creates a metadata variable in the
 * underlying data store. It implements gegl_metadata_store_declare(). A
 * #GParamSpec is used to describe the variable.  If the metadata shadows an
 * object property, shadow should be %TRUE, otherwise %FALSE.  It is acceptable
 * for a subclass to provide additional variables which are implicitly
 * declared, that is, they need not be declared using
 * gegl_metadata_store_declare(), however the @pspec method must still retrieve
 * a #GParamSpec describing such variables.  This method MUST be provided by
 * the subclass.
 * @pspec: The pspec virtual method returns the #GParamSpec used to declare a
 * metadata variable. It is used to implement
 * gegl_metadata_store_typeof_value(). This method MUST be provided by the
 * subclass.
 * @has_value: The has_value virtual method implements
 * gegl_metadata_store_has_value() It should return %TRUE if the variable is
 * declared and contains a valid value of the correct type, otherwise %FALSE.
 * This method MUST be provided by the subclass.
 * @set_value: Set a metadata variable using a #GValue. Implements
 * gegl_metadata_store_set_value().  The metadata variable should be declared
 * and the #GValue must be of the correct type.  Note that failure to set a
 * variable may be dependent of properties of the underlying storage mechanism.
 * This method MUST be provided by the subclass.
 * @_get_value: Return a pointer to a #GValue with the value of the metadata
 * variable or %NULL if not declared or the variable does not contain a valid
 * value.  Implements gegl_metadata_store_get_value().  This method MUST be
 * provided by the subclass.
 * @register_hook: This method is called after a file loader or saver registers
 * a #GeglMetadataMap and before any further processing takes place.  It is
 * intended to allow an application to create further application-specific
 * mappings using gegl_metadata_store_register().  #GeglMetadataStore provides
 * a default method which emits the `::mapped` signal.
 * @parse_value: This method is called to optionally parse image file metadata
 * prior to setting metadata variables in the #GeglMetadataStore. If no parser
 * is available it returns %FALSE and the registered mapping is used.  If a
 * parser available it should set one or more metadata variables using
 * gegl_metadata_store_set_value() and return %TRUE. Note that the parser MUST
 * return %TRUE even if setting individual values fails.  The default method
 * checks if a signal handler is registered for the parse-value signal with
 * the variable name as the detail parameter. If a handler is registered it
 * emits the signal with the file metadata provided as a #GValue and returns
 * %TRUE otherwise %FALSE.
 * @generate_value: This method is called to optionally generate a value to be
 * written to and image file. If no generator is available it returns %FALSE
 * and the registered mapping is used. If a generator is available it should
 * create a suitable value to be written to the image file and return %TRUE.
 * The default method checks if a signal handler is registered for the
 * generate-value signal with the variable name as the detail parameter. If a
 * handler is registered it emits the signal with an initialised #GValue to
 * receive the file metadata and returns %TRUE otherwise %FALSE.  @parse_value
 * and @generate_value are provided to handle the case where some file formats
 * overload, for example, image comments. A typical case is formatting many
 * values into a TIFF file's ImageDescription field.
 *
 * The class structure for the #GeglMetadataStore
 */
struct _GeglMetadataStoreClass
{
  /*< private >*/
  GObjectClass  parent_class;

  /*< public >*/
  /* Subclass MUST provide the following */
  void          (*_declare)       (GeglMetadataStore *self,
                                   GParamSpec *pspec,
                                   gboolean shadow);
  GParamSpec   *(*pspec)          (GeglMetadataStore *self,
                                   const gchar *name);
  void          (*set_value)      (GeglMetadataStore *self,
                                   const gchar *name,
                                   const GValue *value);
  const GValue *(*_get_value)     (GeglMetadataStore *self,
                                   const gchar *name);
  gboolean      (*has_value)      (GeglMetadataStore *self,
                                   const gchar *name);

  /* Subclass MAY provide the following */
  void          (*register_hook)  (GeglMetadataStore *self,
                                   const gchar *file_module_name,
                                   guint flags);
  gboolean      (*parse_value)    (GeglMetadataStore *self,
                                   GParamSpec *pspec,
                                   GValueTransform transform,
                                   const GValue *value);
  gboolean      (*generate_value) (GeglMetadataStore *self,
                                   GParamSpec *pspec,
                                   GValueTransform transform,
                                   GValue *value);

  /*< private >*/
  gpointer      padding[4];
};

/**
 * GeglMetadataStore::changed:
 * @self: The #GeglMetadataStore emitting the signal
 * @pspec: A #GParamSpec declaring the metadata value
 *
 * `::changed` is emitted when a metadata value is changed. This is analogous
 * to the `GObject::notify` signal.
 */

/**
 * GeglMetadataStore::mapped:
 * @self: The #GeglMetadataStore emitting the signal
 * @file_module: The file module name
 * @exclude_unmapped: %TRUE if the file module cannot handle unmapped values
 *
 * `::mapped` is emitted after a file module registers a mapping and before
 * other processing takes place.  An application may respond to the signal by
 * registering additional mappings or overriding existing values, for example
 * it might override the TIFF ImageDescription tag to format multiple metadata
 * values into the description.
 */

/**
 * GeglMetadataStore::unmapped:
 * @self: The #GeglMetadataStore emitting the signal
 * @file_module: The file module name
 * @local_name: The unmapped metadata name as used by the file module
 *
 * `::unmapped` is emitted when a file module tries to look up an unmapped
 * metadata name. When the handler returns a second attempt is made to look
 * up the metadata.
 */

/**
 * GeglMetadataStore::generate-value:
 * @self: The #GeglMetadataStore emitting the signal
 * @pspec: A #GParamSpec declaring the metadata value
 * @value: (inout): An initialised #GValue.
 *
 * If a signal handler is connected to `::generate-value` a signal is emitted
 * when the file module accesses a value using gegl_metadata_get_value().
 * The signal handler must generate a value of the type specified in the pspec
 * argument. The signal handler's return value indicates the success of the
 * operation.
 *
 * If no handler is connected the mapped metadata value is accessed normally,
 *
 * Returns: %TRUE if a value is generated successfully.
 */

/**
 * GeglMetadataStore::parse-value:
 * @self: The #GeglMetadataStore emitting the signal
 * @pspec: A #GParamSpec declaring the metadata value
 * @value: (inout): A #GValue containing the value to parse.
 *
 * If a signal handler is connected to `::parse-value` a signal is emitted when
 * the file module accesses a value using gegl_metadata_set_value().  The
 * signal handler should parse the value supplied in the #GValue and may set
 * any number of metadata values using gegl_metadata_store_set_value().
 *
 * If no handler is connected the mapped metadata value is set normally,
 *
 * Returns: %TRUE if parsing is successful.
 */

/**
 * GeglMetadataStore:resolution-unit:
 *
 * A #GeglResolutionUnit specifying units for the image resolution (density).
 */

/**
 * gegl_metadata_store_set_resolution_unit:
 * @self: A #GeglMetadataStore
 * @unit: Units as a #GeglResolutionUnit
 *
 * Set the units used for the resolution (density) values.
 */
void          gegl_metadata_store_set_resolution_unit
                                                (GeglMetadataStore *self,
                                                 GeglResolutionUnit unit);

/**
 * gegl_metadata_store_get_resolution_unit:
 * @self: A #GeglMetadataStore
 *
 * Get the units used for resolution.
 *
 * Returns: a #GeglResolutionUnit.
 */
GeglResolutionUnit
              gegl_metadata_store_get_resolution_unit
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:resolution-x:
 *
 * X resolution or density in dots per unit.
 */

/**
 * gegl_metadata_store_set_resolution_x:
 * @self: A #GeglMetadataStore
 * @resolution_x: X resolution or density
 *
 * Set the X resolution or density in dots per unit.
 */
void          gegl_metadata_store_set_resolution_x
                                                (GeglMetadataStore *self,
                                                 gdouble resolution_x);

/**
 * gegl_metadata_store_get_resolution_x:
 * @self: A #GeglMetadataStore
 *
 * Get the X resolution or density in dots per unit.
 *
 * Returns: X resolution
 */
gdouble       gegl_metadata_store_get_resolution_x
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:resolution-y:
 *
 * Y resolution or density in dots per unit.
 */

/**
 * gegl_metadata_store_set_resolution_y:
 * @self: A #GeglMetadataStore
 * @resolution_y: Y resolution or density
 *
 * Set the Y resolution or density in dots per unit.
 */
void          gegl_metadata_store_set_resolution_y
                                                (GeglMetadataStore *self,
                                                 gdouble resolution_y);

/**
 * gegl_metadata_store_get_resolution_y:
 * @self: A #GeglMetadataStore
 *
 * Get the Y resolution or density in dots per unit.
 *
 * Returns: Y resolution
 */
gdouble       gegl_metadata_store_get_resolution_y
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:file-module-name:
 *
 * Current file loader/saver module name. Valid only while a #GeglMetadata
 * mapping is registered. This property is mainly provided for use in signal
 * handlers.
 */

/**
 * gegl_metadata_store_get_file_module_name:
 * @self: A #GeglMetadataStore
 *
 * Return the name registered by the current file module.
 *
 * Returns: (transfer none): Current file module name or %NULL.
 */
const gchar * gegl_metadata_store_get_file_module_name
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:title:
 *
 * Short (one line) title or caption for image.
 */

/**
 * gegl_metadata_store_set_title:
 * @self: A #GeglMetadataStore
 * @title: Title string
 *
 * Set title or caption for image.
 */
void          gegl_metadata_store_set_title    (GeglMetadataStore *self,
                                                const gchar *title);

/**
 * gegl_metadata_store_get_title:
 * @self: A #GeglMetadataStore
 *
 * Get title or caption for image.
 *
 * Returns: (transfer none): Title or %NULL if not set
 */
const gchar * gegl_metadata_store_get_title    (GeglMetadataStore *self);

/**
 * GeglMetadataStore:artist:
 *
 * Name of image creator.
 */

/**
 * gegl_metadata_store_set_artist:
 * @self: A #GeglMetadataStore
 * @artist: Artist string
 *
 * Set name of image creator.
 */
void          gegl_metadata_store_set_artist    (GeglMetadataStore *self,
                                                 const gchar *artist);

/**
 * gegl_metadata_store_get_artist:
 * @self: A #GeglMetadataStore
 *
 * Get name of image creator.
 *
 * Returns: (transfer none): Artist or %NULL if not set
 */
const gchar * gegl_metadata_store_get_artist    (GeglMetadataStore *self);

/**
 * GeglMetadataStore:description:
 *
 * Description of image (possibly long).
 */

/**
 * gegl_metadata_store_set_description:
 * @self: A #GeglMetadataStore
 * @description: Description string
 *
 * Set description of image.
 */
void           gegl_metadata_store_set_description
                                                (GeglMetadataStore *self,
                                                 const gchar *description);

/**
 * gegl_metadata_store_get_description:
 * @self: A #GeglMetadataStore
 *
 * Get description of image.
 *
 * Returns: (transfer none): Description or %NULL if not set
 */
const gchar * gegl_metadata_store_get_description
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:copyright:
 *
 * Copyright notice.
 */

/**
 * gegl_metadata_store_set_copyright:
 * @self: A #GeglMetadataStore
 * @copyright: Copyright string
 *
 * Set the copyright notice.
 */
void           gegl_metadata_store_set_copyright
                                                (GeglMetadataStore *self,
                                                 const gchar *copyright);

/**
 * gegl_metadata_store_get_copyright:
 * @self: A #GeglMetadataStore
 *
 * Get the copyright notice.
 *
 * Returns: (transfer none): Copyright or %NULL if not set
 */
const gchar * gegl_metadata_store_get_copyright
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:disclaimer:
 *
 * Legal disclaimer.
 */

/**
 * gegl_metadata_store_set_disclaimer:
 * @self: A #GeglMetadataStore
 * @disclaimer: Disclaimer string
 *
 * Set the legal disclaimer.
 */
void            gegl_metadata_store_set_disclaimer
                                                (GeglMetadataStore *self,
                                                 const gchar *disclaimer);

/**
 * gegl_metadata_store_get_disclaimer:
 * @self: A #GeglMetadataStore
 *
 * Get the legal disclaimer.
 *
 * Returns: (transfer none): Disclaimer or %NULL if not set
 */
const gchar * gegl_metadata_store_get_disclaimer
                                                (GeglMetadataStore *self);

/**
 * GeglMetadataStore:warning:
 *
 * Warning of nature of content.
 */

/**
 * gegl_metadata_store_set_warning:
 * @self: A #GeglMetadataStore
 * @warning: Warning string
 *
 * Set the warning of nature of content.
 */
void          gegl_metadata_store_set_warning   (GeglMetadataStore *self,
                                                 const gchar *warning);

/**
 * gegl_metadata_store_get_warning:
 * @self: A #GeglMetadataStore
 *
 * Get warning.
 *
 * Returns: (transfer none): Warning or %NULL if not set
 */
const gchar * gegl_metadata_store_get_warning   (GeglMetadataStore *self);

/**
 * GeglMetadataStore:comment:
 *
 * Miscellaneous comment; conversion from GIF comment.
 */

/**
 * gegl_metadata_store_set_comment:
 * @self: A #GeglMetadataStore
 * @comment: Comment string
 *
 * Set the miscellaneous comment; conversion from GIF comment.
 */
void          gegl_metadata_store_set_comment   (GeglMetadataStore *self,
                                                 const gchar *comment);

/**
 * gegl_metadata_store_get_comment:
 * @self: A #GeglMetadataStore
 *
 * Get the comment.
 *
 * Returns: (transfer none): Comment or %NULL if not set
 */
const gchar * gegl_metadata_store_get_comment   (GeglMetadataStore *self);

/**
 * GeglMetadataStore:software:
 *
 * Software used to create the image.
 */

/**
 * gegl_metadata_store_set_software:
 * @self: A #GeglMetadataStore
 * @software: Software string
 *
 * Set software used to create the image.
 */
void          gegl_metadata_store_set_software  (GeglMetadataStore *self,
                                                 const gchar *software);

/**
 * gegl_metadata_store_get_software:
 * @self: A #GeglMetadataStore
 *
 * Get software used to create the image.
 *
 * Returns: (transfer none): Software or %NULL if not set
 */
const gchar * gegl_metadata_store_get_software  (GeglMetadataStore *self);

/**
 * GeglMetadataStore:source:
 *
 * Device used to create the image.
 */

/**
 * gegl_metadata_store_set_source:
 * @self: A #GeglMetadataStore
 * @source: Source string
 *
 * Set device used to create the image.
 */
void            gegl_metadata_store_set_source  (GeglMetadataStore *self,
                                                 const gchar *source);

/**
 * gegl_metadata_store_get_source:
 * @self: A #GeglMetadataStore
 *
 * Get device used to create the image.
 *
 * Returns: (transfer none): source or %NULL if not set
 */
const gchar * gegl_metadata_store_get_source    (GeglMetadataStore *self);

/**
 * GeglMetadataStore:timestamp:
 *
 * Time of original image creation.
 */

/**
 * gegl_metadata_store_set_timestamp:
 * @self: A #GeglMetadataStore
 * @timestamp: A #GDateTime
 *
 * Set time of original image creation.
 */
void          gegl_metadata_store_set_timestamp (GeglMetadataStore *self,
                                                 const GDateTime *timestamp);

/**
 * gegl_metadata_store_get_timestamp:
 * @self: A #GeglMetadataStore
 *
 * Get time of original image creation.
 *
 * Returns: (transfer full): #GDateTime or %NULL if not set. Free with
 *                           g_date_time_unref() when done.
 */
GDateTime *   gegl_metadata_store_get_timestamp (GeglMetadataStore *self);

/**
 * gegl_metadata_store_declare:
 * @self: A #GeglMetadataStore
 * @pspec: (transfer none): A #GParamSpec
 *
 * Declare a metadata value using a #GParamSpec.
 */
void          gegl_metadata_store_declare       (GeglMetadataStore *self,
                                                 GParamSpec *pspec);

/**
 * gegl_metadata_store_has_value:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 *
 * Test whether the #GeglMetadataStore contains a value for the specified name.
 *
 * Returns: %TRUE if metadata is declared and contains a valid value.
 */
gboolean      gegl_metadata_store_has_value     (GeglMetadataStore *self,
                                                 const gchar *name);

/**
 * gegl_metadata_store_typeof_value:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 *
 * Get the declared type of the value in the #GeglMetadataStore.
 *
 * Returns: Declared #GType of metadata value or %G_TYPE_INVALID.
 */
GType         gegl_metadata_store_typeof_value  (GeglMetadataStore *self,
                                                 const gchar *name);

/**
 * gegl_metadata_store_set_value:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 * @value: (in): (nullable): A valid #GValue or %NULL
 *
 * Set the specified metadata value. If @value is %NULL the default value from
 * the associated #GParamSpec is used. This operation will fail if the value
 * has not been previously declared.  A `changed::name` signal is emitted when
 * the value is set. If the value is shadowed by a property a `notify::name`
 * signal is also emitted.
 */
void          gegl_metadata_store_set_value     (GeglMetadataStore *self,
                                                 const gchar *name,
                                                 const GValue *value);

/**
 * gegl_metadata_store_get_value:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 * @value: (inout): An initialised #GValue.
 *
 * Retrieve the metadata value. @value must be initialised with a compatible
 * type. If the value is unset or has not been previously declared @value is
 * unchanged and an error message is logged.
 */
void          gegl_metadata_store_get_value     (GeglMetadataStore *self,
                                                 const gchar *name,
                                                 GValue *value);

/**
 * gegl_metadata_store_set_string:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 * @string: String value to set
 *
 * A slightly more efficient version of gegl_metadata_store_set_value()
 * for string values avoiding a duplication. Otherwise it behaves the same
 * gegl_metadata_store_set_value().
 */
void          gegl_metadata_store_set_string    (GeglMetadataStore *self,
                                                 const gchar *name,
                                                 const gchar *string);

/**
 * gegl_metadata_store_get_string:
 * @self: A #GeglMetadataStore
 * @name: Metadata name
 *
 * A slightly more efficient version of gegl_metadata_store_get_value()
 * for string values avoiding a duplication. Otherwise it behaves the same
 * gegl_metadata_store_get_value().

 * Returns: (transfer none): String or %NULL.
 */
const gchar * gegl_metadata_store_get_string    (GeglMetadataStore *self,
                                                 const gchar *name);

/**
 * gegl_metadata_store_register:
 * @self: A #GeglMetadataStore
 * @local_name: Metadata name known to file module
 * @name: Metadata name
 * @transform: (scope async): A #GValueTransform function or %NULL
 *
 */
void          gegl_metadata_store_register      (GeglMetadataStore *self,
                                                 const gchar *local_name,
                                                 const gchar *name,
                                                 GValueTransform transform);

/**
 * gegl_metadata_store_notify:
 * @self: A #GeglMetadataStore
 * @pspec: The #GParamSpec used to declare the variable.
 * @shadow: The metadata variable shadows a property.
 *
 * gegl_metadata_store_notify() is called by subclasses when the value of a
 * metadata variable changes. It emits the `::changed` signal with the variable
 * name as the detail parameter.  Set @shadow = %TRUE if variable is shadowed
 * by a property so that a notify signal is emitted with the property name as
 * the detail parameter.
 */
void          gegl_metadata_store_notify        (GeglMetadataStore *self,
                                                 GParamSpec *pspec,
                                                 gboolean shadow);

#define GEGL_TYPE_RESOLUTION_UNIT       gegl_resolution_unit_get_type ()
GType         gegl_resolution_unit_get_type     (void) G_GNUC_CONST;

G_END_DECLS

#endif

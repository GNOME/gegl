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

#include "gegl-metadatastore.h"

typedef struct _GeglMetadataStorePrivate GeglMetadataStorePrivate;
struct _GeglMetadataStorePrivate
  {
    gchar *file_module_name;

    /* Resolution */
    GeglResolutionUnit resolution_unit;
    gdouble resolution_x;
    gdouble resolution_y;

    /* Temporary name map used by file module */
    GPtrArray *map;
    gboolean exclude_unmapped;
  };

/* Register GeglResolutionUnit with GType {{{1 */

GType
gegl_resolution_unit_get_type (void)
{
  GType type;
  const gchar *name;
  static gsize gegl_resolution_unit_type;
  static const GEnumValue values[] =
    {
      { GEGL_RESOLUTION_UNIT_NONE, "GEGL_RESOLUTION_UNIT_NONE", "none" },
      { GEGL_RESOLUTION_UNIT_DPI,  "GEGL_RESOLUTION_UNIT_DPI",  "dpi" },
      { GEGL_RESOLUTION_UNIT_DPM,  "GEGL_RESOLUTION_UNIT_DPM",  "dpm" },
      { 0, NULL, NULL }
    };

  if (g_once_init_enter (&gegl_resolution_unit_type))
    {
      name = g_intern_static_string ("GeglResolutionUnit");
      type = g_enum_register_static (name, values);
      g_once_init_leave (&gegl_resolution_unit_type, type);
    }
  return gegl_resolution_unit_type;
}

/* GObject {{{1 */

static void gegl_metadata_store_interface_init (GeglMetadataInterface *iface);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (
  GeglMetadataStore, gegl_metadata_store, G_TYPE_OBJECT,
  G_ADD_PRIVATE (GeglMetadataStore)
  G_IMPLEMENT_INTERFACE (GEGL_TYPE_METADATA, gegl_metadata_store_interface_init)
)

enum
  {
    CHANGED,
    MAPPED,
    UNMAPPED,
    GENERATE,
    PARSE,
    LAST_SIGNAL
  };
static guint gegl_metadata_store_signals[LAST_SIGNAL];

enum
  {
    PROP_0,
    PROP_RESOLUTION_UNIT,
    PROP_RESOLUTION_X,
    PROP_RESOLUTION_Y,
    PROP_FILE_MODULE_NAME,

    PROP_METADATA_SHADOW,
    PROP_TITLE = PROP_METADATA_SHADOW,
    PROP_ARTIST,
    PROP_DESCRIPTION,
    PROP_COPYRIGHT,
    PROP_DISCLAIMER,
    PROP_WARNING,
    PROP_COMMENT,
    PROP_SOFTWARE,
    PROP_SOURCE,
    PROP_TIMESTAMP,
    N_PROPERTIES
  };
static GParamSpec *gegl_metadata_store_prop[N_PROPERTIES];

static void gegl_metadata_store_constructed (GObject *object);
static void gegl_metadata_store_finalize (GObject *object);
static void gegl_metadata_store_get_property (GObject *object, guint param_id,
                                              GValue *value, GParamSpec *pspec);
static void gegl_metadata_store_set_property (GObject *object, guint param_id,
                                              const GValue *value, GParamSpec *pspec);
static GParamSpec *gegl_metadata_store_value_pspec (GeglMetadataStore *self,
                                                    const gchar *name);

/* inherited virtual methods */
static void gegl_metadata_store_register_hook (GeglMetadataStore *self,
                                               const gchar *file_module_name,
                                               guint flags);
static gboolean gegl_metadata_store_parse_value (GeglMetadataStore *self,
                                                 GParamSpec *pspec,
                                                 GValueTransform transform,
                                                 const GValue *value);
static gboolean gegl_metadata_store_generate_value (GeglMetadataStore *self,
                                                    GParamSpec *pspec,
                                                    GValueTransform transform,
                                                    GValue *value);

/* GObject {{{1 */

static void
gegl_metadata_store_class_init (GeglMetadataStoreClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->constructed = gegl_metadata_store_constructed;
  gobject_class->finalize = gegl_metadata_store_finalize;
  gobject_class->set_property = gegl_metadata_store_set_property;
  gobject_class->get_property = gegl_metadata_store_get_property;

  class->register_hook = gegl_metadata_store_register_hook;
  class->parse_value = gegl_metadata_store_parse_value;
  class->generate_value = gegl_metadata_store_generate_value;

  gegl_metadata_store_prop[PROP_RESOLUTION_UNIT] = g_param_spec_enum (
        "resolution-unit", "Resolution Unit",
        "Units for image resolution",
        GEGL_TYPE_RESOLUTION_UNIT, GEGL_RESOLUTION_UNIT_DPI,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_RESOLUTION_X] = g_param_spec_double (
        "resolution-x", "Resolution X",
        "X Resolution",
        -G_MAXDOUBLE, G_MAXDOUBLE, 300.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_RESOLUTION_Y] = g_param_spec_double (
        "resolution-y", "Resolution Y",
        "X Resolution",
        -G_MAXDOUBLE, G_MAXDOUBLE, 300.0,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_FILE_MODULE_NAME] = g_param_spec_string (
        "file-module-name", "File Module Name",
        "Name of currently active file module or NULL",
        NULL,
        G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  gegl_metadata_store_prop[PROP_TITLE] = g_param_spec_string (
        "title", "Title",
        "Short title or caption",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_ARTIST] = g_param_spec_string (
        "artist", "Artist",
        "Name of image creator",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_DESCRIPTION] = g_param_spec_string (
        "description", "Description",
        "Description of image (possibly long)",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_COPYRIGHT] = g_param_spec_string (
        "copyright", "Copyright",
        "Copyright notice",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_DISCLAIMER] = g_param_spec_string (
        "disclaimer", "Disclaimer",
        "Legal disclaimer",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_WARNING] = g_param_spec_string (
        "warning", "Warning",
        "Warning of nature of content",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_COMMENT] = g_param_spec_string (
        "comment", "Comment",
        "Miscellaneous comment",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_SOFTWARE] = g_param_spec_string (
        "software", "Software",
        "Software used to create the image",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_SOURCE] = g_param_spec_string (
        "source", "Source",
        "Device used to create the image",
        NULL,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);
  gegl_metadata_store_prop[PROP_TIMESTAMP] = g_param_spec_boxed (
        "timestamp", "Timestamp",
        "Image creation time",
        G_TYPE_DATE_TIME,
        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPERTIES, gegl_metadata_store_prop);

  gegl_metadata_store_signals[CHANGED] = g_signal_new (
        "changed",
        G_TYPE_FROM_CLASS (class), G_SIGNAL_DETAILED | G_SIGNAL_RUN_LAST, 0,
        NULL, NULL,
        NULL, G_TYPE_NONE, 1, G_TYPE_PARAM);
  gegl_metadata_store_signals[MAPPED] = g_signal_new (
        "mapped",
        G_TYPE_FROM_CLASS (class), G_SIGNAL_RUN_LAST, 0,
        NULL, NULL,
        NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_BOOLEAN);
  gegl_metadata_store_signals[UNMAPPED] = g_signal_new (
        "unmapped",
        G_TYPE_FROM_CLASS (class), G_SIGNAL_RUN_LAST, 0,
        NULL, NULL,
        NULL, G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_STRING);
  gegl_metadata_store_signals[GENERATE] = g_signal_new (
        "generate-value",
        G_TYPE_FROM_CLASS (class), G_SIGNAL_DETAILED | G_SIGNAL_RUN_LAST, 0,
        g_signal_accumulator_first_wins, NULL,
        NULL, G_TYPE_BOOLEAN, 2, G_TYPE_PARAM, G_TYPE_VALUE);
  gegl_metadata_store_signals[PARSE] = g_signal_new (
        "parse-value",
        G_TYPE_FROM_CLASS (class), G_SIGNAL_DETAILED | G_SIGNAL_RUN_LAST, 0,
        g_signal_accumulator_first_wins, NULL,
        NULL, G_TYPE_BOOLEAN, 2, G_TYPE_PARAM, G_TYPE_VALUE);
}

/* Make vmethods easier to read and type */
#define VMETHOD(self,name) (*GEGL_METADATA_STORE_GET_CLASS ((self))->name)

static void
gegl_metadata_store_init (GeglMetadataStore *self)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  priv->resolution_unit = GEGL_RESOLUTION_UNIT_DPI;
  priv->resolution_x = 300.0;
  priv->resolution_y = 300.0;
}

static void
gegl_metadata_store_constructed (GObject *object)
{
  GeglMetadataStore *self = (GeglMetadataStore *) object;
  guint i;
  GParamSpec *pspec;

  /* Shadow well-known metadata values with properties */
  for (i = PROP_METADATA_SHADOW; i < N_PROPERTIES; i++)
    {
      pspec = g_param_spec_ref (gegl_metadata_store_prop[i]);
      VMETHOD(self, _declare) (self, pspec, TRUE);
    }

  G_OBJECT_CLASS (gegl_metadata_store_parent_class)->constructed (object);
}

static void
gegl_metadata_store_finalize (GObject *object)
{
  //GeglMetadataStore *self = (GeglMetadataStore *) object;

  G_OBJECT_CLASS (gegl_metadata_store_parent_class)->finalize (object);
}

/* Properties {{{1 */

static void
gegl_metadata_store_set_property (GObject *object, guint prop_id,
                                  const GValue *value, GParamSpec *pspec)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (object);
  const gchar *name;

  switch (prop_id)
    {
    case PROP_RESOLUTION_UNIT:
      gegl_metadata_store_set_resolution_unit (self, g_value_get_enum (value));
      break;
    case PROP_RESOLUTION_X:
      gegl_metadata_store_set_resolution_x (self, g_value_get_double (value));
      break;
    case PROP_RESOLUTION_Y:
      gegl_metadata_store_set_resolution_y (self, g_value_get_double (value));
      break;
    default:
      name = g_param_spec_get_name (pspec);
      gegl_metadata_store_set_value (self, name, value);
      return;
    }
}


static void
gegl_metadata_store_get_property (GObject *object, guint prop_id,
                                  GValue *value, GParamSpec *pspec)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (object);
  const gchar *name;

  switch (prop_id)
    {
    case PROP_RESOLUTION_UNIT:
      g_value_set_enum (value, gegl_metadata_store_get_resolution_unit (self));
      break;
    case PROP_RESOLUTION_X:
      g_value_set_double (value, gegl_metadata_store_get_resolution_x (self));
      break;
    case PROP_RESOLUTION_Y:
      g_value_set_double (value, gegl_metadata_store_get_resolution_y (self));
      break;
    case PROP_FILE_MODULE_NAME:
      g_value_set_string (value, gegl_metadata_store_get_file_module_name (self));
      break;
    default:
      name = g_param_spec_get_name (pspec);
      gegl_metadata_store_get_value (self, name, value);
      break;
    }
}

/* "resolution-unit" {{{2 */

void
gegl_metadata_store_set_resolution_unit (GeglMetadataStore *self,
                                         GeglResolutionUnit unit)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  if (priv->resolution_unit != unit)
    {
      priv->resolution_unit = unit;
      g_object_notify_by_pspec (G_OBJECT (self), gegl_metadata_store_prop[PROP_RESOLUTION_UNIT]);
    }
}

GeglResolutionUnit
gegl_metadata_store_get_resolution_unit (GeglMetadataStore *self)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), GEGL_RESOLUTION_UNIT_DPI);

  return priv->resolution_unit;
}

/* "resolution-x" {{{2 */

void
gegl_metadata_store_set_resolution_x (GeglMetadataStore *self, gdouble resolution_x)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  if (priv->resolution_x != resolution_x)
    {
      priv->resolution_x = resolution_x;
      g_object_notify_by_pspec (G_OBJECT (self), gegl_metadata_store_prop[PROP_RESOLUTION_X]);
    }
}

gdouble
gegl_metadata_store_get_resolution_x (GeglMetadataStore *self)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), 0);

  return priv->resolution_x;
}

/* "resolution-y" {{{2 */

void
gegl_metadata_store_set_resolution_y (GeglMetadataStore *self, gdouble resolution_y)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  if (priv->resolution_y != resolution_y)
    {
      priv->resolution_y = resolution_y;
      g_object_notify_by_pspec (G_OBJECT (self), gegl_metadata_store_prop[PROP_RESOLUTION_Y]);
    }
}

gdouble
gegl_metadata_store_get_resolution_y (GeglMetadataStore *self)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), 0);

  return priv->resolution_y;
}

/* "file-module-name" {{{2 */

const gchar *
gegl_metadata_store_get_file_module_name (GeglMetadataStore *self)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), 0);

  return priv->file_module_name;
}

/* "title" {{{2 */

void
gegl_metadata_store_set_title (GeglMetadataStore *self, const gchar *title)
{
  gegl_metadata_store_set_string (self, "title", title);
}

const gchar *
gegl_metadata_store_get_title (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "title");
}

/* "artist" {{{2 */

void
gegl_metadata_store_set_artist (GeglMetadataStore *self, const gchar *artist)
{
  gegl_metadata_store_set_string (self, "artist", artist);
}

const gchar *
gegl_metadata_store_get_artist (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "artist");
}

/* "description" {{{2 */

void
gegl_metadata_store_set_description (GeglMetadataStore *self, const gchar *description)
{
  gegl_metadata_store_set_string (self, "description", description);
}

const gchar *
gegl_metadata_store_get_description (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "description");
}

/* "copyright" {{{2 */

void
gegl_metadata_store_set_copyright (GeglMetadataStore *self, const gchar *copyright)
{
  gegl_metadata_store_set_string (self, "copyright", copyright);
}

const gchar *
gegl_metadata_store_get_copyright (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "copyright");
}

/* "disclaimer" {{{2 */

void
gegl_metadata_store_set_disclaimer (GeglMetadataStore *self, const gchar *disclaimer)
{
  gegl_metadata_store_set_string (self, "disclaimer", disclaimer);
}

const gchar *
gegl_metadata_store_get_disclaimer (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "disclaimer");
}

/* "warning" {{{2 */

void
gegl_metadata_store_set_warning (GeglMetadataStore *self, const gchar *warning)
{
  gegl_metadata_store_set_string (self, "warning", warning);
}

const gchar *
gegl_metadata_store_get_warning (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "warning");
}

/* "comment" {{{2 */

void
gegl_metadata_store_set_comment (GeglMetadataStore *self, const gchar *comment)
{
  gegl_metadata_store_set_string (self, "comment", comment);
}

const gchar *
gegl_metadata_store_get_comment (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "comment");
}

/* "software" {{{2 */

void
gegl_metadata_store_set_software (GeglMetadataStore *self, const gchar *software)
{
  gegl_metadata_store_set_string (self, "software", software);
}

const gchar *
gegl_metadata_store_get_software (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "software");
}

/* "source" {{{2 */

void
gegl_metadata_store_set_source (GeglMetadataStore *self, const gchar *source)
{
  gegl_metadata_store_set_string (self, "source", source);
}

const gchar *
gegl_metadata_store_get_source (GeglMetadataStore *self)
{
  return gegl_metadata_store_get_string (self, "source");
}

/* "timestamp" {{{2 */

void
gegl_metadata_store_set_timestamp (GeglMetadataStore *self,
                                   const GDateTime *timestamp)
{
  GValue value = G_VALUE_INIT;

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  g_value_init (&value, G_TYPE_DATE_TIME);
  g_value_set_boxed (&value, timestamp);
  gegl_metadata_store_set_value (self, "timestamp", &value);
  g_value_unset (&value);
}

GDateTime *
gegl_metadata_store_get_timestamp (GeglMetadataStore *self)
{
  GValue value = G_VALUE_INIT;
  GDateTime *timestamp = NULL;

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), NULL);

  g_value_init (&value, G_TYPE_DATE_TIME);
  if (gegl_metadata_store_has_value (self, "timestamp"))
    {
      gegl_metadata_store_get_value (self, "timestamp", &value);
      timestamp = g_date_time_ref (g_value_get_boxed (&value));
    }
  g_value_unset (&value);
  return timestamp;
}

/* Declare metadata {{{1 */

void
gegl_metadata_store_declare (GeglMetadataStore *self, GParamSpec *pspec)
{
  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  VMETHOD(self, _declare) (self, pspec, FALSE);
}

/* Metadata accessors {{{1 */

gboolean
gegl_metadata_store_has_value (GeglMetadataStore *self, const gchar *name)
{
  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), FALSE);

  return VMETHOD(self, has_value) (self, name);
}

static GParamSpec *
gegl_metadata_store_value_pspec (GeglMetadataStore *self, const gchar *name)
{
  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), G_TYPE_INVALID);

  return VMETHOD(self, pspec) (self, name);
}

GType
gegl_metadata_store_typeof_value (GeglMetadataStore *self, const gchar *name)
{
  GParamSpec *pspec;

  pspec = gegl_metadata_store_value_pspec (self, name);
  return pspec != NULL ? G_PARAM_SPEC_VALUE_TYPE (pspec) : G_TYPE_INVALID;
}

/* set/get by GValue */

void
gegl_metadata_store_notify (GeglMetadataStore *self, GParamSpec *pspec,
                            gboolean notify)
{
  GQuark quark;

  if (notify)
    g_object_notify_by_pspec (G_OBJECT (self), pspec);
  quark = g_param_spec_get_name_quark (pspec);
  g_signal_emit (self, gegl_metadata_store_signals[CHANGED], quark, pspec);
}

void
gegl_metadata_store_set_value (GeglMetadataStore *self, const gchar *name,
                               const GValue *value)
{
  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  VMETHOD(self, set_value) (self, name, value);
}

void
gegl_metadata_store_get_value (GeglMetadataStore *self, const gchar *name,
                               GValue *value)
{
  const GValue *internal;

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  internal = VMETHOD(self, _get_value) (self, name);
  g_return_if_fail (internal != NULL && G_IS_VALUE (internal));
  g_value_transform (internal, value);
}

/* convenience methods for common case of string, these avoid having to
   manipulate GValues and their associated lifetimes */

void
gegl_metadata_store_set_string (GeglMetadataStore *self, const gchar *name,
                                const gchar *string)
{
  GValue internal = G_VALUE_INIT;

  g_return_if_fail (GEGL_IS_METADATA_STORE (self));

  g_value_init (&internal, G_TYPE_STRING);
  g_value_set_static_string (&internal, string);
  VMETHOD(self, set_value) (self, name, &internal);
  g_value_unset (&internal);
}

const gchar *
gegl_metadata_store_get_string (GeglMetadataStore *self, const gchar *name)
{
  const GValue *internal;

  g_return_val_if_fail (GEGL_IS_METADATA_STORE (self), NULL);

  internal = VMETHOD(self, _get_value) (self, name);
  g_return_val_if_fail (internal != NULL && G_IS_VALUE (internal), NULL);
  g_return_val_if_fail (G_VALUE_HOLDS (internal, G_TYPE_STRING), NULL);

  return g_value_get_string (internal);
}

/* GeglMetadataMapValue {{{1 */

typedef struct
  {
    gchar *local_name;
    gchar *name;
    GValueTransform transform;
  } GeglMetadataMapValue;

static GeglMetadataMapValue *
metadata_map_new (const gchar *local_name, const gchar *name,
                  GValueTransform transform)
{
  GeglMetadataMapValue *map_value;

  map_value = g_slice_new (GeglMetadataMapValue);
  map_value->local_name = g_strdup (local_name);
  map_value->name = g_strdup (name);
  map_value->transform = transform;
  return map_value;
}

static void
metadata_map_free (gpointer data)
{
  GeglMetadataMapValue *map_value = data;

  g_free (map_value->local_name);
  g_free (map_value->name);
  g_slice_free (GeglMetadataMapValue, map_value);
}


static gboolean
metadata_map_equal (gconstpointer a, gconstpointer b)
{
  const GeglMetadataMapValue *map_value = a;
  const gchar *local_name = b;

  return g_strcmp0 (map_value->local_name, local_name) == 0;
}

static GeglMetadataMapValue *
metadata_map_lookup (GeglMetadataStore *self, const gchar *local_name)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);
  guint indx;

  g_return_val_if_fail (priv->map != NULL, NULL);

  if (g_ptr_array_find_with_equal_func (priv->map, local_name,
                                        metadata_map_equal, &indx))
    return g_ptr_array_index (priv->map, indx);
  return NULL;
}

/* Register metadata name and type mappings {{{1 */

void
gegl_metadata_store_register (GeglMetadataStore *self,
                              const gchar *local_name, const gchar *name,
                              GValueTransform transform)
{
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);
  GeglMetadataMapValue *map_value;
  guint indx;

  map_value = metadata_map_new (local_name, name, transform);

  if (g_ptr_array_find_with_equal_func (priv->map, local_name,
                                        metadata_map_equal, &indx))
    {
      metadata_map_free (g_ptr_array_index (priv->map, indx));
      g_ptr_array_index (priv->map, indx) = map_value;
    }
  else
    g_ptr_array_add (priv->map, map_value);
}

/* Metadata Interface {{{1 */

/* register_map {{{2 */

static void
gegl_metadata_store_register_hook (GeglMetadataStore *self,
                                   const gchar *file_module_name, guint flags)
{
  g_signal_emit (self, gegl_metadata_store_signals[MAPPED], 0,
                 file_module_name, !!(flags & GEGL_MAP_EXCLUDE_UNMAPPED));
}

/* map == NULL clears map */
static void
gegl_metadata_store_register_map (GeglMetadata *metadata,
                                  const gchar *file_module_name,
                                  guint flags,
                                  const GeglMetadataMap *map, gsize n_map)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);
  GeglMetadataMapValue *map_value;
  gsize i;

  if (priv->map != NULL)
    g_ptr_array_unref (priv->map);

  if (map != NULL)
    {
      priv->file_module_name = g_strdup (file_module_name);
      priv->exclude_unmapped = !!(flags & GEGL_MAP_EXCLUDE_UNMAPPED);

      priv->map = g_ptr_array_new_full (n_map, metadata_map_free);
      for (i = 0; i < n_map; i++)
        {
          map_value = metadata_map_new (map[i].local_name,
                                        map[i].name, map[i].transform);
          g_ptr_array_add (priv->map, map_value);
        }

      VMETHOD(self, register_hook) (self, file_module_name, flags);
    }
  else
    {
      g_free (priv->file_module_name);

      priv->map = NULL;
      priv->file_module_name = NULL;
      priv->exclude_unmapped = FALSE;
    }
  g_object_notify_by_pspec (G_OBJECT (self), gegl_metadata_store_prop[PROP_FILE_MODULE_NAME]);
}

/* resolution {{{2 */

static gboolean
gegl_metadata_store_set_resolution (GeglMetadata *metadata,
                                    GeglResolutionUnit unit,
                                    gfloat x, gfloat y)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);

  /* sanity check */
  if (x == 0.0f && y == 0.0f)
    return FALSE;
  if (x == 0.0f)
    x = y;
  else if (y == 0.0f)
    y = x;
  gegl_metadata_store_set_resolution_unit (self, unit);
  gegl_metadata_store_set_resolution_x (self, x);
  gegl_metadata_store_set_resolution_y (self, y);
  return TRUE;
}

static gboolean
gegl_metadata_store_get_resolution (GeglMetadata *metadata,
                                    GeglResolutionUnit *unit,
                                    gfloat *x, gfloat *y)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);

  *unit = gegl_metadata_store_get_resolution_unit (self);
  *x = gegl_metadata_store_get_resolution_x (self);
  *y = gegl_metadata_store_get_resolution_y (self);
  return TRUE;
}

/* iterators {{{2 */

#define STAMP           0xa5caf30e
#define INVALID_STAMP   0

static gboolean
gegl_metadata_store_iter_lookup (GeglMetadata *metadata, GeglMetadataIter *iter,
                                 const gchar *local_name)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);
  GeglMetadataMapValue *map_value;

  map_value = metadata_map_lookup (self, local_name);
  if (map_value == NULL)
    {
      if (priv->exclude_unmapped)
        return FALSE;
      /* emit unmapped signal and try again */
      g_signal_emit (self, gegl_metadata_store_signals[UNMAPPED], 0,
                     priv->file_module_name, local_name);
      map_value = metadata_map_lookup (self, local_name);
      if (map_value == NULL)
        return FALSE;
    }
  iter->stamp = STAMP;
  iter->user_data = self;
  iter->user_data2 = NULL;
  iter->user_data3 = map_value;
  return TRUE;
}

static void
gegl_metadata_store_iter_init (GeglMetadata *metadata, GeglMetadataIter *iter)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);

  g_return_if_fail (priv->map != NULL);

  iter->stamp = STAMP;
  iter->user_data = self;
  iter->user_data2 = &g_ptr_array_index (priv->map, 0);
  iter->user_data3 = NULL;
}

static const gchar *
gegl_metadata_store_iter_next (GeglMetadata *metadata, GeglMetadataIter *iter)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataStorePrivate *priv = gegl_metadata_store_get_instance_private (self);
  GeglMetadataMapValue *map_value;
  gpointer *pdata;

  g_return_val_if_fail (iter->stamp == STAMP, NULL);
  g_return_val_if_fail (iter->user_data == self, NULL);
  g_return_val_if_fail (iter->user_data2 != NULL, NULL);

  pdata = iter->user_data2;
  if (pdata < &g_ptr_array_index (priv->map, priv->map->len))
    {
      map_value = *pdata++;
      iter->user_data2 = pdata;
      iter->user_data3 = map_value;
      return map_value->local_name;
    }

  iter->stamp = INVALID_STAMP;
  return NULL;
}

/* get/set mapped values {{{2 */

/* If a "parse-value::name" signal is registered emit the signal to parse the
 * value and return TRUE. If no  handler is registered, return FALSE.  The
 * handler parses the supplied GValue and may set any number of metadata values
 * using gegl_metadata_store_set_value().
 */
static gboolean
gegl_metadata_store_parse_value (GeglMetadataStore *self,
                                 GParamSpec *pspec,
                                 GValueTransform transform,
                                 const GValue *value)
{
  GQuark quark;
  guint signal_id;
  gboolean success;

  quark = g_param_spec_get_name_quark (pspec);
  signal_id = gegl_metadata_store_signals[PARSE];
  if (g_signal_has_handler_pending (self, signal_id, quark, FALSE))
    {
      /* If the GValue types are compatible pass value directly to the signal
         handler.  Otherwise initialise a GValue, attempt to transform the
         value and, if successful, call the signal handler. */
      success = FALSE;
      if (g_value_type_compatible (G_PARAM_SPEC_VALUE_TYPE (pspec),
                                   G_VALUE_TYPE (value)))
        g_signal_emit (self, signal_id, quark, pspec, value, &success);
      else
        {
          GValue temp = G_VALUE_INIT;

          g_value_init (&temp, G_PARAM_SPEC_VALUE_TYPE (pspec));
          if (transform != NULL)
            {
              (*transform) (value, &temp);
              success = TRUE;
            }
          else
            success = g_value_transform (value, &temp);
          if (success)
            g_signal_emit (self, signal_id, quark, pspec, &temp, &success);
          g_value_unset (&temp);
        }
      return success;
    }
  return FALSE;
}

/* Note that the underlying value is not set if a parse_value() returns TRUE
 * and that this processing is performed only when the metadata is accessed via
 * the GeglMetadata interface.
 */
static gboolean
gegl_metadata_store_iter_set_value (GeglMetadata *metadata,
                                    GeglMetadataIter *iter,
                                    const GValue *value)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataMapValue *map_value;
  GParamSpec *pspec;

  g_return_val_if_fail (iter->stamp == STAMP, FALSE);
  g_return_val_if_fail (iter->user_data == self, FALSE);
  g_return_val_if_fail (iter->user_data3 != NULL, FALSE);

  map_value = iter->user_data3;

  pspec = VMETHOD(self, pspec) (self, map_value->name);
  g_return_val_if_fail (pspec != NULL, FALSE);

  /* Try calling parse_value() */
  if (VMETHOD(self, parse_value) (self, pspec, map_value->transform, value))
    return TRUE;

  if (map_value->transform == NULL)
    VMETHOD(self, set_value) (self, map_value->name, value);
  else
    {
      GValue xfrm = G_VALUE_INIT;

      g_value_init (&xfrm, G_PARAM_SPEC_VALUE_TYPE (pspec));
      (*map_value->transform) (value, &xfrm);
      VMETHOD(self, set_value) (self, map_value->name, &xfrm);
      g_value_unset (&xfrm);
    }
  return TRUE;
}

/* If a "generate-value::name" signal is registered emit the signal to parse
 * the value and return TRUE. If no  handler is registered, return FALSE.  The
 * signal handler must set a value of the type specified in the pspec argument
 * and return TRUE if successful.
 */
static gboolean
gegl_metadata_store_generate_value (GeglMetadataStore *self,
                                    GParamSpec *pspec,
                                    GValueTransform transform,
                                    GValue *value)
{
  GQuark quark;
  guint signal_id;
  gboolean success;

  quark = g_param_spec_get_name_quark (pspec);
  signal_id = gegl_metadata_store_signals[GENERATE];
  if (g_signal_has_handler_pending (self, signal_id, quark, FALSE))
    {
      /* if the GValue types are compatible pass the return value directly to
         the signal handler.  Otherwise initialise a GValue, call the signal
         handler and transform the return value as for the regular case below. */
      success = FALSE;
      if (g_value_type_compatible (G_PARAM_SPEC_VALUE_TYPE (pspec),
                                   G_VALUE_TYPE (value)))
        g_signal_emit (self, signal_id, quark, pspec, value, &success);
      else
        {
          GValue temp = G_VALUE_INIT;

          g_value_init (&temp, G_PARAM_SPEC_VALUE_TYPE (pspec));
          g_signal_emit (self, signal_id, quark, pspec, &temp, &success);
          if (success)
            {
              if (transform != NULL)
                (*transform) (&temp, value);
              else
                g_value_transform (&temp, value);
            }
          g_value_unset (&temp);
        }
      return TRUE;
    }
  return FALSE;
}

/* Note that the underlying value is not accessed if a generate_value() returns
 * TRUE that this processing is only performed when accessed via the
 * GeglMetadata interface.  The signal handler can, however, access the actual
 * stored value using gegl_metadata_store_get_value().
 */
static gboolean
gegl_metadata_store_iter_get_value (GeglMetadata *metadata,
                                    GeglMetadataIter *iter,
                                    GValue *value)
{
  GeglMetadataStore *self = GEGL_METADATA_STORE (metadata);
  GeglMetadataMapValue *map_value;
  const GValue *meta;
  GParamSpec *pspec;

  g_return_val_if_fail (iter->stamp == STAMP, FALSE);
  g_return_val_if_fail (iter->user_data == self, FALSE);
  g_return_val_if_fail (iter->user_data3 != NULL, FALSE);

  map_value = iter->user_data3;

  pspec = VMETHOD(self, pspec) (self, map_value->name);
  g_return_val_if_fail (pspec != NULL, FALSE);

  /* Try calling generate_value() */
  if (VMETHOD(self, generate_value) (self, pspec, map_value->transform, value))
    return TRUE;

  /* If a transform function is set, use that to convert the stored gvalue to
     the requested type, otherwise use g_value_transform().  */
  meta = VMETHOD(self, _get_value) (self, map_value->name);
  if (meta == NULL)
    return FALSE;

  if (map_value->transform != NULL)
    {
      (*map_value->transform) (meta, value);
      return TRUE;
    }
  return g_value_transform (meta, value);
}

static void
gegl_metadata_store_interface_init (GeglMetadataInterface *iface)
{
  iface->register_map = gegl_metadata_store_register_map;
  iface->set_resolution = gegl_metadata_store_set_resolution;
  iface->get_resolution = gegl_metadata_store_get_resolution;
  iface->iter_lookup = gegl_metadata_store_iter_lookup;
  iface->iter_init = gegl_metadata_store_iter_init;
  iface->iter_next = gegl_metadata_store_iter_next;
  iface->iter_set_value = gegl_metadata_store_iter_set_value;
  iface->iter_get_value = gegl_metadata_store_iter_get_value;
}

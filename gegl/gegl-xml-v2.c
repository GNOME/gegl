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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås
 * Copyright 2012 Michael Muré <batolettre@gmail.com>
 */
#include "config.h"

#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "gegl.h"
#include "gegl-instrument.h"
#include "property-types/gegl-paramspecs.h"
#include "gegl-types-internal.h"
#include "gegl/graph/gegl-node.h"
#include "gegl/graph/gegl-pad.h"
#include "gegl-xml-v2.h"

#ifdef G_OS_WIN32
#define realpath(a, b)    _fullpath (b, a, _MAX_PATH)
#endif

typedef struct
{
  const gchar *path_root;
  GeglNode    *gegl;  /** the resulting graph */
  GeglNode    *current_node;
  gchar       *param; /** the param we are setting (NULL when not in <param></param>) */
  GeglCurve   *curve; /** the curve whose points we are parsing */

  GHashTable  *ids;   /** node id's string to node */
} ParseData;

/* Search a paired attribute name/value arrays for an entry with a specific
 * name, return the value or null if not found.
 */
static const gchar *name2val (const gchar **attribute_names,
                              const gchar **attribute_values,
                              const gchar  *name)
{
  while (*attribute_names)
    {
      if (!strcmp (*attribute_names, name))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }
  return NULL;
}

/* Check that the attributes for ELEMENT include an attribute of name NAME,
 * and store it in the variable of the same name. Else report an error.
 *
 * eg:  { collect_attribute (value, 'key'); }
 */
#define collect_attribute(NAME, ELEMENT)                                \
{                                                                       \
  const gchar *__value = name2val (a, v, (G_STRINGIFY (NAME)));         \
  if (!__value)                                                         \
    {                                                                   \
      *error = g_error_new (G_MARKUP_ERROR,                             \
                            G_MARKUP_ERROR_MISSING_ATTRIBUTE,           \
                            "Expected attribute '%s' in element '%s'",  \
                            (G_STRINGIFY (NAME)),                       \
                            (ELEMENT));                                 \
      return;                                                           \
    }                                                                   \
  else                                                                  \
    {                                                                   \
      (NAME) = __value;                                                 \
    }                                                                   \
}

static GMarkupParser parser;

static void start_element (GMarkupParseContext *context,
                           const gchar         *element_name,
                           const gchar        **attribute_names,
                           const gchar        **attribute_values,
                           gpointer             user_data,
                           GError             **error)
{
  const gchar **a  = attribute_names;
  const gchar **v  = attribute_values;
  ParseData    *pd = user_data;

  if (!strcmp (element_name, "gegl")) /* Subgraph, we launch recursively a new parser. */
    {
      const gchar *id;
      ParseData   *pd_recurs = g_slice_new0 (ParseData);

      pd_recurs->gegl      = gegl_node_new ();
      pd_recurs->ids       = pd->ids; /* we reuse the same hash table to be able to make input connection of the subgraph. */
      pd_recurs->path_root = pd->path_root;

      collect_attribute (id, "gegl");
      g_hash_table_insert (pd->ids, g_strdup (id), pd_recurs->gegl);

      g_markup_parse_context_push (context, &parser, pd_recurs);
    }
  else if (!strcmp (element_name, "node"))
    {
      const gchar *id;
      const gchar *op;

      collect_attribute (op, "node");
      collect_attribute (id, "node");
      pd->current_node = gegl_node_new_child (pd->gegl, "operation", op, NULL);

      if (pd->current_node == NULL)
      {
        g_warning ("Could not instantiate operation %s", op);
        return;
      }

      if (g_hash_table_lookup(pd->ids, id))
      {
        g_warning ("Duplicate node ID %s", id);
        return;
      }

      /* set the name of the node with the ID */
      gegl_node_set (pd->current_node, "name", id, NULL);

      g_hash_table_insert (pd->ids, g_strdup (id), pd->current_node);
    }
  else if (!strcmp (element_name, "param"))
    {
      const gchar *name;

      collect_attribute (name, "param");

      if (pd->param != NULL)
        g_warning ("eek, haven't cleared previous param");
      pd->param = g_strdup (name);
    }
  else if (!strcmp (element_name, "edge"))
    {
      const gchar *from;
      const gchar *of;
      const gchar *to;
      GeglNode *source_node;
      GeglNode *sink_node;
      gboolean success;

      collect_attribute (from, "edge");
      collect_attribute (of, "edge");
      collect_attribute (to, "edge");

      source_node = g_hash_table_lookup (pd->ids, of);

      if (!source_node)
        {
          g_warning ("Unknow source node id %s at this point of parsing.", of);
          return;
        }

      sink_node = pd->current_node ? pd->current_node : pd->gegl;

      success = gegl_node_connect_to (source_node, from,
                                      sink_node, to);
      if (!success)
        g_warning ("Connection from %s of %s to %s failed.", from, of, to);
    }
  else if (!strcmp (element_name, "curve"))
    {
      const gchar *ymin, *ymax;
      if (pd->curve != NULL)
        g_warning ("we haven't cleared previous curve");

      collect_attribute (ymin, "curve");
      collect_attribute (ymax, "curve");

      pd->curve = gegl_curve_new (g_ascii_strtod (ymin, NULL),
                                  g_ascii_strtod (ymax, NULL));
    }
  else if (!strcmp (element_name, "curve-point"))
    {
      if (!pd->curve)
        g_warning ("curve not instantiated");
      else
        {
          const gchar *x, *y;
          collect_attribute (x, "curve-point");
          collect_attribute (y, "curve-point");

          gegl_curve_add_point (pd->curve,
                                g_ascii_strtod (x, NULL),
                                g_ascii_strtod (y, NULL));
        }
    }
}

static void text (GMarkupParseContext *context,
                  const gchar         *text,
                  gsize                text_len,
                  gpointer             user_data,
                  GError             **error)
{
  ParseData  *pd = user_data;
  GParamSpec *paramspec;

  if (!pd->param || !pd->current_node)
    return;

  paramspec = gegl_node_find_property (pd->current_node, pd->param);

  if (!paramspec)
    {
      g_warning ("property %s not found for %s",
                 pd->param, gegl_node_get_operation (pd->current_node));
    }
  else if (g_type_is_a (G_PARAM_SPEC_TYPE (paramspec),
                        GEGL_TYPE_PARAM_FILE_PATH))
    {
      gchar *buf;

      if (g_path_is_absolute (text))
        {
          gegl_node_set (pd->current_node, pd->param, text, NULL);
        }
      else
        {
          gchar * absolute_path;
          if (pd->path_root)
            {
              buf = g_strdup_printf ("%s/%s", pd->path_root, text);
            }
          else
            {
              buf = g_strdup_printf ("./%s", text);
            }

          absolute_path = realpath (buf, NULL);
          g_free (buf);
          if (absolute_path)
            {
              gegl_node_set (pd->current_node, pd->param, absolute_path, NULL);
              free (absolute_path);
            }
          else
            {
              g_warning ("Unable to obtain absolute path for parameter %s\n",
                         pd->param);
            }
        }
    }
  else if (paramspec->value_type == G_TYPE_INT)
    {
      gegl_node_set (pd->current_node, pd->param, atoi (text), NULL);
    }
  else if (paramspec->value_type == G_TYPE_FLOAT ||
           paramspec->value_type == G_TYPE_DOUBLE)
    {
      gegl_node_set (pd->current_node, pd->param, g_ascii_strtod (text, NULL), NULL);
    }
  else if (paramspec->value_type == G_TYPE_STRING)
    {
      gegl_node_set (pd->current_node, pd->param, text, NULL);
    }
  else if (paramspec->value_type == G_TYPE_BOOLEAN)
    {
      if (!strcmp (text, "true") ||
          !strcmp (text, "TRUE") ||
          !strcmp (text, "YES") ||
          !strcmp (text, "yes") ||
          !strcmp (text, "y") ||
          !strcmp (text, "Y") ||
          !strcmp (text, "1") ||
          !strcmp (text, "on"))
        {
          gegl_node_set (pd->current_node, pd->param, TRUE, NULL);
        }
      else
        {
          gegl_node_set (pd->current_node, pd->param, FALSE, NULL);
        }
    }
  else if (g_type_is_a (paramspec->value_type, G_TYPE_ENUM))
    {
      GEnumClass *eclass = g_type_class_peek (paramspec->value_type);
      GEnumValue *evalue = g_enum_get_value_by_nick (eclass, text);
      gegl_node_set (pd->current_node, pd->param, evalue->value, NULL);
    }
  else if (paramspec->value_type == GEGL_TYPE_COLOR)
    {
      GeglColor *color = gegl_color_new (text);
      gegl_node_set (pd->current_node, pd->param, color, NULL);
      g_object_unref (color);
    }
  else if (paramspec->value_type == GEGL_TYPE_CURVE)
    {
      /* Nothing to do*/
    }
  else if (paramspec->value_type == GEGL_TYPE_PATH)
    {
      GeglPath *path = gegl_path_new ();
      gegl_path_parse_string (path, text);
      gegl_node_set (pd->current_node, pd->param, path, NULL);
    }
  else
    {
      g_warning ("Non-implemented parameter type for %s", pd->param);
    }
}

/* Called for close tags </foo> */
static void end_element (GMarkupParseContext *context,
                         const gchar         *element_name,
                         gpointer             user_data,
                         GError             **error)
{
  ParseData *pd = user_data;

  if (!strcmp (element_name, "gegl"))
    {
      ParseData *pd_recurs = g_markup_parse_context_pop (context);

      if (pd->gegl) /* end of a subgraph */
          gegl_node_add_child (pd->gegl, pd_recurs->gegl);
      else /* end of top-level graph */
          pd->gegl = pd_recurs->gegl;

      g_slice_free (ParseData, pd_recurs);

    }
  else if (!strcmp (element_name, "node"))
    {
      pd->current_node = NULL;

      if (pd->param)
        {
          g_warning ("Should not have a param at this point");
          g_free (pd->param);
          pd->param = NULL;
        }

      if (pd->curve)
        {
          g_warning ("Should not have a curve at this point");
          g_object_unref (pd->curve);
          pd->curve = NULL;
        }
    }
  else if (!strcmp (element_name, "param"))
    {
      g_free (pd->param);
      pd->param = NULL;

      if (pd->curve)
        {
          g_warning ("Should not have a curve at this point");
          g_object_unref (pd->curve);
          pd->curve = NULL;
        }
    }
  else if (!strcmp (element_name, "curve"))
    {
      g_assert (pd->param && pd->current_node && pd->curve);
      gegl_node_set (pd->current_node, pd->param, pd->curve, NULL);
      g_object_unref (pd->curve);
      pd->curve = NULL;
    }
}

/* Called on error, including one set by other
 * methods in the vtable. The GError should not be freed.
 */
static void error (GMarkupParseContext *context,
                   GError              *error,
                   gpointer             user_data)
{
  gint  line_number;
  gint  char_number;

  g_markup_parse_context_get_position (context, &line_number, &char_number);
  g_warning ("XML Parse error %i:%i: %s",
             line_number, char_number, error->message);
}

static GMarkupParser parser = {
  start_element,
  end_element,
  text,
  NULL,
  error
};

GeglNode *gegl_node_new_from_xml_v2 (const gchar *xmldata,
                                     const gchar *path_root)
{
  glong                time = gegl_ticks ();
  ParseData            pd   = { 0, };
  GMarkupParseContext *context;
  gboolean             success = FALSE;

  g_return_val_if_fail (xmldata != NULL, NULL);

  pd.ids       = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  pd.path_root = path_root;

  context = g_markup_parse_context_new   (&parser, 0, &pd, NULL);
  success = g_markup_parse_context_parse (context,
                                          xmldata,
                                          strlen (xmldata),
                                          NULL);
  if (!success)
    {
      if (pd.gegl)
        {
          g_object_unref (pd.gegl);
          pd.gegl = NULL;
        }
    }

  g_markup_parse_context_free (context);
  g_hash_table_destroy (pd.ids);


  time = gegl_ticks () - time;
  gegl_instrument ("gegl", "gegl_parse_xml", time);

  return success ? pd.gegl : NULL;
}

GeglNode *
gegl_node_new_from_file_v2 (const gchar   *path)
{
  GeglNode *node = NULL;
  GError   *err  = NULL;
  gchar    *script;
  gchar    *path_root;
  gchar    *dirname;

  g_assert (path);

  dirname = g_path_get_dirname (path);
  path_root = realpath (dirname, NULL);
  if (!path_root)
    {
      goto cleanup;
    }

  g_file_get_contents (path, &script, NULL, &err);
  if (err != NULL)
    {
      g_warning ("Unable to read file: %s", err->message);
      g_error_free (err);
      goto cleanup;
    }

  node = gegl_node_new_from_xml (script, path_root);

cleanup:
  g_free (path_root);
  g_free (dirname);
  return node;
}

/****/


#define ind    do { gint i; for (i = 0; i < indent; i++) g_string_append (ss->buf, " "); } while (0)

typedef struct
{
  GString     *buf;
  const gchar *path_root;
  GHashTable  *nodes;   /** node id's string to node */
  gint         counter;
} SerializeState;

/** Output an XML attribute, inside of an XML tag */
static void
xml_attr (GString     *buf,
          const gchar *key,
          const gchar *value)
{
  g_assert (key);

  if (value)
    {
      gchar *text = g_markup_escape_text (value, -1);
      gchar *p;

      g_string_append_c (buf, ' ');
      g_string_append (buf, key);
      g_string_append_c (buf, '=');
      g_string_append_c (buf, '\'');
      for (p = text; *p; p++)
        {
          if (*p == '\n')
            g_string_append (buf, "&#10;");
          else
            g_string_append_c (buf, *p);
        }
      g_string_append_c (buf, '\'');

      g_free (text);
    }
}

static void
xml_param_start (SerializeState *ss,
                 gint            indent,
                 const gchar    *key)
{
  g_assert (key);
  ind; g_string_append (ss->buf, "<param name='");
  g_string_append (ss->buf, key);
  g_string_append (ss->buf, "'>");
}

static void
xml_param_end (SerializeState *ss)
{
  g_string_append (ss->buf, "</param>\n");
}

static void
xml_param (SerializeState *ss,
           gint            indent,
           const gchar    *key,
           const gchar    *value)
{
  g_assert (key);
  g_assert (value);

  if (value)
  {
    xml_param_start (ss, indent, key);
    g_string_append (ss->buf, value);
    xml_param_end (ss);
  }
}

static void
xml_curve_point (SerializeState *ss,
                 gint            indent,
                 gfloat          x,
                 gfloat          y)
{
  gchar str[64];
  ind; g_string_append (ss->buf, "<curve-point x='");
  g_ascii_dtostr (str, sizeof(str), x);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "' y='");
  g_ascii_dtostr (str, sizeof(str), y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "'/>\n");
}

static void
xml_curve (SerializeState *ss,
           gint            indent,
           GeglCurve      *curve)
{
  gchar str[G_ASCII_DTOSTR_BUF_SIZE];
  gdouble min_y, max_y;
  guint num_points = gegl_curve_num_points (curve);
  guint i;

  gegl_curve_get_y_bounds (curve, &min_y, &max_y);

  ind; g_string_append (ss->buf, "<curve ymin='");
  g_ascii_dtostr (str, sizeof(str), min_y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "' ymax='");
  g_ascii_dtostr (str, sizeof(str), max_y);
  g_string_append (ss->buf, str);
  g_string_append (ss->buf, "'>\n");
  for (i = 0; i < num_points; ++i)
    {
      gdouble x, y;
      gegl_curve_get_point (curve, i, &x, &y);
      xml_curve_point (ss, indent + 2, x, y);
    }
  ind; g_string_append (ss->buf, "</curve>\n");
}

static void
serialize_properties (SerializeState *ss,
                      gint            indent,
                      GeglNode       *node)
{
  GParamSpec **properties;
  guint        n_properties;
  gint         i;

  properties = gegl_operation_list_properties (gegl_node_get_operation (node),
                                               &n_properties);

  for (i = 0; i < n_properties; i++)
    {
      if (strcmp (properties[i]->name, "input") &&
          strcmp (properties[i]->name, "output") &&
          strcmp (properties[i]->name, "aux") &&
          strcmp (properties[i]->name, "aux2"))
        {
          if (g_type_is_a (G_PARAM_SPEC_TYPE (properties[i]),
                           GEGL_TYPE_PARAM_FILE_PATH))
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);

              if (value)
                {
                  if (ss->path_root &&
                      !strncmp (ss->path_root, value, strlen (ss->path_root)))
                    {
                      xml_param (ss, indent, properties[i]->name, &value[strlen (ss->path_root) + 1]);
                    }
                  else
                    {
                      xml_param (ss, indent, properties[i]->name, value);
                    }
                }

              g_free (value);
            }
          else if (properties[i]->value_type == G_TYPE_FLOAT)
            {
              gfloat value;
              gchar  str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_ascii_dtostr (str, sizeof(str), value);
              xml_param (ss, indent, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_DOUBLE)
            {
              gdouble value;
              gchar   str[G_ASCII_DTOSTR_BUF_SIZE];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_ascii_dtostr (str, sizeof(str), value);
              xml_param (ss, indent, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_INT)
            {
              gint  value;
              gchar str[64];
              gegl_node_get (node, properties[i]->name, &value, NULL);
              g_snprintf (str, sizeof (str), "%i", value);
              xml_param (ss, indent, properties[i]->name, str);
            }
          else if (properties[i]->value_type == G_TYPE_BOOLEAN)
            {
              gboolean value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              if (value)
                {
                  xml_param (ss, indent, properties[i]->name, "true");
                }
              else
                {
                  xml_param (ss, indent, properties[i]->name, "false");
                }
            }
          else if (properties[i]->value_type == G_TYPE_STRING)
            {
              gchar *value;
              gegl_node_get (node, properties[i]->name, &value, NULL);
              xml_param (ss, indent, properties[i]->name, value);
              g_free (value);
            }
          else if (g_type_is_a (properties[i]->value_type, G_TYPE_ENUM))
            {
              GEnumClass *eclass = g_type_class_peek (properties[i]->value_type);
              GEnumValue *evalue;
              gint value;

              gegl_node_get (node, properties[i]->name, &value, NULL);
              evalue = g_enum_get_value (eclass, value);

              xml_param (ss, indent, properties[i]->name, evalue->value_nick);
            }
          else if (properties[i]->value_type == GEGL_TYPE_COLOR)
            {
              GeglColor *color;
              gchar     *value;
              gegl_node_get (node, properties[i]->name, &color, NULL);
              g_object_get (color, "string", &value, NULL);
              g_object_unref (color);
              xml_param (ss, indent, properties[i]->name, value);
              g_free (value);
            }
          else if (properties[i]->value_type == GEGL_TYPE_CURVE)
            {
              GeglCurve *curve;
              gegl_node_get (node, properties[i]->name, &curve, NULL);
              xml_param_start (ss, indent, properties[i]->name);
              g_string_append (ss->buf, "\n");
              xml_curve (ss, indent + 2, curve);
              ind; xml_param_end (ss);
              g_object_unref (curve);
            }
          else if (properties[i]->value_type == GEGL_TYPE_PATH)
            {
              gchar *svg_path;
              GeglPath *path;
              gegl_node_get (node, properties[i]->name, &path, NULL);
              xml_param_start (ss, indent, properties[i]->name);
              svg_path = gegl_path_to_string (path);
              g_string_append (ss->buf, svg_path);
              xml_param_end (ss);

              g_object_unref (path);
            }
          else
            {
              g_warning ("%s: serialization of %s properties not implemented",
                         properties[i]->name, g_type_name (properties[i]->value_type));
            }
        }
    }

  g_free (properties);
}

static void
serialize_node (SerializeState *ss,
                gint            indent,
                GeglNode       *node)
{
  char*       input_name[3] = {"input", "aux", "aux2"};
  GeglNode*   producer[3];
  char*       producer_pad[3];
  gint        i;
  char       *id;
  const char *op;

  if (!node)
    return;

  op = gegl_node_get_operation (node);

  while (node && op && !strcmp (op, "gegl:nop")) {
    node = gegl_node_get_producer (node, "input", NULL);
    op = gegl_node_get_operation (node);
  }

  if (!node || !op) /* non-connected node or empty node */
    return;

  if (!strcmp (op, "GraphNode"))
  {
    g_warning ("Subgraph support non implemented yet.");
    return;
  }

  id = g_strdup_printf ("node%i", ss->counter);
  ss->counter++;

  /* Make sure every producer is already serialized. */
  for (i = 0; i < 3; i++)
  {
    producer[i] = gegl_node_get_producer (node, input_name[i], &producer_pad[i]);
    if (producer[i] && ! g_hash_table_lookup (ss->nodes, producer[i]))
      serialize_node (ss, indent, producer[i]);
  }

  ind; g_string_append (ss->buf, "<node");
  xml_attr (ss->buf, "id", id);
  xml_attr (ss->buf, "op", op);
  g_string_append (ss->buf, ">\n");

  /* edges */
  indent += 2;
  for (i = 0; i < 3; i++)
  {
    if (producer[i])
      {
        char *id = g_hash_table_lookup (ss->nodes, producer[i]);
        ind; g_string_append (ss->buf, "<edge");
        xml_attr (ss->buf, "from", producer_pad[i]);
        xml_attr (ss->buf, "of", id);
        xml_attr (ss->buf, "to", input_name[i]);
        g_string_append (ss->buf, " />\n");
      }
  }

  serialize_properties (ss, indent, node);

  indent -= 2;

  ind; g_string_append (ss->buf, "</node>\n\n");

  g_hash_table_insert (ss->nodes, node, id);
}

gchar *
gegl_node_to_xml_v2 (GeglNode    *gegl,
                     const gchar *path_root)
{
  SerializeState  ss;

  ss.buf         = g_string_new ("");
  ss.path_root   = path_root;
  ss.nodes       = g_hash_table_new (NULL, NULL);
  ss.counter     = 0;

  g_string_append (ss.buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (ss.buf, "<gegl>\n");

  serialize_node (&ss, 2, gegl);

  g_string_append (ss.buf, "</gegl>\n");
  g_hash_table_destroy (ss.nodes);

  return g_string_free (ss.buf, FALSE);
}

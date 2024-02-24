/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2016, 2017 Øyvind Kolås
 */

#include "config.h"
#include "gegl.h"
#include <glib/gi18n-lib.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include "property-types/gegl-paramspecs.h"
#include "property-types/gegl-audio-fragment.h"

#ifdef G_OS_WIN32
#include <direct.h>
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

//#define make_rel(strv) (g_ascii_strtod (strv, NULL) * gegl_node_get_bounding_box
// (iter[0]).height)
#define make_rel(strv) (g_ascii_strtod (strv, NULL) * rel_dim)

static void
remove_in_betweens (GeglNode *nop_raw,
                    GeglNode *nop_transformed)
{
  GeglNode *iter =  nop_raw;
  GList *collect = NULL;

  while (iter && iter != nop_transformed)
    {
      GeglNode **nodes = NULL;
      int count = gegl_node_get_consumers (iter, "output", &nodes, NULL);
      if (count)
        iter = nodes[0];
      else
        iter = NULL;
      g_free (nodes);
      if (iter && iter != nop_transformed)
        collect = g_list_append (collect, iter);
    }
  while (collect)
    {
      g_object_unref (collect->data);
      collect = g_list_remove (collect, collect->data);
    }
}

static void each_knot (const GeglPathItem *path_node,
                       gpointer user_data)
{
  GString *str = user_data;
  gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (fstr, sizeof(fstr), path_node->point[0].x);
  g_string_append_printf (str, " %s=", fstr);
  g_ascii_dtostr (fstr, sizeof(fstr), path_node->point[0].y);
  g_string_append_printf (str, "%s ", fstr);
}

static void each_knot_rel (const GeglPathItem *path_node,
                           gpointer user_data)
{
  GString *str = user_data;
  gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (fstr, sizeof(fstr), path_node->point[0].x);
  g_string_append_printf (str, " %s=", fstr);
  g_ascii_dtostr (fstr, sizeof(fstr), path_node->point[0].y);
  g_string_append_printf (str, "%srel ", fstr);
}


void
gegl_node_set_time (GeglNode   *node,
                    gdouble     time)
{
  GeglNode *iter = NULL;
  if (!node)
    return;

  if (gegl_node_has_pad (node, "input"))
  {
    iter = gegl_node_get_producer (node, "input", NULL);
    if (iter)
      gegl_node_set_time (iter, time);
  }
  if (gegl_node_has_pad (node, "aux"))
  {
    iter = gegl_node_get_producer (node, "aux", NULL);
    if (iter)
      gegl_node_set_time (iter, time);
  }

  {
    gint i;
    guint n_properties;
    GParamSpec **properties;

    properties = gegl_operation_list_properties (gegl_node_get_operation (
                                                   node),
                                                 &n_properties);
    for (i = 0; i < n_properties; i++)
      {
        const gchar *property_name = g_param_spec_get_name (
          properties[i]);
        GType property_type = G_PARAM_SPEC_VALUE_TYPE (properties[i]);
        char tmpbuf[1024];
        GeglPath *anim_path = NULL;
        GQuark anim_quark;//, rel_quark;
        sprintf (tmpbuf, "%s-anim", property_name);
        anim_quark = g_quark_from_string (tmpbuf);
        anim_path = g_object_get_qdata (G_OBJECT (node), anim_quark);

        if (property_type == G_TYPE_FLOAT)
          {
            if (anim_path)
            {
              gdouble y;
              gegl_path_calc_y_for_x (anim_path, time, &y);
              gegl_node_set (node, property_name, y, NULL);
            }
          }
        else if (property_type == G_TYPE_DOUBLE)
          {
            if (anim_path)
            {
              gdouble y;
              gegl_path_calc_y_for_x (anim_path, time, &y);
              gegl_node_set (node, property_name, y, NULL);
            }
          }
        else if (property_type == G_TYPE_INT)
          {
            if (anim_path)
            {
              gdouble y;
              gegl_path_calc_y_for_x (anim_path, time, &y);
              gegl_node_set (node, property_name, (int)y, NULL);
            }
          }
        else if (property_type == G_TYPE_UINT)
          {
            if (anim_path)
            {
              gdouble y;
              gegl_path_calc_y_for_x (anim_path, time, &y);
              gegl_node_set (node, property_name, (guint32)y, NULL);
            }
          }
        else if (property_type == G_TYPE_BOOLEAN)
          {
          }
        else if (property_type == G_TYPE_STRING)
          {
          }
        else if (g_type_is_a (property_type, G_TYPE_ENUM))
          {
          }
        else if (property_type == GEGL_TYPE_COLOR)
          {
          }
        else if (property_type == GEGL_TYPE_PATH)
          {
          }
        else if (property_type == G_TYPE_POINTER &&
                 GEGL_IS_PARAM_SPEC_FORMAT (properties[i]))
          {
          }
        else
          {
          }

      }
    //g_free (properties);
  }
}


#if 0
static char *
gegl_migrate_saturation_0_0_to_1_0 (const char  *input)
{
  //params_add_entry (params, "colorspace=CIE-lab");
  //return TRUE;
  return g_strdup (input);
}




gboolean
gegl_migrate_api (GeglNode    *node,
                  const char  *operation,
                  char       **params)
{
  // if there is no opi entry, nothing to migrate
  // if we find opi key.. look for valid migration

  return TRUE;
}
#endif

#define GEGL_CHAIN_MAX_LEVEL 10

static void
gegl_node_connect_safe (GeglNode *node_a, const char *pad_a,
                        GeglNode *node_b, const char *pad_b,
                        GError **error)
{
  if (*error) return;
  if (!gegl_node_has_pad (node_a, pad_a))
    *error = g_error_new (g_quark_from_static_string("gegl"),0,
                           _("%s does not have a pad called %s"),
                           gegl_node_get_operation (node_a), pad_a);
  else if (!gegl_node_has_pad (node_b, pad_b))
        *error = g_error_new (g_quark_from_static_string("gegl"),0,
                           _("%s does not have a pad called %s"),
                           gegl_node_get_operation (node_b), pad_b);
      else
  gegl_node_connect(node_a, pad_a, node_b, pad_b);
}

void
gegl_create_chain_argv (char      **argv,
                        GeglNode   *start,
                        GeglNode   *proxy,
                        double      time,
                        int         rel_dim,
                        const char *path_root,
                        GError    **error)
{
  GeglNode   *new = NULL;
  gchar     **arg = argv;
  int level = 0;
  GeglNode   *iter[GEGL_CHAIN_MAX_LEVEL] = {start, NULL};
  char       *level_op[GEGL_CHAIN_MAX_LEVEL];
  char       *level_pad[GEGL_CHAIN_MAX_LEVEL];
  int in_keyframes = 0;
  int in_strkeyframes = 0;
  char       *prop = NULL;
  GHashTable *ht = NULL;
  GeglPath   *path = NULL;
  GString    *string = NULL;
  GeglNode **ret_sinkp = NULL;
#if 0
  // for opi migration to work we need to parse into a hashtable of keys..
  // that later get applied..

  // in this database we collect *string values* including strings for animations
  // the opi migration occurs on this list of key/value pairs along with the
  // op name before reaching 

  char      *op_args[64]={NULL, };
  int        n_op_args = 0;
#endif

  if (error && *error)
  {
    GeglNode **an = (void*)error;
    ret_sinkp = (void*)*an;
    *error = NULL;
  }

  remove_in_betweens (start, proxy);

  level_op[level] = 0;//*arg;

  ht = g_hash_table_new (g_str_hash, g_str_equal);

  while (*arg && !*error)
    {
      if (in_keyframes)
        {
          char *key = g_strdup (*arg);
          char *eq = strchr (key, '=');
          char *value = NULL;

          if (eq)
            {
              value = eq + 1;
              value[-1] = '\0';
              if (strstr (value, "rel"))
                {
                  char tmpbuf[1024];
                  GQuark rel_quark;
                  gegl_path_append (path, 'L', g_ascii_strtod (key, NULL),
                                    make_rel (value));

                  sprintf (tmpbuf, "%s-rel", prop);
                  rel_quark = g_quark_from_string (tmpbuf);
                  g_object_set_qdata_full (G_OBJECT (
                                           new),
                                           rel_quark,
                                           g_strdup (value), g_free);
                }
              else
                {
                  gegl_path_append (path, 'L', g_ascii_strtod (key, NULL),
                                    g_ascii_strtod (value, NULL));
                }
            }
          else
            {
              if (!(g_str_equal (key, "}")))
              {
                if (error)
                  {
                    char *error_str = g_strdup_printf (
                       _("unhandled path data %s:%s\n"), key, value);
                    *error = g_error_new_literal (
                                g_quark_from_static_string ( "gegl"),
                                0, error_str);
                    g_free (error_str);
                  }
              }
            }

          g_free (key);

          if (strchr (*arg, '}'))
            {
              gdouble y = 0;
              char tmpbuf[1024];
              GQuark anim_quark;
              sprintf (tmpbuf, "%s-anim", prop);
              anim_quark = g_quark_from_string (tmpbuf);

              if (time == 0.0) /* avoiding ugly start interpolation artifact */
                time = 0.001;
              gegl_path_calc_y_for_x (g_object_get_qdata (G_OBJECT (new),
                                      anim_quark), time, &y);

              {
                GParamSpec *pspec = gegl_operation_find_property (gegl_node_get_operation (new), prop);

                if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
                {
                  GParamSpecDouble *double_spec = (void*)pspec;
                  if (y <= double_spec->minimum)
                    y = double_spec->minimum;
                  if (y >= double_spec->maximum)
                    y = double_spec->maximum;
                  gegl_node_set (new, prop, y, NULL);
                }
                else if (GEGL_IS_PARAM_SPEC_INT (pspec))
                {
                  GParamSpecInt *int_spec = (void*)pspec;
                  if (y <= int_spec->minimum)
                    y = int_spec->minimum;
                  if (y >= int_spec->maximum)
                    y = int_spec->maximum;
                  gegl_node_set (new, prop, (int)(y), NULL);
                }
              }

              in_keyframes = 0;
            }
          ;
        }
      else if (in_strkeyframes)
        {
          char *key = g_strdup (*arg);
          char *eq = strchr (key, '=');
          char *value = NULL;

          if (eq)
            {
              value = eq + 1;
              value[-1] = '\0';
              if (g_ascii_strtod (key, NULL) <= time)
                g_string_assign (string, value);
            }

          g_free (key);

          if (strchr (*arg, '}'))
            {
              gegl_node_set (new, prop, string->str, NULL);
              in_strkeyframes = 0;
            }
          ;
        }
      else if (!strchr(*arg, '=') && strchr (*arg, ']'))
        {
          level--;
          if (level < 0) level = 0;
          gegl_node_connect_safe (iter[level+1], "output", iter[level],
                             level_pad[level], error);
        }
      else
        {
          if (strchr (*arg, '=')) /* contains = sign, must be a property
                                     assignment */
            {
              char *match = strchr (*arg, '=');
              {
                GType target_type = 0;
                GParamSpec *pspec = NULL;
                GValue gvalue = {0,};
                char *key = g_strdup (*arg);
                char *value = strchr (key, '=') + 1;
                int end_block = 0;
                value[-1] = '\0';
                if (strchr (value, ']') &&
                    strrchr (value, ']')[1] == '\0')
                  {
                    end_block = 1;
                    *strchr (value, ']') = 0;
                  }

                if (!strcmp (key, "id"))
                  {
                    g_hash_table_insert (ht, (void*)g_intern_string (
                                           value), iter[level]);
                    g_object_set_data (G_OBJECT(iter[level]),
                                       "refname",
                                       (void*)g_intern_string (value));
                  }
                else if (!strcmp (key, "ref"))
                  {
                    if (g_hash_table_lookup (ht, g_intern_string (value)))
                      iter[level] =
                        g_hash_table_lookup (ht, g_intern_string (value));
                    else
                      g_warning ("unknown id '%s'", value);
                  }
                else if (!strcmp (key, "opi"))
                  {
                    /* should check for incompatibility rather than difference
                     */
                    fprintf (stderr, "{%s}", value);
                    if (!g_str_equal (value,
                                      gegl_operation_get_op_version (level_op[
                                                                       level])))
                      {
                        /* for now - just reporting it */
                        g_print ("operation property interface version mismatch for %s\n"
                                 "parsed %s but GEGL library has %s\n",
                                 level_op[level], value, gegl_operation_get_op_version (
                                   level_op[level]));
                      }
                  }
                else
                  {
                    unsigned int n_props = 0;
                    int i;
                    if (level_op[level])
                      {
                        GParamSpec **pspecs;
                        pspecs = gegl_operation_list_properties (
                          level_op[level], &n_props);
                        for (i = 0; i < n_props; i++)
                          {
                            if (!strcmp (pspecs[i]->name, key))
                              {
                                target_type = pspecs[i]->value_type;
                                pspec = pspecs[i];
                                break;
                              }
                          }
                      }

                    if (match[1] == '{')
                      {
                        char *key = g_strdup (*arg);
                        char *value = strchr (key, '=') + 1;
                        value[-1] = '\0';

                        if (g_type_is_a (target_type, G_TYPE_STRING))
                          {
                            if (string){g_string_assign(string, "");}else string = g_string_new ("");
                            in_strkeyframes = 1;
                            g_free (prop);
                            prop = g_strdup (key);
                          }
                        else
                          {
                            char tmpbuf[1024];
                            GQuark anim_quark;
                            sprintf (tmpbuf, "%s-anim", key);
                            anim_quark = g_quark_from_string (tmpbuf);
                            path = gegl_path_new ();
                            in_keyframes = 1;
                            g_free (prop);
                            prop = g_strdup (key);

                            g_object_set_qdata_full (G_OBJECT (
                                                       new), anim_quark, path,
                                                     g_object_unref);
                          }

                        g_free (key);
                      }
                    else if (match[1] == '[')
                      {
                        char *pad = g_strdup (*arg);
                        char *value = strchr (pad, '=') + 1;
                        value[-1] = '\0';
                        level_pad[level] = (void*)g_intern_string(pad);
                        g_free (pad);
                        level++;
                        if (level >= GEGL_CHAIN_MAX_LEVEL)
                        {
                          break;
                        }
                        iter[level] = NULL;
                        level_op[level] = NULL;
                        level_pad[level] = NULL;

                        if (strlen (&match[2]))
                          {
                            if (strchr (&match[2], ':')) /* contains : is a
                                                            non-prefixed
                                                            operation */
                              {
                                level_op[level] =
                                  (void*)g_intern_string(&match[2]);
                              }
                            else /* default to gegl: as prefix if no : specified
                                  */
                              {
                                char temp[1024];
                                g_snprintf (temp, 1023, "gegl:%s", &match[2]);
                                level_op[level] = (void*)g_intern_string (temp);
                              }

                            if (gegl_has_operation (level_op[level]))
                              {
                                new = gegl_node_new_child (gegl_node_get_parent (
                                                             proxy), "operation",
                                                           level_op[level],
                                                           NULL);

                                if (iter[level])
                                  gegl_node_link_many (iter[level], new, proxy,
                                                       NULL);
                                else
                                  gegl_node_link_many (new, proxy, NULL);
                                iter[level] = new;
                              }
                            else if (error)
                              {
                                GString *str = g_string_new ("");
                                g_string_append_printf (str,
                                                        _("op '%s' not found, partial matches: "),
                                                        level_op[level]);

                                *error = g_error_new_literal (g_quark_from_static_string (
                                                                "gegl"),
                                                              0, str->str);
                                g_string_free (str, TRUE);
                              }
                          }
                        /* XXX: ... */
                      }
                    else
                    if (target_type == 0)
                      {
                        if (error && level_op[level] &&
                            gegl_has_operation (level_op[level]))
                          {
                            unsigned int n_props = 0;
                            int i;
                            GParamSpec **pspecs;

                            GString *str = g_string_new ("");
                            pspecs =
                              gegl_operation_list_properties (level_op[level],
                                                              &n_props);

                            if (n_props <= 0)
                              {
                                g_string_append_printf (str,
                                                        _("%s has no %s property."),
                                                        level_op[level], key);
                              }
                            else
                              {
                                g_string_append_printf (str,
                                                        _("%s has no %s property, properties: "),
                                                        level_op[level], key);

                                for (i = 0; i < n_props; i++)
                                  {
                                    g_string_append_printf (str, "'%s', ",
                                                            pspecs[i]->name);
                                  }
                              }

                            *error = g_error_new_literal (g_quark_from_static_string (
                                                            "gegl"),
                                                          0, str->str);
                            g_string_free (str, TRUE);
                          }
                      }
                    else
                    if (g_type_is_a (target_type, G_TYPE_DOUBLE) ||
                        g_type_is_a (target_type, G_TYPE_FLOAT)  ||
                        g_type_is_a (target_type, G_TYPE_INT)    ||
                        g_type_is_a (target_type, G_TYPE_UINT))
                      {
                        if (strstr (value, "rel"))
                          {
                            char tmpbuf[1024];
                            GQuark rel_quark;
                            sprintf (tmpbuf, "%s-rel", key);
                            rel_quark = g_quark_from_string (tmpbuf);

                            g_object_set_qdata_full (G_OBJECT (
                                                       new),
                                                     rel_quark,
                                                       g_strdup (
                                                       value), g_free);

                            if (g_type_is_a (target_type, G_TYPE_INT))
                              gegl_node_set (iter[level], key,
                                             (int)make_rel (value), NULL);
                            else if (g_type_is_a (target_type, G_TYPE_UINT))
                              gegl_node_set (iter[level], key,
                                             (guint)make_rel (value), NULL);
                            else
                              gegl_node_set (iter[level], key,  make_rel (
                                               value), NULL);
                          }
                        else
                          {
                            if (g_type_is_a (target_type, G_TYPE_INT))
                              gegl_node_set (iter[level], key,
                                             (int)g_ascii_strtod (value, NULL), NULL);
                            else if (g_type_is_a (target_type, G_TYPE_UINT))
                              gegl_node_set (iter[level], key,
                                             (guint)g_ascii_strtod (value, NULL), NULL);
                            else
                              gegl_node_set (iter[level], key,
                                             g_ascii_strtod (value, NULL), NULL);
                          }
                      }
                    else if (g_type_is_a (target_type, G_TYPE_BOOLEAN))
                      {
                        if (!strcmp (value,
                                     "true") || !strcmp (value, "TRUE") ||
                            !strcmp (value, "YES") || !strcmp (value, "yes") ||
                            !strcmp (value, "Yes") || !strcmp (value, "True") ||
                            !strcmp (value, "y") || !strcmp (value, "Y") ||
                            !strcmp (value, "1") || !strcmp (value, "on"))
                          {
                            gegl_node_set (iter[level], key, TRUE, NULL);
                          }
                        else
                          {
                            gegl_node_set (iter[level], key, FALSE, NULL);
                          }
                      }
                    else if (target_type == GEGL_TYPE_COLOR)
                      {
                        GeglColor *color = g_object_new (GEGL_TYPE_COLOR,
                                                         "string", value, NULL);
                        gegl_node_set (iter[level], key, color, NULL);
                      }
                    else if (target_type == GEGL_TYPE_PATH)
                      {
                        GeglPath *path = gegl_path_new ();
                        gegl_path_parse_string (path, value);
                        gegl_node_set (iter[level], key, path, NULL);
                      }
                    else if (target_type == G_TYPE_POINTER &&
                             GEGL_IS_PARAM_SPEC_FORMAT (pspec))
                      {
                        const Babl *format = NULL;

                        if (value[0] && babl_format_exists (value))
                          format = babl_format (value);
                        else if (error)
                          {
                            char *error_str = g_strdup_printf (
                                  _("BablFormat \"%s\" does not exist."),
                                  value);
                            *error = g_error_new_literal (
                                        g_quark_from_static_string ( "gegl"),
                                        0, error_str);
                            g_free (error_str);
                          }

                        gegl_node_set (iter[level], key, format, NULL);
                      }
                    else if (g_type_is_a (G_PARAM_SPEC_TYPE (pspec),
                             GEGL_TYPE_PARAM_FILE_PATH))
                      {
                        gchar *buf;
                        if (g_path_is_absolute (value))
                          {
                            gegl_node_set (iter[level], key, value, NULL);
                          }
                        else
                          {
                            gchar *absolute_path;
                            if (path_root)
                              buf = g_strdup_printf ("%s/%s", path_root, value);
                            else
                              buf = g_strdup_printf ("./%s", value);
                            absolute_path = realpath (buf, NULL);
                            g_free (buf);
                            if (absolute_path)
                              {
                                gegl_node_set (iter[level], key, absolute_path, NULL);
                                free (absolute_path);
                              }
                            else
                              gegl_node_set (iter[level], key, value, NULL);
                          }
                      }
                    else if (g_type_is_a (target_type, G_TYPE_STRING))
                      {
                        gegl_node_set (iter[level], key, value, NULL);
                      }
                    else if (g_type_is_a (target_type, G_TYPE_ENUM))
                      {
                        GEnumClass *eclass = g_type_class_peek (target_type);
                        GEnumValue *evalue = g_enum_get_value_by_nick (eclass,
                                                                       value);
                        if (evalue)
                          {
                            gegl_node_set (new, key, evalue->value, NULL);
                          }
                        else
                          {
                            if (error)
                            {
                              GString *str = g_string_new ("");

                              g_string_append_printf (str,
                              "unhandled enum value: %s\n", value);

                              g_string_append_printf (str, "accepted values:");
                              for (int i = 0; i < eclass->n_values; i++)
                              {
                                g_string_append_printf (str, " %s", eclass->values[i].value_nick);
                              }

                              *error = g_error_new_literal (
                                  g_quark_from_static_string ( "gegl"), 0, str->str);
                              g_string_free (str, TRUE);
                            }
                          }
                      }
                    else
                      {
                        GValue gvalue_transformed = {0,};
                        g_value_init (&gvalue, G_TYPE_STRING);
                        g_value_set_string (&gvalue, value);
                        g_value_init (&gvalue_transformed, target_type);
                        g_value_transform (&gvalue, &gvalue_transformed);
                        gegl_node_set_property (iter[level], key,
                                                &gvalue_transformed);
                        g_value_unset (&gvalue);
                        g_value_unset (&gvalue_transformed);
                      }
                  }
                g_free (key);
                if (end_block && level >0)
                  {
                    level--;
                    gegl_node_connect_safe (iter[level+1], "output",
                                            iter[level], level_pad[level],
                                            error);
                  }
              }
            }
          else
            {
              if (strchr (*arg, ':')) /* contains : is a non-prefixed operation
                                       */
                {
                  level_op[level] = *arg;
                }
              else /* default to gegl: as prefix if no : specified */
                {
                  char temp[1024];
                  g_snprintf (temp, 1023, "gegl:%s", *arg);
                  level_op[level] = (void*)g_intern_string (temp);
                }


              if (gegl_has_operation (level_op[level]))
                {
                  new = gegl_node_new_child (gegl_node_get_parent (
                                               proxy), "operation",
                                             level_op[level], NULL);

                  if (gegl_node_has_pad (new, "output"))
                  {
                    if (iter[level] && gegl_node_has_pad (new, "input"))
                      gegl_node_link_many (iter[level], new, proxy, NULL);
                    else
                      gegl_node_link_many (new, proxy, NULL);
                  }
                  else
                  {
                    gegl_node_link_many (iter[level], new, NULL);
                  }
                  iter[level] = new;
                }
              else if (error)
                {
                  GString *str = g_string_new ("");
                  guint n_operations;
                  gchar  **operations = gegl_list_operations (&n_operations);
                  gint i;
                  gint started = 0;
                  gint max = 12;

                  g_string_append_printf (str, _("No such op '%s'"),
                                          level_op[level]);

                  for (i = 0; i < n_operations; i++)
                    {
                      if (g_str_has_prefix (operations[i], level_op[level]))
                        {
                          if (!started)
                            {
                              started = 1;
                              g_string_append_printf (str," suggestions:");
                            }
                          if (max-- > 0)
                            g_string_append_printf (str, " %s", operations[i]);
                        }
                    }
                  if (!started)
                    for (i = 0; i < n_operations; i++)
                      {
                        if (strstr (operations[i], *arg))
                          {
                            if (!started)
                              {
                                started = 1;
                                g_string_append_printf (str," suggestions:");
                              }
                            if (max-- > 0)
                              g_string_append_printf (str, " %s",
                                                      operations[i]);
                          }
                      }

                  g_free (operations);
                  *error =
                    g_error_new_literal (g_quark_from_static_string ("gegl"),
                                         0, str->str);
                  g_string_free (str, TRUE);
                }
            }
        }
      arg++;
    }


  while (level > 0 && !*error)
    {
      level--;

      gegl_node_connect_safe (iter[level+1], "output",
                              iter[level], level_pad[level], error);
    }

  g_free (prop);
  g_hash_table_unref (ht);


  if (gegl_node_has_pad (iter[level], "output"))
    gegl_node_link (iter[level], proxy);
  else
  {
    if (ret_sinkp)
    {
      *ret_sinkp = iter[level];
    }
  }
}

void
gegl_create_chain (const char *str, GeglNode *op_start, GeglNode *op_end,
                   double time, int rel_dim, const char *path_root,
                   GError **error)
{
  gchar **argv = NULL;
  gint argc = 0;

  g_shell_parse_argv (str, &argc, &argv, NULL);
  if (argv)
    {
      gegl_create_chain_argv (argv, op_start, op_end, time, rel_dim, path_root,
                              error);
      g_strfreev (argv);
    }
}

static gchar *
gegl_serialize2 (GeglNode         *start,
                 GeglNode         *end,
                 const char       *basepath,
                 GHashTable       *ht,
                 GeglSerializeFlag flags)
{
  gboolean trim_defaults = flags & GEGL_SERIALIZE_TRIM_DEFAULTS;
  gboolean bake_anim = flags & GEGL_SERIALIZE_BAKE_ANIM;
  GeglNode *iter;

  GString *str = g_string_new ("");

  if (start == NULL && 0)
    {
      start = end;
      while (gegl_node_get_producer (start, "input", NULL))
        start = gegl_node_get_producer (start, "input", NULL);
    }


  iter = end;
  while (iter)
    {
      GeglNode **nodes = NULL;
      int count = gegl_node_get_consumers (iter, "output", &nodes, NULL);
      if (count>1)
        {
          int val;
          int last = 0;
          int cnt = 0;

          if ((val = GPOINTER_TO_INT (g_hash_table_lookup (ht, iter))))
            {
              cnt = val - 1;
              g_hash_table_insert (ht, iter, GINT_TO_POINTER (cnt));
              if (cnt == 1)
                last = 1;
            }
          else
            {
              g_hash_table_insert (ht, iter, GINT_TO_POINTER (count));
              cnt = count;
            }

          {
            gchar *str3;

            gchar *refname = g_object_get_data (G_OBJECT (iter), "refname");

            if (refname)
              str3  = g_strdup_printf (" %s=%s\n", last ? "id" : "ref", refname);
            else
              str3  = g_strdup_printf (" %s=%p\n", last ? "id" : "ref", iter);
            g_string_prepend (str, str3);
            g_free (str3);
          }
          /* if this is not the last reference to it,. keep recursing
           */
          if (!last)
            iter = NULL;
        }
      g_free (nodes);

      if (iter == start || !iter)
        {
          iter = NULL;
        }
      else
        {
          GString *s2 = g_string_new ("");
          const char *op_name = gegl_node_get_operation (iter);
          if (!(flags & GEGL_SERIALIZE_INDENT))
            g_string_append_printf (s2, " ");
          g_string_append_printf (s2, "%s", op_name);
          if (flags & GEGL_SERIALIZE_VERSION)
            g_string_append_printf (s2, " opi=%s", gegl_operation_get_op_version (
                                      op_name));
          if (flags & GEGL_SERIALIZE_INDENT)
            g_string_append_printf (s2, "\n");
          {
            gint i;
            guint n_properties;
            GParamSpec **properties;
            gboolean printed = FALSE;

            properties = gegl_operation_list_properties (gegl_node_get_operation (
                                                           iter),
                                                         &n_properties);
            for (i = 0; i < n_properties; i++)
              {
                const gchar *property_name = g_param_spec_get_name (
                  properties[i]);
                GType property_type = G_PARAM_SPEC_VALUE_TYPE (properties[i]);
                const GValue*default_value = g_param_spec_get_default_value (
                  properties[i]);
                char tmpbuf[1024];
                GeglPath *anim_path = NULL;
                char *rel_orig = NULL;
                GQuark anim_quark, rel_quark;
                sprintf (tmpbuf, "%s-anim", property_name);
                anim_quark = g_quark_from_string (tmpbuf);
                sprintf (tmpbuf, "%s-rel", property_name);
                rel_quark = g_quark_from_string (tmpbuf);
                if (!bake_anim)
                  anim_path = g_object_get_qdata (G_OBJECT (iter), anim_quark);
                rel_orig = g_object_get_qdata (G_OBJECT (iter), rel_quark);

                if (property_type == G_TYPE_FLOAT)
                  {
                    gfloat defval = g_value_get_float (default_value);
                    gfloat value;
                    gegl_node_get (iter, property_name, &value, NULL);

                    if (anim_path)
                    {
                      g_string_append_printf (s2, " %s={ ", property_name);
                      if (rel_orig)
                        gegl_path_foreach (anim_path, each_knot_rel, s2);
                      else
                        gegl_path_foreach (anim_path, each_knot, s2);
                      g_string_append_printf (s2, " } ");
                    }
                    else if (value != defval || (!trim_defaults))
                      {
                        gchar str[G_ASCII_DTOSTR_BUF_SIZE];
                        g_ascii_dtostr (str, sizeof(str), value);
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_string_append_printf (s2, " %s=%s%s", property_name,
                                                str, rel_orig?"rel":"");
                        printed = TRUE;
                      }
                  }
                else if (property_type == G_TYPE_DOUBLE)
                  {
                    gdouble defval = g_value_get_double (default_value);
                    gdouble value;
                    gegl_node_get (iter, property_name, &value, NULL);

                    if (anim_path)
                    {
                      g_string_append_printf (s2, " %s={ ", property_name);
                      if (rel_orig)
                        gegl_path_foreach (anim_path, each_knot_rel, s2);
                      else
                        gegl_path_foreach (anim_path, each_knot, s2);
                      g_string_append_printf (s2, " } ");
                    }
                    else if (value != defval || (!trim_defaults))
                      {
                        gchar str[G_ASCII_DTOSTR_BUF_SIZE];
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_ascii_dtostr (str, sizeof(str), value);
                        g_string_append_printf (s2, " %s=%s%s", property_name,
                                                str, rel_orig?"rel":"");
                        printed = TRUE;
                      }
                  }
                else if (property_type == G_TYPE_INT)
                  {
                    gint defval = g_value_get_int (default_value);
                    gint value;
                    gchar str[64];
                    gegl_node_get (iter, property_name, &value, NULL);

                    if (anim_path)
                    {
                      g_string_append_printf (s2, " %s={ ", property_name);
                      if (rel_orig)
                        gegl_path_foreach (anim_path, each_knot_rel, s2);
                      else
                        gegl_path_foreach (anim_path, each_knot, s2);
                      g_string_append_printf (s2, " } ");
                    }
                    else if (value != defval || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_snprintf (str, sizeof (str), "%i", value);
                        g_string_append_printf (s2, " %s=%s%s", property_name,
                                                str, rel_orig?"rel":"");
                        printed = TRUE;
                      }
                  }
                else if (property_type == G_TYPE_UINT)
                  {
                    guint defval = g_value_get_uint (default_value);
                    guint value;
                    gchar str[64];
                    gegl_node_get (iter, property_name, &value, NULL);

                    if (anim_path)
                    {
                      g_string_append_printf (s2, " %s={ ", property_name);
                      if (rel_orig)
                        gegl_path_foreach (anim_path, each_knot_rel, s2);
                      else
                        gegl_path_foreach (anim_path, each_knot, s2);
                      g_string_append_printf (s2, " } ");
                    }
                    else if (value != defval || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_snprintf (str, sizeof (str), "%u", value);
                        g_string_append_printf (s2, " %s=%s%s", property_name,
                                                str, rel_orig?"rel":"");
                        printed = TRUE;
                      }
                  }
                else if (property_type == G_TYPE_BOOLEAN)
                  {
                    gboolean value;
                    gboolean defval = g_value_get_boolean (default_value);
                    gegl_node_get (iter, property_name, &value, NULL);
                    if (value != defval || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        if (value)
                          g_string_append_printf (s2, " %s=true",
                                                  property_name);
                        else
                          g_string_append_printf (s2, " %s=false",
                                                  property_name);
                        printed = TRUE;
                      }
                  }
                else if (property_type == G_TYPE_STRING)
                  {
                    gchar *value;
                    const gchar *defval = g_value_get_string (default_value);
                    gegl_node_get (iter, property_name, &value, NULL);
                    if (!g_str_equal (defval, value) || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_string_append_printf (s2, " %s='%s'", property_name,
                                                value);
                        printed = TRUE;
                      }
                    g_free (value);
                  }
                else if (g_type_is_a (property_type, G_TYPE_ENUM))
                  {
                    GEnumClass *eclass = g_type_class_peek (property_type);
                    gint defval = g_value_get_enum (default_value);
                    gint value;

                    gegl_node_get (iter, property_name, &value, NULL);
                    if (value != defval || (!trim_defaults))
                      {
                        GEnumValue *evalue = g_enum_get_value (eclass, value);
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_string_append_printf (s2, " %s=%s", property_name,
                                                evalue->value_nick);
                        printed = TRUE;
                      }
                  }
                else if (property_type == GEGL_TYPE_COLOR)
                  {
                    GeglColor *color;
                    GeglColor *defcolor = g_value_get_object (default_value);
                    gchar     *value;
                    gchar     *defvalue = NULL;
                    gegl_node_get (iter, property_name, &color, NULL);
                    g_object_get (color, "string", &value, NULL);
                    if (defcolor)
                      {
                        g_object_get (defcolor, "string", &defvalue, NULL);
                      }
                    g_object_unref (color);
                    if ((defvalue &&
                         !g_str_equal (defvalue, value)) || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_string_append_printf (s2, " %s='%s'", property_name,
                                                value);
                        printed = TRUE;
                      }
                    g_free (value);
                  }
                else if (property_type == GEGL_TYPE_PATH)
                  {
                    gchar *svg_path;
                    GeglPath *path;
                    gegl_node_get (iter, property_name, &path, NULL);
                    svg_path = gegl_path_to_string (path);
                    g_object_unref (path);
                    if (flags & GEGL_SERIALIZE_INDENT)
                      g_string_append_printf (s2, "  ");
                    g_string_append_printf (s2, " %s='%s'", property_name,
                                            svg_path);
                    printed = TRUE;
                    g_free (svg_path);
                  }
                else if (property_type == G_TYPE_POINTER &&
                         GEGL_IS_PARAM_SPEC_FORMAT (properties[i]))
                  {
                    const Babl *format;
                    const gchar *value = "";
                    gegl_node_get (iter, property_name, &format, NULL);
                    if (format)
                      value = babl_get_name (format);
                    if (value[0] || (!trim_defaults))
                      {
                        if (flags & GEGL_SERIALIZE_INDENT)
                          g_string_append_printf (s2, "  ");
                        g_string_append_printf (s2, " %s='%s'", property_name,
                                            value);
                        printed = TRUE;
                      }
                  }
                else if (property_type == GEGL_TYPE_AUDIO_FRAGMENT)
                  {
                    /* ignore */
                  }
                else
                  {
                    g_warning (
                      "%s: serialization of %s properties not implemented",
                      property_name, g_type_name (property_type));
                  }
              }

              if (printed && (flags & GEGL_SERIALIZE_INDENT))
                  g_string_append_printf (s2, "\n");

              {
                GeglNode *aux = gegl_node_get_producer (iter, "aux", NULL);
                if (aux)
                  {
                    char *str = gegl_serialize2 (NULL, aux, basepath, ht,
                                                 flags);
                    g_string_append_printf (s2, " aux=[ %s ]%s", str,
                            (flags& GEGL_SERIALIZE_INDENT)?"\n":" ");
                    g_free (str);
                  }
              }
          }

          g_string_prepend (str, s2->str);
          g_string_free (s2, TRUE);
          iter = gegl_node_get_producer (iter, "input", NULL);
        }
    }
  return g_string_free (str, FALSE);
}

gchar *
gegl_serialize (GeglNode         *start,
                GeglNode         *end,
                const char       *basepath,
                GeglSerializeFlag flags)
{
  gchar *ret;
  gchar *ret2;

  GHashTable *ht = g_hash_table_new (g_direct_hash, g_direct_equal);

  ret = gegl_serialize2 (start, end, basepath, ht, flags);
  g_hash_table_destroy (ht);
  ret2 = ret;
  while (ret2[0] == ' ')
    ret2++;
  ret2 = g_strdup (ret2);
  g_free (ret);
  return ret2;
}

GeglNode *
gegl_node_new_from_serialized (const gchar *chaindata,
                               const gchar *path_root)
{
  GeglNode *ret;
  GeglNode *foo;
  gdouble time = 0.0;

  ret = gegl_node_new ();
  gegl_node_set (ret, "operation", "gegl:nop", NULL);
  foo = gegl_node_new ();
  gegl_node_set (foo, "operation", "gegl:nop", NULL);

  gegl_node_link_many (foo, ret, NULL);
  gegl_create_chain (chaindata, foo, ret, time, 1024, path_root, NULL);

  return ret;
}

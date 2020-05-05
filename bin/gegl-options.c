/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "gegl-options.h"
#include <gegl.h>

static GeglOptions *opts_new (void)
{
  GeglOptions *o = g_malloc0 (sizeof (GeglOptions));

  o->mode     = GEGL_RUN_MODE_DISPLAY;
  o->xml      = NULL;
  o->output   = NULL;
  o->files    = NULL;
  o->file     = NULL;
  o->rest     = NULL;
  o->scale    = 1.0;
  return o;
}

static G_GNUC_NORETURN void
usage (char *application_name)
{
    fprintf (stderr,
_("usage: %s [options] <file | -- [op [op] ..]>\n"
"\n"
"  Options:\n"
"     -h, --help      this help information\n"
"\n"
"     --list-all      list all known operations\n"
"\n"
"     --exists        return 0 if the operation(s) exist\n"
"\n"
"     --info          output information about the operation:\n"
"                     name, description, properties details.\n"
"\n"
"     -i, --file      read xml from named file\n"
"\n"
"     -x, --xml       use xml provided in next argument\n"
"\n"
"     -o, --output    output generated image to named file, type based\n"
"                     on extension.\n"
"\n"
"     -p              increment frame counters of various elements when\n"
"                     processing is done.\n"
"\n"
"     -s scale, --scale scale  scale output dimensions by this factor.\n"
"\n"
"     -X              output the XML that was read in\n"
"\n"
"     -v, --verbose   print diagnostics while running\n"
"\n"
"All parameters following -- are considered ops to be chained together\n"
"into a small composition instead of using an xml file, this allows for\n"
"easy testing of filters. After chaining a new op in properties can be set\n"
"with property=value pairs as subsequent arguments.\n")
, application_name);
    exit (0);
}

#define match(string) (!strcmp (*curr, (string)))
#define assert_argument() do {\
    if (!curr[1] || curr[1][0]=='-') {\
        fprintf (stderr, _("ERROR: '%s' option expected argument\n"), *curr);\
        exit(-1);\
    }\
}while(0)

#define get_float(var) do{\
    assert_argument();\
    curr++;\
    (var)=atof(*curr);\
}while(0)

#define get_int(var) do{\
    assert_argument();\
    curr++;\
    (var)=atoi(*curr);\
}while(0)

#define get_string(var) do{\
    assert_argument();\
    curr++;\
    (var)=*curr;\
}while(0)

#define get_string_forced(var) do{\
    curr++;\
    (var)=*curr;\
}while(0)

static GeglOptions *
parse_args (gint    argc,
            gchar **argv);

static void
print_opts (GeglOptions *o)
{
  char *mode_str;
  switch (o->mode)
    {
      case GEGL_RUN_MODE_DISPLAY:
        mode_str = _("Display on screen"); break;
      case GEGL_RUN_MODE_XML:
        mode_str = _("Print XML"); break;
      case GEGL_RUN_MODE_OUTPUT:
        mode_str = _("Output in a file"); break;
      case GEGL_RUN_MODE_HELP:
        mode_str = _("Display help information"); break;
      default:
        g_warning (_("Unknown GeglOption mode: %d"), o->mode);
        mode_str = _("unknown mode");
        break;
    }

    fprintf (stderr,
_("Parsed commandline:\n"
"\tmode:   %s\n"
"\tfile:   %s\n"
"\txml:    %s\n"
"\toutput: %s\n"
"\trest:   %s\n"
"\t\n"),
    mode_str,
    o->file==NULL?"(null)":o->file,
    o->xml==NULL?"(null)":o->xml,
    o->output==NULL?"(null)":o->output,
    o->rest==NULL?"":"yes"
);
    {
      GList *files = o->files;
      while (files)
        {
          fprintf (stderr, "\t%s\n", (gchar*)files->data);
          files = g_list_next (files);
        }
    }
}

/**
 * print_key_value:
 * @key:
 * @value:
 * @padding:
 *
 * Print on standard output @key left-adjusted according to @padding,
 * followed by its @value. The value will be pretty-printed by keeping
 * the left-padding after every line feed.
 */
static void
print_key_value (const gchar *key,
                 const gchar *value,
                 gint         padding)
{
  const gint  max_value_length = 80;
  gchar      *val              = g_strdup (value);
  gint        current_val_len;
  gchar      *token;

  token = strtok (val, " \t\n\r");
  if (token)
  {
    current_val_len = strlen (token);
    fprintf (stdout, "%-*s %s", padding, key, token);

    while ((token = strtok (NULL, " \t\n\r")))
    {
      if (current_val_len + strlen (token) > max_value_length)
        {
          fprintf (stdout, "\n%-*s %s", padding, " ", token);
          current_val_len = strlen (token);
        }
      else
        {
          fprintf (stdout, " %s", token);
          current_val_len += strlen (token) + 1;
        }
    }

  fprintf (stdout, "\n");
  }
  g_free (val);
}

GeglOptions *
gegl_options_parse (gint    argc,
                    gchar **argv)
{
    GeglOptions *o;

    o = parse_args (argc, argv);
    if (o->verbose)
        print_opts (o);
    return o;
}

gboolean
gegl_options_next_file (GeglOptions *o)
{
  GList *current = g_list_find (o->files, o->file);
  current = g_list_next (current);
  if (current)
    {
      g_warning ("%s", o->file);
      o->file = current->data;
      g_warning ("%s", o->file);
      return TRUE;
    }
  return FALSE;
}

gboolean
gegl_options_previous_file (GeglOptions *o)
{
  GList *current = g_list_find (o->files, o->file);
  current = g_list_previous (current);
  if (current)
    {
      o->file = current->data;
      return TRUE;
    }
  return FALSE;
}

#ifdef HAVE_MRG
int thumbgen_main (int argc, char **argv);
#else
static int thumbgen_main (int argc, char **argv)
{
 return -222;
}
#endif

static GeglOptions *
parse_args (int    argc,
            char **argv)
{
    GeglOptions *o;
    char **curr;

    o = opts_new ();
    if (argc==1) {
#ifdef HAVE_MRG
      o->mode = GEGL_RUN_MODE_DISPLAY;
#else
      usage (argv[0]);
#endif
    }

    curr = argv+1;

    while (*curr && !o->rest) {
        if (match ("-h")    ||
            match ("--help")) {
            o->mode = GEGL_RUN_MODE_HELP;
            usage (argv[0]);
        }
        else if (match ("--thumbgen")) {
            thumbgen_main (argc, argv);
            exit (0);
        } else if (match ("--list-all")) {
            guint   n_operations;
            gint    i;
            gchar **operations;

            /* initializing opencl for no use in this meta-data only query */
            g_object_set (gegl_config (), "use-opencl", FALSE, NULL);
            gegl_init (NULL, NULL);

            operations  = gegl_list_operations (&n_operations);

            for (i = 0; i < n_operations; i++)
              {
                fprintf (stdout, "%s\n", operations[i]);
              }
            g_free (operations);

            exit (0);
        }

        else if (match ("--exists")) {
            gchar   *op_name;
            /* initializing opencl for no use in this meta-data only query */
            g_object_set (gegl_config (), "use-opencl", FALSE, NULL);
            gegl_init (NULL, NULL);

            /* The option requires at least one argument. */
            get_string (op_name);
            while (op_name)
              {
                if (!gegl_has_operation (op_name))
                  exit (1);
                get_string_forced (op_name);
              }

            exit (0);
        }

        /* --properties is the former option name, kept as alias. */
        else if (match ("--info") || match ("--properties")) {
            gchar  *op_name;
            get_string (op_name);

            /* initializing opencl for no use in this meta-data only query */
            g_object_set (gegl_config (), "use-opencl", FALSE, NULL);
            gegl_init (NULL, NULL);

            if (gegl_has_operation (op_name))
              {
                gint         i;
                guint        n_properties;
                GParamSpec **properties;
                gchar      **keys;
                guint        n_keys;

                keys = gegl_operation_list_keys (op_name, &n_keys);
                for (i = 0; i < n_keys; i++)
                  {
                    print_key_value (keys[i],
                                     gegl_operation_get_key (op_name, keys[i]),
                                     20);
                  }
                g_free (keys);

                fprintf (stdout, "\n%s\n", _("Properties:"));

                properties = gegl_operation_list_properties (op_name, &n_properties);
                for (i = 0; i < n_properties; i++)
                  {
                    const gchar  *property_name;
                    const GValue *property_default;
                    gchar        *property_blurb;
                    gchar        *default_string = NULL;

                    property_name = g_param_spec_get_name (properties[i]);
                    property_default = g_param_spec_get_default_value (properties[i]);
                    switch (G_VALUE_TYPE (property_default))
                      {
                      case G_TYPE_DOUBLE:
                        default_string = g_strdup_printf (" (default: %f)",
                                                          g_value_get_double (property_default));
                        break;
                      case G_TYPE_STRING:
                        default_string = g_strdup_printf (" (default: \"%s\")",
                                                          g_value_get_string (property_default));
                        break;
                      case G_TYPE_INT:
                        default_string = g_strdup_printf (" (default: %d)",
                                                          g_value_get_int (property_default));
                        break;
                      case G_TYPE_BOOLEAN:
                        default_string = g_strdup_printf (" (default: %s)",
                                                          g_value_get_boolean (property_default)?
                                                          "TRUE" : "FALSE");
                        break;
                      default:
                        default_string = NULL;
                        break;
                      }
                    property_blurb = g_strconcat ("[",
                                                  g_type_name (properties[i]->value_type),
                                                  "] ",
                                                  g_param_spec_get_blurb (properties[i]),
                                                  default_string,
                                                  NULL);

                    print_key_value (property_name, property_blurb, 20);

                    g_free (property_blurb);
                    g_free (default_string);
                  }

                g_free (properties);
                exit (0);
              }

            exit (1);
        }

        else if (match ("--verbose") ||
                 match ("-v")) {
            o->verbose=1;
        }

        else if (match ("--g-fatal-warnings") ||
                 match ("-v")) {
            o->fatal_warnings=1;
        }

        else if (match ("--serialize")){
            o->serialize=TRUE;
        }

        else if (match ("-S")){
            o->serialize=TRUE;
        }

        else if (match ("-p")){
            o->play=TRUE;
        }

        else if (match ("--file") ||
                 match ("-i")) {
            const gchar *file_path;
            get_string (file_path);
            o->files = g_list_append (o->files, g_strdup (file_path));
        }

        else if (match ("--xml") ||
                 match ("-x")) {
            get_string (o->xml);
        }

        else if (match ("--output") ||
                 match ("-o")) {
            get_string_forced (o->output);
            o->mode = GEGL_RUN_MODE_OUTPUT;
        }

        else if (match ("--scale") ||
                 match ("-s")) {
            get_float (o->scale);
        }

        else if (match ("-X")) {
            o->mode = GEGL_RUN_MODE_XML;
        }

        else if (match ("--")) {
            o->rest = curr + 1;
            break;
        }

        else if (*curr[0]=='-') {
            fprintf (stderr, _("\n\nunknown argument '%s' giving you help instead\n\n\n"), *curr);
            usage (argv[0]);
        }

        else
          {
            o->files = g_list_append (o->files, g_strdup (*curr));
          }
        curr++;
    }

    if (o->files)
      o->file = o->files->data;
    return o;
}
#undef match
#undef assert_argument

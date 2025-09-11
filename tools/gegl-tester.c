/* This file is an image processing operation for GEGL
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
 * Copyright 2012 Ville Sokk <ville.sokk@gmail.com>
 *           2017 Øyvind Kolås <pippin@gimp.org>
 */

#include <locale.h>

#include <gegl.h>
#include <gegl-plugin.h>
#include <glib.h>

static GRegex   *regex             = NULL;
static GRegex   *exc_regex         = NULL;
static gchar    *data_dir          = NULL;
static gchar    *output_dir        = NULL;
static gchar    *pattern           = "";
static gchar    *exclusion_pattern = "a^"; /* doesn't match anything by default */
static gboolean *output_all        = FALSE;
static gint      failed            = 0;
static GString  *failed_ops        = NULL;
static gint      test_num          = 0;

static const GOptionEntry options[] =
{
  {"data-directory", 'd', 0, G_OPTION_ARG_FILENAME, &data_dir,
   "Root directory of files used in the composition", NULL},

  {"output-directory", 'o', 0, G_OPTION_ARG_FILENAME, &output_dir,
   "Directory where composition output and diff files are saved", NULL},

  {"pattern", 'p', 0, G_OPTION_ARG_STRING, &pattern,
   "Regular expression used to match names of operations to be tested", NULL},

  {"exclusion-pattern", 'e', 0, G_OPTION_ARG_STRING, &exclusion_pattern,
   "Regular expression used to match names of operations not to be tested", NULL},

  {"all", 'a', 0, G_OPTION_ARG_NONE, &output_all,
   "Create output for all operations using a standard composition "
   "if no composition is specified", NULL},

  { NULL }
};

/* convert operation name to output path */
static gchar*
operation_to_path (const gchar *op_name)
{
  gchar *cleaned = g_strdup (op_name);
  gchar *filename, *output_path;

  g_strdelimit (cleaned, ":", '-');
  filename = g_strconcat (cleaned, ".png", NULL);
  output_path = g_build_path (G_DIR_SEPARATOR_S, output_dir, filename, NULL);

  g_free (cleaned);
  g_free (filename);

  return output_path;
}

static void
standard_output (const gchar *op_name)
{
  GeglNode *composition, *input, *aux, *operation, *crop, *output, *translate;
  GeglNode *background,  *over;
  gchar    *input_path  = g_build_path (G_DIR_SEPARATOR_S, data_dir,
                                        "standard-input.png", NULL);
  gchar    *aux_path    = g_build_path (G_DIR_SEPARATOR_S, data_dir,
                                        "standard-aux.png", NULL);
  gchar    *output_path = operation_to_path (op_name);

  composition = gegl_node_new ();
  operation = gegl_node_create_child (composition, op_name);

  if (gegl_node_has_pad (operation, "output"))
    {
      input = gegl_node_new_child (composition,
                                   "operation", "gegl:load",
                                   "path", input_path,
                                   NULL);
      translate  = gegl_node_new_child (composition,
                                 "operation", "gegl:translate",
                                 "x", 0.0,
                                 "y", 80.0,
                                 NULL);
      aux = gegl_node_new_child (composition,
                                 "operation", "gegl:load",
                                 "path", aux_path,
                                 NULL);
      crop = gegl_node_new_child (composition,
                                  "operation", "gegl:crop",
                                  "width", 200.0,
                                  "height", 200.0,
                                  NULL);
      output = gegl_node_new_child (composition,
                                    "operation", "gegl:png-save",
                                    "compression", 9,
                                    "path", output_path,
                                    NULL);
      background = gegl_node_new_child (composition,
                                        "operation", "gegl:checkerboard",
                                        "color1", gegl_color_new ("rgb(0.75,0.75,0.75)"),
                                        "color2", gegl_color_new ("rgb(0.25,0.25,0.25)"),
                                        NULL);
      over = gegl_node_new_child (composition, "operation", "gegl:over", NULL);


      if (gegl_node_has_pad (operation, "input"))
        gegl_node_link (input, operation);

      if (gegl_node_has_pad (operation, "aux"))
        {
          gegl_node_connect (aux, "output", translate, "input");
          gegl_node_connect (translate, "output", operation, "aux");
        }

      gegl_node_connect (background, "output", over, "input");
      gegl_node_connect (operation,  "output", over, "aux");
      gegl_node_connect (over,       "output", crop, "input");
      gegl_node_connect (crop,       "output", output, "input");

      gegl_node_process (output);
    }

  g_free (input_path);
  g_free (aux_path);
  g_free (output_path);
  g_object_unref (composition);
}

static gchar *
compute_hash_for_path (const gchar *path)
{
  GeglNode      *gegl = NULL;
  GeglNode      *img  = NULL;
  gchar         *hash = NULL;
  guchar        *buf  = NULL;
  GeglRectangle  comp_bounds;

  gegl = gegl_node_new ();
  img  = gegl_node_new_child (gegl,
                       "operation", "gegl:load",
                       "path", path,
                       NULL);

  comp_bounds = gegl_node_get_bounding_box (img);
  buf = g_malloc0 (comp_bounds.width * comp_bounds.height * 4);

  gegl_node_blit (img, 1.0, &comp_bounds, babl_format("R'G'B'A u8"), buf, GEGL_AUTO_ROWSTRIDE, GEGL_BLIT_DEFAULT);
  hash = g_compute_checksum_for_data (G_CHECKSUM_MD5, buf, comp_bounds.width * comp_bounds.height * 4);

  g_free (buf);
  g_object_unref (gegl);

  return hash;
}

static gboolean
test_with_hashes (const gchar        *name,
                  const gchar        *output_path,
                  GeglOperationClass *operation_class,
                  gboolean            supported_op)
{
  const gchar *hash    = gegl_operation_class_get_key (operation_class, "reference-hash");
  gboolean     success = TRUE;

  if (hash != NULL)
    {
      const gchar *hashB = gegl_operation_class_get_key (operation_class, "reference-hashB");
      const gchar *hashC = gegl_operation_class_get_key (operation_class, "reference-hashC");
      gchar *gothash     = compute_hash_for_path (output_path);

      if (g_str_equal (hash, gothash))
        g_printf ("ok     %3i - %s\n", test_num, name);
      else if (hashB && g_str_equal (hashB, gothash))
        g_printf ("ok     %3i - %s (ref b)\n", test_num, name);
      else if (hashC && g_str_equal (hashC, gothash))
        g_printf ("ok     %3i - %s (ref c)\n", test_num, name);
      else if (g_str_equal (hash, "unstable"))
        g_printf ("not ok %3i - %s (unstable) # TODO reference is not reproducible\n", test_num, name);
      else
        {
          success = FALSE;

          g_printf ("not ok %3i - %s %s != %s\n", test_num, name, gothash, hash);
          g_string_append_printf (failed_ops, "#  %s %s != %s\n", name, gothash, hash);
        }

      g_free (gothash);
    }
  else if (supported_op)
    {
      gchar *gothash = compute_hash_for_path (output_path);

      if (g_str_equal (gothash, "9bbe341d798da4f7b181c903e6f442fd") ||
          g_str_equal (gothash, "ffb9e86edb25bc92e8d4e68f59bbb04b"))
        {
          g_printf ("not ok %3i - %s (noop) # TODO hash is noop\n", test_num, name);
        }
      else
        {
          g_printf ("not ok %3i - %s (no ref) # TODO hash = %s\n", test_num, name, gothash);
        }

      g_free (gothash);
    }

  return success;
}

static void
process_operations (GType type)
{
  GType    *operations = NULL;
  guint     count      = 0;

  operations = g_type_children (type, &count);

  if (!operations)
    {
      g_free (operations);
      return;
    }

  for (gint i = 0; i < count; i++)
    {
      GeglOperationClass *operation_class = NULL;
      const gchar        *name            = NULL;
      gboolean            matches         = FALSE;

      operation_class = g_type_class_ref (operations[i]);
      name            = gegl_operation_class_get_key (operation_class, "name");

      if (name == NULL)
        {
          process_operations (operations[i]);
          continue;
        }

      matches = g_regex_match(regex, name, 0, NULL) &&
                !g_regex_match(exc_regex, name, 0, NULL);

      if (matches)
        {
          const gchar *chain        = NULL;
          const gchar *xml          = NULL;
          gchar       *output_path  = operation_to_path (name);
          gboolean     supported_op = FALSE;

          supported_op = !(g_type_is_a (operations[i], GEGL_TYPE_OPERATION_SINK) ||
                           g_type_is_a (operations[i], GEGL_TYPE_OPERATION_TEMPORAL));

          xml   = gegl_operation_class_get_key (operation_class, "reference-composition");
          chain = gegl_operation_class_get_key (operation_class, "reference-chain");
          if (chain || xml)
            {
              GeglNode *composition = NULL;

              test_num++;

              if (xml)
                composition = gegl_node_new_from_xml (xml, data_dir);
              else
                composition = gegl_node_new_from_serialized (chain, data_dir);

              if (!composition)
                {
                  g_printf ("not ok %3i - Composition graph is flawed\n", test_num);
                }
              else if (output_all)
                {
                  gchar    *output_path = NULL;
                  GeglNode *output      = NULL;

                  output_path = operation_to_path (name);
                  output      = gegl_node_new_child (composition,
                                                     "operation", "gegl:png-save",
                                                     "compression", 9,
                                                     "path", output_path,
                                                     NULL);
                  gegl_node_link (composition, output);

                  gegl_node_process (output);

                  g_object_unref (composition);
                }
            }
          else if (output_all && supported_op)
            {
              /* If we are running with --all and the operation doesn't have a
                 composition, use standard composition and images, don't test. */
              test_num++;
              standard_output (name);
            }

          if (!test_with_hashes (name, output_path, operation_class, supported_op))
            failed++;

          g_free (output_path);
        }
      process_operations (operations[i]);
    }

  g_free (operations);
}

gint
main (gint    argc,
      gchar **argv)
{
  GError         *error   = NULL;
  GOptionContext *context = NULL;

  setlocale (LC_ALL, "");

  context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gegl_get_option_group ());

  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printf ("%s\n", error->message);
      g_error_free (error);
      return -1;
    }
  else if (output_all && !(data_dir && output_dir))
    {
      g_printf ("Bail out! Data and output directories must be specified\n");
      return -1;
    }
  else if (!(output_all || (data_dir && output_dir)))
    {
      g_printf ("Bail out! Data and output directories must be specified\n");
      return -1;
    }

  failed_ops = g_string_new ("");
  regex      = g_regex_new (pattern, 0, 0, NULL);
  exc_regex  = g_regex_new (exclusion_pattern, 0, 0, NULL);

  process_operations (GEGL_TYPE_OPERATION);

  g_regex_unref (regex);
  g_regex_unref (exc_regex);

  gegl_exit ();

  // TAP plan
  g_printf("1..%i\n", test_num);

  if (failed != 0)
    {
      g_print ("# Maybe see bug https://bugzilla.gnome.org/show_bug.cgi?id=780226\n");
      g_print ("# %i operations producing unexpected hashes:\n%s\n", failed, failed_ops->str);
      // return -1;
    }
  g_string_free (failed_ops, TRUE);

  return 0;
}

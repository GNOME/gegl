/*
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
 * Copyright (C) 2016 OEyvind Kolaas
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include "gegl.h"
#include "gegl-plugin.h"

#define SUCCESS  0
#define FAILURE -1

typedef struct TestCase {
    gchar *argv_chain; gchar *expected_serialization; gchar *expected_error;
} TestCase;

TestCase tests[] = {
    {"invert",
     "gegl:invert-linear",
     ""},

    {"threshold value=0.1",
     "gegl:threshold value=0.10000000000000001",
     ""},


    {"threshold value={ 0=0.1 2=0.33 }",
     "gegl:threshold value={  0=0.10000000149011612  2=0.33000001311302185  } ",
     ""},

    {"invert a=b",
     "gegl:invert-linear",
     "gegl:invert has no a property."},

    {"invert a=c",
     "gegl:invert-linear",
     "gegl:invert has no a property."},

    {"gaussian-blur",
     "gegl:gaussian-blur",
     ""},

    /*
       XXX: commented out until we have internal fonts for reproducible metrix,
       see https://bugzilla.gnome.org/show_bug.cgi?id=772992
    {"over aux=[ text string='foo bar' ]",
     "svg:src-over aux=[  gegl:text string='foo bar' width=33 height=7 ]\n",
     ""},

    {"over aux=[text string='foo bar' ]",
     "svg:src-over aux=[  gegl:text string='foo bar' width=33 height=7 ]\n",
     ""},
    {"over aux=[text string='foo bar']",
     "svg:src-over aux=[  gegl:text string='foo bar' width=33 height=7 ]\n",
     ""},
    {"over aux=[ text string={ 0='foo bar' } ]",
     "svg:src-over aux=[  gegl:text string='foo bar' width=33 height=7 ]\n",
     ""},
     */

    {"over aux= [ ",
     "svg:src-over",
     "gegl:over has no aux property, properties: 'srgb', "},

    /* the following cause undesired warnings on console and does not look nice */
    {"over aux=[ string='foo bar' ]",
     "svg:src-over",
     "(null) does not have a pad called output"},

    /* the following should have better error messages */
    {"over aux=[ ",
     "svg:src-over",
     "(null) does not have a pad called output"},

    {"over aux=[ ]",
     "svg:src-over",
     "(null) does not have a pad called output"},

    {"exposure foo=2",
     "gegl:exposure",
     "gegl:exposure has no foo property, properties: 'black-level', 'exposure', "},

    {"over aux=[ load path=/ ]",
     "svg:src-over aux=[  gegl:load path='/' ] ",
     ""},

    {"inver",
     "",
     "No such op 'gegl:inver' suggestions: gegl:invert-gamma gegl:invert-linear"},

    {"over aux=[ load path=/abc ]",
     "svg:src-over aux=[  gegl:load path='/abc' ] ",
     ""},

    {"id=foo over aux=[ ref=foo invert ]",
     "id=foo\n svg:src-over aux=[  ref=foo\n gegl:invert-linear ] ",
     ""},

    {"id=bar id=foo over aux=[ ref=foo invert ]",
     "id=foo\n svg:src-over aux=[  ref=foo\n gegl:invert-linear ] ",
     ""},


    {NULL, NULL, NULL}
};

static int
test_serialize (void)
{
  gint result = SUCCESS;
  GeglNode *node = gegl_node_new ();
  GeglNode *start = gegl_node_new_child (node, "operation", "gegl:nop", NULL);
  GeglNode *end = gegl_node_new_child (node, "operation", "gegl:nop", NULL);
  int i;

  gegl_node_link_many (start, end, NULL);

  for (i = 0; tests[i].argv_chain; i++)
  {
    GError *error = NULL;
    gint res = SUCCESS;
    gchar *serialization = NULL;
    gegl_create_chain (tests[i].argv_chain, start, end,
                    0.0, 500, NULL, &error);
    serialization = gegl_serialize (start, gegl_node_get_producer (end, "input", NULL), "/", GEGL_SERIALIZE_TRIM_DEFAULTS);
    if (strcmp (serialization, tests[i].expected_serialization))
    {
      printf ("%s\nexpected:\n%s\nbut got:\n%s\n", 
              tests[i].argv_chain,
              tests[i].expected_serialization,
              serialization);
      res = FAILURE;
    }
    g_free (serialization);
    if (error)
    {
      if (g_ascii_strncasecmp (error->message, tests[i].expected_error,
                               strlen (tests[i].expected_serialization)+1))
      {
       printf ("%s\nexpected error:\n%s\nbut got error:\n%s\n", 
                      tests[i].argv_chain,
                      tests[i].expected_error,
                      error->message);
       res = FAILURE;
      }
    }
    else if (tests[i].expected_error && tests[i].expected_error[0])
    {
       printf ("%s\ngot success instead of expected error:%s\n",
                      tests[i].argv_chain,
                      tests[i].expected_error);
       res = FAILURE;
    }

    if (res == FAILURE)
    {
      result = FAILURE;
      printf ("FAILED: %s\n", tests[i].argv_chain);
    }
    else
    {
      printf ("pass: %s\n", tests[i].argv_chain);
    }
  }
  g_object_unref (node);
  
  return result;
}


int main(int argc, char *argv[])
{
  gint result = SUCCESS;

  gegl_init (&argc, &argv);

  if (result == SUCCESS)
    result = test_serialize ();

  gegl_exit ();

  if (result == SUCCESS)
    fprintf (stderr, "\n:)\n");
  else
    fprintf (stderr, "\n:(\n");

  return result;
}

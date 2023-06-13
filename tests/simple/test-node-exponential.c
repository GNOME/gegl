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
 * Copyright (C) 2017 Ell
 */

/* make sure that graphs of the form
 *
 *     ___   ___   ___
 *    /   v /   v /   v 
 *   A     B     C     D ...
 *    \___^ \___^ \___^
 *
 * don't lead to an exponential explosion in run time, during
 * construction and invalidation.
 *
 * see commits ed8af9caeed516d4f39d3ff2c7337d6959425293 and
 * 3d5f4fd38f1b60d5774259062fb64abacf10a7b2.
 */

#include "config.h"

#include "gegl.h"
#include "graph/gegl-node-private.h"

#define SUCCESS  0
#define FAILURE -1

#define TIMEOUT (60 * G_TIME_SPAN_SECOND)

static GMutex            mutex;
static GCond             cond;
static volatile gboolean finished = FALSE;

static gpointer
test (gpointer data)
{
  GeglNode *node;
  GeglNode *input;
  GeglNode *last;
  gint      i;

  node = gegl_node_new ();

  input = gegl_node_get_input_proxy (node, "input");
  last  = input;

  for (i = 0; i < 64; i++)
    {
      GeglNode *over;

      over = gegl_node_new_child (node,
                                  "operation", "gegl:over",
                                  NULL);

      gegl_node_connect (last, "output", over, "input");
      gegl_node_connect (last, "output", over, "aux");

      last = over;
    }

  gegl_node_invalidated (input, NULL, FALSE);

  g_object_unref (node);

  g_mutex_lock (&mutex);

  finished = TRUE;
  g_cond_signal (&cond);

  g_mutex_unlock (&mutex);

  return NULL;
}

int main(int argc, char **argv)
{
  GThread *thread;
  gint64   end_time;

  gegl_init (&argc, &argv);

  thread = g_thread_new (NULL, test, NULL);

  end_time = g_get_monotonic_time () + TIMEOUT;

  g_mutex_lock (&mutex);

  while (! finished)
    {
      if (! g_cond_wait_until (&cond, &mutex, end_time))
        break;
    }

  g_mutex_unlock (&mutex);

  if (finished)
    {
      g_thread_join (thread);

      gegl_exit ();

      return SUCCESS;
    }
  else
    {
      g_print ("timeout expired. failing.\n");

      return FAILURE;
    }
}

/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * mazegen code from rec.games.programmer's maze-faq:
 * * maz.c - generate a maze
 * *
 * * algorithm posted to rec.games.programmer by jallen@ic.sunysb.edu
 * * program cleaned and reorganized by mzraly@ldbvax.dnet.lotus.com
 * *
 * * don't make people pay for this, or I'll jump up and down and
 * * yell and scream and embarrass you in front of your friends...
 *
 * Contains code originaly from GIMP 'maze' Plugin by
 * Kevin Turner <acapnotic@users.sourceforge.net>
 *
 * Maze ported to GEGL:
 * Copyright 2015 Akash Hiremath (akash akya) <akashh246@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_maze_algorithm)
  enum_value (GEGL_MAZE_ALGORITHM_DEPTH_FIRST, "depth-first", N_("Depth first"))
  enum_value (GEGL_MAZE_ALGORITHM_PRIM,        "prim",        N_("Prim's algorithm"))
enum_end (GeglMazeAlgorithm)

property_int    (x, _("Width"), 16)
    description (_("Horizontal width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "x")

property_int    (y, _("Height"), 16)
    description (_("Vertical width of cells pixels"))
    value_range (1, G_MAXINT)
    ui_range    (1, 256)
    ui_gamma    (1.5)
    ui_meta     ("unit", "pixel-distance")
    ui_meta     ("axis", "y")

property_enum (algorithm_type, _("Algorithm type"),
               GeglMazeAlgorithm, gegl_maze_algorithm,
               GEGL_MAZE_ALGORITHM_DEPTH_FIRST)
  description (_("Maze algorithm type"))

property_boolean (tileable, _("Tileable"), FALSE)

property_seed (seed, _("Random seed"), rand)

property_color  (fg_color, _("Foreground Color"), "black")
    description (_("The foreground color"))
    ui_meta     ("role", "color-primary")

property_color  (bg_color, _("Background Color"), "white")
    description (_("The background color"))
    ui_meta     ("role", "color-secondary")

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     maze
#define GEGL_OP_C_SOURCE maze.c

#include "gegl-op.h"
#include <stdio.h>

#define CELL_UP(POS) ((POS) < (x*2) ? -1 : (POS) - x - x)
#define CELL_DOWN(POS) ((POS) >= x*(y-2) ? -1 : (POS) + x + x)
#define CELL_LEFT(POS) (((POS) % x) <= 1 ? -1 : (POS) - 2)
#define CELL_RIGHT(POS) (((POS) % x) >= (x - 2) ? -1 : (POS) + 2)

#define WALL_UP(POS) ((POS) - x)
#define WALL_DOWN(POS) ((POS) + x)
#define WALL_LEFT(POS) ((POS) - 1)
#define WALL_RIGHT(POS) ((POS) + 1)

#define CELL_UP_TILEABLE(POS) ((POS) < (x*2) ? x*(y-2)+(POS) : (POS) - x - x)
#define CELL_DOWN_TILEABLE(POS) ((POS) >= x*(y-2) ? (POS) - x*(y-2) : (POS) + x + x)
#define CELL_LEFT_TILEABLE(POS) (((POS) % x) <= 1 ? (POS) + x - 2 : (POS) - 2)
#define CELL_RIGHT_TILEABLE(POS) (((POS) % x) >= (x - 2) ? (POS) + 2 - x : (POS) + 2)

#define WALL_UP_TILEABLE(POS) ((POS) < x ? x*(y-1)+(POS) : (POS) - x)
#define WALL_DOWN_TILEABLE(POS) ((POS) + x)
#define WALL_LEFT_TILEABLE(POS) (((POS) % x) == 0 ? (POS) + x - 1 : (POS) - 1)
#define WALL_RIGHT_TILEABLE(POS) ((POS) + 1)


enum CellTypes
{
  OUT,
  IN,
  FRONTIER
};

static GRand *gr;
static int    multiple = 57;  /* Must be a prime number */
static int    offset = 1;

static void
depth_first (gint    position,
             guchar *maz,
             gint    w,
             gint    h)
{
  GArray *stack = g_array_new (FALSE, FALSE, sizeof (gint));

  maz[position] = IN;
  g_array_append_val (stack, position);

  while (stack->len)
    {
      gint  directions[4] = {0, };
      gint  n_direction = 0;
      gint  last = stack->len - 1;
      gint  pos  = g_array_index (stack, gint, last);

      /* If there is a wall two rows above us */
      if (! (pos <= (w * 2)) && ! maz[pos - w - w ])
        directions[n_direction++] = -w;

      /* If there is a wall two rows below us */
      if (pos < w * (h - 2) && ! maz[pos + w + w])
        directions[n_direction++] = w;

      /* If there is a wall two columns to the right */
      if (! (pos % w == w - 2) && ! maz[pos + 2])
        directions[n_direction++] = 1;

      /* If there is a wall two colums to the left */
      if (! (pos % w == 1) && ! maz[pos - 2])
        directions[n_direction++] = -1;

      if (n_direction)
        {
          gint j       = directions[g_rand_int_range (gr, 0, n_direction)];
          gint new_pos = pos + 2 * j;
          maz[pos + j] = IN;
          maz[new_pos] = IN;
          g_array_append_val (stack, new_pos);
        }
      else
        {
          g_array_remove_index_fast (stack, last);
        }
    }

  g_array_free (stack, TRUE);
}

static void
depth_first_tileable (gint    position,
                      guchar *maz,
                      gint    x,
                      gint    y)
{
  GArray *stack = g_array_new (FALSE, FALSE, sizeof (gint));

  maz[position] = IN;
  g_array_append_val (stack, position);

  while (stack->len)
    {
      gint  walls[4] = {0, };
      gint  cells[4] = {0, };
      gint  n_direction = 0;
      gint  last = stack->len - 1;
      gint  pos  = g_array_index (stack, gint, last);

      /* If there is a wall two rows above us */
      if (! maz[CELL_UP_TILEABLE(pos)])
        {
          walls[n_direction]   = WALL_UP_TILEABLE (pos);
          cells[n_direction++] = CELL_UP_TILEABLE (pos);
        }

      /* If there is a wall two rows below us */
      if (! maz[CELL_DOWN_TILEABLE(pos)])
        {
          walls[n_direction]   = WALL_DOWN_TILEABLE (pos);
          cells[n_direction++] = CELL_DOWN_TILEABLE (pos);
        }

      /* If there is a wall two columns to the right */
      if (! maz[CELL_RIGHT_TILEABLE(pos)])
        {
          walls[n_direction]   = WALL_RIGHT_TILEABLE (pos);
          cells[n_direction++] = CELL_RIGHT_TILEABLE (pos);
        }

      /* If there is a wall two colums to the left */
      if (! maz[CELL_LEFT_TILEABLE(pos)])
        {
          walls[n_direction]   = WALL_LEFT_TILEABLE (pos);
          cells[n_direction++] = CELL_LEFT_TILEABLE (pos);
        }

      if (n_direction)
        {
          gint n        = g_rand_int_range (gr, 0, n_direction);
          gint new_pos  = cells[n];
          maz[walls[n]] = IN;
          maz[new_pos]  = IN;
          g_array_append_val (stack, new_pos);
        }
      else
        {
          g_array_remove_index_fast (stack, last);
        }
    }

  g_array_free (stack, TRUE);
}

static void
prim (gint    pos,
      guchar *maz,
      guint   x,
      guint   y,
      int     seed)
{
  GSList *front_cells = NULL;
  guint   current;
  gint    up, down, left, right;
  char    d, i;
  guint   c = 0;
  gint    rnd = seed;

  g_rand_set_seed (gr, rnd);

  maz[pos] = IN;

  up    = CELL_UP (pos);
  down  = CELL_DOWN (pos);
  left  = CELL_LEFT (pos);
  right = CELL_RIGHT (pos);

  if (up >= 0)
    {
      maz[up] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
    }

  if (down >= 0)
    {
      maz[down] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
    }

  if (left >= 0)
    {
      maz[left] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
    }

  if (right >= 0)
    {
      maz[right] = FRONTIER;
      front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
    }

  while (g_slist_length (front_cells) > 0)
    {
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_INT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GINT_TO_POINTER (pos));
      maz[pos] = IN;

      up    = CELL_UP (pos);
      down  = CELL_DOWN (pos);
      left  = CELL_LEFT (pos);
      right = CELL_RIGHT (pos);

      d = 0;
      if (up >= 0)
        {
          switch (maz[up])
            {
            case OUT:
              maz[up] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (up));
              break;

            case IN:
              d = 1;
              break;

            default:
              break;
            }
        }

      if (down >= 0)
        {
          switch (maz[down])
            {
            case OUT:
              maz[down] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (down));
              break;

            case IN:
              d = d | 2;
              break;

            default:
              break;
            }
        }

      if (left >= 0)
        {
          switch (maz[left])
            {
            case OUT:
              maz[left] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (left));
              break;

            case IN:
              d = d | 4;
              break;

            default:
              break;
            }
        }

      if (right >= 0)
        {
          switch (maz[right])
            {
            case OUT:
              maz[right] = FRONTIER;
              front_cells = g_slist_prepend (front_cells,
                                             GINT_TO_POINTER (right));
              break;

            case IN:
              d = d | 8;
              break;

            default:
              break;
            }
        }

      if (! d)
        {
          g_warning ("maze: prim: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     seed, x, y, multiple, offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd * multiple + offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, seed, x, y, multiple, offset);
        }
    }

  g_slist_free (front_cells);
}

static void
prim_tileable (guchar *maz,
               guint   x,
               guint   y,
               gint    seed)
{
  GSList *front_cells = NULL;
  guint   current, pos;
  guint   up, down, left, right;
  char    d, i;
  guint   c = 0;
  gint    rnd = seed;

  g_rand_set_seed (gr, rnd);

  pos = x * 2 * g_rand_int_range (gr, 0, y / 2) +
            2 * g_rand_int_range (gr, 0, x / 2);

  maz[pos] = IN;

  up    = CELL_UP_TILEABLE (pos);
  down  = CELL_DOWN_TILEABLE (pos);
  left  = CELL_LEFT_TILEABLE (pos);
  right = CELL_RIGHT_TILEABLE (pos);

  maz[up] = maz[down] = maz[left] = maz[right] = FRONTIER;

  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
  front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));

  while (g_slist_length (front_cells) > 0)
    {
      current = g_rand_int_range (gr, 0, g_slist_length (front_cells));
      pos = GPOINTER_TO_UINT (g_slist_nth (front_cells, current)->data);

      front_cells = g_slist_remove (front_cells, GUINT_TO_POINTER (pos));
      maz[pos] = IN;

      up    = CELL_UP_TILEABLE (pos);
      down  = CELL_DOWN_TILEABLE (pos);
      left  = CELL_LEFT_TILEABLE (pos);
      right = CELL_RIGHT_TILEABLE (pos);

      d = 0;
      switch (maz[up])
        {
        case OUT:
          maz[up] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (up));
          break;

        case IN:
          d = 1;
          break;

        default:
          break;
        }

      switch (maz[down])
        {
        case OUT:
          maz[down] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (down));
          break;

        case IN:
          d = d | 2;
          break;

        default:
          break;
        }

      switch (maz[left])
        {
        case OUT:
          maz[left] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (left));
          break;

        case IN:
          d = d | 4;
          break;

        default:
          break;
        }

      switch (maz[right])
        {
        case OUT:
          maz[right] = FRONTIER;
          front_cells = g_slist_append (front_cells, GINT_TO_POINTER (right));
          break;

        case IN:
          d = d | 8;
          break;

        default:
          break;
        }

      if (! d)
        {
          g_warning ("maze: prim's tileable: Lack of neighbors.\n"
                     "seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     seed, x, y,multiple,offset);
          break;
        }

      c = 0;
      do
        {
          rnd = (rnd *multiple +offset);
          i = 3 & (rnd / d);
          if (++c > 100)
            {
              i = 99;
              break;
            }
        }
      while (!(d & (1 << i)));

      switch (i)
        {
        case 0:
          maz[WALL_UP_TILEABLE (pos)] = IN;
          break;
        case 1:
          maz[WALL_DOWN_TILEABLE (pos)] = IN;
          break;
        case 2:
          maz[WALL_LEFT_TILEABLE (pos)] = IN;
          break;
        case 3:
          maz[WALL_RIGHT_TILEABLE (pos)] = IN;
          break;
        case 99:
          break;
        default:
          g_warning ("maze: prim's tileable: Going in unknown direction.\n"
                     "i: %d, d: %d, seed: %d, mw: %d, mh: %d, mult: %d, offset: %d\n",
                     i, d, seed, x, y, multiple, offset);
        }
    }

  g_slist_free (front_cells);
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space  = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  GeglRectangle  result = *roi;
  GeglRectangle *in_rect;

  in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
    {
      result = *in_rect;
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle   tile;
  GeglRectangle  *in_extent;
  guchar         *maz = NULL;
  gint            mw;
  gint            mh;
  gint            i;
  gint            j;
  gint            offset_x;
  gint            offset_y;

  in_extent = gegl_operation_source_get_bounding_box (operation, "input");

  gegl_buffer_set_color (output, in_extent, o->bg_color);

  tile.height = o->y;
  tile.width  = o->x;

  mh = in_extent->height / tile.height;
  mw = in_extent->width  / tile.width;

  if (mh > 2 && mw > 2)
    {
      gr = g_rand_new_with_seed (o->seed);

      if (o->tileable)
        {
          /* Tileable mazes must be even. */
          mw -= (mw & 1);
          mh -= (mh & 1);
        }
      else
        {
          mw -= !(mw & 1); /* Normal mazes doesn't work with even-sized mazes. */
          mh -= !(mh & 1); /* Note I don't warn the user about this... */
        }

      /* allocate memory for maze and set to zero */

      maz = (guchar *) g_new0 (guchar, mw * mh);

      offset_x = (in_extent->width - (mw * tile.width)) / 2;
      offset_y = (in_extent->height - (mh * tile.height)) / 2;

      switch (o->algorithm_type)
        {
        case GEGL_MAZE_ALGORITHM_DEPTH_FIRST:
          if (o->tileable)
            {
              depth_first_tileable (0, maz, mw, mh);
            }
          else
            {
              depth_first (mw+1, maz, mw, mh);
            }
          break;

        case GEGL_MAZE_ALGORITHM_PRIM:
          if (o->tileable)
            {
              prim_tileable (maz, mw, mh, o->seed);
            }
          else
            {
              prim (mw+1, maz, mw, mh, o->seed);
            }
          break;
        }

      /* start drawing */

      /* fill the walls of the maze area */

      for (j = 0; j < mh; j++)
        {
          for (i = 0; i < mw; i++)
            {
              if (maz[i + j * mw])
                {
                  tile.x = offset_x + i * tile.width;
                  tile.y = offset_y + j * tile.height;
                  gegl_buffer_set_color (output, &tile, o->fg_color);
                }
            }
        }

      /* if tileable, gaps around the maze have to be filled by extending the
       * maze sides
       */

      if (o->tileable)
        {
          gint right_gap  = in_extent->width - mw * o->x - offset_x;
          gint bottom_gap = in_extent->height - mh * o->y - offset_y;

          /* copy sides of the maze into the corresponding gaps */

          if (offset_y)
            {
              GeglRectangle src = {offset_x, offset_y, mw * o->x, offset_y};
              GeglRectangle dst = {offset_x, 0, mw * o->x, offset_y};

              gegl_buffer_copy (output, &src, GEGL_ABYSS_NONE, output, &dst);
            }

          if (bottom_gap)
            {
              GeglRectangle src = {offset_x, offset_y + (mh - 1) * o->y, mw * o->x, bottom_gap};
              GeglRectangle dst = {offset_x, offset_y + mh * o->y, mw * o->x, bottom_gap};

              gegl_buffer_copy (output, &src, GEGL_ABYSS_NONE, output, &dst);
            }

          if (offset_x)
            {
              GeglRectangle src = {offset_x, offset_y, offset_x, mh * o->y};
              GeglRectangle dst = {0, offset_y, offset_x, mh * o->y};

              gegl_buffer_copy (output, &src, GEGL_ABYSS_NONE, output, &dst);
            }

          if (right_gap)
            {
              GeglRectangle src = {offset_x + (mw - 1) * o->x, offset_y, right_gap, mh * o->y};
              GeglRectangle dst = {offset_x + mw * o->x, offset_y, right_gap, mh * o->y};

              gegl_buffer_copy (output, &src, GEGL_ABYSS_NONE, output, &dst);
            }

          /* finally fill the corners of the gaps area if corners of the maze
           * are walls
           */

          if (maz[0])
            {
              tile.x = 0;
              tile.y = 0;
              tile.width  = offset_x;
              tile.height = offset_y;
              gegl_buffer_set_color (output, &tile, o->fg_color);
            }

          if (maz[mw - 1])
            {
              tile.x = offset_x + mw * o->x;
              tile.y = 0;
              tile.width  = right_gap;
              tile.height = offset_y;
              gegl_buffer_set_color (output, &tile, o->fg_color);
            }

          if (maz[mw * (mh - 1)])
            {
              tile.x = 0;
              tile.y = offset_y + mh * o->y;
              tile.width  = offset_x;
              tile.height = bottom_gap;
              gegl_buffer_set_color (output, &tile, o->fg_color);
            }

          if (maz[mw * mh - 1])
            {
              tile.x = offset_x + mw * o->x;;
              tile.y = offset_y + mh * o->y;
              tile.width  = right_gap;
              tile.height = bottom_gap;
              gegl_buffer_set_color (output, &tile, o->fg_color);
            }
        }

      g_rand_free (gr);
      g_free (maz);
    }

  return TRUE;
}

static gboolean
operation_process (GeglOperation        *operation,
                   GeglOperationContext *context,
                   const gchar          *output_prop,
                   const GeglRectangle  *result,
                   gint                  level)
{
  GeglOperationClass  *operation_class;

  const GeglRectangle *in_rect =
    gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && gegl_rectangle_is_infinite_plane (in_rect))
    {
      gpointer in = gegl_operation_context_get_object (context, "input");
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }

  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  return operation_class->process (operation, context, output_prop, result,
                                   gegl_operation_context_get_level (context));
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare           = prepare;
  operation_class->process           = operation_process;
  operation_class->threaded          = FALSE;
  filter_class->process              = process;

  gegl_operation_class_set_keys (operation_class,
   "name",               "gegl:maze",
   "title",              _("Maze"),
   "categories",         "render",
   "license",            "GPL3+",
   "position-dependent", "true",
   "reference-hash",     "3ead3c39fb74390028889e914a898533",
   "description",        _("Draw a labyrinth"),
   NULL);
}

#endif

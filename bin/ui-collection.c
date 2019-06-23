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
 * Copyright (C) 2019 Øyvind Kolås
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <mrg.h>
#include <mrg-string.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <gegl-paramspecs.h>
#include <gegl-operation.h>
#include <gegl-audio-fragment.h>
#include "mrg-gegl.h"
#include "argvs.h"
#include "ui.h"


static int hack_cols = 5;
static float hack_dim = 5;

static void update_grid_dim (GeState *o)
{
  hack_dim = mrg_height (o->mrg) * 0.2 * o->dir_scale;
  hack_cols = mrg_width (o->mrg) / hack_dim;
}

static void draw_left_triangle (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_new_path (cr);
  cairo_move_to (cr, x+0.9*w, y+0.1*h);
  cairo_line_to (cr, x+0.9*w, y+0.9*h);
  cairo_line_to (cr, x+0.1*w, y+0.5*h);
}

static void draw_folder (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_rectangle (cr, 0.00 *w + x, 0.00 * h + y, 0.33 * w, 0.10 * h);
  cairo_rectangle (cr, 0.00 *w + x, 0.10 * h + y, 0.66 * w, 0.66 * h);
}


static void on_viewer_motion (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
  {
    if (!o->show_controls)
    {
      o->show_controls = 1;
      mrg_queue_draw (o->mrg, NULL);
    }
    if (o->controls_timeout)
    {
      mrg_remove_idle (o->mrg, o->controls_timeout);
      o->controls_timeout = 0;
    }
    o->controls_timeout = mrg_add_timeout (o->mrg, 2000, ui_hide_controls_cb, o);
  }
}

static void entry_load (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;

  if (o->rev)
    argvs_eval ("save");

  g_free (o->path);
  o->path = g_strdup (data2);
  ui_load_path (o);

  //


  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
}

static void entry_select (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;
  o->entry_no = GPOINTER_TO_INT (data2);
  mrg_queue_draw (event->mrg, NULL);
}

static void on_dir_drag (MrgEvent *e, void *data1, void *data2)
{
  static float zoom_pinch_coord[4][2] = {0,};
  static int   zoom_pinch = 0;
  static float orig_zoom = 1.0;

  GeState *o = data1;
  if (e->type == MRG_DRAG_RELEASE)
  {
    zoom_pinch = 0;
    mrg_queue_draw (e->mrg, NULL);
  } else if (e->type == MRG_DRAG_PRESS)
  {
    if (e->device_no == 5)
    {
      zoom_pinch_coord[1][0] = e->x;
      zoom_pinch_coord[1][1] = e->y;

      zoom_pinch_coord[2][0] = zoom_pinch_coord[0][0];
      zoom_pinch_coord[2][1] = zoom_pinch_coord[0][1];
      zoom_pinch_coord[3][0] = zoom_pinch_coord[1][0];
      zoom_pinch_coord[3][1] = zoom_pinch_coord[1][1];

      zoom_pinch = 1;


      orig_zoom = o->dir_scale;
    }
    else if (e->device_no == 1 || e->device_no == 4) /* 1 is mouse pointer 4 is first finger */
    {
      zoom_pinch_coord[0][0] = e->x;
      zoom_pinch_coord[0][1] = e->y;
    }
  } else if (e->type == MRG_DRAG_MOTION)
  {
    if (e->device_no == 1 || e->device_no == 4) /* 1 is mouse pointer 4 is first finger */
    {
      zoom_pinch_coord[0][0] = e->x;
      zoom_pinch_coord[0][1] = e->y;
    }
    if (e->device_no == 5)
    {
      zoom_pinch_coord[1][0] = e->x;
      zoom_pinch_coord[1][1] = e->y;
    }

    if (zoom_pinch)
    {
      float orig_dist = hypotf ( zoom_pinch_coord[2][0]- zoom_pinch_coord[3][0],
                                 zoom_pinch_coord[2][1]- zoom_pinch_coord[3][1]);
      float dist = hypotf (zoom_pinch_coord[0][0] - zoom_pinch_coord[1][0],
                           zoom_pinch_coord[0][1] - zoom_pinch_coord[1][1]);
      o->dir_scale = orig_zoom * dist / orig_dist;
      if (o->dir_scale > 2) o->dir_scale = 2;

      ui_center_active_entry (o);
      o->u -= (e->delta_x )/2; /* doing half contribution of motion per finger */
      o->v -= (e->delta_y )/2; /* is simple and roughly right */
    }
    else
    {
       if (e->device_no == 1 || e->device_no == 4)
       {
         o->u -= (e->delta_x );
         o->v -= (e->delta_y );
       }
    }

    /* auto-center */
    {
      int count = ui_items_count (o);
      if (o->v < 0)
        o->v = 0;
      if (o->v > count/hack_cols * hack_dim - mrg_height(e->mrg)/2)
        o->v = count/hack_cols * hack_dim - mrg_height(e->mrg)/2;
    }

    o->renderer_state = 0;
    mrg_queue_draw (e->mrg, NULL);
    mrg_event_stop_propagate (e);
  }
//  drag_preview (e);
}


static void dir_scroll_cb (MrgEvent *event, void *data1, void *data2)
{
  switch (event->scroll_direction)
  {
     case MRG_SCROLL_DIRECTION_DOWN:
       argvs_eval ("zoom out");
       break;
     case MRG_SCROLL_DIRECTION_UP:
       argvs_eval ("zoom in");
       break;
     default:
       break;
  }
}

static void
copy_one_file (const char *file_path,
               const char *dest_dir)
{
  GError *error = NULL;
  GFile *src = g_file_new_for_uri (file_path);
  GFile *dstp = g_file_new_for_path (dest_dir);
  GFile *dst = g_file_get_child (dstp, g_file_get_basename (src));
  g_file_copy (src, dst, G_FILE_COPY_OVERWRITE , NULL, NULL, NULL, &error);
  if (error)
  {
    g_warning ("file copy failed %s to %s %s\n",
               file_path, dest_dir, error->message);
  }
  g_object_unref (src);
  g_object_unref (dstp);
  g_object_unref (dst);
}

static void dir_drop_cb (MrgEvent *event, void *data1, void *data2)
{
  MrgString *str = mrg_string_new ("");
  GeState *o = global_state;
  const char *p;

  for (p = event->string; *p; p++)
  {
    switch (*p)
    {
      case '\r':
      case '\n':
        if (str->str[0])
        {
          copy_one_file (str->str, get_item_dir (o));
          mrg_string_set (str, "");
        }
      break;
      default:
        mrg_string_append_unichar (str, *p);
      break;
    }
  }
  if (str->str[0])
  {
    copy_one_file (str->str, get_item_dir (o));
  }
  mrg_string_free (str, TRUE);

  populate_path_list (o);
}

static void dir_touch_handling (Mrg *mrg, GeState *o)
{
  cairo_new_path (mrg_cr (mrg));
  cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
  mrg_listen (mrg, MRG_DRAG, on_dir_drag, o, NULL);
  mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
  mrg_listen (mrg, MRG_SCROLL, dir_scroll_cb, o, NULL);

  mrg_listen (mrg, MRG_DROP, dir_drop_cb, o, NULL);
  cairo_new_path (mrg_cr (mrg));
}

static int dir_scroll_dragged = 0;
static void on_dir_scroll_drag (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
  switch (e->type)
  {
    default: break;
    case MRG_DRAG_PRESS:
      dir_scroll_dragged = 1;
      break;
    case MRG_DRAG_RELEASE:
      dir_scroll_dragged = 0;
      break;
    case MRG_DRAG_MOTION:
      {
        int count = ui_items_count (o);
        float height = mrg_height (e->mrg);
#if 0
        y = height * ( o->v / (count/hack_cols * hack_dim) )

        y = height * ( o->v / (count/hack_cols * hack_dim) )
        y/height = ( o->v / (count/hack_cols * hack_dim) )
        y/height * (count/hack_cols * hack_dim) = o->v;
#endif

        o->v += e->delta_y /height * (count/hack_cols * hack_dim);

        if (o->v < 0)
          o->v = 0;
        if (o->v > count/hack_cols * hack_dim - height/2)
          o->v = count/hack_cols * hack_dim - height/2;
      }
      break;
  }

  mrg_event_stop_propagate (e);
}


void ui_collection (GeState *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  float dim;
  int   cols;
  int   no = 0;
  int   count;
  float padding = 0.025;
  float em = mrg_em (mrg);
  dir_touch_handling (mrg, o);

  update_grid_dim (o);
  cols = hack_cols;
  dim = hack_dim;

  count = ui_items_count (o);

  cairo_save (cr);
  cairo_translate (cr, 0, -(int)o->v);
  {
    float x = dim * (no%cols);
    float y = dim * (no/cols);
    float wdim = dim * .6;
    float hdim = dim * .6;

    cairo_new_path (mrg_cr(mrg));

    cairo_rectangle (mrg_cr (mrg), x, y, dim, dim);
    if (no == o->entry_no + 1)
    {
      cairo_set_source_rgba (mrg_cr (mrg), 1,1,0,.5);
      cairo_fill_preserve (mrg_cr (mrg));
    }
    mrg_listen_full (mrg, MRG_CLICK, ui_run_command, "parent", NULL, NULL, NULL);

    draw_left_triangle (mrg, x + (dim-wdim)/2 + dim * padding, y + (dim-hdim)/2 + dim * padding,
      wdim * (1.0-padding*2), hdim *(1.0-padding*2));
      cairo_set_source_rgba (mrg_cr (mrg), 1,1,1,.5);
      cairo_fill (mrg_cr (mrg));
#if 0
    mrg_image (mrg, x + (dim-wdim)/2 + dim * padding, y + (dim-hdim)/2 + dim * padding,
        wdim * (1.0-padding*2), hdim *(1.0-padding*2), 1.0,
         "/usr/share/icons/HighContrast/256x256/actions/go-up.png", NULL, NULL);
#endif

    cairo_new_path (mrg_cr(mrg));
    mrg_set_xy (mrg, x, y + dim - mrg_em(mrg) * 2);
    mrg_printf (mrg, "parent\nfolder");
    no++;
  }

  for (int idx = 0; idx < count; idx ++, no++)
  {
    char *basename = meta_get_child (o, o->path, idx);
    struct stat stat_buf;
      int w, h;
      gchar *path = g_strdup_printf ("%s/%s", o->path, basename);
      char *lastslash = strrchr (path, '/');
      float x = dim * (no%cols);
      float y = dim * (no/cols);
      int is_dir = 0;
      g_free (basename);

      if (y < -dim * 4 + o->v || y > mrg_height (mrg) + dim * 1.5 + o->v)
      {
        g_free (path);
        continue;
      }

      lstat (path, &stat_buf);


      if (S_ISDIR (stat_buf.st_mode))
      {
        float wdim = dim * .6;
        float hdim = dim * .6;

        cairo_rectangle (mrg_cr (mrg), x, y, dim, dim);
        if (no == o->entry_no + 1)
        {
          cairo_set_source_rgba (mrg_cr (mrg), 1,1,0,.5);
          cairo_fill (mrg_cr (mrg));
        }

        draw_folder (mrg, x + (dim-wdim)/2 + dim * padding, y + (dim-hdim)/2 + dim * padding,
          wdim * (1.0-padding*2), hdim *(1.0-padding*2));
        cairo_set_source_rgba (mrg_cr (mrg), 1,1,1,.5);
        cairo_fill (mrg_cr (mrg));
#if 0
        mrg_image (mrg, x + (dim-wdim)/2 + dim * padding, y + (dim-hdim)/2 + dim * padding,
        wdim * (1.0-padding*2), hdim *(1.0-padding*2), 1.0,
         "/usr/share/icons/HighContrast/256x256/places/folder.png", NULL, NULL);
#endif

        is_dir = 1;
      }
      else
      {
    struct stat thumb_stat_buf;
    struct stat suffixed_stat_buf;

      gchar *p2 = ui_suffix_path (path);
      gchar *thumbpath = ui_get_thumb_path (path);

      /* we compute the thumbpath as the hash of the suffixed path, even for
 * gegl documents - for gegl documents this is slightly inaccurate but consistent.
       */
      if (g_file_test (thumbpath, G_FILE_TEST_IS_REGULAR))
      {
        int suffix_exist = 0;
        lstat (thumbpath, &thumb_stat_buf);
        if (lstat (p2, &suffixed_stat_buf) == 0)
          suffix_exist = 1;

        if ((suffix_exist &&
                (suffixed_stat_buf.st_mtime > thumb_stat_buf.st_mtime)) ||
             (stat_buf.st_mtime > thumb_stat_buf.st_mtime))
        {
          unlink (thumbpath);
          mrg_forget_image (mrg, thumbpath);
        }
      }
      g_free (p2);

      if (
         access (thumbpath, F_OK) == 0 && //XXX: query image should suffice
         mrg_query_image (mrg, thumbpath, &w, &h))
      {
        float wdim = dim;
        float hdim = dim;

        if (w > h)
          hdim = dim / (1.0 * w / h);
        else
          wdim = dim * (1.0 * w / h);

        cairo_rectangle (mrg_cr (mrg), x, y, wdim, hdim);

        if (no == o->entry_no + 1)
        {
          cairo_set_source_rgba (mrg_cr (mrg), 1,1,0,1.0);
          cairo_fill_preserve (mrg_cr (mrg));
        }

        mrg_listen (mrg, MRG_TAP, entry_load, o, (void*)g_intern_string (path));
        cairo_new_path (mrg_cr (mrg));

        if (w!=0 && h!=0)
          mrg_image (mrg, x + (dim-wdim)/2 + dim * padding, y + (dim-hdim)/2 + dim * padding,
        wdim * (1.0-padding*2), hdim *(1.0-padding*2), 1.0, thumbpath, NULL, NULL);


      }
      else
      {
         if (!g_file_test (thumbpath, G_FILE_TEST_IS_REGULAR))
         {
           ui_queue_thumb (path);
         }
      }
      g_free (thumbpath);


      }
      if (no == o->entry_no + 1 || is_dir)
      {
        mrg_set_xy (mrg, x, y + dim - mrg_em(mrg));
        mrg_printf (mrg, "%s\n", lastslash+1);
        {
          int stars = meta_get_key_int (o, path, "stars");
          if (stars >= 0)
          {
            mrg_start (mrg, "div.collstars", NULL);
            mrg_set_xy (mrg, x + mrg_em (mrg) * .2, y + mrg_em(mrg) * 1.5);
            for (int i = 0; i < stars; i ++)
            {
              mrg_printf (mrg, "★", lastslash+1);
            }
            mrg_set_style (mrg, "color:gray;");
            for (int i = stars; i < 5; i ++)
            {
              mrg_printf (mrg, "★", lastslash+1);
            }
            mrg_end (mrg);
          }
          else if (!is_dir)
          {
            mrg_start (mrg, "div.collstars", NULL);
            mrg_set_xy (mrg, x + mrg_em (mrg) * .2, y + mrg_em(mrg) * 1.5);
            mrg_set_style (mrg, "color:gray;");
            for (int i = 0; i < 5; i ++)
            {
              mrg_printf (mrg, "★", lastslash+1);
            }
            mrg_end (mrg);
          }
        }
      }
      cairo_new_path (mrg_cr(mrg));
      cairo_rectangle (mrg_cr(mrg), x, y, dim, dim);
#if 0
      if (no == o->entry_no + 1)
        cairo_set_source_rgb (mrg_cr(mrg), 1, 1,0);
      else
        cairo_set_source_rgb (mrg_cr(mrg), 0, 0,0);
      cairo_set_line_width (mrg_cr(mrg), 4);
      cairo_stroke_preserve (mrg_cr(mrg));
#endif
      if (no == o->entry_no + 1)
        mrg_listen_full (mrg, MRG_TAP, entry_load, o, (void*)g_intern_string (path), NULL, NULL);
      else
        mrg_listen_full (mrg, MRG_TAP, entry_select, o, GINT_TO_POINTER(no-1), NULL, NULL);
      cairo_new_path (mrg_cr(mrg));
      g_free (path);
  }

  cairo_restore (cr);

  {
      float height = mrg_height(mrg) * ( mrg_height (mrg) / (count/cols * dim) );
    float yoffset = 0;
    if (height < 4 * em)
    {
      yoffset = (4 * em - height)/2;
      height = 4 * em;
    }
  cairo_rectangle (cr,
                   mrg_width(mrg) - 4 * em,
                   mrg_height(mrg) * ( o->v / (count/cols * dim) ) - yoffset,
                   4 * em,
                   height);
  }
  cairo_set_source_rgba (cr, 1,1,1, dir_scroll_dragged?.3:.2);
  mrg_listen (mrg, MRG_DRAG, on_dir_scroll_drag, o, NULL);
  cairo_fill (cr);

  mrg_add_binding (mrg, "control-left", NULL, NULL, ui_run_command, "colswap prev");
  mrg_add_binding (mrg, "control-right", NULL, NULL, ui_run_command, "colswap next");

  mrg_add_binding (mrg, "left", NULL, NULL, ui_run_command, "collection left");
  mrg_add_binding (mrg, "right", NULL, NULL, ui_run_command, "collection right");
  mrg_add_binding (mrg, "up", NULL, NULL, ui_run_command, "collection up");
  mrg_add_binding (mrg, "down", NULL, NULL, ui_run_command, "collection down");

  mrg_add_binding (mrg, "page-up", NULL, NULL, ui_run_command, "collection page-up");
  mrg_add_binding (mrg, "page-down", NULL, NULL, ui_run_command, "collection page-down");

  mrg_add_binding (mrg, "home", NULL, NULL, ui_run_command, "collection first");
  mrg_add_binding (mrg, "end", NULL, NULL, ui_run_command, "collection last");

  if (o->commandline[0] == 0)
  {
    mrg_add_binding (mrg, "space", NULL, NULL,   ui_run_command, "collection right");
    mrg_add_binding (mrg, "backspace", NULL, NULL,  ui_run_command, "collection left");
  }

  mrg_add_binding (mrg, "alt-right", NULL, NULL, ui_run_command, "collection right");
  mrg_add_binding (mrg, "alt-left", NULL, NULL,  ui_run_command, "collection left");

  if (o->commandline[0]==0)
  {
    mrg_add_binding (mrg, "+", NULL, NULL, ui_run_command, "zoom in");
    mrg_add_binding (mrg, "=", NULL, NULL, ui_run_command, "zoom in");
    mrg_add_binding (mrg, "-", NULL, NULL, ui_run_command, "zoom out");


    mrg_add_binding (mrg, "0", NULL, NULL, ui_run_command, "star 0");
    mrg_add_binding (mrg, "1", NULL, NULL, ui_run_command, "star 1");
    mrg_add_binding (mrg, "2", NULL, NULL, ui_run_command, "star 2");
    mrg_add_binding (mrg, "3", NULL, NULL, ui_run_command, "star 3");
    mrg_add_binding (mrg, "4", NULL, NULL, ui_run_command, "star 4");
    mrg_add_binding (mrg, "5", NULL, NULL, ui_run_command, "star 5");
  }
  mrg_add_binding (mrg, "escape", NULL, "parent folder", ui_run_command, "parent");
  mrg_add_binding (mrg, "control-delete", NULL, NULL,  ui_run_command, "discard");
}


int cmd_collection (COMMAND_ARGS); /* "collection", -1, "<up|left|right|down|first|last>", ""*/
  int cmd_collection (COMMAND_ARGS)
{
  GeState *o = global_state;

  if (!argv[1])
  {
    printf ("current item: %i\n", o->entry_no);
    return 0;
  }
  if (!strcmp(argv[1], "first"))
  {
    o->entry_no = -1;
  }
  else if (!strcmp(argv[1], "last"))
  {
    o->entry_no = ui_items_count (o)-1;
  }
  else if (!strcmp(argv[1], "right"))
  {
    o->entry_no++;
  }
  else if (!strcmp(argv[1], "left"))
  {
    o->entry_no--;
  }
  else if (!strcmp(argv[1], "up"))
  {
    o->entry_no-= hack_cols;
  }
  else if (!strcmp(argv[1], "down"))
  {
    o->entry_no+= hack_cols;
  }

  if (o->entry_no < -1)
    o->entry_no = -1;

  if (o->entry_no >= ui_items_count (o))
    o->entry_no = ui_items_count (o)-1;

  ui_center_active_entry (o);

  mrg_queue_draw (o->mrg, NULL);
  return 0;
}


int cmd_colswap (COMMAND_ARGS); /* "colswap", 1, "<prev|next>", "swap with previous or next collection item "*/
int
cmd_colswap (COMMAND_ARGS)
{
  GeState *o = global_state;
  if (!strcmp (argv[1], "prev"))
  {
    if (o->entry_no <= 0)
      return 0;
    else
    {
      char *dirname = get_item_dir (o);
      meta_swap_children (o, dirname, o->entry_no-1, NULL, o->entry_no, NULL);
      g_free (dirname);
      o->entry_no--;
    }
  }
  else if (!strcmp (argv[1], "next"))
  {
    if (o->entry_no + 1>= g_list_length (o->index))
      return 0;
    else
    {
      char *dirname = get_item_dir (o);
      meta_swap_children (o, dirname, o->entry_no, NULL, o->entry_no+1, NULL);
      g_free (dirname);
      o->entry_no++;
    }
  }
  populate_path_list (o);

  mrg_queue_draw (o->mrg, NULL);
  return 0;
}



void ui_center_active_entry (GeState *o)
{
  int row;
  float pos;
  update_grid_dim (o);

  row = (o->entry_no+1) / hack_cols;
  pos = row * hack_dim;

  if (pos > o->v + mrg_height (o->mrg) - hack_dim ||
      pos < o->v)
    o->v = hack_dim * (row) - mrg_height (o->mrg)/2 + hack_dim;
}

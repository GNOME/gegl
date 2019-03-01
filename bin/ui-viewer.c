
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

static void entry_load (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;

  if (o->rev)
    argvs_eval ("save");

  g_free (o->path);
  o->path = g_strdup (data2);
  ui_load_path (o);
  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
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


static int fade_thumbbar_cb (Mrg *mrg, void *data)
{
  GeState *o = data;
  o->show_thumbbar = 1;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}


static void on_thumbbar_motion (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
  on_viewer_motion (e, data1, NULL);
  {
    o->show_thumbbar = 2;
    if (o->thumbbar_timeout)
    {
      mrg_remove_idle (o->mrg, o->thumbbar_timeout);
      o->thumbbar_timeout = 0;
    }
    o->thumbbar_timeout = mrg_add_timeout (o->mrg, 4000, fade_thumbbar_cb, o);
  }
}


static int slide_cb (Mrg *mrg, void *data)
{
  GeState *o = data;
  o->slide_timeout = 0;
  argvs_eval ("next");
  return 0;
}

static void draw_edit (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_arc (cr, x+0.5*w, y+0.5*h, h * .4, 0.0, G_PI * 2);
}

static void draw_grid (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_rectangle (cr, 0.00 *w + x, 0.00 * h + y, 0.33 * w, 0.33 * h);
  cairo_rectangle (cr, 0.66 *w + x, 0.00 * h + y, 0.33 * w, 0.33 * h);
  cairo_rectangle (cr, 0.00 *w + x, 0.66 * h + y, 0.33 * w, 0.33 * h);
  cairo_rectangle (cr, 0.66 *w + x, 0.66 * h + y, 0.33 * w, 0.33 * h);
}


static void draw_back (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_new_path (cr);
  cairo_move_to (cr, x+0.9*w, y+0.1*h);
  cairo_line_to (cr, x+0.9*w, y+0.9*h);
  cairo_line_to (cr, x+0.1*w, y+0.5*h);
}

static void draw_forward (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_move_to (cr, x+0.1*w, y+0.1*h);
  cairo_line_to (cr, x+0.1*w, y+0.9*h);
  cairo_line_to (cr, x+0.9*w, y+0.5*h);

}

static void on_thumbbar_drag (MrgEvent *e, void *data1, void *data2)
{
  static float pinch_coord[4][2] = {0,};
  static int   pinch = 0;
  static float orig_zoom = 1.0;

  GeState *o = data1;
  //GeglNode *node = data2;

  on_viewer_motion (e, data1, data2);
  if (e->type == MRG_DRAG_RELEASE)
  {
    pinch = 0;
  } else if (e->type == MRG_DRAG_PRESS)
  {
    if (e->device_no == 5) /* 5 is second finger/touch point */
    {
      pinch_coord[1][0] = e->device_x;
      pinch_coord[1][1] = e->device_y;
      pinch_coord[2][0] = pinch_coord[0][0];
      pinch_coord[2][1] = pinch_coord[0][1];
      pinch_coord[3][0] = pinch_coord[1][0];
      pinch_coord[3][1] = pinch_coord[1][1];
      pinch = 1;
      orig_zoom = o->graph_scale;
    }
    else if (e->device_no == 1 || e->device_no == 4) /* 1 is mouse pointer 4 is first finger */
    {
      pinch_coord[0][0] = e->device_x;
      pinch_coord[0][1] = e->device_y;
    }
  } else if (e->type == MRG_DRAG_MOTION)
  {
    if (e->device_no == 1 || e->device_no == 4) /* 1 is mouse pointer 4 is first finger */
    {
      pinch_coord[0][0] = e->device_x;
      pinch_coord[0][1] = e->device_y;
    }
    if (e->device_no == 5)
    {
      pinch_coord[1][0] = e->device_x;
      pinch_coord[1][1] = e->device_y;
    }

    if (pinch)
    {
      float orig_dist = hypotf ( pinch_coord[2][0]- pinch_coord[3][0],
                                 pinch_coord[2][1]- pinch_coord[3][1]);
      float dist = hypotf (pinch_coord[0][0] - pinch_coord[1][0],
                           pinch_coord[0][1] - pinch_coord[1][1]);
    {
      float x, y;
      float screen_cx = (pinch_coord[0][0] + pinch_coord[1][0])/2;
      float screen_cy = (pinch_coord[0][1] + pinch_coord[1][1])/2;
      //get_coords_graph (o, screen_cx, screen_cy, &x, &y);

      x = (o->thumbbar_pan_x + screen_cx) / o->thumbbar_scale;
      y = (o->thumbbar_pan_y + screen_cy) / o->thumbbar_scale;

      o->thumbbar_scale = orig_zoom * (dist / orig_dist);

      o->thumbbar_pan_x = x * o->thumbbar_scale - screen_cx;
      o->thumbbar_pan_y = y * o->thumbbar_scale - screen_cy;

      o->thumbbar_pan_x -= (e->delta_x )/2; /* doing half contribution of motion per finger */
      o->thumbbar_pan_y -= (e->delta_y )/2; /* is simple and roughly right */
    }

    }
    else
    {
      if (e->device_no == 1 || e->device_no == 4)
      {
        o->thumbbar_pan_x -= (e->delta_x );
        o->thumbbar_pan_y -= (e->delta_y );
      }
    }
    mrg_queue_draw (e->mrg, NULL);
  }
  mrg_event_stop_propagate (e);
}


static void on_thumbbar_scroll (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;
  on_viewer_motion (event, data1, NULL);
  switch (event->scroll_direction)
  {
     case MRG_SCROLL_DIRECTION_DOWN:
       o->thumbbar_scale /= 1.1;
       if (o->thumbbar_scale < 0.2)
         o->thumbbar_scale = 0.2;
       break;
     case MRG_SCROLL_DIRECTION_UP:
       o->thumbbar_scale *= 1.1;
       if (o->thumbbar_scale > 3)
         o->thumbbar_scale = 3;
       break;
     default:
       break;
  }
  mrg_queue_draw (event->mrg, NULL);
  mrg_event_stop_propagate (event);
}

static void draw_thumb_bar (GeState *o)
{
  Mrg *mrg = o->mrg;
  float width = mrg_width(mrg);
  float height = mrg_height(mrg);
  cairo_t *cr = mrg_cr (mrg);
  GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);
  float dim = height * 0.15 * o->thumbbar_scale;
  float padding = .025;
  float opacity;

  cairo_save (cr);

  if (o->show_thumbbar > 1)
  {
     opacity = o->thumbbar_opacity * (1.0 - 0.14) + 0.14 * 1.0;
     if (opacity < 0.99)
       mrg_queue_draw (o->mrg, NULL);
  }
  else
  {
     opacity = o->thumbbar_opacity * (1.0 - 0.07) + 0.07 * 0.00;
     if (opacity > 0.02)
       mrg_queue_draw (o->mrg, NULL);
  }
  o->thumbbar_opacity = opacity;

  cairo_rectangle (cr, 0, height-dim, width, dim);
  mrg_listen (mrg, MRG_DRAG, on_thumbbar_drag, o, NULL);
  mrg_listen (mrg, MRG_SCROLL, on_thumbbar_scroll, o, NULL);
  mrg_listen (mrg, MRG_DRAG, on_thumbbar_motion, o, NULL);
  mrg_listen (mrg, MRG_MOTION, on_thumbbar_motion, o, NULL);
  mrg_listen (mrg, MRG_SCROLL, on_thumbbar_motion, o, NULL);
  cairo_new_path (cr);

  if (curr && opacity > 0.01)
  {
    GList *iter = curr;
    float x = mrg_width(mrg)/2-dim/2 - o->thumbbar_pan_x;


    for (iter = curr; iter && x < width; iter = iter->next)
    {
      char *path = ui_suffix_path (iter->data);
      char *thumbpath = ui_get_thumb_path (path);
      int w, h;

      if (
         access (thumbpath, F_OK) == 0 &&
         mrg_query_image (mrg, thumbpath, &w, &h))
      {

        float wdim = dim, hdim = dim;
        if (w > h) hdim = dim / (1.0 * w / h);
        else       wdim = dim * (1.0 * w / h);

        if (w!=0 && h!=0)
        {
          cairo_rectangle (mrg_cr (mrg), x, height-dim, wdim, hdim);
          if (iter == curr)
          cairo_set_source_rgba (mrg_cr (mrg), 1,1,0,.7 * opacity);
          else
          cairo_set_source_rgba (mrg_cr (mrg), 1,1,1,.1 * opacity);
          mrg_listen (mrg, MRG_TAP, entry_load, o, (void*)g_intern_string (iter->data));
          cairo_fill (mrg_cr (mrg));
          mrg_image (mrg, x + dim * padding, height-dim*(1.0-padding),
                     wdim * (1.0-padding*2), hdim *(1.0-padding*2), opacity, thumbpath, NULL, NULL);
        }
      }
      else
      {
         if (access (thumbpath, F_OK) != 0) // only queue if does not exist,
                                            // mrg/stb_image seem to suffer on some of our pngs
         {
           ui_queue_thumb (iter->data);
         }
      }
      x += dim;
      g_free (thumbpath);
      g_free (path);
    }
    x = mrg_width(mrg)/2-dim/2 - o->thumbbar_pan_x;
    dim = height * 0.15 * o->thumbbar_scale;
    x -= dim;

    for (iter = curr->prev; iter && x > -dim; iter = iter->prev)
    {
      char *path = ui_suffix_path (iter->data);
      char *thumbpath = ui_get_thumb_path (path);
      int w, h;

      if (
         access (thumbpath, F_OK) == 0 &&
         mrg_query_image (mrg, thumbpath, &w, &h))
      {

        float wdim = dim, hdim = dim;
        if (w > h) hdim = dim / (1.0 * w / h);
        else       wdim = dim * (1.0 * w / h);
        if (w!=0 && h!=0)
        {
          cairo_rectangle (mrg_cr (mrg), x, height-dim, wdim, hdim);
          cairo_set_source_rgba (mrg_cr (mrg), 1,1,1,.1 * opacity);
          mrg_listen (mrg, MRG_TAP, entry_load, o, (void*)g_intern_string (iter->data));
          cairo_fill (mrg_cr (mrg));
          mrg_image (mrg, x + dim * padding, height-dim*(1.0-padding),
                     wdim * (1.0-padding*2), hdim *(1.0-padding*2), opacity, thumbpath, NULL, NULL);
        }
      }
      else
      {
         if (access (thumbpath, F_OK) != 0) // only queue if does not exist,
                                            // mrg/stb_image seem to suffer on some of our pngs
         {
           ui_queue_thumb (iter->data);
         }
      }
      x -= dim;
      g_free (thumbpath);
      g_free (path);
    }

  }

  cairo_restore (cr);
}


void ui_viewer (GeState *o)
{
  Mrg *mrg = o->mrg;
  float width = mrg_width(mrg);
  float height = mrg_height(mrg);
  cairo_t *cr = mrg_cr (mrg);
  cairo_save (cr);
  cairo_rectangle (cr, 0,0, width, height);

  draw_grid (mrg, height * 0.1/4, height * 0.1/4, height * 0.10, height * 0.10);
  if (o->show_controls)
    ui_contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0, 0, height * 0.15, height * 0.15);
  if (o->show_controls)
  {
    cairo_set_source_rgba (cr, 1,1,1,.1);
    cairo_fill_preserve (cr);
  }
  mrg_listen (mrg, MRG_PRESS, ui_run_command, "parent", NULL);

  draw_back (mrg, height * .1 / 4, height * .5, height * .1, height *.1);
  cairo_close_path (cr);
  if (o->show_controls)
    ui_contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0, height * .3, height * .15, height *.7);
  if (o->show_controls)
  {
    cairo_set_source_rgba (cr, 1,1,1,.1);
    cairo_fill_preserve (cr);
  }
  mrg_listen (mrg, MRG_TAP, ui_run_command, "prev", NULL);
  cairo_new_path (cr);

  draw_forward (mrg, width - height * .12, height * .5, height * .1, height *.1);
  cairo_close_path (cr);
  if (o->show_controls)
    ui_contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, width - height * .15, height * .3, height * .15, height *.7);

  if (o->show_controls)
  {
    cairo_set_source_rgba (cr, 1,1,1,.1);
    cairo_fill_preserve (cr);
  }
  mrg_listen (mrg, MRG_TAP, ui_run_command, "next", NULL);
  draw_edit (mrg, width - height * .15, height * .0, height * .15, height *.15);

  if (o->show_controls)
    ui_contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, width - height * .15, height * .0, height * .15, height *.15);
  if (o->show_controls)
  {
    cairo_set_source_rgba (cr, 1,1,1,.1);
    cairo_fill_preserve (cr);
  }
  mrg_listen (mrg, MRG_PRESS, ui_run_command, "toggle editing", NULL);
  cairo_new_path (cr);

  if (o->show_thumbbar)
    draw_thumb_bar (o);

  if (o->slide_enabled && o->slide_timeout == 0)
  {
    o->slide_timeout =
       mrg_add_timeout (o->mrg, o->slide_pause * 1000, slide_cb, o);
  }
 cairo_restore (cr);

 mrg_add_binding (mrg, "control-s", NULL, NULL, ui_run_command, "toggle slideshow");

 if (o->is_fit)
  {
    mrg_add_binding (mrg, "right", NULL, "next image", ui_run_command, "next");
    mrg_add_binding (mrg, "left", NULL, "previous image",  ui_run_command, "prev");
  }

 mrg_add_binding (mrg, "page-down", NULL, NULL, ui_run_command, "next");
 mrg_add_binding (mrg, "page-up", NULL, NULL,  ui_run_command, "prev");

 mrg_add_binding (mrg, "alt-right", NULL, "next image", ui_run_command, "next");
 mrg_add_binding (mrg, "alt-left", NULL, "previous image",  ui_run_command, "prev");

 if (o->commandline[0]==0)
 {
   mrg_add_binding (mrg, "+", NULL, NULL, ui_run_command, "zoom in");
   mrg_add_binding (mrg, "=", NULL, NULL, ui_run_command, "zoom in");
   mrg_add_binding (mrg, "-", NULL, NULL, ui_run_command, "zoom out");
   mrg_add_binding (mrg, "0", NULL, "pixel for pixel", ui_run_command, "zoom 1.0");
   mrg_add_binding (mrg, "9", NULL, NULL, ui_run_command, "zoom fit");
 }

 mrg_add_binding (mrg, "control-m", NULL, NULL, ui_run_command, "toggle mipmap");
 mrg_add_binding (mrg, "control-y", NULL, NULL, ui_run_command, "toggle colormanaged-display");


  mrg_add_binding (mrg, "control-delete", NULL, NULL,  ui_run_command, "discard");
}

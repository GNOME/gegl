/* This file is part of GEGL editor -- an mrg frontend for GEGL
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
 * Copyright (C) 2015, 2018, 2019 Øyvind Kolås pippin@gimp.org
 */

/* The code in this file is an image viewer/editor written using microraptor
 * gui and GEGL. It renders the UI directly from GEGLs data structures.
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#if HAVE_MRG

const char *css =
"div.properties { color: blue; padding-left:1em; padding-bottom: 1em; position: absolute; top: 1em; left: 40%; width:60%; background-color:rgba(1,0,0,0.5);}\n"
"div.property   { color: white; margin-top: -.5em; background:transparent;}\n"
"div.propname { color: white;}\n"
"div.propvalue { color: yellow;}\n"

"dl.bindings   { font-size: 1.8vh; color:white; position:absolute;left:1em;top:0%;background-color: rgba(0,0,0,0.7); width: 100%; height: 40%; padding-left: 1em; padding-top:1em;}\n"
"dt.binding   { color:white; }\n"

"div.graph {position:absolute; top: 0; left: 0; width:30%; height:50%; color:white; }\n"


"div.node {border: 1px solid white; position: absolute; background-color: rgba(0,0,0,0.75); color:white; padding-left:1em;padding-right:1em;height:2em;width:8em;padding-top:1em;}\n"


"div.props {}\n"
"a { color: yellow; text-decoration: none;  }\n"


"div.shell{  color:white; position:fixed;left:0em;top:50%;background-color: rgba(0,0,0,0.35); width:100%; height: 40%; padding-left: 1em; padding-top:1em;}\n"
//"div.shell { font-size: 0.8em; background-color:rgba(0,0,0,0.5);color:white; }\n"
"div.shellline { background-color:rgba(0,0,0,0.0);color:white; }\n"
"div.prompt { color:#7aa; display: inline; }\n"
"div.commandline { color:white; display: inline; }\n"
"";


void mrg_gegl_dirty (void);


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

/* gets the node which is the direct consumer, and not a clone.
 *
 * valid after update_ui_consumers_list (State *o, GeglNode *iter)
 */
static GeglNode *gegl_node_get_ui_consumer (GeglNode    *node,
                                            const char  *output_pad,
                                            const char **consumer_pad);

static GeglNode *gegl_node_get_consumer_no (GeglNode *node,
                                            const char *output_pad,
                                            const char **consumer_pad,
                                            int no)
{
  GeglNode *consumer = NULL;
  GeglNode **nodes = NULL;
  const gchar **consumer_names = NULL;
  int count;
  if (node == NULL)
    return NULL;

  count = gegl_node_get_consumers (node, "output", &nodes, &consumer_names);
  if (count > no){
     /* XXX : look into inverting list in get_consumers */
     //consumer = nodes[count-no-1];
     consumer = nodes[no];
     if (consumer_pad)
       *consumer_pad = g_intern_string (consumer_names[count-no-1]);
  }
  g_free (nodes);
  g_free (consumer_names);
  return consumer;
}

int use_ui = 1;

int renderer_dirty = 0;

#define printf(foo...) \
   do{ MrgString *str = mrg_string_new_printf (foo);\
       if (use_ui) {\
       MrgString *line = mrg_string_new (scrollback?scrollback->data:"");\
       for (char *p= str->str; *p; p++) { \
         if (*p == '\n') { \
           char *old = scrollback ? scrollback->data : NULL;\
           if (old)\
           {\
             mrg_list_remove (&scrollback, old);\
           }\
           mrg_list_prepend (&scrollback, strdup (line->str));\
           mrg_list_prepend (&scrollback, strdup (""));\
           mrg_string_set (line, "");\
         } else { \
           char *old = scrollback ? scrollback->data : NULL;\
           mrg_string_append_byte (line, *p);\
           if (old)\
           {\
             mrg_list_remove (&scrollback, old);\
           }\
           mrg_list_prepend (&scrollback, strdup (line->str));\
         } \
       } \
       mrg_string_free (line, 1);\
       }\
       else \
       { fprintf (stdout, "%s", str->str);\
       }\
       mrg_string_free (str, 1);\
   }while(0)

MrgList *scrollback = NULL; /* scrollback buffer of free() able strings
                               with latest prepended */

//static int audio_start = 0; /* which sample no is at the start of our circular buffer */

static int audio_started = 0;

enum {
  GEGL_RENDERER_BLIT = 0,
  GEGL_RENDERER_BLIT_MIPMAP,
  GEGL_RENDERER_THREAD,
  GEGL_RENDERER_IDLE,
  //GEGL_RENDERER_IDLE_MIPMAP,
  //GEGL_RENDERER_THREAD_MIPMAP,
};
static int renderer = GEGL_RENDERER_BLIT;

/*  this structure contains the full application state, and is what
 *  re-renderings of the UI is directly based on.
 */
typedef struct _State State;
struct _State {
  void      (*ui) (Mrg *mrg, void *state);
  Mrg        *mrg;
  char       *path;      /* path of edited file or open folder  */

  char       *src_path; /* path to (immutable) source image. */

  char       *save_path; /* the exported .gegl file, or .png with embedded .gegl file,
                            the file that is written to on save. This differs depending
                            on type of input file.
                          */
  GList         *paths;  /* list of full paths to entries in collection/path/containing path,
                            XXX: could be replaced with URIs  */



  GeglBuffer    *buffer;
  GeglNode      *gegl;
  GeglNode      *source;  /* a file-loader or another swapped in buffer provider that is the
                             image data source for the loaded image.  */
  GeglNode      *save;    /* node rigged up for saving file XXX: might be bitrotted */

  GeglNode      *sink;    /* the sink which we're rendering content and graph from */
  GeglNode      *active;  /* the node being actively inspected  */

  int            pad_active; /* 0=input 1=aux 2=output (default)*/

  GThread       *renderer_thread; /* only used when GEGL_RENDERER=thread is set in environment */
  int            entry_no; /* used in collection-view, and set by go_parent */

  int            is_dir;  // is current in dir mode

  int            show_bindings;

  GeglNode      *processor_node; /* the node we have a processor for */
  GeglProcessor *processor;
  GeglBuffer    *processor_buffer;
  int            renderer_state;
  int            editing_op_name;
  char           new_opname[1024];
  int            rev;

  int            concurrent_thumbnailers;

  float          u, v;
  float          scale;
  float          dir_scale;
  float          render_quality; /* default (and in code swapped for preview_quality during preview rendering, this is the canonical read location for the value)  */
  float          preview_quality;

  int            show_graph;
  int            show_controls;
  int            controls_timeout;
  int            frame_no;

  char         **ops; // the operations part of the commandline, if any
  float          slide_pause;
  int            slide_enabled;
  int            slide_timeout;

  GeglNode      *gegl_decode;
  GeglNode      *decode_load;
  GeglNode      *decode_store;
  int            playing;
  int            color_manage_display;

  int            is_video;
  int            prev_frame_played;
  double         prev_ms;

  GHashTable    *ui_consumer;
};

static State *global_state = NULL;  // XXX: for now we  rely on
                                    //      global state to make events/scripting work
                                    //      refactoring this away would be nice, but
                                    //      not a problem to have in a lua port of the same

typedef struct Setting {
  char *name;
  char *description;
  int   offset;
  int   type;
  int   read_only;
} Setting;

#define FLOAT_PROP(name, description) \
  {#name, description, offsetof (State, name), 1, 0}
#define INT_PROP(name, description) \
  {#name, description, offsetof (State, name), 0, 0}
#define STRING_PROP(name, description) \
  {#name, description, offsetof (State, name), 2, 0}
#define FLOAT_PROP_RO(name, description) \
  {#name, description, offsetof (State, name), 1, 1}
#define INT_PROP_RO(name, description) \
  {#name, description, offsetof (State, name), 0, 1}
#define STRING_PROP_RO(name, description) \
  {#name, description, offsetof (State, name), 2, 1}

Setting settings[]=
{
  STRING_PROP_RO(path, "path of current document"),
  STRING_PROP_RO(save_path, "save path, might be different from path if current path is an immutable source image itself"),
  STRING_PROP_RO(src_path, "source path the immutable source image currently being edited"),

  FLOAT_PROP(u, "horizontal coordinate of top-left in display/scaled by scale factor coordinates"),
  FLOAT_PROP(v, "vertical coordinate of top-left in display/scaled by scale factor coordinates"),
  FLOAT_PROP(render_quality, "1.0 = normal 2.0 = render at 2.0 zoom factor 4.0 render at 25%"),
  FLOAT_PROP(preview_quality, "preview quality for use during some interactions, same scale as render-quality"),
  INT_PROP(show_graph, "show the graph (and commandline)"),
  INT_PROP(show_controls, "show image viewer controls (maybe merge with show-graph and give better name)"),
  INT_PROP(slide_enabled, "slide show going"),
  INT_PROP_RO(is_video, ""),
  INT_PROP(color_manage_display, "perform ICC color management and convert output to display ICC profile instead of passing out sRGB, passing out sRGB is faster."),
  INT_PROP(playing, "wheter we are playing or not set to 0 for pause 1 for playing"),
  INT_PROP(concurrent_thumbnailers, "number of child processes spawned at the same time doing thumbnailing"),
  INT_PROP(frame_no, "current frame number in video/animation"),
  FLOAT_PROP(scale, "display scale factor"),
  INT_PROP(show_bindings, "show currently valid keybindings"),

};

static char *suffix = "-gegl";


void   gegl_meta_set (const char *path, const char *meta_data);
char * gegl_meta_get (const char *path);
GExiv2Orientation path_get_orientation (const char *path);

static char *suffix_path (const char *path);
static char *unsuffix_path (const char *path);
static int is_gegl_path (const char *path);

static void contrasty_stroke (cairo_t *cr);

static char *thumb_folder (void)
{
  static char *path = NULL;
  if (!path)
  {
    char command[2048];
    path = g_build_filename (g_get_user_cache_dir(),
                             GEGL_LIBRARY,
                             "thumbnails",
                             NULL);
    sprintf (command, "mkdir -p %s > /dev/null 2>&1", path);
    system (command);
  }
  return path;
}

gchar *get_thumb_path (const char *path);
gchar *get_thumb_path (const char *path)
{
  gchar *ret;
  gchar *uri = g_strdup_printf ("file://%s", path);
  gchar *hex = g_compute_checksum_for_string (G_CHECKSUM_MD5, uri, -1);
  int i;
  for (i = 0; hex[i]; i++)
    hex[i] = tolower (hex[i]);
  ret = g_strdup_printf ("%s/%s.jpg", thumb_folder(), hex);
  g_free (uri);
  g_free (hex);
  return ret;
}


static void load_path (State *o);
static void go_next   (State *o);
static void go_prev   (State *o);
static void go_parent (State *o);

static void get_coords                (State *o, float  screen_x, float screen_y,
                                                 float *gegl_x,   float *gegl_y);
static void drag_preview              (MrgEvent *e);
static void load_into_buffer          (State *o, const char *path);
static void zoom_to_fit               (State *o);
static void center                    (State *o);
static void gegl_ui                   (Mrg *mrg, void *data);
int         mrg_ui_main               (int argc, char **argv, char **ops);
void        gegl_meta_set             (const char *path, const char *meta_data);
char *      gegl_meta_get             (const char *path);
static void on_viewer_motion          (MrgEvent *e, void *data1, void *data2);
int         gegl_str_has_image_suffix (char *path);
int         gegl_str_has_video_suffix (char *path);

static int str_has_visual_suffix (char *path)
{
  return gegl_str_has_image_suffix (path) || gegl_str_has_video_suffix (path);
}


static void populate_path_list (State *o)
{
  struct dirent **namelist;
  int i;
  struct stat stat_buf;
  char *path = strdup (o->path);
  int n;
  while (o->paths)
    {
      char *freed = o->paths->data;
      o->paths = g_list_remove (o->paths, freed);
      g_free (freed);
    }

  lstat (o->path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {
    char *lastslash = strrchr (path, '/');
    if (lastslash)
      {
        if (lastslash == path)
          lastslash[1] = '\0';
        else
          lastslash[0] = '\0';
      }
  }
  n = scandir (path, &namelist, NULL, alphasort);

  for (i = 0; i < n; i++)
  {
    if (namelist[i]->d_name[0] != '.')
    {
      gchar *fpath = g_strdup_printf ("%s/%s", path, namelist[i]->d_name);
      lstat (fpath, &stat_buf);
      if (S_ISDIR (stat_buf.st_mode))
      {
         o->paths = g_list_append (o->paths, fpath);
      }
      else
        g_free (fpath);
    }
  }

  for (i = 0; i < n; i++)
  {
    if (namelist[i]->d_name[0] != '.' &&
        str_has_visual_suffix (namelist[i]->d_name))
    {
      gchar *fpath = g_strdup_printf ("%s/%s", path, namelist[i]->d_name);

      lstat (fpath, &stat_buf);
      if (S_ISREG (stat_buf.st_mode))
      {
        if (is_gegl_path (fpath))
        {
          char *tmp = unsuffix_path (fpath);
          g_free (fpath);
          fpath = g_strdup (tmp);
          g_free (tmp);
        }

        if (!g_list_find_custom (o->paths, fpath, (void*)g_strcmp0))
        {
          o->paths = g_list_append (o->paths, fpath);
        }
        else
          g_free (fpath);
      }
    }
  }

  for (int i = 0; i < n;i++)
    free(namelist[i]);
  free (namelist);
}

char **ops = NULL;  /* initialized by the non-ui main() before ours get called */

static void open_audio (Mrg *mrg, int frequency)
{
  mrg_pcm_set_sample_rate (mrg, frequency);
  mrg_pcm_set_format (mrg, MRG_s16S);
}

static void end_audio (void)
{
}

MrgList *thumb_queue = NULL;
typedef struct ThumbQueueItem
{
  char *path;
  char *tempthumbpath;
  char *thumbpath;
  GPid  pid;
} ThumbQueueItem;

static void thumb_queue_item_free (ThumbQueueItem *item)
{
  if (item->path)
    g_free (item->path);
  if (item->thumbpath)
    g_free (item->thumbpath);
  if (item->tempthumbpath)
    g_free (item->tempthumbpath);
  if (item->pid)
    kill (item->pid, 9);
  g_free (item);
}

#if 0
static char *sh_esc (const char *input)
{
  MrgString *str = mrg_string_new ("");
  for (const char *s = input; *s; s++)
  {
    switch (*s)
    {
      case ' ':
        mrg_string_append_byte (str, '\\');
        mrg_string_append_byte (str, ' ');
        break;
      default:
        mrg_string_append_byte (str, *s);
        break;
    }
  }
  return mrg_string_dissolve (str);
}
#endif


static void load_path_inner (State *o, char *path);

static void run_command (MrgEvent *event_or_null, void *commandline, void *ignored);

static void generate_thumb_self (ThumbQueueItem *item)
{
  State *o = global_state;
  load_path_inner (o, item->path);

  run_command (NULL, "convert-space name=sRGB", NULL);
  run_command (NULL, "convert-format format=\"R'G'B' float\"", NULL);
  run_command (NULL, "scale-size-keepaspect x=256 y=0 sampler=cubic", NULL);

  gegl_node_link_many (o->sink, o->save, NULL);
  gegl_node_set (o->save, "path", item->thumbpath, NULL);
  gegl_node_process (o->save);
  mrg_list_remove (&thumb_queue, item);
  thumb_queue_item_free (item);
  mrg_queue_draw (global_state->mrg, NULL);
}

static void generate_thumb (ThumbQueueItem *item)
{
  GPid child_pid = -1;
  GError *error = NULL;
  char  *savepath=g_strdup_printf ("path=%s", item->tempthumbpath);
  char  *path2=g_strdup (item->path);
  char *argv[]={"gegl",
     path2, "--",
     "convert-space", "name=sRGB",
     "convert-format", "format=R'G'B' float",
     "scale-size-keepaspect", "x=256", "y=0", "sampler=cubic",
     "cache",
     "jpg-save", savepath,
     NULL};

  if (item->pid)
  {
    if (kill(item->pid, 0) != 0)
    {
      rename (item->tempthumbpath, item->thumbpath);
      mrg_list_remove (&thumb_queue, item);
      thumb_queue_item_free (item);
      mrg_queue_draw (global_state->mrg, NULL);
    }
    goto cleanup;
  }
  /* spawning new gegl processes takes up a lot of time, perhaps passing multiple files
     in one go, or having a way of appending requests to existing processes would be good..

     perhaps doing simple downscales with no filters should be done with a faster tool?
     for thumbnail generation the mipmap downscale is already a very high quality result-
     that perhaps should be used instead of an actual scale op?
   */
  g_spawn_async (NULL, &argv[0], NULL, G_SPAWN_SEARCH_PATH|G_SPAWN_SEARCH_PATH_FROM_ENVP,  NULL, NULL, &child_pid, &error);
  if (error)
    fprintf (stderr, "%s\n", error->message);

  item->pid = child_pid;
cleanup:
  g_free (path2);
  g_free (savepath);
}


static gboolean renderer_task (gpointer data)
{
  State *o = data;
  static gdouble progress = 0.0;
  void *old_processor = o->processor;
  GeglBuffer *old_buffer = o->processor_buffer;

  if (renderer == GEGL_RENDERER_BLIT||
      renderer == GEGL_RENDERER_BLIT_MIPMAP)
    o->renderer_state = 4;

  switch (o->renderer_state)
  {
    case 0:
      if (renderer_dirty)
      {
        renderer_dirty = 0;
      if (o->processor_node != o->sink)
      {
        o->processor = gegl_node_new_processor (o->sink, NULL);
        o->processor_buffer = g_object_ref (gegl_processor_get_buffer (o->processor));
        if (old_buffer)
          g_object_unref (old_buffer);
        if (old_processor)
          g_object_unref (old_processor);
      if(1){
        GeglRectangle rect = {o->u / o->scale, o->v / o->scale, mrg_width (o->mrg) / o->scale,
                              mrg_height (o->mrg) / o->scale};
        gegl_processor_set_rectangle (o->processor, &rect);
      }
      }
        o->renderer_state = 1;

      }
      else
        if (thumb_queue)
        {
          o->renderer_state = 4;
        }
      else
        g_usleep (4000);
      break; // fallthrough
    case 1:
       if (gegl_processor_work (o->processor, &progress))
       {
         if (o->renderer_state)
           o->renderer_state = 1;
       }
       else
       {
         if (o->renderer_state)
           o->renderer_state = 3;
       }
      break;
    case 3:
      mrg_gegl_dirty ();
      switch (renderer)
      {
        case GEGL_RENDERER_IDLE:
          mrg_queue_draw (o->mrg, NULL);
          break;
        case GEGL_RENDERER_THREAD:
          mrg_queue_draw (o->mrg, NULL);
          g_usleep (4000);
          break;
      }
      o->renderer_state = 0;
      break;
    case 4:

      if (thumb_queue)
      {

        {
          int thumbnailers = o->concurrent_thumbnailers;
          if (thumbnailers < 0) thumbnailers = -thumbnailers;

          if (thumbnailers >= 1)
            generate_thumb (thumb_queue->data);
          if (thumbnailers >=2 && thumb_queue && thumb_queue->next)
            generate_thumb (thumb_queue->next->data);
          if (thumbnailers >=3 && thumb_queue && thumb_queue->next && thumb_queue->next->next)
            generate_thumb (thumb_queue->next->next->data);
        }


        if (o->concurrent_thumbnailers <= 0)
        {

           if (o->is_dir)
           {
             MrgList *item = thumb_queue;
             while (item && ((ThumbQueueItem*)(item->data))->pid == 0) item = item->next;
             if (item)
               generate_thumb_self (item->data);
           }
           else
           {
             fprintf (stderr, "ooof\n");
           }
        }

      }

      o->renderer_state = 0;
      break;
  }
  return TRUE;
}

static gboolean renderer_idle (Mrg *mrg, gpointer data)
{
  return renderer_task (data);
}

static int has_quit = 0;
static gpointer renderer_thread (gpointer data)
{
  while (!has_quit)
  {
    renderer_task (data);
  }
  return 0;
}

int mrg_ui_main (int argc, char **argv, char **ops)
{
  Mrg *mrg = mrg_new (1024, 768, NULL);
  const char *renderer_env = g_getenv ("GEGL_RENDERER");

  State o = {NULL,};

  if (renderer_env)
  {
    if (!strcmp (renderer_env, "blit")) renderer = GEGL_RENDERER_BLIT;
    else if (!strcmp (renderer_env, "blit-mipmap")) renderer = GEGL_RENDERER_BLIT_MIPMAP;
    else if (!strcmp (renderer_env, "mipmap")) renderer = GEGL_RENDERER_BLIT_MIPMAP;
    else if (!strcmp (renderer_env, "thread")) renderer = GEGL_RENDERER_THREAD;
    else if (!strcmp (renderer_env, "idle")) renderer = GEGL_RENDERER_IDLE;
    else renderer = GEGL_RENDERER_IDLE;
  } else
    renderer = GEGL_RENDERER_IDLE;

  mrg_set_title (mrg, "GEGL");

  o.ops = ops;

  gegl_init (&argc, &argv);
  o.gegl            = gegl_node_new (); // so that we have an object to unref
  o.mrg             = mrg;
  o.scale           = 1.0;
  o.render_quality  = 1.0;
  o.preview_quality = 1.0;
  //o.preview_quality = 2.0;
  o.slide_pause     = 5.0;
  o.slide_enabled   = 0;
  o.concurrent_thumbnailers = 2;
  o.show_bindings   = 0;
  o.ui_consumer = g_hash_table_new (g_direct_hash, g_direct_equal);

  if (access (argv[1], F_OK) != -1)
    o.path = realpath (argv[1], NULL);
  else
    {
      printf ("usage: %s <full-path-to-image>\n", argv[0]);
      return -1;
    }

  load_path (&o);
  mrg_set_ui (mrg, gegl_ui, &o);
  global_state = &o;
  on_viewer_motion (NULL, &o, NULL);

  switch (renderer)
  {
    case GEGL_RENDERER_THREAD:
      o.renderer_thread = g_thread_new ("renderer", renderer_thread, &o);
      break;
    case GEGL_RENDERER_IDLE:
      mrg_add_idle (mrg, renderer_idle, &o);
      break;
    case GEGL_RENDERER_BLIT:
    case GEGL_RENDERER_BLIT_MIPMAP:
      break;
  }

  if (o.ops)
  {
    o.show_graph = 1;
  }

  mrg_main (mrg);
  has_quit = 1;
  if (renderer == GEGL_RENDERER_THREAD)
    g_thread_join (o.renderer_thread);


  g_clear_object (&o.gegl);
  g_clear_object (&o.processor);
  g_clear_object (&o.processor_buffer);
  g_clear_object (&o.buffer);
  gegl_exit ();

  end_audio ();
  return 0;
}

static int hide_controls_cb (Mrg *mrg, void *data)
{
  State *o = data;
  o->controls_timeout = 0;
  o->show_controls    = 0;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

static void on_viewer_motion (MrgEvent *e, void *data1, void *data2)
{
  State *o = data1;
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
    o->controls_timeout = mrg_add_timeout (o->mrg, 1000, hide_controls_cb, o);
  }
}

static int node_select_hack = 0;
static void node_press (MrgEvent *e, void *data1, void *data2)
{
  State *o = data2;
  GeglNode *new_active = data1;

  o->active = new_active;
  mrg_event_stop_propagate (e);
  node_select_hack = 1;

  mrg_queue_draw (e->mrg, NULL);
}


static void on_pan_drag (MrgEvent *e, void *data1, void *data2)
{
  static float zoom_pinch_coord[4][2] = {0,};
  static int   zoom_pinch = 0;
  static float orig_zoom = 1.0;

  State *o = data1;
  on_viewer_motion (e, data1, data2);
  if (e->type == MRG_DRAG_RELEASE && node_select_hack == 0)
  {
    float x = (e->x + o->u) / o->scale;
    float y = (e->y + o->v) / o->scale;
    GeglNode *picked = NULL;
    picked = gegl_node_detect (o->sink, x, y);
    if (picked)
    {
      const char *picked_op = gegl_node_get_operation (picked);
      if (g_str_equal (picked_op, "gegl:png-load")||
          g_str_equal (picked_op, "gegl:jpg-load")||
          g_str_equal (picked_op, "gegl:tiff-load"))
      {
        GeglNode *parent = gegl_node_get_parent (picked);
        if (g_str_equal (gegl_node_get_operation (parent), "gegl:load"))
          picked = parent;
      }

      o->active = picked;
    }

    zoom_pinch = 0;
  } else if (e->type == MRG_DRAG_PRESS)
  {
    if (e->device_no == 5) /* first occurence of device_no=5, which is seond finger */
    {
      zoom_pinch_coord[1][0] = e->x;
      zoom_pinch_coord[1][1] = e->y;

      zoom_pinch_coord[2][0] = zoom_pinch_coord[0][0];
      zoom_pinch_coord[2][1] = zoom_pinch_coord[0][1];
      zoom_pinch_coord[3][0] = zoom_pinch_coord[1][0];
      zoom_pinch_coord[3][1] = zoom_pinch_coord[1][1];

      zoom_pinch = 1;
      orig_zoom = o->scale;
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
    {
      float x, y;
      float screen_cx = (zoom_pinch_coord[0][0] + zoom_pinch_coord[1][0])/2;
      float screen_cy = (zoom_pinch_coord[0][1] + zoom_pinch_coord[1][1])/2;
      /* do the zoom-pinch over the average touch position */
      get_coords (o, screen_cx, screen_cy, &x, &y);
      o->scale = orig_zoom * dist / orig_dist;
      o->u = x * o->scale - screen_cx;
      o->v = y * o->scale - screen_cy;

      o->u -= (e->delta_x )/2; /* doing half contribution of motion per finger */
      o->v -= (e->delta_y )/2; /* is simple and roughly right */
    }

    }
    else
    {
      if (e->device_no == 1 || e->device_no == 4)
      {
        o->u -= (e->delta_x );
        o->v -= (e->delta_y );
      }
    }

    o->renderer_state = 0;
    mrg_queue_draw (e->mrg, NULL);
    mrg_event_stop_propagate (e);
  }
  node_select_hack = 0;
  drag_preview (e);
}

static int hack_cols = 5;
static float hack_dim = 5;

static void update_grid_dim (State *o)
{
  hack_dim = mrg_height (o->mrg) * 0.2 * o->dir_scale;
  hack_cols = mrg_width (o->mrg) / hack_dim;
}

static void center_active_entry (State *o)
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


static void on_dir_drag (MrgEvent *e, void *data1, void *data2)
{
  static float zoom_pinch_coord[4][2] = {0,};
  static int   zoom_pinch = 0;
  static float orig_zoom = 1.0;

  State *o = data1;
  if (e->type == MRG_DRAG_RELEASE)
  {
    zoom_pinch = 0;
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

      center_active_entry (o);
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

    o->renderer_state = 0;
    mrg_queue_draw (e->mrg, NULL);
    mrg_event_stop_propagate (e);
  }
  drag_preview (e);
}


static GeglNode *add_output (State *o, GeglNode *active, const char *optype);
static GeglNode *add_aux (State *o, GeglNode *active, const char *optype);

GeglPath *path = NULL;

static void on_paint_drag (MrgEvent *e, void *data1, void *data2)
{
  State *o = data1;
  float x = (e->x + o->u) / o->scale;
  float y = (e->y + o->v) / o->scale;

  switch (e->type)
  {
    default: break;
    case MRG_DRAG_PRESS:
      o->active = add_output (o, o->active, "gegl:over");
      o->active = add_aux (o, o->active, "gegl:vector-stroke");
      /* XXX: gegl:vector-stroke is written to be able to have a chain of it be succesive strokes,
              it seems like tiles of the buffer are not properly synced for that */

      path = gegl_path_new ();
      gegl_path_append (path, 'M', x, y);
      gegl_path_append (path, 'L', x, y);
      gegl_node_set (o->active, "d", path, "color", gegl_color_new("blue"),
                     "width", 16.0 / o->scale, NULL);
      break;
    case MRG_DRAG_MOTION:
      gegl_path_append (path, 'L', x, y);
      break;
    case MRG_DRAG_RELEASE:
      o->active = gegl_node_get_ui_consumer (o->active, "output", NULL);
      break;
  }
  renderer_dirty++;
  o->rev++;
  mrg_queue_draw (e->mrg, NULL);
  mrg_event_stop_propagate (e);
  //drag_preview (e);
}


static void on_move_drag (MrgEvent *e, void *data1, void *data2)
{
  State *o = data1;
  switch (e->type)
  {
    default: break;
    case MRG_DRAG_PRESS:
{
    float x = (e->x + o->u) / o->scale;
    float y = (e->y + o->v) / o->scale;
    GeglNode *picked = NULL;
    picked = gegl_node_detect (o->sink, x, y);
    if (picked)
    {
      const char *picked_op = gegl_node_get_operation (picked);
      if (g_str_equal (picked_op, "gegl:png-load")||
          g_str_equal (picked_op, "gegl:jpg-load")||
          g_str_equal (picked_op, "gegl:gif-load")||
          g_str_equal (picked_op, "gegl:tiff-load"))
      {
        GeglNode *parent = gegl_node_get_parent (picked);
        if (g_str_equal (gegl_node_get_operation (parent), "gegl:load"))
          picked = parent;
      }

      o->active = picked;
    }
}

      if (g_str_equal (gegl_node_get_operation (o->active), "gegl:translate"))
      {
        // no changing of active
      }
      else
      {
        GeglNode *iter = o->active;
        GeglNode *last = o->active;
        while (iter)
        {
          const gchar *input_pad = NULL;
          GeglNode *consumer = gegl_node_get_ui_consumer (iter, "output", &input_pad);
          last = iter;
          if (consumer && g_str_equal (input_pad, "input"))
            iter = consumer;
          else
            iter = NULL;
        }
        if (g_str_equal (gegl_node_get_operation (last), "gegl:translate"))
        {
          o->active = last;
        }
        else
        {
          o->active = add_output (o, last, "gegl:translate");
        }
      }
      break;
    case MRG_DRAG_MOTION:
      {
        gdouble x, y;
        gegl_node_get (o->active, "x", &x, "y", &y, NULL);
        x += e->delta_x / o->scale;
        y += e->delta_y / o->scale;
        gegl_node_set (o->active, "x", floorf(x), "y", floorf(y), NULL);
      }
      break;
    case MRG_DRAG_RELEASE:
      break;
  }
  renderer_dirty++;
  o->rev++;
  mrg_queue_draw (e->mrg, NULL);
  mrg_event_stop_propagate (e);
}

#if 0

static void prop_double_drag_cb (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  GParamSpec *pspec = data2;
  GeglParamSpecDouble *gspec = data2;
  gdouble value = 0.0;
  float range = gspec->ui_maximum - gspec->ui_minimum;

  value = e->x / mrg_width (e->mrg);
  value = value * range + gspec->ui_minimum;
  gegl_node_set (node, pspec->name, value, NULL);

  drag_preview (e);
  mrg_event_stop_propagate (e);

  mrg_queue_draw (e->mrg, NULL);
}

static void prop_int_drag_cb (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  GParamSpec *pspec = data2;
  GeglParamSpecInt *gspec = data2;
  gint value = 0.0;
  float range = gspec->ui_maximum - gspec->ui_minimum;

  value = e->x / mrg_width (e->mrg) * range + gspec->ui_minimum;
  gegl_node_set (node, pspec->name, value, NULL);

  drag_preview (e);
  mrg_event_stop_propagate (e);

  mrg_queue_draw (e->mrg, NULL);
}
#endif


static gchar *edited_prop = NULL;

static void update_prop (const char *new_string, void *node_p)
{
  GeglNode *node = node_p;
  gegl_node_set (node, edited_prop, new_string, NULL);
  renderer_dirty++;
  global_state->rev++;
}


static void update_prop_double (const char *new_string, void *node_p)
{
  GeglNode *node = node_p;
  gdouble value = g_strtod (new_string, NULL);
  gegl_node_set (node, edited_prop, value, NULL);
  renderer_dirty++;
  global_state->rev++;
}

static void update_prop_int (const char *new_string, void *node_p)
{
  GeglNode *node = node_p;
  gint value = g_strtod (new_string, NULL);
  gegl_node_set (node, edited_prop, value, NULL);
  renderer_dirty++;
  global_state->rev++;
}


static void prop_toggle_boolean (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  const char *propname = data2;
  gboolean value;
  gegl_node_get (node, propname, &value, NULL);
  value = value ? FALSE : TRUE;
  gegl_node_set (node, propname, value, NULL);
  renderer_dirty++;
  global_state->rev++;
  mrg_event_stop_propagate (e);
}


static void set_edited_prop (MrgEvent *e, void *data1, void *data2)
{
  if (edited_prop)
    g_free (edited_prop);
  edited_prop = g_strdup (data2);
  mrg_event_stop_propagate (e);

  mrg_set_cursor_pos (e->mrg, 0);
  mrg_queue_draw (e->mrg, NULL);
}

static void unset_edited_prop (MrgEvent *e, void *data1, void *data2)
{
  if (edited_prop)
    g_free (edited_prop);
  edited_prop = NULL;
  mrg_event_stop_propagate (e);

  mrg_queue_draw (e->mrg, NULL);
}


int cmd_todo (COMMAND_ARGS);/* "todo", -1, "", ""*/

int
cmd_todo (COMMAND_ARGS)
{
  printf ("commandline improvements, scrolling, autohide.\n");
  printf ("op selection\n");
  printf ("interpret GUM\n");
  printf ("better int/double edit\n");
  printf ("int/double slider\n");
  printf ("enum selection\n");
  printf ("units in commandline\n");
  printf ("crop mode\n");
  printf ("polyline/bezier on screen editing\n");
  printf ("rewrite in lua\n");
  printf ("animation of properties\n");
  printf ("star/comment storage\n");
  printf ("dir actions: rename, discard\n");
  return 0;
}


int cmd_mipmap (COMMAND_ARGS);/* "mipmap", -1, "", ""*/

int
cmd_mipmap (COMMAND_ARGS)
{
  gboolean curval;
  State *o = global_state;
  if (argv[1])
  {
    if (!strcmp (argv[1], "on")||
        !strcmp (argv[1], "true")||
        !strcmp (argv[1], "1"))
    {
      g_object_set (gegl_config(), "mipmap-rendering", TRUE, NULL);
      renderer = GEGL_RENDERER_BLIT_MIPMAP;
    }
    else
    {
      g_object_set (gegl_config(), "mipmap-rendering", FALSE, NULL);
      renderer = GEGL_RENDERER_IDLE;
    }
  }
  else
  {
    //printf ("mipmap rendering is %s\n", curval?"on":"off");
  }
  g_object_get (gegl_config(), "mipmap-rendering", &curval, NULL);
  printf ("mipmap rendering is %s\n", curval?"on":"off");
  renderer_dirty ++;
  o->rev++;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

static void entry_select (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  o->entry_no = GPOINTER_TO_INT (data2);
  mrg_queue_draw (event->mrg, NULL);
}

static void entry_load (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  g_free (o->path);
  o->path = g_strdup (data2);
  load_path (o);
  mrg_queue_draw (event->mrg, NULL);
}



static void queue_thumb (const char *path, const char *thumbpath)
{
  ThumbQueueItem *item;
  for (MrgList *l = thumb_queue; l; l=l->next)
  {
    item = l->data;
    if (!strcmp (item->path, path))
      return;
    if (!strcmp (item->thumbpath, thumbpath))
      return;
  }
  item = g_malloc0 (sizeof (ThumbQueueItem));
  item->path = g_strdup (path);
  item->thumbpath = g_strdup (thumbpath);
  item->tempthumbpath = g_strdup (item->thumbpath);
  item->tempthumbpath[strlen(item->tempthumbpath)-8]='_';
  mrg_list_append (&thumb_queue, item);
}


static void ui_dir_viewer (State *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  GList *iter;
  float dim;
  int   cols;
  int   no = 0;
  update_grid_dim (o);
  cols = hack_cols;
  dim = hack_dim;

  cairo_rectangle (cr, 0,0, mrg_width(mrg), mrg_height(mrg));
  mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
  cairo_new_path (cr);

  mrg_set_edge_right (mrg, 4095);
  cairo_save (cr);
  cairo_translate (cr, 0, -o->v);
  {
    float x = dim * (no%cols);
    float y = dim * (no/cols);
    mrg_set_xy (mrg, x, y + dim - mrg_em(mrg) * 2);
    mrg_printf (mrg, "parent\nfolder");
    cairo_new_path (mrg_cr(mrg));
    cairo_rectangle (mrg_cr(mrg), x, y, dim, dim);
    if (no == o->entry_no + 1)
      cairo_set_source_rgb (mrg_cr(mrg), 1, 1,0);
    else
      cairo_set_source_rgb (mrg_cr(mrg), 0, 0,0);
    cairo_set_line_width (mrg_cr(mrg), 4);
    cairo_stroke_preserve (mrg_cr(mrg));
    mrg_listen_full (mrg, MRG_CLICK, run_command, "parent", NULL, NULL, NULL);
    cairo_new_path (mrg_cr(mrg));
    no++;
  }

  for (iter = o->paths; iter; iter=iter->next, no++)
  {
    struct stat stat_buf;
      int w, h;
      gchar *path = iter->data;
      char *lastslash = strrchr (path, '/');
      float x = dim * (no%cols);
      float y = dim * (no/cols);
      int is_dir = 0;

      if (y < -dim || y > mrg_height (mrg) + o->v)
        continue;

      lstat (path, &stat_buf);


      if (S_ISDIR (stat_buf.st_mode))
      {
        is_dir = 1;
      }
      else
      {
    struct stat thumb_stat_buf;
    struct stat suffixed_stat_buf;

      gchar *p2 = suffix_path (path);
      gchar *thumbpath = get_thumb_path (p2);
      /* we compute the thumbpath as the hash of the suffixed path, even for gegl
         documents - for gegl documents this is slightly inaccurate.
       */
      if (access (thumbpath, F_OK) == 0)
      {
        int suffix_exist = 0;
        lstat (thumbpath, &thumb_stat_buf);
        if (lstat (p2, &suffixed_stat_buf) == 0)
          suffix_exist = 1;

        if ((suffix_exist && (suffixed_stat_buf.st_mtime >
                              thumb_stat_buf.st_mtime)) ||
                             (stat_buf.st_mtime >
                             thumb_stat_buf.st_mtime))
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

        if (w!=0 && h!=0)
          mrg_image (mrg, x + (dim-wdim)/2, y + (dim-hdim)/2,
                     wdim, hdim, 1.0, thumbpath, NULL, NULL);
      }
      else
      {
         if (access (thumbpath, F_OK) != 0) // only queue if does not exist,
                                            // mrg/stb_image seem to suffer on some of our pngs
         {
           queue_thumb (path, thumbpath);
         }
      }
      g_free (thumbpath);


      }
      if (no == o->entry_no + 1 || is_dir)
      {
        mrg_set_xy (mrg, x, y + dim - mrg_em(mrg));
        mrg_printf (mrg, "%s\n", lastslash+1);
      }
      cairo_new_path (mrg_cr(mrg));
      cairo_rectangle (mrg_cr(mrg), x, y, dim, dim);
      if (no == o->entry_no + 1)
        cairo_set_source_rgb (mrg_cr(mrg), 1, 1,0);
      else
        cairo_set_source_rgb (mrg_cr(mrg), 0, 0,0);
      cairo_set_line_width (mrg_cr(mrg), 4);
      cairo_stroke_preserve (mrg_cr(mrg));
      if (no == o->entry_no + 1)
        mrg_listen_full (mrg, MRG_CLICK, entry_load, o, path, NULL, NULL);
      else
        mrg_listen_full (mrg, MRG_CLICK, entry_select, o, GINT_TO_POINTER(no-1), NULL, NULL);
      cairo_new_path (mrg_cr(mrg));
  }
  cairo_restore (cr);

  mrg_add_binding (mrg, "left", NULL, NULL, run_command, "collection left");
  mrg_add_binding (mrg, "right", NULL, NULL, run_command, "collection right");
  mrg_add_binding (mrg, "up", NULL, NULL, run_command, "collection up");
  mrg_add_binding (mrg, "down", NULL, NULL, run_command, "collection down");
}

static int slide_cb (Mrg *mrg, void *data)
{
  State *o = data;
  o->slide_timeout = 0;
  argvs_eval ("next");
  return 0;
}

static void scroll_cb (MrgEvent *event, void *data1, void *data2);

static void ui_viewer (State *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  cairo_rectangle (cr, 0,0, mrg_width(mrg), mrg_height(mrg));

  cairo_scale (cr, mrg_width(mrg), mrg_height(mrg));
  cairo_new_path (cr);
  cairo_rectangle (cr, 0.05, 0.05, 0.05, 0.05);
  cairo_rectangle (cr, 0.15, 0.05, 0.05, 0.05);
  cairo_rectangle (cr, 0.05, 0.15, 0.05, 0.05);
  cairo_rectangle (cr, 0.15, 0.15, 0.05, 0.05);
  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.0, 0.0, 0.2, 0.2);
  mrg_listen (mrg, MRG_PRESS, run_command, "parent", NULL);

  cairo_new_path (cr);
  cairo_move_to (cr, 0.2, 0.8);
  cairo_line_to (cr, 0.2, 1.0);
  cairo_line_to (cr, 0.0, 0.9);
  cairo_close_path (cr);
  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.0, 0.8, 0.2, 0.2);
  mrg_listen (mrg, MRG_PRESS, run_command, "prev", NULL);
  cairo_new_path (cr);

  cairo_move_to (cr, 0.8, 0.8);
  cairo_line_to (cr, 0.8, 1.0);
  cairo_line_to (cr, 1.0, 0.9);
  cairo_close_path (cr);

  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.8, 0.8, 0.2, 0.2);
  mrg_listen (mrg, MRG_PRESS, run_command, "next", NULL);
  cairo_new_path (cr);

  cairo_arc (cr, 0.9, 0.1, 0.1, 0.0, G_PI * 2);

  if (o->show_controls)
    contrasty_stroke (cr);
  else
    cairo_new_path (cr);
  cairo_rectangle (cr, 0.8, 0.0, 0.2, 0.2);
  mrg_listen (mrg, MRG_PRESS, run_command, "toggle editing", NULL);
  cairo_new_path (cr);


  if (o->slide_enabled && o->slide_timeout == 0)
  {
    o->slide_timeout =
       mrg_add_timeout (o->mrg, o->slide_pause * 1000, slide_cb, o);
  }
}

static int deferred_redraw_action (Mrg *mrg, void *data)
{
  mrg_queue_draw (mrg, NULL);
  return 0;
}

static void deferred_redraw (Mrg *mrg, MrgRectangle *rect)
{
  MrgRectangle r; /* copy in call stack of dereference rectangle if pointer
                     is passed in */
  if (rect)
    r = *rect;
  mrg_add_timeout (mrg, 0, deferred_redraw_action, rect?&r:NULL);
}

enum {
  TOOL_PAN=0,
  TOOL_PICK,
  TOOL_PAINT,
  TOOL_MOVE,
};

static int tool = TOOL_PAN;

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

static void dir_touch_handling (Mrg *mrg, State *o)
{
  cairo_new_path (mrg_cr (mrg));
  cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
  mrg_listen (mrg, MRG_DRAG, on_dir_drag, o, NULL);
  mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
  mrg_listen (mrg, MRG_SCROLL, dir_scroll_cb, o, NULL);
  cairo_new_path (mrg_cr (mrg));
}

static void canvas_touch_handling (Mrg *mrg, State *o)
{
  cairo_new_path (mrg_cr (mrg));
  switch (tool)
  {
    case TOOL_PAN:
      cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
      mrg_listen (mrg, MRG_DRAG, on_pan_drag, o, NULL);
      mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      cairo_new_path (mrg_cr (mrg));
      break;
    case TOOL_PAINT:
      cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
      mrg_listen (mrg, MRG_DRAG, on_paint_drag, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      cairo_new_path (mrg_cr (mrg));
      break;
    case TOOL_MOVE:
      cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
      mrg_listen (mrg, MRG_DRAG, on_move_drag, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      cairo_new_path (mrg_cr (mrg));
      break;
  }
}

static GeglNode *add_aux (State *o, GeglNode *active, const char *optype)
{
  GeglNode *ref = active;
  GeglNode *ret = NULL;
  GeglNode *producer;
  if (!gegl_node_has_pad (ref, "aux"))
    return NULL;
  ret = gegl_node_new_child (o->gegl, "operation", optype, NULL);
  producer = gegl_node_get_producer (ref, "aux", NULL);
  if (producer)
  {
    gegl_node_link_many (producer, ret, NULL);
  }
  gegl_node_connect_to (ret, "output", ref, "aux");
  renderer_dirty++;
  o->rev++;
  return ret;
}

static GeglNode *add_output (State *o, GeglNode *active, const char *optype)
{
  GeglNode *ref = active;
  GeglNode *ret = NULL;
  const char *consumer_name = NULL;
  GeglNode *consumer = gegl_node_get_ui_consumer (ref, "output", &consumer_name);
  if (!gegl_node_has_pad (ref, "output"))
    return NULL;

  if (consumer)
  {
    ret = gegl_node_new_child (o->gegl, "operation", optype, NULL);
    gegl_node_link_many (ref, ret, NULL);
    gegl_node_connect_to (ret, "output", consumer, consumer_name);
  }
  renderer_dirty++;
  o->rev++;
  return ret;
}

int cmd_node_add (COMMAND_ARGS);/* "node-add", 1, "<input|output|aux>", "add a neighboring node and permit entering its name, for use in touch ui."*/
int
cmd_node_add (COMMAND_ARGS)
{
  State *o = global_state;
  if (!strcmp(argv[1], "input"))
  {
    State *o = global_state;
    GeglNode *ref = o->active;
    GeglNode *producer = gegl_node_get_producer (o->active, "input", NULL);
    if (!gegl_node_has_pad (ref, "input"))
      return -1;

    o->active = gegl_node_new_child (o->gegl, "operation", "gegl:nop", NULL);

    if (producer)
    {
      gegl_node_connect_to (producer, "output", o->active, "input");
    }
    gegl_node_connect_to (o->active, "output", ref, "input");

    o->editing_op_name = 1;
    mrg_set_cursor_pos (o->mrg, 0);
    o->new_opname[0]=0;
  }
  else if (!strcmp(argv[1], "aux"))
  {
    GeglNode *ref = o->active;
    GeglNode *producer = gegl_node_get_producer (o->active, "aux", NULL);

    if (!gegl_node_has_pad (ref, "aux"))
      return -1;

    o->active = gegl_node_new_child (o->gegl, "operation", "gegl:nop", NULL);

    if (producer)
    {
      gegl_node_connect_to (producer, "output", o->active, "input");
    }
    gegl_node_connect_to (o->active, "output", ref, "aux");

    o->editing_op_name = 1;
    mrg_set_cursor_pos (o->mrg, 0);
    o->new_opname[0]=0;
  }
  else if (!strcmp(argv[1], "output"))
  {
    GeglNode *ref = o->active;
    const char *consumer_name = NULL;
    GeglNode *consumer = gegl_node_get_ui_consumer (o->active, "output", &consumer_name);
    if (!gegl_node_has_pad (ref, "output"))
      return -1;

    if (consumer)
    {
      o->active = gegl_node_new_child (o->gegl, "operation", "gegl:nop", NULL);
      gegl_node_link_many (ref, o->active, NULL);
      gegl_node_connect_to (o->active, "output", consumer, consumer_name);
      o->editing_op_name = 1;
      mrg_set_cursor_pos (o->mrg, 0);
      o->new_opname[0]=0;
    }
  }
  renderer_dirty++;
  o->rev++;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}




#define INDENT_STR "   "

static void list_node_props (State *o, GeglNode *node, int indent)
{
  Mrg *mrg = o->mrg;
  guint n_props;
  int no = 0;
  const char *op_name;
  GParamSpec **pspecs;

  if (!node)
    return;

  op_name = gegl_node_get_operation (node);
  if (!op_name)
    return;
  pspecs = gegl_operation_list_properties (op_name, &n_props);

  mrg_start (mrg, "div.properties", NULL);

  if (pspecs)
  {
    for (gint i = 0; i < n_props; i++)
    {
      mrg_start (mrg, "div.property", NULL);//
      mrg_start (mrg, "div.propname", NULL);//
      //mrg_set_xy (mrg, x + no * mrg_em (mrg) * 7, y - mrg_em (mrg) * .5);

      if (g_type_is_a (pspecs[i]->value_type, G_TYPE_DOUBLE))
      {
        //float xpos;
        //GeglParamSpecDouble *geglspec = (void*)pspecs[i];
        gdouble value;
        gegl_node_get (node, pspecs[i]->name, &value, NULL);


        if (edited_prop && !strcmp (edited_prop, pspecs[i]->name))
        {
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_text_listen (mrg, MRG_CLICK, unset_edited_prop, node, (void*)pspecs[i]->name);
          mrg_edit_start (mrg, update_prop_double, node);
          mrg_printf (mrg, "%.3f", value);
          mrg_edit_end (mrg);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }
        else
        {
          mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, node, (void*)pspecs[i]->name);
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_printf (mrg, "%.3f", value);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }

        no++;
      }
      else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_INT))
      {
        //float xpos;
        //GeglParamSpecInt *geglspec = (void*)pspecs[i];
        gint value;
        gegl_node_get (node, pspecs[i]->name, &value, NULL);

        //mrg_printf (mrg, "%s\n%i\n", pspecs[i]->name, value);

        if (edited_prop && !strcmp (edited_prop, pspecs[i]->name))
        {
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_text_listen (mrg, MRG_CLICK, unset_edited_prop, node, (void*)pspecs[i]->name);
          mrg_edit_start (mrg, update_prop_int, node);
          mrg_printf (mrg, "%i", value);
          mrg_edit_end (mrg);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }
        else
        {
          mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, node, (void*)pspecs[i]->name);
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_printf (mrg, "%i", value);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }
        no++;
      }
      else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_STRING) ||
               g_type_is_a (pspecs[i]->value_type, GEGL_TYPE_PARAM_FILE_PATH))
      {
        char *value = NULL;
        gegl_node_get (node, pspecs[i]->name, &value, NULL);

        if (edited_prop && !strcmp (edited_prop, pspecs[i]->name))
        {
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_text_listen (mrg, MRG_CLICK, unset_edited_prop, node, (void*)pspecs[i]->name);
          mrg_edit_start (mrg, update_prop, node);
          mrg_printf (mrg, "%s", value);
          mrg_edit_end (mrg);
          mrg_text_listen_done (mrg);
          mrg_end (mrg);
        }
        else
        {
          mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, node, (void*)pspecs[i]->name);
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_printf (mrg, "%s\n", value);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }

        if (value)
          g_free (value);
        no++;
      }
      else if (g_type_is_a (pspecs[i]->value_type, GEGL_TYPE_COLOR))
      {
        GeglColor *color;
        char *value = NULL;
        gegl_node_get (node, pspecs[i]->name, &color, NULL);
        g_object_get (color, "string", &value, NULL);

        if (edited_prop && !strcmp (edited_prop, pspecs[i]->name))
        {
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_text_listen (mrg, MRG_CLICK, unset_edited_prop, node, (void*)pspecs[i]->name);
          mrg_edit_start (mrg, update_prop, node);
          mrg_printf (mrg, "%s", value);
          mrg_edit_end (mrg);
          mrg_text_listen_done (mrg);
          mrg_end (mrg);
        }
        else
        {
          mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, node, (void*)pspecs[i]->name);
          mrg_printf (mrg, "%s", pspecs[i]->name);
          mrg_end (mrg);
          mrg_start (mrg, "div.propvalue", NULL);//
          mrg_printf (mrg, "%s", value);
          mrg_end (mrg);
          mrg_text_listen_done (mrg);
        }

        if (value)
          g_free (value);
        no++;
      }
      else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_BOOLEAN))
      {
        gboolean value = FALSE;
        gegl_node_get (node, pspecs[i]->name, &value, NULL);

        mrg_text_listen (mrg, MRG_CLICK, prop_toggle_boolean, node, (void*)pspecs[i]->name);
        mrg_printf (mrg, "%s", pspecs[i]->name);
        mrg_end (mrg);
        mrg_start (mrg, "div.propvalue", NULL);//
        mrg_printf (mrg, "%s", value?"true":"false");
        mrg_end (mrg);
        mrg_text_listen_done (mrg);
        no++;
      }
      else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_ENUM))
      {
        GEnumClass *eclass = g_type_class_peek (pspecs[i]->value_type);
        GEnumValue *evalue;
        gint value;

        gegl_node_get (node, pspecs[i]->name, &value, NULL);
        evalue  = g_enum_get_value (eclass, value);
        mrg_printf (mrg, "%s:", pspecs[i]->name);
        mrg_end (mrg);
        mrg_start (mrg, "div.propvalue", NULL);//
        mrg_printf (mrg, "%s", evalue->value_nick);
        mrg_end (mrg);
        no++;
      }
      else
      {
        mrg_printf (mrg, "%s", pspecs[i]->name);
        mrg_end (mrg);
        no++;
      }

      mrg_end (mrg);
    }
    g_free (pspecs);
  }

  mrg_end (mrg);
}

static void update_string (const char *new_text, void *data)
{
  char *str = data;
  strcpy (str, new_text);
}

  int cmd_edit_opname (COMMAND_ARGS);
int cmd_edit_opname (COMMAND_ARGS) /* "edit-opname", 0, "", "permits changing the current op by typing in a replacement name."*/
{
  State *o = global_state;
  o->editing_op_name = 1;
  o->new_opname[0]=0;
  mrg_set_cursor_pos (o->mrg, 0);
  return 0;
}

static void activate_sink_producer (State *o)
{
  if (o->sink)
    o->active = gegl_node_get_producer (o->sink, "input", NULL);
  else
    o->active = NULL;
  o->pad_active = 2;
}

  int cmd_activate (COMMAND_ARGS);
int cmd_activate (COMMAND_ARGS) /* "activate", 1, "<input|output|aux|append|source|output-skip>", ""*/
{
  State *o = global_state;
  GeglNode *ref;

  if (o->active == NULL)
  {
    activate_sink_producer (o);
    if (!o->active)
      return -1;
  }
  ref = o->active;

  if (!strcmp (argv[1], "input"))
  {
    ref = gegl_node_get_producer (o->active, "input", NULL);
    if (ref == NULL)
      o->pad_active = 0;
    else
      o->pad_active = 2;
  }
  else if (!strcmp (argv[1], "aux"))
  {
    ref = gegl_node_get_producer (o->active, "aux", NULL);
    if (!ref)
      ref = add_aux (o, o->active, "gegl:nop");
    o->pad_active = 2;
  }
  else if (!strcmp (argv[1], "output"))
  {
    if (o->pad_active != 2)
    {
      o->pad_active = 2;
    }
    else
    {
      ref = gegl_node_get_ui_consumer (o->active, "output", NULL);
      if (ref == o->sink)
        ref = NULL;
      o->pad_active = 2;
    }
  }
  else if (!strcmp (argv[1], "output-skip"))
  {
    GeglNode *iter = o->active;
    int skips = 0;
    if (o->pad_active != 2)
    {
      o->pad_active = 2;
    }

    while (iter)
    {
      const char *consumer_pad = NULL;
      GeglNode *attempt = gegl_node_get_ui_consumer (iter, "output", &consumer_pad);
      if (!strcmp (consumer_pad, "input") && attempt != o->sink)
        {
          iter = attempt;
          skips ++;
        }
      else
        {
          ref = iter;
          iter = NULL;
        }
    }

    if (skips == 0)
    {
      GeglNode *attempt = gegl_node_get_ui_consumer (o->active, "output", NULL);
      if (attempt && attempt != o->sink)
        ref = attempt;
    }
  }
  else if (!strcmp (argv[1], "append"))
  {
    ref = gegl_node_get_producer (o->sink, "input", NULL);
    o->pad_active = 2;
    //activate_sink_producer (o);
  }
  else if (!strcmp (argv[1], "source"))
  {
    ref = o->source;
    o->pad_active = 2;
  }
  else
    ref = NULL;

  if (ref)
    o->active = ref;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

static void set_op (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;

  {
    if (strchr (o->new_opname, ':'))
    {
      gegl_node_set (o->active, "operation", o->new_opname, NULL);
    }
    else
    {
      char temp[1024];
      g_snprintf (temp, 1023, "gegl:%s", o->new_opname);
      gegl_node_set (o->active, "operation", temp, NULL);

    }
  }

  o->new_opname[0]=0;
  o->editing_op_name=0;
  mrg_event_stop_propagate (event);
  mrg_queue_draw (o->mrg, NULL);
}


static void update_ui_consumers_list (State *o, GeglNode *iter)
{
  GList *queue = NULL;
  GeglNode *prev = NULL;

  while (iter)
  {
    queue = g_list_prepend (queue, iter);
    g_hash_table_insert (o->ui_consumer, iter, prev);

    prev = iter;
    iter = gegl_node_get_producer (iter, "input", NULL);

    if (g_hash_table_lookup (o->ui_consumer, iter))
      iter = NULL;
  }

  while (queue)
  {
    GeglNode *aux_producer;
    iter = queue->data;
    aux_producer = gegl_node_get_producer (iter, "aux", NULL);
    if (aux_producer &&
        !g_hash_table_lookup (o->ui_consumer, aux_producer))
      update_ui_consumers_list (o, aux_producer);

    queue = g_list_remove (queue, iter);
  }
}


static void update_ui_consumers (State *o)
{
  g_hash_table_remove_all (o->ui_consumer);

  update_ui_consumers_list (o, o->sink);

}

static GeglNode *gegl_node_get_ui_consumer (GeglNode *node, const char *output_pad, const char **consumer_pad)
{
  GeglNode *ret = g_hash_table_lookup (global_state->ui_consumer, node);
  if (!ret)
  {
    ret = gegl_node_get_consumer_no (node, output_pad, NULL, 0);
  }
  if (consumer_pad)
  {
    GeglNode **nodes = NULL;
    const gchar **consumer_names = NULL;
    int count;
    int i;
    if (node == NULL)
      return NULL;
    count = gegl_node_get_consumers (node, output_pad, &nodes, &consumer_names);
    for (i = 0; i < count; i++)
      if (ret == nodes[i])
        *consumer_pad = consumer_names[i];
    g_free (nodes);
    g_free (consumer_names);
  }

  return ret;
}

typedef struct DrawEdge {
  GeglNode *target;
  int       in_slot_no;  /* 0=input 1=aux */
  int       indent;
  int       line_no;
  GeglNode *source;
} DrawEdge;

GList *edge_queue = NULL; /* queue of edges to be filled in   */


static void
queue_edge (GeglNode *target, int in_slot_no, int indent, int line_no, GeglNode *source)
{
  DrawEdge *edge = g_malloc0 (sizeof (DrawEdge));

  edge->target = target;
  edge->in_slot_no = in_slot_no;
  edge->indent = indent;
  edge->line_no = line_no;
  edge->source = source;

  edge_queue = g_list_prepend (edge_queue, edge);
}


static float compute_node_x (Mrg *mrg, int indent, int line_no)
{
  float em = mrg_em (mrg);
  return (1 + 4 * indent) * em;
}
static float compute_node_y (Mrg *mrg, int indent, int line_no)
{
  float em = mrg_em (mrg);
  return (4 + line_no * 3.5) * em;
}

static float compute_pad_x (Mrg *mrg, int indent, int line_no, int pad_no)
{
  float em = mrg_em (mrg);
  switch (pad_no)
  {
    case 0: // in
      return floor(compute_node_x (mrg, indent, line_no) + em * 3) + .5;
    case 1: // aux
      return floor(compute_node_x (mrg, indent, line_no) + em * 7) + .5;
    case 2: // out
      return floor(compute_node_x (mrg, indent, line_no) + em * 3) + .5;
  }
  return 0;
}

static float compute_pad_y (Mrg *mrg, int indent, int line_no, int pad_no)
{
  float em = mrg_em (mrg);
  switch (pad_no)
  {
    case 0: // in
    case 1: // aux
      return compute_node_y (mrg, indent, line_no) + 2.5 * em;
    case 2: // out
      return compute_node_y (mrg, indent, line_no) + 0.5 * em;
  }
  return 0;
}

static void
draw_node (State *o, int indent, int line_no, GeglNode *node, gboolean active)
{
  char *opname = NULL;
  GList *to_remove = NULL;
  Mrg *mrg = o->mrg;
  float em;
  float x = compute_node_x (mrg, indent, line_no);
  float y = compute_node_y (mrg, indent, line_no);

  /* queue up     */

  if (gegl_node_has_pad (node, "input") &&
      gegl_node_get_producer (node, "input", NULL))
  {
    queue_edge (node, 0, indent, line_no,
                gegl_node_get_producer (node, "input", NULL));
  }

  if (gegl_node_has_pad (node, "aux") &&
      gegl_node_get_producer (node, "aux", NULL))
  {
    queue_edge (node, 1, indent, line_no,
                gegl_node_get_producer (node, "aux", NULL));
  }

  //mrg_set_xy (mrg, x, y);

  g_object_get (node, "operation", &opname, NULL);
  {
    char style[1024];
    sprintf (style, "color:%s;left:%f;top:%f;%s", active?"yellow":"white", x, y, active?"":"border-color:#ccc;");
    mrg_start_with_style (mrg, "div.node", NULL, style);
  }
  em = mrg_em (mrg);

//  if (!active)
//  {
//    mrg_text_listen (mrg, MRG_CLICK, node_press, node, o);
//  }

  if (active && o->editing_op_name)
  {
    mrg_edit_start (mrg, update_string, &o->new_opname[0]);
    mrg_printf (mrg, "%s", o->new_opname);
    mrg_edit_end (mrg);
    mrg_add_binding (mrg, "return", NULL, NULL, set_op, o);
  }
  else
  {
    if (g_str_has_prefix (opname, "gegl:"))
      mrg_printf (mrg, "%s", opname+5);
    else
      mrg_printf (mrg, "%s", opname);
  }

//  if (!active)
//  {
//    mrg_text_listen_done (mrg);
//  }

  if(!active){
    MrgStyle *style = mrg_style (mrg);
    cairo_rectangle (mrg_cr (mrg), style->left, style->top,
                                   style->width + style->padding_left + style->padding_right,
                                   style->height + style->padding_top + style->padding_bottom);

    //cairo_set_source_rgba (mrg_cr (mrg), 0, 1, 0, 1.0);
    //cairo_fill_preserve (mrg_cr (mrg));
    mrg_listen (mrg, MRG_CLICK, node_press, node, o);
  }

  mrg_end (mrg);
  g_free (opname);

  if (gegl_node_has_pad (node, "input"))
  {
      cairo_t *cr = mrg_cr (mrg);
      cairo_new_path (cr);
      cairo_arc (cr, compute_pad_x (mrg, indent, line_no, 0),
                    compute_pad_y (mrg, indent, line_no, 0), 0.3*mrg_em (mrg), 0, G_PI * 2);
      cairo_set_source_rgb (cr, 1.0,1.0,1.0);
      cairo_set_line_width (cr, 1.0f);
      if (active && o->pad_active == 0)
      {
        cairo_fill (cr);
      }
      else
      {
      cairo_new_path (cr);
        cairo_stroke (cr);
      }
  }
  if (gegl_node_has_pad (node, "aux"))
  {
      cairo_t *cr = mrg_cr (mrg);
      cairo_new_path (cr);
      cairo_arc (cr, compute_pad_x (mrg, indent, line_no, 1),
                    compute_pad_y (mrg, indent, line_no, 1), 0.3*mrg_em (mrg), 0, G_PI * 2);
      cairo_set_source_rgb (cr, 1.0,1.0,1.0);
      cairo_set_line_width (cr, 1.0f);

      if (active && o->pad_active == 1)
      {
        cairo_fill (cr);
      }
      else
      {
      cairo_new_path (cr);
        cairo_stroke (cr);
      }
  }
  if (gegl_node_has_pad (node, "output"))
  {
      cairo_t *cr = mrg_cr (mrg);
      cairo_new_path (cr);
      cairo_arc (cr, compute_pad_x (mrg, indent, line_no, 2),
                    compute_pad_y (mrg, indent, line_no, 2), 0.3*mrg_em (mrg), 0, G_PI * 2);
      cairo_set_source_rgb (cr, 1.0,1.0,1.0);
      cairo_set_line_width (cr, 1.0f);
      if (active && o->pad_active == 2)
      {
        cairo_fill (cr);
      }
      else
      {
      cairo_new_path (cr);
        cairo_stroke (cr);
      }
  }


  for (GList *i = edge_queue; i; i = i->next)
  {
    DrawEdge *edge = i->data;
    if (edge->source == node)
    {
      cairo_t *cr = mrg_cr (mrg);
      cairo_new_path (cr);
      cairo_move_to (cr, compute_pad_x (mrg, indent, line_no, 2),
                         compute_pad_y (mrg, indent, line_no, 2));
      cairo_line_to (cr, compute_pad_x (mrg, edge->indent, edge->line_no, edge->in_slot_no),
                         compute_pad_y (mrg, edge->indent, edge->line_no, edge->in_slot_no));
      cairo_set_line_width (cr, 1.75f);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_source_rgba (cr, .0,.0,.0,.5);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, 1.0f);
      cairo_set_source_rgba (cr, 1.0,1.0,1,0.5);
      cairo_stroke (cr);

      to_remove = g_list_prepend (to_remove, edge);
    }
  }
  while (to_remove)
  {
    DrawEdge *edge = to_remove->data;
    edge_queue = g_list_remove (edge_queue, edge);
    to_remove = g_list_remove (to_remove, edge);
  }

}

static void list_ops (State *o, GeglNode *iter, int indent, int *no)
{
  while (iter)
   {
     draw_node (o, indent, *no, iter, iter == o->active);


     (*no)++;


     if (gegl_node_get_producer (iter, "aux", NULL))
     {

     {
       GeglNode *producer = gegl_node_get_producer (iter, "aux", NULL);
       GeglNode *producers_consumer;
       const char *consumer_name = NULL;

       producers_consumer = gegl_node_get_ui_consumer (producer, "output", &consumer_name);

       if (producers_consumer == iter && !strcmp (consumer_name, "aux"))
         {
           list_ops (o, gegl_node_get_producer (iter, "aux", NULL), indent + 1, no);
         }
     }

     }

     {
       GeglNode *producer = gegl_node_get_producer (iter, "input", NULL);
       GeglNode *producers_consumer;

       producers_consumer = gegl_node_get_ui_consumer (producer, "output", NULL);

       if (producers_consumer == iter)
         iter = producer;
        else
        {
          iter = NULL;
        }
     }
   }
}

static void ui_debug_op_chain (State *o)
{
  Mrg *mrg = o->mrg;
  GeglNode *iter;
  int no = 0;

  mrg_start         (mrg, "div.graph", NULL);

  update_ui_consumers (o);

  iter = o->sink;

  /* skip nop-node */
  iter = gegl_node_get_producer (iter, "input", NULL);

  list_ops (o, iter, 0, &no);


  mrg_end (mrg);

  if (o->active)
  {
    //mrg_set_xy (mrg, mrg_width (mrg), 0);
    mrg_start         (mrg, "div.props", NULL);
    //mrg_set_style (mrg, "color:white; background-color: rgba(0,0,0,0.4)");
    list_node_props (o, o->active, 1);
    mrg_end (mrg);
  }
}


static char commandline[1024] = {0,};

static void update_commandline (const char *new_commandline, void *data)
{
  State *o = data;
  strcpy (commandline, new_commandline);
  mrg_queue_draw (o->mrg, NULL);
}

static void
run_command (MrgEvent *event, void *data1, void *data_2)
{
  State *o = global_state; //data1;
  const char *commandline = data1;

  gchar **argv = NULL;
  gint    argc = 0;
  g_shell_parse_argv (commandline, &argc, &argv, NULL);

  /* the commandline has two modes, operation/property mode and argvs command running mode
   * the mode is determined by the first arguement on a passed line if the first word matches
   * an existing argvs command, commandline running mode is used, otherwise operation/property
   * mode is used.
   */

  //printf ("%s\n", commandline);
  if (event)
    mrg_event_stop_propagate (event);

  if (argvs_command_exist (argv[0]))
  {
    argvs_eval (commandline);
  }
  else
  {
    char **arg = argv;

    while (*arg)
    {


  if (strchr (*arg, '='))
  {
    GType target_type = 0;
    GParamSpec *pspec = NULL;
    GParamSpec **pspecs = NULL;
    char *key = g_strdup (*arg);
    char *value = strchr (key, '=') + 1;
    unsigned int n_props = 0;
    value[-1]='\0';
    pspecs = gegl_operation_list_properties (gegl_node_get_operation (o->active), &n_props);
    for (int i = 0; i < n_props; i++)
    {
      if (!strcmp (pspecs[i]->name, key))
      {
        target_type = pspecs[i]->value_type;
        pspec = pspecs[i];
        break;
      }
    }

    if (pspec)
    {
      if (g_type_is_a (target_type, G_TYPE_DOUBLE) ||
          g_type_is_a (target_type, G_TYPE_FLOAT)  ||
          g_type_is_a (target_type, G_TYPE_INT)    ||
          g_type_is_a (target_type, G_TYPE_UINT))
      {
         if (g_type_is_a (target_type, G_TYPE_INT))
           gegl_node_set (o->active, key,
           (int)g_strtod (value, NULL), NULL);
         else if (g_type_is_a (target_type, G_TYPE_UINT))
           gegl_node_set (o->active, key,
           (guint)g_strtod (value, NULL), NULL);
         else
           gegl_node_set (o->active, key,
                          g_strtod (value, NULL), NULL);
      }
      else if (g_type_is_a (target_type, G_TYPE_BOOLEAN))
      {
        if (!strcmp (value, "true") || !strcmp (value, "TRUE") ||
            !strcmp (value, "YES") || !strcmp (value, "yes") ||
            !strcmp (value, "y") || !strcmp (value, "Y") ||
            !strcmp (value, "1") || !strcmp (value, "on"))
        {
          gegl_node_set (o->active, key, TRUE, NULL);
        }
          else
        {
          gegl_node_set (o->active, key, FALSE, NULL);
        }
      }
      else if (target_type == GEGL_TYPE_COLOR)
      {
        GeglColor *color = g_object_new (GEGL_TYPE_COLOR,
                                         "string", value, NULL);
        gegl_node_set (o->active, key, color, NULL);
      }
      else if (target_type == GEGL_TYPE_PATH)
      {
        GeglPath *path = gegl_path_new ();
        gegl_path_parse_string (path, value);
        gegl_node_set (o->active, key, path, NULL);
      }
      else if (target_type == G_TYPE_POINTER &&
               GEGL_IS_PARAM_SPEC_FORMAT (pspec))
      {
        const Babl *format = NULL;

        if (value[0] && babl_format_exists (value))
          format = babl_format (value);
#if 0
        else if (error)
        {
          char *error_str = g_strdup_printf (
                                  _("BablFormat \"%s\" does not exist."),
                                  value);
          *error = g_error_new_literal (g_quark_from_static_string ( "gegl"),
                                        0, error_str);
          g_free (error_str);
        }
#endif

        gegl_node_set (o->active, key, format, NULL);
      }
      else if (g_type_is_a (G_PARAM_SPEC_TYPE (pspec),
               GEGL_TYPE_PARAM_FILE_PATH))
      {
        gchar *buf;
        if (g_path_is_absolute (value))
          {
            gegl_node_set (o->active, key, value, NULL);
          }
        else
          {
            gchar *absolute_path;
#if 0
            if (path_root)
              buf = g_strdup_printf ("%s/%s", path_root, value);
            else
#endif
              buf = g_strdup_printf ("./%s", value);
            absolute_path = realpath (buf, NULL);
            g_free (buf);
            if (absolute_path)
              {
                gegl_node_set (o->active, key, absolute_path, NULL);
                free (absolute_path);
              }
            gegl_node_set (o->active, key, value, NULL);
          }
      }
      else if (g_type_is_a (target_type, G_TYPE_STRING))
      {
        gegl_node_set (o->active, key, value, NULL);
      }
      else if (g_type_is_a (target_type, G_TYPE_ENUM))
      {
        GEnumClass *eclass = g_type_class_peek (target_type);
        GEnumValue *evalue = g_enum_get_value_by_nick (eclass,
                                                       value);
        if (evalue)
          {
            gegl_node_set (o->active, key, evalue->value, NULL);
          }
        else
          {
            /* warn, but try to get a valid nick out of the
               old-style value
             * name
             */
            gchar *nick;
            gchar *c;
            g_printerr (
              "gegl (param_set %s): enum %s has no value '%s'\n",
              key,
              g_type_name (target_type),
              value);
            nick = g_strdup (value);
            for (c = nick; *c; c++)
              {
                *c = g_ascii_tolower (*c);
                if (*c == ' ')
                  *c = '-';
              }
            evalue = g_enum_get_value_by_nick (eclass, nick);
            if (evalue)
              gegl_node_set (o->active, key, evalue->value,
                             NULL);
            g_free (nick);
          }
      }
      else
      {
        GValue gvalue_transformed = {0,};
        GValue gvalue = {0,};
        g_value_init (&gvalue, G_TYPE_STRING);
        g_value_set_string (&gvalue, value);
        g_value_init (&gvalue_transformed, target_type);
        g_value_transform (&gvalue, &gvalue_transformed);
        gegl_node_set_property (o->active, key,
                                &gvalue_transformed);
        g_value_unset (&gvalue);
        g_value_unset (&gvalue_transformed);
      }
    }
    else
    {
       if (!strcmp (key, "op"))
       {
         char temp_op_name[1024];
         if (strchr (*arg, ':'))
         {
           snprintf (temp_op_name, 1023, "%s", value);
         }
         else
         {
           snprintf (temp_op_name, 1023, "gegl:%s", value);
         }
         if (gegl_has_operation (temp_op_name))
           gegl_node_set (o->active, "operation", temp_op_name, NULL);
         else
           printf ("failed to set %s to %s\n", key, value);
       }
       else
       {
         printf ("failed to set %s to %s\n", key, value);
       }
    }
    g_free (key);
  }
  else
  {
    char temp_op_name[1024];
    if (strchr (*arg, ':'))
    {
      snprintf (temp_op_name, 1023, "%s", *arg);
    }
    else
    {
      snprintf (temp_op_name, 1023, "gegl:%s", *arg);
    }
    if (gegl_has_operation (temp_op_name))
    {
        argvs_eval ("node-add output");
        gegl_node_set (o->active, "operation", temp_op_name, NULL);
    }
    else
    {
      printf ("uhandled %s\n", *arg);
    }
    o->editing_op_name=0;
  }
   arg++;
  }
    renderer_dirty++;
    o->rev++;
  }

  g_strfreev (argv);
}

  int cmd_quit (COMMAND_ARGS);
int cmd_quit (COMMAND_ARGS) /* "quit", 0, "", "quit"*/
{
  mrg_quit (global_state->mrg);
  return 0;
}

  int cmd_remove (COMMAND_ARGS);
int cmd_remove (COMMAND_ARGS) /* "remove", 0, "", "removes active node"*/
{
  State *o = global_state;
  GeglNode *node = o->active;
  GeglNode *next, *prev;

  const gchar *consumer_name = NULL;
  prev = gegl_node_get_producer (node, "input", NULL);
  next = gegl_node_get_ui_consumer (node, "output", &consumer_name);

  if (next && prev)
    {
      gegl_node_disconnect (node, "input");
      gegl_node_connect_to (prev, "output", next, consumer_name);
      gegl_node_remove_child (o->gegl, node);
      o->active = prev;
    }

  mrg_queue_draw (o->mrg, NULL);
  renderer_dirty++;
  o->rev++;
  return 0;
}

int cmd_swap (COMMAND_ARGS);/* "swap", 1, "<input|output>", "swaps position with other node, allows doing the equivalent of raise lower and other local reordering of nodes."*/

int
cmd_swap (COMMAND_ARGS)
{
  State *o = global_state;
  GeglNode *node = o->active;
  GeglNode *next, *prev;

  next = gegl_node_get_ui_consumer (node, "output", NULL);
  prev = gegl_node_get_producer (node, "input", NULL);

  if (next && prev)
    {

      if (!strcmp (argv[1], "output") && next != o->sink)
      {
        GeglNode *next_next = gegl_node_get_ui_consumer (next, "output", NULL);

        gegl_node_link_many (prev, next, node, next_next, NULL);
      }
      else if (!strcmp (argv[1], "input") && prev != o->source)
      {
        GeglNode *prev_prev = gegl_node_get_producer (prev, "input", NULL);

        gegl_node_link_many (prev_prev, node, prev, next, NULL);
      }

    }

  mrg_queue_draw (o->mrg, NULL);
  renderer_dirty++;
  o->rev++;
  return 0;
}





  int cmd_move (COMMAND_ARGS);
int cmd_move (COMMAND_ARGS) /* "move", 0, "", "changes to move tool"*/
{
  tool = TOOL_MOVE;
  return 0;
}
  int cmd_paint (COMMAND_ARGS);
int cmd_paint (COMMAND_ARGS) /* "paint", 0, "", "changes to paint tool"*/
{
  tool = TOOL_PAINT;
  return 0;
}
  int cmd_pick (COMMAND_ARGS);
int cmd_pick (COMMAND_ARGS) /* "pick", 0, "", "changes to pick tool"*/
{
  tool = TOOL_PICK;
  return 0;
}
  int cmd_tpan (COMMAND_ARGS);
int cmd_tpan (COMMAND_ARGS) /* "tpan", 0, "", "changes to pan tool"*/
{
  tool = TOOL_PAN;
  return 0;
}

static void commandline_run (MrgEvent *event, void *data1, void *data2)
{
  State *o = data1;
  if (commandline[0])
    run_command (event, commandline, NULL);
  else
    {
      if (o->is_dir)
      {
        if (o->entry_no == -1)
        {
          go_parent (o);
        }
        else
        {
          g_free (o->path);
          o->path = g_strdup (g_list_nth_data (o->paths, o->entry_no));
          load_path (o);

        }
      }
      else
      {
        o->show_graph = !o->show_graph;
      }
    }

  commandline[0]=0;
  mrg_set_cursor_pos (event->mrg, 0);
  mrg_queue_draw (o->mrg, NULL);

  mrg_event_stop_propagate (event);
}

static void iterate_frame (State *o)
{
  Mrg *mrg = o->mrg;

  if (g_str_has_suffix (o->src_path, ".gif") ||
      g_str_has_suffix (o->src_path, ".GIF"))
   {
     int frames = 0;
     int frame_delay = 0;
     gegl_node_get (o->source, "frames", &frames, "frame-delay", &frame_delay, NULL);
     if (o->prev_ms + frame_delay  < mrg_ms (mrg))
     {
       o->frame_no++;
       fprintf (stderr, "\r%i/%i", o->frame_no, frames);   /* */
       if (o->frame_no >= frames)
         o->frame_no = 0;
       gegl_node_set (o->source, "frame", o->frame_no, NULL);
       o->prev_ms = mrg_ms (mrg);
       renderer_dirty++;
    }
    mrg_queue_draw (o->mrg, NULL);
   }
  else if (o->is_video)
   {
     int frames = 0;
     o->frame_no++;
     gegl_node_get (o->source, "frames", &frames, NULL);
     fprintf (stderr, "\r%i/%i", o->frame_no, frames);   /* */
     if (o->frame_no >= frames)
       o->frame_no = 0;
     gegl_node_set (o->source, "frame", o->frame_no, NULL);
     renderer_dirty++;
     mrg_queue_draw (o->mrg, NULL);
    {
      GeglAudioFragment *audio = NULL;
      gdouble fps;
      /* XXX:
           this currently goes wrong with threaded rendering, since we miss audio frames
           from the renderer thread, moving this to the render thread would solve that.
       */
      gegl_node_get (o->source, "audio", &audio, "frame-rate", &fps, NULL);
      if (audio)
      {
       int sample_count = gegl_audio_fragment_get_sample_count (audio);
       if (sample_count > 0)
       {
         int i;
         if (!audio_started)
         {
           open_audio (mrg, gegl_audio_fragment_get_sample_rate (audio));
           audio_started = 1;
         }
         {
         uint16_t temp_buf[sample_count * 2];
         for (i = 0; i < sample_count; i++)
         {
           temp_buf[i*2] = audio->data[0][i] * 32767.0 * 0.46;
           temp_buf[i*2+1] = audio->data[1][i] * 32767.0 * 0.46;
         }

         mrg_pcm_queue (mrg, (void*)&temp_buf[0], sample_count);
         while (mrg_pcm_get_queued (mrg) > sample_count / 2)
            g_usleep (50);
         }


         o->prev_frame_played = o->frame_no;
         deferred_redraw (mrg, NULL);
       }
       g_object_unref (audio);
      }
    }
  }
}


static void ui_show_bindings (Mrg *mrg, void *data)
{
  float em = mrg_em (mrg);
  float h = mrg_height (mrg);
  float x = em;
  int col = 0;
  MrgBinding *bindings = mrg_get_bindings (mrg);

  mrg_start (mrg, "dl.bindings", NULL);
  mrg_set_xy (mrg, x, em * 2);//h/2+em *2);

  for (int i = 0; bindings[i].cb; i++)
  {
    MrgBinding *b = &bindings[i];

    int redefined = 0;
    for (int j = i+1; bindings[j].cb; j++)
      if (!strcmp (bindings[j].nick, bindings[i].nick))
        redefined ++;
    if (redefined)
      continue;  /* we only print the last registered, and handled registration */

    mrg_start (mrg, "dt.binding", NULL);mrg_printf(mrg,"%s", b->nick);mrg_end (mrg);
#if 0
    if (b->command)
    {
      mrg_start (mrg, "dd.binding", NULL);mrg_printf(mrg,"%s", b->command);mrg_end (mrg);
    }
#endif
    if (b->cb == run_command)
    {
      mrg_start (mrg, "dd.binding", NULL);mrg_printf(mrg,"%s", b->cb_data);mrg_end (mrg);
    }
    if (b->label)
    {
      mrg_start (mrg, "dd.binding", NULL);mrg_printf(mrg,"%s", b->label);mrg_end (mrg);
    }

    if (mrg_y (mrg) > h/2 - em * 4)
    {
      col++;
      mrg_set_edge_left (mrg, col * (20 * mrg_em(mrg)));
      mrg_set_xy (mrg, col * (15 * em), em * 2);
    }

  }
  mrg_end (mrg);
}

static void ui_commandline (Mrg *mrg, void *data)
{
  State *o = data;
  float em = mrg_em (mrg);
  float h = mrg_height (mrg);
  //float w = mrg_width (mrg);
  cairo_t *cr = mrg_cr (mrg);
  int row = 1;
  cairo_save (cr);
  if (scrollback)
  mrg_start (mrg, "div.shell", NULL);
  mrg_set_xy (mrg, em, h - em * 1);
  mrg_start (mrg, "div.prompt", NULL);
  mrg_printf (mrg, "> ");
  mrg_end (mrg);
  mrg_start (mrg, "div.commandline", NULL);
    mrg_edit_start (mrg, update_commandline, o);
    mrg_printf (mrg, "%s", commandline);
    mrg_edit_end (mrg);
    mrg_end (mrg);
  mrg_edit_end (mrg);
  row++;

  mrg_set_xy (mrg, em, h * 0.5);

  {
    MrgList *lines = NULL;
    for (MrgList *l = scrollback; l; l = l->next)
      mrg_list_prepend (&lines, l->data);

    for (MrgList *l = lines; l; l = l->next)
    {
      mrg_start (mrg, "div.shellline", NULL);
      mrg_printf (mrg, "%s", l->data);
      mrg_end (mrg);
    }
    {
      if (mrg_y (mrg) > h - em * 1.2 * 1)
      {
        char *data;
        mrg_list_reverse (&scrollback);
        data = scrollback->data;
        mrg_list_remove (&scrollback, data);
        mrg_list_reverse (&scrollback);
        free (data);
        mrg_queue_draw (mrg, NULL);
      }
    }
    mrg_list_free (&lines);

  }
  if (scrollback)
  mrg_end (mrg);

  mrg_add_binding (mrg, "return", NULL, NULL, commandline_run, o);
  cairo_restore (cr);
}


static void gegl_ui (Mrg *mrg, void *data)
{
  State *o = data;
  struct stat stat_buf;

  mrg_stylesheet_add (mrg, css, NULL, 0, NULL);

  lstat (o->path, &stat_buf);
  if (S_ISDIR (stat_buf.st_mode))
  {
    o->is_dir = 1;
  }
  else
  {
    o->is_dir = 0;
  }

  if (o->is_dir)
  {
    cairo_set_source_rgb (mrg_cr (mrg), .0,.0,.0);
    cairo_paint (mrg_cr (mrg));
  }
  else
  switch (renderer)
  {
     case GEGL_RENDERER_BLIT:
     case GEGL_RENDERER_BLIT_MIPMAP:
       mrg_gegl_blit (mrg,
                      0, 0,
                      mrg_width (mrg), mrg_height (mrg),
                      o->sink,
                      o->u, o->v,
                      o->scale,
                      o->render_quality,
                      o->color_manage_display);
     break;
     case GEGL_RENDERER_THREAD:
     case GEGL_RENDERER_IDLE:
       if (o->processor_buffer)
       {
         GeglBuffer *buffer = g_object_ref (o->processor_buffer);
         mrg_gegl_buffer_blit (mrg,
                               0, 0,
                               mrg_width (mrg), mrg_height (mrg),
                               buffer,
                               o->u, o->v,
                               o->scale,
                               o->render_quality,
                               o->color_manage_display);
         g_object_unref (buffer);
       }
       break;
  }

  if (o->playing)
  {
    iterate_frame (o);
  }

  if (o->show_controls && 0)
  {
    mrg_printf (mrg, "%s\n", o->path);
  }


  if (o->is_dir)
    dir_touch_handling (mrg, o);
  else
    canvas_touch_handling (mrg, o);

  cairo_save (mrg_cr (mrg));
  {
    if (edited_prop)
      g_free (edited_prop);
    edited_prop = NULL;

    if (S_ISREG (stat_buf.st_mode))
    {
      if (o->show_graph)
        {
          ui_debug_op_chain (o);
          mrg_add_binding (mrg, "escape", NULL, NULL, run_command, "toggle editing");
        }
      else
        {
          ui_viewer (o);
          mrg_add_binding (mrg, "escape", NULL, NULL, run_command, "parent");
        }

      mrg_add_binding (mrg, "page-down", NULL, NULL, run_command, "next");
      mrg_add_binding (mrg, "alt-right", NULL, NULL, run_command, "next");
      mrg_add_binding (mrg, "page-up", NULL, NULL,  run_command, "prev");
      mrg_add_binding (mrg, "alt-left", NULL, NULL,  run_command, "prev");


    }
    else if (S_ISDIR (stat_buf.st_mode))
    {
      ui_dir_viewer (o);
      mrg_add_binding (mrg, "alt-right", NULL, NULL, run_command, "collection right");
      mrg_add_binding (mrg, "alt-left", NULL, NULL,  run_command, "collection left");
      mrg_add_binding (mrg, "escape", NULL, NULL, run_command, "parent");
    }
  }
  cairo_restore (mrg_cr (mrg));
  cairo_new_path (mrg_cr (mrg));


  mrg_add_binding (mrg, "control-q", NULL, NULL, run_command, "quit");
  mrg_add_binding (mrg, "F11", NULL, NULL,       run_command, "toggle fullscreen");

  if (!edited_prop && !o->editing_op_name && ! o->is_dir)
  {
#if 0
    if (o->active && gegl_node_has_pad (o->active, "output"))
      mrg_add_binding (mrg, "control-o", NULL, NULL, run_command, "node-add output");
    if (o->active && gegl_node_has_pad (o->active, "input"))
      mrg_add_binding (mrg, "control-i", NULL, NULL, run_command, "node-add input");
    if (o->active && gegl_node_has_pad (o->active, "aux"))
      mrg_add_binding (mrg, "control-a", NULL, NULL, run_command, "node-add aux");
#endif

    if (o->active != o->source)
      mrg_add_binding (mrg, "control-x", NULL, NULL, run_command, "remove");

    mrg_add_binding (mrg, "control-s", NULL, NULL, run_command, "toggle slideshow");
  }
  mrg_add_binding (mrg, "control-l", NULL, NULL, run_command, "clear");

  if (!edited_prop && !o->editing_op_name)
  {
    mrg_add_binding (mrg, "tab", NULL, NULL, run_command, "toggle controls");
    mrg_add_binding (mrg, "control-f", NULL, NULL,  run_command, "toggle fullscreen");

    if (commandline[0]==0)
    {
      mrg_add_binding (mrg, "+", NULL, NULL, run_command, "zoom in");
      mrg_add_binding (mrg, "=", NULL, NULL, run_command, "zoom in");
      mrg_add_binding (mrg, "-", NULL, NULL, run_command, "zoom out");
      mrg_add_binding (mrg, "1", NULL, NULL, run_command, "zoom 1.0");
    }
  }

  if (!edited_prop && !o->editing_op_name)
  {
    ui_commandline (mrg, o);
  }
  if (commandline[0]==0)
  {
    /* cursor keys and some more keys are used for commandline entry if there already
       is contents, this frees up some bindings/individual keys for direct binding as
       they would not start a valid command anyways. */

    if (o->is_dir)
    {
      mrg_add_binding (mrg, "left", NULL, NULL, run_command, "collection left");
      mrg_add_binding (mrg, "right", NULL, NULL, run_command, "collection right");
      mrg_add_binding (mrg, "up", NULL, NULL, run_command, "collection up");
      mrg_add_binding (mrg, "down", NULL, NULL, run_command, "collection down");

      mrg_add_binding (mrg, "home", NULL, NULL, run_command, "collection first");
      mrg_add_binding (mrg, "end", NULL, NULL, run_command, "collection last");

      mrg_add_binding (mrg, "space", NULL, NULL,   run_command, "collection right");
      mrg_add_binding (mrg, "backspace", NULL, NULL,  run_command, "collection left");

    }
    else
    {
      mrg_add_binding (mrg, "home",     NULL, NULL, run_command, "activate append");
      mrg_add_binding (mrg, "end",      NULL, NULL, run_command, "activate source");

      if (o->active && gegl_node_has_pad (o->active, "output"))
        mrg_add_binding (mrg, "left", NULL, NULL,        run_command, "activate output-skip");
      if (o->active && gegl_node_has_pad (o->active, "aux"))
        mrg_add_binding (mrg, "right", NULL, NULL, run_command, "activate aux");
      mrg_add_binding (mrg, "space", NULL, NULL,   run_command, "next");
      //mrg_add_binding (mrg, "backspace", NULL, NULL,  run_command, "prev");
    }
#if 0
    mrg_add_binding (mrg, "1", NULL, NULL, run_command, "star 1");
    mrg_add_binding (mrg, "2", NULL, NULL, run_command, "star 2");
    mrg_add_binding (mrg, "3", NULL, NULL, run_command, "star 3");
    mrg_add_binding (mrg, "4", NULL, NULL, run_command, "star 4");
    mrg_add_binding (mrg, "5", NULL, NULL, run_command, "star 5");
#endif
  }
  if (o->is_dir)
  {
  }
  else
  {
    mrg_add_binding (mrg, "control-t", NULL, NULL, run_command, "zoom fit");
    mrg_add_binding (mrg, "control-m", NULL, NULL, run_command, "toggle mipmap");
    mrg_add_binding (mrg, "control-y", NULL, NULL, run_command, "toggle colormanage-display");


    if (o->active && gegl_node_has_pad (o->active, "output"))
      mrg_add_binding (mrg, "up", NULL, NULL,        run_command, "activate output");
    if (o->active && gegl_node_has_pad (o->active, "input"))
      mrg_add_binding (mrg, "down", NULL, NULL,      run_command, "activate input");

    if (o->active && gegl_node_has_pad (o->active, "input") &&
                     gegl_node_has_pad (o->active, "output"))
    {
      mrg_add_binding (mrg, "control-up", NULL, NULL, run_command, "swap output");
      mrg_add_binding (mrg, "control-down", NULL, NULL, run_command, "swap input");
    }


  }

  mrg_add_binding (mrg, "F1", NULL, NULL, run_command, "toggle cheatsheet");
  mrg_add_binding (mrg, "control-h", NULL, NULL, run_command, "toggle cheatsheet");

  mrg_add_binding (mrg, "control-delete", NULL, NULL,  run_command, "discard");

  if (o->show_bindings)
  {
     ui_show_bindings (mrg, o);
  }

}

/***********************************************/

static char *get_path_parent (const char *path)
{
  char *ret = g_strdup (path);
  char *lastslash = strrchr (ret, '/');
  if (lastslash)
  {
    if (lastslash == ret)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';
  }
  return ret;
}

static char *suffix_path (const char *path)
{
  char *ret;
  if (!path)
    return NULL;
  ret  = g_malloc (strlen (path) + strlen (suffix) + 3);
  strcpy (ret, path);
  sprintf (ret, "%s%s", path, ".gegl");
  return ret;
}

static char *unsuffix_path (const char *path)
{
  char *ret = NULL, *last_dot;

  if (!path)
    return NULL;
  ret = g_malloc (strlen (path) + 4);
  strcpy (ret, path);
  last_dot = strrchr (ret, '.');
  *last_dot = '\0';
  return ret;
}

static int is_gegl_path (const char *path)
{
  int ret = 0;
  if (g_str_has_suffix (path, ".gegl"))
  {
    char *unsuffixed = unsuffix_path (path);
    if (access (unsuffixed, F_OK) != -1)
      ret = 1;
    g_free (unsuffixed);
  }
  return ret;
}

static void contrasty_stroke (cairo_t *cr)
{
  double x0 = 6.0, y0 = 6.0;
  double x1 = 4.0, y1 = 4.0;

  cairo_device_to_user_distance (cr, &x0, &y0);
  cairo_device_to_user_distance (cr, &x1, &y1);
  cairo_set_source_rgba (cr, 0,0,0,0.5);
  cairo_set_line_width (cr, y0);
  cairo_stroke_preserve (cr);
  cairo_set_source_rgba (cr, 1,1,1,0.5);
  cairo_set_line_width (cr, y1);
  cairo_stroke (cr);
}


static void load_path_inner (State *o,
                             char *path)
{
  char *meta;
  if (o->src_path)
    g_free (o->src_path);

  if (is_gegl_path (path))
  {
    if (o->save_path)
      g_free (o->save_path);
    o->save_path = path;
    path = unsuffix_path (o->save_path); // or maybe decode first line?
    o->src_path = g_strdup (path);
  }
  else
  {
    if (o->save_path)
      g_free (o->save_path);
    if (g_str_has_suffix (path, ".gegl"))
    {
      //fprintf (stderr, "oooo\n");
      o->save_path = g_strdup (path);
    }
    else
    {
      o->save_path = suffix_path (path);
      o->src_path = g_strdup (path);
    }
  }

  if (access (o->save_path, F_OK) != -1)
  {
    /* XXX : fix this in the fuse layer of zn! XXX XXX XXX XXX */
    if (!strstr (o->save_path, ".zn.fs"))
      path = o->save_path;
  }

  g_object_unref (o->gegl);
  o->gegl = NULL;
  o->sink = NULL;
  o->source = NULL;
  if (o->dir_scale <= 0.001)
    o->dir_scale = 1.0;
  o->rev = 0;
  o->is_video = 0;
  o->frame_no = -1;
  o->prev_frame_played = 0;

  if (g_str_has_suffix (path, ".gif"))
  {
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:gif-load", "path", path, "frame", o->frame_no, NULL);
    o->playing = 1;
    gegl_node_link_many (o->source, o->sink, NULL);
  }
  else if (gegl_str_has_video_suffix (path))
  {
    o->is_video = 1;
    o->playing = 1;
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:ff-load", "path", path, "frame", o->frame_no, NULL);
    gegl_node_link_many (o->source, o->sink, NULL);
  }
  else
  {
    meta = NULL;
    if (is_gegl_path (path) || g_str_has_suffix (path, ".gegl"))
      g_file_get_contents (path, &meta, NULL, NULL);
    if (meta)
    {
      GeglNode *iter;
      GeglNode *prev = NULL;
      char *containing_path = get_path_parent (path);

      o->gegl = gegl_node_new_from_serialized (meta, containing_path);
      g_free (containing_path);
      o->sink = o->gegl;
      o->source = NULL;

      for (iter = o->sink; iter; iter = gegl_node_get_producer (iter, "input", NULL))
      {
        const char *op_name = gegl_node_get_operation (iter);
        if (!strcmp (op_name, "gegl:load"))
        {
          gchar *path;
          gegl_node_get (iter, "path", &path, NULL);

          if (g_str_has_suffix (path, ".gif"))
          {
             o->source = gegl_node_new_child (o->gegl,
             "operation", "gegl:gif-load", "path", path, "frame", o->frame_no, NULL);
             o->playing = 1;
             gegl_node_link_many (o->source, prev, NULL);
          }
          else
          {
            load_into_buffer (o, path);
             o->source = gegl_node_new_child (o->gegl, "operation", "gegl:buffer-source",
                                             "buffer", o->buffer, NULL);

            gegl_node_link_many (o->source, prev, NULL);
          }
          if (o->src_path)
            g_free (o->src_path);
          o->src_path = g_strdup (path);

          o->save = gegl_node_new_child (o->gegl, "operation", "gegl:save",
                                              "path", o->save_path,
                                              NULL);
          g_free (path);
          break;
        }
        prev = iter;
      }

    }
    else
    {
      o->gegl = gegl_node_new ();
      o->sink = gegl_node_new_child (o->gegl,
                         "operation", "gegl:nop", NULL);
      load_into_buffer (o, path);
      if (o->src_path)
        g_free (o->src_path);
      o->src_path = g_strdup (path);
      o->source = gegl_node_new_child (o->gegl,
                                     "operation", "gegl:buffer-source",
                                     NULL);
      o->save = gegl_node_new_child (o->gegl,
                                     "operation", "gegl:save",
                                     "path", o->save_path,
                                     NULL);
    gegl_node_link_many (o->source, o->sink, NULL);
    gegl_node_set (o->source, "buffer", o->buffer, NULL);
  }
  }

  if (o->ops)
  {
    GeglNode *ret_sink = NULL;
    GError *error = (void*)(&ret_sink);

    char *containing_path = get_path_parent (path);
    gegl_create_chain_argv (o->ops,
                    gegl_node_get_producer (o->sink, "input", NULL),
                    o->sink, 0, gegl_node_get_bounding_box (o->sink).height,
                    containing_path,
                    &error);
    g_free (containing_path);
    if (error)
    {
      fprintf (stderr, "Error: %s\n", error->message);
    }
    if (ret_sink)
    {
      gegl_node_process (ret_sink);
      exit(0);
    }
  }

  activate_sink_producer (o);

  if (o->processor)
      g_object_unref (o->processor);
  o->processor = gegl_node_new_processor (o->sink, NULL);
  renderer_dirty++;

}


static void load_path (State *o)
{
  while (thumb_queue)
  {
    ThumbQueueItem *item = thumb_queue->data;
    mrg_list_remove (&thumb_queue, item);
    thumb_queue_item_free (item);
  }

  populate_path_list (o);
  o->playing = 0;

  load_path_inner (o, o->path);

  {
    struct stat stat_buf;
    lstat (o->path, &stat_buf);
    if (S_ISREG (stat_buf.st_mode))
    {
      if (o->is_video)
        center (o);
      else
        zoom_to_fit (o);
    }
  }

  o->entry_no = -1;
  o->scale = 1.0;
  o->u = 0;
  o->v = 0;

  zoom_to_fit (o);
  mrg_queue_draw (o->mrg, NULL);
}

static void go_parent (State *o)
{
  char *prev_path = g_strdup (o->path);
  char *lastslash = strrchr (o->path, '/');
    int entry_no = 0;

  if (lastslash)
  {
    if (lastslash == o->path)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';

    load_path (o);

    {
      int no = 0;
      for (GList *i = o->paths; i; i=i->next, no++)
      {
        if (!strcmp (i->data, prev_path))
        {
          entry_no = no;
          break;
        }
      }
    }

    if (entry_no)
    {
      o->entry_no = entry_no;

      center_active_entry (o);
    }
    mrg_queue_draw (o->mrg, NULL);
  }
  g_free (prev_path);
}

static void go_next (State *o)
{
  GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);

  if (curr && curr->next)
  {
    g_free (o->path);
    o->path = g_strdup (curr->next->data);
    load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }
}

static void go_prev (State *o)
{
  GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);

  if (curr && curr->prev)
  {
    g_free (o->path);
    o->path = g_strdup (curr->prev->data);
    load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }
}

int cmd_clear (COMMAND_ARGS); /* "clear", 0, "", "clears the scrollback and triggers as rerender"*/
int
cmd_clear (COMMAND_ARGS)
{
  while (scrollback)
  {
    char *data = scrollback->data;
    mrg_list_remove (&scrollback, data);
    free (data);
  }
  populate_path_list (global_state);
  renderer_dirty ++; // also force a rerender as a side effect, sometimes useful
  mrg_queue_draw (global_state->mrg, NULL);
  return 0;
}

 int cmd_next (COMMAND_ARGS);
int cmd_next (COMMAND_ARGS) /* "next", 0, "", "next sibling element in current collection/folder"*/
{
  State *o = global_state;
  if (o->rev)
    argvs_eval ("save");
  go_next (o);

  activate_sink_producer (o);

  return 0;
}

 int cmd_parent (COMMAND_ARGS);
int cmd_parent (COMMAND_ARGS) /* "parent", 0, "", "enter parent collection (switches to folder mode)"*/
{
  State *o = global_state;
  if (o->rev)
    argvs_eval ("save");
  go_parent (o);
  o->active = NULL;
  return 0;
}

 int cmd_prev (COMMAND_ARGS);
int cmd_prev (COMMAND_ARGS) /* "prev", 0, "", "previous sibling element in current collection/folder"*/
{
  State *o = global_state;
  if (o->rev)
    argvs_eval ("save");
  go_prev (o);
  activate_sink_producer (o);
  return 0;
}

 int cmd_load (COMMAND_ARGS);
int cmd_load (COMMAND_ARGS) /* "load", 1, "<path>", "load a path/image - can be relative to current pereived folder "*/
{
  State *o = global_state;
  
  if (o->path)
    g_free (o->path);
  o->path = g_strdup (argv[1]);

  load_path (o);

  activate_sink_producer (o);
  return 0;
}


static void drag_preview (MrgEvent *e)
{
  State *o = global_state;
  static float old_factor = 1;
  switch (e->type)
  {
    case MRG_DRAG_PRESS:
      old_factor = o->render_quality;
      if (o->render_quality < o->preview_quality)
        o->render_quality = o->preview_quality;
      break;
    case MRG_DRAG_RELEASE:
      o->render_quality = old_factor;
      mrg_queue_draw (e->mrg, NULL);
      break;
    default:
    break;
  }
}

static void load_into_buffer (State *o, const char *path)
{
  GeglNode *gegl, *load, *sink;
  struct stat stat_buf;

  if (o->buffer)
  {
    g_object_unref (o->buffer);
    o->buffer = NULL;
  }

  lstat (path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {


  gegl = gegl_node_new ();
  load = gegl_node_new_child (gegl, "operation", "gegl:load",
                                    "path", path,
                                    NULL);
  sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink",
                                    "buffer", &(o->buffer),
                                    NULL);
  gegl_node_link_many (load, sink, NULL);
  gegl_node_process (sink);
  g_object_unref (gegl);

  {
    GExiv2Orientation orientation = path_get_orientation (path);
    gboolean hflip = FALSE;
    gboolean vflip = FALSE;
    double degrees = 0.0;
    switch (orientation)
    {
      case GEXIV2_ORIENTATION_UNSPECIFIED:
      case GEXIV2_ORIENTATION_NORMAL:
        break;
      case GEXIV2_ORIENTATION_HFLIP: hflip=TRUE; break;
      case GEXIV2_ORIENTATION_VFLIP: vflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_90: degrees = 90.0; break;
      case GEXIV2_ORIENTATION_ROT_90_HFLIP: degrees = 90.0; hflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_90_VFLIP: degrees = 90.0; vflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_180: degrees = 180.0; break;
      case GEXIV2_ORIENTATION_ROT_270: degrees = 270.0; break;
    }

    if (degrees != 0.0 || vflip || hflip)
     {
       /* XXX: deal with vflip/hflip */
       GeglBuffer *new_buffer = NULL;
       GeglNode *rotate;
       gegl = gegl_node_new ();
       load = gegl_node_new_child (gegl, "operation", "gegl:buffer-source",
                                   "buffer", o->buffer,
                                   NULL);
       sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink",
                                   "buffer", &(new_buffer),
                                   NULL);
       rotate = gegl_node_new_child (gegl, "operation", "gegl:rotate",
                                   "degrees", -degrees,
                                   "sampler", GEGL_SAMPLER_NEAREST,
                                   NULL);
       gegl_node_link_many (load, rotate, sink, NULL);
       gegl_node_process (sink);
       g_object_unref (gegl);
       g_object_unref (o->buffer);
       o->buffer = new_buffer;
     }
  }

#if 0 /* hack to see if having the data in some formats already is faster */
  {
  GeglBuffer *tempbuf;
  tempbuf = gegl_buffer_new (gegl_buffer_get_extent (o->buffer),
                                         babl_format ("RGBA float"));

  gegl_buffer_copy (o->buffer, NULL, GEGL_ABYSS_NONE, tempbuf, NULL);
  g_object_unref (o->buffer);
  o->buffer = tempbuf;
  }
#endif
    }
  else
    {
      GeglRectangle extent = {0,0,1,1}; /* segfaults with NULL / 0,0,0,0*/
      o->buffer = gegl_buffer_new (&extent, babl_format("RGBA float"));
    }
}

#if 0
static GeglNode *locate_node (State *o, const char *op_name)
{
  GeglNode *iter = o->sink;
  while (iter)
   {
     char *opname = NULL;
     g_object_get (iter, "operation", &opname, NULL);
     if (!strcmp (opname, op_name))
       return iter;
     g_free (opname);
     iter = gegl_node_get_producer (iter, "input", NULL);
   }
  return NULL;
}
#endif

static void zoom_to_fit (State *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  float scale, scale2;

  if (rect.width == 0 || rect.height == 0)
  {
    o->scale = 1.0;
    o->u = 0.0;
    o->v = 0.0;
    return;
  }

  scale = 1.0 * mrg_width (mrg) / rect.width;
  scale2 = 1.0 * mrg_height (mrg) / rect.height;

  if (scale2 < scale) scale = scale2;

  o->scale = scale;

  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;

  mrg_queue_draw (mrg, NULL);
}
static void center (State *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  o->scale = 1.0;

  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;

  mrg_queue_draw (mrg, NULL);
}

static void zoom_at (State *o, float screen_cx, float screen_cy, float factor)
{
  float x, y;
  get_coords (o, screen_cx, screen_cy, &x, &y);
  o->scale *= factor;
  o->u = x * o->scale - screen_cx;
  o->v = y * o->scale - screen_cy;

  o->renderer_state = 0;
  mrg_queue_draw (o->mrg, NULL);
}

  int cmd_pan (COMMAND_ARGS);
int cmd_pan (COMMAND_ARGS) /* "pan", 2, "<rel-x> <rel-y>", "pans viewport"*/
{
  State *o = global_state;
  float amount_u = mrg_width (o->mrg)  * g_strtod (argv[1], NULL);
  float amount_v = mrg_height (o->mrg) * g_strtod (argv[2], NULL);
  o->u += amount_u;
  o->v += amount_v;
  return 0;
}


int cmd_collection (COMMAND_ARGS); /* "collection", -1, "<up|left|right|down|first|last>", ""*/
  int cmd_collection (COMMAND_ARGS)
{
  State *o = global_state;

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
    o->entry_no = g_list_length (o->paths)-1;
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

  if (o->entry_no >= (int)g_list_length (o->paths))
    o->entry_no = g_list_length (o->paths)-1;


  center_active_entry (o);

  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

  int cmd_cd (COMMAND_ARGS);
int cmd_cd (COMMAND_ARGS) /* "cd", 1, "<target>", "convenience wrapper making some common commandline navigation commands work"*/
{
  State *o = global_state;
  if (!strcmp (argv[1], ".."))
  {
    argvs_eval ("parent");
  }
  else if (argv[1][0] == '/')
  {
    if (o->path)
      g_free (o->path);
    o->path = g_strdup (argv[1]);
    if (o->path[strlen(o->path)-1]=='/')
      o->path[strlen(o->path)-1]='\0';
    load_path (o);
  }
  else
  {
    char *new_path = g_strdup_printf ("%s/%s", o->path, argv[1]);
    char *rp = realpath (new_path, NULL);

    if (access (rp, F_OK) == 0)
    {
      if (o->path)
        g_free (o->path);
      o->path = g_strdup (rp);
      if (o->path[strlen(o->path)-1]=='/')
        o->path[strlen(o->path)-1]='\0';
      load_path (o);
    }
    free (rp);
    g_free (new_path);
  }
  return 0;
}


  int cmd_zoom (COMMAND_ARGS);
int cmd_zoom (COMMAND_ARGS) /* "zoom", -1, "<fit|in [amt]|out [amt]|zoom-level>", "Changes zoom level, asbolsute or relative, around middle of screen."*/
{
  State *o = global_state;

  if (!argv[1])
  {
    printf ("current scale factor: %2.3f\n", o->is_dir?o->dir_scale:o->scale);
    return 0;
  }

  if (o->is_dir)
  {
     if (!strcmp(argv[1], "in"))
     {
       float zoom_factor = 0.25;
       if (argv[2])
         zoom_factor = g_strtod (argv[2], NULL);
       zoom_factor += 1.0;
       o->dir_scale *= zoom_factor;
      }
      else if (!strcmp(argv[1], "out"))
      {
        float zoom_factor = 0.25;
        if (argv[2])
          zoom_factor = g_strtod (argv[2], NULL);
        zoom_factor += 1.0;
        o->dir_scale /= zoom_factor;
      }
      else
      {
        o->dir_scale = g_strtod(argv[1], NULL);
        if (o->dir_scale < 0.0001 || o->dir_scale > 200.0)
          o->dir_scale = 1;
      }

      if (o->dir_scale > 2.2) o->dir_scale = 2.2;
      if (o->dir_scale < 0.1) o->dir_scale = 0.1;

      center_active_entry (o);

      mrg_queue_draw (o->mrg, NULL);
      return 0;
  }

  if (!strcmp(argv[1], "fit"))
  {
    zoom_to_fit (o);
  }
  else if (!strcmp(argv[1], "in"))
  {
    float zoom_factor = 0.1;
    if (argv[2])
      zoom_factor = g_strtod (argv[2], NULL);
    zoom_factor += 1.0;

    zoom_at (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, zoom_factor);
  }
  else if (!strcmp(argv[1], "out"))
  {
    float zoom_factor = 0.1;
    if (argv[2])
      zoom_factor = g_strtod (argv[2], NULL);
    zoom_factor += 1.0;

    zoom_at (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, 1.0f/zoom_factor);
  }
  else
  {
    float x, y;
    get_coords (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, &x, &y);
    o->scale = g_strtod(argv[1], NULL);
    o->u = x * o->scale - mrg_width(o->mrg)/2;
    o->v = y * o->scale - mrg_height(o->mrg)/2;
    mrg_queue_draw (o->mrg, NULL);
  }
  return 0;
}

static int deferred_zoom_to_fit (Mrg *mrg, void *data)
{
  argvs_eval ("zoom fit");
  return 0;
}

static void get_coords (State *o, float screen_x, float screen_y, float *gegl_x, float *gegl_y)
{
  float scale = o->scale;
  *gegl_x = (o->u + screen_x) / scale;
  *gegl_y = (o->v + screen_y) / scale;
}



static void scroll_cb (MrgEvent *event, void *data1, void *data2)
{
  switch (event->scroll_direction)
  {
     case MRG_SCROLL_DIRECTION_DOWN:
       /* XXX : passing floating point values string formatted is awkard in C,
                so we use the utility function instead - this could be two extra
                relative coordinate args to the zoom command */
       zoom_at (data1, event->device_x, event->device_y, 1.0/1.05);
       break;
     case MRG_SCROLL_DIRECTION_UP:
       zoom_at (data1, event->device_x, event->device_y, 1.05);
       break;
     default:
       break;
  }
}

static void print_setting (Setting *setting)
{
  State *o = global_state;
  switch (setting->type)
  {
    case 0:
      printf ("%s %i%s\n  %s\n", setting->name,
        (*(int*)(((char *)o) + setting->offset)), setting->read_only?"  (RO)":"", setting->description);
    break;
    case 1:
      printf ("%s %f%s\n  %s\n", setting->name,
        (*(float*)(((char *)o) + setting->offset)), setting->read_only?" (RO)":"", setting->description);
     break;
    case 2:
      {
        char *value = NULL;
        memcpy (&value, (((char *)o) + setting->offset), sizeof (void*));
        printf ("%s %s%s\n  %s\n", setting->name, value, setting->read_only?" (RO)":"", setting->description);
      }
    break;
  }
}

static int set_setting (Setting *setting, const char *value)
{
  State *o = global_state;
  if (setting->read_only)
    return -1;
  switch (setting->type)
  {
    case 0:
        (*(int*)(((char *)o) + setting->offset)) = atoi (value);
    break;
    case 1:
        (*(float*)(((char *)o) + setting->offset)) = g_strtod (value, NULL);
    break;
    case 2:
        memcpy ((((char *)o) + setting->offset), g_strdup (value), sizeof (void*));
    break;
  }
  return 0;
}


int cmd_info (COMMAND_ARGS); /* "info", 0, "", "dump information about active node"*/

int
cmd_info (COMMAND_ARGS)
{
  State *o = global_state;
  GeglNode *node = o->active;
  GeglOperation *operation;
  GeglRectangle extent;

  if (!node)
  {
    printf ("no active node\n");
    return 0;
  }
  operation = gegl_node_get_gegl_operation (node);
  printf ("operation: %s   %p\n", gegl_node_get_operation (node), node);
  if (gegl_node_has_pad (node, "input"))
  {
    const Babl *fmt = gegl_operation_get_format (operation, "input");
    printf ("input pixfmt: %s\n", fmt?babl_get_name (fmt):"");
  }
  if (gegl_node_has_pad (node, "aux"))
  {
    const Babl *fmt = gegl_operation_get_format (operation, "aux");
    printf ("aux pixfmt: %s\n", fmt?babl_get_name (fmt):"");
  }
  if (gegl_node_has_pad (node, "output"))
  {
    const Babl *fmt = gegl_operation_get_format (operation, "output");
    printf ("output pixfmt: %s\n", fmt?babl_get_name (fmt):"");
  }
  extent = gegl_node_get_bounding_box (node);
  printf ("bounds: %i %i  %ix%i\n", extent.x, extent.y, extent.width, extent.height);


  printf ("%s\n", o->active);
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

int cmd_set (COMMAND_ARGS); /* "set", -1, "<setting> | <setting> <new value>| empty", "query/set various settings"*/
int
cmd_set (COMMAND_ARGS)
{
  char *key = NULL;
  char *value = NULL;
  int n_settings = sizeof (settings)/sizeof(settings[0]);

  if (argv[1])
  {
    key = argv[1];
    if (argv[2])
      value = argv[2];
  }
  else
  {
    for (int i = 0; i < n_settings; i++)
    {
      print_setting (&settings[i]);
    }
    return 0;
  }

  if (value)
  {
    for (int i = 0; i < n_settings; i++)
    {
      if (!strcmp (key, settings[i].name))
      {
        return set_setting (&settings[i], value);
      }
    }
  }
  else
  {
    for (int i = 0; i < n_settings; i++)
    {
      if (!strcmp (key, settings[i].name))
      {
        print_setting (&settings[i]);
        break;
      }
    }
  }
  return 0;
}

int cmd_toggle (COMMAND_ARGS); /* "toggle", 1, "<editing|fullscreen|cheatsheet|mipmap|controls|slideshow>", ""*/
int
cmd_toggle (COMMAND_ARGS)
{
  State *o = global_state;
  if (!strcmp(argv[1], "editing"))
  {
    o->show_graph = !o->show_graph;
    activate_sink_producer (o);
  }
  else if (!strcmp(argv[1], "fullscreen"))
  {
    mrg_set_fullscreen (o->mrg, !mrg_is_fullscreen (o->mrg));
    mrg_add_timeout (o->mrg, 250, deferred_zoom_to_fit, o);
  }
  else if (!strcmp(argv[1], "cheatsheet"))
  {
    o->show_bindings = !o->show_bindings;
  }
  else if (!strcmp(argv[1], "colormanage-display"))
  {
    o->color_manage_display = !o->color_manage_display;
    printf ("%s colormanagement of display\n", o->color_manage_display?"enabled":"disabled");
    mrg_gegl_dirty ();
  }
  else if (!strcmp(argv[1], "mipmap"))
  {
    gboolean curval;
    g_object_get (gegl_config(), "mipmap-rendering", &curval, NULL);
    if (curval == FALSE) {
      g_object_set (gegl_config(), "mipmap-rendering", TRUE, NULL);
      renderer = GEGL_RENDERER_BLIT_MIPMAP;
      printf ("enabled mipmap rendering\n");
    }
    else
    {
      g_object_set (gegl_config(), "mipmap-rendering", FALSE, NULL);
      renderer = GEGL_RENDERER_IDLE;
      printf ("disabled mipmap rendering\n");
    }
  }
  else if (!strcmp(argv[1], "controls"))
  {
    o->show_controls = !o->show_controls;
  }
  else if (!strcmp(argv[1], "slideshow"))
  {
    o->slide_enabled = !o->slide_enabled;
    if (o->slide_timeout)
      mrg_remove_idle (o->mrg, o->slide_timeout);
    o->slide_timeout = 0;
  }
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}


  int cmd_discard (COMMAND_ARGS);
int cmd_discard (COMMAND_ARGS) /* "discard", 0, "", "moves the current image to a .discard subfolder"*/
{
  State *o = global_state;
  char *old_path;
  char *tmp;
  char *lastslash;
  char *path = o->path;
  if (o->is_dir)
  {
    path = g_list_nth_data (o->paths, o->entry_no);
  }

  old_path = g_strdup (path);
  if (!o->is_dir)
  {
  argvs_eval ("next");
  if (!strcmp (old_path, o->path))
   {
     argvs_eval ("prev");
   }
  }
  tmp = g_strdup (old_path);
  lastslash  = strrchr (tmp, '/');
  if (lastslash)
  {
    char command[2048];
    char *suffixed = suffix_path (old_path);
    if (lastslash == tmp)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';

    // XXX : replace with proper code, also discard thumb?

    sprintf (command, "mkdir %s/.discard > /dev/null 2>&1", tmp);
    system (command);
    sprintf (command, "mv %s %s/.discard > /dev/null 2>&1", old_path, tmp);
    system (command);
    sprintf (command, "mv %s %s/.discard > /dev/null 2>&1", suffixed, tmp);
    system (command);
    g_free (suffixed);
    populate_path_list (o);
  }
  g_free (tmp);
  g_free (old_path);
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

  int cmd_save (COMMAND_ARGS);
int cmd_save (COMMAND_ARGS) /* "save", 0, "", ""*/
{
  State *o = global_state;
  char *serialized;

  {
    char *containing_path = get_path_parent (o->save_path);
    serialized = gegl_serialize (o->source,
                   gegl_node_get_producer (o->sink, "input", NULL),
 containing_path, GEGL_SERIALIZE_TRIM_DEFAULTS|GEGL_SERIALIZE_VERSION|GEGL_SERIALIZE_INDENT);
    g_free (containing_path);
  }

  {
    char *prepended = g_strdup_printf ("gegl:load path=%s\n%s", basename(o->src_path), serialized);
    g_file_set_contents (o->save_path, prepended, -1, NULL);
    g_free (prepended);
  }

  g_free (serialized);
  o->rev = 0;
  return 0;
}

#if 0
void gegl_node_defaults (GeglNode *node)
{
  const gchar* op_name = gegl_node_get_operation (node);
  {
    guint n_props;
    GParamSpec **pspecs = gegl_operation_list_properties (op_name, &n_props);
    if (pspecs)
    {
      for (gint i = 0; i < n_props; i++)
      {
        if (g_type_is_a (pspecs[i]->value_type, G_TYPE_DOUBLE))
        {
          GParamSpecDouble    *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_INT))
        {
          GParamSpecInt *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
        else if (g_type_is_a (pspecs[i]->value_type, G_TYPE_STRING))
        {
          GParamSpecString *pspec = (void*)pspecs[i];
          gegl_node_set (node, pspecs[i]->name, pspec->default_value, NULL);
        }
      }
      g_free (pspecs);
    }
  }
}
#endif

/* loads the source image corresponding to o->path into o->buffer and
 * creates live gegl pipeline, or nops.. rigs up o->save_path to be
 * the location where default saves ends up.
 */
void
gegl_meta_set (const char *path,
               const char *meta_data)
{
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (error)
  {
    g_warning ("%s", error->message);
  }
  else
  {
    if (gexiv2_metadata_has_tag (e2m, "Xmp.xmp.GEGL"))
      gexiv2_metadata_clear_tag (e2m, "Xmp.xmp.GEGL");

    gexiv2_metadata_set_tag_string (e2m, "Xmp.xmp.GEGL", meta_data);
    gexiv2_metadata_save_file (e2m, path, &error);
    if (error)
      g_warning ("%s", error->message);
  }
  g_object_unref (e2m);
}

char *
gegl_meta_get (const char *path)
{
  gchar  *ret   = NULL;
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
    ret = gexiv2_metadata_get_tag_string (e2m, "Xmp.xmp.GEGL");
  /*else
    g_warning ("%s", error->message);*/
  g_object_unref (e2m);
  return ret;
}

GExiv2Orientation path_get_orientation (const char *path)
{
  GExiv2Orientation ret = 0;
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
    ret = gexiv2_metadata_get_orientation (e2m);
  /*else
    g_warning ("%s", error->message);*/
  g_object_unref (e2m);
  return ret;
}

#endif

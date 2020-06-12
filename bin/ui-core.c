/* The code in this file is the core of a image-viewer/editor in progress
 */

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


#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#ifdef HAVE_MRG

#define font_size_scale  0.020

const char *css =
"div.collstars {color: yellow; font-size: 1.3em; background: transparent;};"
"div.viewerstars {color: yellow; font-size: 5vh; background: transparent;};"
"div.lui { font-size: 2.0vh; color: white; padding-left:1em; padding-bottom: 1em; position: absolute; top: 0; right: 1em; width:20em; background-color:rgba(1,0,0,0.0);}\n"
"div.properties { color: blue; padding-left:1em; padding-bottom: 1em; position: absolute; top: 0; right: 1em; width:20em; background-color:rgba(1,0,0,0.75);}\n"
"div.property   { color: white; margin-top: -.5em; background:transparent;}\n"
"div.propname { color: white;}\n"
"div.propvalue { color: yellow;background: transparent;}\n"
"span.propvalue-enum { color: gray; padding-right: 2em; display: box-inline; }\n"
"span.propvalue-enum-selected{ color: yellow; padding-right: 2em; display: box-inline; }\n"

"dl.bindings   { font-size: 1.8vh; color:white; position:absolute;left:1em;top:60%;background-color: rgba(0,0,0,0.7); width: 100%; height: 40%; padding-left: 1em; padding-top:1em;}\n"
"dt.binding   { color:white; }\n"

"div.graph, div.properties, div.scrollback{ font-size: 1.8vh; }\n"
"div.commandline-shell { font-size: 4.0vh; }\n"

"div.graph {position:absolute; top: 0; left: 0; color:white; }\n"

"div.node, div.node-active {border: 1px solid gray; color:#000; position: absolute; background-color: rgba(255,255,255,0.75); padding-left:1em;padding-right:1em;height:1em;width:8em;padding-top:0.25em;}\n"
"div.node-active { color: #000; background-color: rgba(255,255,255,1.0); text-decoration: underline; }\n"

"div.props {}\n"
"a { color: yellow; text-decoration: none;  }\n"


"div.operation-selector { font-size: 3vh; color: green; border: 1px solid red; padding-left:1em; padding-bottom: 1em; position: absolute; top: 4em; left: 2%; width:70%; background-color:rgba(1,0,0,0.0);height: 90%;}\n"
"div.operation-selector-close { color: red; }\n"
"div.operation-selector-op { background: black; color: white; display: inline; padding-right: 1em; }\n"
"div.operation-selector-op-active { background: black; color: yellow; display: inline; padding-right: 1em; }\n"
"div.operation-selector-categories {}\n"
"div.operation-selector-operations {}\n"
"div.operation-selector-category { background: black; color: gray; display: inline; padding-right: 1em; }\n"
"div.operation-selector-category-active { background: black; color: yellow; display: inline; padding-right: 1em; }\n"
"div.operation-selector-operation { background: black; color: white; }\n"


"div.scrollback{ color:white; position:fixed;left:0em;background-color: rgba(0,0,0,0.75); left:0%; width:100%;  padding-left: 1em; padding-top:1em;padding-bottom:1em;}\n"
"div.scrollline { background-color:rgba(0,0,0,0.0);color:white; }\n"


"div.commandline-shell{ color:white; position:fixed;background-color: rgba(0,0,0,0.75); top: 0%; left:0em; width:100%;  padding-left: .5em; padding-top:.5em;padding-bottom:.5em;}\n"

"div.prompt { color:#7aa; display: inline; }\n"
"div.commandline { color:white; display: inline;  }\n"
"span.completion{ color: rgba(255,255,255,0.7); padding-right: 1em; }\n"
"span.completion-selected{ color: rgba(255,255,0,1.0); padding-right: 1em; }\n"
"";


/* these are define here to be near the CSS */

#define active_pad_color          1.0, 1.0, 1.0, 1.0
#define active_pad_stroke_color   1.0, 0.0, 0.0, 1.0

#define pad_color                 0.0, 0.0, 0.0, 0.25
#define pad_stroke_color          1.0, 1.0, 1.0, 1.0
#define pad_radius                0.25
#define active_pad_radius         0.5


#ifdef HAVE_LUA

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#endif

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
GeState *global_state;

G_DEFINE_TYPE (GeState, ge_state, G_TYPE_OBJECT)

enum
{
    PROP_0,
    PROP_SOURCE,
    PROP_ACTIVE,
    PROP_SINK,
    PROP_PATH,
    N_PROPERTIES
};

static GParamSpec *obj_properties[N_PROPERTIES] = { NULL, };

enum {
  GEGL_RENDERER_BLIT = 0,
  GEGL_RENDERER_BLIT_MIPMAP,
  GEGL_RENDERER_THREAD,
  GEGL_RENDERER_IDLE,
  //GEGL_RENDERER_IDLE_MIPMAP,
  //GEGL_RENDERER_THREAD_MIPMAP,
};


static int renderer = GEGL_RENDERER_BLIT;
static void
ge_state_init (GeState *o)
{
  const char *renderer_env = g_getenv ("GEGL_RENDERER");
  o->scale             = 1.0;
  o->graph_scale       = 1.0;
  o->thumbbar_scale    = 1.0;
  o->thumbbar_opacity  = 1.0;
  o->show_thumbbar     = 1.0;
  o->show_bounding_box = 1;
  o->render_quality    = 1.0;
  o->preview_quality   = 1.0;
  //o->preview_quality = 2.0;
  o->slide_pause       = 5.0;
  o->paint_color       = g_strdup ("white");
  o->show_bindings     = 0;
  o->sort_order        = SORT_ORDER_CUSTOM | SORT_ORDER_AZ;
  o->ui_consumer = g_hash_table_new (g_direct_hash, g_direct_equal);

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

  o->renderer_state    = 0;
  o->gegl = gegl_node_new ();
}

static void
ge_state_set_property (GObject      *object,
                          guint         property_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
    GeState *state = GE_STATE (object);

    switch (property_id) {
    case PROP_SOURCE:
        g_set_object (&state->source, g_value_get_object (value));
        break;
    case PROP_SINK:
        g_set_object (&state->sink, g_value_get_object (value));
        break;
    case PROP_ACTIVE:
        g_set_object (&state->active, g_value_get_object (value));
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

static void
ge_state_get_property (GObject    *object,
                          guint       property_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
    GeState *state = GE_STATE (object);

    switch (property_id) {
    case PROP_SOURCE:
        g_value_set_object (value, state->source);
        break;
    case PROP_ACTIVE:
        g_value_set_object (value, state->active);
        break;
    case PROP_SINK:
        g_value_set_object (value, state->sink);
        break;
    case PROP_PATH:
        g_value_set_string (value, state->path);
        break;

    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}

#ifdef HAVE_LUA
static lua_State *L = NULL;
#endif

static void
ge_state_finalize (GObject *object)
{
    GeState *o = GE_STATE (object);

#ifdef HAVE_LUA
    if(L)lua_close(L);
#endif

    g_clear_object (&o->gegl);
    g_clear_object (&o->processor);
    g_clear_object (&o->processor_buffer);
    g_clear_object (&o->buffer);


    G_OBJECT_CLASS (ge_state_parent_class)->finalize (object);
}

static void
ge_state_class_init (GeStateClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->set_property = ge_state_set_property;
    object_class->get_property = ge_state_get_property;
    object_class->finalize = ge_state_finalize;

    obj_properties[PROP_SOURCE] =
        g_param_spec_object ("source", "Source", "Source node in processing chain",
                             GEGL_TYPE_NODE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_SINK] =
        g_param_spec_object ("sink", "Sink", "Sink node in processing chain",
                             GEGL_TYPE_NODE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_ACTIVE] =
        g_param_spec_object ("active", "Active", "Active node",
                             GEGL_TYPE_NODE,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    obj_properties[PROP_PATH] =
        g_param_spec_string ("path", "Path", "Path of active image",
                             NULL,
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

    g_object_class_install_properties (object_class,
                                       N_PROPERTIES,
                                       obj_properties);
}

GeState *ge_state_new (void)
{
  return g_object_new (GE_STATE_TYPE, NULL);
}

int ui_items_count (GeState *o)
{
  return g_list_length (o->index) +
         g_list_length (o->paths);
}

/* gets the node which is the direct consumer, and not a clone.
 *
 * valid after update_ui_consumers_list (State *o, GeglNode *iter)
 */
static GeglNode *gegl_node_get_ui_consumer (GeglNode    *node,
                                            const char  *output_pad,
                                            const char **consumer_pad);

static GeglNode *gegl_node_get_ui_producer (GeglNode    *node,
                                            const char  *input_pad,
                                            char **output_pad)
{
  GeglNode *producer = gegl_node_get_producer (node, input_pad, output_pad);
  if (producer && node == gegl_node_get_ui_consumer (producer, "output", NULL))
    return producer;
  return NULL;
}

enum {
  PAD_INPUT = 0,
  PAD_AUX = 1,
  PAD_OUTPUT = 2,
};


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

  static char *lui_contents = NULL;

static gboolean text_editor_active (GeState *o)
{
  return o->editing_op_name ||
         o->editing_property ||
         (lui_contents && o->show_graph);
}

GeState *global_state = NULL;  // XXX: for now we  rely on
                               //      global state to make events/scripting work
                               //      refactoring this away would be nice, but
                               //      not a problem to have in a lua port of the same

GeState *app_state(void);
GeState *app_state(void)
{
  return global_state;
}

typedef struct Setting {
  char *name;
  char *description;
  int   offset;
  int   type;
  int   read_only;
} Setting;

#define FLOAT_PROP(name, description) \
  {#name, description, offsetof (GeState, name), 1, 0}
#define INT_PROP(name, description) \
  {#name, description, offsetof (GeState, name), 0, 0}
#define STRING_PROP(name, description) \
  {#name, description, offsetof (GeState, name), 2, 0}
#define FLOAT_PROP_RO(name, description) \
  {#name, description, offsetof (GeState, name), 1, 1}
#define INT_PROP_RO(name, description) \
  {#name, description, offsetof (GeState, name), 0, 1}
#define STRING_PROP_RO(name, description) \
  {#name, description, offsetof (GeState, name), 2, 1}

Setting settings[]=
{
  INT_PROP(color_managed_display, "perform ICC color management and convert output to display ICC profile instead of passing out sRGB, passing out sRGB is faster."),
  INT_PROP_RO(is_video, ""),
  STRING_PROP_RO(path, "path of current document"),
  STRING_PROP_RO(src_path, "path of current document"),
  STRING_PROP_RO(chain_path, "path of current document"),
  INT_PROP(playing, "wheter we are playing or not set to 0 for pause 1 for playing"),
  INT_PROP(loop_current, "wheter we are looping current instead of going to next"),
  STRING_PROP_RO(chain_path, "chain path will be different from path if current path is an immutable source image itself or same as path if it is a gegl chain directly"),
  STRING_PROP_RO(src_path, "source path the immutable source image currently being edited"),

//  FLOAT_PROP(u, "horizontal coordinate of top-left in display/scaled by scale factor coordinates"),
//  FLOAT_PROP(v, "vertical coordinate of top-left in display/scaled by scale factor coordinates"),
//  FLOAT_PROP(render_quality, "1.0 = normal 2.0 = render at 2.0 zoom factor 4.0 render at 25%"),
//  FLOAT_PROP(preview_quality, "preview quality for use during some interactions, same scale as render-quality"),
  FLOAT_PROP(scale, "display scale factor"),
  INT_PROP(show_preferences, "show preferences"),
  INT_PROP(show_bindings, "show currently valid keybindings"),
  INT_PROP(show_graph, "show the graph (and commandline)"),
  INT_PROP(show_thumbbar, "show the thumbbar"),
  INT_PROP(show_bounding_box, "show bounding box of active node"),
  INT_PROP(show_controls, "show image viewer controls (maybe merge with show-graph and give better name)"),
  INT_PROP(nearest_neighbor, "nearest neighbor"),
  INT_PROP(frame_cache, "store all rendered frames on disk uncompressed for fast scrubbing"),
  FLOAT_PROP(slide_pause, "display scale factor"),
  FLOAT_PROP(pos, "clip time position, set with apos"),
  FLOAT_PROP(duration, "clip duration, computed on load of clip"),
};


static void queue_draw (GeState *o)
{
  o->renderer_state = 0;
  renderer_dirty++;
  mrg_gegl_dirty (o->mrg);
  mrg_queue_draw (o->mrg, NULL);
}

static void rev_inc (GeState *o)
{
  o->rev++;
  queue_draw (o);
}

//static char *suffix = "-gegl";

void   gegl_meta_set (const char *path, const char *meta_data);
char * gegl_meta_get (const char *path);
GExiv2Orientation path_get_orientation (const char *path);

static int is_gegl_path (const char *path);

#if 0
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
#endif
gchar *ui_get_thumb_path (const char *path)
{
  gchar *ret;
  gchar *basename = g_path_get_basename (path);
  gchar *dirname = g_path_get_dirname (path);
  ret = g_strdup_printf ("%s/.gegl/%s/thumb.jpg", dirname, basename);
  g_free (basename);
  g_free (dirname);
  return ret;
}

gchar *ui_get_metadata_path (const char *path);
gchar *
ui_get_metadata_path (const char *path)
{
  gchar *ret;
  gchar *basename = g_path_get_basename (path);
  gchar *dirname = g_path_get_dirname (path);
  ret = g_strdup_printf ("%s/.gegl/%s/metadata", dirname, basename);
  g_free (basename);
  g_free (dirname);
  return ret;
}

gchar *ui_get_index_path (const char *path);
gchar *
ui_get_index_path (const char *path)
{
  gchar *ret;
  ret = g_strdup_printf ("%s/.gegl/index", path);
  return ret;
}



static void get_coords                (GeState *o, float  screen_x, float screen_y,
                                                 float *gegl_x,   float *gegl_y);
static void drag_preview              (MrgEvent *e);
static void load_into_buffer          (GeState *o, const char *path);
static void zoom_to_fit               (GeState *o);
static void center                    (GeState *o);
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

static int
index_contains (GeState *state,
                const char *name);


static int order_az (gconstpointer a, gconstpointer b)
{
  const char *apath = a;
  const char *bpath = b;

  const char *abasename = strrchr (apath, '/');
  const char *bbasename = strrchr (bpath, '/');

  if (abasename) abasename++;
  else return 0;
  if (bbasename) bbasename++;
  else return 0;

  return strcmp (abasename, bbasename);
}

static int order_stars (gconstpointer a, gconstpointer b, void *data)
{
  GeState *state = data;
  const char *apath = a;
  const char *bpath = b;

  return meta_get_key_int (state, bpath, "stars") -
         meta_get_key_int (state, apath, "stars");
}

static int order_mtime (gconstpointer a, gconstpointer b)
{
  struct stat stat_a;
  struct stat stat_b;
  lstat (a, &stat_a);
  lstat (b, &stat_b);
  return stat_a.st_mtime - stat_b.st_mtime;
}

static int order_exif_time (gconstpointer a, gconstpointer b)
{
  /* XXX : reading out and parsing the exif data twice for each comparison
           is a too severe bottleneck - the data to compare needs to exist
           in the list before sorting
   */
  GError *error = NULL;
  int ret;
  GExiv2Metadata *e2m_a = gexiv2_metadata_new ();
  GExiv2Metadata *e2m_b = gexiv2_metadata_new ();
  char *val_a, *val_b;

  gexiv2_metadata_open_path (e2m_a, a, &error);
  gexiv2_metadata_open_path (e2m_b, b, &error);

  val_a = gexiv2_metadata_get_tag_string (e2m_a, "Exif.Photo.DateTimeOriginal");
  val_b = gexiv2_metadata_get_tag_string (e2m_b, "Exif.Photo.DateTimeOriginal");
  if (val_a && val_b)
    ret = strcmp (val_a, val_b);
  else if (val_a)
    ret = 1;
  else if (val_b)
    ret = -1;
  else
    ret = 0;

  if (val_a)
    g_free (val_a);
  if (val_b)
    g_free (val_b);

  g_object_unref (e2m_a);
  g_object_unref (e2m_b);

  return ret;
}

/*
 *  the path list need repopulation when the folder changes (we do it on all document changes to
 *  get updates). It also needs changing when the sort order changes.
 *
 */



void populate_path_list (GeState *o)
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

  /* chop off basename if path is to a regular file */
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

  /* first list folders */
  for (i = 0; i < n; i++)
  {
    if (namelist[i]->d_name[0] != '.' && !index_contains (o, namelist[i]->d_name))
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

  /* then list files
   */
  {
  GList *temp = NULL;

  for (i = 0; i < n; i++)
    {
      if (namelist[i]->d_name[0] != '.' &&
          !index_contains (o, namelist[i]->d_name) &&
          str_has_visual_suffix (namelist[i]->d_name))
      {
        gchar *fpath = g_strdup_printf ("%s/%s", path, namelist[i]->d_name);

        lstat (fpath, &stat_buf);
        if (S_ISREG (stat_buf.st_mode))
        {
#if 0
          if (is_gegl_path (fpath))
          {
            char *tmp = ui_unsuffix_path (fpath);
            g_free (fpath);
            fpath = g_strdup (tmp);
            g_free (tmp);
          }
#endif

          if (!g_list_find_custom (o->paths, fpath, (void*)g_strcmp0))
          {
            if (o->sort_order & SORT_ORDER_AZ)
              temp = g_list_insert_sorted (temp, fpath, order_az);
            else if (o->sort_order & SORT_ORDER_MTIME)
              temp = g_list_insert_sorted (temp, fpath, order_mtime);
            else if (o->sort_order & SORT_ORDER_EXIF_TIME)
              temp = g_list_insert_sorted (temp, fpath, order_exif_time);
            else if (o->sort_order & SORT_ORDER_STARS)
              temp = g_list_insert_sorted_with_data (temp, fpath, order_stars, o);
          }
          else
            g_free (fpath);
        }
      }
    }
    o->paths = g_list_concat (o->paths, temp);
  }

  for (int i = 0; i < n;i++)
    free(namelist[i]);
//  free (namelist);
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
  char *thumbpath;
} ThumbQueueItem;

static void thumb_queue_item_free (ThumbQueueItem *item)
{
  if (item->path)
    g_free (item->path);
  if (item->thumbpath)
    g_free (item->thumbpath);
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

static void
load_index (GeState *state, const char *path);
static void
store_index (GeState *state, const char *path);

static void load_path_inner (GeState *o, char *path);

static gchar *pos_hash (GeState *o)
{
  GChecksum *hash;
  char *ret;
  gchar *frame_recipe;
  frame_recipe = gegl_serialize (NULL, o->sink, NULL, GEGL_SERIALIZE_BAKE_ANIM);
  hash = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (hash, (void*)frame_recipe, -1);
  g_checksum_update (hash, (void*)o->src_path, -1); /*
     we add this in to make the identical source-buffer based recipies hash to different results
     for now this hack doesn't matter since the frame_recipe is unused
     would be better to rely only on hash of recipe and have recipe be complete thus using real gegl:load
   */
  ret = g_strdup (g_checksum_get_string(hash));
  g_checksum_free (hash);
  //fprintf (stderr, "{%s}\n\n", frame_recipe);
  g_free (frame_recipe);
  return ret;
}

/* .ppm, .exr, .tif, .jpg, .png, .geglbuffer
 *
 */
//static char *frame_ext = ".png";
//static char *frame_ext = ".jpg";
//static char *frame_ext = ".exr";
//static char *frame_ext = ".tif";
//static char *frame_ext = ".ppm";
static char *frame_ext = ".geglbuffer";

static GeglBuffer *_gegl_buffer_load (const char *path)
{
  GeglBuffer *buffer = NULL;
  GeglNode *gegl, *load, *sink;
  if (!strcmp (frame_ext, ".geglbuffer"))
  {
    buffer = gegl_buffer_open (path);
  }
  else
  {
    gegl = gegl_node_new ();
    load = gegl_node_new_child (gegl, "operation", "gegl:load",
                                      "path", path,
                                      NULL);
    sink = gegl_node_new_child (gegl, "operation", "gegl:buffer-sink",
                                      "buffer", &buffer,
                                      NULL);
    gegl_node_link_many (load, sink, NULL);
    gegl_node_process (sink);
    g_object_unref (gegl);
  }
  return buffer;
}

static void _gegl_buffer_save (GeglBuffer *buffer,
                               const char *path)
{
  if (!strcmp (frame_ext, ".geglbuffer"))
  {
    gegl_buffer_save (buffer, path, NULL);
  }
  else
  {
    GeglNode *gegl, *load, *sink;
    gegl = gegl_node_new ();
    load = gegl_node_new_child (gegl, "operation", "gegl:buffer-source",
                                      "buffer", buffer,
                                      NULL);
    if (!strcmp (frame_ext, ".png"))
      sink = gegl_node_new_child (gegl, "operation", "gegl:png-save",
                                        "compression", 2,
                                        "bitdepth", 8,
                                        "path", path,
                                        NULL);
    else
      sink = gegl_node_new_child (gegl, "operation", "gegl:save",
                                        "path", path,
                                        NULL);
    gegl_node_link_many (load, sink, NULL);
    gegl_node_process (sink);
    g_object_unref (gegl);
  }
}


static int has_quit = 0;
static GeglAudioFragment *cached_audio = NULL;

/* returns 1 if frame was loaded from cache */
static int frame_cache_check (GeState *o, const char *hash)
{
  char *path = NULL;
  int ret = 0;
  hash = pos_hash (o);
  {
    char *dir = get_item_dir (o);

    path = g_strdup_printf ("%s/.gegl/frame_cache", dir);
    g_mkdir_with_parents (path, 0777);
    g_free (path);

    path = g_strdup_printf ("%s/.gegl/frame_cache/%s.pcm", dir, hash);
    g_free (dir);
  }

  if (g_file_test (path, G_FILE_TEST_EXISTS))
  {
     char *contents = NULL;
     g_file_get_contents (path, &contents, NULL, NULL);
     if (contents)
     {
       gchar *p;
       GString *word = g_string_new ("");
       int element_no = 0;
       int channels = 2;
       int max_samples = 2000;
       cached_audio = gegl_audio_fragment_new (44100, 2, 0, 44100);
       for (p = contents; p==contents || p[-1] != '\0'; p++)
       {
         switch (p[0])
         {
           case '\0':case ' ':
           if (word->len > 0)
             {
               switch (element_no++)
               {
                 case 0:
                   gegl_audio_fragment_set_sample_rate (cached_audio, g_ascii_strtod (word->str, NULL));
                   break;
                 case 1:
                   channels = g_ascii_strtod (word->str, NULL);
                   gegl_audio_fragment_set_channels (cached_audio, channels);
                   break;
                 case 2:
                   gegl_audio_fragment_set_channel_layout (cached_audio, g_ascii_strtod (word->str, NULL));
                   break;
                 case 3:
                   gegl_audio_fragment_set_sample_count (cached_audio, g_ascii_strtod (word->str, NULL));
                   break;
                 default:
                   {
                     int sample_no = element_no - 4;
                     int channel_no = sample_no % channels;
                     sample_no/=2;
                     if (sample_no < max_samples)
                     cached_audio->data[channel_no][sample_no] = g_ascii_strtod (word->str, NULL);
                   }
                   break;
               }
             }
             g_string_assign (word, "");
             break;
           default:
             g_string_append_c (word, p[0]);
             break;
         }
       }
       g_string_free (word, TRUE);
       g_free (contents);
     }
  }
  g_free (path);

  {
    char *dir = get_item_dir (o);
    path = g_strdup_printf ("%s/.gegl/frame_cache/%s%s", dir, hash, frame_ext);
    g_free (dir);
  }

  if (g_file_test (path, G_FILE_TEST_EXISTS))
  {
    if (o->cached_buffer)
      g_object_unref (o->cached_buffer);
    //o->cached_buffer = gegl_buffer_open (path);
    o->cached_buffer = _gegl_buffer_load (path);
    ret = 1;
  }
  g_free (path);

  /* other loaders, like jpg, png, exr could be added here */
  return ret;
}

static void frame_cache_store (GeState *o, const char *hash)
{
    char *path = NULL;
    char *dir = get_item_dir (o);
    path = g_strdup_printf ("%s/.gegl/frame_cache/%s%s", dir, hash, frame_ext);
    g_free (dir);

    if (!g_file_test (path, G_FILE_TEST_EXISTS))
      {
        //gegl_buffer_save (o->processor_buffer, path, NULL);
        _gegl_buffer_save (o->processor_buffer, path);
      }
    else
      {
        fprintf (stderr, "odd cache resave\n");
      }
    g_free (path);
}

static uint32_t prev_complete_ms = 0;

static int last_ms = 0;
// when set, is the mrg_ms time of last queued render

static gboolean renderer_task (gpointer data)
{
  GeState *o = data;
  static gdouble progress = 0.0;
  void *old_processor = o->processor;
  GeglBuffer *old_buffer = o->processor_buffer;
  static char *hash = NULL;
  static guint32 render_start = 0;

#define TASK_BASE                0
#define TASK_RENDER              1
#define TASK_RENDER_DONE         2
#define TASK_THUMB               3
#define TASK_PCM_FRAME_CACHE     4

  if (prev_complete_ms == 0)
    prev_complete_ms = mrg_ms (o->mrg);

  switch (o->renderer_state)
  {
    case TASK_BASE:
      if (renderer == GEGL_RENDERER_BLIT||
          renderer == GEGL_RENDERER_BLIT_MIPMAP)
      {
         o->renderer_state = TASK_THUMB;
         break;
      }

      if (renderer_dirty)
      {
        renderer_dirty = 0;
        render_start = mrg_ms (o->mrg);

        g_clear_object (&o->cached_buffer);
        if (o->processor_node != o->sink)
        {
          o->processor = gegl_node_new_processor (o->sink, NULL);
          o->processor_buffer = g_object_ref (gegl_processor_get_buffer (o->processor));
          g_clear_object (&old_buffer);
          g_clear_object (&old_processor);

          if(1)
          {
            GeglRectangle rect = {o->u / o->scale, o->v / o->scale, mrg_width (o->mrg) / o->scale,
                          mrg_height (o->mrg) / o->scale};
            gegl_processor_set_rectangle (o->processor, &rect);
          }
        }
        o->renderer_state = TASK_RENDER;
        if (hash)
          g_free (hash);
        hash = NULL;

        g_clear_object (&cached_audio);

        //if (o->frame_cache)
        /* we always check for cache - this makes the cache kick-in when turned off but cached entries are valid */
        hash = pos_hash (o);
        if (frame_cache_check (o, hash))
        {
          o->renderer_state = TASK_RENDER_DONE;
          renderer_task (o);
        }
      }
      else
      {

        /* if it has been more than 1/3s since a queued
           redraw - and the currently cached cairo_surface of the GeglBuffer is
           using nearest neighbor, queue a redraw. */
        if (mrg_ms (o->mrg) - last_ms > 333 && last_ms > 0 && mrg_gegl_got_nearest())
        {
          last_ms = 0;
          mrg_gegl_dirty (o->mrg);
          mrg_queue_draw (o->mrg, NULL);
        }

        if (thumb_queue)
        {
          o->renderer_state = TASK_THUMB;
        }
      else
        {
          g_usleep (500);
          o->renderer_state = TASK_BASE;
        }
      }

      if (o->renderer_state == TASK_RENDER)
        renderer_task (o); /* recursively invoke next state in same iteration of task */

      break;
    case TASK_RENDER:
       if (o->cached_buffer)
       {
         if (o->renderer_state)
         {
           o->renderer_state = TASK_RENDER_DONE;
           renderer_task (o);
         }
       }
       else
       {
         if (gegl_processor_work (o->processor, &progress))
         {
           if (o->renderer_state)
             o->renderer_state = TASK_RENDER;
          }
          else
          {
            if (o->renderer_state)
            {
              o->renderer_state = TASK_RENDER_DONE;
              renderer_task (o);
            }
          }
       }
       break;
    case TASK_RENDER_DONE:
      mrg_gegl_dirty (o->mrg);
      {
        guint32 ms = mrg_ms (o->mrg);
        float fps = 1.0/((ms-prev_complete_ms)/1000.0);
        static float avgfps = 0.0;
        float dt = 0.9;
        avgfps = avgfps * dt + fps * (1.0-dt);

        if(0)fprintf (stderr, "frame delta: %ims %.3ffps render time:%ims  \r", ms-prev_complete_ms,
            avgfps, ms-render_start);
        prev_complete_ms = ms;
      }


      switch (renderer)
      {
        case GEGL_RENDERER_IDLE:
          mrg_queue_draw (o->mrg, NULL);
          break;
        case GEGL_RENDERER_THREAD:
          mrg_queue_draw (o->mrg, NULL);
          g_usleep (500);
          break;
      }

      if ((o->frame_cache && !o->cached_buffer) || o->is_video )
      {
        o->renderer_state = TASK_PCM_FRAME_CACHE;
        renderer_task (o);
      }
      else
        o->renderer_state = TASK_BASE;
      break;
    case TASK_THUMB:

      if (thumb_queue)
      {
        static GPid thumbnailer_pid = 0;
#define THUMB_BATCH_SIZE    16
        char *argv[THUMB_BATCH_SIZE]={"gegl","--thumbgen", NULL};
        int count = 2;

        if (thumbnailer_pid == 0 ||
            kill(thumbnailer_pid, 0) == -1)
        {
        for (MrgList *iter = thumb_queue; iter && count < THUMB_BATCH_SIZE-2; iter=iter->next)
        {
          ThumbQueueItem *item = iter->data;
          if (access (item->thumbpath, F_OK) == -1)
          {
            argv[count++] = item->path;
            argv[count] = NULL;
          }
        }
         {
          GError *error = NULL;
          g_spawn_async (NULL, &argv[0], NULL,
              G_SPAWN_SEARCH_PATH|G_SPAWN_SEARCH_PATH_FROM_ENVP,
              NULL, NULL, &thumbnailer_pid, &error);
          if (error)
            g_warning ("%s", error->message);
#if 0
          else
            fprintf (stderr, "spawned %i items first is %s\n", count-2, argv[2]);
#endif
          }

          while (thumb_queue)
          {
            ThumbQueueItem *item = thumb_queue->data;
            mrg_list_remove (&thumb_queue, item);
            thumb_queue_item_free (item);
          }
        }
        g_usleep (500);
      }

      o->renderer_state = TASK_BASE;
      break;


  case TASK_PCM_FRAME_CACHE:
    if (o->frame_cache && !o->cached_buffer) // store cached render of frame
    {
      frame_cache_store (o, hash);
    }

    if (o->is_video)
    {
      GeglAudioFragment *audio = NULL;
      if (cached_audio)
      {
        audio = cached_audio;
        g_object_ref (cached_audio);
      }
      else
      {
        gegl_node_get (o->source, "audio", &audio, NULL);
      }
      if (audio)
      {
       int sample_count = gegl_audio_fragment_get_sample_count (audio);
       if (sample_count > 0)
       {
         int i;
         if (!audio_started)
         {
           open_audio (o->mrg, gegl_audio_fragment_get_sample_rate (audio));
           audio_started = 1;
         }
         {
         uint16_t temp_buf[sample_count * 2];
         for (i = 0; i < sample_count; i++)
         {
           temp_buf[i*2] = audio->data[0][i] * 32767.0 * 0.46;
           temp_buf[i*2+1] = audio->data[1][i] * 32767.0 * 0.46;
         }
         mrg_pcm_queue (o->mrg, (void*)&temp_buf[0], sample_count);

           /* after queing our currently decoded audio frame, we
              wait until the pcm buffer is nearly ready to play
              back our content
            */
           while (mrg_pcm_get_queued_length (o->mrg) > (1.0/o->fps) * 1.5  )
              g_usleep (10);
          }

       }
       if (audio != cached_audio && o->frame_cache)
       {
         int i, c;
         GString *str = g_string_new ("");
         int sample_count = gegl_audio_fragment_get_sample_count (audio);
         int channels = gegl_audio_fragment_get_channels (audio);
         char *path = NULL;
         char *dir = get_item_dir (o);
         path = g_strdup_printf ("%s/.gegl/frame_cache/%s.pcm", dir, hash);
         g_free (dir);
    
         g_string_append_printf (str, "%i %i %i %i",
                                 gegl_audio_fragment_get_sample_rate (audio),
                                 gegl_audio_fragment_get_channels (audio),
                                 gegl_audio_fragment_get_channel_layout (audio),
                                 gegl_audio_fragment_get_sample_count (audio));
    
         for (i = 0; i < sample_count; i++)
           for (c = 0; c < channels; c++)
              g_string_append_printf (str, " %0.5f", audio->data[c][i]);
    
         g_file_set_contents (path, str->str, -1, NULL);
         g_string_free (str, TRUE);
         g_free (path);
       }
       g_object_unref (audio);
      }
    }
    o->renderer_state = TASK_BASE;
    break;
  }

  if (has_quit)
  {
    if (hash)
      g_free (hash);
     hash = NULL;
  }

  return TRUE;
}

static gboolean renderer_idle (Mrg *mrg, gpointer data)
{
  return renderer_task (data);
}

static gpointer renderer_thread (gpointer data)
{
  while (!has_quit)
  {
    renderer_task (data);
  }
  return 0;
}


static char *binary_relative_data_dir = NULL;

#ifdef HAVE_LUA
static char *
resolve_lua_file (const char *basename);
#endif

int mrg_ui_main (int argc, char **argv, char **ops)
{
  Mrg *mrg = mrg_new (1024, 768, NULL);
  GeState *o;

  {
    char *lastslash;
    char *tmp = g_strdup (realpath (argv[0], NULL));
    if (tmp)
    {
      lastslash = strrchr (tmp, '/');
      *lastslash = 0;
      if (strstr (tmp, "/.libs"))
        *strstr (tmp, "/.libs") = 0;
      binary_relative_data_dir = g_strdup_printf ("%s/lua", tmp);
      g_free (tmp);
    }
  }

  mrg_set_image_cache_mb (mrg, 1024);
  mrg_set_title (mrg, "GEGL");

  gegl_init (&argc, &argv);

  global_state = o = ge_state_new ();

  o->ops = ops;
  o->mrg  = mrg;

  if (access (argv[1], F_OK) != -1)
    o->path = realpath (argv[1], NULL);
  else
    {
      o->path = g_strdup (g_get_home_dir ());
    }

#ifdef HAVE_LUA
  {
    int status, result;
    char *init_path = resolve_lua_file ("init.lua");
    const char * const *data_dirs = g_get_system_data_dirs ();

    L = luaL_newstate ();
    luaL_openlibs(L);

    /* expose o as a global light user data for luajit ffi interactions */
    lua_pushlightuserdata (L, o);
    lua_setglobal(L, "STATE");

    /* set up package path to permit using mrg, mmm and cairo ffi bindings
       we ship with, making it easier to get all dependencies sorted.
     */
    status = luaL_loadstring(L, "package.path = package.path .. ';./lua/?.lua'\n"
);
    result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result){
      fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
    }

    for (int i = 0; data_dirs[i]; i++)
    {
      char *script = g_strdup_printf ("package.path = package.path .. ';%s%sgegl-0.4/lua/?.lua'\n", data_dirs[i], data_dirs[i][strlen(data_dirs[i])-1]=='/'?"":"/");
      status = luaL_loadstring(L, script);
      result = lua_pcall(L, 0, LUA_MULTRET, 0);
      if (result){
        fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
      }
    }

    if (init_path)
    {
      status = luaL_loadfile(L, init_path);
      if (status)
      {
        fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
      }
      else
      {
        result = lua_pcall(L, 0, LUA_MULTRET, 0);
        if (result){
          fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
        }
      }
      g_free (init_path);
    }
  }
#endif

  ui_load_path (o);
  
  {
    int no = 0;
    char *basename = g_path_get_basename (o->path);
    o->entry_no = 0;
    for (GList *iter = o->index; iter && !o->entry_no; iter = iter->next, no++)
    {
      IndexItem *item = iter->data;
      if (!strcmp (item->name, basename))
        o->entry_no = no;
    }
    for (GList *iter = o->paths; iter && !o->entry_no; iter = iter->next, no++)
    {
      if (!strcmp (iter->data, o->path))
        o->entry_no = no;
    }
    g_free (basename);
  }

  mrg_set_ui (mrg, gegl_ui, o);
  on_viewer_motion (NULL, o, NULL);

  switch (renderer)
  {
    case GEGL_RENDERER_THREAD:
      o->renderer_thread = g_thread_new ("renderer", renderer_thread, o);
      break;
    case GEGL_RENDERER_IDLE:
      mrg_add_idle (mrg, renderer_idle, o);
      break;
    case GEGL_RENDERER_BLIT:
    case GEGL_RENDERER_BLIT_MIPMAP:
      break;
  }

  if (o->ops)
  {
    o->show_graph = 1;
  }

  mrg_main (mrg);
  has_quit = 1;
  if (renderer == GEGL_RENDERER_THREAD)
    g_thread_join (o->renderer_thread);

#ifdef HAVE_LUA
  /* manually run lua garbage collection before tearing down GEGL */
  if(L)
  {
    lua_gc(L, LUA_GCCOLLECT, 0);
  }
#endif

  g_object_unref (o);

  gegl_exit ();

  end_audio ();
  return 0;
}

void set_clip_position (GeState *o, double position)
{
  position = ceilf(position * o->fps) / o->fps; // quantize position

  o->pos = position;
  gegl_node_set_time (o->sink, o->pos + o->start);

  if (o->is_video)
  {
    gint frame = 0;

    frame = ceilf ((o->pos + o->start) * o->fps);
    gegl_node_set (o->source, "frame", frame, NULL);
  }
}

int cmd_lued (COMMAND_ARGS); /* "lued", 0, "<>", "" */
int
cmd_lued (COMMAND_ARGS)
{
  system ("gnome-terminal -e 'vim /home/pippin/src/gegl/bin/lua/'");
  return 0;
}

int cmd_apos (COMMAND_ARGS); /* "apos", 1, "<>", "set the animation time, this is time relative to clip, meaning 0.0 is first frame of clips timeline (negative frames will be used for fade-in, to keep timings the same), set position is quantized according to frame rate."*/
int
cmd_apos (COMMAND_ARGS)
{
  GeState *o = global_state;
  set_clip_position (o, g_ascii_strtod (argv[1], NULL));
  return 0;
}

int cmd_thumb (COMMAND_ARGS); /* "thumb", 0, "<>", "generate thumbnail for active image"*/
int
cmd_thumb (COMMAND_ARGS)
{
  GeState *o = global_state;
  gchar *thumbdata;
  GeglBuffer *buffer;
  GeglNode *gegl;
  GeglNode *saver;
  GeglNode *source;
  gchar *thumbpath;

  thumbpath = ui_get_thumb_path (o->path);
  /* protect against some possible repeated requests to generate the same thumb
   */
  if (g_file_test (thumbpath, G_FILE_TEST_EXISTS))
    return 0;

  {
    char *dirname = g_path_get_dirname (thumbpath);
    g_mkdir_with_parents (dirname, 0777);
    g_free (dirname);
  }

  gegl = gegl_node_new ();
  thumbdata = g_malloc0 (256 * 256 * 4);
  buffer = gegl_buffer_linear_new_from_data (thumbdata,
               babl_format ("R'G'B' u8"),
               GEGL_RECTANGLE(0,0,256,256),
               256 * 3, NULL, NULL);
  saver = gegl_node_new_child (gegl,
                  "operation", "gegl:jpg-save",
                  "path", thumbpath,
                  NULL);
  source = gegl_node_new_child (gegl,
                  "operation", "gegl:buffer-source",
                  "buffer", buffer,
                  NULL);
  gegl_node_link_many (source, saver, NULL);

  {
    GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
    float scale, scale2;
    float width = 256, height = 256;

    if (rect.width > 1000000 ||
        rect.height > 1000000)
    {
       rect.x=0;
       rect.y=0;
       rect.width=1024;
       rect.height=1024;
    }

    scale = 1.0 * width / rect.width;
    scale2 = 1.0 * height / rect.height;

    if (scale2 < scale) scale = scale2;

    gegl_node_blit (o->sink, scale,
                    GEGL_RECTANGLE(rect.x * scale - (256-rect.width*scale)/2,
                                   rect.y * scale - (256-rect.height*scale)/2,
                                   256,
                                   256),
                    babl_format("R'G'B' u8"),
                    thumbdata, 256*3, GEGL_BLIT_DEFAULT);
  }
  gegl_node_process (saver);
  g_free (thumbpath);
  g_object_unref (gegl);
  g_object_unref (buffer);
  g_free (thumbdata);
  fflush (NULL);
  sync ();
  return 0;
}

int thumbgen_main (int argc, char **argv);

int thumbgen_main (int argc, char **argv)
{
  GeState *o;
  gegl_init (&argc, &argv);

  o = global_state = ge_state_new ();

  for (char **arg = &argv[2]; *arg; arg++)
  {
    if (o->path)
      g_free (o->path);
    o->path = g_strdup (*arg);
    ui_load_path (o);

    if (o->source &&
        !strcmp (gegl_node_get_operation (o->source), "gegl:pdf-load"))
        gegl_node_set (o->source, "ppi", 72/2.0, NULL);
    argvs_eval ("thumb");
  }

  g_object_unref (o);

  gegl_exit ();

 exit (0);
}

int ui_hide_controls_cb (Mrg *mrg, void *data)
{
  GeState *o = data;
  o->controls_timeout = 0;
  o->show_controls    = 0;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
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

static void on_pan_drag (MrgEvent *e, void *data1, void *data2)
{
  static float zoom_pinch_coord[4][2] = {0,};
  static int   zoom_pinch = 0;
  static float orig_zoom = 1.0;

  GeState *o = data1;
  on_viewer_motion (e, data1, data2);
  if (e->type == MRG_DRAG_RELEASE)
  {
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
      o->is_fit = 0;
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

    o->renderer_state = TASK_BASE;
    queue_draw (o);
    mrg_event_stop_propagate (e);
  }
  drag_preview (e);
}


static void on_pick_drag (MrgEvent *e, void *data1, void *data2)
{
  static float zoom_pinch_coord[4][2] = {0,};
  static int   zoom_pinch = 0;
  static float orig_zoom = 1.0;

  GeState *o = data1;
  on_viewer_motion (e, data1, data2);
  if (e->type == MRG_DRAG_RELEASE)
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
      o->is_fit = 0;
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
  drag_preview (e);
}



static GeglNode *add_output (GeState *o, GeglNode *active, const char *optype);
static GeglNode *add_aux (GeState *o, GeglNode *active, const char *optype);
static GeglNode *add_input (GeState *o, GeglNode *active, const char *optype);

GeglPath *path = NULL;

static void on_paint_drag (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
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
      gegl_node_set (o->active, "d", path, "color", gegl_color_new(o->paint_color),
                     "width", 16.0 / o->scale, NULL);
      rev_inc (o);
      break;
    case MRG_DRAG_MOTION:
      gegl_path_append (path, 'L', x, y);
      //rev_inc (o); XXX : maybe enable if it doesnt interfere with painting
      break;
    case MRG_DRAG_RELEASE:
      o->active = gegl_node_get_ui_consumer (o->active, "output", NULL);
      break;
  }
  rev_inc (o);
  mrg_event_stop_propagate (e);
  //drag_preview (e);
}


static void on_move_drag (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
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

        /* toggle animation of x and y off and back on with new value */
        /* need api to manipulation animation by key instead of only current key */

      }
      break;
    case MRG_DRAG_RELEASE:
      {
        GeglNode *iter = o->active;
        GeglNode *last = iter;
        while (iter)
        {
          iter = gegl_node_get_ui_producer (iter, "input", NULL);
          if (iter) last = iter;
        }
        o->active = last;
      }
      break;
  }
  rev_inc (o);
  mrg_event_stop_propagate (e);
}


#if 0
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


static void set_edited_prop (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = global_state;
  GParamSpec *pspec;
  o->property_focus = g_intern_string (data2);
  o->editing_property = 0;

  pspec = gegl_node_find_property (o->active, o->property_focus);

  o->editing_buf[0]=0;

  if (pspec)
  {
      if (g_type_is_a (pspec->value_type, G_TYPE_DOUBLE))
      {
        double value;
        gegl_node_get (o->active, o->property_focus, &value, NULL);
        snprintf (o->editing_buf, sizeof (o->editing_buf), "%.3f", value);
        o->editing_property = 1;
      }
      else if (g_type_is_a (pspec->value_type, G_TYPE_INT))
      {
        int value;
        gegl_node_get (o->active, o->property_focus, &value, NULL);
        snprintf (o->editing_buf, sizeof (o->editing_buf), "%i", value);
        o->editing_property = 1;
      }
      else if (g_type_is_a (pspec->value_type, G_TYPE_BOOLEAN))
      {
        gboolean value;
        gegl_node_get (o->active, o->property_focus, &value, NULL);
        value = !value;
        gegl_node_set (o->active, o->property_focus, value, NULL);
        o->editing_property = 0;
      }
      else if (g_type_is_a (pspec->value_type, G_TYPE_STRING) ||
               g_type_is_a (pspec->value_type, GEGL_TYPE_PARAM_FILE_PATH))
      {
        char *value;
        gegl_node_get (o->active, o->property_focus, &value, NULL);
        snprintf (o->editing_buf, sizeof (o->editing_buf), "%s", value);
        g_free (value);
        o->editing_property = 1;
      }
      else if (g_type_is_a (pspec->value_type, GEGL_TYPE_COLOR))
      {
        char *value;
        GeglColor *color;
        gegl_node_get (o->active, o->property_focus, &color, NULL);
        g_object_get (color, "string", &value, NULL);
        snprintf (o->editing_buf, sizeof (o->editing_buf), "%s", value);
        o->editing_property = 1;
      }
  }

  if (e)
    mrg_event_stop_propagate (e);

  mrg_set_cursor_pos (o->mrg, 0);
  mrg_queue_draw (o->mrg, NULL);
}

static void cancel_edited_prop (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = global_state;

  o->editing_property = 0;
  o->editing_buf[0]=0;
  mrg_event_stop_propagate (e);
  mrg_set_cursor_pos (e->mrg, 0);
  mrg_queue_draw (e->mrg, NULL);
}

static void unset_edited_prop (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = global_state;
  if ((e->type == MRG_RELEASE) ||
      (e->type == MRG_MOTION))
  {
    mrg_event_stop_propagate (e);
    return;
  }

  {
    GParamSpec *pspec = gegl_node_find_property (o->active, o->property_focus);
    if (pspec)
    {
      if (g_type_is_a (pspec->value_type, G_TYPE_DOUBLE))
      {
        gegl_node_set (o->active, o->property_focus, strtod (o->editing_buf, NULL), NULL);
      }
      else if (g_type_is_a (pspec->value_type, G_TYPE_INT))
      {
        gegl_node_set (o->active, o->property_focus, atoi (o->editing_buf), NULL);
      }
      else if (g_type_is_a (pspec->value_type, G_TYPE_STRING) ||
               g_type_is_a (pspec->value_type, GEGL_TYPE_PARAM_FILE_PATH))
      {
        gegl_node_set (o->active, o->property_focus, o->editing_buf, NULL);
      }
      else if (g_type_is_a (pspec->value_type, GEGL_TYPE_COLOR))
      {
        char buf[2048];
        sprintf (buf, "%s='%s'",  o->property_focus, o->editing_buf);
        ui_run_command (NULL, buf, NULL);
      }
    }
    rev_inc (o);
  }

  cancel_edited_prop (e, NULL, NULL);
}




static void queue_thumb (const char *path, const char *thumbpath)
{
  ThumbQueueItem *item;
  for (MrgList *l = thumb_queue; l; l=l->next)
  {
    item = l->data;
    if (!strcmp (item->path, path))
    {
      return;
    }
    if (!strcmp (item->thumbpath, thumbpath))
    {
      return;
    }
  }
  item = g_malloc0 (sizeof (ThumbQueueItem));
  item->path = g_strdup (path);
  item->thumbpath = g_strdup (thumbpath);
  mrg_list_append (&thumb_queue, item);
}

void ui_queue_thumb (const char *path)
{
  char *thumb_path = ui_get_thumb_path (path);
  queue_thumb (path, thumb_path);
  g_free (thumb_path);
}


//static char commandline[1024] = {0,};
static int  completion_no = -1;


static void scroll_cb (MrgEvent *event, void *data1, void *data2);


static void draw_edit (Mrg *mrg, float x, float y, float w, float h)
{
  cairo_t *cr = mrg_cr (mrg);
  cairo_new_path (cr);
  cairo_arc (cr, x+0.5*w, y+0.5*h, h * .4, 0.0, G_PI * 2);
}

static int deferred_redraw_action (Mrg *mrg, void *data)
{
  mrg_queue_draw (mrg, NULL);
  return 0;
}

static inline void deferred_redraw (Mrg *mrg, MrgRectangle *rect)
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



static void canvas_touch_handling (Mrg *mrg, GeState *o)
{
  cairo_new_path (mrg_cr (mrg));
  cairo_rectangle (mrg_cr (mrg), 0,0, mrg_width(mrg), mrg_height(mrg));
  switch (tool)
  {
    case TOOL_PAN:
    default:
      mrg_listen (mrg, MRG_DRAG, on_pan_drag, o, NULL);
      mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      break;
    case TOOL_PICK:
      mrg_listen (mrg, MRG_DRAG, on_pick_drag, o, NULL);
      mrg_listen (mrg, MRG_MOTION, on_viewer_motion, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      break;
    case TOOL_PAINT:
      mrg_listen (mrg, MRG_DRAG, on_paint_drag, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      break;
    case TOOL_MOVE:
      mrg_listen (mrg, MRG_DRAG, on_move_drag, o, NULL);
      mrg_listen (mrg, MRG_SCROLL, scroll_cb, o, NULL);
      break;
  }
  cairo_new_path (mrg_cr (mrg));
}

static GeglNode *add_aux (GeState *o, GeglNode *active, const char *optype)
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
  return ret;
}

static GeglNode *add_input (GeState *o, GeglNode *active, const char *optype)
{
  GeglNode *ref = active;
  GeglNode *ret = NULL;
  GeglNode *producer;
  if (!gegl_node_has_pad (ref, "input"))
    return NULL;
  ret = gegl_node_new_child (o->gegl, "operation", optype, NULL);
  producer = gegl_node_get_producer (ref, "input", NULL);
  if (producer)
  {
    gegl_node_link_many (producer, ret, NULL);
  }
  gegl_node_connect_to (ret, "output", ref, "input");
  return ret;
}

static GeglNode *add_output (GeState *o, GeglNode *active, const char *optype)
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
  return ret;
}


static void prop_set_enum (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = global_state;
  int value = GPOINTER_TO_INT (data1);
  const char *prop_name = data2;

  gegl_node_set (o->active, prop_name, value, NULL);
  o->property_focus = g_intern_string (prop_name);

  rev_inc (o);
  mrg_event_stop_propagate (event);
}

static void set_int (MrgEvent *event, void *data1, void *data2)
{
  int *intptr = data1;
  int value = GPOINTER_TO_INT(data2);
  *intptr = value;
  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
}

static void set_string (MrgEvent *event, void *data1, void *data2)
{
  char **charptr = data1;
  char *value = data2;
  *charptr = value;
  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
}

static void set_string_b (MrgEvent *event, void *data1, void *data2)
{
  char *value = data2;
  char command[1024];
  sprintf (command, "op=%s", value);
  ui_run_command (NULL, command, NULL);


  set_string (event, data1, data2);
}

static int strsort (const void *a, const void *b)
{
  return strcmp(a,b);
}

#define INDENT_STR "   "

static GList *
gegl_operations_build (GList *list, GType type)
{
  GeglOperationClass *klass;
  GType *ops;
  guint  children;
  gint   no;

  if (!type)
    return list;

  klass = g_type_class_ref (type);
  if (klass->name != NULL)
    list = g_list_prepend (list, klass);

  ops = g_type_children (type, &children);

  for (no=0; no<children; no++)
    {
      list = gegl_operations_build (list, ops[no]);
    }
  if (ops)
    g_free (ops);
  return list;
}

static gint compare_operation_names (gconstpointer a,
                                     gconstpointer b)
{
  const GeglOperationClass *klassA, *klassB;

  klassA = a;
  klassB = b;

  return strcmp (klassA->name, klassB->name);
}

static GList *categories = NULL;

static GList *gegl_operations (void)
{
  static GList *operations = NULL;
  if (!operations)
    {
      GHashTable *categories_ht = NULL;
      operations = gegl_operations_build (NULL, GEGL_TYPE_OPERATION);
      operations = g_list_sort (operations, compare_operation_names);

      categories_ht = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

    for (GList *iter=operations;iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      const char *categoris = gegl_operation_class_get_key (klass, "categories");
#if 0
      g_hash_table_insert (categories_ht, (void*)g_intern_string (
        g_type_name (g_type_parent(G_OBJECT_CLASS_TYPE(klass)))
                              ), (void*)0xff);
#endif
     if (categoris)
      {
        const gchar *ptr = categoris;

        while (ptr && *ptr)
          {
            gchar category[64]="";
            gint i=0;
            while (*ptr && *ptr!=':' && i<63)
              {
                category[i++]=*(ptr++);
                category[i]='\0';
              }
            if (*ptr==':')
              ptr++;
            {
              g_hash_table_insert (categories_ht, (void*)g_intern_string (category), (void*)0xff);
            }
          }
      }
    }

    {
      GList *temp = g_hash_table_get_keys (categories_ht);
      for (GList *iter = temp; iter; iter = iter->next)
        categories = g_list_insert_sorted (categories, iter->data, strsort);
    }
    g_hash_table_destroy (categories_ht);
   }
  return operations;
}

static void draw_key (GeState *o, Mrg *mrg, const char *key)
{
  mrg_start (mrg, "div.propname", NULL);
  mrg_printf (mrg, "%s", key);
  mrg_end (mrg);
}

static void draw_value (GeState *o, Mrg *mrg, const char *value)
{
  mrg_start (mrg, "div.propvalue", NULL);
  mrg_printf (mrg, "%s", value);
  mrg_end (mrg);
}

static void draw_key_value (GeState *o, Mrg *mrg, const char *key, const char *value)
{
  mrg_start (mrg, "div.property", NULL);
  draw_key (o, mrg, key);
  draw_value (o, mrg, value);
  mrg_end (mrg);
}

static void
draw_property_enum (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
    GEnumClass *eclass = g_type_class_peek (pspec->value_type);
    gint value;

    mrg_start (mrg, "div.property", NULL);//

    gegl_node_get (node, pspec->name, &value, NULL);

    draw_key (o, mrg, pspec->name);

    mrg_start (mrg, "div.propvalue", NULL);//
    for (int j = eclass->minimum; j<= eclass->maximum; j++)
    {
      GEnumValue *evalue2 = &eclass->values[j];


      if (evalue2->value == value)
        mrg_start (mrg, "span.propvalue-enum-selected", NULL);//
      else
        mrg_start (mrg, "span.propvalue-enum", NULL);//

      mrg_text_listen (mrg, MRG_CLICK,
         prop_set_enum, GINT_TO_POINTER(evalue2->value), (void*)g_intern_string ((void*)pspec->name));
      mrg_printf (mrg, "%s ", evalue2->value_nick);
      mrg_text_listen_done (mrg);
      mrg_end (mrg);
    }

    //mrg_printf (mrg, "%s", evalue->value_nick);
    mrg_end (mrg);
    mrg_end (mrg);
}

/***************************************************************************/


typedef struct PropIntDragData {
  GeglNode   *node;
  const GParamSpec *pspec;
  float       width;
  float       height;
  float       x;
  float       y;

  double      ui_min;
  double      ui_max;
  int         min;
  int         max;
  double      ui_gamma;

  int         value;
} PropIntDragData;

static void on_prop_int_drag (MrgEvent *e, void *data1, void *data2)
{
  PropIntDragData *drag_data = data1;
  GeState *o = data2;

  gdouble rel_pos;
  gint    value;

  rel_pos = (e->x - drag_data->x) / drag_data->width;
  rel_pos = pow(rel_pos, drag_data->ui_gamma);
  value = rel_pos * (drag_data->ui_max-drag_data->ui_min) + drag_data->ui_min;

  gegl_node_set (drag_data->node, drag_data->pspec->name, value, NULL);

  o->property_focus = g_intern_string (drag_data->pspec->name);
  mrg_event_stop_propagate (e);
  rev_inc (o);
}

static void update_string (const char *new_text, void *data)
{
  char *str = data;
  strcpy (str, new_text);
}

#ifdef HAVE_LUA
static void update_string2 (const char *new_text, void *data)
{
  char **str = data;
  g_free (*str);
  *str = g_strdup (new_text);
}
#endif

static void
draw_property_int (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  cairo_t*cr = mrg_cr (mrg);

  MrgStyle *style;
  PropIntDragData *drag_data = g_malloc0 (sizeof (PropIntDragData));

  mrg_start (mrg, "div.property", NULL);//
  gegl_node_get (node, pspec->name, &drag_data->value, NULL);
  style = mrg_style (mrg);

  drag_data->node  = node;
  drag_data->pspec = pspec;
  drag_data->x = mrg_x (mrg);
  drag_data->y = mrg_y (mrg);

  drag_data->width = style->width;
  drag_data->height = mrg_em (mrg) * 2;



  drag_data->min = G_PARAM_SPEC_INT (drag_data->pspec)->minimum;
  drag_data->ui_min = drag_data->min;
  drag_data->max = G_PARAM_SPEC_INT (drag_data->pspec)->maximum;
  drag_data->ui_max = drag_data->max;

  if (GEGL_IS_PARAM_SPEC_INT (drag_data->pspec))
  {
    GeglParamSpecInt *gspec = (void*)drag_data->pspec;
    drag_data->ui_min   = gspec->ui_minimum;
    drag_data->ui_max   = gspec->ui_maximum;
    drag_data->ui_gamma = gspec->ui_gamma;

    if (drag_data->value > drag_data->ui_max)
      drag_data->ui_max = drag_data->value;
    if (drag_data->value < drag_data->ui_min)
      drag_data->ui_min = drag_data->value;
  }
  else
  {
    drag_data->ui_gamma = 1.0;
  }


  cairo_new_path (cr);
  cairo_rectangle (cr, drag_data->x, drag_data->y, drag_data->width, drag_data->height);
  mrg_listen_full (mrg, MRG_DRAG, on_prop_int_drag, drag_data, o, (void*)g_free, NULL);
  cairo_new_path (cr);

  {
    char *value = g_strdup_printf ("%i", drag_data->value);
    mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, NULL, (void*)pspec->name);
    draw_key (o, mrg, pspec->name);
    mrg_text_listen_done (mrg);
    if (o->editing_property && o->property_focus == pspec->name)
    {
      mrg_edit_start (mrg, update_string, &o->editing_buf[0]);
      mrg_printf_xml (mrg, "<div class='propvalue'>%s</div>", o->editing_buf);
      mrg_edit_end (mrg);
      mrg_add_binding (mrg, "return", NULL, "confirm edit", unset_edited_prop, o);
      mrg_add_binding (mrg, "escape", NULL, "cancel property editing", cancel_edited_prop, o);
    }
    else
    {
      mrg_printf_xml (mrg, "<div class='propvalue'>%s</div>", value);
    }
    g_free (value);
  }

  cairo_rectangle (cr,
   drag_data->x,
   drag_data->y,
   pow((drag_data->value-drag_data->ui_min) /
                      (drag_data->ui_max-drag_data->ui_min), 1.0/drag_data->ui_gamma)* drag_data->width,
   drag_data->height);
  cairo_set_source_rgba (cr,1,1,1, .5);
  cairo_fill (cr);

  cairo_new_path (cr);
  cairo_rectangle (cr, drag_data->x, drag_data->y, drag_data->width, drag_data->height);
  cairo_set_source_rgba (cr,1,1,1, .5);
  cairo_set_line_width (cr, 2);
  cairo_stroke (cr);

  mrg_set_xy (mrg, drag_data->x, drag_data->y + drag_data->height);

  mrg_end (mrg);
}

/***************************************************************************/

static void on_toggle_boolean (MrgEvent *e, void *data1, void *data2)
{
  GeglNode *node = data1;
  const char *propname = data2;
  gboolean value;
  gegl_node_get (node, propname, &value, NULL);
  value = value ? FALSE : TRUE;
  gegl_node_set (node, propname, value, NULL);
  global_state->property_focus = g_intern_string (propname);
  rev_inc (global_state);
  mrg_event_stop_propagate (e);
}

static void
draw_property_boolean (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  gboolean value = FALSE;
  mrg_start (mrg, "div.property", NULL);
  gegl_node_get (node, pspec->name, &value, NULL);

  mrg_text_listen (mrg, MRG_CLICK, on_toggle_boolean, node, (void*)pspec->name);

  draw_key (o, mrg, pspec->name);
  draw_value (o, mrg, value?"true":"false");

  mrg_text_listen_done (mrg);
  mrg_end (mrg);
}

/***************************************************************************/

typedef struct PropDoubleDragData {
  GeglNode   *node;
  const GParamSpec *pspec;
  float       width;
  float       height;
  float       x;
  float       y;

  double      ui_min;
  double      ui_max;
  double      min;
  double      max;
  double      ui_gamma;

  gdouble     value;
} PropDoubleDragData;

static void on_prop_double_drag (MrgEvent *e, void *data1, void *data2)
{
  PropDoubleDragData *drag_data = data1;
  GeState *o = data2;

  gdouble value, rel_pos;

  rel_pos = (e->x - drag_data->x) / drag_data->width;
  rel_pos = pow(rel_pos, drag_data->ui_gamma);
  value = rel_pos * (drag_data->ui_max-drag_data->ui_min) + drag_data->ui_min;

  o->property_focus = g_intern_string (drag_data->pspec->name);

  gegl_node_set (drag_data->node, drag_data->pspec->name, value, NULL);

  mrg_event_stop_propagate (e);
  rev_inc (o);
}


static void
draw_property_double (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  cairo_t*cr = mrg_cr (mrg);

  MrgStyle *style;
  PropDoubleDragData *drag_data = g_malloc0 (sizeof (PropDoubleDragData));

  mrg_start (mrg, "div.property", NULL);//
  gegl_node_get (node, pspec->name, &drag_data->value, NULL);
  style = mrg_style (mrg);

  drag_data->node  = node;
  drag_data->pspec = pspec;
  drag_data->x = mrg_x (mrg);
  drag_data->y = mrg_y (mrg);

  drag_data->width = style->width;
  drag_data->height = mrg_em (mrg) * 2;

  drag_data->min = G_PARAM_SPEC_DOUBLE (drag_data->pspec)->minimum;
  drag_data->ui_min = drag_data->min;
  drag_data->max = G_PARAM_SPEC_DOUBLE (drag_data->pspec)->maximum;
  drag_data->ui_max = drag_data->max;

  if (GEGL_IS_PARAM_SPEC_DOUBLE (drag_data->pspec))
  {
    GeglParamSpecDouble *gspec = (void*)drag_data->pspec;
    drag_data->ui_min   = gspec->ui_minimum;
    drag_data->ui_max   = gspec->ui_maximum;
    drag_data->ui_gamma = gspec->ui_gamma;

    if (drag_data->value > drag_data->ui_max)
      drag_data->ui_max = drag_data->value;
    if (drag_data->value < drag_data->ui_min)
      drag_data->ui_min = drag_data->value;
  }
  else
  {
    drag_data->ui_gamma = 1.0;
  }


  cairo_new_path (cr);
  cairo_rectangle (cr, drag_data->x, drag_data->y, drag_data->width, drag_data->height);
  mrg_listen_full (mrg, MRG_DRAG, on_prop_double_drag, drag_data, o, (void*)g_free, NULL);
  cairo_new_path (cr);

  {
    char *value = g_strdup_printf ("%.3f", drag_data->value);
    mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, NULL, (void*)pspec->name);
    draw_key (o, mrg, pspec->name);
    mrg_text_listen_done (mrg);
    if (o->editing_property && o->property_focus == pspec->name)
    {
      mrg_edit_start (mrg, update_string, &o->editing_buf[0]);
      mrg_printf_xml (mrg, "<div class='propvalue'>%s</div>", o->editing_buf);
      mrg_edit_end (mrg);
      mrg_add_binding (mrg, "return", NULL, "confirm edit", unset_edited_prop, o);
      mrg_add_binding (mrg, "escape", NULL, "cancel property editing", cancel_edited_prop, o);
    }
    else
    {
      mrg_printf_xml (mrg, "<div class='propvalue'>%s</div>", value);
    }
    g_free (value);
  }

  cairo_rectangle (cr,
   drag_data->x,
   drag_data->y,
   pow((drag_data->value-drag_data->ui_min) /
                      (drag_data->ui_max-drag_data->ui_min), 1.0/drag_data->ui_gamma)* drag_data->width,
   drag_data->height);
  cairo_set_source_rgba (cr,1,1,1, .5);
  cairo_fill (cr);

  cairo_new_path (cr);
  cairo_rectangle (cr, drag_data->x, drag_data->y, drag_data->width, drag_data->height);
  cairo_set_source_rgba (cr,1,1,1, .5);
  cairo_set_line_width (cr, 2);
  cairo_stroke (cr);

  mrg_set_xy (mrg, drag_data->x, drag_data->y + drag_data->height);

  mrg_end (mrg);
}

/***************************************************************************/


static void
draw_property_color (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  GeglColor *color;
  char *value = NULL;
  mrg_start (mrg, "div.property", NULL);//
  gegl_node_get (node, pspec->name, &color, NULL);
  g_object_get (color, "string", &value, NULL);

  if (o->editing_property && o->property_focus == pspec->name)
  {
    draw_key (o, mrg, pspec->name);
    mrg_text_listen (mrg, MRG_CLICK, unset_edited_prop, node, (void*)pspec->name);
    mrg_edit_start (mrg, update_string, &o->editing_buf[0]);

    draw_value (o, mrg, o->editing_buf);

    mrg_edit_end (mrg);
    mrg_add_binding (mrg, "return", NULL, "confirm editing", unset_edited_prop, o);
    mrg_add_binding (mrg, "escape", NULL, "cancel property editing", cancel_edited_prop, o);
    mrg_text_listen_done (mrg);
  }
  else
  {
    mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, NULL, (void*)pspec->name);
    draw_key (o, mrg, pspec->name);
    draw_value (o, mrg, value);
    mrg_text_listen_done (mrg);
  }

  if (value)
    g_free (value);
  mrg_end (mrg);
}

/**************************************************************************/

static void
draw_property_string (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  char *value = NULL;
  float x, y;
  float x_final, y_final;
  mrg_start (mrg, "div.property", NULL);//
  gegl_node_get (node, pspec->name, &value, NULL);

  if (o->editing_property && (o->property_focus == g_intern_string (pspec->name)))
  {
    draw_key (o, mrg, pspec->name);
    x = mrg_x (mrg);
    y = mrg_y (mrg);
    draw_value (o, mrg, o->editing_buf);
    x_final = mrg_x (mrg);
    y_final = mrg_y (mrg);
  }
  else
  {
    mrg_text_listen (mrg, MRG_CLICK, set_edited_prop, NULL, (void*)pspec->name);
    draw_key (o, mrg, pspec->name);
    mrg_start_with_style (mrg, "div.propvalue", NULL, "color:transparent;");
    x = mrg_x (mrg);
    y = mrg_y (mrg);
    mrg_printf (mrg, "%s", value);
    mrg_end (mrg);

    x_final = mrg_x (mrg);
    y_final = mrg_y (mrg);

    mrg_text_listen_done (mrg);
  }

  mrg_end (mrg);

  /*  XXX : hack redrawing the string property, in-case of multi-line and triggering
   *        the mrg background overdraw bug
   */
  mrg_set_xy (mrg, x, y);
  mrg_set_style (mrg, "color:yellow");

  if (o->editing_property && (o->property_focus == g_intern_string (pspec->name)))
  {
    const char *multiline = gegl_operation_get_property_key (gegl_node_get_operation(o->active), pspec->name, "multiline");
    mrg_edit_start (mrg, update_string, &o->editing_buf[0]);
    x = mrg_x (mrg);
    y = mrg_y (mrg);
    draw_value (o, mrg, o->editing_buf);
    mrg_edit_end (mrg);

    if (!multiline)
    {
      mrg_add_binding (mrg, "return", NULL, "complete editing", unset_edited_prop, o);
      mrg_add_binding (mrg, "escape", NULL, "cancel property editing", cancel_edited_prop, o);
    }
  }
  else
  {
    mrg_printf (mrg, "%s", value);
  }

  mrg_set_xy (mrg, x_final, y_final);

  if (value)
    g_free (value);
}

/**************************************************************************/

static void
draw_property_focus_box (GeState *o, Mrg *mrg)
{
  /* an overlining with slight curve hack - for now - should make use of CSS  */
  cairo_t *cr = mrg_cr (mrg);
  cairo_save (cr);
  cairo_new_path (cr);
  cairo_move_to (cr, mrg_x (mrg), mrg_y (mrg));
  cairo_rel_line_to (cr, mrg_style (mrg)->width, 0);
  cairo_rel_line_to (cr, -mrg_style (mrg)->width - mrg_em(mrg) * .25, 0);
  cairo_rel_line_to (cr, 0, mrg_em (mrg));
  cairo_set_source_rgb (cr, 1,1,1);
  cairo_stroke (cr);
  cairo_restore (cr);
}

static void
draw_property (GeState *o, Mrg *mrg, GeglNode *node, const GParamSpec *pspec)
{
  gboolean focused_property = g_intern_string (pspec->name) == o->property_focus;

  if (focused_property)
  {
    draw_property_focus_box (o, mrg);
  }

  if (g_type_is_a (pspec->value_type, G_TYPE_DOUBLE))
  {
    draw_property_double (o, mrg, node, pspec);
  }
  else if (g_type_is_a (pspec->value_type, G_TYPE_INT))
  {
    draw_property_int (o, mrg, node, pspec);
  }
  else if (g_type_is_a (pspec->value_type, G_TYPE_STRING) ||
           g_type_is_a (pspec->value_type, GEGL_TYPE_PARAM_FILE_PATH))
  {
    draw_property_string (o, mrg, node, pspec);
  }
  else if (g_type_is_a (pspec->value_type, GEGL_TYPE_COLOR))
  {
    draw_property_color (o, mrg, node, pspec);
  }
  else if (g_type_is_a (pspec->value_type, G_TYPE_BOOLEAN))
  {
    draw_property_boolean (o, mrg, node, pspec);
  }
  else if (g_type_is_a (pspec->value_type, G_TYPE_ENUM))
  {
    draw_property_enum (o, mrg, node, pspec);
  }
  else
  {
    mrg_start (mrg, "div.property", NULL);//
    draw_key (o, mrg, pspec->name);
    mrg_end (mrg);
  }

}

static float properties_height = 100;

static void list_node_props (GeState *o, GeglNode *node, int indent)
{
  static int operation_selector = 0;
  Mrg *mrg = o->mrg;
  guint n_props;
  const char *op_name;
  GParamSpec **pspecs;

  if (!node)
    return;

  op_name = gegl_node_get_operation (node);
  if (!op_name)
    return;
  pspecs = gegl_operation_list_properties (op_name, &n_props);

  mrg_start (mrg, "div.properties", NULL);

  if (o->property_focus == g_intern_string ("operation"))
    draw_property_focus_box (o, mrg);

  mrg_text_listen (mrg, MRG_CLICK, set_int, &operation_selector,
                   GINT_TO_POINTER(1));
  draw_key_value (o, mrg, "operation", op_name);
  mrg_text_listen_done (mrg);

  {
    const char *id = g_object_get_data (G_OBJECT (node), "refname");
    if (id)
    {
      if (o->property_focus == g_intern_string ("id"))
        draw_property_focus_box (o, mrg);
      draw_key_value (o, mrg, "id", id);
    }
  }

  if (pspecs)
  {
    for (gint i = 0; i < n_props; i++)
    {
      draw_property (o, mrg, node, pspecs[i]);
    }
    g_free (pspecs);
  }
  properties_height = mrg_y (mrg) + mrg_em (mrg);

  mrg_end (mrg);
  if (operation_selector)
  {
     static char *active_category = NULL;
     static char *active_operation = NULL;

     {
       static char *prev_category = NULL;
       if (prev_category != active_category)
         active_operation = NULL;

       prev_category = active_category;
     }

     mrg_start (mrg, "div.operation-selector", NULL);

     mrg_start (mrg, "div.operation-selector-close", NULL);
     mrg_text_listen (mrg, MRG_CLICK, set_int, &operation_selector, GINT_TO_POINTER(0));
     mrg_print (mrg, "[ X ]\n");
     mrg_text_listen_done (mrg);
     mrg_end (mrg);

    if(0){
    char ** operations = operations;
    gint i;
    guint n_operations;
    /* the last fragment is what we're completing */
    operations = gegl_list_operations (&n_operations);

    for (i = 0; i < n_operations; i++)
      {
        mrg_start (mrg, "div.operation-selector-op", NULL);
        mrg_printf (mrg, "%s", operations[i]);
        mrg_end (mrg);
      }

     g_free (operations);
    }
    {
      mrg_start (mrg, "div.operation-selector-categories", NULL);
      for (GList *iter = categories; iter; iter = iter->next)
      {
        if (iter->data == active_category)
          mrg_start (mrg, "div.operation-selector-category-active", NULL);
        else
          mrg_start (mrg, "div.operation-selector-category", NULL);
        mrg_text_listen (mrg, MRG_CLICK, set_string, &active_category, iter->data);
        mrg_printf (mrg, "%s", iter->data);
        mrg_end (mrg);
      }
      mrg_end (mrg);
    }

    mrg_start (mrg, "div.operation-selector-operations", NULL);
    for (GList *iter=gegl_operations();iter;iter = g_list_next (iter))
    {
      GeglOperationClass *klass = iter->data;
      const char *categoris = gegl_operation_class_get_key (klass, "categories");
      const char *name = gegl_operation_class_get_key (klass, "name");

      if (active_category == NULL && categoris == NULL)
      {
        mrg_start (mrg, "div.operation-selector-op", NULL);
        mrg_printf (mrg, "%s", name);
        mrg_end (mrg);
      }

      if (active_category && categoris && strstr (categoris, active_category))
      {
        if (active_operation == g_intern_string (name))
          mrg_start (mrg, "div.operation-selector-op-active", NULL);
        else
          mrg_start (mrg, "div.operation-selector-op", NULL);
        {
          const char *displayed_name = name;
          if (g_str_has_prefix (name, "gegl:"))
            displayed_name = name + strlen ("gegl:");
        mrg_text_listen (mrg, MRG_CLICK, set_string_b, &active_operation, (void*)g_intern_string (name));
        mrg_printf (mrg, "%s", displayed_name);
        }
        mrg_text_listen_done (mrg);
        mrg_end (mrg);
      }
    }
    mrg_end (mrg);

    if (active_operation)
    {
      GList *iter;
      for (iter=gegl_operations();iter;iter = g_list_next (iter))
      {
        GeglOperationClass *klass = iter->data;
        const char *name = gegl_operation_class_get_key (klass, "name");
        name = g_intern_string (name);
        if (name == active_operation)
          break;
      }

      if (iter){
        GeglOperationClass *klass = iter->data;
        const char *name = gegl_operation_class_get_key (klass, "name");
        const char *description = gegl_operation_class_get_key (klass, "description");

        mrg_start (mrg, "div", NULL);
        mrg_end (mrg);
        mrg_start (mrg, "div.operation-selector-operation", NULL);
        mrg_printf (mrg, "%s", name);
        mrg_end (mrg);

        mrg_start (mrg, "div.operation-selector-operation", NULL);
        mrg_printf (mrg, "%s", description);
        mrg_end (mrg);

#if 0
       {
         GParamSpec **self;
         GParamSpec **parent;
         guint n_self;
         guint n_parent;
         gint prop_no;
         
         self = g_object_class_list_properties (
            G_OBJECT_CLASS(klass), //G_OBJECT_CLASS (g_type_class_ref (type)),
            &n_self);
          parent = g_object_class_list_properties (
            /*G_OBJECT_CLASS (g_type_class_peek_parent (g_type_class_ref (type))),*/
            G_OBJECT_CLASS (g_type_class_ref (GEGL_TYPE_OPERATION)),
            &n_parent);

  for (prop_no=0;prop_no<n_self;prop_no++)
    {
      gint parent_no;
      gboolean found=FALSE;
      for (parent_no=0;parent_no<n_parent;parent_no++)
        if (self[prop_no]==parent[parent_no])
          found=TRUE;
      /* only print properties if we are an addition compared to
       * GeglOperation
       */
      if (!found)
        {
          const gchar *type_name;
          mrg_printf_xml (mrg, "<div style='margin-top:0.5em;'><div style='padding-left:0.1em;border-left:0.3em solid black;margin-left:-0.35em;'>%s</div>\n",
             g_param_spec_get_nick (self[prop_no]));


          if (g_param_spec_get_blurb (self[prop_no]) &&
              g_param_spec_get_blurb (self[prop_no])[0]!='\0')
          {
            mrg_printf_xml (mrg, "<div>%s</div>", g_param_spec_get_blurb (self[prop_no]));
          }

          mrg_printf_xml (mrg, "<b>name:&nbsp;</b>%s\n<b>type:&nbsp;</b>", g_param_spec_get_name (self[prop_no]));

          type_name = g_type_name (G_OBJECT_TYPE (self[prop_no]));
          if(strstr (type_name, "Param"))
          {
            type_name = strstr (type_name, "Param");
            type_name+=5;
          }
          {
            for (const char *p = type_name; *p; p++)
              mrg_printf (mrg, "%c", g_ascii_tolower (*p));
          }
       }
     }
        if (klass->opencl_support)
          mrg_printf_xml (mrg, "<div style='color:white'>OpenCL</div>\n");
        if (klass->compat_name)
          mrg_printf_xml (mrg, "<div style='color:white'>alias: %s</div>\n", klass->compat_name);
      {
        guint nkeys;
        gchar **keys = gegl_operation_list_keys (name, &nkeys);

        if (keys)
          {
            for (gint i = 0; keys[i]; i++)
              {
                const gchar *value = gegl_operation_get_key (name, keys[i]);

                if (g_str_equal (keys[i], "categories") ||
                    g_str_equal (keys[i], "cl-source") ||
                    g_str_equal (keys[i], "name") ||
                    g_str_equal (keys[i], "source") ||
                    g_str_equal (keys[i], "reference-composition") ||
                    g_str_equal (keys[i], "reference-hash") ||
                    g_str_equal (keys[i], "title") ||
                    g_str_equal (keys[i], "description"))
                  continue;

                mrg_printf_xml (mrg, "<b>%s:</b>&nbsp;", keys[i]);
                mrg_printf (mrg, value);
                mrg_printf_xml (mrg, "<br/>\n");
              }
            g_free (keys);
          }
      }



   }
#endif

      }

    }


     mrg_end (mrg);
  }
}

/* invalidate_signal:
 * @node: The node that was invalidated.
 * @rect: The area that changed.
 * @SDL_Surface: Our user_data param, the window we will write to.
 *
 * Whenever the output of the graph changes this function will copy the new data
 * to the sdl window.
 */
static void
invalidate_signal (GeglNode *node, GeglRectangle *rect, void *userdata)
{
  GeState *o = userdata;
  queue_draw (o); // XXX : should queue only rect with mrg as well,
                  //       and only blit subrect in mrg-gegl integration for image

  // we could increment the revision, thought for .gif and video files
  // that increment would be invalid due to invalidations during playback
}


static void activate_sink_producer (GeState *o)
{
  if (o->sink)
    o->active = gegl_node_get_producer (o->sink, "input", NULL);
  else
    o->active = NULL;
  o->pad_active = PAD_OUTPUT;

  g_signal_connect (o->sink, "invalidated",
                    G_CALLBACK (invalidate_signal), o);
}


static void set_op (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;

  {
    if (strchr (o->editing_buf, ':'))
    {
      gegl_node_set (o->active, "operation", o->editing_buf, NULL);
    }
    else
    {
      char temp[1024];
      g_snprintf (temp, 1023, "gegl:%s", o->editing_buf);
      gegl_node_set (o->active, "operation", temp, NULL);

    }
  }

  o->editing_buf[0]=0;
  o->editing_op_name=0;
  mrg_event_stop_propagate (event);
  mrg_queue_draw (o->mrg, NULL);
}


static void update_ui_consumers_list (GeState *o, GeglNode *iter)
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


static void update_ui_consumers (GeState *o)
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
        *consumer_pad = g_intern_string (consumer_names[i]);
    g_free (nodes);
    g_free (consumer_names);
  }

  return ret;
}

static GeglNode *node_pad_drag_node = NULL;
static GeglNode *node_pad_drag_candidate = NULL;
static int node_pad_drag = -1;
static float node_pad_drag_x = 0;
static float node_pad_drag_y = 0;
static float node_pad_drag_x_start = 0;
static float node_pad_drag_y_start = 0;


#if 0
static void node_press (MrgEvent *e,
                        void *data1,
                        void *data2)
{
  GeState *o = data2;
  GeglNode *new_active = data1;

  o->active = new_active;
  o->pad_active = PAD_OUTPUT;
  mrg_event_stop_propagate (e);

  mrg_queue_draw (e->mrg, NULL);
}
#endif


static void on_graph_scroll (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;

  float x, y;
  float screen_cx = event->device_x;
  float screen_cy = event->device_y;

  x = (o->graph_pan_x + screen_cx) / o->graph_scale;
  y = (o->graph_pan_y + screen_cy) / o->graph_scale;

  switch (event->scroll_direction)
  {
     case MRG_SCROLL_DIRECTION_UP:
       o->graph_scale *= 1.1;
       break;
     case MRG_SCROLL_DIRECTION_DOWN:
       o->graph_scale /= 1.1;
       break;
     default:
       break;
  }

  o->graph_pan_x = x * o->graph_scale - screen_cx;
  o->graph_pan_y = y * o->graph_scale - screen_cy;

  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
}

static void on_graph_drag (MrgEvent *e, void *data1, void *data2)
{
  static float pinch_coord[4][2] = {0,};
  static int   pinch = 0;
  static float orig_zoom = 1.0;

  GeState *o = data1;
  GeglNode *node = data2;

  //on_viewer_motion (e, data1, data2);
  if (e->type == MRG_DRAG_RELEASE)
  {
    //float x = (e->x + o->u) / o->scale;
    //float y = (e->y + o->v) / o->scale;
    //GeglNode *picked = NULL;

    if(node && hypotf (e->device_x - e->start_x, e->device_y - e->start_y) < 10)
    {
      o->active = node;
      o->pad_active = PAD_OUTPUT;
    }

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

      x = (o->graph_pan_x + screen_cx) / o->graph_scale;
      y = (o->graph_pan_y + screen_cy) / o->graph_scale;

      o->graph_scale = orig_zoom * (dist / orig_dist);

      o->graph_pan_x = x * o->graph_scale - screen_cx;
      o->graph_pan_y = y * o->graph_scale - screen_cy;

      o->graph_pan_x -= (e->delta_x * o->graph_scale )/2; /* doing half contribution of motion per finger */
      o->graph_pan_y -= (e->delta_y * o->graph_scale )/2; /* is simple and roughly right */
    }

    }
    else
    {
      if (e->device_no == 1 || e->device_no == 4)
      {
        o->graph_pan_x -= (e->delta_x * o->graph_scale);
        o->graph_pan_y -= (e->delta_y * o->graph_scale);
      }
    }

    //o->renderer_state = 0;
    mrg_queue_draw (e->mrg, NULL);
  }
  mrg_event_stop_propagate (e);
  drag_preview (e);
}



static void on_active_node_drag (MrgEvent *e, void *data1, void *data2, int is_aux)
{
  GeState *o = data1;
  GeglNode *node = data2;

  float em = mrg_em (o->mrg);

  float dist_jitter   = em;
  float dist_add_node = em*2;
  float dist_remove   = em*3;
  float dist_connect_pad  = em*4;

  static float dist = 0;
  static float angle = 0;

  switch (node_pad_drag)
  {
    case -1:

  switch (e->type)
  {
    case MRG_DRAG_PRESS:
      dist = angle = 0.0;
      node_pad_drag_candidate = NULL;
      break;
    case MRG_DRAG_RELEASE:
      if (angle < -120 || angle > 120) // upwards
      {
         if (dist > dist_add_node)
         {
           o->active = add_output (o, o->active, "gegl:nop");
           rev_inc(o);
         }
      }
      else if (angle < 60 && angle > -45) // down
      {
         if (is_aux)
         {
           if (dist > dist_add_node)
           {
             o->active = add_aux (o, o->active, "gegl:nop");
             rev_inc(o);
           }
         }
         else
         {
           if (dist > dist_add_node)
           {
              o->active = add_input (o, o->active, "gegl:nop");
           }
           rev_inc(o);
         }
      }
      else if (angle < -45 && angle > -110) // left
      {
         if (dist > dist_remove)
         {
           o->pad_active = PAD_OUTPUT; // restore the output pad
           argvs_eval ("remove");
         }
      }
      break;
    case MRG_DRAG_MOTION:
      dist = hypot (e->start_x -e->x, e->start_y - e->y);
      angle = atan2(e->x - e->start_x, e->y - e->start_y) * 180.0 / M_PI;

      if (angle < -120 || angle > 120) // upwards
      {
         if (dist > dist_jitter)
           o->pad_active = PAD_OUTPUT;
      }
      else if (angle < 60 && angle > -45) // down
      {
         if (is_aux)
         {
           if (dist > dist_jitter)
             o->pad_active = PAD_AUX;

           if (dist > dist_connect_pad)
           {
             node_pad_drag = PAD_AUX;
             node_pad_drag_node = node;
           }
         }
         else
         {
           if (dist > dist_jitter)
             o->pad_active = PAD_INPUT;
           if (dist > dist_connect_pad)
           {
             node_pad_drag = PAD_INPUT;
             node_pad_drag_node = node;
           }
         }
      }
      else if (angle < -45 && angle > -110) // left
      {
         if (dist > dist_remove)
           o->pad_active = -1; // makes dot vanish
         else
           o->pad_active = PAD_OUTPUT;
      }
      else
      {
           //o->pad_active = PAD_OUTPUT;
      }

      //fprintf (stderr, "aa dist: %f  angle: %f \n", dist, angle);
      break;
    default:
      break;
  }
  break;
    case PAD_INPUT:
      switch (e->type)
       {
         case MRG_DRAG_PRESS:
           node_pad_drag_x = e->x;
           node_pad_drag_y = e->y;
           break;
         case MRG_DRAG_RELEASE:
           node_pad_drag = -1;

           if (node_pad_drag_candidate)
           {
              gegl_node_connect_to (node_pad_drag_candidate, "output", node_pad_drag_node, "input");
              rev_inc (o);
           }
           o->pad_active = PAD_OUTPUT;
           node_pad_drag_candidate = NULL;

           break;
         case MRG_DRAG_MOTION:
           node_pad_drag_x = e->x;
           node_pad_drag_y = e->y;
           break;
         default:
           break;
       }
      break;
    case PAD_AUX:
      switch (e->type)
       {
         case MRG_DRAG_PRESS:
           node_pad_drag_x = e->x;
           node_pad_drag_y = e->y;
           break;
         case MRG_DRAG_RELEASE:
           node_pad_drag = -1;

           if (node_pad_drag_candidate)
           {
              gegl_node_connect_to (node_pad_drag_candidate, "output", node_pad_drag_node, "aux");
              rev_inc (o);
           }
           o->pad_active = PAD_OUTPUT;
           node_pad_drag_candidate = NULL;

           break;
         case MRG_DRAG_MOTION:
           node_pad_drag_x = e->x;
           node_pad_drag_y = e->y;
           break;
         default:
           break;
       }
      break;
  }
  mrg_event_stop_propagate (e);
  mrg_queue_draw (e->mrg, NULL);
}


static void on_active_node_drag_input (MrgEvent *e, void *data1, void *data2)
{
  on_active_node_drag (e, data1, data2, 0);
}

static void on_active_node_drag_aux (MrgEvent *e, void *data1, void *data2)
{
  on_active_node_drag (e, data1, data2, 1);
}

typedef struct DrawEdge {
  int       has_alpha;
  int       color_components;
  int       bit_depth;

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

  {
    GeglOperation *operation = gegl_node_get_gegl_operation (source);
    const Babl *output_format = gegl_operation_get_format (operation, "output");

    if (!output_format && gegl_node_is_graph (source))
    {
      output_format = gegl_operation_get_format (gegl_node_get_gegl_operation(gegl_node_get_output_proxy (source, "output")), "output");
    }

    if (output_format)
      {
        const Babl *type = babl_format_get_type (output_format, 0);
        BablModelFlag flags = babl_get_model_flags (output_format);
        if ((flags & BABL_MODEL_FLAG_ALPHA) != 0)
          edge->has_alpha = 1;
        if ((flags & BABL_MODEL_FLAG_RGB) != 0)
          edge->color_components = 3;
        if ((flags & BABL_MODEL_FLAG_GRAY) != 0)
          edge->color_components = 1;
        if ((flags & BABL_MODEL_FLAG_CMYK) != 0)
          edge->color_components = 4;

        if (type == babl_type ("double"))
          edge->bit_depth=16;
        else if (type == babl_type ("float"))
          edge->bit_depth=8;
        else if (type == babl_type ("half"))
          edge->bit_depth=4;
        else if (type == babl_type ("u16"))
          edge->bit_depth=3;
        else if (type == babl_type ("u8"))
          edge->bit_depth=2;
        else
          edge->bit_depth=1;
      }
  }

  edge_queue = g_list_prepend (edge_queue, edge);
}


static float compute_node_x (Mrg *mrg, int indent, int line_no)
{
  float em = mrg_em (mrg);
  return (0.5 + 4 * indent) * em;
}
static float compute_node_y (Mrg *mrg, int indent, int line_no)
{
  float em = mrg_em (mrg);
  return (0 + line_no * 2.0) * em;
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
      return compute_node_y (mrg, indent, line_no) + 1.5 * em;
    case 2: // out
      return compute_node_y (mrg, indent, line_no) + 0.0 * em;
  }
  return 0;
}

static void
draw_node (GeState *o, int indent, int line_no, GeglNode *node, gboolean active)
{
  char *opname = NULL;
  GList *to_remove = NULL;
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  float x = compute_node_x (mrg, indent, line_no);
  float y = compute_node_y (mrg, indent, line_no);

  if (active){
    double xd=x, yd=y;
    cairo_user_to_device (mrg_cr(mrg), &xd, &yd);

    if (-o->graph_pan_x > mrg_width(mrg) - mrg_height (mrg) * font_size_scale * 25)
    {
      if (yd < properties_height ||
          yd > mrg_height (mrg) - mrg_em (mrg) * 12)
      {
        float blend_factor = 0.20;
        float new_scroll = ( (y*o->graph_scale) - properties_height - 12 * mrg_em (mrg));

        o->graph_pan_y = (1.0-blend_factor) * o->graph_pan_y +
                             blend_factor *  new_scroll;
        mrg_queue_draw (mrg, NULL);
      }
    }
    else
    {
      if (yd < mrg_em(mrg) * 3 ||
          yd > mrg_height (mrg) - mrg_em (mrg) * 3)
      {
        float blend_factor = 0.20;
        float new_scroll = ( (y*o->graph_scale) - 3 * mrg_em (mrg));

        o->graph_pan_y = (1.0-blend_factor) * o->graph_pan_y +
                              blend_factor *  new_scroll;
        mrg_queue_draw (mrg, NULL);
      }
    }
  }

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
    sprintf (style, "left:%f;top:%f;", x, y);
    if (active)
    mrg_start_with_style (mrg, "div.node-active", NULL, style);
    else
    mrg_start_with_style (mrg, "div.node", NULL, style);
  }

  if (active && o->editing_op_name)
  {
    mrg_edit_start (mrg, update_string, &o->editing_buf[0]);
    mrg_printf (mrg, "%s", o->editing_buf);
    mrg_edit_end (mrg);
    mrg_add_binding (mrg, "return", NULL, "set operation", set_op, o);
  }
  else
  {
    if (g_str_has_prefix (opname, "gegl:"))
      mrg_printf (mrg, "%s", opname+5);
    else
      mrg_printf (mrg, "%s", opname);
  }

   {
    MrgStyle *style = mrg_style (mrg);
    float x = style->left;
    float y = style->top - 0.5 * mrg_em (mrg);
    float width = style->width + style->padding_left + style->padding_right;
    float height = style->height + style->padding_top + style->padding_bottom + 1 * mrg_em (mrg);

    cairo_rectangle (mrg_cr (mrg), x, y, width, height);

    if (node_pad_drag >= 0 && cairo_in_fill (mrg_cr (mrg), node_pad_drag_x, node_pad_drag_y) &&
        node != node_pad_drag_node)
    {
       mrg_set_style (mrg, "border: 4px solid yellow;");
       node_pad_drag_candidate = node;
    }

    if(active)
    {
      if (gegl_node_has_pad (node, "aux"))
      {
        cairo_new_path (mrg_cr (mrg));
        cairo_rectangle (mrg_cr (mrg), x, y, width/2, height);
        mrg_listen (mrg, MRG_DRAG, on_active_node_drag_input, o, node);
        mrg_listen (mrg, MRG_SCROLL, on_graph_scroll, o, node);

        cairo_new_path (mrg_cr (mrg));
        cairo_rectangle (mrg_cr (mrg), x + width/2, y, width/2, height);
        mrg_listen (mrg, MRG_DRAG, on_active_node_drag_aux, o, node);
        mrg_listen (mrg, MRG_SCROLL, on_graph_scroll, o, node);
      }
      else
      {
        cairo_new_path (mrg_cr (mrg));
        cairo_rectangle (mrg_cr (mrg), x, y, width, height);
        mrg_listen (mrg, MRG_DRAG, on_active_node_drag_input, o, node);
        mrg_listen (mrg, MRG_SCROLL, on_graph_scroll, o, node);
      }
    }
    else
    {
      mrg_listen (mrg, MRG_DRAG, on_graph_drag, o, node);
      mrg_listen (mrg, MRG_SCROLL, on_graph_scroll, o, node);
    }
    cairo_new_path (mrg_cr (mrg));

  }

  mrg_end (mrg);
  g_free (opname);


  for (GList *i = edge_queue; i; i = i->next)
  {
    DrawEdge *edge = i->data;
    if (edge->source == node)
    {
      float width = 3.0f;
      float padding = 0.75f;
      float rgb[4] = {.9,.9,.9, 1.0};

      cairo_t *cr = mrg_cr (mrg);

      cairo_new_path (cr);
      cairo_move_to (cr, compute_pad_x (mrg, indent, line_no, 2),
                         compute_pad_y (mrg, indent, line_no, 2));
      cairo_line_to (cr, compute_pad_x (mrg, edge->indent, edge->line_no, edge->in_slot_no),
                         compute_pad_y (mrg, edge->indent, edge->line_no, edge->in_slot_no));
      width = edge->bit_depth;
      switch (edge->color_components)
      {
        case 1:
           rgb[0] = .6;
           rgb[1] = .6;
           rgb[2] = .6;
           break;
        case 3:
           rgb[0] = 1;
           rgb[1] = 0;
           rgb[2] = 0;
           break;
        case 4:
           rgb[0] = 0;
           rgb[1] = 1;
           rgb[2] = 1;
           break;
      }
      if (edge->has_alpha)
        rgb[3]=0.5;


      cairo_set_line_width (cr, width + padding);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_source_rgba (cr, .0,.0,.0,.5);
      cairo_stroke_preserve (cr);

      cairo_set_line_width (cr, width);
      cairo_set_source_rgba (cr, rgb[0], rgb[1], rgb[2], rgb[3]);
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

  if (node_pad_drag_node == node)
  {
    node_pad_drag_x_start =
       compute_pad_x (mrg, indent, line_no, node_pad_drag);
    node_pad_drag_y_start =
       compute_pad_y (mrg, indent, line_no, node_pad_drag);
  }

  for (int pad_no = PAD_INPUT; pad_no <= PAD_OUTPUT; pad_no++)
  {
    const char *pad_names[3]={"input", "aux", "output"};

    if (gegl_node_has_pad (node, pad_names[pad_no]))
    {
      gboolean is_active = active && o->pad_active == pad_no;

      cairo_new_path (cr);
      cairo_arc (cr, compute_pad_x (mrg, indent, line_no, pad_no),
                     compute_pad_y (mrg, indent, line_no, pad_no),
                     (is_active?active_pad_radius:pad_radius)*mrg_em (mrg),
                     0, G_PI * 2);

      cairo_set_line_width (cr, 1.0f);
      if (is_active) cairo_set_source_rgba (cr, active_pad_color);
      else           cairo_set_source_rgba (cr, pad_color);
      cairo_fill_preserve (cr);
      if (is_active) cairo_set_source_rgba (cr, active_pad_stroke_color);
      else           cairo_set_source_rgba (cr, pad_stroke_color);
      cairo_stroke (cr);
    }
  }

}

static void list_ops (GeState *o, GeglNode *iter, int indent, int *no)
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



static void draw_graph (GeState *o)
{
  Mrg *mrg = o->mrg;
  GeglNode *iter;
  int no = 0;

  mrg_start         (mrg, "div.graph", NULL);

  cairo_translate (mrg_cr (mrg), - o->graph_pan_x, -o->graph_pan_y);
  cairo_scale (mrg_cr (mrg), o->graph_scale, o->graph_scale);

  update_ui_consumers (o);

  iter = o->sink;

  /* skip nop-node */
  iter = gegl_node_get_producer (iter, "input", NULL);

  list_ops (o, iter, 0, &no);

  if (node_pad_drag >= 0)
  {
    cairo_t *cr = mrg_cr (mrg);
    cairo_new_path (cr);
    cairo_move_to (cr, node_pad_drag_x_start,
                          node_pad_drag_y_start);
    cairo_line_to (cr, node_pad_drag_x,
                          node_pad_drag_y);

    cairo_set_line_width (cr, 3 + .75);
    cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba (cr, .0,.0,.0,.5);
    cairo_stroke_preserve (cr);

    cairo_set_line_width (cr, 3);
    cairo_set_source_rgba (cr, 1, 0, 0, 1.0);
    cairo_stroke (cr);
  }

  mrg_end (mrg);

  if (o->active && !scrollback)
  {
    mrg_start         (mrg, "div.props", NULL);
    list_node_props (o, o->active, 1);
    mrg_end (mrg);
  }


  {
    cairo_t *cr = mrg_cr (mrg);
    float width = mrg_width (mrg);
    float height = mrg_height (mrg);
    draw_edit (mrg, width - height * .15, height * .0, height * .15, height *.15);

    if (o->show_controls)
      ui_contrasty_stroke (cr);
    else
      cairo_new_path (cr);
    cairo_rectangle (cr, width - height * .15, height * .0, height * .15, height *.15);
    if (o->show_controls && 0)
    {
      cairo_set_source_rgba (cr, 1,1,1,.1);
      cairo_fill_preserve (cr);
    }
    mrg_listen (mrg, MRG_PRESS, ui_run_command, "toggle editing", NULL);
    cairo_new_path (cr);
  }
}


static GList *commandline_get_completions (GeglNode   *node,
                                           const char *commandline);

static void update_commandline (const char *new_commandline, void *data)
{
  GeState *o = data;
  if (completion_no>=0)
  {
    char *appended = g_strdup (&new_commandline[strlen(o->commandline)]);
    GList *completions = commandline_get_completions (o->active,
                                                      o->commandline);

    const char *completion = g_list_nth_data (completions, completion_no);
    strcat (o->commandline, completion);
    strcat (o->commandline, appended);
    g_free (appended);
    mrg_set_cursor_pos (o->mrg, strlen (o->commandline));
    while (completions)
    {
      g_free (completions->data);
      completions = g_list_remove (completions, completions->data);
    }
  }
  else
  {
    strcpy (o->commandline, new_commandline);
  }
  completion_no = -1;
  mrg_queue_draw (o->mrg, NULL);
}

/* finds id in iterable subtree from iter
 */
static GeglNode *node_find_by_id (GeState *o, GeglNode *iter,
                                  const char *needle_id)
{
  needle_id = g_intern_string (needle_id);
  while (iter)
   {
     const char *id = g_object_get_data (G_OBJECT (iter), "refname");
     if (id == needle_id)
       return iter;

     if (gegl_node_get_producer (iter, "aux", NULL))
     {

     {
       GeglNode *producer = gegl_node_get_producer (iter, "aux", NULL);
       GeglNode *producers_consumer;
       const char *consumer_name = NULL;

       producers_consumer = gegl_node_get_ui_consumer (
                                           producer, "output", &consumer_name);

       if (producers_consumer == iter && !strcmp (consumer_name, "aux"))
         {
           GeglNode *ret = node_find_by_id (o,
                        gegl_node_get_producer (iter, "aux", NULL), needle_id);
           if (ret)
             return ret;
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
  return NULL;
}

char *get_item_path_no (GeState *o, int child_no)
{
  char *ret;
  if (o->is_dir)
    {
      char *basename = meta_get_child (o, o->path, child_no);
      ret = g_strdup_printf ("%s/%s", o->path, basename);
      g_free (basename);
    }
  else
    {
      char *dirname = g_path_get_dirname (o->path);
      char *basename = meta_get_child (o, dirname, child_no);
      ret = g_strdup_printf ("%s/%s", dirname, basename);
      g_free (dirname);
      g_free (basename);
    }
  return ret;
}


char *get_item_path (GeState *o)
{
  char *path = NULL;
  if (o->is_dir)
  {
    path = get_item_path_no (o, o->entry_no);
    if (g_file_test (path, G_FILE_TEST_IS_DIR))
    {
      g_free (path);
      path = NULL;
    }
  }
  else
  {
    path = g_strdup (o->path);
  }
  return path;
}

char *get_item_dir (GeState *o)
{
  char *path = NULL;
  if (o->is_dir)
  {
    path = g_strdup (o->path);
  }
  else
  {
    path = g_path_get_dirname (o->path);
  }
  return path;
}

int get_item_no (GeState *o)
{
  if (o->is_dir)
  {
  }
  else
  {
    int no = 0;
    if (o->entry_no <= 0)
    {
      char *basename = g_path_get_basename (o->path);
      o->entry_no = 0;
      for (GList *iter = o->index; iter && !o->entry_no; iter = iter->next, no++)
      {
        IndexItem *item = iter->data;
        if (!strcmp (item->name, basename))
          o->entry_no = no;
      }
      for (GList *iter = o->paths; iter && !o->entry_no; iter = iter->next, no++)
      {
        if (!strcmp (iter->data, o->path))
          o->entry_no = no;
      }
      g_free (basename);
    }
  }
  return o->entry_no;
}


void
ui_run_command (MrgEvent *event, void *data1, void *data_2)
{
  GeState *o = global_state;
  char *commandline = data1;

  gchar **argv = NULL;
  gint    argc = 0;

  while (commandline && commandline[strlen(commandline)-1]==' ')
    commandline[strlen(commandline)-1]='\0';

  g_shell_parse_argv (commandline, &argc, &argv, NULL);

  /* the commandline has two modes, operation/property mode and argvs command
   * running mode the mode is determined by the first arguement on a passed line
   * if the first word matches an existing argvs command, commandline running
   * mode is used, otherwise operation/property mode is used.
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


  if (strstr (*arg, "@") && o->is_dir)
  {
    char *path = get_item_path (o);
    char *key = g_strdup (*arg);
    char *value = strstr (key, "@") + 1;
    value[-1]='\0';

    meta_set_attribute (o, NULL, o->entry_no, key, value[0]==0?NULL:value);

    g_free (key);
    g_free (path);
  }
  else if (strchr (*arg, '='))
  {
    if (o->is_dir)
    {
      char *path = get_item_path (o);
      if (path)
      {
        char *key = g_strdup (*arg);
        char *value = strchr (key, '=') + 1;
        value[-1]='\0';

        meta_set_key (o, path, key, value[0]==0?NULL:value);

        g_free (key);
        g_free (path);
      }
    }
    else
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
           (int)g_ascii_strtod (value, NULL), NULL);
         else if (g_type_is_a (target_type, G_TYPE_UINT))
           gegl_node_set (o->active, key,
           (guint)g_ascii_strtod (value, NULL), NULL);
         else
           gegl_node_set (o->active, key,
                          g_ascii_strtod (value, NULL), NULL);
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
       if (!strcmp (key, "id"))
       {
         if (o->active)
         {
           GeglNode *existing = node_find_by_id (o, o->sink, value);

           if (existing)
             printf ("a node with id %s already exists\n", value);
           else
             g_object_set_data (G_OBJECT (o->active), "refname",
                                (void*)g_intern_string (value));
         }
       }
       else if (!strcmp (key, "ref"))
       {
          GeglNode *ref_node = node_find_by_id (o, o->sink, value);
          if (ref_node)
          {
            switch (o->pad_active)
            {
              case PAD_INPUT:
              case PAD_OUTPUT:
                gegl_node_link_many (ref_node, o->active, NULL);
                break;
              case PAD_AUX:
                gegl_node_connect_to (ref_node, "output", o->active, "aux");
                break;
            }
          }
          else
            printf ("no node with id=%s found\n", value);
       }
       else if (!strcmp (key, "op"))
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
        switch (o->pad_active)
        {
          case 0:
            argvs_eval ("node-add input");
            gegl_node_set (o->active, "operation", temp_op_name, NULL);
            if (!gegl_node_has_pad (o->active, "input"))
              o->pad_active = PAD_OUTPUT;
          break;
          case 1:
            argvs_eval ("node-add aux");
            gegl_node_set (o->active, "operation", temp_op_name, NULL);
            if (gegl_node_has_pad (o->active, "input"))
              o->pad_active = PAD_INPUT;
            else
              o->pad_active = PAD_OUTPUT;
          break;
          case 2:
            argvs_eval ("node-add output");
            gegl_node_set (o->active, "operation", temp_op_name, NULL);
            o->pad_active = PAD_OUTPUT;
          break;
        }
    }
    else
    {
      printf ("uhandled %s\n", *arg);
    }
    o->editing_op_name=0;
  }
   arg++;
  }
    rev_inc (o);
  }

  g_strfreev (argv);
}


static void do_commandline_run (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = data1;
  if (o->commandline[0])
  {
    if (completion_no>=0)
     {
       GList *completions = commandline_get_completions (o->active,
                                                         o->commandline);
       const char *completion = g_list_nth_data (completions, completion_no);
         strcat (o->commandline, completion);
       strcat (o->commandline, " ");
       mrg_set_cursor_pos (event->mrg, strlen (o->commandline));
       while (completions)
       {
         g_free (completions->data);
         completions = g_list_remove (completions, completions->data);
       }
       completion_no = -1;
     }

    argvs_eval ("clear");
    ui_run_command (event, o->commandline, NULL);
  }
  else if (scrollback)
  {
    argvs_eval ("clear");
  }
  else if (o->property_focus)
  {
    argvs_eval ("prop-editor return");
  }
  else
    {
      if (o->is_dir)
      {
        if (o->entry_no == -1)
        {
          argvs_eval ("parent");
        }
        else
        {
	  char *basedir = o->path;
          char *basename = meta_get_child (o, basedir, o->entry_no);
          o->path = g_strdup_printf ("%s/%s", basedir, basename);
          g_free (basedir);
          g_free (basename);
          ui_load_path (o);
          if (g_file_test (o->path, G_FILE_TEST_IS_DIR))
            o->entry_no = 0;
        }
      }
      else
      {
        o->show_graph = !o->show_graph;
        if (o->is_fit)
          zoom_to_fit (o);
      }
    }

  o->commandline[0]=0;
  mrg_set_cursor_pos (event->mrg, 0);
  mrg_queue_draw (o->mrg, NULL);

  mrg_event_stop_propagate (event);
}

static void iterate_frame (GeState *o)
{
  Mrg *mrg = o->mrg;
  static uint32_t prev_ms = 0;
  if (prev_ms == 0)
    prev_ms = mrg_ms (mrg);

  if (g_str_has_suffix (o->src_path, ".gif") ||
      g_str_has_suffix (o->src_path, ".GIF"))
   {
     int frames = 0;
     int frame_delay = 0;
     gegl_node_get (o->source, "frames", &frames, "frame-delay", &frame_delay, NULL);
     if (prev_ms + frame_delay < mrg_ms (mrg))
     {
       int frame_no;
       gegl_node_get (o->source, "frame", &frame_no, NULL);

       frame_no++;
       if (frame_no >= frames)
         frame_no = 0;
       gegl_node_set (o->source, "frame", frame_no, NULL);
       prev_ms = mrg_ms (mrg);
    }
    mrg_queue_draw (o->mrg, NULL);
   }
  else
  {
    static uint32_t frame_accum = 0;
    uint32_t ms = mrg_ms (mrg);
    uint32_t delta = ms - prev_ms;

    if (delta < 500) /* filter out any big pauses - ok for slideshow but
                        make realtime video playback more wrong, with buffering
                        that already is bad on clip change */
    {
      if (frame_accum > 1000 / o->fps)
      {
        /*
         *  this mechanism makes us iterate time-line ahead in increments
         *  according to fps in real-time when able to keep up, and otherwise
         *  slowing down increments accordingly while pretending to have
         *  been realtime
         */
        set_clip_position (o, o->pos + 1.0 / o->fps);
        frame_accum = frame_accum-1000/o->fps;
      }
      frame_accum += delta;
    }

    if (o->pos > o->duration)
    {
       if (o->loop_current)
         {
           argvs_eval ("apos 0");
         }
       else
         argvs_eval ("next");
    }
    else
    {
    }
    prev_ms = ms;

  mrg_queue_draw (mrg, NULL);
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
  mrg_set_xy (mrg, x, h * .6 + em * 1.5);

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
    if (b->label)
    {
      mrg_start (mrg, "dd.binding", NULL);mrg_printf(mrg,"%s", b->label);mrg_end (mrg);
    }
    else if (b->cb == ui_run_command)
    {
      mrg_start (mrg, "dd.binding", NULL);mrg_printf(mrg,"%s", b->cb_data);mrg_end (mrg);
    }

    if (mrg_y (mrg) > h - em * 1.5)
    {
      col++;
      mrg_set_edge_left (mrg, col * (20 * mrg_em(mrg)));
      mrg_set_xy (mrg, col * (15 * em), h * .6 + em * 1.5);
    }

  }
  mrg_end (mrg);
}

static GList *commandline_get_completions (GeglNode *node,
                                           const char *commandline)
{
  const gchar *op_name = node?gegl_node_get_operation (node):"nop";
  const char *last = NULL;  /* the string/piece being completed */

  char *key = NULL;         /* if what is being completed is a key/value pair */
  const char *value = "";

  char *prev = NULL;

  GList *completions = NULL;
  int count = 0;
  int bail = 8;
  if (commandline[0]=='\0')
  {
    return NULL;
  }

  last = strrchr (commandline, ' ');
  if (last)
  {
    const char *plast;
    *(char*)last = '\0';
    plast = strrchr (commandline, ' ');
    if (plast)
      {
        prev = g_strdup (plast);
      }
    else
      prev = g_strdup (commandline);
    *(char*)last = ' ';
    last ++;
  }
  else
  {
    last = commandline;
  }

  if (strchr (last, '='))
  {
    key = g_strdup (last);
    value = strchr (key, '=') + 1;
    *strchr (key, '=') = 0;
  }

  /* walk arguments backwards and look for an op-set  */
  {
    char *tmp = g_strdup (commandline);
    char *fragment;
    char ** operations;
    const char *found_op = NULL;
    gint i;
    guint n_operations;
    /* the last fragment is what we're completing */
    operations = gegl_list_operations (&n_operations);

    fragment = strrchr (tmp, ' ');
    if (!fragment)
      fragment = tmp;
    else
      {
        *fragment='\0';
        fragment++;
      }

    while (fragment && !found_op)
    {
      char prefixed_by_gegl[512];
      sprintf (prefixed_by_gegl, "gegl:%s", fragment);

      if (!strchr (fragment, '='))
      {
      for (i = 0; i < n_operations && !found_op; i++)
        if (!strcmp (operations[i], fragment) ||
            !strcmp (operations[i], prefixed_by_gegl))
          {
            found_op = g_intern_string (operations[i]);
          }
      }

      if (fragment == tmp)
        fragment = NULL;
      else
        {
      fragment = strrchr (tmp, ' ');
      if (!fragment)
        fragment = tmp;
      else
        {
          *fragment='\0';
          fragment++;
        }
        }
    }
    if (found_op)
    {
      op_name = found_op;
    }

    g_free (operations);
  }

  if (prev && !strcmp (prev, "set"))
  {
    for (int i = 0; i < sizeof(settings)/sizeof(settings[0]); i++)
    {
       if (g_str_has_prefix (settings[i].name, last))
       {
         char *result = g_strdup_printf ("%s ", settings[i].name + strlen (last));
         completions = g_list_prepend (completions, result);
         count ++;
       }
    }
  }
  else
  {

    if (key)   /* an = is already part of last bit.. this changes our behavior */
    {
        guint n_props;
        gint i;
        GParamSpec **pspecs = gegl_operation_list_properties (op_name, &n_props);
        GParamSpec *pspec = NULL;
        for (i = 0; i < n_props; i++)
        {
          if (!strcmp (pspecs[i]->name, key))
            pspec = pspecs[i];
        }
        g_free (pspecs);

        if (pspec && g_type_is_a (pspec->value_type, G_TYPE_ENUM))
        {
          GEnumClass *eclass = g_type_class_peek (pspec->value_type);
          for (i = eclass->minimum; i<= eclass->maximum; i++)
          {
            GEnumValue *evalue = &eclass->values[i];
            if (g_str_has_prefix (evalue->value_nick, value))
            {
              char *result = g_strdup_printf ("%s", evalue->value_nick + strlen (value));
              completions = g_list_prepend (completions, result);
              count ++;
            }
          }
        }
    }
    else
    {
      {
        guint n_props;
        gint i;
        GParamSpec **pspecs = gegl_operation_list_properties (op_name, &n_props);
        for (i = 0; i < n_props && count < bail; i++)
        {
          if (g_str_has_prefix (pspecs[i]->name, last))
          {
            char *result = g_strdup_printf ("%s=", pspecs[i]->name + strlen (last));
            completions = g_list_prepend (completions, result);
            count ++;
          }
        }
        g_free (pspecs);
      }

     {
      char prefixed_by_gegl[512];
      char ** operations;
      gint i;
      guint n_operations;
      /* the last fragment is what we're completing */
      operations = gegl_list_operations (&n_operations);
      for (i = 0; i < n_operations && count < bail; i++)
      {
        if (g_str_has_prefix (operations[i], last))
        {
          completions = g_list_prepend (completions, g_strdup (operations[i] + strlen (last)));
          count ++;
        }
      }
      sprintf (prefixed_by_gegl, "gegl:%s", last);

      for (i = 0; i < n_operations && count < bail; i++)
      {
        if (g_str_has_prefix (operations[i], prefixed_by_gegl))
        {
          completions = g_list_prepend (completions, g_strdup (operations[i] + strlen (prefixed_by_gegl)));
          count ++;
        }
      }

      g_free (operations);
     }
    }
  }

  if (key)
    g_free (key);
  if (prev)
    g_free (prev);

  return g_list_reverse (completions);
}


static void expand_completion (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = global_state;
  char common_prefix[512]="";

  GList *completions = commandline_get_completions (o->active,
                                                    o->commandline);

  if (data1 && !strcmp (data1, "tab") &&
      completions && g_list_length (completions)!=1)
  {
    GList *iter;
    const char *completion;
    int common_found = 0;
    int mismatch = 0;

    while (!mismatch)
    {
      completion = completions->data;
      common_prefix[common_found]=completion[common_found];

    for (iter=completions;iter && !mismatch; iter=iter->next)
    {
      completion = iter->data;
      if (completion[common_found] != common_prefix[common_found])
        mismatch++;
    }
    if (!mismatch)
      common_found++;
    }

    if (common_found > 0)
    {
      strncat (o->commandline, completions->data, common_found);
      mrg_set_cursor_pos (event->mrg, strlen (o->commandline));
      completion_no = -1;
      goto cleanup;
    }

  }

  if (g_list_length (completions)==1)
  {
    const char *completion = completions->data;
    strcat (o->commandline, completion);
    mrg_set_cursor_pos (event->mrg, strlen (o->commandline));
  }
#if 0
  else if (data1 && !strcmp (data1, "space"))
   {
     const char *completion = g_list_nth_data (completions, completion_no);
     if (completion_no>=0)
       strcat (commandline, completion);
     strcat (commandline, " ");
     completion_no = -1;
     mrg_set_cursor_pos (event->mrg, strlen (commandline));
   }
#endif
  else
   {
     if (data1 && !strcmp (data1, "rtab"))
       completion_no--;
     else
       completion_no++;

     if (completion_no >= g_list_length (completions))
       completion_no = -1;
     if (completion_no < -1)
       completion_no = -1;

   cleanup:

     while (completions)
     {
       g_free (completions->data);
       completions = g_list_remove (completions, completions->data);
     }
   }
  mrg_queue_draw (event->mrg, NULL);
  mrg_event_stop_propagate (event);
}

static void cmd_unhandled (MrgEvent *event, void *data1, void *data2)
{
  GeState *o = global_state;
  if (mrg_utf8_strlen (event->string) != 1)
    return;

  strcpy (o->commandline, event->string);
  mrg_event_stop_propagate (event);
  mrg_set_cursor_pos (event->mrg, 1);
  mrg_queue_draw (event->mrg, NULL);
}

static void ui_commandline (Mrg *mrg, void *data)
{
  GeState *o = data;
  float em = mrg_em (mrg);
  float h = mrg_height (mrg);
  //float w = mrg_width (mrg);
  cairo_t *cr = mrg_cr (mrg);
  cairo_save (cr);

  if (scrollback == NULL && o->commandline[0]==0)
  {
#if 0
    mrg_set_xy (mrg, 0,h*2);
    mrg_edit_start (mrg, update_commandline, o);
    mrg_printf (mrg, "%s", commandline);
    mrg_edit_end (mrg);
#else
    mrg_add_binding (mrg, "unhandled", NULL, "start entering commandline", cmd_unhandled, NULL);
#endif
    /* XXX - a custom listener only for permitted start commandline keys */
    goto jump;
  }


  if(scrollback){
    MrgList *lines = NULL;
    mrg_start (mrg, "div.scrollback", NULL);
    for (MrgList *l = scrollback; l; l = l->next)
      mrg_list_prepend (&lines, l->data);

    mrg_printf (mrg, "\n");
    for (MrgList *l = lines; l; l = l->next)
    {
      mrg_start (mrg, "div.scrollline", NULL);
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

    mrg_end (mrg);
  }


  if (o->commandline[0])
  {
  mrg_start (mrg, "div.commandline-shell", NULL);
  mrg_start (mrg, "div.prompt", NULL);
  mrg_printf (mrg, "> ");
  mrg_end (mrg);
  mrg_start (mrg, "div.commandline", NULL);
    mrg_edit_start (mrg, update_commandline, o);
    mrg_printf (mrg, "%s", o->commandline);
    mrg_edit_end (mrg);
  mrg_edit_end (mrg);
  mrg_end (mrg);

  //mrg_set_xy (mrg, em, h - em * 1);

    if (mrg_get_cursor_pos (mrg) == mrg_utf8_strlen (o->commandline))
    {
      GList *completions = commandline_get_completions (o->active, o->commandline);
      if (completions)
      {
        GList *iter = completions;
        int no = 0;
        const char *last = strrchr (o->commandline, ' ');
        if (!last) last = o->commandline;

        mrg_start (mrg, no==completion_no?"span.completion-selected":"span.completion", NULL);
        mrg_printf (mrg, "%s", iter->data);
        mrg_end (mrg);
        iter = iter->next;
        no++;

        while (iter)
        {
          mrg_start (mrg, no==completion_no?"span.completion-selected":"span.completion", NULL);
          mrg_printf (mrg, "%s%s", last, iter->data);
          mrg_end (mrg);
          iter = iter->next;
          no++;
        }

        mrg_add_binding (mrg, "tab", NULL, "next completion", expand_completion, "tab");
        mrg_add_binding (mrg, "shift-tab", NULL, "previous completion", expand_completion, "rtab");

        //if (completion_no>=0)
        //  mrg_add_binding (mrg, "space", NULL, NULL, expand_completion, "space");
      }
      while (completions)
      {
        char *completion = completions->data;
        completions = g_list_remove (completions, completion);
        g_free (completion);
      }
    }
    mrg_end (mrg);
  }
 else
  {
    if (o->commandline[0])
    {
      mrg_set_xy (mrg, 0,h*2);
      mrg_edit_start (mrg, update_commandline, o);
      mrg_printf (mrg, "%s", o->commandline);
      mrg_edit_end (mrg);
    }
    else
    {
      mrg_add_binding (mrg, "unhandled", NULL, "start entering commandline", cmd_unhandled, NULL);
    }
  }

jump:
  {
    const char *label = "run commandline";
    if (o->commandline[0]==0)
    {
      if (o->is_dir)
      {
        label = "show entry";
      }
      else
      {
        if (o->show_graph)
        {
          if (o->property_focus)
            label = "change property";
          else
            label = "stop editing";
        }
        else
        {
          label = "show editing graph";
        }
      }
    }
    mrg_add_binding (mrg, "return", NULL, label, do_commandline_run, o);
  }

  cairo_restore (cr);
}

static cairo_matrix_t node_get_relative_transform (GeglNode *node_view,
                                                   GeglNode *source);

static gboolean run_lua_file (const char *basename);

static int per_op_canvas_ui (GeState *o)
{
  Mrg *mrg = o->mrg;
  cairo_t  *cr = mrg_cr (mrg);
  cairo_matrix_t mat;

  const char *opname;
  char *luaname;
  if (!o->active)
    return -1;

  cairo_save (cr);
  cairo_translate (cr, -o->u, -o->v);
  cairo_scale (cr, o->scale, o->scale);
  mat = node_get_relative_transform (o->sink, gegl_node_get_consumer_no (o->active, "output", NULL, 0));

  cairo_transform (cr, &mat);

  opname = gegl_node_get_operation (o->active);
  luaname = g_strdup_printf ("%s.lua", opname);

  for (int i = 0; luaname[i]; i++)
  {
    if (luaname[i] == ':'||
        luaname[i] == ' ')
     luaname[i] = '_';
  }

  run_lua_file (luaname);

  cairo_restore (cr);

  g_free (luaname);

  return 0;
}

static cairo_matrix_t node_get_relative_transform (GeglNode *node_view,
                                                   GeglNode *source)
{
  cairo_matrix_t ret;
  GeglNode *iter = source;
  GSList *list = NULL;

  cairo_matrix_init_identity (&ret);

  while (iter && iter != node_view)
  {
    const char *op = gegl_node_get_operation (iter);

    if (!strcmp (op, "gegl:translate") ||
        !strcmp (op, "gegl:scale-ratio") ||
        !strcmp (op, "gegl:rotate"))
    {
      list = g_slist_prepend (list, iter);
    }

    iter = gegl_node_get_consumer_no (iter, "output", NULL, 0);
  }

  while (list)
  {
    const char *op;
    iter = list->data;
    op  = gegl_node_get_operation (iter);

    if (!strcmp (op, "gegl:translate"))
    {
      gdouble x, y;
      gegl_node_get (iter, "x", &x, "y", &y, NULL);
      cairo_matrix_translate (&ret, x, y);
    }
    else if (!strcmp (op, "gegl:rotate"))
    {
      gdouble degrees;
      gegl_node_get (iter, "degrees", &degrees, NULL);
      cairo_matrix_rotate (&ret, -degrees / 360.0 * M_PI * 2);
    }
    else if (!strcmp (op, "gegl:scale-ratio"))
    {
      gdouble x, y;
      gegl_node_get (iter, "x", &x, "y", &y, NULL);
      cairo_matrix_scale (&ret, x, y);
    }
    list = g_slist_remove (list, iter);
  }

  return ret;
}



/* given a basename return fully qualified path, searching through
 * posisble locations, return NULL if not found.
 */
#ifdef HAVE_LUA
static char *
resolve_lua_file2 (const char *basepath, gboolean add_gegl, const char *basename)
{
  char *path;
  int add_slash = TRUE;
  if (basepath[strlen(basepath)-1]=='/')
    add_slash = FALSE;

  if (add_gegl)
    path = g_strdup_printf ("%s%sgegl-0.4/lua/%s", basepath, add_slash?"/":"", basename);
  else
    path = g_strdup_printf ("%s%s%s", basepath, add_slash?"/":"", basename);

  if (g_file_test (path, G_FILE_TEST_EXISTS))
    return path;
  g_free (path);
  return NULL;
}
#endif
#ifdef HAVE_LUA
static char *
resolve_lua_file (const char *basename)
{
  char *ret = NULL;

  const char * const *data_dirs = g_get_system_data_dirs();
  int i;

  if (!ret)
    ret = resolve_lua_file2 ("/tmp", FALSE, basename);

  if (!ret && binary_relative_data_dir)
    ret = resolve_lua_file2 (binary_relative_data_dir, FALSE, basename);

  if (!ret)
    ret = resolve_lua_file2 (g_get_user_data_dir(), TRUE, basename);

  for (i = 0; !ret && data_dirs[i]; i++)
    ret = resolve_lua_file2 (data_dirs[i], TRUE, basename);

  return ret;
}
#endif


static gboolean run_lua_file (const char *basename)
{
  gboolean ret = FALSE;
#ifdef HAVE_LUA
  char *path = resolve_lua_file (basename);
  int status, result;

  if (!path)
  {
    //fprintf (stderr, "tried running non existing lua file %s\n", basename);
    return FALSE;
  }

// should set

  status = luaL_loadstring(L,
"local foo = GObject.Object(STATE)\n"
"active = foo.active\n"
"sink = foo.sink\n"
"cr = mrg:cr()\n"
"dim = mrg:height() * 0.1;\n"
"dim, dimy = cr:device_to_user_distance(dim, dim)\n"
"centerx, centery = cr:device_to_user(mrg:width()/2, mrg:height()/2)\n"

"source = foo.source\n");
  result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result){
      fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
    }

  status = luaL_loadfile(L, path);
  if (status)
  {
    fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
    ret = FALSE;
  }
  else
  {
    result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result){
      fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
      ret = FALSE;
    } else
    {
      ret = TRUE;
    }
  }

  /* reset active if it has changed */
  g_free (path);
#endif
  return ret;
}


static void draw_bounding_box (GeState *o)
{
  Mrg *mrg = o->mrg;
  cairo_t *cr = mrg_cr (mrg);
  GeglRectangle rect = gegl_node_get_bounding_box (o->active);
  cairo_matrix_t mat;

  cairo_save (cr);
  cairo_translate (cr, -o->u, -o->v);
  cairo_scale (cr, o->scale, o->scale);

  mat = node_get_relative_transform (o->sink, gegl_node_get_consumer_no (o->active, "output", NULL, 0));

  cairo_transform (cr, &mat);

  cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
  ui_contrasty_stroke (cr);

  cairo_restore (cr);
}


static void on_editor_timeline_drag (MrgEvent *e, void *data1, void *data2)
{
  GeState *o = data1;
  float end = o->duration;

  on_viewer_motion (e, data1, data2);

  if (e->type == MRG_DRAG_RELEASE)
  {
  } else if (e->type == MRG_DRAG_PRESS)
  {
  } else if (e->type == MRG_DRAG_MOTION)
  {
  }

  set_clip_position (o, e->x / mrg_width (o->mrg) * end);

  mrg_event_stop_propagate (e);
}

static void draw_editor_timeline (GeState *o)
{
  Mrg *mrg = o->mrg;
  float width = mrg_width(mrg);
  float height = mrg_height(mrg);
  cairo_t *cr = mrg_cr (mrg);
  float pos = o->pos;
  float end = o->duration;

  cairo_save (cr);
  cairo_set_line_width (cr, 2);
  cairo_new_path (cr);
  cairo_rectangle (cr, 0, height * .9, width, height * .1);
  cairo_set_source_rgba (cr, 1,1,1,.5);
  mrg_listen (mrg, MRG_DRAG, on_editor_timeline_drag, o, NULL);

  cairo_fill (cr);

  cairo_set_source_rgba (cr, 1,0,0,1);
  cairo_rectangle (cr, width * pos/end, height * .9, 2, height * .1);
  cairo_fill (cr);

  /* go through all nodes */
  /* go through properties of node */
  {
    /* go through keys of property
     *
     */
    GQuark anim_quark;
    GeglPath *path;
    GeglPathItem path_item;
    char tmpbuf[1024];
    gdouble min_y, max_y;

    sprintf (tmpbuf, "%s-anim", o->property_focus);
    anim_quark = g_quark_from_string (tmpbuf);
    path = g_object_get_qdata (G_OBJECT (o->active), anim_quark);
    cairo_set_source_rgba (cr, 0,1,0,1);

    if (path)
    {
      int nodes = gegl_path_get_n_nodes (path);
      int i;
      gegl_path_get_bounds (path, NULL, NULL, &min_y,  &max_y);

      for (i = 0; i < nodes; i ++)
      {
        gegl_path_get_node (path, i, &path_item);
        {
          float x = ((path_item.point[0].x - o->start) / o->duration) * mrg_width (o->mrg) ;
          float y = (path_item.point[0].y - min_y) / (max_y - min_y) * height * .1 + height *.9;
          if (i == 0)
          {
            cairo_move_to (cr, x, y);
          }
          else
          {
            cairo_line_to (cr, x, y);
          }
        }
      }
      cairo_stroke (cr);
      for (i = 0; i < nodes; i ++)
      {
        gegl_path_get_node (path, i, &path_item);
        {
          float x = ((path_item.point[0].x - o->start) / o->duration) * mrg_width (o->mrg) ;
          float y = (path_item.point[0].y - min_y) / (max_y - min_y) * height * .1 + height *.9;
          cairo_arc (cr, x, y, mrg_em (mrg) * 0.5, 0, 2 * M_PI);
          cairo_fill (cr);
        }
      }
    }
  }

  cairo_restore (cr);
}


static void gegl_ui (Mrg *mrg, void *data)
{
  GeState *o = data;
  struct stat stat_buf;
  int full_quality_render = 0;

  if (last_ms == 0)
  {
    full_quality_render = 1;
    last_ms = -1;
  }
  else
  {
    last_ms = mrg_ms (mrg);
  }

  mrg_stylesheet_add (mrg, css, NULL, 0, NULL);

  lstat (o->path, &stat_buf);
  if (S_ISDIR (stat_buf.st_mode))
  {
    o->is_dir = 1;
  }
  else
  {
    o->is_dir = 0;

    { /* keep zoomed fit on resize */
      static int prev_width = 0;
      static int prev_height = 0;
      int width = mrg_width (mrg);
      int height = mrg_height (mrg);

      if (o->is_fit && prev_width &&
          (width != prev_width || height != prev_height))
      {
        argvs_eval ("zoom fit");
      }
      prev_width = width;
      prev_height = height;
    }
  }

  if (o->is_dir)
  {
    cairo_set_source_rgb (mrg_cr (mrg), .0,.0,.0);
    cairo_paint (mrg_cr (mrg));
  }
  else
  {
    int nearest = 1;
    if (full_quality_render)
    {
      nearest = 0;
    }
    if (o->nearest_neighbor)
      nearest = 1;

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
                      nearest,
                      o->color_managed_display);
     break;
     case GEGL_RENDERER_THREAD:
     case GEGL_RENDERER_IDLE:
       if (o->processor_buffer)
       {
         GeglBuffer *buffer;

         if (o->cached_buffer)
           buffer = g_object_ref (o->cached_buffer);
         else
           buffer = g_object_ref (o->processor_buffer);

         mrg_gegl_buffer_blit (mrg,
                               0, 0,
                               mrg_width (mrg), mrg_height (mrg),
                               buffer,
                               o->u, o->v,
                               o->scale,
                               o->render_quality,
                               nearest,
                               o->color_managed_display);


         g_object_unref (buffer);
       } else {
         fprintf (stderr, "lacking buffer\n");
       }
       break;
  }
  }


  if (o->show_controls && 0)
  {
    mrg_printf (mrg, "%s\n", o->path);
  }

  {


  cairo_save (mrg_cr (mrg));
  {
    if (S_ISREG (stat_buf.st_mode))
    {
      if (o->show_graph)
        {
          if (!g_str_has_suffix (o->path, ".lui"))
          {
            canvas_touch_handling (mrg, o);
            per_op_canvas_ui (o);

          if (o->active && o->show_bounding_box)
            draw_bounding_box (o);
          }

          if (g_str_has_suffix (o->path, ".lui"))
          {
#ifdef HAVE_LUA
            gsize length = 0;
            int result;

            if (lui_contents == NULL)
              g_file_get_contents (o->path, &lui_contents, &length, NULL);


            if (lui_contents)
            {
              int status = luaL_loadstring(L, lui_contents);
              if (status)
              {
                fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
              }
              else
              {
                result = lua_pcall(L, 0, LUA_MULTRET, 0);
                if (result){
                  fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
                } else
                {
                }
              }
              mrg_start (mrg, "div.lui", NULL);

              mrg_edit_start (mrg, update_string2, &lui_contents);
              mrg_print (mrg, lui_contents);
              mrg_edit_end (mrg);
              mrg_end (mrg);
            }
#endif
          }
          else
          {
            draw_graph (o);
            if (lui_contents)
              g_free (lui_contents);
            lui_contents = NULL;
          }

          draw_editor_timeline (o);

        }
      else
        {

  if (g_str_has_suffix (o->path, ".lui"))
  {
#ifdef HAVE_LUA
  int result;
  int status;
  if (lui_contents)
    status = luaL_loadstring(L, lui_contents);
  else
    status = luaL_loadfile(L, o->path);
  if (status)
  {
    fprintf(stderr, "Couldn't load file: %s\n", lua_tostring(L, -1));
  }
  else
  {
    result = lua_pcall(L, 0, LUA_MULTRET, 0);
    if (result){
      fprintf (stderr, "lua exec problem %s\n", lua_tostring(L, -1));
    } else
    {
    }
  }
#endif
  }

#ifdef HAVE_LUA
  if (run_lua_file ("viewer.lua"))
  {
  }
  else
#endif
  {
    canvas_touch_handling (mrg, o);
    ui_viewer (o);
  }
        }

    }
    else if (S_ISDIR (stat_buf.st_mode))
    {
#ifdef HAVE_LUA
  if (run_lua_file ("collection.lua"))
  {
  }
  else
#endif
  {
    ui_collection (o);
  }
    }
  }
  cairo_restore (mrg_cr (mrg));
  }
  cairo_new_path (mrg_cr (mrg));

  if (o->show_preferences)
  {
#ifdef HAVE_LUA
  if (run_lua_file ("preferences.lua"))
  {
  }
  else
#endif
  {
    mrg_printf (mrg, "non-lua preferences NYI\n");
    canvas_touch_handling (mrg, o);
    //ui_viewer (o);
  }
  }


  mrg_add_binding (mrg, "control-p", NULL, NULL, ui_run_command, "toggle preferences");
  mrg_add_binding (mrg, "control-q", NULL, NULL, ui_run_command, "quit");
  mrg_add_binding (mrg, "F11", NULL, NULL,       ui_run_command, "toggle fullscreen");
  mrg_add_binding (mrg, "control-f", NULL, NULL,  ui_run_command, "toggle fullscreen");
  mrg_add_binding (mrg, "control-l", NULL, "clear/redraw", ui_run_command, "clear");
  mrg_add_binding (mrg, "F1", NULL, NULL, ui_run_command, "toggle cheatsheet");
  mrg_add_binding (mrg, "control-h", NULL, NULL, ui_run_command, "toggle cheatsheet");



  if (!text_editor_active(o))
  {
    /* cursor keys and some more keys are used for commandline entry if there already
       is contents, this frees up some bindings/individual keys for direct binding as
       they would not start a valid command anyways. */

    if (!o->is_dir && o->show_graph)
    {
      if (o->property_focus)
      {
          mrg_add_binding (mrg, "tab",   NULL, "focus graph", ui_run_command, "prop-editor focus");
          //mrg_add_binding (mrg, "return", NULL, NULL,   ui_run_command, "prop-editor return");
          mrg_add_binding (mrg, "left", NULL, NULL,       ui_run_command, "prop-editor space");
          mrg_add_binding (mrg, "left", NULL, NULL,       ui_run_command, "prop-editor left");
          mrg_add_binding (mrg, "right", NULL, NULL,      ui_run_command, "prop-editor right");
          mrg_add_binding (mrg, "shift-left", NULL, NULL, ui_run_command, "prop-editor shift-left");
          mrg_add_binding (mrg, "shift-right", NULL, NULL,ui_run_command, "prop-editor shift-right");


          mrg_add_binding (mrg, "`", NULL, NULL,ui_run_command, "keyframe toggle");
          mrg_add_binding (mrg, "control-k", NULL, NULL,ui_run_command, "keyframe toggle");


      }
      else
      {
        mrg_add_binding (mrg, "tab",   NULL, "focus properties", ui_run_command, "prop-editor focus");

        if (o->active != o->source)
          mrg_add_binding (mrg, "control-x", NULL, NULL, ui_run_command, "remove");

        if (o->active != o->source)
          mrg_add_binding (mrg, "control-c", NULL, NULL, ui_run_command, "reference");

        mrg_add_binding (mrg, "control-v", NULL, NULL, ui_run_command, "dereference");

        mrg_add_binding (mrg, "home",  NULL, NULL, ui_run_command, "graph-cursor append");
        mrg_add_binding (mrg, "end",   NULL, NULL, ui_run_command, "graph-cursor source");

        if (lui_contents == NULL)
        {
        if (o->active && gegl_node_has_pad (o->active, "output"))
          mrg_add_binding (mrg, "left", NULL, NULL,        ui_run_command, "graph-cursor left");
        if (o->active) // && gegl_node_has_pad (o->active, "aux"))
          mrg_add_binding (mrg, "right", NULL, NULL, ui_run_command, "graph-cursor right");
        }

      //  if (!o->show_graph)
      //    mrg_add_binding (mrg, "space", NULL, "next image",   ui_run_command, "next");
      }
      //mrg_add_binding (mrg, "backspace", NULL, NULL,  ui_run_command, "prev");
    }
  }

  {

    if (o->show_graph && !text_editor_active (o))
    {
        mrg_add_binding (mrg, "escape", NULL, "stop editing", ui_run_command, "toggle editing");
    if (o->property_focus)
    {
      mrg_add_binding (mrg, "up", NULL, NULL,   ui_run_command, "prop-editor up");
      mrg_add_binding (mrg, "down", NULL, NULL, ui_run_command, "prop-editor down");

    }
    else
    {
      if (o->active && gegl_node_has_pad (o->active, "output"))
        mrg_add_binding (mrg, "up", NULL, NULL,        ui_run_command, "graph-cursor up");
      if (o->active && gegl_node_has_pad (o->active, "input"))
        mrg_add_binding (mrg, "down", NULL, NULL,      ui_run_command, "graph-cursor down");

    if (o->active && gegl_node_has_pad (o->active, "input") &&
                     gegl_node_has_pad (o->active, "output"))
    {
      mrg_add_binding (mrg, "control-up", NULL, "swap active with node above", ui_run_command, "swap output");
      mrg_add_binding (mrg, "control-down", NULL, "swap active with node below", ui_run_command, "swap input");
    }
    }
    }
    else
    {
      if (o->show_graph && lui_contents)
        mrg_add_binding (mrg, "escape", NULL, "stop editing", ui_run_command, "toggle editing");
      else
        mrg_add_binding (mrg, "escape", NULL, "collection view", ui_run_command, "parent");
    }
  }



  if (o->editing_property)
  {
    cairo_new_path (mrg_cr (mrg));
    cairo_rectangle (mrg_cr (mrg), -1, 0, mrg_width (mrg)+2, mrg_height (mrg));
    mrg_listen (mrg, MRG_POINTER|MRG_DRAG|MRG_TAPS, unset_edited_prop, NULL, NULL);
    mrg_add_binding (mrg, "escape", NULL, NULL,  unset_edited_prop, NULL);
    cairo_new_path (mrg_cr (mrg));
  }

  if (!text_editor_active (o))// && !o->property_focus)
  {
    //mrg_add_binding (mrg, "tab", NULL, NULL, ui_run_command, "toggle controls");
    ui_commandline (mrg, o);
  }
  else
  {

  }

  if (o->show_bindings)
  {
     ui_show_bindings (mrg, o);
  }

  if (o->playing)
  {
    iterate_frame (o);
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

char *ui_suffix_path (const char *path)
{
  char *ret;
  char *dirname;
  char *basename;

  if (!path)
    return NULL;
  dirname = g_path_get_dirname (path);
  basename = g_path_get_basename (path);
  ret  = g_strdup_printf ("%s/.gegl/%s/chain.gegl", dirname, basename);
  g_free (dirname);
  g_free (basename);
  return ret;
}


static int is_gegl_path (const char *path)
{
  int ret = 0;
#if 0
  if (g_str_has_suffix (path, ".gegl"))
  {
    char *unsuffixed = ui_unsuffix_path (path);
    if (g_file_test (unsuffixed, G_FILE_TEST_EXISTS))
      ret = 1;
    g_free (unsuffixed);
  }
#endif
  return ret;
}

void ui_contrasty_stroke (cairo_t *cr)
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

static gboolean is_xml_fragment (const char *data)
{
  int i;
  for (i = 0; data && data[i]; i++)
    switch (data[i])
    {
      case ' ':case '\t':case '\n':case '\r': break;
      case '<': return TRUE;
      default: return FALSE;
    }
  return FALSE;
}

static void load_path_inner (GeState *o,
                             char *path)
{
  char *meta;

  if (o->index_dirty && o->loaded_path)
  {
    store_index (o, o->loaded_path);
  }
  if (o->loaded_path)
    g_free (o->loaded_path);
  o->loaded_path = g_strdup (path);

  if (o->src_path)
  {
    if (lui_contents)
    {
      /* we always overwrite, this gives an instant apply user experience
         at the cost of risking leaving in a bad state if crashing at the worst
         possible time.
       */
      g_file_set_contents (o->src_path, lui_contents, -1, NULL);
      g_free (lui_contents);
      lui_contents = NULL;
    }

    g_free (o->src_path);
    o->src_path = NULL;
  }

#if 0
  if (is_gegl_path (path))
  {
    if (o->chain_path)
      g_free (o->chain_path);
    o->chain_path = path;
    path = ui_unsuffix_path (o->chain_path); // or maybe decode first line?
    o->src_path = g_strdup (path);
  }
  else
#endif
  {
    if (o->chain_path)
      g_free (o->chain_path);
    if (g_str_has_suffix (path, ".gegl"))
    {
      o->chain_path = g_strdup (path);
    }
    else if (g_str_has_suffix (path, ".xml"))
    {
#if 0
      o->chain_path = g_strdup_printf ("%sl", path);
      strcpy (strstr(o->chain_path, ".xml"), ".gegl");
#else
      o->chain_path = g_strdup_printf ("%s.gegl", path);
#endif
    }
    else
    {
      o->chain_path = ui_suffix_path (path);
      o->src_path = g_strdup (path);
    }
  }
  load_index (o, o->path);

  if (access (o->chain_path, F_OK) != -1)
  {
    /* XXX : fix this in the fuse layer of zn! XXX XXX XXX XXX */
    if (!strstr (o->chain_path, ".zn.fs"))
      path = o->chain_path;
  }

  g_object_unref (o->gegl);
  o->gegl = NULL;
  o->sink = NULL;
  o->source = NULL;
  if (o->dir_scale <= 0.001)
    o->dir_scale = 1.0;
  o->rev = 0;
  o->fps = 40.0;

  o->start = o->end = 0.0;
  o->duration = -1;
  if (o->duration < 0)
  {
    double start = meta_get_attribute_float (o, NULL, o->entry_no, "start");
    double end   = meta_get_attribute_float (o, NULL, o->entry_no, "end");
    if (start >= 0 && end >= 0)
    {
      o->start = start;
      o->end = end;
      o->duration = o->end - o->start;
    }
  }
  if (o->duration < 0)
  {
    o->duration = meta_get_attribute_float (o, NULL, o->entry_no, "duration");
    o->end = o->duration;
  }
  //if (o->duration < 0)

  o->is_video = 0;
  o->prev_frame_played = 0;
  o->thumbbar_pan_x = 0;

  if (g_str_has_suffix (path, ".pdf") ||
      g_str_has_suffix (path, ".PDF"))
  {
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:pdf-load", "path", path, NULL);
    gegl_node_link_many (o->source, o->sink, NULL);
  }
  else if (g_str_has_suffix (path, ".lui"))
  {
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:rectangle", "color", gegl_color_new ("black"), "width", 1024.0, "height", 768.0, NULL);
    gegl_node_link_many (o->source, o->sink, NULL);
  }
  else if (g_str_has_suffix (path, ".gif"))
  {
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:gif-load", "path", path, NULL);
    o->playing = 1;
    gegl_node_link_many (o->source, o->sink, NULL);
  }
  else if (gegl_str_has_video_suffix (path))
  {
    gint frames = 0;
    double fps = 0.0;
    o->is_video = 1;
    o->playing = 1;
    o->gegl = gegl_node_new ();
    o->sink = gegl_node_new_child (o->gegl,
                       "operation", "gegl:nop", NULL);
    o->source = gegl_node_new_child (o->gegl,
         "operation", "gegl:ff-load", "path", path, NULL);
    gegl_node_link_many (o->source, o->sink, NULL);

    gegl_node_process (o->source);

    {

    gegl_node_get (o->source, "frame-rate", &fps, "frames", &frames, NULL);
    }
    o->fps = fps;

    if (o->duration < 0)
    {
      if (fps > 0.0 && frames > 0)
        o->duration = frames / fps;
    }


    if (o->duration > 0)
    {
      gint frame = 0;

      frame = o->start * fps;
      gegl_node_set (o->source, "frame", frame, NULL);
    }

  }
  else
  {
    meta = NULL;
    if (is_gegl_path (path) || g_str_has_suffix (path, ".gegl") || g_str_has_suffix (path, ".xml"))
      g_file_get_contents (path, &meta, NULL, NULL);

    if (meta)
    {
      GeglNode *iter;
      GeglNode *prev = NULL;
      char *containing_path = g_path_get_dirname (path);

      if (strstr (path, "/chain.gegl"))
      {
        char *foo = containing_path;
        containing_path = g_path_get_dirname (foo);
        g_free (foo);
        foo = containing_path;
        containing_path = g_path_get_dirname (foo);
        g_free (foo);
      }


      if (is_xml_fragment (meta))
        o->gegl = gegl_node_new_from_xml (meta, containing_path);
      else
        {
          o->gegl = gegl_node_new_from_serialized (meta, containing_path);
          gegl_node_set_time (o->gegl, o->start);

        }
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
             "operation", "gegl:gif-load", "path", path, NULL);
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
                                              "path", o->chain_path,
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
                                     "path", o->chain_path,
                                     NULL);
    gegl_node_link_many (o->source, o->sink, NULL);
    gegl_node_set (o->source, "buffer", o->buffer, NULL);
  }
  }

  if (o->duration < 0)
  {
    o->duration = o->slide_pause;
    o->end = o->duration;
  }

  if (o->ops)
  {
    GeglNode *ret_sink = NULL;
    GError *error = (void*)(&ret_sink);

    char *containing_path = get_path_parent (path);

    gegl_create_chain_argv (o->ops,
                    gegl_node_get_producer (o->sink, "input", NULL),
                    o->sink, 2.1, gegl_node_get_bounding_box (o->sink).height,
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
  o->pos = 0.0;

  activate_sink_producer (o);

  if (o->processor)
      g_object_unref (o->processor);
  o->processor = gegl_node_new_processor (o->sink, NULL);

  queue_draw (o);
}


void ui_load_path (GeState *o)
{
  while (thumb_queue)
  {
    ThumbQueueItem *item = thumb_queue->data;
    mrg_list_remove (&thumb_queue, item);
    thumb_queue_item_free (item);
  }

  //o->playing = 0;

  load_path_inner (o, o->path);
  populate_path_list (o);

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
    //if (S_ISDIR (stat_buf.st_mode))
    //{
    //  o->entry_no = -1;
    //}
  }

  o->scale = 1.0;
  o->u = 0;
  o->v = 0;

  zoom_to_fit (o);
  mrg_queue_draw (o->mrg, NULL);
}



static void drag_preview (MrgEvent *e)
{
  GeState *o = global_state;
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

/* photos_gegl_buffer_apply_orientation from GNOME Photos */

typedef enum {
  PHOTOS_ORIENTATION_UNSPECIFIED = 0,
  PHOTOS_ORIENTATION_TOP = GEXIV2_ORIENTATION_NORMAL,
  PHOTOS_ORIENTATION_TOP_MIRROR = GEXIV2_ORIENTATION_HFLIP,
  PHOTOS_ORIENTATION_BOTTOM = GEXIV2_ORIENTATION_ROT_180,
  PHOTOS_ORIENTATION_BOTTOM_MIRROR = GEXIV2_ORIENTATION_VFLIP,
  PHOTOS_ORIENTATION_LEFT_MIRROR = GEXIV2_ORIENTATION_ROT_90_HFLIP,
  PHOTOS_ORIENTATION_RIGHT = GEXIV2_ORIENTATION_ROT_90,
  PHOTOS_ORIENTATION_RIGHT_MIRROR =GEXIV2_ORIENTATION_ROT_90_VFLIP,
  PHOTOS_ORIENTATION_LEFT = GEXIV2_ORIENTATION_ROT_270,
} Orientation;

static void
photos_gegl_buffer_apply_orientation_flip_in_place (guchar *buf, gint bpp, gint n_pixels)
{
  gint i;

  for (i = 0; i < n_pixels / 2; i++)
    {
      gint j;
      guchar *pixel_left = buf + i * bpp;
      guchar *pixel_right = buf + (n_pixels - 1 - i) * bpp;

      for (j = 0; j < bpp; j++)
        {
          guchar tmp = pixel_left[j];

          pixel_left[j] = pixel_right[j];
          pixel_right[j] = tmp;
        }
    }
}

static GeglBuffer *
photos_gegl_buffer_apply_orientation (GeglBuffer *buffer_original, Orientation orientation)
{
  const Babl *format;
  g_autoptr (GeglBuffer) buffer_oriented = NULL;
  GeglBuffer *ret_val = NULL;
  GeglRectangle bbox_oriented;
  GeglRectangle bbox_original;
  gint bpp;

  g_return_val_if_fail (GEGL_IS_BUFFER (buffer_original), NULL);
  g_return_val_if_fail (orientation == PHOTOS_ORIENTATION_UNSPECIFIED
                        ||orientation == PHOTOS_ORIENTATION_BOTTOM
                        || orientation == PHOTOS_ORIENTATION_BOTTOM_MIRROR
                        || orientation == PHOTOS_ORIENTATION_LEFT
                        || orientation == PHOTOS_ORIENTATION_LEFT_MIRROR
                        || orientation == PHOTOS_ORIENTATION_RIGHT
                        || orientation == PHOTOS_ORIENTATION_RIGHT_MIRROR
                        || orientation == PHOTOS_ORIENTATION_TOP
                        || orientation == PHOTOS_ORIENTATION_TOP_MIRROR,
                        NULL);

  if (orientation == PHOTOS_ORIENTATION_TOP ||
      orientation == PHOTOS_ORIENTATION_UNSPECIFIED)
    {
      ret_val = g_object_ref (buffer_original);
      goto out;
    }

  bbox_original = *gegl_buffer_get_extent (buffer_original);

  if (orientation == PHOTOS_ORIENTATION_BOTTOM || orientation == PHOTOS_ORIENTATION_BOTTOM_MIRROR)
    {
      /* angle = 180 degrees */
      /* angle = 180 degrees, axis = vertical; or, axis = horizontal */
      bbox_oriented.height = bbox_original.height;
      bbox_oriented.width = bbox_original.width;
      bbox_oriented.x = bbox_original.x;
      bbox_oriented.y = bbox_original.y;
    }
  else if (orientation == PHOTOS_ORIENTATION_LEFT || orientation == PHOTOS_ORIENTATION_LEFT_MIRROR)
    {
      /* angle = -270 or 90 degrees counterclockwise */
      /* angle = -270 or 90 degrees counterclockwise, axis = horizontal */
      bbox_oriented.height = bbox_original.width;
      bbox_oriented.width = bbox_original.height;
      bbox_oriented.x = bbox_original.x;
      bbox_oriented.y = bbox_original.y;
    }
  else if (orientation == PHOTOS_ORIENTATION_RIGHT || orientation == PHOTOS_ORIENTATION_RIGHT_MIRROR)
    {
      /* angle = -90 or 270 degrees counterclockwise */
      /* angle = -90 or 270 degrees counterclockwise, axis = horizontal */
      bbox_oriented.height = bbox_original.width;
      bbox_oriented.width = bbox_original.height;
      bbox_oriented.x = bbox_original.x;
      bbox_oriented.y = bbox_original.y;
    }
  else if (orientation == PHOTOS_ORIENTATION_TOP_MIRROR)
    {
      /* axis = vertical */
      bbox_oriented.height = bbox_original.height;
      bbox_oriented.width = bbox_original.width;
      bbox_oriented.x = bbox_original.x;
      bbox_oriented.y = bbox_original.y;
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  format = gegl_buffer_get_format (buffer_original);
  bpp = babl_format_get_bytes_per_pixel (format);
  buffer_oriented = gegl_buffer_new (&bbox_oriented, format);

  if (orientation == PHOTOS_ORIENTATION_BOTTOM || orientation == PHOTOS_ORIENTATION_BOTTOM_MIRROR)
    {
      GeglRectangle bbox_destination;
      GeglRectangle bbox_source;

      /* angle = 180 degrees */
      /* angle = 180 degrees, axis = vertical; or, axis = horizontal */

      g_return_val_if_fail (bbox_oriented.height == bbox_original.height, NULL);
      g_return_val_if_fail (bbox_oriented.width == bbox_original.width, NULL);

      gegl_rectangle_set (&bbox_destination, bbox_oriented.x, bbox_oriented.y, (guint) bbox_oriented.width, 1);

      bbox_source.x = bbox_original.x;
      bbox_source.y = bbox_original.y + bbox_original.height - 1;
      bbox_source.height = 1;
      bbox_source.width = bbox_original.width;

      if (orientation == PHOTOS_ORIENTATION_BOTTOM)
        {
          gint i;
          g_autofree guchar *buf = NULL;

          buf = g_malloc0_n (bbox_oriented.width, bpp);

          for (i = 0; i < bbox_original.height; i++)
            {
              gegl_buffer_get (buffer_original,
                               &bbox_source,
                               1.0,
                               format,
                               buf,
                               GEGL_AUTO_ROWSTRIDE,
                               GEGL_ABYSS_NONE);
              photos_gegl_buffer_apply_orientation_flip_in_place (buf, bpp, bbox_original.width);
              gegl_buffer_set (buffer_oriented, &bbox_destination, 0, format, buf, GEGL_AUTO_ROWSTRIDE);

              bbox_destination.y++;
              bbox_source.y--;
            }
        }
      else
        {
          gint i;

          for (i = 0; i < bbox_original.height; i++)
            {
              gegl_buffer_copy (buffer_original, &bbox_source, GEGL_ABYSS_NONE, buffer_oriented, &bbox_destination);
              bbox_destination.y++;
              bbox_source.y--;
            }
        }
    }
  else if (orientation == PHOTOS_ORIENTATION_LEFT || orientation == PHOTOS_ORIENTATION_LEFT_MIRROR)
    {
      GeglRectangle bbox_source;
      g_autofree guchar *buf = NULL;

      /* angle = -270 or 90 degrees counterclockwise */
      /* angle = -270 or 90 degrees counterclockwise, axis = horizontal */

      g_return_val_if_fail (bbox_oriented.height == bbox_original.width, NULL);
      g_return_val_if_fail (bbox_oriented.width == bbox_original.height, NULL);

      bbox_source.x = bbox_original.x + bbox_original.width - 1;
      bbox_source.y = bbox_original.y;
      bbox_source.height = bbox_original.height;
      bbox_source.width = 1;

      buf = g_malloc0_n (bbox_oriented.width, bpp);

      if (orientation == PHOTOS_ORIENTATION_LEFT)
        {
          GeglRectangle bbox_destination;
          gint i;

          gegl_rectangle_set (&bbox_destination, bbox_oriented.x, bbox_oriented.y, (guint) bbox_oriented.width, 1);

          for (i = 0; i < bbox_original.width; i++)
            {
              gegl_buffer_get (buffer_original,
                               &bbox_source,
                               1.0,
                               format,
                               buf,
                               GEGL_AUTO_ROWSTRIDE,
                               GEGL_ABYSS_NONE);
              gegl_buffer_set (buffer_oriented, &bbox_destination, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
              bbox_destination.y++;
              bbox_source.x--;
            }
        }
      else
        {
          GeglRectangle bbox_destination;
          gint i;

          bbox_destination.x = bbox_oriented.x;
          bbox_destination.y = bbox_oriented.y + bbox_oriented.height - 1;
          bbox_destination.height = 1;
          bbox_destination.width = bbox_oriented.width;

          for (i = 0; i < bbox_original.width; i++)
            {
              gegl_buffer_get (buffer_original,
                               &bbox_source,
                               1.0,
                               format,
                               buf,
                               GEGL_AUTO_ROWSTRIDE,
                               GEGL_ABYSS_NONE);
              gegl_buffer_set (buffer_oriented, &bbox_destination, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
              bbox_destination.y--;
              bbox_source.x--;
            }
        }
    }
  else if (orientation == PHOTOS_ORIENTATION_RIGHT || orientation == PHOTOS_ORIENTATION_RIGHT_MIRROR)
    {
      GeglRectangle bbox_destination;
      GeglRectangle bbox_source;
      g_autofree guchar *buf = NULL;

      /* angle = -90 or 270 degrees counterclockwise */
      /* angle = -90 or 270 degrees counterclockwise, axis = horizontal */

      g_return_val_if_fail (bbox_oriented.height == bbox_original.width, NULL);
      g_return_val_if_fail (bbox_oriented.width == bbox_original.height, NULL);

      gegl_rectangle_set (&bbox_destination, bbox_oriented.x, bbox_oriented.y, 1, (guint) bbox_oriented.height);

      bbox_source.x = bbox_original.x;
      bbox_source.y = bbox_original.y + bbox_original.height - 1;
      bbox_source.height = 1;
      bbox_source.width = bbox_original.width;

      buf = g_malloc0_n (bbox_oriented.height, bpp);

      if (orientation == PHOTOS_ORIENTATION_RIGHT)
        {
          gint i;

          for (i = 0; i < bbox_original.height; i++)
            {
              gegl_buffer_get (buffer_original,
                               &bbox_source,
                               1.0,
                               format,
                               buf,
                               GEGL_AUTO_ROWSTRIDE,
                               GEGL_ABYSS_NONE);
              gegl_buffer_set (buffer_oriented, &bbox_destination, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
              bbox_destination.x++;
              bbox_source.y--;
            }
        }
      else
        {
          gint i;

          for (i = 0; i < bbox_original.height; i++)
            {
              gegl_buffer_get (buffer_original,
                               &bbox_source,
                               1.0,
                               format,
                               buf,
                               GEGL_AUTO_ROWSTRIDE,
                               GEGL_ABYSS_NONE);
              photos_gegl_buffer_apply_orientation_flip_in_place (buf, bpp, bbox_original.width);
              gegl_buffer_set (buffer_oriented, &bbox_destination, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
              bbox_destination.x++;
              bbox_source.y--;
            }
        }
    }
  else if (orientation == PHOTOS_ORIENTATION_TOP_MIRROR)
    {
      GeglRectangle bbox_destination;
      GeglRectangle bbox_source;
      gint i;

      /* axis = vertical */

      g_return_val_if_fail (bbox_oriented.height == bbox_original.height, NULL);
      g_return_val_if_fail (bbox_oriented.width == bbox_original.width, NULL);

      bbox_destination.x = bbox_oriented.x + bbox_oriented.width - 1;
      bbox_destination.y = bbox_oriented.y;
      bbox_destination.height = bbox_oriented.height;
      bbox_destination.width = 1;

      gegl_rectangle_set (&bbox_source, bbox_original.x, bbox_original.y, 1, (guint) bbox_original.height);

      for (i = 0; i < bbox_original.width; i++)
        {
          gegl_buffer_copy (buffer_original, &bbox_source, GEGL_ABYSS_NONE, buffer_oriented, &bbox_destination);
          bbox_destination.x--;
          bbox_source.x++;
        }
    }
  else
    {
      g_return_val_if_reached (NULL);
    }

  ret_val = g_object_ref (buffer_oriented);

 out:
  return ret_val;
}


static void load_into_buffer (GeState *o, const char *path)
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
#if 0
    gboolean hflip = FALSE;
    gboolean vflip = FALSE;
    double degrees = 0.0;
    float tx = 0, ty= 0;
    int width = gegl_buffer_get_extent (o->buffer)->width;
    int height = gegl_buffer_get_extent (o->buffer)->height;

    switch (orientation)
    {
      case GEXIV2_ORIENTATION_UNSPECIFIED:
      case GEXIV2_ORIENTATION_NORMAL:
        break;
      case GEXIV2_ORIENTATION_HFLIP: hflip=TRUE; break;
      case GEXIV2_ORIENTATION_VFLIP: vflip=TRUE; break;
      case GEXIV2_ORIENTATION_ROT_90: degrees = 90.0; tx=height; break;
      case GEXIV2_ORIENTATION_ROT_90_HFLIP: degrees = 90.0; hflip=TRUE; tx=height; break;
      case GEXIV2_ORIENTATION_ROT_90_VFLIP: degrees = 90.0; vflip=TRUE; tx=height; break;
      case GEXIV2_ORIENTATION_ROT_180: degrees = 180.0; tx=width;ty=height;break;
      case GEXIV2_ORIENTATION_ROT_270: degrees = 270.0; ty=width;  break;
    }

    if (degrees != 0.0 || vflip || hflip)
     {
       /* XXX: deal with vflip/hflip */
       GeglBuffer *new_buffer = NULL;
       GeglNode *rotate, *translate;

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
       translate = gegl_node_new_child (gegl, "operation", "gegl:translate",
                                   "sampler", GEGL_SAMPLER_NEAREST,
                                   "x", tx,
                                   "y", ty,
                                   NULL);
       gegl_node_link_many (load, rotate, translate, sink, NULL);
       gegl_node_process (sink);
       g_object_unref (gegl);
       g_object_unref (o->buffer);
       o->buffer = new_buffer;
     }
#else
    {
       GeglBuffer *orig = o->buffer;
       o->buffer = photos_gegl_buffer_apply_orientation (orig, orientation);
       g_object_unref (orig);
    }
#endif
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
static GeglNode *locate_node (GeState *o, const char *op_name)
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

static void zoom_to_fit (GeState *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  float scale, scale2;
  float width = 256, height = 256;

  if (rect.width == 0 || rect.height == 0)
  {
    o->scale = 1.0;
    o->u = 0.0;
    o->v = 0.0;
    return;
  }

  if (rect.width >  1000000 ||
      rect.height > 1000000)
  {
     rect.x=0;
     rect.y=0;
     rect.width=1024;
     rect.height=1024;
  }

  if (mrg)
  {
    width = mrg_width (mrg);
    height = mrg_height (mrg);

    o->graph_pan_x = -(width - height * font_size_scale * 22);
    if (o->show_graph)
    {
      width = width - height * font_size_scale * 22;
    }
  }

  scale = 1.0 * width / rect.width;
  scale2 = 1.0 * height / rect.height;

  if (scale2 < scale) scale = scale2;

  o->scale = scale;

  o->u = -(width - rect.width * o->scale) / 2;
  o->v = -(height - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;


  o->is_fit = 1;

  if (mrg)
    mrg_queue_draw (mrg, NULL);
}
static void center (GeState *o)
{
  Mrg *mrg = o->mrg;
  GeglRectangle rect = gegl_node_get_bounding_box (o->sink);
  o->scale = 1.0;


  o->u = -(mrg_width (mrg) - rect.width * o->scale) / 2;
  o->v = -(mrg_height (mrg) - rect.height * o->scale) / 2;
  o->u += rect.x * o->scale;
  o->v += rect.y * o->scale;
  o->is_fit = 0;

  mrg_queue_draw (mrg, NULL);
}

static void zoom_at (GeState *o, float screen_cx, float screen_cy, float factor)
{
  float x, y;
  get_coords (o, screen_cx, screen_cy, &x, &y);
  o->scale *= factor;
  o->u = x * o->scale - screen_cx;
  o->v = y * o->scale - screen_cy;
  o->is_fit = 0;

  queue_draw (o);
}


static int deferred_zoom_to_fit (Mrg *mrg, void *data)
{
  argvs_eval ("zoom fit");
  return 0;
}

static void get_coords (GeState *o, float screen_x, float screen_y, float *gegl_x, float *gegl_y)
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
  GeState *o = global_state;
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
  GeState *o = global_state;
  if (setting->read_only)
    return -1;
  switch (setting->type)
  {
    case 0:
        (*(int*)(((char *)o) + setting->offset)) = atoi (value);
    break;
    case 1:
        (*(float*)(((char *)o) + setting->offset)) = g_ascii_strtod (value, NULL);
    break;
    case 2:
        memcpy ((((char *)o) + setting->offset), g_strdup (value), sizeof (void*));
    break;
  }
  return 0;
}



/* loads the source image corresponding to o->path into o->buffer and
 * creates live gegl pipeline, or nops.. rigs up o->chain_path to be
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


// defcom

  int cmd_save (COMMAND_ARGS);
int cmd_save (COMMAND_ARGS) /* "save", 0, "", ""*/
{
  GeState *o = global_state;
  char *serialized;

  {
    char *containing_path = get_path_parent (o->chain_path);
    serialized = gegl_serialize (o->source,
                   gegl_node_get_producer (o->sink, "input", NULL),
 containing_path, GEGL_SERIALIZE_TRIM_DEFAULTS|GEGL_SERIALIZE_VERSION|GEGL_SERIALIZE_INDENT);
    g_free (containing_path);
  }

  if (o->src_path)
  {
    char *prepended = g_strdup_printf ("gegl:load path=%s\n%s", g_basename(o->src_path), serialized);
    g_file_set_contents (o->chain_path, prepended, -1, NULL);
    g_free (prepended);
  }
  else
  {
    g_file_set_contents (o->chain_path, serialized, -1, NULL);
  }

  g_free (serialized);
  argvs_eval ("thumb");
  o->rev = 0;
  return 0;
}

static void gegl_node_defaults (GeglNode *node)
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


int cmd_node_defaults (COMMAND_ARGS); /* "node-defaults", -1, "", "reset properties to default values"*/

int
cmd_node_defaults (COMMAND_ARGS)
{
  GeState *o = global_state;

  if (o->active)
    gegl_node_defaults (o->active);

  rev_inc (o);
  return 0;
}




int cmd_info (COMMAND_ARGS); /* "info", 0, "", "dump information about active node"*/

int
cmd_info (COMMAND_ARGS)
{
  GeState *o = global_state;
  GeglNode *node = o->active;
  GeglOperation *operation;
  GeglRectangle extent;

  {
    char *path;
    char **attributes = NULL;
    char **keys = NULL;

     if (o->is_dir)
     {
       path = get_item_path (o);
     }
     else
     {
       path = g_strdup (o->path);
     }
    if (!path)
      return -1;

    attributes = meta_list_attributes (o, path, o->entry_no);
    for (char **a = attributes; a && *a; a++)
    {
      const char *value = meta_get_attribute (o, path, o->entry_no, *a);
      printf ("%s@%s\n", *a, value);
    }
    if (attributes) g_free (attributes);

    keys = meta_list_keys (o, path);
    for (char **a = keys; a && *a; a++)
    {
      const char *value = meta_get_key (o, path, *a);
      printf ("%s=%s\n", *a, value);
    }
    if (keys) g_free (keys);
    printf ("\n");

    g_free (path);
  }

  if (!node)
  {
    printf ("no active node\n");
    return 0;
  }
  operation = gegl_node_get_gegl_operation (node);
  printf ("operation: %s   %p\n", gegl_node_get_operation (node), node);
  extent = gegl_node_get_bounding_box (node);
  printf ("bounds: %i %i  %ix%i\n", extent.x, extent.y, extent.width, extent.height);

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

int cmd_toggle (COMMAND_ARGS); /* "toggle", 1, "<editing|fullscreen|cheatsheet|mipmap|controls|playing>", ""*/
int
cmd_toggle (COMMAND_ARGS)
{
  GeState *o = global_state;
  if (!strcmp(argv[1], "editing"))
  {
    o->show_graph = !o->show_graph;
    o->property_focus = NULL;
    if (o->is_fit)
      zoom_to_fit (o);
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
  else if (!strcmp(argv[1], "preferences"))
  {
    o->show_preferences = !o->show_preferences;
  }
  else if (!strcmp(argv[1], "colormanaged-display"))
  {
    o->color_managed_display = !o->color_managed_display;
    printf ("%s colormanagement of display\n", o->color_managed_display?"enabled":"disabled");
    mrg_gegl_dirty (o->mrg);
  }
  else if (!strcmp(argv[1], "opencl"))
  {
    gboolean curval;
    g_object_get (gegl_config(), "use-opencl", &curval, NULL);
    if (curval == FALSE) {
      g_object_set (gegl_config(), "use-opencl", TRUE, NULL);
      printf ("enabled opencl\n");
    }
    else
    {
      g_object_set (gegl_config(), "use-opencl", FALSE, NULL);
      printf ("disabled opencl\n");
    }
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
      renderer = GEGL_RENDERER_IDLE; // XXX : becomes wrong if thred was original
      printf ("disabled mipmap rendering\n");
    }
  }
  else if (!strcmp(argv[1], "controls"))
  {
    o->show_controls = !o->show_controls;
  }
  else if (!strcmp(argv[1], "playing"))
  {
    o->playing = !o->playing;
  }
  else if (!strcmp(argv[1], "loop-current"))
  {
    o->loop_current = !o->loop_current;
  }
  queue_draw (o);
  return 0;
}

  int cmd_keyframe (COMMAND_ARGS);
int cmd_keyframe (COMMAND_ARGS) /* "keyframe", 1, "<set|unset|toggle|clear>", "manipulate keyframe"*/
{
  GeState *o = global_state;
  GQuark anim_quark;
  GeglPath *path;
  GeglPathItem path_item;
  char tmpbuf[1024];
  gdouble clip_pos = o->pos + o->start;

  sprintf (tmpbuf, "%s-anim", o->property_focus);
  anim_quark = g_quark_from_string (tmpbuf);
  path = g_object_get_qdata (G_OBJECT (o->active), anim_quark);

  if (!strcmp (argv[1], "set"))
  {
    gdouble value;
    gegl_node_get (o->active, o->property_focus, &value, NULL);
    if (!path)
    {
      path = gegl_path_new ();
      g_object_set_qdata (G_OBJECT (o->active), anim_quark, path);
    }

    {
      int nodes = gegl_path_get_n_nodes (path);
      int i;
      int done = 0;
      GeglPathItem new_item;
      new_item.type = 'L';
      new_item.point[0].x = clip_pos;
      new_item.point[0].y = value;

      for (i = 0; i < nodes && !done; i ++)
      {
        GeglPathItem iter_item;
        gegl_path_get_node (path, i, &iter_item);

        if (fabs (iter_item.point[0].x - clip_pos) < 1.0/30.0)
        {
          gegl_path_replace_node (path, i, &new_item);
          done = 1;
        }
        else if (iter_item.point[0].x > clip_pos)
        {
          gegl_path_insert_node (path, i-1, &new_item);
          done = 1;
        }
      }
      if (!done)
        gegl_path_insert_node (path, -1, &new_item);
    }
  }
  else if (!strcmp (argv[1], "unset"))
  {
    if (path)
    {
      int nodes = gegl_path_get_n_nodes (path);
      int i;
      int done = 0;

      for (i = 0; i < nodes && !done; i ++)
      {
        GeglPathItem iter_item;
        gegl_path_get_node (path, i, &iter_item);

        if (fabs (iter_item.point[0].x - clip_pos) < 1.0/30.0)
        {
          int nodes;
          gegl_path_remove_node (path, i);
          done = 1;

          nodes = gegl_path_get_n_nodes (path);
          if (nodes == 0)
          {
            g_clear_object (&path);
            g_object_set_qdata (G_OBJECT (o->active), anim_quark, path);
          }
        }
      }
    }
  }
  else if (!strcmp (argv[1], "toggle"))
  {
    /* if no keyframe value for frame or if value is different than configured value, set
       if value is same as value, unset (and reinterpolate) */

    gdouble value;
    gegl_node_get (o->active, o->property_focus, &value, NULL);
    if (!path)
    {
      path = gegl_path_new ();
      g_object_set_qdata (G_OBJECT (o->active), anim_quark, path);
    }

    {
      int nodes = gegl_path_get_n_nodes (path);
      int i;
      int done = 0;
      GeglPathItem new_item;
      new_item.type = 'L';
      new_item.point[0].x = clip_pos;
      new_item.point[0].y = value;

      for (i = 0; i < nodes && !done; i ++)
      {
        GeglPathItem iter_item;
        gegl_path_get_node (path, i, &iter_item);

        if (fabs (iter_item.point[0].x - clip_pos) < 1.0/30.0)
        {
          if (fabs (iter_item.point[0].y - value) < 0.001)
          {
            int nodes;
            gegl_path_remove_node (path, i);
            nodes = gegl_path_get_n_nodes (path);
            if (nodes == 0)
            {
              g_clear_object (&path);
              g_object_set_qdata (G_OBJECT (o->active), anim_quark, path);
            }

          }
          else
          {
            gegl_path_replace_node (path, i, &new_item);
          }
          done = 1;
        }
        else if (iter_item.point[0].x > clip_pos)
        {
          gegl_path_insert_node (path, i-1, &new_item);
          done = 1;
        }
      }
      if (!done)
        gegl_path_insert_node (path, -1, &new_item);
    }

  }
  else if (!strcmp (argv[1], "clear"))
  {
    g_clear_object (&path);
    g_object_set_qdata (G_OBJECT (o->active), anim_quark, path);
  }
  else if (!strcmp (argv[1], "list"))
  {
    if (path)
    {
      int nodes = gegl_path_get_n_nodes (path);
      int i;
      for (i = 0; i < nodes; i ++)
      {
        gegl_path_get_node (path, i, &path_item);
        printf ("%f %f\n", path_item.point[0].x,
                           path_item.point[0].y);
      }
    }
  }

  mrg_queue_draw (o->mrg, NULL);

  return 0;
}


  int cmd_star (COMMAND_ARGS);
int cmd_star (COMMAND_ARGS) /* "star", -1, "", "query or set number of stars"*/
{
  GeState *o = global_state;
  char *path = get_item_path (o);
  if (!path)
    return -1;

  if (argv[1])
  {
    meta_set_key_int (o, path, "stars", atoi(argv[1]));
  }
  else
  {
    int stars = meta_get_key_int (o, path, "stars");
    if (stars >= 0)
      printf ("%s has %i stars\n", path, stars);
    else
      printf ("stars have not been set on %s\n", path);
  }
  g_free (path);
  mrg_queue_draw (o->mrg, NULL);

  return 0;
}

static void
index_item_destroy (IndexItem *item);

  int cmd_system (COMMAND_ARGS);
int cmd_system (COMMAND_ARGS) /* "system", -1, "", "systemes passed commandline"*/
{
  GError *error = NULL;
  static GPid pid = 0;
  g_spawn_async (NULL, &argv[1], NULL,
                 G_SPAWN_SEARCH_PATH|G_SPAWN_SEARCH_PATH_FROM_ENVP,
                 NULL, NULL, &pid, &error);
  return 0;
}


  int cmd_discard (COMMAND_ARGS);
int cmd_discard (COMMAND_ARGS) /* "discard", 0, "", "moves the current image to a .discard subfolder"*/
{
  GeState *o = global_state;

  char *path = get_item_path (o);
  char *folder = get_item_dir (o);
  int   entry_no = get_item_no (o);
  char *basename = g_path_get_basename (path);

  if (!o->is_dir)
  {
    if (o->entry_no == ui_items_count (o) - 1)
    {
      argvs_eval ("prev");
    }
    else
    {
      argvs_eval ("next");
    }
  }
  {
    char command[4096];

    sprintf (command, "mkdir %s/.discard > /dev/null 2>&1", folder);
    system (command);
    sprintf (command, "mv %s %s/.discard > /dev/null 2>&1", path, folder);
    system (command);
    sprintf (command, "rm %s/.gegl/%s/thumb.jpg > /dev/null 2>&1", folder, basename);
    system (command);
    sprintf (command, "mv %s/.gegl/%s/chain.gegl %s/.discard/%s.gegl > /dev/null 2>&1", folder, basename, folder, basename);
    system (command);

    sprintf (command, "mv %s/.gegl/%s/metadata %s/.discard/%s.meta > /dev/null 2>&1", folder, basename, folder, basename);
    system (command);
    sprintf (command, "rmdir %s/.gegl/%s", folder, basename);
    system (command);

    populate_path_list (o);
  }
  if (entry_no < g_list_length (o->index))
  {
    IndexItem *item = g_list_nth_data (o->index, entry_no);
    o->index = g_list_remove (o->index, item);
    index_item_destroy (item);
    o->index_dirty++;
  }

  g_free (folder);
  g_free (path);
  g_free (basename);
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}

  int cmd_cd (COMMAND_ARGS);
int cmd_cd (COMMAND_ARGS) /* "cd", 1, "<target>", "convenience wrapper making some common commandline navigation commands work"*/
{
  GeState *o = global_state;
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
    ui_load_path (o);
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
      ui_load_path (o);
    }
    free (rp);
    g_free (new_path);
  }
  return 0;
}


  int cmd_order (COMMAND_ARGS);
int cmd_order (COMMAND_ARGS) /* "order", -1, "<az|time|exif-time|stars>", "Sets sort order."*/
{
  GeState *o = global_state;

  if (!argv[1])
  {
    printf ("current sort order: %i\n", o->sort_order);
    return 0;
  }

  {
  int was_custom = o->sort_order & SORT_ORDER_CUSTOM;
    if (!strcmp (argv[1], "az"))
    {
      o->sort_order = SORT_ORDER_AZ;
    }
    else if (!strcmp (argv[1], "stars"))
    {
      o->sort_order = SORT_ORDER_STARS;
    }
    else if (!strcmp (argv[1], "time"))
    {
      o->sort_order = SORT_ORDER_MTIME;
    }
    else if (!strcmp (argv[1], "exif-time"))
    {
      o->sort_order = SORT_ORDER_EXIF_TIME;
    }
    else if (!strcmp (argv[1], "custom"))
    {
      o->sort_order = SORT_ORDER_CUSTOM;
    }
    else
    {
      printf ("unknown sort order %s\n", argv[1]);
    }
    if (was_custom)
      o->sort_order &= SORT_ORDER_CUSTOM;

  }

  populate_path_list (o);

  return 0;
}


  int cmd_zoom (COMMAND_ARGS);
int cmd_zoom (COMMAND_ARGS) /* "zoom", -1, "<fit|in [amt]|out [amt]|zoom-level>", "Changes zoom level, asbolsute or relative, around middle of screen."*/
{
  GeState *o = global_state;

  if (!argv[1])
  {
    printf ("current scale factor: %2.3f\n", o->is_dir?o->dir_scale:o->scale);
    return 0;
  }

  if (o->is_dir)
  {
     float zoom_factor = 0.05;
     if (!strcmp(argv[1], "in"))
     {
       if (argv[2])
         zoom_factor = g_ascii_strtod (argv[2], NULL);
       zoom_factor += 1.0;
       o->dir_scale *= zoom_factor;
      }
      else if (!strcmp(argv[1], "out"))
      {
        if (argv[2])
          zoom_factor = g_ascii_strtod (argv[2], NULL);
        zoom_factor += 1.0;
        o->dir_scale /= zoom_factor;
      }
      else
      {
        o->dir_scale = g_ascii_strtod(argv[1], NULL);
        if (o->dir_scale < 0.0001 || o->dir_scale > 200.0)
          o->dir_scale = 1;
      }

      if (o->dir_scale > 2.2) o->dir_scale = 2.2;
      if (o->dir_scale < 0.1) o->dir_scale = 0.1;

      ui_center_active_entry (o);

      mrg_queue_draw (o->mrg, NULL);
      return 0;
  }

  if (!strcmp(argv[1], "fit"))
  {
    zoom_to_fit (o);
    return 0; // to keep fit state
  }
  else if (!strcmp(argv[1], "in"))
  {
    float zoom_factor = 0.1;
    if (argv[2])
      zoom_factor = g_ascii_strtod (argv[2], NULL);
    zoom_factor += 1.0;

    zoom_at (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, zoom_factor);
  }
  else if (!strcmp(argv[1], "out"))
  {
    float zoom_factor = 0.1;
    if (argv[2])
      zoom_factor = g_ascii_strtod (argv[2], NULL);
    zoom_factor += 1.0;

    zoom_at (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, 1.0f/zoom_factor);
  }
  else
  {
    float x, y;
    get_coords (o, mrg_width(o->mrg)/2, mrg_height(o->mrg)/2, &x, &y);
    o->scale = g_ascii_strtod(argv[1], NULL);
    o->u = x * o->scale - mrg_width(o->mrg)/2;
    o->v = y * o->scale - mrg_height(o->mrg)/2;
    mrg_queue_draw (o->mrg, NULL);
  }
  o->is_fit = 0;
  return 0;
}


int cmd_propeditor (COMMAND_ARGS); /* "prop-editor", 1, "<subcommand>", "used for property editing keybindings"*/
int
cmd_propeditor (COMMAND_ARGS)
{
  GeState *o = global_state;
  GParamSpec *pspec = o->property_focus?gegl_node_find_property (o->active, o->property_focus):NULL;


  if (!strcmp (argv[1], "left") ||
      !strcmp (argv[1], "shift-left"))
  {
    if (!pspec)
      return 0;
    if (g_type_is_a (pspec->value_type, G_TYPE_DOUBLE))
    {
      double value;
      double step = 1.0;
      if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
      {
        if (!strcmp (argv[1], "shift-left"))
          step = GEGL_PARAM_SPEC_DOUBLE (pspec)->ui_step_big;
        else
          step = GEGL_PARAM_SPEC_DOUBLE (pspec)->ui_step_small;
      }

      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value -= step;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_INT) ||
             g_type_is_a (pspec->value_type, G_TYPE_ENUM))
    {
      int value;
      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value -= 1;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_STRING) ||
             g_type_is_a (pspec->value_type, GEGL_TYPE_PARAM_FILE_PATH))
    {
    }
    else if (g_type_is_a (pspec->value_type, GEGL_TYPE_COLOR))
    {
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_BOOLEAN))
    {
      gboolean value;
      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value = !value;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else
    {
    }

    rev_inc (o);
  }
  else if (!strcmp (argv[1], "right")||
           !strcmp (argv[1], "shift-right"))
  {
    if (!pspec)
      return 0;
    if (g_type_is_a (pspec->value_type, G_TYPE_DOUBLE))
    {
      double value;
      double step = 1.0;
      if (GEGL_IS_PARAM_SPEC_DOUBLE (pspec))
      {
        if (!strcmp (argv[1], "shift-right"))
          step = GEGL_PARAM_SPEC_DOUBLE (pspec)->ui_step_big;
        else
          step = GEGL_PARAM_SPEC_DOUBLE (pspec)->ui_step_small;
      }
      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value += step;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_INT) ||
             g_type_is_a (pspec->value_type, G_TYPE_ENUM))
    {
      int value;
      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value += 1;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_STRING) ||
             g_type_is_a (pspec->value_type, GEGL_TYPE_PARAM_FILE_PATH))
    {
    }
    else if (g_type_is_a (pspec->value_type, GEGL_TYPE_COLOR))
    {
    }
    else if (g_type_is_a (pspec->value_type, G_TYPE_BOOLEAN))
    {
      gboolean value;
      gegl_node_get (o->active, o->property_focus, &value, NULL);
      value = !value;
      gegl_node_set (o->active, o->property_focus, value, NULL);
    }
    else
    {
    }

    rev_inc (o);
  }
  else if (!strcmp (argv[1], "focus"))
  {
    if (o->property_focus)
    {
      o->property_focus = NULL;
    }
    else
    {
      o->property_focus = g_intern_string ("operation");
    }
  }
  else if (!strcmp (argv[1], "down"))
  {
    GParamSpec **pspecs = NULL;
    unsigned int n_props = 0;
    int i, next = -1;
    pspecs = gegl_operation_list_properties (gegl_node_get_operation (o->active), &n_props);
    for (i = 0; i < n_props; i++)
      if (g_intern_string (pspecs[i]->name) == o->property_focus)
      {
        next = i; break;
      }
    next++;
    if (next < n_props)
      o->property_focus = g_intern_string (pspecs[next]->name);

    g_free (pspecs);
  }
  else if (!strcmp (argv[1], "up"))
  {
    GParamSpec **pspecs = NULL;
    unsigned int n_props = 0;
    int i, next = -1;
    pspecs = gegl_operation_list_properties (gegl_node_get_operation (o->active), &n_props);
    for (i = 0; i < n_props; i++)
      if (g_intern_string (pspecs[i]->name) == o->property_focus)
      {
        next = i; break;
      }
    next--;
    if (next >= 0)
      o->property_focus = g_intern_string (pspecs[next]->name);
    else
      o->property_focus = g_intern_string ("operation");

    g_free (pspecs);
  }
  else if (!strcmp (argv[1], "return"))
  {
    set_edited_prop (NULL, NULL, (void*)o->property_focus);
  }

  mrg_queue_draw (o->mrg, NULL);
  return 0;
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
  queue_draw (global_state);
  return 0;
}

 int cmd_next (COMMAND_ARGS);
int cmd_next (COMMAND_ARGS) /* "next", 0, "", "next sibling element in current collection/folder"*/
{
  GeState *o = global_state;
  if (o->rev)
    argvs_eval ("save");

  if (o->entry_no >= ui_items_count (o) -1)
    return 0;
  o->entry_no ++;
    //o->entry_no = 0;

  {
    char *new_path = get_item_path_no (o, o->entry_no);
    g_free (o->path);
    o->path = new_path;
    ui_load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }

  activate_sink_producer (o);
  return 0;
}

 int cmd_parent (COMMAND_ARGS);
int cmd_parent (COMMAND_ARGS) /* "parent", 0, "", "enter parent collection (switches to folder mode)"*/
{
  GeState *o = global_state;
  char *prev_path = g_strdup (o->path);
  char *prev_basename = g_path_get_basename (o->path);
  char *lastslash = strrchr (o->path, '/');
  int entry_no = 0;

  o->playing = 0;
  if (o->rev)
    argvs_eval ("save");

  if (lastslash)
  {
    if (lastslash == o->path)
      lastslash[1] = '\0';
    else
      lastslash[0] = '\0';

    ui_load_path (o);
    if (g_file_test (prev_path, G_FILE_TEST_IS_DIR))
    {
      int no = 0;
      for (GList *i = o->index; i; i=i->next, no++)
      {
        IndexItem *item = i->data;
        if (!strcmp (item->name, prev_basename))
        {
          entry_no = no;
          goto yep;
        }
      }
      for (GList *i = o->paths; i; i=i->next, no++)
      {
        if (!strcmp (i->data, prev_path))
        {
          entry_no = no;
          goto yep;
        }
      }

      yep:
        do{}while(0);
    }

    if (entry_no)
    {
      o->entry_no = entry_no;

    }
    ui_center_active_entry (o);
    mrg_queue_draw (o->mrg, NULL);
  }
  g_free (prev_path);
  g_free (prev_basename);
  o->active = NULL;
  return 0;
}

 int cmd_prev (COMMAND_ARGS);
int cmd_prev (COMMAND_ARGS) /* "prev", 0, "", "previous sibling element in current collection/folder"*/
{
  GeState *o = global_state;
  //GList *curr = g_list_find_custom (o->paths, o->path, (void*)g_strcmp0);
  if (o->rev)
    argvs_eval ("save");

  if (o->entry_no>0)
    o->entry_no--;

  {
    char *new_path = get_item_path_no (o, o->entry_no);
    g_free (o->path);
    o->path = new_path;
    ui_load_path (o);
    mrg_queue_draw (o->mrg, NULL);
  }

  activate_sink_producer (o);
  return 0;
}

 int cmd_load (COMMAND_ARGS);
int cmd_load (COMMAND_ARGS) /* "load-path", 1, "<path>", "load a path/image - can be relative to current pereived folder "*/
{
  GeState *o = global_state;
  
  if (o->path)
    g_free (o->path);
  o->path = g_strdup (argv[1]);

  ui_load_path (o);

  activate_sink_producer (o);
  return 0;
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
  GeState *o = global_state;
  GeglNode *node = o->active;
  GeglNode *next, *prev;

  const gchar *consumer_name = NULL;

  if (o->active == o->source)
    return -1;

  switch (o->pad_active)
  {
    case PAD_INPUT:
      prev = gegl_node_get_producer (node, "input", NULL);
      if (gegl_node_get_ui_consumer (prev, "output", NULL) != node)
        gegl_node_disconnect (node, "input");
      break;
    case PAD_AUX:
      prev = gegl_node_get_producer (node, "aux", NULL);
      if (gegl_node_get_ui_consumer (prev, "output", NULL) != node)
        gegl_node_disconnect (node, "aux");
      break;
    case PAD_OUTPUT:
      prev = gegl_node_get_producer (node, "input", NULL);
      next = gegl_node_get_ui_consumer (node, "output", &consumer_name);

      if (next && prev)
      {
        gegl_node_disconnect (node, "input");
        gegl_node_connect_to (prev, "output", next, consumer_name);
        gegl_node_remove_child (o->gegl, node);
        o->active = prev;
      }
      else if (next)
      {
        gegl_node_disconnect (next, consumer_name);
        gegl_node_remove_child (o->gegl, node);
        o->active = next;
      }
    break;
  }

  rev_inc (o);
  return 0;
}

int cmd_swap (COMMAND_ARGS);/* "swap", 1, "<input|output>", "swaps position with other node, allows doing the equivalent of raise lower and other local reordering of nodes."*/

int
cmd_swap (COMMAND_ARGS)
{
  GeState *o = global_state;
  GeglNode *node = o->active;
  GeglNode *next, *prev;
  const char *consumer_name = NULL;

  next = gegl_node_get_ui_consumer (node, "output", &consumer_name);
  prev = gegl_node_get_ui_producer (node, "input", NULL);
  consumer_name = g_intern_string (consumer_name?consumer_name:"");

  if (1)//next && prev)
    {
      if (!strcmp (argv[1], "output") && next != o->sink)
      {
        const char *next_next_consumer = NULL;
        GeglNode *next_next = gegl_node_get_ui_consumer (next, "output", &next_next_consumer);

        if (next_next && g_str_equal (consumer_name, "input"))
        {
          gegl_node_disconnect (next_next, next_next_consumer);
          gegl_node_disconnect (node, "input");
          gegl_node_disconnect (next, "input");
          if (prev)
            gegl_node_link_many (prev, next, node, NULL);
          else
            gegl_node_link_many (next, node, NULL);
          gegl_node_connect_to (node, "output", next_next, next_next_consumer);
        }
      }
      else if (prev && !strcmp (argv[1], "input") && prev != o->source)
      {
        GeglNode *prev_prev = gegl_node_get_ui_producer (prev, "input", NULL);

        if (prev_prev)
        {
          gegl_node_link_many (prev_prev, node, prev, NULL);
          gegl_node_connect_to (prev, "output", next, consumer_name);
        }
        else
        {
          if (gegl_node_has_pad (prev, "input"))
          {
            gegl_node_disconnect (next, consumer_name);
            gegl_node_disconnect (node, "input");
            gegl_node_link_many (node, prev, NULL);
            gegl_node_connect_to (prev, "output", next, consumer_name);
          }
        }
      }

    }

  rev_inc (o);
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
  int cmd_pan (COMMAND_ARGS);
int cmd_pan (COMMAND_ARGS) /* "pan", 0, "", "changes to pan tool"*/
{
  tool = TOOL_PAN;
  return 0;
}

  int cmd_find_id (COMMAND_ARGS);
int cmd_find_id (COMMAND_ARGS) /* "/", 1, "<id-to-jump-to>", "set focus on node with given id"*/
{
  GeState *o = global_state;
  GeglNode *found = node_find_by_id (o, o->sink, argv[1]);

  if (found)
    o->active = found;
  else
    printf ("no node with id %s found", argv[1]);

  mrg_queue_draw (o->mrg, NULL);

  return 0;
}

  int cmd_edit_opname (COMMAND_ARGS);
int cmd_edit_opname (COMMAND_ARGS) /* "edit-opname", 0, "", "permits changing the current op by typing in a replacement name."*/
{
  GeState *o = global_state;
  o->editing_op_name = 1;
  o->editing_buf[0]=0;
  mrg_set_cursor_pos (o->mrg, 0);
  return 0;
}


  int cmd_graph_cursor (COMMAND_ARGS);
int cmd_graph_cursor (COMMAND_ARGS) /* "graph-cursor", 1, "<left|right|up|down|source|append>", "position the graph cursor, this navigates both pads and nodes simultanously."*/
{
  GeState *o = global_state;
  GeglNode *ref;

  if (o->active == NULL)
  {
    activate_sink_producer (o);
    if (!o->active)
      return -1;
  }
  ref = o->active;

  if (!strcmp (argv[1], "down"))
  {
    switch (o->pad_active)
    {
       case PAD_AUX:
        ref = gegl_node_get_ui_producer (o->active, "aux", NULL);
        if (ref == NULL)
          o->pad_active = PAD_INPUT;
        else
          o->pad_active = PAD_OUTPUT;
        break;

       case PAD_INPUT:
        ref = gegl_node_get_producer (o->active, "input", NULL);
        if (ref == NULL)
          o->pad_active = PAD_INPUT;
        else
          o->pad_active = PAD_OUTPUT;


       break;
       case PAD_OUTPUT:
        ref = gegl_node_get_ui_producer (o->active, "input", NULL);
        if (ref == NULL)
          o->pad_active = PAD_INPUT;
        else
          o->pad_active = PAD_OUTPUT;
       break;

    }
  }
  else if (!strcmp (argv[1], "right"))
  {
    if (o->pad_active == PAD_AUX)
    {
      ref = gegl_node_get_producer (o->active, "aux", NULL);
      if (!ref)
      {
        ref = o->active;
        if (gegl_node_has_pad (o->active, "aux"))
          o->pad_active = PAD_AUX;
        else if (gegl_node_has_pad (o->active, "input"))
          o->pad_active = PAD_INPUT;
    }
    else
    {
        o->pad_active = PAD_OUTPUT;
    }
    }
    else
    {
      if (gegl_node_has_pad (o->active, "aux"))
        o->pad_active = PAD_AUX;
      else if (gegl_node_has_pad (o->active, "input"))
        o->pad_active = PAD_INPUT;
      else
        o->pad_active = PAD_OUTPUT;
    }
  }
  else if (!strcmp (argv[1], "up"))
  {
    if (o->pad_active != PAD_OUTPUT)
    {
      o->pad_active = PAD_OUTPUT;
    }
    else
    {
      ref = gegl_node_get_ui_consumer (o->active, "output", NULL);
      if (ref == o->sink)
        ref = NULL;
      o->pad_active = PAD_OUTPUT;
    }
  }
  else if (!strcmp (argv[1], "left"))
  {
    GeglNode *iter = o->active;
    int skips = 0;

    if (o->pad_active == PAD_INPUT)
    {
      o->pad_active = PAD_OUTPUT;
    }
    else if (o->pad_active == PAD_AUX)
    {
      o->pad_active = PAD_INPUT;
    }
    else
    {

    if (o->pad_active != PAD_OUTPUT)
    {
      o->pad_active = PAD_OUTPUT;
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

    //if (skips == 0)
    {
      GeglNode *attempt = gegl_node_get_ui_consumer (ref, "output", NULL);
      if (attempt && attempt != o->sink)
        ref = attempt;
    }

   }
  }
  else if (!strcmp (argv[1], "append"))
  {
    ref = gegl_node_get_producer (o->sink, "input", NULL);
    o->pad_active = PAD_OUTPUT;
  }
  else if (!strcmp (argv[1], "source"))
  {
    ref = o->source;
    o->pad_active = PAD_OUTPUT;
  }
  else {
    printf ("unkown graph cursor sub command %s\n", argv[1]);
    ref = NULL;
  }

  if (ref)
    o->active = ref;
  mrg_queue_draw (o->mrg, NULL);
  return 0;
}


int cmd_reference (COMMAND_ARGS);/* "reference", -1, "", ""*/
int
cmd_reference (COMMAND_ARGS)
{
  GeState *o = global_state;
  o->reference_node = o->active;
  return 0;
}

int cmd_dereference (COMMAND_ARGS);/* "dereference", -1, "", ""*/
int
cmd_dereference (COMMAND_ARGS)
{
  GeState *o = global_state;

  if (o->reference_node)
  switch (o->pad_active)
  {
    case PAD_INPUT:
    case PAD_OUTPUT:
      gegl_node_link_many (o->reference_node, o->active, NULL);
      break;
    case PAD_AUX:
      gegl_node_connect_to (o->reference_node, "output", o->active, "aux");
      break;
  }

  rev_inc (o);
  return 0;
}


int cmd_mipmap (COMMAND_ARGS);/* "mipmap", -1, "", ""*/

int
cmd_mipmap (COMMAND_ARGS)
{
  gboolean curval;
  GeState *o = global_state;
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
  rev_inc (o);
  return 0;
}

int cmd_node_add (COMMAND_ARGS);/* "node-add", 1, "<input|output|aux>", "add a neighboring node and permit entering its name, for use in touch ui."*/
int
cmd_node_add (COMMAND_ARGS)
{
  GeState *o = global_state;
  if (!strcmp(argv[1], "input"))
  {
    GeState *o = global_state;
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
    o->editing_buf[0]=0;
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
    o->editing_buf[0]=0;
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
      if (o->mrg) mrg_set_cursor_pos (o->mrg, 0);
      o->editing_buf[0]=0;
    }
  }
  if (o->mrg) rev_inc (o);
  return 0;
}

int cmd_about (COMMAND_ARGS);/* "about", -1, "", ""*/
int
cmd_about (COMMAND_ARGS)
{
  printf (
"This is an integrated image browser, viewer and editor using GEGL.\n"
"It is a testbed for studying and improving GEGL in operation in isolation, \n"
"It uses micro-raptor GUI to provide interactivity and CSS layout and\n"
"styling on top of cairo for the user interface. For the graph editor\n"
"GEGLs native data representation is used as the scene-graph.\n"
"\n"
"The internal commandline is a fallback for easy development, and the\n"
"basis that event dispatch for pointer/touch events and keybindings\n"
"are dispatched.\n"
"\n"
"Thumbnails are stored in ~/.cache/gegl-0.6/thumbnails as 256x256jpg\n"
"files, the thumbnails kept up to date reflecting any edits, thumbnailing\n"
"happens on demand by starting a second instance with a batch of paths - or\n"
"when leaving a modified image for to view/edit another."
"\n"
"File types supported are: gegl, jpg, svg, png, tif, exr, gif, mp4, avi, mpg.\n"
"video and gif files are opened looping.\n"
"Source files are left intact, modifications are stored in a corresponding .gegl file\n"
"next to the sources. .gegl documents without a corresponding document when\n"
"the name is stripped are treated as separate documents, .gegl files may\n"
"contain references to multiple source images - but starting out with a photo\n"
"is the most common use case\n"
"\n");
  return 0;
}

int cmd_todo (COMMAND_ARGS);/* "todo", -1, "", ""*/
int
cmd_todo (COMMAND_ARGS)
{
  printf ("store timed and named revisions of documents\n");
  printf ("export panel, with named, scaled and cropped settings previously used for this image and others.\n");
  printf ("tab-completion for cd command\n");
  printf ("visual color picker\n");
  printf ("more per-op lua uis\n");
  printf ("histogram for threshold/levels/curve uis\n");
  printf ("make axis constrained vertical drag up/down adjust linear small increments on double\n");
  printf ("units in commandline\n");
  printf ("interpret GUM\n");
  printf ("star/comment/title storage\n");
  printf ("rewrite of core in lua?\n");
  printf ("keep track of \"orphaned\" nodes as free-floating new columns\n");
  printf ("video/audio playback time controls\n");
  printf ("animation curves for properties\n");
  printf ("dir actions: rename, discard\n");
  printf ("setting of id in ui?\n");
  printf ("context/pie/tool menu/slab\n");

  return 0;
}

static void
_meta_unset_key (GeState *state,const char *path, const char *key)
{
  gchar *metadata_path = ui_get_metadata_path (path);
  gchar *contents = NULL;

  g_file_get_contents (metadata_path, &contents, NULL, NULL);
  if (contents)
  {
    char *line = contents;

    while (*line)
    {
      if (0==memcmp (line, key, strlen (key)) &&
          line[strlen(key)]=='=')
      {
        char *start = &line[strlen(key)+1];
        char *end = start;

        for (end = start; *end != '\n' && *end != '\0'; end++);
        if (*end == 0)
        {
          *line = 0;
        }
        else
        {
          memmove (line, end + 1, strlen (end));
        }
        goto prepped;
      }

        while (*line && *line != '\n')
        {
          line++;
        }
        if (*line == '\n')
          line++;
      }

    prepped:
    g_file_set_contents (metadata_path, contents, -1, NULL);

    g_free (contents);
  }

  g_free (metadata_path);
}

void
meta_set_key (GeState *state,const char *path, const char *key, const char *value)
{
  gchar *metadata_path;
  gchar *contents = NULL;
  char *alloc_value = NULL;

  if (value == NULL)
  {
    _meta_unset_key (state, path, key);
    return;
  }

  metadata_path  = ui_get_metadata_path (path);
  if (strchr (value, '\n'))
  {
    const char *src;
    char *dst = alloc_value;
    alloc_value = g_malloc (strlen (value) * 2 + 1);
    for (src = value; *src; src++)
    {
      if (*src == '\n')
      {
        *dst = '\\';
        dst++;
        *dst = 'n';
        dst++;
      }
      else
      {
        *dst = *src;
        dst++;
      }
    }
    *dst = 0;
    value = alloc_value;
  }

  g_file_get_contents (metadata_path, &contents, NULL, NULL);
  if (contents)
  {
    char *line = contents;

    while (*line)
    {
      if (0==memcmp (line, key, strlen (key)) &&
          line[strlen(key)]=='=')
      {
        char *start = &line[strlen(key)+1];
        char *end = start;

        for (end = start; *end != '\n' && *end != '\0'; end++);
        if (*end == 0)
        {
          *line = 0;
        }
        else
        {
          memmove (line, end + 1, strlen (end));
        }
        goto prepped;
      }

        while (*line && *line != '\n')
        {
          line++;
        }
        if (*line == '\n')
          line++;
      }

    prepped:
    {
      char *str = g_strdup_printf ("%s%s=%s\n", contents, key, value);
      g_file_set_contents (metadata_path, str, -1, NULL);
      g_free (str);
    }

    g_free (contents);
  }
  else
  {
    char *str = g_strdup_printf ("%s=%s\n", key, value);
    char *dirname = g_path_get_dirname (metadata_path);
    g_mkdir_with_parents (dirname, 0777);
    g_free (dirname);

    g_file_set_contents (metadata_path, str, -1, NULL);
    g_free (str);
  }

  g_free (metadata_path);
  if (alloc_value)
    g_free (alloc_value);
}

static IndexItem *
index_item_new (void)
{
  return g_malloc0 (sizeof (IndexItem));
}

static void
index_item_destroy (IndexItem *item)
{
  g_free (item->name);
  for (int i = 0; i < INDEX_MAX_ATTRIBUTES; i++)
  {
    if (item->attribute[i])
      g_free (item->attribute[i]);
    if (item->detail[i])
      g_free (item->detail[i]);
  }
}

static void
store_index (GeState *state, const char *path)
{
  GString *str;

  struct stat stat_buf;
  char *dirname;
  char *index_path = NULL;
  lstat (path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {
    dirname = g_path_get_dirname (path);
  }
  else if (S_ISDIR (stat_buf.st_mode))
  {
    dirname = g_strdup (path);
  }
  else
  {
    return;
  }
  str = g_string_new ("");
  index_path = ui_get_index_path (dirname);

  for (GList *iter = state->index; iter; iter=iter->next)
  {
    IndexItem *item = iter->data;
    g_string_append_printf (str, "%s\n", item->name);
    for (int i = 0; i < INDEX_MAX_ATTRIBUTES; i++)
    {
      if (item->attribute[i])
      {
        g_string_append_printf (str, "\t%s\n", item->attribute[i]);
        if (item->detail[i])
        {
          g_string_append_printf (str, "\t\t%s\n", item->detail[i]);
        }
      }
    }
  }
  g_file_set_contents (index_path, str->str, -1, NULL);

  g_free (dirname);
  g_free (index_path);
  g_string_free (str, TRUE);
}

static void
load_index (GeState *state, const char *path)
{
  /* if path is not a folder load its parent */
  struct stat stat_buf;
  char *index_path = NULL;
  gchar *contents = NULL;
  gchar *dirname = NULL;

  while (state->index)
  {
    index_item_destroy (state->index->data);
    state->index = g_list_remove (state->index, state->index->data);
  }

  lstat (path, &stat_buf);
  if (S_ISREG (stat_buf.st_mode))
  {
    dirname = g_path_get_dirname (path);
  }
  else if (S_ISDIR (stat_buf.st_mode))
  {
    dirname = g_strdup (path);
  }
  else
  {
    return;
  }
  index_path = ui_get_index_path (dirname);

  g_file_get_contents (index_path, &contents, NULL, NULL);
  if (contents)
  {
    char *line = contents;

    int   child_no = -1;
    char *name = NULL;
    char *attribute = NULL;
    char *detail = NULL;
    while (*line)
    {
      char *nextline = line;
      /* skip to next line */
      while (*nextline && *nextline != '\n')
        nextline++;
      if (*nextline == '\n')
        nextline++;

      if (line[0] != '\t') /* name */
      {
         char *end;
         name = line;
         for (end = name; *end && *end != '\n'; end++);
         *end = 0;
         child_no ++;
         meta_insert_child (state, dirname, child_no, name);
      }
      else /* key or value*/
      {
        if (line[1] != '\t') /* attribute */
        {
          char *end;
          attribute = &line[1];
          for (end = attribute; *end && *end != '\n'; end++);
          *end = 0;
        }
        else /* detail */
        {
          char *end;
          detail = &line[2];
          for (end = detail; *end && *end != '\n'; end++);
          *end = 0;
          meta_set_attribute (state, dirname, child_no, attribute, detail);
        }
      }
      line = nextline;
    }
    g_free (contents);
  }
  g_free (dirname);
  if (index_path)
    g_free (index_path);
  state->index_dirty = 0;
}



const char *
meta_get_key (GeState *state, const char *path, const char *key)
{
  const char *ret = NULL;
  gchar *metadata_path = ui_get_metadata_path (path);
  gchar *contents = NULL;
  g_file_get_contents (metadata_path, &contents, NULL, NULL);
  if (contents)
  {
    char *line = contents;
    while (*line)
    {

      if (0==memcmp (line, key, strlen (key)) &&
        line[strlen(key)]=='=')
      {
        char *start = &line[strlen(key)+1];
        char *end = start;
        for (end = start; *end != '\n' && *end != '\0'; end++);
        *end = 0;

        for (char *p = start; *p && p != end && *p != '\n'; p++)
        {
          if (p[0] == '\\' && p[1]=='n')
          {
            p[0] = '\n';
            memmove (line, end + 1, strlen (end));
          }
        }

        ret = g_intern_string (start);

        g_free (contents);
        return ret;
      }

      while (*line && *line != '\n')
      {
        line++;
      }
      if (*line == '\n')
        line++;
    }
    g_free (contents);
  }
  return NULL;
}


void
meta_insert_child (GeState *state,const char *path,
                   int         value_no,
                   const char *child_name)
{
  IndexItem *item = index_item_new ();
  item->name = g_strdup (child_name);
  state->index = g_list_insert (state->index, item, value_no);
  state->index_dirty ++;
}

int
meta_remove_child (GeState *state,const char *path,
                   int         value_no, /* or -1 to remove first match
                                            or -2 to remove all matching child_name,
                                            or specific number >=0 to remove specific */
                   const char *child_name)
{
  int no;
  int ret = -1;
  GList *iter;

  again:
  for (iter = state->index, no=0; iter;iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (child_name)
    {
      if (!strcmp (child_name, item->name))
      {
        if (value_no == -1)
        {
          state->index = g_list_remove (state->index, item);
          index_item_destroy (item);
          state->index_dirty++;
          return no;
        }
        else if (value_no == -2)
        {
          state->index = g_list_remove (state->index, item);
          index_item_destroy (item);
          ret = no;
          state->index_dirty++;
          goto again;
        }
        else if (value_no == no)
        {
          state->index = g_list_remove (state->index, item);
          index_item_destroy (item);
          state->index_dirty++;
          return no;
        }
        else
        {
          g_assert (0);
        }
      }
    }
    else
    {
      g_assert (value_no>=0);
      if (value_no == no)
      {
        state->index = g_list_remove (state->index, item);
        index_item_destroy (item);
        return no;
      }
    }
  }

  return ret;
}

void
meta_replace_child (GeState *state,const char *path,
                    int         old_value_no, /* or -1 for first matching old_child_name */
                    const char *old_child_name, /* NULL to use >=0 old_value_no or a string */
                    const char *new_child_name)
{
  int old_val_no = meta_remove_child (state, path, old_value_no, old_child_name);
  meta_insert_child (state, path, old_val_no, new_child_name);
}

static
void turn_paths_into_index (GeState *o)
{
  //fprintf (stderr, "turn paths into index\n");
  while (o->paths)
   {
      char *basename = g_path_get_basename (o->paths->data);
      meta_insert_child (o, o->path, -1, basename);
      g_free (basename);
      g_free (o->paths->data);
      o->paths = g_list_remove (o->paths, o->paths->data);
   }

}




void
meta_swap_children (GeState *o,const char *path,
                    int         value_no1, /* -1 to use only name */
                    const char *child_name1,  /* or NULL to use no1 (which cannot be -1) */
                    int         value_no2,
                    const char *child_name2)
{
  GList *iter_A, *iter_B;
  /* XXX only works with ascening ordered value_no1 and value_no2 arguments for now */
  int index_len = g_list_length (o->index);
  if (value_no1 >= index_len)
  {
    turn_paths_into_index (o);
    index_len = g_list_length (o->index);
  }
  if (value_no2 >= index_len)
  {
    turn_paths_into_index (o);
    index_len = g_list_length (o->index);
  }


  iter_A =  g_list_nth (o->index, value_no1);
  iter_B =  g_list_nth (o->index, value_no2);

  o->index = g_list_remove_link (o->index, iter_A);
  o->index = g_list_remove_link (o->index, iter_B);

  o->index = g_list_insert (o->index, iter_B->data, value_no1);
  o->index = g_list_insert (o->index, iter_A->data, value_no2);

  g_list_free (iter_A);
  g_list_free (iter_B);

  o->index_dirty ++;
}

static void
_meta_unset_attribute (GeState    *state,
                       const char *path,
                       int         value_no,
                       const char *attribute)
{
  GList *iter;
  int no;
  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == value_no)
    {
      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano] && !strcmp (item->attribute[ano], attribute))
        {
          g_free (item->attribute[ano]);
          item->attribute[ano] = NULL;
          if (item->detail[ano])
            g_free (item->detail[ano]);
          item->detail[ano] = NULL;
          state->index_dirty ++;
        }
      }
    }
  }
}


void
meta_set_attribute (GeState    *state,
                    const char *path,
                    int         value_no,
                    // also have child name
                    const char *attribute,
                    const char *detail)
{
  GList *iter;
  int no;
  if (detail == NULL)
    {
      _meta_unset_attribute (state, path, value_no, attribute);
      return;
    }

  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == value_no)
    {
      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano] && !strcmp (item->attribute[ano], attribute))
        {
          if (item->detail[ano])
          {
            g_free (item->detail[ano]);
            item->detail[ano] = NULL;
          }
          if (detail)
          {
            item->detail[ano] = g_strdup (detail);
          }
          state->index_dirty ++;
          return;
        }
      }

      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano] == NULL)
        {
          if (item->detail[ano])
          {
            g_free (item->detail[ano]);
            item->detail[ano] = NULL;
          }
          if (detail)
          {
            item->detail[ano] = g_strdup (detail);
          }
          item->attribute[ano] = g_strdup (attribute);
          state->index_dirty ++;
          return;
        }
      }
    }
  }
}

const char *
meta_get_attribute (GeState    *state,
                    const char *path,
                    int         value_no,
                    const char *attribute)
{
  GList *iter;
  int no;
  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == value_no)
    {
      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano] && !strcmp (item->attribute[ano], attribute))
        {
          return item->detail[ano];
        }
      }
    }
  }

  /* XXX; fall back to loading key/value ? */

  return NULL;
}



int
meta_has_attribute (GeState    *state,
                    const char *path,
                    int         value_no,
                    const char *attribute)
{
  GList *iter;
  int no;
  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == value_no)
    {
      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano] && !strcmp (item->attribute[ano], attribute))
        {
          return 1;
        }
      }
    }
  }
  return 0;
}

char **
meta_list_keys (GeState    *state,
                const char *path)
{
  int count = 0;
  int tot_len = 0;
  int array_byte_size = 0;
  char **pasp = NULL;
  char  *pasp_data = NULL;
  gchar *metadata_path = ui_get_metadata_path (path);
  gchar *contents = NULL;
  g_file_get_contents (metadata_path, &contents, NULL, NULL);
  if (contents)
  {
    char *line = contents;
    while (*line)
    {
      if (strchr (line, '='))
      {
      char *key = g_strdup (line);
      count++;
      *strchr (key, '=') = 0;
      tot_len += strlen (key);
      g_free (key);
      }

      while (*line && *line != '\n')
      {
        line++;
      }
      if (*line == '\n')
        line++;
    }

    array_byte_size = (count + 1) * sizeof (char *);
    pasp = g_malloc0 ( array_byte_size + count + tot_len);
    pasp_data = &(((char*)(pasp))[array_byte_size]);
    count = 0;

    line = contents;
    while (*line)
    {
      if (strchr (line, '='))
      {
        char *key = g_strdup (line);
        *strchr (key, '=') = 0;

        pasp[count++] = pasp_data;
        strcpy (pasp_data, key);
        pasp_data += strlen (key) + 1;

        g_free (key);
      }

      while (*line && *line != '\n')
      {
        line++;
      }
      if (*line == '\n')
        line++;
    }

    g_free (contents);
  }
  return pasp;
}

char **
meta_list_attributes (GeState    *state,
                      const char *path,
                      int         item_no)
{
  GList *iter;
  int count = 0;
  int tot_len = 0;
  int no;
  int array_byte_size = 0;
  char **pasp = NULL;
  char  *pasp_data = NULL;

  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == item_no)
    for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
    {
      if (item->attribute[ano])
      {
        count++;
        tot_len += strlen (item->attribute[ano]);
      }
    }
  }

  if (count == 0)
    return NULL;

  array_byte_size = (count + 1) * sizeof (char *);
  pasp = g_malloc0 ( array_byte_size + count + tot_len);
  pasp_data = &(((char*)(pasp))[array_byte_size]);
  no = 0;
  count = 0;

  for (iter = state->index, no = 0; iter; iter=iter->next, no++)
  {
    IndexItem *item = iter->data;
    if (no == item_no)
      for (int ano = 0; ano < INDEX_MAX_ATTRIBUTES; ano++)
      {
        if (item->attribute[ano])
        {
          pasp[count] = pasp_data;
          strcpy (pasp[count++], item->attribute[ano]);
          pasp_data += strlen (item->attribute[ano]) + 1;
        }
      }
  }
  return pasp;
}


#if 1
static int
index_contains (GeState *state,
                const char *name)
{
  for (GList *iter = state->index; iter; iter = iter->next)
  {
    IndexItem *item = iter->data;
    if (!strcmp (item->name, name))
      return 1;
  }
  return 0;
}
#endif

char *
meta_get_child (GeState    *state,
                const char *path,
                int         child_no)
{
  int items = g_list_length (state->index);
  GList *iter;
  int no = 0;
  if (child_no >= 0 && child_no < items) {
    IndexItem *item = g_list_nth_data (state->index, child_no);
    return g_strdup (item->name);
  }
  no = items;
  for (iter = state->paths; iter; iter=iter->next)
  {
    char *basename = g_path_get_basename (iter->data);
//    if (!index_contains (state, basename))
    {
      if (no == child_no)
      {
        return basename;
      }
      no++;
    }
    g_free (basename);
  }
  return NULL;
}

/* integer/float abstraction for path key/values */

void
meta_set_key_int (GeState    *state,
                  const char *path,
                  const char *key,
                  int         value)
{
  char *buf = g_strdup_printf ("%i", value);
  meta_set_key (state, path, key, buf);
  g_free (buf);
}

int
meta_get_key_int (GeState    *state,
                  const char *path,
                  const char *key)
{
  const char *value = meta_get_key (state, path, key);
  if (!value)
    return -999999;
  return atoi (value);
}

int
meta_get_attribute_int (GeState    *state,
                        const char *path,
                        int         value_no,
                        const char *attribute)
{
  const char *value = meta_get_attribute (state, path, value_no, attribute);
  if (!value)
    return -999999;
  return atoi (value);
}

float
meta_get_attribute_float (GeState    *state,
                          const char *path,
                          int         value_no,
                          const char *attribute)
{
  const char *value = meta_get_attribute (state, path, value_no, attribute);
  if (!value)
    return -999999.99999;
  return g_ascii_strtod (value, NULL);
}

void
meta_set_key_float (GeState    *state,
                    const char *path,
                    const char *key,
                    float       value)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
  meta_set_key (state, path, key, buf);
}

void
meta_set_attribute_float (GeState    *state,
                          const char *path,
                          int         value_no,
                          // also have child name
                          const char *attribute,
                          float       detail)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, detail);
  meta_set_attribute (state, path, value_no, attribute, buf);
}

void
meta_set_attribute_int (GeState    *state,
                        const char *path,
                        int         value_no,
                        // also have child name
                        const char *attribute,
                        int         detail)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  sprintf (buf, "%d", detail);
  meta_set_attribute (state, path, value_no, attribute, buf);
}

float
meta_get_key_float (GeState    *state,
                    const char *path,
                    const char *key)
{
  const char *value = meta_get_key (state, path, key);
  if (!value)
    return -999999.99999;
  return g_ascii_strtod (value, NULL);
}

#endif

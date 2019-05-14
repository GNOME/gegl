
ffi = require('ffi')
lgi = require 'lgi'
math = require 'math'

os      = require 'os'
GLib    = lgi.GLib
GObject = lgi.GObject
Mrg     = require('mrg')
Gegl    = lgi.Gegl

ffi.cdef[[

typedef void* GList;
typedef void* GeglNode;
typedef void* GeglBuffer;
typedef void* GeglProcessor;
typedef void* GThread;
typedef void* GHashTable;

enum _SortOrder
{
  SORT_ORDER_AZ         = 1,
  SORT_ORDER_MTIME      = 2,
  SORT_ORDER_EXIF_TIME  = 4,
  SORT_ORDER_STARS      = 8,
  SORT_ORDER_CUSTOM     = 512, /* gets or'ed with - other selection */
};
typedef enum _SortOrder SortOrder;

struct _GeState {
  int64_t   pad_for_gobject[3];

  void      (*ui) (Mrg *mrg, void *state);
  Mrg        *mrg;
  char       *path;      /* path of edited file or open folder  */

  char       *src_path; /* path to (immutable) source image. */

  char       *composition_path; /* the exported .gegl file, or .png with embedded .gegl file,
                            the file that is written to on save. This differs depending
                            on type of input file.
                          */
  GList       *index;
  int          index_dirty;
  GList       *paths;  /* list of full paths to entries in collection/path/containing path,
                          XXX: could be replaced with URIs, and each
                          element should perhaps contain more internal info
                          like stars, tags etc.  */
  GeglBuffer  *buffer;
  GeglNode    *gegl;
  GeglNode    *source;  /* a file-loader or another swapped in buffer provider that is the
                             image data source for the loaded image.  */
  GeglNode    *save;    /* node rigged up for saving file XXX: might be bitrotted */

  GeglNode    *sink;    /* the sink which we're rendering content and graph from */
  GeglNode    *active;  /* the node being actively inspected  */

  int          pad_active; /* 0=input 1=aux 2=output (default)*/

  GThread     *renderer_thread; /* only used when GEGL_RENDERER=thread is set in environment */
  int          entry_no; /* used in collection-view, and set by "parent" */

  int          is_dir;  // is current in dir mode

  int          show_bindings;

  GeglNode    *reference_node;

  GeglNode    *processor_node; /* the node we have a processor for */
  GeglProcessor *processor;
  GeglBuffer  *processor_buffer;
  GeglBuffer  *cached_buffer;
  int          frame_cache;
  int          renderer_state;
  int          editing_op_name;
  char         editing_buf[1024];
  char         commandline[1024];
  int          rev;

  const char  *property_focus; // interned string of property name, or "operation" or "id"
  int          editing_property;
  int          show_preferences;


  float        u, v;
  float        scale;
  float        fps;

  int          is_fit;
  int          show_bounding_box;
  float        dir_scale;
  int          nearest_neighbor;
  float        render_quality; /* default (and in code swapped for preview_quality during preview rendering, this is the canonical read location for the value)  */
  float        preview_quality;
  SortOrder    sort_order;

  float        graph_pan_x;
  float        graph_pan_y;
  int          show_graph;
  float        graph_scale;

  float        thumbbar_pan_x;
  float        thumbbar_pan_y;
  int          show_thumbbar;
  float        thumbbar_scale;
  float        thumbbar_opacity;
  int          thumbbar_timeout;

  int          show_controls;
  int          controls_timeout;


  char       **ops; // the operations part of the commandline, if any
  float        slide_pause;
  int          slide_enabled;
  int          slide_timeout;
  char        *paint_color;    // XXX : should be a GeglColor

  GeglNode    *gegl_decode;
  GeglNode    *decode_load;
  GeglNode    *decode_store;
  int          playing;
  int          loop_current;
  double       pos;
  double       duration;
  double       start;
  double       end;
  int          color_managed_display;

  int          is_video;
  int          prev_frame_played;
  double       prev_ms;

  GHashTable  *ui_consumer;
};

typedef struct _GeState GeState;

float hypotf (float a, float b);
GeState *app_state(void);
int argvs_eval (const char *str);

char  *ui_suffix_path   (const char *path);
char  *ui_unsuffix_path (const char *path);

int    ui_hide_controls_cb (Mrg *mrg, void *data);
char  *ui_get_thumb_path   (const char *path);
void   ui_queue_thumb      (const char *path);
void   ui_load_path        (GeState *o);
void   ui_contrasty_stroke (cairo_t *cr);



void        meta_set_key (GeState    *state,
                          const char *path,
                          const char *key,
                          const char *value);

const char *meta_get_key (GeState    *state, const char *path, const char *key);

void        meta_set_key_int (GeState    *state, const char *path, const char *key, int value);
int         meta_get_key_int (GeState    *state, const char *path, const char *key); 
void        meta_set_key_float (GeState *state, const char *path, const char *key, float value);
float       meta_get_key_float (GeState *state, const char *path, const char *key);

int
meta_get_attribute_int (GeState    *state,
                        const char *path,
                        int         child_no,
                        // also have child name
                        const char *attribute);

float
meta_get_attribute_float (GeState    *state,
                          const char *path,
                          int         child_no,
                          // also have child name
                          const char *attribute);

void
meta_set_attribute_float (GeState    *state,
                          const char *path,
                          int         child_no,
                          // also have child name
                          const char *attribute,
                          float       detail);

void
meta_set_attribute_int (GeState    *state,
                        const char *path,
                        int         child_no,
                        // also have child name
                        const char *attribute,
                        int         detail);

/* --- the display order is overrides, then dirlist.. this
 *     should be configurable
 *
 * the display order should be a second list of
 */

void
meta_set_attribute (GeState    *state,
                    const char *path,
                    int         child_no,
                    const char *attribute,
                    const char *detail);
const char *
meta_get_attribute (GeState    *state,
                    const char *path,
                    int         child_no,
                    const char *attribute);
int
meta_has_attribute (GeState    *state,
                    const char *path,
                    int         child_no,
                    const char *attribute);
void
meta_unset_attribute (GeState    *state,
                      const char *path,
                      int         child_no,
                      const char *attribute);

/* for now - not supporting multivalue on attribute/details  */

char *
meta_get_child (GeState    *state,
                const char *path,
                int         child_no);

void
meta_insert_child (GeState    *state,
                   const char *path,
                   int         child_no,
                   const char *child_name);
int
meta_remove_child (GeState    *state,
                   const char *path,
                   int         child_no,
                   const char *child_name);

void
meta_replace_child (GeState    *state,
                    const char *path,
                    int         old_child_no,
                    const char *old_child_name,
                    const char *new_child_name);

void
meta_swap_children (GeState    *state,
                    const char *path,
                    int         child_no1,
                    const char *child_name1,
                    int         child_no2,
                    const char *child_name2);


int         ui_items_count         (GeState *o);


char *get_item_dir     (GeState *o);  /* current dir */
int   get_item_no      (GeState *o);  /* current no */
char *get_item_path    (GeState *o);  /* currently selected path */
char *get_item_path_no (GeState *o, int child_no);

void g_free (void *data);
void set_clip_position (GeState *o, double position);

]]

o = ffi.C.app_state()
mrg = o.mrg

ffi.metatype('GeState', {__index = {
   set_clip_position = function(...) return ffi.C.set_clip_position(...) end;
   get_key_int = function(...) return ffi.C.meta_get_key_int(...) end;
   items_count = function(...) return ffi.C.ui_items_count(...) end;
   item_no = function(...) return ffi.C.get_item_no(...) end;
   item_path = function(...) new = ffi.C.get_item_path(...); local ret = ffi.string(new) ; ffi.C.g_free(new); return ret; end;
   item_path_no = function(...) new = ffi.C.get_item_path_no(...); local ret = ffi.string(new) ; ffi.C.g_free(new); return ret; end;
 }})

function touch_point(x, y)
  cr:new_path()
  cr:arc(x, y, dim/2, 0, 3.1415 * 2)
  cr:set_source_rgba(1,0,0,0.5)
  cr:fill_preserve()
end

function contrasty_stroke()
  local x0 = 3.0
  local y0 = 3.0
  local x1 = 2.0
  local y1 = 2.0

  x0, y0 = cr:device_to_user_distance (x0, y0)
  x1, y1 = cr:device_to_user_distance (x1, y1)
  cr:set_source_rgba (0,0,0,0.5)
  cr:set_line_width (y0)
  cr:stroke_preserve ()
  cr:set_source_rgba (1,1,1,0.5)
  cr:set_line_width (y1)
  cr:stroke ()
end

function html(xhtml)
  mrg:xml_render(nil, function(event, link)
           ffi.C.argvs_eval(link)
  end,
  NULL, xhtml)
end



#ifndef __UI_H__
#define __UI_H__

#include <glib.h>
#include <glib-object.h>
#include <gegl.h>

#define GE_STATE_TYPE                \
    (ge_state_get_type())
#define GE_STATE(o)                  \
    (G_TYPE_CHECK_INSTANCE_CAST ((o), GE_STATE_TYPE, GeState))
#define GE_STATE_CLASS(c)            \
    (G_TYPE_CHECK_CLASS_CAST ((c), GE_STATE_TYPE, GeStateClass))
#define GE_IS_STATE(o)               \
    (G_TYPE_CHECK_INSTANCE_TYPE ((o), GE_STATE_TYPE))
#define GE_IS_STATE_CLASS(c)         \
    (G_TYPE_CHECK_CLASS_TYPE ((c),  GE_STATE_TYPE))
#define GE_STATE_GET_CLASS(o)        \
    (G_TYPE_INSTANCE_GET_CLASS ((o), GE_STATE_TYPE, GeStateClass))

typedef struct _GeState              GeState;
typedef struct _GeStatePrivate       GeStatePrivate;
typedef struct _GeStateClass         GeStateClass;


/*  this structure contains the full application state, and is what
 *  re-renderings of the UI is directly based on.
 *
 *
 */

typedef struct _IndexItem IndexItem;

#define INDEX_MAX_ATTRIBUTES  16

struct _IndexItem {
  char *name;
  char *attribute[INDEX_MAX_ATTRIBUTES];
  char *detail[INDEX_MAX_ATTRIBUTES];

  /* we could cache item meta data for reading here */
  //char *key[INDEX_MAX_ATTRIBUTES];
  //char *value[INDEX_MAX_ATTRIBUTES];
};

typedef enum _SortOrder SortOrder;

enum _SortOrder
{
  SORT_ORDER_AZ         = 1,
  SORT_ORDER_MTIME      = 2,
  SORT_ORDER_EXIF_TIME  = 4,
  SORT_ORDER_STARS      = 8,
  SORT_ORDER_CUSTOM     = 512, /* gets or'ed with - other selection */
};

struct _GeState {
  GObject   parent;
  void      (*ui) (Mrg *mrg, void *state);
  Mrg        *mrg;

  char       *path;      /* path of edited file or open folder  */



  char       *src_path; /* path to (immutable) source image. */

  char       *chain_path; /* .gegl/basename/chain - unless the src_path
                                   itself directl is a gegl comp  */
  GList      *index;  /* dictates manual order and attribute/details for entries */
  int         index_dirty; /* when non 0 we save upon leaving the folder */
  GList      *paths;  /* list of full paths to entries in collection/path/containing path,
                            XXX: could be replaced with URIs, and each
                            element should perhaps contain more internal info
                            like stars, tags etc.  */
  GeglBuffer    *buffer;
  GeglNode      *gegl;
  GeglNode      *source;  /* a file-loader or another swapped in buffer provider that is the
                             image data source for the loaded image.  */
  GeglNode      *save;    /* node rigged up for saving file XXX: might be bitrotted */

  GeglNode      *sink;    /* the sink which we're rendering content and graph from */
  GeglNode      *active;  /* the node being actively inspected  */

  int            pad_active; /* 0=input 1=aux 2=output (default)*/

  GThread       *renderer_thread; /* only used when GEGL_RENDERER=thread is set in environment */
  int            entry_no; /* used in collection-view, and set by "parent" */

  int            is_dir;  // is current in dir mode

  int            show_bindings;

  GeglNode      *reference_node;

  GeglNode      *processor_node; /* the node we have a processor for */
  GeglProcessor *processor;
  GeglBuffer    *processor_buffer;

  GeglBuffer    *cached_buffer;
  int            frame_cache; /* turns on a violent caching regime caching all completed renders */

  int            renderer_state;
  int            editing_op_name;
  char           editing_buf[1024];
  char           commandline[1024];
  int            rev;

  const char    *property_focus; // interned string of property name, or "operation" or "id"
  int            editing_property;
  int            show_preferences;

  float          u, v;
  float          scale;
  float          fps;

  int            is_fit;
  int            show_bounding_box;
  float          dir_scale;

  int            nearest_neighbor;

  float          render_quality; /* default (and in code swapped for preview_quality during preview rendering, this is the canonical read location for the value)  */
  float          preview_quality;
  SortOrder      sort_order;

  float          graph_pan_x;
  float          graph_pan_y;
  int            show_graph;
  float          graph_scale;

  float          thumbbar_pan_x;
  float          thumbbar_pan_y;
  int            show_thumbbar;
  float          thumbbar_scale;
  float          thumbbar_opacity;
  int            thumbbar_timeout;

  int            show_controls;
  int            controls_timeout;


  char         **ops; // the operations part of the commandline, if any
  float          slide_pause;
  int            slide_enabled;
  int            slide_timeout;
  char          *paint_color;    // XXX : should be a GeglColor

  GeglNode      *gegl_decode;
  GeglNode      *decode_load;
  GeglNode      *decode_store;
  int            playing;
  int            loop_current;
  double         pos;
  double         duration;
  double         start;
  double         end;
  int            color_managed_display;

  int            is_video;
  int            prev_frame_played;
  double         prev_ms;

  GHashTable    *ui_consumer;

  char          *loaded_path; /* used to persist the currently loaded path
                                 for use on triggers when leaving/replacing it  */

};

struct _GeStateClass {
  GObjectClass parent;
};

GType       ge_state_get_type      (void) G_GNUC_CONST;
GeState*    ge_state_new           (void);
const char *ge_state_get_path      (GeState *state, int no);
int         ge_state_get_n_paths   (GeState *state);
void        ui_load_path           (GeState *o);
void        ui_center_active_entry (GeState *o);


int         ui_items_count         (GeState *o);

void        meta_set_key (GeState    *state,
                          const char *path,
                          const char *key,
                          const char *value);

void        meta_unset_key (GeState    *state,
                            const char *path,
                            const char *key);

const char *meta_get_key (GeState    *state, const char *path, const char *key);

void        meta_set_key_int (GeState    *state, const char *path, const char *key, int value);
int         meta_get_key_int (GeState    *state, const char *path, const char *key);
void        meta_set_key_float (GeState *state, const char *path, const char *key, float value);
float       meta_get_key_float (GeState *state, const char *path, const char *key);

char      **meta_list_keys (GeState *state, const char *path);
char      **meta_list_attributes (GeState *state, const char *path, int item_no);

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

/* passing NULL for detail unsets the attribute
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


void   ui_viewer           (GeState *o);
void   ui_collection       (GeState *o);
char  *ui_suffix_path      (const char *path);
char  *ui_unsuffix_path    (const char *path);

int    ui_hide_controls_cb (Mrg *mrg, void *data);
gchar *ui_get_thumb_path   (const char *path);
void   ui_queue_thumb      (const char *path);

void   ui_contrasty_stroke (cairo_t *cr);

void ui_run_command (MrgEvent *event_or_null, void *commandline, void *ignored);


char *get_item_dir     (GeState *o);  /* current dir */
int   get_item_no      (GeState *o);  /* current no */
char *get_item_path    (GeState *o);  /* currently selected path */
char *get_item_path_no (GeState *o, int child_no);

extern GeState *global_state;

void populate_path_list (GeState *o);
void set_clip_position (GeState *o, double position);

#endif

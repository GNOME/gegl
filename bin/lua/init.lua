
ffi = require('ffi')
lgi = require 'lgi'

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

struct State {
  int64_t   pad_for_gobject[3];

  void      (*ui) (Mrg *mrg, void *state);
  Mrg        *mrg;
  char       *path;      /* path of edited file or open folder  */

  char       *src_path; /* path to (immutable) source image. */

  char       *save_path; /* the exported .gegl file, or .png with embedded .gegl file,
                            the file that is written to on save. This differs depending
                            on type of input file.
                          */
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
  int          renderer_state;
  int          editing_op_name;
  char         editing_buf[1024];
  char         commandline[1024];
  int          rev;

  const char  *property_focus; // interned string of property name, or "operation" or "id"
  int          editing_property;


  float        u, v;
  float        scale;

  int          is_fit;
  int          show_bounding_box;
  float        dir_scale;
  float        render_quality; /* default (and in code swapped for preview_quality during preview rendering, this is the canonical read location for the value)  */
  float        preview_quality;

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
  int          color_managed_display;

  int          is_video;
  int          prev_frame_played;
  double       prev_ms;

  GHashTable  *ui_consumer;
};

float hypotf (float a, float b);
struct State *app_state(void);
int argvs_eval (const char *str);
]]

o = ffi.C.app_state()
mrg = o.mrg


function touch_point(x, y)
  cr:new_path()
  cr:arc(x, y, dim/2, 0, 3.1415 * 2)
  cr:set_source_rgba(1,0,0,0.5)
  cr:fill_preserve()
end

function contrasty_stroke()
  local x0 = 3.0
  local y0 = 3.0;
  local x1 = 2.0
  local y1 = 2.0;

  x0, y0 = cr:device_to_user_distance (x0, y0)
  x1, y1 = cr:device_to_user_distance (x1, y1)
  cr:set_source_rgba (0,0,0,0.5);
  cr:set_line_width (y0);
  cr:stroke_preserve ()
  cr:set_source_rgba (1,1,1,0.5);
  cr:set_line_width (y1);
  cr:stroke ();
end

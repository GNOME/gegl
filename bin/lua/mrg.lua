-- ffi lua binding for microraptor gui by Øyvind Kolås, public domain
--
local ffi = require('ffi')
local cairo = require('cairo')
local C = ffi.load('mrg')
local M = setmetatable({C=C},{__index = C})

ffi.cdef[[

typedef struct _MrgList MrgList;

struct _MrgList {
  void *data;
  MrgList *next;
  void (*freefunc)(void *data, void *freefunc_data);
  void *freefunc_data;
}
;
void mrg_list_prepend_full (MrgList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data);

int mrg_list_length (MrgList *list);

void mrg_list_prepend (MrgList **list, void *data);

void mrg_list_append_full (MrgList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data);

void mrg_list_append (MrgList **list, void *data);

void mrg_list_remove (MrgList **list, void *data);
void mrg_list_free (MrgList **list);


MrgList *mrg_list_nth (MrgList *list, int no);

MrgList *mrg_list_find (MrgList *list, void *data);

void mrg_list_sort (MrgList **list, 
    int(*compare)(const void *a, const void *b, void *userdata),
    void *userdata);

void
mrg_list_insert_sorted (MrgList **list, void *data,
                       int(*compare)(const void *a, const void *b, void *userdata),
                       void *userdata);

typedef struct _Mrg Mrg;
typedef struct _MrgColor     MrgColor;
typedef struct _MrgStyle     MrgStyle;
typedef struct _MrgRectangle MrgRectangle;
typedef struct _MrgEvent     MrgEvent;

enum _MrgType {
  MRG_PRESS          = 1 << 0,
  MRG_MOTION         = 1 << 1,
  MRG_RELEASE        = 1 << 2,
  MRG_ENTER          = 1 << 3,
  MRG_LEAVE          = 1 << 4,
  MRG_TAP            = 1 << 5, /* NYI */
  MRG_TAP_AND_HOLD   = 1 << 6, /* NYI */
  MRG_DRAG_PRESS     = 1 << 7,
  MRG_DRAG_MOTION    = 1 << 8,
  MRG_DRAG_RELEASE   = 1 << 9,
  MRG_KEY_DOWN       = 1 << 10,
  MRG_KEY_UP         = 1 << 11,
  MRG_SCROLL         = 1 << 12,
  MRG_MESSAGE        = 1 << 13,
  MRG_DROP           = 1 << 14,

  MRG_TAPS     = (MRG_TAP | MRG_TAP_AND_HOLD),
  MRG_POINTER  = (MRG_PRESS | MRG_MOTION | MRG_RELEASE),
  MRG_CROSSING = (MRG_ENTER | MRG_LEAVE),
  MRG_DRAG     = (MRG_DRAG_PRESS | MRG_DRAG_MOTION | MRG_DRAG_RELEASE),
  MRG_KEY      = (MRG_KEY_DOWN | MRG_KEY_UP),


  MRG_COORD    = (MRG_POINTER | MRG_DRAG | MRG_CROSSING | MRG_TAPS),
  MRG_ANY      = (MRG_POINTER | MRG_DRAG | MRG_CROSSING | MRG_KEY | MRG_TAPS),
};


typedef enum _MrgType MrgType;

typedef void (*MrgCb) (MrgEvent *event,
                       void     *data,
                       void     *data2);

typedef void (*MrgLinkCb) (MrgEvent *event,
                           const char *href,
                           void     *data);

typedef void(*MrgDestroyNotify) (void     *data);
typedef int(*MrgTimeoutCb) (Mrg *mrg, void  *data);
typedef int(*MrgIdleCb)    (Mrg *mrg, void  *data);

typedef void (*MrgNewText) (const char *new_text, void *data);
typedef void (*UiRenderFun)(Mrg *mrg, void *ui_data);


void  mrg_destroy       (Mrg *mrg);

void mrg_clear_bindings (Mrg *mrg);
void  mrg_set_size      (Mrg *mrg, int width, int height);
void  mrg_set_position  (Mrg *mrg, int x, int y);
void  mrg_get_position  (Mrg *mrg, int *x, int *y);

void  mrg_message           (Mrg *mrg, const char *message);
void  mrg_set_title         (Mrg *mrg, const char *title);
void  mrg_window_set_value  (Mrg *mrg, const char *name, const char *value);
const char *mrg_get_title   (Mrg *mrg);
void *mrg_mmm               (Mrg *mrg);

int   mrg_width         (Mrg *mrg);
int   mrg_height        (Mrg *mrg);

Mrg *mrg_new(int width, int height, const char *backend);

void  mrg_set_ui        (Mrg *mrg, UiRenderFun, void *ui_data);

void  mrg_main          (Mrg *mrg);
void  mrg_quit          (Mrg *mrg);
cairo_t *mrg_cr         (Mrg *mrg);
void mrg_queue_draw     (Mrg *mrg, MrgRectangle *rectangle);

int  mrg_is_terminal    (Mrg *mrg);
void mrg_set_fullscreen (Mrg *mrg, int fullscreen);
int  mrg_is_fullscreen  (Mrg *mrg);

float mrg_ddpx          (Mrg *mrg);

unsigned char *mrg_get_pixels (Mrg *mrg, int *rowstride);

void mrg_restarter_add_path (Mrg *mrg, const char *path);
/*********************************************/

Mrg *mrg_new_mrg (Mrg *parent, int width, int height); // NYI

typedef struct _MrgImage MrgImage;

/* force a ui render, this is included for use with the headless backend.
 */
void  mrg_ui_update (Mrg *mrg);

MrgImage *mrg_query_image (Mrg *mrg, const char *path,
                           int *width,
                           int *height);

void mrg_forget_image (Mrg *mrg, const char *path);

/* built in http / local file URI fetcher, this is the callback interface
 * that needs to be implemented for mrg_xml_render if external resources (css
 * files / png images) are to be retrieved and rendered.
 */
int mrg_get_contents (Mrg         *mrg,
                      const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length);

int
mrg_get_contents_default (const char  *referer,
                          const char  *input_uri,
                          char       **contents,
                          long        *length,
                          void        *ignored_user_data);

void mrg_set_mrg_get_contents (Mrg *mrg,
  int (*mrg_get_contents) (const char  *referer,
                      const char  *input_uri,
                      char       **contents,
                      long        *length,
                      void        *get_contents_data),
  void *get_contents_data);


void mrg_render_to_mrg (Mrg *mrg, Mrg *mrg2, float x, float y);

int mrg_add_idle (Mrg *mrg, int (*idle_cb)(Mrg *mrg, void *idle_data), void *   idle_data);
int mrg_add_timeout (Mrg *mrg, int ms, int (*idle_cb)(Mrg *mrg, void *          idle_data), void *idle_data);
void mrg_remove_idle (Mrg *mrg, int handle);

int mrg_add_idle_full (Mrg *mrg, int (*idle_cb)(Mrg *mrg, void *idle_data),     void *idle_data,
                       void (*destroy_notify)(void *destroy_data),    void *destroy_data);

int mrg_add_timeout_full (Mrg *mrg, int ms, MrgTimeoutCb idle_cb, void *idle_data,
                       void (*destroy_notify)(void *destroy_data),    void *destroy_data);


void  mrg_set_line_spacing (Mrg *mrg, float line_spacing);
float mrg_line_spacing     (Mrg *mrg);

int mrg_print (Mrg *mrg, const char *string);

int   mrg_print_get_xy     (Mrg *mrg, const char *str, int no, float *x, float *y);


int mrg_print_xml (Mrg *mrg, const char *string);

void mrg_xml_render (Mrg *mrg, const char *uri_base,
                     MrgLinkCb linkcb,
                     void *link_data,
                     void   (*finalize)(void *listen_data, void *listen_data2,  void *finalize_data),
                     void    *finalize_data,
                     const char *html);

void mrg_set_target_fps (Mrg *mrg, float fps);
float mrg_get_target_fps (Mrg *mrg);

void mrg_listen      (Mrg     *mrg,
                      MrgType  types,
                      MrgCb    cb,
                      void    *data1,
                      void    *data2);

void mrg_listen_full (Mrg     *mrg,
                      MrgType  types,
                      MrgCb    cb,
                      void    *data1,
                      void    *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,  void *finalize_data),
                      void    *finalize_data);

uint32_t mrg_ms (Mrg *mrg);

int mrg_pointer_press    (Mrg *mrg, float x, float y, int device_no, uint32_t time);
int mrg_pointer_release  (Mrg *mrg, float x, float y, int device_no, uint32_t time);
int mrg_pointer_motion   (Mrg *mrg, float x, float y, int device_no, uint32_t time);
int mrg_key_press        (Mrg *mrg, unsigned int keyval, const char *string, uint32_t time);

int mrg_is_printing      (Mrg *mrg);
void mrg_new_page (Mrg *mrg);
void mrg_render_pdf (Mrg *mrg, const char *pdf_path);
void mrg_render_svg (Mrg *mrg, const char *svg_path);

/* these deal with pointer_no 0 only
 */
void  mrg_warp_pointer (Mrg *mrg, float x, float y);
float mrg_pointer_x    (Mrg *mrg);
float mrg_pointer_y    (Mrg *mrg);

float mrg_em (Mrg *mrg);
void  mrg_set_xy (Mrg *mrg, float x, float y);
float  mrg_x (Mrg *mrg);
float  mrg_y (Mrg *mrg);

typedef enum _MrgModifierState MrgModifierState;
  
enum _MrgModifierState
{   
  MRG_MODIFIER_STATE_SHIFT   = (1<<0), 
  MRG_MODIFIER_STATE_CONTROL = (1<<1),
  MRG_MODIFIER_STATE_ALT     = (1<<2),
  MRG_MODIFIER_STATE_BUTTON1 = (1<<3),
  MRG_MODIFIER_STATE_BUTTON2 = (1<<4),
  MRG_MODIFIER_STATE_BUTTON3 = (1<<5)
};

typedef enum _MrgScrollDirection MrgScrollDirection;
enum _MrgScrollDirection
{
  MRG_SCROLL_DIRECTION_UP,
  MRG_SCROLL_DIRECTION_DOWN,
  MRG_SCROLL_DIRECTION_LEFT,
  MRG_SCROLL_DIRECTION_RIGHT
};

struct _MrgEvent {
  MrgType  type;
  Mrg     *mrg;
  uint32_t time;

  MrgModifierState state;

  int      device_no; /* 0 = left mouse button / virtual focus */
                      /* 1 = middle mouse button */
                      /* 2 = right mouse button */
                      /* 3 = first multi-touch .. (NYI) */

  float   device_x; /* untransformed (device) coordinates  */
  float   device_y;

  /* coordinates; and deltas for motion/drag events in user-coordinates: */
  float   x;
  float   y;
  float   start_x; /* start-coordinates (press) event for drag, */
  float   start_y; /*    untransformed coordinates */
  float   prev_x;  /* previous events coordinates */
  float   prev_y;
  float   delta_x; /* x - prev_x, redundant - but often useful */
  float   delta_y; /* y - prev_y, redundant - ..  */

  MrgScrollDirection scroll_direction;

  /* only valid for key-events */
  unsigned int unicode;
  const char *string;   /* can be "up" "down" "space" "backspace" "a" "b" "ø" etc .. */
                        /* this is also where the message is delivered for
                         * MESSAGE events
                         */
//  int stop_propagate; /* */
};

void mrg_event_stop_propagate (MrgEvent *event);

void  mrg_text_listen (Mrg *mrg, MrgType types,
                       MrgCb cb, void *data1, void *data2);

void  mrg_text_listen_full (Mrg *mrg, MrgType types,
                            MrgCb cb, void *data1, void *data2,
          void (*finalize)(void *listen_data, void *listen_data2, void *       finalize_data),
          void  *finalize_data);

void  mrg_text_listen_done (Mrg *mrg);

struct _MrgColor {
  float red;
  float green;
  float blue;
  float alpha;
};

typedef enum {
  MRG_DISPLAY_INLINE = 0,
  MRG_DISPLAY_BLOCK,
  MRG_DISPLAY_LIST_ITEM,
  MRG_DISPLAY_HIDDEN
} MrgDisplay;

/* matches cairo order */
typedef enum
{
  MRG_FONT_WEIGHT_NORMAL = 0,
  MRG_FONT_WEIGHT_BOLD
} MrgFontWeight;

typedef enum
{
  MRG_FILL_RULE_NONZERO = 0,
  MRG_FILL_RULE_EVEN_ODD
} MrgFillRule;

/* matches cairo order */
typedef enum
{
  MRG_FONT_STYLE_NORMAL = 0,
  MRG_FONT_STYLE_ITALIC,
  MRG_FONT_STYLE_OBLIQUE
} MrgFontStyle;

typedef enum
{
  MRG_BOX_SIZING_CONTENT_BOX = 0,
  MRG_BOX_SIZING_BORDER_BOX
} MrgBoxSizing;

/* matching nchanterm definitions */

typedef enum {
  MRG_REGULAR     = 0,
  MRG_BOLD        = (1 << 0),
  MRG_DIM         = (1 << 1),
  MRG_UNDERLINE   = (1 << 2),
  MRG_REVERSE     = (1 << 3),
  MRG_OVERLINE    = (1 << 4),
  MRG_LINETHROUGH = (1 << 5),
  MRG_BLINK       = (1 << 6)
} MrgTextDecoration;

typedef enum {
  MRG_POSITION_STATIC = 0,
  MRG_POSITION_RELATIVE,
  MRG_POSITION_FIXED,
  MRG_POSITION_ABSOLUTE
} MrgPosition;

typedef enum {
  MRG_OVERFLOW_VISIBLE = 0,
  MRG_OVERFLOW_HIDDEN,
  MRG_OVERFLOW_SCROLL,
  MRG_OVERFLOW_AUTO
} MrgOverflow;

typedef enum {
  MRG_FLOAT_NONE = 0,
  MRG_FLOAT_LEFT,
  MRG_FLOAT_RIGHT,
  MRG_FLOAT_FIXED
} MrgFloat;

typedef enum {
  MRG_CLEAR_NONE = 0,
  MRG_CLEAR_LEFT,
  MRG_CLEAR_RIGHT,
  MRG_CLEAR_BOTH
} MrgClear;

typedef enum {
  MRG_TEXT_ALIGN_LEFT = 0,
  MRG_TEXT_ALIGN_RIGHT,
  MRG_TEXT_ALIGN_JUSTIFY,
  MRG_TEXT_ALIGN_CENTER
} MrgTextAlign;

typedef enum {
  MRG_WHITE_SPACE_NORMAL = 0,
  MRG_WHITE_SPACE_NOWRAP,
  MRG_WHITE_SPACE_PRE,
  MRG_WHITE_SPACE_PRE_LINE,
  MRG_WHITE_SPACE_PRE_WRAP
} MrgWhiteSpace;

typedef enum {
  MRG_VERTICAL_ALIGN_BASELINE = 0,
  MRG_VERTICAL_ALIGN_MIDDLE,
  MRG_VERTICAL_ALIGN_BOTTOM,
  MRG_VERTICAL_ALIGN_TOP,
  MRG_VERTICAL_ALIGN_SUB,
  MRG_VERTICAL_ALIGN_SUPER
} MrgVerticalAlign;

typedef enum {
  MRG_CURSOR_AUTO = 0,
  MRG_CURSOR_ALIAS,
  MRG_CURSOR_ALL_SCROLL,
  MRG_CURSOR_CELL,
  MRG_CURSOR_CONTEXT_MENU,
  MRG_CURSOR_COL_RESIZE,
  MRG_CURSOR_COPY,
  MRG_CURSOR_CROSSHAIR,
  MRG_CURSOR_DEFAULT,
  MRG_CURSOR_E_RESIZE,
  MRG_CURSOR_EW_RESIZE,
  MRG_CURSOR_HELP,
  MRG_CURSOR_MOVE,
  MRG_CURSOR_N_RESIZE,
  MRG_CURSOR_NE_RESIZE,
  MRG_CURSOR_NESW_RESIZE,
  MRG_CURSOR_NS_RESIZE,
  MRG_CURSOR_NW_RESIZE,
  MRG_CURSOR_NO_DROP,
  MRG_CURSOR_NONE,
  MRG_CURSOR_NOT_ALLOWED,
  MRG_CURSOR_POINTER,
  MRG_CURSOR_PROGRESS,
  MRG_CURSOR_ROW_RESIZE,
  MRG_CURSOR_S_RESIZE,
  MRG_CURSOR_SE_RESIZE,
  MRG_CURSOR_SW_RESIZE,
  MRG_CURSOR_TEXT,
  MRG_CURSOR_VERTICAL_TEXT,
  MRG_CURSOR_W_RESIZE,
  MRG_CURSOR_WAIT,
  MRG_CURSOR_ZOOM_IN,
  MRG_CURSOR_ZOOM_OUT
} MrgCursor;

typedef enum {
  MRG_LINE_CAP_BUTT,
  MRG_LINE_CAP_ROUND,
  MRG_LINE_CAP_SQUARE
} MrgLineCap;

typedef enum {
  MRG_LINE_JOIN_MITER,
  MRG_LINE_JOIN_ROUND,
  MRG_LINE_JOIN_BEVEL
} MrgLineJoin;

typedef enum {
  MRG_UNICODE_BIDI_NORMAL = 0,
  MRG_UNICODE_BIDI_EMBED,
  MRG_UNICODE_BIDI_BIDI_OVERRIDE
} MrgUnicodeBidi;

typedef enum {
  MRG_DIRECTION_LTR = 0,
  MRG_DIRECTION_RTL
} MrgDirection;

typedef enum {
  MRG_VISIBILITY_VISIBLE = 0,
  MRG_VISIBILITY_HIDDEN
} MrgVisibility;

typedef enum {
  MRG_LIST_STYLE_OUTSIDE = 0,
  MRG_LIST_STYLE_INSIDE
} MrgListStyle;

struct _MrgStyle {
  /* text-related */
  float             font_size;
  char              font_family[128];
  MrgColor          color;
  float             text_indent;
  float             letter_spacing;
  float             word_spacing;

  MrgVisibility     visibility;

  MrgTextDecoration text_decoration;
  float             line_height;
  float             line_width;

  MrgColor          background_color;

  float             stroke_width;
  float             text_stroke_width;
  MrgColor          text_stroke_color;
  float             tab_size;

  /* copying style structs takes quite a bit of the time in the system,
   * packing bits ends up saving a lot
   *
   * XXX: this is a gcc extension...
   */
  MrgFillRule         fill_rule;
  MrgFontStyle        font_style;
  MrgFontWeight       font_weight;
  MrgLineCap          stroke_linecap;
  MrgLineJoin         stroke_linejoin;
  MrgTextAlign        text_align;
  MrgFloat            float_;
  MrgClear            clear;
  MrgOverflow         overflow;
  MrgDisplay          display;
  MrgPosition         position;
  MrgBoxSizing        box_sizing;
  MrgVerticalAlign    vertical_align;
  MrgWhiteSpace       white_space;
  MrgUnicodeBidi      unicode_bidi;
  MrgDirection        direction;
  MrgListStyle        list_style;
  unsigned char       stroke;
  unsigned char       fill;
  unsigned char       width_auto;
  unsigned char       margin_left_auto;
  unsigned char       margin_right_auto;
  unsigned char       print_symbols;
  MrgColor            stroke_color;


  /* vector shape / box related */
  MrgColor          fill_color;

  MrgColor          border_top_color;
  MrgColor          border_left_color;
  MrgColor          border_right_color;
  MrgColor          border_bottom_color;

  MrgCursor         cursor;

  /* layout related */

  float             top;
  float             left;
  float             right;
  float             bottom;
  float             width;
  float             height;
  float             min_height;
  float             max_height;

  float             border_top_width;
  float             border_left_width;
  float             border_right_width;
  float             border_bottom_width;

  float             padding_top;
  float             padding_left;
  float             padding_right;
  float             padding_bottom;

  float             margin_top;
  float             margin_left;
  float             margin_right;
  float             margin_bottom;

  void             *id_ptr;
};

/* XXX: need full version for lua ffi */
void mrg_add_binding (Mrg *mrg,
                      const char *key,
                      const char *action,
                      const char *label,
                      MrgCb cb,
                      void  *cb_data);


void mrg_add_binding_full (Mrg *mrg,
                           const char *key,
                           const char *action,
                           const char *label,
                           MrgCb cb,
                           void  *cb_data,
                           MrgDestroyNotify destroy_notify,
                           void  *destroy_data);
/**
 * mrg_style:
 * @mrg the mrg-context
 *
 * Returns the currently 
 *
 */
MrgStyle *mrg_style (Mrg *mrg);

void mrg_start     (Mrg *mrg, const char *class_name, void *id_ptr);
void mrg_start_with_style (Mrg        *mrg,
                           const char *style_id,
                           void       *id_ptr,
                           const char *style);
void mrg_end       (Mrg *mrg);


void mrg_stylesheet_clear (Mrg *mrg);
void mrg_stylesheet_add (Mrg *mrg, const char *css, const char *uri,
                         int priority,
                         char **error);

void mrg_css_set (Mrg *mrg, const char *css);
void mrg_css_add (Mrg *mrg, const char *css);

void mrg_set_style (Mrg *mrg, const char *style);


void  mrg_set_line_height (Mrg *mrg, float line_height);
void  mrg_set_font_size (Mrg *mrg, float font_size);
float  mrg_line_height (Mrg *mrg);

/* XXX: doesnt feel like it belongs here */
void mrg_image (Mrg *mrg, float x0, float y0, float width, float height, float opacity, const char *path, int *used_width, int *used_height);
void mrg_image_memory (Mrg *mrg, float x0, float y0, float width, float height, float opacity, const char *data, int length, const char *eid, int *used_width, int *used_height);

MrgImage *mrg_query_image_memory (Mrg *mrg,
                                  const char *contents,
                                  int         length,
                                  const char *eid,
                                  int        *width,
                                  int        *height);


int mrg_get_cursor_pos  (Mrg *mrg);
void mrg_set_cursor_pos (Mrg *mrg, int pos);

void mrg_edit_start (Mrg *mrg,
                     MrgNewText update_string,
                     void *user_data);

void mrg_edit_start_full (Mrg *mrg,
                           MrgNewText update_string,
                           void *user_data,
                           MrgDestroyNotify destroy,
                           void *destroy_data);
void  mrg_edit_end (Mrg *mrg);

void  mrg_set_edge_left   (Mrg *mrg, float edge);
void  mrg_set_edge_top    (Mrg *mrg, float edge);
void  mrg_set_edge_right  (Mrg *mrg, float edge);
void  mrg_set_edge_bottom (Mrg *mrg, float edge);
float mrg_edge_left       (Mrg *mrg);
float mrg_edge_top        (Mrg *mrg);
float mrg_edge_right      (Mrg *mrg);
float mrg_edge_bottom     (Mrg *mrg);

typedef struct _MrgHost   MrgHost;
typedef struct _MrgClient MrgClient;

void       mrg_host_add_client_mrg   (MrgHost     *host,
                                      Mrg         *mrg,
                                      float        x,
                                      float        y);
MrgHost   *mrg_host_new              (Mrg *mrg, const char *path);
void       mrg_host_destroy          (MrgHost *host);
void       mrg_host_set_focused      (MrgHost *host, MrgClient *client);
MrgClient *mrg_host_get_focused      (MrgHost *host);
MrgClient *mrg_host_monitor_dir      (MrgHost *host);
void       mrg_host_register_events  (MrgHost *host);
MrgList   *mrg_host_clients          (MrgHost *host);

void       mrg_host_set_default_size (MrgHost *host, int width, int height);
void       mrg_host_get_default_size (MrgHost *host, int *width, int *height);

void       mrg_client_render_sloppy  (MrgClient *client, float x, float y);
int        mrg_client_get_pid        (MrgClient *client);
void       mrg_client_kill           (MrgClient *client);
void       mrg_client_raise_top      (MrgClient *client);
void       mrg_client_render         (MrgClient *client, Mrg *mrg, float x, float y);
void       mrg_client_maximize       (MrgClient *client);
float      mrg_client_get_x          (MrgClient *client);
float      mrg_client_get_y          (MrgClient *client);
void       mrg_client_set_x          (MrgClient *client, float x);
void       mrg_client_set_y          (MrgClient *client, float y);
void       mrg_client_get_size       (MrgClient *client, int *width, int *height);
void       mrg_client_set_size       (MrgClient *client, int width,  int height);
const char *mrg_client_get_title     (MrgClient *client);
const char *mrg_client_get_value     (MrgClient *client, const char *name);
void        mrg_client_set_value     (MrgClient *client, const char *name, const char *value);

void        mrg_client_send_message  (MrgClient *client, const char *message);

const char *mrg_client_get_message   (MrgClient *client);
int         mrg_client_has_message   (MrgClient *client);

void mrg_client_set_stack_order (MrgClient *client, int zpos);
int  mrg_client_get_stack_order (MrgClient *client);
void        host_add_client_mrg      (MrgHost     *host,
                                      Mrg         *mrg,
                                      float        x,
                                      float        y);

typedef struct MrgBinding {
  char *nick;
  char *command;
  char *label;
  MrgCb cb;
  void *cb_data;
  MrgDestroyNotify destroy_notify;
  void  *destroy_data;
} MrgBinding;

MrgBinding *mrg_get_bindings (Mrg *mrg);

int mrg_parse_svg_path (Mrg *mrg, const char *str);




typedef struct _MrgVT MrgVT;

MrgVT      *mrg_vt_new                  (Mrg *mrg, const char *commandline);
const char *mrg_vt_find_shell_command   (void);
void        mrg_vt_poll                 (MrgVT *vt);
long        mrg_vt_rev                  (MrgVT *vt);
void        mrg_vt_destroy              (MrgVT *vt);
void        mrg_vt_set_term_size        (MrgVT *vt, int cols, int rows);
void        mrg_vt_feed_keystring       (MrgVT *vt, const char *str);
void        mrg_vt_feed_byte            (MrgVT *vt, int byte);
const char *mrg_vt_get_commandline      (MrgVT *vt);
void        mrg_vt_set_scrollback_lines (MrgVT *vt, int scrollback_lines);
int         mrg_vt_get_scrollback_lines (MrgVT *vt);
int         mrg_vt_get_line_count       (MrgVT *vt);
const char *mrg_vt_get_line             (MrgVT *vt, int no);
int         mrg_vt_get_cols             (MrgVT *vt);
int         mrg_vt_get_rows             (MrgVT *vt);
int         mrg_vt_get_cursor_x         (MrgVT *vt);
int         mrg_vt_get_cursor_y         (MrgVT *vt);
int         mrg_vt_is_done              (MrgVT *vt);
int         mrg_vt_get_result           (MrgVT *vt);

void        mrg_vt_draw             (MrgVT *vt, Mrg *mrg, double x, double y, float font_size, float line_spacing);
]]

function M.new(width,height, backend)
  return ffi.gc(
           C.mrg_new(width, height, backend),
           C.mrg_destroy)
end

ffi.metatype('Mrg', {__index = {
  -- maybe we should add a _full version to this as well, then all callback
  -- things in the code would be lifecycle managed.
  set_ui           = function (mrg, uifun, uidata) C.mrg_set_ui(mrg, uifun, uidata) end,
  style            = function (...) return C.mrg_style (...) end;
  width            = function (...) return C.mrg_width (...) end,
  mmm              = function (...) return C.mrg_mmm (...) end,
  height           = function (...) return C.mrg_height (...) end,
  pointer_x        = function (...) return C.mrg_pointer_x (...) end,
  pointer_y        = function (...) return C.mrg_pointer_y (...) end,
  text_listen_done = function (...) C.mrg_text_listen_done(...) end,
  warp_pointer     = function (...) C.mrg_warp_pointer(...) end,
  quit             = function (...) C.mrg_quit(...) end,
  image_memory = function (mrg, x, y, w, h, opacity, data, len, eid)
    local rw = ffi.new'int[1]'
    local rh = ffi.new'int[1]'
    C.mrg_image_memory(mrg, x, y, w, h, opacity, data, len, eid, rw, rh)
    return rw[0], rh[0]
  end,
  image = function (mrg, x, y, w, h, opacity, path)
    local rw = ffi.new'int[1]'
    local rh = ffi.new'int[1]'
    C.mrg_image (mrg, x, y, w, h, opacity, path, rw, rh)
    return rw[0], rh[0]
  end,
  image            = function (...) C.mrg_image(...) end,
  forget_image     = function (...) C.mrg_forget_image(...) end,
  render_pdf       = function (...) C.mrg_render_pdf(...) end,
  render_svg       = function (...) C.mrg_render_svg(...) end,
  is_printing      = function (...) return C.mrg_is_printing (...) end,
  new_page         = function (...) C.mrg_new_page(...) end,
  message          = function (...) C.mrg_message(...) end,
  parse_svg_path   = function (...) return C.mrg_parse_svg_path(...) end,
  clear_bindings = function (...) C.mrg_clear_bindings(...) end,


  stylesheet_add = function(...) C.mrg_stylesheet_add(...) end,
  stylesheet_clear = function(...) C.mrg_stylesheet_clear(...) end,


  image_size       = function (mrg, path)
    local rw = ffi.new'int[1]'
    local rh = ffi.new'int[1]'
    rw[0] = -1
    rh[0] = -1
    C.mrg_query_image (mrg, path, rw, rh)
    return rw[0], rh[0]
  end,


  image_size_memory       = function (mrg, data, len, eid)
    local rw = ffi.new'int[1]'
    local rh = ffi.new'int[1]'
    rw[0] = -1
    rh[0] = -1
    C.mrg_query_image_memory (mrg, data,len,eid, rw, rh)
    return rw[0], rh[0]
  end,


  css_set          = function (...) C.mrg_css_set(...) end,
  set_style        = function (...) C.mrg_set_style(...) end,
  css_add          = function (...) C.mrg_css_add(...) end,
  set_em           = function (...) C.mrg_set_em(...) end,

  edit_start = function (mrg, cb, data)
    -- manually cast and destroy resources held by lua/C binding
    local notify_fun, cb_fun;
    local notify_cb = function (finalize_data)
      cb_fun:free();
      notify_fun:free();
    end
    notify_fun = ffi.cast ("MrgDestroyNotify", notify_cb)
    local wrap_cb = function(new_text,data)
      cb(ffi.string(new_text),data)
    end
    cb_fun = ffi.cast ("MrgNewText", wrap_cb)
    return C.mrg_edit_start_full (mrg, cb_fun, data, notify_fun, NULL)
  end,

  edit_end = function (...) C.mrg_edit_end (...) end,
  set_edge_top     = function (...) C.mrg_set_edge_top(...) end,
  edge_top         = function (...) return C.mrg_get_edge_top(...) end,
  set_edge_bottom     = function (...) C.mrg_set_edge_bottom(...) end,
  edge_bottom         = function (...) return C.mrg_get_edge_bottom(...) end,
  set_edge_left     = function (...) C.mrg_set_edge_left(...) end,
  edge_left         = function (...) return C.mrg_get_edge_left(...) end,
  set_edge_right     = function (...) C.mrg_set_edge_right(...) end,
  edge_right         = function (...) return C.mrg_get_edge_right(...) end,
  set_rem          = function (...) C.mrg_set_rem(...) end,
  main             = function (...) C.mrg_main(...) end,
  get_bindings     = function (...) return C.mrg_get_bindings(...) end,
  start            = function (mrg, cssid) C.mrg_start(mrg, cssid, NULL) end,
  start_with_style = function (mrg, cssid, style) C.mrg_start_with_style(mrg, cssid, NULL, style) end,
  restarter_add_path = function (...) C.mrg_restarter_add_path(...) end,
  close            = function (...) C.mrg_end(...) end,
  queue_draw       = function (...) C.mrg_queue_draw(...) end,
  ms               = function (...) return C.mrg_ms(...) end,
  add_binding = function(mrg,key,action,label,cb,db_data)
    local notify_fun, cb_fun;
    local notify_cb = function (data1, data2,finalize_data)
      cb_fun:free();
      notify_fun:free();
    end
    notify_fun = ffi.cast ("MrgDestroyNotify", notify_cb)
    cb_fun = ffi.cast ("MrgCb", cb)
    return C.mrg_add_binding_full (mrg, key, action,label,cb_fun, cb_data, notify_fun, NULL)
  end,

  text_listen      = function (mrg, types, cb, data1, data2)
    -- manually cast and destroy resources held by lua/C binding
    local notify_fun, cb_fun;
    local notify_cb = function (data1, data2,finalize_data)
      cb_fun:free();
      notify_fun:free();
    end

    notify_fun = ffi.cast ("MrgCb", notify_cb)
    cb_fun = ffi.cast ("MrgCb", cb)
    return C.mrg_text_listen_full (mrg, types, cb_fun, data1, data2, notify_fun, NULL)
  end,
  listen           = function (mrg, types, cb, data1, data2)
    local notify_fun, cb_fun;
    local notify_cb = function (data1, data2, finalize_data)
      cb_fun:free();
      notify_fun:free();
    end
    notify_fun = ffi.cast ("MrgCb", notify_cb)
    cb_fun = ffi.cast ("MrgCb", cb)
    return C.mrg_listen_full (mrg, types, cb_fun, data1, data2, notify_fun, NULL)
  end,
  print_get_xy     = function (mrg, str, no) 
                         local rx = ffi.new'float[1]'
                         local ry = ffi.new'float[1]'
                         C.mrg_print_get_xy(mrg, str, no, rx, ry)
                         return rx[0], ry[0]
                       end,
  get_cursor_pos   = function (...) return C.mrg_get_cursor_pos(...) end,
  set_cursor_pos   = function (...) C.mrg_set_cursor_pos(...) end,
  print            = function (...) C.mrg_print(...) end,
  print_xml        = function (...) C.mrg_print_xml(...) end,

  xml_render       = function (mrg, uri_base, link_cb, data, html)
   local notify_fun, cb_fun;
   local notify_cb = function (data, html, finalize_data)
     cb_fun:free();
     notify_fun:free();
   end
   local wrap_cb = function(event, href, data)
      link_cb(event, ffi.string(href), data)
   end
   notify_fun = ffi.cast("MrgCb", notify_cb)
   cb_fun = ffi.cast ("MrgLinkCb", wrap_cb)
   return C.mrg_xml_render (mrg, uri_base, cb_fun, data, notify_fun, NULL, html)
   end,
  cr               = function (...) return C.mrg_cr (...) end,
  width            = function (...) return C.mrg_width (...) end,
  height           = function (...) return C.mrg_height (...) end,
  set_fullscreen   = function (...) C.mrg_set_fullscreen(...) end,
  is_fullscreen    = function (...) return C.mrg_is_fullscreen(...) ~= 0 end,
  x                = function (...) return C.mrg_x (...) end,
  y                = function (...) return C.mrg_y (...) end,
  xy               = function (mrg) return mrg:x(), mrg:y() end,
  em               = function (...) return C.mrg_em (...) end,
  rem              = function (...) return C.mrg_rem (...) end,
  set_xy           = function (...) C.mrg_set_xy (...) end,
  set_size         = function (...) C.mrg_set_size (...) end,
  set_position     = function (...) C.mrg_set_position(...) end,
  set_title        = function (...) C.mrg_set_title (...) end,
  window_set_value = function (...) C.mrg_window_set_value (...) end,
  get_title        = function (...) return C.mrg_get_title (...) end,
  set_font_size    = function (...) C.mrg_set_font_size (...) end,
  set_target_fps   = function (...) C.mrg_set_target_fps (...) end,
  target_fps       = function (...) return C.mrg_get_target_fps (...) end,

  pointer_press = function (...) C.mrg_pointer_press (...) end,
  pointer_release = function (...) C.mrg_pointer_release (...) end,
  pointer_motion = function (...) C.mrg_pointer_motion (...) end,
  key_press      = function (...) C.mrg_key_press (...) end,

  add_idle = function (mrg, ms, cb, data1)
    local notify_fun, cb_fun;
    local notify_cb = function (a,b)
      cb_fun:free();
      notify_fun:free();
    end
    notify_fun = ffi.cast ("MrgDestroyNotify", notify_cb)
    cb_fun = ffi.cast ("MrgIdleCb", cb)
    return C.mrg_add_idle_full (mrg, cb_fun, data1, notify_fun, NULL)
    -- XXX: wrap the callback so that the return isn't mandatory?
  end,

  add_timeout = function (mrg, ms, cb, data1)
    local notify_fun, cb_fun;
    local notify_cb = function (a,b)
      cb_fun:free();
      notify_fun:free();
    end
    notify_fun = ffi.cast ("MrgDestroyNotify", notify_cb)
    cb_fun = ffi.cast ("MrgTimeoutCb", cb)
    return C.mrg_add_timeout_full (mrg, ms, cb_fun, data1, notify_fun, NULL)
  end,

  host_new = function (...) 
             return C.mrg_host_new(...)
  end,
  vt_new = function (...) 
             return C.mrg_vt_new (...)
  end,

  remove_idle      = function (...) return C.mrg_remove_idle (...) end,
  edge_left        = function (...) return C.mrg_edge_left (...) end,
  set_edge_left    = function (...) C.mrg_set_edge_left (...) end,
  edge_right       = function (...) return C.mrg_edge_right (...) end,
  set_edge_right   = function (...) C.mrg_set_edge_right (...) end,
  edge_top         = function (...) return C.mrg_edge_top (...) end,
  set_edge_top     = function (...) C.mrg_set_edge_top (...) end,
  edge_bottom      = function (...) return C.mrg_edge_bottom (...) end,
  set_edge_bottom  = function (...) C.mrg_set_edge_bottom (...) end,
}})
ffi.metatype('MrgEvent',     {__index = { 

  stop_propagate = function (...) return C.mrg_event_stop_propagate (...) end,
  message = function (event) return ffi.string(event.string) end,

}})

ffi.metatype('MrgVT', {__index = {
  poll = function (...) C.mrg_vt_poll (...) end,
  draw = function (...) C.mrg_vt_draw (...) end,
  rev  = function (...) return C.mrg_vt_rev (...) end,
  destroy = function (vt) C.mrg_vt_destroy (vt); ffi.gc (vt, nil) end,
  set_term_size = function (...) C.mrg_vt_set_term_size (...) end,
  feed_keystring = function (...) C.mrg_vt_feed_keystring (...) end,
  feed_byte = function (...) C.mrg_vt_feed_byte (...) end,
  is_done = function (...) return C.mrg_vt_is_done (...) end,
  get_result = function (...) return C.mrg_vt_get_result (...) end,
  get_scrollback_lines = function (...) return C.mrg_vt_get_scrollback_lines (...) end,
  set_scrollback_lines = function (...) C.mrg_vt_set_scrollback_lines (...) end,
  get_commandline = function (...) return C.mrg_vt_get_commandline (...) end,
  get_line_count = function (...) return C.mrg_vt_get_line_count (...) end,
  get_line = function (...) return C.mrg_vt_get_line (...) end,
  get_cols = function (...) return C.mrg_vt_get_cols (...) end,
  get_rows = function (...) return C.mrg_vt_get_rows (...) end,
  get_cursor_x = function (...) return C.mrg_vt_get_cursor_x (...) end,
  get_cursor_y = function (...) return C.mrg_vt_get_cursor_y (...) end,

}})


ffi.metatype('MrgColor',     {__index = { }})
ffi.metatype('MrgStyle',     {__index = { }})
ffi.metatype('MrgRectangle', {__index = { }})
ffi.metatype('MrgList',      {__index = { }})
ffi.metatype('MrgHost',      {__index = {
  set_default_size = function (...) C.mrg_host_set_default_size (...) end,
  -- get_default_size not bound until needed
  set_focused     = function (...) C.mrg_host_set_focused (...) end,
  focused         = function (...) return C.mrg_host_get_focused (...) end,
  monitor_dir     = function (...) return C.mrg_host_monitor_dir (...) end,
  register_events = function (...) C.mrg_host_register_events (...) end,
  destroy         = function (...) C.mrg_host_destroy (...) end,
  clients         = function (...) 
    local ret = {}
    local iter = C.mrg_host_clients (...)
    while iter ~= NULL do
      local client 
      ret[#ret+1] = ffi.cast("MrgClient*", iter.data)
      iter = iter.next
    end
    return ret
  end,

  add_client_mrg  = function (...) C.mrg_host_add_client_mrg (...) end

}})
ffi.metatype('MrgClient',    {__index = {
  render_sloppy = function (...) C.mrg_client_render_sloppy (...) end,
  render        = function (...) C.mrg_client_render (...) end,
  pid           = function (...) return C.mrg_client_get_pid (...) end,
  kill          = function (...) C.mrg_client_kill (...) end,
  raise_top     = function (...) C.mrg_client_raise_top (...) end,
  set_stack_order = function (...) C.mrg_client_set_stack_order (...) end,
  stack_order   = function (...) return C.mrg_client_get_stack_order (...) end,
  send_message  = function (...) C.mrg_client_send_message (...) end,
  set_value     = function (...) C.mrg_client_set_value (...) end,
  get_value     = function (...) 
    local f = C.mrg_client_get_value (...)
    if f ~= NULL then
      return ffi.string(f)
    end
    return nil
  end,
  has_message   = function (...) return C.mrg_client_has_message (...) end,
  get_message   = function (...)
    local f = C.mrg_client_get_message (...)
    if f ~= NULL then
      return ffi.string(f)
    end
    return nil 
  end,
  maximize      = function (...) C.mrg_client_maximize (...) end,
  title         = function (...) return C.mrg_client_get_title (...) end,
  x             = function (...) return C.mrg_client_get_x (...) end,
  y             = function (...) return C.mrg_client_get_y (...) end,
  set_x         = function (...) C.mrg_client_set_x (...) end,
  set_y         = function (...) C.mrg_client_set_y (...) end,
  set_xy        = function (client, x, y) client:set_x(x) client:set_y(y) end,
  xy            = function (client) return client:x(), client:y() end,
  set_size      = function (...) C.mrg_client_set_size (...) end,
  size          = function (client)
    local rw = ffi.new'int[1]'
    local rh = ffi.new'int[1]'
    C.mrg_client_get_size (client, rw, rh)
    return rw[0], rh[0]
  end,
 }})

  M.MOTION = C.MRG_MOTION;
  M.ENTER = C.MRG_ENTER;
  M.LEAVE = C.MRG_LEAVE;
  M.PRESS = C.MRG_PRESS;
  M.KEY_DOWN = C.MRG_KEY_DOWN;
  M.KEY_UP = C.MRG_KEY_UP;
  M.RELEASE = C.MRG_RELEASE;
  M.TAP = C.MRG_TAP;
  M.TAP_AND_HOLD = C.MRG_TAP_AND_HOLD;
  M.DRAG_MOTION = C.MRG_DRAG_MOTION;
  M.DRAG_PRESS = C.MRG_DRAG_PRESS;
  M.DRAG_RELEASE = C.MRG_DRAG_RELEASE;
  M.SCROLL = C.MRG_SCROLL;
  M.MESSAGE= C.MRG_MESSAGE;
  M.DRAG = C.MRG_DRAG;
  M.ANY = C.MRG_ANY;
  M.KEY = C.MRG_KEY;
  M.COORD = C.MRG_COORD;


  M.SCROLL_DIRECTION_UP = C.MRG_SCROLL_DIRECTION_UP
  M.SCROLL_DIRECTION_DOWN = C.MRG_SCROLL_DIRECTION_DOWN

keyboard_visible = false
--local keyboard_visible = true


local quite_complete={

  {x=5,   y=26, w=44, label='esc'},
  {x=55,   y=26, label='F1'},
  {x=95,   y=26, label='F2'},
  {x=135,   y=26, label='F3'},
  {x=175,   y=26, label='F4'},
  {x=215,   y=26, label='F5'},
  {x=255,   y=26, label='F6'},
  {x=295,   y=26, label='F7'},
  {x=335,   y=26, label='F8'},
  {x=375,   y=26, label='F9'},
  {x=415,   y=26, label='F10'},
  {x=455,   y=26, label='F11'},
  {x=495,   y=26, label='F12'},


  {x=5,   y=65, label='`', shifted='~'},
  {x=45,   y=65, label='1', shifted='!'},
  {x=85,  y=65, label='2', shifted='@'},
  {x=125,  y=65, label='3', shifted='#'},
  {x=165,  y=65, label='4', shifted='$'},
  {x=205,  y=65, label='5', shifted='%'},
  {x=245,  y=65, label='6', shifted='^'},
  {x=285,  y=65, label='7', shifted='&'},
  {x=325,  y=65, label='8', shifted='*'},
  {x=365,  y=65, label='9', shifted='('},
  {x=405,  y=65, label='0', shifted=')'},
  {x=445,  y=65, label='-', shifted='_'},
  {x=485,  y=65, label='=', shifted='+'},
  {x=525,  w=40, y=65, label='⌫', code='backspace'},

  --[[{x=355,  w=40, y=225, label='ins', code='insert'},
  {x=400,  w=40, y=225, label='del', code='delete'},]]

  {x=60,   y=105, label='q', shifted='Q'},
  {x=100,  y=105, label='w', shifted='W'},
  {x=140,  y=105, label='e', shifted='E'},
  {x=180,  y=105, label='r', shifted='R'},
  {x=220,  y=105, label='t', shifted='T'},
  {x=260,  y=105, label='y', shifted='Y'},
  {x=300,  y=105, label='u', shifted='U'},
  {x=340,  y=105, label='i', shifted='I'},
  {x=380,  y=105, label='o', shifted='O'},
  {x=420,  y=105, label='p', shifted='P'},
  {x=460,  y=105, label='[', shifted='{'},
  {x=500,  y=105, label=']', shifted='}'},

  {x=20,   y=105, label='↹', code='tab'},
  {x=70,   y=145, label='a', shifted='A'},
  {x=110,  y=145, label='s', shifted='S'},
  {x=150,  y=145, label='d', shifted='D'},
  {x=190,  y=145, label='f', shifted='F'},
  {x=230,  y=145, label='g', shifted='G'},
  {x=270,  y=145, label='h', shifted='H'},
  {x=310,  y=145, label='j', shifted='J'},
  {x=350,  y=145, label='k', shifted='K'},
  {x=390,  y=145, label='l', shifted='L'},
  {x=430,  y=145, label=';', shifted=':'},
  {x=470,  y=145, label='\'', shifted='"'},
  {x=510, w=45, y=145, label='⏎', code='return' },

  {x=20,   w=45, y=145, label='⇧', type='modal', isshift='true'},  -- ⇧ XXX:  code relies on label
  {x=40,  y=185, label='\\', shifted='|'},
  {x=80,   y=185, label='z', shifted='Z'},
  {x=120,  y=185, label='x', shifted='X'},
  {x=160,  y=185, label='c', shifted='C'},
  {x=200,  y=185, label='v', shifted='V'},
  {x=240,  y=185, label='b', shifted='B'},
  {x=280,  y=185, label='n', shifted='N'},
  {x=320,  y=185, label='m', shifted='M'},
  {x=360,  y=185, label=',', shifted='<'},
  {x=400,  y=185, label='.', shifted='>'},
  {x=440,  y=185, label='/', shifted='?'},

  {x=20, w=50, y=225, label='ctrl', type='modal'},
  {x=75, w=40,  y=225, label='alt', type='modal'},
  {x=120,  y=225, w=230, label=' ', code='space'},

  {x=440,  y=225, label='←', code='left'},
  {x=480,  y=225, label='↓', code='down'},
  {x=480,  y=185, label='↑', code='up'},
  {x=520,  y=225, label='→', code='right'},

  {x=355,  y=225, w=45, label='⌨', type='keyboard'},
}

local minimal={


  --[[{x=355,  w=40, y=225, label='ins', code='insert'},
  {x=400,  w=40, y=225, label='del', code='delete'},]]

  {x=60,   y=105, label='q', shifted='Q', fn_label='1'},
  {x=100,  y=105, label='w', shifted='W', fn_label='2'},
  {x=140,  y=105, label='e', shifted='E', fn_label='3'},
  {x=180,  y=105, label='r', shifted='R', fn_label='4'},
  {x=220,  y=105, label='t', shifted='T', fn_label='5'},
  {x=260,  y=105, label='y', shifted='Y', fn_label='6'},
  {x=300,  y=105, label='u', shifted='U', fn_label='7'},
  {x=340,  y=105, label='i', shifted='I', fn_label='8'},
  {x=380,  y=105, label='o', shifted='O', fn_label='9'},
  {x=420,  y=105, label='p', shifted='P', fn_label='0'},

  {x=460,  y=105, w=28, label='[', shifted='{', fn_label='-'},
  {x=490,  y=105, w=28, label=']', shifted='}', fn_label='+'},

  {x=525,  w=40, y=105, label='⌫', code='backspace', fn_label='del', fn_code='delete'},

  {x=20,   y=105, label='esc', code='escape', fn_label='↹', fn_code='tab'},
  {x=70,   y=145, label='a', shifted='A', fn_label='!'},
  {x=110,  y=145, label='s', shifted='S', fn_label='@'},
  {x=150,  y=145, label='d', shifted='D', fn_label='#'},
  {x=190,  y=145, label='f', shifted='F', fn_label='$'},
  {x=230,  y=145, label='g', shifted='G', fn_label='%'},
  {x=270,  y=145, label='h', shifted='H', fn_label='^'},
  {x=310,  y=145, label='j', shifted='J', fn_label='&'},
  {x=350,  y=145, label='k', shifted='K', fn_label='*'},
  {x=390,  y=145, label='l', shifted='L', fn_label='('},
  {x=430,  y=145, label=';', shifted=':', fn_label=')'},
  {x=470,  y=145, label='\'', shifted='"', fn_label='_'},
  {x=510, w=45, y=145, label='⏎', code='return' },

  {x=20,   w=45, y=145, label='⇧', type='modal', isshift='true'},  --  XXX:  code relies on label
  {x=40,  y=185, label='\\', shifted='|', fn_label='='},
  {x=80,   y=185, label='z', shifted='Z', fn_label='Ø'},
  {x=120,  y=185, label='x', shifted='X', fn_label='å'},
  {x=160,  y=185, label='c', shifted='C', fn_label='ø'},
  {x=200,  y=185, label='v', shifted='V', fn_label='Å'},
  {x=240,  y=185, label='b', shifted='B', fn_label='æ'},
  {x=280,  y=185, label='n', shifted='N', fn_label='Æ'},
  {x=320,  y=185, label='m', shifted='M', fn_label='…'},
  {x=360,  y=185, label=',', shifted='<', fn_label='“'},
  {x=400,  y=185, label='.', shifted='>', fn_label='”'},
  {x=480,  y=185, label='/', shifted='?', fn_label='€'},

  {x=20,  y=225, w=30, label='Fn', shifted='Fx', type='modal'},
  {x=55, w=50, y=225, label='ctrl', type='modal'},
  {x=110, w=40,  y=225, label='alt', type='modal'},
  {x=155,  y=225, w=230, label=' ', code='space'},

  {x=400,  y=225, label='←', code='left', fn_label='pup', fn_code='page-up'},
  {x=440,  y=225, label='↓', code='down', fn_label='end', fn_code='end'},
  {x=440,  y=185, label='↑', code='up', fn_label='home', fn_code='home'},
  {x=480 , y=225, label='→', code='right', fn_label='pdn', fn_code='page-down'},


  {x=520,   y=185, label='`', shifted='~', fn_label='ins', fn_code='insert'},

  {x=520, w=45, y=225, label='⌨', type='keyboard'},
}

local keys, y_start

if false then
  keys= quite_complete
  y_start=42
else
  keys= minimal
  y_start=82
end

local pan_y = 0
local pan_x = 0
local shifted = false
local alted = false
local ctrld = false
local fnd = false
local px = 0
local py = 0

M.draw_keyboard = function (mrg)
  local em = 20
  local cr = mrg:cr()
  local fg = {r=0,g=0,b=0}
  local bg = {r=1,g=1,b=1}


  local dim = mrg:width() / 555

  cr:set_font_size(em * 0.8)

  cr:new_path()
  if not keyboard_visible then
    cr:rectangle (mrg:width() - 4 * em, mrg:height() - 3 * em, 4 * em, 3 * em)
    mrg:listen(M.TAP+M.RELEASE, function(event)
      keyboard_visible = true
      mrg:queue_draw(nil)
    end)
    cr:set_source_rgba(bg.r,bg.g,bg.b,0.2)
    cr:fill()
    mrg:set_style('background:transparent; color: rgba(0,0,0,0.5); font-size: 1.9em')
    mrg:set_xy(mrg:width() - 3 * em, mrg:height() - 0.5 * em)
    mrg:print('⌨')
  else
    cr:translate(0,mrg:height()-250* dim)
    cr:scale(dim, dim)
    cr:translate(pan_x, pan_y)

    cr:rectangle(0,y_start,620,166)
    cr:set_source_rgba(bg.r,bg.g,bg.b,0.6)
    mrg:listen(M.COORD, function(e) 
        px = e.x;
        py = e.y;
        mrg:queue_draw(nil)
       e:stop_propagate() end)
    cr:fill()

    for k,v in ipairs(keys) do
      cr:new_path()
      if not v.w then v.w = 34 end
      if v.w then
        cr:rectangle(v.x - em * 0.8, v.y - em, v.w, em * 1.8)
      else
        cr:arc (v.x, v.y, em, 0, 3.1415*2)
      end
      mrg:listen(M.TAP+M.RELEASE, function(event)
        px = event.x;
        py = event.y;
        if v.type == 'keyboard' then
          keyboard_visible = false
        elseif (not v.isshift) and (v.label ~= 'Fn') and (v.label ~= 'ctrl') then
          local keystr = v.label
          if shifted and v.shifted then keystr = v.shifted end
          if fnd and v.fn_label then keystr = v.fn_label end

          if v.code            then keystr = v.code end
          if fnd and v.fn_code then keystr = v.fn_code end
          if alted             then keystr = 'alt-' .. keystr end
          if ctrld             then keystr = 'control-' .. keystr end

          mrg:key_press(0, keystr, 0)
        end
        mrg:queue_draw(nil)
        event:stop_propagate()
      end)
      if v.active or cr:in_fill(px,py) then
        cr:set_source_rgba (bg.r,bg.g,bg.b,0.8)
      else
        cr:set_source_rgba (fg.r,fg.g,fg.b,0.8)
      end
      cr:set_line_width(1)
      cr:stroke_preserve()
      if v.type == 'modal' then

        mrg:listen(M.TAP+M.RELEASE, function(event)
          if v.isshift then
            shifted = not shifted
            v.active = shifted
            mrg:queue_draw(nil)
          end
          if v.label == 'ctrl' then
            ctrld = not ctrld
            v.active = ctrld
            mrg:queue_draw(nil)
          end
          if v.label == 'alt' then
            alted = not alted
            v.active = alted
            mrg:queue_draw(nil)
          end
          if v.label == 'Fn' then
            fnd = not fnd
            v.active = fnd
            mrg:queue_draw(nil)
          end
        end)
        if (v.active and false) then
          cr:set_source_rgba (fg.r,fg.g,fg.b,0.8)
          cr:set_line_width(4)
          cr:stroke_preserve()
        end

      end
      if v.active or cr:in_fill(px,py) then
        cr:set_source_rgba (fg.r,fg.g,fg.b,0.5)
      else
        cr:set_source_rgba (bg.r,bg.g,bg.b,0.5)
      end
      cr:fill ()
      cr:move_to (v.x - 6, v.y + 4)
      if v.active then
        cr:set_source_rgba (bg.r,bg.g,bg.b,1.0)
      else
        cr:set_source_rgba (fg.r,fg.g,fg.b,1.0)
      end

      if fnd then
        if v.fn_label then
            cr:show_text (v.fn_label)
        else
            cr:show_text (v.label)
        end
      else
        if shifted then
          if v.shifted then
            cr:show_text (v.shifted)
          else
            cr:show_text (v.label)
          end
        else
          cr:show_text (v.label)
        end
      end
    end

    cr:rectangle(340,205,30,35)
    mrg:listen(M.DRAG, function(event)
      if pan_y + event.delta_y < 0  and event.device_y > 180 then
        pan_y = pan_y + event.delta_y
      end
      mrg:queue_draw(nil)
      event.stop_propagate()
    end)
    cr:new_path()
  end
end

return M

/* 
 * ctx.h is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ctx.h is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ctx; if not, see <https://www.gnu.org/licenses/>.
 *
 * 2012, 2015, 2019, 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * ctx is a single header 2d vector graphics processing framework.
 *
 * To use ctx in a project, do the following:
 *
 * #define CTX_IMPLEMENTATION
 * #include "ctx.h"
 *
 * Ctx contains a minimal default fallback font with only ascii, so
 * you probably want to also include a font, and perhaps enable
 * the cairo or SDL2 optional renderers, a more complete example
 * could be:
 *
 * #include <cairo.h>
 * #include <SDL.h>
 * #include "ctx-font-regular.h"
 * #define CTX_IMPLEMENTATION
 * #include "ctx.h"
 *
 * The behavior of ctx can be tweaked, and features can be configured, enabled
 * or disabled with other #defines, see further down in the start of this file
 * for details.
 */

#ifndef CTX_H
#define CTX_H

#ifdef __cplusplus
extern "C" {
#endif

#if !__COSMOPOLITAN__
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#endif

typedef struct _Ctx            Ctx;

/* The pixel formats supported as render targets
 */
enum _CtxPixelFormat
{
  CTX_FORMAT_NONE=0,
  CTX_FORMAT_GRAY8,  // 1  - these enum values are not coincidence
  CTX_FORMAT_GRAYA8, // 2  -
  CTX_FORMAT_RGB8,   // 3  -
  CTX_FORMAT_RGBA8,  // 4  -
  CTX_FORMAT_BGRA8,  // 5
  CTX_FORMAT_RGB565, // 6
  CTX_FORMAT_RGB565_BYTESWAPPED, // 7
  CTX_FORMAT_RGB332, // 8
  CTX_FORMAT_RGBAF,  // 9
  CTX_FORMAT_GRAYF,  // 10
  CTX_FORMAT_GRAYAF, // 11
  CTX_FORMAT_GRAY1,  //12 MONO
  CTX_FORMAT_GRAY2,  //13 DUO
  CTX_FORMAT_GRAY4,  //14
  CTX_FORMAT_CMYK8,  //15
  CTX_FORMAT_CMYKA8, //16 
  CTX_FORMAT_CMYKAF, //17
};
typedef enum   _CtxPixelFormat CtxPixelFormat;

typedef struct _CtxGlyph       CtxGlyph;

/**
 * ctx_new:
 *
 * Create a new drawing context, this context has no pixels but
 * accumulates commands and can be played back on other ctx
 * render contexts.
 */
Ctx *ctx_new (void);

/**
 * ctx_new_for_framebuffer:
 *
 * Create a new drawing context for a framebuffer, rendering happens
 * immediately.
 */
Ctx *ctx_new_for_framebuffer (void *data,
                              int   width,
                              int   height,
                              int   stride,
                              CtxPixelFormat pixel_format);
/**
 * ctx_new_ui:
 *
 * Create a new interactive ctx context, might depend on additional
 * integration.
 */
Ctx *ctx_new_ui (int width, int height);

/**
 * ctx_new_for_drawlist:
 *
 * Create a new drawing context for a pre-existing drawlist.
 */
Ctx *ctx_new_for_drawlist (void *data, size_t length);


/**
 * ctx_dirty_rect:
 *
 * Query the dirtied bounding box of drawing commands thus far.
 */
void  ctx_dirty_rect      (Ctx *ctx, int *x, int *y, int *width, int *height);

/**
 * ctx_free:
 * @ctx: a ctx context
 */
void ctx_free (Ctx *ctx);

/* clears and resets a context */
void ctx_reset          (Ctx *ctx);
void ctx_begin_path     (Ctx *ctx);
void ctx_save           (Ctx *ctx);
void ctx_restore        (Ctx *ctx);
void ctx_start_group    (Ctx *ctx);
void ctx_end_group      (Ctx *ctx);
void ctx_clip           (Ctx *ctx);
void ctx_identity       (Ctx *ctx);
void ctx_rotate         (Ctx *ctx, float x);

void ctx_image_smoothing     (Ctx *ctx, int enabled);
int  ctx_get_image_smoothing (Ctx *ctx);

#define CTX_LINE_WIDTH_HAIRLINE -1000.0
#define CTX_LINE_WIDTH_ALIASED  -1.0
#define CTX_LINE_WIDTH_FAST     -1.0  /* aliased 1px wide line */
void ctx_miter_limit (Ctx *ctx, float limit);
float ctx_get_miter_limit (Ctx *ctx);
void ctx_line_width       (Ctx *ctx, float x);
void ctx_line_dash_offset (Ctx *ctx, float line_dash);
float ctx_get_line_dash_offset (Ctx *ctx);
void ctx_apply_transform  (Ctx *ctx, float a,  float b,  // hscale, hskew
                                     float c,  float d,  // vskew,  vscale
                                     float e,  float f); // htran,  vtran
void ctx_set_transform    (Ctx *ctx, float a, float b, float c, float d, float e, float f);
void  ctx_line_dash       (Ctx *ctx, float *dashes, int count);
void  ctx_font_size       (Ctx *ctx, float x);
void  ctx_font            (Ctx *ctx, const char *font);
void  ctx_font_family     (Ctx *ctx, const char *font_family);
void  ctx_scale           (Ctx *ctx, float x, float y);
void  ctx_translate       (Ctx *ctx, float x, float y);
void  ctx_line_to         (Ctx *ctx, float x, float y);
void  ctx_move_to         (Ctx *ctx, float x, float y);
void  ctx_curve_to        (Ctx *ctx, float cx0, float cy0,
                           float cx1, float cy1,
                           float x, float y);
void  ctx_quad_to         (Ctx *ctx, float cx, float cy,
                           float x, float y);
void  ctx_arc             (Ctx  *ctx,
                           float x, float y,
                           float radius,
                           float angle1, float angle2,
                           int   direction);
void  ctx_arc_to          (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);
void  ctx_rel_arc_to      (Ctx *ctx, float x1, float y1,
                           float x2, float y2, float radius);
void  ctx_rectangle       (Ctx *ctx,
                           float x0, float y0,
                           float w, float h);
void  ctx_round_rectangle (Ctx *ctx,
                           float x0, float y0,
                           float w, float h,
                           float radius);
void  ctx_rel_line_to     (Ctx *ctx,
                           float x, float y);
void  ctx_rel_move_to     (Ctx *ctx,
                           float x, float y);
void  ctx_rel_curve_to    (Ctx *ctx,
                           float x0, float y0,
                           float x1, float y1,
                           float x2, float y2);
void  ctx_rel_quad_to     (Ctx *ctx,
                           float cx, float cy,
                           float x, float y);
void  ctx_close_path      (Ctx *ctx);
float ctx_get_font_size   (Ctx *ctx);
const char *ctx_get_font  (Ctx *ctx);
float ctx_get_line_width  (Ctx *ctx);
int   ctx_width           (Ctx *ctx);
int   ctx_height          (Ctx *ctx);
int   ctx_rev             (Ctx *ctx);
float ctx_x               (Ctx *ctx);
float ctx_y               (Ctx *ctx);
void  ctx_current_point   (Ctx *ctx, float *x, float *y);
void  ctx_get_transform   (Ctx *ctx, float *a, float *b,
                           float *c, float *d,
                           float *e, float *f);

CtxGlyph *ctx_glyph_allocate (int n_glyphs);

void gtx_glyph_free       (CtxGlyph *glyphs);

int  ctx_glyph            (Ctx *ctx, uint32_t unichar, int stroke);

void ctx_preserve         (Ctx *ctx);
void ctx_fill             (Ctx *ctx);
void ctx_stroke           (Ctx *ctx);

void ctx_parse            (Ctx *ctx, const char *string);

void ctx_shadow_rgba      (Ctx *ctx, float r, float g, float b, float a);
void ctx_shadow_blur      (Ctx *ctx, float x);
void ctx_shadow_offset_x  (Ctx *ctx, float x);
void ctx_shadow_offset_y  (Ctx *ctx, float y);
void ctx_view_box         (Ctx *ctx,
                           float x0, float y0,
                           float w, float h);
void
ctx_set_pixel_u8          (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void  ctx_global_alpha     (Ctx *ctx, float global_alpha);
float ctx_get_global_alpha (Ctx *ctx);

void ctx_named_source (Ctx *ctx, const char *name);
// followed by a color, gradient or pattern definition

void ctx_stroke_source  (Ctx *ctx); // next source definition is for stroking

void ctx_rgba_stroke   (Ctx *ctx, float r, float g, float b, float a);
void ctx_rgb_stroke    (Ctx *ctx, float r, float g, float b);
void ctx_rgba8_stroke  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_gray_stroke   (Ctx *ctx, float gray);
void ctx_drgba_stroke  (Ctx *ctx, float r, float g, float b, float a);
void ctx_cmyka_stroke  (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_cmyk_stroke   (Ctx *ctx, float c, float m, float y, float k);
void ctx_dcmyka_stroke (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_dcmyk_stroke  (Ctx *ctx, float c, float m, float y, float k);



void ctx_rgba   (Ctx *ctx, float r, float g, float b, float a);
void ctx_rgb    (Ctx *ctx, float r, float g, float b);
void ctx_rgba8  (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void ctx_gray   (Ctx *ctx, float gray);
void ctx_drgba  (Ctx *ctx, float r, float g, float b, float a);
void ctx_cmyka  (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_cmyk   (Ctx *ctx, float c, float m, float y, float k);
void ctx_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a);
void ctx_dcmyk  (Ctx *ctx, float c, float m, float y, float k);

/* there is also getters for colors, by first setting a color in one format and getting
 * it with another color conversions can be done
 */

void ctx_get_rgba   (Ctx *ctx, float *rgba);
void ctx_get_graya  (Ctx *ctx, float *ya);
void ctx_get_drgba  (Ctx *ctx, float *drgba);
void ctx_get_cmyka  (Ctx *ctx, float *cmyka);
void ctx_get_dcmyka (Ctx *ctx, float *dcmyka);
int  ctx_in_fill    (Ctx *ctx, float x, float y);
int  ctx_in_stroke  (Ctx *ctx, float x, float y);

void ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1);
void ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0,
                          float x1, float y1, float r1);
/* XXX should be ctx_gradient_add_stop_rgba */
void ctx_gradient_add_stop (Ctx *ctx, float pos, float r, float g, float b, float a);

void ctx_gradient_add_stop_u8 (Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a);


/*
 *
 */
void ctx_define_texture (Ctx *ctx,
                         const char *eid,
                         int         width,
                         int         height,
                         int         stride,
                         int         format,
                         void       *data,
                         char       *ret_eid);

void
ctx_get_image_data (Ctx *ctx, int sx, int sy, int sw, int sh,
                    CtxPixelFormat format, int dst_stride,
                    uint8_t *dst_data);

void
ctx_put_image_data (Ctx *ctx, int w, int h, int stride, int format,
                    uint8_t *data,
                    int ox, int oy,
                    int dirtyX, int dirtyY,
                    int dirtyWidth, int dirtyHeight);


/* loads an image file from disk into texture, returning pixel width, height
 * and eid, the eid is based on the path; not the contents - avoiding doing
 * sha1 checksum of contents. The width and height of the image is returned
 * along with the used eid, width height or eid can be NULL if we
 * do not care about their values.
 */
void ctx_texture_load (Ctx        *ctx,
                       const char *path,
                       int        *width,
                       int        *height,
                       char       *eid);

/* sets the paint source to be a texture by eid
 */
void ctx_texture              (Ctx *ctx, const char *eid, float x, float y);

void ctx_draw_texture         (Ctx *ctx, const char *eid, float x, float y, float w, float h);

void ctx_draw_texture_clipped (Ctx *ctx, const char *eid, float x, float y, float w, float h, float sx, float sy, float swidth, float sheight);

void ctx_draw_image           (Ctx *ctx, const char *path, float x, float y, float w, float h);

void ctx_draw_image_clipped   (Ctx *ctx, const char *path, float x, float y, float w, float h, float sx, float sy, float swidth, float sheight);

/* used by the render threads of fb and sdl backends.
 */
void ctx_set_texture_source (Ctx *ctx, Ctx *texture_source);
/* used when sharing cache state of eids between clients
 */
void ctx_set_texture_cache (Ctx *ctx, Ctx *texture_cache);

typedef struct _CtxDrawlist CtxDrawlist;
typedef void (*CtxFullCb) (CtxDrawlist *drawlist, void *data);

int ctx_pixel_format_bits_per_pixel (CtxPixelFormat format); // bits per pixel
int ctx_pixel_format_get_stride (CtxPixelFormat format, int width);
int ctx_pixel_format_components (CtxPixelFormat format);

void _ctx_set_store_clear (Ctx *ctx);
void _ctx_set_transformation (Ctx *ctx, int transformation);

Ctx *ctx_hasher_new (int width, int height, int cols, int rows);
uint8_t *ctx_hasher_get_hash (Ctx *ctx, int col, int row);

int ctx_utf8_strlen (const char *s);

#ifdef _BABL_H
#define CTX_BABL 1
#else
#define CTX_BABL 0
#endif

/* If cairo.h is included before ctx.h add cairo integration code
 */
#ifdef CAIRO_H
#define CTX_CAIRO 1
#else
#define CTX_CAIRO 0
#endif

#ifdef SDL_h_
#define CTX_SDL 1
#else
#define CTX_SDL 0
#endif

#ifndef CTX_FB
#if CTX_SDL
#define CTX_FB 1
#else
#define CTX_FB 0
#endif
#endif

#if CTX_SDL
#define ctx_mutex_t            SDL_mutex
#define ctx_create_mutex()     SDL_CreateMutex()
#define ctx_lock_mutex(a)      SDL_LockMutex(a)
#define ctx_unlock_mutex(a)    SDL_UnlockMutex(a)
#else
#define ctx_mutex_t           int
#define ctx_create_mutex()    NULL
#define ctx_lock_mutex(a)   
#define ctx_unlock_mutex(a)  
#endif

#if CTX_CAIRO

/* render the deferred commands of a ctx context to a cairo
 * context
 */
void  ctx_render_cairo  (Ctx *ctx, cairo_t *cr);

/* create a ctx context that directly renders to the specified
 * cairo context
 */
Ctx * ctx_new_for_cairo (cairo_t *cr);
#endif

/* free with free() */
char *ctx_render_string (Ctx *ctx, int longform, int *retlen);

void ctx_render_stream  (Ctx *ctx, FILE *stream, int formatter);

void ctx_render_ctx     (Ctx *ctx, Ctx *d_ctx);

void ctx_start_move     (Ctx *ctx);


int ctx_add_single      (Ctx *ctx, void *entry);

uint32_t ctx_utf8_to_unichar (const char *input);
int      ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);


typedef enum
{
  CTX_FILL_RULE_EVEN_ODD,
  CTX_FILL_RULE_WINDING
} CtxFillRule;

typedef enum
{
  CTX_COMPOSITE_SOURCE_OVER,
  CTX_COMPOSITE_COPY,
  CTX_COMPOSITE_SOURCE_IN,
  CTX_COMPOSITE_SOURCE_OUT,
  CTX_COMPOSITE_SOURCE_ATOP,
  CTX_COMPOSITE_CLEAR,

  CTX_COMPOSITE_DESTINATION_OVER,
  CTX_COMPOSITE_DESTINATION,
  CTX_COMPOSITE_DESTINATION_IN,
  CTX_COMPOSITE_DESTINATION_OUT,
  CTX_COMPOSITE_DESTINATION_ATOP,
  CTX_COMPOSITE_XOR,
} CtxCompositingMode;

typedef enum
{
  CTX_BLEND_NORMAL,
  CTX_BLEND_MULTIPLY,
  CTX_BLEND_SCREEN,
  CTX_BLEND_OVERLAY,
  CTX_BLEND_DARKEN,
  CTX_BLEND_LIGHTEN,
  CTX_BLEND_COLOR_DODGE,
  CTX_BLEND_COLOR_BURN,
  CTX_BLEND_HARD_LIGHT,
  CTX_BLEND_SOFT_LIGHT,
  CTX_BLEND_DIFFERENCE,
  CTX_BLEND_EXCLUSION,
  CTX_BLEND_HUE, 
  CTX_BLEND_SATURATION, 
  CTX_BLEND_COLOR, 
  CTX_BLEND_LUMINOSITY,  // 15
  CTX_BLEND_DIVIDE,
  CTX_BLEND_ADDITION,
  CTX_BLEND_SUBTRACT,    // 18
} CtxBlend;

void ctx_blend_mode (Ctx *ctx, CtxBlend mode);

typedef enum
{
  CTX_JOIN_BEVEL = 0,
  CTX_JOIN_ROUND = 1,
  CTX_JOIN_MITER = 2
} CtxLineJoin;

typedef enum
{
  CTX_CAP_NONE   = 0,
  CTX_CAP_ROUND  = 1,
  CTX_CAP_SQUARE = 2
} CtxLineCap;

typedef enum
{
  CTX_TEXT_BASELINE_ALPHABETIC = 0,
  CTX_TEXT_BASELINE_TOP,
  CTX_TEXT_BASELINE_HANGING,
  CTX_TEXT_BASELINE_MIDDLE,
  CTX_TEXT_BASELINE_IDEOGRAPHIC,
  CTX_TEXT_BASELINE_BOTTOM
} CtxTextBaseline;

typedef enum
{
  CTX_TEXT_ALIGN_START = 0,
  CTX_TEXT_ALIGN_END,
  CTX_TEXT_ALIGN_CENTER,
  CTX_TEXT_ALIGN_LEFT,
  CTX_TEXT_ALIGN_RIGHT
} CtxTextAlign;

typedef enum
{
  CTX_TEXT_DIRECTION_INHERIT = 0,
  CTX_TEXT_DIRECTION_LTR,
  CTX_TEXT_DIRECTION_RTL
} CtxTextDirection;

struct
_CtxGlyph
{
  uint32_t index;
  float    x;
  float    y;
};

CtxTextAlign ctx_get_text_align (Ctx *ctx);
CtxTextBaseline ctx_get_text_baseline (Ctx *ctx);
CtxTextDirection ctx_get_text_direction (Ctx *ctx);
CtxFillRule ctx_get_fill_rule (Ctx *ctx);
CtxLineCap ctx_get_line_cap (Ctx *ctx);
CtxLineJoin ctx_get_line_join (Ctx *ctx);
CtxCompositingMode ctx_get_compositing_mode (Ctx *ctx);
CtxBlend ctx_get_blend_mode (Ctx *ctx);

void ctx_gradient_add_stop_string (Ctx *ctx, float pos, const char *color);

void ctx_text_align           (Ctx *ctx, CtxTextAlign      align);
void ctx_text_baseline        (Ctx *ctx, CtxTextBaseline   baseline);
void ctx_text_direction       (Ctx *ctx, CtxTextDirection  direction);
void ctx_fill_rule            (Ctx *ctx, CtxFillRule       fill_rule);
void ctx_line_cap             (Ctx *ctx, CtxLineCap        cap);
void ctx_line_join            (Ctx *ctx, CtxLineJoin       join);
void ctx_compositing_mode     (Ctx *ctx, CtxCompositingMode mode);
int  ctx_set_drawlist     (Ctx *ctx, void *data, int length);
typedef struct _CtxEntry CtxEntry;
/* we only care about the tight packing for this specific
 * struct as we do indexing across members in arrays of it,
 * to make sure its size becomes 9bytes -
 * the pack pragma is also sufficient on recent gcc versions
 */
#pragma pack(push,1)
struct
  _CtxEntry
{
  uint8_t code;
  union
  {
    float    f[2];
    uint8_t  u8[8];
    int8_t   s8[8];
    uint16_t u16[4];
    int16_t  s16[4];
    uint32_t u32[2];
    int32_t  s32[2];
    uint64_t u64[1]; // unused
  } data; // 9bytes long, we're favoring compactness and correctness
  // over performance. By sacrificing float precision, zeroing
  // first 8bit of f[0] would permit 8bytes long and better
  // aglinment and cacheline behavior.
};
#pragma pack(pop)
const CtxEntry *ctx_get_drawlist (Ctx *ctx);
int  ctx_append_drawlist  (Ctx *ctx, void *data, int length);

/* these are only needed for clients rendering text, as all text gets
 * converted to paths.
 */
void  ctx_glyphs        (Ctx        *ctx,
                         CtxGlyph   *glyphs,
                         int         n_glyphs);

void  ctx_glyphs_stroke (Ctx       *ctx,
                         CtxGlyph   *glyphs,
                         int         n_glyphs);

void  ctx_text          (Ctx        *ctx,
                         const char *string);
void  ctx_text_stroke   (Ctx        *ctx,
                         const char *string);

void  ctx_fill_text     (Ctx        *ctx,
                         const char *string,
                         float       x,
                         float       y);

void  ctx_stroke_text   (Ctx        *ctx,
                         const char *string,
                         float       x,
                         float       y);

/* returns the total horizontal advance if string had been rendered */
float ctx_text_width    (Ctx        *ctx,
                         const char *string);

float ctx_glyph_width   (Ctx *ctx, int unichar);

int   ctx_load_font_ttf (const char *name, const void *ttf_contents, int length);



enum _CtxModifierState
{
  CTX_MODIFIER_STATE_SHIFT   = (1<<0),
  CTX_MODIFIER_STATE_CONTROL = (1<<1),
  CTX_MODIFIER_STATE_ALT     = (1<<2),
  CTX_MODIFIER_STATE_BUTTON1 = (1<<3),
  CTX_MODIFIER_STATE_BUTTON2 = (1<<4),
  CTX_MODIFIER_STATE_BUTTON3 = (1<<5),
  CTX_MODIFIER_STATE_DRAG    = (1<<6), // pointer button is down (0 or any)
};
typedef enum _CtxModifierState CtxModifierState;

enum _CtxScrollDirection
{
  CTX_SCROLL_DIRECTION_UP,
  CTX_SCROLL_DIRECTION_DOWN,
  CTX_SCROLL_DIRECTION_LEFT,
  CTX_SCROLL_DIRECTION_RIGHT
};
typedef enum _CtxScrollDirection CtxScrollDirection;

typedef struct _CtxEvent CtxEvent;

void ctx_set_renderer (Ctx *ctx,
                       void *renderer);
void *ctx_get_renderer (Ctx *ctx);

int ctx_renderer_is_sdl (Ctx *ctx);
int ctx_renderer_is_fb (Ctx *ctx);
int ctx_renderer_is_ctx (Ctx *ctx);
int ctx_renderer_is_term (Ctx *ctx);

/* the following API is only available when CTX_EVENTS is defined to 1
 *
 * it provides the ability to register callbacks with the current path
 * that get delivered with transformed coordinates.
 */
int ctx_is_dirty (Ctx *ctx);
void ctx_set_dirty (Ctx *ctx, int dirty);
float ctx_get_float (Ctx *ctx, uint64_t hash);
void ctx_set_float (Ctx *ctx, uint64_t hash, float value);

unsigned long ctx_ticks (void);
void ctx_flush (Ctx *ctx);

void ctx_set_clipboard (Ctx *ctx, const char *text);
char *ctx_get_clipboard (Ctx *ctx);

void _ctx_events_init     (Ctx *ctx);
typedef struct _CtxIntRectangle CtxIntRectangle;
struct _CtxIntRectangle {
  int x;
  int y;
  int width;
  int height;
};

void ctx_quit (Ctx *ctx);
int  ctx_has_quit (Ctx *ctx);

typedef void (*CtxCb) (CtxEvent *event,
                       void     *data,
                       void     *data2);
typedef void (*CtxDestroyNotify) (void *data);

enum _CtxEventType {
  CTX_PRESS        = 1 << 0,
  CTX_MOTION       = 1 << 1,
  CTX_RELEASE      = 1 << 2,
  CTX_ENTER        = 1 << 3,
  CTX_LEAVE        = 1 << 4,
  CTX_TAP          = 1 << 5,
  CTX_TAP_AND_HOLD = 1 << 6,

  /* NYI: SWIPE, ZOOM ROT_ZOOM, */

  CTX_DRAG_PRESS   = 1 << 7,
  CTX_DRAG_MOTION  = 1 << 8,
  CTX_DRAG_RELEASE = 1 << 9,
  CTX_KEY_PRESS    = 1 << 10,
  CTX_KEY_DOWN     = 1 << 11,
  CTX_KEY_UP       = 1 << 12,
  CTX_SCROLL       = 1 << 13,
  CTX_MESSAGE      = 1 << 14,
  CTX_DROP         = 1 << 15,

  CTX_SET_CURSOR   = 1 << 16, // used internally

  /* client should store state - preparing
                                 * for restart
                                 */
  CTX_POINTER  = (CTX_PRESS | CTX_MOTION | CTX_RELEASE | CTX_DROP),
  CTX_TAPS     = (CTX_TAP | CTX_TAP_AND_HOLD),
  CTX_CROSSING = (CTX_ENTER | CTX_LEAVE),
  CTX_DRAG     = (CTX_DRAG_PRESS | CTX_DRAG_MOTION | CTX_DRAG_RELEASE),
  CTX_KEY      = (CTX_KEY_DOWN | CTX_KEY_UP | CTX_KEY_PRESS),
  CTX_MISC     = (CTX_MESSAGE),
  CTX_ANY      = (CTX_POINTER | CTX_DRAG | CTX_CROSSING | CTX_KEY | CTX_MISC | CTX_TAPS),
};
typedef enum _CtxEventType CtxEventType;

#define CTX_CLICK   CTX_PRESS   // SHOULD HAVE MORE LOGIC

struct _CtxEvent {
  CtxEventType  type;
  uint32_t time;
  Ctx     *ctx;
  int stop_propagate; /* when set - propagation is stopped */

  CtxModifierState state;

  int     device_no; /* 0 = left mouse button / virtual focus */
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


  unsigned int unicode; /* only valid for key-events, re-use as keycode? */
  const char *string;   /* as key can be "up" "down" "space" "backspace" "a" "b" "ø" etc .. */
                        /* this is also where the message is delivered for
                         * MESSAGE events
                         *
                         * and the data for drop events are delivered
                         */
  CtxScrollDirection scroll_direction;


  // would be nice to add the bounding box of the hit-area causing
  // the event, making for instance scissored enter/leave repaint easier.
};

// layer-event "layer"  motion x y device_no 

void ctx_add_key_binding_full (Ctx *ctx,
                               const char *key,
                               const char *action,
                               const char *label,
                               CtxCb       cb,
                               void       *cb_data,
                               CtxDestroyNotify destroy_notify,
                               void       *destroy_data);
void ctx_add_key_binding (Ctx *ctx,
                          const char *key,
                          const char *action,
                          const char *label,
                          CtxCb cb,
                          void  *cb_data);
typedef struct CtxBinding {
  char *nick;
  char *command;
  char *label;
  CtxCb cb;
  void *cb_data;
  CtxDestroyNotify destroy_notify;
  void  *destroy_data;
} CtxBinding;
CtxBinding *ctx_get_bindings (Ctx *ctx);
void  ctx_clear_bindings     (Ctx *ctx);
void  ctx_remove_idle        (Ctx *ctx, int handle);
int   ctx_add_timeout_full   (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                              void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_timeout        (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);
int   ctx_add_idle_full      (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                              void (*destroy_notify)(void *destroy_data), void *destroy_data);
int   ctx_add_idle           (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data);


void ctx_add_hit_region (Ctx *ctx, const char *id);

void ctx_set_title (Ctx *ctx, const char *title);

void ctx_listen_full (Ctx     *ctx,
                      float    x,
                      float    y,
                      float    width,
                      float    height,
                      CtxEventType  types,
                      CtxCb    cb,
                      void    *data1,
                      void    *data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data);
void  ctx_event_stop_propagate (CtxEvent *event);
void  ctx_listen               (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2);
void  ctx_listen_with_finalize (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data);

void ctx_init (int *argc, char ***argv); // is a no-op but could launch
                                         // terminal
CtxEvent *ctx_get_event (Ctx *ctx);
int       ctx_has_event (Ctx *ctx, int timeout);
void      ctx_get_event_fds (Ctx *ctx, int *fd, int *count);

int   ctx_pointer_is_down (Ctx *ctx, int no);
float ctx_pointer_x (Ctx *ctx);
float ctx_pointer_y (Ctx *ctx);
void  ctx_freeze (Ctx *ctx);
void  ctx_thaw   (Ctx *ctx);
int   ctx_events_frozen (Ctx *ctx);
void  ctx_events_clear_items (Ctx *ctx);
int   ctx_events_width (Ctx *ctx);
int   ctx_events_height (Ctx *ctx);

/* The following functions drive the event delivery, registered callbacks
 * are called in response to these being called.
 */

int ctx_key_down  (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);
int ctx_key_up    (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);
int ctx_key_press (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time);


int ctx_scrolled  (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time);
void ctx_incoming_message (Ctx *ctx, const char *message, long time);
int ctx_pointer_motion    (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_release   (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_press     (Ctx *ctx, float x, float y, int device_no, uint32_t time);
int ctx_pointer_drop      (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                           char *string);

typedef enum
{
  CTX_CONT             = '\0', // - contains args from preceding entry
  CTX_NOP              = ' ', //
  CTX_DATA             = '(', // size size-in-entries - u32
  CTX_DATA_REV         = ')', // reverse traversal data marker
  CTX_SET_RGBA_U8      = '*', // r g b a - u8
  CTX_NEW_EDGE         = '+', // x0 y0 x1 y1 - s16
  // set pixel might want a shorter ascii form? or keep it an embedded
  // only option?
  CTX_SET_PIXEL        = '-', // 8bit "fast-path" r g b a x y - u8 for rgba, and u16 for x,y
  /* optimizations that reduce the number of entries used,
   * not visible outside the drawlist compression, thus
   * using entries that cannot be used directly as commands
   * since they would be interpreted as numbers - if values>127
   * then the embedded font data is harder to escape.
   */
  CTX_REL_LINE_TO_X4            = '0', // x1 y1 x2 y2 x3 y3 x4 y4   -- s8
  CTX_REL_LINE_TO_REL_CURVE_TO  = '1', // x1 y1 cx1 cy1 cx2 cy2 x y -- s8
  CTX_REL_CURVE_TO_REL_LINE_TO  = '2', // cx1 cy1 cx2 cy2 x y x1 y1 -- s8
  CTX_REL_CURVE_TO_REL_MOVE_TO  = '3', // cx1 cy1 cx2 cy2 x y x1 y1 -- s8
  CTX_REL_LINE_TO_X2            = '4', // x1 y1 x2 y2 -- s16
  CTX_MOVE_TO_REL_LINE_TO       = '5', // x1 y1 x2 y2 -- s16
  CTX_REL_LINE_TO_REL_MOVE_TO   = '6', // x1 y1 x2 y2 -- s16
  CTX_FILL_MOVE_TO              = '7', // x y
  CTX_REL_QUAD_TO_REL_QUAD_TO   = '8', // cx1 x1 cy1 y1 cx1 x2 cy1 y1 -- s8
  CTX_REL_QUAD_TO_S16           = '9', // cx1 cy1 x y                 - s16
  // expand with: . : 
  CTX_FLUSH            = ';',

  CTX_DEFINE_GLYPH     = '@', // unichar width - u32
  CTX_ARC_TO           = 'A', // x1 y1 x2 y2 radius
  CTX_ARC              = 'B', // x y radius angle1 angle2 direction
  CTX_CURVE_TO         = 'C', // cx1 cy1 cx2 cy2 x y
  CTX_STROKE           = 'E', //
  CTX_FILL             = 'F', //
  CTX_RESTORE          = 'G', //
  CTX_HOR_LINE_TO      = 'H', // x
  CTX_DEFINE_TEXTURE   = 'I', // "eid" width height format "data"
  CTX_ROTATE           = 'J', // radians
  CTX_COLOR            = 'K', // model, c1 c2 c3 ca - has a variable set of
  // arguments.
  CTX_LINE_TO          = 'L', // x y
  CTX_MOVE_TO          = 'M', // x y
  CTX_BEGIN_PATH       = 'N', //
  CTX_SCALE            = 'O', // xscale yscale
  CTX_NEW_PAGE         = 'P', // - NYI - optional page-size
  CTX_QUAD_TO          = 'Q', // cx cy x y
  CTX_VIEW_BOX         = 'R', // x y width height
  CTX_SMOOTH_TO        = 'S', // cx cy x y
  CTX_SMOOTHQ_TO       = 'T', // x y
  CTX_RESET            = 'U', //
  CTX_VER_LINE_TO      = 'V', // y
  CTX_APPLY_TRANSFORM  = 'W', // a b c d e f - for set_transform combine with identity
  CTX_EXIT             = 'X', //
  CTX_ROUND_RECTANGLE  = 'Y', // x y width height radius

  CTX_CLOSE_PATH2      = 'Z', //
  CTX_STROKE_SOURCE    = '_', // next source definition applies to strokes
  CTX_KERNING_PAIR     = '[', // glA glB kerning, glA and glB in u16 kerning in s32
  CTX_COLOR_SPACE      = ']', // IccSlot  data  data_len,
                         //    data can be a string with a name,
                         //    icc data or perhaps our own serialization
                         //    of profile data
  CTX_EDGE_FLIPPED     = '`', // x0 y0 x1 y1 - s16
  CTX_REL_ARC_TO       = 'a', // x1 y1 x2 y2 radius
  CTX_CLIP             = 'b',
  CTX_REL_CURVE_TO     = 'c', // cx1 cy1 cx2 cy2 x y
  CTX_LINE_DASH        = 'd', // dashlen0 [dashlen1 ...]
  CTX_TRANSLATE        = 'e', // x y
  CTX_LINEAR_GRADIENT  = 'f', // x1 y1 x2 y2
  CTX_SAVE             = 'g',
  CTX_REL_HOR_LINE_TO  = 'h', // x
  CTX_TEXTURE          = 'i',
  CTX_PRESERVE         = 'j', // 
  CTX_SET_KEY          = 'k', // - used together with another char to identify
                              //   a key to set
  CTX_REL_LINE_TO      = 'l', // x y
  CTX_REL_MOVE_TO      = 'm', // x y
  CTX_FONT             = 'n', // as used by text parser
  CTX_RADIAL_GRADIENT  = 'o', // x1 y1 radius1 x2 y2 radius2
  CTX_GRADIENT_STOP    = 'p', // argument count depends on current color model
  CTX_REL_QUAD_TO      = 'q', // cx cy x y
  CTX_RECTANGLE        = 'r', // x y width height
  CTX_REL_SMOOTH_TO    = 's', // cx cy x y
  CTX_REL_SMOOTHQ_TO   = 't', // x y
  CTX_STROKE_TEXT      = 'u', // string - utf8 string
  CTX_REL_VER_LINE_TO  = 'v', // y
  CTX_GLYPH            = 'w', // unichar fontsize
  CTX_TEXT             = 'x', // string | kern - utf8 data to shape or horizontal kerning amount
  CTX_IDENTITY         = 'y', //
  CTX_CLOSE_PATH       = 'z', //
  CTX_START_GROUP      = '{',
  CTX_END_GROUP        = '}',
  CTX_EDGE             = ',',

  /* though expressed as two chars in serialization we have
   * dedicated byte commands for the setters to keep the dispatch
   * simpler. There is no need for these to be human readable thus we go >128
   *
   * unused:        !&<=>?:.=/\
   * reserved:      '"&   #  %^@
   */


  CTX_FILL_RULE        = 128, // kr rule - u8, default = CTX_FILLE_RULE_EVEN_ODD
  CTX_BLEND_MODE       = 129, // kB mode - u8 , default=0

  CTX_MITER_LIMIT      = 130, // km limit - float, default = 0.0

  CTX_LINE_JOIN        = 131, // kj join - u8 , default=0
  CTX_LINE_CAP         = 132, // kc cap - u8, default = 0
  CTX_LINE_WIDTH       = 133, // kw width, default = 2.0
  CTX_GLOBAL_ALPHA     = 134, // ka alpha - default=1.0
  CTX_COMPOSITING_MODE = 135, // kc mode - u8 , default=0

  CTX_FONT_SIZE        = 136, // kf size - float, default=?
  CTX_TEXT_ALIGN       = 137, // kt align - u8, default = CTX_TEXT_ALIGN_START
  CTX_TEXT_BASELINE    = 138, // kb baseline - u8, default = CTX_TEXT_ALIGN_ALPHABETIC
  CTX_TEXT_DIRECTION   = 139, // kd

  CTX_SHADOW_BLUR      = 140, // ks
  CTX_SHADOW_COLOR     = 141, // kC
  CTX_SHADOW_OFFSET_X  = 142, // kx
  CTX_SHADOW_OFFSET_Y  = 143, // ky
  CTX_IMAGE_SMOOTHING  = 144, // kS
  CTX_LINE_DASH_OFFSET = 145, // kD lineDashOffset

  // items marked with % are currently only for the parser
  // for instance for svg compatibility or simulated/converted color spaces
  // not the serialization/internal render stream
  //
  CTX_STROKE_RECT      = 200, // strokeRect - only exist in long form
  CTX_FILL_RECT        = 201, // fillRect   - only exist in long form
} CtxCode;


#pragma pack(push,1)

typedef struct _CtxCommand CtxCommand;
typedef struct _CtxIterator CtxIterator;

CtxIterator *
ctx_current_path (Ctx *ctx);
void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2);

#define CTX_ASSERT               0

#if CTX_ASSERT==1
#define ctx_assert(a)  if(!(a)){fprintf(stderr,"%s:%i assertion failed\n", __FUNCTION__, __LINE__);  }
#else
#define ctx_assert(a)
#endif

int ctx_get_drawlist_count (Ctx *ctx);

struct
  _CtxCommand
{
  union
  {
    uint8_t  code;
    CtxEntry entry;
    struct
    {
      uint8_t code;
      float scalex;
      float scaley;
    } scale;
    struct
    {
      uint8_t code;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t cont;
      uint8_t data[8]; /* ... and continues */
    } data;
    struct
    {
      uint8_t code;
      uint32_t stringlen;
      uint32_t blocklen;
    } data_rev;
    struct
    {
      uint8_t code;
      float pad;
      float pad2;
      uint8_t code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t code_cont;
      uint8_t utf8[8]; /* .. and continues */
    } text;
    struct
    {
      uint8_t  code;
      uint32_t key_hash;
      float    pad;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } set;
    struct
    {
      uint8_t  code;
      uint32_t pad0;
      float    pad1;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } get;
    struct {
      uint8_t  code;
      uint32_t count; /* better than byte_len in code, but needs to then be set   */
      float    pad1;
      uint8_t  code_data;
      uint32_t byte_len;
      uint32_t blocklen;
      uint8_t  code_cont;
      float    data[2]; /* .. and - possibly continues */
    } line_dash;
    struct {
      uint8_t  code;
      uint32_t space_slot;
      float    pad1;
      uint8_t  code_data;
      uint32_t data_len;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  data[8]; /* .. and continues */
    } colorspace;
    struct
    {
      uint8_t  code;
      float    x;
      float    y;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      char     eid[8]; /* .. and continues */
    } texture;
    struct
    {
      uint8_t  code;
      uint32_t width;
      uint32_t height;
      uint8_t  code_cont0;
      uint16_t format;
      uint16_t pad0;
      uint32_t pad1;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont1;
      char     eid[8]; /* .. and continues */
      // followed by - in variable offset code_Data, data_len, datablock_len, cont, pixeldata
    } define_texture;
    struct
    {
      uint8_t  code;
      float    pad;
      float    pad2;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } text_stroke;
    struct
    {
      uint8_t  code;
      float    pad;
      float    pad2;
      uint8_t  code_data;
      uint32_t stringlen;
      uint32_t blocklen;
      uint8_t  code_cont;
      uint8_t  utf8[8]; /* .. and continues */
    } set_font;
    struct
    {
      uint8_t code;
      float model;
      float r;
      uint8_t pad1;
      float g;
      float b;
      uint8_t pad2;
      float a;
    } rgba;
    struct
    {
      uint8_t code;
      float model;
      float c;
      uint8_t pad1;
      float m;
      float y;
      uint8_t pad2;
      float k;
      float a;
    } cmyka;
    struct
    {
      uint8_t code;
      float model;
      float g;
      uint8_t pad1;
      float a;
    } graya;
    struct
    {
      uint8_t code;
      float model;
      float c0;
      uint8_t pad1;
      float c1;
      float c2;
      uint8_t pad2;
      float c3;
      float c4;
      uint8_t pad3;
      float c5;
      float c6;
      uint8_t pad4;
      float c7;
      float c8;
      uint8_t pad5;
      float c9;
      float c10;
    } set_color;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } rel_move_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } rel_line_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } line_to;
    struct
    {
      uint8_t code;
      float cx1;
      float cy1;
      uint8_t pad0;
      float cx2;
      float cy2;
      uint8_t pad1;
      float x;
      float y;
    } rel_curve_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
    } move_to;
    struct
    {
      uint8_t code;
      float cx1;
      float cy1;
      uint8_t pad0;
      float cx2;
      float cy2;
      uint8_t pad1;
      float x;
      float y;
    } curve_to;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float r1;
      float x2;
      uint8_t pad1;
      float y2;
      float r2;
    } radial_gradient;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float x2;
      float y2;
    } linear_gradient;
    struct
    {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float width;
      float height;
      uint8_t pad1;
      float radius;
    } rectangle;
    struct {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float width;
      float height;
    } view_box;

    struct
    {
      uint8_t code;
      uint16_t glyph_before;
      uint16_t glyph_after;
       int32_t amount;
    } kern;

    struct
    {
      uint8_t code;
      uint32_t glyph;
      uint32_t advance; // * 256
    } define_glyph;

    struct
    {
      uint8_t code;
      uint8_t rgba[4];
      uint16_t x;
      uint16_t y;
    } set_pixel;
    struct
    {
      uint8_t code;
      float cx;
      float cy;
      uint8_t pad0;
      float x;
      float y;
    } quad_to;
    struct
    {
      uint8_t code;
      float cx;
      float cy;
      uint8_t pad0;
      float x;
      float y;
    } rel_quad_to;
    struct
    {
      uint8_t code;
      float x;
      float y;
      uint8_t pad0;
      float radius;
      float angle1;
      uint8_t pad1;
      float angle2;
      float direction;
    }
    arc;
    struct
    {
      uint8_t code;
      float x1;
      float y1;
      uint8_t pad0;
      float x2;
      float y2;
      uint8_t pad1;
      float radius;
    }
    arc_to;
    /* some format specific generic accesors:  */
    struct
    {
      uint8_t code;
      float   x0;
      float   y0;
      uint8_t pad0;
      float   x1;
      float   y1;
      uint8_t pad1;
      float   x2;
      float   y2;
      uint8_t pad2;
      float   x3;
      float   y3;
      uint8_t pad3;
      float   x4;
      float   y4;
    } c;
    struct
    {
      uint8_t code;
      float   a0;
      float   a1;
      uint8_t pad0;
      float   a2;
      float   a3;
      uint8_t pad1;
      float   a4;
      float   a5;
      uint8_t pad2;
      float   a6;
      float   a7;
      uint8_t pad3;
      float   a8;
      float   a9;
    } f;
    struct
    {
      uint8_t  code;
      uint32_t a0;
      uint32_t a1;
      uint8_t  pad0;
      uint32_t a2;
      uint32_t a3;
      uint8_t  pad1;
      uint32_t a4;
      uint32_t a5;
      uint8_t  pad2;
      uint32_t a6;
      uint32_t a7;
      uint8_t  pad3;
      uint32_t a8;
      uint32_t a9;
    } u32;
    struct
    {
      uint8_t  code;
      uint64_t a0;
      uint8_t  pad0;
      uint64_t a1;
      uint8_t  pad1;
      uint64_t a2;
      uint8_t  pad2;
      uint64_t a3;
      uint8_t  pad3;
      uint64_t a4;
    } u64;
    struct
    {
      uint8_t code;
      int32_t a0;
      int32_t a1;
      uint8_t pad0;
      int32_t a2;
      int32_t a3;
      uint8_t pad1;
      int32_t a4;
      int32_t a5;
      uint8_t pad2;
      int32_t a6;
      int32_t a7;
      uint8_t pad3;
      int32_t a8;
      int32_t a9;
    } s32;
    struct
    {
      uint8_t code;
      int16_t a0;
      int16_t a1;
      int16_t a2;
      int16_t a3;
      uint8_t pad0;
      int16_t a4;
      int16_t a5;
      int16_t a6;
      int16_t a7;
      uint8_t pad1;
      int16_t a8;
      int16_t a9;
      int16_t a10;
      int16_t a11;
      uint8_t pad2;
      int16_t a12;
      int16_t a13;
      int16_t a14;
      int16_t a15;
      uint8_t pad3;
      int16_t a16;
      int16_t a17;
      int16_t a18;
      int16_t a19;
    } s16;
    struct
    {
      uint8_t code;
      uint16_t a0;
      uint16_t a1;
      uint16_t a2;
      uint16_t a3;
      uint8_t pad0;
      uint16_t a4;
      uint16_t a5;
      uint16_t a6;
      uint16_t a7;
      uint8_t pad1;
      uint16_t a8;
      uint16_t a9;
      uint16_t a10;
      uint16_t a11;
      uint8_t pad2;
      uint16_t a12;
      uint16_t a13;
      uint16_t a14;
      uint16_t a15;
      uint8_t pad3;
      uint16_t a16;
      uint16_t a17;
      uint16_t a18;
      uint16_t a19;
    } u16;
    struct
    {
      uint8_t code;
      uint8_t a0;
      uint8_t a1;
      uint8_t a2;
      uint8_t a3;
      uint8_t a4;
      uint8_t a5;
      uint8_t a6;
      uint8_t a7;
      uint8_t pad0;
      uint8_t a8;
      uint8_t a9;
      uint8_t a10;
      uint8_t a11;
      uint8_t a12;
      uint8_t a13;
      uint8_t a14;
      uint8_t a15;
      uint8_t pad1;
      uint8_t a16;
      uint8_t a17;
      uint8_t a18;
      uint8_t a19;
      uint8_t a20;
      uint8_t a21;
      uint8_t a22;
      uint8_t a23;
    } u8;
    struct
    {
      uint8_t code;
      int8_t a0;
      int8_t a1;
      int8_t a2;
      int8_t a3;
      int8_t a4;
      int8_t a5;
      int8_t a6;
      int8_t a7;
      uint8_t pad0;
      int8_t a8;
      int8_t a9;
      int8_t a10;
      int8_t a11;
      int8_t a12;
      int8_t a13;
      int8_t a14;
      int8_t a15;
      uint8_t pad1;
      int8_t a16;
      int8_t a17;
      int8_t a18;
      int8_t a19;
      int8_t a20;
      int8_t a21;
      int8_t a22;
      int8_t a23;
    } s8;
  };
  CtxEntry next_entry; // also pads size of CtxCommand slightly.
};

typedef struct _CtxImplementation CtxImplementation;
struct _CtxImplementation
{
  void (*process)        (void *renderer, CtxCommand *entry);
  void (*reset)          (void *renderer);
  void (*flush)          (void *renderer);
  char *(*get_clipboard) (void *ctxctx);
  void (*set_clipboard)  (void *ctxctx, const char *text);
  void (*free)           (void *renderer);
};

CtxCommand *ctx_iterator_next (CtxIterator *iterator);

#define ctx_arg_string()  ((char*)&entry[2].data.u8[0])


/* The above should be public API
 */

#pragma pack(pop)

/* access macros for nth argument of a given type when packed into
 * an CtxEntry pointer in current code context
 */
#define ctx_arg_float(no) entry[(no)>>1].data.f[(no)&1]
#define ctx_arg_u64(no)   entry[(no)].data.u64[0]
#define ctx_arg_u32(no)   entry[(no)>>1].data.u32[(no)&1]
#define ctx_arg_s32(no)   entry[(no)>>1].data.s32[(no)&1]
#define ctx_arg_u16(no)   entry[(no)>>2].data.u16[(no)&3]
#define ctx_arg_s16(no)   entry[(no)>>2].data.s16[(no)&3]
#define ctx_arg_u8(no)    entry[(no)>>3].data.u8[(no)&7]
#define ctx_arg_s8(no)    entry[(no)>>3].data.s8[(no)&7]
#define ctx_arg_string()  ((char*)&entry[2].data.u8[0])

typedef enum
{
  CTX_GRAY           = 1,
  CTX_RGB            = 3,
  CTX_DRGB           = 4,
  CTX_CMYK           = 5,
  CTX_DCMYK          = 6,
  CTX_LAB            = 7,
  CTX_LCH            = 8,
  CTX_GRAYA          = 101,
  CTX_RGBA           = 103,
  CTX_DRGBA          = 104,
  CTX_CMYKA          = 105,
  CTX_DCMYKA         = 106,
  CTX_LABA           = 107,
  CTX_LCHA           = 108,
  CTX_GRAYA_A        = 201,
  CTX_RGBA_A         = 203,
  CTX_RGBA_A_DEVICE  = 204,
  CTX_CMYKA_A        = 205,
  CTX_DCMYKA_A       = 206,
  // RGB  device and  RGB  ?
} CtxColorModel;

enum _CtxAntialias
{
  CTX_ANTIALIAS_DEFAULT,
  CTX_ANTIALIAS_NONE, // non-antialiased
  CTX_ANTIALIAS_FAST, // aa 3
  CTX_ANTIALIAS_GOOD, // aa 5
  CTX_ANTIALIAS_BEST  // aa 17
};
typedef enum _CtxAntialias CtxAntialias;

enum _CtxCursor
{
  CTX_CURSOR_UNSET,
  CTX_CURSOR_NONE,
  CTX_CURSOR_ARROW,
  CTX_CURSOR_IBEAM,
  CTX_CURSOR_WAIT,
  CTX_CURSOR_HAND,
  CTX_CURSOR_CROSSHAIR,
  CTX_CURSOR_RESIZE_ALL,
  CTX_CURSOR_RESIZE_N,
  CTX_CURSOR_RESIZE_S,
  CTX_CURSOR_RESIZE_E,
  CTX_CURSOR_RESIZE_NE,
  CTX_CURSOR_RESIZE_SE,
  CTX_CURSOR_RESIZE_W,
  CTX_CURSOR_RESIZE_NW,
  CTX_CURSOR_RESIZE_SW,
  CTX_CURSOR_MOVE
};
typedef enum _CtxCursor CtxCursor;

/* to be used immediately after a ctx_listen or ctx_listen_full causing the
 * cursor to change when hovering the listen area.
 */
void ctx_listen_set_cursor (Ctx      *ctx,
                            CtxCursor cursor);

/* lower level cursor setting that is independent of ctx event handling
 */
void         ctx_set_cursor (Ctx *ctx, CtxCursor cursor);
CtxCursor    ctx_get_cursor (Ctx *ctx);
void         ctx_set_antialias (Ctx *ctx, CtxAntialias antialias);
CtxAntialias ctx_get_antialias (Ctx *ctx);
void         ctx_set_render_threads   (Ctx *ctx, int n_threads);
int          ctx_get_render_threads   (Ctx *ctx);

void         ctx_set_hash_cache (Ctx *ctx, int enable_hash_cache);
int          ctx_get_hash_cache (Ctx *ctx);


typedef struct _CtxParser CtxParser;
  CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data);


enum _CtxColorSpace
{
  CTX_COLOR_SPACE_DEVICE_RGB,
  CTX_COLOR_SPACE_DEVICE_CMYK,
  CTX_COLOR_SPACE_USER_RGB,
  CTX_COLOR_SPACE_USER_CMYK,
  CTX_COLOR_SPACE_TEXTURE
};
typedef enum _CtxColorSpace CtxColorSpace;

/* sets the color space for a slot, the space is either a string of
 * "sRGB" "rec2020" .. etc or an icc profile.
 *
 * The slots device_rgb and device_cmyk is mostly to be handled outside drawing 
 * code, and user_rgb and user_cmyk is to be used. With no user_cmyk set
 * user_cmyk == device_cmyk.
 *
 * The set profiles follows the graphics state.
 */
void ctx_color_space (Ctx           *ctx,
                      CtxColorSpace  space_slot,
                      unsigned char *data,
                      int            data_length);

void
ctx_parser_set_size (CtxParser *parser,
                     int        width,
                     int        height,
                     float      cell_width,
                     float      cell_height);

void ctx_parser_feed_byte (CtxParser *parser, int byte);

int
ctx_get_contents (const char     *path,
                   unsigned char **contents,
                   long           *length);

void ctx_parser_free (CtxParser *parser);
typedef struct _CtxSHA1 CtxSHA1;

void
ctx_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii);
int
ctx_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin);
float ctx_term_get_cell_width (Ctx *ctx);
float ctx_term_get_cell_height (Ctx *ctx);

#ifndef CTX_CODEC_CHAR
//#define CTX_CODEC_CHAR '\035'
//#define CTX_CODEC_CHAR 'a'
#define CTX_CODEC_CHAR '\020' // datalink escape
//#define CTX_CODEC_CHAR '^'
#endif

#ifndef assert
#define assert(a)
#endif

#ifdef __cplusplus
}
#endif
#endif
#ifndef __CTX_H__
#define __CTX_H__
#ifndef CTX_STRING_H
#define CTX_STRING_H

typedef struct _CtxString CtxString;
struct _CtxString
{
  char *str;
  int   length;
  int   utf8_length;
  int   allocated_length;
  int   is_line;
};

CtxString   *ctx_string_new_with_size  (const char *initial, int initial_size);
CtxString   *ctx_string_new            (const char *initial);
void        ctx_string_free           (CtxString *string, int freealloc);
const char *ctx_string_get            (CtxString *string);
uint32_t    ctx_string_get_unichar    (CtxString *string, int pos);
int         ctx_string_get_length     (CtxString *string);
int         ctx_string_get_utf8length (CtxString *string);
void        ctx_string_set            (CtxString *string, const char *new_string);
void        ctx_string_clear          (CtxString *string);
void        ctx_string_append_str     (CtxString *string, const char *str);
void        ctx_string_append_byte    (CtxString *string, char  val);
void        ctx_string_append_string  (CtxString *string, CtxString *string2);
void        ctx_string_append_unichar (CtxString *string, unsigned int unichar);
void        ctx_string_append_data    (CtxString *string, const char *data, int len);

void        ctx_string_append_utf8char (CtxString *string, const char *str);
void        ctx_string_append_printf  (CtxString *string, const char *format, ...);
void        ctx_string_replace_utf8   (CtxString *string, int pos, const char *new_glyph);
void        ctx_string_insert_utf8    (CtxString *string, int pos, const char *new_glyph);

void        ctx_string_insert_unichar (CtxString *string, int pos, uint32_t unichar);
void        ctx_string_replace_unichar (CtxString *string, int pos, uint32_t unichar);
void        ctx_string_remove         (CtxString *string, int pos);
char       *ctx_strdup_printf         (const char *format, ...);

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
#ifndef _CTX_INTERNAL_FONT_
#define _CTX_INTERNAL_FONT_

#ifndef CTX_FONT_ascii
/* this is a ctx encoded font based on DejaVuSans.ttf */
/* CTX_SUBDIV:8  CTX_BAKE_FONT_SIZE:160 */
/* glyphs covered: 

 !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghi
  jklmnopqrstuvwxyz{|}~  */
static const struct __attribute__ ((packed)) {uint8_t code; uint32_t a; uint32_t b;}
ctx_font_ascii[]={
{'@', 0x00000020, 0x00002bb0},/*                 x-advance: 43.687500 */
{'@', 0x00000021, 0x00003719},/*        !        x-advance: 55.097656 */
{'M', 0x41a5e7f2, 0xc1886037},
{'l', 0x4159fc90, 0x00000000},
{'4', 0x00880000, 0x0000ff94},
{'6', 0xff780000, 0xfd670000},
{'l', 0x4159fc90, 0x00000000},
{'l', 0x00000000, 0x422fd6c4},
{'l', 0xbfabcfe0, 0x41bfad86},
{'l', 0xc12df5b2, 0x00000000},
{'l', 0xbfb46710, 0xc1bfad86},
{'l', 0x00000000, 0xc22fd6c4},
{'@', 0x00000022, 0x00003f38},/*        "        x-advance: 63.218750 */
{'M', 0x41c50c07, 0xc2c86716},
{'l', 0x00000000, 0x4214fe48},
{'4', 0x0000ffa5, 0xfed70000},
{'6', 0x0000005b, 0x000000ca},
{'l', 0x00000000, 0x4214fe48},
{'l', 0xc1368ce4, 0x00000000},
{'l', 0x00000000, 0xc214fe48},
{'l', 0x41368ce4, 0x00000000},
{'@', 0x00000023, 0x0000732a},/*        #        x-advance: 115.164062 */
{'M', 0x428c8973, 0xc271e113},
{'l', 0xc19c3dda, 0x00000000},
{'4', 0x00b3ffd3, 0x0000009d},
{'6', 0xff4d002c, 0xfecfffb0},
{'l', 0xc0df5b10, 0x41ded19c},
{'l', 0x419cc74c, 0x00000000},
{'l', 0x40e180e0, 0xc1ded19c},
{'l', 0x412bcfe8, 0x00000000},
{'l', 0xc0dd3540, 0x41ded19c},
{'l', 0x41a78448, 0x00000000},
{'l', 0x00000000, 0x41255e7c},
{'l', 0xc1bc74d4, 0x00000000},
{'l', 0xc0b01b80, 0x41b35430},
{'l', 0x41aabd00, 0x00000000},
{'l', 0x00000000, 0x41244b9a},
{'l', 0xc1bfad88, 0x00000000},
{'l', 0xc0df5b10, 0x41de4829},
{'l', 0xc12bcfe4, 0x00000000},
{'l', 0x40dd3540, 0xc1de4829},
{'l', 0xc19d50c0, 0x00000000},
{'l', 0xc0dd3540, 0x41de4829},
{'l', 0xc12ce2ca, 0x00000000},
{'l', 0x40df5b10, 0xc1de4829},
{'l', 0xc1a920a5, 0x00000000},
{'l', 0x00000000, 0xc1244b9a},
{'l', 0x41bcfe48, 0x00000000},
{'l', 0x40b46718, 0xc1b35430},
{'l', 0xc1ace2cb, 0x00000000},
{'l', 0x00000000, 0xc1255e7c},
{'l', 0x41c1d353, 0x00000000},
{'l', 0x40db0f78, 0xc1ded19c},
{'l', 0x412df5b0, 0x00000000},
{'@', 0x00000024, 0x00005773},/*        $        x-advance: 87.449219 */
{'M', 0x4239c595, 0x41a19c59},
{'4', 0x0000ffcb, 0xff5f0000},
{'q', 0xc0e180d8, 0xbe09731d},
{0, 0xc16180dc, 0xbfce2cac},
{'9', 0xfff4ffc8, 0xffdcff8f},
{'l', 0x00000000, 0xc14149e1},
{'q', 0x40db0f76, 0x4089731e},
{0, 0x415d3543, 0x40d05278},
{'9', 0x00110038, 0x00110073},
{'l', 0x00000000, 0xc1f4d50c},
{'q', 0xc16d50c2, 0xc01aa180},
{0, 0xc1ace2cb, 0xc10301b8},
{'q', 0xc0d6c3de, 0xc0b8b2b0},
{0, 0xc0d6c3de, 0xc17d6c3c},
{'q', 0x00000000, 0xc12f0898},
{0, 0x40ea180e, 0xc189fc90},
{'9', 0xffce003a, 0xffc700a8},
{'4', 0xff820000, 0x00000035},
{'l', 0x00000000, 0x417920a8},
{'8', 0x0a600231, 0x165b082e},
{'l', 0x00000000, 0x413beb60},
{'8', 0xdea5ead4, 0xf2a0f4d2},
{'l', 0x00000000, 0x41e54302},
{'q', 0x4173c228, 0x401655f0},
{0, 0x41b35432, 0x41063a6c},
{'q', 0x40e5cc70, 0x40c149e0},
{0, 0x40e5cc70, 0x4184149e},
{'q', 0x00000000, 0x413579fc},
{0, 0xc0f4d510, 0x418f5b0f},
{'9', 0x0034ffc4, 0x003cff51},
{'6', 0x00a20000, 0xfdc1ffcb},
{'l', 0x00000000, 0xc1dc2258},
{'8', 0x23a106c2, 0x4be01ce0},
{'8', 0x471e2e00, 0x2561191e},
{'m', 0x40d6c3d8, 0x414e2cac},
{'l', 0x00000000, 0x41e87bb4},
{'8', 0xda66f744, 0xb322e322},
{'8', 0xb5dfd100, 0xd898e5e0},
{'@', 0x00000025, 0x0000829a},/*        %        x-advance: 130.601562 */
{'M', 0x42c7dda3, 0xc2306037},
{'8', 0x27b700d2, 0x6ee627e6},
{'8', 0x6e1a4500, 0x2749271a},
{'8', 0xd947002d, 0x921ad81a},
{'8', 0x92e6ba00, 0xd8b9d8e6},
{'m', 0x00000000, 0xc1086034},
{'q', 0x4129aa18, 0x00000000},
{0, 0x4186c3dc, 0x40ec3dd8},
{'q', 0x40c7bb50, 0x40ec3dd8},
{0, 0x40c7bb50, 0x419f768c},
{'q', 0x00000000, 0x4148ce2d},
{0, 0xc0c9e120, 0x419f768d},
{'q', 0xc0c7bb40, 0x40ea180d},
{0, 0xc1863a68, 0x40ea180d},
{'q', 0xc12bcfe8, 0x34000000},
{0, 0xc187d6c4, 0xc0ea180d},
{'q', 0xc0c7bb40, 0xc0ec3dda},
{0, 0xc0c7bb40, 0xc19f768d},
{'q', 0x00000000, 0xc149e114},
{0, 0x40c7bb40, 0xc19f768c},
{'9', 0xffc50032, 0xffc50087},
{'m', 0xc28a8603, 0xc2237d6c},
{'8', 0x28b700d2, 0x6de627e6},
{'8', 0x6e1a4600, 0x2749271a},
{'8', 0xd949002e, 0x921ad91a},
{'8', 0x93e6bb00, 0xd8b7d8e6},
{'m', 0x42726a86, 0xc1086038},
{'l', 0x412bcfe0, 0x00000000},
{'4', 0x033ffe0b, 0x0000ffab},
{'6', 0xfcc101f5, 0x0000fe1c},
{'q', 0x4129aa14, 0x00000000},
{0, 0x41874d50, 0x40ec3de0},
{'q', 0x40c9e110, 0x40ea1800},
{0, 0x40c9e110, 0x419eed18},
{'q', 0x00000000, 0x414af3f8},
{0, 0xc0c9e110, 0x41a00000},
{'q', 0xc0c7bb48, 0x40ea1808},
{0, 0xc1874d51, 0x40ea1808},
{'q', 0xc12abcfe, 0x00000000},
{0, 0xc1874d51, 0xc0ea1808},
{'q', 0xc0c59579, 0xc0ec3dd8},
{0, 0xc0c59579, 0xc1a00000},
{'q', 0x00000000, 0xc147bb48},
{0, 0x40c7bb47, 0xc19eed18},
{'q', 0x40c7bb46, 0xc0ec3de0},
{0, 0x4186c3de, 0xc0ec3de0},
{'@', 0x00000026, 0x00006b2e},/*        &        x-advance: 107.179688 */
{'M', 0x4205b0f7, 0xc257920a},
{'8', 0x56b92bd0, 0x5aea2aea},
{'q', 0x00000000, 0x411cc74e},
{0, 0x40e3a6a8, 0x41827845},
{'q', 0x40e3a6a8, 0x40d05278},
{0, 0x418ed19c, 0x40d05278},
{'8', 0xf05f0033, 0xcd53ef2c},
{'6', 0xfeddfee4, 0xffc4004b},
{'l', 0x42086037, 0x420b98e9},
{'q', 0x407d6c40, 0xc0bf2410},
{0, 0x40c59570, 0xc14c06e0},
{'9', 0xffca0011, 0xff8d0014},
{'l', 0x4147bb40, 0x00000000},
{'q', 0xbf4e2c80, 0x410dbeb8},
{0, 0xc0897310, 0x418c225c},
{'9', 0x0045ffe5, 0x0088ffb3},
{'4', 0x00990095, 0x0000ff79},
{'l', 0xc1198e98, 0xc11dda33},
{'q', 0xc0df5b10, 0x40bf2414},
{0, 0xc16a1810, 0x410ed19c},
{'q', 0xc0f4d510, 0x4038b2ae},
{0, 0xc1838b2c, 0x4038b2ae},
{'q', 0xc181655e, 0x34000000},
{0, 0xc1d38b2a, 0xc1131d35},
{'q', 0xc1244b99, 0xc114301c},
{0, 0xc1244b99, 0xc1bd87bb},
{'q', 0x00000000, 0xc109731e},
{0, 0x408fe482, 0xc180dbeb},
{'q', 0x408fe484, 0xc0f2af40},
{0, 0x4157d6c4, 0xc163a6a8},
{'8', 0xbdd9dfe7, 0xbef3dff3},
{'q', 0x00000000, 0xc12df5b0},
{0, 0x40ee63a4, 0xc18b98e8},
{'q', 0x40ee63a8, 0xc0d49e10},
{0, 0x419e63a6, 0xc0d49e10},
{'8', 0x0958002c, 0x1c5a092c},
{'l', 0x00000000, 0x41436fb0},
{'8', 0xdaa7e7d2, 0xf3b2f3d6},
{'8', 0x1ea500c8, 0x4cde1dde},
{'8', 0x370f1b00, 0x4d401b10},
{'@', 0x00000027, 0x000025c9},/*        '        x-advance: 37.785156 */
{'M', 0x41c50c07, 0xc2c86716},
{'l', 0x00000000, 0x4214fe48},
{'l', 0xc1368ce3, 0x00000000},
{'l', 0x00000000, 0xc214fe48},
{'l', 0x41368ce3, 0x00000000},
{'@', 0x00000028, 0x0000359f},/*        (        x-advance: 53.621094 */
{'M', 0x422a7844, 0xc2d09732},
{'q', 0xc10fe480, 0x4176fae0},
{0, 0xc155b0f6, 0x41f44b9c},
{'q', 0xc08b98e8, 0x41719c54},
{0, 0xc08b98e8, 0x41f4d50a},
{'q', 0x00000000, 0x41780dbe},
{0, 0x408b98e8, 0x41f5e7f2},
{'9', 0x00790023, 0x00f4006a},
{'l', 0xc12bcfe2, 0x00000000},
{'q', 0xc12112e6, 0xc17c5958},
{0, 0xc1719c5a, 0xc1f80dbf},
{'q', 0xc09eed18, 0xc173c224},
{0, 0xc09eed18, 0xc1f225cc},
{'q', 0x00000000, 0xc16f768c},
{0, 0x409eed18, 0xc1f112e6},
{'q', 0x409eed1c, 0xc172af40},
{0, 0x41719c5a, 0xc1f80dc0},
{'l', 0x412bcfe2, 0x00000000},
{'@', 0x00000029, 0x0000359f},/*        )        x-advance: 53.621094 */
{'M', 0x41301b7d, 0xc2d09732},
{'l', 0x412bcfe5, 0x00000000},
{'q', 0x412112e6, 0x417d6c40},
{0, 0x41708972, 0x41f80dc0},
{'q', 0x40a112e8, 0x4172af40},
{0, 0x40a112e8, 0x41f112e6},
{'q', 0x00000000, 0x41708974},
{0, 0xc0a112e8, 0x41f225cc},
{'9', 0x0079ffd9, 0x00f8ff88},
{'l', 0xc12bcfe5, 0x00000000},
{'q', 0x410ed19d, 0xc175e7f3},
{0, 0x41549e11, 0xc1f44b99},
{'q', 0x408dbeb4, 0xc173c226},
{0, 0x408dbeb4, 0xc1f5e7f2},
{'q', 0x00000000, 0xc1780dc0},
{0, 0xc08dbeb4, 0xc1f4d50a},
{'q', 0xc08b98e8, 0xc1719c58},
{0, 0xc1549e11, 0xc1f44b9c},
{'@', 0x0000002a, 0x000044b9},/*        *        x-advance: 68.722656 */
{'M', 0x42814302, 0xc2a761ef},
{'l', 0xc1c0c070, 0x41505278},
{'l', 0x41c0c070, 0x41516560},
{'l', 0xc07920b0, 0x40d27848},
{'l', 0xc1b46716, 0xc159fc94},
{'l', 0x00000000, 0x41ca6a88},
{'l', 0xc0f4d50c, 0x00000000},
{'l', 0x00000000, 0xc1ca6a88},
{'l', 0xc1b46716, 0x4159fc94},
{'l', 0xc07920a8, 0xc0d27848},
{'l', 0x41c0c06e, 0xc1516560},
{'l', 0xc1c0c06e, 0xc1505278},
{'l', 0x407920a4, 0xc0d49e10},
{'l', 0x41b46716, 0x4159fc90},
{'l', 0x36000000, 0xc1ca6a84},
{'l', 0x40f4d50c, 0x00000000},
{'l', 0x00000000, 0x41ca6a84},
{'l', 0x41b46716, 0xc159fc90},
{'l', 0x407920b0, 0x40d49e10},
{'@', 0x0000002b, 0x0000732a},/*        +        x-advance: 115.164062 */
{'M', 0x427ce2ca, 0xc2ac5957},
{'l', 0x00000000, 0x421587ba},
{'l', 0x421587bc, 0x00000000},
{'l', 0x00000000, 0x41368ce4},
{'l', 0xc21587bc, 0x00000000},
{'l', 0x00000000, 0x421587bb},
{'l', 0xc1346714, 0x00000000},
{'l', 0x00000000, 0xc21587bb},
{'l', 0xc21587bb, 0x00000000},
{'l', 0xb5800000, 0xc1368ce4},
{'l', 0x421587bb, 0x00000000},
{'l', 0x00000000, 0xc21587ba},
{'l', 0x41346714, 0x00000000},
{'@', 0x0000002c, 0x00002bb0},/*        ,        x-advance: 43.687500 */
{'M', 0x4180dbeb, 0xc1886037},
{'l', 0x416293c2, 0x00000000},
{'l', 0x00000000, 0x4138b2b0},
{'l', 0xc1301b7c, 0x41abcfe4},
{'l', 0xc10a8604, 0x00000000},
{'l', 0x40b01b7c, 0xc1abcfe4},
{'l', 0x00000000, 0xc138b2b0},
{'@', 0x0000002d, 0x00003198},/*        -        x-advance: 49.593750 */
{'M', 0x40d6c3dd, 0xc22c9e11},
{'l', 0x4210b2af, 0x00000000},
{'l', 0x00000000, 0x41301b7c},
{'l', 0xc210b2af, 0x00000000},
{'l', 0xb5c00000, 0xc1301b7c},
{'[', 0x0047002d, 0x00000508},
{'[', 0x004a002d, 0x000007a6},
{'[', 0x004f002d, 0x000003d3},
{'[', 0x0051002d, 0x00000508},
{'[', 0x006f002d, 0x0000028c},
{'@', 0x0000002e, 0x00002bb0},/*        .        x-advance: 43.687500 */
{'M', 0x416b2af4, 0xc1886037},
{'l', 0x416293c2, 0x00000000},
{'l', 0x00000000, 0x41886037},
{'l', 0xc16293c2, 0x00000000},
{'l', 0x00000000, 0xc1886037},
{'@', 0x0000002f, 0x00002e4f},/*        /        x-advance: 46.308594 */
{'M', 0x420b98e9, 0xc2c86716},
{'l', 0x41368ce4, 0x00000000},
{'l', 0xc20b98e9, 0x42e1e7f2},
{'l', 0xc1368ce4, 0xb5800000},
{'l', 0x420b98e9, 0xc2e1e7f2},
{'@', 0x00000030, 0x00005773},/*        0        x-advance: 87.449219 */
{'M', 0x422ec3dd, 0xc2b68ce3},
{'q', 0xc1278448, 0x00000000},
{0, 0xc17c5956, 0x41255e80},
{'q', 0xc0a7844c, 0x41244b98},
{0, 0xc0a7844c, 0x41f7844c},
{'q', 0x00000000, 0x41a4d50c},
{0, 0x40a7844c, 0x41f7844c},
{'q', 0x40a9aa1c, 0x41244b98},
{0, 0x417c5956, 0x41244b98},
{'q', 0x41289734, 0x00000000},
{0, 0x417c5958, 0xc1244b98},
{'q', 0x40a9aa18, 0xc1255e80},
{0, 0x40a9aa18, 0xc1f7844c},
{'q', 0x00000000, 0xc1a55e80},
{0, 0xc0a9aa18, 0xc1f7844c},
{'9', 0xffaeffd7, 0xffaeff82},
{'m', 0x00000000, 0xc12bcfe0},
{'q', 0x4186c3de, 0x00000000},
{0, 0x41cda33a, 0x4155b0f8},
{'q', 0x410ed198, 0x41549e10},
{0, 0x410ed198, 0x421aa180},
{'q', 0x00000000, 0x41ca6a86},
{0, 0xc10ed198, 0x421aa181},
{'q', 0xc10dbeb8, 0x41549e11},
{0, 0xc1cda33a, 0x41549e11},
{'q', 0xc186c3dd, 0xb4c00000},
{0, 0xc1ce2cab, 0xc1549e11},
{'q', 0xc10dbeb5, 0xc155b0f8},
{0, 0xc10dbeb5, 0xc21aa181},
{'q', 0x00000000, 0xc1caf3f8},
{0, 0x410dbeb5, 0xc21aa180},
{'q', 0x410ed19c, 0xc155b0f8},
{0, 0x41ce2cab, 0xc155b0f8},
{'@', 0x00000031, 0x00005773},/*        1        x-advance: 87.449219 */
{'M', 0x41886037, 0xc1368ce3},
{'l', 0x41b12e63, 0x00000000},
{'l', 0x00000000, 0xc298e2cb},
{'l', 0xc1c0c06e, 0x409aa180},
{'l', 0x35800000, 0xc1459578},
{'l', 0x41bfad88, 0xc09aa180},
{'l', 0x4158e9a8, 0x00000000},
{'l', 0x00000000, 0x42b1957a},
{'l', 0x41b12e64, 0xb6400000},
{'l', 0x00000000, 0x41368ce3},
{'l', 0xc266df5a, 0x00000000},
{'l', 0xb6000000, 0xc1368ce3},
{'@', 0x00000032, 0x00005773},/*        2        x-advance: 87.449219 */
{'M', 0x41d301b8, 0xc1368ce3},
{'l', 0x423d4302, 0x00000000},
{'4', 0x005b0000, 0x0000fe04},
{'l', 0xb6000000, 0xc1368ce3},
{'q', 0x40f6fad8, 0xc0ff920a},
{0, 0x41a80dbe, 0xc1ab4670},
{'q', 0x4155b0f6, 0xc157d6c4},
{0, 0x41863a6b, 0xc18b0f76},
{'8', 0x9e48c634, 0xb114d814},
{'q', 0x00000000, 0xc0ff9210},
{0, 0xc0b46718, 0xc1505278},
{'q', 0xc0b24148, 0xc0a112f0},
{0, 0xc1690528, 0xc0a112f0},
{'q', 0xc0cc06e0, 0x00000000},
{0, 0xc157d6c2, 0x400dbec0},
{'9', 0x0011ffc8, 0x0035ff88},
{'l', 0x00000000, 0xc15b0f78},
{'q', 0x410301b8, 0xc0527840},
{0, 0x4174d50c, 0xc09eed20},
{'q', 0x40e3a6a8, 0xbfd6c3c0},
{0, 0x41505278, 0xbfd6c3c0},
{'q', 0x417920a4, 0x00000000},
{0, 0x41c6a860, 0x40f920a0},
{'q', 0x41143018, 0x40f920b0},
{0, 0x41143018, 0x41a67168},
{'8', 0x5dee3100, 0x68bd2cee},
{'q', 0xbfd6c3e0, 0x3ff920c0},
{0, 0xc12abd00, 0x41346718},
{'q', 0xc10fe480, 0x4114301c},
{0, 0xc1caf3f8, 0x41cfc904},
{'@', 0x00000033, 0x00005773},/*        3        x-advance: 87.449219 */
{'M', 0x425f1656, 0xc2581b7d},
{'q', 0x411bb468, 0x40052780},
{0, 0x4172af40, 0x410a8604},
{'q', 0x40b01b80, 0x40d27840},
{0, 0x40b01b80, 0x4181eed1},
{'q', 0x00000000, 0x416d50c0},
{0, 0xc12338b8, 0x41b79fc8},
{'q', 0xc12338b0, 0x4101eed3},
{0, 0xc1e7f240, 0x4101eed3},
{'8', 0xf79800ce, 0xe292f6cb},
{'l', 0x00000000, 0xc151655e},
{'q', 0x40b46716, 0x40527844},
{0, 0x4145957b, 0x409eed18},
{'q', 0x40d6c3dc, 0x3fd6c3e0},
{0, 0x41606df6, 0x3fd6c3e0},
{'q', 0x414c06dc, 0x00000000},
{0, 0x419b2af2, 0xc0a112e6},
{'q', 0x40d6c3e0, 0xc0a112e6},
{0, 0x40d6c3e0, 0xc16a180d},
{'q', 0x00000000, 0xc10dbeb6},
{0, 0xc0c7bb48, 0xc15d3542},
{'9', 0xffd8ffcf, 0xffd8ff77},
{'4', 0x0000ffa3, 0xffa70000},
{'l', 0x41436fae, 0x00000000},
{'q', 0x41200000, 0x00000000},
{0, 0x4174d50c, 0xc07d6c40},
{'8', 0xa42ae02a, 0xa2d4c300},
{'q', 0xc0adf5b0, 0xc0852790},
{0, 0xc17a338c, 0xc0852790},
{'q', 0xc0b24148, 0x00000000},
{0, 0xc13f2414, 0x3f9aa180},
{'9', 0x0009ffcd, 0x001eff90},
{'l', 0x00000000, 0xc14149e0},
{'q', 0x40f6fad8, 0xc0097320},
{0, 0x4166df5a, 0xc04e2cc0},
{'q', 0x40d8e9ac, 0xbf897300},
{0, 0x414c06de, 0xbf897300},
{'q', 0x4176fad8, 0x00000000},
{0, 0x41c36fae, 0x40e180e0},
{'q', 0x410fe480, 0x40df5b10},
{0, 0x410fe480, 0x419768cc},
{'q', 0x00000000, 0x41052788},
{0, 0xc0987bb0, 0x416180dc},
{'q', 0xc0987bb0, 0x40b68ce8},
{0, 0xc158e9a8, 0x40fd6c40},
{'@', 0x00000034, 0x00005773},/*        4        x-advance: 87.449219 */
{'M', 0x424fc905, 0xc2b0c74d},
{'4', 0x01abfeef, 0x00000111},
{'6', 0xfe550000, 0xffa2ffe4},
{'l', 0x41886038, 0x00000000},
{'l', 0x00000000, 0x42829aa2},
{'l', 0x4164b990, 0xb6800000},
{'l', 0x00000000, 0x41346714},
{'l', 0xc164b990, 0x00000000},
{'l', 0x00000000, 0x41bcfe48},
{'l', 0xc157d6c4, 0x00000000},
{'l', 0x00000000, 0xc1bcfe48},
{'l', 0xc234f089, 0x00000000},
{'l', 0xb5c00000, 0xc151655c},
{'l', 0x4226b61e, 0xc27df5b1},
{'@', 0x00000035, 0x00005773},/*        5        x-advance: 87.449219 */
{'M', 0x416d50c0, 0xc2c86716},
{'l', 0x4254e2ca, 0x00000000},
{'4', 0x005b0000, 0x0000feba},
{'l', 0x00000000, 0x41c48294},
{'8', 0xf52ff817, 0xfc2ffc17},
{'q', 0x41863a6a, 0x00000000},
{0, 0x41d49e10, 0x41131d34},
{'q', 0x411cc750, 0x41131d34},
{0, 0x411cc750, 0x41c731d2},
{'q', 0x00000000, 0x4181655f},
{0, 0xc12112e8, 0x41c957a0},
{'q', 0xc12112e4, 0x410ed19d},
{0, 0xc1e31d34, 0x410ed19d},
{'8', 0xf89900ce, 0xe795f8cc},
{'l', 0x00000000, 0xc159fc90},
{'8', 0x27631a30, 0x0c6c0c33},
{'q', 0x4139c598, 0x00000000},
{0, 0x41931d36, 0xc0c36fae},
{'q', 0x40d8e9a8, 0xc0c36fae},
{0, 0x40d8e9a8, 0xc1849e12},
{'q', 0x00000000, 0xc1278448},
{0, 0xc0d8e9a8, 0xc1849e10},
{'q', 0xc0d8e9a8, 0xc0c36fb0},
{0, 0xc1931d36, 0xc0c36fb0},
{'8', 0x09aa00d5, 0x1ea809d6},
{'l', 0x00000000, 0xc249579f},
{'@', 0x00000036, 0x00005773},/*        6        x-advance: 87.449219 */
{'M', 0x423579fc, 0xc25e036f},
{'q', 0xc1120a4c, 0x00000000},
{0, 0xc167f240, 0x40c7bb40},
{'q', 0xc0a9aa18, 0x40c7bb48},
{0, 0xc0a9aa18, 0x4188e9aa},
{'q', 0x00000000, 0x412ce2cc},
{0, 0x40a9aa18, 0x4188e9aa},
{'q', 0x40abcfe8, 0x40c7bb48},
{0, 0x4167f240, 0x40c7bb48},
{'q', 0x41120a50, 0x00000000},
{0, 0x4166df5c, 0xc0c7bb46},
{'q', 0x40abcfe8, 0xc0c9e112},
{0, 0x40abcfe8, 0xc188e9aa},
{'q', 0x00000000, 0xc12df5b0},
{0, 0xc0abcfe8, 0xc188e9aa},
{'9', 0xffcfffd6, 0xffcfff8d},
{'m', 0x41d74d50, 0xc229eed1},
{'l', 0x00000000, 0x41459578},
{'8', 0xe3aeedd8, 0xf6aef6d7},
{'q', 0xc156c3e0, 0x00000000},
{0, 0xc1a44b99, 0x4110f768},
{'q', 0xc0e180dc, 0x4110f768},
{0, 0xc100dbec, 0x41db0f78},
{'8', 0xb94fd21f, 0xe769e72f},
{'q', 0x41719c58, 0x00000000},
{0, 0x41be9aa2, 0x41131d34},
{'q', 0x410cabd0, 0x41120a50},
{0, 0x410cabd0, 0x41c731d2},
{'q', 0x00000000, 0x4176fada},
{0, 0xc1120a50, 0x41c61eee},
{'q', 0xc1120a50, 0x41154301},
{0, 0xc1c25cc8, 0x41154301},
{'q', 0xc18b0f76, 0xb4c00000},
{0, 0xc1d49e10, 0xc1549e11},
{'q', 0xc1131d36, 0xc155b0f8},
{0, 0xc1131d36, 0xc21aa181},
{'q', 0x00000000, 0xc1be112c},
{0, 0x41346716, 0xc21768ce},
{'q', 0x41346718, 0xc16293c0},
{0, 0x41f225cc, 0xc16293c0},
{'8', 0x08520028, 0x18560829},
{'@', 0x00000037, 0x00005773},/*        7        x-advance: 87.449219 */
{'M', 0x41346716, 0xc2c86716},
{'l', 0x4280dbeb, 0x00000000},
{'l', 0x00000000, 0x40b8b2b0},
{'l', 0xc21180dc, 0x42bcdbeb},
{'l', 0xc16293c2, 0x00000000},
{'l', 0x4208e9aa, 0xc2b1957a},
{'l', 0xc2407bb4, 0x00000000},
{'l', 0xb6000000, 0xc1368ce0},
{'@', 0x00000038, 0x00005773},/*        8        x-advance: 87.449219 */
{'M', 0x422ec3dd, 0xc23e55e8},
{'q', 0xc11aa180, 0x00000000},
{0, 0xc173c224, 0x40a55e80},
{'q', 0xc0b01b7c, 0x40a55e80},
{0, 0xc0b01b7c, 0x4163a6a8},
{'q', 0x00000000, 0x4110f76a},
{0, 0x40b01b7c, 0x4163a6a9},
{'q', 0x40b24148, 0x40a55e7e},
{0, 0x4173c224, 0x40a55e7e},
{'q', 0x411aa184, 0x00000000},
{0, 0x4173c228, 0xc0a55e7e},
{'q', 0x40b24148, 0xc0a7844a},
{0, 0x40b24148, 0xc163a6a9},
{'q', 0x00000000, 0xc110f768},
{0, 0xc0b24148, 0xc163a6a8},
{'9', 0xffd7ffd4, 0xffd7ff87},
{'m', 0xc158e9a8, 0xc0b8b2b0},
{'q', 0xc10b98ea, 0xc0097310},
{0, 0xc159fc90, 0xc101eed0},
{'q', 0xc09aa182, 0xc0bf2410},
{0, 0xc09aa182, 0xc1690528},
{'q', 0x00000000, 0xc14036f8},
{0, 0x41086037, 0xc197f240},
{'q', 0x4109731e, 0xc0df5b10},
{0, 0x41bbeb61, 0xc0df5b10},
{'q', 0x416f7690, 0x00000000},
{0, 0x41bbeb62, 0x40df5b10},
{'q', 0x41086038, 0x40df5b10},
{0, 0x41086038, 0x4197f240},
{'q', 0x00000000, 0x41097320},
{0, 0xc09cc750, 0x41690528},
{'q', 0xc09aa180, 0x40bf2418},
{0, 0xc157d6c4, 0x4101eed0},
{'q', 0x411cc74c, 0x40120a50},
{0, 0x4173c224, 0x410ed1a0},
{'q', 0x40b01b80, 0x40d49e10},
{0, 0x40b01b80, 0x4181eed0},
{'q', 0x00000000, 0x41690528},
{0, 0xc10ed198, 0x41b2cabd},
{'q', 0xc10dbeb8, 0x40f920a5},
{0, 0xc1cb7d6e, 0x40f920a5},
{'q', 0xc1849e10, 0x34000000},
{0, 0xc1cc06de, 0xc0f920a4},
{'q', 0xc10dbeb7, 0xc0f920a5},
{0, 0xc10dbeb7, 0xc1b2cabd},
{'q', 0x00000000, 0xc1198e98},
{0, 0x40b01b7e, 0xc181eed0},
{'9', 0xffcb002c, 0xffb9007a},
{'m', 0xc09eed1c, 0xc1ab4670},
{'8', 0x61263e00, 0x226d2227},
{'8', 0xde6c0045, 0x9f27de27},
{'8', 0x9fd9c200, 0xde94ded9},
{'8', 0x229300ba, 0x61da22da},
{'@', 0x00000039, 0x00005773},/*        9        x-advance: 87.449219 */
{'M', 0x41719c59, 0xc0052784},
{'l', 0x00000000, 0xc145957a},
{'8', 0x1d521328, 0x0a520a29},
{'q', 0x4156c3dc, 0x00000000},
{0, 0x41a3c226, 0xc10fe482},
{'q', 0x40e3a6a8, 0xc110f768},
{0, 0x4101eed4, 0xc1db98ea},
{'8', 0x46b22ee1, 0x189718d1},
{'q', 0xc1708974, 0x00000000},
{0, 0xc1be9aa2, 0xc110f768},
{'q', 0xc10b98e9, 0xc1120a50},
{0, 0xc10b98e9, 0xc1c731d4},
{'q', 0x00000000, 0xc176fad8},
{0, 0x41120a4f, 0xc1c61eec},
{'q', 0x41120a4e, 0xc1154300},
{0, 0x41c25cc7, 0xc1154300},
{'q', 0x418b0f76, 0x00000000},
{0, 0x41d4149c, 0x4155b0f8},
{'q', 0x41131d38, 0x41549e10},
{0, 0x41131d38, 0x421aa180},
{'q', 0x00000000, 0x41bd87bc},
{0, 0xc1346718, 0x421768ce},
{'q', 0xc133542c, 0x416180dd},
{0, 0xc1f19c58, 0x416180dd},
{'8', 0xf8ae00d8, 0xe8aaf8d7},
{'m', 0x41d7d6c4, 0xc229eed2},
{'q', 0x41120a50, 0x00000000},
{0, 0x4166df5c, 0xc0c7bb40},
{'q', 0x40abcfe0, 0xc0c7bb48},
{0, 0x40abcfe0, 0xc188e9ac},
{'q', 0x00000000, 0xc12ce2c8},
{0, 0xc0abcfe0, 0xc1886034},
{'q', 0xc0a9aa18, 0xc0c9e120},
{0, 0xc166df5c, 0xc0c9e120},
{'q', 0xc1120a50, 0x00000000},
{0, 0xc167f240, 0x40c9e120},
{'q', 0xc0a9aa18, 0x40c7bb40},
{0, 0xc0a9aa18, 0x41886034},
{'q', 0x00000000, 0x412df5b4},
{0, 0x40a9aa18, 0x4188e9ac},
{'q', 0x40abcfe0, 0x40c7bb40},
{0, 0x4167f240, 0x40c7bb40},
{'@', 0x0000003a, 0x00002e4f},/*        :        x-advance: 46.308594 */
{'M', 0x4180dbeb, 0xc1886037},
{'l', 0x416293c2, 0x00000000},
{'4', 0x00880000, 0x0000ff8f},
{'6', 0xff780000, 0xfe500000},
{'l', 0x416293c2, 0x00000000},
{'l', 0x00000000, 0x41886036},
{'l', 0xc16293c2, 0x00000000},
{'l', 0x00000000, 0xc1886036},
{'@', 0x0000003b, 0x00002e4f},/*        ;        x-advance: 46.308594 */
{'M', 0x4180dbeb, 0xc28e25cc},
{'l', 0x416293c2, 0x00000000},
{'4', 0x00880000, 0x0000ff8f},
{'6', 0xff780000, 0x01b00000},
{'l', 0x416293c2, 0x36000000},
{'l', 0x00000000, 0x4138b2b0},
{'l', 0xc1301b7c, 0x41abcfe4},
{'l', 0xc10a8604, 0x00000000},
{'l', 0x40b01b7c, 0xc1abcfe4},
{'l', 0x00000000, 0xc138b2b0},
{'@', 0x0000003c, 0x0000732a},/*        <        x-advance: 115.164062 */
{'M', 0x42c93543, 0xc2874d51},
{'l', 0xc28a8604, 0x41c50c08},
{'l', 0x428a8604, 0x41c3f921},
{'l', 0x00000000, 0x41436fac},
{'l', 0xc2ac149e, 0xc1f9aa17},
{'l', 0xb5800000, 0xc132414c},
{'l', 0x42ac149e, 0xc1f9aa16},
{'l', 0x00000000, 0x41436fa8},
{'@', 0x0000003d, 0x0000732a},/*        =        x-advance: 115.164062 */
{'M', 0x41690527, 0xc279aa18},
{'l', 0x42ac149e, 0x00000000},
{'4', 0x005a0000, 0x0000fd50},
{'6', 0xffa60000, 0x00db0000},
{'l', 0x42ac149e, 0x00000000},
{'l', 0x00000000, 0x41368ce4},
{'l', 0xc2ac149e, 0x00000000},
{'l', 0xb5800000, 0xc1368ce4},
{'@', 0x0000003e, 0x0000732a},/*        >        x-advance: 115.164062 */
{'M', 0x41690527, 0xc2874d51},
{'l', 0x00000000, 0xc1436fa8},
{'l', 0x42ac149e, 0x41f9aa16},
{'l', 0x00000000, 0x4132414c},
{'l', 0xc2ac149e, 0x41f9aa17},
{'l', 0xb5800000, 0xc1436fac},
{'l', 0x428a414a, 0xc1c3f921},
{'l', 0xc28a414a, 0xc1c50c08},
{'@', 0x0000003f, 0x000048f3},/*        ?        x-advance: 72.949219 */
{'M', 0x41d1eed1, 0xc1886037},
{'l', 0x4159fc92, 0x00000000},
{'4', 0x00880000, 0x0000ff94},
{'6', 0xff780000, 0xffb20069},
{'4', 0x0000ff9a, 0xffae0000},
{'8', 0xa70fca00, 0xaf3fde0f},
{'l', 0x40c149e0, 0xc0bf2418},
{'8', 0xcb2ce41e, 0xcd0de70d},
{'8', 0xb3ddd100, 0xe3a4e3de},
{'8', 0x12a600d6, 0x369d12d1},
{'l', 0x00000000, 0xc149e110},
{'8', 0xd366e232, 0xf16bf134},
{'q', 0x41459578, 0x00000000},
{0, 0x419e63a6, 0x40d05270},
{'q', 0x40f08970, 0x40d05280},
{0, 0x40f08970, 0x41897320},
{'8', 0x4ded2800, 0x52bd24ed},
{'l', 0xc0bcfe48, 0x40b8b2b0},
{'8', 0x27dd19e7, 0x1bf20df6},
{'8', 0x1bfc0bfd, 0x2cff10ff},
{'l', 0x00000000, 0x410414a0},
{'@', 0x00000040, 0x00008973},/*        @        x-advance: 137.449219 */
{'M', 0x424c9052, 0xc210293c},
{'q', 0x00000000, 0x41198e9a},
{0, 0x40987bb8, 0x41719c5a},
{'8', 0x2b682b26, 0xd4670042},
{'q', 0x40987bc0, 0xc0b01b80},
{0, 0x40987bc0, 0xc1708974},
{'q', 0x00000000, 0xc11655e8},
{0, 0xc09aa180, 0xc16e63a4},
{'8', 0xd498d4da, 0x2c9900c0},
{'9', 0x002cffda, 0x0077ffda},
{'m', 0x42124f08, 0x41a08973},
{'8', 0x3db629e0, 0x13a013d7},
{'q', 0xc138b2b0, 0x00000000},
{0, 0xc19655e8, 0xc1052784},
{'q', 0xc0e5cc78, 0xc1063a6a},
{0, 0xc0e5cc78, 0xc1ae7f24},
{'q', 0x00000000, 0xc156c3dc},
{0, 0x40e7f240, 0xc1ae7f24},
{'q', 0x40e7f240, 0xc1063a68},
{0, 0x4195cc76, 0xc1063a68},
{'8', 0x14610037, 0x3c491329},
{'4', 0xffba0000, 0x0000004c},
{'l', 0x00000000, 0x4245957a},
{'q', 0x411cc748, 0xbfbcfe40},
{0, 0x4174d508, 0xc10ed19a},
{'q', 0x40b24150, 0xc0f08974},
{0, 0x40b24150, 0xc19b2af3},
{'8', 0x95efc700, 0xa3cdcef0},
{'q', 0xc0df5b10, 0xc10cabd0},
{0, 0xc1886038, 0xc156c3e0},
{'q', 0xc1200000, 0xc09655e0},
{0, 0xc1ae7f24, 0xc09655e0},
{'q', 0xc104149c, 0x00000000},
{0, 0xc17d6c3c, 0x400dbea0},
{'q', 0xc0f2af40, 0x40097320},
{0, 0xc1606df4, 0x40ce2cb0},
{'q', 0xc1289734, 0x40db0f80},
{0, 0xc184149f, 0x418fe482},
{'q', 0xc0bcfe48, 0x41312e64},
{0, 0xc0bcfe48, 0x41c036fc},
{'q', 0x00000000, 0x412abcfc},
{0, 0x4074d510, 0x419fffff},
{'q', 0x407920a0, 0x41154302},
{0, 0x4133542e, 0x41838b2b},
{'q', 0x40e180e0, 0x40df5b0f},
{0, 0x41827846, 0x412abcfe},
{'q', 0x41143018, 0x4067f240},
{0, 0x419e63a6, 0x4067f240},
{'q', 0x410a8600, 0x00000000},
{0, 0x4187d6c4, 0xc038b2ac},
{'9', 0xffe90043, 0xffbd007a},
{'l', 0x40c149e0, 0x40ee63a6},
{'q', 0xc1063a68, 0x40d0527a},
{0, 0xc19293c0, 0x41200001},
{'q', 0xc11dda38, 0x405b0f70},
{0, 0xc1a08974, 0x405b0f70},
{'q', 0xc146a860, 0x00000000},
{0, 0xc1bb61ee, 0xc08b98e8},
{'q', 0xc1301b80, 0xc08dbeb6},
{0, 0xc19cc74e, 0xc14d19c6},
{'q', 0xc109731e, 0xc1063a6a},
{0, 0xc151655e, 0xc19b2af4},
{'q', 0xc08fe482, 0xc1312e62},
{0, 0xc08fe482, 0xc1be112d},
{'q', 0x00000000, 0xc1436fb0},
{0, 0x40920a4e, 0xc1ba4f08},
{'q', 0x40920a50, 0xc1312e64},
{0, 0x41505278, 0xc19bb466},
{'q', 0x410a8604, 0xc1086038},
{0, 0x41a00000, 0xc1505278},
{'q', 0x413579fc, 0xc0920a50},
{0, 0x41c036fc, 0xc0920a50},
{'q', 0x4163a6a8, 0x00000000},
{0, 0x41d301b4, 0x40bad870},
{'q', 0x41436fb0, 0x40bad880},
{0, 0x41a3c228, 0x41849e14},
{'q', 0x40a112e0, 0x40d27840},
{0, 0x40f4d510, 0x4164b98c},
{'q', 0x402bcfe0, 0x40f6fad8},
{0, 0x402bcfe0, 0x417f9208},
{'q', 0x00000000, 0x418d3543},
{0, 0xc12abd00, 0x41ded19d},
{'q', 0xc12abd00, 0x412338b2},
{0, 0xc1ebb468, 0x4129aa17},
{'l', 0x00000000, 0xc1255e7f},
{'@', 0x00000041, 0x00005e06},/*        A        x-advance: 94.023438 */
{'M', 0x423beb62, 0xc2adb0f7},
{'4', 0x018eff6d, 0x00000126},
{'6', 0xfe72ff6d, 0xff96ffc3},
{'l', 0x4175e7f4, 0x00000000},
{'l', 0x4218c06d, 0x42c86716},
{'l', 0xc16180d8, 0x00000000},
{'l', 0xc1120a50, 0xc1cda338},
{'l', 0xc234abd0, 0x00000000},
{'l', 0xc1120a4e, 0x41cda338},
{'l', 0xc164b98e, 0x00000000},
{'l', 0x42190527, 0xc2c86716},
{'[', 0x00410041, 0x000003d3},
{'@', 0x00000042, 0x00005e4b},/*        B        x-advance: 94.292969 */
{'M', 0x41d86037, 0xc23f68ce},
{'4', 0x01250000, 0x000000ad},
{'q', 0x412f0894, 0x00000000},
{0, 0x4181655c, 0xc08fe482},
{'8', 0x912adc2a, 0x92d6b500},
{'9', 0xffddffd7, 0xffddff7f},
{'6', 0x0000ff53, 0xfeb70000},
{'4', 0x00f10000, 0x000000a0},
{'q', 0x411eed18, 0x00000000},
{0, 0x416c3dd8, 0xc06c3de0},
{'8', 0xa527e227, 0xa6d9c400},
{'9', 0xffe2ffda, 0xffe2ff8a},
{'6', 0x0000ff60, 0xffa7ff94},
{'l', 0x420a8603, 0x00000000},
{'q', 0x41780dc0, 0x00000000},
{0, 0x41bf2414, 0x40ce2cb0},
{'q', 0x41063a68, 0x40ce2ca0},
{0, 0x41063a68, 0x419293c0},
{'q', 0x00000000, 0x41131d38},
{0, 0xc0897310, 0x416a1810},
{'q', 0xc0897320, 0x40adf5b0},
{0, 0xc149e114, 0x40d8e9a8},
{'q', 0x411ffffc, 0x40097320},
{0, 0x41780dbc, 0x410fe480},
{'q', 0x40b24150, 0x40d8e9b0},
{0, 0x40b24150, 0x4187d6c5},
{'q', 0x00000000, 0x4156c3dd},
{0, 0xc1120a50, 0x41a5e7f2},
{'9', 0x003affb7, 0x003aff31},
{'l', 0xc20fe482, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x00000043, 0x00005ff9},/*        C        x-advance: 95.972656 */
{'M', 0x42b10c07, 0xc2b8f769},
{'l', 0x00000000, 0x4164b990},
{'q', 0xc0db0f80, 0xc0cc06e0},
{0, 0xc16a1810, 0xc1187bb0},
{'q', 0xc0f6fae0, 0xc049e120},
{0, 0xc1838b2c, 0xc049e120},
{'q', 0xc189731c, 0x00000000},
{0, 0xc1d27843, 0x41289730},
{'q', 0xc1120a50, 0x41278450},
{0, 0xc1120a50, 0x41f2af40},
{'q', 0x00000000, 0x419e63a7},
{0, 0x41120a50, 0x41f2af40},
{'q', 0x41120a4e, 0x4127844b},
{0, 0x41d27843, 0x4127844b},
{'q', 0x410b98e8, 0x00000000},
{0, 0x41838b2c, 0xc049e114},
{'9', 0xffe7003e, 0xffb40075},
{'l', 0x00000000, 0x416293c2},
{'q', 0xc0e3a6b0, 0x409aa180},
{0, 0xc1719c60, 0x40e7f241},
{'q', 0xc0fd6c30, 0x401aa180},
{0, 0xc1863a68, 0x401aa180},
{'q', 0xc1b60370, 0x34000000},
{0, 0xc20f5b10, 0xc15e4828},
{'q', 0xc151655c, 0xc15f5b10},
{0, 0xc151655c, 0xc21836fb},
{'q', 0x00000000, 0xc1c149e0},
{0, 0x4151655e, 0xc21836fa},
{'q', 0x4151655e, 0xc15f5b10},
{0, 0x420f5b10, 0xc15f5b10},
{'q', 0x410fe480, 0x00000000},
{0, 0x41874d50, 0x401aa180},
{'q', 0x40ff9210, 0x401655e0},
{0, 0x416f7690, 0x40e3a6a0},
{'@', 0x00000044, 0x000069d6},/*        D        x-advance: 105.835938 */
{'M', 0x41d86037, 0xc2b21eed},
{'4', 0x026f0000, 0x00000083},
{'q', 0x41a5e7f2, 0x00000000},
{0, 0x41f2af3e, 0xc11655e8},
{'q', 0x411aa180, 0xc11655e8},
{0, 0x411aa180, 0xc1ed50bf},
{'q', 0x00000000, 0xc1a112e8},
{0, 0xc11aa180, 0xc1ebb468},
{'9', 0xffb5ffb4, 0xffb5ff0e},
{'6', 0x0000ff7d, 0xffa7ff94},
{'l', 0x41ded19c, 0x00000000},
{'q', 0x41e90526, 0x00000000},
{0, 0x422b01b7, 0x41425cc8},
{'q', 0x4159fc90, 0x414149e0},
{0, 0x4159fc90, 0x421768ce},
{'q', 0x00000000, 0x41cf3f91},
{0, 0xc15b0f78, 0x421836fa},
{'9', 0x0061ff93, 0x0061feab},
{'l', 0xc1ded19c, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x00000045, 0x000056d8},/*        E        x-advance: 86.843750 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x427d6c3d, 0x00000000},
{'l', 0x00000000, 0x41368ce0},
{'l', 0xc24731d2, 0x00000000},
{'l', 0xb6000000, 0x41ed50c2},
{'l', 0x423edf5a, 0x00000000},
{'l', 0x00000000, 0x41368ce0},
{'l', 0xc23edf5a, 0x00000000},
{'l', 0xb6000000, 0x42113c22},
{'l', 0x424c06de, 0x35800000},
{'l', 0x00000000, 0x41368ce3},
{'l', 0xc28120a4, 0x00000000},
{'l', 0xb6800000, 0xc2c86716},
{'@', 0x00000046, 0x00004f0f},/*        F        x-advance: 79.058594 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x426655e7, 0x00000000},
{'l', 0x00000000, 0x41368ce0},
{'l', 0xc2301b7c, 0x00000000},
{'l', 0xb6000000, 0x41ec3dda},
{'l', 0x421eed18, 0x00000000},
{'l', 0x00000000, 0x41368ce4},
{'l', 0xc21eed18, 0x00000000},
{'l', 0xb6000000, 0x423f68ce},
{'l', 0xc158e9aa, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x00000047, 0x00006a82},/*        G        x-advance: 106.507812 */
{'M', 0x42a39fc9, 0xc164b98e},
{'l', 0x00000000, 0xc1d74d51},
{'l', 0xc1b12e64, 0x00000000},
{'4', 0xffa70000, 0x0000011c},
{'l', 0x00000000, 0x422c149e},
{'q', 0xc0fb4670, 0x40b24146},
{0, 0xc18a8600, 0x41074d4f},
{'q', 0xc11768d0, 0x40346716},
{0, 0xc1a19c5a, 0x40346716},
{'q', 0xc1bbeb62, 0x34000000},
{0, 0xc2131d36, 0xc15b0f76},
{'q', 0xc1538b28, 0xc15c225c},
{0, 0xc1538b28, 0xc2190528},
{'q', 0x00000000, 0xc1c48294},
{0, 0x41538b2a, 0xc2190526},
{'q', 0x41549e12, 0xc15c2260},
{0, 0x42131d36, 0xc15c2260},
{'q', 0x411cc748, 0x00000000},
{0, 0x4194b98c, 0x401aa180},
{'9', 0x00130046, 0x00380082},
{'l', 0x00000000, 0x4166df60},
{'q', 0xc0f08970, 0xc0cc06e0},
{0, 0xc17f9208, 0xc1198e98},
{'q', 0xc1074d50, 0xc04e2cc0},
{0, 0xc18e482a, 0xc04e2cc0},
{'q', 0xc1931d34, 0x00000000},
{0, 0xc1dd3542, 0x41244b98},
{'q', 0xc1131d36, 0x41244b98},
{0, 0xc1131d36, 0x41f4d50c},
{'q', 0x00000000, 0x41a225cd},
{0, 0x41131d36, 0x41f44b99},
{'q', 0x4114301c, 0x41244b99},
{0, 0x41dd3542, 0x41244b99},
{'8', 0xf7660039, 0xe151f62d},
{'@', 0x00000048, 0x0000675b},/*        H        x-advance: 103.355469 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x4158e9aa, 0x00000000},
{'l', 0x00000000, 0x42244b99},
{'l', 0x42450c06, 0x00000000},
{'l', 0x00000000, 0xc2244b99},
{'l', 0x4158e9a8, 0x00000000},
{'l', 0x00000000, 0x42c86716},
{'l', 0xc158e9a8, 0x00000000},
{'l', 0x00000000, 0xc23edf5b},
{'l', 0xc2450c06, 0x00000000},
{'l', 0xb6000000, 0x423edf5b},
{'l', 0xc158e9aa, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x00000049, 0x00002889},/*        I        x-advance: 40.535156 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x4158e9aa, 0x00000000},
{'l', 0x00000000, 0x42c86716},
{'l', 0xc158e9aa, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x0000004a, 0x00002889},/*        J        x-advance: 40.535156 */
{'M', 0x4157d6c4, 0xc2c86716},
{'4', 0x0000006c, 0x02e90000},
{'q', 0x00000000, 0x4190f769},
{0, 0xc0dd3544, 0x41d27845},
{'9', 0x0041ffca, 0x0041ff50},
{'4', 0x0000ffd7, 0xffa50000},
{'l', 0x40874d50, 0x00000000},
{'q', 0x410fe482, 0x00000000},
{0, 0x414af3f9, 0xc0a112e6},
{'q', 0x406c3ddc, 0xc0a112e5},
{0, 0x406c3ddc, 0xc1906df5},
{'l', 0x00000000, 0xc2ba7165},
{'@', 0x0000004b, 0x00005a22},/*        K        x-advance: 90.132812 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x4158e9aa, 0x00000000},
{'l', 0x00000000, 0x4229655e},
{'l', 0x4233dda2, 0xc229655e},
{'l', 0x418b98ec, 0x00000000},
{'l', 0xc246ed1a, 0x423ad87b},
{'l', 0x42552784, 0x4255f5b1},
{'l', 0xc18ed19c, 0x00000000},
{'l', 0xc2407bb4, 0xc2410527},
{'l', 0xb6000000, 0x42410527},
{'l', 0xc158e9aa, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x0000004c, 0x00004c93},/*        L        x-advance: 76.574219 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x4158e9aa, 0x00000000},
{'l', 0x00000000, 0x42b1957a},
{'l', 0x42432af4, 0xb6400000},
{'l', 0x00000000, 0x41368ce3},
{'l', 0xc279655f, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'[', 0x0041004c, 0x00000327},
{'@', 0x0000004d, 0x00007697},/*        M        x-advance: 118.589844 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x41a19c58, 0x00000000},
{'l', 0x41cc9054, 0x42886036},
{'l', 0x41cda336, 0xc2886036},
{'l', 0x41a19c5c, 0x00000000},
{'l', 0x00000000, 0x42c86716},
{'l', 0xc1538b30, 0x00000000},
{'l', 0x00000000, 0xc2aff920},
{'l', 0xc1ceb61c, 0x4289731c},
{'l', 0xc159fc94, 0x36800000},
{'l', 0xc1ceb61e, 0xc289731c},
{'l', 0x00000000, 0x42aff920},
{'l', 0xc1527844, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x0000004e, 0x000066d1},/*        N        x-advance: 102.816406 */
{'M', 0x4157d6c4, 0xc2c86716},
{'l', 0x41920a4f, 0x00000000},
{'l', 0x4231b7d6, 0x42a7a6a8},
{'l', 0x00000000, 0xc2a7a6a8},
{'l', 0x41527848, 0x00000000},
{'l', 0x00000000, 0x42c86716},
{'l', 0xc1920a50, 0x00000000},
{'l', 0xc231b7d6, 0xc2a7a6a8},
{'l', 0x00000000, 0x42a7a6a8},
{'l', 0xc1527844, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x0000004f, 0x00006c30},/*        O        x-advance: 108.187500 */
{'M', 0x4258a4f0, 0xc2b6036f},
{'q', 0xc16c3dd8, 0x00000000},
{0, 0xc1bbeb61, 0x41301b78},
{'q', 0xc10a8604, 0x41301b80},
{0, 0xc10a8604, 0x41f00000},
{'q', 0x00000000, 0x419768cf},
{0, 0x410a8604, 0x41ef768d},
{'q', 0x410b98ea, 0x41301b7d},
{0, 0x41bbeb61, 0x41301b7d},
{'q', 0x416c3dd8, 0x00000000},
{0, 0x41bad87c, 0xc1301b7d},
{'q', 0x410a8600, 0xc1301b7c},
{0, 0x410a8600, 0xc1ef768d},
{'q', 0x00000000, 0xc197f240},
{0, 0xc10a8600, 0xc1f00000},
{'9', 0xffa8ffbc, 0xffa8ff46},
{'m', 0x00000000, 0xc1301b80},
{'q', 0x41a89730, 0x00000000},
{0, 0x4206c3de, 0x416293c0},
{'q', 0x4149e110, 0x416180e0},
{0, 0x4149e110, 0x421768ce},
{'q', 0x00000000, 0x41bd87bc},
{0, 0xc149e110, 0x421768ce},
{'q', 0xc149e118, 0x416180dd},
{0, 0xc206c3de, 0x416180dd},
{'q', 0xc1a920a4, 0xb4c00000},
{0, 0xc2074d50, 0xc16180dc},
{'q', 0xc149e114, 0xc16180db},
{0, 0xc149e114, 0xc21768ce},
{'q', 0x00000000, 0xc1be112c},
{0, 0x4149e112, 0xc21768ce},
{'q', 0x414af3fa, 0xc16293c0},
{0, 0x42074d50, 0xc16293c0},
{'[', 0x002d004f, 0x000003d3},
{'@', 0x00000050, 0x000052e2},/*        P        x-advance: 82.882812 */
{'M', 0x41d86037, 0xc2b21eed},
{'4', 0x012d0000, 0x00000088},
{'q', 0x411768cc, 0x00000000},
{0, 0x416a180c, 0xc09cc750},
{'8', 0x9129d929, 0x91d7b900},
{'9', 0xffd9ffd7, 0xffd9ff8b},
{'6', 0x0000ff78, 0xffa7ff94},
{'l', 0x41f4d50c, 0x00000000},
{'q', 0x4186c3dc, 0x00000000},
{0, 0x41cb7d6a, 0x40f4d510},
{'q', 0x410a8608, 0x40f2af40},
{0, 0x410a8608, 0x41b24148},
{'q', 0x00000000, 0x416d50c0},
{0, 0xc10a8608, 0x41b35430},
{'9', 0x003cffbc, 0x003cff35},
{'l', 0xc1886037, 0x00000000},
{'l', 0x00000000, 0x422112e6},
{'l', 0xc158e9aa, 0x00000000},
{'l', 0x00000000, 0xc2c86716},
{'@', 0x00000051, 0x00006c30},/*        Q        x-advance: 108.187500 */
{'M', 0x4258a4f0, 0xc2b6036f},
{'q', 0xc16c3dd8, 0x00000000},
{0, 0xc1bbeb61, 0x41301b78},
{'q', 0xc10a8604, 0x41301b80},
{0, 0xc10a8604, 0x41f00000},
{'q', 0x00000000, 0x419768cf},
{0, 0x410a8604, 0x41ef768d},
{'q', 0x410b98ea, 0x41301b7d},
{0, 0x41bbeb61, 0x41301b7d},
{'q', 0x416c3dd8, 0x00000000},
{0, 0x41bad87c, 0xc1301b7d},
{'q', 0x410a8600, 0xc1301b7c},
{0, 0x410a8600, 0xc1ef768d},
{'q', 0x00000000, 0xc197f240},
{0, 0xc10a8600, 0xc1f00000},
{'9', 0xffa8ffbc, 0xffa8ff46},
{'m', 0x4197f240, 0x42b263a6},
{'4', 0x009c008e, 0x0000ff7d},
{'l', 0xc16d50bc, 0xc1805278},
{'8', 0x01e501ef, 0x00ef00f7},
{'q', 0xc1a920a4, 0x00000000},
{0, 0xc2074d50, 0xc16180dc},
{'q', 0xc149e114, 0xc16293c1},
{0, 0xc149e114, 0xc21768ce},
{'q', 0x00000000, 0xc1be112c},
{0, 0x4149e112, 0xc21768ce},
{'q', 0x414af3fa, 0xc16293c0},
{0, 0x42074d50, 0xc16293c0},
{'q', 0x41a89730, 0x00000000},
{0, 0x4206c3de, 0x416293c0},
{'q', 0x4149e110, 0x416180e0},
{0, 0x4149e110, 0x421768ce},
{'q', 0x00000000, 0x418b98ea},
{0, 0xc0e180e0, 0x41eeed1a},
{'q', 0xc0df5b10, 0x4146a860},
{0, 0xc1a225cc, 0x419293c2},
{'[', 0x002d0051, 0x000003d3},
{'@', 0x00000052, 0x00005f80},/*        R        x-advance: 95.500000 */
{'M', 0x427406df, 0xc23beb62},
{'8', 0x32430b22, 0x6a422621},
{'4', 0x00db006e, 0x0000ff8c},
{'l', 0xc14d19c8, 0xc1cda338},
{'8', 0x96b3b0d9, 0xe69be6db},
{'l', 0xc16c3dda, 0x00000000},
{'l', 0x00000000, 0x4229655e},
{'4', 0x0000ff94, 0xfcdf0000},
{'l', 0x41f4d50c, 0x00000000},
{'q', 0x4189731c, 0x00000000},
{0, 0x41cd19c6, 0x40e5cc70},
{'q', 0x41074d50, 0x40e5cc80},
{0, 0x41074d50, 0x41ad6c40},
{'q', 0x00000000, 0x411768cc},
{0, 0xc08dbec0, 0x417b4670},
{'9', 0x0031ffde, 0x0045ff9a},
{'m', 0xc207d6c4, 0xc2285278},
{'4', 0x011c0000, 0x00000088},
{'q', 0x411cc74c, 0x00000000},
{0, 0x416c3dd8, 0xc08fe480},
{'8', 0x9628dc28, 0x97d8ba00},
{'q', 0xc09eed18, 0xc08fe480},
{0, 0xc16c3dd8, 0xc08fe480},
{'l', 0xc1886037, 0x00000000},
{'@', 0x00000053, 0x0000573f},/*        S        x-advance: 87.246094 */
{'M', 0x42931d35, 0xc2c1d354},
{'l', 0x00000000, 0x41538b28},
{'q', 0xc0f6fad0, 0xc06c3dc0},
{0, 0xc1690528, 0xc0b01b70},
{'q', 0xc0db0f70, 0xbfe7f240},
{0, 0xc1538b28, 0xbfe7f240},
{'q', 0xc1312e64, 0x00000000},
{0, 0xc188e9ab, 0x40897310},
{'8', 0x61d122d1, 0x501f3500},
{'9', 0x001a0020, 0x002b0079},
{'l', 0x410301b8, 0x3fd6c3e0},
{'q', 0x4172af40, 0x4038b2b0},
{0, 0x41b2cabc, 0x412338b0},
{'q', 0x40e7f240, 0x40e7f240},
{0, 0x40e7f240, 0x419bb467},
{'q', 0x00000000, 0x41690528},
{0, 0xc11cc750, 0x41b0a4f0},
{'q', 0xc11bb464, 0x40f08975},
{0, 0xc1e4b98c, 0x40f08975},
{'q', 0xc0e3a6a8, 0x34000000},
{0, 0xc172af40, 0xbfce2cab},
{'9', 0xfff4ffc1, 0xffdaff7c},
{'l', 0x00000000, 0xc15f5b0f},
{'q', 0x4104149e, 0x4094301c},
{0, 0x4181655e, 0x40df5b0e},
{'q', 0x40fd6c3c, 0x401655e8},
{0, 0x417920a6, 0x401655e8},
{'q', 0x4139c594, 0x00000000},
{0, 0x418f5b0e, 0xc0920a4e},
{'8', 0x9832dc32, 0xa4dcc500},
{'9', 0xffdfffdd, 0xffcfff8a},
{'l', 0xc10414a0, 0xbfce2cc0},
{'q', 0xc172af3e, 0xc04149e0},
{0, 0xc1af920a, 0xc11768cc},
{'q', 0xc0d8e9a6, 0xc0ce2cb0},
{0, 0xc0d8e9a6, 0xc18f5b0e},
{'q', 0x00000000, 0xc1549e18},
{0, 0x41154301, 0xc1a7844c},
{'q', 0x411655e8, 0xc0f4d510},
{0, 0x41ceb61f, 0xc0f4d510},
{'q', 0x40e180d8, 0x00000000},
{0, 0x4165cc74, 0x3fa338c0},
{'q', 0x40ea1808, 0x3fa338c0},
{0, 0x416f768c, 0x4074d500},
{'[', 0x00410053, 0x0000028c},
{'@', 0x00000054, 0x000053f5},/*        T        x-advance: 83.957031 */
{'M', 0xbece2cac, 0xc2c86716},
{'l', 0x42a987bb, 0x00000000},
{'l', 0x00000000, 0x41368ce0},
{'l', 0xc20e4828, 0x00000000},
{'l', 0x00000000, 0x42b1957a},
{'l', 0xc159fc90, 0x00000000},
{'l', 0x00000000, 0xc2b1957a},
{'l', 0xc20e4829, 0x00000000},
{'l', 0xb5b00000, 0xc1368ce0},
{'@', 0x00000055, 0x0000649a},/*        U        x-advance: 100.601562 */
{'M', 0x413f2414, 0xc2c86716},
{'4', 0x0000006c, 0x01e60000},
{'q', 0x00000000, 0x4180dbeb},
{0, 0x40bad87c, 0x41b9c595},
{'q', 0x40bad87c, 0x40e180da},
{0, 0x419768cd, 0x40e180da},
{'q', 0x41505278, 0x00000000},
{0, 0x4196df5a, 0xc0e180da},
{'9', 0xffc8002e, 0xff47002e},
{'4', 0xfe1a0000, 0x0000006c},
{'l', 0x00000000, 0x427a338b},
{'q', 0x00000000, 0x419cc74d},
{0, 0xc11bb468, 0x41ecc74c},
{'q', 0xc11aa180, 0x41200001},
{0, 0xc1e4b98e, 0x41200001},
{'q', 0xc197f240, 0xb4c00000},
{0, 0xc1e5cc74, 0xc1200000},
{'q', 0xc11aa180, 0xc11fffff},
{0, 0xc11aa180, 0xc1ecc74c},
{'l', 0x00000000, 0xc27a338b},
{'@', 0x00000056, 0x00005e06},/*        V        x-advance: 94.023438 */
{'M', 0x421d50c0, 0x00000000},
{'l', 0xc2190527, 0xc2c86716},
{'l', 0x416293c1, 0x00000000},
{'l', 0x41fdf5b2, 0x42a8b98e},
{'l', 0x41fe7f24, 0xc2a8b98e},
{'l', 0x416180d8, 0x00000000},
{'l', 0xc218c06d, 0x42c86716},
{'l', 0xc175e7f4, 0x00000000},
{'@', 0x00000057, 0x000087e7},/*        W        x-advance: 135.902344 */
{'M', 0x40920a4f, 0xc2c86716},
{'l', 0x415b0f76, 0x00000000},
{'l', 0x41a89731, 0x42a9655e},
{'l', 0x41a80dbe, 0xc2a9655e},
{'l', 0x4173c224, 0x00000000},
{'l', 0x41a89734, 0x42a9655e},
{'l', 0x41a80dbc, 0xc2a9655e},
{'l', 0x415c2260, 0x00000000},
{'l', 0xc1c957a0, 0x42c86716},
{'l', 0xc1886038, 0x00000000},
{'l', 0xc1a920a4, 0xc2adf5b1},
{'l', 0xc1aabcfe, 0x42adf5b1},
{'l', 0xc1886036, 0x00000000},
{'l', 0xc1c8ce2c, 0xc2c86716},
{'@', 0x00000058, 0x00005e29},/*        X        x-advance: 94.160156 */
{'M', 0x410a8603, 0xc2c86716},
{'l', 0x41690527, 0x00000000},
{'l', 0x41c731d3, 0x4214fe48},
{'l', 0x41c844b8, 0xc214fe48},
{'l', 0x41690528, 0x00000000},
{'l', 0xc200dbeb, 0x42407bb4},
{'l', 0x4209731d, 0x42505278},
{'l', 0xc1690528, 0x00000000},
{'l', 0xc1e180da, 0xc22a7844},
{'l', 0xc1e31d35, 0x422a7844},
{'l', 0xc16a180e, 0x00000000},
{'l', 0x420f1656, 0xc255f5b1},
{'l', 0xc1f9aa18, 0xc23ad87b},
{'@', 0x00000059, 0x000053f5},/*        Y        x-advance: 83.957031 */
{'M', 0xbe89731d, 0xc2c86716},
{'l', 0x41690527, 0x00000000},
{'l', 0x41de4829, 0x4224d50c},
{'l', 0x41dcabd0, 0xc224d50c},
{'l', 0x41690528, 0x00000000},
{'l', 0xc20dbeb6, 0x4251eed1},
{'l', 0x00000000, 0x423edf5b},
{'l', 0xc159fc90, 0x00000000},
{'l', 0x00000000, 0xc23edf5b},
{'l', 0xc20dbeb6, 0xc251eed1},
{'@', 0x0000005a, 0x00005e29},/*        Z        x-advance: 94.160156 */
{'M', 0x40f6fad8, 0xc2c86716},
{'l', 0x429d731c, 0x00000000},
{'l', 0x00000000, 0x41255e80},
{'l', 0xc27d6c3c, 0x429ce9aa},
{'l', 0x4281cc74, 0xb6400000},
{'l', 0x00000000, 0x41368ce3},
{'l', 0xc2a39fc8, 0x00000000},
{'l', 0xb6400000, 0xc1255e7f},
{'l', 0x427d6c3d, 0xc29ce9aa},
{'l', 0xc2773f91, 0x00000000},
{'l', 0x00000000, 0xc1368ce0},
{'@', 0x0000005b, 0x0000359f},/*        [        x-advance: 53.621094 */
{'M', 0x413cfe48, 0xc2d0dbeb},
{'l', 0x41e3a6a8, 0x00000000},
{'l', 0x00000000, 0x41198e98},
{'l', 0xc180dbeb, 0x00000000},
{'l', 0x00000000, 0x42ceb61f},
{'l', 0x4180dbeb, 0xb5800000},
{'l', 0x00000000, 0x41198e9b},
{'l', 0xc1e3a6a8, 0x00000000},
{'l', 0x00000000, 0xc2f519c5},
{'@', 0x0000005c, 0x00002e4f},/*       \         x-advance: 46.308594 */
{'M', 0x41368ce3, 0xc2c86716},
{'l', 0x420b98e9, 0x42e1e7f2},
{'l', 0xc1368ce4, 0xb5800000},
{'l', 0xc20b98e9, 0xc2e1e7f2},
{'l', 0x41368ce3, 0x00000000},
{'@', 0x0000005d, 0x0000359f},/*        ]        x-advance: 53.621094 */
{'M', 0x42273f92, 0xc2d0dbeb},
{'l', 0x00000000, 0x42f519c5},
{'l', 0xc1e3a6a8, 0x36000000},
{'l', 0xb5800000, 0xc1198e9b},
{'l', 0x41805278, 0x00000000},
{'l', 0x00000000, 0xc2ceb61f},
{'l', 0xc1805278, 0x00000000},
{'l', 0xb5800000, 0xc1198e98},
{'l', 0x41e3a6a8, 0x00000000},
{'@', 0x0000005e, 0x0000732a},/*        ^        x-advance: 115.164062 */
{'M', 0x42805278, 0xc2c86716},
{'l', 0x4211c596, 0x421587bb},
{'l', 0xc157d6c8, 0x00000000},
{'l', 0xc1ec3dd8, 0xc1d4149e},
{'l', 0xc1ec3ddb, 0x41d4149e},
{'l', 0xc157d6c3, 0x00000000},
{'l', 0x4211c595, 0xc21587bb},
{'l', 0x41527844, 0x00000000},
{'@', 0x0000005f, 0x000044b9},/*        _        x-advance: 68.722656 */
{'M', 0x428c225d, 0x41b68ce3},
{'l', 0x00000000, 0x41198e9a},
{'l', 0xc28ed19d, 0x00000000},
{'l', 0x36600000, 0xc1198e9a},
{'l', 0x428ed19d, 0x00000000},
{'@', 0x00000060, 0x000044b9},/*        `        x-advance: 68.722656 */
{'M', 0x41c50c07, 0xc2dbdda3},
{'l', 0x419768cd, 0x41c8ce2c},
{'l', 0xc1244b98, 0x00000000},
{'l', 0xc1af0896, 0xc1c8ce2c},
{'l', 0x41538b2a, 0x00000000},
{'@', 0x00000061, 0x0000543a},/*        a        x-advance: 84.226562 */
{'M', 0x423c74d5, 0xc2172414},
{'q', 0xc16f768c, 0x00000000},
{0, 0xc1a5e7f2, 0x405b0f70},
{'8', 0x5dd21bd2, 0x53223400},
{'q', 0x408b98e8, 0x4074d50c},
{0, 0x413cfe46, 0x4074d50c},
{'q', 0x41244b9c, 0x00000000},
{0, 0x41838b2c, 0xc0e7f242},
{'9', 0xffc60031, 0xff650031},
{'4', 0xffea0000, 0x0000ff9e},
{'m', 0x41c50c06, 0xc0a338b8},
{'4', 0x01570000, 0x0000ff9e},
{'l', 0x00000000, 0xc1368ce3},
{'q', 0xc0874d50, 0x40db0f77},
{0, 0xc1289734, 0x412225cd},
{'q', 0xc0c9e110, 0x404e2caa},
{0, 0xc176fad8, 0x404e2caa},
{'q', 0xc138b2ae, 0x34000000},
{0, 0xc1931d34, 0xc0ce2cab},
{'q', 0xc0d8e9ac, 0xc0d05278},
{0, 0xc0d8e9ac, 0xc18b0f76},
{'q', 0x00000000, 0xc14af3fc},
{0, 0x41074d50, 0xc1990528},
{'9', 0xffcd0044, 0xffcd00ca},
{'4', 0x0000008a, 0xfff70000},
{'q', 0x00000000, 0xc1086034},
{0, 0xc0b46718, 0xc1527844},
{'q', 0xc0b24148, 0xc09655e0},
{0, 0xc17b4670, 0xc09655e0},
{'8', 0x0c9c00cd, 0x25a30cd0},
{'l', 0x00000000, 0xc1368ce4},
{'8', 0xe169ec36, 0xf663f633},
{'q', 0x41827844, 0x00000000},
{0, 0x41c2e63a, 0x41074d50},
{'q', 0x4100dbec, 0x41074d54},
{0, 0x4100dbec, 0x41cd19c6},
{'@', 0x00000062, 0x0000573f},/*        b        x-advance: 87.246094 */
{'M', 0x4285d354, 0xc216112e},
{'q', 0x00000000, 0xc159fc90},
{0, 0xc0b46718, 0xc1aabcfe},
{'q', 0xc0b24148, 0xc0f920a8},
{0, 0xc175e7f0, 0xc0f920a8},
{'q', 0xc11cc750, 0x00000000},
{0, 0xc176fada, 0x40f920a8},
{'q', 0xc0b24148, 0x40f6fad8},
{0, 0xc0b24148, 0x41aabcfe},
{'q', 0x00000000, 0x4159fc90},
{0, 0x40b24148, 0x41ab4671},
{'q', 0x40b46714, 0x40f6fad8},
{0, 0x4176fada, 0x40f6fad8},
{'q', 0x411cc74c, 0x00000000},
{0, 0x4175e7f0, 0xc0f6fad8},
{'9', 0xffc2002d, 0xff55002d},
{'m', 0xc2280dbe, 0xc1d1eed2},
{'q', 0x407920a0, 0xc0d6c3d8},
{0, 0x411cc74c, 0xc11eed1c},
{'q', 0x40bf2410, 0xc0527840},
{0, 0x4163a6a8, 0xc0527840},
{'q', 0x415b0f74, 0x00000000},
{0, 0x41b1b7d6, 0x412df5b0},
{'q', 0x41097320, 0x412df5b4},
{0, 0x41097320, 0x41e4b990},
{'q', 0x00000000, 0x418dbeb6},
{0, 0xc1097320, 0x41e4b98e},
{'q', 0xc1086038, 0x412df5b1},
{0, 0xc1b1b7d6, 0x412df5b1},
{'q', 0xc10414a0, 0xb4c00000},
{0, 0xc163a6a8, 0xc04e2cad},
{'9', 0xffe6ffd1, 0xffb0ffb2},
{'l', 0x00000000, 0x41346716},
{'l', 0xc146a860, 0x00000000},
{'l', 0x00000000, 0xc2d0dbeb},
{'l', 0x4146a860, 0x00000000},
{'l', 0x00000000, 0x4222af3f},
{'@', 0x00000063, 0x00004b92},/*        c        x-advance: 75.570312 */
{'M', 0x4286180e, 0xc2909052},
{'l', 0x00000000, 0x4138b2ac},
{'8', 0xdeace9d7, 0xf5acf5d7},
{'q', 0xc14036fc, 0x00000000},
{0, 0xc1954302, 0x40f4d508},
{'q', 0xc0d49e10, 0x40f2af40},
{0, 0xc0d49e10, 0x41aabcfe},
{'q', 0x00000000, 0x415c225c},
{0, 0x40d49e10, 0x41ab4671},
{'q', 0x40d49e10, 0x40f2af3e},
{0, 0x41954302, 0x40f2af3e},
{'8', 0xf554002a, 0xde54f52a},
{'l', 0x00000000, 0x41368ce2},
{'8', 0x1cab13d7, 0x09a309d4},
{'q', 0xc187d6c4, 0x00000000},
{0, 0xc1d7d6c4, 0xc12abcfe},
{'q', 0xc1200000, 0xc12abcff},
{0, 0xc1200000, 0xc1e655e8},
{'q', 0xb5000000, 0xc1931d36},
{0, 0x412112e6, 0xc1e768d0},
{'q', 0x412225cc, 0xc1289730},
{0, 0x41ddbeb5, 0xc1289730},
{'8', 0x0959002d, 0x1b54092b},
{'@', 0x00000064, 0x0000573f},/*        d        x-advance: 87.246094 */
{'M', 0x4279aa18, 0xc27f0897},
{'l', 0x00000000, 0xc222af3f},
{'l', 0x41459578, 0x00000000},
{'4', 0x03430000, 0x0000ff9e},
{'l', 0x00000000, 0xc1346716},
{'q', 0xc07920b0, 0x40d6c3dd},
{0, 0xc11dda34, 0x41200000},
{'q', 0xc0bcfe48, 0x404e2caa},
{0, 0xc163a6a8, 0x404e2caa},
{'q', 0xc159fc90, 0x34000000},
{0, 0xc1b1b7d7, 0xc12df5b0},
{'q', 0xc1086036, 0xc12df5b0},
{0, 0xc1086036, 0xc1e4b98e},
{'q', 0xb5000000, 0xc18dbeb6},
{0, 0x41086036, 0xc1e4b990},
{'q', 0x4109731e, 0xc12df5b0},
{0, 0x41b1b7d7, 0xc12df5b0},
{'q', 0x41052784, 0x00000000},
{0, 0x4163a6a8, 0x40527840},
{'9', 0x0019002f, 0x004f004e},
{'m', 0xc2285278, 0x41d1eed2},
{'q', 0xb6000000, 0x4159fc90},
{0, 0x40b24148, 0x41ab4671},
{'q', 0x40b46714, 0x40f6fad8},
{0, 0x4176fad8, 0x40f6fad8},
{'q', 0x411cc74c, 0x00000000},
{0, 0x4176fad8, 0xc0f6fad8},
{'q', 0x40b46718, 0xc0f920a4},
{0, 0x40b46718, 0xc1ab4671},
{'q', 0x00000000, 0xc159fc90},
{0, 0xc0b46718, 0xc1aabcfe},
{'q', 0xc0b46718, 0xc0f920a8},
{0, 0xc176fad8, 0xc0f920a8},
{'q', 0xc11cc74e, 0x00000000},
{0, 0xc176fad8, 0x40f920a8},
{'q', 0xc0b2414c, 0x40f6fad8},
{0, 0xc0b2414c, 0x41aabcfe},
{'@', 0x00000065, 0x00005490},/*        e        x-advance: 84.562500 */
{'M', 0x429a7f24, 0xc222af3f},
{'4', 0x00300000, 0x0000fe3a},
{'q', 0x3f4e2ca0, 0x414c06de},
{0, 0x40f4d508, 0x419bb466},
{'q', 0x40dd3548, 0x40d49e12},
{0, 0x41998e9a, 0x40d49e12},
{'8', 0xf36e0038, 0xd76af335},
{'l', 0x00000000, 0x413ad87b},
{'q', 0xc0d49e10, 0x40346716},
{0, 0xc159fc8c, 0x4089731d},
{'q', 0xc0df5b10, 0x3fbcfe49},
{0, 0xc16293c4, 0x3fbcfe49},
{'q', 0xc18fe482, 0x00000000},
{0, 0xc1e4301b, 0xc127844c},
{'q', 0xc127844a, 0xc127844b},
{0, 0xc127844a, 0xc1e293c2},
{'q', 0xb5000000, 0xc193a6a8},
{0, 0x411eed1a, 0xc1ea180e},
{'q', 0x411ffffe, 0xc12df5b0},
{0, 0x41d74d4f, 0xc12df5b0},
{'q', 0x4172af40, 0x00000000},
{0, 0x41bfad88, 0x411cc750},
{'9', 0x004d0046, 0x00d40046},
{'m', 0xc1459578, 0xc067f240},
{'q', 0xbe097400, 0xc12225cc},
{0, 0xc0b68ce8, 0xc1816560},
{'q', 0xc0b01b80, 0xc0c149d8},
{0, 0xc16a180c, 0xc0c149d8},
{'q', 0xc1255e80, 0x00000000},
{0, 0xc1849e12, 0x40bad878},
{'q', 0xc0c59578, 0x40bad878},
{0, 0xc0e3a6a8, 0x41838b2a},
{'l', 0x42301b7e, 0xbd897200},
{'@', 0x00000066, 0x00003063},/*        f        x-advance: 48.386719 */
{'M', 0x424c06df, 0xc2d0dbeb},
{'4', 0x00520000, 0x0000ffa2},
{'8', 0x15b600cb, 0x4dec15ec},
{'l', 0x00000000, 0x40d49e10},
{'l', 0x41a2af40, 0x00000000},
{'l', 0x00000000, 0x41198ea0},
{'l', 0xc1a2af40, 0x00000000},
{'l', 0x00000000, 0x42832414},
{'l', 0xc146a85f, 0x00000000},
{'l', 0x00000000, 0xc2832414},
{'0', 0xb40000a2, 0xd700005e},
{'q', 0x00000000, 0xc148ce30},
{0, 0x40bad87a, 0xc1920a50},
{'q', 0x40bad87c, 0xc0b8b2b0},
{0, 0x4194301b, 0xc0b8b2b0},
{'l', 0x413ad87c, 0x00000000},
{'@', 0x00000067, 0x0000573f},/*        g        x-advance: 87.246094 */
{'M', 0x4279aa18, 0xc219d354},
{'q', 0x00000000, 0xc156c3dc},
{0, 0xc0b24150, 0xc1a67166},
{'q', 0xc0b01b78, 0xc0ec3dd8},
{0, 0xc1780dbc, 0xc0ec3dd8},
{'q', 0xc11eed1a, 0x00000000},
{0, 0xc1780dbe, 0x40ec3dd8},
{'q', 0xc0b01b80, 0x40ec3de0},
{0, 0xc0b01b80, 0x41a67166},
{'q', 0x00000000, 0x4155b0f8},
{0, 0x40b01b80, 0x41a5e7f2},
{'q', 0x40b24148, 0x40ec3dda},
{0, 0x41780dbe, 0x40ec3dda},
{'q', 0x41200000, 0x00000000},
{0, 0x41780dbc, 0xc0ec3dda},
{'9', 0xffc5002c, 0xff5b002c},
{'m', 0x41459578, 0x41e90528},
{'q', 0x00000000, 0x41998e9a},
{0, 0xc1086038, 0x41e4b98e},
{'q', 0xc1086034, 0x41154300},
{0, 0xc1d0dbea, 0x41154300},
{'8', 0xf99e00cc, 0xe8a7f8d2},
{'l', 0x00000000, 0xc14036fb},
{'8', 0x2255172b, 0x0b560b2a},
{'q', 0x41425cc4, 0x00000000},
{0, 0x419180dc, 0xc0c9e112},
{'9', 0xffcd0030, 0xff670030},
{'l', 0x00000000, 0xc0c36fb0},
{'q', 0xc074d510, 0x40d49e12},
{0, 0xc11cc750, 0x411eed1a},
{'q', 0xc0bf2410, 0x40527844},
{0, 0xc164b98c, 0x40527844},
{'q', 0xc15d3544, 0x00000000},
{0, 0xc1b2414a, 0xc1289732},
{'q', 0xc1074d50, 0xc1289732},
{0, 0xc1074d50, 0xc1df5b0f},
{'q', 0xb5000000, 0xc18b98ea},
{0, 0x41074d50, 0xc1dfe484},
{'q', 0x41074d50, 0xc1289730},
{0, 0x41b2414a, 0xc1289730},
{'q', 0x41052784, 0x00000000},
{0, 0x4164b98c, 0x40527840},
{'9', 0x001a002f, 0x004f004e},
{'l', 0x00000000, 0xc1368ce4},
{'l', 0x41459578, 0x00000000},
{'l', 0x00000000, 0x4283ad88},
{'@', 0x00000068, 0x0000571d},/*        h        x-advance: 87.113281 */
{'M', 0x4296df5b, 0xc23579fc},
{'4', 0x016a0000, 0x0000ff9e},
{'l', 0x00000000, 0xc233dda3},
{'q', 0x00000000, 0xc12abcfc},
{0, 0xc0852780, 0xc17f9208},
{'q', 0xc0852788, 0xc0a9aa18},
{0, 0xc147bb48, 0xc0a9aa18},
{'q', 0xc1200000, 0x00000000},
{0, 0xc17c5956, 0x40cc06d8},
{'9', 0x0033ffd2, 0x008bffd2},
{'l', 0x00000000, 0x4229eed1},
{'l', 0xc146a860, 0x00000000},
{'4', 0xfcbd0000, 0x00000063},
{'l', 0x00000000, 0x4223c225},
{'8', 0xaf53ca23, 0xe66fe630},
{'q', 0x414f3f90, 0x00000000},
{0, 0x419cc74e, 0x4100dbf0},
{'q', 0x40d49e10, 0x40ff9208},
{0, 0x40d49e10, 0x41bc74d4},
{'@', 0x00000069, 0x00002630},/*        i        x-advance: 38.187500 */
{'M', 0x414f3f92, 0xc29655e8},
{'l', 0x4145957a, 0x00000000},
{'4', 0x02590000, 0x0000ff9e},
{'6', 0xfda70000, 0xff160000},
{'l', 0x4145957a, 0x00000000},
{'l', 0x00000000, 0x417a3388},
{'l', 0xc145957a, 0x00000000},
{'l', 0x00000000, 0xc17a3388},
{'@', 0x0000006a, 0x00002630},/*        j        x-advance: 38.187500 */
{'M', 0x414f3f92, 0xc29655e8},
{'4', 0x00000062, 0x02640000},
{'q', 0x00000000, 0x4165cc71},
{0, 0xc0b01b80, 0x41a67163},
{'9', 0x0033ffd5, 0x0033ff74},
{'4', 0x0000ffdb, 0xffad0000},
{'l', 0x40527845, 0x00000000},
{'8', 0xe74c0038, 0x9414e614},
{'6', 0xfd9c0000, 0xff160000},
{'l', 0x4145957a, 0x00000000},
{'l', 0x00000000, 0x417a3388},
{'l', 0xc145957a, 0x00000000},
{'l', 0x00000000, 0xc17a3388},
{'@', 0x0000006b, 0x00004f98},/*        k        x-advance: 79.593750 */
{'M', 0x4147bb46, 0xc2d0dbeb},
{'l', 0x4146a860, 0x00000000},
{'l', 0x00000000, 0x4276b61e},
{'l', 0x421361ee, 0xc201aa18},
{'l', 0x417c5958, 0x00000000},
{'l', 0xc21f768d, 0x420cabd0},
{'l', 0x42262cab, 0x42200000},
{'l', 0xc180dbea, 0x00000000},
{'l', 0xc218c06e, 0xc212d87b},
{'l', 0x36000000, 0x4212d87b},
{'l', 0xc146a860, 0x00000000},
{'l', 0x00000000, 0xc2d0dbeb},
{'@', 0x0000006c, 0x00002630},/*        l        x-advance: 38.187500 */
{'M', 0x414f3f92, 0xc2d0dbeb},
{'l', 0x4145957a, 0x00000000},
{'l', 0x00000000, 0x42d0dbeb},
{'l', 0xc145957a, 0x00000000},
{'l', 0x00000000, 0xc2d0dbeb},
{'@', 0x0000006d, 0x000085e4},/*        m        x-advance: 133.890625 */
{'M', 0x428ef3f9, 0xc272f3f9},
{'q', 0x40943020, 0xc1052784},
{0, 0x41312e60, 0xc1448294},
{'q', 0x40ce2cb0, 0xc07d6c40},
{0, 0x4172af40, 0xc07d6c40},
{'q', 0x413beb60, 0x00000000},
{0, 0x4190f768, 0x410414a0},
{'9', 0x00410033, 0x00ba0033},
{'4', 0x016a0000, 0x0000ff9d},
{'l', 0x00000000, 0xc233dda3},
{'q', 0x00000000, 0xc12ce2cc},
{0, 0xc074d500, 0xc1805278},
{'q', 0xc074d500, 0xc0a78448},
{0, 0xc13ad878, 0xc0a78448},
{'q', 0xc1198ea0, 0x00000000},
{0, 0xc172af40, 0x40cc06d8},
{'9', 0x0033ffd4, 0x008bffd4},
{'4', 0x01530000, 0x0000ff9d},
{'l', 0x00000000, 0xc233dda3},
{'q', 0x00000000, 0xc12df5b0},
{0, 0xc074d510, 0xc1805278},
{'q', 0xc074d500, 0xc0a78448},
{0, 0xc13cfe48, 0xc0a78448},
{'q', 0xc11768cc, 0x00000000},
{0, 0xc1708972, 0x40ce2ca8},
{'9', 0x0033ffd4, 0x008affd4},
{'l', 0x00000000, 0x4229eed1},
{'l', 0xc146a860, 0x00000000},
{'4', 0xfda70000, 0x00000063},
{'l', 0x00000000, 0x413ad87c},
{'q', 0x40874d50, 0xc0dd3538},
{0, 0x412225ce, 0xc12338b4},
{'q', 0x40bcfe48, 0xc0527840},
{0, 0x41606df4, 0xc0527840},
{'8', 0x216f0041, 0x6044212e},
{'@', 0x0000006e, 0x0000571d},/*        n        x-advance: 87.113281 */
{'M', 0x4296df5b, 0xc23579fc},
{'4', 0x016a0000, 0x0000ff9e},
{'l', 0x00000000, 0xc233dda3},
{'q', 0x00000000, 0xc12abcfc},
{0, 0xc0852780, 0xc17f9208},
{'q', 0xc0852788, 0xc0a9aa18},
{0, 0xc147bb48, 0xc0a9aa18},
{'q', 0xc1200000, 0x00000000},
{0, 0xc17c5956, 0x40cc06d8},
{'9', 0x0033ffd2, 0x008bffd2},
{'l', 0x00000000, 0x4229eed1},
{'l', 0xc146a860, 0x00000000},
{'4', 0xfda70000, 0x00000063},
{'l', 0x00000000, 0x413ad87c},
{'8', 0xaf53ca23, 0xe66fe630},
{'q', 0x414f3f90, 0x00000000},
{0, 0x419cc74e, 0x4100dbf0},
{'q', 0x40d49e10, 0x40ff9208},
{0, 0x40d49e10, 0x41bc74d4},
{'@', 0x0000006f, 0x00005418},/*        o        x-advance: 84.093750 */
{'M', 0x42285278, 0xc2850527},
{'q', 0xc11eed18, 0x00000000},
{0, 0xc17b4670, 0x40f920a0},
{'q', 0xc0b8b2b0, 0x40f6fad8},
{0, 0xc0b8b2b0, 0x41a9aa18},
{'q', 0x00000000, 0x4157d6c2},
{0, 0x40b68ce0, 0x41aa338b},
{'q', 0x40b8b2b0, 0x40f6fad6},
{0, 0x417c5958, 0x40f6fad6},
{'q', 0x411dda34, 0x00000000},
{0, 0x417a338c, 0xc0f920a6},
{'q', 0x40b8b2b0, 0xc0f920a4},
{0, 0x40b8b2b0, 0xc1a9aa17},
{'q', 0x00000000, 0xc155b0f8},
{0, 0xc0b8b2b0, 0xc1a920a6},
{'9', 0xffc2ffd2, 0xffc2ff83},
{'m', 0x00000000, 0xc1278450},
{'q', 0x4180dbec, 0x00000000},
{0, 0x41ca6a84, 0x41278450},
{'q', 0x41131d38, 0x41278448},
{0, 0x41131d38, 0x41e7f240},
{'q', 0x00000000, 0x4193a6a8},
{0, 0xc1131d38, 0x41e7f240},
{'q', 0xc1131d30, 0x4127844d},
{0, 0xc1ca6a84, 0x4127844d},
{'q', 0xc181655e, 0xb4c00000},
{0, 0xc1caf3f9, 0xc127844c},
{'q', 0xc1120a4e, 0xc1289731},
{0, 0xc1120a4e, 0xc1e7f240},
{'q', 0xb5000000, 0xc194301c},
{0, 0x41120a4e, 0xc1e7f240},
{'q', 0x41131d36, 0xc1278450},
{0, 0x41caf3f9, 0xc1278450},
{'[', 0x002d006f, 0x0000028c},
{'@', 0x00000070, 0x0000573f},/*        p        x-advance: 87.246094 */
{'M', 0x41c731d3, 0xc1346716},
{'l', 0x00000000, 0x421f768c},
{'l', 0xc146a860, 0x36000000},
{'4', 0xfcc20000, 0x00000063},
{'l', 0x00000000, 0x41368ce4},
{'q', 0x407920a8, 0xc0d6c3d8},
{0, 0x411cc74e, 0xc11eed1c},
{'q', 0x40bf2410, 0xc0527840},
{0, 0x4163a6a8, 0xc0527840},
{'q', 0x415b0f74, 0x00000000},
{0, 0x41b1b7d6, 0x412df5b0},
{'q', 0x41097320, 0x412df5b4},
{0, 0x41097320, 0x41e4b990},
{'q', 0x00000000, 0x418dbeb6},
{0, 0xc1097320, 0x41e4b98e},
{'q', 0xc1086038, 0x412df5b1},
{0, 0xc1b1b7d6, 0x412df5b1},
{'q', 0xc10414a0, 0xb4c00000},
{0, 0xc163a6a8, 0xc04e2cad},
{'9', 0xffe6ffd1, 0xffb0ffb2},
{'m', 0x42280dbe, 0xc1d1eed1},
{'q', 0x00000000, 0xc159fc90},
{0, 0xc0b46718, 0xc1aabcfe},
{'q', 0xc0b24148, 0xc0f920a8},
{0, 0xc175e7f0, 0xc0f920a8},
{'q', 0xc11cc750, 0x00000000},
{0, 0xc176fada, 0x40f920a8},
{'q', 0xc0b24148, 0x40f6fad8},
{0, 0xc0b24148, 0x41aabcfe},
{'q', 0x00000000, 0x4159fc90},
{0, 0x40b24148, 0x41ab4671},
{'q', 0x40b46714, 0x40f6fad8},
{0, 0x4176fada, 0x40f6fad8},
{'q', 0x411cc74c, 0x00000000},
{0, 0x4175e7f0, 0xc0f6fad8},
{'q', 0x40b46718, 0xc0f920a4},
{0, 0x40b46718, 0xc1ab4671},
{'@', 0x00000071, 0x0000573f},/*        q        x-advance: 87.246094 */
{'M', 0x41a2af3f, 0xc216112e},
{'q', 0x00000000, 0x4159fc90},
{0, 0x40b2414c, 0x41ab4671},
{'q', 0x40b46714, 0x40f6fad8},
{0, 0x4176fad8, 0x40f6fad8},
{'q', 0x411cc74c, 0x00000000},
{0, 0x4176fad8, 0xc0f6fad8},
{'q', 0x40b46718, 0xc0f920a4},
{0, 0x40b46718, 0xc1ab4671},
{'q', 0x00000000, 0xc159fc90},
{0, 0xc0b46718, 0xc1aabcfe},
{'q', 0xc0b46718, 0xc0f920a8},
{0, 0xc176fad8, 0xc0f920a8},
{'q', 0xc11cc74e, 0x00000000},
{0, 0xc176fad8, 0x40f920a8},
{'9', 0x003dffd4, 0x00aaffd4},
{'m', 0x42285278, 0x41d1eed1},
{'q', 0xc07920b0, 0x40d6c3dd},
{0, 0xc11dda34, 0x41200000},
{'q', 0xc0bcfe48, 0x404e2caa},
{0, 0xc163a6a8, 0x404e2caa},
{'q', 0xc159fc90, 0x34000000},
{0, 0xc1b1b7d7, 0xc12df5b0},
{'q', 0xc1086036, 0xc12df5b0},
{0, 0xc1086036, 0xc1e4b98e},
{'q', 0xb5000000, 0xc18dbeb6},
{0, 0x41086036, 0xc1e4b990},
{'q', 0x4109731e, 0xc12df5b0},
{0, 0x41b1b7d7, 0xc12df5b0},
{'q', 0x41052784, 0x00000000},
{0, 0x4163a6a8, 0x40527840},
{'9', 0x0019002f, 0x004f004e},
{'l', 0x00000000, 0xc1368ce4},
{'l', 0x41459578, 0x00000000},
{'l', 0x00000000, 0x42cf844c},
{'l', 0xc1459578, 0xb6800000},
{'l', 0x00000000, 0xc21f768c},
{'@', 0x00000072, 0x00003882},/*        r        x-advance: 56.507812 */
{'M', 0x42620a4f, 0xc27e7f24},
{'8', 0xf3dcf7f0, 0xfcd6fced},
{'q', 0xc127844c, 0x00000000},
{0, 0xc180dbeb, 0x40db0f78},
{'9', 0x0036ffd4, 0x009cffd4},
{'l', 0x00000000, 0x421e63a6},
{'l', 0xc146a860, 0x00000000},
{'4', 0xfda70000, 0x00000063},
{'l', 0x00000000, 0x413ad87c},
{'q', 0x407920a8, 0xc0db0f78},
{0, 0x412225ce, 0xc12225cc},
{'q', 0x40c7bb40, 0xc056c3e0},
{0, 0x4172af3c, 0xc056c3e0},
{'8', 0x0116000a, 0x031b010c},
{'l', 0x3d897400, 0x414af3f8},
{'@', 0x00000073, 0x0000479c},/*        s        x-advance: 71.609375 */
{'M', 0x42737d6c, 0xc291e7f2},
{'l', 0x00000000, 0x413ad87c},
{'8', 0xe0aaebd7, 0xf6a3f6d3},
{'8', 0x169200b7, 0x43dc16dc},
{'8', 0x361a2200, 0x2569131a},
{'l', 0x40874d50, 0x3f708980},
{'q', 0x41527844, 0x40346720},
{0, 0x41954302, 0x40ff9208},
{'q', 0x40b24150, 0x40a338b4},
{0, 0x40b24150, 0x4164b990},
{'q', 0x00000000, 0x4127844b},
{0, 0xc1052788, 0x41849e11},
{'q', 0xc104149c, 0x40c36fad},
{0, 0xc1b6036e, 0x40c36fad},
{'8', 0xf79c00d0, 0xe492f7cc},
{'l', 0x00000000, 0xc14c06df},
{'8', 0x2a6b1c36, 0x0d690d35},
{'8', 0xe96b0045, 0xbd25e825},
{'q', 0x00000000, 0xc0a112e8},
{0, 0xc05b0f70, 0xc0f6fad8},
{'9', 0xffebffe6, 0xffd7ff8a},
{'l', 0xc0897320, 0xbf80dbe0},
{'q', 0xc1379fc8, 0xc01aa180},
{0, 0xc1849e11, 0xc0ec3de0},
{'q', 0xc0a338b2, 0xc0a112e0},
{0, 0xc0a338b2, 0xc15c225c},
{'q', 0x00000000, 0xc129aa18},
{0, 0x40f08972, 0xc18301b8},
{'q', 0x40f08974, 0xc0b8b2b0},
{0, 0x41aabcff, 0xc0b8b2b0},
{'8', 0x08670036, 0x18590830},
{'@', 0x00000074, 0x000035e4},/*        t        x-advance: 53.890625 */
{'M', 0x41c9579f, 0xc2c10527},
{'l', 0x00000000, 0x41aabcfc},
{'l', 0x41cb7d6d, 0x00000000},
{'4', 0x004c0000, 0x0000ff35},
{'l', 0x00000000, 0x422338b2},
{'8', 0x5e134900, 0x14521414},
{'4', 0x00000065, 0x00520000},
{'l', 0xc14af3f8, 0x00000000},
{'q', 0xc164b990, 0x00000000},
{0, 0xc19dda34, 0xc0a9aa18},
{'9', 0xffd6ffd5, 0xff65ffd5},
{'l', 0x00000000, 0xc22338b2},
{'l', 0xc110f768, 0x00000000},
{'l', 0xb5000000, 0xc1198ea0},
{'l', 0x4110f768, 0x00000000},
{'l', 0x35800000, 0xc1aabcfc},
{'l', 0x4146a85f, 0x00000000},
{'@', 0x00000075, 0x0000571d},/*        u        x-advance: 87.113281 */
{'M', 0x413ad87b, 0xc1ed50c0},
{'4', 0xfe940000, 0x00000062},
{'l', 0x00000000, 0x4234225d},
{'q', 0x00000000, 0x412abcfe},
{0, 0x40852784, 0x41805278},
{'q', 0x40852780, 0x40a9aa18},
{0, 0x4147bb44, 0x40a9aa18},
{'q', 0x41200000, 0x00000000},
{0, 0x417c5958, 0xc0cc06de},
{'9', 0xffcd002e, 0xff75002e},
{'l', 0x00000000, 0xc22a7845},
{'l', 0x41459574, 0x00000000},
{'4', 0x02590000, 0x0000ff9e},
{'l', 0x00000000, 0xc138b2af},
{'8', 0x51ad36dd, 0x1a921ad1},
{'q', 0xc14f3f94, 0x34000000},
{0, 0xc19d50c1, 0xc100dbeb},
{'9', 0xffc0ffcb, 0xff44ffcb},
{'m', 0x41f89732, 0xc23d4302},
{'l', 0x00000000, 0x00000000},
{'@', 0x00000076, 0x00005157},/*        v        x-advance: 81.339844 */
{'M', 0x408301b8, 0xc29655e8},
{'l', 0x4151655e, 0x00000000},
{'l', 0x41bbeb61, 0x427c5958},
{'l', 0x41bbeb62, 0xc27c5958},
{'l', 0x41516560, 0x00000000},
{'l', 0xc1e180dc, 0x429655e8},
{'l', 0xc1863a6a, 0x00000000},
{'l', 0xc1e180dc, 0xc29655e8},
{'@', 0x00000077, 0x0000706a},/*        w        x-advance: 112.414062 */
{'M', 0x40b8b2af, 0xc29655e8},
{'l', 0x4145957a, 0x00000000},
{'l', 0x4176fad6, 0x426aa181},
{'l', 0x4175e7f4, 0xc26aa181},
{'l', 0x41690528, 0x00000000},
{'l', 0x4176fad4, 0x426aa181},
{'l', 0x4175e7f8, 0xc26aa181},
{'l', 0x41459578, 0x00000000},
{'l', 0xc19d50c0, 0x429655e8},
{'l', 0xc1690528, 0x00000000},
{'l', 0xc1816560, 0xc2767165},
{'l', 0xc181eed0, 0x42767165},
{'l', 0xc1690528, 0x00000000},
{'l', 0xc19d50c0, 0xc29655e8},
{'@', 0x00000078, 0x00005157},/*        x        x-advance: 81.339844 */
{'M', 0x4296df5b, 0xc29655e8},
{'l', 0xc1d9731e, 0x42124f09},
{'l', 0x41e4b98e, 0x421a5cc7},
{'l', 0xc1690524, 0x00000000},
{'l', 0xc1af0898, 0xc1ec3dda},
{'l', 0xc1af0897, 0x41ec3dda},
{'l', 0xc1690527, 0x00000000},
{'l', 0x41e98e9a, 0xc21d50c0},
{'l', 0xc1d5b0f7, 0xc20f5b10},
{'l', 0x41690526, 0x00000000},
{'l', 0x419f768e, 0x41d63a6c},
{'l', 0x419f768c, 0xc1d63a6c},
{'l', 0x41690528, 0x00000000},
{'@', 0x00000079, 0x00005157},/*        y        x-advance: 81.339844 */
{'M', 0x4230e9aa, 0x40df5b0f},
{'q', 0xc0a78450, 0x4156c3dc},
{0, 0xc12338b4, 0x418c225c},
{'9', 0x0020ffd9, 0x0020ff96},
{'4', 0x0000ffb2, 0xffae0000},
{'l', 0x40e7f242, 0x00000000},
{'8', 0xed3f0028, 0xa531ed16},
{'l', 0x400dbeb0, 0xc0b46716},
{'l', 0xc1f338b2, 0xc293eb62},
{'l', 0x4151655e, 0x00000000},
{'l', 0x41bbeb61, 0x426b2af4},
{'l', 0x41bbeb62, 0xc26b2af4},
{'l', 0x41516560, 0x00000000},
{'l', 0xc204149e, 0x42a44b99},
{'@', 0x0000007a, 0x00004825},/*        z        x-advance: 72.144531 */
{'M', 0x40f2af3f, 0xc29655e8},
{'l', 0x426aa180, 0x00000000},
{'l', 0x00000000, 0x41346718},
{'l', 0xc239c595, 0x42581b7d},
{'l', 0x4239c595, 0x35800000},
{'l', 0x00000000, 0x411dda33},
{'l', 0xc271579f, 0x00000000},
{'l', 0x00000000, 0xc1346716},
{'l', 0x4239c595, 0xc2581b7c},
{'l', 0xc2330f76, 0x00000000},
{'l', 0xb5000000, 0xc11dda38},
{'@', 0x0000007b, 0x00005773},/*        {        x-advance: 87.449219 */
{'M', 0x428c8973, 0x414c06df},
{'4', 0x004d0000, 0x0000ffdf},
{'q', 0xc185b0f8, 0x00000000},
{0, 0xc1b35432, 0xc09eed1c},
{'9', 0xffd9ffd3, 0xff62ffd3},
{'l', 0x00000000, 0xc1805278},
{'q', 0x00000000, 0xc12225cc},
{0, 0xc067f240, 0xc1606df6},
{'9', 0xffe1ffe4, 0xffe1ff97},
{'4', 0x0000ffe0, 0xffb40000},
{'l', 0x408301b8, 0x00000000},
{'8', 0xe269004c, 0x911ce11c},
{'l', 0x00000000, 0xc180dbec},
{'q', 0x00000000, 0xc16d50c0},
{0, 0x40b46710, 0xc19dda30},
{'9', 0xffd9002d, 0xffd900b3},
{'4', 0x00000021, 0x004c0000},
{'l', 0xc0920a50, 0x00000000},
{'8', 0x179e00b5, 0x63e917e9},
{'l', 0x00000000, 0x41852786},
{'q', 0x00000000, 0x41289730},
{0, 0xc0459580, 0x4174d50c},
{'8', 0x33ad26e8, 0x34530e3b},
{'9', 0x00260018, 0x00790018},
{'l', 0x00000000, 0x41852785},
{'8', 0x63174b00, 0x17621717},
{'l', 0x40920a50, 0x00000000},
{'@', 0x0000007c, 0x00002e4f},/*        |        x-advance: 46.308594 */
{'M', 0x41e6df5b, 0xc2d2112e},
{'l', 0x00000000, 0x4309731d},
{'l', 0xc1368ce4, 0x00000000},
{'l', 0x00000000, 0xc309731d},
{'l', 0x41368ce4, 0x00000000},
{'@', 0x0000007d, 0x00005773},/*        }        x-advance: 87.449219 */
{'M', 0x4189731d, 0x414c06df},
{'l', 0x409655e8, 0x00000000},
{'8', 0xe961004b, 0x9d17e917},
{'l', 0x00000000, 0xc1852784},
{'q', 0x00000000, 0xc127844a},
{0, 0x404149e0, 0xc173c224},
{'8', 0xcc53da18, 0xcdadf3c5},
{'9', 0xffdaffe8, 0xff86ffe8},
{'l', 0x00000000, 0xc1852786},
{'8', 0x9de9b400, 0xe99fe9ea},
{'4', 0x0000ffdb, 0xffb40000},
{'l', 0x40874d50, 0x00000000},
{'q', 0x4185b0f7, 0x00000000},
{0, 0x41b24149, 0x409eed20},
{'9', 0x0027002d, 0x009d002d},
{'l', 0x00000000, 0x4180dbec},
{'8', 0x6f1c5000, 0x1e691e1c},
{'4', 0x00000021, 0x004c0000},
{'l', 0xc0852780, 0x00000000},
{'q', 0xc1187bb8, 0x00000000},
{0, 0xc1527848, 0x407920a0},
{'9', 0x001fffe4, 0x0070ffe4},
{'l', 0x00000000, 0x41805278},
{'q', 0x00000000, 0x416d50c0},
{0, 0xc0b46718, 0x419e63a6},
{'9', 0x0027ffd4, 0x0027ff4e},
{'l', 0xc0874d50, 0x00000000},
{'l', 0x00000000, 0xc11aa181},
{'@', 0x0000007e, 0x0000732a},/*        ~        x-advance: 115.164062 */
{'M', 0x42c93543, 0xc25b5430},
{'l', 0x00000000, 0x413f2414},
{'8', 0x3c982ac8, 0x129d12d1},
{'q', 0xc0ec3dd0, 0x00000000},
{0, 0xc189731c, 0xc07d6c40},
{'8', 0xfdf8fefb, 0xfcf5fffd},
{'q', 0xc1267168, 0xc0852780},
{0, 0xc185b0f8, 0xc0852780},
{'8', 0x14a300d1, 0x409e14d2},
{'l', 0x00000000, 0xc13f2414},
{'8', 0xc468d638, 0xee64ee30},
{'q', 0x40ec3dd8, 0x00000000},
{0, 0x4189fc90, 0x4080dbe8},
{'8', 0x03080205, 0x040b0104},
{'q', 0x41267164, 0x40852780},
{0, 0x4185b0f6, 0x40852780},
{'8', 0xec5b002e, 0xbf64ec2d},
};
#define CTX_FONT_ascii 1
#endif
#endif //_CTX_INTERNAL_FONT_
#ifndef __CTX_LIST__
#define __CTX_LIST__

#if !__COSMOPOLITAN__
#include <stdlib.h>
#endif

/* The whole ctx_list implementation is in the header and will be inlined
 * wherever it is used.
 */

static inline void *ctx_calloc (size_t size, size_t count)
{
  size_t byte_size = size * count;
  char *ret = (char*)malloc (byte_size);
  for (size_t i = 0; i < byte_size; i++)
     ret[i] = 0;
  return ret;
}

typedef struct _CtxList CtxList;
struct _CtxList {
  void *data;
  CtxList *next;
  void (*freefunc)(void *data, void *freefunc_data);
  void *freefunc_data;
};

static inline void ctx_list_prepend_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_= (CtxList*)ctx_calloc (sizeof (CtxList), 1);
  new_->next = *list;
  new_->data=data;
  new_->freefunc=freefunc;
  new_->freefunc_data = freefunc_data;
  *list = new_;
}

static inline int ctx_list_length (CtxList *list)
{
  int length = 0;
  CtxList *l;
  for (l = list; l; l = l->next, length++);
  return length;
}

static inline void ctx_list_prepend (CtxList **list, void *data)
{
  CtxList *new_ = (CtxList*) ctx_calloc (sizeof (CtxList), 1);
  new_->next= *list;
  new_->data=data;
  *list = new_;
}

static inline CtxList *ctx_list_nth (CtxList *list, int no)
{
  while (no-- && list)
    { list = list->next; }
  return list;
}

static inline void *ctx_list_nth_data (CtxList *list, int no)
{
  CtxList *l = ctx_list_nth (list, no);
  if (l)
    return l->data;
  return NULL;
}


static inline void
ctx_list_insert_before (CtxList **list, CtxList *sibling,
                       void *data)
{
  if (*list == NULL || *list == sibling)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      CtxList *prev = NULL;
      for (CtxList *l = *list; l; l=l->next)
        {
          if (l == sibling)
            { break; }
          prev = l;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)ctx_calloc (sizeof (CtxList), 1);
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
        }
    }
}

static inline void ctx_list_remove_link (CtxList **list, CtxList *link)
{
  CtxList *iter, *prev = NULL;
  if ((*list) == link)
    {
      prev = (*list)->next;
      *list = prev;
      link->next = NULL;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter == link)
      {
        if (prev)
          prev->next = iter->next;
        link->next = NULL;
        return;
      }
    else
      prev = iter;
}

static inline void ctx_list_remove (CtxList **list, void *data)
{
  CtxList *iter, *prev = NULL;
  if ((*list)->data == data)
    {
      if ((*list)->freefunc)
        (*list)->freefunc ((*list)->data, (*list)->freefunc_data);
      prev = (*list)->next;
      free (*list);
      *list = prev;
      return;
    }
  for (iter = *list; iter; iter = iter->next)
    if (iter->data == data)
      {
        if (iter->freefunc)
          iter->freefunc (iter->data, iter->freefunc_data);
        prev->next = iter->next;
        free (iter);
        break;
      }
    else
      prev = iter;
}

static inline void ctx_list_free (CtxList **list)
{
  while (*list)
    ctx_list_remove (list, (*list)->data);
}

static inline void
ctx_list_reverse (CtxList **list)
{
  CtxList *new_ = NULL;
  CtxList *l;
  for (l = *list; l; l=l->next)
    ctx_list_prepend (&new_, l->data);
  ctx_list_free (list);
  *list = new_;
}

static inline void *ctx_list_last (CtxList *list)
{
  if (list)
    {
      CtxList *last;
      for (last = list; last->next; last=last->next);
      return last->data;
    }
  return NULL;
}

static inline void ctx_list_concat (CtxList **list, CtxList *list_b)
{
  if (*list)
    {
      CtxList *last;
      for (last = *list; last->next; last=last->next);
      last->next = list_b;
      return;
    }
  *list = list_b;
}

static inline void ctx_list_append_full (CtxList **list, void *data,
    void (*freefunc)(void *data, void *freefunc_data),
    void *freefunc_data)
{
  CtxList *new_ = (CtxList*) ctx_calloc (sizeof (CtxList), 1);
  new_->data=data;
  new_->freefunc = freefunc;
  new_->freefunc_data = freefunc_data;
  ctx_list_concat (list, new_);
}

static inline void ctx_list_append (CtxList **list, void *data)
{
  ctx_list_append_full (list, data, NULL, NULL);
}

static inline void
ctx_list_insert_at (CtxList **list,
                    int       no,
                    void     *data)
{
  if (*list == NULL || no == 0)
    {
      ctx_list_prepend (list, data);
    }
  else
    {
      int pos = 0;
      CtxList *prev = NULL;
      CtxList *sibling = NULL;
      for (CtxList *l = *list; l && pos < no; l=l->next)
        {
          prev = sibling;
          sibling = l;
          pos ++;
        }
      if (prev)
        {
          CtxList *new_ = (CtxList*)ctx_calloc (sizeof (CtxList), 1);
          new_->next = sibling;
          new_->data = data;
          prev->next=new_;
          return;
        }
      ctx_list_append (list, data);
    }
}

static CtxList*
ctx_list_merge_sorted (CtxList* list1,
                       CtxList* list2,
    int(*compare)(const void *a, const void *b, void *userdata), void *userdata
)
{
  if (list1 == NULL)
     return(list2);
  else if (list2==NULL)
     return(list1);

  if (compare (list1->data, list2->data, userdata) >= 0)
  {
    list1->next = ctx_list_merge_sorted (list1->next,list2, compare, userdata);
    /*list1->next->prev = list1;
      list1->prev = NULL;*/
    return list1;
  }
  else
  {
    list2->next = ctx_list_merge_sorted (list1,list2->next, compare, userdata);
    /*list2->next->prev = list2;
      list2->prev = NULL;*/
    return list2;
  }
}

static void
ctx_list_split_half (CtxList*  head,
                     CtxList** list1,
                     CtxList** list2)
{
  CtxList* fast;
  CtxList* slow;
  if (head==NULL || head->next==NULL)
  {
    *list1 = head;
    *list2 = NULL;
  }
  else
  {
    slow = head;
    fast = head->next;

    while (fast != NULL)
    {
      fast = fast->next;
      if (fast != NULL)
      {
        slow = slow->next;
        fast = fast->next;
      }
    }

    *list1 = head;
    *list2 = slow->next;
    slow->next = NULL;
  }
}

static inline void ctx_list_sort (CtxList **head,
    int(*compare)(const void *a, const void *b, void *userdata),
    void *userdata)
{
  CtxList* list1;
  CtxList* list2;

  /* Base case -- length 0 or 1 */
  if ((*head == NULL) || ((*head)->next == NULL))
  {
    return;
  }

  ctx_list_split_half (*head, &list1, &list2);
  ctx_list_sort (&list1, compare, userdata);
  ctx_list_sort (&list2, compare, userdata);
  *head = ctx_list_merge_sorted (list1, list2, compare, userdata);
}

static inline void ctx_list_insert_sorted (CtxList **list,
                                           void     *item,
    int(*compare)(const void *a, const void *b, void *userdata),
                                           void     *userdata)
{
  ctx_list_prepend (list, item);
  ctx_list_sort (list, compare, userdata);
}


static inline CtxList *ctx_list_find_custom (CtxList *list,
                                         void    *needle,
                                         int(*compare)(const void *a, const void *b),
                                         void *userdata)
{
  CtxList *l;
  for (l = list; l; l = l->next)
  {
    if (compare (l->data, needle) == 0)
      return l;
  }
  return NULL;
}

#endif

/* definitions that determine which features are included and their settings,
 * for particular platforms - in particular microcontrollers ctx might need
 * tuning for different quality/performance/resource constraints.
 *
 * the way to configure ctx is to set these defines, before both including it
 * as a header and in the file where CTX_IMPLEMENTATION is set to include the
 * implementation for different featureset and runtime settings.
 *
 */

/* whether the font rendering happens in backend or front-end of API, the
 * option is used set to 0 by the tool that converts ttf fonts to ctx internal
 * representation - both should be possible so that this tool can be made
 * into a TTF/OTF font import at runtime (perhaps even with live subsetting).
 */
#ifndef CTX_BACKEND_TEXT
#define CTX_BACKEND_TEXT 1
#endif


#define CTX_RASTERIZER_AA_SLOPE_LIMIT3    (13424/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT5    (20425/CTX_SUBDIV/15)
#define CTX_RASTERIZER_AA_SLOPE_LIMIT15   (50425/CTX_SUBDIV/15)

/* subpixel-aa coordinates used in BITPACKing of drawlist
 *
 * powers of 2 is faster
 */
#ifndef CTX_SUBDIV
#define CTX_SUBDIV   8  //  max framebufer width 4095
//#define CTX_SUBDIV    10  //  max framebufer width 3250
//#define CTX_SUBDIV  16  //  max framebufer width 2047
//#define CTX_SUBDIV  24  //  max framebufer width 1350
//#define CTX_SUBDIV  32  //  max framebufer width 1023
#endif


// 8    12 68 40 24
// 16   12 68 40 24
/* scale-factor for font outlines prior to bit quantization by CTX_SUBDIV
 *
 * changing this also changes font file format - the value should be baked
 * into the ctxf files making them less dependent on the ctx used to
 * generate them
 */
#define CTX_BAKE_FONT_SIZE    160

/* pack some linetos/curvetos/movetos into denser drawlist instructions,
 * permitting more vectors to be stored in the same space, experimental
 * feature with added overhead.
 */
#ifndef CTX_BITPACK
#define CTX_BITPACK           1
#endif

/* whether we have a shape-cache where we keep pre-rasterized bitmaps of
 * commonly occuring small shapes, disabled by default since it has some
 * glitches (and potential hangs with multi threading).
 */
#ifndef CTX_SHAPE_CACHE
#define CTX_SHAPE_CACHE        0
#endif

/* size (in pixels, w*h) that we cache rasterization for
 */
#ifndef CTX_SHAPE_CACHE_DIM
#define CTX_SHAPE_CACHE_DIM      (16*16)
#endif

#ifndef CTX_SHAPE_CACHE_MAX_DIM
#define CTX_SHAPE_CACHE_MAX_DIM  32
#endif

/* maximum number of entries in shape cache
 */
#ifndef CTX_SHAPE_CACHE_ENTRIES
#define CTX_SHAPE_CACHE_ENTRIES  160
#endif


#ifndef CTX_PARSER_FIXED_TEMP
#define CTX_PARSER_FIXED_TEMP 0
         // when 1  CTX_PARSER_MAXLEN is the fixed max stringlen
#endif   // and no allocations happens beyond creating the parser,
         // when 0 the scratchbuf for parsing is a separate dynamically
         // growing buffer, that maxes out at CTX_PARSER_MAXLEN
         //
#ifndef CTX_PARSER_MAXLEN
#if CTX_PARSER_FIXED_TEMP
#define CTX_PARSER_MAXLEN  1024*128        // This is the maximum texture/string size supported
#else
#define CTX_PARSER_MAXLEN  1024*1024*16    // 16mb
#endif
#endif

#ifndef CTX_COMPOSITING_GROUPS
#define CTX_COMPOSITING_GROUPS   1
#endif

/* maximum nesting level of compositing groups
 */
#ifndef CTX_GROUP_MAX
#define CTX_GROUP_MAX             8
#endif

#ifndef CTX_ENABLE_CLIP
#define CTX_ENABLE_CLIP           1
#endif

/* use a 1bit clip buffer, saving RAM on microcontrollers, other rendering
 * will still be antialiased.
 */
#ifndef CTX_1BIT_CLIP
#define CTX_1BIT_CLIP             0
#endif


#ifndef CTX_ENABLE_SHADOW_BLUR
#define CTX_ENABLE_SHADOW_BLUR    1
#endif

#ifndef CTX_GRADIENTS
#define CTX_GRADIENTS             1
#endif

/* some optinal micro-optimizations that are known to increase code size
 */
#ifndef CTX_SORTING_NETWORKS
#define CTX_SORTING_NETWORKS         1
#endif

#ifndef CTX_ALIGNED_STRUCTS
#define CTX_ALIGNED_STRUCTS       1
#endif

#ifndef CTX_GRADIENT_CACHE
#define CTX_GRADIENT_CACHE        1
#endif

#ifndef CTX_FONTS_FROM_FILE
#define CTX_FONTS_FROM_FILE 1
#endif

#ifndef CTX_FORMATTER
#define CTX_FORMATTER       1
#endif

#ifndef CTX_PARSER
#define CTX_PARSER          1
#endif

#ifndef CTX_CURRENT_PATH
#define CTX_CURRENT_PATH    1
#endif

#ifndef CTX_XML
#define CTX_XML             1
#endif

/* when ctx_math is defined, which it is by default, we use ctx' own
 * implementations of math functions, instead of relying on math.h
 * the possible inlining gives us a slight speed-gain, and on
 * embedded platforms guarantees that we do not do double precision
 * math.
 */
#ifndef CTX_MATH
#define CTX_MATH           1  // use internal fast math for sqrt,sin,cos,atan2f etc.
#endif

#define ctx_log(fmt, ...)
//#define ctx_log(str, a...) fprintf(stderr, str, ##a)

/* the initial journal size - for both rasterizer
 * edgelist and drawlist.
 */
#ifndef CTX_MIN_JOURNAL_SIZE
#define CTX_MIN_JOURNAL_SIZE      512
#endif

/* The maximum size we permit the drawlist to grow to,
 * the memory used is this number * 9, where 9 is sizeof(CtxEntry)
 */
#ifndef CTX_MAX_JOURNAL_SIZE
//#define CTX_MAX_JOURNAL_SIZE   CTX_MIN_JOURNAL_SIZE
#define CTX_MAX_JOURNAL_SIZE 1024*1024*16
#endif

#ifndef CTX_DRAWLIST_STATIC
#define CTX_DRAWLIST_STATIC  0
#endif

#ifndef CTX_MIN_EDGE_LIST_SIZE
#define CTX_MIN_EDGE_LIST_SIZE   1024
#endif

#ifndef CTX_RASTERIZER_AA
#define CTX_RASTERIZER_AA 15   // vertical-AA of CTX_ANTIALIAS_DEFAULT
#endif

/* The maximum complexity of a single path
 */
#ifndef CTX_MAX_EDGE_LIST_SIZE
#define CTX_MAX_EDGE_LIST_SIZE  CTX_MIN_EDGE_LIST_SIZE
#endif

#ifndef CTX_STRINGPOOL_SIZE
  // XXX should be possible to make zero and disappear when codepaths not in use
  //     to save size, for card10 this is defined as a low number (some text
  //     properties still make use of it)
  //     
  //     for desktop-use this should be fully dynamic, possibly
  //     with chained pools, gradients are stored here.
#define CTX_STRINGPOOL_SIZE     1000 //
#endif

/* whether we dither or not for gradients
 */
#ifndef CTX_DITHER
#define CTX_DITHER 1
#endif

/*  only source-over clear and copy will work, the API still
 *  through - but the renderer is limited, for use to measure
 *  size and possibly in severely constrained ROMs.
 */
#ifndef CTX_BLENDING_AND_COMPOSITING
#define CTX_BLENDING_AND_COMPOSITING 1
#endif

/*  this forces the inlining of some performance
 *  critical paths.
 */
#ifndef CTX_FORCE_INLINES
#define CTX_FORCE_INLINES               1
#endif

/* create one-off inlined inner loop for normal blend mode
 */
#ifndef CTX_INLINED_NORMAL     
#define CTX_INLINED_NORMAL      1
#endif

#ifndef CTX_INLINED_GRADIENTS
#define CTX_INLINED_GRADIENTS   1
#endif

#ifndef CTX_BRAILLE_TEXT
#define CTX_BRAILLE_TEXT        0
#endif

/* including immintrin.h triggers building of AVX2 code paths, if - like
 * sometimes when including SDL one does want it at all do a
 * #define CTX_AVX2 0  before including ctx.h for implementation.
 */
#ifndef CTX_AVX2
#ifdef _IMMINTRIN_H_INCLUDED
#define CTX_AVX2         1
#else
#define CTX_AVX2         0
#endif
#endif

/* Build code paths for grayscale rasterization, normally this is handled
 * by the RGBA8 codepaths; on microcontrollers with eink this might be
 * a better option.
 */
#ifndef CTX_NATIVE_GRAYA8
#define CTX_NATIVE_GRAYA8       0
#endif

/* enable CMYK rasterization targets
 */
#ifndef CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK         1
#endif

/* enable color management, slightly increases CtxColor struct size, can
 * be disabled for microcontrollers.
 */
#ifndef CTX_ENABLE_CM
#define CTX_ENABLE_CM           1
#endif

#ifndef CTX_EVENTS
#define CTX_EVENTS              1
#endif

#ifndef CTX_LIMIT_FORMATS
#define CTX_LIMIT_FORMATS       0
#endif

#ifndef CTX_ENABLE_FLOAT
#define CTX_ENABLE_FLOAT 0
#endif

/* by default ctx includes all pixel formats, on microcontrollers
 * it can be useful to slim down code and runtime size by only
 * defining the used formats, set CTX_LIMIT_FORMATS to 1, and
 * manually add CTX_ENABLE_ flags for each of them.
 */
#if CTX_LIMIT_FORMATS
#else

#define CTX_ENABLE_GRAY1                1
#define CTX_ENABLE_GRAY2                1
#define CTX_ENABLE_GRAY4                1
#define CTX_ENABLE_GRAY8                1
#define CTX_ENABLE_GRAYA8               1
#define CTX_ENABLE_GRAYF                1
#define CTX_ENABLE_GRAYAF               1

#define CTX_ENABLE_RGB8                 1
#define CTX_ENABLE_RGBA8                1
#define CTX_ENABLE_BGRA8                1
#define CTX_ENABLE_RGB332               1
#define CTX_ENABLE_RGB565               1
#define CTX_ENABLE_RGB565_BYTESWAPPED   1
#define CTX_ENABLE_RGBAF                1
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT                1

#if CTX_ENABLE_CMYK
#define CTX_ENABLE_CMYK8                1
#define CTX_ENABLE_CMYKA8               1
#define CTX_ENABLE_CMYKAF               1
#endif
#endif

#ifndef CTX_RGB565_ALPHA
#define CTX_RGB565_ALPHA                0   // when enabled pure purple is transparent,
                                            // for a ~15% overall performance hit
#endif

#ifndef CTX_RGB332_ALPHA
#define CTX_RGB332_ALPHA                0   // when enabled pure purple is transparent,
                                            // for a ~15% overall performance hit
#endif

/* by including ctx-font-regular.h, or ctx-font-mono.h the
 * built-in fonts using ctx drawlist encoding is enabled
 */
#if CTX_FONT_regular || CTX_FONT_mono || CTX_FONT_bold \
  || CTX_FONT_italic || CTX_FONT_sans || CTX_FONT_serif \
  || CTX_FONT_ascii
#ifndef CTX_FONT_ENGINE_CTX
#define CTX_FONT_ENGINE_CTX        1
#endif
#endif

#ifndef CTX_FONT_ENGINE_CTX_FS
#define CTX_FONT_ENGINE_CTX_FS 0
#endif

/* If stb_strutype.h is included before ctx.h add integration code for runtime loading
 * of opentype fonts.
 */
#ifdef __STB_INCLUDE_STB_TRUETYPE_H__
#ifndef CTX_FONT_ENGINE_STB
#define CTX_FONT_ENGINE_STB        1
#endif
#else
#define CTX_FONT_ENGINE_STB        0
#endif

#ifdef _BABL_H
#define CTX_BABL 1
#else
#define CTX_BABL 0
#endif


/* force add format if we have shape cache */
#if CTX_SHAPE_CACHE
#ifdef CTX_ENABLE_GRAY8
#undef CTX_ENABLE_GRAY8
#endif
#define CTX_ENABLE_GRAY8  1
#endif

/* include the bitpack packer, can be opted out of to decrease code size
 */
#ifndef CTX_BITPACK_PACKER
#define CTX_BITPACK_PACKER 0
#endif

/* enable RGBA8 intermediate format for
 *the indirectly implemented pixel-formats.
 */
#if CTX_ENABLE_GRAY1 | CTX_ENABLE_GRAY2 | CTX_ENABLE_GRAY4 | CTX_ENABLE_RGB565 | CTX_ENABLE_RGB565_BYTESWAPPED | CTX_ENABLE_RGB8 | CTX_ENABLE_RGB332

  #ifdef CTX_ENABLE_RGBA8
    #undef CTX_ENABLE_RGBA8
  #endif
  #define CTX_ENABLE_RGBA8  1
#endif

#ifdef CTX_ENABLE_CMYKF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_GRAYF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_GRAYAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_RGBAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_CMYKAF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif

#ifdef CTX_ENABLE_CMYKF
#ifdef CTX_ENABLE_FLOAT
#undef CTX_ENABLE_FLOAT
#endif
#define CTX_ENABLE_FLOAT 1
#endif


/* enable cmykf which is cmyk intermediate format
 */
#ifdef CTX_ENABLE_CMYK8
#ifdef CTX_ENABLE_CMYKF
#undef CTX_ENABLE_CMYKF
#endif
#define CTX_ENABLE_CMYKF  1
#endif
#ifdef CTX_ENABLE_CMYKA8
#ifdef CTX_ENABLE_CMYKF
#undef CTX_ENABLE_CMYKF
#endif
#define CTX_ENABLE_CMYKF  1
#endif

#ifdef CTX_ENABLE_CMYKF8
#ifdef CTX_ENABLE_CMYK
#undef CTX_ENABLE_CMYK
#endif
#define CTX_ENABLE_CMYK   1
#endif

#define CTX_PI                              3.141592653589793f
#ifndef CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS  120
#endif

#ifndef CTX_MAX_FONTS
#define CTX_MAX_FONTS            3
#endif

#ifndef CTX_MAX_STATES
#define CTX_MAX_STATES           10
#endif

#ifndef CTX_MAX_EDGES
#define CTX_MAX_EDGES            257
#endif

#ifndef CTX_MAX_LINGERING_EDGES
#define CTX_MAX_LINGERING_EDGES  64
#endif


#ifndef CTX_MAX_PENDING
#define CTX_MAX_PENDING          128
#endif

#ifndef CTX_MAX_TEXTURES
#define CTX_MAX_TEXTURES         16
#endif

#ifndef CTX_HASH_ROWS
#define CTX_HASH_ROWS            8
#endif
#ifndef CTX_HASH_COLS
#define CTX_HASH_COLS            8
#endif

#ifndef CTX_MAX_THREADS
#define CTX_MAX_THREADS          8 // runtime is max of cores/2 and this
#endif



#define CTX_RASTERIZER_EDGE_MULTIPLIER  1024

#ifndef CTX_COMPOSITE_SUFFIX
#define CTX_COMPOSITE_SUFFIX(symbol)     symbol##_default
#endif

#ifndef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 0
#else
#undef CTX_IMPLEMENTATION
#define CTX_IMPLEMENTATION 1
#endif


#ifdef CTX_RASTERIZER
#if CTX_RASTERIZER==0
#if CTX_SDL || CTX_FB
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#else
#undef CTX_RASTERIZER
#define CTX_RASTERIZER 1
#endif
#endif

#if CTX_RASTERIZER
#ifndef CTX_COMPOSITE
#define CTX_COMPOSITE 1
#endif
#else
#ifndef CTX_COMPOSITE
#define CTX_COMPOSITE 0
#endif
#endif

#ifndef CTX_GRADIENT_CACHE_ELEMENTS
#define CTX_GRADIENT_CACHE_ELEMENTS 256
#endif

#ifndef CTX_PARSER_MAX_ARGS
#define CTX_PARSER_MAX_ARGS 20
#endif


#ifndef CTX_SCREENSHOT
#define CTX_SCREENSHOT 0
#endif

#ifndef CTX_ALSA_AUDIO
#define CTX_ALSA_AUDIO 0
#endif

#if NO_ALSA
#undef CTX_ALSA_AUDIO
#define CTX_ALSA_AUDIO 0
#endif

#ifndef CTX_AUDIO
#define CTX_AUDIO 1
#endif

#ifndef CTX_TILED
#if CTX_SDL || CTX_FB
#define CTX_TILED 1
#else
#define CTX_TILED 0
#endif
#endif

#if CTX_FB
#define CTX_THREADS 1
#else
#define CTX_THREADS 0
#endif


 /* Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 */

#if CTX_FORMATTER

/* returns the maximum string length including terminating \0 */
int ctx_a85enc_len (int input_length);
int ctx_a85enc (const void *srcp, char *dst, int count);

#if CTX_PARSER

int ctx_a85dec (const char *src, char *dst, int count);
int ctx_a85len (const char *src, int count);
#endif

#endif
#ifndef __CTX_EXTRA_H
#define __CTX_EXTRA_H


#define CTX_CLAMP(val,min,max) ((val)<(min)?(min):(val)>(max)?(max):(val))
static inline int   ctx_mini (int a, int b)     { if (a < b) return a; return b; }
static inline float ctx_minf (float a, float b) { if (a < b) return a; return b; }
static inline int   ctx_maxi (int a, int b)     { if (a > b) return a; return b; }
static inline float ctx_maxf (float a, float b) { if (a > b) return a; return b; }


typedef enum CtxOutputmode
{
  CTX_OUTPUT_MODE_QUARTER,
  CTX_OUTPUT_MODE_BRAILLE,
  CTX_OUTPUT_MODE_SIXELS,
  CTX_OUTPUT_MODE_GRAYS,
  CTX_OUTPUT_MODE_CTX,
  CTX_OUTPUT_MODE_CTX_COMPACT,
  CTX_OUTPUT_MODE_UI
} CtxOutputmode;





#if CTX_FORCE_INLINES
#define CTX_INLINE inline __attribute__((always_inline))
#else
#define CTX_INLINE inline
#endif

static inline float ctx_pow2 (float a) { return a * a; }
#if CTX_MATH

static inline float
ctx_fabsf (float x)
{
  union
  {
    float f;
    uint32_t i;
  } u = { x };
  u.i &= 0x7fffffff;
  return u.f;
}

static inline float
ctx_invsqrtf (float x)
{
  void *foo = &x;
  float xhalf = 0.5f * x;
  int i=* (int *) foo;
  void *bar = &i;
  i = 0x5f3759df - (i >> 1);
  x = * (float *) bar;
  x *= (1.5f - xhalf * x * x);
  x *= (1.5f - xhalf * x * x); //repeating Newton-Raphson step for higher precision
  return x;
}

CTX_INLINE static float
ctx_sinf (float x)
{
  if (x < -CTX_PI * 2)
    {
      x = -x;
      long ix = x / (CTX_PI * 2);
      x = x - ix * CTX_PI * 2;
      x = -x;
    }
  if (x < -CTX_PI * 1000)
  {
    x = -0.5;
  }
  if (x > CTX_PI * 1000)
  {
          // really large numbers tend to cause practically inifinite
          // loops since the > CTX_PI * 2 seemingly fails
    x = 0.5;
  }
  if (x > CTX_PI * 2)
    { 
      long ix = x / (CTX_PI * 2);
      x = x - (ix * CTX_PI * 2);
    }
  while (x < -CTX_PI)
    { x += CTX_PI * 2; }
  while (x > CTX_PI)
    { x -= CTX_PI * 2; }

  /* source : http://mooooo.ooo/chebyshev-sine-approximation/ */
  float coeffs[]=
  {
    -0.10132118f,           // x
      0.0066208798f,         // x^3
      -0.00017350505f,        // x^5
      0.0000025222919f,      // x^7
      -0.000000023317787f,    // x^9
      0.00000000013291342f
    }; // x^11
  float x2 = x*x;
  float p11 = coeffs[5];
  float p9  = p11*x2 + coeffs[4];
  float p7  = p9*x2  + coeffs[3];
  float p5  = p7*x2  + coeffs[2];
  float p3  = p5*x2  + coeffs[1];
  float p1  = p3*x2  + coeffs[0];
  return (x - CTX_PI + 0.00000008742278f) *
         (x + CTX_PI - 0.00000008742278f) * p1 * x;
}

static inline float ctx_atan2f (float y, float x)
{
  float atan, z;
  if ( x == 0.0f )
    {
      if ( y > 0.0f )
        { return CTX_PI/2; }
      if ( y == 0.0f )
        { return 0.0f; }
      return -CTX_PI/2;
    }
  z = y/x;
  if ( ctx_fabsf ( z ) < 1.0f )
    {
      atan = z/ (1.0f + 0.28f*z*z);
      if (x < 0.0f)
        {
          if ( y < 0.0f )
            { return atan - CTX_PI; }
          return atan + CTX_PI;
        }
    }
  else
    {
      atan = CTX_PI/2 - z/ (z*z + 0.28f);
      if ( y < 0.0f ) { return atan - CTX_PI; }
    }
  return atan;
}

CTX_INLINE static float ctx_sqrtf (float a)
{
  return 1.0f/ctx_invsqrtf (a);
}

CTX_INLINE static float ctx_hypotf (float a, float b)
{
  return ctx_sqrtf (ctx_pow2 (a)+ctx_pow2 (b) );
}

static inline float ctx_atanf (float a)
{
  return ctx_atan2f ( (a), 1.0f);
}

static inline float ctx_asinf (float x)
{
  return ctx_atanf ( (x) * (ctx_invsqrtf (1.0f-ctx_pow2 (x) ) ) );
}

static inline float ctx_acosf (float x)
{
  return ctx_atanf ( (ctx_sqrtf (1.0f-ctx_pow2 (x) ) / (x) ) );
}

CTX_INLINE static float ctx_cosf (float a)
{
  return ctx_sinf ( (a) + CTX_PI/2.0f);
}

static inline float ctx_tanf (float a)
{
  return (ctx_cosf (a) /ctx_sinf (a) );
}
static inline float
ctx_floorf (float x)
{
  return (int)x; // XXX
}
static inline float
ctx_expf (float x)
{
  union { uint32_t i; float f; } v =
    { (1 << 23) * (x + 183.1395965) };
  return v.f;
}

/* define more trig based on having sqrt, sin and atan2 */

#else
#if !__COSMOPOLITAN__
#include <math.h>
#endif
static inline float ctx_fabsf (float x)           { return fabsf (x); }
static inline float ctx_floorf (float x)          { return floorf (x); }
static inline float ctx_sinf (float x)            { return sinf (x); }
static inline float ctx_atan2f (float y, float x) { return atan2f (y, x); }
static inline float ctx_hypotf (float a, float b) { return hypotf (a, b); }
static inline float ctx_acosf (float a)           { return acosf (a); }
static inline float ctx_cosf (float a)            { return cosf (a); }
static inline float ctx_tanf (float a)            { return tanf (a); }
static inline float ctx_expf (float p)            { return expf (p); }
static inline float ctx_sqrtf (float a)           { return sqrtf (a); }
#endif

static inline float _ctx_parse_float (const char *str, char **endptr)
{
  return strtod (str, endptr); /* XXX: , vs . problem in some locales */
}

const char *ctx_get_string (Ctx *ctx, uint64_t hash);
void ctx_set_string (Ctx *ctx, uint64_t hash, const char *value);
typedef struct _CtxColor CtxColor;
typedef struct _CtxBuffer CtxBuffer;

typedef struct _CtxMatrix     CtxMatrix;
struct
  _CtxMatrix
{
  float m[3][2];
};
void ctx_get_matrix (Ctx *ctx, CtxMatrix *matrix);

int ctx_color (Ctx *ctx, const char *string);
typedef struct _CtxState CtxState;
CtxColor *ctx_color_new ();
CtxState *ctx_get_state (Ctx *ctx);
void ctx_color_get_rgba (CtxState *state, CtxColor *color, float *out);
void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
void ctx_color_free (CtxColor *color);
void ctx_set_color (Ctx *ctx, uint64_t hash, CtxColor *color);
int  ctx_get_color (Ctx *ctx, uint64_t hash, CtxColor *color);
int  ctx_color_set_from_string (Ctx *ctx, CtxColor *color, const char *string);

int ctx_color_is_transparent (CtxColor *color);
int ctx_utf8_len (const unsigned char first_byte);

void ctx_user_to_device          (Ctx *ctx, float *x, float *y);
void ctx_user_to_device_distance (Ctx *ctx, float *x, float *y);
const char *ctx_utf8_skip (const char *s, int utf8_length);
void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix);
void ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y);
void ctx_matrix_invert (CtxMatrix *m);
void ctx_matrix_identity (CtxMatrix *matrix);
void ctx_matrix_scale (CtxMatrix *matrix, float x, float y);
void ctx_matrix_rotate (CtxMatrix *matrix, float angle);
void ctx_matrix_multiply (CtxMatrix       *result,
                          const CtxMatrix *t,
                          const CtxMatrix *s);
void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y);
int ctx_is_set_now (Ctx *ctx, uint64_t hash);
void ctx_set_size (Ctx *ctx, int width, int height);

static inline float ctx_matrix_get_scale (CtxMatrix *matrix)
{
   return ctx_maxf (ctx_maxf (ctx_fabsf (matrix->m[0][0]),
                         ctx_fabsf (matrix->m[0][1]) ),
               ctx_maxf (ctx_fabsf (matrix->m[1][0]),
                         ctx_fabsf (matrix->m[1][1]) ) );
}

#if CTX_FONTS_FROM_FILE
int   ctx_load_font_ttf_file (const char *name, const char *path);
int
_ctx_file_get_contents (const char     *path,
                        unsigned char **contents,
                        long           *length);
#endif

#endif
#ifndef __CTX_CONSTANTS
#define __CTX_CONSTANTS

#define TOKENHASH(a)    ((uint64_t)a)

#define CTX_strokeSource TOKENHASH(3061861651908008)
#define CTX_add_stop 	TOKENHASH(1274978316678)
#define CTX_addStop 	TOKENHASH(40799943078278)
#define CTX_alphabetic 	TOKENHASH(2629359926678406)
#define CTX_arc 	TOKENHASH(11526)
#define CTX_arc_to 	TOKENHASH(1187065094)
#define CTX_arcTo 	TOKENHASH(38558051590)
#define CTX_begin_path 	TOKENHASH(3004110681622984)
#define CTX_beginPath 	TOKENHASH(8437143659599196)
#define CTX_bevel 	TOKENHASH(29868488)
#define CTX_bottom 	TOKENHASH(1043772488)
#define CTX_cap 	TOKENHASH(37066)
#define CTX_center 	TOKENHASH(1358332362)
#define CTX_clear 	TOKENHASH(42154890)
#define CTX_color 	TOKENHASH(43086922)
#define CTX_copy 	TOKENHASH(1807434)
#define CTX_clip 	TOKENHASH(1203082)
#define CTX_close_path 	TOKENHASH(3004110663420810)
#define CTX_closePath 	TOKENHASH(8437144279135038)
#define CTX_cmyka 	TOKENHASH(7199690)
#define CTX_cmyk 	TOKENHASH(908234)
#define CTX_cmykaS 	TOKENHASH(36313095114)
#define CTX_cmykS 	TOKENHASH(1135467466)
#define CTX_color 	TOKENHASH(43086922)
#define CTX_blending 	TOKENHASH(653586873224)
#define CTX_blend 	TOKENHASH(13646728)
#define CTX_blending_mode 	TOKENHASH(8147360531130856)
#define CTX_blendingMode 	TOKENHASH(7483585768187540)
#define CTX_blend_mode 	TOKENHASH(2758775686577032)
#define CTX_blendMode 	TOKENHASH(7773213171090182)
#define CTX_composite 	TOKENHASH(16930059746378)
#define CTX_compositing_mode 	TOKENHASH(2417816728103524)
#define CTX_compositingMode 	TOKENHASH(2807194446992106)
#define CTX_curve_to 	TOKENHASH(1215559149002)
#define CTX_curveTo 	TOKENHASH(39483449320906)
#define CTX_darken 	TOKENHASH(1089315020)
#define CTX_defineGlyph 	TOKENHASH(2497926167421194)
#define CTX_defineTexture 	TOKENHASH(2623744577477404)
#define CTX_kerningPair 	TOKENHASH(6964644556489058)
#define CTX_destinationIn 	TOKENHASH(8153299143600102)
#define CTX_destination_in 	TOKENHASH(3824201982576824)
#define CTX_destinationAtop 	TOKENHASH(8185118415574560)
#define CTX_destination_atop 	TOKENHASH(7742210324901698)
#define CTX_destinationOver 	TOKENHASH(3261713333438500)
#define CTX_destination_over 	TOKENHASH(7742210324728474)
#define CTX_destinationOut 	TOKENHASH(7742210322269456)
#define CTX_destination_out 	TOKENHASH(8153299143489102)
#define CTX_difference 	TOKENHASH(2756492040618700)
#define CTX_done 	TOKENHASH(492620)
#define CTX_drgba 	TOKENHASH(6573324)
#define CTX_drgb 	TOKENHASH(281868)
#define CTX_drgbaS 	TOKENHASH(36312468748)
#define CTX_drgbS 	TOKENHASH(1134841100)
#define CTX_end 	TOKENHASH(13326)
#define CTX_endfun 	TOKENHASH(1122513934)
#define CTX_end_group 	TOKENHASH(41200834917390)
#define CTX_endGroup 	TOKENHASH(3570227948106766)
#define CTX_even_odd 	TOKENHASH(426345774606)
#define CTX_evenOdd 	TOKENHASH(13671748091406)
#define CTX_exit 	TOKENHASH(1465998)
#define CTX_fill 	TOKENHASH(946896)
#define CTX_fill_rule 	TOKENHASH(16405972808400)
#define CTX_fillRule 	TOKENHASH(2776813389378256)
#define CTX_flush 	TOKENHASH(22395792)
#define CTX_font 	TOKENHASH(1475664)
#define CTX_font_size 	TOKENHASH(17342343316560)
#define CTX_setFontSize 	TOKENHASH(8657699789799734)
#define CTX_fontSize 	TOKENHASH(2806775148872784)
#define CTX_function 	TOKENHASH(1136803546576)
#define CTX_getkey 	TOKENHASH(1827516882)
#define CTX_global_alpha 	TOKENHASH(6945103263242432)
#define CTX_globalAlpha 	TOKENHASH(2684560928159160)
#define CTX_glyph 	TOKENHASH(22207378)
#define CTX_gradient_add_stop 	TOKENHASH(7829524561074416)
#define CTX_gradientAddStop 	TOKENHASH(8126442749593072)
#define CTX_graya 	TOKENHASH(8068370)
#define CTX_gray 	TOKENHASH(1776914)
#define CTX_grayaS 	TOKENHASH(36313963794)
#define CTX_grayS 	TOKENHASH(1136336146)
#define CTX_hanging 	TOKENHASH(20424786132)
#define CTX_height 	TOKENHASH(1497979348)
#define CTX_hor_line_to 	TOKENHASH(8345271542735158)
#define CTX_horLineTo 	TOKENHASH(3629696407754856)
#define CTX_hue 	TOKENHASH(15828)
#define CTX_identity 	TOKENHASH(1903455910294)
#define CTX_ideographic 	TOKENHASH(4370819675496700)
#define CTX_imageSmoothing 	TOKENHASH(4268778175825416)
#define CTX_join 	TOKENHASH(1072216)
#define CTX_laba 	TOKENHASH(205020)
#define CTX_lab 	TOKENHASH(8412)
#define CTX_lcha 	TOKENHASH(217436)
#define CTX_lch 	TOKENHASH(20828)
#define CTX_labaS 	TOKENHASH(1134764252)
#define CTX_labS 	TOKENHASH(35463388)
#define CTX_lchaS 	TOKENHASH(1134776668)
#define CTX_lchS 	TOKENHASH(35475804)
#define CTX_left 	TOKENHASH(1458652)
#define CTX_lighter 	TOKENHASH(43466246876)
#define CTX_lighten 	TOKENHASH(34876312284)
#define CTX_linear_gradient 	TOKENHASH(7801595375834212)
#define CTX_linearGradient 	TOKENHASH(4439260636789186)
#define CTX_line_cap 	TOKENHASH(1243731165916)
#define CTX_lineCap 	TOKENHASH(3436510399409980)
#define CTX_setLineCap 	TOKENHASH(7897176123029482)
#define CTX_line_height 	TOKENHASH(3300223516389168)
#define CTX_line_join 	TOKENHASH(35977601450716)
#define CTX_lineJoin 	TOKENHASH(3403122163024604)
#define CTX_setLineJoin 	TOKENHASH(2768281536656332)
#define CTX_line_spacing 	TOKENHASH(2519451230887150)
#define CTX_line_to 	TOKENHASH(37986206428)
#define CTX_lineTo 	TOKENHASH(1233857774300)
#define CTX_lineDash 	TOKENHASH(3001937455186652)
#define CTX_lineDashOffset 	TOKENHASH(3704120356324362)
#define CTX_line_width 	TOKENHASH(3004303386575580)
#define CTX_lineWidth 	TOKENHASH(8241159254028040)
#define CTX_setLineWidth 	TOKENHASH(8037913618228476)
#define CTX_view_box 	TOKENHASH(1823485803248)
#define CTX_viewBox 	TOKENHASH(3915860941641152)
#define CTX_middle 	TOKENHASH(499528414)
#define CTX_miter 	TOKENHASH(42447582)
#define CTX_miter_limit 	TOKENHASH(4281255327472850)
#define CTX_miterLimit 	TOKENHASH(7937453649653124)
#define CTX_move_to 	TOKENHASH(37986223198)
#define CTX_moveTo 	TOKENHASH(1233857791070)
#define CTX_multiply 	TOKENHASH(1886723143134)
#define CTX_new_page 	TOKENHASH(500602882528)
#define CTX_newPage 	TOKENHASH(16020123011552)
#define CTX_new_path 	TOKENHASH(734678600160)
#define CTX_newPath 	TOKENHASH(23510545975776)
#define CTX_new_state 	TOKENHASH(16912954280416)
#define CTX_none 	TOKENHASH(492640)
#define CTX_normal 	TOKENHASH(946840672)
#define CTX_quad_to 	TOKENHASH(37986115046)
#define CTX_quadTo 	TOKENHASH(1233857682918)
#define CTX_radial_gradient 	TOKENHASH(8267515704460560)
#define CTX_radialGradient 	TOKENHASH(4399889250822134)
#define CTX_rectangle 	TOKENHASH(16375644301800)
#define CTX_rect 	TOKENHASH(1452520)
#define CTX_rel_arc_to 	TOKENHASH(3496527781786088)
#define CTX_relArcTo 	TOKENHASH(3209152175601038)
#define CTX_rel_curve_to 	TOKENHASH(4439651822639910)
#define CTX_relCurveTo 	TOKENHASH(7294415873689320)
#define CTX_rel_hor_line_to 	TOKENHASH(7051067105640810)
#define CTX_relHorLineTo 	TOKENHASH(8737419863647946)
#define CTX_relVerLineTo 	TOKENHASH(8737441317512906)
#define CTX_rel_line_to 	TOKENHASH(8345271542378314)
#define CTX_relLineTo 	TOKENHASH(3629696197927444)
#define CTX_rel_move_to 	TOKENHASH(8344984486309706)
#define CTX_relMoveTo 	TOKENHASH(3571677202293268)
#define CTX_rel_quad_to 	TOKENHASH(8343627754794826)
#define CTX_relQuadTo 	TOKENHASH(7894357900599828)
#define CTX_rel_smoothq_to 	TOKENHASH(7340038162167138)
#define CTX_relSmoothqTo 	TOKENHASH(3188040406230844)
#define CTX_rel_smooth_to 	TOKENHASH(8144941131301668)
#define CTX_relSmoothTo 	TOKENHASH(8947422784198618)
#define CTX_rel_ver_line_to 	TOKENHASH(8148126344839530)
#define CTX_restore 	TOKENHASH(16411699688)
#define CTX_reset 	TOKENHASH(46639592)
#define CTX_rgba 	TOKENHASH(205416)
#define CTX_rgb 	TOKENHASH(8808)
#define CTX_rgbaS 	TOKENHASH(1134764648)
#define CTX_rgbS 	TOKENHASH(35463784)
#define CTX_right 	TOKENHASH(46811880)
#define CTX_rotate 	TOKENHASH(516142184)
#define CTX_round 	TOKENHASH(13679720)
#define CTX_round_rectangle 	TOKENHASH(4332080966833870)
#define CTX_roundRectangle 	TOKENHASH(8317255488676642)
#define CTX_save 	TOKENHASH(508138)
#define CTX_scale 	TOKENHASH(15604074)
#define CTX_screen 	TOKENHASH(1088921962)
#define CTX_setkey 	TOKENHASH(1827516906)
#define CTX_shadowBlur 	TOKENHASH(2924056626980284)
#define CTX_shadowColor 	TOKENHASH(3509599043947446)
#define CTX_shadowOffsetX 	TOKENHASH(8499312693589794)
#define CTX_shadowOffsetY 	TOKENHASH(8499312693589796)
#define CTX_smooth_quad_to 	TOKENHASH(6832232668547050)
#define CTX_smoothQuadTo 	TOKENHASH(8278352345012646)
#define CTX_smooth_to 	TOKENHASH(38898089692138)
#define CTX_smoothTo 	TOKENHASH(3515270388878314)
#define CTX_sourceIn 	TOKENHASH(3444145493687402)
#define CTX_source_in 	TOKENHASH(35942915423338)
#define CTX_sourceAtop 	TOKENHASH(2920281959978332)
#define CTX_source_atop 	TOKENHASH(3007410591464110)
#define CTX_sourceOut 	TOKENHASH(7371294932695718)
#define CTX_source_out 	TOKENHASH(3851660580666474)
#define CTX_sourceOver 	TOKENHASH(7584784067385004)
#define CTX_source_over 	TOKENHASH(8690648756484770)
#define CTX_square 	TOKENHASH(511950058)
#define CTX_start 	TOKENHASH(47455658)
#define CTX_start_move 	TOKENHASH(2798358138985898)
#define CTX_start_group 	TOKENHASH(7836274863228782)
#define CTX_startGroup 	TOKENHASH(3812645199786240)
#define CTX_stroke 	TOKENHASH(498181546)
#define CTX_text_align 	TOKENHASH(3398277113762284)
#define CTX_textAlign 	TOKENHASH(3063795447820748)
#define CTX_texture 	TOKENHASH(16424292844)
#define CTX_text_baseline 	TOKENHASH(2589194334827348)
#define CTX_text_baseline 	TOKENHASH(2589194334827348)
#define CTX_textBaseline 	TOKENHASH(8381669925369340)
#define CTX_fillRect 	TOKENHASH(3811453831115472)
#define CTX_text 	TOKENHASH(1495532)
#define CTX_text_direction 	TOKENHASH(3614589880641524)
#define CTX_textDirection 	TOKENHASH(6790122975782654)
#define CTX_text_indent 	TOKENHASH(3633795456290560)
#define CTX_text_stroke 	TOKENHASH(8259523149811490)
#define CTX_strokeText 	TOKENHASH(8131451867629426)
#define CTX_strokeRect 	TOKENHASH(8165399289138988)
#define CTX_top 	TOKENHASH(37996)
#define CTX_transform 	TOKENHASH(34396827557164)
#define CTX_translate 	TOKENHASH(16912418348332)
#define CTX_verLineTo 	TOKENHASH(3629696413166220)
#define CTX_ver_line_to 	TOKENHASH(8345271542726354)
#define CTX_width 	TOKENHASH(22426354)
#define CTX_winding 	TOKENHASH(20424590066)
#define CTX_x 	TOKENHASH(52)
#define CTX_xor 	TOKENHASH(42100)
#define CTX_y 	TOKENHASH(54)
#define CTX_colorSpace 	TOKENHASH(3674150843793134)
#define CTX_userRGB 	TOKENHASH(59177128181102)
#define CTX_userCMYK 	TOKENHASH(3354734206905240)
#define CTX_deviceRGB 	TOKENHASH(7818727413767480)
#define CTX_deviceCMYK 	TOKENHASH(8943291245184210)
#define CTX_silver 	TOKENHASH(1358459626)
#define CTX_fuchsia 	TOKENHASH(7225355728)
#define CTX_gray 	TOKENHASH(1776914)
#define CTX_yellow 	TOKENHASH(1714319862)
#define CTX_white 	TOKENHASH(16145074)
#define CTX_maroon 	TOKENHASH(1110548702)
#define CTX_magenta 	TOKENHASH(7952877790)
#define CTX_blue 	TOKENHASH(506760)
#define CTX_green 	TOKENHASH(34028818)
#define CTX_red 	TOKENHASH(12776)
#define CTX_purple 	TOKENHASH(500344292)
#define CTX_olive 	TOKENHASH(16276386)
#define CTX_teal 	TOKENHASH(924140)
#define CTX_black 	TOKENHASH(27597704)
#define CTX_cyan 	TOKENHASH(1056458)
#define CTX_navy 	TOKENHASH(1818848)
#define CTX_lime 	TOKENHASH(490204)
#define CTX_aqua 	TOKENHASH(244934)
#define CTX_transparent 	TOKENHASH(3654078210101184)
#define CTX_currentColor 	TOKENHASH(7501877057638746)

#endif
#ifndef __CTX_LIBC_H
#define __CTX_LIBC_H

#if !__COSMOPOLITAN__
#include <stddef.h>
#endif

#if 0
static inline void
ctx_memset (void *ptr, uint8_t val, int length)
{
  uint8_t *p = (uint8_t *) ptr;
  for (int i = 0; i < length; i ++)
    { p[i] = val; }
}
#else
#define ctx_memset memset
#endif


static inline void ctx_strcpy (char *dst, const char *src)
{
  int i = 0;
  for (i = 0; src[i]; i++)
    { dst[i] = src[i]; }
  dst[i] = 0;
}

static inline char *_ctx_strchr (const char *haystack, char needle)
{
  const char *p = haystack;
  while (*p && *p != needle)
    {
      p++;
    }
  if (*p == needle)
    { return (char *) p; }
  return NULL;
}
static inline char *ctx_strchr (const char *haystack, char needle)
{
  return _ctx_strchr (haystack, needle);
}

static inline int ctx_strcmp (const char *a, const char *b)
{
  int i;
  for (i = 0; a[i] && b[i]; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  if (a[0] == 0 && b[0] == 0) { return 0; }
  return 1;
}

static inline int ctx_strncmp (const char *a, const char *b, size_t n)
{
  size_t i;
  for (i = 0; a[i] && b[i] && i < n; a++, b++)
    if (a[0] != b[0])
      { return 1; }
  return 0;
}

static inline int ctx_strlen (const char *s)
{
  int len = 0;
  for (; *s; s++) { len++; }
  return len;
}

static inline char *ctx_strstr (const char *h, const char *n)
{
  int needle_len = ctx_strlen (n);
  if (n[0]==0)
    { return (char *) h; }
  while (h)
    {
      h = ctx_strchr (h, n[0]);
      if (!h)
        { return NULL; }
      if (!ctx_strncmp (h, n, needle_len) )
        { return (char *) h; }
      h++;
    }
  return NULL;
}

#endif

#if CTX_IMPLEMENTATION|CTX_COMPOSITE

#ifndef __CTX_INTERNAL_H
#define __CTX_INTERNAL_H

#if !__COSMOPOLITAN__
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/select.h> 
#endif

#define CTX_BRANCH_HINTS  1

#if CTX_BRANCH_HINTS
#define CTX_LIKELY(x)      __builtin_expect(!!(x), 1)
#define CTX_UNLIKELY(x)    __builtin_expect(!!(x), 0)
#else
#define CTX_LIKELY(x)      (x)
#define CTX_UNLIKELY(x)    (x)
#endif

typedef struct _CtxRasterizer CtxRasterizer;
typedef struct _CtxGState     CtxGState;
typedef struct _CtxState      CtxState;

typedef struct _CtxSource CtxSource;


#define CTX_VALID_RGBA_U8     (1<<0)
#define CTX_VALID_RGBA_DEVICE (1<<1)
#if CTX_ENABLE_CM
#define CTX_VALID_RGBA        (1<<2)
#endif
#if CTX_ENABLE_CMYK
#define CTX_VALID_CMYKA       (1<<3)
#define CTX_VALID_DCMYKA      (1<<4)
#endif
#define CTX_VALID_GRAYA       (1<<5)
#define CTX_VALID_GRAYA_U8    (1<<6)
#define CTX_VALID_LABA        ((1<<7) | CTX_VALID_GRAYA)

struct _CtxColor
{
  uint8_t magic; // for colors used in keydb, set to a non valid start of
                 // string value.
  uint8_t rgba[4];
  uint8_t l_u8;
  uint8_t original; // the bitmask of the originally set color
  uint8_t valid;    // bitmask of which members contain valid
  // values, gets denser populated as more
  // formats are requested from a set color.
  float   device_red;
  float   device_green;
  float   device_blue;
  float   alpha;
  float   l;        // luminance and gray
#if CTX_ENABLE_LAB  // NYI
  float   a;
  float   b;
#endif
#if CTX_ENABLE_CMYK
  float   device_cyan;
  float   device_magenta;
  float   device_yellow;
  float   device_key;
  float   cyan;
  float   magenta;
  float   yellow;
  float   key;
#endif

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl *space; // gets copied from state when color is declared
#else
  void   *space; // gets copied from state when color is declared, 
#endif
  float   red;
  float   green;
  float   blue;
#endif
};

typedef struct _CtxGradientStop CtxGradientStop;

struct _CtxGradientStop
{
  float   pos;
  CtxColor color;
};


enum _CtxSourceType
{
  CTX_SOURCE_COLOR = 0,
  CTX_SOURCE_TEXTURE,
  CTX_SOURCE_LINEAR_GRADIENT,
  CTX_SOURCE_RADIAL_GRADIENT,
  CTX_SOURCE_INHERIT_FILL
};

typedef enum _CtxSourceType CtxSourceType;

typedef struct _CtxPixelFormatInfo CtxPixelFormatInfo;


struct _CtxBuffer
{
  void               *data;
  int                 width;
  int                 height;
  int                 stride;
  char               *eid;        // might be NULL, when not - should be unique for pixel contents
  int                 frame;      // last frame used in, everything > 3 can be removed,
                                  // as clients wont rely on it.
  CtxPixelFormatInfo *format;
  void (*free_func) (void *pixels, void *user_data);
  void               *user_data;

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl *space;
#else
  void       *space; 
#endif
#endif
#if 1
  CtxBuffer          *color_managed; /* only valid for one render target, cache
                                        for a specific space
                                        */
#endif
};

//void _ctx_user_to_device          (CtxState *state, float *x, float *y);
//void _ctx_user_to_device_distance (CtxState *state, float *x, float *y);

typedef struct _CtxGradient CtxGradient;
struct _CtxGradient
{
  CtxGradientStop stops[16];
  int n_stops;
};

struct _CtxSource
{
  int type;
  CtxMatrix  transform;
  union
  {
    CtxColor color;
    struct
    {
      uint8_t rgba[4]; // shares data with set color
      uint8_t pad;
      float x0;
      float y0;
      CtxBuffer *buffer;
    } texture;
    struct
    {
      float x0;
      float y0;
      float x1;
      float y1;
      float dx;
      float dy;
      float start;
      float end;
      float length;
      float rdelta;
    } linear_gradient;
    struct
    {
      float x0;
      float y0;
      float r0;
      float x1;
      float y1;
      float r1;
      float rdelta;
    } radial_gradient;
  };
};

struct _CtxGState
{
  int           keydb_pos;
  int           stringpool_pos;

  CtxMatrix     transform;
  CtxSource     source_stroke;
  CtxSource     source_fill;
  float         global_alpha_f;
  uint8_t       global_alpha_u8;

  float         line_width;
  float         line_dash_offset;
  float         miter_limit;
  float         font_size;
#if CTX_ENABLE_SHADOW_BLUR
  float         shadow_blur;
  float         shadow_offset_x;
  float         shadow_offset_y;
#endif
  int           clipped:1;

  int16_t       clip_min_x;
  int16_t       clip_min_y;
  int16_t       clip_max_x;
  int16_t       clip_max_y;

#if CTX_ENABLE_CM
#if CTX_BABL
  const Babl   *device_space;
  const Babl   *texture_space;
  const Babl   *rgb_space;       
  const Babl   *cmyk_space;

  const Babl   *fish_rgbaf_user_to_device;
  const Babl   *fish_rgbaf_texture_to_device;
  const Babl   *fish_rgbaf_device_to_user;

#else
  void         *device_space;
  void         *texture_space;
  void         *rgb_space;       
  void         *cmyk_space;
  void         *fish_rgbaf_user_to_device; // dummy padding
  void         *fish_rgbaf_texture_to_device; // dummy padding
  void         *fish_rgbaf_device_to_user; // dummy padding
#endif
#endif
  CtxCompositingMode  compositing_mode; // bitfield refs lead to
  CtxBlend                  blend_mode; // non-vectorization

  float dashes[CTX_PARSER_MAX_ARGS];
  int n_dashes;

  CtxColorModel    color_model;
  /* bitfield-pack small state-parts */
  CtxLineCap          line_cap:2;
  CtxLineJoin        line_join:2;
  CtxFillRule        fill_rule:1;
  unsigned int image_smoothing:1;
  unsigned int            font:6;
  unsigned int            bold:1;
  unsigned int          italic:1;
};

typedef enum
{
  CTX_TRANSFORMATION_NONE         = 0,
  CTX_TRANSFORMATION_SCREEN_SPACE = 1,
  CTX_TRANSFORMATION_RELATIVE     = 2,
#if CTX_BITPACK
  CTX_TRANSFORMATION_BITPACK      = 4,
#endif
  CTX_TRANSFORMATION_STORE_CLEAR  = 16,
} CtxTransformation;

#define CTX_DRAWLIST_DOESNT_OWN_ENTRIES   64
#define CTX_DRAWLIST_EDGE_LIST            128
#define CTX_DRAWLIST_CURRENT_PATH         512
// BITPACK

struct _CtxDrawlist
{
  CtxEntry *entries;
  int       count;
  int       size;
  uint32_t  flags;
  int       bitpack_pos;  // stream is bitpacked up to this offset
};

#define CTX_MAX_KEYDB 64 // number of entries in keydb
                         // entries are "copy-on-change" between states

// the keydb consists of keys set to floating point values,
// that might also be interpreted as integers for enums.
//
// the hash
typedef struct _CtxKeyDbEntry CtxKeyDbEntry;
struct _CtxKeyDbEntry
{
  uint64_t key;
  float value;
  //union { float f[1]; uint8_t u8[4]; }value;
};

struct _CtxState
{
  int           has_moved:1;
  int           has_clipped:1;
  float         x;
  float         y;
  int           min_x;
  int           min_y;
  int           max_x;
  int           max_y;
  int16_t       gstate_no;
  CtxGState     gstate;
  CtxGState     gstate_stack[CTX_MAX_STATES];//at end, so can be made dynamic
#if CTX_GRADIENTS
  CtxGradient   gradient; /* we keep only one gradient,
                             this goes icky with multiple
                             restores - it should really be part of
                             graphics state..
                             XXX, with the stringpool gradients
                             can be stored there.
                           */
#endif
  CtxKeyDbEntry keydb[CTX_MAX_KEYDB];
  char          stringpool[CTX_STRINGPOOL_SIZE];
  int8_t        source; // used for the single-shifting to stroking
                // 0  = fill
                // 1  = start_stroke
                // 2  = in_stroke
                //
                //   if we're at in_stroke at start of a source definition
                //   we do filling
};


typedef struct _CtxFont       CtxFont;
typedef struct _CtxFontEngine CtxFontEngine;

struct _CtxFontEngine
{
#if CTX_FONTS_FROM_FILE
  int   (*load_file)   (const char *name, const char *path);
#endif
  int   (*load_memory) (const char *name, const void *data, int length);
  int   (*glyph)       (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke);
  float (*glyph_width) (CtxFont *font, Ctx *ctx, uint32_t unichar);
  float (*glyph_kern)  (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);
};

struct _CtxFont
{
  CtxFontEngine *engine;
  const char *name;
  int type; // 0 ctx    1 stb    2 monobitmap
  union
  {
    struct
    {
      CtxEntry *data;
      int length;
      /* we've got ~110 bytes to fill to cover as
         much data as stbtt_fontinfo */
      //int16_t glyph_pos[26]; // for a..z
      int       glyphs; // number of glyphs
      uint32_t *index;
    } ctx;
    struct
    {
      char *path;
    } ctx_fs;
#if CTX_FONT_ENGINE_STB
    struct
    {
      stbtt_fontinfo ttf_info;
      int cache_index;
      uint32_t cache_unichar;
    } stb;
#endif
    struct { int start; int end; int gw; int gh; const uint8_t *data;} monobitmap;
  };
};


enum _CtxIteratorFlag
{
  CTX_ITERATOR_FLAT           = 0,
  CTX_ITERATOR_EXPAND_BITPACK = 2,
  CTX_ITERATOR_DEFAULTS       = CTX_ITERATOR_EXPAND_BITPACK
};
typedef enum _CtxIteratorFlag CtxIteratorFlag;


struct
  _CtxIterator
{
  int              pos;
  int              first_run;
  CtxDrawlist *drawlist;
  int              end_pos;
  int              flags;

  int              bitpack_pos;
  int              bitpack_length;     // if non 0 bitpack is active
  CtxEntry         bitpack_command[6]; // the command returned to the
  // user if unpacking is needed.
};
#define CTX_MAX_DEVICES 16
#define CTX_MAX_KEYBINDINGS         256

#if CTX_EVENTS 

// include list implementation - since it already is a header+inline online
// implementation?

typedef struct CtxItemCb {
  CtxEventType types;
  CtxCb        cb;
  void*        data1;
  void*        data2;

  void (*finalize) (void *data1, void *data2, void *finalize_data);
  void  *finalize_data;

} CtxItemCb;


#define CTX_MAX_CBS              128

typedef struct CtxItem {
  CtxMatrix inv_matrix;  /* for event coordinate transforms */

  /* bounding box */
  float          x0;
  float          y0;
  float          x1;
  float          y1;

  void *path;
  double          path_hash;

  CtxCursor       cursor; /* if 0 then UNSET and no cursor change is requested
                           */

  CtxEventType   types;   /* all cb's ored together */
  CtxItemCb cb[CTX_MAX_CBS];
  int       cb_count;
  int       ref_count;
} CtxItem;


typedef struct _CtxEvents CtxEvents;
struct _CtxEvents
{
  int             frozen;
  int             fullscreen;
  CtxList        *grabs; /* could split the grabs per device in the same way,
                            to make dispatch overhead smaller,. probably
                            not much to win though. */
  CtxItem         *prev[CTX_MAX_DEVICES];
  float            pointer_x[CTX_MAX_DEVICES];
  float            pointer_y[CTX_MAX_DEVICES];
  unsigned char    pointer_down[CTX_MAX_DEVICES];
  CtxEvent         drag_event[CTX_MAX_DEVICES];
  CtxList         *idles;
  CtxList         *events; // for ctx_get_event
  int              ctx_get_event_enabled;
  int              idle_id;
  CtxBinding       bindings[CTX_MAX_KEYBINDINGS]; /*< better as list, uses no mem if unused */
  int              n_bindings;
  int              width;
  int              height;
  CtxList         *items;
  CtxItem         *last_item;
  CtxModifierState modifier_state;
  int              tap_delay_min;
  int              tap_delay_max;
  int              tap_delay_hold;
  double           tap_hysteresis;
};


#endif

typedef struct _CtxEidInfo
{
  char *eid;
  int   frame;
  int   width;
  int   height;
} CtxEidInfo;

struct _Ctx
{
  CtxImplementation *renderer;
  CtxDrawlist        drawlist;
  int                transformation;
  CtxBuffer          texture[CTX_MAX_TEXTURES];
  Ctx               *texture_cache;
  CtxList           *eid_db;
  int                rev;
  void              *backend;
  CtxState           state;        /**/
  int                frame; /* used for texture lifetime */
#if CTX_EVENTS 
  CtxCursor          cursor;
  int                quit;
  int                dirty;
  CtxEvents          events;
  int                mouse_fd;
  int                mouse_x;
  int                mouse_y;
#endif
#if CTX_CURRENT_PATH
  CtxDrawlist    current_path; // possibly transformed coordinates !
  CtxIterator        current_path_iterator;
#endif
};


void ctx_process (Ctx *ctx, CtxEntry *entry);
CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format);
void ctx_buffer_free (CtxBuffer *buffer);

void
ctx_state_gradient_clear_stops (CtxState *state);

void ctx_interpret_style         (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_transforms    (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_pos           (CtxState *state, CtxEntry *entry, void *data);
void ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data);

struct _CtxInternalFsEntry
{
  char *path;
  int   length;
  char *data;
};

struct _CtxPixelFormatInfo
{
  CtxPixelFormat pixel_format;
  uint8_t        components:4; /* number of components */
  uint8_t        bpp; /* bits  per pixel - for doing offset computations
                         along with rowstride found elsewhere, if 0 it indicates
                         1/8  */
  uint8_t        ebpp; /*effective bytes per pixel - for doing offset
                         computations, for formats that get converted, the
                         ebpp of the working space applied */
  uint8_t        dither_red_blue;
  uint8_t        dither_green;
  CtxPixelFormat composite_format;

  void         (*to_comp) (CtxRasterizer *r,
                           int x, const void * __restrict__ src, uint8_t * __restrict__ comp, int count);
  void         (*from_comp) (CtxRasterizer *r,
                             int x, const uint8_t * __restrict__ comp, void *__restrict__ dst, int count);
  void         (*apply_coverage) (CtxRasterizer *r, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x, uint8_t *coverage,
                          int count);
  void         (*setup) (CtxRasterizer *r);
};


static void
_ctx_user_to_device (CtxState *state, float *x, float *y);
static void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y);
static void ctx_state_init (CtxState *state);
void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data);
void
ctx_drawlist_deinit (CtxDrawlist *drawlist);

CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format);


int ctx_utf8_len (const unsigned char first_byte);
const char *ctx_utf8_skip (const char *s, int utf8_length);
int ctx_utf8_strlen (const char *s);
int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest);

uint32_t
ctx_utf8_to_unichar (const char *input);


typedef struct _CtxHasher CtxHasher;

typedef struct CtxEdge
{
#if CTX_ALIGNED_STRUCTS==1
  uint32_t index;
#else
  uint16_t index;
#endif
  int32_t  val;     /* the center-line intersection      */
  int32_t  delta;
} CtxEdge;

typedef void (*CtxFragment) (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy);

#define CTX_MAX_GAUSSIAN_KERNEL_DIM    512

struct _CtxShapeEntry
{
  uint32_t hash;
  uint16_t width;
  uint16_t height;
  int      last_frame; // xxx
  uint32_t uses;  // instrumented for longer keep-alive
  uint8_t  data[];
};

typedef struct _CtxShapeEntry CtxShapeEntry;


struct _CtxShapeCache
{
  CtxShapeEntry *entries[CTX_SHAPE_CACHE_ENTRIES];
  long size;
};

typedef struct _CtxShapeCache CtxShapeCache;


struct _CtxRasterizer
{
  CtxImplementation vfuncs;
  /* these should be initialized and used as the bounds for rendering into the
     buffer as well XXX: not yet in use, and when in use will only be
     correct for axis aligned clips - proper rasterization of a clipping path
     would be yet another refinement on top.
   */

#if CTX_ENABLE_SHADOW_BLUR
  float      kernel[CTX_MAX_GAUSSIAN_KERNEL_DIM];
#endif

  int        aa;          // level of vertical aa
  int        active_edges;
  int        active_edges2;
  int        pending_edges;   // this-scanline
  int        ending_edges;    // count of edges ending this scanline
  int        edge_pos;         // where we're at in iterating all edges
  CtxEdge    edges[CTX_MAX_EDGES];

  int        scanline;
  int        scan_min;
  int        scan_max;
  int        col_min;
  int        col_max;

  CtxDrawlist edge_list;

  CtxState  *state;
  Ctx       *ctx;
  Ctx       *texture_source; /* normally same as ctx */

  void      *buf;


#if CTX_COMPOSITING_GROUPS
  void      *saved_buf; // when group redirected
  CtxBuffer *group[CTX_GROUP_MAX];
#endif


  float      x;  // < redundant? use state instead?
  float      y;

  float      first_x;
  float      first_y;
  int8_t     needs_aa3; // count of how many edges implies antialiasing
  int8_t     needs_aa5; // count of how many edges implies antialiasing
  int8_t     needs_aa15; // count of how many edges implies antialiasing
  int        has_shape:2;
  int        has_prev:2;
  int        preserve:1;
  int        uses_transforms:1;

  int16_t    blit_x;
  int16_t    blit_y;
  int16_t    blit_width;
  int16_t    blit_height;
  int16_t    blit_stride;

  CtxPixelFormatInfo *format;

#if CTX_ENABLE_SHADOW_BLUR
  int in_shadow;
#endif
  int in_text;
  int shadow_x;
  int shadow_y;

  CtxFragment         fragment;
  int swap_red_green;
  uint8_t             color[4*5];

#define CTX_COMPOSITE_ARGUMENTS CtxRasterizer *rasterizer, uint8_t * __restrict__ dst, uint8_t * __restrict__ src, int x0, uint8_t * __restrict__ coverage, int count

  void (*comp_op)(CTX_COMPOSITE_ARGUMENTS);
#if CTX_ENABLE_CLIP
  CtxBuffer *clip_buffer;
#endif

  int clip_rectangle;

#if CTX_SHAPE_CACHE
  CtxShapeCache shape_cache;
#endif
#if CTX_BRAILLE_TEXT
  int        term_glyphs:1; // store appropriate glyphs for redisplay
  CtxList   *glyphs;
#endif
};

struct _CtxSHA1 {
    uint64_t length;
    uint32_t state[5], curlen;
    unsigned char buf[64];
};


struct _CtxHasher
{
  CtxRasterizer rasterizer;
  int           cols;
  int           rows;
  uint8_t      *hashes;
  CtxSHA1       sha1_fill; 
  CtxSHA1       sha1_stroke;
};

#if CTX_RASTERIZER
void ctx_rasterizer_deinit (CtxRasterizer *rasterizer);
#endif

#if CTX_EVENTS
extern int ctx_native_events;

#if CTX_SDL
extern int ctx_sdl_events;
int ctx_sdl_consume_events (Ctx *ctx);
#endif

#if CTX_FB
extern int ctx_fb_events;
int ctx_fb_consume_events (Ctx *ctx);
#endif


int ctx_nct_consume_events (Ctx *ctx);
int ctx_ctx_consume_events (Ctx *ctx);

#endif

enum {
  NC_MOUSE_NONE  = 0,
  NC_MOUSE_PRESS = 1,  /* "mouse-pressed", "mouse-released" */
  NC_MOUSE_DRAG  = 2,  /* + "mouse-drag"   (motion with pressed button) */
  NC_MOUSE_ALL   = 3   /* + "mouse-motion" (also delivered for release) */
};
void _ctx_mouse (Ctx *term, int mode);
void nc_at_exit (void);

int ctx_terminal_width  (void);
int ctx_terminal_height (void);
int ctx_terminal_cols   (void);
int ctx_terminal_rows   (void);
extern int ctx_frame_ack;

int ctx_nct_consume_events (Ctx *ctx);

typedef struct _CtxCtx CtxCtx;
struct _CtxCtx
{
   void (*render) (void *ctxctx, CtxCommand *command);
   void (*reset)  (void *ctxvtx);
   void (*flush)  (void *ctxctx);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *ctxctx);
   Ctx *ctx;
   int  width;
   int  height;
   int  cols;
   int  rows;
   int  was_down;
};


extern int _ctx_max_threads;
extern int _ctx_enable_hash_cache;
void
ctx_set (Ctx *ctx, uint64_t key_hash, const char *string, int len);
const char *
ctx_get (Ctx *ctx, const char *key);

int ctx_renderer_is_term (Ctx *ctx);
Ctx *ctx_new_ctx (int width, int height);
Ctx *ctx_new_fb (int width, int height, int drm);
Ctx *ctx_new_sdl (int width, int height);
Ctx *ctx_new_term (int width, int height);
Ctx *ctx_new_termimg (int width, int height);

int ctx_resolve_font (const char *name);
extern float ctx_u8_float[256];
#define ctx_u8_to_float(val_u8) ctx_u8_float[((uint8_t)(val_u8))]
//#define ctx_u8_to_float(val_u8) (val_u8/255.0f)
//
//


static uint8_t ctx_float_to_u8 (float val_f)
{
  return val_f < 0.0f ? 0 : val_f > 1.0f ? 0xff : 0xff * val_f +  0.5f;
#if 0
  int val_i = val_f * 255.999f;
  if (val_i < 0) { return 0; }
  else if (val_i > 255) { return 255; }
  return val_i;
#endif
}


#define CTX_CSS_LUMINANCE_RED   0.3f
#define CTX_CSS_LUMINANCE_GREEN 0.59f
#define CTX_CSS_LUMINANCE_BLUE  0.11f

/* works on both float and uint8_t */
#define CTX_CSS_RGB_TO_LUMINANCE(rgb)  (\
  (rgb[0]) * CTX_CSS_LUMINANCE_RED + \
  (rgb[1]) * CTX_CSS_LUMINANCE_GREEN +\
  (rgb[2]) * CTX_CSS_LUMINANCE_BLUE)

const char *ctx_nct_get_event (Ctx *n, int timeoutms, int *x, int *y);
const char *ctx_native_get_event (Ctx *n, int timeoutms);
void
ctx_color_get_rgba8 (CtxState *state, CtxColor *color, uint8_t *out);
void ctx_color_get_graya_u8 (CtxState *state, CtxColor *color, uint8_t *out);
float ctx_float_color_rgb_to_gray (CtxState *state, const float *rgb);
void ctx_color_get_graya (CtxState *state, CtxColor *color, float *out);
void ctx_rgb_to_cmyk (float r, float g, float b,
              float *c_out, float *m_out, float *y_out, float *k_out);
uint8_t ctx_u8_color_rgb_to_gray (CtxState *state, const uint8_t *rgb);
#if CTX_ENABLE_CMYK
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out);
#endif
static void ctx_color_set_RGBA8 (CtxState *state, CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
static void ctx_color_set_drgba (CtxState *state, CtxColor *color, float r, float g, float b, float a);
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out);
static void ctx_color_set_cmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a);
static void ctx_color_set_dcmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a);
static void ctx_color_set_graya (CtxState *state, CtxColor *color, float gray, float alpha);

int ctx_color_model_get_components (CtxColorModel model);

void ctx_state_set (CtxState *state, uint64_t key, float value);
static void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f);
static void ctx_font_setup ();
float ctx_state_get (CtxState *state, uint64_t hash);

#if CTX_RASTERIZER

static void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y);
static void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y);

static void
ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y);
static void
ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y);
static void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);
static void
ctx_rasterizer_rel_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2);

static void
ctx_rasterizer_reset (CtxRasterizer *rasterizer);
static uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer);
static void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
                    float        x,
                    float        y,
                    float        radius,
                    float        start_angle,
                    float        end_angle,
                    int          anticlockwise);

static void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y);

static void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y);

static void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height);

static void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer);
static void ctx_rasterizer_clip (CtxRasterizer *rasterizer);
static void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name);

static void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba);
static void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a);
static void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height);
static void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius);

#endif

#if CTX_ENABLE_CM // XXX to be moved to ctx.h
void
ctx_set_drgb_space (Ctx *ctx, int device_space);
void
ctx_set_dcmyk_space (Ctx *ctx, int device_space);
void
ctx_rgb_space (Ctx *ctx, int device_space);
void
ctx_set_cmyk_space (Ctx *ctx, int device_space);
#endif

#endif

CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias);


CTX_INLINE static uint8_t ctx_lerp_u8 (uint8_t v0, uint8_t v1, uint8_t dx)
{
#if 0
  return v0 + ((v1-v0) * dx)/255;
#else
  return ( ( ( ( (v0) <<8) + (dx) * ( (v1) - (v0) ) ) ) >>8);
#endif
}

CTX_INLINE static float
ctx_lerpf (float v0, float v1, float dx)
{
  return v0 + (v1-v0) * dx;
}


#ifndef CTX_MIN
#define CTX_MIN(a,b)  (((a)<(b))?(a):(b))
#endif
#ifndef CTX_MAX
#define CTX_MAX(a,b)  (((a)>(b))?(a):(b))
#endif

static inline void *ctx_calloc (size_t size, size_t count);

void ctx_screenshot (Ctx *ctx, const char *output_path);


CtxSHA1 *ctx_sha1_new (void);
void ctx_sha1_free (CtxSHA1 *sha1);
int ctx_sha1_process(CtxSHA1 *sha1, const unsigned char * msg, unsigned long len);
int ctx_sha1_done(CtxSHA1 * sha1, unsigned char *out);

void _ctx_texture_lock (void);
void _ctx_texture_unlock (void);
uint8_t *ctx_define_texture_pixel_data (CtxEntry *entry);
void ctx_buffer_pixels_free (void *pixels, void *userdata);

/*ctx_texture_init:
 * return value: eid, as passed in or if NULL generated by hashing pixels and width/height
 * XXX  this is low-level and not to be used directly use define_texture instead.  XXX
 */
const char *ctx_texture_init (
                      Ctx        *ctx,
                      const char *eid,
                      int         width,
                      int         height,
                      int         stride,
                      CtxPixelFormat format,
                      void       *space,
                      uint8_t    *pixels,
                      void (*freefunc) (void *pixels, void *user_data),
                      void *user_data);

#if CTX_TILED
#if !__COSMOPOLITAN__
#include <threads.h>
#endif
#endif
typedef struct _CtxTiled CtxTiled;

struct _CtxTiled
{
   void (*render)    (void *term, CtxCommand *command);
   void (*reset)     (void *term);
   void (*flush)     (void *term);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)      (void *term);
   Ctx          *ctx;
   int           width;
   int           height;
   int           cols;
   int           rows;
   int           was_down;
   uint8_t      *pixels;
   Ctx          *ctx_copy;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
   int           quit;
#if CTX_TILED
   _Atomic int   thread_quit;
#endif
   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int       min_col; // hasher cols and rows
   int       min_row;
   int       max_col;
   int       max_row;
   uint8_t  hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t    tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //

   int           pointer_down[3];

   CtxCursor     shown_cursor;
#if CTX_TILED
   cnd_t  cond;
   mtx_t  mtx;
#endif
};

void
_ctx_texture_prepare_color_management (CtxRasterizer *rasterizer,
                                      CtxBuffer     *buffer);

#endif


#if CTX_IMPLEMENTATION

#define SHA1_IMPLEMENTATION
/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 *
 * The plain ANSIC sha1 functionality has been extracted from libtomcrypt,
 * and is included directly in the sources. /Øyvind K. - since libtomcrypt
 * is public domain the adaptations done here to make the sha1 self contained
 * also is public domain.
 */
#ifndef __SHA1_H
#define __SHA1_H
#if !__COSMOPOLITAN__
#include <inttypes.h>
#endif


int ctx_sha1_init(CtxSHA1 * sha1);
CtxSHA1 *ctx_sha1_new (void)
{
  CtxSHA1 *state = (CtxSHA1*)calloc (sizeof (CtxSHA1), 1);
  ctx_sha1_init (state);
  return state;
}
void ctx_sha1_free (CtxSHA1 *sha1)
{
  free (sha1);
}

#if 0
          CtxSHA1 sha1;
          ctx_sha1_init (&sha1);
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
#endif

#ifdef SHA1_FF0
#undef SHA1_FF0
#endif
#ifdef SHA1_FF1
#undef SHA1_FF1
#endif

#ifdef SHA1_IMPLEMENTATION
#if !__COSMOPOLITAN__
#include <stdlib.h>
#include <string.h>
#endif

#define STORE64H(x,                                                             y)                                                                     \
   { (y)[0] = (unsigned char)(((x)>>56)&255); (y)[1] = (unsigned                char)(((x)>>48)&255);     \
     (y)[2] = (unsigned char)(((x)>>40)&255); (y)[3] = (unsigned                char)(((x)>>32)&255);     \
     (y)[4] = (unsigned char)(((x)>>24)&255); (y)[5] = (unsigned                char)(((x)>>16)&255);     \
     (y)[6] = (unsigned char)(((x)>>8)&255); (y)[7] = (unsigned char)((x)&255); }

#define STORE32H(x,                                                             y)                                                                     \
     { (y)[0] = (unsigned char)(((x)>>24)&255); (y)[1] = (unsigned              char)(((x)>>16)&255);   \
       (y)[2] = (unsigned char)(((x)>>8)&255); (y)[3] = (unsigned               char)((x)&255); }

#define LOAD32H(x, y)                            \
     { x = ((unsigned long)((y)[0] & 255)<<24) | \
           ((unsigned long)((y)[1] & 255)<<16) | \
           ((unsigned long)((y)[2] & 255)<<8)  | \
           ((unsigned long)((y)[3] & 255)); }

/* rotates the hard way */
#define ROL(x, y)  ((((unsigned long)(x)<<(unsigned long)((y)&31)) | (((unsigned long)(x)&0xFFFFFFFFUL)>>(unsigned long)(32-((y)&31)))) & 0xFFFFFFFFUL)
#define ROLc(x, y) ROL(x,y)

#define CRYPT_OK     0
#define CRYPT_ERROR  1
#define CRYPT_NOP    2

#ifndef MAX
   #define MAX(x, y) ( ((x)>(y))?(x):(y) )
#endif
#ifndef MIN
   #define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif

/* a simple macro for making hash "process" functions */
#define HASH_PROCESS(func_name, compress_name, state_var, block_size)               \
int func_name (CtxSHA1 *sha1, const unsigned char *in, unsigned long inlen)      \
{                                                                                   \
    unsigned long n;                                                                \
    int           err;                                                              \
    assert (sha1 != NULL);                                                          \
    assert (in != NULL);                                                            \
    if (sha1->curlen > sizeof(sha1->buf)) {                                         \
       return -1;                                                                   \
    }                                                                               \
    while (inlen > 0) {                                                             \
        if (sha1->curlen == 0 && inlen >= block_size) {                             \
           if ((err = compress_name (sha1, (unsigned char *)in)) != CRYPT_OK) {     \
              return err;                                                           \
           }                                                                        \
           sha1->length += block_size * 8;                                          \
           in             += block_size;                                            \
           inlen          -= block_size;                                            \
        } else {                                                                    \
           n = MIN(inlen, (block_size - sha1->curlen));                             \
           memcpy(sha1->buf + sha1->curlen, in, (size_t)n);                         \
           sha1->curlen += n;                                                       \
           in             += n;                                                     \
           inlen          -= n;                                                     \
           if (sha1->curlen == block_size) {                                        \
              if ((err = compress_name (sha1, sha1->buf)) != CRYPT_OK) {            \
                 return err;                                                        \
              }                                                                     \
              sha1->length += 8*block_size;                                         \
              sha1->curlen = 0;                                                     \
           }                                                                        \
       }                                                                            \
    }                                                                               \
    return CRYPT_OK;                                                                \
}

/**********************/

#define F0(x,y,z)  (z ^ (x & (y ^ z)))
#define F1(x,y,z)  (x ^ y ^ z)
#define F2(x,y,z)  ((x & y) | (z & (x | y)))
#define F3(x,y,z)  (x ^ y ^ z)

static int  ctx_sha1_compress(CtxSHA1 *sha1, unsigned char *buf)
{
    uint32_t a,b,c,d,e,W[80],i;

    /* copy the state into 512-bits into W[0..15] */
    for (i = 0; i < 16; i++) {
        LOAD32H(W[i], buf + (4*i));
    }

    /* copy state */
    a = sha1->state[0];
    b = sha1->state[1];
    c = sha1->state[2];
    d = sha1->state[3];
    e = sha1->state[4];

    /* expand it */
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1); 
    }

    /* compress */
    /* round one */
    #define SHA1_FF0(a,b,c,d,e,i) e = (ROLc(a, 5) + F0(b,c,d) + e + W[i] + 0x5a827999UL); b = ROLc(b, 30);
    #define SHA1_FF1(a,b,c,d,e,i) e = (ROLc(a, 5) + F1(b,c,d) + e + W[i] + 0x6ed9eba1UL); b = ROLc(b, 30);
    #define SHA1_FF2(a,b,c,d,e,i) e = (ROLc(a, 5) + F2(b,c,d) + e + W[i] + 0x8f1bbcdcUL); b = ROLc(b, 30);
    #define SHA1_FF3(a,b,c,d,e,i) e = (ROLc(a, 5) + F3(b,c,d) + e + W[i] + 0xca62c1d6UL); b = ROLc(b, 30);
 
    for (i = 0; i < 20; ) {
       SHA1_FF0(a,b,c,d,e,i++);
       SHA1_FF0(e,a,b,c,d,i++);
       SHA1_FF0(d,e,a,b,c,i++);
       SHA1_FF0(c,d,e,a,b,i++);
       SHA1_FF0(b,c,d,e,a,i++);
    }

    /* round two */
    for (; i < 40; )  { 
       SHA1_FF1(a,b,c,d,e,i++);
       SHA1_FF1(e,a,b,c,d,i++);
       SHA1_FF1(d,e,a,b,c,i++);
       SHA1_FF1(c,d,e,a,b,i++);
       SHA1_FF1(b,c,d,e,a,i++);
    }

    /* round three */
    for (; i < 60; )  { 
       SHA1_FF2(a,b,c,d,e,i++);
       SHA1_FF2(e,a,b,c,d,i++);
       SHA1_FF2(d,e,a,b,c,i++);
       SHA1_FF2(c,d,e,a,b,i++);
       SHA1_FF2(b,c,d,e,a,i++);
    }

    /* round four */
    for (; i < 80; )  { 
       SHA1_FF3(a,b,c,d,e,i++);
       SHA1_FF3(e,a,b,c,d,i++);
       SHA1_FF3(d,e,a,b,c,i++);
       SHA1_FF3(c,d,e,a,b,i++);
       SHA1_FF3(b,c,d,e,a,i++);
    }

    #undef SHA1_FF0
    #undef SHA1_FF1
    #undef SHA1_FF2
    #undef SHA1_FF3

    /* store */
    sha1->state[0] = sha1->state[0] + a;
    sha1->state[1] = sha1->state[1] + b;
    sha1->state[2] = sha1->state[2] + c;
    sha1->state[3] = sha1->state[3] + d;
    sha1->state[4] = sha1->state[4] + e;

    return CRYPT_OK;
}

/**
   Initialize the hash state
   @param md   The hash state you wish to initialize
   @return CRYPT_OK if successful
*/
int ctx_sha1_init(CtxSHA1 * sha1)
{
   assert(sha1 != NULL);
   sha1->state[0] = 0x67452301UL;
   sha1->state[1] = 0xefcdab89UL;
   sha1->state[2] = 0x98badcfeUL;
   sha1->state[3] = 0x10325476UL;
   sha1->state[4] = 0xc3d2e1f0UL;
   sha1->curlen = 0;
   sha1->length = 0;
   return CRYPT_OK;
}

/**
   Process a block of memory though the hash
   @param md     The hash state
   @param in     The data to hash
   @param inlen  The length of the data (octets)
   @return CRYPT_OK if successful
*/
HASH_PROCESS(ctx_sha1_process, ctx_sha1_compress, sha1, 64)

/**
   Terminate the hash to get the digest
   @param md  The hash state
   @param out [out] The destination of the hash (20 bytes)
   @return CRYPT_OK if successful
*/
int ctx_sha1_done(CtxSHA1 * sha1, unsigned char *out)
{
    int i;

    assert(sha1 != NULL);
    assert(out != NULL);

    if (sha1->curlen >= sizeof(sha1->buf)) {
       return -1;
    }

    /* increase the length of the message */
    sha1->length += sha1->curlen * 8;

    /* append the '1' bit */
    sha1->buf[sha1->curlen++] = (unsigned char)0x80;

    /* if the length is currently above 56 bytes we append zeros
     * then compress.  Then we can fall back to padding zeros and length
     * encoding like normal.
     */
    if (sha1->curlen > 56) {
        while (sha1->curlen < 64) {
            sha1->buf[sha1->curlen++] = (unsigned char)0;
        }
        ctx_sha1_compress(sha1, sha1->buf);
        sha1->curlen = 0;
    }

    /* pad upto 56 bytes of zeroes */
    while (sha1->curlen < 56) {
        sha1->buf[sha1->curlen++] = (unsigned char)0;
    }

    /* store length */
    STORE64H(sha1->length, sha1->buf+56);
    ctx_sha1_compress(sha1, sha1->buf);

    /* copy output */
    for (i = 0; i < 5; i++) {
        STORE32H(sha1->state[i], out+(4*i));
    }
    return CRYPT_OK;
}
#endif

#endif
#endif
#ifndef CTX_AUDIO_H
#define CTX_AUDIO_H

#if !__COSMOPOLITAN__
#include <stdint.h>
#endif

/* This enum should be kept in sync with the corresponding mmm enum.
 */
typedef enum {
  CTX_f32,
  CTX_f32S,
  CTX_s16,
  CTX_s16S
} CtxPCM;

void   ctx_pcm_set_format        (Ctx *ctx, CtxPCM format);
CtxPCM ctx_pcm_get_format        (Ctx *ctx);
int    ctx_pcm_get_sample_rate   (Ctx *ctx);
void   ctx_pcm_set_sample_rate   (Ctx *ctx, int sample_rate);
int    ctx_pcm_get_frame_chunk   (Ctx *ctx);
int    ctx_pcm_get_queued        (Ctx *ctx);
float  ctx_pcm_get_queued_length (Ctx *ctx);
int    ctx_pcm_queue             (Ctx *ctx, const int8_t *data, int frames);

#endif

#if CTX_IMPLEMENTATION
#if CTX_AUDIO

//#include <string.h>
//#include "ctx-internal.h"
//#include "mmm.h"

#if !__COSMOPOLITAN__

#include <pthread.h>
#if CTX_ALSA_AUDIO
#include <alsa/asoundlib.h>
#endif
#include <alloca.h>

#endif

#define DESIRED_PERIOD_SIZE 1000

int ctx_pcm_bytes_per_frame (CtxPCM format)
{
  switch (format)
  {
    case CTX_f32:  return 4;
    case CTX_f32S: return 8;
    case CTX_s16:  return 2;
    case CTX_s16S: return 4;
    default: return 1;
  }
}

static float    ctx_host_freq     = 48000;
static CtxPCM   ctx_host_format   = CTX_s16S;
static float    client_freq   = 48000;
static CtxPCM   ctx_client_format = CTX_s16S;
static int      ctx_pcm_queued    = 0;
static int      ctx_pcm_cur_left  = 0;
static CtxList *ctx_pcm_list;                 /* data is a blob a 32bit uint first, followed by pcm-data */


//static long int ctx_pcm_queued_ticks = 0;  /*  the number of ticks into the future
  //                                      *  we've queued audio for
                                       


int
ctx_pcm_channels (CtxPCM format)
{
  switch (format)
  {
    case CTX_s16:
    case CTX_f32:
      return 1;
    case CTX_s16S:
    case CTX_f32S:
      return 2;
  }
  return 0;
}

/* todo: only start audio thread on first write - enabling dynamic choice
 * of sample-rate? or is it better to keep to opening 48000 as a standard
 * and do better internal resampling for others?
 */

#if CTX_ALSA_AUDIO
static snd_pcm_t *alsa_open (char *dev, int rate, int channels)
{
   snd_pcm_hw_params_t *hwp;
   snd_pcm_sw_params_t *swp;
   snd_pcm_t *h;
   int r;
   int dir;
   snd_pcm_uframes_t period_size_min;
   snd_pcm_uframes_t period_size_max;
   snd_pcm_uframes_t period_size;
   snd_pcm_uframes_t buffer_size;

   if ((r = snd_pcm_open(&h, dev, SND_PCM_STREAM_PLAYBACK, 0) < 0))
           return NULL;

   hwp = alloca(snd_pcm_hw_params_sizeof());
   memset(hwp, 0, snd_pcm_hw_params_sizeof());
   snd_pcm_hw_params_any(h, hwp);

   snd_pcm_hw_params_set_access(h, hwp, SND_PCM_ACCESS_RW_INTERLEAVED);
   snd_pcm_hw_params_set_format(h, hwp, SND_PCM_FORMAT_S16_LE);
   snd_pcm_hw_params_set_rate(h, hwp, rate, 0);
   snd_pcm_hw_params_set_channels(h, hwp, channels);
   dir = 0;
   snd_pcm_hw_params_get_period_size_min(hwp, &period_size_min, &dir);
   dir = 0;
   snd_pcm_hw_params_get_period_size_max(hwp, &period_size_max, &dir);

   period_size = DESIRED_PERIOD_SIZE;

   dir = 0;
   r = snd_pcm_hw_params_set_period_size_near(h, hwp, &period_size, &dir);
   r = snd_pcm_hw_params_get_period_size(hwp, &period_size, &dir);
   buffer_size = period_size * 4;
   r = snd_pcm_hw_params_set_buffer_size_near(h, hwp, &buffer_size);
   r = snd_pcm_hw_params(h, hwp);
   swp = alloca(snd_pcm_sw_params_sizeof());
   memset(hwp, 0, snd_pcm_sw_params_sizeof());
   snd_pcm_sw_params_current(h, swp);
   r = snd_pcm_sw_params_set_avail_min(h, swp, period_size);
   snd_pcm_sw_params_set_start_threshold(h, swp, 0);
   r = snd_pcm_sw_params(h, swp);
   r = snd_pcm_prepare(h);

   return h;
}

static  snd_pcm_t *h = NULL;
static void *ctx_alsa_audio_start(Ctx *ctx)
{
//  Lyd *lyd = aux;
  int c;

  /* The audio handler is implemented as a mixer that adds data on top
   * of 0s, XXX: it should be ensured that minimal work is there is
   * no data available.
   */
  for (;;)
  {
    int client_channels = ctx_pcm_channels (ctx_client_format);
    int is_float = 0;
    int16_t data[81920*8]={0,};

    if (ctx_client_format == CTX_f32 ||
        ctx_client_format == CTX_f32S)
      is_float = 1;

    c = snd_pcm_wait(h, 1000);

    if (c >= 0)
       c = snd_pcm_avail_update(h);

    if (c > 1000) c = 1000; // should use max mmm buffer sizes

    if (c == -EPIPE)
      snd_pcm_prepare(h);

    if (c > 0)
    {
      int i;
      for (i = 0; i < c && ctx_pcm_cur_left; i ++)
      {
        if (ctx_pcm_cur_left)  //  XXX  this line can be removed
        {
          uint32_t *packet_sizep = (ctx_pcm_list->data);
          uint32_t packet_size = *packet_sizep;
          uint16_t left = 0, right = 0;

          if (is_float)
          {
            float *packet = (ctx_pcm_list->data);
            packet += 4;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;
            left = right = packet[0] * (1<<15);
            if (client_channels > 1)
              right = packet[0] * (1<<15);
          }
          else // s16
          {
            uint16_t *packet = (ctx_pcm_list->data);
            packet += 8;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;

            left = right = packet[0];
            if (client_channels > 1)
              right = packet[1];
          }
          data[i * 2 + 0] = left;
          data[i * 2 + 1] = right;

          ctx_pcm_cur_left--;
          ctx_pcm_queued --;
          if (ctx_pcm_cur_left == 0)
          {
            void *old = ctx_pcm_list->data;
            ctx_list_remove (&ctx_pcm_list, ctx_pcm_list->data);
            free (old);
            ctx_pcm_cur_left = 0;
            if (ctx_pcm_list)
            {
              uint32_t *packet_sizep = (ctx_pcm_list->data);
              uint32_t packet_size = *packet_sizep;
              ctx_pcm_cur_left = packet_size;
            }
          }
        }
      }

    c = snd_pcm_writei(h, data, c);
    if (c < 0)
      c = snd_pcm_recover (h, c, 0);
     }else{
      if (getenv("LYD_FATAL_UNDERRUNS"))
        {
          printf ("dying XXxx need to add API for this debug\n");
          //printf ("%i", lyd->active);
          exit(0);
        }
      fprintf (stderr, "ctx alsa underun\n");
      //exit(0);
    }
  }
}
#endif

static char MuLawCompressTable[256] =
{
   0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
   4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
   5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
   7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};

static unsigned char LinearToMuLawSample(int16_t sample)
{
  const int cBias = 0x84;
  const int cClip = 32635;
  int sign = (sample >> 8) & 0x80;

  if (sign)
    sample = (int16_t)-sample;

  if (sample > cClip)
    sample = cClip;

  sample = (int16_t)(sample + cBias);

  int exponent = (int)MuLawCompressTable[(sample>>7) & 0xFF];
  int mantissa = (sample >> (exponent+3)) & 0x0F;

  int compressedByte = ~ (sign | (exponent << 4) | mantissa);

  return (unsigned char)compressedByte;
}

void ctx_ctx_pcm (Ctx *ctx)
{
    int client_channels = ctx_pcm_channels (ctx_client_format);
    int is_float = 0;
    uint8_t data[81920*8]={0,};
    int c;

    if (ctx_client_format == CTX_f32 ||
        ctx_client_format == CTX_f32S)
      is_float = 1;

    c = 2000;

    if (c > 0)
    {
      int i;
      for (i = 0; i < c && ctx_pcm_cur_left; i ++)
      {
        if (ctx_pcm_cur_left)  //  XXX  this line can be removed
        {
          uint32_t *packet_sizep = (ctx_pcm_list->data);
          uint32_t packet_size = *packet_sizep;
          int left = 0, right = 0;

          if (is_float)
          {
            float *packet = (ctx_pcm_list->data);
            packet += 4;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;
            left = right = packet[0] * (1<<15);
            if (client_channels > 1)
              right = packet[1] * (1<<15);
          }
          else // s16
          {
            uint16_t *packet = (ctx_pcm_list->data);
            packet += 8;
            packet += (packet_size - ctx_pcm_cur_left) * client_channels;

            left = right = packet[0];
            if (client_channels > 1)
              right = packet[1];
          }
          data[i] = LinearToMuLawSample((left+right)/2);

          ctx_pcm_cur_left--;
          ctx_pcm_queued --;
          if (ctx_pcm_cur_left == 0)
          {
            void *old = ctx_pcm_list->data;
            ctx_list_remove (&ctx_pcm_list, ctx_pcm_list->data);
            free (old);
            ctx_pcm_cur_left = 0;
            if (ctx_pcm_list)
            {
              uint32_t *packet_sizep = (ctx_pcm_list->data);
              uint32_t packet_size = *packet_sizep;
              ctx_pcm_cur_left = packet_size;
            }
          }
        }
      }

    char encoded[81920*8]="";

    int encoded_len = ctx_a85enc (data, encoded, i);
    fprintf (stdout, "\033_Af=%i;", i);
    fwrite (encoded, 1, encoded_len, stdout);
    fwrite ("\e\\", 1, 2, stdout);
    fflush (stdout);
    }
}

int ctx_pcm_init (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return 0;
  }
  else
#endif
  if (ctx_renderer_is_ctx (ctx))
  {
     ctx_host_freq = 8000;
     ctx_host_format = CTX_s16;
#if 0
     pthread_t tid;
     pthread_create(&tid, NULL, (void*)ctx_audio_start, ctx);
#endif
  }
  else
  {
#if CTX_ALSA_AUDIO
     pthread_t tid;
     h = alsa_open("default", ctx_host_freq, ctx_pcm_channels (ctx_host_format));
  if (!h) {
    fprintf(stderr, "ctx unable to open ALSA device (%d channels, %f Hz), dying\n",
            ctx_pcm_channels (ctx_host_format), ctx_host_freq);
    return -1;
  }
  pthread_create(&tid, NULL, (void*)ctx_alsa_audio_start, ctx);
#endif
  }
  return 0;
}

int ctx_pcm_queue (Ctx *ctx, const int8_t *data, int frames)
{
  static int inited = 0;
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_queue (ctx->backend_data, data, frames);
  }
  else
#endif
  {
    if (!inited)
    {
      ctx_pcm_init (ctx);
      inited = 1;
    }
    float factor = client_freq * 1.0 / ctx_host_freq;
    int   scaled_frames = frames / factor;
    int   bpf = ctx_pcm_bytes_per_frame (ctx_client_format);

    uint8_t *packet = malloc (scaled_frames * ctx_pcm_bytes_per_frame (ctx_client_format) + 16);
    *((uint32_t *)packet) = scaled_frames;

    if (factor > 0.999 && factor < 1.0001)
    {
       memcpy (packet + 16, data, frames * bpf);
    }
    else
    {
      /* a crude nearest / sample-and hold resampler */
      int i;
      for (i = 0; i < scaled_frames; i++)
      {
        int source_frame = i * factor;
        memcpy (packet + 16 + bpf * i, data + source_frame * bpf, bpf);
      }
    }
    if (ctx_pcm_list == NULL)     // otherwise it is another frame at front
      ctx_pcm_cur_left = scaled_frames;  // and current cur_left is valid

    ctx_list_append (&ctx_pcm_list, packet);
    ctx_pcm_queued += scaled_frames;

    return frames;
  }
  return 0;
}

static int ctx_pcm_get_queued_frames (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_queued_frames (ctx->backend_data);
  }
#endif
  return ctx_pcm_queued;
}

int ctx_pcm_get_queued (Ctx *ctx)
{
  return ctx_pcm_get_queued_frames (ctx);
}

float ctx_pcm_get_queued_length (Ctx *ctx)
{
  return 1.0 * ctx_pcm_get_queued_frames (ctx) / ctx_host_freq;
}

int ctx_pcm_get_frame_chunk (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_frame_chunk (ctx->backend_data);
  }
#endif
  if (ctx_renderer_is_ctx (ctx))
  {
    // 300 stuttering
    // 350 nothing
    // 380 slight buzz
    // 390  buzzing
    // 400 ok - but sometimes falling out
    // 410 buzzing
    // 420 ok - but odd latency
    // 450 buzzing

    if (ctx_pcm_get_queued_frames (ctx) > 400)
      return 0;
    else
      return 400 - ctx_pcm_get_queued_frames (ctx);

  }

  if (ctx_pcm_get_queued_frames (ctx) > 1000)
    return 0;
  else
    return 1000 - ctx_pcm_get_queued_frames (ctx);
}

void ctx_pcm_set_sample_rate (Ctx *ctx, int sample_rate)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    mmm_pcm_set_sample_rate (ctx->backend_data, sample_rate);
  }
  else
#endif
    client_freq = sample_rate;
}

void ctx_pcm_set_format (Ctx *ctx, CtxPCM format)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    mmm_pcm_set_format (ctx->backend_data, format);
  }
  else
#endif
    ctx_client_format = format;
}

CtxPCM ctx_pcm_get_format (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_format (ctx->backend_data);
  }
#endif
  return ctx_client_format;
}

int ctx_pcm_get_sample_rate (Ctx *ctx)
{
#if 0
  if (!strcmp (ctx->backend->name, "mmm") ||
      !strcmp (ctx->backend->name, "mmm-client"))
  {
    return mmm_pcm_get_sample_rate (ctx->backend_data);
  }
#endif
  return client_freq;
}

#endif
 /* Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 */

#if CTX_FORMATTER

/* returns the maximum string length including terminating \0 */
int ctx_a85enc_len (int input_length)
{
  return (input_length / 4 + 1) * 5;
}

int ctx_a85enc (const void *srcp, char *dst, int count)
{
  const uint8_t *src = (uint8_t*)srcp;
  int out_len = 0;

  int padding = 4-(count % 4);
  if (padding == 4) padding = 0;

  for (int i = 0; i < (count+3)/4; i ++)
  {
    uint32_t input = 0;
    for (int j = 0; j < 4; j++)
    {
      input = (input << 8);
      if (i*4+j<=count)
        input += src[i*4+j];
    }

    int divisor = 85 * 85 * 85 * 85;
#if 0
    if (input == 0)
    {
        dst[out_len++] = 'z';
    }
    /* todo: encode 4 spaces as 'y' */
    else
#endif
    {
      for (int j = 0; j < 5; j++)
      {
        dst[out_len++] = ((input / divisor) % 85) + '!';
        divisor /= 85;
      }
    }
  }
  out_len -= padding;
  dst[out_len]=0;
  return out_len;
}
#endif

#if CTX_PARSER

int ctx_a85dec (const char *src, char *dst, int count)
{
  int out_len = 0;
  uint32_t val = 0;
  int k = 0;
  int i = 0;
  int p = 0;
  for (i = 0; i < count; i ++)
  {
    p = src[i];
    val *= 85;
    if (p == '~')
    {
      break;
    }
#if 0
    else if (p == 'z')
    {
      for (int j = 0; j < 4; j++)
        dst[out_len++] = 0;
      k = 0;
    }
    else if (p == 'y') /* lets support this extension */
    {
      for (int j = 0; j < 4; j++)
        dst[out_len++] = 32;
      k = 0;
    }
#endif
    else if (p >= '!' && p <= 'u')
    {
      val += p-'!';
      if (k % 5 == 4)
      {
         for (int j = 0; j < 4; j++)
         {
           dst[out_len++] = (val & (0xff << 24)) >> 24;
           val <<= 8;
         }
         val = 0;
      }
      k++;
    }
    // we treat all other chars as whitespace
  }
  if (p != '~')
  { 
    val *= 85;
  }
  k = k % 5;
  if (k)
  {
    val += 84;
    for (int j = k; j < 4; j++)
    {
      val *= 85;
      val += 84;
    }

    for (int j = 0; j < k-1; j++)
    {
      dst[out_len++] = (val & (0xff << 24)) >> 24;
      val <<= 8;
    }
    val = 0;
  }
  dst[out_len] = 0;
  return out_len;
}

#if 0
int ctx_a85len (const char *src, int count)
{
  int out_len = 0;
  int k = 0;
  for (int i = 0; i < count; i ++)
  {
    if (src[i] == '~')
      break;
    else if (src[i] == 'z')
    {
      for (int j = 0; j < 4; j++)
        out_len++;
      k = 0;
    }
    else if (src[i] >= '!' && src[i] <= 'u')
    {
      if (k % 5 == 4)
        out_len += 4;
      k++;
    }
    // we treat all other chars as whitespace
  }
  k = k % 5;
  if (k)
    out_len += k-1;
  return out_len;
}
#endif

#endif
#ifndef THASH_H
#define THASH_H

#if !__COSMOPOLITAN__
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#endif

#define THASH_NO_INTERNING   // ctx doesn't make use of thash_decode

#define THASH_ENTER_DIRECT     16

#define THASH_SPACE            0
#define THASH_DEC_OFFSET       29
#define THASH_INC_OFFSET       30
#define THASH_ENTER_UTF5       31
#define THASH_START_OFFSET     'l'
#define THASH_JUMP_OFFSET      27
#define THASH_MAXLEN           10

// todo: better whitespace handling for double version


static inline int thash_new_offset (uint32_t unichar)
{
   int offset = unichar % 32;
   return unichar - offset + 14; // this gives ~85% compression on test corpus
   return unichar; // this gives 88% compression on test corpus
}

static int thash_is_in_range (uint32_t offset, uint32_t unichar)
{
  if (unichar == 32)
    return 1;
  if (offset - unichar <= 13 ||
      unichar - offset <= 14)
      return 1;
  return 0;
}

static int thash_is_in_jump_range_dec (uint32_t offset, uint32_t unichar)
{
  return thash_is_in_range (offset - THASH_JUMP_OFFSET, unichar);
}

static int thash_is_in_jump_range_inc (uint32_t offset, uint32_t unichar)
{
  return thash_is_in_range (offset + THASH_JUMP_OFFSET, unichar);
}

//uint32_t ctx_utf8_to_unichar (const char *input);
//int ctx_unichar_to_utf8      (uint32_t ch, uint8_t  *dest);
//int ctx_utf8_len             (const unsigned char first_byte);

static int thash_utf5_length (uint32_t unichar)
{
  int octets = 0;
  if (unichar == 0) return 1;
  while (unichar)
  {  octets ++;
     unichar /= 16;
  }
  return octets;
}

typedef struct EncodeUtf5 {
  int      is_utf5;
  int      offset;
  int      length;
  void    *write_data;
  uint32_t current;
} EncodeUtf5;

void thash_encode_utf5 (const char *input, int inlen,
                   char *output, int *r_outlen)
{
  uint32_t offset = THASH_START_OFFSET;

  int is_utf5 = 1;
  int len = 0;

  for (int i = 0; i < inlen; i+= ctx_utf8_len (input[i]))
  {
    int val = ctx_utf8_to_unichar(&input[i]);
    int next_val = ' '; // always in range
    int next_next_val = ' ';
    int first_len = ctx_utf8_len (input[i]);
    if (i + first_len < inlen)
    {
        int next_len = ctx_utf8_to_unichar (&input[i + first_len]);
        if (i + first_len + next_len < inlen)
        {
          next_next_val = ctx_utf8_to_unichar (&input[i + first_len + next_len]);
        }
    }

    if (is_utf5)
    {
      int in_range = 
          thash_is_in_range (offset, val) +
          thash_is_in_range (offset, next_val) +
          thash_is_in_range (offset, next_next_val);
      int change_cost = 4;
      int no_change_cost = thash_utf5_length (val) + thash_utf5_length (next_val)
                                                   + thash_utf5_length (next_next_val);

      if (in_range > 2 && change_cost < no_change_cost)
      {
        output[len++] = THASH_ENTER_DIRECT;
        is_utf5 = 0;
      }
    } else
    {
      if (!thash_is_in_range(offset, val))
      {
        if (thash_is_in_jump_range_dec (offset, val))
        {
            output[len++] = THASH_DEC_OFFSET;
            offset -= THASH_JUMP_OFFSET;
        }
        else if (thash_is_in_jump_range_inc (offset, val))
        {
          output[len++] = THASH_INC_OFFSET;
          offset += THASH_JUMP_OFFSET;
        }
        else
        {
          output[len++] = THASH_ENTER_UTF5;
          is_utf5 = 1;
        }
      }
    }

    if (is_utf5)
    {
          int octets = 0;
          offset = thash_new_offset (val);
          while (val)
          {
            int oval = val % 16;
            int last = 0;
            if (val / 32 == 0) last = 16;
            output[len+ (octets++)] = oval + last;
            val /= 16;
          }
          for (int j = 0; j < octets/2; j++) // mirror in-place
          {
            int tmp = output[len+j];
            output[len+j] = output[len+octets-1-j];
            output[len+octets-1-j] = tmp;
          }
          len += octets;
        }
        else 
      {
        if (val == 32)
        {
          output[len++] = THASH_SPACE;
        }
        else
        {
          output[len++]= val-offset+14;
        }
      }
  }
  if (len && output[len-1]==0)
    output[len++] = 16;
  output[len]=0;
  *r_outlen = len;
}

uint64_t _thash (const char *utf8)
{
  char encoded[4096]="";
  int  encoded_len=0;
  int  wordlen = 0;
  thash_encode_utf5 (utf8, strlen (utf8), encoded, &encoded_len);
#if 0
  Word word = {0};
  word.utf5 = (encoded[0] != THASH_ENTER_DIRECT);
  for (int i = !word.utf5; i < encoded_len; i++)
    word_set_val (&word, wordlen++, encoded[i]);
  return word.hash;
#else
  uint64_t hash = 0;
  int  utf5 = (encoded[0] != THASH_ENTER_DIRECT);
  for (int i = !utf5; i < encoded_len; i++)
  {
    uint64_t val = encoded[i];

    if (wordlen < THASH_MAXLEN)
    {
      hash = hash | (val << (5*wordlen));
      hash &= (((uint64_t)1<<52)-1);
    }
    else
    {
      hash = hash ^ ((hash << 4) + val);
      hash &= (((uint64_t)1<<52)-1);
    }
    wordlen++;
  }
  hash <<= 1;
  if (wordlen >= THASH_MAXLEN) 
    hash |= ((uint64_t)1<<51); // overflowed
  return hash |  utf5;
#endif
}

typedef struct _Interned Interned;

struct _Interned {
    uint64_t   hash;
    char      *string;
};

static Interned *interned = NULL;
static int n_interned = 0;
static int s_interned = 0;
static int interned_sorted = 1;

static int interned_compare (const void *a, const void *b)
{
  const Interned *ia = (Interned*)a;
  const Interned *ib = (Interned*)b;
  if (ia->hash < ib->hash ) return -1;
  else if (ia->hash > ib->hash ) return 1;
  return 0;
}


uint64_t thash (const char *utf8)
{
  uint64_t hash = _thash (utf8);
#ifdef THASH_NO_INTERNING
  return hash;
#endif
  if (hash & ((uint64_t)1<<51)) /* overflowed */
  {
    int i;
    for (i = 0; i < n_interned; i++)
    {
      Interned *entry = &interned[i];
      if (entry->hash == hash)
        return hash;
    }

    if (n_interned + 1 >= s_interned)
    {
       s_interned = (s_interned + 128)*1.5;
       //fprintf (stderr, "\r%p %i ", interned, s_interned);
       interned = (Interned*)realloc (interned, s_interned * sizeof (Interned));
    }

    {
      Interned *entry = &interned[n_interned];
      entry->hash = hash;
      entry->string = strdup (utf8);
    }
    n_interned++;
    interned_sorted = 0;
  }
  return hash;
}
uint64_t ctx_strhash(const char *str, int ignored) { return thash (str);}

typedef struct ThashUtf5Dec {
  int      is_utf5;
  int      offset;
  void    *write_data;
  uint32_t current;
  void   (*append_unichar) (uint32_t unichar, void *write_data);
} ThashUtf5Dec;

typedef struct ThashUtf5DecDefaultData {
   uint8_t *buf;
   int     length;
} ThashUtf5DecDefaultData;

static void thash_decode_utf5_append_unichar_as_utf8 (uint32_t unichar, void *write_data)
{
  ThashUtf5DecDefaultData *data = (ThashUtf5DecDefaultData*)write_data;
  unsigned char utf8[8]="";
  utf8[ctx_unichar_to_utf8 (unichar, utf8)]=0;
  for (int j = 0; utf8[j]; j++)
    data->buf[data->length++]=utf8[j];
  data->buf[data->length]=0;
}

void thash_decode_utf5 (ThashUtf5Dec *dec, uint8_t in)
{
  if (dec->is_utf5)
  {
      if (in > 16)
      {
        if (dec->current)
        {
          dec->offset = thash_new_offset (dec->current);
          dec->append_unichar (dec->current, dec->write_data);
          dec->current = 0;
        }
      }
      if (in == THASH_ENTER_DIRECT)
      {
        if (dec->current)
        {
          dec->offset = thash_new_offset (dec->current);
          dec->append_unichar (dec->current, dec->write_data);
          dec->current = 0;
        }
        dec->is_utf5 = 0;
      }
      else
      {
        dec->current = dec->current * 16 + (in % 16);
      }
  }
  else
  {
      switch (in)
      {
        case THASH_ENTER_UTF5: dec->is_utf5 = 1;  break;
        case THASH_SPACE:      dec->append_unichar (' ', dec->write_data); break;
        case THASH_DEC_OFFSET: dec->offset -= THASH_JUMP_OFFSET; break;
        case THASH_INC_OFFSET: dec->offset += THASH_JUMP_OFFSET; break;
        default:
          dec->append_unichar (dec->offset + in - 14, dec->write_data);
      }
  }
}

void thash_decode_utf5_bytes (int is_utf5, 
                        const unsigned char *input, int inlen,
                        char *output, int *r_outlen)
{
  ThashUtf5DecDefaultData append_data= {(unsigned char*)output, };
  ThashUtf5Dec dec = {is_utf5,
                    THASH_START_OFFSET,
                    &append_data,
  0, thash_decode_utf5_append_unichar_as_utf8
  };
  for (int i = 0; i < inlen; i++)
  {
    thash_decode_utf5 (&dec, input[i]);
  }
  if (dec.current)
    dec.append_unichar (dec.current, dec.write_data);
  if (r_outlen)*r_outlen = append_data.length;
}

const char *thash_decode (uint64_t hash)
{
  if (!interned_sorted && interned)
  {
    qsort (interned, n_interned, sizeof (Interned), interned_compare);
    interned_sorted = 1;
  }
  if (hash &  ((uint64_t)1<<51))
  {

    for (int i = 0; i < n_interned; i++)
    {
      Interned *entry = &interned[i];
      if (entry->hash == hash)
        return entry->string;
    }
    return "[missing string]";
  }

  static char ret[4096]="";
  uint8_t utf5[40]="";
  uint64_t tmp = hash & (((uint64_t)1<<51)-1);
  int len = 0;
  int is_utf5 = tmp & 1;
  tmp /= 2;
  int in_utf5 = is_utf5;
  while (tmp > 0)
  {
    uint64_t remnant = tmp % 32;
    uint64_t val = remnant;

    if      ( in_utf5 && val == THASH_ENTER_DIRECT) in_utf5 = 0;
    else if (!in_utf5 && val == THASH_ENTER_UTF5) in_utf5 = 1;

    utf5[len++] = val;
    tmp -= remnant;
    tmp /= 32;
  }
  if (in_utf5 && len && utf5[len-1] > 'G')
  {
    utf5[len++] = 0;//utf5_alphabet[0]; 
  }
  utf5[len]=0;
  int retlen = sizeof (ret);
  thash_decode_utf5_bytes (is_utf5, utf5, len, ret, &retlen);
  ret[len]=0;
  return ret;
}

#if 0

#include <assert.h>
#pragma pack(push,1)
typedef union Word
{
  uint64_t hash;
  struct {
    unsigned int utf5:1;
    unsigned int c0:5;
    unsigned int c1:5;
    unsigned int c2:5;
    unsigned int c3:5;
    unsigned int c4:5;
    unsigned int c5:5;
    unsigned int c6:5;
    unsigned int c7:5;
    unsigned int c8:5;
    unsigned int c9:5;
    unsigned int overflowed:1;
  };
} Word;

static inline void word_set_val (Word *word, int no, int val)
{
#if 0
  switch(no)
  {
     case 0: word->c0 = val; break;
     case 1: word->c1 = val; break;
     case 2: word->c2 = val; break;
     case 3: word->c3 = val; break;
     case 4: word->c4 = val; break;
     case 5: word->c5 = val; break;
     case 6: word->c6 = val; break;
     case 7: word->c7 = val; break;
     case 8: word->c8 = val; break;
     case 9: word->c9 = val; break;
     default:
       // for overflow only works when setting all in sequence
       word->hash = word->hash + ((uint64_t)(val) << (5*no+1));
       word->overflowed = 1;
       break;
  }
#else
  word->hash = word->hash | ((uint64_t)(val) << (5*no+1));
  if (no >= 9) 
    word->hash |= ((uint64_t)1<<51);
  word->hash &= (((uint64_t)1<<52)-1);
#endif
}

static inline int word_get_val (Word *word, int no)
{
  switch(no)
  {
     case 0: return word->c0;break;
     case 1: return word->c1;break;
     case 2: return word->c2;break;
     case 3: return word->c3;break;
     case 4: return word->c4;break;
     case 5: return word->c5;break;
     case 6: return word->c6;break;
     case 7: return word->c7;break;
     case 8: return word->c8;break;
     case 9: return word->c9;break;
  }
}

static inline int word_get_length (Word *word)
{
   int len = 0;
   if (word->c0) len ++; else return len;
   if (word->c1) len ++; else return len;
   if (word->c2) len ++; else return len;
   if (word->c3) len ++; else return len;
   if (word->c4) len ++; else return len;
   if (word->c5) len ++; else return len;
   if (word->c6) len ++; else return len;
   if (word->c7) len ++; else return len;
   if (word->c8) len ++; else return len;
   if (word->c9) len ++; else return len;
   return len;
}


static Word *word_append_unichar (Word *word, uint32_t unichar)
{
  //word_set_char (word, word_get_length (word), unichar);
  // append unichar - possibly advancing.
  return word;
}
#endif

#endif
/* atty - audio interface and driver for terminals
 * Copyright (C) 2020 Øyvind Kolås <pippin@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>. 
 */

static const char *base64_map="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
static void bin2base64_group (const unsigned char *in, int remaining, char *out)
{
  unsigned char digit[4] = {0,0,64,64};
  int i;
  digit[0] = in[0] >> 2;
  digit[1] = ((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4);
  if (remaining > 1)
    {
      digit[2] = ((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6);
      if (remaining > 2)
        digit[3] = ((in[2] & 0x3f));
    }
  for (i = 0; i < 4; i++)
    out[i] = base64_map[digit[i]];
}

void
ctx_bin2base64 (const void *bin,
                int         bin_length,
                char       *ascii)
{
  /* this allocation is a hack to ensure we always produce the same result,
   * regardless of padding data accidentally taken into account.
   */
  unsigned char *bin2 = (unsigned char*)calloc (bin_length + 4, 1);
  unsigned const char *p = bin2;
  int i;
  memcpy (bin2, bin, bin_length);
  for (i=0; i*3 < bin_length; i++)
   {
     int remaining = bin_length - i*3;
     bin2base64_group (&p[i*3], remaining, &ascii[i*4]);
   }
  free (bin2);
  ascii[i*4]=0;
}

static unsigned char base64_revmap[255];
static void base64_revmap_init (void)
{
  static int done = 0;
  if (done)
    return;

  for (int i = 0; i < 255; i ++)
    base64_revmap[i]=255;
  for (int i = 0; i < 64; i ++)
    base64_revmap[((const unsigned char*)base64_map)[i]]=i;
  /* include variants used in URI encodings for decoder,
   * even if that is not how we encode
  */
  base64_revmap['-']=62;
  base64_revmap['_']=63;
  base64_revmap['+']=62;
  base64_revmap['/']=63;

  done = 1;
}


int
ctx_base642bin (const char    *ascii,
                int           *length,
                unsigned char *bin)
{
  int i;
  int charno = 0;
  int outputno = 0;
  int carry = 0;
  base64_revmap_init ();
  for (i = 0; ascii[i]; i++)
    {
      int bits = base64_revmap[((const unsigned char*)ascii)[i]];
      if (length && outputno > *length)
        {
          *length = -1;
          return -1;
        }
      if (bits != 255)
        {
          switch (charno % 4)
            {
              case 0:
                carry = bits;
                break;
              case 1:
                bin[outputno] = (carry << 2) | (bits >> 4);
                outputno++;
                carry = bits & 15;
                break;
              case 2:
                bin[outputno] = (carry << 4) | (bits >> 2);
                outputno++;
                carry = bits & 3;
                break;
              case 3:
                bin[outputno] = (carry << 6) | bits;
                outputno++;
                carry = 0;
                break;
            }
          charno++;
        }
    }
  bin[outputno]=0;
  if (length)
    *length= outputno;
  return outputno;
}
#ifndef CTX_DRAWLIST_H
#define CTX_DRAWLIST_H

static int
ctx_conts_for_entry (CtxEntry *entry);
static void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxDrawlist  *drawlist,
                   int               start_pos,
                   int               flags);

int ctx_iterator_pos (CtxIterator *iterator);

static void
ctx_drawlist_resize (CtxDrawlist *drawlist, int desired_size);
static int
ctx_drawlist_add_single (CtxDrawlist *drawlist, CtxEntry *entry);
static int ctx_drawlist_add_entry (CtxDrawlist *drawlist, CtxEntry *entry);
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry);
int
ctx_add_data (Ctx *ctx, void *data, int length);

int ctx_drawlist_add_u32 (CtxDrawlist *drawlist, CtxCode code, uint32_t u32[2]);
int ctx_drawlist_add_data (CtxDrawlist *drawlist, const void *data, int length);

static CtxEntry
ctx_void (CtxCode code);
static CtxEntry
ctx_f (CtxCode code, float x, float y);
static CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y);
#if 0
static CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y);
#endif

static inline CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1);
static CtxEntry
ctx_u8 (CtxCode code,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d,
        uint8_t e, uint8_t f, uint8_t g, uint8_t h);

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry command = ctx_void (cmd); \
  ctx_process (ctx, &command);}while(0) \

#define CTX_PROCESS_F(cmd, x, y) do {\
  CtxEntry command = ctx_f(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_F1(cmd, x) do {\
  CtxEntry command = ctx_f(cmd, x, 0);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry command = ctx_u32(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U8(cmd, x) do {\
  CtxEntry command = ctx_u8(cmd, x,0,0,0,0,0,0,0);\
  ctx_process (ctx, &command);}while(0)


#if CTX_BITPACK_PACKER
static int
ctx_last_history (CtxDrawlist *drawlist);
#endif

#if CTX_BITPACK_PACKER
static void
ctx_drawlist_remove_tiny_curves (CtxDrawlist *drawlist, int start_pos);

static void
ctx_drawlist_bitpack (CtxDrawlist *drawlist, int start_pos);
#endif

static void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1);
static void
ctx_process_cmd_str_float (Ctx *ctx, CtxCode code, const char *string, float arg0, float arg1);
static void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len);

#endif

#ifndef __CTX_UTIL_H
#define __CTX_UTIL_H

inline static float ctx_fast_hypotf (float x, float y)
{
  if (x < 0) { x = -x; }
  if (y < 0) { y = -y; }
  if (x < y)
    { return 0.96f * y + 0.4f * x; }
  else
    { return 0.96f * x + 0.4f * y; }
}

static int ctx_str_is_number (const char *str)
{
  int got_digit = 0;
  for (int i = 0; str[i]; i++)
  {
    if (str[i] >= '0' && str[i] <= '9')
    {
       got_digit ++;
    }
    else if (str[i] == '.')
    {
    }
    else
      return 0;
  }
  if (got_digit)
    return 1;
  return 0;
}

#if CTX_FONTS_FROM_FILE

typedef struct CtxFileContent
{
  char *path;
  unsigned char *contents;
  long  length;
  int   free_data;
} CtxFileContent;

CtxList *registered_contents = NULL;

void
ctx_register_contents (const char *path,
                       const unsigned char *contents,
                       long length,
                       int  free_data)
{
  // if (path[0] != '/') && strchr(path, ':')) 
  //   with this check regular use is faster, but we lose
  //   generic filesystem overrides..
  for (CtxList *l = registered_contents; l; l = l->next)
  {
    CtxFileContent *c = (CtxFileContent*)l->data;
    if (!strcmp (c->path, path))
    {
       if (c->free_data)
       {
         free (c->contents);
       }
       c->free_data = free_data;
       c->contents = (unsigned char*)contents;
       c->length = length;
       return;
    }
  }
  CtxFileContent *c = (CtxFileContent*)calloc (sizeof (CtxFileContent), 1);
  c->free_data = free_data;
  c->contents = (unsigned char*)contents;
  c->length    = length;
  ctx_list_append (&registered_contents, c);
}

void
_ctx_file_set_contents (const char     *path,
                        const unsigned char  *contents,
                        long            length)
{
  FILE *file;
  file = fopen (path, "wb");
  if (!file)
    { return; }
  if (length < 0) length = strlen ((const char*)contents);
  fwrite (contents, 1, length, file);
  fclose (file);
}

static int
__ctx_file_get_contents (const char     *path,
                        unsigned char **contents,
                        long           *length)
{
  FILE *file;
  long  size;
  long  remaining;
  char *buffer;
  file = fopen (path, "rb");
  if (!file)
    { return -1; }
  fseek (file, 0, SEEK_END);
  size = remaining = ftell (file);
  if (length)
    { *length =size; }
  rewind (file);
  buffer = (char*)malloc (size + 8);
  if (!buffer)
    {
      fclose (file);
      return -1;
    }
  remaining -= fread (buffer, 1, remaining, file);
  if (remaining)
    {
      fclose (file);
      free (buffer);
      return -1;
    }
  fclose (file);
  *contents = (unsigned char*) buffer;
  buffer[size] = 0;
  return 0;
}

#if !__COSMOPOLITAN__
#include <limits.h>
#endif




#endif


#endif


static int
ctx_conts_for_entry (CtxEntry *entry)
{
    switch (entry->code)
    {
      case CTX_DATA:
        return entry->data.u32[1];
      case CTX_LINEAR_GRADIENT:
      //case CTX_DEFINE_TEXTURE:
        return 1;
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
      case CTX_COLOR:
      case CTX_ROUND_RECTANGLE:
      case CTX_SHADOW_COLOR:
        return 2;
      case CTX_FILL_RECT:
      case CTX_STROKE_RECT:
      case CTX_RECTANGLE:
      case CTX_VIEW_BOX:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
        return 1;

      case CTX_TEXT:
      case CTX_LINE_DASH:
      case CTX_COLOR_SPACE:
      case CTX_STROKE_TEXT:
      case CTX_FONT:
      case CTX_TEXTURE:
        {
          int eid_len = entry[1].data.u32[1];
          return eid_len + 1;
        }
      case CTX_DEFINE_TEXTURE:
        {
          int eid_len = entry[2].data.u32[1];
          int pix_len = entry[2 + eid_len + 1].data.u32[1];
          return eid_len + pix_len + 2 + 1;
        }

      default:
        return 0;
    }
}

// expanding arc_to to arc can be the job
// of a layer in front of renderer?
//   doing:
//     rectangle
//     arc
//     ... etc reduction to beziers
//     or even do the reduction to
//     polylines directly here...
//     making the rasterizer able to
//     only do poly-lines? will that be faster?

/* the iterator - should decode bitpacked data as well -
 * making the rasterizers simpler, possibly do unpacking
 * all the way to absolute coordinates.. unless mixed
 * relative/not are wanted.
 */


static void
ctx_iterator_init (CtxIterator      *iterator,
                   CtxDrawlist  *drawlist,
                   int               start_pos,
                   int               flags)
{
  iterator->drawlist   = drawlist;
  iterator->flags          = flags;
  iterator->bitpack_pos    = 0;
  iterator->bitpack_length = 0;
  iterator->pos            = start_pos;
  iterator->end_pos        = drawlist->count;
  iterator->first_run      = 1; // -1 is a marker used for first run
  ctx_memset (iterator->bitpack_command, 0, sizeof (iterator->bitpack_command) );
}

int ctx_iterator_pos (CtxIterator *iterator)
{
  return iterator->pos;
}

static CtxEntry *_ctx_iterator_next (CtxIterator *iterator)
{
  int ret = iterator->pos;
  CtxEntry *entry = &iterator->drawlist->entries[ret];
  if (ret >= iterator->end_pos)
    { return NULL; }
  if (iterator->first_run == 0)
    { iterator->pos += (ctx_conts_for_entry (entry) + 1); }
  iterator->first_run = 0;
  if (iterator->pos >= iterator->end_pos)
    { return NULL; }
  return &iterator->drawlist->entries[iterator->pos];
}

// 6024x4008
#if CTX_BITPACK
static void
ctx_iterator_expand_s8_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 4; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] =
        entry->data.s8[no] * 1.0f / CTX_SUBDIV;
  iterator->bitpack_command[0].code =
    iterator->bitpack_command[1].code =
      iterator->bitpack_command[2].code =
        iterator->bitpack_command[3].code = CTX_CONT;
  iterator->bitpack_length = 4;
  iterator->bitpack_pos = 0;
}

static void
ctx_iterator_expand_s16_args (CtxIterator *iterator, CtxEntry *entry)
{
  int no = 0;
  for (int cno = 0; cno < 2; cno++)
    for (int d = 0; d < 2; d++, no++)
      iterator->bitpack_command[cno].data.f[d] = entry->data.s16[no] * 1.0f /
          CTX_SUBDIV;
  iterator->bitpack_command[0].code =
    iterator->bitpack_command[1].code = CTX_CONT;
  iterator->bitpack_length = 2;
  iterator->bitpack_pos    = 0;
}
#endif

CtxCommand *
ctx_iterator_next (CtxIterator *iterator)
{
  CtxEntry *ret;
#if CTX_BITPACK
  int expand_bitpack = iterator->flags & CTX_ITERATOR_EXPAND_BITPACK;
again:
  if (iterator->bitpack_length)
    {
      ret = &iterator->bitpack_command[iterator->bitpack_pos];
      iterator->bitpack_pos += (ctx_conts_for_entry (ret) + 1);
      if (iterator->bitpack_pos >= iterator->bitpack_length)
        {
          iterator->bitpack_length = 0;
        }
      return (CtxCommand *) ret;
    }
#endif
  ret = _ctx_iterator_next (iterator);
#if CTX_BITPACK
  if (ret && expand_bitpack)
    switch ((CtxCode)(ret->code))
      {
        case CTX_REL_CURVE_TO_REL_LINE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
          iterator->bitpack_command[1].code =
          iterator->bitpack_command[2].code = CTX_CONT;
          iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
          // 0.0 here is a common optimization - so check for it
          if (ret->data.s8[6]== 0 && ret->data.s8[7] == 0)
            { iterator->bitpack_length = 3; }
          else
            iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_LINE_TO_REL_CURVE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
          iterator->bitpack_command[1].code = CTX_REL_CURVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_CURVE_TO;
          iterator->bitpack_command[3].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_LINE_TO_X4:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[1].code =
          iterator->bitpack_command[2].code =
          iterator->bitpack_command[3].code = CTX_REL_LINE_TO;
          iterator->bitpack_length          = 4;
          goto again;
        case CTX_REL_QUAD_TO_S16:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_QUAD_TO;
          iterator->bitpack_length          = 1;
          goto again;
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
          ctx_iterator_expand_s8_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[2].code = CTX_REL_QUAD_TO;
          iterator->bitpack_length          = 3;
          goto again;
        case CTX_REL_LINE_TO_X2:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code =
          iterator->bitpack_command[1].code = CTX_REL_LINE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_REL_LINE_TO_REL_MOVE_TO:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_REL_LINE_TO;
          iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_MOVE_TO_REL_LINE_TO:
          ctx_iterator_expand_s16_args (iterator, ret);
          iterator->bitpack_command[0].code = CTX_MOVE_TO;
          iterator->bitpack_command[1].code = CTX_REL_MOVE_TO;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_FILL_MOVE_TO:
          iterator->bitpack_command[1]      = *ret;
          iterator->bitpack_command[0].code = CTX_FILL;
          iterator->bitpack_command[1].code = CTX_MOVE_TO;
          iterator->bitpack_pos             = 0;
          iterator->bitpack_length          = 2;
          goto again;
        case CTX_LINEAR_GRADIENT:
        case CTX_QUAD_TO:
        case CTX_REL_QUAD_TO:
        case CTX_TEXTURE:
        case CTX_RECTANGLE:
        case CTX_VIEW_BOX:
        case CTX_ARC:
        case CTX_ARC_TO:
        case CTX_REL_ARC_TO:
        case CTX_COLOR:
        case CTX_SHADOW_COLOR:
        case CTX_RADIAL_GRADIENT:
        case CTX_CURVE_TO:
        case CTX_REL_CURVE_TO:
        case CTX_APPLY_TRANSFORM:
        case CTX_ROUND_RECTANGLE:
        case CTX_TEXT:
        case CTX_STROKE_TEXT:
        case CTX_FONT:
        case CTX_LINE_DASH:
        case CTX_FILL:
        case CTX_NOP:
        case CTX_MOVE_TO:
        case CTX_LINE_TO:
        case CTX_REL_MOVE_TO:
        case CTX_REL_LINE_TO:
        case CTX_VER_LINE_TO:
        case CTX_REL_VER_LINE_TO:
        case CTX_HOR_LINE_TO:
        case CTX_REL_HOR_LINE_TO:
        case CTX_ROTATE:
        case CTX_FLUSH:
        case CTX_TEXT_ALIGN:
        case CTX_TEXT_BASELINE:
        case CTX_TEXT_DIRECTION:
        case CTX_MITER_LIMIT:
        case CTX_GLOBAL_ALPHA:
        case CTX_COMPOSITING_MODE:
        case CTX_BLEND_MODE:
        case CTX_SHADOW_BLUR:
        case CTX_SHADOW_OFFSET_X:
        case CTX_SHADOW_OFFSET_Y:
        case CTX_RESET:
        case CTX_EXIT:
        case CTX_BEGIN_PATH:
        case CTX_CLOSE_PATH:
        case CTX_SAVE:
        case CTX_CLIP:
        case CTX_PRESERVE:
        case CTX_DEFINE_GLYPH:
        case CTX_IDENTITY:
        case CTX_FONT_SIZE:
        case CTX_START_GROUP:
        case CTX_END_GROUP:
        case CTX_RESTORE:
        case CTX_LINE_WIDTH:
        case CTX_LINE_DASH_OFFSET:
        case CTX_STROKE:
        case CTX_KERNING_PAIR:
        case CTX_SCALE:
        case CTX_GLYPH:
        case CTX_SET_PIXEL:
        case CTX_FILL_RULE:
        case CTX_LINE_CAP:
        case CTX_LINE_JOIN:
        case CTX_NEW_PAGE:
        case CTX_SET_KEY:
        case CTX_TRANSLATE:
        case CTX_DEFINE_TEXTURE:
        case CTX_GRADIENT_STOP:
        case CTX_DATA: // XXX : would be better if we hide the DATAs
        case CTX_CONT: // shouldnt happen
        default:
          iterator->bitpack_length = 0;
          return (CtxCommand *) ret;
#if 0
        default: // XXX remove - and get better warnings
          iterator->bitpack_command[0] = ret[0];
          iterator->bitpack_command[1] = ret[1];
          iterator->bitpack_command[2] = ret[2];
          iterator->bitpack_command[3] = ret[3];
          iterator->bitpack_command[4] = ret[4];
          iterator->bitpack_pos = 0;
          iterator->bitpack_length = 1;
          goto again;
#endif
      }
#endif
  return (CtxCommand *) ret;
}

static void ctx_drawlist_compact (CtxDrawlist *drawlist);
static void
ctx_drawlist_resize (CtxDrawlist *drawlist, int desired_size)
{
#if CTX_DRAWLIST_STATIC
  if (drawlist->flags & CTX_DRAWLIST_EDGE_LIST)
    {
      static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = &sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (drawlist->flags & CTX_DRAWLIST_CURRENT_PATH)
    {
      static CtxEntry sbuf[CTX_MAX_EDGE_LIST_SIZE];
      drawlist->entries = &sbuf[0];
      drawlist->size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      static CtxEntry sbuf[CTX_MAX_JOURNAL_SIZE];
      drawlist->entries = &sbuf[0];
      drawlist->size = CTX_MAX_JOURNAL_SIZE;
      ctx_drawlist_compact (drawlist);
    }
#else
  int new_size = desired_size;
  int min_size = CTX_MIN_JOURNAL_SIZE;
  int max_size = CTX_MAX_JOURNAL_SIZE;
  if ((drawlist->flags & CTX_DRAWLIST_EDGE_LIST))
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (drawlist->flags & CTX_DRAWLIST_CURRENT_PATH)
    {
      min_size = CTX_MIN_EDGE_LIST_SIZE;
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else
    {
      ctx_drawlist_compact (drawlist);
    }

  if (new_size < drawlist->size)
    { return; }
  if (drawlist->size == max_size)
    { return; }
  if (new_size < min_size)
    { new_size = min_size; }
  if (new_size < drawlist->count)
    { new_size = drawlist->count + 4; }
  if (new_size >= max_size)
    { new_size = max_size; }
  if (new_size != drawlist->size)
    {
      //fprintf (stderr, "growing drawlist %p %i to %d from %d\n", drawlist, drawlist->flags, new_size, drawlist->size);
  if (drawlist->entries)
    {
      //printf ("grow %p to %d from %d\n", drawlist, new_size, drawlist->size);
      CtxEntry *ne =  (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
      memcpy (ne, drawlist->entries, drawlist->size * sizeof (CtxEntry) );
      free (drawlist->entries);
      drawlist->entries = ne;
      //drawlist->entries = (CtxEntry*)malloc (drawlist->entries, sizeof (CtxEntry) * new_size);
    }
  else
    {
      //fprintf (stderr, "allocating for %p %d\n", drawlist, new_size);
      drawlist->entries = (CtxEntry *) malloc (sizeof (CtxEntry) * new_size);
    }
  drawlist->size = new_size;
    }
  //fprintf (stderr, "drawlist %p is %d\n", drawlist, drawlist->size);
#endif
}

static int
ctx_drawlist_add_single (CtxDrawlist *drawlist, CtxEntry *entry)
{
  int max_size = CTX_MAX_JOURNAL_SIZE;
  int ret = drawlist->count;
  if (drawlist->flags & CTX_DRAWLIST_EDGE_LIST)
    {
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  else if (drawlist->flags & CTX_DRAWLIST_CURRENT_PATH)
    {
      max_size = CTX_MAX_EDGE_LIST_SIZE;
    }
  if (drawlist->flags & CTX_DRAWLIST_DOESNT_OWN_ENTRIES)
    {
      return ret;
    }
  if (ret + 1024 >= drawlist->size - 40)
    {
      int new_ = CTX_MAX (drawlist->size * 2, ret + 1024);
      ctx_drawlist_resize (drawlist, new_);
    }

  if (drawlist->count >= max_size - 20)
    {
      return 0;
    }
  drawlist->entries[drawlist->count] = *entry;
  ret = drawlist->count;
  drawlist->count++;
  return ret;
}

int
ctx_add_single (Ctx *ctx, void *entry)
{
  return ctx_drawlist_add_single (&ctx->drawlist, (CtxEntry *) entry);
}

static int
ctx_drawlist_add_entry (CtxDrawlist *drawlist, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int ret = 0;
  for (int i = 0; i < length; i ++)
    {
      ret = ctx_drawlist_add_single (drawlist, &entry[i]);
    }
  return ret;
}

#if 0
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int tmp_pos = ctx_drawlist_add_entry (drawlist, entry);
  for (int i = 0; i < length; i++)
  {
    for (int j = pos + i + 1; j < tmp_pos; j++)
      drawlist->entries[j] = entry[j-1];
    drawlist->entries[pos + i] = entry[i];
  }
  return pos;
}
#endif
int
ctx_drawlist_insert_entry (CtxDrawlist *drawlist, int pos, CtxEntry *entry)
{
  int length = ctx_conts_for_entry (entry) + 1;
  int tmp_pos = ctx_drawlist_add_entry (drawlist, entry);
#if 1
  for (int i = 0; i < length; i++)
  {
    for (int j = tmp_pos; j > pos + i; j--)
      drawlist->entries[j] = drawlist->entries[j-1];
    drawlist->entries[pos + i] = entry[i];
  }
  return pos;
#endif
  return tmp_pos;
}

int ctx_append_drawlist (Ctx *ctx, void *data, int length)
{
  CtxEntry *entries = (CtxEntry *) data;
  if (length % sizeof (CtxEntry) )
    {
      ctx_log("drawlist not multiple of 9\n");
      return -1;
    }
  for (unsigned int i = 0; i < length / sizeof (CtxEntry); i++)
    {
      ctx_drawlist_add_single (&ctx->drawlist, &entries[i]);
    }
  return 0;
}

int ctx_set_drawlist (Ctx *ctx, void *data, int length)
{
  CtxDrawlist *drawlist = &ctx->drawlist;
  ctx->drawlist.count = 0;
  if (drawlist->flags & CTX_DRAWLIST_DOESNT_OWN_ENTRIES)
    {
      return -1;
    }
  if (length % 9) return -1;
  ctx_drawlist_resize (drawlist, length/9);
  memcpy (drawlist->entries, data, length);
  drawlist->count = length / 9;
  return length;
}

int ctx_get_drawlist_count (Ctx *ctx)
{
  return ctx->drawlist.count;
}

const CtxEntry *ctx_get_drawlist (Ctx *ctx)
{
  return ctx->drawlist.entries;
}

int
ctx_add_data (Ctx *ctx, void *data, int length)
{
  if (length % sizeof (CtxEntry) )
    {
      //ctx_log("err\n");
      return -1;
    }
  /* some more input verification might be in order.. like
   * verify that it is well-formed up to length?
   *
   * also - it would be very useful to stop processing
   * upon flush - and do drawlist resizing.
   */
  return ctx_drawlist_add_entry (&ctx->drawlist, (CtxEntry *) data);
}

int ctx_drawlist_add_u32 (CtxDrawlist *drawlist, CtxCode code, uint32_t u32[2])
{
  CtxEntry entry = {code, {{0},}};
  entry.data.u32[0] = u32[0];
  entry.data.u32[1] = u32[1];
  return ctx_drawlist_add_single (drawlist, &entry);
}

int ctx_drawlist_add_data (CtxDrawlist *drawlist, const void *data, int length)
{
  CtxEntry entry = {CTX_DATA, {{0},}};
  entry.data.u32[0] = 0;
  entry.data.u32[1] = 0;
  int ret = ctx_drawlist_add_single (drawlist, &entry);
  if (!data) { return -1; }
  int length_in_blocks;
  if (length <= 0) { length = strlen ( (char *) data) + 1; }
  length_in_blocks = length / sizeof (CtxEntry);
  length_in_blocks += (length % sizeof (CtxEntry) ) ?1:0;
  if (drawlist->count + length_in_blocks + 4 > drawlist->size)
    { ctx_drawlist_resize (drawlist, drawlist->count * 1.2 + length_in_blocks + 32); }
  if (drawlist->count >= drawlist->size)
    { return -1; }
  drawlist->count += length_in_blocks;
  drawlist->entries[ret].data.u32[0] = length;
  drawlist->entries[ret].data.u32[1] = length_in_blocks;
  memcpy (&drawlist->entries[ret+1], data, length);
  {
    //int reverse = ctx_drawlist_add (drawlist, CTX_DATA_REV);
    CtxEntry entry = {CTX_DATA_REV, {{0},}};
    entry.data.u32[0] = length;
    entry.data.u32[1] = length_in_blocks;
    ctx_drawlist_add_single (drawlist, &entry);
    /* this reverse marker exist to enable more efficient
       front to back traversal, can be ignored in other
       direction, is this needed after string setters as well?
     */
  }
  return ret;
}

static CtxEntry
ctx_void (CtxCode code)
{
  CtxEntry command;
  command.code = code;
  command.data.u32[0] = 0;
  command.data.u32[1] = 0;
  return command;
}

static CtxEntry
ctx_f (CtxCode code, float x, float y)
{
  CtxEntry command = ctx_void (code);
  command.data.f[0] = x;
  command.data.f[1] = y;
  return command;
}

static CtxEntry
ctx_u32 (CtxCode code, uint32_t x, uint32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.u32[0] = x;
  command.data.u32[1] = y;
  return command;
}

#if 0
static CtxEntry
ctx_s32 (CtxCode code, int32_t x, int32_t y)
{
  CtxEntry command = ctx_void (code);
  command.data.s32[0] = x;
  command.data.s32[1] = y;
  return command;
}
#endif

static inline CtxEntry
ctx_s16 (CtxCode code, int x0, int y0, int x1, int y1)
{
  CtxEntry command = ctx_void (code);
  command.data.s16[0] = x0;
  command.data.s16[1] = y0;
  command.data.s16[2] = x1;
  command.data.s16[3] = y1;
  return command;
}

static CtxEntry
ctx_u8 (CtxCode code,
        uint8_t a, uint8_t b, uint8_t c, uint8_t d,
        uint8_t e, uint8_t f, uint8_t g, uint8_t h)
{
  CtxEntry command = ctx_void (code);
  command.data.u8[0] = a;
  command.data.u8[1] = b;
  command.data.u8[2] = c;
  command.data.u8[3] = d;
  command.data.u8[4] = e;
  command.data.u8[5] = f;
  command.data.u8[6] = g;
  command.data.u8[7] = h;
  return command;
}

#define CTX_PROCESS_VOID(cmd) do {\
  CtxEntry command = ctx_void (cmd); \
  ctx_process (ctx, &command);}while(0) \

#define CTX_PROCESS_F(cmd, x, y) do {\
  CtxEntry command = ctx_f(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_F1(cmd, x) do {\
  CtxEntry command = ctx_f(cmd, x, 0);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U32(cmd, x, y) do {\
  CtxEntry command = ctx_u32(cmd, x, y);\
  ctx_process (ctx, &command);}while(0)

#define CTX_PROCESS_U8(cmd, x) do {\
  CtxEntry command = ctx_u8(cmd, x,0,0,0,0,0,0,0);\
  ctx_process (ctx, &command);}while(0)


static void
ctx_process_cmd_str_with_len (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1, int len)
{
  CtxEntry commands[1 + 2 + (len+1+1)/9];
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (code, arg0, arg1);
  commands[1].code = CTX_DATA;
  commands[1].data.u32[0] = len;
  commands[1].data.u32[1] = (len+1+1)/9 + 1;
  memcpy( (char *) &commands[2].data.u8[0], string, len);
  ( (char *) (&commands[2].data.u8[0]) ) [len]=0;
  ctx_process (ctx, commands);
}

static void
ctx_process_cmd_str (Ctx *ctx, CtxCode code, const char *string, uint32_t arg0, uint32_t arg1)
{
  ctx_process_cmd_str_with_len (ctx, code, string, arg0, arg1, strlen (string));
}

static void
ctx_process_cmd_str_float (Ctx *ctx, CtxCode code, const char *string, float arg0, float arg1)
{
  uint32_t iarg0;
  uint32_t iarg1;
  memcpy (&iarg0, &arg0, sizeof (iarg0));
  memcpy (&iarg1, &arg1, sizeof (iarg1));
  ctx_process_cmd_str_with_len (ctx, code, string, iarg0, iarg1, strlen (string));
}

#if CTX_BITPACK_PACKER
static int
ctx_last_history (CtxDrawlist *drawlist)
{
  int last_history = 0;
  int i = 0;
  while (i < drawlist->count)
    {
      CtxEntry *entry = &drawlist->entries[i];
      i += (ctx_conts_for_entry (entry) + 1);
    }
  return last_history;
}
#endif

#if CTX_BITPACK_PACKER

static float
find_max_dev (CtxEntry *entry, int nentrys)
{
  float max_dev = 0.0;
  for (int c = 0; c < nentrys; c++)
    {
      for (int d = 0; d < 2; d++)
        {
          if (entry[c].data.f[d] > max_dev)
            { max_dev = entry[c].data.f[d]; }
          if (entry[c].data.f[d] < -max_dev)
            { max_dev = -entry[c].data.f[d]; }
        }
    }
  return max_dev;
}

static void
pack_s8_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s8[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}

static void
pack_s16_args (CtxEntry *entry, int npairs)
{
  for (int c = 0; c < npairs; c++)
    for (int d = 0; d < 2; d++)
      { entry[0].data.s16[c*2+d]=entry[c].data.f[d] * CTX_SUBDIV; }
}
#endif

#if CTX_BITPACK_PACKER
static void
ctx_drawlist_remove_tiny_curves (CtxDrawlist *drawlist, int start_pos)
{
  CtxIterator iterator;
  if ( (drawlist->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_iterator_init (&iterator, drawlist, start_pos, CTX_ITERATOR_FLAT);
  iterator.end_pos = drawlist->count - 5;
  CtxCommand *command = NULL;
  while ( (command = ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      /* things smaller than this have probably been scaled down
         beyond recognition, bailing for both better packing and less rasterization work
       */
      if (command[0].code == CTX_REL_CURVE_TO)
        {
          float max_dev = find_max_dev (entry, 3);
          if (max_dev < 1.0)
            {
              entry[0].code = CTX_REL_LINE_TO;
              entry[0].data.f[0] = entry[2].data.f[0];
              entry[0].data.f[1] = entry[2].data.f[1];
              entry[1].code = CTX_NOP;
              entry[2].code = CTX_NOP;
            }
        }
    }
}
#endif

#if CTX_BITPACK_PACKER
static void
ctx_drawlist_bitpack (CtxDrawlist *drawlist, int start_pos)
{
#if CTX_BITPACK
  int i = 0;
  if ( (drawlist->flags & CTX_TRANSFORMATION_BITPACK) == 0)
    { return; }
  ctx_drawlist_remove_tiny_curves (drawlist, drawlist->bitpack_pos);
  i = drawlist->bitpack_pos;
  if (start_pos > i)
    { i = start_pos; }
  while (i < drawlist->count - 4) /* the -4 is to avoid looking past
                                    initialized data we're not ready
                                    to bitpack yet*/
    {
      CtxEntry *entry = &drawlist->entries[i];
      if (entry[0].code == CTX_SET_RGBA_U8 &&
          entry[1].code == CTX_MOVE_TO &&
          entry[2].code == CTX_REL_LINE_TO &&
          entry[3].code == CTX_REL_LINE_TO &&
          entry[4].code == CTX_REL_LINE_TO &&
          entry[5].code == CTX_REL_LINE_TO &&
          entry[6].code == CTX_FILL &&
          ctx_fabsf (entry[2].data.f[0] - 1.0f) < 0.02f &&
          ctx_fabsf (entry[3].data.f[1] - 1.0f) < 0.02f)
        {
          entry[0].code = CTX_SET_PIXEL;
          entry[0].data.u16[2] = entry[1].data.f[0];
          entry[0].data.u16[3] = entry[1].data.f[1];
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
          entry[3].code = CTX_NOP;
          entry[4].code = CTX_NOP;
          entry[5].code = CTX_NOP;
          entry[6].code = CTX_NOP;
        }
#if 1
      else if (entry[0].code == CTX_REL_LINE_TO)
        {
          if (entry[1].code == CTX_REL_LINE_TO &&
              entry[2].code == CTX_REL_LINE_TO &&
              entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_X4;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_CURVE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_REL_CURVE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_LINE_TO &&
                   entry[2].code == CTX_REL_LINE_TO &&
                   entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_LINE_TO_X4;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_MOVE_TO)
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 31000 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_LINE_TO_REL_MOVE_TO;
                  entry[1].code = CTX_NOP;
                }
            }
          else if (entry[1].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 31000 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_LINE_TO_X2;
                  entry[1].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_REL_CURVE_TO)
        {
          if (entry[3].code == CTX_REL_LINE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else if (entry[3].code == CTX_REL_MOVE_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_CURVE_TO_REL_MOVE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else
            {
              float max_dev = find_max_dev (entry, 3);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 3);
                  ctx_arg_s8 (6) =
                    ctx_arg_s8 (7) = 0;
                  entry[0].code = CTX_REL_CURVE_TO_REL_LINE_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_REL_QUAD_TO)
        {
          if (entry[2].code == CTX_REL_QUAD_TO)
            {
              float max_dev = find_max_dev (entry, 4);
              if (max_dev < 114 / CTX_SUBDIV)
                {
                  pack_s8_args (entry, 4);
                  entry[0].code = CTX_REL_QUAD_TO_REL_QUAD_TO;
                  entry[1].code = CTX_NOP;
                  entry[2].code = CTX_NOP;
                  entry[3].code = CTX_NOP;
                }
            }
          else
            {
              float max_dev = find_max_dev (entry, 2);
              if (max_dev < 3100 / CTX_SUBDIV)
                {
                  pack_s16_args (entry, 2);
                  entry[0].code = CTX_REL_QUAD_TO_S16;
                  entry[1].code = CTX_NOP;
                }
            }
        }
#endif
#if 1
      else if (entry[0].code == CTX_FILL &&
               entry[1].code == CTX_MOVE_TO)
        {
          entry[0] = entry[1];
          entry[0].code = CTX_FILL_MOVE_TO;
          entry[1].code = CTX_NOP;
        }
#endif
#if 1
      else if (entry[0].code == CTX_MOVE_TO &&
               entry[1].code == CTX_MOVE_TO &&
               entry[2].code == CTX_MOVE_TO)
        {
          entry[0]      = entry[2];
          entry[0].code = CTX_MOVE_TO;
          entry[1].code = CTX_NOP;
          entry[2].code = CTX_NOP;
        }
#endif
#if 1
      else if ( (entry[0].code == CTX_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) ||
                (entry[0].code == CTX_REL_MOVE_TO &&
                 entry[1].code == CTX_MOVE_TO) )
        {
          entry[0]      = entry[1];
          entry[0].code = CTX_MOVE_TO;
          entry[1].code = CTX_NOP;
        }
#endif
      i += (ctx_conts_for_entry (entry) + 1);
    }
  int source = drawlist->bitpack_pos;
  int target = drawlist->bitpack_pos;
  int removed = 0;
  /* remove nops that have been inserted as part of shortenings
   */
  while (source < drawlist->count)
    {
      CtxEntry *sentry = &drawlist->entries[source];
      CtxEntry *tentry = &drawlist->entries[target];
      while (sentry->code == CTX_NOP && source < drawlist->count)
        {
          source++;
          sentry = &drawlist->entries[source];
          removed++;
        }
      if (sentry != tentry)
        { *tentry = *sentry; }
      source ++;
      target ++;
    }
  drawlist->count -= removed;
  drawlist->bitpack_pos = drawlist->count;
#endif
}

#endif

static void
ctx_drawlist_compact (CtxDrawlist *drawlist)
{
#if CTX_BITPACK_PACKER
  int last_history;
  last_history = ctx_last_history (drawlist);
#else
  if (drawlist) {};
#endif
#if CTX_BITPACK_PACKER
  ctx_drawlist_bitpack (drawlist, last_history);
#endif
}

uint8_t *ctx_define_texture_pixel_data (CtxEntry *entry)
{
  return &entry[2 + 1 + 1 + ctx_conts_for_entry (&entry[2])].data.u8[0];
}
#ifndef __CTX_TRANSFORM
#define __CTX_TRANSFORM

static void
_ctx_user_to_device (CtxState *state, float *x, float *y)
{
  ctx_matrix_apply_transform (&state->gstate.transform, x, y);
}

static void
_ctx_user_to_device_distance (CtxState *state, float *x, float *y)
{
  const CtxMatrix *m = &state->gstate.transform;
  ctx_matrix_apply_transform (m, x, y);
  *x -= m->m[2][0];
  *y -= m->m[2][1];
}

void ctx_user_to_device          (Ctx *ctx, float *x, float *y)
{
  _ctx_user_to_device (&ctx->state, x, y);
}
void ctx_user_to_device_distance (Ctx *ctx, float *x, float *y)
{
  _ctx_user_to_device_distance (&ctx->state, x, y);
}

static void
ctx_matrix_set (CtxMatrix *matrix, float a, float b, float c, float d, float e, float f)
{
  matrix->m[0][0] = a;
  matrix->m[0][1] = b;
  matrix->m[1][0] = c;
  matrix->m[1][1] = d;
  matrix->m[2][0] = e;
  matrix->m[2][1] = f;
}

void
ctx_matrix_identity (CtxMatrix *matrix)
{
  matrix->m[0][0] = 1.0f;
  matrix->m[0][1] = 0.0f;
  matrix->m[1][0] = 0.0f;
  matrix->m[1][1] = 1.0f;
  matrix->m[2][0] = 0.0f;
  matrix->m[2][1] = 0.0f;
}

void
ctx_matrix_multiply (CtxMatrix       *result,
                     const CtxMatrix *t,
                     const CtxMatrix *s)
{
  CtxMatrix r;
  r.m[0][0] = t->m[0][0] * s->m[0][0] + t->m[0][1] * s->m[1][0];
  r.m[0][1] = t->m[0][0] * s->m[0][1] + t->m[0][1] * s->m[1][1];
  r.m[1][0] = t->m[1][0] * s->m[0][0] + t->m[1][1] * s->m[1][0];
  r.m[1][1] = t->m[1][0] * s->m[0][1] + t->m[1][1] * s->m[1][1];
  r.m[2][0] = t->m[2][0] * s->m[0][0] + t->m[2][1] * s->m[1][0] + s->m[2][0];
  r.m[2][1] = t->m[2][0] * s->m[0][1] + t->m[2][1] * s->m[1][1] + s->m[2][1];
  *result = r;
}


void
ctx_matrix_translate (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = 1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] = x;
  transform.m[2][1] = y;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

void
ctx_matrix_scale (CtxMatrix *matrix, float x, float y)
{
  CtxMatrix transform;
  transform.m[0][0] = x;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = 0.0f;
  transform.m[1][1] = y;
  transform.m[2][0] = 0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

void
ctx_matrix_rotate (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_sin = ctx_sinf (angle);
  float val_cos = ctx_cosf (angle);
  transform.m[0][0] =  val_cos;
  transform.m[0][1] = val_sin;
  transform.m[1][0] = -val_sin;
  transform.m[1][1] = val_cos;
  transform.m[2][0] =     0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

#if 0
static void
ctx_matrix_skew_x (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = 0.0f;
  transform.m[1][0] = val_tan;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}

static void
ctx_matrix_skew_y (CtxMatrix *matrix, float angle)
{
  CtxMatrix transform;
  float val_tan = ctx_tanf (angle);
  transform.m[0][0] =    1.0f;
  transform.m[0][1] = val_tan;
  transform.m[1][0] =    0.0f;
  transform.m[1][1] = 1.0f;
  transform.m[2][0] =    0.0f;
  transform.m[2][1] = 0.0f;
  ctx_matrix_multiply (matrix, &transform, matrix);
}
#endif


void
ctx_identity (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_IDENTITY);
}

void
ctx_apply_transform (Ctx *ctx, float a, float b,  // hscale, hskew
                     float c, float d,  // vskew,  vscale
                     float e, float f)  // htran,  vtran
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_APPLY_TRANSFORM, a, b),
    ctx_f (CTX_CONT,            c, d),
    ctx_f (CTX_CONT,            e, f)
  };
  ctx_process (ctx, command);
}

void
ctx_get_transform  (Ctx *ctx, float *a, float *b,
                    float *c, float *d,
                    float *e, float *f)
{
  if (a) { *a = ctx->state.gstate.transform.m[0][0]; }
  if (b) { *b = ctx->state.gstate.transform.m[0][1]; }
  if (c) { *c = ctx->state.gstate.transform.m[1][0]; }
  if (d) { *d = ctx->state.gstate.transform.m[1][1]; }
  if (e) { *e = ctx->state.gstate.transform.m[2][0]; }
  if (f) { *f = ctx->state.gstate.transform.m[2][1]; }
}

void ctx_apply_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_apply_transform (ctx,
                       matrix->m[0][0], matrix->m[0][1],
                       matrix->m[1][0], matrix->m[1][1],
                       matrix->m[2][0], matrix->m[2][1]);
}

void ctx_get_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  *matrix = ctx->state.gstate.transform;
}

void ctx_set_matrix (Ctx *ctx, CtxMatrix *matrix)
{
  ctx_identity (ctx);
  ctx_apply_matrix (ctx, matrix);
}

void ctx_rotate (Ctx *ctx, float x)
{
  if (x == 0.0f)
    return;
  CTX_PROCESS_F1 (CTX_ROTATE, x);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void ctx_scale (Ctx *ctx, float x, float y)
{
  if (x == 1.0f && y == 1.0f)
    return;
  CTX_PROCESS_F (CTX_SCALE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void ctx_translate (Ctx *ctx, float x, float y)
{
  if (x == 0.0f && y == 0.0f)
    return;
  CTX_PROCESS_F (CTX_TRANSLATE, x, y);
  if (ctx->transformation & CTX_TRANSFORMATION_SCREEN_SPACE)
    { ctx->drawlist.count--; }
}

void
ctx_matrix_invert (CtxMatrix *m)
{
  CtxMatrix t = *m;
  float invdet, det = m->m[0][0] * m->m[1][1] -
                      m->m[1][0] * m->m[0][1];
  if (det > -0.0000001f && det < 0.0000001f)
    {
      m->m[0][0] = m->m[0][1] =
                     m->m[1][0] = m->m[1][1] =
                                    m->m[2][0] = m->m[2][1] = 0.0;
      return;
    }
  invdet = 1.0f / det;
  m->m[0][0] = t.m[1][1] * invdet;
  m->m[1][0] = -t.m[1][0] * invdet;
  m->m[2][0] = (t.m[1][0] * t.m[2][1] - t.m[1][1] * t.m[2][0]) * invdet;
  m->m[0][1] = -t.m[0][1] * invdet;
  m->m[1][1] = t.m[0][0] * invdet;
  m->m[2][1] = (t.m[0][1] * t.m[2][0] - t.m[0][0] * t.m[2][1]) * invdet ;
}

void
ctx_matrix_apply_transform (const CtxMatrix *m, float *x, float *y)
{
  float x_in = *x;
  float y_in = *y;
  *x = ( (x_in * m->m[0][0]) + (y_in * m->m[1][0]) + m->m[2][0]);
  *y = ( (y_in * m->m[1][1]) + (x_in * m->m[0][1]) + m->m[2][1]);
}


#endif
#ifndef __CTX_COLOR
#define __CTX_COLOR

int ctx_color_model_get_components (CtxColorModel model)
{
  switch (model)
    {
      case CTX_GRAY:
        return 1;
      case CTX_GRAYA:
      case CTX_GRAYA_A:
        return 1;
      case CTX_RGB:
      case CTX_LAB:
      case CTX_LCH:
      case CTX_DRGB:
        return 3;
      case CTX_CMYK:
      case CTX_DCMYK:
      case CTX_LABA:
      case CTX_LCHA:
      case CTX_RGBA:
      case CTX_DRGBA:
      case CTX_RGBA_A:
      case CTX_RGBA_A_DEVICE:
        return 4;
      case CTX_DCMYKA:
      case CTX_CMYKA:
      case CTX_CMYKA_A:
      case CTX_DCMYKA_A:
        return 5;
    }
  return 0;
}

#if 0
inline static float ctx_u8_to_float (uint8_t val_u8)
{
  float val_f = val_u8 / 255.0;
  return val_f;
}
#else
float ctx_u8_float[256];
#endif

CtxColor *ctx_color_new ()
{
  CtxColor *color = (CtxColor*)ctx_calloc (sizeof (CtxColor), 1);
  return color;
}

int ctx_color_is_transparent (CtxColor *color)
{
  return color->alpha <= 0.001f;
}


void ctx_color_free (CtxColor *color)
{
  free (color);
}

static void ctx_color_set_RGBA8 (CtxState *state, CtxColor *color, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  color->original = color->valid = CTX_VALID_RGBA_U8;
  color->rgba[0] = r;
  color->rgba[1] = g;
  color->rgba[2] = b;
  color->rgba[3] = a;
#if CTX_ENABLE_CM
  color->space = state->gstate.device_space;
#endif
}

#if 0
static void ctx_color_set_RGBA8_ (CtxColor *color, const uint8_t *in)
{
  ctx_color_set_RGBA8 (color, in[0], in[1], in[2], in[3]);
}
#endif

static void ctx_color_set_graya (CtxState *state, CtxColor *color, float gray, float alpha)
{
  color->original = color->valid = CTX_VALID_GRAYA;
  color->l = gray;
  color->alpha = alpha;
}
#if 0
static void ctx_color_set_graya_ (CtxColor *color, const float *in)
{
  return ctx_color_set_graya (color, in[0], in[1]);
}
#endif

void ctx_color_set_rgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
{
#if CTX_ENABLE_CM
  color->original = color->valid = CTX_VALID_RGBA;
  color->red      = r;
  color->green    = g;
  color->blue     = b;
  color->space    = state->gstate.rgb_space;
#else
  color->original     = color->valid = CTX_VALID_RGBA_DEVICE;
  color->device_red   = r;
  color->device_green = g;
  color->device_blue  = b;
#endif
  color->alpha        = a;
}

static void ctx_color_set_drgba (CtxState *state, CtxColor *color, float r, float g, float b, float a)
{
#if CTX_ENABLE_CM
  color->original     = color->valid = CTX_VALID_RGBA_DEVICE;
  color->device_red   = r;
  color->device_green = g;
  color->device_blue  = b;
  color->alpha        = a;
  color->space        = state->gstate.device_space;
#else
  ctx_color_set_rgba (state, color, r, g, b, a);
#endif
}

#if 0
static void ctx_color_set_rgba_ (CtxState *state, CtxColor *color, const float *in)
{
  ctx_color_set_rgba (color, in[0], in[1], in[2], in[3]);
}
#endif

/* the baseline conversions we have whether CMYK support is enabled or not,
 * providing an effort at right rendering
 */
static void ctx_cmyk_to_rgb (float c, float m, float y, float k, float *r, float *g, float *b)
{
  *r = (1.0f-c) * (1.0f-k);
  *g = (1.0f-m) * (1.0f-k);
  *b = (1.0f-y) * (1.0f-k);
}

void ctx_rgb_to_cmyk (float r, float g, float b,
                      float *c_out, float *m_out, float *y_out, float *k_out)
{
  float c = 1.0f - r;
  float m = 1.0f - g;
  float y = 1.0f - b;
  float k = ctx_minf (c, ctx_minf (y, m) );
  if (k < 1.0f)
    {
      c = (c - k) / (1.0f - k);
      m = (m - k) / (1.0f - k);
      y = (y - k) / (1.0f - k);
    }
  else
    {
      c = m = y = 0.0f;
    }
  *c_out = c;
  *m_out = m;
  *y_out = y;
  *k_out = k;
}

#if CTX_ENABLE_CMYK
static void ctx_color_set_cmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original = color->valid = CTX_VALID_CMYKA;
  color->cyan     = c;
  color->magenta  = m;
  color->yellow   = y;
  color->key      = k;
  color->alpha    = a;
#if CTX_ENABLE_CM
  color->space    = state->gstate.cmyk_space;
#endif
}

static void ctx_color_set_dcmyka (CtxState *state, CtxColor *color, float c, float m, float y, float k, float a)
{
  color->original       = color->valid = CTX_VALID_DCMYKA;
  color->device_cyan    = c;
  color->device_magenta = m;
  color->device_yellow  = y;
  color->device_key     = k;
  color->alpha          = a;
#if CTX_ENABLE_CM
  color->space = state->gstate.device_space;
#endif
}

#endif

#if CTX_ENABLE_CM

static void ctx_rgb_user_to_device (CtxState *state, float rin, float gin, float bin,
                                    float *rout, float *gout, float *bout)
{
#if CTX_BABL
#if 0
  fprintf (stderr, "-[%p %p\n",
    state->gstate.fish_rgbaf_user_to_device,
    state->gstate.fish_rgbaf_device_to_user);
#endif
  if (state->gstate.fish_rgbaf_user_to_device)
  {
    float rgbaf[4]={rin,gin,bin,1.0};
    float rgbafo[4];
    babl_process (state->gstate.fish_rgbaf_user_to_device,
                  rgbaf, rgbafo, 1);

    *rout = rgbafo[0];
    *gout = rgbafo[1];
    *bout = rgbafo[2];
    return;
  }
#endif
  *rout = rin;
  *gout = gin;
  *bout = bin;
}

static void ctx_rgb_device_to_user (CtxState *state, float rin, float gin, float bin,
                                    float *rout, float *gout, float *bout)
{
#if CTX_BABL
#if 0
  fprintf (stderr, "=[%p %p\n",
    state->gstate.fish_rgbaf_user_to_device,
    state->gstate.fish_rgbaf_device_to_user);
#endif
  if (state->gstate.fish_rgbaf_device_to_user)
  {
    float rgbaf[4]={rin,gin,bin,1.0};
    float rgbafo[4];
    babl_process (state->gstate.fish_rgbaf_device_to_user,
                  rgbaf, rgbafo, 1);

    *rout = rgbafo[0];
    *gout = rgbafo[1];
    *bout = rgbafo[2];
    return;
  }
#endif
  *rout = rin;
  *gout = gin;
  *bout = bin;
}
#endif

static void ctx_color_get_drgba (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_RGBA_DEVICE) )
    {
#if CTX_ENABLE_CM
      if (color->valid & CTX_VALID_RGBA)
        {
          ctx_rgb_user_to_device (state, color->red, color->green, color->blue,
                                  & (color->device_red), & (color->device_green), & (color->device_blue) );
        }
      else
#endif
        if (color->valid & CTX_VALID_RGBA_U8)
          {
            float red = ctx_u8_to_float (color->rgba[0]);
            float green = ctx_u8_to_float (color->rgba[1]);
            float blue = ctx_u8_to_float (color->rgba[2]);
#if CTX_ENABLE_CM
            ctx_rgb_user_to_device (state, red, green, blue,
                                  & (color->device_red), & (color->device_green), & (color->device_blue) );
#else
            color->device_red = red;
            color->device_green = green;
            color->device_blue = blue;
#endif
            color->alpha        = ctx_u8_to_float (color->rgba[3]);
          }
#if CTX_ENABLE_CMYK
        else if (color->valid & CTX_VALID_CMYKA)
          {
            ctx_cmyk_to_rgb (color->cyan, color->magenta, color->yellow, color->key,
                             &color->device_red,
                             &color->device_green,
                             &color->device_blue);
          }
#endif
        else if (color->valid & CTX_VALID_GRAYA)
          {
            color->device_red   =
              color->device_green =
                color->device_blue  = color->l;
          }
      color->valid |= CTX_VALID_RGBA_DEVICE;
    }
  out[0] = color->device_red;
  out[1] = color->device_green;
  out[2] = color->device_blue;
  out[3] = color->alpha;
}

void ctx_color_get_rgba (CtxState *state, CtxColor *color, float *out)
{
#if CTX_ENABLE_CM
  if (! (color->valid & CTX_VALID_RGBA) )
    {
      ctx_color_get_drgba (state, color, out);
      if (color->valid & CTX_VALID_RGBA_DEVICE)
        {
          ctx_rgb_device_to_user (state, color->device_red, color->device_green, color->device_blue,
                                  & (color->red), & (color->green), & (color->blue) );
        }
      color->valid |= CTX_VALID_RGBA;
    }
  out[0] = color->red;
  out[1] = color->green;
  out[2] = color->blue;
  out[3] = color->alpha;
#else
  ctx_color_get_drgba (state, color, out);
#endif
}


float ctx_float_color_rgb_to_gray (CtxState *state, const float *rgb)
{
        // XXX todo replace with correct according to primaries
  return CTX_CSS_RGB_TO_LUMINANCE(rgb);
}
uint8_t ctx_u8_color_rgb_to_gray (CtxState *state, const uint8_t *rgb)
{
        // XXX todo replace with correct according to primaries
  return CTX_CSS_RGB_TO_LUMINANCE(rgb);
}

void ctx_color_get_graya (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_GRAYA) )
    {
      float rgba[4];
      ctx_color_get_drgba (state, color, rgba);
      color->l = ctx_float_color_rgb_to_gray (state, rgba);
      color->valid |= CTX_VALID_GRAYA;
    }
  out[0] = color->l;
  out[1] = color->alpha;
}

#if CTX_ENABLE_CMYK
void ctx_color_get_cmyka (CtxState *state, CtxColor *color, float *out)
{
  if (! (color->valid & CTX_VALID_CMYKA) )
    {
      if (color->valid & CTX_VALID_GRAYA)
        {
          color->cyan = color->magenta = color->yellow = 0.0;
          color->key = color->l;
        }
      else
        {
          float rgba[4];
          ctx_color_get_rgba (state, color, rgba);
          ctx_rgb_to_cmyk (rgba[0], rgba[1], rgba[2],
                           &color->cyan, &color->magenta, &color->yellow, &color->key);
          color->alpha = rgba[3];
        }
      color->valid |= CTX_VALID_CMYKA;
    }
  out[0] = color->cyan;
  out[1] = color->magenta;
  out[2] = color->yellow;
  out[3] = color->key;
  out[4] = color->alpha;
}

#if 0
static void ctx_color_get_cmyka_u8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_CMYKA_U8) )
    {
      float cmyka[5];
      ctx_color_get_cmyka (color, cmyka);
      for (int i = 0; i < 5; i ++)
        { color->cmyka[i] = ctx_float_to_u8 (cmyka[i]); }
      color->valid |= CTX_VALID_CMYKA_U8;
    }
  out[0] = color->cmyka[0];
  out[1] = color->cmyka[1];
  out[2] = color->cmyka[2];
  out[3] = color->cmyka[3];
}
#endif
#endif

void
ctx_color_get_rgba8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_RGBA_U8) )
    {
      float rgba[4];
      ctx_color_get_drgba (state, color, rgba);
      for (int i = 0; i < 4; i ++)
        { color->rgba[i] = ctx_float_to_u8 (rgba[i]); }
      color->valid |= CTX_VALID_RGBA_U8;
    }
  out[0] = color->rgba[0];
  out[1] = color->rgba[1];
  out[2] = color->rgba[2];
  out[3] = color->rgba[3];
}

void ctx_color_get_graya_u8 (CtxState *state, CtxColor *color, uint8_t *out)
{
  if (! (color->valid & CTX_VALID_GRAYA_U8) )
    {
      float graya[2];
      ctx_color_get_graya (state, color, graya);
      color->l_u8 = ctx_float_to_u8 (graya[0]);
      color->rgba[3] = ctx_float_to_u8 (graya[1]);
      color->valid |= CTX_VALID_GRAYA_U8;
    }
  out[0] = color->l_u8;
  out[1] = color->rgba[3];
}

#if 0
void
ctx_get_rgba (Ctx *ctx, float *rgba)
{
  ctx_color_get_rgba (& (ctx->state), &ctx->state.gstate.source.color, rgba);
}

void
ctx_get_drgba (Ctx *ctx, float *rgba)
{
  ctx_color_get_drgba (& (ctx->state), &ctx->state.gstate.source.color, rgba);
}
#endif

int ctx_in_fill (Ctx *ctx, float x, float y)
{
  float x1, y1, x2, y2;
  ctx_path_extents (ctx, &x1, &y1, &x2, &y2);

  if (x1 <= x && x <= x2 && // XXX - just bounding box for now
      y1 <= y && y <= y2)   //
    return 1;
  return 0;
}


#if CTX_ENABLE_CMYK
#if 0
void
ctx_get_cmyka (Ctx *ctx, float *cmyka)
{
  ctx_color_get_cmyka (& (ctx->state), &ctx->state.gstate.source.color, cmyka);
}
#endif
#endif
#if 0
void
ctx_get_graya (Ctx *ctx, float *ya)
{
  ctx_color_get_graya (& (ctx->state), &ctx->state.gstate.source.color, ya);
}
#endif

void ctx_stroke_source (Ctx *ctx)
{
  CtxEntry set_stroke = ctx_void (CTX_STROKE_SOURCE);
  ctx_process (ctx, &set_stroke);
}

void ctx_color_raw (Ctx *ctx, CtxColorModel model, float *components, int stroke)
{
#if 0
  CtxSource *source = stroke?
          &ctx->state.gstate.source_stroke:
          &ctx->state.gstate.source_fill;

  if (model == CTX_RGB || model == CTX_RGBA)
  {
    float rgba[4];
  // XXX it should be possible to disable this, to get a more accurate record
  // when it is intentional
    float a = 1.0f;
    if (model == CTX_RGBA) a = components[3];
    ctx_color_get_rgba (&ctx->state, &source->color, rgba);
    if (rgba[0] == components[0] && rgba[1] == components[1] && rgba[2] == components[2] && rgba[3] == a)
     return;
  }
#endif

  if (stroke)
  {
    ctx_stroke_source (ctx);
  }

  CtxEntry command[3]= {
  ctx_f (CTX_COLOR, model, 0)
  };
  switch (model)
  {
    case CTX_RGBA:
    case CTX_RGBA_A:
    case CTX_RGBA_A_DEVICE:
    case CTX_DRGBA:
    case CTX_LABA:
    case CTX_LCHA:
      command[2].data.f[0]=components[3];
      /*FALLTHROUGH*/
    case CTX_RGB:
    case CTX_LAB:
    case CTX_LCH:
    case CTX_DRGB:
      command[0].data.f[1]=components[0];
      command[1].data.f[0]=components[1];
      command[1].data.f[1]=components[2];
      break;
    case CTX_DCMYKA:
    case CTX_CMYKA:
    case CTX_DCMYKA_A:
    case CTX_CMYKA_A:
      command[2].data.f[1]=components[4];
      /*FALLTHROUGH*/
    case CTX_CMYK:
    case CTX_DCMYK:
      command[0].data.f[1]=components[0];
      command[1].data.f[0]=components[1];
      command[1].data.f[1]=components[2];
      command[2].data.f[0]=components[3];
      break;
    case CTX_GRAYA:
    case CTX_GRAYA_A:
      command[1].data.f[0]=components[1];
      /*FALLTHROUGH*/
    case CTX_GRAY:
      command[0].data.f[1]=components[0];
      break;
  }
  ctx_process (ctx, command);
}

void ctx_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  float components[4]={r,g,b,a};
  ctx_color_raw (ctx, CTX_RGBA, components, 0);
}

void ctx_rgba_stroke (Ctx *ctx, float r, float g, float b, float a)
{
  float components[4]={r,g,b,a};
  ctx_color_raw (ctx, CTX_RGBA, components, 1);
}

void ctx_rgb (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_rgba (ctx, r, g, b, 1.0f);
}

void ctx_rgb_stroke (Ctx *ctx, float   r, float   g, float   b)
{
  ctx_rgba_stroke (ctx, r, g, b, 1.0f);
}

void ctx_gray_stroke   (Ctx *ctx, float gray)
{
  ctx_color_raw (ctx, CTX_GRAY, &gray, 1);
}
void ctx_gray (Ctx *ctx, float gray)
{
  ctx_color_raw (ctx, CTX_GRAY, &gray, 0);
}

void ctx_drgba_stroke (Ctx *ctx, float r, float g, float b, float a)
{
  float components[4]={r,g,b,a};
  ctx_color_raw (ctx, CTX_DRGBA, components, 1);
}
void ctx_drgba (Ctx *ctx, float r, float g, float b, float a)
{
  float components[4]={r,g,b,a};
  ctx_color_raw (ctx, CTX_DRGBA, components, 0);
}

#if CTX_ENABLE_CMYK

void ctx_cmyka_stroke (Ctx *ctx, float c, float m, float y, float k, float a)
{
  float components[5]={c,m,y,k,a};
  ctx_color_raw (ctx, CTX_CMYKA, components, 1);
}
void ctx_cmyka (Ctx *ctx, float c, float m, float y, float k, float a)
{
  float components[5]={c,m,y,k,a};
  ctx_color_raw (ctx, CTX_CMYKA, components, 0);
}
void ctx_cmyk_stroke   (Ctx *ctx, float c, float m, float y, float k)
{
  float components[4]={c,m,y,k};
  ctx_color_raw (ctx, CTX_CMYK, components, 1);
}
void ctx_cmyk (Ctx *ctx, float c, float m, float y, float k)
{
  float components[4]={c,m,y,k};
  ctx_color_raw (ctx, CTX_CMYK, components, 0);
}

static void ctx_dcmyk_raw (Ctx *ctx, float c, float m, float y, float k, int stroke)
{
  float components[5]={c,m,y,k,1.0f};
  ctx_color_raw (ctx, CTX_DCMYKA, components, stroke);
}

static void ctx_dcmyka_raw (Ctx *ctx, float c, float m, float y, float k, float a, int stroke)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_COLOR, CTX_DCMYKA + 512 * stroke, c),
    ctx_f (CTX_CONT, m, y),
    ctx_f (CTX_CONT, k, a)
  };
  ctx_process (ctx, command);
}

void ctx_dcmyk_stroke   (Ctx *ctx, float c, float m, float y, float k)
{
  float components[5]={c,m,y,k,1.0f};
  ctx_color_raw (ctx, CTX_DCMYK, components, 1);
}
void ctx_dcmyk (Ctx *ctx, float c, float m, float y, float k)
{
  float components[5]={c,m,y,k,1.0f};
  ctx_color_raw (ctx, CTX_DCMYK, components, 0);
}

void ctx_dcmyka_stroke   (Ctx *ctx, float c, float m, float y, float k, float a)
{
  float components[5]={c,m,y,k,a};
  ctx_color_raw (ctx, CTX_DCMYKA, components, 1);
}
void ctx_dcmyka (Ctx *ctx, float c, float m, float y, float k, float a)
{
  float components[5]={c,m,y,k,a};
  ctx_color_raw (ctx, CTX_DCMYKA, components, 0);
}

#endif

/* XXX: missing CSS1:
 *
 *   EM { color: rgb(110%, 0%, 0%) }  // clipped to 100% 
 *
 *
 *   :first-letter
 *   :first-list
 *   :link :visited :active
 *
 */

typedef struct ColorDef {
  uint64_t name;
  float r;
  float g;
  float b;
  float a;
} ColorDef;

#if 0
#define CTX_silver 	CTX_STRH('s','i','l','v','e','r',0,0,0,0,0,0,0,0)
#define CTX_fuchsia 	CTX_STRH('f','u','c','h','s','i','a',0,0,0,0,0,0,0)
#define CTX_gray 	CTX_STRH('g','r','a','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_yellow 	CTX_STRH('y','e','l','l','o','w',0,0,0,0,0,0,0,0)
#define CTX_white 	CTX_STRH('w','h','i','t','e',0,0,0,0,0,0,0,0,0)
#define CTX_maroon 	CTX_STRH('m','a','r','o','o','n',0,0,0,0,0,0,0,0)
#define CTX_magenta 	CTX_STRH('m','a','g','e','n','t','a',0,0,0,0,0,0,0)
#define CTX_blue 	CTX_STRH('b','l','u','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_green 	CTX_STRH('g','r','e','e','n',0,0,0,0,0,0,0,0,0)
#define CTX_red 	CTX_STRH('r','e','d',0,0,0,0,0,0,0,0,0,0,0)
#define CTX_purple 	CTX_STRH('p','u','r','p','l','e',0,0,0,0,0,0,0,0)
#define CTX_olive 	CTX_STRH('o','l','i','v','e',0,0,0,0,0,0,0,0,0)
#define CTX_teal        CTX_STRH('t','e','a','l',0,0,0,0,0,0,0,0,0,0)
#define CTX_black 	CTX_STRH('b','l','a','c','k',0,0,0,0,0,0,0,0,0)
#define CTX_cyan 	CTX_STRH('c','y','a','n',0,0,0,0,0,0,0,0,0,0)
#define CTX_navy 	CTX_STRH('n','a','v','y',0,0,0,0,0,0,0,0,0,0)
#define CTX_lime 	CTX_STRH('l','i','m','e',0,0,0,0,0,0,0,0,0,0)
#define CTX_aqua 	CTX_STRH('a','q','u','a',0,0,0,0,0,0,0,0,0,0)
#define CTX_transparent CTX_STRH('t','r','a','n','s','p','a','r','e','n','t',0,0,0)
#endif

static ColorDef _ctx_colors[]={
  {CTX_black,    0, 0, 0, 1},
  {CTX_red,      1, 0, 0, 1},
  {CTX_green,    0, 1, 0, 1},
  {CTX_yellow,   1, 1, 0, 1},
  {CTX_blue,     0, 0, 1, 1},
  {CTX_fuchsia,  1, 0, 1, 1},
  {CTX_cyan,     0, 1, 1, 1},
  {CTX_white,    1, 1, 1, 1},
  {CTX_silver,   0.75294, 0.75294, 0.75294, 1},
  {CTX_gray,     0.50196, 0.50196, 0.50196, 1},
  {CTX_magenta,  0.50196, 0, 0.50196, 1},
  {CTX_maroon,   0.50196, 0, 0, 1},
  {CTX_purple,   0.50196, 0, 0.50196, 1},
  {CTX_green,    0, 0.50196, 0, 1},
  {CTX_lime,     0, 1, 0, 1},
  {CTX_olive,    0.50196, 0.50196, 0, 1},
  {CTX_navy,     0, 0,      0.50196, 1},
  {CTX_teal,     0, 0.50196, 0.50196, 1},
  {CTX_aqua,     0, 1, 1, 1},
  {CTX_transparent, 0, 0, 0, 0},
  {CTX_none,     0, 0, 0, 0},
};

static int xdigit_value(const char xdigit)
{
  if (xdigit >= '0' && xdigit <= '9')
   return xdigit - '0';
  switch (xdigit)
  {
    case 'A':case 'a': return 10;
    case 'B':case 'b': return 11;
    case 'C':case 'c': return 12;
    case 'D':case 'd': return 13;
    case 'E':case 'e': return 14;
    case 'F':case 'f': return 15;
  }
  return 0;
}

static int
ctx_color_parse_rgb (CtxState *ctxstate, CtxColor *color, const char *color_string)
{
  float dcolor[4] = {0,0,0,1};
  while (*color_string && *color_string != '(')
    color_string++;
  if (*color_string) color_string++;

  {
    int n_floats = 0;
    char *p =    (char*)color_string;
    char *prev = (char*)NULL;
    for (; p && n_floats < 4 && p != prev && *p; )
    {
      float val;
      prev = p;
      val = _ctx_parse_float (p, &p);
      if (p != prev)
      {
        if (n_floats < 3)
          dcolor[n_floats++] = val/255.0;
        else
          dcolor[n_floats++] = val;

        while (*p == ' ' || *p == ',')
        {
          p++;
          prev++;
        }
      }
    }
  }
  ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
  return 0;
}

static int ctx_isxdigit (uint8_t ch)
{
  if (ch >= '0' && ch <= '9') return 1;
  if (ch >= 'a' && ch <= 'f') return 1;
  if (ch >= 'A' && ch <= 'F') return 1;
  return 0;
}

static int
mrg_color_parse_hex (CtxState *ctxstate, CtxColor *color, const char *color_string)
{
  float dcolor[4]={0,0,0,1};
  int string_length = strlen (color_string);
  int i;
  dcolor[3] = 1.0;

  if (string_length == 7 ||  /* #rrggbb   */
      string_length == 9)    /* #rrggbbaa */
    {
      int num_iterations = (string_length - 1) / 2;
  
      for (i = 0; i < num_iterations; ++i)
        {
          if (ctx_isxdigit (color_string[2 * i + 1]) &&
              ctx_isxdigit (color_string[2 * i + 2]))
            {
              dcolor[i] = (xdigit_value (color_string[2 * i + 1]) << 4 |
                           xdigit_value (color_string[2 * i + 2])) / 255.f;
            }
          else
            {
              return 0;
            }
        }
      /* Successful #rrggbb(aa) parsing! */
      ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
      return 1;
    }
  else if (string_length == 4 ||  /* #rgb  */
           string_length == 5)    /* #rgba */
    {
      int num_iterations = string_length - 1;
      for (i = 0; i < num_iterations; ++i)
        {
          if (ctx_isxdigit (color_string[i + 1]))
            {
              dcolor[i] = (xdigit_value (color_string[i + 1]) << 4 |
                           xdigit_value (color_string[i + 1])) / 255.f;
            }
          else
            {
              return 0;
            }
        }
      ctx_color_set_rgba (ctxstate, color, dcolor[0], dcolor[1],dcolor[2],dcolor[3]);
      /* Successful #rgb(a) parsing! */
      return 0;
    }
  /* String was of unsupported length. */
  return 1;
}

//#define CTX_currentColor 	CTX_STRH('c','u','r','r','e','n','t','C','o','l','o','r',0,0)

int ctx_color_set_from_string (Ctx *ctx, CtxColor *color, const char *string)
{
  int i;
  uint64_t hash = ctx_strhash (string, 0);
//  ctx_color_set_rgba (&(ctx->state), color, 0.4,0.1,0.9,1.0);
//  return 0;
    //rgba[0], rgba[1], rgba[2], rgba[3]);

  if (hash == CTX_currentColor)
  {
    float rgba[4];
    CtxColor ccolor;
    ctx_get_color (ctx, CTX_color, &ccolor);
    ctx_color_get_rgba (&(ctx->state), &ccolor, rgba);
    ctx_color_set_rgba (&(ctx->state), color, rgba[0], rgba[1], rgba[2], rgba[3]);
    return 0;
  }

  for (i = (sizeof(_ctx_colors)/sizeof(_ctx_colors[0]))-1; i>=0; i--)
  {
    if (hash == _ctx_colors[i].name)
    {
      ctx_color_set_rgba (&(ctx->state), color,
       _ctx_colors[i].r, _ctx_colors[i].g, _ctx_colors[i].b, _ctx_colors[i].a);
      return 0;
    }
  }

  if (string[0] == '#')
    mrg_color_parse_hex (&(ctx->state), color, string);
  else if (string[0] == 'r' &&
      string[1] == 'g' &&
      string[2] == 'b'
      )
    ctx_color_parse_rgb (&(ctx->state), color, string);

  return 0;
}

int ctx_color (Ctx *ctx, const char *string)
{
  CtxColor color = {0,};
  ctx_color_set_from_string (ctx, &color, string);
  float rgba[4];
  ctx_color_get_rgba (&(ctx->state), &color, rgba);
  ctx_color_raw (ctx, CTX_RGBA, rgba, 0);
  return 0;
}

void
ctx_rgba8 (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
#if 0
  CtxEntry command = ctx_u8 (CTX_SET_RGBA_U8, r, g, b, a, 0, 0, 0, 0);

  uint8_t rgba[4];
  ctx_color_get_rgba8 (&ctx->state, &ctx->state.gstate.source.color, rgba);
  if (rgba[0] == r && rgba[1] == g && rgba[2] == b && rgba[3] == a)
     return;

  ctx_process (ctx, &command);
#else
  ctx_rgba (ctx, r/255.0f, g/255.0f, b/255.0f, a/255.0f);
#endif
}

void ctx_rgba8_stroke (Ctx *ctx, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  ctx_rgba_stroke (ctx, r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}


#endif 

#if CTX_BABL
void ctx_rasterizer_colorspace_babl (CtxState      *state,
                                     CtxColorSpace  space_slot,
                                     const Babl    *space)
{
  switch (space_slot)
  {
    case CTX_COLOR_SPACE_DEVICE_RGB:
      state->gstate.device_space = space;
      break;
    case CTX_COLOR_SPACE_DEVICE_CMYK:
      state->gstate.device_space = space;
      break;
    case CTX_COLOR_SPACE_USER_RGB:
      state->gstate.rgb_space = space;
      break;
    case CTX_COLOR_SPACE_USER_CMYK:
      state->gstate.cmyk_space = space;
      break;
    case CTX_COLOR_SPACE_TEXTURE:
      state->gstate.texture_space = space;
      break;
  }

  const Babl *srgb = babl_space ("sRGB");
  if (!state->gstate.texture_space) 
       state->gstate.texture_space = srgb;
  if (!state->gstate.device_space) 
       state->gstate.device_space = srgb;
  if (!state->gstate.rgb_space) 
       state->gstate.rgb_space = srgb;

  //fprintf (stderr, "%s\n", babl_get_name (state->gstate.device_space));

  state->gstate.fish_rgbaf_device_to_user = babl_fish (
       babl_format_with_space ("R'G'B'A float", state->gstate.device_space),
       babl_format_with_space ("R'G'B'A float", state->gstate.rgb_space));
  state->gstate.fish_rgbaf_user_to_device = babl_fish (
       babl_format_with_space ("R'G'B'A float", state->gstate.rgb_space),
       babl_format_with_space ("R'G'B'A float", state->gstate.device_space));
  state->gstate.fish_rgbaf_texture_to_device = babl_fish (
       babl_format_with_space ("R'G'B'A float", state->gstate.texture_space),
       babl_format_with_space ("R'G'B'A float", state->gstate.device_space));
}
#endif

void ctx_rasterizer_colorspace_icc (CtxState      *state,
                                    CtxColorSpace  space_slot,
                                    char          *icc_data,
                                    int            icc_length)
{
#if CTX_BABL
   const char *error = NULL;
   const Babl *space = NULL;

   if (icc_data == NULL) space = babl_space ("sRGB");
   else if (icc_length < 16)
   {
      char tmp[24];
      int i;
      for (i = 0; i < icc_length; i++)
        tmp[i]= (icc_data[i]>='A' && icc_data[i]<='Z')?icc_data[i]+('a'-'A'):icc_data[i];
      tmp[icc_length]=0;
      if (!strcmp (tmp, "srgb"))            space = babl_space ("sRGB");
      else if (!strcmp (tmp, "scrgb"))      space = babl_space ("scRGB");
      else if (!strcmp (tmp, "acescg"))     space = babl_space ("ACEScg");
      else if (!strcmp (tmp, "adobe"))      space = babl_space ("Adobe");
      else if (!strcmp (tmp, "apple"))      space = babl_space ("Apple");
      else if (!strcmp (tmp, "rec2020"))    space = babl_space ("Rec2020");
      else if (!strcmp (tmp, "aces2065-1")) space = babl_space ("ACES2065-1");
   }

   if (!space)
   {
     space = babl_space_from_icc (icc_data, icc_length, BABL_ICC_INTENT_RELATIVE_COLORIMETRIC, &error);
   }
   if (space)
   {
     ctx_rasterizer_colorspace_babl (state, space_slot, space);
   }
#endif
}

void ctx_colorspace (Ctx           *ctx,
                     CtxColorSpace  space_slot,
                     unsigned char *data,
                     int            data_length)
{
  if (data)
  {
    if (data_length <= 0) data_length = (int)strlen ((char*)data);
    ctx_process_cmd_str_with_len (ctx, CTX_COLOR_SPACE, (char*)data, space_slot, 0, data_length);
  }
  else
  {
    ctx_process_cmd_str_with_len (ctx, CTX_COLOR_SPACE, "sRGB", space_slot, 0, 4);
  }
}

void ctx_gradient_add_stop_u8
(Ctx *ctx, float pos, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry entry = ctx_f (CTX_GRADIENT_STOP, pos, 0);
  entry.data.u8[4+0] = r;
  entry.data.u8[4+1] = g;
  entry.data.u8[4+2] = b;
  entry.data.u8[4+3] = a;
  ctx_process (ctx, &entry);
}

void ctx_gradient_add_stop
(Ctx *ctx, float pos, float r, float g, float b, float a)
{
  int ir = r * 255;
  int ig = g * 255;
  int ib = b * 255;
  int ia = a * 255;
  ir = CTX_CLAMP (ir, 0,255);
  ig = CTX_CLAMP (ig, 0,255);
  ib = CTX_CLAMP (ib, 0,255);
  ia = CTX_CLAMP (ia, 0,255);
  ctx_gradient_add_stop_u8 (ctx, pos, ir, ig, ib, ia);
}

void ctx_gradient_add_stop_string
(Ctx *ctx, float pos, const char *string)
{
  CtxColor color = {0,};
  ctx_color_set_from_string (ctx, &color, string);
  float rgba[4];
  ctx_color_get_rgba (&(ctx->state), &color, rgba);
  ctx_gradient_add_stop (ctx, pos, rgba[0], rgba[1], rgba[2], rgba[3]);
}

//  deviceRGB .. settable when creating an RGB image surface..
//               queryable when running in terminal - is it really needed?
//               though it is settable ; and functional for changing this state at runtime..
//
//  userRGB - settable at any time, stored in save|restore 
//  texture - set as the space of data on subsequent 

float ctx_state_get (CtxState *state, uint64_t hash)
{
  for (int i = state->gstate.keydb_pos-1; i>=0; i--)
    {
      if (state->keydb[i].key == hash)
        { return state->keydb[i].value; }
    }
  return -0.0;
}

void ctx_state_set (CtxState *state, uint64_t key, float value)
{
  if (key != CTX_new_state)
    {
      if (ctx_state_get (state, key) == value)
        { return; }
      for (int i = state->gstate.keydb_pos-1;
           state->keydb[i].key != CTX_new_state && i >=0;
           i--)
        {
          if (state->keydb[i].key == key)
            {
              state->keydb[i].value = value;
              return;
            }
        }
    }
  if (state->gstate.keydb_pos >= CTX_MAX_KEYDB)
    { return; }
  state->keydb[state->gstate.keydb_pos].key = key;
  state->keydb[state->gstate.keydb_pos].value = value;
  state->gstate.keydb_pos++;
}


#define CTX_KEYDB_STRING_START (-90000.0)
#define CTX_KEYDB_STRING_END   (CTX_KEYDB_STRING_START + CTX_STRINGPOOL_SIZE)

static int ctx_float_is_string (float val)
{
  return val >= CTX_KEYDB_STRING_START && val <= CTX_KEYDB_STRING_END;
}

static int ctx_float_to_string_index (float val)
{
  int idx = -1;
  if (ctx_float_is_string (val))
  {
    idx = val - CTX_KEYDB_STRING_START;
  }
  return idx;
}

static float ctx_string_index_to_float (int index)
{
  return CTX_KEYDB_STRING_START + index;
}

void *ctx_state_get_blob (CtxState *state, uint64_t key)
{
  float stored = ctx_state_get (state, key);
  int idx = ctx_float_to_string_index (stored);
  if (idx >= 0)
  {
     // can we know length?
     return &state->stringpool[idx];
  }

  // format number as string?
  return NULL;
}

const char *ctx_state_get_string (CtxState *state, uint64_t key)
{
  const char *ret = (char*)ctx_state_get_blob (state, key);
  if (ret && ret[0] == 127)
    return NULL;
  return ret;
}


static void ctx_state_set_blob (CtxState *state, uint64_t key, uint8_t *data, int len)
{
  int idx = state->gstate.stringpool_pos;

  if (idx + len > CTX_STRINGPOOL_SIZE)
  {
    ctx_log ("blowing varpool size [%c..]\n", data[0]);
    //fprintf (stderr, "blowing varpool size [%c%c%c..]\n", data[0],data[1], data[1]?data[2]:0);
#if 0
    for (int i = 0; i< CTX_STRINGPOOL_SIZE; i++)
    {
       if (i==0) fprintf (stderr, "\n%i ", i);
       else      fprintf (stderr, "%c", state->stringpool[i]);
    }
#endif
    return;
  }

  memcpy (&state->stringpool[idx], data, len);
  state->gstate.stringpool_pos+=len;
  state->stringpool[state->gstate.stringpool_pos++]=0;
  ctx_state_set (state, key, ctx_string_index_to_float (idx));
}

static void ctx_state_set_string (CtxState *state, uint64_t key, const char *string)
{
  float old_val = ctx_state_get (state, key);
  int   old_idx = ctx_float_to_string_index (old_val);

  if (old_idx >= 0)
  {
    const char *old_string = ctx_state_get_string (state, key);
    if (old_string && !strcmp (old_string, string))
      return;
  }

  if (ctx_str_is_number (string))
  {
    ctx_state_set (state, key, strtod (string, NULL));
    return;
  }
  // should do same with color
 
  // XXX should special case when the string modified is at the
  //     end of the stringpool.
  //
  //     for clips the behavior is howevre ideal, since
  //     we can have more than one clip per save/restore level
  ctx_state_set_blob (state, key, (uint8_t*)string, strlen(string));
}

static int ctx_state_get_color (CtxState *state, uint64_t key, CtxColor *color)
{
  CtxColor *stored = (CtxColor*)ctx_state_get_blob (state, key);
  if (stored)
  {
    if (stored->magic == 127)
    {
      *color = *stored;
      return 0;
    }
  }
  return -1;
}

static void ctx_state_set_color (CtxState *state, uint64_t key, CtxColor *color)
{
  CtxColor mod_color;
  CtxColor old_color;
  mod_color = *color;
  mod_color.magic = 127;
  if (ctx_state_get_color (state, key, &old_color)==0)
  {
    if (!memcmp (&mod_color, &old_color, sizeof (mod_color)))
      return;
  }
  ctx_state_set_blob (state, key, (uint8_t*)&mod_color, sizeof (CtxColor));
}

const char *ctx_get_string (Ctx *ctx, uint64_t hash)
{
  return ctx_state_get_string (&ctx->state, hash);
}
float ctx_get_float (Ctx *ctx, uint64_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
int ctx_get_int (Ctx *ctx, uint64_t hash)
{
  return ctx_state_get (&ctx->state, hash);
}
void ctx_set_float (Ctx *ctx, uint64_t hash, float value)
{
  ctx_state_set (&ctx->state, hash, value);
}
void ctx_set_string (Ctx *ctx, uint64_t hash, const char *value)
{
  ctx_state_set_string (&ctx->state, hash, value);
}
void ctx_set_color (Ctx *ctx, uint64_t hash, CtxColor *color)
{
  ctx_state_set_color (&ctx->state, hash, color);
}
int  ctx_get_color (Ctx *ctx, uint64_t hash, CtxColor *color)
{
  return ctx_state_get_color (&ctx->state, hash, color);
}
int ctx_is_set (Ctx *ctx, uint64_t hash)
{
  return ctx_get_float (ctx, hash) != -0.0f;
}
int ctx_is_set_now (Ctx *ctx, uint64_t hash)
{
  return ctx_is_set (ctx, hash);
}
#if CTX_RASTERIZER

void ctx_compositor_setup_default (CtxRasterizer *rasterizer);

#define CTX_FULL_AA 15
inline static void
ctx_rasterizer_apply_coverage (CtxRasterizer *rasterizer,
                               uint8_t * __restrict__ dst,
                               int            x,
                               uint8_t * __restrict__ coverage,
                               int            count)
{
  if (rasterizer->format->apply_coverage)
    rasterizer->format->apply_coverage(rasterizer, dst, rasterizer->color, x, coverage, count);
  else
    rasterizer->comp_op (rasterizer, dst, rasterizer->color, x, coverage, count);
}

static void
ctx_rasterizer_gradient_add_stop (CtxRasterizer *rasterizer, float pos, float *rgba)
{
  /* FIXME XXX we only have one gradient, but might need separate gradients
   * for fill/stroke !
   * 
   */
  CtxGradient *gradient = &rasterizer->state->gradient;
  CtxGradientStop *stop = &gradient->stops[gradient->n_stops];
  stop->pos = pos;
  ctx_color_set_rgba (rasterizer->state, & (stop->color), rgba[0], rgba[1], rgba[2], rgba[3]);
  if (gradient->n_stops < 15) //we'll keep overwriting the last when out of stops
    { gradient->n_stops++; }
}

static int ctx_rasterizer_add_point (CtxRasterizer *rasterizer, int x1, int y1)
{
  CtxEntry entry = {CTX_EDGE, {{0},}};
  if (y1 < rasterizer->scan_min)
    { rasterizer->scan_min = y1; }
  if (y1 > rasterizer->scan_max)
    { rasterizer->scan_max = y1; }

  if (x1 < rasterizer->col_min)
    { rasterizer->col_min = x1; }
  if (x1 > rasterizer->col_max)
    { rasterizer->col_max = x1; }

  entry.data.s16[2]=x1;
  entry.data.s16[3]=y1;
  return ctx_drawlist_add_single (&rasterizer->edge_list, &entry);
}

#if 0
#define CTX_SHAPE_CACHE_PRIME1   7853
#define CTX_SHAPE_CACHE_PRIME2   4129
#define CTX_SHAPE_CACHE_PRIME3   3371
#define CTX_SHAPE_CACHE_PRIME4   4221
#else
#define CTX_SHAPE_CACHE_PRIME1   283
#define CTX_SHAPE_CACHE_PRIME2   599
#define CTX_SHAPE_CACHE_PRIME3   101
#define CTX_SHAPE_CACHE_PRIME4   661
#endif

float ctx_shape_cache_rate = 0.0;
#if CTX_SHAPE_CACHE
int   _ctx_shape_cache_enabled = 1;

//static CtxShapeCache ctx_cache = {{NULL,}, 0};

static long ctx_shape_cache_hits = 0;
static long ctx_shape_cache_misses = 0;


/* this returns the buffer to use for rendering, it always
   succeeds..
 */
static CtxShapeEntry *ctx_shape_entry_find (CtxRasterizer *rasterizer, uint32_t hash, int width, int height)
{
  /* use both some high and some low bits  */
  int entry_no = ( (hash >> 10) ^ (hash & 1023) ) % CTX_SHAPE_CACHE_ENTRIES;
  int i;
  {
    static int i = 0;
    i++;
    if (i>1000)
      {
        ctx_shape_cache_rate = ctx_shape_cache_hits * 100.0  / (ctx_shape_cache_hits+ctx_shape_cache_misses);
        i = 0;
        ctx_shape_cache_hits = 0;
        ctx_shape_cache_misses = 0;
      }
  }
// XXX : this 1 one is needed  to silence a false positive:
// ==90718== Invalid write of size 1
// ==90718==    at 0x1189EF: ctx_rasterizer_generate_coverage (ctx.h:4786)
// ==90718==    by 0x118E57: ctx_rasterizer_rasterize_edges (ctx.h:4907)
//
  int size = sizeof (CtxShapeEntry) + width * height + 1;

  i = entry_no;
  if (rasterizer->shape_cache.entries[i])
    {
      CtxShapeEntry *entry = rasterizer->shape_cache.entries[i];
      int old_size = sizeof (CtxShapeEntry) + width + height + 1;
      if (entry->hash == hash &&
          entry->width == width &&
          entry->height == height)
        {
          if (entry->uses < 1<<30)
            { entry->uses++; }
          ctx_shape_cache_hits ++;
          return entry;
        }

      if (old_size >= size)
      {
      }
      else
      {
        rasterizer->shape_cache.entries[i] = NULL;
        rasterizer->shape_cache.size -= entry->width * entry->height;
        rasterizer->shape_cache.size -= sizeof (CtxShapeEntry);
        free (entry);
        rasterizer->shape_cache.entries[i] = (CtxShapeEntry *) calloc (size, 1);
      }
    }
  else
    {
        rasterizer->shape_cache.entries[i] = (CtxShapeEntry *) calloc (size, 1);
    }

  ctx_shape_cache_misses ++;
  rasterizer->shape_cache.size              += size;
  rasterizer->shape_cache.entries[i]->hash   = hash;
  rasterizer->shape_cache.entries[i]->width  = width;
  rasterizer->shape_cache.entries[i]->height = height;
  rasterizer->shape_cache.entries[i]->uses = 0;
  return rasterizer->shape_cache.entries[i];
}

#endif

static uint32_t ctx_rasterizer_poly_to_hash (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;

  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = ox;//(ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += oy; //(oy % CTX_RASTERIZER_AA);
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      x = entry->data.s16[2];
      y = entry->data.s16[3];
      int dx = x-ox;
      int dy = y-oy;
      ox = x;
      oy = y;
      hash *= CTX_SHAPE_CACHE_PRIME3;
      hash += dx;
      hash *= CTX_SHAPE_CACHE_PRIME4;
      hash += dy;
    }
  return hash;
}

static uint32_t ctx_rasterizer_poly_to_edges (CtxRasterizer *rasterizer)
{
  int16_t x = 0;
  int16_t y = 0;
  if (rasterizer->edge_list.count == 0)
     return 0;
#if CTX_SHAPE_CACHE
  CtxEntry *entry = &rasterizer->edge_list.entries[0];
  int ox = entry->data.s16[2];
  int oy = entry->data.s16[3];
  uint32_t hash = rasterizer->edge_list.count;
  hash = (ox % CTX_SUBDIV);
  hash *= CTX_SHAPE_CACHE_PRIME1;
  hash += (oy % CTX_FULL_AA);
#endif
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      if (entry->code == CTX_NEW_EDGE)
        {
          entry->code = CTX_EDGE;
#if CTX_SHAPE_CACHE
          hash *= CTX_SHAPE_CACHE_PRIME2;
#endif
        }
      else
        {
          entry->data.s16[0] = x;
          entry->data.s16[1] = y;
        }
      x = entry->data.s16[2];
      y = entry->data.s16[3];
#if CTX_SHAPE_CACHE
      int dx = x-ox;
      int dy = y-oy;
      ox = x;
      oy = y;
      hash *= CTX_SHAPE_CACHE_PRIME3;
      hash += dx;
      hash *= CTX_SHAPE_CACHE_PRIME4;
      hash += dy;
#endif
      if (entry->data.s16[3] < entry->data.s16[1])
        {
          *entry = ctx_s16 (CTX_EDGE_FLIPPED,
                            entry->data.s16[2], entry->data.s16[3],
                            entry->data.s16[0], entry->data.s16[1]);
        }
    }
#if CTX_SHAPE_CACHE
  return hash;
#else
  return 0;
#endif
}

static void ctx_rasterizer_finish_shape (CtxRasterizer *rasterizer)
{
  if (rasterizer->has_shape && rasterizer->has_prev)
    {
      ctx_rasterizer_line_to (rasterizer, rasterizer->first_x, rasterizer->first_y);
      rasterizer->has_prev = 0;
    }
}

static void ctx_rasterizer_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x; float ty = y;
  int aa = 15;//rasterizer->aa;
  rasterizer->x        = x;
  rasterizer->y        = y;
  rasterizer->first_x  = x;
  rasterizer->first_y  = y;
  rasterizer->has_prev = -1;
  if (rasterizer->uses_transforms)
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }

  tx = (tx - rasterizer->blit_x) * CTX_SUBDIV;
  ty = ty * aa;

  if (ty < rasterizer->scan_min)
    { rasterizer->scan_min = ty; }
  if (ty > rasterizer->scan_max)
    { rasterizer->scan_max = ty; }
  if (tx < rasterizer->col_min)
    { rasterizer->col_min = tx; }
  if (tx > rasterizer->col_max)
    { rasterizer->col_max = tx; }
}



static inline void ctx_rasterizer_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  float tx = x;
  float ty = y;
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  if (rasterizer->uses_transforms)
    {
      _ctx_user_to_device (rasterizer->state, &tx, &ty);
    }
  tx -= rasterizer->blit_x;
#define MIN_Y -1000
#define MAX_Y 1400

  if (ty < MIN_Y) ty = MIN_Y;
  if (ty > MAX_Y) ty = MAX_Y;
  
  ctx_rasterizer_add_point (rasterizer, tx * CTX_SUBDIV, ty * 15);//rasterizer->aa);

  if (rasterizer->has_prev<=0)
    {
      if (rasterizer->uses_transforms)
      {
        // storing transformed would save some processing for a tiny
        // amount of runtime RAM XXX
        _ctx_user_to_device (rasterizer->state, &ox, &oy);
      }
      ox -= rasterizer->blit_x;

  if (oy < MIN_Y) oy = MIN_Y;
  if (oy > MAX_Y) oy = MAX_Y;

      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[0] = ox * CTX_SUBDIV;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].data.s16[1] = oy * 15;//rasterizer->aa;
      rasterizer->edge_list.entries[rasterizer->edge_list.count-1].code = CTX_NEW_EDGE;
      rasterizer->has_prev = 1;
    }
  rasterizer->has_shape = 1;
  rasterizer->y         = y;
  rasterizer->x         = x;
}


CTX_INLINE static float
ctx_bezier_sample_1d (float x0, float x1, float x2, float x3, float dt)
{
  float ab   = ctx_lerpf (x0, x1, dt);
  float bc   = ctx_lerpf (x1, x2, dt);
  float cd   = ctx_lerpf (x2, x3, dt);
  float abbc = ctx_lerpf (ab, bc, dt);
  float bccd = ctx_lerpf (bc, cd, dt);
  return ctx_lerpf (abbc, bccd, dt);
}

inline static void
ctx_bezier_sample (float x0, float y0,
                   float x1, float y1,
                   float x2, float y2,
                   float x3, float y3,
                   float dt, float *x, float *y)
{
  *x = ctx_bezier_sample_1d (x0, x1, x2, x3, dt);
  *y = ctx_bezier_sample_1d (y0, y1, y2, y3, dt);
}

static inline void
ctx_rasterizer_bezier_divide (CtxRasterizer *rasterizer,
                              float ox, float oy,
                              float x0, float y0,
                              float x1, float y1,
                              float x2, float y2,
                              float sx, float sy,
                              float ex, float ey,
                              float s,
                              float e,
                              int   iteration,
                              float tolerance)
{
  if (iteration > 8)
    { return; }
  float t = (s + e) * 0.5f;
  float x, y, lx, ly, dx, dy;
  ctx_bezier_sample (ox, oy, x0, y0, x1, y1, x2, y2, t, &x, &y);
  if (iteration)
    {
      lx = ctx_lerpf (sx, ex, t);
      ly = ctx_lerpf (sy, ey, t);
      dx = lx - x;
      dy = ly - y;
      if ( (dx*dx+dy*dy) < tolerance)
        /* bailing - because for the mid-point straight line difference is
           tiny */
        { return; }
      dx = sx - ex;
      dy = ey - ey;
      if ( (dx*dx+dy*dy) < tolerance)
        /* bailing on tiny segments */
        { return; }
    }
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                sx, sy, x, y, s, t, iteration + 1,
                                tolerance);
  ctx_rasterizer_line_to (rasterizer, x, y);
  ctx_rasterizer_bezier_divide (rasterizer, ox, oy, x0, y0, x1, y1, x2, y2,
                                x, y, ex, ey, t, e, iteration + 1,
                                tolerance);
}

static void
ctx_rasterizer_curve_to (CtxRasterizer *rasterizer,
                         float x0, float y0,
                         float x1, float y1,
                         float x2, float y2)
{
  float tolerance =
    ctx_pow2 (rasterizer->state->gstate.transform.m[0][0]) +
    ctx_pow2 (rasterizer->state->gstate.transform.m[1][1]);
  float ox = rasterizer->x;
  float oy = rasterizer->y;
  ox = rasterizer->state->x;
  oy = rasterizer->state->y;
  tolerance = 1.0f/tolerance * 2;
#if 1 // skipping this to preserve hash integrity
  if (tolerance == 1.0f || 1)
  {
  float maxx = ctx_maxf (x1,x2);
  maxx = ctx_maxf (maxx, ox);
  maxx = ctx_maxf (maxx, x0);
  float maxy = ctx_maxf (y1,y2);
  maxy = ctx_maxf (maxy, oy);
  maxy = ctx_maxf (maxy, y0);
  float minx = ctx_minf (x1,x2);
  minx = ctx_minf (minx, ox);
  minx = ctx_minf (minx, x0);
  float miny = ctx_minf (y1,y2);
  miny = ctx_minf (miny, oy);
  miny = ctx_minf (miny, y0);
  
  _ctx_user_to_device (rasterizer->state, &minx, &miny);
  _ctx_user_to_device (rasterizer->state, &maxx, &maxy);
#if 1
    if(
        (minx > rasterizer->blit_x + rasterizer->blit_width) ||
        (miny > rasterizer->blit_y + rasterizer->blit_height) ||
        (maxx < rasterizer->blit_x) ||
        (maxy < rasterizer->blit_y) )
    {
    }
    else
#endif
    {
      ctx_rasterizer_bezier_divide (rasterizer,
                                    ox, oy, x0, y0,
                                    x1, y1, x2, y2,
                                    ox, oy, x2, y2,
                                    0.0f, 1.0f, 0.0f, tolerance);
    }
  }
  else
#endif
    {
      ctx_rasterizer_bezier_divide (rasterizer,
                                    ox, oy, x0, y0,
                                    x1, y1, x2, y2,
                                    ox, oy, x2, y2,
                                    0.0f, 1.0f, 0.0f, tolerance);
    }
  ctx_rasterizer_line_to (rasterizer, x2, y2);
}

static void
ctx_rasterizer_rel_move_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (x == 0.f && y == 0.f)
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_move_to (rasterizer, x, y);
}

static void
ctx_rasterizer_rel_line_to (CtxRasterizer *rasterizer, float x, float y)
{
  if (x== 0.f && y==0.f)
    { return; }
  x += rasterizer->x;
  y += rasterizer->y;
  ctx_rasterizer_line_to (rasterizer, x, y);
}

static void
ctx_rasterizer_rel_curve_to (CtxRasterizer *rasterizer,
                             float x0, float y0, float x1, float y1, float x2, float y2)
{
  x0 += rasterizer->x;
  y0 += rasterizer->y;
  x1 += rasterizer->x;
  y1 += rasterizer->y;
  x2 += rasterizer->x;
  y2 += rasterizer->y;
  ctx_rasterizer_curve_to (rasterizer, x0, y0, x1, y1, x2, y2);
}


static int
ctx_rasterizer_find_texture (CtxRasterizer *rasterizer,
                             const char *eid)
{
  int no;
  for (no = 0; no < CTX_MAX_TEXTURES; no++)
  {
    if (rasterizer->texture_source->texture[no].data &&
        rasterizer->texture_source->texture[no].eid &&
        !strcmp (rasterizer->texture_source->texture[no].eid, eid))
      return no;
  }
  return -1;
}

static void
ctx_rasterizer_set_texture (CtxRasterizer *rasterizer,
                            const char *eid,
                            float x,
                            float y)
{
  int is_stroke = (rasterizer->state->source != 0);
  CtxSource *source = is_stroke && (rasterizer->state->gstate.source_stroke.type != CTX_SOURCE_INHERIT_FILL)?
                        &rasterizer->state->gstate.source_stroke:
                        &rasterizer->state->gstate.source_fill;
  rasterizer->state->source = 0;

  int no = ctx_rasterizer_find_texture (rasterizer, eid);
  if (no < 0 || no >= CTX_MAX_TEXTURES) { no = 0; }
  if (rasterizer->texture_source->texture[no].data == NULL)
    {
      fprintf (stderr, "ctx tex fail %p %s %i\n", rasterizer->texture_source, eid, no);
      return;
    }
  else
  {
    rasterizer->texture_source->texture[no].frame = rasterizer->texture_source->frame;
  }
  source->type = CTX_SOURCE_TEXTURE;
  source->texture.buffer = &rasterizer->texture_source->texture[no];
  source->texture.x0 = 0;
  source->texture.y0 = 0;
  source->transform = rasterizer->state->gstate.transform;
  ctx_matrix_translate (&source->transform, x, y);
  ctx_matrix_invert (&source->transform);
}


static void ctx_rasterizer_define_texture (CtxRasterizer *rasterizer,
                                           const char *eid,
                                           int width,
                                           int height,
                                           int format,
                                           char unsigned *data)
{
  _ctx_texture_lock (); // we're using the same texture_source from all threads, keeping allocaitons down
                        // need synchronizing (it could be better to do a pre-pass)
  ctx_texture_init (rasterizer->texture_source,
                    eid,
                    width,
                    height,
                    ctx_pixel_format_get_stride ((CtxPixelFormat)format, width),
                    (CtxPixelFormat)format,
#if CTX_ENABLE_CM
                    (void*)rasterizer->state->gstate.texture_space,
#else
                    NULL,
#endif
                    data,
                    ctx_buffer_pixels_free, (void*)23);
                    /*  when userdata for ctx_buffer_pixels_free is 23, texture_init dups the data on
                     *  use
                     */

  _ctx_texture_unlock ();
  ctx_rasterizer_set_texture (rasterizer, eid, 0.0, 0.0);
}


CTX_INLINE static int ctx_compare_edges (const void *ap, const void *bp)
{
  const CtxEntry *a = (const CtxEntry *) ap;
  const CtxEntry *b = (const CtxEntry *) bp;
  int ycompare = a->data.s16[1] - b->data.s16[1];
  if (ycompare)
    { return ycompare; }
  int xcompare = a->data.s16[0] - b->data.s16[0];
  return xcompare;
}

CTX_INLINE static int ctx_edge_qsort_partition (CtxEntry *A, int low, int high)
{
  CtxEntry pivot = A[ (high+low) /2];
  int i = low;
  int j = high;
  while (i <= j)
    {
      while (ctx_compare_edges (&A[i], &pivot) <0) { i ++; }
      while (ctx_compare_edges (&pivot, &A[j]) <0) { j --; }
      if (i <= j)
        {
          CtxEntry tmp = A[i];
          A[i] = A[j];
          A[j] = tmp;
          i++;
          j--;
        }
    }
  return i;
}

static void ctx_edge_qsort (CtxEntry *entries, int low, int high)
{
  {
    int p = ctx_edge_qsort_partition (entries, low, high);
    if (low < p -1 )
      { ctx_edge_qsort (entries, low, p - 1); }
    if (low < high)
      { ctx_edge_qsort (entries, p, high); }
  }
}

static CTX_INLINE void ctx_rasterizer_sort_edges (CtxRasterizer *rasterizer)
{
  ctx_edge_qsort (& (rasterizer->edge_list.entries[0]), 0, rasterizer->edge_list.count-1);
}


static inline void ctx_rasterizer_discard_edges (CtxRasterizer *rasterizer)
{
  int scanline = rasterizer->scanline;
  int slope_limit3 = CTX_RASTERIZER_AA_SLOPE_LIMIT3;
  int slope_limit5 = CTX_RASTERIZER_AA_SLOPE_LIMIT5;
  int slope_limit15 = CTX_RASTERIZER_AA_SLOPE_LIMIT15;
  rasterizer->ending_edges = 0;
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      int edge_end =rasterizer->edge_list.entries[rasterizer->edges[i].index].data.s16[3]-1;
      if (CTX_UNLIKELY(edge_end < scanline))
        {
          int dx_dy = abs(rasterizer->edges[i].delta);
          if (dx_dy > slope_limit15)
            { rasterizer->needs_aa15 --; }
          else if (dx_dy > slope_limit5)
            { rasterizer->needs_aa5 --; }
          else if (dx_dy > slope_limit3)
            { rasterizer->needs_aa3 --; }
          rasterizer->edges[i] = rasterizer->edges[rasterizer->active_edges-1];
          rasterizer->active_edges--;
          i--;
        }
      else if (edge_end < scanline + CTX_FULL_AA)
        rasterizer->ending_edges = 1; // only used as a flag!
    }
}

inline static void ctx_rasterizer_increment_edges (CtxRasterizer *rasterizer, int count)
{
  rasterizer->scanline += count;
  for (int i = 0; i < rasterizer->active_edges; i++)
    {
      rasterizer->edges[i].val += rasterizer->edges[i].delta * count;
    }
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      rasterizer->edges[CTX_MAX_EDGES-1-i].val += rasterizer->edges[CTX_MAX_EDGES-1-i].delta * count;
    }
}

/* feeds up to rasterizer->scanline,
   keeps a pending buffer of edges - that encompass
   the full incoming scanline,
   feed until the start of the scanline and check for need for aa
   in all of pending + active edges, then
   again feed_edges until middle of scanline if doing non-AA
   or directly render when doing AA
*/
inline static void ctx_rasterizer_feed_edges (CtxRasterizer *rasterizer)
{
  int miny;
  CtxEntry *entries = rasterizer->edge_list.entries;
  for (int i = 0; i < rasterizer->pending_edges; i++)
    {
      if (entries[rasterizer->edges[CTX_MAX_EDGES-1-i].index].data.s16[1] - 1 <= rasterizer->scanline)
        {
          if (CTX_LIKELY(rasterizer->active_edges < CTX_MAX_EDGES-2))
            {
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no] = rasterizer->edges[CTX_MAX_EDGES-1-i];
              rasterizer->edges[CTX_MAX_EDGES-1-i] =
                rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges + 1];
              rasterizer->pending_edges--;
              i--;
            }
        }
    }
  int scanline = rasterizer->scanline;
  while (CTX_LIKELY(rasterizer->edge_pos < rasterizer->edge_list.count &&
         (miny=entries[rasterizer->edge_pos].data.s16[1]-1)  <= scanline + 15))
    {
      int maxy=entries[rasterizer->edge_pos].data.s16[3]-1;
      if (CTX_LIKELY(rasterizer->active_edges < CTX_MAX_EDGES-2) &&
          maxy >= scanline)
        {
          int dy = (entries[rasterizer->edge_pos].data.s16[3] - 1 - miny);
          if (dy) /* skipping horizontal edges */
            {
              int yd = scanline - miny;
              int no = rasterizer->active_edges;
              rasterizer->active_edges++;
              rasterizer->edges[no].index = rasterizer->edge_pos;
              int index = rasterizer->edges[no].index;
              int x0 = entries[index].data.s16[0];
              int x1 = entries[index].data.s16[2];
              rasterizer->edges[no].val = x0 * CTX_RASTERIZER_EDGE_MULTIPLIER;
              int dx_dy;
              dx_dy = CTX_RASTERIZER_EDGE_MULTIPLIER * (x1 - x0) / dy;
              rasterizer->edges[no].delta = dx_dy;
              rasterizer->edges[no].val += (yd * dx_dy);

              {
                int abs_dx_dy = abs(dx_dy);
                if (abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT3)
                  { rasterizer->needs_aa3 ++; }
                if (abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT5)
                  { rasterizer->needs_aa5 ++; }
                if (abs_dx_dy> CTX_RASTERIZER_AA_SLOPE_LIMIT15)
                  { rasterizer->needs_aa15 ++; }
              }

              if ((miny > scanline) )
                {
                  /* it is a pending edge - we add it to the end of the array
                     and keep a different count for items stored here, like
                     a heap and stack growing against each other
                  */
                  if (rasterizer->pending_edges < CTX_MAX_PENDING-1)
                  {
                    rasterizer->edges[CTX_MAX_EDGES-1-rasterizer->pending_edges] =
                    rasterizer->edges[no];
                    rasterizer->pending_edges++;
                    rasterizer->active_edges--;
                  }
                }
            }
        }
      rasterizer->edge_pos++;
    }

    ctx_rasterizer_discard_edges (rasterizer);
}

CTX_INLINE static int ctx_compare_edges2 (const void *ap, const void *bp)
{
  const CtxEdge *a = (const CtxEdge *) ap;
  const CtxEdge *b = (const CtxEdge *) bp;
  return a->val - b->val;
}

CTX_INLINE static void ctx_edge2_insertion_sort (CtxEdge *entries, int count)
{
  for(int i=1; i<count; i++)
   {
     CtxEdge temp = entries[i];
     int j = i-1;
     while (j >= 0 && ctx_compare_edges2 (&temp, &entries[j])<0)
     {
       entries[j+1] = entries[j];
       j--;
     }
     entries[j+1] = temp;
   }
}

static inline void ctx_rasterizer_sort_active_edges (CtxRasterizer *rasterizer)
{
  ctx_edge2_insertion_sort (rasterizer->edges, rasterizer->active_edges);
}

#undef CTX_CMPSWP

void ctx_coverage_post_process (CtxRasterizer *rasterizer, int minx, int maxx, uint8_t *coverage)
{
  int scanline     = rasterizer->scanline;
#if CTX_ENABLE_SHADOW_BLUR
  if (CTX_UNLIKELY(rasterizer->in_shadow))
  {
    float radius = rasterizer->state->gstate.shadow_blur;
    int dim = 2 * radius + 1;
    if (CTX_UNLIKELY (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM))
      dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
    {
      uint16_t temp[maxx-minx+1];
      memset (temp, 0, sizeof (temp));
      for (int x = dim/2; x < maxx-minx + 1 - dim/2; x ++)
        for (int u = 0; u < dim; u ++)
        {
            temp[x] += coverage[minx+x+u-dim/2] * rasterizer->kernel[u] * 256;
        }
      for (int x = 0; x < maxx-minx + 1; x ++)
        coverage[minx+x] = temp[x] >> 8;
    }
  }
#endif

#if CTX_ENABLE_CLIP
  if (CTX_UNLIKELY(rasterizer->clip_buffer &&  !rasterizer->clip_rectangle))
  {
    /* perhaps not working right for clear? */
    int y = scanline / 15;//rasterizer->aa;
    uint8_t *clip_line = &((uint8_t*)(rasterizer->clip_buffer->data))[rasterizer->blit_width*y];
    // XXX SIMD candidate
    for (int x = minx; x <= maxx; x ++)
    {
#if CTX_1BIT_CLIP
        coverage[x] = (coverage[x] * ((clip_line[x/8]&(1<<(x%8)))?255:0))/255;
#else
        coverage[x] = (coverage[x] * clip_line[x])/255;
#endif
    }
  }
  if (CTX_UNLIKELY(rasterizer->aa == 1))
  {
    for (int x = minx; x <= maxx; x ++)
     coverage[x] = coverage[x] > 127?255:0;
  }
#endif
}

inline static void
ctx_rasterizer_generate_coverage (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            winding,
                                  int            aa_factor)
{
  CtxEntry *entries = rasterizer->edge_list.entries;;
  CtxEdge  *edges = rasterizer->edges;
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  int fraction = 255/aa_factor;
  coverage -= minx;
#define CTX_EDGE(no)      entries[edges[no].index]
#define CTX_EDGE_YMIN(no) (CTX_EDGE(no).data.s16[1]-1)
#define CTX_EDGE_X(no)    (edges[no].val)
  for (int t = 0; t < active_edges -1;t++)
    {
      int ymin = CTX_EDGE_YMIN (t);
      if (scanline != ymin)
        {
          if (winding)
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
          else
            { parity = 1 - parity; }
        }

       if (parity)
        {
          int x0 = CTX_EDGE_X (t);
          int x1 = CTX_EDGE_X (t+1);
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart / 256;
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int last      = grayend / 256;

          if (CTX_UNLIKELY(first < minx))
          { 
            first = minx;
            graystart=0;//255;
          }
          if (CTX_UNLIKELY(last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart=fraction- (graystart&0xff)/aa_factor;
          grayend = (grayend & 0xff) / aa_factor;

          if (first == last)
            coverage[first] += (graystart-(fraction-grayend));
          else if (first < last)
          {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x] += fraction;
              coverage[last]  += grayend;
          }
        }
   }
}

inline static void
ctx_rasterizer_generate_coverage_set (CtxRasterizer *rasterizer,
                                  int            minx,
                                  int            maxx,
                                  uint8_t       *coverage,
                                  int            winding,
                                  int            aa_factor)
{
  CtxEntry *entries = rasterizer->edge_list.entries;;
  CtxEdge  *edges = rasterizer->edges;
  int scanline     = rasterizer->scanline;
  int active_edges = rasterizer->active_edges;
  int parity = 0;
  int fraction = 255/aa_factor;
  coverage -= minx;
#define CTX_EDGE(no)      entries[edges[no].index]
#define CTX_EDGE_YMIN(no) (CTX_EDGE(no).data.s16[1]-1)
#define CTX_EDGE_X(no)    (edges[no].val)
  for (int t = 0; t < active_edges -1;t++)
    {
      int ymin = CTX_EDGE_YMIN (t);
      if (scanline != ymin)
        {
          if (winding)
            { parity += ( (CTX_EDGE (t).code == CTX_EDGE_FLIPPED) ?1:-1); }
          else
            { parity = 1 - parity; }
        }

       if (parity)
        {
          int x0 = CTX_EDGE_X (t);
          int x1 = CTX_EDGE_X (t+1);
          int graystart = x0 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int first     = graystart / 256;
          int grayend   = x1 / (CTX_RASTERIZER_EDGE_MULTIPLIER*CTX_SUBDIV/256);
          int last      = grayend / 256;

          if (CTX_UNLIKELY(first < minx))
          { 
            first = minx;
            graystart=0;//255;
          }
          if (CTX_UNLIKELY(last > maxx))
          {
            last = maxx;
            grayend=255;
          }

          graystart=fraction- (graystart&0xff)/aa_factor;
          grayend = (grayend & 0xff) / aa_factor;

          if (first == last)
            coverage[first] += (graystart-(fraction-grayend));
          else if (first < last)
          {
              coverage[first] += graystart;
              for (int x = first + 1; x < last; x++)
                coverage[x] = fraction;
              coverage[last]  += grayend;
          }
        }
   }
}

#undef CTX_EDGE_Y0
#undef CTX_EDGE

static void
ctx_rasterizer_reset (CtxRasterizer *rasterizer)
{
  rasterizer->pending_edges   = 0;
  rasterizer->active_edges    = 0;
  rasterizer->has_shape       = 0;
  rasterizer->has_prev        = 0;
  rasterizer->edge_list.count = 0; // ready for new edges
  rasterizer->edge_pos        = 0;
  rasterizer->needs_aa3       = 0;
  rasterizer->needs_aa5       = 0;
  rasterizer->needs_aa15      = 0;
  rasterizer->scanline        = 0;
  if (!rasterizer->preserve)
  {
    rasterizer->scan_min      = 5000;
    rasterizer->scan_max      = -5000;
    rasterizer->col_min       = 5000;
    rasterizer->col_max       = -5000;
  }
  //rasterizer->comp_op       = NULL;
}

static void
ctx_rasterizer_rasterize_edges (CtxRasterizer *rasterizer, int winding
#if CTX_SHAPE_CACHE
                                ,CtxShapeEntry *shape
#endif
                               )
{
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);

  int real_aa = rasterizer->aa;

  int scan_start = rasterizer->blit_y * CTX_FULL_AA;
  int scan_end   = scan_start + rasterizer->blit_height * CTX_FULL_AA;
  int blit_width = rasterizer->blit_width;
  int blit_max_x = rasterizer->blit_x + blit_width;
  int minx       = rasterizer->col_min / CTX_SUBDIV - rasterizer->blit_x;
  int maxx       = (rasterizer->col_max + CTX_SUBDIV-1) / CTX_SUBDIV - rasterizer->blit_x;

#if 1
  if (
#if CTX_SHAPE_CACHE
    !shape &&
#endif
    maxx > blit_max_x - 1)
    { maxx = blit_max_x - 1; }
#endif
#if 1
  if (rasterizer->state->gstate.clip_min_x>
      minx)
    { minx = rasterizer->state->gstate.clip_min_x; }
  if (rasterizer->state->gstate.clip_max_x <
      maxx)
    { maxx = rasterizer->state->gstate.clip_max_x; }
#endif
  if (minx < 0)
    { minx = 0; }
  if (minx >= maxx)
    {
      ctx_rasterizer_reset (rasterizer);
      return;
    }
#if CTX_SHAPE_CACHE
  uint8_t _coverage[shape?2:maxx-minx+1];
#else
  uint8_t _coverage[maxx-minx+1];
#endif
  uint8_t *coverage = &_coverage[0];


#if CTX_SHAPE_CACHE
  if (shape)
    {
      coverage = &shape->data[0];
    }
#endif
  ctx_assert (coverage);
  rasterizer->scan_min -= (rasterizer->scan_min % CTX_FULL_AA);
#if CTX_SHAPE_CACHE
  if (shape)
    {
      scan_start = rasterizer->scan_min;
      scan_end   = rasterizer->scan_max;
    }
  else
#endif
    {
      if (rasterizer->scan_min > scan_start)
        {
          dst += (rasterizer->blit_stride * (rasterizer->scan_min-scan_start) / CTX_FULL_AA);
          scan_start = rasterizer->scan_min;
        }
      if (rasterizer->scan_max < scan_end)
        { scan_end = rasterizer->scan_max; }
    }
  if (rasterizer->state->gstate.clip_min_y * CTX_FULL_AA > scan_start )
    { 
       dst += (rasterizer->blit_stride * (rasterizer->state->gstate.clip_min_y * CTX_FULL_AA -scan_start) / CTX_FULL_AA);
       scan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA; 
    }
  if (rasterizer->state->gstate.clip_max_y * CTX_FULL_AA < scan_end)
    { scan_end = rasterizer->state->gstate.clip_max_y * CTX_FULL_AA; }
  if (scan_start > scan_end ||
      (scan_start > (rasterizer->blit_y + rasterizer->blit_height) * CTX_FULL_AA) ||
      (scan_end < (rasterizer->blit_y) * CTX_FULL_AA))
  { 
    /* not affecting this rasterizers scanlines */
    ctx_rasterizer_reset (rasterizer);
    return;
  }

  ctx_rasterizer_sort_edges (rasterizer);
  {
    int halfstep2 = CTX_FULL_AA/2;
    int halfstep  = halfstep2 + 1;
    rasterizer->needs_aa3  = 0;
    rasterizer->needs_aa5  = 0;
    rasterizer->needs_aa15 = 0;
    rasterizer->scanline = scan_start;
    ctx_rasterizer_feed_edges (rasterizer); 

  for (; rasterizer->scanline <= scan_end;)
    {
      int contains_edge_end = rasterizer->pending_edges ||
                              rasterizer->ending_edges;

      // check if all pending edges are scanline aligned at start
      ctx_memset (coverage, 0,
#if CTX_SHAPE_CACHE
                  shape?shape->width:
#endif
                  sizeof (_coverage) );

    if (contains_edge_end)
    {
        for (int i = 0; i < real_aa; i++)
        {
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, real_aa);
          ctx_rasterizer_increment_edges (rasterizer, CTX_FULL_AA/real_aa);
        }
    }
    else if (!rasterizer->needs_aa3) // if it doesnt need aa3 it doesnt need aa5 or aa15 either
    {
      ctx_rasterizer_increment_edges (rasterizer, halfstep2);
      ctx_rasterizer_feed_edges (rasterizer);

      ctx_rasterizer_sort_active_edges (rasterizer);
      ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, winding, 1);
      ctx_rasterizer_increment_edges (rasterizer, halfstep);
    }
    else  if (rasterizer->needs_aa15)
      {
        for (int i = 0; i < CTX_FULL_AA; i++)
        {
          ctx_rasterizer_feed_edges (rasterizer);
          ctx_rasterizer_sort_active_edges (rasterizer);
          ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA);
          ctx_rasterizer_increment_edges (rasterizer, 1);
        }
    }
    else if (rasterizer->needs_aa5)
    {
      for (int i = 0; i < CTX_FULL_AA; i+=3)
      {
        ctx_rasterizer_feed_edges (rasterizer);
        ctx_rasterizer_sort_active_edges (rasterizer);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA/3);
        ctx_rasterizer_increment_edges (rasterizer, 3);
      }
        //for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.5;
    }
    else if (rasterizer->needs_aa3)
    {
      for (int i = 0; i < CTX_FULL_AA; i+=5)
      {
        ctx_rasterizer_feed_edges (rasterizer);
        ctx_rasterizer_sort_active_edges (rasterizer);
        ctx_rasterizer_generate_coverage (rasterizer, minx, maxx, coverage, winding, CTX_FULL_AA/5);
        ctx_rasterizer_increment_edges (rasterizer, 5);
      }
      //for (int x = minx; x <= maxx; x++) coverage[x-minx] *= 0.75;
    }
    else
    {
      ctx_rasterizer_increment_edges (rasterizer, halfstep2);
      ctx_rasterizer_feed_edges (rasterizer);

      ctx_rasterizer_sort_active_edges (rasterizer);
      ctx_rasterizer_generate_coverage_set (rasterizer, minx, maxx, coverage, winding, 1);
      ctx_rasterizer_increment_edges (rasterizer, halfstep);
    }
  ctx_coverage_post_process (rasterizer, minx, maxx, coverage - minx);




        {
#if CTX_SHAPE_CACHE
          if (shape == NULL)
#endif
            {
#if 0
              if (CTX_FULL_AA==1)
              {
                for (int x = 0; x < maxx-minx; x++)
                  coverage
              }
#endif
              ctx_rasterizer_apply_coverage (rasterizer,
                                             &dst[(minx * rasterizer->format->bpp) /8],
                                             minx,
                                             coverage, maxx-minx + 1);
            }
        }
#if CTX_SHAPE_CACHE
      if (shape)
        {
          coverage += shape->width;
        }
#endif
      dst += rasterizer->blit_stride;
    }
  }

  if (rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_OUT ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_SOURCE_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_IN ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_COPY ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_DESTINATION_ATOP ||
      rasterizer->state->gstate.compositing_mode == CTX_COMPOSITE_CLEAR)
  {
     /* fill in the rest of the blitrect when compositing mode permits it */
     uint8_t nocoverage[rasterizer->blit_width];
     //int gscan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA;
     int gscan_start = rasterizer->state->gstate.clip_min_y * CTX_FULL_AA;
     int gscan_end = rasterizer->state->gstate.clip_max_y * CTX_FULL_AA;
     memset (nocoverage, 0, sizeof(nocoverage));
     int startx   = rasterizer->state->gstate.clip_min_x;
     int endx     = rasterizer->state->gstate.clip_max_x;
     int clipw    = endx-startx + 1;
     uint8_t *dst = ( (uint8_t *) rasterizer->buf);

     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (gscan_start / CTX_FULL_AA);
     for (rasterizer->scanline = gscan_start; rasterizer->scanline < scan_start;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw);
       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
     if (minx < startx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, minx-startx);
       dst += rasterizer->blit_stride;
     }
     }
     if (endx > maxx)
     {
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_start / CTX_FULL_AA);
     for (rasterizer->scanline = scan_start; rasterizer->scanline < scan_end;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (maxx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, endx-maxx);

       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
     }
     dst = (uint8_t*)(rasterizer->buf) + rasterizer->blit_stride * (scan_end / CTX_FULL_AA);
     // XXX valgrind/asan this
     if(0)for (rasterizer->scanline = scan_end; rasterizer->scanline/CTX_FULL_AA < gscan_end-1;)
     {
       ctx_rasterizer_apply_coverage (rasterizer,
                                      &dst[ (startx * rasterizer->format->bpp) /8],
                                      0,
                                      nocoverage, clipw-1);

       rasterizer->scanline += CTX_FULL_AA;
       dst += rasterizer->blit_stride;
     }
  }
  ctx_rasterizer_reset (rasterizer);
}

inline static int
ctx_is_transparent (CtxRasterizer *rasterizer, int stroke)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 == 0)
    return 1;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source_fill.color, ga);
    if (ga[1] == 0)
      return 1;
  }
  return 0;
}

static void
ctx_rasterizer_fill (CtxRasterizer *rasterizer)
{
  int count = rasterizer->preserve?rasterizer->edge_list.count:0;

  CtxEntry temp[count]; /* copy of already built up path's poly line
                          XXX - by building a large enough path
                          the stack can be smashed!
                         */
  if (rasterizer->preserve)
    { memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) ); }

#if CTX_ENABLE_SHADOW_BLUR
  if (rasterizer->in_shadow)
  {
  for (int i = 0; i < rasterizer->edge_list.count; i++)
    {
      CtxEntry *entry = &rasterizer->edge_list.entries[i];
      entry->data.s16[2] += rasterizer->shadow_x * CTX_SUBDIV;
      entry->data.s16[3] += rasterizer->shadow_y * CTX_FULL_AA;
    }
    rasterizer->scan_min += rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->scan_max += rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->col_min  += (rasterizer->shadow_x - rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  += (rasterizer->shadow_x + rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif

  if (ctx_is_transparent (rasterizer, 0) ||
      rasterizer->scan_min / CTX_FULL_AA > rasterizer->blit_y + rasterizer->blit_height ||
      rasterizer->scan_max / CTX_FULL_AA < rasterizer->blit_y ||
      rasterizer->col_min / CTX_SUBDIV > rasterizer->blit_x + rasterizer->blit_width ||
      rasterizer->col_max / CTX_SUBDIV < rasterizer->blit_x)
    {
      ctx_rasterizer_reset (rasterizer);
    }
  else
  {
    if (rasterizer->comp_op == NULL)
      ctx_compositor_setup_default (rasterizer);

    rasterizer->state->min_x =
      ctx_mini (rasterizer->state->min_x, rasterizer->col_min / CTX_SUBDIV);
    rasterizer->state->max_x =
      ctx_maxi (rasterizer->state->max_x, rasterizer->col_max / CTX_SUBDIV);
    rasterizer->state->min_y =
      ctx_mini (rasterizer->state->min_y, rasterizer->scan_min / CTX_FULL_AA);
    rasterizer->state->max_y =
      ctx_maxi (rasterizer->state->max_y, rasterizer->scan_max / CTX_FULL_AA);

    ctx_rasterizer_finish_shape (rasterizer);

    uint32_t hash = ctx_rasterizer_poly_to_edges (rasterizer);
    if (hash){};

#if CTX_SHAPE_CACHE
    int width = (rasterizer->col_max + (CTX_SUBDIV-1) ) / CTX_SUBDIV - rasterizer->col_min/CTX_SUBDIV + 1;
    int height = (rasterizer->scan_max + (CTX_FULL_AA-1) ) / CTX_FULL_AA - rasterizer->scan_min / CTX_FULL_AA + 1;
    if (width * height < CTX_SHAPE_CACHE_DIM && width >=1 && height >= 1
        && width < CTX_SHAPE_CACHE_MAX_DIM
        && height < CTX_SHAPE_CACHE_MAX_DIM 
#if CTX_ENABLE_SHADOW_BLUR
        && !rasterizer->in_shadow
#endif
        )
      {
        int scan_min = rasterizer->scan_min;
        int col_min = rasterizer->col_min;
        scan_min -= (scan_min % CTX_FULL_AA);
        int y0 = scan_min / CTX_FULL_AA;
        int y1 = y0 + height;
        int x0 = col_min / CTX_SUBDIV;
        int ymin = y0;
        int x1 = x0 + width;
        int clip_x_min = rasterizer->blit_x;
        int clip_x_max = rasterizer->blit_x + rasterizer->blit_width - 1;
        int clip_y_min = rasterizer->blit_y;
        int clip_y_max = rasterizer->blit_y + rasterizer->blit_height - 1;

        int dont_cache = 0;
        if (x1 >= clip_x_max)
          { x1 = clip_x_max;
            dont_cache = 1;
          }
        int xo = 0;
        if (x0 < clip_x_min)
          {
            xo = clip_x_min - x0;
            x0 = clip_x_min;
            dont_cache = 1;
          }
        if (y0 < clip_y_min || y1 >= clip_y_max)
          dont_cache = 1;
        if (dont_cache || !_ctx_shape_cache_enabled)
        {
          ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                        , NULL
#endif
                                       );
        }
        else
        {

        rasterizer->scanline = scan_min;
        CtxShapeEntry *shape = ctx_shape_entry_find (rasterizer, hash, width, height); 

        if (shape->uses == 0)
          {
            CtxBuffer *buffer_backup = rasterizer->clip_buffer;
            rasterizer->clip_buffer = NULL;
            ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule, shape);
            rasterizer->clip_buffer = buffer_backup;
          }
        rasterizer->scanline = scan_min;

        int ewidth = x1 - x0;
        if (ewidth>0)
        {
          if (rasterizer->clip_buffer && !rasterizer->clip_rectangle)
          {
          uint8_t composite[ewidth];
          for (int y = y0; y < y1; y++)
            {
              if ( (y >= clip_y_min) && (y <= clip_y_max) )
                {
                    for (int x = 0; x < ewidth; x++)
                    {
                      int val = shape->data[shape->width * (int)(y-ymin) + xo + x];
                      // XXX : not valid for 1bit clip buffers
                      val = (val*((uint8_t*)rasterizer->clip_buffer->data) [
                              ((y-rasterizer->blit_y) * rasterizer->blit_width) + x0 + x])/255;
                      composite[x] = val;
                    }
                    ctx_rasterizer_apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                                 x0, // is 0
                                                 composite,
                                                 ewidth );
               rasterizer->scanline += CTX_FULL_AA;
            }
          }
          }
          else
          for (int y = y0; y < y1; y++)
            {
              if ( (y >= clip_y_min) && (y <= clip_y_max) )
                {
                    ctx_rasterizer_apply_coverage (rasterizer,
                                                 ( (uint8_t *) rasterizer->buf) + (y-rasterizer->blit_y) * rasterizer->blit_stride + (int) (x0) * rasterizer->format->bpp/8,
                                                 x0,
                                                 &shape->data[shape->width * (int) (y-ymin) + xo],
                                                 ewidth );
                }
               rasterizer->scanline += CTX_FULL_AA;
            }
        }
        if (shape->uses != 0)
          {
            ctx_rasterizer_reset (rasterizer);
          }
        }
      }
    else
#endif
    ctx_rasterizer_rasterize_edges (rasterizer, rasterizer->state->gstate.fill_rule
#if CTX_SHAPE_CACHE
                                    , NULL
#endif
                                   );
  }
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
    }
#if CTX_ENABLE_SHADOW_BLUR
  if (rasterizer->in_shadow)
  {
    rasterizer->scan_min -= rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->scan_max -= rasterizer->shadow_y * CTX_FULL_AA;
    rasterizer->col_min  -= (rasterizer->shadow_x - rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
    rasterizer->col_max  -= (rasterizer->shadow_x + rasterizer->state->gstate.shadow_blur * 3 + 1) * CTX_SUBDIV;
  }
#endif
  rasterizer->preserve = 0;
}

#if 0
static void
ctx_rasterizer_triangle (CtxRasterizer *rasterizer,
                         int x0, int y0,
                         int x1, int y1,
                         int x2, int y2,
                         int r0, int g0, int b0, int a0,
                         int r1, int g1, int b1, int a1,
                         int r2, int g2, int b2, int a2,
                         int u0, int v0,
                         int u1, int v1)
{

}
#endif


typedef struct _CtxTermGlyph CtxTermGlyph;

struct _CtxTermGlyph
{
  uint32_t unichar;
  int      col;
  int      row;
  uint8_t  rgba_bg[4];
  uint8_t  rgba_fg[4];
};

static int _ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke);
static void
ctx_rasterizer_glyph (CtxRasterizer *rasterizer, uint32_t unichar, int stroke)
{
  float tx = rasterizer->state->x;
  float ty = rasterizer->state->y - rasterizer->state->gstate.font_size;
  float tx2 = rasterizer->state->x + rasterizer->state->gstate.font_size;
  float ty2 = rasterizer->state->y + rasterizer->state->gstate.font_size;
  _ctx_user_to_device (rasterizer->state, &tx, &ty);
  _ctx_user_to_device (rasterizer->state, &tx2, &ty2);

  if (tx2 < rasterizer->blit_x || ty2 < rasterizer->blit_y) return;
  if (tx  > rasterizer->blit_x + rasterizer->blit_width ||
      ty  > rasterizer->blit_y + rasterizer->blit_height)
          return;

#if CTX_BRAILLE_TEXT
  float font_size = 0;
  int ch = 1;
  int cw = 1;

  if (rasterizer->term_glyphs)
  {
    float tx = 0;
    float ty = rasterizer->state->gstate.font_size;
    float txb = 0;
    float tyb = 0;

    ch = ctx_term_get_cell_height (rasterizer->ctx);
    cw = ctx_term_get_cell_width (rasterizer->ctx);

    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    _ctx_user_to_device (rasterizer->state, &txb, &tyb);
    font_size = ty-tyb;
  }
  if (rasterizer->term_glyphs && !stroke &&
      fabs (font_size - ch) < 0.5)
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = tx / cw + 1;
    int row = ty / ch + 1;
    CtxTermGlyph *glyph = ctx_calloc (sizeof (CtxTermGlyph), 1);
    ctx_list_append (&rasterizer->glyphs, glyph);
    glyph->unichar = unichar;
    glyph->col = col;
    glyph->row = row;
    ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                         &glyph->rgba_fg[0]);
  }
  else
#endif
  _ctx_glyph (rasterizer->ctx, unichar, stroke);
}


static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible);
static void
ctx_rasterizer_text (CtxRasterizer *rasterizer, const char *string, int stroke)
{
#if CTX_BRAILLE_TEXT
  float font_size = 0;
  if (rasterizer->term_glyphs)
  {
  float tx = 0;
  float ty = rasterizer->state->gstate.font_size;
  _ctx_user_to_device (rasterizer->state, &tx, &ty);
  font_size = ty;
  }
  int   ch = ctx_term_get_cell_height (rasterizer->ctx);
  int   cw = ctx_term_get_cell_width (rasterizer->ctx);

  if (rasterizer->term_glyphs && !stroke &&
      fabs (font_size - ch) < 0.5)
  {
    float tx = rasterizer->x;
    float ty = rasterizer->y;
    _ctx_user_to_device (rasterizer->state, &tx, &ty);
    int col = tx / cw + 1;
    int row = ty / ch + 1;
    for (int i = 0; string[i]; i++, col++)
    {
      CtxTermGlyph *glyph = ctx_calloc (sizeof (CtxTermGlyph), 1);
      ctx_list_prepend (&rasterizer->glyphs, glyph);
      glyph->unichar = string[i];
      glyph->col = col;
      glyph->row = row;
      ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color,
                      glyph->rgba_fg);
    }
  }
  else
#endif
  {
    _ctx_text (rasterizer->ctx, string, stroke, 1);
  }
}

void
_ctx_font (Ctx *ctx, const char *name);
static void
ctx_rasterizer_set_font (CtxRasterizer *rasterizer, const char *font_name)
{
  _ctx_font (rasterizer->ctx, font_name);
}

static void
ctx_rasterizer_arc (CtxRasterizer *rasterizer,
                    float        x,
                    float        y,
                    float        radius,
                    float        start_angle,
                    float        end_angle,
                    int          anticlockwise)
{
  int full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS;
  full_segments = radius * CTX_PI * 2;
  if (full_segments > CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS)
    { full_segments = CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS; }
  float step = CTX_PI*2.0/full_segments;
  int steps;

  if (end_angle < -30.0)
    end_angle = -30.0;
  if (start_angle < -30.0)
    start_angle = -30.0;
  if (end_angle > 30.0)
    end_angle = 30.0;
  if (start_angle > 30.0)
    start_angle = 30.0;

  if (radius <= 0.0001)
          return;

  if (end_angle == start_angle)
          // XXX also detect arcs fully outside render view
    {
    if (rasterizer->has_prev!=0)
      ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                              y + ctx_sinf (end_angle) * radius);
      else
      ctx_rasterizer_move_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                            y + ctx_sinf (end_angle) * radius);
      return;
    }
#if 1
  if ( (!anticlockwise && fabsf((end_angle - start_angle) - CTX_PI*2) < 0.01f)  ||
       ( (anticlockwise && fabsf((start_angle - end_angle) - CTX_PI*2) < 0.01f ) ) 
  ||   (anticlockwise && fabsf((end_angle - start_angle) - CTX_PI*2) < 0.01f)  ||  (!anticlockwise && fabsf((start_angle - end_angle) - CTX_PI*2) < 0.01f )  )
    {
      start_angle = start_angle;
      steps = full_segments - 1;
    }
  else
#endif
    {
      steps = (end_angle - start_angle) / (CTX_PI*2) * full_segments;
      if (anticlockwise)
        { steps = full_segments - steps; };
   // if (steps > full_segments)
   //   steps = full_segments;
    }
  if (anticlockwise) { step = step * -1; }
  int first = 1;
  if (steps == 0 /* || steps==full_segments -1  || (anticlockwise && steps == full_segments) */)
    {
      float xv = x + ctx_cosf (start_angle) * radius;
      float yv = y + ctx_sinf (start_angle) * radius;
      if (!rasterizer->has_prev)
        { ctx_rasterizer_move_to (rasterizer, xv, yv); }
      first = 0;
    }
  else
    {
      for (float angle = start_angle, i = 0; i < steps; angle += step, i++)
        {
          float xv = x + ctx_cosf (angle) * radius;
          float yv = y + ctx_sinf (angle) * radius;
          if (first && !rasterizer->has_prev)
            { ctx_rasterizer_move_to (rasterizer, xv, yv); }
          else
            { ctx_rasterizer_line_to (rasterizer, xv, yv); }
          first = 0;
        }
    }
  ctx_rasterizer_line_to (rasterizer, x + ctx_cosf (end_angle) * radius,
                          y + ctx_sinf (end_angle) * radius);
}

static void
ctx_rasterizer_quad_to (CtxRasterizer *rasterizer,
                        float        cx,
                        float        cy,
                        float        x,
                        float        y)
{
  /* XXX : it is probably cheaper/faster to do quad interpolation directly -
   *       though it will increase the code-size, an
   *       alternative is to turn everything into cubic
   *       and deal with cubics more directly during
   *       rasterization
   */
  ctx_rasterizer_curve_to (rasterizer,
                           (cx * 2 + rasterizer->x) / 3.0f, (cy * 2 + rasterizer->y) / 3.0f,
                           (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                           x,                              y);
}

static void
ctx_rasterizer_rel_quad_to (CtxRasterizer *rasterizer,
                            float cx, float cy,
                            float x,  float y)
{
  ctx_rasterizer_quad_to (rasterizer, cx + rasterizer->x, cy + rasterizer->y,
                          x  + rasterizer->x, y  + rasterizer->y);
}

#define LENGTH_OVERSAMPLE 1
static void
ctx_rasterizer_pset (CtxRasterizer *rasterizer, int x, int y, uint8_t cov)
{
  // XXX - we avoid rendering here x==0 - to keep with
  //  an off-by one elsewhere
  //
  //  XXX onlt works in rgba8 formats
  if (x <= 0 || y < 0 || x >= rasterizer->blit_width ||
      y >= rasterizer->blit_height)
    { return; }
  uint8_t fg_color[4];
  ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, fg_color);
  uint8_t pixel[4];
  uint8_t *dst = ( (uint8_t *) rasterizer->buf);
  dst += y * rasterizer->blit_stride;
  dst += x * rasterizer->format->bpp / 8;
  if (!rasterizer->format->to_comp ||
      !rasterizer->format->from_comp)
    { return; }
  if (cov == 255)
    {
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = fg_color[c];
        }
    }
  else
    {
      rasterizer->format->to_comp (rasterizer, x, dst, &pixel[0], 1);
      for (int c = 0; c < 4; c++)
        {
          pixel[c] = ctx_lerp_u8 (pixel[c], fg_color[c], cov);
        }
    }
  rasterizer->format->from_comp (rasterizer, x, &pixel[0], dst, 1);
}

static void
ctx_rasterizer_stroke_1px (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry *temp = rasterizer->edge_list.entries;
  float prev_x = 0.0f;
  float prev_y = 0.0f;
  int aa = 15;//rasterizer->aa;
  int start = 0;
  int end = 0;
#if 0
  float factor = ctx_matrix_get_scale (&state->gstate.transform);
#endif

  while (start < count)
    {
      int started = 0;
      int i;
      for (i = start; i < count; i++)
        {
          CtxEntry *entry = &temp[i];
          float x, y;
          if (entry->code == CTX_NEW_EDGE)
            {
              if (started)
                {
                  end = i - 1;
                  goto foo;
                }
              prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
              prev_y = entry->data.s16[1] * 1.0f / aa;
              started = 1;
              start = i;
            }
          x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
          y = entry->data.s16[3] * 1.0f / aa;
          int dx = x - prev_x;
          int dy = y - prev_y;
          int length = ctx_maxf (abs (dx), abs (dy) );
          if (length)
            {
              length *= LENGTH_OVERSAMPLE;
              int len = length;
              int tx = prev_x * 256;
              int ty = prev_y * 256;
              dx *= 256;
              dy *= 256;
              dx /= length;
              dy /= length;
              for (int i = 0; i < len; i++)
                {
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                  tx += dx;
                  ty += dy;
                  ctx_rasterizer_pset (rasterizer, tx/256, ty/256, 255);
                }
            }
          prev_x = x;
          prev_y = y;
        }
      end = i-1;
foo:
      start = end+1;
    }
  ctx_rasterizer_reset (rasterizer);
}

static void
ctx_rasterizer_stroke (CtxRasterizer *rasterizer)
{
  CtxSource source_backup = rasterizer->state->gstate.source_fill;
  if (rasterizer->state->gstate.source_stroke.type != CTX_SOURCE_INHERIT_FILL)
    rasterizer->state->gstate.source_fill = rasterizer->state->gstate.source_stroke;
  CtxState *state = rasterizer->state;
  int count = rasterizer->edge_list.count;
  int preserved = rasterizer->preserve;
  float factor = ctx_matrix_get_scale (&state->gstate.transform);

  int aa = 15;//rasterizer->aa;
  CtxEntry temp[count]; /* copy of already built up path's poly line  */
  memcpy (temp, rasterizer->edge_list.entries, sizeof (temp) );
#if 1
  if (rasterizer->state->gstate.line_width * factor <= 0.0f &&
      rasterizer->state->gstate.line_width * factor > -10.0f)
    {
      ctx_rasterizer_stroke_1px (rasterizer);
    }
  else
#endif
    {
      factor *= 0.86; /* this hack adjustment makes sharp 1px and 2px strokewidths
                            end up sharp without erronious AA
                       */
      ctx_rasterizer_reset (rasterizer); /* then start afresh with our stroked shape  */
      CtxMatrix transform_backup = rasterizer->state->gstate.transform;
      ctx_matrix_identity (&rasterizer->state->gstate.transform);
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float half_width_x = rasterizer->state->gstate.line_width * factor/2;
      float half_width_y = rasterizer->state->gstate.line_width * factor/2;
      if (rasterizer->state->gstate.line_width <= 0.0f)
        {
          half_width_x = .5;
          half_width_y = .5;
        }
      int start = 0;
      int end   = 0;
      while (start < count)
        {
          int started = 0;
          int i;
          for (i = start; i < count; i++)
            {
              CtxEntry *entry = &temp[i];
              float x, y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  if (started)
                    {
                      end = i - 1;
                      goto foo;
                    }
                  prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  prev_y = entry->data.s16[1] * 1.0f / aa;
                  started = 1;
                  start = i;
                }
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              if (length>0.001f)
                {
                  dx = dx/length * half_width_x;
                  dy = dy/length * half_width_y;
                  if (entry->code == CTX_NEW_EDGE)
                    {
                      ctx_rasterizer_finish_shape (rasterizer);
                      ctx_rasterizer_move_to (rasterizer, prev_x+dy, prev_y-dx);
                    }
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  
                  // we need to know the slope of the other side

                  // XXX possible miter line-to
                  //ctx_rasterizer_line_to (rasterizer, prev_x-dy+4, prev_y+dx+10);
                  //ctx_rasterizer_line_to (rasterizer, prev_x-dy+8, prev_y+dx+0);


                  ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                }
              prev_x = x;
              prev_y = y;
            }
          end = i-1;
foo:
          for (int i = end; i >= start; i--)
            {
              CtxEntry *entry = &temp[i];
              float x, y, dx, dy;
              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              dx = x - prev_x;
              dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);
              dx = dx/length * half_width_x;
              dy = dy/length * half_width_y;
              if (length>0.001f)
                {
                  ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                  // XXX possible miter line-to
             //   ctx_rasterizer_line_to (rasterizer, prev_x-dy+10, prev_y+dx+10);
                  ctx_rasterizer_line_to (rasterizer, x-dy,      y+dx);
                }
              prev_x = x;
              prev_y = y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[1] * 1.0f / aa;
                  dx = x - prev_x;
                  dy = y - prev_y;
                  length = ctx_fast_hypotf (dx, dy);
                  if (length>0.001f)
                    {
                      dx = dx / length * half_width_x;
                      dy = dy / length * half_width_y;
                      ctx_rasterizer_line_to (rasterizer, prev_x-dy, prev_y+dx);
                      ctx_rasterizer_line_to (rasterizer, x-dy, y+dx);
                    }
                }
              if ( (prev_x != x) && (prev_y != y) )
                {
                  prev_x = x;
                  prev_y = y;
                }
            }
          start = end+1;
        }
      ctx_rasterizer_finish_shape (rasterizer);
      switch (rasterizer->state->gstate.line_cap)
        {
          case CTX_CAP_SQUARE: // XXX: incorrect - if rectangles were in
                               //                  reverse order - rotation would be off
                               //                  better implement correct here
            {
              float x = 0, y = 0;
              int has_prev = 0;
              for (int i = 0; i < count; i++)
                {
                  CtxEntry *entry = &temp[i];
                  if (entry->code == CTX_NEW_EDGE)
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x, half_width_y);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / aa;
                      ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  has_prev = 1;
                }
              ctx_rasterizer_rectangle (rasterizer, x - half_width_x, y - half_width_y, half_width_x * 2, half_width_y * 2);
              ctx_rasterizer_finish_shape (rasterizer);
            }
            break;
          case CTX_CAP_NONE: /* nothing to do */
            break;
          case CTX_CAP_ROUND:
            {
              float x = 0, y = 0;
              int has_prev = 0;
              for (int i = 0; i < count; i++)
                {
                  CtxEntry *entry = &temp[i];
                  if (entry->code == CTX_NEW_EDGE)
                    {
                      if (has_prev)
                        {
                          ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                          ctx_rasterizer_finish_shape (rasterizer);
                        }
                      x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                      y = entry->data.s16[1] * 1.0f / aa;
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  has_prev = 1;
                }
              ctx_rasterizer_move_to (rasterizer, x, y);
              ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*3, 0, 1);
              ctx_rasterizer_finish_shape (rasterizer);
              break;
            }
        }
      switch (rasterizer->state->gstate.line_join)
        {
          case CTX_JOIN_BEVEL:
          case CTX_JOIN_MITER:
            break;
          case CTX_JOIN_ROUND:
            {
              float x = 0, y = 0;
              for (int i = 0; i < count-1; i++)
                {
                  CtxEntry *entry = &temp[i];
                  x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
                  y = entry->data.s16[3] * 1.0f / aa;
                  if (entry[1].code == CTX_EDGE)
                    {
                      ctx_rasterizer_arc (rasterizer, x, y, half_width_x, CTX_PI*2, 0, 1);
                      ctx_rasterizer_finish_shape (rasterizer);
                    }
                }
              break;
            }
        }
      CtxFillRule rule_backup = rasterizer->state->gstate.fill_rule;
      rasterizer->state->gstate.fill_rule = CTX_FILL_RULE_WINDING;
      rasterizer->preserve = 0; // so fill isn't tripped
      ctx_rasterizer_fill (rasterizer);
      rasterizer->state->gstate.fill_rule = rule_backup;
      //rasterizer->state->gstate.source = source_backup;
      rasterizer->state->gstate.transform = transform_backup;
    }
  if (preserved)
    {
      memcpy (rasterizer->edge_list.entries, temp, sizeof (temp) );
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
  rasterizer->state->gstate.source_fill = source_backup;
}

#if CTX_1BIT_CLIP
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY1
#else
#define CTX_CLIP_FORMAT CTX_FORMAT_GRAY8
#endif


static void
ctx_rasterizer_clip_reset (CtxRasterizer *rasterizer)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
   ctx_buffer_free (rasterizer->clip_buffer);
  rasterizer->clip_buffer = NULL;
#endif
  rasterizer->state->gstate.clip_min_x = rasterizer->blit_x;
  rasterizer->state->gstate.clip_min_y = rasterizer->blit_y;

  rasterizer->state->gstate.clip_max_x = rasterizer->blit_x + rasterizer->blit_width - 1;
  rasterizer->state->gstate.clip_max_y = rasterizer->blit_y + rasterizer->blit_height - 1;
}

static void
ctx_rasterizer_clip_apply (CtxRasterizer *rasterizer,
                           CtxEntry      *edges)
{
  int count = edges[0].data.u32[0];

  int minx = 5000;
  int miny = 5000;
  int maxx = -5000;
  int maxy = -5000;
  int prev_x = 0;
  int prev_y = 0;
  int blit_width = rasterizer->blit_width;
  int blit_height = rasterizer->blit_height;

  int aa = 15;//rasterizer->aa;
  float coords[6][2];

  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / aa;
          if (prev_x < minx) { minx = prev_x; }
          if (prev_y < miny) { miny = prev_y; }
          if (prev_x > maxx) { maxx = prev_x; }
          if (prev_y > maxy) { maxy = prev_y; }
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / aa;
      if (x < minx) { minx = x; }
      if (y < miny) { miny = y; }
      if (x > maxx) { maxx = x; }
      if (y > maxy) { maxy = y; }

      if (i < 6)
      {
        coords[i][0] = x;
        coords[i][1] = y;
      }
    }

#if CTX_ENABLE_CLIP

  if ((rasterizer->clip_rectangle==1
                          || !rasterizer->clip_buffer)
                  )
  //  XXX  disabled, it makes clip test fail, a lot of unneded work
  //  can be skipped here.
  {
    if (count == 6)
    {
      if (coords[3][0] == coords[5][0] &&
          coords[3][1] == coords[5][1])
      {
#if 0
        printf ("%d,%d %dx%d\n", minx, miny,
                                       maxx-minx+1, maxy-miny+1);
#endif

         rasterizer->state->gstate.clip_min_x =
            ctx_maxi (minx, rasterizer->state->gstate.clip_min_x);
         rasterizer->state->gstate.clip_min_y =
            ctx_maxi (miny, rasterizer->state->gstate.clip_min_y);
         rasterizer->state->gstate.clip_max_x =
            ctx_mini (maxx, rasterizer->state->gstate.clip_max_x);
         rasterizer->state->gstate.clip_max_y =
            ctx_mini (maxy, rasterizer->state->gstate.clip_max_y);

         rasterizer->clip_rectangle = 1;

#if 0
         if (!rasterizer->clip_buffer)
           rasterizer->clip_buffer = ctx_buffer_new (blit_width,
                                                     blit_height,
                                                     CTX_CLIP_FORMAT);

         memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height);
         int i = 0;
         for (int y = rasterizer->state->gstate.clip_min_y;
                  y <= rasterizer->state->gstate.clip_max_y;
                  y++)
         for (int x = rasterizer->state->gstate.clip_min_x;
                  x <= rasterizer->state->gstate.clip_max_x;
                  x++, i++)
         {
           ((uint8_t*)(rasterizer->clip_buffer->data))[i] = 255;
         }
#endif

         return;
      }
    }
  }
  rasterizer->clip_rectangle = 0;

  if ((minx == maxx) || (miny == maxy)) // XXX : reset hack
  {
    ctx_rasterizer_clip_reset (rasterizer);
    return;//goto done;
  }

  int we_made_it = 0;
  CtxBuffer *clip_buffer;

  if (!rasterizer->clip_buffer)
  {
    rasterizer->clip_buffer = ctx_buffer_new (blit_width,
                                              blit_height,
                                              CTX_CLIP_FORMAT);
    clip_buffer = rasterizer->clip_buffer;
    we_made_it = 1;
    if (CTX_CLIP_FORMAT == CTX_FORMAT_GRAY1)
    memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height/8);
    else
    memset (rasterizer->clip_buffer->data, 0, blit_width * blit_height);
  }
  else
  {
    clip_buffer = ctx_buffer_new (blit_width, blit_height,
                                  CTX_CLIP_FORMAT);
  }

  {

  int prev_x = 0;
  int prev_y = 0;

    Ctx *ctx = ctx_new_for_framebuffer (clip_buffer->data, blit_width, blit_height,
       blit_width,
       CTX_CLIP_FORMAT);

  for (int i = 0; i < count; i++)
    {
      CtxEntry *entry = &edges[i+1];
      float x, y;
      if (entry->code == CTX_NEW_EDGE)
        {
          prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
          prev_y = entry->data.s16[1] * 1.0f / aa;
          ctx_move_to (ctx, prev_x, prev_y);
        }
      x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
      y = entry->data.s16[3] * 1.0f / aa;
      ctx_line_to (ctx, x, y);
    }
    ctx_gray (ctx, 1.0f);
    ctx_fill (ctx);
    ctx_free (ctx);
  }

  int maybe_rect = 1;
  rasterizer->clip_rectangle = 0;

  if (CTX_CLIP_FORMAT == CTX_FORMAT_GRAY1)
  {
    int count = blit_width * blit_height / 8;
    for (int i = 0; i < count; i++)
    {
      ((uint8_t*)rasterizer->clip_buffer->data)[i] =
      (((uint8_t*)rasterizer->clip_buffer->data)[i] &
      ((uint8_t*)clip_buffer->data)[i]);
    }
  }
  else
  {
    int count = blit_width * blit_height;


    int i;
    int x0 = 0;
    int y0 = 0;
    int width = -1;
    int next_stage = 0;
    uint8_t *p_data = (uint8_t*)rasterizer->clip_buffer->data;
    uint8_t *data = (uint8_t*)clip_buffer->data;

    i=0;
    /* find upper left */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;
      switch (val)
      {
        case 255:
          x0 = i % blit_width;
          y0 = i / blit_width;
          next_stage = 1;
          break;
        case 0: break;
        default:
          maybe_rect = 0;
          break;
      }
    }

    next_stage = 0;
    /* figure out with */
    for (; i < count && !next_stage && maybe_rect; i++)
    {
      int x = i % blit_width;
      int y = i / blit_width;
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (y == y0)
      {
        switch (val)
        {
          case 255:
            width = x - x0 + 1;
            break;
          case 0:
            next_stage = 1;
            break;
          default:
            maybe_rect = 0;
            break;
        }
        if (x % blit_width == blit_width - 1) next_stage = 1;
      }
      else next_stage = 1;
    }

    next_stage = 0;
    /* body */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      int x = i % blit_width;
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (x < x0)
      {
        if (val != 0){ maybe_rect = 0; next_stage = 1; }
      } else if (x < x0 + width)
      {
        if (val != 255){ if (val != 0) maybe_rect = 0; next_stage = 1; }
      } else {
        if (val != 0){ maybe_rect = 0; next_stage = 1; }
      }
    }

    next_stage = 0;
    /* foot */
    for (; i < count && maybe_rect && !next_stage; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;

      if (val != 0){ maybe_rect = 0; next_stage = 1; }
    }


    for (; i < count; i++)
    {
      uint8_t val = (p_data[i] * data[i])/255;
      data[i] = val;
    }

    if (maybe_rect)
       rasterizer->clip_rectangle = 1;
  }
  if (!we_made_it)
   ctx_buffer_free (clip_buffer);
#else
  if (coords[0][0]){};
#endif
  
  rasterizer->state->gstate.clip_min_x = ctx_maxi (minx,
                                         rasterizer->state->gstate.clip_min_x);
  rasterizer->state->gstate.clip_min_y = ctx_maxi (miny,
                                         rasterizer->state->gstate.clip_min_y);
  rasterizer->state->gstate.clip_max_x = ctx_mini (maxx,
                                         rasterizer->state->gstate.clip_max_x);
  rasterizer->state->gstate.clip_max_y = ctx_mini (maxy,
                                         rasterizer->state->gstate.clip_max_y);
}

static void
ctx_rasterizer_clip (CtxRasterizer *rasterizer)
{
  int count = rasterizer->edge_list.count;
  CtxEntry temp[count+1]; /* copy of already built up path's poly line  */
  rasterizer->state->has_clipped=1;
  rasterizer->state->gstate.clipped=1;
  //if (rasterizer->preserve)
    { memcpy (temp + 1, rasterizer->edge_list.entries, sizeof (temp) - sizeof (temp[0]));
      temp[0].code = CTX_NOP;
      temp[0].data.u32[0] = count;
      ctx_state_set_blob (rasterizer->state, CTX_clip, (uint8_t*)temp, sizeof(temp));
    }
  ctx_rasterizer_clip_apply (rasterizer, temp);
  ctx_rasterizer_reset (rasterizer);
  if (rasterizer->preserve)
    {
      memcpy (rasterizer->edge_list.entries, temp + 1, sizeof (temp) - sizeof(temp[0]));
      rasterizer->edge_list.count = count;
      rasterizer->preserve = 0;
    }
}


#if 0
static void
ctx_rasterizer_load_image (CtxRasterizer *rasterizer,
                           const char  *path,
                           float x,
                           float y)
{
  // decode PNG, put it in image is slot 1,
  // magic width height stride format data
  ctx_buffer_load_png (&rasterizer->ctx->texture[0], path);
  ctx_rasterizer_set_texture (rasterizer, 0, x, y);
}
#endif


CTX_INLINE void
ctx_rasterizer_rectangle (CtxRasterizer *rasterizer,
                          float x,
                          float y,
                          float width,
                          float height)
{
  ctx_rasterizer_move_to (rasterizer, x, y);
  ctx_rasterizer_rel_line_to (rasterizer, width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, height);
  ctx_rasterizer_rel_line_to (rasterizer, -width, 0);
  ctx_rasterizer_rel_line_to (rasterizer, 0, -height);
  ctx_rasterizer_rel_line_to (rasterizer, width/2, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_set_pixel (CtxRasterizer *rasterizer,
                          uint16_t x,
                          uint16_t y,
                          uint8_t r,
                          uint8_t g,
                          uint8_t b,
                          uint8_t a)
{
  rasterizer->state->gstate.source_fill.type = CTX_SOURCE_COLOR;
  ctx_color_set_RGBA8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, r, g, b, a);
#if 0
  // XXX : doesn't take transforms into account - and has
  // received less testing than code paths part of protocol,
  // using rectangle properly will trigger the fillrect fastpath
  ctx_rasterizer_pset (rasterizer, x, y, 255);
#else
  ctx_rasterizer_rectangle (rasterizer, x, y, 1.0, 1.0);
  ctx_rasterizer_fill (rasterizer);
#endif
}

#if CTX_ENABLE_SHADOW_BLUR
static float
ctx_gaussian (float x, float mu, float sigma)
{
  float a = ( x- mu) / sigma;
  return ctx_expf (-0.5 * a * a);
}

static void
ctx_compute_gaussian_kernel (int dim, float radius, float *kernel)
{
  float sigma = radius / 2;
  float sum = 0.0;
  int i = 0;
  //for (int row = 0; row < dim; row ++)
    for (int col = 0; col < dim; col ++, i++)
    {
      float val = //ctx_gaussian (row, radius, sigma) *
                            ctx_gaussian (col, radius, sigma);
      kernel[i] = val;
      sum += val;
    }
  i = 0;
  //for (int row = 0; row < dim; row ++)
    for (int col = 0; col < dim; col ++, i++)
        kernel[i] /= sum;
}
#endif

static void
ctx_rasterizer_round_rectangle (CtxRasterizer *rasterizer, float x, float y, float width, float height, float corner_radius)
{
  float aspect  = 1.0f;
  float radius  = corner_radius / aspect;
  float degrees = CTX_PI / 180.0f;

  if (radius > width/2) radius = width/2;
  if (radius > height/2) radius = height/2;

  ctx_rasterizer_finish_shape (rasterizer);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees, 0);
  ctx_rasterizer_arc (rasterizer, x + radius, y + radius, radius, 180 * degrees, 270 * degrees, 0);
  ctx_rasterizer_finish_shape (rasterizer);
}

static void
ctx_rasterizer_process (void *user_data, CtxCommand *command);

int
_ctx_is_rasterizer (Ctx *ctx)
{
  if (ctx->renderer && ctx->renderer->process == ctx_rasterizer_process)
    return 1;
  return 0;
}

#if CTX_COMPOSITING_GROUPS
static void
ctx_rasterizer_start_group (CtxRasterizer *rasterizer)
{
  CtxEntry save_command = ctx_void(CTX_SAVE);
  // allocate buffer, and set it as temporary target
  int no;
  if (rasterizer->group[0] == NULL) // first group
  {
    rasterizer->saved_buf = rasterizer->buf;
  }
  for (no = 0; rasterizer->group[no] && no < CTX_GROUP_MAX; no++);

  if (no >= CTX_GROUP_MAX)
     return;
  rasterizer->group[no] = ctx_buffer_new (rasterizer->blit_width,
                                          rasterizer->blit_height,
                                          rasterizer->format->composite_format);
  rasterizer->buf = rasterizer->group[no]->data;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
}

static void
ctx_rasterizer_end_group (CtxRasterizer *rasterizer)
{
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  CtxEntry save_command = ctx_void(CTX_SAVE);
  int no = 0;
  for (no = 0; rasterizer->group[no] && no < CTX_GROUP_MAX; no++);
  no--;

  if (no < 0)
    return;

  CtxCompositingMode comp = rasterizer->state->gstate.compositing_mode;
  CtxBlend blend = rasterizer->state->gstate.blend_mode;
  float global_alpha = rasterizer->state->gstate.global_alpha_f;
  // fetch compositing, blending, global alpha
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
  CtxEntry set_state[3]=
  {
    ctx_u8 (CTX_COMPOSITING_MODE, comp,  0,0,0,0,0,0,0),
    ctx_u8 (CTX_BLEND_MODE,       blend, 0,0,0,0,0,0,0),
    ctx_f  (CTX_GLOBAL_ALPHA,     global_alpha, 0.0)
  };
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[0]);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[1]);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_state[2]);
  if (no == 0)
  {
    rasterizer->buf = rasterizer->saved_buf;
  }
  else
  {
    rasterizer->buf = rasterizer->group[no-1]->data;
  }
  // XXX use texture_source ?
   ctx_texture_init (rasterizer->ctx, ".ctx-group", // XXX ? count groups..
                  rasterizer->blit_width,  // or have group based on thread-id?
                  rasterizer->blit_height, // .. this would mean threadsafe
                                           // allocation
                  rasterizer->blit_width * rasterizer->format->bpp/8,
                  rasterizer->format->pixel_format,
                  NULL, // space
                  (uint8_t*)rasterizer->group[no]->data,
                  NULL, NULL);
  {
     char *eid = ".ctx-group";
     int   eid_len = strlen (eid);

     CtxEntry commands[4] =
      {
       ctx_f  (CTX_TEXTURE, rasterizer->blit_x, rasterizer->blit_y), 
       ctx_u32 (CTX_DATA, eid_len, eid_len/9+1),
       ctx_u32 (CTX_CONT, 0,0),
       ctx_u32 (CTX_CONT, 0,0)
      };
     memcpy( (char *) &commands[2].data.u8[0], eid, eid_len);
     ( (char *) (&commands[2].data.u8[0]) ) [eid_len]=0;

     ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[2]=
    {
      ctx_f (CTX_RECTANGLE, rasterizer->blit_x, rasterizer->blit_y),
      ctx_f (CTX_CONT,      rasterizer->blit_width, rasterizer->blit_height)
    };
    ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  {
    CtxEntry commands[1]= { ctx_void (CTX_FILL) };
    ctx_rasterizer_process (rasterizer, (CtxCommand*)commands);
  }
  //ctx_texture_release (rasterizer->ctx, ".ctx-group");
  ctx_buffer_free (rasterizer->group[no]);
  rasterizer->group[no] = 0;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}
#endif

#if CTX_ENABLE_SHADOW_BLUR
static void
ctx_rasterizer_shadow_stroke (CtxRasterizer *rasterizer)
{
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);
  {
    int i = 0;
    for (int v = 0; v < dim; v += 1, i++)
      {
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command[0]);
#if CTX_ENABLE_SHADOW_BLUR
        rasterizer->in_shadow = 1;
#endif
        rasterizer->shadow_x = rasterizer->state->gstate.shadow_offset_x;
        rasterizer->shadow_y = dy;
        rasterizer->preserve = 1;
        ctx_rasterizer_stroke (rasterizer);
#if CTX_ENABLE_SHADOW_BLUR
        rasterizer->in_shadow = 0;
#endif
      }
  }
  //free (kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}

static void
ctx_rasterizer_shadow_text (CtxRasterizer *rasterizer, const char *str)
{
  float x = rasterizer->state->x;
  float y = rasterizer->state->y;
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry move_to_command [1]=
  {
    ctx_f (CTX_MOVE_TO, x, y),
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);

  {
      {
        move_to_command[0].data.f[0] = x;
        move_to_command[0].data.f[1] = y;
        set_color_command[2].data.f[0] = rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command);
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&move_to_command);
        rasterizer->in_shadow=1;
        ctx_rasterizer_text (rasterizer, str, 0);
        rasterizer->in_shadow=0;
      }
  }
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
  move_to_command[0].data.f[0] = x;
  move_to_command[0].data.f[1] = y;
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&move_to_command);
}

static void
ctx_rasterizer_shadow_fill (CtxRasterizer *rasterizer)
{
  CtxColor color;
  CtxEntry save_command = ctx_void(CTX_SAVE);

  float rgba[4] = {0, 0, 0, 1.0};
  if (ctx_get_color (rasterizer->ctx, CTX_shadowColor, &color) == 0)
    ctx_color_get_rgba (rasterizer->state, &color, rgba);

  CtxEntry set_color_command [3]=
  {
    ctx_f (CTX_COLOR, CTX_RGBA, rgba[0]),
    ctx_f (CTX_CONT, rgba[1], rgba[2]),
    ctx_f (CTX_CONT, rgba[3], 0)
  };
  CtxEntry restore_command = ctx_void(CTX_RESTORE);
  float radius = rasterizer->state->gstate.shadow_blur;
  int dim = 2 * radius + 1;
  if (dim > CTX_MAX_GAUSSIAN_KERNEL_DIM)
    dim = CTX_MAX_GAUSSIAN_KERNEL_DIM;
  ctx_compute_gaussian_kernel (dim, radius, rasterizer->kernel);
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&save_command);

  {
    for (int v = 0; v < dim; v ++)
      {
        int i = v;
        float dy = rasterizer->state->gstate.shadow_offset_y + v - dim/2;
        set_color_command[2].data.f[0] = rasterizer->kernel[i] * rgba[3];
        ctx_rasterizer_process (rasterizer, (CtxCommand*)&set_color_command);
        rasterizer->in_shadow = 1;
        rasterizer->shadow_x = rasterizer->state->gstate.shadow_offset_x;
        rasterizer->shadow_y = dy;
        rasterizer->preserve = 1;
        ctx_rasterizer_fill (rasterizer);
        rasterizer->in_shadow = 0;
      }
  }
  ctx_rasterizer_process (rasterizer, (CtxCommand*)&restore_command);
}
#endif

static void
ctx_rasterizer_line_dash (CtxRasterizer *rasterizer, int count, float *dashes)
{
  if (!dashes)
  {
    rasterizer->state->gstate.n_dashes = 0;
    return;
  }
  count = CTX_MIN(count, CTX_PARSER_MAX_ARGS-1);
  rasterizer->state->gstate.n_dashes = count;
  memcpy(&rasterizer->state->gstate.dashes[0], dashes, count * sizeof(float));
  for (int i = 0; i < count; i ++)
  {
    if (rasterizer->state->gstate.dashes[i] < 0.0001f)
      rasterizer->state->gstate.dashes[i] = 0.0001f; // hang protection
  }
}


static void
ctx_rasterizer_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  int clear_clip = 0;
  ctx_interpret_style (rasterizer->state, entry, NULL);
  switch (c->code)
    {
#if CTX_ENABLE_SHADOW_BLUR
      case CTX_SHADOW_COLOR:
        {
          CtxColor  col;
          CtxColor *color = &col;
          //state->gstate.source_fill.type = CTX_SOURCE_COLOR;
          switch ((int)c->rgba.model)
            {
              case CTX_RGB:
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
                break;
              case CTX_RGBA:
                //ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                ctx_color_set_rgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
              case CTX_DRGBA:
                ctx_color_set_drgba (state, color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYKA:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_CMYK:
                ctx_color_set_cmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
              case CTX_DCMYKA:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_DCMYK:
                ctx_color_set_dcmyka (state, color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
#endif
              case CTX_GRAYA:
                ctx_color_set_graya (state, color, c->graya.g, c->graya.a);
                break;
              case CTX_GRAY:
                ctx_color_set_graya (state, color, c->graya.g, 1.0f);
                break;
            }
          ctx_set_color (rasterizer->ctx, CTX_shadowColor, color);
        }
        break;
#endif
      case CTX_LINE_DASH:
        if (c->line_dash.count)
          {
            ctx_rasterizer_line_dash (rasterizer, c->line_dash.count, c->line_dash.data);
          }
        else
        ctx_rasterizer_line_dash (rasterizer, 0, NULL);
        break;

      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_DEFINE_TEXTURE:
        {
          uint8_t *pixel_data = ctx_define_texture_pixel_data (entry);
          ctx_rasterizer_define_texture (rasterizer, c->define_texture.eid,
                                         c->define_texture.width, c->define_texture.height,
                                         c->define_texture.format,
                                         pixel_data);
          rasterizer->comp_op = NULL;
        }
        break;
      case CTX_TEXTURE:
        ctx_rasterizer_set_texture (rasterizer, c->texture.eid,
                                    c->texture.x, c->texture.y);
        rasterizer->comp_op = NULL;
        break;
#if 0
      case CTX_LOAD_IMAGE:
        ctx_rasterizer_load_image (rasterizer, ctx_arg_string(),
                                   ctx_arg_float (0), ctx_arg_float (1) );
        break;
#endif
#if CTX_GRADIENTS
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_rasterizer_gradient_add_stop (rasterizer,
                                            ctx_arg_float (0), rgba);
          rasterizer->comp_op = NULL;
        }
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        rasterizer->comp_op = NULL;
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_state_gradient_clear_stops (rasterizer->state);
        rasterizer->comp_op = NULL;
        break;
#endif
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_COLOR:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
        rasterizer->comp_op = NULL;
        break;
#if CTX_COMPOSITING_GROUPS
      case CTX_START_GROUP:
        ctx_rasterizer_start_group (rasterizer);
        break;
      case CTX_END_GROUP:
        ctx_rasterizer_end_group (rasterizer);
        break;
#endif

      case CTX_RESTORE:
        for (int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == CTX_clip)
          {
            clear_clip = 1;
          }
        }
        /* FALLTHROUGH */
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_IDENTITY:
      case CTX_SAVE:
        rasterizer->comp_op = NULL;
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);
        if (clear_clip)
        {
          ctx_rasterizer_clip_reset (rasterizer);
        for (int i = state->gstate_no?state->gstate_stack[state->gstate_no-1].keydb_pos:0;
             i < state->gstate.keydb_pos; i++)
        {
          if (state->keydb[i].key == CTX_clip)
          {
            int idx = ctx_float_to_string_index (state->keydb[i].value);
            if (idx >=0)
            {
              CtxEntry *edges = (CtxEntry*)&state->stringpool[idx];
              ctx_rasterizer_clip_apply (rasterizer, edges);
            }
          }
        }
        }
        break;
      case CTX_STROKE:
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_stroke (rasterizer);
#endif
        if (rasterizer->state->gstate.n_dashes)
        {
          int n_dashes = rasterizer->state->gstate.n_dashes;
          float *dashes = rasterizer->state->gstate.dashes;
          float factor = ctx_matrix_get_scale (&state->gstate.transform);

          int count = rasterizer->edge_list.count;
          int aa = 15;//rasterizer->aa;
          CtxEntry temp[count]; /* copy of already built up path's poly line  */
          memcpy (temp, rasterizer->edge_list.entries, sizeof (temp));
          int start = 0;
          int end   = 0;
      CtxMatrix transform_backup = rasterizer->state->gstate.transform;
      ctx_matrix_identity (&rasterizer->state->gstate.transform);
      ctx_rasterizer_reset (rasterizer); /* for dashing we create
                                            a dashed path to stroke */
      float prev_x = 0.0f;
      float prev_y = 0.0f;
      float pos = 0.0;

      int   dash_no  = 0.0;
      float dash_lpos = rasterizer->state->gstate.line_dash_offset * factor;
      int   is_down = 0;

          while (start < count)
          {
            int started = 0;
            int i;
            is_down = 0;

            if (!is_down)
            {
              CtxEntry *entry = &temp[0];
              prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
              prev_y = entry->data.s16[1] * 1.0f / aa;
              ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
              is_down = 1;
            }


            for (i = start; i < count; i++)
            {
              CtxEntry *entry = &temp[i];
              float x, y;
              if (entry->code == CTX_NEW_EDGE)
                {
                  if (started)
                    {
                      end = i - 1;
                      dash_no = 0;
                      dash_lpos = 0.0;
                      goto foo;
                    }
                  prev_x = entry->data.s16[0] * 1.0f / CTX_SUBDIV;
                  prev_y = entry->data.s16[1] * 1.0f / aa;
                  started = 1;
                  start = i;
                  is_down = 1;
                  ctx_rasterizer_move_to (rasterizer, prev_x, prev_y);
                }

again:

              x = entry->data.s16[2] * 1.0f / CTX_SUBDIV;
              y = entry->data.s16[3] * 1.0f / aa;
              float dx = x - prev_x;
              float dy = y - prev_y;
              float length = ctx_fast_hypotf (dx, dy);


              if (dash_lpos + length >= dashes[dash_no] * factor)
              {
                float p = (dashes[dash_no] * factor - dash_lpos) / length;
                float splitx = x * p + (1.0f - p) * prev_x;
                float splity = y * p + (1.0f - p) * prev_y;
                if (is_down)
                {
                  ctx_rasterizer_line_to (rasterizer, splitx, splity);
                  is_down = 0;
                }
                else
                {
                  ctx_rasterizer_move_to (rasterizer, splitx, splity);
                  is_down = 1;
                }
                prev_x = splitx;
                prev_y = splity;
                dash_no++;
                dash_lpos=0;
                if (dash_no >= n_dashes) dash_no = 0;
                goto again;
              }
              else
              {
                pos += length;
                dash_lpos += length;
                {
                  if (is_down)
                    ctx_rasterizer_line_to (rasterizer, x, y);
                }
              }
              prev_x = x;
              prev_y = y;
            }
          end = i-1;
foo:
          start = end+1;
        }
      rasterizer->state->gstate.transform = transform_backup;
        }

        ctx_rasterizer_stroke (rasterizer);
        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_TEXT:
        rasterizer->in_text++;
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0)
          ctx_rasterizer_shadow_text (rasterizer, ctx_arg_string ());
#endif
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 0);
        rasterizer->in_text--;
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_STROKE_TEXT:
        ctx_rasterizer_text (rasterizer, ctx_arg_string(), 1);
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
        ctx_rasterizer_glyph (rasterizer, entry[0].data.u32[0], entry[0].data.u8[4]);
        break;
      case CTX_FILL:
#if CTX_ENABLE_SHADOW_BLUR
        if (rasterizer->state->gstate.shadow_blur > 0.0 &&
            !rasterizer->in_text)
          ctx_rasterizer_shadow_fill (rasterizer);
#endif
        ctx_rasterizer_fill (rasterizer);
        break;
      case CTX_RESET:
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
    }
  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
}

void
ctx_rasterizer_deinit (CtxRasterizer *rasterizer)
{
  ctx_drawlist_deinit (&rasterizer->edge_list);
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
  {
    ctx_buffer_free (rasterizer->clip_buffer);
    rasterizer->clip_buffer = NULL;
  }
#endif
#if CTX_SHAPE_CACHE
  for (int i = 0; i < CTX_SHAPE_CACHE_ENTRIES; i ++)
    if (rasterizer->shape_cache.entries[i])
    {
      free (rasterizer->shape_cache.entries[i]);
      rasterizer->shape_cache.entries[i] = NULL;
    }

#endif
  free (rasterizer);
}


CtxAntialias ctx_get_antialias (Ctx *ctx)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->renderer);
     return fb->antialias;
  }
#endif
  if (!_ctx_is_rasterizer (ctx)) return CTX_ANTIALIAS_DEFAULT;

  switch (((CtxRasterizer*)(ctx->renderer))->aa)
  {
    case 1: return CTX_ANTIALIAS_NONE;
    case 3: return CTX_ANTIALIAS_FAST;
    case 5: return CTX_ANTIALIAS_GOOD;
    default:
    case 15: return CTX_ANTIALIAS_DEFAULT;
    case 17: return CTX_ANTIALIAS_BEST;
  }
}

int _ctx_antialias_to_aa (CtxAntialias antialias)
{
  switch (antialias)
  {
    case CTX_ANTIALIAS_NONE: return 1;
    case CTX_ANTIALIAS_FAST: return 3;
    case CTX_ANTIALIAS_GOOD: return 5;
    default:
    case CTX_ANTIALIAS_DEFAULT: return CTX_RASTERIZER_AA;
    case CTX_ANTIALIAS_BEST: return 17;
  }
}

void
ctx_set_antialias (Ctx *ctx, CtxAntialias antialias)
{
#if CTX_EVENTS
  if (ctx_renderer_is_sdl (ctx) || ctx_renderer_is_fb (ctx))
  {
     CtxTiled *fb = (CtxTiled*)(ctx->renderer);
     fb->antialias = antialias;
     for (int i = 0; i < _ctx_max_threads; i++)
     {
       ctx_set_antialias (fb->host[i], antialias);
     }
     return;
  }
#endif
  if (!_ctx_is_rasterizer (ctx)) return;

  ((CtxRasterizer*)(ctx->renderer))->aa = 
     _ctx_antialias_to_aa (antialias);
/* vertical level of supersampling at full/forced AA.
 *
 * 1 is none, 3 is fast 5 is good 15 or 17 is best for 8bit
 *
 * valid values:  - for other values we do not add up to 255
 * 3 5 15 17 51
 *
 */
}

CtxRasterizer *
ctx_rasterizer_init (CtxRasterizer *rasterizer, Ctx *ctx, Ctx *texture_source, CtxState *state, void *data, int x, int y, int width, int height, int stride, CtxPixelFormat pixel_format, CtxAntialias antialias)
{
#if CTX_ENABLE_CLIP
  if (rasterizer->clip_buffer)
    ctx_buffer_free (rasterizer->clip_buffer);
#endif
  if (rasterizer->edge_list.size)
    ctx_drawlist_deinit (&rasterizer->edge_list);

  ctx_memset (rasterizer, 0, sizeof (CtxRasterizer) );
  rasterizer->vfuncs.process = ctx_rasterizer_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  rasterizer->texture_source = texture_source?texture_source:ctx;
  rasterizer->aa          = _ctx_antialias_to_aa (antialias);
  ctx_state_init (rasterizer->state);
  rasterizer->buf         = data;
  rasterizer->blit_x      = x;
  rasterizer->blit_y      = y;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = x;
  rasterizer->state->gstate.clip_min_y  = y;
  rasterizer->state->gstate.clip_max_x  = x + width - 1;
  rasterizer->state->gstate.clip_max_y  = y + height - 1;
  rasterizer->blit_stride = stride;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;
  rasterizer->format = ctx_pixel_format_info (pixel_format);

  return rasterizer;
}

Ctx *
ctx_new_for_buffer (CtxBuffer *buffer)
{
  Ctx *ctx = ctx_new ();
  ctx_set_renderer (ctx,
                    ctx_rasterizer_init ( (CtxRasterizer *) malloc (sizeof (CtxRasterizer) ),
                                          ctx, NULL, &ctx->state,
                                          buffer->data, 0, 0, buffer->width, buffer->height,
                                          buffer->stride, buffer->format->pixel_format,
                                          CTX_ANTIALIAS_DEFAULT));
  return ctx;
}

Ctx *
ctx_new_for_framebuffer (void *data, int width, int height,
                         int stride,
                         CtxPixelFormat pixel_format)
{
  Ctx *ctx = ctx_new ();
  CtxRasterizer *r = ctx_rasterizer_init ( (CtxRasterizer *) ctx_calloc (sizeof (CtxRasterizer), 1),
                                          ctx, NULL, &ctx->state, data, 0, 0, width, height,
                                          stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
  ctx_set_renderer (ctx, r);
  return ctx;
}

// ctx_new_for_stream (FILE *stream);

#if 0
CtxRasterizer *ctx_rasterizer_new (void *data, int x, int y, int width, int height,
                                   int stride, CtxPixelFormat pixel_format)
{
  CtxState    *state    = (CtxState *) malloc (sizeof (CtxState) );
  CtxRasterizer *rasterizer = (CtxRasterizer *) malloc (sizeof (CtxRenderer) );
  ctx_rasterizer_init (rasterizer, state, data, x, y, width, height,
                       stride, pixel_format, CTX_ANTIALIAS_DEFAULT);
}
#endif

CtxPixelFormatInfo *ctx_pixel_formats = NULL;

extern CtxPixelFormatInfo ctx_pixel_formats_default[];


CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  if (!ctx_pixel_formats)
  {
    ctx_pixel_formats = ctx_pixel_formats_default;

  }

  for (unsigned int i = 0; ctx_pixel_formats[i].pixel_format; i++)
    {
      if (ctx_pixel_formats[i].pixel_format == format)
        {
          return &ctx_pixel_formats[i];
        }
    }
  return NULL;
}
#else

CtxPixelFormatInfo *
ctx_pixel_format_info (CtxPixelFormat format)
{
  return NULL;
}
#endif

void
ctx_current_point (Ctx *ctx, float *x, float *y)
{
  if (!ctx)
    { 
      if (x) { *x = 0.0f; }
      if (y) { *y = 0.0f; }
    }
#if CTX_RASTERIZER
  if (ctx->renderer)
    {
      if (x) { *x = ( (CtxRasterizer *) (ctx->renderer) )->x; }
      if (y) { *y = ( (CtxRasterizer *) (ctx->renderer) )->y; }
      return;
    }
#endif
  if (x) { *x = ctx->state.x; }
  if (y) { *y = ctx->state.y; }
}

float ctx_x (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return x;
}

float ctx_y (Ctx *ctx)
{
  float x = 0, y = 0;
  ctx_current_point (ctx, &x, &y);
  return y;
}

void
ctx_process (Ctx *ctx, CtxEntry *entry)
{
#if CTX_CURRENT_PATH
  switch (entry->code)
    {
      case CTX_TEXT:
      case CTX_STROKE_TEXT:
      case CTX_BEGIN_PATH:
        ctx->current_path.count = 0;
        break;
      case CTX_CLIP:
      case CTX_FILL:
      case CTX_STROKE:
              // XXX unless preserve
        ctx->current_path.count = 0;
        break;
      case CTX_CLOSE_PATH:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_QUAD_TO:
      case CTX_SMOOTH_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_QUAD_TO:
      case CTX_REL_SMOOTH_TO:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE:
        ctx_drawlist_add_entry (&ctx->current_path, entry);
        break;
      default:
        break;
    }
#endif
  if (ctx->renderer && ctx->renderer->process)
    {
      ctx->renderer->process (ctx->renderer, (CtxCommand *) entry);
    }
  else
    {
      /* these functions might alter the code and coordinates of
         command that in the end gets added to the drawlist
       */
      ctx_interpret_style (&ctx->state, entry, ctx);
      ctx_interpret_transforms (&ctx->state, entry, ctx);
      ctx_interpret_pos (&ctx->state, entry, ctx);
      ctx_drawlist_add_entry (&ctx->drawlist, entry);
    }
}


int ctx_gradient_cache_valid = 0;

void
ctx_state_gradient_clear_stops (CtxState *state)
{
//#if CTX_GRADIENT_CACHE
//  ctx_gradient_cache_reset ();
//#endif
  ctx_gradient_cache_valid = 0;
  state->gradient.n_stops = 0;
}

uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
uint8_t ctx_gradient_cache_u8_a[CTX_GRADIENT_CACHE_ELEMENTS][4];

/****  end of engine ****/

CtxBuffer *ctx_buffer_new_bare (void)
{
  CtxBuffer *buffer = (CtxBuffer *) ctx_calloc (sizeof (CtxBuffer), 1);
  return buffer;
}

void ctx_buffer_set_data (CtxBuffer *buffer,
                          void *data, int width, int height,
                          int stride,
                          CtxPixelFormat pixel_format,
                          void (*freefunc) (void *pixels, void *user_data),
                          void *user_data)
{
  if (buffer->free_func)
    { buffer->free_func (buffer->data, buffer->user_data); }
  if (stride <= 0)
    stride = ctx_pixel_format_get_stride (pixel_format, width);
  buffer->data      = data;
  buffer->width     = width;
  buffer->height    = height;
  buffer->stride    = stride;
  buffer->format    = ctx_pixel_format_info (pixel_format);
  buffer->free_func = freefunc;
  buffer->user_data = user_data;
}

CtxBuffer *ctx_buffer_new_for_data (void *data, int width, int height,
                                    int stride,
                                    CtxPixelFormat pixel_format,
                                    void (*freefunc) (void *pixels, void *user_data),
                                    void *user_data)
{
  CtxBuffer *buffer = ctx_buffer_new_bare ();
  ctx_buffer_set_data (buffer, data, width, height, stride, pixel_format,
                       freefunc, user_data);
  return buffer;
}

void ctx_buffer_pixels_free (void *pixels, void *userdata)
{
  free (pixels);
}

CtxBuffer *ctx_buffer_new (int width, int height,
                           CtxPixelFormat pixel_format)
{
  //CtxPixelFormatInfo *info = ctx_pixel_format_info (pixel_format);
  CtxBuffer *buffer = ctx_buffer_new_bare ();
  int stride = ctx_pixel_format_get_stride (pixel_format, width);
  uint8_t *pixels = (uint8_t*)ctx_calloc (stride, height + 1);

  ctx_buffer_set_data (buffer, pixels, width, height, stride, pixel_format,
                       ctx_buffer_pixels_free, NULL);
  return buffer;
}

void ctx_buffer_deinit (CtxBuffer *buffer)
{
  if (buffer->free_func)
    buffer->free_func (buffer->data, buffer->user_data);
  if (buffer->eid)
  {
    free (buffer->eid);
  }
  buffer->eid = NULL;
  buffer->data = NULL;
  buffer->free_func = NULL;
  buffer->user_data  = NULL;
  if (buffer->color_managed)
  {
    if (buffer->color_managed != buffer)
    {
      ctx_buffer_free (buffer->color_managed);
    }
    buffer->color_managed = NULL;
  }
}

void ctx_buffer_free (CtxBuffer *buffer)
{
  ctx_buffer_deinit (buffer);
  free (buffer);
}

static int
ctx_texture_check_eid (Ctx *ctx, const char *eid, int *tw, int *th)
{
  for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
  {
    if (ctx->texture[i].data &&
        ctx->texture[i].eid  &&
        !strcmp (ctx->texture[i].eid, eid))
    {
      if (tw) *tw = ctx->texture[i].width;
      if (th) *th = ctx->texture[i].height;
      ctx->texture[i].frame = ctx->texture_cache->frame;
      return i;
    }
  }
  return -1;
}

const char* ctx_texture_init (Ctx           *ctx,
                              const char    *eid,
                              int            width,
                              int            height,
                              int            stride,
                              CtxPixelFormat format,
                              void          *space,
                              uint8_t       *pixels,
                              void (*freefunc) (void *pixels, void *user_data),
                              void *user_data)
{
  int id = 0;
  if (eid)
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data &&
          ctx->texture[i].eid &&
          !strcmp (ctx->texture[i].eid, eid))
      {
        ctx->texture[i].frame = ctx->texture_cache->frame;
        if (freefunc && user_data != (void*)23)
          freefunc (pixels, user_data);
        return ctx->texture[i].eid;
      }
      if (ctx->texture[i].data == NULL 
          ||   (ctx->texture_cache->frame - ctx->texture[i].frame >= 2))
        id = i;
    }
  } else
  {
    for (int i = 0; i <  CTX_MAX_TEXTURES; i++)
    {
      if (ctx->texture[i].data == NULL 
          || (ctx->texture_cache->frame - ctx->texture[i].frame > 2))
        id = i;
    }
  }
  //int bpp = ctx_pixel_format_bits_per_pixel (format);
  ctx_buffer_deinit (&ctx->texture[id]);

  if (stride<=0)
  {
    stride = ctx_pixel_format_get_stride ((CtxPixelFormat)format, width);
  }

  if (freefunc == ctx_buffer_pixels_free && user_data == (void*)23)
  {
     uint8_t *tmp = (uint8_t*)malloc (height * stride);
     memcpy (tmp, pixels, height * stride);
     pixels = tmp;
  }

  ctx_buffer_set_data (&ctx->texture[id],
                       pixels, width, height,
                       stride, format,
                       freefunc, user_data);
#if CTX_ENABLE_CM
  ctx->texture[id].space = space;
#endif
  ctx->texture[id].frame = ctx->texture_cache->frame;
  if (eid)
  {
    /* we got an eid, this is the fast path */
    ctx->texture[id].eid = strdup (eid);
  }
  else
  {
    uint8_t hash[20];
    char ascii[41];

    CtxSHA1 *sha1 = ctx_sha1_new ();
    ctx_sha1_process (sha1, pixels, stride * height);
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    ctx->texture[id].eid = strdup (ascii);
  }
  return ctx->texture[id].eid;
}

void
_ctx_texture_prepare_color_management (CtxRasterizer *rasterizer,
                                      CtxBuffer     *buffer)
{
   switch (buffer->format->pixel_format)
   {
#ifndef NO_BABL
#if CTX_BABL
     case CTX_FORMAT_RGBA8:
        {
          buffer->color_managed = ctx_buffer_new (buffer->width, buffer->height,
                                                  CTX_FORMAT_RGBA8);
          babl_process (
             babl_fish (babl_format_with_space ("R'G'B'A u8", buffer->space),
                        babl_format_with_space ("R'G'B'A u8", rasterizer->state->gstate.device_space)),
             buffer->data, buffer->color_managed->data,
             buffer->width * buffer->height
             );
       }
       break;
     case CTX_FORMAT_RGB8:
       {
       buffer->color_managed = ctx_buffer_new (buffer->width, buffer->height,
                                               CTX_FORMAT_RGB8);
          babl_process (
             babl_fish (babl_format_with_space ("R'G'B' u8", buffer->space),
                        babl_format_with_space ("R'G'B' u8", rasterizer->state->gstate.device_space)),
             buffer->data, buffer->color_managed->data,
             buffer->width * buffer->height
             );
       }
       break;
#endif
#endif
     default:
       buffer->color_managed = buffer;
   }
}



int ctx_utf8_len (const unsigned char first_byte)
{
  if      ( (first_byte & 0x80) == 0)
    { return 1; } /* ASCII */
  else if ( (first_byte & 0xE0) == 0xC0)
    { return 2; }
  else if ( (first_byte & 0xF0) == 0xE0)
    { return 3; }
  else if ( (first_byte & 0xF8) == 0xF0)
    { return 4; }
  return 1;
}


const char *ctx_utf8_skip (const char *s, int utf8_length)
{
  int count;
  if (!s)
    { return NULL; }
  for (count = 0; *s; s++)
    {
      if ( (*s & 0xC0) != 0x80)
        { count++; }
      if (count == utf8_length + 1)
        { return s; }
    }
  return s;
}

//  XXX  :  unused
int ctx_utf8_strlen (const char *s)
{
  int count;
  if (!s)
    { return 0; }
  for (count = 0; *s; s++)
    if ( (*s & 0xC0) != 0x80)
      { count++; }
  return count;
}

int
ctx_unichar_to_utf8 (uint32_t  ch,
                     uint8_t  *dest)
{
  /* http://www.cprogramming.com/tutorial/utf8.c  */
  /*  Basic UTF-8 manipulation routines
    by Jeff Bezanson
    placed in the public domain Fall 2005 ... */
  if (ch < 0x80)
    {
      dest[0] = (char) ch;
      return 1;
    }
  if (ch < 0x800)
    {
      dest[0] = (ch>>6) | 0xC0;
      dest[1] = (ch & 0x3F) | 0x80;
      return 2;
    }
  if (ch < 0x10000)
    {
      dest[0] = (ch>>12) | 0xE0;
      dest[1] = ( (ch>>6) & 0x3F) | 0x80;
      dest[2] = (ch & 0x3F) | 0x80;
      return 3;
    }
  if (ch < 0x110000)
    {
      dest[0] = (ch>>18) | 0xF0;
      dest[1] = ( (ch>>12) & 0x3F) | 0x80;
      dest[2] = ( (ch>>6) & 0x3F) | 0x80;
      dest[3] = (ch & 0x3F) | 0x80;
      return 4;
    }
  return 0;
}

uint32_t
ctx_utf8_to_unichar (const char *input)
{
  const uint8_t *utf8 = (const uint8_t *) input;
  uint8_t c = utf8[0];
  if ( (c & 0x80) == 0)
    { return c; }
  else if ( (c & 0xE0) == 0xC0)
    return ( (utf8[0] & 0x1F) << 6) |
           (utf8[1] & 0x3F);
  else if ( (c & 0xF0) == 0xE0)
    return ( (utf8[0] & 0xF)  << 12) |
           ( (utf8[1] & 0x3F) << 6) |
           (utf8[2] & 0x3F);
  else if ( (c & 0xF8) == 0xF0)
    return ( (utf8[0] & 0x7)  << 18) |
           ( (utf8[1] & 0x3F) << 12) |
           ( (utf8[2] & 0x3F) << 6) |
           (utf8[3] & 0x3F);
  else if ( (c & 0xFC) == 0xF8)
    return ( (utf8[0] & 0x3)  << 24) |
           ( (utf8[1] & 0x3F) << 18) |
           ( (utf8[2] & 0x3F) << 12) |
           ( (utf8[3] & 0x3F) << 6) |
           (utf8[4] & 0x3F);
  else if ( (c & 0xFE) == 0xFC)
    return ( (utf8[0] & 0x1)  << 30) |
           ( (utf8[1] & 0x3F) << 24) |
           ( (utf8[2] & 0x3F) << 18) |
           ( (utf8[3] & 0x3F) << 12) |
           ( (utf8[4] & 0x3F) << 6) |
           (utf8[5] & 0x3F);
  return 0;
}

#if CTX_RASTERIZER



static int
ctx_rect_intersect (const CtxIntRectangle *a, const CtxIntRectangle *b)
{
  if (a->x >= b->x + b->width ||
      b->x >= a->x + a->width ||
      a->y >= b->y + b->height ||
      b->y >= a->y + a->height) return 0;

  return 1;
}

static void
_ctx_add_hash (CtxHasher *hasher, CtxIntRectangle *shape_rect, char *hash)
{
  CtxIntRectangle rect = {0,0, hasher->rasterizer.blit_width/hasher->cols,
                            hasher->rasterizer.blit_height/hasher->rows};
  int hno = 0;
  for (int row = 0; row < hasher->rows; row++)
    for (int col = 0; col < hasher->cols; col++, hno++)
     {
      rect.x = col * rect.width;
      rect.y = row * rect.height;
      if (ctx_rect_intersect (shape_rect, &rect))
      {
        int temp = hasher->hashes[(row * hasher->cols + col)  *20 + 0];
        for (int i = 0; i <19;i++)
           hasher->hashes[(row * hasher->cols + col)  *20 + i] =
             hasher->hashes[(row * hasher->cols + col)  *20 + i+1]^
             hash[i];
        hasher->hashes[(row * hasher->cols + col)  *20 + 19] =
                temp ^ hash[19];
      }
    }
}


static void
ctx_hasher_process (void *user_data, CtxCommand *command)
{
  CtxEntry *entry = &command->entry;
  CtxRasterizer *rasterizer = (CtxRasterizer *) user_data;
  CtxHasher *hasher = (CtxHasher*) user_data;
  CtxState *state = rasterizer->state;
  CtxCommand *c = (CtxCommand *) entry;
  int aa = 15;//rasterizer->aa;

  ctx_interpret_pos_bare (rasterizer->state, entry, NULL);
  ctx_interpret_style (rasterizer->state, entry, NULL);

  switch (c->code)
    {
      case CTX_TEXT:
        {
          CtxSHA1 sha1;
          memcpy (&sha1, &hasher->sha1_fill, sizeof (CtxSHA1));
          char ctx_sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());


          float height = ctx_get_font_size (rasterizer->ctx);
           CtxIntRectangle shape_rect;
          
           shape_rect.x=rasterizer->x;
           shape_rect.y=rasterizer->y - height,
           shape_rect.width = width;
           shape_rect.height = height * 2;
          switch ((int)ctx_state_get (rasterizer->state, CTX_text_align))
          {
          case CTX_TEXT_ALIGN_LEFT:
          case CTX_TEXT_ALIGN_START:
                  break;
          case CTX_TEXT_ALIGN_END:
          case CTX_TEXT_ALIGN_RIGHT:
           shape_rect.x -= shape_rect.width;
           break;
          case CTX_TEXT_ALIGN_CENTER:
           shape_rect.x -= shape_rect.width/2;
           break;
                   // XXX : doesn't take all text-alignments into account
          }

#if 0
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
#endif
          ctx_sha1_process(&sha1, (const unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
#if 0
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
#endif
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_STROKE_TEXT:
        {
          CtxSHA1 sha1;
          memcpy (&sha1, &hasher->sha1_stroke, sizeof (CtxSHA1));
          char ctx_sha1_hash[20];
          float width = ctx_text_width (rasterizer->ctx, ctx_arg_string());
          float height = ctx_get_font_size (rasterizer->ctx);

           CtxIntRectangle shape_rect = {
              rasterizer->x, rasterizer->y - height,
              width, height * 2
           };

#if 0
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));
#endif
          ctx_sha1_process(&sha1, (unsigned char*)ctx_arg_string(), strlen  (ctx_arg_string()));
#if 0
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
#endif
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
        }
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_GLYPH:
         {
          CtxSHA1 sha1;
          memcpy (&sha1, &hasher->sha1_fill, sizeof (CtxSHA1));

          char ctx_sha1_hash[20];
          uint8_t string[8];
          string[ctx_unichar_to_utf8 (c->u32.a0, string)]=0;
          float width = ctx_text_width (rasterizer->ctx, (char*)string);
          float height = ctx_get_font_size (rasterizer->ctx);

          float tx = rasterizer->x;
          float ty = rasterizer->y;
          float tw = width;
          float th = height * 2;

          _ctx_user_to_device (rasterizer->state, &tx, &ty);
          _ctx_user_to_device_distance (rasterizer->state, &tw, &th);
          CtxIntRectangle shape_rect = {tx,ty-th/2,tw,th};


#if 0
          uint32_t color;
          ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
#endif
          ctx_sha1_process(&sha1, string, strlen ((const char*)string));
#if 0
          ctx_sha1_process(&sha1, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);
#endif
          ctx_sha1_process(&sha1, (unsigned char*)&shape_rect, sizeof (CtxIntRectangle));
          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

          ctx_rasterizer_rel_move_to (rasterizer, width, 0);
          ctx_rasterizer_reset (rasterizer);
         }
        break;

      case CTX_FILL:
        {
          CtxSHA1 sha1;
          memcpy (&sha1, &hasher->sha1_fill, sizeof (CtxSHA1));
          char ctx_sha1_hash[20];

          /* we eant this hasher to be as good as possible internally,
           * since it is also used in the small shapes rasterization
           * cache
           */
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer); // + hasher->salt;
        CtxIntRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV - 2,
          rasterizer->scan_min / aa - 2,
          3+(rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV,
          3+(rasterizer->scan_max - rasterizer->scan_min + 1) / aa
        };

        hash ^= (rasterizer->state->gstate.fill_rule * 23);

        ctx_sha1_process(&sha1, (unsigned char*)&hash, 8);

        {
          int is = rasterizer->state->gstate.image_smoothing;
          ctx_sha1_process(&sha1, (uint8_t*)&is, sizeof(int));
        }

          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);

        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        }
        break;
      case CTX_STROKE:
        {
          CtxSHA1 sha1;
          memcpy (&sha1, &hasher->sha1_stroke, sizeof (CtxSHA1));
          char ctx_sha1_hash[20];
        uint64_t hash = ctx_rasterizer_poly_to_hash (rasterizer);
        CtxIntRectangle shape_rect = {
          rasterizer->col_min / CTX_SUBDIV - rasterizer->state->gstate.line_width,
          rasterizer->scan_min / aa - rasterizer->state->gstate.line_width,
          (rasterizer->col_max - rasterizer->col_min + 1) / CTX_SUBDIV + rasterizer->state->gstate.line_width,
          (rasterizer->scan_max - rasterizer->scan_min + 1) / aa + rasterizer->state->gstate.line_width
        };

        shape_rect.width += rasterizer->state->gstate.line_width * 2;
        shape_rect.height += rasterizer->state->gstate.line_width * 2;
        shape_rect.x -= rasterizer->state->gstate.line_width;
        shape_rect.y -= rasterizer->state->gstate.line_width;

        hash ^= (int)(rasterizer->state->gstate.line_width * 110);
        hash ^= (rasterizer->state->gstate.line_cap * 23);
        hash ^= (rasterizer->state->gstate.source_stroke.type * 117);

        ctx_sha1_process(&sha1, (unsigned char*)&hash, 8);

        uint32_t color;
        ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));

          ctx_sha1_process(&sha1, (unsigned char*)&color, 4);

          ctx_sha1_done(&sha1, (unsigned char*)ctx_sha1_hash);
          _ctx_add_hash (hasher, &shape_rect, ctx_sha1_hash);
        }
        if (!rasterizer->preserve)
          ctx_rasterizer_reset (rasterizer);
        rasterizer->preserve = 0;
        break;
        /* the above cases are the painting cases and 
         * the only ones differing from the rasterizer's process switch
         */

      case CTX_LINE_TO:
        ctx_rasterizer_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_LINE_TO:
        ctx_rasterizer_rel_line_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_MOVE_TO:
        ctx_rasterizer_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rasterizer_rel_move_to (rasterizer, c->c.x0, c->c.y0);
        break;
      case CTX_CURVE_TO:
        ctx_rasterizer_curve_to (rasterizer, c->c.x0, c->c.y0,
                                 c->c.x1, c->c.y1,
                                 c->c.x2, c->c.y2);
        break;
      case CTX_REL_CURVE_TO:
        ctx_rasterizer_rel_curve_to (rasterizer, c->c.x0, c->c.y0,
                                     c->c.x1, c->c.y1,
                                     c->c.x2, c->c.y2);
        break;
      case CTX_QUAD_TO:
        ctx_rasterizer_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_REL_QUAD_TO:
        ctx_rasterizer_rel_quad_to (rasterizer, c->c.x0, c->c.y0, c->c.x1, c->c.y1);
        break;
      case CTX_ARC:
        ctx_rasterizer_arc (rasterizer, c->arc.x, c->arc.y, c->arc.radius, c->arc.angle1, c->arc.angle2, c->arc.direction);
        break;
      case CTX_RECTANGLE:
        ctx_rasterizer_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                  c->rectangle.width, c->rectangle.height);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_rasterizer_round_rectangle (rasterizer, c->rectangle.x, c->rectangle.y,
                                        c->rectangle.width, c->rectangle.height,
                                        c->rectangle.radius);
        break;
      case CTX_SET_PIXEL:
        ctx_rasterizer_set_pixel (rasterizer, c->set_pixel.x, c->set_pixel.y,
                                  c->set_pixel.rgba[0],
                                  c->set_pixel.rgba[1],
                                  c->set_pixel.rgba[2],
                                  c->set_pixel.rgba[3]);
        break;
      case CTX_PRESERVE:
        rasterizer->preserve = 1;
        break;
      case CTX_ROTATE:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_SAVE:
      case CTX_RESTORE:
        rasterizer->uses_transforms = 1;
        ctx_interpret_transforms (rasterizer->state, entry, NULL);

        
        break;
      case CTX_FONT:
        ctx_rasterizer_set_font (rasterizer, ctx_arg_string() );
        break;
      case CTX_BEGIN_PATH:
        ctx_rasterizer_reset (rasterizer);
        break;
      case CTX_CLIP:
        // should perhaps modify a global state to include
        // in hash?
        ctx_rasterizer_clip (rasterizer);
        break;
      case CTX_CLOSE_PATH:
        ctx_rasterizer_finish_shape (rasterizer);
        break;
      case CTX_DEFINE_TEXTURE:
        {
        ctx_sha1_init (&hasher->sha1_fill);
        ctx_sha1_process (&hasher->sha1_fill, (uint8_t*)c->define_texture.eid, strlen (c->define_texture.eid));
        ctx_sha1_process(&hasher->sha1_fill, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));

        rasterizer->comp_op = NULL; // why?
        }
        break;
      case CTX_TEXTURE:
        ctx_sha1_init (&hasher->sha1_fill);
        ctx_sha1_process (&hasher->sha1_fill, (uint8_t*)c->texture.eid, strlen (c->texture.eid));
        ctx_sha1_process (&hasher->sha1_fill, (uint8_t*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        rasterizer->comp_op = NULL; // why?
        break;
      case CTX_COLOR:
        {
          uint32_t color;
          if (((int)(ctx_arg_float(0))&512))
          {
            ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_stroke.color, (uint8_t*)(&color));
            ctx_sha1_init (&hasher->sha1_stroke);
            ctx_sha1_process(&hasher->sha1_stroke, (unsigned char*)&color, 4);
          }
          else
          {
            ctx_color_get_rgba8 (rasterizer->state, &rasterizer->state->gstate.source_fill.color, (uint8_t*)(&color));
            ctx_sha1_init (&hasher->sha1_fill);
            ctx_sha1_process(&hasher->sha1_fill, (unsigned char*)&color, 4);
          }
        }
        break;
      case CTX_LINEAR_GRADIENT:
          ctx_sha1_init (&hasher->sha1_fill);
          ctx_sha1_process(&hasher->sha1_fill, 
                           (uint8_t*)c, sizeof (c->linear_gradient));
          ctx_sha1_process (&hasher->sha1_fill, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        break;
      case CTX_RADIAL_GRADIENT:
          ctx_sha1_init (&hasher->sha1_fill);
          ctx_sha1_process(&hasher->sha1_fill, 
                           (uint8_t*)c, sizeof (c->radial_gradient));
          ctx_sha1_process (&hasher->sha1_fill, (unsigned char*)(&rasterizer->state->gstate.transform), sizeof (rasterizer->state->gstate.transform));
        //ctx_state_gradient_clear_stops (rasterizer->state);
        break;
#if CTX_GRADIENTS
      case CTX_GRADIENT_STOP:
        {
          float rgba[4]= {ctx_u8_to_float (ctx_arg_u8 (4) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+1) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+2) ),
                          ctx_u8_to_float (ctx_arg_u8 (4+3) )
                         };
          ctx_sha1_process(&hasher->sha1_fill, 
                           (uint8_t*) &rgba[0], sizeof(rgba));
        }
        break;
#endif
    }
  if (command->code == CTX_LINE_WIDTH)
    {
      float x = state->gstate.line_width;
      /* normalize line width according to scaling factor
       */
      x = x * ctx_maxf (ctx_maxf (ctx_fabsf (state->gstate.transform.m[0][0]),
                                  ctx_fabsf (state->gstate.transform.m[0][1]) ),
                        ctx_maxf (ctx_fabsf (state->gstate.transform.m[1][0]),
                                  ctx_fabsf (state->gstate.transform.m[1][1]) ) );
      state->gstate.line_width = x;
    }
}

static CtxRasterizer *
ctx_hasher_init (CtxRasterizer *rasterizer, Ctx *ctx, CtxState *state, int width, int height, int cols, int rows)
{
  CtxHasher *hasher = (CtxHasher*)rasterizer;
  ctx_memset (rasterizer, 0, sizeof (CtxHasher) );
  rasterizer->vfuncs.process = ctx_hasher_process;
  rasterizer->vfuncs.free    = (CtxDestroyNotify)ctx_rasterizer_deinit;
  // XXX need own destructor to not leak ->hashes
  rasterizer->edge_list.flags |= CTX_DRAWLIST_EDGE_LIST;
  rasterizer->state       = state;
  rasterizer->ctx         = ctx;
  ctx_state_init (rasterizer->state);
  rasterizer->blit_x      = 0;
  rasterizer->blit_y      = 0;
  rasterizer->blit_width  = width;
  rasterizer->blit_height = height;
  rasterizer->state->gstate.clip_min_x  = 0;
  rasterizer->state->gstate.clip_min_y  = 0;
  rasterizer->state->gstate.clip_max_x  = width - 1;
  rasterizer->state->gstate.clip_max_y  = height - 1;
  rasterizer->scan_min    = 5000;
  rasterizer->scan_max    = -5000;
  //rasterizer->aa          = 15;

  hasher->rows = rows;
  hasher->cols = cols;

  hasher->hashes = (uint8_t*)ctx_calloc (20, rows * cols);
  ctx_sha1_init (&hasher->sha1_fill);
  ctx_sha1_init (&hasher->sha1_stroke);

  return rasterizer;
}

Ctx *ctx_hasher_new (int width, int height, int cols, int rows)
{
  Ctx *ctx           = ctx_new ();
  CtxState    *state = &ctx->state;
  CtxRasterizer *rasterizer = (CtxRasterizer *) ctx_calloc (sizeof (CtxHasher), 1);
  ctx_hasher_init (rasterizer, ctx, state, width, height, cols, rows);
  ctx_set_renderer (ctx, (void*)rasterizer);
  return ctx;
}
uint8_t *ctx_hasher_get_hash (Ctx *ctx, int col, int row)
{
  CtxHasher *hasher = (CtxHasher*)ctx->renderer;
  if (row < 0) row =0;
  if (col < 0) col =0;
  if (row >= hasher->rows) row = hasher->rows-1;
  if (col >= hasher->cols) col = hasher->cols-1;

  return &hasher->hashes[(row*hasher->cols+col)*20];
}

#endif
#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <termios.h>

#include <fcntl.h>
#include <sys/ioctl.h>
#endif

int ctx_terminal_width (void)
{
  char buf[1024];
  struct termios orig_attr;
  struct termios raw;
  tcgetattr (STDIN_FILENO, &orig_attr);
  raw = orig_attr;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0;
  fprintf (stderr, "\e[14t");
  //tcflush(STDIN_FILENO, 1);
#if __COSMOPOLITAN__
  /// XXX ?
#else
  tcdrain(STDIN_FILENO);
#endif
  int length = 0;
  usleep (1000 * 60); // to account for possibly lowish latency ssh,
                      // should be made configurable ; perhaps in
                      // an env var
  struct timeval tv = {0,0};
  fd_set rfds;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_usec = 1000 * 5;

  for (int n = 0; select(1, &rfds, NULL, NULL, &tv) && n < 20; n++)
  {
    length += read (STDIN_FILENO, &buf[length], 1);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  if (length == -1)
  {
    return 0;
  }
  char *semi = strchr (buf, ';');
  buf[length]=0;
  if (semi) {semi++; semi = strchr (semi, ';');}
  if (semi)
  {
    return atoi(semi + 1);
  }
  return 0;
}

int ctx_terminal_height (void)
{
  char buf[1024];
  struct termios orig_attr;
  struct termios raw;
  tcgetattr (STDIN_FILENO, &orig_attr);
  raw = orig_attr;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0;
  fprintf (stderr, "\e[14t");
  //tcflush(STDIN_FILENO, 1);
#if !__COSMOPOLITAN__
  tcdrain(STDIN_FILENO);
#endif
  int length = 0;
  usleep (1000 * 60); // to account for possibly lowish latency ssh,
                      // should be made configurable ; perhaps in
                      // an env var
  struct timeval tv = {0,0};
  fd_set rfds;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_usec = 1000 * 5;

  for (int n = 0; select(1, &rfds, NULL, NULL, &tv) && n < 20; n++)
  {
    length += read (STDIN_FILENO, &buf[length], 1);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  if (length == -1)
  {
    return 0;
  }
  char *semi = strchr (buf, ';');
  buf[length]=0;
  if (semi)
  {
    return atoi(semi + 1);
  }
  return 0;
}

int ctx_terminal_cols (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 80;
  return ws.ws_col;
} 

int ctx_terminal_rows (void)
{
  struct winsize ws; 
  if (ioctl(0,TIOCGWINSZ,&ws)!=0)
    return 25;
  return ws.ws_row;
}





#define DECTCEM_CURSOR_SHOW      "\033[?25h"
#define DECTCEM_CURSOR_HIDE      "\033[?25l"
#define TERMINAL_MOUSE_OFF       "\033[?1000l\033[?1003l"
#define TERMINAL_MOUSE_ON_BASIC  "\033[?1000h"
#define TERMINAL_MOUSE_ON_DRAG   "\033[?1000h\033[?1003h" /* +ON_BASIC for wider */
#define TERMINAL_MOUSE_ON_FULL   "\033[?1000h\033[?1004h" /* compatibility */
#define XTERM_ALTSCREEN_ON       "\033[?47h"
#define XTERM_ALTSCREEN_OFF      "\033[?47l"

/*************************** input handling *************************/

#if !__COSMOPOLITAN__
#include <termios.h>
#include <errno.h>
#include <signal.h>
#endif

#define DELAY_MS  100  

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif

static int  size_changed = 0;       /* XXX: global state */
static int  signal_installed = 0;   /* XXX: global state */

static const char *mouse_modes[]=
{TERMINAL_MOUSE_OFF,
 TERMINAL_MOUSE_ON_BASIC,
 TERMINAL_MOUSE_ON_DRAG,
 TERMINAL_MOUSE_ON_FULL,
 NULL};

/* note that a nick can have multiple occurences, the labels
 * should be kept the same for all occurences of a combination. */
typedef struct NcKeyCode {
  const char *nick;          /* programmers name for key (combo) */
  const char *label;         /* utf8 label for key */
  const char  sequence[10];  /* terminal sequence */
} NcKeyCode;
static const NcKeyCode keycodes[]={  

  {"up",                  "↑",     "\033[A"},
  {"down",                "↓",     "\033[B"},
  {"right",               "→",     "\033[C"},
  {"left",                "←",     "\033[D"},

  {"shift-up",            "⇧↑",    "\033[1;2A"},
  {"shift-down",          "⇧↓",    "\033[1;2B"},
  {"shift-right",         "⇧→",    "\033[1;2C"},
  {"shift-left",          "⇧←",    "\033[1;2D"},

  {"alt-up",              "^↑",    "\033[1;3A"},
  {"alt-down",            "^↓",    "\033[1;3B"},
  {"alt-right",           "^→",    "\033[1;3C"},
  {"alt-left",            "^←",    "\033[1;3D"},

  {"alt-shift-up",        "alt-s↑", "\033[1;4A"},
  {"alt-shift-down",      "alt-s↓", "\033[1;4B"},
  {"alt-shift-right",     "alt-s→", "\033[1;4C"},
  {"alt-shift-left",      "alt-s←", "\033[1;4D"},

  {"control-up",          "^↑",    "\033[1;5A"},
  {"control-down",        "^↓",    "\033[1;5B"},
  {"control-right",       "^→",    "\033[1;5C"},
  {"control-left",        "^←",    "\033[1;5D"},

  /* putty */
  {"control-up",          "^↑",    "\033OA"},
  {"control-down",        "^↓",    "\033OB"},
  {"control-right",       "^→",    "\033OC"},
  {"control-left",        "^←",    "\033OD"},

  {"control-shift-up",    "^⇧↑",   "\033[1;6A"},
  {"control-shift-down",  "^⇧↓",   "\033[1;6B"},
  {"control-shift-right", "^⇧→",   "\033[1;6C"},
  {"control-shift-left",  "^⇧←",   "\033[1;6D"},

  {"control-up",          "^↑",    "\033Oa"},
  {"control-down",        "^↓",    "\033Ob"},
  {"control-right",       "^→",    "\033Oc"},
  {"control-left",        "^←",    "\033Od"},

  {"shift-up",            "⇧↑",    "\033[a"},
  {"shift-down",          "⇧↓",    "\033[b"},
  {"shift-right",         "⇧→",    "\033[c"},
  {"shift-left",          "⇧←",    "\033[d"},

  {"insert",              "ins",   "\033[2~"},
  {"delete",              "del",   "\033[3~"},
  {"page-up",             "PgUp",  "\033[5~"},
  {"page-down",           "PdDn",  "\033[6~"},
  {"home",                "Home",  "\033OH"},
  {"end",                 "End",   "\033OF"},
  {"home",                "Home",  "\033[H"},
  {"end",                 "End",   "\033[F"},
  {"control-delete",      "^del",  "\033[3;5~"},
  {"shift-delete",        "⇧del",  "\033[3;2~"},
  {"control-shift-delete","^⇧del", "\033[3;6~"},

  {"F1",        "F1",  "\033[11~"},
  {"F2",        "F2",  "\033[12~"},
  {"F3",        "F3",  "\033[13~"},
  {"F4",        "F4",  "\033[14~"},
  {"F1",        "F1",  "\033OP"},
  {"F2",        "F2",  "\033OQ"},
  {"F3",        "F3",  "\033OR"},
  {"F4",        "F4",  "\033OS"},
  {"F5",        "F5",  "\033[15~"},
  {"F6",        "F6",  "\033[16~"},
  {"F7",        "F7",  "\033[17~"},
  {"F8",        "F8",  "\033[18~"},
  {"F9",        "F9",  "\033[19~"},
  {"F9",        "F9",  "\033[20~"},
  {"F10",       "F10", "\033[21~"},
  {"F11",       "F11", "\033[22~"},
  {"F12",       "F12", "\033[23~"},
  {"tab",       "↹",     {9, '\0'}},
  {"shift-tab", "shift+↹",  "\033[Z"},
  {"backspace", "⌫",  {127, '\0'}},
  {"space",     "␣",   " "},
  {"esc",        "␛",  "\033"},
  {"return",    "⏎",  {10,0}},
  {"return",    "⏎",  {13,0}},
  /* this section could be autogenerated by code */
  {"control-a", "^A",  {1,0}},
  {"control-b", "^B",  {2,0}},
  {"control-c", "^C",  {3,0}},
  {"control-d", "^D",  {4,0}},
  {"control-e", "^E",  {5,0}},
  {"control-f", "^F",  {6,0}},
  {"control-g", "^G",  {7,0}},
  {"control-h", "^H",  {8,0}}, /* backspace? */
  {"control-i", "^I",  {9,0}}, /* tab */
  {"control-j", "^J",  {10,0}},
  {"control-k", "^K",  {11,0}},
  {"control-l", "^L",  {12,0}},
  {"control-n", "^N",  {14,0}},
  {"control-o", "^O",  {15,0}},
  {"control-p", "^P",  {16,0}},
  {"control-q", "^Q",  {17,0}},
  {"control-r", "^R",  {18,0}},
  {"control-s", "^S",  {19,0}},
  {"control-t", "^T",  {20,0}},
  {"control-u", "^U",  {21,0}},
  {"control-v", "^V",  {22,0}},
  {"control-w", "^W",  {23,0}},
  {"control-x", "^X",  {24,0}},
  {"control-y", "^Y",  {25,0}},
  {"control-z", "^Z",  {26,0}},
  {"alt-0",     "%0",  "\0330"},
  {"alt-1",     "%1",  "\0331"},
  {"alt-2",     "%2",  "\0332"},
  {"alt-3",     "%3",  "\0333"},
  {"alt-4",     "%4",  "\0334"},
  {"alt-5",     "%5",  "\0335"},
  {"alt-6",     "%6",  "\0336"},
  {"alt-7",     "%7",  "\0337"}, /* backspace? */
  {"alt-8",     "%8",  "\0338"},
  {"alt-9",     "%9",  "\0339"},
  {"alt-+",     "%+",  "\033+"},
  {"alt--",     "%-",  "\033-"},
  {"alt-/",     "%/",  "\033/"},
  {"alt-a",     "%A",  "\033a"},
  {"alt-b",     "%B",  "\033b"},
  {"alt-c",     "%C",  "\033c"},
  {"alt-d",     "%D",  "\033d"},
  {"alt-e",     "%E",  "\033e"},
  {"alt-f",     "%F",  "\033f"},
  {"alt-g",     "%G",  "\033g"},
  {"alt-h",     "%H",  "\033h"}, /* backspace? */
  {"alt-i",     "%I",  "\033i"},
  {"alt-j",     "%J",  "\033j"},
  {"alt-k",     "%K",  "\033k"},
  {"alt-l",     "%L",  "\033l"},
  {"alt-n",     "%N",  "\033m"},
  {"alt-n",     "%N",  "\033n"},
  {"alt-o",     "%O",  "\033o"},
  {"alt-p",     "%P",  "\033p"},
  {"alt-q",     "%Q",  "\033q"},
  {"alt-r",     "%R",  "\033r"},
  {"alt-s",     "%S",  "\033s"},
  {"alt-t",     "%T",  "\033t"},
  {"alt-u",     "%U",  "\033u"},
  {"alt-v",     "%V",  "\033v"},
  {"alt-w",     "%W",  "\033w"},
  {"alt-x",     "%X",  "\033x"},
  {"alt-y",     "%Y",  "\033y"},
  {"alt-z",     "%Z",  "\033z"},
  {"shift-tab", "shift-↹", {27, 9, 0}},
  /* Linux Console  */
  {"home",      "Home", "\033[1~"},
  {"end",       "End",  "\033[4~"},
  {"F1",        "F1",   "\033[[A"},
  {"F2",        "F2",   "\033[[B"},
  {"F3",        "F3",   "\033[[C"},
  {"F4",        "F4",   "\033[[D"},
  {"F5",        "F5",   "\033[[E"},
  {"F6",        "F6",   "\033[[F"},
  {"F7",        "F7",   "\033[[G"},
  {"F8",        "F8",   "\033[[H"},
  {"F9",        "F9",   "\033[[I"},
  {"F10",       "F10",  "\033[[J"},
  {"F11",       "F11",  "\033[[K"},
  {"F12",       "F12",  "\033[[L"}, 
  {"ok",        "",     "\033[0n"},
  {NULL, }
};

static struct termios orig_attr;    /* in order to restore at exit */
static int    nc_is_raw = 0;
static int    atexit_registered = 0;
static int    mouse_mode = NC_MOUSE_NONE;

static void _nc_noraw (void)
{
  if (nc_is_raw && tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr) != -1)
    nc_is_raw = 0;
}

void
nc_at_exit (void)
{
  printf (TERMINAL_MOUSE_OFF);
  printf (XTERM_ALTSCREEN_OFF);
  _nc_noraw();
  fprintf (stdout, "\e[?25h");
  //if (ctx_native_events)
  fprintf (stdout, "\e[?201l");
  fprintf (stdout, "\e[?1049l");
}

static const char *mouse_get_event_int (Ctx *n, int *x, int *y)
{
  static int prev_state = 0;
  const char *ret = "mouse-motion";
  float relx, rely;
  signed char buf[3];
  read (n->mouse_fd, buf, 3);
  relx = buf[1];
  rely = -buf[2];

  n->mouse_x += relx * 0.1;
  n->mouse_y += rely * 0.1;

  if (n->mouse_x < 1) n->mouse_x = 1;
  if (n->mouse_y < 1) n->mouse_y = 1;
  if (n->mouse_x >= n->events.width)  n->mouse_x = n->events.width;
  if (n->mouse_y >= n->events.height) n->mouse_y = n->events.height;

  if (x) *x = n->mouse_x;
  if (y) *y = n->mouse_y;

  if ((prev_state & 1) != (buf[0] & 1))
    {
      if (buf[0] & 1) ret = "mouse-press";
    }
  else if (buf[0] & 1)
    ret = "mouse-drag";

  if ((prev_state & 2) != (buf[0] & 2))
    {
      if (buf[0] & 2) ret = "mouse2-press";
    }
  else if (buf[0] & 2)
    ret = "mouse2-drag";

  if ((prev_state & 4) != (buf[0] & 4))
    {
      if (buf[0] & 4) ret = "mouse1-press";
    }
  else if (buf[0] & 4)
    ret = "mouse1-drag";

  prev_state = buf[0];
  return ret;
}

static const char *mev_type = NULL;
static int         mev_x = 0;
static int         mev_y = 0;
static int         mev_q = 0;

static const char *mouse_get_event (Ctx  *n, int *x, int *y)
{
  if (!mev_q)
    return NULL;
  *x = mev_x;
  *y = mev_y;
  mev_q = 0;
  return mev_type;
}

static int mouse_has_event (Ctx *n)
{
  struct timeval tv;
  int retval;

  if (mouse_mode == NC_MOUSE_NONE)
    return 0;

  if (mev_q)
    return 1;

  if (n->mouse_fd == 0)
    return 0;
  return 0;

  {
    fd_set rfds;
    FD_ZERO (&rfds);
    FD_SET(n->mouse_fd, &rfds);
    tv.tv_sec = 0; tv.tv_usec = 0;
    retval = select (n->mouse_fd+1, &rfds, NULL, NULL, &tv);
  }

  if (retval != 0)
    {
      int nx = 0, ny = 0;
      const char *type = mouse_get_event_int (n, &nx, &ny);

      if ((mouse_mode < NC_MOUSE_DRAG && mev_type && !strcmp (mev_type, "drag")) ||
          (mouse_mode < NC_MOUSE_ALL && mev_type && !strcmp (mev_type, "motion")))
        {
          mev_q = 0;
          return mouse_has_event (n);
        }

      if ((mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse-motion")) ||
         (mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse1-drag")) ||
         (mev_type && !strcmp (type, mev_type) && !strcmp (type, "mouse2-drag")))
        {
          if (nx == mev_x && ny == mev_y)
          {
            mev_q = 0;
            return mouse_has_event (n);
          }
        }
      mev_x = nx;
      mev_y = ny;
      mev_type = type;
      mev_q = 1;
    }
  return retval != 0;
}


static int _nc_raw (void)
{
  struct termios raw;
  if (!isatty (STDIN_FILENO))
    return -1;
  if (!atexit_registered)
    {
      atexit (nc_at_exit);
      atexit_registered = 1;
    }
  if (tcgetattr (STDIN_FILENO, &orig_attr) == -1)
    return -1;
  raw = orig_attr;  /* modify the original mode */
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return -1;
  nc_is_raw = 1;
#if !__COSMOPOLITAN__
  tcdrain(STDIN_FILENO);
  tcflush(STDIN_FILENO, 1);
#endif
  return 0;
}

static int match_keycode (const char *buf, int length, const NcKeyCode **ret)
{
  int i;
  int matches = 0;

  if (!strncmp (buf, "\033[M", MIN(length,3)))
    {
      if (length >= 6)
        return 9001;
      return 2342;
    }
  for (i = 0; keycodes[i].nick; i++)
    if (!strncmp (buf, keycodes[i].sequence, length))
      {
        matches ++;
        if ((int)strlen (keycodes[i].sequence) == length && ret)
          {
            *ret = &keycodes[i];
            return 1;
          }
      }
  if (matches != 1 && ret)
    *ret = NULL;
  return matches==1?2:matches;
}

static void nc_resize_term (int  dummy)
{
  size_changed = 1;
}

int ctx_nct_has_event (Ctx  *n, int delay_ms)
{
  struct timeval tv;
  int retval;
  fd_set rfds;

  if (size_changed)
    return 1;
  FD_ZERO (&rfds);
  FD_SET (STDIN_FILENO, &rfds);
  tv.tv_sec = 0; tv.tv_usec = delay_ms * 1000; 
  retval = select (1, &rfds, NULL, NULL, &tv);
  if (size_changed)
    return 1;
  return retval == 1 && retval != -1;
}

const char *ctx_nct_get_event (Ctx *n, int timeoutms, int *x, int *y)
{
  unsigned char buf[20];
  int length;


  if (x) *x = -1;
  if (y) *y = -1;

  if (!signal_installed)
    {
      _nc_raw ();
      signal_installed = 1;
      signal (SIGWINCH, nc_resize_term);
    }
  if (mouse_mode) // XXX too often to do it all the time!
    printf("%s", mouse_modes[mouse_mode]);

  {
    int elapsed = 0;
    int got_event = 0;

    do {
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      got_event = mouse_has_event (n);
      if (!got_event)
        got_event = ctx_nct_has_event (n, MIN(DELAY_MS, timeoutms-elapsed));
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      /* only do this if the client has asked for idle events,
       * and perhaps programmed the ms timer?
       */
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
        return "idle";
    } while (!got_event);
  }

  if (mouse_has_event (n))
    return mouse_get_event (n, x, y);

  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const NcKeyCode *match = NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * DELAY_MS;
            if (select (1, &rfds, NULL, NULL, &tv) == 0)
              return "esc";
          }

        switch (match_keycode ((const char*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              if (!strcmp(match->nick, "ok"))
              {
                ctx_frame_ack = 1;
                return NULL;
              }
              return match->nick;
              break;
            case 9001: /* mouse event */
              if (x) *x = ((unsigned char)buf[4]-32)*1.0;
              if (y) *y = ((unsigned char)buf[5]-32)*1.0;
              switch (buf[3])
                {
                        /* XXX : todo reduce this to less string constants */
                  case 32:  return "mouse-press";
                  case 33:  return "mouse1-press";
                  case 34:  return "mouse2-press";
                  case 40:  return "alt-mouse-press";
                  case 41:  return "alt-mouse1-press";
                  case 42:  return "alt-mouse2-press";
                  case 48:  return "control-mouse-press";
                  case 49:  return "control-mouse1-press";
                  case 50:  return "control-mouse2-press";
                  case 56:  return "alt-control-mouse-press";
                  case 57:  return "alt-control-mouse1-press";
                  case 58:  return "alt-control-mouse2-press";
                  case 64:  return "mouse-drag";
                  case 65:  return "mouse1-drag";
                  case 66:  return "mouse2-drag";
                  case 71:  return "mouse-motion"; /* shift+motion */
                  case 72:  return "alt-mouse-drag";
                  case 73:  return "alt-mouse1-drag";
                  case 74:  return "alt-mouse2-drag";
                  case 75:  return "mouse-motion"; /* alt+motion */
                  case 80:  return "control-mouse-drag";
                  case 81:  return "control-mouse1-drag";
                  case 82:  return "control-mouse2-drag";
                  case 83:  return "mouse-motion"; /* ctrl+motion */
                  case 91:  return "mouse-motion"; /* ctrl+alt+motion */
                  case 95:  return "mouse-motion"; /* ctrl+alt+shift+motion */
                  case 96:  return "scroll-up";
                  case 97:  return "scroll-down";
                  case 100: return "shift-scroll-up";
                  case 101: return "shift-scroll-down";
                  case 104: return "alt-scroll-up";
                  case 105: return "alt-scroll-down";
                  case 112: return "control-scroll-up";
                  case 113: return "control-scroll-down";
                  case 116: return "control-shift-scroll-up";
                  case 117: return "control-shift-scroll-down";
                  case 35: /* (or release) */
                  case 51: /* (or ctrl-release) */
                  case 43: /* (or alt-release) */
                  case 67: return "mouse-motion";
                           /* have a separate mouse-drag ? */
                  default: {
                             static char rbuf[100];
                             sprintf (rbuf, "mouse (unhandled state: %i)", buf[3]);
                             return rbuf;
                           }
                }
            case 0: /* no matches, bail*/
              { 
                static char ret[256];
                if (length == 0 && ctx_utf8_len (buf[0])>1) /* single unicode
                                                               char */
                  {
                    int n_read = 
                    read (STDIN_FILENO, &buf[length+1], ctx_utf8_len(buf[0])-1);
                    if (n_read)
                    {
                      buf[ctx_utf8_len(buf[0])]=0;
                      strcpy (ret, (const char*)buf);
                    }
                    return ret;
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (const char*)buf);
                    return ret;
                  }
                sprintf (ret, "unhandled %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c'",
                  length>=0? buf[0]: 0, length>=0? buf[0]>31?buf[0]:'?': ' ', 
                  length>=1? buf[1]: 0, length>=1? buf[1]>31?buf[1]:'?': ' ', 
                  length>=2? buf[2]: 0, length>=2? buf[2]>31?buf[2]:'?': ' ', 
                  length>=3? buf[3]: 0, length>=3? buf[3]>31?buf[3]:'?': ' ',
                  length>=4? buf[4]: 0, length>=4? buf[4]>31?buf[4]:'?': ' ',
                  length>=5? buf[5]: 0, length>=5? buf[5]>31?buf[5]:'?': ' ',
                  length>=6? buf[6]: 0, length>=6? buf[6]>31?buf[6]:'?': ' ');
                return ret;
              }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return "key read eek";
  return "fail";
}

int ctx_nct_consume_events (Ctx *ctx)
{
  int ix, iy;
  CtxCtx *ctxctx = (CtxCtx*)ctx->renderer;
  const char *event = NULL;

  {
    float x, y;
    event = ctx_nct_get_event (ctx, 50, &ix, &iy);

    x = (ix - 1.0 + 0.5) / ctxctx->cols * ctx->events.width;
    y = (iy - 1.0)       / ctxctx->rows * ctx->events.height;

    if (!strcmp (event, "mouse-press"))
    {
      ctx_pointer_press (ctx, x, y, 0, 0);
      ctxctx->was_down = 1;
    } else if (!strcmp (event, "mouse-release"))
    {
      ctx_pointer_release (ctx, x, y, 0, 0);
      ctxctx->was_down = 0;
    } else if (!strcmp (event, "mouse-motion"))
    {
      //nct_set_cursor_pos (backend->term, ix, iy);
      //nct_flush (backend->term);
      if (ctxctx->was_down)
      {
        ctx_pointer_release (ctx, x, y, 0, 0);
        ctxctx->was_down = 0;
      }
      ctx_pointer_motion (ctx, x, y, 0, 0);
    } else if (!strcmp (event, "mouse-drag"))
    {
      ctx_pointer_motion (ctx, x, y, 0, 0);
    } else if (!strcmp (event, "size-changed"))
    {
#if 0
      int width = nct_sys_terminal_width ();
      int height = nct_sys_terminal_height ();
      nct_set_size (backend->term, width, height);
      width *= CPX;
      height *= CPX;
      free (mrg->glyphs);
      free (mrg->styles);
      free (backend->nct_pixels);
      backend->nct_pixels = calloc (width * height * 4, 1);
      mrg->glyphs = calloc ((width/CPX) * (height/CPX) * 4, 1);
      mrg->styles = calloc ((width/CPX) * (height/CPX) * 1, 1);
      mrg_set_size (mrg, width, height);
      mrg_queue_draw (mrg, NULL);
#endif

    }
    else
    {
      if (!strcmp (event, "esc"))
        ctx_key_press (ctx, 0, "escape", 0);
      else if (!strcmp (event, "space"))
        ctx_key_press (ctx, 0, "space", 0);
      else if (!strcmp (event, "enter"))
        ctx_key_press (ctx, 0, "\n", 0);
      else if (!strcmp (event, "return"))
        ctx_key_press (ctx, 0, "return", 0);
      else if (!strcmp (event, "idle"))
      {
      }
      else
      ctx_key_press (ctx, 0, event, 0);
    }
  }

  return 1;
}

const char *ctx_native_get_event (Ctx *n, int timeoutms)
{
  static unsigned char buf[256];
  int length;

  if (!signal_installed)
    {
      _nc_raw ();
      signal_installed = 1;
      signal (SIGWINCH, nc_resize_term);
    }
//if (mouse_mode) // XXX too often to do it all the time!
//  printf("%s", mouse_modes[mouse_mode]);

    int got_event = 0;
  {
    int elapsed = 0;

    do {
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      got_event = ctx_nct_has_event (n, MIN(DELAY_MS, timeoutms-elapsed));
      if (size_changed)
        {
          size_changed = 0;
          return "size-changed";
        }
      /* only do this if the client has asked for idle events,
       * and perhaps programmed the ms timer?
       */
      elapsed += MIN(DELAY_MS, timeoutms-elapsed);
      if (!got_event && timeoutms && elapsed >= timeoutms)
      {
        return "idle";
      }
    } while (!got_event);
  }

  for (length = 0; got_event && length < 200; length ++)
  {
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
         buf[length+1] = 0;
         if (!strcmp ((char*)buf, "\e[0n"))
         {
           ctx_frame_ack = 1;
           return NULL;
         }
         else if (buf[length]=='\n')
         {
           buf[length]=0;
           return (const char*)buf;
         }
      }
      got_event = ctx_nct_has_event (n, 5);
    }
  return NULL;
}

const char *ctx_key_get_label (Ctx  *n, const char *nick)
{
  int j;
  int found = -1;
  for (j = 0; keycodes[j].nick; j++)
    if (found == -1 && !strcmp (keycodes[j].nick, nick))
      return keycodes[j].label;
  return NULL;
}

void _ctx_mouse (Ctx *term, int mode)
{
  //if (term->is_st && mode > 1)
  //  mode = 1;
  if (mode != mouse_mode)
  {
    printf ("%s", mouse_modes[mode]);
    fflush (stdout);
  }
  mouse_mode = mode;
}


#endif

#if !__COSMOPOLITAN__
#include <sys/time.h>
#endif


#define usecs(time)    ((uint64_t)(time.tv_sec - start_time.tv_sec) * 1000000 + time.     tv_usec)

#if CTX_EVENTS
#if !__COSMOPOLITAN__
#if CTX_THREADS
#include <threads.h>
#endif
#endif
static struct timeval start_time;

static void
_ctx_init_ticks (void)
{
  static int done = 0;
  if (done)
    return;
  done = 1;
  gettimeofday (&start_time, NULL);
}

static inline unsigned long
_ctx_ticks (void)
{
  struct timeval measure_time;
  gettimeofday (&measure_time, NULL);
  return usecs (measure_time) - usecs (start_time);
}

unsigned long
ctx_ticks (void)
{
  _ctx_init_ticks ();
  return _ctx_ticks ();
}

uint32_t ctx_ms (Ctx *ctx)
{
  return _ctx_ticks () / 1000;
}


typedef enum _CtxFlags CtxFlags;

enum _CtxFlags {
   CTX_FLAG_DIRECT = (1<<0),
};


int _ctx_max_threads = 1;
int _ctx_enable_hash_cache = 1;
#if CTX_SHAPE_CACHE
extern int _ctx_shape_cache_enabled;
#endif

#if CTX_THREADS
static mtx_t _ctx_texture_mtx;
#endif

void _ctx_texture_lock (void)
{
#if CTX_THREADS
  mtx_lock (&_ctx_texture_mtx);
#endif
}

void _ctx_texture_unlock (void)
{
#if CTX_THREADS
  mtx_unlock (&_ctx_texture_mtx);
#endif
}


void
ctx_init (int *argc, char ***argv)
{
#if 0
  if (!getenv ("CTX_VERSION"))
  {
    int i;
    char *new_argv[*argc+3];
    new_argv[0] = "ctx";
    for (i = 0; i < *argc; i++)
    {
      new_argv[i+1] = *argv[i];
    }
    new_argv[i+1] = NULL;
    execvp (new_argv[0], new_argv);
    // if this fails .. we continue normal startup
    // and end up in self-hosted braille
  }
#endif
}

int ctx_count (Ctx *ctx)
{
  return ctx->drawlist.count;
}


static int is_in_ctx (void)
{
  char buf[1024];
  struct termios orig_attr;
  struct termios raw;
  tcgetattr (STDIN_FILENO, &orig_attr);
  raw = orig_attr;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0;
  fprintf (stderr, "\e[?200$p");
  //tcflush(STDIN_FILENO, 1);
#if !__COSMOPOLITAN__
  tcdrain(STDIN_FILENO);
#endif
  int length = 0;
  usleep (1000 * 60); // to account for possibly lowish latency ssh,
                      // should be made configurable ; perhaps in
                      // an env var
  struct timeval tv = {0,0};
  fd_set rfds;
  
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);
  tv.tv_usec = 1000 * 5;

  for (int n = 0; select(1, &rfds, NULL, NULL, &tv) && n < 20; n++)
  {
    length += read (STDIN_FILENO, &buf[length], 1);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  if (length == -1)
  {
    return 0;
  }
  char *semi = strchr (buf, ';');
  buf[length]=0;
  if (semi &&  semi[1] == '2')
  {
    return 1;
  }
  return 0;
}

extern int _ctx_damage_control;

static void ctx_list_backends()
{
    fprintf (stderr, "possible values for CTX_BACKEND:\n");
    fprintf (stderr, " ctx");
#if CTX_SDL
    fprintf (stderr, " SDL");
#endif
#if CTX_FB
    fprintf (stderr, " fb");
    fprintf (stderr, " drm");
#endif
    fprintf (stderr, " term");
    fprintf (stderr, " termimg");
    fprintf (stderr, "\n");
}

Ctx *ctx_new_ui (int width, int height)
{
#if CTX_TILED
  if (getenv ("CTX_DAMAGE_CONTROL"))
  {
    const char * val = getenv ("CTX_DAMAGE_CONTROL");
    if (!strcmp (val, "0") ||
        !strcmp (val, "off"))
      _ctx_damage_control = 0;
    else
      _ctx_damage_control = 1;
  }
#endif

  if (getenv ("CTX_HASH_CACHE"))
  {
    const char * val = getenv ("CTX_HASH_CACHE");
    if (!strcmp (val, "0"))
      _ctx_enable_hash_cache = 0;
    if (!strcmp (val, "off"))
      _ctx_enable_hash_cache = 0;
  }
#if CTX_SHAPE_CACHE
  if (getenv ("CTX_SHAPE_CACHE"))
  {
    const char * val = getenv ("CTX_SHAPE_CACHE");
    if (!strcmp (val, "0"))
      _ctx_shape_cache_enabled = 0;
    if (!strcmp (val, "off"))
      _ctx_shape_cache_enabled = 0;
  }
#endif

  if (getenv ("CTX_THREADS"))
  {
    int val = atoi (getenv ("CTX_THREADS"));
    _ctx_max_threads = val;
  }
  else
  {
    _ctx_max_threads = 2;
#ifdef _SC_NPROCESSORS_ONLN
    _ctx_max_threads = sysconf (_SC_NPROCESSORS_ONLN) / 2;
#endif
  }
  
#if CTX_THREADS
  mtx_init (&_ctx_texture_mtx, mtx_plain);
#endif

  if (_ctx_max_threads < 1) _ctx_max_threads = 1;
  if (_ctx_max_threads > CTX_MAX_THREADS) _ctx_max_threads = CTX_MAX_THREADS;

  //fprintf (stderr, "ctx using %i threads\n", _ctx_max_threads);
  const char *backend = getenv ("CTX_BACKEND");

  if (backend && !strcmp (backend, ""))
    backend = NULL;
  if (backend && !strcmp (backend, "auto"))
    backend = NULL;
  if (backend && !strcmp (backend, "list"))
  {
    ctx_list_backends ();
    exit (-1);
  }

  Ctx *ret = NULL;

  /* we do the query on auto but not on directly set ctx
   *
   */
  if ((backend && !strcmp(backend, "ctx")) ||
      (backend == NULL && is_in_ctx ()))
  {
    if (!backend || !strcmp (backend, "ctx"))
    {
      // full blown ctx protocol - in terminal or standalone
      ret = ctx_new_ctx (width, height);
    }
  }

#if CTX_SDL
  if (!ret && getenv ("DISPLAY"))
  {
    if ((backend==NULL) || (!strcmp (backend, "SDL")))
      ret = ctx_new_sdl (width, height);
  }
#endif

#if CTX_FB
  if (!ret && !getenv ("DISPLAY"))
  {
    if ((backend==NULL) || (!strcmp (backend, "drm")))
    ret = ctx_new_fb (width, height, 1);

    if (!ret)
    {
      if ((backend==NULL) || (!strcmp (backend, "fb")))
        ret = ctx_new_fb (width, height, 0);
    }
  }
#endif

#if CTX_RASTERIZER
  // braille in terminal
  if (!ret)
  {
    if ((backend==NULL) || (!strcmp (backend, "term")))
    ret = ctx_new_term (width, height);
  }
  if (!ret)
  {
    if ((backend==NULL) || (!strcmp (backend, "termimg")))
    ret = ctx_new_termimg (width, height);
  }
#endif
  if (!ret)
  {
    fprintf (stderr, "no interactive ctx backend\n");
    ctx_list_backends ();
    exit (2);
  }
  ctx_get_event (ret); // enables events
  return ret;
}
#else
void _ctx_texture_unlock (void)
{
}
void _ctx_texture_lock (void)
{
}

#endif
void _ctx_resized (Ctx *ctx, int width, int height, long time);

void ctx_set_size (Ctx *ctx, int width, int height)
{
#if CTX_EVENTS
  if (ctx->events.width != width || ctx->events.height != height)
  {
    ctx->events.width = width;
    ctx->events.height = height;
    _ctx_resized (ctx, width, height, 0);
  }
#endif
}

#if CTX_EVENTS
typedef struct CtxIdleCb {
  int (*cb) (Ctx *ctx, void *idle_data);
  void *idle_data;

  void (*destroy_notify)(void *destroy_data);
  void *destroy_data;

  int   ticks_full;
  int   ticks_remaining;
  int   is_idle;
  int   id;
} CtxIdleCb;

void _ctx_events_init (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  _ctx_init_ticks ();
  events->tap_delay_min  = 40;
  events->tap_delay_max  = 800;
  events->tap_delay_max  = 8000000; /* quick reflexes needed making it hard for some is an argument against very short values  */

  events->tap_delay_hold = 1000;
  events->tap_hysteresis = 32;  /* XXX: should be ppi dependent */
}


void _ctx_idle_iteration (Ctx *ctx)
{
  static unsigned long prev_ticks = 0;
  CtxList *l;
  CtxList *to_remove = NULL;
  unsigned long ticks = _ctx_ticks ();
  unsigned long tick_delta = (prev_ticks == 0) ? 0 : ticks - prev_ticks;
  prev_ticks = ticks;

  if (!ctx->events.idles)
  {
    return;
  }
  for (l = ctx->events.idles; l; l = l->next)
  {
    CtxIdleCb *item = l->data;

    if (item->ticks_remaining >= 0)
      item->ticks_remaining -= tick_delta;

    if (item->ticks_remaining < 0)
    {
      if (item->cb (ctx, item->idle_data) == 0)
        ctx_list_prepend (&to_remove, item);
      else
        item->ticks_remaining = item->ticks_full;
    }
  }
  for (l = to_remove; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
    ctx_list_remove (&ctx->events.idles, l->data);
  }
}


void ctx_add_key_binding_full (Ctx *ctx,
                           const char *key,
                           const char *action,
                           const char *label,
                           CtxCb       cb,
                           void       *cb_data,
                           CtxDestroyNotify destroy_notify,
                           void       *destroy_data)
{
  CtxEvents *events = &ctx->events;
  if (events->n_bindings +1 >= CTX_MAX_KEYBINDINGS)
  {
    fprintf (stderr, "warning: binding overflow\n");
    return;
  }
  events->bindings[events->n_bindings].nick = strdup (key);
  strcpy (events->bindings[events->n_bindings].nick, key);

  if (action)
    events->bindings[events->n_bindings].command = action ? strdup (action) : NULL;
  if (label)
    events->bindings[events->n_bindings].label = label ? strdup (label) : NULL;
  events->bindings[events->n_bindings].cb = cb;
  events->bindings[events->n_bindings].cb_data = cb_data;
  events->bindings[events->n_bindings].destroy_notify = destroy_notify;
  events->bindings[events->n_bindings].destroy_data = destroy_data;
  events->n_bindings++;
}

void ctx_add_key_binding (Ctx *ctx,
                          const char *key,
                          const char *action,
                          const char *label,
                          CtxCb       cb,
                          void       *cb_data)
{
  ctx_add_key_binding_full (ctx, key, action, label, cb, cb_data, NULL, NULL);
}

void ctx_clear_bindings (Ctx *ctx)
{
  CtxEvents *events = &ctx->events;
  int i;
  for (i = 0; events->bindings[i].nick; i ++)
  {
    if (events->bindings[i].destroy_notify)
      events->bindings[i].destroy_notify (events->bindings[i].destroy_data);
    free (events->bindings[i].nick);
    if (events->bindings[i].command)
      free (events->bindings[i].command);
    if (events->bindings[i].label)
      free (events->bindings[i].label);
  }
  memset (&events->bindings, 0, sizeof (events->bindings));
  events->n_bindings = 0;
}

static void
ctx_collect_events (CtxEvent *event, void *data, void *data2);
static void _ctx_bindings_key_press (CtxEvent *event, void *data1, void *data2)
{
  Ctx *ctx = event->ctx;
  CtxEvents *events = &ctx->events;
  int i;
  int handled = 0;

  for (i = events->n_bindings-1; i>=0; i--)
    if (!strcmp (events->bindings[i].nick, event->string))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
        handled = 1;
      }
    }
  if (!handled)
  for (i = events->n_bindings-1; i>=0; i--)
    if (!strcmp (events->bindings[i].nick, "unhandled"))
    {
      if (events->bindings[i].cb)
      {
        events->bindings[i].cb (event, events->bindings[i].cb_data, NULL);
        if (event->stop_propagate)
          return;
      }
    }
  ctx_collect_events (event, data1, data2);
}

CtxBinding *ctx_get_bindings (Ctx *ctx)
{
  return &ctx->events.bindings[0];
}

void ctx_remove_idle (Ctx *ctx, int handle)
{
  CtxList *l;
  CtxList *to_remove = NULL;

  if (!ctx->events.idles)
  {
    return;
  }
  for (l = ctx->events.idles; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->id == handle)
      ctx_list_prepend (&to_remove, item);
  }
  for (l = to_remove; l; l = l->next)
  {
    CtxIdleCb *item = l->data;
    if (item->destroy_notify)
      item->destroy_notify (item->destroy_data);
    ctx_list_remove (&ctx->events.idles, l->data);
  }
}

int ctx_add_timeout_full (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                          void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb              = idle_cb;
  item->idle_data       = idle_data;
  item->id              = ++ctx->events.idle_id;
  item->ticks_full      = 
  item->ticks_remaining = ms * 1000;
  item->destroy_notify  = destroy_notify;
  item->destroy_data    = destroy_data;
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_timeout (Ctx *ctx, int ms, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_timeout_full (ctx, ms, idle_cb, idle_data, NULL, NULL);
}

int ctx_add_idle_full (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data,
                                 void (*destroy_notify)(void *destroy_data), void *destroy_data)
{
  CtxIdleCb *item = calloc (sizeof (CtxIdleCb), 1);
  item->cb = idle_cb;
  item->idle_data = idle_data;
  item->id = ++ctx->events.idle_id;
  item->ticks_full =
  item->ticks_remaining = -1;
  item->is_idle = 1;
  item->destroy_notify = destroy_notify;
  item->destroy_data = destroy_data;
  ctx_list_append (&ctx->events.idles, item);
  return item->id;
}

int ctx_add_idle (Ctx *ctx, int (*idle_cb)(Ctx *ctx, void *idle_data), void *idle_data)
{
  return ctx_add_idle_full (ctx, idle_cb, idle_data, NULL, NULL);
}

#endif
/* using bigger primes would be a good idea, this falls apart due to rounding
 * when zoomed in close
 */
static inline double ctx_path_hash (void *path)
{
  double ret = 0;
#if 0
  int i;
  cairo_path_data_t *data;
  if (!path)
    return 0.99999;
  for (i = 0; i <path->num_data; i += path->data[i].header.length)
  {
    data = &path->data[i];
    switch (data->header.type) {
      case CAIRO_PATH_MOVE_TO:
        ret *= 17;
        ret += data[1].point.x;
        ret *= 113;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_LINE_TO:
        ret *= 121;
        ret += data[1].point.x;
        ret *= 1021;
        ret += data[1].point.y;
        break;
      case CAIRO_PATH_CURVE_TO:
        ret *= 3111;
        ret += data[1].point.x;
        ret *= 23;
        ret += data[1].point.y;
        ret *= 107;
        ret += data[2].point.x;
        ret *= 739;
        ret += data[2].point.y;
        ret *= 3;
        ret += data[3].point.x;
        ret *= 51;
        ret += data[3].point.y;
        break;
      case CAIRO_PATH_CLOSE_PATH:
        ret *= 51;
        break;
    }
  }
#endif
  return ret;
}

#if CTX_EVENTS
void _ctx_item_ref (CtxItem *item)
{
  if (item->ref_count < 0)
  {
    fprintf (stderr, "EEEEK!\n");
  }
  item->ref_count++;
}


void _ctx_item_unref (CtxItem *item)
{
  if (item->ref_count <= 0)
  {
    fprintf (stderr, "EEEEK!\n");
    return;
  }
  item->ref_count--;
  if (item->ref_count <=0)
  {
    {
      int i;
      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].finalize)
          item->cb[i].finalize (item->cb[i].data1, item->cb[i].data2,
                                   item->cb[i].finalize_data);
      }
    }
    if (item->path)
    {
      //cairo_path_destroy (item->path);
    }
    free (item);
  }
}


static int
path_equal (void *path,
            void *path2)
{
  //  XXX
  return 0;
}

void ctx_listen_set_cursor (Ctx      *ctx,
                            CtxCursor cursor)
{
  if (ctx->events.last_item)
  {
    ctx->events.last_item->cursor = cursor;
  }
}

void ctx_listen_full (Ctx     *ctx,
                      float    x,
                      float    y,
                      float    width,
                      float    height,
                      CtxEventType  types,
                      CtxCb    cb,
                      void    *data1,
                      void    *data2,
                      void   (*finalize)(void *listen_data,
                                         void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data)
{
  if (!ctx->events.frozen)
  {
    CtxItem *item;

    /* early bail for listeners outside screen  */
    /* XXX: fixme respect clipping */
    {
      float tx = x;
      float ty = y;
      float tw = width;
      float th = height;
      _ctx_user_to_device (&ctx->state, &tx, &ty);
      _ctx_user_to_device_distance (&ctx->state, &tw, &th);
      if (ty > ctx->events.height * 2 ||
          tx > ctx->events.width * 2 ||
          tx + tw < 0 ||
          ty + th < 0)
      {
        if (finalize)
          finalize (data1, data2, finalize_data);
        return;
      }
    }

    item = calloc (sizeof (CtxItem), 1);
    item->x0 = x;
    item->y0 = y;
    item->x1 = x + width;
    item->y1 = y + height;
    item->cb[0].types = types;
    item->cb[0].cb = cb;
    item->cb[0].data1 = data1;
    item->cb[0].data2 = data2;
    item->cb[0].finalize = finalize;
    item->cb[0].finalize_data = finalize_data;
    item->cb_count = 1;
    item->types = types;
    //item->path = cairo_copy_path (cr); // XXX
    item->path_hash = ctx_path_hash (item->path);
    ctx_get_matrix (ctx, &item->inv_matrix);
    ctx_matrix_invert (&item->inv_matrix);

    if (ctx->events.items)
    {
      CtxList *l;
      for (l = ctx->events.items; l; l = l->next)
      {
        CtxItem *item2 = l->data;

        /* store multiple callbacks for one entry when the paths
         * are exact matches, reducing per event traversal checks at the
         * cost of a little paint-hit (XXX: is this the right tradeoff,
         * perhaps it is better to spend more time during event processing
         * than during paint?)
         */
        if (item->path_hash == item2->path_hash &&
            path_equal (item->path, item2->path))
        {
          /* found an item, copy over cb data  */
          item2->cb[item2->cb_count] = item->cb[0];
          free (item);
          item2->cb_count++;
          item2->types |= types;
          return;
        }
      }
    }
    item->ref_count       = 1;
    ctx->events.last_item = item;
    ctx_list_prepend_full (&ctx->events.items, item, (void*)_ctx_item_unref, NULL);
  }
}

void ctx_event_stop_propagate (CtxEvent *event)
{
  if (event)
    event->stop_propagate = 1;
}

void ctx_listen (Ctx          *ctx,
                 CtxEventType  types,
                 CtxCb         cb,
                 void*         data1,
                 void*         data2)
{
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  if (types & CTX_KEY)
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }
  else
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }

  if (types == CTX_DRAG_MOTION)
    types = CTX_DRAG_MOTION | CTX_DRAG_PRESS;
  return ctx_listen_full (ctx, x, y, width, height, types, cb, data1, data2, NULL, NULL);
}

void  ctx_listen_with_finalize (Ctx          *ctx,
                                CtxEventType  types,
                                CtxCb         cb,
                                void*         data1,
                                void*         data2,
                      void   (*finalize)(void *listen_data, void *listen_data2,
                                         void *finalize_data),
                      void    *finalize_data)
{
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  if (types & CTX_KEY)
  {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
  }
  else
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }

  if (types == CTX_DRAG_MOTION)
    types = CTX_DRAG_MOTION | CTX_DRAG_PRESS;
  return ctx_listen_full (ctx, x, y, width, height, types, cb, data1, data2, finalize, finalize_data);
}


static void ctx_report_hit_region (CtxEvent *event,
                       void     *data,
                       void     *data2)
{
  const char *id = data;

  fprintf (stderr, "hit region %s\n", id);
  // XXX: NYI
}

void ctx_add_hit_region (Ctx *ctx, const char *id)
{
  char *id_copy = strdup (id);
  float x, y, width, height;
  /* generate bounding box of what to listen for - from current cairo path */
  {
     float ex1,ey1,ex2,ey2;
     ctx_path_extents (ctx, &ex1, &ey1, &ex2, &ey2);
     x = ex1;
     y = ey1;
     width = ex2 - ex1;
     height = ey2 - ey1;
  }
  
  return ctx_listen_full (ctx, x, y, width, height,
                          CTX_POINTER, ctx_report_hit_region,
                          id_copy, NULL, (void*)free, NULL);
}

typedef struct _CtxGrab CtxGrab;

struct _CtxGrab
{
  CtxItem *item;
  int      device_no;
  int      timeout_id;
  int      start_time;
  float    x; // for tap and hold
  float    y;
  CtxEventType  type;
};

static void grab_free (Ctx *ctx, CtxGrab *grab)
{
  if (grab->timeout_id)
  {
    ctx_remove_idle (ctx, grab->timeout_id);
    grab->timeout_id = 0;
  }
  _ctx_item_unref (grab->item);
  free (grab);
}

static void device_remove_grab (Ctx *ctx, CtxGrab *grab)
{
  ctx_list_remove (&ctx->events.grabs, grab);
  grab_free (ctx, grab);
}

static CtxGrab *device_add_grab (Ctx *ctx, int device_no, CtxItem *item, CtxEventType type)
{
  CtxGrab *grab = calloc (1, sizeof (CtxGrab));
  grab->item = item;
  grab->type = type;
  _ctx_item_ref (item);
  grab->device_no = device_no;
  ctx_list_append (&ctx->events.grabs, grab);
  return grab;
}

static CtxList *_ctx_device_get_grabs (Ctx *ctx, int device_no)
{
  CtxList *ret = NULL;
  CtxList *l;
  for (l = ctx->events.grabs; l; l = l->next)
  {
    CtxGrab *grab = l->data;
    if (grab->device_no == device_no)
      ctx_list_append (&ret, grab);
  }
  return ret;
}

static void _mrg_restore_path (Ctx *ctx, void *path)  //XXX
{
  //int i;
  //cairo_path_data_t *data;
  //cairo_new_path (cr);
  //cairo_append_path (cr, path);
}

CtxList *_ctx_detect_list (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *a;
  CtxList *ret = NULL;

  if (type == CTX_KEY_DOWN ||
      type == CTX_KEY_UP ||
      type == CTX_KEY_PRESS ||
      type == CTX_MESSAGE ||
      type == (CTX_KEY_DOWN|CTX_MESSAGE) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP) ||
      type == (CTX_KEY_DOWN|CTX_KEY_UP|CTX_MESSAGE))
  {
    for (a = ctx->events.items; a; a = a->next)
    {
      CtxItem *item = a->data;
      if (item->types & type)
      {
        ctx_list_prepend (&ret, item);
        return ret;
      }
    }
    return NULL;
  }

  for (a = ctx->events.items; a; a = a->next)
  {
    CtxItem *item= a->data;
  
    float u, v;
    u = x;
    v = y;
    ctx_matrix_apply_transform (&item->inv_matrix, &u, &v);

    if (u >= item->x0 && v >= item->y0 &&
        u <  item->x1 && v <  item->y1 && 
        ((item->types & type) || ((type == CTX_SET_CURSOR) &&
        item->cursor)))
    {
      if (item->path)
      {
        _mrg_restore_path (ctx, item->path);
        if (ctx_in_fill (ctx, u, v))
        {
          ctx_begin_path (ctx);
          ctx_list_prepend (&ret, item);
        }
        ctx_begin_path (ctx);
      }
      else
      {
        ctx_list_prepend (&ret, item);
      }
    }
  }
  return ret;
}

CtxItem *_ctx_detect (Ctx *ctx, float x, float y, CtxEventType type)
{
  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    CtxItem *ret = l->data;
    ctx_list_free (&l);
    return ret;
  }
  return NULL;
}

static int
_ctx_emit_cb_item (Ctx *ctx, CtxItem *item, CtxEvent *event, CtxEventType type, float x, float y)
{
  static CtxEvent s_event;
  CtxEvent transformed_event;
  int i;


  if (!event)
  {
    event = &s_event;
    event->type = type;
    event->x = x;
    event->y = y;
  }
  event->ctx = ctx;
  transformed_event = *event;
  transformed_event.device_x = event->x;
  transformed_event.device_y = event->y;

  {
    float tx, ty;
    tx = transformed_event.x;
    ty = transformed_event.y;
    ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.x = tx;
    transformed_event.y = ty;

    if ((type & CTX_DRAG_PRESS) ||
        (type & CTX_DRAG_MOTION) ||
        (type & CTX_MOTION))   /* probably a worthwhile check for the performance 
                                  benefit
                                */
    {
      tx = transformed_event.start_x;
      ty = transformed_event.start_y;
      ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
      transformed_event.start_x = tx;
      transformed_event.start_y = ty;
    }


    tx = transformed_event.delta_x;
    ty = transformed_event.delta_y;
    ctx_matrix_apply_transform (&item->inv_matrix, &tx, &ty);
    transformed_event.delta_x = tx;
    transformed_event.delta_y = ty;
  }

  transformed_event.state = ctx->events.modifier_state;
  transformed_event.type = type;

  for (i = item->cb_count-1; i >= 0; i--)
  {
    if (item->cb[i].types & type)
    {
      item->cb[i].cb (&transformed_event, item->cb[i].data1, item->cb[i].data2);
      event->stop_propagate = transformed_event.stop_propagate; /* copy back the response */
      if (event->stop_propagate)
        return event->stop_propagate;
    }
  }
  return 0;
}
#endif

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <stdatomic.h>
#endif

int ctx_native_events = 0;
#if CTX_SDL
int ctx_sdl_events = 0;
int ctx_sdl_consume_events (Ctx *ctx);
#endif

#if CTX_FB
int ctx_fb_events = 0;
int ctx_fb_consume_events (Ctx *ctx);
#endif

int ctx_nct_consume_events (Ctx *ctx);
int ctx_nct_has_event (Ctx  *n, int delay_ms);
int ctx_ctx_consume_events (Ctx *ctx);



void ctx_consume_events (Ctx *ctx)
{
#if CTX_SDL
  if (ctx_sdl_events)
    ctx_sdl_consume_events (ctx);
  else
#endif
#if CTX_FB
  if (ctx_fb_events)
    ctx_fb_consume_events (ctx);
  else
#endif
  if (ctx_native_events)
    ctx_ctx_consume_events (ctx);
  else
    ctx_nct_consume_events (ctx);
}

int ctx_has_event (Ctx *ctx, int timeout)
{
#if CTX_SDL
  if (ctx_sdl_events)
  {
    return SDL_WaitEventTimeout (NULL, timeout);
  }
  else
#endif
#if CTX_FB
  if (ctx_fb_events)
  {
    return ctx_nct_has_event (ctx, timeout);
  }
  else
#endif
  if (ctx_native_events)
  {
    return ctx_nct_has_event (ctx, timeout);
  }
  else
  {
    return ctx_nct_has_event (ctx, timeout);
  }

  ctx_consume_events (ctx);
  if (ctx->events.events)
    return 1;
  return 0;
}

#if CTX_FB
static int ctx_fb_get_mice_fd (Ctx *ctx);
#endif

void ctx_get_event_fds (Ctx *ctx, int *fd, int *count)
{
#if CTX_SDL
  if (ctx_sdl_events)
  {
    *count = 0;
  }
  else
#endif
#if CTX_FB
  if (ctx_fb_events)
  {
    int mice_fd = ctx_fb_get_mice_fd (ctx);
    fd[0] = STDIN_FILENO;
    if (mice_fd)
    {
      fd[1] = mice_fd;
      *count = 2;
    }
    else
    {
      *count = 1;
    }
  }
  else
#endif
  if (ctx_native_events)
  {
    fd[0] = STDIN_FILENO;
    *count = 1;
  }
  else
  {
    fd[0] = STDIN_FILENO;
    *count = 1;
  }
}

CtxEvent *ctx_get_event (Ctx *ctx)
{
  static CtxEvent event_copy;
  _ctx_idle_iteration (ctx);
  if (!ctx->events.ctx_get_event_enabled)
    ctx->events.ctx_get_event_enabled = 1;

  ctx_consume_events (ctx);

  if (ctx->events.events)
    {
      event_copy = *((CtxEvent*)(ctx->events.events->data));
      ctx_list_remove (&ctx->events.events, ctx->events.events->data);
      return &event_copy;
    }
  return NULL;
}

static int
_ctx_emit_cb (Ctx *ctx, CtxList *items, CtxEvent *event, CtxEventType type, float x, float y)
{
  CtxList *l;
  event->stop_propagate = 0;
  for (l = items; l; l = l->next)
  {
    _ctx_emit_cb_item (ctx, l->data, event, type, x, y);
    if (event->stop_propagate)
      return event->stop_propagate;
  }
  return 0;
}

/*
 * update what is the currently hovered item and returns it.. and the list of hits
 * a well.
 *
 */
static CtxItem *_ctx_update_item (Ctx *ctx, int device_no, float x, float y, CtxEventType type, CtxList **hitlist)
{
  CtxItem *current = NULL;

  CtxList *l = _ctx_detect_list (ctx, x, y, type);
  if (l)
  {
    ctx_list_reverse (&l);
    current = l->data;
  }
  if (hitlist)
    *hitlist = l;
  else
    ctx_list_free (&l);

  if (ctx->events.prev[device_no] == NULL || current == NULL || (current->path_hash != ctx->events.prev[device_no]->path_hash))
  {
// enter/leave should snapshot chain to root
// and compare with previous snapshotted chain to root
// and emit/enter/leave as appropriate..
//
// leave might be registered for emission on enter..emission?


    //int focus_radius = 2;
    if (current)
      _ctx_item_ref (current);

    if (ctx->events.prev[device_no])
    {
      {
#if 0
        CtxIntRectangle rect = {floor(ctx->events.prev[device_no]->x0-focus_radius),
                             floor(ctx->events.prev[device_no]->y0-focus_radius),
                             ceil(ctx->events.prev[device_no]->x1)-floor(ctx->events.prev[device_no]->x0) + focus_radius * 2,
                             ceil(ctx->events.prev[device_no]->y1)-floor(ctx->events.prev[device_no]->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
#endif 
      }

      _ctx_emit_cb_item (ctx, ctx->events.prev[device_no], NULL, CTX_LEAVE, x, y);
      _ctx_item_unref (ctx->events.prev[device_no]);
      ctx->events.prev[device_no] = NULL;
    }
    if (current)
    {
#if 0
      {
        CtxIntRectangle rect = {floor(current->x0-focus_radius),
                             floor(current->y0-focus_radius),
                             ceil(current->x1)-floor(current->x0) + focus_radius * 2,
                             ceil(current->y1)-floor(current->y0) + focus_radius * 2};
        mrg_queue_draw (mrg, &rect);
      }
#endif
      _ctx_emit_cb_item (ctx, current, NULL, CTX_ENTER, x, y);
      ctx->events.prev[device_no] = current;
    }
  }
  current = _ctx_detect (ctx, x, y, type);
  //fprintf (stderr, "%p\n", current);
  return current;
}

static int tap_and_hold_fire (Ctx *ctx, void *data)
{
  CtxGrab *grab = data;
  CtxList *list = NULL;
  ctx_list_prepend (&list, grab->item);
  CtxEvent event = {0, };

  event.ctx = ctx;
  event.time = ctx_ms (ctx);

  event.device_x = 
  event.x = ctx->events.pointer_x[grab->device_no];
  event.device_y = 
  event.y = ctx->events.pointer_y[grab->device_no];

  // XXX: x and y coordinates
  int ret = _ctx_emit_cb (ctx, list, &event, CTX_TAP_AND_HOLD,
      ctx->events.pointer_x[grab->device_no], ctx->events.pointer_y[grab->device_no]);

  ctx_list_free (&list);

  grab->timeout_id = 0;

  return 0;

  return ret;
}

int ctx_pointer_drop (Ctx *ctx, float x, float y, int device_no, uint32_t time,
                      char *string)
{
  CtxList *l;
  CtxList *hitlist = NULL;

  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx = ctx;
  event->x = x;
  event->y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->string    = string;
  event->time      = time;
  event->stop_propagate = 0;

  _ctx_update_item (ctx, device_no, x, y, CTX_DROP, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    _ctx_emit_cb_item (ctx, item, event, CTX_DROP, x, y);

    if (event->stop_propagate)
    {
      ctx_list_free (&hitlist);
      return 0;
    }
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);

  return 0;
}

int ctx_pointer_press (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  CtxList *hitlist = NULL;
  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->x = event->start_x = event->prev_x = x;
  event->y = event->start_y = event->prev_y = y;

  event->delta_x = event->delta_y = 0;

  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;

  if (events->pointer_down[device_no] == 1)
  {
    fprintf (stderr, "events thought device %i was already down\n", device_no);
  }
  /* doing just one of these two should be enough? */
  events->pointer_down[device_no] = 1;
  switch (device_no)
  {
    case 1:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      events->modifier_state |= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  CtxGrab *grab = NULL;
  CtxList *l;

  _ctx_update_item (ctx, device_no, x, y, 
      CTX_PRESS | CTX_DRAG_PRESS | CTX_TAP | CTX_TAP_AND_HOLD, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;
    if (item &&
        ((item->types & CTX_DRAG)||
         (item->types & CTX_TAP) ||
         (item->types & CTX_TAP_AND_HOLD)))
    {
      grab = device_add_grab (ctx, device_no, item, item->types);
      grab->start_time = time;

      if (item->types & CTX_TAP_AND_HOLD)
      {
         grab->timeout_id = ctx_add_timeout (ctx, events->tap_delay_hold, tap_and_hold_fire, grab);
      }
    }
    _ctx_emit_cb_item (ctx, item, event, CTX_PRESS, x, y);
    if (!event->stop_propagate)
      _ctx_emit_cb_item (ctx, item, event, CTX_DRAG_PRESS, x, y);

    if (event->stop_propagate)
    {
      ctx_list_free (&hitlist);
      return 0;
    }
  }

  //events_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}

void _ctx_resized (Ctx *ctx, int width, int height, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_PRESS);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);
  
  event.ctx = ctx;
  event.time = time;
  event.string = "resize-event"; /* gets delivered to clients as a key_down event, maybe message shouldbe used instead?
   */

  if (item)
  {
    event.stop_propagate = 0;
    _ctx_emit_cb_item (ctx, item, &event, CTX_KEY_PRESS, 0, 0);
  }

}

int ctx_pointer_release (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxEvents *events = &ctx->events;
  if (time == 0)
    time = ctx_ms (ctx);

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &events->drag_event[device_no];

  event->time = time;
  event->x = x;
  event->ctx = ctx;
  event->y = y;
  event->device_no = device_no;
  event->stop_propagate = 0;

  switch (device_no)
  {
    case 1:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON1)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON1;
      break;
    case 2:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON2)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON2;
      break;
    case 3:
      if (events->modifier_state & CTX_MODIFIER_STATE_BUTTON3)
        events->modifier_state -= CTX_MODIFIER_STATE_BUTTON3;
      break;
    default:
      break;
  }

  //events_queue_draw (mrg, NULL); /* in case of style change */

  if (events->pointer_down[device_no] == 0)
  {
    fprintf (stderr, "device %i already up\n", device_no);
  }
  events->pointer_down[device_no] = 0;

  events->pointer_x[device_no] = x;
  events->pointer_y[device_no] = y;
  if (device_no <= 3)
  {
    events->pointer_x[0] = x;
    events->pointer_y[0] = y;
  }
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL , *g= NULL;
  CtxGrab *grab;

  _ctx_update_item (ctx, device_no, x, y, CTX_RELEASE | CTX_DRAG_RELEASE, &hitlist);
  grablist = _ctx_device_get_grabs (ctx, device_no);

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if (!event->stop_propagate)
    {
      if (grab->item->types & CTX_TAP)
      {
        long delay = time - grab->start_time;

        if (delay > events->tap_delay_min &&
            delay < events->tap_delay_max &&
            (
              (event->start_x - x) * (event->start_x - x) +
              (event->start_y - y) * (event->start_y - y)) < ctx_pow2(events->tap_hysteresis)
            )
        {
          _ctx_emit_cb_item (ctx, grab->item, event, CTX_TAP, x, y);
        }
      }

      if (!event->stop_propagate && grab->item->types & CTX_DRAG_RELEASE)
      {
        _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_RELEASE, x, y);
      }
    }

    device_remove_grab (ctx, grab);
  }

  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_RELEASE, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

/*  for multi-touch, we use a list of active grabs - thus a grab corresponds to
 *  a device id. even during drag-grabs events propagate; to stop that stop
 *  propagation.
 */
int ctx_pointer_motion (Ctx *ctx, float x, float y, int device_no, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *grablist = NULL, *g;
  CtxGrab *grab;

  if (device_no < 0) device_no = 0;
  if (device_no >= CTX_MAX_DEVICES) device_no = CTX_MAX_DEVICES-1;
  CtxEvent *event = &ctx->events.drag_event[device_no];

  if (time == 0)
    time = ctx_ms (ctx);

  event->ctx       = ctx;
  event->x         = x;
  event->y         = y;
  event->time      = time;
  event->device_no = device_no;
  event->stop_propagate = 0;
  
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  if (device_no <= 3)
  {
    ctx->events.pointer_x[0] = x;
    ctx->events.pointer_y[0] = y;
  }

  grablist = _ctx_device_get_grabs (ctx, device_no);
  _ctx_update_item (ctx, device_no, x, y, CTX_MOTION, &hitlist);

  {
    CtxItem  *cursor_item = _ctx_detect (ctx, x, y, CTX_SET_CURSOR);
    if (cursor_item)
    {
      ctx_set_cursor (ctx, cursor_item->cursor);
    }
    else
    {
      ctx_set_cursor (ctx, CTX_CURSOR_ARROW);
    }
    CtxItem  *hovered_item = _ctx_detect (ctx, x, y, CTX_ANY);
    static CtxItem *prev_hovered_item = NULL;
    if (prev_hovered_item != hovered_item)
    {
      ctx_set_dirty (ctx, 1);
    }
    prev_hovered_item = hovered_item;
  }

  event->delta_x = x - event->prev_x;
  event->delta_y = y - event->prev_y;
  event->prev_x  = x;
  event->prev_y  = y;

  CtxList *remove_grabs = NULL;

  for (g = grablist; g; g = g->next)
  {
    grab = g->data;

    if ((grab->type & CTX_TAP) ||
        (grab->type & CTX_TAP_AND_HOLD))
    {
      if (
          (
            (event->start_x - x) * (event->start_x - x) +
            (event->start_y - y) * (event->start_y - y)) >
              ctx_pow2(ctx->events.tap_hysteresis)
         )
      {
        //fprintf (stderr, "-");
        ctx_list_prepend (&remove_grabs, grab);
      }
      else
      {
        //fprintf (stderr, ":");
      }
    }

    if (grab->type & CTX_DRAG_MOTION)
    {
      _ctx_emit_cb_item (ctx, grab->item, event, CTX_DRAG_MOTION, x, y);
      if (event->stop_propagate)
        break;
    }
  }
  if (remove_grabs)
  {
    for (g = remove_grabs; g; g = g->next)
      device_remove_grab (ctx, g->data);
    ctx_list_free (&remove_grabs);
  }
  if (hitlist)
  {
    if (!event->stop_propagate)
      _ctx_emit_cb (ctx, hitlist, event, CTX_MOTION, x, y);
    ctx_list_free (&hitlist);
  }
  ctx_list_free (&grablist);
  return 0;
}

void ctx_incoming_message (Ctx *ctx, const char *message, long time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_MESSAGE);
  CtxEvent event = {0, };

  if (!time)
    time = ctx_ms (ctx);

  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_MESSAGE;
    event.time = time;
    event.string = message;

    fprintf (stderr, "{%s|\n", message);

      for (i = 0; i < item->cb_count; i++)
      {
        if (item->cb[i].types & (CTX_MESSAGE))
        {
          event.state = ctx->events.modifier_state;
          item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
          if (event.stop_propagate)
            return;// event.stop_propagate;
        }
      }
  }
}

int ctx_scrolled (Ctx *ctx, float x, float y, CtxScrollDirection scroll_direction, uint32_t time)
{
  CtxList *hitlist = NULL;
  CtxList *l;

  int device_no = 0;
  ctx->events.pointer_x[device_no] = x;
  ctx->events.pointer_y[device_no] = y;

  CtxEvent *event = &ctx->events.drag_event[device_no];  /* XXX: might
                                       conflict with other code
                                       create a sibling member
                                       of drag_event?*/
  if (time == 0)
    time = ctx_ms (ctx);

  event->x         = event->start_x = event->prev_x = x;
  event->y         = event->start_y = event->prev_y = y;
  event->delta_x   = event->delta_y = 0;
  event->device_no = device_no;
  event->time      = time;
  event->stop_propagate = 0;
  event->scroll_direction = scroll_direction;

  _ctx_update_item (ctx, device_no, x, y, CTX_SCROLL, &hitlist);

  for (l = hitlist; l; l = l?l->next:NULL)
  {
    CtxItem *item = l->data;

    _ctx_emit_cb_item (ctx, item, event, CTX_SCROLL, x, y);

    if (event->stop_propagate)
      l = NULL;
  }

  //mrg_queue_draw (mrg, NULL); /* in case of style change, and more  */
  ctx_list_free (&hitlist);
  return 0;
}

static int ctx_str_has_prefix (const char *string, const char *prefix)
{
  for (int i = 0; prefix[i]; i++)
  {
    if (!string[i]) return 0;
    if (string[i] != prefix[i]) return 0;
  }
  return 0;
}

int ctx_key_press (Ctx *ctx, unsigned int keyval,
                   const char *string, uint32_t time)
{
  char event_type[128]="";
  float x, y; int b;
  sscanf (string, "%s %f %f %i", event_type, &x, &y, &b);
  if (!strcmp (event_type, "mouse-motion") ||
      !strcmp (event_type, "mouse-drag"))
    return ctx_pointer_motion (ctx, x, y, b, 0);
  else if (!strcmp (event_type, "mouse-press"))
    return ctx_pointer_press (ctx, x, y, b, 0);
  else if (!strcmp (event_type, "mouse-release"))
    return ctx_pointer_release (ctx, x, y, b, 0);
  //else if (!strcmp (event_type, "keydown"))
  //  return ctx_key_down (ctx, keyval, string + 8, time);
  //else if (!strcmp (event_type, "keyup"))
  //  return ctx_key_up (ctx, keyval, string + 6, time);

  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_PRESS);
  CtxEvent event = {0,};

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_KEY_PRESS;
    event.unicode = keyval; 
    event.string = strdup(string);
    event.stop_propagate = 0;
    event.time = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_PRESS))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;
}

int ctx_key_down (Ctx *ctx, unsigned int keyval,
                  const char *string, uint32_t time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_DOWN);
  CtxEvent event = {0,};

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx     = ctx;
    event.type    = CTX_KEY_DOWN;
    event.unicode = keyval; 
    event.string  = strdup(string);
    event.stop_propagate = 0;
    event.time    = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_DOWN))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;
}

int ctx_key_up (Ctx *ctx, unsigned int keyval,
                const char *string, uint32_t time)
{
  CtxItem *item = _ctx_detect (ctx, 0, 0, CTX_KEY_UP);
  CtxEvent event = {0,};

  if (time == 0)
    time = ctx_ms (ctx);
  if (item)
  {
    int i;
    event.ctx = ctx;
    event.type = CTX_KEY_UP;
    event.unicode = keyval; 
    event.string = strdup(string);
    event.stop_propagate = 0;
    event.time = time;

    for (i = 0; i < item->cb_count; i++)
    {
      if (item->cb[i].types & (CTX_KEY_UP))
      {
        event.state = ctx->events.modifier_state;
        item->cb[i].cb (&event, item->cb[i].data1, item->cb[i].data2);
        if (event.stop_propagate)
        {
          free ((void*)event.string);
          return event.stop_propagate;
        }
      }
    }
    free ((void*)event.string);
  }
  return 0;

  return 0;
}

void ctx_freeze           (Ctx *ctx)
{
  ctx->events.frozen ++;
}

void ctx_thaw             (Ctx *ctx)
{
  ctx->events.frozen --;
}
int ctx_events_frozen (Ctx *ctx)
{
  return ctx && ctx->events.frozen;
}
void ctx_events_clear_items (Ctx *ctx)
{
  ctx_list_free (&ctx->events.items);
}
int ctx_events_width (Ctx *ctx)
{
  return ctx->events.width;
}
int ctx_events_height (Ctx *ctx)
{
  return ctx->events.height;
}

float ctx_pointer_x (Ctx *ctx)
{
  return ctx->events.pointer_x[0];
}

float ctx_pointer_y (Ctx *ctx)
{
  return ctx->events.pointer_y[0];
}

int ctx_pointer_is_down (Ctx *ctx, int no)
{
  if (no < 0 || no > CTX_MAX_DEVICES) return 0;
  return ctx->events.pointer_down[no];
}

void _ctx_debug_overlays (Ctx *ctx)
{
  CtxList *a;
  ctx_save (ctx);

  ctx_line_width (ctx, 2);
  ctx_rgba (ctx, 0,0,0.8,0.5);
  for (a = ctx->events.items; a; a = a->next)
  {
    float current_x = ctx_pointer_x (ctx);
    float current_y = ctx_pointer_y (ctx);
    CtxItem *item = a->data;
    CtxMatrix matrix = item->inv_matrix;

    ctx_matrix_apply_transform (&matrix, &current_x, &current_y);

    if (current_x >= item->x0 && current_x < item->x1 &&
        current_y >= item->y0 && current_y < item->y1)
    {
      ctx_matrix_invert (&matrix);
      ctx_set_matrix (ctx, &matrix);
      _mrg_restore_path (ctx, item->path);
      ctx_stroke (ctx);
    }
  }
  ctx_restore (ctx);
}

void ctx_set_render_threads   (Ctx *ctx, int n_threads)
{
  // XXX
}
int ctx_get_render_threads   (Ctx *ctx)
{
  return _ctx_max_threads;
}
void ctx_set_hash_cache (Ctx *ctx, int enable_hash_cache)
{
  _ctx_enable_hash_cache = enable_hash_cache;
}
int ctx_get_hash_cache (Ctx *ctx)
{
  return _ctx_enable_hash_cache;
}

int ctx_is_dirty (Ctx *ctx)
{
  return ctx->dirty;
}
void ctx_set_dirty (Ctx *ctx, int dirty)
{
  ctx->dirty = dirty;
}

/*
 * centralized global API for managing file descriptors that
 * wake us up, this to remove sleeping and polling
 */

#define CTX_MAX_LISTEN_FDS 128 // becomes max clients..

static int _ctx_listen_fd[CTX_MAX_LISTEN_FDS];
static int _ctx_listen_fds    = 0;
static int _ctx_listen_max_fd = 0;

void _ctx_add_listen_fd (int fd)
{
  _ctx_listen_fd[_ctx_listen_fds++]=fd;
  if (fd > _ctx_listen_max_fd)
    _ctx_listen_max_fd = fd;
}

void _ctx_remove_listen_fd (int fd)
{
  for (int i = 0; i < _ctx_listen_fds; i++)
  {
    if (_ctx_listen_fd[i] == fd)
    {
      _ctx_listen_fd[i] = _ctx_listen_fd[_ctx_listen_fds-1];
      _ctx_listen_fds--;
      return;
    }
  }
}

int ctx_input_pending (Ctx *ctx, int timeout)
{
  struct timeval tv;
  fd_set fdset;
  FD_ZERO (&fdset);
  for (int i = 0; i < _ctx_listen_fds; i++)
  {
    FD_SET (_ctx_listen_fd[i], &fdset);
  }
  int input_fds[5];
  int n_fds;
  ctx_get_event_fds (ctx, input_fds, &n_fds);
  for (int i = 0; i < n_fds; i++)
  {
    FD_SET (input_fds[i], &fdset);
  }

  tv.tv_sec = 0;
  tv.tv_usec = timeout;
  tv.tv_sec = timeout / 1000000;
  tv.tv_usec = timeout % 1000000;
  int retval = select (_ctx_listen_max_fd + 1, &fdset, NULL, NULL, &tv);
  if (retval == -1)
  {
    perror ("select");
    return 0;
  }
  return retval;
}

void ctx_sdl_set_title (void *self, const char *new_title);
void ctx_set_title (Ctx *ctx, const char *title)
{
#if CTX_SDL
     // XXX also check we're first/only client?
   if (ctx_renderer_is_sdl (ctx))
     ctx_sdl_set_title (ctx_get_renderer (ctx), title);
#endif
}

#endif
/* the parser comes in the end, nothing in ctx knows about the parser  */

#if CTX_PARSER

/* ctx parser, */

#define CTX_ID_MAXLEN 64 // in use should not be more than 40!
                         // to offer headroom for multiplexing


struct
  _CtxParser
{
  Ctx       *ctx;
  int        t_args; // total number of arguments seen for current command
  int        state;
#if CTX_PARSER_FIXED_TEMP
  uint8_t    holding[CTX_PARSER_MAXLEN]; /*  */
#else
  uint8_t   *holding;
#endif
  int        hold_len;
  int        pos;


  int        line; /*  for error reporting */
  int        col;  /*  for error reporting */
  float      numbers[CTX_PARSER_MAX_ARGS+1];
  int        n_numbers;
  int        decimal;
  CtxCode    command;
  int        expected_args; /* low digits are literal higher values
                               carry special meaning */
  int        n_args;
  int        texture_done;
  uint8_t    texture_id[CTX_ID_MAXLEN]; // used in defineTexture only
  uint64_t   set_key_hash;
  float      pcx;
  float      pcy;
  int        color_components;
  int        color_stroke; // 0 is fill source  1 is stroke source
  CtxColorModel   color_model; // 1 gray 3 rgb 4 cmyk
  float      left_margin; // set by last user provided move_to
  int        width;       // <- maybe should be float
  int        height;
  float      cell_width;
  float      cell_height;
  int        cursor_x;    // <- leaking in from terminal
  int        cursor_y;

  int        translate_origin;

  CtxColorSpace   color_space_slot;

  void (*exit) (void *exit_data);
  void *exit_data;
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len);
  int   (*get_prop)(void *prop_data, const char *key, char **data, int *len);
  void *prop_data;
};

void
ctx_parser_set_size (CtxParser *parser,
                 int        width,
                 int        height,
                 float      cell_width,
                 float      cell_height)
{
  if (cell_width > 0)
    parser->cell_width       = cell_width;
  if (cell_height > 0)
    parser->cell_height      = cell_height;
  if (width > 0)
    parser->width            = width;
  if (height > 0)
    parser->height           = height;
}

static CtxParser *
ctx_parser_init (CtxParser *parser,
                 Ctx       *ctx,
                 int        width,
                 int        height,
                 float      cell_width,
                 float      cell_height,
                 int        cursor_x,
                 int        cursor_y,
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
                 void  *prop_data,
                 void (*exit) (void *exit_data),
                 void *exit_data
                )
{
  ctx_memset (parser, 0, sizeof (CtxParser) );
  parser->line             = 1;
  parser->ctx              = ctx;
  parser->cell_width       = cell_width;
  parser->cell_height      = cell_height;
  parser->cursor_x         = cursor_x;
  parser->cursor_y         = cursor_y;
  parser->width            = width;
  parser->height           = height;
  parser->exit             = exit;
  parser->exit_data        = exit_data;
  parser->color_model      = CTX_RGBA;
  parser->color_stroke     = 0;
  parser->color_components = 4;
  parser->command          = CTX_MOVE_TO;
  parser->set_prop         = set_prop;
  parser->get_prop         = get_prop;
  parser->prop_data        = prop_data;
  return parser;
}

CtxParser *ctx_parser_new (
  Ctx       *ctx,
  int        width,
  int        height,
  float      cell_width,
  float      cell_height,
  int        cursor_x,
  int        cursor_y,
  int   (*set_prop)(void *prop_data, uint64_t key, const char *data,  int len),
  int   (*get_prop)(void *prop_Data, const char *key, char **data, int *len),
  void  *prop_data,
  void (*exit) (void *exit_data),
  void *exit_data)
{
  return ctx_parser_init ( (CtxParser *) ctx_calloc (sizeof (CtxParser), 1),
                           ctx,
                           width, height,
                           cell_width, cell_height,
                           cursor_x, cursor_y, set_prop, get_prop, prop_data,
                           exit, exit_data);
}

void ctx_parser_free (CtxParser *parser)
{
#if !CTX_PARSER_FIXED_TEMP
  if (parser->holding)
    free (parser->holding);
#endif
  free (parser);
}

#define CTX_ARG_COLLECT_NUMBERS             50
#define CTX_ARG_STRING_OR_NUMBER            100
#define CTX_ARG_NUMBER_OF_COMPONENTS        200
#define CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1 201

static int ctx_arguments_for_code (CtxCode code)
{
  switch (code)
    {
      case CTX_SAVE:
      case CTX_START_GROUP:
      case CTX_END_GROUP:
      case CTX_IDENTITY:
      case CTX_CLOSE_PATH:
      case CTX_BEGIN_PATH:
      case CTX_RESET:
      case CTX_FLUSH:
      case CTX_RESTORE:
      case CTX_STROKE:
      case CTX_FILL:
      case CTX_NEW_PAGE:
      case CTX_CLIP:
      case CTX_EXIT:
        return 0;
      case CTX_GLOBAL_ALPHA:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_FONT_SIZE:
      case CTX_LINE_JOIN:
      case CTX_LINE_CAP:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
      case CTX_IMAGE_SMOOTHING:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_FILL_RULE:
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_MITER_LIMIT:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_VER_LINE_TO:
      case CTX_FONT:
      case CTX_ROTATE:
      case CTX_GLYPH:
        return 1;
      case CTX_TRANSLATE:
      case CTX_REL_SMOOTHQ_TO:
      case CTX_LINE_TO:
      case CTX_MOVE_TO:
      case CTX_SCALE:
      case CTX_REL_LINE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
        return 2;
      case CTX_LINEAR_GRADIENT:
      case CTX_REL_QUAD_TO:
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_FILL_RECT:
      case CTX_STROKE_RECT:
      case CTX_REL_SMOOTH_TO:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
        return 4;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
      case CTX_ROUND_RECTANGLE:
        return 5;
      case CTX_ARC:
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_APPLY_TRANSFORM:
      case CTX_RADIAL_GRADIENT:
        return 6;
      case CTX_STROKE_TEXT:
      case CTX_TEXT:
      case CTX_COLOR_SPACE:
      case CTX_DEFINE_GLYPH:
      case CTX_KERNING_PAIR:
      case CTX_TEXTURE:
      case CTX_DEFINE_TEXTURE:
        return CTX_ARG_STRING_OR_NUMBER;
      case CTX_LINE_DASH: /* append to current dashes for each argument encountered */
        return CTX_ARG_COLLECT_NUMBERS;
      //case CTX_SET_KEY:
      case CTX_COLOR:
      case CTX_SHADOW_COLOR:
        return CTX_ARG_NUMBER_OF_COMPONENTS;
      case CTX_GRADIENT_STOP:
        return CTX_ARG_NUMBER_OF_COMPONENTS_PLUS_1;

        default:
#if 1
        case CTX_SET_RGBA_U8:
        case CTX_NOP:
        case CTX_NEW_EDGE:
        case CTX_EDGE:
        case CTX_EDGE_FLIPPED:
        case CTX_CONT:
        case CTX_DATA:
        case CTX_DATA_REV:
        case CTX_SET_PIXEL:
        case CTX_REL_LINE_TO_X4:
        case CTX_REL_LINE_TO_REL_CURVE_TO:
        case CTX_REL_CURVE_TO_REL_LINE_TO:
        case CTX_REL_CURVE_TO_REL_MOVE_TO:
        case CTX_REL_LINE_TO_X2:
        case CTX_MOVE_TO_REL_LINE_TO:
        case CTX_REL_LINE_TO_REL_MOVE_TO:
        case CTX_FILL_MOVE_TO:
        case CTX_REL_QUAD_TO_REL_QUAD_TO:
        case CTX_REL_QUAD_TO_S16:
        case CTX_STROKE_SOURCE:
#endif
        return 0;
    }
}

static int ctx_parser_set_command (CtxParser *parser, CtxCode code)
{
  if (code < 150 && code >= 32)
  {
  parser->expected_args = ctx_arguments_for_code (code);
  parser->n_args = 0;
  parser->texture_done = 0;
  if (parser->expected_args >= CTX_ARG_NUMBER_OF_COMPONENTS)
    {
      parser->expected_args = (parser->expected_args % 100) + parser->color_components;
    }
  }
  return code;
}

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model, int stroke);

static int ctx_parser_resolve_command (CtxParser *parser, const uint8_t *str)
{
  uint64_t ret = str[0]; /* if it is single char it already is the CtxCode */

  /* this is handled outside the hashing to make it possible to be case insensitive
   * with the rest.
   */
  if (str[0] == CTX_SET_KEY && str[1] && str[2] == 0)
  {
    switch (str[1])
    {
      case 'm': return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);
      case 'B': return ctx_parser_set_command (parser, CTX_BLEND_MODE);
      case 'l': return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
      case 't': return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
      case 'b': return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
      case 'd': return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
      case 'j': return ctx_parser_set_command (parser, CTX_LINE_JOIN);
      case 'c': return ctx_parser_set_command (parser, CTX_LINE_CAP);
      case 'w': return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
      case 'D': return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
      case 'S': return ctx_parser_set_command (parser, CTX_IMAGE_SMOOTHING);
      case 'C': return ctx_parser_set_command (parser, CTX_SHADOW_COLOR);
      case 's': return ctx_parser_set_command (parser, CTX_SHADOW_BLUR);
      case 'x': return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_X);
      case 'y': return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_Y);
      case 'a': return ctx_parser_set_command (parser, CTX_GLOBAL_ALPHA);
      case 'f': return ctx_parser_set_command (parser, CTX_FONT_SIZE);
      case 'r': return ctx_parser_set_command (parser, CTX_FILL_RULE);
    }
  }

  if (str[0] && str[1])
    {
      uint64_t str_hash;
      /* trim ctx_ and CTX_ prefix */
      if ( (str[0] == 'c' && str[1] == 't' && str[2] == 'x' && str[3] == '_') ||
           (str[0] == 'C' && str[1] == 'T' && str[2] == 'X' && str[3] == '_') )
        {
          str += 4;
        }
      if ( (str[0] == 's' && str[1] == 'e' && str[2] == 't' && str[3] == '_') )
        { str += 4; }
      str_hash = ctx_strhash ( (char *) str, 0);
      switch (str_hash)
        {
          /* first a list of mappings to one_char hashes, handled in a
           * separate fast path switch without hashing
           */
          case CTX_arcTo:          ret = CTX_ARC_TO; break;
          case CTX_arc:            ret = CTX_ARC; break;
          case CTX_curveTo:        ret = CTX_CURVE_TO; break;
          case CTX_restore:        ret = CTX_RESTORE; break;
          case CTX_stroke:         ret = CTX_STROKE; break;
          case CTX_fill:           ret = CTX_FILL; break;
          case CTX_flush:          ret = CTX_FLUSH; break;
          case CTX_horLineTo:      ret = CTX_HOR_LINE_TO; break;
          case CTX_rotate:         ret = CTX_ROTATE; break;
          case CTX_color:          ret = CTX_COLOR; break;
          case CTX_lineTo:         ret = CTX_LINE_TO; break;
          case CTX_moveTo:         ret = CTX_MOVE_TO; break;
          case CTX_scale:          ret = CTX_SCALE; break;
          case CTX_newPage:        ret = CTX_NEW_PAGE; break;
          case CTX_quadTo:         ret = CTX_QUAD_TO; break;
          case CTX_viewBox:        ret = CTX_VIEW_BOX; break;
          case CTX_smooth_to:      ret = CTX_SMOOTH_TO; break;
          case CTX_smooth_quad_to: ret = CTX_SMOOTHQ_TO; break;
          case CTX_clear:          ret = CTX_COMPOSITE_CLEAR; break;
          case CTX_copy:           ret = CTX_COMPOSITE_COPY; break;
          case CTX_destinationOver:  ret = CTX_COMPOSITE_DESTINATION_OVER; break;
          case CTX_destinationIn:    ret = CTX_COMPOSITE_DESTINATION_IN; break;
          case CTX_destinationOut:   ret = CTX_COMPOSITE_DESTINATION_OUT; break;
          case CTX_sourceOver:       ret = CTX_COMPOSITE_SOURCE_OVER; break;
          case CTX_sourceAtop:       ret = CTX_COMPOSITE_SOURCE_ATOP; break;
          case CTX_destinationAtop:  ret = CTX_COMPOSITE_DESTINATION_ATOP; break;
          case CTX_sourceOut:        ret = CTX_COMPOSITE_SOURCE_OUT; break;
          case CTX_sourceIn:         ret = CTX_COMPOSITE_SOURCE_IN; break;
          case CTX_xor:              ret = CTX_COMPOSITE_XOR; break;
          case CTX_darken:           ret = CTX_BLEND_DARKEN; break;
          case CTX_lighten:          ret = CTX_BLEND_LIGHTEN; break;
          //case CTX_color:          ret = CTX_BLEND_COLOR; break;
          //
          //  XXX check that he special casing for color works
          //      it is the first collision and it is due to our own
          //      color, not w3c for now unique use of it
          //
          case CTX_hue:            ret = CTX_BLEND_HUE; break;
          case CTX_multiply:       ret = CTX_BLEND_MULTIPLY; break;
          case CTX_normal:         ret = CTX_BLEND_NORMAL;break;
          case CTX_screen:         ret = CTX_BLEND_SCREEN;break;
          case CTX_difference:     ret = CTX_BLEND_DIFFERENCE; break;
          case CTX_reset:          ret = CTX_RESET; break;
          case CTX_verLineTo:      ret = CTX_VER_LINE_TO; break;
          case CTX_exit:
          case CTX_done:           ret = CTX_EXIT; break;
          case CTX_closePath:      ret = CTX_CLOSE_PATH; break;
          case CTX_beginPath:
          case CTX_newPath:        ret = CTX_BEGIN_PATH; break;
          case CTX_relArcTo:       ret = CTX_REL_ARC_TO; break;
          case CTX_clip:           ret = CTX_CLIP; break;
          case CTX_relCurveTo:     ret = CTX_REL_CURVE_TO; break;
          case CTX_startGroup:     ret = CTX_START_GROUP; break;
          case CTX_endGroup:       ret = CTX_END_GROUP; break;
          case CTX_save:           ret = CTX_SAVE; break;
          case CTX_translate:      ret = CTX_TRANSLATE; break;
          case CTX_linearGradient: ret = CTX_LINEAR_GRADIENT; break;
          case CTX_relHorLineTo:   ret = CTX_REL_HOR_LINE_TO; break;
          case CTX_relLineTo:      ret = CTX_REL_LINE_TO; break;
          case CTX_relMoveTo:      ret = CTX_REL_MOVE_TO; break;
          case CTX_font:           ret = CTX_FONT; break;
          case CTX_radialGradient:ret = CTX_RADIAL_GRADIENT; break;
          case CTX_gradientAddStop:
          case CTX_addStop:        ret = CTX_GRADIENT_STOP; break;
          case CTX_relQuadTo:      ret = CTX_REL_QUAD_TO; break;
          case CTX_rectangle:
          case CTX_rect:           ret = CTX_RECTANGLE; break;
          case CTX_roundRectangle: ret = CTX_ROUND_RECTANGLE; break;
          case CTX_relSmoothTo:    ret = CTX_REL_SMOOTH_TO; break;
          case CTX_relSmoothqTo:   ret = CTX_REL_SMOOTHQ_TO; break;
          case CTX_strokeText:     ret = CTX_STROKE_TEXT; break;
          case CTX_strokeRect:     ret = CTX_STROKE_RECT; break;
          case CTX_fillRect:       ret = CTX_FILL_RECT; break;
          case CTX_relVerLineTo:   ret = CTX_REL_VER_LINE_TO; break;
          case CTX_text:           ret = CTX_TEXT; break;
          case CTX_identity:       ret = CTX_IDENTITY; break;
          case CTX_transform:      ret = CTX_APPLY_TRANSFORM; break;
          case CTX_texture:        ret = CTX_TEXTURE; break;
          case CTX_defineTexture:  ret = CTX_DEFINE_TEXTURE; break;
#if 0
          case CTX_rgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_RGB_SPACE);
          case CTX_cmykSpace:
            return ctx_parser_set_command (parser, CTX_SET_CMYK_SPACE);
          case CTX_drgbSpace:
            return ctx_parser_set_command (parser, CTX_SET_DRGB_SPACE);
#endif
          case CTX_defineGlyph:
            return ctx_parser_set_command (parser, CTX_DEFINE_GLYPH);
          case CTX_kerningPair:
            return ctx_parser_set_command (parser, CTX_KERNING_PAIR);

          case CTX_colorSpace:
            return ctx_parser_set_command (parser, CTX_COLOR_SPACE);
          case CTX_fillRule:
            return ctx_parser_set_command (parser, CTX_FILL_RULE);
          case CTX_fontSize:
          case CTX_setFontSize:
            return ctx_parser_set_command (parser, CTX_FONT_SIZE);
          case CTX_compositingMode:
            return ctx_parser_set_command (parser, CTX_COMPOSITING_MODE);

          case CTX_blend:
          case CTX_blending:
          case CTX_blendMode:
            return ctx_parser_set_command (parser, CTX_BLEND_MODE);

          case CTX_miterLimit:
            return ctx_parser_set_command (parser, CTX_MITER_LIMIT);
          case CTX_textAlign:
            return ctx_parser_set_command (parser, CTX_TEXT_ALIGN);
          case CTX_textBaseline:
            return ctx_parser_set_command (parser, CTX_TEXT_BASELINE);
          case CTX_textDirection:
            return ctx_parser_set_command (parser, CTX_TEXT_DIRECTION);
          case CTX_join:
          case CTX_lineJoin:
          case CTX_setLineJoin:
            return ctx_parser_set_command (parser, CTX_LINE_JOIN);
          case CTX_glyph:
            return ctx_parser_set_command (parser, CTX_GLYPH);
          case CTX_cap:
          case CTX_lineCap:
          case CTX_setLineCap:
            return ctx_parser_set_command (parser, CTX_LINE_CAP);
          case CTX_lineDash:
            return ctx_parser_set_command (parser, CTX_LINE_DASH);
          case CTX_lineWidth:
          case CTX_setLineWidth:
            return ctx_parser_set_command (parser, CTX_LINE_WIDTH);
          case CTX_lineDashOffset:
            return ctx_parser_set_command (parser, CTX_LINE_DASH_OFFSET);
          case CTX_imageSmoothing:
            return ctx_parser_set_command (parser, CTX_IMAGE_SMOOTHING);
          case CTX_shadowColor:
            return ctx_parser_set_command (parser, CTX_SHADOW_COLOR);
          case CTX_shadowBlur:
            return ctx_parser_set_command (parser, CTX_SHADOW_BLUR);
          case CTX_shadowOffsetX:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_X);
          case CTX_shadowOffsetY:
            return ctx_parser_set_command (parser, CTX_SHADOW_OFFSET_Y);
          case CTX_globalAlpha:
            return ctx_parser_set_command (parser, CTX_GLOBAL_ALPHA);

          case CTX_strokeSource:
            return ctx_parser_set_command (parser, CTX_STROKE_SOURCE);

          /* strings are handled directly here,
           * instead of in the one-char handler, using return instead of break
           */
          case CTX_gray:
            ctx_parser_set_color_model (parser, CTX_GRAY, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_graya:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgb:
            ctx_parser_set_color_model (parser, CTX_RGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgb:
            ctx_parser_set_color_model (parser, CTX_DRGB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgba:
            ctx_parser_set_color_model (parser, CTX_RGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgba:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyk:
            ctx_parser_set_color_model (parser, CTX_CMYK, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmyka:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lab:
            ctx_parser_set_color_model (parser, CTX_LAB, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_laba:
            ctx_parser_set_color_model (parser, CTX_LABA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lch:
            ctx_parser_set_color_model (parser, CTX_LCH, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lcha:
            ctx_parser_set_color_model (parser, CTX_LCHA, 0);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* and a full repeat of the above, with S for Stroke suffix */
          case CTX_grayS:
            ctx_parser_set_color_model (parser, CTX_GRAY, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_grayaS:
            ctx_parser_set_color_model (parser, CTX_GRAYA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgbS:
            ctx_parser_set_color_model (parser, CTX_RGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgbS:
            ctx_parser_set_color_model (parser, CTX_DRGB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_rgbaS:
            ctx_parser_set_color_model (parser, CTX_RGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_drgbaS:
            ctx_parser_set_color_model (parser, CTX_DRGBA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmykS:
            ctx_parser_set_color_model (parser, CTX_CMYK, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_cmykaS:
            ctx_parser_set_color_model (parser, CTX_CMYKA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_labS:
            ctx_parser_set_color_model (parser, CTX_LAB, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_labaS:
            ctx_parser_set_color_model (parser, CTX_LABA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lchS:
            ctx_parser_set_color_model (parser, CTX_LCH, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);
          case CTX_lchaS:
            ctx_parser_set_color_model (parser, CTX_LCHA, 1);
            return ctx_parser_set_command (parser, CTX_COLOR);

          /* words that correspond to low integer constants
          */
          case CTX_winding:     return CTX_FILL_RULE_WINDING;
          case CTX_evenOdd:
          case CTX_even_odd:    return CTX_FILL_RULE_EVEN_ODD;
          case CTX_bevel:       return CTX_JOIN_BEVEL;
          case CTX_round:       return CTX_JOIN_ROUND;
          case CTX_miter:       return CTX_JOIN_MITER;
          case CTX_none:        return CTX_CAP_NONE;
          case CTX_square:      return CTX_CAP_SQUARE;
          case CTX_start:       return CTX_TEXT_ALIGN_START;
          case CTX_end:         return CTX_TEXT_ALIGN_END;
          case CTX_left:        return CTX_TEXT_ALIGN_LEFT;
          case CTX_right:       return CTX_TEXT_ALIGN_RIGHT;
          case CTX_center:      return CTX_TEXT_ALIGN_CENTER;
          case CTX_top:         return CTX_TEXT_BASELINE_TOP;
          case CTX_bottom :     return CTX_TEXT_BASELINE_BOTTOM;
          case CTX_middle:      return CTX_TEXT_BASELINE_MIDDLE;
          case CTX_alphabetic:  return CTX_TEXT_BASELINE_ALPHABETIC;
          case CTX_hanging:     return CTX_TEXT_BASELINE_HANGING;
          case CTX_ideographic: return CTX_TEXT_BASELINE_IDEOGRAPHIC;

          case CTX_userRGB:     return CTX_COLOR_SPACE_USER_RGB;
          case CTX_deviceRGB:   return CTX_COLOR_SPACE_DEVICE_RGB;
          case CTX_userCMYK:    return CTX_COLOR_SPACE_USER_CMYK;
          case CTX_deviceCMYK:  return CTX_COLOR_SPACE_DEVICE_CMYK;
#undef STR
#undef LOWER
          default:
            ret = str_hash;
        }
    }
  if (ret == CTX_CLOSE_PATH2)
   {
     ret = CTX_CLOSE_PATH;
   }

  return ctx_parser_set_command (parser, (CtxCode) ret);
}

enum
{
  CTX_PARSER_NEUTRAL = 0,
  CTX_PARSER_NUMBER,
  CTX_PARSER_NEGATIVE_NUMBER,
  CTX_PARSER_WORD,
  CTX_PARSER_COMMENT,
  CTX_PARSER_STRING_APOS,
  CTX_PARSER_STRING_QUOT,
  CTX_PARSER_STRING_APOS_ESCAPED,
  CTX_PARSER_STRING_QUOT_ESCAPED,
  CTX_PARSER_STRING_A85,
} CTX_STATE;

static void ctx_parser_set_color_model (CtxParser *parser, CtxColorModel color_model, int stroke)
{
  parser->color_model      = color_model;
  parser->color_stroke     = stroke;
  parser->color_components = ctx_color_model_get_components (color_model);
}

static void ctx_parser_get_color_rgba (CtxParser *parser, int offset, float *red, float *green, float *blue, float *alpha)
{
  /* XXX - this function is to be deprecated */
  *alpha = 1.0;
  switch (parser->color_model)
    {
      case CTX_GRAYA:
        *alpha = parser->numbers[offset + 1];
        /* FALLTHROUGH */
      case CTX_GRAY:
        *red = *green = *blue = parser->numbers[offset + 0];
        break;
      default:
      case CTX_LABA: // NYI - needs RGB profile
      case CTX_LCHA: // NYI - needs RGB profile
      case CTX_RGBA:
        *alpha = parser->numbers[offset + 3];
        /* FALLTHROUGH */
      case CTX_LAB: // NYI
      case CTX_LCH: // NYI
      case CTX_RGB:
        *red = parser->numbers[offset + 0];
        *green = parser->numbers[offset + 1];
        *blue = parser->numbers[offset + 2];
        break;
      case CTX_CMYKA:
        *alpha = parser->numbers[offset + 4];
        /* FALLTHROUGH */
      case CTX_CMYK:
        /* should use profile instead  */
        *red = (1.0-parser->numbers[offset + 0]) *
               (1.0 - parser->numbers[offset + 3]);
        *green = (1.0-parser->numbers[offset + 1]) *
                 (1.0 - parser->numbers[offset + 3]);
        *blue = (1.0-parser->numbers[offset + 2]) *
                (1.0 - parser->numbers[offset + 3]);
        break;
    }
}

static void ctx_parser_dispatch_command (CtxParser *parser)
{
  CtxCode cmd = parser->command;
  Ctx *ctx = parser->ctx;
#if 1
  if (parser->expected_args != CTX_ARG_STRING_OR_NUMBER &&
      parser->expected_args != CTX_ARG_COLLECT_NUMBERS &&
      parser->expected_args != parser->n_numbers)
    {
      if (0)
         fprintf (stderr, "ctx:%i:%i %c got %i instead of %i args\n",
               parser->line, parser->col,
               cmd, parser->n_numbers, parser->expected_args);
    }
#endif

#define arg(a)  (parser->numbers[a])
  parser->command = CTX_NOP;
  //parser->n_args = 0;
  switch (cmd)
    {
      default:
        break; // to silence warnings about missing ones
      case CTX_PRESERVE:
        ctx_preserve (ctx);
        break;
      case CTX_FILL:
        ctx_fill (ctx);
        break;
      case CTX_SAVE:
        ctx_save (ctx);
        break;
      case CTX_START_GROUP:
        ctx_start_group (ctx);
        break;
      case CTX_END_GROUP:
        ctx_end_group (ctx);
        break;
      case CTX_STROKE:
        ctx_stroke (ctx);
        break;
      case CTX_STROKE_SOURCE:
        ctx_stroke_source (ctx);
        break;
      case CTX_RESTORE:
        ctx_restore (ctx);
        break;
#if CTX_ENABLE_CM
      case CTX_COLOR_SPACE:
        if (parser->n_numbers == 1)
        {
          parser->color_space_slot = (CtxColorSpace) arg(0);
          parser->command = CTX_COLOR_SPACE; // did this work without?
        }
        else
        {
          ctx_colorspace (ctx, (CtxColorSpace)parser->color_space_slot,
                               parser->holding, parser->pos);
        }
        break;
#endif
      case CTX_KERNING_PAIR:
        switch (parser->n_args)
        {
          case 0:
            parser->numbers[0] = ctx_utf8_to_unichar ((char*)parser->holding);
            break;
          case 1:
            parser->numbers[1] = ctx_utf8_to_unichar ((char*)parser->holding);
            break;
          case 2:
            parser->numbers[2] = strtod ((char*)parser->holding, NULL);
            {
              CtxEntry e = {CTX_KERNING_PAIR, };
              e.data.u16[0] = parser->numbers[0];
              e.data.u16[1] = parser->numbers[1];
              e.data.s32[1] = parser->numbers[2] * 256;
              ctx_process (ctx, &e);
            }
            break;
        }
        parser->command = CTX_KERNING_PAIR;
        parser->n_args ++; // make this more generic?
        break;             
      case CTX_TEXTURE:
        if (parser->texture_done)
        {
        }
        else
        if (parser->n_numbers == 2)
        {
          const char *eid = (char*)parser->holding;
          float x0 = arg(0);
          float x1 = arg(1);
          ctx_texture (ctx, eid, x0, x1);
          parser->texture_done = 1;
        }
        parser->command = CTX_TEXTURE;
        //parser->n_args++;
        break;
      case CTX_DEFINE_TEXTURE:
        if (parser->texture_done)
        {
          if (parser->texture_done++ == 1)
          {
             const char *eid = (char*)parser->texture_id;
             int width  = arg(0);
             int height = arg(1);
             CtxPixelFormat format = (CtxPixelFormat)arg(2);
             int stride = ctx_pixel_format_get_stride (format, width);


             if (parser->pos != stride * height)
             {
             fprintf (stderr, "unexpected datasize for define texture %s %ix%i\n size:%i != expected:%i - start of data: %i %i %i %i\n", eid, width, height,
                               parser->pos,
                               stride * height,
                               parser->holding[0],
                               parser->holding[1],
                               parser->holding[2],
                               parser->holding[3]
                               );
             }
             else
             ctx_define_texture (ctx, eid, width, height, stride, format, parser->holding, NULL);
          }
        }
        else
        {
        switch (parser->n_numbers)
        {
          case 0:
             strncpy ((char*)parser->texture_id, (char*)parser->holding, sizeof(parser->texture_id));
             parser->texture_id[sizeof(parser->texture_id)-1]=0;
             break;
          case 1:
          case 2:
             break;
          case 3:
             parser->texture_done = 1;
             break;
          default:
             fprintf (stderr, "!!%i\n", parser->n_numbers);
             break;
        }
        }
        parser->command = CTX_DEFINE_TEXTURE;
        break;


      case CTX_DEFINE_GLYPH:
        /* XXX : reuse n_args logic - to enforce order */
        if (parser->n_numbers == 1)
        {
          CtxEntry e = {CTX_DEFINE_GLYPH, };
          e.data.u32[0] = parser->color_space_slot;
          e.data.u32[1] = arg(0) * 256;
          ctx_process (ctx, &e);
        }
        else
        {
          int unichar = ctx_utf8_to_unichar ((char*)parser->holding);
          parser->color_space_slot = (CtxColorSpace)unichar;
        }
        parser->command = CTX_DEFINE_GLYPH;
        break;             

      case CTX_COLOR:
        {
          switch (parser->color_model)
            {
              case CTX_GRAY:
              case CTX_GRAYA:
              case CTX_RGB:
              case CTX_RGBA:
              case CTX_DRGB:
              case CTX_DRGBA:
                ctx_color_raw (ctx, parser->color_model, parser->numbers, parser->color_stroke);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYK:
              case CTX_CMYKA:
                ctx_color_raw (ctx, parser->color_model, parser->numbers, parser->color_stroke);
                break;
#else
              /* when there is no cmyk support at all in rasterizer
               * do a naive mapping to RGB on input.
               */
              case CTX_CMYK:
              case CTX_CMYKA:
              case CTX_DCMYKA:
                {
                  float rgba[4] = {1,1,1,1.0f};

                  ctx_cmyk_to_rgb (arg(0), arg(1), arg(2), arg(3), &rgba[0], &rgba[1], &rgba[2]);
                  if (parser->color_model == CTX_CMYKA)
                    { rgba[3] = arg(4); }
                  ctx_color_raw (ctx, CTX_RGBA, rgba, parser->color_stroke);
                }
                break;
#endif
              case CTX_LAB:
              case CTX_LCH:
              default:
                break;
            }
        }
        break;
      case CTX_LINE_DASH:
        if (parser->n_numbers)
        {
          ctx_line_dash (ctx, parser->numbers, parser->n_numbers);
        }
        else
        {
          ctx_line_dash (ctx, NULL, 0);
        }
        //append_dash_val (ctx, arg(0));
        break;
      case CTX_ARC_TO:
        ctx_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
        break;
      case CTX_REL_ARC_TO:
        ctx_rel_arc_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4) );
        break;
      case CTX_REL_SMOOTH_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          float ax = 2 * ctx_x (ctx) - cx;
          float ay = 2 * ctx_y (ctx) - cy;
          ctx_curve_to (ctx, ax, ay, arg(0) +  cx, arg(1) + cy,
                        arg(2) + cx, arg(3) + cy);
          parser->pcx = arg(0) + cx;
          parser->pcy = arg(1) + cy;
        }
        break;
      case CTX_SMOOTH_TO:
        {
          float ax = 2 * ctx_x (ctx) - parser->pcx;
          float ay = 2 * ctx_y (ctx) - parser->pcy;
          ctx_curve_to (ctx, ax, ay, arg(0), arg(1),
                        arg(2), arg(3) );
          parser->pcx = arg(0);
          parser->pcx = arg(1);
        }
        break;
      case CTX_SMOOTHQ_TO:
        ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0), arg(1) );
        break;
      case CTX_REL_SMOOTHQ_TO:
        {
          float cx = parser->pcx;
          float cy = parser->pcy;
          parser->pcx = 2 * ctx_x (ctx) - parser->pcx;
          parser->pcy = 2 * ctx_y (ctx) - parser->pcy;
          ctx_quad_to (ctx, parser->pcx, parser->pcy, arg(0) +  cx, arg(1) + cy);
        }
        break;
      case CTX_VER_LINE_TO:
        ctx_line_to (ctx, ctx_x (ctx), arg(0) );
        parser->command = CTX_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_HOR_LINE_TO:
        ctx_line_to (ctx, arg(0), ctx_y (ctx) );
        parser->command = CTX_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_HOR_LINE_TO:
        ctx_rel_line_to (ctx, arg(0), 0.0f);
        parser->command = CTX_REL_HOR_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_REL_VER_LINE_TO:
        ctx_rel_line_to (ctx, 0.0f, arg(0) );
        parser->command = CTX_REL_VER_LINE_TO;
        parser->pcx = ctx_x (ctx);
        parser->pcy = ctx_y (ctx);
        break;
      case CTX_ARC:
        ctx_arc (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_APPLY_TRANSFORM:
        ctx_apply_transform (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_CURVE_TO:
        ctx_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        parser->pcx = arg(2);
        parser->pcy = arg(3);
        parser->command = CTX_CURVE_TO;
        break;
      case CTX_REL_CURVE_TO:
        parser->pcx = arg(2) + ctx_x (ctx);
        parser->pcy = arg(3) + ctx_y (ctx);
        ctx_rel_curve_to (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        parser->command = CTX_REL_CURVE_TO;
        break;
      case CTX_LINE_TO:
        ctx_line_to (ctx, arg(0), arg(1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        break;
      case CTX_MOVE_TO:
        ctx_move_to (ctx, arg(0), arg(1) );
        parser->command = CTX_LINE_TO;
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        parser->left_margin = parser->pcx;
        break;
      case CTX_FONT_SIZE:
        ctx_font_size (ctx, arg(0) );
        break;
      case CTX_MITER_LIMIT:
        ctx_miter_limit (ctx, arg(0) );
        break;
      case CTX_SCALE:
        ctx_scale (ctx, arg(0), arg(1) );
        break;
      case CTX_QUAD_TO:
        parser->pcx = arg(0);
        parser->pcy = arg(1);
        ctx_quad_to (ctx, arg(0), arg(1), arg(2), arg(3) );
        parser->command = CTX_QUAD_TO;
        break;
      case CTX_REL_QUAD_TO:
        parser->pcx = arg(0) + ctx_x (ctx);
        parser->pcy = arg(1) + ctx_y (ctx);
        ctx_rel_quad_to (ctx, arg(0), arg(1), arg(2), arg(3) );
        parser->command = CTX_REL_QUAD_TO;
        break;
      case CTX_CLIP:
        ctx_clip (ctx);
        break;
      case CTX_TRANSLATE:
        ctx_translate (ctx, arg(0), arg(1) );
        break;
      case CTX_ROTATE:
        ctx_rotate (ctx, arg(0) );
        break;
      case CTX_FONT:
        ctx_font (ctx, (char *) parser->holding);
        break;

      case CTX_STROKE_TEXT:
      case CTX_TEXT:
        if (parser->n_numbers == 1)
          { ctx_rel_move_to (ctx, -parser->numbers[0], 0.0); }  //  XXX : scale by font(size)
        else
          {
            for (char *c = (char *) parser->holding; c; )
              {
                char *next_nl = ctx_strchr (c, '\n');
                if (next_nl)
                  { *next_nl = 0; }
                /* do our own layouting on a per-word basis?, to get justified
                 * margins? then we'd want explict margins rather than the
                 * implicit ones from move_to's .. making move_to work within
                 * margins.
                 */
                if (cmd == CTX_STROKE_TEXT)
                  { ctx_text_stroke (ctx, c); }
                else
                  { ctx_text (ctx, c); }
                if (next_nl)
                  {
                    *next_nl = '\n'; // swap it newline back in
                    ctx_move_to (ctx, parser->left_margin, ctx_y (ctx) +
                                 ctx_get_font_size (ctx) );
                    c = next_nl + 1;
                    if (c[0] == 0)
                      { c = NULL; }
                  }
                else
                  {
                    c = NULL;
                  }
              }
          }
        if (cmd == CTX_STROKE_TEXT)
          { parser->command = CTX_STROKE_TEXT; }
        else
          { parser->command = CTX_TEXT; }
        break;
      case CTX_REL_LINE_TO:
        ctx_rel_line_to (ctx, arg(0), arg(1) );
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        break;
      case CTX_REL_MOVE_TO:
        ctx_rel_move_to (ctx, arg(0), arg(1) );
        parser->pcx += arg(0);
        parser->pcy += arg(1);
        parser->left_margin = ctx_x (ctx);
        break;
      case CTX_LINE_WIDTH:
        ctx_line_width (ctx, arg(0));
        break;
      case CTX_LINE_DASH_OFFSET:
        ctx_line_dash_offset (ctx, arg(0));
        break;
      case CTX_IMAGE_SMOOTHING:
        ctx_image_smoothing (ctx, arg(0));
        break;
      case CTX_SHADOW_COLOR:
        ctx_shadow_rgba (ctx, arg(0), arg(1), arg(2), arg(3));
        break;
      case CTX_SHADOW_BLUR:
        ctx_shadow_blur (ctx, arg(0) );
        break;
      case CTX_SHADOW_OFFSET_X:
        ctx_shadow_offset_x (ctx, arg(0) );
        break;
      case CTX_SHADOW_OFFSET_Y:
        ctx_shadow_offset_y (ctx, arg(0) );
        break;
      case CTX_LINE_JOIN:
        ctx_line_join (ctx, (CtxLineJoin) arg(0) );
        break;
      case CTX_LINE_CAP:
        ctx_line_cap (ctx, (CtxLineCap) arg(0) );
        break;
      case CTX_COMPOSITING_MODE:
        ctx_compositing_mode (ctx, (CtxCompositingMode) arg(0) );
        break;
      case CTX_BLEND_MODE:
        {
          int blend_mode = arg(0);
          if (blend_mode == CTX_COLOR) blend_mode = CTX_BLEND_COLOR;
          ctx_blend_mode (ctx, (CtxBlend)blend_mode);
        }
        break;
      case CTX_FILL_RULE:
        ctx_fill_rule (ctx, (CtxFillRule) arg(0) );
        break;
      case CTX_TEXT_ALIGN:
        ctx_text_align (ctx, (CtxTextAlign) arg(0) );
        break;
      case CTX_TEXT_BASELINE:
        ctx_text_baseline (ctx, (CtxTextBaseline) arg(0) );
        break;
      case CTX_TEXT_DIRECTION:
        ctx_text_direction (ctx, (CtxTextDirection) arg(0) );
        break;
      case CTX_IDENTITY:
        ctx_identity (ctx);
        break;
      case CTX_RECTANGLE:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        break;
      case CTX_FILL_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        ctx_fill (ctx);
        break;
      case CTX_STROKE_RECT:
        ctx_rectangle (ctx, arg(0), arg(1), arg(2), arg(3) );
        ctx_stroke (ctx);
        break;
      case CTX_ROUND_RECTANGLE:
        ctx_round_rectangle (ctx, arg(0), arg(1), arg(2), arg(3), arg(4));
        break;
      case CTX_VIEW_BOX:
        ctx_view_box (ctx, arg(0), arg(1), arg(2), arg(3) );
        break;
      case CTX_LINEAR_GRADIENT:
        ctx_linear_gradient (ctx, arg(0), arg(1), arg(2), arg(3) );
        break;
      case CTX_RADIAL_GRADIENT:
        ctx_radial_gradient (ctx, arg(0), arg(1), arg(2), arg(3), arg(4), arg(5) );
        break;
      case CTX_GRADIENT_STOP:
        {
          float red, green, blue, alpha;
          ctx_parser_get_color_rgba (parser, 1, &red, &green, &blue, &alpha);
          ctx_gradient_add_stop (ctx, arg(0), red, green, blue, alpha);
        }
        break;
      case CTX_GLOBAL_ALPHA:
        ctx_global_alpha (ctx, arg(0) );
        break;
      case CTX_BEGIN_PATH:
        ctx_begin_path (ctx);
        break;
      case CTX_GLYPH:
        ctx_glyph (ctx, arg(0), 0);
        break;
      case CTX_CLOSE_PATH:
        ctx_close_path (ctx);
        break;
      case CTX_EXIT:
        if (parser->exit)
          { parser->exit (parser->exit_data);
            return;
          }
        break;
      case CTX_FLUSH:
        //ctx_flush (ctx);
        break;
      case CTX_RESET:
        ctx_reset (ctx);
        if (parser->translate_origin)
        {
          ctx_translate (ctx,
                         (parser->cursor_x-1) * parser->cell_width * 1.0,
                         (parser->cursor_y-1) * parser->cell_height * 1.0);
        }
        break;
    }
#undef arg
//  parser->n_numbers = 0;
}

static void ctx_parser_holding_append (CtxParser *parser, int byte)
{
#if !CTX_PARSER_FIXED_TEMP
  if (parser->hold_len < parser->pos + 1 + 1)
  {
    int new_len = parser->hold_len * 1.5;
    if (new_len < 512) new_len = 512;
    parser->holding = (uint8_t*)realloc (parser->holding, new_len);
    parser->hold_len = new_len;
  }
#endif

  parser->holding[parser->pos++]=byte;
#if CTX_PARSER_FIXED_TEMP
  if (parser->pos > (int) sizeof (parser->holding)-2)
    { parser->pos = sizeof (parser->holding)-2; }
#endif
  parser->holding[parser->pos]=0;
}

static void ctx_parser_transform_percent (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  int big   = parser->width;
  int small = parser->height;
  if (big < small)
    {
      small = parser->width;
      big   = parser->height;
    }
  switch (code)
    {
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
        switch (arg_no)
          {
            case 0:
            case 3:
              *value *= (parser->width/100.0);
              break;
            case 1:
            case 4:
              *value *= (parser->height/100.0);
              break;
            case 2:
            case 5:
              *value *= small/100.0;
              break;
          }
        break;
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
        {
          *value *= (small/100.0);
        }
        break;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        if (arg_no > 3)
          {
            *value *= (small/100.0);
          }
        else
          {
            if (arg_no % 2 == 0)
              { *value  *= ( (parser->width) /100.0); }
            else
              { *value *= ( (parser->height) /100.0); }
          }
        break;
      case CTX_ROUND_RECTANGLE:
        if (arg_no == 4)
        {
          { *value *= ((parser->height)/100.0); }
          return;
        }
        /* FALLTHROUGH */
      default: // even means x coord
        if (arg_no % 2 == 0)
          { *value  *= ((parser->width)/100.0); }
        else
          { *value *= ((parser->height)/100.0); }
        break;
    }
}

static void ctx_parser_transform_percent_height (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0);
}

static void ctx_parser_transform_percent_width (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  *value *= (parser->height/100.0);
}

static void ctx_parser_transform_cell (CtxParser *parser, CtxCode code, int arg_no, float *value)
{
  float small = parser->cell_width;
  if (small > parser->cell_height)
    { small = parser->cell_height; }
  switch (code)
    {
      case CTX_RADIAL_GRADIENT:
      case CTX_ARC:
        switch (arg_no)
          {
            case 0:
            case 3:
              *value *= parser->cell_width;
              break;
            case 1:
            case 4:
              *value *= parser->cell_height;
              break;
            case 2:
            case 5:
              *value *= small; // use height?
              break;
          }
        break;
      case CTX_MITER_LIMIT:
      case CTX_FONT_SIZE:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
        {
          *value *= parser->cell_height;
        }
        break;
      case CTX_ARC_TO:
      case CTX_REL_ARC_TO:
        if (arg_no > 3)
          {
            *value *= small;
          }
        else
          {
            *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
          }
        break;
      case CTX_RECTANGLE:
        if (arg_no % 2 == 0)
          { *value *= parser->cell_width; }
        else
          {
            if (! (arg_no > 1) )
              { (*value) -= 1.0f; }
            *value *= parser->cell_height;
          }
        break;
      default: // even means x coord odd means y coord
        *value *= (arg_no%2==0) ?parser->cell_width:parser->cell_height;
        break;
    }
}

// %h %v %m %M

static void ctx_parser_number_done (CtxParser *parser)
{

}

static void ctx_parser_word_done (CtxParser *parser)
{
  parser->holding[parser->pos]=0;
  //int old_args = parser->expected_args;
  int command = ctx_parser_resolve_command (parser, parser->holding);
  if ((command >= 0 && command < 32)
      || (command > 150) || (command < 0)
      )  // special case low enum values
    {                   // and enum values too high to be
                        // commands - permitting passing words
                        // for strings in some cases
      parser->numbers[parser->n_numbers] = command;

      // trigger transition from number
      parser->state = CTX_PARSER_NUMBER;
      ctx_parser_feed_byte (parser, ',');
    }
  else if (command > 0)
    {
#if 0
      if (old_args == CTX_ARG_COLLECT_NUMBERS ||
          old_args == CTX_ARG_STRING_OR_NUMBER)
      {
        int tmp1 = parser->command;
        int tmp2 = parser->expected_args;
        int tmp3 = parser->n_numbers;
 //     int tmp4 = parser->n_args;
        ctx_parser_dispatch_command (parser);
        parser->command = (CtxCode)tmp1;
        parser->expected_args = tmp2;
        parser->n_numbers = tmp3;
 //     parser->n_args = tmp4;
      }
#endif

      parser->command = (CtxCode) command;
      parser->n_numbers = 0;
      parser->n_args = 0;
      if (parser->expected_args == 0)
        {
          ctx_parser_dispatch_command (parser);
        }
    }
  else
    {
      /* interpret char by char */
      uint8_t buf[16]=" ";
      for (int i = 0; parser->pos && parser->holding[i] > ' '; i++)
        {
          buf[0] = parser->holding[i];
          parser->command = (CtxCode) ctx_parser_resolve_command (parser, buf);
          parser->n_numbers = 0;
          parser->n_args = 0;
          if (parser->command > 0)
            {
              if (parser->expected_args == 0)
                {
                  ctx_parser_dispatch_command (parser);
                }
            }
          else
            {
              ctx_log ("unhandled command '%c'\n", buf[0]);
            }
        }
    }
}

static void ctx_parser_string_done (CtxParser *parser)
{
  if (parser->expected_args == CTX_ARG_STRING_OR_NUMBER)
  {
          /*
    if (parser->state != CTX_PARSER_NUMBER &&
        parser->state != CTX_PARSER_NEGATIVE_NUMBER &&
        parser->state != CTX_PARSER_STRING_A85 &&
        parser->state != CTX_PARSER_STRING_APOS &&
        parser->state != CTX_PARSER_STRING_QUOT
        )
        */
    {
    int tmp1 = parser->command;
    int tmp2 = parser->expected_args;
    int tmp3 = parser->n_numbers;
    int tmp4 = parser->n_args;
    ctx_parser_dispatch_command (parser);
    parser->command = (CtxCode)tmp1;
    parser->expected_args = tmp2;
    parser->n_numbers = tmp3;
    parser->n_args = tmp4;
    }
  }
  else
  {
    ctx_parser_dispatch_command (parser);
  }
}

void ctx_parser_feed_byte (CtxParser *parser, int byte)
{
  switch (byte)
    {
      case '\n':
        parser->col=0;
        parser->line++;
        break;
      default:
        parser->col++;
    }
  switch (parser->state)
    {
      case CTX_PARSER_NEUTRAL:
        switch (byte)
          {
            case  0: case  1: case  2: case  3:  case 4:  case 5:
            case  6: case  7: case  8: case 11: case 12: case 14:
            case 15: case 16: case 17: case 18: case 19: case 20:
            case 21: case 22: case 23: case 24: case 25: case 26:
            case 27: case 28: case 29: case 30: case 31:
              break;
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')':
            case '{': case '}':
            case '=':
              break;
            case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
            case '\'':
              parser->state = CTX_PARSER_STRING_APOS;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '~':
              parser->state = CTX_PARSER_STRING_A85;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '"':
              parser->state = CTX_PARSER_STRING_QUOT;
              parser->pos = 0;
              parser->holding[0] = 0;
              break;
            case '-':
              parser->state = CTX_PARSER_NEGATIVE_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->decimal = 1;
              break;
            default:
              parser->state = CTX_PARSER_WORD;
              parser->pos = 0;
              ctx_parser_holding_append (parser, byte);
              break;
          }
        break;
      case CTX_PARSER_NUMBER:
      case CTX_PARSER_NEGATIVE_NUMBER:
        {
          switch (byte)
            {
              case 0: case 1: case 2: case 3: case 4: case 5:
              case 6: case 7: case 8:
              case 11: case 12: case 14: case 15: case 16:
              case 17: case 18: case 19: case 20: case 21:
              case 22: case 23: case 24: case 25: case 26:
              case 27: case 28: case 29: case 30: case 31:
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case ' ':
              case '\t':
              case '\r':
              case '\n':
              case ';':
              case ',':
              case '(':
              case ')':
              case '{':
              case '}':
              case '=':
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '#':
                parser->state = CTX_PARSER_COMMENT;
                break;
              case '-':
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_NEGATIVE_NUMBER;
                parser->numbers[parser->n_numbers+1] = 0;
                parser->n_numbers ++;
                parser->decimal = 0;
                break;
              case '.':
                //if (parser->decimal) // TODO permit .13.32.43 to equivalent to .12 .32 .43
                parser->decimal = 1;
                break;
              case '0': case '1': case '2': case '3': case '4':
              case '5': case '6': case '7': case '8': case '9':
                if (parser->decimal)
                  {
                    parser->decimal *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0') / (1.0 * parser->decimal);
                  }
                else
                  {
                    parser->numbers[parser->n_numbers] *= 10;
                    parser->numbers[parser->n_numbers] += (byte - '0');
                  }
                break;
              case '@': // cells
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_cell (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '%': // percent of width/height
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '^': // percent of height
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_height (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              case '~': // percent of width
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                {
                float fval = parser->numbers[parser->n_numbers];
                ctx_parser_transform_percent_width (parser, parser->command, parser->n_numbers, &fval);
                parser->numbers[parser->n_numbers]= fval;
                }
                parser->state = CTX_PARSER_NEUTRAL;
                break;
              default:
                if (parser->state == CTX_PARSER_NEGATIVE_NUMBER)
                  { parser->numbers[parser->n_numbers] *= -1; }
                parser->state = CTX_PARSER_WORD;
                parser->pos = 0;
                ctx_parser_holding_append (parser, byte);
                break;
            }
          if ( (parser->state != CTX_PARSER_NUMBER) &&
               (parser->state != CTX_PARSER_NEGATIVE_NUMBER))
            {
              parser->n_numbers ++;
              ctx_parser_number_done (parser);

              if (parser->n_numbers == parser->expected_args ||
                  parser->expected_args == CTX_ARG_COLLECT_NUMBERS ||
                  parser->expected_args == CTX_ARG_STRING_OR_NUMBER)
                {
                  int tmp1 = parser->n_numbers;
                  int tmp2 = parser->n_args;
                  CtxCode tmp3 = parser->command;
                  int tmp4 = parser->expected_args;
                  ctx_parser_dispatch_command (parser);
                  parser->command = tmp3;
                  switch (parser->command)
                  {
                    case CTX_DEFINE_TEXTURE:
                    case CTX_TEXTURE:
                      parser->n_numbers = tmp1;
                      parser->n_args = tmp2;
                      break;
                          default:
                      parser->n_numbers = 0;
                      parser->n_args = 0;
                      break;
                  }
                  parser->expected_args = tmp4;
                }
              if (parser->n_numbers > CTX_PARSER_MAX_ARGS)
                { parser->n_numbers = CTX_PARSER_MAX_ARGS;
                }
            }
        }
        break;
      case CTX_PARSER_WORD:
        switch (byte)
          {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            case 8: case 11: case 12: case 14: case 15: case 16: case 17:
            case 18: case 19: case 20: case 21: case 22: case 23: case 24:
            case 25: case 26: case 27: case 28: case 29: case 30: case 31:
            case ' ': case '\t': case '\r': case '\n':
            case ';': case ',':
            case '(': case ')': case '=': case '{': case '}':
              parser->state = CTX_PARSER_NEUTRAL;
              break;
            case '#':
              parser->state = CTX_PARSER_COMMENT;
              break;
            case '-':
              parser->state = CTX_PARSER_NEGATIVE_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->decimal = 0;
              break;
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->numbers[parser->n_numbers] += (byte - '0');
              parser->decimal = 0;
              break;
            case '.':
              parser->state = CTX_PARSER_NUMBER;
              parser->numbers[parser->n_numbers] = 0;
              parser->decimal = 1;
              break;
            default:
              ctx_parser_holding_append (parser, byte);
              break;
          }
        if (parser->state != CTX_PARSER_WORD)
          {
            ctx_parser_word_done (parser);
          }
        break;
      case CTX_PARSER_STRING_A85:
        switch (byte)
          {
            case '~':
              parser->state = CTX_PARSER_NEUTRAL;
                 //   fprintf (stderr, "got %i\n", parser->pos);
              parser->pos = ctx_a85dec ((char*)parser->holding, (char*)parser->holding, parser->pos);
                 //   fprintf (stderr, "dec got %i\n", parser->pos);
              ctx_parser_string_done (parser);
              break;
            default:
              ctx_parser_holding_append (parser, byte);
              break;
          }
        break;
      case CTX_PARSER_STRING_APOS:
        switch (byte)
          {
            case '\\': parser->state = CTX_PARSER_STRING_APOS_ESCAPED; break;
            case '\'': parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser);
              break;
            default:
              ctx_parser_holding_append (parser, byte); break;
          }
        break;
      case CTX_PARSER_STRING_APOS_ESCAPED:
        switch (byte)
          {
            case '0': byte = '\0'; break;
            case 'b': byte = '\b'; break;
            case 'f': byte = '\f'; break;
            case 'n': byte = '\n'; break;
            case 'r': byte = '\r'; break;
            case 't': byte = '\t'; break;
            case 'v': byte = '\v'; break;
            default: break;
          }
        ctx_parser_holding_append (parser, byte);
        parser->state = CTX_PARSER_STRING_APOS;
        break;
      case CTX_PARSER_STRING_QUOT_ESCAPED:
        switch (byte)
          {
            case '0': byte = '\0'; break;
            case 'b': byte = '\b'; break;
            case 'f': byte = '\f'; break;
            case 'n': byte = '\n'; break;
            case 'r': byte = '\r'; break;
            case 't': byte = '\t'; break;
            case 'v': byte = '\v'; break;
            default: break;
          }
        ctx_parser_holding_append (parser, byte);
        parser->state = CTX_PARSER_STRING_QUOT;
        break;
      case CTX_PARSER_STRING_QUOT:
        switch (byte)
          {
            case '\\':
              parser->state = CTX_PARSER_STRING_QUOT_ESCAPED;
              break;
            case '"':
              parser->state = CTX_PARSER_NEUTRAL;
              ctx_parser_string_done (parser);
              break;
            default:
              ctx_parser_holding_append (parser, byte);
              break;
          }
        break;
      case CTX_PARSER_COMMENT:
        switch (byte)
          {
            case '\r':
            case '\n':
              parser->state = CTX_PARSER_NEUTRAL;
            default:
              break;
          }
        break;
    }
}

void ctx_parse (Ctx *ctx, const char *string)
{
  if (!string)
    return;
  CtxParser *parser = ctx_parser_new (ctx, ctx_width(ctx),
                                           ctx_height(ctx),
                                           ctx_get_font_size(ctx),
                                           ctx_get_font_size(ctx),
                                           0, 0, NULL, NULL, NULL, NULL, NULL);
  for (int i = 0; string[i]; i++)
     ctx_parser_feed_byte (parser, string[i]);
  ctx_parser_free (parser);
}

#endif

static CtxFont ctx_fonts[CTX_MAX_FONTS];
static int     ctx_font_count = 0;

#if CTX_FONT_ENGINE_STB
static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar);
static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB);
static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke);

CtxFontEngine ctx_font_engine_stb =
{
#if CTX_FONTS_FROM_FILE
  ctx_load_font_ttf_file,
#endif
  ctx_load_font_ttf,
  ctx_glyph_stb,
  ctx_glyph_width_stb,
  ctx_glyph_kern_stb,
};

int
ctx_load_font_ttf (const char *name, const void *ttf_contents, int length)
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }
  ctx_fonts[ctx_font_count].type = 1;
  ctx_fonts[ctx_font_count].name = (char *) malloc (strlen (name) + 1);
  ctx_strcpy ( (char *) ctx_fonts[ctx_font_count].name, name);
  if (!stbtt_InitFont (&ctx_fonts[ctx_font_count].stb.ttf_info, ttf_contents, 0) )
    {
      ctx_log ( "Font init failed\n");
      return -1;
    }
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_stb;
  ctx_font_count ++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ttf_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  ctx_get_contents (path, &contents, &length);
  if (!contents)
    {
      ctx_log ( "File load failed\n");
      return -1;
    }
  return ctx_load_font_ttf (name, contents, length);
}
#endif

static int
ctx_glyph_stb_find (CtxFont *font, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int index = font->stb.cache_index;
  if (font->stb.cache_unichar == unichar)
    {
      return index;
    }
  font->stb.cache_unichar = 0;
  index = font->stb.cache_index = stbtt_FindGlyphIndex (ttf_info, unichar);
  font->stb.cache_unichar = unichar;
  return index;
}

static float
ctx_glyph_width_stb (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size          = ctx->state.gstate.font_size;
  float scale              = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int advance, lsb;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    { return 0.0f; }
  stbtt_GetGlyphHMetrics (ttf_info, glyph, &advance, &lsb);
  return (advance * scale);
}

static float
ctx_glyph_kern_stb (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  float font_size = ctx->state.gstate.font_size;
  float scale = stbtt_ScaleForPixelHeight (ttf_info, font_size);
  int glyphA = ctx_glyph_stb_find (font, unicharA);
  int glyphB = ctx_glyph_stb_find (font, unicharB);
  return stbtt_GetGlyphKernAdvance (ttf_info, glyphA, glyphB) * scale;
}

static int
ctx_glyph_stb (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  stbtt_fontinfo *ttf_info = &font->stb.ttf_info;
  int glyph = ctx_glyph_stb_find (font, unichar);
  if (glyph==0)
    { return -1; }
  float font_size = ctx->state.gstate.font_size;
  int   baseline = ctx->state.y;
  float origin_x = ctx->state.x;
  float origin_y = baseline;
  float scale    = stbtt_ScaleForPixelHeight (ttf_info, font_size);;
  stbtt_vertex *vertices = NULL;
  ctx_begin_path (ctx);
  int num_verts = stbtt_GetGlyphShape (ttf_info, glyph, &vertices);
  for (int i = 0; i < num_verts; i++)
    {
      stbtt_vertex *vertex = &vertices[i];
      switch (vertex->type)
        {
          case STBTT_vmove:
            ctx_move_to (ctx,
                         origin_x + vertex->x * scale, origin_y - vertex->y * scale);
            break;
          case STBTT_vline:
            ctx_line_to (ctx,
                         origin_x + vertex->x * scale, origin_y - vertex->y * scale);
            break;
          case STBTT_vcubic:
            ctx_curve_to (ctx,
                          origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
                          origin_x + vertex->cx1 * scale, origin_y - vertex->cy1 * scale,
                          origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
            break;
          case STBTT_vcurve:
            ctx_quad_to (ctx,
                         origin_x + vertex->cx  * scale, origin_y - vertex->cy  * scale,
                         origin_x + vertex->x   * scale, origin_y - vertex->y   * scale);
            break;
        }
    }
  stbtt_FreeShape (ttf_info, vertices);
  if (stroke)
    {
      ctx_stroke (ctx);
    }
  else
    { ctx_fill (ctx); }
  return 0;
}
#endif

#if CTX_FONT_ENGINE_CTX

/* XXX: todo remove this, and rely on a binary search instead
 */
static int ctx_font_find_glyph_cached (CtxFont *font, uint32_t glyph)
{
  for (int i = 0; i < font->ctx.glyphs; i++)
    {
      if (font->ctx.index[i * 2] == glyph)
        { return font->ctx.index[i * 2 + 1]; }
    }
  return -1;
}

static int ctx_glyph_find_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  int ret = ctx_font_find_glyph_cached (font, unichar);
  if (ret >= 0) return ret;

  for (int i = 0; i < font->ctx.length; i++)
  {
    CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
    if (entry->code == CTX_DEFINE_GLYPH &&
        entry->data.u32[0] == unichar)
    {
       return i;
       // XXX this could be prone to insertion of valid header
       // data in included bitmaps.. is that an issue?
       //   
    }
  }
  return -1;
}


static float
ctx_glyph_kern_ctx (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
  float font_size = ctx->state.gstate.font_size;
  int first_kern = ctx_glyph_find_ctx (font, ctx, unicharA);
  if (first_kern < 0) return 0.0;
  for (int i = first_kern + 1; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE; }
        }
      if (entry->code == CTX_DEFINE_GLYPH)
        return 0.0;
    }
  return 0.0;
}
#if 0
static int ctx_glyph_find (Ctx *ctx, CtxFont *font, uint32_t unichar)
{
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        { return i; }
    }
  return 0;
}
#endif


static float
ctx_glyph_width_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  CtxState *state = &ctx->state;
  float font_size = state->gstate.font_size;
  int   start     = ctx_glyph_find_ctx (font, ctx, unichar);
  if (start < 0)
    { return 0.0; }  // XXX : fallback
  for (int i = start; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        if (entry->data.u32[0] == (unsigned) unichar)
          { return (entry->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE); }
    }
  return 0.0;
}

static int
ctx_glyph_drawlist (CtxFont *font, Ctx *ctx, CtxDrawlist *drawlist, uint32_t unichar, int stroke)
{
  CtxState *state = &ctx->state;
  CtxIterator iterator;
  float origin_x = state->x;
  float origin_y = state->y;
  ctx_current_point (ctx, &origin_x, &origin_y);
  int in_glyph = 0;
  float font_size = state->gstate.font_size;
  int start = 0;
  if (font->type == 0)
  {
  start = ctx_glyph_find_ctx (font, ctx, unichar);
  if (start < 0)
    { return -1; }  // XXX : fallback glyph
  }
  ctx_iterator_init (&iterator, drawlist, start, CTX_ITERATOR_EXPAND_BITPACK);
  CtxCommand *command;

  /* XXX :  do a binary search instead of a linear search */
  while ( (command= ctx_iterator_next (&iterator) ) )
    {
      CtxEntry *entry = &command->entry;
      if (in_glyph)
        {
          if (entry->code == CTX_DEFINE_GLYPH)
            {
              if (stroke)
                { ctx_stroke (ctx); }
              else
                {
#if CTX_RASTERIZER
#if CTX_ENABLE_SHADOW_BLUR
      if (ctx->renderer && ((CtxRasterizer*)(ctx->renderer))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->renderer);
        ((CtxRasterizer*)(ctx->renderer))->in_shadow = 1;
      }
      else
#endif
#endif
         ctx_fill (ctx); 
               
                }
              ctx_restore (ctx);
              return 0;
            }
          ctx_process (ctx, entry);
        }
      else if (entry->code == CTX_DEFINE_GLYPH && entry->data.u32[0] == unichar)
        {
          in_glyph = 1;
          ctx_save (ctx);
          ctx_translate (ctx, origin_x, origin_y);
          ctx_move_to (ctx, 0, 0);
          ctx_begin_path (ctx);
          ctx_scale (ctx, font_size / CTX_BAKE_FONT_SIZE,
                     font_size / CTX_BAKE_FONT_SIZE);
        }
    }
  if (stroke)
    { ctx_stroke (ctx);
    }
  else
    { 
    
#if CTX_RASTERIZER
#if CTX_ENABLE_SHADOW_BLUR
      if (ctx->renderer && ((CtxRasterizer*)(ctx->renderer))->in_shadow)
      {
        ctx_rasterizer_shadow_fill ((CtxRasterizer*)ctx->renderer);
        ((CtxRasterizer*)(ctx->renderer))->in_shadow = 1;
      }
      else
#endif
#endif
      {
         ctx_fill (ctx); 
      }
    }
  ctx_restore (ctx);
  return -1;
}

static int
ctx_glyph_ctx (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxDrawlist drawlist = { (CtxEntry *) font->ctx.data,
                           font->ctx.length,
                           font->ctx.length, 0, 0
                         };
  return ctx_glyph_drawlist (font, ctx, &drawlist, unichar, stroke);
}

uint32_t ctx_glyph_no (Ctx *ctx, int no)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  if (no < 0 || no >= font->ctx.glyphs)
    { return 0; }
  return font->ctx.index[no*2];
}

static void ctx_font_init_ctx (CtxFont *font)
{
  int glyph_count = 0;
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        { glyph_count ++; }
    }
  font->ctx.glyphs = glyph_count;
#if CTX_DRAWLIST_STATIC
  static uint32_t idx[512]; // one might have to adjust this for
  // larger fonts XXX
  // should probably be made a #define
  font->ctx.index = &idx[0];
#else
  font->ctx.index = (uint32_t *) malloc (sizeof (uint32_t) * 2 * glyph_count);
#endif
  int no = 0;
  for (int i = 0; i < font->ctx.length; i++)
    {
      CtxEntry *entry = &font->ctx.data[i];
      if (entry->code == CTX_DEFINE_GLYPH)
        {
          font->ctx.index[no*2]   = entry->data.u32[0];
          font->ctx.index[no*2+1] = i;
          no++;
        }
    }
}

int
ctx_load_font_ctx (const char *name, const void *data, int length);
#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path);
#endif

static CtxFontEngine ctx_font_engine_ctx =
{
#if CTX_FONTS_FROM_FILE
  ctx_load_font_ctx_file,
#endif
  ctx_load_font_ctx,
  ctx_glyph_ctx,
  ctx_glyph_width_ctx,
  ctx_glyph_kern_ctx,
};

int
ctx_load_font_ctx (const char *name, const void *data, int length)
{
  if (length % sizeof (CtxEntry) )
    { return -1; }
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }
  ctx_fonts[ctx_font_count].type = 0;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx.data = (CtxEntry *) data;
  ctx_fonts[ctx_font_count].ctx.length = length / sizeof (CtxEntry);
  ctx_font_init_ctx (&ctx_fonts[ctx_font_count]);
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx;
  ctx_font_count++;
  return ctx_font_count-1;
}

#if CTX_FONTS_FROM_FILE
int
ctx_load_font_ctx_file (const char *name, const char *path)
{
  uint8_t *contents = NULL;
  long length = 0;
  ctx_get_contents (path, &contents, &length);
  if (!contents)
    {
      ctx_log ( "File load failed\n");
      return -1;
    }
  return ctx_load_font_ctx (name, contents, length);
}
#endif
#endif

#if CTX_FONT_ENGINE_CTX_FS

static float
ctx_glyph_kern_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unicharA, uint32_t unicharB)
{
#if 0
  float font_size = ctx->state.gstate.font_size;
  int first_kern = ctx_glyph_find_ctx (font, ctx, unicharA);
  if (first_kern < 0) return 0.0;
  for (int i = first_kern + 1; i < font->ctx.length; i++)
    {
      CtxEntry *entry = (CtxEntry *) &font->ctx.data[i];
      if (entry->code == CTX_KERNING_PAIR)
        {
          if (entry->data.u16[0] == unicharA && entry->data.u16[1] == unicharB)
            { return entry->data.s32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE; }
        }
      if (entry->code == CTX_DEFINE_GLYPH)
        return 0.0;
    }
#endif
  return 0.0;
}

static float
ctx_glyph_width_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unichar)
{
  CtxState *state = &ctx->state;
  char path[1024];
  sprintf (path, "%s/%010p", font->ctx_fs.path, unichar);
  uint8_t *data = NULL;
  long int len_bytes = 0;
  ctx_get_contents (path, &data, &len_bytes);
  float ret = 0.0;
  float font_size = state->gstate.font_size;
  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    for (int i = 0; i < glyph_ctx->drawlist.count; i++)
    {
      CtxEntry *e = &glyph_ctx->drawlist.entries[i];
      if (e->code == CTX_DEFINE_GLYPH)
        ret = e->data.u32[1] / 255.0 * font_size / CTX_BAKE_FONT_SIZE;
    }
    free (data);
    ctx_free (glyph_ctx);
  }
  return ret;
}

static int
ctx_glyph_ctx_fs (CtxFont *font, Ctx *ctx, uint32_t unichar, int stroke)
{
  char path[1024];
  sprintf (path, "file://%s/%010p", font->ctx_fs.path, unichar);
  uint8_t *data = NULL;
  long int len_bytes = 0;
  ctx_get_contents (path, &data, &len_bytes);

  if (data){
    Ctx *glyph_ctx = ctx_new ();
    ctx_parse (glyph_ctx, data);
    int ret = ctx_glyph_drawlist (font, ctx, &(glyph_ctx->drawlist),
                                  unichar, stroke);
    free (data);
    ctx_free (glyph_ctx);
    return ret;
  }
  return -1;
}

int
ctx_load_font_ctx_fs (const char *name, const void *data, int length);

static CtxFontEngine ctx_font_engine_ctx_fs =
{
#if CTX_FONTS_FROM_FILE
  NULL,
#endif
  ctx_load_font_ctx_fs,
  ctx_glyph_ctx_fs,
  ctx_glyph_width_ctx_fs,
  ctx_glyph_kern_ctx_fs,
};

int
ctx_load_font_ctx_fs (const char *name, const void *path, int length) // length is ignored
{
  if (ctx_font_count >= CTX_MAX_FONTS)
    { return -1; }

  ctx_fonts[ctx_font_count].type = 42;
  ctx_fonts[ctx_font_count].name = name;
  ctx_fonts[ctx_font_count].ctx_fs.path = strdup (path);
  int path_len = strlen (path);
  if (ctx_fonts[ctx_font_count].ctx_fs.path[path_len-1] == '/')
   ctx_fonts[ctx_font_count].ctx_fs.path[path_len-1] = 0;
  ctx_fonts[ctx_font_count].engine = &ctx_font_engine_ctx_fs;
  ctx_font_count++;
  return ctx_font_count-1;
}

#endif

int
_ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  // a begin-path here did not remove stray spikes in terminal
  return font->engine->glyph (font, ctx, unichar, stroke);
}

int
ctx_glyph (Ctx *ctx, uint32_t unichar, int stroke)
{
#if CTX_BACKEND_TEXT
  CtxEntry commands[3]; // 3 to silence incorrect warning from static analysis
  ctx_memset (commands, 0, sizeof (commands) );
  commands[0] = ctx_u32 (CTX_GLYPH, unichar, 0);
  commands[0].data.u8[4] = stroke;
  ctx_process (ctx, commands);
  return 0; // XXX is return value used?
#else
  return _ctx_glyph (ctx, unichar, stroke);
#endif
}

float
ctx_glyph_width (Ctx *ctx, int unichar)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];

  return font->engine->glyph_width (font, ctx, unichar);
}

static float
ctx_glyph_kern (Ctx *ctx, int unicharA, int unicharB)
{
  CtxFont *font = &ctx_fonts[ctx->state.gstate.font];
  return font->engine->glyph_kern (font, ctx, unicharA, unicharB);
}

float
ctx_text_width (Ctx        *ctx,
                const char *string)
{
  float sum = 0.0;
  if (!string)
    return 0.0f;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      sum += ctx_glyph_width (ctx, ctx_utf8_to_unichar (utf8) );
    }
  return sum;
}

static void
_ctx_glyphs (Ctx     *ctx,
             CtxGlyph *glyphs,
             int       n_glyphs,
             int       stroke)
{
  for (int i = 0; i < n_glyphs; i++)
    {
      {
        uint32_t unichar = glyphs[i].index;
        ctx_move_to (ctx, glyphs[i].x, glyphs[i].y);
        ctx_glyph (ctx, unichar, stroke);
      }
    }
}

static void
_ctx_text (Ctx        *ctx,
           const char *string,
           int         stroke,
           int         visible)
{
  CtxState *state = &ctx->state;
  float x = ctx->state.x;
  switch ( (int) ctx_state_get (state, CTX_text_align) )
    //switch (state->gstate.text_align)
    {
      case CTX_TEXT_ALIGN_START:
      case CTX_TEXT_ALIGN_LEFT:
        break;
      case CTX_TEXT_ALIGN_CENTER:
        x -= ctx_text_width (ctx, string) /2;
        break;
      case CTX_TEXT_ALIGN_END:
      case CTX_TEXT_ALIGN_RIGHT:
        x -= ctx_text_width (ctx, string);
        break;
    }
  float y = ctx->state.y;
  float baseline_offset = 0.0f;
  switch ( (int) ctx_state_get (state, CTX_text_baseline) )
    {
      case CTX_TEXT_BASELINE_HANGING:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.55;
        break;
      case CTX_TEXT_BASELINE_TOP:
        /* XXX : crude */
        baseline_offset = ctx->state.gstate.font_size  * 0.7;
        break;
      case CTX_TEXT_BASELINE_BOTTOM:
        baseline_offset = -ctx->state.gstate.font_size * 0.1;
        break;
      case CTX_TEXT_BASELINE_ALPHABETIC:
      case CTX_TEXT_BASELINE_IDEOGRAPHIC:
        baseline_offset = 0.0f;
        break;
      case CTX_TEXT_BASELINE_MIDDLE:
        baseline_offset = ctx->state.gstate.font_size * 0.25;
        break;
    }
  float x0 = x;
  for (const char *utf8 = string; *utf8; utf8 = ctx_utf8_skip (utf8, 1) )
    {
      if (*utf8 == '\n')
        {
          y += ctx->state.gstate.font_size * ctx_state_get (state, CTX_line_spacing);
          x = x0;
          if (visible)
            { ctx_move_to (ctx, x, y); }
        }
      else
        {
          uint32_t unichar = ctx_utf8_to_unichar (utf8);
          if (visible)
            {
              ctx_move_to (ctx, x, y + baseline_offset);
              _ctx_glyph (ctx, unichar, stroke);
            }
          const char *next_utf8 = ctx_utf8_skip (utf8, 1);
          if (next_utf8)
            {
              x += ctx_glyph_width (ctx, unichar);
              x += ctx_glyph_kern (ctx, unichar, ctx_utf8_to_unichar (next_utf8) );
            }
          if (visible)
            { ctx_move_to (ctx, x, y); }
        }
    }
  if (!visible)
    { ctx_move_to (ctx, x, y); }
}


CtxGlyph *
ctx_glyph_allocate (int n_glyphs)
{
  return (CtxGlyph *) malloc (sizeof (CtxGlyph) * n_glyphs);
}
void
gtx_glyph_free     (CtxGlyph *glyphs)
{
  free (glyphs);
}

void
ctx_glyphs (Ctx        *ctx,
            CtxGlyph   *glyphs,
            int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 0);
}

void
ctx_glyphs_stroke (Ctx        *ctx,
                   CtxGlyph   *glyphs,
                   int         n_glyphs)
{
  _ctx_glyphs (ctx, glyphs, n_glyphs, 1);
}

void
ctx_text (Ctx        *ctx,
          const char *string)
{
  if (!string)
    return;
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_TEXT, string, 0, 0);
  _ctx_text (ctx, string, 0, 0);
#else
  _ctx_text (ctx, string, 0, 1);
#endif
}


void
ctx_fill_text (Ctx *ctx, const char *string,
               float x, float y)
{
  ctx_move_to (ctx, x, y);
  ctx_text (ctx, string);
}

void
ctx_text_stroke (Ctx        *ctx,
                 const char *string)
{
  if (!string)
    return;
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_STROKE_TEXT, string, 0, 0);
  _ctx_text (ctx, string, 1, 0);
#else
  _ctx_text (ctx, string, 1, 1);
#endif
}

void
ctx_stroke_text (Ctx *ctx, const char *string,
               float x, float y)
{
  ctx_move_to (ctx, x, y);
  ctx_text_stroke (ctx, string);
}

static int _ctx_resolve_font (const char *name)
{
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (!ctx_strcmp (ctx_fonts[i].name, name) )
        { return i; }
    }
  for (int i = 0; i < ctx_font_count; i ++)
    {
      if (ctx_strstr (ctx_fonts[i].name, name) )
        { return i; }
    }
  return -1;
}

int ctx_resolve_font (const char *name)
{
  int ret = _ctx_resolve_font (name);
  if (ret >= 0)
    { return ret; }
  if (!ctx_strcmp (name, "regular") )
    {
      int ret = _ctx_resolve_font ("sans");
      if (ret >= 0) { return ret; }
      ret = _ctx_resolve_font ("serif");
      if (ret >= 0) { return ret; }
    }
  return 0;
}

static void ctx_font_setup ()
{
  static int initialized = 0;
  if (initialized) { return; }
  initialized = 1;
#if CTX_FONT_ENGINE_CTX
  ctx_font_count = 0; // oddly - this is needed in arduino

#if CTX_FONT_ENGINE_CTX_FS
  ctx_load_font_ctx_fs ("sans-ctx", "/tmp/ctx-regular", 0);
#else
#if CTX_FONT_ascii
  ctx_load_font_ctx ("sans-ctx", ctx_font_ascii, sizeof (ctx_font_ascii) );
#endif
#if CTX_FONT_regular
  ctx_load_font_ctx ("sans-ctx", ctx_font_regular, sizeof (ctx_font_regular) );
#endif
#endif

#if CTX_FONT_mono
  ctx_load_font_ctx ("mono-ctx", ctx_font_mono, sizeof (ctx_font_mono) );
#endif
#if CTX_FONT_bold
  ctx_load_font_ctx ("bold-ctx", ctx_font_bold, sizeof (ctx_font_bold) );
#endif
#if CTX_FONT_italic
  ctx_load_font_ctx ("italic-ctx", ctx_font_italic, sizeof (ctx_font_italic) );
#endif
#if CTX_FONT_sans
  ctx_load_font_ctx ("sans-ctx", ctx_font_sans, sizeof (ctx_font_sans) );
#endif
#if CTX_FONT_serif
  ctx_load_font_ctx ("serif-ctx", ctx_font_serif, sizeof (ctx_font_serif) );
#endif
#if CTX_FONT_symbol
  ctx_load_font_ctx ("symbol-ctx", ctx_font_symbol, sizeof (ctx_font_symbol) );
#endif
#if CTX_FONT_emoji
  ctx_load_font_ctx ("emoji-ctx", ctx_font_emoji, sizeof (ctx_font_emoji) );
#endif
#endif

#if NOTO_EMOJI_REGULAR
  ctx_load_font_ttf ("sans-NotoEmoji_Regular", ttf_NotoEmoji_Regular_ttf, ttf_NotoEmoji_Regular_ttf_len);
#endif
#if ROBOTO_LIGHT
  ctx_load_font_ttf ("sans-light-Roboto_Light", ttf_Roboto_Light_ttf, ttf_Roboto_Light_ttf_len);
#endif
#if ROBOTO_REGULAR
  ctx_load_font_ttf ("sans-Roboto_Regular", ttf_Roboto_Regular_ttf, ttf_Roboto_Regular_ttf_len);
#endif
#if ROBOTO_BOLD
  ctx_load_font_ttf ("sans-bold-Roboto_Bold", ttf_Roboto_Bold_ttf, ttf_Roboto_Bold_ttf_len);
#endif
#if DEJAVU_SANS
  ctx_load_font_ttf ("sans-DejaVuSans", ttf_DejaVuSans_ttf, ttf_DejaVuSans_ttf_len);
#endif
#if VERA
  ctx_load_font_ttf ("sans-Vera", ttf_Vera_ttf, ttf_Vera_ttf_len);
#endif
#if UNSCII_16
  ctx_load_font_ttf ("mono-unscii16", ttf_unscii_16_ttf, ttf_unscii_16_ttf_len);
#endif
#if XA000_MONO
  ctx_load_font_ttf ("mono-0xA000", ttf_0xA000_Mono_ttf, ttf_0xA000_Mono_ttf_len);
#endif
#if DEJAVU_SANS_MONO
  ctx_load_font_ttf ("mono-DejaVuSansMono", ttf_DejaVuSansMono_ttf, ttf_DejaVuSansMono_ttf_len);
#endif
#if NOTO_MONO_REGULAR
  ctx_load_font_ttf ("mono-NotoMono_Regular", ttf_NotoMono_Regular_ttf, ttf_NotoMono_Regular_ttf_len);
#endif
}


#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#if !__COSMOPOLITAN__
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

//#include "ctx.h"
/* instead of including ctx.h we declare the few utf8
 * functions we use
 */
uint32_t ctx_utf8_to_unichar (const char *input);
int ctx_unichar_to_utf8 (uint32_t  ch, uint8_t  *dest);
int ctx_utf8_strlen (const char *s);


void ctx_string_init (CtxString *string, int initial_size)
{
  string->allocated_length = initial_size;
  string->length = 0;
  string->utf8_length = 0;
  string->str = (char*)malloc (string->allocated_length + 1);
  string->str[0]='\0';
}

static void ctx_string_destroy (CtxString *string)
{
  if (string->str)
    {
      free (string->str);
      string->str = NULL;
    }
}

void ctx_string_clear (CtxString *string)
{
  string->length = 0;
  string->utf8_length = 0;
  string->str[string->length]=0;
}

static inline void _ctx_string_append_byte (CtxString *string, char  val)
{
  if ( (val & 0xC0) != 0x80)
    { string->utf8_length++; }
  if (string->length + 2 >= string->allocated_length)
    {
      char *old = string->str;
      string->allocated_length = CTX_MAX (string->allocated_length * 2, string->length + 2);
      string->str = (char*)realloc (old, string->allocated_length);
    }
  string->str[string->length++] = val;
  string->str[string->length] = '\0';
}

void ctx_string_append_byte (CtxString *string, char  val)
{
  _ctx_string_append_byte (string, val);
}

void ctx_string_append_unichar (CtxString *string, unsigned int unichar)
{
  char *str;
  char utf8[5];
  utf8[ctx_unichar_to_utf8 (unichar, (unsigned char *) utf8)]=0;
  str = utf8;
  while (str && *str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

static inline void _ctx_string_append_str (CtxString *string, const char *str)
{
  if (!str) { return; }
  while (*str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

void ctx_string_append_utf8char (CtxString *string, const char *str)
{
  if (!str) { return; }
  int len = ctx_utf8_len (*str);
  for (int i = 0; i < len && *str; i++)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

void ctx_string_append_str (CtxString *string, const char *str)
{
  _ctx_string_append_str (string, str);
}

CtxString *ctx_string_new_with_size (const char *initial, int initial_size)
{
  CtxString *string = (CtxString*)ctx_calloc (sizeof (CtxString), 1);
  ctx_string_init (string, initial_size);
  if (initial)
    { _ctx_string_append_str (string, initial); }
  return string;
}

CtxString *ctx_string_new (const char *initial)
{
  return ctx_string_new_with_size (initial, 8);
}

void ctx_string_append_data (CtxString *string, const char *str, int len)
{
  int i;
  for (i = 0; i<len; i++)
    { _ctx_string_append_byte (string, str[i]); }
}

void ctx_string_append_string (CtxString *string, CtxString *string2)
{
  const char *str = ctx_string_get (string2);
  while (str && *str)
    {
      _ctx_string_append_byte (string, *str);
      str++;
    }
}

const char *ctx_string_get (CtxString *string)
{
  return string->str;
}

int ctx_string_get_utf8length (CtxString *string)
{
  return string->utf8_length;
}

int ctx_string_get_length (CtxString *string)
{
  return string->length;
}

void
ctx_string_free (CtxString *string, int freealloc)
{
  if (freealloc)
    {
      ctx_string_destroy (string);
    }
#if 0
  if (string->is_line)
  {
    VtLine *line = (VtLine*)string;
    if (line->style)
      { free (line->style); }
    if (line->ctx)
      { ctx_free (line->ctx); }
    if (line->ctx_copy)
      { ctx_free (line->ctx_copy); }
  }
#endif
  free (string);
}

void
ctx_string_set (CtxString *string, const char *new_string)
{
  ctx_string_clear (string);
  _ctx_string_append_str (string, new_string);
}

static char *ctx_strdup (const char *str)
{
  int len = strlen (str);
  char *ret = (char*)malloc (len + 1);
  memcpy (ret, str, len);
  ret[len]=0;
  return ret;
}

void ctx_string_replace_utf8 (CtxString *string, int pos, const char *new_glyph)
{
  int new_len = ctx_utf8_len (*new_glyph);
#if 1
  int old_len = string->utf8_length;
#else
  int old_len = ctx_utf8_strlen (string->str);// string->utf8_length;
#endif
  char tmpg[3]=" ";
  if (pos == old_len)
    {
      _ctx_string_append_str (string, new_glyph);
      return;
    }
  if (new_len <= 1 && new_glyph[0] < 32)
    {
      new_len = 1;
      tmpg[0]=new_glyph[0]+64;
      new_glyph = tmpg;
    }
  {
    for (int i = old_len; i <= pos + 2; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  if (string->length + new_len  >= string->allocated_length - 2)
    {
      char *tmp;
      char *defer;
      string->allocated_length = string->length + new_len + 2;
      tmp = (char*) ctx_calloc (string->allocated_length + 1 + 8, 1);
      strcpy (tmp, string->str);
      defer = string->str;
      string->str = tmp;
      free (defer);
    }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (*p == 0 || * (p+prev_len) == 0)
    {
      rest = ctx_strdup ("");
    }
  else
    {
      if (p + prev_len >= string->length  + string->str)
        { rest = ctx_strdup (""); }
      else
        { rest = ctx_strdup (p + prev_len); }
    }
  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  string->length += new_len;
  string->length -= prev_len;
  free (rest);
  //string->length = strlen (string->str);
  //string->utf8_length = ctx_utf8_strlen (string->str);
}

void ctx_string_replace_unichar (CtxString *string, int pos, uint32_t unichar)
{
  uint8_t utf8[8];
  ctx_unichar_to_utf8 (unichar, utf8);
  ctx_string_replace_utf8 (string, pos, (char *) utf8);
}

uint32_t ctx_string_get_unichar (CtxString *string, int pos)
{
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  if (!p)
    { return 0; }
  return ctx_utf8_to_unichar (p);
}


void ctx_string_insert_utf8 (CtxString *string, int pos, const char *new_glyph)
{
  int new_len = ctx_utf8_len (*new_glyph);
  int old_len = string->utf8_length;
  char tmpg[3]=" ";
  if (old_len == pos && 0)
    {
      ctx_string_append_str (string, new_glyph);
      return;
    }
  if (new_len <= 1 && new_glyph[0] < 32)
    {
      tmpg[0]=new_glyph[0]+64;
      new_glyph = tmpg;
    }
  {
    for (int i = old_len; i <= pos; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  if (string->length + new_len + 1  > string->allocated_length)
    {
      char *tmp;
      char *defer;
      string->allocated_length = string->length + new_len + 1;
      tmp = (char*) ctx_calloc (string->allocated_length + 1, 1);
      strcpy (tmp, string->str);
      defer = string->str;
      string->str = tmp;
      free (defer);
    }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if ( (*p == 0 || * (p+prev_len) == 0) && pos != 0)
    {
      rest = ctx_strdup ("");
    }
  else
    {
      rest = ctx_strdup (p);
    }
  memcpy (p, new_glyph, new_len);
  memcpy (p + new_len, rest, strlen (rest) + 1);
  free (rest);
  string->length = strlen (string->str);
  string->utf8_length = ctx_utf8_strlen (string->str);
}

void ctx_string_insert_unichar (CtxString *string, int pos, uint32_t unichar)
{
  uint8_t utf8[5]="";
  utf8[ctx_unichar_to_utf8(unichar, utf8)]=0;
  ctx_string_insert_utf8 (string, pos, (char*)utf8);
}

void ctx_string_remove (CtxString *string, int pos)
{
  int old_len = string->utf8_length;
  {
    for (int i = old_len; i <= pos; i++)
      {
        _ctx_string_append_byte (string, ' ');
        old_len++;
      }
  }
  char *p = (char *) ctx_utf8_skip (string->str, pos);
  int prev_len = ctx_utf8_len (*p);
  char *rest;
  if (!p || *p == 0)
    {
      return;
      rest = ctx_strdup ("");
      prev_len = 0;
    }
  else if (* (p+prev_len) == 0)
  {
      rest = ctx_strdup ("");
  }
  else
    {
      rest = ctx_strdup (p + prev_len);
    }
  strcpy (p, rest);
  string->str[string->length - prev_len] = 0;
  free (rest);
  string->length = strlen (string->str);
  string->utf8_length = ctx_utf8_strlen (string->str);
}

char *ctx_strdup_printf (const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  return buffer;
}

void ctx_string_append_printf (CtxString *string, const char *format, ...)
{
  va_list ap;
  size_t needed;
  char *buffer;
  va_start (ap, format);
  needed = vsnprintf (NULL, 0, format, ap) + 1;
  buffer = (char*)malloc (needed);
  va_end (ap);
  va_start (ap, format);
  vsnprintf (buffer, needed, format, ap);
  va_end (ap);
  ctx_string_append_str (string, buffer);
  free (buffer);
}

#if CTX_CAIRO

typedef struct _CtxCairo CtxCairo;
struct
  _CtxCairo
{
  CtxImplementation vfuncs;
  Ctx              *ctx;
  cairo_t          *cr;
  cairo_pattern_t  *pat;
  cairo_surface_t  *image;
  int               preserve;
};

static void
ctx_cairo_process (CtxCairo *ctx_cairo, CtxCommand *c)
{
  CtxEntry *entry = (CtxEntry *) &c->entry;
  cairo_t *cr = ctx_cairo->cr;
  switch (entry->code)
    {
      case CTX_LINE_TO:
        cairo_line_to (cr, c->line_to.x, c->line_to.y);
        break;
      case CTX_REL_LINE_TO:
        cairo_rel_line_to (cr, c->rel_line_to.x, c->rel_line_to.y);
        break;
      case CTX_MOVE_TO:
        cairo_move_to (cr, c->move_to.x, c->move_to.y);
        break;
      case CTX_REL_MOVE_TO:
        cairo_rel_move_to (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_CURVE_TO:
        cairo_curve_to (cr, ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_REL_CURVE_TO:
        cairo_rel_curve_to (cr,ctx_arg_float (0), ctx_arg_float (1),
                            ctx_arg_float (2), ctx_arg_float (3),
                            ctx_arg_float (4), ctx_arg_float (5) );
        break;
      case CTX_PRESERVE:
        ctx_cairo->preserve = 1;
        break;
      case CTX_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float (0);
          float cy = ctx_arg_float (1);
          float  x = ctx_arg_float (2);
          float  y = ctx_arg_float (3);
          cairo_curve_to (cr,
                          (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
                          (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                          x,                              y);
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          double x0, y0;
          cairo_get_current_point (cr, &x0, &y0);
          float cx = ctx_arg_float (0) + x0;
          float cy = ctx_arg_float (1) + y0;
          float  x = ctx_arg_float (2) + x0;
          float  y = ctx_arg_float (3) + y0;
          cairo_curve_to (cr,
                          (cx * 2 + x0) / 3.0f, (cy * 2 + y0) / 3.0f,
                          (cx * 2 + x) / 3.0f,           (cy * 2 + y) / 3.0f,
                          x,                              y);
        }
        break;
      /* rotate/scale/translate does not occur in fully minified data stream */
      case CTX_ROTATE:
        cairo_rotate (cr, ctx_arg_float (0) );
        break;
      case CTX_SCALE:
        cairo_scale (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_TRANSLATE:
        cairo_translate (cr, ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_LINE_WIDTH:
        cairo_set_line_width (cr, ctx_arg_float (0) );
        break;
      case CTX_ARC:
#if 0
        fprintf (stderr, "F %2.1f %2.1f %2.1f %2.1f %2.1f %2.1f\n",
                        ctx_arg_float(0),
                        ctx_arg_float(1),
                        ctx_arg_float(2),
                        ctx_arg_float(3),
                        ctx_arg_float(4),
                        ctx_arg_float(5),
                        ctx_arg_float(6));
#endif
        if (ctx_arg_float (5) == 1)
          cairo_arc (cr, ctx_arg_float (0), ctx_arg_float (1),
                     ctx_arg_float (2), ctx_arg_float (3),
                     ctx_arg_float (4) );
        else
          cairo_arc_negative (cr, ctx_arg_float (0), ctx_arg_float (1),
                              ctx_arg_float (2), ctx_arg_float (3),
                              ctx_arg_float (4) );
        break;
      case CTX_SET_RGBA_U8:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        break;
#if 0
      case CTX_SET_RGBA_STROKE: // XXX : we need to maintain
        //       state for the two kinds
        cairo_set_source_rgba (cr, ctx_arg_u8 (0) /255.0,
                               ctx_arg_u8 (1) /255.0,
                               ctx_arg_u8 (2) /255.0,
                               ctx_arg_u8 (3) /255.0);
        break;
#endif
      case CTX_RECTANGLE:
      case CTX_ROUND_RECTANGLE: // XXX - arcs
        cairo_rectangle (cr, c->rectangle.x, c->rectangle.y,
                         c->rectangle.width, c->rectangle.height);
        break;
      case CTX_SET_PIXEL:
        cairo_set_source_rgba (cr, ctx_u8_to_float (ctx_arg_u8 (0) ),
                               ctx_u8_to_float (ctx_arg_u8 (1) ),
                               ctx_u8_to_float (ctx_arg_u8 (2) ),
                               ctx_u8_to_float (ctx_arg_u8 (3) ) );
        cairo_rectangle (cr, ctx_arg_u16 (2), ctx_arg_u16 (3), 1, 1);
        cairo_fill (cr);
        break;
      case CTX_FILL:
        if (ctx_cairo->preserve)
        {
          cairo_fill_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_fill (cr);
        }
        break;
      case CTX_STROKE:
        if (ctx_cairo->preserve)
        {
          cairo_stroke_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_stroke (cr);
        }
        break;
      case CTX_IDENTITY:
        cairo_identity_matrix (cr);
        break;
      case CTX_CLIP:
        if (ctx_cairo->preserve)
        {
          cairo_clip_preserve (cr);
          ctx_cairo->preserve = 0;
        }
        else
        {
          cairo_clip (cr);
        }
        break;
        break;
      case CTX_BEGIN_PATH:
        cairo_new_path (cr);
        break;
      case CTX_CLOSE_PATH:
        cairo_close_path (cr);
        break;
      case CTX_SAVE:
        cairo_save (cr);
        break;
      case CTX_RESTORE:
        cairo_restore (cr);
        break;
      case CTX_FONT_SIZE:
        cairo_set_font_size (cr, ctx_arg_float (0) );
        break;
      case CTX_MITER_LIMIT:
        cairo_set_miter_limit (cr, ctx_arg_float (0) );
        break;
      case CTX_LINE_CAP:
        {
          int cairo_val = CAIRO_LINE_CAP_SQUARE;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_CAP_ROUND:
                cairo_val = CAIRO_LINE_CAP_ROUND;
                break;
              case CTX_CAP_SQUARE:
                cairo_val = CAIRO_LINE_CAP_SQUARE;
                break;
              case CTX_CAP_NONE:
                cairo_val = CAIRO_LINE_CAP_BUTT;
                break;
            }
          cairo_set_line_cap (cr, cairo_val);
        }
        break;
      case CTX_BLEND_MODE:
        {
          // does not map to cairo
        }
        break;
      case CTX_COMPOSITING_MODE:
        {
          int cairo_val = CAIRO_OPERATOR_OVER;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_COMPOSITE_SOURCE_OVER:
                cairo_val = CAIRO_OPERATOR_OVER;
                break;
              case CTX_COMPOSITE_COPY:
                cairo_val = CAIRO_OPERATOR_SOURCE;
                break;
            }
          cairo_set_operator (cr, cairo_val);
        }
      case CTX_LINE_JOIN:
        {
          int cairo_val = CAIRO_LINE_JOIN_ROUND;
          switch (ctx_arg_u8 (0) )
            {
              case CTX_JOIN_ROUND:
                cairo_val = CAIRO_LINE_JOIN_ROUND;
                break;
              case CTX_JOIN_BEVEL:
                cairo_val = CAIRO_LINE_JOIN_BEVEL;
                break;
              case CTX_JOIN_MITER:
                cairo_val = CAIRO_LINE_JOIN_MITER;
                break;
            }
          cairo_set_line_join (cr, cairo_val);
        }
        break;
      case CTX_LINEAR_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_linear (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3) );
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 0, 0, 0, 0, 1);
          cairo_pattern_add_color_stop_rgba (ctx_cairo->pat, 1, 1, 1, 1, 1);
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          if (ctx_cairo->pat)
            {
              cairo_pattern_destroy (ctx_cairo->pat);
              ctx_cairo->pat = NULL;
            }
          ctx_cairo->pat = cairo_pattern_create_radial (ctx_arg_float (0), ctx_arg_float (1),
                           ctx_arg_float (2), ctx_arg_float (3),
                           ctx_arg_float (4), ctx_arg_float (5) );
          cairo_set_source (cr, ctx_cairo->pat);
        }
        break;
      case CTX_GRADIENT_STOP:
        cairo_pattern_add_color_stop_rgba (ctx_cairo->pat,
                                           ctx_arg_float (0),
                                           ctx_u8_to_float (ctx_arg_u8 (4) ),
                                           ctx_u8_to_float (ctx_arg_u8 (5) ),
                                           ctx_u8_to_float (ctx_arg_u8 (6) ),
                                           ctx_u8_to_float (ctx_arg_u8 (7) ) );
        break;
        // XXX  implement TEXTURE
#if 0
      case CTX_LOAD_IMAGE:
        {
          if (image)
            {
              cairo_surface_destroy (image);
              image = NULL;
            }
          if (pat)
            {
              cairo_pattern_destroy (pat);
              pat = NULL;
            }
          image = cairo_image_surface_create_from_png (ctx_arg_string() );
          cairo_set_source_surface (cr, image, ctx_arg_float (0), ctx_arg_float (1) );
        }
        break;
#endif
      case CTX_TEXT:
        /* XXX: implement some linebreaking/wrap, positioning
         *      behavior here
         */
        cairo_show_text (cr, ctx_arg_string () );
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
        break;
    }
  ctx_process (ctx_cairo->ctx, entry);
}

void ctx_cairo_free (CtxCairo *ctx_cairo)
{
  if (ctx_cairo->pat)
    { cairo_pattern_destroy (ctx_cairo->pat); }
  if (ctx_cairo->image)
    { cairo_surface_destroy (ctx_cairo->image); }
  free (ctx_cairo);
}

void
ctx_render_cairo (Ctx *ctx, cairo_t *cr)
{
  CtxIterator iterator;
  CtxCommand *command;
  CtxCairo    ctx_cairo = {{(void*)ctx_cairo_process, NULL, NULL}, ctx, cr, NULL, NULL};
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_cairo_process (&ctx_cairo, command); }
}

Ctx *
ctx_new_for_cairo (cairo_t *cr)
{
  Ctx *ctx = ctx_new ();
  CtxCairo *ctx_cairo = calloc(sizeof(CtxCairo),1);
  ctx_cairo->vfuncs.free = (void*)ctx_cairo_free;
  ctx_cairo->vfuncs.process = (void*)ctx_cairo_process;
  ctx_cairo->ctx = ctx;
  ctx_cairo->cr = cr;

  ctx_set_renderer (ctx, (void*)ctx_cairo);
  return ctx;
}

#endif

#if CTX_EVENTS

static int ctx_find_largest_matching_substring
 (const char *X, const char *Y, int m, int n, int *offsetY, int *offsetX) 
{ 
  int longest_common_suffix[2][n+1];
  int best_length = 0;
  for (int i=0; i<=m; i++)
  {
    for (int j=0; j<=n; j++)
    {
      if (i == 0 || j == 0 || !(X[i-1] == Y[j-1]))
      {
        longest_common_suffix[i%2][j] = 0;
      }
      else
      {
          longest_common_suffix[i%2][j] = longest_common_suffix[(i-1)%2][j-1] + 1;
          if (best_length < longest_common_suffix[i%2][j])
          {
            best_length = longest_common_suffix[i%2][j];
            if (offsetY) *offsetY = j - best_length;
            if (offsetX) *offsetX = i - best_length;
          }
      }
    }
  }
  return best_length;
} 

typedef struct CtxSpan {
  int from_prev;
  int start;
  int length;
} CtxSpan;

#define CHUNK_SIZE 32
#define MIN_MATCH  7        // minimum match length to be encoded
#define WINDOW_PADDING 16   // look-aside amount

#if 0
static void _dassert(int line, int condition, const char *str, int foo, int bar, int baz)
{
  if (!condition)
  {
    FILE *f = fopen ("/tmp/cdebug", "a");
    fprintf (f, "%i: %s    %i %i %i\n", line, str, foo, bar, baz);
    fclose (f);
  }
}
#define dassert(cond, foo, bar, baz) _dassert(__LINE__, cond, #cond, foo, bar ,baz)
#endif
#define dassert(cond, foo, bar, baz)

/* XXX repeated substring matching is slow, we'll be
 * better off with a hash-table with linked lists of
 * matching 3-4 characters in previous.. or even
 * a naive approach that expects rough alignment..
 */
static char *encode_in_terms_of_previous (
                const char *src,  int src_len,
                const char *prev, int prev_len,
                int *out_len,
                int max_ticks)
{
  CtxString *string = ctx_string_new ("");
  CtxList *encoded_list = NULL;

  /* TODO : make expected position offset in prev slide based on
   * matches and not be constant */

  long ticks_start = ctx_ticks ();
  int start = 0;
  int length = CHUNK_SIZE;
  for (start = 0; start < src_len; start += length)
  {
    CtxSpan *span = calloc (sizeof (CtxSpan), 1);
    span->start = start;
    if (start + length > src_len)
      span->length = src_len - start;
    else
      span->length = length;
    span->from_prev = 0;
    ctx_list_append (&encoded_list, span);
  }

  for (CtxList *l = encoded_list; l; l = l->next)
  {
    CtxSpan *span = l->data;
    if (!span->from_prev)
    {
      if (span->length >= MIN_MATCH)
      {
         int prev_pos = 0;
         int curr_pos = 0;
         assert(1);
#if 0
         int prev_start =  0;
         int prev_window_length = prev_len;
#else
         int window_padding = WINDOW_PADDING;
         int prev_start = span->start - window_padding;
         if (prev_start < 0)
           prev_start = 0;

         dassert(span->start>=0 , 0,0,0);

         int prev_window_length = prev_len - prev_start;
         if (prev_window_length > span->length + window_padding * 2 + span->start)
           prev_window_length = span->length + window_padding * 2 + span->start;
#endif
         int match_len = 0;
         if (prev_window_length > 0)
           match_len = ctx_find_largest_matching_substring(prev + prev_start, src + span->start, prev_window_length, span->length, &curr_pos, &prev_pos);
#if 1
         prev_pos += prev_start;
#endif

         if (match_len >= MIN_MATCH)
         {
            int start  = span->start;
            int length = span->length;

            span->from_prev = 1;
            span->start     = prev_pos;
            span->length    = match_len;
            dassert (span->start >= 0, prev_pos, prev_start, span->start);
            dassert (span->length > 0, prev_pos, prev_start, span->length);

            if (curr_pos)
            {
              CtxSpan *prev = calloc (sizeof (CtxSpan), 1);
              prev->start = start;
              prev->length =  curr_pos;
            dassert (prev->start >= 0, prev_pos, prev_start, prev->start);
            dassert (prev->length > 0, prev_pos, prev_start, prev->length);
              prev->from_prev = 0;
              ctx_list_insert_before (&encoded_list, l, prev);
            }


            if (match_len + curr_pos < start + length)
            {
              CtxSpan *next = calloc (sizeof (CtxSpan), 1);
              next->start = start + curr_pos + match_len;
              next->length = (start + length) - next->start;
            dassert (next->start >= 0, prev_pos, prev_start, next->start);
      //    dassert (next->length > 0, prev_pos, prev_start, next->length);
              next->from_prev = 0;
              if (next->length)
              {
                if (l->next)
                  ctx_list_insert_before (&encoded_list, l->next, next);
                else
                  ctx_list_append (&encoded_list, next);
              }
              else
                free (next);
            }

            if (curr_pos) // step one item back for forloop
            {
              CtxList *tmp = encoded_list;
              int found = 0;
              while (!found && tmp && tmp->next)
              {
                if (tmp->next == l)
                {
                  l = tmp;
                  break;
                }
                tmp = tmp->next;
              }
            }
         }
      }
    }

    if (ctx_ticks ()-ticks_start > (unsigned long)max_ticks)
      break;
  }

  /* merge adjecant prev span references  */
  {
    for (CtxList *l = encoded_list; l; l = l->next)
    {
      CtxSpan *span = l->data;
again:
      if (l->next)
      {
        CtxSpan *next_span = l->next->data;
        if (span->from_prev && next_span->from_prev &&
            span->start + span->length == 
            next_span->start)
        {
           span->length += next_span->length;
           ctx_list_remove (&encoded_list, next_span);
           goto again;
        }
      }
    }
  }

  while (encoded_list)
  {
    CtxSpan *span = encoded_list->data;
    if (span->from_prev)
    {
      char ref[128];
      sprintf (ref, "%c%i %i%c", CTX_CODEC_CHAR, span->start, span->length, CTX_CODEC_CHAR);
      ctx_string_append_data (string, ref, strlen(ref));
    }
    else
    {
      for (int i = span->start; i< span->start+span->length; i++)
      {
        if (src[i] == CTX_CODEC_CHAR)
        {
          ctx_string_append_byte (string, CTX_CODEC_CHAR);
          ctx_string_append_byte (string, CTX_CODEC_CHAR);
        }
        else
        {
          ctx_string_append_byte (string, src[i]);
        }
      }
    }
    free (span);
    ctx_list_remove (&encoded_list, span);
  }

  char *ret = string->str;
  if (out_len) *out_len = string->length;
  ctx_string_free (string, 0);
  return ret;
}

#if 0 // for documentation/reference purposes
static char *decode_ctx (const char *encoded, int enc_len, const char *prev, int prev_len, int *out_len)
{
  CtxString *string = ctx_string_new ("");
  char reference[32]="";
  int ref_len = 0;
  int in_ref = 0;
  for (int i = 0; i < enc_len; i++)
  {
    if (encoded[i] == CTX_CODEC_CHAR)
    {
      if (!in_ref)
      {
        in_ref = 1;
      }
      else
      {
        int start = atoi (reference);
        int len = 0;
        if (strchr (reference, ' '))
          len = atoi (strchr (reference, ' ')+1);

        if (start < 0)start = 0;
        if (start >= prev_len)start = prev_len-1;
        if (len + start > prev_len)
          len = prev_len - start;

        if (start == 0 && len == 0)
          ctx_string_append_byte (string, CTX_CODEC_CHAR);
        else
          ctx_string_append_data (string, prev + start, len);
        ref_len = 0;
        in_ref = 0;
      }
    }
    else
    {
      if (in_ref)
      {
        if (ref_len < 16)
        {
          reference[ref_len++] = encoded[i];
          reference[ref_len] = 0;
        }
      }
      else
      ctx_string_append_byte (string, encoded[i]);
    }
  }
  char *ret = string->str;
  if (out_len) *out_len = string->length;
  ctx_string_free (string, 0);
  return ret;
}
#endif

#define CTX_START_STRING "U\n"  // or " reset "
#define CTX_END_STRING   "\nX"  // or "\ndone"
#define CTX_END_STRING2  "\n\e"

int ctx_frame_ack = -1;
static char *prev_frame_contents = NULL;
static int   prev_frame_len = 0;

static void ctx_ctx_flush (CtxCtx *ctxctx)
{
#if 0
  FILE *debug = fopen ("/tmp/ctx-debug", "a");
  fprintf (debug, "------\n");
#endif

  if (ctx_native_events)
    fprintf (stdout, "\e[?201h");
  fprintf (stdout, "\e[H\e[?25l\e[?200h");
#if 0
  fprintf (stdout, CTX_START_STRING);
  ctx_render_stream (ctxctx->ctx, stdout, 0);
  fprintf (stdout, CTX_END_STRING);
#else
  {
    int cur_frame_len = 0;
    char *rest = ctx_render_string (ctxctx->ctx, 0, &cur_frame_len);
    char *cur_frame_contents = malloc (cur_frame_len + strlen(CTX_START_STRING) + strlen (CTX_END_STRING) + 1);

    cur_frame_contents[0]=0;
    strcat (cur_frame_contents, CTX_START_STRING);
    strcat (cur_frame_contents, rest);
    strcat (cur_frame_contents, CTX_END_STRING);
    free (rest);
    cur_frame_len += strlen (CTX_START_STRING) + strlen (CTX_END_STRING);

    if (prev_frame_contents && 0)  // XXX : 
    {
      char *encoded;
      int encoded_len = 0;
      //uint64_t ticks_start = ctx_ticks ();

      encoded = encode_in_terms_of_previous (cur_frame_contents, cur_frame_len, prev_frame_contents, prev_frame_len, &encoded_len, 1000 * 10);
//    encoded = strdup (cur_frame_contents);
//    encoded_len = strlen (encoded);
      //uint64_t ticks_end = ctx_ticks ();

      fwrite (encoded, encoded_len, 1, stdout);
//    fwrite (encoded, cur_frame_len, 1, stdout);
#if 0
      fprintf (debug, "---prev-frame(%i)\n%s", (int)strlen(prev_frame_contents), prev_frame_contents);
      fprintf (debug, "---cur-frame(%i)\n%s", (int)strlen(cur_frame_contents), cur_frame_contents);
      fprintf (debug, "---encoded(%.4f %i)---\n%s--------\n",
                      (ticks_end-ticks_start)/1000.0,
                      (int)strlen(encoded), encoded);
#endif
      free (encoded);
    }
    else
    {
      fwrite (cur_frame_contents, cur_frame_len, 1, stdout);
    }

    if (prev_frame_contents)
      free (prev_frame_contents);
    prev_frame_contents = cur_frame_contents;
    prev_frame_len = cur_frame_len;
  }
#endif
#if 0
    fclose (debug);
#endif
  fprintf (stdout, CTX_END_STRING2);

  fprintf (stdout, "\e[5n");
  fflush (stdout);

  ctx_frame_ack = 0;
  do {
     ctx_consume_events (ctxctx->ctx);
  } while (ctx_frame_ack != 1);
}

void ctx_ctx_free (CtxCtx *ctx)
{
  nc_at_exit ();
  free (ctx);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

Ctx *ctx_new_ctx (int width, int height)
{
  Ctx *ctx = ctx_new ();
  CtxCtx *ctxctx = (CtxCtx*)calloc (sizeof (CtxCtx), 1);
  fprintf (stdout, "\e[?1049h");
  fflush (stdout);
  //fprintf (stderr, "\e[H");
  //fprintf (stderr, "\e[2J");
  ctx_native_events = 1;
  if (width <= 0 || height <= 0)
  {
    ctxctx->cols = ctx_terminal_cols ();
    ctxctx->rows = ctx_terminal_rows ();
    width  = ctxctx->width  = ctx_terminal_width ();
    height = ctxctx->height = ctx_terminal_height ();
  }
  else
  {
    ctxctx->width  = width;
    ctxctx->height = height;
    ctxctx->cols   = width / 80;
    ctxctx->rows   = height / 24;
  }
  ctxctx->ctx = ctx;
  if (!ctx_native_events)
    _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, ctxctx);
  ctx_set_size (ctx, width, height);
  ctxctx->flush = (void(*)(void *))ctx_ctx_flush;
  ctxctx->free  = (void(*)(void *))ctx_ctx_free;
  return ctx;
}

void ctx_ctx_pcm (Ctx *ctx);

int ctx_ctx_consume_events (Ctx *ctx)
{
  int ix, iy;
  CtxCtx *ctxctx = (CtxCtx*)ctx->renderer;
  const char *event = NULL;
#if CTX_AUDIO
  ctx_ctx_pcm (ctx);
#endif
  if (ctx_native_events)
    {
      float x = 0, y = 0;
      int b = 0;
      char event_type[128]="";
      event = ctx_native_get_event (ctx, 1000/120);
#if 0
      if(event){
        FILE *file = fopen ("/tmp/log", "a");
        fprintf (file, "[%s]\n", event);
        fclose (file);
      }
#endif
      if (event)
      {
      sscanf (event, "%s %f %f %i", event_type, &x, &y, &b);
      if (!strcmp (event_type, "idle"))
      {
      }
      else if (!strcmp (event_type, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "mouse-drag")||
               !strcmp (event_type, "mouse-motion"))
      {
        ctx_pointer_motion (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, b, 0);
      }
      else if (!strcmp (event_type, "message"))
      {
        ctx_incoming_message (ctx, event + strlen ("message"), 0);
      } else if (!strcmp (event, "size-changed"))
      {
        fprintf (stdout, "\e[H\e[2J\e[?25l");
        ctxctx->cols = ctx_terminal_cols ();
        ctxctx->rows = ctx_terminal_rows ();
        ctxctx->width  = ctx_terminal_width ();
        ctxctx->height = ctx_terminal_height ();
        ctx_set_size (ctx, ctxctx->width, ctxctx->height);

        if (prev_frame_contents)
          free (prev_frame_contents);
        prev_frame_contents = NULL;
        prev_frame_len = 0;
        ctx_set_dirty (ctx, 1);
        //ctx_key_press (ctx, 0, "size-changed", 0);
      }
      else if (!strcmp (event_type, "keyup"))
      {
        char buf[4]={ x, 0 };
        ctx_key_up (ctx, (int)x, buf, 0);
      }
      else if (!strcmp (event_type, "keydown"))
      {
        char buf[4]={ x, 0 };
        ctx_key_down (ctx, (int)x, buf, 0);
      }
      else
      {
        ctx_key_press (ctx, 0, event, 0);
      }
      }
    }
  else
    {
      float x, y;
      event = ctx_nct_get_event (ctx, 20, &ix, &iy);

      x = (ix - 1.0 + 0.5) / ctxctx->cols * ctx->events.width;
      y = (iy - 1.0)       / ctxctx->rows * ctx->events.height;

      if (!strcmp (event, "mouse-press"))
      {
        ctx_pointer_press (ctx, x, y, 0, 0);
        ctxctx->was_down = 1;
      } else if (!strcmp (event, "mouse-release"))
      {
        ctx_pointer_release (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-motion"))
      {
        //nct_set_cursor_pos (backend->term, ix, iy);
        //nct_flush (backend->term);
        if (ctxctx->was_down)
        {
          ctx_pointer_release (ctx, x, y, 0, 0);
          ctxctx->was_down = 0;
        }
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "mouse-drag"))
      {
        ctx_pointer_motion (ctx, x, y, 0, 0);
      } else if (!strcmp (event, "size-changed"))
      {
        fprintf (stdout, "\e[H\e[2J\e[?25l");
        ctxctx->cols = ctx_terminal_cols ();
        ctxctx->rows = ctx_terminal_rows ();
        ctxctx->width  = ctx_terminal_width ();
        ctxctx->height = ctx_terminal_height ();
        ctx_set_size (ctx, ctxctx->width, ctxctx->height);

        if (prev_frame_contents)
           free (prev_frame_contents);
        prev_frame_contents = NULL;
        prev_frame_len = 0;
        ctx_set_dirty (ctx, 1);
        //ctx_key_press (ctx, 0, "size-changed", 0);
      }
      else
      {
        if (!strcmp (event, "esc"))
          ctx_key_press (ctx, 0, "escape", 0);
        else if (!strcmp (event, "space"))
          ctx_key_press (ctx, 0, "space", 0);
        else if (!strcmp (event, "enter")||
                 !strcmp (event, "return"))
          ctx_key_press (ctx, 0, "\n", 0);
        else
        ctx_key_press (ctx, 0, event, 0);
      }
    }

  return 1;
}

int ctx_renderer_is_ctx (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_ctx_free)
          return 1;
  return 0;
}

#endif

#if CTX_TILED
static inline int
ctx_tiled_threads_done (CtxTiled *tiled)
{
  int sum = 0;
  for (int i = 0; i < _ctx_max_threads; i++)
  {
     if (tiled->rendered_frame[i] == tiled->render_frame)
       sum ++;
  }
  return sum;
}

int _ctx_damage_control = 0;

void ctx_tiled_free (CtxTiled *tiled)
{
  tiled->quit = 1;
  mtx_lock (&tiled->mtx);
  cnd_broadcast (&tiled->cond);
  mtx_unlock (&tiled->mtx);

  while (tiled->thread_quit < _ctx_max_threads)
    usleep (1000);

  if (tiled->pixels)
  {
    free (tiled->pixels);
  tiled->pixels = NULL;
  for (int i = 0 ; i < _ctx_max_threads; i++)
  {
    ctx_free (tiled->host[i]);
    tiled->host[i]=NULL;
  }

  ctx_free (tiled->ctx_copy);
  }
  // leak?
}
static unsigned char *sdl_icc = NULL;
static long sdl_icc_length = 0;

inline static void ctx_tiled_flush (CtxTiled *tiled)
{
  if (tiled->shown_frame == tiled->render_frame)
  {
    int dirty_tiles = 0;
    ctx_set_drawlist (tiled->ctx_copy, &tiled->ctx->drawlist.entries[0],
                                           tiled->ctx->drawlist.count * 9);
    if (_ctx_enable_hash_cache)
    {
      Ctx *hasher = ctx_hasher_new (tiled->width, tiled->height,
                        CTX_HASH_COLS, CTX_HASH_ROWS);
      ctx_render_ctx (tiled->ctx_copy, hasher);

      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          uint8_t *new_hash = ctx_hasher_get_hash (hasher, col, row);
          if (new_hash && memcmp (new_hash, &tiled->hashes[(row * CTX_HASH_COLS + col) *  20], 20))
          {
            memcpy (&tiled->hashes[(row * CTX_HASH_COLS +  col)*20], new_hash, 20);
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
            dirty_tiles++;
          }
          else
          {
            tiled->tile_affinity[row * CTX_HASH_COLS + col] = -1;
          }
        }
      free (((CtxHasher*)(hasher->renderer))->hashes);
      ctx_free (hasher);
    }
    else
    {
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = 1;
          dirty_tiles++;
        }
    }
    int dirty_no = 0;
    if (dirty_tiles)
    for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        if (tiled->tile_affinity[row * CTX_HASH_COLS + col] != -1)
        {
          tiled->tile_affinity[row * CTX_HASH_COLS + col] = dirty_no * (_ctx_max_threads) / dirty_tiles;
          dirty_no++;
          if (col > tiled->max_col) tiled->max_col = col;
          if (col < tiled->min_col) tiled->min_col = col;
          if (row > tiled->max_row) tiled->max_row = row;
          if (row < tiled->min_row) tiled->min_row = row;
        }
      }

    if (_ctx_damage_control)
    {
      for (int i = 0; i < tiled->width * tiled->height; i++)
      {
        tiled->pixels[i*4+2]  = (tiled->pixels[i*4+2] + 255)/2;
      }
    }

    tiled->render_frame = ++tiled->frame;

#if 0

          //if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * 0;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * 0;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;
            Ctx *host = tiled->host[0];

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->renderer;
            int swap_red_green = ((CtxRasterizer*)(host->renderer))->swap_red_green;
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, 1, 1,
                                 tiled->width*4, CTX_FORMAT_RGBA8,
                                 tiled->antialias);
            //((CtxRasterizer*)(host->renderer))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
#endif


    mtx_lock (&tiled->mtx);
    cnd_broadcast (&tiled->cond);
    mtx_unlock (&tiled->mtx);
  }
}

static
void ctx_tiled_render_fun (void **data)
{
  int      no = (size_t)data[0];
  CtxTiled *tiled = data[1];

  while (!tiled->quit)
  {
    Ctx *host = tiled->host[no];

    mtx_lock (&tiled->mtx);
    cnd_wait(&tiled->cond, &tiled->mtx);
    mtx_unlock (&tiled->mtx);

    if (tiled->render_frame != tiled->rendered_frame[no])
    {
      int hno = 0;
      for (int row = 0; row < CTX_HASH_ROWS; row++)
        for (int col = 0; col < CTX_HASH_COLS; col++, hno++)
        {
          if (tiled->tile_affinity[hno]==no)
          {
            int x0 = ((tiled->width)/CTX_HASH_COLS) * col;
            int y0 = ((tiled->height)/CTX_HASH_ROWS) * row;
            int width = tiled->width / CTX_HASH_COLS;
            int height = tiled->height / CTX_HASH_ROWS;

            CtxRasterizer *rasterizer = (CtxRasterizer*)host->renderer;
#if 1 // merge horizontally adjecant tiles of same affinity into one job
            while (col + 1 < CTX_HASH_COLS &&
                   tiled->tile_affinity[hno+1] == no)
            {
              width += tiled->width / CTX_HASH_COLS;
              col++;
              hno++;
            }
#endif
            int swap_red_green = ((CtxRasterizer*)(host->renderer))->swap_red_green;
            ctx_rasterizer_init (rasterizer,
                                 host, tiled->ctx, &host->state,
                                 &tiled->pixels[tiled->width * 4 * y0 + x0 * 4],
                                 0, 0, width, height,
                                 tiled->width*4, CTX_FORMAT_RGBA8,
                                 tiled->antialias);
            ((CtxRasterizer*)(host->renderer))->swap_red_green = swap_red_green;
            if (sdl_icc_length)
              ctx_colorspace (host, CTX_COLOR_SPACE_DEVICE_RGB, sdl_icc, sdl_icc_length);

            ctx_translate (host, -x0, -y0);
            ctx_render_ctx (tiled->ctx_copy, host);
          }
        }
      tiled->rendered_frame[no] = tiled->render_frame;
    }
  }
  tiled->thread_quit++; // need atomic?
}

#endif


#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif


#if CTX_FB
  #include <linux/fb.h>
  #include <linux/vt.h>
  #include <linux/kd.h>
  #include <sys/mman.h>
  #include <threads.h>
  #include <libdrm/drm.h>
  #include <libdrm/drm_mode.h>

typedef struct _EvSource EvSource;
 

struct _EvSource
{
  void   *priv; /* private storage  */

  /* returns non 0 if there is events waiting */
  int   (*has_event) (EvSource *ev_source);

  /* get an event, the returned event should be freed by the caller  */
  char *(*get_event) (EvSource *ev_source);

  /* destroy/unref this instance */
  void  (*destroy)   (EvSource *ev_source);

  /* get the underlying fd, useful for using select on  */
  int   (*get_fd)    (EvSource *ev_source);


  void  (*set_coord) (EvSource *ev_source, double x, double y);
  /* set_coord is needed to warp relative cursors into normalized range,
   * like normal mice/trackpads/nipples - to obey edges and more.
   */

  /* if this returns non-0 select can be used for non-blocking.. */
};


typedef struct _CtxFb CtxFb;
struct _CtxFb
{
   CtxTiled tiled;
#if 0
   void (*render) (void *fb, CtxCommand *command);
   void (*reset)  (void *fb);
   void (*flush)  (void *fb);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *fb);
   Ctx          *ctx;
   int           width;
   int           height;
   int           cols; // unused
   int           rows; // unused
   int           was_down;
   uint8_t      *pixels;
   Ctx          *ctx_copy;
   Ctx          *host[CTX_MAX_THREADS];
   CtxAntialias  antialias;
   int           quit;
   _Atomic int   thread_quit;
   int           shown_frame;
   int           render_frame;
   int           rendered_frame[CTX_MAX_THREADS];
   int           frame;
   int           min_col; // hasher cols and rows
   int           min_row;
   int           max_col;
   int           max_row;
   uint8_t       hashes[CTX_HASH_ROWS * CTX_HASH_COLS *  20];
   int8_t        tile_affinity[CTX_HASH_ROWS * CTX_HASH_COLS]; // which render thread no is
                                                           // responsible for a tile
                                                           //


   int           pointer_down[3];
#endif
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;

   uint8_t      *fb;

   int          fb_fd;
   char        *fb_path;
   int          fb_bits;
   int          fb_bpp;
   int          fb_mapped_size;
   struct       fb_var_screeninfo vinfo;
   struct       fb_fix_screeninfo finfo;
   int          vt;
   int          tty;
   int          vt_active;
   EvSource    *evsource[4];
   int          evsource_count;
   int          is_drm;
   cnd_t        cond;
   mtx_t        mtx;
   struct drm_mode_crtc crtc;
};

static char *ctx_fb_clipboard = NULL;
static void ctx_fb_set_clipboard (CtxFb *fb, const char *text)
{
  if (ctx_fb_clipboard)
    free (ctx_fb_clipboard);
  ctx_fb_clipboard = NULL;
  if (text)
  {
    ctx_fb_clipboard = strdup (text);
  }
}

static char *ctx_fb_get_clipboard (CtxFb *sdl)
{
  if (ctx_fb_clipboard) return strdup (ctx_fb_clipboard);
  return strdup ("");
}

#if UINTPTR_MAX == 0xffFFffFF
  #define fbdrmuint_t uint32_t
#elif UINTPTR_MAX == 0xffFFffFFffFFffFF
  #define fbdrmuint_t uint64_t
#endif

void *ctx_fbdrm_new (CtxFb *fb, int *width, int *height)
{
   int got_master = 0;
   fb->fb_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
   if (!fb->fb_fd)
     return NULL;
   static fbdrmuint_t res_conn_buf[20]={0}; // this is static since its contents
                                         // are used by the flip callback
   fbdrmuint_t res_fb_buf[20]={0};
   fbdrmuint_t res_crtc_buf[20]={0};
   fbdrmuint_t res_enc_buf[20]={0};
   struct   drm_mode_card_res res={0};

   if (ioctl(fb->fb_fd, DRM_IOCTL_SET_MASTER, 0))
     goto cleanup;
   got_master = 1;

   if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
     goto cleanup;
   res.fb_id_ptr=(fbdrmuint_t)res_fb_buf;
   res.crtc_id_ptr=(fbdrmuint_t)res_crtc_buf;
   res.connector_id_ptr=(fbdrmuint_t)res_conn_buf;
   res.encoder_id_ptr=(fbdrmuint_t)res_enc_buf;
   if(ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
      goto cleanup;


   unsigned int i;
   for (i=0;i<res.count_connectors;i++)
   {
     struct drm_mode_modeinfo conn_mode_buf[20]={0};
     fbdrmuint_t conn_prop_buf[20]={0},
                     conn_propval_buf[20]={0},
                     conn_enc_buf[20]={0};

     struct drm_mode_get_connector conn={0};

     conn.connector_id=res_conn_buf[i];

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     conn.modes_ptr=(fbdrmuint_t)conn_mode_buf;
     conn.props_ptr=(fbdrmuint_t)conn_prop_buf;
     conn.prop_values_ptr=(fbdrmuint_t)conn_propval_buf;
     conn.encoders_ptr=(fbdrmuint_t)conn_enc_buf;

     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
       goto cleanup;

     //Check if the connector is OK to use (connected to something)
     if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
       continue;

//------------------------------------------------------------------------------
//Creating a dumb buffer
//------------------------------------------------------------------------------
     struct drm_mode_create_dumb create_dumb={0};
     struct drm_mode_map_dumb    map_dumb={0};
     struct drm_mode_fb_cmd      cmd_dumb={0};
     create_dumb.width  = conn_mode_buf[0].hdisplay;
     create_dumb.height = conn_mode_buf[0].vdisplay;
     create_dumb.bpp   = 32;
     create_dumb.flags = 0;
     create_dumb.pitch = 0;
     create_dumb.size  = 0;
     create_dumb.handle = 0;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) ||
         !create_dumb.handle)
       goto cleanup;

     cmd_dumb.width =create_dumb.width;
     cmd_dumb.height=create_dumb.height;
     cmd_dumb.bpp   =create_dumb.bpp;
     cmd_dumb.pitch =create_dumb.pitch;
     cmd_dumb.depth =24;
     cmd_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb))
       goto cleanup;

     map_dumb.handle=create_dumb.handle;
     if (ioctl(fb->fb_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb))
       goto cleanup;

     void *base = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       fb->fb_fd, map_dumb.offset);
     if (!base)
     {
       goto cleanup;
     }
     *width  = create_dumb.width;
     *height = create_dumb.height;

     struct drm_mode_get_encoder enc={0};
     enc.encoder_id=conn.encoder_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETENCODER, &enc))
        goto cleanup;

     fb->crtc.crtc_id=enc.crtc_id;
     if (ioctl(fb->fb_fd, DRM_IOCTL_MODE_GETCRTC, &fb->crtc))
        goto cleanup;

     fb->crtc.fb_id=cmd_dumb.fb_id;
     fb->crtc.set_connectors_ptr=(fbdrmuint_t)&res_conn_buf[i];
     fb->crtc.count_connectors=1;
     fb->crtc.mode=conn_mode_buf[0];
     fb->crtc.mode_valid=1;
     return base;
   }
cleanup:
   if (got_master)
     ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
   fb->fb_fd = 0;
   return NULL;
}

void ctx_fbdrm_flip (CtxFb *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_MODE_SETCRTC, &fb->crtc);
}

void ctx_fbdrm_close (CtxFb *fb)
{
  if (!fb->fb_fd)
    return;
  ioctl(fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
  close (fb->fb_fd);
  fb->fb_fd = 0;
}

static void ctx_fb_flip (CtxFb *fb)
{
  if (fb->is_drm)
    ctx_fbdrm_flip (fb);
  else
    ioctl (fb->fb_fd, FBIOPAN_DISPLAY, &fb->vinfo);
}

inline static uint32_t
ctx_swap_red_green2 (uint32_t orig)
{
  uint32_t  green_alpha = (orig & 0xff00ff00);
  uint32_t  red_blue    = (orig & 0x00ff00ff);
  uint32_t  red         = red_blue << 16;
  uint32_t  blue        = red_blue >> 16;
  return green_alpha | red | blue;
}

static int       ctx_fb_cursor_drawn   = 0;
static int       ctx_fb_cursor_drawn_x = 0;
static int       ctx_fb_cursor_drawn_y = 0;
static CtxCursor ctx_fb_cursor_drawn_shape = 0;


#define CTX_FB_HIDE_CURSOR_FRAMES 200

static int ctx_fb_cursor_same_pos = CTX_FB_HIDE_CURSOR_FRAMES;

static inline int ctx_is_in_cursor (int x, int y, int size, CtxCursor shape)
{
  switch (shape)
  {
    case CTX_CURSOR_ARROW:
      if (x > ((size * 4)-y*4)) return 0;
      if (x < y && x > y / 16)
        return 1;
      return 0;

    case CTX_CURSOR_RESIZE_SE:
    case CTX_CURSOR_RESIZE_NW:
    case CTX_CURSOR_RESIZE_SW:
    case CTX_CURSOR_RESIZE_NE:
      {
        float theta = -45.0/180 * M_PI;
        float cos_theta;
        float sin_theta;

        if ((shape == CTX_CURSOR_RESIZE_SW) ||
            (shape == CTX_CURSOR_RESIZE_NE))
        {
          theta = -theta;
          cos_theta = cos (theta);
          sin_theta = sin (theta);
        }
        else
        {
          cos_theta = cos (theta);
          sin_theta = sin (theta);
        }
        int rot_x = x * cos_theta - y * sin_theta;
        int rot_y = y * cos_theta + x * sin_theta;
        x = rot_x;
        y = rot_y;
      }
      /*FALLTHROUGH*/
    case CTX_CURSOR_RESIZE_W:
    case CTX_CURSOR_RESIZE_E:
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs(y) < size/10)
        {
          return 1;
        }
      }
      if ((abs (x) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(y) < (size/2.8)-(abs(x) - (size/2)))
          return 1;
      }
      if (shape != CTX_CURSOR_RESIZE_ALL)
        break;
      /* FALLTHROUGH */
    case CTX_CURSOR_RESIZE_S:
    case CTX_CURSOR_RESIZE_N:
      if (abs (y) < size/2 && abs (x) < size/2)
      {
        if (abs(x) < size/10)
        {
          return 1;
        }
      }
      if ((abs (y) - size/ (shape == CTX_CURSOR_RESIZE_ALL?2:2.7)) >= 0)
      {
        if (abs(x) < (size/2.8)-(abs(y) - (size/2)))
          return 1;
      }
      break;
#if 0
    case CTX_CURSOR_RESIZE_ALL:
      if (abs (x) < size/2 && abs (y) < size/2)
      {
        if (abs (x) < size/10 || abs(y) < size/10)
          return 1;
      }
      break;
#endif
    default:
      return (x ^ y) & 1;
  }
  return 0;
}

static void ctx_fb_undraw_cursor (CtxFb *fb)
{
    CtxTiled *tiled = (void*)fb;
    int cursor_size = ctx_height (tiled->ctx) / 28;

    if (ctx_fb_cursor_drawn)
    {
      int no = 0;
      int startx = -cursor_size;
      int starty = -cursor_size;
      if (ctx_fb_cursor_drawn_shape == CTX_CURSOR_ARROW)
        startx = starty = 0;

      for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + ctx_fb_cursor_drawn_x < tiled->width && y + ctx_fb_cursor_drawn_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, ctx_fb_cursor_drawn_shape))
          {
            int o = ((ctx_fb_cursor_drawn_y + y) * tiled->width + (ctx_fb_cursor_drawn_x + x)) * 4;
            fb->fb[o+0]^=0x88;
            fb->fb[o+1]^=0x88;
            fb->fb[o+2]^=0x88;
          }
        }
      }

    ctx_fb_cursor_drawn = 0;
    }
}

static void ctx_fb_draw_cursor (CtxFb *fb)
{
    CtxTiled *tiled = (void*)fb;
    int cursor_x    = ctx_pointer_x (tiled->ctx);
    int cursor_y    = ctx_pointer_y (tiled->ctx);
    int cursor_size = ctx_height (tiled->ctx) / 28;
    CtxCursor cursor_shape = tiled->ctx->cursor;
    int no = 0;

    if (cursor_x == ctx_fb_cursor_drawn_x &&
        cursor_y == ctx_fb_cursor_drawn_y &&
        cursor_shape == ctx_fb_cursor_drawn_shape)
      ctx_fb_cursor_same_pos ++;
    else
      ctx_fb_cursor_same_pos = 0;

    if (ctx_fb_cursor_same_pos >= CTX_FB_HIDE_CURSOR_FRAMES)
    {
      if (ctx_fb_cursor_drawn)
        ctx_fb_undraw_cursor (fb);
      return;
    }

    /* no need to flicker when stationary, motion flicker can also be removed
     * by combining the previous and next position masks when a motion has
     * occured..
     */
    if (ctx_fb_cursor_same_pos && ctx_fb_cursor_drawn)
      return;

    ctx_fb_undraw_cursor (fb);

    no = 0;

    int startx = -cursor_size;
    int starty = -cursor_size;

    if (cursor_shape == CTX_CURSOR_ARROW)
      startx = starty = 0;

    for (int y = starty; y < cursor_size; y++)
      for (int x = startx; x < cursor_size; x++, no+=4)
      {
        if (x + cursor_x < tiled->width && y + cursor_y < tiled->height)
        {
          if (ctx_is_in_cursor (x, y, cursor_size, cursor_shape))
          {
            int o = ((cursor_y + y) * tiled->width + (cursor_x + x)) * 4;
            fb->fb[o+0]^=0x88;
            fb->fb[o+1]^=0x88;
            fb->fb[o+2]^=0x88;
          }
        }
      }
    ctx_fb_cursor_drawn = 1;
    ctx_fb_cursor_drawn_x = cursor_x;
    ctx_fb_cursor_drawn_y = cursor_y;
    ctx_fb_cursor_drawn_shape = cursor_shape;
}

static void ctx_fb_show_frame (CtxFb *fb, int block)
{
  CtxTiled *tiled = (void*)fb;
  if (tiled->shown_frame == tiled->render_frame)
  {
    if (block == 0) // consume event call
    {
      ctx_fb_draw_cursor (fb);
      ctx_fb_flip (fb);
    }
    return;
  }

  if (block)
  {
    int count = 0;
    while (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
    {
      usleep (500);
      count ++;
      if (count > 2000)
      {
        tiled->shown_frame = tiled->render_frame;
        return;
      }
    }
  }
  else
  {
    if (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
      return;
  }

    if (fb->vt_active)
    {
       int pre_skip = tiled->min_row * tiled->height/CTX_HASH_ROWS * tiled->width;
       int post_skip = (CTX_HASH_ROWS-tiled->max_row-1) * tiled->height/CTX_HASH_ROWS * tiled->width;

       int rows = ((tiled->width * tiled->height) - pre_skip - post_skip)/tiled->width;

       int col_pre_skip = tiled->min_col * tiled->width/CTX_HASH_COLS;
       int col_post_skip = (CTX_HASH_COLS-tiled->max_col-1) * tiled->width/CTX_HASH_COLS;
       if (_ctx_damage_control)
       {
         pre_skip = post_skip = col_pre_skip = col_post_skip = 0;
       }

       if (pre_skip < 0) pre_skip = 0;
       if (post_skip < 0) post_skip = 0;

     __u32 dummy = 0;

       if (tiled->min_row == 100){
          pre_skip = 0;
          post_skip = 0;
          // not when drm ?
          ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
          ctx_fb_undraw_cursor (fb);
       }
       else
       {

      tiled->min_row = 100;
      tiled->max_row = 0;
      tiled->min_col = 100;
      tiled->max_col = 0;

     // not when drm ?
     ioctl (fb->fb_fd, FBIO_WAITFORVSYNC, &dummy);
     ctx_fb_undraw_cursor (fb);
     switch (fb->fb_bits)
     {
       case 32:
#if 1
         {
           uint8_t *dst = fb->fb + pre_skip * 4;
           uint8_t *src = tiled->pixels + pre_skip * 4;
           int pre = col_pre_skip * 4;
           int post = col_post_skip * 4;
           int core = tiled->width * 4 - pre - post;
           for (int i = 0; i < rows; i++)
           {
             dst  += pre;
             src  += pre;
             memcpy (dst, src, core);
             src  += core;
             dst  += core;
             dst  += post;
             src  += post;
           }
         }
#else
         { int count = tiled->width * tiled->height;
           const uint32_t *src = (void*)tiled->pixels;
           uint32_t *dst = (void*)fb->fb;
           count-= pre_skip;
           src+= pre_skip;
           dst+= pre_skip;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = ctx_swap_red_green2 (src[0]);
             src++;
             dst++;
           }
         }
#endif
         break;
         /* XXX  :  note: converting a scanline (or all) to target and
          * then doing a bulk memcpy be faster (at least with som /dev/fbs)  */
       case 24:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = fb->fb;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 3;
           count-= post_skip;
           while (count -- > 0)
           {
             dst[0] = src[0];
             dst[1] = src[1];
             dst[2] = src[2];
             dst+=3;
             src+=4;
           }
         }
         break;
       case 16:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[0] >> 3)) +
                ((src[1] >> 2)<<5) +
                ((src[2] >> 3)<<11);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
         break;
       case 15:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip * 2;
           while (count -- > 0)
           {
             int big = ((src[2] >> 3)) +
                       ((src[1] >> 2)<<5) +
                       ((src[0] >> 3)<<10);
             dst[0] = big & 255;
             dst[1] = big >>  8;
             dst+=2;
             src+=4;
           }
         }
         break;
       case 8:
         { int count = tiled->width * tiled->height;
           const uint8_t *src = tiled->pixels;
           uint8_t *dst = fb->fb;
           count-= post_skip;
           count-= pre_skip;
           src+= pre_skip * 4;
           dst+= pre_skip;
           while (count -- > 0)
           {
             dst[0] = ((src[0] >> 5)) +
                      ((src[1] >> 5)<<3) +
                      ((src[2] >> 6)<<6);
             dst+=1;
             src+=4;
           }
         }
         break;
     }
    }
    ctx_fb_cursor_drawn = 0;
    ctx_fb_draw_cursor (fb);
    ctx_fb_flip (fb);
    tiled->shown_frame = tiled->render_frame;
  }
}


#define evsource_has_event(es)   (es)->has_event((es))
#define evsource_get_event(es)   (es)->get_event((es))
#define evsource_destroy(es)     do{if((es)->destroy)(es)->destroy((es));}while(0)
#define evsource_set_coord(es,x,y) do{if((es)->set_coord)(es)->set_coord((es),(x),(y));}while(0)
#define evsource_get_fd(es)      ((es)->get_fd?(es)->get_fd((es)):0)



static int mice_has_event ();
static char *mice_get_event ();
static void mice_destroy ();
static int mice_get_fd (EvSource *ev_source);
static void mice_set_coord (EvSource *ev_source, double x, double y);

static EvSource ctx_ev_src_mice = {
  NULL,
  (void*)mice_has_event,
  (void*)mice_get_event,
  (void*)mice_destroy,
  mice_get_fd,
  mice_set_coord
};

typedef struct Mice
{
  int     fd;
  double  x;
  double  y;
  int     button;
  int     prev_state;
} Mice;

Mice *_mrg_evsrc_coord = NULL;
static int _ctx_mice_fd = 0;

void _mmm_get_coords (Ctx *ctx, double *x, double *y)
{
  if (!_mrg_evsrc_coord)
    return;
  if (x)
    *x = _mrg_evsrc_coord->x;
  if (y)
    *y = _mrg_evsrc_coord->y;
}

static Mice  mice;
static Mice* mrg_mice_this = &mice;

static int mmm_evsource_mice_init ()
{
  unsigned char reset[]={0xff};
  /* need to detect which event */

  mrg_mice_this->prev_state = 0;
  mrg_mice_this->fd = open ("/dev/input/mice", O_RDONLY | O_NONBLOCK);
  if (mrg_mice_this->fd == -1)
  {
    fprintf (stderr, "error opening /dev/input/mice device, maybe add user to input group if such group exist, or otherwise make the rights be satisfied.\n");
    return -1;
  }
  if (write (mrg_mice_this->fd, reset, 1) == -1)
  {
    // might happen if we're a regular user with only read permission
  }
  _ctx_mice_fd = mrg_mice_this->fd;
  _mrg_evsrc_coord = mrg_mice_this;
  return 0;
}

static void mice_destroy ()
{
  if (mrg_mice_this->fd != -1)
    close (mrg_mice_this->fd);
}

static int mice_has_event ()
{
  struct timeval tv;
  int retval;

  if (mrg_mice_this->fd == -1)
    return 0;

  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET(mrg_mice_this->fd, &rfds);
  tv.tv_sec = 0; tv.tv_usec = 0;
  retval = select (mrg_mice_this->fd+1, &rfds, NULL, NULL, &tv);
  if (retval == 1)
    return FD_ISSET (mrg_mice_this->fd, &rfds);
  return 0;
}

static char *mice_get_event ()
{
  const char *ret = "mouse-motion";
  double relx, rely;
  signed char buf[3];
  int n_read = 0;
  CtxFb *fb = ctx_ev_src_mice.priv;
  CtxTiled *tiled = (void*)fb;
  n_read = read (mrg_mice_this->fd, buf, 3);
  if (n_read == 0)
     return strdup ("");
  relx = buf[1];
  rely = -buf[2];

  if (relx < 0)
  {
    if (relx > -6)
    relx = - relx*relx;
    else
    relx = -36;
  }
  else
  {
    if (relx < 6)
    relx = relx*relx;
    else
    relx = 36;
  }

  if (rely < 0)
  {
    if (rely > -6)
    rely = - rely*rely;
    else
    rely = -36;
  }
  else
  {
    if (rely < 6)
    rely = rely*rely;
    else
    rely = 36;
  }

  mrg_mice_this->x += relx;
  mrg_mice_this->y += rely;

  if (mrg_mice_this->x < 0)
    mrg_mice_this->x = 0;
  if (mrg_mice_this->y < 0)
    mrg_mice_this->y = 0;
  if (mrg_mice_this->x >= tiled->width)
    mrg_mice_this->x = tiled->width -1;
  if (mrg_mice_this->y >= tiled->height)
    mrg_mice_this->y = tiled->height -1;
  int button = 0;
  
  if ((mrg_mice_this->prev_state & 1) != (buf[0] & 1))
    {
      if (buf[0] & 1)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 1;
    }
  else if (buf[0] & 1)
  {
    ret = "mouse-drag";
    button = 1;
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 2) != (buf[0] & 2))
    {
      if (buf[0] & 2)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 3;
    }
    else if (buf[0] & 2)
    {
      ret = "mouse-drag";
      button = 3;
    }
  }

  if (!button)
  {
    if ((mrg_mice_this->prev_state & 4) != (buf[0] & 4))
    {
      if (buf[0] & 4)
        {
          ret = "mouse-press";
        }
      else
        {
          ret = "mouse-release";
        }
      button = 2;
    }
    else if (buf[0] & 4)
    {
      ret = "mouse-drag";
      button = 2;
    }
  }

  mrg_mice_this->prev_state = buf[0];

  {
    char *r = malloc (64);
    sprintf (r, "%s %.0f %.0f %i", ret, mrg_mice_this->x, mrg_mice_this->y, button);
    return r;
  }

  return NULL;
}

static int mice_get_fd (EvSource *ev_source)
{
  return mrg_mice_this->fd;
}

static void mice_set_coord (EvSource *ev_source, double x, double y)
{
  mrg_mice_this->x = x;
  mrg_mice_this->y = y;
}

static EvSource *evsource_mice_new (void)
{
  if (mmm_evsource_mice_init () == 0)
    {
      mrg_mice_this->x = 0;
      mrg_mice_this->y = 0;
      return &ctx_ev_src_mice;
    }
  return NULL;
}

static int evsource_kb_has_event (void);
static char *evsource_kb_get_event (void);
static void evsource_kb_destroy (int sign);
static int evsource_kb_get_fd (void);

/* kept out of struct to be reachable by atexit */
static EvSource ctx_ev_src_kb = {
  NULL,
  (void*)evsource_kb_has_event,
  (void*)evsource_kb_get_event,
  (void*)evsource_kb_destroy,
  (void*)evsource_kb_get_fd,
  NULL
};

static struct termios orig_attr;

static void real_evsource_kb_destroy (int sign)
{
  static int done = 0;

  if (sign == 0)
    return;

  if (done)
    return;
  done = 1;

  switch (sign)
  {
    case  -11:break; /* will be called from atexit with sign==-11 */
    case   SIGSEGV: break;//fprintf (stderr, " SIGSEGV\n");break;
    case   SIGABRT: fprintf (stderr, " SIGABRT\n");break;
    case   SIGBUS:  fprintf (stderr, " SIGBUS\n");break;
    case   SIGKILL: fprintf (stderr, " SIGKILL\n");break;
    case   SIGINT:  fprintf (stderr, " SIGINT\n");break;
    case   SIGTERM: fprintf (stderr, " SIGTERM\n");break;
    case   SIGQUIT: fprintf (stderr, " SIGQUIT\n");break;
    default: fprintf (stderr, "sign: %i\n", sign);
             fprintf (stderr, "%i %i %i %i %i %i %i\n", SIGSEGV, SIGABRT, SIGBUS, SIGKILL, SIGINT, SIGTERM, SIGQUIT);
  }
  tcsetattr (STDIN_FILENO, TCSAFLUSH, &orig_attr);
  //fprintf (stderr, "evsource kb destroy\n");
}

static void evsource_kb_destroy (int sign)
{
  real_evsource_kb_destroy (-11);
}

static int evsource_kb_init ()
{
//  ioctl(STDIN_FILENO, KDSKBMODE, K_RAW);
  atexit ((void*) real_evsource_kb_destroy);
  signal (SIGSEGV, (void*) real_evsource_kb_destroy);
  signal (SIGABRT, (void*) real_evsource_kb_destroy);
  signal (SIGBUS,  (void*) real_evsource_kb_destroy);
  signal (SIGKILL, (void*) real_evsource_kb_destroy);
  signal (SIGINT,  (void*) real_evsource_kb_destroy);
  signal (SIGTERM, (void*) real_evsource_kb_destroy);
  signal (SIGQUIT, (void*) real_evsource_kb_destroy);

  struct termios raw;
  if (tcgetattr (STDIN_FILENO, &orig_attr) == -1)
    {
      fprintf (stderr, "error initializing keyboard\n");
      return -1;
    }
  raw = orig_attr;

  cfmakeraw (&raw);

  raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0; /* 1 byte, no timer */
  if (tcsetattr (STDIN_FILENO, TCSAFLUSH, &raw) < 0)
    return 0; // XXX? return other value?

  return 0;
}
static int evsource_kb_has_event (void)
{
  struct timeval tv;
  int retval;

  fd_set rfds;
  FD_ZERO (&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  tv.tv_sec = 0; tv.tv_usec = 0;
  retval = select (STDIN_FILENO+1, &rfds, NULL, NULL, &tv);
  return retval == 1;
}

/* note that a nick can have multiple occurences, the labels
 * should be kept the same for all occurences of a combination.
 *
 * this table is taken from nchanterm.
 */
typedef struct MmmKeyCode {
  char *nick;          /* programmers name for key */
  char  sequence[10];  /* terminal sequence */
} MmmKeyCode;
static const MmmKeyCode ufb_keycodes[]={
  {"up",                  "\e[A"},
  {"down",                "\e[B"},
  {"right",               "\e[C"},
  {"left",                "\e[D"},

  {"shift-up",            "\e[1;2A"},
  {"shift-down",          "\e[1;2B"},
  {"shift-right",         "\e[1;2C"},
  {"shift-left",          "\e[1;2D"},

  {"alt-up",              "\e[1;3A"},
  {"alt-down",            "\e[1;3B"},
  {"alt-right",           "\e[1;3C"},
  {"alt-left",            "\e[1;3D"},
  {"alt-shift-up",         "\e[1;4A"},
  {"alt-shift-down",       "\e[1;4B"},
  {"alt-shift-right",      "\e[1;4C"},
  {"alt-shift-left",       "\e[1;4D"},

  {"control-up",          "\e[1;5A"},
  {"control-down",        "\e[1;5B"},
  {"control-right",       "\e[1;5C"},
  {"control-left",        "\e[1;5D"},

  /* putty */
  {"control-up",          "\eOA"},
  {"control-down",        "\eOB"},
  {"control-right",       "\eOC"},
  {"control-left",        "\eOD"},

  {"control-shift-up",    "\e[1;6A"},
  {"control-shift-down",  "\e[1;6B"},
  {"control-shift-right", "\e[1;6C"},
  {"control-shift-left",  "\e[1;6D"},

  {"control-up",          "\eOa"},
  {"control-down",        "\eOb"},
  {"control-right",       "\eOc"},
  {"control-left",        "\eOd"},

  {"shift-up",            "\e[a"},
  {"shift-down",          "\e[b"},
  {"shift-right",         "\e[c"},
  {"shift-left",          "\e[d"},

  {"insert",              "\e[2~"},
  {"delete",              "\e[3~"},
  {"page-up",             "\e[5~"},
  {"page-down",           "\e[6~"},
  {"home",                "\eOH"},
  {"end",                 "\eOF"},
  {"home",                "\e[H"},
  {"end",                 "\e[F"},
 {"control-delete",       "\e[3;5~"},
  {"shift-delete",        "\e[3;2~"},
  {"control-shift-delete","\e[3;6~"},

  {"F1",         "\e[25~"},
  {"F2",         "\e[26~"},
  {"F3",         "\e[27~"},
  {"F4",         "\e[26~"},


  {"F1",         "\e[11~"},
  {"F2",         "\e[12~"},
  {"F3",         "\e[13~"},
  {"F4",         "\e[14~"},
  {"F1",         "\eOP"},
  {"F2",         "\eOQ"},
  {"F3",         "\eOR"},
  {"F4",         "\eOS"},
  {"F5",         "\e[15~"},
  {"F6",         "\e[16~"},
  {"F7",         "\e[17~"},
  {"F8",         "\e[18~"},
  {"F9",         "\e[19~"},
  {"F9",         "\e[20~"},
  {"F10",        "\e[21~"},
  {"F11",        "\e[22~"},
  {"F12",        "\e[23~"},
  {"tab",         {9, '\0'}},
  {"shift-tab",   {27, 9, '\0'}}, // also generated by alt-tab in linux console
  {"alt-space",   {27, ' ', '\0'}},
  {"shift-tab",   "\e[Z"},
  {"backspace",   {127, '\0'}},
  {"space",       " "},
  {"\e",          "\e"},
  {"return",      {10,0}},
  {"return",      {13,0}},
  /* this section could be autogenerated by code */
  {"control-a",   {1,0}},
  {"control-b",   {2,0}},
  {"control-c",   {3,0}},
  {"control-d",   {4,0}},
  {"control-e",   {5,0}},
  {"control-f",   {6,0}},
  {"control-g",   {7,0}},
  {"control-h",   {8,0}}, /* backspace? */
  {"control-i",   {9,0}},
  {"control-j",   {10,0}},
  {"control-k",   {11,0}},
  {"control-l",   {12,0}},
  {"control-n",   {14,0}},
  {"control-o",   {15,0}},
  {"control-p",   {16,0}},
  {"control-q",   {17,0}},
  {"control-r",   {18,0}},
  {"control-s",   {19,0}},
  {"control-t",   {20,0}},
  {"control-u",   {21,0}},
  {"control-v",   {22,0}},
  {"control-w",   {23,0}},
  {"control-x",   {24,0}},
  {"control-y",   {25,0}},
  {"control-z",   {26,0}},
  {"alt-`",       "\e`"},
  {"alt-0",       "\e0"},
  {"alt-1",       "\e1"},
  {"alt-2",       "\e2"},
  {"alt-3",       "\e3"},
  {"alt-4",       "\e4"},
  {"alt-5",       "\e5"},
  {"alt-6",       "\e6"},
  {"alt-7",       "\e7"}, /* backspace? */
  {"alt-8",       "\e8"},
  {"alt-9",       "\e9"},
  {"alt-+",       "\e+"},
  {"alt--",       "\e-"},
  {"alt-/",       "\e/"},
  {"alt-a",       "\ea"},
  {"alt-b",       "\eb"},
  {"alt-c",       "\ec"},
  {"alt-d",       "\ed"},
  {"alt-e",       "\ee"},
  {"alt-f",       "\ef"},
  {"alt-g",       "\eg"},
  {"alt-h",       "\eh"}, /* backspace? */
  {"alt-i",       "\ei"},
  {"alt-j",       "\ej"},
  {"alt-k",       "\ek"},
  {"alt-l",       "\el"},
  {"alt-n",       "\em"},
  {"alt-n",       "\en"},
  {"alt-o",       "\eo"},
  {"alt-p",       "\ep"},
  {"alt-q",       "\eq"},
  {"alt-r",       "\er"},
  {"alt-s",       "\es"},
  {"alt-t",       "\et"},
  {"alt-u",       "\eu"},
  {"alt-v",       "\ev"},
  {"alt-w",       "\ew"},
  {"alt-x",       "\ex"},
  {"alt-y",       "\ey"},
  {"alt-z",       "\ez"},
  /* Linux Console  */
  {"home",       "\e[1~"},
  {"end",        "\e[4~"},
  {"F1",         "\e[[A"},
  {"F2",         "\e[[B"},
  {"F3",         "\e[[C"},
  {"F4",         "\e[[D"},
  {"F5",         "\e[[E"},
  {"F6",         "\e[[F"},
  {"F7",         "\e[[G"},
  {"F8",         "\e[[H"},
  {"F9",         "\e[[I"},
  {"F10",        "\e[[J"},
  {"F11",        "\e[[K"},
  {"F12",        "\e[[L"},
  {NULL, }
};
static int fb_keyboard_match_keycode (const char *buf, int length, const MmmKeyCode **ret)
{
  int i;
  int matches = 0;

  if (!strncmp (buf, "\e[M", MIN(length,3)))
    {
      if (length >= 6)
        return 9001;
      return 2342;
    }
  for (i = 0; ufb_keycodes[i].nick; i++)
    if (!strncmp (buf, ufb_keycodes[i].sequence, length))
      {
        matches ++;
        if ((int)strlen (ufb_keycodes[i].sequence) == length && ret)
          {
            *ret = &ufb_keycodes[i];
            return 1;
          }
      }
  if (matches != 1 && ret)
    *ret = NULL;
  return matches==1?2:matches;
}

//int is_active (void *host)
//{
//        return 1;
//}

static char *evsource_kb_get_event (void)
{
  unsigned char buf[20];
  int length;


  for (length = 0; length < 10; length ++)
    if (read (STDIN_FILENO, &buf[length], 1) != -1)
      {
        const MmmKeyCode *match = NULL;

        //if (!is_active (ctx_ev_src_kb.priv))
        //  return NULL;

        /* special case ESC, so that we can use it alone in keybindings */
        if (length == 0 && buf[0] == 27)
          {
            struct timeval tv;
            fd_set rfds;
            FD_ZERO (&rfds);
            FD_SET (STDIN_FILENO, &rfds);
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * 120;
            if (select (STDIN_FILENO+1, &rfds, NULL, NULL, &tv) == 0)
              return strdup ("escape");
          }

        switch (fb_keyboard_match_keycode ((void*)buf, length + 1, &match))
          {
            case 1: /* unique match */
              if (!match)
                return NULL;
              return strdup (match->nick);
              break;
            case 0: /* no matches, bail*/
             {
                static char ret[256]="";
                if (length == 0 && ctx_utf8_len (buf[0])>1) /* read a
                                                             * single unicode
                                                             * utf8 character
                                                             */
                  {
                    int bytes = read (STDIN_FILENO, &buf[length+1], ctx_utf8_len(buf[0])-1);
                    if (bytes)
                    {
                      buf[ctx_utf8_len(buf[0])]=0;
                      strcpy (ret, (void*)buf);
                    }
                    return strdup(ret); //XXX: simplify
                  }
                if (length == 0) /* ascii */
                  {
                    buf[1]=0;
                    strcpy (ret, (void*)buf);
                    return strdup(ret);
                  }
                sprintf (ret, "unhandled %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c' %i:'%c'",
                    length >=0 ? buf[0] : 0,
                    length >=0 ? buf[0]>31?buf[0]:'?' : ' ',
                    length >=1 ? buf[1] : 0,
                    length >=1 ? buf[1]>31?buf[1]:'?' : ' ',
                    length >=2 ? buf[2] : 0,
                    length >=2 ? buf[2]>31?buf[2]:'?' : ' ',
                    length >=3 ? buf[3] : 0,
                    length >=3 ? buf[3]>31?buf[3]:'?' : ' ',
                    length >=4 ? buf[4] : 0,
                    length >=4 ? buf[4]>31?buf[4]:'?' : ' ',
                    length >=5 ? buf[5] : 0,
                    length >=5 ? buf[5]>31?buf[5]:'?' : ' ',
                    length >=6 ? buf[6] : 0,
                    length >=6 ? buf[6]>31?buf[6]:'?' : ' '
                    );
                return strdup(ret);
            }
              return NULL;
            default: /* continue */
              break;
          }
      }
    else
      return strdup("key read eek");
  return strdup("fail");
}

static int evsource_kb_get_fd (void)
{
  return STDIN_FILENO;
}


static EvSource *evsource_kb_new (void)
{
  if (evsource_kb_init() == 0)
  {
    return &ctx_ev_src_kb;
  }
  return NULL;
}

static int event_check_pending (CtxFb *fb)
{
  CtxTiled *tiled = (void*)fb;
  int events = 0;
  for (int i = 0; i < fb->evsource_count; i++)
  {
    while (evsource_has_event (fb->evsource[i]))
    {
      char *event = evsource_get_event (fb->evsource[i]);
      if (event)
      {
        if (fb->vt_active)
        {
          ctx_key_press (tiled->ctx, 0, event, 0); // we deliver all events as key-press, the key_press handler disambiguates
          events++;
        }
        free (event);
      }
    }
  }
  return events;
}

int ctx_fb_consume_events (Ctx *ctx)
{
  CtxFb *fb = (void*)ctx->renderer;
  ctx_fb_show_frame (fb, 0);
  event_check_pending (fb);
  return 0;
}

inline static void ctx_fb_reset (CtxFb *fb)
{
  ctx_fb_show_frame (fb, 1);
}

inline static void ctx_fb_flush (CtxFb *fb)
{
  ctx_tiled_flush ((CtxTiled*)fb);
}

void ctx_fb_free (CtxFb *fb)
{
  if (fb->is_drm)
  {
    ctx_fbdrm_close (fb);
  }

  ioctl (0, KDSETMODE, KD_TEXT);
  if (system("stty sane")){};
  ctx_tiled_free ((CtxTiled*)fb);
  //free (fb);
#if CTX_BABL
  babl_exit ();
#endif
}

//static unsigned char *fb_icc = NULL;
//static long fb_icc_length = 0;

int ctx_renderer_is_fb (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_fb_free)
          return 1;
  return 0;
}

static CtxFb *ctx_fb = NULL;
static void vt_switch_cb (int sig)
{
  CtxTiled *tiled = (void*)ctx_fb;
  if (sig == SIGUSR1)
  {
    if (ctx_fb->is_drm)
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_DROP_MASTER, 0);
    ioctl (0, VT_RELDISP, 1);
    ctx_fb->vt_active = 0;
    ioctl (0, KDSETMODE, KD_TEXT);
  }
  else
  {
    ioctl (0, VT_RELDISP, VT_ACKACQ);
    ctx_fb->vt_active = 1;
    // queue draw
    tiled->render_frame = ++tiled->frame;
    ioctl (0, KDSETMODE, KD_GRAPHICS);
    if (ctx_fb->is_drm)
    {
      ioctl(ctx_fb->fb_fd, DRM_IOCTL_SET_MASTER, 0);
      ctx_fb_flip (ctx_fb);
    }
    else
    {
      tiled->ctx->dirty=1;

      for (int row = 0; row < CTX_HASH_ROWS; row++)
      for (int col = 0; col < CTX_HASH_COLS; col++)
      {
        tiled->hashes[(row * CTX_HASH_COLS + col) *  20] += 1;
      }
    }
  }
}

static int ctx_fb_get_mice_fd (Ctx *ctx)
{
  //CtxFb *fb = (void*)ctx->renderer;
  return _ctx_mice_fd;
}

Ctx *ctx_new_fb (int width, int height, int drm)
{
#if CTX_RASTERIZER
  CtxFb *fb = calloc (sizeof (CtxFb), 1);

  CtxTiled *tiled = (void*)fb;
  ctx_fb = fb;
  if (drm)
    fb->fb = ctx_fbdrm_new (fb, &tiled->width, &tiled->height);
  if (fb->fb)
  {
    fb->is_drm         = 1;
    width              = tiled->width;
    height             = tiled->height;
    /*
       we're ignoring the input width and height ,
       maybe turn them into properties - for
       more generic handling.
     */
    fb->fb_mapped_size = tiled->width * tiled->height * 4;
    fb->fb_bits        = 32;
    fb->fb_bpp         = 4;
  }
  else
  {
  fb->fb_fd = open ("/dev/fb0", O_RDWR);
  if (fb->fb_fd > 0)
    fb->fb_path = strdup ("/dev/fb0");
  else
  {
    fb->fb_fd = open ("/dev/graphics/fb0", O_RDWR);
    if (fb->fb_fd > 0)
    {
      fb->fb_path = strdup ("/dev/graphics/fb0");
    }
    else
    {
      free (fb);
      return NULL;
    }
  }

  if (ioctl(fb->fb_fd, FBIOGET_FSCREENINFO, &fb->finfo))
    {
      fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
    }

   if (ioctl(fb->fb_fd, FBIOGET_VSCREENINFO, &fb->vinfo))
     {
       fprintf (stderr, "error getting fbinfo\n");
      close (fb->fb_fd);
      free (fb->fb_path);
      free (fb);
      return NULL;
     }

//fprintf (stderr, "%s\n", fb->fb_path);
  width = tiled->width = fb->vinfo.xres;
  height = tiled->height = fb->vinfo.yres;

  fb->fb_bits = fb->vinfo.bits_per_pixel;
//fprintf (stderr, "fb bits: %i\n", fb->fb_bits);

  if (fb->fb_bits == 16)
    fb->fb_bits =
      fb->vinfo.red.length +
      fb->vinfo.green.length +
      fb->vinfo.blue.length;

   else if (fb->fb_bits == 8)
  {
    unsigned short red[256],  green[256],  blue[256];
    unsigned short original_red[256];
    unsigned short original_green[256];
    unsigned short original_blue[256];
    struct fb_cmap cmap = {0, 256, red, green, blue, NULL};
    struct fb_cmap original_cmap = {0, 256, original_red, original_green, original_blue, NULL};
    int i;

    /* do we really need to restore it ? */
    if (ioctl (fb->fb_fd, FBIOPUTCMAP, &original_cmap) == -1)
    {
      fprintf (stderr, "palette initialization problem %i\n", __LINE__);
    }

    for (i = 0; i < 256; i++)
    {
      red[i]   = ((( i >> 5) & 0x7) << 5) << 8;
      green[i] = ((( i >> 2) & 0x7) << 5) << 8;
      blue[i]  = ((( i >> 0) & 0x3) << 6) << 8;
    }

    if (ioctl (fb->fb_fd, FBIOPUTCMAP, &cmap) == -1)
    {
      fprintf (stderr, "palette initialization problem %i\n", __LINE__);
    }
  }

  fb->fb_bpp = fb->vinfo.bits_per_pixel / 8;
  fb->fb_mapped_size = fb->finfo.smem_len;
                                              
  fb->fb = mmap (NULL, fb->fb_mapped_size, PROT_READ|PROT_WRITE, MAP_SHARED, fb->fb_fd, 0);
  }
  if (!fb->fb)
    return NULL;
  tiled->pixels = calloc (fb->fb_mapped_size, 1);
  ctx_fb_events = 1;

#if CTX_BABL
  babl_init ();
#endif

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);

  tiled->ctx      = ctx_new ();
  tiled->ctx_copy = ctx_new ();
  tiled->width    = width;
  tiled->height   = height;

  ctx_set_renderer (tiled->ctx, fb);
  ctx_set_renderer (tiled->ctx_copy, fb);
  ctx_set_texture_cache (tiled->ctx_copy, tiled->ctx);

  ctx_set_size (tiled->ctx, width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

  tiled->flush = (void*)ctx_fb_flush;
  tiled->reset = (void*)ctx_fb_reset;
  tiled->free  = (void*)ctx_fb_free;
  tiled->set_clipboard = (void*)ctx_fb_set_clipboard;
  tiled->get_clipboard = (void*)ctx_fb_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                   tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                   tiled->width * 4, CTX_FORMAT_RGBA8); // this format
                                  // is overriden in  thread
    ((CtxRasterizer*)(tiled->host[i]->renderer))->swap_red_green = 1;
    ctx_set_texture_source (tiled->host[i], tiled->ctx);
  }

  mtx_init (&tiled->mtx, mtx_plain);
  cnd_init (&tiled->cond);

#define start_thread(no)\
  if(_ctx_max_threads>no){ \
    static void *args[2]={(void*)no, };\
    thrd_t tid;\
    args[1]=fb;\
    thrd_create (&tid, (void*)ctx_tiled_render_fun, args);\
  }
  start_thread(0);
  start_thread(1);
  start_thread(2);
  start_thread(3);
  start_thread(4);
  start_thread(5);
  start_thread(6);
  start_thread(7);
  start_thread(8);
  start_thread(9);
  start_thread(10);
  start_thread(11);
  start_thread(12);
  start_thread(13);
  start_thread(14);
  start_thread(15);
#undef start_thread

  ctx_flush (tiled->ctx);

  EvSource *kb = evsource_kb_new ();
  if (kb)
  {
    fb->evsource[fb->evsource_count++] = kb;
    kb->priv = fb;
  }
  EvSource *mice  = evsource_mice_new ();
  if (mice)
  {
    fb->evsource[fb->evsource_count++] = mice;
    mice->priv = fb;
  }

  fb->vt_active = 1;
  ioctl(0, KDSETMODE, KD_GRAPHICS);
  signal (SIGUSR1, vt_switch_cb);
  signal (SIGUSR2, vt_switch_cb);
  struct vt_stat st;
  if (ioctl (0, VT_GETSTATE, &st) == -1)
  {
    ctx_log ("VT_GET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }

  fb->vt = st.v_active;

  struct vt_mode mode;
  mode.mode   = VT_PROCESS;
  mode.relsig = SIGUSR1;
  mode.acqsig = SIGUSR2;
  if (ioctl (0, VT_SETMODE, &mode) < 0)
  {
    ctx_log ("VT_SET_MODE on vt %i failed\n", fb->vt);
    return NULL;
  }

  return tiled->ctx;
#else
  return NULL;
#endif
}
#else

int ctx_renderer_is_fb (Ctx *ctx)
{
  return 0;
}
#endif
#endif

#if CTX_SDL

/**/

typedef struct _CtxSDL CtxSDL;
struct _CtxSDL
{
   CtxTiled  tiled;
   /* where we diverge from fb*/
   int           key_balance;
   int           key_repeat;
   int           lctrl;
   int           lalt;
   int           rctrl;
   int           lshift;
   int           rshift;

   SDL_Window   *window;
   SDL_Renderer *renderer;
   SDL_Texture  *texture;

// cnd_t  cond;
// mtx_t  mtx;
   int           fullscreen;
};

#include "stb_image_write.h"

void ctx_screenshot (Ctx *ctx, const char *output_path)
{
#if CTX_SCREENSHOT
  int valid = 0;
  CtxSDL *sdl = (void*)ctx->renderer;

  if (ctx_renderer_is_sdl (ctx)) valid = 1;
#if CTX_FB
  if (ctx_renderer_is_fb  (ctx)) valid = 1;
#endif

  if (!valid)
    return;

#if CTX_FB
  // we rely on the same layout
  for (int i = 0; i < sdl->width * sdl->height; i++)
  {
    int tmp = sdl->pixels[i*4];
    sdl->pixels[i*4] = sdl->pixels[i*4 + 2];
    sdl->pixels[i*4 + 2] = tmp;
  }
#endif

  stbi_write_png (output_path, sdl->width, sdl->height, 4, sdl->pixels, sdl->width*4);

#if CTX_FB
  for (int i = 0; i < sdl->width * sdl->height; i++)
  {
    int tmp = sdl->pixels[i*4];
    sdl->pixels[i*4] = sdl->pixels[i*4 + 2];
    sdl->pixels[i*4 + 2] = tmp;
  }
#endif
#endif
}

void ctx_sdl_set_title (void *self, const char *new_title)
{
   CtxSDL *sdl = self;
   SDL_SetWindowTitle (sdl->window, new_title);
}

static void ctx_sdl_show_frame (CtxSDL *sdl, int block)
{
  CtxTiled *tiled = &sdl->tiled;
  if (tiled->shown_cursor != tiled->ctx->cursor)
  {
    tiled->shown_cursor = tiled->ctx->cursor;
    SDL_Cursor *new_cursor =  NULL;
    switch (tiled->shown_cursor)
    {
      case CTX_CURSOR_UNSET: // XXX: document how this differs from none
                             //      perhaps falling back to arrow?
        break;
      case CTX_CURSOR_NONE:
        new_cursor = NULL;
        break;
      case CTX_CURSOR_ARROW:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        break;
      case CTX_CURSOR_CROSSHAIR:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        break;
      case CTX_CURSOR_WAIT:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
        break;
      case CTX_CURSOR_HAND:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
        break;
      case CTX_CURSOR_IBEAM:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
        break;
      case CTX_CURSOR_MOVE:
      case CTX_CURSOR_RESIZE_ALL:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        break;
      case CTX_CURSOR_RESIZE_N:
      case CTX_CURSOR_RESIZE_S:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        break;
      case CTX_CURSOR_RESIZE_E:
      case CTX_CURSOR_RESIZE_W:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        break;
      case CTX_CURSOR_RESIZE_NE:
      case CTX_CURSOR_RESIZE_SW:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
        break;
      case CTX_CURSOR_RESIZE_NW:
      case CTX_CURSOR_RESIZE_SE:
        new_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
        break;
    }
    if (new_cursor)
    {
      SDL_Cursor *old_cursor = SDL_GetCursor();
      SDL_SetCursor (new_cursor);
      SDL_ShowCursor (1);
      if (old_cursor)
        SDL_FreeCursor (old_cursor);
    }
    else
    {
      SDL_ShowCursor (0);
    }
  }

  if (tiled->shown_frame == tiled->render_frame)
  {
    return;
  }

  if (block)
  {
    int count = 0;
    while (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
    {
      usleep (50);
      count ++;
      if (count > 2000)
      {
        tiled->shown_frame = tiled->render_frame;
        return;
      }
    }
  }
  else
  {
    if (ctx_tiled_threads_done (tiled) != _ctx_max_threads)
      return;
  }

  if (tiled->min_row == 100)
  {
  }
  else
  {
#if 1
    int x = tiled->min_col * tiled->width/CTX_HASH_COLS;
    int y = tiled->min_row * tiled->height/CTX_HASH_ROWS;
    int x1 = (tiled->max_col+1) * tiled->width/CTX_HASH_COLS;
    int y1 = (tiled->max_row+1) * tiled->height/CTX_HASH_ROWS;
    int width = x1 - x;
    int height = y1 - y;
#endif
    tiled->min_row = 100;
    tiled->max_row = 0;
    tiled->min_col = 100;
    tiled->max_col = 0;

    SDL_Rect r = {x, y, width, height};
    SDL_UpdateTexture (sdl->texture, &r,
                      //(void*)sdl->pixels,
                      (void*)(tiled->pixels + y * tiled->width * 4 + x * 4),
                      
                      tiled->width * 4);
    SDL_RenderClear (sdl->renderer);
    SDL_RenderCopy (sdl->renderer, sdl->texture, NULL, NULL);
    SDL_RenderPresent (sdl->renderer);
  }
  tiled->shown_frame = tiled->render_frame;
}

static const char *ctx_sdl_keysym_to_name (unsigned int sym, int *r_keycode)
{
  static char buf[16]="";
  buf[ctx_unichar_to_utf8 (sym, (void*)buf)]=0;
  int code = sym;
  const char *name = &buf[0];
   switch (sym)
   {
     case SDLK_RSHIFT: code = 16 ; break;
     case SDLK_LSHIFT: code = 16 ; break;
     case SDLK_LCTRL: code = 17 ; break;
     case SDLK_RCTRL: code = 17 ; break;
     case SDLK_LALT:  code = 18 ; break;
     case SDLK_RALT:  code = 18 ; break;
     case SDLK_CAPSLOCK: name = "capslock"; code = 20 ; break;
     //case SDLK_NUMLOCK: name = "numlock"; code = 144 ; break;
     //case SDLK_SCROLLLOCK: name = "scrollock"; code = 145 ; break;

     case SDLK_F1:     name = "F1"; code = 112; break;
     case SDLK_F2:     name = "F2"; code = 113; break;
     case SDLK_F3:     name = "F3"; code = 114; break;
     case SDLK_F4:     name = "F4"; code = 115; break;
     case SDLK_F5:     name = "F5"; code = 116; break;
     case SDLK_F6:     name = "F6"; code = 117; break;
     case SDLK_F7:     name = "F7"; code = 118; break;
     case SDLK_F8:     name = "F8"; code = 119; break;
     case SDLK_F9:     name = "F9"; code = 120; break;
     case SDLK_F10:    name = "F10"; code = 121; break;
     case SDLK_F11:    name = "F11"; code = 122; break;
     case SDLK_F12:    name = "F12"; code = 123; break;
     case SDLK_ESCAPE: name = "escape"; break;
     case SDLK_DOWN:   name = "down"; code = 40; break;
     case SDLK_LEFT:   name = "left"; code = 37; break;
     case SDLK_UP:     name = "up"; code = 38;  break;
     case SDLK_RIGHT:  name = "right"; code = 39; break;
     case SDLK_BACKSPACE: name = "backspace"; break;
     case SDLK_SPACE:  name = "space"; break;
     case SDLK_TAB:    name = "tab"; break;
     case SDLK_DELETE: name = "delete"; code = 46; break;
     case SDLK_INSERT: name = "insert"; code = 45; break;
     case SDLK_RETURN:
       //if (key_repeat == 0) // return never should repeat
       name = "return";   // on a DEC like terminal
       break;
     case SDLK_HOME:     name = "home"; code = 36; break;
     case SDLK_END:      name = "end"; code = 35; break;
     case SDLK_PAGEDOWN: name = "page-down"; code = 34; break;
     case SDLK_PAGEUP:   name = "page-up"; code = 33; break;
     case ',': code = 188; break;
     case '.': code = 190; break;
     case '/': code = 191; break;
     case '`': code = 192; break;
     case '[': code = 219; break;
     case '\\': code = 220; break;
     case ']':  code = 221; break;
     case '\'': code = 222; break;
     default:
       ;
   }
   if (sym >= 'a' && sym <='z') code -= 32;
   if (r_keycode)
   {
     *r_keycode = code;
   }
   return name;
}

int ctx_sdl_consume_events (Ctx *ctx)
{
  CtxTiled *tiled = (void*)ctx->renderer;
  CtxSDL *sdl = (void*)ctx->renderer;
  SDL_Event event;
  int got_events = 0;

  ctx_sdl_show_frame (sdl, 0);

  while (SDL_PollEvent (&event))
  {
    got_events ++;
    switch (event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
        SDL_CaptureMouse (1);
        ctx_pointer_press (ctx, event.button.x, event.button.y, event.button.button, 0);
        break;
      case SDL_MOUSEBUTTONUP:
        SDL_CaptureMouse (0);
        ctx_pointer_release (ctx, event.button.x, event.button.y, event.button.button, 0);
        break;
      case SDL_MOUSEMOTION:
        //  XXX : look at mask and generate motion for each pressed
        //        button
        ctx_pointer_motion (ctx, event.motion.x, event.motion.y, 1, 0);
        break;
      case SDL_FINGERMOTION:
        ctx_pointer_motion (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height,
            (event.tfinger.fingerId%10) + 4, 0);
        break;
      case SDL_FINGERDOWN:
        {
        static int fdowns = 0;
        fdowns ++;
        if (fdowns > 1) // the very first finger down from SDL seems to be
                        // mirrored as mouse events, later ones not - at
                        // least under wayland
        {
          ctx_pointer_press (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height, 
          (event.tfinger.fingerId%10) + 4, 0);
        }
        }
        break;
      case SDL_FINGERUP:
        ctx_pointer_release (ctx, event.tfinger.x * tiled->width, event.tfinger.y * tiled->height,
          (event.tfinger.fingerId%10) + 4, 0);
        break;
#if 1
      case SDL_TEXTINPUT:
    //  if (!active)
    //    break;
        if (!sdl->lctrl && !sdl->rctrl && !sdl->lalt 
           //&& ( (vt && vt_keyrepeat (vt) ) || (key_repeat==0) )
           )
          {
            const char *name = event.text.text;
            int keycode = 0;
            if (!strcmp (name, " ") ) { name = "space"; }
            if (name[0] && name[1] == 0)
            {
              keycode = name[0];
              keycode = toupper (keycode);
              switch (keycode)
              {
                case '.':  keycode = 190; break;
                case ';':  keycode = 59; break;
                case ',':  keycode = 188; break;
                case '/':  keycode = 191; break;
                case '\'': keycode = 222; break;
                case '`':  keycode = 192; break;
                case '[':  keycode = 219; break;
                case ']':  keycode = 221; break;
                case '\\': keycode = 220; break;
              }
            }
            ctx_key_press (ctx, keycode, name, 0);
            //got_event = 1;
          }
        break;
#endif
      case SDL_KEYDOWN:
        {
          char buf[32] = "";
          const char *name = buf;
          if (!event.key.repeat)
          {
            sdl->key_balance ++;
            sdl->key_repeat = 0;
          }
          else
          {
            sdl->key_repeat ++;
          }
          switch (event.key.keysym.sym)
          {
            case SDLK_LSHIFT: sdl->lshift = 1; break;
            case SDLK_RSHIFT: sdl->rshift = 1; break;
            case SDLK_LCTRL:  sdl->lctrl = 1; break;
            case SDLK_LALT:   sdl->lalt = 1; break;
            case SDLK_RCTRL:  sdl->rctrl = 1; break;
          }
          if (sdl->lshift | sdl->rshift | sdl->lctrl | sdl->lalt | sdl->rctrl)
          {
            ctx->events.modifier_state ^= ~(CTX_MODIFIER_STATE_CONTROL|
                                            CTX_MODIFIER_STATE_ALT|
                                            CTX_MODIFIER_STATE_SHIFT);
            if (sdl->lshift | sdl->rshift)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_SHIFT;
            if (sdl->lctrl | sdl->rctrl)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_CONTROL;
            if (sdl->lalt)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_ALT;
          }
          int keycode;
          name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);
          ctx_key_down (ctx, keycode, name, 0);

          if (strlen (name)
              &&(event.key.keysym.mod & (KMOD_CTRL) ||
                 event.key.keysym.mod & (KMOD_ALT) ||
                 strlen (name) >= 2))
          {
            if (event.key.keysym.mod & (KMOD_CTRL) )
              {
                static char buf[64] = "";
                sprintf (buf, "control-%s", name);
                name = buf;
              }
            if (event.key.keysym.mod & (KMOD_ALT) )
              {
                static char buf[128] = "";
                sprintf (buf, "alt-%s", name);
                name = buf;
              }
            if (event.key.keysym.mod & (KMOD_SHIFT) )
              {
                static char buf[196] = "";
                sprintf (buf, "shift-%s", name);
                name = buf;
              }
            if (strcmp (name, "space"))
              {
               ctx_key_press (ctx, keycode, name, 0);
              }
          }
          else
          {
#if 0
             ctx_key_press (ctx, 0, buf, 0);
#endif
          }
        }
        break;
      case SDL_KEYUP:
        {
           sdl->key_balance --;
           switch (event.key.keysym.sym)
           {
             case SDLK_LSHIFT: sdl->lshift = 0; break;
             case SDLK_RSHIFT: sdl->rshift = 0; break;
             case SDLK_LCTRL: sdl->lctrl = 0; break;
             case SDLK_RCTRL: sdl->rctrl = 0; break;
             case SDLK_LALT:  sdl->lalt  = 0; break;
           }

          {
            ctx->events.modifier_state ^= ~(CTX_MODIFIER_STATE_CONTROL|
                                            CTX_MODIFIER_STATE_ALT|
                                            CTX_MODIFIER_STATE_SHIFT);
            if (sdl->lshift | sdl->rshift)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_SHIFT;
            if (sdl->lctrl | sdl->rctrl)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_CONTROL;
            if (sdl->lalt)
              ctx->events.modifier_state |= CTX_MODIFIER_STATE_ALT;
          }

           int keycode;
           const char *name = ctx_sdl_keysym_to_name (event.key.keysym.sym, &keycode);
           ctx_key_up (ctx, keycode, name, 0);
        }
        break;
      case SDL_QUIT:
        ctx_quit (ctx);
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_RESIZED)
        {
          ctx_sdl_show_frame (sdl, 1);
          int width = event.window.data1;
          int height = event.window.data2;
          SDL_DestroyTexture (sdl->texture);
          sdl->texture = SDL_CreateTexture (sdl->renderer, SDL_PIXELFORMAT_ABGR8888,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
          free (tiled->pixels);
          tiled->pixels = calloc (4, width * height);

          tiled->width  = width;
          tiled->height = height;
          ctx_set_size (tiled->ctx, width, height);
          ctx_set_size (tiled->ctx_copy, width, height);
        }
        break;
    }
  }
  return 1;
}
#else
void ctx_screenshot (Ctx *ctx, const char *path)
{
}
#endif

#if CTX_SDL

static void ctx_sdl_set_clipboard (CtxSDL *sdl, const char *text)
{
  if (text)
    SDL_SetClipboardText (text);
}

static char *ctx_sdl_get_clipboard (CtxSDL *sdl)
{
  return SDL_GetClipboardText ();
}

inline static void ctx_sdl_reset (CtxSDL *sdl)
{
  ctx_sdl_show_frame (sdl, 1);
}

inline static void ctx_sdl_flush (CtxSDL *sdl)
{
  ctx_tiled_flush ((void*)sdl);
  //CtxTiled *tiled = (void*)sdl;
}

void ctx_sdl_free (CtxSDL *sdl)
{

  if (sdl->texture)
  SDL_DestroyTexture (sdl->texture);
  if (sdl->renderer)
  SDL_DestroyRenderer (sdl->renderer);
  if (sdl->window)
  SDL_DestroyWindow (sdl->window);

  ctx_tiled_free ((CtxTiled*)sdl);
#if CTX_BABL
  babl_exit ();
#endif
}


int ctx_renderer_is_sdl (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_sdl_free)
          return 1;
  return 0;
}

void ctx_sdl_set_fullscreen (Ctx *ctx, int val)
{
  CtxSDL *sdl = (void*)ctx->renderer;

  if (val)
  {
    SDL_SetWindowFullscreen (sdl->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
  else
  {
    SDL_SetWindowFullscreen (sdl->window, 0);
  }
  // XXX we're presuming success
  sdl->fullscreen = val;
}
int ctx_sdl_get_fullscreen (Ctx *ctx)
{
  CtxSDL *sdl = (void*)ctx->renderer;
  return sdl->fullscreen;
}


Ctx *ctx_new_sdl (int width, int height)
{
#if CTX_RASTERIZER

  CtxSDL *sdl = (CtxSDL*)calloc (sizeof (CtxSDL), 1);
  CtxTiled *tiled = (void*)sdl;

  ctx_get_contents ("file:///tmp/ctx.icc", &sdl_icc, &sdl_icc_length);
  if (width <= 0 || height <= 0)
  {
    width  = 1920;
    height = 1080;
  }
  sdl->window = SDL_CreateWindow("ctx", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN|SDL_WINDOW_RESIZABLE);
  //sdl->renderer = SDL_CreateRenderer (sdl->window, -1, SDL_RENDERER_SOFTWARE);
  sdl->renderer = SDL_CreateRenderer (sdl->window, -1, 0);
  if (!sdl->renderer)
  {
     ctx_free (tiled->ctx);
     free (sdl);
     return NULL;
  }
#if CTX_BABL
  babl_init ();
#endif
  sdl->fullscreen = 0;

  ctx_sdl_events = 1;
  sdl->texture = SDL_CreateTexture (sdl->renderer,
        SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        width, height);

  SDL_StartTextInput ();
  SDL_EnableScreenSaver ();

  tiled->ctx      = ctx_new ();
  tiled->ctx_copy = ctx_new ();
  tiled->width    = width;
  tiled->height   = height;
  tiled->cols     = 80;
  tiled->rows     = 20;
  ctx_set_renderer (tiled->ctx, sdl);
  ctx_set_renderer (tiled->ctx_copy, sdl);
  ctx_set_texture_cache (tiled->ctx_copy, tiled->ctx);

  tiled->pixels = (uint8_t*)malloc (width * height * 4);

  ctx_set_size (tiled->ctx,      width, height);
  ctx_set_size (tiled->ctx_copy, width, height);

  tiled->flush = (void*)ctx_sdl_flush;
  tiled->reset = (void*)ctx_sdl_reset;
  tiled->free  = (void*)ctx_sdl_free;
  tiled->set_clipboard = (void*)ctx_sdl_set_clipboard;
  tiled->get_clipboard = (void*)ctx_sdl_get_clipboard;

  for (int i = 0; i < _ctx_max_threads; i++)
  {
    tiled->host[i] = ctx_new_for_framebuffer (tiled->pixels,
                     tiled->width/CTX_HASH_COLS, tiled->height/CTX_HASH_ROWS,
                     tiled->width * 4, CTX_FORMAT_RGBA8);
    ctx_set_texture_source (tiled->host[i], tiled->ctx);
  }

  mtx_init (&tiled->mtx, mtx_plain);
  cnd_init (&tiled->cond);

#define start_thread(no)\
  if(_ctx_max_threads>no){ \
    static void *args[2]={(void*)no, };\
    thrd_t tid;\
    args[1]=sdl;\
    thrd_create (&tid, (void*)ctx_tiled_render_fun, args);\
  }
  start_thread(0);
  start_thread(1);
  start_thread(2);
  start_thread(3);
  start_thread(4);
  start_thread(5);
  start_thread(6);
  start_thread(7);
  start_thread(8);
  start_thread(9);
  start_thread(10);
  start_thread(11);
  start_thread(12);
  start_thread(13);
  start_thread(14);
  start_thread(15);
#undef start_thread

  ctx_flush (tiled->ctx);
  return tiled->ctx;
#else
  return NULL;
#endif
}
#else

int ctx_renderer_is_sdl (Ctx *ctx)
{
  return 0;
}
#endif

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

typedef struct CtxTermCell
{
  char    utf8[5];
  uint8_t fg[4];
  uint8_t bg[4];

  char    prev_utf8[5];
  uint8_t prev_fg[4];
  uint8_t prev_bg[4];
} CtxTermCell;

typedef struct CtxTermLine
{
  CtxTermCell *cells;
  int maxcol;
  int size;
} CtxTermLine;

typedef enum
{
  CTX_TERM_ASCII,
  CTX_TERM_ASCII_MONO,
  CTX_TERM_SEXTANT,
  CTX_TERM_BRAILLE_MONO,
  CTX_TERM_BRAILLE,
  CTX_TERM_QUARTER,
} CtxTermMode;

typedef struct _CtxTerm CtxTerm;
struct _CtxTerm
{
   void (*render) (void *term, CtxCommand *command);
   void (*reset)  (void *term);
   void (*flush)  (void *term);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard) (void *ctxctx, const char *text);
   void (*free)   (void *term);
   Ctx      *ctx;
   int       width;
   int       height;
   int       cols;
   int       rows;
   int       was_down;

   uint8_t  *pixels;

   Ctx      *host;
   CtxList  *lines;
   CtxTermMode mode;
};

static int ctx_term_ch = 8;
static int ctx_term_cw = 8;

void ctx_term_set (CtxTerm *term,
                      int col, int row, const char *utf8,
                      uint8_t *fg, uint8_t *bg)
{
  if (col < 1 || row < 1 || col > term->cols  || row > term->rows) return;
  while (ctx_list_length (term->lines) < row)
  {
    ctx_list_append (&term->lines, calloc (sizeof (CtxTermLine), 1));
  }
  CtxTermLine *line = ctx_list_nth_data (term->lines, row-1);
  assert (line);
  if (line->size < col)
  {
     int new_size = ((col + 128)/128)*128;
     line->cells = realloc (line->cells, sizeof (CtxTermCell) * new_size);
     memset (&line->cells[line->size], 0, sizeof (CtxTermCell) * (new_size - line->size) );
     line->size = new_size;
  }
  if (col > line->maxcol) line->maxcol = col;
  strncpy (line->cells[col-1].utf8, (char*)utf8, 4);
  memcpy  (line->cells[col-1].fg, fg, 4);
  memcpy  (line->cells[col-1].bg, bg, 4);
}

static int _ctx_term256 = 0; // XXX TODO implement autodetect for this
static long _ctx_curfg = -1;
static long _ctx_curbg = -1;

static long ctx_rgb_to_long (int r,int g, int b)
{
  return r * 256 * 256 + g * 256 + b;
}


static void ctx_term_set_fg (int red, int green, int blue)
{
  long lc = ctx_rgb_to_long (red, green, blue);
  if (lc == _ctx_curfg)
    return;
  _ctx_curfg=lc;
  if (_ctx_term256 == 0)
  {
    printf("\e[38;2;%i;%i;%im", red,green,blue);
  }
  else
  {
    int gray = (green /255.0) * 24 + 0.5;
    int r    = (red/255.0)    * 6 + 0.5;
    int g    = (green/255.0)  * 6 + 0.5;
    int b    = (blue/255.0)   * 6 + 0.5;
    if (gray > 23) gray = 23;

    if (r > 5) r = 5;
    if (g > 5) g = 5;
    if (b > 5) b = 5;

    if (((int)(r/1.66)== (int)(g/1.66)) && ((int)(g/1.66) == ((int)(b/1.66))))
    {
      printf("\e[38;5;%im", 16 + 216 + gray);
    }
    else
      printf("\e[38;5;%im", 16 + r * 6 * 6 + g * 6  + b);
  }
}

static void ctx_term_set_bg(int red, int green, int blue)
{
  long lc = ctx_rgb_to_long (red, green, blue);
//if (lc == _ctx_curbg)
//  return;
  _ctx_curbg=lc;
  if (_ctx_term256 == 0)
  {
    printf("\e[48;2;%i;%i;%im", red,green,blue);
  }
  else
  {
    int gray = (green /255.0) * 24 + 0.5;
    int r    = (red/255.0)    * 6 + 0.5;
    int g    = (green/255.0)  * 6 + 0.5;
    int b    = (blue/255.0)   * 6 + 0.5;
    if (gray > 23) gray = 23;

    if (r > 5) r = 5;
    if (g > 5) g = 5;
    if (b > 5) b = 5;

    if (((int)(r/1.66)== (int)(g/1.66)) && ((int)(g/1.66) == ((int)(b/1.66))))
    {
      printf("\e[48;5;%im", 16 + 216 + gray);
    }
    else
      printf("\e[48;5;%im", 16 + r * 6 * 6 + g * 6  + b);
  }
}

static int _ctx_term_force_full = 0;

void ctx_term_scanout (CtxTerm *term)
{
  int row = 1;
  printf ("\e[H");
//  printf ("\e[?25l");
  printf ("\e[0m");
  for (CtxList *l = term->lines; l; l = l->next)
  {
    CtxTermLine *line = l->data;
    for (int col = 1; col <= line->maxcol; col++)
    {
      CtxTermCell *cell = &line->cells[col-1];

      if (strcmp(cell->utf8, cell->prev_utf8) ||
          memcmp(cell->fg, cell->prev_fg, 3) ||
          memcmp(cell->bg, cell->prev_bg, 3) || _ctx_term_force_full)
      {
        ctx_term_set_fg (cell->fg[0], cell->fg[1], cell->fg[2]);
        ctx_term_set_bg (cell->bg[0], cell->bg[1], cell->bg[2]);
        printf ("%s", cell->utf8);
      }
      else
      {
        // TODO: accumulate succesive such, and compress them
        // into one
        printf ("\e[C");
      }
      strcpy (cell->prev_utf8, cell->utf8);
      memcpy (cell->prev_fg, cell->fg, 3);
      memcpy (cell->prev_bg, cell->bg, 3);
    }
    if (row != term->rows)
      printf ("\n\r");
    row ++;
  }
  printf ("\e[0m");
  //printf ("\e[?25h");
  //
}

// xx
// xx
// xx
//

static inline int _ctx_rgba8_manhattan_diff (const uint8_t *a, const uint8_t *b)
{
  int c;
  int diff = 0;
  for (c = 0; c<3;c++)
    diff += ctx_pow2(a[c]-b[c]);
  return sqrtf(diff);
  return diff;
}

static void ctx_term_output_buf_half (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;
  const char *sextants[]={
   " ","▘","▝","▀","▖","▌", "▞", "▛", "▗", "▚", "▐", "▜","▄","▙","▟","█",

  };
  for (int row = 0; row < height/2; row++)
    {
      for (int col = 0; col < width-3; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};
          int i = 0;

          int  rgbasum[2][4] = {0,};
          int  sumcount[2];

          int curdiff = 0;
          /* first find starting point colors */
          for (int yi = 0; yi < ctx_term_ch; yi++)
            for (int xi = 0; xi < ctx_term_cw; xi++, i++)
                {
                  int noi = (row * ctx_term_ch + yi) * stride + (col*ctx_term_cw+xi) * 4;

                  if (rgba[0][3] == 0)
                  {
                    for (int c = 0; c < 3; c++)
                      rgba[0][c] = pixels[noi + c];
                    rgba[0][3] = 255; // used only as mark of in-use
                  }
                  else
                  {
                    int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[0]);
                    if (diff > curdiff)
                    {
                      curdiff = diff;
                      for (int c = 0; c < 3; c++)
                        rgba[1][c] = pixels[noi + c];
                    }
                  }

                }

          for (int iters = 0; iters < 1; iters++)
          {
                  i= 0;
          for (int i = 0; i < 4; i ++)
             rgbasum[0][i] = rgbasum[1][i]=0;
          sumcount[0] = sumcount[1] = 0;

          for (int yi = 0; yi < ctx_term_ch; yi++)
            for (int xi = 0; xi < ctx_term_cw; xi++, i++)
                {
                  int noi = (row * ctx_term_ch + yi) * stride + (col*ctx_term_cw+xi) * 4;

                  int diff1 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[0]);
                  int diff2 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[1]);
                  int cluster = 0;
                  if (diff1 <= diff2)
                    cluster = 0;
                  else
                    cluster = 1;
                  sumcount[cluster]++;
                  for (int c = 0; c < 3; c++)
                    rgbasum[cluster][c] += pixels[noi+c];
                }


          if (sumcount[0])
          for (int c = 0; c < 3; c++)
          {
            rgba[0][c] = rgbasum[0][c] / sumcount[0];
          }
          if (sumcount[1])
          for (int c = 0; c < 3; c++)
          {
            rgba[1][c] = rgbasum[1][c] / sumcount[1];
          }
          }

          int pixels_set = 0;
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }
           if (pixels_set == 4)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}

void ctx_term_find_color_pair (CtxTerm *term, int x0, int y0, int w, int h,
                uint8_t rgba[2][4])
        //uint8_t *rgba0, uint8_t *rgba1)
{
int curdiff = 0;
int stride = term->width * 4;
uint8_t *pixels = term->pixels;
/* first find starting point colors */
for (int y = y0; y < y0 + h; y++)
  for (int x = x0; x < x0 + w; x++)
      {
        int noi = (y) * stride + (x) * 4;

        if (rgba[0][3] == 0)
        {
          for (int c = 0; c < 3; c++)
            rgba[0][c] = pixels[noi + c];
          rgba[0][3] = 255; // used only as mark of in-use
        }
        else
        {
          int diff = _ctx_rgba8_manhattan_diff (&pixels[noi], &rgba[0][0]);
          if (diff > curdiff)
          {
            curdiff = diff;
            for (int c = 0; c < 3; c++)
              rgba[1][c] = pixels[noi + c];
          }
        }
      }
          int  rgbasum[2][4] = {0,};
          int  sumcount[2];

          for (int iters = 0; iters < 1; iters++)
          {
          for (int i = 0; i < 4; i ++)
             rgbasum[0][i] = rgbasum[1][i]=0;
          sumcount[0] = sumcount[1] = 0;

          for (int y = y0; y < y0 + h; y++)
            for (int x = x0; x < x0 + w; x++)
                {
                  int noi = (y) * stride + (x) * 4;

                  int diff1 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[0]);
                  int diff2 = _ctx_rgba8_manhattan_diff (&pixels[noi], rgba[1]);
                  int cluster = 0;
                  if (diff1 <= diff2)
                    cluster = 0;
                  else
                    cluster = 1;
                  sumcount[cluster]++;
                  for (int c = 0; c < 3; c++)
                    rgbasum[cluster][c] += pixels[noi+c];
                }


          if (sumcount[0])
          for (int c = 0; c < 3; c++)
          {
            rgba[0][c] = rgbasum[0][c] / sumcount[0];
          }
          if (sumcount[1])
          for (int c = 0; c < 3; c++)
          {
            rgba[1][c] = rgbasum[1][c] / sumcount[1];
          }
          }

}



static void ctx_term_output_buf_quarter (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;
  const char *sextants[]={
   " ","▘","▝","▀","▖","▌", "▞", "▛", "▗", "▚", "▐", "▜","▄","▙","▟","█"

  };
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};
          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);

          int pixels_set = 0;
          for (int y = 0; y < 2; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }
           if (pixels_set == 4)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}


static void ctx_term_output_buf_sextant (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term)
{
  int stride = width * 4;

  const char *sextants[]={
   " ","🬀","🬁","🬂","🬃","🬄","🬅","🬆","🬇","🬈","🬉","🬊","🬋","🬌","🬍","🬎","🬏","🬐","🬑","🬒","🬓","▌","🬔","🬕","🬖","🬗","🬘","🬙","🬚","🬛","🬜","🬝","🬞","🬟","🬠","🬡","🬢","🬣","🬤","🬥","🬦","🬧","▐","🬨","🬩","🬪","🬫","🬬","🬭","🬮","🬯","🬰","🬱","🬲","🬳","🬴","🬵","🬶","🬷","🬸","🬹","🬺","🬻","█"
  };

  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);

          int pixels_set = 0;
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }

          if (pixels_set == 6)
            ctx_term_set (term, col +1, row + 1, " ",
                          rgba[1], rgba[0]);
          else
            ctx_term_set (term, col +1, row + 1, sextants[unicode], rgba[0], rgba[1]);
        }
    }
}

static void ctx_term_output_buf_ascii (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term,
                          int mono)
{
  /* this is a crude ascii-mode built on a quick mapping of sexels to ascii */
  int stride = width * 4;
  const char *sextants[]={
   " ","`","'","^","🬃","`","~","\"","-","\"","'","\"","-","\"","~","^",",",";",
   "=","/","i","[","p","P","z",")","/","7","f",">","/","F",",","\\",":",":",
   "\\","\\","(","T","j","T","]","?","s","\\","<","q","_","=","=","=","c","L",
   "Q","C","a","b","J","]","m","b","d","@"
  };
  uint8_t black[4] = {0,0,0,255};
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);


          if (_ctx_rgba8_manhattan_diff (black, rgba[1]) >
              _ctx_rgba8_manhattan_diff (black, rgba[0]))
          {
            for (int c = 0; c < 4; c ++)
            {
              int tmp = rgba[0][c];
              rgba[0][c] = rgba[1][c];
              rgba[1][c] = tmp;
            }
          }
          if (mono)
          {
            rgba[1][0] = 0;
            rgba[1][1] = 0;
            rgba[1][2] = 0;
          }


          int brightest_dark_diff = _ctx_rgba8_manhattan_diff (black, rgba[0]);

          int pixels_set = 0;
          for (int y = 0; y < ctx_term_ch; y++)
            for (int x = 0; x < ctx_term_cw; x++)
              {
                int no = (row * ctx_term_ch + y) * stride + (col*ctx_term_cw+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
#undef CHECK_IS_SET
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }


           if (pixels_set == 6 && brightest_dark_diff < 40)
             ctx_term_set (term, col +1, row + 1, " ",
                           rgba[1], rgba[0]);
           else
             ctx_term_set (term, col +1, row + 1, sextants[unicode],
                           rgba[0], rgba[1]);
        }
    }
}

static void ctx_term_output_buf_braille (uint8_t *pixels,
                          int width,
                          int height,
                          CtxTerm *term,
                          int mono)
{
  int reverse = 0;
  int stride = width * 4;
  uint8_t black[4] = {0,0,0,255};
  for (int row = 0; row < height/ctx_term_ch; row++)
    {
      for (int col = 0; col < width /ctx_term_cw; col++)
        {
          int     unicode = 0;
          int     bitno = 0;
          uint8_t rgba[2][4] = {
                             {255,255,255,0},
                             {0,0,0,0}};

          ctx_term_find_color_pair (term, col * ctx_term_cw,
                                    row * ctx_term_ch,
                                    ctx_term_cw,
                                    ctx_term_ch, rgba);


          /* make darkest consistently be background  */
          if (_ctx_rgba8_manhattan_diff (black, rgba[1]) >
              _ctx_rgba8_manhattan_diff (black, rgba[0]))
          {
            for (int c = 0; c < 4; c ++)
            {
              int tmp = rgba[0][c];
              rgba[0][c] = rgba[1][c];
              rgba[1][c] = tmp;
            }
          }
          if (mono)
          {
            rgba[1][0] = 0;
            rgba[1][1] = 0;
            rgba[1][2] = 0;
          }

          int pixels_set = 0;
          for (int x = 0; x < 2; x++)
            for (int y = 0; y < 3; y++)
              {
                int no = (row * 4 + y) * stride + (col*2+x) * 4;
#define CHECK_IS_SET \
      (_ctx_rgba8_manhattan_diff (&pixels[no], rgba[0])< \
       _ctx_rgba8_manhattan_diff (&pixels[no], rgba[1]))

                int set = CHECK_IS_SET;
                if (reverse) { set = !set; }
                if (set)
                  { unicode |=  (1<< (bitno) ); 
                    pixels_set ++; 
                  }
                bitno++;
              }
          {
            int x = 0;
            int y = 3;
            int no = (row * 4 + y) * stride + (col*2+x) * 4;
            int setA = CHECK_IS_SET;
            no = (row * 4 + y) * stride + (col*2+x+1) * 4;
            int setB = CHECK_IS_SET;

            pixels_set += setA;
            pixels_set += setB;
#undef CHECK_IS_SET
            if (reverse) { setA = !setA; }
            if (reverse) { setB = !setB; }
            if (setA != 0 && setB==0)
              { unicode += 0x2840; }
            else if (setA == 0 && setB)
              { unicode += 0x2880; }
            else if ( (setA != 0) && (setB != 0) )
              { unicode += 0x28C0; }
            else
              { unicode += 0x2800; }
            char utf8[5];
            utf8[ctx_unichar_to_utf8 (unicode, (uint8_t*)utf8)]=0;

#if 0
            if (pixels_set == 8)
            {
              if (rgba[0][0] < 32 && rgba[0][1] < 32 && rgba[0][2] < 32)
              {
                ctx_term_set (term, col +1, row + 1, " ",
                                 rgba[1], rgba[0]);
                continue;
              }
            }
#endif
            {
              ctx_term_set (term, col +1, row + 1, utf8,
                               rgba[0], rgba[1]);
            }
          }
        }
    }
}


inline static void ctx_term_render (void *ctx,
                                       CtxCommand *command)
{
  CtxTerm *term = (void*)ctx;
  /* directly forward */
  ctx_process (term->host, &command->entry);
}

inline static void ctx_term_flush (CtxTerm *term)
{
  int width =  term->width;
  int height = term->height;
  switch (term->mode)
  {
    case CTX_TERM_QUARTER:
       ctx_term_output_buf_quarter (term->pixels,
                                width, height, term);
       break;
    case CTX_TERM_ASCII:
       ctx_term_output_buf_ascii (term->pixels,
                                width, height, term, 0);
       break;
    case CTX_TERM_ASCII_MONO:
       ctx_term_output_buf_ascii (term->pixels,
                                width, height, term, 1);
       break;
    case CTX_TERM_SEXTANT:
       ctx_term_output_buf_sextant (term->pixels,
                                width, height, term);
       break;
    case CTX_TERM_BRAILLE:
       ctx_term_output_buf_braille (term->pixels,
                                width, height, term, 0);
       break;
    case CTX_TERM_BRAILLE_MONO:
       ctx_term_output_buf_braille (term->pixels,
                                width, height, term, 1);
       break;
  }
#if CTX_BRAILLE_TEXT
  CtxRasterizer *rasterizer = (CtxRasterizer*)(term->host->renderer);
  // XXX instead sort and inject along with braille
  //

  //uint8_t rgba_bg[4]={0,0,0,0};
  //uint8_t rgba_fg[4]={255,0,255,255};

  for (CtxList *l = rasterizer->glyphs; l; l = l->next)
  {
    CtxTermGlyph *glyph = l->data;

    uint8_t *pixels = term->pixels;
    long rgb_sum[4]={0,0,0};
    for (int v = 0; v <  ctx_term_ch; v ++)
    for (int u = 0; u <  ctx_term_cw; u ++)
    {
      int i = ((glyph->row-1) * ctx_term_ch + v) * rasterizer->blit_width + 
              ((glyph->col-1) * ctx_term_cw + u);
      for (int c = 0; c < 3; c ++)
        rgb_sum[c] += pixels[i*4+c];
    }
    for (int c = 0; c < 3; c ++)
      glyph->rgba_bg[c] = rgb_sum[c] / (ctx_term_ch * ctx_term_cw);
    char utf8[8];
    utf8[ctx_unichar_to_utf8(glyph->unichar, (uint8_t*)utf8)]=0;
    ctx_term_set (term, glyph->col, glyph->row, 
                     utf8, glyph->rgba_fg, glyph->rgba_bg);
    free (glyph);
  }

  printf ("\e[H");
  printf ("\e[0m");
  ctx_term_scanout (term);
  printf ("\e[0m");
  fflush(NULL);
  while (rasterizer->glyphs)
    ctx_list_remove (&rasterizer->glyphs, rasterizer->glyphs->data);
#endif
}

void ctx_term_free (CtxTerm *term)
{
  while (term->lines)
  {
    free (term->lines->data);
    ctx_list_remove (&term->lines, term->lines->data);
  }
  printf ("\e[?25h"); // cursor on
  nc_at_exit ();
  free (term->pixels);
  ctx_free (term->host);
  free (term);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

int ctx_renderer_is_term (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_term_free)
          return 1;
  return 0;
}

float ctx_term_get_cell_width (Ctx *ctx)
{
  return ctx_term_cw;
}

float ctx_term_get_cell_height (Ctx *ctx)
{
  return ctx_term_ch;
}

Ctx *ctx_new_term (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  CtxTerm *term = (CtxTerm*)calloc (sizeof (CtxTerm), 1);
 
  const char *mode = getenv ("CTX_TERM_MODE");
  ctx_term_cw = 2;
  ctx_term_ch = 3;

  if (!mode) term->mode = CTX_TERM_SEXTANT;
  else if (!strcmp (mode, "sextant")) term->mode = CTX_TERM_SEXTANT;
  else if (!strcmp (mode, "ascii")) term->mode = CTX_TERM_ASCII_MONO;
  //else if (!strcmp (mode, "ascii-mono")) term->mode = CTX_TERM_ASCII_MONO;
  else if (!strcmp (mode, "quarter")) term->mode = CTX_TERM_QUARTER;
  //else if (!strcmp (mode, "braille")){
  //  term->mode = CTX_TERM_BRAILLE;
  //  ctx_term_ch = 4;
  //}
  else if (!strcmp (mode, "braille")){
    term->mode = CTX_TERM_BRAILLE_MONO;
    ctx_term_ch = 4;
  }
  else {
    fprintf (stderr, "recognized values for CTX_TERM_MODE:\n"
                    " sextant ascii quarter braille\n");
    exit (1);
  }

  mode = getenv ("CTX_TERM_FORCE_FULL");
  if (mode && strcmp (mode, "0") && strcmp (mode, "no"))
    _ctx_term_force_full = 1;

  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off

  int maxwidth = ctx_terminal_cols  () * ctx_term_cw;
  int maxheight = (ctx_terminal_rows ()) * ctx_term_ch;
  if (width <= 0 || height <= 0)
  {
    width = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  term->ctx = ctx;
  term->width  = width;
  term->height = height;

  term->cols = (width + 1) / ctx_term_cw;
  term->rows = (height + 2) / ctx_term_ch;
  term->lines = 0;
  term->pixels = (uint8_t*)malloc (width * height * 4);
  term->host = ctx_new_for_framebuffer (term->pixels,
                                           width, height,
                                           width * 4, CTX_FORMAT_RGBA8);
#if CTX_BRAILLE_TEXT
  ((CtxRasterizer*)term->host->renderer)->term_glyphs=1;
#endif
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, term);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, ctx_term_ch); 
  term->render = ctx_term_render;
  term->flush = (void(*)(void*))ctx_term_flush;
  term->free  = (void(*)(void*))ctx_term_free;
#endif


  return ctx;
}

#endif

#if CTX_EVENTS

#if !__COSMOPOLITAN__
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

typedef struct _CtxTermImg CtxTermImg;
struct _CtxTermImg
{
   void (*render)         (void *termimg, CtxCommand *command);
   void (*reset)          (void *termimg);
   void (*flush)          (void *termimg);
   char *(*get_clipboard) (void *ctxctx);
   void (*set_clipboard)  (void *ctxctx, const char *text);
   void (*free)           (void *termimg);
   Ctx      *ctx;
   int       width;
   int       height;
   int       cols;
   int       rows;
   int       was_down;
   // we need to have the above members in that order up to here
   uint8_t  *pixels;
   Ctx      *host;
   CtxList  *lines;
};

inline static void ctx_termimg_render (void       *ctx,
                                       CtxCommand *command)
{
  CtxTermImg *termimg = (void*)ctx;
  /* directly forward */
  ctx_process (termimg->host, &command->entry);
}

inline static void ctx_termimg_flush (CtxTermImg *termimg)
{
  int width =  termimg->width;
  int height = termimg->height;
  if (!termimg->pixels) return;
  char *encoded = malloc (width * height * 3 * 3);
  ctx_bin2base64 (termimg->pixels, width * height * 3,
                  encoded);
  int encoded_len = strlen (encoded);

  int i = 0;

  printf ("\e[H");
  printf ("\e_Gf=24,s=%i,v=%i,t=d,a=T,m=1;\e\\", width, height);
  while (i <  encoded_len)
  {
     if (i + 4096 <  encoded_len)
     {
       printf  ("\e_Gm=1;");
     }
     else
     {
       printf  ("\e_Gm=0;");
     }
     for (int n = 0; n < 4000 && i < encoded_len; n++)
     {
       printf ("%c", encoded[i]);
       i++;
     }
     printf ("\e\\");
  }
  free (encoded);
  
  fflush (NULL);
}

void ctx_termimg_free (CtxTermImg *termimg)
{
  while (termimg->lines)
  {
    free (termimg->lines->data);
    ctx_list_remove (&termimg->lines, termimg->lines->data);
  }
  printf ("\e[?25h"); // cursor on
  nc_at_exit ();
  free (termimg->pixels);
  ctx_free (termimg->host);
  free (termimg);
  /* we're not destoring the ctx member, this is function is called in ctx' teardown */
}

int ctx_renderer_is_termimg (Ctx *ctx)
{
  if (ctx->renderer &&
      ctx->renderer->free == (void*)ctx_termimg_free)
          return 1;
  return 0;
}

Ctx *ctx_new_termimg (int width, int height)
{
  Ctx *ctx = ctx_new ();
#if CTX_RASTERIZER
  fprintf (stdout, "\e[?1049h");
  fprintf (stdout, "\e[?25l"); // cursor off
  CtxTermImg *termimg = (CtxTermImg*)calloc (sizeof (CtxTermImg), 1);


  int maxwidth = ctx_terminal_width ();

  int colwidth = maxwidth/ctx_terminal_cols ();
  maxwidth-=colwidth;

  int maxheight = ctx_terminal_height ();
  if (width <= 0 || height <= 0)
  {
    width  = maxwidth;
    height = maxheight;
  }
  if (width > maxwidth) width = maxwidth;
  if (height > maxheight) height = maxheight;
  termimg->ctx = ctx;
  termimg->width  = width;
  termimg->height = height;
  termimg->lines = 0;
  termimg->pixels = (uint8_t*)malloc (width * height * 3);
  termimg->host = ctx_new_for_framebuffer (termimg->pixels,
                                           width, height,
                                           width * 3, CTX_FORMAT_RGB8);
  _ctx_mouse (ctx, NC_MOUSE_DRAG);
  ctx_set_renderer (ctx, termimg);
  ctx_set_size (ctx, width, height);
  ctx_font_size (ctx, 14.0f);
  termimg->render = ctx_termimg_render;
  termimg->flush = (void(*)(void*))ctx_termimg_flush;
  termimg->free  = (void(*)(void*))ctx_termimg_free;
#endif

  return ctx;
}

#endif

#if CTX_FORMATTER

typedef struct _CtxFormatter  CtxFormatter;
struct _CtxFormatter 
{
  void *target; // FILE
  int   longform;
  int   indent;

  void (*add_str)(CtxFormatter *formatter, const char *str, int len);
};

static void ctx_formatter_addstr (CtxFormatter *formatter, const char *str, int len)
{
  formatter->add_str (formatter, str, len);
}

static void ctx_formatter_addstrf (CtxFormatter *formatter, const char *format, ...)
{
   va_list ap;
   size_t needed;
   char *buffer;
   va_start (ap, format);
   needed = vsnprintf (NULL, 0, format, ap) + 1;
   buffer = (char*) malloc (needed);
   va_end (ap);
   va_start (ap, format);
   vsnprintf (buffer, needed, format, ap);
   va_end (ap);
   ctx_formatter_addstr (formatter, buffer, -1);
   free (buffer);
}

static void _ctx_stream_addstr (CtxFormatter *formatter, const char *str, int len)
{
  if (!str || len == 0)
  {
    return;
  }
  if (len < 0) len = strlen (str);
  fwrite (str, len, 1, (FILE*)formatter->target);
}

void _ctx_string_addstr (CtxFormatter *formatter, const char *str, int len)
{
  if (!str || len == 0)
  {
    return;
  }
  if (len < 0) len = strlen (str);
  ctx_string_append_data ((CtxString*)(formatter->target), str, len);
}


static void _ctx_print_endcmd (CtxFormatter *formatter)
{
  if (formatter->longform)
    {
      ctx_formatter_addstr (formatter, ");\n", 3);
    }
}

static void _ctx_indent (CtxFormatter *formatter)
{
  for (int i = 0; i < formatter->indent; i++)
    { ctx_formatter_addstr (formatter, "  ", 2);
    }
}

const char *_ctx_code_to_name (int code)
{
      switch (code)
        {
          case CTX_REL_LINE_TO_X4:           return "relLinetoX4"; break;
          case CTX_REL_LINE_TO_REL_CURVE_TO: return "relLineToRelCurveTo"; break;
          case CTX_REL_CURVE_TO_REL_LINE_TO: return "relCurveToRelLineTo"; break;
          case CTX_REL_CURVE_TO_REL_MOVE_TO: return "relCurveToRelMoveTo"; break;
          case CTX_REL_LINE_TO_X2:           return "relLineToX2"; break;
          case CTX_MOVE_TO_REL_LINE_TO:      return "moveToRelLineTo"; break;
          case CTX_REL_LINE_TO_REL_MOVE_TO:  return "relLineToRelMoveTo"; break;
          case CTX_FILL_MOVE_TO:             return "fillMoveTo"; break;
          case CTX_REL_QUAD_TO_REL_QUAD_TO:  return "relQuadToRelQuadTo"; break;
          case CTX_REL_QUAD_TO_S16:          return "relQuadToS16"; break;

          case CTX_SET_KEY:              return "setParam"; break;
          case CTX_COLOR:                return "setColor"; break;
          case CTX_DEFINE_GLYPH:         return "defineGlyph"; break;
          case CTX_KERNING_PAIR:         return "kerningPair"; break;
          case CTX_SET_PIXEL:            return "setPixel"; break;
          case CTX_GLOBAL_ALPHA:         return "globalAlpha"; break;
          case CTX_TEXT:                 return "text"; break;
          case CTX_STROKE_TEXT:          return "strokeText"; break;
          case CTX_SAVE:                 return "save"; break;
          case CTX_RESTORE:              return "restore"; break;
          case CTX_STROKE_SOURCE:        return "strokeSource"; break;
          case CTX_NEW_PAGE:             return "newPage"; break;
          case CTX_START_GROUP:          return "startGroup"; break;
          case CTX_END_GROUP:            return "endGroup"; break;
          case CTX_RECTANGLE:            return "rectangle"; break;
          case CTX_ROUND_RECTANGLE:      return "roundRectangle"; break;
          case CTX_LINEAR_GRADIENT:      return "linearGradient"; break;
          case CTX_RADIAL_GRADIENT:      return "radialGradient"; break;
          case CTX_GRADIENT_STOP:        return "gradientAddStop"; break;
          case CTX_VIEW_BOX:             return "viewBox"; break;
          case CTX_MOVE_TO:              return "moveTo"; break;
          case CTX_LINE_TO:              return "lineTo"; break;
          case CTX_BEGIN_PATH:           return "beginPath"; break;
          case CTX_REL_MOVE_TO:          return "relMoveTo"; break;
          case CTX_REL_LINE_TO:          return "relLineTo"; break;
          case CTX_FILL:                 return "fill"; break;
          case CTX_EXIT:                 return "exit"; break;
          case CTX_APPLY_TRANSFORM:      return "transform"; break;
          case CTX_REL_ARC_TO:           return "relArcTo"; break;
          case CTX_GLYPH:                return "glyph"; break;
          case CTX_TEXTURE:              return "texture"; break;
          case CTX_DEFINE_TEXTURE:       return "defineTexture"; break;
          case CTX_IDENTITY:             return "identity"; break;
          case CTX_CLOSE_PATH:           return "closePath"; break;
          case CTX_PRESERVE:             return "preserve"; break;
          case CTX_FLUSH:                return "flush"; break;
          case CTX_RESET:                return "reset"; break;
          case CTX_FONT:                 return "font"; break;
          case CTX_STROKE:               return "stroke"; break;
          case CTX_CLIP:                 return "clip"; break;
          case CTX_ARC:                  return "arc"; break;
          case CTX_SCALE:                return "scale"; break;
          case CTX_TRANSLATE:            return "translate"; break;
          case CTX_ROTATE:               return "rotate"; break;
          case CTX_ARC_TO:               return "arcTo"; break;
          case CTX_CURVE_TO:             return "curveTo"; break;
          case CTX_REL_CURVE_TO:         return "relCurveTo"; break;
          case CTX_REL_QUAD_TO:          return "relQuadTo"; break;
          case CTX_QUAD_TO:              return "quadTo"; break;
          case CTX_SMOOTH_TO:            return "smoothTo"; break;
          case CTX_REL_SMOOTH_TO:        return "relSmoothTo"; break;
          case CTX_SMOOTHQ_TO:           return "smoothqTo"; break;
          case CTX_REL_SMOOTHQ_TO:       return "relSmoothqTo"; break;
          case CTX_HOR_LINE_TO:          return "horLineTo"; break;
          case CTX_VER_LINE_TO:          return "verLineTo"; break;
          case CTX_REL_HOR_LINE_TO:      return "relHorLineTo"; break;
          case CTX_REL_VER_LINE_TO:      return "relVerLineTo"; break;
          case CTX_COMPOSITING_MODE:     return "compositingMode"; break;
          case CTX_BLEND_MODE:           return "blendMode"; break;
          case CTX_TEXT_ALIGN:           return "textAlign"; break;
          case CTX_TEXT_BASELINE:        return "textBaseline"; break;
          case CTX_TEXT_DIRECTION:       return "textDirection"; break;
          case CTX_FONT_SIZE:            return "fontSize"; break;
          case CTX_MITER_LIMIT:          return "miterLimit"; break;
          case CTX_LINE_JOIN:            return "lineJoin"; break;
          case CTX_LINE_CAP:             return "lineCap"; break;
          case CTX_LINE_WIDTH:           return "lineWidth"; break;
          case CTX_LINE_DASH_OFFSET:     return "lineDashOffset"; break;
          case CTX_IMAGE_SMOOTHING:      return "imageSmoothing"; break;
          case CTX_SHADOW_BLUR:          return "shadowBlur";  break;
          case CTX_FILL_RULE:            return "fillRule"; break;
        }
      return NULL;
}

static void _ctx_print_name (CtxFormatter *formatter, int code)
{
#define CTX_VERBOSE_NAMES 1
#if CTX_VERBOSE_NAMES
  if (formatter->longform)
    {
      const char *name = NULL;
      _ctx_indent (formatter);
      //switch ((CtxCode)code)
      name = _ctx_code_to_name (code);
      if (name)
        {
          ctx_formatter_addstr (formatter, name, -1);
          ctx_formatter_addstr (formatter, " (", 2);
          if (code == CTX_SAVE)
            { formatter->indent ++; }
          else if (code == CTX_RESTORE)
            { formatter->indent --; }
          return;
        }
    }
#endif
  {
    char name[3];
    name[0]=CTX_SET_KEY;
    name[2]='\0';
    switch (code)
      {
        case CTX_GLOBAL_ALPHA:      name[1]='a'; break;
        case CTX_COMPOSITING_MODE:  name[1]='m'; break;
        case CTX_BLEND_MODE:        name[1]='B'; break;
        case CTX_TEXT_ALIGN:        name[1]='t'; break;
        case CTX_TEXT_BASELINE:     name[1]='b'; break;
        case CTX_TEXT_DIRECTION:    name[1]='d'; break;
        case CTX_FONT_SIZE:         name[1]='f'; break;
        case CTX_MITER_LIMIT:       name[1]='l'; break;
        case CTX_LINE_JOIN:         name[1]='j'; break;
        case CTX_LINE_CAP:          name[1]='c'; break;
        case CTX_LINE_WIDTH:        name[1]='w'; break;
        case CTX_LINE_DASH_OFFSET:  name[1]='D'; break;
        case CTX_IMAGE_SMOOTHING:   name[1]='S'; break;
        case CTX_SHADOW_BLUR:       name[1]='s'; break;
        case CTX_SHADOW_COLOR:      name[1]='C'; break;
        case CTX_SHADOW_OFFSET_X:   name[1]='x'; break;
        case CTX_SHADOW_OFFSET_Y:   name[1]='y'; break;
        case CTX_FILL_RULE:         name[1]='r'; break;
        default:
          name[0] = code;
          name[1] = 0;
          break;
      }
    ctx_formatter_addstr (formatter, name, -1);
    if (formatter->longform)
      ctx_formatter_addstr (formatter, " (", 2);
    else
      ctx_formatter_addstr (formatter, " ", 1);
  }
}

static void
ctx_print_entry_enum (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  for (int i = 0; i <  args; i ++)
    {
      int val = ctx_arg_u8 (i);
      if (i>0)
        { 
          ctx_formatter_addstr (formatter, " ", 1);
        }
#if CTX_VERBOSE_NAMES
      if (formatter->longform)
        {
          const char *str = NULL;
          switch (entry->code)
            {
              case CTX_TEXT_BASELINE:
                switch (val)
                  {
                    case CTX_TEXT_BASELINE_ALPHABETIC: str = "alphabetic"; break;
                    case CTX_TEXT_BASELINE_TOP:        str = "top";        break;
                    case CTX_TEXT_BASELINE_BOTTOM:     str = "bottom";     break;
                    case CTX_TEXT_BASELINE_HANGING:    str = "hanging";    break;
                    case CTX_TEXT_BASELINE_MIDDLE:     str = "middle";     break;
                    case CTX_TEXT_BASELINE_IDEOGRAPHIC:str = "ideographic";break;
                  }
                break;
              case CTX_TEXT_ALIGN:
                switch (val)
                  {
                    case CTX_TEXT_ALIGN_LEFT:   str = "left"; break;
                    case CTX_TEXT_ALIGN_RIGHT:  str = "right"; break;
                    case CTX_TEXT_ALIGN_START:  str = "start"; break;
                    case CTX_TEXT_ALIGN_END:    str = "end"; break;
                    case CTX_TEXT_ALIGN_CENTER: str = "center"; break;
                  }
                break;
              case CTX_LINE_CAP:
                switch (val)
                  {
                    case CTX_CAP_NONE:   str = "none"; break;
                    case CTX_CAP_ROUND:  str = "round"; break;
                    case CTX_CAP_SQUARE: str = "square"; break;
                  }
                break;
              case CTX_LINE_JOIN:
                switch (val)
                  {
                    case CTX_JOIN_MITER: str = "miter"; break;
                    case CTX_JOIN_ROUND: str = "round"; break;
                    case CTX_JOIN_BEVEL: str = "bevel"; break;
                  }
                break;
              case CTX_FILL_RULE:
                switch (val)
                  {
                    case CTX_FILL_RULE_WINDING:  str = "winding"; break;
                    case CTX_FILL_RULE_EVEN_ODD: str = "evenodd"; break;
                  }
                break;
              case CTX_BLEND_MODE:
                switch (val)
                  {
            case CTX_BLEND_NORMAL:      str = "normal"; break;
            case CTX_BLEND_MULTIPLY:    str = "multiply"; break;
            case CTX_BLEND_SCREEN:      str = "screen"; break;
            case CTX_BLEND_OVERLAY:     str = "overlay"; break;
            case CTX_BLEND_DARKEN:      str = "darken"; break;
            case CTX_BLEND_LIGHTEN:     str = "lighten"; break;
            case CTX_BLEND_COLOR_DODGE: str = "colorDodge"; break;
            case CTX_BLEND_COLOR_BURN:  str = "colorBurn"; break;
            case CTX_BLEND_HARD_LIGHT:  str = "hardLight"; break;
            case CTX_BLEND_SOFT_LIGHT:  str = "softLight"; break;
            case CTX_BLEND_DIFFERENCE:  str = "difference"; break;
            case CTX_BLEND_EXCLUSION:   str = "exclusion"; break;
            case CTX_BLEND_HUE:         str = "hue"; break;
            case CTX_BLEND_SATURATION:  str = "saturation"; break;
            case CTX_BLEND_COLOR:       str = "color"; break; 
            case CTX_BLEND_LUMINOSITY:  str = "luminosity"; break;
                  }
                break;
              case CTX_COMPOSITING_MODE:
                switch (val)
                  {
              case CTX_COMPOSITE_SOURCE_OVER: str = "sourceOver"; break;
              case CTX_COMPOSITE_COPY: str = "copy"; break;
              case CTX_COMPOSITE_CLEAR: str = "clear"; break;
              case CTX_COMPOSITE_SOURCE_IN: str = "sourceIn"; break;
              case CTX_COMPOSITE_SOURCE_OUT: str = "sourceOut"; break;
              case CTX_COMPOSITE_SOURCE_ATOP: str = "sourceAtop"; break;
              case CTX_COMPOSITE_DESTINATION: str = "destination"; break;
              case CTX_COMPOSITE_DESTINATION_OVER: str = "destinationOver"; break;
              case CTX_COMPOSITE_DESTINATION_IN: str = "destinationIn"; break;
              case CTX_COMPOSITE_DESTINATION_OUT: str = "destinationOut"; break;
              case CTX_COMPOSITE_DESTINATION_ATOP: str = "destinationAtop"; break;
              case CTX_COMPOSITE_XOR: str = "xor"; break;
                  }

               break;
            }
          if (str)
            {
              ctx_formatter_addstr (formatter, str, -1);
            }
          else
            {
              ctx_formatter_addstrf (formatter, "%i", val);
            }
        }
      else
#endif
        {
          ctx_formatter_addstrf (formatter, "%i", val);
        }
    }
  _ctx_print_endcmd (formatter);
}


static void
ctx_print_a85 (CtxFormatter *formatter, uint8_t *data, int length)
{
  char *tmp = (char*)malloc (ctx_a85enc_len (length));
  ctx_a85enc (data, tmp, length);
  ctx_formatter_addstr (formatter, " ~", 2);
  ctx_formatter_addstr (formatter, tmp, -1);
  ctx_formatter_addstr (formatter, "~ ", 2);
  free (tmp);
}

static void
ctx_print_escaped_string (CtxFormatter *formatter, const char *string)
{
  if (!string) { return; }
  for (int i = 0; string[i]; i++)
    {
      switch (string[i])
        {
          case '"':
            ctx_formatter_addstr (formatter, "\\\"", 2);
            break;
          case '\\':
            ctx_formatter_addstr (formatter, "\\\\", 2);
            break;
          case '\n':
            ctx_formatter_addstr (formatter, "\\n", 2);
            break;
          default:
            ctx_formatter_addstr (formatter, &string[i], 1);
        }
    }
}

static void
ctx_print_float (CtxFormatter *formatter, float val)
{
  char temp[128];
  sprintf (temp, "%0.3f", val);
  int j;
  for (j = 0; temp[j]; j++)
    if (j == ',') { temp[j] = '.'; }
  j--;
  if (j>0)
    while (temp[j] == '0')
      {
        temp[j]=0;
        j--;
      }
  if (temp[j]=='.')
    { temp[j]='\0'; }
  ctx_formatter_addstr (formatter, temp, -1);
}

static void
ctx_print_int (CtxFormatter *formatter, int val)
{
  ctx_formatter_addstrf (formatter, "%i", val);
}

static void
ctx_print_entry (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  for (int i = 0; i <  args; i ++)
    {
      float val = ctx_arg_float (i);
      if (i>0 && val >= 0.0f)
        {
          if (formatter->longform)
            {
              ctx_formatter_addstr (formatter, ", ", 2);
            }
          else
            {
              if (val >= 0.0f)
                ctx_formatter_addstr (formatter, " ", 1);
            }
        }
      ctx_print_float (formatter, val);
    }
  _ctx_print_endcmd (formatter);
}

static void
ctx_print_glyph (CtxFormatter *formatter, CtxEntry *entry, int args)
{
  _ctx_print_name (formatter, entry->code);
  ctx_formatter_addstrf (formatter, "%i", entry->data.u32[0]);
  _ctx_print_endcmd (formatter);
}

static void
ctx_formatter_process (void *user_data, CtxCommand *c);


static void
ctx_formatter_process (void *user_data, CtxCommand *c)
{
  CtxEntry *entry = &c->entry;
  CtxFormatter *formatter = (CtxFormatter*)user_data;

  switch (entry->code)
  //switch ((CtxCode)(entry->code))
    {
      case CTX_GLYPH:
        ctx_print_glyph (formatter, entry, 1);
        break;
      case CTX_LINE_TO:
      case CTX_REL_LINE_TO:
      case CTX_SCALE:
      case CTX_TRANSLATE:
      case CTX_MOVE_TO:
      case CTX_REL_MOVE_TO:
      case CTX_SMOOTHQ_TO:
      case CTX_REL_SMOOTHQ_TO:
        ctx_print_entry (formatter, entry, 2);
        break;
      case CTX_TEXTURE:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstrf (formatter, "\"");
        ctx_print_escaped_string (formatter, c->texture.eid);
        ctx_formatter_addstrf (formatter, "\", ");
        ctx_print_float (formatter, c->texture.x);
        ctx_formatter_addstrf (formatter, ", ");
        ctx_print_float (formatter, c->texture.y);
        ctx_formatter_addstrf (formatter, " ");
        _ctx_print_endcmd (formatter);
        break;

      case CTX_DEFINE_TEXTURE:
        {
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstrf (formatter, "\"");
        ctx_print_escaped_string (formatter, c->define_texture.eid);
        ctx_formatter_addstrf (formatter, "\", ");
        ctx_print_int (formatter, c->define_texture.width);
        ctx_formatter_addstrf (formatter, ", ");
        ctx_print_int (formatter, c->define_texture.height);
        ctx_formatter_addstrf (formatter, ",%i, ", c->define_texture.format);

        uint8_t *pixel_data = ctx_define_texture_pixel_data (entry);
#if 1

        int stride = ctx_pixel_format_get_stride ((CtxPixelFormat)c->define_texture.format, c->define_texture.width);
        //fprintf (stderr, "encoding %i bytes\n", c->define_texture.height *stride);
        ctx_print_a85 (formatter, pixel_data, c->define_texture.height * stride);
#else
        ctx_formatter_addstrf (formatter, "\"");
        ctx_print_escaped_string (formatter, pixel_data);
        ctx_formatter_addstrf (formatter, "\" ");

#endif

        _ctx_print_endcmd (formatter);
        }
        break;

      case CTX_REL_ARC_TO:
      case CTX_ARC_TO:
      case CTX_ROUND_RECTANGLE:
        ctx_print_entry (formatter, entry, 5);
        break;
      case CTX_CURVE_TO:
      case CTX_REL_CURVE_TO:
      case CTX_ARC:
      case CTX_RADIAL_GRADIENT:
      case CTX_APPLY_TRANSFORM:
        ctx_print_entry (formatter, entry, 6);
        break;
      case CTX_QUAD_TO:
      case CTX_RECTANGLE:
      case CTX_REL_QUAD_TO:
      case CTX_LINEAR_GRADIENT:
      case CTX_VIEW_BOX:
      case CTX_SMOOTH_TO:
      case CTX_REL_SMOOTH_TO:
        ctx_print_entry (formatter, entry, 4);
        break;
      case CTX_FONT_SIZE:
      case CTX_MITER_LIMIT:
      case CTX_ROTATE:
      case CTX_LINE_WIDTH:
      case CTX_LINE_DASH_OFFSET:
      case CTX_GLOBAL_ALPHA:
      case CTX_SHADOW_BLUR:
      case CTX_SHADOW_OFFSET_X:
      case CTX_SHADOW_OFFSET_Y:
      case CTX_VER_LINE_TO:
      case CTX_HOR_LINE_TO:
      case CTX_REL_VER_LINE_TO:
      case CTX_REL_HOR_LINE_TO:
        ctx_print_entry (formatter, entry, 1);
        break;
#if 0
      case CTX_SET:
        _ctx_print_name (formatter, entry->code);
        switch (c->set.key_hash)
        {
           case CTX_x: ctx_formatter_addstrf (formatter, " 'x' "); break;
           case CTX_y: ctx_formatter_addstrf (formatter, " 'y' "); break;
           case CTX_width: ctx_formatter_addstrf (formatter, " width "); break;
           case CTX_height: ctx_formatter_addstrf (formatter, " height "); break;
           default:
             ctx_formatter_addstrf (formatter, " %d ", c->set.key_hash);
        }
        ctx_formatter_addstrf (formatter, "\"");
        ctx_print_escaped_string (formatter, (char*)c->set.utf8);
        ctx_formatter_addstrf (formatter, "\"");
        _ctx_print_endcmd (formatter);
        break;
#endif
      case CTX_COLOR:
        if (formatter->longform ||  1)
          {
            _ctx_indent (formatter);
            int model = (int) c->set_color.model;
            const char *suffix="";
            if (model & 512)
            {
              model = model & 511;
              suffix = "S";
            }
            switch (model)
              {
                case CTX_GRAY:
                  ctx_formatter_addstrf (formatter, "gray%s ", suffix);
                  ctx_print_float (formatter, c->graya.g);
                  break;
                case CTX_GRAYA:
                  ctx_formatter_addstrf (formatter, "graya%s ", suffix);
                  ctx_print_float (formatter, c->graya.g);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->graya.a);
                  break;
                case CTX_RGBA:
                  if (c->rgba.a != 1.0)
                  {
                    ctx_formatter_addstrf (formatter, "rgba%s ", suffix);
                    ctx_print_float (formatter, c->rgba.r);
                    ctx_formatter_addstrf (formatter, " ");
                    ctx_print_float (formatter, c->rgba.g);
                    ctx_formatter_addstrf (formatter, " ");
                    ctx_print_float (formatter, c->rgba.b);
                    ctx_formatter_addstrf (formatter, " ");
                    ctx_print_float (formatter, c->rgba.a);
                    break;
                  }
                  /* FALLTHROUGH */
                case CTX_RGB:
                  if (c->rgba.r == c->rgba.g && c->rgba.g == c->rgba.b)
                  {
                    ctx_formatter_addstrf (formatter, "gray%s ", suffix);
                    ctx_print_float (formatter, c->rgba.r);
                    ctx_formatter_addstrf (formatter, " ");
                    break;
                  }
                  ctx_formatter_addstrf (formatter, "rgb%s ", suffix);
                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.b);
                  break;
                case CTX_DRGB:
                  ctx_formatter_addstrf (formatter, "drgb%s ", suffix);
                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.b);
                  break;
                case CTX_DRGBA:
                  ctx_formatter_addstrf (formatter, "drgba%s ", suffix);
                  ctx_print_float (formatter, c->rgba.r);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.g);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.b);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->rgba.a);
                  break;
                case CTX_CMYK:
                  ctx_formatter_addstrf (formatter, "cmyk%s ", suffix);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.k);
                  break;
                case CTX_CMYKA:
                  ctx_formatter_addstrf (formatter, "cmyka%s ", suffix);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.k);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.a);
                  break;
                case CTX_DCMYK:
                  ctx_formatter_addstrf (formatter, "dcmyk%s ", suffix);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.k);
                  break;
                case CTX_DCMYKA:
                  ctx_formatter_addstrf (formatter, "dcmyka%s ", suffix);
                  ctx_print_float (formatter, c->cmyka.c);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.m);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.y);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.k);
                  ctx_formatter_addstrf (formatter, " ");
                  ctx_print_float (formatter, c->cmyka.a);
                  break;
              }
          }
        else
          {
            ctx_print_entry (formatter, entry, 1);
          }
        break;
      case CTX_SET_RGBA_U8:
        if (formatter->longform)
          {
            _ctx_indent (formatter);
            ctx_formatter_addstrf (formatter, "rgba (");
          }
        else
          {
            ctx_formatter_addstrf (formatter, "rgba (");
          }
        for (int c = 0; c < 4; c++)
          {
            if (c)
              {
                if (formatter->longform)
                  ctx_formatter_addstrf (formatter, ", ");
                else
                  ctx_formatter_addstrf (formatter, " ");
              }
            ctx_print_float (formatter, ctx_u8_to_float (ctx_arg_u8 (c) ) );
          }
        _ctx_print_endcmd (formatter);
        break;
      case CTX_SET_PIXEL:
#if 0
        ctx_set_pixel_u8 (d_ctx,
                          ctx_arg_u16 (2), ctx_arg_u16 (3),
                          ctx_arg_u8 (0),
                          ctx_arg_u8 (1),
                          ctx_arg_u8 (2),
                          ctx_arg_u8 (3) );
#endif
        break;
      case CTX_FILL:
      case CTX_RESET:
      case CTX_STROKE:
      case CTX_IDENTITY:
      case CTX_CLIP:
      case CTX_BEGIN_PATH:
      case CTX_CLOSE_PATH:
      case CTX_SAVE:
      case CTX_PRESERVE:
      case CTX_START_GROUP:
      case CTX_NEW_PAGE:
      case CTX_END_GROUP:
      case CTX_RESTORE:
      case CTX_STROKE_SOURCE:
        ctx_print_entry (formatter, entry, 0);
        break;
      case CTX_TEXT_ALIGN:
      case CTX_TEXT_BASELINE:
      case CTX_TEXT_DIRECTION:
      case CTX_FILL_RULE:
      case CTX_LINE_CAP:
      case CTX_LINE_JOIN:
      case CTX_COMPOSITING_MODE:
      case CTX_BLEND_MODE:
      case CTX_IMAGE_SMOOTHING:
        ctx_print_entry_enum (formatter, entry, 1);
        break;
      case CTX_GRADIENT_STOP:
        _ctx_print_name (formatter, entry->code);
        for (int c = 0; c < 4; c++)
          {
            if (c)
              ctx_formatter_addstrf (formatter, " ");
            ctx_print_float (formatter, ctx_u8_to_float (ctx_arg_u8 (4+c) ) );
          }
        _ctx_print_endcmd (formatter);
        break;
      case CTX_TEXT:
      case CTX_STROKE_TEXT:
      case CTX_FONT:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstrf (formatter, "\"");
        ctx_print_escaped_string (formatter, ctx_arg_string() );
        ctx_formatter_addstrf (formatter, "\"");
        _ctx_print_endcmd (formatter);
        break;
      case CTX_CONT:
      case CTX_EDGE:
      case CTX_DATA:
      case CTX_DATA_REV:
      case CTX_FLUSH:
        break;
      case CTX_KERNING_PAIR:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstrf (formatter, "\"");
        {
           uint8_t utf8[16];
           utf8[ctx_unichar_to_utf8 (c->kern.glyph_before, utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstrf (formatter, "\", \"");
           utf8[ctx_unichar_to_utf8 (c->kern.glyph_after, utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstrf (formatter, "\"");
           sprintf ((char*)utf8, ", %f", c->kern.amount / 256.0);
           ctx_print_escaped_string (formatter, (char*)utf8);
        }
        _ctx_print_endcmd (formatter);
        break;

      case CTX_DEFINE_GLYPH:
        _ctx_print_name (formatter, entry->code);
        ctx_formatter_addstrf (formatter, "\"");
        {
           uint8_t utf8[16];
           utf8[ctx_unichar_to_utf8 (entry->data.u32[0], utf8)]=0;
           ctx_print_escaped_string (formatter, (char*)utf8);
           ctx_formatter_addstrf (formatter, "\"");
           sprintf ((char*)utf8, ", %f", entry->data.u32[1]/256.0);
           ctx_print_escaped_string (formatter, (char*)utf8);
        }
        _ctx_print_endcmd (formatter);
        break;
    }
}

void
ctx_render_stream (Ctx *ctx, FILE *stream, int longform)
{
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target= stream;
  formatter.longform = longform;
  formatter.indent = 0;
  formatter.add_str = _ctx_stream_addstr;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_formatter_process (&formatter, command); }
  fprintf (stream, "\n");
}

char *
ctx_render_string (Ctx *ctx, int longform, int *retlen)
{
  CtxString *string = ctx_string_new ("");
  CtxIterator iterator;
  CtxFormatter formatter;
  formatter.target= string;
  formatter.longform = longform;
  formatter.indent = 0;
  formatter.add_str = _ctx_string_addstr;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    { ctx_formatter_process (&formatter, command); }
  char *ret = string->str;
  if (retlen)
    *retlen = string->length;
  ctx_string_free (string, 0);
  return ret;
}


#endif

#if CTX_EVENTS
int ctx_width (Ctx *ctx)
{
  return ctx->events.width;
}
int ctx_height (Ctx *ctx)
{
  return ctx->events.height;
}
#else
int ctx_width (Ctx *ctx)
{
  return 512;
}
int ctx_height (Ctx *ctx)
{
  return 384;
}
#endif

int ctx_rev (Ctx *ctx)
{
  return ctx->rev;
}

CtxState *ctx_get_state (Ctx *ctx)
{
  return &ctx->state;
}

void ctx_dirty_rect (Ctx *ctx, int *x, int *y, int *width, int *height)
{
  if ( (ctx->state.min_x > ctx->state.max_x) ||
       (ctx->state.min_y > ctx->state.max_y) )
    {
      if (x) { *x = 0; }
      if (y) { *y = 0; }
      if (width) { *width = 0; }
      if (height) { *height = 0; }
      return;
    }
  if (ctx->state.min_x < 0)
    { ctx->state.min_x = 0; }
  if (ctx->state.min_y < 0)
    { ctx->state.min_y = 0; }
  if (x) { *x = ctx->state.min_x; }
  if (y) { *y = ctx->state.min_y; }
  if (width) { *width = ctx->state.max_x - ctx->state.min_x; }
  if (height) { *height = ctx->state.max_y - ctx->state.min_y; }
}

#if CTX_CURRENT_PATH
CtxIterator *
ctx_current_path (Ctx *ctx)
{
  CtxIterator *iterator = &ctx->current_path_iterator;
  ctx_iterator_init (iterator, &ctx->current_path, 0, CTX_ITERATOR_EXPAND_BITPACK);
  return iterator;
}

void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2)
{
  float minx = 50000.0;
  float miny = 50000.0;
  float maxx = -50000.0;
  float maxy = -50000.0;
  float x = 0;
  float y = 0;

  CtxIterator *iterator = ctx_current_path (ctx);
  CtxCommand *command;

  while ((command = ctx_iterator_next (iterator)))
  {
     int got_coord = 0;
     switch (command->code)
     {
        // XXX missing many curve types
        case CTX_LINE_TO:
        case CTX_MOVE_TO:
          x = command->move_to.x;
          y = command->move_to.y;
          got_coord++;
          break;
        case CTX_REL_LINE_TO:
        case CTX_REL_MOVE_TO:
          x += command->move_to.x;
          y += command->move_to.y;
          got_coord++;
          break;
        case CTX_CURVE_TO:
          x = command->curve_to.x;
          y = command->curve_to.y;
          got_coord++;
          break;
        case CTX_REL_CURVE_TO:
          x += command->curve_to.x;
          y += command->curve_to.y;
          got_coord++;
          break;
        case CTX_ARC:
          minx = ctx_minf (minx, command->arc.x - command->arc.radius);
          miny = ctx_minf (miny, command->arc.y - command->arc.radius);
          maxx = ctx_maxf (maxx, command->arc.x + command->arc.radius);
          maxy = ctx_maxf (maxy, command->arc.y + command->arc.radius);

          break;
        case CTX_RECTANGLE:
        case CTX_ROUND_RECTANGLE:
          x = command->rectangle.x;
          y = command->rectangle.y;
          minx = ctx_minf (minx, x);
          miny = ctx_minf (miny, y);
          maxx = ctx_maxf (maxx, x);
          maxy = ctx_maxf (maxy, y);

          x += command->rectangle.width;
          y += command->rectangle.height;
          got_coord++;
          break;
     }
    if (got_coord)
    {
      minx = ctx_minf (minx, x);
      miny = ctx_minf (miny, y);
      maxx = ctx_maxf (maxx, x);
      maxy = ctx_maxf (maxy, y);
    }
  }

  if (ex1) *ex1 = minx;
  if (ey1) *ey1 = miny;
  if (ex2) *ex2 = maxx;
  if (ey2) *ey2 = maxy;
}

#else
void
ctx_path_extents (Ctx *ctx, float *ex1, float *ey1, float *ex2, float *ey2)
{
}
#endif


static void
ctx_gstate_push (CtxState *state)
{
  if (state->gstate_no + 1 >= CTX_MAX_STATES)
    { return; }
  state->gstate_stack[state->gstate_no] = state->gstate;
  state->gstate_no++;
  ctx_state_set (state, CTX_new_state, 0.0);
  state->has_clipped=0;
}

static void
ctx_gstate_pop (CtxState *state)
{
  if (state->gstate_no <= 0)
    { return; }
  state->gstate = state->gstate_stack[state->gstate_no-1];
  state->gstate_no--;
}

void
ctx_close_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLOSE_PATH);
}

int _ctx_is_rasterizer (Ctx *ctx);

void
ctx_get_image_data (Ctx *ctx, int sx, int sy, int sw, int sh,
                    CtxPixelFormat format, int dst_stride,
                    uint8_t *dst_data)
{
   if (0)
   {
   }
#if CTX_RASTERIZER
   else if (_ctx_is_rasterizer (ctx))
   {
     CtxRasterizer *rasterizer = (CtxRasterizer*)ctx->renderer;
     if (rasterizer->format->pixel_format == format)
     {
       if (dst_stride <= 0) dst_stride = ctx_pixel_format_get_stride (format, sw);
       int bytes_per_pix = rasterizer->format->bpp/8;
       int y = 0;
       for (int v = sy; v < sy + sh; v++, y++)
       {
         int x = 0;
         for (int u = sx; u < sx + sw; u++, x++)
         {
            uint8_t* src_buf = (uint8_t*)rasterizer->buf;
            memcpy (&dst_data[y * dst_stride + x * bytes_per_pix], &src_buf[v * rasterizer->blit_stride + u * bytes_per_pix], bytes_per_pix);
         }
       }
       return;
     }
   }
#endif
#if CTX_FB
   else if (format == CTX_FORMAT_RGBA8 &&
                   (
                   ctx_renderer_is_fb (ctx)
#if CTX_SDL
                   || ctx_renderer_is_sdl (ctx)
#endif
                   ))
   {
     CtxTiled *tiled = (CtxTiled*)ctx->renderer;
     {
       if (dst_stride <= 0) dst_stride = ctx_pixel_format_get_stride (format, sw);
       int bytes_per_pix = 4;
       int y = 0;
       for (int v = sy; v < sy + sh; v++, y++)
       {
         int x = 0;
         for (int u = sx; u < sx + sw; u++, x++)
         {
            uint8_t* src_buf = (uint8_t*)tiled->pixels;
            memcpy (&dst_data[y * dst_stride + x * bytes_per_pix], &src_buf[v * tiled->width * bytes_per_pix + u * bytes_per_pix], bytes_per_pix);
         }
       }
       return;
     }
   }
#endif
}

void
ctx_put_image_data (Ctx *ctx, int w, int h, int stride, int format,
                    uint8_t *data,
                    int ox, int oy,
                    int dirtyX, int dirtyY,
                    int dirtyWidth, int dirtyHeight)
{
   char eid[65]="";
   ctx_save (ctx);
   ctx_identity (ctx);
   ctx_define_texture (ctx, NULL, w, h, stride, format, data, eid);
   if (eid[0])
   {
     // XXX set compositor to source
     ctx_compositing_mode (ctx, CTX_COMPOSITE_COPY);
     ctx_draw_texture_clipped (ctx, eid, ox, oy, w, h, dirtyX, dirtyY, dirtyWidth, dirtyHeight);
   }
   ctx_restore (ctx);
}

/* checking if an eid is valid also sets the frame for it
 */
static int ctx_eid_valid (Ctx *ctx, const char *eid, int *w, int *h)
{
  ctx = ctx->texture_cache;
  CtxList *to_remove = NULL;
  int ret = 0;
  //fprintf (stderr, "{%i}\n", ctx->frame);
  for (CtxList *l = ctx->eid_db; l; l = l->next)
  {
    CtxEidInfo *eid_info = (CtxEidInfo*)l->data;
    if (ctx->frame - eid_info->frame >= 2)
            /* XXX XXX XXX this breaks texture caching since
             *   it is wrong in some cases where more frames
             *   have passed?
             */
    {
      ctx_list_prepend (&to_remove, eid_info);
    }
    else if (!strcmp (eid_info->eid, eid) &&
             ctx->frame - eid_info->frame < 2)
    {
    //fclose (f);
      eid_info->frame = ctx->frame;
      if (w) *w = eid_info->width;
      if (h) *h = eid_info->height;
      ret = 1;
    }
  }
  while (to_remove)
  {
    CtxEidInfo *eid_info = (CtxEidInfo*)to_remove->data;
    //FILE  *f  = fopen ("/tmp/l", "a");
    //fprintf (f, "%i client removing %s\n", getpid(), eid_info->eid);
    //fclose (f);
    free (eid_info->eid);
    free (eid_info);
    ctx_list_remove (&ctx->eid_db, eid_info);
    ctx_list_remove (&to_remove, eid_info);
  }
  return ret;
}

void ctx_texture (Ctx *ctx, const char *eid, float x, float y)
{
  int eid_len = strlen (eid);
  char ascii[41]="";
  //fprintf (stderr, "tx %s\n", eid);
  if (eid_len > 50)
  {
    CtxSHA1 *sha1 = ctx_sha1_new ();
    uint8_t hash[20]="";
    ctx_sha1_process (sha1, (uint8_t*)eid, eid_len);
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    eid=ascii;
  }

    //FILE  *f = fopen ("/tmp/l", "a");
  if (ctx_eid_valid (ctx, eid, 0, 0))
  {
    ctx_process_cmd_str_float (ctx, CTX_TEXTURE, eid, x, y);
    //fprintf (stderr, "setting texture eid %s\n", eid);
  }
  else
  {
    //fprintf (stderr, "tried setting invalid texture eid %s\n", eid);
  }
    //fclose (f);
}
int
_ctx_frame (Ctx *ctx)
{
   return ctx->frame;
}
int
_ctx_set_frame (Ctx *ctx, int frame)
{
   return ctx->frame = frame;
}

void ctx_define_texture (Ctx *ctx, const char *eid, int width, int height, int stride, int format, void *data, char *ret_eid)
{
  uint8_t hash[20]="";
  char ascii[41]="";
  int dst_stride = width;
  //fprintf (stderr, "df %s\n", eid);

  dst_stride = ctx_pixel_format_get_stride ((CtxPixelFormat)format, width);
  if (stride <= 0)
    stride = dst_stride;

  int data_len = height * dst_stride;

  if (eid == NULL)
  {
    CtxSHA1 *sha1 = ctx_sha1_new ();

    {
      uint8_t *src = (uint8_t*)data;
      for (int y = 0; y < height; y++)
      {
         ctx_sha1_process (sha1, src, dst_stride);
         src += stride;
      }
    }
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    eid = ascii;
  }

  int eid_len = strlen (eid);

  if (eid_len > 50)
  {
    CtxSHA1 *sha1 = ctx_sha1_new ();
    uint8_t hash[20]="";
    ctx_sha1_process (sha1, (uint8_t*)eid, eid_len);
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    eid = ascii;
    eid_len = 40;
  }

  // we now have eid

  if (ctx_eid_valid (ctx, eid, 0, 0))
  {
    ctx_texture (ctx, eid, 0.0, 0.0);
  }
  else

  {
    CtxEntry *commands;
    int command_size = 1 + (data_len+1+1)/9 + 1 + (eid_len+1+1)/9 + 1 +   8;
    if (ctx->renderer && ctx->renderer->process)
    {
       commands = (CtxEntry*)calloc (sizeof (CtxEntry), command_size);
    }
    else
    {
       commands = NULL;
       ctx_drawlist_resize (&ctx->drawlist, ctx->drawlist.count + command_size);
       commands = &(ctx->drawlist.entries[ctx->drawlist.count]);
       memset (commands, 0, sizeof (CtxEntry) * command_size);
    }
    /* bottleneck,  we can avoid copying sometimes - and even when copying
     * we should cut this down to one copy, direct to the drawlist.
     *
     */
    commands[0] = ctx_u32 (CTX_DEFINE_TEXTURE, width, height);
    commands[1].data.u16[0] = format;

    int pos = 2;

    commands[pos].code        = CTX_DATA;
    commands[pos].data.u32[0] = eid_len;
    commands[pos].data.u32[1] = (eid_len+1+1)/9 + 1;
    memcpy ((char *) &commands[pos+1].data.u8[0], eid, eid_len);
    ((char *) &commands[pos+1].data.u8[0])[eid_len]=0;

    pos = 2 + 1 + ctx_conts_for_entry (&commands[2]);
    commands[pos].code        = CTX_DATA;
    commands[pos].data.u32[0] = data_len;
    commands[pos].data.u32[1] = (data_len+1+1)/9 + 1;
    {
      uint8_t *src = (uint8_t*)data;
      uint8_t *dst = &commands[pos+1].data.u8[0];
      for (int y = 0; y < height; y++)
      {
         memcpy (dst, src, dst_stride);
         src += stride;
         dst += dst_stride;
      }
    }
    ((char *) &commands[pos+1].data.u8[0])[data_len]=0;

    if (ctx->renderer && ctx->renderer->process)
    {
      ctx_process (ctx, commands);
      free (commands);
    }
    else
    {
       ctx->drawlist.count += ctx_conts_for_entry (commands) + 1;
    }

    CtxEidInfo *eid_info = (CtxEidInfo*)calloc (sizeof (CtxEidInfo), 1);
    eid_info->eid        = strdup (eid);
    eid_info->width      = width;
    eid_info->height     = height;
    eid_info->frame      = ctx->texture_cache->frame;
    //fprintf (stderr, "%i\n", eid_info->frame);
    ctx_list_prepend (&ctx->texture_cache->eid_db, eid_info);
  }

  if (ret_eid)
  {
    strcpy (ret_eid, eid);
    ret_eid[64]=0;
  }
}

void
ctx_texture_load (Ctx *ctx, const char *path, int *tw, int *th, char *reid)
{
  const char *eid = path;
  char ascii[41]="";
  int eid_len = strlen (eid);
  if (eid_len > 50)
  {
    CtxSHA1 *sha1 = ctx_sha1_new ();
    uint8_t hash[20]="";
    ctx_sha1_process (sha1, (uint8_t*)eid, eid_len);
    ctx_sha1_done (sha1, hash);
    ctx_sha1_free (sha1);
    const char *hex="0123456789abcdef";
    for (int i = 0; i < 20; i ++)
    {
       ascii[i*2]=hex[hash[i]/16];
       ascii[i*2+1]=hex[hash[i]%16];
    }
    ascii[40]=0;
    eid = ascii;
  }

  if (ctx_eid_valid (ctx, eid , tw, th))
  {
     if (reid)
     {
       strcpy (reid, eid);
     }
     return;
  }

#ifdef STBI_INCLUDE_STB_IMAGE_H
    CtxPixelFormat pixel_format = CTX_FORMAT_RGBA8;
  int w, h, components;
  unsigned char *pixels = NULL;

  if (!strncmp (path, "file://", 7))
  {
    pixels = stbi_load (path + 7, &w, &h, &components, 0);
  }
  else
  {
    unsigned char *data = NULL;
    long length = 0;
    ctx_get_contents (path, &data, &length);
    if (data)
    {
       pixels = stbi_load_from_memory (data, length, &w, &h, &components, 0);
       free (data);
    }
  }

  if (pixels)
  {
    switch (components)
    {
      case 1: pixel_format = CTX_FORMAT_GRAY8;  break;
      case 2: pixel_format = CTX_FORMAT_GRAYA8; break;
      case 3: pixel_format = CTX_FORMAT_RGB8;   break;
      case 4: pixel_format = CTX_FORMAT_RGBA8;  break;
    }
    if (tw) *tw = w;
    if (th) *th = h;
    ctx_define_texture (ctx, eid, w, h, w * components, pixel_format, pixels, 
                             reid);
    free (pixels);
  }
  else
  {
    fprintf (stderr, "texture loading problem for %s\n", path);
  }
#endif
}

void
ctx_draw_texture_clipped  (Ctx *ctx, const char *eid,
                           float x, float y,
                           float width, float height,
                           float clip_x, float clip_y,
                           float clip_width, float clip_height)
{
  int tex_width  = 0;
  int tex_height = 0;
  if (ctx_eid_valid (ctx, eid , &tex_width, &tex_height))
  {
    if (width > 0.0 && height > 0.0)
    {
      ctx_save (ctx);
#if 0
      if (clip_width > 0.0f)
      {
        ctx_rectangle (ctx, clip_x, clip_y, clip_width, clip_height);
        ctx_clip (ctx);
      }
#endif
      ctx_rectangle (ctx, x, y, width, height);
      if (clip_width > 0.0f)
      {
        ctx_translate (ctx, -clip_x, -clip_y);
        ctx_scale (ctx, width/clip_width, height/clip_height);
      }
      else
      {
        ctx_scale (ctx, width/tex_width, height/tex_height);
      }
      //ctx_texture (ctx, eid, x / (width/tex_width), y / (height/tex_height));
      ctx_texture (ctx, eid, x, y);// / (width/tex_width), y / (height/tex_height));
      ctx_fill (ctx);
      ctx_restore (ctx);
    }
  }
}

void ctx_draw_texture (Ctx *ctx, const char *eid, float x, float y, float w, float h)
{
  ctx_draw_texture_clipped (ctx, eid, x, y, w, h, 0,0,0,0);
}

void ctx_draw_image_clipped (Ctx *ctx, const char *path, float x, float y, float w, float h, float sx, float sy, float swidth, float sheight)
{
  char reteid[65];
  int width, height;
  ctx_texture_load (ctx, path, &width, &height, reteid);
  if (reteid[0])
  {
    ctx_draw_texture_clipped (ctx, reteid, x, y, w, h, sx, sy, swidth, sheight);
  }
}

void
ctx_draw_image (Ctx *ctx, const char *path, float x, float y, float w, float h)
{
  ctx_draw_image_clipped (ctx, path, x, y, w, h, 0,0,0,0);
}

void
ctx_set_pixel_u8 (Ctx *ctx, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
  CtxEntry command = ctx_u8 (CTX_SET_PIXEL, r, g, b, a, 0, 0, 0, 0);
  command.data.u16[2]=x;
  command.data.u16[3]=y;
  ctx_process (ctx, &command);
}

void
ctx_linear_gradient (Ctx *ctx, float x0, float y0, float x1, float y1)
{
  CtxEntry command[2]=
  {
    ctx_f (CTX_LINEAR_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            x1, y1)
  };
  ctx_process (ctx, command);
}

void
ctx_radial_gradient (Ctx *ctx, float x0, float y0, float r0, float x1, float y1, float r1)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RADIAL_GRADIENT, x0, y0),
    ctx_f (CTX_CONT,            r0, x1),
    ctx_f (CTX_CONT,            y1, r1)
  };
  ctx_process (ctx, command);
}


void ctx_preserve (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_PRESERVE);
}

void ctx_fill (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_FILL);
}

void ctx_stroke (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_STROKE);
}


static void ctx_empty (Ctx *ctx)
{
#if CTX_RASTERIZER
  if (ctx->renderer == NULL)
#endif
    {
      ctx->drawlist.count = 0;
      ctx->drawlist.bitpack_pos = 0;
    }
}

void _ctx_set_store_clear (Ctx *ctx)
{
  ctx->transformation |= CTX_TRANSFORMATION_STORE_CLEAR;
}

#if CTX_EVENTS
static void
ctx_event_free (void *event, void *user_data)
{
  free (event);
}

static void
ctx_collect_events (CtxEvent *event, void *data, void *data2)
{
  Ctx *ctx = (Ctx*)data;
  CtxEvent *copy;
  if (event->type == CTX_KEY_PRESS && !strcmp (event->string, "idle"))
    return;
  copy = (CtxEvent*)malloc (sizeof (CtxEvent));
  *copy = *event;
  if (copy->string)
    copy->string = strdup (event->string);
  ctx_list_append_full (&ctx->events.events, copy, ctx_event_free, NULL);
}
#endif

#if CTX_EVENTS
static void _ctx_bindings_key_press (CtxEvent *event, void *data1, void *data2);
#endif

void ctx_reset (Ctx *ctx)
{
        /* we do the callback reset first - maybe we need two cbs,
         * one for before and one after default impl?
         *
         * tiled fb and sdl needs to sync
         */
  if (ctx->renderer && ctx->renderer->reset)
    ctx->renderer->reset (ctx->renderer);

  //CTX_PROCESS_VOID (CTX_RESET);
  //if (ctx->transformation & CTX_TRANSFORMATION_STORE_CLEAR)
  //  { return; }
  ctx_empty (ctx);
  ctx_state_init (&ctx->state);
#if CTX_EVENTS
  ctx_list_free (&ctx->events.items);
  ctx->events.last_item = NULL;

  if (ctx->events.ctx_get_event_enabled)
  {
    ctx_clear_bindings (ctx);
    ctx_listen_full (ctx, 0,0,0,0,
                     CTX_KEY_PRESS, _ctx_bindings_key_press, ctx, ctx,
                     NULL, NULL);

    ctx_listen_full (ctx, 0,0,0,0,
                     CTX_KEY_UP, ctx_collect_events, ctx, ctx,
                     NULL, NULL);
    ctx_listen_full (ctx, 0,0,0,0,
                     CTX_KEY_DOWN, ctx_collect_events, ctx, ctx,
                     NULL, NULL);

    ctx_listen_full (ctx, 0, 0, ctx->events.width, ctx->events.height,
                     (CtxEventType)(CTX_PRESS|CTX_RELEASE|CTX_MOTION),
                     ctx_collect_events, ctx, ctx,
                     NULL, NULL);
  }
#endif
}

void ctx_begin_path (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_BEGIN_PATH);
}

void ctx_clip (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_CLIP);
}

void
ctx_set (Ctx *ctx, uint64_t key_hash, const char *string, int len);

void ctx_save (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_SAVE);
}
void ctx_restore (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_RESTORE);
}

void ctx_start_group (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_START_GROUP);
}

void ctx_end_group (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_END_GROUP);
}

void ctx_line_width (Ctx *ctx, float x)
{
  if (ctx->state.gstate.line_width != x)
    CTX_PROCESS_F1 (CTX_LINE_WIDTH, x);
}

float ctx_get_miter_limit (Ctx *ctx)
{
  return ctx->state.gstate.miter_limit;
}

float ctx_get_line_dash_offset (Ctx *ctx)
{
  return ctx->state.gstate.line_dash_offset;
}

void ctx_line_dash_offset (Ctx *ctx, float x)
{
  if (ctx->state.gstate.line_dash_offset != x)
    CTX_PROCESS_F1 (CTX_LINE_DASH_OFFSET, x);
}

int ctx_get_image_smoothing (Ctx *ctx)
{
  return ctx->state.gstate.image_smoothing;
}

void ctx_image_smoothing (Ctx *ctx, int enabled)
{
  if (ctx_get_image_smoothing (ctx) != enabled)
    CTX_PROCESS_U8 (CTX_IMAGE_SMOOTHING, enabled);
}


void ctx_line_dash (Ctx *ctx, float *dashes, int count)
{
  ctx_process_cmd_str_with_len (ctx, CTX_LINE_DASH, (char*)(dashes), count, 0, count * 4);
}

void ctx_shadow_blur (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_blur != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_BLUR, x);
}

void ctx_shadow_rgba (Ctx *ctx, float r, float g, float b, float a)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_SHADOW_COLOR, CTX_RGBA, r),
    ctx_f (CTX_CONT, g, b),
    ctx_f (CTX_CONT, a, 0)
  };
  ctx_process (ctx, command);
}

void ctx_shadow_offset_x (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_offset_x != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_OFFSET_X, x);
}

void ctx_shadow_offset_y (Ctx *ctx, float x)
{
#if CTX_ENABLE_SHADOW_BLUR
  if (ctx->state.gstate.shadow_offset_y != x)
#endif
    CTX_PROCESS_F1 (CTX_SHADOW_OFFSET_Y, x);
}

void
ctx_global_alpha (Ctx *ctx, float global_alpha)
{
  if (ctx->state.gstate.global_alpha_f != global_alpha)
    CTX_PROCESS_F1 (CTX_GLOBAL_ALPHA, global_alpha);
}

float
ctx_get_global_alpha (Ctx *ctx)
{
  return ctx->state.gstate.global_alpha_f;
}

void
ctx_font_size (Ctx *ctx, float x)
{
  CTX_PROCESS_F1 (CTX_FONT_SIZE, x);
}

float ctx_get_font_size  (Ctx *ctx)
{
  return ctx->state.gstate.font_size;
}

void
ctx_miter_limit (Ctx *ctx, float limit)
{
  CTX_PROCESS_F1 (CTX_MITER_LIMIT, limit);
}

float ctx_get_line_width (Ctx *ctx)
{
  return ctx->state.gstate.line_width;
}

void
_ctx_font (Ctx *ctx, const char *name)
{
  ctx->state.gstate.font = ctx_resolve_font (name);
}

#if 0
void
ctx_set (Ctx *ctx, uint64_t key_hash, const char *string, int len)
{
  if (len <= 0) len = strlen (string);
  ctx_process_cmd_str (ctx, CTX_SET, string, key_hash, len);
}

const char *
ctx_get (Ctx *ctx, const char *key)
{
  static char retbuf[32];
  int len = 0;
  CTX_PROCESS_U32(CTX_GET, ctx_strhash (key, 0), 0);
  while (read (STDIN_FILENO, &retbuf[len], 1) != -1)
    {
      if(retbuf[len]=='\n')
        break;
      retbuf[++len]=0;
    }
  return retbuf;
}
#endif

void
ctx_font_family (Ctx *ctx, const char *name)
{
#if CTX_BACKEND_TEXT
  ctx_process_cmd_str (ctx, CTX_FONT, name, 0, 0);
  _ctx_font (ctx, name);
#else
  _ctx_font (ctx, name);
#endif
}

void
ctx_font (Ctx *ctx, const char *family_name)
{
  // should also parse size
  ctx_font_family (ctx, family_name);
}

const char *
ctx_get_font (Ctx *ctx)
{
  return ctx_fonts[ctx->state.gstate.font].name;
}

void ctx_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { CTX_PROCESS_F (CTX_MOVE_TO, x, y); }
  else
    { CTX_PROCESS_F (CTX_LINE_TO, x, y); }
}

void ctx_move_to (Ctx *ctx, float x, float y)
{
  CTX_PROCESS_F (CTX_MOVE_TO,x,y);
}

void ctx_curve_to (Ctx *ctx, float x0, float y0,
                   float x1, float y1,
                   float x2, float y2)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT,     x1, y1),
    ctx_f (CTX_CONT,     x2, y2)
  };
  ctx_process (ctx, command);
}

void ctx_round_rectangle (Ctx *ctx,
                          float x0, float y0,
                          float w, float h,
                          float radius)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_ROUND_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,            w, h),
    ctx_f (CTX_CONT,            radius, 0)
  };
  ctx_process (ctx, command);
}

void ctx_view_box (Ctx *ctx,
                   float x0, float y0,
                   float w, float h)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_VIEW_BOX, x0, y0),
    ctx_f (CTX_CONT,     w, h)
  };
  ctx_process (ctx, command);
}

void ctx_rectangle (Ctx *ctx,
                    float x0, float y0,
                    float w, float h)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_RECTANGLE, x0, y0),
    ctx_f (CTX_CONT,      w, h)
  };
  ctx_process (ctx, command);
}

void ctx_rel_line_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CTX_PROCESS_F (CTX_REL_LINE_TO,x,y);
}

void ctx_rel_move_to (Ctx *ctx, float x, float y)
{
  if (!ctx->state.has_moved)
    {
      CTX_PROCESS_F (CTX_MOVE_TO,x,y);
      return;
    }
  CTX_PROCESS_F (CTX_REL_MOVE_TO,x,y);
}

CtxLineJoin ctx_get_line_join (Ctx *ctx)
{
  return ctx->state.gstate.line_join;
}

CtxCompositingMode ctx_get_compositing_mode (Ctx *ctx)
{
  return ctx->state.gstate.compositing_mode;
}

CtxBlend ctx_get_blend_mode (Ctx *ctx)
{
  return ctx->state.gstate.blend_mode;
}

CtxTextAlign ctx_get_text_align  (Ctx *ctx)
{
  return (CtxTextAlign)ctx_state_get (&ctx->state, CTX_text_align);
}

CtxTextBaseline ctx_get_text_baseline (Ctx *ctx)
{
  return (CtxTextBaseline)ctx_state_get (&ctx->state, CTX_text_baseline);
}

CtxLineCap ctx_get_line_cap (Ctx *ctx)
{
  return ctx->state.gstate.line_cap;
}

CtxFillRule ctx_get_fill_rule (Ctx *ctx)
{
  return ctx->state.gstate.fill_rule;
}

void ctx_line_cap (Ctx *ctx, CtxLineCap cap)
{
  if (ctx->state.gstate.line_cap != cap)
    CTX_PROCESS_U8 (CTX_LINE_CAP, cap);
}

void ctx_fill_rule (Ctx *ctx, CtxFillRule fill_rule)
{
  if (ctx->state.gstate.fill_rule != fill_rule)
    CTX_PROCESS_U8 (CTX_FILL_RULE, fill_rule);
}
void ctx_line_join (Ctx *ctx, CtxLineJoin join)
{
  if (ctx->state.gstate.line_join != join)
    CTX_PROCESS_U8 (CTX_LINE_JOIN, join);
}
void ctx_blend_mode (Ctx *ctx, CtxBlend mode)
{
  if (ctx->state.gstate.blend_mode != mode)
    CTX_PROCESS_U8 (CTX_BLEND_MODE, mode);
}
void ctx_compositing_mode (Ctx *ctx, CtxCompositingMode mode)
{
  if (ctx->state.gstate.compositing_mode != mode)
    CTX_PROCESS_U8 (CTX_COMPOSITING_MODE, mode);
}
void ctx_text_align (Ctx *ctx, CtxTextAlign text_align)
{
  CTX_PROCESS_U8 (CTX_TEXT_ALIGN, text_align);
}
void ctx_text_baseline (Ctx *ctx, CtxTextBaseline text_baseline)
{
  CTX_PROCESS_U8 (CTX_TEXT_BASELINE, text_baseline);
}
void ctx_text_direction (Ctx *ctx, CtxTextDirection text_direction)
{
  CTX_PROCESS_U8 (CTX_TEXT_DIRECTION, text_direction);
}

void
ctx_rel_curve_to (Ctx *ctx,
                  float x0, float y0,
                  float x1, float y1,
                  float x2, float y2)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[3]=
  {
    ctx_f (CTX_REL_CURVE_TO, x0, y0),
    ctx_f (CTX_CONT, x1, y1),
    ctx_f (CTX_CONT, x2, y2)
  };
  ctx_process (ctx, command);
}

void
ctx_rel_quad_to (Ctx *ctx,
                 float cx, float cy,
                 float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_REL_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void
ctx_quad_to (Ctx *ctx,
             float cx, float cy,
             float x,  float y)
{
  if (!ctx->state.has_moved)
    { return; }
  CtxEntry command[2]=
  {
    ctx_f (CTX_QUAD_TO, cx, cy),
    ctx_f (CTX_CONT, x, y)
  };
  ctx_process (ctx, command);
}

void ctx_arc (Ctx  *ctx,
              float x0, float y0,
              float radius,
              float angle1, float angle2,
              int   direction)
{
  CtxEntry command[3]=
  {
    ctx_f (CTX_ARC, x0, y0),
    ctx_f (CTX_CONT, radius, angle1),
    ctx_f (CTX_CONT, angle2, direction)
  };
  ctx_process (ctx, command);
}

static int ctx_coords_equal (float x1, float y1, float x2, float y2, float tol)
{
  float dx = x2 - x1;
  float dy = y2 - y1;
  return dx*dx + dy*dy < tol*tol;
}

static float
ctx_point_seg_dist_sq (float x, float y,
                       float vx, float vy, float wx, float wy)
{
  float l2 = ctx_pow2 (vx-wx) + ctx_pow2 (vy-wy);
  if (l2 < 0.0001)
    { return ctx_pow2 (x-vx) + ctx_pow2 (y-vy); }
  float t = ( (x - vx) * (wx - vx) + (y - vy) * (wy - vy) ) / l2;
  t = ctx_maxf (0, ctx_minf (1, t) );
  float ix = vx + t * (wx - vx);
  float iy = vy + t * (wy - vy);
  return ctx_pow2 (x-ix) + ctx_pow2 (y-iy);
}

static void
ctx_normalize (float *x, float *y)
{
  float length = ctx_hypotf ( (*x), (*y) );
  if (length > 1e-6f)
    {
      float r = 1.0f / length;
      *x *= r;
      *y *= r;
    }
}

void
ctx_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  // XXX : should partially move into rasterizer to preserve comand
  //       even if an arc preserves all geometry, just to ensure roundtripping
  //       of data
  /* from nanovg - but not quite working ; uncertain if arc or wrong
   * transfusion is the cause.
   */
  float x0 = ctx->state.x;
  float y0 = ctx->state.y;
  float dx0,dy0, dx1,dy1, a, d, cx,cy, a0,a1;
  int dir;
  if (!ctx->state.has_moved)
    { return; }
  if (1)
    {
      // Handle degenerate cases.
      if (ctx_coords_equal (x0,y0, x1,y1, 0.5f) ||
          ctx_coords_equal (x1,y1, x2,y2, 0.5f) ||
          ctx_point_seg_dist_sq (x1,y1, x0,y0, x2,y2) < 0.5 ||
          radius < 0.5)
        {
          ctx_line_to (ctx, x1,y1);
          return;
        }
    }
  // Calculate tangential circle to lines (x0,y0)-(x1,y1) and (x1,y1)-(x2,y2).
  dx0 = x0-x1;
  dy0 = y0-y1;
  dx1 = x2-x1;
  dy1 = y2-y1;
  ctx_normalize (&dx0,&dy0);
  ctx_normalize (&dx1,&dy1);
  a = ctx_acosf (dx0*dx1 + dy0*dy1);
  d = radius / ctx_tanf (a/2.0f);
#if 0
  if (d > 10000.0f)
    {
      ctx_line_to (ctx, x1, y1);
      return;
    }
#endif
  if ( (dx1*dy0 - dx0*dy1) > 0.0f)
    {
      cx = x1 + dx0*d + dy0*radius;
      cy = y1 + dy0*d + -dx0*radius;
      a0 = ctx_atan2f (dx0, -dy0);
      a1 = ctx_atan2f (-dx1, dy1);
      dir = 0;
    }
  else
    {
      cx = x1 + dx0*d + -dy0*radius;
      cy = y1 + dy0*d + dx0*radius;
      a0 = ctx_atan2f (-dx0, dy0);
      a1 = ctx_atan2f (dx1, -dy1);
      dir = 1;
    }
  ctx_arc (ctx, cx, cy, radius, a0, a1, dir);
}

void
ctx_rel_arc_to (Ctx *ctx, float x1, float y1, float x2, float y2, float radius)
{
  x1 += ctx->state.x;
  y1 += ctx->state.y;
  x2 += ctx->state.x;
  y2 += ctx->state.y;
  ctx_arc_to (ctx, x1, y1, x2, y2, radius);
}

void
ctx_exit (Ctx *ctx)
{
  CTX_PROCESS_VOID (CTX_EXIT);
}

void
ctx_flush (Ctx *ctx)
{
  /* XXX: should be fully moved into the renderers
   *      to permit different behavior and get rid
   *      of the extranous flush() vfunc.
   */
  ctx->rev++;
//  CTX_PROCESS_VOID (CTX_FLUSH);
#if 0
  //printf (" \e[?2222h");
  ctx_drawlist_compact (&ctx->drawlist);
  for (int i = 0; i < ctx->drawlist.count - 1; i++)
    {
      CtxEntry *entry = &ctx->drawlist.entries[i];
      fwrite (entry, 9, 1, stdout);
#if 0
      uint8_t  *buf = (void *) entry;
      for (int j = 0; j < 9; j++)
        { printf ("%c", buf[j]); }
#endif
    }
  printf ("Xx.Xx.Xx.");
  fflush (NULL);
#endif
  if (ctx->renderer && ctx->renderer->flush)
    ctx->renderer->flush (ctx->renderer);
  ctx->frame++;
  if (ctx->texture_cache != ctx)
    ctx->texture_cache->frame++;
  ctx->drawlist.count = 0;
  ctx_state_init (&ctx->state);
}

////////////////////////////////////////

void
ctx_interpret_style (CtxState *state, CtxEntry *entry, void *data)
{
  CtxCommand *c = (CtxCommand *) entry;
  switch (entry->code)
    {
      case CTX_LINE_DASH_OFFSET:
        state->gstate.line_dash_offset = ctx_arg_float (0);
        break;
      case CTX_LINE_WIDTH:
        state->gstate.line_width = ctx_arg_float (0);
        break;
#if CTX_ENABLE_SHADOW_BLUR
      case CTX_SHADOW_BLUR:
        state->gstate.shadow_blur = ctx_arg_float (0);
        break;
      case CTX_SHADOW_OFFSET_X:
        state->gstate.shadow_offset_x = ctx_arg_float (0);
        break;
      case CTX_SHADOW_OFFSET_Y:
        state->gstate.shadow_offset_y = ctx_arg_float (0);
        break;
#endif
      case CTX_LINE_CAP:
        state->gstate.line_cap = (CtxLineCap) ctx_arg_u8 (0);
        break;
      case CTX_FILL_RULE:
        state->gstate.fill_rule = (CtxFillRule) ctx_arg_u8 (0);
        break;
      case CTX_LINE_JOIN:
        state->gstate.line_join = (CtxLineJoin) ctx_arg_u8 (0);
        break;
      case CTX_COMPOSITING_MODE:
        state->gstate.compositing_mode = (CtxCompositingMode) ctx_arg_u8 (0);
        break;
      case CTX_BLEND_MODE:
        state->gstate.blend_mode = (CtxBlend) ctx_arg_u8 (0);
        break;
      case CTX_TEXT_ALIGN:
        ctx_state_set (state, CTX_text_align, ctx_arg_u8 (0) );
        break;
      case CTX_TEXT_BASELINE:
        ctx_state_set (state, CTX_text_baseline, ctx_arg_u8 (0) );
        break;
      case CTX_TEXT_DIRECTION:
        ctx_state_set (state, CTX_text_direction, ctx_arg_u8 (0) );
        break;
      case CTX_GLOBAL_ALPHA:
        state->gstate.global_alpha_u8 = ctx_float_to_u8 (ctx_arg_float (0) );
        state->gstate.global_alpha_f = ctx_arg_float (0);
        break;
      case CTX_FONT_SIZE:
        state->gstate.font_size = ctx_arg_float (0);
        break;
      case CTX_MITER_LIMIT:
        state->gstate.miter_limit = ctx_arg_float (0);
        break;
      case CTX_COLOR_SPACE:
        /* move this out of this function and only do it in rasterizer? XXX */
        ctx_rasterizer_colorspace_icc (state, (CtxColorSpace)c->colorspace.space_slot,
                                              (char*)c->colorspace.data,
                                              c->colorspace.data_len);
        break;
      case CTX_IMAGE_SMOOTHING:
        state->gstate.image_smoothing = c->entry.data.u8[0];
        break;
      case CTX_STROKE_SOURCE:
        state->source = 1;
        break;

      case CTX_COLOR:
        {
          int is_stroke = (state->source != 0);
          CtxSource *source = is_stroke ?
                                &state->gstate.source_stroke:
                                &state->gstate.source_fill;
          state->source = 0;

          source->type = CTX_SOURCE_COLOR;
         
          //float components[5]={c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a};
          switch ( ((int) ctx_arg_float (0)) & 511) // XXX remove 511 after stroke source is complete
            {
              case CTX_RGB:
                ctx_color_set_rgba (state, &source->color, c->rgba.r, c->rgba.g, c->rgba.b, 1.0f);
                break;
              case CTX_RGBA:
                ctx_color_set_rgba (state, &source->color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
              case CTX_DRGBA:
                ctx_color_set_drgba (state, &source->color, c->rgba.r, c->rgba.g, c->rgba.b, c->rgba.a);
                break;
#if CTX_ENABLE_CMYK
              case CTX_CMYKA:
                ctx_color_set_cmyka (state, &source->color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_CMYK:
                ctx_color_set_cmyka (state, &source->color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
              case CTX_DCMYKA:
                ctx_color_set_dcmyka (state, &source->color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, c->cmyka.a);
                break;
              case CTX_DCMYK:
                ctx_color_set_dcmyka (state, &source->color, c->cmyka.c, c->cmyka.m, c->cmyka.y, c->cmyka.k, 1.0f);
                break;
#endif
              case CTX_GRAYA:
                ctx_color_set_graya (state, &source->color, c->graya.g, c->graya.a);
                break;
              case CTX_GRAY:
                ctx_color_set_graya (state, &source->color, c->graya.g, 1.0f);
                break;
            }
        }
        break;
      case CTX_SET_RGBA_U8:
        //ctx_source_deinit (&state->gstate.source);
        //state->gstate.source_fill.type = CTX_SOURCE_COLOR;
        {
          int is_stroke = (state->source != 0);
          CtxSource *source = is_stroke ?
                                &state->gstate.source_stroke:
                                &state->gstate.source_fill;
          state->source = 0;

          source->type = CTX_SOURCE_COLOR;

          ctx_color_set_RGBA8 (state, &source->color,
                               ctx_arg_u8 (0),
                               ctx_arg_u8 (1),
                               ctx_arg_u8 (2),
                               ctx_arg_u8 (3) );
        }
        //for (int i = 0; i < 4; i ++)
        //  state->gstate.source.color.rgba[i] = ctx_arg_u8(i);
        break;
      //case CTX_TEXTURE:
      //  state->gstate.source.type = CTX_SOURCE_
      //  break;
      case CTX_LINEAR_GRADIENT:
        {
          int is_stroke = (state->source != 0);
          CtxSource *source = is_stroke ?
                                &state->gstate.source_stroke:
                                &state->gstate.source_fill;
          state->source = is_stroke ? 2 : 0;

          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float x1 = ctx_arg_float (2);
          float y1 = ctx_arg_float (3);
          float dx, dy, length, start, end;

          length = ctx_hypotf (x1-x0,y1-y0);
          dx = (x1-x0) / length;
          dy = (y1-y0) / length;
          start = (x0 * dx + y0 * dy) / length;
          end =   (x1 * dx + y1 * dy) / length;
          source->linear_gradient.length = length;
          source->linear_gradient.dx = dx;
          source->linear_gradient.dy = dy;
          source->linear_gradient.start = start;
          source->linear_gradient.end = end;
          source->linear_gradient.rdelta = (end-start)!=0.0?1.0f/(end - start):1.0;
          source->type = CTX_SOURCE_LINEAR_GRADIENT;
          source->transform = state->gstate.transform;
          ctx_matrix_invert (&source->transform);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        {
          int is_stroke = (state->source != 0);
          CtxSource *source = is_stroke ?
                                &state->gstate.source_stroke:
                                &state->gstate.source_fill;
          state->source = is_stroke ? 2 : 0;

          float x0 = ctx_arg_float (0);
          float y0 = ctx_arg_float (1);
          float r0 = ctx_arg_float (2);
          float x1 = ctx_arg_float (3);
          float y1 = ctx_arg_float (4);
          float r1 = ctx_arg_float (5);
          source->radial_gradient.x0 = x0;
          source->radial_gradient.y0 = y0;
          source->radial_gradient.r0 = r0;
          source->radial_gradient.x1 = x1;
          source->radial_gradient.y1 = y1;
          source->radial_gradient.r1 = r1;
          source->radial_gradient.rdelta = (r1 - r0) != 0.0 ? 1.0f/(r1-r0):0.0;
          source->type      = CTX_SOURCE_RADIAL_GRADIENT;
          source->transform = state->gstate.transform;
          ctx_matrix_invert (&source->transform);
        }
        break;
    }
}

void
ctx_interpret_transforms (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
    {
      case CTX_SAVE:
        ctx_gstate_push (state);
        break;
      case CTX_RESTORE:
        ctx_gstate_pop (state);
        break;
      case CTX_IDENTITY:
        ctx_matrix_identity (&state->gstate.transform);
        break;
      case CTX_TRANSLATE:
        ctx_matrix_translate (&state->gstate.transform,
                              ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_SCALE:
        ctx_matrix_scale (&state->gstate.transform,
                          ctx_arg_float (0), ctx_arg_float (1) );
        break;
      case CTX_ROTATE:
        ctx_matrix_rotate (&state->gstate.transform, ctx_arg_float (0) );
        break;
      case CTX_APPLY_TRANSFORM:
        {
          CtxMatrix m;
          ctx_matrix_set (&m,
                          ctx_arg_float (0), ctx_arg_float (1),
                          ctx_arg_float (2), ctx_arg_float (3),
                          ctx_arg_float (4), ctx_arg_float (5) );
          ctx_matrix_multiply (&state->gstate.transform,
                               &state->gstate.transform, &m); // XXX verify order
        }
#if 0
        ctx_matrix_set (&state->gstate.transform,
                        ctx_arg_float (0), ctx_arg_float (1),
                        ctx_arg_float (2), ctx_arg_float (3),
                        ctx_arg_float (4), ctx_arg_float (5) );
#endif
        break;
    }
}

/*
 * this transforms the contents of entry according to ctx->transformation
 */
void
ctx_interpret_pos_transform (CtxState *state, CtxEntry *entry, void *data)
{
  CtxCommand *c = (CtxCommand *) entry;
  float start_x = state->x;
  float start_y = state->y;
  int had_moved = state->has_moved;
  switch (entry->code)
    {
      case CTX_MOVE_TO:
      case CTX_LINE_TO:
        {
          float x = c->c.x0;
          float y = c->c.y0;
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              _ctx_user_to_device (state, &x, &y);
              ctx_arg_float (0) = x;
              ctx_arg_float (1) = y;
            }
        }
        break;
      case CTX_ARC:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            float temp;
            _ctx_user_to_device (state, &c->arc.x, &c->arc.y);
            temp = 0;
            _ctx_user_to_device_distance (state, &c->arc.radius, &temp);
          }
        break;
      case CTX_LINEAR_GRADIENT:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
        {
        _ctx_user_to_device (state, &c->linear_gradient.x1, &c->linear_gradient.y1);
        _ctx_user_to_device (state, &c->linear_gradient.x2, &c->linear_gradient.y2);
        }
        break;
      case CTX_RADIAL_GRADIENT:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
        {
          float temp;
          _ctx_user_to_device (state, &c->radial_gradient.x1, &c->radial_gradient.y1);
          temp = 0;
          _ctx_user_to_device_distance (state, &c->radial_gradient.r1, &temp);
          _ctx_user_to_device (state, &c->radial_gradient.x2, &c->radial_gradient.y2);
          temp = 0;
          _ctx_user_to_device_distance (state, &c->radial_gradient.r2, &temp);
        }
        break;
      case CTX_CURVE_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 3; c ++)
              {
                float x = entry[c].data.f[0];
                float y = entry[c].data.f[1];
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
          }
        break;
      case CTX_QUAD_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 2; c ++)
              {
                float x = entry[c].data.f[0];
                float y = entry[c].data.f[1];
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
          }
        break;
      case CTX_REL_MOVE_TO:
      case CTX_REL_LINE_TO:
        if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
          {
            for (int c = 0; c < 1; c ++)
              {
                float x = state->x;
                float y = state->y;
                _ctx_user_to_device (state, &x, &y);
                entry[c].data.f[0] = x;
                entry[c].data.f[1] = y;
              }
            if (entry->code == CTX_REL_MOVE_TO)
              { entry->code = CTX_MOVE_TO; }
            else
              { entry->code = CTX_LINE_TO; }
          }
        break;
      case CTX_REL_CURVE_TO:
        {
          float nx = state->x + ctx_arg_float (4);
          float ny = state->y + ctx_arg_float (5);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 3; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  _ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_CURVE_TO;
            }
        }
        break;
      case CTX_REL_QUAD_TO:
        {
          float nx = state->x + ctx_arg_float (2);
          float ny = state->y + ctx_arg_float (3);
          if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) )
            {
              for (int c = 0; c < 2; c ++)
                {
                  float x = nx + entry[c].data.f[0];
                  float y = ny + entry[c].data.f[1];
                  _ctx_user_to_device (state, &x, &y);
                  entry[c].data.f[0] = x;
                  entry[c].data.f[1] = y;
                }
              entry->code = CTX_QUAD_TO;
            }
        }
        break;
    }
  if ((((Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE))
    {
      int components = 0;
      _ctx_user_to_device (state, &start_x, &start_y);
      switch (entry->code)
        {
          case CTX_MOVE_TO:
            if (had_moved) { components = 1; }
            break;
          case CTX_LINE_TO:
            components = 1;
            break;
          case CTX_CURVE_TO:
            components = 3;
            break;
          case CTX_QUAD_TO:
            components = 2;
            break;
        }
      if (components)
        {
          for (int c = 0; c < components; c++)
            {
              entry[c].data.f[0] -= start_x;
              entry[c].data.f[1] -= start_y;
            }
          switch (entry->code)
            {
              case CTX_MOVE_TO:
                entry[0].code = CTX_REL_MOVE_TO;
                break;
              case CTX_LINE_TO:
                entry[0].code = CTX_REL_LINE_TO;
                break;
                break;
              case CTX_CURVE_TO:
                entry[0].code = CTX_REL_CURVE_TO;
                break;
              case CTX_QUAD_TO:
                entry[0].code = CTX_REL_QUAD_TO;
                break;
            }
        }
    }
}

void
ctx_interpret_pos_bare (CtxState *state, CtxEntry *entry, void *data)
{
  switch (entry->code)
    {
      case CTX_RESET:
        ctx_state_init (state);
        break;
      case CTX_CLIP:
      case CTX_BEGIN_PATH:
      case CTX_FILL:
      case CTX_STROKE:
        state->has_moved = 0;
        break;
      case CTX_MOVE_TO:
      case CTX_LINE_TO:
        {
          float x = ctx_arg_float (0);
          float y = ctx_arg_float (1);
          state->x = x;
          state->y = y;
          if (!state->has_moved)
            {
              state->has_moved = 1;
            }
        }
        break;
      case CTX_CURVE_TO:
        state->x = ctx_arg_float (4);
        state->y = ctx_arg_float (5);
        if (!state->has_moved)
          {
            state->has_moved = 1;
          }
        break;
      case CTX_QUAD_TO:
        state->x = ctx_arg_float (2);
        state->y = ctx_arg_float (3);
        if (!state->has_moved)
          {
            state->has_moved = 1;
          }
        break;
      case CTX_ARC:
        state->x = ctx_arg_float (0) + ctx_cosf (ctx_arg_float (4) ) * ctx_arg_float (2);
        state->y = ctx_arg_float (1) + ctx_sinf (ctx_arg_float (4) ) * ctx_arg_float (2);
        break;
      case CTX_REL_MOVE_TO:
      case CTX_REL_LINE_TO:
        state->x += ctx_arg_float (0);
        state->y += ctx_arg_float (1);
        break;
      case CTX_REL_CURVE_TO:
        state->x += ctx_arg_float (4);
        state->y += ctx_arg_float (5);
        break;
      case CTX_REL_QUAD_TO:
        state->x += ctx_arg_float (2);
        state->y += ctx_arg_float (3);
        break;
        // XXX missing some smooths
    }
}

void
ctx_interpret_pos (CtxState *state, CtxEntry *entry, void *data)
{
  if ( ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_SCREEN_SPACE) ||
       ( ( (Ctx *) (data) )->transformation & CTX_TRANSFORMATION_RELATIVE) )
    {
      ctx_interpret_pos_transform (state, entry, data);
    }
  ctx_interpret_pos_bare (state, entry, data);
}

#if CTX_BABL
void ctx_colorspace_babl (CtxState   *state,
                          CtxColorSpace  icc_slot,
                          const Babl *space);
#endif

static void
ctx_state_init (CtxState *state)
{
  ctx_memset (state, 0, sizeof (CtxState) );
  state->gstate.global_alpha_u8 = 255;
  state->gstate.global_alpha_f  = 1.0;
  state->gstate.font_size       = 12;
  state->gstate.line_width      = 2.0;
  state->gstate.image_smoothing = 1;
  state->gstate.source_stroke.type = CTX_SOURCE_INHERIT_FILL;
  ctx_state_set (state, CTX_line_spacing, 1.0f);
  state->min_x                  = 8192;
  state->min_y                  = 8192;
  state->max_x                  = -8192;
  state->max_y                  = -8192;
  ctx_matrix_identity (&state->gstate.transform);
#if CTX_CM
#if CTX_BABL
  //ctx_colorspace_babl (state, CTX_COLOR_SPACE_USER_RGB,   babl_space ("sRGB"));
  //ctx_colorspace_babl (state, CTX_COLOR_SPACE_DEVICE_RGB, babl_space ("ACEScg"));
#endif
#endif
}

void _ctx_set_transformation (Ctx *ctx, int transformation)
{
  ctx->transformation = transformation;
}

static void
_ctx_init (Ctx *ctx)
{
  for (int i = 0; i <256;i++)
    ctx_u8_float[i] = i/255.0f;

  ctx_state_init (&ctx->state);

  ctx->renderer = NULL;
#if CTX_CURRENT_PATH
  ctx->current_path.flags |= CTX_DRAWLIST_CURRENT_PATH;
#endif
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_SCREEN_SPACE;
  //ctx->transformation |= (CtxTransformation) CTX_TRANSFORMATION_RELATIVE;
#if CTX_BITPACK
  ctx->drawlist.flags |= CTX_TRANSFORMATION_BITPACK;
#endif
  ctx->texture_cache = ctx;
}

static void ctx_setup ();

#if CTX_DRAWLIST_STATIC
static Ctx ctx_state;
#endif

void ctx_set_renderer (Ctx  *ctx,
                       void *renderer)
{
  if (ctx->renderer && ctx->renderer->free)
    ctx->renderer->free (ctx->renderer);
  ctx->renderer = (CtxImplementation*)renderer;
}

void *ctx_get_renderer (Ctx *ctx)
{
  return ctx->renderer;
}

Ctx *
ctx_new (void)
{
  ctx_setup ();
#if CTX_DRAWLIST_STATIC
  Ctx *ctx = &ctx_state;
#else
  Ctx *ctx = (Ctx *) malloc (sizeof (Ctx) );
#endif
  ctx_memset (ctx, 0, sizeof (Ctx) );
  _ctx_init (ctx);
  return ctx;
}

void
ctx_drawlist_deinit (CtxDrawlist *drawlist)
{
#if !CTX_DRAWLIST_STATIC
  if (drawlist->entries && ! (drawlist->flags & CTX_DRAWLIST_DOESNT_OWN_ENTRIES) )
    {
      free (drawlist->entries); 
    }
#endif
  drawlist->entries = NULL;
  drawlist->size = 0;
}

static void ctx_deinit (Ctx *ctx)
{
  if (ctx->renderer)
    {
      if (ctx->renderer->free)
        ctx->renderer->free (ctx->renderer);
      ctx->renderer    = NULL;
    }
  ctx_drawlist_deinit (&ctx->drawlist);
#if CTX_CURRENT_PATH
  ctx_drawlist_deinit (&ctx->current_path);
#endif
}

void ctx_free (Ctx *ctx)
{
  if (!ctx)
    { return; }
#if CTX_EVENTS
  ctx_clear_bindings (ctx);
#endif
  ctx_deinit (ctx);
#if !CTX_DRAWLIST_STATIC
  free (ctx);
#endif
}

Ctx *ctx_new_for_drawlist (void *data, size_t length)
{
  Ctx *ctx = ctx_new ();
  ctx->drawlist.flags   |= CTX_DRAWLIST_DOESNT_OWN_ENTRIES;
  ctx->drawlist.entries  = (CtxEntry *) data;
  ctx->drawlist.count    = length / sizeof (CtxEntry);
  return ctx;
}

#ifdef CTX_HAVE_SIMD
void ctx_simd_setup ();
#endif

static void ctx_setup ()
{
#ifdef CTX_HAVE_SIMD
  ctx_simd_setup ();
#endif
  ctx_font_setup ();
}

void
ctx_render_ctx (Ctx *ctx, Ctx *d_ctx)
{
  CtxIterator iterator;
  CtxCommand *command;
  ctx_iterator_init (&iterator, &ctx->drawlist, 0,
                     CTX_ITERATOR_EXPAND_BITPACK);
  while ( (command = ctx_iterator_next (&iterator) ) )
    {
       ctx_process (d_ctx, &command->entry);
    }
}

void ctx_quit (Ctx *ctx)
{
#if CTX_EVENTS
  ctx->quit ++;
#endif
}

int  ctx_has_quit (Ctx *ctx)
{
#if CTX_EVENTS
  return (ctx->quit);
#else
  return 1; 
#endif
}

int ctx_pixel_format_bits_per_pixel (CtxPixelFormat format)
{
  CtxPixelFormatInfo *info = ctx_pixel_format_info (format);
  if (info)
    return info->bpp;
  return -1;
}

int ctx_pixel_format_get_stride (CtxPixelFormat format, int width)
{
  CtxPixelFormatInfo *info = ctx_pixel_format_info (format);
  if (info)
  {
    switch (info->bpp)
    {
      case 0:
      case 1:
        return (width + 7)/8;
      case 2:
        return (width + 3)/4;
      case 4:
        return (width + 1)/2;
      default:
        return width * (info->bpp / 8);
    }
  }
  return width;
}

int ctx_pixel_format_ebpp (CtxPixelFormat format)
{
  CtxPixelFormatInfo *info = ctx_pixel_format_info (format);
  if (info)
    return info->ebpp;
  return -1;
}

int ctx_pixel_format_components (CtxPixelFormat format)
{
  CtxPixelFormatInfo *info = ctx_pixel_format_info (format);
  if (info)
    return info->components;
  return -1;
}

#if CTX_EVENTS
void         ctx_set_cursor (Ctx *ctx, CtxCursor cursor)
{
  if (ctx->cursor != cursor)
  {
    ctx_set_dirty (ctx, 1);
    ctx->cursor = cursor;
  }
}
CtxCursor    ctx_get_cursor (Ctx *ctx)
{
  return ctx->cursor;
}

void ctx_set_clipboard (Ctx *ctx, const char *text)
{
  if (ctx->renderer && ctx->renderer->set_clipboard)
  {
    ctx->renderer->set_clipboard (ctx->renderer, text);
    return;
  }
}

char *ctx_get_clipboard (Ctx *ctx)
{
  if (ctx->renderer && ctx->renderer->get_clipboard)
  {
    return ctx->renderer->get_clipboard (ctx->renderer);
  }
  return strdup ("");
}

void ctx_set_texture_source (Ctx *ctx, Ctx *texture_source)
{
  ((CtxRasterizer*)ctx->renderer)->texture_source = texture_source;
}

void ctx_set_texture_cache (Ctx *ctx, Ctx *texture_cache)
{
  ctx->texture_cache = texture_cache;
}

void ctx_set_transform (Ctx *ctx, float a, float b, float c, float d, float e, float f)
{
  ctx_identity (ctx);
  ctx_apply_transform (ctx, a, b, c, d, e, f);
}
#ifndef NO_LIBCURL
#include <curl/curl.h>
static size_t
ctx_string_append_callback (void *contents, size_t size, size_t nmemb, void *userp)
{
  CtxString *string = (CtxString*)userp;
  ctx_string_append_data ((CtxString*)string, contents, size * nmemb);
  return size * nmemb;
}

#endif

int
ctx_get_contents (const char     *uri,
                  unsigned char **contents,
                  long           *length)
{
  char temp_uri[PATH_MAX]; // XXX XXX breaks with data uri's
  if (uri[0] == '/')
  {
    snprintf (temp_uri, sizeof (temp_uri)-1, "file://%s", uri);
    uri = temp_uri;
  }
  else
  {
    snprintf (temp_uri, sizeof (temp_uri)-1, uri);
    uri = temp_uri;
  }

  if (strchr (uri, '#'))
   strchr (uri, '#')[0]=0;

  for (CtxList *l = registered_contents; l; l = l->next)
  {
    CtxFileContent *c = (CtxFileContent*)l->data;
    if (!strcmp (c->path, uri))
    {
      contents = malloc (c->length+1);
      contents[c->length]=0;
      if (length) *length = c->length;
      return 0;
    }
  }

  if (!strncmp (uri, "file://", 5))
  {
    if (strchr (uri, '?'))
     strchr (uri, '?')[0]=0;
  }

  if (!strncmp (uri, "file://", 7))
    return __ctx_file_get_contents (uri + 7, contents, length);
  else
  {
#ifndef NO_LIBCURL
  CURL *curl = curl_easy_init ();
  CURLcode res;

  curl_easy_setopt(curl, CURLOPT_URL, uri);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CtxString *string = ctx_string_new ("");

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ctx_string_append_callback);
   /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)string);

  curl_easy_setopt(curl, CURLOPT_USERAGENT, "ctx/0.0");

   res = curl_easy_perform(curl);
  /* check for errors */
  if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
            curl_easy_strerror(res));
     curl_easy_cleanup (curl);
  }
  else
  {
     *contents = (unsigned char*)string->str;
     *length = string->length;
     ctx_string_free (string, 0);
     curl_easy_cleanup (curl);
     return 0;
  }
#else
    return __ctx_file_get_contents (uri, contents, length);
#endif
  }
  return -1;
}


#endif

#endif // CTX_IMPLEMENTATION

#if CTX_COMPOSITE


#define CTX_RGBA8_R_SHIFT  0
#define CTX_RGBA8_G_SHIFT  8
#define CTX_RGBA8_B_SHIFT  16
#define CTX_RGBA8_A_SHIFT  24

#define CTX_RGBA8_R_MASK   (0xff << CTX_RGBA8_R_SHIFT)
#define CTX_RGBA8_G_MASK   (0xff << CTX_RGBA8_G_SHIFT)
#define CTX_RGBA8_B_MASK   (0xff << CTX_RGBA8_B_SHIFT)
#define CTX_RGBA8_A_MASK   (0xff << CTX_RGBA8_A_SHIFT)

#define CTX_RGBA8_RB_MASK  (CTX_RGBA8_R_MASK | CTX_RGBA8_B_MASK)
#define CTX_RGBA8_GA_MASK  (CTX_RGBA8_G_MASK | CTX_RGBA8_A_MASK)

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE


inline static int ctx_grad_index (float v)
{
  int ret = v * (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f) + 0.5f;
  if (ret >= CTX_GRADIENT_CACHE_ELEMENTS)
    return CTX_GRADIENT_CACHE_ELEMENTS - 1;
  if (ret >= 0 && ret < CTX_GRADIENT_CACHE_ELEMENTS)
    return ret;
  return 0;
}

extern uint8_t ctx_gradient_cache_u8[CTX_GRADIENT_CACHE_ELEMENTS][4];
extern uint8_t ctx_gradient_cache_u8_a[CTX_GRADIENT_CACHE_ELEMENTS][4];
extern int ctx_gradient_cache_valid;

//static void
//ctx_gradient_cache_reset (void)
//{
//  ctx_gradient_cache_valid = 0;
//}


#endif

CTX_INLINE static void
_ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }

  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v * 255;
      rgba[3] = 255;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      uint8_t stop_rgba[4];
      uint8_t next_rgba[4];
      ctx_color_get_rgba8 (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_rgba8 (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) * 255 / (next_stop->pos - stop->pos);
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba8 (rasterizer->state, color, rgba);
  if (rasterizer->swap_red_green)
  {
    uint8_t tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
}

#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer);
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
#if CTX_GRADIENT_CACHE
  uint32_t* rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(x)][0]));
  *((uint32_t*)rgba) = *rgbap;
#else
 _ctx_fragment_gradient_1d_RGBA8 (rasterizer, x, y, rgba);
#endif
}
#endif

CTX_INLINE static void
ctx_RGBA8_associate_alpha (uint8_t *u8)
{
  uint32_t val = *((uint32_t*)(u8));
  int a = val >> CTX_RGBA8_A_SHIFT;
  if (a!=255)
  {
    if (a)
    {
      uint32_t g = (((val & CTX_RGBA8_G_MASK) * a) >> 8) & CTX_RGBA8_G_MASK;
      uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * a) >> 8) & CTX_RGBA8_RB_MASK;
      *((uint32_t*)(u8)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
    }
    else
    {
      *((uint32_t*)(u8)) = 0;
    }
  }
}

CTX_INLINE static void
ctx_u8_associate_alpha (int components, uint8_t *u8)
{
  switch (u8[components-1])
  {
          case 255:break;
          case 0: 
            for (int c = 0; c < components-1; c++)
             u8[c] = 0;
            break;
          default:
  for (int c = 0; c < components-1; c++)
    u8[c] = (u8[c] * u8[components-1]) /255;
  }
}

#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
static void
ctx_gradient_cache_prime (CtxRasterizer *rasterizer)
{
  if (ctx_gradient_cache_valid)
    return;
  for (int u = 0; u < CTX_GRADIENT_CACHE_ELEMENTS; u++)
  {
    float v = u / (CTX_GRADIENT_CACHE_ELEMENTS - 1.0f);
    _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0f, &ctx_gradient_cache_u8[u][0]);
    //*((uint32_t*)(&ctx_gradient_cache_u8_a[u][0]))= *((uint32_t*)(&ctx_gradient_cache_u8[u][0]));
    memcpy(&ctx_gradient_cache_u8_a[u][0], &ctx_gradient_cache_u8[u][0], 4);
    ctx_RGBA8_associate_alpha (&ctx_gradient_cache_u8_a[u][0]);
  }
  ctx_gradient_cache_valid = 1;
}
#endif

CTX_INLINE static void
ctx_fragment_gradient_1d_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, uint8_t *rgba)
{
  float v = x;
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }
  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v * 255;
      rgba[1] = 255;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      uint8_t stop_rgba[4];
      uint8_t next_rgba[4];
      ctx_color_get_graya_u8 (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_graya_u8 (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) * 255 / (next_stop->pos - stop->pos);
      for (int c = 0; c < 2; c++)
        { rgba[c] = ctx_lerp_u8 (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_graya_u8 (rasterizer->state, color, rgba);
}

CTX_INLINE static void
ctx_fragment_gradient_1d_RGBAF (CtxRasterizer *rasterizer, float v, float y, float *rgba)
{
  CtxGradient *g = &rasterizer->state->gradient;
  if (v < 0) { v = 0; }
  if (v > 1) { v = 1; }
  if (g->n_stops == 0)
    {
      rgba[0] = rgba[1] = rgba[2] = v;
      rgba[3] = 1.0;
      return;
    }
  CtxGradientStop *stop      = NULL;
  CtxGradientStop *next_stop = &g->stops[0];
  CtxColor *color;
  for (int s = 0; s < g->n_stops; s++)
    {
      stop      = &g->stops[s];
      next_stop = &g->stops[s+1];
      if (s + 1 >= g->n_stops) { next_stop = NULL; }
      if (v >= stop->pos && next_stop && v < next_stop->pos)
        { break; }
      stop = NULL;
      next_stop = NULL;
    }
  if (stop == NULL && next_stop)
    {
      color = & (next_stop->color);
    }
  else if (stop && next_stop == NULL)
    {
      color = & (stop->color);
    }
  else if (stop && next_stop)
    {
      float stop_rgba[4];
      float next_rgba[4];
      ctx_color_get_rgba (rasterizer->state, & (stop->color), stop_rgba);
      ctx_color_get_rgba (rasterizer->state, & (next_stop->color), next_rgba);
      int dx = (v - stop->pos) / (next_stop->pos - stop->pos);
      for (int c = 0; c < 4; c++)
        { rgba[c] = ctx_lerpf (stop_rgba[c], next_rgba[c], dx); }
      return;
    }
  else
    {
      color = & (g->stops[g->n_stops-1].color);
    }
  ctx_color_get_rgba (rasterizer->state, color, rgba);
}
#endif

static void
ctx_fragment_image_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);

  for (int i = 0; i < count; i ++)
  {

  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  int width = buffer->width;
  int height = buffer->height;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba)) = 0;
    }
  else
    {
      int bpp = buffer->format->bpp/8;
      if (rasterizer->state->gstate.image_smoothing)
      {
      uint8_t *src00 = (uint8_t *) buffer->data;
      src00 += v * buffer->stride + u * bpp;
      uint8_t *src01 = src00;
      if ( u + 1 < width)
      {
        src01 = src00 + bpp;
      }
      uint8_t *src11 = src01;
      uint8_t *src10 = src00;
      if ( v + 1 < height)
      {
        src10 = src00 + buffer->stride;
        src11 = src01 + buffer->stride;
      }
      float dx = (x-(int)(x)) * 255.9;
      float dy = (y-(int)(y)) * 255.9;

      switch (bpp)
      {
      case 1:
        rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dx),
                               ctx_lerp_u8 (src10[0], src11[0], dx), dy);
        rgba[3] = 255;
        break;
      case 2:
        rgba[0] = rgba[1] = rgba[2] = ctx_lerp_u8 (ctx_lerp_u8 (src00[0], src01[0], dx),
                               ctx_lerp_u8 (src10[0], src11[0], dx), dy);
        rgba[3] = ctx_lerp_u8 (ctx_lerp_u8 (src00[1], src01[1], dx),
                               ctx_lerp_u8 (src10[1], src11[1], dx), dy);
        break;
      case 3:
      for (int c = 0; c < bpp; c++)
        { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                                 ctx_lerp_u8 (src10[c], src11[c], dx), dy);
                
        }
        rgba[3]=255;
        break;
      break;
      case 4:
      for (int c = 0; c < bpp; c++)
        { rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                                 ctx_lerp_u8 (src10[c], src11[c], dx), dy);
                
        }
      }
      }
      else
      {
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u * bpp;
      switch (bpp)
        {
          case 1:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = 255;
            break;
          case 2:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[0]; }
            rgba[3] = src[1];
            break;
          case 3:
            for (int c = 0; c < 3; c++)
              { rgba[c] = src[c]; }
            rgba[3] = 255;
            break;
          case 4:
            for (int c = 0; c < 4; c++)
              { rgba[c] = src[c]; }
            break;
        }
      }
      if (rasterizer->swap_red_green)
      {
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
      }
    }
    rgba += 4;
    x += dx;
    y += dy;
  }
}

#if CTX_DITHER
static inline int ctx_dither_mask_a (int x, int y, int c, int divisor)
{
  /* https://pippin.gimp.org/a_dither/ */
  return ( ( ( ( (x + c * 67) + y * 236) * 119) & 255 )-127) / divisor;
}

inline static void
ctx_dither_rgba_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    { return; }
  for (int c = 0; c < 3; c ++)
    {
      int val = rgba[c] + ctx_dither_mask_a (x, y, 0, c==1?dither_green:dither_red_blue);
      rgba[c] = CTX_CLAMP (val, 0, 255);
    }
}

inline static void
ctx_dither_graya_u8 (uint8_t *rgba, int x, int y, int dither_red_blue, int dither_green)
{
  if (dither_red_blue == 0)
    { return; }
  for (int c = 0; c < 1; c ++)
    {
      int val = rgba[c] + ctx_dither_mask_a (x, y, 0, dither_red_blue);
      rgba[c] = CTX_CLAMP (val, 0, 255);
    }
}
#endif

CTX_INLINE static void
ctx_RGBA8_deassociate_alpha (const uint8_t *in, uint8_t *out)
{
    uint32_t val = *((uint32_t*)(in));
    int a = val >> CTX_RGBA8_A_SHIFT;
    if (a)
    {
    if (a ==255)
    {
      *((uint32_t*)(out)) = val;
    } else
    {
      uint32_t g = (((val & CTX_RGBA8_G_MASK) * 255 / a) >> 8) & CTX_RGBA8_G_MASK;
      uint32_t rb =(((val & CTX_RGBA8_RB_MASK) * 255 / a) >> 8) & CTX_RGBA8_RB_MASK;
      *((uint32_t*)(out)) = g|rb|(a << CTX_RGBA8_A_SHIFT);
    }
    }
    else
    {
      *((uint32_t*)(out)) = 0;
    }
}

CTX_INLINE static void
ctx_u8_deassociate_alpha (int components, const uint8_t *in, uint8_t *out)
{
  if (in[components-1])
  {
    if (in[components-1] != 255)
    for (int c = 0; c < components-1; c++)
      out[c] = (in[c] * 255) / in[components-1];
    else
    for (int c = 0; c < components-1; c++)
      out[c] = in[c];
    out[components-1] = in[components-1];
  }
  else
  {
  for (int c = 0; c < components; c++)
    out[c] = 0;
  }
}

CTX_INLINE static void
ctx_float_associate_alpha (int components, float *rgba)
{
  float alpha = rgba[components-1];
  for (int c = 0; c < components-1; c++)
    rgba[c] *= alpha;
}

CTX_INLINE static void
ctx_float_deassociate_alpha (int components, float *rgba, float *dst)
{
  float ralpha = rgba[components-1];
  if (ralpha != 0.0) ralpha = 1.0/ralpha;

  for (int c = 0; c < components-1; c++)
    dst[c] = (rgba[c] * ralpha);
  dst[components-1] = rgba[components-1];
}

CTX_INLINE static void
ctx_RGBAF_associate_alpha (float *rgba)
{
  ctx_float_associate_alpha (4, rgba);
}

CTX_INLINE static void
ctx_RGBAF_deassociate_alpha (float *rgba, float *dst)
{
  ctx_float_deassociate_alpha (4, rgba, dst);
}


static inline void ctx_swap_red_green_u8 (void *data)
{
  uint8_t *rgba = (uint8_t*)data;
  uint8_t tmp = rgba[0];
  rgba[0] = rgba[2];
  rgba[2] = tmp;
}

static void
ctx_fragment_swap_red_green_u8 (void *out, int count)
{
  uint8_t *rgba = (uint8_t*)out;
  for (int x = 0; x < count; x++)
  {
    ctx_swap_red_green_u8 (rgba);
    rgba += 4;
  }
}

/**** rgb8 ***/

static void
ctx_fragment_image_rgb8_RGBA8_box (CtxRasterizer *rasterizer,
                                   float x,
                                   float y,
                                   void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  int width = buffer->width;
  int height = buffer->height;

  for (int i = 0; i < count; i++)
  {

  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 3;
      rgba[3]=255;
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
          int dim = (1.0 / factor) / 2;
          uint64_t sum[4]={0,0,0,0};
          int count = 0;
          for (int ou = - dim; ou < dim; ou++)
          for (int ov = - dim; ov < dim; ov++)
          {
            uint8_t *src = (uint8_t *) buffer->data;
            int o = (v+ov) * width + (u + ou);

            if (o>=0 && o < width * height)
            {
              src += o * bpp;

              for (int c = 0; c < bpp; c++)
                sum[c] += src[c];
              count ++;
            }
          }
          if (count)
            for (int c = 0; c < bpp; c++)
              rgba[c] = sum[c]/count;
    }
    rgba += 4;
    x += dx;
    y += dy;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgb8_RGBA8_box_swap_red_green (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgb8_RGBA8_bi (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  int width = buffer->width;
  int height = buffer->height;

  for (int i = 0; i < count; i++)
  {

  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  if ( u < 0 || v < 0 ||
       u >= width ||
       v >= height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 3;
      rgba[3]=255;
      uint8_t *src00 = (uint8_t *) buffer->data;
      int stride = buffer->stride;
      src00 += v * stride + u * bpp;
      uint8_t *src01 = src00;
      if ( u + 1 < width)
      {
        src01 = src00 + bpp;
      }
      uint8_t *src11 = src01;
      uint8_t *src10 = src00;
      if ( v + 1 < height)
      {
        src10 = src00 + stride;
        src11 = src01 + stride;
      }
      float dx = (x-(int)(x)) * 255.9f;
      float dy = (y-(int)(y)) * 255.9f;
      for (int c = 0; c < bpp; c++)
      {
        rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                               ctx_lerp_u8 (src10[c], src11[c], dx), dy);
      }
    }
    x += dx;
    y += dy;
    rgba += 4;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (CtxRasterizer *rasterizer,
                                  float x,
                                  float y,
                                  void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static CTX_INLINE void
ctx_fragment_image_rgb8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                       float x,
                                       float y,
                                       void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  uint8_t *rgba = (uint8_t *) out;
  uint8_t *src = (uint8_t *) buffer->data;

  x += 0.5f;
  y += 0.5f;

  if (dy == 0.0f && dx > 0.999f && dx < 1.001f)
  {
    int v = y - g->texture.y0;
    int u = x - g->texture.x0;
  
    if (v < buffer->height && v > 0)
    {
      int o = v * buffer->stride + u * 3;
      int i;
      for (i = 0; i < count && u < buffer->width; i++)
      {
        if (CTX_UNLIKELY(u < 0))
          {
            *((uint32_t*)(rgba))= 0;
          }
        else
          {
            rgba[0] = src[o];
            rgba[1] = src[o+1];
            rgba[2] = src[o+2]; 
            rgba[3]=255;
          }
        rgba += 4;
        o += 3;
        u+=1;
      }
      for (; i < count; i++)
      {
        *((uint32_t*)(rgba))= 0;
        rgba += 4;
      }
    }
    else
    {
      for (int i = 0; i < count; i++)
      {
        *((uint32_t*)(rgba))= 0;
        rgba+=4;
      }
    }
  }
  else
  {
    for (int i = 0; i < count; i++)
    {
  
    int u = x - g->texture.x0;
    int v = y - g->texture.y0;
    if (CTX_UNLIKELY( u < 0 || v < 0 ||
         u >= buffer->width ||
         v >= buffer->height))
      {
        *((uint32_t*)(rgba))= 0;
      }
    else
      {
        int      o = v * buffer->stride + u * 3;
        uint8_t *src00 = (uint8_t *) buffer->data;
        rgba[0] = src00[o];
        rgba[1] = src00[o+1];
        rgba[2] = src00[o+2]; 
        rgba[3]=255;
      }
  
      rgba += 4;
      x += dx;
      y += dy;
    }
  }
#if CTX_DITHER
  //ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
  //                    rasterizer->format->dither_green);
#endif
}


static CTX_INLINE void
ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (CtxRasterizer *rasterizer,
                                                      float x,
                                                      float y,
                                                      void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgb8_RGBA8 (CtxRasterizer *rasterizer,
                               float x,
                               float y,
                               void *out, int count, float dx, float dy)
{
  if (rasterizer->state->gstate.image_smoothing)
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    if (factor <= 0.50f)
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_box_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
    }
    else if (factor > 0.99f && factor < 1.01f)
    {
      // XXX missing translate test
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
    }
    else
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgb8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
    }
  }
  else
  {
    if (rasterizer->swap_red_green)
      ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
    else
      ctx_fragment_image_rgb8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  }
#if CTX_DITHER
  {
  uint8_t *rgba = (uint8_t*)out;
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
  }
#endif
}


/************** rgba8 */

static void
ctx_fragment_image_rgba8_RGBA8_box (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;

  for (int i = 0; i < count; i ++)
  {

  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 4;
      float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
          int dim = (1.0 / factor) / 2;
          uint64_t sum[4]={0,0,0,0};
          int count = 0;
          int width = buffer->width;
          int height = buffer->height;
          for (int ou = - dim; ou < dim; ou++)
          for (int ov = - dim; ov < dim; ov++)
          {
            uint8_t *src = (uint8_t *) buffer->data;
            int o = (v+ov) * width + (u + ou);

            if (o>=0 && o < width * height)
            {
              src += o * bpp;

              for (int c = 0; c < bpp; c++)
                sum[c] += src[c];
              count ++;
            }
          }
          if (count)
            for (int c = 0; c < bpp; c++)
              rgba[c] = sum[c]/count;
    }
    rgba += 4;
    x += dx;
    y += dy;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgba8_RGBA8_bi (CtxRasterizer *rasterizer,
                                   float x,
                                   float y,
                                   void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;

  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;

  for (int i = 0; i < count; i ++)
  {

  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      *((uint32_t*)(rgba))= 0;
    }
  else
    {
      int bpp = 4;
      uint8_t *src00 = (uint8_t *) buffer->data;
      int stride = buffer->stride;
      src00 += v * stride + u * bpp;
      uint8_t *src01 = src00;
      if ( u + 1 < buffer->width)
      {
        src01 = src00 + bpp;
      }
      uint8_t *src11 = src01;
      uint8_t *src10 = src00;
      if ( v + 1 < buffer->height)
      {
        src10 = src00 + stride;
        src11 = src01 + stride;
      }
      float dx = (x-(int)(x)) * 255.9;
      float dy = (y-(int)(y)) * 255.9;
      for (int c = 0; c < bpp; c++)
      {
        rgba[c] = ctx_lerp_u8 (ctx_lerp_u8 (src00[c], src01[c], dx),
                               ctx_lerp_u8 (src10[c], src11[c], dx), dy);
      }
    }


    x += dx;
    y += dy;
    rgba += 4;
  }
#if CTX_DITHER
//ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
//                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest (CtxRasterizer *rasterizer,
                                        float x,
                                        float y,
                                        void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer->color_managed;
  int bwidth = buffer->width;
  int bheight = buffer->height;
  x += 0.5f;
  y += 0.5f;

  if (dy == 0.0f && dx > 0.999f && dx < 1.001f)
  {
    int u = x - g->texture.x0;
    int v = y - g->texture.y0;
    if (u >= 0 && v >= 0 && u + count < bwidth && v < bheight)
    {
      uint32_t *src = (uint32_t *) buffer->data;
      memcpy (out, &src[v * bwidth + u], count * 4);
      return;
    }
  }
  //else
  {
    for (int i = 0; i < count; i ++)
    {
  
    int u = x - g->texture.x0;
    int v = y - g->texture.y0;
    if ( u < 0 || v < 0 ||
         u >= bwidth ||
         v >= bheight)
      {
        *((uint32_t*)(rgba))= 0;
      }
    else
      {
        int      i = (v * bwidth + u);
        uint32_t *src = (uint32_t *) buffer->data;
        *((uint32_t*)(rgba))= src[i];
      }
  
      x += dx;
      y += dy;
      rgba += 4;
    }
  }
#if CTX_DITHER
  //ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
  //                    rasterizer->format->dither_green);
#endif


#if CTX_DITHER
  //ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
  //                    rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_rgba8_RGBA8_box_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (CtxRasterizer *rasterizer,
                                    float x,
                                    float y,
                                    void *out, int count, float dx, float dy)
{
  ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  ctx_fragment_swap_red_green_u8 (out, count);
}

static void
ctx_fragment_image_rgba8_RGBA8 (CtxRasterizer *rasterizer,
                                float x,
                                float y,
                                void *out, int count, float dx, float dy)
{
  if (rasterizer->state->gstate.image_smoothing)
  {
    float factor = ctx_matrix_get_scale (&rasterizer->state->gstate.transform);
    if (factor <= 0.50f)
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_box_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_box (rasterizer, x, y, out, count, dx, dy);
    }
    else if (factor > 0.99f && factor < 1.01f)
    {
      // XXX: also verify translate == 0 for this fast path to be valid
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
    }
    else
    {
      if (rasterizer->swap_red_green)
        ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green (rasterizer, x, y, out, count, dx, dy);
      else
        ctx_fragment_image_rgba8_RGBA8_bi (rasterizer, x, y, out, count, dx, dy);
    }
  }
  else
  {
    if (rasterizer->swap_red_green)
      ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green (rasterizer, x, y, out, count, dx, dy);
    else
      ctx_fragment_image_rgba8_RGBA8_nearest (rasterizer, x, y, out, count, dx, dy);
  }
  //ctx_fragment_swap_red_green_u8 (out, count);
#if CTX_DITHER
  uint8_t *rgba = (uint8_t*)out;
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

static void
ctx_fragment_image_gray1_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  CtxBuffer *buffer = g->texture.buffer;
  ctx_assert (rasterizer);
  ctx_assert (g);
  ctx_assert (buffer);
  for (int i = 0; i < count; i ++)
  {
  int u = x - g->texture.x0;
  int v = y - g->texture.y0;
  if ( u < 0 || v < 0 ||
       u >= buffer->width ||
       v >= buffer->height)
    {
      rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
    }
  else
    {
      uint8_t *src = (uint8_t *) buffer->data;
      src += v * buffer->stride + u / 8;
      if (*src & (1<< (u & 7) ) )
        {
          rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0;
        }
      else
        {
          for (int c = 0; c < 4; c++)
            { rgba[c] = 255;
            }//g->texture.rgba[c];
            //}
        }
    }

    rgba += 4;
    x += dx;
    y += dy;
  }
}

#if CTX_GRADIENTS
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i <  count; i ++)
  {
    float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
#if CTX_GRADIENT_CACHE
    uint32_t *rgbap = (uint32_t*)&ctx_gradient_cache_u8[ctx_grad_index(v)][0];
    *((uint32_t*)rgba) = *rgbap;
#else
    ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#endif
#if CTX_DITHER
    ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                        rasterizer->format->dither_green);
#endif
    rgba += 4;
    x += dx;
    y += dy;
  }
}

static void
ctx_fragment_linear_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i <  count; i ++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
#if CTX_GRADIENT_CACHE
  uint32_t*rgbap = ((uint32_t*)(&ctx_gradient_cache_u8[ctx_grad_index(v)][0]));
  *((uint32_t*)rgba) = *rgbap;
#else
  _ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 1.0, rgba);
#endif
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
    rgba += 4;
    x += dx;
    y += dy;
  }
}

#endif

static void
ctx_fragment_color_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t  rgba[4];
  uint8_t *rgba_out = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  ctx_color_get_rgba8 (rasterizer->state, &g->color, rgba);

  if (rasterizer->swap_red_green)
  {
    int tmp = rgba[0];
    rgba[0] = rgba[2];
    rgba[2] = tmp;
  }
  for (int i = 0; i < count; i++, rgba_out+=4)
    memcpy (rgba_out, rgba, 4);
}
#if CTX_ENABLE_FLOAT

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
    float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                  g->linear_gradient.length) -
                g->linear_gradient.start) * (g->linear_gradient.rdelta);
    ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0f, rgba);
    x += dx;
    y += dy;
    rgba += 4;
  }
}

static void
ctx_fragment_radial_gradient_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *rgba = (float *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
  float v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
        v = (v - g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0f, rgba);
    x+=dx;
    y+=dy;
    rgba +=4;
  }
}
#endif


static void
ctx_fragment_color_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *rgba = (float *) out;
  for (int i = 0; i < count; i++)
  {
    CtxSource *g = &rasterizer->state->gstate.source_fill;
    ctx_color_get_rgba (rasterizer->state, &g->color, rgba);
    rgba += 4;
  }
}

static void ctx_fragment_image_RGBAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *outf = (float *) out;
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
    }
  for (int c = 0; c < 4 * count; c ++) { outf[c] = ctx_u8_to_float (rgba[c]); }
}

static CtxFragment ctx_rasterizer_get_fragment_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:           return ctx_fragment_image_RGBAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBAF;
#endif
    }
  return ctx_fragment_color_RGBAF;
}
#endif

static CtxFragment ctx_rasterizer_get_fragment_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
        switch (buffer->format->bpp)
          {
            case 1:  return ctx_fragment_image_gray1_RGBA8;
            case 24: 
              {
                if (gstate->image_smoothing)
                {
                  float factor = ctx_matrix_get_scale (&gstate->transform);
                          //fprintf (stderr, "{%.3f}", factor);
                  if (factor < 0.5f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_box_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_box;
                  }
                  else if (factor > 0.99f && factor < 1.01f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_nearest;
                  }
                  else
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgb8_RGBA8_bi_swap_red_green;
                    return ctx_fragment_image_rgb8_RGBA8_bi;
                  }
                }
                else
                {
                  if (rasterizer->swap_red_green)
                    return ctx_fragment_image_rgb8_RGBA8_nearest_swap_red_green;
                  return ctx_fragment_image_rgb8_RGBA8_nearest;
                }
              }
              break;
            case 32:
              {
                if (gstate->image_smoothing)
                {
                  float factor = ctx_matrix_get_scale (&gstate->transform);
                          //fprintf (stderr, "[%.3f]", factor);
                  if (factor < 0.5f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_box_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_box;
                  }
                  else if (factor > 0.99f && factor < 1.01f)
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_nearest;
                  }
                  else
                  {
                    if (rasterizer->swap_red_green)
                      return ctx_fragment_image_rgba8_RGBA8_bi_swap_red_green;
                    return ctx_fragment_image_rgba8_RGBA8_bi;
                  }
                }
                else
                {
                  if (rasterizer->swap_red_green)
                    return ctx_fragment_image_rgba8_RGBA8_nearest_swap_red_green;
                  return ctx_fragment_image_rgba8_RGBA8_nearest;
                }
              }
            default: return ctx_fragment_image_RGBA8;
          }
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_RGBA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_RGBA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_RGBA8;
#endif
    }
  return ctx_fragment_color_RGBA8;
}

static void
ctx_init_uv (CtxRasterizer *rasterizer,
             int x0, int count,
             float *u0, float *v0, float *ud, float *vd)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  *u0 = x0;
  *v0 = rasterizer->scanline / 15;//rasterizer->aa;
  float u1 = *u0 + count;
  float v1 = *v0;

  ctx_matrix_apply_transform (&gstate->source_fill.transform, u0, v0);
  ctx_matrix_apply_transform (&gstate->source_fill.transform, &u1, &v1);

  *ud = (u1-*u0) / (count);
  *vd = (v1-*v0) / (count);
}

#if 1
static void
ctx_u8_source_over_normal_opaque_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
    int cov = *coverage;
    if (cov)
    {
    if (cov == 255)
    {
        switch (components)
        {
          case 4:
            *((uint32_t*)(dst)) = *((uint32_t*)(src));
            break;
          default:
            for (int c = 0; c < components; c++)
              dst[c] = src[c];
        }
    }
    else
    {
        for (int c = 0; c < components; c++)
          dst[c] = dst[c]+((src[c]-dst[c]) * cov) / 255;
    }
    }
    coverage ++;
    dst+=components;
  }
}
#endif

static void
ctx_u8_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  if (rasterizer->fragment)
    {
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    }

  while (count--)
  {
    int cov = *coverage;
    if (cov == 0)
    {
      for (int c = 0; c < components; c++)
        { dst[c] = 0; }
    }
    else
    {
      if (rasterizer->fragment)
      {
        rasterizer->fragment (rasterizer, u0, v0, src, 1, ud, vd);
        u0+=ud;
        v0+=vd;
      }
    if (cov == 255)
    {
      for (int c = 0; c < components; c++)
        dst[c] = src[c];
    }
    else
    {
      uint8_t ralpha = 255 - cov;
      for (int c = 0; c < components; c++)
        { dst[c] = (src[c]*cov + 0 * ralpha) / 255; }
    }
    }
    dst += components;
    coverage ++;
  }
}

static void
ctx_u8_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  while (count--)
  {
#if 0
    int cov = *coverage;
    if (cov)
    {
      if (cov == 255)
      {
#endif
           //__attribute__ ((fallthrough));
        switch (components)
        {
          case 1: dst[0] = 0; break;
          case 3: dst[2] = 0;
           /* FALLTHROUGH */
          case 2: *((uint16_t*)(dst)) = 0; break;
          case 5: dst[4] = 0;
           /* FALLTHROUGH */
          case 4: *((uint32_t*)(dst)) = 0; break;
          default:
            for (int c = 0; c < components; c ++)
              dst[c] = 0;
            break;
        }
#if 0
      }
      else
      {
        uint8_t ralpha = 255 - cov;
        for (int c = 0; c < components; c++)
          { dst[c] = (dst[c] * ralpha) / 255; }
      }
    }
    coverage ++;
#endif
    dst += components;
  }
}

typedef enum {
  CTX_PORTER_DUFF_0,
  CTX_PORTER_DUFF_1,
  CTX_PORTER_DUFF_ALPHA,
  CTX_PORTER_DUFF_1_MINUS_ALPHA,
} CtxPorterDuffFactor;

#define  \
ctx_porter_duff_factors(mode, foo, bar)\
{\
  switch (mode)\
  {\
     case CTX_COMPOSITE_SOURCE_ATOP:\
        f_s = CTX_PORTER_DUFF_ALPHA;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_ALPHA;\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_1;\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
        f_s = CTX_PORTER_DUFF_1;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_1;\
       break;\
     case CTX_COMPOSITE_XOR:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
        f_s = CTX_PORTER_DUFF_1_MINUS_ALPHA;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
        f_s = CTX_PORTER_DUFF_ALPHA;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     case CTX_COMPOSITE_COPY:\
        f_s = CTX_PORTER_DUFF_1;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
     default:\
     case CTX_COMPOSITE_CLEAR:\
        f_s = CTX_PORTER_DUFF_0;\
        f_d = CTX_PORTER_DUFF_0;\
       break;\
  }\
}

#if 1
static void
ctx_u8_source_over_normal_color (int components,
                                 CtxRasterizer         *rasterizer,
                                 uint8_t * __restrict__ dst,
                                 uint8_t * __restrict__ src,
                                 int                    x0,
                                 uint8_t * __restrict__ coverage,
                                 int                    count)
{
  uint8_t tsrc[5];
  *((uint32_t*)tsrc) = *((uint32_t*)src);
  ctx_u8_associate_alpha (components, tsrc);

    while (count--)
    {
      int cov = *coverage;
      if (cov)
      {
        if (cov == 255)
        {
        for (int c = 0; c < components; c++)
          dst[c] = (tsrc[c]) + (dst[c] * (255-(tsrc[components-1])))/(255);
        }
        else
        {
          for (int c = 0; c < components; c++)
            dst[c] = (tsrc[c] * cov)/255 + (dst[c] * ((255*255)-(tsrc[components-1] * cov)))/(255*255);
         }
      }
      coverage ++;
      dst+=components;
    }
}
#endif

#if CTX_AVX2

#if !__COSMOPOLITAN__
#include <stdalign.h>
#endif
#endif

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_buf) (CTX_COMPOSITE_ARGUMENTS, uint8_t *tsrc)
{
    int x = 0;

#if CTX_AVX2
    if (((size_t)(dst) & 31))
#endif
    {
      for (; (x < count) 
#if CTX_AVX2
                        && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
    {
      int cov = coverage[0];
      if (cov)
      {
        uint32_t *sip = ((uint32_t*)(tsrc));
        uint32_t si = *sip;
        uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
        uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
        uint32_t *dip = ((uint32_t*)(dst));
        int      si_a  = si >> CTX_RGBA8_A_SHIFT;
        uint32_t di = *dip;
        uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
        uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
        int ir_cov_si_a = 255-((cov*si_a)/255);
        *((uint32_t*)(dst)) = 
         (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
         (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
      }
      dst += 4;
      tsrc += 4;
      coverage ++;
    }
  }

#if CTX_AVX2
    __m256i a_mask = _mm256_set1_epi32 (0xFF000000);
    __m256i lo_mask = _mm256_set1_epi32 (0x00FF00FF);
    __m256i hi_mask = _mm256_set1_epi32 (0xFF00FF00);
    __m256i x00ff =   _mm256_set1_epi16(255);
    __m256i x0101 =   _mm256_set1_epi16(0x0101);
    __m256i x0080 =   _mm256_set1_epi16(0x0080);

    int trailer = (count - x) % 8 + 1;

    for (; x <= count-trailer; x+=8)
    {
     if (((uint64_t*)(coverage))[0])
     {
       __m256i xcov;
       __m256i x1_minus_cov_mul_a;
       __m256i xsrc = _mm256_load_si256((__m256i*)(tsrc));

       __m256i xsrc_a = _mm256_and_si256(xsrc, a_mask);
       xsrc_a = _mm256_srli_epi32 (xsrc_a, 24);
       xsrc_a |= xsrc_a << 16;
       xcov  = _mm256_set_epi32(coverage[7],
                                coverage[6],
                                coverage[5],
                                coverage[4],
                                coverage[3],
                                coverage[2],
                                coverage[1],
                                coverage[0]
                                );
       xcov |= xcov << 16;
       __m256i xcov_mul_a  =
         _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(xcov,  xsrc_a), x0080), x0101);
       x1_minus_cov_mul_a = _mm256_sub_epi16(x00ff, xcov_mul_a);

       __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
       __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
       __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
       __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
       __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        
       dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
       dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);

       src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
       src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi, xcov), x0080), x0101);

       dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
       dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

       _mm256_store_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi,8)|dst_lo);
     }

      dst  += 4 * 8;
      tsrc += 4 * 8;
      coverage += 8;
    }
    for (; x < count; x++)
    {
      int cov = coverage[0];
      if (cov)
      {
        uint32_t *sip = ((uint32_t*)(tsrc));
        uint32_t si = *sip;
        uint32_t *dip = ((uint32_t*)(dst));
        uint32_t di = *dip;

        uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
        uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
        int      si_a  = si >> CTX_RGBA8_A_SHIFT;

        uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
        uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
        int ir_cov_si_a = 255-((cov*si_a)/255);
        *((uint32_t*)(dst)) = 
         (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
         (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
      }
      dst  += 4;
      tsrc += 4;
      coverage ++;
    }
#endif
}

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_fragment) (CTX_COMPOSITE_ARGUMENTS)
{
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
#endif
  CtxFragment fragment = rasterizer->fragment;
  int fudge = ((size_t)(dst) & 31);
#if CTX_AVX2
  alignas(32)
#endif
  uint8_t _tsrc[4 * (count + fudge)];
  uint8_t *tsrc = &_tsrc[fudge];
  fragment (rasterizer, u0, v0, tsrc, count, ud, vd);
#if CTX_DITHER
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
#endif
  for (int x = 0; x < count ; x++)
  {
    ctx_RGBA8_associate_alpha (tsrc);
#if CTX_DITHER
    ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif
    tsrc += 4;
#if CTX_DITHER
    u0 += ud;
    v0 += vd;
#endif
  }
  tsrc = &_tsrc[fudge];

  CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_buf) (rasterizer,
                       dst, src, x0, coverage, count, tsrc);

}


#if CTX_GRADIENTS
#if CTX_INLINED_GRADIENTS
static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_linear_gradient) (CTX_COMPOSITE_ARGUMENTS)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  float linear_gradient_dx = g->linear_gradient.dx;
  float linear_gradient_dy = g->linear_gradient.dy;
  float linear_gradient_rdelta = g->linear_gradient.rdelta;
  float linear_gradient_start = g->linear_gradient.start;
  float linear_gradient_length = g->linear_gradient.length;
#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
#endif
  int fudge = ((size_t)(dst) & 31);
#if CTX_AVX2
  alignas(32)
#endif
  uint8_t _tsrc[4 * (count + fudge)];
  uint8_t *tsrc = &_tsrc[fudge];
  for (int x = 0; x < count ; x++)
  {
      float vv = ( ( (linear_gradient_dx * u0 + linear_gradient_dy * v0) / linear_gradient_length) -
            linear_gradient_start) * (linear_gradient_rdelta);
      uint32_t *tsrci = (uint32_t*)tsrc;
#if CTX_GRADIENT_CACHE
      uint32_t *cachei = ((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
      *tsrci = *cachei;
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif
#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    tsrc += 4;
    u0 += ud;
    v0 += vd;
  }
  tsrc = &_tsrc[fudge];
  CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_buf) (rasterizer,
               dst, src, x0, coverage, count, tsrc);
}

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_radial_gradient) (CTX_COMPOSITE_ARGUMENTS)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
  float radial_gradient_x0 = g->radial_gradient.x0;
  float radial_gradient_y0 = g->radial_gradient.y0;
  float radial_gradient_r0 = g->radial_gradient.r0;
  float radial_gradient_rdelta = g->radial_gradient.rdelta;
#if CTX_DITHER
  int dither_red_blue = rasterizer->format->dither_red_blue;
  int dither_green = rasterizer->format->dither_green;
#endif
  int fudge = ((size_t)(dst) & 31);
#if CTX_AVX2
  alignas(32)
#endif
  uint8_t _tsrc[4 * (count + fudge)];
  uint8_t *tsrc = &_tsrc[fudge];
  for (int x = 0; x < count ; x++)
  {
      float vv = ctx_hypotf (radial_gradient_x0 - u0, radial_gradient_y0 - v0);
            vv = (vv - radial_gradient_r0) * (radial_gradient_rdelta);
#if CTX_GRADIENT_CACHE
      uint32_t *tsrcp = (uint32_t*)tsrc;
      uint32_t *cp = ((uint32_t*)(&ctx_gradient_cache_u8_a[ctx_grad_index(vv)][0]));
      *tsrcp = *cp;
#else
      ctx_fragment_gradient_1d_RGBA8 (rasterizer, vv, 1.0, tsrc);
      ctx_RGBA8_associate_alpha (tsrc);
#endif

#if CTX_DITHER
      ctx_dither_rgba_u8 (tsrc, u0, v0, dither_red_blue, dither_green);
#endif

    tsrc += 4;
    u0 += ud;
    v0 += vd;
  }

  CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_buf) (rasterizer,
               dst, src, x0, coverage, count, &_tsrc[fudge]);
}


#endif
#endif

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color) (CTX_COMPOSITE_ARGUMENTS)
{
#if 0
  ctx_u8_source_over_normal_color (4, rasterizer, dst, src, clip, x0, coverage, count);
  return;
#endif
  {
    uint8_t tsrc[4];
    memcpy (tsrc, src, 4);
    ctx_RGBA8_associate_alpha (tsrc);
    uint8_t a = src[3];
    int x = 0;

#if CTX_AVX2
    if ((size_t)(dst) & 31)
#endif
    {
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      if (a==255)
      {

      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
    {
      int cov = coverage[0];
      if (cov)
      {
        int r_cov = 255-cov;
        uint32_t di = *((uint32_t*)(dst));
        uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
        uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
        *((uint32_t*)(dst)) = 
         (((si_rb * cov + di_rb * r_cov) >> 8) & CTX_RGBA8_RB_MASK) |
         (((si_ga * cov + di_ga * r_cov) >> 8) & CTX_RGBA8_GA_MASK);
      }
      dst += 4;
      coverage ++;
    }
  }
    else
    {
      int si_a = si >> CTX_RGBA8_A_SHIFT;
      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
      {
        int cov = *coverage;
        if (cov)
        {
          uint32_t di = *((uint32_t*)(dst));
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
      }
    }
    }

#if CTX_AVX2
                    
    __m256i xsrc = _mm256_set1_epi32( *((uint32_t*)tsrc)) ;
    __m256i lo_mask = _mm256_set1_epi32 (0x00FF00FF);
    __m256i hi_mask = _mm256_set1_epi32 (0xFF00FF00);
    __m256i x00ff =   _mm256_set1_epi16(255);
    __m256i x0101 =   _mm256_set1_epi16(0x0101);
    __m256i x0080 =   _mm256_set1_epi16(0x0080);

    int trailer = (count - x) % 8 + 1;

    for (; x <= count-trailer; x+=8)
    {
      __m256i xcov;
      __m256i x1_minus_cov_mul_a;
     
     if (((uint64_t*)(coverage))[0])
     {
       if (CTX_LIKELY(((uint64_t*)(coverage))[0] != 0xffffffffffffffff))
       {
         xcov  = _mm256_set_epi32(coverage[7],
                                  coverage[6],
                                  coverage[5],
                                  coverage[4],
                                  coverage[3],
                                  coverage[2],
                                  coverage[1],
                                  coverage[0]
                                  );
         xcov |= xcov << 16;

        x1_minus_cov_mul_a = 
           _mm256_sub_epi16(x00ff, _mm256_mulhi_epu16 (
                   _mm256_adds_epu16 (_mm256_mullo_epi16(xcov,
                                      _mm256_set1_epi16(a)), x0080), x0101));
       }
       else
       {
          if (a == 255)
          {
            _mm256_store_si256((__m256i*)dst, xsrc);
            dst += 4 * 8;
            coverage += 8;
            continue;
          }

          xcov = x00ff;
          x1_minus_cov_mul_a = _mm256_set1_epi16(255-a);
       }
      __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
      __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
      __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
      __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
      __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        
      dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
      dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);

      src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
      src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi,  xcov), x0080), x0101);

      dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
      dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

      _mm256_store_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi,8)|dst_lo);
     }

      dst += 4 * 8;
      coverage += 8;
    }

    if (x < count)
    {
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      int si_a = si >> CTX_RGBA8_A_SHIFT;
      for (; x < count; x++)
      {
        int cov = *coverage;
        if (cov)
        {
          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst      += 4;
        coverage ++;
      }
    }
#endif
  }
}



static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color_solid) (CTX_COMPOSITE_ARGUMENTS)
{
#if 0
  ctx_u8_source_over_normal_color (4, rasterizer, dst, src, clip, x0, coverage, count);
  return;
#endif
    uint8_t *tsrc = src;
    int x = 0;

#if CTX_AVX2
    if ((size_t)(dst) & 31)
#endif
    {
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      for (; (x < count) 
#if CTX_AVX2
                      && ((size_t)(dst)&31)
#endif
                      ; 
                      x++)
    {
      int cov = coverage[0];
      if (cov)
      {
        int r_cov = 255-cov;
        uint32_t *dip = ((uint32_t*)(dst));
        uint32_t di = *dip;
        uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
        uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
        *((uint32_t*)(dst)) = 
         (((si_rb * cov + di_rb * r_cov) >> 8) & CTX_RGBA8_RB_MASK) |
         (((si_ga * cov + di_ga * r_cov) >> 8) & CTX_RGBA8_GA_MASK);
      }
      dst += 4;
      coverage ++;
    }
  }

#if CTX_AVX2
                    
    __m256i xsrc = _mm256_set1_epi32( *((uint32_t*)tsrc)) ;
    int trailer = (count - x) % 8 + 1;
    for (; x < count-trailer; x+=8)
    {
      __m256i xcov;
      __m256i x1_minus_cov_mul_a;
    __m256i lo_mask = _mm256_set1_epi32 (0x00FF00FF);
    __m256i hi_mask = _mm256_set1_epi32 (0xFF00FF00);
    __m256i x00ff =   _mm256_set1_epi16(255);
    __m256i x0101 =   _mm256_set1_epi16(0x0101);
    __m256i x0080 =   _mm256_set1_epi16(0x0080);
     uint64_t cov = ((uint64_t*)(coverage))[0];
     if (cov)
     {
       if (CTX_UNLIKELY(cov == 0xffffffffffffffff))
       {
         _mm256_store_si256((__m256i*)dst, xsrc);
       }
       else
       {
          __m256i xdst   = _mm256_load_si256((__m256i*)(dst));
          __m256i dst_lo = _mm256_and_si256 (xdst, lo_mask);
          __m256i dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8);
          __m256i src_lo = _mm256_and_si256 (xsrc, lo_mask);
          __m256i src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
            
          xcov  = _mm256_set_epi32(coverage[7],
                                   coverage[6],
                                   coverage[5],
                                   coverage[4],
                                   coverage[3],
                                   coverage[2],
                                   coverage[1],
                                   coverage[0]
                                   );
          xcov |= xcov << 16;

          x1_minus_cov_mul_a = _mm256_sub_epi16(x00ff, xcov);
    
          dst_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_hi,  x1_minus_cov_mul_a), x0080), x0101);
          dst_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(dst_lo,  x1_minus_cov_mul_a), x0080), x0101);
    
          src_lo  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_lo, xcov), x0080), x0101);
          src_hi  = _mm256_mulhi_epu16(_mm256_adds_epu16(_mm256_mullo_epi16(src_hi,  xcov), x0080), x0101);
    
          dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
          dst_lo = _mm256_adds_epu16(dst_lo, src_lo);
    
          _mm256_store_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi,8)|dst_lo);
        }
      }

      dst += 4 * 8;
      coverage += 8;
    }

    if (x < count)
    {
      uint32_t *sip = ((uint32_t*)(tsrc));
      uint32_t si = *sip;
      uint64_t si_ga = si & CTX_RGBA8_GA_MASK;
      uint32_t si_rb = si & CTX_RGBA8_RB_MASK;
      int si_a = si >> CTX_RGBA8_A_SHIFT;
      for (; x < count; x++)
      {
        int cov = *coverage;
        if (cov)
        {
          uint32_t *dip = ((uint32_t*)(dst));
          uint32_t di = *dip;
          uint64_t di_ga = di & CTX_RGBA8_GA_MASK;
          uint32_t di_rb = di & CTX_RGBA8_RB_MASK;
          int ir_cov_si_a = 255-((cov*si_a)>>8);
          *((uint32_t*)(dst)) = 
           (((si_rb * cov + di_rb * ir_cov_si_a) >> 8) & CTX_RGBA8_RB_MASK) |
           (((si_ga * cov + di_ga * ir_cov_si_a) >> 8) & CTX_RGBA8_GA_MASK);
        }
        dst += 4;
        coverage ++;
      }
    }
#endif
}

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_copy_normal) (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_copy_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_RGBA8_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_clear_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_u8_blend_normal (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)
{
  switch (components)
  {
     case 3:
       ((uint8_t*)(blended))[2] = ((uint8_t*)(src))[2];
      /* FALLTHROUGH */
     case 2:
       *((uint16_t*)(blended)) = *((uint16_t*)(src));
       ctx_u8_associate_alpha (components, blended);
       break;
     case 5:
       ((uint8_t*)(blended))[4] = ((uint8_t*)(src))[4];
       /* FALLTHROUGH */
     case 4:
       *((uint32_t*)(blended)) = *((uint32_t*)(src));
       ctx_u8_associate_alpha (components, blended);
       break;
     default:
       {
         uint8_t alpha = src[components-1];
         for (int i = 0; i<components - 1;i++)
           blended[i] = (src[i] * alpha)/255;
         blended[components-1]=alpha;
       }
       break;
  }
}

/* branchless 8bit add that maxes out at 255 */
static inline uint8_t ctx_sadd8(uint8_t a, uint8_t b)
{
  uint16_t s = (uint16_t)a+b;
  return -(s>>8) | (uint8_t)s;
}

#if CTX_BLENDING_AND_COMPOSITING

#define ctx_u8_blend_define(name, CODE) \
static void \
ctx_u8_blend_##name (int components, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)\
{\
  uint8_t *s=src; uint8_t b[components];\
  ctx_u8_deassociate_alpha (components, dst, b);\
    CODE;\
  blended[components-1] = src[components-1];\
  ctx_u8_associate_alpha (components, blended);\
}

#define ctx_u8_blend_define_seperable(name, CODE) \
        ctx_u8_blend_define(name, for (int c = 0; c < components-1; c++) { CODE ;}) \

ctx_u8_blend_define_seperable(multiply,     blended[c] = (b[c] * s[c])/255;)
ctx_u8_blend_define_seperable(screen,       blended[c] = s[c] + b[c] - (s[c] * b[c])/255;)
ctx_u8_blend_define_seperable(overlay,      blended[c] = b[c] < 127 ? (s[c] * b[c])/255 :
                                                         s[c] + b[c] - (s[c] * b[c])/255;)
ctx_u8_blend_define_seperable(darken,       blended[c] = ctx_mini (b[c], s[c]))
ctx_u8_blend_define_seperable(lighten,      blended[c] = ctx_maxi (b[c], s[c]))
ctx_u8_blend_define_seperable(color_dodge,  blended[c] = b[c] == 0 ? 0 :
                                     s[c] == 255 ? 255 : ctx_mini(255, (255 * b[c]) / (255-s[c])))
ctx_u8_blend_define_seperable(color_burn,   blended[c] = b[c] == 1 ? 1 :
                                     s[c] == 0 ? 0 : 255 - ctx_mini(255, (255*(255 - b[c])) / s[c]))
ctx_u8_blend_define_seperable(hard_light,   blended[c] = s[c] < 127 ? (b[c] * s[c])/255 :
                                                          b[c] + s[c] - (b[c] * s[c])/255;)
ctx_u8_blend_define_seperable(difference,   blended[c] = (b[c] - s[c]))
ctx_u8_blend_define_seperable(divide,       blended[c] = s[c]?(255 * b[c]) / s[c]:0)
ctx_u8_blend_define_seperable(addition,     blended[c] = ctx_sadd8 (s[c], b[c]))
ctx_u8_blend_define_seperable(subtract,     blended[c] = ctx_maxi(0, s[c]-b[c]))
ctx_u8_blend_define_seperable(exclusion,    blended[c] = b[c] + s[c] - 2 * (b[c] * s[c]/255))
ctx_u8_blend_define_seperable(soft_light,
  if (s[c] <= 255/2)
  {
    blended[c] = b[c] - (255 - 2 * s[c]) * b[c] * (255 - b[c]) / (255 * 255);
  }
  else
  {
    int d;
    if (b[c] <= 255/4)
      d = (((16 * b[c] - 12 * 255)/255 * b[c] + 4 * 255) * b[c])/255;
    else
      d = ctx_sqrtf(b[c]/255.0) * 255.4;
    blended[c] = (b[c] + (2 * s[c] - 255) * (d - b[c]))/255;
  }
)

static int ctx_int_get_max (int components, int *c)
{
  int max = 0;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] > max) max = c[i];
  }
  return max;
}

static int ctx_int_get_min (int components, int *c)
{
  int min = 400;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] < min) min = c[i];
  }
  return min;
}

static int ctx_int_get_lum (int components, int *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         int sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
            break;
  }
}

static int ctx_u8_get_lum (int components, uint8_t *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         int sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
            break;
  }
}
static int ctx_u8_get_sat (int components, uint8_t *c)
{
  switch (components)
  {
    case 3:
    case 4:
            { int r = c[0];
              int g = c[1];
              int b = c[2];
              return ctx_maxi(r, ctx_maxi(g,b)) - ctx_mini(r,ctx_mini(g,b));
            }
            break;
    case 1:
    case 2:
            return 0.0;
            break;
    default:
       {
         int min = 1000;
         int max = -1000;
         for (int i = 0; i < components - 1; i ++)
         {
           if (c[i] < min) min = c[i];
           if (c[i] > max) max = c[i];
         }
         return max-min;
       }
       break;
  }
}

static void ctx_u8_set_lum (int components, uint8_t *c, uint8_t lum)
{
  int d = lum - ctx_u8_get_lum (components, c);
  int tc[components];
  for (int i = 0; i < components - 1; i++)
  {
    tc[i] = c[i] + d;
  }

  int l = ctx_int_get_lum (components, tc);
  int n = ctx_int_get_min (components, tc);
  int x = ctx_int_get_max (components, tc);

  if (n < 0 && l!=n)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 255 && x!=l)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * (255 - l)) / (x-l));
  }
  for (int i = 0; i < components - 1; i++)
    c[i] = tc[i];
}

static void ctx_u8_set_sat (int components, uint8_t *c, uint8_t sat)
{
  int max = 0, mid = 1, min = 2;
  
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}
  if (c[mid] > c[max]){int t = mid; mid = max; max = t;}
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}

  if (c[max] > c[min])
  {
    c[mid] = ((c[mid]-c[min]) * sat) / (c[max] - c[min]);
    c[max] = sat;
  }
  else
  {
    c[mid] = c[max] = 0;
  }
  c[min] = 0;
}

ctx_u8_blend_define(color,
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_u8_set_lum(components, blended, ctx_u8_get_lum (components, s));
)

ctx_u8_blend_define(hue,
  int in_sat = ctx_u8_get_sat(components, b);
  int in_lum = ctx_u8_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_u8_set_sat(components, blended, in_sat);
  ctx_u8_set_lum(components, blended, in_lum);
)

ctx_u8_blend_define(saturation,
  int in_sat = ctx_u8_get_sat(components, s);
  int in_lum = ctx_u8_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_u8_set_sat(components, blended, in_sat);
  ctx_u8_set_lum(components, blended, in_lum);
)

ctx_u8_blend_define(luminosity,
  int in_lum = ctx_u8_get_lum(components, s);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_u8_set_lum(components, blended, in_lum);
)
#endif

CTX_INLINE static void
ctx_u8_blend (int components, CtxBlend blend, uint8_t * __restrict__ dst, uint8_t *src, uint8_t *blended)
{
#if CTX_BLENDING_AND_COMPOSITING
  switch (blend)
  {
    case CTX_BLEND_NORMAL:      ctx_u8_blend_normal      (components, dst, src, blended); break;
    case CTX_BLEND_MULTIPLY:    ctx_u8_blend_multiply    (components, dst, src, blended); break;
    case CTX_BLEND_SCREEN:      ctx_u8_blend_screen      (components, dst, src, blended); break;
    case CTX_BLEND_OVERLAY:     ctx_u8_blend_overlay     (components, dst, src, blended); break;
    case CTX_BLEND_DARKEN:      ctx_u8_blend_darken      (components, dst, src, blended); break;
    case CTX_BLEND_LIGHTEN:     ctx_u8_blend_lighten     (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_DODGE: ctx_u8_blend_color_dodge (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_BURN:  ctx_u8_blend_color_burn  (components, dst, src, blended); break;
    case CTX_BLEND_HARD_LIGHT:  ctx_u8_blend_hard_light  (components, dst, src, blended); break;
    case CTX_BLEND_SOFT_LIGHT:  ctx_u8_blend_soft_light  (components, dst, src, blended); break;
    case CTX_BLEND_DIFFERENCE:  ctx_u8_blend_difference  (components, dst, src, blended); break;
    case CTX_BLEND_EXCLUSION:   ctx_u8_blend_exclusion   (components, dst, src, blended); break;
    case CTX_BLEND_COLOR:       ctx_u8_blend_color       (components, dst, src, blended); break;
    case CTX_BLEND_HUE:         ctx_u8_blend_hue         (components, dst, src, blended); break;
    case CTX_BLEND_SATURATION:  ctx_u8_blend_saturation  (components, dst, src, blended); break;
    case CTX_BLEND_LUMINOSITY:  ctx_u8_blend_luminosity  (components, dst, src, blended); break;
    case CTX_BLEND_ADDITION:    ctx_u8_blend_addition    (components, dst, src, blended); break;
    case CTX_BLEND_DIVIDE:      ctx_u8_blend_divide      (components, dst, src, blended); break;
    case CTX_BLEND_SUBTRACT:    ctx_u8_blend_subtract    (components, dst, src, blended); break;
  }
#else
  switch (blend)
  {
    default:                    ctx_u8_blend_normal      (components, dst, src, blended); break;
  }

#endif
}

CTX_INLINE static void
__ctx_u8_porter_duff (CtxRasterizer         *rasterizer,
                     int                    components,
                     uint8_t * __restrict__ dst,
                     uint8_t * __restrict__ src,
                     int                    x0,
                     uint8_t * __restrict__ coverage,
                     int                    count,
                     CtxCompositingMode     compositing_mode,
                     CtxFragment            fragment,
                     CtxBlend               blend)
{
  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;

  {
    uint8_t tsrc[components];
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;
    if (CTX_UNLIKELY(fragment))
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

    while (count--)
    {
      int cov = *coverage;

      if (CTX_UNLIKELY(
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 255)||
        (compositing_mode == CTX_COMPOSITE_SOURCE_OVER      && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_XOR              && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_DESTINATION_OUT  && cov == 0) ||
        (compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      && cov == 0)
        ))
      {
        u0 += ud;
        v0 += vd;
        coverage ++;
        dst+=components;
        continue;
      }

      if (fragment)
      {
        fragment (rasterizer, u0, v0, tsrc, 1, ud, vd);
        if (blend != CTX_BLEND_NORMAL)
          ctx_u8_blend (components, blend, dst, tsrc, tsrc);
      }
      else
      {
        ctx_u8_blend (components, blend, dst, src, tsrc);
      }

      u0 += ud;
      v0 += vd;
      if (global_alpha_u8 != 255)
        cov = (cov * global_alpha_u8)/255;

      if (cov != 255)
        for (int c = 0; c < components; c++)
          tsrc[c] = (tsrc[c] * cov)/255;

      for (int c = 0; c < components; c++)
      {
        int res = 0;
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res += (tsrc[c] * dst[components-1])/255; break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (tsrc[c] * (255-dst[components-1]))/255; break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: break;
          case CTX_PORTER_DUFF_1:             res += dst[c]; break;
          case CTX_PORTER_DUFF_ALPHA:         res += (dst[c] * tsrc[components-1])/255; break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res += (dst[c] * (255-tsrc[components-1]))/255; break;
        }
        dst[c] = res;
      }
      coverage ++;
      dst+=components;
    }
  }
}

#if CTX_AVX2
CTX_INLINE static void
ctx_avx2_porter_duff (CtxRasterizer         *rasterizer,
                      int                    components,
                      uint8_t * dst,
                      uint8_t * src,
                      int                    x0,
                      uint8_t * coverage,
                      int                    count,
                      CtxCompositingMode     compositing_mode,
                      CtxFragment            fragment,
                      CtxBlend               blend)
{
  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
//assert ((((size_t)dst) & 31) == 0);
  int n_pix = 32/components;
  uint8_t tsrc[components * n_pix];
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;
  int x = 0;
  if (fragment)
    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

  __m256i lo_mask = _mm256_set1_epi32 (0x00FF00FF);
  __m256i hi_mask = _mm256_set1_epi32 (0xFF00FF00);
  __m256i x00ff =   _mm256_set1_epi16(255);
  __m256i x0101 =   _mm256_set1_epi16(0x0101);
  __m256i x0080 =   _mm256_set1_epi16(0x0080);
  for (; x < count; x+=n_pix)
  {
    __m256i xdst  = _mm256_loadu_si256((__m256i*)(dst)); 
    __m256i xcov;
    __m256i xsrc;
    __m256i xsrc_a;
    __m256i xdst_a;

    int is_blank = 1;
    int is_full = 0;
    switch (n_pix)
    {
      case 16:
        if (((uint64_t*)(coverage))[0] &&
            ((uint64_t*)(coverage))[1])
           is_blank = 0;
        else if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff &&
                 ((uint64_t*)(coverage))[1] == 0xffffffffffffffff)
           is_full = 1;
        break;
      case 8:
        if (((uint64_t*)(coverage))[0])
           is_blank = 0;
        else if (((uint64_t*)(coverage))[0] == 0xffffffffffffffff)
           is_full = 1;
        break;
      case 4:
        if (((uint32_t*)(coverage))[0])
           is_blank = 0;
        else if (((uint32_t*)(coverage))[0] == 0xffffffff)
           is_full = 1;
        break;
      default:
        break;
    }

#if 1
    if (
      //(compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 255)||
      (compositing_mode == CTX_COMPOSITE_SOURCE_OVER      && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_XOR              && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_DESTINATION_OUT  && is_blank) ||
      (compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      && is_blank)
      )
    {
      u0 += ud * n_pix;
      v0 += vd * n_pix;
      coverage += n_pix;
      dst+=32;
      continue;
    }
#endif

    if (fragment)
    {
      for (int i = 0; i < n_pix; i++)
      {
         fragment (rasterizer, u0, v0, &tsrc[i*components], 1, ud, vd);
         ctx_u8_associate_alpha (components, &tsrc[i*components]);
         ctx_u8_blend (components, blend,
                       &dst[i*components],
                       &tsrc[i*components],
                       &tsrc[i*components]);
         u0 += ud;
         v0 += vd;
      }
      xsrc = _mm256_loadu_si256((__m256i*)tsrc);
    }
    else
    {
#if 0
      if (blend == CTX_BLEND_NORMAL && components == 4)
        xsrc = _mm256_set1_epi32 (*((uint32_t*)src));
    else
#endif
      {
 //     for (int i = 0; i < n_pix; i++)
 //       for (int c = 0; c < components; c++)
 //         tsrc[i*components+c]=src[c];
#if 1
        uint8_t lsrc[components];
        for (int i = 0; i < components; i ++)
          lsrc[i] = src[i];
  //    ctx_u8_associate_alpha (components, lsrc);
        for (int i = 0; i < n_pix; i++)
          ctx_u8_blend (components, blend,
                        &dst[i*components],
                        lsrc,
                        &tsrc[i*components]);
#endif
        xsrc = _mm256_loadu_si256((__m256i*)tsrc);
      }
    }

    if (is_full)
       xcov = _mm256_set1_epi16(255);
    else
    switch (n_pix)
    {
      case 4: xcov  = _mm256_set_epi16(
               (coverage[3]), (coverage[3]), coverage[3], coverage[3],
               (coverage[2]), (coverage[2]), coverage[2], coverage[2],
               (coverage[1]), (coverage[1]), coverage[1], coverage[1],
               (coverage[0]), (coverage[0]), coverage[0], coverage[0]);
              break;
      case 8: 
          xcov  = _mm256_set_epi32(coverage[7],
                                   coverage[6],
                                   coverage[5],
                                   coverage[4],
                                   coverage[3],
                                   coverage[2],
                                   coverage[1],
                                   coverage[0]
                                   );
          xcov |= xcov << 16;
              break;
      case 16: xcov  = _mm256_set_epi16(
               (coverage[15]),
               (coverage[14]),
               (coverage[13]),
               (coverage[12]),
               (coverage[11]),
               (coverage[10]),
               (coverage[9]),
               (coverage[8]),
               (coverage[7]),
               (coverage[6]),
               (coverage[5]),
               (coverage[4]),
               (coverage[3]),
               (coverage[2]),
               (coverage[1]),
               (coverage[0]));
              break;
    }
#if 0
    switch (n_pix)
    {
      case 4:
      xsrc_a = _mm256_set_epi16(
            tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)], tsrc[0*components+(components-1)],tsrc[0*components+(components-1)], tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[3*components+(components-1)], dst[3*components+(components-1)],dst[3*components+(components-1)], dst[3*components+(components-1)],
            dst[2*components+(components-1)], dst[2*components+(components-1)],dst[2*components+(components-1)], dst[2*components+(components-1)],
            dst[1*components+(components-1)], dst[1*components+(components-1)],dst[1*components+(components-1)], dst[1*components+(components-1)],
            dst[0*components+(components-1)], dst[0*components+(components-1)],dst[0*components+(components-1)], dst[0*components+(components-1)]);

              break;
      case 8:
      xsrc_a = _mm256_set_epi16(
            tsrc[7*components+(components-1)], tsrc[7*components+(components-1)],
            tsrc[6*components+(components-1)], tsrc[6*components+(components-1)],
            tsrc[5*components+(components-1)], tsrc[5*components+(components-1)],
            tsrc[4*components+(components-1)], tsrc[4*components+(components-1)],
            tsrc[3*components+(components-1)], tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)], tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)], tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)], tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[7*components+(components-1)], dst[7*components+(components-1)],
            dst[6*components+(components-1)], dst[6*components+(components-1)],
            dst[5*components+(components-1)], dst[5*components+(components-1)],
            dst[4*components+(components-1)], dst[4*components+(components-1)],
            dst[3*components+(components-1)], dst[3*components+(components-1)],
            dst[2*components+(components-1)], dst[2*components+(components-1)],
            dst[1*components+(components-1)], dst[1*components+(components-1)],
            dst[0*components+(components-1)], dst[0*components+(components-1)]);
              break;
      case 16: 
      xsrc_a = _mm256_set_epi16(
            tsrc[15*components+(components-1)],
            tsrc[14*components+(components-1)],
            tsrc[13*components+(components-1)],
            tsrc[12*components+(components-1)],
            tsrc[11*components+(components-1)],
            tsrc[10*components+(components-1)],
            tsrc[9*components+(components-1)],
            tsrc[8*components+(components-1)],
            tsrc[7*components+(components-1)],
            tsrc[6*components+(components-1)],
            tsrc[5*components+(components-1)],
            tsrc[4*components+(components-1)],
            tsrc[3*components+(components-1)],
            tsrc[2*components+(components-1)],
            tsrc[1*components+(components-1)],
            tsrc[0*components+(components-1)]);
      xdst_a = _mm256_set_epi16(
            dst[15*components+(components-1)],
            dst[14*components+(components-1)],
            dst[13*components+(components-1)],
            dst[12*components+(components-1)],
            dst[11*components+(components-1)],
            dst[10*components+(components-1)],
            dst[9*components+(components-1)],
            dst[8*components+(components-1)],
            dst[7*components+(components-1)],
            dst[6*components+(components-1)],
            dst[5*components+(components-1)],
            dst[4*components+(components-1)],
            dst[3*components+(components-1)],
            dst[2*components+(components-1)],
            dst[1*components+(components-1)],
            dst[0*components+(components-1)]);
              break;
    }
#endif

    if (global_alpha_u8 != 255)
    {
      xcov = _mm256_mulhi_epu16(
              _mm256_adds_epu16(
                 _mm256_mullo_epi16(xcov,
                                    _mm256_set1_epi16(global_alpha_u8)),
                 x0080), x0101);
      is_full = 0;
    }


    xsrc_a = _mm256_srli_epi32(xsrc, 24);  // XX 24 is RGB specific
    if (!is_full)
    xsrc_a = _mm256_mulhi_epu16(
              _mm256_adds_epu16(
                 _mm256_mullo_epi16(xsrc_a, xcov),
                 x0080), x0101);
    xsrc_a = xsrc_a | _mm256_slli_epi32(xsrc, 16);

    xdst_a = _mm256_srli_epi32(xdst, 24);
    xdst_a = xdst_a |  _mm256_slli_epi32(xdst, 16);


 //  case CTX_COMPOSITE_SOURCE_OVER:
 //     f_s = CTX_PORTER_DUFF_1;
 //     f_d = CTX_PORTER_DUFF_1_MINUS_ALPHA;


    __m256i dst_lo;
    __m256i dst_hi; 
    __m256i src_lo; 
    __m256i src_hi;

    switch (f_s)
    {
      case CTX_PORTER_DUFF_0:
        src_lo = _mm256_set1_epi32(0);
        src_hi = _mm256_set1_epi32(0);
        break;
      case CTX_PORTER_DUFF_1:
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);

        //if (!is_full)
        {
          src_lo = _mm256_mulhi_epu16(
                   _mm256_adds_epu16(
                   _mm256_mullo_epi16(src_lo, xcov),
                   x0080), x0101);
          src_hi = _mm256_mulhi_epu16(
                   _mm256_adds_epu16(
                   _mm256_mullo_epi16(src_hi, xcov),
                   x0080), x0101);
        }
        break;
      case CTX_PORTER_DUFF_ALPHA:
        // res += (tsrc[c] *      dst[components-1])/255;
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
        if (!is_full)
        {
           src_lo = _mm256_mulhi_epu16(
                 _mm256_adds_epu16(
                 _mm256_mullo_epi16(src_lo, xcov),
                 x0080), x0101);
           src_hi = _mm256_mulhi_epu16(
                    _mm256_adds_epu16(
                    _mm256_mullo_epi16(src_hi, xcov),
                    x0080), x0101);
        }
        src_lo = _mm256_mulhi_epu16 (
                      _mm256_adds_epu16 (_mm256_mullo_epi16(src_lo,
                                         xdst_a), x0080), x0101);
        src_hi = _mm256_mulhi_epu16 (
                      _mm256_adds_epu16 (_mm256_mullo_epi16(src_hi,
                                         xdst_a), x0080), x0101);
        break;
      case CTX_PORTER_DUFF_1_MINUS_ALPHA:
        // res += (tsrc[c] * (255-dst[components-1]))/255;
        src_lo = _mm256_and_si256 (xsrc, lo_mask); 
        src_hi = _mm256_srli_epi16 (_mm256_and_si256 (xsrc, hi_mask), 8);
  //    if (!is_full)
        {
          src_lo = _mm256_mulhi_epu16(
                        _mm256_adds_epu16(
                        _mm256_mullo_epi16(src_lo, xcov),
                        x0080), x0101);
          src_hi = _mm256_mulhi_epu16(
                        _mm256_adds_epu16(
                        _mm256_mullo_epi16(src_hi, xcov),
                        x0080), x0101);
        }
        src_lo = _mm256_mulhi_epu16 (
                  _mm256_adds_epu16 (_mm256_mullo_epi16(src_lo,
                                     _mm256_sub_epi16(x00ff,xdst_a)), x0080),
                  x0101);
        src_hi = _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(src_hi,
                                    _mm256_sub_epi16(x00ff,xdst_a)), x0080),
                 x0101);
        break;
    }
    switch (f_d)
    {
      case CTX_PORTER_DUFF_0: 
        dst_lo = _mm256_set1_epi32(0);
        dst_hi = _mm256_set1_epi32(0);
        break;
      case CTX_PORTER_DUFF_1:
        dst_lo = _mm256_and_si256 (xdst, lo_mask); 
        dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 
        break;
      case CTX_PORTER_DUFF_ALPHA:        
          //res += (dst[c] * tsrc[components-1])/255;
          dst_lo = _mm256_and_si256 (xdst, lo_mask); 
          dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 

          dst_lo =
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_lo,
                                    xsrc_a), x0080), x0101);
          dst_hi =
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_hi,
                                    xsrc_a), x0080), x0101);
          break;
      case CTX_PORTER_DUFF_1_MINUS_ALPHA:
          dst_lo = _mm256_and_si256 (xdst, lo_mask); 
          dst_hi = _mm256_srli_epi16 (_mm256_and_si256 (xdst, hi_mask), 8); 
          dst_lo = 
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_lo,
                                    _mm256_sub_epi16(x00ff,xsrc_a)), x0080),
                 x0101);
          dst_hi = 
             _mm256_mulhi_epu16 (
                 _mm256_adds_epu16 (_mm256_mullo_epi16(dst_hi,
                                    _mm256_sub_epi16(x00ff,xsrc_a)), x0080),
                 x0101);
          break;
    }

    dst_hi = _mm256_adds_epu16(dst_hi, src_hi);
    dst_lo = _mm256_adds_epu16(dst_lo, src_lo);

#if 0 // to toggle source vs dst
      src_hi = _mm256_slli_epi16 (src_hi, 8);
      _mm256_storeu_si256((__m256i*)dst, _mm256_blendv_epi8(src_lo, src_hi, hi_mask));
#else
      _mm256_storeu_si256((__m256i*)dst, _mm256_slli_epi16 (dst_hi, 8) | dst_lo);
#endif

    coverage += n_pix;
    dst      += 32;
  }
}
#endif

CTX_INLINE static void
_ctx_u8_porter_duff (CtxRasterizer         *rasterizer,
                     int                    components,
                     uint8_t *              dst,
                     uint8_t * __restrict__ src,
                     int                    x0,
                     uint8_t *              coverage,
                     int                    count,
                     CtxCompositingMode     compositing_mode,
                     CtxFragment            fragment,
                     CtxBlend               blend)
{
#if NOT_USABLE_CTX_AVX2
  int pre_count = 0;
  if ((size_t)(dst)&31)
  {
    pre_count = (32-(((size_t)(dst))&31))/components;
  __ctx_u8_porter_duff (rasterizer, components,
     dst, src, x0, coverage, pre_count, compositing_mode, fragment, blend);
    dst += components * pre_count;
    x0 += pre_count;
    coverage += pre_count;
    count -= pre_count;
  }
  if (count < 0)
     return;
  int post_count = (count & 31);
  if (src && 0)
  {
    src[0]/=2;
    src[1]/=2;
    src[2]/=2;
    src[3]/=2;
  }
#if 0
  __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count-post_count, compositing_mode, fragment, blend);
#else
  ctx_avx2_porter_duff (rasterizer, components, dst, src, x0, coverage, count-post_count, compositing_mode, fragment, blend);
#endif
  if (src && 0)
  {
    src[0]*=2;
    src[1]*=2;
    src[2]*=2;
    src[3]*=2;
  }
  if (post_count > 0)
  {
       x0 += (count - post_count);
       dst += components * (count-post_count);
       coverage += (count - post_count);
       __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, post_count, compositing_mode, fragment, blend);
  }
#else
  __ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, compositing_mode, fragment, blend);
#endif
}

#define _ctx_u8_porter_duffs(comp_format, components, source, fragment, blend) \
   switch (rasterizer->state->gstate.compositing_mode) \
   { \
     case CTX_COMPOSITE_SOURCE_ATOP: \
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count, \
        CTX_COMPOSITE_SOURCE_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_IN, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_XOR:\
      _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_XOR, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_IN, fragment, blend);\
       break;\
     case CTX_COMPOSITE_COPY:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_COPY, fragment, blend);\
       break;\
     case CTX_COMPOSITE_CLEAR:\
       _ctx_u8_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_CLEAR, fragment, blend);\
       break;\
   }

/* generating one function per compositing_mode would be slightly more efficient,
 * but on embedded targets leads to slightly more code bloat,
 * here we trade off a slight amount of performance
 */
#define ctx_u8_porter_duff(comp_format, components, source, fragment, blend) \
static void \
CTX_COMPOSITE_SUFFIX(ctx_##comp_format##_porter_duff_##source) (CTX_COMPOSITE_ARGUMENTS) \
{ \
  _ctx_u8_porter_duffs(comp_format, components, source, fragment, blend);\
}

ctx_u8_porter_duff(RGBA8, 4,color,   NULL,                 rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(RGBA8, 4,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

//ctx_u8_porter_duff(comp_name, components,color_##blend_name,  NULL, blend_mode)

static void
CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop) (CTX_COMPOSITE_ARGUMENTS)
{
}


static void
ctx_setup_RGBA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBA8 (rasterizer);
  rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_generic);

#if 1
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
  {
    rasterizer->comp_op = ctx_RGBA8_clear_normal;
    return;
  }
#endif


#if CTX_INLINED_GRADIENTS
#if CTX_GRADIENTS
  if (gstate->source_fill.type == CTX_SOURCE_LINEAR_GRADIENT &&
      gstate->blend_mode == CTX_BLEND_NORMAL &&
      gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
  {
     rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_linear_gradient);
     return;
  }
  if (gstate->source_fill.type == CTX_SOURCE_RADIAL_GRADIENT &&
      gstate->blend_mode == CTX_BLEND_NORMAL &&
      gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
  {
     rasterizer->comp_op = CTX_COMPOSITE_SUFFIX (ctx_RGBA8_source_over_normal_radial_gradient);
     return;
  }
#endif
#endif

  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      ctx_color_get_rgba8 (rasterizer->state, &gstate->source_fill.color, rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        rasterizer->color[components-1] = (rasterizer->color[components-1] * gstate->global_alpha_u8)/255;
      if (rasterizer->swap_red_green)
      {
        uint8_t *rgba = (uint8_t*)&rasterizer->color[0];
        uint8_t tmp = rgba[0];
        rgba[0] = rgba[2];
        rgba[2] = tmp;
      }

      switch (gstate->blend_mode)
      {
        case CTX_BLEND_NORMAL:
          if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
          {
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_copy_normal);
            return;
          }
          else if (gstate->global_alpha_u8 == 0)
          {
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
          }
          else if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
          {
             if (rasterizer->color[components-1] == 0)
                 rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
             else if (rasterizer->color[components-1] == 255)
                 rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color_solid);
             else
                 rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_color);
         }
         break;
      default:
         rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_porter_duff_color);
         break;
    }
    //rasterizer->comp_op = ctx_RGBA8_porter_duff_color; // XXX overide to make all go
                                                       // through generic code path
    rasterizer->fragment = NULL;
    return;
  }


#if 1
  if (gstate->blend_mode == CTX_BLEND_NORMAL &&
      gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER &&
      rasterizer->fragment)
  {
          // only really valid for image sources
     rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_source_over_normal_fragment);
     return;
  }
#endif
}

/*
 * we could use this instead of NULL in the pixfmt table - but such dispatch
 * is slightly slower
 */
inline static void
ctx_composite_direct (CTX_COMPOSITE_ARGUMENTS)
{
  rasterizer->comp_op (rasterizer, dst, rasterizer->color, x0, coverage, count);
}

static void
ctx_composite_convert (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * rasterizer->format->ebpp];
  rasterizer->format->to_comp (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  rasterizer->format->from_comp (rasterizer, x0, &pixels[0], dst, count);
}

#if CTX_ENABLE_FLOAT
static void
ctx_float_copy_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;
  float u0 = 0; float v0 = 0;
  float ud = 0; float vd = 0;

  if (rasterizer->fragment)
    {
      ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);
    }

  while (count--)
  {
    int cov = *coverage;
    if (cov == 0)
    {
      for (int c = 0; c < components; c++)
        { dst[c] = 0; }
    }
    else
    {
      if (rasterizer->fragment)
      {
        rasterizer->fragment (rasterizer, u0, v0, src, 1, ud, vd);
        u0+=ud;
        v0+=vd;
      }
    if (cov == 255)
    {
      for (int c = 0; c < components; c++)
        dstf[c] = srcf[c];
    }
    else
    {
      float covf = ctx_u8_to_float (cov);
      for (int c = 0; c < components; c++)
        dstf[c] = srcf[c]*covf;
    }
    }
    dstf += components;
    coverage ++;
  }
}

static void
ctx_float_clear_normal (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  while (count--)
  {
#if 0
    int cov = *coverage;
    if (cov == 0)
    {
    }
    else if (cov == 255)
    {
#endif
      switch (components)
      {
        case 2:
          ((uint64_t*)(dst))[0] = 0;
          break;
        case 4:
          ((uint64_t*)(dst))[0] = 0;
          ((uint64_t*)(dst))[1] = 0;
          break;
        default:
          for (int c = 0; c < components; c++)
            dstf[c] = 0.0f;
      }
#if 0
    }
    else
    {
      float ralpha = 1.0 - ctx_u8_to_float (cov);
      for (int c = 0; c < components; c++)
        { dstf[c] = (dstf[c] * ralpha); }
    }
    coverage ++;
#endif
    dstf += components;
  }
}

static void
ctx_float_source_over_normal_opaque_color (int components, CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  while (count--)
  {
    int cov = *coverage;
    if (cov)
    {
      if (cov == 255)
      {
        for (int c = 0; c < components; c++)
          dstf[c] = srcf[c];
      }
      else
      {
        float fcov = ctx_u8_to_float (cov);
        float ralpha = 1.0f - fcov;
        for (int c = 0; c < components-1; c++)
          dstf[c] = (srcf[c]*fcov + dstf[c] * ralpha);
      }
    }
    coverage ++;
    dstf+= components;
  }
}

inline static void
ctx_float_blend_normal (int components, float *dst, float *src, float *blended)
{
  float a = src[components-1];
  for (int c = 0; c <  components - 1; c++)
    blended[c] = src[c] * a;
  blended[components-1]=a;
}

static float ctx_float_get_max (int components, float *c)
{
  float max = -1000.0f;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] > max) max = c[i];
  }
  return max;
}

static float ctx_float_get_min (int components, float *c)
{
  float min = 400.0;
  for (int i = 0; i < components - 1; i ++)
  {
    if (c[i] < min) min = c[i];
  }
  return min;
}

static float ctx_float_get_lum (int components, float *c)
{
  switch (components)
  {
    case 3:
    case 4:
            return CTX_CSS_RGB_TO_LUMINANCE(c);
    case 1:
    case 2:
            return c[0];
            break;
    default:
       {
         float sum = 0;
         for (int i = 0; i < components - 1; i ++)
         {
           sum += c[i];
         }
         return sum / (components - 1);
       }
  }
}

static float ctx_float_get_sat (int components, float *c)
{
  switch (components)
  {
    case 3:
    case 4:
            { float r = c[0];
              float g = c[1];
              float b = c[2];
              return ctx_maxf(r, ctx_maxf(g,b)) - ctx_minf(r,ctx_minf(g,b));
            }
            break;
    case 1:
    case 2: return 0.0;
            break;
    default:
       {
         float min = 1000;
         float max = -1000;
         for (int i = 0; i < components - 1; i ++)
         {
           if (c[i] < min) min = c[i];
           if (c[i] > max) max = c[i];
         }
         return max-min;
       }
  }
}

static void ctx_float_set_lum (int components, float *c, float lum)
{
  float d = lum - ctx_float_get_lum (components, c);
  float tc[components];
  for (int i = 0; i < components - 1; i++)
  {
    tc[i] = c[i] + d;
  }

  float l = ctx_float_get_lum (components, tc);
  float n = ctx_float_get_min (components, tc);
  float x = ctx_float_get_max (components, tc);

  if (n < 0.0f && l != n)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * l) / (l-n));
  }

  if (x > 1.0f && x != l)
  {
    for (int i = 0; i < components - 1; i++)
      tc[i] = l + (((tc[i] - l) * (1.0f - l)) / (x-l));
  }
  for (int i = 0; i < components - 1; i++)
    c[i] = tc[i];
}

static void ctx_float_set_sat (int components, float *c, float sat)
{
  int max = 0, mid = 1, min = 2;
  
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}
  if (c[mid] > c[max]){int t = mid; mid = max; max = t;}
  if (c[min] > c[mid]){int t = min; min = mid; mid = t;}

  if (c[max] > c[min])
  {
    c[mid] = ((c[mid]-c[min]) * sat) / (c[max] - c[min]);
    c[max] = sat;
  }
  else
  {
    c[mid] = c[max] = 0.0f;
  }
  c[min] = 0.0f;

}

#define ctx_float_blend_define(name, CODE) \
static void \
ctx_float_blend_##name (int components, float * __restrict__ dst, float *src, float *blended)\
{\
  float *s = src; float b[components];\
  ctx_float_deassociate_alpha (components, dst, b);\
    CODE;\
  blended[components-1] = s[components-1];\
  ctx_float_associate_alpha (components, blended);\
}

#define ctx_float_blend_define_seperable(name, CODE) \
        ctx_float_blend_define(name, for (int c = 0; c < components-1; c++) { CODE ;}) \

ctx_float_blend_define_seperable(multiply,    blended[c] = (b[c] * s[c]);)
ctx_float_blend_define_seperable(screen,      blended[c] = b[c] + s[c] - (b[c] * s[c]);)
ctx_float_blend_define_seperable(overlay,     blended[c] = b[c] < 0.5f ? (s[c] * b[c]) :
                                                          s[c] + b[c] - (s[c] * b[c]);)
ctx_float_blend_define_seperable(darken,      blended[c] = ctx_minf (b[c], s[c]))
ctx_float_blend_define_seperable(lighten,     blended[c] = ctx_maxf (b[c], s[c]))
ctx_float_blend_define_seperable(color_dodge, blended[c] = (b[c] == 0.0f) ? 0.0f :
                                     s[c] == 1.0f ? 1.0f : ctx_minf(1.0f, (b[c]) / (1.0f-s[c])))
ctx_float_blend_define_seperable(color_burn,  blended[c] = (b[c] == 1.0f) ? 1.0f :
                                     s[c] == 0.0f ? 0.0f : 1.0f - ctx_minf(1.0f, ((1.0f - b[c])) / s[c]))
ctx_float_blend_define_seperable(hard_light,  blended[c] = s[c] < 0.f ? (b[c] * s[c]) :
                                                          b[c] + s[c] - (b[c] * s[c]);)
ctx_float_blend_define_seperable(difference,  blended[c] = (b[c] - s[c]))

ctx_float_blend_define_seperable(divide,      blended[c] = s[c]?(b[c]) / s[c]:0.0f)
ctx_float_blend_define_seperable(addition,    blended[c] = s[c]+b[c])
ctx_float_blend_define_seperable(subtract,    blended[c] = s[c]-b[c])

ctx_float_blend_define_seperable(exclusion,   blended[c] = b[c] + s[c] - 2.0f * b[c] * s[c])
ctx_float_blend_define_seperable(soft_light,
  if (s[c] <= 0.5f)
  {
    blended[c] = b[c] - (1.0f - 2.0f * s[c]) * b[c] * (1.0f - b[c]);
  }
  else
  {
    int d;
    if (b[c] <= 255/4)
      d = (((16 * b[c] - 12.0f) * b[c] + 4.0f) * b[c]);
    else
      d = ctx_sqrtf(b[c]);
    blended[c] = (b[c] + (2.0f * s[c] - 1.0f) * (d - b[c]));
  }
)


ctx_float_blend_define(color,
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_float_set_lum(components, blended, ctx_float_get_lum (components, s));
)

ctx_float_blend_define(hue,
  float in_sat = ctx_float_get_sat(components, b);
  float in_lum = ctx_float_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = s[i];
  ctx_float_set_sat(components, blended, in_sat);
  ctx_float_set_lum(components, blended, in_lum);
)

ctx_float_blend_define(saturation,
  float in_sat = ctx_float_get_sat(components, s);
  float in_lum = ctx_float_get_lum(components, b);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_float_set_sat(components, blended, in_sat);
  ctx_float_set_lum(components, blended, in_lum);
)

ctx_float_blend_define(luminosity,
  float in_lum = ctx_float_get_lum(components, s);
  for (int i = 0; i < components; i++)
    blended[i] = b[i];
  ctx_float_set_lum(components, blended, in_lum);
)

inline static void
ctx_float_blend (int components, CtxBlend blend, float * __restrict__ dst, float *src, float *blended)
{
  switch (blend)
  {
    case CTX_BLEND_NORMAL:      ctx_float_blend_normal      (components, dst, src, blended); break;
    case CTX_BLEND_MULTIPLY:    ctx_float_blend_multiply    (components, dst, src, blended); break;
    case CTX_BLEND_SCREEN:      ctx_float_blend_screen      (components, dst, src, blended); break;
    case CTX_BLEND_OVERLAY:     ctx_float_blend_overlay     (components, dst, src, blended); break;
    case CTX_BLEND_DARKEN:      ctx_float_blend_darken      (components, dst, src, blended); break;
    case CTX_BLEND_LIGHTEN:     ctx_float_blend_lighten     (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_DODGE: ctx_float_blend_color_dodge (components, dst, src, blended); break;
    case CTX_BLEND_COLOR_BURN:  ctx_float_blend_color_burn  (components, dst, src, blended); break;
    case CTX_BLEND_HARD_LIGHT:  ctx_float_blend_hard_light  (components, dst, src, blended); break;
    case CTX_BLEND_SOFT_LIGHT:  ctx_float_blend_soft_light  (components, dst, src, blended); break;
    case CTX_BLEND_DIFFERENCE:  ctx_float_blend_difference  (components, dst, src, blended); break;
    case CTX_BLEND_EXCLUSION:   ctx_float_blend_exclusion   (components, dst, src, blended); break;
    case CTX_BLEND_COLOR:       ctx_float_blend_color       (components, dst, src, blended); break;
    case CTX_BLEND_HUE:         ctx_float_blend_hue         (components, dst, src, blended); break;
    case CTX_BLEND_SATURATION:  ctx_float_blend_saturation  (components, dst, src, blended); break;
    case CTX_BLEND_LUMINOSITY:  ctx_float_blend_luminosity  (components, dst, src, blended); break;
    case CTX_BLEND_ADDITION:    ctx_float_blend_addition    (components, dst, src, blended); break;
    case CTX_BLEND_SUBTRACT:    ctx_float_blend_subtract    (components, dst, src, blended); break;
    case CTX_BLEND_DIVIDE:      ctx_float_blend_divide      (components, dst, src, blended); break;
  }
}

/* this is the grunt working function, when inlined code-path elimination makes
 * it produce efficient code.
 */
CTX_INLINE static void
ctx_float_porter_duff (CtxRasterizer         *rasterizer,
                       int                    components,
                       uint8_t * __restrict__ dst,
                       uint8_t * __restrict__ src,
                       int                    x0,
                       uint8_t * __restrict__ coverage,
                       int                    count,
                       CtxCompositingMode     compositing_mode,
                       CtxFragment            fragment,
                       CtxBlend               blend)
{
  float *dstf = (float*)dst;
  float *srcf = (float*)src;

  CtxPorterDuffFactor f_s, f_d;
  ctx_porter_duff_factors (compositing_mode, &f_s, &f_d);
  uint8_t global_alpha_u8 = rasterizer->state->gstate.global_alpha_u8;
  float   global_alpha_f = rasterizer->state->gstate.global_alpha_f;
  
  {
    float tsrc[components];
    float u0 = 0; float v0 = 0;
    float ud = 0; float vd = 0;

    ctx_init_uv (rasterizer, x0, count, &u0, &v0, &ud, &vd);

    while (count--)
    {
      int cov = *coverage;
#if 1
      if (
        CTX_UNLIKELY((compositing_mode == CTX_COMPOSITE_DESTINATION_OVER && dst[components-1] == 1.0f)||
        (cov == 0 && (compositing_mode == CTX_COMPOSITE_SOURCE_OVER ||
        compositing_mode == CTX_COMPOSITE_XOR               ||
        compositing_mode == CTX_COMPOSITE_DESTINATION_OUT   ||
        compositing_mode == CTX_COMPOSITE_SOURCE_ATOP      
        ))))
      {
        u0 += ud;
        v0 += vd;
        coverage ++;
        dstf+=components;
        continue;
      }
#endif

      fragment (rasterizer, u0, v0, tsrc, 1, ud, vd);
      if (blend != CTX_BLEND_NORMAL)
        ctx_float_blend (components, blend, dstf, tsrc, tsrc);
      u0 += ud;
      v0 += vd;
      float covf = ctx_u8_to_float (cov);

      if (global_alpha_u8 != 255)
        covf = covf * global_alpha_f;

      if (covf != 1.0f)
      {
        for (int c = 0; c < components; c++)
          tsrc[c] *= covf;
      }

      for (int c = 0; c < components; c++)
      {
        float res;
        /* these switches and this whole function disappear when
         * compiled when the enum values passed in are constants.
         */
        switch (f_s)
        {
          case CTX_PORTER_DUFF_0: res = 0.0f; break;
          case CTX_PORTER_DUFF_1:             res = (tsrc[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         res = (tsrc[c] *       dstf[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: res = (tsrc[c] * (1.0f-dstf[components-1])); break;
        }
        switch (f_d)
        {
          case CTX_PORTER_DUFF_0: dstf[c] = res; break;
          case CTX_PORTER_DUFF_1:             dstf[c] = res + (dstf[c]); break;
          case CTX_PORTER_DUFF_ALPHA:         dstf[c] = res + (dstf[c] *       tsrc[components-1]); break;
          case CTX_PORTER_DUFF_1_MINUS_ALPHA: dstf[c] = res + (dstf[c] * (1.0f-tsrc[components-1])); break;
        }
      }
      coverage ++;
      dstf+=components;
    }
  }
}

/* generating one function per compositing_mode would be slightly more efficient,
 * but on embedded targets leads to slightly more code bloat,
 * here we trade off a slight amount of performance
 */
#define ctx_float_porter_duff(compformat, components, source, fragment, blend) \
static void \
ctx_##compformat##_porter_duff_##source (CTX_COMPOSITE_ARGUMENTS) \
{ \
   switch (rasterizer->state->gstate.compositing_mode) \
   { \
     case CTX_COMPOSITE_SOURCE_ATOP: \
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count, \
        CTX_COMPOSITE_SOURCE_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_ATOP:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_ATOP, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION_IN:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_IN, fragment, blend);\
      break;\
     case CTX_COMPOSITE_DESTINATION:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OVER:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OVER:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OVER, fragment, blend);\
       break;\
     case CTX_COMPOSITE_XOR:\
      ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_XOR, fragment, blend);\
       break;\
     case CTX_COMPOSITE_DESTINATION_OUT:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_DESTINATION_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_OUT:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_OUT, fragment, blend);\
       break;\
     case CTX_COMPOSITE_SOURCE_IN:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_SOURCE_IN, fragment, blend);\
       break;\
     case CTX_COMPOSITE_COPY:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_COPY, fragment, blend);\
       break;\
     case CTX_COMPOSITE_CLEAR:\
       ctx_float_porter_duff (rasterizer, components, dst, src, x0, coverage, count,\
        CTX_COMPOSITE_CLEAR, fragment, blend);\
       break;\
   }\
}
#endif

#if CTX_ENABLE_RGBAF

ctx_float_porter_duff(RGBAF, 4,color,           rasterizer->fragment,                               rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,generic,         rasterizer->fragment,               rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
#if CTX_GRADIENTS
ctx_float_porter_duff(RGBAF, 4,linear_gradient, ctx_fragment_linear_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(RGBAF, 4,radial_gradient, ctx_fragment_radial_gradient_RGBAF, rasterizer->state->gstate.blend_mode)
#endif
ctx_float_porter_duff(RGBAF, 4,image,           ctx_fragment_image_RGBAF,           rasterizer->state->gstate.blend_mode)


#if CTX_GRADIENTS
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            NULL,                               blend_mode)\
ctx_float_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_float_porter_duff(comp_name, components,linear_gradient_##blend_name,  ctx_fragment_linear_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,radial_gradient_##blend_name,  ctx_fragment_radial_gradient_RGBA8, blend_mode)\
ctx_float_porter_duff(comp_name, components,image_##blend_name,            ctx_fragment_image_RGBAF,           blend_mode)
#else
#define ctx_float_porter_duff_blend(comp_name, components, blend_mode, blend_name)\
ctx_float_porter_duff(comp_name, components,color_##blend_name,            NULL,                               blend_mode)\
ctx_float_porter_duff(comp_name, components,generic_##blend_name,          rasterizer->fragment,               blend_mode)\
ctx_float_porter_duff(comp_name, components,image_##blend_name,            ctx_fragment_image_RGBAF,           blend_mode)
#endif

ctx_float_porter_duff_blend(RGBAF, 4, CTX_BLEND_NORMAL, normal)


static void
ctx_RGBAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (4, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_RGBAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (4, rasterizer, dst, src, x0, coverage, count);
}

#if 0
static void
ctx_RGBAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (4, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif
#endif

static void
ctx_setup_RGBAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 4;
  rasterizer->fragment = ctx_rasterizer_get_fragment_RGBAF (rasterizer);
#if 1
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
      ctx_color_get_rgba (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
    }
  else
#endif
  {
    rasterizer->comp_op = ctx_RGBAF_porter_duff_generic;
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_RGBAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_RGBAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
        {
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        }
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)(rasterizer->color))[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
            }
            else
            {
              rasterizer->comp_op = ctx_RGBAF_porter_duff_color_normal;
            }
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_linear_gradient_normal;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_radial_gradient_normal;
            break;
#endif
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image_normal;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_color;
//          rasterizer->fragment = NULL;
            break;
#if CTX_GRADIENTS
          case CTX_SOURCE_LINEAR_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_linear_gradient;
            break;
          case CTX_SOURCE_RADIAL_GRADIENT:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_radial_gradient;
            break;
#endif
          case CTX_SOURCE_TEXTURE:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_image;
            break;
          default:
            rasterizer->comp_op = ctx_RGBAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_GRAYAF

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0 ; i < count; i++)
  {
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 1.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}

static void
ctx_fragment_radial_gradient_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float rgba[4];
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i ++)
  {
  float v = 0.0f;
  if ((g->radial_gradient.r1-g->radial_gradient.r0) > 0.0f)
    {
      v = ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y);
      v = (v - g->radial_gradient.r0) / (g->radial_gradient.rdelta);
    }
  ctx_fragment_gradient_1d_RGBAF (rasterizer, v, 0.0, rgba);
  ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgba);
  ((float*)out)[1] = rgba[3];
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}
#endif

static void
ctx_fragment_color_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  for (int i = 0; i < count; i++)
  {
     ctx_color_get_graya (rasterizer->state, &g->color, (float*)out);
     out = ((float*)(out)) + 2;
     x += dx;
     y += dy;
  }
}

static void ctx_fragment_image_GRAYAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t rgba[4];
  float rgbaf[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
    }
  for (int c = 0; c < 2 * count; c ++) { 
    rgbaf[c] = ctx_u8_to_float (rgba[c]);
    ((float*)out)[0] = ctx_float_color_rgb_to_gray (rasterizer->state, rgbaf);
    ((float*)out)[1] = rgbaf[3];
    out = ((float*)out) + 2;
  }
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:           return ctx_fragment_image_GRAYAF;
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYAF;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYAF;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYAF;
#endif
    }
  return ctx_fragment_color_GRAYAF;
}

ctx_float_porter_duff(GRAYAF, 2,color,   rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff(GRAYAF, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL
ctx_float_porter_duff(GRAYAF, 2,color_normal,   rasterizer->fragment, CTX_BLEND_NORMAL)
ctx_float_porter_duff(GRAYAF, 2,generic_normal, rasterizer->fragment, CTX_BLEND_NORMAL)

static void
ctx_GRAYAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_GRAYAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYAF (rasterizer);
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
  //  rasterizer->fragment = NULL;
      ctx_color_get_rgba (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          ((float*)rasterizer->color)[c] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic;
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_GRAYAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_GRAYAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
#if 0
              else if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = ctx_GRAYAF_source_over_normal_opaque_color;
#endif
              else
                rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
//            rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = ctx_GRAYAF_porter_duff_color_normal;
//            rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_color;
//          rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_GRAYAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_GRAYF

static void
ctx_composite_GRAYF (CTX_COMPOSITE_ARGUMENTS)
{
  float *dstf = (float*)dst;

  float temp[count*2];
  for (int i = 0; i < count; i++)
  {
    temp[i*2] = dstf[i];
    temp[i*2+1] = 1.0f;
  }
  rasterizer->comp_op (rasterizer, (uint8_t*)temp, rasterizer->color, x0, coverage, count);
  for (int i = 0; i < count; i++)
  {
    dstf[i] = temp[i*2];
  }
}

#endif
#if CTX_ENABLE_BGRA8

inline static void
ctx_swap_red_green (uint8_t *rgba)
{
  uint32_t *buf  = (uint32_t *) rgba;
  uint32_t  orig = *buf;
  uint32_t  green_alpha = (orig & 0xff00ff00);
  uint32_t  red_blue    = (orig & 0x00ff00ff);
  uint32_t  red         = red_blue << 16;
  uint32_t  blue        = red_blue >> 16;
  *buf = green_alpha | red | blue;
}

static void
ctx_BGRA8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  uint32_t *srci = (uint32_t *) buf;
  uint32_t *dsti = (uint32_t *) rgba;
  while (count--)
    {
      uint32_t val = *srci++;
      ctx_swap_red_green ( (uint8_t *) &val);
      *dsti++      = val;
    }
}

static void
ctx_RGBA8_to_BGRA8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  ctx_BGRA8_to_RGBA8 (rasterizer, x, rgba, (uint8_t *) buf, count);
}

static void
ctx_composite_BGRA8 (CTX_COMPOSITE_ARGUMENTS)
{
  // for better performance, this could be done without a pre/post conversion,
  // by swapping R and B of source instead... as long as it is a color instead
  // of gradient or image
  //
  //
  uint8_t pixels[count * 4];
  ctx_BGRA8_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_BGRA8_to_RGBA8  (rasterizer, x0, &pixels[0], dst, count);
}


#endif
#if CTX_ENABLE_CMYKAF

static void
ctx_fragment_other_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  float *cmyka = (float*)out;
  float _rgba[4 * count];
  float *rgba = &_rgba[0];
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:
        ctx_fragment_image_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
      case CTX_SOURCE_COLOR:
        ctx_fragment_color_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
#if CTX_GRADIENTS
      case CTX_SOURCE_LINEAR_GRADIENT:
        ctx_fragment_linear_gradient_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
      case CTX_SOURCE_RADIAL_GRADIENT:
        ctx_fragment_radial_gradient_RGBAF (rasterizer, x, y, rgba, count, dx, dy);
        break;
#endif
      default:
        rgba[0]=rgba[1]=rgba[2]=rgba[3]=0.0f;
        break;
    }
  for (int i = 0; i < count; i++)
  {
    cmyka[4]=rgba[3];
    ctx_rgb_to_cmyk (rgba[0], rgba[1], rgba[2], &cmyka[0], &cmyka[1], &cmyka[2], &cmyka[3]);
    cmyka += 5;
    rgba += 4;
  }
}

static void
ctx_fragment_color_CMYKAF (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  float *cmyka = (float*)out;
  float cmyka_in[5];
  ctx_color_get_cmyka (rasterizer->state, &gstate->source_fill.color, cmyka_in);
  for (int i = 0; i < count; i++)
  {
    for (int c = 0; c < 4; c ++)
    {
      cmyka[c] = (1.0f - cmyka_in[c]);
    }
    cmyka[4] = cmyka_in[4];
    cmyka += 5;
  }
}

static CtxFragment ctx_rasterizer_get_fragment_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_COLOR:
        return ctx_fragment_color_CMYKAF;
    }
  return ctx_fragment_other_CMYKAF;
}

ctx_float_porter_duff (CMYKAF, 5,color,           rasterizer->fragment, rasterizer->state->gstate.blend_mode)
ctx_float_porter_duff (CMYKAF, 5,generic,         rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL

ctx_float_porter_duff (CMYKAF, 5,color_normal,            rasterizer->fragment, CTX_BLEND_NORMAL)
ctx_float_porter_duff (CMYKAF, 5,generic_normal,          rasterizer->fragment, CTX_BLEND_NORMAL)

static void
ctx_CMYKAF_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_copy_normal (5, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_CMYKAF_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_clear_normal (5, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_CMYKAF_source_over_normal_opaque_color (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_float_source_over_normal_opaque_color (5, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

static void
ctx_setup_CMYKAF (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 5;
  rasterizer->fragment = ctx_rasterizer_get_fragment_CMYKAF (rasterizer);
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
 //     rasterizer->fragment = NULL;
      ctx_color_get_cmyka (rasterizer->state, &gstate->source_fill.color, (float*)rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        ((float*)rasterizer->color)[components-1] *= gstate->global_alpha_f;
    }
  else
  {
    rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
  }


#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_CMYKAF_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_CMYKAF_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (((float*)rasterizer->color)[components-1] == 0.0f)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
#if 0
              else if (((float*)rasterizer->color)[components-1] == 1.0f)
                rasterizer->comp_op = ctx_CMYKAF_source_over_normal_opaque_color;
              else
                rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
              rasterizer->fragment = NULL;
#endif
            }
            else
            {
              rasterizer->comp_op = ctx_CMYKAF_porter_duff_color_normal;
   //         rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic_normal;
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_color;
    //      rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = ctx_CMYKAF_porter_duff_generic;
            break;
        }
        break;
    }
#endif
}

#endif
#if CTX_ENABLE_CMYKA8

static void
ctx_CMYKA8_to_CMYKAF (CtxRasterizer *rasterizer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      for (int c = 0; c < 4; c ++)
        { dst[c] = ctx_u8_to_float ( (255-src[c]) ); }
      dst[4] = ctx_u8_to_float (src[4]);
      for (int c = 0; c < 4; c++)
        { dst[c] *= dst[4]; }
      src += 5;
      dst += 5;
    }
}
static void
ctx_CMYKAF_to_CMYKA8 (CtxRasterizer *rasterizer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      int a = ctx_float_to_u8 (src[4]);
      if (a != 0 && a != 255)
      {
        float recip = 1.0f/src[4];
        for (int c = 0; c < 4; c++)
        {
          dst[c] = ctx_float_to_u8 (1.0f - src[c] * recip);
        }
      }
      else
      {
        for (int c = 0; c < 4; c++)
          dst[c] = 255 - ctx_float_to_u8 (src[c]);
      }
      dst[4]=a;

      src += 5;
      dst += 5;
    }
}

static void
ctx_composite_CMYKA8 (CTX_COMPOSITE_ARGUMENTS)
{
  float pixels[count * 5];
  ctx_CMYKA8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, (uint8_t *) &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_CMYKAF_to_CMYKA8 (rasterizer, &pixels[0], dst, count);
}

#endif
#if CTX_ENABLE_CMYK8

static void
ctx_CMYK8_to_CMYKAF (CtxRasterizer *rasterizer, uint8_t *src, float *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      dst[0] = ctx_u8_to_float (255-src[0]);
      dst[1] = ctx_u8_to_float (255-src[1]);
      dst[2] = ctx_u8_to_float (255-src[2]);
      dst[3] = ctx_u8_to_float (255-src[3]);
      dst[4] = 1.0f;
      src += 4;
      dst += 5;
    }
}
static void
ctx_CMYKAF_to_CMYK8 (CtxRasterizer *rasterizer, float *src, uint8_t *dst, int count)
{
  for (int i = 0; i < count; i ++)
    {
      float c = src[0];
      float m = src[1];
      float y = src[2];
      float k = src[3];
      float a = src[4];
      if (a != 0.0f && a != 1.0f)
        {
          float recip = 1.0f/a;
          c *= recip;
          m *= recip;
          y *= recip;
          k *= recip;
        }
      c = 1.0 - c;
      m = 1.0 - m;
      y = 1.0 - y;
      k = 1.0 - k;
      dst[0] = ctx_float_to_u8 (c);
      dst[1] = ctx_float_to_u8 (m);
      dst[2] = ctx_float_to_u8 (y);
      dst[3] = ctx_float_to_u8 (k);
      src += 5;
      dst += 4;
    }
}

static void
ctx_composite_CMYK8 (CTX_COMPOSITE_ARGUMENTS)
{
  float pixels[count * 5];
  ctx_CMYK8_to_CMYKAF (rasterizer, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, (uint8_t *) &pixels[0], src, x0, coverage, count);
  ctx_CMYKAF_to_CMYK8 (rasterizer, &pixels[0], dst, count);
}
#endif

#if CTX_ENABLE_RGB8

inline static void
ctx_RGB8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[1];
      rgba[2] = pixel[2];
      rgba[3] = 255;
      pixel+=3;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_RGB8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = rgba[0];
      pixel[1] = rgba[1];
      pixel[2] = rgba[2];
      pixel+=3;
      rgba +=4;
    }
}

#endif
#if CTX_ENABLE_GRAY1

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY1_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      if (*pixel & (1<< (x&7) ) )
        {
          rgba[0] = 255;
          rgba[1] = 255;
        }
      else
        {
          rgba[0] = 0;
          rgba[1] = 255;
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = rgba[0];
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray < 127)
        {
          *pixel = *pixel & (~ (1<< (x&7) ) );
        }
      else
        {
          *pixel = *pixel | (1<< (x&7) );
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

#else

inline static void
ctx_GRAY1_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      if (*pixel & (1<< (x&7) ) )
        {
          rgba[0] = 255;
          rgba[1] = 255;
          rgba[2] = 255;
          rgba[3] = 255;
        }
      else
        {
          rgba[0] = 0;
          rgba[1] = 0;
          rgba[2] = 0;
          rgba[3] = 255;
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY1 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int gray = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      //gray += ctx_dither_mask_a (x, rasterizer->scanline/aa, 0, 127);
      if (gray < 127)
        {
          *pixel = *pixel & (~ (1<< (x&7) ) );
        }
      else
        {
          *pixel = *pixel | (1<< (x&7) );
        }
      if ( (x&7) ==7)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY2

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY2_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (3 << ( (x & 3) <<1) ) ) >> ( (x&3) <<1);
      val <<= 6;
      rgba[0] = val;
      rgba[1] = 255;
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY2 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = rgba[0];
      val >>= 6;
      *pixel = *pixel & (~ (3 << ( (x&3) <<1) ) );
      *pixel = *pixel | ( (val << ( (x&3) <<1) ) );
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}
#else

inline static void
ctx_GRAY2_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (3 << ( (x & 3) <<1) ) ) >> ( (x&3) <<1);
      val <<= 6;
      rgba[0] = val;
      rgba[1] = val;
      rgba[2] = val;
      rgba[3] = 255;
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY2 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      val >>= 6;
      *pixel = *pixel & (~ (3 << ( (x&3) <<1) ) );
      *pixel = *pixel | ( (val << ( (x&3) <<1) ) );
      if ( (x&3) ==3)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY4

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY4_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (15 << ( (x & 1) <<2) ) ) >> ( (x&1) <<2);
      val <<= 4;
      rgba[0] = val;
      rgba[1] = 255;
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY4 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = rgba[0];
      val >>= 4;
      *pixel = *pixel & (~ (15 << ( (x&1) <<2) ) );
      *pixel = *pixel | ( (val << ( (x&1) <<2) ) );
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=2;
    }
}
#else
inline static void
ctx_GRAY4_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = (*pixel & (15 << ( (x & 1) <<2) ) ) >> ( (x&1) <<2);
      val <<= 4;
      rgba[0] = val;
      rgba[1] = val;
      rgba[2] = val;
      rgba[3] = 255;
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY4 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      int val = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      val >>= 4;
      *pixel = *pixel & (~ (15 << ( (x&1) <<2) ) );
      *pixel = *pixel | ( (val << ( (x&1) <<2) ) );
      if ( (x&1) ==1)
        { pixel+=1; }
      x++;
      rgba +=4;
    }
}
#endif

#endif
#if CTX_ENABLE_GRAY8

#if CTX_NATIVE_GRAYA8
inline static void
ctx_GRAY8_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = 255;
      pixel+=1;
      rgba +=2;
    }
}

inline static void
ctx_GRAYA8_to_GRAY8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = rgba[0];
      pixel+=1;
      rgba +=2;
    }
}
#else
inline static void
ctx_GRAY8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[0];
      rgba[2] = pixel[0];
      rgba[3] = 255;
      pixel+=1;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAY8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  for (int i = 0; i < count; i ++)
    {
      pixel[i] = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba + i * 4);
    }
}
#endif

#endif
#if CTX_ENABLE_GRAYA8

inline static void
ctx_GRAYA8_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (const uint8_t *) buf;
  while (count--)
    {
      rgba[0] = pixel[0];
      rgba[1] = pixel[0];
      rgba[2] = pixel[0];
      rgba[3] = pixel[1];
      pixel+=2;
      rgba +=4;
    }
}

inline static void
ctx_RGBA8_to_GRAYA8 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      pixel[0] = ctx_u8_color_rgb_to_gray (rasterizer->state, rgba);
      pixel[1] = rgba[3];
      pixel+=2;
      rgba +=4;
    }
}

#if CTX_NATIVE_GRAYA8
CTX_INLINE static void ctx_rgba_to_graya_u8 (CtxState *state, uint8_t *in, uint8_t *out)
{
  out[0] = ctx_u8_color_rgb_to_gray (state, in);
  out[1] = in[3];
}

#if CTX_GRADIENTS
static void
ctx_fragment_linear_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float v = ( ( (g->linear_gradient.dx * x + g->linear_gradient.dy * y) /
                g->linear_gradient.length) -
              g->linear_gradient.start) * (g->linear_gradient.rdelta);
  ctx_fragment_gradient_1d_GRAYA8 (rasterizer, v, 1.0, (uint8_t*)out);
#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)out, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}

#if 0
static void
ctx_fragment_radial_gradient_RGBA8 (CtxRasterizer *rasterizer, float x, float y, void *out)
{
  uint8_t *rgba = (uint8_t *) out;
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, rgba);
#if CTX_DITHER
  ctx_dither_rgba_u8 (rgba, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}
#endif


static void
ctx_fragment_radial_gradient_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  float v = (ctx_hypotf (g->radial_gradient.x0 - x, g->radial_gradient.y0 - y) -
              g->radial_gradient.r0) * (g->radial_gradient.rdelta);
  ctx_fragment_gradient_1d_RGBA8 (rasterizer, v, 0.0, (uint8_t*)out);
#if CTX_DITHER
  ctx_dither_graya_u8 ((uint8_t*)out, x, y, rasterizer->format->dither_red_blue,
                      rasterizer->format->dither_green);
#endif
}
#endif

static void
ctx_fragment_color_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  CtxSource *g = &rasterizer->state->gstate.source_fill;
  ctx_color_get_graya_u8 (rasterizer->state, &g->color, out);
}

static void ctx_fragment_image_GRAYA8 (CtxRasterizer *rasterizer, float x, float y, void *out, int count, float dx, float dy)
{
  uint8_t rgba[4];
  CtxGState *gstate = &rasterizer->state->gstate;
  CtxBuffer *buffer = gstate->source_fill.texture.buffer;
  switch (buffer->format->bpp)
    {
      case 1:  ctx_fragment_image_gray1_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      case 24: ctx_fragment_image_rgb8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);  break;
      case 32: ctx_fragment_image_rgba8_RGBA8 (rasterizer, x, y, rgba, count, dx, dy); break;
      default: ctx_fragment_image_RGBA8 (rasterizer, x, y, rgba, count, dx, dy);       break;
    }
  ctx_rgba_to_graya_u8 (rasterizer->state, rgba, (uint8_t*)out);
}

static CtxFragment ctx_rasterizer_get_fragment_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
    {
      case CTX_SOURCE_TEXTURE:           return ctx_fragment_image_GRAYA8;
#if CTX_GRADIENTS
      case CTX_SOURCE_COLOR:           return ctx_fragment_color_GRAYA8;
      case CTX_SOURCE_LINEAR_GRADIENT: return ctx_fragment_linear_gradient_GRAYA8;
      case CTX_SOURCE_RADIAL_GRADIENT: return ctx_fragment_radial_gradient_GRAYA8;
#endif
    }
  return ctx_fragment_color_GRAYA8;
}

ctx_u8_porter_duff(GRAYA8, 2,color,   NULL,                 rasterizer->state->gstate.blend_mode)
ctx_u8_porter_duff(GRAYA8, 2,generic, rasterizer->fragment, rasterizer->state->gstate.blend_mode)

#if CTX_INLINED_NORMAL

ctx_u8_porter_duff(GRAYA8, 2,color_normal,   NULL,                 CTX_BLEND_NORMAL)
ctx_u8_porter_duff(GRAYA8, 2,generic_normal, rasterizer->fragment, CTX_BLEND_NORMAL)

static void
ctx_GRAYA8_copy_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_copy_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
ctx_GRAYA8_clear_normal (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_clear_normal (2, rasterizer, dst, src, x0, coverage, count);
}

static void
CTX_COMPOSITE_SUFFIX (ctx_GRAYA8_source_over_normal_color) (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_source_over_normal_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}

static void
CTX_COMPOSITE_SUFFIX (ctx_GRAYA8_source_over_normal_opaque_color) (CTX_COMPOSITE_ARGUMENTS)
{
  ctx_u8_source_over_normal_opaque_color (2, rasterizer, dst, rasterizer->color, x0, coverage, count);
}
#endif

inline static int
ctx_is_opaque_color (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  if (gstate->global_alpha_u8 != 255)
    return 0;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
  {
    uint8_t ga[2];
    ctx_color_get_graya_u8 (rasterizer->state, &gstate->source_fill.color, ga);
    return ga[1] == 255;
  }
  return 0;
}

static void
ctx_setup_GRAYA8 (CtxRasterizer *rasterizer)
{
  CtxGState *gstate = &rasterizer->state->gstate;
  int components = 2;
  if (gstate->source_fill.type == CTX_SOURCE_COLOR)
    {
      rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_color);
      rasterizer->fragment = NULL;
      ctx_color_get_rgba8 (rasterizer->state, &gstate->source_fill.color, rasterizer->color);
      if (gstate->global_alpha_u8 != 255)
        for (int c = 0; c < components; c ++)
          rasterizer->color[c] = (rasterizer->color[c] * gstate->global_alpha_u8)/255;
      rasterizer->color[0] = ctx_u8_color_rgb_to_gray (rasterizer->state, rasterizer->color);
      rasterizer->color[1] = rasterizer->color[3];
    }
  else
  {
    rasterizer->fragment = ctx_rasterizer_get_fragment_GRAYA8 (rasterizer);
    rasterizer->comp_op  = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_generic);
  }

#if CTX_INLINED_NORMAL
  if (gstate->compositing_mode == CTX_COMPOSITE_CLEAR)
    rasterizer->comp_op = ctx_GRAYA8_clear_normal;
  else
    switch (gstate->blend_mode)
    {
      case CTX_BLEND_NORMAL:
        if (gstate->compositing_mode == CTX_COMPOSITE_COPY)
        {
          rasterizer->comp_op = ctx_GRAYA8_copy_normal;
        }
        else if (gstate->global_alpha_u8 == 0)
          rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
        else
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            if (gstate->compositing_mode == CTX_COMPOSITE_SOURCE_OVER)
            {
              if (rasterizer->color[components-1] == 0)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_RGBA8_nop);
              else if (rasterizer->color[components-1] == 255)
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_source_over_normal_opaque_color);
              else
                rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_source_over_normal_color);
              rasterizer->fragment = NULL;
            }
            else
            {
              rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_color_normal);
              rasterizer->fragment = NULL;
            }
            break;
          default:
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_generic_normal);
            break;
        }
        break;
      default:
        switch (gstate->source_fill.type)
        {
          case CTX_SOURCE_COLOR:
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_color);
            rasterizer->fragment = NULL;
            break;
          default:
            rasterizer->comp_op = CTX_COMPOSITE_SUFFIX(ctx_GRAYA8_porter_duff_generic);
            break;
        }
        break;
    }
#endif
}
#endif

#endif
#if CTX_ENABLE_RGB332

inline static void
ctx_332_unpack (uint8_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue)
{
  *blue   = (pixel & 3) <<6;
  *green = ( (pixel >> 2) & 7) <<5;
  *red   = ( (pixel >> 5) & 7) <<5;
  if (*blue > 223)  { *blue  = 255; }
  if (*green > 223) { *green = 255; }
  if (*red > 223)   { *red   = 255; }
}

static inline uint8_t
ctx_332_pack (uint8_t red,
              uint8_t green,
              uint8_t blue)
{
  uint8_t c  = (red >> 5) << 5;
  c |= (green >> 5) << 2;
  c |= (blue >> 6);
  return c;
}

static inline void
ctx_RGB332_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
      ctx_332_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2]);
#if CTX_RGB332_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
#endif
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB332 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint8_t *pixel = (uint8_t *) buf;
  while (count--)
    {
#if CTX_RGB332_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_332_pack (255, 0, 255); }
      else
#endif
        { pixel[0] = ctx_332_pack (rgba[0], rgba[1], rgba[2]); }
      pixel+=1;
      rgba +=4;
    }
}

#endif
#if CTX_ENABLE_RGB565 | CTX_ENABLE_RGB565_BYTESWAPPED

static inline void
ctx_565_unpack (uint16_t pixel,
                uint8_t *red,
                uint8_t *green,
                uint8_t *blue,
                int      byteswap)
{
  uint16_t byteswapped;
  if (byteswap)
    { byteswapped = (pixel>>8) | (pixel<<8); }
  else
    { byteswapped  = pixel; }
  *blue   = (byteswapped & 31) <<3;
  *green = ( (byteswapped>>5) & 63) <<2;
  *red   = ( (byteswapped>>11) & 31) <<3;
  if (*blue > 248) { *blue = 255; }
  if (*green > 248) { *green = 255; }
  if (*red > 248) { *red = 255; }
}

static inline uint16_t
ctx_565_pack (uint8_t red,
              uint8_t green,
              uint8_t blue,
              int     byteswap)
{
  uint32_t c = (red >> 3) << 11;
  c |= (green >> 2) << 5;
  c |= blue >> 3;
  if (byteswap)
    { return (c>>8) | (c<<8); } /* swap bytes */
  return c;
}

static inline uint16_t
ctx_888_to_565 (uint32_t in, int byteswap)
{
  uint8_t *rgb=(uint8_t*)(&in);
  return ctx_565_pack (rgb[0],rgb[1],rgb[2], byteswap);
}

static inline uint32_t
ctx_565_to_888 (uint16_t in, int byteswap)
{
  uint32_t ret = 0;
  uint8_t *rgba=(uint8_t*)&ret;
  ctx_565_unpack (in,
                  &rgba[0],
                  &rgba[1],
                  &rgba[2],
                  byteswap);
  return ret;
}

#endif
#if CTX_ENABLE_RGB565


static inline void
ctx_RGB565_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 0);
#if CTX_RGB565_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
#endif
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB565 (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
#if CTX_RGB565_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 0); }
      else
#endif
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 0); }
      pixel+=1;
      rgba +=4;
    }
}

static void
ctx_composite_RGB565 (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * 4];
  ctx_RGB565_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB565 (rasterizer, x0, &pixels[0], dst, count);
}


#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED

static inline void
ctx_RGB565_BS_to_RGBA8 (CtxRasterizer *rasterizer, int x, const void *buf, uint8_t *rgba, int count)
{
  const uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
      ctx_565_unpack (*pixel, &rgba[0], &rgba[1], &rgba[2], 1);
#if CTX_RGB565_ALPHA
      if (rgba[0]==255 && rgba[2] == 255 && rgba[1]==0)
        { rgba[3] = 0; }
      else
#endif
        { rgba[3] = 255; }
      pixel+=1;
      rgba +=4;
    }
}

static inline void
ctx_RGBA8_to_RGB565_BS (CtxRasterizer *rasterizer, int x, const uint8_t *rgba, void *buf, int count)
{
  uint16_t *pixel = (uint16_t *) buf;
  while (count--)
    {
#if CTX_RGB565_ALPHA
      if (rgba[3]==0)
        { pixel[0] = ctx_565_pack (255, 0, 255, 1); }
      else
#endif
        { pixel[0] = ctx_565_pack (rgba[0], rgba[1], rgba[2], 1); }
      pixel+=1;
      rgba +=4;
    }
}

static void
ctx_composite_RGB565_BS (CTX_COMPOSITE_ARGUMENTS)
{
  uint8_t pixels[count * 4];
  ctx_RGB565_BS_to_RGBA8 (rasterizer, x0, dst, &pixels[0], count);
  rasterizer->comp_op (rasterizer, &pixels[0], rasterizer->color, x0, coverage, count);
  ctx_RGBA8_to_RGB565_BS (rasterizer, x0, &pixels[0], dst, count);
}


#endif

CtxPixelFormatInfo CTX_COMPOSITE_SUFFIX(ctx_pixel_formats)[]=
{
#if CTX_ENABLE_RGBA8
  {
    CTX_FORMAT_RGBA8, 4, 32, 4, 0, 0, CTX_FORMAT_RGBA8,
    NULL, NULL, NULL, ctx_setup_RGBA8
  },
#endif
#if CTX_ENABLE_BGRA8
  {
    CTX_FORMAT_BGRA8, 4, 32, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_BGRA8_to_RGBA8, ctx_RGBA8_to_BGRA8, ctx_composite_BGRA8, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_GRAYF
  {
    CTX_FORMAT_GRAYF, 1, 32, 4 * 2, 0, 0, CTX_FORMAT_GRAYAF,
    NULL, NULL, ctx_composite_GRAYF, ctx_setup_GRAYAF,
  },
#endif
#if CTX_ENABLE_GRAYAF
  {
    CTX_FORMAT_GRAYAF, 2, 64, 4 * 2, 0, 0, CTX_FORMAT_GRAYAF,
    NULL, NULL, NULL, ctx_setup_GRAYAF,
  },
#endif
#if CTX_ENABLE_RGBAF
  {
    CTX_FORMAT_RGBAF, 4, 128, 4 * 4, 0, 0, CTX_FORMAT_RGBAF,
    NULL, NULL, NULL, ctx_setup_RGBAF,
  },
#endif
#if CTX_ENABLE_RGB8
  {
    CTX_FORMAT_RGB8, 3, 24, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_RGB8_to_RGBA8, ctx_RGBA8_to_RGB8, ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_GRAY1
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY1, 1, 1, 2, 1, 1, CTX_FORMAT_GRAYA8,
    ctx_GRAY1_to_GRAYA8, ctx_GRAYA8_to_GRAY1, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY1, 1, 1, 4, 1, 1, CTX_FORMAT_RGBA8,
    ctx_GRAY1_to_RGBA8, ctx_RGBA8_to_GRAY1, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY2
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY2, 1, 2, 2, 4, 4, CTX_FORMAT_GRAYA8,
    ctx_GRAY2_to_GRAYA8, ctx_GRAYA8_to_GRAY2, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY2, 1, 2, 4, 4, 4, CTX_FORMAT_RGBA8,
    ctx_GRAY2_to_RGBA8, ctx_RGBA8_to_GRAY2, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY4
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY4, 1, 4, 2, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_GRAYA8, ctx_GRAYA8_to_GRAY4, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY4, 1, 4, 4, 16, 16, CTX_FORMAT_GRAYA8,
    ctx_GRAY4_to_RGBA8, ctx_RGBA8_to_GRAY4, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAY8
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAY8, 1, 8, 2, 0, 0, CTX_FORMAT_GRAYA8,
    ctx_GRAY8_to_GRAYA8, ctx_GRAYA8_to_GRAY8, ctx_composite_convert, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAY8, 1, 8, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_GRAY8_to_RGBA8, ctx_RGBA8_to_GRAY8, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_GRAYA8
  {
#if CTX_NATIVE_GRAYA8
    CTX_FORMAT_GRAYA8, 2, 16, 2, 0, 0, CTX_FORMAT_GRAYA8,
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, NULL, ctx_setup_GRAYA8,
#else
    CTX_FORMAT_GRAYA8, 2, 16, 4, 0, 0, CTX_FORMAT_RGBA8,
    ctx_GRAYA8_to_RGBA8, ctx_RGBA8_to_GRAYA8, ctx_composite_convert, ctx_setup_RGBA8,
#endif
  },
#endif
#if CTX_ENABLE_RGB332
  {
    CTX_FORMAT_RGB332, 3, 8, 4, 10, 12, CTX_FORMAT_RGBA8,
    ctx_RGB332_to_RGBA8, ctx_RGBA8_to_RGB332,
    ctx_composite_convert, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_RGB565
  {
    CTX_FORMAT_RGB565, 3, 16, 4, 32, 64, CTX_FORMAT_RGBA8,
    ctx_RGB565_to_RGBA8, ctx_RGBA8_to_RGB565,
    ctx_composite_RGB565, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_RGB565_BYTESWAPPED
  {
    CTX_FORMAT_RGB565_BYTESWAPPED, 3, 16, 4, 32, 64, CTX_FORMAT_RGBA8,
    ctx_RGB565_BS_to_RGBA8,
    ctx_RGBA8_to_RGB565_BS,
    ctx_composite_RGB565_BS, ctx_setup_RGBA8,
  },
#endif
#if CTX_ENABLE_CMYKAF
  {
    CTX_FORMAT_CMYKAF, 5, 160, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, NULL, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_CMYKA8
  {
    CTX_FORMAT_CMYKA8, 5, 40, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYKA8, ctx_setup_CMYKAF,
  },
#endif
#if CTX_ENABLE_CMYK8
  {
    CTX_FORMAT_CMYK8, 5, 32, 4 * 5, 0, 0, CTX_FORMAT_CMYKAF,
    NULL, NULL, ctx_composite_CMYK8, ctx_setup_CMYKAF,
  },
#endif
  {
    CTX_FORMAT_NONE
  }
};


void
CTX_COMPOSITE_SUFFIX(ctx_compositor_setup) (CtxRasterizer *rasterizer)
{
  if (rasterizer->state->gstate.source_fill.type == CTX_SOURCE_TEXTURE)
  {
    if (!rasterizer->state->gstate.source_fill.texture.buffer->color_managed)
      _ctx_texture_prepare_color_management (rasterizer,
        rasterizer->state->gstate.source_fill.texture.buffer);
  }

  if (rasterizer->format->setup)
  {
    // this works also when _default is passed
    rasterizer->format->setup (rasterizer);
  }
#if CTX_GRADIENTS
#if CTX_GRADIENT_CACHE
  CtxGState *gstate = &rasterizer->state->gstate;
  switch (gstate->source_fill.type)
  {
    case CTX_SOURCE_LINEAR_GRADIENT:
    case CTX_SOURCE_RADIAL_GRADIENT:
      ctx_gradient_cache_prime (rasterizer);
  }
#endif
#endif
}

#endif
#endif //  __CTX_H__

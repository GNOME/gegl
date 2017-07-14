#if TODO // this is the projects todo-list

  bugs
    huge video files cause (cairo) thumtrack overflow, vertical also has this problem - how to split?
    if the initial frame is not a representative video frame of the comp, audio glitches
    ogv render files are more compatible than generated mp4 files, firefox among few that renders the mp4 in audio sync

  roadmap/missing features
    move gcut to gegl git repo

    engine
      rescaling playback speed of clips
      support importing clips of different fps

      support for configuring the final background render to be as high
      fidelity as GEGL processing allows - rather than sharing tuning for
      preview rendering.

      support for other timecodes, mm:ss:ff and s
      using edl files as clip sources - hopefully without even needing caches.

      global filters
        overlaying of audio from wav / mp3 files
        operation chains
        subtitles

    ui
      port gcut-ui.c to lua
      detect locked or crashed ui, kill and respawn
      trimming by mouse / dragging clips around by mouse
      show a modal ui-block when generating proxies/thumbtrack on media import, instead of blocking/being blank
      gaps in timeline (will be implemented as blank clips - but ui could be different)
      insert videos from the commandline
      ui for adding/editing global filters/annotation/sound bits/beeps


#endif

#define CACHE_FORMAT "jpg"
#define GEDL_SAMPLER   GEGL_SAMPLER_NEAREST

#ifndef GEDL_H
#define GEDL_H

#include <gio/gio.h>

typedef struct _GeglEDL GeglEDL;
typedef struct _Clip    Clip;

void gcut_set_use_proxies (GeglEDL *edl, int use_proxies);
int         gegl_make_thumb_video (GeglEDL *edl, const char *path, const char *thumb_path);
char *gcut_make_proxy_path (GeglEDL *edl, const char *clip_path);
const char *compute_cache_path    (const char *path);

#define CACHE_TRY_SIMPLE    (1<<0)
#define CACHE_TRY_MIX       (1<<1)
#define CACHE_TRY_FILTERED  (1<<2)
#define CACHE_TRY_ALL       (CACHE_TRY_SIMPLE | CACHE_TRY_FILTERED | CACHE_TRY_MIX)
#define CACHE_MAKE_FILTERED (1<<3)
#define CACHE_MAKE_SIMPLE   (1<<4)
#define CACHE_MAKE_MIX      (1<<5)
#define CACHE_MAKE_ALL      (CACHE_MAKE_SIMPLE|CACHE_MAKE_MIX|CACHE_MAKE_FILTERED)

enum {
  GEDL_UI_MODE_FULL = 0,
  GEDL_UI_MODE_NONE = 1,
  GEDL_UI_MODE_TIMELINE = 2,
  GEDL_UI_MODE_PART = 3,
};

#define GEDL_LAST_UI_MODE 1

GeglEDL    *gcut_new                (void);
void        gcut_free               (GeglEDL    *edl);
void        gcut_set_fps            (GeglEDL    *edl,
                                     double      fps);
double      gcut_get_fps            (GeglEDL    *edl);
int         gcut_get_duration       (GeglEDL    *edl);
double      gcut_get_time           (GeglEDL    *edl);
void        gcut_parse_line         (GeglEDL    *edl, const char *line);
GeglEDL    *gcut_new_from_path      (const char *path);
void        gcut_load_path          (GeglEDL    *edl, const char *path);
void        gcut_save_path          (GeglEDL    *edl, const char *path);
GeglAudioFragment  *gcut_get_audio  (GeglEDL    *edl);
Clip       *gcut_get_clip           (GeglEDL *edl, int frame, int *clip_frame_no);

void        gcut_set_frame          (GeglEDL    *edl, int frame);
void        gcut_set_time           (GeglEDL    *edl, double seconds);
int         gcut_get_frame          (GeglEDL    *edl);
char       *gcut_serialize          (GeglEDL    *edl);

void        gcut_set_range          (GeglEDL    *edl, int start_frame, int end_frame);
void        gcut_get_range          (GeglEDL    *edl,
                                     int        *start_frame,
                                     int        *end_frame);

void        gcut_set_selection      (GeglEDL    *edl, int start_frame, int end_frame);
void        gcut_get_selection      (GeglEDL    *edl,
                                     int        *start_frame,
                                     int        *end_frame);
char       *gcut_make_thumb_path    (GeglEDL    *edl, const char *clip_path);
guchar     *gcut_get_cache_bitmap   (GeglEDL *edl, int *length_ret);


Clip       *clip_new               (GeglEDL *edl);
void        clip_free              (Clip *clip);
const char *clip_get_path     (Clip *clip);
void        clip_set_path          (Clip *clip, const char *path);
int         clip_get_start         (Clip *clip);
int         clip_get_end           (Clip *clip);
int         clip_get_frames        (Clip *clip);
void        clip_set_start         (Clip *clip, int start);
void        clip_set_end           (Clip *clip, int end);
void        clip_set_range         (Clip *clip, int start, int end);
int         clip_is_static_source  (Clip *clip);
gchar *     clip_get_frame_hash    (Clip *clip, int clip_frame_no);
Clip  *     clip_get_next          (Clip *self);
Clip  *     clip_get_prev          (Clip *self);
void        clip_fetch_audio       (Clip *clip);
void        clip_set_full          (Clip *clip, const char *path, int start, int end);
Clip  *     clip_new_full          (GeglEDL *edl, const char *path, int start, int end);

//void   clip_set_frame_no      (Clip *clip, int frame_no);
void        clip_render_frame       (Clip *clip, int clip_frame_no);

Clip *      edl_get_clip_for_frame (GeglEDL           *edl, int frame);
void        gcut_make_proxies      (GeglEDL           *edl);
void        gcut_get_video_info    (const char        *path, int *duration, double *fps);
void        gegl_meta_set_audio    (const char        *path,
                                    GeglAudioFragment *audio);
void        gegl_meta_get_audio    (const char        *path,
                                    GeglAudioFragment *audio);

#define SPLIT_VER  0.8

extern char *gcut_binary_path;

/*********/

typedef struct SourceClip
{
  char  *path;
  int    start;
  int    end;
  char  *title;
  int    duration;
  int    editing;
  char  *filter_graph; /* chain of gegl filters */
} SourceClip;

struct _Clip
{
  char  *path;  /*path to media file */
  int    start; /*frame number starting with 0 */
  int    end;   /*last frame, inclusive fro single frame, make equal to start */
  char  *title;
  int    duration;
  int    editing;
  char  *filter_graph; /* chain of gegl filters */
  /* to here Clip must match start of SourceClip */
  GeglEDL *edl;

  double fps;
  int    fade; /* the main control for fading in.. */
  int    static_source;
  int    is_chain;
  int    is_meta;

  int    abs_start;

  const char        *clip_path;
  GeglNode          *gegl;
  GeglAudioFragment *audio;
  GeglNode          *chain_loader;
  GeglNode          *full_loader;
  GeglNode          *proxy_loader;
  GeglNode          *loader; /* nop that one of the prior is linked to */

  GeglNode          *nop_scaled;
  GeglNode          *nop_crop;
  GeglNode          *nop_store_buf;

  GMutex             mutex;
};

struct _GeglEDL
{
  GFileMonitor *monitor;
  char         *path;
  char         *parent_path;
  GList        *clip_db;
  GList        *clips;
  int           frame; /* render thread, frame_no is ui side */
  double        fps;
  GeglBuffer   *buffer;
  GeglBuffer   *buffer_copy_temp;
  GeglBuffer   *buffer_copy;
  GMutex        buffer_copy_mutex;
  GeglNode     *cached_result;
  GeglNode     *gegl;
  int           playing;
  int           width;
  int           height;
  GeglNode     *cache_loader;
  int           cache_flags;
  int           selection_start;
  int           selection_end;
  int           range_start;
  int           range_end;
  const char   *output_path;
  const char   *video_codec;
  const char   *audio_codec;
  int           proxy_width;
  int           proxy_height;
  int           video_width;
  int           video_height;
  int           video_size_default;
  int           video_bufsize;
  int           video_bitrate;
  int           video_tolerance;
  int           audio_bitrate;
  int           audio_samplerate;
  int           frame_no;
  int           source_frame_no;
  int           use_proxies;
  int           framedrop;
  int           ui_mode;

  GeglNode     *result;
  GeglNode     *store_final_buf;
  GeglNode     *mix;

  GeglNode     *encode;
  double        scale;
  double        t0;
  Clip         *active_clip;

  void         *mrg;

  char         *clip_query;
  int           clip_query_edited;
  int           filter_edited;
} _GeglEDL;

void update_size (GeglEDL *edl, Clip *clip);
void remove_in_betweens (GeglNode *nop_scaled, GeglNode *nop_filtered);
int  is_connected (GeglNode *a, GeglNode *b);
void gcut_update_buffer (GeglEDL *edl);
void gcut_cache_invalid (GeglEDL *edl);


gchar *gcut_get_frame_hash (GeglEDL *edl, int frame);

gchar *gcut_get_frame_hash_full (GeglEDL *edl, int frame,
                                 Clip **clip0, int *clip0_frame,
                                 Clip **clip1, int *clip1_frame,
                                 double *mix);
int gegl_make_thumb_image (GeglEDL *edl, const char *path, const char *icon_path);

#ifdef MICRO_RAPTOR_GUI
 /* renderer.h */
void renderer_toggle_playing (MrgEvent *event, void *data1, void *data2);
void gcut_cache_invalid (GeglEDL *edl);
int renderer_done (GeglEDL *edl);
void renderer_start (GeglEDL *edl);
gboolean cache_renderer_iteration (Mrg *mrg, gpointer data);

#endif

#endif

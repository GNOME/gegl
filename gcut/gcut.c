#include "config.h"
#include <string.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <gegl.h>
#include <libgen.h>
#include <gegl-audio-fragment.h>

#if HAVE_GEXIV2
#include <gexiv2/gexiv2.h>
#endif

#ifdef G_OS_WIN32
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#define SIGUSR2 (-1) /* quick and dirty, FIXME */
#endif

/* GEGL edit decision list - a digital video cutter and splicer */

/* take a string and expand {t=v i t=v t=v }  to numeric or string
   value. Having it that way.. makes it hard to keep parts of graph,.
   unless graph is kept when constructing... and values are filled in
   if topology of graphs match..
 */

#include "gcut.h"

#define DEFAULT_output_path     "output.mp4"
#define DEFAULT_video_codec     "auto"
#define DEFAULT_audio_codec     "auto"
#define DEFAULT_video_width      0
#define DEFAULT_video_height     0
#define DEFAULT_proxy_width      0
#define DEFAULT_proxy_height     0
#define DEFAULT_video_bufsize    0
#define DEFAULT_video_bitrate    256
#define DEFAULT_video_tolerance  -1
#define DEFAULT_audio_bitrate    64
#define DEFAULT_audio_samplerate 64
#define DEFAULT_frame_start      0
#define DEFAULT_frame_end        0
#define DEFAULT_selection_start  0
#define DEFAULT_selection_end    0
#define DEFAULT_range_start      0
#define DEFAULT_range_end        0
#define DEFAULT_framedrop        0

char *gcut_binary_path = NULL;

const char *default_edl =
#include "default.edl.inc"
;

static char *escaped_base_path (GeglEDL *edl, const char *clip_path)
{
  char *path0= g_strdup (clip_path);
  char *path = path0;
  int i;
  char *ret = 0;
  if (!strncmp (path, edl->parent_path, strlen(edl->parent_path)))
      path += strlen (edl->parent_path);
  for (i = 0; path[i];i ++)
  {
    switch (path[i]){
      case '/':
      case ' ':
      case '\'':
      case '#':
      case '%':
      case '?':
      case '*':
        path[i]='_';
        break;
    }
  }
  ret = g_strdup (path);
  g_free (path0);
  return ret;
}

char *gcut_make_thumb_path (GeglEDL *edl, const char *clip_path)
{
  gchar *ret;
  gchar *path = escaped_base_path (edl, clip_path);
  ret = g_strdup_printf ("%s.gcut/thumb/%s.png", edl->parent_path, path); // XXX: should escape relative/absolute path instead of basename - or add bit of its hash
  g_free (path);
  return ret;
}

char *gcut_make_proxy_path (GeglEDL *edl, const char *clip_path)
{
  gchar *ret;
  gchar *path = escaped_base_path (edl, clip_path);
  ret = g_strdup_printf ("%s.gcut/proxy/%s-%ix%i.mp4", edl->parent_path, path, edl->proxy_width, edl->proxy_height);
  g_free (path);
  return ret;
}


#include <stdlib.h>

GeglEDL *gcut_new           (void)
{
  GeglRectangle roi = {0,0,1024, 1024};
  GeglEDL *edl = g_malloc0(sizeof (GeglEDL));

  edl->gegl = gegl_node_new ();
  edl->cache_flags = 0;
  edl->cache_flags = CACHE_TRY_ALL;
  //edl->cache_flags = CACHE_TRY_ALL | CACHE_MAKE_ALL;
  edl->selection_start = 23;
  edl->selection_end = 42;

  edl->cache_loader     = gegl_node_new_child (edl->gegl, "operation", "gegl:" CACHE_FORMAT  "-load", NULL);

  edl->output_path        = DEFAULT_output_path;
  edl->video_codec        = DEFAULT_video_codec;
  edl->audio_codec        = DEFAULT_audio_codec;
  edl->video_width        = DEFAULT_video_width;
  edl->video_height       = DEFAULT_video_height;
  edl->proxy_width        = DEFAULT_proxy_width;
  edl->proxy_height       = DEFAULT_proxy_height;
  edl->video_size_default = 1;
  edl->video_bufsize      = DEFAULT_video_bufsize;
  edl->video_bitrate      = DEFAULT_video_bitrate;
  edl->video_tolerance    = DEFAULT_video_tolerance;;
  edl->audio_bitrate      = DEFAULT_audio_bitrate;
  edl->audio_samplerate   = DEFAULT_audio_samplerate;
  edl->framedrop          = DEFAULT_framedrop;
  edl->frame_pos_ui       = 0.0; /* frame-no in ui shell */
  edl->frame = -1;        /* frame-no in renderer thread */
  edl->pos  = -1.0;       /* frame-no in renderer thread */
  edl->scale = 1.0;

  edl->buffer = gegl_buffer_new (&roi, babl_format ("R'G'B'A u8"));
  edl->buffer_copy = gegl_buffer_new (&roi, babl_format ("R'G'B'A u8"));
  edl->buffer_copy_temp = gegl_buffer_new (&roi, babl_format ("R'G'B'A u8"));

  edl->clip_query = strdup ("");
  edl->use_proxies = 0;
  edl->ui_mode = GEDL_UI_MODE_PART;

  g_mutex_init (&edl->buffer_copy_mutex);
  return edl;
}

void gcut_set_size (GeglEDL *edl, int width, int height);
void gcut_set_size (GeglEDL *edl, int width, int height)
{
  edl->width = width;
  edl->height = height;
}

void     gcut_free          (GeglEDL *edl)
{
  while (edl->clips)
  {
    clip_free (edl->clips->data);
    edl->clips = g_list_remove (edl->clips, edl->clips->data);
  }
  if (edl->path)
    g_free (edl->path);
  if (edl->parent_path)
    g_free (edl->parent_path);

  g_object_unref (edl->gegl);
  if (edl->buffer)
    g_object_unref (edl->buffer);
  if (edl->buffer_copy)
    g_object_unref (edl->buffer_copy);
  if (edl->buffer_copy_temp)
    g_object_unref (edl->buffer_copy_temp);
  g_mutex_clear (&edl->buffer_copy_mutex);
  g_free (edl);
}

Clip *gcut_get_clip (GeglEDL *edl, double frame_pos, double *clip_frame_pos)
{
  GList *l;
  double clip_start = 0;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    double clip_duration = clip_get_duration (clip);
    if (clip->is_meta)
      continue;

    if (frame_pos - clip_start < clip_duration)
    {
      /* found right clip */
      if (clip_frame_pos)
       *clip_frame_pos = (frame_pos - clip_start) + clip_get_start (clip);
      return clip;
    }
    clip_start += clip_duration;
  }
  return NULL;
}

void gcut_set_use_proxies (GeglEDL *edl, int use_proxies)
{
  double frame_pos;
  edl->use_proxies = use_proxies;

  if (edl->use_proxies)
    gcut_set_size (edl, edl->proxy_width, edl->proxy_height);
  else
    gcut_set_size (edl, edl->video_width, edl->video_height);

  frame_pos = edl->pos;

  if (frame_pos > 0)
  {
    edl->frame--;
    gcut_set_pos (edl, frame_pos);
  }

}

/* computes the hash of a given rendered frame - without altering
 * any state
 */
gchar *gcut_get_pos_hash_full (GeglEDL *edl, double pos,
                               Clip **clip0, double *clip0_pos,
                               Clip **clip1, double *clip1_pos,
                               double *mix)
{
  GString *str = g_string_new ("");
  GList *l;
  double clip_start = 0;
  double prev_clip_start = 0;

  pos = gcut_snap_pos (edl->fps, pos);

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    double clip_duration = clip_get_duration (clip);

    if (clip->is_meta)
      continue;

    if (pos - clip_start < clip_duration)
    {
      double clip_frame_pos = (pos - clip_start) + clip_get_start (clip);

      GList *lp = l->prev;
      GList *ln = l->next;
      Clip *prev = lp?lp->data:NULL;
      Clip *next = ln?ln->data:NULL;
      double prev_fade_len;
      double next_fade_len;

      while (prev && prev->is_meta)
      {
        lp = lp->prev;
        prev = lp?lp->data:NULL;
      }

      while (next && next->is_meta)
      {
        ln = ln->next;
        next = ln?ln->data:NULL;
      }

      /* XXX: fade in from black if there is no previous clip */

      prev_fade_len = prev ? clip_get_duration (prev) : clip_duration;
      next_fade_len = next ? clip_get_duration (next) : clip_duration;

      if (prev_fade_len > clip_duration) prev_fade_len = clip_duration;
      if (next_fade_len > clip_duration) next_fade_len = clip_duration;

      prev_fade_len /= 2;
      next_fade_len /= 2;  /* 1/4 the length of the smallest of this or other
                              clip is our maximum fade - should be adjusted to
                              1/2 later - when perhaps a scaling factor or a
                              per-case duration is set which gets maxed by this
                              heuristic   */
      if (clip->fade/2 < prev_fade_len)
        prev_fade_len = clip->fade/2;

      if (next)
      {
        if (next->fade/2 < next_fade_len)
          next_fade_len = next->fade/2;
      }

      if (prev && pos - clip_start < prev_fade_len)                   /* in */
      {
        char *clip0_hash = clip_get_pos_hash (clip, clip_frame_pos);
        char *clip1_hash = clip_get_pos_hash (prev, pos - prev_clip_start + clip_get_start (prev));
        double ratio = 0.5 + ((pos -clip_start) * 1.0 / prev_fade_len)/2;
        gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];
        g_ascii_dtostr (fstr, sizeof(fstr), ratio);
        g_string_append_printf (str, "%s %s %s", clip1_hash, clip0_hash, fstr);

        g_free (clip0_hash);
        g_free (clip1_hash);
        if (clip0) *clip0 = prev;
        if (clip0_pos) *clip0_pos = pos - prev_clip_start + clip_get_start (prev);
        if (clip1) *clip1 = clip;
        if (clip1_pos) *clip1_pos = clip_frame_pos;
        if (mix) *mix = ratio;
        goto done;
      }

      if (next && pos - clip_start > clip_duration - next_fade_len)/* out*/
      {
        char *clip0_hash = clip_get_pos_hash (clip, clip_frame_pos);
        char *clip1_hash = clip_get_pos_hash (next, pos - (clip_start + clip_duration) + clip_get_start (next));
        double ratio = (1.0-(clip_duration -(pos -clip_start)) * 1.0 / next_fade_len)/2;
        gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];
        g_ascii_dtostr (fstr, sizeof(fstr), ratio);
        g_string_append_printf (str, "%s %s %s", clip0_hash, clip1_hash, fstr);
        g_free (clip0_hash);
        g_free (clip1_hash);
        if (clip0) *clip0 = clip;
        if (clip0_pos) *clip0_pos = clip_frame_pos;
        if (clip1_pos) *clip1_pos = pos - (clip_start +clip_duration) + clip_get_start (next);
        if (clip1) *clip1 = next;
        if (mix)   *mix = ratio;
        goto done;
      }
      else
      {
        char *clip0_hash = clip_get_pos_hash (clip, clip_frame_pos);
        g_string_append_printf (str, "%s ", clip0_hash);
        g_free (clip0_hash);

        if (clip0) *clip0 = clip;
        if (clip0_pos) *clip0_pos = clip_frame_pos;
        if (clip1) *clip1 = NULL;
        if (mix)   *mix = 0.0;
        goto done;
      }
    }
    prev_clip_start = clip_start;
    clip_start += clip_duration;
  }

  if (clip0)     *clip0 = NULL;
  if (clip0_pos) *clip0_pos = 0;
  if (clip1_pos) *clip1_pos = 0;
  if (clip1)     *clip1 = NULL;
  if (mix)       *mix = 0.0;
  return NULL;
done:

  {
    GList *l;
    for (l = edl->clips; l; l = l->next)
    {
      Clip *c = l->data;
      if (c->is_meta)
      {
        if (pos >= c->start && pos < c->end)
          g_string_append_printf (str, "[%s]\n", c->filter_graph);
      }
    }
  }

  {
    GChecksum *hash = g_checksum_new (G_CHECKSUM_MD5);
    char *ret;
    g_checksum_update (hash, (void*)str->str, -1);
    ret = g_strdup (g_checksum_get_string(hash));
    g_checksum_free (hash);
    return ret;
  }
}

gchar *gcut_get_pos_hash (GeglEDL *edl, double pos)
{
  return gcut_get_pos_hash_full (edl, pos, NULL, NULL, NULL, NULL, NULL);
}

void gcut_update_buffer (GeglEDL *edl)
{
  g_mutex_lock (&edl->buffer_copy_mutex);
  {
    GeglBuffer *t = edl->buffer_copy;
    edl->buffer_copy = gegl_buffer_dup (edl->buffer);
    if (t)
      g_object_unref (t);
  }
  g_mutex_unlock (&edl->buffer_copy_mutex);
}
/*  calling this causes gcut to rig up its graphs for providing/rendering this frame
 */
void gcut_set_pos (GeglEDL *edl, double pos)
{
  int frame;
  Clip *clip0; double clip0_pos;
  Clip *clip1; double clip1_pos;

  pos = gcut_snap_pos (edl->fps, pos);
  frame  = pos * edl->fps;

  if ((edl->frame) == frame && (frame != 0))
  {
    return;
  }

  {

  double mix;

  char *frame_hash = gcut_get_pos_hash_full (edl, pos, &clip0, &clip0_pos, &clip1, &clip1_pos, &mix);
  char *cache_path = g_strdup_printf ("%s.gcut/cache/%s", edl->parent_path, frame_hash);
  edl->pos = pos;
  edl->frame = frame;
  g_free (frame_hash);
  if (g_file_test (cache_path, G_FILE_TEST_IS_REGULAR) &&
      (edl->cache_flags & CACHE_TRY_ALL))
  {
    Clip *clip = NULL;
    gegl_node_set (edl->cache_loader, "path", cache_path, NULL);
    gegl_node_link_many (edl->cache_loader, edl->final_result, NULL);
    clip = edl_get_clip_for_pos (edl, pos);
    if (clip)
    {
    if (clip->audio)
      {
        g_object_unref (clip->audio);
        clip->audio = NULL;
      }
    clip->audio = gegl_audio_fragment_new (44100, 2, 0, 44100);
    gegl_meta_get_audio (cache_path, clip->audio);
    }
    {
    GeglRectangle ext = gegl_node_get_bounding_box (edl->final_result);
    gegl_buffer_set_extent (edl->buffer, &ext);
    }
    gegl_node_process (edl->store_final_buf);

    gcut_update_buffer (edl);
    g_free (cache_path);
    return;
  }

  if (clip0 == NULL)
  {
    g_free (cache_path);
    return;
  }

  if (clip1 == NULL)
  {
    clip_render_pos (clip0, clip0_pos);
    gegl_node_link_many (clip0->nop_crop, edl->video_result, NULL);
  }
  else
  {
    gegl_node_set (edl->mix, "ratio", mix, NULL);
    clip_render_pos (clip0, clip0_pos);
    clip_render_pos (clip1, clip1_pos);
    gegl_node_link_many (clip0->nop_crop, edl->mix, edl->video_result, NULL);
    gegl_node_connect_to (clip1->nop_crop, "output", edl->mix, "aux");
  }
  gegl_node_link_many (edl->video_result, edl->final_result, NULL);

  {
    GList *l;
    for (l = edl->clips; l; l = l->next)
    {
      Clip *c = l->data;
      if (c->is_meta)
      {
        if (pos >= c->start && pos < c->end)
        {
          GeglNode *prev = gegl_node_get_producer (edl->final_result, "input", NULL);
          gegl_create_chain (c->filter_graph, prev, edl->final_result,
                             pos - c->start,
                             edl->height, NULL, NULL);
        }
      }
    }
  }

  gegl_node_process (edl->store_final_buf);
  gcut_update_buffer (edl);

  /* write cached render of this frame */
  if (cache_path &&
      !strstr (clip0->path, ".gcut/cache") && (!edl->use_proxies))
    {
      const gchar *cache_path_final = cache_path;
      gchar *cache_path       = g_strdup_printf ("%s~", cache_path_final);

      if (!g_file_test (cache_path_final, G_FILE_TEST_IS_REGULAR) && !edl->playing)
        {
          GeglNode *save_graph = gegl_node_new ();
          GeglNode *save;
          save = gegl_node_new_child (save_graph,
                      "operation", "gegl:" CACHE_FORMAT "-save",
                      "path", cache_path,
                      NULL);
          if (!strcmp (CACHE_FORMAT, "png"))
          {
            gegl_node_set (save, "bitdepth", 8, NULL);
          }
          gegl_node_link_many (edl->final_result, save, NULL);
          gegl_node_process (save);
          if (clip1 && clip1->audio && mix > 0.5)
            gegl_meta_set_audio (cache_path, clip1->audio);
          else if (clip0->audio) // XXX: mix audio
            gegl_meta_set_audio (cache_path, clip0->audio);
          rename (cache_path, cache_path_final);
          g_object_unref (save_graph);
        }
      g_free (cache_path);
    }

  g_free (cache_path);
  }
}

void gcut_set_fps (GeglEDL *edl, double fps)
{
  edl->fps = fps;
}
double gcut_get_fps (GeglEDL *edl)
{
  return edl->fps;
}
double gcut_get_pos (GeglEDL *edl)
{
  return edl->pos;
}

GeglAudioFragment *gcut_get_audio (GeglEDL *edl)
{
  Clip * clip = edl_get_clip_for_pos (edl, edl->pos);
  return clip?clip->audio:NULL;
}

double gcut_get_duration (GeglEDL *edl)
{
  double count = 0;
  GList *l;
  for (l = edl->clips; l; l = l->next)
  {
    ((Clip*)(l->data))->abs_start = count;
    count += clip_get_duration (l->data);
  }
  return count;
}
#include <string.h>


void gcut_parse_line (GeglEDL *edl, const char *line)
{
  double start = 0; double end = 0;
  const char *rest = NULL;
  char path[1024];
  if (line[0] == '#' ||
      line[1] == '#' ||
      strlen (line) < 4)
    return;

  if (strchr (line, '=') && !strstr(line, "--"))
   {
     char *key = g_strdup (line);
     char *value = strchr (key, '=') + 1;
     value[-1]='\0';

     while (value[strlen(value)-1]==' ' ||
            value[strlen(value)-1]=='\n')
            value[strlen(value)-1]='\0';
     if (!strcmp (key, "fps"))               gcut_set_fps (edl, g_strtod (value, NULL));
     if (!strcmp (key, "framedrop"))         edl->framedrop     = g_strtod (value, NULL);
     if (!strcmp (key, "output-path"))       edl->output_path = g_strdup (value);
     if (!strcmp (key, "video-codec"))       edl->video_codec = g_strdup (value);
     if (!strcmp (key, "audio-codec"))       edl->audio_codec = g_strdup (value);
     if (!strcmp (key, "audio-sample-rate")) edl->audio_samplerate = g_strtod (value, NULL);
     if (!strcmp (key, "video-bufsize"))     edl->video_bufsize = g_strtod (value, NULL);
     if (!strcmp (key, "video-bitrate"))     edl->video_bitrate = g_strtod (value, NULL);
     if (!strcmp (key, "audio-bitrate"))     edl->audio_bitrate = g_strtod (value, NULL);
     if (!strcmp (key, "video-width"))       edl->video_width = g_strtod (value, NULL);
     if (!strcmp (key, "video-height"))      edl->video_height = g_strtod (value, NULL);
     if (!strcmp (key, "proxy-width"))       edl->proxy_width = g_strtod (value, NULL);
     if (!strcmp (key, "proxy-height"))      edl->proxy_height = g_strtod (value, NULL);
     if (!strcmp (key, "frame-start"))       edl->range_start = g_strtod (value, NULL);
     if (!strcmp (key, "frame-end"))         edl->range_end = g_strtod (value, NULL);
     if (!strcmp (key, "selection-start"))   edl->selection_start = g_strtod (value, NULL);
     if (!strcmp (key, "selection-end"))     edl->selection_end = g_strtod (value, NULL);
     if (!strcmp (key, "frame-pos"))         edl->frame_pos_ui = g_strtod (value, NULL);
     if (!strcmp (key, "frame-scale"))       edl->scale = g_strtod (value, NULL);
     if (!strcmp (key, "t0"))                edl->t0 = g_strtod (value, NULL);

     g_free (key);
     return;
   }
  if (strstr (line, "--"))
    rest = strstr (line, "--") + 2;


  {
    const char *p = strstr (line, "--");
    if (!p)
      p = line + strlen(line)-1;
    {
      int is_seconds = 0;
      if (p>line) p --;
      while (p>line && *p == ' ') p --;

      while (p>line && (isdigit (*p) || (*p=='s') || (*p=='.') || (*p==':' ))){
        if (*p == 's') is_seconds = 1;
        p --;
      }
      end = g_strtod (p+1, NULL);
      if (!is_seconds)
        end /= edl->fps;

      if (p>line) p --;
      while (p>line && *p == ' ') p --;
      is_seconds = 0;
      while (p>line && (isdigit (*p) || (*p=='s') || (*p=='.') || (*p==':'))){
        if (*p == 's') is_seconds = 1;
        p --;
      }
      start = g_strtod (p+1, NULL);
      if (!is_seconds)
        start /= edl->fps;

      if (p>line) p --;
      while (p>line && *p == ' ') p --;

      memcpy (path, line, (p-line) + 1);
      path[(p-line)+1]=0;
    }
  }

  if (strlen (path) > 3)
    {
      Clip *clip = NULL;
      int ff_probe = 0;
     clip = clip_new_full (edl, path, start, end);


     if (!clip_is_static_source (clip) &&
         (start == 0 && end == 0))
       ff_probe = 1;
     edl->clips = g_list_append (edl->clips, clip);
     if (rest && strstr (rest, "[fade="))
       {
         int was_seconds = 0;
         ff_probe = 1;
         rest = strstr (rest, "[fade=") + strlen ("[fade=");
         clip->fade = g_strtod (rest, NULL);
         while (*rest && *rest != ']'){ if (*rest == 's') was_seconds =1; rest++;}
         if (!was_seconds)
         {
           clip->fade = clip->fade / edl->fps;
         }
         if (*rest == ']') rest++;
       }
     if (rest && strstr (rest, "[fps="))
       {
         ff_probe = 1;
         rest = strstr (rest, "[fps=") + strlen ("[fps=");
         clip->fps = g_strtod (rest, NULL);
         while (*rest && *rest != ']'){ rest++;}
         if (*rest == ']') rest++;
       }
     if (rest && strstr (rest, "[rate="))
       {
         ff_probe = 1;
         rest = strstr (rest, "[rate=") + strlen ("[rate=");
         clip->rate = g_strtod (rest, NULL);
         while (*rest && *rest != ']'){ rest++;}
         if (*rest == ']') rest++;
       }

     if (rest) while (*rest == ' ')rest++;

     if (clip == edl->clips->data || clip->fps < 0.001)
     {
       ff_probe = 1;
     }

     if (ff_probe && !clip_is_static_source (clip))
       {
         int clip_frames;
         gcut_get_video_info (clip->path, &clip_frames, &clip->duration, &clip->fps);

         if (edl->fps == 0.0)
         {
           gcut_set_fps (edl, clip->fps);
         }
       }

     if (rest)
       {
         clip->filter_graph = g_strdup (rest);
         while (clip->filter_graph[strlen(clip->filter_graph)-1]==' ' ||
                clip->filter_graph[strlen(clip->filter_graph)-1]=='\n')
                clip->filter_graph[strlen(clip->filter_graph)-1]='\0';
       }

     if (clip->end == 0)
     {
        clip->end = clip->duration;
     }
    }
  else if (start == 0 && end == 0 && rest)
  {
    Clip *clip = clip_new_full (edl, NULL, 0, 0);
    const char *first = NULL;
    const char *second = NULL;
    const char *p = rest;
    while (*p == ' ') p++;
    if (isdigit(*p))
    {
      first = p;
      while (isdigit(*p) || (*p == '.') || (*p == 's')) p++;
      while (*p == ' ') p++;
      if (isdigit(*p))
      {
        second = p;
        while (isdigit(*p) || (*p == '.') || (*p == 's')) p++;
      }
      while (*p == ' ') p++;
      rest = p;
    }
    clip->filter_graph = g_strdup (rest);
    if (first)
      clip->start = g_strtod (first, NULL);
    if (second)
      clip->end = g_strtod (second, NULL);
    else
      clip->end = clip->start;

    edl->clips = g_list_append (edl->clips, clip);
  }
}

#include <string.h>

void gcut_update_video_size (GeglEDL *edl);
GeglEDL *gcut_new_from_string (const char *string, const char *parent_path);
GeglEDL *gcut_new_from_string (const char *string, const char *parent_path)
{
  GString *line = g_string_new ("");
  GeglEDL *edl = gcut_new ();
  int clips_done = 0;
  int newlines = 0;
  edl->parent_path = g_strdup (parent_path);

  for (const char *p = string; p==string || *(p-1); p++)
  {
    switch (*p)
    {
      case 0:
      case '\n':
       if (clips_done)
       {
      //   if (line->len > 2)
      //     gcut_parse_clip (edl, line->str);
         g_string_assign (line, "");
       }
       else
       {
         if (line->str[0] == '-' &&
             line->str[1] == '-' &&
             line->str[2] == '-')
         {
           clips_done = 1;
           g_string_assign (line, "");
         }
         else
         {
           if (*p == 0)
           {
             newlines = 2;
           }
           else if (*p == '\n')
           {
             newlines ++;
           }
           else
           {
             newlines = 0;
           }
           if (strchr (line->str, '='))
             newlines = 3;

           if (newlines >= 2)
           {
             gcut_parse_line (edl, line->str);
             g_string_assign (line, "");
           }
           else
             g_string_append_c (line, *p);
         }
       }
       break;
      default: g_string_append_c (line, *p);
       break;
    }
  }
  g_string_free (line, TRUE);

  gcut_update_video_size (edl);
  gcut_set_use_proxies (edl, edl->use_proxies);

  return edl;
}

void gcut_save_path (GeglEDL *edl, const char *path)
{
  char *serialized;
  serialized = gcut_serialize (edl);

  if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
  {
     char backup_path[4096];
     struct tm *tim;
     char *old_contents = NULL;

     g_file_get_contents (path, &old_contents, NULL, NULL);
     if (old_contents)
     {
       char *oc, *s;

       oc = strstr (old_contents, "\n\n");
       s  = strstr (serialized, "\n\n");
       if (oc && s && !strcmp (oc, s))
       {
         g_free (old_contents);
         return;
       }
       g_free (old_contents);
     }

     sprintf (backup_path, "%s.gcut/history/%s-", edl->parent_path, basename(edl->path));

     {
     time_t now = time(NULL);
     tim = gmtime(&now);
     }

     strftime(backup_path + strlen(backup_path), sizeof(backup_path)-strlen(backup_path), "%Y%m%d_%H%M%S", tim);
     rename (path, backup_path);
  }

  {
  FILE *file = fopen (path, "w");
  if (!file)
  {
    g_free (serialized);
    return;
  }

  if (serialized)
  {
    fprintf (file, "%s", serialized);
    g_free (serialized);
  }
  fclose (file);
  }
}

void gcut_update_video_size (GeglEDL *edl)
{
  if ((edl->video_width == 0 || edl->video_height == 0) && edl->clips)
    {
      Clip *clip = edl->clips->data;
      GeglNode *gegl = gegl_node_new ();
      GeglRectangle rect;
      // XXX: is ff-load good for pngs and jpgs as well?
      GeglNode *probe;
      probe = gegl_node_new_child (gegl, "operation", "gegl:ff-load", "path", clip->path, NULL);
      gegl_node_process (probe);
      rect = gegl_node_get_bounding_box (probe);
      edl->video_width = rect.width;
      edl->video_height = rect.height;
      g_object_unref (gegl);
    }
  if ((edl->proxy_width <= 0) && edl->video_width)
  {
    edl->proxy_width = 320;
  }
  if ((edl->proxy_height <= 0) && edl->video_width)
  {
    edl->proxy_height = edl->proxy_width * (1.0 * edl->video_height / edl->video_width);
  }
}

static void generate_gcut_dir (GeglEDL *edl)
{
  char *tmp = g_strdup_printf ("cd %s; mkdir .gcut 2>/dev/null ; mkdir .gcut/cache 2>/dev/null ; mkdir .gcut/proxy 2>/dev/null ; mkdir .gcut/thumb 2>/dev/null ; mkdir .gcut/video 2>/dev/null; mkdir .gcut/history 2>/dev/null", edl->parent_path);
  system (tmp);
  g_free (tmp);
}

static GTimer *  timer            = NULL;
static guint     timeout_id       = 0;
static gdouble   throttle         = 4.0;

static void gcut_reread (GeglEDL *edl)
{
  GeglEDL *new_edl = gcut_new_from_path (edl->path);
  GList *l;

  /* swap clips */
  l = edl->clips;
  edl->clips = new_edl->clips;
  new_edl->clips = l;
  edl->active_clip = NULL; // XXX: better to resolve?
  edl->active_overlay = NULL;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    clip->edl = edl;
  }

  for (l = new_edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    clip->edl = new_edl;
  }

  gcut_free (new_edl);
}

static gboolean timeout (gpointer user_data)
{
  GeglEDL *edl = user_data;
  gcut_reread (edl);
  // system (user_data);
  g_timer_start (timer);
  timeout_id = 0;
  return FALSE;
}


static void file_changed (GFileMonitor     *monitor,
                          GFile            *file,
                          GFile            *other_file,
                          GFileMonitorEvent event_type,
                          GeglEDL          *edl)
{
  if (event_type == G_FILE_MONITOR_EVENT_CHANGED)
    {
          if (!timeout_id)
            {
              gdouble elapsed = g_timer_elapsed (timer, NULL);
              gdouble wait    = throttle - elapsed;

              if (wait <= 0.0)
                wait = 0.0;

              timeout_id = g_timeout_add (wait * 1000, timeout, edl);
            }
    }
}

static void
gcut_monitor_start (GeglEDL *edl)
{
  if (!edl->path)
    return;
  /* save to make sure file exists */
  gcut_save_path (edl, edl->path);
  /* start monitor */
  timer = g_timer_new ();
  edl->monitor = g_file_monitor_file (g_file_new_for_path (edl->path),
                                      G_FILE_MONITOR_NONE,
                                      NULL, NULL);
  if(0)g_signal_connect (edl->monitor, "changed", G_CALLBACK (file_changed), edl);
}

GeglEDL *gcut_new_from_path (const char *path)
{
  GeglEDL *edl = NULL;
  gchar *string = NULL;

  g_file_get_contents (path, &string, NULL, NULL);
  if (string)
  {
    char *rpath = realpath (path, NULL);
    char *parent = g_strdup (rpath);
    strrchr(parent, '/')[1]='\0';

      edl = gcut_new_from_string (string, parent);

    g_free (parent);
    g_free (string);
    if (!edl->path)
      edl->path = g_strdup (realpath (path, NULL)); // XXX: leak
  }
  else
  {
    char *parent = NULL;
    if (path[0] == '/')
    {
      parent = strdup (path);
      strrchr(parent, '/')[1]='\0';
    }
    else
    {
      parent = g_malloc0 (strlen(path));
      getcwd (parent, strlen(path));
    }
    edl = gcut_new_from_string ("", parent);
    if (!edl->path)
    {
      if (path[0] == '/')
      {
        edl->path = g_strdup (path);
      }
      else
      {
        edl->path = g_strdup_printf ("%s/%s", parent, basename ((void*)path));
      }
    }
    g_free (parent);
  }
  generate_gcut_dir (edl);

  return edl;
}

static void setup (GeglEDL *edl)
{
  edl->video_result = gegl_node_new_child (edl->gegl, "operation", "gegl:nop", NULL);
  edl->final_result = gegl_node_new_child (edl->gegl, "operation", "gegl:nop", NULL);
  edl->mix = gegl_node_new_child (edl->gegl, "operation", "gegl:mix", NULL);
  edl->encode = gegl_node_new_child (edl->gegl, "operation", "gegl:ff-save",
                                     "path",           edl->output_path,
                                     "frame-rate",     gcut_get_fps (edl),
                                     "video-bit-rate", edl->video_bitrate,
                                     "video-bufsize",  edl->video_bufsize,
                                     "audio-bit-rate", edl->audio_bitrate,
                                     "audio-codec",    edl->audio_codec,
                                     "video-codec",    edl->video_codec,
                                     NULL);
  edl->cached_result = gegl_node_new_child (edl->gegl, "operation", "gegl:buffer-source", "buffer", edl->buffer, NULL);
  edl->store_final_buf = gegl_node_new_child (edl->gegl, "operation", "gegl:write-buffer", "buffer", edl->buffer, NULL);

  gegl_node_link_many (edl->video_result, edl->final_result, NULL);
  gegl_node_link_many (edl->final_result, edl->store_final_buf, NULL);
  gegl_node_link_many (edl->cached_result, edl->encode, NULL);
}

static void init (int argc, char **argv)
{
  gegl_init (&argc, &argv);
  g_object_set (gegl_config (),
                "application-license", "GPL3",
                NULL);
}

static void encode_frames (GeglEDL *edl)
{
  int frame_no;
  for (frame_no = edl->range_start * edl->fps; frame_no <= edl->range_end * edl->fps; frame_no++)
  {
    int frame_no_ui = frame_no;
    gcut_set_pos (edl, frame_no_ui / edl->fps);

    fprintf (stdout, "\r%1.2f%% %04d / %04d   ",
     100.0 * (frame_no/edl->fps-edl->range_start) * 1.0 / (edl->range_end - edl->range_start),
     frame_no, (int)(edl->range_end * edl->fps));

    gegl_node_set (edl->encode, "audio", gcut_get_audio (edl), NULL);
    gegl_node_process (edl->encode);
    fflush (0);
  }
  fprintf (stdout, "\n");
}

static void nop_handler(int sig)
{
}

static int stop_cacher = 0;

static void handler1(int sig)
{
  stop_cacher = 1;
}

static int cacheno = 0;
static int cachecount = 2;

static inline int this_cacher (int frame_no)
{
  if (frame_no % cachecount == cacheno) return 1;
  return 0;
}

static void process_frames_cache (GeglEDL *edl)
{
  int frame_no = edl->frame_pos_ui * edl->fps;
  int frame_start = frame_no;
  int frames;

  GList *l;
  double clip_start = 0;

  signal(SIGUSR2, handler1);
  frames = gcut_get_duration (edl) * edl->fps;
  // TODO: use bitmap from ui to speed up check

  edl->frame_pos_ui = frame_start / edl->fps;
  if (this_cacher (floor (edl->frame_pos_ui * edl->fps)))
    gcut_set_pos (edl, edl->frame_pos_ui);
   if (stop_cacher)
    return;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    double clip_duration = clip_get_duration (clip);
    int frame_pos_ui = floor (clip_start * edl->fps);
    if (this_cacher (floor (frame_pos_ui * edl->fps)))
    {
      gcut_set_pos (edl, frame_pos_ui / edl->fps);
    }

    clip_start += clip_duration;
    if (stop_cacher)
      return;
  }

  for (frame_no = frame_start - 3; frame_no < frames; frame_no++)
  {
    double frame_pos_ui = frame_no / edl->fps;

    if (this_cacher (frame_no))
      gcut_set_pos (edl, frame_pos_ui);
    if (stop_cacher)
      return;
  }
  for (frame_no = 0; frame_no < frame_start; frame_no++)
  {
    double frame_pos_ui = frame_no / edl->fps;
    if (this_cacher (frame_no))
      gcut_set_pos (edl, frame_pos_ui);
    if (stop_cacher)
      return;
  }
}

static inline void set_bit (guchar *bitmap, int no)
{
  bitmap[no/8] |= (1 << (no % 8));
}

guchar *gcut_get_cache_bitmap (GeglEDL *edl, int *length_ret)
{
  double duration = gcut_get_duration (edl);
  int frames = duration * edl->fps;
  int frame_no;
  int length = (frames / 8) + 1;
  guchar *ret = g_malloc0 (length);

  if (length_ret)
    *length_ret = length;

  for (frame_no = 0; frame_no < frames; frame_no++)
  {
    const gchar *hash = gcut_get_pos_hash (edl, frame_no / edl->fps);
    gchar *path = g_strdup_printf ("%s.gcut/cache/%s", edl->parent_path, hash);
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
      set_bit (ret, frame_no);
    g_free (path);
  }

  return ret;
}

static void process_frames_cache_stat (GeglEDL *edl)
{
  int frame_no = edl->frame_pos_ui / edl->fps;
  double duration;
  signal(SIGUSR2, handler1);
  duration = gcut_get_duration (edl);

  /* XXX: should probably do first frame of each clip - since
          these are used for quick keyboard navigation of the
          project
   */

  for (frame_no = 0; frame_no < duration * edl->fps; frame_no++)
  {
    const gchar *hash = gcut_get_pos_hash (edl, frame_no / edl->fps);
    gchar *path = g_strdup_printf ("%s.gcut/cache/%s", edl->parent_path, hash);
    if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
      fprintf (stdout, "%i ", frame_no);
    //  fprintf (stdout, ". %i %s\n", frame_no, hash);
   // else
    //  fprintf (stdout, "  %i %s\n", frame_no, hash);
    g_free (path);
  }
}

int gegl_make_thumb_image (GeglEDL *edl, const char *path, const char *icon_path)
{
  GString *str = g_string_new ("");

  g_string_assign (str, "");
  g_string_append_printf (str, "%s iconographer -p -h -f 'mid-col 96 audio' %s -a %s",
  //g_string_append_printf (str, "iconographer -p -h -f 'thumb 96' %s -a %s",
                          gcut_binary_path, path, icon_path);
  system (str->str);

  g_string_free (str, TRUE);

  return 0;
}

int gegl_make_thumb_video (GeglEDL *edl, const char *path, const char *thumb_path)
{
  GString *str = g_string_new ("");

  g_string_assign (str, "");
  g_string_append_printf (str, "ffmpeg -y -i %s -vf scale=%ix%i %s", path, edl->proxy_width, edl->proxy_height, thumb_path);
  system (str->str);
  g_string_free (str, TRUE);

  return 0;
#if 0  // much slower and worse for fps/audio than ffmpeg method for creating thumbs
  int tot_frames; //
  g_string_append_printf (str, "video-bitrate=100\n\noutput-path=%s\nvideo-width=256\nvideo-height=144\n\n%s\n", thumb_path, path);
  edl = gcut_new_from_string (str->str);
  setup (edl);
  tot_frames = gcut_get_duration (edl);

  if (edl->range_end == 0)
    edl->range_end = tot_frames-1;
  process_frames (edl);
  gcut_free (edl);
  g_string_free (str, TRUE);
  return 0;
#endif
}

#if HAVE_MRG
int gcut_ui_main (GeglEDL *edl);
#else
int gcut_ui_main (GeglEDL *edl);
int gcut_ui_main (GeglEDL *edl)
{
  fprintf (stderr, "gcut built without mrg UI\n");
  return -1;
}
#endif

int gegl_make_thumb_video (GeglEDL *edl, const char *path, const char *thumb_path);
void gcut_make_proxies (GeglEDL *edl)
{
  GList *l;
  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    if (clip->is_chain == 0 && clip->static_source == 0 && clip->is_meta == 0)
    {
      char *proxy_path = gcut_make_proxy_path (edl, clip->path);
      char *thumb_path = gcut_make_thumb_path (edl, clip->path);
      if (!g_file_test (proxy_path, G_FILE_TEST_IS_REGULAR))
        gegl_make_thumb_video (edl, clip->path, proxy_path);
      if (!g_file_test (thumb_path, G_FILE_TEST_IS_REGULAR))
        gegl_make_thumb_image(edl, proxy_path, thumb_path);
      g_free (proxy_path);
      g_free (thumb_path);
    }
  }
}

static void gcut_start_sanity (void)
{
  int fails = 0;
  if (system("which ffmpeg > /dev/null") != 0)
  {
    fprintf (stderr, "gcut missing runtime dependency: ffmpeg command in PATH\n");
    fails ++;
  }
  if (!gegl_has_operation ("gegl:ff-load"))
  {
    fprintf (stderr, "gcut missing runtime dependency: gegl:ff-load operation\n");
    fails ++;
  }
  if (!gegl_has_operation ("gegl:ff-save"))
  {
    fprintf (stderr, "gcut missing runtime dependency: gegl:ff-save operation\n");
    fails ++;
  }
  if (fails)
    exit (-1);
}

gint iconographer_main (gint    argc, gchar **argv);

gint str_has_video_suffix (const gchar *edl_path);
gint str_has_video_suffix (const gchar *edl_path)
{
  if (g_str_has_suffix (edl_path, ".mp4") ||
      g_str_has_suffix (edl_path, ".avi") ||
      g_str_has_suffix (edl_path, ".ogv") ||
      g_str_has_suffix (edl_path, ".mkv") ||
      g_str_has_suffix (edl_path, ".webm") ||
      g_str_has_suffix (edl_path, ".MP4") ||
      g_str_has_suffix (edl_path, ".OGV") ||
      g_str_has_suffix (edl_path, ".MKV") ||
      g_str_has_suffix (edl_path, ".WEBM") ||
      g_str_has_suffix (edl_path, ".AVI"))
    return 1;
  return 0;
}

int main (int argc, char **argv)
{
  GeglEDL *edl = NULL;
  const char *edl_path = "input.edl";
  int tot_frames;

  gcut_binary_path = realpath (argv[0], NULL);
  if (!gcut_binary_path)
    gcut_binary_path = "gcut";

  if (argv[1] && !strcmp (argv[1], "iconographer"))
  {
    argv[1] = argv[0];
    return iconographer_main (argc-1, argv + 1);
  }

  g_setenv ("GEGL_USE_OPENCL", "no", 1);
  g_setenv ("GEGL_MIPMAP_RENDERING", "1", 1);

  init (argc, argv);
  gcut_start_sanity ();

  if (!argv[1])
  {
    static char *new_argv[3]={NULL, "gcut.edl", NULL};
    new_argv[0] = argv[0];
    argv = new_argv;
    argc++;
    g_file_set_contents (argv[1], default_edl, -1, NULL);
  }

  edl_path = argv[1]; //realpath (argv[1], NULL);

  if (str_has_video_suffix (edl_path))
  {
    char * path = realpath (edl_path, NULL);
    char * rpath = g_strdup_printf ("%s.edl", path);
    if (g_file_test (rpath, G_FILE_TEST_IS_REGULAR))
      edl_path = rpath;
  }

  if (str_has_video_suffix (edl_path))
  {
    char str[1024];
    int frames;
    double duration;
    double fps;
    gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];

    GeglNode *gegl = gegl_node_new ();
    GeglNode *probe = gegl_node_new_child (gegl, "operation",
                                           "gegl:ff-load", "path", edl_path,
                                           NULL);
    gegl_node_process (probe);

    gegl_node_get (probe, "frames", &frames, NULL);
    gegl_node_get (probe, "frame-rate", &fps, NULL);
    duration = frames / fps;
    g_object_unref (gegl);

    g_ascii_dtostr (fstr, sizeof(fstr), duration);
    sprintf (str, "%s 0.0s %ss\n", edl_path, fstr);
    {
      char * path = realpath (edl_path, NULL); 
      char * rpath = g_strdup_printf ("%s.edl", path);
      char * parent = g_strdup (rpath);
      strrchr(parent, '/')[1]='\0';
      edl = gcut_new_from_string (str, parent);
      g_free (parent);
      edl->path = rpath;
      free (path);
    }
    generate_gcut_dir (edl);
  }
  else
  {
    edl = gcut_new_from_path (edl_path);
  }

  chdir (edl->parent_path); /* we try as good as we can to deal with absolute
                               paths correctly,  */

  setup (edl);

  {
#define RUNMODE_UI     0
#define RUNMODE_RENDER 1
#define RUNMODE_CACHE  2
#define RUNMODE_CACHE_STAT  3
#define RUNMODE_RESERIALIZE  4
    int runmode = RUNMODE_UI;
    for (int i = 0; argv[i]; i++)
    {
      if (!strcmp (argv[i], "render")) runmode = RUNMODE_RENDER;
      if (!strcmp (argv[i], "reserialize")) runmode = RUNMODE_RESERIALIZE;
      if (!strcmp (argv[i], "cachestat")) runmode = RUNMODE_CACHE_STAT;
      if (!strcmp (argv[i], "cache"))
      {
        runmode = RUNMODE_CACHE;
        if (argv[i+1])
        {
          cacheno = atoi (argv[i+1]);
          if (argv[i+2])
            cachecount = atoi (argv[i+2]);
        }
      }
    }

    switch (runmode)
    {
      case RUNMODE_RESERIALIZE:
        printf ("%s", gcut_serialize (edl));
        exit (0);
        break;
      case RUNMODE_UI:

        signal(SIGUSR2, nop_handler);

        gcut_monitor_start (edl);

        return gcut_ui_main (edl);
      case RUNMODE_RENDER:
        tot_frames  = gcut_get_duration (edl);
        if (edl->range_end == 0)
          edl->range_end = tot_frames-1;
        encode_frames (edl);
        gcut_free (edl);
        return 0;
      case RUNMODE_CACHE:
        tot_frames = gcut_get_duration (edl) * edl->fps;
        if (edl->range_end == 0)
          edl->range_end = gcut_get_duration (edl);
        process_frames_cache (edl);
        gcut_free (edl);
        return 0;
      case RUNMODE_CACHE_STAT:
        process_frames_cache_stat (edl);
        gcut_free (edl);
        return 0;
     }
  }
  return -1;
}

char *gcut_serialize (GeglEDL *edl)
{
  GList *l;
  char *ret;
  GString *ser = g_string_new ("");
  gchar fstr[G_ASCII_DTOSTR_BUF_SIZE];

  if (edl->proxy_width != DEFAULT_proxy_width)
    g_string_append_printf (ser, "proxy-width=%i\n",  edl->proxy_width);
  if (edl->proxy_height != DEFAULT_proxy_height)
    g_string_append_printf (ser, "proxy-height=%i\n", edl->proxy_height);
  if (edl->framedrop != DEFAULT_framedrop)
    g_string_append_printf (ser, "framedrop=%i\n", edl->framedrop);

  if (strcmp(edl->output_path, DEFAULT_output_path))
    g_string_append_printf (ser, "output-path=%s\n", edl->output_path);
  if (strcmp(edl->video_codec, DEFAULT_video_codec))
    g_string_append_printf (ser, "video-codec=%s\n", edl->video_codec);
  if (strcmp(edl->audio_codec, DEFAULT_audio_codec))
    g_string_append_printf (ser, "audio-codec=%s\n", edl->audio_codec);
  if (edl->video_width != DEFAULT_video_width)
    g_string_append_printf (ser, "video-width=%i\n",  edl->video_width);
  if (edl->video_height != DEFAULT_video_height)
    g_string_append_printf (ser, "video-height=%i\n", edl->video_height);
  if (edl->video_bufsize != DEFAULT_video_bufsize)
    g_string_append_printf (ser, "video-bufsize=%i\n", edl->video_bufsize);
  if (edl->video_bitrate != DEFAULT_video_bitrate)
    g_string_append_printf (ser, "video-bitrate=%i\n",  edl->video_bitrate);
  if (edl->video_tolerance != DEFAULT_video_tolerance)
    g_string_append_printf (ser, "video-tolerance=%i\n", edl->video_tolerance);
  if (edl->audio_bitrate != DEFAULT_audio_bitrate)
    g_string_append_printf (ser, "audio-bitrate=%i\n",  edl->audio_bitrate);
  if (edl->audio_samplerate != DEFAULT_audio_samplerate)
    g_string_append_printf (ser, "audio-samplerate=%i\n",  edl->audio_samplerate);

  g_ascii_dtostr (fstr, sizeof(fstr), gcut_get_fps (edl));
  g_string_append_printf (ser, "fps=%s\n", fstr);

  if (edl->range_start != DEFAULT_range_start)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->range_start);
    g_string_append_printf (ser, "range-start=%s\n",  fstr);
  }
  if (edl->range_end != DEFAULT_range_end)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->range_end);
    g_string_append_printf (ser, "range-end=%s\n", fstr);
  }

  if (edl->selection_start != DEFAULT_selection_start)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->selection_start);
    g_string_append_printf (ser, "selection-start=%s\n", fstr);
  }

  if (edl->selection_end != DEFAULT_selection_end)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->selection_end);
    g_string_append_printf (ser, "selection-end=%s\n", fstr);
  }

  if (edl->scale != 1.0)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->scale);
    g_string_append_printf (ser, "frame-scale=%s\n", fstr);
  }
  if (edl->t0 != 1.0)
  {
    g_ascii_dtostr (fstr, sizeof(fstr), edl->t0);
    g_string_append_printf (ser, "t0=%s\n", fstr);
  }

  g_ascii_dtostr (fstr, sizeof(fstr), edl->frame_pos_ui);
  g_string_append_printf (ser, "frame-pos=%s\n", fstr);
  g_string_append_printf (ser, "\n");

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    gchar *path = clip->path;
    if (!path)
      path = "";
    if (!strncmp (path, edl->parent_path, strlen(edl->parent_path)))
      path += strlen (edl->parent_path);

    if (clip->is_meta)
    {
      if (clip->start == 0 && clip->end == 0)
        g_string_append_printf (ser, "-- ");
      else
      {
        g_ascii_dtostr (fstr, sizeof(fstr), clip->start);
        g_string_append_printf (ser, "-- %ss", fstr);
        g_ascii_dtostr (fstr, sizeof(fstr), clip->end);
        g_string_append_printf (ser, " %ss ", fstr);
      }
      g_string_append_printf (ser, "%s\n", clip->filter_graph);
    }
    else if (strlen(path)== 0 &&
         clip->start == 0 &&
         clip->end == 0 &&
         clip->filter_graph)
    {
      g_string_append_printf (ser, "--%s\n", clip->filter_graph);
    }
    else
    {
      g_ascii_dtostr (fstr, sizeof(fstr), clip->start);
      g_string_append_printf (ser, "%s %ss ", path, fstr);
      g_ascii_dtostr (fstr, sizeof(fstr), clip->end);
      g_string_append_printf (ser, "%ss ", fstr);
      if (clip->filter_graph||clip->fade)
        g_string_append_printf (ser, "-- ");
      if (clip->fade)
      {
        g_ascii_dtostr (fstr, sizeof(fstr), clip->fade);
        g_string_append_printf (ser, "[fade=%ss] ", fstr);
      }
      if (clip->fps>0.001)
      {
        g_ascii_dtostr (fstr, sizeof(fstr), clip->fps);
        g_string_append_printf (ser, "[fps=%s] ", fstr);
      }
      if (fabs(clip->rate - 1.0 )>0.001)
      {
        g_ascii_dtostr (fstr, sizeof(fstr), clip->rate);
        g_string_append_printf (ser, "[rate=%s] ", fstr);
      }
      if (clip->filter_graph)
        g_string_append_printf (ser, "%s", clip->filter_graph);
      g_string_append_printf (ser, "\n");
    }
  }
  g_string_append_printf (ser, "-----\n");
  ret=ser->str;
  g_string_free (ser, FALSE);
  return ret;
}

void gcut_set_selection (GeglEDL *edl, double start_frame, double end_frame)
{
  edl->selection_start = start_frame;
  edl->selection_end   = end_frame;
}

void gcut_get_selection (GeglEDL *edl,
                         double  *start_frame,
                         double  *end_frame)
{
  if (start_frame)
    *start_frame = edl->selection_start;
  if (end_frame)
    *end_frame = edl->selection_end;
}

void gcut_set_range (GeglEDL *edl, double start_frame, double end_frame)
{
  edl->range_start = start_frame;
  edl->range_end   = end_frame;
}

void gcut_get_range (GeglEDL *edl,
                     double  *start_frame,
                     double  *end_frame)
{
  if (start_frame)
    *start_frame = edl->range_start;
  if (end_frame)
    *end_frame = edl->range_end;
}

Clip * edl_get_clip_for_pos (GeglEDL *edl, double pos)
{
  GList *l;
  double t = 0;
  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    double duration = clip_get_duration (clip);
    if (pos >= t && pos < t + duration)
    {
      return clip;
    }
    t += duration;
  }
  return NULL;
}

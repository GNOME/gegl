#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <gegl.h>
#include <gegl-audio-fragment.h>
#include "gcut.h"

Clip *clip_new (GeglEDL *edl)
{
  Clip *clip = g_malloc0 (sizeof (Clip));
  clip->edl  = edl;
  clip->gegl = gegl_node_new ();
  clip->rate = 1.0;

  clip->chain_loader = gegl_node_new_child (clip->gegl, "operation", "gegl:nop", NULL);

  clip->full_loader  = gegl_node_new_child (clip->gegl, "operation", "gegl:ff-load", NULL);
  clip->proxy_loader = gegl_node_new_child (clip->gegl, "operation", "gegl:ff-load", NULL);
  clip->loader       = gegl_node_new_child (clip->gegl, "operation", "gegl:nop", NULL);

  clip->nop_scaled = gegl_node_new_child (clip->gegl, "operation", "gegl:scale-size-keepaspect",
                                       "y", 0.0, //
                                       "x", 1.0 * edl->width,
                                       "sampler", GEDL_SAMPLER,
                                       NULL);
  clip->nop_crop     = gegl_node_new_child (clip->gegl, "operation", "gegl:crop", "x", 0.0, "y", 0.0, "width", 1.0 * edl->width,
                                        "height", 1.0 * edl->height, NULL);

  clip->nop_store_buf = gegl_node_new_child (clip->gegl, "operation", "gegl:write-buffer", "buffer", edl->buffer, NULL);
#if 0
  clip->full_store_buf = gegl_node_new_child (clip->gegl, "operation", "gegl:write-buffer", "buffer", edl->buffer, NULL);
  clip->preview_store_buf = gegl_node_new_child (clip->gegl, "operation", "gegl:write-buffer", "buffer", edl->buffer, NULL);
#endif

  gegl_node_link_many (clip->full_loader,
                       clip->loader,
                       clip->nop_scaled,
                       clip->nop_crop,
                       clip->nop_store_buf,
                       NULL);

  g_mutex_init (&clip->mutex);

  return clip;
}

Clip *clip_get_prev (Clip *self)
{
  GList *l;
  GeglEDL *edl;
  Clip *prev = NULL;
  if (!self)
    return NULL;
  edl = self->edl;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    if (clip->is_meta)
      continue;
    if (clip == self)
      return prev;
    prev = clip;
  }
  return NULL;
}
Clip *clip_get_next (Clip *self)
{
  GList *l;
  GeglEDL *edl;
  int found = 0;
  if (!self)
    return NULL;
  edl = self->edl;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    if (clip->is_meta)
      continue;
    if (found)
      return clip;
    if (clip == self)
      found = 1;
  }
  return NULL;
}

void clip_free (Clip *clip)
{
  if (clip->path)
    g_free (clip->path);
  clip->path = NULL;

  if (clip->gegl)
    g_object_unref (clip->gegl);
  clip->gegl = NULL;
  g_mutex_clear (&clip->mutex);
  g_free (clip);
}

void clip_set_path (Clip *clip, const char *in_path)
{
  char *path = NULL;
  clip->is_chain = 0;
  clip->is_meta = 0;

  if (!in_path)
  {
    clip->is_meta = 1;
    if (clip->path)
      g_free (clip->path);
    clip->path = NULL;
    return;
  }

  if (!strcmp (in_path, "black") ||
      !strcmp (in_path, "blue") ||
      strstr (in_path, "gegl:"))
    clip->is_chain = 1;

  if (in_path[0] == '/' || clip->is_chain)
  {
    path = g_strdup (in_path);
  }
  else
  {
    if (clip->edl->parent_path)
      path = g_strdup_printf ("%s%s", clip->edl->parent_path, in_path);
    else
      path = g_strdup_printf ("%s", in_path);
  }

  if (clip->path && !strcmp (clip->path, path))
  {
    g_free (path);
    return;
  }

  if (clip->path)
    g_free (clip->path);
  clip->path = path;

  if (clip->is_chain)
  {
    GError *error = NULL;
    if (is_connected (clip->chain_loader, clip->loader))
      remove_in_betweens (clip->chain_loader, clip->loader);
    else
      gegl_node_link_many (clip->chain_loader, clip->loader, NULL);

    gegl_create_chain (path, clip->chain_loader, clip->loader, 0,
                       400, //edl->height,
                       NULL, &error);
    if (error)
      {
        /* should set error string */
        fprintf (stderr, "chain source: %s\n", error->message);
        g_error_free (error);
      }
  }
  else
  {
    if (g_str_has_suffix (path, ".png") ||
        g_str_has_suffix (path, ".jpg") ||
        g_str_has_suffix (path, ".exr") ||
        g_str_has_suffix (path, ".EXR") ||
        g_str_has_suffix (path, ".PNG") ||
        g_str_has_suffix (path, ".JPG"))
     {
       g_object_set (clip->full_loader, "operation", "gegl:load", NULL);
       clip->static_source = 1;
     }
    else
     {
       g_object_set (clip->full_loader, "operation", "gegl:ff-load", NULL);
       clip->static_source = 0;
     }
  }
}

double clip_get_start (Clip *clip)
{
  return clip->start;
}

double clip_get_end (Clip *clip)
{
  return clip->end;
}

static inline double clip_fps (Clip *clip)
{
  if (!clip) return 0.0;
  return clip->fps>0.01?clip->fps:clip->edl->fps;
}

double clip_get_duration (Clip *clip)
{
  double duration = clip_get_end (clip) - clip_get_start (clip) + 1.0/clip_fps(clip);
  if (duration < 0) duration = 0;
  if (clip->is_meta)
    return 0;
  return duration;
}

void clip_set_start (Clip *clip, double start)
{
  clip->start = start;
}
void clip_set_end (Clip *clip, double end)
{
  clip->end = end;
}

void clip_set_range (Clip *clip, double start, double end)
{
  clip_set_start (clip, start);
  clip_set_end (clip, end);
}

void clip_set_full (Clip *clip, const char *path, double start, double end)
{
  clip_set_path (clip, path);
  clip_set_range (clip, start, end);
}

Clip *clip_new_full (GeglEDL *edl, const char *path, double start, double end)
{
  Clip *clip = clip_new (edl);
  clip_set_full (clip, path, start, end);
  return clip;
}

const char *clip_get_path (Clip *clip)
{
  return clip->path;
}

static void clip_set_proxied (Clip *clip)
{
  if (clip->is_chain)
    return;

  if (clip->edl->use_proxies)
    {
      char *path = gcut_make_proxy_path (clip->edl, clip->path);
      gchar *old = NULL;
      gegl_node_get (clip->proxy_loader, "path", &old, NULL);

      if (!old || !strcmp (old, "") || !strcmp (path, old))
        gegl_node_set (clip->proxy_loader, "path", path, NULL);
      gegl_node_link_many (clip->proxy_loader, clip->loader, NULL);
      g_free (path);
    }
  else
    {
      gchar *old = NULL;
      gegl_node_get (clip->full_loader, "path", &old, NULL);
      if (!old || !strcmp (old, "") || !strcmp (clip->path, old))
        gegl_node_set (clip->full_loader, "path", clip->path, NULL);
      gegl_node_link_many (clip->full_loader, clip->loader, NULL);
    }
}

void clip_set_frame_no (Clip *clip, double clip_frame_no);
void clip_set_frame_no (Clip *clip, double clip_frame_no)
{
  if (clip_frame_no < 0)
    clip_frame_no = 0;

  clip_set_proxied (clip);
#if 0
  {
    gchar *old = NULL;
    gegl_node_get (clip->full_loader, "path", &old, NULL);
    if (!old || !strcmp (old, "") || !strcmp (clip->path, old))
      gegl_node_set (clip->full_loader, "path", clip->path, NULL);
  }
#endif

  if (!clip_is_static_source (clip))
    {
      double fps = clip_fps (clip);

      if (clip->edl->use_proxies)
      {
        gegl_node_set (clip->proxy_loader, "frame", (gint)(clip_frame_no * fps), NULL);
      }
      else
        gegl_node_set (clip->full_loader, "frame", (gint)(clip_frame_no * fps), NULL);

    }
}

int clip_is_static_source (Clip *clip)
{
  return clip->static_source;
}

void clip_fetch_audio (Clip *clip)
{
  int use_proxies = clip->edl->use_proxies;

  if (clip->audio)
    {
      g_object_unref (clip->audio);
      clip->audio = NULL;
    }

  if (clip_is_static_source (clip))
    clip->audio = NULL;
  else
    {
      if (use_proxies)
        gegl_node_get (clip->proxy_loader, "audio", &clip->audio, NULL);
      else
        gegl_node_get (clip->full_loader, "audio", &clip->audio, NULL);
    }
}

int is_connected (GeglNode *a, GeglNode *b)
{
  GeglNode *iter = a;
  while (iter && iter != b)
  {
    GeglNode **nodes = NULL;
    int count = gegl_node_get_consumers (iter, "output", &nodes, NULL);
    if (count) iter = nodes[0];
    else
            iter = NULL;
    g_free (nodes);
  }
  if (iter == b)
    return 1;
  return 0;
}

void remove_in_betweens (GeglNode *nop_scaled, GeglNode *nop_filtered)
{
 GeglNode *iter = nop_scaled;
 GList *collect = NULL;

 iter = nop_filtered;
 while (iter && iter != nop_scaled)
 {
   GeglNode **nodes = NULL;
   iter = gegl_node_get_producer (iter, "input", NULL);
   g_free (nodes);
   if (iter && iter != nop_scaled)
     collect = g_list_append (collect, iter);
 }

 while (collect)
 {
    g_object_unref (collect->data);
    collect = g_list_remove (collect, collect->data);
 }
 gegl_node_link_many (nop_scaled, nop_filtered, NULL);
}

static void clip_rig_chain (Clip *clip, double clip_pos)
{
  GeglEDL *edl = clip->edl;
  int use_proxies = edl->use_proxies;

  g_mutex_lock (&clip->mutex);

  remove_in_betweens (clip->nop_scaled, clip->nop_crop);

  gegl_node_set (clip->nop_scaled, "operation", "gegl:scale-size-keepaspect",
                               "y", 0.0,
                               "x", 1.0 * edl->width,
                               "sampler", use_proxies?GEDL_SAMPLER:GEGL_SAMPLER_CUBIC,
                               NULL);

  gegl_node_set (clip->nop_crop, "width", 1.0 * edl->width,
                                 "height", 1.0 * edl->height,
                                 NULL);
  if (clip->is_chain)
  {
    if (is_connected (clip->chain_loader, clip->loader))
      remove_in_betweens (clip->chain_loader, clip->loader);
    else
      gegl_node_link_many (clip->chain_loader, clip->loader, NULL);

    gegl_create_chain (clip->path, clip->chain_loader, clip->loader, clip_pos,
                       edl->height,
                       NULL, NULL);//&error);
  }

  if (clip->filter_graph)
    {
       GError *error = NULL;
       gegl_create_chain (clip->filter_graph, clip->nop_scaled, clip->nop_crop, clip_pos, edl->height, NULL, &error);
       if (error)
         {
           /* should set error string */
           fprintf (stderr, "%s\n", error->message);
           g_error_free (error);
         }
     }
  /**********************************************************************/

  // flags,..    FULL   PREVIEW   FULL_CACHE|PREVIEW  STORE_FULL_CACHE
  clip_set_frame_no (clip, clip_pos);
  g_mutex_unlock (&clip->mutex);
}

void clip_render_pos (Clip *clip, double clip_frame_pos)
{
  double pos = 
    clip->start + (clip_frame_pos - clip->start) * clip->rate;

  clip_rig_chain (clip, pos);
  g_mutex_lock (&clip->mutex);
  gegl_node_process (clip->loader); // for the audio fetch
  clip_fetch_audio (clip);
  g_mutex_unlock (&clip->mutex);
}

gchar *clip_get_pos_hash (Clip *clip, double clip_frame_pos)
{
  GeglEDL *edl = clip->edl;
  gchar *frame_recipe;
  char *ret;
  GChecksum *hash;
  int is_static_source = clip_is_static_source (clip);

  // quantize to clip/project fps 
  //clip_frame_pos = ((int)(clip_frame_pos * clip_fps (clip) + 0.5))/ clip_fps (clip);

  frame_recipe = g_strdup_printf ("%s: %.3f %s %.3f %s %ix%i",
      "gcut-pre-4", clip->rate,
      clip_get_path (clip),
      clip->filter_graph || (!is_static_source) ? clip_frame_pos : 0.0,
      clip->filter_graph,
      edl->video_width,
      edl->video_height);

  hash = g_checksum_new (G_CHECKSUM_MD5);
  g_checksum_update (hash, (void*)frame_recipe, -1);
  ret = g_strdup (g_checksum_get_string(hash));

  g_checksum_free (hash);
  g_free (frame_recipe);

  return ret;
}

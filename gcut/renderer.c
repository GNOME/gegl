#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <gegl.h>
#include <mrg.h>
#include "gcut.h"
#include <gegl-audio-fragment.h>

static GThread *thread = NULL;
long babl_ticks (void);
static long prev_ticks = 0;
int rendering_frame = -1;
int done_frame     = -1;
static int audio_started = 0;

void gcut_cache_invalid (GeglEDL *edl)
{
  edl->frame = -1;
  done_frame=-1;
  rendering_frame=-1;
}

static void open_audio (Mrg *mrg, int frequency)
{
  mrg_pcm_set_sample_rate (mrg, frequency);
  mrg_pcm_set_format      (mrg, MRG_s16S);
}

static void end_audio (void)
{
}


static inline void skipped_frames (int count)
{
  //fprintf (stderr, "[%i]", count);
}

static inline void wait_for_frame (void)
{
  //fprintf (stderr, ".");
}


void playing_iteration (Mrg *mrg, GeglEDL *edl);

extern int got_cached;

static gpointer renderer_thread (gpointer data)
{
  GeglEDL *edl = data;

  for (;;)
  {
    playing_iteration (edl->mrg, edl);
    {
      if ((int)(edl->frame_pos_ui * edl->fps) != done_frame)
      {
        rendering_frame = edl->frame_pos_ui * edl->fps;

        {
          GeglRectangle ext = {0,0,edl->width, edl->height};
          //GeglRectangle ext = gegl_node_get_bounding_box (edl->result);
          gegl_buffer_set_extent (edl->buffer, &ext);
        }
        gcut_set_pos (edl, edl->frame_pos_ui); /* this does the frame-set, which causes render  */
#if 0
        {
          GeglRectangle ext = gegl_node_get_bounding_box (edl->result);
          gegl_buffer_set_extent (edl->buffer, &ext);
        }
#endif

        {
          GeglAudioFragment *audio = gcut_get_audio (edl);
          if (audio)
          {
            int sample_count = gegl_audio_fragment_get_sample_count (audio);
            if (sample_count > 0)
            {
              int i;
              if (!audio_started)
              {
                open_audio (edl->mrg, gegl_audio_fragment_get_sample_rate (audio));
                audio_started = 1;
              }

              {
                uint16_t temp_buf[sample_count*2];
                for (i = 0; i < sample_count; i++)
                {
                  temp_buf[i*2] = audio->data[0][i] * 32767.0 * 0.46;
                  temp_buf[i*2+1] = audio->data[1][i] * 32767.0 * 0.46;
                }
                mrg_pcm_queue (edl->mrg, (void*)&temp_buf[0], sample_count);
              }
            }
          }
        }
        done_frame = rendering_frame;
        mrg_queue_draw (edl->mrg, NULL); // could queue only rect - if we had it
      }
      else
      {
        g_usleep (50);
      }
    }
  }
  end_audio ();
  return NULL;
}

void renderer_start (GeglEDL *edl)
{
  if (!thread)
    thread = g_thread_new ("renderer", renderer_thread, edl);
}

gboolean cache_renderer_iteration (Mrg *mrg, gpointer data);

void renderer_toggle_playing (MrgEvent *event, void *data1, void *data2)
{
  GeglEDL *edl = data1;
  edl->playing =  !edl->playing;
  if (!edl->playing)
  {
    cache_renderer_iteration (event->mrg, edl);
  }
  else
  {
    killpg(0, SIGUSR2);
  }
  mrg_event_stop_propagate (event);
  mrg_queue_draw (event->mrg, NULL);
  prev_ticks = babl_ticks ();
}

static double max_pos (GeglEDL *edl)
{
  GList *l;
  int t = 0;
  double start, end;

  gcut_get_range (edl, &start, &end);
  if (end)
    return end;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    t += clip_get_duration (clip);
  }

  return t;
}


void playing_iteration (Mrg *mrg, GeglEDL *edl)
{
  long ticks = 0;
  double delta = 1;
  double fragment = 1.0 / edl->fps;
  ticks = babl_ticks ();
  if (prev_ticks == 0) prev_ticks = ticks;

  if (edl->playing)
    {
#if 0
      if (prev_ticks - ticks < 1000000.0 / gcut_get_fps (edl))
        return;
#endif
      delta = (((ticks - prev_ticks) / 1000000.0) * ( edl->fps ));
      //fprintf (stderr, "%f\n", delta);
      if (delta < 1.0)
      {
        wait_for_frame ();
        mrg_queue_draw (mrg, NULL);
        return;
      }
      {
#if 0
        static int frameskip = -1;
        if (frameskip < 0)
        {
          if (getenv ("GEDL_FRAMESKIP"))
            frameskip = 1;
          else
            frameskip = 0;
        }
        if (!frameskip)
          delta = 1;
#else
        if (edl->framedrop)
        {
          if (delta >= 2.0)
            {
              skipped_frames ( (int)(delta)-1 );
            }
        }
        else
        {
          if (delta > 1.0)
            delta = 1;
          else
            delta = 0;
        }
#endif
      }
      if (rendering_frame != done_frame)
        return;
      if (delta >= 1.0)
      {

      if (edl->active_clip)
      {
        double start, end;
        edl->frame_pos_ui += delta * fragment;
        gcut_get_range (edl, &start, &end);
        if (edl->frame_pos_ui > max_pos (edl))
        {
           edl->frame_pos_ui = 0;
           if (end)
             edl->frame_pos_ui = start;
        }
        gcut_snap_ui_pos (edl);
        edl->active_clip = edl_get_clip_for_pos (edl, edl->frame_pos_ui);
        prev_ticks = ticks;
      }
      }
    }
}

int renderer_done (GeglEDL *edl)
{
  return done_frame == (gint)(edl->frame_pos_ui * edl->fps); //rendering_frame;
}

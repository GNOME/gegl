#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <gegl.h>
#include <mrg.h>
#include "gcut.h"
#include <SDL.h>
#include <gegl-audio-fragment.h>

static GThread *thread = NULL;
long babl_ticks (void);
static long prev_ticks = 0;
int rendering_frame = -1;
int done_frame     = -1;
static int audio_started = 0;
static int audio_len    = 0;
static int audio_pos    = 0;
static int audio_post   = 0;

#define AUDIO_BUF_LEN 819200000

int16_t audio_data[AUDIO_BUF_LEN];

void gcut_cache_invalid (GeglEDL *edl)
{
  edl->frame = -1;
  done_frame=-1;
  rendering_frame=-1;
}


static void sdl_audio_cb(void *udata, Uint8 *stream, int len)
{
  int audio_remaining = audio_len - audio_pos;
  if (audio_remaining < 0)
    return;

  if (audio_remaining < len) len = audio_remaining;

  //SDL_MixAudio(stream, (uint8_t*)&audio_data[audio_pos/2], len, SDL_MIX_MAXVOLUME);
  memcpy (stream, (uint8_t*)&audio_data[audio_pos/2], len);
  audio_pos += len;
  audio_post += len;
  if (audio_pos >= AUDIO_BUF_LEN)
  {
    audio_pos = 0;
  }
}

static void sdl_add_audio_sample (int sample_pos, float left, float right)
{
   audio_data[audio_len/2 + 0] = left * 32767.0 * 0.46;
   audio_data[audio_len/2 + 1] = right * 32767.0 * 0.46;
   audio_len += 4;

   if (audio_len >= AUDIO_BUF_LEN)
   {
     audio_len = 0;
   }
}

static void open_audio (int frequency)
{
  SDL_AudioSpec spec = {0};
  SDL_Init(SDL_INIT_AUDIO);
  spec.freq = frequency;
  spec.format = AUDIO_S16SYS;
  spec.channels = 2;
  spec.samples = 1024;
  spec.callback = sdl_audio_cb;
  SDL_OpenAudio(&spec, 0);

  if (spec.format != AUDIO_S16SYS)
   {
      fprintf (stderr, "not getting format we wanted\n");
   }
  if (spec.freq != frequency)
   {
      fprintf (stderr, "not getting desires samplerate(%i) we wanted got %i instead\n", frequency, spec.freq);
   }
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
      if (edl->frame_no != done_frame)
      {
        rendering_frame = edl->frame_no;

        {
          GeglRectangle ext = {0,0,edl->width, edl->height};
          //GeglRectangle ext = gegl_node_get_bounding_box (edl->result);
          gegl_buffer_set_extent (edl->buffer, &ext);
        }
        gcut_set_frame (edl, edl->frame_no); /* this does the frame-set, which causes render  */
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
                open_audio (gegl_audio_fragment_get_sample_rate (audio));
                SDL_PauseAudio(0);
                audio_started = 1;
              }
              for (i = 0; i < sample_count; i++)
              {
                sdl_add_audio_sample (0, audio->data[0][i], audio->data[1][i]);
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

static int max_frame (GeglEDL *edl)
{
  GList *l;
  int t = 0;
  int start, end;

  gcut_get_range (edl, &start, &end);
  if (end)
    return end;

  for (l = edl->clips; l; l = l->next)
  {
    Clip *clip = l->data;
    t += clip_get_frames (clip);
  }

  return t;
}


void playing_iteration (Mrg *mrg, GeglEDL *edl)
{
  long ticks = 0;
  double delta = 1;
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
        int start, end;
        edl->frame_no += delta;
        gcut_get_range (edl, &start, &end);
        if (edl->frame_no > max_frame (edl))
        {
           edl->frame_no = 0;
           if (end)
             edl->frame_no = start;
        }
        edl->active_clip = edl_get_clip_for_frame (edl, edl->frame_no);
        prev_ticks = ticks;
      }
      }
    }
}

int renderer_done (GeglEDL *edl)
{
  return done_frame == edl->frame_no; //rendering_frame;
}

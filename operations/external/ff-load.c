/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2003, 2006, 2015 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#define realpath(a,b) _fullpath(b,a,_MAX_PATH)
#endif

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
   description (_("Path of video file to load"))

property_int (frame, _("Frame number"), 0)
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_int (frames, _("frames"), 0)
   description (_("Number of frames in video, updates at least when first frame has been decoded."))
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_int (audio_sample_rate, _("audio_sample_rate"), 0)
property_int (audio_channels, _("audio_channels"), 0)

property_double (frame_rate, _("frame-rate"), 0)
   description (_("Frames per second, permits computing time vs frame"))
   value_range (0, G_MAXINT)
   ui_range (0, 10000)

property_string (video_codec, _("video-codec"), "")
property_string (audio_codec, _("audio-codec"), "")

property_audio_fragment (audio, _("audio"), 0)

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME ff_load
#define GEGL_OP_C_SOURCE ff-load.c

#include "gegl-op.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include <libavutil/channel_layout.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>


typedef struct
{
  gint             width;
  gint             height;
  gdouble          fps;
  gint             codec_delay;
  int64_t          first_dts;

  gchar           *loadedfilename; /* to remember which file is "cached"     */

  AVFormatContext *audio_fcontext;
  const AVCodec   *audio_codec;
  int              audio_index;
  GList           *audio_track;
  long             audio_cursor_pos;
  long             audio_pos;
  gdouble          prevapts;
  glong            a_prevframe;   /* previously decoded a_frame in loadedfile */


  AVFormatContext *video_fcontext;
  int              video_index;
  AVStream        *video_stream;
  AVCodecContext  *video_ctx;
  AVStream        *audio_stream;
  AVCodecContext  *audio_ctx;
  const AVCodec   *video_codec;
  AVFrame         *lavc_frame;
  AVFrame         *rgb_frame;
  glong            prevframe;      /* previously decoded frame number */
  gdouble          prevpts;        /* timestamp in seconds of last decoded frame */

} Priv;

static void
print_error (const char *filename, int err)
{
  switch (err)
    {
    case AVERROR(EINVAL):
      g_warning ("%s: Incorrect image filename syntax.\n"
                 "Use '%%d' to specify the image number:\n"
                 "  for img1.jpg, img2.jpg, ..., use 'img%%d.jpg';\n"
                 "  for img001.jpg, img002.jpg, ..., use 'img%%03d.jpg'.\n",
                 filename);
      break;
    case AVERROR_INVALIDDATA:
      g_warning ("%s: Error while parsing header or unknown format\n", filename);
      break;
    default:
      g_warning ("%s: Error while opening file\n", filename);
      break;
    }
}

static void
clear_audio_track (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;
  while (p->audio_track)
    {
      g_object_unref (p->audio_track->data);
      p->audio_track = g_list_remove (p->audio_track, p->audio_track->data);
    }
  p->prevapts = 0.0;
}

static void
ff_cleanup (GeglProperties *o)
{
  Priv *p = (Priv*)o->user_data;
  if (p)
    {
      clear_audio_track (o);
      g_free (p->loadedfilename);
      avcodec_free_context (&p->video_ctx);
      avcodec_free_context (&p->audio_ctx);
      if (p->video_fcontext)
        avformat_close_input(&p->video_fcontext);
      if (p->audio_fcontext)
        avformat_close_input(&p->audio_fcontext);
      if (p->rgb_frame)
        av_free (p->rgb_frame);
      if (p->lavc_frame)
        av_free (p->lavc_frame);

      p->video_fcontext = NULL;
      p->audio_fcontext = NULL;
      p->lavc_frame = NULL;
      p->rgb_frame = NULL;
      p->loadedfilename = NULL;
    }
}

static void
init (GeglProperties *o)
{
  Priv       *p = (Priv*)o->user_data;

  if (p == NULL)
    {
      p = g_new0 (Priv, 1);
      o->user_data = (void*) p;
    }

  p->width = 320;
  p->height = 200;

  clear_audio_track (o);
  p->loadedfilename = g_strdup ("");

  ff_cleanup (o);
  av_log_set_level (AV_LOG_WARNING);
}

/* maintain list of audio samples */
static int
decode_audio (GeglOperation *operation,
              gdouble        pts1,
              gdouble        pts2)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  pts1 -= 2.0;
  if (pts1 < 0.0)pts1 = 0.0;

  if(pts1 - 15.0 > p->prevapts){
  int64_t seek_target = av_rescale_q (pts1 * AV_TIME_BASE, AV_TIME_BASE_Q, p->audio_stream->time_base);
     clear_audio_track (o);
     p->prevapts = 0.0;

     if (av_seek_frame (p->audio_fcontext, p->audio_stream->index, seek_target, (AVSEEK_FLAG_BACKWARD)) < 0)
      fprintf (stderr, "audio seek error!\n");
     else
      avcodec_flush_buffers (p->audio_ctx);

  }

  while (p->prevapts <= pts2)
    {
      AVPacket  pkt = {0,};
      int       ret;

      if (av_read_frame (p->audio_fcontext, &pkt) < 0)
         {
           av_packet_unref (&pkt);
           return -1;
         }
      if (pkt.stream_index==p->audio_index && p->audio_stream)
        {
          static AVFrame frame;

          ret = avcodec_send_packet (p->audio_ctx, &pkt);
          if (ret < 0)
            {
              fprintf (stderr, "avcodec_send_packet failed for %s\n",
                                o->path);
            }
          while (ret == 0)
            {
              ret = avcodec_receive_frame (p->audio_ctx, &frame);
              if (ret == AVERROR(EAGAIN))
                {
                  // no more frames; should send the next packet now
                  ret = 0;
                  break;
                }
              else if (ret < 0)
                {
                  fprintf (stderr, "avcodec_receive_frame failed for %s\n",
                                    o->path);
                  break;
                }
              int samples_left = frame.nb_samples;
              int si = 0;

              while (samples_left)
              {
                 int sample_count = samples_left;
#if LIBAVCODEC_VERSION_MAJOR < 61
                 int channels = MIN(p->audio_stream->codecpar->channels, GEGL_MAX_AUDIO_CHANNELS);
#else
                 int channels = MIN(p->audio_stream->codecpar->ch_layout.nb_channels, GEGL_MAX_AUDIO_CHANNELS);
#endif
                 GeglAudioFragment *af = gegl_audio_fragment_new (o->audio_sample_rate, channels,
                            AV_CH_LAYOUT_STEREO, samples_left);
  //);
                 switch (p->audio_ctx->sample_fmt)
                 {
                   case AV_SAMPLE_FMT_FLT:
                     for (gint i = 0; i < sample_count; i++)
                       for (gint c = 0; c < channels; c++)
                         af->data[c][i] = ((int16_t *)frame.data[0])[(i + si) * channels + c];
                     break;
                   case AV_SAMPLE_FMT_FLTP:
                     for (gint i = 0; i < sample_count; i++)
                       for (gint c = 0; c < channels; c++)
                         {
                           af->data[c][i] = ((float *)frame.data[c])[i + si];
                         }
                     break;
                   case AV_SAMPLE_FMT_S16:
                     for (gint i = 0; i < sample_count; i++)
                       for (gint c = 0; c < channels; c++)
                         af->data[c][i] = ((int16_t *)frame.data[0])[(i + si) * channels + c] / 32768.0;
                     break;
                   case AV_SAMPLE_FMT_S16P:
                     for (gint i = 0; i < sample_count; i++)
                       for (gint c = 0; c < channels; c++)
                         af->data[c][i] = ((int16_t *)frame.data[c])[i + si] / 32768.0;
                     break;
                   case AV_SAMPLE_FMT_S32:
                     for (gint i = 0; i < sample_count; i++)
                       for (gint c = 0; c < channels; c++)
                         af->data[c][i] = ((int32_t *)frame.data[0])[(i + si) * channels + c] / 2147483648.0;
                    break;
                  case AV_SAMPLE_FMT_S32P:
                     for (gint i = 0; i < sample_count; i++)
                      for (gint c = 0; c < channels; c++)
                        af->data[c][i] = ((int32_t *)frame.data[c])[i + si] / 2147483648.0;
                    break;
                  default:
                    g_warning ("undealt with sample format\n");
                  }
                  gegl_audio_fragment_set_sample_count (af, sample_count);
                  gegl_audio_fragment_set_pos (
                    af,
                    (long int)av_rescale_q (
                      (pkt.pts),
                      p->audio_stream->time_base,
                      AV_TIME_BASE_Q
                    ) * o->audio_sample_rate / AV_TIME_BASE
                  );

                  p->audio_pos += sample_count;
                  p->audio_track = g_list_append (p->audio_track, af);

                  samples_left -= sample_count;
                  si += sample_count;
                }
              p->prevapts = pkt.pts * av_q2d (p->audio_stream->time_base);
            }
        }
      av_packet_unref (&pkt);
    }
  return 0;
}

static int
decode_frame (GeglOperation *operation,
              glong          frame)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;
  glong       prevframe = p->prevframe;
  glong       decodeframe = prevframe;

  if (frame < 0)
    {
      frame = 0;
    }
  else if (frame >= o->frames)
    {
      frame = o->frames - 1;
    }
  if (frame == prevframe)
    {
      return 0;
    }

  decodeframe = frame;
  if (p->video_stream)
  {

  if (frame < 2 || frame > prevframe + 64 || frame < prevframe )
  {
    int64_t seek_target = av_rescale_q (((frame) * AV_TIME_BASE * 1.0) / o->frame_rate
#if LIBAVCODEC_VERSION_MAJOR < 60
, AV_TIME_BASE_Q, p->video_stream->time_base) / p->video_ctx->ticks_per_frame;
#else
, AV_TIME_BASE_Q, p->video_stream->time_base) / (p->video_ctx->codec_descriptor->props & AV_CODEC_PROP_FIELDS ? 2 : 1);
#endif

    if (av_seek_frame (p->video_fcontext, p->video_index, seek_target, (AVSEEK_FLAG_BACKWARD )) < 0)
      fprintf (stderr, "video seek error!\n");
    else
      avcodec_flush_buffers (p->video_ctx);

    prevframe = -1;
  }

  do
    {
      int       got_picture = 0;
      do
        {
          int       ret;
          AVPacket  pkt = {0,};

          do
          {
            av_packet_unref (&pkt);
            if (av_read_frame (p->video_fcontext, &pkt) < 0)
            {
              av_packet_unref (&pkt);
              return -1;
            }
          }
          while (pkt.stream_index != p->video_index);

          ret = avcodec_send_packet (p->video_ctx, &pkt);
          if (ret < 0)
            {
              fprintf (stderr, "avcodec_send_packet failed for %s\n",
                       o->path);
              return -1;
            }
          while (ret == 0)
            {
              if (!p->first_dts)
                p->first_dts = pkt.dts;
              ret = avcodec_receive_frame (p->video_ctx, p->lavc_frame);
              if (ret == AVERROR(EAGAIN))
                {
                  // no more frames; should send the next packet now
                  ret = 0;
                  break;
                }
              else if (ret < 0)
                {
                  fprintf (stderr, "avcodec_receive_frame failed for %s\n",
                                    o->path);
                  break;
                }
              got_picture = 1;
#if LIBAVUTIL_VERSION_MAJOR < 58
              if ((pkt.dts == pkt.pts) || (p->lavc_frame->key_frame!=0))
#else
              if ((pkt.dts == pkt.pts) || (p->lavc_frame->flags & AV_FRAME_FLAG_KEY))
#endif
                {
                  // cur_dts and first_dts are moved to libavformat/internal.h
                  /*
                  p->lavc_frame->pts = (p->video_stream->cur_dts -
                                        p->video_stream->first_dts);
                  */
                  p->lavc_frame->pts = pkt.dts - p->first_dts;
                  p->prevpts =  av_rescale_q (p->lavc_frame->pts,
                                              p->video_stream->time_base,
                                              AV_TIME_BASE_Q) * 1.0 / AV_TIME_BASE;
                  decodeframe = roundf( p->prevpts * o->frame_rate);
                }
              else
                {
                  p->prevpts += 1.0 / o->frame_rate;
                  decodeframe = roundf ( p->prevpts * o->frame_rate);
                }
              if (decodeframe > frame + p->codec_delay)
                break;
            }
#if 0
          if (decoded_bytes != pkt.size)
            fprintf (stderr, "bytes left!\n");
#endif
          av_packet_unref (&pkt);
        }
      while (!got_picture);
    }
    while (decodeframe <= frame + p->codec_delay);
  }

  p->prevframe = frame;
  return 0;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  if (p == NULL)
    init (o);
  p = (Priv*)o->user_data;

  g_assert (o->user_data != NULL);

  gegl_operation_set_format (operation, "output", babl_format ("R'G'B' u8"));

  if (o->path &&
      (!p->loadedfilename ||
      strcmp (p->loadedfilename, o->path) ||
       p->prevframe > o->frame  /* a bit heavy handed, but improves consistency */
      ))
    {
      gint i;
      gchar *dereferenced_path;
      gint err;

      ff_cleanup (o);
      dereferenced_path = realpath (o->path, NULL);
      if (!dereferenced_path)
        return;
      err = avformat_open_input(&p->video_fcontext, dereferenced_path, NULL, 0);
      free (dereferenced_path);
      if (err < 0)
        {
          print_error (o->path, err);
          return;
        }
      err = avformat_find_stream_info (p->video_fcontext, NULL);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);
          return;
        }
      err = avformat_open_input(&p->audio_fcontext, o->path, NULL, 0);
      if (err < 0)
        {
          print_error (o->path, err);
          return;
        }
      err = avformat_find_stream_info (p->audio_fcontext, NULL);
      if (err < 0)
        {
          g_warning ("ff-load: error finding stream info for %s", o->path);
          return;
        }

      for (i = 0; i< p->video_fcontext->nb_streams; i++)
        {
          AVCodecParameters *codecpar = p->video_fcontext->streams[i]->codecpar;
          if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
              p->video_stream = p->video_fcontext->streams[i];
              p->video_index = i;
            }
          if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
              p->audio_stream = p->audio_fcontext->streams[i];
              p->audio_index = i;
            }
        }

      if (p->video_stream)
        {
          p->video_codec = avcodec_find_decoder (p->video_stream->codecpar->codec_id);
          if (p->video_codec == NULL)
            {
              g_warning ("video codec not found");
              p->video_ctx = NULL;
              return;
            }
          p->video_ctx = avcodec_alloc_context3 (p->video_codec);
          if (avcodec_parameters_to_context (p->video_ctx, p->video_stream->codecpar) < 0)
            {
              fprintf (stderr, "cannot copy video codec parameters\n");
              return;
            }
          p->video_ctx->err_recognition = AV_EF_IGNORE_ERR |
                                                    AV_EF_BITSTREAM |
                                                    AV_EF_BUFFER;
          p->video_ctx->workaround_bugs = FF_BUG_AUTODETECT;


          if (avcodec_open2 (p->video_ctx, p->video_codec, NULL) < 0)
          {
            g_warning ("error opening codec %s", p->video_ctx->codec->name);
            return;
          }
        }

      if (p->audio_stream)
        {
          p->audio_codec = avcodec_find_decoder (p->audio_stream->codecpar->codec_id);
          if (p->audio_codec == NULL)
            {
              g_warning ("audio codec not found");
              p->audio_ctx = NULL;
              return;
            }
          p->audio_ctx = avcodec_alloc_context3 (p->audio_codec);
          if (avcodec_parameters_to_context (p->audio_ctx, p->audio_stream->codecpar) < 0)
            {
              fprintf (stderr, "cannot copy audio codec parameters\n");
              return;
            }
          if (avcodec_open2 (p->audio_ctx, p->audio_codec, NULL) < 0)
            {
              g_warning ("error opening codec %s", p->audio_ctx->codec->name);
            }
          else
            {
              o->audio_sample_rate = p->audio_stream->codecpar->sample_rate;
#if LIBAVCODEC_VERSION_MAJOR < 61
              o->audio_channels = MIN(p->audio_stream->codecpar->channels, GEGL_MAX_AUDIO_CHANNELS);
#else
              o->audio_channels = MIN(p->audio_stream->codecpar->ch_layout.nb_channels, GEGL_MAX_AUDIO_CHANNELS);
#endif
            }
        }

      if (p->video_stream)
        {
          p->width = p->video_stream->codecpar->width;
          p->height = p->video_stream->codecpar->height;
        }
      p->lavc_frame = av_frame_alloc ();

      g_free (o->video_codec);
      if (p->video_codec && p->video_codec->name)
        o->video_codec = g_strdup (p->video_codec->name);
      else
        o->video_codec = g_strdup ("");

      g_free (o->audio_codec);
      if (p->audio_codec && p->audio_codec->name)
        o->audio_codec = g_strdup (p->audio_codec->name);
      else
        o->audio_codec = g_strdup ("");

      g_free (p->loadedfilename);
      p->loadedfilename = g_strdup (o->path);
      p->prevframe = -1;
      p->a_prevframe = -1;

      if (p->video_stream)
        {
          o->frames = p->video_stream->nb_frames;
          o->frame_rate = av_q2d (av_guess_frame_rate (p->video_fcontext, p->video_stream, NULL));
          if (!o->frames)
           {
             /* this is a guesstimate of frame-count */
             o->frames = p->video_fcontext->duration * o->frame_rate / AV_TIME_BASE;
             /* make second guess for things like luxo */
             if (o->frames < 1)
               o->frames = 23;
           }
#if 0
           {
             int m ,h;
             int s = o->frames / o->frame_rate;
             m = s / 60;
             s -= m * 60;
             h = m / 60;
             m -= h * 60;
             fprintf (stdout, "duration: %02i:%02i:%02i\n", h, m, s);
           }
#endif
          p->codec_delay = 0;
#if 0
          p->codec_delay = p->video_ctx->delay;
          if (!strcmp (o->video_codec, "mpeg1video"))
            p->codec_delay = 1;
          else if (!strcmp (o->video_codec, "h264"))
            {
               if (strstr (p->loadedfilename, ".mp4") ||
                   strstr (p->loadedfilename, ".MP4"))
 /* XXX: too hacky, isn't there an avformat thing to use?,
 or perhaps we can measure this when decoding the first frame.
 */
                p->codec_delay = 1;
              else
                p->codec_delay = 0;
            }
#endif
        }
      else
        {
          o->frame_rate = 10;
          if (p->audio_stream)
            o->frames = p->audio_fcontext->duration * o->frame_rate / AV_TIME_BASE;
          if (o->frames < 1)
            o->frames = 23;
        }
      clear_audio_track (o);
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {0, 0, 320, 200};
  Priv *p = (Priv*)GEGL_PROPERTIES (operation)->user_data;
  result.width = p->width;
  result.height = p->height;
  return result;
}

static int
samples_per_frame (int    frame,           /* frame no    */
                   double frame_rate,      /* frame rate  */
                   int    sample_rate,     /* sample rate */
                   int   *ceiled,          /* rounded up */
                   long  *start)           /* */
{
  double osamples;
  double samples = 0;
  double samples_per_frame = sample_rate / frame_rate;

  if (fabs(fmod (sample_rate, frame_rate)) < 0.0001f)
  {
    if (start)
      *start = (samples_per_frame) * frame;
    if (ceiled)
      *ceiled = samples_per_frame;
    return samples_per_frame;
  }

  samples = samples_per_frame * frame;

  osamples = samples;
  samples += samples_per_frame;
  if (start)
    (*start) = ceil(osamples);
  if (ceiled)
    *ceiled = ceil(samples_per_frame);
  return ceil(samples)-ceil(osamples);
}

static void get_sample_data (Priv *p, long sample_no, float *left, float *right)
{
  int to_remove = 0;
  GList *l;
  if (sample_no < 0)
    return;
  for (l = p->audio_track; l; l = l->next)
  {
    GeglAudioFragment *af = l->data;
    int channels = gegl_audio_fragment_get_channels (af);
    int pos = gegl_audio_fragment_get_pos (af);
    int sample_count = gegl_audio_fragment_get_sample_count (af);
    if (sample_no > pos + sample_count)
    {
      to_remove ++;
    }

    if (pos <= sample_no &&
        sample_no < pos + sample_count)
      {
        int i = sample_no - pos;
        *left  = af->data[0][i];
        if (channels == 1)
          *right = af->data[0][i];
        else
          *right = af->data[1][i];

        if (to_remove)  /* consuming audiotrack */
        {
          again:
          for (l = p->audio_track; l; l = l->next)
          {
            GeglAudioFragment *af = l->data;
            int pos = gegl_audio_fragment_get_pos (af);
            int sample_count = gegl_audio_fragment_get_sample_count (af);
            if (sample_no > pos + sample_count)
            {
              p->audio_track = g_list_remove (p->audio_track, af);
              g_object_unref (af);
              goto again;
            }
          }
        }
        return;
      }
  }
  *left  = 0;
  *right = 0;
}

static AVFrame *
alloc_picture (int pix_fmt, int width, int height)
{
  AVFrame  *picture;
  uint8_t  *picture_buf;
  int       size;

  picture = av_frame_alloc ();
  if (!picture)
    return NULL;
  size = av_image_get_buffer_size (pix_fmt, width + 1, height + 1, 16);
  picture_buf = malloc (size);
  if (!picture_buf)
    {
      av_free (picture);
      return NULL;
    }
  av_image_fill_arrays (picture->data, picture->linesize, picture_buf, pix_fmt, width, height, 1);
  return picture;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  {
    if (o->path && p->video_fcontext && !decode_frame (operation, o->frame))
      {
        long sample_start = 0;

        if (p->audio_stream)
        {
          int sample_count;
          gegl_audio_fragment_set_sample_rate (o->audio, p->audio_stream->codecpar->sample_rate);
          gegl_audio_fragment_set_channels    (o->audio, 2);
          gegl_audio_fragment_set_channel_layout    (o->audio, GEGL_CH_LAYOUT_STEREO);
          samples_per_frame (o->frame,
               o->frame_rate, p->audio_stream->codecpar->sample_rate,
               &sample_count,
               &sample_start);

          gegl_audio_fragment_set_sample_count (o->audio, sample_count);

          if (p->video_stream != NULL)
          {
            /* if we got video stream
               request audio to be decoded between prevpts and 5s into future*/
            decode_audio (operation, p->prevpts, p->prevpts + 5.0);

          }
          else
          {
            decode_audio (operation, o->frame / o->frame_rate, o->frame / o->frame_rate + 5);
          }

          {
            int i;
            for (i = 0; i < sample_count; i++)
            {
              get_sample_data (p, sample_start + i, &o->audio->data[0][i],
                                  &o->audio->data[1][i]);
            }
          }
        }

        if (p->video_stream == NULL)
          return TRUE;

        if (p->video_ctx->pix_fmt == AV_PIX_FMT_RGB24)
        {
          GeglRectangle extent = {0,0,p->width,p->height};
          gegl_buffer_set (output, &extent, 0, babl_format("R'G'B' u8"), p->lavc_frame->data[0], GEGL_AUTO_ROWSTRIDE);
        }
        else
        {
          struct SwsContext *img_convert_ctx;
          GeglRectangle extent = {0,0,p->width,p->height};

          img_convert_ctx = sws_getContext(p->width, p->height, p->video_ctx->pix_fmt,
                                           p->width, p->height, AV_PIX_FMT_RGB24,
                                           SWS_BICUBIC, NULL, NULL, NULL);
          if (!p->rgb_frame)
            p->rgb_frame = alloc_picture (AV_PIX_FMT_RGB24, p->width, p->height);
          sws_scale (img_convert_ctx, (void*)p->lavc_frame->data,
                     p->lavc_frame->linesize, 0, p->height, p->rgb_frame->data, p->rgb_frame->linesize);
          gegl_buffer_set (output, &extent, 0, babl_format("R'G'B' u8"), p->rgb_frame->data[0], GEGL_AUTO_ROWSTRIDE);
          sws_freeContext (img_convert_ctx);
        }
      }
  }
  return TRUE;
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;
      ff_cleanup (o);
      g_free (p->loadedfilename);

      g_clear_pointer (&o->user_data, g_free);
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:ff-load",
    "title",        _("FFmpeg Frame Loader"),
    "categories"  , "input:video",
    "description" , _("FFmpeg video frame importer."),
    NULL);
}

#endif

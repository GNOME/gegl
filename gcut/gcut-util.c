#include "config.h"
#include <gegl.h>
#include <gegl-audio-fragment.h>
#include "gcut.h"
#if HAVE_GEXIV2
#include <gexiv2/gexiv2.h>
#endif

void gcut_get_video_info (const char *path,
                          int        *frames,
                          double     *duration,
                          double     *fps)
{
  GeglNode *gegl = gegl_node_new ();
  GeglNode *probe = gegl_node_new_child (gegl, "operation",
                                         "gegl:ff-load", "path", path, NULL);
  double r_fps;
  int    r_frames;
  gegl_node_process (probe);

  gegl_node_get (probe, "frames", &r_frames, NULL);
  gegl_node_get (probe, "frame-rate", &r_fps, NULL);

  if (frames)
    *frames = r_frames;
  if (fps)
    *fps = r_fps;
  if (duration)
    *duration = r_frames / r_fps;
  g_object_unref (gegl);
}


void
gegl_meta_set_audio (const char        *path,
                     GeglAudioFragment *audio)
{
#if HAVE_GEXIV2
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (error)
  {
    g_warning ("%s", error->message);
  }
  else
  {
    int i, c;
    GString *str = g_string_new ("");
    int sample_count = gegl_audio_fragment_get_sample_count (audio);
    int channels = gegl_audio_fragment_get_channels (audio);
    if (gexiv2_metadata_has_tag (e2m, "Xmp.xmp.GEGL"))
      gexiv2_metadata_clear_tag (e2m, "Xmp.xmp.GEGL");

    g_string_append_printf (str, "%i %i %i %i",
                            gegl_audio_fragment_get_sample_rate (audio),
                            gegl_audio_fragment_get_channels (audio),
                            gegl_audio_fragment_get_channel_layout (audio),
                            gegl_audio_fragment_get_sample_count (audio));

    for (i = 0; i < sample_count; i++)
      for (c = 0; c < channels; c++)
        g_string_append_printf (str, " %0.5f", audio->data[c][i]);

    gexiv2_metadata_set_tag_string (e2m, "Xmp.xmp.GeglAudio", str->str);
    gexiv2_metadata_save_file (e2m, path, &error);
    if (error)
      g_warning ("%s", error->message);
    g_string_free (str, TRUE);
  }
  g_object_unref (e2m);
#endif
}

void
gegl_meta_get_audio (const char        *path,
                     GeglAudioFragment *audio)
{
#if HAVE_GEXIV2
  GError *error = NULL;
  GExiv2Metadata *e2m = gexiv2_metadata_new ();
  gexiv2_metadata_open_path (e2m, path, &error);
  if (!error)
  {
    GString *word = g_string_new ("");
    gchar *p;
    gchar *ret = gexiv2_metadata_get_tag_string (e2m, "Xmp.xmp.GeglAudio");
    int element_no = 0;
    int channels = 2;
    int max_samples = 2000;

    if (ret)
    for (p = ret; p==ret || p[-1] != '\0'; p++)
    {
      switch (p[0])
      {
        case '\0':case ' ':
        if (word->len > 0)
         {
           switch (element_no++)
           {
             case 0:
               gegl_audio_fragment_set_sample_rate (audio, g_strtod (word->str, NULL));
               break;
             case 1:
               channels = g_strtod (word->str, NULL);
               gegl_audio_fragment_set_channels (audio, channels);
               break;
             case 2:
               gegl_audio_fragment_set_channel_layout (audio, g_strtod (word->str, NULL));
               break;
             case 3:
               gegl_audio_fragment_set_sample_count (audio, g_strtod (word->str, NULL));
               break;
             default:
               {
                 int sample_no = element_no - 4;
                 int channel_no = sample_no % channels;
                 sample_no/=2;
                 if (sample_no < max_samples)
                 audio->data[channel_no][sample_no] = g_strtod (word->str, NULL);
               }
               break;
           }
         }
        g_string_assign (word, "");
        break;
        default:
        g_string_append_c (word, p[0]);
        break;
      }
    }
    g_string_free (word, TRUE);
    g_free (ret);
  }
  else
    g_warning ("%s", error->message);
  g_object_unref (e2m);
#endif
}


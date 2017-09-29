/*
 * Copyright (c) 2015 Rituwall Inc, authored by pippin@gimp.org
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define main iconographer_main

#include <gegl.h>
#include <gegl-audio-fragment.h>
#include <glib/gprintf.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define NEGL_RGB_HEIGHT     42
#define NEGL_RGB_THEIGHT    42
#define NEGL_RGB_HIST_DIM    6      // if changing make dim * dim * dim divisible by 3
#define NEGL_RGB_HIST_SLOTS (NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM)
#define NEGL_FFT_DIM        64

/* each row in the video terrain is the following 8bit RGB (24bit) data: */
typedef struct FrameInfo  
{
  uint8_t rgb_hist[NEGL_RGB_HIST_SLOTS];
  uint8_t rgb_square_diff[3];
  uint8_t audio_energy[3];
} FrameInfo;

char *format="histogram diff audio 4 thumb 64 mid-col 20";

int frame_start = 0;
int frame_end   = 0;
int total_frames = 0;
double frame_rate = 0; 

char *video_path = NULL;
char *thumb_path = NULL;
char *input_analysis_path = NULL;
char *output_analysis_path = NULL;
int   show_progress = 0;
int   frame_thumb = 0;
int   horizontal = 0;
int   time_out = 0;

long  babl_ticks (void);

static void  usage(void)
{
      printf ("usage: iconographer [options] <video> [thumb]\n"
" -p, --progress   - show /progress in terminal\n"
" -a <analysis-path>, ---analysis\n"
"                  - path to store information extraction result, if the file\n"
"                    already exists it will be reused for best frame analysis\n"
"                    instead of a full dump happening again.\n"
" -h, --horizontal   store a horizontal strata instead of vertical\n"
" -t, --timeout - stop doing frame info dump after this many seconds have passed)\n"
/*" -s <frame>, --start-frame <frame>\n"
"           - first frame to extract analysis from (default 0)\n"*/
" -e <frame>, --end-frame <frame>\n"
"           - last frame to extract analysis from (default is 0 which means auto end)\n"
" -f, --format - format string, specify which forms of analysis to put in the analysis file,\n"
"                the default format is: \"histogram audio thumb 40 mid-col 20\"\n"
"\n"
"\n"
"Options can also follow the video (and thumb) arguments.\n"
"\n");
  exit (0);
}

static void parse_args (int argc, char **argv)
{
  int i;
  int stage = 0;
  for (i = 1; i < argc; i++)
  {
    if (g_str_equal (argv[i], "-f")||
        g_str_equal (argv[i], "--format"))
    {
      format = g_strdup (argv[i+1]);
      i++;
    } 
    else if (g_str_equal (argv[i], "-p") ||
        g_str_equal (argv[i], "--progress"))
    {
      show_progress = 1;
    }
    else if (g_str_equal (argv[i], "-h") ||
        g_str_equal (argv[i], "--horizontal"))
    {
      horizontal = 1;
    }
    else if (g_str_equal (argv[i], "-v") ||
        g_str_equal (argv[i], "--vertical"))
    {
      horizontal = 0;
    }
    else if (g_str_equal (argv[i], "-a") ||
        g_str_equal (argv[i], "--analysis"))
    {
      input_analysis_path = g_strdup (argv[i+1]);
      output_analysis_path = g_strdup (argv[i+1]);
      i++;
    } 

    else if (g_str_equal (argv[i], "-s") ||
             g_str_equal (argv[i], "--start-frame"))
    {
      frame_start = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-t") ||
             g_str_equal (argv[i], "--time-out"))
    {
      time_out = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "-e") ||
             g_str_equal (argv[i], "--end-frame"))
    {
      frame_end = g_strtod (argv[i+1], NULL);
      i++;
    } 
    else if (g_str_equal (argv[i], "--help"))
    {
      usage();
    }
    else if (stage == 0)
    {
      video_path = g_strdup (argv[i]);
      stage = 1;
    } else if (stage == 1)
    {
      thumb_path = g_strdup (argv[i]);
      stage = 2;
    }
  }
}

#include <string.h>

GeglNode   *gegl_decode  = NULL;

GeglBuffer *previous_video_frame  = NULL;
GeglBuffer *video_frame  = NULL;
GeglBuffer *terrain      = NULL;

uint8_t rgb_hist_shuffler[NEGL_RGB_HIST_SLOTS];
uint8_t rgb_hist_unshuffler[NEGL_RGB_HIST_SLOTS];

typedef struct {
  int r;
  int g;
  int b;
  int no; 
} Entry;

static int rgb_hist_inited = 0;

static gint sorted_color (gconstpointer a, gconstpointer b)
{
  const Entry *ea = a;
  const Entry *eb = b;
  return (ea->g * 110011 + ea->r * 213 + ea->b) -
         (eb->g * 110011 + eb->r * 213 + eb->b);
}

static inline void init_rgb_hist (void)
{
  /* sort RGB histogram slots by luminance for human readability
   */
  if (!rgb_hist_inited)
    {
      GList *list = NULL;
      GList *l;
      int r, g, b, i;
      i = 0;
      for (r = 0; r < NEGL_RGB_HIST_DIM; r++)
        for (g = 0; g < NEGL_RGB_HIST_DIM; g++)
          for (b = 0; b < NEGL_RGB_HIST_DIM; b++, i++)
          {
            Entry *e = g_malloc0 (sizeof (Entry));
            e->r = r;
            e->g = g;
            e->b = b;
            e->no = i;
            list = g_list_prepend (list, e);
          }
      list = g_list_sort (list, sorted_color);
      i = 0;
      for (l = list; l; l = l->next, i++)
      {
        Entry *e = l->data;
        int no = e->no;
        int li = i;
        rgb_hist_shuffler[li] = no;
        rgb_hist_unshuffler[no] = li;
        g_free (e);
      }
      g_list_free (list);
      rgb_hist_inited = 1;
    }
}

int rgb_hist_shuffle (int in);
int rgb_hist_shuffle (int in)
{
  init_rgb_hist();
  return rgb_hist_unshuffler[in];
}


int rgb_hist_unshuffle (int in);
int rgb_hist_unshuffle (int in)
{
  init_rgb_hist();
  return rgb_hist_unshuffler[in];
}

GeglNode *store, *load;

static void decode_frame_no (int frame)
{
  if (video_frame)
  {
    if (strstr (format, "histogram"))
    {
       if (previous_video_frame)
         g_object_unref (previous_video_frame);
       previous_video_frame = gegl_buffer_dup (video_frame);
    }
    g_object_unref (video_frame);
  }
  video_frame = NULL;
  gegl_node_set (load, "frame", frame, NULL);
  gegl_node_process (store);
}

static int count_color_bins (FrameInfo *info, int threshold)
{
  int count = 0;
  int i;
  for (i = 0; i < NEGL_RGB_HIST_SLOTS; i++)
    if (info->rgb_hist[i] > threshold)
      count ++;

  return count;
}

static float score_frame (FrameInfo *info, int frame_no)
{
  float sum_score             = 0.0;
  float rgb_histogram_count   = count_color_bins (info, 1) * 1.0 / NEGL_RGB_HIST_SLOTS;
  float audio_energy          = info->audio_energy[1] / 255.0;
  float new_scene             = (info->rgb_square_diff[0] / 255.0 +
                                info->rgb_square_diff[1] / 255.0 +
                                info->rgb_square_diff[2] / 255.0) * 3;
  float after_first_40_sec    = frame_no / frame_rate > 40.0 ? 1.0 : 0.3;
  float after_first_12_sec    = frame_no / frame_rate > 12.0 ? 1.0 : 0.1;
  float within_first_third    = frame_no < total_frames / 3  ? 1 : 0.6;

  sum_score = rgb_histogram_count;
  sum_score *= within_first_third  * 0.33;
  sum_score *= after_first_40_sec  * 0.33;
  sum_score *= after_first_12_sec  * 0.33;
  sum_score *= (audio_energy       + 0.1) * 0.7;
  sum_score *= (new_scene          + 0.05);
  return sum_score;
}

static void find_best_thumb (void)
{
  int frame = 0;
  float best_score = 0.0;
  frame_thumb = 0;
  
  for (frame = 0; frame < frame_end; frame++)
  {
    FrameInfo info;
    float score;
    GeglRectangle terrain_row;
    if (horizontal)
      terrain_row = (GeglRectangle){frame-frame_start, 0, 1, sizeof (FrameInfo)};
    else
      terrain_row = (GeglRectangle){0, frame-frame_start, sizeof (FrameInfo), 1};
    gegl_buffer_get (terrain, &terrain_row, 1.0, babl_format("RGB u8"),
                     &info, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
    score = score_frame (&info, frame);
    if (score > best_score)
      {
        best_score = score;
        frame_thumb = frame;
      }
  }
  fprintf (stderr, "best frame: %i\n", frame_thumb);
}

GeglNode *translate = NULL;

static int extract_mid_col (GeglBuffer *buffer, void *rgb_mid_col, int samples)
{
  GeglRectangle mid_col;
  mid_col.x = 0;
  mid_col.y = 
     gegl_buffer_get_extent (buffer)-> height * 1.0 *
      samples / gegl_buffer_get_extent (buffer)->width / 2.0;
  mid_col.height = 1;
  mid_col.width = samples;
  gegl_buffer_get (video_frame, &mid_col, 
     1.0 * samples / gegl_buffer_get_extent (buffer)->width,
     babl_format ("R'G'B' u8"),
     rgb_mid_col,
     GEGL_AUTO_ROWSTRIDE,
     GEGL_ABYSS_NONE);
  return 3 * samples;
}

static int extract_thumb (GeglBuffer *buffer, void *rgb_thumb, int samples, int samples2)
{
  GeglRectangle thumb_scan;
  static float vpos = 0.0;
  if (horizontal)
  {
    thumb_scan.y = 0;
    thumb_scan.x = 
       gegl_buffer_get_extent (buffer)-> width * 1.0 *
       samples / gegl_buffer_get_extent (buffer)->width * vpos;
    vpos += (1.0/samples2);
    if (vpos > 1.0) vpos = 0.0;
    thumb_scan.width = 1;
    thumb_scan.height = samples;
  }
  else
  {
    thumb_scan.x = 0;
    thumb_scan.y = 
       gegl_buffer_get_extent (buffer)-> height * 1.0 *
       samples / gegl_buffer_get_extent (buffer)->width * vpos;
    vpos += (1.0/samples2);
    if (vpos > 1.0) vpos = 0.0;
    thumb_scan.height = 1;
    thumb_scan.width = samples;
  }
  gegl_buffer_get (buffer, &thumb_scan, 
     horizontal?1.0 * samples/ gegl_buffer_get_extent (buffer)->height:
                1.0 * samples/ gegl_buffer_get_extent (buffer)->width,
     babl_format ("R'G'B' u8"),
     (void*)rgb_thumb,
     //(void*)&(info->rgb_thumb)[0],
     GEGL_AUTO_ROWSTRIDE,
     GEGL_ABYSS_NONE);
  return 3 * samples;
}


static int extract_audio_energy (GeglAudioFragment *audio, uint8_t *audio_energy, int dups)
{
  int i;
  float left_max = 0;
  float right_max = 0;
  float left_sum = 0;
  float right_sum = 0;
  int sample_count = gegl_audio_fragment_get_sample_count (audio);
  for (i = 0; i < sample_count; i++)
  {
    left_sum += fabs (audio->data[0][i]);
    right_sum += fabs (audio->data[1][i]);
    if (fabs (audio->data[0][i]) > left_max)
      left_max = fabs (audio->data[0][i]);
    if (fabs (audio->data[1][i]) > right_max)
      right_max = fabs (audio->data[1][i]);
  }
  left_sum /= sample_count;
  right_sum /= sample_count;
  left_max = left_sum;
  right_max = right_sum;
  
  left_max *= 255;
  if (left_max > 255)
    left_max = 255;         
  right_max *= 255;
  if (right_max > 255)
    right_max = 255;         

  for (i = 0; i < dups; i ++)
  {
    audio_energy[3*i+0] = left_max;
    audio_energy[3*i+1] = (left_max+right_max)/2;
    audio_energy[3*i+2] = right_max;
  }
  return 3 * dups;
}

static int extract_mid_row (GeglBuffer *buffer, void *rgb_mid_row, int samples)
{
  GeglRectangle mid_row;
  mid_row.width = 1;
  mid_row.height = samples;
  mid_row.x = 
     gegl_buffer_get_extent (buffer)-> width * 1.0 *
      samples / gegl_buffer_get_extent (buffer)->height / 2.0;
  mid_row.y = 0;
  gegl_buffer_get (buffer, &mid_row, 
    1.0 * samples / gegl_buffer_get_extent (buffer)->height,
    babl_format ("R'G'B' u8"),
    rgb_mid_row,
    GEGL_AUTO_ROWSTRIDE,
    GEGL_ABYSS_NONE);
  return 3 * samples;
}

static void record_pix_stats (GeglBuffer *buffer, GeglBuffer *previous_buffer,
         uint8_t *info_rgb_hist,
         uint8_t *rgb_square_diff)
{
  int rgb_hist[NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM]={0,};
  int sum = 0;
  int second_max_hist = 0;
  int max_hist = 0;
  int slot;
  long r_square_diff = 0;
  long g_square_diff = 0;
  long b_square_diff = 0;
  GeglBufferIterator *it = gegl_buffer_iterator_new (buffer, NULL, 0,
          babl_format ("R'G'B' u8"),
          GEGL_BUFFER_READ,
          GEGL_ABYSS_NONE);
  if (previous_video_frame)
    gegl_buffer_iterator_add (it, previous_buffer, NULL, 0,
          babl_format ("R'G'B' u8"),
          GEGL_BUFFER_READ,
          GEGL_ABYSS_NONE);

  for (slot = 0; slot < NEGL_RGB_HIST_SLOTS; slot ++)
    rgb_hist[slot] = 0;

  while (gegl_buffer_iterator_next (it))
  {
    uint8_t *data = (void*)it->data[0];
    int i;
    if (strstr (format, "histogram"))
    {
      for (i = 0; i < it->length; i++)
      {
        int r = data[i * 3 + 0] / 256.0 * NEGL_RGB_HIST_DIM;
        int g = data[i * 3 + 1] / 256.0 * NEGL_RGB_HIST_DIM;
        int b = data[i * 3 + 2] / 256.0 * NEGL_RGB_HIST_DIM;
        int slot = r * NEGL_RGB_HIST_DIM * NEGL_RGB_HIST_DIM +
                   g * NEGL_RGB_HIST_DIM +
                   b;
        if (slot < 0) slot = 0;
        if (slot >= NEGL_RGB_HIST_SLOTS)
            slot = NEGL_RGB_HIST_SLOTS;

        rgb_hist[slot]++;
        if (rgb_hist[slot] > max_hist)
        {
          second_max_hist = max_hist;
          max_hist = rgb_hist[slot];
        }
        sum++;
      }
    }
  
      if (previous_buffer)
      {
        uint8_t *data_prev = (void*)it->data[1];
        int i;
        for (i = 0; i < it->length; i++)
        {
  #define square(a) ((a)*(a))
          r_square_diff += square(data[i * 3 + 0] -data_prev [i * 3 + 0]);
          g_square_diff += square(data[i * 3 + 1] -data_prev [i * 3 + 1]);
          b_square_diff += square(data[i * 3 + 2] -data_prev [i * 3 + 2]);
        }
      }
  }

  if (strstr(format, "histogram"))
  {
   int slot;
   for (slot = 0; slot < NEGL_RGB_HIST_SLOTS; slot ++)
   {
     int val = (sqrtf(rgb_hist[slot]) / (sqrtf(second_max_hist) * 0.9 + sqrtf(max_hist) * 0.1)) * 255;
     if (val > 255)
       val = 255; 
     info_rgb_hist[rgb_hist_shuffle (slot)] = val;
    }
  }
  if (previous_buffer)
  {
    rgb_square_diff[0] = sqrt(r_square_diff) * 255 / sum;
    rgb_square_diff[1] = sqrt(g_square_diff) * 255 / sum;
    rgb_square_diff[2] = sqrt(b_square_diff) * 255 / sum;
  }
}

gint iconographer_main (gint    argc, gchar **argv);

gint
main (gint    argc,
      gchar **argv)
{
  GeglRectangle terrain_rect;
  if (argc < 2)
    usage();

  gegl_init (&argc, &argv);
  parse_args (argc, argv);

  gegl_decode = gegl_node_new ();

  store = gegl_node_new_child (gegl_decode,
                               "operation", "gegl:buffer-sink",
                               "buffer", &video_frame, NULL);
  load = gegl_node_new_child (gegl_decode,
                              "operation", "gegl:ff-load",
                              "frame", 0,
                              "path", video_path,
                              NULL);
  gegl_node_link_many (load, store, NULL);

  decode_frame_no (0); /* we issue a processing/decoding of a frame - to get metadata */
  {
    gegl_node_get (load, "frame-rate", &frame_rate, NULL);
    total_frames = 0; gegl_node_get (load, "frames", &total_frames, NULL);
    
    if (frame_end == 0)
      frame_end = total_frames;
  }

  if (horizontal)
   terrain_rect = (GeglRectangle){0, 0,
                                frame_end - frame_start + 1,
                                1024};
  else
   terrain_rect = (GeglRectangle){0, 0,
                                1024,
                                frame_end - frame_start + 1};

  if (input_analysis_path && g_file_test (input_analysis_path, G_FILE_TEST_IS_REGULAR))
  {
    GeglNode *load_graph = gegl_node_new ();
    GeglNode *load = gegl_node_new_child (load_graph, "operation", "gegl:load", "path", input_analysis_path, NULL);
    GeglNode *store = gegl_node_new_child (load_graph, "operation",
                                           "gegl:buffer-sink",
                                           "buffer", &terrain, NULL);
    gegl_node_link_many (load, store, NULL);
    gegl_node_process (store);
    g_object_unref (load_graph);

    frame_end = frame_start + gegl_buffer_get_extent (terrain)->height;
    /* the last frame aavilavle for analysis is the last one loaded rom cache,.. perhaps with some timeout */
  }
  else
  {
    terrain = gegl_buffer_new (&terrain_rect, babl_format ("R'G'B' u8"));
    {
      gint frame;
      gint max_buf_pos = 0;
      for (frame = frame_start; frame <= frame_end; frame++)
        {
          FrameInfo info = {{0}};
          uint8_t buffer[4096] = {0,};
          int buffer_pos = 0;
          GeglRectangle terrain_row;
          char *p = format;
          GString *word = g_string_new ("");

          if (show_progress)
          {
            double percent_full = 100.0 * (frame-frame_start) / (frame_end-frame_start);
            double percent_time = time_out?100.0 * babl_ticks()/1000.0/1000.0 / time_out:0.0;
            fprintf (stdout, "\r%2.1f%% %i/%i (%i)", 
                     percent_full>percent_time?percent_full:percent_time,
                     frame-frame_start,
                     frame_end-frame_start,
                     frame);
            fflush (stdout);
          }

          if (horizontal)
            terrain_row = (GeglRectangle){frame-frame_start, 0, 1, 1024};
          else
            terrain_row = (GeglRectangle){0, frame-frame_start, 1024, 1};

          decode_frame_no (frame);

          //for (int i=0;i<(signed)sizeof(buffer);i++)buffer[i]=0;

          while (*p == ' ') p++;
          for (p= format;p==format || p[-1]!='\0';p++)
          {
            if (*p != '\0' && *p != ' ')
              {
                g_string_append_c (word, *p);
              }
            else
              {
               if (!strcmp (word->str, "histogram"))
               {
                 record_pix_stats (video_frame, previous_video_frame,
                                   &(info.rgb_hist[0]),
                                   &(info.rgb_square_diff)[0]);
                 for (int i = 0; i < NEGL_RGB_HIST_SLOTS; i++)
                 {
                  buffer[buffer_pos] = info.rgb_hist[i];
                  buffer_pos++;
                 }
                 for (int i = 0; i < 3; i++)
                 {
                   buffer[buffer_pos] = info.rgb_square_diff[i];
                   buffer_pos++;
                 }
               }
               else if (!strcmp (word->str, "mid-row"))
               {
                  int samples = NEGL_RGB_HEIGHT;
                  if (p[1] >= '0' && p[1] <= '9')
                  {
                    samples = g_strtod (&p[1], &p);
                  }
                  buffer_pos += extract_mid_row (video_frame, &(buffer)[buffer_pos], samples);
               }
               else if (!strcmp (word->str, "mid-col"))
               {
                  int samples = NEGL_RGB_HEIGHT;
                  if (p[1] >= '0' && p[1] <= '9')
                  {
                    samples = g_strtod (&p[1], &p);
                  }
                  buffer_pos += extract_mid_col (video_frame, &(buffer)[buffer_pos], samples);
               }
               else if (!strcmp (word->str, "thumb"))
               {
                  int samples  = NEGL_RGB_THEIGHT;
                  int samples2;

                  if (p[1] >= '0' && p[1] <= '9')
                  {
                    samples = g_strtod (&p[1], &p);
                  }

                  if (horizontal)
                    samples2 = samples * gegl_buffer_get_width (video_frame)/gegl_buffer_get_height(video_frame);
                  else
                    samples2 = samples * gegl_buffer_get_height (video_frame)/gegl_buffer_get_width(video_frame);

                  buffer_pos += extract_thumb (video_frame, &(buffer)[buffer_pos], samples, samples2);
               }
               else if (!strcmp (word->str, "audio"))
               {
                  int dups = 1;
                  GeglAudioFragment *audio = NULL;

                  if (p[1] >= '0' && p[1] <= '9')
                  {
                    dups = g_strtod (&p[1], &p);
                  }
                 gegl_node_get (load, "audio", &audio, NULL);
                 if (audio)
                  {
                    extract_audio_energy (audio, &buffer[buffer_pos], dups);
                    g_object_unref (audio);
                  }
                 buffer_pos+=3 * dups;
               }
               g_string_assign (word, "");
            }
          }
          max_buf_pos = buffer_pos;
          g_string_free (word, TRUE);

          gegl_buffer_set (terrain, &terrain_row, 0, babl_format("RGB u8"),
                           buffer,
                           GEGL_AUTO_ROWSTRIDE);

          if (time_out > 1.0 &&
              babl_ticks()/1000.0/1000.0 > time_out)
            {
               frame_end = frame;
               if (horizontal)
                 terrain_rect.width = frame_end - frame_start + 1;
               else
                 terrain_rect.height = frame_end - frame_start + 1;
          //     gegl_buffer_set_extent (terrain, &terrain_rect);
            }

          if (horizontal)
            terrain_rect.height = max_buf_pos/3;
          else
            terrain_rect.width = max_buf_pos/3;
          gegl_buffer_set_extent (terrain, &terrain_rect);
        }
        if (show_progress)
        {
          fprintf (stdout, "\n");
          fflush (stdout);
        }
    }

    if (output_analysis_path)
    {
      GeglNode *save_graph = gegl_node_new ();
      GeglNode *readbuf = gegl_node_new_child (save_graph, "operation", "gegl:buffer-source", "buffer", terrain, NULL);
      GeglNode *save = gegl_node_new_child (save_graph, "operation", "gegl:png-save",
        "path", output_analysis_path, NULL);
        gegl_node_link_many (readbuf, save, NULL);
      gegl_node_process (save);
      g_object_unref (save_graph);
    }
  }
  
  if (thumb_path)
  {
    GeglNode *save_graph = gegl_node_new ();
    find_best_thumb ();
    if (frame_thumb != 0)
      decode_frame_no (frame_thumb-1);
    decode_frame_no (frame_thumb);
    {
    GeglNode *readbuf = gegl_node_new_child (save_graph, "operation", "gegl:buffer-source", "buffer", video_frame, NULL);
    GeglNode *save = gegl_node_new_child (save_graph, "operation", "gegl:png-save",
      "path", thumb_path, NULL);
      gegl_node_link_many (readbuf, save, NULL);
    gegl_node_process (save);
    g_object_unref (save_graph);
    }
  }

  if (video_frame)
    g_object_unref (video_frame);
  video_frame = NULL;
  if (previous_video_frame)
    g_object_unref (previous_video_frame);
  previous_video_frame = NULL;
  if (terrain)
    g_object_unref (terrain);
  terrain = NULL;
  g_object_unref (gegl_decode);

  gegl_exit ();

  return 0;
}

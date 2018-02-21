/* pixel-duster
 * a context aware pixel inpainting algorithm
 * 2018 (c) Øyvind Kolås pippin@gimp.org
 */

/*
    todo:
         median/mean for noise redux
         exclude identicals - when it is obvious
 */

#define POW2(x) ((x)*(x))

#define INITIAL_SCORE 1200000000

typedef struct
{
  GeglOperation *op;
  GeglBuffer    *input;
  GeglBuffer    *output;
  GeglRectangle  in_rect;
  GeglRectangle  out_rect;
  int            seek_radius;
  int            minimum_neighbors;
  int            minimum_iterations;
  float          try_chance;
  float          retry_chance;
  float          scale_x;
  float          scale_y;

  GHashTable    *ht[4096];

  GHashTable    *probes_ht;

  int            order[512][3];
} PixelDuster;

typedef struct Probe {
  int            target_x;
  int            target_y;
  int            score;
  int            age;
  int            source_x;
  int            source_y;
} Probe;

/* used for hash-table keys */
#define xy2offset(x,y)   GINT_TO_POINTER(((y) * 65536 + (x)))

#define NEIGHBORHOOD 23

/* when going through the image preparing the index - only look at the subset
 * needed pixels - and later when fetching out hashed pixels - investigate
 * these ones in particular. would only be win for limited inpainting..
 */


static void init_order(PixelDuster *duster)
{
  int i, x, y;
#if 0
  int order_2d[][15]={
                 {  0,  0,  0,  0,  0,142,111,112,126,128,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,124,110, 86, 87, 88,113,129,143,  0,  0,  0},
                 {  0,  0,  0,125,109, 85, 58, 59, 60, 89, 90,114,  0,  0,  0},
                 {  0,  0,  0,108, 78, 57, 38, 39, 40, 61, 79,115,130,  0,  0},
                 {  0,  0,107, 84, 76, 37, 26, 21, 27, 41, 62, 91,116,  0,  0},
                 {  0,153,106, 77, 56, 25, 17,  9, 13, 28, 42, 62, 92,152,  0},
                 {  0,149,105, 55, 36, 16,  8,  1,  5, 18, 29, 43, 63,150,  0},
                 {  0,145, 75, 54, 24, 12,  4,  0,  2, 10, 22, 44, 64,147,  0},
                 {  0,146,104, 53, 35, 20,  7,  3,  6, 14, 30, 45, 65,148,  0},
                 {  0,154,103, 74, 52, 34, 15, 11, 19, 31, 46, 66, 93,152,  0},
                 {  0,156,102, 83, 73, 51, 33, 23, 32, 47, 67, 80,117,158,  0},
                 {  0,  0,123,101, 82, 72, 50, 49, 48, 68, 81, 94,131,159,  0},
                 {  0,  0,  0,122,100, 99, 71, 70, 69, 95,118,132,140,  0,  0},
                 {  0,  0,  0,  0,121,120, 98, 97, 96,119,133,139,  0,  0,  0},
                 {  0,  0,  0,  0,144,138,136,134,135,137,141,  0,  0,  0,  0},
               };
#else
  int order_2d[][15]={
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 21,  0,  0, 22,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0, 17,  0,  0,  0, 18,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  9,  0, 13,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 16,  8,  1,  5,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  4,  0,  2, 10,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 12,  7,  3,  6,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0, 11,  0,  0, 23,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0, 15,  0,  0, 14,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0, 20,  0,  0, 19,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
                 {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
               };
#endif

  duster->order[0][0] = 0;
  duster->order[0][1] = 0;
  duster->order[0][2] = 1;

  for (i = 1; i < 159; i++)
  for (y = -7; y <= 7; y ++)
    for (x = -7; x <= 7; x ++)
      {
        if (order_2d[x+7][y+7] == i)
        {
          duster->order[i][0] = x;
          duster->order[i][1] = y;
          duster->order[i][2] = POW2(x)+POW2(y);
        }
      }
}

static PixelDuster * pixel_duster_new (GeglBuffer *input,
                                       GeglBuffer *output,
                                       const GeglRectangle *in_rect,
                                       const GeglRectangle *out_rect,
                                       int         seek_radius,
                                       int         minimum_neighbors,
                                       int         minimum_iterations,
                                       float       try_chance,
                                       float       retry_chance,
                                       float       scale_x,
                                       float       scale_y,
                                       GeglOperation *op)
{
  PixelDuster *ret = g_malloc0 (sizeof (PixelDuster));
  ret->input = input;
  ret->output = output;
  ret->seek_radius = seek_radius;
  ret->minimum_neighbors = minimum_neighbors;
  ret->minimum_iterations = minimum_iterations;
  ret->try_chance = try_chance;
  ret->retry_chance = retry_chance;
  ret->op = op;
  ret->in_rect = *in_rect;
  ret->out_rect = *out_rect;
  ret->scale_x = scale_x;
  ret->scale_y = scale_y;
  for (int i = 0; i < 4096; i++)
    ret->ht[i] = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  ret->probes_ht = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  init_order (ret);
  return ret;
}

static inline void pixel_duster_remove_probes (PixelDuster *duster)
{
  if (duster->probes_ht)
  {
    g_hash_table_destroy (duster->probes_ht);
    duster->probes_ht = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, g_free);
  }
}

static void pixel_duster_destroy (PixelDuster *duster)
{
  pixel_duster_remove_probes (duster);
  for (int i = 0; i < 4096; i++)
  {
    if (g_hash_table_size (duster->ht[i]))
      fprintf (stderr, "%i:%i ", i, g_hash_table_size (duster->ht[i]));
    g_hash_table_destroy (duster->ht[i]);
  }
  fprintf (stderr, "\n");
  g_free (duster);
}

static void extract_site (PixelDuster *duster, GeglBuffer *input, int x, int y, guchar *dst)
{
  static const Babl *format = NULL;
  static const Babl *yformat = NULL;
  guchar lum[8];
  int bdir = 0;
  if (!format){
    format = babl_format ("R'G'B'A u8");
    yformat = babl_format ("Y'aA u8");
  }
#if 1
  /* figure out which of the up/down/left/right pixels are brightest,
     using premultiplied alpha - do punish blank spots  */

  gegl_buffer_sample (input,  x + 1, y + 0, NULL, &lum[0], yformat, GEGL_SAMPLER_NEAREST, 0);
  gegl_buffer_sample (input,  x - 1, y + 0, NULL, &lum[2], yformat, GEGL_SAMPLER_NEAREST, 0);
  gegl_buffer_sample (input,  x + 0, y + 1, NULL, &lum[3], yformat, GEGL_SAMPLER_NEAREST, 0);
  gegl_buffer_sample (input,  x + 0, y - 1, NULL, &lum[4], yformat, GEGL_SAMPLER_NEAREST, 0);

  for (int i = 1; i < 4; i++)
    if (lum[i*2] > bdir)
      bdir = i;
#endif
  /* and orient our neighborhood fetching based on that */
  switch (bdir)
  {
    case 0:
      for (int i = 0; i <= NEIGHBORHOOD; i++)
        gegl_buffer_sample (input,
                            x + duster->order[i][0],
                            y + duster->order[i][1],
                            NULL, &dst[i*4], format,
                            GEGL_SAMPLER_NEAREST, 0);
      break;
    case 1:
      for (int i = 0; i <= NEIGHBORHOOD; i++)
        gegl_buffer_sample (input,
                            x - duster->order[i][0],
                            y - duster->order[i][1],
                            NULL, &dst[i*4], format,
                            GEGL_SAMPLER_NEAREST, 0);
      break;
    case 2:
      for (int i = 0; i <= NEIGHBORHOOD; i++)
        gegl_buffer_sample (input,
                            x + duster->order[i][1],
                            y + duster->order[i][0],
                            NULL, &dst[i*4], format,
                            GEGL_SAMPLER_NEAREST, 0);
      break;
    case 3:
      for (int i = 0; i <= NEIGHBORHOOD; i++)
        gegl_buffer_sample (input,
                            x - duster->order[i][1],
                            y - duster->order[i][0],
                            NULL, &dst[i*4], format,
                            GEGL_SAMPLER_NEAREST, 0);
      break;

    break;
  }
}

static inline int u8_rgb_diff (guchar *a, guchar *b)
{
  return POW2(a[0]-b[0]) + POW2(a[1]-b[1]) * 2 + POW2(a[2]-b[2]);
}

static int inline
score_site (PixelDuster *duster,
            guchar      *needle,
            guchar      *hay,
            int          bail)
{
  int i;
  int score = 0;
  /* bail early with really bad score - the target site doesnt have opacity */

  if (hay[3] < 2)
  {
    return INITIAL_SCORE;
  }

  for (i = 1; i < NEIGHBORHOOD && score < bail; i++)
  {
    if (needle[i*4 + 3] && hay[i*4 + 3])
    {
      score += u8_rgb_diff (&needle[i*4 + 0], &hay[i*4 + 0]) * 10 / duster->order[i][2];
    }
    else
    {
      /* we score missing cells as if it is a big diff */
      score += ((POW2(36))*3) * 40 / duster->order[i][2];
    }
  }
  return score;
}

static Probe *
add_probe (PixelDuster *duster, int target_x, int target_y)
{
  Probe *probe    = g_malloc0 (sizeof (Probe));
  probe->target_x = target_x;
  probe->target_y = target_y;
  probe->source_x = 0;//target_x / duster->scale_x;
  probe->source_y = 0;//target_y / duster->scale_y;
  probe->score    = INITIAL_SCORE;
  g_hash_table_insert (duster->probes_ht,
                       xy2offset(target_x, target_y), probe);
  return probe;
}

static int
probe_rel_is_set (PixelDuster *duster, GeglBuffer *output, Probe *probe, int rel_x, int rel_y)
{
#if 1
  static const Babl *format = NULL;
  guchar pix[4];
  if (!format) format = babl_format ("R'G'B'A u8");
  gegl_buffer_sample (output, probe->target_x + rel_x, probe->target_y + rel_y, NULL, &pix[0], format, GEGL_SAMPLER_NEAREST, 0);
  return pix[3] > 5;
#else
  Probe *neighbor_probe = g_hash_table_lookup (duster->probes_ht,
        xy2offset(probe->target_x + rel_x, probe->target_y + rel_y));
  if (!neighbor_probe || neighbor_probe->age)
    return 1;
  return 0;
#endif
}

static int
probe_neighbors (PixelDuster *duster, GeglBuffer *output, Probe *probe, int min)
{
  int found = 0;
  if (probe_rel_is_set (duster, output, probe, -1, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  1, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, 1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, -1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  1, 1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -1,-1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  1,-1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -1, 1)) found ++;
  if (found >=min) return found;
#if 0
  if (probe_rel_is_set (duster, output, probe,  2, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, 2)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -2, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, -2)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -3, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  3, 0)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, 3)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  0, -3)) found ++;
#endif
  return found;
}

static inline int
spread_relative (PixelDuster *duster, Probe *probe, int dx, int dy)
{
  static const Babl *format = NULL;

  if (!format)
    format = babl_format ("RGBA float");

  /* spread our resulting neighborhodo to unset neighbors */
  if (!probe_rel_is_set (duster, duster->output, probe, dx, dy))
    {
      Probe *neighbor_probe = g_hash_table_lookup (duster->probes_ht,
              xy2offset(probe->target_x + dx, probe->target_y + dy));
      if (neighbor_probe)
        {
          gfloat rgba[4];
          gegl_buffer_sample (duster->input, probe->source_x + dx, probe->source_y + dy,  NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
          if (rgba[3] > 0.001)
           {
             neighbor_probe->source_x = probe->source_x + dx;
             neighbor_probe->source_y = probe->source_y + dy;
             neighbor_probe->score = INITIAL_SCORE - 1;
             gegl_buffer_set (duster->output,
                              GEGL_RECTANGLE(neighbor_probe->target_x,
                                             neighbor_probe->target_y, 1, 1),
                              0, format, &rgba[0], 0);
             return 1;
           }
        }
      }
  return 0;
}

static int site_subset (guchar *site)
{
  int a = (site[4  + 1]/16) +
          (site[8  + 1]/16) * 16 +
          (site[12 + 1]/16) * 16 * 16;
  return a;
}


static void site_subset2 (guchar *site, gint *min, gint *max)
{
  int v[3] = {(site[4  + 1]/4), (site[8  + 1]/4), (site[12 + 1]/4)};
  for (int i = 0; i < 3; i ++)
  {
    if (site[(4 + 4 *i)+3] == 0)
    {
       min[i] = 0;
       max[i] = 15;
    }
    else
    {
      switch (v[i] % 4)
      {
        case 0: min[i] = v[i]/4-1; max[i] = v[i]/4;
                if (min[i] < 0)  min[i] = 0;
        break;
        case 1: min[i] = v[i]/4; max[i] = v[i]/4;  break;
        case 2: min[i] = v[i]/4; max[i] = v[i]/4;  break;
        case 3: min[i] = v[i]/4; max[i] = v[i]/4+1;
          if (max[i] > 15) max[i] = 15;
        break;
      }
    }
  }
}


static guchar *ensure_hay (PixelDuster *duster, int x, int y, int subset)
{
  guchar *hay = NULL;

  if (subset >= 0)
    hay = g_hash_table_lookup (duster->ht[subset], xy2offset(x, y));

  if (!hay)
    {
      hay = g_malloc (4 * NEIGHBORHOOD);
      extract_site (duster, duster->input, x, y, hay);
      if (subset < 0)
      {
        subset = site_subset (hay);
        if (hay[3] == 0)
        {
          g_free (hay);
          return NULL;
        }
      }
      g_hash_table_insert (duster->ht[subset], xy2offset(x, y), hay);
    }

  return hay;
}

static void compare_needle (gpointer key, gpointer value, gpointer data)
{
  void **ptr = data;
  PixelDuster *duster = ptr[0];
  Probe       *probe  = ptr[1];
  guchar      *needle = ptr[2];
  guchar      *hay    = value;
  gint         offset = GPOINTER_TO_INT (key);
  gint x = offset % 65536;
  gint y = offset / 65536;
  int score;

  if (duster->scale_x == 1.0 && x == probe->target_x &&
-     duster->scale_y == 1.0 && y == probe->target_y )
   return;

  score = score_site (duster, &needle[0], hay, probe->score);

  if (score < probe->score)
    {
      probe->source_x = x;
      probe->source_y = y;
      probe->score = score;
    }
}

static int probe_improve (PixelDuster *duster,
                          Probe       *probe)
{
  guchar needle[4 * NEIGHBORHOOD];
  gint  dst_x  = probe->target_x;
  gint  dst_y  = probe->target_y;
  void *ptr[3] = {duster, probe, &needle[0]};
  int old_score = probe->score;
  static const Babl *format = NULL;
  int    set_start[3];
  int    set_end[3];

  if (!format)
    format = babl_format ("RGBA float");

  extract_site (duster, duster->output, dst_x, dst_y, &needle[0]);
  site_subset2 (&needle[0], set_start, set_end);

  if (set_end[0] == set_start[0] &&
      set_end[1] == set_start[1] &&
      set_end[2] == set_start[2])
  {
    int subset = set_start[0] + set_start[1] * 16 + set_start[2] * 16 * 16;
    g_hash_table_foreach (duster->ht[subset], compare_needle, ptr);
  }
  else
  {
    int i[3];
    for (i[0]=0;i[0]<3;i[0]++)
    if (set_start[i[0]] < 0)
      set_start[i[0]] = 0;
    for (i[0]=0;i[0]<3;i[0]++)
    if (set_end[i[0]] > 15)
      set_end[i[0]] = 15;


    for (i[0] = set_start[0]; i[0] <= set_end[0]; i[0]++)
    for (i[1] = set_start[1]; i[1] <= set_end[1]; i[1]++)
    for (i[2] = set_start[2]; i[2] <= set_end[2]; i[2]++)
    {
      int subset = i[0] + i[1] * 16 + i[2] * 16 * 16;
      g_hash_table_foreach (duster->ht[subset], compare_needle, ptr);
    }
  }

  if (probe->score == old_score)
    return -1;
#if 0
  spread_relative (duster, probe, -1, -1);
  spread_relative (duster, probe, -1, 0);
  spread_relative (duster, probe,  0, -1);
  spread_relative (duster, probe,  1, 0);
  spread_relative (duster, probe,  0, 1);
#endif
  probe->age++;

  if (probe->age > 15)
    {
      g_hash_table_remove (duster->probes_ht, xy2offset(probe->target_x, probe->target_y));
    }

  return 0;
}

static inline void pixel_duster_add_probes_for_transparent (PixelDuster *duster)
{
  const Babl *format = babl_format ("RGBA float");
  GeglBufferIterator *i = gegl_buffer_iterator_new (duster->output,
                                                    &duster->out_rect,
                                                    0,
                                                    format,
                                                    GEGL_ACCESS_WRITE,
                                                    GEGL_ABYSS_NONE);
  while (gegl_buffer_iterator_next (i))
  {
    gint x = i->roi[0].x;
    gint y = i->roi[0].y;
    gint n_pixels  = i->roi[0].width * i->roi[0].height;
    float *out_pix = i->data[0];
    while (n_pixels--)
    {
      if (out_pix[3] <= 0.001)
      {
        add_probe (duster, x, y);
      }
      out_pix += 4;

      x++;
      if (x >= i->roi[0].x + i->roi[0].width)
      {
        x = i->roi[0].x;
        y++;
      }
    }
  }
}

static inline void pixel_duster_fill (PixelDuster *duster)
{
  const Babl *format = babl_format ("RGBA float");
  gint missing = 1;
  gint old_missing = 3;
  gint total = 0;
  gint runs = 0;

  while (  (missing >0 && missing != old_missing) || runs < duster->minimum_iterations)
  { runs++;
    total = 0;
    old_missing = missing;
    missing = 0;
  for (GList *p= g_hash_table_get_values (duster->probes_ht); p; p= p->next)
  {
    Probe *probe = p->data;
    gint try_replace;

    if (probe->score == INITIAL_SCORE)
    {
      missing ++;
      try_replace =  0;
    }
    else
    {
      try_replace = ((rand()%100)/100.0) < duster->retry_chance;
    }
    total ++;

    if (probe->score == INITIAL_SCORE || try_replace)
    {
       if ((probe->source_x == probe->target_x &&
            probe->source_y == probe->target_y))
         try_replace = 0;

      if ((rand()%100)/100.0 < duster->try_chance &&
              probe_neighbors (duster, duster->output, probe, duster->minimum_neighbors) >=
              duster->minimum_neighbors)
      {
        if(try_replace)
          probe->score = INITIAL_SCORE;
        if (probe_improve (duster, probe) == 0)
        {
          gfloat rgba[4];
          gegl_buffer_sample (duster->input, probe->source_x, probe->source_y,
                              NULL, &rgba[0], format, GEGL_SAMPLER_NEAREST, 0);
          if (rgba[3] <= 0.01)
            fprintf (stderr, "eek %i,%i %f %f %f %f\n", probe->source_x, probe->source_y, rgba[0], rgba[1], rgba[2], rgba[3]);
          gegl_buffer_set (duster->output, GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1), 0, format, &rgba[0], 0);
         }
      }
    }
  }
  if (duster->op)
    gegl_operation_progress (duster->op, (total-missing) * 1.0 / total,
                             "finding suitable pixels");
#if 1
  fprintf (stderr, "\r%i/%i %2.2f run#:%i  ", total-missing, total, (total-missing) * 100.0 / total, runs);
#endif
  }
#if 1
  fprintf (stderr, "\n");
#endif
}

static void seed_db (PixelDuster *duster)
{
  fprintf (stderr, "db seed");

  for (gint y = 0; y < duster->in_rect.height; y++)
    for (gint x = 0; x < duster->in_rect.width; x++)
      {
        ensure_hay (duster, x, y, -1);
      }

  fprintf (stderr, "ed\n");
}

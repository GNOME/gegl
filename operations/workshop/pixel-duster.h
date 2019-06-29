//if 0
/* pixel-duster
 *
 * the pixel duster data structures and functions are used by multiple ops,
 * but kept in one place since they share so much implementation
 *
 * a context aware pixel inpainting framework..

   avoid building a database of puzzle pieces - since the puzzle pieces are
   pixels samples we condense search space by rectifying rotation - only keep a
   cache.

   (the puzzle pieces are stored by prefix match in a large continuous
   memory region,)

   seed database with used spots and immediate neighbors, for faster
   subsequent lookup

   keep bloom filter - or perhaps even bitmap in GeglBuffer!! for knowing
   contained in db or not.

 * 2018 (c) Øyvind Kolås pippin@gimp.org
 */

/*
    todo:

         threading
           create list of hashtables and to hashtable list per thread

         adjust precision of matching

         replace hashtables with just lists - and include coords in element - perhaps with count..
         for identical entries - thus not losing accurate median computation capabilitiy..
 */
#include <math.h>

#define POW2(x) ((x)*(x))

#define INITIAL_SCORE 1200000000

typedef struct
{
  GeglOperation *op;
  GeglBuffer    *reference;
  GeglBuffer    *input;
  GeglBuffer    *output;
  GeglRectangle  in_rect;
  GeglRectangle  out_rect;
  GeglSampler   *in_sampler_f;
  GeglSampler   *ref_sampler_f;
  GeglSampler   *out_sampler_f;
  int            max_k;
  int            seek_radius;
  int            minimum_neighbors;
  int            minimum_iterations;
  int            max_age;
  float          try_chance;
  float          retry_chance;
  float          scale_x;
  float          scale_y;

  float          ring_gap;
  float          ring_gamma;
  float          ring_twist;

  float          metric_dist_powk;
  float          metric_empty_score;

  GHashTable    *ht[1];

  GHashTable    *probes_ht;

  float          min_x;
  float          min_y;
  float          max_x;
  float          max_y;

  float          order[512][3];
} PixelDuster;


#define MAX_K                   4

#define RINGS                   3   // increments works up to 7-8 with no adver
#define RAYS                    12   // good values for testing 6 8 10 12 16
#define NEIGHBORHOOD            (RINGS*RAYS+1)

#define N_SCALE_NEEDLES         1


//#define BATCH_PROBES 64     // batch this many probes to process concurently
                            // it is slightly faster than doing a full ht
                            // iteration per probe on single core

typedef struct _Probe Probe;

struct _Probe {
  float   needles[N_SCALE_NEEDLES][4 * NEIGHBORHOOD ];
  int     target_x;
  int     target_y;
  int     target_neighbors;
  int     age;
  int     k;
  float   score;
  float   old_score;
  Probe  *neighbors[4];
  float   k_score[MAX_K];
  float   source_x[MAX_K];
  float   source_y[MAX_K];
  gfloat *hay[MAX_K];
};

/* used for hash-table keys */
#define xy2offset(x,y)   GINT_TO_POINTER(((y) * 65536 + (x)))


/* when going through the image preparing the index - only look at the subset
 * needed pixels - and later when fetching out hashed pixels - investigate
 * these ones in particular. would only be win for limited inpainting..
 *
 * making the pixel duster scale invariant on a subpixel level would be neat
 * especially for supersampling, taking the reverse jacobian into account
 * would be even neater.
 *
 */


static void init_order(PixelDuster *duster)
{
  int i;
  duster->order[0][0] = 0;
  duster->order[0][1] = 0;
  duster->order[0][2] = 1.0;
  i = 1;
{

  for (int circleno = 0; circleno < RINGS; circleno++)
  for (float angleno = 0; angleno < RAYS; angleno++)
  {
    float mag = pow(duster->ring_gap * (circleno + 1), duster->ring_gamma);
    float x = cosf ((angleno / RAYS + duster->ring_twist*circleno) * M_PI * 2) * mag;
    float y = sinf ((angleno / RAYS + duster->ring_twist*circleno) * M_PI * 2) * mag;
    duster->order[i][0] = x;
    duster->order[i][1] = y;
    duster->order[i][2] = powf (1.0 / (POW2(x)+POW2(y)), duster->metric_dist_powk);
    i++;
  }
}
}

static void duster_idx_to_x_y (PixelDuster *duster, int index, int *x, int *y)
{
  *x =  duster->order[index][0];
  *y =  duster->order[index][1];
}


static PixelDuster * pixel_duster_new (GeglBuffer *reference,
                                       GeglBuffer *input,
                                       GeglBuffer *output,
                                       const GeglRectangle *in_rect,
                                       const GeglRectangle *out_rect,
                                       int         seek_radius,
                                       int         max_k,
                                       int         minimum_neighbors,
                                       int         minimum_iterations,
                                       float       try_chance,
                                       float       retry_chance,
                                       float       scale_x,
                                       float       scale_y,
                                       int         improvement_iterations,
                                       float       ring_gap,
                                       float       ring_gamma,
                                       float       ring_twist,
                                       float       metric_dist_powk,
                                       float       metric_empty_score,
                                       GeglOperation *op)
{
  PixelDuster *ret = g_malloc0 (sizeof (PixelDuster));
  ret->reference   = reference;
  ret->input       = input;
  ret->output      = output;
  ret->seek_radius = seek_radius;
  ret->minimum_neighbors  = minimum_neighbors;
  ret->minimum_iterations = minimum_iterations;
  ret->try_chance   = try_chance;
  ret->retry_chance = retry_chance;
  ret->op = op;
  ret->max_x = 0;
  ret->max_y = 0;
  ret->min_x = 10000;
  ret->min_y = 10000;
  ret->max_age = improvement_iterations;
  ret->ring_gap = ring_gap;
  ret->ring_gamma = ring_gamma;
  ret->ring_twist = ring_twist;

  if (max_k < 1) max_k = 1;
  if (max_k > MAX_K) max_k = MAX_K;
  ret->max_k = max_k;
  ret->in_rect  = *in_rect;
  ret->out_rect = *out_rect;
  ret->scale_x  = scale_x;
  ret->scale_y  = scale_y;
  ret->metric_dist_powk = metric_dist_powk;
  ret->metric_empty_score = metric_empty_score;

  ret->in_sampler_f = gegl_buffer_sampler_new (input,
                                               babl_format ("RGBA float"),
                                               GEGL_SAMPLER_CUBIC);

  ret->ref_sampler_f = gegl_buffer_sampler_new (reference,
                                               babl_format ("RGBA float"),
                                               GEGL_SAMPLER_CUBIC);

  ret->out_sampler_f = gegl_buffer_sampler_new (output,
                                               babl_format ("RGBA float"),
                                               GEGL_SAMPLER_CUBIC);

  for (int i = 0; i < 1; i++)
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
  for (int i = 0; i < 1; i++)
  {
    g_hash_table_destroy (duster->ht[i]);
  }

  g_object_unref (duster->ref_sampler_f);
  g_object_unref (duster->in_sampler_f);
  g_object_unref (duster->out_sampler_f);

  g_free (duster);
}


void gegl_sampler_prepare (GeglSampler *sampler);

/*  extend with scale factor/matrix
 *
 */
static void extract_site (PixelDuster *duster, GeglBuffer *buffer, double x, double y, float scale, gfloat *dst)
{
  static const Babl *format = NULL;

  GeglSampler *sampler_f;

  if (buffer == duster->output)
  {
    sampler_f = duster->out_sampler_f;
    gegl_sampler_prepare (sampler_f);
  }
  else if (buffer == duster->reference)
  {
    sampler_f   = duster->ref_sampler_f;
  }
  else if (buffer == duster->input)
  {
    sampler_f   = duster->in_sampler_f;
  }

  if (!format){
    format = babl_format ("RGBA float");
  }

#if 1
  for (int i = 0; i < NEIGHBORHOOD; i++)
  {
    int dx, dy;
    duster_idx_to_x_y (duster, i, &dx, &dy);
    gegl_sampler_get (sampler_f, x + dx * scale, y + dy * scale, NULL, &dst[i*4], 0);
  }

  {
    int warmest_ray = 0;
    float warmest_ray_energy = 0;
    gfloat tmp[4 * NEIGHBORHOOD];

    for (int ray = 0; ray < RAYS; ray ++)
    {
      float energy = 0.0;
      for (int circle = 0; circle < RINGS; circle++)
        for (int c = 0; c < 3; c++)
          energy += dst[ ( circle * RAYS  + ray )*4 + c];
      if (energy > warmest_ray_energy)
      {
        warmest_ray = ray;
        warmest_ray_energy = energy;
      }
    }

    if (warmest_ray)
    {
      for (int i = 0; i < NEIGHBORHOOD*4; i++)
        tmp[i] = dst[i];

      for (int ray = 0; ray < RAYS; ray ++)
      {
        int swapped_ray = ray + warmest_ray;
        while (swapped_ray >= RAYS) swapped_ray -= RAYS;

        for (int circle = 0; circle < RINGS; circle++)
        for (int c = 0; c < 4; c++)
          dst[ ( circle * RAYS  + ray )*4 + c] =
          tmp[ ( circle * RAYS  + swapped_ray )*4 + c];
       }
    }
  }
#endif
}

static inline int u8_rgb_diff (guchar *a, guchar *b)
{
  return POW2(a[0]-b[0]) * 2 + POW2(a[1]-b[1]) * 3 + POW2(a[2]-b[2]);
}


static inline float f_rgb_diff (float *a, float *b)
{
  return POW2(a[0]-b[0]) + POW2(a[1]-b[1]) + POW2(a[2]-b[2]);
}

static float inline
score_site (PixelDuster *duster,
            gfloat      *needle,
            gfloat      *hay,
            float        bail)
{
  int i;
  float score = 0;
  /* bail early with really bad score - the target site doesnt have opacity */

  if (hay[3] < 0.001f)
  {
    return INITIAL_SCORE;
  }

  for (i = 1; i < NEIGHBORHOOD && score < bail; i++)
  {
    if (needle[i*4 + 3]<1.0f && hay[i*4 + 3]>0.001f)
    {
      score += f_rgb_diff (&needle[i*4 + 0], &hay[i*4 + 0]) * duster->order[i][2];
    }
    else
    {
      score += duster->metric_empty_score * duster->order[i][2];
    }
  }
  return sqrtf (score);
}

static Probe *
add_probe (PixelDuster *duster, int target_x, int target_y)
{
  Probe *probe    = g_malloc0 (sizeof (Probe));
  if (target_x < duster->min_x)
    duster->min_x = target_x;
  if (target_y < duster->min_y)
    duster->min_y = target_y;
  if (target_x > duster->max_x)
    duster->max_x = target_x;
  if (target_y > duster->max_y)
    duster->max_y = target_y;

  probe->target_x = target_x;
  probe->target_y = target_y;
  probe->source_x[0] = target_x / duster->scale_x;
  probe->source_y[0] = target_y / duster->scale_y;
  probe->k_score[0]   = INITIAL_SCORE;
  probe->k = 0;
  probe->score   = INITIAL_SCORE;
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

#define ret_if_good     if (found >=min) goto ret;

static int
probe_neighbors (PixelDuster *duster, GeglBuffer *output, Probe *probe, int min)
{
  int found = 0;
  found = probe->target_neighbors;

  ret_if_good

  found = 0;

  if (probe_rel_is_set (duster, output, probe, -1, 0)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe,  1, 0)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe,  0, 1)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe,  0, -1)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe,  1, 1)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe, -1,-1)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe,  1,-1)) found ++;
  ret_if_good
  if (probe_rel_is_set (duster, output, probe, -1, 1)) found ++;

ret:
  probe->target_neighbors = found;
  return found;
}

static gfloat *ensure_hay (PixelDuster *duster, int x, int y)
{
  gfloat *hay = NULL;

  hay = g_hash_table_lookup (duster->ht[0], xy2offset(x, y));

  if (!hay)
    {
      hay = g_malloc ((4 * NEIGHBORHOOD) * 4);
      extract_site (duster, duster->reference, x, y, 1.0, hay);
      g_hash_table_insert (duster->ht[0], xy2offset(x, y), hay);
    }

  return hay;
}

static inline void
probe_prep (PixelDuster *duster,
            Probe *probe)
{
  gint  dst_x  = probe->target_x;
  gint  dst_y  = probe->target_y;
  extract_site (duster, duster->output, dst_x, dst_y, 1.0, &probe->needles[0][0]);
  if (N_SCALE_NEEDLES > 1)
    extract_site (duster, duster->output, dst_x, dst_y, 0.9, &probe->needles[1][0]);
  if (N_SCALE_NEEDLES > 2)
    extract_site (duster, duster->output, dst_x, dst_y, 1.1, &probe->needles[2][0]);
  if (N_SCALE_NEEDLES > 3)
    extract_site (duster, duster->output, dst_x, dst_y, 0.8, &probe->needles[3][0]);
  if (N_SCALE_NEEDLES > 4)
    extract_site (duster, duster->output, dst_x, dst_y, 1.2, &probe->needles[4][0]);
  if (N_SCALE_NEEDLES > 5)
    extract_site (duster, duster->output, dst_x, dst_y, 0.66, &probe->needles[5][0]);
  if (N_SCALE_NEEDLES > 6)
    extract_site (duster, duster->output, dst_x, dst_y, 1.5, &probe->needles[6][0]);
  probe->old_score = probe->score;

  {
  int neighbours = 0;
  for (GList *p= g_hash_table_get_values (duster->probes_ht); p; p= p->next)
  {
    Probe *oprobe = p->data;
    if (oprobe != probe)
    {
      if ( (probe->target_x == oprobe->target_x - 1) &&
           (probe->target_y == oprobe->target_y))
        probe->neighbors[neighbours++] = oprobe;
      if ( (probe->target_x == oprobe->target_x + 1) &&
           (probe->target_y == oprobe->target_y))
        probe->neighbors[neighbours++] = oprobe;
      if ( (probe->target_x == oprobe->target_x) &&
           (probe->target_y == oprobe->target_y - 1))
        probe->neighbors[neighbours++] = oprobe;
      if ( (probe->target_x == oprobe->target_x) &&
           (probe->target_y == oprobe->target_y + 1))
        probe->neighbors[neighbours++] = oprobe;
    }
  }
    for (;neighbours < 4; neighbours++)
      probe->neighbors[neighbours] = NULL;
  }


  if (probe->score == INITIAL_SCORE)
  {
    
  }

}

static void probe_compare_hay (PixelDuster *duster,
                               Probe *probe,
                               gfloat *hay,
                               gint x, gint y)
{

  float score;

#define pow2(a)   ((a)*(a))

  if ( duster->seek_radius > 1 &&
       pow2 (probe->target_x / duster->scale_x - x) +
       pow2 (probe->target_y / duster->scale_y - y) >
       pow2 (duster->seek_radius))
#if 0
  if (duster->scale_x == 1.0 && x == probe->target_x &&
-     duster->scale_y == 1.0 && y == probe->target_y )
   return;
#endif
    {
    }
  else
    {
      for (int n = 0; n < N_SCALE_NEEDLES; n++)
      {
        score = score_site (duster, &probe->needles[n][0], hay, probe->score);

        if (score < probe->score)
        {
          int j;
          for (j = duster->max_k-1; j >= 1; j --)
          {
            probe->source_x[j] = probe->source_x[j-1];
            probe->source_y[j] = probe->source_y[j-1];
            probe->hay[j] = probe->hay[j-1];
            probe->k_score[j] = probe->k_score[j-1];
          }
          probe->k++;
          if (probe->k > duster->max_k)
            probe->k = duster->max_k;
          probe->source_x[0] = x;
          probe->source_y[0] = y;
          probe->hay[0] = hay;
          probe->score = probe->k_score[0] = score;
        }
      }
    }
}

static void compare_needle (gpointer key, gpointer value, gpointer data)
{
  void **ptr = data;
  PixelDuster *duster = ptr[0];
  Probe       *probe  = ptr[1];
  gfloat      *hay    = value;
  gint         offset = GPOINTER_TO_INT (key);
  gint x = offset % 65536;
  gint y = offset / 65536;

  probe_compare_hay (duster, probe, hay, x, y);
}

static void
probe_post_search (PixelDuster *duster, Probe *probe)
{
  static const Babl *format = NULL;
  if (!format)
    format = babl_format ("RGBA float");
  probe->age++;

  if (probe->score != probe->old_score)
  {
    gfloat sum_rgba[4]={0.0,0.0,0.0,0.0};
    gfloat rgba[4];

    if (probe->k>1)
    {
      for (gint i = 0; i < probe->k; i++)
      {
        gegl_sampler_get (duster->in_sampler_f,
                          probe->source_x[i], probe->source_y[i],
                          NULL, &rgba[0], 0);
        for (gint c = 0; c < 4; c++)
          sum_rgba[c] += rgba[c];
      }
      for (gint c = 0; c < 4; c++)
        rgba[c] = sum_rgba[c] / probe->k;
    }
    else
    {
      gegl_sampler_get (duster->in_sampler_f,
                        probe->source_x[0], probe->source_y[0], NULL,
                        &rgba[0], 0);
    }

    gegl_buffer_set (duster->output,
                     GEGL_RECTANGLE(probe->target_x, probe->target_y, 1, 1),
                     0, format, &rgba[0], 0);
  }
}

static int probe_improve (PixelDuster *duster,
                          Probe       *probe)
{
  void *ptr[2] = {duster, probe};
  static const Babl *format = NULL;

  if (probe->age >= duster->max_age)
    {
      g_hash_table_remove (duster->probes_ht,
                           xy2offset(probe->target_x, probe->target_y));
      return -1;
    }

  if (!format)
    format = babl_format ("RGBA float");

  probe_prep (duster, probe);

  g_hash_table_foreach (duster->ht[0], compare_needle, ptr);

  probe_post_search (duster, probe);

  return 0;
}

static inline void compare_probes (gpointer key, gpointer value, gpointer data)
{
  void **ptr = data;
  PixelDuster *duster = ptr[0];
  Probe      **probes  = ptr[1];
  int          n_probes = GPOINTER_TO_INT (ptr[2]);
  gfloat      *hay    = value;
  gint         offset = GPOINTER_TO_INT (key);
  gint x = offset % 65536;
  gint y = offset / 65536;

  for (int p = 0; p < n_probes; p++)
  if (probes[p])
  {
    Probe *probe = probes[p];
    if (probe)
      probe_compare_hay (duster, probe, hay, x, y);
  }
}


static inline int probes_improve (PixelDuster *duster,
                                  Probe       **probes,
                                  int           n_probes)
{
#ifdef BATCH_PROBES
  void *ptr[3] = {duster, probes, GINT_TO_POINTER (n_probes)};

  for (int i = 0; i < n_probes; i++)
  {
    Probe *probe = probes[i];

    if (probe->age >= duster->max_age)
    {
      g_hash_table_remove (duster->probes_ht, xy2offset(probe->target_x, probe->target_y));
      probes[i] = NULL;
    }
    else
    {
      probe_prep (duster, probe);
   }
  }
  g_hash_table_foreach (duster->ht[0], compare_probes, ptr);

  for (int i = 0; i < n_probes; i++)
  {
    if (probes[i])
      probe_post_search (duster, probes[i]);
  }
#else
  for (int i = 0; i < n_probes; i++)
  {
    if (probes[i])
      probe_improve (duster, probes[i]);
  }
#endif
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
                                                    GEGL_ABYSS_NONE, 1);
  while (gegl_buffer_iterator_next (i))
  {
    gint x = i->items[0].roi.x;
    gint y = i->items[0].roi.y;
    gint n_pixels  = i->items[0].roi.width * i->items[0].roi.height;
    float *out_pix = i->items[0].data;
    while (n_pixels--)
    {
      if (out_pix[3] < 1.0f /* ||
          (out_pix[0] <= 0.1 &&
           out_pix[1] <= 0.1 &&
           out_pix[2] <= 0.1) */)
      {
        /* we process all - also partially transparent pixels, making the op work well in conjuction with a small hard eraser brush. And improvement could be to re-composite partially transparent pixels back on top as a final step, making the alpha values continuously rather than binary meaningful.
         */
        add_probe (duster, x, y);
      }
      out_pix += 4;

      x++;
      if (x >= i->items[0].roi.x + i->items[0].roi.width)
      {
        x = i->items[0].roi.x;
        y++;
      }
    }
  }
}

static inline void pixel_duster_fill (PixelDuster *duster)
{
  gint missing = 1;
  gint total = 0;
  gint runs = 0;

  while (  ((missing >0) /* && (missing != old_missing) */) ||
           (runs < duster->minimum_iterations))
  {
#ifdef BATCH_PROBES
    Probe *probes[BATCH_PROBES];
    int n_probes = 0;
#endif

    runs++;
    total = 0;
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

#if 0 // can be useful for enlarge ? then needs scale factor
    if ((probe->source_x[0] == probe->target_x &&
         probe->source_y[0] == probe->target_y))
      try_replace = 0;
#endif

    if (probe->score == INITIAL_SCORE || try_replace)
    {
      if ((rand()%100)/100.0 < duster->try_chance &&
              probe_neighbors (duster, duster->output, probe, duster->minimum_neighbors) >=
              duster->minimum_neighbors)
      {
#ifdef BATCH_PROBES
        probes[n_probes++] = probe;
#else
        probe_improve (duster, probe);
#endif
      }
    }
#ifdef BATCH_PROBES
    if (n_probes>=BATCH_PROBES)
    {
      probes_improve (duster, probes, n_probes);
      n_probes = 0;
    }
#endif
  }
#ifdef BATCH_PROBES
  if (n_probes)
    probes_improve (duster, probes, n_probes);
#endif

  if (duster->op)
    gegl_operation_progress (duster->op, 0.2 + (total-missing) * 0.8 / total,
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

  if (duster->max_x > duster->min_x)
  {
    fprintf (stderr, "ing");
    for (gint y = duster->min_y - duster->seek_radius;
              y < duster->max_y + duster->seek_radius; y++)
    {
      for (gint x = duster->min_x - duster->seek_radius;
                x < duster->max_x + duster->seek_radius; x++)
      {
        ensure_hay (duster, x, y);
      }


      if (y % 13 == 0)
      {
  if (duster->op)
    gegl_operation_progress (duster->op,

     (y - duster->min_y + duster->seek_radius) * 0.2 / (duster->seek_radius * 2 + duster->max_y-duster->min_y),
                             "generating neighborhood db");


      fprintf (stderr, "\rdb seeding %s (%.2f%%)", y%4==0?"-":y%4==1?"/":y%4==2?"\\":"|",
     (y - duster->min_y + duster->seek_radius) * 100.0 / (duster->seek_radius * 2 + duster->max_y-duster->min_y));
      }
    }
    fprintf (stderr, "\rdb seeded                   \n");
  }
  else
  {

    for (gint y = 0; y < duster->in_rect.height; y++)
    for (gint x = 0; x < duster->in_rect.width; x++)
      {
        ensure_hay (duster, x, y);
      }
    fprintf (stderr, "ed\n");
  }
}

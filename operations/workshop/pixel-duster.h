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
         exclude identicals - when it is obvious

         threading
           create list of hashtables and to hashtable list per thread

         adjust precision of matching

         replace hashtables with just lists - and include coords in element - perhaps with count..
         for identical entries - thus not losing accurate median computation capabilitiy..
         do median instead of mean for matched pixel components

         complete code using relative to center pixel instead of center pixel -
         thus permitting a wider range of neighborhoods to produce valid data - thus
         will be good at least for superresolution

         add more symmetries mirroring each doubling data - add full rotation
         invariance
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
  GeglSampler   *in_sampler_yu8;
  GeglSampler   *in_sampler_f;
  GeglSampler   *ref_sampler_yu8;
  GeglSampler   *ref_sampler_f;
  GeglSampler   *out_sampler_yu8;
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

  GHashTable    *ht[1];

  GHashTable    *probes_ht;

  float          min_x;
  float          min_y;
  float          max_x;
  float          max_y;

  float          order[512][3];
} PixelDuster;


#define MAX_K               4

#define RINGS               3
#define RAYS                16
#define GAP                 1.1
#define RINGGAMMA           1.45
#define TWIST               0.03

#define NEIGHBORHOOD        (RINGS*RAYS+1)

#define MAX_DIR             4

// safe to leave always on since only really used when epplies
#define PIXDUST_DIR_INVARIANT 1


//#define ONLY_DIR            1

typedef struct Probe {
  int     target_x;
  int     target_y;

  int     age;
  int     k;
  float   score;
  float   k_score[MAX_K];
  /* should store sampled coordinates instead */
  /* the only real datamembers are below and should be float*/
  float   source_x[MAX_K];
  float   source_y[MAX_K];
  gfloat *hay[MAX_K];
} Probe;

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

  for (float angleno = 0; angleno < RAYS; angleno++)
  for (int circleno = 0; circleno < RINGS; circleno++)
  {
    float mag = pow(GAP * (circleno + 1), RINGGAMMA);
    float x = cosf ((angleno / RAYS + TWIST*circleno) * M_PI * 2) * mag;
    float y = sinf ((angleno / RAYS + TWIST*circleno) * M_PI * 2) * mag;
    duster->order[i][0] = x;
    duster->order[i][1] = y;
    duster->order[i][2] = powf (1.0 / (POW2(x)+POW2(y)), 0.8);
    i++;
  }
}
#if 0
  for (i = 1; i < 159; i++)
  for (y = -7; y <= 7; y ++)
    for (x = -7; x <= 7; x ++)
      {
        if (order_2d[y+7][x+7] == i)
        {
          duster->order[i][0] = y;
          duster->order[i][1] = x;
          duster->order[i][2] = pow (1.0 / (POW2(x)+POW2(y)), 0.8);
        }
      }
#endif
}

static void duster_idx_to_x_y (PixelDuster *duster, int index, int dir, int *x, int *y)
{
  switch (dir)
  {
    default:
    case 0: /* right */
      *x =  -duster->order[index][0];
      *y =  -duster->order[index][1];
      break;
    case 1: /* left */
      *x =  duster->order[index][0];
      *y =  duster->order[index][1];
      break;
    case 2: /* down */
      *x =  -duster->order[index][1];
      *y =  -duster->order[index][0];
      break;
    case 3: /* up */
      *x =  duster->order[index][1];
      *y =  duster->order[index][0];
      break;

    case 4: /* right */
      *x =  -duster->order[index][0];
      *y =   duster->order[index][1];
      break;
    case 5: /* left */
      *x =  duster->order[index][0];
      *y =  -duster->order[index][1];
      break;
    case 6: /* down */
      *x =  -duster->order[index][1];
      *y =  duster->order[index][0];
      break;
    case 7: /* up */
      *x =  duster->order[index][1];
      *y =  -duster->order[index][0];
      break;
  }
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
  ret->max_age = 5;

  if (max_k < 1) max_k = 1;
  if (max_k > MAX_K) max_k = MAX_K;
  ret->max_k = max_k;
  ret->in_rect  = *in_rect;
  ret->out_rect = *out_rect;
  ret->scale_x  = scale_x;
  ret->scale_y  = scale_y;

  ret->in_sampler_yu8 = gegl_buffer_sampler_new (input, babl_format ("Y'aA u8"),
                                                 GEGL_SAMPLER_CUBIC);
  ret->in_sampler_f = gegl_buffer_sampler_new (input,
                                               babl_format ("RGBA float"),
                                               GEGL_SAMPLER_CUBIC);

  ret->ref_sampler_yu8 = gegl_buffer_sampler_new (reference, babl_format ("Y'aA u8"),
                                                 GEGL_SAMPLER_CUBIC);
  ret->ref_sampler_f = gegl_buffer_sampler_new (reference,
                                               babl_format ("RGBA float"),
                                               GEGL_SAMPLER_CUBIC);

  ret->out_sampler_yu8 = gegl_buffer_sampler_new (output, babl_format ("Y'aA u8"),
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
#if 0
    if (g_hash_table_size (duster->ht[i]))
      fprintf (stderr, "%i:%i ", i, g_hash_table_size (duster->ht[i]));
#endif
    g_hash_table_destroy (duster->ht[i]);
  }
  fprintf (stderr, "\n");

  g_object_unref (duster->ref_sampler_yu8);
  g_object_unref (duster->ref_sampler_f);

  g_object_unref (duster->in_sampler_yu8);
  g_object_unref (duster->in_sampler_f);

  g_free (duster);
}


void gegl_sampler_prepare (GeglSampler *sampler);

/*  extend with scale factor/matrix
 *
 */
static void extract_site (PixelDuster *duster, GeglBuffer *buffer, double x, double y, float scale, gfloat *dst)
{
  static const Babl *format = NULL;
  guchar lum[8];
  int bdir, maxlum;
  //uint64_t hist3dmask=0;

  GeglSampler *sampler_yu8;
  GeglSampler *sampler_f;

  if (buffer == duster->output)
  {
    sampler_yu8 = duster->out_sampler_yu8;
    sampler_f = duster->out_sampler_f;
    gegl_sampler_prepare (sampler_yu8);
    gegl_sampler_prepare (sampler_f);
  }
  else if (buffer == duster->reference)
  {
    sampler_yu8 = duster->ref_sampler_yu8;
    sampler_f = duster->ref_sampler_f;
  }
  else if (buffer == duster->input)
  {
    sampler_yu8 = duster->in_sampler_yu8;
    sampler_f = duster->in_sampler_f;
  }

  if (!format){
    format = babl_format ("RGBA float");
  }

#if PIXDUST_DIR_INVARIANT==1
  /* figure out which of the up/down/left/right pixels are brightest,
     using premultiplied alpha - do punish blank spots  */

  gegl_sampler_get (sampler_yu8, x + 1 *scale, y + 0, NULL, &lum[0], 0);
  gegl_sampler_get (sampler_yu8, x - 1 *scale, y + 0, NULL, &lum[2], 0);
  gegl_sampler_get (sampler_yu8, x + 0, y + 1 * scale, NULL, &lum[4], 0);
  gegl_sampler_get (sampler_yu8, x + 0, y - 1 * scale, NULL, &lum[6], 0);

 bdir = 0;

 maxlum = lum[0*2];
 for (int i = 1; i < MIN(4,MAX_DIR); i++)
   if (lum[i*2] > maxlum)
     {
       bdir = i;
       maxlum = lum[i*2];
     }

 if (MAX_DIR > 4)
 {
   switch (bdir)
   {
     case 0: if (lum[4] > lum[6]) bdir += 4; break;
     case 1: if (lum[6] > lum[4]) bdir += 4; break;
     case 2: if (lum[0] > lum[2]) bdir += 4; break;
     case 3: if (lum[2] > lum[0]) bdir += 4; break;
   }
 }

#ifdef ONLY_DIR
  bdir = ONLY_DIR;
#endif
#endif

#if 1
  for (int i = 0; i <= NEIGHBORHOOD; i++)
  {
    int dx, dy;
    duster_idx_to_x_y (duster, i, bdir, &dx, &dy);
    gegl_sampler_get (sampler_f, x + dx * scale, y + dy * scale, NULL, &dst[i*4], 0);
#if 0
    {
      int hist_r = dst[i*4+0]/80;
      int hist_g = dst[i*4+1]/80;
      int hist_b = dst[i*4+2]/80;
      int hist_bit = hist_r * 4 * 4 + hist_g * 4 + hist_b;
      hist3dmask |= (1 << hist_bit);
    }
#endif
  }
 dst[0] = bdir;
#if 0
 *((uint64_t*)(&dst[4*NEIGHBORHOOD])) = hist3dmask;
#endif
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

  if (hay[3] < 0.001)
  {
    return INITIAL_SCORE;
  }

#if 0
  {
    uint64_t *needle_hist = (void*)&needle[NEIGHBORHOOD * 4];
    uint64_t *hay_hist    = (void*)&hay[NEIGHBORHOOD * 4];
    uint64_t  diff_hist = *needle_hist ^ *hay_hist;
    int missing = 0;
  for (i = 0; i < 64; i ++)
  {
    if ((diff_hist & (1 << i)) &&
        (*needle_hist & ( 1 <<i)))
      missing ++;
    //else if ( *needle_hist & (i<<i)) missing ++;
  }
    if (missing > 23)
      return INITIAL_SCORE;
  }
#endif

  for (i = 1; i < NEIGHBORHOOD && score < bail; i++)
  {
    if (needle[i*4 + 3]>0.001 && hay[i*4 + 3]>0.001)
    {
      score += f_rgb_diff (&needle[i*4 + 0], &hay[i*4 + 0]) * duster->order[i][2];
    }
    else
    {
      score += 10;//256;// * duster->order[i][2];
    }
  }
  return score;
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
#if 1
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  1, 1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -1,-1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe,  1,-1)) found ++;
  if (found >=min) return found;
  if (probe_rel_is_set (duster, output, probe, -1, 1)) found ++;
#endif
#if 0
  if (found >=min) return found;
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

#if 0
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
#endif

#if 0
static void inline compare_needle_exact (gpointer key, gpointer value, gpointer data)
{
  void **ptr = data;
  //PixelDuster *duster = ptr[0];
  gfloat *new_hay;
  gfloat *val_hay;
  gint   *found_count = ptr[2];
  if (*found_count)
    return;

  new_hay = ptr[1];
  val_hay = value;

  for (int i = 4; i < 4 * NEIGHBORHOOD; i++)
    {
      int diff = ((int)(new_hay[i])) - ((int)(val_hay[i]));
      if ( (diff <= -2) || (diff >= 2))
        return;
    }
 (*found_count)++;
}
#endif

static gfloat *ensure_hay (PixelDuster *duster, int x, int y, int subset)
{
  gfloat *hay = NULL;

  if (subset >= 0)
    hay = g_hash_table_lookup (duster->ht[subset], xy2offset(x, y));

  if (!hay)
    {
      hay = g_malloc ((4 * NEIGHBORHOOD + 8) * 4);
      extract_site (duster, duster->reference, x, y, 1.0, hay);
      if (subset < 0)
      {
        if (hay[3] == 0)
        {
          g_free (hay);
          return NULL;
        }
      }
      subset = 0;
      {
#if 0
        gint found_count = 0;
        void *ptr[3] = {duster, &hay[0], &found_count};
        g_hash_table_foreach (duster->ht[subset], compare_needle_exact, ptr);
        if (found_count)
          {
            g_free (hay);
          }
        else
#endif
          {
            g_hash_table_insert (duster->ht[subset], xy2offset(x, y), hay);
          }
      }
    }

  return hay;
}

/* XXX : replace with compare_needles */
static void compare_needle (gpointer key, gpointer value, gpointer data)
{
  void **ptr = data;
  PixelDuster *duster = ptr[0];
  Probe       *probe  = ptr[1];
  gfloat      *needle = ptr[2];
  gfloat      *hay    = value;
  gint         offset = GPOINTER_TO_INT (key);
  gint x = offset % 65536;
  gint y = offset / 65536;
  float score;

#if 1
#define pow2(a)   ((a)*(a))
  if ( duster->seek_radius > 1 &&
       pow2 (probe->target_x / duster->scale_x - x) +
       pow2 (probe->target_y / duster->scale_y - y) >
       pow2 (duster->seek_radius))
    return;
#endif

#if 0
  if (duster->scale_x == 1.0 && x == probe->target_x &&
-     duster->scale_y == 1.0 && y == probe->target_y )
   return;
#endif

  score = score_site (duster, &needle[0], hay, probe->score);

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

static int probe_improve (PixelDuster *duster,
                          Probe       *probe)
{
  gfloat needle[4 * NEIGHBORHOOD + 8];

  gint  dst_x  = probe->target_x;
  gint  dst_y  = probe->target_y;
  void *ptr[3] = {duster, probe, &needle[0]};
  float old_score = probe->score;
  static const Babl *format = NULL;

  if (!format)
    format = babl_format ("RGBA float");

  extract_site (duster, duster->output, dst_x, dst_y, 1.0, &needle[0]);
  g_hash_table_foreach (duster->ht[0], compare_needle, ptr);

  extract_site (duster, duster->output, dst_x, dst_y, 1.2, &needle[0]);
  g_hash_table_foreach (duster->ht[0], compare_needle, ptr);

  extract_site (duster, duster->output, dst_x, dst_y, 0.83, &needle[0]);
  g_hash_table_foreach (duster->ht[0], compare_needle, ptr);

  probe->age++;

#if 0
  spread_relative (duster, probe, -1, -1);
  spread_relative (duster, probe, -1, 0);
  spread_relative (duster, probe,  0, -1);
  spread_relative (duster, probe,  1, 0);
  spread_relative (duster, probe,  0, 1);
#endif

  if (probe->age > duster->max_age)
    {
      g_hash_table_remove (duster->probes_ht, xy2offset(probe->target_x, probe->target_y));
    }
  if (probe->score == old_score)
    return -1;

  return 0;
}

static inline int probes_improve (PixelDuster *duster)
{
  int ret = -1;

  for (GList *p= g_hash_table_get_values (duster->probes_ht); p; p= p->next)
  {
    Probe *probe = p->data;
    probe_improve (duster, probe);
    if (ret == 0)
      ret = 0;
  }
  return ret;
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
  const Babl *format = babl_format ("RGBA float");
  gint missing = 1;
  //gint old_missing = 3;
  gint total = 0;
  gint runs = 0;

  while (  ((missing >0) /* && (missing != old_missing) */) ||
           (runs < duster->minimum_iterations))
  { runs++;
    total = 0;
    //old_missing = missing;
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
    if ((probe->source_x[0] == probe->target_x &&
        probe->source_y[0] == probe->target_y))
      try_replace = 0;

    if (probe->score == INITIAL_SCORE || try_replace)
    {

      if ((rand()%100)/100.0 < duster->try_chance &&
              probe_neighbors (duster, duster->output, probe, duster->minimum_neighbors) >=
              duster->minimum_neighbors)
      {
        //if(try_replace)
        //  probe->score = INITIAL_SCORE;

        if (probe_improve (duster, probe) == 0)
        {
          gfloat sum_rgba[4]={0.0,0.0,0.0,0.0};
          gfloat rgba[4];

          if (probe->k>1)
          {
            for (gint i = 0; i < probe->k; i++)
            {
              gegl_sampler_get (duster->in_sampler_f, probe->source_x[i], probe->source_y[i],
                                NULL, &rgba[0], 0);
              for (gint c = 0; c < 4; c++)
                sum_rgba[c] += rgba[c];
            }
            for (gint c = 0; c < 4; c++)
              rgba[c] = sum_rgba[c] / probe->k;

#if 0
          if (rgba[3] <= 0.01)
            fprintf (stderr, "eek %f,%f %f %f %f %f\n", probe->source_x[MAX_K/2], probe->source_y[MAX_K/2], rgba[0], rgba[1], rgba[2], rgba[3]);
#endif
          }
          else
          {
            gegl_sampler_get (duster->in_sampler_f, probe->source_x[0], probe->source_y[0], NULL, &rgba[0], 0);
          }

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

  if (duster->max_x > duster->min_x)
  {
    fprintf (stderr, "ing");
    for (gint y = duster->min_y - duster->seek_radius;
              y < duster->max_y + duster->seek_radius; y++)
    {
      for (gint x = duster->min_x - duster->seek_radius;
                x < duster->max_x + duster->seek_radius; x++)
      {
        ensure_hay (duster, x, y, -1);
      }
      if (y % 13 == 0)
      {
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
        ensure_hay (duster, x, y, -1);
      }
    fprintf (stderr, "ed\n");
  }

}

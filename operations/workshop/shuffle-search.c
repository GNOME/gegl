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
 * Copyright 2024 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


/* random is not portable to all platforms */
#define random() g_random_int ()
// todo : use gegl_random for reproducible results
//        chunking/progress reporting, right now it blocks the GIMP ui while processing


#if 0
#include <stdio.h>
#define DEV_MODE 1
#else
#define DEV_MODE 0
#endif
#define CENTER_BIAS  10

#ifdef GEGL_PROPERTIES

property_int  (iterations, _("iterations"), 3)
               description(_("How many times to run optimization"))
               value_range (0, 32)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

property_int  (chance, _("chance"), 50)
               description(_("Chance of doing optimization"))
               value_range (1, 100)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

property_int  (levels, _("levels"), 3)
               description(_("Only used if no aux image is provided"))
               value_range (2, 255)
property_int  (center_bias, _("center bias"), 2)
               value_range (0, 16)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME     shuffle_search
#define GEGL_OP_C_SOURCE shuffle-search.c

#include "gegl-op.h"


#include <stdio.h>

static void
prepare (GeglOperation *operation)
{
  GeglRectangle *aux_bounds = gegl_operation_source_get_bounding_box(operation,"aux");

  if (aux_bounds && aux_bounds->width)
  {
    GeglNode *aux_node = gegl_operation_get_source_node (operation, "aux");
    GeglOperation *aux_op = gegl_node_get_gegl_operation(aux_node);
    gegl_operation_set_format (operation, "output",
      gegl_operation_get_format (aux_op, "output"));
  }
  else
  {
    const Babl *space = gegl_operation_get_source_space (operation, "input");
    gegl_operation_set_format (operation, "output",
                               babl_format_with_space ("Y' u8", space));
  }
}


static uint8_t compute_val(int center_bias, const uint8_t *bits, int stride, int x, int y)
{
  int count = 0;
  int sum = 0;
  for (int v = y-1; v <= y+1; v++)
  for (int u = x-1; u <= x+1; u++)
  {
    int val = bits[v*stride+u];
    int contrib = 1 + (u == x && v == y) * center_bias;
    count += contrib;
    sum += val * contrib;
  }
  if (count)
  return (sum)/count;
  return 0;
}

static void compute_rgb(int center_bias, const uint8_t *rgb, int stride, int x, int y, uint8_t *rgb_out)
{
  int count = 0;
  int red_sum = 0;
  int green_sum = 0;
  int blue_sum = 0;

  for (int v = y-1; v <= y+1; v++)
  for (int u = x-1; u <= x+1; u++)
  {
      int o = 3*(v*stride+u);
      int contrib = 1 + (u == x && v == y) * center_bias;
      count += contrib;
      red_sum += rgb[o+0] * contrib;
      green_sum += rgb[o+1] * contrib;
      blue_sum += rgb[o+2] * contrib;
  }

  if (count)
  {
    rgb_out[0] = red_sum / count;
    rgb_out[1] = green_sum / count;
    rgb_out[2] = blue_sum / count;
  }
  else
  {
    rgb_out[0] =
    rgb_out[1] =
    rgb_out[2] = 0;
  }
}


static int rgb_diff(const uint8_t *a, const uint8_t* b)
{
  int sum_sq_diff = 0;
  for (int c = 0; c < 3; c++)
    sum_sq_diff += (a[c]-b[c])*(a[c]-b[c]);
  return sqrtf(sum_sq_diff);
}



static void
improve_rect_1bpp (GeglOperation *operation, GeglBuffer          *input,
              GeglBuffer          *output,
              const GeglRectangle *roi,
              int                  iterations,
              int                  chance)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *fmt_y_u8 = babl_format("Y' u8");
  GeglRectangle ref_rect  = {roi->x-1, roi->y-1, roi->width+3, roi->height+3};
  GeglRectangle bit_rect = {roi->x-2, roi->y-2, roi->width+5, roi->height+5};
  uint8_t *bits = malloc (bit_rect.width*bit_rect.height);
  uint8_t *ref  = malloc (ref_rect.width*ref_rect.height);
  gegl_buffer_get(output,  &bit_rect, 1.0f, fmt_y_u8, bits, bit_rect.width, GEGL_ABYSS_CLAMP);
  gegl_buffer_get(input, &ref_rect, 1.0f, fmt_y_u8, ref, ref_rect.width, GEGL_ABYSS_CLAMP);


  int prev_swaps = 1;

  for (int i = 0; i < iterations; i++)
  {
    int hswaps = 0;
    int vswaps = 0;
    int dswaps = 0;

  for (int y = 0; y < roi->height; y++)
  for (int x = 0; x < roi->width; x++)
  if ((random()%100) < chance){
    
    int sq_diff = 0;
    int sq_diff_hswap = 0;
    int sq_diff_vswap = 0;
    int sq_diff_dswap = 0;

    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int ref_val = ref[ref_rect.width * ((y+v)+1) + (x + u) + 1];
      int val = compute_val(o->center_bias, bits+2+(bit_rect.width*2), bit_rect.width, x+u, y+v);

      sq_diff += (val-ref_val)*(val-ref_val);
    }

#define DO_SWAP(rx,ry)\
    {int tmp = bits[bit_rect.width* (y+2) + x + 2+ bit_rect.width * ry+ rx];\
    bits[bit_rect.width* (y+2) + x + 2+bit_rect.width * ry + rx] = bits[bit_rect.width* (y+2) + x + 2];\
    bits[bit_rect.width* (y+2) + x + 2] = tmp;}

#define DO_HSWAP DO_SWAP(1,0)
#define DO_VSWAP DO_SWAP(0,1)
#define DO_DSWAP DO_SWAP(1,1)

    DO_HSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int ref_val = ref[ref_rect.width * ((y+v)+1) + (x + u) + 1];
      int val = compute_val(o->center_bias, bits+2+(bit_rect.width*2), bit_rect.width, x+u, y+v);

      sq_diff_hswap += (val-ref_val)*(val-ref_val);
    }
    DO_HSWAP;

    DO_VSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int ref_val = ref[ref_rect.width * ((y+v)+1) + (x + u) + 1];
      int val = compute_val(o->center_bias, bits+2+(bit_rect.width*2), bit_rect.width, x+u, y+v);

      sq_diff_vswap += (val-ref_val)*(val-ref_val);
    }
    DO_VSWAP;

    DO_DSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int ref_val = ref[ref_rect.width * ((y+v)+1) + (x + u) + 1];
      int val = compute_val(o->center_bias, bits+2+(bit_rect.width*2), bit_rect.width, x+u, y+v);

      sq_diff_dswap += (val-ref_val)*(val-ref_val);
    }
    DO_DSWAP;

    if (sq_diff_hswap < sq_diff && sq_diff_hswap <= sq_diff_vswap && sq_diff_hswap <= sq_diff_dswap)
    {
      hswaps++;
      DO_HSWAP;
    }
    else if (sq_diff_vswap < sq_diff && sq_diff_vswap <= sq_diff_dswap)
    {
      vswaps++;
      DO_VSWAP;
    }
    else if (sq_diff_dswap < sq_diff)
    {
      dswaps++;
      DO_DSWAP;
    }

  }
#if DEV_MODE
    printf("%i hswaps:%i vswaps:%i dswaps:%i\n", i, hswaps, vswaps, dswaps);
#endif

    if ((prev_swaps==0))
     break;

    prev_swaps = hswaps + vswaps + dswaps;
  }

  gegl_buffer_set(output, &bit_rect, 0, fmt_y_u8, bits, bit_rect.width);
  free (bits);
  free (ref);

#undef DO_HSWAP
#undef DO_VSWAP
#undef DO_DSWAP
#undef DO_SWAP
}

static void
improve_rect (GeglOperation *operation, GeglBuffer          *input,
              GeglBuffer          *output,
              const GeglRectangle *roi,
              int                  iterations,
              int                  chance)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  int bpp = babl_format_get_bytes_per_pixel (gegl_buffer_get_format(output));
  const Babl *fmt_raw = gegl_buffer_get_format(output);

  if (bpp == 1 && babl_get_name(fmt_raw)[0]=='Y')
  {
    improve_rect_1bpp (operation, input, output, roi, iterations, chance);
    return;
  }

  const Babl *fmt_rgb_u8 = babl_format("R'G'B' u8");

  GeglRectangle ref_rect  = {roi->x-1, roi->y-1, roi->width+3, roi->height+3};
  GeglRectangle bit_rect = {roi->x-2, roi->y-2, roi->width+5, roi->height+5};

  uint8_t *bits = malloc (bit_rect.width*bit_rect.height*bpp);
  uint8_t *bits_rgb = malloc (bit_rect.width*bit_rect.height*3);
  uint8_t *ref  = malloc (ref_rect.width*ref_rect.height*3);

  gegl_buffer_get(output,  &bit_rect, 1.0f, fmt_raw, bits, bit_rect.width*bpp, GEGL_ABYSS_CLAMP);
  gegl_buffer_get(output,  &bit_rect, 1.0f, fmt_rgb_u8, bits_rgb, bit_rect.width*3, GEGL_ABYSS_CLAMP);
  gegl_buffer_get(input, &ref_rect, 1.0f, fmt_rgb_u8, ref, ref_rect.width*3, GEGL_ABYSS_CLAMP);


  int prev_swaps = 1;

  for (int i = 0; i < iterations; i++)
  {
    int hswaps = 0;
    int vswaps = 0;
    int dswaps = 0;

  for (int y = 0; y < roi->height; y++)
  for (int x = 0; x < roi->width; x++)
  if ((random()%100) < chance){
    
    int sq_diff = 0;
    int sq_diff_hswap = 0;
    int sq_diff_vswap = 0;
    int sq_diff_dswap = 0;

    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      uint8_t computed_rgb[4];
      compute_rgb(o->center_bias, bits_rgb + 3*(2+(bit_rect.width)*2), bit_rect.width, x+u, y+v, &computed_rgb[0]);
      for (int c = 0; c<3; c++)
      {
        int val = ref[3*(ref_rect.width * ((y+v)+1) + (x + u) + 1)+c] - computed_rgb[c];
        sq_diff += val*val;
      }
    }

#define DO_SWAP(rx,ry) \
    {char tmp[16]; memcpy(tmp, &bits[bpp*(bit_rect.width* (y+2) + x + 2+bit_rect.width*ry+1*rx)], bpp);\
    memcpy(&bits[bpp*(bit_rect.width* (y+2) + x + 2+bit_rect.width*ry+rx)], \
           &bits[bpp*(bit_rect.width* (y+2) + x + 2)], bpp);\
    memcpy(&bits[bpp*(bit_rect.width* (y+2) + x + 2)], \
           tmp, bpp);\
    memcpy(tmp, &bits_rgb[3*(bit_rect.width* (y+2) + x + 2+bit_rect.width*ry+rx)], 3);\
    memcpy(&bits_rgb[3*(bit_rect.width* (y+2) + x + 2+bit_rect.width*ry+rx)], \
           &bits_rgb[3*(bit_rect.width* (y+2) + x + 2)], 3);\
    memcpy(&bits_rgb[3*(bit_rect.width* (y+2) + x + 2)],  tmp, 3);}

#define DO_HSWAP DO_SWAP(1,0)
#define DO_VSWAP DO_SWAP(0,1)
#define DO_DSWAP DO_SWAP(1,1)

    DO_HSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int sq_diff = 0;
      uint8_t computed_rgb[4];
      compute_rgb(o->center_bias, bits_rgb + 3*(2+(bit_rect.width)*2), bit_rect.width, x+u, y+v, &computed_rgb[0]);
      for (int c = 0; c<3; c++)
      {
        int val = ref[3*(ref_rect.width * ((y+v)+1) + (x + u) + 1)+c] - computed_rgb[c];
        sq_diff += val*val;
      }
      sq_diff_hswap += sq_diff;
    }
    DO_HSWAP;

    DO_VSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int sq_diff = 0;
      uint8_t computed_rgb[4];
      compute_rgb(o->center_bias, bits_rgb + 3*(2+(bit_rect.width)*2), bit_rect.width, x+u, y+v, &computed_rgb[0]);
      for (int c = 0; c<3; c++)
      {
        int val = ref[3*(ref_rect.width * ((y+v)+1) + (x + u) + 1)+c] - computed_rgb[c];
        sq_diff += val*val;
      }
      sq_diff_vswap += sq_diff;
    }
    DO_VSWAP;

    DO_DSWAP;
    for (int v = -1; v <= 2; v++)
    for (int u = -1; u <= 2; u++)
    {
      int sq_diff = 0;
      uint8_t computed_rgb[4];
      compute_rgb(o->center_bias, bits_rgb + 3*(2+(bit_rect.width)*2), bit_rect.width, x+u, y+v, &computed_rgb[0]);
      for (int c = 0; c<3; c++)
      {
        int val = ref[3*(ref_rect.width * ((y+v)+1) + (x + u) + 1)+c] - computed_rgb[c];
        sq_diff += val*val;
      }
      sq_diff_dswap += sq_diff;
    }
    DO_DSWAP;

    if (sq_diff_hswap < sq_diff && sq_diff_hswap <= sq_diff_vswap && sq_diff_hswap <= sq_diff_dswap)
    {
      hswaps++;
      DO_HSWAP;
    }
    else if (sq_diff_vswap < sq_diff && sq_diff_vswap <= sq_diff_dswap)
    {
      vswaps++;
      DO_VSWAP;
    }
    else if (sq_diff_dswap < sq_diff)
    {
      dswaps++;
      DO_DSWAP;
    }

  }
#if DEV_MODE
    printf("%i hswaps:%i vswaps:%i dswaps:%i\n", i, hswaps, vswaps, dswaps);
#endif

    if ((prev_swaps==0))
     break;

    prev_swaps = hswaps + vswaps + dswaps;
  }

  gegl_buffer_set(output, &bit_rect, 0, fmt_raw, bits, bit_rect.width*bpp);
  free (bits);
  free (bits_rgb);
  free (ref);

#undef DO_HSWAP
#undef DO_VSWAP
#undef DO_DSWAP
#undef DO_SWAP
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *arg_aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglBuffer          *aux = arg_aux;
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *fmt_y_u8 = babl_format("Y' u8");
  GeglBufferIterator *gi;

  if(!aux)
  {
    /* do our own first-stage dithering to requested number of gray-scale levels */
    aux = gegl_buffer_new (result, fmt_y_u8);
 
    gi = gegl_buffer_iterator_new (aux, result, 0, fmt_y_u8,
                                   GEGL_ACCESS_READ|GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE, 2);
    gegl_buffer_iterator_add (gi, input, result, 0, fmt_y_u8,
                              GEGL_ACCESS_READ, GEGL_ABYSS_NONE);



#if 1
//#define dither_mask(u,v) (((((u) ) ^ (v) * 149) * 1234) & 255)
#define dither_mask(u,v)   (((((u)+ 0* 67) + (v) * 236) * 119) & 255)
#else
#include "../common/blue-noise-data.inc"
//#define dither_mask(u,v) blue_noise_data_u8[0][((v) % 256) * 256 + ((u) % 256)]
#endif

  while (gegl_buffer_iterator_next (gi))
  {
      guint8 *data = (guint8*) gi->items[0].data;
      guint8 *in   = (guint8*) gi->items[1].data;
      GeglRectangle *roi = &gi->items[0].roi;
      int i = 0;
      int levels = o->levels - 1;
      int rlevels = 256/levels;
      for (int y = 0; y < roi->height; y++)
      for (int x = 0; x < roi->width; x++, i++)
       {
          int mask = (dither_mask(roi->x+x, roi->y+y) - 128)/levels;
          int value = in[i] + mask + rlevels/2;
          value = (value/(rlevels))*(rlevels);
          if (value < 0) value = 0;
          if (value > 255.0) value = 255;
          data[i] = value;
       }

   }
  }

  gegl_buffer_copy(aux, NULL, GEGL_ABYSS_NONE, output, NULL);

  //for (int i = 0; i < 2;i++)
  {
    int pixels_at_a_time = 65536;
    int chunk_height = pixels_at_a_time / result->width;
    if (chunk_height < 4)
      chunk_height = 4;

    int y = result->y;

    while (y < result->y + result->height)
    {
      GeglRectangle rect = {result->x, y, result->width, chunk_height};

      if (result->y + result->height - y < chunk_height)
        rect.height = result->y + result->height - y;
      improve_rect(operation, input, output, &rect, o->iterations, o->chance);
      y+= chunk_height - 4;
    }
  }

  // post-process - 
  if(aux){
    int bpp = babl_format_get_bytes_per_pixel (gegl_buffer_get_format(output));

    if (bpp == 1)
    {
    gi = gegl_buffer_iterator_new (output, result, 0, fmt_y_u8,
                                   GEGL_ACCESS_READ|GEGL_ACCESS_WRITE,
                                   GEGL_ABYSS_NONE, 3);
    gegl_buffer_iterator_add (gi, aux, result, 0, fmt_y_u8,
                              GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    gegl_buffer_iterator_add (gi, input, result, 0, fmt_y_u8,
                              GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
    while (gegl_buffer_iterator_next (gi))
    {
      guint8 *data  = (guint8*) gi->items[0].data;
      guint8 *aux   = (guint8*) gi->items[1].data;
      guint8 *in    = (guint8*) gi->items[2].data;

      GeglRectangle *roi = &gi->items[0].roi;
      int i = 0;
      for (int y = 0; y < roi->height; y++)
      for (int x = 0; x < roi->width; x++, i++)
      {
        int new_delta  = abs(data[i]-in[i]);
        int orig_delta = abs(aux[i]-in[i]);

        int min_neigh_delta = 255;
        int self = aux[i];
        for (int u = -1; u<=1;u++)
        for (int v = -1; v<=1;v++)
        {
          if (u + x >=0 && u + x <= roi->width &&
              v + y >=0 && v + y <= roi->height)
          {
            int neigh_delta = abs(aux[(y+v) * roi->width + (x+u)] - self);
            if (neigh_delta && neigh_delta < min_neigh_delta)
              min_neigh_delta = neigh_delta;
          }
        }

        if (orig_delta < new_delta - min_neigh_delta)
          data[i] = aux[i];
      }
    }
    }
    else
    {
      const Babl *fmt_rgb8 = babl_format("R'G'B' u8");
      gi = gegl_buffer_iterator_new (input, result, 0, fmt_rgb8,
                                     GEGL_ACCESS_READ,
                                     GEGL_ABYSS_NONE, 5);

      gegl_buffer_iterator_add (gi, aux, result, 0, NULL,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      gegl_buffer_iterator_add (gi, output, result, 0, NULL,
                                GEGL_ACCESS_READ|GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (gi, aux, result, 0, fmt_rgb8,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

      gegl_buffer_iterator_add (gi, output, result, 0, fmt_rgb8,
                                GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

    while (gegl_buffer_iterator_next (gi))
    {
      guint8 *in_rgb   = (guint8*) gi->items[0].data;
      guint8 *aux_raw  = (guint8*) gi->items[1].data;
      guint8 *data_raw = (guint8*) gi->items[2].data;
      guint8 *aux_rgb  = (guint8*) gi->items[3].data;
      guint8 *data_rgb = (guint8*) gi->items[4].data;

      GeglRectangle *roi = &gi->items[0].roi;
      int i = 0;
      for (int y = 0; y < roi->height; y++)
      for (int x = 0; x < roi->width; x++, i++)
      {
        int new_delta  = rgb_diff(&data_rgb[i*3], &in_rgb[i*3]);
        int orig_delta  = rgb_diff(&aux_rgb[i*3], &in_rgb[i*3]);

        int min_neigh_delta = 255;
        for (int v = -1; v<=1;v++)
        for (int u = -1; u<=1;u++)
        {
          if (u + x >=0 && u + x <= roi->width &&
              v + y >=0 && v + y <= roi->height)
          {
            int neigh_delta = rgb_diff(&aux_rgb[((y+v) * roi->width + (x+u))*3], &aux_rgb[i*3]);
            if (neigh_delta && neigh_delta < min_neigh_delta)
              min_neigh_delta = neigh_delta;
          }
        }

        if (orig_delta < new_delta - min_neigh_delta) // + a bit?
          memcpy(&data_raw[i*bpp], &aux_raw[i*bpp], bpp);
      }
    }
    }
  }

  if (aux != arg_aux)
    g_clear_object (&aux);

  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *self,
                   const GeglRectangle *roi)
{
  const GeglRectangle *in_rect =
          gegl_operation_source_get_bounding_box (self, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
  {
      return *in_rect;
  }
  return *roi;
}

static GeglRectangle
get_required_for_output (GeglOperation       *self,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return get_cached_region (self, roi);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class           = GEGL_OPERATION_CLASS (klass);
  composer_class            = GEGL_OPERATION_COMPOSER_CLASS (klass);
  operation_class->threaded = FALSE;
  operation_class->prepare  = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  composer_class->process   = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shuffle-search",
    "compat-name",        "gegl:shopt",
    "title",       _("Optimize Dither"),
    "categories",  "dither",
    "reference-hash", "e9de784b7a9c200bb7652b6b58a4c94a",
    "description", _("Shuffles pixels with neighbors to optimize dither, by shuffling neighboring pixels; if an image is provided as aux input, it is used as dithering starting point."),
    "gimp:menu-path", "<Image>/Colors",
    NULL);
}

#endif

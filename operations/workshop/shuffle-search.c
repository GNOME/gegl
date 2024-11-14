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

#if 1
#include <stdio.h>
#define DEV_MODE 1
#else
#define DEV_MODE 0
#endif

#ifdef GEGL_PROPERTIES

property_int  (iterations, _("Iterations"), 3)
               description(_("How many times to run optimization"))
               value_range (0, 64)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

property_int  (chance, _("Chance"), 100)
               description(_("Chance of doing optimization"))
               value_range (1, 100)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

property_int  (phase, _("Phase"), 0)
               value_range (0, 16)

property_int  (levels, _("Levels"), 3)
               description(_("Only used if no aux image is provided"))
               value_range (2, 255)


property_int  (center_bias, _("Center bias"), 18)
               value_range (0, 1024)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

property_seed (seed, _("Random seed"), rand)
#if DEV_MODE==0
               ui_meta("visible", "0")
#endif

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     shuffle_search
#define GEGL_OP_C_SOURCE shuffle-search.c

#include "gegl-op.h"


#include <stdio.h>

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("Y' u16", space));
}

static uint16_t compute_val(int levels, const uint16_t *bits, int stride, int x, int y)
{
  int count = 0;
  long int sum = 0;
  if (levels > 32) levels -= 1;
  if (levels > 16) levels -= 1;
  if (levels > 8) levels -= 1;
  if (levels > 4) levels -= 1;

  for (int v = y-1; v <= y+1; v++)
  for (int u = x-1; u <= x+1; u++)
  {
    int val = bits[v*stride+u];
    int contrib = 2+(u == x && v == y) * (levels);
    count += contrib;
    sum   += val * contrib;
  }
  return (sum)/count;
}

enum {
  MUTATE_NONE  = 0,

  MUTATE_SWAP_10,
  MUTATE_SWAP_01,
  MUTATE_SWAP_11,
  MUTATE_SWAP_D,
  MUTATE_HGROW,
  MUTATE_HSHRINK,
  MUTATE_VGROW,
  MUTATE_VSHRINK,

  MUTATE_INC,
  MUTATE_DEC,
  MUTATE_INC_10,
  MUTATE_DEC_10,
  MUTATE_INC_01,
  MUTATE_DEC_01,
  MUTATE_INC_11,
  MUTATE_DEC_11,
  MUTATE_COUNT
};


static uint16_t linear[65536];
static uint16_t nonlinear[65536];
static int to_linear (int in_val)
{
  if (in_val<0) return 0;
  if (in_val>65535) return 65535;
  return linear[in_val];
}

static int to_nonlinear (int in_val)
{
  if (in_val<0) return 0;
  if (in_val>65535) return 65535;
  return nonlinear[in_val];
}

static int dec_val(int levels, int in_val)
{
  int amt = 65536 / (levels-1);
  int res = to_linear(to_nonlinear(in_val) - amt);
  if (res >0) return res;
  return 0;
}

static int inc_val(int levels, int in_val)
{
  int amt = 65536 / (levels-1);
  int res = to_linear(to_nonlinear(in_val) + amt);
  if (res < 65535) return res;
  return 65535;
}

static void
improve_rect (GeglOperation *operation, GeglBuffer          *input,
              GeglBuffer          *output,
              const GeglRectangle *roi,
              int                  iterations,
              int                  chance)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *fmt_y_u16 = babl_format("Y u16");
  GeglRectangle ref_rect  = {roi->x-1, roi->y-1, roi->width+3, roi->height+3};
  GeglRectangle bit_rect = {roi->x-2, roi->y-2, roi->width+5, roi->height+5};
  uint16_t *bits = malloc (bit_rect.width*bit_rect.height*2);
  uint16_t *ref  = malloc (ref_rect.width*ref_rect.height*2);
  gegl_buffer_get(output,  &bit_rect, 1.0f, fmt_y_u16, bits, bit_rect.width*2, GEGL_ABYSS_CLAMP);
  gegl_buffer_get(input, &ref_rect, 1.0f, fmt_y_u16, ref, ref_rect.width*2, GEGL_ABYSS_CLAMP);

  int prev_swaps = 1;

  for (int i = 0; i < iterations && prev_swaps; i++)
  {
    int hswaps = 0;
    int vswaps = 0;
    int dswaps = 0;
    int dswap2s = 0;
    int mutates = 0;
    int grows = 0;

  for (int y = 0; y < roi->height; y+=1)
  for (int x = 0; x < roi->width; x+=1)
  if ((gegl_random_int_range(o->rand, x, y, 0, i, 0, 100)) < chance)
  {
    long int score[MUTATE_COUNT] = {0,};
    
    uint16_t backup[4];
    uint16_t backup01[4];
    uint16_t backup11[4];
    uint16_t backup10[4];

#define PXAT(rx,ry) bits[bit_rect.width* (y+2) + x + 2+ bit_rect.width * (ry)+ (rx)]

#define DO_BACKUP() \
    {memcpy(backup,   &PXAT(0,0),2);\
     memcpy(backup01, &PXAT(0,1),2);\
     memcpy(backup10, &PXAT(1,0),2);\
     memcpy(backup11, &PXAT(1,1),2);}
#define DO_RESTORE() \
    {memcpy(&PXAT(0,0),backup,2);\
     memcpy(&PXAT(0,1),backup01,2);\
     memcpy(&PXAT(1,0),backup10,2);\
     memcpy(&PXAT(1,1),backup11,2);}

#define DO_INC(rx,ry) PXAT(rx,ry) = inc_val(o->levels, PXAT(rx,ry));
#define DO_DEC(rx,ry) PXAT(rx,ry) = dec_val(o->levels, PXAT(rx,ry));


#define MEASURE(score_name) \
    for (int v = -1; v <= 2; v++) \
    for (int u = -1; u <= 2; u++) \
    { \
      int ref_val = ref[ref_rect.width * ((y+v)+1) + (x + u) + 1];\
      int val = compute_val(o->levels, bits+2+(bit_rect.width*2), bit_rect.width, x+u, y+v);\
      score[score_name] += (val-ref_val)*(val-ref_val);\
    }
 
    MEASURE(MUTATE_NONE);

    DO_BACKUP();
    { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(1,0); PXAT(1,0)=tmp;}
    MEASURE(MUTATE_SWAP_10);
    DO_RESTORE()


    DO_BACKUP();
    { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(0,1); PXAT(0,1)=tmp;}
    MEASURE(MUTATE_SWAP_01);
    DO_RESTORE()

    DO_BACKUP();
    { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(1,1); PXAT(1,1)=tmp;}
    MEASURE(MUTATE_SWAP_11);
    DO_RESTORE()

    DO_BACKUP();
    { uint16_t tmp = PXAT(1,0); PXAT(1,0)=PXAT(1,0); PXAT(1,0)=tmp;}
    MEASURE(MUTATE_SWAP_D);
    DO_RESTORE()

#if 0
    DO_BACKUP();
    PXAT(1,0)=PXAT(0,0); 
    MEASURE(MUTATE_HGROW);
    DO_RESTORE()

    DO_BACKUP();
    PXAT(0,0)=PXAT(1,0); 
    MEASURE(MUTATE_HSHRINK);
    DO_RESTORE()

    DO_BACKUP();
    PXAT(0,1)=PXAT(0,0); 
    MEASURE(MUTATE_VGROW);
    DO_RESTORE()

    DO_BACKUP();
    PXAT(0,0)=PXAT(0,1); 
    MEASURE(MUTATE_VSHRINK);
    DO_RESTORE()
#endif

    {
    float factor = 1.2f;
    score[MUTATE_HGROW] *= factor;
    score[MUTATE_VGROW] *= factor;
    score[MUTATE_HSHRINK] *= factor;
    score[MUTATE_VSHRINK] *= factor;
    }

    {

#if 1
    if (PXAT(0,0)<65535)
    {
    DO_BACKUP();
    DO_INC(0,0)
    MEASURE(MUTATE_INC);
    DO_RESTORE()
    }

    if (PXAT(0,0)>0)
    {
      DO_BACKUP();
      DO_DEC(0,0)
      MEASURE(MUTATE_DEC);
      DO_RESTORE()
    }

    {float factor = 1.6f;
    score[MUTATE_INC] *= factor;
    score[MUTATE_DEC] *= factor;
    }
#endif

#if 1
    if (PXAT(0,0)>0 && PXAT(1,0)<65535)
    {
      DO_BACKUP();
      DO_INC(1,0);DO_DEC(0,0)
      MEASURE(MUTATE_INC_10);
      DO_RESTORE();
    }

    if (PXAT(0,0)>0 && PXAT(0,1)<65535)
    {
      DO_BACKUP();
      DO_INC(0,1);DO_DEC(0,0)
      MEASURE(MUTATE_INC_01);
      DO_RESTORE();
    }

    if (PXAT(0,0)>0 && PXAT(1,1)<65535)
    {
      DO_BACKUP();
      DO_INC(1,1);DO_DEC(0,0)
      MEASURE(MUTATE_INC_11);
      DO_RESTORE();
    }


    if (PXAT(0,0)<65535 && PXAT(1,0)>0)
    {
      DO_BACKUP();
      DO_DEC(1,0);DO_INC(0,0)
      MEASURE(MUTATE_DEC_10);
      DO_RESTORE();
    }

    if (PXAT(0,0)<65535 && PXAT(0,1)>0)
    {
      DO_BACKUP();
      DO_DEC(0,1);DO_INC(0,0)
      MEASURE(MUTATE_DEC_01);
      DO_RESTORE();
    }

    if (PXAT(0,0)<65535 && PXAT(1,1)>0)
    {
      DO_BACKUP();
      DO_DEC(1,1);DO_INC(0,0)
      MEASURE(MUTATE_DEC_11);
      DO_RESTORE();
    }

    float factor = 1.5f;
    score[MUTATE_INC_01] *= factor;
    score[MUTATE_INC_10] *= factor;
    score[MUTATE_INC_11] *= factor;
    score[MUTATE_DEC_01] *= factor;
    score[MUTATE_DEC_10] *= factor;
    score[MUTATE_DEC_11] *= factor;
#endif

    }

    int best = 0;
    for (int j = 0; j < MUTATE_COUNT;j++)
      if (score[j] && score[j] < score[best]) best = j;

    switch(best)
    {
       case MUTATE_NONE: break;
       case MUTATE_SWAP_10:
        { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(1,0); PXAT(1,0)=tmp;}
        hswaps++;
        break;
       case MUTATE_SWAP_01:
        { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(0,1); PXAT(0,1)=tmp;}
        vswaps++;
        break;
       case MUTATE_SWAP_11:
        { uint16_t tmp = PXAT(0,0); PXAT(0,0)=PXAT(1,1); PXAT(1,1)=tmp;}
        dswaps++;
        break;
       case MUTATE_SWAP_D:
        { uint16_t tmp = PXAT(1,0); PXAT(1,0)=PXAT(0,1); PXAT(0,1)=tmp;}
        dswaps++;
        break;
       case MUTATE_DEC:     mutates++; DO_DEC(0,0); break;
       case MUTATE_INC:     mutates++; DO_INC(0,0); break;
       case MUTATE_INC_10:  mutates++; DO_INC(1,0); DO_DEC(0,0); break;
       case MUTATE_INC_01:  mutates++; DO_INC(0,1); DO_DEC(0,0); break;
       case MUTATE_INC_11:  mutates++; DO_INC(1,1); DO_DEC(0,0); break;
       case MUTATE_HGROW:   grows++;   PXAT(1,0)=PXAT(0,0); break;
       case MUTATE_VGROW:   grows++;   PXAT(0,1)=PXAT(0,0); break;
       case MUTATE_VSHRINK: grows++;   PXAT(0,0)=PXAT(0,1); break;
       case MUTATE_HSHRINK: grows++;   PXAT(0,0)=PXAT(1,0); break;
       case MUTATE_DEC_10:  mutates++; DO_DEC(1,0); DO_INC(0,0); break;
       case MUTATE_DEC_01:  mutates++; DO_DEC(0,1); DO_INC(0,0); break;
       case MUTATE_DEC_11:  mutates++; DO_DEC(1,1); DO_INC(0,0); break;
    }
  }
#if DEV_MODE
    printf("%i hswap:%i vswap:%i dswap:%i dswap2:%i mutates:%i\n", i, hswaps, vswaps, dswaps, dswap2s, mutates);
#endif

    prev_swaps = hswaps + vswaps + dswaps + dswap2s + mutates + grows;
  }

  gegl_buffer_set(output, &bit_rect, 0, fmt_y_u16, bits, bit_rect.width * 2);
  free (bits);
  free (ref);

#undef DO_UNSET
#undef MEASURE
}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *fmt_y_u16 = babl_format("Y' u16");
  GeglBufferIterator *gi;

  GeglRectangle needed_rect = *result;
  needed_rect.x -= 8;
  needed_rect.y -= 8;
  needed_rect.width  += 16;
  needed_rect.height += 16;

  GeglBuffer *temp = gegl_buffer_new (&needed_rect, fmt_y_u16);

  gi = gegl_buffer_iterator_new (temp, &needed_rect, 0, fmt_y_u16,
                                 GEGL_ACCESS_READ|GEGL_ACCESS_WRITE,
                                 GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (gi, input, &needed_rect, 0, fmt_y_u16,
                            GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

#if 1
//#define dither_mask(u,v,c) (((((u)+ c* 67) ^ (v) * 149) * 1234) & 255)
#define dither_mask(u,v,c)   (((((u)+ c* 67) + (v) * 236) * 119) & 255)
#else
#include "../common/blue-noise-data.inc"
#define dither_mask(u,v,c) blue_noise_data_u8[c][((v) % 256) * 256 + ((u) % 256)]
#endif
//#undef dither_mask
//#define dither_mask(u,v,c)   127

  while (gegl_buffer_iterator_next (gi))
  {
      guint16 *data = (guint16*) gi->items[0].data;
      guint16 *in   = (guint16*) gi->items[1].data;
      GeglRectangle *roi = &gi->items[0].roi;
      int i = 0;
      int levels = o->levels - 1;
      int rlevels = 65536/levels;
      if (levels == 1)
      for (int y = 0; y < roi->height; y++)
      for (int x = 0; x < roi->width; x++, i++)
       {
          int input = to_linear(in[i]);
#if 0
          mask = ((mask * mask) -32767)/levels;
#else
          int mask = ((dither_mask(roi->x+x, roi->y+y, o->phase)/255.0)*65535-32767)/(levels);
#endif
          int value = input + mask;
          value = ((value + rlevels/2) /rlevels)*(rlevels);


          if (value < 0) data[i] = 0;
          else if (value > 65535) data[i] = 65535;
          else data[i] = to_nonlinear(value);
       }
      else
      for (int y = 0; y < roi->height; y++)
      for (int x = 0; x < roi->width; x++, i++)
       {
          int input = in[i];
#if 0
          mask = ((mask * mask) -32767)/levels;
#else
          int mask = ((dither_mask(roi->x+x, roi->y+y, o->phase)/255.0)*65535-32767)/(levels);
#endif
          int value = input + mask;
          value = ((value + rlevels/2) /rlevels)*(rlevels);


          if (value < 0) data[i] = 0;
          else if (value > 65535) data[i] = 65535;
          else data[i] = value;
       }
   }

  if (o->iterations)
  {
    GeglRectangle left   = { result->x-4, result->y-4, 8, result->height+8 };
    GeglRectangle right  = { result->x+result->width-4, result->y-4, 8, result->height+8 };
    GeglRectangle top    = { result->x-4, result->y-4, result->width+8, 8 };
    GeglRectangle bottom = { result->x-4, result->y+result->height-4, result->width+8, 8 };

    improve_rect(operation, input, temp, &left, 2, 100);
    improve_rect(operation, input, temp, &right, 2, 100);
    improve_rect(operation, input, temp, &top, 2, 100);
    improve_rect(operation, input, temp, &bottom, 2, 100);
    improve_rect(operation, input, temp, result, o->iterations+o->levels, o->chance);
  }

  gegl_buffer_copy(temp, result, GEGL_ABYSS_NONE, output, result);
  g_clear_object (&temp);

  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  const GeglRectangle *bounds = gegl_operation_source_get_bounding_box (operation, "input");
  // force chunking to 64x64 aligned blocks
  GeglRectangle rect = *roi;
  
  int dx = (roi->x & 0x2f);

  rect.x -= dx;
  rect.width  += dx;

  dx = (8192 - rect.width) & 0x2f;

  rect.width += dx;


  int dy = (roi->y & 0x2f);
  rect.y -= dy;
  rect.height += dy;

  dy = (8192 - rect.height) & 0x2f;

  rect.height += dy;
#if 1
  if (rect.x + rect.width > bounds->x + bounds->width)
    rect.width = bounds->x + bounds->width - rect.x;
  if (rect.y + rect.height > bounds->y + bounds->height)
    rect.height = bounds->y + bounds->height - rect.y;
#endif
  return rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  const GeglRectangle *in_rect =
          gegl_operation_source_get_bounding_box (operation, "input");

  if (in_rect && ! gegl_rectangle_is_infinite_plane (in_rect))
  {
    GeglRectangle rect = get_cached_region (operation, roi);
    return rect;
  }
  return *roi;
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass      *operation_class=GEGL_OPERATION_CLASS(klass);
  GeglOperationFilterClass*filter_class   =GEGL_OPERATION_FILTER_CLASS(klass);

  {
    uint16_t *tmp = calloc(2,65536);
    for (int i = 0; i < 65536; i++) tmp[i]=i;
    babl_process(babl_fish(babl_format("Y' u16"), babl_format("Y u16")),
                 tmp, linear, 65536);
    babl_process(babl_fish(babl_format("Y u16"), babl_format("Y' u16")),
                 tmp, nonlinear, 65536);
    free(tmp);
  }

  operation_class->cache_policy            = GEGL_CACHE_POLICY_ALWAYS;
  operation_class->threaded                = FALSE;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;
  filter_class->process                    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shuffle-search",
    "title",       _("Optimize Dither"),
    "categories",  "dither",
    "reference-hash", "e9de784b7a9c200bb7652b6b58a4c94a",
    "description", _("Shuffles pixels, and quantization with neighbors to optimize dither, by shuffling neighboring pixels."),
    "gimp:menu-path", "<Image>/Colors",
    NULL);
}

#endif

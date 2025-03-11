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
 * Copyright 2006, 2018 Øyvind Kolås <pippin@gimp.org>
 */


#include "config.h"
#include <glib/gi18n-lib.h>
#include "ctx.h"

#ifdef GEGL_PROPERTIES


#define SAMPLE \
"duration 5.0   # duration of this page/scene\n" \
"\n" \
"rgba 0 0 0 (0=0 3=1.0) paint\n" \
"\n" \
"\n" \
"save\n" \
"globalAlpha (0=0 1=1.0)\n" \
"\n" \
"translate 50% 50% \n" \
"scale 100^ 100^\n" \
"\n" \
"translate -0.6 -0.6 scale 0.041 0.041 g M0 0m24.277 20.074m-.473.020m-1.607 1.364m.148.745m.097.182c5.014.017.027.034.041.051c.495.602 1.252.616 1.736.726c.484.110.843.406 1.020.729l-.010 0c.149.270.440-1.029.334-1.932c-.085-.725-.417-1.263-.840-1.616z gray0F G g M0 0m24.679 1.686c.029 0.056 0.081 0c.099.016.217.122.258.242c.041.120 1.672 8.369-.655 13.117c-2.327 4.748-7.474 6.185-10.439 6.165c-4.982.073-9.310-1.706-11.300-5.760c2.161-.073 2.879-2.166 2.914-3.909c.011-.538.854-6.389 1.047-6.646c.053-.065.131-.032.169.027c.810 1.266 1.555 1.920 2.648 2.518c1.737.750 2.868 1.026 5.430.570c2.563-.456 6.783-1.977 9.550-6.130c.106-.136.209-.186.296-.196z rgb.549.502.451F G g g Y9.339 11.583O2.856 3.200B0 0 1 0 6.283 0G gray1F G g g Y9.955 11.701O1.718 2.089B0 0 1 0 6.283 0G gray0F G g B9.961 10.547.783 0 6.283 0gray1F G g W.979.202 0-.204.979 0 0 0 1g Y.016 12.121O2.432 3.136B0 0 1 0 6.283 0G gray0F G g B2.168 10.276 1.324 0 6.283 0gray1F G g M0 0m18.543 16.076c-.174-.121.034-.311.411-.324c.226-.010.513.048.813.219c.809.462.836 1.031.571 1.154gray0F G g M0 0m19.337 16.213c-1.594 2.213-4.031 3.547-8.009 2.984c-.519-.069-.913.615 1.453.712c2.966.121 5.525-.764 7.267-3.182z gray0F G g M0 0m18.995 17.907c-.661-.276-1.568.662-1.225.914c.527.338 2.364 1.513 2.752 1.719c.450.239 1.092-.188 1.092-.188l.200-.377c0 0-.010-.771-.456-1.010c-.291-.154-1.504-.686-2.355-1.055c0 0-.010 0-.010 0z rgb.949.518.051F G g M0 0m21.071 20.510c.084.297.380.162.559.262c.179.100.221.422.517.336c.296-.085.685-.784.601-1.081c-.084-.297-.380-.162-.559-.262c-.179-.100-.221-.422-.517-.337c-.297.085-.685.784-.601 1.082z gray.733F G g g Y15.632 11.590O3.750 3.828B0 0 1 0 6.283 0G gray1F G g g Y16.539 11.730O2.322 2.545B0 0 1 0 6.283 0G gray0F G g B16.539 10.344.997 0 6.283 0gray1F G g M0 0m23.353 19.831c-.354.005-.671.119-.880.341c-.613.639-.497 1.610-.029 2.216c-.078-.261-.171-.718-.033-.816c.160-.115.532.539.838.350c.305-.189-.289-.712.010-.959c.312-.247.734.444.997.133c.225-.274-.505-.683-.390-.872c.106-.174.610-.005.858.118c-.417-.348-.924-.517-1.372-.511z rgb.549.502.451F G\n" \
"restore\n" \
"\n" \
"save\n" \
"translate (0=50 1=75 2=33  4=65 5=50)% (0=50 1=75 2=23 3=40 4=14 5=50)%\n" \
"scale (0=30 3=60 5=30)^ (0=30 3=60 5=30)^\n" \
"\n" \
"translate -0.5 -0.5\n" \
"\n" \
"rgba 1 1 1 0.4\n" \
"m 0.43956786,0.90788066 c 0.0195929,0.0102943 0.0716181,0.0218038 0.10361884,-0.0167646 L 0.93768705,0.37887837 c 0.019925,-0.0342044 -0.00963,-0.0544608 -0.0308834,-0.0508084 -0.17965502,0.0285588 -0.35466092,-0.055125 -0.45096394,-0.21253089 -0.0176003,-0.02988716 -0.0594422,-0.01560777 -0.0594422,0.0139473 0,0.0591101 0.003321,0.49845135 0.001991,0.70699722 0.00039042,0.0283487 0.0157362,0.0529866 0.0408456,0.070733 F\n" \
"f 0.0525 0 0.9905 0\n" \
"p 0.0 1.0 1.0 0.66 1.0\n" \
"p 0.2 1 0.66 0 1.0\n" \
"p 0.5 1 0.0 0 1.0\n" \
"p 1.0 0.4 0.0 0.53 1.0\n" \
"m 0.39772584,0.91850721 h -0.0664159 c -0.15408489,0 -0.27894675,-0.12486192 -0.27894675,-0.2789468 0,-0.15408489 0.12486186,-0.27861466 0.27894675,-0.27894675 l 0.18585599,0.0000662 c 0.0111839,0.00017138 0.0158287,0.001542 0.0263337,0.0134822 0.11733258,0.14373102 0.3018009,0.36870115 0.3942639,0.49195316 0.0185394,0.0332794 -0.0106225,0.0505515 -0.0228143,0.0505207 F\n" \
"f 0.697 0.17 0.4318 0.884\n" \
"p 0.0 0.26 0.26 1 1.0\n" \
"p 0.3 0 1 1 0.4\n" \
"p 1.0 0 1 0.26 1.0\n" \
"m 0.43956786,0.90788066 c 0.0195929,0.0102943 0.0716181,0.0218038 0.10361884,-0.0167646 L 0.93768705,0.37887837 c 0.019925,-0.0342044 -0.00963,-0.0544608 -0.0308834,-0.0508084 -0.17965502,0.0285588 -0.35466092,-0.055125 -0.45096394,-0.21253089 -0.0176003,-0.02988716 -0.0594422,-0.01560777 -0.0594422,0.0139473 0,0.0591101 0.003321,0.49845135 0.001991,0.70699722 0.0039042,0.0283487 0.0157362,0.0529866 0.0408456,0.070733 F\n" \
"restore\n" \
"\n" \
"newPage\n" \
"duration 1\n" \
"gray (0=0 1=1) paint\n" \
"\n" \
"newPage\n" \
"duration 1\n" \
"rgba 0 0 0 (0=1.0 1=0) paint\n" \
"\n" \
"newPage\n" \
"duration 2\n" \
"save\n" \
" rgba\n" \
" conicGradient 50% 50%  (0=0 5=14) 7\n" \
" addStop 0.0 1 0 0 1\n" \
" addStop (0=0.5 1=0.8  2=0.5) 1 0 0 0.0\n" \
" addStop 1.0 1 0 0 1.0\n" \
"\n" \
" paint\n" \
"restore\n" \
"\n" \
"newPage\n" \
"rgb 1 1 0 paint\n" \
"duration 1\n" 

property_boolean (u8, "8bit sRGB ", FALSE)
  description ("Render using R'aG'AB'a u8 (rather than linear RaGaBaA float); the user->device space color maping is identity by default; thus changing the meaning of colors set in the script.")
  ui_meta ("visible", "0")

property_string (d, "ctx data", SAMPLE)
  description ("A string containing a ctx protocol document.")
  ui_meta ("multiline", "true")

property_boolean (play, "play", FALSE)
property_boolean (loop_scene, "loop scene", FALSE)
property_int (page, "page", 0)
  value_range (0, 1000)
  ui_range (0, 100)
property_double (time, "time", 0.0)
  value_range (0.0, 120.0)
  ui_range (0.0, 10.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     ctx_script
#define GEGL_OP_C_SOURCE ctx-script.c

#include "gegl-op.h"

typedef struct
{
  int   width;
  int   height;
  char *str;
  Ctx  *drawing;
  const guint8 *icc;
  int   icc_length;
  GTimer *timer;
  guint playback_handle;
} State;

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  defined = { 0, 0, 512, 512 };
  GeglRectangle *in_rect;
  in_rect =  gegl_operation_source_get_bounding_box (operation, "input");
  if (in_rect)
    { 
      return *in_rect;
      gegl_rectangle_bounding_box (&defined, &defined, in_rect);
    }
  return defined;
}


static gboolean playback_cb (gpointer user_data)
{
  GeglOperation *operation = user_data;
  GeglProperties    *o     = GEGL_PROPERTIES (operation);
  State *state             = o->user_data;
  float elapsed = g_timer_elapsed (state->timer, NULL);
  if (operation->node)
  {
    gegl_node_set (operation->node, "time", o->time + elapsed, "page", o->page, NULL);
  }
  g_timer_start (state->timer);
  return TRUE;
}


static void
prepare (GeglOperation *operation)
{
  GeglProperties    *o       = GEGL_PROPERTIES (operation);
  State *state;
  if (!o->user_data) {
     o->user_data = g_malloc0 (sizeof(State));
  }
  state = o->user_data;

  const Babl *input_format  = gegl_operation_get_source_format (operation, "input");
  const Babl *input_space   = input_format?babl_format_get_space (input_format):NULL;

  if (o->u8)
  {
    gegl_operation_set_format (operation, "output", babl_format_with_space ("R'aG'aB'aA u8", input_space));
  }
  else
  {
    gegl_operation_set_format (operation, "output", babl_format_with_space ("R'aG'aB'aA float", input_space));
  }

  GeglRectangle bounds = get_bounding_box (operation);


  if (state->width != bounds.width ||
      state->height != bounds.height ||
      state->str == NULL ||
      strcmp(state->str, o->d) || 
      1)
  {
     if (state->drawing)
     {
       ctx_destroy (state->drawing);
     }
     if (state->str)
     {
       g_free (state->str);
     }
     state->width = bounds.width;
     state->height = bounds.height;
     state->str = g_strdup (o->d);
     state->drawing = ctx_new_drawlist (bounds.width, bounds.height);

     float time = o->time;
     int   scene_no = o->page;

     ctx_parse_animation (state->drawing, state->str, &time, &scene_no);
     if (scene_no != o->page)
     {
	if (o->loop_scene)
	{
	  time = o->time = 0.0f;
          scene_no = o->page;
          ctx_parse_animation (state->drawing, state->str, &time, &scene_no);
	}
	else
	{
	  o->page = scene_no;
	  o->time = time;
	}
     }

     state->icc = NULL;
     state->icc_length = 0;
     if (input_space && input_space != babl_space("sRGB"))
     {
       state->icc = (uint8_t*)babl_space_get_icc (input_space, &state->icc_length);
     }
  }

  if (o->play)
  {
    if (!state->timer)
    {
      state->timer = g_timer_new ();
    }
    if (state->playback_handle == 0)
    {
      state->playback_handle = g_idle_add (playback_cb, operation);
      g_timer_start (state->timer);
    }
  }
  else
  {
    if (state->playback_handle)
    {
       g_source_remove (state->playback_handle);
       state->playback_handle = 0;
    }
  }

}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  State *state = o->user_data;
  const Babl *format =  gegl_operation_get_format (operation, "output");

  int bpp = o->u8?4:16;
  guchar *data = gegl_malloc(result->width*result->height*bpp);
  Ctx *ctx;
  gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, output, result);
  gegl_buffer_get (input, result, 1.0, format, data, result->width*bpp, GEGL_ABYSS_NONE);
  ctx = ctx_new_for_framebuffer (data, result->width, result->height,
                                 result->width * bpp,
				 o->u8?CTX_FORMAT_RGBA8:CTX_FORMAT_RGBAF);

  if (state->icc)
  {
    ctx_colorspace (ctx, CTX_COLOR_SPACE_DEVICE_RGB,
                    state->icc, state->icc_length);
  }

  ctx_translate (ctx, -result->x, -result->y);
  ctx_save(ctx);
  ctx_render_ctx (state->drawing, ctx);
  ctx_restore(ctx);

  ctx_destroy (ctx);

  gegl_buffer_set (output, result, 0, format, data, result->width*bpp);
  gegl_free (data);

  return  TRUE;
}

static void state_destroy (State *state)
{
  if (state->str)
  {
    g_free (state->str);
  }
  if (state->drawing)
  {
    ctx_destroy (state->drawing);
  }
  if (state->playback_handle)
  {
    g_source_remove (state->playback_handle);
    state->playback_handle = 0;
  }

  g_free (state);
}

static void
dispose (GObject *object)
{
  GeglProperties  *o = GEGL_PROPERTIES (object);
  if (o->user_data)
  {
    state_destroy (o->user_data);
    o->user_data = NULL;
  }
  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static GeglSplitStrategy
get_split_strategy (GeglOperation        *operation,
                    GeglOperationContext *context,   
                    const gchar          *output_prop,
                    const GeglRectangle  *result,
                    gint                  level)
{
  return GEGL_SPLIT_STRATEGY_HORIZONTAL;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class;
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  filter_class->get_split_strategy = get_split_strategy;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  object_class->dispose    = dispose;
  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:ctx-script",
    "title",       "Ctx script",
    "categories",  "render:vector",
    "description", "Renders a ctx vector graphics script",
    NULL);
}

#endif

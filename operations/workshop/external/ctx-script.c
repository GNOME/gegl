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
"save\n" \
"scale 50^ 50^\n"\
"rgba 1 1 1 0.4\n"\
"m 0.43956786,0.90788066 c 0.0195929,0.0102943 0.0716181,0.0218038 0.10361884,-0.0167646 L 0.93768705,0.37887837 c 0.019925,-0.0342044 -0.00963,-0.0544608 -0.0308834,-0.0508084 -0.17965502,0.0285588 -0.35466092,-0.055125 -0.45096394,-0.21253089 -0.0176003,-0.02988716 -0.0594422,-0.01560777 -0.0594422,0.0139473 0,0.0591101 0.003321,0.49845135 0.001991,0.70699722 0.00039042,0.0283487 0.0157362,0.0529866 0.0408456,0.070733 F\n"\
"f 0.0525 0 0.9905 0\n"\
"p 0.0 1.0 1.0 0.66 1.0\n"\
"p 0.2 1 0.66 0 1.0\n"\
"p 0.5 1 0.0 0 1.0\n"\
"p 1.0 0.4 0.0 0.53 1.0\n"\
"m 0.39772584,0.91850721 h -0.0664159 c -0.15408489,0 -0.27894675,-0.12486192 -0.27894675,-0.2789468 0,-0.15408489 0.12486186,-0.27861466 0.27894675,-0.27894675 l 0.18585599,0.0000662 c 0.0111839,0.00017138 0.0158287,0.001542 0.0263337,0.0134822 0.11733258,0.14373102 0.3018009,0.36870115 0.3942639,0.49195316 0.0185394,0.0332794 -0.0106225,0.0505515 -0.0228143,0.0505207 F\n"\
"f 0.697 0.17 0.4318 0.884\n"\
"p 0.0 0.26 0.26 1 1.0\n"\
"p 0.3 0 1 1 0.4\n"\
"p 1.0 0 1 0.26 1.0\n"\
"m 0.43956786,0.90788066 c 0.0195929,0.0102943 0.0716181,0.0218038 0.10361884,-0.0167646 L 0.93768705,0.37887837 c 0.019925,-0.0342044 -0.00963,-0.0544608 -0.0308834,-0.0508084 -0.17965502,0.0285588 -0.35466092,-0.055125 -0.45096394,-0.21253089 -0.0176003,-0.02988716 -0.0594422,-0.01560777 -0.0594422,0.0139473 0,0.0591101 0.003321,0.49845135 0.001991,0.70699722 0.0039042,0.0283487 0.0157362,0.0529866 0.0408456,0.070733 F\n"\
"restore\n" \
"save\n" \
"translate 50% 50%\n" \
"scale 50^ 50^\n" \
"translate -0.6 -0.6\n" \
"scale 0.041 0.041\n"\
"g g m24.230 20.050m-0.473 .020m-1.607 1.364m.148 .745m.097 .182c.013 .017 .027 .034 .041 .051c.495 .602 1.252 .616 1.736 .726c.484 .110 .843 .406 1.020 .729l-0.010 0c.149 .270 .440-1.029 .334-1.932c-0.085-0.725-0.417-1.263-0.840-1.616l-0.450-0.269z gray0F G\n" \
"m24.680 1.650l.081 0c.099 .016 .217 .122 .258 .242c.041 .120 1.672 8.369-0.655 13.117c-2.327 4.748-7.474 6.185-10.439 6.165c-4.982 .073-9.310-1.706-11.300-5.760c2.161-0.073 2.879-2.166 2.914-3.909c.011 -0.538 .854 -6.389 1.047 -6.646c.053 -0.065 .131 -0.032 .169 .027c.810 1.266 1.555 1.920 2.648 2.518c1.737 .750 2.868 1.026 5.430 .570c2.563 -0.456 6.783-1.977 9.550-6.130c.106-0.136 .209-0.186 .296-0.196z rgb.549 .502 .451F\n" \
"g Y9.340 11.550O2.856 3.200B0 0 1 0 6.283 0 z G gray1 F\n" \
"g Y9.950 11.669O1.719 2.089B0 0 1 0 6.283 0 z G gray0 F\n" \
"g Y9.960 10.520O.783 .783B0 0 1 0 6.283 0 z G gray1 F \n" \
"g W .979 .202 0-0.204 .979 0 0 0 1\n" \
"g Y.023 12.092O2.432 3.136B0 0 1 0 6.283 0G gray0F G\n" \
"B2.170 10.240 1.324 0 6.283 0gray1F\n" \
"g m18.540 16.040c-0.174-0.121 .034-0.311 .411-0.324c.226-0.010 .513 .048 .813 .219c.809 .462 .836 1.031 .571 1.154c-0.186 .086-0.902-0.449-0.902-0.449c0 0-0.522-0.342-0.893-0.600z gray0F G g M0 0m19.340 16.180c-1.594 2.213 -4.031 3.547 -8.009 2.984c-0.519 -0.069 -0.913 .615 1.453 .712c2.966 .121 5.525 -0.764 7.267 -3.182z gray0F G g M0 0m18.990 17.880c-0.661 -0.276 -1.568 .662 -1.225 .914c.527 .338 2.364 1.513 2.752 1.719c.450 .239 1.092 -0.188 1.092 -0.188l.200 -0.377c0 0 -0.010 -0.771 -0.456 -1.010c-0.291 -0.154 -1.504-0.686-2.355-1.055l-0.010 0z rgb.949 .518 .051F G g M0 0m21.070 20.480c.084 .297 .380 .162 .559 .262c.179 .100 .221 .422 .517 .336c.296-0.085 .685-0.784 .601-1.081c-0.084-0.297-0.380-0.162-0.559-0.262c-0.179-0.100-0.221-0.422-0.517-0.337c-0.297 .085-0.685 .784-0.601 1.082z gray.733F G\n"\
"g Y15.630 11.560O3.749 3.827B0 0 1 0 6.283 0z G gray1 F\n"\
"g Y16.540 11.700O2.321 2.545B0 0 1 0 6.283 0z G gray0 F\n"\
"g Y16.540 10.310O.996 .996B0 0 1 0 6.283 0z G gray1 F\n"\
"g m23.330 19.800c-0.354 .005-0.671 .119-0.880 .341c-0.614 .640-0.497 1.613-0.027 2.219c-0.066-0.221-0.120-0.452-0.100-0.683c0-0.192 .208-0.167 .294-0.047c.149 .116 .290 .283 .492 .296c.158 .006 .253-0.169 .190-0.307c-0.047-0.184-0.17-0.356-0.145-0.553c.027-0.154 .212-0.228 .348-0.168c.198 .062 .354 .235 .567 .250c.150 .003 .260-0.176 .181-0.307c-0.099 -0.217 -0.319 -0.349 -0.415 -0.566c-0.010 -0.026 -0.010 -0.057 .010 -0.081l0 0c.091 -0.112 .256 -0.072 .379 -0.052c.166 .039 .330 .093 .482 .171c-0.418 -0.349 -0.925 -0.519 -1.374 -0.513z rgb.549 .502 .451F G\nG\n" \
"restore\n" 

property_boolean (u8, "8bit sRGB ", TRUE)
  description ("Render using R'aG'AB'a u8 (rather than linear RaGaBaA float); the user->device space color maping is identity by default; thus changing the meaning of colors set in the script.")

property_string (d, "Vector data", SAMPLE)
  description ("A string containing a ctx protocol drawing.")
  ui_meta ("multiline", "true")


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
  int     icc_length;
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


static void
prepare (GeglOperation *operation)
{
  GeglProperties    *o       = GEGL_PROPERTIES (operation);
  State *state;
  if (!o->user_data)
     o->user_data = g_malloc0 (sizeof(State));
  state = o->user_data;

  const Babl *input_format  = gegl_operation_get_source_format (operation, "input");
  const Babl *input_space   = input_format?babl_format_get_space (input_format):NULL;

  if (o->u8)
    gegl_operation_set_format (operation, "output", babl_format_with_space ("R'aG'aB'aA u8", input_space));
  else
    gegl_operation_set_format (operation, "output", babl_format_with_space ("R'aG'aB'aA float", input_space));

  GeglRectangle bounds = get_bounding_box (operation);


  if (state->width != bounds.width ||
      state->height != bounds.height ||
      state->str == NULL ||
      strcmp(state->str, o->d))
  {
     if (state->drawing)
       ctx_destroy (state->drawing);
     if (state->str)
       g_free (state->str);
     state->width = bounds.width;
     state->height = bounds.height;
     state->str = g_strdup (o->d);
     state->drawing = ctx_new_drawlist (bounds.width, bounds.height);
     ctx_parse (state->drawing, state->str);
     state->icc = NULL;
     state->icc_length = 0;
     if (input_space)
       state->icc = babl_space_get_icc (input_space, &state->icc_length);
  }

}

#include <stdio.h>


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
  if (state->str) g_free (state->str);
  if (state->drawing) ctx_destroy (state->drawing);
  g_free (state);
}

static void
dispose (GObject *object)
{
  GeglProperties  *o = GEGL_PROPERTIES (object);
  state_destroy (o->user_data);
  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
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

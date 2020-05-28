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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2013 Daniel Sabo
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (value, _("Opacity"), 1.0)
    description (_("Global opacity value that is always used on top of the optional auxiliary input buffer."))
    value_range (-10.0, 10.0)
    ui_range    (0.0, 1.0)

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     opacity
#define GEGL_OP_C_SOURCE opacity.c

#include "gegl-op.h"

#include <stdio.h>

#define EPSILON 1e-6f

static void
prepare (GeglOperation *self)
{
  const Babl *space = gegl_operation_get_source_space (self, "input");
  const Babl *fmt = gegl_operation_get_source_format (self, "input");

  fmt = gegl_babl_variant (fmt, GEGL_BABL_VARIANT_ALPHA);

  gegl_operation_set_format (self, "input", fmt);
  gegl_operation_set_format (self, "output", fmt);
  gegl_operation_set_format (self, "aux", babl_format_with_space ("Y float", space));

  return;
}

static inline gfloat int_fabsf (const gfloat x)
{
  union {gfloat f; guint32 i;} u = {x};
  u.i &= 0x7fffffff;
  return u.f;
}

static void
process_premultiplied_float (GeglOperation       *op,
                      void                *in_buf,
                      void                *aux_buf,
                      void                *out_buf,
                      glong                samples,
                      const GeglRectangle *roi,
                      gint                 level,
                      gint                 components)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gfloat value = GEGL_PROPERTIES (op)->value;

  if (aux == NULL)
    {
      while (samples--)
        {
          gint j;
          for (j=0; j<components; j++)
            out[j] = in[j] * value;
          in  += components;
          out += components;
        }
    }
  else if (int_fabsf (value - 1.0f) <= EPSILON)
    while (samples--)
      {
        gint j;
        for (j=0; j<components; j++)
          out[j] = in[j] * (*aux);
        in  += components;
        out += components;
        aux += 1;
      }
  else
    while (samples--)
      {
        gfloat v = (*aux) * value;
        gint j;
        for (j=0; j<components; j++)
          out[j] = in[j] * v;
        in  += components;
        out += components;
        aux += 1;
      }
}

static void
process_with_alpha_float (GeglOperation       *op,
                          void                *in_buf,
                          void                *aux_buf,
                          void                *out_buf,
                          glong                samples,
                          const GeglRectangle *roi,
                          gint                 level,
                          gint                 components)
{
  gfloat *in = in_buf;
  gfloat *out = out_buf;
  gfloat *aux = aux_buf;
  gint ccomponents = components - 1;
  gfloat value = GEGL_PROPERTIES (op)->value;

  if (aux == NULL)
    {
      while (samples--)
        {
          gint j;
          for (j=0; j<ccomponents; j++)
            out[j] = in[j];
          out[ccomponents] = in[ccomponents] * value;
          in  += components;
          out += components;
        }
    }
  else if (int_fabsf (value - 1.0f) <= EPSILON)
    while (samples--)
      {
        gint j;
        for (j=0; j<ccomponents; j++)
          out[j] = in[j];
        out[ccomponents] = in[ccomponents] * (*aux);
        in  += components;
        out += components;
        aux += 1;
      }
  else
    while (samples--)
      {
        gfloat v = (*aux) * value;
        gint j;
        for (j=0; j<ccomponents; j++)
          out[j] = in[j];
        out[ccomponents] = in[ccomponents] * v;
        in  += components;
        out += components;
        aux += 1;
      }
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *aux_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  const Babl *format = gegl_operation_get_format (op, "output");
  int components = babl_format_get_n_components (format);

  if (babl_get_model_flags (format) & BABL_MODEL_FLAG_ASSOCIATED)
    process_premultiplied_float (op, in_buf, aux_buf, out_buf, samples, roi, level, components);
  else
    process_with_alpha_float (op, in_buf, aux_buf, out_buf, samples, roi, level, components);

  return TRUE;
}

#include "opencl/gegl-cl.h"

#include "opencl/opacity.cl.h"

static GeglClRunData *cl_data = NULL;

static gboolean
cl_process (GeglOperation       *op,
            cl_mem               in_tex,
            cl_mem               aux_tex,
            cl_mem               out_tex,
            size_t               global_worksize,
            const GeglRectangle *roi,
            gint                 level)
{
  cl_int cl_err = 0;
  int kernel;
  gfloat value;
  const Babl *fmt;

  if (!cl_data)
    {
      const char *kernel_name[] = {"gegl_opacity_RaGaBaA_float", "gegl_opacity_RGBA_float", NULL};
      cl_data = gegl_cl_compile_and_build (opacity_cl_source, kernel_name);
    }
  if (!cl_data) return TRUE;

  fmt = gegl_operation_get_format(op, "input");
  value = GEGL_PROPERTIES (op)->value;

  kernel = ((babl_get_model_flags(fmt) & BABL_MODEL_FLAG_ASSOCIATED) == 0)? 1 : 0;

  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 0, sizeof(cl_mem),   (void*)&in_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 1, sizeof(cl_mem),   (aux_tex)? (void*)&aux_tex : NULL);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 2, sizeof(cl_mem),   (void*)&out_tex);
  CL_CHECK;
  cl_err = gegl_clSetKernelArg(cl_data->kernel[kernel], 3, sizeof(cl_float), (void*)&value);
  CL_CHECK;

  cl_err = gegl_clEnqueueNDRangeKernel(gegl_cl_get_command_queue (),
                                        cl_data->kernel[kernel], 1,
                                        NULL, &global_worksize, NULL,
                                        0, NULL, NULL);
  CL_CHECK;

  return FALSE;

error:
  return TRUE;
}

/* Fast path when opacity is a no-op
 */
static gboolean operation_process (GeglOperation        *operation,
                                   GeglOperationContext *context,
                                   const gchar          *output_prop,
                                   const GeglRectangle  *result,
                                   gint                  level)
{
  GeglOperationClass  *operation_class;
  gpointer in, aux;
  gfloat value = GEGL_PROPERTIES (operation)->value;
  operation_class = GEGL_OPERATION_CLASS (gegl_op_parent_class);

  /* get the raw values this does not increase the reference count */
  in = gegl_operation_context_get_object (context, "input");
  aux = gegl_operation_context_get_object (context, "aux");

  if (in && !aux && int_fabsf (value - 1.0f) <= EPSILON)
    {
      gegl_operation_context_take_object (context, "output",
                                          g_object_ref (G_OBJECT (in)));
      return TRUE;
    }
  /* chain up, which will create the needed buffers for our actual
   * process function
   */
  return operation_class->process (operation, context, output_prop, result,
                                  gegl_operation_context_get_level (context));
}


static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_composer_class;

  operation_class      = GEGL_OPERATION_CLASS (klass);
  point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->process = operation_process;
  point_composer_class->process = process;
  point_composer_class->cl_process = cl_process;

  operation_class->opencl_support = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:opacity",
    "categories" , "transparency",
    "title",       _("Opacity"),
    "reference-hash", "b20e8c1d7bb20af95f724191feb10103",
    "description",
          _("Weights the opacity of the input both the value of the aux"
            " input and the global value property."),
    NULL);
}

#endif

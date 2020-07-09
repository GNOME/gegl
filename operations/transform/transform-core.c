/* This file is part of GEGL
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
 * Copyright 2006 Philip Lafleur
 *           2006-2018 Øyvind Kolås
 *           2009 Martin Nordholts
 *           2010 Debarshi Ray
 *           2011 Mikael Magnusson
 *           2011-2012 Massimo Valentini
 *           2011 Adam Turcotte
 *           2012 Kevin Cozens
 *           2012 Nicolas Robidoux
 */


#include "config.h"
#include <glib/gi18n-lib.h>

#include <gegl.h>
#include <gegl-plugin.h>

#include "opencl/gegl-buffer-cl-cache.h"

#include "gegl-config.h"

#include "transform-core.h"
#include "module.h"

/*
 * Use to determine if key transform matrix coefficients are close
 * enough to zero or integers.
 */
#define GEGL_TRANSFORM_CORE_EPSILON ((gdouble) 0.0000001)

enum
{
  PROP_ORIGIN_X = 1,
  PROP_ORIGIN_Y,
  PROP_NEAR_Z,
  PROP_SAMPLER
};

static void          gegl_transform_get_property                 (GObject              *object,
                                                                  guint                 prop_id,
                                                                  GValue               *value,
                                                                  GParamSpec           *pspec);
static void          gegl_transform_set_property                 (GObject              *object,
                                                                  guint                 prop_id,
                                                                  const GValue         *value,
                                                                  GParamSpec           *pspec);
static void          gegl_transform_bounding_box                 (const gdouble        *points,
                                                                  const gint            num_points,
                                                                  const GeglRectangle  *context_rect,
                                                                  GeglRectangle        *output);
static gint          gegl_transform_depth_clip                   (const GeglMatrix3    *matrix,
                                                                  gdouble               near_z,
                                                                  const gdouble        *vertices,
                                                                  gint                  n_vertices,
                                                                  gdouble              *output);
static gboolean      gegl_transform_scanline_limits              (const GeglMatrix3    *inverse,
                                                                  gdouble               inverse_near_z,
                                                                  const GeglRectangle  *bounding_box,
                                                                  gdouble               u0,
                                                                  gdouble               v0,
                                                                  gdouble               w0,
                                                                  gint                 *first,
                                                                  gint                 *last);
static gboolean      gegl_transform_is_intermediate_node         (OpTransform          *transform);
static gboolean      gegl_transform_is_composite_node            (OpTransform          *transform);
static void          gegl_transform_get_source_matrix            (OpTransform          *transform,
                                                                  GeglMatrix3          *output);
static GeglRectangle gegl_transform_get_bounding_box             (GeglOperation        *op);
static GeglRectangle gegl_transform_get_invalidated_by_change    (GeglOperation        *operation,
                                                                  const gchar          *input_pad,
                                                                  const GeglRectangle  *input_region);
static GeglRectangle gegl_transform_get_required_for_output      (GeglOperation        *self,
                                                                  const gchar          *input_pad,
                                                                  const GeglRectangle  *region);
static gboolean      gegl_transform_process                      (GeglOperation        *operation,
                                                                  GeglOperationContext *context,
                                                                  const gchar          *output_prop,
                                                                  const GeglRectangle  *result,
                                                                  gint                  level);
static GeglNode     *gegl_transform_detect                       (GeglOperation        *operation,
                                                                  gint                  x,
                                                                  gint                  y);

static gboolean      gegl_transform_matrix3_allow_fast_translate (GeglMatrix3          *matrix);
static void          gegl_transform_create_composite_matrix      (OpTransform          *transform,
                                                                  GeglMatrix3          *matrix);

/* ************************* */

static void         op_transform_init                            (OpTransform          *self);
static void         op_transform_class_init                      (OpTransformClass     *klass);
static gpointer     op_transform_parent_class = NULL;

static void
op_transform_class_intern_init (gpointer klass)
{
  op_transform_parent_class = g_type_class_peek_parent (klass);
  op_transform_class_init ((OpTransformClass *) klass);
}

GType
op_transform_get_type (void)
{
  static GType g_define_type_id = 0;
  if (G_UNLIKELY (g_define_type_id == 0))
    {
      static const GTypeInfo g_define_type_info =
        {
          sizeof (OpTransformClass),
          (GBaseInitFunc) NULL,
          (GBaseFinalizeFunc) NULL,
          (GClassInitFunc) op_transform_class_intern_init,
          (GClassFinalizeFunc) NULL,
          NULL,   /* class_data */
          sizeof (OpTransform),
          0,      /* n_preallocs */
          (GInstanceInitFunc) op_transform_init,
          NULL    /* value_table */
        };

      g_define_type_id =
        gegl_module_register_type (transform_module_get_module (),
                                   GEGL_TYPE_OPERATION_FILTER,
                                   "GeglOpPlugIn-transform-core",
                                   &g_define_type_info, 0);
    }
  return g_define_type_id;
}

static void
gegl_transform_prepare (GeglOperation *operation)
{
  const Babl *source_format = gegl_operation_get_source_format (operation, "input");
  const Babl *space = source_format?babl_format_get_space (source_format):NULL;
  GeglMatrix3  matrix;
  OpTransform *transform = (OpTransform *) operation;
  const Babl *format = source_format;

  gegl_transform_create_composite_matrix (transform, &matrix);

  /* The identity matrix is also a fast translate matrix. */
  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_transform_matrix3_allow_fast_translate (&matrix) ||
      (gegl_matrix3_is_translate (&matrix) &&
       transform->sampler == GEGL_SAMPLER_NEAREST))
    {
    }
  else if (transform->sampler == GEGL_SAMPLER_NEAREST)
    {
      if (source_format && ! babl_format_has_alpha (source_format))
        format = gegl_babl_variant (source_format, GEGL_BABL_VARIANT_ALPHA);
    }
  else
    {
      BablModelFlag model_flags = babl_get_model_flags (source_format);
      if (model_flags & BABL_MODEL_FLAG_CMYK)
        format = babl_format_with_space ("camayakaA float", space);
      else if (model_flags & BABL_MODEL_FLAG_GRAY)
        format = babl_format_with_space ("YaA float", space);
      else
        format = babl_format_with_space ("RaGaBaA float", space);
    }

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static void
op_transform_class_init (OpTransformClass *klass)
{
  GObjectClass         *gobject_class = G_OBJECT_CLASS (klass);
  GeglOperationClass   *op_class      = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property         = gegl_transform_set_property;
  gobject_class->get_property         = gegl_transform_get_property;

  op_class->get_invalidated_by_change =
    gegl_transform_get_invalidated_by_change;
  op_class->get_bounding_box          = gegl_transform_get_bounding_box;
  op_class->get_required_for_output   = gegl_transform_get_required_for_output;
  op_class->detect                    = gegl_transform_detect;
  op_class->process                   = gegl_transform_process;
  op_class->prepare                   = gegl_transform_prepare;
  op_class->threaded                  = TRUE;

  klass->create_matrix                = NULL;
  klass->get_abyss_policy             = NULL;

  gegl_operation_class_set_key (op_class, "categories", "transform");

  g_object_class_install_property (gobject_class, PROP_ORIGIN_X,
                                   g_param_spec_double (
                                     "origin-x",
                                     _("Origin-x"),
                                     _("X coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_ORIGIN_Y,
                                   g_param_spec_double (
                                     "origin-y",
                                     _("Origin-y"),
                                     _("Y coordinate of origin"),
                                     -G_MAXDOUBLE, G_MAXDOUBLE,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_NEAR_Z,
                                   g_param_spec_double (
                                     "near-z",
                                     _("Near-z"),
                                     _("Z coordinate of the near clipping plane"),
                                     0., 1.,
                                     0.,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_SAMPLER,
                                   g_param_spec_enum (
                                     "sampler",
                                     _("Sampler"),
                                     _("Sampler used internally"),
                                     gegl_sampler_type_get_type (),
                                     GEGL_SAMPLER_LINEAR,
                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
op_transform_init (OpTransform *self)
{
}

static void
gegl_transform_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  OpTransform *self = OP_TRANSFORM (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      g_value_set_double (value, self->origin_x);
      break;
    case PROP_ORIGIN_Y:
      g_value_set_double (value, self->origin_y);
      break;
    case PROP_NEAR_Z:
      g_value_set_double (value, self->near_z);
      break;
    case PROP_SAMPLER:
      g_value_set_enum (value, self->sampler);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_transform_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  OpTransform *self = OP_TRANSFORM (object);

  switch (prop_id)
    {
    case PROP_ORIGIN_X:
      self->origin_x = g_value_get_double (value);
      break;
    case PROP_ORIGIN_Y:
      self->origin_y = g_value_get_double (value);
      break;
    case PROP_NEAR_Z:
      self->near_z = g_value_get_double (value);
      break;
    case PROP_SAMPLER:
      self->sampler = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gegl_transform_create_matrix (OpTransform *transform,
                              GeglMatrix3 *matrix)
{
  gegl_matrix3_identity (matrix);

  if (OP_TRANSFORM_GET_CLASS (transform))
    {
      OP_TRANSFORM_GET_CLASS (transform)->create_matrix (transform, matrix);

      gegl_matrix3_round_error (matrix);
    }
}

static void
gegl_transform_create_composite_matrix (OpTransform *transform,
                                        GeglMatrix3 *matrix)
{
  gegl_transform_create_matrix (transform, matrix);

  if (transform->origin_x || transform->origin_y)
    gegl_matrix3_originate (matrix, transform->origin_x, transform->origin_y);

  if (gegl_transform_is_composite_node (transform))
    {
      GeglMatrix3 source;

      gegl_transform_get_source_matrix (transform, &source);
      gegl_matrix3_multiply (matrix, &source, matrix);
    }
}

static GeglAbyssPolicy
gegl_transform_get_abyss_policy (OpTransform *transform)
{
  if (OP_TRANSFORM_GET_CLASS (transform)->get_abyss_policy)
    return OP_TRANSFORM_GET_CLASS (transform)->get_abyss_policy (transform);

  return GEGL_ABYSS_NONE;
}

static void
gegl_transform_bounding_box (const gdouble       *points,
                             const gint           num_points,
                             const GeglRectangle *context_rect,
                             GeglRectangle       *output)
{
  /*
   * Take the points defined by consecutive pairs of gdoubles as
   * absolute positions, that is, positions in the coordinate system
   * with origin at the center of the pixel with index [0][0].
   *
   * Compute from these the smallest rectangle of pixel indices such
   * that the absolute positions of the four outer corners of the four
   * outer pixels contains all the given points.
   *
   * If the points are pixel centers (which are shifted by 0.5
   * w.r.t. their pixel indices), the returned GEGLRectangle is the
   * exactly the one that would be obtained by min-ing and max-ing the
   * corresponding indices.
   *
   * This function purposely deviates from the "boundary between two
   * pixel areas is owned by the right/bottom one" convention. This
   * may require adding a bit of elbow room in the results when used
   * to give enough data to computations.
   */

  const GeglRectangle pixel_rect = {0, 0, 1, 1};
  gint                i,
                      num_coords;
  gdouble             min_x,
                      min_y,
                      max_x,
                      max_y;

  if (num_points < 1)
    return;

  if (! context_rect)
    context_rect = &pixel_rect;

  num_coords = 2 * num_points;

  min_x = max_x = points [0];
  min_y = max_y = points [1];

  for (i = 2; i < num_coords;)
    {
      if (points [i] < min_x)
        min_x = points [i];
      else if (points [i] > max_x)
        max_x = points [i];
      i++;

      if (points [i] < min_y)
        min_y = points [i];
      else if (points [i] > max_y)
        max_y = points [i];
      i++;
    }

  /*
   * Clamp the coordinates so that they don't overflow when converting to int,
   * with wide enough margins for the sampler context rect.
   */
  min_x = CLAMP (min_x,
                 G_MININT / 2 - context_rect->x,
                 G_MAXINT / 2 + context_rect->width + context_rect->x - 1);
  min_y = CLAMP (min_y,
                 G_MININT / 2 - context_rect->y,
                 G_MAXINT / 2 + context_rect->height + context_rect->y - 1);
  max_x = CLAMP (max_x,
                 G_MININT / 2 - context_rect->x,
                 G_MAXINT / 2 + context_rect->width + context_rect->x - 1);
  max_y = CLAMP (max_y,
                 G_MININT / 2 - context_rect->y,
                 G_MAXINT / 2 + context_rect->height + context_rect->y - 1);

  output->x = (gint) floor ((double) min_x);
  output->y = (gint) floor ((double) min_y);
  /*
   * Warning: width may be 0 when min_x=max_x=integer. Same with
   * height.
   *
   * If you decide to enforce the "boundary between two pixels is
   * owned by the right/bottom one" policy, replace ceil by floor +
   * (gint) 1. This often enlarges result by one pixel at the right
   * and bottom.
   */
  /* output->width  = (gint) floor ((double) max_x) + ((gint) 1 - output->x); */
  /* output->height = (gint) floor ((double) max_y) + ((gint) 1 - output->y); */
  output->width  = (gint) ceil ((double) max_x) - output->x;
  output->height = (gint) ceil ((double) max_y) - output->y;
}

/*
 * Clip the polygon defined by 'vertices' to the near-plane/horizon, according
 * to the transformation defined by 'matrix'.  Store the vertices of the
 * resulting polygon in 'output', and return their count.  If the polygon is
 * convex, the number of output vertices is at most 'n_vertices + 1'.
 */
static gint
gegl_transform_depth_clip (const GeglMatrix3 *matrix,
                           gdouble            near_z,
                           const gdouble     *vertices,
                           gint               n_vertices,
                           gdouble           *output)
{
  const gdouble a = matrix->coeff[2][0];
  const gdouble b = matrix->coeff[2][1];
  const gdouble c = matrix->coeff[2][2] -
                    MAX (near_z, GEGL_TRANSFORM_CORE_EPSILON);

  gint          n = 0;
  gint          i;

  for (i = 0; i < 2 * n_vertices; i += 2)
    {
      const gdouble x1 = vertices[i];
      const gdouble y1 = vertices[i + 1];
      const gdouble x2 = vertices[(i + 2) % (2 * n_vertices)];
      const gdouble y2 = vertices[(i + 3) % (2 * n_vertices)];

      gdouble w1 = a * x1 + b * y1 + c;
      gdouble w2 = a * x2 + b * y2 + c;

      if (near_z > 1.0)
        {
          w1 = -w1;
          w2 = -w2;
        }

      if (w1 >= 0.0)
        {
          output[n++] = x1;
          output[n++] = y1;
        }

      if ((w1 >= 0.0) != (w2 >= 0.0))
        {
          output[n++] = (b * (x1 * y2 - x2 * y1) - c * (x2 - x1)) /
                        (a * (x2 - x1) + b * (y2 - y1));
          output[n++] = (a * (y1 * x2 - y2 * x1) - c * (y2 - y1)) /
                        (a * (x2 - x1) + b * (y2 - y1));
        }
    }

  return n / 2;
}

/*
 * Calculate the limits, '[first, last)', of the scanline whose initial
 * coordinates are '(u0, v0, w0)', given the inverse transform 'inverse', and
 * the input bounding box 'bounding_box' (including any margins needed for
 * the sampler).
 *
 * Returns TRUE if the scanline should be rasterized.
 */
static gboolean
gegl_transform_scanline_limits (const GeglMatrix3   *inverse,
                                gdouble              inverse_near_z,
                                const GeglRectangle *bounding_box,
                                gdouble              u0,
                                gdouble              v0,
                                gdouble              w0,
                                gint                *first,
                                gint                *last)
{
  const gdouble a  = inverse->coeff[0][0];
  const gdouble b  = inverse->coeff[1][0];
  const gdouble c  = inverse->coeff[2][0];

  const gdouble x1 = bounding_box->x;
  const gdouble y1 = bounding_box->y;
  const gdouble x2 = bounding_box->x + bounding_box->width;
  const gdouble y2 = bounding_box->y + bounding_box->height;

  gdouble       i1 = *first;
  gdouble       i2 = *last;

  inverse_near_z = MIN (inverse_near_z, 1.0 / GEGL_TRANSFORM_CORE_EPSILON);

  /*
   * Left edge.
   */
  if (a - x1 * c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (x1 * w0 - u0) / (a - x1 * c);

      i1 = MAX (i1, min_i);
    }
  else if (a - x1 * c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (x1 * w0 - u0) / (a - x1 * c);

      i2 = MIN (i2, max_i);
    }
  else if (u0 < x1 * w0)
    {
      return FALSE;
    }

  /*
   * Top edge.
   */
  if (b - y1 * c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (y1 * w0 - v0) / (b - y1 * c);

      i1 = MAX (i1, min_i);
    }
  else if (b - y1 * c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (y1 * w0 - v0) / (b - y1 * c);

      i2 = MIN (i2, max_i);
    }
  else if (v0 < y1 * w0)
    {
      return FALSE;
    }

  /*
   * Right edge.
   */
  if (a - x2 * c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (x2 * w0 - u0) / (a - x2 * c);

      i2 = MIN (i2, max_i);
    }
  else if (a - x2 * c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (x2 * w0 - u0) / (a - x2 * c);

      i1 = MAX (i1, min_i);
    }
  else if (u0 > x2 * w0)
    {
      return FALSE;
    }

  /*
   * Bottom edge.
   */
  if (b - y2 * c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (y2 * w0 - v0) / (b - y2 * c);

      i2 = MIN (i2, max_i);
    }
  else if (b - y2 * c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (y2 * w0 - v0) / (b - y2 * c);

      i1 = MAX (i1, min_i);
    }
  else if (v0 > y2 * w0)
    {
      return FALSE;
    }

  /*
   * Add a 1-pixel border, to accommodate for box filtering.
   */
  i1 = MAX (i1 - 1.0, *first);
  i2 = MIN (i2 + 1.0, *last);

  /*
   * Horizon.
   */
  if (c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (GEGL_TRANSFORM_CORE_EPSILON - w0) / c;

      i1 = MAX (i1, min_i);
    }
  else if (c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (GEGL_TRANSFORM_CORE_EPSILON - w0) / c;

      i2 = MIN (i2, max_i);
    }
  else if (w0 < GEGL_TRANSFORM_CORE_EPSILON)
    {
      return FALSE;
    }

  /*
   * Near plane.
   */
  if (c > GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble max_i = (inverse_near_z - w0) / c;

      i2 = MIN (i2, max_i);
    }
  else if (c < -GEGL_TRANSFORM_CORE_EPSILON)
    {
      const gdouble min_i = (inverse_near_z - w0) / c;

      i1 = MAX (i1, min_i);
    }
  else if (w0 > inverse_near_z)
    {
      return FALSE;
    }

  i1 = CLAMP (i1, G_MININT / 2, G_MAXINT / 2);
  i2 = CLAMP (i2, G_MININT / 2, G_MAXINT / 2);

  *first = ceil (i1);
  *last  = ceil (i2);

  return *first < *last;
}

static gboolean
gegl_transform_is_intermediate_node (OpTransform *transform)
{
  GeglOperation *op = GEGL_OPERATION (transform);

  gboolean is_intermediate = TRUE;

  GeglNode **consumers = NULL;

  if (0 == gegl_node_get_consumers (op->node, "output", &consumers, NULL))
    {
      is_intermediate = FALSE;
    }
  else
    {
      int i;
      for (i = 0; consumers[i]; ++i)
        {
          GeglOperation *sink = gegl_node_get_gegl_operation (consumers[i]);

          if (! IS_OP_TRANSFORM (sink)                              ||
              transform->sampler != OP_TRANSFORM (sink)->sampler    ||
              gegl_transform_get_abyss_policy (transform) !=
              gegl_transform_get_abyss_policy (OP_TRANSFORM (sink)) ||
              transform->near_z != OP_TRANSFORM (sink)->near_z)
            {
              is_intermediate = FALSE;
              break;
            }
        }
    }

  g_free (consumers);

  return is_intermediate;
}

static gboolean
gegl_transform_is_composite_node (OpTransform *transform)
{
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglNode *source_node;
  GeglOperation *source;

  source_node = gegl_node_get_producer (op->node, "input", NULL);

  if (!source_node)
    return FALSE;

  source = gegl_node_get_gegl_operation (source_node);

  return (IS_OP_TRANSFORM (source) &&
          gegl_transform_is_intermediate_node (OP_TRANSFORM (source)));
}

static void
gegl_transform_get_source_matrix (OpTransform *transform,
                                  GeglMatrix3 *output)
{
  GeglOperation *op = GEGL_OPERATION (transform);
  GeglNode *source_node;
  GeglOperation *source;

  source_node = gegl_node_get_producer (op->node, "input", NULL);

  g_assert (source_node);

  source = gegl_node_get_gegl_operation (source_node);
  g_assert (IS_OP_TRANSFORM (source));

  gegl_transform_create_composite_matrix (OP_TRANSFORM (source), output);
  /*gegl_matrix3_copy (output, OP_TRANSFORM (source)->matrix);*/
}

static GeglRectangle
gegl_transform_get_bounding_box (GeglOperation *op)
{
  OpTransform  *transform = OP_TRANSFORM (op);
  GeglMatrix3   matrix;
  GeglRectangle in_rect   = {0,0,0,0},
                have_rect = {0,0,0,0};
  gdouble       vertices [8];
  gdouble       have_points [10];
  gint          n_have_points;
  gint          i;

  /*
   * Gets the bounding box of the forward mapped outer input pixel
   * corners that correspond to the involved indices, where "bounding"
   * is defined by output pixel areas. The output space indices of the
   * bounding output pixels is returned.
   *
   * Note: Don't forget that the boundary between two pixel areas is
   * "owned" by the pixel to the right/bottom.
   */

  if (gegl_operation_source_get_bounding_box (op, "input"))
    in_rect = *gegl_operation_source_get_bounding_box (op, "input");

  if (gegl_rectangle_is_empty (&in_rect) ||
      gegl_rectangle_is_infinite_plane (&in_rect))
    return in_rect;

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    return in_rect;

  /*
   * Convert indices to absolute positions of the left and top outer
   * pixel corners.
   */
  vertices [0] = in_rect.x;
  vertices [1] = in_rect.y;

  /*
   * When there are n pixels, their outer corners are distant by n (1
   * more than the distance between the outer pixel centers).
   */
  vertices [2] = vertices [0] + in_rect.width;
  vertices [3] = vertices [1];

  vertices [4] = vertices [2];
  vertices [5] = vertices [3] + in_rect.height;

  vertices [6] = vertices [0];
  vertices [7] = vertices [5];

  /*
   * Clip polygon to the near plane.
   */
  n_have_points = gegl_transform_depth_clip (&matrix, transform->near_z,
                                             vertices, 4, have_points);

  if (n_have_points > 1)
    {
      for (i = 0; i < 2 * n_have_points; i += 2)
        gegl_matrix3_transform_point (&matrix,
                                      have_points + i,
                                      have_points + i + 1);

      gegl_transform_bounding_box (have_points, n_have_points, NULL,
                                   &have_rect);
    }

  return have_rect;
}

static GeglNode *
gegl_transform_detect (GeglOperation *operation,
                       gint           x,
                       gint           y)
{
  OpTransform   *transform = OP_TRANSFORM (operation);
  GeglNode      *source_node;
  GeglMatrix3    inverse;
  gdouble        need_points [2];
  GeglOperation *source;

  /*
   * transform_detect figures out which pixel in the input most
   * closely corresponds to the pixel with index [x][y] in the output.
   * Ties are resolved toward the right and bottom.
   */

  source_node = gegl_operation_get_source_node (operation, "input");

  if (!source_node)
    return NULL;

  source = gegl_node_get_gegl_operation (source_node);

  if (!source)
    return NULL;

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return gegl_operation_detect (source, x, y);

  gegl_transform_create_composite_matrix (transform, &inverse);
  gegl_matrix3_invert (&inverse);

  /*
   * The center of the pixel with index [x][y] is at (x+.5,y+.5).
   */
  need_points [0] = x + (gdouble) 0.5;
  need_points [1] = y + (gdouble) 0.5;

  gegl_matrix3_transform_point (&inverse,
                                need_points,
                                need_points + 1);

  /*
   * With the "origin at top left corner of pixel [0][0]" convention,
   * the index of the nearest pixel is given by floor.
   */
  return gegl_operation_detect (source,
                                (gint) floor ((double) need_points [0]),
                                (gint) floor ((double) need_points [1]));
}

static GeglRectangle
gegl_transform_get_required_for_output (GeglOperation       *op,
                                        const gchar         *input_pad,
                                        const GeglRectangle *region)
{
  OpTransform   *transform = OP_TRANSFORM (op);
  GeglMatrix3    inverse;
  GeglRectangle  requested_rect,
                 need_rect = {};
  GeglRectangle  context_rect;
  GeglSampler   *sampler;
  gdouble        vertices [8];
  gdouble        temp_points [10];
  gint           n_temp_points;
  gdouble        need_points [12];
  gint           n_need_points;
  gint           i;

  requested_rect = *region;

  if (gegl_rectangle_is_empty (&requested_rect) ||
      gegl_rectangle_is_infinite_plane (&requested_rect))
    return requested_rect;

  gegl_transform_create_composite_matrix (transform, &inverse);
  gegl_matrix3_invert (&inverse);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&inverse))
    return requested_rect;

  sampler = gegl_buffer_sampler_new_at_level (NULL,
                                     babl_format("RaGaBaA float"),
                                     transform->sampler,
                                     0); //XXX: need level?
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  /*
   * Convert indices to absolute positions:
   */
  vertices [0] = requested_rect.x;
  vertices [1] = requested_rect.y;

  vertices [2] = vertices [0] + requested_rect.width;
  vertices [3] = vertices [1];

  vertices [4] = vertices [2];
  vertices [5] = vertices [3] + requested_rect.height;

  vertices [6] = vertices [0];
  vertices [7] = vertices [5];

  /*
   * Clip polygon to the horizon.
   */
  n_temp_points = gegl_transform_depth_clip (&inverse, 0.0, vertices, 4,
                                             temp_points);

  /*
   * Clip polygon to the near plane.
   */
  n_need_points = gegl_transform_depth_clip (&inverse, 1.0 / transform->near_z,
                                             temp_points, n_temp_points,
                                             need_points);

  if (n_need_points > 1)
    {
      for (i = 0; i < 2 * n_need_points; i += 2)
        {
          gegl_matrix3_transform_point (&inverse,
                                        need_points + i,
                                        need_points + i + 1);
        }

      gegl_transform_bounding_box (need_points, n_need_points, &context_rect,
                                   &need_rect);

      need_rect.x += context_rect.x;
      need_rect.y += context_rect.y;
      /*
       * One of the pixels of the width (resp. height) has to be
       * already in the rectangle; It does not need to be counted
       * twice, hence the "- (gint) 1"s.
       */
      need_rect.width  += context_rect.width  - (gint) 1;
      need_rect.height += context_rect.height - (gint) 1;
    }

  return need_rect;
}

static GeglRectangle
gegl_transform_get_invalidated_by_change (GeglOperation       *op,
                                          const gchar         *input_pad,
                                          const GeglRectangle *input_region)
{
  OpTransform   *transform = OP_TRANSFORM (op);
  GeglMatrix3    matrix;
  GeglRectangle  affected_rect = {};

  GeglRectangle  context_rect;
  GeglSampler   *sampler;

  gdouble        vertices [8];
  gdouble        affected_points [10];
  gint           n_affected_points;
  gint           i;
  GeglRectangle  region = *input_region;

  if (gegl_rectangle_is_empty (&region) ||
      gegl_rectangle_is_infinite_plane (&region))
    return region;

  /*
   * Why does transform_get_bounding_box NOT propagate the region
   * enlarged by context_rect but transform_get_invalidated_by_change
   * does propaged the enlarged region? Reason:
   * transform_get_bounding_box appears (to Nicolas) to compute the
   * image of the ROI under the transformation: nothing to do with the
   * context_rect. On the other hand,
   * transform_get_invalidated_by_change has to do with knowing which
   * pixel indices are affected by changes in the input. Since,
   * effectively, any output pixel that maps back to something within
   * the region enlarged by the context_rect will be affected, we can
   * invert this correspondence and what it says is that we should
   * forward propagate the input region fattened by the context_rect.
   *
   * This also explains why we compute the bounding box based on pixel
   * centers, no outer corners.
   */
  /*
   * TODO: Should the result be given extra elbow room all around to
   * allow for round off error (for "safety")?
   *
   * ^-- Looks like the answer is "yes": if we cut things too close,
   * there can indeed be "missing pixels" at the edge of the input
   * buffer (due to similar logic in get_required_for_output()).
   * This might suggest that our sampling coordinates are not accurate
   * enough, but for now, allowing some wiggle room, by computing the
   * bounding box based on pixel corners, rather that pixel centers
   * (in contrast to the last sentence of the previous comment) seems
   * to be enough.
   */

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    return region;

  sampler = gegl_buffer_sampler_new_at_level (NULL,
                                     babl_format_with_space ("RaGaBaA float", NULL),
                                     transform->sampler,
                                     0); // XXX: need level?
  context_rect = *gegl_sampler_get_context_rect (sampler);
  g_object_unref (sampler);

  /*
   * Fatten (dilate) the input region by the context_rect.
   */
  region.x += context_rect.x;
  region.y += context_rect.y;
  /*
   * One of the context_rect's pixels must already be in the region
   * (in both directions), hence the "-1".
   */
  region.width  += context_rect.width  - (gint) 1;
  region.height += context_rect.height - (gint) 1;

  /*
   * Convert indices to absolute positions:
   */
  vertices [0] = region.x;
  vertices [1] = region.y;

  vertices [2] = vertices [0] + region.width;
  vertices [3] = vertices [1];

  vertices [4] = vertices [2];
  vertices [5] = vertices [3] + region.height;

  vertices [6] = vertices [0];
  vertices [7] = vertices [5];

  /*
   * Clip polygon to the near plane.
   */
  n_affected_points = gegl_transform_depth_clip (&matrix, transform->near_z,
                                                 vertices, 4, affected_points);

  if (n_affected_points > 1)
    {
      for (i = 0; i < 2 * n_affected_points; i += 2)
        gegl_matrix3_transform_point (&matrix,
                                      affected_points + i,
                                      affected_points + i + 1);

      gegl_transform_bounding_box (affected_points, n_affected_points, NULL,
                                   &affected_rect);
    }

  return affected_rect;
}

typedef struct ThreadData
{
  void (*func) (GeglOperation       *operation,
                GeglBuffer          *dest,
                GeglBuffer          *src,
                GeglMatrix3         *matrix,
                const GeglRectangle *roi,
                gint                 level);


  GeglOperation        *operation;
  GeglOperationContext *context;
  GeglBuffer           *input;
  GeglBuffer           *output;
  GeglMatrix3          *matrix;
  const GeglRectangle  *roi;
  gint                  level;
} ThreadData;

static void
thread_process (const GeglRectangle *area,
                ThreadData          *data)
{
  GeglBuffer *input;

  if (gegl_rectangle_equal (area, data->roi))
    {
      input = g_object_ref (data->input);
    }
  else
    {
      input = gegl_operation_context_dup_input_maybe_copy (data->context,
                                                           "input", area);
    }

  data->func (data->operation,
              data->output,
              input,
              data->matrix,
              area,
              data->level);

  g_object_unref (input);
}


static void
transform_affine (GeglOperation       *operation,
                  GeglBuffer          *dest,
                  GeglBuffer          *src,
                  GeglMatrix3         *matrix,
                  const GeglRectangle *roi,
                  gint                 level)
{
  gint             factor = 1 << level;
  OpTransform     *transform = (OpTransform *) operation;
  const Babl      *format = gegl_operation_get_format (operation, "output");
  GeglMatrix3      inverse;
  gdouble          inverse_near_z = 1.0 / transform->near_z;
  GeglBufferMatrix2 inverse_jacobian;
  GeglAbyssPolicy  abyss_policy = gegl_transform_get_abyss_policy (transform);
  GeglSampler     *sampler = gegl_buffer_sampler_new_at_level (src,
                                         format,
                                         level?GEGL_SAMPLER_NEAREST:transform->sampler,
                                         level);

  GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

  GeglRectangle  bounding_box = *gegl_buffer_get_abyss (src);
  GeglRectangle  context_rect = *gegl_sampler_get_context_rect (sampler);
  GeglRectangle  dest_extent  = *roi;
  gint           components = babl_format_get_n_components (format);

  bounding_box.x      += context_rect.x;
  bounding_box.y      += context_rect.y;
  bounding_box.width  += context_rect.width  - 1;
  bounding_box.height += context_rect.height - 1;

  dest_extent.x      >>= level;
  dest_extent.y      >>= level;
  dest_extent.width  >>= level;
  dest_extent.height >>= level;

  /*
   * XXX: fast paths as existing in files in the same dir as
   *      transform.c should probably be hooked in here, and bailing
   *      out before using the generic code.
   */
  /*
   * It is assumed that the affine transformation has been normalized,
   * so that inverse.coeff[2][0] = inverse.coeff[2][1] = 0 and
   * inverse.coeff[2][2] = 1 (roughly within
   * GEGL_TRANSFORM_CORE_EPSILON).
   */

  gegl_matrix3_copy_into (&inverse, matrix);

  if (factor)
  {
    inverse.coeff[0][0] /= factor;
    inverse.coeff[0][1] /= factor;
    inverse.coeff[0][2] /= factor;
    inverse.coeff[1][0] /= factor;
    inverse.coeff[1][1] /= factor;
    inverse.coeff[1][2] /= factor;
  }

  gegl_matrix3_invert (&inverse);

  {
    GeglBufferIterator *i = gegl_buffer_iterator_new (dest,
                                                      &dest_extent,
                                                      level,
                                                      format,
                                                      GEGL_ACCESS_WRITE,
                                                      GEGL_ABYSS_NONE, 1);

    /*
     * Hoist most of what can out of the while loop:
     */
    const gdouble base_u = inverse.coeff [0][0] * ((gdouble) 0.5) +
                           inverse.coeff [0][1] * ((gdouble) 0.5) +
                           inverse.coeff [0][2];
    const gdouble base_v = inverse.coeff [1][0] * ((gdouble) 0.5) +
                           inverse.coeff [1][1] * ((gdouble) 0.5) +
                           inverse.coeff [1][2];

    inverse_jacobian.coeff [0][0] =
      inverse.coeff [0][0];
    inverse_jacobian.coeff [1][0] =
      inverse.coeff [1][0];
    inverse_jacobian.coeff [0][1] =
      inverse.coeff [0][1];
    inverse_jacobian.coeff [1][1] =
      inverse.coeff [1][1];

    while (gegl_buffer_iterator_next (i))
      {
        GeglRectangle *roi = &i->items[0].roi;
        gfloat * restrict dest_ptr = (gfloat *)i->items[0].data;

        gdouble u_start =
          base_u +
          inverse.coeff [0][0] * ( roi->x ) +
          inverse.coeff [0][1] * ( roi->y );
        gdouble v_start =
          base_v +
          inverse.coeff [1][0] * ( roi->x ) +
          inverse.coeff [1][1] * ( roi->y );

        gint y = roi->height;
        do {
          gint x1 = 0;
          gint x2 = roi->width;

          if (gegl_transform_scanline_limits (&inverse, inverse_near_z,
                                              &bounding_box,
                                              u_start, v_start, 1.0,
                                              &x1, &x2))
            {
              gdouble u_float = u_start;
              gdouble v_float = v_start;

              gint x;

              memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * x1);
              dest_ptr += (gint) components * x1;

              u_float += x1 * inverse_jacobian.coeff [0][0];
              v_float += x1 * inverse_jacobian.coeff [1][0];

              for (x = x1; x < x2; x++)
                {
                  sampler_get_fun (sampler,
                                   u_float, v_float,
                                   &inverse_jacobian,
                                   dest_ptr,
                                   abyss_policy);
                  dest_ptr += (gint) components;

                  u_float += inverse_jacobian.coeff [0][0];
                  v_float += inverse_jacobian.coeff [1][0];
                }

              memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * (roi->width - x2));
              dest_ptr += (gint) components * (roi->width - x2);
            }
          else
            {
              memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * roi->width);
              dest_ptr += (gint) components * roi->width;
            }

          u_start += inverse_jacobian.coeff [0][1];
          v_start += inverse_jacobian.coeff [1][1];
        } while (--y);
      }
  }

  g_object_unref (sampler);
}

static void
transform_generic (GeglOperation       *operation,
                   GeglBuffer          *dest,
                   GeglBuffer          *src,
                   GeglMatrix3         *matrix,
                   const GeglRectangle *roi,
                   gint                 level)
{
  OpTransform *transform = (OpTransform *) operation;
  const Babl          *format = gegl_operation_get_format (operation, "output");
  gint                 factor = 1 << level;
  GeglBufferIterator  *i;
  GeglMatrix3          inverse;
  gdouble              inverse_near_z = 1.0 / transform->near_z;
  GeglAbyssPolicy      abyss_policy = gegl_transform_get_abyss_policy (transform);
  GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src, format,
                                         level?GEGL_SAMPLER_NEAREST:
                                               transform->sampler,
                                         level);
  GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

  GeglRectangle  bounding_box = *gegl_buffer_get_abyss (src);
  GeglRectangle  context_rect = *gegl_sampler_get_context_rect (sampler);
  GeglRectangle  dest_extent  = *roi;
  gint           components   = babl_format_get_n_components (format);

  bounding_box.x      += context_rect.x;
  bounding_box.y      += context_rect.y;
  bounding_box.width  += context_rect.width  - 1;
  bounding_box.height += context_rect.height - 1;

  dest_extent.x      >>= level;
  dest_extent.y      >>= level;
  dest_extent.width  >>= level;
  dest_extent.height >>= level;

  /*
   * Construct an output tile iterator.
   */
  i = gegl_buffer_iterator_new (dest,
                                &dest_extent,
                                level,
                                format,
                                GEGL_ACCESS_WRITE,
                                GEGL_ABYSS_NONE, 1);

  gegl_matrix3_copy_into (&inverse, matrix);

  if (factor)
  {
    inverse.coeff[0][0] /= factor;
    inverse.coeff[0][1] /= factor;
    inverse.coeff[0][2] /= factor;
    inverse.coeff[1][0] /= factor;
    inverse.coeff[1][1] /= factor;
    inverse.coeff[1][2] /= factor;
  }

  gegl_matrix3_invert (&inverse);

  /*
   * Fill the output tiles.
   */
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->items[0].roi;

      gdouble u_start =
        inverse.coeff [0][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [0][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [0][2];
      gdouble v_start =
        inverse.coeff [1][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [1][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [1][2];
      gdouble w_start =
        inverse.coeff [2][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [2][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [2][2];

      gfloat * restrict dest_ptr = (gfloat *)i->items[0].data;

      /*
       * Assumes that height and width are > 0.
       */
      gint y = roi->height;
      do {
        gint x1 = 0;
        gint x2 = roi->width;

        if (gegl_transform_scanline_limits (&inverse, inverse_near_z,
                                            &bounding_box,
                                            u_start, v_start, w_start,
                                            &x1, &x2))
          {
            gdouble u_float = u_start;
            gdouble v_float = v_start;
            gdouble w_float = w_start;

            gint x;

            memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * x1);
            dest_ptr += (gint) components * x1;

            u_float += x1 * inverse.coeff [0][0];
            v_float += x1 * inverse.coeff [1][0];
            w_float += x1 * inverse.coeff [2][0];

            for (x = x1; x < x2; x++)
              {
                gdouble w_recip = (gdouble) 1.0 / w_float;
                gdouble u = u_float * w_recip;
                gdouble v = v_float * w_recip;

                GeglBufferMatrix2 inverse_jacobian;
                inverse_jacobian.coeff [0][0] =
                  (inverse.coeff [0][0] - inverse.coeff [2][0] * u) * w_recip;
                inverse_jacobian.coeff [0][1] =
                  (inverse.coeff [0][1] - inverse.coeff [2][1] * u) * w_recip;
                inverse_jacobian.coeff [1][0] =
                  (inverse.coeff [1][0] - inverse.coeff [2][0] * v) * w_recip;
                inverse_jacobian.coeff [1][1] =
                  (inverse.coeff [1][1] - inverse.coeff [2][1] * v) * w_recip;

                sampler_get_fun (sampler,
                                 u, v,
                                 &inverse_jacobian,
                                 dest_ptr,
                                 abyss_policy);

                dest_ptr += (gint) components;
                u_float += inverse.coeff [0][0];
                v_float += inverse.coeff [1][0];
                w_float += inverse.coeff [2][0];
              }

            memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * (roi->width - x2));
            dest_ptr += (gint) components * (roi->width - x2);
          }
        else
          {
            memset (dest_ptr, 0, (gint) components * sizeof (gfloat) * roi->width);
            dest_ptr += (gint) components * roi->width;
          }

        u_start += inverse.coeff [0][1];
        v_start += inverse.coeff [1][1];
        w_start += inverse.coeff [2][1];
      } while (--y);
    }

  g_object_unref (sampler);
}

static void
transform_nearest (GeglOperation       *operation,
                   GeglBuffer          *dest,
                   GeglBuffer          *src,
                   GeglMatrix3         *matrix,
                   const GeglRectangle *roi,
                   gint                 level)
{
  OpTransform         *transform = (OpTransform *) operation;
  const Babl          *format    = gegl_buffer_get_format (dest);
  gint                 factor    = 1 << level;
  gint                 px_size   = babl_format_get_bytes_per_pixel (format);
  GeglBufferIterator  *i;
  GeglMatrix3          inverse;
  gdouble              inverse_near_z = 1.0 / transform->near_z;
  GeglAbyssPolicy      abyss_policy = gegl_transform_get_abyss_policy (transform);
  GeglSampler *sampler = gegl_buffer_sampler_new_at_level (src, format,
                                         GEGL_SAMPLER_NEAREST,
                                         level);
  GeglSamplerGetFun sampler_get_fun = gegl_sampler_get_fun (sampler);

  GeglRectangle  bounding_box = *gegl_buffer_get_abyss (src);
  GeglRectangle  dest_extent  = *roi;

  dest_extent.x      >>= level;
  dest_extent.y      >>= level;
  dest_extent.width  >>= level;
  dest_extent.height >>= level;

  /*
   * Construct an output tile iterator.
   */
  i = gegl_buffer_iterator_new (dest,
                                &dest_extent,
                                level,
                                format,
                                GEGL_ACCESS_WRITE,
                                GEGL_ABYSS_NONE, 1);

  gegl_matrix3_copy_into (&inverse, matrix);

  if (factor)
  {
    inverse.coeff[0][0] /= factor;
    inverse.coeff[0][1] /= factor;
    inverse.coeff[0][2] /= factor;
    inverse.coeff[1][0] /= factor;
    inverse.coeff[1][1] /= factor;
    inverse.coeff[1][2] /= factor;
  }

  gegl_matrix3_invert (&inverse);

  /*
   * Fill the output tiles.
   */
  while (gegl_buffer_iterator_next (i))
    {
      GeglRectangle *roi = &i->items[0].roi;

      gdouble u_start =
        inverse.coeff [0][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [0][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [0][2];
      gdouble v_start =
        inverse.coeff [1][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [1][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [1][2];
      gdouble w_start =
        inverse.coeff [2][0] * (roi->x + (gdouble) 0.5) +
        inverse.coeff [2][1] * (roi->y + (gdouble) 0.5) +
        inverse.coeff [2][2];

      guchar * restrict dest_ptr = (guchar *) i->items[0].data;

      /*
       * Assumes that height and width are > 0.
       */
      gint y = roi->height;
      do {
        gint x1 = 0;
        gint x2 = roi->width;

        if (gegl_transform_scanline_limits (&inverse, inverse_near_z,
                                            &bounding_box,
                                            u_start, v_start, w_start,
                                            &x1, &x2))
          {
            gdouble u_float = u_start;
            gdouble v_float = v_start;
            gdouble w_float = w_start;

            gint x;

            memset (dest_ptr, 0, px_size * x1);
            dest_ptr += px_size * x1;

            u_float += x1 * inverse.coeff [0][0];
            v_float += x1 * inverse.coeff [1][0];
            w_float += x1 * inverse.coeff [2][0];

            for (x = x1; x < x2; x++)
              {
                gdouble w_recip = (gdouble) 1.0 / w_float;
                gdouble u = u_float * w_recip;
                gdouble v = v_float * w_recip;

                sampler_get_fun (sampler,
                                 u, v,
                                 NULL,
                                 dest_ptr,
                                 abyss_policy);

                dest_ptr += px_size;
                u_float += inverse.coeff [0][0];
                v_float += inverse.coeff [1][0];
                w_float += inverse.coeff [2][0];
              }

            memset (dest_ptr, 0, px_size * (roi->width - x2));
            dest_ptr += px_size * (roi->width - x2);
          }
        else
          {
            memset (dest_ptr, 0, px_size * roi->width);
            dest_ptr += px_size * roi->width;
          }

        u_start += inverse.coeff [0][1];
        v_start += inverse.coeff [1][1];
        w_start += inverse.coeff [2][1];
      } while (--y);
    }

  g_object_unref (sampler);
}

static inline gboolean is_zero (const gdouble f)
{
  return (((gdouble) f)*((gdouble) f)
          <=
          GEGL_TRANSFORM_CORE_EPSILON*GEGL_TRANSFORM_CORE_EPSILON);
}

static gboolean
gegl_transform_matrix3_allow_fast_translate (GeglMatrix3 *matrix)
{
  /*
   * Assuming that it is a translation matrix, check if it is an
   * integer translation. If not, exit.
   *
   * This test is first because it is cheaper than checking if it's a
   * translation matrix.
   */
  if (! is_zero((gdouble) (matrix->coeff [0][2] -
                           round ((double) matrix->coeff [0][2]))) ||
      ! is_zero((gdouble) (matrix->coeff [1][2] -
                           round ((double) matrix->coeff [1][2]))))
    return FALSE;

  /*
   * Check if it is a translation matrix.
   */
  return gegl_matrix3_is_translate (matrix);
}

static gboolean
gegl_transform_process (GeglOperation        *operation,
                        GeglOperationContext *context,
                        const gchar          *output_prop,
                        const GeglRectangle  *result,
                        gint                  level)
{
  GeglBuffer  *input;
  GeglBuffer  *output;
  GeglMatrix3  matrix;
  OpTransform *transform = (OpTransform *) operation;

  gegl_transform_create_composite_matrix (transform, &matrix);

  if (gegl_transform_is_intermediate_node (transform) ||
      gegl_matrix3_is_identity (&matrix))
    {
      /* passing straight through (like gegl:nop) */
      input  = (GeglBuffer*)gegl_operation_context_dup_object (context, "input");
      if (!input)
        {
          g_warning ("transform received NULL input");
          return FALSE;
        }

      gegl_operation_context_take_object (context, "output", G_OBJECT (input));
    }
  else if ((gegl_transform_matrix3_allow_fast_translate (&matrix) ||
           (gegl_matrix3_is_translate (&matrix) &&
            transform->sampler == GEGL_SAMPLER_NEAREST)))
    {
      /*
       * Buffer shifting trick (enhanced nop). Do it if it is a
       * translation by an integer vector with arbitrary samplers, and
       * with arbitrary translations if the sampler is nearest
       * neighbor.
       *
       * TODO: Should not be taken by non-interpolatory samplers (the
       * current cubic, for example).
       */
      input  = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
      output =
        g_object_new (GEGL_TYPE_BUFFER,
                      "source", input,
                      "shift-x", -(gint) round((double) matrix.coeff [0][2]),
                      "shift-y", -(gint) round((double) matrix.coeff [1][2]),
                      "abyss-width", -1, /* Turn off abyss (use the
                                            source abyss) */
                      NULL);

      if (gegl_object_get_has_forked (G_OBJECT (input)))
        gegl_object_set_has_forked (G_OBJECT (output));

      gegl_operation_context_take_object (context, "output", G_OBJECT (output));

      g_clear_object (&input);
    }
  else
    {
      gboolean is_cmyk =
       ((babl_get_model_flags (gegl_operation_get_format (operation, "output"))
           & BABL_MODEL_FLAG_CMYK) != 0);
      /*
       * For other cases, do a proper resampling
       */
      void (*func) (GeglOperation       *operation,
                    GeglBuffer          *dest,
                    GeglBuffer          *src,
                    GeglMatrix3         *matrix,
                    const GeglRectangle *roi,
                    gint                 level) = transform_generic;

      /* XXX; why does the affine code path mangle CMYK colors when
              the generic one does not?
       */
      if (gegl_matrix3_is_affine (&matrix) && !is_cmyk)
        func = transform_affine;

      if (transform->sampler == GEGL_SAMPLER_NEAREST)
        func = transform_nearest;

      input  = (GeglBuffer*) gegl_operation_context_dup_object (context, "input");
      output = gegl_operation_context_get_target (context, "output");

      /* flush opencl caches, to avoid racy flushing
       */
      gegl_buffer_flush_ext (input, NULL);

      if (gegl_operation_use_threading (operation, result))
      {
        ThreadData data;

        data.func = func;
        data.matrix = &matrix;
        data.operation = operation;
        data.context = context;
        data.input = input;
        data.output = output;
        data.roi = result;
        data.level = level;

        gegl_parallel_distribute_area (
          result,
          gegl_operation_get_pixels_per_thread (operation),
          GEGL_SPLIT_STRATEGY_AUTO,
          (GeglParallelDistributeAreaFunc) thread_process,
          &data);
      }
      else
      {
        func (operation, output, input, &matrix, result, level);
      }

      g_clear_object (&input);
    }

  return TRUE;
}

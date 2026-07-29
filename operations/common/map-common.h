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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 *
 */


#define EPSILON 1e-6
#define KERNEL_SIZE 1

        static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "aux", babl_format_n (babl_type ("float"), 2));
  gegl_operation_set_format (operation, "output", format);
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (! strcmp (input_pad, "input"))
    {
      return *gegl_operation_source_get_bounding_box (operation, "input");
    }
  else
    {
      GeglRectangle rect = *region;

      if (o->sampler_type != GEGL_SAMPLER_NEAREST)
        {
          rect.x      -= KERNEL_SIZE;
          rect.y      -= KERNEL_SIZE;
          rect.width  += 2 * KERNEL_SIZE;
          rect.height += 2 * KERNEL_SIZE;
        }

      return rect;
    }
}

static GeglRectangle
get_invalidated_by_change (GeglOperation       *operation,
                           const gchar         *input_pad,
                           const GeglRectangle *region)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (! strcmp (input_pad, "input"))
    {
      return gegl_operation_get_bounding_box (operation);
    }
  else
    {
      GeglRectangle rect = *region;

      if (o->sampler_type != GEGL_SAMPLER_NEAREST)
        {
          rect.x      -= KERNEL_SIZE;
          rect.y      -= KERNEL_SIZE;
          rect.width  += 2 * KERNEL_SIZE;
          rect.height += 2 * KERNEL_SIZE;
        }

      return rect;
    }
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties     *o                 = GEGL_PROPERTIES (operation);
  const Babl         *format_io         = gegl_operation_get_format (operation, "output");
  const Babl         *format_coords     = babl_format_n (babl_type ("float"), 2);
  const gint          io_components     = babl_format_get_n_components (format_io);
  const gint          coords_components = babl_format_get_n_components (format_coords);
  const GeglRectangle in_roi            = {
    result->x - KERNEL_SIZE,
    result->y - KERNEL_SIZE,
    result->width + KERNEL_SIZE * 2,
    result->height + KERNEL_SIZE * 2,
  };

  GeglSampler *sampler = gegl_buffer_sampler_new_at_level (input, format_io,
                                                           o->sampler_type, level);

  if (aux != NULL
#ifdef MAP_RELATIVE
      && fabs (o->scaling) > EPSILON
#endif
     )
    {
      GeglBufferIterator *it           = gegl_buffer_iterator_new (output, result, level, format_io,
                                                                   GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 3);
      gint                index_out    = 0;
      gint                index_in     = gegl_buffer_iterator_add (it, input, &in_roi, level, format_io,
                                                                   GEGL_ACCESS_READ, o->abyss_policy);
      gint                index_coords = gegl_buffer_iterator_add (it, aux, &in_roi, level, format_coords,
                                                                   GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

      while (gegl_buffer_iterator_next (it))
        {
#ifdef MAP_RELATIVE
          gdouble     scaling = GEGL_PROPERTIES (operation)->scaling;
          gdouble     scaling_2 = scaling / 2.0;
#endif
          const GeglRectangle *out_roi    = &it->items[index_out].roi;
          const GeglRectangle *in_roi     = &it->items[index_in].roi;
          const GeglRectangle *coords_roi = &it->items[index_coords].roi;
          const gfloat        *in         = it->items[index_in].data;
          const gfloat        *coords     = it->items[index_coords].data;
          gfloat              *out        = it->items[index_out].data;

#define IN_KERNEL(dc, dr, ch) in[(((KERNEL_SIZE + r + dr) * in_roi->width) + (KERNEL_SIZE + c + dc)) * io_components + ch]
#define COORDS_KERNEL(dc, dr, ch) coords[(((KERNEL_SIZE + r + dr) * coords_roi->width) + (KERNEL_SIZE + c + dc)) * coords_components + ch]

          if (o->sampler_type == GEGL_SAMPLER_NEAREST)
            {
              gfloat y = out_roi->y + 0.5; /* initial y coordinate */

              for (gint r = 0; r < out_roi->height; r++, y++)
                {
                  gfloat x = out_roi->x + 0.5; /* initial x coordinate */

                  for (gint c = 0; c < out_roi->width; c++, x++)
                    {
                      /* if the coordinate asked is an exact pixel, we
                       * fetch it directly */
                      gdouble coords_x = COORDS_KERNEL(0, 0, 0);
                      gdouble coords_y = COORDS_KERNEL(0, 0, 1);
#ifdef MAP_RELATIVE
                      if (coords_x == 0.0f && coords_y == 0.0f)
#else
                      if (coords_x == x    && coords_y == y)
#endif
                        {
                          for (int i = 0; i < 4; ++i)
                            out[i] = IN_KERNEL(0, 0, i);
                        }
                      else
                        {
#ifdef MAP_RELATIVE
                          coords_x = x + coords_x * scaling;
                          coords_y = y + coords_y * scaling;
#endif

                          gegl_sampler_get (sampler,
                                            coords_x, coords_y,
                                            NULL, out,
                                            o->abyss_policy);
                        }

                      out += io_components;
                    }
                }
            }
          else
            {
              gfloat y = out_roi->y + 0.5; /* initial y coordinate */

              for (gint r = 0; r < out_roi->height; r++, y++)
                {
                  gfloat x = out_roi->x + 0.5; /* initial x coordinate */

                  for (gint c = 0; c < out_roi->width; c++, x++)
                    {
                      GeglBufferMatrix2 scale;

                      // left coords subtracted from right coords
                      scale.coeff[0][0] = COORDS_KERNEL (1, 0, 0) - COORDS_KERNEL (-1, 0, 0);
                      scale.coeff[1][0] = COORDS_KERNEL (1, 0, 1) - COORDS_KERNEL (-1, 0, 1);

                      // top coords subtracted from bottom coords
                      scale.coeff[0][1] = COORDS_KERNEL (0, 1, 0) - COORDS_KERNEL (0, -1, 0);
                      scale.coeff[1][1] = COORDS_KERNEL (0, 1, 1) - COORDS_KERNEL (0, -1, 1);

#ifdef MAP_RELATIVE
                      scale.coeff[0][0] = scale.coeff[0][0] * scaling_2 + 1.0;
                      scale.coeff[0][1] = scale.coeff[0][1] * scaling_2;
                      scale.coeff[1][0] = scale.coeff[1][0] * scaling_2;
                      scale.coeff[1][1] = scale.coeff[1][1] * scaling_2 + 1.0;
#else
                      scale.coeff[0][0] /= 2.0;
                      scale.coeff[0][1] /= 2.0;
                      scale.coeff[1][0] /= 2.0;
                      scale.coeff[1][1] /= 2.0;
#endif

                      /* if the coordinate asked is an exact pixel, we fetch it
                       * directly, to avoid the blur of sampling */
                      gdouble coords_x = COORDS_KERNEL(0, 0, 0);
                      gdouble coords_y = COORDS_KERNEL(0, 0, 1);
#ifdef MAP_RELATIVE
                      if (coords_x == 0.0f && coords_y == 0.0f &&
#else
                      if (coords_x == x    && coords_y == y    &&
#endif
                          gegl_buffer_matrix2_is_identity (&scale))
                        {
                          for (int i = 0; i < 4; ++i)
                            out[i] = IN_KERNEL(0, 0, i);
                        }
                      else
                        {
#ifdef MAP_RELATIVE
                          coords_x = x + coords_x * scaling;
                          coords_y = y + coords_y * scaling;
#endif

                          gegl_sampler_get (sampler,
                                            coords_x, coords_y,
                                            &scale, out,
                                            o->abyss_policy);
                        }

                      out += io_components;
                    }
                }
            }
        }
#undef IN_KERNEL
#undef COORDS_KERNEL
    }
  else
    {
      gegl_buffer_copy (input, result, o->abyss_policy,
                        output, result);
    }

  g_object_unref (sampler);

  return TRUE;
}

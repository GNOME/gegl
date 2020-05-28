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
          rect.x      -= 1;
          rect.y      -= 1;
          rect.width  += 2;
          rect.height += 2;
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
          rect.x      -= 1;
          rect.y      -= 1;
          rect.width  += 2;
          rect.height += 2;
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
  GeglProperties     *o = GEGL_PROPERTIES (operation);
  const Babl         *format_io, *format_coords;
  GeglSampler        *sampler;
  GeglBufferIterator *it;
  gint                index_in, index_out, index_coords;

  format_io = gegl_operation_get_format (operation, "output");
  format_coords = babl_format_n (babl_type ("float"), 2);

  sampler = gegl_buffer_sampler_new_at_level (input, format_io,
                                              o->sampler_type, level);

  if (aux != NULL
#ifdef MAP_RELATIVE
      && fabs (o->scaling) > EPSILON
#endif
     )
    {
      it = gegl_buffer_iterator_new (output, result, level, format_io,
                                     GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, 3);
      index_out = 0;

      index_coords = gegl_buffer_iterator_add (it, aux, result, level, format_coords,
                                               GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
      index_in = gegl_buffer_iterator_add (it, input, result, level, format_io,
                                           GEGL_ACCESS_READ, o->abyss_policy);

      while (gegl_buffer_iterator_next (it))
        {
          gint        c;
          gint        r;
          gfloat      x;
          gfloat      y;
#ifdef MAP_RELATIVE
          gdouble     scaling = GEGL_PROPERTIES (operation)->scaling;
          gdouble     scaling_2 = scaling / 2.0;
#endif
          gfloat     *in = it->items[index_in].data;
          gfloat     *out = it->items[index_out].data;
          gfloat     *coords = it->items[index_coords].data;
          GeglRectangle *roi = &it->items[0].roi;

          y = roi->y + 0.5; /* initial y coordinate */

          if (o->sampler_type == GEGL_SAMPLER_NEAREST)
            {
              for (r = 0; r < roi->height; r++, y++)
                {
                  x = roi->x + 0.5; /* initial x coordinate */

                  for (c = 0; c < roi->width; c++, x++)
                    {
                      /* if the coordinate asked is an exact pixel, we
                       * fetch it directly
                       */
#ifdef MAP_RELATIVE
                      if (coords[0] == 0.0f && coords[1] == 0.0f)
#else
                      if (coords[0] == x    && coords[1] == y)
#endif
                        {
                          out[0] = in[0];
                          out[1] = in[1];
                          out[2] = in[2];
                          out[3] = in[3];
                        }
                      else
                        {
                          gdouble coords_x = coords[0];
                          gdouble coords_y = coords[1];

#ifdef MAP_RELATIVE
                          coords_x = x + coords_x * scaling;
                          coords_y = y + coords_y * scaling;
#endif

                          gegl_sampler_get (sampler,
                                            coords_x, coords_y,
                                            NULL, out,
                                            o->abyss_policy);
                        }

                      coords += 2;
                      in += 4;
                      out += 4;
                    }
                }
            }
          else
            {
              gint   stride = 2 * roi->width;
              gfloat coords_top[2 * roi->width];
              gfloat coords_bottom[2 * roi->width];
              gfloat coords_left[2 * roi->height];
              gfloat coords_right[2 * roi->height];

              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x, roi->y - 1,
                                               roi->width, 1),
                               1.0, format_coords, coords_top,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x, roi->y + roi->height,
                                               roi->width, 1),
                               1.0, format_coords, coords_bottom,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x - 1, roi->y,
                                               1, roi->height),
                               1.0, format_coords, coords_left,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);
              gegl_buffer_get (aux,
                               GEGL_RECTANGLE (roi->x + roi->width, roi->y,
                                               1, roi->height),
                               1.0, format_coords, coords_right,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

              for (r = 0; r < roi->height; r++, y++)
                {
                  x = roi->x + 0.5; /* initial x coordinate */

                  for (c = 0; c < roi->width; c++, x++)
                    {
                      GeglBufferMatrix2 scale;

                      if (c < roi->width - 1)
                        {
                          scale.coeff[0][0] = coords[2];
                          scale.coeff[1][0] = coords[3];
                        }
                      else
                        {
                          scale.coeff[0][0] = coords_right[2 * r + 0];
                          scale.coeff[1][0] = coords_right[2 * r + 1];
                        }

                      if (c > 0)
                        {
                          scale.coeff[0][0] -= coords[-2];
                          scale.coeff[1][0] -= coords[-1];
                        }
                      else
                        {
                          scale.coeff[0][0] -= coords_left[2 * r + 0];
                          scale.coeff[1][0] -= coords_left[2 * r + 1];
                        }

                      if (r < roi->height - 1)
                        {
                          scale.coeff[0][1] = coords[stride + 0];
                          scale.coeff[1][1] = coords[stride + 1];
                        }
                      else
                        {
                          scale.coeff[0][1] = coords_bottom[2 * c + 0];
                          scale.coeff[1][1] = coords_bottom[2 * c + 1];
                        }

                      if (r > 0)
                        {
                          scale.coeff[0][1] -= coords[-stride + 0];
                          scale.coeff[1][1] -= coords[-stride + 1];
                        }
                      else
                        {
                          scale.coeff[0][1] -= coords_top[2 * c + 0];
                          scale.coeff[1][1] -= coords_top[2 * c + 1];
                        }

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
                       * directly, to avoid the blur of sampling
                       */
#ifdef MAP_RELATIVE
                      if (coords[0] == 0.0f && coords[1] == 0.0f &&
#else
                      if (coords[0] == x    && coords[1] == y    &&
#endif
                          gegl_buffer_matrix2_is_identity (&scale))
                        {
                          out[0] = in[0];
                          out[1] = in[1];
                          out[2] = in[2];
                          out[3] = in[3];
                        }
                      else
                        {
                          gdouble coords_x = coords[0];
                          gdouble coords_y = coords[1];

#ifdef MAP_RELATIVE
                          coords_x = x + coords_x * scaling;
                          coords_y = y + coords_y * scaling;
#endif

                          gegl_sampler_get (sampler,
                                            coords_x, coords_y,
                                            &scale, out,
                                            o->abyss_policy);
                        }

                      coords += 2;
                      in += 4;
                      out += 4;
                    }
                }
            }
        }
    }
  else
    {
      gegl_buffer_copy (input, result, o->abyss_policy,
                        output, result);
    }

  g_object_unref (sampler);

  return TRUE;
}

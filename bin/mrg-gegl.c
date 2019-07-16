/* This file is part of GEGL editor -- an mrg frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2015 Øyvind Kolås pippin@gimp.org
 */

/* The code in this file is an image viewer/editor written using microraptor
 * gui and GEGL. It renders the UI directly from GEGLs data structures.
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#ifdef HAVE_MRG

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <math.h>
#include <mrg.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <gegl-paramspecs.h>
#include <gegl-audio-fragment.h>
#include "mrg-gegl.h"

static unsigned char *copy_buf = NULL;
static int copy_buf_len = 0;

static float cached_x0 = 0;
static float cached_y0 = 0;
static float cached_width = 0;
static float cached_height = 0;
static float cached_u = 0;
static float cached_v = 0;
static float cached_scale = 0;
static float cached_prev_mul = 0;
static int   cached_nearest = 0;
static int   cached_dirty = 0;
static cairo_surface_t *cached_surface = NULL;

void mrg_gegl_dirty (Mrg *mrg)
{
  cached_dirty++;
}

int mrg_gegl_got_nearest (void)
{
  return cached_nearest;
}

void mrg_gegl_buffer_blit (Mrg *mrg,
                           float x0, float y0,
                           float width, float height,
                           GeglBuffer *buffer,
                           float u, float v,
                           float scale,
                           float preview_multiplier,
                           int   nearest_neighbor,
                           int   color_manage_display)
{
  float fake_factor = preview_multiplier;
  GeglRectangle bounds;

  cairo_t *cr = mrg_cr (mrg);
  cairo_surface_t *surface = NULL;

  if (!buffer)
    return;

  bounds = *gegl_buffer_get_extent (buffer);

  if (width == -1 && height == -1)
  {
    width  = bounds.width;
    height = bounds.height;
  }

  if (width == -1)
    width = bounds.width * height / bounds.height;
  if (height == -1)
    height = bounds.height * width / bounds.width;

  if (cached_x0 == x0 &&
      cached_y0 == y0 &&
      cached_width == width &&
      cached_height == height &&
      cached_u == u &&
      cached_v == v &&
      cached_scale == scale &&
      cached_prev_mul == preview_multiplier &&
      cached_dirty == 0)
  {
    width /= fake_factor;
    height /= fake_factor;
    u /= fake_factor;
    v /= fake_factor;

    surface = cached_surface;
  }
  else
  {
    cached_x0 = x0;
    cached_y0 = y0;
    cached_width = width;
    cached_height = height;
    cached_u = u;
    cached_v = v;
    cached_scale = scale;
    cached_nearest = nearest_neighbor;
    cached_prev_mul = preview_multiplier;
    cached_dirty = 0;

  width /= fake_factor;
  height /= fake_factor;
  u /= fake_factor;
  v /= fake_factor;

  if (copy_buf_len < width * height * 4)
  {
    if (copy_buf)
      free (copy_buf);

    /* we over-allocate ones scanline, otherwise we sometimes get
       crashes.
     */
    copy_buf_len = width * (height+1) * 4;
    copy_buf = malloc (copy_buf_len);
  }
  {
    unsigned char *buf = copy_buf;
    GeglRectangle roi = {u, v, width, height};
    const Babl *fmt = NULL;
    static const Babl *fmt_icc = NULL;
    static const Babl *fmt_srgb = NULL;

    if (color_manage_display)
    {
      if (!fmt_icc)
      {
        int icc_length = 0;
        unsigned const char *icc_data = mrg_get_profile (mrg, &icc_length);
        const Babl *space = NULL;
        if (icc_data)
          space = babl_space_from_icc ((char*)icc_data, icc_length, BABL_ICC_INTENT_RELATIVE_COLORIMETRIC, NULL);
        fmt_icc = babl_format_with_space ("cairo-RGB24", space);
      }
      fmt = fmt_icc;
    }
    else
    {
      if (!fmt_srgb)
        fmt_srgb = babl_format_with_space ("cairo-RGB24", NULL);
      fmt = fmt_srgb;
    }
    gegl_buffer_get (buffer, &roi, scale / fake_factor, fmt, buf, width * 4,
         GEGL_ABYSS_NONE | (nearest_neighbor?GEGL_BUFFER_FILTER_NEAREST:0));
    surface = cairo_image_surface_create_for_data (buf, CAIRO_FORMAT_RGB24, width, height, width * 4);
  }

    cairo_surface_set_device_scale (surface, 1.0/fake_factor, 1.0/fake_factor);
    if (cached_surface)
      cairo_surface_destroy (cached_surface);
    cached_surface = surface;
  }
  cairo_save (cr);

  width *= fake_factor;
  height *= fake_factor;
  u *= fake_factor;
  v *= fake_factor;

  cairo_rectangle (cr, x0, y0, width, height);

  cairo_clip (cr);
  cairo_translate (cr, floorf (x0 * fake_factor), floorf (y0 * fake_factor));
  cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  cairo_set_source_surface (cr, surface, 0, 0);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);

  //cairo_surface_destroy (surface);
  cairo_restore (cr);
}



void mrg_gegl_blit (Mrg *mrg,
                    float x0, float y0,
                    float width, float height,
                    GeglNode *node,
                    float u, float v,
                    float scale,
                    float preview_multiplier,
                    int nearest_neighbor,
                    int color_manage_display)
{
  float fake_factor = preview_multiplier;
  GeglRectangle bounds;

  cairo_t *cr = mrg_cr (mrg);
  cairo_surface_t *surface = NULL;

  if (!node)
    return;

  bounds = gegl_node_get_bounding_box (node);

  if (width == -1 && height == -1)
  {
    width  = bounds.width;
    height = bounds.height;
  }

  if (width == -1)
    width = bounds.width * height / bounds.height;
  if (height == -1)
    height = bounds.height * width / bounds.width;

  width /= fake_factor;
  height /= fake_factor;
  u /= fake_factor;
  v /= fake_factor;

  if (copy_buf_len < width * height * 4)
  {
    if (copy_buf)
      free (copy_buf);

    /* XXX: same overallocation as in other code-path, overallocating a fixed
            amount might also be a sufficient workaround until the underlying
            issue is resolved.
     */
    copy_buf_len = width * (height+1) * 4;
    copy_buf = malloc (copy_buf_len);
  }
  {
    unsigned char *buf = copy_buf;
    GeglRectangle roi = {u, v, width, height};
    const Babl *fmt = NULL;
    static const Babl *fmt_icc = NULL;
    static const Babl *fmt_srgb = NULL;

    if (color_manage_display)
    {
      if (!fmt_icc)
      {
        int icc_length = 0;
        unsigned const char *icc_data = mrg_get_profile (mrg, &icc_length);
        const Babl *space = NULL;
        if (icc_data)
          space = babl_space_from_icc ((char*)icc_data, icc_length, BABL_ICC_INTENT_RELATIVE_COLORIMETRIC, NULL);
        fmt_icc = babl_format_with_space ("cairo-RGB24", space);
      }
      fmt = fmt_icc;
    }
    else
    {
      if (!fmt_srgb)
        fmt_srgb = babl_format_with_space ("cairo-RGB24", NULL);
      fmt = fmt_srgb;
    }

    gegl_node_blit (node, scale / fake_factor, &roi, fmt, buf, width * 4,
         GEGL_BLIT_DEFAULT | (nearest_neighbor?GEGL_BUFFER_FILTER_NEAREST:0));
  surface = cairo_image_surface_create_for_data (buf, CAIRO_FORMAT_RGB24, width, height, width * 4);
  }

  cairo_save (cr);
  cairo_surface_set_device_scale (surface, 1.0/fake_factor, 1.0/fake_factor);

  width *= fake_factor;
  height *= fake_factor;
  u *= fake_factor;
  v *= fake_factor;

  cairo_rectangle (cr, x0, y0, width, height);

  cairo_clip (cr);
  cairo_translate (cr, floorf (x0 * fake_factor), floorf (y0 * fake_factor));
  cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  cairo_set_source_surface (cr, surface, 0, 0);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
  cairo_restore (cr);
}


#endif

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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2015 Øyvind Kolås pippin@gimp.org
 */

/* The code in this file is an image viewer/editor written using microraptor
 * gui and GEGL. It renders the UI directly from GEGLs data structures.
 */

#define _BSD_SOURCE
#define _DEFAULT_SOURCE

#include "config.h"

#if HAVE_MRG

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <mrg.h>
#include <gegl.h>
#include <gexiv2/gexiv2.h>
#include <gegl-paramspecs.h>
#include <SDL.h>
#include <gegl-audio-fragment.h>

static unsigned char *copy_buf = NULL;
static int copy_buf_len = 0;

void mrg_gegl_blit (Mrg *mrg,
                    float x0, float y0,
                    float width, float height,
                    GeglNode *node,
                    float u, float v,
                    float scale,
                    float preview_multiplier);

void mrg_gegl_blit (Mrg *mrg,
                    float x0, float y0,
                    float width, float height,
                    GeglNode *node,
                    float u, float v,
                    float scale,
                    float preview_multiplier)
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
    copy_buf_len = width * height * 4;
    copy_buf = malloc (copy_buf_len);
  }
  {
    static int foo = 0;
    unsigned char *buf = copy_buf;
    GeglRectangle roi = {u, v, width, height};
    static const Babl *fmt = NULL;

foo++;
    if (!fmt) fmt = babl_format ("cairo-RGB24");
    gegl_node_blit (node, scale / fake_factor, &roi, fmt, buf, width * 4,
         GEGL_BLIT_DEFAULT);
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
  cairo_translate (cr, x0 * fake_factor, y0 * fake_factor);
  cairo_pattern_set_filter (cairo_get_source (cr), CAIRO_FILTER_NEAREST);
  cairo_set_source_surface (cr, surface, 0, 0);

  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  cairo_surface_destroy (surface);
  cairo_restore (cr);
}


#endif

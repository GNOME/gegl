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
 * Copyright 2018 Øyvind Kolås <pippin@gimp.org>
 *
 * This code is based on the decode_gif example coming with netsurf.
 *
 * Copyright 2004 Richard Wilson <richard.wilson@netsurf-browser.org>
 * Copyright 2008 Sean Fox <dyntryx@gmail.com>
 *
 * This file is part of NetSurf's libnsgif, http://www.netsurf-browser.org/
 * Licenced under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_int (frame, _("frame"), 0)
  description (_("frame number to decode"))
property_int (frames, _("frames"), 0)
  description (_("Number of frames in gif animation"))
property_int (frame_delay, _("frame-delay"), 100)
  description (_("Delay in ms for last decoded frame"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME gif_load
#define GEGL_OP_C_SOURCE gif-load.c

#include <gegl-op.h>
#include <gegl-gio-private.h>

#include <assert.h>

#include "libnsgif.h"

#define IO_BUFFER_SIZE 4096

typedef struct
{
  GFile *file;
  GInputStream *stream;

  gif_animation gif;
  unsigned char *gif_data;

  const Babl *format;

  gint width;
  gint height;
} Priv;

static void
cleanup(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      gif_finalise (&p->gif);
      if (p->gif_data) g_free (p->gif_data);

      if (p->stream != NULL)
        g_input_stream_close (G_INPUT_STREAM (p->stream), NULL, NULL);

      g_clear_object (&p->stream);
      g_clear_object (&p->file);

      p->width = p->height = 0;
      p->format = NULL;
    }
}

static void *bitmap_create(int width, int height)
{
  return calloc(width * height, 4);
}

static void bitmap_set_opaque(void *bitmap, bool opaque)
{
  (void) opaque;  /* unused */
  assert(bitmap);
}

static bool bitmap_test_opaque(void *bitmap)
{
  assert(bitmap);
  return false;
}

static unsigned char *bitmap_get_buffer(void *bitmap)
{
  assert(bitmap);
  return bitmap;
}

static void bitmap_destroy(void *bitmap)
{
  assert(bitmap);
  free(bitmap);
}

static void bitmap_modified(void *bitmap)
{
  assert(bitmap);
  return;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (o->user_data) ? o->user_data : g_new0 (Priv, 1);

  g_assert (p != NULL);

  p->format = babl_format ("R'G'B'A u8");
  o->user_data = (void*) p;

  if (p->gif_data == NULL)
    {
      gsize length;
      gif_bitmap_callback_vt bitmap_callbacks = {
        bitmap_create,
        bitmap_destroy,
        bitmap_get_buffer,
        bitmap_set_opaque,
        bitmap_test_opaque,
        bitmap_modified
      };
      g_file_get_contents (o->path, (void*)&p->gif_data, &length, NULL);
      gif_create(&p->gif, &bitmap_callbacks);
      gif_initialise(&p->gif, length, p->gif_data);

      o->frames = p->gif.frame_count;
      g_assert (p->gif_data != NULL);
    }

  gegl_operation_set_format (operation, "output", p->format);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglRectangle result = { 0, 0, 0, 0 };
  Priv *p = (Priv*) o->user_data;

  result.width = p->gif.width;
  result.height = p->gif.height;

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p = (Priv*) o->user_data;
  int code;

  if (o->frame > o->frames-1) o->frame = o->frames-1;
  if (o->frame < 0) o->frame = 0;

  code = gif_decode_frame (&p->gif, o->frame);
  if (code != GIF_OK)
    g_warning ("gif_decode_frame:%i\n", code);

  gegl_buffer_set (output, result, 0, p->format,
                   p->gif.frame_image,
                   p->gif.width * 4);
  o->frame_delay = p->gif.frames[o->frame].frame_delay * 10;
  return FALSE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
finalize(GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data != NULL)
    {
      cleanup (GEGL_OPERATION (object));
      g_clear_pointer (&o->user_data, g_free);
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS(klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:gif-load",
    "title",        _("GIF File Loader"),
    "categories"  , "hidden",
    "description" , _("GIF image loader."),
    NULL);

  gegl_operation_handlers_register_loader (
    "image/gif", "gegl:gif-load");
  gegl_operation_handlers_register_loader (
    ".gif", "gegl:gif-load");
}

#endif

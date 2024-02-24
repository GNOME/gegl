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
 * Copyright 2011 Rasmus Hahn <rassahah@googlemail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path(path, _("File"), "")
   description(_("path of file to write to."))
property_int  (tile, "Tile", 0)
   description (_("tile size to use."))
   value_range (0, 2048)

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME exr_save
#define GEGL_OP_C_SOURCE   exr-save.cc
#define GEGL_OP_NO_SOURCE

extern "C" {
#include "gegl-op.h"
} /* extern "C" */

#include <exception>
#include <ImfTiledOutputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>
#include <ImfChromaticities.h>
#include <ImfStandardAttributes.h>
#include <ImfArray.h>
#include <ImfFrameBuffer.h>
#include "ImathRandom.h"




/**
 * create an Imf::Header for writing up to 4 channels (given in d).
 * d must be between 1 and 4.
 */
static Imf::Header
create_header (int w,
               int h,
               int d,
               int bits)
{
  Imf::Header header (w, h);
  Imf::FrameBuffer fbuf;

  if (bits == 16)
  {
    if (d <= 2)
      {
        header.channels ().insert ("Y", Imf::Channel (Imf::HALF));
      }
    else
      {
        header.channels ().insert ("R", Imf::Channel (Imf::HALF));
        header.channels ().insert ("G", Imf::Channel (Imf::HALF));
        header.channels ().insert ("B", Imf::Channel (Imf::HALF));
      }
    if (d == 2 || d == 4)
      {
        header.channels ().insert ("A", Imf::Channel (Imf::HALF));
      }
  }
  else
  {
    if (d <= 2)
      {
        header.channels ().insert ("Y", Imf::Channel (Imf::FLOAT));
      }
    else
      {
        header.channels ().insert ("R", Imf::Channel (Imf::FLOAT));
        header.channels ().insert ("G", Imf::Channel (Imf::FLOAT));
        header.channels ().insert ("B", Imf::Channel (Imf::FLOAT));
      }
    if (d == 2 || d == 4)
      {
        header.channels ().insert ("A", Imf::Channel (Imf::FLOAT));
      }
  }
  return header;
}

/**
 * create an Imf::FrameBuffer object for w*h*d floats and return it.
 */
static Imf::FrameBuffer
create_frame_buffer_f32 (int          w,
                         int          h,
                         int          d,
                         const float *data)
{
  Imf::FrameBuffer fbuf;

  if (d <= 2)
    {
      fbuf.insert ("Y", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
    }
  else
    {
      fbuf.insert ("R", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("G", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 1),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("B", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 2),
          d * sizeof *data, d * sizeof *data * w));
    }
  if (d == 2 || d == 4)
    {
      fbuf.insert ("A", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + (d - 1)),
          d * sizeof *data, d * sizeof *data * w));
    }
  return fbuf;
}

static Imf::FrameBuffer
create_frame_buffer_f16 (int          w,
                         int          h,
                         int          d,
                         const short *data)
{
  Imf::FrameBuffer fbuf;

  if (d <= 2)
    {
      fbuf.insert ("Y", Imf::Slice (Imf::HALF, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
    }
  else
    {
      fbuf.insert ("R", Imf::Slice (Imf::HALF, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("G", Imf::Slice (Imf::HALF, (char *) (&data[0] + 1),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("B", Imf::Slice (Imf::HALF, (char *) (&data[0] + 2),
          d * sizeof *data, d * sizeof *data * w));
    }
  if (d == 2 || d == 4)
    {
      fbuf.insert ("A", Imf::Slice (Imf::HALF, (char *) (&data[0] + (d - 1)),
          d * sizeof *data, d * sizeof *data * w));
    }
  return fbuf;
}


static Imf::FrameBuffer
create_frame_buffer (int          w,
                     int          h,
                     int          d,
                     int          bits,
                     const void  *data)
{
  if (bits == 16)
    return create_frame_buffer_f16 (w, h, d, (short*)data);
  else // if (bits == 32)
    return create_frame_buffer_f32 (w, h, d, (float*)data);
}


/**
 * write the float buffer to an exr file with tile-size tw x th.
 * The buffer must contain w*h*d floats.
 * Currently supported are only values 1-4 for d:
 * d=1: write a single Y-channel.
 * d=2: write Y and A.
 * d=3: write RGB.
 * d=4: write RGB and A.
 */
static void
write_tiled_exr (const void        *pixels,
                 const Babl        *space,
                 int                w,
                 int                h,
                 int                d,
                 int                tw,
                 int                th,
                 int                bits,
                 const std::string &filename)
{
  Imf::Header header (create_header (w, h, d, bits));
  header.setTileDescription (Imf::TileDescription (tw, th, Imf::ONE_LEVEL));

  {
    double wp[2];
    double red[2];
    double green[2];
    double blue[2];
    babl_space_get (space, &wp[0], &wp[1],
                          &red[0], &red[1],
                          &green[0], &green[1],
                          &blue[0], &blue[1],
                          NULL, NULL, NULL);
    {
    Imf::Chromaticities c1 (Imath::V2f(red[0],red[1]),
                            Imath::V2f(green[0],green[1]),
                            Imath::V2f(blue[0],blue[1]),
                            Imath::V2f(wp[0],wp[1]));
    Imf::addChromaticities (header, c1);
    }
  }

  Imf::TiledOutputFile out (filename.c_str (), header);
  Imf::FrameBuffer fbuf (create_frame_buffer (w, h, d, bits, pixels));
  out.setFrameBuffer (fbuf);
  out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
}

/**
 * write an openexr file in scanline mode.
 * pixels contains the image data as an array of w*h*d floats.
 * The data is written to the file named filename.
 */
static void
write_scanline_exr (const void        *pixels,
                    const Babl        *space,
                    int                w,
                    int                h,
                    int                d,
                    int                bits,
                    const std::string &filename)
{
  Imf::Header header (create_header (w, h, d, bits));

  {
    double wp[2];
    double red[2];
    double green[2];
    double blue[2];
    babl_space_get (space, &wp[0], &wp[1],
                           &red[0], &red[1],
                           &green[0], &green[1],
                           &blue[0], &blue[1],
                           NULL, NULL, NULL);
    Imf::Chromaticities c1 (Imath::V2f(red[0],red[1]),
                            Imath::V2f(green[0],green[1]),
                            Imath::V2f(blue[0],blue[1]),
                            Imath::V2f(wp[0],wp[1]));
    Imf::addChromaticities (header, c1);
  }

  Imf::OutputFile out (filename.c_str (), header);
  Imf::FrameBuffer fbuf (create_frame_buffer (w, h, d, bits, pixels));
  out.setFrameBuffer (fbuf);
  out.writePixels (h);
}

/**
 * write the given pixel buffer, which is w * h * d to filename using the
 * tilesize as tile width and height. This is the only function calling
 * the openexr lib and therefore should be exception save.
 */
static void
exr_save_process (const void        *pixels,
                  const Babl        *space,
                  int                w,
                  int                h,
                  int                d,
                  int                tile_size,
                  int                bits,
                  const std::string &filename)
{
  if (tile_size == 0)
    {
      /* write a scanline exr image. */
      write_scanline_exr (pixels, space, w, h, d, bits, filename);
    }
  else
    {
      /* write a tiled exr image. */
      write_tiled_exr (pixels, space, w, h, d, tile_size, tile_size, bits, filename);
    }
}

/**
 * main entry function for the exr saver.
 */
static gboolean
gegl_exr_save_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       const GeglRectangle *rect,
                       gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  std::string filename (o->path);
  std::string output_format;
  int tile_size (o->tile);
  /*
   * determine the number of channels in the input buffer and determine
   * format, data is written with. Currently this only checks for the
   * numbers of channels and writes only Y or RGB data with optional alpha.
   */
  const Babl *original_format = gegl_buffer_get_format (input);
  const Babl *original_space = babl_format_get_space (original_format);
  unsigned n_components = babl_format_get_n_components (original_format);

  int bits_per_component = 8*babl_format_get_bytes_per_pixel (original_format)/n_components;

  if (bits_per_component == 16)
  {
    switch (n_components)
      {
        case 1: output_format = "Y half";       break;
        case 2: output_format = "YaA half";     break;
        case 3: output_format = "RGB half";     break;
        case 4: output_format = "RaGaBaA half"; break;
        default:
          g_warning ("exr-save: cannot write exr with n_components %d.", n_components);
          return FALSE;
          break;
      }
  }
  else
  {
    bits_per_component=32;
    switch (n_components)
      {
        case 1: output_format = "Y float";       break;
        case 2: output_format = "YaA float";     break;
        case 3: output_format = "RGB float";     break;
        case 4: output_format = "RaGaBaA float"; break;
        default:
          g_warning ("exr-save: cannot write exr with n_components %d.", n_components);
          return FALSE;
          break;
      }
  }
  /*
   * get the pixel data. The position of the rectangle is effectively
   * ignored. Always write a file width x height; @todo: check if exr
   * can set the origin.
   */
  void *pixels
    = g_malloc (rect->width * rect->height * n_components * bits_per_component/8);
  if (pixels == 0)
    {
      g_warning ("exr-save: could allocate %d*%d*%d pixels.",
        rect->width, rect->height, n_components);
      return FALSE;
    }


  gegl_buffer_get (input, rect, 1.0, babl_format_with_space (output_format.c_str (), original_space),
                   pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  bool status;
  try
    {
      exr_save_process (pixels, original_space, rect->width, rect->height,
                        n_components, tile_size, bits_per_component, filename);
      status = TRUE;
    }
  catch (std::exception &e)
    {
      g_warning ("exr-save: failed to write to '%s': %s",
         filename.c_str (), e.what ());
      status = FALSE;
    }
  g_free (pixels);
  return status;
}

static void gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = gegl_exr_save_process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:exr-save",
    "categories"  , "output",
    "description" , "OpenEXR image saver",
    NULL);

  gegl_operation_handlers_register_saver (
    ".exr", "gegl:exr-save");
}

#endif


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
 * Copyright 2015 Martin Blanchard <tchaik@gmx.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gegl-metadata.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description (_("Target path and filename, use '-' for stdout"))
property_int (bitdepth, _("Bitdepth"), -1)
  description (_("-1, 8, 16, 32 and 64 are the currently accepted values, -1 means auto"))
  value_range (-1, 64)
property_int (fp, _("use floating point"), -1)
  description (_("floating point -1 means auto, 0 means integer, 1 means float."))
  value_range (-1, 1)

property_object(metadata, _("Metadata"), GEGL_TYPE_METADATA)
  description (_("Object to receive image metadata"))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME     tiff_save
#define GEGL_OP_C_SOURCE tiff-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <glib/gprintf.h>
#include <tiffio.h>

typedef struct
{
  GFile *file;
  GOutputStream *stream;
  gboolean can_seek;

  gchar *buffer;
  gsize allocated;
  gsize position;

  TIFF *tiff;
} Priv;

static void
tiff_format_timestamp (const GValue *src_value, GValue *dest_value)
{
  GDateTime *datetime;
  gchar *datestr;

  g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (src_value, G_TYPE_DATE_TIME));
  g_return_if_fail (G_VALUE_HOLDS_STRING (dest_value));

  datetime = g_value_get_boxed (src_value);
  g_return_if_fail (datetime != NULL);

  datestr = g_date_time_format (datetime, "%Y:%m:%d %T");
  g_return_if_fail (datestr != NULL);

  g_value_take_string (dest_value, datestr);
}

static const GeglMetadataMap tiff_save_metadata[] =
{
  { "Artist",               "artist",       NULL },
  { "Copyright",            "copyright",    NULL },
  { "DateTime",             "timestamp",    tiff_format_timestamp },
  { "ImageDescription",     "description",  NULL },
  { "PageName",             "title",        NULL },
  { "Software",             "software",     NULL },
};

static void
cleanup(GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;

  if (p != NULL)
    {
      if (p->tiff != NULL)
        TIFFClose(p->tiff);
      else if (p->stream != NULL)
        g_output_stream_close(G_OUTPUT_STREAM(p->stream), NULL, NULL);

      g_clear_object (&p->stream);
      p->tiff = NULL;

      g_clear_object (&p->file);
    }
}

static GSeekType
lseek_to_seek_type(int whence)
{
  switch (whence)
    {
    default:
    case SEEK_SET:
      return G_SEEK_SET;

    case SEEK_CUR:
      return G_SEEK_CUR;

    case SEEK_END:
      return G_SEEK_END;
    }
}

static void
error_handler(const char *module,
              const char *format,
              va_list arguments)
{
  gchar *message;

  g_vasprintf(&message, format, arguments);
  g_warning("%s", message);

  g_free(message);
}

static void
warning_handler(const char *module,
                const char *format,
                va_list arguments)
{
  gchar *message;

  g_vasprintf(&message, format, arguments);
  g_message("%s", message);

  g_free(message);
}

static tsize_t
read_from_stream(thandle_t handle,
                 tdata_t buffer,
                 tsize_t size)
{
  Priv *p = (Priv*) handle;

  g_assert(p->stream && FALSE);

  return -1;
}

static tsize_t
write_to_stream(thandle_t handle,
                tdata_t buffer,
                tsize_t size)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gchar *new_buffer;
  gsize new_size;
  gssize written = -1;

  g_assert(p->stream);

  if (p->can_seek)
    {
      written = g_output_stream_write(G_OUTPUT_STREAM(p->stream),
                                      (void *) buffer, (gsize) size,
                                      NULL, &error);
      if (written < 0)
        {
          g_warning("%s", error->message);
          g_error_free(error);
        }
    }
  else
    {
      if (p->position + size > p->allocated)
        {
          new_size = p->position + size;
          new_buffer = g_try_realloc(p->buffer, new_size);
          if (!new_buffer)
            return -1;

          p->allocated = new_size;
          p->buffer = new_buffer;
        }

      g_assert(p->position + size >= p->allocated);

      memcpy(p->buffer + p->position, buffer, size);
      p->position += size;
      written = size;
    }

  return (tsize_t) written;
}

static toff_t
seek_in_stream(thandle_t handle,
               toff_t offset,
               int whence)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gboolean sought = FALSE;
  goffset position = -1;

  g_assert(p->stream);

  if (p->can_seek)
    {
      sought = g_seekable_seek(G_SEEKABLE(p->stream),
                               (goffset) offset, lseek_to_seek_type(whence),
                               NULL, &error);
      if (sought)
        position = g_seekable_tell(G_SEEKABLE(p->stream));
      else
        {
          g_warning("%s", error->message);
          g_error_free(error);
        }
    }
  else
    {
      switch (whence)
        {
        default:
        case SEEK_SET:
          if (offset <= p->allocated)
            position = p->position = offset;
          break;

        case SEEK_CUR:
          if (p->position + offset <= p->allocated)
            position = p->position += offset;
          break;

        case G_SEEK_END:
          position = p->position = p->allocated + offset;
          break;
        }
    }

  return (toff_t) position;
}

static int
close_stream(thandle_t handle)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  gsize total = 0;
  gboolean closed = FALSE;

  g_assert(p->stream);

  /* !can_seek: file content is now fully cached, time to write it down. */
  if (!p->can_seek && p->buffer != NULL && p->allocated > 0)
    {
      while (total < p->allocated)
        {
          gssize written;

          written = g_output_stream_write(G_OUTPUT_STREAM(p->stream),
                                          (void *) (p->buffer + total),
                                          p->allocated - total,
                                          NULL, &error);
          if (written < 0)
            {
                  g_warning("%s", error->message);
                  g_error_free(error);
                  break;
            }

          total += written;
        }
    }

  closed = g_output_stream_close(G_OUTPUT_STREAM(p->stream),
                                 NULL, &error);
  if (!closed)
    {
      g_warning("%s", error->message);
      g_error_free(error);
    }

  g_clear_object(&p->stream);

  p->position = 0;

  g_clear_pointer(&p->buffer, g_free);

  p->allocated = 0;

  return (closed) ? 0 : -1;
}

static toff_t
get_file_size(thandle_t handle)
{
  Priv *p = (Priv*) handle;
  GError *error = NULL;
  GFileInfo *info;
  goffset size;

  g_assert(p->stream);

  size = p->allocated;

  if (p->file != NULL)
    {
      info = g_file_query_info(p->file,
                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
                               G_FILE_QUERY_INFO_NONE,
                               NULL, &error);
      if (info == NULL)
        {
          g_warning("%s", error->message);
          g_error_free(error);
        }
      else
        {
          if (g_file_info_has_attribute(info, G_FILE_ATTRIBUTE_STANDARD_SIZE))
            size = g_file_info_get_size(info);
          g_object_unref(info);
        }
    }

  return (toff_t) size;
}

static gint
save_contiguous(GeglOperation *operation,
                GeglBuffer    *input,
                const GeglRectangle *result,
                const Babl *format)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  gint bytes_per_pixel, bytes_per_row;
  gint tile_width = result->width;
  gint tile_height = result->height;
  guchar *buffer;
  gint x, y;

  g_return_val_if_fail(p->tiff != NULL, -1);

  bytes_per_pixel = babl_format_get_bytes_per_pixel(format);
  bytes_per_row = bytes_per_pixel * tile_width;

  buffer = g_try_new(guchar, bytes_per_row * tile_height);

  g_assert(buffer != NULL);

  for (y = result->y; y < result->y + tile_height; y += tile_height)
    {
      for (x = result->x; x < result->x + tile_width; x += tile_width)
        {
          GeglRectangle tile = { x, y, tile_width, tile_height };
          gint row;

          gegl_buffer_get(input, &tile, 1.0, format, buffer,
                          GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          for (row = 0; row < tile_height; row++)
            {
              guchar *tile_row = buffer + (bytes_per_row * row);
              gint written;

              written = TIFFWriteScanline(p->tiff, tile_row, row, 0);

              if (!written)
                {
                  g_critical("failed a scanline write on row %d", row);
                  continue;
                }
            }
        }
    }

  TIFFFlushData(p->tiff);

  g_free(buffer);
  return 0;
}

static void
SetFieldString (TIFF *tiff, guint tag, GeglMetadata *metadata, const gchar *name)
{
  GValue gvalue = G_VALUE_INIT;
  GeglMetadataIter iter;

  g_value_init (&gvalue, G_TYPE_STRING);
  if (gegl_metadata_iter_lookup (metadata, &iter, name)
      && gegl_metadata_iter_get_value (metadata, &iter, &gvalue))
    {
      TIFFSetField (tiff, tag, g_value_get_string (&gvalue));
    }
  g_value_unset (&gvalue);
}

static int
export_tiff (GeglOperation *operation,
             GeglBuffer *input,
             const GeglRectangle *result)
{
  const Babl *space;
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = (Priv*) o->user_data;
  gshort color_space, compression = COMPRESSION_NONE;
  gushort bits_per_sample, samples_per_pixel;
  gboolean has_alpha, alpha_is_premultiplied = FALSE;
  BablModelFlag model_flags;
  gushort sample_format, predictor = 0;
  gushort extra_types[1];
  glong rows_per_stripe = 1;
  gint bytes_per_row;
  const Babl *type, *model;
  gchar format_string[32];
  const Babl *format;

  g_return_val_if_fail(p->tiff != NULL, -1);

  TIFFSetField(p->tiff, TIFFTAG_SUBFILETYPE, 0);
  TIFFSetField(p->tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);

  TIFFSetField(p->tiff, TIFFTAG_IMAGEWIDTH, result->width);
  TIFFSetField(p->tiff, TIFFTAG_IMAGELENGTH, result->height);

  format = gegl_buffer_get_format(input);
  model = babl_format_get_model(format);
  space = babl_format_get_space (format);
  type = babl_format_get_type(format, 0);
  model_flags = babl_get_model_flags (model);

  {
    int icc_len;
    const char *icc_profile;
    icc_profile = babl_space_get_icc (space, &icc_len);
    if (icc_profile)
      TIFFSetField (p->tiff, TIFFTAG_ICCPROFILE, icc_len, icc_profile);
  }

  if (babl_space_is_cmyk (space))
    {
      color_space = PHOTOMETRIC_SEPARATED;
      predictor = 2;
      if (model_flags & BABL_MODEL_FLAG_ALPHA)
      {
        has_alpha = TRUE;
        alpha_is_premultiplied = FALSE;
        model = babl_model("CMYKA");
        samples_per_pixel = 5;
      }
      else
      {
        has_alpha = FALSE;
        model = babl_model("CMYK");
        samples_per_pixel = 4;
      }
    }
  if (babl_model_is (model, "Y") || babl_model_is (model, "Y'"))
    {
      has_alpha = FALSE;
      color_space = PHOTOMETRIC_MINISBLACK;
      model = babl_model("Y'");
      samples_per_pixel = 1;
    }
  else if (babl_model_is (model, "YA") || babl_model_is (model, "Y'A"))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = FALSE;
      color_space = PHOTOMETRIC_MINISBLACK;
      model = babl_model("Y'A");
      samples_per_pixel = 2;
    }
  else if (babl_model_is (model, "YaA") || babl_model_is (model, "Y'aA"))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = TRUE;
      color_space = PHOTOMETRIC_MINISBLACK;
      model = babl_model("Y'aA");
      samples_per_pixel = 2;
    }
  else if (babl_model_is (model, "cmykA") ||
           babl_model_is (model, "CMYKA") ||
           babl_model_is (model, "camayakaA")||
           babl_model_is (model, "CaMaYaKaA") ||
           babl_space_is_cmyk (space))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = FALSE;
      color_space = PHOTOMETRIC_SEPARATED;
      model = babl_model("CMYKA");
      samples_per_pixel = 5;
      predictor = 2;
    }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "CMYK"))
    {
      has_alpha = FALSE;
      color_space = PHOTOMETRIC_SEPARATED;
      model = babl_model("CMYK");
      samples_per_pixel = 4;
      predictor = 2;
    }
  else if (babl_model_is (model, "RGB") || babl_model_is (model, "R'G'B'"))
    {
      has_alpha = FALSE;
      color_space = PHOTOMETRIC_RGB;
      model = babl_model("R'G'B'");
      samples_per_pixel = 3;
      predictor = 2;
    }
#if 0
  else if (babl_model_is (model, "camayakaA")||
           babl_model_is (model, "CaMaYaKaA"))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = TRUE;
      color_space = PHOTOMETRIC_SEPARATED;
      model = babl_model("CaMaYaKaA");
      samples_per_pixel = 5;
      predictor = 2;
    }
#endif
  else if (babl_model_is (model, "RGBA") || babl_model_is (model, "R'G'B'A"))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = FALSE;
      color_space = PHOTOMETRIC_RGB;
      model = babl_model("R'G'B'A");
      samples_per_pixel = 4;
      predictor = 2;
    }
  else if (babl_model_is (model, "RaGaBaA") || babl_model_is (model, "R'aG'aB'aA"))
    {
      has_alpha = TRUE;
      alpha_is_premultiplied = TRUE;
      color_space = PHOTOMETRIC_RGB;
      model = babl_model("R'aG'aB'aA");
      samples_per_pixel = 4;
      predictor = 2;
    }
  else
    {
      g_warning("color space not supported: %s", babl_get_name(model));

      has_alpha = TRUE;
      alpha_is_premultiplied = TRUE;
      color_space = PHOTOMETRIC_RGB;
      model = babl_model("R'aG'aB'aA");
      samples_per_pixel = 4;
      predictor = 2;
    }

  TIFFSetField(p->tiff, TIFFTAG_PHOTOMETRIC, color_space);
  TIFFSetField(p->tiff, TIFFTAG_SAMPLESPERPIXEL, samples_per_pixel);
  TIFFSetField(p->tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (has_alpha)
    {
      if (alpha_is_premultiplied)
        extra_types[0] = EXTRASAMPLE_ASSOCALPHA;
      else
        extra_types[0] = EXTRASAMPLE_UNASSALPHA;

      TIFFSetField(p->tiff, TIFFTAG_EXTRASAMPLES, 1, extra_types);
    }

  if (predictor != 0)
    {
      if (compression == COMPRESSION_LZW)
        TIFFSetField(p->tiff, TIFFTAG_PREDICTOR, predictor);
      else if (compression == COMPRESSION_ADOBE_DEFLATE)
        TIFFSetField(p->tiff, TIFFTAG_PREDICTOR, predictor);
    }

  if (type == babl_type("u8"))
    {
      sample_format = SAMPLEFORMAT_UINT;
      bits_per_sample = 8;
    }
  else if (type == babl_type("half"))
    {
      sample_format = SAMPLEFORMAT_IEEEFP;
      bits_per_sample = 16;
    }
  else if (type == babl_type("u16"))
    {
      sample_format = SAMPLEFORMAT_UINT;
      bits_per_sample = 16;
    }
  else if (type == babl_type("float"))
    {
      sample_format = SAMPLEFORMAT_IEEEFP;
      bits_per_sample = 32;
    }
  else if (type == babl_type("u32"))
    {
      sample_format = SAMPLEFORMAT_UINT;
      bits_per_sample = 32;
    }
  else  if (type == babl_type("double"))
    {
      sample_format = SAMPLEFORMAT_IEEEFP;
      bits_per_sample = 64;
    }
  else
    {
      g_warning("sample format not supported: %s", babl_get_name(type));
      sample_format = SAMPLEFORMAT_UINT;
      type = babl_type("u8");
      bits_per_sample = 8;
    }

  if (o->bitdepth > 0)
  {
    switch (o->bitdepth)
    {
      case 8:
        bits_per_sample = 8;
      break;
      case 16:
        bits_per_sample = 16;
      break;
      case 32:
        bits_per_sample = 32;
      break;
      case 64:
        bits_per_sample = 64;
      break;
    }
  }
  if (o->fp>= 0)
  {
    if (o->fp== 1)
      sample_format = SAMPLEFORMAT_IEEEFP;
    else
      sample_format = SAMPLEFORMAT_UINT;
  }


  TIFFSetField(p->tiff, TIFFTAG_BITSPERSAMPLE, bits_per_sample);
  TIFFSetField(p->tiff, TIFFTAG_SAMPLEFORMAT, sample_format);

  TIFFSetField(p->tiff, TIFFTAG_COMPRESSION, compression);

  if ((compression == COMPRESSION_CCITTFAX3 ||
       compression == COMPRESSION_CCITTFAX4) &&
       (bits_per_sample != 1 || samples_per_pixel != 1))
    {
      g_critical("only monochrome pictures can be compressed "
                 "with \"CCITT Group 4\" or \"CCITT Group 3\"");
      return -1;
    }

  if (o->bitdepth > 0 || o->fp >= 0)
  {
    int ieeef = o->fp;
    if (ieeef == -1)
      ieeef = (sample_format == SAMPLEFORMAT_IEEEFP);

    switch (bits_per_sample)
    {
      case 8: type = babl_type ("u8");
              sample_format = SAMPLEFORMAT_UINT;
              break;
      case 16: type = babl_type (ieeef==1?"half":"u16"); break;
      case 32: type = babl_type (ieeef==1?"float":"u32"); break;
      case 64: type = babl_type ("double");
              sample_format = SAMPLEFORMAT_IEEEFP;
              break;
    }
  }

  g_snprintf(format_string, 32, "%s %s",
              babl_get_name(model), babl_get_name(type));

  format = babl_format_with_space (format_string, space);

  /* "Choose RowsPerStrip such that each strip is about 8K bytes." */
  bytes_per_row = babl_format_get_bytes_per_pixel(format) * result->width;
  while (bytes_per_row * rows_per_stripe <= 8192)
    rows_per_stripe++;

  rows_per_stripe = MIN(rows_per_stripe, result->height);

  TIFFSetField(p->tiff, TIFFTAG_ROWSPERSTRIP, rows_per_stripe);

  if (o->metadata != NULL)
    {
      GeglResolutionUnit unit;
      gfloat xres, yres;

      gegl_metadata_register_map (GEGL_METADATA (o->metadata),
                                  "gegl:tiff-save",
                                  GEGL_MAP_EXCLUDE_UNMAPPED,
                                  tiff_save_metadata,
                                  G_N_ELEMENTS (tiff_save_metadata));

      if (gegl_metadata_get_resolution (GEGL_METADATA (o->metadata),
                                        &unit, &xres, &yres))
        switch (unit)
        {
          case GEGL_RESOLUTION_UNIT_DPI:
            TIFFSetField (p->tiff, TIFFTAG_XRESOLUTION, xres);
            TIFFSetField (p->tiff, TIFFTAG_YRESOLUTION, yres);
            TIFFSetField (p->tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
            break;
          case GEGL_RESOLUTION_UNIT_DPM:
            TIFFSetField (p->tiff, TIFFTAG_XRESOLUTION, xres / 100.0f);
            TIFFSetField (p->tiff, TIFFTAG_YRESOLUTION, yres / 100.0f);
            TIFFSetField (p->tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
            break;
          case GEGL_RESOLUTION_UNIT_NONE:
          default:
            TIFFSetField (p->tiff, TIFFTAG_XRESOLUTION, xres);
            TIFFSetField (p->tiff, TIFFTAG_YRESOLUTION, yres);
            TIFFSetField (p->tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);
            break;
        }

      //XXX make and model for scanner

      SetFieldString (p->tiff, TIFFTAG_ARTIST,
                      GEGL_METADATA (o->metadata), "Artist");
      SetFieldString (p->tiff, TIFFTAG_COPYRIGHT,
                      GEGL_METADATA (o->metadata), "Copyright");
      SetFieldString (p->tiff, TIFFTAG_PAGENAME,
                      GEGL_METADATA (o->metadata), "PageName");
      SetFieldString (p->tiff, TIFFTAG_SOFTWARE,
                      GEGL_METADATA (o->metadata), "Software");
      SetFieldString (p->tiff, TIFFTAG_DATETIME,
                      GEGL_METADATA (o->metadata), "DateTime");
      SetFieldString (p->tiff, TIFFTAG_IMAGEDESCRIPTION,
                      GEGL_METADATA (o->metadata), "ImageDescription");

      gegl_metadata_unregister_map (GEGL_METADATA (o->metadata));
    }

  return save_contiguous(operation, input, result, format);
}

static gboolean
process(GeglOperation *operation,
        GeglBuffer *input,
        const GeglRectangle *result,
        int level)
{
  GeglProperties *o = GEGL_PROPERTIES(operation);
  Priv *p = g_new0(Priv, 1);
  gboolean status = TRUE;
  GError *error = NULL;

  g_assert(p != NULL);

  o->user_data = (void*) p;

  p->stream = gegl_gio_open_output_stream(NULL, o->path, &p->file, &error);
  if (p->stream != NULL && p->file != NULL)
    p->can_seek = g_seekable_can_seek(G_SEEKABLE(p->stream));
  if (p->stream == NULL)
    {
      status = FALSE;
      g_warning("%s", error->message);
      goto cleanup;
    }

  TIFFSetErrorHandler(error_handler);
  TIFFSetWarningHandler(warning_handler);

  p->tiff = TIFFClientOpen("GEGL-tiff-save", "w", (thandle_t) p,
                           read_from_stream, write_to_stream,
                           seek_in_stream, close_stream,
                           get_file_size, NULL, NULL);
  if (p->tiff == NULL)
    {
      status = FALSE;
      g_warning("failed to open TIFF from %s", o->path);
      goto cleanup;
    }

  if (export_tiff(operation, input, result))
    {
      status = FALSE;
      g_warning("could not export TIFF file");
      goto cleanup;
    }

cleanup:
  cleanup(operation);
  g_clear_pointer(&o->user_data, g_free);
  g_clear_error(&error);
  return status;
}

static void
gegl_op_class_init(GeglOpClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS(klass);
  sink_class = GEGL_OPERATION_SINK_CLASS(klass);

  sink_class->needs_full = TRUE;
  sink_class->process = process;

  gegl_operation_class_set_keys(operation_class,
    "name",          "gegl:tiff-save",
    "title",       _("TIFF File Saver"),
    "categories",    "output",
    "description", _("TIFF image saver using libtiff"),
    NULL);

  gegl_operation_handlers_register_saver(
    ".tiff", "gegl:tiff-save");
  gegl_operation_handlers_register_saver(
    ".tif", "gegl:tiff-save");
}

#endif

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
 * Copyright 2011 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gegl-metadata.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Target path and filename, use '-' for stdout"))

property_int (quality, _("Quality"), 90)
  description (_("JPEG compression quality (between 1 and 100)"))
  value_range (1, 100)

property_int (smoothing, _("Smoothing"), 0)
  description(_("Smoothing factor from 1 to 100; 0 disables smoothing"))
  value_range (0, 100)

property_boolean (optimize, _("Optimize"), TRUE)
  description (_("Use optimized huffman tables"))
property_boolean (progressive, _("Progressive"), TRUE)
  description (_("Create progressive JPEG images"))

property_boolean (grayscale, _("Grayscale"), FALSE)
  description (_("Create a grayscale (monochrome) image"))

property_object(metadata, _("Metadata"), GEGL_TYPE_METADATA)
  description (_("Object providing image metadata"))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME jpg_save
#define GEGL_OP_C_SOURCE jpg-save.c

#include <gegl-op.h>
#include <gegl-gio-private.h>
#include <stdio.h> /* jpeglib.h needs FILE... */
#include <jpeglib.h>

static const gsize buffer_size = 4096;

static void
iso8601_format_timestamp (const GValue *src_value, GValue *dest_value)
{
  GDateTime *datetime;
  gchar *datestr;

  g_return_if_fail (G_TYPE_CHECK_VALUE_TYPE (src_value, G_TYPE_DATE_TIME));
  g_return_if_fail (G_VALUE_HOLDS_STRING (dest_value));

  datetime = g_value_get_boxed (src_value);
  g_return_if_fail (datetime != NULL);

#if GLIB_CHECK_VERSION(2,62,0)
  datestr = g_date_time_format_iso8601 (datetime);
#else
  datestr = g_date_time_format (datetime, "%FT%TZ");
#endif
  g_return_if_fail (datestr != NULL);

  g_value_take_string (dest_value, datestr);
}

static const GeglMetadataMap jpeg_save_metadata[] =
{
  { "Artist",                 "artist",       NULL },
  { "Comment",                "comment",      NULL },
  { "Copyright",              "copyright",    NULL },
  { "Description",            "description",  NULL },
  { "Disclaimer",             "disclaimer",   NULL },
  { "Software",               "software",     NULL },
  { "Timestamp",              "timestamp",    iso8601_format_timestamp },
  { "Title",                  "title",        NULL },
  { "Warning",                "warning",      NULL },
};


static void
init_buffer (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  guchar *buffer;

  buffer = g_try_new (guchar, buffer_size);

  g_assert (buffer != NULL);

  dest->next_output_byte = buffer;
  dest->free_in_buffer = buffer_size;
}

static boolean
write_to_stream (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  GOutputStream *stream = (GOutputStream *) cinfo->client_data;
  GError *error = NULL;
  guchar *buffer;
  gsize size;
  gboolean success;
  gsize written;

  g_assert (stream);

  size = buffer_size - dest->free_in_buffer;
  buffer = (guchar *) dest->next_output_byte - size;

  success = g_output_stream_write_all (G_OUTPUT_STREAM (stream),
                                       (const void *) buffer, buffer_size, &written,
                                       NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return FALSE;
    }

  dest->next_output_byte = buffer;
  dest->free_in_buffer = buffer_size;

  return TRUE;
}

static void
close_stream (j_compress_ptr cinfo)
{
  struct jpeg_destination_mgr *dest = cinfo->dest;
  GOutputStream *stream = (GOutputStream *) cinfo->client_data;
  GError *error = NULL;
  guchar *buffer;
  gsize size;
  gboolean success;
  gsize written;
  gboolean closed;

  g_assert (stream);

  size = buffer_size - dest->free_in_buffer;
  buffer = (guchar *) dest->next_output_byte - size;

  success = g_output_stream_write_all (G_OUTPUT_STREAM (stream),
                                       (const void *) buffer, size, &written,
                                       NULL, &error);
  if (!success || error != NULL)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  closed = g_output_stream_close (G_OUTPUT_STREAM (stream),
                                  NULL, &error);
  if (!closed)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_free (buffer);

  dest->next_output_byte = NULL;
  dest->free_in_buffer = 0;
}

/*
 * Since an ICC profile can be larger than the maximum size of a JPEG marker
 * (64K), we need provisions to split it into multiple markers.  The format
 * defined by the ICC specifies one or more APP2 markers containing the
 * following data:
 *	Identifying string	ASCII "ICC_PROFILE\0"  (12 bytes)
 *	Marker sequence number	1 for first APP2, 2 for next, etc (1 byte)
 *	Number of markers	Total number of APP2's used (1 byte)
 *      Profile data		(remainder of APP2 data)
 * Decoders should use the marker sequence numbers to reassemble the profile,
 * rather than assuming that the APP2 markers appear in the correct sequence.
 */

#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

/*
 * This routine writes the given ICC profile data into a JPEG file.
 * It *must* be called AFTER calling jpeg_start_compress() and BEFORE
 * the first call to jpeg_write_scanlines().
 * (This ordering ensures that the APP2 marker(s) will appear after the
 * SOI and JFIF or Adobe markers, but before all else.)
 */

static void
write_icc_profile (j_compress_ptr cinfo,
		   const JOCTET *icc_data_ptr,
		   unsigned int icc_data_len)
{
  unsigned int num_markers;	/* total number of markers we'll write */
  int cur_marker = 1;		/* per spec, counting starts at 1 */
  unsigned int length;		/* number of bytes to write in this marker */

  /* Calculate the number of markers we'll need, rounding up of course */
  num_markers = icc_data_len / MAX_DATA_BYTES_IN_MARKER;
  if (num_markers * MAX_DATA_BYTES_IN_MARKER != icc_data_len)
    num_markers++;

  while (icc_data_len > 0) {
    /* length of profile to put in this marker */
    length = icc_data_len;
    if (length > MAX_DATA_BYTES_IN_MARKER)
      length = MAX_DATA_BYTES_IN_MARKER;
    icc_data_len -= length;

    /* Write the JPEG marker header (APP2 code and marker length) */
    jpeg_write_m_header(cinfo, ICC_MARKER,
			(unsigned int) (length + ICC_OVERHEAD_LEN));

    /* Write the marker identifying string "ICC_PROFILE" (null-terminated).
     * We code it in this less-than-transparent way so that the code works
     * even if the local character set is not ASCII.
     */
    jpeg_write_m_byte(cinfo, 0x49);
    jpeg_write_m_byte(cinfo, 0x43);
    jpeg_write_m_byte(cinfo, 0x43);
    jpeg_write_m_byte(cinfo, 0x5F);
    jpeg_write_m_byte(cinfo, 0x50);
    jpeg_write_m_byte(cinfo, 0x52);
    jpeg_write_m_byte(cinfo, 0x4F);
    jpeg_write_m_byte(cinfo, 0x46);
    jpeg_write_m_byte(cinfo, 0x49);
    jpeg_write_m_byte(cinfo, 0x4C);
    jpeg_write_m_byte(cinfo, 0x45);
    jpeg_write_m_byte(cinfo, 0x0);

    /* Add the sequencing info */
    jpeg_write_m_byte(cinfo, cur_marker);
    jpeg_write_m_byte(cinfo, (int) num_markers);

    /* Add the profile data */
    while (length--) {
      jpeg_write_m_byte(cinfo, *icc_data_ptr);
      icc_data_ptr++;
    }
    cur_marker++;
  }
}



static gint
export_jpg (GeglOperation               *operation,
            GeglBuffer                  *input,
            const GeglRectangle         *result,
            struct jpeg_compress_struct  cinfo,
            gint                         quality,
            gint                         smoothing,
            gboolean                     optimize,
            gboolean                     progressive,
            gboolean                     grayscale,
            GeglMetadata                *metadata)
{
  gint     src_x, src_y;
  gint     width, height;
  JSAMPROW row_pointer[1];
  const Babl *format;
  const Babl *fmt = gegl_buffer_get_format (input);
  const Babl *space = babl_format_get_space (fmt);
  gint     cmyk = babl_space_is_cmyk (space);
  gint     gray = babl_space_is_gray (space);

  src_x = result->x;
  src_y = result->y;
  width = result->width;
  height = result->height;

  if (gray)
    grayscale = 1;

  cinfo.image_width = width;
  cinfo.image_height = height;

  if (!grayscale)
    {
      if (cmyk)
      {
        cinfo.input_components = 4;
        cinfo.in_color_space = JCS_CMYK;
      }
      else
      {
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;
      }
    }
  else
    {
      cinfo.input_components = 1;
      cinfo.in_color_space = JCS_GRAYSCALE;
    }

  jpeg_set_defaults (&cinfo);
  jpeg_set_quality (&cinfo, quality, TRUE);
  cinfo.smoothing_factor = smoothing;
  cinfo.optimize_coding = optimize;
  if (progressive)
    jpeg_simple_progression (&cinfo);

  /* Use 1x1,1x1,1x1 MCUs and no subsampling */
  cinfo.comp_info[0].h_samp_factor = 1;
  cinfo.comp_info[0].v_samp_factor = 1;

  if (!grayscale)
    {
      cinfo.comp_info[1].h_samp_factor = 1;
      cinfo.comp_info[1].v_samp_factor = 1;
      cinfo.comp_info[2].h_samp_factor = 1;
      cinfo.comp_info[2].v_samp_factor = 1;
    }

  /* No restart markers */
  cinfo.restart_interval = 0;
  cinfo.restart_in_rows = 0;

  /* Resolution */
  if (metadata != NULL)
    {
      GeglResolutionUnit unit;
      gfloat resx, resy;

      gegl_metadata_register_map (metadata, "gegl:jpg-save", 0,
                                  jpeg_save_metadata,
                                  G_N_ELEMENTS (jpeg_save_metadata));

      if (gegl_metadata_get_resolution (metadata, &unit, &resx, &resy))
        switch (unit)
          {
          case GEGL_RESOLUTION_UNIT_DPI:
            cinfo.density_unit = 1;               /* dots/inch */
            cinfo.X_density = lroundf (resx);
            cinfo.Y_density = lroundf (resy);
            break;
          case GEGL_RESOLUTION_UNIT_DPM:
            cinfo.density_unit = 2;               /* dots/cm */
            cinfo.X_density = lroundf (resx / 100.0f);
            cinfo.Y_density = lroundf (resy / 100.0f);
            break;
          case GEGL_RESOLUTION_UNIT_NONE:
          default:
            cinfo.density_unit = 0;               /* unknown */
            cinfo.X_density = lroundf (resx);
            cinfo.Y_density = lroundf (resy);
            break;
          }
    }

  jpeg_start_compress (&cinfo, TRUE);

  if (metadata != NULL)
    {
      GValue value = G_VALUE_INIT;
      GeglMetadataIter iter;
      const gchar *keyword, *text;
      GString *string;

      string = g_string_new (NULL);

      g_value_init (&value, G_TYPE_STRING);
      gegl_metadata_iter_init (metadata, &iter);
      while ((keyword = gegl_metadata_iter_next (metadata, &iter)) != NULL)
        {
          if (gegl_metadata_iter_get_value (metadata, &iter, &value))
            {
              text = g_value_get_string (&value);
              g_string_append_printf (string, "## %s\n", keyword);
              g_string_append (string, text);
              g_string_append (string, "\n\n");
            }
        }
      jpeg_write_marker (&cinfo, JPEG_COM, (guchar *) string->str, string->len);
      g_value_unset (&value);
      g_string_free (string, TRUE);

      gegl_metadata_unregister_map (metadata);
    }

  {
    int icc_len;
    const char *icc_profile;
    icc_profile = babl_space_get_icc (space, &icc_len);
    /* XXX : we should write a grayscale profile - possible created from the
             RGB - if the incoming space has a non-grayscale ICC profile */
    if (icc_profile)
      write_icc_profile (&cinfo, (void*)icc_profile, icc_len);
  }

  if (!grayscale)
    {
      if (cmyk)
      {
        format = babl_format_with_space ("cmyk u8", space);
        row_pointer[0] = g_malloc (width * 4);
      }
      else
      {
        format = babl_format_with_space ("R'G'B' u8", space);
        row_pointer[0] = g_malloc (width * 3);
      }
    }
  else
    {
      format = babl_format_with_space ("Y' u8", space);
      row_pointer[0] = g_malloc (width);
    }

  while (cinfo.next_scanline < cinfo.image_height) {
    GeglRectangle rect;

    rect.x = src_x;
    rect.y = src_y + cinfo.next_scanline;
    rect.width = width;
    rect.height = 1;

    gegl_buffer_get (input, &rect, 1.0, format,
                     row_pointer[0], GEGL_AUTO_ROWSTRIDE,
                     GEGL_ABYSS_NONE);

    jpeg_write_scanlines (&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress (&cinfo);

  g_free (row_pointer[0]);

  return 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *result,
         int                  level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct jpeg_destination_mgr dest;
  GOutputStream *stream;
  GFile *file = NULL;
  gboolean status = TRUE;
  GError *error = NULL;

  cinfo.err = jpeg_std_error (&jerr);

  jpeg_create_compress (&cinfo);

  stream = gegl_gio_open_output_stream (NULL, o->path, &file, &error);
  if (stream == NULL)
    {
      status = FALSE;
      g_warning ("%s", error->message);
      goto cleanup;
    }

  dest.init_destination = init_buffer;
  dest.empty_output_buffer = write_to_stream;
  dest.term_destination = close_stream;

  cinfo.client_data = stream;
  cinfo.dest = &dest;

  if (export_jpg (operation, input, result, cinfo,
                  o->quality, o->smoothing, o->optimize, o->progressive, o->grayscale,
                  GEGL_METADATA (o->metadata)))
    {
      status = FALSE;
      g_warning("could not export JPEG file");
      goto cleanup;
    }

cleanup:
  jpeg_destroy_compress (&cinfo);

  if (stream != NULL)
    g_clear_object (&stream);

  if (file != NULL)
    g_clear_object (&file);

  return  status;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process    = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",          "gegl:jpg-save",
    "title",       _("JPEG File Saver"),
    "categories",    "output",
    "description", _("JPEG image saver, using libjpeg"),
    NULL);

  gegl_operation_handlers_register_saver (
    ".jpeg", "gegl:jpg-save");
  gegl_operation_handlers_register_saver (
    ".jpg", "gegl:jpg-save");
}

#endif

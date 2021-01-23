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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
  description (_("Path of file to load"))
property_uri (uri, _("URI"), "")
  description (_("URI of file to load"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME jpg_load
#define GEGL_OP_C_SOURCE jpg-load.c

#include "gegl-op.h"
#include <stdio.h>
#include <jpeglib.h>
#include <gegl-gio-private.h>

/* icc-loading code from:  http://www.littlecms.com/1/iccjpeg.c */

static boolean
read_icc_profile (j_decompress_ptr cinfo,
		  JOCTET **icc_data_ptr,
		  unsigned int *icc_data_len);
static void
setup_read_icc_profile (j_decompress_ptr cinfo);


static const gchar *
jpeg_colorspace_name(J_COLOR_SPACE space)
{
    static const gchar * const names[] = {
        "Unknown",
        "Grayscale",
        "RGB",
        "YCbCr",
        "CMYK",
        "YCCK"
    };
    const gint n_valid_names = G_N_ELEMENTS(names);
    const gint idx = (space > 0 && space < n_valid_names) ? (gint)space : 0;
    return names[idx];
}
#include <stdio.h>
static const Babl *
babl_from_jpeg_colorspace(J_COLOR_SPACE jpgspace, const Babl *space)
{
    // XXX: assumes bitdepth == 8
  const Babl *format = NULL;
  if (jpgspace == JCS_GRAYSCALE)
    format = babl_format_with_space ("Y' u8", space);
  else if (jpgspace == JCS_RGB)
    format = babl_format_with_space ("R'G'B' u8", space);
  else if (jpgspace == JCS_CMYK) {
    format = babl_format_with_space ("cmyk u8", space);
  }

  return format;
}

typedef struct {
    GInputStream *stream;
    gchar *buffer;
    gsize buffer_size;
} GioSource;

static boolean
gio_source_fill(j_decompress_ptr cinfo)
{
    struct jpeg_source_mgr* src = cinfo->src;
    GioSource *self = (GioSource *)cinfo->client_data;
    GError *err = NULL;

    const gssize bytes_read = g_input_stream_read(self->stream, self->buffer,
                                                  self->buffer_size, NULL, &err);
    if (!err)
      {
        src->next_input_byte = (JOCTET*)self->buffer;
        src->bytes_in_buffer = bytes_read;
      }
    else
      {
        g_print("%s: %s\n", __PRETTY_FUNCTION__, err->message);
      }
    

    /* FIXME: needed for EOF?
    static const JOCTET EOI_BUFFER[ 2 ] = { (JOCTET)0xFF, (JOCTET)JPEG_EOI };
    src->next_input_byte = EOI_BUFFER;
    src->bytes_in_buffer = sizeof( EOI_BUFFER );
    */

    return TRUE;
}

static void
gio_source_skip(j_decompress_ptr cinfo, long num_bytes)
{
    struct jpeg_source_mgr* src = (struct jpeg_source_mgr*)cinfo->src;
    GioSource *self = (GioSource *)cinfo->client_data;

    if (num_bytes < src->bytes_in_buffer)
      {
        // just skip inside buffer
        src->next_input_byte += (size_t)num_bytes;
        src->bytes_in_buffer -= (size_t)num_bytes;
      }
    else
      {
        // skip in stream, discard whole buffer
        GError *err = NULL;
        const gssize bytes_to_skip = num_bytes-src->bytes_in_buffer;
        const gssize skipped = g_input_stream_skip(self->stream, bytes_to_skip, NULL, &err);
        if (err)
          {
            g_printerr("%s: skipped=%"G_GSSIZE_FORMAT", err=%s\n", __PRETTY_FUNCTION__, skipped, err->message);
            g_error_free(err);
          }
        src->bytes_in_buffer = 0;
        src->next_input_byte = NULL;
      }

}

static void
gio_source_init(j_decompress_ptr cinfo)
{
    GioSource *self = (GioSource *)cinfo->client_data;
    self->buffer = g_new(gchar, self->buffer_size);
}
 
static void
gio_source_destroy(j_decompress_ptr cinfo)
{
    GioSource *self = (GioSource *)cinfo->client_data;
    g_free(self->buffer);
}

static void
gio_source_enable(j_decompress_ptr cinfo, struct jpeg_source_mgr* src, GioSource *data)
{
    src->resync_to_restart = jpeg_resync_to_restart;

    src->init_source = gio_source_init;
    src->fill_input_buffer = gio_source_fill;
    src->skip_input_data = gio_source_skip;
    src->term_source = gio_source_destroy;

    // force fill at once
    src->bytes_in_buffer = 0;
    src->next_input_byte = (JOCTET*)NULL;

    //g_assert(!cinfo->client_data);
    cinfo->client_data = data;
    cinfo->src = src;
}

static const Babl *jpg_get_space (struct jpeg_decompress_struct *cinfo)
{
  JOCTET *icc_data = NULL;
  unsigned int icc_len;
  read_icc_profile (cinfo, &icc_data, &icc_len);
  if (icc_data)
  {
    const char *error = NULL;
    const Babl *ret;
    ret = babl_space_from_icc ((char*)icc_data, (int)icc_len,
                               BABL_ICC_INTENT_RELATIVE_COLORIMETRIC,
                               &error);
    if (error)
      g_warning ("error creating space from icc: %s\n", error);
    free (icc_data);
    return ret;
  }
  return NULL;
}

static gint
gegl_jpg_load_query_jpg (GInputStream *stream,
                         gint         *width,
                         gint         *height,
                         const Babl  **out_format)
{
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  struct jpeg_source_mgr         src;
  gint                           status = 0;
  const Babl *format = NULL;
  GioSource gio_source = { stream, NULL, 1024 };

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  setup_read_icc_profile (&cinfo);

  gio_source_enable(&cinfo, &src, &gio_source);

  (void) jpeg_read_header (&cinfo, TRUE);

  format = babl_from_jpeg_colorspace(cinfo.out_color_space,
                                     jpg_get_space (&cinfo));
  if (width)
    *width = cinfo.image_width;
  if (height)
    *height = cinfo.image_height;
  if (out_format)
    *out_format = format;
  if (!format)
    {
      g_warning ("attempted to load JPEG with unsupported color space: '%s'",
                 jpeg_colorspace_name(cinfo.out_color_space));
      status = -1;
    }

  jpeg_destroy_decompress (&cinfo);

  return status;
}

static gint
gegl_jpg_load_buffer_import_jpg (GeglBuffer   *gegl_buffer,
                                 GInputStream *stream,
                                 gint          dest_x,
                                 gint          dest_y)
{
  gint row_stride;
  struct jpeg_decompress_struct  cinfo;
  struct jpeg_error_mgr          jerr;
  struct jpeg_source_mgr         src;
  JSAMPARRAY                     buffer;
  const Babl                    *format;
  GeglRectangle                  write_rect;
  GioSource gio_source = { stream, NULL, 1024 };

  cinfo.err = jpeg_std_error (&jerr);
  jpeg_create_decompress (&cinfo);
  setup_read_icc_profile (&cinfo);

  gio_source_enable(&cinfo, &src, &gio_source);

  (void) jpeg_read_header (&cinfo, TRUE);

  /* This is the most accurate method and could be the fastest too. But
   * the results may vary on different platforms due to different
   * rounding behavior and precision.
   */
  cinfo.dct_method = JDCT_FLOAT;

  (void) jpeg_start_decompress (&cinfo);

  format = babl_from_jpeg_colorspace(cinfo.out_color_space,
                                     jpg_get_space (&cinfo));
  if (!format)
    {
      g_warning ("attempted to load JPEG with unsupported color space: '%s'",
                 jpeg_colorspace_name(cinfo.out_color_space));
      jpeg_destroy_decompress (&cinfo);
      return -1;
    }

  row_stride = cinfo.output_width * cinfo.output_components;

  if ((row_stride) % 2)
    (row_stride)++;

  /* allocated with the jpeg library, and freed with the decompress context */
  buffer = (*cinfo.mem->alloc_sarray)
    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  write_rect.x = dest_x;
  write_rect.y = dest_y;
  write_rect.width  = cinfo.output_width;
  write_rect.height = 1;

  // Most CMYK JPEG files are produced by Adobe Photoshop. Each component is stored where 0 means 100% ink
  // However this might not be case for all. Gory details: https://bugzilla.mozilla.org/show_bug.cgi?id=674619
  //
  // inverted cmyks are however how babl now expects jpgs so we're good

  while (cinfo.output_scanline < cinfo.output_height)
    {
      jpeg_read_scanlines (&cinfo, buffer, 1);

      gegl_buffer_set (gegl_buffer, &write_rect, 0,
                       format, buffer[0],
                       GEGL_AUTO_ROWSTRIDE);
      write_rect.y += 1;
    }

  jpeg_destroy_decompress (&cinfo);

  return 0;
}

static GeglRectangle
gegl_jpg_load_get_bounding_box (GeglOperation *operation)
{
  gint width, height;
  GeglProperties   *o = GEGL_PROPERTIES (operation);
  const Babl *format = NULL;
  GFile *file = NULL;
  GError *err = NULL;
  gint status = -1;

  GInputStream *stream = gegl_gio_open_input_stream(o->uri, o->path, &file, &err);
  if (!stream)
    return (GeglRectangle) {0, 0, 0, 0};
  status = gegl_jpg_load_query_jpg (stream, &width, &height, &format);
  g_input_stream_close(stream, NULL, NULL);

  if (format)
    gegl_operation_set_format (operation, "output", format);

  g_object_unref(stream);
  g_clear_object(&file);
  if (err || status)
    return (GeglRectangle) {0, 0, 0, 0};
  else
    return (GeglRectangle) {0, 0, width, height};
}

static gboolean
gegl_jpg_load_process (GeglOperation       *operation,
                       GeglBuffer          *output,
                       const GeglRectangle *result,
                       gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GFile *file = NULL;
  GError *err = NULL;
  gint status = -1;
  GInputStream *stream = gegl_gio_open_input_stream(o->uri, o->path, &file, &err);
  if (!stream)
    return FALSE;
  status = gegl_jpg_load_buffer_import_jpg(output, stream, 0, 0);
  g_input_stream_close(stream, NULL, NULL);

  if (err)
    {
      g_warning ("%s failed to open file %s for reading: %s",
        G_OBJECT_TYPE_NAME (operation), o->path, err->message);
      g_object_unref(stream);
      return FALSE;
    }

  g_object_unref(stream);
  return status != 1;
}

static GeglRectangle
gegl_jpg_load_get_cached_region (GeglOperation       *operation,
                                 const GeglRectangle *roi)
{
  return gegl_jpg_load_get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = gegl_jpg_load_process;
  operation_class->get_bounding_box = gegl_jpg_load_get_bounding_box;
  operation_class->get_cached_region = gegl_jpg_load_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:jpg-load",
    "title",       _("JPEG File Loader"),
    "categories",  "hidden",
    "description", _("JPEG image loader using libjpeg"),
    NULL);

/*  static gboolean done=FALSE;
    if (done)
      return; */
  gegl_operation_handlers_register_loader (
    "image/jpeg", "gegl:jpg-load");
  gegl_operation_handlers_register_loader (
    ".jpeg", "gegl:jpg-load");
  gegl_operation_handlers_register_loader (
    ".jpg", "gegl:jpg-load");
/*  done = TRUE; */
}


/*
 * See if there was an ICC profile in the JPEG file being read;
 * if so, reassemble and return the profile data.
 *
 * TRUE is returned if an ICC profile was found, FALSE if not.
 * If TRUE is returned, *icc_data_ptr is set to point to the
 * returned data, and *icc_data_len is set to its length.
 *
 * IMPORTANT: the data at **icc_data_ptr has been allocated with malloc()
 * and must be freed by the caller with free() when the caller no longer
 * needs it.  (Alternatively, we could write this routine to use the
 * IJG library's memory allocator, so that the data would be freed implicitly
 * at jpeg_finish_decompress() time.  But it seems likely that many apps
 * will prefer to have the data stick around after decompression finishes.)
 *
 * NOTE: if the file contains invalid ICC APP2 markers, we just silently
 * return FALSE.  You might want to issue an error message instead.
 */




#define ICC_MARKER  (JPEG_APP0 + 2)	/* JPEG marker code for ICC */
#define ICC_OVERHEAD_LEN  14		/* size of non-profile data in APP2 */
#define MAX_BYTES_IN_MARKER  65533	/* maximum data len of a JPEG marker */
#define MAX_DATA_BYTES_IN_MARKER  (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)


/*
 * Prepare for reading an ICC profile
 */

static void
setup_read_icc_profile (j_decompress_ptr cinfo)
{
  /* Tell the library to keep any APP2 data it may find */
  jpeg_save_markers(cinfo, ICC_MARKER, 0xFFFF);
}


/*
 * Handy subroutine to test whether a saved marker is an ICC profile marker.
 */

static boolean
marker_is_icc (jpeg_saved_marker_ptr marker)
{
  return
    marker->marker == ICC_MARKER &&
    marker->data_length >= ICC_OVERHEAD_LEN &&
    /* verify the identifying string */
    GETJOCTET(marker->data[0]) == 0x49 &&
    GETJOCTET(marker->data[1]) == 0x43 &&
    GETJOCTET(marker->data[2]) == 0x43 &&
    GETJOCTET(marker->data[3]) == 0x5F &&
    GETJOCTET(marker->data[4]) == 0x50 &&
    GETJOCTET(marker->data[5]) == 0x52 &&
    GETJOCTET(marker->data[6]) == 0x4F &&
    GETJOCTET(marker->data[7]) == 0x46 &&
    GETJOCTET(marker->data[8]) == 0x49 &&
    GETJOCTET(marker->data[9]) == 0x4C &&
    GETJOCTET(marker->data[10]) == 0x45 &&
    GETJOCTET(marker->data[11]) == 0x0;
}

static boolean
read_icc_profile (j_decompress_ptr cinfo,
		  JOCTET **icc_data_ptr,
		  unsigned int *icc_data_len)
{
  jpeg_saved_marker_ptr marker;
  int num_markers = 0;
  int seq_no;
  JOCTET *icc_data;
  unsigned int total_length;
#define MAX_SEQ_NO  255		/* sufficient since marker numbers are bytes */
  char marker_present[MAX_SEQ_NO+1];	  /* 1 if marker found */
  unsigned int data_length[MAX_SEQ_NO+1]; /* size of profile data in marker */
  unsigned int data_offset[MAX_SEQ_NO+1]; /* offset for data in marker */

  *icc_data_ptr = NULL;		/* avoid confusion if FALSE return */
  *icc_data_len = 0;

  /* This first pass over the saved markers discovers whether there are
   * any ICC markers and verifies the consistency of the marker numbering.
   */

  for (seq_no = 1; seq_no <= MAX_SEQ_NO; seq_no++)
    marker_present[seq_no] = 0;

  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      if (num_markers == 0)
	num_markers = GETJOCTET(marker->data[13]);
      else if (num_markers != GETJOCTET(marker->data[13]))
	return FALSE;		/* inconsistent num_markers fields */
      seq_no = GETJOCTET(marker->data[12]);
      if (seq_no <= 0 || seq_no > num_markers)
	return FALSE;		/* bogus sequence number */
      if (marker_present[seq_no])
	return FALSE;		/* duplicate sequence numbers */
      marker_present[seq_no] = 1;
      data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
    }
  }

  if (num_markers == 0)
    return FALSE;

  /* Check for missing markers, count total space needed,
   * compute offset of each marker's part of the data.
   */

  total_length = 0;
  for (seq_no = 1; seq_no <= num_markers; seq_no++) {
    if (marker_present[seq_no] == 0)
      return FALSE;		/* missing sequence number */
    data_offset[seq_no] = total_length;
    total_length += data_length[seq_no];
  }

  if (total_length <= 0)
    return FALSE;		/* found only empty markers? */

  /* Allocate space for assembled data */
  icc_data = (JOCTET *) malloc(total_length * sizeof(JOCTET));
  if (icc_data == NULL)
    return FALSE;		/* oops, out of memory */

  /* and fill it in */
  for (marker = cinfo->marker_list; marker != NULL; marker = marker->next) {
    if (marker_is_icc(marker)) {
      JOCTET FAR *src_ptr;
      JOCTET *dst_ptr;
      unsigned int length;
      seq_no = GETJOCTET(marker->data[12]);
      dst_ptr = icc_data + data_offset[seq_no];
      src_ptr = marker->data + ICC_OVERHEAD_LEN;
      length = data_length[seq_no];
      while (length--) {
	*dst_ptr++ = *src_ptr++;
      }
    }
  }

  *icc_data_ptr = icc_data;
  *icc_data_len = total_length;

  return TRUE;
}




#endif

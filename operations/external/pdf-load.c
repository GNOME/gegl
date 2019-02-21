/* Video4Linux2 frame source op for GEGL
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
 * Copyright 2019 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_file_path (path, _("Path"), "")
     description (_("file to load"))

property_uri (uri, _("URI"), "")
     description (_("uri of file to load"))

property_int  (page,  _("Page"),  1)
     description (_("Page to render"))
     value_range (1, 10000)

property_int  (pages,  _("Pages"),  1)
     description (_("Total pages, provided as a visual read-only property"))
     value_range (1, 10000)

property_double (ppi,  _("PPI"),  200.0)
     description (_("Point/pixels per inch"))
     ui_range (72.0, 1000.0)
     value_range (10.0, 2400.0)

property_string (password,  _("Password"), "")
     description (_("Password to use for decryption of PDF, or blank for none"))

#else

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     pdf_load
#define GEGL_OP_C_SOURCE pdf-load.c

#include "gegl-op.h"
#include <poppler.h>

typedef struct
{
  char *path;
  char *uri;

  int   page_no;
  PopplerDocument *document;
  PopplerPage     *page;
  int width;
  int height;
  double scale;
  double ppi;
} Priv;

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result ={0,0,640,480};
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p= (Priv*)o->user_data;
  if (p)
  {
    result.width  = p->width;
    result.height = p->height;
  }
  return result;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv *p= (Priv*)o->user_data;
  if (p == NULL)
  {
    p = g_new0 (Priv, 1);
    o->user_data = (void*) p;
  }

  if (p->path == NULL || strcmp (p->path, o->path) ||
      p->uri  == NULL || strcmp (p->uri, o->uri))
  {
    const char *password = o->password[0]?o->password:NULL;

    if (p->path)
      g_free (p->path);
    if (p->uri)
      g_free (p->uri);
    if (p->document)
      g_object_unref (p->document);
    p->path = g_strdup (o->path);
    p->uri = g_strdup (o->uri);

    if (p->uri[0])
    {
      p->document = poppler_document_new_from_file (p->uri, password, NULL);
    }
    else
    {
      GFile *file = g_file_new_for_path (p->path);
      char *uri = g_file_get_uri (file);
      p->document = poppler_document_new_from_file (uri, password, NULL);
      g_free (uri);
      g_object_unref (file);
    }
    p->page = NULL;
    p->page_no = -1;
  }

  if (p->page_no != o->page-1 || p->ppi != o->ppi)
  {
    double width, height;
    p->scale = o->ppi / 72.0;
    p->ppi = o->ppi;
    p->page_no = o->page -1;
    if (p->page)
      g_object_unref (p->page);
    o->pages = poppler_document_get_n_pages (p->document);
    if (p->page_no >= 0 && p->page_no < o->pages)
      p->page = poppler_document_get_page (p->document, p->page_no);
    else
      p->page = NULL;

    if (p->page)
    {
      poppler_page_get_size (p->page, &width, &height);
      p->width = width * p->scale;
      p->height = height * p->scale;
    }
    else
    {
      p->width = 23 * p->scale;
      p->height = 42 * p->scale;
    }
  }

  gegl_operation_set_format (operation, "output",
                            babl_format ("R'G'B'A u8"));
}

static void
finalize (GObject *object)
{
 GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      Priv *p = (Priv*)o->user_data;
      if (p->page)
        g_object_unref (p->page);
      if (p->document)
        g_object_unref (p->document);
      if (p->path)
        g_free (p->path);
      g_clear_pointer (&o->user_data, g_free);
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Priv       *p = (Priv*)o->user_data;

  if (p->page)
  {
    cairo_surface_t   *surface;
    cairo_t           *cr;
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, p->width, p->height);
    cr = cairo_create (surface);
    cairo_set_source_rgb (cr, 1,1,1);
    cairo_paint (cr);
    cairo_scale (cr, p->scale, p->scale);

    if (p->page)
      poppler_page_render (p->page, cr);

    cairo_surface_flush (surface);

    gegl_buffer_set (output,
                     GEGL_RECTANGLE (0, 0, p->width, p->height),
                     0,
                     babl_format ("cairo-ARGB32"),
                     cairo_image_surface_get_data (surface),
                     cairo_image_surface_get_stride (surface));

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
  }
  return  TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  G_OBJECT_CLASS (klass)->finalize = finalize;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process              = process;
  operation_class->get_bounding_box  = get_bounding_box;
  operation_class->get_cached_region = get_cached_region;
  operation_class->prepare           = prepare;

  gegl_operation_handlers_register_loader(
    "application/pdf", "gegl:pdf-load");
  gegl_operation_handlers_register_loader(
    ".pdf", "gegl:pdf-load");

  gegl_operation_class_set_keys (operation_class,
    "name",         "gegl:pdf-load",
    "title",        _("pdf loader"),
    "categories",   "input",
    "description",  _("PDF page decoder"),
    NULL);
}

#endif

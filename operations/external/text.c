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
 * Copyright 2006,2018 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <cairo.h>
#include <gegl.h>
#include <pango/pango-attributes.h>
#include <pango/pangocairo.h>

#ifdef GEGL_PROPERTIES

property_string (string, _("Text"), "Hello")
    description(_("String to display (utf8)"))
    ui_meta    ("multiline", "true")

property_string (font, _("Font family"), "Sans")
    description(_("Font family (utf8)"))

property_double (size, _("Size"), 10.0)
    description (_("Font size in pixels."))
    value_range (0.0, 2048.0)
    ui_meta ("unit", "pixel-distance")

property_color  (color, _("Color"), "black")
    /* TRANSLATORS: the string 'black' should not be translated */
    description(_("Color for the text (defaults to 'black')"))

property_int  (wrap, _("Wrap width"), -1)
    description (_("Sets the width in pixels at which long lines will wrap. "
                     "Use -1 for no wrapping."))
    ui_meta ("unit", "pixel-distance")
    value_range (-1, 1000000)
property_int  (vertical_wrap, _("Wrap height"), -1)
    description (_("Sets the height in pixels according to which the text is "
                   "vertically justified. "
                   "Use -1 for no vertical justification."))
    ui_meta ("unit", "pixel-distance")
    value_range (-1, 1000000)

property_int    (alignment, _("Justification"), 0)
    value_range (0, 2)
    description (_("Alignment for multi-line text (0=Left, 1=Center, 2=Right)"))

property_int    (vertical_alignment, _("Vertical justification"), 0)
    value_range (0, 2)
    description (_("Vertical text alignment (0=Top, 1=Middle, 2=Bottom)"))

property_int (width, _("Width"), 1)
    description (_("Rendered width in pixels. (read only)"))
property_int    (height, _("Height"), 0)
    description (_("Rendered height in pixels. (read only)"))

#else

typedef struct {
  gchar         *string;
  gchar         *font;
  gdouble        size;
  gint           wrap;
  gint           vertical_wrap;
  gint           alignment;
  gint           vertical_alignment;
  GeglRectangle  defined;
} TextOpData;

#define GEGL_OP_SOURCE
#define GEGL_OP_NAME     text
#define GEGL_OP_C_SOURCE text.c
#include "gegl-op.h"

static void text_layout_text (GeglOperation *operation,
                              cairo_t       *cr,
                              gdouble        rowstride,
                              GeglRectangle *bounds,
                              int            component_set)
{
  GeglProperties       *props = GEGL_PROPERTIES (operation);
  PangoFontDescription *desc;
  PangoLayout          *layout;
  PangoAttrList        *attrs;
  guint16               color[4];
  gchar                *string;
  gint                  alignment = 0;
  PangoRectangle        ink_rect;
  PangoRectangle        logical_rect;
  gint                  vertical_offset = 0;

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  string = g_strcompress (props->string);
  pango_layout_set_text (layout, string, -1);
  g_free (string);

  desc = pango_font_description_from_string (props->font);
  pango_font_description_set_absolute_size (desc, props->size * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);

  switch (props->alignment)
  {
  case 0:
    alignment = PANGO_ALIGN_LEFT;
    break;
  case 1:
    alignment = PANGO_ALIGN_CENTER;
    break;
  case 2:
    alignment = PANGO_ALIGN_RIGHT;
    break;
  }
  pango_layout_set_alignment (layout, alignment);
  pango_layout_set_width (layout, props->wrap * PANGO_SCALE);

  attrs = pango_attr_list_new ();

  switch (component_set)
  {
    case 0:
      gegl_color_get_pixel (props->color, babl_format ("R'G'B'A u16"), color);
      break;
    case 1:
      gegl_color_get_pixel (props->color, babl_format ("cykA u16"), color);
      break;
    case 2:
      gegl_color_get_pixel (props->color, babl_format ("cmkA u16"), color);
      break;
  }


  pango_attr_list_insert (
    attrs,
    pango_attr_foreground_new (color[0], color[1], color[2]));
  pango_attr_list_insert (
    attrs,
    pango_attr_foreground_alpha_new (color[3]));

  pango_layout_set_attributes (layout, attrs);

  /* Inform Pango to re-layout the text with the new transformation */
  pango_cairo_update_layout (cr, layout);

  pango_layout_get_pixel_extents (layout, &ink_rect, &logical_rect);
  if (props->vertical_wrap >= 0)
    {
      switch (props->vertical_alignment)
      {
      case 0: /* top */
        vertical_offset = 0;
        break;
      case 1: /* middle */
        vertical_offset = (props->vertical_wrap - logical_rect.height) / 2;
        break;
      case 2: /* bottom */
        vertical_offset = props->vertical_wrap - logical_rect.height;
        break;
      }
    }

  if (bounds)
    {
      *bounds = *GEGL_RECTANGLE (ink_rect.x,     ink_rect.y + vertical_offset,
                                 ink_rect.width, ink_rect.height);
    }
  else
    {
      /* When alpha is 0, Pango goes full alpha (by design).  Go figure... */
      if (color[3] > 0)
        {
          cairo_translate (cr, 0, vertical_offset);

          pango_cairo_show_layout (cr, layout);
        }
    }

  pango_font_description_free (desc);
  pango_attr_list_unref (attrs);
  g_object_unref (layout);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  const Babl *format     =  gegl_operation_get_format (operation, "output");
  const Babl *formats[4] = {NULL, NULL, NULL, NULL};
  int is_cmyk            = babl_get_model_flags (format) & BABL_MODEL_FLAG_CMYK ? 1 : 0;

  cairo_t         *cr;
  cairo_surface_t *surface;
  if (is_cmyk)
  {
    formats[0]=babl_format ("cairo-ACYK32");
    formats[1]=babl_format ("cairo-ACMK32");
  }
  else
  {
    formats[0]=babl_format ("cairo-ARGB32");
  }

  for (int i = 0; formats[i]; i++)
  {
    guchar *data;
    data  = g_new0 (guchar, result->width * result->height * 4);

    surface = cairo_image_surface_create_for_data (data,
                                                 CAIRO_FORMAT_ARGB32,
                                                 result->width,
                                                 result->height,
                                                 result->width * 4);
    cr = cairo_create (surface);
    cairo_translate (cr, -result->x, -result->y);
    text_layout_text (operation, cr, 0, NULL, i+is_cmyk);

    gegl_buffer_set (output, result, 0, formats[i], data,
                     GEGL_AUTO_ROWSTRIDE);

    cairo_destroy (cr);
    cairo_surface_destroy (surface);
    g_free (data);
  }

  return  TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties *props  = GEGL_PROPERTIES (operation);
  TextOpData     *data   = (TextOpData *) props->user_data;
  gint            status = FALSE;

  if ((data->string && strcmp (data->string, props->string)) ||
      (data->font && strcmp (data->font, props->font)) ||
      data->size != props->size ||
      data->wrap != props->wrap ||
      data->vertical_wrap != props->vertical_wrap ||
      data->alignment != props->alignment ||
      data->vertical_alignment != props->vertical_alignment)
    { /* get extents */
      cairo_t         *cr      = NULL;
      cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1, 1);

      cr = cairo_create (surface);
      text_layout_text (operation, cr, 0, &data->defined, 0);
      cairo_destroy (cr);
      cairo_surface_destroy (surface);

      g_free (data->string);
      data->string = g_strdup (props->string);
      g_free (data->font);
      data->font = g_strdup (props->font);
      data->size = props->size;
      data->wrap = props->wrap;
      data->vertical_wrap = props->vertical_wrap;
      data->vertical_alignment = props->vertical_alignment;

      /* store the measured size for later use */
      props->width  = data->defined.width  - data->defined.x;
      props->height = data->defined.height - data->defined.y;
    }

  if (status)
    {
      g_warning ("get defined region for text '%s' failed", props->string);
    }

  return data->defined;
}

static void
finalize (GObject *object)
{
  TextOpData *data = (TextOpData *) GEGL_PROPERTIES (object)->user_data;

  if (data != NULL)
    {
      g_free (data->string);
      g_free (data->font);
    }
  g_free (data);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *props        = GEGL_PROPERTIES (operation);
  const Babl     *color_format = gegl_color_get_format (props->color);
  BablModelFlag   model_flags  = babl_get_model_flags (color_format);

  if (model_flags & BABL_MODEL_FLAG_CMYK)
  {
    gegl_operation_set_format (operation, "output",
                               babl_format ("camayakaA u8"));
  }
  else
  {
    gegl_operation_set_format (operation, "output",
                               babl_format ("RaGaBaA float"));
  }

  if (props->user_data == NULL)
    props->user_data = g_new0 (TextOpData, 1);
}

static const gchar *composition =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<gegl>"
    "<node operation='gegl:crop' width='200' height='200'/>"
    "<node operation='gegl:text'>"
    "  <params>"
    "    <param name='size'>20</param>"
    "    <param name='wrap'>200</param>"
    "    <param name='color'>green</param>"
    "    <param name='string'>loves or pursues or desires to obtain pain of itself, because it is pain, but occasionally circumstances occur in which toil and pain can procure him some great pleasure. To take a trivial example, which of us ever undertakes laborious physical exercise, except to obtain some advantage from it? But who has any right to find fault with a man who chooses to enjoy a pleasure that has no annoying consequences, or one who avoids a pain that produces no</param>"
    "  </params>"
    "</node>"
    "</gegl>";

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass             *object_class           = G_OBJECT_CLASS (klass);
  GeglOperationClass       *operation_class        = GEGL_OPERATION_CLASS (klass);
  GeglOperationSourceClass *operation_source_class = GEGL_OPERATION_SOURCE_CLASS (klass);

  object_class->finalize = finalize;

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  operation_source_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "reference-composition", composition,
    "title",          _("Render Text"),
    "name",           "gegl:text",
    "categories",     "render",
    "reference-hash", "5cb6c205c2846d2d585b484e96c38b6e",
    "description",  _("Display a string of text using pango and cairo."),
    NULL);

}


#endif

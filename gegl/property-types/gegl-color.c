/* This file is part of GEGL
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
 * Copyright 2006 Martin Nordholts <enselic@hotmail.com>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-color.h"


enum
{
  PROP_0,
  PROP_STRING
};

typedef struct _ColorNameEntity ColorNameEntity;

struct _GeglColorPrivate
{
  const Babl *format;

  union
  {
    /* Some babl code (SSE2 codepath in particular) requires source address to
     * be 16-byte aligned and would crash otherwise.
     * See: https://gitlab.gnome.org/GNOME/gegl/-/merge_requests/142
     */
    guint8  pixel[48] __attribute__((aligned(16)));
    gdouble alignment;
  };
};

struct _ColorNameEntity
{
  const gchar *color_name;
  const gfloat rgba_color[4];
};

static gboolean  parse_float_argument_list (float rgba_color[4],
                                            GScanner  *scanner,
                                            gint       num_arguments);
static gboolean  parse_color_name (float rgba_color[4],
                                   const gchar *color_string);
static gboolean  parse_hex (float rgba_color[4],
                            const gchar *color_string);
static void      set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec);
static void      get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec);

/* These color names are based on those defined in the HTML 4.01 standard. See
 * http://www.w3.org/TR/html4/types.html#h-6.5
 *
 * Note: these values are stored with gamma
 */
static const ColorNameEntity color_names[] =
{
  { "black",   { 0.f,      0.f,      0.f,      1.f } },
  { "silver",  { 0.75294f, 0.75294f, 0.75294f, 1.f } },
  { "gray",    { 0.50196f, 0.50196f, 0.50196f, 1.f } },
  { "white",   { 1.f,      1.f,      1.f,      1.f } },
  { "maroon",  { 0.50196f, 0.f,      0.f,      1.f } },
  { "red",     { 1.f,      0.f,      0.f,      1.f } },
  { "purple",  { 0.50196f, 0.f,      0.50196f, 1.f } },
  { "fuchsia", { 1.f,      0.f,      1.f,      1.f } },
  { "green",   { 0.f,      0.50196f, 0.f,      1.f } },
  { "lime",    { 0.f,      1.f,      0.f,      1.f } },
  { "olive",   { 0.50196f, 0.50196f, 0.f,      1.f } },
  { "yellow",  { 1.f,      1.f,      0.f,      1.f } },
  { "navy",    { 0.f,      0.f,      0.50196f, 1.f } },
  { "blue",    { 0.f,      0.f,      1.f,      1.f } },
  { "teal",    { 0.f,      0.50196f, 0.50196f, 1.f } },
  { "aqua",    { 0.f,      1.f,      1.f,      1.f } },
  { "none",    { 0.f,      0.f,      0.f,      0.f } },
  { "transparent",  { 0.f, 0.f,      0.f,      0.f } }
};

/* Copied into GeglColor:s instances when parsing a color from a string fails. */
static const gfloat parsing_error_color[4] = { 0.f, 1.f, 1.f, 0.67f };

/* Copied into all GeglColor:s at their instantiation. */
static const gfloat init_color[4] = { 1.f, 1.f, 1.f, 1.f };

G_DEFINE_TYPE_WITH_PRIVATE (GeglColor, gegl_color, G_TYPE_OBJECT)


static void
gegl_color_init (GeglColor *self)
{
  self->priv = gegl_color_get_instance_private ((self));

  self->priv->format = gegl_babl_rgba_linear_float ();

  memcpy (self->priv->pixel, init_color, sizeof (init_color));
}

static void
gegl_color_class_init (GeglColorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_STRING,
                                   g_param_spec_string ("string",
                                                        "String",
                                                        "A String representation of the GeglColor",
                                                        "",
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));
}

static gboolean
parse_float_argument_list (float rgba_color[4],
                           GScanner  *scanner,
                           gint       num_arguments)
{
  GTokenType        token_type;
  GTokenValue       token_value;
  gint              i;

  /* Make sure there is a leading '(' */
  if (g_scanner_get_next_token (scanner) != G_TOKEN_LEFT_PAREN)
    {
      return FALSE;
    }

  /* Iterate through the arguments and copy each value
   * to the rgba_color array of GeglColor.
   */
  for (i = 0; i < num_arguments; ++i)
    {
      switch (g_scanner_get_next_token (scanner))
        {
          case G_TOKEN_FLOAT:
            token_value = g_scanner_cur_value (scanner);
            rgba_color[i] = token_value.v_float;
            break;

          case G_TOKEN_INT:
            token_value = g_scanner_cur_value (scanner);
            rgba_color[i] = token_value.v_int64;
            break;

          default:
            return FALSE;
        }

      /* Verify that there is a ',' after each float, except the last one */
      if (i < (num_arguments - 1))
        {
          token_type = g_scanner_get_next_token (scanner);
          if (token_type != G_TOKEN_COMMA)
            {
              return FALSE;
            }
        }
    }

  /* Make sure there is a trailing ')' and that that is the last token. */
  if (g_scanner_get_next_token (scanner) == G_TOKEN_RIGHT_PAREN &&
      g_scanner_get_next_token (scanner) == G_TOKEN_EOF)
    {
      return TRUE;
    }

  return FALSE;
}

static gboolean
parse_color_name (float rgba_color[4],
                  const gchar *color_string)
{
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (color_names); ++i)
    {
      if (g_ascii_strcasecmp (color_names[i].color_name, color_string) == 0)
        {
          memcpy (rgba_color, color_names[i].rgba_color, sizeof (color_names[i].rgba_color));
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
parse_hex (float rgba_color[4],
           const gchar *color_string)
{
  gint              i;
  gsize             string_length = strlen (color_string);

  if (string_length == 7 ||  /* #rrggbb   */
      string_length == 9)    /* #rrggbbaa */
    {
      gint num_iterations = (string_length - 1) / 2;
      for (i = 0; i < num_iterations; ++i)
        {
          if (g_ascii_isxdigit (color_string[2 * i + 1]) &&
              g_ascii_isxdigit (color_string[2 * i + 2]))
            {
              rgba_color[i] = (g_ascii_xdigit_value (color_string[2 * i + 1]) << 4 |
                                     g_ascii_xdigit_value (color_string[2 * i + 2])) / 255.f;
            }
          else
            {
              return FALSE;
            }
        }

      /* Successful #rrggbb(aa) parsing! */
      return TRUE;
    }
  else if (string_length == 4 ||  /* #rgb  */
           string_length == 5)    /* #rgba */
    {
      gint num_iterations = string_length - 1;
      for (i = 0; i < num_iterations; ++i)
        {
          if (g_ascii_isxdigit (color_string[i + 1]))
            {
              rgba_color[i] = (g_ascii_xdigit_value (color_string[i + 1]) << 4 |
                                     g_ascii_xdigit_value (color_string[i + 1])) / 255.f;
            }
          else
            {
              return FALSE;
            }
        }

      /* Successful #rgb(a) parsing! */
      return TRUE;
    }

  /* String was of unsupported length. */
  return FALSE;
}

#if 0
const gfloat *
gegl_color_float4 (GeglColor *self)
{
  g_return_val_if_fail (GEGL_IS_COLOR (self), NULL);
  return &self->rgba_color[0];
}
#endif

void
gegl_color_set_pixel (GeglColor   *color,
                      const Babl  *format,
                      const void  *pixel)
{
  gint bpp;

  g_return_if_fail (GEGL_IS_COLOR (color));
  g_return_if_fail (format);
  g_return_if_fail (pixel);

  bpp = babl_format_get_bytes_per_pixel (format);

  if (bpp <= sizeof (color->priv->pixel))
    color->priv->format = format;
  else
    color->priv->format = gegl_babl_rgba_linear_float ();

  babl_process (babl_fish (format, color->priv->format),
                pixel, color->priv->pixel, 1);
}

const Babl *
gegl_color_get_format (GeglColor *color)
{
  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);
  return color->priv->format;
}

void
gegl_color_get_pixel (GeglColor   *color,
                      const Babl  *format,
                      void        *pixel)
{
  g_return_if_fail (GEGL_IS_COLOR (color));
  g_return_if_fail (format);
  g_return_if_fail (pixel);

  babl_process (babl_fish (color->priv->format, format),
                color->priv->pixel, pixel, 1);
}

void
gegl_color_set_bytes (GeglColor  *color,
                      const Babl *format,
                      GBytes     *bytes)
{
  gint bpp;

  g_return_if_fail (GEGL_IS_COLOR (color));
  g_return_if_fail (format);
  g_return_if_fail (bytes);

  bpp = babl_format_get_bytes_per_pixel (format);
  g_return_if_fail (g_bytes_get_size (bytes) == bpp);

  if (bpp <= sizeof (color->priv->pixel))
    color->priv->format = format;
  else
    color->priv->format = gegl_babl_rgba_linear_float ();

  babl_process (babl_fish (format, color->priv->format),
                g_bytes_get_data (bytes, NULL), color->priv->pixel, 1);
}

GBytes *
gegl_color_get_bytes (GeglColor  *color,
                      const Babl *format)
{
  guint8 *data;
  gint    bpp;

  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);
  g_return_val_if_fail (format, NULL);

  bpp  = babl_format_get_bytes_per_pixel (format);
  data = g_malloc0 (bpp);

  babl_process (babl_fish (color->priv->format, format),
                color->priv->pixel, data, 1);

  return g_bytes_new_take (data, bpp);
}

void
gegl_color_set_rgba (GeglColor *self,
                     gdouble    r,
                     gdouble    g,
                     gdouble    b,
                     gdouble    a)
{
  const gfloat rgba[4] = {r, g, b, a};

  g_return_if_fail (GEGL_IS_COLOR (self));

  gegl_color_set_pixel (self, gegl_babl_rgba_linear_float (), rgba);
}

void
gegl_color_get_rgba (GeglColor *self,
                     gdouble   *r,
                     gdouble   *g,
                     gdouble   *b,
                     gdouble   *a)
{
  gfloat rgba[4];

  g_return_if_fail (GEGL_IS_COLOR (self));

  gegl_color_get_pixel (self, gegl_babl_rgba_linear_float (), rgba);

  if (r) *r = rgba[0];
  if (g) *g = rgba[1];
  if (b) *b = rgba[2];
  if (a) *a = rgba[3];
}

void
gegl_color_set_rgba_with_space (GeglColor  *self,
                                gdouble     r,
                                gdouble     g,
                                gdouble     b,
                                gdouble     a,
                                const Babl *space)
{
  const Babl   *format  = babl_format_with_space ("R'G'B'A float", space);
  const gfloat  rgba[4] = {r, g, b, a};

  space = babl_format_get_space (format);

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_space_is_rgb (space));

  gegl_color_set_pixel (self, format, rgba);
}

void
gegl_color_get_rgba_with_space (GeglColor  *self,
                                gdouble    *r,
                                gdouble    *g,
                                gdouble    *b,
                                gdouble    *a,
                                const Babl *space)
{
  const Babl *format  = babl_format_with_space ("R'G'B'A float", space);
  gfloat      rgba[4];

  space = babl_format_get_space (format);

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_space_is_rgb (space));

  gegl_color_get_pixel (self, format, rgba);

  if (r) *r = rgba[0];
  if (g) *g = rgba[1];
  if (b) *b = rgba[2];
  if (a) *a = rgba[3];
}

void
gegl_color_set_cmyk (GeglColor  *self,
                     gdouble     c,
                     gdouble     m,
                     gdouble     y,
                     gdouble     k,
                     gdouble     a,
                     const Babl *space)
{
  const Babl   *format  = babl_format_with_space ("CMYK float", space);
  const gfloat  cmyk[5] = {c, m, y, k, a};

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_format_get_space (format));

  gegl_color_set_pixel (self, format, cmyk);
}

void
gegl_color_get_cmyk (GeglColor  *self,
                     gdouble    *c,
                     gdouble    *m,
                     gdouble    *y,
                     gdouble    *k,
                     gdouble    *a,
                     const Babl *space)
{
  const Babl *format  = babl_format_with_space ("CMYK float", space);
  gfloat      cmyk[5];

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_space_is_cmyk (babl_format_get_space (format)));

  gegl_color_get_pixel (self, format, cmyk);

  if (c) *c = cmyk[0];
  if (m) *m = cmyk[1];
  if (y) *y = cmyk[2];
  if (k) *k = cmyk[2];
  if (a) *a = cmyk[3];
}

void
gegl_color_set_hsva (GeglColor  *self,
                     gdouble     h,
                     gdouble     s,
                     gdouble     v,
                     gdouble     a,
                     const Babl *space)
{
  const Babl   *format  = babl_format_with_space ("HSVA float", space);
  const gfloat  hsva[4] = {h, s, v, a};

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_format_get_space (format));

  gegl_color_set_pixel (self, format, hsva);
}

void
gegl_color_get_hsva (GeglColor  *self,
                     gdouble    *h,
                     gdouble    *s,
                     gdouble    *v,
                     gdouble    *a,
                     const Babl *space)
{
  const Babl *format  = babl_format_with_space ("HSVA float", space);
  gfloat      hsva[4];

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_space_is_rgb (babl_format_get_space (format)));

  gegl_color_get_pixel (self, format, hsva);

  if (h) *h = hsva[0];
  if (s) *s = hsva[1];
  if (v) *v = hsva[2];
  if (a) *a = hsva[3];
}

void
gegl_color_set_hsla (GeglColor  *self,
                     gdouble     h,
                     gdouble     s,
                     gdouble     l,
                     gdouble     a,
                     const Babl *space)
{
  const Babl   *format  = babl_format_with_space ("HSLA float", space);
  const gfloat  hsla[4] = {h, s, l, a};

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_format_get_space (format));

  gegl_color_set_pixel (self, format, hsla);
}

void
gegl_color_get_hsla (GeglColor  *self,
                     gdouble    *h,
                     gdouble    *s,
                     gdouble    *l,
                     gdouble    *a,
                     const Babl *space)
{
  const Babl *format  = babl_format_with_space ("HSLA float", space);
  gfloat      hsla[4];

  g_return_if_fail (GEGL_IS_COLOR (self));
  g_return_if_fail (space == NULL || babl_space_is_rgb (babl_format_get_space (format)));

  gegl_color_get_pixel (self, format, hsla);

  if (h) *h = hsla[0];
  if (s) *s = hsla[1];
  if (l) *l = hsla[2];
  if (a) *a = hsla[3];
}

static void
gegl_color_set_from_string (GeglColor   *self,
                            const gchar *color_string)
{
  GScanner         *scanner;
  GTokenType        token_type;
  GTokenValue       token_value;
  gboolean          color_parsing_successfull;
  float cmyka[5] = {0.0, 0.0, 0.0, 1.0, 1.0};
  const Babl *format = gegl_babl_rgba_float ();
  float *rgba=&cmyka[0];

  scanner                               = g_scanner_new (NULL);
  scanner->config->cpair_comment_single = "";
  g_scanner_input_text (scanner, color_string, strlen (color_string));

  token_type  = g_scanner_get_next_token (scanner);
  token_value = g_scanner_cur_value (scanner);

  if (token_type == G_TOKEN_IDENTIFIER &&
      g_ascii_strcasecmp (token_value.v_identifier, "cmyk") == 0)
    {
      color_parsing_successfull = parse_float_argument_list (cmyka, scanner, 4);
      for (int i = 0; i<4;i++)
        cmyka[i] /= 100.0;

      format = babl_format ("CMYK float");
    }
  else if (token_type == G_TOKEN_IDENTIFIER &&
      g_ascii_strcasecmp (token_value.v_identifier, "cmyka") == 0)
    {
      color_parsing_successfull = parse_float_argument_list (cmyka, scanner, 5);
      for (int i = 0; i<4;i++)
        cmyka[i] /= 100.0;
      format = babl_format ("CMYKA float");
    }
  else if (token_type == G_TOKEN_IDENTIFIER &&
      g_ascii_strcasecmp (token_value.v_identifier, "rgb") == 0)
    {
      color_parsing_successfull = parse_float_argument_list (rgba, scanner, 3);
      format = gegl_babl_rgba_linear_float ();
    }
  else if (token_type == G_TOKEN_IDENTIFIER &&
           g_ascii_strcasecmp (token_value.v_identifier, "rgba") == 0)
    {
      rgba[3] = 1.0;
      color_parsing_successfull = parse_float_argument_list (rgba, scanner, 4);
      format = gegl_babl_rgba_linear_float ();
    }
  else if (token_type == '#')
    {
      color_parsing_successfull = parse_hex (rgba, color_string);
    }
  else if (token_type == G_TOKEN_IDENTIFIER)
    {
      color_parsing_successfull = parse_color_name (rgba, color_string);
    }
  else
    {
      color_parsing_successfull = FALSE;
    }

  if (color_parsing_successfull)
    {
      gegl_color_set_pixel(self, format, cmyka);
    }
  else
    {
      gegl_color_set_pixel(self,
                           gegl_babl_rgba_linear_float (),
                           parsing_error_color);

      g_warning ("Parsing of color string \"%s\" into GeglColor failed! "
                 "Using transparent cyan instead",
                 color_string);
    }

  g_scanner_destroy (scanner);
}

static gchar *
gegl_color_get_string (GeglColor *color)
{
  gfloat rgba[4];

  gegl_color_get_pixel (color, gegl_babl_rgba_linear_float (), rgba);

  if (babl_get_model_flags (color->priv->format) & BABL_MODEL_FLAG_CMYK)
  {
    gfloat cmyka[5];
    gchar buf [5][G_ASCII_DTOSTR_BUF_SIZE];
    gegl_color_get_pixel (color, babl_format ("CMYKA float"), cmyka);
    g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%1.1f", cmyka[0]*100);
    g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%1.1f", cmyka[1]*100);
    g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%1.1f", cmyka[2]*100);
    g_ascii_formatd (buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%1.1f", cmyka[3]*100);
    g_ascii_formatd (buf[4], G_ASCII_DTOSTR_BUF_SIZE, "%1.1f", cmyka[3]);
    if (cmyka[4] == 1.0)
    {
      return g_strdup_printf ("cmyk(%s, %s, %s, %s)", buf[0], buf[1], buf[2], buf[3]);
    }
    else
    {
      return g_strdup_printf ("cmyka(%s, %s, %s, %s, %s)", buf[0], buf[1], buf[2], buf[3], buf[4]);
    }
  }

  if (rgba[3] == 1.0)
    {
      gchar buf [3][G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[0]);
      g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[1]);
      g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[2]);
      return g_strdup_printf ("rgb(%s, %s, %s)", buf[0], buf[1], buf[2]);
    }
  else
    {
      gchar buf [4][G_ASCII_DTOSTR_BUF_SIZE];
      g_ascii_formatd (buf[0], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[0]);
      g_ascii_formatd (buf[1], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[1]);
      g_ascii_formatd (buf[2], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[2]);
      g_ascii_formatd (buf[3], G_ASCII_DTOSTR_BUF_SIZE, "%1.3f", rgba[3]);
      return g_strdup_printf ("rgba(%s, %s, %s, %s)", buf[0], buf[1], buf[2], buf[3]);
    }
}


static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglColor *color = GEGL_COLOR (gobject);

  switch (property_id)
    {
      case PROP_STRING:
        gegl_color_set_from_string (color, g_value_get_string (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglColor *color = GEGL_COLOR (gobject);

  switch (property_id)
    {
      case PROP_STRING:
      {
        gchar *string = gegl_color_get_string (color);
        g_value_set_string (value, string);
        g_free (string);
      }
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

GeglColor *
gegl_color_new (const gchar *string)
{
  if (string)
    return g_object_new (GEGL_TYPE_COLOR, "string", string, NULL);

  return g_object_new (GEGL_TYPE_COLOR, NULL);
}

GeglColor *
gegl_color_duplicate (GeglColor *color)
{
  GeglColor *new;

  g_return_val_if_fail (GEGL_IS_COLOR (color), NULL);

  new = g_object_new (GEGL_TYPE_COLOR, NULL);

  memcpy (new->priv, color->priv, sizeof (GeglColorPrivate));

  return new;
}

/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglColor as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_COLOR, GeglParamColor))
#define GEGL_IS_PARAM_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_COLOR))

typedef struct _GeglParamColor GeglParamColor;

struct _GeglParamColor
{
  GParamSpec parent_instance;

  GeglColor *default_color;
};

static void
gegl_param_color_init (GParamSpec *self)
{
  GEGL_PARAM_COLOR (self)->default_color = NULL;
}

GeglColor *
gegl_param_spec_color_get_default (GParamSpec *self)
{
  return GEGL_PARAM_COLOR (self)->default_color;
}

static void
gegl_param_color_finalize (GParamSpec *self)
{
  GeglParamColor  *param_color  = GEGL_PARAM_COLOR (self);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (GEGL_TYPE_PARAM_COLOR));

  g_clear_object (&param_color->default_color);

  parent_class->finalize (self);
}

static void
gegl_param_color_set_default (GParamSpec *param_spec,
                              GValue     *value)
{
  GeglParamColor *gegl_color = GEGL_PARAM_COLOR (param_spec);

  if (gegl_color->default_color)
    g_value_take_object (value, gegl_color_duplicate (gegl_color->default_color));
}

static gint
gegl_param_color_cmp (GParamSpec   *param_spec,
                      const GValue *value1,
                      const GValue *value2)
{
  GeglColor *color1 = g_value_get_object (value1);
  GeglColor *color2 = g_value_get_object (value2);

  if (! color1 || ! color2)
    return color2 ? -1 : (color1 ? 1 : 0);

  if (color1->priv->format != color2->priv->format)
    return 1;
  else
    return memcmp (color1->priv->pixel, color2->priv->pixel,
                   babl_format_get_bytes_per_pixel (color1->priv->format));
}

GType
gegl_param_color_get_type (void)
{
  static GType param_color_type = 0;

  if (G_UNLIKELY (param_color_type == 0))
    {
      static GParamSpecTypeInfo param_color_type_info = {
        sizeof (GeglParamColor),
        0,
        gegl_param_color_init,
        0,
        gegl_param_color_finalize,
        gegl_param_color_set_default,
        NULL,
        gegl_param_color_cmp
      };
      param_color_type_info.value_type = GEGL_TYPE_COLOR;

      param_color_type = g_param_type_register_static ("GeglParamColor",
                                                       &param_color_type_info);
    }

  return param_color_type;
}

GParamSpec *
gegl_param_spec_color (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GeglColor   *default_color,
                       GParamFlags  flags)
{
  GeglParamColor *param_color;

  param_color = g_param_spec_internal (GEGL_TYPE_PARAM_COLOR,
                                       name, nick, blurb, flags);

  param_color->default_color = default_color;
  if (default_color)
    g_object_ref (default_color);

  return G_PARAM_SPEC (param_color);
}

GParamSpec *
gegl_param_spec_color_from_string (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   const gchar *default_color_string,
                                   GParamFlags  flags)
{
  GeglParamColor *param_color;

  param_color = g_param_spec_internal (GEGL_TYPE_PARAM_COLOR,
                                       name, nick, blurb, flags);

  param_color->default_color = g_object_new (GEGL_TYPE_COLOR,
                                             "string", default_color_string,
                                             NULL);

  return G_PARAM_SPEC (param_color);
}

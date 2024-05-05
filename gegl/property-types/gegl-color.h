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

#ifndef __GEGL_COLOR_H__
#define __GEGL_COLOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_TYPE_COLOR            (gegl_color_get_type ())
#define GEGL_COLOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_COLOR, GeglColor))
#define GEGL_COLOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_COLOR, GeglColorClass))
#define GEGL_IS_COLOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_COLOR))
#define GEGL_IS_COLOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_COLOR))
#define GEGL_COLOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_COLOR, GeglColorClass))

typedef struct _GeglColorClass   GeglColorClass;
typedef struct _GeglColorPrivate GeglColorPrivate;

struct _GeglColor
{
  GObject           parent_instance;
  GeglColorPrivate *priv;
};

struct _GeglColorClass
{
  GObjectClass parent_class;
};

GType        gegl_color_get_type               (void) G_GNUC_CONST;


/***
 * GeglColor:
 *
 * GeglColor is an object containing a color at the moment only RGB colors
 * are supported, in the future a GeglColor might also indicate other
 * enumerated or natively in other color representations colors.
 *
 * GeglColor accepts a subset of format string as defined by the CSS color specification:
 *  http://dev.w3.org/csswg/css-color/
 *
 * - RGB hexadecimal notation: #rrggbb[aa] / #rgb[a]
 * - Named colors, limited to the 16 specified in HTML4
 *
 * To specify linear-light floating-point RGB, use: rgb[a](0.40, 0.44, 0.92 [, a])
 * The normal bounds are [0.0 1.0], unlike CSS which is [0 255]. Out-of-bounds values are allowed.
 */

/**
 * gegl_color_new:
 * @string: a string describing the color to be created.
 *
 * Creates a new #GeglColor.
 *
 * Returns the newly created #GeglColor.
 */
GeglColor *  gegl_color_new                    (const gchar *string);

/**
 * gegl_color_duplicate:
 * @color: the color to duplicate.
 *
 * Creates a copy of @color.
 *
 * Return value: (transfer full): A new copy of @color.
 */
GeglColor *  gegl_color_duplicate              (GeglColor   *color);

/**
 * gegl_color_get_rgba:
 * @color: a #GeglColor
 * @red: (out): red return location.
 * @green: (out): green return location.
 * @blue: (out): blue return location.
 * @alpha: (out): alpha return location.
 *
 * Retrieves the current set color as linear light non premultipled RGBA data,
 * any of the return pointers can be omitted.
 */
void         gegl_color_get_rgba               (GeglColor   *color,
                                                gdouble     *red,
                                                gdouble     *green,
                                                gdouble     *blue,
                                                gdouble     *alpha);

/**
 * gegl_color_set_rgba:
 * @color: a #GeglColor
 * @red: red value
 * @green: green value
 * @blue: blue value
 * @alpha: alpha value
 *
 * Set color as linear light non premultipled RGBA data
 */
void         gegl_color_set_rgba               (GeglColor   *color,
                                                gdouble      red,
                                                gdouble      green,
                                                gdouble      blue,
                                                gdouble      alpha);

/**
 * gegl_color_get_rgba_with_space:
 * @color: a #GeglColor
 * @red: (out): red return location.
 * @green: (out): green return location.
 * @blue: (out): blue return location.
 * @alpha: (out): alpha return location.
 * @space: RGB space.
 *
 * Retrieves the current set color stored as @space.
 * If @space is %NULL, this is equivalent to requesting color in sRGB.
 */
void         gegl_color_get_rgba_with_space    (GeglColor   *color,
                                                gdouble     *red,
                                                gdouble     *green,
                                                gdouble     *blue,
                                                gdouble     *alpha,
                                                const Babl  *space);

/**
 * gegl_color_set_rgba_with_space:
 * @color: a #GeglColor
 * @red: red value
 * @green: green value
 * @blue: blue value
 * @alpha: alpha value
 * @space: RGB space.
 *
 * Set color as RGBA data stored as @space. If @space is %NULL, this is
 * equivalent to storing as sRGB.
 */
void         gegl_color_set_rgba_with_space    (GeglColor   *color,
                                                gdouble      red,
                                                gdouble      green,
                                                gdouble      blue,
                                                gdouble      alpha,
                                                const Babl  *space);

/**
 * gegl_color_get_cmyk:
 * @color: a #GeglColor
 * @cyan: (out): cyan return location.
 * @magenta: (out): magenta return location.
 * @yellow: (out): yellow return location.
 * @key: (out): key return location.
 * @alpha: (out): alpha return location.
 * @space: (nullable): CMYK space.
 *
 * Retrieves the current set color stored as @space.
 * If @space is %NULL, this is equivalent to requesting color in the default
 * naive CMYK space.
 */
void         gegl_color_get_cmyk               (GeglColor   *color,
                                                gdouble     *cyan,
                                                gdouble     *magenta,
                                                gdouble     *yellow,
                                                gdouble     *key,
                                                gdouble     *alpha,
                                                const Babl  *space);

/**
 * gegl_color_set_cmyk:
 * @color: a #GeglColor
 * @cyan: cyan value
 * @magenta: magenta value
 * @yellow: yellow value
 * @key: key value
 * @alpha: alpha value
 * @space: (nullable): CMYK space.
 *
 * Set color as CMYK data stored as @space. If @space is %NULL, this is
 * equivalent to storing with the default naive CMYK space.
 */
void         gegl_color_set_cmyk               (GeglColor   *color,
                                                gdouble      cyan,
                                                gdouble      magenta,
                                                gdouble      yellow,
                                                gdouble      key,
                                                gdouble      alpha,
                                                const Babl  *space);

/**
 * gegl_color_get_hsva:
 * @color: a #GeglColor
 * @hue: (out): hue return location.
 * @saturation: (out): saturation return location.
 * @value: (out): value return location.
 * @alpha: (out): alpha return location.
 * @space: (nullable): RGB space.
 *
 * Retrieves the current set color stored as @space.
 * If @space is %NULL, this is equivalent to requesting color in the default
 * sRGB space.
 */
void         gegl_color_get_hsva               (GeglColor   *color,
                                                gdouble     *hue,
                                                gdouble     *saturation,
                                                gdouble     *value,
                                                gdouble     *alpha,
                                                const Babl  *space);

/**
 * gegl_color_set_hsva:
 * @color: a #GeglColor
 * @hue: hue value.
 * @saturation: saturation value.
 * @value: value value.
 * @alpha: alpha value.
 * @space: (nullable): RGB space.
 *
 * Set color as HSVA data stored as @space. If @space is %NULL, this is
 * equivalent to storing with the default sRGB space.
 */
void         gegl_color_set_hsva               (GeglColor   *color,
                                                gdouble      hue,
                                                gdouble      saturation,
                                                gdouble      value,
                                                gdouble      alpha,
                                                const Babl  *space);

/**
 * gegl_color_get_hsla:
 * @color: a #GeglColor
 * @hue: (out): hue return location.
 * @saturation: (out): saturation return location.
 * @lightness: (out): value return location.
 * @alpha: (out): alpha return location.
 * @space: (nullable): RGB space.
 *
 * Retrieves the current set color stored as @space.
 * If @space is %NULL, this is equivalent to requesting color in the default
 * sRGB space.
 */
void         gegl_color_get_hsla               (GeglColor   *color,
                                                gdouble     *hue,
                                                gdouble     *saturation,
                                                gdouble     *lightness,
                                                gdouble     *alpha,
                                                const Babl  *space);

/**
 * gegl_color_set_hsla:
 * @color: a #GeglColor
 * @hue: hue value.
 * @saturation: saturation value.
 * @lightness: lightness value.
 * @alpha: alpha value.
 * @space: (nullable): RGB space.
 *
 * Set color as HSLA data stored as @space. If @space is %NULL, this is
 * equivalent to storing with the default sRGB space.
 */
void         gegl_color_set_hsla               (GeglColor   *color,
                                                gdouble      hue,
                                                gdouble      saturation,
                                                gdouble      lightness,
                                                gdouble      alpha,
                                                const Babl  *space);

/**
 * gegl_color_set_pixel: (skip)
 * @color: a #GeglColor
 * @format: a babl pixel format
 * @pixel: (not nullable): pointer to a pixel
 *
 * Set a GeglColor from a pointer to a pixel and it's babl format.
 */
void         gegl_color_set_pixel              (GeglColor   *color,
                                                const Babl  *format,
                                                const void  *pixel);

/**
 * gegl_color_get_pixel: (skip)
 * @color: a #GeglColor
 * @format: a babl pixel format
 * @pixel: pointer to a pixel
 *
 * Store the color in a pixel in the given format.
 */
void         gegl_color_get_pixel              (GeglColor   *color,
                                                const Babl  *format,
                                                void        *pixel);
/**
 * gegl_color_set_bytes:
 * @color:  a #GeglColor
 * @format: a babl pixel format
 * @bytes:  color stored as @format
 *
 * Set a GeglColor from a pixel stored in a %GBytes and it's babl format.
 */
void         gegl_color_set_bytes              (GeglColor   *color,
                                                const Babl  *format,
                                                GBytes      *bytes);

/**
 * gegl_color_get_bytes:
 * @color: a #GeglColor
 * @format: a babl pixel format
 *
 * Returns: the color in the given @format.
 */
GBytes     * gegl_color_get_bytes              (GeglColor   *color,
                                                const Babl  *format);

/***
 */

#define GEGL_TYPE_PARAM_COLOR           (gegl_param_color_get_type ())
#define GEGL_IS_PARAM_SPEC_COLOR(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), GEGL_TYPE_PARAM_COLOR))

GType        gegl_param_color_get_type         (void) G_GNUC_CONST;

/**
 * gegl_param_spec_color:
 * @name: canonical name of the property specified
 * @nick: nick name for the property specified
 * @blurb: description of the property specified
 * @default_color: the default value for the property specified
 * @flags: flags for the property specified
 *
 * Creates a new #GParamSpec instance specifying a #GeglColor property.
 *
 * Returns: (transfer full): a newly created parameter specification
 */
GParamSpec * gegl_param_spec_color             (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                GeglColor   *default_color,
                                                GParamFlags  flags);

/**
 * gegl_param_spec_color_from_string:
 * @name: canonical name of the property specified
 * @nick: nick name for the property specified
 * @blurb: description of the property specified
 * @default_color_string: the default value for the property specified
 * @flags: flags for the property specified
 *
 * Creates a new #GParamSpec instance specifying a #GeglColor property.
 *
 * Returns: (transfer full): a newly created parameter specification
 */
GParamSpec * gegl_param_spec_color_from_string (const gchar *name,
                                                const gchar *nick,
                                                const gchar *blurb,
                                                const gchar *default_color_string,
                                                GParamFlags  flags);
/**
 * gegl_param_spec_color_get_default:
 * @self: a #GeglColor #GParamSpec
 *
 * Get the default color value of the param spec
 *
 * Returns: (transfer none): the default #GeglColor
 */
GeglColor *
gegl_param_spec_color_get_default (GParamSpec *self);


/**
 * gegl_color_get_format:
 * @color: a #GeglColor
 *
 * Return: (transfer none): the pixel format encoding of the set color.
 */
const Babl *
gegl_color_get_format (GeglColor *color);


G_END_DECLS

#endif /* __GEGL_COLOR_H__ */

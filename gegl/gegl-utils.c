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
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås.
 */

#include "config.h"

#include <string.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-utils.h"


void
gegl_buffer_set_color (GeglBuffer          *dst,
                       const GeglRectangle *dst_rect,
                       GeglColor           *color)
{
  guchar pixel[128];
  const Babl *format;
  g_return_if_fail (GEGL_IS_BUFFER (dst));
  g_return_if_fail (color);
  format = gegl_buffer_get_format (dst);

  gegl_color_get_pixel (color, format, pixel);
  gegl_buffer_set_color_from_pixel (dst, dst_rect, &pixel[0], format);
}


static const Babl *
gegl_babl_format_linear_float (const Babl *format)
{
  const Babl *space = babl_format_get_space (format);
  const Babl *model = NULL;
  if (!format)
    return babl_format ("RGBA float");

  model = babl_format_get_model (format);

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~"))
  {
    format = babl_format_with_space ("Y float", space);
  }
  else if (babl_model_is (model, "YA") ||
           babl_model_is (model, "Y'A") ||
           babl_model_is (model, "Y~A") ||
           babl_model_is (model, "Y~aA") ||
           babl_model_is (model, "YaA") ||
           babl_model_is (model, "Y'aA"))
  {
    format = babl_format_with_space ("YA float", space);
  }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "cmykA") ||
           babl_model_is (model, "camayakaA") ||
           babl_model_is (model, "CMYK") ||
           babl_model_is (model, "CMYKA") ||
           babl_model_is (model, "CaMaYaKaA"))
  {
    format  = babl_format_with_space ("cmykA float", space);
  }
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~"))
  {
    format = babl_format_with_space ("RGB float", space);
  }
#if 0 // just treat as else
           babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~aA float":"RaGaBaA float", space);
  }
#endif
  else
  {
    format  = babl_format_with_space ("RGBA float", space);
  }
  return format;
}

static const Babl *
gegl_babl_format_perceptual_float (const Babl *format)
{
  const Babl *space = babl_format_get_space (format);
  const Babl *model = NULL;
  if (!format)
    return babl_format ("R~G~B~A float");

  model = babl_format_get_model (format);

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~"))
  {
    format = babl_format_with_space ("Y~ float", space);
  }
  else if (babl_model_is (model, "YA") ||
           babl_model_is (model, "Y'A") ||
           babl_model_is (model, "Y~A") ||
           babl_model_is (model, "Y~aA") ||
           babl_model_is (model, "YaA") ||
           babl_model_is (model, "Y'aA"))
  {
    format = babl_format_with_space ("Y~A float", space);
  }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "cmykA") ||
           babl_model_is (model, "camayakaA") ||
           babl_model_is (model, "CMYK") ||
           babl_model_is (model, "CMYKA") ||
           babl_model_is (model, "CaMaYaKaA"))
  {
    format  = babl_format_with_space ("cmykA float", space);
  }
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~"))
  {
    format = babl_format_with_space ("R~G~B~ float", space);
  }
#if 0 // just treat as else
           babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~aA float":"RaGaBaA float", space);
  }
#endif
  else
  {
    format  = babl_format_with_space ("R~G~B~A float", space);
  }
  return format;
}

static const Babl *
gegl_babl_format_nonlinear_float (const Babl *format)
{
  const Babl *space = babl_format_get_space (format);
  const Babl *model = NULL;
  if (!format)
    return babl_format ("R'G'B'A float");

  model = babl_format_get_model (format);

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~"))
  {
    format = babl_format_with_space ("Y' float", space);
  }
  else if (babl_model_is (model, "YA") ||
           babl_model_is (model, "Y'A") ||
           babl_model_is (model, "Y~A") ||
           babl_model_is (model, "Y~aA") ||
           babl_model_is (model, "YaA") ||
           babl_model_is (model, "Y'aA"))
  {
    format = babl_format_with_space ("Y'A float", space);
  }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "cmykA") ||
           babl_model_is (model, "camayakaA") ||
           babl_model_is (model, "CMYK") ||
           babl_model_is (model, "CMYKA") ||
           babl_model_is (model, "CaMaYaKaA"))
  {
    format  = babl_format_with_space ("cmykA float", space);
  }
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~"))
  {
    format = babl_format_with_space ("R'G'B' float", space);
  }
#if 0 // just treat as else
           babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~aA float":"RaGaBaA float", space);
  }
#endif
  else
  {
    format  = babl_format_with_space ("R'G'B'A float", space);
  }
  return format;
}

static const Babl *
gegl_babl_format_premultiplied_linear_float (const Babl *format)
{
  const Babl *space = babl_format_get_space (format);
  const Babl *model = NULL;
  if (!format)
    return babl_format ("RaGaBaA float");

  model = babl_format_get_model (format);

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~") ||
      babl_model_is (model, "YA") ||
      babl_model_is (model, "Y'A") ||
      babl_model_is (model, "Y~A") ||
      babl_model_is (model, "Y~aA") ||
      babl_model_is (model, "YaA") ||
      babl_model_is (model, "Y'aA"))
  {
    format = babl_format_with_space ("YaA float", space);
  }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "cmykA") ||
           babl_model_is (model, "camayakaA") ||
           babl_model_is (model, "CMYK") ||
           babl_model_is (model, "CMYKA") ||
           babl_model_is (model, "CaMaYaKaA"))
  {
    format  = babl_format_with_space ("camayakaA float", space);
  }
#if 0 // just treat as else
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~") ||
           babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "RGB")     ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~aA float":"RaGaBaA float", space);
  }
#endif
  else
  {
    format  = babl_format_with_space ("RaGaBaA float", space);
  }
  return format;
}

static const Babl *
gegl_babl_format_premultiplied_perceptual_float (const Babl *format)
{
  const Babl *space = babl_format_get_space (format);
  const Babl *model = NULL;
  if (!format)
    return babl_format ("R~aG~aB~aA float");

  model = babl_format_get_model (format);

  if (babl_model_is (model, "Y") ||
      babl_model_is (model, "Y'") ||
      babl_model_is (model, "Y~") ||
      babl_model_is (model, "YA") ||
      babl_model_is (model, "Y'A") ||
      babl_model_is (model, "Y~A") ||
      babl_model_is (model, "Y~aA") ||
      babl_model_is (model, "YaA") ||
      babl_model_is (model, "Y'aA"))
  {
    format = babl_format_with_space ("Y~aA float", space);
  }
  else if (babl_model_is (model, "cmyk") ||
      babl_model_is (model, "cmykA") ||
      babl_model_is (model, "camayakaA") ||
      babl_model_is (model, "CMYK") ||
      babl_model_is (model, "CMYKA") ||
      babl_model_is (model, "CaMaYaKaA"))
  {
    format  = babl_format_with_space ("camayakaA float", space);
  }
#if 0 // just treat as else
  else if (babl_model_is (model, "RGB") ||
           babl_model_is (model, "R'G'B'") ||
           babl_model_is (model, "R~G~B~") ||
           babl_model_is (model, "RGBA")    ||
           babl_model_is (model, "RGB")     ||
           babl_model_is (model, "R'G'B'A") ||
           babl_model_is (model, "R'G'B'")  ||
           babl_model_is (model, "R~G~B~A") ||
           babl_model_is (model, "R~G~B~")  ||
           babl_model_is (model, "RaGaBaA") ||
           babl_model_is (model, "R'aG'aB'aA"))
  {
    format  = babl_format_with_space (use_srgb?"R~aG~aB~aA float":"RaGaBaA float", space);
  }
#endif
  else
  {
    format  = babl_format_with_space ("R~aG~aB~aA float", space);
  }
  return format;
}

static const Babl *
gegl_babl_format_float (const Babl *format)
{
  const Babl *space;
  const char *encoding;
  if (!format)
    return NULL;
  space = babl_format_get_space (format);
  encoding  = babl_format_get_encoding (format);

  {
  char *encdup = g_strdup (encoding);
  char *newenc;
  char *s = strrchr (encdup, ' ');
  if (s) *s = 0;
  newenc = g_strdup_printf ("%s float", encdup);
  format = babl_format_with_space (newenc, space);

  g_free (encdup);
  g_free (newenc);
  }
  return format;
}

static const Babl *
gegl_babl_format_alpha (const Babl *format)
{
  const Babl *model = babl_format_get_model (format);
  BablModelFlag model_flags = babl_get_model_flags (model);
  if (model_flags & BABL_MODEL_FLAG_ALPHA)
  {
    const Babl *type = babl_format_get_type (format, 0);
    if (type == babl_type ("float"))
      return format;
  }

  if (babl_model_is (model, "Y'"))
  {
    return babl_format_with_space ("Y'A float", format);
  }
  else if (babl_model_is (model, "Y"))
  {
    return babl_format_with_space ("YA float", format);
  }
  else if (babl_model_is (model, "RGB")||
           babl_model_is (model, "RGBA"))
  {
    return babl_format_with_space ("RGBA float", format);
  }
  else if (babl_model_is (model, "RaGaBaA"))
  {
    return babl_format_with_space ("RaGaBaA float", format);
  }
  else if (babl_model_is (model, "R'aG'aB'aA"))
  {
    return babl_format_with_space ("R'aG'aB'aA float", format);
  }
  else if (babl_model_is (model, "R'G'B'")||
           babl_model_is (model, "R'G'B'A"))
  {
    return babl_format_with_space ("R'G'B'A float", format);
  }
  else if (babl_model_is (model, "cmyk") ||
           babl_model_is (model, "cmykA"))
  {
    return babl_format_with_space ("cmykA float", format);
  }
  else if (babl_model_is (model, "CMYK") ||
           babl_model_is (model, "CMYKA"))
  {
    return babl_format_with_space ("cmykA float", format);
  }
  else if (babl_model_is (model, "CaMaYaKaA"))
  {
    return babl_format_with_space ("CaMaYaKaA float", format);
  }
  else if (babl_model_is (model, "camayakaA"))
  {
    return babl_format_with_space ("camayakaA float", format);
  }

  return babl_format_with_space ("RGBA float", format);
}


static const Babl *
gegl_babl_format_float_premultiplied_linear_if_alpha (const Babl *format)
{
  if (!format)
    return NULL;
  if (babl_format_has_alpha (format))
    return gegl_babl_format_premultiplied_linear_float (format);
  return gegl_babl_format_float (format);
}

static const Babl *
gegl_babl_format_float_premultiplied_perceptual_if_alpha (const Babl *format)
{
  if (!format)
    return NULL;
  if (babl_format_has_alpha (format))
    return gegl_babl_format_premultiplied_perceptual_float (format);
  return gegl_babl_format_float (format);
}

const Babl *
gegl_babl_variant (const Babl *format, GeglBablVariant variant)
{
  if (!format)
    return NULL;
  switch (variant)
  {
    case GEGL_BABL_VARIANT_ALPHA:
      return gegl_babl_format_alpha (format);
    case GEGL_BABL_VARIANT_FLOAT:
      return gegl_babl_format_float (format);
    case GEGL_BABL_VARIANT_LINEAR:
      return gegl_babl_format_linear_float (format);
    case GEGL_BABL_VARIANT_NONLINEAR:
      return gegl_babl_format_nonlinear_float (format);
    case GEGL_BABL_VARIANT_PERCEPTUAL:
      return gegl_babl_format_perceptual_float (format);
    case GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED:
      return gegl_babl_format_premultiplied_linear_float (format);
    case GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED:
      return gegl_babl_format_premultiplied_perceptual_float (format);
    case GEGL_BABL_VARIANT_LINEAR_PREMULTIPLIED_IF_ALPHA:
      return gegl_babl_format_float_premultiplied_linear_if_alpha (format);
    case GEGL_BABL_VARIANT_PERCEPTUAL_PREMULTIPLIED_IF_ALPHA:
      return gegl_babl_format_float_premultiplied_perceptual_if_alpha (format);
  }
  return format;
}


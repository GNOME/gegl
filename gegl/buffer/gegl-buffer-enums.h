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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 *
 */

/* This file holds public enums from GEGL
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 *
 * Normally, gegl-enums.c file would be be generated my glib-mkenums,
 * but we use the enum values' registered names for translatable,
 * human readable labels for the GUI, so gegl-enums.c is maintained
 * manually.
 *
 * DON'T FORGET TO UPDATE gegl-enums.c AFTER CHANGING THIS HEADER
 *
 * !!!!!!!!!!!! NOTE !!!!!!!!!!!!!!
 */

#ifndef __GEGL_BUFFER_ENUMS_H__
#define __GEGL_BUFFER_ENUMS_H__

G_BEGIN_DECLS

typedef enum {
  /* this enum should be renamed GeglBufferFlags, since it contains
   * multiple flags - and possibly more in the future
   */

  /* XXX: API tidying of the following would be to make them be
   *      GEGL_BUFFER_EDGE_NONE or GEGL_BUFFER_REPEAT_NONE instead.
  */
  GEGL_ABYSS_NONE  = 0,
  GEGL_ABYSS_CLAMP = 1,
  GEGL_ABYSS_LOOP  = 2,
  GEGL_ABYSS_BLACK = 3,
  GEGL_ABYSS_WHITE = 4,

  GEGL_BUFFER_FILTER_AUTO     = 0,
  /* auto gives bilinear for scales <1.0 box for <2.0 and nearest above */
  GEGL_BUFFER_FILTER_BILINEAR = 16,
  GEGL_BUFFER_FILTER_NEAREST  = 32,
  GEGL_BUFFER_FILTER_BOX      = 48,
  GEGL_BUFFER_FILTER_ALL      = (GEGL_BUFFER_FILTER_BILINEAR|
                                 GEGL_BUFFER_FILTER_NEAREST|
                                 GEGL_BUFFER_FILTER_BOX),
} GeglAbyssPolicy;

GType gegl_abyss_policy_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ABYSS_POLICY (gegl_abyss_policy_get_type ())


typedef enum {
  GEGL_ACCESS_READ      = 1 << 0,
  GEGL_ACCESS_WRITE     = 1 << 1,
  GEGL_ACCESS_READWRITE = (GEGL_ACCESS_READ | GEGL_ACCESS_WRITE)
} GeglAccessMode;

GType gegl_access_mode_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_ACCESS_MODE (gegl_access_mode_get_type ())

typedef enum {
  GEGL_SAMPLER_NEAREST,
  GEGL_SAMPLER_LINEAR,
  GEGL_SAMPLER_CUBIC,
  GEGL_SAMPLER_NOHALO,
  GEGL_SAMPLER_LOHALO
} GeglSamplerType;

GType gegl_sampler_type_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_SAMPLER_TYPE (gegl_sampler_type_get_type ())

typedef enum {
  GEGL_RECTANGLE_ALIGNMENT_SUBSET,
  GEGL_RECTANGLE_ALIGNMENT_SUPERSET,
  GEGL_RECTANGLE_ALIGNMENT_NEAREST
} GeglRectangleAlignment;

GType gegl_rectangle_alignment_get_type (void) G_GNUC_CONST;

#define GEGL_TYPE_RECTANGLE_ALIGNMENT (gegl_rectangle_alignment_get_type ())

G_END_DECLS

#endif /* __GEGL_ENUMS_H__ */

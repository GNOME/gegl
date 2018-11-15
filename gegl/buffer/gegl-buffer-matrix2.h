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
 * Copyright 2006-2018 GEGL developers
 */

#ifndef __GEGL_BUFFER_MATRIX2_H__
#define __GEGL_BUFFER_MATRIX2_H__


#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* Currenly only used internally.
 * Note: If making use of this in public API, add a boxed type for introspection
 */
typedef struct {
    gdouble coeff[2][2];
} GeglBufferMatrix2;

/*
 * gegl_buffer_matrix2_is_identity:
 * @matrix: a #GeglBufferMatrix2
 *
 * Check if a matrix is the identity matrix.
 *
 * Returns TRUE if the matrix is the identity matrix.
 */
gboolean   gegl_buffer_matrix2_is_identity (GeglBufferMatrix2 *matrix);

/*
 * gegl_buffer_matrix2_is_scale:
 * @matrix: a #GeglBufferMatrix2
 *
 * Check if a matrix only does scaling.
 *
 * Returns TRUE if the matrix only does scaling.
 */
gboolean   gegl_buffer_matrix2_is_scale    (GeglBufferMatrix2 *matrix);

/*
 * gegl_buffer_matrix2_determinant:
 * @matrix: a #GeglBufferMatrix2
 *
 * Returns the determinant of @matrix.
 */
gdouble    gegl_buffer_matrix2_determinant (GeglBufferMatrix2 *matrix);


G_END_DECLS

#endif

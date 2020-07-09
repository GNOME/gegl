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

#ifndef __GEGL_MATRIX_H__
#define __GEGL_MATRIX_H__


#include <glib.h>
#include <glib-object.h>
#include <gegl-buffer-matrix2.h>

G_BEGIN_DECLS

/***
 * GeglMatrix3:
 *
 * #GeglMatrix3 is a 3x3 matrix for GEGL.
 * Matrixes are currently used by #GeglPath and the affine operations,
 * they might be used more centrally in the core of GEGL later.
 *
 */
typedef struct {
    gdouble coeff[3][3];
} GeglMatrix3;

#define GEGL_TYPE_MATRIX3               (gegl_matrix3_get_type ())
#define GEGL_VALUE_HOLDS_MATRIX3(value) (G_TYPE_CHECK_VALUE_TYPE ((value), GEGL_TYPE_MATRIX3))

/**
 * gegl_matrix3_get_type:
 *
 * Returns: the #GType for GeglMatrix3 objects
 *
 **/
GType         gegl_matrix3_get_type     (void) G_GNUC_CONST;

/**
 * gegl_matrix3_new:
 *
 * Return: A newly allocated #GeglMatrix3
 */
GeglMatrix3 * gegl_matrix3_new (void);

/**
 * gegl_matrix3_identity:
 * @matrix: a #GeglMatrix3
 *
 * Set the provided @matrix to the identity matrix.
 */
void       gegl_matrix3_identity        (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_round_error:
 * @matrix: a #GeglMatrix3
 *
 * Rounds numerical errors in @matrix to the nearest integer.
 */
void       gegl_matrix3_round_error     (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_equal:
 * @matrix1: a #GeglMatrix3
 * @matrix2: a #GeglMatrix3
 *
 * Check if two matrices are equal.
 *
 * Returns TRUE if the matrices are equal.
 */
gboolean   gegl_matrix3_equal           (GeglMatrix3 *matrix1,
                                         GeglMatrix3 *matrix2);

/**
 * gegl_matrix3_is_identity:
 * @matrix: a #GeglMatrix3
 *
 * Check if a matrix is the identity matrix.
 *
 * Returns TRUE if the matrix is the identity matrix.
 */
gboolean   gegl_matrix3_is_identity     (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_is_scale:
 * @matrix: a #GeglMatrix3
 *
 * Check if a matrix only does scaling.
 *
 * Returns TRUE if the matrix only does scaling.
 */
gboolean   gegl_matrix3_is_scale        (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_is_translate:
 * @matrix: a #GeglMatrix3
 *
 * Check if a matrix only does translation.
 *
 * Returns TRUE if the matrix only does trasnlation.
 */
gboolean   gegl_matrix3_is_translate    (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_is_affine:
 * @matrix: a #GeglMatrix3
 *
 * Check if a matrix only does an affine transformation.
 *
 * Returns TRUE if the matrix only does an affine transformation.
 */
gboolean   gegl_matrix3_is_affine       (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_copy_into:
 * @dst: a #GeglMatrix3
 * @src: a #GeglMatrix3
 *
 * Copies the matrix in @src into @dst.
 */
void  gegl_matrix3_copy_into (GeglMatrix3 *dst,
                              GeglMatrix3 *src);

/**
 * gegl_matrix3_copy:
 * @matrix: a #GeglMatrix3
 *
 * Returns a copy of @src.
 */
GeglMatrix3 *   gegl_matrix3_copy (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_determinant:
 * @matrix: a #GeglMatrix3
 *
 * Returns the determinant for the matrix.
 */
gdouble    gegl_matrix3_determinant     (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_invert:
 * @matrix: a #GeglMatrix3
 *
 * Inverts @matrix.
 */
void       gegl_matrix3_invert          (GeglMatrix3 *matrix);

/**
 * gegl_matrix3_multiply:
 * @left: a #GeglMatrix3
 * @right: a #GeglMatrix3
 * @product: a #GeglMatrix3 to store the result in.
 *
 * Multiples @product = @left · @right
 */
void       gegl_matrix3_multiply        (GeglMatrix3 *left,
                                         GeglMatrix3 *right,
                                         GeglMatrix3 *product);

/**
 * gegl_matrix3_originate:
 * @matrix: a #GeglMatrix3
 * @x: x coordinate of new origin
 * @y: y coordinate of new origin.
 *
 * Shift the origin of the transformation specified by @matrix
 * to (@x, @y). In other words, calculate the matrix that:
 *
 * 1. Translates the input by (-@x, -@y).
 *
 * 2. Transforms the result using the original @matrix.
 *
 * 3. Translates the result by (@x, @y).
 *
 */
void       gegl_matrix3_originate       (GeglMatrix3 *matrix,
                                         gdouble      x,
                                         gdouble      y);


/**
 * gegl_matrix3_transform_point:
 * @matrix: a #GeglMatrix3
 * @x: pointer to an x coordinate
 * @y: pointer to an y coordinate
 *
 * transforms the coordinates provided in @x and @y and changes to the
 * coordinates gotten when the transformed with the matrix.
 *
 */
void       gegl_matrix3_transform_point (GeglMatrix3 *matrix,
                                         gdouble    *x,
                                         gdouble    *y);

/**
 * gegl_matrix3_parse_string:
 * @matrix: a #GeglMatrix3
 * @string: a string describing the matrix (right now a small subset of the
 * transform strings allowed by SVG)
 *
 * Parse a transofmation matrix from a string.
 */
void       gegl_matrix3_parse_string    (GeglMatrix3 *matrix,
                                         const gchar *string);
/**
 * gegl_matrix3_to_string:
 * @matrix: a #GeglMatrix3
 *
 * Serialize a #GeglMatrix3 to a string.
 *
 * Returns a freshly allocated string representing that #GeglMatrix3, the
 * returned string should be g_free()'d.
 *
 */
gchar *    gegl_matrix3_to_string       (GeglMatrix3 *matrix);

/***
 */

G_END_DECLS

#endif

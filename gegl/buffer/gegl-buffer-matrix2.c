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
 */

#include "config.h"
#include <math.h>

#include "gegl-buffer-matrix2.h"


gboolean
gegl_buffer_matrix2_is_identity (GeglBufferMatrix2 *matrix)
{
  return matrix->coeff[0][0] == 1.0 && matrix->coeff[0][1] == 0.0 &&
         matrix->coeff[1][0] == 0.0 && matrix->coeff[1][1] == 1.0;
}

gboolean
gegl_buffer_matrix2_is_scale (GeglBufferMatrix2 *matrix)
{
  return matrix->coeff[0][1] == 0.0 && matrix->coeff[1][0] == 0.0;
}

gdouble
gegl_buffer_matrix2_determinant (GeglBufferMatrix2 *matrix)
{
  return matrix->coeff[0][0] * matrix->coeff[1][1] -
         matrix->coeff[1][0] * matrix->coeff[0][1];
}

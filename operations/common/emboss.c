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
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 * 
 * Copyright 1997 Eric L. Hernes <erich@rrnet.com>
 * Copyright 2012 Hans Lo <hansshulo@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_emboss_type)
  enum_value (GEGl_EMBOSS, "Emboss")
  enum_value (GEGl_BUMPMAP, "Bumpmap")
gegl_chant_register_enum_end (GeglEmboss)

gegl_chant_enum (emboss_type, _("Function"), GeglEmboss, gegl_emboss_type,
                 GEGl_EMBOSS, _("Emboss or Bumpmap"))

gegl_chant_double (azimuth, _("Azimuth"), 0.0, 360.0, 10.0,
                   _("Azimuth"))

gegl_chant_double (elevation, _("Elevation"), 0.0, 180.0, 45.0,
                   _("Elevation"))

gegl_chant_double (depth, _("Depth"), 1.0, 100.0, 20.0,
                   _("Depth"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "emboss.c"

#include "gegl-chant.h"
#include <math.h>

#define DtoR(d) ((d)*(G_PI/(gdouble)180))

typedef struct {
  gdouble      Lx;
  gdouble      Ly;
  gdouble      Lz;
  gdouble      bg;
  gint         Nz;
  gint         Nz2;
  gint         NzLz;
  
} LightVector;

static void
emboss_init (gdouble azimuth,
             gdouble elevation,
             gint depth,
	     LightVector *vec)
{
  /*
   * compute the light vector from the input parameters.
   * normalize the length to pixelScale for fast shading calculation.
   */
  vec->Lx = cos (azimuth) * cos (elevation);
  vec->Ly = sin (azimuth) * cos (elevation);
  vec->Lz = sin (elevation);

  /*
   * constant z component of image surface normal - this depends on the
   * image slope we wish to associate with an angle of 45 degrees, which
   * depends on the width of the filter used to produce the source image.
   */
  vec->Nz = 1.0 / depth;
  vec->Nz2 = vec->Nz * vec->Nz;
  vec->NzLz = vec->Nz * vec->Lz;
  
  /* optimization for vertical normals: L.[0 0 1] */
  vec->bg = vec->Lz;
}

static void
emboss_pixel (gint x,
	      gint y,
	      LightVector vec,
	      GeglSampler *sampler,
	      gfloat* dst_pix,
	      gint bytes) //may need emboss/bumpmap
{
  gfloat  M[3][3];
    
  gfloat   Nx, Ny, NdotL;
  gfloat   shade;
  gint     i, j, b;
  gfloat   pix[2];
  gfloat   center_p[bytes];
  
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      M[i][j] = 0.0;
   
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	gegl_sampler_get (sampler,
			  x + i,
			  y + j,
			  NULL,
			  pix);
	M[i][j] += pix[1] * pix[0];
	if (i == 1 && j == 1)
	  {
	    for (b = 0; b < bytes; b++)
	      center_p[b] = pix[b];
	  }
      }
 
  Nx = M[0][0] + M[1][0] + M[2][0] - M[0][2] - M[1][2] - M[2][2];
  Ny = M[2][0] + M[2][1] + M[2][2] - M[0][0] - M[0][1] - M[0][2];

  /* shade with distant light source */
  if ( Nx == 0 && Ny == 0 )
    shade = vec.bg;
  else if ( (NdotL = Nx * vec.Lx + Ny * vec.Ly + vec.NzLz) < 0 )
    shade = 0.0;
  else
    shade = NdotL / sqrt(Nx*Nx + Ny*Ny + vec.Nz2);
  
  /* do something with the shading result */
  if (bytes == 4) /* rgba, bumpmapping */
    {
      for (b = 0; b < 3; b++)
	*dst_pix++ = center_p[b] * shade;
      
      *dst_pix = center_p[3];
    }
  else /* ya, emboss */
    {
      *dst_pix++ = shade;
      *dst_pix = center_p[1];
    }
}

static void prepare (GeglOperation *operation)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);
  
  const Babl *format;
  if (o->emboss_type == GEGl_EMBOSS) 
    format = babl_format ("YA float");
  else
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o                    = GEGL_CHANT_PROPERTIES (operation);
  
  gfloat *dst_buf;
  GeglSampler *sampler;
  LightVector vec;
  gfloat *dst_pix;
  gint x, y;
  gint n_pixels;
  const Babl *format;
  gint bytes;
  
  if (o->emboss_type == GEGl_EMBOSS)
    {
      format = babl_format ("YA float");
      bytes  = 2;
    }
  else 
    {
      format = babl_format ("RGBA float");
      bytes  = 4;
    }
  
  dst_buf = g_slice_alloc (result->width * result->height * bytes * sizeof(gfloat));
  dst_pix = dst_buf;
  sampler = gegl_buffer_sampler_new (input,
				     format,
				     GEGL_SAMPLER_CUBIC);
  
  emboss_init(DtoR(o->azimuth), DtoR(o->elevation), o->depth, &vec);
  x = result->x;
  y = result->y;
  n_pixels = result->width * result->height;
  while (n_pixels--)
    {
      emboss_pixel(x, y, vec, sampler, dst_pix, bytes);
      dst_pix += bytes;
      x++;
      if (x == result->x + result->width)
	{
	  x = result->x;
	  y++;
	}
    }

  g_object_unref (sampler);
  gegl_buffer_set (output, result, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_slice_free1 (result->width * result->height * bytes * sizeof(gfloat), dst_buf);

  return TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationFilterClass  *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
      "name"       , "gegl:emboss",
      "categories" , "distort",
      "description", _("Emulate an emboss effect."),
      NULL);
}

#endif

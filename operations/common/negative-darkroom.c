/* This file is an image processing operation for GEGL
* foo
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
* Copyright 2015 Red Hat, Inc.
*/

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>
#include <stdio.h>

#ifdef GEGL_PROPERTIES

#include "negative-darkroom/negative-darkroom-curve-enum.c"

property_enum (curve, _("Characteristic curve"),
		NegCurve, neg_curve, 0)
	description(_("Hardcoded characteristic curve and color data"))

property_double (exposure, _("Exposure"), -9.0)
	description(_("Base enlargement exposure"))
	value_range (-20, 10)
	ui_range (-15, 0)

property_double (expC, _("Filter cyan"), 0.0)
	description(_("Cyan exposure compensation for the negative image"))
	value_range (-3, 3)

property_double (expM, _("Filter magenta"), 0.0)
	description(_("Magenta exposure compensation for the negative image"))
	value_range (-3, 3)

property_double (expY, _("Filter yellow"), 0.0)
	description(_("Yellow exposure compensation for the negative image"))
	value_range (-3, 3)

property_boolean (clip, _("Clip base + fog"), TRUE)
	description (_("Clip base + fog to have a pure white output value"))

property_double (boost, _("Density boost"), 1.0)
	description(_("Boost paper density to take advantage of increased dynamic range of a monitor compared to a photographic paper"))
	value_range (1, 3)

property_double (dodge, _("Dodge/burn multiplier"), 1.0)
	description(_("The f-stop of dodge/burn for pure white/black auxillary input"))
	value_range (-4.0, 4.0)

property_boolean (preflash, _("Enable preflashing"), FALSE)
	description (_("Show preflash controls"))

property_double (flashC, _("Cyan preflash"), 0)
	description(_("Preflash the negative with cyan light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

property_double (flashM, _("Magenta preflash"), 0)
	description(_("Preflash the negative with magenta light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

property_double (flashY, _("Yellow preflash"), 0)
	description(_("Preflash the negative with yellow light to reduce contrast of the print"))
	value_range (0, 1)
	ui_meta("visible", "preflash")

#else

#define GEGL_OP_POINT_COMPOSER
#define GEGL_OP_NAME     negative_darkroom
#define GEGL_OP_C_SOURCE negative-darkroom.c

#include "gegl-op.h"

// Color space
typedef struct cieXYZ {
	gfloat X;
	gfloat Y;
	gfloat Z;
} cieXYZ;

// RGB Hurter–Driffield characteristic curve

typedef struct HDCurve {
	gfloat *rx;
	gfloat *ry;
	guint   rn;
	gfloat *gx;
	gfloat *gy;
	guint   gn;
	gfloat *bx;
	gfloat *by;
	guint   bn;
	cieXYZ rsens;
	cieXYZ gsens;
	cieXYZ bsens;
	cieXYZ cdens;
	cieXYZ mdens;
	cieXYZ ydens;
} HDCurve;

#include "negative-darkroom/negative-darkroom-curve-enum.h"

static void
prepare (GeglOperation *operation)
{
	const Babl *space = gegl_operation_get_source_space (operation, "input");
	const Babl *fXYZ;
	const Babl *fRGB;

	fXYZ  = babl_format_with_space ("CIE XYZ float", space);
	fRGB = babl_format ("R~G~B~ float");

	gegl_operation_set_format (operation, "input", fXYZ);
	gegl_operation_set_format (operation, "aux", fRGB);
	gegl_operation_set_format (operation, "output", fXYZ);
}

static gfloat
curve_lerp (gfloat * xs, gfloat * ys, guint n, gfloat in)
{
	if (in <= xs[0])
		return(ys[0]);
	for (guint i = 1; i <= n; i++)
	{
		if (in <= xs[i])
		{
			return(ys[i-1] + (in - xs[i-1]) * \
			       ((ys[i] - ys[i-1]) / (xs[i] - xs[i-1])));
		}
	}	
	return(ys[n-1]);
}

static gfloat
array_min (gfloat * x, guint n)
{
	gfloat min = x[0];
	for (guint i = 1; i < n; i++)
	{
		if (x[i] < min)
			min = x[i];
	}
	return(min);
}

static gboolean
process (GeglOperation       *operation,
	 void                *in_buf,
	 void                *aux_buf,
	 void                *out_buf,
	 glong                n_pixels,
	 const GeglRectangle *roi,
	 gint                 level)
{
	GeglProperties *o = GEGL_PROPERTIES (operation);

	gfloat *in   = in_buf;
	gfloat *aux  = aux_buf;
	gfloat *out  = out_buf;

	gfloat Dfogc = 0;
	gfloat Dfogm = 0;
	gfloat Dfogy = 0;

	gfloat rcomp = 0;
	gfloat gcomp = 0;
	gfloat bcomp = 0;

	gfloat r = 0;
	gfloat g = 0;
	gfloat b = 0;

	// Calculate base+fog
	if (o->clip)
	{
		Dfogc = array_min(curves[o->curve].ry, curves[o->curve].rn) * o->boost;
		Dfogm = array_min(curves[o->curve].gy, curves[o->curve].gn) * o->boost;
		Dfogy = array_min(curves[o->curve].by, curves[o->curve].bn) * o->boost;
	}

	for (glong i = 0; i < n_pixels; i++)
	{
		/*printf("---\n");*/
		/*printf("Input XYZ intensity %f %f %f\n", in[0], in[1], in[2]);*/

		// Calculate exposure compensation from global+filter+dodge
		if (!aux)
		{
			rcomp = o->exposure + o->expC;
			gcomp = o->exposure + o->expM;
			bcomp = o->exposure + o->expY;
		}
		else
		{
			rcomp = o->exposure + o->expC + 2*o->dodge*(aux[0]-0.5);
			gcomp = o->exposure + o->expM + 2*o->dodge*(aux[1]-0.5);
			bcomp = o->exposure + o->expY + 2*o->dodge*(aux[2]-0.5);
			aux  += 3;
		}

		/*printf("===\nRGB compensation %f %f %f\n", rcomp, gcomp, bcomp);*/

		// Calculate RGB from XYZ using paper sensitivity
		r = in[0] * curves[o->curve].rsens.X +
		    in[1] * curves[o->curve].gsens.X +
		    in[2] * curves[o->curve].bsens.X;
		g = in[0] * curves[o->curve].rsens.Y +
		    in[1] * curves[o->curve].gsens.Y +
		    in[2] * curves[o->curve].bsens.Y;
		b = in[0] * curves[o->curve].rsens.Z +
		    in[1] * curves[o->curve].gsens.Z +
		    in[2] * curves[o->curve].bsens.Z;

		/*printf("Linear RGB intensity %f %f %f\n", r, g, b);*/

		// Apply the preflash
		r = r + o->flashC;
		g = g + o->flashM;
		b = b + o->flashY;

		/*printf("Linear RGB intensity after preflash %f %f %f\n", r, g, b);*/

		// Logarithmize the input and apply compensation
		r = log(r / pow(2, rcomp)) / log(10);
		g = log(g / pow(2, gcomp)) / log(10);
		b = log(b / pow(2, bcomp)) / log(10);

		/*printf("Logarithmic RGB intensity %f %f %f\n", r, g, b);*/

		// Apply the DH curve
		r = curve_lerp(curves[o->curve].rx,
			       curves[o->curve].ry,
			       curves[o->curve].rn,
			       r);
		g = curve_lerp(curves[o->curve].gx,
			       curves[o->curve].gy,
			       curves[o->curve].gn,
			       g);
		b = curve_lerp(curves[o->curve].bx,
			       curves[o->curve].by,
			       curves[o->curve].bn,
			       b);

		// Apply density boost
		r = r * o->boost;
		g = g * o->boost;
		b = b * o->boost;

		// Compensate for fog
		r -= Dfogc;
		g -= Dfogm;
		b -= Dfogy;

		/*printf("RGB density %f %f %f\n", r, g, b);*/

		// Exponentiate to get the linear representation in tramsmittance back
		r = 1-1/pow(10, r);
		g = 1-1/pow(10, g);
		b = 1-1/pow(10, b);

		/*printf("RGB absolute transparency %f %f %f\n", r, g, b);*/

		// Calculate and return XYZ
		out[0] = 1 - (r * curves[o->curve].cdens.X) -
			     (g * curves[o->curve].mdens.X) -
			     (b * curves[o->curve].ydens.X);
		out[1] = 1 - (r * curves[o->curve].cdens.Y) -
			     (g * curves[o->curve].mdens.Y) -
			     (b * curves[o->curve].ydens.Y);
		out[2] = 1 - (r * curves[o->curve].cdens.Z) -
			     (g * curves[o->curve].mdens.Z) -
			     (b * curves[o->curve].ydens.Z);

		/*printf("XYZ output %f %f %f\n", out[0], out[1], out[2]);*/

		in   += 3;
		out  += 3;
	}

	return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
	GeglOperationClass               *operation_class;
	GeglOperationPointComposerClass *point_composer_class;

	operation_class      = GEGL_OPERATION_CLASS (klass);
	point_composer_class = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

	operation_class->prepare = prepare;
	operation_class->threaded = TRUE;
	operation_class->opencl_support = FALSE;

	point_composer_class->process = process;

	gegl_operation_class_set_keys (operation_class,
		"name"       , "gegl:negative-darkroom",
		"title",       _("Negative Darkroom"),
		"categories" , "color",
		"description", _("Simulate a negative film enlargement in an analog darkroom."),
		NULL);
}

#endif


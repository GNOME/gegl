/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2017 Ell
 */

#ifndef __GEGL_STATS_H__
#define __GEGL_STATS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GEGL_STATS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_STATS, GeglStatsClass))
#define GEGL_IS_STATS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_STATS))
#define GEGL_STATS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_STATS, GeglStatsClass))
/* The rest is in gegl-types.h */

typedef struct _GeglStatsClass GeglStatsClass;

struct _GeglStats
{
  GObject  parent_instance;
};

struct _GeglStatsClass
{
  GObjectClass  parent_class;
};

void   gegl_stats_reset (GeglStats *stats);

G_END_DECLS

#endif

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
 * Copyright 2018 Ell
 */

#ifndef __GEGL_PARALLEL_PRIVATE_H__
#define __GEGL_PARALLEL_PRIVATE_H__


G_BEGIN_DECLS


void      gegl_parallel_init                             (void);
void      gegl_parallel_cleanup                          (void);

gdouble   gegl_parallel_distribute_get_thread_time       (void);

gint      gegl_parallel_distribute_get_optimal_n_threads (gdouble n_elements,
                                                          gdouble thread_cost);


/*  stats  */

gint      gegl_parallel_get_n_assigned_worker_threads    (void);
gint      gegl_parallel_get_n_active_worker_threads      (void);


G_END_DECLS


#endif /* __GEGL_PARALLEL_PRIVATE_H__ */

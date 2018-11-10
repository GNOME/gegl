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

#ifndef __GEGL_PARALLEL_H__
#define __GEGL_PARALLEL_H__


G_BEGIN_DECLS


/**
 * GeglParallelDistributeFunc:
 * @i: the current thread index, in the range [0,@n)
 * @n: the number of threads execution is distributed across
 * @user_data: user data pointer
 *
 * Specifies the type of function passed to gegl_parallel_distribute().
 *
 * The function should process the @i-th part of the data, out of @n
 * equal parts.  @n may be less-than or equal-to the @max_n argument
 * passed to gegl_parallel_distribute().
 */
typedef void (* GeglParallelDistributeFunc)      (gint                 i,
                                                  gint                 n,
                                                  gpointer             user_data);

/**
 * GeglParallelDistributeRangeFunc:
 * @offset: the current data offset
 * @size: the current data size
 * @user_data: user data pointer
 *
 * Specifies the type of function passed to gegl_parallel_distribute_range().
 *
 * The function should process @size elements of the data, starting
 * at @offset.  @size may be greater-than or equal-to the @min_sub_size
 * argument passed to gegl_parallel_distribute_range().
 */
typedef void (* GeglParallelDistributeRangeFunc) (gsize                offset,
                                                  gsize                size,
                                                  gpointer             user_data);

/**
 * GeglParallelDistributeAreaFunc:
 * @area: the current sub-region
 * @user_data: user data pointer
 *
 * Specifies the type of function passed to gegl_parallel_distribute_area().
 *
 * The function should process the sub-region specified by @area, whose
 * area may be greater-than or equal-to the @min_sub_area argument passed
 * to gegl_parallel_distribute_area().
 *
 */
typedef void (* GeglParallelDistributeAreaFunc)  (const GeglRectangle *area,
                                                  gpointer             user_data);


/**
 * gegl_parallel_distribute:
 * @max_n: the maximal number of threads to use
 * @func: (closure user_data) (scope call): the function to call
 * @user_data: user data to pass to the function
 *
 * Distributes the execution of a function across multiple threads,
 * by calling it with a different index on each thread.
 */
void   gegl_parallel_distribute       (gint                             max_n,
                                       GeglParallelDistributeFunc       func,
                                       gpointer                         user_data);

/**
 * gegl_parallel_distribute_range:
 * @size: the total size of the data
 * @min_sub_size: the minimal data size to be processed by each thread
 * @func: (closure user_data) (scope call): the function to call
 * @user_data: user data to pass to the function
 *
 * Distributes the processing of a linear data-structure across
 * multiple threads, by calling the given function with different
 * sub-ranges on different threads.
 */
void   gegl_parallel_distribute_range (gsize                            size,
                                       gsize                            min_sub_size,
                                       GeglParallelDistributeRangeFunc  func,
                                       gpointer                         user_data);

/**
 * gegl_parallel_distribute_area:
 * @area: the region to process
 * @min_sub_area: the minimal area to be processed by each thread
 * @split_strategy: the strategy to use for dividing the region
 * @func: (closure user_data) (scope call): the function to call
 * @user_data: user data to pass to the function
 *
 * Distributes the processing of a planar data-structure across
 * multiple threads, by calling the given function with different
 * sub-regions on different threads.
 */
void   gegl_parallel_distribute_area  (const GeglRectangle             *area,
                                       gsize                            min_sub_area,
                                       GeglSplitStrategy                split_strategy,
                                       GeglParallelDistributeAreaFunc   func,
                                       gpointer                         user_data);


#ifdef __cplusplus

extern "C++"
{

template <class ParallelDistributeFunc>
inline void
gegl_parallel_distribute (gint                   max_n,
                          ParallelDistributeFunc func)
{
  gegl_parallel_distribute (max_n,
                            [] (gint     i,
                                gint     n,
                                gpointer user_data)
                            {
                              ParallelDistributeFunc func_copy (
                                *(const ParallelDistributeFunc *) user_data);

                              func_copy (i, n);
                            },
                            &func);
}

template <class ParallelDistributeRangeFunc>
inline void
gegl_parallel_distribute_range (gsize                       size,
                                gsize                       min_sub_size,
                                ParallelDistributeRangeFunc func)
{
  gegl_parallel_distribute_range (size, min_sub_size,
                                  [] (gsize    offset,
                                      gsize    size,
                                      gpointer user_data)
                                  {
                                    ParallelDistributeRangeFunc func_copy (
                                      *(const ParallelDistributeRangeFunc *) user_data);

                                    func_copy (offset, size);
                                  },
                                  &func);
}

template <class ParallelDistributeAreaFunc>
inline void
gegl_parallel_distribute_area (const GeglRectangle        *area,
                               gsize                       min_sub_area,
                               GeglSplitStrategy           split_strategy,
                               ParallelDistributeAreaFunc  func)
{
  gegl_parallel_distribute_area (area, min_sub_area, split_strategy,
                                 [] (const GeglRectangle *area,
                                     gpointer             user_data)
                                 {
                                   ParallelDistributeAreaFunc func_copy (
                                     *(const ParallelDistributeAreaFunc *) user_data);

                                   func_copy (area);
                                 },
                                 &func);
}

}

#endif /* __cplusplus */


G_END_DECLS


#endif /* __GEGL_PARALLEL_H__ */

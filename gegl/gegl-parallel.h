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
 * Specifies the type of function passed to
 * gegl_parallel_distribute_range().
 *
 * The function should process @size elements of the data, starting
 * at @offset.
 */
typedef void (* GeglParallelDistributeRangeFunc) (gsize                offset,
                                                  gsize                size,
                                                  gpointer             user_data);

/**
 * GeglParallelDistributeAreaFunc:
 * @area: the current sub-area
 * @user_data: user data pointer
 *
 * Specifies the type of function passed to
 * gegl_parallel_distribute_area().
 *
 * The function should process the sub-area specified by @area.
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
 * @thread_cost: the cost of using each additional thread, relative
 *               to the cost of processing a single data element
 * @func: (closure user_data) (scope call): the function to call
 * @user_data: user data to pass to the function
 *
 * Distributes the processing of a linear data-structure across
 * multiple threads, by calling the given function with different
 * sub-ranges on different threads.
 */
void   gegl_parallel_distribute_range (gsize                            size,
                                       gdouble                          thread_cost,
                                       GeglParallelDistributeRangeFunc  func,
                                       gpointer                         user_data);

/**
 * gegl_parallel_distribute_area:
 * @area: the area to process
 * @thread_cost: the cost of using each additional thread, relative
 *               to the cost of processing a single data element
 * @split_strategy: the strategy to use for dividing the area
 * @func: (closure user_data) (scope call): the function to call
 * @user_data: user data to pass to the function
 *
 * Distributes the processing of a planar data-structure across
 * multiple threads, by calling the given function with different
 * sub-areas on different threads.
 */
void   gegl_parallel_distribute_area  (const GeglRectangle             *area,
                                       gdouble                          thread_cost,
                                       GeglSplitStrategy                split_strategy,
                                       GeglParallelDistributeAreaFunc   func,
                                       gpointer                         user_data);


#ifdef __cplusplus
#if __cplusplus >= 201103

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
                                gdouble                     thread_cost,
                                ParallelDistributeRangeFunc func)
{
  gegl_parallel_distribute_range (size, thread_cost,
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
                               gdouble                     thread_cost,
                               GeglSplitStrategy           split_strategy,
                               ParallelDistributeAreaFunc  func)
{
  gegl_parallel_distribute_area (area, thread_cost, split_strategy,
                                 [] (const GeglRectangle *area,
                                     gpointer             user_data)
                                 {
                                   ParallelDistributeAreaFunc func_copy (
                                     *(const ParallelDistributeAreaFunc *) user_data);

                                   func_copy (area);
                                 },
                                 &func);
}

template <class ParallelDistributeAreaFunc>
inline void
gegl_parallel_distribute_area (const GeglRectangle        *area,
                               gdouble                     thread_cost,
                               ParallelDistributeAreaFunc  func)
{
  gegl_parallel_distribute_area (area, thread_cost, GEGL_SPLIT_STRATEGY_AUTO,
                                 func);
}

}

#endif /* __cplusplus >= 201103 */
#endif


G_END_DECLS


#endif /* __GEGL_PARALLEL_H__ */

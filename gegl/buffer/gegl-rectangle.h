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
 * Copyright 2003 Calvin Williamson
 */


#ifndef __GEGL_RECTANGLE_H__
#define __GEGL_RECTANGLE_H__

G_BEGIN_DECLS


GType gegl_rectangle_get_type (void) G_GNUC_CONST;
#define GEGL_TYPE_RECTANGLE   (gegl_rectangle_get_type())

#ifndef __cplusplus

#define  GEGL_RECTANGLE(x,y,w,h) (&((GeglRectangle){(x), (y),   (w), (h)}))

#else

static inline GeglRectangle
_gegl_rectangle_helper (gint x,
                               gint y,
                               gint width,
                               gint height)
{
  GeglRectangle result = {x, y, width, height};
  return result;
}

#define  GEGL_RECTANGLE(x,y,w,h) \
  ((GeglRectangle *) &(const GeglRectangle &) ::_gegl_rectangle_helper (x, y, w, h))

#endif /* __cplusplus */

/***
 * GeglRectangle:
 *
 * GeglRectangles are used in #gegl_node_get_bounding_box and #gegl_node_blit
 * for specifying rectangles.
 *
 * </p><pre>struct GeglRectangle
 * {
 *   gint x;
 *   gint y;
 *   gint width;
 *   gint height;
 * };</pre><p>
 *
 */

/**
 * gegl_rectangle_new:
 * @x: upper left x coordinate
 * @y: upper left y coordinate
 * @width: width in pixels.
 * @height: height in pixels.
 *
 * Creates a new rectangle set with the values from @x, @y, @width and @height.
 */
GeglRectangle *gegl_rectangle_new        (gint                 x,
                                          gint                 y,
                                          guint                width,
                                          guint                height);

/**
 * gegl_rectangle_set:
 * @rectangle: a #GeglRectangle
 * @x: upper left x coordinate
 * @y: upper left y coordinate
 * @width: width in pixels.
 * @height: height in pixels.
 *
 * Sets the @x, @y, @width and @height on @rectangle.
 */
void        gegl_rectangle_set           (GeglRectangle       *rectangle,
                                          gint                 x,
                                          gint                 y,
                                          guint                width,
                                          guint                height);

/**
 * gegl_rectangle_equal:
 * @rectangle1: a #GeglRectangle
 * @rectangle2: a #GeglRectangle
 *
 * Check if two #GeglRectangles are equal.
 *
 * Returns TRUE if @rectangle and @rectangle2 are equal.
 */
gboolean    gegl_rectangle_equal         (const GeglRectangle *rectangle1,
                                          const GeglRectangle *rectangle2);

/**
 * gegl_rectangle_equal_coords:
 * @rectangle: a #GeglRectangle
 * @x: X coordinate
 * @y: Y coordinate
 * @width: width of rectangle
 * @height: height of rectangle
 *
 * Check if a rectangle is equal to a set of parameters.
 *
 * Returns TRUE if @rectangle and @x,@y @width x @height are equal.
 */
gboolean    gegl_rectangle_equal_coords  (const GeglRectangle *rectangle,
                                          gint                 x,
                                          gint                 y,
                                          gint                 width,
                                          gint                 height);

/**
 * gegl_rectangle_is_empty:
 * @rectangle: a #GeglRectangle
 *
 * Check if a rectangle has zero area.
 *
 * Returns TRUE if the width or height of @rectangle is 0.
 */
gboolean    gegl_rectangle_is_empty     (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_dup:
 * @rectangle: the #GeglRectangle to duplicate
 *
 * Create a new copy of @rectangle.
 *
 * Return value: (transfer full): a #GeglRectangle
 */
GeglRectangle *gegl_rectangle_dup       (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_copy:
 * @destination: a #GeglRectangle
 * @source: a #GeglRectangle
 *
 * Copies the rectangle information stored in @source over the information in
 * @destination.
 *
 * @destination may point to the same object as @source.
 */
void        gegl_rectangle_copy          (GeglRectangle       *destination,
                                          const GeglRectangle *source);

/**
 * gegl_rectangle_align:
 * @destination: a #GeglRectangle
 * @rectangle: a #GeglRectangle
 * @tile: a #GeglRectangle
 * @alignment: a #GeglRectangleAlignment value
 *
 * Aligns @rectangle to a regular tile grid, of which @tile is a representative
 * tile, and stores the result in @destination.
 *
 * @alignment can be one of:
 *
 *   GEGL_RECTANGLE_ALIGNMENT_SUBSET:  Calculate the biggest aligned rectangle
 *   contained in @rectangle.
 *
 *   GEGL_RECTANGLE_ALIGNMENT_SUPERSET:  Calculate the smallest aligned
 *   rectangle containing @rectangle.
 *
 *   GEGL_RECTANGLE_ALIGNMENT_NEAREST:  Calculate the nearest aligned rectangle
 *   to @rectangle.
 *
 * @destination may point to the same object as @rectangle or @tile.
 *
 * Returns TRUE if the result is not empty.
 */
gboolean    gegl_rectangle_align         (GeglRectangle          *destination,
                                          const GeglRectangle    *rectangle,
                                          const GeglRectangle    *tile,
                                          GeglRectangleAlignment  alignment);

/**
 * gegl_rectangle_align_to_buffer:
 * @destination: a #GeglRectangle
 * @rectangle: a #GeglRectangle
 * @buffer: a #GeglBuffer
 * @alignment: a #GeglRectangleAlignment value
 *
 * Aligns @rectangle to the tile grid of @buffer, and stores the result in
 * @destination.
 *
 * @alignment has the same meaning as for gegl_rectangle_align().
 *
 * @destination may point to the same object as @rectangle.
 *
 * Returns TRUE if the result is not empty.
 */
gboolean  gegl_rectangle_align_to_buffer (GeglRectangle          *destination,
                                          const GeglRectangle    *rectangle,
                                          GeglBuffer             *buffer,
                                          GeglRectangleAlignment  alignment);

/**
 * gegl_rectangle_bounding_box:
 * @destination: a #GeglRectangle
 * @source1: a #GeglRectangle
 * @source2: a #GeglRectangle
 *
 * Computes the bounding box of the rectangles @source1 and @source2 and stores the
 * resulting bounding box in @destination.
 *
 * @destination may point to the same object as @source1 or @source2.
 */
void        gegl_rectangle_bounding_box  (GeglRectangle       *destination,
                                          const GeglRectangle *source1,
                                          const GeglRectangle *source2);

/**
 * gegl_rectangle_intersect:
 * @dest: return location for the intersection of @src1 and @src2, or NULL.
 * @src1: a #GeglRectangle
 * @src2: a #GeglRectangle
 *
 * Calculates the intersection of two rectangles. If the rectangles do not
 * intersect, dest's width and height are set to 0 and its x and y values
 * are undefined.
 *
 * @dest may point to the same object as @src1 or @src2.
 *
 * Returns TRUE if the rectangles intersect.
 */
gboolean    gegl_rectangle_intersect     (GeglRectangle       *dest,
                                          const GeglRectangle *src1,
                                          const GeglRectangle *src2);

/**
 * gegl_rectangle_subtract:
 * @destination: an array of 4 #GeglRectangle elements
 * @minuend: a #GeglRectangle
 * @subtrahend: a #GeglRectangle
 *
 * Subtracts @subtrahend from @minuend, and stores the resulting rectangles in
 * @destination.  Between 0 and 4 disjoint rectangles may be produced.
 *
 * @destination may contain @minuend or @subtrahend.
 *
 * Returns the number of resulting rectangles.
 */
gint        gegl_rectangle_subtract      (GeglRectangle        destination[4],
                                          const GeglRectangle *minuend,
                                          const GeglRectangle *subtrahend);

/**
 * gegl_rectangle_subtract_bounding_box:
 * @destination: a #GeglRectangle
 * @minuend: a #GeglRectangle
 * @subtrahend: a #GeglRectangle
 *
 * Computes the bounding box of the area formed by subtracting @subtrahend
 * from @minuend, and stores the result in @destination.
 *
 * @destination may point to the same object as @minuend or @subtrahend.
 *
 * Returns TRUE if the result is not empty.
 */
gboolean
    gegl_rectangle_subtract_bounding_box (GeglRectangle       *destination,
                                          const GeglRectangle *minuend,
                                          const GeglRectangle *subtrahend);

/**
 * gegl_rectangle_xor:
 * @destination: an array of 4 #GeglRectangle elements
 * @source1: a #GeglRectangle
 * @source2: a #GeglRectangle
 *
 * Computes the symmetric difference of the rectangles @source1 and @source2,
 * and stores the resulting rectangles in @destination.  Between 0 and 4
 * disjoint rectangles may be produced.
 *
 * @destination may contain @rectangle1 or @rectangle2.
 *
 * Returns the number of resulting rectangles.
 */
gint        gegl_rectangle_xor           (GeglRectangle        destination[4],
                                          const GeglRectangle *source1,
                                          const GeglRectangle *source2);

/**
 * gegl_rectangle_contains:
 * @parent: a #GeglRectangle
 * @child: a #GeglRectangle
 *
 * Checks if the #GeglRectangle @child is fully contained within @parent.
 *
 * Returns TRUE if the @child is fully contained in @parent.
 */
gboolean    gegl_rectangle_contains      (const GeglRectangle *parent,
                                          const GeglRectangle *child);

/**
 * gegl_rectangle_infinite_plane:
 *
 * Returns a GeglRectangle that represents an infininte plane.
 */
GeglRectangle gegl_rectangle_infinite_plane (void);

/**
 * gegl_rectangle_is_infinite_plane:
 * @rectangle: A GeglRectangle.
 *
 * Returns TRUE if the GeglRectangle represents an infininte plane,
 * FALSE otherwise.
 */
gboolean gegl_rectangle_is_infinite_plane (const GeglRectangle *rectangle);

/**
 * gegl_rectangle_dump:
 * @rectangle: A GeglRectangle.
 *
 * For debugging purposes, not stable API.
 */
void     gegl_rectangle_dump              (const GeglRectangle *rectangle);




#define GEGL_FLOAT_EPSILON            (1e-5)
#define GEGL_FLOAT_IS_ZERO(value)     (_gegl_float_epsilon_zero ((value)))
#define GEGL_FLOAT_EQUAL(v1, v2)      (_gegl_float_epsilon_equal ((v1), (v2)))

gint        _gegl_float_epsilon_zero  (float     value);
gint        _gegl_float_epsilon_equal (float     v1,
                                              float     v2);



G_END_DECLS

#endif /* __GEGL_UTILS_H__ */

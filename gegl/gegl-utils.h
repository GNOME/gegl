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
 * Copyright 2003-2018 GEGL contributors.
 */


#ifndef __GEGL_UTILS_H__
#define __GEGL_UTILS_H__

G_BEGIN_DECLS

typedef enum GeglSerializeFlag {
  GEGL_SERIALIZE_TRIM_DEFAULTS = (1<<0),
  GEGL_SERIALIZE_VERSION       = (1<<1),
  GEGL_SERIALIZE_INDENT        = (1<<2),
  GEGL_SERIALIZE_BAKE_ANIM     = (1<<3),
} GeglSerializeFlag;

/**
 * gegl_create_chain_argv:
 * @ops: an argv style, NULL terminated array of arguments
 * @op_start: node to pass in as input of chain
 * @op_end: node to get processed data
 * @time: the time to use for interpolatino of keyframed values
 * @rel_dim: relative dimension to scale rel suffixed values by
 * @path_root: path in filesystem to use as relative root
 * @error: error for signalling parsing errors
 *
 * Create a node chain from argv style list of op data.
 */
void gegl_create_chain_argv (char      **ops,
                             GeglNode   *op_start,
                             GeglNode   *op_end,
                             double      time,
                             int         rel_dim,
                             const char *path_root,
                             GError    **error);
/**
 * gegl_create_chain:
 * @ops: an argv style, NULL terminated array of arguments
 * @op_start: node to pass in as input of chain
 * @op_end: node to get processed data
 * @time: the time to use for interpolatino of keyframed values
 * @rel_dim: relative dimension to scale rel suffixed values by
 * @path_root: path in filesystem to use as relative root
 * @error: error for signalling parsing errors
 *
 * Create a node chain from an unparsed commandline string.
 */
void gegl_create_chain (const char *ops,
                        GeglNode   *op_start,
                        GeglNode   *op_end,
                        double      time,
                        int         rel_dim,
                        const char *path_root,
                        GError    **error);

/**
 * gegl_serialize:
 * @start: first node in chain to serialize
 * @end: last node in chain to serialize
 * @basepath: top-level absolute path to turn into relative root
 * @serialize_flags: anded together combination of:
 * GEGL_SERIALIZE_TRIM_DEFAULTS, GEGL_SERIALIZE_VERSION, GEGL_SERIALIZE_INDENT.
 */

gchar *gegl_serialize  (GeglNode         *start,
                        GeglNode         *end,
                        const char       *basepath,
                        GeglSerializeFlag serialize_flags);

/* gegl_node_new_from_serialized:
 * @chaindata: string of chain serialized to parse.
 * @path_root: absolute file system root to use as root for relative paths.
 *
 * create a composition from chain serialization, creating end-points for
 * chain as/if needed.
 */
GeglNode *gegl_node_new_from_serialized (const gchar *chaindata,
                                         const gchar *path_root);



/**
 * gegl_node_set_time:
 * @node: a a GeglNode
 * @time: the time to set the properties which have keyfraes attached to
 *
 * Sets the right value in animated properties of this node and all its
 * dependendcies to be the specified time position.
 */

void gegl_node_set_time (GeglNode   *node,
                         double      time);


/**
 * gegl_buffer_set_color:
 * @buffer: a #GeglBuffer
 * @rect: a rectangular region to fill with a color.
 * @color: the GeglColor to fill with.
 *
 * Sets the region covered by rect to the specified color.
 */
void            gegl_buffer_set_color         (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               GeglColor           *color);

const Babl *gegl_babl_variant (const Babl *format, GeglBablVariant variant);


G_END_DECLS

#endif /* __GEGL_UTILS_H__ */

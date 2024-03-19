/* This file is the public GEGL API
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
 * 2000-2008 © Calvin Williamson, Øyvind Kolås.
 */

#ifndef __GEGL_OPERATIONS_UTIL_H__
#define __GEGL_OPERATIONS_UTIL_H__

#if !defined (__GEGL_H_INSIDE__)
#error "This file can not be included directly, use <gegl.h> instead."
#endif

G_BEGIN_DECLS

/***
 * Available operations:
 * Gegl provides means to check for available processing operations that
 * can be used with nodes using #gegl_list_operations and #gegl_has_operation.
 * For a specified op, you can get a list of properties with
 * #gegl_operation_list_properties.
 */

/**
 * gegl_list_operations:
 * @n_operations_p: (out caller-allocates): return location for number of operations.
 *
 * Return value: (transfer container) (array length=n_operations_p): An
 * alphabetically sorted array of available operation names. This excludes any
 * compat-name registered by operations. The list should be freed with g_free
 * after use.
 * ---
 * gchar **operations;
 * guint   n_operations;
 * gint i;
 *
 * operations = gegl_list_operations (&n_operations);
 * g_print ("Available operations:\n");
 * for (i=0; i < n_operations; i++)
 *   {
 *     g_print ("\t%s\n", operations[i]);
 *   }
 * g_free (operations);
 */
gchar        **gegl_list_operations         (guint *n_operations_p);

/**
 * gegl_has_operation:
 * @operation_type: the name of the operation
 *
 * Return value: A boolean telling whether the operation is present or not. This
 * also returns true for any compat-name registered by operations.
 */

gboolean       gegl_has_operation           (const gchar *operation_type);

/**
 * gegl_operation_list_properties:
 * @operation_type: the name of the operation type we want to query to properties of.
 * @n_properties_p: (out caller-allocates): return location for number of properties.
 *
 * This function will return all properties in the last version of
 * @operation_type. If you are looking for the property list of a specific
 * version of the same operation, call [method@Gegl.Node.list_properties]
 * instead on the node where the operation version was set.
 *
 * Returns: (transfer container) (array length=n_properties_p): An
 *          allocated array of [class@GObject.ParamSpec] describing the
 *          properties of the operation available when a node has
 *          @operation_type set. The array should be freed with `g_free` after
 *          use.
 */
GParamSpec** gegl_operation_list_properties (const gchar   *operation_type,
                                             guint         *n_properties_p);


/**
 * gegl_operation_find_property::
 * @operation_type: the name of the operation type we want to locate a property on.
 * @property_name: the name of the property we seek.
 *
 * This function will search a property named @property_name in the last
 * version of @operation_type. If you are looking for a property on a specific
 * version of the same operation, call
 * [func@Gegl.Operation.find_property_for_version] instead.
 *
 * Returns: (transfer none): The paramspec of the matching property - or
 *          %NULL if there are no match.
 */
GParamSpec * gegl_operation_find_property (const gchar *operation_type,
                                           const gchar *property_name);

/**
 * gegl_operation_find_property_for_version:
 * @operation_name: the name of the operation type we want to locate a property on.
 * @property_name: the name of the property we seek.
 * @op_version: the version of @operation_name which @property pertains to.
 *
 * This function should be used instead of [func@Gegl.Operation.find_property]
 * when looking up a property for a specific version of this operation.
 *
 * Returns: (transfer none): The paramspec of the matching property - or
 *          %NULL if there are no match.
 */
GParamSpec * gegl_operation_find_property_for_version (const gchar *operation_name,
                                                       const gchar *property_name,
                                                       const gchar *op_version);

/**
 * gegl_operation_get_property_key:
 * @operation_type: the name of the operation type we want to query to property keys for.
 * @property_name: the property to query a key for.
 * @property_key_name: the property mata data key to query
 *
 * Return value: NULL or a string with the meta-data value for the operation
 * key.
 */
const gchar *
gegl_operation_get_property_key (const gchar *operation_type,
                                 const gchar *property_name,
                                 const gchar *property_key_name);

/**
 * gegl_operation_list_property_keys:
 * @operation_type: the name of the operation type we want to query to property keys for.
 * @property_name: the property to query a key for.
 * @n_keys: (out caller-allocates): return location for number of property
 * keys.
 *
 * Return value: (transfer container) (array length=n_keys): An allocated NULL terminated array of property-key names. The list should be freed with g_free after use.
 */
gchar **
gegl_operation_list_property_keys (const gchar *operation_type,
                                   const gchar *property_name,
                                   guint       *n_keys);

const gchar *gegl_param_spec_get_property_key (GParamSpec  *pspec,
                                               const gchar *key_name);

void         gegl_param_spec_set_property_key (GParamSpec  *pspec,
                                               const gchar *key_name,
                                               const gchar *value);

static inline gdouble
gegl_coordinate_relative_to_pixel (gdouble relative, gdouble pixel_dim)
{
    return relative * pixel_dim;
}

static inline gdouble
gegl_coordinate_pixel_to_relative (gdouble pixel, gdouble pixel_dim)
{
    return pixel / pixel_dim;
}


/**
 * gegl_operation_list_keys:
 * @operation_type: the name of the operation type we want to query to property keys for.
 * @n_keys: (out caller-allocates): return location for number of property keys.
 *
 * Return value: (transfer container) (array length=n_keys): An allocated NULL
 * terminated array of operation-key names. The list should be freed with g_free after use.
 */
gchar      ** gegl_operation_list_keys         (const gchar *operation_type,
                                                guint       *n_keys);

const gchar * gegl_operation_get_key            (const gchar *operation_type,
                                                 const gchar *key_name);

/**
 * gegl_operation_get_op_version:
 * @operation_name: the name of the operation type we want to know the version of.
 *
 * Gets the current version of @operation_name (i.e. the default version for a
 * node of this operation type, if no "op-version" is set explicitly).
 *
 * To know the operation version actually set on a [class@Node], use
 * [method@Gegl.Node.get_op_version] instead.
 *
 * Returns: the default version for this operation.
 */
const char  * gegl_operation_get_op_version         (const gchar  *operation_name);

/**
 * gegl_operation_get_supported_versions:
 * @operation_name: the name of the operation
 *
 * Returns: (array zero-terminated=1) (transfer full): the list of supported
 *          versions. The order matters (first is older, last is current version).
 */
gchar      ** gegl_operation_get_supported_versions (const gchar *operation_name);

/**
 * gegl_operation_is_supported_version:
 * @operation_name: the name of the operation
 * @op_version:     a version string.
 *
 * Returns: a boolean telling whether the operation can run with arguments of @op_version.
 */
gboolean      gegl_operation_is_supported_version   (const gchar *operation_name,
                                                     const gchar *op_version);


G_END_DECLS

#endif /* __GEGL_OPERATIONS_UTIL_H__ */

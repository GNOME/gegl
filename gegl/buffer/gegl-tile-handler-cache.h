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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_HANDLER_CACHE_H__
#define __GEGL_TILE_HANDLER_CACHE_H__

#include "gegl-tile-handler.h"

/***
 * GeglTileHandlerCache is a GeglTileHandler that cache recently used tiles in memory.
 */

#define GEGL_TYPE_TILE_HANDLER_CACHE            (gegl_tile_handler_cache_get_type ())
#define GEGL_TILE_HANDLER_CACHE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_HANDLER_CACHE, GeglTileHandlerCache))
#define GEGL_TILE_HANDLER_CACHE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_HANDLER_CACHE, GeglTileHandlerCacheClass))
#define GEGL_IS_TILE_HANDLER_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_HANDLER_CACHE))
#define GEGL_IS_TILE_HANDLER_CACHE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_HANDLER_CACHE))
#define GEGL_TILE_HANDLER_CACHE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_HANDLER_CACHE, GeglTileHandlerCacheClass))


typedef struct _GeglTileHandlerCache      GeglTileHandlerCache;
typedef struct _GeglTileHandlerCacheClass GeglTileHandlerCacheClass;

struct _GeglTileHandlerCache
{
  GeglTileHandler  parent_instance;
  GeglTileStorage *tile_storage;
  GList            link;
  GHashTable      *items;
  GQueue           queue;
  guintptr         time;
  guintptr         stamp;
};

struct _GeglTileHandlerCacheClass
{
  GeglTileHandlerClass parent_class;
};

GType             gegl_tile_handler_cache_get_type           (void) G_GNUC_CONST;

GeglTileHandler * gegl_tile_handler_cache_new                (void);

void              gegl_tile_handler_cache_connect            (GeglTileHandlerCache *cache);
void              gegl_tile_handler_cache_disconnect         (GeglTileHandlerCache *cache);

void              gegl_tile_handler_cache_insert             (GeglTileHandlerCache *cache,
                                                              GeglTile             *tile,
                                                              gint                  x,
                                                              gint                  y,
                                                              gint                  z);
void              gegl_tile_handler_cache_remove             (GeglTileHandlerCache *cache,
                                                              gint                  x,
                                                              gint                  y,
                                                              gint                  z);
GeglTile        * gegl_tile_handler_cache_get_tile           (GeglTileHandlerCache *cache,
                                                              gint                  x,
                                                              gint                  y,
                                                              gint                  z);
void              gegl_tile_handler_cache_tile_uncloned      (GeglTileHandlerCache *cache,
                                                              GeglTile             *tile);

gsize             gegl_tile_handler_cache_get_total              (void);
gsize             gegl_tile_handler_cache_get_total_max          (void);
gsize             gegl_tile_handler_cache_get_total_uncompressed (void);
gint              gegl_tile_handler_cache_get_hits               (void);
gint              gegl_tile_handler_cache_get_misses             (void);

void              gegl_tile_handler_cache_reset_stats            (void);

#endif

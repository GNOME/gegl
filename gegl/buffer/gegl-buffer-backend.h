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
 * Copyright 2006-2008 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_BUFFER_BACKEND_H__
#define __GEGL_BUFFER_BACKEND_H__


G_BEGIN_DECLS

typedef struct _GeglTile                  GeglTile;
typedef struct _GeglTileSource            GeglTileSource;
typedef struct _GeglTileHandler           GeglTileHandler;


typedef void   (*GeglTileCallback)       (GeglTile *tile,
                                          gpointer user_data);



/* All commands have the ability to pass commands to all tiles the handlers
 * add abstraction to the commands the documentaiton given here is valid
 * when the commands are issued to a full blown GeglBuffer instance.
 */
typedef enum
{
  GEGL_TILE_IDLE = 0,
  GEGL_TILE_SET,
  GEGL_TILE_GET,
  GEGL_TILE_IS_CACHED,
  GEGL_TILE_EXIST,
  GEGL_TILE_VOID,
  GEGL_TILE_FLUSH,
  GEGL_TILE_REFETCH,
  GEGL_TILE_REINIT,

  _GEGL_TILE_LAST_0_4_8_COMMAND,

  GEGL_TILE_COPY = _GEGL_TILE_LAST_0_4_8_COMMAND,

  GEGL_TILE_LAST_COMMAND
} GeglTileCommand;

typedef struct _GeglTileCopyParams
{
  GeglBuffer *dst_buffer;

  gint        dst_x;
  gint        dst_y;
  gint        dst_z;
} GeglTileCopyParams;

G_END_DECLS

#include "gegl-buffer.h"
#include "gegl-buffer-enums.h"
#include "gegl-tile-backend.h"
#include "gegl-tile-source.h"
#include "gegl-tile-handler.h"
#include "gegl-tile.h"

#endif

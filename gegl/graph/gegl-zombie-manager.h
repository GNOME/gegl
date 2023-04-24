#pragma once

#include <stdbool.h>
#include <glib.h>
#include "gegl-buffer-backend.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GeglZombieManager GeglZombieManager;
typedef struct _GeglCache GeglCache;
typedef struct _GeglNode GeglNode;

bool use_zombie();

void gegl_zombie_link_test();

GeglZombieManager* make_zombie_manager(GeglNode*);

void destroy_zombie_manager(GeglZombieManager*);

void zombie_manager_prepare(GeglZombieManager*);

void zombie_manager_commit(GeglZombieManager*,
                           GeglBuffer* buffer,
                           const GeglRectangle *roi,
                           gint                level);

gpointer zombie_manager_command (GeglZombieManager *self,
                                 GeglTileCommand   command,
                                 gint              x,
                                 gint              y,
                                 gint              z,
                                 gpointer          data);

void zombie_manager_set_cache (GeglZombieManager *self,
                               GeglCache* cache);

#ifdef __cplusplus
}
#endif

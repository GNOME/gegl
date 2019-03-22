#ifndef MRG_GEGL_H
#define MRG_GEGL_H

#include <mrg/mrg.h>
#include <gegl/gegl.h>

void mrg_gegl_buffer_blit (Mrg *mrg,
                           float x0, float y0,
                           float width, float height,
                           GeglBuffer *buffer,
                           float u, float v,
                           float scale,
                           float preview_multiplier,
                           int   nearest_neighbor,
                           int   color_manage);


void mrg_gegl_blit (Mrg *mrg,
                    float x0, float y0,
                    float width, float height,
                    GeglNode *node,
                    float u, float v,
                    float scale,
                    float preview_multiplier,
                    int   nearest_neighbor,
                    int   color_manage);


void mrg_gegl_dirty (void);

#endif

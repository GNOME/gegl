#include "babl/babl.h"

#include "gegl-config.h"

#define KERNEL_WIDTH  5
#define KERNEL_HEIGHT 7

#define OUT_ROI_WIDTH  gegl_config()->tile_width * 2
#define OUT_ROI_HEIGHT gegl_config()->tile_height * 2

/* Add an extra border to the input buffer (beyond what the out ROI is)
 * iterator ROI that will be filled according to the used abyss policy. */
#define IN_ROI_EXTRA_WIDTH  7
#define IN_ROI_EXTRA_HEIGHT 7

#include "buffer_iterator_halo.h"

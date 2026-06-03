#include "babl/babl.h"

#include "gegl-config.h"

#define KERNEL_WIDTH  3
#define KERNEL_HEIGHT 3

#define OUT_ROI_WIDTH  gegl_config()->tile_width / 2
#define OUT_ROI_HEIGHT gegl_config()->tile_height / 2

/* Add an extra border to the input buffer iterator ROI. */
#define IN_ROI_EXTRA_WIDTH  4
#define IN_ROI_EXTRA_HEIGHT 4

#include "buffer_iterator_halo.h"

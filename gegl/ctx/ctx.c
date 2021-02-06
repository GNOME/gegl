#include <stdint.h>
#include <termios.h>
#include <unistd.h>

#include <babl/babl.h>

#define CTX_PARSER               1
#define CTX_FORMATTER            1
#define CTX_EVENTS               1
#define CTX_BITPACK_PACKER       0 // turned of due to asan report
#define CTX_GRADIENT_CACHE       1
#define CTX_ENABLE_CMYK          1
#define CTX_ENABLE_CM            1
#define CTX_RASTERIZER_AA        5
#define CTX_FORCE_AA             1
#define CTX_STRINGPOOL_SIZE      10000 // for misc storage with compressed/
                                       // variable size for each save|restore
#define CTX_SHAPE_CACHE          0
#define CTX_IMPLEMENTATION 1

#include "ctx.h"

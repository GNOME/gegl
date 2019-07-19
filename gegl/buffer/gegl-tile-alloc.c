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
 * Copyright 2019 Ell
 */

#include "config.h"

#include <math.h>
#include <string.h>

#ifdef HAVE_MALLOC_TRIM
#include <malloc.h>
#endif

#include <glib-object.h>

#include "gegl-buffer-config.h"
#include "gegl-memory.h"
#include "gegl-memory-private.h"
#include "gegl-tile-alloc.h"


#define GEGL_TILE_MIN_SIZE            sizeof (gpointer)
#define GEGL_TILE_MAX_SIZE_LOG2       24
#define GEGL_TILE_MAX_SIZE            (1 << GEGL_TILE_MAX_SIZE_LOG2)

#define GEGL_TILE_BUFFER_DATA_OFFSET  GEGL_ALIGNMENT

#define GEGL_TILE_BLOCK_BUFFER_OFFSET GEGL_ALIGN (sizeof (GeglTileBlock))
#define GEGL_TILE_BLOCK_SIZE_RATIO    0.01
#define GEGL_TILE_BLOCK_MAX_BUFFERS   1024
#define GEGL_TILE_BLOCKS_PER_TRIM     10
#define GEGL_TILE_SENTINEL_BLOCK      ((GeglTileBlock *) ~(guintptr) 0)


/*  private types  */

typedef struct _GeglTileBuffer GeglTileBuffer;
typedef struct _GeglTileBlock  GeglTileBlock;

struct _GeglTileBuffer
{
  GeglTileBlock *block;
};

G_STATIC_ASSERT (sizeof (GeglTileBuffer) + 2 * sizeof (gint) <=
                 GEGL_TILE_BUFFER_DATA_OFFSET);

struct _GeglTileBlock
{
  GeglTileBlock * volatile *block_ptr;
  guintptr                  size;

  GeglTileBuffer           *head;
  gint                      n_allocated;

  GeglTileBlock            *next;
  GeglTileBlock            *prev;
};


/*  local function prototypes  */

static gint                    gegl_tile_log2i            (guint                      n);

static GeglTileBlock         * gegl_tile_block_new        (GeglTileBlock * volatile  *block_ptr,
                                                           gsize                      size);
static void                    gegl_tile_block_free       (GeglTileBlock             *block,
                                                           GeglTileBlock            **head_block);

static inline gpointer         gegl_tile_buffer_to_data   (GeglTileBuffer            *buffer);
static inline GeglTileBuffer * gegl_tile_buffer_from_data (gpointer                   data);

static gpointer                gegl_tile_alloc_fallback   (gsize                      size);


/*  local variables  */

static const gint     gegl_tile_divisors[] = {1, 3, 5};
static GeglTileBlock *gegl_tile_blocks[G_N_ELEMENTS (gegl_tile_divisors)]
                                      [GEGL_TILE_MAX_SIZE_LOG2];
static gint           gegl_tile_n_blocks;
static gint           gegl_tile_max_n_blocks;

static guintptr       gegl_tile_alloc_total;


/*  private functions  */

#ifdef HAVE___BUILTIN_CLZ

static gint
gegl_tile_log2i (guint n)
{
  return 8 * sizeof (guint) - __builtin_clz (n) - 1;
}

#else /* ! HAVE___BUILTIN_CLZ */

static gint
gegl_tile_log2i (guint n)
{
  gint result = 0;
  gint i;

  for (i = 8 * sizeof (guint) / 2; i; i /= 2)
    {
      guint m = n >> i;

      if (m)
        {
          n = m;

          result |= i;
        }
    }

  return result;
}

#endif /* HAVE___BUILTIN_CLZ */

static GeglTileBlock *
gegl_tile_block_new (GeglTileBlock * volatile *block_ptr,
                     gsize                     size)
{
  GeglTileBlock   *block;
  GeglTileBuffer  *buffer;
  GeglTileBuffer **next_buffer;
  gsize            block_size;
  gsize            buffer_size;
  gsize            n_buffers;
  gint             n_blocks;
  gint             i;

  buffer_size = GEGL_TILE_BUFFER_DATA_OFFSET + GEGL_ALIGN (size);

  block_size  = floor (gegl_buffer_config ()->tile_cache_size *
                       GEGL_TILE_BLOCK_SIZE_RATIO);
  block_size -= block_size % buffer_size;

  n_buffers = block_size / buffer_size;
  n_buffers = MIN (n_buffers, GEGL_TILE_BLOCK_MAX_BUFFERS);

  if (n_buffers <= 1)
    return NULL;

  block_size = n_buffers * buffer_size;

  block = gegl_try_malloc (GEGL_TILE_BLOCK_BUFFER_OFFSET + block_size);

  if (! block)
    return NULL;

  block->block_ptr   = block_ptr;
  block->size        = GEGL_TILE_BLOCK_BUFFER_OFFSET + block_size;

  block->head        = (GeglTileBuffer *) ((guint8 *) block +
                                           GEGL_TILE_BLOCK_BUFFER_OFFSET);
  block->n_allocated = 0;

  block->prev        = NULL;
  block->next        = NULL;

  buffer = block->head;

  for (i = n_buffers; i; i--)
    {
      buffer->block = block;

      next_buffer = gegl_tile_buffer_to_data (buffer);

      if (i > 1)
        buffer = (GeglTileBuffer *) ((guint8 *) buffer + buffer_size);
      else
        buffer = NULL;

      *next_buffer = buffer;
    }

  n_blocks = g_atomic_int_add (&gegl_tile_n_blocks, +1) + 1;

  if (n_blocks % GEGL_TILE_BLOCKS_PER_TRIM == 0)
    gegl_tile_max_n_blocks = MAX (gegl_tile_max_n_blocks, n_blocks);

  g_atomic_pointer_add (&gegl_tile_alloc_total, +block->size);

  return block;
}

static void
gegl_tile_block_free (GeglTileBlock  *block,
                      GeglTileBlock **head_block)
{
  guintptr block_size = block->size;
  gint     n_blocks;

  if (block->prev)
    block->prev->next = block->next;
  else
    *head_block = block->next;

  if (block->next)
    block->next->prev = block->prev;

  gegl_free (block);

  n_blocks = g_atomic_int_add (&gegl_tile_n_blocks, -1) - 1;

  g_atomic_pointer_add (&gegl_tile_alloc_total, -block_size);

#ifdef HAVE_MALLOC_TRIM
  if (gegl_tile_max_n_blocks - n_blocks >= GEGL_TILE_BLOCKS_PER_TRIM)
    {
      gegl_tile_max_n_blocks = (n_blocks + (GEGL_TILE_BLOCKS_PER_TRIM - 1)) /
                               GEGL_TILE_BLOCKS_PER_TRIM                    *
                               GEGL_TILE_BLOCKS_PER_TRIM;

      malloc_trim (block_size);
    }
#endif
}

static inline gpointer
gegl_tile_buffer_to_data (GeglTileBuffer *buffer)
{
  return (guint8 *) buffer + GEGL_TILE_BUFFER_DATA_OFFSET;
}

static inline GeglTileBuffer *
gegl_tile_buffer_from_data (gpointer data)
{
  return (GeglTileBuffer *) ((guint8 *) data - GEGL_TILE_BUFFER_DATA_OFFSET);
}

static gpointer
gegl_tile_alloc_fallback (gsize size)
{
  GeglTileBuffer *buffer = gegl_malloc (GEGL_TILE_BUFFER_DATA_OFFSET + size);

  buffer->block = NULL;

  return gegl_tile_buffer_to_data (buffer);
}


/*  public functions  */

gpointer
gegl_tile_alloc (gsize size)
{
  GeglTileBlock * volatile  *block_ptr;
  GeglTileBlock             *block;
  GeglTileBuffer            *buffer;
  GeglTileBuffer           **next_buffer;
  gint                       n;
  gint                       i;
  gint                       j;

  if (size > GEGL_TILE_MAX_SIZE)
    return gegl_tile_alloc_fallback (size);

  size = MAX (size, GEGL_TILE_MIN_SIZE);

  n = size;

  for (i = G_N_ELEMENTS (gegl_tile_divisors) - 1; i; i--)
    {
      if (size % gegl_tile_divisors[i] == 0)
        {
          n /= gegl_tile_divisors[i];

          break;
        }
    }

  if (n & (n - 1))
    return gegl_tile_alloc_fallback (size);

  j = gegl_tile_log2i (n);

  block_ptr = &gegl_tile_blocks[i][j];

  do
    {
      block = *block_ptr;
    }
  while (block == GEGL_TILE_SENTINEL_BLOCK ||
         ! g_atomic_pointer_compare_and_exchange (block_ptr,
                                                  block,
                                                  GEGL_TILE_SENTINEL_BLOCK));

  if (! block)
    {
      block = gegl_tile_block_new (block_ptr, size);

      if (! block)
        {
          g_atomic_pointer_set (block_ptr, block);

          return gegl_tile_alloc_fallback (size);
        }
    }

  buffer      = block->head;
  next_buffer = gegl_tile_buffer_to_data (buffer);

  block->head = *next_buffer;
  block->n_allocated++;

  if (! block->head)
    {
      if (block->next)
        block->next->prev = NULL;

      block = block->next;
    }

  g_atomic_pointer_set (block_ptr, block);

  return gegl_tile_buffer_to_data (buffer);
}

gpointer
gegl_tile_alloc0 (gsize size)
{
  gpointer result = gegl_tile_alloc (size);

  memset (result, 0, size);

  return result;
}

void
gegl_tile_free (gpointer ptr)
{
  GeglTileBlock * volatile *block_ptr;
  GeglTileBlock            *block;
  GeglTileBlock            *head_block;
  GeglTileBuffer           *buffer;

  if (! ptr)
    return;

  buffer = gegl_tile_buffer_from_data (ptr);

  if (! buffer->block)
    {
      gegl_free (buffer);

      return;
    }

  block     = buffer->block;
  block_ptr = block->block_ptr;

  do
    {
      head_block = *block_ptr;
    }
  while (head_block == GEGL_TILE_SENTINEL_BLOCK ||
         ! g_atomic_pointer_compare_and_exchange (block_ptr,
                                                  head_block,
                                                  GEGL_TILE_SENTINEL_BLOCK));

  block->n_allocated--;

  if (block->n_allocated == 0)
    {
      gegl_tile_block_free (block, &head_block);
    }
  else
    {
      GeglTileBuffer **next_buffer = gegl_tile_buffer_to_data (buffer);

      *next_buffer = block->head;

      if (! block->head)
        {
          block->prev = NULL;
          block->next = head_block;

          if (head_block)
            head_block->prev = block;

          head_block = block;
        }

      block->head = buffer;
    }

  g_atomic_pointer_set (block_ptr, head_block);
}


/*  public functions (stats)  */

guint64
gegl_tile_alloc_get_total (void)
{
  return gegl_tile_alloc_total;
}

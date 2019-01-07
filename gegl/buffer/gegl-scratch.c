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

#include <string.h>

#include <glib-object.h>

#include "gegl-memory-private.h"
#include "gegl-scratch.h"
#include "gegl-scratch-private.h"


#define GEGL_SCRATCH_ALIGNMENT         GEGL_ALIGN
#define GEGL_SCRATCH_MAX_BLOCK_SIZE    (1 << 20)
#define GEGL_SCRATCH_BLOCK_DATA_OFFSET ((sizeof (GeglScratchBlockHeader) + \
                                         (GEGL_SCRATCH_ALIGNMENT - 1))   / \
                                        GEGL_SCRATCH_ALIGNMENT           * \
                                        GEGL_SCRATCH_ALIGNMENT)


G_STATIC_ASSERT (GEGL_SCRATCH_ALIGNMENT <= G_MAXUINT8);


/*  private types  */

typedef struct _GeglScratchBlockHeader GeglScratchBlockHeader;
typedef struct _GeglScratchBlock       GeglScratchBlock;
typedef struct _GeglScratchContext     GeglScratchContext;

struct _GeglScratchBlockHeader
{
  GeglScratchContext *context;
  gsize               size;
  guint8              offset;
};

struct _GeglScratchBlock
{
  GeglScratchBlockHeader header;
  guint8                 padding[GEGL_SCRATCH_BLOCK_DATA_OFFSET -
                                 sizeof (GeglScratchBlockHeader)];
  guint8                 data[];
};

struct _GeglScratchContext
{
  GeglScratchBlock **blocks;
  gint               n_blocks;
  gint               n_available_blocks;
};


/*  local function prototypes  */

static GeglScratchContext * gegl_scratch_context_new  (void);
static void                 gegl_scratch_context_free (GeglScratchContext *context);

static GeglScratchBlock   * gegl_scratch_block_new    (GeglScratchContext *context,
                                                       gsize               size);
static void                 gegl_scratch_block_free   (GeglScratchBlock   *block);


/*  local variables  */

static GPrivate                 gegl_scratch_context = G_PRIVATE_INIT (
  (GDestroyNotify) gegl_scratch_context_free);
static const GeglScratchContext void_context;
static volatile guintptr        gegl_scratch_total;


/*  private functions  */

GeglScratchContext *
gegl_scratch_context_new (void)
{
  return g_slice_new0 (GeglScratchContext);
}

void
gegl_scratch_context_free (GeglScratchContext *context)
{
  gint i;

  for (i = 0; i < context->n_available_blocks; i++)
    gegl_scratch_block_free (context->blocks[i]);

  g_free (context->blocks);

  g_slice_free (GeglScratchContext, context);
}

GeglScratchBlock *
gegl_scratch_block_new (GeglScratchContext *context,
                        gsize               size)
{
  GeglScratchBlock *block;
  gint              offset;

  g_atomic_pointer_add (&gegl_scratch_total, +size);

  block = g_malloc ((GEGL_SCRATCH_ALIGNMENT - 1) +
                    sizeof (GeglScratchBlock)    +
                    size);

  offset  = GEGL_SCRATCH_ALIGNMENT -
            ((guintptr) block) % GEGL_SCRATCH_ALIGNMENT;
  offset %= GEGL_SCRATCH_ALIGNMENT;

  block = (GeglScratchBlock *) ((guint8 *) block + offset);

  block->header.context = context;
  block->header.size    = size;
  block->header.offset  = offset;

  return block;
}

void
gegl_scratch_block_free (GeglScratchBlock *block)
{
  g_atomic_pointer_add (&gegl_scratch_total, -block->header.size);

  g_free ((guint8 *) block - block->header.offset);
}


/*  public functions  */

gpointer
gegl_scratch_alloc (gsize size)
{
  GeglScratchContext *context;
  GeglScratchBlock   *block;

  if (G_UNLIKELY (! size))
    return NULL;

  if (G_UNLIKELY (size > GEGL_SCRATCH_MAX_BLOCK_SIZE))
    {
      block = gegl_scratch_block_new ((GeglScratchContext *) &void_context,
                                      size);

      return block->data;
    }

  context = g_private_get (&gegl_scratch_context);

  if (G_UNLIKELY (! context))
    {
      context = gegl_scratch_context_new ();

      g_private_set (&gegl_scratch_context, context);
    }

  if (G_LIKELY (context->n_available_blocks))
    {
      block = context->blocks[--context->n_available_blocks];

      if (G_LIKELY (size <= block->header.size))
        return block->data;

      gegl_scratch_block_free (block);
    }

  block = gegl_scratch_block_new (context, size);

  return block->data;
}

gpointer
gegl_scratch_alloc0 (gsize size)
{
  gpointer ptr;

  if (G_UNLIKELY (! size))
    return NULL;

  ptr = gegl_scratch_alloc (size);

  memset (ptr, 0, size);

  return ptr;
}

void
gegl_scratch_free (gpointer ptr)
{
  GeglScratchContext *context;
  GeglScratchBlock   *block;

  if (G_UNLIKELY (! ptr))
    return;

  context = g_private_get (&gegl_scratch_context);
  block   = (GeglScratchBlock *) ((guint8 *) ptr -
                                  GEGL_SCRATCH_BLOCK_DATA_OFFSET);

  if (G_UNLIKELY (block->header.context != context))
    {
      gegl_scratch_block_free (block);

      return;
    }

  if (G_UNLIKELY (context->n_available_blocks == context->n_blocks))
    {
      context->n_blocks = MAX (2 * context->n_blocks, 1);
      context->blocks   = g_renew (GeglScratchBlock *, context->blocks,
                                   context->n_blocks);
    }

  context->blocks[context->n_available_blocks++] = block;
}


/*   public functions (stats)  */

guint64
gegl_scratch_get_total (void)
{
  return gegl_scratch_total;
}

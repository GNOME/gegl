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


#define GEGL_SCRATCH_MAX_BLOCK_SIZE    (1 << 20)
#define GEGL_SCRATCH_BLOCK_DATA_OFFSET GEGL_ALIGN (sizeof (GeglScratchBlock))


G_STATIC_ASSERT (GEGL_ALIGNMENT <= G_MAXUINT8);


/*  private types  */

typedef struct _GeglScratchBlock   GeglScratchBlock;
typedef struct _GeglScratchContext GeglScratchContext;

struct _GeglScratchBlock
{
  GeglScratchContext *context;
  gsize               size;
  guint8              offset;
};

struct _GeglScratchContext
{
  GeglScratchBlock **blocks;
  gint               n_blocks;
  gint               n_available_blocks;
};


/*  local function prototypes  */

static GeglScratchContext      * gegl_scratch_context_new     (void);
static void                      gegl_scratch_context_free    (GeglScratchContext *context);

static GeglScratchBlock        * gegl_scratch_block_new       (GeglScratchContext *context,
                                                               gsize               size);
static void                      gegl_scratch_block_free      (GeglScratchBlock   *block);

static inline gpointer           gegl_scratch_block_to_data   (GeglScratchBlock   *block);
static inline GeglScratchBlock * gegl_scratch_block_from_data (gpointer            data);


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

  block = g_malloc ((GEGL_ALIGNMENT - 1)           +
                    GEGL_SCRATCH_BLOCK_DATA_OFFSET +
                    size);

  offset = GEGL_ALIGN ((guintptr) block) - (guintptr) block;

  block = (GeglScratchBlock *) ((guint8 *) block + offset);

  block->context = context;
  block->size    = size;
  block->offset  = offset;

  return block;
}

void
gegl_scratch_block_free (GeglScratchBlock *block)
{
  g_atomic_pointer_add (&gegl_scratch_total, -block->size);

  g_free ((guint8 *) block - block->offset);
}

static inline gpointer
gegl_scratch_block_to_data (GeglScratchBlock *block)
{
  return (guint8 *) block + GEGL_SCRATCH_BLOCK_DATA_OFFSET;
}

static inline GeglScratchBlock *
gegl_scratch_block_from_data (gpointer data)
{
  return (GeglScratchBlock *) ((guint8 *) data -
                               GEGL_SCRATCH_BLOCK_DATA_OFFSET);
}


/*  public functions  */

gpointer
gegl_scratch_alloc (gsize size)
{
  GeglScratchContext *context;
  GeglScratchBlock   *block;

  if (G_UNLIKELY (size > GEGL_SCRATCH_MAX_BLOCK_SIZE))
    {
      block = gegl_scratch_block_new ((GeglScratchContext *) &void_context,
                                      size);

      return gegl_scratch_block_to_data (block);
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

      if (G_LIKELY (size <= block->size))
        return gegl_scratch_block_to_data (block);

      gegl_scratch_block_free (block);
    }

  block = gegl_scratch_block_new (context, size);

  return gegl_scratch_block_to_data (block);
}

gpointer
gegl_scratch_alloc0 (gsize size)
{
  gpointer ptr;

  ptr = gegl_scratch_alloc (size);

  memset (ptr, 0, size);

  return ptr;
}

void
gegl_scratch_free (gpointer ptr)
{
  GeglScratchContext *context;
  GeglScratchBlock   *block;

  context = g_private_get (&gegl_scratch_context);
  block   = gegl_scratch_block_from_data (ptr);

  if (G_UNLIKELY (block->context != context))
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

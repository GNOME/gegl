#include "config.h"

#include <glib-object.h>
#define GEGL_ITERATOR2_API // opt in to new buffer iterator API

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-operation-point-filter.h"
#include "gegl-operation-point-composer.h"
#include "gegl-operation-point-composer3.h"
#include "gegl-operation-context.h"
#include "gegl-operation-pipeline.h"
#include "gegl-config.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include <gegl-node-private.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PIPELINE_MAX  64

typedef struct {
  GeglOperation *operation;
  int            in_pads;
  void         (*process)(void *);
  GeglBuffer    *aux;
  GeglBuffer    *aux2;
  int            aux_handle;  /* these values are deterministic */
  int            aux2_handle; /* and can scarily be shared among processes */
  const Babl    *input_fish;
  const Babl    *in_format;
  const Babl    *aux_format;
  const Babl    *aux2_format;
  const Babl    *out_format;
} PipeEntry;


/* first make it work for a filter op masking its own output as
 * a single entry pipeline
 *
 * then generalize/expand
 */


struct _GeglOperationPipeLine {
  int entries;
  GeglBuffer *input;
  int buffers_used;
  PipeEntry entry[PIPELINE_MAX];
};

/* returns true if the passed op could be part of a pipeline
 */
gboolean gegl_operation_is_pipelinable (GeglOperation *op)
{
  gboolean ret = GEGL_IS_OPERATION_POINT_FILTER (op) ||
                 GEGL_IS_OPERATION_POINT_COMPOSER (op) ||
                 GEGL_IS_OPERATION_POINT_COMPOSER3 (op);
  if (ret)
  {
    GeglOperationClass *op_klass = GEGL_OPERATION_GET_CLASS (op);
    if (op_klass->want_in_place == FALSE)
      return FALSE;
  }
  if (0 && ret)
  {
    const char *name = gegl_operation_get_name (op);
    if (!strcmp (name, "gimp:mask-components"))
      return FALSE;
  }
  return ret;
}

static GeglNode *gegl_node_get_non_nop_producer (GeglNode *n)
{
  GeglNode *node = gegl_operation_get_source_node (n->operation, "input");
  gint n_consumers;
  fprintf (stderr, "%s.\n", gegl_node_get_operation (n));
  n_consumers = gegl_node_get_consumers (node, "output", NULL, NULL);
  fprintf (stderr, ".%s %i.\n", node?gegl_node_get_operation (node):"-", n_consumers);
  while (node &&
   (!strcmp (gegl_node_get_operation (node), "gegl:nop") ||
   !strcmp (gegl_node_get_operation (node), "GraphNode"))
   && (n_consumers == 1 /*|| n_consumers ==2*/) && !
   node->priv->eval_manager)
  {
    node = gegl_operation_get_source_node (node->operation, "input");
    n_consumers = gegl_node_get_consumers (node, "output", NULL, NULL);
  }
  if (n_consumers != 1 /*||n_consumers == 2*/)
    node = NULL;
  fprintf (stderr, "..%s %i\n", node?gegl_node_get_operation (node):"-", n_consumers);
  return node;
}

GeglOperationPipeLine *
gegl_operation_pipeline_ensure (GeglOperation        *operation,
                                GeglOperationContext *context,
                                GeglBuffer           *input)
{
  GeglOperationPipeLine *pipeline = NULL;
  GeglNode *source_node;
  GeglOperationContext *source_context;
  source_node = gegl_node_get_non_nop_producer (operation->node);
  if (source_node && !source_node->priv->eval_manager)
  {
    source_context = gegl_operation_context_node_get_context (context, source_node);
    pipeline = gegl_operation_context_get_pipeline (source_context);
    gegl_operation_context_set_pipeline (source_context, NULL);
  }

  if (!pipeline)
  {
    pipeline = g_malloc0 (sizeof (GeglOperationPipeLine));
    pipeline->buffers_used = 2; // input and output
  }
  gegl_operation_context_set_pipeline (context, pipeline);

  if (gegl_operation_pipeline_get_entries (pipeline) == 0)
    gegl_operation_pipeline_set_input (pipeline, input);

  return pipeline;
}


/* we should not be intermediate if adding more bits to the pipeline
   would not work
 */
gboolean
gegl_operation_pipeline_is_intermediate_node (GeglOperation *op,
                                              GeglOperationPipeLine *pipeline)
{
  gboolean is_intermediate = TRUE;
  gint n_consumers;
  GeglNode *it = op->node;
  GeglNode **consumers = NULL;

  fprintf (stderr, "[%s\n", gegl_operation_get_name (op));
  if (op->node->priv->eval_manager)
  {
    fprintf (stderr, "chaughta a fraggel!\n");
    return FALSE;
  }

  n_consumers = gegl_node_get_consumers2 (it, "output", &consumers, NULL);

  if (n_consumers == 0)
    {
      fprintf (stderr, "[[--\n");
      is_intermediate = FALSE;
    }

  it = consumers[0];
#if 1
  while ((n_consumers == 1 /* || n_consumers==2*/)&& 
       (!strcmp (gegl_node_get_operation (consumers[0]), "gegl:nop")||
        !strcmp (gegl_node_get_operation (consumers[0]), "GraphNode")))
  {
    it = consumers[0];
    g_free (consumers);
    n_consumers = gegl_node_get_consumers2 (it, "output", &consumers, NULL);
  }
#endif

  if (n_consumers == 0 && it != op->node)
    {
      return TRUE;
    }
  else if (n_consumers == 1)
    {
      GeglOperation *sink = gegl_node_get_gegl_operation (consumers[0]);

      if (! gegl_operation_is_pipelinable (sink))
          is_intermediate = FALSE;
      else if (pipeline->entries + 1 >= PIPELINE_MAX)
          is_intermediate = FALSE;
      else
          is_intermediate = TRUE;
      fprintf (stderr, "[[%s %s\n", gegl_operation_get_name (sink), is_intermediate?"pipelineable":"not pipelineable");
    }
  else
   {
    is_intermediate = FALSE;
      fprintf (stderr, "[%s=--%i-\n",
         gegl_operation_get_name (consumers[0]->operation), n_consumers);
   }

  g_free (consumers);

  return is_intermediate;
}

static gboolean
_gegl_operation_pipeline_process (GeglOperationPipeLine  *pipeline,
                                  GeglBuffer             *output,
                                  const GeglRectangle    *result,
                                  gint                    level)
{
  GeglBufferIterator *i = gegl_buffer_iterator_new (output, result, level, pipeline->entry[pipeline->entries-1].out_format,
                                                    GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE, pipeline->buffers_used+1);
  gint input_handle = 0;
  void *temp[2]={NULL, NULL};
  glong high_tide = 0;
  void *cur_input = NULL;
  void *cur_output = NULL;
  int   buf_mod = 0;

  if (pipeline->input)
    input_handle = gegl_buffer_iterator_add (i, pipeline->input, result, level,
                                            pipeline->entry[0].in_format,
                                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  {
    gint e;
    for (e = 0; e < pipeline->entries; e++)
    {
      PipeEntry *entry = &pipeline->entry[e];
      switch (pipeline->entry[e].in_pads)
      {
        case 3:
          if (entry->aux2)
            entry->aux2_handle =
               gegl_buffer_iterator_add (i, entry->aux2, result, level,
                                         entry->aux2_format,
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        case 2:
          if (entry->aux)
            entry->aux_handle =
               gegl_buffer_iterator_add (i, entry->aux, result, level,
                                         entry->aux_format,
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE);
        case 1:
        default:
          break;
      }
    }
  }

  while (gegl_buffer_iterator_next (i))
    {
      gint e;
      if (i->length > high_tide)
      {
        if (temp[0])
          g_free (temp[0]);
        high_tide = i->length;
        temp[0] = g_malloc (4 * 8 * high_tide * 2);
        temp[1] = (char*)(temp[0]) + 4 * 8 * high_tide;
        /* the two allocations are merged into one, to reduce overhead,
           if sufficiently small for the architecture - this could use
           stack allocations
         */
      }

      for (e = 0; e < pipeline->entries; e++)
      {
        PipeEntry *entry = &pipeline->entry[e];
        if (e == 0)
          {
            if (pipeline->input)
              cur_input = i->items[input_handle].data;
          }
        else
          {
            if (!entry->input_fish)
            {
              cur_input = cur_output;
            }
            else
            {
              cur_input = temp[(buf_mod++)&1];
              babl_process (entry->input_fish,
                            cur_output, cur_input, i->length);

            }
          }
        if (pipeline->entries == e+1)
        {
          cur_output = i->items[0].data;
        }
        else
        {
          if (entry->in_format == entry->out_format)
            cur_output = cur_input;
          else
            cur_output = temp[(buf_mod++)&1];
        }

        switch (entry->in_pads)
        {
          case 0:
          {
            gboolean (*process)(GeglOperation *,
                                void *,
                                glong,
                                const GeglRectangle *,
                                gint) = (void*)entry->process;

            process (entry->operation,
                     cur_output,
                     i->length,
                     &(i->items[0].roi),
                     level);
          }break;
          case 1:
          {
            gboolean (*process)(GeglOperation *,
                                void *,
                                void *,
                                glong,
                                const GeglRectangle *,
                                gint) = (void*)entry->process;

            process (entry->operation,
                     cur_input,
                     cur_output,
                     i->length,
                     &(i->items[0].roi),
                     level);
          }break;

          case 2:
          {
            gboolean (*process)(GeglOperation *,
                                void *,
                                void *,
                                void *,
                                glong,
                                const GeglRectangle *,
                                gint) = (void*)entry->process;

            process (entry->operation,
                     cur_input,
                     entry->aux?
                       i->items[entry->aux_handle].data:
                       NULL,
                     cur_output,
                     i->length,
                     &(i->items[0].roi),
                     level);
          }break;

          case 3:
          {
            gboolean (*process)(GeglOperation *,
                                void *,
                                void *,
                                void *,
                                void *,
                                glong,
                                const GeglRectangle *,
                                gint) = (void*)entry->process;
            process (entry->operation,
                     cur_input,
                     entry->aux?
                       i->items[entry->aux_handle].data:
                       NULL,
                     entry->aux2?
                       i->items[entry->aux2_handle].data:
                       NULL,
                     cur_output,
                     i->length,
                     &(i->items[0].roi),
                     level);
          }break;
        }
      }
    }


  if (temp[0])
    g_free (temp[0]);

  return TRUE;
}

void
gegl_operation_pipeline_destroy (GeglOperationPipeLine *pipeline)
{
  g_clear_object (&pipeline->input);
  {
    gint e;
    for (e = 0; e < pipeline->entries; e++)
    {
      PipeEntry *entry = &pipeline->entry[e];

      switch (entry->in_pads)
      {
        case 3:
          if (entry->aux2)
            g_clear_object (&entry->aux2);
        case 2:
          if (entry->aux)
            g_clear_object (&entry->aux);
        case 1:
        default:
          break;
      }
    }
  }

  g_free (pipeline);
}

typedef struct ThreadData
{
  GeglOperationPipeLine *pipeline;
  GeglBuffer            *output;
  gint                  *pending;
  gint                   level;
  GeglRectangle          result;
} ThreadData;

static void thread_process (gpointer thread_data, gpointer unused)
{
  ThreadData *data = thread_data;
  _gegl_operation_pipeline_process (data->pipeline, data->output, &data->result, data->level);
  g_atomic_int_add (data->pending, -1);
}

static GThreadPool *thread_pool (void)
{
  static GThreadPool *pool = NULL;
  if (!pool)
    {
      pool =  g_thread_pool_new (thread_process, NULL, gegl_config_threads (),
                                 FALSE, NULL);
    }
  return pool;
}

gboolean
gegl_operation_pipeline_process (GeglOperationPipeLine *pipeline,
                                 GeglBuffer            *output,
                                 const GeglRectangle   *result,
                                 gint                   level)
{
  gint threads = gegl_config_threads();

  if (1) {
    gint e;
    fprintf (stderr, "{%i}", pipeline->entries);
    for (e = 0; e < pipeline->entries; e++)
    {
      PipeEntry *entry = &pipeline->entry[e];
      fprintf (stderr, "%s ", gegl_operation_get_name (entry->operation));
  }
  fprintf (stderr, "\n");
  }

  if (threads == 1 || result->width * result->height < 64*64 || result->height < threads)
  {
    return _gegl_operation_pipeline_process (pipeline, output, result, level);
  }
  else
  {
    ThreadData thread_data[threads];
    GThreadPool *pool = thread_pool ();
    gint pending;
    gint j;
    if (result->width > result->height)
          for (j = 0; j < threads; j++)
          {
            GeglRectangle rect = *result;

            rect.width /= threads;
            rect.x += rect.width * j;

            if (j == threads-1)
              rect.width = (result->width + result->x) - rect.x;

            thread_data[j].result = rect;
          }
        else
          for (j = 0; j < threads; j++)
          {
            GeglRectangle rect = *result;

            rect = *result;
            rect.height /= threads;
            rect.y += rect.height * j;

            if (j == threads-1)
              rect.height = (result->height + result->y) - rect.y;

            thread_data[j].result = rect;
          }
    for (j = 0; j < threads; j++)
     {
       thread_data[j].output = output;
       thread_data[j].pipeline = pipeline;
       thread_data[j].pending = &pending;
       thread_data[j].level = level;
     }
    pending = threads;

    // XXX cl flushes?

    for (gint j = 1; j < threads; j++)
      g_thread_pool_push (pool, &thread_data[j], NULL);
    thread_process (&thread_data[0], NULL);
    while (g_atomic_int_get (&pending)) {};

    return TRUE;
  }
}

void
gegl_operation_pipeline_add (GeglOperationPipeLine      *pipeline,
                             GeglOperation *operation,
                             gint           in_pads,
                             const Babl    *in_format,
                             const Babl    *out_format,
                             const Babl    *aux_format,
                             const Babl    *aux2_format,
                             GeglBuffer    *aux,
                             GeglBuffer    *aux2,
                             void *process)
{
  PipeEntry *entry;
  PipeEntry *prev_entry;

  g_assert (pipeline);
  g_assert (pipeline->entries + 1 <= PIPELINE_MAX);

  entry = &pipeline->entry[pipeline->entries];
  prev_entry = pipeline->entries?&pipeline->entry[pipeline->entries-1]:NULL;

  entry->operation  = operation;
  entry->in_pads    = in_pads;
  entry->in_format  = in_format;
  entry->aux_format = aux_format;
  entry->aux        = aux;
  entry->aux2_format= aux2_format;
  entry->aux2       = aux2;
  entry->out_format = out_format;
  entry->process    = process;

  if (prev_entry && entry->in_format != prev_entry->out_format)
  {
    entry->input_fish = babl_fish (prev_entry->out_format, entry->in_format);
  }

  pipeline->entries++;

  pipeline->buffers_used += ( (aux?1:0) + (aux2?1:0));
}

int gegl_operation_pipeline_get_entries (GeglOperationPipeLine *pipeline)
{
  return pipeline->entries;
}



void gegl_operation_pipeline_set_input  (GeglOperationPipeLine   *pipeline,
                                         GeglBuffer *buffer)
{
  pipeline->input = buffer;
}

#ifndef _GEGL_OPERATION_PIPELINE_H
#define _GEGL_OPERATION_PIPELINE_H


int gegl_operation_pipeline_get_entries (GeglOperationPipeLine   *pipeline);
void gegl_operation_pipeline_set_input  (GeglOperationPipeLine   *pipeline,
                                         GeglBuffer *buffer);

gboolean
gegl_operation_pipeline_is_intermediate_node (GeglOperation *op,
                                              GeglOperationPipeLine *pipeline);
gboolean gegl_operation_pipeline_is_composite_node (GeglOperation *op);

gboolean gegl_operation_pipeline_process (GeglOperationPipeLine            *pipeline,
                                          GeglBuffer          *output,
                                          const GeglRectangle *result,
                                          gint                 level);
GeglOperationPipeLine *
gegl_operation_pipeline_ensure (GeglOperation *operation,
                                GeglOperationContext *context,
                                GeglBuffer           *input);


void
gegl_operation_pipeline_add (GeglOperationPipeLine *pipeline,
                             GeglOperation *operation,
                             gint           in_pads,
                             const Babl    *in_format,
                             const Babl    *out_format,
                             const Babl    *aux_format,
                             const Babl    *aux2_format,
                             GeglBuffer    *aux,
                             GeglBuffer    *aux2,
                             void *process);

gboolean
gegl_operation_pipeline_process_threaded  (GeglOperationPipeLine            *pipeline,
                                           GeglBuffer          *output,
                                           const GeglRectangle *result,
                                           gint                 level);

gboolean gegl_operation_is_pipelinable (GeglOperation *op);
void
gegl_operation_pipeline_destroy (GeglOperationPipeLine *pipeline);

#endif

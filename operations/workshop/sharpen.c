#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (amount, _("Amount"), 10.0)
  description (_("Controls the amount of sharpness applied to an image ") )
  value_range (1.0, 99.0)
  ui_range    (1.0, 99.0)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     sharpen
#define GEGL_OP_C_SOURCE sharpen.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");

  gegl_operation_set_format (operation, "input",
                             babl_format_with_space ("RGBA float", space));
  gegl_operation_set_format (operation, "output",
                             babl_format_with_space ("RGBA float", space));
}

static gboolean
process (GeglOperation       *op,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
      GeglProperties     *o               = GEGL_PROPERTIES (op);
      const Babl         *format          = babl_format_with_space ("RGBA float",
                                                                     gegl_operation_get_source_space (op, "input"));
      gfloat              scale           = 8.0f / (1.0f - (o->amount / 100.0f));
      gfloat              neg_factor      = (scale - 8.0f) / 8.0f;

      GeglBufferIterator *iter;
      gint                x;
      gint                y;
      gfloat             *input_data;
      gfloat             *output_data;
      gint                height;
      gint                width;
      gint                c;
      gint                t; 
      gint                b;
      gfloat              pos_r;
      gfloat              pos_g;
      gfloat              pos_b;

      iter = gegl_buffer_iterator_new (output, roi, level, format,
                                       GEGL_ACCESS_WRITE,
                                       GEGL_ABYSS_NONE, 2);
      
      gegl_buffer_iterator_add (iter, input, roi, level, format, GEGL_ACCESS_READ, GEGL_ABYSS_CLAMP);

       while (gegl_buffer_iterator_next (iter))
         {
            input_data  = iter->items[1].data;
            output_data = iter->items[0].data;
            height      = iter->items[1].roi.height;
            width       = iter->items[1].roi.width;

            for (y = 0; y < height; y++)
              {
                for (x = 0; x < width; x++)
                  {
                    c = (y * width + x) * 4;
                    t = ((y-1) * width + x) * 4;
                    b = ((y+1) * width + x) * 4;
                  }
              } 
         }
      return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare        = prepare;
  operation_class->opencl_support = FALSE;
  filter_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:sharpen",
    "title",       _("Sharpen"),
    "categories",  "enhance",
    "description", _("Sharpen the input image"),
    NULL);
}

#endif
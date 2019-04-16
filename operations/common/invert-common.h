#ifndef PROCESS_MODEL_SUFFIX

#define INVERT_CAT(x, y)   INVERT_CAT_I (x, y)
#define INVERT_CAT_I(x, y) x ## y

typedef gboolean (* ProcessFunc) (GeglOperation       *op,
                                  void                *in_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level);

#define PROCESS_FUNC(model_suffix, type_suffix) \
  INVERT_CAT (process_, INVERT_CAT (model_suffix, INVERT_CAT (_, type_suffix)))

static gboolean
process_int (GeglOperation       *op,
             void                *in_buf,
             void                *out_buf,
             glong                samples,
             const GeglRectangle *roi,
             gint                 level,
             guint32              mask,
             gint                 bpp,
             ProcessFunc          func)
{
  const guint8  *in  = in_buf;
  guint8        *out = out_buf;
  const guint32 *in32;
  guint32       *out32;

  if (((guintptr) in - (guintptr) out) % 4 ||
      (G_BYTE_ORDER != G_LITTLE_ENDIAN && ~mask))
    {
      return func (op, in_buf, out_buf, samples, roi, level);
    }

  samples *= bpp;

  while (samples && (guintptr) in % 4)
    {
      *out++ = *in++ ^ (guint8) mask;

      mask = (mask >> 8) | (mask << 24);

      samples--;
    }

  in32  = (const guint32 *) in;
  out32 = (guint32 *)       out;

  while (samples >= 4)
    {
      *out32++ = *in32++ ^ mask;

      samples -= 4;
    }

  in  = (const guint8 *) in32;
  out = (guint8 *)       out32;

  while (samples)
    {
      *out++ = *in++ ^ (guint8) mask;

      mask >>= 8;

      samples--;
    }

  return TRUE;
}

#define PROCESS_MODEL_SUFFIX y
#define PROCESS_N_COMPONENTS 1
#define PROCESS_HAS_ALPHA    FALSE
#include "invert-common.h"

#define PROCESS_MODEL_SUFFIX ya
#define PROCESS_N_COMPONENTS 1
#define PROCESS_HAS_ALPHA    TRUE
#include "invert-common.h"

#define PROCESS_MODEL_SUFFIX rgb
#define PROCESS_N_COMPONENTS 3
#define PROCESS_HAS_ALPHA    FALSE
#include "invert-common.h"

#define PROCESS_MODEL_SUFFIX rgba
#define PROCESS_N_COMPONENTS 3
#define PROCESS_HAS_ALPHA    TRUE
#include "invert-common.h"

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  const Babl     *in_format = gegl_operation_get_source_format (operation,
                                                                "input");
  const Babl     *format    = NULL;

  if (in_format)
    {
      const Babl *model = babl_format_get_model (in_format);
      const Babl *type  = babl_format_get_type  (in_format, 0);

      #define DISPATCH_TYPE(model_suffix, type_name, type_suffix)    \
        if (type == babl_type (type_name))                           \
          {                                                          \
            o->user_data = PROCESS_FUNC (model_suffix, type_suffix); \
                                                                     \
            format = in_format;                                      \
          }

      #define DISPATCH_MODEL(model_name, model_suffix)        \
        if (babl_model_is (model, model_name))                \
          {                                                   \
                 DISPATCH_TYPE (model_suffix, "u8",    u8)    \
            else DISPATCH_TYPE (model_suffix, "u16",   u16)   \
            else DISPATCH_TYPE (model_suffix, "u32",   u32)   \
            else DISPATCH_TYPE (model_suffix, "float", float) \
          }

           DISPATCH_MODEL ("Y" INVERT_GAMMA,
                           y)
      else DISPATCH_MODEL ("Y" INVERT_GAMMA
                           "A",
                           ya)
      else DISPATCH_MODEL ("R" INVERT_GAMMA
                           "G" INVERT_GAMMA
                           "B" INVERT_GAMMA,
                           rgb)
      else DISPATCH_MODEL ("R" INVERT_GAMMA
                           "G" INVERT_GAMMA
                           "B" INVERT_GAMMA
                           "A",
                           rgba)

      #undef DISPATCH_TYPE
      #undef DISPATCH_MODEL
    }

  if (! format || gegl_operation_use_opencl (operation))
    {
      o->user_data = PROCESS_FUNC (rgba, float);

      if (in_format)
        {
          format = babl_format_with_space ("R" INVERT_GAMMA
                                           "G" INVERT_GAMMA
                                           "B" INVERT_GAMMA
                                           "A"
                                           " float",
                                           in_format);
        }
      else
        {
          format = babl_format ("R" INVERT_GAMMA
                                "G" INVERT_GAMMA
                                "B" INVERT_GAMMA
                                "A"
                                " float");
        }
    }

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *op,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o    = GEGL_PROPERTIES (op);
  ProcessFunc     func = o->user_data;

  return func (op, in_buf, out_buf, samples, roi, level);
}

#else /* defined (PROCESS_MODEL_SUFFIX) */

#ifndef PROCESS_TYPE_SUFFIX

#define PROCESS_TYPE_SUFFIX u8_
#define PROCESS_TYPE        guint8
#define PROCESS_INVERT(x)   (~(x))
#include "invert-common.h"

static gboolean
PROCESS_FUNC (PROCESS_MODEL_SUFFIX, u8) (GeglOperation       *op,
                                         void                *in_buf,
                                         void                *out_buf,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  return process_int (op, in_buf, out_buf, samples, roi, level,
                      #if ! PROCESS_HAS_ALPHA
                        0xffffffff,
                      #elif PROCESS_N_COMPONENTS == 1
                        0x00ff00ff,
                      #elif PROCESS_N_COMPONENTS == 3
                        0x00ffffff,
                      #endif
                      PROCESS_N_COMPONENTS + PROCESS_HAS_ALPHA,
                      PROCESS_FUNC (PROCESS_MODEL_SUFFIX, u8_));
}

#define PROCESS_TYPE_SUFFIX u16
#define PROCESS_TYPE        guint16
#define PROCESS_INVERT(x)   (~(x))
#include "invert-common.h"

#define PROCESS_TYPE_SUFFIX u32
#define PROCESS_TYPE        guint32
#define PROCESS_INVERT(x)   (~(x))
#include "invert-common.h"

#define PROCESS_TYPE_SUFFIX float
#define PROCESS_TYPE        gfloat
#define PROCESS_INVERT(x)   (1.0f - (x))
#include "invert-common.h"

#undef PROCESS_MODEL_SUFFIX
#undef PROCESS_N_COMPONENTS
#undef PROCESS_HAS_ALPHA

#else /* defined (PROCESS_TYPE_SUFFIX) */

static gboolean
PROCESS_FUNC (PROCESS_MODEL_SUFFIX, PROCESS_TYPE_SUFFIX) (GeglOperation       *op,
                                                          void                *in_buf,
                                                          void                *out_buf,
                                                          glong                samples,
                                                          const GeglRectangle *roi,
                                                          gint                 level)
{
  const PROCESS_TYPE *in  = in_buf;
  PROCESS_TYPE       *out = out_buf;

  while (samples--)
    {
      gint i;

      for (i = 0; i < PROCESS_N_COMPONENTS; i++)
        out[i] = PROCESS_INVERT (in[i]);

      #if PROCESS_HAS_ALPHA
        out[PROCESS_N_COMPONENTS] = in[PROCESS_N_COMPONENTS];
      #endif

      in  += PROCESS_N_COMPONENTS + PROCESS_HAS_ALPHA;
      out += PROCESS_N_COMPONENTS + PROCESS_HAS_ALPHA;
    }

  return TRUE;
}

#undef PROCESS_TYPE_SUFFIX
#undef PROCESS_TYPE
#undef PROCESS_INVERT

#endif /* defined (PROCESS_TYPE_SUFFIX) */

#endif /* defined (PROCESS_MODEL_SUFFIX) */

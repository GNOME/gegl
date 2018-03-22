#ifndef __OP_SCALE_H__
#define __OP_SCALE_H__

#include "transform-core.h"

G_BEGIN_DECLS

#define TYPE_OP_SCALE            (op_scale_get_type ())
#define OP_SCALE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_OP_SCALE, OpScale))
#define OP_SCALE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_OP_SCALE, OpScaleClass))
#define IS_OP_SCALE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_OP_SCALE))
#define IS_OP_SCALE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_OP_SCALE))
#define OP_SCALE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_OP_SCALE, OpScaleClass))

typedef struct _OpScale OpScale;

struct _OpScale
{
  OpTransform     parent_instance;

  GeglAbyssPolicy abyss_policy;
};

typedef struct _OpScaleClass OpScaleClass;

struct _OpScaleClass
{
  OpTransformClass parent_class;
};

GType op_scale_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif

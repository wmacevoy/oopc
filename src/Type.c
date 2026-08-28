#include "Type.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*
    The generic view of an object: a pointer to a vtable whose first slot is a
    Type.  Every class in the pattern starts this way, so reading through this
    view is the common-initial-sequence rule the whole prefix layout rests on.
  */
  typedef struct { const Type *type; } AnyVftbl;
  typedef struct { const AnyVftbl *vftbl; } AnyObject;

  const Type *Type_of(const void *object)
  {
    if (object == 0) return 0;
    return ((const AnyObject *)object)->vftbl->type;
  }

  int Type_isa(const Type *type, const Type *base)
  {
    for (; type != 0; type = type->parent) {
      if (type == base) return 1;
    }
    return 0;
  }

  void *Type_cast(void *object, const Type *base)
  {
    return Type_isa(Type_of(object), base) ? object : 0;
  }

  const void *Type_castConst(const void *object, const Type *base)
  {
    return Type_isa(Type_of(object), base) ? object : 0;
  }

#ifdef __cplusplus
} /* extern "C" */
#endif

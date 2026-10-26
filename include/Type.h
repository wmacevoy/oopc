#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

  struct TypeStruct;
  typedef struct TypeStruct Type;

  /*
    One Type per class -- three words, shared by every instance.  Objects carry
    no type tag of their own; they reach it through the vtable they already
    point at.

    `size` is here so a HOST can allocate an object without knowing C's sizeof.
    Together with the Class_init/Class_fini split (which works on storage the
    caller supplies), that lets Python or JavaScript own the memory and still
    get correctly constructed C objects.
  */
  struct TypeStruct {
    const char *name;
    const Type *parent;   /* 0 at the root of a hierarchy */
    size_t size;          /* sizeof the C struct */
  };

  /*
    Every vtable in the pattern begins with its Type, and every object begins
    with a pointer to its vtable.  That is the whole mechanism: given any object
    in the hierarchy, its dynamic type is one double indirection away, with no
    per-object cost.
  */
  const Type *Type_of(const void *object);

  /* Is `type` `base`, or descended from it?  Walks the parent chain. */
  int Type_isa(const Type *type, const Type *base);

  /* Checked downcast: returns `object` if it really is a `base`, else 0. */
  void *Type_cast(void *object, const Type *base);
  const void *Type_castConst(const void *object, const Type *base);

  /*
    The fields again, as functions.  C reads the struct directly; a host calls
    these instead, so it never needs to know the struct's layout -- which
    differs between 32- and 64-bit targets.
  */
  const char *Type_name(const Type *type);
  const Type *Type_parent(const Type *type);
  size_t Type_size(const Type *type);

#ifdef __cplusplus
}
#endif

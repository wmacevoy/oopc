/*
  cpp-1: the C ABI, emitted from the C++ model.

  This file is the entire cost of the inversion, and it is worth looking at
  closely: one line per entry point, and each line is checked against the
  published declaration in include/Bottle.h / include/SizedBottle.h -- if a
  signature drifts, this file stops compiling.

  It cannot be shorter.  C has no reflection and neither does C++17, so nothing
  can enumerate a class's methods and mint C names for them.  C++26 static
  reflection (P2996) can enumerate the members, but still does not hand you
  arbitrary extern "C" symbols, so some form of this list survives.

  Compare the cost: src/Bottle.c writes each method THREE times (implementation,
  vtable slot, dispatcher) plus two mirrored declarations in the header.  Here a
  method is written once, appears once in the table, and once here.

  DELIBERATE ABI DELTA.  Bottle_vftbl and SizedBottle_vftbl are declared by the
  C headers but are NOT defined here.  They exist so that a derived class
  written in C, in another translation unit, can name base slots in a static
  initializer.  In the inverted model that job belongs to C++ inheritance, and
  exporting a second copy of a table would be a lie.  Every other symbol in
  those headers -- including Bottle_type and SizedBottle_type, which Type_isa
  compares by address -- is defined.
*/

#include <cstdlib>
#include <new>

#include "oopc/SizedBottle.hpp"

/*
  Two names for one object, and neither is ever written unqualified:
  `oopc1::Bottle` is the C++ class, `::Bottle` the opaque C handle.  The
  static_asserts in the headers prove they are the same bytes at the same
  address; keeping the names distinct keeps the boundary visible.

  Allocation and initialization stay separate for the same reason they are
  separate in C -- a host that knows only Type::size can own the memory -- and
  in C++ that split already has a name: Class_init is `::new (storage) Class()`
  and Class_fini runs the destructor chain.
*/
extern "C" {

  /* ---- Bottle ---------------------------------------------------------- */

  ::Bottle *Bottle_new(void)
  {
    ::Bottle *me = (::Bottle *)std::malloc(sizeof(oopc1::Bottle));
    TRACE("Bottle_new()=%p\n", (void *)me);
    if (me != nullptr) { Bottle_init(me); }
    return me;
  }

  void Bottle_delete(::Bottle *me)
  {
    TRACE("Bottle_delete(%p)\n", (void *)me);
    if (me != nullptr) {
      reinterpret_cast<oopc1::Bottle *>(me)->fini();  /* dispatches: the whole chain */
      std::free(me);
    }
  }

  void Bottle_init(::Bottle *me)
  { TRACE("Bottle_init(%p)\n", (void *)me); ::new ((void *)me) oopc1::Bottle(); }

  void Bottle_fini(::Bottle *me)
  { return oopc::Export<&oopc1::Bottle::fini>::call(me); }

  double Bottle_getVolume(const ::Bottle *me)
  { return oopc::Export<&oopc1::Bottle::getVolume>::call(me); }

  void Bottle_setVolume(::Bottle *me, double value)
  { return oopc::Export<&oopc1::Bottle::setVolume>::call(me, value); }

  ::Bottle *Bottle_cast(void *object)
  { return (::Bottle *)Type_cast(object, &Bottle_type); }

  const ::Type *Bottle_getType(void)
  { return &Bottle_type; }

  /* ---- SizedBottle ----------------------------------------------------- */

  ::SizedBottle *SizedBottle_new(void)
  {
    ::SizedBottle *me = (::SizedBottle *)std::malloc(sizeof(oopc1::SizedBottle));
    TRACE("SizedBottle_new()=%p\n", (void *)me);
    if (me != nullptr) { SizedBottle_init(me); }
    return me;
  }

  void SizedBottle_delete(::SizedBottle *me)
  {
    TRACE("SizedBottle_delete(%p)\n", (void *)me);
    if (me != nullptr) {
      reinterpret_cast<oopc1::Bottle *>(me)->fini();
      std::free(me);
    }
  }

  void SizedBottle_init(::SizedBottle *me)
  { TRACE("SizedBottle_init(%p)\n", (void *)me); ::new ((void *)me) oopc1::SizedBottle(); }

  void SizedBottle_fini(::SizedBottle *me)
  { return oopc::Export<&oopc1::Bottle::fini>::call(me); }

  /* Inherited, and that is the whole point: these name Bottle's methods, which
     dispatch through the table into SizedBottle's override. */
  double SizedBottle_getVolume(const ::SizedBottle *me)
  { return oopc::Export<&oopc1::Bottle::getVolume>::call(me); }

  void SizedBottle_setVolume(::SizedBottle *me, double value)
  { return oopc::Export<&oopc1::Bottle::setVolume>::call(me, value); }

  double SizedBottle_getMaxVolume(const ::SizedBottle *me)
  { return oopc::Export<&oopc1::SizedBottle::getMaxVolume>::call(me); }

  void SizedBottle_setMaxVolume(::SizedBottle *me, double value)
  { return oopc::Export<&oopc1::SizedBottle::setMaxVolume>::call(me, value); }

  ::SizedBottle *SizedBottle_cast(void *object)
  { return (::SizedBottle *)Type_cast(object, &SizedBottle_type); }

  const ::Type *SizedBottle_getType(void)
  { return &SizedBottle_type; }
}

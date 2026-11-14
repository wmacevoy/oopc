#pragma once

#include <stdio.h>

#include "Bottle.hpp"

#include "SizedBottle.h"

namespace cxx {

  /*
    Derived wrapper.  Follow this shape for every new class:

      1. Reserve the bytes your C struct adds beyond your base's C struct.
         Do NOT embed your C struct -- the base already owns its prefix, and
         embedding a second one gives you two disjoint objects (base methods
         would then operate on the wrong, uninitialized one).
      2. Define me() as a downcast of your parent's me().  Every level in the
         chain has this same shape, and no level reaches past its immediate
         parent's storage.
      3. Constructor: base(UNINITIALIZED), then your C _init on me().
      4. Destructor: EMPTY.  ~Bottle dispatches ::Bottle_fini through the C
         vtable, which runs SizedBottle's finalizer and then Bottle's.
      5. static_assert the size and alignment against your C struct.
  */
  class SizedBottle : public Bottle {

    /* Storage for the fields ::SizedBottle adds on top of ::Bottle.  These
       bytes belong to the C layer -- SizedBottle_init writes them and C++ never
       names them.  Together with the base's storage they make this object
       byte-identical to a ::SizedBottle; the static_asserts below prove it. */
  private: alignas(::SizedBottle) char extra[sizeof(::SizedBottle) - sizeof(::Bottle)];

    /* C++ never reads or writes extra -- the C layer owns those bytes -- so the
       only thing to say about it is that it covers exactly the gap between the
       base's C struct and ours.  Stating that here also keeps the field from
       looking unused to -Wunused-private-field. */
    static_assert(sizeof(::Bottle) + sizeof(extra) == sizeof(::SizedBottle),
                  "extra must cover exactly what ::SizedBottle adds to ::Bottle");

  public: inline ::SizedBottle *me() 
    { 
      return reinterpret_cast< ::SizedBottle *>(Bottle::me()); 
    }
  public: inline const ::SizedBottle *me() const 
    { 
      return reinterpret_cast<const ::SizedBottle *>(Bottle::me()); 
    }

  protected: inline SizedBottle(enum UNINITIALIZED) 
    : Bottle(Bottle::UNINITIALIZED)
    {
      printf("SizedBottle(UNINITIALIZED)\n");
    }

  public: inline SizedBottle()
    : Bottle(Bottle::UNINITIALIZED) 
    { 
      printf("SizedBottle()\n");
      ::SizedBottle_init(me()); 
    }

    /* Empty on purpose -- see note 4 above. */
  public: inline ~SizedBottle() 
    { 
      printf("~SizedBottle()\n");
    }

  public: inline void setMaxVolume(const double &value) 
    {
      printf("SizedBottle.setMaxVolume(%lf)\n",value);
      ::SizedBottle_setMaxVolume(me(),value);
    }

  public: inline double getMaxVolume() const 
    {
      printf("SizedBottle.getMaxVolume()\n");
      return ::SizedBottle_getMaxVolume(me());
    }
  };

  /*
    IS me() == this HERE?

    Yes on every real ABI, but the standard does not promise it.  This class
    holds data in two places (Bottle::_me and extra), so unlike cxx::Bottle it is
    NOT standard-layout, and [class.mem]'s "points at its first member" rule does
    not apply.  What the standard leaves open is where the Bottle base subobject
    sits; the Itanium C++ ABI (gcc/clang everywhere) and MSVC both place a lone
    non-virtual base with no virtual functions at offset 0.

    The size assertion below is what actually pins it down.  Call the base
    subobject B bytes and extra E, so B + E == sizeof(::SizedBottle) (asserted
    above).  If the base sat at any offset O > 0 the object would be at least
    O + B + E bytes and the assertion would fail.  Equality forces O == 0 and
    forces extra to start exactly at B -- which is where ::SizedBottle keeps
    maxVolume, on every target.  Note that me() never computes an offset from
    `this`: it downcasts Bottle::me(), so it stays correct regardless.
  */
  static_assert(sizeof(SizedBottle) == sizeof(::SizedBottle),
                "cxx::SizedBottle must be byte-identical to ::SizedBottle");
  static_assert(alignof(SizedBottle) == alignof(::SizedBottle),
                "cxx::SizedBottle must have ::SizedBottle's alignment");
}

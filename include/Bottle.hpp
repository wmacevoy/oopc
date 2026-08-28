#pragma once

#include <stdio.h>

#include <type_traits>

#include "Bottle.h"

namespace cxx {

  /*
    Zero-overhead C++ view of ::Bottle.

    STORAGE.  The embedded ::Bottle below is the *only* storage in the whole
    wrapper hierarchy: sizeof(cxx::Bottle) == sizeof(::Bottle).  A derived
    wrapper does NOT embed a second C struct -- it reserves exactly the extra
    bytes its own C struct adds, so the C++ object and the C object are the same
    bytes at the same address.  See SizedBottle.hpp.

    DESTRUCTION.  ~Bottle runs the *entire* finalizer chain, because
    ::Bottle_fini dispatches through the C vtable.  Derived wrappers therefore
    leave their destructors empty; finalizing again there would run the chain
    twice.

    POLYMORPHISM.  Use cxx::Bottle & (or ::Bottle *), not new/delete.  A
    cxx::Bottle & bound to a cxx::SizedBottle dispatches virtually through the C
    vtable, with no C++ vtable pointer in the object.  ~Bottle is deliberately
    non-virtual: making it virtual would add a second vtable pointer on top of
    the C one, which is the overhead this pattern exists to avoid.  The cost of
    that choice is that you must not `delete` a derived wrapper through a
    cxx::Bottle *.  Heap-allocate through the C API (Bottle_new/Bottle_delete)
    instead -- those dispatch correctly.
  */
  class Bottle {

  private: ::Bottle _me;

    /* Every wrapper exposes its own C object as me(), so C++ code can hand the
       object straight to the C API.  This is the base of the chain; a derived
       wrapper's me() is a downcast of its parent's me(), so no level ever
       reaches past its immediate parent's storage.  A derived me() hides this
       one -- say b.Bottle::me() when you want the base view. */
  public: inline ::Bottle *me() { return &_me; }
  public: inline const ::Bottle *me() const { return &_me; }

    /* Derived wrappers pass this tag to the base constructor: the most-derived
       class's C _init initializes the whole object, this prefix included. */
  protected: enum UNINITIALIZED { UNINITIALIZED };
  protected: inline Bottle(enum UNINITIALIZED) 
    { 
      printf("Bottle(UNINITIALIZED)\n"); 
    }    

  public: inline Bottle() 
    { 
      printf("Bottle()\n");
      ::Bottle_init(me()); 
    }

  public: inline ~Bottle() 
    { 
      printf("~Bottle()\n");
      ::Bottle_fini(me()); 
    }

  public: inline void setVolume(const double &value) 
    {
      printf("Bottle.setVolume(%lf)\n",value);
      ::Bottle_setVolume(me(),value);
    }

  public: inline double getVolume() const 
    {
      printf("Bottle.getVolume()\n");
      return ::Bottle_getVolume(me());
    }
  };

  /* The wrapper must be exactly the C struct -- no more, no less. */
  static_assert(sizeof(Bottle) == sizeof(::Bottle),
                "cxx::Bottle must be byte-identical to ::Bottle");
  static_assert(alignof(Bottle) == alignof(::Bottle),
                "cxx::Bottle must have ::Bottle's alignment");

  /* Standard-layout: one class holds all the non-static data, nothing virtual.
     That is what makes (void*)me() == (void*)this GUARANTEED here -- [class.mem]
     says a pointer to a standard-layout object, reinterpret_cast, points at its
     first member.  Derived wrappers hold data in two classes (base + their own
     reserved bytes), so they are NOT standard-layout and get no such promise
     from the standard; see the note in SizedBottle.hpp. */
  static_assert(std::is_standard_layout<Bottle>::value,
                "cxx::Bottle must be standard-layout");
}

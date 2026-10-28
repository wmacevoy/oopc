#pragma once

/*
  cpp-1 machinery: the inverted oopc model.

  In the C version (the src/ .c files) a class is written three times -- the vf_
  implementations, the vtable literal, and the public dispatchers -- and four
  invariants are maintained by hand and checked by nobody:

    1. a derived vtable restates every base slot first, in order;
    2. a derived struct restates every base field first, in order;
    3. an override downcasts its Bottle * argument with a raw cast;
    4. an override chains to its base with Base_vf_f, never Base_f.

  Here C++ is the source and the C ABI is the artifact.  Ordinary single
  inheritance supplies 1 and 2, `static_cast` in a generated thunk supplies 3,
  and destructor chaining supplies 4 -- so all four stop being human problems.

  Nothing in this file adds a byte to an object.  There is no `virtual` keyword
  anywhere in the model: the vtable is authored, so its layout is ours, it is
  constexpr (hence read-only, no lazy fill-in, no race), and C can read it.

  Requires C++17: `auto` non-type template parameters and inline variables.
*/

#include <cstddef>
#include <type_traits>

#include "Type.h"

namespace oopc {

  /*
    Per-class constants, specialized once per class right after its definition.
    A specialization supplies:

      static constexpr const ::Type      *type;   /-* the runtime Type *-/
      static constexpr const C::Vtbl     *vtbl;   /-* the dispatch table *-/

    Keeping them here rather than in the class lets the class stay a plain
    aggregate whose size is exactly its C struct's, and lets the table -- which
    needs the class to be complete -- be written after the class body.
  */
  template <class C> struct Class;

  /*
    SLOT THUNK.  A vtable slot is typed on the class that DECLARED it, so an
    override written on a derived class does not fit the slot directly.  This
    adapts it, and the adaptation is a `static_cast` -- a compiler-computed,
    statically checked downcast -- rather than the raw `(SizedBottle*)base`
    every override in src/SizedBottle.c writes by hand.

    Two specializations: non-const and const member functions.
  */
  template <class Decl, auto M> struct Slot;

  template <class Decl, class Self, class R, class... A, R (Self::*M)(A...)>
  struct Slot<Decl, M> {
    static_assert(std::is_base_of<Decl, Self>::value,
                  "a slot declared on Decl can only be filled by Decl or a class derived from it");
    static R fn(Decl *me, A... a) { return (static_cast<Self *>(me)->*M)(a...); }
  };

  template <class Decl, class Self, class R, class... A, R (Self::*M)(A...) const>
  struct Slot<Decl, M> {
    static_assert(std::is_base_of<Decl, Self>::value,
                  "a slot declared on Decl can only be filled by Decl or a class derived from it");
    static R fn(const Decl *me, A... a) { return (static_cast<const Self *>(me)->*M)(a...); }
  };

  /*
    The fini slot.  It calls the MOST-DERIVED destructor, so ~Self runs and then
    C++ chains automatically into ~Base.  That is the entire finalizer chain,
    and it is why footgun 4 above cannot be written here: chaining is the
    language's job, not the author's.

    It also means members with non-trivial destructors just work -- something a
    C fini has to do by hand, field by field.
  */
  template <class Decl, class Self>
  struct Destroy {
    static void fn(Decl *me) { static_cast<Self *>(me)->~Self(); }
  };

  /*
    EXPORT ADAPTER.  Turns a C++ method into the body of an extern "C" entry
    point: one expression, whatever the arity.  `Handle` is the opaque C struct
    from the published header (::Bottle), which the static_asserts in each class
    header prove is the same bytes as the C++ class.
  */
  template <auto M> struct Export;

  template <class Self, class R, class... A, R (Self::*M)(A...)>
  struct Export<M> {
    template <class Handle>
    static R call(Handle *me, A... a) { return (reinterpret_cast<Self *>(me)->*M)(a...); }
  };

  template <class Self, class R, class... A, R (Self::*M)(A...) const>
  struct Export<M> {
    template <class Handle>
    static R call(const Handle *me, A... a) { return (reinterpret_cast<const Self *>(me)->*M)(a...); }
  };
}

/* A slot filled from a member function: OOPC_SLOT(Bottle, &SizedBottle::vf_setVolume) */
#define OOPC_SLOT(Decl, Method) (&::oopc::Slot<Decl, Method>::fn)

/* The fini slot for class Self, in a table whose slots are typed on Decl. */
#define OOPC_FINI(Decl, Self) (&::oopc::Destroy<Decl, Self>::fn)

/* Every class header ends with this: the C++ object must BE the C struct. */
#define OOPC_ABI(Cxx, C)                                                      \
  static_assert(sizeof(Cxx) == sizeof(C),                                     \
                #Cxx " must be byte-identical to " #C);                       \
  static_assert(alignof(Cxx) == alignof(C),                                   \
                #Cxx " must have " #C "'s alignment")

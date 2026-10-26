#pragma once

/*
  cpp-1: Bottle, written as C++, flattened to the C ABI in include/Bottle.h.

  Compare src/Bottle.c side by side.  Every construct below has an exact image
  there, and the mapping is mechanical:

    class Bottle             <->  struct BottleStruct
    struct Bottle::Vtbl      <->  BottleVftbl
    oopc::Class<Bottle>      <->  Bottle_type, Bottle_vftbl
    Bottle::vf_f()           <->  Bottle_vf_f()      (the implementation)
    Bottle::f()              <->  Bottle_f()         (the dispatcher)
    Bottle()                 <->  Bottle_init()
    ~Bottle()                <->  Bottle_vf_fini()

  The public/vf_ split is the Non-Virtual Interface idiom, and it is not a
  stylistic choice: it is what the C pattern already does, named the same way.
  The public method dispatches through the table; the vf_ method is the
  overridable implementation.  Calling b.getVolume() therefore reaches the
  DERIVED override, exactly as ::Bottle_getVolume does on an upcast pointer.
*/

#include "Bottle.h"        /* the published C ABI this file implements */
#include "Trace.h"
#include "oopc/Class.hpp"

namespace oopc1 {

  class Bottle {

    /*
      THE VTABLE.  Nested (`Derived::Vtbl` has a `Base::Vtbl` as its first
      member), not inherited.  Nesting keeps every level standard-layout, so
      "a derived table starts with the base table" is guaranteed by
      [class.mem] -- the compiler's promise, not the ABI's.  Inheriting the
      table would read a little nicer and give up that guarantee.

      Slots are typed on the class that DECLARES them, not on the root as in
      the C version.  A derived class fills an inherited slot through
      oopc::Slot, which does the downcast; no function pointer is ever cast.
    */
  public:
    struct Vtbl {
      const ::Type *type;
      void   (*fini)(Bottle *me);
      double (*getVolume)(const Bottle *me);
      void   (*setVolume)(Bottle *me, double value);
    };

    /* The table builder needs the addresses of the vf_ methods below.  In C
       those had to be non-static -- fully public -- for exactly this reason
       (see the "protected" note in include/Bottle.h).  Here a friend
       declaration says the same thing and actually enforces it. */
    friend struct ::oopc::Class<Bottle>;

    /*
      STORAGE.  These two members, in this order, are struct BottleStruct.
      This class has no base and no virtual functions, so it is standard-layout
      and `(void*)this == (void*)&_vptr` is guaranteed.
    */
  private:
    const Vtbl *_vptr;
    double _volume;

    /*
      The only two casts in the entire model, written once, here at the root.
      Both are between pointer-interconvertible standard-layout types (a nested
      table and its first member), so both are well defined.
    */
  protected:
    template <class C> const typename C::Vtbl *vt() const
    { return reinterpret_cast<const typename C::Vtbl *>(_vptr); }

    template <class C> void bind()
    { _vptr = reinterpret_cast<const Vtbl *>(::oopc::Class<C>::vtbl); }

    /* ---- implementations: the vf_ layer -------------------------------- */
  protected:
    double vf_getVolume() const;
    void   vf_setVolume(double value);

    /* ~Bottle IS Bottle_vf_fini: it does this level's teardown and nothing
       else.  The chain is the language's job -- see oopc::Destroy. */
  public:
    ~Bottle();

    /* ---- construction --------------------------------------------------- */
  public:
    Bottle();

  protected:
    /* Derived constructors take this: the base must not install its own table
       before the derived class installs the real one.  ::SizedBottle_init does
       install the base table and then overwrite it; both orders are correct,
       and the difference is discussed in cpp1/README.md. */
    enum Uninitialized { UNINITIALIZED };
    explicit Bottle(Uninitialized) {}

    /* No copying: the C ABI has no notion of it, and a copy would have to
       decide what to do with the vtable pointer. */
  public:
    Bottle(const Bottle &) = delete;
    Bottle &operator=(const Bottle &) = delete;

    /* ---- dispatchers: the public layer ---------------------------------- */
  public:
    void   fini();
    double getVolume() const;
    void   setVolume(double value);

    /* Type_of, with no per-object cost: the Type is the table's first slot. */
    const ::Type *type() const { return vt<Bottle>()->type; }
  };
}

/*
  CLASS CONSTANTS.  constexpr, so they are constant-initialized into read-only
  memory: no lazy fill-in, no data race, nothing writable to corrupt -- the same
  properties src/Bottle.c gets from a static const initializer.

  Bottle_type is defined here rather than as a private C++ object because
  Type_isa compares Type addresses: the symbol include/Bottle.h publishes and
  the one this vtable points at must be the same object.
*/
extern "C" inline constexpr ::Type Bottle_type =
  { "Bottle", nullptr, sizeof(oopc1::Bottle) };

namespace oopc {
  template <> struct Class<oopc1::Bottle> {
    static constexpr const ::Type *type = &Bottle_type;

    static constexpr oopc1::Bottle::Vtbl table = {
      &Bottle_type,
      OOPC_FINI(oopc1::Bottle, oopc1::Bottle),
      OOPC_SLOT(oopc1::Bottle, &oopc1::Bottle::vf_getVolume),
      OOPC_SLOT(oopc1::Bottle, &oopc1::Bottle::vf_setVolume)
    };
    static constexpr const oopc1::Bottle::Vtbl *vtbl = &table;
  };
}

namespace oopc1 {

  /* ---- definitions ----------------------------------------------------- */
  /* Out of line because bind<Bottle>() needs oopc::Class<Bottle> above. */

  inline Bottle::Bottle()
  {
    bind<Bottle>();
    _volume = 0;
  }

  inline Bottle::~Bottle()
  {
    TRACE("Bottle vf fini(%p)\n", (void *)this);
  }

  inline double Bottle::vf_getVolume() const
  {
    TRACE("Bottle vf getVolume(%p)\n", (const void *)this);
    return _volume;
  }

  inline void Bottle::vf_setVolume(double value)
  {
    TRACE("Bottle vf setVolume(%p,%lf)\n", (void *)this, value);
    _volume = value;
  }

  inline void Bottle::fini()
  {
    TRACE("Bottle_fini(%p)\n", (void *)this);
    vt<Bottle>()->fini(this);
  }

  inline double Bottle::getVolume() const
  {
    TRACE("Bottle_getVolume(%p)\n", (const void *)this);
    return vt<Bottle>()->getVolume(this);
  }

  inline void Bottle::setVolume(double value)
  {
    TRACE("Bottle_setVolume(%p,%lf)\n", (void *)this, value);
    vt<Bottle>()->setVolume(this, value);
  }

  OOPC_ABI(Bottle, ::Bottle);
  static_assert(std::is_standard_layout<Bottle>::value,
                "oopc1::Bottle must be standard-layout, like cxx::Bottle");
  static_assert(sizeof(Bottle::Vtbl) == sizeof(::BottleVftbl),
                "the authored vtable must be the published BottleVftbl");
}

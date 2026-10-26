#pragma once

/*
  cpp-1: the derived class.  This is where the inversion earns its keep.

  Read src/SizedBottle.c next to this file.  Four things that file maintains by
  hand, with nothing checking them, are gone here:

  1. LAYOUT.  struct SizedBottleStruct restates `vftbl` and `volume` before
     adding `maxVolume`, and CLAUDE.md warns that reordering Bottle.h means
     mirroring it here in the same commit.  Below, `: public Bottle` says it,
     and the compiler enforces it.

  2. VTABLE PREFIX.  SizedBottleVftbl restates all four BottleVftbl slots, in
     order, with matching types.  Below, `Bottle::Vtbl base;` says it once.

  3. DOWNCASTS.  Every SizedBottle_vf_* opens with `(SizedBottle*)base`, a raw
     reinterpret.  Below, the methods are ordinary members and the cast lives
     in oopc::Slot as a checked static_cast.

  4. THE FINI FOOTGUN.  SizedBottle_vf_fini must chain with Bottle_vf_fini and
     never Bottle_fini, or it recurses forever.  Below, ~SizedBottle is empty
     and the chain is the language's.  The wrong version cannot be written.

  What is NOT gone: the vtable literal still names its slots, and each exported
  C entry point is still one line.  Those are the irreducible cost -- C++ has no
  reflection, so nothing can enumerate the methods for you.
*/

#include "SizedBottle.h"   /* the published C ABI this file implements */
#include "oopc/Bottle.hpp"

namespace oopc1 {

  class SizedBottle : public Bottle {

  public:
    struct Vtbl {
      Bottle::Vtbl base;                       /* invariant 2, stated once */
      double (*getMaxVolume)(const Bottle *me);
      void   (*setMaxVolume)(Bottle *me, double value);
    };

    friend struct ::oopc::Class<SizedBottle>;

    /* invariant 1: the base's fields come first because the base is first */
  private:
    double _maxVolume;

    /* ---- implementations ------------------------------------------------ */
    /* Only what this class overrides or adds.  vf_getVolume is inherited by
       being absent from the table below -- not by being restated, as
       src/SizedBottle.c must restate Bottle_vf_getVolume. */
  protected:
    void   vf_setVolume(double value);
    double vf_getMaxVolume() const;
    void   vf_setMaxVolume(double value);

    /* Empty on purpose: ~Bottle runs next, automatically.  This is
       SizedBottle_vf_fini, minus the hazard. */
  public:
    ~SizedBottle();

  public:
    SizedBottle();

    /* ---- dispatchers ---------------------------------------------------- */
    /* getVolume/setVolume/fini are inherited from Bottle and dispatch through
       the same table, so they already reach the override above.  Only the new
       methods need a dispatcher. */
  public:
    double getMaxVolume() const;
    void   setMaxVolume(double value);
  };
}

extern "C" inline constexpr ::Type SizedBottle_type =
  { "SizedBottle", &Bottle_type, sizeof(oopc1::SizedBottle) };

namespace oopc {
  template <> struct Class<oopc1::SizedBottle> {
    static constexpr const ::Type *type = &SizedBottle_type;

    /*
      THE TABLE, BUILT BY PATCHING THE BASE.

      This is what a vtable actually is, and it is the one thing the C version
      cannot express: `Class<Bottle>::table` is copied wholesale, then the
      overridden slots are patched.  getVolume is inherited by NOT being
      mentioned.  Add a slot to Bottle and this file keeps compiling and stays
      correct; src/SizedBottle.c would silently need a new line in the right
      position.

      Still constexpr, so still one const table in read-only memory.
    */
    static constexpr oopc1::SizedBottle::Vtbl table = [] {
      oopc1::SizedBottle::Vtbl v = {
        Class<oopc1::Bottle>::table,           /* inherit every base slot */
        OOPC_SLOT(oopc1::Bottle, &oopc1::SizedBottle::vf_getMaxVolume),
        OOPC_SLOT(oopc1::Bottle, &oopc1::SizedBottle::vf_setMaxVolume)
      };
      v.base.type      = &SizedBottle_type;    /* ...then patch the overrides */
      v.base.fini      = OOPC_FINI(oopc1::Bottle, oopc1::SizedBottle);
      v.base.setVolume = OOPC_SLOT(oopc1::Bottle, &oopc1::SizedBottle::vf_setVolume);
      return v;
    }();

    static constexpr const oopc1::SizedBottle::Vtbl *vtbl = &table;
  };
}

namespace oopc1 {

  /* ---- definitions ----------------------------------------------------- */

  inline SizedBottle::SizedBottle()
    : Bottle()                    /* == Bottle_init: base fields, base table */
  {
    bind<SizedBottle>();          /* == overwriting me->vftbl in the C init  */
    _maxVolume = 1;
  }

  inline SizedBottle::~SizedBottle()
  {
    TRACE("SizedBottle vf fini(%p)\n", (void *)this);
  }

  inline void SizedBottle::vf_setVolume(double value)
  {
    TRACE("SizedBottle vf setVolume(%p,%lf)\n", (void *)this, value);
    if (value < 0) value = 0;
    if (value > _maxVolume) value = _maxVolume;
    /* Chain to the base implementation.  `Bottle::vf_setVolume` is a qualified
       call, so it is non-virtual BY DEFINITION -- it is the exact image of
       `Bottle_vf_setVolume(base, value)` in src/SizedBottle.c, and the runaway
       recursion that file warns about is not expressible. */
    Bottle::vf_setVolume(value);
  }

  inline double SizedBottle::vf_getMaxVolume() const
  {
    TRACE("SizedBottle vf getMaxVolume(%p)\n", (const void *)this);
    return _maxVolume;
  }

  inline void SizedBottle::vf_setMaxVolume(double value)
  {
    TRACE("SizedBottle vf setMaxVolume(%p,%lf)\n", (void *)this, value);
    if (value < 0) value = 0;
    _maxVolume = value;
    /* keep volume <= maxVolume; Bottle's _volume is private, so through its
       non-virtual implementations, as a derived C class would */
    if (Bottle::vf_getVolume() > value) Bottle::vf_setVolume(value);
  }

  inline double SizedBottle::getMaxVolume() const
  {
    TRACE("SizedBottle_getMaxVolume(%p)\n", (const void *)this);
    return vt<SizedBottle>()->getMaxVolume(this);
  }

  inline void SizedBottle::setMaxVolume(double value)
  {
    TRACE("SizedBottle_setMaxVolume(%p,%lf)\n", (void *)this, value);
    vt<SizedBottle>()->setMaxVolume(this, value);
  }

  OOPC_ABI(SizedBottle, ::SizedBottle);
  static_assert(sizeof(SizedBottle::Vtbl) == sizeof(::SizedBottleVftbl),
                "the authored vtable must be the published SizedBottleVftbl");

  /*
    NOT standard-layout: data lives in two classes (Bottle's and this one), so
    [class.mem] says nothing about where the base subobject sits.  This is the
    same gap cxx::SizedBottle documents, and the same argument closes it: a
    base at any offset O > 0 would make the object at least O bytes larger than
    the base plus _maxVolume, and fail the size assert above.

    C is no better off here -- the prefix-cast idiom src/SizedBottle.c relies on
    is only formally blessed for a common initial sequence seen through a union.
    The difference is that upcasts and downcasts BELOW are compiler-computed and
    would stay correct even if the base did not sit at offset 0; only the C and
    host views of the bytes depend on that.
  */
}

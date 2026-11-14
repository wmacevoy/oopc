/*
  The two contracts of Bottle, as claims a model checker can check.

  A whole-program proof (oopc_proof.c) reads every class at once.  That does
  not scale, and it proves nothing about a class nobody has written yet.
  Contracts are how a proof is cut into pieces, and a class has two:

    THE CALLER'S CONTRACT is what holds through a Bottle *, whatever the object
    really is.  It has to be weak enough for every subclass.  "A Bottle keeps
    what it is given" is NOT in it: a SizedBottle clamps.  What every class
    keeps is below, in caller_contract.

    THE EXTENDER'S CONTRACT is what a derived class may rely on from its base:
    Bottle_init and the non-virtual Bottle_vf_ functions, exactly.  It is in
    extender_contract.c, and bottle_contract.c is the least Bottle that meets it.

  For a real program this is the only way a proof stays possible: nobody can
  check everything at once, so each piece is checked against a page of
  contract, and the pieces are joined by argument, not by a bigger run.

  If Bottle meets both, then to add a class you only need to show
    1. it keeps the caller's contract            (caller_contract.c)
    2. it is correct over the extender's contract (oopc_proof.c, with
       bottle_contract.c in place of a Bottle)
  and no proof of the pair, or of any client, is needed again.

  The assumptions are those of oopc_proof.c: finite numbers, and the checker's
  model of C.  No bound on history is needed: every proof here starts from an
  arbitrary state.  (REQ10/PRF-5, REQ10/PRF-6)
*/

#pragma once

#include "Bottle.h"
#include "Type.h"

int nondet_int(void);
double nondet_double(void);

static double any_finite(void)
{
  double x = nondet_double();
  __CPROVER_assume(__CPROVER_isfinited(x));
  return x;
}

/* THE CALLER'S CONTRACT, for an object in any state its class allows. */
static void caller_contract(Bottle *b)
{
  const Type *class_before = Type_of(b);
  double held = Bottle_getVolume(b);

  __CPROVER_assert(__CPROVER_isfinited(held), "REQ10/PRF-5 C1 getVolume is finite");
  __CPROVER_assert(Bottle_getVolume(b) == held, "REQ10/PRF-5 C1 getVolume changes nothing");

  Bottle_setVolume(b, held);
  __CPROVER_assert(Bottle_getVolume(b) == held,
                   "REQ10/PRF-5 C2 setting what it holds changes nothing");

  Bottle_setVolume(b, any_finite());
  __CPROVER_assert(__CPROVER_isfinited(Bottle_getVolume(b)),
                   "REQ10/PRF-5 C1 and after any setVolume it is still finite");
  __CPROVER_assert(Type_of(b) == class_before, "REQ10/PRF-5 C3 no call changes the class");
}

/* A CLIENT, written against Bottle * and nothing else. */
static double settle(Bottle *b, double x)
{
  double held;
  Bottle_setVolume(b, x);
  held = Bottle_getVolume(b);
  Bottle_setVolume(b, held);
  return held;
}

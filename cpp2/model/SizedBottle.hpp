/*
  cpp-2 model: the derived class, in ordinary C++.

  Note what is NOT here, compared with src/SizedBottle.c:

    - no restated vtable slots      -- `: public Bottle` says it
    - no restated struct fields     -- same
    - no `(SizedBottle*)base` cast  -- these are ordinary members
    - no Bottle_vf_fini chaining    -- ~Bottle runs automatically

  The generator puts all four back, correctly, on the C side.
*/

#pragma once

#include "Bottle.hpp"

class SizedBottle : public Bottle {

public:
  SizedBottle()
  {
    maxVolume = 1;
  }

  virtual ~SizedBottle()
  {
  }

  virtual void setVolume(double value)
  {
    if (value < 0) value = 0;
    if (value > maxVolume) value = maxVolume;
    /* Qualified, therefore non-virtual: the generator turns this into
       Bottle_vf_setVolume((Bottle *)me, value). */
    Bottle::setVolume(value);
  }

  virtual double getMaxVolume() const
  {
    return maxVolume;
  }

  virtual void setMaxVolume(double value)
  {
    if (value < 0) value = 0;
    maxVolume = value;
    if (volume > value) volume = value;   /* keep volume <= maxVolume */
  }

protected:
  double maxVolume;
};

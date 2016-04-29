#pragma once

#include "SizedBottle.h"

namespace cxx {
  class SizedBottle : public Bottle {

  private: ::SizedBottle me;

  protected: inline SizedBottle(enum UNINITIALIZED value) 
    : Bottle(UNINITIALIZED)
    {
      printf("SizedBottle(UNINITIALIZED)\n");
    }

  public: inline SizedBottle()
    : Bottle(UNINITIALIZED) 
    { 
      printf("SizedBottle()\n");
      ::SizedBottle_init(&me); 
    }

  public: inline ~SizedBottle() 
    { 
      printf("~SizedBottle()\n");
      ::SizedBottle_fini(&me); 
    }

  public: inline void setMaxVolume(const double &value) 
    {
      printf("SizedBottle.setMaxVolume(%lf)\n",value);
      ::SizedBottle_setMaxVolume(&me,value);
    }

  public: inline double getMaxVolume() const 
    {
      printf("SizedBottle.getMaxVolume()\n");
      return ::SizedBottle_getMaxVolume(&me);
    }
  };
}


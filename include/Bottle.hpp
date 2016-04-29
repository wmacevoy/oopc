#pragma once

#include "Bottle.h"
namespace cxx {
  class Bottle {
  private: ::Bottle me;
  protected: enum UNINITIALIZED { UNINITIALIZED };
  protected: inline Bottle(enum UNINITIALIZED value) 
    { 
      printf("Bottle(UNINITIALIZED)\n"); 
    }    
  public: inline Bottle() 
    { 
      printf("Bottle()\n");
      ::Bottle_init(&me); 
    }
  public: inline ~Bottle() 
    { 
      printf("~Bottle()\n");
      ::Bottle_fini(&me); 
    }
  public: inline void setVolume(const double &value) 
    {
      printf("Bottle.setVolume(%lf)\n",value);
      ::Bottle_setVolume(&me,value);
    }
  public: inline double getVolume() const 
    {
      printf("Bottle.getVolume()\n");
      return ::Bottle_getVolume(&me);
    }
  };
}


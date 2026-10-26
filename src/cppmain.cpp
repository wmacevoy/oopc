#include <stdio.h>

#include "Bottle.hpp"
#include "SizedBottle.hpp"

/*
  Polymorphic through a base reference.  There is no C++ vtable pointer in these
  objects -- the override is selected by the C vtable, so passing a
  cxx::SizedBottle here runs SizedBottle's clamping setVolume.
*/
void test(cxx::Bottle &b)
{
  b.setVolume(1.0);
  printf("bottle volume=%lf\n",b.getVolume());
  b.setVolume(100.0);
  printf("bottle volume=%lf\n",b.getVolume());
}

int main()
{
  printf("sizeof(Bottle)=%zu\n",sizeof(Bottle));
  printf("sizeof(cxx::Bottle)=%zu\n",sizeof(cxx::Bottle));
  printf("sizeof(SizedBottle)=%zu\n",sizeof(SizedBottle));
  printf("sizeof(cxx::SizedBottle)=%zu\n",sizeof(cxx::SizedBottle));

  {
    cxx::Bottle b;
    /* Guaranteed equal: cxx::Bottle is standard-layout. */
    printf("&b=%p b.me()=%p same=%d\n",
           (void*)&b,(void*)b.me(),(void*)&b == (void*)b.me());
    test(b);
  }

  {
    cxx::SizedBottle sb;
    /* Equal on every real ABI; the size static_assert is what pins it down.
       me() itself never assumes it -- it downcasts Bottle::me(). */
    printf("&sb=%p sb.me()=%p same=%d\n",
           (void*)&sb,(void*)sb.me(),(void*)&sb == (void*)sb.me());
    printf("sb.Bottle::me()=%p (base view of the same bytes)\n",
           (void*)sb.Bottle::me());
    sb.setMaxVolume(10);
    test(sb);
    printf("max volume=%lf\n",sb.getMaxVolume());
  }

  return 0;
}

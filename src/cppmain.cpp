#include <stdio.h>

#include "Bottle.hpp"
#include "SizedBottle.hpp"

void testAndDelete(cxx::Bottle *b)
{
  b->setVolume(1.0);
  printf("bottle volume=%lf\n",b->getVolume());
  b->setVolume(100.0);
  printf("bottle volume=%lf\n",b->getVolume());
  delete b;
}

int main()
{
  printf("sizeof(Bottle)=%lu\n",sizeof(Bottle));
  printf("sizeof(cxx::Bottle)=%lu\n",sizeof(cxx::Bottle));
  printf("sizeof(SizedBottle)=%lu\n",sizeof(SizedBottle));
  printf("sizeof(cxx::SizedBottle)=%lu\n",sizeof(cxx::SizedBottle));

  {
    cxx::Bottle *b = new cxx::Bottle();
    printf("Bottle @ %p\n",b);
    testAndDelete(b);
  }

  {
    cxx::SizedBottle *sb = new cxx::SizedBottle();
    printf("SizedBottle @ %p\n",sb);
    sb->setMaxVolume(10);
    testAndDelete(sb);
  }

  return 0;
}

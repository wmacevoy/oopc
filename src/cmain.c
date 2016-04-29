#include <stdio.h>

#include "Bottle.h"
#include "SizedBottle.h"

void testAndDelete(Bottle *b)
{
  printf("bottle volume=%lf\n",Bottle_getVolume(b));
  Bottle_setVolume(b,0.0);
  printf("bottle volume=%lf\n",Bottle_getVolume(b));
  Bottle_setVolume(b,1.0);
  printf("bottle volume=%lf\n",Bottle_getVolume(b));
  Bottle_setVolume(b,100.0);
  printf("bottle volume=%lf\n",Bottle_getVolume(b));
  Bottle_delete(b);
}

int main()
{
  {
    printf("plain bottle\n");
    Bottle *b = Bottle_new();
    testAndDelete(b);
  }

  {
    printf("sized bottle\n");    
    SizedBottle *sb = SizedBottle_new();
    SizedBottle_setMaxVolume(sb,10);
    testAndDelete((Bottle*)sb);
  }

  return 0;
}

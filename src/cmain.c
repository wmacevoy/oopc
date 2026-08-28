#include <stdio.h>

#include "Bottle.h"
#include "SizedBottle.h"
#include "Type.h"

/* Takes the base type; the override is selected by the vtable. */
static void fill(Bottle *b)
{
  printf("  %-12s volume ", Type_of(b)->name);
  Bottle_setVolume(b,1.0);
  printf("%.1lf ", Bottle_getVolume(b));
  Bottle_setVolume(b,100.0);
  printf("-> %.1lf\n", Bottle_getVolume(b));
}

int main(void)
{
  Bottle *plain  = Bottle_new();
  Bottle *sized  = (Bottle*)SizedBottle_new();

  SizedBottle_setMaxVolume((SizedBottle*)sized,10);

  printf("dispatch through Bottle*\n");
  fill(plain);
  fill(sized);           /* clamps at 10 -- SizedBottle's setVolume runs */

  printf("\nruntime types\n");
  printf("  %-12s parent=%-12s size=%zu\n",
         Bottle_type.name,
         Bottle_type.parent ? Bottle_type.parent->name : "(root)",
         Bottle_type.size);
  printf("  %-12s parent=%-12s size=%zu\n",
         SizedBottle_type.name,
         SizedBottle_type.parent ? SizedBottle_type.parent->name : "(root)",
         SizedBottle_type.size);

  printf("\nchecked downcasts\n");
  printf("  SizedBottle_cast(plain) = %s  (a plain Bottle is not a SizedBottle)\n",
         SizedBottle_cast(plain) ? "ok" : "NULL");
  printf("  SizedBottle_cast(sized) = %s\n",
         SizedBottle_cast(sized) ? "ok" : "NULL");
  printf("  Bottle_cast(sized)      = %s  (a SizedBottle IS a Bottle)\n",
         Bottle_cast(sized) ? "ok" : "NULL");

  Bottle_delete(plain);
  Bottle_delete(sized);  /* virtual fini: SizedBottle's, then Bottle's */

  return 0;
}

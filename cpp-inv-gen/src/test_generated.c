/* REQ4/INV-3: the shared checks against the generated C.  Run: make test-cpp-inv-gen */

#include "SizedBottle.h"

#define CHECK_SIDE "generated C"
#include "checks.inc"

int main(void)
{
  return run_checks();
}

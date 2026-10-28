"""Tests / demo for the Python binding. Run: python3 python/test_oopc.py"""

import ctypes
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oopc  # noqa: E402


def check(label, got, want):
    assert got == want, "%s: got %r, want %r" % (label, got, want)
    print("  ok  %-46s %r" % (label, got))


print("dispatch: a Python subclass over a C hierarchy")
b = oopc.Bottle()
sb = oopc.SizedBottle()
sb.set_max_volume(10)

# set_volume/get_volume are INHERITED from the Python Bottle class and call the
# C function Bottle_setVolume. On the SizedBottle they still clamp, because the
# override is selected by the C vtable rather than by Python.
b.set_volume(100)
sb.set_volume(100)
check("REQ2/BND-1 Bottle.set_volume(100) -> volume", b.get_volume(), 100.0)
check("REQ2/BND-1 SizedBottle.set_volume(100) -> volume", sb.get_volume(), 10.0)
check("REQ2/BND-1 inherited method reached the override",
      type(sb).get_volume is type(b).get_volume, True)

sb.set_volume(8)
sb.set_max_volume(3)
check("REQ2/OM-6 lowering the max lowers the volume", sb.get_volume(), 3.0)

print("\nruntime types")
check("REQ2/TY-1 Bottle.c_type().name", oopc.Bottle.c_type().name, b"Bottle")
check("REQ2/TY-1 SizedBottle.c_type().name", oopc.SizedBottle.c_type().name, b"SizedBottle")
check("REQ2/TY-1 SizedBottle parent", oopc.SizedBottle.c_type().parent.contents.name, b"Bottle")
check("REQ2/TY-1 Bottle is a root", bool(oopc.Bottle.c_type().parent), False)
check("REQ2/TY-1 dynamic type of sb", sb.type_name, "SizedBottle")

print("\nchecked downcasts")
check("REQ2/BND-2 SizedBottle.cast(plain Bottle)", oopc.SizedBottle.cast(b), None)
check("REQ2/BND-2 SizedBottle.cast(SizedBottle) is not None",
      oopc.SizedBottle.cast(sb) is not None, True)
check("REQ2/BND-2 Bottle.cast(SizedBottle) is not None",
      oopc.Bottle.cast(sb) is not None, True)

print("\nhost-owned memory (why Type carries size)")
# The C library never allocates here. Python allocates the bytes and calls
# init/fini on them -- which is what the Class_init/Class_fini split is for.
size = oopc.SizedBottle.c_type().size
buf = ctypes.create_string_buffer(size)
oopc._lib.SizedBottle_init(ctypes.addressof(buf))
view = oopc.SizedBottle(ctypes.addressof(buf), owned=False)
view.set_max_volume(3)
view.set_volume(99)
check("REQ2/BND-2 SizedBottle.c_type().size", size, 24)
check("REQ2/BND-2 clamped in Python-owned memory", view.get_volume(), 3.0)
check("REQ2/BND-2 still reports its type", view.type_name, "SizedBottle")
oopc._lib.SizedBottle_fini(ctypes.addressof(buf))

print("\ncontext manager")
with oopc.SizedBottle() as tmp:
    tmp.set_max_volume(2)
    tmp.set_volume(5)
    check("with-block value", tmp.get_volume(), 2.0)

b.close()
sb.close()
print("\nPython: all assertions passed")

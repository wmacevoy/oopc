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
check("Bottle.set_volume(100) -> volume", b.get_volume(), 100.0)
check("SizedBottle.set_volume(100) -> volume", sb.get_volume(), 10.0)
check("inherited method reached the override",
      type(sb).get_volume is type(b).get_volume, True)

print("\nruntime types")
check("Bottle.c_type().name", oopc.Bottle.c_type().name, b"Bottle")
check("SizedBottle.c_type().name", oopc.SizedBottle.c_type().name, b"SizedBottle")
check("SizedBottle parent", oopc.SizedBottle.c_type().parent.contents.name, b"Bottle")
check("Bottle is a root", bool(oopc.Bottle.c_type().parent), False)
check("dynamic type of sb", sb.type_name, "SizedBottle")

print("\nchecked downcasts")
check("SizedBottle.cast(plain Bottle)", oopc.SizedBottle.cast(b), None)
check("SizedBottle.cast(SizedBottle) is not None",
      oopc.SizedBottle.cast(sb) is not None, True)
check("Bottle.cast(SizedBottle) is not None",
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
check("SizedBottle.c_type().size", size, 24)
check("clamped in Python-owned memory", view.get_volume(), 3.0)
check("still reports its type", view.type_name, "SizedBottle")
oopc._lib.SizedBottle_fini(ctypes.addressof(buf))

print("\ncontext manager")
with oopc.SizedBottle() as tmp:
    tmp.set_max_volume(2)
    tmp.set_volume(5)
    check("with-block value", tmp.get_volume(), 2.0)

b.close()
sb.close()
print("\nPython: all assertions passed")

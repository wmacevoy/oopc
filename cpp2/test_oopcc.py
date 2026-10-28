"""
Tests for the cpp-2 generator.  Run: python3 cpp2/test_oopcc.py

Two halves, and the second is the important one.

POSITIVE: the generator turns ordinary C++ into C that compiles warning-free
under -Wall -Wextra -Wpedantic -Werror and behaves the same.

NEGATIVE: everything outside the flattenable subset is REFUSED, with a message
that says why.  A generator that silently emitted plausible-looking C for a
construct it does not really support would be worse than no generator at all --
the whole value of cpp-2 is that the C is trustworthy without reading it.
"""

import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
OOPCC = os.path.join(HERE, "oopcc.py")

passed = 0


def run(sources, out):
    return subprocess.run([sys.executable, OOPCC] + sources + ["-o", out],
                          capture_output=True, text=True)


def accepts(label, code, main=None, expect=None):
    """The generator flattens `code`, and the C compiles and runs."""
    global passed
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "T.hpp")
        open(src, "w").write("#pragma once\n" + code)
        r = run([src], d)
        assert r.returncode == 0, "%s: generator failed\n%s" % (label, r.stderr)

        cs = sorted(os.path.join(d, f) for f in os.listdir(d) if f.endswith(".c"))
        warnings = ["-Wall", "-Wextra", "-Wpedantic", "-Werror"]
        includes = ["-I" + d, "-I" + os.path.join(ROOT, "include")]
        cc = os.environ.get("CC", "cc")
        exe = os.path.join(d, "a.out")
        if main is None:
            argv = [cc] + warnings + includes + ["-c", "-o", os.path.join(d, "o.o")] + cs
        else:
            m = os.path.join(d, "m.c")
            open(m, "w").write(main)
            argv = ([cc] + warnings + includes + ["-o", exe] + cs + [m] +
                    [os.path.join(ROOT, "src", "Type.c"),
                     os.path.join(ROOT, "src", "Trace.c")])
        out = subprocess.run(argv, capture_output=True, text=True)
        assert out.returncode == 0, "%s: generated C did not compile\n%s" % (label, out.stderr)

        if main is not None:
            got = subprocess.run([exe], capture_output=True, text=True).stdout
            assert got == expect, "%s: got %r, want %r" % (label, got, expect)
    print("  ok  %-52s accepted" % label)
    passed += 1


def refuses(label, code, because):
    """The generator refuses `code` and says why."""
    global passed
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "T.hpp")
        open(src, "w").write("#pragma once\n" + code)
        r = run([src], d)
        assert r.returncode == 2, \
            "%s: expected a refusal, got returncode %d\n%s%s" % (
                label, r.returncode, r.stdout, r.stderr)
        assert because in r.stderr, \
            "%s: refusal did not mention %r\n%s" % (label, because, r.stderr)
        assert not [f for f in os.listdir(d) if f.endswith((".c", ".h"))], \
            "%s: refused but still wrote files" % label
    print("  ok  %-52s refused" % label)
    passed += 1


OK_BASE = """
class A {
public:
  A() { x = 0; }
  virtual ~A() {}
  virtual int get() const { return x; }
  virtual void set(int v) { x = v; }
protected:
  int x;
};
"""

print("accepts the flattenable subset")

accepts("plain class", OK_BASE)

accepts("single inheritance, override, chain to base", OK_BASE + """
class B : public A {
public:
  B() { lo = 10; }
  virtual ~B() {}
  virtual void set(int v) { if (v < lo) v = lo; A::set(v); }
  virtual int getLo() const { return lo; }
protected:
  int lo;
};
""", main="""
#include <stdio.h>
#include "B.h"
int main(void) {
  B *b = B_new();
  A *a = (A*)b;
  A_set(a, 3);                 /* dispatches into B's clamping override */
  printf("%d %s %d\\n", A_get(a), Type_of(a)->name, (int)sizeof(B));
  B_delete(b);
  return 0;
}
""", expect="10 B 16\n")

accepts("this->, implicit this, nested virtual call", """
class C {
public:
  C() { n = 0; this->m = 0; }
  virtual ~C() {}
  virtual int get() const { return this->n; }
  virtual void set(int v) { n = v; m = this->n; }
  virtual void bump() { set(get() + 1); }
protected:
  int n;
  int m;
};
""", main="""
#include <stdio.h>
#include "C.h"
int main(void) { C *c = C_new(); C_set(c, 5); C_bump(c); C_bump(c);
  printf("%d\\n", C_get(c)); C_delete(c); return 0; }
""", expect="7\n")

accepts("destructor body becomes the fini body", """
#include <stdio.h>
class D {
public:
  D() { n = 1; }
  virtual ~D() { printf("bye %d\\n", n); }
  virtual int get() const { return n; }
protected:
  int n;
};
""", main="""
#include "D.h"
int main(void) { D_delete(D_new()); return 0; }
""", expect="bye 1\n")

print("\nrefuses what it cannot flatten")

refuses("multiple inheritance", OK_BASE + """
class Z { public: Z() {} virtual ~Z() {} virtual int z() const { return 0; } protected: int q; };
class B : public A, public Z { public: B() {} virtual ~B() {} };
""", "single inheritance")

refuses("virtual base", OK_BASE + """
class B : public virtual A { public: B() {} virtual ~B() {} };
""", "virtual bases")

refuses("private base", OK_BASE + """
class B : private A { public: B() {} virtual ~B() {} };
""", "base must be public")

refuses("non-virtual method", """
class A { public: A() {} virtual ~A() {} int get() const { return x; } protected: int x; };
""", "is not virtual")

refuses("overloaded method", """
class A { public: A() {} virtual ~A() {}
  virtual void set(int v) { x = v; }
  virtual void set(double v) { x = (int)v; }
protected: int x; };
""", "overloaded")

refuses("non-virtual destructor", """
class A { public: A() {} ~A() {} virtual int get() const { return x; } protected: int x; };
""", "must be virtual")

refuses("constructor with arguments", """
class A { public: A(int v) { x = v; } virtual ~A() {} virtual int get() const { return x; } protected: int x; };
""", "default constructor")

refuses("copy constructor", """
class A { public: A() {} A(const A &o) { x = o.x; } virtual ~A() {}
  virtual int get() const { return x; } protected: int x; };
""", "copy/move constructor")

refuses("class-typed field", """
struct Pair { int a; int b; };   /* a struct: a helper type, not a model class */
class A { public: A() {} virtual ~A() {} virtual int get() const { return p.a; }
protected: Pair p; };
""", "cannot be flattened")

refuses("class-typed parameter", """
struct P { int v; };
class A { public: A() {} virtual ~A() {} virtual void take(P p) { x = p.v; }
protected: int x; };
""", "cannot be flattened")

refuses("reference parameter", """
class A { public: A() {} virtual ~A() {} virtual void take(int &v) { x = v; }
protected: int x; };
""", "cannot be flattened")

refuses("member template", """
class A { public: A() {} virtual ~A() {} virtual int get() const { return x; }
  template <class T> void put(T t) { x = (int)t; }
protected: int x; };
""", "templates")

print("\ncpp2: %d assertions passed" % passed)

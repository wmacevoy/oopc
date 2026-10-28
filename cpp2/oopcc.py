#!/usr/bin/env python3
"""
oopcc -- flatten single-inheritance polymorphic C++ to the oopc C pattern.

This is cpp-2: C++ is the source, C is the artifact.  The input is ORDINARY C++
(cpp2/model/*.hpp) -- `virtual`, a virtual destructor, single inheritance,
protected data, and nothing that knows this tool exists.  The output is C in
exactly the shape of src/Bottle.c: a const vtable whose slots all take the root
base type, a prefix-compatible struct, split init/fini, a Type, and the three
layers (vf_ implementations, table, dispatchers).

Everything the human maintains by hand in src/*.c and is warned about in
CLAUDE.md is emitted here instead:

  * a derived vtable restating every base slot, in order, same types
  * a derived struct restating every base field, in order
  * `SizedBottle *me = (SizedBottle*)base;` at the top of every override
  * chaining a fini to Bottle_vf_fini and never Bottle_fini
  * the TRACE lines

WHAT IS A MODEL CLASS.  Every `class` at file scope is one, and must satisfy the
subset.  A `struct` is left alone -- it is an ordinary helper type, the same
split C++ convention already uses.

THE SUBSET.  The generator refuses what it cannot flatten rather than emitting C
that quietly means something else.  See read_class() below for the exact rules;
cpp2/README.md explains why each one is there.

Usage:  python3 cpp2/oopcc.py cpp2/model/Bottle.hpp cpp2/model/SizedBottle.hpp -o cpp2/gen
"""

import argparse
import os
import subprocess
import sys

import clang.cindex as ci
from clang.cindex import CursorKind as CK


# ---------------------------------------------------------------------------
# libclang discovery.  The pip package `libclang` bundles its own; otherwise
# fall back to whatever the system toolchain shipped.
# ---------------------------------------------------------------------------

def configure_libclang():
    """
    Prefer the toolchain's own libclang: it has to parse the same standard
    library headers the compiler will, and a pip-installed libclang that is
    older than the installed SDK cannot always do that.  Fall back to whatever
    the `libclang` wheel bundles.
    """
    for path in (
        "/Library/Developer/CommandLineTools/usr/lib/libclang.dylib",
        "/opt/homebrew/opt/llvm/lib/libclang.dylib",
        "/usr/local/opt/llvm/lib/libclang.dylib",
        "/usr/lib/llvm-18/lib/libclang.so.1",
        "/usr/lib/x86_64-linux-gnu/libclang-18.so.1",
    ):
        if os.path.exists(path):
            try:
                ci.Config.set_library_file(path)
                ci.Index.create()
                return
            except Exception:
                ci.Config.loaded = False
    try:
        ci.Index.create()          # the wheel's bundled copy
        return
    except Exception:
        raise SystemExit("oopcc: no libclang found -- pip install libclang")


def probe(argv, flag):
    """[flag, value] from a compiler query, or [] if the query fails."""
    try:
        out = subprocess.run(argv, capture_output=True, text=True, check=True)
        value = out.stdout.strip()
        return [flag, value] if value and os.path.exists(value) else []
    except Exception:
        return []


class Unsupported(Exception):
    """A construct outside the flattenable subset."""


def where(cursor):
    loc = cursor.location
    return "%s:%d:%d" % (os.path.basename(loc.file.name) if loc.file else "?",
                         loc.line, loc.column)


# ---------------------------------------------------------------------------
# The model, as read out of the C++
# ---------------------------------------------------------------------------

class Method:
    def __init__(self, cursor, owner):
        self.cursor = cursor
        self.owner = owner                      # class name that declares it
        self.name = cursor.spelling
        self.const = cursor.is_const_method()
        self.result = cursor.result_type.spelling
        self.params = [(a.type.spelling, a.spelling) for a in cursor.get_arguments()]

    def slot_decl(self, root):
        """The function-pointer member as it appears in the vtable struct."""
        me = "const %s *me" % root if self.const else "%s *me" % root
        args = ", ".join([me] + ["%s %s" % (t, n) for t, n in self.params])
        return "%s (*%s)(%s);" % (self.result, self.name, args)

    def impl_signature(self, root):
        me = "const %s *me" % root if self.const else "%s *me" % root
        args = ", ".join([me] + ["%s %s" % (t, n) for t, n in self.params])
        return "%s %s_vf_%s(%s)" % (self.result, self.owner, self.name, args)

    def dispatch_signature(self, cls):
        me = "const %s *me" % cls if self.const else "%s *me" % cls
        args = ", ".join([me] + ["%s %s" % (t, n) for t, n in self.params])
        return "%s %s_%s(%s)" % (self.result, cls, self.name, args)


class Klass:
    def __init__(self, cursor):
        self.cursor = cursor
        self.name = cursor.spelling
        self.base = None            # Klass, filled in by resolve()
        self.base_name = None
        self.fields = []            # (type, name) declared by THIS class
        self.methods = []           # virtual methods declared by THIS class
        self.ctor = None            # the default constructor cursor
        self.dtor = None            # the virtual destructor cursor

    # -- derived views ------------------------------------------------------

    @property
    def root(self):
        k = self
        while k.base is not None:
            k = k.base
        return k.name

    @property
    def chain(self):
        """Base-first list of classes from the root down to self."""
        return (self.base.chain if self.base else []) + [self]

    def all_fields(self):
        out = []
        for k in self.chain:
            out += k.fields
        return out

    def slots(self):
        """
        The vtable, in order: every base slot first (invariant 1 in CLAUDE.md),
        then this class's new methods.  An override replaces the OWNER of an
        inherited slot and nothing else, so the position never moves.
        """
        out = []
        for k in self.chain:
            for m in k.methods:
                for i, have in enumerate(out):
                    if have.name == m.name:
                        out[i] = m          # override: same slot, new owner
                        break
                else:
                    out.append(m)
        return out


# ---------------------------------------------------------------------------
# Subset enforcement
# ---------------------------------------------------------------------------

SCALARS = {"double", "float", "int", "unsigned int", "long", "unsigned long",
           "char", "unsigned char", "short", "unsigned short", "size_t",
           "long long", "unsigned long long", "_Bool", "void"}


def scalar(t):
    return t.replace("const ", "").strip() in SCALARS


def check_type(t, cursor, what):
    if not scalar(t):
        raise Unsupported(
            "%s: %s has type '%s'.  The C ABI can only carry scalars across "
            "the boundary; class types, references and templates cannot be "
            "flattened without inventing an ownership rule." % (where(cursor), what, t))


def read_class(cursor, path):
    k = Klass(cursor)
    seen = set()

    for c in cursor.get_children():
        if c.kind == CK.CXX_BASE_SPECIFIER:
            if k.base_name is not None:
                raise Unsupported(
                    "%s: %s has more than one base.  oopc is single inheritance: "
                    "a second base has no prefix to live in."
                    % (where(c), k.name))
            if "virtual" in [t.spelling for t in c.get_tokens()]:
                raise Unsupported(
                    "%s: virtual bases have no oopc image -- the base subobject "
                    "would not sit at a fixed prefix offset." % where(c))
            if c.access_specifier.name != "PUBLIC":
                raise Unsupported(
                    "%s: base must be public -- a C upcast is a plain pointer "
                    "cast and cannot be access-checked." % where(c))
            k.base_name = c.type.spelling

        elif c.kind == CK.FIELD_DECL:
            check_type(c.type.spelling, c, "field '%s'" % c.spelling)
            k.fields.append((c.type.spelling, c.spelling))

        elif c.kind == CK.CONSTRUCTOR:
            if c.is_copy_constructor() or c.is_move_constructor():
                raise Unsupported(
                    "%s: %s has a copy/move constructor.  oopc has no notion of "
                    "copying an object; give the class a clone slot instead."
                    % (where(c), k.name))
            if len(list(c.get_arguments())) != 0:
                raise Unsupported(
                    "%s: only a default constructor can become Class_init -- a C "
                    "host allocating from Type::size has no arguments to pass."
                    % where(c))
            if k.ctor is not None:
                raise Unsupported("%s: more than one constructor." % where(c))
            k.ctor = c

        elif c.kind == CK.DESTRUCTOR:
            if not c.is_virtual_method():
                raise Unsupported(
                    "%s: ~%s must be virtual -- it becomes the fini slot, and a "
                    "non-virtual one could not run the derived teardown."
                    % (where(c), k.name))
            k.dtor = c

        elif c.kind == CK.CXX_METHOD:
            if c.is_static_method():
                continue
            if not c.is_virtual_method():
                raise Unsupported(
                    "%s: %s::%s is not virtual.  Every method must be virtual: a "
                    "non-virtual method has no slot, so a host language calling "
                    "through the C ABI could not reach the right one."
                    % (where(c), k.name, c.spelling))
            if c.spelling in seen:
                raise Unsupported(
                    "%s: %s::%s is overloaded.  C has one name per function; "
                    "give the overloads distinct names."
                    % (where(c), k.name, c.spelling))
            seen.add(c.spelling)
            check_type(c.result_type.spelling, c, "return of '%s'" % c.spelling)
            for a in c.get_arguments():
                check_type(a.type.spelling, a, "parameter '%s'" % a.spelling)
            k.methods.append(Method(c, k.name))

        elif c.kind in (CK.CLASS_TEMPLATE, CK.FUNCTION_TEMPLATE):
            raise Unsupported(
                "%s: templates have no oopc image -- only instantiations could "
                "cross, and C cannot name one." % where(c))

    if k.ctor is None:
        raise Unsupported("%s: %s needs a default constructor to become "
                          "%s_init." % (where(cursor), k.name, k.name))
    if k.dtor is None:
        raise Unsupported("%s: %s needs a virtual destructor to become the fini "
                          "slot." % (where(cursor), k.name))
    return k


# ---------------------------------------------------------------------------
# Body translation
#
# A method body is lifted verbatim and three things are rewritten:
#   this->x / x  (a field)      ->  me->x
#   this                        ->  me
#   Base::m(a)                  ->  Base_vf_m((Base *)me, a)     (non-virtual)
#   m(a)         (own virtual)  ->  Class_m(me, a)               (virtual)
# Anything else must already be valid C; the generator does not check that, the
# C compiler does.
# ---------------------------------------------------------------------------

class Body:
    def __init__(self, source_bytes, klass, classes):
        self.src = source_bytes
        self.klass = klass
        self.classes = classes
        self.edits = []          # (start, end, replacement)

    def text(self, cursor):
        e = cursor.extent
        return self.src[e.start.offset:e.end.offset].decode("utf-8")

    def walk(self, cursor):
        k = cursor.kind

        if k == CK.MEMBER_REF_EXPR:
            ref = cursor.referenced
            if ref is not None and ref.kind == CK.FIELD_DECL:
                e = cursor.extent
                self.edits.append((e.start.offset, e.end.offset, "me->" + ref.spelling))
                return                       # covers the implicit `this` too

        if k == CK.CALL_EXPR:
            ref = cursor.referenced
            if ref is not None and ref.kind == CK.CXX_METHOD:
                self.call(cursor, ref)
                return

        if k == CK.CXX_THIS_EXPR:
            e = cursor.extent
            if e.end.offset > e.start.offset:
                self.edits.append((e.start.offset, e.end.offset, "me"))
            return

        for c in cursor.get_children():
            self.walk(c)

    def call(self, cursor, ref):
        owner = ref.semantic_parent.spelling
        children = list(cursor.get_children())
        if not children:
            raise Unsupported("%s: cannot read the callee of this call." % where(cursor))
        callee, args = children[0], children[1:]
        ce = callee.extent

        qualified = "::" in self.text(callee)
        if qualified:
            # Bottle::setVolume(v) -- a non-virtual, chain-to-base call.  This is
            # the one src/SizedBottle.c warns about: it MUST become the _vf_
            # implementation, never the dispatcher, or it recurses forever.
            name = "%s_vf_%s" % (owner, ref.spelling)
            recv = "(%s *)me" % self.klass.root
        else:
            # An unqualified call to a virtual method is a virtual call, so it
            # goes through the dispatcher.
            name = "%s_%s" % (self.klass.name, ref.spelling)
            recv = "me"

        if ce.end.offset <= ce.start.offset:
            raise Unsupported(
                "%s: implicit-receiver call to '%s' has no source range; write "
                "it as this->%s(...)." % (where(cursor), ref.spelling, ref.spelling))

        self.edits.append((ce.start.offset, ce.end.offset, name))

        # Insert the receiver just after the '(' that opens the argument list.
        i = self.src.index(b"(", ce.end.offset)
        sep = ", " if args else ""
        self.edits.append((i + 1, i + 1, recv + sep))

        for a in args:
            self.walk(a)

    def render(self, cursor, indent="  "):
        """The compound statement of `cursor`, translated, braces stripped."""
        body = None
        for c in cursor.get_children():
            if c.kind == CK.COMPOUND_STMT:
                body = c
        if body is None:
            return ""
        self.edits = []
        for c in body.get_children():
            self.walk(c)

        e = body.extent
        start, end = e.start.offset, e.end.offset
        text = bytearray(self.src[start:end])
        # Apply back to front so earlier offsets do not shift.  At a shared
        # start offset the wider edit must go first: an inserted receiver sits
        # at the same offset as the callee it precedes, and applying the
        # insertion first would leave the replacement overwriting it.
        edits = sorted(self.edits, key=lambda x: (-x[0], -x[1]))
        last = None
        for a, b, rep in edits:
            if last is not None and b > last:
                raise Unsupported(
                    "%s: overlapping rewrites in this body -- the construct is "
                    "outside the flattenable subset." % where(cursor))
            last = a
            text[a - start:b - start] = rep.encode("utf-8")
        text = text.decode("utf-8").strip()
        assert text.startswith("{") and text.endswith("}")
        text = text[1:-1]

        lines = [ln.rstrip() for ln in text.split("\n")]
        while lines and not lines[0].strip():
            lines.pop(0)
        while lines and not lines[-1].strip():
            lines.pop()
        if not lines:
            return ""
        pad = min((len(ln) - len(ln.lstrip()) for ln in lines if ln.strip()), default=0)
        return "\n".join(indent + ln[pad:] if ln.strip() else "" for ln in lines)


# ---------------------------------------------------------------------------
# Emission
# ---------------------------------------------------------------------------

BANNER = """/*
 * GENERATED by cpp2/oopcc.py from %s -- do not edit.
 *
 * This is the oopc C pattern, emitted from ordinary single-inheritance
 * polymorphic C++.  Read it against src/%s.c: the shape is the same, but every
 * hand-maintained invariant in that file is machine-maintained here.
 */
"""


def emit_header(k, src_name):
    root = k.root
    n = k.name
    L = [BANNER % (src_name, n), "", "#pragma once", ""]
    L.append('#include "%s.h"' % k.base.name if k.base else '#include "Type.h"')
    L += ["", "#ifdef __cplusplus", 'extern "C" {', "#endif", ""]
    L += ["  struct %sStruct;" % n, "  typedef struct %sStruct %s;" % (n, n), ""]

    L.append("  /* Every slot takes %s * -- the ROOT base type -- so a derived class" % root)
    L.append("     inherits a slot by naming the same function pointer, with no cast. */")
    L.append("  typedef struct {")
    L.append("    const Type *type;")
    L.append("    void (*fini)(%s *me);" % root)
    for m in k.slots():
        L.append("    " + m.slot_decl(root))
    L.append("  } %sVftbl;" % n)
    L.append("")

    L.append("  /* The base's fields come first, in the base's order: an upcast is a")
    L.append("     plain pointer cast. */")
    L.append("  struct %sStruct {" % n)
    L.append("    const %sVftbl *vftbl;" % n)
    for t, f in k.all_fields():
        L.append("    %s %s;" % (t, f))
    L.append("  };")
    L.append("")

    L += ["  %s *%s_new(void);" % (n, n),
          "  void %s_delete(%s *me);" % (n, n),
          "  void %s_init(%s *me);" % (n, n),
          "  void %s_fini(%s *me);" % (n, n), ""]
    for m in k.slots():
        L.append("  %s;" % m.dispatch_signature(n))
    L += ["",
          "  %s *%s_cast(void *object);" % (n, n),
          "  const Type *%s_getType(void);" % n, "",
          "  /* \"protected\": named by a derived class's static initializer. */",
          "  extern const Type %s_type;" % n,
          "  extern const %sVftbl %s_vftbl;" % (n, n), ""]
    L.append("  void %s_vf_fini(%s *me);" % (n, root))
    for m in k.methods:
        L.append("  %s;" % m.impl_signature(root))
    L += ["", "#ifdef __cplusplus", "}", "#endif", ""]
    return "\n".join(L)


def system_includes(source_bytes):
    """
    <angle> includes from the model are carried into the generated C: a body
    that calls printf needs stdio.h wherever it ends up.  "quoted" includes are
    NOT carried -- those name other model headers, which become their own
    generated pair.
    """
    out = []
    for line in source_bytes.decode("utf-8").split("\n"):
        t = line.strip()
        if t.startswith("#include") and "<" in t and t not in out:
            out.append(t)
    return out


def emit_source(k, src_name, body):
    root, n = k.root, k.name
    derived = k.base is not None
    L = [BANNER % (src_name, n), "", "#include <stdlib.h>"]
    L += system_includes(body.src)
    L += ["", '#include "%s.h"' % n, '#include "Trace.h"', ""]

    def downcast():
        """The line every override in src/SizedBottle.c writes by hand."""
        return ["  %s *me = (%s *)base;" % (n, n)] if derived else []

    # -- fini: the destructor, plus the chain to the base ------------------
    arg = "%s *base" % root if derived else "%s *me" % root
    L.append("void %s_vf_fini(%s)" % (n, arg))
    L.append("{")
    L += downcast()
    L.append('  TRACE("%s vf fini(%%p)\\n",(void*)me);' % n)
    dtor_body = body.render(k.dtor)
    if dtor_body:
        L.append(dtor_body)
    if derived:
        L.append("  /* Chain to the base: a direct, NON-virtual call.  %s_fini here"
                 % k.base.name)
        L.append("     would re-dispatch through the vtable and recurse forever. */")
        L.append("  %s_vf_fini((%s *)me);" % (k.base.name, root))
    L += ["}", ""]

    # -- the implementations this class declares ---------------------------
    for m in k.methods:
        cst = "const " if m.const else ""
        arg = "%s%s *base" % (cst, root) if derived else "%s%s *me" % (cst, root)
        args = ", ".join([arg] + ["%s %s" % (t, p) for t, p in m.params])
        L.append("%s %s_vf_%s(%s)" % (m.result, n, m.name, args))
        L.append("{")
        if derived:
            L.append("  %s%s *me = (%s%s *)base;" % (cst, n, cst, n))
        L.append('  TRACE("%s vf %s(%%p)\\n",(const void*)me);' % (n, m.name))
        b = body.render(m.cursor)
        if b:
            L.append(b)
        L += ["}", ""]

    # -- class constants ---------------------------------------------------
    parent = "&%s_type" % k.base.name if derived else "0"
    L.append("const Type %s_type = { \"%s\", %s, sizeof(%s) };" % (n, n, parent, n))
    L.append("")
    L.append("/* Slots resolved to the most-derived implementation.  An inherited slot")
    L.append("   names the BASE's function verbatim -- no cast, because every slot takes")
    L.append("   %s *. */" % root)
    L.append("const %sVftbl %s_vftbl = {" % (n, n))
    L.append("  &%s_type," % n)
    L.append("  %s_vf_fini," % n)
    L.append(",\n".join("  %s_vf_%s" % (m.owner, m.name) for m in k.slots()))
    L += ["};", ""]

    # -- lifecycle ---------------------------------------------------------
    L += ["%s *%s_new(void)" % (n, n),
          "{",
          "  %s *me = (%s *)malloc(sizeof(%s));" % (n, n, n),
          '  TRACE("%s_new()=%%p\\n",(void*)me);' % n,
          "  if (me != 0) { %s_init(me); }" % n,
          "  return me;",
          "}", "",
          "void %s_delete(%s *me)" % (n, n),
          "{",
          '  TRACE("%s_delete(%%p)\\n",(void*)me);' % n,
          "  if (me != 0) {",
          "    %s_fini(me);" % n,
          "    free(me);",
          "  }",
          "}", "",
          "void %s_init(%s *me)" % (n, n),
          "{",
          '  TRACE("%s_init(%%p)\\n",(void*)me);' % n]
    if derived:
        L.append("  %s_init((%s *)me);" % (k.base.name, k.base.name))
    L.append("  me->vftbl = &%s_vftbl;" % n)
    ctor_body = body.render(k.ctor)
    if ctor_body:
        L.append(ctor_body)
    L += ["}", "",
          "void %s_fini(%s *me)" % (n, n),
          "{",
          '  TRACE("%s_fini(%%p)\\n",(void*)me);' % n,
          "  me->vftbl->fini((%s *)me);" % root,
          "}", ""]

    # -- dispatchers -------------------------------------------------------
    for m in k.slots():
        L.append(m.dispatch_signature(n))
        L.append("{")
        L.append('  TRACE("%s_%s(%%p)\\n",(const void*)me);' % (n, m.name))
        call = "me->vftbl->%s(%s(%s%s *)me%s)" % (
            m.name, "", "const " if m.const else "", root,
            "".join(", " + p for _, p in m.params))
        L.append("  %s%s;" % ("return " if m.result != "void" else "", call))
        L += ["}", ""]

    L += ["%s *%s_cast(void *object)" % (n, n),
          "{",
          "  return (%s *)Type_cast(object,&%s_type);" % (n, n),
          "}", "",
          "const Type *%s_getType(void)" % n,
          "{",
          "  return &%s_type;" % n,
          "}", ""]
    return "\n".join(L)


# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description="flatten C++ to the oopc C pattern")
    ap.add_argument("sources", nargs="+")
    ap.add_argument("-o", "--out", default="cpp2/gen")
    ap.add_argument("-I", "--include", action="append", default=[])
    ap.add_argument("--std", default="c++17")
    args = ap.parse_args()

    configure_libclang()
    index = ci.Index.create()

    # libclang's parse() does not apply the driver's default sysroot, so on
    # macOS a model that includes <stdio.h> would not parse.  Ask xcrun once.
    # libclang's parse() is not the clang driver: it applies neither the
    # default sysroot nor the builtin-header resource directory, so a model
    # that includes <stdio.h> would not parse.  Ask the real compiler once.
    sysroot = []
    if sys.platform == "darwin":
        sysroot += probe(["xcrun", "--show-sdk-path"], "-isysroot")
    sysroot += probe([os.environ.get("CC", "cc"), "-print-resource-dir"],
                     "-resource-dir")
    os.makedirs(args.out, exist_ok=True)

    classes = {}
    order = []
    for path in args.sources:
        flags = (["-x", "c++", "-std=" + args.std] + sysroot +
                 ["-I" + i for i in args.include])
        tu = index.parse(path, args=flags)
        fatal = [d for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if fatal:
            for d in fatal:
                print("oopcc: %s" % d, file=sys.stderr)
            return 1
        for c in tu.cursor.get_children():
            if (c.kind == CK.CLASS_DECL and c.is_definition()
                    and c.location.file
                    and os.path.abspath(c.location.file.name) == os.path.abspath(path)):
                k = read_class(c, path)
                classes[k.name] = k
                order.append((k, path))

    for k, _ in order:
        if k.base_name:
            if k.base_name not in classes:
                raise Unsupported("%s: base '%s' was not among the sources."
                                  % (k.name, k.base_name))
            k.base = classes[k.base_name]

    for k, path in order:
        src = open(path, "rb").read()
        body = Body(src, k, classes)
        name = os.path.basename(path)
        open(os.path.join(args.out, k.name + ".h"), "w").write(emit_header(k, name))
        open(os.path.join(args.out, k.name + ".c"), "w").write(emit_source(k, name, body))
        print("oopcc: %s -> %s/%s.{h,c}" % (name, args.out, k.name))
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Unsupported as e:
        print("oopcc: outside the flattenable subset\n  %s" % e, file=sys.stderr)
        sys.exit(2)

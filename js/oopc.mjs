/*
 * JavaScript binding for the oopc single-inheritance C pattern.
 *
 * No glue module and no per-class C code -- this drives the compiled wasm
 * exports directly.  The C ABI *is* the binding.
 *
 * The point of the exercise: JavaScript's own single inheritance lines up 1:1
 * with the C hierarchy.  SizedBottle below declares only the methods it ADDS --
 * it has no constructor at all -- and inherits setVolume/getVolume from Bottle.
 * Those call the C functions Bottle_setVolume/Bottle_getVolume, which dispatch
 * through the C vtable and land in SizedBottle's clamping override.  JS supplies
 * the class hierarchy, C supplies the virtual dispatch.
 *
 * Works with either an Emscripten Module (functions at Module._Name) or a raw
 * WebAssembly instance (functions at instance.exports.Name).
 */

export function bind(module) {
  const ex = module.exports ?? module;
  const fn = (name) => {
    const f = ex["_" + name] ?? ex[name];
    if (typeof f !== "function") throw new Error(`wasm export missing: ${name}`);
    return f;
  };

  // The memory object moves when it grows, so views are made per access.
  const buffer = () =>
    module.HEAPU8 ? module.HEAPU8.buffer : (ex.memory ?? module.wasmMemory).buffer;

  const decoder = new TextDecoder();
  function cstring(ptr) {
    const u8 = new Uint8Array(buffer());
    let end = ptr;
    while (u8[end] !== 0) end++;
    return decoder.decode(u8.subarray(ptr, end));
  }

  /* struct TypeStruct on wasm32: char* name; Type* parent; size_t size. */
  function readType(ptr) {
    if (!ptr) return null;
    const dv = new DataView(buffer());
    return {
      address: ptr,
      name: cstring(dv.getUint32(ptr, true)),
      parent: dv.getUint32(ptr + 4, true),
      size: dv.getUint32(ptr + 8, true),
    };
  }

  const typeOf = (ptr) => readType(fn("Type_of")(ptr));

  class Bottle {
    static _C = "Bottle";

    /* new.target is the class actually being constructed, so a subclass gets
       its own C constructor with no code of its own. */
    constructor(ptr, owned = true) {
      if (ptr === undefined) {
        ptr = fn(new.target._C + "_new")();
        if (!ptr) throw new Error(new.target._C + "_new failed");
        owned = true;
      }
      this._ptr = ptr;
      this._owned = owned;
    }

    /* Bottle_delete dispatches fini through the C vtable, so one deleter is
       correct for every class in the hierarchy. */
    close() {
      if (this._ptr && this._owned) fn("Bottle_delete")(this._ptr);
      this._ptr = 0;
    }

    /** The static Type of this class (its declared type). */
    static get cType() {
      return readType(fn(this._C + "_getType")());
    }

    /** The dynamic type -- what this object actually is. */
    get typeName() {
      return typeOf(this._ptr).name;
    }

    /** Checked downcast: a view of obj as this class, or null on mismatch. */
    static cast(obj) {
      const ptr = fn(this._C + "_cast")(obj._ptr);
      return ptr ? new this(ptr, false) : null;
    }

    setVolume(value) { fn("Bottle_setVolume")(this._ptr, value); }
    getVolume() { return fn("Bottle_getVolume")(this._ptr); }

    get volume() { return this.getVolume(); }
    set volume(v) { this.setVolume(v); }

    toString() { return `${this.typeName}(volume=${this.getVolume()})`; }
  }

  /* Only the NEW methods. No constructor: Bottle's uses new.target. */
  class SizedBottle extends Bottle {
    static _C = "SizedBottle";

    setMaxVolume(value) { fn("SizedBottle_setMaxVolume")(this._ptr, value); }
    getMaxVolume() { return fn("SizedBottle_getMaxVolume")(this._ptr); }

    get maxVolume() { return this.getMaxVolume(); }
    set maxVolume(v) { this.setMaxVolume(v); }
  }

  return { Bottle, SizedBottle, typeOf, readType };
}

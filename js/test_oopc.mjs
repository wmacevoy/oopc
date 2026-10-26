/* Tests / demo for the JS binding.  Run: node js/test_oopc.mjs */

import { existsSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import { bind } from "./oopc.mjs";
import { loadRaw, loadEmscripten } from "./load.mjs";

const root = join(dirname(fileURLToPath(import.meta.url)), "..");

let pass = 0;
function check(label, got, want) {
  const ok = Object.is(got, want);
  if (!ok) { console.error(`  FAIL ${label}: got ${got}, want ${want}`); process.exit(1); }
  console.log(`  ok  ${label.padEnd(46)} ${got}`);
  pass++;
}

async function load() {
  const em = join(root, "bin", "oopc.js");
  if (existsSync(em)) return ["emscripten", await loadEmscripten(em)];
  const raw = join(root, "bin", "oopc.raw.wasm");
  if (existsSync(raw)) return ["raw wasm", await loadRaw(raw)];
  console.error("no wasm build found -- run `make wasm` or `make wasm-raw`");
  process.exit(1);
}

const [flavor, module] = await load();
console.log(`module: ${flavor}\n`);

const { Bottle, SizedBottle } = bind(module);

console.log("dispatch: a JS subclass over a C hierarchy");
const b = new Bottle();
const sb = new SizedBottle();
sb.setMaxVolume(10);

// setVolume is INHERITED from the JS Bottle class and calls the C function
// Bottle_setVolume. On the SizedBottle it still clamps, because the override is
// selected by the C vtable rather than by JavaScript.
b.setVolume(100);
sb.setVolume(100);
check("REQ2/BND-1 Bottle.setVolume(100) -> volume", b.getVolume(), 100);
check("REQ2/BND-1 SizedBottle.setVolume(100) -> volume", sb.getVolume(), 10);
check("REQ2/BND-1 SizedBottle inherits setVolume",
      Object.getPrototypeOf(SizedBottle.prototype) === Bottle.prototype, true);
// SizedBottle's prototype carries ONLY the members it adds. Everything else --
// including construction, which routes through new.target -- is inherited.
check("REQ2/BND-1 SizedBottle adds only its new members",
      Object.getOwnPropertyNames(SizedBottle.prototype).sort().join(","),
      "constructor,getMaxVolume,maxVolume,setMaxVolume");

sb.setVolume(8);
sb.setMaxVolume(3);
check("REQ2/OM-6 lowering the max lowers the volume", sb.getVolume(), 3);

console.log("\nruntime types");
check("REQ2/TY-4 Bottle.cType.name", Bottle.cType.name, "Bottle");
check("REQ2/TY-4 SizedBottle.cType.name", SizedBottle.cType.name, "SizedBottle");
check("REQ2/TY-4 SizedBottle.cType.size", SizedBottle.cType.size, 24);
check("REQ2/TY-1 dynamic type of sb", sb.typeName, "SizedBottle");
check("REQ2/TY-1 dynamic type of b", b.typeName, "Bottle");

check("REQ2/TY-4 SizedBottle.cType.parent is Bottle's",
      SizedBottle.cType.parent, Bottle.cType.address);
check("REQ2/TY-4 Bottle is a root", Bottle.cType.parent, 0);

console.log("\nchecked downcasts");
check("REQ2/BND-2 SizedBottle.cast(plain Bottle)", SizedBottle.cast(b), null);
check("REQ2/BND-2 SizedBottle.cast(SizedBottle) is not null", SizedBottle.cast(sb) !== null, true);
check("REQ2/BND-2 Bottle.cast(SizedBottle) is not null", Bottle.cast(sb) !== null, true);
check("REQ2/BND-2 cast preserves dynamic type", Bottle.cast(sb).typeName, "SizedBottle");

console.log("\ninstanceof still works (real JS classes)");
check("REQ2/BND-1 sb instanceof SizedBottle", sb instanceof SizedBottle, true);
check("REQ2/BND-1 sb instanceof Bottle", sb instanceof Bottle, true);
check("REQ2/BND-1 b instanceof SizedBottle", b instanceof SizedBottle, false);

console.log("\nhost-owned memory (why Type carries size)");
{
  // Allocate the bytes on the host side and call init/fini on them; C's own
  // new/delete are never used.
  const ex = module.exports ?? module;              // raw instance or Emscripten
  const c = (name) => ex["_" + name] ?? ex[name];
  const malloc = module._malloc ?? module.malloc;
  const free = module._free ?? module.free;
  const ptr = malloc(SizedBottle.cType.size);
  c("SizedBottle_init")(ptr);
  const view = new SizedBottle(ptr, false);
  view.setMaxVolume(3);
  view.setVolume(99);
  check("REQ2/BND-2 clamped in host-owned memory", view.getVolume(), 3);
  check("REQ2/BND-2 still reports its type", view.typeName, "SizedBottle");
  c("SizedBottle_fini")(ptr);
  free(ptr);
}

b.close();
sb.close();
console.log(`\nJS: ${pass} assertions passed`);

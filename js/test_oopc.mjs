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
check("Bottle.setVolume(100) -> volume", b.getVolume(), 100);
check("SizedBottle.setVolume(100) -> volume", sb.getVolume(), 10);
check("SizedBottle inherits setVolume",
      Object.getPrototypeOf(SizedBottle.prototype) === Bottle.prototype, true);
// SizedBottle's prototype carries ONLY the members it adds. Everything else --
// including construction, which routes through new.target -- is inherited.
check("SizedBottle adds only its new members",
      Object.getOwnPropertyNames(SizedBottle.prototype).sort().join(","),
      "constructor,getMaxVolume,maxVolume,setMaxVolume");

console.log("\nruntime types");
check("Bottle.cType.name", Bottle.cType.name, "Bottle");
check("SizedBottle.cType.name", SizedBottle.cType.name, "SizedBottle");
check("SizedBottle.cType.size", SizedBottle.cType.size, 24);
check("dynamic type of sb", sb.typeName, "SizedBottle");
check("dynamic type of b", b.typeName, "Bottle");

console.log("\nchecked downcasts");
check("SizedBottle.cast(plain Bottle)", SizedBottle.cast(b), null);
check("SizedBottle.cast(SizedBottle) is not null", SizedBottle.cast(sb) !== null, true);
check("Bottle.cast(SizedBottle) is not null", Bottle.cast(sb) !== null, true);
check("cast preserves dynamic type", Bottle.cast(sb).typeName, "SizedBottle");

console.log("\ninstanceof still works (real JS classes)");
check("sb instanceof SizedBottle", sb instanceof SizedBottle, true);
check("sb instanceof Bottle", sb instanceof Bottle, true);
check("b instanceof SizedBottle", b instanceof SizedBottle, false);

b.close();
sb.close();
console.log(`\nJS: ${pass} assertions passed`);

/*
 * Loaders. The binding in oopc.mjs does not care which of these produced the
 * module -- it accepts either shape.
 */

async function readBytes(path) {
  if (typeof fetch === "function" && /^https?:|^\//.test(path) === false) {
    return new Uint8Array(await (await fetch(path)).arrayBuffer());
  }
  const { readFile } = await import("node:fs/promises");
  return await readFile(path);
}

/**
 * A raw wasm build (clang, no libc linked). malloc/free are imports, so the
 * HOST supplies them -- which is the point: the C layer never assumes an
 * allocator, and Type.size plus Class_init/Class_fini let the host place
 * objects wherever it likes.
 */
export async function loadRaw(path) {
  let memory = null;
  let brk = 0;
  const sizeOf = new Map();   // ptr -> rounded size
  const bins = new Map();     // rounded size -> free ptrs

  const malloc = (n) => {
    n = (n + 7) & ~7;
    const bin = bins.get(n);
    if (bin && bin.length) return bin.pop();
    const ptr = brk;
    brk += n;
    const need = brk - memory.buffer.byteLength;
    if (need > 0) memory.grow(Math.ceil(need / 65536));
    sizeOf.set(ptr, n);
    return ptr;
  };
  const free = (ptr) => {
    if (!ptr) return;
    const n = sizeOf.get(ptr);
    if (n === undefined) return;
    if (!bins.has(n)) bins.set(n, []);
    bins.get(n).push(ptr);
  };

  /* Supply what the module actually asks for, and stub anything else it
     imports (a -DDEBUG build also imports vprintf for the trace sink).
     A real host would point that at console.log; see Trace_to in Trace.h. */
  const provided = { malloc, free };
  const compiled = await WebAssembly.compile(await readBytes(path));
  const imports = {};
  for (const { module: m, name, kind } of WebAssembly.Module.imports(compiled)) {
    imports[m] ??= {};
    if (kind === "function") imports[m][name] = provided[name] ?? (() => 0);
  }
  const instance = await WebAssembly.instantiate(compiled, imports);
  memory = instance.exports.memory;
  brk = instance.exports.__heap_base?.value ?? 0x10000;
  return instance;
}

/**
 * An Emscripten build. Emscripten wires printf to console.log and stderr to
 * console.error, so a -DDEBUG build traces straight into the JS console.
 */
export async function loadEmscripten(modulePath) {
  // node needs a file:// URL to import an absolute path
  let spec = modulePath;
  if (typeof process !== "undefined" && /^[/\\]/.test(modulePath)) {
    const { pathToFileURL } = await import("node:url");
    spec = pathToFileURL(modulePath).href;
  }
  const { default: createOopc } = await import(spec);
  return await createOopc();
}

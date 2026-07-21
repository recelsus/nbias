# wasm/

Emscripten bindings for `nbias_core`, meant to be consumed by other projects (a Chrome
extension, some other browser tool, etc.) — not used by the `nbias` CLI itself. Anything
specific to a particular consumer (UI, manifest, DOM integration) belongs in that consumer's
own repo, not here; this only translates `nbias_core`'s C++ API into a JS-callable ABI.

See `reference/requirements.md` section 15 for the background, and the memory file
`nbias_wasm_architecture` for why this lives here instead of in a downstream project.

## Building

Requires the Emscripten SDK (`emcmake`/`emcc` on `PATH`, e.g. via
`source /path/to/emsdk/emsdk_env.sh`).

```bash
emcmake cmake -S . -B build-wasm -DNBIAS_BUILD_WASM=ON
cmake --build build-wasm --target nbias_wasm
```

Produces `build-wasm/nbias_core.mjs` (an ES6 module factory, `MODULARIZE=1 -sEXPORT_ES6=1`,
export name `createNbiasCoreModule`) and `build-wasm/nbias_core.wasm`.

The first configure also fetches libsodium's source and builds it for wasm32 via libsodium's
own `dist-build/emscripten.sh --sumo` script — this takes a few minutes the first time, and is
cached afterward.

### Test

```bash
cmake --build build-wasm --target nbias_wasm
ctest --test-dir build-wasm --output-on-failure
```

`wasm_smoke` (registered only when `NBIAS_BUILD_WASM=ON`) runs `wasm/test/smoke_test.mjs`
under Node (bundled with the Emscripten SDK) and exercises the full round trip described below.

## Binding style

Plain `extern "C"` exports (`EMSCRIPTEN_KEEPALIVE`), not Embind — deliberately, for a smaller
build and no Embind runtime overhead, at the cost of manual pointer/length marshaling on the
JS side. Single-threaded (no pthreads), so no `SharedArrayBuffer`/COOP/COEP requirement on the
hosting page.

Built with `-fwasm-exceptions` (the native WebAssembly exception-handling proposal, not the
older JS-based `-fexceptions` emulation). Wasm disables exception catching by default; without
this flag, a thrown `vault_format_error`/`vault_auth_error` aborts the whole module instead of
being caught by the `try`/`catch` in `bindings.cpp`. Both `nbias_core` and `wasm/src/bindings.cpp`
are compiled with this flag (`nbias_core`'s `target_compile_options` sets it `PUBLIC` so it
propagates automatically). This requires a reasonably modern wasm runtime; Node ≥ 18 and
current Chrome/Firefox/Safari all support it.

## API

All functions are declared in `wasm/src/bindings.cpp`. Every function that returns a status
uses this convention:

- Return value `0` = success (`nbias_wasm_ok`).
- `1` = format error (corrupted/malformed vault — `vault_format_error` on the C++ side).
- `2` = auth error (wrong password, or tampered ciphertext — `vault_auth_error`).
- `3` = unexpected internal error.
- On any non-zero return, call `nbias_wasm_last_error_message()` for a human-readable message
  (valid only until the next call into the module).

### Memory ownership

Every non-null pointer this API hands back through an out-parameter was allocated with
`malloc()` inside the module and **must** be released by the caller with
`nbias_wasm_free_buffer(ptr)` once no longer needed. Buffers you pass *in* (plaintext, vault
bytes, password/orig_name strings) are copied internally; the module never takes ownership of
caller-provided memory.

### Functions

```c
int  nbias_wasm_has_vault_header(unsigned char const* bytes, size_t len);
// -> 1 if `bytes` starts with a nbias vault header, 0 otherwise. Never fails.

int  nbias_wasm_peek_header(
    unsigned char const* vault_bytes, size_t vault_len,
    uint8_t* out_version,
    uint8_t* out_auth_method,   // 0=password, 1=no_password, 2/3=reserved (keyfile, unimplemented)
    uint8_t* out_kdf_profile,   // 0=fast, 1=balanced, 2=hardened
    char**   out_orig_name);    // malloc'd, null-terminated

int  nbias_wasm_encrypt_note(
    unsigned char const* plaintext, size_t plaintext_len,
    char const* orig_name,           // null-terminated
    uint8_t auth_method,             // 0 or 1 (2/3 not supported)
    uint8_t kdf_profile,             // 0/1/2, ignored when auth_method == 1
    char const* passphrase,          // null-terminated, or NULL when auth_method == 1
    unsigned char** out_vault_bytes, // malloc'd
    size_t* out_vault_len);

int  nbias_wasm_decrypt_note(
    unsigned char const* vault_bytes, size_t vault_len,
    char const* passphrase,          // null-terminated, or NULL
    unsigned char** out_plaintext,   // malloc'd
    size_t* out_plaintext_len,
    char** out_orig_name,            // malloc'd, null-terminated
    uint8_t* out_auth_method);

void nbias_wasm_free_buffer(void* ptr);
char const* nbias_wasm_last_error_message(void);
```

`wasm/test/smoke_test.mjs` is a working, minimal example of calling these from JS (manual
`_malloc`/`HEAPU8`/`UTF8ToString` marshaling) and is the best starting reference for a
consumer project's own binding layer.

# Third-Party Notices

nbias does not vendor third-party source code in this repository.

The core library and CLI link against system or package-manager provided
libraries at build or runtime:

- libsodium for the encryption engine (Argon2id key derivation and the
  XChaCha20-Poly1305 AEAD cipher).

The `wasm/` Emscripten bindings build libsodium from source instead, fetched
via CMake `FetchContent` from the upstream `jedisct1/libsodium` repository and
compiled for wasm32 using libsodium's own `dist-build/emscripten.sh` build
script.

Container images, CI workflows, and the build toolchain use third-party
tools and actions:

- The Emscripten SDK (`emsdk`) for the `wasm/` build.
- Node.js, bundled with the Emscripten SDK, to run the `wasm/` smoke test.
- GitHub Actions maintained by GitHub (`actions/checkout`, `actions/cache`,
  `actions/setup-node`).

Those components remain under their respective licenses. Package managers
and the Emscripten SDK provide their own license metadata for the exact
versions used.

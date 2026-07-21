# nbias

A local, CLI-based encrypted note tool.
Intended for lightweight personal text you don't want to keep as plaintext on GitHub.
Not intended for credentials or other sensitive/confidential information.

## Overview

- Encrypted files use the `.knty` extension. Appended to the filename rather than replacing it (`recipe.md` → `recipe.md.knty`).
- The original filename is stored in the header, so a renamed `.knty` file is still restored to its original name on decryption.
- Password protection is off by default. Encrypting without `-K` uses a fixed key embedded in the `nbias` binary.

## Build

Requires a C++23-capable compiler, CMake 3.22+, and libsodium (`libsodium-dev`/`libsodium`, etc.).

```bash
cmake -S . -B build
cmake --build build
```

The `nbias` binary is produced at `build/nbias`.

### Test

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage

```
nbias enc <path...> [--output-dir <dir>] [-K] [--key|-k <value>] [--no-password] [--kdf-profile <fast|balanced|hardened>]
nbias dec <path...> [--output-dir <dir>] [-K] [--key|-k <value>]
nbias edit <path.knty> [--editor <cmd>] [-K] [--key|-k <value>] [--yes]
nbias info <path.knty>
```

### `enc`

```bash
nbias enc unity.md                       # -> unity.md.knty (no password)
nbias enc unity.md -K --key idolic       # -> unity.md.knty (password protected)
nbias enc me.md you.md --output-dir out  # multiple files, flat output directory
```

- If the target already ends in `.knty`, it's skipped (no double-encryption).
- If the output `.knty` already exists, you're asked to confirm before overwriting.
- `--kdf-profile` only affects password-protected vaults. The default is `fast`.

### `dec`

```bash
nbias dec note.md.knty
nbias dec note.md.knty --key hunter2 --output-dir restored
```

- The restored filename comes from the original filename stored in the header, not from stripping `.knty` off the vault's own filename.
- If the target already exists: identical content overwrites silently (effectively a no-op); differing content asks `[y/N]`, and declining saves under a numbered name instead (`unity.01.md`, `unity.02.md`, ...).
- `-K` means "I expect this vault to be password-protected." If the header actually shows no password, nbias just decrypts with the embedded key and notes that no password was needed.
- If the header shows password protection, nbias resolves a password in the order below and retries interactively if it's wrong.

### `edit`

```bash
nbias edit note.md.knty
nbias edit note.md.knty --key hunter2 --editor "code --wait"
```

- Takes the encrypted file's path directly. `edit` doesn't guess a `.knty` path from a plaintext filename, and it doesn't perform first-time encryption either (use `enc` for that).
- Decrypts to a temporary file and launches an editor; on exit it re-encrypts over the original vault with the same auth method/KDF profile but a freshly generated salt/nonce.
- If editing didn't change the content, nothing happens.
- Editor resolution order: `--editor` > `$VISUAL` > `$EDITOR` > `nvim` > `vim` > `vi` > `nano`

### `info`

```bash
nbias info note.md.knty
```

Prints the format version, auth method (password / no password), KDF profile (password-protected only), and original filename — all from the header, no password required.

## Password

When a password is needed (`-K` given, or the header shows password protection), nbias tries in this order:

1. `--key`/`-k` on the command line
2. `NBIAS_KEY` in a `.env` file in the current directory
3. `NBIAS_KEY` environment variable
4. Interactive prompt (echo disabled, up to 3 retries)

## `.env`

Create a `.env` file in the directory you run `nbias` from.

```bash
# .env
OUTPUT=encrypted
INPUT=.
NBIAS_KEY=love_kenty
```

- `OUTPUT`: base directory for `enc` output. Input subdirectories are reproduced as-is (`docs/notes/file.md` → `encrypted/docs/notes/file.md.knty`).
- `INPUT`: base directory `dec` restores into, inverting the `OUTPUT` mapping (`nbias dec encrypted/docs/notes/file.md.knty` → `./docs/notes/file.md`).
- `--output-dir` on the command line overrides both; no subdirectory reproduction.
- `NBIAS_KEY`: see the password resolution order above.

### `.gitignore`

```gitignore
*.md
!*.md.knty
.env
```

## License

nbias is licensed under the MIT License. See `LICENSE`.

## Third-Party Notices

Third-party dependency and tooling notices are listed in
`THIRD_PARTY_NOTICES.md`.

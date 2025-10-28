# nbias

## Status

**Note:** This project is currently in its planning and early design stage.  
Most implementations and supporting materials have not yet been completed.

---

`nbias` is a local, CLI-based encrypted note manager.  
It is intended for lightweight personal text storage where you prefer **not** to keep plain text on GitHub.

It is **not** meant for `.env`-level secrets, IDs, or passwords.  
Think of it as a secure place for something like your secret recipe, not your API keys.

You can open plain text files (`.txt`, `.md`, etc.) in any editor, and upon closing they are automatically encrypted into `.nbv` files.

---

## Overview

- Works completely offline – no network activity at any time.  
- Compatible with Vim, Neovim, VSCode, Sublime, and others without changing your settings.  
- Uses PBKDF2 + AES-256-GCM on macOS, and scrypt + AES-256-GCM on Linux.  
- Decryption creates a temporary plaintext in RAM, which is deleted upon exit.  
- The encrypted file stores the original extension inside its header for proper restoration.  
- Only text formats are allowed – binary or oversized files are ignored.

---

## Design Goals

- Minimise dependencies: only standard C++ and the system crypto library.  
- Ensure identical behaviour across macOS, Arch Linux, and Ubuntu.  
- Make local encryption of personal notes and recipes effortless.  
- Complete the edit-encrypt cycle with a single command.

---

## File Format

Encrypted files use the `.nbv` extension (nbias vault).  
Each file header contains version, cipher type, KDF parameters, and the original extension,  
allowing nbias to restore the correct file type on decryption.

---

## Typical Usage

```bash
nbias edit note.md      # edit plaintext, then decide whether to encrypt
nbias edit memo.nbv     # open encrypted note, edit, and auto re-encrypt
nbias encrypt -i note.txt -o note.nbv
nbias decrypt -i note.nbv -o note.txt


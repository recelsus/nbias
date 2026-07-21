// Minimal end-to-end check for the wasm/ bindings: loads the compiled module and exercises
// has_vault_header/peek_header/encrypt_note/decrypt_note via the raw extern "C" ABI, using
// only the low-level Emscripten runtime helpers (getValue/setValue/UTF8ToString/stringToUTF8),
// no Embind. Doubles as a reference for how a consumer project would call this API.
//
// Usage: node smoke_test.mjs <path-to-nbias_core.mjs>

import { pathToFileURL } from "node:url";

const modulePath = process.argv[2];
if (!modulePath) {
    console.error("usage: node smoke_test.mjs <path-to-nbias_core.mjs>");
    process.exit(1);
}

const { default: createNbiasCoreModule } = await import(pathToFileURL(modulePath).href);
const Module = await createNbiasCoreModule();

const STATUS_OK = 0;
const STATUS_FORMAT_ERROR = 1;
const STATUS_AUTH_ERROR = 2;

const AUTH_PASSWORD = 0;
const AUTH_NO_PASSWORD = 1;
const KDF_FAST = 0;

let failures = 0;

function check(name, condition) {
    if (condition) {
        console.log(`[PASS] ${name}`);
    } else {
        console.error(`[FAIL] ${name}`);
        failures += 1;
    }
}

function mallocCString(text) {
    if (text === null) {
        return 0;
    }
    const byteLength = Module.lengthBytesUTF8(text) + 1;
    const ptr = Module._malloc(byteLength);
    Module.stringToUTF8(text, ptr, byteLength);
    return ptr;
}

function mallocBytes(bytes) {
    const ptr = Module._malloc(bytes.length || 1);
    Module.HEAPU8.set(bytes, ptr);
    return ptr;
}

function readBytes(ptr, len) {
    return Module.HEAPU8.slice(ptr, ptr + len);
}

function bytesEqual(a, b) {
    if (a.length !== b.length) {
        return false;
    }
    for (let i = 0; i < a.length; i += 1) {
        if (a[i] !== b[i]) {
            return false;
        }
    }
    return true;
}

function lastErrorMessage() {
    return Module.UTF8ToString(Module._nbias_wasm_last_error_message());
}

// out_version/out_auth_method/out_kdf_profile are 1-byte out-params; out_orig_name is a
// pointer-sized (4-byte, wasm32) out-param holding a malloc'd C string.
function peekHeader(vaultBytes) {
    const vaultPtr = mallocBytes(vaultBytes);
    const versionPtr = Module._malloc(1);
    const authPtr = Module._malloc(1);
    const profilePtr = Module._malloc(1);
    const origNamePtrPtr = Module._malloc(4);

    const status = Module._nbias_wasm_peek_header(vaultPtr, vaultBytes.length, versionPtr, authPtr, profilePtr, origNamePtrPtr);

    let result = null;
    if (status === STATUS_OK) {
        const origNamePtr = Module.getValue(origNamePtrPtr, "i32");
        result = {
            version: Module.getValue(versionPtr, "i8") & 0xff,
            authMethod: Module.getValue(authPtr, "i8") & 0xff,
            kdfProfile: Module.getValue(profilePtr, "i8") & 0xff,
            origName: Module.UTF8ToString(origNamePtr),
        };
        Module._nbias_wasm_free_buffer(origNamePtr);
    }

    Module._free(vaultPtr);
    Module._free(versionPtr);
    Module._free(authPtr);
    Module._free(profilePtr);
    Module._free(origNamePtrPtr);

    return { status, result };
}

function encryptNote(plaintext, origName, authMethod, kdfProfile, passphrase) {
    const plaintextPtr = mallocBytes(plaintext);
    const origNamePtr = mallocCString(origName);
    const passphrasePtr = mallocCString(passphrase);
    const outVaultPtrPtr = Module._malloc(4);
    const outVaultLenPtr = Module._malloc(4);

    const status = Module._nbias_wasm_encrypt_note(
        plaintextPtr, plaintext.length,
        origNamePtr, authMethod, kdfProfile, passphrasePtr,
        outVaultPtrPtr, outVaultLenPtr);

    let vaultBytes = null;
    if (status === STATUS_OK) {
        const vaultPtr = Module.getValue(outVaultPtrPtr, "i32");
        const vaultLen = Module.getValue(outVaultLenPtr, "i32");
        vaultBytes = readBytes(vaultPtr, vaultLen);
        Module._nbias_wasm_free_buffer(vaultPtr);
    }

    Module._free(plaintextPtr);
    Module._free(origNamePtr);
    if (passphrasePtr) Module._free(passphrasePtr);
    Module._free(outVaultPtrPtr);
    Module._free(outVaultLenPtr);

    return { status, vaultBytes };
}

function decryptNote(vaultBytes, passphrase) {
    const vaultPtr = mallocBytes(vaultBytes);
    const passphrasePtr = mallocCString(passphrase);
    const outPlaintextPtrPtr = Module._malloc(4);
    const outPlaintextLenPtr = Module._malloc(4);
    const outOrigNamePtrPtr = Module._malloc(4);
    const outAuthMethodPtr = Module._malloc(1);

    const status = Module._nbias_wasm_decrypt_note(
        vaultPtr, vaultBytes.length, passphrasePtr,
        outPlaintextPtrPtr, outPlaintextLenPtr, outOrigNamePtrPtr, outAuthMethodPtr);

    let result = null;
    if (status === STATUS_OK) {
        const plaintextPtr = Module.getValue(outPlaintextPtrPtr, "i32");
        const plaintextLen = Module.getValue(outPlaintextLenPtr, "i32");
        const origNamePtr = Module.getValue(outOrigNamePtrPtr, "i32");
        result = {
            plaintext: readBytes(plaintextPtr, plaintextLen),
            origName: Module.UTF8ToString(origNamePtr),
            authMethod: Module.getValue(outAuthMethodPtr, "i8") & 0xff,
        };
        Module._nbias_wasm_free_buffer(plaintextPtr);
        Module._nbias_wasm_free_buffer(origNamePtr);
    }

    Module._free(vaultPtr);
    if (passphrasePtr) Module._free(passphrasePtr);
    Module._free(outPlaintextPtrPtr);
    Module._free(outPlaintextLenPtr);
    Module._free(outOrigNamePtrPtr);
    Module._free(outAuthMethodPtr);

    return { status, result };
}

const encoder = new TextEncoder();

// has_vault_header rejects plain text
{
    const notAVault = encoder.encode("just some plain text, not a vault");
    const ptr = mallocBytes(notAVault);
    const isVault = Module._nbias_wasm_has_vault_header(ptr, notAVault.length);
    Module._free(ptr);
    check("has_vault_header rejects non-vault bytes", isVault === 0);
}

// encrypt_note -> decrypt_note round trip, no password
{
    const plaintext = encoder.encode("hello from the wasm smoke test");
    const encrypted = encryptNote(plaintext, "note.md", AUTH_NO_PASSWORD, KDF_FAST, null);
    check("encrypt_note (no password) succeeds", encrypted.status === STATUS_OK);

    const isVault = Module._nbias_wasm_has_vault_header(mallocBytes(encrypted.vaultBytes), encrypted.vaultBytes.length);
    check("has_vault_header recognizes the encrypted output", isVault === 1);

    const peeked = peekHeader(encrypted.vaultBytes);
    check("peek_header reports auth_method = no_password", peeked.status === STATUS_OK && peeked.result.authMethod === AUTH_NO_PASSWORD);
    check("peek_header reports the original filename", peeked.result.origName === "note.md");

    const decrypted = decryptNote(encrypted.vaultBytes, null);
    check("decrypt_note (no password) succeeds", decrypted.status === STATUS_OK);
    check("decrypt_note recovers the original plaintext", decrypted.status === STATUS_OK && bytesEqual(decrypted.result.plaintext, plaintext));
}

// encrypt_note -> decrypt_note round trip, with password
{
    const plaintext = encoder.encode("a password protected note");
    const encrypted = encryptNote(plaintext, "diary.md", AUTH_PASSWORD, KDF_FAST, "hunter2");
    check("encrypt_note (password) succeeds", encrypted.status === STATUS_OK);

    const correct = decryptNote(encrypted.vaultBytes, "hunter2");
    check("decrypt_note succeeds with the correct password", correct.status === STATUS_OK && bytesEqual(correct.result.plaintext, plaintext));

    const wrong = decryptNote(encrypted.vaultBytes, "wrong-password");
    check("decrypt_note rejects the wrong password", wrong.status === STATUS_AUTH_ERROR);
    check("last_error_message is non-empty after a failure", lastErrorMessage().length > 0);
}

// peek_header rejects a corrupted header
{
    const plaintext = encoder.encode("tamper target");
    const encrypted = encryptNote(plaintext, "a.md", AUTH_NO_PASSWORD, KDF_FAST, null);
    const tampered = encrypted.vaultBytes.slice();
    tampered[0] = 0x00;
    const peeked = peekHeader(tampered);
    check("peek_header rejects a corrupted magic", peeked.status === STATUS_FORMAT_ERROR);
}

process.exit(failures === 0 ? 0 : 1);

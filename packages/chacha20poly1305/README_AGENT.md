# chacha20poly1305 — AI Agent Quick Ref

- **Package**: `chacha20poly1305` (`1.0.0`) | **Source**: crates.io `chacha20poly1305` (RFC 8439 + XChaCha20)
- **Import**: `import { aeadEncrypt, aeadDecrypt, xchacha20Poly1305Encrypt, xchacha20Poly1305Decrypt } from "chacha20poly1305";`
- **Allocation**: Zero-heap (caller supplies all output buffers).

## Constants

```typescript
CHACHA20_KEY_LEN: usize = 32;
CHACHA20_NONCE_LEN: usize = 12;
CHACHA20_BLOCK_LEN: usize = 64;
CHACHA20_TAG_LEN: usize = 16;
XCHACHA20_NONCE_LEN: usize = 24;
```

## Core Signatures

```typescript
// RFC 8439 ChaCha20-Poly1305 (key=32B, nonce=12B, tag=16B, ciphertext.len >= plaintext.len)
export fn aeadEncrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;
export fn aeadDecrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;

// XChaCha20-Poly1305 (key=32B, nonce24=24B, tag=16B)
export fn xchacha20Poly1305Encrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;
export fn xchacha20Poly1305Decrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;

// Primitives
export fn xchacha20HChaCha20(key: &[]u8, nonce16: &[]u8, out_subkey: []u8): bool;
export fn chacha20Block(key: &[]u8, counter: u32, nonce: &[]u8, out: []u8): bool;
export fn chacha20Xor(key: &[]u8, counter: u32, nonce: &[]u8, input: &[]u8, out: []u8): bool;
export fn poly1305KeyGen(key: &[]u8, nonce: &[]u8, out: []u8): bool;
```

## Minimal Pattern

```typescript
import { aeadEncrypt, aeadDecrypt } from "chacha20poly1305";

let ct: [16]u8 = undefined;
let tag: [16]u8 = undefined;
const ok = aeadEncrypt(key[0..], nonce12[0..], aad, pt, ct[0..], tag[0..]);
let out_pt: [16]u8 = undefined;
const valid = aeadDecrypt(key[0..], nonce12[0..], aad, ct[0..], tag[0..], out_pt[0..]);
```

## Pitfalls

1. **Buffer lengths must be exact/sufficient**: `key.len == 32`, `nonce.len == 12` (or `24` for XChaCha20), `tag.len == 16`, and `ciphertext.len >= plaintext.len`; otherwise functions return `false`.
2. **Never reuse `(key, nonce)`**: Reusing a nonce with the same key breaks confidentiality and Poly1305 authenticity. Prefer `xchacha20Poly1305*` when using random nonces.
3. **Always check the `bool` return value**: On `aeadDecrypt` returning `false`, discard `plaintext` immediately.
4. **Slice types**: Read-only inputs are `&[]u8`, mutable output buffers are `[]u8`.

# chacha20poly1305@1.0.0 (Agent Quickref)

> Pure CP zero-allocation RFC 8439 ChaCha20-Poly1305 and XChaCha20-Poly1305 AEAD cipher (reuses poly1305, rand_chacha, and subtle) | License: MIT
> Import: `import { ... } from "chacha20poly1305";` | Full types: `src/index.d.cp`
> Deps: `poly1305`, `rand_chacha`, `subtle`

## Functions
- `export fn chacha20Block(key: &[]u8, counter: u32, nonce: &[]u8, out: []u8): bool;`
- `export fn chacha20Xor(key: &[]u8, counter: u32, nonce: &[]u8, input: &[]u8, out: []u8): bool;`
- `export fn poly1305KeyGen(key: &[]u8, nonce: &[]u8, out: []u8): bool;`
- `export fn aeadEncrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;`
- `export fn aeadDecrypt(key: &[]u8, nonce: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;`
- `export fn xchacha20HChaCha20(key: &[]u8, nonce16: &[]u8, out_subkey: []u8): bool;`
- `export fn xchacha20Poly1305Encrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, plaintext: &[]u8, ciphertext: []u8, tag: []u8): bool;`
- `export fn xchacha20Poly1305Decrypt(key: &[]u8, nonce24: &[]u8, aad: &[]u8, ciphertext: &[]u8, tag: &[]u8, plaintext: []u8): bool;`

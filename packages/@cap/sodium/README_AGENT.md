# @cap/sodium@1.0.20 (Agent Quickref)

> libsodium 1.0.20 modern cryptography suite (Ed25519, XChaCha20-Poly1305 AEAD, BLAKE2b, CSPRNG) for CP | License: ISC
> Import: `import { ... } from "@cap/sodium";` | Full types: `src/index.d.cp`

## Functions
- `export fn sodium_init(): i32;`
- `export fn sodium_version_string(): [*:0]&u8!?;`
- `export fn randombytes_buf(buf: [*]u8!, size: usize);`
- `export fn crypto_sign_ed25519_keypair(pk: [*]u8!, sk: [*]u8!): i32;`
- `export fn crypto_sign_ed25519_detached(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32;`
- `export fn crypto_sign_ed25519_verify_detached(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32;`
- `export fn crypto_aead_xchacha20poly1305_ietf_encrypt(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32;`
- `export fn crypto_aead_xchacha20poly1305_ietf_decrypt(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32;`
- `export fn crypto_generichash(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32;`

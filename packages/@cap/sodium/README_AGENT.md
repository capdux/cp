# @cap/sodium — AI Agent Quick Ref

libsodium 1.0.20 modern cryptography suite (Ed25519, XChaCha20-Poly1305 AEAD, BLAKE2b, CSPRNG) for CP (`v1.0.20`, `ISC`)

## Import

```cp
import {
    SodiumError,
    Ed25519Keypair,
    Ed25519Signature,
    Sodium,
    sodium_init,
    sodium_version_string,
    randombytes_buf,
    crypto_sign_ed25519_keypair,
} from "@cap/sodium";
```

## Core Types & Constants

```cp
error SodiumError { InitFailed, InvalidKeyLength, InvalidNonceLength, InvalidSignatureLength, InvalidHashLength, BufferTooSmall }
class Ed25519Keypair { pk: [32]u8, sk: [64]u8 }
class Ed25519Signature { bytes: [64]u8 }
class Sodium {  } // methods: init(...) +8
```

## API Signatures

```cp
fn sodium_init(): i32;
fn sodium_version_string(): [*:0]&u8!?;
fn randombytes_buf(buf: [*]u8!, size: usize);
fn crypto_sign_ed25519_keypair(pk: [*]u8!, sk: [*]u8!): i32;
fn crypto_sign_ed25519_detached(sig: [*]u8!, siglen_p: *u64!?, m: [*]&u8!?, mlen: u64, sk: [*]&u8!): i32;
fn crypto_sign_ed25519_verify_detached(sig: [*]&u8!, m: [*]&u8!?, mlen: u64, pk: [*]&u8!): i32;
fn crypto_aead_xchacha20poly1305_ietf_encrypt(c: [*]u8!, clen_p: *u64!?, m: [*]&u8!?, mlen: u64, ad: [*]&u8!?, adlen: u64, nsec: [*]&u8!?, npub: [*]&u8!, k: [*]&u8!): i32;
fn crypto_aead_xchacha20poly1305_ietf_decrypt(m: [*]u8!?, mlen_p: *u64!?, nsec: [*]u8!?, c: [*]&u8!, clen: u64, ad: [*]&u8!?, adlen: u64, npub: [*]&u8!, k: [*]&u8!): i32;
fn crypto_generichash(out: [*]u8!, outlen: usize, in_buf: [*]&u8!?, inlen: u64, key: [*]&u8!?, keylen: usize): i32;
```

## Minimal Usage

```cp
import { sodium_init } from "@cap/sodium";

pub fn main() {
    let result = sodium_init();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/sodium"`; install via `cap add @cap/sodium`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

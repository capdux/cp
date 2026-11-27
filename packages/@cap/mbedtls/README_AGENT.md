# @cap/mbedtls — AI Agent Quick Ref

ARM Mbed TLS 3.6.2 core cryptography suite (SHA-256/512, AES-GCM, ChaCha20-Poly1305, Base64) for CP (`v3.6.2`, `Apache-2.0`)

## Import

```cp
import {
    MbedTlsError,
    cp_mbedtls_version,
    cp_mbedtls_sha256,
    cp_mbedtls_aes_gcm_roundtrip_test,
    versionInto,
    sha256,
    aesGcmRoundtripTest,
} from "@cap/mbedtls";
```

## Core Types & Constants

```cp
error MbedTlsError { BufferTooSmall, HashFailed, AesGcmFailed }
```

## API Signatures

```cp
fn cp_mbedtls_version(buf: [*]u8!, cap: usize): usize;
fn cp_mbedtls_sha256(data: [*]&u8!?, len: usize, out32: [*]u8!): i32;
fn cp_mbedtls_aes_gcm_roundtrip_test(): i32;
fn versionInto(buf: []u8): !string;
fn sha256(data: &[]u8): ![32]u8;
fn aesGcmRoundtripTest(): bool;
```

## Minimal Usage

```cp
import { versionInto } from "@cap/mbedtls";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = versionInto(out_buf[0..256]) catch return;
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/mbedtls"`; install via `cap add @cap/mbedtls`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

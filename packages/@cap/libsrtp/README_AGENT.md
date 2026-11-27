# @cap/libsrtp — AI Agent Quick Ref

Cisco libsrtp2 2.6.0 SRTP/SRTCP encrypted media transport native C binding for CP (`v2.6.0`, `BSD-3-Clause`)

## Import

```cp
import {
    SrtpError,
    SrtpSession,
    cp_srtp_init,
    cp_srtp_shutdown,
    cp_srtp_version,
    cp_srtp_create_aes128_sha1,
    cp_srtp_protect,
    cp_srtp_unprotect,
} from "@cap/libsrtp";
```

## Core Types & Constants

```cp
error SrtpError { InitFailed, BadParam, AllocFail, InvalidState, BufferTooSmall, AuthFail }
class SrtpSession { raw: *auto!?, inbound: bool } // methods: createAes128Sha1(...) +3
```

## API Signatures

```cp
fn cp_srtp_init(): i32;
fn cp_srtp_shutdown(): i32;
fn cp_srtp_version(): [*:0]&u8!?;
fn cp_srtp_create_aes128_sha1(key30: [*]&u8!, inbound: i32): *auto!?;
fn cp_srtp_protect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;
fn cp_srtp_unprotect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;
fn cp_srtp_destroy(session: *auto!?);
fn mapSrtpStatus(code: i32, is_unprotect: bool): SrtpError;
fn srtpInit(): !void;
fn srtpShutdown(): !void;
fn versionString(): string;
```

## Minimal Usage

```cp
import { versionString } from "@cap/libsrtp";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/libsrtp"`; install via `cap add @cap/libsrtp`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

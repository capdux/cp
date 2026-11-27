[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/libsrtp

Cisco libsrtp2 2.6.0 SRTP/SRTCP encrypted media transport native C binding for CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/libsrtp` |
| Version | `2.6.0` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/libsrtp
```

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

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SrtpError` | `InitFailed, BadParam, AllocFail, InvalidState, BufferTooSmall, AuthFail, ReplayFail, ProtectFailed ...` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SrtpSession` | `class` | Fields: `raw: *auto!?, inbound: bool` · Methods: `createAes128Sha1, protect, unprotect, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_srtp_init` | `(): i32` | Invokes `cp_srtp_init` and returns `i32`. |
| `cp_srtp_shutdown` | `(): i32` | Invokes `cp_srtp_shutdown` and returns `i32`. |
| `cp_srtp_version` | `(): [*:0]&u8!?` | Invokes `cp_srtp_version` and returns `[*:0]&u8!?`. |
| `cp_srtp_create_aes128_sha1` | `(key30: [*]&u8!, inbound: i32): *auto!?` | Invokes `cp_srtp_create_aes128_sha1` and returns `*auto!?`. |
| `cp_srtp_protect` | `(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32` | Invokes `cp_srtp_protect` and returns `i32`. |
| `cp_srtp_unprotect` | `(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32` | Invokes `cp_srtp_unprotect` and returns `i32`. |
| `cp_srtp_destroy` | `(session: *auto!?)` | Executes `cp_srtp_destroy` in-place. |
| `mapSrtpStatus` | `(code: i32, is_unprotect: bool): SrtpError` | Invokes `mapSrtpStatus` and returns `SrtpError`. |
| `srtpInit` | `(): !void` | Invokes `srtpInit` and returns `!void`. |
| `srtpShutdown` | `(): !void` | Invokes `srtpShutdown` and returns `!void`. |
| `versionString` | `(): string` | Invokes `versionString` and returns `string`. |

## Minimal Example

```cp
import { versionString } from "@cap/libsrtp";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

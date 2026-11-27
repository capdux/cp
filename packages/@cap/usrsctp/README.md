[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# @cap/usrsctp

Portable userland SCTP stack (usrsctp 0.9.5.0) for WebRTC DataChannel in CP

| Metadata | Value |
| --- | --- |
| Package | `@cap/usrsctp` |
| Version | `0.9.5` |
| License | `BSD-3-Clause` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add @cap/usrsctp
```

```cp
import {
    SctpError,
    SctpSocket,
    cp_usrsctp_init,
    cp_usrsctp_finish,
    cp_usrsctp_socket_create,
    cp_usrsctp_socket_close,
    cp_usrsctp_socket_create_and_close,
    sctpInit,
} from "@cap/usrsctp";
```

## Core API

### Types, Enums & Errors

| Kind | Name | Definition |
| --- | --- | --- |
| `error` | `SctpError` | `InitFailed, FinishFailed, SocketCreateFailed` |

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `SctpSocket` | `class` | Fields: `raw: *auto!?` · Methods: `createStream, isOpen, close, __cp_deinit` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `cp_usrsctp_init` | `(udp_port: u16): i32` | Invokes `cp_usrsctp_init` and returns `i32`. |
| `cp_usrsctp_finish` | `(): i32` | Invokes `cp_usrsctp_finish` and returns `i32`. |
| `cp_usrsctp_socket_create` | `(domain: i32, sock_type: i32): *auto!?` | Invokes `cp_usrsctp_socket_create` and returns `*auto!?`. |
| `cp_usrsctp_socket_close` | `(sock: *auto!?)` | Executes `cp_usrsctp_socket_close` in-place. |
| `cp_usrsctp_socket_create_and_close` | `(): i32` | Invokes `cp_usrsctp_socket_create_and_close` and returns `i32`. |
| `sctpInit` | `(udp_port: u16): !void` | Invokes `sctpInit` and returns `!void`. |
| `sctpFinish` | `(): !void` | Invokes `sctpFinish` and returns `!void`. |
| `socketCreateAndClose` | `(): !void` | Invokes `socketCreateAndClose` and returns `!void`. |

## Minimal Example

```cp
import { cp_usrsctp_init } from "@cap/usrsctp";

pub fn main() {
    let result = cp_usrsctp_init(0);
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers.
- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow.
- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

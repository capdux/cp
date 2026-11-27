# @cap/usrsctp — AI Agent Quick Ref

Portable userland SCTP stack (usrsctp 0.9.5.0) for WebRTC DataChannel in CP (`v0.9.5`, `BSD-3-Clause`)

## Import

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

## Core Types & Constants

```cp
error SctpError { InitFailed, FinishFailed, SocketCreateFailed }
class SctpSocket { raw: *auto!? } // methods: createStream(...) +3
```

## API Signatures

```cp
fn cp_usrsctp_init(udp_port: u16): i32;
fn cp_usrsctp_finish(): i32;
fn cp_usrsctp_socket_create(domain: i32, sock_type: i32): *auto!?;
fn cp_usrsctp_socket_close(sock: *auto!?);
fn cp_usrsctp_socket_create_and_close(): i32;
fn sctpInit(udp_port: u16): !void;
fn sctpFinish(): !void;
fn socketCreateAndClose(): !void;
```

## Minimal Usage

```cp
import { cp_usrsctp_init } from "@cap/usrsctp";

pub fn main() {
    let result = cp_usrsctp_init(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/usrsctp"`; install via `cap add @cap/usrsctp`.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.

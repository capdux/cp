# @cap/usrsctp@0.9.5 (Agent Quickref)

> Portable userland SCTP stack (usrsctp 0.9.5.0) for WebRTC DataChannel in CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/usrsctp";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_usrsctp_init(udp_port: u16): i32;`
- `export fn cp_usrsctp_finish(): i32;`
- `export fn cp_usrsctp_socket_create(domain: i32, sock_type: i32): *auto!?;`
- `export fn cp_usrsctp_socket_close(sock: *auto!?);`
- `export fn cp_usrsctp_socket_create_and_close(): i32;`
- `export fn sctpInit(udp_port: u16): !void;`
- `export fn sctpFinish(): !void;`
- `export fn socketCreateAndClose(): !void;`

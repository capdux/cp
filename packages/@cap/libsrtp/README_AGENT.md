# @cap/libsrtp@2.6.0 (Agent Quickref)

> Cisco libsrtp2 2.6.0 SRTP/SRTCP encrypted media transport native C binding for CP | License: BSD-3-Clause
> Import: `import { ... } from "@cap/libsrtp";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_srtp_init(): i32;`
- `export fn cp_srtp_shutdown(): i32;`
- `export fn cp_srtp_version(): [*:0]&u8!?;`
- `export fn cp_srtp_create_aes128_sha1(key30: [*]&u8!, inbound: i32): *auto!?;`
- `export fn cp_srtp_protect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;`
- `export fn cp_srtp_unprotect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;`
- `export fn cp_srtp_destroy(session: *auto!?);`
- `export fn mapSrtpStatus(code: i32, is_unprotect: bool): SrtpError;`
- `export fn srtpInit(): !void;`
- `export fn srtpShutdown(): !void;`
- `export fn versionString(): string;`

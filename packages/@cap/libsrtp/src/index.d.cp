// Type definitions for CP

import {  } from "std";
export fn cp_srtp_init(): i32;
export fn cp_srtp_shutdown(): i32;
export fn cp_srtp_version(): [*:0]&u8!?;
export fn cp_srtp_create_aes128_sha1(key30: [*]&u8!, inbound: i32): *auto!?;
export fn cp_srtp_protect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;
export fn cp_srtp_unprotect(session: *auto!?, buf: [*]u8!, len_inout: *i32!): i32;
export fn cp_srtp_destroy(session: *auto!?);
export error SrtpError { InitFailed, BadParam, AllocFail, InvalidState, BufferTooSmall, AuthFail, ReplayFail, ProtectFailed, UnprotectFailed }

export fn mapSrtpStatus(code: i32, is_unprotect: bool): SrtpError;
export fn srtpInit(): !void;
export fn srtpShutdown(): !void;
export fn versionString(): string;
export class SrtpSession {
    raw: *auto!?,
    inbound: bool,
    fn createAes128Sha1(key30: &[]u8, inbound: bool): !SrtpSession;
    fn protect(buf: []u8, plaintext_len: usize): !usize;
    fn unprotect(buf: []u8, protected_len: usize): !usize;
    fn __cp_deinit(_: &mut SrtpSession);
}


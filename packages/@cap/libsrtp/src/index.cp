// @cap/libsrtp — Cisco libsrtp2 2.6.0 SRTP/SRTCP native C binding for CP
// License: BSD-3-Clause (Upstream: Cisco Systems / libsrtp)

import std from "std";

pub @c {
    fn cp_srtp_init(): i32;
    fn cp_srtp_shutdown(): i32;
    fn cp_srtp_version(): [*:0]const u8?;
    fn cp_srtp_create_aes128_sha1(key30: [*]const u8!, inbound: i32): *!?;
    fn cp_srtp_protect(session: *!?, buf: [*]u8!, len_inout: *i32!): i32;
    fn cp_srtp_unprotect(session: *!?, buf: [*]u8!, len_inout: *i32!): i32;
    fn cp_srtp_destroy(session: *!?);
}

pub const SRTP_MASTER_KEY_LEN: usize = 30;
pub const SRTP_MAX_TRAILER_LEN: usize = 16;

pub err SrtpError {
    InitFailed,
    BadParam,
    AllocFail,
    InvalidState,
    BufferTooSmall,
    AuthFail,
    ReplayFail,
    ProtectFailed,
    UnprotectFailed,
}

fn mapSrtpStatus(code: i32, is_unprotect: bool): SrtpError {
    if (code == 2) return SrtpError.BadParam;
    if (code == 3) return SrtpError.AllocFail;
    if (code == 7) return SrtpError.AuthFail;
    if (code == 9 or code == 10) return SrtpError.ReplayFail;
    if (is_unprotect) return SrtpError.UnprotectFailed;
    return SrtpError.ProtectFailed;
}

/// Initializes the global libsrtp2 library state. Must be called before creating sessions.
pub fn srtpInit(): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_srtp_init();
    }
    if (rc != 0) return SrtpError.InitFailed;
}

/// Shuts down the global libsrtp2 library state.
pub fn srtpShutdown(): void! {
    let rc: i32 = 0;
    unsafe {
        rc = cp_srtp_shutdown();
    }
    if (rc != 0) return SrtpError.InvalidState;
}

/// Returns the linked libsrtp2 version string (e.g. "libsrtp2 2.6.0").
pub fn versionString(): string {
    unsafe {
        if (ver := cp_srtp_version()) {
            return std.mem.span(ver);
        }
    }
    return "libsrtp2 2.6.0";
}

/// Owned RAII SRTP session (AES-128-CM + HMAC-SHA1-80). Automatically destroyed on scope exit.
pub struct SrtpSession {
    raw: *!? = null,
    pub inbound: bool = false,

    /// Creates an SRTP session using a 30-byte master key (16-byte AES-128 key + 14-byte salt).
    pub static fn createAes128Sha1(key30: []const u8, inbound: bool): SrtpSession! {
        if (key30.len < SRTP_MASTER_KEY_LEN) return SrtpError.BadParam;
        const dir: i32 = if (inbound) 1 else 0;
        let sess: *!? = null;
        unsafe {
            sess = cp_srtp_create_aes128_sha1(key30.ptr, dir);
        }
        if (sess == null) return SrtpError.AllocFail;
        return SrtpSession{
            .raw = sess,
            .inbound = inbound,
        };
    }

    /// Encrypts and authenticates an RTP packet in-place.
    /// `buf` must have at least `plaintext_len + SRTP_MAX_TRAILER_LEN` bytes of capacity.
    /// Returns the resulting SRTP packet length in bytes.
    pub fn protect(buf: []u8, plaintext_len: usize): usize! {
        if (_.raw == null) return SrtpError.InvalidState;
        if (plaintext_len == 0 or plaintext_len > 0x7FFFFF00) return SrtpError.BadParam;
        if (buf.len < plaintext_len + SRTP_MAX_TRAILER_LEN) return SrtpError.BufferTooSmall;
        let pkt_len: i32 = @intCast(plaintext_len);
        let rc: i32 = 0;
        unsafe {
            rc = cp_srtp_protect(_.raw, buf.ptr, &pkt_len);
        }
        if (rc != 0) return mapSrtpStatus(rc, false);
        if (pkt_len < 0) return SrtpError.ProtectFailed;
        return @intCast(pkt_len);
    }

    /// Verifies and decrypts an SRTP packet in-place.
    /// Returns the resulting plaintext RTP packet length in bytes.
    pub fn unprotect(buf: []u8, protected_len: usize): usize! {
        if (_.raw == null) return SrtpError.InvalidState;
        if (protected_len == 0 or protected_len > buf.len or protected_len > 0x7FFFFF00) {
            return SrtpError.BadParam;
        }
        let pkt_len: i32 = @intCast(protected_len);
        let rc: i32 = 0;
        unsafe {
            rc = cp_srtp_unprotect(_.raw, buf.ptr, &pkt_len);
        }
        if (rc != 0) return mapSrtpStatus(rc, true);
        if (pkt_len < 0) return SrtpError.UnprotectFailed;
        return @intCast(pkt_len);
    }

    ~init() {
        if (p := _.raw) {
            unsafe { cp_srtp_destroy(p); }
            _.raw = null;
        }
    }
}

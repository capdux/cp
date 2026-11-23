// Reference-hardened against constantoine/totp-rs (MIT) & RFC 7636
// Source: cp/lib/oauth/pkce.cp (RFC 7636 PKCE S256 & State Token Generator)
// Pure CP RFC 7636 Proof Key for Code Exchange (PKCE S256) and high-entropy CSRF state generator.

const std = @import("std");

type PkceVec16u8 = @Vector(16, u8);

/// Unified error set for `cap:oauth` PKCE, URL/form codecs, token parsing, and Auth headers.
pub err OAuthError {
    InvalidVerifier,
    InvalidConfig,
    InvalidTokenResponse,
    InvalidAuthHeader,
    UnsupportedChallengeMethod,
    OutOfMemory,
}

/// RFC 7636 PKCE code challenge transformation method.
pub enum PkceChallengeMethod {
    S256,
    Plain,
}

fn fillCsprng(buf: []u8) {
    let off: usize = 0;
    while (off < buf.len) {
        const rem = buf[off..];
        const rc = std.os.linux.getrandom(rem.ptr, rem.len, 0);
        const e = std.os.linux.errno(rc);
        if (e == .INTR) continue;
        if (e == .SUCCESS) {
            if (rc == 0) continue;
            off += rc;
            continue;
        }
        @panic("getrandom failed");
    }
}

fn isUnreservedPkceChar(ch: u8): bool {
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= '0' && ch <= '9') return true;
    return ch == '-' || ch == '.' || ch == '_' || ch == '~';
}

/// Scalar reference RFC 7636 §4.1 `code_verifier` validator (length 43..=128, unreserved `[A-Za-z0-9\-._~]`).
pub fn scalarIsValidPkceVerifier(verifier: string): bool {
    if (verifier.len < 43 || verifier.len > 128) {
        return false;
    }
    let i: usize = 0;
    while (i < verifier.len) : (i += 1) {
        if (!isUnreservedPkceChar(verifier[i])) {
            return false;
        }
    }
    return true;
}

/// 16-lane SIMD accelerated RFC 7636 §4.1 `code_verifier` validator (length 43..=128, unreserved `[A-Za-z0-9\-._~]`).
pub fn simdIsValidPkceVerifier(verifier: string): bool {
    if (verifier.len < 43 || verifier.len > 128) {
        return false;
    }
    const lo_A: PkceVec16u8 = @splat('A');
    const hi_Z: PkceVec16u8 = @splat('Z');
    const lo_a: PkceVec16u8 = @splat('a');
    const hi_z: PkceVec16u8 = @splat('z');
    const lo_0: PkceVec16u8 = @splat('0');
    const hi_9: PkceVec16u8 = @splat('9');
    const v_dash: PkceVec16u8 = @splat('-');
    const v_dot: PkceVec16u8 = @splat('.');
    const v_under: PkceVec16u8 = @splat('_');
    const v_tilde: PkceVec16u8 = @splat('~');

    let i: usize = 0;
    while (i + 16 <= verifier.len) {
        const v: PkceVec16u8 = verifier[i .. i + 16][0..16].*;
        const ok = ((v >= lo_A) & (v <= hi_Z))
            | ((v >= lo_a) & (v <= hi_z))
            | ((v >= lo_0) & (v <= hi_9))
            | (v == v_dash)
            | (v == v_dot)
            | (v == v_under)
            | (v == v_tilde);
        const mask: u16 = @bitCast(ok);
        if (mask != 0xFFFF) return false;
        i += 16;
    }
    while (i < verifier.len) : (i += 1) {
        if (!isUnreservedPkceChar(verifier[i])) {
            return false;
        }
    }
    return true;
}

fn simdPkceCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let acc: PkceVec16u8 = @splat(0);
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: PkceVec16u8 = a[i .. i + 16][0..16].*;
        const vb: PkceVec16u8 = b[i .. i + 16][0..16].*;
        acc = acc | (va ^ vb);
        i += 16;
    }
    let diff: u8 = @reduce(.Or, acc);
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

/// Owned PKCE S256 verifier/challenge pair. Call `deinit()` to release owned buffers.
pub struct PkceChallenge {
    alloc: std.mem.Allocator = undefined,
    verifier: string = "",
    challenge: string = "",
    method: string = "S256",

    /// Releases `verifier` and `challenge` buffers allocated by `Pkce.generate` or `Pkce.fromVerifier`.
    pub fn deinit(*_) {
        if (_.verifier.len > 0) {
            _.alloc.free(_.verifier);
            _.verifier = "";
        }
        if (_.challenge.len > 0) {
            _.alloc.free(_.challenge);
            _.challenge = "";
        }
    }
}

/// RFC 7636 PKCE S256 generator, validator, constant-time verifier, and CSRF state token generator.
pub static struct Pkce {
    /// Validates RFC 7636 §4.1 `code_verifier` length (43..=128) and allowed unreserved charset `[A-Za-z0-9\-._~]` using 16-lane SIMD.
    pub static fn isValidVerifier(verifier: string): bool {
        return simdIsValidPkceVerifier(verifier);
    }

    /// Computes `BASE64URL-NOPAD(SHA256(verifier))` after validating `verifier` per RFC 7636 §4.1.
    pub static fn computeS256Challenge(alloc: std.mem.Allocator, verifier: string): ([]u8) ! OAuthError {
        if (!simdIsValidPkceVerifier(verifier)) {
            return OAuthError.InvalidVerifier;
        }
        let digest: [32]u8 = undefined;
        defer std.crypto.secureZero(u8, &digest);
        std.crypto.hash.sha2.Sha256.hash(verifier, &digest, .{});

        const encoder = std.base64.url_safe_no_pad.Encoder;
        const enc_len = encoder.calcSize(32);
        let out = alloc.alloc(u8, enc_len) catch return OAuthError.OutOfMemory;
        let * = encoder.encode(out, &digest);
        return out;
    }

    /// Generates a 32-byte CSPRNG `code_verifier` (43 Base64URL chars) and its S256 `code_challenge`.
    pub static fn generate(alloc: std.mem.Allocator): PkceChallenge ! OAuthError {
        let entropy: [32]u8 = undefined;
        defer std.crypto.secureZero(u8, &entropy);
        fillCsprng(&entropy);

        const encoder = std.base64.url_safe_no_pad.Encoder;
        const ver_len = encoder.calcSize(32);
        let verifier = alloc.alloc(u8, ver_len) catch return OAuthError.OutOfMemory;
        errdefer alloc.free(verifier);
        * = encoder.encode(verifier, &entropy);

        const challenge = Pkce.computeS256Challenge(alloc, verifier)?;
        return PkceChallenge{
            alloc: alloc,
            verifier: verifier,
            challenge: challenge,
            method: "S256",
        };
    }

    /// Builds a `PkceChallenge` from an existing caller-supplied `verifier` after RFC 7636 validation.
    pub static fn fromVerifier(alloc: std.mem.Allocator, verifier: string): PkceChallenge ! OAuthError {
        if (!simdIsValidPkceVerifier(verifier)) {
            return OAuthError.InvalidVerifier;
        }
        const owned_verifier = alloc.dupe(u8, verifier) catch return OAuthError.OutOfMemory;
        errdefer alloc.free(owned_verifier);

        const challenge = Pkce.computeS256Challenge(alloc, owned_verifier)?;
        return PkceChallenge{
            alloc: alloc,
            verifier: owned_verifier,
            challenge: challenge,
            method: "S256",
        };
    }

    /// Constant-time 16-lane SIMD RFC 7636 §4.6 verification of `BASE64URL-NOPAD(SHA256(verifier)) == challenge`.
    pub static fn verifyS256(verifier: string, challenge: string): bool {
        if (!simdIsValidPkceVerifier(verifier)) {
            return false;
        }
        if (challenge.len != 43) {
            return false;
        }
        let digest: [32]u8 = undefined;
        defer std.crypto.secureZero(u8, &digest);
        std.crypto.hash.sha2.Sha256.hash(verifier, &digest, .{});

        let expected: [43]u8 = undefined;
        defer std.crypto.secureZero(u8, &expected);
        * = std.base64.url_safe_no_pad.Encoder.encode(&expected, &digest);

        return simdPkceCtEqual(&expected, challenge);
    }

    /// Verifies a PKCE `code_verifier` against `challenge` with explicit method policy.
    /// Rejects insecure `plain` downgrade unless `allow_plain` is explicitly `true`.
    pub static fn verifyWithMethod(verifier: string, challenge: string, method: string, allow_plain: bool): bool {
        if (std.mem.eql(u8, method, "S256")) {
            return Pkce.verifyS256(verifier, challenge);
        }
        if (std.mem.eql(u8, method, "plain")) {
            if (!allow_plain) return false;
            if (!simdIsValidPkceVerifier(verifier)) return false;
            return simdPkceCtEqual(verifier, challenge);
        }
        return false;
    }

    /// Default-safe RFC 7636 PKCE verification: accepts `S256` only and rejects `plain` downgrade.
    pub static fn verifyChallenge(verifier: string, challenge: string, method: string): bool {
        return Pkce.verifyWithMethod(verifier, challenge, method, false);
    }

    /// Generates a high-entropy (256-bit) Base64URL-encoded anti-CSRF `state` parameter.
    pub static fn generateState(alloc: std.mem.Allocator): ([]u8) ! OAuthError {
        let entropy: [32]u8 = undefined;
        defer std.crypto.secureZero(u8, &entropy);
        fillCsprng(&entropy);

        const encoder = std.base64.url_safe_no_pad.Encoder;
        const enc_len = encoder.calcSize(32);
        let out = alloc.alloc(u8, enc_len) catch return OAuthError.OutOfMemory;
        let * = encoder.encode(out, &entropy);
        return out;
    }
}

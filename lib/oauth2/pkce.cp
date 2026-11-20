// Source: cp/lib/oauth2/pkce.cp (RFC 7636 PKCE Code Verifier, S256 Challenge & Constant-Time Verifier)

const std = @import("std");
import {
    OAuth2Error,
    PkceMethod,
    PkcePair,
    constantTimeEql,
    isUnreservedChar,
} from "./types.cp";

const PKCE_CHARSET: []const u8 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";

fn fillRandomBytes(buf: []u8) {
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

/// Validates RFC 7636 Section 4.1 `code_verifier`: length in [43, 128] and unreserved chars `[A-Za-z0-9-._~]`.
pub fn isValidCodeVerifier(verifier: []const u8): bool {
    if (verifier.len < 43 || verifier.len > 128) {
        return false;
    }
    let i: usize = 0;
    while (i < verifier.len) {
        if (!isUnreservedChar(verifier[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Generates a cryptographically secure PKCE `code_verifier` of length `len_43_to_128` (43..128).
pub fn generateCodeVerifier(alloc: std.mem.Allocator, len_43_to_128: usize): ([]u8) ! OAuth2Error {
    if (len_43_to_128 < 43 || len_43_to_128 > 128) {
        return OAuth2Error.InvalidVerifier;
    }

    let out = alloc.alloc(u8, len_43_to_128) catch return OAuth2Error.OutOfMemory;
    errdefer alloc.free(out);

    // Rejection sampling over 66-character unreserved alphabet to eliminate modulo bias (66 * 3 = 198).
    let rand_buf: [64]u8 = [0; 64];
    let filled: usize = 0;
    while (filled < len_43_to_128) {
        fillRandomBytes(&rand_buf);
        let j: usize = 0;
        while (j < rand_buf.len && filled < len_43_to_128) {
            const b = rand_buf[j];
            if (b < 198) {
                out[filled] = PKCE_CHARSET[b % 66];
                filled += 1;
            }
            j += 1;
        }
    }
    return out;
}

/// Derives `BASE64URL-NOPAD(SHA256(verifier))` per RFC 7636 Section 4.2.
pub fn deriveCodeChallengeS256(alloc: std.mem.Allocator, verifier: []const u8): ([]u8) ! OAuth2Error {
    if (!isValidCodeVerifier(verifier)) {
        return OAuth2Error.InvalidVerifier;
    }

    let digest: [32]u8 = [0; 32];
    std.crypto.hash.sha2.Sha256.hash(verifier, &digest, .{});

    const encoder = std.base64.url_safe_no_pad.Encoder;
    const enc_len = encoder.calcSize(32);
    let out = alloc.alloc(u8, enc_len) catch return OAuth2Error.OutOfMemory;
    let * = encoder.encode(out, &digest);
    return out;
}

/// Generates a complete `PkcePair` (`code_verifier`, `code_challenge`, `method`).
pub fn generatePkcePair(alloc: std.mem.Allocator, method: PkceMethod, len_43_to_128: usize): PkcePair ! OAuth2Error {
    const verifier = generateCodeVerifier(alloc, len_43_to_128)?;
    errdefer alloc.free(verifier);

    const challenge = switch (method) {
        .S256 => deriveCodeChallengeS256(alloc, verifier)?,
        .Plain => alloc.dupe(u8, verifier) catch return OAuth2Error.OutOfMemory,
    };

    return PkcePair{
        .code_verifier = verifier,
        .code_challenge = challenge,
        .method = method,
    };
}

/// Constant-time verification of `verifier` against `challenge` using `method` (S256 or Plain).
/// Validates RFC 7636 length (43..128) and unreserved charset before comparison; zero heap allocation.
pub fn verifyPkceChallenge(verifier: []const u8, challenge: []const u8, method: PkceMethod): bool {
    if (!isValidCodeVerifier(verifier)) {
        return false;
    }
    return switch (method) {
        .Plain => constantTimeEql(verifier, challenge),
        .S256 => {
            if (challenge.len != 43) return false;
            let digest: [32]u8 = [0; 32];
            std.crypto.hash.sha2.Sha256.hash(verifier, &digest, .{});
            let expected: [43]u8 = [0; 43];
            let * = std.base64.url_safe_no_pad.Encoder.encode(&expected, &digest);
            return constantTimeEql(&expected, challenge);
        },
    };
}

// Source: Absorbed from /data/dev/bun/src/install/integrity.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/resolver/integrity.cp — W3C Subresource Integrity (SRI) Parser & Verifier
// ============================================================================

const std = @import("std");
import { Base64 } from "cap:encoding";

pub const MAX_DIGEST_LEN: usize = 64;

pub err IntegrityError {
    InvalidIntegrity,
    UnsupportedAlgorithm,
    InvalidBase64,
    DigestLengthMismatch,
    BufferTooSmall,
    OutOfMemory,
}

/// W3C SRI digest algorithm tag ordered by cryptographic strength.
pub enum IntegrityTag: u8 {
    Unknown = 0,
    Sha1 = 1,
    Sha256 = 2,
    Sha384 = 3,
    Sha512 = 4,

    fn isSupported(): bool {
        return _ != .Unknown;
    }

    fn digestLen(): usize {
        return switch (_) {
            .Sha1 => 20,
            .Sha256 => 32,
            .Sha384 => 48,
            .Sha512 => 64,
            .Unknown => 0,
        };
    }

    fn prefix(): []const u8 {
        return switch (_) {
            .Sha1 => "sha1-",
            .Sha256 => "sha256-",
            .Sha384 => "sha384-",
            .Sha512 => "sha512-",
            .Unknown => "",
        };
    }

    fn strength(): u8 {
        return _ as u8;
    }
}

fn sriConstantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    for (i of 0..a.len) {
        diff |= (a[i] ^ b[i]);
    }
    return diff == 0;
}

struct TagAndOffset {
    tag: IntegrityTag = .Unknown,
    offset: usize = 0,
}

fn parseTagAndOffset(s: []const u8): TagAndOffset {
    if (std.mem.startsWith(u8, s, "sha512-")) return TagAndOffset{ tag: IntegrityTag.Sha512, offset: 7 };
    if (std.mem.startsWith(u8, s, "sha384-")) return TagAndOffset{ tag: IntegrityTag.Sha384, offset: 7 };
    if (std.mem.startsWith(u8, s, "sha256-")) return TagAndOffset{ tag: IntegrityTag.Sha256, offset: 7 };
    if (std.mem.startsWith(u8, s, "sha1-")) return TagAndOffset{ tag: IntegrityTag.Sha1, offset: 5 };
    return TagAndOffset{ tag: IntegrityTag.Unknown, offset: 0 };
}

/// W3C Subresource Integrity (SRI) descriptor and verifier.
pub struct Integrity {
    tag: IntegrityTag = .Unknown,
    digest: [MAX_DIGEST_LEN]u8 = [0; MAX_DIGEST_LEN],

    /// Returns the active digest byte slice for this integrity tag (requires mutable/lvalue receiver).
    pub fn slice(*_): []const u8 {
        return _.digest[0.._.tag.digestLen()];
    }

    /// Parses a single SRI token (e.g. `"sha512-<base64>"`).
    pub static fn parse(sri: []const u8): Integrity! {
        const trimmed = std.mem.trim(u8, sri, " \t\r\n");
        const parsed_tag = parseTagAndOffset(trimmed);
        const tag = parsed_tag.tag;
        const offset = parsed_tag.offset;
        if (tag == .Unknown) {
            return error.UnsupportedAlgorithm;
        }

        const expected_len = tag.digestLen();
        // Strip optional W3C SRI `?options` suffix if present
        let b64_part = trimmed[offset..];
        if (q_idx := std.mem.indexOfScalar(u8, b64_part, '?')) {
            b64_part = b64_part[0..q_idx];
        }
        if (b64_part.len == 0) {
            return error.InvalidIntegrity;
        }
        if (!Base64.isValid(b64_part)) {
            return error.InvalidBase64;
        }

        const exact_len = Base64.exactDecodedLen(b64_part);
        if (exact_len != expected_len) {
            return error.DigestLengthMismatch;
        }

        let out = Integrity{ tag: tag };
        const decoded_len = Base64.decode(out.digest[0..expected_len], b64_part) catch return error.InvalidBase64;
        if (decoded_len != expected_len) {
            return error.DigestLengthMismatch;
        }
        return out;
    }

    /// Parses a whitespace-separated SRI list and selects the strongest valid algorithm
    /// (`Sha512 > Sha384 > Sha256 > Sha1`). Unknown algorithms are skipped per W3C SRI.
    pub static fn parseMultiple(sri_list: []const u8): Integrity! {
        let best: Integrity? = null;
        let i: usize = 0;
        while (i < sri_list.len) {
            while (i < sri_list.len && (sri_list[i] == ' ' || sri_list[i] == '\t' || sri_list[i] == '\r' || sri_list[i] == '\n')) {
                i += 1;
            }
            if (i >= sri_list.len) break;
            const start = i;
            while (i < sri_list.len && sri_list[i] != ' ' && sri_list[i] != '\t' && sri_list[i] != '\r' && sri_list[i] != '\n') {
                i += 1;
            }
            const token = sri_list[start..i];
            const parsed = Integrity.parse(token) catch continue;
            if (cur := best) {
                if (parsed.tag.strength() > cur.tag.strength()) {
                    best = parsed;
                }
            } else {
                best = parsed;
            }
        }
        if (res := best) {
            return res;
        }
        return error.InvalidIntegrity;
    }

    /// Computes a SHA-1 SRI digest from `data`.
    pub static fn computeSha1(data: []const u8): Integrity {
        let out = Integrity{ tag: .Sha1 };
        std.crypto.hash.Sha1.hash(data, out.digest[0..20], .{});
        return out;
    }

    /// Computes a SHA-256 SRI digest from `data`.
    pub static fn computeSha256(data: []const u8): Integrity {
        let out = Integrity{ tag: .Sha256 };
        std.crypto.hash.sha2.Sha256.hash(data, out.digest[0..32], .{});
        return out;
    }

    /// Computes a SHA-384 SRI digest from `data`.
    pub static fn computeSha384(data: []const u8): Integrity {
        let out = Integrity{ tag: .Sha384 };
        std.crypto.hash.sha2.Sha384.hash(data, out.digest[0..48], .{});
        return out;
    }

    /// Computes a SHA-512 SRI digest from `data`.
    pub static fn computeSha512(data: []const u8): Integrity {
        let out = Integrity{ tag: .Sha512 };
        std.crypto.hash.sha2.Sha512.hash(data, out.digest[0..64], .{});
        return out;
    }

    /// Verifies `data` against this integrity descriptor using constant-time comparison.
    pub fn verify(data: []const u8): bool {
        switch (_.tag) {
            .Sha1 => {
                let actual: [20]u8 = undefined;
                std.crypto.hash.Sha1.hash(data, &actual, .{});
                return sriConstantTimeEql(&actual, _.digest[0..20]);
            },
            .Sha256 => {
                let actual: [32]u8 = undefined;
                std.crypto.hash.sha2.Sha256.hash(data, &actual, .{});
                return sriConstantTimeEql(&actual, _.digest[0..32]);
            },
            .Sha384 => {
                let actual: [48]u8 = undefined;
                std.crypto.hash.sha2.Sha384.hash(data, &actual, .{});
                return sriConstantTimeEql(&actual, _.digest[0..48]);
            },
            .Sha512 => {
                let actual: [64]u8 = undefined;
                std.crypto.hash.sha2.Sha512.hash(data, &actual, .{});
                return sriConstantTimeEql(&actual, _.digest[0..64]);
            },
            .Unknown => return false,
        }
    }

    /// Constant-time equality comparison with another `Integrity`.
    pub fn eql(other: Integrity): bool {
        if (_.tag != other.tag) return false;
        const len = _.tag.digestLen();
        if (len == 0) return true;
        return sriConstantTimeEql(_.digest[0..len], other.digest[0..len]);
    }

    /// Formats the SRI string (`"<algo>-<base64>"`) into `buf`.
    pub fn format(buf: []u8): []const u8! {
        if (_.tag == .Unknown) return error.InvalidIntegrity;
        const pfx = _.tag.prefix();
        const raw = _.digest[0.._.tag.digestLen()];
        const b64_len = Base64.encodeLen(raw.len);
        const total = pfx.len + b64_len;
        if (buf.len < total) return error.BufferTooSmall;
        @memcpy(buf[0..pfx.len], pfx);
        const written = Base64.encode(buf[pfx.len..total], raw);
        return buf[0 .. pfx.len + written];
    }

    /// Allocates and formats the SRI string (`"<algo>-<base64>"`).
    pub fn formatAlloc(allocator: std.mem.Allocator): []u8! {
        if (_.tag == .Unknown) return error.InvalidIntegrity;
        const pfx = _.tag.prefix();
        const raw = _.digest[0.._.tag.digestLen()];
        const b64_len = Base64.encodeLen(raw.len);
        const total = pfx.len + b64_len;
        let buf = allocator.alloc(u8, total) catch return error.OutOfMemory;
        @memcpy(buf[0..pfx.len], pfx);
        const written = Base64.encode(buf[pfx.len..total], raw);
        return buf[0 .. pfx.len + written];
    }
}

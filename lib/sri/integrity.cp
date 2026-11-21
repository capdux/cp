// Source: Absorbed from /data/dev/bun/src/install/integrity.cp
//! W3C Subresource Integrity (SRI) parser, formatter, one-shot & streaming verifier
//! (`sha1-`, `sha256-`, `sha384-`, `sha512-`, plus 40-char hex shasum).

import std from "std";

export const MAX_DIGEST_LEN: usize = 64; // SHA-512

export err SriError {
    InvalidFormat,
    UnsupportedAlgorithm,
    InvalidBase64,
    InvalidHex,
    InvalidDigestLength,
    BufferTooSmall,
}

export enum IntegrityTag: u8 {
    unknown = 0,
    sha1 = 1,
    sha256 = 2,
    sha384 = 3,
    sha512 = 4,

    pub fn isSupported(): bool {
        return _ == .sha1 || _ == .sha256 || _ == .sha384 || _ == .sha512;
    }

    pub fn digestLen(): usize {
        return switch (_) {
            .sha1 => 20,
            .sha256 => 32,
            .sha384 => 48,
            .sha512 => 64,
            .unknown => 0,
        };
    }

    pub fn prefix(): []const u8 {
        return switch (_) {
            .sha1 => "sha1-",
            .sha256 => "sha256-",
            .sha384 => "sha384-",
            .sha512 => "sha512-",
            .unknown => "",
        };
    }

    pub fn strength(): u8 {
        return @intFromEnum(_);
    }
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn constantTimeEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff |= a[i] ^ b[i];
    }
    return diff == 0;
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

/// Parsed Subresource Integrity digest (stack-allocated, zero heap usage).
export struct Integrity {
    tag: IntegrityTag = .unknown,
    value: [MAX_DIGEST_LEN]u8 = [0; MAX_DIGEST_LEN],

    pub fn digestSlice(): []const u8 {
        return _.value[0.._.tag.digestLen()];
    }

    pub fn isSupported(): bool {
        return _.tag.isSupported();
    }

    pub fn eql(other: Integrity): bool {
        if (_.tag != other.tag) return false;
        const n = _.tag.digestLen();
        return constantTimeEql(_.value[0..n], other.value[0..n]);
    }

    /// Parse an SRI string such as `"sha256-<base64>"` or `"sha512-<base64>"`.
    /// Supports multiple space-separated entries (selects the strongest supported algorithm).
    pub static fn parse(input: []const u8): Integrity! {
        let best = Integrity{};
        let found = false;
        let pos: usize = 0;
        while (pos < input.len) {
            while (pos < input.len && (input[pos] == ' ' || input[pos] == '\t' || input[pos] == '\r' || input[pos] == '\n')) : (pos += 1) {}
            if (pos >= input.len) break;
            const start = pos;
            while (pos < input.len && input[pos] != ' ' && input[pos] != '\t' && input[pos] != '\r' && input[pos] != '\n') : (pos += 1) {}
            const tok = input[start..pos];

            if (parsed := parseSingle(tok)) {
                if (!found || parsed.tag.strength() > best.tag.strength()) {
                    best = parsed;
                    found = true;
                }
            }
        }
        if (!found) return err.InvalidFormat;
        return best;
    }

    /// Parse a single SRI token (`"sha256-..."`).
    pub static fn parseSingle(tok: []const u8): Integrity? {
        let dash: usize? = null;
        let i: usize = 0;
        const limit = if (tok.len < 8) tok.len else 8;
        while (i < limit) : (i += 1) {
            if (tok[i] == '-') {
                dash = i;
                break;
            }
        }
        const d = dash ?? return null;
        if (d + 1 >= tok.len) return null;

        const algo = tok[0..d];
        let tag: IntegrityTag = .unknown;
        if (eqlBytes(algo, "sha1")) {
            tag = .sha1;
        } else if (eqlBytes(algo, "sha256")) {
            tag = .sha256;
        } else if (eqlBytes(algo, "sha384")) {
            tag = .sha384;
        } else if (eqlBytes(algo, "sha512")) {
            tag = .sha512;
        } else {
            return null;
        }

        // Strip optional `?options` suffix per W3C SRI spec
        let b64 = tok[d + 1 ..];
        let q: usize = 0;
        while (q < b64.len) : (q += 1) {
            if (b64[q] == '?') {
                b64 = b64[0..q];
                break;
            }
        }

        const expected_len = tag.digestLen();
        let out = Integrity{ tag: tag, value: [0; MAX_DIGEST_LEN] };
        const dec_len = std.base64.standard.Decoder.calcSizeForSlice(b64) catch return null;
        if (dec_len != expected_len) return null;
        std.base64.standard.Decoder.decode(out.value[0..expected_len], b64) catch return null;
        return out;
    }

    /// Parse a 40-character hexadecimal SHA-1 shasum string.
    pub static fn parseSha1Hex(hex: []const u8): Integrity! {
        if (hex.len != 40) return err.InvalidHex;
        let out = Integrity{ tag: .sha1, value: [0; MAX_DIGEST_LEN] };
        let i: usize = 0;
        while (i < 20) : (i += 1) {
            const hi = hexVal(hex[i * 2]) ?? return err.InvalidHex;
            const lo = hexVal(hex[i * 2 + 1]) ?? return err.InvalidHex;
            out.value[i] = (hi << 4) | lo;
        }
        return out;
    }

    /// Compute the SRI digest of `data` using the given algorithm.
    pub static fn compute(tag: IntegrityTag, data: []const u8): Integrity! {
        let out = Integrity{ tag: tag, value: [0; MAX_DIGEST_LEN] };
        switch (tag) {
            .sha1 => {
                let h = std.crypto.hash.Sha1.init(.{});
                h.update(data);
                h.final(out.value[0..20]);
            },
            .sha256 => {
                let h = std.crypto.hash.sha2.Sha256.init(.{});
                h.update(data);
                h.final(out.value[0..32]);
            },
            .sha384 => {
                let h = std.crypto.hash.sha2.Sha384.init(.{});
                h.update(data);
                h.final(out.value[0..48]);
            },
            .sha512 => {
                let h = std.crypto.hash.sha2.Sha512.init(.{});
                h.update(data);
                h.final(out.value[0..64]);
            },
            .unknown => return err.UnsupportedAlgorithm,
        }
        return out;
    }

    /// Verify `data` against this expected SRI digest in constant time.
    pub fn verify(data: []const u8): bool {
        if (!_.tag.isSupported()) return false;
        const computed = Integrity.compute(_.tag, data) catch return false;
        return _.eql(computed);
    }

    /// Format as `"<algo>-<base64>"` into `out` buffer and return the written slice.
    pub fn format(out: []u8): ([]const u8)! {
        if (!_.tag.isSupported()) return err.UnsupportedAlgorithm;
        const pfx = _.tag.prefix();
        const dlen = _.tag.digestLen();
        const b64_len = std.base64.standard.Encoder.calcSize(dlen);
        const total = pfx.len + b64_len;
        if (out.len < total) return err.BufferTooSmall;
        @memcpy(out[0..pfx.len], pfx);
        * = std.base64.standard.Encoder.encode(out[pfx.len..total], _.value[0..dlen]);
        return out[0..total];
    }
}

/// Streaming SRI verifier (supports incremental `update` chunks).
export struct IntegrityVerifier {
    expected: Integrity,
    sha1: std.crypto.hash.Sha1 = std.crypto.hash.Sha1.init(.{}),
    sha256: std.crypto.hash.sha2.Sha256 = std.crypto.hash.sha2.Sha256.init(.{}),
    sha384: std.crypto.hash.sha2.Sha384 = std.crypto.hash.sha2.Sha384.init(.{}),
    sha512: std.crypto.hash.sha2.Sha512 = std.crypto.hash.sha2.Sha512.init(.{}),

    pub static fn init(expected: Integrity): IntegrityVerifier {
        return IntegrityVerifier{
            expected: expected,
            sha1: std.crypto.hash.Sha1.init(.{}),
            sha256: std.crypto.hash.sha2.Sha256.init(.{}),
            sha384: std.crypto.hash.sha2.Sha384.init(.{}),
            sha512: std.crypto.hash.sha2.Sha512.init(.{}),
        };
    }

    pub fn update(*_, chunk: []const u8) {
        if (chunk.len == 0) return;
        switch (_.expected.tag) {
            .sha1 => _.sha1.update(chunk),
            .sha256 => _.sha256.update(chunk),
            .sha384 => _.sha384.update(chunk),
            .sha512 => _.sha512.update(chunk),
            .unknown => {},
        }
    }

    pub fn final(*_): Integrity {
        let out = Integrity{ tag: _.expected.tag, value: [0; MAX_DIGEST_LEN] };
        switch (_.expected.tag) {
            .sha1 => _.sha1.final(out.value[0..20]),
            .sha256 => _.sha256.final(out.value[0..32]),
            .sha384 => _.sha384.final(out.value[0..48]),
            .sha512 => _.sha512.final(out.value[0..64]),
            .unknown => {},
        }
        return out;
    }

    pub fn verify(*_): bool {
        if (!_.expected.tag.isSupported()) return false;
        const computed = _.final();
        return _.expected.eql(computed);
    }
}

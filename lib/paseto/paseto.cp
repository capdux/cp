// Source: cp/lib/paseto/paseto.cp (PASETO v4 Platform-Agnostic Security Tokens)
// Pure CP, Zero-C-Dependency Implementation of PASETO v4 (v4.local & v4.public)
// Spec: https://github.com/paseto-standard/paseto-spec

const std = @import("std");
import { JSON, JsonValue } from "cap:json";

pub const V4_LOCAL_HEADER: string = "v4.local.";
pub const V4_PUBLIC_HEADER: string = "v4.public.";
pub const LOCAL_NONCE_LEN: usize = 32;
pub const LOCAL_TAG_LEN: usize = 16;
pub const PUBLIC_SIG_LEN: usize = 64;

pub err PasetoError {
    InvalidTokenFormat,
    InvalidHeader,
    InvalidSignature,
    DecryptionFailed,
    TokenExpired,
    TokenNotYetValid,
    ClaimMismatch,
    InvalidKey,
    OutOfMemory,
}

pub enum PasetoPurpose {
    Local,
    Public,
}

pub struct PasetoParsed {
    header: string;
    payload_json: []u8;
    footer: []u8;
    purpose: PasetoPurpose;
    allocator: std.mem.Allocator;

    pub fn deinit(*_) {
        if (_.payload_json.len > 0) {
            _.allocator.free(_.payload_json);
            _.payload_json = "";
        }
        if (_.footer.len > 0) {
            _.allocator.free(_.footer);
            _.footer = "";
        }
    }
}

pub struct VerifyClaimsOptions {
    now_unix: i64 = 0;
    expected_iss: string? = null;
    expected_aud: string? = null;
    expected_sub: string? = null;
    clock_skew_sec: u64 = 0;
}

/// Pre-Authentication Encoding (PAE) per PASETO Specification:
/// PAE(pieces) = LE64(len(pieces)) || LE64(len(pieces[0])) || pieces[0] || ...
pub fn paeEncode(alloc: std.mem.Allocator, pieces: []const []const u8): PasetoError![]u8 {
    let total_len: usize = 8;
    for (p of pieces) {
        total_len += 8 + p.len;
    }

    let out = alloc.alloc(u8, total_len) catch return err.OutOfMemory;
    writeLe64(out[0..8], u64(pieces.len));

    let pos: usize = 8;
    for (p of pieces) {
        writeLe64(out[pos .. pos + 8], u64(p.len));
        pos += 8;
        if (p.len > 0) {
            @memcpy(out[pos .. pos + p.len], p);
            pos += p.len;
        }
    }
    return out;
}

fn writeLe64(dst: []u8, val: u64) {
    let v = val;
    for (i of 0..8) {
        dst[i] = u8(v & 0xFF);
        v = v >> 8;
    }
}

fn b64UrlEncodeAlloc(alloc: std.mem.Allocator, data: []const u8): PasetoError![]u8 {
    const encoder = std.base64.url_safe_no_pad.Encoder;
    const enc_len = encoder.calcSize(data.len);
    let out = alloc.alloc(u8, enc_len) catch return err.OutOfMemory;
    zig {
        _ = encoder.encode(out, data);
    }
    return out;
}

fn b64UrlDecodeAlloc(alloc: std.mem.Allocator, encoded: []const u8): PasetoError![]u8 {
    const decoder = std.base64.url_safe_no_pad.Decoder;
    const dec_len = decoder.calcSizeForSlice(encoded) catch return err.InvalidTokenFormat;
    let out = alloc.alloc(u8, dec_len) catch return err.OutOfMemory;
    decoder.decode(out, encoded) catch {
        alloc.free(out);
        return err.InvalidTokenFormat;
    };
    return out;
}

struct SplitPasetoBody {
    body_b64: []const u8;
    footer_b64: []const u8;
    has_footer: bool;
}

fn splitBodyAndFooter(rest: []const u8): PasetoError!SplitPasetoBody {
    if (rest.len == 0) return err.InvalidTokenFormat;
    if (dot_idx := std.mem.indexOfScalar(u8, rest, '.')) {
        if (dot_idx == 0) return err.InvalidTokenFormat;
        const footer_b64 = rest[dot_idx + 1 ..];
        if (footer_b64.len == 0) return err.InvalidTokenFormat;
        if (std.mem.indexOfScalar(u8, footer_b64, '.') != null) return err.InvalidTokenFormat;
        return SplitPasetoBody{
            body_b64: rest[0..dot_idx],
            footer_b64: footer_b64,
            has_footer: true,
        };
    }
    return SplitPasetoBody{
        body_b64: rest,
        footer_b64: "",
        has_footer: false,
    };
}

fn formatPasetoToken(
    alloc: std.mem.Allocator,
    header: string,
    raw_body: []const u8,
    footer: string,
): PasetoError![]u8 {
    const body_b64 = b64UrlEncodeAlloc(alloc, raw_body)?;
    defer alloc.free(body_b64);

    if (footer.len == 0) {
        const total = header.len + body_b64.len;
        let out = alloc.alloc(u8, total) catch return err.OutOfMemory;
        @memcpy(out[0..header.len], header);
        @memcpy(out[header.len..total], body_b64);
        return out;
    }

    const footer_b64 = b64UrlEncodeAlloc(alloc, footer)?;
    defer alloc.free(footer_b64);

    const total = header.len + body_b64.len + 1 + footer_b64.len;
    let out = alloc.alloc(u8, total) catch return err.OutOfMemory;
    @memcpy(out[0..header.len], header);
    @memcpy(out[header.len .. header.len + body_b64.len], body_b64);
    out[header.len + body_b64.len] = '.';
    @memcpy(out[header.len + body_b64.len + 1 .. total], footer_b64);
    return out;
}

struct DerivedLocalKeys {
    ek: [32]u8;
    n2: [24]u8;
}

fn deriveLocalKeys(key32: [32]u8, nonce32: [32]u8): DerivedLocalKeys {
    let ek: [32]u8 = undefined;
    let n2_full: [32]u8 = undefined;

    let ek_in: [50]u8 = undefined;
    @memcpy(ek_in[0..18], "paseto-v4-local-ek");
    @memcpy(ek_in[18..50], &nonce32);
    zig {
        std.crypto.hash.Blake3.hash(&ek_in, &ek, .{ .key = key32 });
    }

    let n2_in: [50]u8 = undefined;
    @memcpy(n2_in[0..18], "paseto-v4-local-n2");
    @memcpy(n2_in[18..50], &nonce32);
    zig {
        std.crypto.hash.Blake3.hash(&n2_in, &n2_full, .{ .key = key32 });
    }

    let n2: [24]u8 = undefined;
    @memcpy(&n2, n2_full[0..24]);
    return DerivedLocalKeys{ ek: ek, n2: n2 };
}

pub class PasetoV4 {
    /// Encrypts `payload_json` into a `v4.local.` token using BLAKE3 key derivation + XChaCha20-Poly1305 AEAD.
    pub static fn encryptLocal(
        alloc: std.mem.Allocator,
        key32: [32]u8,
        payload_json: string,
        footer: string,
        implicit: string,
    ): PasetoError![]u8 {
        let nonce: [32]u8 = undefined;
        zig {
            _ = std.os.linux.getrandom(&nonce, 32, 0);
        }

        const keys = deriveLocalKeys(key32, nonce);
        let pieces: [4][]const u8 = [ V4_LOCAL_HEADER, &nonce, footer, implicit ];
        const aad = paeEncode(alloc, &pieces)?;
        defer alloc.free(aad);

        const raw_len = LOCAL_NONCE_LEN + payload_json.len + LOCAL_TAG_LEN;
        let raw = alloc.alloc(u8, raw_len) catch return err.OutOfMemory;
        defer alloc.free(raw);

        @memcpy(raw[0..LOCAL_NONCE_LEN], &nonce);
        const ct_slice = raw[LOCAL_NONCE_LEN .. LOCAL_NONCE_LEN + payload_json.len];
        let tag: [16]u8 = undefined;
        std.crypto.aead.chacha_poly.XChaCha20Poly1305.encrypt(
            ct_slice,
            &tag,
            payload_json,
            aad,
            keys.n2,
            keys.ek,
        );
        @memcpy(raw[LOCAL_NONCE_LEN + payload_json.len .. raw_len], &tag);

        return formatPasetoToken(alloc, V4_LOCAL_HEADER, raw, footer);
    }

    /// Decrypts and authenticates a `v4.local.` token.
    pub static fn decryptLocal(
        alloc: std.mem.Allocator,
        key32: [32]u8,
        token: string,
        implicit: string,
    ): PasetoError!PasetoParsed {
        if (!std.mem.startsWith(u8, token, V4_LOCAL_HEADER)) {
            return err.InvalidHeader;
        }
        const split = splitBodyAndFooter(token[V4_LOCAL_HEADER.len..])?;

        const raw = b64UrlDecodeAlloc(alloc, split.body_b64)?;
        defer alloc.free(raw);

        if (raw.len < LOCAL_NONCE_LEN + LOCAL_TAG_LEN) {
            return err.InvalidTokenFormat;
        }

        let footer_owned: []u8 = "";
        if (split.has_footer) {
            footer_owned = b64UrlDecodeAlloc(alloc, split.footer_b64)?;
        }
        errdefer if (footer_owned.len > 0) alloc.free(footer_owned);

        let nonce: [32]u8 = undefined;
        @memcpy(&nonce, raw[0..LOCAL_NONCE_LEN]);
        const ct_len = raw.len - LOCAL_NONCE_LEN - LOCAL_TAG_LEN;
        const ct_slice = raw[LOCAL_NONCE_LEN .. LOCAL_NONCE_LEN + ct_len];
        let tag: [16]u8 = undefined;
        @memcpy(&tag, raw[LOCAL_NONCE_LEN + ct_len .. raw.len]);

        const keys = deriveLocalKeys(key32, nonce);
        let pieces: [4][]const u8 = [ V4_LOCAL_HEADER, &nonce, footer_owned, implicit ];
        const aad = paeEncode(alloc, &pieces)?;
        defer alloc.free(aad);

        let plaintext: []u8 = "";
        if (ct_len > 0) {
            plaintext = alloc.alloc(u8, ct_len) catch return err.OutOfMemory;
        }
        errdefer if (plaintext.len > 0) alloc.free(plaintext);

        std.crypto.aead.chacha_poly.XChaCha20Poly1305.decrypt(
            plaintext,
            ct_slice,
            tag,
            aad,
            keys.n2,
            keys.ek,
        ) catch return err.DecryptionFailed;

        return PasetoParsed{
            header: V4_LOCAL_HEADER,
            payload_json: plaintext,
            footer: footer_owned,
            purpose: .Local,
            allocator: alloc,
        };
    }

    /// Signs `payload_json` into a `v4.public.` token using Ed25519.
    pub static fn signPublic(
        alloc: std.mem.Allocator,
        ed25519_secret64: [64]u8,
        payload_json: string,
        footer: string,
        implicit: string,
    ): PasetoError![]u8 {
        const sk = std.crypto.sign.Ed25519.SecretKey.fromBytes(ed25519_secret64) catch return err.InvalidKey;
        const kp = std.crypto.sign.Ed25519.KeyPair.fromSecretKey(sk) catch return err.InvalidKey;

        let pieces: [4][]const u8 = [ V4_PUBLIC_HEADER, payload_json, footer, implicit ];
        const m2 = paeEncode(alloc, &pieces)?;
        defer alloc.free(m2);

        const sig = kp.sign(m2, null) catch return err.InvalidKey;
        const sig_bytes = sig.toBytes();

        const raw_len = payload_json.len + PUBLIC_SIG_LEN;
        let raw = alloc.alloc(u8, raw_len) catch return err.OutOfMemory;
        defer alloc.free(raw);

        if (payload_json.len > 0) {
            @memcpy(raw[0..payload_json.len], payload_json);
        }
        @memcpy(raw[payload_json.len..raw_len], &sig_bytes);

        return formatPasetoToken(alloc, V4_PUBLIC_HEADER, raw, footer);
    }

    /// Verifies a `v4.public.` token using an Ed25519 32-byte public key.
    pub static fn verifyPublic(
        alloc: std.mem.Allocator,
        ed25519_pub32: [32]u8,
        token: string,
        implicit: string,
    ): PasetoError!PasetoParsed {
        if (!std.mem.startsWith(u8, token, V4_PUBLIC_HEADER)) {
            return err.InvalidHeader;
        }
        const split = splitBodyAndFooter(token[V4_PUBLIC_HEADER.len..])?;

        const raw = b64UrlDecodeAlloc(alloc, split.body_b64)?;
        defer alloc.free(raw);

        if (raw.len < PUBLIC_SIG_LEN) {
            return err.InvalidTokenFormat;
        }

        let footer_owned: []u8 = "";
        if (split.has_footer) {
            footer_owned = b64UrlDecodeAlloc(alloc, split.footer_b64)?;
        }
        errdefer if (footer_owned.len > 0) alloc.free(footer_owned);

        const payload_len = raw.len - PUBLIC_SIG_LEN;
        const payload_slice = raw[0..payload_len];
        let sig_bytes: [64]u8 = undefined;
        @memcpy(&sig_bytes, raw[payload_len..raw.len]);

        const pk = std.crypto.sign.Ed25519.PublicKey.fromBytes(ed25519_pub32) catch return err.InvalidKey;
        const parsed_sig = std.crypto.sign.Ed25519.Signature.fromBytes(sig_bytes);

        let pieces: [4][]const u8 = [ V4_PUBLIC_HEADER, payload_slice, footer_owned, implicit ];
        const m2 = paeEncode(alloc, &pieces)?;
        defer alloc.free(m2);

        parsed_sig.verify(m2, pk) catch return err.InvalidSignature;

        let payload_owned: []u8 = "";
        if (payload_len > 0) {
            payload_owned = alloc.alloc(u8, payload_len) catch return err.OutOfMemory;
            @memcpy(payload_owned, payload_slice);
        }

        return PasetoParsed{
            header: V4_PUBLIC_HEADER,
            payload_json: payload_owned,
            footer: footer_owned,
            purpose: .Public,
            allocator: alloc,
        };
    }

    /// Validates standard PASETO registered claims (`exp`, `nbf`, `iat`, `iss`, `aud`, `sub`, `jti`).
    pub static fn verifyClaims(
        alloc: std.mem.Allocator,
        payload_json: string,
        opts: VerifyClaimsOptions,
    ): PasetoError!void {
        let tmp_arena = std.heap.ArenaAllocator.init(alloc);
        defer tmp_arena.deinit();

        const root = JSON.parse(tmp_arena.allocator(), payload_json) catch return err.InvalidTokenFormat;
        if (!root.isObject()) return err.ClaimMismatch;

        const skew: i64 = i64(opts.clock_skew_sec);

        if (exp_val := root.get("exp")) {
            if (exp_num := exp_val.asNumber()) {
                const exp_unix: i64 = i64(exp_num);
                if (opts.now_unix >= exp_unix + skew) {
                    return err.TokenExpired;
                }
            } else {
                return err.ClaimMismatch;
            }
        }

        if (nbf_val := root.get("nbf")) {
            if (nbf_num := nbf_val.asNumber()) {
                const nbf_unix: i64 = i64(nbf_num);
                if (opts.now_unix + skew < nbf_unix) {
                    return err.TokenNotYetValid;
                }
            } else {
                return err.ClaimMismatch;
            }
        }

        if (iat_val := root.get("iat")) {
            if (iat_val.asNumber() == null) return err.ClaimMismatch;
        }

        if (jti_val := root.get("jti")) {
            if (jti_val.asString() == null) return err.ClaimMismatch;
        }

        if (exp_iss := opts.expected_iss) {
            if (iss_val := root.get("iss")) {
                if (iss_str := iss_val.asString()) {
                    if (!std.mem.eql(u8, iss_str, exp_iss)) return err.ClaimMismatch;
                } else {
                    return err.ClaimMismatch;
                }
            } else {
                return err.ClaimMismatch;
            }
        }

        if (exp_aud := opts.expected_aud) {
            if (aud_val := root.get("aud")) {
                if (aud_str := aud_val.asString()) {
                    if (!std.mem.eql(u8, aud_str, exp_aud)) return err.ClaimMismatch;
                } else {
                    return err.ClaimMismatch;
                }
            } else {
                return err.ClaimMismatch;
            }
        }

        if (exp_sub := opts.expected_sub) {
            if (sub_val := root.get("sub")) {
                if (sub_str := sub_val.asString()) {
                    if (!std.mem.eql(u8, sub_str, exp_sub)) return err.ClaimMismatch;
                } else {
                    return err.ClaimMismatch;
                }
            } else {
                return err.ClaimMismatch;
            }
        }
    }
}

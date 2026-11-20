// Source: cp/lib/paseto/v4_local.cp (PASETO v4.local XChaCha20 + BLAKE2b Authenticated Encryption)

const std = @import("std");
import {
    PasetoError,
    PasetoPurpose,
    paeEncode,
    encodeBase64Url,
    decodeBase64Url,
    parseToken,
} from "./types.cp";

pub const HEADER_LOCAL: []const u8 = "v4.local.";
const ENC_KEY_DOMAIN: []const u8 = "paseto-encryption-key";
const AUTH_KEY_DOMAIN: []const u8 = "paseto-auth-key-for-aead";

const Blake2b448 = std.crypto.hash.blake2.Blake2b(448);
const Blake2b256 = std.crypto.hash.blake2.Blake2b256;

/// Generates a random 32-byte symmetric key for PASETO v4.local.
pub fn generateV4LocalKey(): [32]u8 {
    let key: [32]u8 = undefined;
    * = std.os.linux.getrandom(&key, 32, 0);
    return key;
}

const EncKeyNonce = struct {
    ek: [32]u8,
    n2: [24]u8,
};

fn deriveEncryptionKeyAndNonce(key_32: [32]u8, nonce_32: [32]u8): EncKeyNonce {
    let h = Blake2b448.init(.{ .key = &key_32 });
    h.update(ENC_KEY_DOMAIN);
    h.update(&nonce_32);
    let tmp: [56]u8 = undefined;
    h.final(&tmp);
    return EncKeyNonce{
        .ek = tmp[0..32].*,
        .n2 = tmp[32..56].*,
    };
}

fn deriveAuthKey(key_32: [32]u8, nonce_32: [32]u8): [32]u8 {
    let h = Blake2b256.init(.{ .key = &key_32 });
    h.update(AUTH_KEY_DOMAIN);
    h.update(&nonce_32);
    let ak: [32]u8 = undefined;
    h.final(&ak);
    return ak;
}

fn computeLocalTag(ak: [32]u8, pre_auth: []const u8): [32]u8 {
    let h = Blake2b256.init(.{ .key = &ak });
    h.update(pre_auth);
    let tag: [32]u8 = undefined;
    h.final(&tag);
    return tag;
}

/// Encrypts a payload using PASETO `v4.local.` (XChaCha20 + BLAKE2b key splitting & MAC).
pub fn encryptV4Local(
    alloc: std.mem.Allocator,
    key_32: [32]u8,
    payload: []const u8,
    footer: []const u8,
    implicit_assertion: []const u8,
): ([]u8) ! PasetoError {
    let n: [32]u8 = undefined;
    * = std.os.linux.getrandom(&n, 32, 0);

    const enc = deriveEncryptionKeyAndNonce(key_32, n);
    const ak = deriveAuthKey(key_32, n);

    const raw = alloc.alloc(u8, 32 + payload.len + 32) catch return PasetoError.OutOfMemory;
    defer alloc.free(raw);

    @memcpy(raw[0..32], &n);
    const c = raw[32 .. 32 + payload.len];
    std.crypto.stream.chacha.XChaCha20IETF.xor(c, payload, 0, enc.ek, enc.n2);

    const pieces: [5][]const u8 = [ HEADER_LOCAL, &n, c, footer, implicit_assertion ];
    const pre_auth = paeEncode(alloc, &pieces) ?;
    defer alloc.free(pre_auth);

    const tag = computeLocalTag(ak, pre_auth);
    @memcpy(raw[32 + payload.len .. 32 + payload.len + 32], &tag);

    const payload_b64 = encodeBase64Url(alloc, raw) ?;
    defer alloc.free(payload_b64);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, HEADER_LOCAL) catch return PasetoError.OutOfMemory;
    out.appendSlice(alloc, payload_b64) catch return PasetoError.OutOfMemory;

    if (footer.len > 0) {
        const footer_b64 = encodeBase64Url(alloc, footer) ?;
        defer alloc.free(footer_b64);
        out.append(alloc, '.') catch return PasetoError.OutOfMemory;
        out.appendSlice(alloc, footer_b64) catch return PasetoError.OutOfMemory;
    }

    return out.toOwnedSlice(alloc) catch return PasetoError.OutOfMemory;
}

/// Decrypts and authenticates a PASETO `v4.local.` token, returning the owned plaintext payload.
pub fn decryptV4Local(
    alloc: std.mem.Allocator,
    key_32: [32]u8,
    token: []const u8,
    implicit_assertion: []const u8,
): ([]u8) ! PasetoError {
    const parsed = parseToken(token) ?;
    if (parsed.purpose != PasetoPurpose.Local) {
        return PasetoError.InvalidPurpose;
    }

    const raw = decodeBase64Url(alloc, parsed.payload_b64) ?;
    defer alloc.free(raw);

    if (raw.len < 64) {
        return PasetoError.InvalidFormat;
    }

    const n = raw[0..32].*;
    const c_len = raw.len - 64;
    const c = raw[32 .. 32 + c_len];
    const t = raw[32 + c_len .. raw.len];

    let footer: []u8 = alloc.alloc(u8, 0) catch return PasetoError.OutOfMemory;
    if (parsed.footer_b64.len > 0) {
        alloc.free(footer);
        footer = decodeBase64Url(alloc, parsed.footer_b64) ?;
    }
    defer alloc.free(footer);

    const ak = deriveAuthKey(key_32, n);
    const pieces: [5][]const u8 = [ HEADER_LOCAL, &n, c, footer, implicit_assertion ];
    const pre_auth = paeEncode(alloc, &pieces) ?;
    defer alloc.free(pre_auth);

    const t2 = computeLocalTag(ak, pre_auth);
    if (!std.crypto.timing_safe.eql([32]u8, t[0..32].*, t2)) {
        return PasetoError.AuthenticationFailed;
    }

    const enc = deriveEncryptionKeyAndNonce(key_32, n);
    const out = alloc.alloc(u8, c_len) catch return PasetoError.OutOfMemory;
    std.crypto.stream.chacha.XChaCha20IETF.xor(out, c, 0, enc.ek, enc.n2);
    return out;
}

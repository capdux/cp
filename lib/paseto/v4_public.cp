// Source: cp/lib/paseto/v4_public.cp (PASETO v4.public Ed25519 Asymmetric Sign & Verify)

const std = @import("std");
import {
    PasetoError,
    PasetoPurpose,
    paeEncode,
    encodeBase64Url,
    decodeBase64Url,
    parseToken,
} from "./types.cp";

pub const HEADER_PUBLIC: []const u8 = "v4.public.";

pub const V4PublicKeyPair = struct {
    secret_key: [64]u8,
    public_key: [32]u8,
};

/// Generates a fresh Ed25519 key pair for PASETO v4.public.
pub fn generateV4PublicKeyPair(): V4PublicKeyPair {
    let seed: [32]u8 = undefined;
    * = std.os.linux.getrandom(&seed, 32, 0);
    const kp = std.crypto.sign.Ed25519.KeyPair.generateDeterministic(seed) catch unreachable;
    return V4PublicKeyPair{
        .secret_key = kp.secret_key.toBytes(),
        .public_key = kp.public_key.toBytes(),
    };
}

/// Signs a payload with Ed25519 to produce a PASETO `v4.public.` token.
pub fn signV4Public(
    alloc: std.mem.Allocator,
    secret_key_64: [64]u8,
    payload: []const u8,
    footer: []const u8,
    implicit_assertion: []const u8,
): ([]u8) ! PasetoError {
    const sk = std.crypto.sign.Ed25519.SecretKey.fromBytes(secret_key_64) catch {
        return PasetoError.InvalidKey;
    };
    const kp = std.crypto.sign.Ed25519.KeyPair.fromSecretKey(sk) catch {
        return PasetoError.InvalidKey;
    };

    const pieces: [4][]const u8 = [ HEADER_PUBLIC, payload, footer, implicit_assertion ];
    const m2 = paeEncode(alloc, &pieces) ?;
    defer alloc.free(m2);

    const sig = kp.sign(m2, null) catch {
        return PasetoError.InvalidKey;
    };
    const sig_bytes = sig.toBytes();

    const raw = alloc.alloc(u8, payload.len + 64) catch return PasetoError.OutOfMemory;
    defer alloc.free(raw);
    @memcpy(raw[0..payload.len], payload);
    @memcpy(raw[payload.len .. payload.len + 64], &sig_bytes);

    const payload_b64 = encodeBase64Url(alloc, raw) ?;
    defer alloc.free(payload_b64);

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    out.appendSlice(alloc, HEADER_PUBLIC) catch return PasetoError.OutOfMemory;
    out.appendSlice(alloc, payload_b64) catch return PasetoError.OutOfMemory;

    if (footer.len > 0) {
        const footer_b64 = encodeBase64Url(alloc, footer) ?;
        defer alloc.free(footer_b64);
        out.append(alloc, '.') catch return PasetoError.OutOfMemory;
        out.appendSlice(alloc, footer_b64) catch return PasetoError.OutOfMemory;
    }

    return out.toOwnedSlice(alloc) catch return PasetoError.OutOfMemory;
}

/// Verifies a PASETO `v4.public.` token using an Ed25519 public key and returns the owned payload.
pub fn verifyV4Public(
    alloc: std.mem.Allocator,
    public_key_32: [32]u8,
    token: []const u8,
    implicit_assertion: []const u8,
): ([]u8) ! PasetoError {
    const parsed = parseToken(token) ?;
    if (parsed.purpose != PasetoPurpose.Public) {
        return PasetoError.InvalidPurpose;
    }

    const raw = decodeBase64Url(alloc, parsed.payload_b64) ?;
    defer alloc.free(raw);

    if (raw.len < 64) {
        return PasetoError.InvalidFormat;
    }

    const msg_len = raw.len - 64;
    const payload = raw[0..msg_len];
    const sig_slice = raw[msg_len..raw.len];

    let footer: []u8 = alloc.alloc(u8, 0) catch return PasetoError.OutOfMemory;
    if (parsed.footer_b64.len > 0) {
        alloc.free(footer);
        footer = decodeBase64Url(alloc, parsed.footer_b64) ?;
    }
    defer alloc.free(footer);

    const pieces: [4][]const u8 = [ HEADER_PUBLIC, payload, footer, implicit_assertion ];
    const m2 = paeEncode(alloc, &pieces) ?;
    defer alloc.free(m2);

    const pk = std.crypto.sign.Ed25519.PublicKey.fromBytes(public_key_32) catch {
        return PasetoError.InvalidKey;
    };
    const sig = std.crypto.sign.Ed25519.Signature.fromBytes(sig_slice[0..64].*);
    sig.verify(m2, pk) catch {
        return PasetoError.InvalidSignature;
    };

    const out = alloc.alloc(u8, msg_len) catch return PasetoError.OutOfMemory;
    @memcpy(out, payload);
    return out;
}

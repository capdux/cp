// Reference Only: Functional overlap with lib/crypto/asymmetric.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (RFC 8032 Ed25519 Signatures)

const std = @import("std");
const Ed25519 = std.crypto.sign.Ed25519;

export const PUBLIC_KEY_LEN: usize = 32;
export const SECRET_KEY_LEN: usize = 64;
export const SEED_LEN: usize = 32;
export const SIGNATURE_LEN: usize = 64;

export const Ed25519Error = error {
    InvalidLength,
    InvalidHex,
    InvalidEncoding,
    SignatureVerificationFailed,
    KeyGenerationFailed,
    IdentityElement,
    NonCanonical,
    WeakPublicKey,
};

fn nibbleFromChar(ch: u8): Ed25519Error!u8 {
    if (ch >= '0' and ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'a' and ch <= 'f') {
        return ch - 'a' + 10;
    }
    if (ch >= 'A' and ch <= 'F') {
        return ch - 'A' + 10;
    }
    return Ed25519Error.InvalidHex;
}

fn encodeHexImpl(bytes: []const u8, out: []u8): Ed25519Error![]const u8 {
    const needed: usize = bytes.len * 2;
    if (out.len < needed) {
        return Ed25519Error.InvalidLength;
    }
    const alphabet = "0123456789abcdef";
    let i: usize = 0;
    while (i < bytes.len) {
        const b = bytes[i];
        const hi: usize = @intCast(b >> 4);
        const lo: usize = @intCast(b & 0x0f);
        const out_hi: usize = i * 2;
        const out_lo: usize = i * 2 + 1;
        out[out_hi] = alphabet[hi];
        out[out_lo] = alphabet[lo];
        i += 1;
    }
    return out[0..needed];
}

fn decodeHexImpl(hex: []const u8, out: []u8): Ed25519Error![]u8 {
    if (hex.len % 2 != 0) {
        return Ed25519Error.InvalidLength;
    }
    const byte_len: usize = hex.len / 2;
    if (out.len < byte_len) {
        return Ed25519Error.InvalidLength;
    }
    let i: usize = 0;
    while (i < byte_len) {
        const hi_idx: usize = i * 2;
        const lo_idx: usize = i * 2 + 1;
        const hi = try nibbleFromChar(hex[hi_idx]);
        const lo = try nibbleFromChar(hex[lo_idx]);
        out[i] = (hi << 4) | lo;
        i += 1;
    }
    return out[0..byte_len];
}

export fn toHex(bytes: []const u8, out: []u8): Ed25519Error![]const u8 {
    return encodeHexImpl(bytes, out);
}

export fn fromHex(hex: []const u8, out: []u8): Ed25519Error![]u8 {
    return decodeHexImpl(hex, out);
}

export struct PublicKey {
    bytes: [32]u8,

    pub fn fromBytes(raw: [32]u8): PublicKey {
        return PublicKey{ .bytes = raw };
    }

    pub fn fromSlice(slice: []const u8): Ed25519Error!PublicKey {
        if (slice.len != PUBLIC_KEY_LEN) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [32]u8 = undefined;
        @memcpy(&buf, slice);
        return PublicKey{ .bytes = buf };
    }

    pub fn fromHex(hex: []const u8): Ed25519Error!PublicKey {
        if (hex.len != PUBLIC_KEY_LEN * 2) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [32]u8 = undefined;
        * = try decodeHexImpl(hex, &buf);
        return PublicKey{ .bytes = buf };
    }

    pub fn toBytes(): [32]u8 {
        return _.bytes;
    }

    pub fn toHex(out: *[64]u8) {
        * = encodeHexImpl(&_.bytes, out) catch {};
    }

    pub fn verify(msg: []const u8, sig: Signature): bool {
        return verifyImpl(_, msg, sig);
    }
}

export struct SecretKey {
    bytes: [64]u8,

    pub fn fromBytes(raw: [64]u8): SecretKey {
        return SecretKey{ .bytes = raw };
    }

    pub fn fromSlice(slice: []const u8): Ed25519Error!SecretKey {
        if (slice.len != SECRET_KEY_LEN) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [64]u8 = undefined;
        @memcpy(&buf, slice);
        return SecretKey{ .bytes = buf };
    }

    pub fn fromHex(hex: []const u8): Ed25519Error!SecretKey {
        if (hex.len != SECRET_KEY_LEN * 2) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [64]u8 = undefined;
        * = try decodeHexImpl(hex, &buf);
        return SecretKey{ .bytes = buf };
    }

    pub fn toBytes(): [64]u8 {
        return _.bytes;
    }

    pub fn seedBytes(): [32]u8 {
        let out: [32]u8 = undefined;
        @memcpy(&out, _.bytes[0..32]);
        return out;
    }

    pub fn publicKey(): PublicKey {
        let out: [32]u8 = undefined;
        @memcpy(&out, _.bytes[32..64]);
        return PublicKey{ .bytes = out };
    }
}

export struct Signature {
    bytes: [64]u8,

    pub fn fromBytes(raw: [64]u8): Signature {
        return Signature{ .bytes = raw };
    }

    pub fn fromSlice(slice: []const u8): Ed25519Error!Signature {
        if (slice.len != SIGNATURE_LEN) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [64]u8 = undefined;
        @memcpy(&buf, slice);
        return Signature{ .bytes = buf };
    }

    pub fn fromHex(hex: []const u8): Ed25519Error!Signature {
        if (hex.len != SIGNATURE_LEN * 2) {
            return Ed25519Error.InvalidLength;
        }
        let buf: [64]u8 = undefined;
        * = try decodeHexImpl(hex, &buf);
        return Signature{ .bytes = buf };
    }

    pub fn toBytes(): [64]u8 {
        return _.bytes;
    }

    pub fn toHex(out: *[128]u8) {
        * = encodeHexImpl(&_.bytes, out) catch {};
    }

    pub fn verify(msg: []const u8, pub_key: PublicKey): bool {
        return verifyImpl(pub_key, msg, _);
    }
}

export struct KeyPair {
    public_key: PublicKey,
    secret_key: SecretKey,

    pub fn generate(): Ed25519Error!KeyPair {
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        let seed_buf: [32]u8 = undefined;
        threaded.io().random(&seed_buf);
        return KeyPair.fromSeed(seed_buf);
    }

    pub static fn fromSeed(seed_bytes: [32]u8): Ed25519Error!KeyPair {
        const kp = Ed25519.KeyPair.generateDeterministic(seed_bytes) catch {
            return Ed25519Error.KeyGenerationFailed;
        };
        return KeyPair{
            .public_key = PublicKey{ .bytes = kp.public_key.toBytes() },
            .secret_key = SecretKey{ .bytes = kp.secret_key.toBytes() },
        };
    }

    pub fn fromSecretKey(sk: SecretKey): KeyPair {
        return KeyPair{
            .public_key = sk.publicKey(),
            .secret_key = sk,
        };
    }

    pub fn sign(msg: []const u8): Ed25519Error!Signature {
        const pk = Ed25519.PublicKey.fromBytes(_.public_key.bytes) catch {
            return Ed25519Error.InvalidEncoding;
        };
        const sk = Ed25519.SecretKey.fromBytes(_.secret_key.bytes) catch {
            return Ed25519Error.InvalidEncoding;
        };
        const kp = Ed25519.KeyPair{
            .public_key = pk,
            .secret_key = sk,
        };
        const sig = kp.sign(msg, null) catch {
            return Ed25519Error.KeyGenerationFailed;
        };
        return Signature{ .bytes = sig.toBytes() };
    }

    pub fn signRandomized(msg: []const u8): Ed25519Error!Signature {
        const pk = Ed25519.PublicKey.fromBytes(_.public_key.bytes) catch {
            return Ed25519Error.InvalidEncoding;
        };
        const sk = Ed25519.SecretKey.fromBytes(_.secret_key.bytes) catch {
            return Ed25519Error.InvalidEncoding;
        };
        const kp = Ed25519.KeyPair{
            .public_key = pk,
            .secret_key = sk,
        };
        let threaded = std.Io.Threaded.init(std.heap.page_allocator, .{});
        defer threaded.deinit();
        let noise: [32]u8 = undefined;
        threaded.io().random(&noise);
        const sig = kp.sign(msg, noise) catch {
            return Ed25519Error.KeyGenerationFailed;
        };
        return Signature{ .bytes = sig.toBytes() };
    }
}

fn verifyImpl(pub_key: PublicKey, msg: []const u8, sig: Signature): bool {
    const pk = Ed25519.PublicKey.fromBytes(pub_key.bytes) catch {
        return false;
    };
    const s = Ed25519.Signature.fromBytes(sig.bytes);
    s.verify(msg, pk) catch {
        return false;
    };
    return true;
}

export fn verify(pub_key: PublicKey, msg: []const u8, sig: Signature): bool {
    return verifyImpl(pub_key, msg, sig);
}

export fn verifyBatch(pub_keys: []const PublicKey, msgs: []const []const u8, sigs: []const Signature): bool {
    if (pub_keys.len != msgs.len or pub_keys.len != sigs.len) {
        return false;
    }
    let i: usize = 0;
    while (i < pub_keys.len) {
        if (!verify(pub_keys[i], msgs[i], sigs[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

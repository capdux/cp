// Reference-hardened against RustCrypto/SSH (MIT/Apache-2.0)
// Source: cp/lib/ssh_wire/keys.cp (OpenSSH Public Key & Fingerprint)
const std = @import("std");
import {
    SSH_MAX_WIRE_STRING_LEN,
    SshError,
    SshReader,
    SshWriter,
    simdSshCopyBytes,
    simdSshCtEqual,
} from "./sshWire.cp";

pub enum SshKeyType {
    Ed25519,
    EcdsaP256,

    pub fn toAlgorithmName() -> []const u8 {
        return switch (_) {
            .Ed25519 => "ssh-ed25519",
            .EcdsaP256 => "ecdsa-sha2-nistp256",
        };
    }

    static fn fromAlgorithmName(alg_name: []const u8) -> SshError!SshKeyType {
        if (std.mem.eql(u8, alg_name, "ssh-ed25519")) {
            return .Ed25519;
        }
        if (std.mem.eql(u8, alg_name, "ecdsa-sha2-nistp256")) {
            return .EcdsaP256;
        }
        return SshError.InvalidKeyType;
    }
}

pub struct SshPublicKey {
    key_type: SshKeyType = .Ed25519;
    ed25519_pub: [32]u8;
    ecdsa_p256_pub: [65]u8;
    comment: []u8;
    raw_blob: []u8;

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.comment);
        alloc.free(_.raw_blob);
    }
}

pub fn buildEd25519PublicKeyBlob(alloc: std.mem.Allocator, pub_32: [32]u8) -> SshError![]u8 {
    let writer = SshWriter.init(alloc);
    errdefer writer.deinit();
    try writer.writeString("ssh-ed25519");
    try writer.writeString(&pub_32);
    return try writer.toOwnedSlice();
}

pub fn buildEcdsaP256PublicKeyBlob(alloc: std.mem.Allocator, pub_65: [65]u8) -> SshError![]u8 {
    if (pub_65[0] != 0x04) {
        return SshError.InvalidFormat;
    }
    let writer = SshWriter.init(alloc);
    errdefer writer.deinit();
    try writer.writeString("ecdsa-sha2-nistp256");
    try writer.writeString("nistp256");
    try writer.writeString(&pub_65);
    return try writer.toOwnedSlice();
}

pub fn parsePublicKeyLine(alloc: std.mem.Allocator, line: []const u8) -> SshError!SshPublicKey {
    const trimmed = std.mem.trim(u8, line, " \t\r\n");
    if (trimmed.len == 0) {
        return SshError.InvalidFormat;
    }
    for (ch of trimmed) {
        if (ch == '\r' or ch == '\n' or ch == 0) {
            return SshError.InvalidFormat;
        }
    }

    const first_space = std.mem.indexOfAny(u8, trimmed, " \t") orelse return SshError.InvalidFormat;
    const alg_str = trimmed[0..first_space];
    const key_type = try SshKeyType.fromAlgorithmName(alg_str);

    const after_alg = std.mem.trimStart(u8, trimmed[first_space + 1 ..], " \t");
    if (after_alg.len == 0) {
        return SshError.InvalidFormat;
    }

    let b64_str: []const u8 = after_alg;
    let comment_slice: []const u8 = "";
    if (second_space := std.mem.indexOfAny(u8, after_alg, " \t")) {
        b64_str = after_alg[0..second_space];
        comment_slice = std.mem.trimStart(u8, after_alg[second_space + 1 ..], " \t");
    }
    if (b64_str.len == 0) {
        return SshError.InvalidFormat;
    }

    const decoded_len = std.base64.standard.Decoder.calcSizeForSlice(b64_str) catch return SshError.InvalidBase64;
    if (decoded_len > SSH_MAX_WIRE_STRING_LEN) {
        return SshError.PayloadTooLarge;
    }
    const blob = alloc.alloc(u8, decoded_len) catch return SshError.OutOfMemory;
    errdefer alloc.free(blob);
    std.base64.standard.Decoder.decode(blob, b64_str) catch return SshError.InvalidBase64;

    let reader = SshReader.init(blob);
    const inner_alg = try reader.readString();
    if (!simdSshCtEqual(inner_alg, alg_str)) {
        return SshError.KeyTypeMismatch;
    }

    let ed25519_pub = std.mem.zeroes([32]u8);
    let ecdsa_p256_pub = std.mem.zeroes([65]u8);

    switch (key_type) {
        .Ed25519 => {
            const key_bytes = try reader.readString();
            if (key_bytes.len != 32) {
                return SshError.InvalidFormat;
            }
            simdSshCopyBytes(&ed25519_pub, key_bytes);
        },
        .EcdsaP256 => {
            const curve = try reader.readString();
            if (!simdSshCtEqual(curve, "nistp256")) {
                return SshError.InvalidFormat;
            }
            const point = try reader.readString();
            if (point.len != 65 or point[0] != 0x04) {
                return SshError.InvalidFormat;
            }
            simdSshCopyBytes(&ecdsa_p256_pub, point);
        },
    }

    if (reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }

    const comment_owned = alloc.dupe(u8, comment_slice) catch return SshError.OutOfMemory;
    errdefer alloc.free(comment_owned);

    return SshPublicKey{
        key_type: key_type,
        ed25519_pub: ed25519_pub,
        ecdsa_p256_pub: ecdsa_p256_pub,
        comment: comment_owned,
        raw_blob: blob,
    };
}

pub fn formatPublicKeyLine(alloc: std.mem.Allocator, key: *const SshPublicKey) -> SshError![]u8 {
    const alg = key.key_type.toAlgorithmName();
    const b64_len = std.base64.standard.Encoder.calcSize(key.raw_blob.len);
    const b64_buf = alloc.alloc(u8, b64_len) catch return SshError.OutOfMemory;
    defer alloc.free(b64_buf);
    const b64 = std.base64.standard.Encoder.encode(b64_buf, key.raw_blob);

    if (key.comment.len > 0) {
        return std.fmt.allocPrint(alloc, "{s} {s} {s}", .{ alg, b64, key.comment }) catch return SshError.OutOfMemory;
    }
    return std.fmt.allocPrint(alloc, "{s} {s}", .{ alg, b64 }) catch return SshError.OutOfMemory;
}

pub fn computeSha256Fingerprint(alloc: std.mem.Allocator, raw_blob: []const u8) -> SshError![]u8 {
    let digest = std.mem.zeroes([32]u8);
    std.crypto.hash.sha2.Sha256.hash(raw_blob, &digest, .{});

    const b64_len = std.base64.standard_no_pad.Encoder.calcSize(digest.len);
    const b64_buf = alloc.alloc(u8, b64_len) catch return SshError.OutOfMemory;
    defer alloc.free(b64_buf);
    const b64 = std.base64.standard_no_pad.Encoder.encode(b64_buf, &digest);

    return std.fmt.allocPrint(alloc, "SHA256:{s}", .{b64}) catch return SshError.OutOfMemory;
}

/// Constant-time comparison of two `SshPublicKey` values (ignores comment).
pub fn publicKeyEquals(a: *const SshPublicKey, b: *const SshPublicKey): bool {
    if (a.key_type != b.key_type) {
        return false;
    }
    if (!simdSshCtEqual(a.raw_blob, b.raw_blob)) {
        return false;
    }
    return switch (a.key_type) {
        .Ed25519 => simdSshCtEqual(&a.ed25519_pub, &b.ed25519_pub),
        .EcdsaP256 => simdSshCtEqual(&a.ecdsa_p256_pub, &b.ecdsa_p256_pub),
    };
}

/// Computes the SHA-256 fingerprint of `raw_blob` and compares it in constant time
/// against `expected_fp` (e.g. `"SHA256:..."`).
pub fn verifySha256Fingerprint(
    alloc: std.mem.Allocator,
    raw_blob: []const u8,
    expected_fp: []const u8,
) -> SshError!bool {
    const actual = try computeSha256Fingerprint(alloc, raw_blob);
    defer alloc.free(actual);
    return simdSshCtEqual(actual, expected_fp);
}

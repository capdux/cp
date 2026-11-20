// Source: cp/lib/ssh_wire/sshsig.cp (OpenSSH SSHSIG Signing & Verification)
const std = @import("std");
import { SshError, SshReader, SshWriter } from "./wire.cp";
import { buildEd25519PublicKeyBlob } from "./keys.cp";
import { decodePemArmoring, encodePemArmoring } from "./openssh_key.cp";

const SSHSIG_MAGIC = "SSHSIG";
const SSHSIG_VERSION: u32 = 1;
const SSHSIG_PEM_HEADER = "-----BEGIN SSH SIGNATURE-----";
const SSHSIG_PEM_FOOTER = "-----END SSH SIGNATURE-----";

fn buildSshSigSignedData(
    alloc: std.mem.Allocator,
    namespace: []const u8,
    reserved: []const u8,
    hash_alg: []const u8,
    msg_hash: []const u8,
) -> SshError![]u8 {
    let w = SshWriter.init(alloc);
    errdefer w.deinit();
    try w.writeRaw(SSHSIG_MAGIC);
    try w.writeString(namespace);
    try w.writeString(reserved);
    try w.writeString(hash_alg);
    try w.writeString(msg_hash);
    return try w.toOwnedSlice();
}

pub fn signSshSigEd25519(
    alloc: std.mem.Allocator,
    secret_64: [64]u8,
    namespace: []const u8,
    message: []const u8,
) -> SshError![]u8 {
    if (namespace.len == 0) {
        return SshError.InvalidNamespace;
    }

    let msg_hash = std.mem.zeroes([64]u8);
    std.crypto.hash.sha2.Sha512.hash(message, &msg_hash, .{});

    const signed_data = try buildSshSigSignedData(alloc, namespace, "", "sha512", &msg_hash);
    defer alloc.free(signed_data);

    const sk = std.crypto.sign.Ed25519.SecretKey.fromBytes(secret_64) catch return SshError.InvalidFormat;
    const kp = std.crypto.sign.Ed25519.KeyPair.fromSecretKey(sk) catch return SshError.InvalidFormat;
    const sig = kp.sign(signed_data, null) catch return SshError.InvalidSignature;
    const sig_bytes = sig.toBytes();

    let sig_blob_writer = SshWriter.init(alloc);
    defer sig_blob_writer.deinit();
    try sig_blob_writer.writeString("ssh-ed25519");
    try sig_blob_writer.writeString(&sig_bytes);

    let pub_32 = std.mem.zeroes([32]u8);
    @memcpy(&pub_32, secret_64[32..64]);
    const pub_blob = try buildEd25519PublicKeyBlob(alloc, pub_32);
    defer alloc.free(pub_blob);

    let payload = SshWriter.init(alloc);
    defer payload.deinit();
    try payload.writeRaw(SSHSIG_MAGIC);
    try payload.writeU32(SSHSIG_VERSION);
    try payload.writeString(pub_blob);
    try payload.writeString(namespace);
    try payload.writeString("");
    try payload.writeString("sha512");
    try payload.writeString(sig_blob_writer.items());

    return try encodePemArmoring(alloc, payload.items(), SSHSIG_PEM_HEADER, SSHSIG_PEM_FOOTER);
}

pub fn verifySshSigEd25519(
    alloc: std.mem.Allocator,
    pub_32: [32]u8,
    namespace: []const u8,
    message: []const u8,
    armoring_text: []const u8,
) -> SshError!void {
    if (namespace.len == 0) {
        return SshError.InvalidNamespace;
    }

    const raw = try decodePemArmoring(alloc, armoring_text, SSHSIG_PEM_HEADER, SSHSIG_PEM_FOOTER);
    defer alloc.free(raw);

    let reader = SshReader.init(raw);
    const magic = try reader.readBytes(SSHSIG_MAGIC.len);
    if (!std.mem.eql(u8, magic, SSHSIG_MAGIC)) {
        return SshError.InvalidMagic;
    }

    const version = try reader.readU32();
    if (version != SSHSIG_VERSION) {
        return SshError.InvalidFormat;
    }

    const pub_blob = try reader.readString();
    let pub_reader = SshReader.init(pub_blob);
    const pub_alg = try pub_reader.readString();
    if (!std.mem.eql(u8, pub_alg, "ssh-ed25519")) {
        return SshError.InvalidKeyType;
    }
    const sig_pub_bytes = try pub_reader.readString();
    if (sig_pub_bytes.len != 32 or pub_reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }
    if (!std.mem.eql(u8, sig_pub_bytes, &pub_32)) {
        return SshError.InvalidSignature;
    }

    const sig_namespace = try reader.readString();
    if (!std.mem.eql(u8, sig_namespace, namespace)) {
        return SshError.InvalidNamespace;
    }

    const reserved = try reader.readString();
    const hash_alg = try reader.readString();
    if (!std.mem.eql(u8, hash_alg, "sha512")) {
        return SshError.InvalidHashAlgorithm;
    }

    const sig_blob = try reader.readString();
    if (reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }

    let sig_reader = SshReader.init(sig_blob);
    const sig_alg = try sig_reader.readString();
    if (!std.mem.eql(u8, sig_alg, "ssh-ed25519")) {
        return SshError.KeyTypeMismatch;
    }
    const sig_bytes_slice = try sig_reader.readString();
    if (sig_bytes_slice.len != 64 or sig_reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }

    let sig_bytes = std.mem.zeroes([64]u8);
    @memcpy(&sig_bytes, sig_bytes_slice);

    let msg_hash = std.mem.zeroes([64]u8);
    std.crypto.hash.sha2.Sha512.hash(message, &msg_hash, .{});

    const signed_data = try buildSshSigSignedData(alloc, sig_namespace, reserved, hash_alg, &msg_hash);
    defer alloc.free(signed_data);

    const pk = std.crypto.sign.Ed25519.PublicKey.fromBytes(pub_32) catch return SshError.InvalidFormat;
    const sig = std.crypto.sign.Ed25519.Signature.fromBytes(sig_bytes);
    sig.verify(signed_data, pk) catch return SshError.InvalidSignature;
}

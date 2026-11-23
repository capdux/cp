// Reference-hardened against RustCrypto/SSH (MIT/Apache-2.0)
// Source: cp/lib/ssh_wire/openssh_key.cp (OpenSSH openssh-key-v1 Private Key Codec)
const std = @import("std");
import {
    SSH_MAX_WIRE_STRING_LEN,
    SshError,
    SshReader,
    SshWriter,
    simdSshCopyBytes,
    simdSshCtEqual,
} from "./sshWire.cp";
import { SshKeyType, buildEd25519PublicKeyBlob } from "./sshKeys.cp";

const OPENSSH_KEY_MAGIC = "openssh-key-v1\x00";
const PEM_HEADER = "-----BEGIN OPENSSH PRIVATE KEY-----";
const PEM_FOOTER = "-----END OPENSSH PRIVATE KEY-----";
const OPENSSH_NONE_CIPHER_BLOCK_SIZE: usize = 8;

pub struct OpenSshPrivateKey {
    key_type: SshKeyType = .Ed25519;
    ed25519_pub: [32]u8;
    ed25519_secret: [64]u8;
    comment: []u8;

    fn deinit(*_, alloc: std.mem.Allocator) {
        std.crypto.secureZero(u8, &_.ed25519_secret);
        alloc.free(_.comment);
    }
}

pub fn decodePemArmoring(alloc: std.mem.Allocator, pem_text: []const u8, header: []const u8, footer: []const u8) -> SshError![]u8 {
    const start_idx = std.mem.indexOf(u8, pem_text, header) orelse return SshError.InvalidFormat;
    const after_header = pem_text[start_idx + header.len ..];
    const end_idx = std.mem.indexOf(u8, after_header, footer) orelse return SshError.InvalidFormat;
    const body = after_header[0..end_idx];

    let b64_clean = std.ArrayList(u8).empty;
    defer {
        std.crypto.secureZero(u8, b64_clean.items);
        b64_clean.deinit(alloc);
    }
    for (ch of body) {
        if (ch != ' ' and ch != '\t' and ch != '\r' and ch != '\n') {
            if (b64_clean.items.len >= SSH_MAX_WIRE_STRING_LEN) {
                return SshError.PayloadTooLarge;
            }
            b64_clean.append(alloc, ch) catch return SshError.OutOfMemory;
        }
    }
    if (b64_clean.items.len == 0) {
        return SshError.InvalidFormat;
    }

    const dec_len = std.base64.standard.Decoder.calcSizeForSlice(b64_clean.items) catch return SshError.InvalidBase64;
    if (dec_len > SSH_MAX_WIRE_STRING_LEN) {
        return SshError.PayloadTooLarge;
    }
    const out = alloc.alloc(u8, dec_len) catch return SshError.OutOfMemory;
    errdefer {
        std.crypto.secureZero(u8, out);
        alloc.free(out);
    }
    std.base64.standard.Decoder.decode(out, b64_clean.items) catch return SshError.InvalidBase64;
    return out;
}

pub fn encodePemArmoring(alloc: std.mem.Allocator, payload: []const u8, header: []const u8, footer: []const u8) -> SshError![]u8 {
    const b64_len = std.base64.standard.Encoder.calcSize(payload.len);
    const b64_buf = alloc.alloc(u8, b64_len) catch return SshError.OutOfMemory;
    defer {
        std.crypto.secureZero(u8, b64_buf);
        alloc.free(b64_buf);
    }
    const b64 = std.base64.standard.Encoder.encode(b64_buf, payload);

    let out = std.ArrayList(u8).empty;
    errdefer {
        std.crypto.secureZero(u8, out.items);
        out.deinit(alloc);
    }

    out.appendSlice(alloc, header) catch return SshError.OutOfMemory;
    out.append(alloc, '\n') catch return SshError.OutOfMemory;

    let pos: usize = 0;
    while (pos < b64.len) {
        const end = @min(pos + 70, b64.len);
        out.appendSlice(alloc, b64[pos..end]) catch return SshError.OutOfMemory;
        out.append(alloc, '\n') catch return SshError.OutOfMemory;
        pos = end;
    }

    out.appendSlice(alloc, footer) catch return SshError.OutOfMemory;
    out.append(alloc, '\n') catch return SshError.OutOfMemory;
    return out.toOwnedSlice(alloc) catch return SshError.OutOfMemory;
}

pub fn parseOpenSshPrivateKey(alloc: std.mem.Allocator, pem_text: []const u8) -> SshError!OpenSshPrivateKey {
    const raw = try decodePemArmoring(alloc, pem_text, PEM_HEADER, PEM_FOOTER);
    defer {
        std.crypto.secureZero(u8, raw);
        alloc.free(raw);
    }

    let reader = SshReader.init(raw);
    const magic = try reader.readBytes(OPENSSH_KEY_MAGIC.len);
    if (!simdSshCtEqual(magic, OPENSSH_KEY_MAGIC)) {
        return SshError.InvalidMagic;
    }

    const ciphername = try reader.readString();
    if (!std.mem.eql(u8, ciphername, "none")) {
        return SshError.UnsupportedCipher;
    }
    const kdfname = try reader.readString();
    if (!std.mem.eql(u8, kdfname, "none")) {
        return SshError.UnsupportedKdf;
    }
    const kdfoptions = try reader.readString();
    if (kdfoptions.len != 0) {
        return SshError.UnsupportedKdf;
    }

    const num_keys = try reader.readU32();
    if (num_keys != 1) {
        return SshError.InvalidFormat;
    }

    const pub_blob = try reader.readString();
    let pub_reader = SshReader.init(pub_blob);
    const pub_alg = try pub_reader.readString();
    if (!std.mem.eql(u8, pub_alg, "ssh-ed25519")) {
        return SshError.InvalidKeyType;
    }
    const pub_bytes_outer = try pub_reader.readString();
    if (pub_bytes_outer.len != 32 or pub_reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }

    const priv_section = try reader.readString();
    if (reader.remaining() != 0) {
        return SshError.InvalidFormat;
    }

    let priv_reader = SshReader.init(priv_section);
    const checkint1 = try priv_reader.readU32();
    const checkint2 = try priv_reader.readU32();
    if (checkint1 != checkint2) {
        return SshError.CheckintMismatch;
    }

    const priv_alg = try priv_reader.readString();
    if (!std.mem.eql(u8, priv_alg, "ssh-ed25519")) {
        return SshError.KeyTypeMismatch;
    }

    const pub_bytes_inner = try priv_reader.readString();
    if (pub_bytes_inner.len != 32 or !simdSshCtEqual(pub_bytes_inner, pub_bytes_outer)) {
        return SshError.InvalidFormat;
    }

    const priv_bytes = try priv_reader.readString();
    if (priv_bytes.len != 64) {
        return SshError.InvalidFormat;
    }
    // OpenSSH PROTOCOL.key: Ed25519 64-byte private key is seed[0..32] || pub[32..64].
    if (!simdSshCtEqual(priv_bytes[32..64], pub_bytes_inner)) {
        return SshError.InvalidFormat;
    }

    const comment_slice = try priv_reader.readString();

    // OpenSSH PROTOCOL.key padding: for cipher "none" (block size 8),
    // remaining padding bytes must be strictly 0..7 bytes equal to 1, 2, ..., k.
    const rem = priv_reader.remaining();
    if (rem >= OPENSSH_NONE_CIPHER_BLOCK_SIZE) {
        return SshError.InvalidFormat;
    }
    if (rem > 0) {
        const pad = try priv_reader.readBytes(rem);
        let i: usize = 0;
        while (i < pad.len) {
            if (pad[i] != @as(u8, @intCast(i + 1))) {
                return SshError.InvalidFormat;
            }
            i += 1;
        }
    }

    let ed25519_pub = std.mem.zeroes([32]u8);
    let ed25519_secret = std.mem.zeroes([64]u8);
    errdefer std.crypto.secureZero(u8, &ed25519_secret);
    simdSshCopyBytes(&ed25519_pub, pub_bytes_inner);
    simdSshCopyBytes(&ed25519_secret, priv_bytes);

    const comment_owned = alloc.dupe(u8, comment_slice) catch return SshError.OutOfMemory;
    return OpenSshPrivateKey{
        key_type: .Ed25519,
        ed25519_pub: ed25519_pub,
        ed25519_secret: ed25519_secret,
        comment: comment_owned,
    };
}

pub fn encodeOpenSshPrivateKeyEd25519(
    alloc: std.mem.Allocator,
    secret_64: [64]u8,
    comment: []const u8,
    checkint: u32,
) -> SshError![]u8 {
    let pub_32 = std.mem.zeroes([32]u8);
    simdSshCopyBytes(&pub_32, secret_64[32..64]);

    const pub_blob = try buildEd25519PublicKeyBlob(alloc, pub_32);
    defer alloc.free(pub_blob);

    let priv_writer = SshWriter.init(alloc);
    defer {
        std.crypto.secureZero(u8, priv_writer.buf.items);
        priv_writer.deinit();
    }
    try priv_writer.writeU32(checkint);
    try priv_writer.writeU32(checkint);
    try priv_writer.writeString("ssh-ed25519");
    try priv_writer.writeString(&pub_32);
    try priv_writer.writeString(&secret_64);
    try priv_writer.writeString(comment);

    const mod_len = priv_writer.items().len % OPENSSH_NONE_CIPHER_BLOCK_SIZE;
    if (mod_len != 0) {
        const pad_len = OPENSSH_NONE_CIPHER_BLOCK_SIZE - mod_len;
        let i: usize = 1;
        while (i <= pad_len) {
            try priv_writer.writeByte(@as(u8, @intCast(i)));
            i += 1;
        }
    }

    let outer = SshWriter.init(alloc);
    defer {
        std.crypto.secureZero(u8, outer.buf.items);
        outer.deinit();
    }
    try outer.writeRaw(OPENSSH_KEY_MAGIC);
    try outer.writeString("none");
    try outer.writeString("none");
    try outer.writeString("");
    try outer.writeU32(1);
    try outer.writeString(pub_blob);
    try outer.writeString(priv_writer.items());

    return try encodePemArmoring(alloc, outer.items(), PEM_HEADER, PEM_FOOTER);
}

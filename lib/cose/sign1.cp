// Source: cp/lib/cose/sign1.cp (RFC 9052 §4.2 COSE_Sign1 EdDSA & ES256)
const std = @import("std");
import { CoseError, CoseAlgorithm, CoseHeader, COSE_TAG_SIGN1 } from "./types.cp";
import { CborWriter, CborReader } from "./cbor.cp";

const Ed25519 = std.crypto.sign.Ed25519;
const EcdsaP256 = std.crypto.sign.ecdsa.EcdsaP256Sha256;

pub const CoseSign1 = struct {
    protected_bytes: []const u8,
    unprotected: CoseHeader,
    payload: []const u8,
    signature: []const u8,

    pub fn deinit(self: *CoseSign1, alloc: std.mem.Allocator) void {
        alloc.free(self.protected_bytes);
        self.unprotected.deinit(alloc);
        alloc.free(self.payload);
        alloc.free(self.signature);
    }
};

pub fn encodeHeaderMap(alloc: std.mem.Allocator, header: CoseHeader): CoseError![]u8 {
    let count: usize = 0;
    if (header.alg != null) count += 1;
    if (header.content_type != null) count += 1;
    if (header.kid != null) count += 1;

    let w = CborWriter.init();
    defer w.deinit(alloc);

    try w.writeMapHeader(alloc, count);
    if (alg := header.alg) {
        try w.writeInt(alloc, 1);
        try w.writeInt(alloc, alg);
    }
    if (ct := header.content_type) {
        try w.writeInt(alloc, 3);
        try w.writeTstr(alloc, ct);
    }
    if (kid := header.kid) {
        try w.writeInt(alloc, 4);
        try w.writeBstr(alloc, kid);
    }
    return try w.toOwnedSlice(alloc);
}

pub fn encodeProtectedHeader(alloc: std.mem.Allocator, header: CoseHeader): CoseError![]u8 {
    if (header.alg == null and header.kid == null and header.content_type == null) {
        return alloc.dupe(u8, "") catch return CoseError.OutOfMemory;
    }
    return try encodeHeaderMap(alloc, header);
}

pub fn decodeHeaderMap(alloc: std.mem.Allocator, r: *CborReader): CoseError!CoseHeader {
    const map_len = try r.readMapHeader();
    let header = CoseHeader{};
    errdefer header.deinit(alloc);

    let i: usize = 0;
    while (i < map_len) : (i += 1) {
        const label = try r.readInt();
        switch (label) {
            1 => {
                const v = try r.readInt();
                if (v < std.math.minInt(i32) or v > std.math.maxInt(i32)) return CoseError.InvalidAlgorithm;
                header.alg = @as(i32, @intCast(v));
            },
            3 => {
                const s = try r.readTstr();
                header.content_type = alloc.dupe(u8, s) catch return CoseError.OutOfMemory;
            },
            4 => {
                const b = try r.readBstr();
                header.kid = alloc.dupe(u8, b) catch return CoseError.OutOfMemory;
            },
            else => {
                try r.skipValue();
            },
        }
    }
    return header;
}

pub fn decodeProtectedHeader(alloc: std.mem.Allocator, protected_bytes: []const u8): CoseError!CoseHeader {
    if (protected_bytes.len == 0) {
        return CoseHeader{};
    }
    let r = CborReader.init(protected_bytes);
    return try decodeHeaderMap(alloc, &r);
}

pub fn encodeSigStructure(
    alloc: std.mem.Allocator,
    protected_bytes: []const u8,
    external_aad: []const u8,
    payload: []const u8,
): CoseError![]u8 {
    let w = CborWriter.init();
    defer w.deinit(alloc);

    try w.writeArrayHeader(alloc, 4);
    try w.writeTstr(alloc, "Signature1");
    try w.writeBstr(alloc, protected_bytes);
    try w.writeBstr(alloc, external_aad);
    try w.writeBstr(alloc, payload);

    return try w.toOwnedSlice(alloc);
}

pub fn encodeCoseSign1(alloc: std.mem.Allocator, sign1: CoseSign1, with_tag: bool): CoseError![]u8 {
    let w = CborWriter.init();
    defer w.deinit(alloc);

    if (with_tag) {
        try w.writeTag(alloc, COSE_TAG_SIGN1);
    }
    try w.writeArrayHeader(alloc, 4);
    try w.writeBstr(alloc, sign1.protected_bytes);

    const unprot_bytes = try encodeHeaderMap(alloc, sign1.unprotected);
    defer alloc.free(unprot_bytes);
    w.buf.appendSlice(alloc, unprot_bytes) catch return CoseError.OutOfMemory;

    try w.writeBstr(alloc, sign1.payload);
    try w.writeBstr(alloc, sign1.signature);

    return try w.toOwnedSlice(alloc);
}

pub fn decodeCoseSign1(alloc: std.mem.Allocator, cbor_bytes: []const u8): CoseError!CoseSign1 {
    let r = CborReader.init(cbor_bytes);
    if (tag := try r.readTag()) {
        if (tag != COSE_TAG_SIGN1) return CoseError.UnexpectedTag;
    }

    const arr_len = try r.readArrayHeader();
    if (arr_len != 4) return CoseError.InvalidCbor;

    const prot_slice = try r.readBstr();
    const protected_bytes = alloc.dupe(u8, prot_slice) catch return CoseError.OutOfMemory;
    errdefer alloc.free(protected_bytes);

    let unprotected = try decodeHeaderMap(alloc, &r);
    errdefer unprotected.deinit(alloc);

    let payload_slice: []const u8 = "";
    if (!(try r.isNull())) {
        payload_slice = try r.readBstr();
    }
    const payload = alloc.dupe(u8, payload_slice) catch return CoseError.OutOfMemory;
    errdefer alloc.free(payload);

    const sig_slice = try r.readBstr();
    const signature = alloc.dupe(u8, sig_slice) catch return CoseError.OutOfMemory;

    return CoseSign1{
        .protected_bytes = protected_bytes,
        .unprotected = unprotected,
        .payload = payload,
        .signature = signature,
    };
}

pub fn sign1Ed25519(
    alloc: std.mem.Allocator,
    secret_seed: [32]u8,
    payload: []const u8,
    external_aad: []const u8,
    kid: []const u8?,
    with_tag: bool,
): CoseError![]u8 {
    const prot_header = CoseHeader{ .alg = CoseAlgorithm.EdDSA.toInt(), .kid = kid };
    const protected_bytes = try encodeProtectedHeader(alloc, prot_header);
    defer alloc.free(protected_bytes);

    const to_be_signed = try encodeSigStructure(alloc, protected_bytes, external_aad, payload);
    defer alloc.free(to_be_signed);

    const kp = Ed25519.KeyPair.generateDeterministic(secret_seed) catch return CoseError.InvalidKeyLength;
    const sig = kp.sign(to_be_signed, null) catch return CoseError.InvalidSignature;
    const sig_bytes = sig.toBytes();

    const sign1 = CoseSign1{
        .protected_bytes = protected_bytes,
        .unprotected = CoseHeader{},
        .payload = payload,
        .signature = &sig_bytes,
    };
    return try encodeCoseSign1(alloc, sign1, with_tag);
}

pub fn verify1Ed25519(
    alloc: std.mem.Allocator,
    public_key: [32]u8,
    cbor_bytes: []const u8,
    external_aad: []const u8,
): CoseError![]u8 {
    let sign1 = try decodeCoseSign1(alloc, cbor_bytes);
    defer sign1.deinit(alloc);

    let prot = try decodeProtectedHeader(alloc, sign1.protected_bytes);
    defer prot.deinit(alloc);

    const alg = prot.alg orelse sign1.unprotected.alg orelse return CoseError.MissingField;
    if (alg != CoseAlgorithm.EdDSA.toInt()) return CoseError.InvalidAlgorithm;
    if (sign1.signature.len != 64) return CoseError.InvalidSignature;

    const to_be_signed = try encodeSigStructure(alloc, sign1.protected_bytes, external_aad, sign1.payload);
    defer alloc.free(to_be_signed);

    const pk = Ed25519.PublicKey.fromBytes(public_key) catch return CoseError.InvalidKeyLength;
    let sig_arr: [64]u8 = undefined;
    @memcpy(&sig_arr, sign1.signature[0..64]);
    const sig = Ed25519.Signature.fromBytes(sig_arr);

    sig.verify(to_be_signed, pk) catch return CoseError.InvalidSignature;

    return alloc.dupe(u8, sign1.payload) catch return CoseError.OutOfMemory;
}

pub fn sign1Es256(
    alloc: std.mem.Allocator,
    secret_key: [32]u8,
    payload: []const u8,
    external_aad: []const u8,
    kid: []const u8?,
    with_tag: bool,
): CoseError![]u8 {
    const prot_header = CoseHeader{ .alg = CoseAlgorithm.ES256.toInt(), .kid = kid };
    const protected_bytes = try encodeProtectedHeader(alloc, prot_header);
    defer alloc.free(protected_bytes);

    const to_be_signed = try encodeSigStructure(alloc, protected_bytes, external_aad, payload);
    defer alloc.free(to_be_signed);

    const sk = EcdsaP256.SecretKey.fromBytes(secret_key) catch return CoseError.InvalidKeyLength;
    const kp = EcdsaP256.KeyPair.fromSecretKey(sk) catch return CoseError.InvalidKeyLength;
    const sig = kp.sign(to_be_signed, null) catch return CoseError.InvalidSignature;
    const sig_bytes = sig.toBytes();

    const sign1 = CoseSign1{
        .protected_bytes = protected_bytes,
        .unprotected = CoseHeader{},
        .payload = payload,
        .signature = &sig_bytes,
    };
    return try encodeCoseSign1(alloc, sign1, with_tag);
}

pub fn verify1Es256(
    alloc: std.mem.Allocator,
    x: [32]u8,
    y: [32]u8,
    cbor_bytes: []const u8,
    external_aad: []const u8,
): CoseError![]u8 {
    let sign1 = try decodeCoseSign1(alloc, cbor_bytes);
    defer sign1.deinit(alloc);

    let prot = try decodeProtectedHeader(alloc, sign1.protected_bytes);
    defer prot.deinit(alloc);

    const alg = prot.alg orelse sign1.unprotected.alg orelse return CoseError.MissingField;
    if (alg != CoseAlgorithm.ES256.toInt()) return CoseError.InvalidAlgorithm;
    if (sign1.signature.len != 64) return CoseError.InvalidSignature;

    const to_be_signed = try encodeSigStructure(alloc, sign1.protected_bytes, external_aad, sign1.payload);
    defer alloc.free(to_be_signed);

    let sec1: [65]u8 = undefined;
    sec1[0] = 0x04;
    @memcpy(sec1[1..33], &x);
    @memcpy(sec1[33..65], &y);

    const pk = EcdsaP256.PublicKey.fromSec1(&sec1) catch return CoseError.InvalidKeyLength;
    let sig_arr: [64]u8 = undefined;
    @memcpy(&sig_arr, sign1.signature[0..64]);
    const sig = EcdsaP256.Signature.fromBytes(sig_arr);

    sig.verify(to_be_signed, pk) catch return CoseError.InvalidSignature;

    return alloc.dupe(u8, sign1.payload) catch return CoseError.OutOfMemory;
}

// Source: cp/lib/cose/mac0.cp (RFC 9052 §6.2 COSE_Mac0 HS256)
const std = @import("std");
import { CoseError, CoseAlgorithm, CoseHeader, COSE_TAG_MAC0 } from "./types.cp";
import { CborWriter, CborReader } from "./cbor.cp";
import { encodeHeaderMap, encodeProtectedHeader, decodeHeaderMap, decodeProtectedHeader } from "./sign1.cp";

const HmacSha256 = std.crypto.auth.hmac.sha2.HmacSha256;

pub const CoseMac0 = struct {
    protected_bytes: []const u8,
    unprotected: CoseHeader,
    payload: []const u8,
    tag: []const u8,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.protected_bytes);
        _.unprotected.deinit(alloc);
        alloc.free(_.payload);
        alloc.free(_.tag);
    }
};

pub fn encodeMacStructure(
    alloc: std.mem.Allocator,
    protected_bytes: []const u8,
    external_aad: []const u8,
    payload: []const u8,
): CoseError![]u8 {
    let w = CborWriter.init();
    defer w.deinit(alloc);

    try w.writeArrayHeader(alloc, 4);
    try w.writeTstr(alloc, "MAC0");
    try w.writeBstr(alloc, protected_bytes);
    try w.writeBstr(alloc, external_aad);
    try w.writeBstr(alloc, payload);

    return try w.toOwnedSlice(alloc);
}

pub fn encodeCoseMac0(alloc: std.mem.Allocator, mac0: CoseMac0, with_tag: bool): CoseError![]u8 {
    let w = CborWriter.init();
    defer w.deinit(alloc);

    if (with_tag) {
        try w.writeTag(alloc, COSE_TAG_MAC0);
    }
    try w.writeArrayHeader(alloc, 4);
    try w.writeBstr(alloc, mac0.protected_bytes);

    const unprot_bytes = try encodeHeaderMap(alloc, mac0.unprotected);
    defer alloc.free(unprot_bytes);
    w.buf.appendSlice(alloc, unprot_bytes) catch return CoseError.OutOfMemory;

    try w.writeBstr(alloc, mac0.payload);
    try w.writeBstr(alloc, mac0.tag);

    return try w.toOwnedSlice(alloc);
}

pub fn decodeCoseMac0(alloc: std.mem.Allocator, cbor_bytes: []const u8): CoseError!CoseMac0 {
    let r = CborReader.init(cbor_bytes);
    if (tag := try r.readTag()) {
        if (tag != COSE_TAG_MAC0) return CoseError.UnexpectedTag;
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

    const tag_slice = try r.readBstr();
    const tag = alloc.dupe(u8, tag_slice) catch return CoseError.OutOfMemory;

    return CoseMac0{
        .protected_bytes = protected_bytes,
        .unprotected = unprotected,
        .payload = payload,
        .tag = tag,
    };
}

pub fn mac0Hs256(
    alloc: std.mem.Allocator,
    key: []const u8,
    payload: []const u8,
    external_aad: []const u8,
    kid: []const u8?,
    with_tag: bool,
): CoseError![]u8 {
    if (key.len == 0) return CoseError.InvalidKeyLength;

    const prot_header = CoseHeader{ .alg = CoseAlgorithm.HS256.toInt(), .kid = kid };
    const protected_bytes = try encodeProtectedHeader(alloc, prot_header);
    defer alloc.free(protected_bytes);

    const to_be_maced = try encodeMacStructure(alloc, protected_bytes, external_aad, payload);
    defer alloc.free(to_be_maced);

    let mac: [HmacSha256.mac_length]u8 = undefined;
    HmacSha256.create(&mac, to_be_maced, key);

    const mac0 = CoseMac0{
        .protected_bytes = protected_bytes,
        .unprotected = CoseHeader{},
        .payload = payload,
        .tag = &mac,
    };
    return try encodeCoseMac0(alloc, mac0, with_tag);
}

pub fn verifyMac0Hs256(
    alloc: std.mem.Allocator,
    key: []const u8,
    cbor_bytes: []const u8,
    external_aad: []const u8,
): CoseError![]u8 {
    if (key.len == 0) return CoseError.InvalidKeyLength;

    let mac0 = try decodeCoseMac0(alloc, cbor_bytes);
    defer mac0.deinit(alloc);

    let prot = try decodeProtectedHeader(alloc, mac0.protected_bytes);
    defer prot.deinit(alloc);

    const alg = prot.alg orelse mac0.unprotected.alg orelse return CoseError.MissingField;
    if (alg != CoseAlgorithm.HS256.toInt()) return CoseError.InvalidAlgorithm;
    if (mac0.tag.len != HmacSha256.mac_length) return CoseError.InvalidMac;

    const to_be_maced = try encodeMacStructure(alloc, mac0.protected_bytes, external_aad, mac0.payload);
    defer alloc.free(to_be_maced);

    let expected: [HmacSha256.mac_length]u8 = undefined;
    HmacSha256.create(&expected, to_be_maced, key);

    let actual: [HmacSha256.mac_length]u8 = undefined;
    @memcpy(&actual, mac0.tag[0..HmacSha256.mac_length]);

    if (!std.crypto.timing_safe.eql([HmacSha256.mac_length]u8, expected, actual)) {
        return CoseError.InvalidMac;
    }

    return alloc.dupe(u8, mac0.payload) catch return CoseError.OutOfMemory;
}

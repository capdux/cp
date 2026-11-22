// Source: cp/cap/test/test_base58_check_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { Base58Error, encode, decode, encodeCheck, decodeCheck } from "cap:finance";

test "base58_check: raw Base58 encode/decode with leading zero bytes" {
    const alloc = std.testing.allocator;
    const raw: [5]u8 = [ 0x00, 0x00, 0x01, 0x02, 0x03 ];
    const enc = try encode(alloc, &raw);
    defer alloc.free(enc);
    try expect_eq(enc[0] == '1' and enc[1] == '1', true);

    const out = try decode(alloc, enc);
    defer alloc.free(out);
    try expect_eq(std.mem.eql(u8, out, &raw), true);
}

test "base58_check: Bitcoin Genesis P2PKH address official vector" {
    const alloc = std.testing.allocator;
    // Satoshi's Genesis block P2PKH hash160 -> 1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa
    const hash160: [20]u8 = [
        0x62, 0xe9, 0x07, 0xb1, 0x5c, 0xbf, 0x27, 0xd5,
        0x42, 0x53, 0x99, 0xeb, 0xf6, 0xf0, 0xfb, 0x50,
        0xeb, 0xb8, 0x8f, 0x18,
    ];
    const addr = try encodeCheck(alloc, 0x00, &hash160);
    defer alloc.free(addr);
    try expect_eq(std.mem.eql(u8, addr, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"), true);

    const decoded = try decodeCheck(alloc, addr);
    defer decoded.deinit(alloc);
    try expect_eq(decoded.version, 0x00);
    try expect_eq(std.mem.eql(u8, decoded.payload, &hash160), true);
}

test "base58_check: rejects corrupted checksum & invalid characters" {
    const alloc = std.testing.allocator;
    // Flip last char 'a' -> 'b'
    try expect_eq(decodeCheck(alloc, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNb") == Base58Error.InvalidChecksum, true);
    // '0', 'O', 'I', 'l' are forbidden in Base58
    try expect_eq(decode(alloc, "0OIl") == Base58Error.InvalidCharacter, true);
    try expect_eq(decodeCheck(alloc, "11") == Base58Error.PayloadTooShort, true);
}

test "base58_check: empty input and versioned WIF-style payload" {
    const alloc = std.testing.allocator;
    const empty_enc = try encode(alloc, "");
    defer alloc.free(empty_enc);
    try expect_eq(empty_enc.len, 0);

    const wif = try encodeCheck(alloc, 0x80, "secret-key-32-bytes-placeholder!");
    defer alloc.free(wif);
    try expect_eq(wif[0] == '5' or wif[0] == 'K' or wif[0] == 'L', true);
    const dec_wif = try decodeCheck(alloc, wif);
    defer dec_wif.deinit(alloc);
    try expect_eq(dec_wif.version, 0x80);
}

test "base58_check: cap.base58_check dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const enc = try cap.finance.encodeCheck(alloc, 0x05, "p2sh-payload-20bytes");
    defer alloc.free(enc);
    const dec_res = try cap.finance.decodeCheck(alloc, enc);
    defer dec_res.deinit(alloc);
    try expect_eq(dec_res.version, 0x05);
    try expect_eq(std.mem.eql(u8, dec_res.payload, "p2sh-payload-20bytes"), true);
}

// Source: cp/cap/test/test_base58_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:base58 & cap.base58

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Base58Error,
    Base58,
    Bech32Encoding,
    Bech32Decoded,
    Bech32,
} from "cap:base58";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn emptyBech32Decoded(): Bech32Decoded {
    return Bech32Decoded{};
}

fn runTest1(): !void {
    const alloc = std.testing.allocator;

    // Empty input
    const empty_enc = Base58.encode(alloc, "")?;
    defer alloc.free(empty_enc);
    expect_eq(empty_enc.len, 0 as usize);

    const empty_dec = Base58.decode(alloc, "")?;
    defer alloc.free(empty_dec);
    expect_eq(empty_dec.len, 0 as usize);

    // All-zero leading bytes -> "1111"
    const zeros: [4]u8 = [0x00, 0x00, 0x00, 0x00];
    const zeros_enc = Base58.encode(alloc, &zeros)?;
    defer alloc.free(zeros_enc);
    expect_eq(std.mem.eql(u8, zeros_enc, "1111"), true);

    const zeros_dec = Base58.decode(alloc, "1111")?;
    defer alloc.free(zeros_dec);
    expect_eq(std.mem.eql(u8, zeros_dec, &zeros), true);

    // Leading zeros + payload
    const mixed: [6]u8 = [0x00, 0x00, 0x28, 0x7f, 0xb4, 0xcd];
    const mixed_enc = Base58.encode(alloc, &mixed)?;
    defer alloc.free(mixed_enc);
    expect_eq(std.mem.eql(u8, mixed_enc, "11233QC4"), true);

    const mixed_dec = Base58.decode(alloc, "11233QC4")?;
    defer alloc.free(mixed_dec);
    expect_eq(std.mem.eql(u8, mixed_dec, &mixed), true);

    // Classic text vector
    const hello_enc = Base58.encode(alloc, "Hello World!")?;
    defer alloc.free(hello_enc);
    expect_eq(std.mem.eql(u8, hello_enc, "2NEpo7TZRRrLZSi2U"), true);

    const hello_dec = Base58.decode(alloc, "2NEpo7TZRRrLZSi2U")?;
    defer alloc.free(hello_dec);
    expect_eq(std.mem.eql(u8, hello_dec, "Hello World!"), true);

    // Invalid Base58 characters: '0', 'O', 'I', 'l'
    let bad0 = false;
    * = Base58.decode(alloc, "123045") catch e => err0: {
        bad0 = (e == Base58Error.InvalidCharacter);
        break :err0 emptyMutSlice();
    };
    expect_eq(bad0, true);

    let badO = false;
    * = Base58.decode(alloc, "123O45") catch e => errO: {
        badO = (e == Base58Error.InvalidCharacter);
        break :errO emptyMutSlice();
    };
    expect_eq(badO, true);

    let badI = false;
    * = Base58.decode(alloc, "123I45") catch e => errI: {
        badI = (e == Base58Error.InvalidCharacter);
        break :errI emptyMutSlice();
    };
    expect_eq(badI, true);

    let badl = false;
    * = Base58.decode(alloc, "123l45") catch e => errl: {
        badl = (e == Base58Error.InvalidCharacter);
        break :errl emptyMutSlice();
    };
    expect_eq(badl, true);
}

fn runTest2(): !void {
    const alloc = std.testing.allocator;

    // Classic Bitcoin Genesis P2PKH (version 0x00 + 20-byte hash160)
    const p2pkh: [21]u8 = [
        0x00,
        0x62, 0xe9, 0x07, 0xb1, 0x5c, 0xbf, 0x27, 0xd5,
        0x42, 0x53, 0x99, 0xeb, 0xf6, 0xf0, 0xfb, 0x50,
        0xeb, 0xb8, 0x8f, 0x18,
    ];
    const encoded = Base58.encodeCheck(alloc, &p2pkh)?;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"), true);

    const decoded = Base58.decodeCheck(alloc, encoded)?;
    defer alloc.free(decoded);
    expect_eq(std.mem.eql(u8, decoded, &p2pkh), true);

    // Single-character tampering -> InvalidChecksum
    let bad_chk = false;
    * = Base58.decodeCheck(alloc, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNb") catch e => errChk: {
        bad_chk = (e == Base58Error.InvalidChecksum);
        break :errChk emptyMutSlice();
    };
    expect_eq(bad_chk, true);

    // Too short for 4-byte checksum -> InvalidLength
    let bad_len = false;
    * = Base58.decodeCheck(alloc, "123") catch e => errLen: {
        bad_len = (e == Base58Error.InvalidLength);
        break :errLen emptyMutSlice();
    };
    expect_eq(bad_len, true);
}

fn runTest3(): !void {
    const alloc = std.testing.allocator;

    // BIP-173 empty data vector (uppercase)
    let dec_empty = Bech32.decode(alloc, "A12UEL5L")?;
    defer dec_empty.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_empty.hrp, "a"), true);
    expect_eq(dec_empty.data.len, 0 as usize);
    expect_eq(dec_empty.encoding == Bech32Encoding.Bech32, true);

    // BIP-173 SegWit v0 uppercase vector
    const segwit_v0 = "BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4";
    let dec_v0 = Bech32.decode(alloc, segwit_v0)?;
    defer dec_v0.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_v0.hrp, "bc"), true);
    expect_eq(dec_v0.encoding == Bech32Encoding.Bech32, true);
    expect_eq(dec_v0.data[0], 0 as u8);

    const prog_v0 = Bech32.convertBits(alloc, dec_v0.data[1..], 5, 8, false)?;
    defer alloc.free(prog_v0);
    const expected_prog: [20]u8 = [
        0x75, 0x1e, 0x76, 0xe8, 0x19, 0x91, 0x96, 0xd4,
        0x54, 0x94, 0x1c, 0x45, 0xd1, 0xb3, 0xa3, 0x23,
        0xf1, 0x43, 0x3b, 0xd6,
    ];
    expect_eq(std.mem.eql(u8, prog_v0, &expected_prog), true);

    // Re-encode via convertBits 8->5 -> Bech32.encode
    const words5 = Bech32.convertBits(alloc, prog_v0, 8, 5, true)?;
    defer alloc.free(words5);
    let full_data5 = alloc.alloc(u8, 1 + words5.len)?;
    defer alloc.free(full_data5);
    full_data5[0] = 0;
    @memcpy(full_data5[1..], words5);

    const reencoded = Bech32.encode(alloc, "bc", full_data5, Bech32Encoding.Bech32)?;
    defer alloc.free(reencoded);
    expect_eq(std.mem.eql(u8, reencoded, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4"), true);

    // BIP-350 Bech32m vectors
    let dec_350_empty = Bech32.decode(alloc, "a1lqfn3a")?;
    defer dec_350_empty.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_350_empty.hrp, "a"), true);
    expect_eq(dec_350_empty.data.len, 0 as usize);
    expect_eq(dec_350_empty.encoding == Bech32Encoding.Bech32m, true);

    const taproot = "bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0";
    let dec_tr = Bech32.decode(alloc, taproot)?;
    defer dec_tr.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_tr.hrp, "bc"), true);
    expect_eq(dec_tr.encoding == Bech32Encoding.Bech32m, true);
    expect_eq(dec_tr.data[0], 1 as u8);
}

fn runTest4(): !void {
    const alloc = std.testing.allocator;
    const empty_u8: [0]u8 = [];

    // MixedCase on decode & encode
    let mixed_dec = false;
    * = Bech32.decode(alloc, "A12uel5l") catch e => errM1: {
        mixed_dec = (e == Base58Error.MixedCase);
        break :errM1 emptyBech32Decoded();
    };
    expect_eq(mixed_dec, true);

    let mixed_enc = false;
    * = Bech32.encode(alloc, "Bc", &empty_u8, Bech32Encoding.Bech32) catch e => errM2: {
        mixed_enc = (e == Base58Error.MixedCase);
        break :errM2 emptyMutSlice();
    };
    expect_eq(mixed_enc, true);

    // InvalidHrp: empty HRP or out-of-range ASCII
    let hrp1 = false;
    * = Bech32.decode(alloc, "1qqqqqqq") catch e => errH1: {
        hrp1 = (e == Base58Error.InvalidHrp);
        break :errH1 emptyBech32Decoded();
    };
    expect_eq(hrp1, true);

    let hrp2 = false;
    * = Bech32.encode(alloc, "", &empty_u8, Bech32Encoding.Bech32) catch e => errH2: {
        hrp2 = (e == Base58Error.InvalidHrp);
        break :errH2 emptyMutSlice();
    };
    expect_eq(hrp2, true);

    let hrp3 = false;
    * = Bech32.encode(alloc, "bc\x1f", &empty_u8, Bech32Encoding.Bech32) catch e => errH3: {
        hrp3 = (e == Base58Error.InvalidHrp);
        break :errH3 emptyMutSlice();
    };
    expect_eq(hrp3, true);

    // MissingSeparator
    let miss_sep = false;
    * = Bech32.decode(alloc, "abcdefgh") catch e => errSep: {
        miss_sep = (e == Base58Error.MissingSeparator);
        break :errSep emptyBech32Decoded();
    };
    expect_eq(miss_sep, true);

    // Single-char flip -> InvalidChecksum
    let bad_chk = false;
    * = Bech32.decode(alloc, "A12UEL5M") catch e => errB32Chk: {
        bad_chk = (e == Base58Error.InvalidChecksum);
        break :errB32Chk emptyBech32Decoded();
    };
    expect_eq(bad_chk, true);

    // InvalidPadding on convertBits(..., pad=false)
    const pad_case1: [1]u8 = [0x01];
    let bad_pad1 = false;
    * = Bech32.convertBits(alloc, &pad_case1, 5, 8, false) catch e => errP1: {
        bad_pad1 = (e == Base58Error.InvalidPadding);
        break :errP1 emptyMutSlice();
    };
    expect_eq(bad_pad1, true);

    const pad_case2: [2]u8 = [0x00, 0x01];
    let bad_pad2 = false;
    * = Bech32.convertBits(alloc, &pad_case2, 5, 8, false) catch e => errP2: {
        bad_pad2 = (e == Base58Error.InvalidPadding);
        break :errP2 emptyMutSlice();
    };
    expect_eq(bad_pad2, true);
}

fn runTest5(): !void {
    const alloc = std.testing.allocator;

    // Base58 & Base58Check via cap.base58
    const b58 = cap.base58.Base58.encode(alloc, "cap.base58")?;
    defer alloc.free(b58);
    const b58_dec = cap.base58.Base58.decode(alloc, b58)?;
    defer alloc.free(b58_dec);
    expect_eq(std.mem.eql(u8, b58_dec, "cap.base58"), true);

    const chk = cap.base58.Base58.encodeCheck(alloc, "payload")?;
    defer alloc.free(chk);
    const chk_dec = cap.base58.Base58.decodeCheck(alloc, chk)?;
    defer alloc.free(chk_dec);
    expect_eq(std.mem.eql(u8, chk_dec, "payload"), true);

    // Bech32m via cap.base58
    const data5 = cap.base58.Bech32.convertBits(alloc, "ok", 8, 5, true)?;
    defer alloc.free(data5);
    const b32 = cap.base58.Bech32.encode(alloc, "cap", data5, cap.base58.Bech32Encoding.Bech32m)?;
    defer alloc.free(b32);

    let decoded = cap.base58.Bech32.decode(alloc, b32)?;
    defer decoded.deinit(alloc);
    expect_eq(std.mem.eql(u8, decoded.hrp, "cap"), true);
    expect_eq(decoded.encoding == cap.base58.Bech32Encoding.Bech32m, true);

    const restored = cap.base58.Bech32.convertBits(alloc, decoded.data, 5, 8, false)?;
    defer alloc.free(restored);
    expect_eq(std.mem.eql(u8, restored, "ok"), true);
}

test("1. Base58 empty input, leading zero bytes, classic vectors, and invalid characters", () => {
    runTest1() catch unreachable;
});

test("2. Base58Check P2PKH roundtrip and single-char tampering InvalidChecksum", () => {
    runTest2() catch unreachable;
});

test("3. Bech32 and Bech32m BIP-173/350 vectors, convertBits 8->5->8, and uppercase input", () => {
    runTest3() catch unreachable;
});

test("4. Bech32 error handling: MixedCase, InvalidHrp, MissingSeparator, InvalidChecksum, InvalidPadding", () => {
    runTest4() catch unreachable;
});

test("5. import cap from \"cap\"; cap.base58 dynamic mount and std.testing.allocator zero leak", () => {
    runTest5() catch unreachable;
});

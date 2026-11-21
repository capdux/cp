// Source: cp/cap/test/test_bencode_production_deep.cp
// Deep Production Test Suite for cap:bencode
// 5 test cases:
//   1. BEP-3 round-trip (Int/Bytes/List/Dict), get/getBytes/getInt, equals, clone
//   2. Strict validation guards (i03e, i-0e, ie, UnorderedDictKeys, DuplicateDictKey, TrailingBytes, negative len, MaxDepthExceeded, MaxSizeExceeded)
//   3. Canonical dictionary sorting on encode
//   4. Zero-copy extractRawSection & computeInfoHashSha1 + formatInfoHashHex
//   5. Dynamic cap.bencode namespace mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BENCODE_MAX_DEPTH,
    BencodeError,
    BencodeDictEntry,
    BencodeValue,
    decode,
    decodeWithLimits,
    encode,
    extractRawSection,
    computeInfoHashSha1,
    formatInfoHashHex,
} from "cap:bencode";
import cap from "cap";

fn expectDecodeError(alloc: std.mem.Allocator, payload: []const u8, expected: BencodeError) {
    switch (decode(alloc, payload)) {
        val => {
            let mut_val = val;
            mut_val.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == expected, true);
        },
    }
}

test("bencode: BEP-3 round-trip, accessors, equals and clone", () => {
    const alloc = std.testing.allocator;

    // Integers: positive, zero, negative
    let v_pos = decode(alloc, "i42e")?;
    defer v_pos.deinit(alloc);
    expect_eq(v_pos.asInt().?, 42 as i64);

    let v_zero = decode(alloc, "i0e")?;
    defer v_zero.deinit(alloc);
    expect_eq(v_zero.asInt().?, 0 as i64);

    let v_neg = decode(alloc, "i-9876543210e")?;
    defer v_neg.deinit(alloc);
    expect_eq(v_neg.asInt().?, -9876543210 as i64);

    // Binary bytes containing \x00 and \xFF
    let v_bin = decode(alloc, "5:a\x00b\xFFc")?;
    defer v_bin.deinit(alloc);
    expect_eq(std.mem.eql(u8, v_bin.asBytes().?, "a\x00b\xFFc"), true);

    // Nested dict + list round-trip
    const payload = "d5:alphali-7e3:x\x00ye3:mid10:cp-bencode4:zetai100ee";
    let decoded = decode(alloc, payload)?;
    defer decoded.deinit(alloc);

    expect_eq(decoded.getInt("zeta").?, 100 as i64);
    expect_eq(std.mem.eql(u8, decoded.getBytes("mid").?, "cp-bencode"), true);
    expect_eq(decoded.getInt("missing") == null, true);
    expect_eq(decoded.getBytes("zeta") == null, true);

    const alpha_list = decoded.get("alpha").?.asList().?;
    expect_eq(alpha_list.len, 2 as usize);
    expect_eq(alpha_list[0].asInt().?, -7 as i64);
    expect_eq(std.mem.eql(u8, alpha_list[1].asBytes().?, "x\x00y"), true);

    // clone & equals
    let cloned = decoded.clone(alloc)?;
    defer cloned.deinit(alloc);
    expect_eq(decoded.equals(cloned), true);

    const reencoded = encode(alloc, cloned)?;
    defer alloc.free(reencoded);
    expect_eq(std.mem.eql(u8, reencoded, payload), true);
});

test("bencode: strict BEP-3 error guards and resource limits", () => {
    const alloc = std.testing.allocator;

    // Leading zeros
    expectDecodeError(alloc, "i03e", BencodeError.LeadingZero);
    expectDecodeError(alloc, "i-03e", BencodeError.LeadingZero);
    expectDecodeError(alloc, "02:ab", BencodeError.LeadingZero);

    // Negative zero & empty integer
    expectDecodeError(alloc, "i-0e", BencodeError.NegativeZero);
    expectDecodeError(alloc, "ie", BencodeError.InvalidInteger);
    expectDecodeError(alloc, "i-e", BencodeError.InvalidInteger);

    // Negative string length / invalid syntax
    expectDecodeError(alloc, "-3:abc", BencodeError.InvalidSyntax);
    expectDecodeError(alloc, "d1:ai1e1:be", BencodeError.UnexpectedEof);
    expectDecodeError(alloc, "5:ab", BencodeError.UnexpectedEof);
    expectDecodeError(alloc, "", BencodeError.UnexpectedEof);

    // Unordered & duplicate dict keys
    expectDecodeError(alloc, "d1:bi1e1:ai2ee", BencodeError.UnorderedDictKeys);
    expectDecodeError(alloc, "d1:ai1e1:ai2ee", BencodeError.DuplicateDictKey);

    // Trailing bytes
    expectDecodeError(alloc, "i42egarb", BencodeError.TrailingBytes);

    // Depth bomb: 64 allowed, 65 rejected
    let ok_buf: [130]u8 = undefined;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        ok_buf[i] = 'l';
        ok_buf[64 + i] = 'e';
    }
    let ok_val = decode(alloc, ok_buf[0..128])?;
    ok_val.deinit(alloc);

    let bad_buf: [130]u8 = undefined;
    i = 0;
    while (i < 65) : (i += 1) {
        bad_buf[i] = 'l';
        bad_buf[65 + i] = 'e';
    }
    expectDecodeError(alloc, bad_buf[0..130], BencodeError.MaxDepthExceeded);

    // Item limit
    switch (decodeWithLimits(alloc, "li1ei2ei3ee", BENCODE_MAX_DEPTH, 3)) {
        val => {
            let mv = val;
            mv.deinit(alloc);
            expect_eq(true, false);
        },
        err(e) => {
            expect_eq(e == BencodeError.MaxSizeExceeded, true);
        },
    }
});

test("bencode: canonical dictionary key sorting on encode", () => {
    const alloc = std.testing.allocator;

    let inner_list: [2]BencodeValue = [
        BencodeValue.makeInt(-7),
        BencodeValue.makeBytes("x\x00y"),
    ];
    let entries: [3]BencodeDictEntry = [
        BencodeDictEntry.init("zeta", BencodeValue.makeInt(100)),
        BencodeDictEntry.init("alpha", BencodeValue.makeList(&inner_list)),
        BencodeDictEntry.init("mid", BencodeValue.makeBytes("cp-bencode")),
    ];
    const root_val = BencodeValue.makeDict(&entries);
    const encoded = encode(alloc, root_val)?;
    defer alloc.free(encoded);

    const expected_canonical = "d5:alphali-7e3:x\x00ye3:mid10:cp-bencode4:zetai100ee";
    expect_eq(std.mem.eql(u8, encoded, expected_canonical), true);
});

test("bencode: zero-copy extractRawSection and computeInfoHashSha1", () => {
    const info_raw = "d6:lengthi1024e4:name8:test.bin12:piece lengthi512e6:pieces40:01234567890123456789abcdefghijabcdefghije";
    const torrent_bytes = "d8:announce23:http://tracker.test/ann4:info" ++ info_raw ++ "e";

    const extracted = extractRawSection(torrent_bytes, "info")?;
    expect_eq(std.mem.eql(u8, extracted, info_raw), true);

    const digest = computeInfoHashSha1(torrent_bytes)?;
    let expected_digest: [20]u8 = [0; 20];
    std.crypto.hash.Sha1.hash(info_raw, &expected_digest, .{});
    expect_eq(std.mem.eql(u8, &digest, &expected_digest), true);

    const hex = formatInfoHashHex(digest);
    const expected_hex = std.fmt.bytesToHex(expected_digest, .lower);
    expect_eq(std.mem.eql(u8, &hex, &expected_hex), true);

    switch (extractRawSection(torrent_bytes, "nonexistent")) {
        * => expect_eq(true, false),
        err(e) => expect_eq(e == BencodeError.KeyNotFound, true),
    }
});

test("bencode: dynamic cap.bencode namespace mounting and zero-leak check", () => {
    const alloc = std.testing.allocator;
    expect_eq(cap.bencode.BENCODE_MAX_DEPTH, 64 as usize);

    let val = cap.bencode.decode(alloc, "d3:cap7:bencode3:veri1ee")?;
    defer val.deinit(alloc);
    expect_eq(std.mem.eql(u8, val.getBytes("cap").?, "bencode"), true);
    expect_eq(val.getInt("ver").?, 1 as i64);

    const encoded = cap.bencode.encode(alloc, val)?;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, "d3:cap7:bencode3:veri1ee"), true);
});

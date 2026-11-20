// Source: cp/cap/test/test_cbor_production_deep.cp
// Deep Production Test Suite for cap:cbor (RFC 8949 CBOR & Section 4.2 Deterministic Canonical Encoding)
// Covers:
//   1. RFC 8949 Major Type 0..7 official vectors (unsigned/negative boundaries, bytes/text, bool/null/undefined, f16/f32/f64)
//   2. Nested array/map & Tag semantic tag encode/decode round-trip
//   3. encodeCanonical deterministic Map key sorting by CBOR-encoded byte order (WebAuthn/COSE style)
//   4. Security & malformed input guards: InvalidAdditionalInfo(28), TrailingBytes, UnexpectedEof, InvalidUtf8, InvalidBreak, UnsupportedIndefinite, IntegerOverflow, MaxDepthExceeded
//   5. Dynamic lazy namespace mounting (`import cap from "cap"; cap.cbor`) & `std.testing.allocator` zero memory leaks

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CborError,
    CborMapEntry,
    CborTag,
    CborValue,
    encode,
    encodeCanonical,
    decode,
    decodeWithMaxDepth,
} from "cap:cbor";
import cap from "cap";

fn expectEncodedEq(alloc: std.mem.Allocator, val: CborValue, expected: []const u8)! {
    const encoded = encode(alloc, val) catch unreachable;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, expected), true);
}

test("1. RFC 8949 Major Type 0..7 Standard Vectors Round-Trip") {
    const alloc = std.testing.allocator;

    // Major 0: Unsigned integer boundaries
    expectEncodedEq(alloc, CborValue.makeUnsigned(0), "\x00") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeUnsigned(23), "\x17") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeUnsigned(24), "\x18\x18") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeUnsigned(1000), "\x19\x03\xE8") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeUnsigned(1000000), "\x1A\x00\x0F\x42\x40") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeUnsigned(std.math.maxInt(u64)), "\x1B\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF") catch unreachable;

    // Major 1: Negative integer boundaries (-1 .. minInt(i64))
    expectEncodedEq(alloc, CborValue.makeNegative(-1), "\x20") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeNegative(-10), "\x29") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeNegative(-100), "\x38\x63") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeNegative(-1000), "\x39\x03\xE7") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeNegative(-9223372036854775808), "\x3B\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF") catch unreachable;

    let d_neg = decode(alloc, "\x3B\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF") catch unreachable;
    defer d_neg.deinit(alloc);
    expect_eq(d_neg.eql(CborValue.makeNegative(-9223372036854775808)), true);

    // Major 2 & 3: Bytes & UTF-8 Text
    expectEncodedEq(alloc, CborValue.makeBytes("\x01\x02\x03\x04"), "\x44\x01\x02\x03\x04") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeText("IETF"), "\x64IETF") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeText("\xC3\xBC"), "\x62\xC3\xBC") catch unreachable;

    let d_txt = decode(alloc, "\x62\xC3\xBC") catch unreachable;
    defer d_txt.deinit(alloc);
    expect_eq(d_txt.eql(CborValue.makeText("\xC3\xBC")), true);

    // Major 7: Simple values (false/true/null/undefined) & IEEE 754 f16/f32/f64
    expectEncodedEq(alloc, CborValue.makeBool(false), "\xF4") catch unreachable;
    expectEncodedEq(alloc, CborValue.makeBool(true), "\xF5") catch unreachable;
    expectEncodedEq(alloc, .null_val, "\xF6") catch unreachable;
    expectEncodedEq(alloc, .undefined_val, "\xF7") catch unreachable;

    let d_false = decode(alloc, "\xF4") catch unreachable;
    defer d_false.deinit(alloc);
    expect_eq(d_false.eql(CborValue.makeBool(false)), true);

    let d_true = decode(alloc, "\xF5") catch unreachable;
    defer d_true.deinit(alloc);
    expect_eq(d_true.eql(CborValue.makeBool(true)), true);

    let d_null = decode(alloc, "\xF6") catch unreachable;
    defer d_null.deinit(alloc);
    expect_eq(d_null.eql(.null_val), true);

    let d_undef = decode(alloc, "\xF7") catch unreachable;
    defer d_undef.deinit(alloc);
    expect_eq(d_undef.eql(.undefined_val), true);

    // f16: 0xF9 0x3C 0x00 -> 1.0; 0xF9 0xC0 0x00 -> -2.0
    let d_f16 = decode(alloc, "\xF9\x3C\x00") catch unreachable;
    defer d_f16.deinit(alloc);
    expect_eq(d_f16.eql(CborValue.makeFloat(1.0)), true);
    expectEncodedEq(alloc, CborValue.makeFloat(1.0), "\xF9\x3C\x00") catch unreachable;

    let d_f16_neg = decode(alloc, "\xF9\xC0\x00") catch unreachable;
    defer d_f16_neg.deinit(alloc);
    expect_eq(d_f16_neg.eql(CborValue.makeFloat(-2.0)), true);

    // f32: 0xFA 0x47 0xC3 0x50 0x00 -> 100000.0
    let d_f32 = decode(alloc, "\xFA\x47\xC3\x50\x00") catch unreachable;
    defer d_f32.deinit(alloc);
    expect_eq(d_f32.eql(CborValue.makeFloat(100000.0)), true);
    expectEncodedEq(alloc, CborValue.makeFloat(100000.0), "\xFA\x47\xC3\x50\x00") catch unreachable;

    // f64: 0xFB 0x3F 0xF1 0x99 0x99 0x99 0x99 0x99 0x9A -> 1.1
    let d_f64 = decode(alloc, "\xFB\x3F\xF1\x99\x99\x99\x99\x99\x9A") catch unreachable;
    defer d_f64.deinit(alloc);
    expect_eq(d_f64.eql(CborValue.makeFloat(1.1)), true);
    expectEncodedEq(alloc, CborValue.makeFloat(1.1), "\xFB\x3F\xF1\x99\x99\x99\x99\x99\x9A") catch unreachable;
}

test("2. Nested Array/Map & Semantic Tag Encode/Decode Round-Trip") {
    const alloc = std.testing.allocator;

    let inner_arr: [3]CborValue = [
        CborValue.makeUnsigned(10),
        CborValue.makeNegative(-42),
        CborValue.makeBool(true),
    ];
    let tag_inner = CborValue.makeText("2026-11-20T00:00:00Z");
    let root_entries: [4]CborMapEntry = [
        CborMapEntry.init(CborValue.makeText("ts"), CborValue.makeTag(0, &tag_inner)),
        CborMapEntry.init(CborValue.makeText("vec"), CborValue.makeArray(&inner_arr)),
        CborMapEntry.init(CborValue.makeText("raw"), CborValue.makeBytes("\xDE\xAD\xBE\xEF")),
        CborMapEntry.init(CborValue.makeText("empty"), .undefined_val),
    ];
    const doc = CborValue.makeMap(&root_entries);

    const encoded = encode(alloc, doc) catch unreachable;
    defer alloc.free(encoded);

    let decoded = decode(alloc, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(doc), true);
}

test("3. encodeCanonical Deterministic Map Key Ordering (RFC 8949 Section 4.2)") {
    const alloc = std.testing.allocator;

    // Mixed keys in unsorted order:
    //   "aa" (0x62 'a' 'a')
    //   "b"  (0x61 'b')
    //   100  (0x18 0x64)
    //   -1   (0x20)
    //   1    (0x01)
    // Expected canonical byte-wise order:
    //   1 (0x01) < 100 (0x18 0x64) < -1 (0x20) < "b" (0x61 0x62) < "aa" (0x62 0x61 0x61)
    let nested_entries: [2]CborMapEntry = [
        CborMapEntry.init(CborValue.makeText("z"), CborValue.makeUnsigned(2)),
        CborMapEntry.init(CborValue.makeText("a"), CborValue.makeUnsigned(1)),
    ];
    let entries: [5]CborMapEntry = [
        CborMapEntry.init(CborValue.makeText("aa"), CborValue.makeUnsigned(5)),
        CborMapEntry.init(CborValue.makeText("b"), CborValue.makeMap(&nested_entries)),
        CborMapEntry.init(CborValue.makeUnsigned(100), CborValue.makeUnsigned(2)),
        CborMapEntry.init(CborValue.makeNegative(-1), CborValue.makeUnsigned(3)),
        CborMapEntry.init(CborValue.makeUnsigned(1), CborValue.makeUnsigned(1)),
    ];
    const val = CborValue.makeMap(&entries);

    const canonical = encodeCanonical(alloc, val) catch unreachable;
    defer alloc.free(canonical);

    // Expected:
    // A5
    //   01 01
    //   18 64 02
    //   20 03
    //   61 'b' A2 61 'a' 01 61 'z' 02
    //   62 'a' 'a' 05
    const expected = "\xA5\x01\x01\x18\x64\x02\x20\x03\x61b\xA2\x61a\x01\x61z\x02\x62aa\x05";
    expect_eq(std.mem.eql(u8, canonical, expected), true);

    let decoded = decode(alloc, canonical) catch unreachable;
    defer decoded.deinit(alloc);
    switch (decoded) {
        .map m => {
            expect_eq(m.len, 5 as usize);
            expect_eq(m[0].key.eql(CborValue.makeUnsigned(1)), true);
            expect_eq(m[1].key.eql(CborValue.makeUnsigned(100)), true);
            expect_eq(m[2].key.eql(CborValue.makeNegative(-1)), true);
            expect_eq(m[3].key.eql(CborValue.makeText("b")), true);
            expect_eq(m[4].key.eql(CborValue.makeText("aa")), true);
        },
        else => expect_eq(true, false),
    }
}

test("4. Malformed Input Guards: AI 28, TrailingBytes, UnexpectedEof, InvalidUtf8 & MaxDepthExceeded") {
    const alloc = std.testing.allocator;

    // 1. Invalid Additional Info 28 (0x1C)
    let hit_ai28 = false;
    let r_ai = decode(alloc, "\x1C") catch |e| {
        if (e == err.InvalidAdditionalInfo) hit_ai28 = true;
        CborValue.null_val
    };
    r_ai.deinit(alloc);
    expect_eq(hit_ai28, true);

    // 2. TrailingBytes
    let hit_trailing = false;
    let r_trail = decode(alloc, "\x01\x02") catch |e| {
        if (e == err.TrailingBytes) hit_trailing = true;
        CborValue.null_val
    };
    r_trail.deinit(alloc);
    expect_eq(hit_trailing, true);

    // 3. Truncated UnexpectedEof (empty & truncated 2-byte uint)
    let hit_eof_empty = false;
    let r_empty = decode(alloc, "") catch |e| {
        if (e == err.UnexpectedEof) hit_eof_empty = true;
        CborValue.null_val
    };
    r_empty.deinit(alloc);
    expect_eq(hit_eof_empty, true);

    let hit_eof_trunc = false;
    let r_trunc = decode(alloc, "\x19\x03") catch |e| {
        if (e == err.UnexpectedEof) hit_eof_trunc = true;
        CborValue.null_val
    };
    r_trunc.deinit(alloc);
    expect_eq(hit_eof_trunc, true);

    // 4. InvalidUtf8 in Major 3 text (0x62 0xFF 0xFE)
    let hit_utf8 = false;
    let r_utf8 = decode(alloc, "\x62\xFF\xFE") catch |e| {
        if (e == err.InvalidUtf8) hit_utf8 = true;
        CborValue.null_val
    };
    r_utf8.deinit(alloc);
    expect_eq(hit_utf8, true);

    // 5. InvalidBreak (0xFF) & UnsupportedIndefinite (0x9F 0xFF)
    let hit_break = false;
    let r_brk = decode(alloc, "\xFF") catch |e| {
        if (e == err.InvalidBreak) hit_break = true;
        CborValue.null_val
    };
    r_brk.deinit(alloc);
    expect_eq(hit_break, true);

    let hit_indef = false;
    let r_indef = decode(alloc, "\x9F\xFF") catch |e| {
        if (e == err.UnsupportedIndefinite) hit_indef = true;
        CborValue.null_val
    };
    r_indef.deinit(alloc);
    expect_eq(hit_indef, true);

    // 6. IntegerOverflow (Major 1 with n = maxInt(u64) -> -1 - 2^64+1 overflows i64)
    let hit_overflow = false;
    let r_ovf = decode(alloc, "\x3B\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF") catch |e| {
        if (e == err.IntegerOverflow) hit_overflow = true;
        CborValue.null_val
    };
    r_ovf.deinit(alloc);
    expect_eq(hit_overflow, true);

    // 7. MaxDepthExceeded with decodeWithMaxDepth(alloc, ..., 3)
    // 4 nested arrays: [[[[1]]]] -> 0x81 0x81 0x81 0x81 0x01
    let hit_depth = false;
    let r_depth = decodeWithMaxDepth(alloc, "\x81\x81\x81\x81\x01", 3) catch |e| {
        if (e == err.MaxDepthExceeded) hit_depth = true;
        CborValue.null_val
    };
    r_depth.deinit(alloc);
    expect_eq(hit_depth, true);
}

test("5. Dynamic `cap.cbor` Namespace Mounting & Zero-Leak Round-Trip") {
    const alloc = std.testing.allocator;

    let entries: [2]CborMapEntry = [
        CborMapEntry.init(CborValue.makeText("alg"), CborValue.makeNegative(-7)),
        CborMapEntry.init(CborValue.makeUnsigned(1), CborValue.makeUnsigned(2)),
    ];
    const cose_key = CborValue.makeMap(&entries);

    const bytes = cap.cbor.encodeCanonical(alloc, cose_key) catch unreachable;
    defer alloc.free(bytes);

    let decoded = cap.cbor.decode(alloc, bytes) catch unreachable;
    defer decoded.deinit(alloc);

    switch (decoded) {
        .map m => {
            expect_eq(m.len, 2 as usize);
            // Key 1 (0x01) comes before key "alg" (0x63...)
            expect_eq(m[0].key.eql(CborValue.makeUnsigned(1)), true);
            expect_eq(m[0].value.eql(CborValue.makeUnsigned(2)), true);
            expect_eq(m[1].key.eql(CborValue.makeText("alg")), true);
            expect_eq(m[1].value.eql(CborValue.makeNegative(-7)), true);
        },
        else => expect_eq(true, false),
    }
}

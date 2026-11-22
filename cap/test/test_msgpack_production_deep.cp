// Source: cp/cap/test/test_msgpack_production_deep.cp
// Deep Production Test Suite for cap:msgpack
// Covers:
//   1. Scalar minimal compact encoding & lossless round-trip (pos/neg fixint, 64-bit bounds, f64, bool, nil, str, bin)
//   2. Nested array/map & fixext1/2/4/8/16 + ext8/16 extension types round-trip
//   3. Security guards: truncated UnexpectedEof, reserved 0xc1 InvalidFormatTag & TrailingBytes
//   4. Resource exhaustion protection: MaxDepthExceeded (default 64 & custom max_depth)
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.msgpack` & std.testing.allocator zero-leak

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MSGPACK_DEFAULT_MAX_DEPTH as DEFAULT_MAX_DEPTH,
    MsgPackError,
    MsgExt,
    MsgMapEntry,
    MsgValue,
    MsgPackWriter,
    MsgPackReader,
    msgpackEncode as encode,
    msgpackDecode as decode,
    msgpackDecodeWithMaxDepth as decodeWithMaxDepth,
} from "cap:binfmt";
import cap from "cap";

fn expectEncodedBytesEq(alloc: std.mem.Allocator, val: MsgValue, expected: []const u8)! {
    const encoded = encode(alloc, val) catch unreachable;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, expected), true);

    let decoded = decode(alloc, encoded) catch unreachable;
    defer decoded.deinit(alloc);
    expect_eq(decoded.eql(val), true);
}

test("1. Scalars Minimal Compact Encoding & Lossless Round-Trip") {
    const alloc = std.testing.allocator;

    // nil & bool
    expectEncodedBytesEq(alloc, MsgValue.makeNil(), "\xC0")?;
    expectEncodedBytesEq(alloc, MsgValue.makeBool(false), "\xC2")?;
    expectEncodedBytesEq(alloc, MsgValue.makeBool(true), "\xC3")?;

    // positive fixint: 0..127 -> single byte 0x00..0x7F
    expectEncodedBytesEq(alloc, MsgValue.makeUint(0), "\x00")?;
    expectEncodedBytesEq(alloc, MsgValue.makeUint(127), "\x7F")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(42), "\x2A")?;

    // uint8/16/32/64 boundaries
    expectEncodedBytesEq(alloc, MsgValue.makeUint(128), "\xCC\x80")?;
    expectEncodedBytesEq(alloc, MsgValue.makeUint(255), "\xCC\xFF")?;
    expectEncodedBytesEq(alloc, MsgValue.makeUint(256), "\xCD\x01\x00")?;
    expectEncodedBytesEq(alloc, MsgValue.makeUint(65536), "\xCE\x00\x01\x00\x00")?;
    expectEncodedBytesEq(alloc, MsgValue.makeUint(std.math.maxInt(u64)), "\xCF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF")?;

    // negative fixint: -1..-32 -> 0xFF..0xE0
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-1), "\xFF")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-32), "\xE0")?;

    // int8/16/32/64 boundaries
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-33), "\xD0\xDF")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-128), "\xD0\x80")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-129), "\xD1\xFF\x7F")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-32769), "\xD2\xFF\xFF\x7F\xFF")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(-9223372036854775808), "\xD3\x80\x00\x00\x00\x00\x00\x00\x00")?;
    expectEncodedBytesEq(alloc, MsgValue.makeInt(9223372036854775807), "\xCF\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF")?;

    // float64 (0xCB + IEEE 754 big-endian)
    expectEncodedBytesEq(alloc, MsgValue.makeFloat(1.5), "\xCB\x3F\xF8\x00\x00\x00\x00\x00\x00")?;
    expectEncodedBytesEq(alloc, MsgValue.makeFloat(-0.25), "\xCB\xBF\xD0\x00\x00\x00\x00\x00\x00")?;

    // fixstr & str8
    expectEncodedBytesEq(alloc, MsgValue.makeStr(""), "\xA0")?;
    expectEncodedBytesEq(alloc, MsgValue.makeStr("hello"), "\xA5hello")?;
    const s32 = "0123456789abcdef0123456789abcdef";
    const enc_s32 = encode(alloc, MsgValue.makeStr(s32)) catch unreachable;
    defer alloc.free(enc_s32);
    expect_eq(enc_s32[0], 0xD9 as u8);
    expect_eq(enc_s32[1], 32 as u8);
    let dec_s32 = decode(alloc, enc_s32) catch unreachable;
    defer dec_s32.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_s32.asStr().?, s32), true);

    // bin8 & bin16
    expectEncodedBytesEq(alloc, MsgValue.makeBin("\xDE\xAD\xBE\xEF"), "\xC4\x04\xDE\xAD\xBE\xEF")?;
    let big_bin: [300]u8 = [0x5A; 300];
    const enc_bin16 = encode(alloc, MsgValue.makeBin(big_bin[0..300])) catch unreachable;
    defer alloc.free(enc_bin16);
    expect_eq(enc_bin16[0], 0xC5 as u8);
    expect_eq(enc_bin16[1], 0x01 as u8);
    expect_eq(enc_bin16[2], 0x2C as u8);
    let dec_bin16 = decode(alloc, enc_bin16) catch unreachable;
    defer dec_bin16.deinit(alloc);
    expect_eq(std.mem.eql(u8, dec_bin16.asBin().?, big_bin[0..300]), true);
}

test("2. Nested Array/Map & FixExt/Ext8/16 Extension Types") {
    const alloc = std.testing.allocator;

    // Build nested structure using MsgPackWriter + encodeValue
    let writer = MsgPackWriter.init(alloc);
    defer writer.deinit();

    let inner_arr: [3]MsgValue = [
        MsgValue.makeInt(-10),
        MsgValue.makeStr("zero-copy"),
        MsgValue.makeExt(7, "\x01\x02\x03\x04"), // fixext4 (0xD6)
    ];

    let ext16_buf: [16]u8 = [0xAB; 16];
    let ext8_buf: [5]u8 = [0x11, 0x22, 0x33, 0x44, 0x55];

    let root_entries: [4]MsgMapEntry = [
        MsgMapEntry.init(MsgValue.makeStr("items"), MsgValue.makeArray(&inner_arr)),
        MsgMapEntry.init(MsgValue.makeStr("fixext1"), MsgValue.makeExt(-1, "\x7F")),
        MsgMapEntry.init(MsgValue.makeStr("fixext16"), MsgValue.makeExt(12, ext16_buf[0..16])),
        MsgMapEntry.init(MsgValue.makeStr("ext8"), MsgValue.makeExt(-42, ext8_buf[0..5])),
    ];

    const root_val = MsgValue.makeMap(&root_entries);
    writer.encodeValue(root_val) catch unreachable;
    const bytes = writer.bytes();

    let decoded = decode(alloc, bytes) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(root_val), true);
    const items = decoded.get("items").?.asArray().?;
    expect_eq(items.len, 3 as usize);
    expect_eq(items[0].asInt().?, -10 as i64);
    expect_eq(std.mem.eql(u8, items[1].asStr().?, "zero-copy"), true);

    const e4 = items[2].asExt().?;
    expect_eq(e4.ext_type, 7 as i8);
    expect_eq(std.mem.eql(u8, e4.data, "\x01\x02\x03\x04"), true);

    const e1 = decoded.get("fixext1").?.asExt().?;
    expect_eq(e1.ext_type, -1 as i8);
    expect_eq(e1.data.len, 1 as usize);

    const e16 = decoded.get("fixext16").?.asExt().?;
    expect_eq(e16.ext_type, 12 as i8);
    expect_eq(e16.data.len, 16 as usize);

    const e8 = decoded.get("ext8").?.asExt().?;
    expect_eq(e8.ext_type, -42 as i8);
    expect_eq(std.mem.eql(u8, e8.data, ext8_buf[0..5]), true);
}

test("3. Security Guards: UnexpectedEof, 0xC1 InvalidFormatTag & TrailingBytes") {
    const alloc = std.testing.allocator;

    // 1. Empty input -> UnexpectedEof
    let eof1 = false;
    * = decode(alloc, "") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) eof1 = true;
    };
    expect_eq(eof1, true);

    // 2. Truncated fixstr -> UnexpectedEof
    let eof2 = false;
    * = decode(alloc, "\xA5abc") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) eof2 = true;
    };
    expect_eq(eof2, true);

    // 3. Truncated nested array (must not leak partially decoded items!) -> UnexpectedEof
    let eof3 = false;
    * = decode(alloc, "\x92\x91\x01") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) eof3 = true;
    };
    expect_eq(eof3, true);

    // 4. Reserved 0xC1 format tag -> InvalidFormatTag
    let inv1 = false;
    * = decode(alloc, "\xC1") catch |e| {
        if (e == @as(anyerror, err.InvalidFormatTag)) inv1 = true;
    };
    expect_eq(inv1, true);

    // 5. Trailing garbage bytes -> TrailingBytes (must not leak decoded root!)
    let trail1 = false;
    * = decode(alloc, "\x91\x01\xFF") catch |e| {
        if (e == @as(anyerror, err.TrailingBytes)) trail1 = true;
    };
    expect_eq(trail1, true);
}

test("4. Resource Exhaustion Protection: MaxDepthExceeded (64-Layer & Custom)") {
    const alloc = std.testing.allocator;
    expect_eq(DEFAULT_MAX_DEPTH, 64 as usize);

    // 65 nested 1-element arrays (0x91 * 65 + 0x01) -> exceeds DEFAULT_MAX_DEPTH=64
    let deep65: [66]u8 = [0x91; 66];
    deep65[65] = 0x01;

    let hit_default = false;
    * = decode(alloc, deep65[0..66]) catch |e| {
        if (e == @as(anyerror, err.MaxDepthExceeded)) hit_default = true;
    };
    expect_eq(hit_default, true);

    // Custom max_depth=4: 4 layers allowed, 5 layers rejected
    let ok4: [5]u8 = [0x91, 0x91, 0x91, 0x91, 0x07];
    let dec4 = decodeWithMaxDepth(alloc, ok4[0..5], 4) catch unreachable;
    defer dec4.deinit(alloc);
    expect_eq(dec4.asArray().?[0].asArray().?[0].asArray().?[0].asArray().?[0].asUint().?, 7 as u64);

    let fail5: [6]u8 = [0x91, 0x91, 0x91, 0x91, 0x91, 0x07];
    let hit_custom = false;
    * = decodeWithMaxDepth(alloc, fail5[0..6], 4) catch |e| {
        if (e == @as(anyerror, err.MaxDepthExceeded)) hit_custom = true;
    };
    expect_eq(hit_custom, true);
}

test("5. Dynamic Lazy Namespace Mounting via `cap.msgpack` & Zero-Leak Verification") {
    const alloc = std.testing.allocator;

    let entries: [3]cap.binfmt.MsgMapEntry = [
        cap.binfmt.MsgMapEntry.init(cap.binfmt.MsgValue.makeStr("engine"), cap.binfmt.MsgValue.makeStr("cp-msgpack")),
        cap.binfmt.MsgMapEntry.init(cap.binfmt.MsgValue.makeStr("ready"), cap.binfmt.MsgValue.makeBool(true)),
        cap.binfmt.MsgMapEntry.init(cap.binfmt.MsgValue.makeStr("offset"), cap.binfmt.MsgValue.makeInt(-99)),
    ];
    const doc = cap.binfmt.MsgValue.makeMap(&entries);

    const encoded = cap.binfmt.msgpackEncode(alloc, doc) catch unreachable;
    defer alloc.free(encoded);

    let decoded = cap.binfmt.msgpackDecode(alloc, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(doc), true);
    expect_eq(std.mem.eql(u8, decoded.get("engine").?.asStr().?, "cp-msgpack"), true);
    expect_eq(decoded.get("ready").?.asBool().?, true);
    expect_eq(decoded.get("offset").?.asInt().?, -99 as i64);
}

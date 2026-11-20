// Source: cp/cap/test/test_cbor_production_deep.cp
// Deep Production Test Suite for cap:cbor
// Covers:
//   1. RFC 8949 CBOR official byte test vectors (uint, negint, Float16 0x3c00->1.0, Bytes, Text, Array, Map, Tag 0/1/24, Simple, Indefinite)
//   2. CBOR complex nested document lossless round-trip & BinDocument Arena deinit
//   3. MessagePack all-type lossless round-trip (fixint, uint8/16/32/64, int8/16/32/64, nil, bool, float32/64, str8/16, bin8/16, array16, map16)
//   4. Truncated payload (UnexpectedEof), trailing garbage bytes (TrailingBytes), invalid format & 64-layer max nesting depth protection
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.cbor`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BinError,
    BinTag,
    BinMapEntry,
    BinValue,
    BinDocument,
    CBOR_MAX_DEPTH,
    CBOR_TAG_DATETIME_TEXT,
    CBOR_TAG_EPOCH_TIME,
    CBOR_TAG_ENCODED_CBOR,
    Cbor,
    MSGPACK_MAX_DEPTH,
    MsgPack,
} from "cap:cbor";
import cap from "cap";

fn expectCborBytesEq(alloc: std.mem.Allocator, val: BinValue, expected: []const u8)! {
    const encoded = Cbor.encode(alloc, val) catch unreachable;
    defer alloc.free(encoded);
    expect_eq(std.mem.eql(u8, encoded, expected), true);
}

fn testRfc8949OfficialVectors()! {
    const alloc = std.heap.page_allocator;

    // 1. Unsigned integers (Major 0)
    expectCborBytesEq(alloc, BinValue.makeUint(0), "\x00")?;
    expectCborBytesEq(alloc, BinValue.makeUint(1), "\x01")?;
    expectCborBytesEq(alloc, BinValue.makeUint(23), "\x17")?;
    expectCborBytesEq(alloc, BinValue.makeUint(24), "\x18\x18")?;
    expectCborBytesEq(alloc, BinValue.makeUint(25), "\x18\x19")?;
    expectCborBytesEq(alloc, BinValue.makeUint(100), "\x18\x64")?;
    expectCborBytesEq(alloc, BinValue.makeUint(1000), "\x19\x03\xE8")?;
    expectCborBytesEq(alloc, BinValue.makeUint(1000000), "\x1A\x00\x0F\x42\x40")?;
    expectCborBytesEq(alloc, BinValue.makeUint(1000000000000), "\x1B\x00\x00\x00\xE8\xD4\xA5\x10\x00")?;
    expectCborBytesEq(alloc, BinValue.makeUint(std.math.maxInt(u64)), "\x1B\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF")?;

    // 2. Negative integers (Major 1: -1 - n)
    expectCborBytesEq(alloc, BinValue.makeInt(-1), "\x20")?;
    expectCborBytesEq(alloc, BinValue.makeInt(-10), "\x29")?;
    expectCborBytesEq(alloc, BinValue.makeInt(-100), "\x38\x63")?;
    expectCborBytesEq(alloc, BinValue.makeInt(-1000), "\x39\x03\xE7")?;
    expectCborBytesEq(alloc, BinValue.makeInt(-9223372036854775808), "\x3B\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF")?;

    let d_neg = Cbor.decode(alloc, "\x3B\x7F\xFF\xFF\xFF\xFF\xFF\xFF\xFF") catch unreachable;
    defer d_neg.deinit();
    expect_eq(d_neg.root.asInt().?, -9223372036854775808 as i64);

    // 3. IEEE 754 Float16 / Float32 / Float64 & Simple values (Major 7)
    // 0xf93c00 -> Float16 1.0
    let d_f16 = Cbor.decode(alloc, "\xF9\x3C\x00") catch unreachable;
    defer d_f16.deinit();
    expect_eq(d_f16.root.asFloat().?, 1.0);
    expectCborBytesEq(alloc, BinValue.makeFloat(1.0), "\xF9\x3C\x00")?;

    // 0xf9c000 -> Float16 -2.0; 0xf90001 -> Float16 5.960464477539063e-8
    let d_f16_neg2 = Cbor.decode(alloc, "\xF9\xC0\x00") catch unreachable;
    defer d_f16_neg2.deinit();
    expect_eq(d_f16_neg2.root.asFloat().?, -2.0);

    let d_f16_sub = Cbor.decode(alloc, "\xF9\x00\x01") catch unreachable;
    defer d_f16_sub.deinit();
    expect_eq(d_f16_sub.root.asFloat().?, 5.960464477539063e-8);

    // 0xfa47c35000 -> Float32 100000.0
    let d_f32 = Cbor.decode(alloc, "\xFA\x47\xC3\x50\x00") catch unreachable;
    defer d_f32.deinit();
    expect_eq(d_f32.root.asFloat().?, 100000.0);
    expectCborBytesEq(alloc, BinValue.makeFloat(100000.0), "\xFA\x47\xC3\x50\x00")?;

    // 0xfb3ff199999999999a -> Float64 1.1
    let d_f64 = Cbor.decode(alloc, "\xFB\x3F\xF1\x99\x99\x99\x99\x99\x9A") catch unreachable;
    defer d_f64.deinit();
    expect_eq(d_f64.root.asFloat().?, 1.1);
    expectCborBytesEq(alloc, BinValue.makeFloat(1.1), "\xFB\x3F\xF1\x99\x99\x99\x99\x99\x9A")?;

    // Simple: false(0xf4), true(0xf5), null(0xf6), undefined(0xf7)
    let d_false = Cbor.decode(alloc, "\xF4") catch unreachable;
    defer d_false.deinit();
    expect_eq(d_false.root.asBool().?, false);
    let d_true = Cbor.decode(alloc, "\xF5") catch unreachable;
    defer d_true.deinit();
    expect_eq(d_true.root.asBool().?, true);
    let d_null = Cbor.decode(alloc, "\xF6") catch unreachable;
    defer d_null.deinit();
    expect_eq(d_null.root.isNull(), true);
    let d_undef = Cbor.decode(alloc, "\xF7") catch unreachable;
    defer d_undef.deinit();
    expect_eq(d_undef.root.isNull(), true);

    // 4. Bytes (Major 2) & Text (Major 3)
    expectCborBytesEq(alloc, BinValue.makeBytes("\x01\x02\x03\x04"), "\x44\x01\x02\x03\x04")?;
    expectCborBytesEq(alloc, BinValue.makeText("IETF"), "\x64IETF")?;
    expectCborBytesEq(alloc, BinValue.makeText("\xC3\xBC"), "\x62\xC3\xBC")?;

    // 5. Array (Major 4) & Map (Major 5)
    let arr_items: [3]BinValue = [
        BinValue.makeUint(1),
        BinValue.makeUint(2),
        BinValue.makeUint(3),
    ];
    expectCborBytesEq(alloc, BinValue.makeArray(&arr_items), "\x83\x01\x02\x03")?;

    let map_items: [2]BinMapEntry = [
        BinMapEntry.init(BinValue.makeUint(1), BinValue.makeUint(2)),
        BinMapEntry.init(BinValue.makeUint(3), BinValue.makeUint(4)),
    ];
    expectCborBytesEq(alloc, BinValue.makeMap(&map_items), "\xA2\x01\x02\x03\x04")?;

    let d_map = Cbor.decode(alloc, "\xA2\x01\x02\x03\x04") catch unreachable;
    defer d_map.deinit();
    expect_eq(d_map.root.eql(BinValue.makeMap(&map_items)), true);

    // 6. Semantic Tags (Major 6): Tag 0, Tag 1, Tag 24
    // 0("2013-03-21T20:04:00Z") -> 0xc0 0x74 "2013-03-21T20:04:00Z"
    const dt_inner = BinValue.makeText("2013-03-21T20:04:00Z");
    const tag0 = BinValue.makeTag(CBOR_TAG_DATETIME_TEXT, &dt_inner);
    expectCborBytesEq(alloc, tag0, "\xC0\x742013-03-21T20:04:00Z")?;

    let d_tag0 = Cbor.decode(alloc, "\xC0\x742013-03-21T20:04:00Z") catch unreachable;
    defer d_tag0.deinit();
    expect_eq(d_tag0.root.asTag().?.tag, 0 as u64);
    expect_eq(std.mem.eql(u8, d_tag0.root.asTag().?.value.asText().?, "2013-03-21T20:04:00Z"), true);

    // 1(1363896240) -> 0xc1 0x1a 0x51 0x4b 0x67 0xb0
    const epoch_inner = BinValue.makeUint(1363896240);
    const tag1 = BinValue.makeTag(CBOR_TAG_EPOCH_TIME, &epoch_inner);
    expectCborBytesEq(alloc, tag1, "\xC1\x1A\x51\x4B\x67\xB0")?;

    // 24(h'6449455446') -> 0xd8 0x18 0x45 0x64 0x49 0x45 0x54 0x46
    const emb_inner = BinValue.makeBytes("\x64\x49\x45\x54\x46");
    const tag24 = BinValue.makeTag(CBOR_TAG_ENCODED_CBOR, &emb_inner);
    expectCborBytesEq(alloc, tag24, "\xD8\x18\x45\x64\x49\x45\x54\x46")?;

    // 7. Indefinite-length CBOR decoding: (_ "strea" "ming") & [_ 1 [_ 2 3] [_ 4 5]]
    let d_indef_str = Cbor.decode(alloc, "\x7F\x65strea\x64ming\xFF") catch unreachable;
    defer d_indef_str.deinit();
    expect_eq(std.mem.eql(u8, d_indef_str.root.asText().?, "streaming"), true);

    let d_indef_arr = Cbor.decode(alloc, "\x9F\x01\x82\x02\x03\x9F\x04\x05\xFF\xFF") catch unreachable;
    defer d_indef_arr.deinit();
    expect_eq(d_indef_arr.root.len(), 3 as usize);
    expect_eq(d_indef_arr.root.getIndex(0).?.asUint().?, 1 as u64);
    expect_eq(d_indef_arr.root.getIndex(2).?.getIndex(1).?.asUint().?, 5 as u64);
}

fn testCborComplexNestedRoundTrip()! {
    const alloc = std.heap.page_allocator;

    let inner_arr: [4]BinValue = [
        BinValue.makeNull(),
        BinValue.makeBool(true),
        BinValue.makeInt(-424242),
        BinValue.makeFloat(3.141592653589793),
    ];
    const ts_val = BinValue.makeUint(1700000000);
    const tagged_ts = BinValue.makeTag(CBOR_TAG_EPOCH_TIME, &ts_val);

    let sub_map: [2]BinMapEntry = [
        BinMapEntry.init(BinValue.makeUint(10), BinValue.makeBytes("\xDE\xAD\xBE\xEF\x00\xFF")),
        BinMapEntry.init(BinValue.makeText("created_at"), tagged_ts),
    ];

    let root_entries: [5]BinMapEntry = [
        BinMapEntry.init(BinValue.makeText("service"), BinValue.makeText("cp-cbor-engine")),
        BinMapEntry.init(BinValue.makeText("version"), BinValue.makeUint(8949)),
        BinMapEntry.init(BinValue.makeText("neg_edge"), BinValue.makeInt(-9223372036854775808)),
        BinMapEntry.init(BinValue.makeText("metrics"), BinValue.makeArray(&inner_arr)),
        BinMapEntry.init(BinValue.makeText("meta"), BinValue.makeMap(&sub_map)),
    ];
    const original = BinValue.makeMap(&root_entries);

    const encoded = Cbor.encode(alloc, original) catch unreachable;
    defer alloc.free(encoded);

    let decoded = Cbor.decode(alloc, encoded) catch unreachable;
    defer decoded.deinit();

    expect_eq(decoded.root.eql(original), true);
    expect_eq(std.mem.eql(u8, decoded.root.get("service").?.asText().?, "cp-cbor-engine"), true);
    expect_eq(decoded.root.get("version").?.asUint().?, 8949 as u64);
    expect_eq(decoded.root.get("neg_edge").?.asInt().?, -9223372036854775808 as i64);
    expect_eq(decoded.root.get("metrics").?.getIndex(2).?.asInt().?, -424242 as i64);
    expect_eq(decoded.root.get("meta").?.get("created_at").?.asTag().?.tag, 1 as u64);
    expect_eq(decoded.root.get("meta").?.get("created_at").?.asTag().?.value.asUint().?, 1700000000 as u64);
}

fn testMsgPackAllTypesRoundTrip()! {
    const alloc = std.heap.page_allocator;

    // 1. Exact MessagePack wire format checks
    const enc_pos = MsgPack.encode(alloc, BinValue.makeUint(127)) catch unreachable;
    defer alloc.free(enc_pos);
    expect_eq(std.mem.eql(u8, enc_pos, "\x7F"), true);

    const enc_neg = MsgPack.encode(alloc, BinValue.makeInt(-32)) catch unreachable;
    defer alloc.free(enc_neg);
    expect_eq(std.mem.eql(u8, enc_neg, "\xE0"), true);

    const enc_nil = MsgPack.encode(alloc, BinValue.makeNull()) catch unreachable;
    defer alloc.free(enc_nil);
    expect_eq(std.mem.eql(u8, enc_nil, "\xC0"), true);

    // 2. Large strings (> 31 bytes -> str8, > 255 bytes -> str16) and bin8/bin16
    let str_300: [300]u8 = ['a'; 300];
    let bin_300: [300]u8 = [0xAB; 300];

    // Array with 20 elements (> 15 -> array16)
    let arr_20: [20]BinValue = [BinValue.makeNull(); 20];
    arr_20[0] = BinValue.makeUint(0);
    arr_20[1] = BinValue.makeUint(127);
    arr_20[2] = BinValue.makeUint(200);
    arr_20[3] = BinValue.makeUint(50000);
    arr_20[4] = BinValue.makeUint(3000000000);
    arr_20[5] = BinValue.makeUint(std.math.maxInt(u64));
    arr_20[6] = BinValue.makeInt(-1);
    arr_20[7] = BinValue.makeInt(-32);
    arr_20[8] = BinValue.makeInt(-100);
    arr_20[9] = BinValue.makeInt(-20000);
    arr_20[10] = BinValue.makeInt(-1000000000);
    arr_20[11] = BinValue.makeInt(-9223372036854775808);
    arr_20[12] = BinValue.makeBool(false);
    arr_20[13] = BinValue.makeBool(true);
    arr_20[14] = BinValue.makeFloat(1.5);
    arr_20[15] = BinValue.makeFloat(3.141592653589793);
    arr_20[16] = BinValue.makeText("short");
    arr_20[17] = BinValue.makeText(str_300[0..40]);
    arr_20[18] = BinValue.makeText(str_300[0..300]);
    arr_20[19] = BinValue.makeBytes(bin_300[0..300]);

    // Map with 18 entries (> 15 -> map16)
    let map_18: [18]BinMapEntry = [BinMapEntry{}; 18];
    let idx: usize = 0;
    while (idx < 17) : (idx += 1) {
        map_18[idx] = BinMapEntry.init(BinValue.makeUint(idx as u64), BinValue.makeInt(-((idx as i64) + 1)));
    }
    map_18[17] = BinMapEntry.init(BinValue.makeText("payload"), BinValue.makeArray(&arr_20));

    const root = BinValue.makeMap(&map_18);
    const mp_encoded = MsgPack.encode(alloc, root) catch unreachable;
    defer alloc.free(mp_encoded);

    let mp_decoded = MsgPack.decode(alloc, mp_encoded) catch unreachable;
    defer mp_decoded.deinit();

    expect_eq(mp_decoded.root.eql(root), true);
    const dec_payload = mp_decoded.root.get("payload").?;
    expect_eq(dec_payload.len(), 20 as usize);
    expect_eq(dec_payload.getIndex(5).?.asUint().?, std.math.maxInt(u64));
    expect_eq(dec_payload.getIndex(11).?.asInt().?, -9223372036854775808 as i64);
    expect_eq(dec_payload.getIndex(15).?.asFloat().?, 3.141592653589793);
    expect_eq(dec_payload.getIndex(19).?.asBytes().?.len, 300 as usize);
}

fn testSecurityGuardsAndErrors()! {
    const alloc = std.heap.page_allocator;

    // 1. Empty / truncated payloads -> UnexpectedEof
    let eof_hit = false;
    * = Cbor.decode(alloc, "") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) eof_hit = true;
    };
    expect_eq(eof_hit, true);

    let cbor_trunc_hit = false;
    * = Cbor.decode(alloc, "\x65abc") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) cbor_trunc_hit = true;
    };
    expect_eq(cbor_trunc_hit, true);

    let mp_trunc_hit = false;
    * = MsgPack.decode(alloc, "\xD9\x05abc") catch |e| {
        if (e == @as(anyerror, err.UnexpectedEof)) mp_trunc_hit = true;
    };
    expect_eq(mp_trunc_hit, true);

    // 2. Trailing garbage bytes -> TrailingBytes
    let cbor_trail_hit = false;
    * = Cbor.decode(alloc, "\x01\x02") catch |e| {
        if (e == @as(anyerror, err.TrailingBytes)) cbor_trail_hit = true;
    };
    expect_eq(cbor_trail_hit, true);

    let mp_trail_hit = false;
    * = MsgPack.decode(alloc, "\xC3\x00") catch |e| {
        if (e == @as(anyerror, err.TrailingBytes)) mp_trail_hit = true;
    };
    expect_eq(mp_trail_hit, true);

    // 3. Invalid format checks (0xFF break at root in CBOR, 0xC1 unused in MsgPack, invalid UTF-8)
    let cbor_bad_hit = false;
    * = Cbor.decode(alloc, "\xFF") catch |e| {
        if (e == @as(anyerror, err.InvalidFormat)) cbor_bad_hit = true;
    };
    expect_eq(cbor_bad_hit, true);

    let cbor_utf8_hit = false;
    * = Cbor.decode(alloc, "\x62\xFF\xFE") catch |e| {
        if (e == @as(anyerror, err.InvalidFormat)) cbor_utf8_hit = true;
    };
    expect_eq(cbor_utf8_hit, true);

    let mp_unused_hit = false;
    * = MsgPack.decode(alloc, "\xC1") catch |e| {
        if (e == @as(anyerror, err.InvalidFormat)) mp_unused_hit = true;
    };
    expect_eq(mp_unused_hit, true);

    // 4. Max nesting depth 64 guard:
    // 64 nested single-element arrays (0x81 * 64 + 0x01) must succeed; 65 (0x81 * 65 + 0x01) must fail!
    let cbor_64: [65]u8 = [0x81; 65];
    cbor_64[64] = 0x01;
    let d_ok_64 = Cbor.decode(alloc, cbor_64[0..65]) catch unreachable;
    d_ok_64.deinit();

    let cbor_65: [66]u8 = [0x81; 66];
    cbor_65[65] = 0x01;
    let cbor_depth_hit = false;
    * = Cbor.decode(alloc, cbor_65[0..66]) catch |e| {
        if (e == @as(anyerror, err.MaxDepthExceeded)) cbor_depth_hit = true;
    };
    expect_eq(cbor_depth_hit, true);

    // Same 64/65 nesting depth guard for MessagePack (0x91 = fixarray(1))
    let mp_64: [65]u8 = [0x91; 65];
    mp_64[64] = 0x01;
    let mp_ok_64 = MsgPack.decode(alloc, mp_64[0..65]) catch unreachable;
    mp_ok_64.deinit();

    let mp_65: [66]u8 = [0x91; 66];
    mp_65[65] = 0x01;
    let mp_depth_hit = false;
    * = MsgPack.decode(alloc, mp_65[0..66]) catch |e| {
        if (e == @as(anyerror, err.MaxDepthExceeded)) mp_depth_hit = true;
    };
    expect_eq(mp_depth_hit, true);

    expect_eq(CBOR_MAX_DEPTH, 64 as usize);
    expect_eq(MSGPACK_MAX_DEPTH, 64 as usize);
}

fn testCapCborDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;

    let entries: [3]cap.cbor.BinMapEntry = [
        cap.cbor.BinMapEntry.init(cap.cbor.BinValue.makeText("kind"), cap.cbor.BinValue.makeText("telemetry")),
        cap.cbor.BinMapEntry.init(cap.cbor.BinValue.makeText("ok"), cap.cbor.BinValue.makeBool(true)),
        cap.cbor.BinMapEntry.init(cap.cbor.BinValue.makeText("delta"), cap.cbor.BinValue.makeInt(-17)),
    ];
    const v = cap.cbor.BinValue.makeMap(&entries);

    const cbor_buf = cap.cbor.Cbor.encode(alloc, v) catch unreachable;
    defer alloc.free(cbor_buf);
    let cbor_doc = cap.cbor.Cbor.decode(alloc, cbor_buf) catch unreachable;
    defer cbor_doc.deinit();
    expect_eq(cbor_doc.root.eql(v), true);

    const mp_buf = cap.cbor.MsgPack.encode(alloc, v) catch unreachable;
    defer alloc.free(mp_buf);
    let mp_doc = cap.cbor.MsgPack.decode(alloc, mp_buf) catch unreachable;
    defer mp_doc.deinit();
    expect_eq(mp_doc.root.eql(v), true);
    expect_eq(mp_doc.root.get("delta").?.asInt().?, -17 as i64);
}

fn main() {
    test("1. RFC 8949 CBOR Official Byte Test Vectors (Major 0-7, Float16, Tags & Indefinite)", () => {
        testRfc8949OfficialVectors()?;
    });

    test("2. CBOR Complex Nested Document Lossless Round-Trip & Arena BinDocument", () => {
        testCborComplexNestedRoundTrip()?;
    });

    test("3. MessagePack Full Type Matrix Lossless Round-Trip", () => {
        testMsgPackAllTypesRoundTrip()?;
    });

    test("4. Security Guards: Truncation, Trailing Bytes, Invalid Format & Depth-64 Limit", () => {
        testSecurityGuardsAndErrors()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.cbor`", () => {
        testCapCborDynamicLazyMount()?;
    });
}

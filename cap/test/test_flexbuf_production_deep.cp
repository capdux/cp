// Source: cp/cap/test/test_flexbuf_production_deep.cp (Production deep tests for cap:flexbuf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    FlexType,
    FlexBitWidth,
    flexBitWidthBytes,
    unpackFlexType,
    packFlexType,
    FlexRef,
    parseFlexRoot,
} from "cap:compiler";

fn putU32LE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

test("1. root scalars: int, uint, float32, bool, null, string, and blob", () => {
    expect_eq(flexBitWidthBytes(FlexBitWidth.w8), 1);
    expect_eq(flexBitWidthBytes(FlexBitWidth.w64), 8);

    // Root int8 = -42 (0xD6), packed = (int=1 << 2) | w8 = 0x04, root_bw = 1
    const raw_int: [3]u8 = [0xD6, packFlexType(FlexType.int, FlexBitWidth.w8), 1];
    const r_int = parseFlexRoot(raw_int[0..3]) ?? unreachable;
    expect_eq(r_int.asInt() ?? 0, -42);
    expect_eq(r_int.asNull(), false);

    // Root uint16 = 0x1234
    const raw_uint: [4]u8 = [0x34, 0x12, packFlexType(FlexType.uint, FlexBitWidth.w8), 2];
    const r_uint = parseFlexRoot(raw_uint[0..4]) ?? unreachable;
    expect_eq(r_uint.asUint() ?? 0, 0x1234);

    // Root float32 = 3.5 (IEEE 754: 0x40600000)
    let raw_f32: [6]u8 = [0; 6];
    let rf = raw_f32[0..6];
    putU32LE(*rf, 0, 0x40600000);
    rf[4] = packFlexType(FlexType.float, FlexBitWidth.w8);
    rf[5] = 4;
    const r_flt = parseFlexRoot(rf) ?? unreachable;
    expect_eq(r_flt.asFloat() ?? 0.0, 3.5);

    // Root bool = true & null
    const raw_bool: [3]u8 = [1, packFlexType(FlexType.bool_val, FlexBitWidth.w8), 1];
    expect_eq((parseFlexRoot(raw_bool[0..3]) ?? unreachable).asBool() ?? false, true);

    const raw_null: [3]u8 = [0, packFlexType(FlexType.null_val, FlexBitWidth.w8), 1];
    expect_eq((parseFlexRoot(raw_null[0..3]) ?? unreachable).asNull(), true);

    // Root string "hi": [len=2, 'h', 'i', 0, rel_off=3, packed(string, w8), root_bw=1]
    // offset 4 holds rel_off=3 => target = 4 - 3 = 1; target - 1 = 0 holds len=2
    const raw_str: [7]u8 = [2, 'h', 'i', 0, 3, packFlexType(FlexType.string, FlexBitWidth.w8), 1];
    const r_str = parseFlexRoot(raw_str[0..7]) ?? unreachable;
    expect_eq(std.mem.eql(u8, r_str.asString() ?? "", "hi"), true);

    // Root blob [0xCA, 0xFE, 0xBA]
    const raw_blob: [7]u8 = [3, 0xCA, 0xFE, 0xBA, 3, packFlexType(FlexType.blob, FlexBitWidth.w8), 1];
    const r_blob = parseFlexRoot(raw_blob[0..7]) ?? unreachable;
    const blob = r_blob.asBlob() ?? unreachable;
    expect_eq(blob.len, 3);
    expect_eq(blob[0], 0xCA);
    expect_eq(blob[2], 0xBA);
});

test("2. heterogeneous vector and typed vector_int indexing", () => {
    // Typed vector_int (w8): [len=3, 10, -5(0xFB), 42, rel_off=3, packed(vector_int, w8), root_bw=1]
    // root offset = 4, rel_off = 3 => target = 1, len at 0 = 3
    const vec_int: [7]u8 = [3, 10, 0xFB, 42, 3, packFlexType(FlexType.vector_int, FlexBitWidth.w8), 1];
    const rv = parseFlexRoot(vec_int[0..7]) ?? unreachable;
    expect_eq(rv.vecLen() ?? 0, 3);
    expect_eq((rv.vecGet(0) ?? unreachable).asInt() ?? 0, 10);
    expect_eq((rv.vecGet(1) ?? unreachable).asInt() ?? 0, -5);
    expect_eq((rv.vecGet(2) ?? unreachable).asInt() ?? 0, 42);
    expect_eq(rv.vecGet(3) == null, true);

    // Heterogeneous vector: [len=2, elem0=99 (uint), elem1=1 (bool), type0, type1, rel_off=4, packed(vector, w8), bw=1]
    // offset 5 holds rel_off=4 => target = 1, types at 1 + 2 = 3..5
    const poly_vec: [8]u8 = [
        2,
        99,
        1,
        packFlexType(FlexType.uint, FlexBitWidth.w8),
        packFlexType(FlexType.bool_val, FlexBitWidth.w8),
        4,
        packFlexType(FlexType.vector, FlexBitWidth.w8),
        1,
    ];
    const rp = parseFlexRoot(poly_vec[0..8]) ?? unreachable;
    expect_eq(rp.vecLen() ?? 0, 2);
    expect_eq((rp.vecGet(0) ?? unreachable).asUint() ?? 0, 99);
    expect_eq((rp.vecGet(1) ?? unreachable).asBool() ?? false, true);
});

test("3. ordered Map binary search via mapGet and nested string value lookup", () => {
    // Layout (all widths = 1B):
    // 0..4:  key0 = "age\0"  (offset 0)
    // 4..9:  key1 = "name\0" (offset 4)
    // 9..13: str  = [2, 'c', 'p', 0] (string payload at offset 10, len at offset 9)
    // 13:    keys_vec len = 2
    // 14:    key0 rel_off = 14 - 0 = 14
    // 15:    key1 rel_off = 15 - 4 = 11
    // 16:    map keys_rel_off = 16 - 14 = 2  (target - 3 = 16)
    // 17:    map keys_bw = 1                 (target - 2 = 17)
    // 18:    map len = 2                     (target - 1 = 18)
    // 19:    val0 (age) = 25 (int)           (target = 19)
    // 20:    val1 (name) rel_off = 20 - 10 = 10 (string)
    // 21:    type0 = int(w8)
    // 22:    type1 = string(w8)
    // 23:    root rel_off = 23 - 19 = 4
    // 24:    root packed = map(w8)
    // 25:    root bw = 1
    const map_buf: [26]u8 = [
        'a', 'g', 'e', 0,
        'n', 'a', 'm', 'e', 0,
        2, 'c', 'p', 0,
        2, 14, 11,
        2, 1, 2,
        25, 10,
        packFlexType(FlexType.int, FlexBitWidth.w8),
        packFlexType(FlexType.string, FlexBitWidth.w8),
        4,
        packFlexType(FlexType.map, FlexBitWidth.w8),
        1,
    ];

    const root = parseFlexRoot(map_buf[0..26]) ?? unreachable;
    expect_eq(root.mapLen() ?? 0, 2);
    expect_eq(std.mem.eql(u8, root.mapKeyAt(0) ?? "", "age"), true);
    expect_eq(std.mem.eql(u8, root.mapKeyAt(1) ?? "", "name"), true);

    const age = root.mapGet("age") ?? unreachable;
    expect_eq(age.asInt() ?? 0, 25);

    const name = root.mapGet("name") ?? unreachable;
    expect_eq(std.mem.eql(u8, name.asString() ?? "", "cp"), true);

    expect_eq(root.mapGet("missing") == null, true);
    expect_eq(root.mapGet("aaa") == null, true);
    expect_eq(root.mapGet("zzz") == null, true);
});

test("4. indirect_int and indirect_float relative offset dereferencing", () => {
    // 0..4: i32 = -100000 (LE: 0x60, 0x79, 0xFE, 0xFF)
    // 4:    root rel_off = 4 (points to 0)
    // 5:    packed(indirect_int, w32)
    // 6:    root_bw = 1
    let ibuf: [7]u8 = [0; 7];
    let ib = ibuf[0..7];
    const neg_val: i32 = -100000;
    putU32LE(*ib, 0, neg_val as u32);
    ib[4] = 4;
    ib[5] = packFlexType(FlexType.indirect_int, FlexBitWidth.w32);
    ib[6] = 1;

    const r_iint = parseFlexRoot(ib) ?? unreachable;
    expect_eq(r_iint.asInt() ?? 0, -100000);

    // indirect_float (f32 = 2.25 => 0x40100000)
    putU32LE(*ib, 0, 0x40100000);
    ib[4] = 4;
    ib[5] = packFlexType(FlexType.indirect_float, FlexBitWidth.w32);
    ib[6] = 1;

    const r_iflt = parseFlexRoot(ib) ?? unreachable;
    expect_eq(r_iflt.asFloat() ?? 0.0, 2.25);
});

test("5. truncated buffers, invalid root_byte_width, unknown type, and out-of-bounds offsets fail safely", () => {
    const short_buf: [2]u8 = [0, 1];
    expect_eq(parseFlexRoot(short_buf[0..2]) == null, true);

    // Invalid root_byte_width = 3 (not 1/2/4/8)
    const bad_bw: [4]u8 = [0, 0, packFlexType(FlexType.int, FlexBitWidth.w8), 3];
    expect_eq(parseFlexRoot(bad_bw[0..4]) == null, true);

    // Unknown FlexType (raw = 30)
    expect_eq(unpackFlexType(30 << 2) == null, true);
    const bad_type: [3]u8 = [0, 30 << 2, 1];
    expect_eq(parseFlexRoot(bad_type[0..3]) == null, true);

    // Out-of-bounds relative offset (rel_off = 99 > root_offset = 1)
    const bad_rel: [4]u8 = [0, 99, packFlexType(FlexType.string, FlexBitWidth.w8), 1];
    const r_bad = parseFlexRoot(bad_rel[0..4]) ?? unreachable;
    expect_eq(r_bad.asString() == null, true);
});

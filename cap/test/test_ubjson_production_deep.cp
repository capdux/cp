// Source: cp/cap/test/test_ubjson_production_deep.cp (Production deep tests for cap:ubjson)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    UBJ_NULL,
    UBJ_NOOP,
    UBJ_TRUE,
    UBJ_FALSE,
    UBJ_INT8,
    UBJ_UINT8,
    UBJ_INT16,
    UBJ_INT32,
    UBJ_INT64,
    UBJ_FLOAT32,
    UBJ_FLOAT64,
    UBJ_CHAR,
    UBJ_STRING,
    UBJ_ARRAY_START,
    UBJ_ARRAY_END,
    UBJ_OBJECT_START,
    UBJ_OBJECT_END,
    UBJ_OPT_TYPE,
    UBJ_OPT_COUNT,
    MAX_UBJ_DEPTH,
    UbjsonContainerHeader,
    UbjsonWriter,
    UbjsonReader,
} from "cap:binfmt";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Full UBJSON scalar types and writeCompactInt minimal encoding roundtrip", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = UbjsonWriter.init(buf[0..]);

    expect_eq(w.writeNull(), true);
    expect_eq(w.writeBool(true), true);
    expect_eq(w.writeBool(false), true);
    expect_eq(w.writeCompactInt(200), true);          // 'U' (2B)
    expect_eq(w.writeCompactInt(-50), true);          // 'i' (2B)
    expect_eq(w.writeCompactInt(10000), true);        // 'I' (3B)
    expect_eq(w.writeCompactInt(-1_000_000), true);   // 'l' (5B)
    expect_eq(w.writeCompactInt(5_000_000_000), true); // 'L' (9B)
    expect_eq(w.writeFloat32(3.5), true);
    expect_eq(w.writeFloat64(-123.456), true);
    expect_eq(w.writeChar('A'), true);
    expect_eq(w.writeString("ubjson"), true);

    const enc = w.written();
    // Verify minimal integer markers selected by writeCompactInt
    expect_eq(enc[3], UBJ_UINT8);
    expect_eq(enc[5], UBJ_INT8);
    expect_eq(enc[7], UBJ_INT16);
    expect_eq(enc[10], UBJ_INT32);
    expect_eq(enc[15], UBJ_INT64);

    let r = UbjsonReader.init(enc);
    expect_eq(r.readNull(), true);
    expect_eq(r.readBool() ?? false, true);
    expect_eq(r.readBool() ?? true, false);
    expect_eq(r.readInt() ?? 0, 200);
    expect_eq(r.readInt() ?? 0, -50);
    expect_eq(r.readInt() ?? 0, 10000);
    expect_eq(r.readInt() ?? 0, -1_000_000);
    expect_eq(r.readInt() ?? 0, 5_000_000_000);
    const f32v = r.readFloat32() ?? 0.0;
    expect_eq(f32v > 3.49 && f32v < 3.51, true);
    const f64v = r.readFloat64() ?? 0.0;
    expect_eq(f64v > -123.457 && f64v < -123.455, true);
    expect_eq(r.readChar() ?? 0, 'A');
    const s = r.readString() ?? unreachable;
    expect_eq(sliceEq(s, "ubjson"), true);
    expect_eq(r.remaining(), 0);
});

test("2. Nested Object + Array with interspersed No-Op ('N') automatic skipping", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = UbjsonWriter.init(buf[0..]);

    expect_eq(w.writeNoop(), true);
    expect_eq(w.writeObjectStart(), true);
    expect_eq(w.writeNoop(), true);
    expect_eq(w.writeKey("id"), true);
    expect_eq(w.writeNoop(), true);
    expect_eq(w.writeCompactInt(77), true);
    expect_eq(w.writeKey("tags"), true);
    expect_eq(w.writeArrayStart(), true);
    expect_eq(w.writeNoop(), true);
    expect_eq(w.writeString("cp"), true);
    expect_eq(w.writeString("fast"), true);
    expect_eq(w.writeArrayEnd(), true);
    expect_eq(w.writeNoop(), true);
    expect_eq(w.writeObjectEnd(), true);

    let r = UbjsonReader.init(w.written());
    const obj_hdr = r.readContainerHeader(true) ?? unreachable;
    expect_eq(obj_hdr.is_object, true);
    expect_eq(obj_hdr.count == null, true);

    const k1 = r.readKey() ?? unreachable;
    expect_eq(sliceEq(k1, "id"), true);
    expect_eq(r.readInt() ?? 0, 77);

    const k2 = r.readKey() ?? unreachable;
    expect_eq(sliceEq(k2, "tags"), true);
    const arr_hdr = r.readContainerHeader(false) ?? unreachable;
    expect_eq(arr_hdr.is_object, false);
    const t0 = r.readString() ?? unreachable;
    const t1 = r.readString() ?? unreachable;
    expect_eq(sliceEq(t0, "cp"), true);
    expect_eq(sliceEq(t1, "fast"), true);
    expect_eq(r.peekMarker() ?? 0, UBJ_ARRAY_END);
    * = r.readRawU8(); // consume ']'

    expect_eq(r.readKey() == null, true); // consumes '}'
    expect_eq(r.remaining(), 0);
});

test("3. Optimized typed arrays ([$U#count and [$l#count) encoding and decoding", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let w = UbjsonWriter.init(buf[0..]);

    // [$U#4 + 4 raw bytes
    expect_eq(w.writeTypedArrayHeader(UBJ_UINT8, 4), true);
    const bytes_in: [4]u8 = [10, 20, 30, 40];
    expect_eq(w.writeRawBytes(bytes_in[0..]), true);

    // [$l#2 + 2 raw i32 BE values
    expect_eq(w.writeTypedArrayHeader(UBJ_INT32, 2), true);
    expect_eq(w.writeRawI32BE(-100), true);
    expect_eq(w.writeRawI32BE(200000), true);

    let r = UbjsonReader.init(w.written());
    const h1 = r.readContainerHeader(false) ?? unreachable;
    expect_eq(h1.elem_type ?? 0, UBJ_UINT8);
    expect_eq(h1.count ?? 0, 4);
    expect_eq(r.readRawU8() ?? 0, 10);
    expect_eq(r.readRawU8() ?? 0, 20);
    expect_eq(r.readRawU8() ?? 0, 30);
    expect_eq(r.readRawU8() ?? 0, 40);

    const h2 = r.readContainerHeader(false) ?? unreachable;
    expect_eq(h2.elem_type ?? 0, UBJ_INT32);
    expect_eq(h2.count ?? 0, 2);
    expect_eq(r.readRawI32BE() ?? 0, -100);
    expect_eq(r.readRawI32BE() ?? 0, 200000);
    expect_eq(r.remaining(), 0);
});

test("4. skipValue skips complex nested objects, typed arrays, and lands cleanly on next value", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let w = UbjsonWriter.init(buf[0..]);

    // First value: complex nested object with typed array inside
    expect_eq(w.writeObjectStart(), true);
    expect_eq(w.writeKey("pixels"), true);
    expect_eq(w.writeTypedArrayHeader(UBJ_UINT8, 3), true);
    const px: [3]u8 = [0xFF, 0x80, 0x01];
    expect_eq(w.writeRawBytes(px[0..]), true);
    expect_eq(w.writeKey("nested"), true);
    expect_eq(w.writeArrayStart(), true);
    expect_eq(w.writeNull(), true);
    expect_eq(w.writeFloat64(9.25), true);
    expect_eq(w.writeArrayEnd(), true);
    expect_eq(w.writeObjectEnd(), true);

    // Second value: target integer 9999
    expect_eq(w.writeCompactInt(9999), true);

    let r = UbjsonReader.init(w.written());
    expect_eq(r.skipValue(), true);
    expect_eq(r.readInt() ?? 0, 9999);
    expect_eq(r.remaining(), 0);
});

test("5. Malformed UBJSON protection: invalid marker, negative length, missing count after $, truncated, and max depth", () => {
    // Invalid marker 'X'
    const bad_m: [1]u8 = ['X'];
    let rm = UbjsonReader.init(bad_m[0..]);
    expect_eq(rm.skipValue(), false);

    // Negative string length: 'S', 'i', -5 (0xFB)
    const neg_str: [3]u8 = [UBJ_STRING, UBJ_INT8, 0xFB];
    let rns = UbjsonReader.init(neg_str[0..]);
    expect_eq(rns.readString() == null, true);

    // '$' type without '#' count rejected per Draft 12
    const bad_opt: [4]u8 = [UBJ_ARRAY_START, UBJ_OPT_TYPE, UBJ_UINT8, UBJ_ARRAY_END];
    let ro = UbjsonReader.init(bad_opt[0..]);
    expect_eq(ro.readContainerHeader(false) == null, true);

    // Truncated typed array claiming 1000 i32s on 8-byte buffer
    let trunc: [16]u8 = std.mem.zeroes([16]u8);
    let wt = UbjsonWriter.init(trunc[0..]);
    expect_eq(wt.writeTypedArrayHeader(UBJ_INT32, 1000), true);
    let rt = UbjsonReader.init(wt.written());
    expect_eq(rt.skipValue(), false);

    // Excessive nesting depth (> MAX_UBJ_DEPTH = 64) rejected safely
    let deep: [80]u8 = std.mem.zeroes([80]u8);
    let i: usize = 0;
    while (i < (MAX_UBJ_DEPTH as usize) + 6) {
        deep[i] = UBJ_ARRAY_START;
        i += 1;
    }
    let rd = UbjsonReader.init(deep[0..i]);
    expect_eq(rd.skipValue(), false);
});

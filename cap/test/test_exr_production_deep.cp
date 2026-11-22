// Source: cp/cap/test/test_exr_production_deep.cp (Production deep tests for cap:exr)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    EXR_MAGIC,
    EXR_FLAG_TILED,
    EXR_FLAG_LONG_NAMES,
    EXR_FLAG_NON_IMAGE_PARTS,
    EXR_FLAG_MULTIPART,
    ExrCompression,
    ExrBox2i,
    ExrAttr,
    ExrHeader,
    compressionFromByte,
    isExr,
    parseExrBox2i,
    ExrAttrIter,
    parseExrHeader,
} from "cap:image";

fn putU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putI32LE(buf: []u8, off: usize, v: i32) {
    const raw: u32 = @bitCast(v);
    putU32LE(buf, off, raw);
}

fn writeCString(buf: []u8, off: usize, s: []const u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        buf[off + i] = s[i];
        i += 1;
    }
    buf[off + s.len] = 0;
    return s.len + 1;
}

fn writeAttr(buf: []u8, off: usize, name: []const u8, type_name: []const u8, val: []const u8): usize {
    let p = off;
    p += writeCString(buf, p, name);
    p += writeCString(buf, p, type_name);
    putU32LE(buf, p, val.len as u32);
    p += 4;
    let i: usize = 0;
    while (i < val.len) {
        buf[p + i] = val[i];
        i += 1;
    }
    return (p + val.len) - off;
}

test("1. parseExrHeader decodes standard single-part scanline EXR header with dataWindow and compression", () => {
    let buf: [256]u8 = undefined;
    let s = buf[0..256];

    // 8-byte EXR magic + version 2 (scanline, no flags)
    putU32LE(s, 0, EXR_MAGIC);
    putU32LE(s, 4, 2);

    let box_bytes: [16]u8 = undefined;
    putI32LE(box_bytes[0..16], 0, 0);    // x_min
    putI32LE(box_bytes[0..16], 4, 0);    // y_min
    putI32LE(box_bytes[0..16], 8, 1919); // x_max -> width 1920
    putI32LE(box_bytes[0..16], 12, 1079); // y_max -> height 1080

    const comp_val: [1]u8 = [4]; // PIZ = 4

    let pos: usize = 8;
    pos += writeAttr(s, pos, "compression", "compression", comp_val[0..1]);
    pos += writeAttr(s, pos, "dataWindow", "box2i", box_bytes[0..16]);
    s[pos] = 0; // End of header
    pos += 1;
    const expected_data_off = pos;

    // Append 8 dummy pixel bytes
    let k: usize = 0;
    while (k < 8) {
        s[pos + k] = 0xAB;
        k += 1;
    }

    const total = pos + 8;
    expect_eq(isExr(s[0..total]), true);

    const hdr = parseExrHeader(s[0..total]) ?? unreachable;
    expect_eq(hdr.version, 2);
    expect_eq(hdr.is_tiled, false);
    expect_eq(hdr.has_long_names, false);
    expect_eq(hdr.has_non_image_parts, false);
    expect_eq(hdr.is_multipart, false);
    expect_eq(hdr.compression == ExrCompression.piz, true);
    expect_eq(hdr.width, 1920);
    expect_eq(hdr.height, 1080);
    expect_eq(hdr.attr_count, 2);
    expect_eq(hdr.data_offset, expected_data_off);
});

test("2. parseExrHeader decodes Tiled, LongNames, NonImageParts, and Multipart version flags", () => {
    let buf: [64]u8 = undefined;
    let s = buf[0..64];
    putU32LE(s, 0, EXR_MAGIC);
    putU32LE(s, 4, 2 | EXR_FLAG_TILED | EXR_FLAG_LONG_NAMES | EXR_FLAG_NON_IMAGE_PARTS | EXR_FLAG_MULTIPART);
    s[8] = 0; // Empty attribute list terminated immediately

    const hdr = parseExrHeader(s[0..9]) ?? unreachable;
    expect_eq(hdr.version, 2);
    expect_eq(hdr.is_tiled, true);
    expect_eq(hdr.has_long_names, true);
    expect_eq(hdr.has_non_image_parts, true);
    expect_eq(hdr.is_multipart, true);
    expect_eq(hdr.attr_count, 0);
    expect_eq(hdr.data_offset, 9);
});

test("3. ExrAttrIter iterates custom attributes and parseExrBox2i computes negative-origin dimensions", () => {
    let buf: [160]u8 = undefined;
    let s = buf[0..160];
    putU32LE(s, 0, EXR_MAGIC);
    putU32LE(s, 4, 2);

    let box_bytes: [16]u8 = undefined;
    putI32LE(box_bytes[0..16], 0, -10);
    putI32LE(box_bytes[0..16], 4, -20);
    putI32LE(box_bytes[0..16], 8, 9);   // width = 9 - (-10) + 1 = 20
    putI32LE(box_bytes[0..16], 12, 29); // height = 29 - (-20) + 1 = 50

    let pos: usize = 8;
    pos += writeAttr(s, pos, "owner", "string", "cp-renderer");
    pos += writeAttr(s, pos, "displayWindow", "box2i", box_bytes[0..16]);
    s[pos] = 0;
    pos += 1;

    let it = ExrAttrIter.init(s[0..pos]);
    const a0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, a0.name, "owner"), true);
    expect_eq(std.mem.eql(u8, a0.type_name, "string"), true);
    expect_eq(std.mem.eql(u8, a0.value, "cp-renderer"), true);

    const a1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, a1.name, "displayWindow"), true);
    const box = parseExrBox2i(a1.value) ?? unreachable;
    expect_eq(box.x_min, -10);
    expect_eq(box.y_min, -20);
    expect_eq(box.width(), 20);
    expect_eq(box.height(), 50);

    expect_eq(it.next() == null, true);
    expect_eq(it.finished, true);
    expect_eq(it.had_error, false);
    expect_eq(it.data_offset, pos);
});

test("4. compressionFromByte maps all 10 OpenEXR compression codecs and unknown fallback", () => {
    expect_eq(compressionFromByte(0) == ExrCompression.none, true);
    expect_eq(compressionFromByte(1) == ExrCompression.rle, true);
    expect_eq(compressionFromByte(2) == ExrCompression.zips, true);
    expect_eq(compressionFromByte(3) == ExrCompression.zip, true);
    expect_eq(compressionFromByte(4) == ExrCompression.piz, true);
    expect_eq(compressionFromByte(5) == ExrCompression.pxr24, true);
    expect_eq(compressionFromByte(6) == ExrCompression.b44, true);
    expect_eq(compressionFromByte(7) == ExrCompression.b44a, true);
    expect_eq(compressionFromByte(8) == ExrCompression.dwaa, true);
    expect_eq(compressionFromByte(9) == ExrCompression.dwab, true);
    expect_eq(compressionFromByte(99) == ExrCompression.unknown, true);
});

test("5. parseExrHeader rejects bad magic, missing terminator, negative/OOB attribute size, and invalid box2i", () => {
    // Bad magic
    const bad_magic: [10]u8 = [0x89, 'P', 'N', 'G', 0, 0, 0, 0, 0, 0];
    expect_eq(isExr(bad_magic[0..10]), false);
    expect_eq(parseExrHeader(bad_magic[0..10]) == null, true);

    // Truncated header without terminating '\0'
    let trunc: [16]u8 = undefined;
    putU32LE(trunc[0..16], 0, EXR_MAGIC);
    putU32LE(trunc[0..16], 4, 2);
    let i: usize = 8;
    while (i < 16) {
        trunc[i] = 'a';
        i += 1;
    }
    expect_eq(parseExrHeader(trunc[0..16]) == null, true);

    // Attribute claiming out-of-bounds size 0x10000
    let oob: [32]u8 = undefined;
    const s = oob[0..32];
    putU32LE(s, 0, EXR_MAGIC);
    putU32LE(s, 4, 2);
    let p: usize = 8;
    p += writeCString(s, p, "foo");
    p += writeCString(s, p, "int");
    putU32LE(s, p, 0x10000);
    expect_eq(parseExrHeader(s) == null, true);

    // Invalid box2i length & inverted box
    expect_eq(parseExrBox2i(s[0..8]) == null, true);
    const inv = ExrBox2i { x_min: 10, y_min: 10, x_max: 5, y_max: 5 };
    expect_eq(inv.width(), 0);
    expect_eq(inv.height(), 0);
});

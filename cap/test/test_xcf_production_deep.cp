// Source: cp/cap/test/test_xcf_production_deep.cp (Production deep tests for cap:xcf)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XCF_MAGIC,
    PROP_END,
    PROP_COMPRESSION,
    PROP_RESOLUTION,
    XcfBaseType,
    XcfCompression,
    XcfPrecision,
    XcfHeader,
    XcfProperty,
    XcfPropIter,
    XcfPointerIter,
    XcfLayerInfo,
    parseXcfCompression,
    isXcf,
    parseXcfHeader,
    parseXcfLayerHeader,
} from "cap:xcf";

fn putU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64BE(buf: *[]u8, off: usize, v: u64) {
    putU32BE(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32BE(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: *[]u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. parseXcfHeader decodes v000 'file\\0' (32-bit) and v011+ (64-bit + precision)", () => {
    // v0 header (26 bytes): "gimp xcf file\0" + 1920x1080 + RGB(0)
    let v0: [26]u8 = [0; 26];
    let b0 = v0[0..26];
    copyBytes(*b0, 0, "gimp xcf file");
    b0[13] = 0;
    putU32BE(*b0, 14, 1920);
    putU32BE(*b0, 18, 1080);
    putU32BE(*b0, 22, 0);

    expect_eq(isXcf(b0), true);
    const h0 = parseXcfHeader(b0) ?? unreachable;
    expect_eq(h0.version, 0);
    expect_eq(h0.width, 1920);
    expect_eq(h0.height, 1080);
    expect_eq(h0.base_type == XcfBaseType.rgb, true);
    expect_eq(h0.precision == XcfPrecision.u8_non_linear, true);
    expect_eq(h0.uses_64bit_offsets, false);
    expect_eq(h0.props_offset, 26);

    // v12 header (30 bytes): "gimp xcf v012\0" + 512x256 + Grayscale(1) + float_linear(600)
    let v12: [30]u8 = [0; 30];
    let b12 = v12[0..30];
    copyBytes(*b12, 0, "gimp xcf v012");
    b12[13] = 0;
    putU32BE(*b12, 14, 512);
    putU32BE(*b12, 18, 256);
    putU32BE(*b12, 22, 1);
    putU32BE(*b12, 26, 600);

    const h12 = parseXcfHeader(b12) ?? unreachable;
    expect_eq(h12.version, 12);
    expect_eq(h12.base_type == XcfBaseType.grayscale, true);
    expect_eq(h12.precision == XcfPrecision.float_linear, true);
    expect_eq(h12.uses_64bit_offsets, true);
    expect_eq(h12.props_offset, 30);
});

test("2. XcfPropIter iterates PROP_COMPRESSION, PROP_RESOLUTION, and stops at PROP_END", () => {
    // 26B v0 header + PROP_COMPRESSION(17, 1B=[1=rle]) + PROP_RESOLUTION(19, 8B) + PROP_END(0, 0B)
    // = 26 + 9 + 16 + 8 = 59 bytes
    let buf: [64]u8 = [0; 64];
    let b = buf[0..64];
    copyBytes(*b, 0, "gimp xcf file");
    b[13] = 0;
    putU32BE(*b, 14, 64);
    putU32BE(*b, 18, 64);
    putU32BE(*b, 22, 0);

    let pos: usize = 26;
    // PROP_COMPRESSION (17), size = 1, value = 1 (RLE)
    putU32BE(*b, pos, PROP_COMPRESSION);
    putU32BE(*b, pos + 4, 1);
    b[pos + 8] = 1;
    pos += 9;

    // PROP_RESOLUTION (19), size = 8
    putU32BE(*b, pos, PROP_RESOLUTION);
    putU32BE(*b, pos + 4, 8);
    pos += 16;

    // PROP_END (0), size = 0
    putU32BE(*b, pos, PROP_END);
    putU32BE(*b, pos + 4, 0);
    pos += 8;

    const hdr = parseXcfHeader(b) ?? unreachable;
    let pit = XcfPropIter.init(b, hdr.props_offset);

    const p0 = pit.next() ?? unreachable;
    expect_eq(p0.prop_type, PROP_COMPRESSION);
    expect_eq(parseXcfCompression(p0.payload) == XcfCompression.rle, true);

    const p1 = pit.next() ?? unreachable;
    expect_eq(p1.prop_type, PROP_RESOLUTION);
    expect_eq(p1.payload.len, 8);

    expect_eq(pit.next() == null, true);
    expect_eq(pit.layersListOffset() ?? 0, pos);
});

test("3. 32-bit XcfPointerIter and parseXcfLayerHeader extract layer dimensions and UTF-8 names", () => {
    // 26B v0 header + 8B PROP_END (34) + 12B layer ptr list ([80, 120, 0] -> ends at 46)
    // Layer 0 at 80: 640x480, type=1 (RGBA), name="Background\0" (11B)
    // Layer 1 at 120: 100x100, type=1 (RGBA), name="Overlay\0" (8B)
    let buf: [160]u8 = [0; 160];
    let b = buf[0..160];
    copyBytes(*b, 0, "gimp xcf file");
    b[13] = 0;
    putU32BE(*b, 14, 640);
    putU32BE(*b, 18, 480);
    putU32BE(*b, 22, 0);
    // PROP_END at 26..34
    putU32BE(*b, 26, PROP_END);
    putU32BE(*b, 30, 0);
    // Layer pointer list at 34..46
    putU32BE(*b, 34, 80);
    putU32BE(*b, 38, 120);
    putU32BE(*b, 42, 0);

    // Layer 0 at 80
    putU32BE(*b, 80, 640);
    putU32BE(*b, 84, 480);
    putU32BE(*b, 88, 1);
    putU32BE(*b, 92, 11);
    copyBytes(*b, 96, "Background");
    b[106] = 0;

    // Layer 1 at 120
    putU32BE(*b, 120, 100);
    putU32BE(*b, 124, 100);
    putU32BE(*b, 128, 1);
    putU32BE(*b, 132, 8);
    copyBytes(*b, 136, "Overlay");
    b[143] = 0;

    const hdr = parseXcfHeader(b) ?? unreachable;
    let pit = XcfPropIter.init(b, hdr.props_offset);
    expect_eq(pit.next() == null, true);
    const list_off = pit.layersListOffset() ?? unreachable;
    expect_eq(list_off, 34);

    let lit = XcfPointerIter.init(b, list_off, hdr.uses_64bit_offsets);
    const off0 = lit.next() ?? unreachable;
    expect_eq(off0, 80);
    const l0 = parseXcfLayerHeader(b, off0, false) ?? unreachable;
    expect_eq(l0.width, 640);
    expect_eq(l0.height, 480);
    expect_eq(l0.layer_type, 1);
    expect_eq(std.mem.eql(u8, l0.name, "Background"), true);
    expect_eq(l0.props_offset, 107);

    const off1 = lit.next() ?? unreachable;
    expect_eq(off1, 120);
    const l1 = parseXcfLayerHeader(b, off1, false) ?? unreachable;
    expect_eq(std.mem.eql(u8, l1.name, "Overlay"), true);

    expect_eq(lit.next() == null, true);
    expect_eq(lit.nextOffset(), 46);
});

test("4. 64-bit XcfPointerIter handles v011+ 8-byte layer offsets accurately", () => {
    let buf: [128]u8 = [0; 128];
    let b = buf[0..128];
    copyBytes(*b, 0, "gimp xcf v011");
    b[13] = 0;
    putU32BE(*b, 14, 32);
    putU32BE(*b, 18, 32);
    putU32BE(*b, 22, 0);
    putU32BE(*b, 26, 150); // u8_non_linear
    // PROP_END at 30..38
    putU32BE(*b, 30, PROP_END);
    putU32BE(*b, 34, 0);
    // 64-bit layer list at 38..54: [80, 0]
    putU64BE(*b, 38, 80);
    putU64BE(*b, 46, 0);

    // Layer at 80
    putU32BE(*b, 80, 32);
    putU32BE(*b, 84, 32);
    putU32BE(*b, 88, 0);
    putU32BE(*b, 92, 4);
    copyBytes(*b, 96, "Top");
    b[99] = 0;

    const hdr = parseXcfHeader(b) ?? unreachable;
    expect_eq(hdr.uses_64bit_offsets, true);
    let lit = XcfPointerIter.init(b, 38, true);
    const off0 = lit.next() ?? unreachable;
    expect_eq(off0, 80);
    const l0 = parseXcfLayerHeader(b, off0, true) ?? unreachable;
    expect_eq(std.mem.eql(u8, l0.name, "Top"), true);
    expect_eq(lit.next() == null, true);
});

test("5. bad magic, invalid version tags, out-of-bounds properties and layer offsets fail safely", () => {
    const short_buf: [10]u8 = [0; 10];
    expect_eq(isXcf(short_buf[0..10]), false);
    expect_eq(parseXcfHeader(short_buf[0..10]) == null, true);

    let bad: [32]u8 = [0; 32];
    let b = bad[0..32];
    // Wrong magic
    copyBytes(*b, 0, "NOT_XCF_FILE!");
    expect_eq(isXcf(b), false);

    // Unsupported version v099
    copyBytes(*b, 0, "gimp xcf v099");
    b[13] = 0;
    expect_eq(isXcf(b), false);

    // Missing NUL terminator at byte 13
    copyBytes(*b, 0, "gimp xcf fileX");
    expect_eq(isXcf(b), false);

    // Out-of-bounds property payload size
    copyBytes(*b, 0, "gimp xcf file");
    b[13] = 0;
    putU32BE(*b, 26, PROP_PARASITES_TEST_ID);
    let prop_buf: [40]u8 = [0; 40];
    let pb = prop_buf[0..40];
    putU32BE(*pb, 0, PROP_COMPRESSION);
    putU32BE(*pb, 4, 9999); // exceeds buffer
    let pit = XcfPropIter.init(pb, 0);
    expect_eq(pit.next() == null, true);

    // Out-of-bounds layer pointer (> data.len)
    let ptr_buf: [8]u8 = [0; 8];
    let ptb = ptr_buf[0..8];
    putU32BE(*ptb, 0, 5000);
    let lit = XcfPointerIter.init(ptb, 0, false);
    expect_eq(lit.next() == null, true);

    // Out-of-bounds layer name length
    let lbuf: [20]u8 = [0; 20];
    let lb = lbuf[0..20];
    putU32BE(*lb, 12, 500);
    expect_eq(parseXcfLayerHeader(lb, 0, false) == null, true);
});

const PROP_PARASITES_TEST_ID: u32 = 21;

// Source: cp/cap/test/test_psd_production_deep.cp (Production deep tests for cap:psd)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PSD_HEADER_SIZE,
    PSD_VERSION,
    PSB_VERSION,
    PSD_RES_RESOLUTION_INFO,
    PSD_RES_ICC_PROFILE,
    PSD_RES_XMP_METADATA,
    PsdColorMode,
    PsdCompression,
    PsdHeader,
    PsdSections,
    PsdResource,
    isPsd,
    parsePsdHeader,
    parsePsdSections,
    PsdResourceIter,
    findPsdResource,
} from "cap:image";

fn putU16BE(buf: []u8, off: usize, val: u16): usize {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
    return off + 2;
}

fn putU32BE(buf: []u8, off: usize, val: u32): usize {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
    return off + 4;
}

fn putU64BE(buf: []u8, off: usize, val: u64): usize {
    let p = putU32BE(buf, off, ((val >> 32) & 0xFFFF_FFFF) as u32);
    return putU32BE(buf, p, (val & 0xFFFF_FFFF) as u32);
}

fn putBytes(buf: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn writePsdHeader(buf: []u8, ver: u16, ch: u16, h: u32, w: u32, depth: u16, cm: u16) {
    buf[0] = '8'; buf[1] = 'B'; buf[2] = 'P'; buf[3] = 'S';
    * = putU16BE(buf, 4, ver);
    let i: usize = 6;
    while (i < 12) {
        buf[i] = 0;
        i += 1;
    }
    * = putU16BE(buf, 12, ch);
    * = putU32BE(buf, 14, h);
    * = putU32BE(buf, 18, w);
    * = putU16BE(buf, 22, depth);
    * = putU16BE(buf, 24, cm);
}

fn write8BimResource(buf: []u8, off: usize, id: u16, name: []const u8, payload: []const u8): usize {
    let p = putBytes(buf, off, "8BIM");
    p = putU16BE(buf, p, id);
    buf[p] = name.len as u8;
    p += 1;
    p = putBytes(buf, p, name);
    if (((1 + name.len) & 1) != 0) {
        buf[p] = 0;
        p += 1;
    }
    p = putU32BE(buf, p, payload.len as u32);
    p = putBytes(buf, p, payload);
    if ((payload.len & 1) != 0) {
        buf[p] = 0;
        p += 1;
    }
    return p;
}

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. Standard PSD v1 RGB 8-bit full 5-section parsing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    writePsdHeader(buf[0..], PSD_VERSION, 4, 1080, 1920, 8, 3);
    let p: usize = PSD_HEADER_SIZE;

    // Color mode data: 0 bytes
    p = putU32BE(buf[0..], p, 0);
    // Image resources: 0 bytes
    p = putU32BE(buf[0..], p, 0);
    // Layer and mask: 4 bytes dummy
    p = putU32BE(buf[0..], p, 4);
    buf[p] = 0xAA; buf[p + 1] = 0xBB; buf[p + 2] = 0xCC; buf[p + 3] = 0xDD;
    p += 4;
    // Image data: RLE compression (1) + 3 bytes payload
    p = putU16BE(buf[0..], p, 1);
    buf[p] = 0x11; buf[p + 1] = 0x22; buf[p + 2] = 0x33;
    p += 3;

    expect_eq(isPsd(buf[0..p]), true);
    const sec = parsePsdSections(buf[0..p]) ?? unreachable;
    expect_eq(sec.header.version, 1);
    expect_eq(sec.header.isPsb(), false);
    expect_eq(sec.header.channels, 4);
    expect_eq(sec.header.height, 1080);
    expect_eq(sec.header.width, 1920);
    expect_eq(sec.header.depth, 8);
    expect_eq(sec.header.color_mode, PsdColorMode.rgb);
    expect_eq(sec.color_mode_data.len, 0);
    expect_eq(sec.image_resources.len, 0);
    expect_eq(sec.layer_and_mask.len, 4);
    expect_eq(sec.layer_and_mask[0], 0xAA);
    expect_eq(sec.image_data_compression, PsdCompression.rle);
    expect_eq(sec.image_data.len, 3);
    expect_eq(sec.image_data[2], 0x33);
});

test("2. PSB v2 Large Document 64-bit Layer/Mask length and CMYK 16-bit parsing", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    writePsdHeader(buf[0..], PSB_VERSION, 4, 40000, 50000, 16, 4);
    let p: usize = PSD_HEADER_SIZE;

    // Color mode data: 0
    p = putU32BE(buf[0..], p, 0);
    // Image resources: 0
    p = putU32BE(buf[0..], p, 0);
    // PSB Layer and mask: 8-byte u64 length = 6
    p = putU64BE(buf[0..], p, 6);
    p = putBytes(buf[0..], p, "layers");
    // Image data: ZIP with prediction (3) + 2 bytes
    p = putU16BE(buf[0..], p, 3);
    buf[p] = 0x78; buf[p + 1] = 0x9C;
    p += 2;

    const sec = parsePsdSections(buf[0..p]) ?? unreachable;
    expect_eq(sec.header.isPsb(), true);
    expect_eq(sec.header.width, 50000);
    expect_eq(sec.header.height, 40000);
    expect_eq(sec.header.depth, 16);
    expect_eq(sec.header.color_mode, PsdColorMode.cmyk);
    expect_eq(sliceEq(sec.layer_and_mask, "layers"), true);
    expect_eq(sec.image_data_compression, PsdCompression.zip_pred);
    expect_eq(sec.image_data.len, 2);
});

test("3. 8BIM Image Resource Blocks with odd/even Pascal names and payloads + findPsdResource", () => {
    let res_buf: [128]u8 = std.mem.zeroes([128]u8);
    let rp: usize = 0;
    // Block 0: empty name (0 -> padded to 2B), odd payload 3B (padded to 4B)
    rp = write8BimResource(res_buf[0..], rp, PSD_RES_RESOLUTION_INFO, "", "dpi");
    // Block 1: odd name "a" (1+1=2B even), even payload 4B
    rp = write8BimResource(res_buf[0..], rp, PSD_RES_ICC_PROFILE, "a", "sRGB");
    // Block 2: even name "xm" (1+2=3B -> padded to 4B), odd payload 5B
    rp = write8BimResource(res_buf[0..], rp, PSD_RES_XMP_METADATA, "xm", "<xmp>");

    // Assemble full PSD with these resources
    let psd: [256]u8 = std.mem.zeroes([256]u8);
    writePsdHeader(psd[0..], PSD_VERSION, 3, 100, 200, 8, 3);
    let p: usize = PSD_HEADER_SIZE;
    p = putU32BE(psd[0..], p, 0);
    p = putU32BE(psd[0..], p, rp as u32);
    p = putBytes(psd[0..], p, res_buf[0..rp]);
    p = putU32BE(psd[0..], p, 0);
    p = putU16BE(psd[0..], p, 0);

    const sec = parsePsdSections(psd[0..p]) ?? unreachable;
    let it = PsdResourceIter.init(sec.image_resources);

    const r0 = it.next() ?? unreachable;
    expect_eq(r0.id, PSD_RES_RESOLUTION_INFO);
    expect_eq(r0.name.len, 0);
    expect_eq(sliceEq(r0.payload, "dpi"), true);

    const r1 = it.next() ?? unreachable;
    expect_eq(r1.id, PSD_RES_ICC_PROFILE);
    expect_eq(sliceEq(r1.name, "a"), true);
    expect_eq(sliceEq(r1.payload, "sRGB"), true);

    const r2 = it.next() ?? unreachable;
    expect_eq(r2.id, PSD_RES_XMP_METADATA);
    expect_eq(sliceEq(r2.name, "xm"), true);
    expect_eq(sliceEq(r2.payload, "<xmp>"), true);

    expect_eq(it.next() == null, true);

    const icc = findPsdResource(sec, PSD_RES_ICC_PROFILE) ?? unreachable;
    expect_eq(sliceEq(icc.payload, "sRGB"), true);
    expect_eq(findPsdResource(sec, 0x9999) == null, true);
});

test("4. ColorMode and BitDepth validation across all legal values", () => {
    let hdr: [26]u8 = std.mem.zeroes([26]u8);

    // Bitmap 1-bit
    writePsdHeader(hdr[0..], 1, 1, 64, 64, 1, 0);
    const h1 = parsePsdHeader(hdr[0..]) ?? unreachable;
    expect_eq(h1.color_mode, PsdColorMode.bitmap);
    expect_eq(h1.depth, 1);

    // Lab 32-bit float
    writePsdHeader(hdr[0..], 1, 3, 64, 64, 32, 9);
    const h2 = parsePsdHeader(hdr[0..]) ?? unreachable;
    expect_eq(h2.color_mode, PsdColorMode.lab);
    expect_eq(h2.depth, 32);

    // Illegal depth (24) rejected
    writePsdHeader(hdr[0..], 1, 3, 64, 64, 24, 3);
    expect_eq(parsePsdHeader(hdr[0..]) == null, true);

    // Illegal channels (0 or 57) rejected
    writePsdHeader(hdr[0..], 1, 0, 64, 64, 8, 3);
    expect_eq(parsePsdHeader(hdr[0..]) == null, true);
    writePsdHeader(hdr[0..], 1, 57, 64, 64, 8, 3);
    expect_eq(parsePsdHeader(hdr[0..]) == null, true);
});

test("5. Malformed PSD protection: wrong magic, non-zero reserved, bad version, and truncated sections", () => {
    let hdr: [26]u8 = std.mem.zeroes([26]u8);
    writePsdHeader(hdr[0..], 1, 3, 10, 10, 8, 3);

    // Wrong magic
    let bad_magic = hdr;
    bad_magic[0] = 'X';
    expect_eq(isPsd(bad_magic[0..]), false);

    // Non-zero reserved byte
    let bad_res = hdr;
    bad_res[8] = 1;
    expect_eq(isPsd(bad_res[0..]), false);

    // Invalid version 3
    let bad_ver = hdr;
    bad_ver[5] = 3;
    expect_eq(isPsd(bad_ver[0..]), false);

    // Out-of-bounds Color Mode Data section length
    let oob: [34]u8 = std.mem.zeroes([34]u8);
    writePsdHeader(oob[0..], 1, 3, 10, 10, 8, 3);
    * = putU32BE(oob[0..], 26, 9999);
    expect_eq(parsePsdSections(oob[0..]) == null, true);
});

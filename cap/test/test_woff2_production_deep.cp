// Source: cp/cap/test/test_woff2_production_deep.cp (Production deep tests for cap:woff2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    WOFF2_HEADER_SIZE,
    WOFF2_SIGNATURE,
    WOFF2_FLAVOR_TRUETYPE,
    WOFF2_FLAVOR_CFF,
    WOFF2_FLAVOR_COLLECTION,
    Woff2Header,
    UIntBase128Result,
    Woff2TableEntry,
    decodeUIntBase128,
    isWoff2,
    parseWoff2Header,
    Woff2TableIter,
} from "cap:gfx";

fn writeU16BE(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn makeValidHeader(buf: []u8, flavor: u32, total_len: u32, num_tables: u16) {
    writeU32BE(buf, 0, WOFF2_SIGNATURE);
    writeU32BE(buf, 4, flavor);
    writeU32BE(buf, 8, total_len);
    writeU16BE(buf, 12, num_tables);
    writeU16BE(buf, 14, 0); // reserved
    writeU32BE(buf, 16, 4096); // total_sfnt_size
    writeU32BE(buf, 20, 512); // total_compressed_size
    writeU16BE(buf, 24, 1); // major_version
    writeU16BE(buf, 26, 2); // minor_version
    writeU32BE(buf, 28, 0); // meta_offset
    writeU32BE(buf, 32, 0); // meta_length
    writeU32BE(buf, 36, 0); // meta_orig_length
    writeU32BE(buf, 40, 0); // priv_offset
    writeU32BE(buf, 44, 0); // priv_length
}

test("1. decodeUIntBase128 enforces W3C WOFF2 section 4.1 limits and rejects non-minimal/overflow encodings", () => {
    const b0: [1]u8 = [0x00];
    const r0 = decodeUIntBase128(b0[0..]) ?? unreachable;
    expect_eq(r0.value, 0);
    expect_eq(r0.bytes_read, 1);

    const b1: [1]u8 = [0x7F];
    const r1 = decodeUIntBase128(b1[0..]) ?? unreachable;
    expect_eq(r1.value, 127);
    expect_eq(r1.bytes_read, 1);

    const b2: [2]u8 = [0x81, 0x00];
    const r2 = decodeUIntBase128(b2[0..]) ?? unreachable;
    expect_eq(r2.value, 128);
    expect_eq(r2.bytes_read, 2);

    // Max u32: 0xFFFFFFFF -> 0x8F 0xFF 0xFF 0xFF 0x7F
    const bmax: [5]u8 = [0x8F, 0xFF, 0xFF, 0xFF, 0x7F];
    const rmax = decodeUIntBase128(bmax[0..]) ?? unreachable;
    expect_eq(rmax.value, 0xFFFF_FFFF);
    expect_eq(rmax.bytes_read, 5);

    // Reject empty, leading 0x80, u32 overflow, >5 bytes, and truncated continuation
    expect_eq(decodeUIntBase128(b0[0..0]) == null, true);

    const lead80: [2]u8 = [0x80, 0x01];
    expect_eq(decodeUIntBase128(lead80[0..]) == null, true);

    const overflow: [5]u8 = [0x90, 0x80, 0x80, 0x80, 0x00];
    expect_eq(decodeUIntBase128(overflow[0..]) == null, true);

    const too_long: [6]u8 = [0x81, 0x81, 0x81, 0x81, 0x81, 0x00];
    expect_eq(decodeUIntBase128(too_long[0..]) == null, true);

    const truncated: [2]u8 = [0x81, 0x82];
    expect_eq(decodeUIntBase128(truncated[0..]) == null, true);
});

test("2. isWoff2 and parseWoff2Header decode TrueType, CFF, Collection, and metadata headers", () => {
    let buf: [96]u8 = std.mem.zeroes([96]u8);
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 96, 4);
    writeU32BE(buf[0..], 28, 64); // meta_offset
    writeU32BE(buf[0..], 32, 16); // meta_length
    writeU32BE(buf[0..], 36, 40); // meta_orig_length
    writeU32BE(buf[0..], 40, 80); // priv_offset
    writeU32BE(buf[0..], 44, 16); // priv_length

    expect_eq(isWoff2(buf[0..]), true);
    const h1 = parseWoff2Header(buf[0..]) ?? unreachable;
    expect_eq(h1.signature, WOFF2_SIGNATURE);
    expect_eq(h1.isTrueType(), true);
    expect_eq(h1.isCff(), false);
    expect_eq(h1.isCollection(), false);
    expect_eq(h1.hasMetadata(), true);
    expect_eq(h1.num_tables, 4);
    expect_eq(h1.major_version, 1);
    expect_eq(h1.minor_version, 2);
    expect_eq(h1.meta_offset, 64);
    expect_eq(h1.priv_length, 16);

    makeValidHeader(buf[0..], WOFF2_FLAVOR_CFF, 64, 2);
    const h2 = parseWoff2Header(buf[0..]) ?? unreachable;
    expect_eq(h2.isCff(), true);
    expect_eq(h2.hasMetadata(), false);

    makeValidHeader(buf[0..], WOFF2_FLAVOR_COLLECTION, 64, 3);
    const h3 = parseWoff2Header(buf[0..]) ?? unreachable;
    expect_eq(h3.isCollection(), true);
});

test("3. parseWoff2Header rejects invalid magic, reserved, zero tables, and out-of-bounds/overflow offsets", () => {
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    expect_eq(isWoff2(buf[0..3]), false);
    expect_eq(parseWoff2Header(buf[0..47]) == null, true);

    // Wrong signature
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 64, 1);
    buf[3] = '1';
    expect_eq(isWoff2(buf[0..]), false);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);

    // Non-zero reserved
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 64, 1);
    writeU16BE(buf[0..], 14, 1);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);

    // Zero num_tables
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 64, 0);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);

    // Declared length exceeds buffer
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 128, 1);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);

    // Metadata bounds overflow (0xFFFF_FFF0 + 32 wraps u32 if unchecked)
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 64, 1);
    writeU32BE(buf[0..], 28, 0xFFFF_FFF0);
    writeU32BE(buf[0..], 32, 32);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);

    // Private block out of bounds
    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, 64, 1);
    writeU32BE(buf[0..], 40, 60);
    writeU32BE(buf[0..], 44, 8);
    expect_eq(parseWoff2Header(buf[0..]) == null, true);
});

test("4. Woff2TableIter decodes known tags, custom 4-byte tags, and glyf/loca/hmtx transform lengths", () => {
    let buf: [96]u8 = std.mem.zeroes([96]u8);
    let pos: usize = WOFF2_HEADER_SIZE;

    // Entry 0: known tag 0 ("cmap"), transform_version=0 (null transform), orig_length=100
    buf[pos] = 0; pos += 1;
    buf[pos] = 100; pos += 1;

    // Entry 1: known tag 10 ("glyf"), transform_version=0 (transformed!), orig_length=256 (0x82 0x00), transform_length=120
    buf[pos] = 10; pos += 1;
    buf[pos] = 0x82; pos += 1;
    buf[pos] = 0x00; pos += 1;
    buf[pos] = 120; pos += 1;

    // Entry 2: known tag 11 ("loca"), transform_version=3 (null transform for loca!), flags = (3<<6)|11 = 0xCB, orig_length=64
    buf[pos] = 0xCB; pos += 1;
    buf[pos] = 64; pos += 1;

    // Entry 3: known tag 3 ("hmtx"), transform_version=1 (transformed), flags = (1<<6)|3 = 0x43, orig_length=80, transform_length=40
    buf[pos] = 0x43; pos += 1;
    buf[pos] = 80; pos += 1;
    buf[pos] = 40; pos += 1;

    // Entry 4: custom tag (idx 63 = 0x3F), tag="DSIG", transform_version=0, orig_length=12
    buf[pos] = 0x3F; pos += 1;
    buf[pos] = 'D'; pos += 1;
    buf[pos] = 'S'; pos += 1;
    buf[pos] = 'I'; pos += 1;
    buf[pos] = 'G'; pos += 1;
    buf[pos] = 12; pos += 1;

    makeValidHeader(buf[0..], WOFF2_FLAVOR_TRUETYPE, pos as u32, 5);

    let it = Woff2TableIter.init(buf[0..pos]);

    const e0 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e0.tag[0..], "cmap"), true);
    expect_eq(e0.orig_length, 100);
    expect_eq(e0.transform_length == null, true);

    const e1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e1.tag[0..], "glyf"), true);
    expect_eq(e1.transform_version, 0);
    expect_eq(e1.orig_length, 256);
    expect_eq(e1.transform_length ?? 0, 120);

    const e2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e2.tag[0..], "loca"), true);
    expect_eq(e2.transform_version, 3);
    expect_eq(e2.orig_length, 64);
    expect_eq(e2.transform_length == null, true);

    const e3 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e3.tag[0..], "hmtx"), true);
    expect_eq(e3.transform_version, 1);
    expect_eq(e3.orig_length, 80);
    expect_eq(e3.transform_length ?? 0, 40);

    const e4 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, e4.tag[0..], "DSIG"), true);
    expect_eq(e4.orig_length, 12);
    expect_eq(e4.transform_length == null, true);

    expect_eq(it.next() == null, true);
});

test("5. Woff2TableIter handles invalid header, truncated custom tags, and reserved glyf transform safely", () => {
    // Invalid header yields immediate null
    let bad: [16]u8 = std.mem.zeroes([16]u8);
    let it0 = Woff2TableIter.init(bad[0..]);
    expect_eq(it0.next() == null, true);

    // Truncated custom 4-byte tag
    let trunc: [51]u8 = std.mem.zeroes([51]u8);
    makeValidHeader(trunc[0..], WOFF2_FLAVOR_TRUETYPE, 51, 1);
    trunc[48] = 0x3F;
    trunc[49] = 'A';
    trunc[50] = 'B';
    let it1 = Woff2TableIter.init(trunc[0..]);
    expect_eq(it1.next() == null, true);

    // Reserved transform_version=1 on glyf (flags = (1<<6)|10 = 0x4A) is rejected cleanly
    let resv: [52]u8 = std.mem.zeroes([52]u8);
    makeValidHeader(resv[0..], WOFF2_FLAVOR_TRUETYPE, 52, 1);
    resv[48] = 0x4A;
    resv[49] = 10;
    let it2 = Woff2TableIter.init(resv[0..]);
    expect_eq(it2.next() == null, true);
});

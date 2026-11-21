// Source: cp/cap/test/test_heif_production_deep.cp (Production deep tests for cap:heif)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    HEIF_HVCC_MIN_SIZE,
    HeifBrandInfo,
    HevcConfig,
    HeifColr,
    HeifInfo,
    isHeif,
    parseHeifBrand,
    parseHvcC,
    parseHeifInfo,
} from "cap:heif";

fn writeU16BE(buf: []u8, off: usize, val: u16) {
    buf[off] = ((val >> 8) & 0xFF) as u8;
    buf[off + 1] = (val & 0xFF) as u8;
}

fn writeU32BE(buf: []u8, off: usize, val: u32) {
    buf[off] = ((val >> 24) & 0xFF) as u8;
    buf[off + 1] = ((val >> 16) & 0xFF) as u8;
    buf[off + 2] = ((val >> 8) & 0xFF) as u8;
    buf[off + 3] = (val & 0xFF) as u8;
}

fn writeFourcc(buf: []u8, off: usize, a: u8, b: u8, c: u8, d: u8) {
    buf[off] = a;
    buf[off + 1] = b;
    buf[off + 2] = c;
    buf[off + 3] = d;
}

fn writeBoxHeader(buf: []u8, off: usize, total_size: usize, a: u8, b: u8, c: u8, d: u8) {
    writeU32BE(buf, off, total_size as u32);
    writeFourcc(buf, off + 4, a, b, c, d);
}

fn copySlice(dst: []u8, off: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
    return off + src.len;
}

fn buildHvcC(
    out: []u8,
    profile_space: u8,
    tier_flag: bool,
    profile_idc: u8,
    level_idc: u8,
    chroma_fmt: u8,
    luma_minus_8: u8,
    chroma_minus_8: u8,
    len_size_minus_1: u8,
    num_arrays: u8,
) {
    out[0] = 1; // configurationVersion == 1
    let b1: u8 = ((profile_space & 0x03) << 6) | (profile_idc & 0x1F);
    if (tier_flag) {
        b1 = b1 | 0x20;
    }
    out[1] = b1;
    out[12] = level_idc;
    out[16] = (0xFC as u8) | (chroma_fmt & 0x03);
    out[17] = (0xF8 as u8) | (luma_minus_8 & 0x07);
    out[18] = (0xF8 as u8) | (chroma_minus_8 & 0x07);
    out[21] = (0xFC as u8) | (len_size_minus_1 & 0x03);
    out[22] = num_arrays;
}

test("1. iPhone-style HEIC Main 8-bit and Main10 10-bit hvcC decoding", () => {
    // Main Profile (1), Tier 0, Level 4.0 (120), 4:2:0 (1), 8-bit luma/chroma, 4-byte NALU len (3), 3 arrays (VPS/SPS/PPS)
    let hvcc8: [23]u8 = std.mem.zeroes([23]u8);
    buildHvcC(hvcc8[0..], 0, false, 1, 120, 1, 0, 0, 3, 3);
    const c8 = parseHvcC(hvcc8[0..]) ?? unreachable;
    expect_eq(c8.configuration_version, 1);
    expect_eq(c8.general_profile_space, 0);
    expect_eq(c8.general_tier_flag, false);
    expect_eq(c8.general_profile_idc, 1);
    expect_eq(c8.general_level_idc, 120);
    expect_eq(c8.chroma_format_idc, 1);
    expect_eq(c8.bit_depth_luma, 8);
    expect_eq(c8.bit_depth_chroma, 8);
    expect_eq(c8.length_size_minus_one, 3);
    expect_eq(c8.num_of_arrays, 3);

    // Main10 Profile (2), High Tier (true), Level 5.0 (150), 4:2:2 (2), 10-bit luma/chroma
    let hvcc10: [23]u8 = std.mem.zeroes([23]u8);
    buildHvcC(hvcc10[0..], 0, true, 2, 150, 2, 2, 2, 3, 4);
    const c10 = parseHvcC(hvcc10[0..]) ?? unreachable;
    expect_eq(c10.general_tier_flag, true);
    expect_eq(c10.general_profile_idc, 2);
    expect_eq(c10.general_level_idc, 150);
    expect_eq(c10.chroma_format_idc, 2);
    expect_eq(c10.bit_depth_luma, 10);
    expect_eq(c10.bit_depth_chroma, 10);
    expect_eq(c10.num_of_arrays, 4);

    // Invalid hvcC: wrong version (!= 1) or truncated (< 23B)
    let bad_ver = hvcc8;
    bad_ver[0] = 2;
    expect_eq(parseHvcC(bad_ver[0..]) == null, true);
    expect_eq(parseHvcC(hvcc8[0..(HEIF_HVCC_MIN_SIZE - 1)]) == null, true);
});

test("2. Full HEIC container parses ispe, pixi, hvcC, colr, irot, and auxC alpha + depth", () => {
    let buf: [320]u8 = std.mem.zeroes([320]u8);

    // ftyp: major="heic", minor=0, compat=["mif1", "heic"] -> 24B
    writeBoxHeader(buf[0..], 0, 24, 'f', 't', 'y', 'p');
    writeFourcc(buf[0..], 8, 'h', 'e', 'i', 'c');
    writeU32BE(buf[0..], 12, 0);
    writeFourcc(buf[0..], 16, 'm', 'i', 'f', '1');
    writeFourcc(buf[0..], 20, 'h', 'e', 'i', 'c');

    const ipco_s: usize = 44;
    let p: usize = ipco_s + 8;

    // ispe: 4032x3024
    writeBoxHeader(buf[0..], p, 20, 'i', 's', 'p', 'e');
    writeU32BE(buf[0..], p + 12, 4032);
    writeU32BE(buf[0..], p + 16, 3024);
    p += 20;

    // pixi: 3 channels, 10-bit
    writeBoxHeader(buf[0..], p, 16, 'p', 'i', 'x', 'i');
    buf[p + 12] = 3;
    buf[p + 13] = 10;
    buf[p + 14] = 10;
    buf[p + 15] = 10;
    p += 16;

    // hvcC: 8B hdr + 23B payload = 31B
    writeBoxHeader(buf[0..], p, 31, 'h', 'v', 'c', 'C');
    buildHvcC(buf[(p + 8)..(p + 31)], 0, false, 2, 123, 1, 2, 2, 3, 3);
    p += 31;

    // colr: nclx Display P3 (prim=12, trc=13, mat=6, full_range=true)
    writeBoxHeader(buf[0..], p, 19, 'c', 'o', 'l', 'r');
    writeFourcc(buf[0..], p + 8, 'n', 'c', 'l', 'x');
    writeU16BE(buf[0..], p + 12, 12);
    writeU16BE(buf[0..], p + 14, 13);
    writeU16BE(buf[0..], p + 16, 6);
    buf[p + 18] = 0x80;
    p += 19;

    // irot: angle 2 => 180 deg
    writeBoxHeader(buf[0..], p, 9, 'i', 'r', 'o', 't');
    buf[p + 8] = 2;
    p += 9;

    // auxC alpha (HEVC auxid:1)
    const alpha_urn = "urn:mpeg:hevc:2015:auxid:1";
    const alpha_sz: usize = alpha_urn.len + 13;
    writeBoxHeader(buf[0..], p, alpha_sz, 'a', 'u', 'x', 'C');
    const a_end = copySlice(buf[0..], p + 12, alpha_urn);
    buf[a_end] = 0;
    p += alpha_sz;

    // auxC depth (HEVC auxid:2)
    const depth_urn = "urn:mpeg:hevc:2015:auxid:2";
    const depth_sz: usize = depth_urn.len + 13;
    writeBoxHeader(buf[0..], p, depth_sz, 'a', 'u', 'x', 'C');
    const d_end = copySlice(buf[0..], p + 12, depth_urn);
    buf[d_end] = 0;
    p += depth_sz;

    writeBoxHeader(buf[0..], ipco_s, p - ipco_s, 'i', 'p', 'c', 'o');
    writeBoxHeader(buf[0..], 36, p - 36, 'i', 'p', 'r', 'p');
    writeBoxHeader(buf[0..], 24, p - 24, 'm', 'e', 't', 'a');

    expect_eq(isHeif(buf[0..p]), true);
    const info = parseHeifInfo(buf[0..p]) ?? unreachable;
    expect_eq(info.brand.is_heic, true);
    expect_eq(info.brand.is_sequence, false);
    expect_eq(info.width, 4032);
    expect_eq(info.height, 3024);
    expect_eq(info.pixi_channels, 3);
    expect_eq(info.pixi_bit_depth, 10);
    expect_eq(info.rotation_degrees, 180);
    expect_eq(info.has_alpha, true);
    expect_eq(info.has_depth, true);
    expect_eq(info.has_icc_profile, false);

    const hv = info.hvcc ?? unreachable;
    expect_eq(hv.general_profile_idc, 2);
    expect_eq(hv.general_level_idc, 123);
    expect_eq(hv.bit_depth_luma, 10);

    const co = info.colr ?? unreachable;
    expect_eq(co.color_primaries, 12);
    expect_eq(co.transfer_characteristics, 13);
    expect_eq(co.matrix_coefficients, 6);
    expect_eq(co.full_range, true);
});

test("3. msf1 / hevc image sequence brand and ICC profile flag detection", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    // ftyp: major="msf1", minor=2, compat=["hevc"]
    writeBoxHeader(buf[0..], 0, 20, 'f', 't', 'y', 'p');
    writeFourcc(buf[0..], 8, 'm', 's', 'f', '1');
    writeU32BE(buf[0..], 12, 2);
    writeFourcc(buf[0..], 16, 'h', 'e', 'v', 'c');

    const ipco_s: usize = 40;
    let p: usize = ipco_s + 8;

    writeBoxHeader(buf[0..], p, 20, 'i', 's', 'p', 'e');
    writeU32BE(buf[0..], p + 12, 1280);
    writeU32BE(buf[0..], p + 16, 720);
    p += 20;

    writeBoxHeader(buf[0..], p, 16, 'c', 'o', 'l', 'r');
    writeFourcc(buf[0..], p + 8, 'p', 'r', 'o', 'f');
    p += 16;

    writeBoxHeader(buf[0..], ipco_s, p - ipco_s, 'i', 'p', 'c', 'o');
    writeBoxHeader(buf[0..], 32, p - 32, 'i', 'p', 'r', 'p');
    writeBoxHeader(buf[0..], 20, p - 20, 'm', 'e', 't', 'a');

    expect_eq(isHeif(buf[0..p]), true);
    const info = parseHeifInfo(buf[0..p]) ?? unreachable;
    expect_eq(info.brand.is_sequence, true);
    expect_eq(info.brand.is_heic, false);
    expect_eq(info.brand.minor_version, 2);
    expect_eq(info.width, 1280);
    expect_eq(info.height, 720);
    expect_eq(info.has_icc_profile, true);
});

test("4. Pure AVIF/AVIS and plain MP4 containers are rejected by isHeif/parseHeifBrand", () => {
    // Pure AVIF with mif1 compatible brand must be rejected
    let avif: [24]u8 = std.mem.zeroes([24]u8);
    writeBoxHeader(avif[0..], 0, 24, 'f', 't', 'y', 'p');
    writeFourcc(avif[0..], 8, 'a', 'v', 'i', 'f');
    writeFourcc(avif[0..], 16, 'm', 'i', 'f', '1');
    writeFourcc(avif[0..], 20, 'm', 'i', 'a', 'f');
    expect_eq(isHeif(avif[0..]), false);
    expect_eq(parseHeifBrand(avif[0..]) == null, true);

    // Pure AVIS sequence with mif1 must be rejected
    let avis: [20]u8 = std.mem.zeroes([20]u8);
    writeBoxHeader(avis[0..], 0, 20, 'f', 't', 'y', 'p');
    writeFourcc(avis[0..], 8, 'm', 'i', 'f', '1');
    writeFourcc(avis[0..], 16, 'a', 'v', 'i', 's');
    expect_eq(isHeif(avis[0..]), false);

    // Plain MP4 isom/mp41 rejected
    let mp4: [20]u8 = std.mem.zeroes([20]u8);
    writeBoxHeader(mp4[0..], 0, 20, 'f', 't', 'y', 'p');
    writeFourcc(mp4[0..], 8, 'i', 's', 'o', 'm');
    writeFourcc(mp4[0..], 16, 'm', 'p', '4', '1');
    expect_eq(isHeif(mp4[0..]), false);
});

test("5. Malformed, truncated, and out-of-bounds BMFF boxes fail safely", () => {
    let empty: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(isHeif(empty[0..0]), false);
    expect_eq(isHeif(empty[0..4]), false);

    // Out-of-bounds ftyp box
    let oob: [20]u8 = std.mem.zeroes([20]u8);
    writeBoxHeader(oob[0..], 0, 999, 'f', 't', 'y', 'p');
    writeFourcc(oob[0..], 8, 'h', 'e', 'i', 'c');
    expect_eq(isHeif(oob[0..]), false);
    expect_eq(parseHeifInfo(oob[0..]) == null, true);

    // Valid ftyp + truncated meta box
    let trunc: [28]u8 = std.mem.zeroes([28]u8);
    writeBoxHeader(trunc[0..], 0, 16, 'f', 't', 'y', 'p');
    writeFourcc(trunc[0..], 8, 'h', 'e', 'i', 'c');
    writeBoxHeader(trunc[0..], 16, 80, 'm', 'e', 't', 'a');
    expect_eq(isHeif(trunc[0..]), true);
    expect_eq(parseHeifInfo(trunc[0..]) == null, true);

    // Valid ftyp + meta box with < 4B payload (missing FullBox header)
    let tiny_meta: [26]u8 = std.mem.zeroes([26]u8);
    writeBoxHeader(tiny_meta[0..], 0, 16, 'f', 't', 'y', 'p');
    writeFourcc(tiny_meta[0..], 8, 'h', 'e', 'i', 'c');
    writeBoxHeader(tiny_meta[0..], 16, 10, 'm', 'e', 't', 'a');
    expect_eq(parseHeifInfo(tiny_meta[0..]) == null, true);
});

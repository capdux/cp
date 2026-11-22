// Source: cp/cap/test/test_avif_production_deep.cp (Production deep tests for cap:avif)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    AvifBrandInfo,
    Av1Config,
    AvifColr,
    AvifInfo,
    isAvif,
    parseAvifBrand,
    parseAv1C,
    parseAvifInfo,
} from "cap:image";

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

test("1. Full AVIF container parses ftyp + meta/iprp/ipco (ispe, pixi, av1C, colr, irot)", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    // ftyp: 8B hdr + 4B major("avif") + 4B minor(0) + 4B "mif1" + 4B "miaf" = 24B
    writeBoxHeader(buf[0..], 0, 24, 'f', 't', 'y', 'p');
    writeFourcc(buf[0..], 8, 'a', 'v', 'i', 'f');
    writeU32BE(buf[0..], 12, 0);
    writeFourcc(buf[0..], 16, 'm', 'i', 'f', '1');
    writeFourcc(buf[0..], 20, 'm', 'i', 'a', 'f');

    // Build ipco children starting at offset 24 + 12(meta) + 8(iprp) + 8(ipco) = 52
    const ipco_start: usize = 44;
    let p: usize = ipco_start + 8;

    // ispe: 8B hdr + 4B fullbox + 4B width(1920) + 4B height(1080) = 20B
    writeBoxHeader(buf[0..], p, 20, 'i', 's', 'p', 'e');
    writeU32BE(buf[0..], p + 12, 1920);
    writeU32BE(buf[0..], p + 16, 1080);
    p += 20;

    // pixi: 8B hdr + 4B fullbox + 1B num_ch(3) + 3B depths(8,8,8) = 16B
    writeBoxHeader(buf[0..], p, 16, 'p', 'i', 'x', 'i');
    buf[p + 12] = 3;
    buf[p + 13] = 8;
    buf[p + 14] = 8;
    buf[p + 15] = 8;
    p += 16;

    // av1C: 8B hdr + 4B config (marker=1, ver=1, profile=0 Main, level=4, tier=0, 8-bit, 4:2:0) = 12B
    writeBoxHeader(buf[0..], p, 12, 'a', 'v', '1', 'C');
    buf[p + 8] = 0x81;  // marker=1, version=1
    buf[p + 9] = 0x04;  // profile=0, level=4
    buf[p + 10] = 0x0C; // tier=0, high_bitdepth=0, twelve=0, mono=0, sub_x=1, sub_y=1
    buf[p + 11] = 0x00;
    p += 12;

    // colr: 8B hdr + 4B "nclx" + 2B prim(1 BT.709) + 2B trc(13 sRGB) + 2B mat(1) + 1B full_range(0x80) = 19B
    writeBoxHeader(buf[0..], p, 19, 'c', 'o', 'l', 'r');
    writeFourcc(buf[0..], p + 8, 'n', 'c', 'l', 'x');
    writeU16BE(buf[0..], p + 12, 1);
    writeU16BE(buf[0..], p + 14, 13);
    writeU16BE(buf[0..], p + 16, 1);
    buf[p + 18] = 0x80;
    p += 19;

    // irot: 8B hdr + 1B angle_code(1 => 90 deg) = 9B
    writeBoxHeader(buf[0..], p, 9, 'i', 'r', 'o', 't');
    buf[p + 8] = 1;
    p += 9;

    const ipco_size = p - ipco_start;
    writeBoxHeader(buf[0..], ipco_start, ipco_size, 'i', 'p', 'c', 'o');

    const iprp_start: usize = 36;
    writeBoxHeader(buf[0..], iprp_start, p - iprp_start, 'i', 'p', 'r', 'p');

    const meta_start: usize = 24;
    writeBoxHeader(buf[0..], meta_start, p - meta_start, 'm', 'e', 't', 'a');

    const total = p;
    expect_eq(isAvif(buf[0..total]), true);

    const info = parseAvifInfo(buf[0..total]) ?? unreachable;
    expect_eq(info.brand.major_brand[0], 'a');
    expect_eq(info.brand.major_brand[3], 'f');
    expect_eq(info.brand.is_sequence, false);
    expect_eq(info.brand.has_mif1, true);
    expect_eq(info.brand.has_miaf, true);
    expect_eq(info.width, 1920);
    expect_eq(info.height, 1080);
    expect_eq(info.pixi_channels, 3);
    expect_eq(info.pixi_bit_depth, 8);
    expect_eq(info.rotation_degrees, 90);
    expect_eq(info.has_alpha, false);
    expect_eq(info.has_icc_profile, false);

    const av1c = info.av1c ?? unreachable;
    expect_eq(av1c.marker_ok, true);
    expect_eq(av1c.version, 1);
    expect_eq(av1c.seq_profile, 0);
    expect_eq(av1c.seq_level_idx_0, 4);
    expect_eq(av1c.high_bitdepth, false);
    expect_eq(av1c.twelve_bit, false);
    expect_eq(av1c.monochrome, false);
    expect_eq(av1c.chroma_subsampling_x, true);
    expect_eq(av1c.chroma_subsampling_y, true);
    expect_eq(av1c.bitDepth(), 8);

    const colr = info.colr ?? unreachable;
    expect_eq(colr.color_type[0], 'n');
    expect_eq(colr.color_primaries, 1);
    expect_eq(colr.transfer_characteristics, 13);
    expect_eq(colr.matrix_coefficients, 1);
    expect_eq(colr.full_range, true);
});

test("2. 10-bit and 12-bit HDR av1C bitDepth() and BT.2020 PQ nclx color metadata", () => {
    // 10-bit Main Profile (0), level 8, tier 1, high_bitdepth=1, twelve_bit=0, 4:2:0
    const av1c_10: [4]u8 = [0x81, 0x08, 0xCC, 0x00];
    const c10 = parseAv1C(av1c_10[0..]) ?? unreachable;
    expect_eq(c10.marker_ok, true);
    expect_eq(c10.seq_profile, 0);
    expect_eq(c10.seq_level_idx_0, 8);
    expect_eq(c10.seq_tier_0, 1);
    expect_eq(c10.high_bitdepth, true);
    expect_eq(c10.twelve_bit, false);
    expect_eq(c10.bitDepth(), 10);

    // 12-bit Professional Profile (2), level 12, high_bitdepth=1, twelve_bit=1, monochrome=1
    // b1 = (2 << 5) | 12 = 0x4C; b2 = 0x40 | 0x20 | 0x10 = 0x70
    const av1c_12: [4]u8 = [0x81, 0x4C, 0x70, 0x00];
    const c12 = parseAv1C(av1c_12[0..]) ?? unreachable;
    expect_eq(c12.seq_profile, 2);
    expect_eq(c12.seq_level_idx_0, 12);
    expect_eq(c12.high_bitdepth, true);
    expect_eq(c12.twelve_bit, true);
    expect_eq(c12.monochrome, true);
    expect_eq(c12.chroma_subsampling_x, false);
    expect_eq(c12.chroma_subsampling_y, false);
    expect_eq(c12.bitDepth(), 12);

    // Truncated av1C (< 4B) returns null
    const short_av1c: [3]u8 = [0x81, 0x04, 0x0C];
    expect_eq(parseAv1C(short_av1c[0..]) == null, true);

    // Embed 10-bit HDR BT.2020 (prim=9) + PQ (trc=16) + BT.2020 NCL (mat=9) in full AVIF
    let hdr: [160]u8 = std.mem.zeroes([160]u8);
    writeBoxHeader(hdr[0..], 0, 16, 'f', 't', 'y', 'p');
    writeFourcc(hdr[0..], 8, 'a', 'v', 'i', 'f');

    const ipco_s: usize = 36;
    let p: usize = ipco_s + 8;

    writeBoxHeader(hdr[0..], p, 20, 'i', 's', 'p', 'e');
    writeU32BE(hdr[0..], p + 12, 3840);
    writeU32BE(hdr[0..], p + 16, 2160);
    p += 20;

    writeBoxHeader(hdr[0..], p, 12, 'a', 'v', '1', 'C');
    p = copySlice(hdr[0..], p + 8, av1c_10[0..]);

    writeBoxHeader(hdr[0..], p, 19, 'c', 'o', 'l', 'r');
    writeFourcc(hdr[0..], p + 8, 'n', 'c', 'l', 'x');
    writeU16BE(hdr[0..], p + 12, 9);
    writeU16BE(hdr[0..], p + 14, 16);
    writeU16BE(hdr[0..], p + 16, 9);
    hdr[p + 18] = 0x00; // limited range
    p += 19;

    writeBoxHeader(hdr[0..], ipco_s, p - ipco_s, 'i', 'p', 'c', 'o');
    writeBoxHeader(hdr[0..], 28, p - 28, 'i', 'p', 'r', 'p');
    writeBoxHeader(hdr[0..], 16, p - 16, 'm', 'e', 't', 'a');

    const info = parseAvifInfo(hdr[0..p]) ?? unreachable;
    expect_eq(info.width, 3840);
    expect_eq(info.height, 2160);
    const av = info.av1c ?? unreachable;
    expect_eq(av.bitDepth(), 10);
    const co = info.colr ?? unreachable;
    expect_eq(co.color_primaries, 9);
    expect_eq(co.transfer_characteristics, 16);
    expect_eq(co.matrix_coefficients, 9);
    expect_eq(co.full_range, false);
});

test("3. avis animated sequence brand, ICC profile flag, 270-deg irot, and auxC alpha detection", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);

    // ftyp with major="mif1", compatible=["avis", "miaf"] -> 24B
    writeBoxHeader(buf[0..], 0, 24, 'f', 't', 'y', 'p');
    writeFourcc(buf[0..], 8, 'm', 'i', 'f', '1');
    writeU32BE(buf[0..], 12, 1);
    writeFourcc(buf[0..], 16, 'a', 'v', 'i', 's');
    writeFourcc(buf[0..], 20, 'm', 'i', 'a', 'f');

    const ipco_s: usize = 44;
    let p: usize = ipco_s + 8;

    // ispe: 512x512
    writeBoxHeader(buf[0..], p, 20, 'i', 's', 'p', 'e');
    writeU32BE(buf[0..], p + 12, 512);
    writeU32BE(buf[0..], p + 16, 512);
    p += 20;

    // colr with rICC (ICC profile)
    writeBoxHeader(buf[0..], p, 16, 'c', 'o', 'l', 'r');
    writeFourcc(buf[0..], p + 8, 'r', 'I', 'C', 'C');
    p += 16;

    // irot: code 3 => 270 deg
    writeBoxHeader(buf[0..], p, 9, 'i', 'r', 'o', 't');
    buf[p + 8] = 3;
    p += 9;

    // auxC with AVIF alpha URN + NUL
    const alpha_urn = "urn:mpeg:mpegB:cicp:systems:auxiliary:alpha";
    const auxc_size: usize = alpha_urn.len + 13;
    writeBoxHeader(buf[0..], p, auxc_size, 'a', 'u', 'x', 'C');
    const after_urn = copySlice(buf[0..], p + 12, alpha_urn);
    buf[after_urn] = 0;
    p += auxc_size;

    writeBoxHeader(buf[0..], ipco_s, p - ipco_s, 'i', 'p', 'c', 'o');
    writeBoxHeader(buf[0..], 36, p - 36, 'i', 'p', 'r', 'p');
    writeBoxHeader(buf[0..], 24, p - 24, 'm', 'e', 't', 'a');

    expect_eq(isAvif(buf[0..p]), true);
    const info = parseAvifInfo(buf[0..p]) ?? unreachable;
    expect_eq(info.brand.is_sequence, true);
    expect_eq(info.brand.has_mif1, true);
    expect_eq(info.brand.has_miaf, true);
    expect_eq(info.brand.minor_version, 1);
    expect_eq(info.width, 512);
    expect_eq(info.height, 512);
    expect_eq(info.has_icc_profile, true);
    expect_eq(info.rotation_degrees, 270);
    expect_eq(info.has_alpha, true);
});

test("4. Non-AVIF MP4/ISOM/HEIC containers are cleanly rejected", () => {
    let mp4: [24]u8 = std.mem.zeroes([24]u8);
    writeBoxHeader(mp4[0..], 0, 24, 'f', 't', 'y', 'p');
    writeFourcc(mp4[0..], 8, 'i', 's', 'o', 'm');
    writeU32BE(mp4[0..], 12, 512);
    writeFourcc(mp4[0..], 16, 'm', 'p', '4', '1');
    writeFourcc(mp4[0..], 20, 'h', 'e', 'i', 'c');

    expect_eq(isAvif(mp4[0..]), false);
    expect_eq(parseAvifBrand(mp4[0..]) == null, true);
    expect_eq(parseAvifInfo(mp4[0..]) == null, true);

    // Non-ftyp first box rejected
    let non_ftyp: [16]u8 = std.mem.zeroes([16]u8);
    writeBoxHeader(non_ftyp[0..], 0, 16, 'f', 'r', 'e', 'e');
    expect_eq(isAvif(non_ftyp[0..]), false);
});

test("5. Malformed, truncated, and out-of-bounds BMFF boxes fail safely without panic", () => {
    // Empty and too-short slices
    let empty: [4]u8 = std.mem.zeroes([4]u8);
    expect_eq(isAvif(empty[0..0]), false);
    expect_eq(isAvif(empty[0..4]), false);

    // ftyp declaring size 100 on a 20-byte slice
    let oob_ftyp: [20]u8 = std.mem.zeroes([20]u8);
    writeBoxHeader(oob_ftyp[0..], 0, 100, 'f', 't', 'y', 'p');
    writeFourcc(oob_ftyp[0..], 8, 'a', 'v', 'i', 'f');
    expect_eq(isAvif(oob_ftyp[0..]), false);
    expect_eq(parseAvifInfo(oob_ftyp[0..]) == null, true);

    // ftyp valid (16B) followed by truncated meta box (declares 64B, only 12B left)
    let trunc_meta: [28]u8 = std.mem.zeroes([28]u8);
    writeBoxHeader(trunc_meta[0..], 0, 16, 'f', 't', 'y', 'p');
    writeFourcc(trunc_meta[0..], 8, 'a', 'v', 'i', 'f');
    writeBoxHeader(trunc_meta[0..], 16, 64, 'm', 'e', 't', 'a');
    expect_eq(isAvif(trunc_meta[0..]), true);
    expect_eq(parseAvifInfo(trunc_meta[0..]) == null, true);

    // ftyp valid (16B) with zero-length/invalid size (< 8) second box
    let tiny_box: [24]u8 = std.mem.zeroes([24]u8);
    writeBoxHeader(tiny_box[0..], 0, 16, 'f', 't', 'y', 'p');
    writeFourcc(tiny_box[0..], 8, 'a', 'v', 'i', 'f');
    writeBoxHeader(tiny_box[0..], 16, 4, 'm', 'e', 't', 'a');
    expect_eq(parseAvifInfo(tiny_box[0..]) == null, true);
});

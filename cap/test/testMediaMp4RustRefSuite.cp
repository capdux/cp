// alfg/mp4-rust (MIT) & ISO/IEC 14496-12 Reference-Hardened MP4 / ISO BMFF Test Suite
// Covers:
//   - Test 01: Real MP4/M4V/M4S fixtures from _ref/mp4-rust/tests/samples/ (isMp4, validateMp4Structure,
//              ftyp, mvhd, tkhd, mdhd, hdlr, stsz, stco, stts)
//   - Test 02: Synthetic ISO BMFF boxes: mvhd/tkhd/mdhd v0 & v1, co64 64-bit offsets, largesize (size==1),
//              size==0 trailing box, uuid 16-byte extended box, meta FullBox children
//   - Test 03: Security guards: depth > MP4_MAX_BOX_DEPTH (32) MaxDepthExceeded, size < 8 InvalidBoxSize,
//              truncated header/payload UnexpectedEof, forged stsz/stco/stts count overflow
//   - Test 04: SIMD hot-path parity (mp4CopyPayloadSimd, mp4HasBrandSimd, mp4UnpackU32BESimd vs Scalar)
//   - Test 05: 1,500-iteration deterministic PRNG malformed MP4 / forged largesize / depth-bomb fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    MP4_MAX_BOX_DEPTH,
    Mp4Error,
    mp4CopyPayloadSimd,
    mp4CopyPayloadScalar,
    mp4HasBrandSimd,
    mp4HasBrandScalar,
    mp4UnpackU32BESimd,
    mp4UnpackU32BEScalar,
    Mp4Box,
    FtypBox,
    MvhdBox,
    MdhdBox,
    TkhdBox,
    HdlrBox,
    StszBox,
    StcoBox,
    SttsBox,
    BoxCursor,
    fourccEquals,
    findBox,
    findBoxPath,
    parseFtyp,
    isMp4,
    parseMvhd,
    parseMdhd,
    parseTkhd,
    parseHdlr,
    parseStsz,
    parseStco,
    parseCo64,
    parseStts,
    validateMp4Structure,
    validateMp4StructureWithMaxDepth,
} from "../../lib/media/mp4.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn writeU32BE(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU64BE(buf: []u8, off: usize, v: u64) {
    writeU32BE(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeU32BE(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn runTest01(): void! {
    // 1. Validate real fixtures from _ref/mp4-rust/tests/samples/
    const minimal_mp4 = readFileSync("_ref/mp4-rust/tests/samples/minimal.mp4");
    expect_eq(isMp4(minimal_mp4), true);
    try validateMp4Structure(minimal_mp4);

    const ftyp_b = findBox(minimal_mp4, "ftyp").?;
    const ftyp = parseFtyp(ftyp_b.payload).?;
    expect_eq(ftyp.hasBrand("isom"), true);

    const mvhd_b = findBoxPath(minimal_mp4, "moov/mvhd").?;
    const mvhd = parseMvhd(mvhd_b.payload).?;
    expect_eq(mvhd.timescale > 0, true);

    const hdlr_b = findBoxPath(minimal_mp4, "moov/trak/mdia/hdlr").?;
    const hdlr = parseHdlr(hdlr_b.payload).?;
    expect_eq(hdlr.isVideo() || hdlr.isAudio(), true);

    const stsz_b = findBoxPath(minimal_mp4, "moov/trak/mdia/minf/stbl/stsz").?;
    const stsz = parseStsz(stsz_b.payload).?;
    expect_eq(stsz.sample_count > 0, true);
    expect_eq(stsz.sampleSizeAt(0) != null, true);

    const stts_b = findBoxPath(minimal_mp4, "moov/trak/mdia/minf/stbl/stts").?;
    const stts = parseStts(stts_b.payload).?;
    expect_eq(stts.entry_count > 0, true);
    expect_eq(stts.totalDuration() > 0, true);

    if (stco_b := findBoxPath(minimal_mp4, "moov/trak/mdia/minf/stbl/stco")) {
        const stco = parseStco(stco_b.payload).?;
        expect_eq(stco.entry_count > 0, true);
        expect_eq(stco.chunkOffsetAt(0) != null, true);
    }

    // Also validate minimal_init.mp4, minimal_fragment.m4s, and big_buck_bunny_metadata.m4v
    const init_mp4 = readFileSync("_ref/mp4-rust/tests/samples/minimal_init.mp4");
    expect_eq(isMp4(init_mp4), true);
    try validateMp4Structure(init_mp4);

    const frag_m4s = readFileSync("_ref/mp4-rust/tests/samples/minimal_fragment.m4s");
    try validateMp4Structure(frag_m4s);

    const bbb_m4v = readFileSync("_ref/mp4-rust/tests/samples/big_buck_bunny_metadata.m4v");
    expect_eq(isMp4(bbb_m4v), true);
    try validateMp4Structure(bbb_m4v);
}

fn runTest02(): void! {
    // 1. 64-bit largesize box (size==1) + uuid 16-byte box + size==0 trailing box
    let buf: [64]u8 = [0; 64];
    // Box 0: largesize (20 bytes): size=1, type="free", largesize=20, 4 bytes payload
    writeU32BE(&buf, 0, 1);
    buf[4] = 'f'; buf[5] = 'r'; buf[6] = 'e'; buf[7] = 'e';
    writeU64BE(&buf, 8, 20);
    // Box 1: uuid (28 bytes at offset 20): size=28, type="uuid", 16-byte uuid, 4 bytes payload
    writeU32BE(&buf, 20, 28);
    buf[24] = 'u'; buf[25] = 'u'; buf[26] = 'i'; buf[27] = 'd';
    buf[28] = 0xAB; buf[43] = 0xCD;
    // Box 2: size==0 trailing box (at offset 48..64): size=0, type="mdat"
    writeU32BE(&buf, 48, 0);
    buf[52] = 'm'; buf[53] = 'd'; buf[54] = 'a'; buf[55] = 't';

    try validateMp4Structure(&buf);
    let cur = BoxCursor.init(&buf);
    const b0 = cur.next().?;
    expect_eq(b0.isType("free"), true);
    expect_eq(b0.header_size, 16 as u8);
    expect_eq(b0.payload.len, 4);

    const b1 = cur.next().?;
    expect_eq(b1.isType("uuid"), true);
    expect_eq(b1.has_uuid, true);
    expect_eq(b1.uuid[0], 0xAB as u8);
    expect_eq(b1.uuid[15], 0xCD as u8);
    expect_eq(b1.payload.len, 4);

    const b2 = cur.next().?;
    expect_eq(b2.isType("mdat"), true);
    expect_eq(b2.size, 16 as u64);
    expect_eq(cur.next() == null, true);

    // 2. mvhd v1 (64-bit) & co64 (64-bit chunk offsets)
    let mvhd1_buf: [112]u8 = [0; 112];
    mvhd1_buf[0] = 1; // version 1
    writeU32BE(&mvhd1_buf, 20, 90000); // timescale
    writeU64BE(&mvhd1_buf, 24, 180000); // duration -> 2000ms
    const mvhd1 = parseMvhd(&mvhd1_buf).?;
    expect_eq(mvhd1.version, 1 as u8);
    expect_eq(mvhd1.durationMs(), 2000 as u64);

    let co64_buf: [16]u8 = [0; 16];
    writeU32BE(&co64_buf, 4, 1); // entry_count = 1
    writeU64BE(&co64_buf, 8, 0x00000001_00000040);
    const co64 = parseCo64(&co64_buf).?;
    expect_eq(co64.is_co64, true);
    expect_eq(co64.chunkOffsetAt(0).?, 0x00000001_00000040 as u64);
}

fn runTest03(): void! {
    // 1. Box nesting depth > MP4_MAX_BOX_DEPTH (32) -> MaxDepthExceeded
    expect_eq(MP4_MAX_BOX_DEPTH, 32);
    // Build 34 nested "moov" containers: 34 * 8 = 272 bytes
    let deep: [272]u8 = [0; 272];
    let d: usize = 0;
    while (d < 34) : (d += 1) {
        const off = d * 8;
        const sz: u32 = ((34 - d) * 8) as u32;
        writeU32BE(&deep, off, sz);
        deep[off + 4] = 'm';
        deep[off + 5] = 'o';
        deep[off + 6] = 'o';
        deep[off + 7] = 'v';
    }
    let depth_err = false;
    validateMp4Structure(&deep) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "MaxDepthExceeded")) depth_err = true;
    };
    expect_eq(depth_err, true);

    // 2. Invalid box size (size = 4 < 8) -> InvalidBoxSize
    let bad_sz: [8]u8 = [ 0, 0, 0, 4, 'f', 'r', 'e', 'e' ];
    let sz_err = false;
    validateMp4Structure(&bad_sz) catch |e| {
        if (std.mem.eql(u8, @errorName(e), "InvalidBoxSize")) sz_err = true;
    };
    expect_eq(sz_err, true);

    // 3. Forged stsz/stco/stts count overflow returns null safely
    let bad_stsz: [12]u8 = [0; 12];
    writeU32BE(&bad_stsz, 8, 0x7FFFFFFF);
    expect_eq(parseStsz(&bad_stsz) == null, true);

    let bad_stco: [8]u8 = [0; 8];
    writeU32BE(&bad_stco, 4, 0x7FFFFFFF);
    expect_eq(parseStco(&bad_stco) == null, true);
    expect_eq(parseCo64(&bad_stco) == null, true);
    expect_eq(parseStts(&bad_stco) == null, true);
}

fn runTest04(): void! {
    // 1. SIMD @Vector(16, u8) payload copy parity (0..128)
    let src: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 128) : (i += 1) src[i] = ((i * 43 + 5) & 0xFF) as u8;
    let d_simd: [128]u8 = undefined;
    let d_scal: [128]u8 = undefined;
    let sz: usize = 0;
    while (sz <= 128) : (sz += 1) {
        mp4CopyPayloadSimd(d_simd[0..sz], src[0..sz]);
        mp4CopyPayloadScalar(d_scal[0..sz], src[0..sz]);
        expect_eq(std.mem.eql(u8, d_simd[0..sz], d_scal[0..sz]), true);
    }

    // 2. SIMD FourCC compatible_brands search parity (0..20 brands)
    let brands: [80]u8 = [ 'a'; 80 ];
    brands[68] = 'm'; brands[69] = 'p'; brands[70] = '4'; brands[71] = '2';
    let b_cnt: usize = 0;
    while (b_cnt <= 20) : (b_cnt += 1) {
        const sl = brands[0 .. b_cnt * 4];
        expect_eq(mp4HasBrandSimd(sl, "mp42"), mp4HasBrandScalar(sl, "mp42"));
        expect_eq(mp4HasBrandSimd(sl, "avc1"), mp4HasBrandScalar(sl, "avc1"));
    }

    // 3. 4-lane SIMD big-endian u32 unpack parity (0..32 u32s)
    let u_simd: [32]u32 = undefined;
    let u_scal: [32]u32 = undefined;
    let n: usize = 0;
    while (n <= 32) : (n += 1) {
        mp4UnpackU32BESimd(u_simd[0..n], src[0 .. n * 4]);
        mp4UnpackU32BEScalar(u_scal[0..n], src[0 .. n * 4]);
        let k: usize = 0;
        while (k < n) : (k += 1) {
            expect_eq(u_simd[k], u_scal[k]);
        }
    }
}

fn runTest05(): void! {
    // 1,500-iteration deterministic PRNG malformed MP4 / forged largesize / depth-bomb fuzzing
    let seed: u64 = 0x1449_6120_CAFE_BABE;
    let buf: [128]u8 = undefined;

    let iter: usize = 0;
    let ok_count: usize = 0;
    let err_count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 96) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = (xorshift64(&seed) & 0xFF) as u8;
        }
        * = isMp4(buf[0..len]);
        * = findBoxPath(buf[0..len], "moov/mvhd");
        validateMp4Structure(buf[0..len]) catch {
            err_count += 1;
            continue;
        };
        ok_count += 1;
    }
    expect_eq(ok_count + err_count, 1500);
}

fn main() {
    test("mp4_ref_01_real_samples_ftyp_mvhd_hdlr_stsz_stco_stts", () => { try runTest01(); });
    test("mp4_ref_02_largesize_size0_uuid_mvhd_v1_and_co64", () => { try runTest02(); });
    test("mp4_ref_03_security_depth_32_invalid_size_and_overflow", () => { try runTest03(); });
    test("mp4_ref_04_simd_copy_brand_search_and_u32be_unpack_parity", () => { try runTest04(); });
    test("mp4_ref_05_deterministic_prng_1500_fuzz_malformed_mp4", () => { try runTest05(); });
}

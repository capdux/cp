// Wave 30 R2 (Lane 2B) Rust Reference Hardening Test Suite for lib/media/flac.cp
// Reference benchmark: ruuda/claxon (Apache-2.0)
// Covers:
//   1. Real Apache-2.0 FLAC files from _ref/claxon/testsamples/ (pop, short, wasted_bits,
//      empty_vorbis_comment, repeated_vorbis_comment, non_subset) with 128-bit MD5 0-bit lossless verification
//   2. ruuda/claxon reference vectors: UTF-8 sample numbers, CRC-8/16 tamper rejection,
//      Left/Right/Mid-Side stereo decorrelation, SEEKTABLE & VORBIS_COMMENT metadata
//   3. 0-bit exact SIMD vs Scalar differential verification (decorrelate*Simd & lpcPredictSimd, orders 1..32)
//   4. Real claxon fuzz crash-regression files + 1,250 PRNG malformed FLAC fuzz cases (0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { readFileSync } from "cap:fs";
import {
    FlacError,
    FlacBlockType,
    FlacStreamInfo,
    FlacSeekPoint,
    FlacVorbisCommentHeader,
    FlacUtf8Result,
    FlacDecodeSummary,
    flacCrc8,
    flacCrc16,
    decodeFlacUtf8,
    decorrelateLeftSideScalar,
    decorrelateLeftSideSimd,
    decorrelateRightSideScalar,
    decorrelateRightSideSimd,
    decorrelateMidSideScalar,
    decorrelateMidSideSimd,
    lpcPredictScalar,
    lpcPredictSimd,
    isFlac,
    FlacBlockIter,
    parseFlacStreamInfo,
    parseSeekTable,
    parseVorbisComment,
    findAudioDataOffset,
    decodeFlacStream,
} from "../../lib/media/flac.cp";

const PATH_POP: []const u8 = "_ref/claxon/testsamples/pop.flac";
const PATH_SHORT: []const u8 = "_ref/claxon/testsamples/short.flac";
const PATH_WASTED: []const u8 = "_ref/claxon/testsamples/wasted_bits.flac";
const PATH_EMPTY_VC: []const u8 = "_ref/claxon/testsamples/empty_vorbis_comment.flac";
const PATH_REP_VC: []const u8 = "_ref/claxon/testsamples/repeated_vorbis_comment.flac";
const PATH_NON_SUBSET: []const u8 = "_ref/claxon/testsamples/non_subset.flac";

const PATH_FUZZ_LARGE_VENDOR: []const u8 = "_ref/claxon/testsamples/large_vendor_string.flac";
const PATH_FUZZ_LARGE_VC: []const u8 = "_ref/claxon/testsamples/large_vorbis_comment_block.flac";
const PATH_FUZZ_01: []const u8 = "_ref/claxon/testsamples/fuzz/07d990b7992590022e2721bfaa83e4bd3b905443.flac";
const PATH_FUZZ_02: []const u8 = "_ref/claxon/testsamples/fuzz/1cc7059cbdac01b75e91eef8e826895cdfbaccca.flac";
const PATH_FUZZ_03: []const u8 = "_ref/claxon/testsamples/fuzz/23d1288573bb3485769da0a969bf5a5c20bd54f2.flac";
const PATH_FUZZ_04: []const u8 = "_ref/claxon/testsamples/fuzz/5ef80ac28c9554fc3bdfa0096dac28f35c79c652.flac";
const PATH_FUZZ_05: []const u8 = "_ref/claxon/testsamples/fuzz/848d2895bf8191ff6a94ccacb4e7fb696c62fa05.flac";
const PATH_FUZZ_06: []const u8 = "_ref/claxon/testsamples/fuzz/c6c1d767d72b388aad8a020f661c90a2684a9470.flac";

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

struct XorShift64 {
    state: u64 = 0x1E37_79B9_7F4A_7C15;

    fn next(*_): u64 {
        let x: u64 = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn range(*_, bound: usize): usize {
        if (bound == 0) return 0;
        return @as(usize, @intCast(_.next() % @as(u64, @intCast(bound))));
    }
}

fn verifyDecodedMd5(samples: []const i32, bps: u8, expected_md5: [16]u8): void! {
    // Check if STREAMINFO MD5 is non-zero (unset MD5 is all 0x00 per RFC 9639)
    let any_nz: bool = false;
    let m: usize = 0;
    while (m < 16) : (m += 1) {
        if (expected_md5[m] != 0) any_nz = true;
    }
    if (!any_nz) return;

    const bytes_per_sample: usize = (@as(usize, bps) + 7) / 8;
    let hasher = std.crypto.hash.Md5.init(.{});
    let tmp: [4]u8 = [0; 4];
    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const u: u32 = @bitCast(samples[i]);
        tmp[0] = @truncate(u);
        tmp[1] = @truncate(u >> 8);
        tmp[2] = @truncate(u >> 16);
        tmp[3] = @truncate(u >> 24);
        hasher.update(tmp[0..bytes_per_sample]);
    }
    let digest: [16]u8 = [0; 16];
    hasher.final(&digest);
    m = 0;
    while (m < 16) : (m += 1) {
        expect_eq(digest[m], expected_md5[m]);
    }
}

fn verifyRealFlacFile(alloc: std.mem.Allocator, data: []const u8): void! {
    expect_true(isFlac(data))?;
    const si_opt: FlacStreamInfo? = parseFlacStreamInfo(data);
    expect_true(si_opt != null)?;
    const si: FlacStreamInfo = si_opt ?? return;
    expect_true(si.channels >= 1 && si.channels <= 8)?;
    expect_true(si.sample_rate > 0)?;

    const scratch_len: usize = @as(usize, si.max_block_size) * @as(usize, si.channels) + 64;
    const out_len: usize = (@as(usize, @intCast(si.total_samples)) + @as(usize, si.max_block_size)) * @as(usize, si.channels) + 64;

    let scratch = alloc.alloc(i32, scratch_len) catch { expect_true(false)?; return; };
    defer alloc.free(scratch);
    let out = alloc.alloc(i32, out_len) catch { expect_true(false)?; return; };
    defer alloc.free(out);

    const summary: FlacDecodeSummary = decodeFlacStream(data, scratch, out) catch {
        expect_true(false)?;
        return;
    };
    expect_true(summary.frames_decoded >= 1)?;
    if (si.total_samples > 0) {
        expect_eq(summary.total_samples_per_channel, @as(usize, @intCast(si.total_samples)));
    }
    expect_eq(summary.total_interleaved_samples, summary.total_samples_per_channel * @as(usize, si.channels));

    // Cryptographic 128-bit MD5 verification of exact decoded PCM bytes!
    verifyDecodedMd5(out[0..summary.total_interleaved_samples], si.bits_per_sample, si.md5)?;
}

// ============================================================================
// Test 1: Real Apache-2.0 FLAC Files from _ref/claxon/testsamples/ + MD5 Proof
// ============================================================================
fn runTestRealClaxonSamples(): void! {
    const alloc = std.heap.page_allocator;
    const sample_pop = readFileSync(PATH_POP);
    const sample_short = readFileSync(PATH_SHORT);
    const sample_wasted = readFileSync(PATH_WASTED);
    const sample_empty_vc = readFileSync(PATH_EMPTY_VC);
    const sample_rep_vc = readFileSync(PATH_REP_VC);
    const sample_non_subset = readFileSync(PATH_NON_SUBSET);

    verifyRealFlacFile(alloc, sample_pop)?;
    verifyRealFlacFile(alloc, sample_short)?;
    verifyRealFlacFile(alloc, sample_wasted)?;
    verifyRealFlacFile(alloc, sample_empty_vc)?;
    verifyRealFlacFile(alloc, sample_rep_vc)?;
    verifyRealFlacFile(alloc, sample_non_subset)?;

    // Also verify VORBIS_COMMENT metadata parsing on empty_vorbis_comment.flac
    let it = FlacBlockIter.init(sample_empty_vc);
    let saw_vc: bool = false;
    while (blk := it.next()) {
        if (blk.block_type == FlacBlockType.vorbis_comment) {
            const vc: FlacVorbisCommentHeader = parseVorbisComment(blk.payload) catch {
                expect_true(false)?;
                return;
            };
            expect_eq(vc.comment_count, 3);
            saw_vc = true;
        }
    }
    expect_true(saw_vc)?;
}

// ============================================================================
// Test 2: Claxon Unit Vectors (UTF-8, CRC-8/16 Tamper, Stereo Decorrelation, SeekTable)
// ============================================================================
fn runTestClaxonReferenceVectors(): void! {
    // 1. UTF-8 variable-length integers (from claxon/src/frame.rs)
    const u_stream: [13]u8 = [0x24, 0xc2, 0xa2, 0xe2, 0x82, 0xac, 0xf0, 0x90, 0x8d, 0x88, 0xc2, 0x00, 0x80];
    const r0: FlacUtf8Result = decodeFlacUtf8(u_stream[0..], 0) catch { expect_true(false)?; return; };
    expect_eq(r0.value, 0x24);
    expect_eq(r0.bytes_read, 1);
    const r1: FlacUtf8Result = decodeFlacUtf8(u_stream[0..], 1) catch { expect_true(false)?; return; };
    expect_eq(r1.value, 0xa2);
    expect_eq(r1.bytes_read, 2);
    const r2: FlacUtf8Result = decodeFlacUtf8(u_stream[0..], 3) catch { expect_true(false)?; return; };
    expect_eq(r2.value, 0x20ac);
    expect_eq(r2.bytes_read, 3);
    const r3: FlacUtf8Result = decodeFlacUtf8(u_stream[0..], 6) catch { expect_true(false)?; return; };
    expect_eq(r3.value, 0x010348);
    expect_eq(r3.bytes_read, 4);

    let bad_cont: bool = false;
    * = decodeFlacUtf8(u_stream[0..], 10) catch |e| { if (e == FlacError.InvalidUtf8Number) bad_cont = true; };
    expect_true(bad_cont)?;
    let bad_lead: bool = false;
    * = decodeFlacUtf8(u_stream[0..], 12) catch |e| { if (e == FlacError.InvalidUtf8Number) bad_lead = true; };
    expect_true(bad_lead)?;

    // 2. Exact Left-Side / Right-Side / Mid-Side vectors (from claxon/src/frame.rs)
    const exp_l: [8]i32 = [2, 5, 83, 113, 127, -63, -45, -15];
    const exp_r: [8]i32 = [-5, -33, -59, -125, 127, 89, 7, 3];

    let ls_side: [8]i32 = [7, 38, 142, 238, 0, -152, -52, -18];
    let ls_s: []i32 = ls_side[0..];
    decorrelateLeftSideSimd(exp_l[0..], ls_s);
    let i: usize = 0;
    while (i < 8) : (i += 1) expect_eq(ls_side[i], exp_r[i]);

    let rs_side: [8]i32 = [7, 38, 142, 238, 0, -152, -52, -18];
    let rs_s: []i32 = rs_side[0..];
    decorrelateRightSideSimd(rs_s, exp_r[0..]);
    i = 0;
    while (i < 8) : (i += 1) expect_eq(rs_side[i], exp_l[i]);

    let ms_mid: [8]i32 = [-2, -14, 12, -6, 127, 13, -19, -6];
    let ms_side: [8]i32 = [7, 38, 142, 238, 0, -152, -52, -18];
    let mm_s: []i32 = ms_mid[0..];
    let ms_s: []i32 = ms_side[0..];
    decorrelateMidSideSimd(mm_s, ms_s);
    i = 0;
    while (i < 8) : (i += 1) {
        expect_eq(ms_mid[i], exp_l[i]);
        expect_eq(ms_side[i], exp_r[i]);
    }

    // 3. CRC-8 & CRC-16 tamper rejection on real pop.flac
    const sample_pop = readFileSync(PATH_POP);
    let mut_pop: [256]u8 = [0; 256];
    i = 0;
    while (i < sample_pop.len) : (i += 1) mut_pop[i] = sample_pop[i];
    const audio_off: usize = findAudioDataOffset(sample_pop) catch { expect_true(false)?; return; };
    let scr: [256]i32 = [0; 256];
    let out: [256]i32 = [0; 256];
    let scr_s: []i32 = scr[0..];
    let out_s: []i32 = out[0..];

    // Flip 1 bit in frame header -> Crc8Mismatch (or InvalidFrameHeader)
    mut_pop[audio_off + 2] ^= 0x01;
    let hdr_err: bool = false;
    * = decodeFlacStream(mut_pop[0..sample_pop.len], scr_s, out_s) catch { hdr_err = true; };
    expect_true(hdr_err)?;
    mut_pop[audio_off + 2] ^= 0x01;

    // Flip 1 bit in trailing CRC-16 -> Crc16Mismatch
    mut_pop[sample_pop.len - 1] ^= 0x01;
    let crc16_err: bool = false;
    * = decodeFlacStream(mut_pop[0..sample_pop.len], scr_s, out_s) catch |e| {
        if (e == FlacError.Crc16Mismatch) crc16_err = true;
    };
    expect_true(crc16_err)?;

    // 4. SEEKTABLE parsing (18-byte entry)
    let st_raw: [18]u8 = [0; 18];
    st_raw[7] = 42; // sample_number = 42
    st_raw[15] = 99; // stream_offset = 99
    st_raw[16] = 0x10;
    st_raw[17] = 0x00; // frame_samples = 4096
    let pts: [4]FlacSeekPoint = [FlacSeekPoint{}; 4];
    let pts_s: []FlacSeekPoint = pts[0..];
    const np: usize = parseSeekTable(st_raw[0..], pts_s) catch { expect_true(false)?; return; };
    expect_eq(np, 1);
    expect_eq(pts[0].sample_number, 42);
    expect_eq(pts[0].stream_offset, 99);
    expect_eq(pts[0].frame_samples, 4096);
    * = flacCrc8(st_raw[0..]);
    * = flacCrc16(st_raw[0..]);
}

// ============================================================================
// Test 3: 0-Bit Exact SIMD vs Scalar Differential Verification
// ============================================================================
fn runTestSimdZeroBitExact(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x428A_2F98_7137_4491 };

    let a_simd: [96]i32 = [0; 96];
    let b_simd: [96]i32 = [0; 96];
    let a_ref: [96]i32 = [0; 96];
    let b_ref: [96]i32 = [0; 96];

    // 1. Stereo decorrelation SIMD vs Scalar across lengths 0..67
    let len: usize = 0;
    while (len <= 67) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const v1: i32 = @as(i32, @intCast(rng.range(16_000_000))) - 8_000_000;
            const v2: i32 = @as(i32, @intCast(rng.range(16_000_000))) - 8_000_000;
            a_simd[i] = v1;
            b_simd[i] = v2;
            a_ref[i] = v1;
            b_ref[i] = v2;
        }
        let as_s: []i32 = a_simd[0..len];
        let bs_s: []i32 = b_simd[0..len];
        let ar_s: []i32 = a_ref[0..len];
        let br_s: []i32 = b_ref[0..len];
        decorrelateMidSideSimd(as_s, bs_s);
        decorrelateMidSideScalar(ar_s, br_s);
        i = 0;
        while (i < len) : (i += 1) {
            expect_eq(a_simd[i], a_ref[i]);
            expect_eq(b_simd[i], b_ref[i]);
        }

        decorrelateLeftSideSimd(a_simd[0..len], bs_s);
        decorrelateLeftSideScalar(a_ref[0..len], br_s);
        decorrelateRightSideSimd(as_s, b_simd[0..len]);
        decorrelateRightSideScalar(ar_s, b_ref[0..len]);
        i = 0;
        while (i < len) : (i += 1) {
            expect_eq(a_simd[i], a_ref[i]);
            expect_eq(b_simd[i], b_ref[i]);
        }
    }

    // 2. LPC prediction SIMD vs Scalar across ALL orders 1..32
    let coeffs: [32]i32 = [0; 32];
    let order: usize = 1;
    while (order <= 32) : (order += 1) {
        let c: usize = 0;
        while (c < order) : (c += 1) {
            coeffs[c] = @as(i32, @intCast(rng.range(4096))) - 2048;
        }
        const n: usize = order + 48;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const val: i32 = if (i < order)
                (@as(i32, @intCast(rng.range(65536))) - 32768)
            else
                (@as(i32, @intCast(rng.range(1024))) - 512);
            a_simd[i] = val;
            a_ref[i] = val;
        }
        const shift: u6 = @intCast(rng.range(12) + 4);
        let as_s: []i32 = a_simd[0..n];
        let ar_s: []i32 = a_ref[0..n];
        lpcPredictSimd(as_s, coeffs[0..order], shift);
        lpcPredictScalar(ar_s, coeffs[0..order], shift);
        i = 0;
        while (i < n) : (i += 1) {
            expect_eq(a_simd[i], a_ref[i]); // 0-bit exact!
        }
    }
}

// ============================================================================
// Test 4: Real Claxon Fuzz Regression Corpus + 1,250 PRNG Malformed FLAC Fuzz
// ============================================================================
fn fuzzProbeFlac(data: []const u8, scr: *[]i32, out: *[]i32) {
    * = parseFlacStreamInfo(data);
    * = findAudioDataOffset(data) catch 0;
    * = decodeFlacStream(data, scr, out) catch FlacDecodeSummary{};
}

fn runTestMalformedFuzz1250(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x70FF_1234_5678_9ABC };
    let scr: [2048]i32 = [0; 2048];
    let out: [2048]i32 = [0; 2048];
    let scr_s: []i32 = scr[0..];
    let out_s: []i32 = out[0..];

    const sample_pop = readFileSync(PATH_POP);
    const sample_short = readFileSync(PATH_SHORT);

    // 1. Real claxon fuzz crash-regression files (0 panic / 0 OOB / 0 hang)
    fuzzProbeFlac(readFileSync(PATH_FUZZ_LARGE_VENDOR), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_LARGE_VC), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_01), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_02), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_03), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_04), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_05), scr_s, out_s);
    fuzzProbeFlac(readFileSync(PATH_FUZZ_06), scr_s, out_s);

    // 2. Exhaustive truncation of real pop.flac & short.flac
    let t: usize = 0;
    while (t < sample_pop.len) : (t += 1) {
        fuzzProbeFlac(sample_pop[0..t], scr_s, out_s);
    }
    t = 0;
    while (t < sample_short.len) : (t += 1) {
        fuzzProbeFlac(sample_short[0..t], scr_s, out_s);
    }

    // 3. 1,250 PRNG byte mutations, forged headers, pathological zero runs & random noise
    let mut_buf: [256]u8 = [0; 256];
    let trial: usize = 0;
    while (trial < 1250) : (trial += 1) {
        const mode: usize = rng.range(4);
        let flen: usize = 0;
        if (mode == 0) {
            // Mutate real short.flac
            flen = sample_short.len;
            let j: usize = 0;
            while (j < flen) : (j += 1) mut_buf[j] = sample_short[j];
            const flips: usize = rng.range(6) + 1;
            let k: usize = 0;
            while (k < flips) : (k += 1) {
                const pos: usize = rng.range(flen);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
        } else if (mode == 1) {
            // Mutate real pop.flac with random truncation
            const max_l: usize = sample_pop.len;
            flen = rng.range(max_l + 1);
            let j: usize = 0;
            while (j < flen) : (j += 1) mut_buf[j] = sample_pop[j];
            if (flen > 0) {
                const pos: usize = rng.range(flen);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
        } else if (mode == 2) {
            // Valid fLaC + STREAMINFO + forged frame sync + all-zero unary flood
            flen = 128;
            let j: usize = 0;
            while (j < 42) : (j += 1) mut_buf[j] = sample_pop[j];
            mut_buf[42] = 0xFF;
            mut_buf[43] = 0xF8;
            j = 44;
            while (j < flen) : (j += 1) mut_buf[j] = 0x00;
        } else {
            // Pure random garbage payload
            flen = rng.range(200);
            let j: usize = 0;
            while (j < flen) : (j += 1) {
                mut_buf[j] = @as(u8, @intCast(rng.next() & 0xFF));
            }
        }
        fuzzProbeFlac(mut_buf[0..flen], scr_s, out_s);
    }
}

fn main() {
    test("FLAC Real Samples: 6 claxon testsamples + 128-bit MD5 0-bit lossless proof", () => {
        runTestRealClaxonSamples()?;
    });
    test("FLAC Reference Vectors: UTF-8, CRC-8/16 tamper, Stereo decorrelation & SeekTable", () => {
        runTestClaxonReferenceVectors()?;
    });
    test("FLAC SIMD: 4-lane Stereo decorrelation & LPC (orders 1..32) 0-bit exact vs scalar", () => {
        runTestSimdZeroBitExact()?;
    });
    test("FLAC Fuzz: claxon fuzz corpus + 1,250 PRNG malformed streams (0 panic, 0 OOB)", () => {
        runTestMalformedFuzz1250()?;
    });
}

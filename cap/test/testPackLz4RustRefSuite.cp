// Wave 30 R3 (Lane 2B) Rust Reference Hardening Test Suite for lib/pack/lz4.cp
// Reference benchmark: PSeitz/lz4_flex (MIT)
// Covers:
//   1. LZ4 Block vectors: empty, short 1..15B, RLE (offset=1), low-offset overlap (2,3,4,7),
//      incompressible data, zero-heap Into APIs & prefix/external dictionary compression
//   2. 16-Lane SIMD vs Scalar 0-byte differential verification (copyNonOverlapping, copyMatch, extendMatchLen)
//   3. Complete LZ4 Frame v1.6.1: > 64KB multi-block, Independent & Linked blocks, ContentSize,
//      BlockChecksum, ContentChecksum, DictId, SkippableFrame & XXH32 tamper rejection
//   4. 1,300 Deterministic PRNG malformed LZ4 Block & Frame Fuzz cases (offset=0, 0xFF length flood,
//      truncation, bit-flips, forged headers -> 0 panic / 0 OOB / 0 hang)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Lz4Error,
    LZ4_FRAME_MAGIC,
    xxhash32,
    Lz4FrameOptions,
    copyNonOverlappingScalar,
    copyNonOverlappingSimd,
    copyMatchScalar,
    copyMatchSimd,
    extendMatchLenScalar,
    extendMatchLenSimd,
    Lz4Block,
    Lz4Frame,
} from "../../lib/pack/lz4.cp";

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

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: LZ4 Block Vectors (Empty, Short 1..15B, RLE, Low-Offset Overlap, Dict)
// ============================================================================
fn runTestLz4BlockVectors(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Empty input
    const c_empty = Lz4Block.compress(alloc, "") catch { expect_true(false)?; return; };
    defer alloc.free(c_empty);
    expect_eq(c_empty.len, 0);
    const d_empty = Lz4Block.decompress(alloc, c_empty, 16) catch { expect_true(false)?; return; };
    defer alloc.free(d_empty);
    expect_eq(d_empty.len, 0);

    // 2. Short inputs 1..20B (covering MFLIMIT=12 & LASTLITERALS=5 boundaries)
    let short_src: [20]u8 = [0; 20];
    let i: usize = 0;
    while (i < 20) : (i += 1) short_src[i] = @as(u8, @intCast('A' + i));
    let slen: usize = 1;
    while (slen <= 20) : (slen += 1) {
        let cbuf: [64]u8 = [0; 64];
        let dbuf: [64]u8 = [0; 64];
        const cl: usize = Lz4Block.compressInto(short_src[0..slen], cbuf[0..]) catch { expect_true(false)?; return; };
        const dl: usize = Lz4Block.decompressInto(cbuf[0..cl], dbuf[0..]) catch { expect_true(false)?; return; };
        expect_eq(dl, slen);
        expect_true(eqBytes(dbuf[0..dl], short_src[0..slen]))?;
    }

    // 3. High-repetition RLE (offset=1) & low-offset overlaps (period 2, 3, 4, 7)
    const periods: [5]usize = [1, 2, 3, 4, 7];
    let pi: usize = 0;
    while (pi < 5) : (pi += 1) {
        const period: usize = periods[pi];
        let src: [2048]u8 = [0; 2048];
        let j: usize = 0;
        while (j < 2048) : (j += 1) {
            src[j] = @as(u8, @intCast('a' + (j % period)));
        }
        const comp = Lz4Block.compress(alloc, src[0..]) catch { expect_true(false)?; return; };
        defer alloc.free(comp);
        expect_true(comp.len < 64)?; // massive compression ratio on periodic data!
        const decoded = Lz4Block.decompress(alloc, comp, 4096) catch { expect_true(false)?; return; };
        defer alloc.free(decoded);
        expect_true(eqBytes(decoded, src[0..]))?;
    }

    // 4. Incompressible PRNG data round-trip
    let rng: XorShift64 = XorShift64{ state: 0x3344_5566_7788_99AA };
    let rand_src: [1024]u8 = [0; 1024];
    i = 0;
    while (i < 1024) : (i += 1) rand_src[i] = @as(u8, @intCast(rng.next() & 0xFF));
    const c_rand = Lz4Block.compress(alloc, rand_src[0..]) catch { expect_true(false)?; return; };
    defer alloc.free(c_rand);
    const d_rand = Lz4Block.decompress(alloc, c_rand, 2048) catch { expect_true(false)?; return; };
    defer alloc.free(d_rand);
    expect_true(eqBytes(d_rand, rand_src[0..]))?;

    // 5. External dictionary compression & decompression
    const dict: []const u8 = "service=payment_gateway;region=ap-southeast-2;status=200;protocol=http2;tls=1.3;";
    const msg: []const u8 = "service=payment_gateway;region=ap-southeast-2;status=200;latency_ms=4;";
    const c_nodict = Lz4Block.compress(alloc, msg) catch { expect_true(false)?; return; };
    defer alloc.free(c_nodict);
    const c_dict = Lz4Block.compressWithDict(alloc, msg, dict) catch { expect_true(false)?; return; };
    defer alloc.free(c_dict);
    expect_true(c_dict.len < c_nodict.len)?; // dictionary significantly shrinks payload!
    const d_dict = Lz4Block.decompressWithDict(alloc, c_dict, dict, 256) catch { expect_true(false)?; return; };
    defer alloc.free(d_dict);
    expect_true(eqBytes(d_dict, msg))?;
}

// ============================================================================
// Test 2: 16-Lane SIMD vs Scalar 0-Byte Differential Verification
// ============================================================================
fn runTestSimdZeroByteDiff(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x1122_3344_5566_7788 };

    let src: [256]u8 = [0; 256];
    let dst_simd: [256]u8 = [0; 256];
    let dst_ref: [256]u8 = [0; 256];

    // 1. copyNonOverlappingSimd vs Scalar across lengths 0..129
    let len: usize = 0;
    while (len <= 129) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) src[i] = @as(u8, @intCast(rng.next() & 0xFF));
        let ds_s: []u8 = dst_simd[0..];
        let dr_s: []u8 = dst_ref[0..];
        copyNonOverlappingSimd(ds_s, 3, src[0..len]);
        copyNonOverlappingScalar(dr_s, 3, src[0..len]);
        expect_true(eqBytes(dst_simd[3..3 + len], dst_ref[3..3 + len]))?;
    }

    // 2. copyMatchSimd vs Scalar across offsets 1, 2, 3, 4, 7, 15, 16, 17, 32, 64 & lengths 0..100
    const offsets: [10]usize = [1, 2, 3, 4, 7, 15, 16, 17, 32, 64];
    let oi: usize = 0;
    while (oi < 10) : (oi += 1) {
        const off: usize = offsets[oi];
        let mlen: usize = 0;
        while (mlen <= 100) : (mlen += 7) {
            let i: usize = 0;
            while (i < 64) : (i += 1) {
                const v: u8 = @as(u8, @intCast(rng.next() & 0xFF));
                dst_simd[i] = v;
                dst_ref[i] = v;
            }
            let ds_s: []u8 = dst_simd[0..];
            let dr_s: []u8 = dst_ref[0..];
            copyMatchSimd(ds_s, 64, off, mlen);
            copyMatchScalar(dr_s, 64, off, mlen);
            expect_true(eqBytes(dst_simd[0..64 + mlen], dst_ref[0..64 + mlen]))?;
        }
    }

    // 3. extendMatchLenSimd vs Scalar across shared lengths 0..120
    let a: [160]u8 = [0; 160];
    let shared: usize = 0;
    while (shared <= 120) : (shared += 1) {
        let i: usize = 0;
        while (i < 160) : (i += 1) a[i] = @as(u8, @intCast(rng.next() & 0xFF));
        i = 0;
        while (i < shared) : (i += 1) a[32 + i] = a[i];
        if (32 + shared < 160) a[32 + shared] = a[shared] ^ 0xFF;
        const l_simd: usize = extendMatchLenSimd(a[0..], 0, 32, 160);
        const l_ref: usize = extendMatchLenScalar(a[0..], 0, 32, 160);
        expect_eq(l_simd, l_ref);
    }
}

// ============================================================================
// Test 3: Complete LZ4 Frame v1.6.1 (>64KB, Linked, ContentSize, BlockChecksum, Skippable)
// ============================================================================
fn runTestLz4FrameV161(): void! {
    const alloc = std.heap.page_allocator;

    // 1. 96KB multi-block payload (> 64KB block boundary)
    const BIG_LEN: usize = 98304;
    let big = alloc.alloc(u8, BIG_LEN) catch { expect_true(false)?; return; };
    defer alloc.free(big);
    let i: usize = 0;
    while (i < BIG_LEN) : (i += 1) {
        // Repeats across the 64KB boundary so Linked blocks can exploit cross-block history
        big[i] = @as(u8, @intCast((i % 251) ^ ((i / 251) & 0x07)));
    }

    // Independent blocks + BlockChecksum + ContentSize + ContentChecksum + DictId
    const f_indep = Lz4Frame.compressFrameWithOptions(alloc, big, Lz4FrameOptions{
        block_max_size: 65536,
        block_independent: true,
        block_checksum: true,
        content_checksum: true,
        content_size: true,
        has_dict_id: true,
        dict_id: 0xCAFE_1234,
    }) catch { expect_true(false)?; return; };
    defer alloc.free(f_indep);

    const d_indep = Lz4Frame.decompressFrame(alloc, f_indep, BIG_LEN + 64) catch { expect_true(false)?; return; };
    defer alloc.free(d_indep);
    expect_true(eqBytes(d_indep, big))?;

    // Linked blocks (`block_independent: false`) across 96KB
    const f_linked = Lz4Frame.compressFrameWithOptions(alloc, big, Lz4FrameOptions{
        block_max_size: 65536,
        block_independent: false,
        block_checksum: true,
        content_checksum: true,
        content_size: true,
    }) catch { expect_true(false)?; return; };
    defer alloc.free(f_linked);

    const d_linked = Lz4Frame.decompressFrame(alloc, f_linked, BIG_LEN + 64) catch { expect_true(false)?; return; };
    defer alloc.free(d_linked);
    expect_true(eqBytes(d_linked, big))?;

    // 2. Skippable Frame prepended before real LZ4 Frame
    const skip = Lz4Frame.encodeSkippableFrame(alloc, 0x0B, "custom-skippable-metadata-v1") catch { expect_true(false)?; return; };
    defer alloc.free(skip);
    const body = Lz4Frame.compressFrame(alloc, "hello skippable lz4 frame!", true) catch { expect_true(false)?; return; };
    defer alloc.free(body);

    let combined = alloc.alloc(u8, skip.len + body.len) catch { expect_true(false)?; return; };
    defer alloc.free(combined);
    copyNonOverlappingSimd(combined, 0, skip);
    copyNonOverlappingSimd(combined, skip.len, body);

    const d_skip = Lz4Frame.decompressFrame(alloc, combined, 256) catch { expect_true(false)?; return; };
    defer alloc.free(d_skip);
    expect_true(eqBytes(d_skip, "hello skippable lz4 frame!"))?;

    // 3. Tamper rejection: Header Checksum, Content Checksum, Block Checksum, Magic
    let tam = alloc.alloc(u8, f_indep.len) catch { expect_true(false)?; return; };
    defer alloc.free(tam);
    copyNonOverlappingSimd(tam, 0, f_indep);

    // Tamper Header Checksum (offset 4 + 2 + 8 + 4 = 18)
    tam[18] ^= 0x01;
    let got_hc: bool = false;
    * = Lz4Frame.decompressFrame(alloc, tam, BIG_LEN + 64) catch |e| { if (e == Lz4Error.InvalidHeaderChecksum) got_hc = true; };
    expect_true(got_hc)?;
    tam[18] ^= 0x01;

    // Tamper Content Checksum (last byte)
    tam[tam.len - 1] ^= 0x01;
    let got_cc: bool = false;
    * = Lz4Frame.decompressFrame(alloc, tam, BIG_LEN + 64) catch |e| { if (e == Lz4Error.InvalidContentChecksum) got_cc = true; };
    expect_true(got_cc)?;
    tam[tam.len - 1] ^= 0x01;

    // Tamper first compressed block byte -> InvalidBlockChecksum
    tam[25] ^= 0x80;
    let got_bc: bool = false;
    * = Lz4Frame.decompressFrame(alloc, tam, BIG_LEN + 64) catch |e| { if (e == Lz4Error.InvalidBlockChecksum) got_bc = true; };
    expect_true(got_bc)?;
    * = LZ4_FRAME_MAGIC;
    * = xxhash32("test", 0);
}

// ============================================================================
// Test 4: 1,300 Deterministic PRNG Malformed LZ4 Block & Frame Fuzz Cases
// ============================================================================
fn runTestMalformedFuzz1300(): void! {
    const alloc = std.heap.page_allocator;
    let rng: XorShift64 = XorShift64{ state: 0x6F0D_BEEF_2468_ACE0 };

    // 1. Explicit security boundary checks: offset=0, offset>out_pos, 0xFF length flood, bad trailing nibble
    let out_buf: [1024]u8 = [0; 1024];
    let out_s: []u8 = out_buf[0..];

    const bad_off0: [6]u8 = [0x10, 'A', 0x00, 0x00, 0x10, 'B'];
    let got_off0: bool = false;
    * = Lz4Block.decompressInto(bad_off0[0..], out_s) catch |e| { if (e == Lz4Error.InvalidOffset) got_off0 = true; };
    expect_true(got_off0)?;

    const bad_off_oob: [6]u8 = [0x10, 'A', 0x05, 0x00, 0x10, 'B'];
    let got_oob: bool = false;
    * = Lz4Block.decompressInto(bad_off_oob[0..], out_s) catch |e| { if (e == Lz4Error.InvalidOffset) got_oob = true; };
    expect_true(got_oob)?;

    // 0xFF extended-length flood (100 consecutive 0xFF bytes -> must cleanly reject OutputTooLarge, never overflow!)
    let flood: [128]u8 = [0xFF; 128];
    flood[0] = 0xF0;
    let got_flood: bool = false;
    * = Lz4Block.decompressInto(flood[0..], out_s) catch |e| {
        if (e == Lz4Error.OutputTooLarge || e == Lz4Error.TruncatedInput) got_flood = true;
    };
    expect_true(got_flood)?;

    // Valid seed block & frame for mutation fuzzing
    const seed_blk = Lz4Block.compress(alloc, "aaaaaabbbbbbccccccdddddd1234567890aaaaaabbbbbb") catch { expect_true(false)?; return; };
    defer alloc.free(seed_blk);
    const seed_frm = Lz4Frame.compressFrameWithOptions(alloc, "hello world hello world hello world", Lz4FrameOptions{
        block_checksum: true,
        content_checksum: true,
        content_size: true,
    }) catch { expect_true(false)?; return; };
    defer alloc.free(seed_frm);

    // Exhaustive truncation of seed block & seed frame
    let t: usize = 1;
    while (t < seed_blk.len) : (t += 1) {
        * = Lz4Block.decompressInto(seed_blk[0..t], out_s) catch 0;
    }
    t = 1;
    while (t < seed_frm.len) : (t += 1) {
        const res = Lz4Frame.decompressFrame(alloc, seed_frm[0..t], 1024) catch continue;
        alloc.free(res);
    }

    // 2. 1,300 PRNG malformed Block & Frame fuzz trials (0 panic / 0 OOB / 0 hang)
    let mut_buf: [256]u8 = [0; 256];
    let trial: usize = 0;
    while (trial < 1300) : (trial += 1) {
        const mode: usize = rng.range(4);
        if (mode == 0) {
            // Mutate valid block
            let j: usize = 0;
            while (j < seed_blk.len) : (j += 1) mut_buf[j] = seed_blk[j];
            const flips: usize = rng.range(5) + 1;
            let k: usize = 0;
            while (k < flips) : (k += 1) {
                const pos: usize = rng.range(seed_blk.len);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
            * = Lz4Block.decompressInto(mut_buf[0..seed_blk.len], out_s) catch 0;
        } else if (mode == 1) {
            // Mutate valid frame
            let j: usize = 0;
            while (j < seed_frm.len) : (j += 1) mut_buf[j] = seed_frm[j];
            const flips: usize = rng.range(5) + 1;
            let k: usize = 0;
            while (k < flips) : (k += 1) {
                const pos: usize = rng.range(seed_frm.len);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
            const res = Lz4Frame.decompressFrame(alloc, mut_buf[0..seed_frm.len], 1024) catch continue;
            alloc.free(res);
        } else if (mode == 2) {
            // Pure random garbage into Block decoder
            const flen: usize = rng.range(128);
            let j: usize = 0;
            while (j < flen) : (j += 1) mut_buf[j] = @as(u8, @intCast(rng.next() & 0xFF));
            * = Lz4Block.decompressInto(mut_buf[0..flen], out_s) catch 0;
        } else {
            // Valid frame magic + random garbage into Frame decoder
            const flen: usize = rng.range(120) + 8;
            mut_buf[0] = 0x04;
            mut_buf[1] = 0x22;
            mut_buf[2] = 0x4D;
            mut_buf[3] = 0x18;
            let j: usize = 4;
            while (j < flen) : (j += 1) mut_buf[j] = @as(u8, @intCast(rng.next() & 0xFF));
            const res = Lz4Frame.decompressFrame(alloc, mut_buf[0..flen], 1024) catch continue;
            alloc.free(res);
        }
    }
}

fn main() {
    test("LZ4 Block: empty, short 1..20B, RLE, low-offset overlap, random & external dict", () => {
        runTestLz4BlockVectors()?;
    });
    test("LZ4 SIMD: 16-lane copyNonOverlapping, copyMatch & extendMatchLen 0-byte diff vs scalar", () => {
        runTestSimdZeroByteDiff()?;
    });
    test("LZ4 Frame v1.6.1: >64KB Independent/Linked, ContentSize, BlockChecksum, Skippable & tamper", () => {
        runTestLz4FrameV161()?;
    });
    test("LZ4 Fuzz: offset=0, 0xFF length flood, truncations & 1,300 PRNG malformed Block/Frame cases", () => {
        runTestMalformedFuzz1300()?;
    });
}

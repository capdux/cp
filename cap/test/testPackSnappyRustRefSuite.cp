// Wave 30 R5 (Lane 2B) Rust Reference Hardening Test Suite for lib/pack/snappy.cp
// Reference benchmark: BurntSushi/rust-snappy (BSD-3-Clause)
// Covers:
//   1. Snappy Raw Block vectors: empty, short 1..20B, low-offset overlap (1..7), incompressible,
//      4-byte Copy tag (offset=70000 > 64KB), zero-heap compressInto/decompressInto
//   2. 16-Lane SIMD vs Scalar 0-byte differential verification (CopyLit, CopyMatch, ExtendMatch)
//   3. Complete Snappy Framing Format (`sNaPpY`): CRC32C RFC 3720 vector + Masked CRC32C,
//      >64KB multi-chunk (3 chunks), Padding (0xFE), Skippable (0x80..0xFD), Unskippable (0x02..0x7F) reject,
//      and single-bit Masked CRC32C tamper rejection
//   4. 1,300 Deterministic PRNG malformed Raw & Framed Fuzz cases (huge Varint, offset=0, truncation -> 0 panic/0 OOB)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SNAPPY_FRAME_CHUNK_SIZE,
    SNAPPY_STREAM_IDENTIFIER,
    SnappyError,
    snappyCrc32c,
    snappyMaskChecksum,
    snappyUnmaskChecksum,
    snappyCopyLitScalar,
    snappyCopyLitSimd,
    snappyCopyMatchScalar,
    snappyCopyMatchSimd,
    snappyExtendMatchScalar,
    snappyExtendMatchSimd,
    maxCompressLen,
    decompressLen,
    compressInto,
    decompressInto,
    compress,
    decompress,
    decompressWithMaxSize,
    encodeFramedPaddingChunk,
    encodeFramedSkippableChunk,
    compressFramed,
    decompressFramed,
    decompressFramedWithMaxSize,
} from "../../lib/pack/snappy.cp";

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
// Test 1: Snappy Raw Block Vectors + 4B Copy Tag (offset=70000) + Into APIs
// ============================================================================
fn runTestSnappyRawBlockVectors(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Empty input
    const c_empty = compress(alloc, "") catch { expect_true(false)?; return; };
    defer alloc.free(c_empty);
    expect_eq(decompressLen(c_empty) catch 999, 0);
    const d_empty = decompress(alloc, c_empty) catch { expect_true(false)?; return; };
    defer alloc.free(d_empty);
    expect_eq(d_empty.len, 0);

    // 2. Short inputs 1..20B via zero-heap compressInto / decompressInto
    let short_src: [20]u8 = [0; 20];
    let i: usize = 0;
    while (i < 20) : (i += 1) short_src[i] = @as(u8, @intCast('A' + i));
    let slen: usize = 1;
    while (slen <= 20) : (slen += 1) {
        let cbuf: [128]u8 = [0; 128];
        let dbuf: [128]u8 = [0; 128];
        const cl: usize = compressInto(short_src[0..slen], cbuf[0..]) catch { expect_true(false)?; return; };
        expect_eq(decompressLen(cbuf[0..cl]) catch 0, slen);
        const dl: usize = decompressInto(cbuf[0..cl], dbuf[0..]) catch { expect_true(false)?; return; };
        expect_eq(dl, slen);
        expect_true(eqBytes(dbuf[0..dl], short_src[0..slen]))?;
    }

    // 3. Low-offset overlapping periods (1, 2, 3, 4, 7)
    const periods: [5]usize = [1, 2, 3, 4, 7];
    let pi: usize = 0;
    while (pi < 5) : (pi += 1) {
        const period: usize = periods[pi];
        let src: [2048]u8 = [0; 2048];
        let j: usize = 0;
        while (j < 2048) : (j += 1) src[j] = @as(u8, @intCast('a' + (j % period)));
        const comp = compress(alloc, src[0..]) catch { expect_true(false)?; return; };
        defer alloc.free(comp);
        expect_true(comp.len < 200)?;
        const decoded = decompress(alloc, comp) catch { expect_true(false)?; return; };
        defer alloc.free(decoded);
        expect_true(eqBytes(decoded, src[0..]))?;
    }

    // 4. Explicit 4-byte Copy Tag (`tag & 0x03 == 0x03`, offset = 70000 > 65535)
    // Construct valid stream: declared_len = 70008, literal of 70000 bytes (tag=62<<2 + 3B len 69999),
    // then 4B Copy of 8 bytes at offset=70000 (tag = 0x03 | ((8-1)<<2) = 0x1F, offset LE32 = 70000)
    const lit_n: usize = 70000;
    const total_n: usize = 70008;
    let raw_4b = alloc.alloc(u8, lit_n + 12) catch { expect_true(false)?; return; };
    defer alloc.free(raw_4b);
    // Varint(70008): 70008 = 0x11178 -> [0xF8, 0xA2, 0x04]
    raw_4b[0] = 0xF8;
    raw_4b[1] = 0xA2;
    raw_4b[2] = 0x04;
    // Literal tag 62 (3-byte length = 69999 = 0x01116F)
    raw_4b[3] = (62 << 2);
    raw_4b[4] = 0x6F;
    raw_4b[5] = 0x11;
    raw_4b[6] = 0x01;
    const magic_prefix: [8]u8 = ['S', 'N', 'A', 'P', '4', 'B', 'C', 'P'];
    i = 0;
    while (i < 8) : (i += 1) raw_4b[7 + i] = magic_prefix[i];
    while (i < lit_n) : (i += 1) raw_4b[7 + i] = 0x2E;
    // 4-byte Copy tag: len=8 ((7<<2)|3 = 0x1F), offset=70000 (0x00011170)
    const cp_off: usize = lit_n + 7;
    raw_4b[cp_off + 0] = 0x1F;
    raw_4b[cp_off + 1] = 0x70;
    raw_4b[cp_off + 2] = 0x11;
    raw_4b[cp_off + 3] = 0x01;
    raw_4b[cp_off + 4] = 0x00;

    const dec_4b = decompressWithMaxSize(alloc, raw_4b, total_n) catch { expect_true(false)?; return; };
    defer alloc.free(dec_4b);
    expect_eq(dec_4b.len, total_n);
    expect_true(eqBytes(dec_4b[lit_n..total_n], magic_prefix[0..]))?;
    * = maxCompressLen(100);
}

// ============================================================================
// Test 2: 16-Lane SIMD vs Scalar 0-Byte Differential Verification
// ============================================================================
fn runTestSimdZeroByteDiff(): void! {
    let rng: XorShift64 = XorShift64{ state: 0x2468_ACE0_1357_9BDF };
    let src: [256]u8 = [0; 256];
    let dst_simd: [256]u8 = [0; 256];
    let dst_ref: [256]u8 = [0; 256];

    let len: usize = 0;
    while (len <= 129) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) src[i] = @as(u8, @intCast(rng.next() & 0xFF));
        let ds_s: []u8 = dst_simd[0..];
        let dr_s: []u8 = dst_ref[0..];
        snappyCopyLitSimd(ds_s, 5, src[0..len]);
        snappyCopyLitScalar(dr_s, 5, src[0..len]);
        expect_true(eqBytes(dst_simd[5..5 + len], dst_ref[5..5 + len]))?;
    }

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
            snappyCopyMatchSimd(ds_s, 64, off, mlen);
            snappyCopyMatchScalar(dr_s, 64, off, mlen);
            expect_true(eqBytes(dst_simd[0..64 + mlen], dst_ref[0..64 + mlen]))?;
        }
    }

    let a: [160]u8 = [0; 160];
    let shared: usize = 0;
    while (shared <= 120) : (shared += 1) {
        let i: usize = 0;
        while (i < 160) : (i += 1) a[i] = @as(u8, @intCast(rng.next() & 0xFF));
        i = 0;
        while (i < shared) : (i += 1) a[32 + i] = a[i];
        if (32 + shared < 160) a[32 + shared] = a[shared] ^ 0xFF;
        expect_eq(snappyExtendMatchSimd(a[0..], 0, 32, 160), snappyExtendMatchScalar(a[0..], 0, 32, 160));
    }
}

// ============================================================================
// Test 3: Complete Snappy Framing Format (`sNaPpY`) + Masked CRC32C
// ============================================================================
fn runTestSnappyFramingFormat(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Official RFC 3720 CRC32C check value for "123456789" == 0xE3069283 + mask round-trip
    const c_rfc: u32 = snappyCrc32c("123456789");
    expect_eq(c_rfc, 0xE3069283);
    const masked: u32 = snappyMaskChecksum(c_rfc);
    expect_eq(snappyUnmaskChecksum(masked), c_rfc);

    // 2. Multi-chunk (> 64KB: 140KB = 3 chunks, mixing compressible + incompressible PRNG chunks)
    const BIG_LEN: usize = 140000;
    let big = alloc.alloc(u8, BIG_LEN) catch { expect_true(false)?; return; };
    defer alloc.free(big);
    let rng: XorShift64 = XorShift64{ state: 0x5555_AAAA_1234_5678 };
    let i: usize = 0;
    while (i < 65536) : (i += 1) big[i] = @as(u8, @intCast('A' + (i % 16))); // Chunk 0: highly compressible (0x00)
    while (i < 131072) : (i += 1) big[i] = @as(u8, @intCast(rng.next() & 0xFF)); // Chunk 1: incompressible (0x01)
    while (i < BIG_LEN) : (i += 1) big[i] = @as(u8, @intCast('0' + (i % 10))); // Chunk 2: compressible tail

    const framed = compressFramed(alloc, big) catch { expect_true(false)?; return; };
    defer alloc.free(framed);
    const decoded = decompressFramed(alloc, framed) catch { expect_true(false)?; return; };
    defer alloc.free(decoded);
    expect_true(eqBytes(decoded, big))?;

    // 3. Padding (0xFE) + Reserved Skippable (0x85) + Repeated Stream Identifier (0xFF)
    const pad = encodeFramedPaddingChunk(alloc, 12) catch { expect_true(false)?; return; };
    defer alloc.free(pad);
    const skip = encodeFramedSkippableChunk(alloc, 0x85, "skip-me-meta") catch { expect_true(false)?; return; };
    defer alloc.free(skip);
    const f_part = compressFramed(alloc, "snappy framed ok!") catch { expect_true(false)?; return; };
    defer alloc.free(f_part);

    let full = alloc.alloc(u8, f_part.len + pad.len + skip.len + SNAPPY_STREAM_IDENTIFIER.len) catch { expect_true(false)?; return; };
    defer alloc.free(full);
    snappyCopyLitSimd(full, 0, f_part);
    snappyCopyLitSimd(full, f_part.len, pad);
    snappyCopyLitSimd(full, f_part.len + pad.len, skip);
    snappyCopyLitSimd(full, f_part.len + pad.len + skip.len, SNAPPY_STREAM_IDENTIFIER[0..]);

    const dec_full = decompressFramed(alloc, full) catch { expect_true(false)?; return; };
    defer alloc.free(dec_full);
    expect_true(eqBytes(dec_full, "snappy framed ok!"))?;

    // 4. Reserved Unskippable chunk (0x42 in 0x02..0x7F) -> UnsupportedChunkType
    let bad_unskip: [16]u8 = [0; 16];
    snappyCopyLitSimd(bad_unskip[0..], 0, SNAPPY_STREAM_IDENTIFIER[0..]);
    bad_unskip[10] = 0x42;
    bad_unskip[11] = 0x02;
    bad_unskip[12] = 0x00;
    bad_unskip[13] = 0x00;
    let got_unskip: bool = false;
    * = decompressFramed(alloc, bad_unskip[0..]) catch |e| { if (e == SnappyError.UnsupportedChunkType) got_unskip = true; };
    expect_true(got_unskip)?;

    // 5. Single-bit Masked CRC32C tamper -> ChecksumMismatch
    let tam = alloc.alloc(u8, f_part.len) catch { expect_true(false)?; return; };
    defer alloc.free(tam);
    snappyCopyLitSimd(tam, 0, f_part);
    tam[14] ^= 0x01; // flip 1 bit in Masked CRC32C of chunk 0
    let got_crc_err: bool = false;
    * = decompressFramed(alloc, tam) catch |e| { if (e == SnappyError.ChecksumMismatch) got_crc_err = true; };
    expect_true(got_crc_err)?;
    * = SNAPPY_FRAME_CHUNK_SIZE;
}

// ============================================================================
// Test 4: 1,300 Deterministic PRNG Malformed Snappy Raw & Framed Fuzz Cases
// ============================================================================
fn runTestMalformedFuzz1300(): void! {
    const alloc = std.heap.page_allocator;
    let rng: XorShift64 = XorShift64{ state: 0x7777_8888_9999_AAAA };
    let out_buf: [1024]u8 = [0; 1024];
    let out_s: []u8 = out_buf[0..];

    // Explicit edge guards: huge varint (output too large / invalid varint) & offset=0
    const huge_varint: [10]u8 = [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02];
    let got_v_err: bool = false;
    * = decompressInto(huge_varint[0..], out_s) catch |e| {
        if (e == SnappyError.InvalidVarint || e == SnappyError.OutputTooLarge) got_v_err = true;
    };
    expect_true(got_v_err)?;

    const bad_off0: [4]u8 = [0x08, 0x00, 'A', 0x01]; // truncated/offset=0
    let got_o_err: bool = false;
    * = decompressInto(bad_off0[0..], out_s) catch { got_o_err = true; };
    expect_true(got_o_err)?;

    const seed_raw = compress(alloc, "snappy_test_payload_snappy_test_payload_12345678") catch { expect_true(false)?; return; };
    defer alloc.free(seed_raw);
    const seed_frm = compressFramed(alloc, "snappy_framed_payload_snappy_framed_payload_12345678") catch { expect_true(false)?; return; };
    defer alloc.free(seed_frm);

    // Exhaustive truncation of seed raw & seed framed
    let t: usize = 0;
    while (t < seed_raw.len) : (t += 1) {
        * = decompressInto(seed_raw[0..t], out_s) catch 0;
    }
    t = 0;
    while (t < seed_frm.len) : (t += 1) {
        const res = decompressFramedWithMaxSize(alloc, seed_frm[0..t], 1024) catch continue;
        alloc.free(res);
    }

    // 1,300 PRNG malformed Raw & Framed fuzz cases (0 panic / 0 OOB / 0 hang)
    let mut_buf: [256]u8 = [0; 256];
    let trial: usize = 0;
    while (trial < 1300) : (trial += 1) {
        const mode: usize = rng.range(4);
        if (mode == 0) {
            let j: usize = 0;
            while (j < seed_raw.len) : (j += 1) mut_buf[j] = seed_raw[j];
            const flips: usize = rng.range(5) + 1;
            let k: usize = 0;
            while (k < flips) : (k += 1) {
                const pos: usize = rng.range(seed_raw.len);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
            * = decompressInto(mut_buf[0..seed_raw.len], out_s) catch 0;
        } else if (mode == 1) {
            let j: usize = 0;
            while (j < seed_frm.len) : (j += 1) mut_buf[j] = seed_frm[j];
            const flips: usize = rng.range(5) + 1;
            let k: usize = 0;
            while (k < flips) : (k += 1) {
                const pos: usize = rng.range(seed_frm.len);
                mut_buf[pos] ^= @as(u8, @intCast(rng.range(255) + 1));
            }
            const res = decompressFramedWithMaxSize(alloc, mut_buf[0..seed_frm.len], 1024) catch continue;
            alloc.free(res);
        } else if (mode == 2) {
            const flen: usize = rng.range(128);
            let j: usize = 0;
            while (j < flen) : (j += 1) mut_buf[j] = @as(u8, @intCast(rng.next() & 0xFF));
            * = decompressInto(mut_buf[0..flen], out_s) catch 0;
        } else {
            // Valid Stream Identifier + random framed chunk garbage
            const flen: usize = rng.range(120) + 10;
            snappyCopyLitSimd(mut_buf[0..], 0, SNAPPY_STREAM_IDENTIFIER[0..]);
            let j: usize = 10;
            while (j < flen) : (j += 1) mut_buf[j] = @as(u8, @intCast(rng.next() & 0xFF));
            const res = decompressFramedWithMaxSize(alloc, mut_buf[0..flen], 1024) catch continue;
            alloc.free(res);
        }
    }
}

fn main() {
    test("Snappy Raw Block: empty, short 1..20B, low-offset overlap, 4B Copy (70KB) & Into APIs", () => {
        runTestSnappyRawBlockVectors()?;
    });
    test("Snappy SIMD: 16-lane CopyLit, CopyMatch & ExtendMatch 0-byte diff vs scalar", () => {
        runTestSimdZeroByteDiff()?;
    });
    test("Snappy Framing: CRC32C RFC 3720, >64KB multi-chunk, Padding/Skippable/Unskippable & tamper", () => {
        runTestSnappyFramingFormat()?;
    });
    test("Snappy Fuzz: huge Varint, offset=0, truncations & 1,300 PRNG malformed Raw/Framed cases", () => {
        runTestMalformedFuzz1300()?;
    });
}

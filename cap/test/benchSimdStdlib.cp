// Reproducible Standard Library SIMD vs Scalar Performance Benchmark Suite (Lane 2J)
// Covers 8 representative pure-CP SIMD hot paths with strict 0-diff verification & anti-DCE checksums:
//   1. cap:xml    - 16-lane @Vector(16, u8) simdFindXmlTextDelim & simdHasInvalidXmlControl
//   2. cap:ds     - 4-lane @Vector(4, u64) Roaring simdBitmapAnd / simdBitmapOr / simdBitmapPopcount
//   3. cap:ds     - 16-lane @Vector(16, u8) HyperLogLog @max simdHllMergeRegisters
//   4. cap:vector - 4-lane @Vector(4, f32) HNSW/IVF-PQ simdVecIdxL2SqF32 & simdVecIdxDotF32
//   5. cap:math   - 4-lane @Vector(4, f64) StatsDist simdMathDotF64 & simdMathSumSqDiffF64
//   6. cap:net    - 8-lane @Vector(8, u32) RFC 1071 simdIcmpChecksumFold
//   7. cap:auth   - 16-lane @Vector(16, u8) constant-time simdAuthCtEqual (no early exit)
//   8. cap:text   - 16-lane @Vector(16, u8) Myers diff simdCommonPrefixLen & simdFindNewline

const std = @import("std");
const builtin = @import("builtin");
import { test, expect_eq } from "cap:test";
import { HwTimer } from "../../lib/bench/timer.cp";
import {
    simdFindXmlTextDelim,
    simdHasInvalidXmlControl,
} from "../../lib/xml/parser.cp";
import {
    simdBitmapAnd,
    simdBitmapOr,
    simdBitmapPopcount,
} from "../../lib/ds/roaring.cp";
import {
    simdHllMergeRegisters,
} from "../../lib/ds/hll.cp";
import {
    simdVecIdxL2SqF32,
    simdVecIdxDotF32,
} from "../../lib/vector/hnswFlat.cp";
import {
    simdMathDotF64,
    simdMathSumSqDiffF64,
} from "../../lib/math/statsDist.cp";
import {
    simdIcmpChecksumFold,
} from "../../lib/net/icmp.cp";
import {
    simdAuthCtEqual,
} from "../../lib/auth/totpHotp.cp";
import {
    simdCommonPrefixLen,
    simdFindNewline,
} from "../../lib/text/myersDiff.cp";

fn scaleIters(base: usize): usize {
    const mult: usize = if (builtin.mode == .debug) 1 else 256;
    return base * mult;
}

fn nsToMs(ns: u64): f64 {
    const clamped: u64 = if (ns > 1) ns else 1;
    return @as(f64, @floatFromInt(clamped)) / 1.0e6;
}

fn bytesPerSecToMBs(total_bytes: u64, elapsed_ns: u64): f64 {
    const clamped: u64 = if (elapsed_ns > 1) elapsed_ns else 1;
    const mb: f64 = @as(f64, @floatFromInt(total_bytes)) / (1024.0 * 1024.0);
    const sec: f64 = @as(f64, @floatFromInt(clamped)) / 1.0e9;
    return mb / sec;
}

fn opsPerSecToMops(total_ops: u64, elapsed_ns: u64): f64 {
    const clamped: u64 = if (elapsed_ns > 1) elapsed_ns else 1;
    const mops: f64 = @as(f64, @floatFromInt(total_ops)) / 1.0e6;
    const sec: f64 = @as(f64, @floatFromInt(clamped)) / 1.0e9;
    return mops / sec;
}

fn benchAbsF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

// ============================================================================
// Scalar Reference Implementations (exact semantic equivalents of lib/* SIMD)
// ============================================================================

fn refFindXmlTextDelim(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b == '<' || b == '&' || b == ']') {
            return i;
        }
    }
    return null;
}

fn refHasInvalidXmlControl(data: []const u8): bool {
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const b = data[i];
        if (b < 0x20 && b != 0x09 && b != 0x0A && b != 0x0D) {
            return true;
        }
    }
    return false;
}

fn refBitmapPopcount(words: []const u64): u32 {
    let total: u32 = 0;
    let i: usize = 0;
    while (i < words.len) : (i += 1) {
        total += @as(u32, @popCount(words[i]));
    }
    return total;
}

fn refBitmapAnd(dst: []u64, a: []const u64, b: []const u64): u32 {
    let n = dst.len;
    if (a.len < n) n = a.len;
    if (b.len < n) n = b.len;
    let card: u32 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const w = a[i] & b[i];
        dst[i] = w;
        card += @as(u32, @popCount(w));
    }
    return card;
}

fn refBitmapOr(dst: []u64, a: []const u64, b: []const u64): u32 {
    let n = dst.len;
    if (a.len < n) n = a.len;
    if (b.len < n) n = b.len;
    let card: u32 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const w = a[i] | b[i];
        dst[i] = w;
        card += @as(u32, @popCount(w));
    }
    return card;
}

fn refHllMergeRegisters(dst: []u8, src: []const u8): usize {
    const n = if (dst.len <= src.len) dst.len else src.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (src[i] > dst[i]) {
            dst[i] = src[i];
        }
    }
    return n;
}

fn refVecIdxL2SqF32(a: []const f32, b: []const f32): f32 {
    const n = if (a.len <= b.len) a.len else b.len;
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const d: f32 = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

fn refVecIdxDotF32(a: []const f32, b: []const f32): f32 {
    const n = if (a.len <= b.len) a.len else b.len;
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        sum += a[i] * b[i];
    }
    return sum;
}

fn refMathDotF64(xs: []const f64, ys: []const f64): f64 {
    const n = if (xs.len <= ys.len) xs.len else ys.len;
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const a: f64 = xs[i];
        const b: f64 = ys[i];
        if (!std.math.isFinite(a) || !std.math.isFinite(b)) {
            return std.math.nan(f64);
        }
        total += a * b;
    }
    return total;
}

fn refMathSumSqDiffF64(xs: []const f64, mean: f64): f64 {
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        const x: f64 = xs[i];
        if (!std.math.isFinite(x)) {
            return std.math.nan(f64);
        }
        const d: f64 = x - mean;
        total += d * d;
    }
    return total;
}

fn refIcmpFoldU64(acc_in: u64): u16 {
    let acc: u64 = acc_in;
    while ((acc >> 16) != 0) {
        acc = (acc & 0xFFFF) + (acc >> 16);
    }
    return @as(u16, @intCast(acc & 0xFFFF));
}

fn refIcmpChecksumFold(acc_in: u32, data: []const u8): u16 {
    let acc: u64 = acc_in as u64;
    let i: usize = 0;
    while (i + 1 < data.len) : (i += 2) {
        const word: u64 = ((data[i] as u64) << 8) | (data[i + 1] as u64);
        acc += word;
    }
    if (i < data.len) {
        acc += (data[i] as u64) << 8;
    }
    return refIcmpFoldU64(acc);
}

fn refAuthCtEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        diff = diff | (a[i] ^ b[i]);
    }
    return diff == 0;
}

fn refCommonPrefixLen(a: []const u8, b: []const u8): usize {
    const limit = if (a.len <= b.len) a.len else b.len;
    let i: usize = 0;
    while (i < limit && a[i] == b[i]) : (i += 1) {}
    return i;
}

fn refFindNewline(data: []const u8, start: usize): usize? {
    if (start >= data.len) return null;
    let i: usize = start;
    while (i < data.len) : (i += 1) {
        if (data[i] == '\n') {
            return i;
        }
    }
    return null;
}

// ============================================================================
// 1. cap:xml (16-lane simdFindXmlTextDelim & simdHasInvalidXmlControl)
// ============================================================================

fn bench1XmlScanner(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 16384;
    const ITERS: usize = scaleIters(4);
    let buf = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(buf);

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const c: u8 = @as(u8, @intCast('a' + (i % 26)));
        buf[i] = c;
    }
    // Place XML delimiters near the tail so scanners traverse almost the entire 16 KB buffer
    buf[N - 17] = '<';
    buf[N - 5] = '&';

    // 0-diff verification
    expect_eq(simdFindXmlTextDelim(buf, 0), refFindXmlTextDelim(buf, 0));
    expect_eq(simdFindXmlTextDelim(buf, N - 16), refFindXmlTextDelim(buf, N - 16));
    expect_eq(simdHasInvalidXmlControl(buf), refHasInvalidXmlControl(buf));

    // Warmup
    let w: usize = 0;
    while (w < 4) : (w += 1) {
        std.mem.doNotOptimizeAway(simdFindXmlTextDelim(buf, 0));
        std.mem.doNotOptimizeAway(refFindXmlTextDelim(buf, 0));
    }

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const start: usize = it & 7;
        const p = refFindXmlTextDelim(buf, start) orelse 0;
        const bad = refHasInvalidXmlControl(buf);
        scalar_chk +%= @as(u64, p) + (if (bad) @as(u64, 1) else @as(u64, 0));
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const start: usize = it2 & 7;
        const p = simdFindXmlTextDelim(buf, start) orelse 0;
        const bad = simdHasInvalidXmlControl(buf);
        simd_chk +%= @as(u64, p) + (if (bad) @as(u64, 1) else @as(u64, 0));
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    // 2 full-buffer scans per iteration
    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, N) * 2;
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 1/8 cap:xml 16B-scan] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

// ============================================================================
// 2. cap:ds Roaring Bitmap (4-lane u64 simdBitmapAnd/Or/Popcount)
// ============================================================================

fn bench2RoaringBitmap(): void! {
    const alloc = std.heap.page_allocator;
    const WORDS: usize = 4096; // 32 KB per bitmap
    const ITERS: usize = scaleIters(4);
    let a = alloc.alloc(u64, WORDS) catch { expect_true(false)?; return; };
    defer alloc.free(a);
    let b = alloc.alloc(u64, WORDS) catch { expect_true(false)?; return; };
    defer alloc.free(b);
    let dst_s = alloc.alloc(u64, WORDS) catch { expect_true(false)?; return; };
    defer alloc.free(dst_s);
    let dst_v = alloc.alloc(u64, WORDS) catch { expect_true(false)?; return; };
    defer alloc.free(dst_v);

    let i: usize = 0;
    while (i < WORDS) : (i += 1) {
        const x: u64 = (@as(u64, i) *% 0x517CC1B727220A95) ^ 0x5A5A5A5A5A5A5A5A;
        const y: u64 = (@as(u64, i) *% 0x2545F4914F6CDD1D) ^ 0x5555555533333333;
        a[i] = x;
        b[i] = y;
    }

    // 0-diff verification
    const c_and_s = refBitmapAnd(dst_s, a, b);
    const c_and_v = simdBitmapAnd(dst_v, a, b);
    expect_eq(c_and_v, c_and_s);
    let k: usize = 0;
    while (k < WORDS) : (k += 1) {
        expect_eq(dst_v[k], dst_s[k]);
    }
    const c_or_s = refBitmapOr(dst_s, a, b);
    const c_or_v = simdBitmapOr(dst_v, a, b);
    expect_eq(c_or_v, c_or_s);
    expect_eq(simdBitmapPopcount(dst_v), refBitmapPopcount(dst_s));

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 3;
        const c1 = refBitmapAnd(dst_s[off..], a[off..], b[off..]);
        const c2 = refBitmapOr(dst_s[off..], a[off..], b[off..]);
        const c3 = refBitmapPopcount(dst_s[off..]);
        scalar_chk +%= @as(u64, c1) + @as(u64, c2) + @as(u64, c3);
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 3;
        const c1 = simdBitmapAnd(dst_v[off..], a[off..], b[off..]);
        const c2 = simdBitmapOr(dst_v[off..], a[off..], b[off..]);
        const c3 = simdBitmapPopcount(dst_v[off..]);
        simd_chk +%= @as(u64, c1) + @as(u64, c2) + @as(u64, c3);
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    // Total bytes processed across AND (read 2 + write 1), OR (read 2 + write 1), POPCOUNT (read 1) => 7 * 32KB
    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, WORDS) * 8 * 7;
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 2/8 cap:ds Roaring 4xu64] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

// ============================================================================
// 3. cap:ds HyperLogLog (16-lane @max simdHllMergeRegisters)
// ============================================================================

fn bench3HllMerge(): void! {
    const alloc = std.heap.page_allocator;
    const REGS: usize = 16384; // p = 14 standard HLL register count
    const ITERS: usize = scaleIters(8);
    let src = alloc.alloc(u8, REGS) catch { expect_true(false)?; return; };
    defer alloc.free(src);
    let base = alloc.alloc(u8, REGS) catch { expect_true(false)?; return; };
    defer alloc.free(base);
    let dst_s = alloc.alloc(u8, REGS) catch { expect_true(false)?; return; };
    defer alloc.free(dst_s);
    let dst_v = alloc.alloc(u8, REGS) catch { expect_true(false)?; return; };
    defer alloc.free(dst_v);

    let i: usize = 0;
    while (i < REGS) : (i += 1) {
        base[i] = @as(u8, @intCast((i * 13 + 7) & 63));
        src[i] = @as(u8, @intCast((i * 29 + 3) & 63));
        dst_s[i] = base[i];
        dst_v[i] = base[i];
    }

    // 0-diff verification
    expect_eq(simdHllMergeRegisters(dst_v, src), refHllMergeRegisters(dst_s, src));
    let k: usize = 0;
    while (k < REGS) : (k += 1) {
        expect_eq(dst_v[k], dst_s[k]);
    }

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 15;
        dst_s[off] = base[off];
        const merged = refHllMergeRegisters(dst_s[off..], src[off..]);
        scalar_chk +%= @as(u64, merged) + @as(u64, dst_s[off]);
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 15;
        dst_v[off] = base[off];
        const merged = simdHllMergeRegisters(dst_v[off..], src[off..]);
        simd_chk +%= @as(u64, merged) + @as(u64, dst_v[off]);
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    // Read dst + read src + write dst => 3 * REGS bytes per merge
    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, REGS) * 3;
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 3/8 cap:ds HLL 16B-max] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

// ============================================================================
// 4. cap:vector HNSW/IVF-PQ (4-lane f32 simdVecIdxL2SqF32 & simdVecIdxDotF32)
// ============================================================================

fn bench4VectorDistF32(): void! {
    const alloc = std.heap.page_allocator;
    const DIM: usize = 1536; // Standard OpenAI embedding dimension
    const ITERS: usize = scaleIters(16);
    let a = alloc.alloc(f32, DIM) catch { expect_true(false)?; return; };
    defer alloc.free(a);
    let b = alloc.alloc(f32, DIM) catch { expect_true(false)?; return; };
    defer alloc.free(b);

    let i: usize = 0;
    while (i < DIM) : (i += 1) {
        a[i] = (@as(f32, @floatFromInt((i * 19) % 101)) - 50.0) * 0.02;
        b[i] = (@as(f32, @floatFromInt((i * 37) % 97)) - 48.0) * 0.02;
    }

    // 0-diff verification (floating-point reduction tolerance < 1e-3)
    const l2_s = refVecIdxL2SqF32(a, b);
    const l2_v = simdVecIdxL2SqF32(a, b);
    const dot_s = refVecIdxDotF32(a, b);
    const dot_v = simdVecIdxDotF32(a, b);
    expect_true(benchAbsF64(@as(f64, l2_v) - @as(f64, l2_s)) / (benchAbsF64(@as(f64, l2_s)) + 1.0) < 1.0e-4)?;
    expect_true(benchAbsF64(@as(f64, dot_v) - @as(f64, dot_s)) / (benchAbsF64(@as(f64, dot_s)) + 1.0) < 1.0e-4)?;

    // Scalar timing
    let scalar_acc: f64 = 0.0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 3;
        scalar_acc += @as(f64, refVecIdxL2SqF32(a[off..], b[off..]));
        scalar_acc += @as(f64, refVecIdxDotF32(a[off..], b[off..]));
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_acc: f64 = 0.0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 3;
        simd_acc += @as(f64, simdVecIdxL2SqF32(a[off..], b[off..]));
        simd_acc += @as(f64, simdVecIdxDotF32(a[off..], b[off..]));
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    const rel_err: f64 = benchAbsF64(simd_acc - scalar_acc) / (benchAbsF64(scalar_acc) + 1.0);
    expect_true(rel_err < 1.0e-5)?;
    const chk: u64 = @as(u64, @intFromFloat(benchAbsF64(simd_acc) * 100.0));
    std.mem.doNotOptimizeAway(chk);

    // 2 kernels x DIM elements
    const total_ops: u64 = @as(u64, ITERS) * @as(u64, DIM) * 2;
    const scalar_mops: f64 = opsPerSecToMops(total_ops, scalar_ns);
    const simd_mops: f64 = opsPerSecToMops(total_ops, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 4/8 cap:vector 4xf32] Scalar={d:.3}ms ({d:.1} Mops/s) | SIMD={d:.3}ms ({d:.1} Mops/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mops, nsToMs(simd_ns), simd_mops, speedup, chk },
    );
}

// ============================================================================
// 5. cap:math StatsDist (4-lane f64 simdMathDotF64 & simdMathSumSqDiffF64)
// ============================================================================

fn bench5MathStatsF64(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 4096;
    const ITERS: usize = scaleIters(8);
    let xs = alloc.alloc(f64, N) catch { expect_true(false)?; return; };
    defer alloc.free(xs);
    let ys = alloc.alloc(f64, N) catch { expect_true(false)?; return; };
    defer alloc.free(ys);

    let mean: f64 = 0.0;
    let i: usize = 0;
    while (i < N) : (i += 1) {
        xs[i] = (@as(f64, @floatFromInt((i * 23) % 103)) - 51.0) * 0.05;
        ys[i] = (@as(f64, @floatFromInt((i * 41) % 89)) - 44.0) * 0.05;
        mean += xs[i];
    }
    mean /= @as(f64, @floatFromInt(N));

    // 0-diff verification
    const dot_s = refMathDotF64(xs, ys);
    const dot_v = simdMathDotF64(xs, ys) catch { expect_true(false)?; return; };
    const ssd_s = refMathSumSqDiffF64(xs, mean);
    const ssd_v = simdMathSumSqDiffF64(xs, mean) catch { expect_true(false)?; return; };
    expect_true(benchAbsF64(dot_v - dot_s) < 1.0e-8)?;
    expect_true(benchAbsF64(ssd_v - ssd_s) < 1.0e-8)?;

    // Scalar timing
    let scalar_acc: f64 = 0.0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 3;
        scalar_acc += refMathDotF64(xs[off..], ys[off..]);
        scalar_acc += refMathSumSqDiffF64(xs[off..], mean);
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_acc: f64 = 0.0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 3;
        simd_acc += simdMathDotF64(xs[off..], ys[off..]) catch 0.0;
        simd_acc += simdMathSumSqDiffF64(xs[off..], mean) catch 0.0;
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    const rel_err: f64 = benchAbsF64(simd_acc - scalar_acc) / (benchAbsF64(scalar_acc) + 1.0);
    expect_true(rel_err < 1.0e-11)?;
    const chk: u64 = @as(u64, @intFromFloat(benchAbsF64(simd_acc) * 10.0));
    std.mem.doNotOptimizeAway(chk);

    const total_ops: u64 = @as(u64, ITERS) * @as(u64, N) * 2;
    const scalar_mops: f64 = opsPerSecToMops(total_ops, scalar_ns);
    const simd_mops: f64 = opsPerSecToMops(total_ops, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 5/8 cap:math 4xf64] Scalar={d:.3}ms ({d:.1} Mops/s) | SIMD={d:.3}ms ({d:.1} Mops/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mops, nsToMs(simd_ns), simd_mops, speedup, chk },
    );
}

// ============================================================================
// 6. cap:net ICMP RFC 1071 (8-lane u32 simdIcmpChecksumFold)
// ============================================================================

fn bench6NetIcmpChecksum(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 16384; // 16 KB jumbo payload
    const ITERS: usize = scaleIters(8);
    let pkt = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(pkt);

    let i: usize = 0;
    while (i < N) : (i += 1) {
        pkt[i] = @as(u8, @intCast((i * 73 + 19) & 255));
    }

    // 0-diff verification
    expect_eq(simdIcmpChecksumFold(0, pkt), refIcmpChecksumFold(0, pkt));
    expect_eq(simdIcmpChecksumFold(0x12345, pkt[1..]), refIcmpChecksumFold(0x12345, pkt[1..]));

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 7;
        const sum: u16 = refIcmpChecksumFold(@as(u32, @intCast(it & 0xFFFF)), pkt[off..]);
        scalar_chk +%= @as(u64, sum);
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 7;
        const sum: u16 = simdIcmpChecksumFold(@as(u32, @intCast(it2 & 0xFFFF)), pkt[off..]);
        simd_chk +%= @as(u64, sum);
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, N);
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 6/8 cap:net ICMP 8xu32] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

// ============================================================================
// 7. cap:auth Constant-Time Compare (16-lane no-early-exit simdAuthCtEqual)
// ============================================================================

fn bench7AuthCtEqual(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 4096;
    const ITERS: usize = scaleIters(16);
    let a = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(a);
    let b_eq = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(b_eq);
    let b_ne = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(b_ne);

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const v: u8 = @as(u8, @intCast((i * 53 + 11) & 255));
        a[i] = v;
        b_eq[i] = v;
        b_ne[i] = v;
    }
    b_ne[0] ^= 0x01; // Differ at byte 0: constant-time must still scan full length

    // 0-diff verification
    expect_eq(simdAuthCtEqual(a, b_eq), refAuthCtEqual(a, b_eq));
    expect_eq(simdAuthCtEqual(a, b_ne), refAuthCtEqual(a, b_ne));
    expect_eq(simdAuthCtEqual(a, b_eq), true);
    expect_eq(simdAuthCtEqual(a, b_ne), false);

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 7;
        const r1 = refAuthCtEqual(a[off..], b_eq[off..]);
        const r2 = refAuthCtEqual(a[off..], b_ne[off..]);
        scalar_chk +%= (if (r1) @as(u64, 1) else @as(u64, 0)) + (if (r2) @as(u64, 2) else @as(u64, 0));
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 7;
        const r1 = simdAuthCtEqual(a[off..], b_eq[off..]);
        const r2 = simdAuthCtEqual(a[off..], b_ne[off..]);
        simd_chk +%= (if (r1) @as(u64, 1) else @as(u64, 0)) + (if (r2) @as(u64, 2) else @as(u64, 0));
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    // 2 calls x 2 buffers read per call = 4 * N bytes
    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, N) * 4;
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 7/8 cap:auth CtEqual 16B] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

// ============================================================================
// 8. cap:text Myers Diff (16-lane simdCommonPrefixLen & simdFindNewline)
// ============================================================================

fn bench8TextMyersScan(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 16384;
    const ITERS: usize = scaleIters(8);
    let a = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(a);
    let b = alloc.alloc(u8, N) catch { expect_true(false)?; return; };
    defer alloc.free(b);

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const ch: u8 = @as(u8, @intCast('A' + (i % 26)));
        a[i] = ch;
        b[i] = ch;
    }
    b[N - 13] = '!';
    a[N - 9] = '\n';

    // 0-diff verification
    expect_eq(simdCommonPrefixLen(a, b), refCommonPrefixLen(a, b));
    expect_eq(simdFindNewline(a, 0), refFindNewline(a, 0));

    // Scalar timing
    let scalar_chk: u64 = 0;
    const t0: u64 = HwTimer.nowNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        const off: usize = it & 7;
        const pfx = refCommonPrefixLen(a[off..], b[off..]);
        const nl = refFindNewline(a, off) orelse 0;
        scalar_chk +%= @as(u64, pfx) + @as(u64, nl);
    }
    const scalar_ns: u64 = HwTimer.nowNs() - t0;

    // SIMD timing
    let simd_chk: u64 = 0;
    const t1: u64 = HwTimer.nowNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        const off: usize = it2 & 7;
        const pfx = simdCommonPrefixLen(a[off..], b[off..]);
        const nl = simdFindNewline(a, off) orelse 0;
        simd_chk +%= @as(u64, pfx) + @as(u64, nl);
    }
    const simd_ns: u64 = HwTimer.nowNs() - t1;

    expect_eq(simd_chk, scalar_chk);
    std.mem.doNotOptimizeAway(simd_chk);

    // CommonPrefix reads 2*N + FindNewline reads 1*N => 3 * N bytes
    const total_bytes: u64 = @as(u64, ITERS) * @as(u64, N) * 3;
    const scalar_mbs: f64 = bytesPerSecToMBs(total_bytes, scalar_ns);
    const simd_mbs: f64 = bytesPerSecToMBs(total_bytes, simd_ns);
    const speedup: f64 = @as(f64, @floatFromInt(if (scalar_ns > 1) scalar_ns else 1)) /
        @as(f64, @floatFromInt(if (simd_ns > 1) simd_ns else 1));

    std.debug.print(
        "[BENCH 8/8 cap:text Myers 16B] Scalar={d:.3}ms ({d:.1} MB/s) | SIMD={d:.3}ms ({d:.1} MB/s) | Speedup={d:.2}x | chk={d}\n",
        .{ nsToMs(scalar_ns), scalar_mbs, nsToMs(simd_ns), simd_mbs, speedup, simd_chk },
    );
}

pub fn main() {
    test("Bench 1/8: cap:xml 16-lane simdFindXmlTextDelim & simdHasInvalidXmlControl", () => {
        bench1XmlScanner()?;
    });
    test("Bench 2/8: cap:ds Roaring 4-lane u64 simdBitmapAnd/Or/Popcount", () => {
        bench2RoaringBitmap()?;
    });
    test("Bench 3/8: cap:ds HyperLogLog 16-lane @max simdHllMergeRegisters", () => {
        bench3HllMerge()?;
    });
    test("Bench 4/8: cap:vector HNSW 4-lane f32 simdVecIdxL2SqF32 & simdVecIdxDotF32", () => {
        bench4VectorDistF32()?;
    });
    test("Bench 5/8: cap:math StatsDist 4-lane f64 simdMathDotF64 & simdMathSumSqDiffF64", () => {
        bench5MathStatsF64()?;
    });
    test("Bench 6/8: cap:net ICMP 8-lane RFC 1071 simdIcmpChecksumFold", () => {
        bench6NetIcmpChecksum()?;
    });
    test("Bench 7/8: cap:auth Constant-Time 16-lane simdAuthCtEqual", () => {
        bench7AuthCtEqual()?;
    });
    test("Bench 8/8: cap:text Myers Diff 16-lane simdCommonPrefixLen & simdFindNewline", () => {
        bench8TextMyersScan()?;
    });
}

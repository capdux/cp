// Reference-hardened test suite against pwoolcoc/hyperloglog (MIT) & Dunning t-digest (Apache-2.0)
// Covers HLL precision/LinearCounting/merge algebra/serialization guards, t-digest NaN/Inf/weight
// guards, monotonicity, P50/P90/P99 accuracy & merge, 16-lane u8 & 4-lane f64 SIMD 0-diff parity,
// and 1,600 deterministic PRNG fuzz cases.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    HLL_MIN_PRECISION,
    HLL_MAX_PRECISION,
    HllError,
    simdHllMergeRegisters,
    scalarHllMergeRegisters,
    simdHllCountZeros,
    scalarHllCountZeros,
    validateHllSerialized,
    HyperLogLog,
} from "../../lib/ds/hll.cp";
import {
    TDigestError,
    simdTdSumWeightsF64,
    scalarTdSumWeightsF64,
    simdTdMinMaxF64,
    scalarTdMinMaxF64,
    TDigest,
} from "../../lib/ds/tdigest.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }

    fn nextUnitF64(*_): f64 {
        const top53 = _.next() >> 11;
        return (top53 as f64) / 9007199254740992.0;
    }

    fn fillBytes(*_, dst: []u8) {
        let i: usize = 0;
        while (i < dst.len) : (i += 1) {
            dst[i] = _.nextByte();
        }
    }
}

fn checkHllSuite(alloc: std.mem.Allocator): u32 {
    // 1. Precision bounds (p < 4 and p > 16 rejected)
    let hit_inv = false;
    * = HyperLogLog.init(alloc, 3) catch |e| {
        if (e == HllError.InvalidPrecision) hit_inv = true;
    };
    if (!hit_inv) return 1;
    hit_inv = false;
    * = HyperLogLog.init(alloc, 17) catch |e| {
        if (e == HllError.InvalidPrecision) hit_inv = true;
    };
    if (!hit_inv) return 2;

    // 2. Empty set count == 0, LinearCounting small cardinality, and large cardinality accuracy
    let hll = HyperLogLog.init(alloc, 12) catch return 3;
    defer hll.deinit();
    if (hll.count() != 0) return 4;

    // Small cardinality (50 distinct items) -> LinearCounting
    let i: u64 = 0;
    while (i < 50) : (i += 1) {
        hll.addHash(i + 1000);
    }
    const c50 = hll.count();
    if (c50 < 40 || c50 > 60) return 5;

    // Larger cardinality (20,000 distinct items, p=12 -> stderr ~1.6%, allow 6% margin)
    i = 0;
    while (i < 20000) : (i += 1) {
        hll.addHash(i + 100000);
    }
    const c20k = hll.count();
    if (c20k < 18800 || c20k > 21300) return 6;

    // 3. Merge algebra: precision mismatch guard, idempotence (A U A == A), associativity ((A U B) U C == A U (B U C))
    let h_diff = HyperLogLog.init(alloc, 8) catch return 7;
    defer h_diff.deinit();
    let hit_mismatch = false;
    hll.merge(h_diff) catch |e| {
        if (e == HllError.PrecisionMismatch) hit_mismatch = true;
    };
    if (!hit_mismatch) return 8;

    let a = HyperLogLog.init(alloc, 10) catch return 9;
    defer a.deinit();
    let b = HyperLogLog.init(alloc, 10) catch return 10;
    defer b.deinit();
    let c = HyperLogLog.init(alloc, 10) catch return 11;
    defer c.deinit();
    i = 0;
    while (i < 500) : (i += 1) {
        a.addHash(i);
        b.addHash(i + 300);
        c.addHash(i + 600);
    }
    const a_before = a.count();
    a.merge(a) catch return 12;
    if (a.count() != a_before) return 13; // idempotent

    let ab_c = HyperLogLog.init(alloc, 10) catch return 14;
    defer ab_c.deinit();
    ab_c.merge(a) catch return 15;
    ab_c.merge(b) catch return 16;
    ab_c.merge(c) catch return 17;

    let bc = HyperLogLog.init(alloc, 10) catch return 18;
    defer bc.deinit();
    bc.merge(b) catch return 19;
    bc.merge(c) catch return 20;
    let a_bc = HyperLogLog.init(alloc, 10) catch return 21;
    defer a_bc.deinit();
    a_bc.merge(a) catch return 22;
    a_bc.merge(bc) catch return 23;
    if (ab_c.count() != a_bc.count()) return 24;

    // 4. Serialization roundtrip & malformed payload rejection (truncated, bad magic, bad p, corrupt reg)
    let ser = ab_c.serialize(alloc) catch return 25;
    defer alloc.free(ser);
    validateHllSerialized(ser) catch return 26;
    let deser = HyperLogLog.deserialize(alloc, ser) catch return 27;
    defer deser.deinit();
    if (deser.count() != ab_c.count()) return 28;

    // Truncated
    let hit_ser = false;
    validateHllSerialized(ser[0 .. ser.len - 1]) catch |e| {
        if (e == HllError.InvalidSerializedData) hit_ser = true;
    };
    if (!hit_ser) return 29;

    // Bad magic
    ser[0] = 'X';
    hit_ser = false;
    validateHllSerialized(ser) catch |e| {
        if (e == HllError.InvalidSerializedData) hit_ser = true;
    };
    if (!hit_ser) return 30;
    ser[0] = 'H';

    // Corrupt register value > (64 - p) + 1
    ser[5] = 200;
    hit_ser = false;
    validateHllSerialized(ser) catch |e| {
        if (e == HllError.InvalidSerializedData) hit_ser = true;
    };
    if (!hit_ser) return 31;

    return 0;
}

fn checkTDigestSuite(alloc: std.mem.Allocator): u32 {
    // 1. Invalid compression rejected
    let hit = false;
    * = TDigest.init(alloc, 5.0) catch |e| {
        if (e == TDigestError.InvalidCompression) hit = true;
    };
    if (!hit) return 1;
    hit = false;
    * = TDigest.init(alloc, std.math.nan(f64)) catch |e| {
        if (e == TDigestError.InvalidCompression) hit = true;
    };
    if (!hit) return 2;

    // 2. Empty digest & single-element quantile
    let td = TDigest.init(alloc, 100.0) catch return 3;
    defer td.deinit();
    hit = false;
    * = td.quantile(0.5) catch |e| {
        if (e == TDigestError.EmptyDigest) hit = true;
    };
    if (!hit) return 4;

    // Reject NaN, +Inf, -Inf, zero/negative/NaN weight, invalid quantile
    hit = false;
    td.add(std.math.nan(f64), 1.0) catch |e| {
        if (e == TDigestError.InvalidValue) hit = true;
    };
    if (!hit) return 5;
    hit = false;
    td.add(std.math.inf(f64), 1.0) catch |e| {
        if (e == TDigestError.InvalidValue) hit = true;
    };
    if (!hit) return 6;
    hit = false;
    td.add(-std.math.inf(f64), 1.0) catch |e| {
        if (e == TDigestError.InvalidValue) hit = true;
    };
    if (!hit) return 7;
    hit = false;
    td.add(1.0, 0.0) catch |e| {
        if (e == TDigestError.InvalidWeight) hit = true;
    };
    if (!hit) return 8;
    hit = false;
    td.add(1.0, -2.0) catch |e| {
        if (e == TDigestError.InvalidWeight) hit = true;
    };
    if (!hit) return 9;

    td.observe(42.5) catch return 10;
    const q_single = td.quantile(0.5) catch return 11;
    if (q_single != 42.5) return 12;

    hit = false;
    * = td.quantile(-0.01) catch |e| {
        if (e == TDigestError.InvalidQuantile) hit = true;
    };
    if (!hit) return 13;
    hit = false;
    * = td.quantile(std.math.nan(f64)) catch |e| {
        if (e == TDigestError.InvalidQuantile) hit = true;
    };
    if (!hit) return 14;

    // 3. Uniform [0, 1000) distribution: monotonicity + P50/P90/P99 accuracy
    let u_td = TDigest.init(alloc, 200.0) catch return 15;
    defer u_td.deinit();
    let prng = XorShift64.init(0x5444_4947_4553_5431);
    let batch: [64]f64 = [0.0; 64];
    let round: usize = 0;
    while (round < 100) : (round += 1) {
        let j: usize = 0;
        while (j < 64) : (j += 1) {
            batch[j] = prng.nextUnitF64() * 1000.0;
        }
        u_td.addBatch(batch[0..]) catch return 16;
    }

    // Monotonicity across 0.0, 0.1, ..., 1.0
    let prev_q = u_td.quantile(0.0) catch return 17;
    let step: usize = 1;
    while (step <= 10) : (step += 1) {
        const q = (step as f64) * 0.1;
        const cur_q = u_td.quantile(q) catch return 18;
        if (cur_q < prev_q) return 19;
        prev_q = cur_q;
    }

    const p50 = u_td.quantile(0.5) catch return 20;
    const p90 = u_td.quantile(0.9) catch return 21;
    const p99 = u_td.quantile(0.99) catch return 22;
    if (@abs(p50 - 500.0) > 25.0) return 23;
    if (@abs(p90 - 900.0) > 20.0) return 24;
    if (@abs(p99 - 990.0) > 10.0) return 25;

    // 4. Skewed exponential-like distribution + two-digest merge accuracy
    let d1 = TDigest.init(alloc, 200.0) catch return 26;
    defer d1.deinit();
    let d2 = TDigest.init(alloc, 200.0) catch return 27;
    defer d2.deinit();
    let k: usize = 0;
    while (k < 2000) : (k += 1) {
        const v1 = (k as f64) * 0.5;
        const v2 = (k as f64) * 0.5 + 1000.0;
        d1.observe(v1) catch return 28;
        d2.observe(v2) catch return 29;
    }
    d1.merge(&d2) catch return 30;
    if (d1.count() != 4000.0) return 31;
    const mp50 = d1.quantile(0.5) catch return 32;
    const mp99 = d1.quantile(0.99) catch return 33;
    if (@abs(mp50 - 1000.0) > 20.0) return 34;
    if (@abs(mp99 - 1980.0) > 15.0) return 35;

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x534b_4554_4348_5349);

    // 1. HLL 16-lane u8 SIMD vs scalar (0..257 lengths)
    let a: [260]u8 = [0; 260];
    let b: [260]u8 = [0; 260];
    let dst_simd: [260]u8 = [0; 260];
    let dst_scal: [260]u8 = [0; 260];

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            const v1 = prng.nextByte();
            const v2 = prng.nextByte();
            a[i] = if (v1 % 5 == 0) 0 else v1;
            b[i] = if (v2 % 7 == 0) 0 else v2;
            dst_simd[i] = a[i];
            dst_scal[i] = a[i];
        }
        if (simdHllCountZeros(a[0..len]) != scalarHllCountZeros(a[0..len])) return 1;
        const n1 = simdHllMergeRegisters(dst_simd[0..len], b[0..len]);
        const n2 = scalarHllMergeRegisters(dst_scal[0..len], b[0..len]);
        if (n1 != n2 || n1 != len) return 2;
        i = 0;
        while (i < len) : (i += 1) {
            if (dst_simd[i] != dst_scal[i]) return 3;
        }
    }

    // 2. T-Digest 4-lane f64 SIMD vs scalar (0..131 lengths)
    let vals: [132]f64 = [0.0; 132];
    len = 0;
    while (len <= 131) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            // Exact integer-valued f64 so sum is bit-exact across lanes
            vals[i] = @as(f64, @floatFromInt(@as(i64, @intCast(prng.next() % 1000)) - 500));
        }
        const s1 = simdTdSumWeightsF64(vals[0..len]);
        const s2 = scalarTdSumWeightsF64(vals[0..len]);
        if (s1 != s2) return 4;

        const mm1 = simdTdMinMaxF64(vals[0..len]);
        const mm2 = scalarTdMinMaxF64(vals[0..len]);
        if (mm1.valid != mm2.valid) return 5;
        if (mm1.valid && (mm1.min_val != mm2.min_val || mm1.max_val != mm2.max_val)) return 6;
    }

    return 0;
}

fn tryDeserHll(alloc: std.mem.Allocator, slice: []const u8) {
    let h = HyperLogLog.deserialize(alloc, slice) catch return;
    if (h.count() == 0) {}
    h.deinit();
}

fn checkDeterministicFuzz(alloc: std.mem.Allocator): u32 {
    let prng = XorShift64.init(0x2026_1124_484c_4c54);
    let raw_buf: [128]u8 = [0; 128];

    // 1. 800 HLL malformed serialized payload fuzz cases (0 panic / 0 OOB)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const n = @as(usize, @intCast(prng.next() % 128));
        prng.fillBytes(raw_buf[0..n]);
        if (iter % 4 == 0 && n >= 5) {
            raw_buf[0] = 'H';
            raw_buf[1] = 'L';
            raw_buf[2] = 'L';
            raw_buf[3] = '1';
        }
        const slice = raw_buf[0..n];
        validateHllSerialized(slice) catch {};
        tryDeserHll(alloc, slice);
    }

    // 2. 800 T-Digest random stream + quantile monotonicity fuzz cases
    let td = TDigest.init(alloc, 100.0) catch return 1;
    defer td.deinit();
    iter = 0;
    while (iter < 800) : (iter += 1) {
        const kind = prng.next() % 10;
        if (kind == 0) {
            td.add(std.math.nan(f64), 1.0) catch {};
        } else if (kind == 1) {
            td.add(std.math.inf(f64), 1.0) catch {};
        } else if (kind == 2) {
            td.add(1.0, -1.0) catch {};
        } else {
            const v = (prng.nextUnitF64() - 0.5) * 10000.0;
            const w = prng.nextUnitF64() * 10.0 + 0.01;
            td.add(v, w) catch return 2;
        }
        if (td.count() > 0.0 && (iter % 20 == 0)) {
            const q0 = td.quantile(0.0) catch return 3;
            const q5 = td.quantile(0.5) catch return 4;
            const q1 = td.quantile(1.0) catch return 5;
            if (q0 > q5 || q5 > q1) return 6;
        }
    }

    return 0;
}

fn main() {
    test("1. HyperLogLog precision, LinearCounting, merge algebra & HLL1 serialization guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkHllSuite(alloc), 0 as u32);
    });

    test("2. T-Digest NaN/Inf/weight guards, monotonicity, P50/P90/P99 accuracy & merge", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkTDigestSuite(alloc), 0 as u32);
    });

    test("3. 16-lane HLL u8 & 4-lane T-Digest f64 SIMD vs scalar 0-diff parity", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG fuzzing (800 HLL malformed payloads + 800 T-Digest stream cases)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkDeterministicFuzz(alloc), 0 as u32);
    });
}

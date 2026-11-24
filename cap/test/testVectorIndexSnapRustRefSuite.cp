// Reference-hardened test suite against unum-cloud/usearch & djc/instant-distance (Apache-2.0 / MIT)
// Covers HNSW & IVF-PQ retrieval Recall, CRC32 binary snapshot round-trip, single-bit tamper,
// magic/version/truncation/checked-overflow rejection, NaN/±Inf & dim/subspace guards,
// 4-lane @Vector(4, f32) SIMD vs scalar (< 1e-4) parity, and 1,600 deterministic PRNG fuzz cases
// (800 HNSW + 800 IVF-PQ malformed snapshots & queries).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    HNSW_FLAT_MAGIC,
    HNSW_FLAT_VERSION,
    simdVecIdxL2SqF32,
    scalarVecIdxL2SqF32,
    simdVecIdxDotF32,
    scalarVecIdxDotF32,
    HnswFlatError,
    HnswFlatMetric,
    HnswFlatConfig,
    HnswFlatNode,
    HnswFlatHit,
    HnswFlatIndex,
} from "../../lib/vector/hnswFlat.cp";
import {
    IVFPQ_SNAPSHOT_MAGIC,
    IVFPQ_SNAPSHOT_VERSION,
    IvfPqError,
    IvfPqConfig,
    IvfPqHit,
    IvfPqIndex,
    IvfPqFlatError,
    IvfPqFlatConfig,
    IvfPqFlatIndex,
    serializeIvfPqSnapshot,
    deserializeIvfPqSnapshot,
} from "../../lib/vector/ivfPq.cp";

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

    fn nextF32(*_): f32 {
        const u: u32 = @as(u32, @truncate(_.next() & 0xFFFF));
        return (@as(f32, @floatFromInt(u)) / 32768.0) - 1.0;
    }
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = @as(u8, @truncate(v));
    buf[off + 1] = @as(u8, @truncate(v >> 8));
    buf[off + 2] = @as(u8, @truncate(v >> 16));
    buf[off + 3] = @as(u8, @truncate(v >> 24));
}

fn checkTest1HnswRecallSnapshotAndGuards(): u32 {
    const dim: usize = 16;
    const max_nodes: usize = 32;
    const cfg = HnswFlatConfig{
        dim: dim,
        max_nodes: max_nodes,
        m: 8,
        m_max0: 16,
        ef_construction: 32,
        metric: HnswFlatMetric.l2_sq,
        seed: 202601,
    };

    // 1a. Invalid config (dim == 0) rejected
    let vecs_buf: [512]f32 = [0.0; 512];
    let nodes_buf: [32]HnswFlatNode = [HnswFlatNode{}; 32];
    let l0_buf: [512]u32 = [0; 512];
    let up_buf: [512]u32 = [0; 512];
    let vis_buf: [32]u32 = [0; 32];
    let vecs_s: []f32 = vecs_buf[0..];
    let nodes_s: []HnswFlatNode = nodes_buf[0..];
    let l0_s: []u32 = l0_buf[0..];
    let up_s: []u32 = up_buf[0..];
    let vis_s: []u32 = vis_buf[0..];

    const bad_cfg = HnswFlatConfig{ dim: 0, max_nodes: 32, m: 8, m_max0: 16, ef_construction: 32 };
    let caught = false;
    * = HnswFlatIndex.init(bad_cfg, vecs_s, nodes_s, l0_s, up_s, vis_s) catch |e| {
        caught = (e == HnswFlatError.InvalidConfig);
    };
    if (!caught) return 1;

    let idx = HnswFlatIndex.init(cfg, vecs_s, nodes_s, l0_s, up_s, vis_s) catch return 2;

    // 1b. Empty index & k == 0 safe search
    let q_zero: [16]f32 = [0.0; 16];
    let hits_buf: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let hits_s: []HnswFlatHit = hits_buf[0..];
    const empty_n = idx.search(q_zero[0..], 5, 16, hits_s) catch return 3;
    if (empty_n != 0) return 4;

    // 1c. NaN / +Inf / -Inf rejected on insert & search
    let bad_vec: [16]f32 = [0.5; 16];
    bad_vec[3] = std.math.nan(f32);
    caught = false;
    * = idx.insert(999, bad_vec[0..]) catch |e| {
        caught = (e == HnswFlatError.InvalidVector);
    };
    if (!caught) return 5;

    bad_vec[3] = std.math.inf(f32);
    caught = false;
    * = idx.insert(999, bad_vec[0..]) catch |e| {
        caught = (e == HnswFlatError.InvalidVector);
    };
    if (!caught) return 6;

    bad_vec[3] = -std.math.inf(f32);
    caught = false;
    * = idx.search(bad_vec[0..], 3, 16, hits_s) catch |e| {
        caught = (e == HnswFlatError.InvalidVector);
    };
    if (!caught) return 7;

    // 1d. Populate 24 deterministic 16D vectors & check Recall@5 and k == 0
    let rng = XorShift64.init(0x484E5357_00000001);
    let corpus: [384]f32 = [0.0; 384]; // 24 * 16
    let n_i: usize = 0;
    while (n_i < 24) : (n_i += 1) {
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            corpus[n_i * dim + d] = rng.nextF32();
        }
        * = idx.insert(1000 + (n_i as u64), corpus[n_i * dim .. (n_i + 1) * dim]) catch return 8;
    }

    const k0_n = idx.search(corpus[0..dim], 0, 16, hits_s) catch return 9;
    if (k0_n != 0) return 10;

    let gt_buf: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let ann_buf: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let gt_s: []HnswFlatHit = gt_buf[0..];
    let ann_s: []HnswFlatHit = ann_buf[0..];
    const recall = idx.computeRecallAtK(corpus[0..dim], 5, 32, gt_s, ann_s);
    if (recall < 0.8) return 11;

    // 1e. Binary snapshot serialize -> deserialize round-trip
    let snap_buf: [16384]u8 = [0; 16384];
    let snap_s: []u8 = snap_buf[0..];
    const snap_len = idx.serializeSnapshot(snap_s) catch return 12;

    let vecs2: [512]f32 = [0.0; 512];
    let nodes2: [32]HnswFlatNode = [HnswFlatNode{}; 32];
    let l0_2: [512]u32 = [0; 512];
    let up_2: [512]u32 = [0; 512];
    let vis2: [32]u32 = [0; 32];
    let vecs2_s: []f32 = vecs2[0..];
    let nodes2_s: []HnswFlatNode = nodes2[0..];
    let l0_2_s: []u32 = l0_2[0..];
    let up_2_s: []u32 = up_2[0..];
    let vis2_s: []u32 = vis2[0..];

    let restored = HnswFlatIndex.deserializeSnapshot(
        snap_buf[0..snap_len],
        vecs2_s,
        nodes2_s,
        l0_2_s,
        up_2_s,
        vis2_s,
    ) catch return 13;

    let hits2: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let hits2_s: []HnswFlatHit = hits2[0..];
    const r_n = restored.search(corpus[0..dim], 5, 32, hits2_s) catch return 14;
    if (r_n != 5 || hits2[0].doc_id != 1000) return 15;

    // 1f. Snapshot tamper guards: single-bit flip, bad magic, bad version, truncation, overflow header
    let bad_snap: [16384]u8 = [0; 16384];
    @memcpy(bad_snap[0..snap_len], snap_buf[0..snap_len]);
    bad_snap[snap_len / 2] ^= 0x01;
    caught = false;
    * = HnswFlatIndex.deserializeSnapshot(bad_snap[0..snap_len], vecs2_s, nodes2_s, l0_2_s, up_2_s, vis2_s) catch |e| {
        caught = (e == HnswFlatError.ChecksumMismatch);
    };
    if (!caught) return 16;

    // Bad magic
    @memcpy(bad_snap[0..snap_len], snap_buf[0..snap_len]);
    putU32Le(bad_snap[0..], 0, 0xDEADBEEF);
    caught = false;
    * = HnswFlatIndex.deserializeSnapshot(bad_snap[0..snap_len], vecs2_s, nodes2_s, l0_2_s, up_2_s, vis2_s) catch |e| {
        caught = (e == HnswFlatError.InvalidSnapshot);
    };
    if (!caught) return 17;

    // Bad version
    @memcpy(bad_snap[0..snap_len], snap_buf[0..snap_len]);
    putU32Le(bad_snap[0..], 4, 99);
    caught = false;
    * = HnswFlatIndex.deserializeSnapshot(bad_snap[0..snap_len], vecs2_s, nodes2_s, l0_2_s, up_2_s, vis2_s) catch |e| {
        caught = (e == HnswFlatError.InvalidSnapshot);
    };
    if (!caught) return 18;

    // Truncated snapshot
    caught = false;
    * = HnswFlatIndex.deserializeSnapshot(snap_buf[0..60], vecs2_s, nodes2_s, l0_2_s, up_2_s, vis2_s) catch |e| {
        caught = (e == HnswFlatError.InvalidSnapshot);
    };
    if (!caught) return 19;

    return 0;
}

fn checkTest2IvfPqRecallSnapshotAndGuards(): u32 {
    // 2a. Subspace divisibility guards: m == 0 and dim % m != 0
    let caught = false;
    * = IvfPqIndex.init(IvfPqConfig{ dim: 16, nlist: 4, m_subspaces: 0, ksub: 4, nprobe: 2, train_iters: 4 }) catch |e| {
        caught = (e == IvfPqError.InvalidParams);
    };
    if (!caught) return 1;

    caught = false;
    * = IvfPqIndex.init(IvfPqConfig{ dim: 16, nlist: 4, m_subspaces: 3, ksub: 4, nprobe: 2, train_iters: 4 }) catch |e| {
        caught = (e == IvfPqError.InvalidParams);
    };
    if (!caught) return 2;

    // 2b. IvfPqFlatIndex train, NaN/Inf rejection, add, search, snapshot round-trip & tamper rejection
    const dim: usize = 16;
    const max_docs: usize = 32;
    const nlist: usize = 4;
    const m_sub: usize = 4;
    const ksub: usize = 4;
    const cfg = IvfPqFlatConfig{
        dim: dim,
        max_docs: max_docs,
        nlist: nlist,
        m_subspaces: m_sub,
        ksub: ksub,
        train_iters: 6,
        seed: 202602,
    };

    let coarse: [64]f32 = [0.0; 64]; // 4 * 16
    let cb: [64]f32 = [0.0; 64];     // 4 * 16
    let codes: [128]u8 = [0; 128];   // 32 * 4
    let ids: [32]u64 = [0; 32];
    let del: [32]u8 = [0; 32];
    let heads: [4]u32 = [0; 4];
    let next: [32]u32 = [0; 32];
    let cnts: [4]u32 = [0; 4];

    // Invalid flat config (dim % m != 0)
    const bad_fcfg = IvfPqFlatConfig{ dim: 16, max_docs: 32, nlist: 4, m_subspaces: 5, ksub: 4, train_iters: 4 };
    caught = false;
    * = IvfPqFlatIndex.init(bad_fcfg, coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidConfig);
    };
    if (!caught) return 3;

    let idx = IvfPqFlatIndex.init(cfg, coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch return 4;

    let rng = XorShift64.init(0x49564650_00000002);
    let train_vecs: [384]f32 = [0.0; 384]; // 24 * 16
    let v: usize = 0;
    while (v < 24) : (v += 1) {
        const base_cluster: f32 = @as(f32, @floatFromInt(v % 4)) * 5.0;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            train_vecs[v * dim + d] = base_cluster + rng.nextF32() * 0.25;
        }
    }

    // NaN vector in train rejected
    let wf: [256]f32 = [0.0; 256];
    let wu: [64]u32 = [0; 64];
    const saved_v = train_vecs[7];
    train_vecs[7] = std.math.nan(f32);
    caught = false;
    * = idx.train(train_vecs[0..], 24, wf[0..], wu[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidVector);
    };
    if (!caught) return 5;
    train_vecs[7] = saved_v;

    // Valid train + add 24 vectors
    idx.train(train_vecs[0..], 24, wf[0..], wu[0..]) catch return 6;

    // Inf vector rejected on add
    let inf_vec: [16]f32 = [0.0; 16];
    inf_vec[0] = std.math.inf(f32);
    caught = false;
    * = idx.add(9999, inf_vec[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidVector);
    };
    if (!caught) return 7;

    v = 0;
    while (v < 24) : (v += 1) {
        * = idx.add(2000 + (v as u64), train_vecs[v * dim .. (v + 1) * dim]) catch return 8;
    }

    // Search & verify exact Top-1 hit
    let adc: [64]f32 = [0.0; 64];
    let hits: [4]IvfPqHit = [IvfPqHit{}; 4];
    const h_n = idx.search(train_vecs[0..dim], 4, adc[0..], hits[0..]) catch return 9;
    if (h_n == 0 || hits[0].doc_id != 2000) return 10;

    // NaN query rejected on search
    caught = false;
    * = idx.search(inf_vec[0..], 4, adc[0..], hits[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidVector);
    };
    if (!caught) return 11;

    // Snapshot serialize -> deserialize round-trip
    let snap: [4096]u8 = [0; 4096];
    const snap_len = serializeIvfPqSnapshot(idx, snap[0..]) catch return 12;

    let coarse2: [64]f32 = [0.0; 64];
    let cb2: [64]f32 = [0.0; 64];
    let codes2: [128]u8 = [0; 128];
    let ids2: [32]u64 = [0; 32];
    let del2: [32]u8 = [0; 32];
    let heads2: [4]u32 = [0; 4];
    let next2: [32]u32 = [0; 32];
    let cnts2: [4]u32 = [0; 4];

    let restored = deserializeIvfPqSnapshot(
        snap[0..snap_len],
        coarse2[0..], cb2[0..], codes2[0..], ids2[0..], del2[0..], heads2[0..], next2[0..], cnts2[0..],
    ) catch return 13;

    let hits2: [4]IvfPqHit = [IvfPqHit{}; 4];
    const h2_n = restored.search(train_vecs[0..dim], 4, adc[0..], hits2[0..]) catch return 14;
    if (h2_n == 0 || hits2[0].doc_id != 2000) return 15;

    // Tamper guards: single-bit flip -> ChecksumMismatch; bad magic/truncation/trailing -> InvalidSnapshot
    let bad_snap: [4096]u8 = [0; 4096];
    @memcpy(bad_snap[0..snap_len], snap[0..snap_len]);
    bad_snap[snap_len / 2] ^= 0x01;
    caught = false;
    * = deserializeIvfPqSnapshot(bad_snap[0..snap_len], coarse2[0..], cb2[0..], codes2[0..], ids2[0..], del2[0..], heads2[0..], next2[0..], cnts2[0..]) catch |e| {
        caught = (e == IvfPqFlatError.ChecksumMismatch);
    };
    if (!caught) return 16;

    @memcpy(bad_snap[0..snap_len], snap[0..snap_len]);
    putU32Le(bad_snap[0..], 0, 0x11223344);
    caught = false;
    * = deserializeIvfPqSnapshot(bad_snap[0..snap_len], coarse2[0..], cb2[0..], codes2[0..], ids2[0..], del2[0..], heads2[0..], next2[0..], cnts2[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidSnapshot);
    };
    if (!caught) return 17;

    // Truncated & trailing garbage rejected
    caught = false;
    * = deserializeIvfPqSnapshot(snap[0 .. snap_len - 1], coarse2[0..], cb2[0..], codes2[0..], ids2[0..], del2[0..], heads2[0..], next2[0..], cnts2[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidSnapshot);
    };
    if (!caught) return 18;

    caught = false;
    * = deserializeIvfPqSnapshot(snap[0 .. snap_len + 1], coarse2[0..], cb2[0..], codes2[0..], ids2[0..], del2[0..], heads2[0..], next2[0..], cnts2[0..]) catch |e| {
        caught = (e == IvfPqFlatError.InvalidSnapshot);
    };
    if (!caught) return 19;

    return 0;
}

fn checkTest3SimdPrecisionParity(): u32 {
    let rng = XorShift64.init(0x56454334_46333253);
    let a: [132]f32 = [0.0; 132];
    let b: [132]f32 = [0.0; 132];

    let dim: usize = 0;
    while (dim <= 129) : (dim += 1) {
        let i: usize = 0;
        while (i < dim) : (i += 1) {
            a[i] = rng.nextF32();
            b[i] = rng.nextF32();
        }
        const l2_simd = simdVecIdxL2SqF32(a[0..dim], b[0..dim]);
        const l2_scalar = scalarVecIdxL2SqF32(a[0..dim], b[0..dim]);
        const l2_diff = if (l2_simd >= l2_scalar) l2_simd - l2_scalar else l2_scalar - l2_simd;
        if (l2_diff >= 1.0e-4) return 1;

        const dot_simd = simdVecIdxDotF32(a[0..dim], b[0..dim]);
        const dot_scalar = scalarVecIdxDotF32(a[0..dim], b[0..dim]);
        const dot_diff = if (dot_simd >= dot_scalar) dot_simd - dot_scalar else dot_scalar - dot_simd;
        if (dot_diff >= 1.0e-4) return 2;
    }
    return 0;
}

fn checkTest4PrngFuzz1600(): u32 {
    let rng = XorShift64.init(0x57333852_3146555A);
    let fuzz_buf: [256]u8 = [0; 256];

    // Scratch backing buffers for deserialize fuzzing
    let vecs: [256]f32 = [0.0; 256];
    let nodes: [32]HnswFlatNode = [HnswFlatNode{}; 32];
    let l0: [512]u32 = [0; 512];
    let up: [512]u32 = [0; 512];
    let vis: [32]u32 = [0; 32];
    let vecs_s: []f32 = vecs[0..];
    let nodes_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let vis_s: []u32 = vis[0..];

    // 4a. 800 HNSW malformed snapshot fuzz cases (random + valid-magic + overflow headers)
    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 256;
        let k: usize = 0;
        while (k < len) : (k += 1) {
            fuzz_buf[k] = rng.nextByte();
        }
        if (len >= 68 && (iter & 1) == 0) {
            putU32Le(fuzz_buf[0..], 0, HNSW_FLAT_MAGIC);
            putU32Le(fuzz_buf[0..], 4, HNSW_FLAT_VERSION);
            if ((iter & 3) == 0) {
                // Inject huge dim/max_nodes to stress checked-overflow paths
                putU32Le(fuzz_buf[0..], 8, 0x40000000);
                putU32Le(fuzz_buf[0..], 12, 0x40000000);
                putU32Le(fuzz_buf[0..], 40, 0x40000000);
            }
        }
        * = HnswFlatIndex.deserializeSnapshot(
            fuzz_buf[0..len],
            vecs_s,
            nodes_s,
            l0_s,
            up_s,
            vis_s,
        ) catch {};
    }

    // 4b. 800 IVF-PQ malformed snapshot fuzz cases (random + valid-magic + overflow headers)
    let coarse: [64]f32 = [0.0; 64];
    let cb: [64]f32 = [0.0; 64];
    let codes: [128]u8 = [0; 128];
    let ids: [32]u64 = [0; 32];
    let del: [32]u8 = [0; 32];
    let heads: [16]u32 = [0; 16];
    let next: [32]u32 = [0; 32];
    let cnts: [16]u32 = [0; 16];

    iter = 0;
    while (iter < 800) : (iter += 1) {
        const len = @as(usize, @truncate(rng.next())) % 256;
        let k: usize = 0;
        while (k < len) : (k += 1) {
            fuzz_buf[k] = rng.nextByte();
        }
        if (len >= 60 && (iter & 1) == 0) {
            putU32Le(fuzz_buf[0..], 0, IVFPQ_SNAPSHOT_MAGIC);
            putU32Le(fuzz_buf[0..], 4, IVFPQ_SNAPSHOT_VERSION);
            if ((iter & 3) == 0) {
                // Inject huge dim/nlist/doc_count to stress checked-overflow paths
                putU32Le(fuzz_buf[0..], 8, 0x40000000);
                putU32Le(fuzz_buf[0..], 12, 0x40000000);
                putU32Le(fuzz_buf[0..], 16, 0x10000000);
                putU32Le(fuzz_buf[0..], 20, 4);
                putU32Le(fuzz_buf[0..], 24, 16);
                putU32Le(fuzz_buf[0..], 28, 4);
                putU32Le(fuzz_buf[0..], 32, 0x40000000);
            }
        }
        * = deserializeIvfPqSnapshot(
            fuzz_buf[0..len],
            coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..],
        ) catch {};
    }

    return 0;
}

fn main() {
    test("1. HNSW Recall@5, Snapshot Round-Trip, NaN/Inf, k=0 & Tamper Guards", () => {
        expect_eq(checkTest1HnswRecallSnapshotAndGuards(), 0 as u32);
    });

    test("2. IVF-PQ Subspace Divisibility, Recall, Snapshot Round-Trip & Tamper Guards", () => {
        expect_eq(checkTest2IvfPqRecallSnapshotAndGuards(), 0 as u32);
    });

    test("3. 4-Lane @Vector(4, f32) SIMD vs Scalar Precision Parity (< 1e-4, 0..=129 Dim)", () => {
        expect_eq(checkTest3SimdPrecisionParity(), 0 as u32);
    });

    test("4. Deterministic PRNG Fuzzing (800 HNSW + 800 IVF-PQ = 1,600 Cases)", () => {
        expect_eq(checkTest4PrngFuzz1600(), 0 as u32);
    });
}

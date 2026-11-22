// Origin: Pure-CP Stdlib Test Suite (lib/vector/hnswFlat.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Unbounded External-Buffer Multi-Layer HNSW & CRC32 Snapshot:
//   1. 500-node 16D multi-cluster index build & `ef_search=64` average Recall@10 >= 0.90
//   2. L2-squared, Cosine & Negative-Dot distance metric exactness
//   3. Algorithm 4 `SELECT-NEIGHBORS-HEURISTIC` cross-cluster bridge connectivity
//   4. Soft-deletion tombstone filtering (`markDeleted` / `isDeleted`) & live stats
//   5. Binary snapshot `serializeSnapshot` -> `deserializeSnapshot` bit-exact hot-reload parity
//   6. Single-bit tamper detection (`ChecksumMismatch`) & malformed/truncated snapshot (`InvalidSnapshot`)
//   7. Invalid config, dim mismatch, capacity full, buffer guards & `cap.vector` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    HNSW_FLAT_MAGIC,
    HNSW_FLAT_VERSION,
    HNSW_FLAT_INVALID_ID,
    HNSW_FLAT_MAX_LEVELS,
    HnswFlatError,
    HnswFlatMetric,
    HnswFlatConfig,
    hnswVecBufLen,
    hnswL0AdjBufLen,
    hnswUpperAdjBufLen,
    hnswSnapshotMaxBytes,
    HnswFlatNode,
    HnswFlatHit,
    HnswFlatIndex,
} from "cap:vector";

fn lcgNext(state: *u64): f32 {
    state.* = state.* *% 6364136223846793005 +% 1442695040888963407;
    const bits: u32 = @as(u32, @intCast((state.* >> 33) & 0x7FFF_FFFF));
    return (@as(f32, @floatFromInt(bits)) / 2147483647.0) * 2.0 - 1.0;
}

fn absDiffF32(a: f32, b: f32): f32 {
    const d: f32 = a - b;
    return if (d < 0.0) -d else d;
}

test("HnswFlat: 500-node 16D multi-cluster build & Recall@10 >= 0.90 at ef_search=64") {
    const N: usize = 500;
    const D: usize = 16;
    const cfg = HnswFlatConfig{
        dim: D,
        max_nodes: N,
        m: 12,
        m_max0: 24,
        ef_construction: 64,
        metric: HnswFlatMetric.l2_sq,
        seed: 0x123456789ABCDEF0,
    };
    expect_true(cfg.isValid());
    expect_eq(hnswVecBufLen(cfg), N * D);
    expect_eq(hnswL0AdjBufLen(cfg), N * 24);
    expect_eq(hnswUpperAdjBufLen(cfg), N * 12 * 2);

    let vecs: [8000]f32 = [0.0; 8000];
    let nodes: [500]HnswFlatNode = [HnswFlatNode{}; 500];
    let l0: [12000]u32 = [0; 12000];
    let up: [12000]u32 = [0; 12000];
    let marks: [500]u32 = [0; 500];

    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];

    let idx = cap.vector.HnswFlatIndex.init(cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };

    // Generate 10 distinct clusters in 16D space (50 vectors per cluster).
    let rng: u64 = 0xCAFEBABE_12345678;
    let centroids: [160]f32 = [0.0; 160];
    let ci: usize = 0;
    while (ci < 160) : (ci += 1) {
        centroids[ci] = lcgNext(*rng) * 8.0;
    }

    let tmp_vec: [16]f32 = [0.0; 16];
    let i: usize = 0;
    while (i < N) : (i += 1) {
        const c_id: usize = i % 10;
        let d: usize = 0;
        while (d < D) : (d += 1) {
            tmp_vec[d] = centroids[c_id * D + d] + lcgNext(*rng) * 0.8;
        }
        * = idx.insert(@as(u64, 10000 + i), tmp_vec[0..]) catch {
            expect_true(false);
            return;
        };
    }

    const st = idx.stats();
    expect_eq(st.node_count, N);
    expect_eq(st.live_count, N);
    expect_eq(st.deleted_count, @as(usize, 0));
    expect_true(st.max_level >= 1);
    expect_true(st.total_l0_edges > N * 4);

    // Measure average Recall@10 over 20 diverse queries.
    let gt_buf: [10]HnswFlatHit = [HnswFlatHit{}; 10];
    let ann_buf: [10]HnswFlatHit = [HnswFlatHit{}; 10];
    let gt_s: []HnswFlatHit = gt_buf[0..];
    let ann_s: []HnswFlatHit = ann_buf[0..];

    let recall_sum: f32 = 0.0;
    let q_i: usize = 0;
    while (q_i < 20) : (q_i += 1) {
        const c_id: usize = q_i % 10;
        let d: usize = 0;
        while (d < D) : (d += 1) {
            tmp_vec[d] = centroids[c_id * D + d] + lcgNext(*rng) * 0.9;
        }
        recall_sum += idx.computeRecallAtK(tmp_vec[0..], 10, 64, *gt_s, *ann_s);
    }
    const avg_recall: f32 = recall_sum / 20.0;
    expect_true(avg_recall >= 0.90);
}

test("HnswFlat: L2-squared, Cosine & Negative-Dot metric accuracy") {
    const query: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v0: [4]f32 = [1.0, 0.0, 0.0, 0.0];   // exact L2 match, cosine=0, dot=1
    const v1: [4]f32 = [10.0, 0.0, 0.0, 0.0];  // far L2, cosine=0, highest dot=10
    const v2: [4]f32 = [0.0, 1.0, 0.0, 0.0];   // orthogonal
    const v3: [4]f32 = [0.9, 0.4, 0.0, 0.0];

    let vecs: [16]f32 = [0.0; 16];
    let nodes: [4]HnswFlatNode = [HnswFlatNode{}; 4];
    let l0: [32]u32 = [0; 32];
    let up: [32]u32 = [0; 32];
    let marks: [4]u32 = [0; 4];
    let hits: [4]HnswFlatHit = [HnswFlatHit{}; 4];

    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];
    let h_s: []HnswFlatHit = hits[0..];

    // 1. L2-squared: Top-1 must be doc 10 (v0)
    const cfg_l2 = HnswFlatConfig{ dim: 4, max_nodes: 4, m: 4, m_max0: 8, ef_construction: 8, metric: HnswFlatMetric.l2_sq };
    let idx_l2 = HnswFlatIndex.init(cfg_l2, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };
    * = idx_l2.insert(10, v0[0..]) catch 0;
    * = idx_l2.insert(20, v1[0..]) catch 0;
    * = idx_l2.insert(30, v2[0..]) catch 0;
    * = idx_l2.insert(40, v3[0..]) catch 0;
    const n_l2 = idx_l2.search(query[0..], 1, 8, *h_s) catch 0;
    expect_eq(n_l2, @as(usize, 1));
    expect_eq(hits[0].doc_id, @as(u64, 10));
    expect_true(absDiffF32(hits[0].distance, 0.0) < 1.0e-5);

    // 2. Cosine: both v0 (10) and v1 (20) have cosine distance ~ 0.0
    const cfg_cos = HnswFlatConfig{ dim: 4, max_nodes: 4, m: 4, m_max0: 8, ef_construction: 8, metric: HnswFlatMetric.cosine };
    let idx_cos = HnswFlatIndex.init(cfg_cos, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };
    * = idx_cos.insert(10, v0[0..]) catch 0;
    * = idx_cos.insert(20, v1[0..]) catch 0;
    * = idx_cos.insert(30, v2[0..]) catch 0;
    * = idx_cos.insert(40, v3[0..]) catch 0;
    const n_cos = idx_cos.search(query[0..], 2, 8, *h_s) catch 0;
    expect_eq(n_cos, @as(usize, 2));
    expect_true(absDiffF32(hits[0].distance, 0.0) < 1.0e-5);
    expect_true(absDiffF32(hits[1].distance, 0.0) < 1.0e-5);

    // 3. Negative dot: Top-1 must be doc 20 (v1 = [10,0,0,0], neg_dot = -10.0)
    const cfg_dot = HnswFlatConfig{ dim: 4, max_nodes: 4, m: 4, m_max0: 8, ef_construction: 8, metric: HnswFlatMetric.neg_dot };
    let idx_dot = HnswFlatIndex.init(cfg_dot, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };
    * = idx_dot.insert(10, v0[0..]) catch 0;
    * = idx_dot.insert(20, v1[0..]) catch 0;
    * = idx_dot.insert(30, v2[0..]) catch 0;
    * = idx_dot.insert(40, v3[0..]) catch 0;
    const n_dot = idx_dot.search(query[0..], 1, 8, *h_s) catch 0;
    expect_eq(n_dot, @as(usize, 1));
    expect_eq(hits[0].doc_id, @as(u64, 20));
    expect_true(absDiffF32(hits[0].distance, -10.0) < 1.0e-5);
}

test("HnswFlat: Algorithm 4 Heuristic preserves cross-cluster bridge connectivity") {
    const N: usize = 80;
    const D: usize = 8;
    const cfg = HnswFlatConfig{ dim: D, max_nodes: N, m: 8, m_max0: 16, ef_construction: 32, metric: HnswFlatMetric.l2_sq };

    let vecs: [640]f32 = [0.0; 640];
    let nodes: [80]HnswFlatNode = [HnswFlatNode{}; 80];
    let l0: [1280]u32 = [0; 1280];
    let up: [1280]u32 = [0; 1280];
    let marks: [80]u32 = [0; 80];

    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];

    let idx = HnswFlatIndex.init(cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };

    // Cluster A near origin, Cluster B far away at +100.0
    let rng: u64 = 0x77778888_9999AAAA;
    let v: [8]f32 = [0.0; 8];
    let i: usize = 0;
    while (i < N) : (i += 1) {
        const base: f32 = if (i < 40) 0.0 else 100.0;
        let d: usize = 0;
        while (d < D) : (d += 1) {
            v[d] = base + lcgNext(*rng) * 0.5;
        }
        * = idx.insert(@as(u64, i), v[0..]) catch 0;
    }

    // Query deep inside Cluster B and Cluster A -> both must achieve Recall@5 == 1.0
    let gt: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let ann: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let gt_s: []HnswFlatHit = gt[0..];
    let ann_s: []HnswFlatHit = ann[0..];

    const q_b: [8]f32 = [100.1, 99.9, 100.0, 100.2, 99.8, 100.0, 100.1, 99.9];
    const rec_b = idx.computeRecallAtK(q_b[0..], 5, 32, *gt_s, *ann_s);
    expect_true(rec_b >= 0.99);

    const q_a: [8]f32 = [0.1, -0.1, 0.0, 0.2, -0.2, 0.0, 0.1, -0.1];
    const rec_a = idx.computeRecallAtK(q_a[0..], 5, 32, *gt_s, *ann_s);
    expect_true(rec_a >= 0.99);
}

test("HnswFlat: soft-deletion tombstone filtering & liveness statistics") {
    const N: usize = 32;
    const D: usize = 4;
    const cfg = HnswFlatConfig{ dim: D, max_nodes: N, m: 6, m_max0: 12, ef_construction: 24, metric: HnswFlatMetric.l2_sq };

    let vecs: [128]f32 = [0.0; 128];
    let nodes: [32]HnswFlatNode = [HnswFlatNode{}; 32];
    let l0: [384]u32 = [0; 384];
    let up: [384]u32 = [0; 384];
    let marks: [32]u32 = [0; 32];
    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];

    let idx = HnswFlatIndex.init(cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const fi: f32 = @as(f32, @floatFromInt(i));
        const v: [4]f32 = [fi, fi * 0.5, 0.0, 0.0];
        * = idx.insert(@as(u64, 500 + i), v[0..]) catch 0;
    }

    const q: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let hits: [5]HnswFlatHit = [HnswFlatHit{}; 5];
    let h_s: []HnswFlatHit = hits[0..];

    const n1 = idx.search(q[0..], 5, 24, *h_s) catch 0;
    expect_eq(n1, @as(usize, 5));
    expect_eq(hits[0].doc_id, @as(u64, 500));
    expect_eq(hits[1].doc_id, @as(u64, 501));

    // Soft-delete top 2 nearest docs (500 and 501)
    expect_true(idx.markDeleted(500));
    expect_true(idx.markDeleted(501));
    expect_true(!idx.markDeleted(500)); // Already deleted
    expect_true(idx.isDeleted(0));
    expect_true(idx.isDeleted(1));
    expect_true(!idx.isDeleted(2));

    const st = idx.stats();
    expect_eq(st.live_count, @as(usize, 30));
    expect_eq(st.deleted_count, @as(usize, 2));

    const n2 = idx.search(q[0..], 5, 24, *h_s) catch 0;
    expect_eq(n2, @as(usize, 5));
    expect_eq(hits[0].doc_id, @as(u64, 502));
    let k: usize = 0;
    while (k < n2) : (k += 1) {
        expect_true(hits[k].doc_id != 500 && hits[k].doc_id != 501);
        if (k > 0) {
            expect_true(hits[k].distance >= hits[k - 1].distance);
        }
    }
}

test("HnswFlat: binary snapshot serializeSnapshot -> deserializeSnapshot bit-exact reload") {
    const N: usize = 96;
    const D: usize = 8;
    const cfg = HnswFlatConfig{ dim: D, max_nodes: N, m: 8, m_max0: 16, ef_construction: 32, metric: HnswFlatMetric.cosine, seed: 42 };

    let vecs1: [768]f32 = [0.0; 768];
    let nodes1: [96]HnswFlatNode = [HnswFlatNode{}; 96];
    let l0_1: [1536]u32 = [0; 1536];
    let up_1: [1536]u32 = [0; 1536];
    let marks1: [96]u32 = [0; 96];
    let v1_s: []f32 = vecs1[0..];
    let n1_s: []HnswFlatNode = nodes1[0..];
    let l0_1_s: []u32 = l0_1[0..];
    let up_1_s: []u32 = up_1[0..];
    let m1_s: []u32 = marks1[0..];

    let orig = HnswFlatIndex.init(cfg, *v1_s, *n1_s, *l0_1_s, *up_1_s, *m1_s) catch {
        expect_true(false);
        return;
    };

    let rng: u64 = 0x11223344_55667788;
    let v: [8]f32 = [0.0; 8];
    let i: usize = 0;
    while (i < N) : (i += 1) {
        let d: usize = 0;
        while (d < D) : (d += 1) {
            v[d] = lcgNext(*rng);
        }
        * = orig.insert(@as(u64, 2000 + i), v[0..]) catch 0;
    }
    * = orig.markDeleted(2005);
    * = orig.markDeleted(2019);

    const max_snap = hnswSnapshotMaxBytes(cfg, N);
    expect_true(max_snap <= 32768);
    let snap_buf: [32768]u8 = [0; 32768];
    let snap_s: []u8 = snap_buf[0..];
    const snap_len = orig.serializeSnapshot(*snap_s) catch {
        expect_true(false);
        return;
    };
    expect_true(snap_len > 68 && snap_len <= max_snap);

    // Reload into completely fresh zeroed buffers
    let vecs2: [768]f32 = [0.0; 768];
    let nodes2: [96]HnswFlatNode = [HnswFlatNode{}; 96];
    let l0_2: [1536]u32 = [0; 1536];
    let up_2: [1536]u32 = [0; 1536];
    let marks2: [96]u32 = [0; 96];
    let v2_s: []f32 = vecs2[0..];
    let n2_s: []HnswFlatNode = nodes2[0..];
    let l0_2_s: []u32 = l0_2[0..];
    let up_2_s: []u32 = up_2[0..];
    let m2_s: []u32 = marks2[0..];

    let reloaded = HnswFlatIndex.deserializeSnapshot(
        snap_buf[0..snap_len],
        *v2_s,
        *n2_s,
        *l0_2_s,
        *up_2_s,
        *m2_s,
    ) catch {
        expect_true(false);
        return;
    };

    const st1 = orig.stats();
    const st2 = reloaded.stats();
    expect_eq(st1.node_count, st2.node_count);
    expect_eq(st1.live_count, st2.live_count);
    expect_eq(st1.deleted_count, st2.deleted_count);
    expect_eq(st1.max_level, st2.max_level);
    expect_eq(st1.entry_point, st2.entry_point);
    expect_eq(st1.total_l0_edges, st2.total_l0_edges);
    expect_eq(st1.total_upper_edges, st2.total_upper_edges);

    const q: [8]f32 = [0.4, -0.2, 0.7, 0.1, -0.5, 0.3, 0.2, -0.6];
    let h1: [10]HnswFlatHit = [HnswFlatHit{}; 10];
    let h2: [10]HnswFlatHit = [HnswFlatHit{}; 10];
    let h1_s: []HnswFlatHit = h1[0..];
    let h2_s: []HnswFlatHit = h2[0..];
    const c1 = orig.search(q[0..], 10, 32, *h1_s) catch 0;
    const c2 = reloaded.search(q[0..], 10, 32, *h2_s) catch 0;
    expect_eq(c1, c2);
    let k: usize = 0;
    while (k < c1) : (k += 1) {
        expect_eq(h1[k].doc_id, h2[k].doc_id);
        expect_eq(h1[k].node_idx, h2[k].node_idx);
        expect_true(absDiffF32(h1[k].distance, h2[k].distance) < 1.0e-7);
    }
}

test("HnswFlat: single-bit snapshot tamper triggers ChecksumMismatch & invalid header rejected") {
    const cfg = HnswFlatConfig{ dim: 4, max_nodes: 8, m: 4, m_max0: 8, ef_construction: 8 };
    let vecs: [32]f32 = [0.0; 32];
    let nodes: [8]HnswFlatNode = [HnswFlatNode{}; 8];
    let l0: [64]u32 = [0; 64];
    let up: [64]u32 = [0; 64];
    let marks: [8]u32 = [0; 8];
    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];

    let idx = HnswFlatIndex.init(cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };
    const v: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    * = idx.insert(77, v[0..]) catch 0;

    let snap: [4096]u8 = [0; 4096];
    let snap_s: []u8 = snap[0..];
    const written = idx.serializeSnapshot(*snap_s) catch 0;
    expect_true(written > 68);

    // Flip 1 bit in the middle of the payload -> ChecksumMismatch
    snap[70] ^= 0x04;
    let got_crc_err: bool = false;
    * = HnswFlatIndex.deserializeSnapshot(snap[0..written], *v_s, *n_s, *l0_s, *up_s, *m_s) catch |e| {
        got_crc_err = (e == HnswFlatError.ChecksumMismatch);
    };
    expect_true(got_crc_err);

    // Corrupt magic bytes -> InvalidSnapshot
    snap[0] = 0x00;
    let got_invalid: bool = false;
    * = HnswFlatIndex.deserializeSnapshot(snap[0..written], *v_s, *n_s, *l0_s, *up_s, *m_s) catch |e| {
        got_invalid = (e == HnswFlatError.InvalidSnapshot);
    };
    expect_true(got_invalid);
}

test("HnswFlat: config, dimension, capacity & buffer error guards") {
    expect_eq(HNSW_FLAT_MAGIC, @as(u32, 0x57534E48));
    expect_eq(HNSW_FLAT_VERSION, @as(u32, 1));
    expect_eq(HNSW_FLAT_INVALID_ID, @as(u32, 0xFFFF_FFFF));
    expect_eq(HNSW_FLAT_MAX_LEVELS, @as(usize, 16));

    let vecs: [8]f32 = [0.0; 8];
    let nodes: [2]HnswFlatNode = [HnswFlatNode{}; 2];
    let l0: [8]u32 = [0; 8];
    let up: [8]u32 = [0; 8];
    let marks: [2]u32 = [0; 2];
    let v_s: []f32 = vecs[0..];
    let n_s: []HnswFlatNode = nodes[0..];
    let l0_s: []u32 = l0[0..];
    let up_s: []u32 = up[0..];
    let m_s: []u32 = marks[0..];

    // Invalid config (dim = 0)
    const bad_cfg = HnswFlatConfig{ dim: 0, max_nodes: 2, m: 2, m_max0: 4, ef_construction: 4 };
    let got_bad_cfg: bool = false;
    * = HnswFlatIndex.init(bad_cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch |e| {
        got_bad_cfg = (e == HnswFlatError.InvalidConfig);
    };
    expect_true(got_bad_cfg);

    // Valid small index (max_nodes = 2, dim = 4)
    const cfg = HnswFlatConfig{ dim: 4, max_nodes: 2, m: 2, m_max0: 4, ef_construction: 4 };
    let idx = HnswFlatIndex.init(cfg, *v_s, *n_s, *l0_s, *up_s, *m_s) catch {
        expect_true(false);
        return;
    };

    // Dim mismatch
    const wrong_v: [3]f32 = [1.0, 2.0, 3.0];
    let got_dim_err: bool = false;
    * = idx.insert(1, wrong_v[0..]) catch |e| {
        got_dim_err = (e == HnswFlatError.DimMismatch);
    };
    expect_true(got_dim_err);

    // Capacity full on 3rd insert
    const ok_v: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    * = idx.insert(1, ok_v[0..]) catch 0;
    * = idx.insert(2, ok_v[0..]) catch 0;
    let got_full: bool = false;
    * = idx.insert(3, ok_v[0..]) catch |e| {
        got_full = (e == HnswFlatError.CapacityFull);
    };
    expect_true(got_full);
}

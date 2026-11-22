// Origin: Pure-CP Stdlib Test Suite (lib/vector/ivfPq.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc SIMD @l2sq Residual IVF-PQ Engine:
//   1. Config validation (`isValid`, `InvalidDim`, `InvalidParams`) & `NotTrained` guard
//   2. Two-stage residual training (coarse K-Means + per-subspace residual PQ) convergence
//   3. `add` & `reconstruct` accuracy verifying low residual quantization error
//   4. Multi-probe recall improvement (`nprobe=1` vs `nprobe=4`) on cluster-boundary queries
//   5. Multi-cluster Top-K ADC search ranking accuracy & monotonic distance ordering
//   6. High compression ratio `stats()`, `DimMismatch` guard & `cap.vector` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    IVFPQ_MAX_DIM,
    IvfPqConfig,
    IvfPqHit,
    IvfPqIndex,
    IVFPQ_SNAPSHOT_MAGIC,
    IVFPQ_SNAPSHOT_VERSION,
    IVFPQ_INVALID_LINK,
    IvfPqFlatError,
    IvfPqFlatConfig,
    ivfPqCoarseBufLen,
    ivfPqCodebookBufLen,
    ivfPqCodesBufLen,
    ivfPqTrainWorkF32Len,
    ivfPqTrainWorkU32Len,
    ivfPqSearchWorkF32Len,
    ivfPqSnapshotMaxBytes,
    IvfPqFlatIndex,
    serializeIvfPqSnapshot,
    deserializeIvfPqSnapshot,
} from "cap:vector";

fn buildClusteredDataset(out_flat: *[]f32, dim: usize, n_clusters: usize, per_cluster: usize) {
    let idx: usize = 0;
    let c: usize = 0;
    while (c < n_clusters) : (c += 1) {
        let p: usize = 0;
        while (p < per_cluster) : (p += 1) {
            const base: f32 = @as(f32, @floatFromInt(c)) * 20.0;
            const offset: f32 = @as(f32, @floatFromInt(p)) * 0.25 - 1.0;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                const wave: f32 = if ((d % 2) == 0) offset else (0.0 - offset * 0.5);
                out_flat[idx * dim + d] = base + wave + @as(f32, @floatFromInt(d)) * 0.05;
            }
            idx += 1;
        }
    }
}

test("IvfPqIndex: config validation & NotTrained guard") {
    // Invalid dim = 0 or > IVFPQ_MAX_DIM
    const bad_dim = IvfPqConfig{ dim: 0, nlist: 2, m_subspaces: 2, ksub: 2, nprobe: 1, train_iters: 4 };
    expect_true(!bad_dim.isValid());
    const bad_dim2 = IvfPqConfig{ dim: IVFPQ_MAX_DIM + 4, nlist: 2, m_subspaces: 2, ksub: 2, nprobe: 1, train_iters: 4 };
    expect_true(!bad_dim2.isValid());

    // dim not divisible by m_subspaces
    const bad_div = IvfPqConfig{ dim: 10, nlist: 2, m_subspaces: 4, ksub: 2, nprobe: 1, train_iters: 4 };
    expect_true(!bad_div.isValid());

    // nprobe > nlist
    const bad_probe = IvfPqConfig{ dim: 8, nlist: 2, m_subspaces: 2, ksub: 2, nprobe: 3, train_iters: 4 };
    expect_true(!bad_probe.isValid());

    // Valid init succeeds, but add before train fails with NotTrained
    const ok_cfg = IvfPqConfig{ dim: 8, nlist: 2, m_subspaces: 2, ksub: 2, nprobe: 1, train_iters: 4 };
    expect_true(ok_cfg.isValid());
    let idx = IvfPqIndex.init(ok_cfg) catch {
        expect_true(false);
        return;
    };
    expect_true(!idx.stats().trained);

    const v: [8]f32 = [1.0; 8];
    let got_not_trained = false;
    * = idx.add(1, v[0..]) catch {
        got_not_trained = true;
        0
    };
    expect_true(got_not_trained);
}

test("IvfPqIndex: two-stage residual training & codebook convergence") {
    const dim: usize = 8;
    const n_clusters: usize = 4;
    const per_cluster: usize = 8;
    const total_vecs: usize = n_clusters * per_cluster; // 32

    let data: [256]f32 = [0.0; 256];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, dim, n_clusters, per_cluster);

    const cfg = IvfPqConfig{ dim: dim, nlist: 4, m_subspaces: 4, ksub: 8, nprobe: 2, train_iters: 8 };
    let idx = IvfPqIndex.init(cfg) catch {
        expect_true(false);
        return;
    };
    idx.train(data[0..], total_vecs) catch {
        expect_true(false);
        return;
    };
    expect_true(idx.stats().trained);

    // Add all 32 vectors and verify they spread across all 4 coarse lists
    let list_seen: [4]bool = [false; 4];
    let i: usize = 0;
    while (i < total_vecs) : (i += 1) {
        const lid = idx.add(@as(u64, 100 + i), data[i * dim .. i * dim + dim]) catch {
            expect_true(false);
            return;
        };
        expect_true(@as(usize, lid) < 4);
        list_seen[@as(usize, lid)] = true;
    }
    expect_eq(idx.stats().doc_count, total_vecs);
    expect_true(list_seen[0] && list_seen[1] && list_seen[2] && list_seen[3]);
}

test("IvfPqIndex: add & reconstruct low residual quantization error") {
    const dim: usize = 8;
    let data: [256]f32 = [0.0; 256];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, dim, 4, 8);

    const cfg = IvfPqConfig{ dim: dim, nlist: 4, m_subspaces: 4, ksub: 8, nprobe: 2, train_iters: 10 };
    let idx = IvfPqIndex.init(cfg) catch {
        expect_true(false);
        return;
    };
    idx.train(data[0..], 32) catch {
        expect_true(false);
        return;
    };

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        * = idx.add(@as(u64, i + 1), data[i * dim .. i * dim + dim]) catch {
            expect_true(false);
            return;
        };
    }

    // Reconstruct every stored vector and verify residual quantization error is very small
    let recon: [8]f32 = [0.0; 8];
    let recon_s: []f32 = recon[0..];
    let max_err_sq: f32 = 0.0;
    let j: usize = 0;
    while (j < 32) : (j += 1) {
        expect_true(idx.reconstruct(j, *recon_s));
        const orig: []const f32 = data[j * dim .. j * dim + dim];
        const err_sq: f32 = @l2sq(orig, recon[0..]);
        if (err_sq > max_err_sq) max_err_sq = err_sq;
    }
    expect_true(max_err_sq < 0.05);

    // Out-of-bounds reconstruct returns false cleanly
    expect_true(!idx.reconstruct(999, *recon_s));
}

test("IvfPqIndex: multi-probe recall improvement (nprobe=1 vs nprobe=4)") {
    const dim: usize = 8;
    let data: [256]f32 = [0.0; 256];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, dim, 4, 8);

    const cfg = IvfPqConfig{ dim: dim, nlist: 4, m_subspaces: 4, ksub: 8, nprobe: 1, train_iters: 8 };
    let idx = IvfPqIndex.init(cfg) catch {
        expect_true(false);
        return;
    };
    idx.train(data[0..], 32) catch {
        expect_true(false);
        return;
    };

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        * = idx.add(@as(u64, i + 1), data[i * dim .. i * dim + dim]) catch {
            expect_true(false);
            return;
        };
    }

    // Boundary query halfway between cluster 1 (base=20) and cluster 2 (base=40)
    const boundary_q: [8]f32 = [30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0, 30.0];

    let hits1: [16]IvfPqHit = [IvfPqHit{}; 16];
    let hits1_s: []IvfPqHit = hits1[0..];
    const n1 = idx.search(boundary_q[0..], 1, *hits1_s);

    let hits4: [16]IvfPqHit = [IvfPqHit{}; 16];
    let hits4_s: []IvfPqHit = hits4[0..];
    const n4 = idx.search(boundary_q[0..], 4, *hits4_s);

    // Probing 1 list returns at most 8 docs (1 cluster); probing 4 lists fills all 16 top-K slots
    expect_eq(n1, @as(usize, 8));
    expect_eq(n4, @as(usize, 16));
    expect_true(hits4[0].approx_l2_sq <= hits1[0].approx_l2_sq + 0.001);
}

test("IvfPqIndex: multi-cluster Top-K ADC ranking accuracy & monotonic ordering") {
    const dim: usize = 8;
    let data: [256]f32 = [0.0; 256];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, dim, 4, 8);

    const cfg = IvfPqConfig{ dim: dim, nlist: 4, m_subspaces: 4, ksub: 8, nprobe: 2, train_iters: 10 };
    let idx = IvfPqIndex.init(cfg) catch {
        expect_true(false);
        return;
    };
    idx.train(data[0..], 32) catch {
        expect_true(false);
        return;
    };

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        * = idx.add(@as(u64, 1000 + i), data[i * dim .. i * dim + dim]) catch {
            expect_true(false);
            return;
        };
    }

    // Query exact vector #19 (doc_id = 1019, inside cluster 2)
    const target_idx: usize = 19;
    const query: []const f32 = data[target_idx * dim .. target_idx * dim + dim];

    let hits: [5]IvfPqHit = [IvfPqHit{}; 5];
    let hits_s: []IvfPqHit = hits[0..];
    const n = idx.search(query, 2, *hits_s);
    expect_eq(n, @as(usize, 5));
    expect_eq(hits[0].doc_id, @as(u64, 1019));
    expect_true(hits[0].approx_l2_sq < 0.05);

    // Verify monotonic non-decreasing approx_l2_sq ordering
    let k: usize = 1;
    while (k < n) : (k += 1) {
        expect_true(hits[k - 1].approx_l2_sq <= hits[k].approx_l2_sq);
    }
}

test("IvfPqIndex: compression ratio stats(), DimMismatch guard & cap.vector facade parity") {
    // dim=64 (256B raw f32) compressed to m_subspaces=8 (8B code) -> 32.00x (3200)
    const cfg = cap.vector.IvfPqConfig{
        dim: 64,
        nlist: 2,
        m_subspaces: 8,
        ksub: 4,
        nprobe: 2,
        train_iters: 4,
    };
    let idx = cap.vector.IvfPqIndex.init(cfg) catch {
        expect_true(false);
        return;
    };
    const st = idx.stats();
    expect_eq(st.bytes_per_code, @as(usize, 8));
    expect_eq(st.compression_ratio_x100, @as(u32, 3200));
    expect_eq(st.nlist, @as(usize, 2));
    expect_eq(st.m_subspaces, @as(usize, 8));
    expect_eq(st.ksub, @as(usize, 4));
    expect_true(!st.trained);

    // DimMismatch on train with too-short flat buffer
    const short_buf: [16]f32 = [0.0; 16];
    let got_dim_err = false;
    idx.train(short_buf[0..], 4) catch {
        got_dim_err = true;
    };
    expect_true(got_dim_err);
}

fn fill1024x32Dataset(out_flat: []f32) {
    // 16 distinct 32D clusters x 64 vectors/cluster = 1,024 vectors (32,768 floats)
    const dim: usize = 32;
    let c: usize = 0;
    while (c < 16) : (c += 1) {
        let p: usize = 0;
        while (p < 64) : (p += 1) {
            const idx: usize = c * 64 + p;
            const c_f: f32 = @as(f32, @floatFromInt(c));
            const p_f: f32 = (@as(f32, @floatFromInt(p)) - 31.5) * 0.03;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                const center: f32 = if ((d % 4) == (c % 4)) (c_f * 8.0 + 4.0) else (c_f * 2.0);
                const jitter: f32 = if ((d % 2) == 0) p_f else (0.0 - p_f * 0.5);
                out_flat[idx * dim + d] = center + jitter + @as(f32, @floatFromInt(d)) * 0.02;
            }
        }
    }
}

test("IvfPqFlatIndex: 1,024x32D K-Means++ training, Multi-Probe (nprobe=1 vs 4) & 16x compression") {
    const cfg = IvfPqFlatConfig{
        dim: 32,
        max_docs: 1024,
        nlist: 16,
        m_subspaces: 8,
        ksub: 16,
        train_iters: 6,
        seed: 20261123,
    };
    expect_true(cfg.isValid());
    expect_eq(ivfPqCoarseBufLen(cfg), @as(usize, 512));
    expect_eq(ivfPqCodebookBufLen(cfg), @as(usize, 512));
    expect_eq(ivfPqCodesBufLen(cfg), @as(usize, 8192));
    expect_eq(ivfPqSearchWorkF32Len(cfg), @as(usize, 176));

    let data: [32768]f32 = [0.0; 32768];
    fill1024x32Dataset(data[0..]);

    let coarse: [512]f32 = [0.0; 512];
    let cb: [512]f32 = [0.0; 512];
    let codes: [8192]u8 = [0; 8192];
    let doc_ids: [1024]u64 = [0; 1024];
    let deleted: [1024]u8 = [0; 1024];
    let heads: [16]u32 = [0; 16];
    let next: [1024]u32 = [0; 1024];
    let counts: [16]u32 = [0; 16];

    let idx = IvfPqFlatIndex.init(
        cfg,
        coarse[0..],
        cb[0..],
        codes[0..],
        doc_ids[0..],
        deleted[0..],
        heads[0..],
        next[0..],
        counts[0..],
    ) catch {
        expect_true(false);
        return;
    };

    let wf: [1536]f32 = [0.0; 1536]; // >= ivfPqTrainWorkF32Len(cfg, 1024) == 1536
    let wu: [1040]u32 = [0; 1040];   // >= ivfPqTrainWorkU32Len(cfg, 1024) == 1040
    expect_eq(ivfPqTrainWorkF32Len(cfg, 1024), @as(usize, 1536));
    expect_eq(ivfPqTrainWorkU32Len(cfg, 1024), @as(usize, 1040));

    idx.train(data[0..], 1024, wf[0..], wu[0..]) catch {
        expect_true(false);
        return;
    };

    let i: usize = 0;
    while (i < 1024) : (i += 1) {
        * = idx.add(@as(u64, 10000 + i), data[i * 32 .. i * 32 + 32]) catch {
            expect_true(false);
            return;
        };
    }

    const st = idx.stats();
    expect_eq(st.doc_count, @as(usize, 1024));
    expect_eq(st.bytes_per_code, @as(usize, 8));
    expect_eq(st.compression_ratio_x100, @as(u32, 1600)); // 128B f32 -> 8B PQ code = 16.00x
    expect_true(st.trained);

    // Exact query lookup for doc #530 (doc_id = 10530) must land in Top-3 with tiny L2 error
    let adc_work: [176]f32 = [0.0; 176];
    let hits: [8]IvfPqHit = [IvfPqHit{}; 8];
    const n_hits = idx.search(data[530 * 32 .. 530 * 32 + 32], 4, adc_work[0..], hits[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n_hits, @as(usize, 8));
    let found_target: bool = false;
    let hi: usize = 0;
    while (hi < 3) : (hi += 1) {
        if (hits[hi].doc_id == 10530) found_target = true;
    }
    expect_true(found_target);

    // Multi-probe comparison on boundary query halfway between cluster 4 and cluster 5
    let boundary_q: [32]f32 = [0.0; 32];
    let bd: usize = 0;
    while (bd < 32) : (bd += 1) {
        boundary_q[bd] = (data[4 * 64 * 32 + bd] + data[5 * 64 * 32 + bd]) * 0.5;
    }
    let h1: [100]IvfPqHit = [IvfPqHit{}; 100];
    let h4: [100]IvfPqHit = [IvfPqHit{}; 100];
    const cnt1 = idx.search(boundary_q[0..], 1, adc_work[0..], h1[0..]) catch {
        expect_true(false);
        return;
    };
    const cnt4 = idx.search(boundary_q[0..], 4, adc_work[0..], h4[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(cnt4 >= cnt1);
    expect_true(h4[0].approx_l2_sq <= h1[0].approx_l2_sq + 0.001);
}

test("IvfPqFlatIndex: soft deletion markDeleted filters search & updates live doc_count") {
    const cfg = IvfPqFlatConfig{
        dim: 16,
        max_docs: 64,
        nlist: 4,
        m_subspaces: 4,
        ksub: 8,
        train_iters: 6,
        seed: 777,
    };
    let data: [1024]f32 = [0.0; 1024];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, 16, 4, 16);

    let coarse: [64]f32 = [0.0; 64];
    let cb: [128]f32 = [0.0; 128];
    let codes: [256]u8 = [0; 256];
    let doc_ids: [64]u64 = [0; 64];
    let deleted: [64]u8 = [0; 64];
    let heads: [4]u32 = [0; 4];
    let next: [64]u32 = [0; 64];
    let counts: [4]u32 = [0; 4];

    let idx = IvfPqFlatIndex.init(cfg, coarse[0..], cb[0..], codes[0..], doc_ids[0..], deleted[0..], heads[0..], next[0..], counts[0..]) catch {
        expect_true(false);
        return;
    };
    let wf: [256]f32 = [0.0; 256];
    let wu: [128]u32 = [0; 128];
    idx.train(data[0..], 64, wf[0..], wu[0..]) catch {
        expect_true(false);
        return;
    };
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        * = idx.add(@as(u64, 500 + i), data[i * 16 .. i * 16 + 16]) catch {
            expect_true(false);
            return;
        };
    }

    let adc: [64]f32 = [0.0; 64];
    let hits: [5]IvfPqHit = [IvfPqHit{}; 5];
    const q = data[20 * 16 .. 20 * 16 + 16];
    const n_before = idx.search(q, 4, adc[0..], hits[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n_before, @as(usize, 5));
    const top1_id = hits[0].doc_id;

    // Soft-delete top1_id -> second delete returns false, live doc_count == 63, search excludes top1_id
    expect_true(idx.markDeleted(top1_id));
    expect_true(!idx.markDeleted(top1_id));
    expect_eq(idx.stats().doc_count, @as(usize, 63));

    const n_after = idx.search(q, 4, adc[0..], hits[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n_after, @as(usize, 5));
    let k: usize = 0;
    while (k < n_after) : (k += 1) {
        expect_true(hits[k].doc_id != top1_id);
    }
}

test("IvfPqFlatIndex: CRC32 binary snapshot roundtrip yields bit-exact search & reconstruction") {
    const cfg = IvfPqFlatConfig{
        dim: 16,
        max_docs: 64,
        nlist: 4,
        m_subspaces: 4,
        ksub: 8,
        train_iters: 6,
        seed: 12345,
    };
    let data: [1024]f32 = [0.0; 1024];
    let data_s: []f32 = data[0..];
    buildClusteredDataset(*data_s, 16, 4, 16);

    let coarse1: [64]f32 = [0.0; 64];
    let cb1: [128]f32 = [0.0; 128];
    let codes1: [256]u8 = [0; 256];
    let ids1: [64]u64 = [0; 64];
    let del1: [64]u8 = [0; 64];
    let heads1: [4]u32 = [0; 4];
    let next1: [64]u32 = [0; 64];
    let cnts1: [4]u32 = [0; 4];

    let idx1 = IvfPqFlatIndex.init(cfg, coarse1[0..], cb1[0..], codes1[0..], ids1[0..], del1[0..], heads1[0..], next1[0..], cnts1[0..]) catch {
        expect_true(false);
        return;
    };
    let wf: [256]f32 = [0.0; 256];
    let wu: [128]u32 = [0; 128];
    idx1.train(data[0..], 64, wf[0..], wu[0..]) catch {
        expect_true(false);
        return;
    };
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        * = idx1.add(@as(u64, 2000 + i), data[i * 16 .. i * 16 + 16]) catch {
            expect_true(false);
            return;
        };
    }
    expect_true(idx1.markDeleted(2010));

    // Serialize into snapshot buffer
    const snap_need = ivfPqSnapshotMaxBytes(cfg, 64);
    let snap: [4096]u8 = [0; 4096];
    expect_true(snap.len >= snap_need);
    const snap_len = serializeIvfPqSnapshot(idx1, snap[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(snap_len, snap_need);

    // Deserialize into completely independent backing slices (zero re-training!)
    let coarse2: [64]f32 = [0.0; 64];
    let cb2: [128]f32 = [0.0; 128];
    let codes2: [256]u8 = [0; 256];
    let ids2: [64]u64 = [0; 64];
    let del2: [64]u8 = [0; 64];
    let heads2: [4]u32 = [0; 4];
    let next2: [64]u32 = [0; 64];
    let cnts2: [4]u32 = [0; 4];

    let idx2 = deserializeIvfPqSnapshot(
        snap[0..snap_len],
        coarse2[0..],
        cb2[0..],
        codes2[0..],
        ids2[0..],
        del2[0..],
        heads2[0..],
        next2[0..],
        cnts2[0..],
    ) catch {
        expect_true(false);
        return;
    };

    expect_eq(idx2.stats().doc_count, @as(usize, 63));
    expect_true(idx2.stats().trained);

    // Search results must match bit-for-bit
    let adc: [64]f32 = [0.0; 64];
    let hits1: [5]IvfPqHit = [IvfPqHit{}; 5];
    let hits2: [5]IvfPqHit = [IvfPqHit{}; 5];
    const q = data[10 * 16 .. 10 * 16 + 16];
    const n1 = idx1.search(q, 4, adc[0..], hits1[0..]) catch {
        expect_true(false);
        return;
    };
    const n2 = idx2.search(q, 4, adc[0..], hits2[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n1, n2);
    let k: usize = 0;
    while (k < n1) : (k += 1) {
        expect_eq(hits1[k].doc_id, hits2[k].doc_id);
        expect_eq(hits1[k].approx_l2_sq, hits2[k].approx_l2_sq);
        expect_true(hits2[k].doc_id != 2010);
    }

    // Reconstruct output must match bit-for-bit
    let r1: [16]f32 = [0.0; 16];
    let r2: [16]f32 = [0.0; 16];
    expect_true(idx1.reconstruct(7, r1[0..]));
    expect_true(idx2.reconstruct(7, r2[0..]));
    let d: usize = 0;
    while (d < 16) : (d += 1) {
        expect_eq(r1[d], r2[d]);
    }
}

test("IvfPqFlatIndex: snapshot tamper ChecksumMismatch, InvalidSnapshot & BufferTooSmall guards") {
    const cfg = IvfPqFlatConfig{ dim: 8, max_docs: 16, nlist: 2, m_subspaces: 4, ksub: 4, train_iters: 4, seed: 99 };
    let coarse: [16]f32 = [0.0; 16];
    let cb: [32]f32 = [0.0; 32];
    let codes: [64]u8 = [0; 64];
    let ids: [16]u64 = [0; 16];
    let del: [16]u8 = [0; 16];
    let heads: [2]u32 = [0; 2];
    let next: [16]u32 = [0; 16];
    let cnts: [2]u32 = [0; 2];

    let idx = IvfPqFlatIndex.init(cfg, coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch {
        expect_true(false);
        return;
    };

    // Tiny output buffer -> BufferTooSmall
    let tiny: [32]u8 = [0; 32];
    let got_small: bool = false;
    * = serializeIvfPqSnapshot(idx, tiny[0..]) catch |e| {
        if (e == IvfPqFlatError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);

    let snap: [512]u8 = [0; 512];
    const snap_len = serializeIvfPqSnapshot(idx, snap[0..]) catch {
        expect_true(false);
        return;
    };

    // Flip 1 byte in payload -> ChecksumMismatch
    snap[60] ^= 0x80;
    let got_crc: bool = false;
    * = deserializeIvfPqSnapshot(snap[0..snap_len], coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch |e| {
        if (e == IvfPqFlatError.ChecksumMismatch) got_crc = true;
    };
    expect_true(got_crc);

    // Corrupt magic -> InvalidSnapshot
    snap[0] = 0x00;
    let got_inv: bool = false;
    * = deserializeIvfPqSnapshot(snap[0..snap_len], coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch |e| {
        if (e == IvfPqFlatError.InvalidSnapshot) got_inv = true;
    };
    expect_true(got_inv);
}

test("IvfPqFlatIndex: error guards (InvalidConfig, NotTrained, DimMismatch) & cap.vector facade parity") {
    expect_eq(cap.vector.IVFPQ_SNAPSHOT_MAGIC, IVFPQ_SNAPSHOT_MAGIC);
    expect_eq(cap.vector.IVFPQ_SNAPSHOT_VERSION, IVFPQ_SNAPSHOT_VERSION);
    expect_eq(cap.vector.IVFPQ_INVALID_LINK, IVFPQ_INVALID_LINK);

    // ksub > 256 -> InvalidConfig
    const bad_cfg = cap.vector.IvfPqFlatConfig{ dim: 16, max_docs: 16, nlist: 2, m_subspaces: 4, ksub: 300, train_iters: 4 };
    expect_true(!bad_cfg.isValid());
    let coarse: [32]f32 = [0.0; 32];
    let cb: [64]f32 = [0.0; 64];
    let codes: [64]u8 = [0; 64];
    let ids: [16]u64 = [0; 16];
    let del: [16]u8 = [0; 16];
    let heads: [2]u32 = [0; 2];
    let next: [16]u32 = [0; 16];
    let cnts: [2]u32 = [0; 2];

    let got_cfg: bool = false;
    * = cap.vector.IvfPqFlatIndex.init(bad_cfg, coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch |e| {
        if (e == IvfPqFlatError.InvalidConfig) got_cfg = true;
    };
    expect_true(got_cfg);

    // Valid init -> add before train fails with NotTrained
    const ok_cfg = IvfPqFlatConfig{ dim: 8, max_docs: 4, nlist: 2, m_subspaces: 4, ksub: 4, train_iters: 4 };
    let idx = IvfPqFlatIndex.init(ok_cfg, coarse[0..], cb[0..], codes[0..], ids[0..], del[0..], heads[0..], next[0..], cnts[0..]) catch {
        expect_true(false);
        return;
    };
    const v8: [8]f32 = [1.0; 8];
    let got_nt: bool = false;
    * = idx.add(1, v8[0..]) catch |e| {
        if (e == IvfPqFlatError.NotTrained) got_nt = true;
    };
    expect_true(got_nt);
}

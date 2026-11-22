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

// Origin: Pure-CP Stdlib Test Suite (IVF-Flat Coarse-Quantized Inverted File ANN Index) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/ivf.cp
// Covers:
//   1. 8-cluster 240-doc 16D CSR build & exact sum(clusterSize) == 240
//   2. nprobe == 1 fast same-cluster hit & nprobe == n_clusters 100% match with brute-force (L2 & Cosine)
//   3. Monotonic recall@5 as nprobe increases (1 -> 2 -> 4 -> 8) on a multi-cluster boundary query
//   4. Empty buckets safely skipped (nprobe=1 empty bucket -> 0 hits; nprobe=4 skips empty buckets)
//   5. n_clusters=0, dim=0, nprobe=0, buffer underflow & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    IvfHit,
    IvfBuildStats,
    IvfFlatIndex,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn bruteL2Sq(a: []const f64, b: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

fn bruteCosDist(a: []const f64, b: []const f64): f64 {
    let dot: f64 = 0.0;
    let na: f64 = 0.0;
    let nb: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    if (na == 0.0 || nb == 0.0) return 1.0;
    let c = dot / (std.math.sqrt(na) * std.math.sqrt(nb));
    if (c > 1.0) c = 1.0;
    if (c < -1.0) c = -1.0;
    return 1.0 - c;
}

fn fill8Clusters240Docs(centroids: *[]f64, corpus: *[]f64) {
    const DIM: usize = 16;
    const K: usize = 8;
    const PER_CLUSTER: usize = 30;

    let c: usize = 0;
    while (c < K) : (c += 1) {
        let d: usize = 0;
        while (d < DIM) : (d += 1) {
            centroids[c * DIM + d] = if (d == c * 2 || d == c * 2 + 1) 10.0 else 0.0;
        }
        let m: usize = 0;
        while (m < PER_CLUSTER) : (m += 1) {
            const doc = c * PER_CLUSTER + m;
            const fm = m as f64;
            let j: usize = 0;
            while (j < DIM) : (j += 1) {
                const fj = j as f64;
                const base_v = centroids[c * DIM + j];
                corpus[doc * DIM + j] = base_v + 0.15 * std.math.sin(fm * 0.7 + fj * 0.3) + 0.02 * fm;
            }
        }
    }
}

fn testBuild8Clusters240DocsAndClusterSizes()! {
    let centroids_buf: [128]f64 = [0.0; 128];  // 8 * 16
    let corpus_buf: [3840]f64 = [0.0; 3840];  // 240 * 16
    let centroids: []f64 = centroids_buf[0..];
    let corpus: []f64 = corpus_buf[0..];
    fill8Clusters240Docs(centroids, corpus);

    let offsets_buf: [9]usize = [0; 9];
    let doc_ids_buf: [240]usize = [0; 240];
    let counts_buf: [8]usize = [0; 8];
    let offsets: []usize = offsets_buf[0..];
    let doc_ids: []usize = doc_ids_buf[0..];
    let counts: []usize = counts_buf[0..];

    const st_opt = IvfFlatIndex.buildFromCentroids(
        corpus_buf[0..], 240, 16, centroids_buf[0..], 8, offsets, doc_ids, counts,
    );
    expect_eq(st_opt != null, true);
    const st = st_opt ?? IvfBuildStats{};
    expect_eq(st.n_clusters, 8);
    expect_eq(st.n_docs, 240);
    expect_eq(st.min_cluster_size, 30);
    expect_eq(st.max_cluster_size, 30);

    const idx = IvfFlatIndex{
        centroids: centroids_buf[0..],
        list_offsets: offsets,
        list_doc_ids: doc_ids,
        n_clusters: 8,
        dim: 16,
    };
    let sum_sizes: usize = 0;
    let c: usize = 0;
    while (c < 8) : (c += 1) {
        const sz = IvfFlatIndex.clusterSize(idx, c);
        expect_eq(sz, 30);
        sum_sizes += sz;
    }
    expect_eq(sum_sizes, 240);
    expect_eq(IvfFlatIndex.clusterSize(idx, 99), 0);
}

fn testNprobe1AndFullProbeExactMatchL2AndCosine()! {
    let centroids_buf: [128]f64 = [0.0; 128];
    let corpus_buf: [3840]f64 = [0.0; 3840];
    let centroids: []f64 = centroids_buf[0..];
    let corpus: []f64 = corpus_buf[0..];
    fill8Clusters240Docs(centroids, corpus);

    let offsets_buf: [9]usize = [0; 9];
    let doc_ids_buf: [240]usize = [0; 240];
    let counts_buf: [8]usize = [0; 8];
    let offsets: []usize = offsets_buf[0..];
    let doc_ids: []usize = doc_ids_buf[0..];
    let counts: []usize = counts_buf[0..];
    expect_eq(
        IvfFlatIndex.buildFromCentroids(corpus_buf[0..], 240, 16, centroids_buf[0..], 8, offsets, doc_ids, counts) != null,
        true,
    );

    const idx = IvfFlatIndex{
        centroids: centroids_buf[0..],
        list_offsets: offsets,
        list_doc_ids: doc_ids,
        n_clusters: 8,
        dim: 16,
    };

    // Query = exact copy of doc 100 (cluster 3, m = 10) + tiny perturbation
    let query: [16]f64 = [0.0; 16];
    let d: usize = 0;
    while (d < 16) : (d += 1) {
        query[d] = corpus_buf[100 * 16 + d] + 0.001;
    }

    let p_ids_buf: [8]usize = [0; 8];
    let p_dists_buf: [8]f64 = [0.0; 8];
    let hits_buf: [5]IvfHit = [IvfHit{}; 5];
    let p_ids: []usize = p_ids_buf[0..];
    let p_dists: []f64 = p_dists_buf[0..];
    let hits: []IvfHit = hits_buf[0..];

    // 1. nprobe == 1 fast same-cluster hit
    const n1 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 1, 5, p_ids, p_dists, hits) ?? 0;
    expect_eq(n1, 5);
    expect_eq(hits_buf[0].doc_id, 100);
    expect_eq(hits_buf[0].cluster_id, 3);

    // 2. nprobe == 8 (all clusters) must match brute-force L2 Top-5 100%
    const n_l2 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 8, 5, p_ids, p_dists, hits) ?? 0;
    expect_eq(n_l2, 5);

    let bf_ids: [5]usize = [0; 5];
    let bf_dists: [5]f64 = [1000000000.0; 5];
    let doc: usize = 0;
    while (doc < 240) : (doc += 1) {
        const dist = bruteL2Sq(query[0..], corpus_buf[doc * 16 .. (doc + 1) * 16]);
        let pos: usize = 5;
        while (pos > 0 && (dist < bf_dists[pos - 1] || (dist == bf_dists[pos - 1] && doc < bf_ids[pos - 1]))) {
            if (pos < 5) {
                bf_dists[pos] = bf_dists[pos - 1];
                bf_ids[pos] = bf_ids[pos - 1];
            }
            pos -= 1;
        }
        if (pos < 5) {
            bf_dists[pos] = dist;
            bf_ids[pos] = doc;
        }
    }
    let k: usize = 0;
    while (k < 5) : (k += 1) {
        expect_eq(hits_buf[k].doc_id, bf_ids[k]);
        expect_eq(approxF64(hits_buf[k].dist_sq, bf_dists[k], EPS_F64), true);
    }

    // 3. nprobe == 8 searchCosine must match brute-force Cosine-distance Top-5 100%
    const n_cos = IvfFlatIndex.searchCosine(idx, corpus_buf[0..], query[0..], 8, 5, p_ids, p_dists, hits) ?? 0;
    expect_eq(n_cos, 5);

    let bfc_ids: [5]usize = [0; 5];
    let bfc_dists: [5]f64 = [10.0; 5];
    let doc2: usize = 0;
    while (doc2 < 240) : (doc2 += 1) {
        const dist = bruteCosDist(query[0..], corpus_buf[doc2 * 16 .. (doc2 + 1) * 16]);
        let pos: usize = 5;
        while (pos > 0 && (dist < bfc_dists[pos - 1] || (dist == bfc_dists[pos - 1] && doc2 < bfc_ids[pos - 1]))) {
            if (pos < 5) {
                bfc_dists[pos] = bfc_dists[pos - 1];
                bfc_ids[pos] = bfc_ids[pos - 1];
            }
            pos -= 1;
        }
        if (pos < 5) {
            bfc_dists[pos] = dist;
            bfc_ids[pos] = doc2;
        }
    }
    let kc: usize = 0;
    while (kc < 5) : (kc += 1) {
        expect_eq(hits_buf[kc].doc_id, bfc_ids[kc]);
        expect_eq(approxF64(hits_buf[kc].dist_sq, bfc_dists[kc], EPS_F64), true);
    }
}

fn countRecallAt5(hits: []const IvfHit, n_hits: usize, truth: []const usize): usize {
    let matched: usize = 0;
    let i: usize = 0;
    while (i < n_hits) : (i += 1) {
        let j: usize = 0;
        while (j < truth.len) : (j += 1) {
            if (hits[i].doc_id == truth[j]) {
                matched += 1;
                break;
            }
        }
    }
    return matched;
}

fn testMonotonicRecallWithIncreasingNprobe()! {
    let centroids_buf: [128]f64 = [0.0; 128];
    let corpus_buf: [3840]f64 = [0.0; 3840];
    let centroids: []f64 = centroids_buf[0..];
    let corpus: []f64 = corpus_buf[0..];
    fill8Clusters240Docs(centroids, corpus);

    let offsets_buf: [9]usize = [0; 9];
    let doc_ids_buf: [240]usize = [0; 240];
    let counts_buf: [8]usize = [0; 8];
    let offsets: []usize = offsets_buf[0..];
    let doc_ids: []usize = doc_ids_buf[0..];
    let counts: []usize = counts_buf[0..];
    * = IvfFlatIndex.buildFromCentroids(corpus_buf[0..], 240, 16, centroids_buf[0..], 8, offsets, doc_ids, counts);

    const idx = IvfFlatIndex{
        centroids: centroids_buf[0..],
        list_offsets: offsets,
        list_doc_ids: doc_ids,
        n_clusters: 8,
        dim: 16,
    };

    // Boundary query sitting between clusters 0, 1, and 2
    let query: [16]f64 = [0.0; 16];
    query[0] = 5.2;
    query[1] = 5.2;
    query[2] = 5.1;
    query[3] = 5.1;
    query[4] = 5.0;
    query[5] = 5.0;

    let p_ids_buf: [8]usize = [0; 8];
    let p_dists_buf: [8]f64 = [0.0; 8];
    let hits_buf: [5]IvfHit = [IvfHit{}; 5];
    let p_ids: []usize = p_ids_buf[0..];
    let p_dists: []f64 = p_dists_buf[0..];
    let hits: []IvfHit = hits_buf[0..];

    // Ground truth from full probe (nprobe == 8)
    const n_full = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 8, 5, p_ids, p_dists, hits) ?? 0;
    expect_eq(n_full, 5);
    let truth: [5]usize = [0; 5];
    let t: usize = 0;
    while (t < 5) : (t += 1) {
        truth[t] = hits_buf[t].doc_id;
    }

    const n1 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 1, 5, p_ids, p_dists, hits) ?? 0;
    const r1 = countRecallAt5(hits_buf[0..], n1, truth[0..]);
    const n2 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 2, 5, p_ids, p_dists, hits) ?? 0;
    const r2 = countRecallAt5(hits_buf[0..], n2, truth[0..]);
    const n4 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 4, 5, p_ids, p_dists, hits) ?? 0;
    const r4 = countRecallAt5(hits_buf[0..], n4, truth[0..]);
    const n8 = IvfFlatIndex.searchL2(idx, corpus_buf[0..], query[0..], 8, 5, p_ids, p_dists, hits) ?? 0;
    const r8 = countRecallAt5(hits_buf[0..], n8, truth[0..]);

    expect_eq(r1 <= r2, true);
    expect_eq(r2 <= r4, true);
    expect_eq(r4 <= r8, true);
    expect_eq(r8, 5);
}

fn testEmptyBucketsSkippedSafely()! {
    // 4 centroids in 2D; all 4 docs are near centroids 0 and 3 (clusters 1 & 2 are empty)
    const centroids: [8]f64 = [
        0.0, 0.0,     // cluster 0
        10.0, 0.0,    // cluster 1 (empty)
        0.0, 10.0,    // cluster 2 (empty)
        10.0, 10.0,   // cluster 3
    ];
    const corpus: [8]f64 = [
        0.1, 0.0,     // doc 0 -> cluster 0
        -0.1, 0.2,    // doc 1 -> cluster 0
        9.9, 10.0,    // doc 2 -> cluster 3
        10.2, 9.8,    // doc 3 -> cluster 3
    ];
    let offsets_buf: [5]usize = [0; 5];
    let doc_ids_buf: [4]usize = [0; 4];
    let counts_buf: [4]usize = [0; 4];
    let offsets: []usize = offsets_buf[0..];
    let doc_ids: []usize = doc_ids_buf[0..];
    let counts: []usize = counts_buf[0..];

    const st = IvfFlatIndex.buildFromCentroids(corpus[0..], 4, 2, centroids[0..], 4, offsets, doc_ids, counts) ?? IvfBuildStats{};
    expect_eq(st.min_cluster_size, 0);
    expect_eq(st.max_cluster_size, 2);

    const idx = IvfFlatIndex{
        centroids: centroids[0..],
        list_offsets: offsets,
        list_doc_ids: doc_ids,
        n_clusters: 4,
        dim: 2,
    };
    expect_eq(IvfFlatIndex.clusterSize(idx, 1), 0);
    expect_eq(IvfFlatIndex.clusterSize(idx, 2), 0);

    let p_ids_buf: [4]usize = [0; 4];
    let p_dists_buf: [4]f64 = [0.0; 4];
    let hits_buf: [4]IvfHit = [IvfHit{}; 4];
    let p_ids: []usize = p_ids_buf[0..];
    let p_dists: []f64 = p_dists_buf[0..];
    let hits: []IvfHit = hits_buf[0..];

    // Query at empty cluster 1 with nprobe=1 -> 0 hits safely
    const q_empty: [2]f64 = [10.0, 0.0];
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_empty[0..], 1, 2, p_ids, p_dists, hits), 0);

    // Same query with nprobe=4 -> skips empty buckets and returns closest docs from clusters 0 & 3
    const n_all = IvfFlatIndex.searchL2(idx, corpus[0..], q_empty[0..], 4, 2, p_ids, p_dists, hits) ?? 0;
    expect_eq(n_all, 2);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const centroids: [4]f64 = [0.0, 0.0, 1.0, 1.0];
    const centroids_nan: [4]f64 = [0.0, nan_v, 1.0, 1.0];
    const corpus: [4]f64 = [0.1, 0.1, 0.9, 0.9];
    const corpus_nan: [4]f64 = [0.1, nan_v, 0.9, 0.9];

    let off3_buf: [3]usize = [0; 3];
    let off2_buf: [2]usize = [0; 2];
    let ids2_buf: [2]usize = [0; 2];
    let ids1_buf: [1]usize = [0; 1];
    let cnt2_buf: [2]usize = [0; 2];
    let cnt1_buf: [1]usize = [0; 1];
    let off3: []usize = off3_buf[0..];
    let off2: []usize = off2_buf[0..];
    let ids2: []usize = ids2_buf[0..];
    let ids1: []usize = ids1_buf[0..];
    let cnt2: []usize = cnt2_buf[0..];
    let cnt1: []usize = cnt1_buf[0..];

    // 1. buildFromCentroids defenses
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids[0..], 0, off3, ids2, cnt2) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 0, centroids[0..], 2, off3, ids2, cnt2) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids[0..], 2, off2, ids2, cnt2) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids[0..], 2, off3, ids1, cnt2) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids[0..], 2, off3, ids2, cnt1) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids_nan[0..], 2, off3, ids2, cnt2) == null, true);
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus_nan[0..], 2, 2, centroids[0..], 2, off3, ids2, cnt2) == null, true);

    // Build valid index for search defenses
    expect_eq(IvfFlatIndex.buildFromCentroids(corpus[0..], 2, 2, centroids[0..], 2, off3, ids2, cnt2) != null, true);
    const idx = IvfFlatIndex{
        centroids: centroids[0..],
        list_offsets: off3,
        list_doc_ids: ids2,
        n_clusters: 2,
        dim: 2,
    };

    const q_ok: [2]f64 = [0.2, 0.2];
    const q_nan: [2]f64 = [0.2, nan_v];
    const q_bad_dim: [3]f64 = [0.2, 0.2, 0.2];
    let p2_ids_buf: [2]usize = [0; 2];
    let p2_dists_buf: [2]f64 = [0.0; 2];
    let h2_buf: [2]IvfHit = [IvfHit{}; 2];
    let p2_ids: []usize = p2_ids_buf[0..];
    let p1_ids: []usize = p2_ids_buf[0..1];
    let p2_dists: []f64 = p2_dists_buf[0..];
    let h2: []IvfHit = h2_buf[0..];
    let h1: []IvfHit = h2_buf[0..1];

    // 2. searchL2 / searchCosine defenses
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_ok[0..], 0, 2, p2_ids, p2_dists, h2) == null, true);
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_ok[0..], 2, 0, p2_ids, p2_dists, h2), 0);
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_bad_dim[0..], 2, 2, p2_ids, p2_dists, h2) == null, true);
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_nan[0..], 2, 2, p2_ids, p2_dists, h2) == null, true);
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_ok[0..], 2, 2, p1_ids, p2_dists, h2) == null, true);
    expect_eq(IvfFlatIndex.searchL2(idx, corpus[0..], q_ok[0..], 2, 2, p2_ids, p2_dists, h1) == null, true);
    expect_eq(IvfFlatIndex.searchCosine(idx, corpus[0..], q_nan[0..], 2, 2, p2_ids, p2_dists, h2) == null, true);
}

fn main() {
    test("1. ivf: 8-cluster 240-doc 16D CSR build & sum(clusterSize) == 240", () => {
        testBuild8Clusters240DocsAndClusterSizes()?;
    });
    test("2. ivf: nprobe=1 same-cluster hit & nprobe=K 100% match with brute-force L2/Cosine", () => {
        testNprobe1AndFullProbeExactMatchL2AndCosine()?;
    });
    test("3. ivf: monotonic recall@5 with increasing nprobe (1->2->4->8)", () => {
        testMonotonicRecallWithIncreasingNprobe()?;
    });
    test("4. ivf: empty buckets safely skipped & out-of-bounds clusterSize", () => {
        testEmptyBucketsSkippedSafely()?;
    });
    test("5. ivf: n_clusters=0, dim=0, nprobe=0, buffer underflow & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}

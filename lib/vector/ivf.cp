// Origin: Pure-CP Stdlib (RAG IVF-Flat Coarse-Quantized Inverted File ANN Index) | Created: 2026-11-22
// Zero-heap-allocation Inverted File (IVF-Flat) approximate nearest neighbor index backed by
// caller-owned CSR inverted lists, supporting L2 squared and Cosine multi-probe (`nprobe`) search.

const std = @import("std");

/// Single nearest-neighbor hit returned by `IvfFlatIndex.searchL2` / `searchCosine`.
/// For `searchCosine`, `dist_sq` stores the cosine distance `1.0 - cos(query, doc)` in `[0.0, 2.0]`.
pub struct IvfHit {
    doc_id: usize = 0;
    dist_sq: f64 = 0.0;
    cluster_id: usize = 0;
}

/// Summary statistics returned by `IvfFlatIndex.buildFromCentroids`.
pub struct IvfBuildStats {
    n_clusters: usize = 0;
    n_docs: usize = 0;
    min_cluster_size: usize = 0;
    max_cluster_size: usize = 0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn l2DistSqSlice(a: []const f64, b: []const f64): f64? {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const va = a[i];
        const vb = b[i];
        if (!isFiniteF64(va) || !isFiniteF64(vb)) return null;
        const diff = va - vb;
        sum += diff * diff;
    }
    if (!isFiniteF64(sum)) return null;
    return sum;
}

fn cosineDistSlice(a: []const f64, a_norm: f64, b: []const f64): f64? {
    let dot: f64 = 0.0;
    let nb_sq: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const vb = b[i];
        if (!isFiniteF64(vb)) return null;
        dot += a[i] * vb;
        nb_sq += vb * vb;
    }
    if (!isFiniteF64(dot) || !isFiniteF64(nb_sq)) return null;
    if (a_norm == 0.0 || nb_sq == 0.0) return 1.0;
    const denom = a_norm * std.math.sqrt(nb_sq);
    if (denom == 0.0 || !isFiniteF64(denom)) return 1.0;
    let cos = dot / denom;
    if (cos > 1.0) cos = 1.0;
    if (cos < -1.0) cos = -1.0;
    return 1.0 - cos;
}

fn probeBetterAsc(id_a: usize, dist_a: f64, id_b: usize, dist_b: f64): bool {
    if (dist_a != dist_b) return dist_a < dist_b;
    return id_a < id_b;
}

fn selectTopProbes(
    centroids: []const f64,
    n_clusters: usize,
    dim: usize,
    query: []const f64,
    use_cosine: bool,
    q_norm: f64,
    eff_nprobe: usize,
    probe_ids: *[]usize,
    probe_dists: *[]f64,
): bool {
    let filled: usize = 0;
    let c: usize = 0;
    while (c < n_clusters) : (c += 1) {
        const base = c * dim;
        const c_vec = centroids[base .. base + dim];
        const dist_opt = if (use_cosine) cosineDistSlice(query, q_norm, c_vec) else l2DistSqSlice(query, c_vec);
        if (dist_opt == null) return false;
        const dist = dist_opt ?? 0.0;

        if (filled < eff_nprobe) {
            // Insert in sorted ascending order
            let pos = filled;
            while (pos > 0 && probeBetterAsc(c, dist, probe_ids[pos - 1], probe_dists[pos - 1])) {
                probe_ids[pos] = probe_ids[pos - 1];
                probe_dists[pos] = probe_dists[pos - 1];
                pos -= 1;
            }
            probe_ids[pos] = c;
            probe_dists[pos] = dist;
            filled += 1;
        } else if (probeBetterAsc(c, dist, probe_ids[eff_nprobe - 1], probe_dists[eff_nprobe - 1])) {
            let pos = eff_nprobe - 1;
            while (pos > 0 && probeBetterAsc(c, dist, probe_ids[pos - 1], probe_dists[pos - 1])) {
                probe_ids[pos] = probe_ids[pos - 1];
                probe_dists[pos] = probe_dists[pos - 1];
                pos -= 1;
            }
            probe_ids[pos] = c;
            probe_dists[pos] = dist;
        }
    }
    return true;
}

fn ivfHitBetterAsc(a: IvfHit, b: IvfHit): bool {
    if (a.dist_sq != b.dist_sq) return a.dist_sq < b.dist_sq;
    return a.doc_id < b.doc_id;
}

fn ivfSiftDownWorstRoot(heap: *[]IvfHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && ivfHitBetterAsc(heap[worst], heap[right])) {
            worst = right;
        }
        if (!ivfHitBetterAsc(heap[idx], heap[worst])) break;
        const tmp = heap[idx];
        heap[idx] = heap[worst];
        heap[worst] = tmp;
        idx = worst;
    }
}

fn sortIvfHitsAsc(arr: *[]IvfHit, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        ivfSiftDownWorstRoot(arr, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        ivfSiftDownWorstRoot(arr, 0, end);
    }
}

fn ivfSearchInternal(
    index: IvfFlatIndex,
    corpus: []const f64,
    query: []const f64,
    nprobe: usize,
    top_k: usize,
    use_cosine: bool,
    probe_ids_s: *[]usize,
    probe_dists_s: *[]f64,
    out_hits: *[]IvfHit,
): usize? {
    if (index.n_clusters == 0 || index.dim == 0) return null;
    if (nprobe == 0) return null;
    if (top_k == 0) return 0;
    if (query.len != index.dim) return null;
    if (index.n_clusters > std.math.maxInt(usize) / index.dim) return null;
    if (index.centroids.len < index.n_clusters * index.dim) return null;
    if (index.list_offsets.len < index.n_clusters + 1) return null;

    // Validate CSR monotonicity & bounds
    let c_chk: usize = 0;
    while (c_chk < index.n_clusters) : (c_chk += 1) {
        const s = index.list_offsets[c_chk];
        const e = index.list_offsets[c_chk + 1];
        if (e < s || e > index.list_doc_ids.len) return null;
    }

    // Validate query finiteness and compute query norm
    let q_norm_sq: f64 = 0.0;
    let qi: usize = 0;
    while (qi < index.dim) : (qi += 1) {
        const qv = query[qi];
        if (!isFiniteF64(qv)) return null;
        q_norm_sq += qv * qv;
    }
    if (!isFiniteF64(q_norm_sq)) return null;
    const q_norm = if (q_norm_sq > 0.0) std.math.sqrt(q_norm_sq) else 0.0;

    const eff_nprobe = if (nprobe < index.n_clusters) nprobe else index.n_clusters;
    if (probe_ids_s.len < eff_nprobe || probe_dists_s.len < eff_nprobe) return null;

    const total_indexed = index.list_offsets[index.n_clusters];
    if (total_indexed == 0) return 0;
    const max_k = if (top_k < total_indexed) top_k else total_indexed;
    if (out_hits.len < max_k) return null;

    // Stage 1: select `eff_nprobe` closest centroids
    if (!selectTopProbes(
        index.centroids,
        index.n_clusters,
        index.dim,
        query,
        use_cosine,
        q_norm,
        eff_nprobe,
        probe_ids_s,
        probe_dists_s,
    )) {
        return null;
    }

    // Stage 2: scan selected inverted buckets and maintain Top-K heap
    let heap_len: usize = 0;
    let p: usize = 0;
    while (p < eff_nprobe) : (p += 1) {
        const cid = probe_ids_s[p];
        const start = index.list_offsets[cid];
        const end = index.list_offsets[cid + 1];
        if (start == end) continue;

        let pos = start;
        while (pos < end) : (pos += 1) {
            const doc_id = index.list_doc_ids[pos];
            if (doc_id > std.math.maxInt(usize) / index.dim) return null;
            const base = doc_id * index.dim;
            if (base > corpus.len || index.dim > corpus.len - base) return null;

            const doc_vec = corpus[base .. base + index.dim];
            const dist_opt = if (use_cosine) cosineDistSlice(query, q_norm, doc_vec) else l2DistSqSlice(query, doc_vec);
            if (dist_opt == null) return null;
            const dist = dist_opt ?? 0.0;

            const hit = IvfHit{
                doc_id: doc_id,
                dist_sq: dist,
                cluster_id: cid,
            };

            if (heap_len < max_k) {
                out_hits[heap_len] = hit;
                heap_len += 1;
                if (heap_len == max_k) {
                    let h: usize = max_k / 2;
                    while (h > 0) {
                        h -= 1;
                        ivfSiftDownWorstRoot(out_hits, h, max_k);
                    }
                }
            } else if (ivfHitBetterAsc(hit, out_hits[0])) {
                out_hits[0] = hit;
                ivfSiftDownWorstRoot(out_hits, 0, max_k);
            }
        }
    }

    sortIvfHitsAsc(out_hits, heap_len);
    return heap_len;
}

/// Zero-heap-allocation CSR Inverted File (IVF-Flat) ANN index.
pub struct IvfFlatIndex {
    centroids: []const f64;
    list_offsets: []usize;
    list_doc_ids: []usize;
    n_clusters: usize = 0;
    dim: usize = 0;

    /// Assigns every vector in `corpus` to its nearest centroid (L2 squared) and populates
    /// caller-provided CSR arrays (`list_offsets_out`, `list_doc_ids_out`) using `counts_scratch`.
    /// Returns `null` on invalid dimensions, NaN/Inf, or insufficient buffer capacity.
    static fn buildFromCentroids(
        corpus: []const f64,
        n_docs: usize,
        dim: usize,
        centroids: []const f64,
        n_clusters: usize,
        list_offsets_out: *[]usize,
        list_doc_ids_out: *[]usize,
        counts_scratch: *[]usize,
    ): IvfBuildStats? {
        if (n_clusters == 0 || dim == 0) return null;
        if (n_clusters > std.math.maxInt(usize) / dim) return null;
        const cent_elems = n_clusters * dim;
        if (centroids.len < cent_elems) return null;
        if (n_docs > 0 && n_docs > std.math.maxInt(usize) / dim) return null;
        const corp_elems = n_docs * dim;
        if (corpus.len < corp_elems) return null;
        if (list_offsets_out.len < n_clusters + 1) return null;
        if (list_doc_ids_out.len < n_docs) return null;
        if (counts_scratch.len < n_clusters) return null;

        // Validate finiteness of centroids & corpus
        let ci: usize = 0;
        while (ci < cent_elems) : (ci += 1) {
            if (!isFiniteF64(centroids[ci])) return null;
        }
        let di: usize = 0;
        while (di < corp_elems) : (di += 1) {
            if (!isFiniteF64(corpus[di])) return null;
        }

        let k: usize = 0;
        while (k < n_clusters) : (k += 1) {
            counts_scratch[k] = 0;
        }

        // Pass 1: count docs per nearest centroid
        let doc: usize = 0;
        while (doc < n_docs) : (doc += 1) {
            const doc_vec = corpus[doc * dim .. (doc + 1) * dim];
            let best_c: usize = 0;
            let best_dist: f64 = std.math.inf(f64);
            let c: usize = 0;
            while (c < n_clusters) : (c += 1) {
                const c_vec = centroids[c * dim .. (c + 1) * dim];
                const d_opt = l2DistSqSlice(doc_vec, c_vec);
                if (d_opt == null) return null;
                const d_val = d_opt ?? 0.0;
                if (c == 0 || d_val < best_dist) {
                    best_dist = d_val;
                    best_c = c;
                }
            }
            counts_scratch[best_c] += 1;
        }

        // Prefix sum into list_offsets_out & reset counts_scratch to write cursors
        list_offsets_out[0] = 0;
        let min_sz: usize = counts_scratch[0];
        let max_sz: usize = counts_scratch[0];
        let c_sum: usize = 0;
        while (c_sum < n_clusters) : (c_sum += 1) {
            const cnt = counts_scratch[c_sum];
            if (cnt < min_sz) min_sz = cnt;
            if (cnt > max_sz) max_sz = cnt;
            list_offsets_out[c_sum + 1] = list_offsets_out[c_sum] + cnt;
            counts_scratch[c_sum] = list_offsets_out[c_sum];
        }

        // Pass 2: scatter doc_ids into CSR buckets
        let doc2: usize = 0;
        while (doc2 < n_docs) : (doc2 += 1) {
            const doc_vec = corpus[doc2 * dim .. (doc2 + 1) * dim];
            let best_c: usize = 0;
            let best_dist: f64 = std.math.inf(f64);
            let c: usize = 0;
            while (c < n_clusters) : (c += 1) {
                const c_vec = centroids[c * dim .. (c + 1) * dim];
                const d_val = l2DistSqSlice(doc_vec, c_vec) ?? 0.0;
                if (c == 0 || d_val < best_dist) {
                    best_dist = d_val;
                    best_c = c;
                }
            }
            const write_pos = counts_scratch[best_c];
            list_doc_ids_out[write_pos] = doc2;
            counts_scratch[best_c] = write_pos + 1;
        }

        return IvfBuildStats{
            n_clusters: n_clusters,
            n_docs: n_docs,
            min_cluster_size: min_sz,
            max_cluster_size: max_sz,
        };
    }

    /// Multi-probe L2 squared ANN search over `nprobe` closest centroids.
    /// Outputs Top-K hits sorted ascending by `(dist_sq, doc_id)`.
    static fn searchL2(
        index: IvfFlatIndex,
        corpus: []const f64,
        query: []const f64,
        nprobe: usize,
        top_k: usize,
        probe_ids_s: *[]usize,
        probe_dists_s: *[]f64,
        out_hits: *[]IvfHit,
    ): usize? {
        return ivfSearchInternal(index, corpus, query, nprobe, top_k, false, probe_ids_s, probe_dists_s, out_hits);
    }

    /// Multi-probe Cosine-distance (`1 - cos(q, x)`) ANN search over `nprobe` closest centroids.
    /// Outputs Top-K hits sorted ascending by `(dist_sq, doc_id)`.
    static fn searchCosine(
        index: IvfFlatIndex,
        corpus: []const f64,
        query: []const f64,
        nprobe: usize,
        top_k: usize,
        probe_ids_s: *[]usize,
        probe_dists_s: *[]f64,
        out_hits: *[]IvfHit,
    ): usize? {
        return ivfSearchInternal(index, corpus, query, nprobe, top_k, true, probe_ids_s, probe_dists_s, out_hits);
    }

    /// Returns the number of vectors assigned to `cluster_id` (`0` if out of bounds).
    static fn clusterSize(index: IvfFlatIndex, cluster_id: usize): usize {
        if (cluster_id >= index.n_clusters) return 0;
        if (cluster_id + 1 >= index.list_offsets.len) return 0;
        const s = index.list_offsets[cluster_id];
        const e = index.list_offsets[cluster_id + 1];
        if (e < s) return 0;
        return e - s;
    }
}

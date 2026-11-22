// Origin: Pure-CP Stdlib (RAG Product Quantization & Asymmetric Distance Computation ADC) | Created: 2026-11-22
// Zero-heap-allocation Product Quantization (PQ) codebook encoder/decoder and O(m) lookup-table
// Asymmetric Distance Computation (ADC) Top-K search for billion-scale IVF-PQ retrieval.

const std = @import("std");

/// Product Quantization codebook partitioning `dim` into `m` subspaces of `sub_dim = dim / m`,
/// each with `ks` (`1 <= ks <= 256`) centroids laid out flat as `[m][ks][sub_dim]`.
pub struct PqCodebook {
    dim: usize = 0;
    m: usize = 0;
    sub_dim: usize = 0;
    ks: usize = 0;
    centroids: []const f64;

    /// Validates codebook dimensions (`dim > 0`, `m > 0`, `dim % m == 0`, `1 <= ks <= 256`),
    /// backing slice length (`>= m * ks * sub_dim`), and finite centroid values.
    static fn init(dim: usize, m: usize, ks: usize, centroids: []const f64): PqCodebook? {
        if (dim == 0 || m == 0 || ks == 0 || ks > 256) return null;
        if (dim % m != 0) return null;
        const sub_dim = dim / m;
        if (m > std.math.maxInt(usize) / ks) return null;
        const m_ks = m * ks;
        if (m_ks > std.math.maxInt(usize) / sub_dim) return null;
        const total_elems = m_ks * sub_dim;
        if (centroids.len < total_elems) return null;

        let i: usize = 0;
        while (i < total_elems) : (i += 1) {
            if (!isFiniteF64(centroids[i])) return null;
        }

        return PqCodebook{
            dim: dim,
            m: m,
            sub_dim: sub_dim,
            ks: ks,
            centroids: centroids,
        };
    }
}

/// Top-K search hit returned by `pqSearchAdcTopK`.
pub struct PqHit {
    doc_id: usize = 0;
    dist_sq: f64 = 0.0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isValidCodebook(cb: PqCodebook): bool {
    if (cb.dim == 0 || cb.m == 0 || cb.ks == 0 || cb.ks > 256) return false;
    if (cb.dim % cb.m != 0 || cb.sub_dim != cb.dim / cb.m) return false;
    if (cb.m > std.math.maxInt(usize) / cb.ks) return false;
    const m_ks = cb.m * cb.ks;
    if (m_ks > std.math.maxInt(usize) / cb.sub_dim) return false;
    return cb.centroids.len >= m_ks * cb.sub_dim;
}

fn subL2DistSq(a: []const f64, b: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const d = a[i] - b[i];
        sum += d * d;
    }
    return sum;
}

/// Encodes a `dim`-dimensional vector into `m` `u8` subspace codeword indices written to `code_out`.
/// Returns `false` on invalid codebook, length mismatch, NaN/Inf input, or small `code_out`.
pub fn pqEncodeVector(cb: PqCodebook, vec: []const f64, code_out: *[]u8): bool {
    if (!isValidCodebook(cb)) return false;
    if (vec.len != cb.dim || code_out.len < cb.m) return false;

    let vi: usize = 0;
    while (vi < cb.dim) : (vi += 1) {
        if (!isFiniteF64(vec[vi])) return false;
    }

    let s: usize = 0;
    while (s < cb.m) : (s += 1) {
        const v_sub = vec[s * cb.sub_dim .. (s + 1) * cb.sub_dim];
        const sub_base = s * cb.ks * cb.sub_dim;

        let best_c: usize = 0;
        let best_dist: f64 = std.math.inf(f64);
        let c: usize = 0;
        while (c < cb.ks) : (c += 1) {
            const c_off = sub_base + c * cb.sub_dim;
            const c_sub = cb.centroids[c_off .. c_off + cb.sub_dim];
            const dist = subL2DistSq(v_sub, c_sub);
            if (c == 0 || dist < best_dist) {
                best_dist = dist;
                best_c = c;
            }
        }
        code_out[s] = best_c as u8;
    }
    return true;
}

/// Reconstructs an approximate `dim`-dimensional vector from `m` PQ codewords into `vec_out`.
/// Returns `false` if any codeword is `>= cb.ks` or buffers are too small.
pub fn pqDecodeVector(cb: PqCodebook, code: []const u8, vec_out: *[]f64): bool {
    if (!isValidCodebook(cb)) return false;
    if (code.len < cb.m || vec_out.len < cb.dim) return false;

    let s_chk: usize = 0;
    while (s_chk < cb.m) : (s_chk += 1) {
        if ((code[s_chk] as usize) >= cb.ks) return false;
    }

    let s: usize = 0;
    while (s < cb.m) : (s += 1) {
        const c: usize = code[s] as usize;
        const c_off = (s * cb.ks + c) * cb.sub_dim;
        const out_off = s * cb.sub_dim;
        let d: usize = 0;
        while (d < cb.sub_dim) : (d += 1) {
            vec_out[out_off + d] = cb.centroids[c_off + d];
        }
    }
    return true;
}

/// Precomputes the `m * ks` Asymmetric Distance Computation (ADC) squared-distance lookup table:
/// `lut_out[s * ks + c] = || query_sub[s] - centroid[s][c] ||^2`.
/// Returns `false` on invalid codebook, query mismatch, NaN/Inf, or small `lut_out`.
pub fn pqBuildAdcTable(cb: PqCodebook, query: []const f64, lut_out: *[]f64): bool {
    if (!isValidCodebook(cb)) return false;
    if (query.len != cb.dim || lut_out.len < cb.m * cb.ks) return false;

    let qi: usize = 0;
    while (qi < cb.dim) : (qi += 1) {
        if (!isFiniteF64(query[qi])) return false;
    }

    let s: usize = 0;
    while (s < cb.m) : (s += 1) {
        const q_sub = query[s * cb.sub_dim .. (s + 1) * cb.sub_dim];
        const sub_base = s * cb.ks * cb.sub_dim;
        const lut_base = s * cb.ks;

        let c: usize = 0;
        while (c < cb.ks) : (c += 1) {
            const c_off = sub_base + c * cb.sub_dim;
            const c_sub = cb.centroids[c_off .. c_off + cb.sub_dim];
            lut_out[lut_base + c] = subL2DistSq(q_sub, c_sub);
        }
    }
    return true;
}

/// Computes asymmetric squared L2 distance in O(m) table lookups and additions (zero FP multiplies).
/// Returns `+Inf` if parameters are invalid or any codeword is `>= ks`.
pub fn pqAdcDistanceSq(lut: []const f64, code: []const u8, m: usize, ks: usize): f64 {
    if (m == 0 || ks == 0 || code.len < m) return std.math.inf(f64);
    if (m > std.math.maxInt(usize) / ks || lut.len < m * ks) return std.math.inf(f64);

    let sum: f64 = 0.0;
    let s: usize = 0;
    while (s < m) : (s += 1) {
        const c: usize = code[s] as usize;
        if (c >= ks) return std.math.inf(f64);
        sum += lut[s * ks + c];
    }
    return sum;
}

fn pqHitBetterAsc(a: PqHit, b: PqHit): bool {
    if (a.dist_sq != b.dist_sq) return a.dist_sq < b.dist_sq;
    return a.doc_id < b.doc_id;
}

fn pqSiftDownWorstRoot(heap: *[]PqHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && pqHitBetterAsc(heap[worst], heap[right])) {
            worst = right;
        }
        if (!pqHitBetterAsc(heap[idx], heap[worst])) break;
        const tmp = heap[idx];
        heap[idx] = heap[worst];
        heap[worst] = tmp;
        idx = worst;
    }
}

fn sortPqHitsAsc(arr: *[]PqHit, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        pqSiftDownWorstRoot(arr, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        pqSiftDownWorstRoot(arr, 0, end);
    }
}

/// Zero-heap-allocation O(n * m) ADC Top-K search over a flat PQ-encoded corpus (`n_docs * m` bytes).
/// Builds the ADC lookup table into `lut_scratch` and outputs Top-K hits sorted ascending by
/// `(dist_sq, doc_id)` into `out_hits`. Returns `null` on invalid inputs, out-of-range codewords,
/// or insufficient scratch/output capacity.
pub fn pqSearchAdcTopK(
    cb: PqCodebook,
    codes_flat: []const u8,
    n_docs: usize,
    query: []const f64,
    top_k: usize,
    lut_scratch: *[]f64,
    out_hits: *[]PqHit,
): usize? {
    if (!isValidCodebook(cb)) return null;
    if (n_docs == 0 || top_k == 0) return 0;
    if (query.len != cb.dim) return null;
    if (n_docs > std.math.maxInt(usize) / cb.m) return null;
    if (codes_flat.len < n_docs * cb.m) return null;

    const k = if (top_k < n_docs) top_k else n_docs;
    if (out_hits.len < k) return null;

    if (!pqBuildAdcTable(cb, query, lut_scratch)) return null;
    const lut_slice = lut_scratch[0 .. cb.m * cb.ks];

    let heap_len: usize = 0;
    let doc: usize = 0;
    while (doc < n_docs) : (doc += 1) {
        const base = doc * cb.m;
        const code = codes_flat[base .. base + cb.m];

        let s_chk: usize = 0;
        while (s_chk < cb.m) : (s_chk += 1) {
            if ((code[s_chk] as usize) >= cb.ks) return null;
        }

        const dist = pqAdcDistanceSq(lut_slice, code, cb.m, cb.ks);
        const hit = PqHit{
            doc_id: doc,
            dist_sq: dist,
        };

        if (heap_len < k) {
            out_hits[heap_len] = hit;
            heap_len += 1;
            if (heap_len == k) {
                let h: usize = k / 2;
                while (h > 0) {
                    h -= 1;
                    pqSiftDownWorstRoot(out_hits, h, k);
                }
            }
        } else if (pqHitBetterAsc(hit, out_hits[0])) {
            out_hits[0] = hit;
            pqSiftDownWorstRoot(out_hits, 0, k);
        }
    }

    sortPqHitsAsc(out_hits, heap_len);
    return heap_len;
}

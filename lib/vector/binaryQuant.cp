// Origin: Pure-CP Stdlib (RAG 1-Bit Binary Vector Quantization, Hamming Search & Two-Stage Rescore) | Created: 2026-11-22
// Zero-heap-allocation 1-bit binary vector quantization (32x compression vs f32, 64x vs f64),
// hardware-accelerated popcount Hamming search, Charikar cosine estimation, and two-stage
// coarse-to-fine (1-bit Hamming Top-M -> Full-Precision Cosine Top-K) retrieval.

const std = @import("std");

/// Coarse 1-bit Hamming search candidate.
pub struct BinaryCandidate {
    index: usize = 0;
    hamming: u32 = 0;
}

/// Fine-stage rescored hit combining full-precision cosine similarity and coarse Hamming distance.
pub struct RescoredHit {
    index: usize = 0;
    cosine: f64 = 0.0;
    hamming: u32 = 0;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Returns the number of 64-bit words required to pack `dim` binary bits: `ceil(dim / 64)`.
pub fn binaryWordsForDim(dim: usize): usize {
    if (dim == 0) return 0;
    return ((dim - 1) / 64) + 1;
}

/// 1-bit sign binary quantization: sets bit `i` to `1` iff `vec[i] >= threshold`.
/// Packs bits LSB-first into `u64` words in `words_out`.
/// Returns `false` if `vec` is empty, `threshold` or any component is NaN/Inf, or `words_out` is too small.
pub fn quantizeBinary(vec: []const f64, threshold: f64, words_out: *[]u64): bool {
    if (vec.len == 0) return false;
    if (!isFiniteF64(threshold)) return false;

    const needed = binaryWordsForDim(vec.len);
    if (words_out.len < needed) return false;

    let i_check: usize = 0;
    while (i_check < vec.len) : (i_check += 1) {
        if (!isFiniteF64(vec[i_check])) return false;
    }

    let w: usize = 0;
    while (w < needed) : (w += 1) {
        words_out[w] = 0;
    }

    let i: usize = 0;
    let word_idx: usize = 0;
    let bit_mask: u64 = 1;
    while (i < vec.len) : (i += 1) {
        if (vec[i] >= threshold) {
            words_out[word_idx] |= bit_mask;
        }
        if ((i & 63) == 63) {
            word_idx += 1;
            bit_mask = 1;
        } else {
            bit_mask = bit_mask << 1;
        }
    }
    return true;
}

/// Computes the exact Hamming distance between two bit-packed `u64` slices using `@popCount`.
pub fn hammingDistanceWords(a: []const u64, b: []const u64): u32 {
    const n = if (a.len < b.len) a.len else b.len;
    let dist: u32 = 0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const diff: u64 = a[i] ^ b[i];
        const bits: u32 = @popCount(diff);
        dist += bits;
    }
    return dist;
}

/// Charikar unbiased cosine similarity estimator from 1-bit Hamming distance:
/// `cos(pi * hamming / dim)`. Returns `0.0` when `dim == 0`.
pub fn binaryCosineApprox(hamming: u32, dim: usize): f64 {
    if (dim == 0) return 0.0;
    const h_usize: usize = hamming as usize;
    const clamped: usize = if (h_usize > dim) dim else h_usize;
    const angle = std.math.pi * ((clamped as f64) / (dim as f64));
    return std.math.cos(angle);
}

fn candidateBetterAsc(a: BinaryCandidate, b: BinaryCandidate): bool {
    if (a.hamming != b.hamming) return a.hamming < b.hamming;
    return a.index < b.index;
}

fn candidateSiftDownWorstRoot(heap: *[]BinaryCandidate, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && candidateBetterAsc(heap[worst], heap[right])) {
            worst = right;
        }
        if (!candidateBetterAsc(heap[idx], heap[worst])) break;
        const tmp = heap[idx];
        heap[idx] = heap[worst];
        heap[worst] = tmp;
        idx = worst;
    }
}

fn sortCandidatesAsc(arr: *[]BinaryCandidate, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        candidateSiftDownWorstRoot(arr, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        candidateSiftDownWorstRoot(arr, 0, end);
    }
}

/// Zero-heap-allocation Top-K 1-bit Hamming distance search over a packed binary corpus.
/// Outputs up to `min(top_k, n_docs)` candidates sorted ascending by `hamming` with
/// deterministic tie-breaking by ascending `index`.
pub fn searchBinaryTopK(
    query_words: []const u64,
    corpus_words: []const u64,
    n_docs: usize,
    words_per_vec: usize,
    top_k: usize,
    out_candidates: *[]BinaryCandidate,
): usize? {
    if (n_docs == 0 || top_k == 0) return 0;
    if (words_per_vec == 0) return null;
    if (query_words.len < words_per_vec) return null;
    if (n_docs > std.math.maxInt(usize) / words_per_vec) return null;
    if (corpus_words.len < n_docs * words_per_vec) return null;

    const k = if (top_k < n_docs) top_k else n_docs;
    if (out_candidates.len < k) return null;

    const q_slice = query_words[0..words_per_vec];
    let heap_len: usize = 0;

    let doc: usize = 0;
    while (doc < n_docs) : (doc += 1) {
        const base = doc * words_per_vec;
        const doc_slice = corpus_words[base .. base + words_per_vec];
        const dist = hammingDistanceWords(q_slice, doc_slice);
        const cand = BinaryCandidate{
            index: doc,
            hamming: dist,
        };

        if (heap_len < k) {
            out_candidates[heap_len] = cand;
            heap_len += 1;
            if (heap_len == k) {
                // Build max-heap (worst candidate at root)
                let h: usize = k / 2;
                while (h > 0) {
                    h -= 1;
                    candidateSiftDownWorstRoot(out_candidates, h, k);
                }
            }
        } else if (candidateBetterAsc(cand, out_candidates[0])) {
            out_candidates[0] = cand;
            candidateSiftDownWorstRoot(out_candidates, 0, k);
        }
    }

    sortCandidatesAsc(out_candidates, heap_len);
    return heap_len;
}

fn rescoredBetterDesc(a: RescoredHit, b: RescoredHit): bool {
    if (a.cosine != b.cosine) return a.cosine > b.cosine;
    if (a.hamming != b.hamming) return a.hamming < b.hamming;
    return a.index < b.index;
}

fn rescoredSiftDownWorstRoot(heap: *[]RescoredHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && rescoredBetterDesc(heap[worst], heap[right])) {
            worst = right;
        }
        if (!rescoredBetterDesc(heap[idx], heap[worst])) break;
        const tmp = heap[idx];
        heap[idx] = heap[worst];
        heap[worst] = tmp;
        idx = worst;
    }
}

fn sortRescoredDesc(arr: *[]RescoredHit, count: usize) {
    if (count <= 1) return;
    let i: usize = count / 2;
    while (i > 0) {
        i -= 1;
        rescoredSiftDownWorstRoot(arr, i, count);
    }
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        rescoredSiftDownWorstRoot(arr, 0, end);
    }
}

/// Zero-heap-allocation two-stage coarse-to-fine vector retrieval:
/// - Stage 1 (Coarse): 1-bit Hamming Top-M candidate recall into `coarse_scratch` (`M = clamp(oversample_m, top_k, n_docs)`).
/// - Stage 2 (Fine): Full-precision cosine similarity rescore over Top-M candidates, outputting Top-K into `out_hits` descending.
pub fn twoStageSearch(
    query: []const f64,
    query_words: []const u64,
    corpus_words: []const u64,
    corpus_fp: []const f64,
    n_docs: usize,
    dim: usize,
    oversample_m: usize,
    top_k: usize,
    coarse_scratch: *[]BinaryCandidate,
    out_hits: *[]RescoredHit,
): usize? {
    if (n_docs == 0 || top_k == 0) return 0;
    if (dim == 0) return null;
    if (query.len != dim) return null;

    const words_per_vec = binaryWordsForDim(dim);
    if (query_words.len < words_per_vec) return null;
    if (n_docs > std.math.maxInt(usize) / words_per_vec) return null;
    if (corpus_words.len < n_docs * words_per_vec) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;
    if (corpus_fp.len < n_docs * dim) return null;

    let q_norm_sq: f64 = 0.0;
    let qi: usize = 0;
    while (qi < dim) : (qi += 1) {
        const v = query[qi];
        if (!isFiniteF64(v)) return null;
        q_norm_sq += v * v;
    }
    const q_norm = if (q_norm_sq > 0.0) std.math.sqrt(q_norm_sq) else 0.0;

    const k = if (top_k < n_docs) top_k else n_docs;
    const m_req = if (oversample_m < k) k else oversample_m;
    const m = if (m_req < n_docs) m_req else n_docs;
    if (coarse_scratch.len < m) return null;
    if (out_hits.len < k) return null;

    const coarse_opt = searchBinaryTopK(query_words, corpus_words, n_docs, words_per_vec, m, coarse_scratch);
    if (coarse_opt == null) return null;
    const coarse_count = coarse_opt ?? 0;

    let hit_len: usize = 0;
    let c: usize = 0;
    while (c < coarse_count) : (c += 1) {
        const cand = coarse_scratch[c];
        const base = cand.index * dim;

        let dot: f64 = 0.0;
        let d_norm_sq: f64 = 0.0;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const dv = corpus_fp[base + d];
            if (!isFiniteF64(dv)) return null;
            dot += query[d] * dv;
            d_norm_sq += dv * dv;
        }

        let cos_sim: f64 = 0.0;
        if (q_norm > 0.0 && d_norm_sq > 0.0) {
            const denom = q_norm * std.math.sqrt(d_norm_sq);
            if (denom > 0.0 && isFiniteF64(denom)) {
                cos_sim = dot / denom;
                if (cos_sim > 1.0) cos_sim = 1.0;
                if (cos_sim < -1.0) cos_sim = -1.0;
            }
        }

        const hit = RescoredHit{
            index: cand.index,
            cosine: cos_sim,
            hamming: cand.hamming,
        };

        if (hit_len < k) {
            out_hits[hit_len] = hit;
            hit_len += 1;
            if (hit_len == k) {
                let h: usize = k / 2;
                while (h > 0) {
                    h -= 1;
                    rescoredSiftDownWorstRoot(out_hits, h, k);
                }
            }
        } else if (rescoredBetterDesc(hit, out_hits[0])) {
            out_hits[0] = hit;
            rescoredSiftDownWorstRoot(out_hits, 0, k);
        }
    }

    sortRescoredDesc(out_hits, hit_len);
    return hit_len;
}

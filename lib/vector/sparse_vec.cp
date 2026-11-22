// Origin: Pure-CP Stdlib (AI Learned Sparse Retrieval: SPLADE/BGE-M3 Sparse Vector & Merge-Join Scorer) | Created: 2026-11-22
// Pure CP zero-heap-allocation learned sparse lexical vector primitives for SPLADE / BGE-M3:
// in-place canonicalization, O(m+n) two-pointer merge-join dot product with hardware @fma,
// sparse cosine similarity, Top-K magnitude pruning, and CSR corpus batch Top-K retrieval.

const std = @import("std");

/// Single `(term_id, weight)` entry in a sparse lexical vector.
pub struct SparseEntry {
    term_id: u32 = 0;
    weight: f32 = 0.0;
}

/// Ranked document hit returned by `sparseBatchTopK`.
pub struct SparseHit {
    doc_id: u32 = 0;
    score: f32 = 0.0;
}

// ============================================================================
// Internal Numeric, Ordering & In-Place Heapsort Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

/// Sifts down `arr[start..end]` to maintain a max-heap by `term_id` (used for ascending heapsort).
fn siftDownTermMax(arr: *[]SparseEntry, start: usize, end: usize) {
    let root: usize = start;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= end) break;
        let largest: usize = root;
        if (arr[left].term_id > arr[largest].term_id) {
            largest = left;
        }
        const right: usize = left + 1;
        if (right < end && arr[right].term_id > arr[largest].term_id) {
            largest = right;
        }
        if (largest == root) break;
        const tmp = arr[root];
        arr[root] = arr[largest];
        arr[largest] = tmp;
        root = largest;
    }
}

/// In-place O(n log n), O(1)-space heapsort of `arr[0..n]` ascending by `term_id`.
fn sortEntriesByTermIdAsc(arr: *[]SparseEntry, n: usize) {
    if (n <= 1) return;
    let start: usize = n / 2;
    while (start > 0) {
        start -= 1;
        siftDownTermMax(arr, start, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        siftDownTermMax(arr, 0, end);
    }
}

/// Returns `true` if `a` is strictly preferred over `b` for Top-K pruning:
/// larger `|weight|`, tie-broken by smaller `term_id`.
fn entryBetterAbsWeight(a: SparseEntry, b: SparseEntry): bool {
    const aa: f32 = absF32(a.weight);
    const ba: f32 = absF32(b.weight);
    if (aa > ba) return true;
    if (aa < ba) return false;
    return a.term_id < b.term_id;
}

/// Sifts up in a min-heap (root = worst kept entry) for Top-K pruning.
fn pruneHeapSiftUp(heap: *[]SparseEntry, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (entryBetterAbsWeight(heap[parent], heap[idx])) {
            const tmp = heap[parent];
            heap[parent] = heap[idx];
            heap[idx] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

/// Sifts down in a min-heap (root = worst kept entry) for Top-K pruning.
fn pruneHeapSiftDown(heap: *[]SparseEntry, count: usize) {
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (entryBetterAbsWeight(heap[worst], heap[left])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && entryBetterAbsWeight(heap[worst], heap[right])) {
            worst = right;
        }
        if (worst == root) break;
        const tmp = heap[root];
        heap[root] = heap[worst];
        heap[worst] = tmp;
        root = worst;
    }
}

/// Returns `true` if hit `a` ranks strictly ahead of hit `b`:
/// higher `score`, tie-broken by smaller `doc_id`.
fn hitBetter(a: SparseHit, b: SparseHit): bool {
    if (a.score > b.score) return true;
    if (a.score < b.score) return false;
    return a.doc_id < b.doc_id;
}

/// Sifts up in a min-heap (root = worst kept hit) for Top-K document scoring.
fn hitHeapSiftUp(heap: *[]SparseHit, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (hitBetter(heap[parent], heap[idx])) {
            const tmp = heap[parent];
            heap[parent] = heap[idx];
            heap[idx] = tmp;
            idx = parent;
        } else {
            break;
        }
    }
}

/// Sifts down in a min-heap (root = worst kept hit) for Top-K document scoring.
fn hitHeapSiftDown(heap: *[]SparseHit, count: usize) {
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (hitBetter(heap[worst], heap[left])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && hitBetter(heap[worst], heap[right])) {
            worst = right;
        }
        if (worst == root) break;
        const tmp = heap[root];
        heap[root] = heap[worst];
        heap[worst] = tmp;
        root = worst;
    }
}

// ============================================================================
// Public Learned Sparse Vector API (SPLADE / BGE-M3)
// ============================================================================

/// Canonicalizes a sparse vector in-place:
/// 1. Filters out non-finite (`NaN` / `Inf`) entries.
/// 2. Sorts entries ascending by `term_id` in O(n log n) time, O(1) space.
/// 3. Merges duplicate `term_id` entries by summing their weights.
/// 4. Drops any resulting entry whose merged weight is `<= 0.0` or non-finite.
///
/// Returns the normalized valid length (`<= entries.len`).
pub fn sparseNormalizeInPlace(entries: *[]SparseEntry): usize {
    if (entries.len == 0) return 0;

    // Step 1: Compact finite entries to the front so NaN/Inf cannot poison duplicate merges.
    let valid_n: usize = 0;
    let i: usize = 0;
    while (i < entries.len) : (i += 1) {
        if (isFiniteF32(entries[i].weight)) {
            entries[valid_n] = entries[i];
            valid_n += 1;
        }
    }
    if (valid_n == 0) return 0;

    // Step 2: In-place ascending sort by `term_id`.
    sortEntriesByTermIdAsc(entries, valid_n);

    // Step 3 & 4: Merge duplicate `term_id` runs and drop non-positive / non-finite sums.
    let out_len: usize = 0;
    let r: usize = 0;
    while (r < valid_n) {
        const tid: u32 = entries[r].term_id;
        let sum: f32 = entries[r].weight;
        r += 1;
        while (r < valid_n && entries[r].term_id == tid) : (r += 1) {
            sum += entries[r].weight;
        }
        if (isFiniteF32(sum) && sum > 0.0) {
            entries[out_len] = SparseEntry{ term_id: tid, weight: sum };
            out_len += 1;
        }
    }

    return out_len;
}

/// Computes the exact O(|a| + |b|) two-pointer merge-join inner product of two sparse vectors
/// sorted strictly ascending by `term_id`, accumulating via hardware `@fma`.
/// Safely skips non-finite weights and returns `0.0` on empty inputs.
pub fn sparseDot(a: []const SparseEntry, b: []const SparseEntry): f32 {
    if (a.len == 0 || b.len == 0) return 0.0;

    let i: usize = 0;
    let j: usize = 0;
    let acc: f32 = 0.0;
    while (i < a.len && j < b.len) {
        const ta: u32 = a[i].term_id;
        const tb: u32 = b[j].term_id;
        if (ta == tb) {
            const wa: f32 = a[i].weight;
            const wb: f32 = b[j].weight;
            if (isFiniteF32(wa) && isFiniteF32(wb)) {
                acc = @fma(wa, wb, acc);
            }
            i += 1;
            j += 1;
        } else if (ta < tb) {
            i += 1;
        } else {
            j += 1;
        }
    }

    if (!isFiniteF32(acc)) return 0.0;
    return acc;
}

/// Computes the sparse cosine similarity of two strictly ascending sparse vectors,
/// clamped to `[-1.0, 1.0]`. Safely returns `0.0` for empty or zero-norm vectors.
pub fn sparseCosine(a: []const SparseEntry, b: []const SparseEntry): f32 {
    if (a.len == 0 || b.len == 0) return 0.0;

    let norm_a_sq: f32 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        const w: f32 = a[i].weight;
        if (isFiniteF32(w)) {
            norm_a_sq = @fma(w, w, norm_a_sq);
        }
    }

    let norm_b_sq: f32 = 0.0;
    let j: usize = 0;
    while (j < b.len) : (j += 1) {
        const w: f32 = b[j].weight;
        if (isFiniteF32(w)) {
            norm_b_sq = @fma(w, w, norm_b_sq);
        }
    }

    if (norm_a_sq <= 0.0 || norm_b_sq <= 0.0 || !isFiniteF32(norm_a_sq) || !isFiniteF32(norm_b_sq)) {
        return 0.0;
    }

    const denom: f32 = @sqrt(norm_a_sq) * @sqrt(norm_b_sq);
    if (denom <= 0.0 || !isFiniteF32(denom)) {
        return 0.0;
    }

    const dot: f32 = sparseDot(a, b);
    return clampCosineF32(dot / denom);
}

/// SPLADE sparsity control: retains up to `min(k, out_dst.len)` finite non-zero entries
/// with the largest `|weight|` (tie-broken by ascending `term_id`), then sorts the kept
/// entries ascending by `term_id` in `out_dst` for downstream merge-join scoring.
///
/// Returns the number of retained entries written to `out_dst`.
pub fn sparsePruneTopK(
    src: []const SparseEntry,
    k: usize,
    out_dst: *[]SparseEntry,
): usize {
    const limit: usize = if (k < out_dst.len) k else out_dst.len;
    if (limit == 0 || src.len == 0) return 0;

    let count: usize = 0;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const e = src[i];
        if (!isFiniteF32(e.weight) || e.weight == 0.0) {
            continue;
        }
        if (count < limit) {
            out_dst[count] = e;
            pruneHeapSiftUp(out_dst, count);
            count += 1;
        } else if (entryBetterAbsWeight(e, out_dst[0])) {
            out_dst[0] = e;
            pruneHeapSiftDown(out_dst, count);
        }
    }

    sortEntriesByTermIdAsc(out_dst, count);
    return count;
}

/// Zero-heap-allocation batch Top-K sparse retrieval over a CSR-packed corpus.
///
/// - `doc_offsets` has length `n_docs + 1`, where document `d` spans
///   `corpus_entries[doc_offsets[d] .. doc_offsets[d + 1]]`.
/// - Only documents with positive lexical overlap (`score > 0.0`) are returned,
///   sorted descending by `score` (tie-broken by ascending `doc_id`).
/// - Returns `null` on empty/non-monotonic/out-of-bounds `doc_offsets` or `out_hits.len < k`.
pub fn sparseBatchTopK(
    query: []const SparseEntry,
    corpus_entries: []const SparseEntry,
    doc_offsets: []const usize,
    k: usize,
    out_hits: *[]SparseHit,
): usize? {
    if (doc_offsets.len == 0) return null;
    if (out_hits.len < k) return null;

    // Validate monotonic CSR offsets within `corpus_entries.len`.
    if (doc_offsets[0] > corpus_entries.len) return null;
    const n_docs: usize = doc_offsets.len - 1;
    let v: usize = 0;
    while (v < n_docs) : (v += 1) {
        if (doc_offsets[v] > doc_offsets[v + 1] || doc_offsets[v + 1] > corpus_entries.len) {
            return null;
        }
    }

    if (n_docs == 0 || k == 0 || query.len == 0) {
        return 0;
    }

    // Score each document via O(|q| + |d|) merge-join dot product and maintain a Top-K min-heap.
    let count: usize = 0;
    let d: usize = 0;
    while (d < n_docs) : (d += 1) {
        const start: usize = doc_offsets[d];
        const end: usize = doc_offsets[d + 1];
        if (start == end) continue;

        const score: f32 = sparseDot(query, corpus_entries[start..end]);
        if (!isFiniteF32(score) || score <= 0.0) continue;

        const cand = SparseHit{ doc_id: d as u32, score: score };
        if (count < k) {
            out_hits[count] = cand;
            hitHeapSiftUp(out_hits, count);
            count += 1;
        } else if (hitBetter(cand, out_hits[0])) {
            out_hits[0] = cand;
            hitHeapSiftDown(out_hits, count);
        }
    }

    // Extract min-heap in reverse into descending `(score, -doc_id)` order.
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp = out_hits[0];
        out_hits[0] = out_hits[end];
        out_hits[end] = tmp;
        hitHeapSiftDown(out_hits, end);
    }

    return count;
}

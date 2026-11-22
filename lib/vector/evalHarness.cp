// Origin: Pure-CP Stdlib (AI Vector ANN Evaluation Harness: SIMD Ground-Truth, Recall@K & Latency Percentiles) | Created: 2026-11-22
// Pure CP zero-heap-allocation offline evaluation and latency percentile harness for ANN vector
// indices: in-place heapsort P50/P95/P99 latency statistics, native 8-lane SIMD `@l2sq` brute-force
// ground-truth Top-K generator, deduplicated Recall@K, and batch ANN evaluation summary.

const std = @import("std");

const EVAL_FAST_MAX_K: usize = 256;

/// Summary of latency distribution in microseconds computed by `computeLatencyPercentilesInPlace`.
pub struct LatencyPercentiles {
    count: usize = 0;
    min_us: u64 = 0;
    p50_us: u64 = 0;
    p95_us: u64 = 0;
    p99_us: u64 = 0;
    max_us: u64 = 0;
    mean_us: f64 = 0.0;
}

/// Aggregated offline evaluation metrics across a batch of ANN queries.
pub struct AnnEvalSummary {
    queries: usize = 0;
    k: usize = 0;
    mean_recall_at_k: f64 = 0.0;
    min_recall_at_k: f64 = 0.0;
    exact_top1_rate: f64 = 0.0;
}

// ============================================================================
// Internal Validation, Heapsort & Top-K Heap Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) {
            return false;
        }
    }
    return true;
}

/// Sifts down `arr[start..end]` for a max-heap of `u64` (used for ascending in-place heapsort).
fn siftDownU64Max(arr: *[]u64, start: usize, end: usize) {
    let root: usize = start;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= end) break;
        let largest: usize = root;
        if (arr[left] > arr[largest]) {
            largest = left;
        }
        const right: usize = left + 1;
        if (right < end && arr[right] > arr[largest]) {
            largest = right;
        }
        if (largest == root) break;
        const tmp: u64 = arr[root];
        arr[root] = arr[largest];
        arr[largest] = tmp;
        root = largest;
    }
}

/// In-place O(n log n), O(1)-space ascending heapsort for `u64` slices.
fn sortU64AscInPlace(arr: *[]u64) {
    const n: usize = arr.len;
    if (n <= 1) return;
    let start: usize = n / 2;
    while (start > 0) {
        start -= 1;
        siftDownU64Max(arr, start, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp: u64 = arr[0];
        arr[0] = arr[end];
        arr[end] = tmp;
        siftDownU64Max(arr, 0, end);
    }
}

/// Nearest-rank percentile index for `p` in `[1, 100]` over `n >= 1` sorted elements.
fn nearestRankIndex(n: usize, p: usize): usize {
    const rank: usize = (p * n + 99) / 100;
    if (rank == 0) return 0;
    if (rank >= n) return n - 1;
    return rank - 1;
}

/// Returns `true` if `(a_dist, a_id)` is strictly worse (farther) than `(b_dist, b_id)`.
fn l2Worse(a_dist: f32, a_id: u32, b_dist: f32, b_id: u32): bool {
    if (a_dist > b_dist) return true;
    if (a_dist < b_dist) return false;
    return a_id > b_id;
}

/// Max-heap sift-up (root = farthest kept candidate) over parallel `(ids, dists)` buffers.
fn gtHeapSiftUp(ids: *[]u32, dists: *[]f32, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (l2Worse(dists[idx], ids[idx], dists[parent], ids[parent])) {
            const tmp_id: u32 = ids[parent];
            const tmp_d: f32 = dists[parent];
            ids[parent] = ids[idx];
            dists[parent] = dists[idx];
            ids[idx] = tmp_id;
            dists[idx] = tmp_d;
            idx = parent;
        } else {
            break;
        }
    }
}

/// Max-heap sift-down (root = farthest kept candidate) over parallel `(ids, dists)` buffers.
fn gtHeapSiftDown(ids: *[]u32, dists: *[]f32, count: usize) {
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (l2Worse(dists[left], ids[left], dists[worst], ids[worst])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && l2Worse(dists[right], ids[right], dists[worst], ids[worst])) {
            worst = right;
        }
        if (worst == root) break;
        const tmp_id: u32 = ids[root];
        const tmp_d: f32 = dists[root];
        ids[root] = ids[worst];
        dists[root] = dists[worst];
        ids[worst] = tmp_id;
        dists[worst] = tmp_d;
        root = worst;
    }
}

// ============================================================================
// Public Evaluation Harness & Latency Percentile API
// ============================================================================

/// Sorts `samples_us` in-place via O(n log n) zero-alloc heapsort and computes
/// `min`, `p50`, `p95`, `p99`, `max` (nearest-rank), and `mean_us`.
/// Returns `null` if `samples_us` is empty.
pub fn computeLatencyPercentilesInPlace(samples_us: *[]u64): LatencyPercentiles? {
    const n: usize = samples_us.len;
    if (n == 0) return null;

    sortU64AscInPlace(samples_us);

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        sum += samples_us[i] as f64;
    }

    return LatencyPercentiles{
        count: n,
        min_us: samples_us[0],
        p50_us: samples_us[nearestRankIndex(n, 50)],
        p95_us: samples_us[nearestRankIndex(n, 95)],
        p99_us: samples_us[nearestRankIndex(n, 99)],
        max_us: samples_us[n - 1],
        mean_us: sum / (n as f64),
    };
}

/// Computes exact brute-force Top-K nearest neighbor ground-truth IDs for `query` over a flat
/// row-major `[n_docs, dim]` `f32` corpus using CP's native 8-lane SIMD `@l2sq` intrinsic.
///
/// Writes up to `min(k, n_docs)` IDs sorted ascending by `(dist_sq, doc_id)` into `out_gt_ids`.
/// Returns `null` on invalid dimension, non-finite query/distance, undersized buffers, or overflow.
pub fn computeGroundTruthL2TopK(
    query: []const f32,
    corpus_flat: []const f32,
    n_docs: usize,
    dim: usize,
    k: usize,
    out_gt_ids: *[]u32,
): usize? {
    if (dim == 0 || query.len != dim) return null;
    if (out_gt_ids.len < k) return null;
    if (!isFiniteSliceF32(query)) return null;
    if (n_docs == 0 || k == 0) return 0;
    if (n_docs > std.math.maxInt(usize) / dim) return null;
    if (corpus_flat.len < n_docs * dim) return null;
    if (n_docs > (std.math.maxInt(u32) as usize)) return null;

    const eff_k: usize = if (k < n_docs) k else n_docs;

    // Fast path (k <= 256): O(n log k) max-heap with zero heap allocation.
    if (eff_k <= EVAL_FAST_MAX_K) {
        let dists_buf: [EVAL_FAST_MAX_K]f32 = [0.0; EVAL_FAST_MAX_K];
        let dists_s: []f32 = dists_buf[0..eff_k];
        let count: usize = 0;

        let d: usize = 0;
        while (d < n_docs) : (d += 1) {
            const vec: []const f32 = corpus_flat[d * dim .. (d + 1) * dim];
            let dist_sq: f32 = @l2sq(query, vec);
            if (!isFiniteF32(dist_sq)) return null;
            if (dist_sq < 0.0) dist_sq = 0.0;

            const doc_id: u32 = d as u32;
            if (count < eff_k) {
                out_gt_ids[count] = doc_id;
                dists_s[count] = dist_sq;
                gtHeapSiftUp(out_gt_ids, dists_s, count);
                count += 1;
            } else if (l2Worse(dists_s[0], out_gt_ids[0], dist_sq, doc_id)) {
                out_gt_ids[0] = doc_id;
                dists_s[0] = dist_sq;
                gtHeapSiftDown(out_gt_ids, dists_s, count);
            }
        }

        // Extract max-heap in reverse into strictly ascending `(dist_sq, doc_id)` order.
        let end: usize = count;
        while (end > 1) {
            end -= 1;
            const tmp_id: u32 = out_gt_ids[0];
            const tmp_d: f32 = dists_s[0];
            out_gt_ids[0] = out_gt_ids[end];
            dists_s[0] = dists_s[end];
            out_gt_ids[end] = tmp_id;
            dists_s[end] = tmp_d;
            gtHeapSiftDown(out_gt_ids, dists_s, end);
        }

        return count;
    }

    // Fallback for k > 256: zero-alloc sorted insertion using `@l2sq`.
    let count: usize = 0;
    let d: usize = 0;
    while (d < n_docs) : (d += 1) {
        const vec: []const f32 = corpus_flat[d * dim .. (d + 1) * dim];
        let dist_sq: f32 = @l2sq(query, vec);
        if (!isFiniteF32(dist_sq)) return null;
        if (dist_sq < 0.0) dist_sq = 0.0;
        const doc_id: u32 = d as u32;

        if (count == eff_k) {
            const last_id: u32 = out_gt_ids[count - 1];
            const last_vec: []const f32 = corpus_flat[(last_id as usize) * dim .. ((last_id as usize) + 1) * dim];
            let last_dist: f32 = @l2sq(query, last_vec);
            if (last_dist < 0.0) last_dist = 0.0;
            if (!l2Worse(last_dist, last_id, dist_sq, doc_id)) {
                continue;
            }
        }

        let pos: usize = 0;
        while (pos < count) : (pos += 1) {
            const cur_id: u32 = out_gt_ids[pos];
            const cur_vec: []const f32 = corpus_flat[(cur_id as usize) * dim .. ((cur_id as usize) + 1) * dim];
            let cur_dist: f32 = @l2sq(query, cur_vec);
            if (cur_dist < 0.0) cur_dist = 0.0;
            if (l2Worse(cur_dist, cur_id, dist_sq, doc_id)) {
                break;
            }
        }

        const new_count: usize = if (count < eff_k) count + 1 else eff_k;
        let shift: usize = new_count - 1;
        while (shift > pos) : (shift -= 1) {
            out_gt_ids[shift] = out_gt_ids[shift - 1];
        }
        out_gt_ids[pos] = doc_id;
        count = new_count;
    }

    return count;
}

/// Computes exact Recall@K between `predicted_ids` and `ground_truth_ids`, automatically
/// deduplicating both predicted and ground-truth IDs within the top-`k` window so Recall@K
/// is strictly bounded in `[0.0, 1.0]`.
/// Returns `null` if `k == 0` or `ground_truth_ids` is empty.
pub fn annRecallAtK(
    predicted_ids: []const u32,
    ground_truth_ids: []const u32,
    k: usize,
): f64? {
    if (k == 0 || ground_truth_ids.len == 0) return null;

    const gt_len: usize = if (k < ground_truth_ids.len) k else ground_truth_ids.len;
    const pred_len: usize = if (k < predicted_ids.len) k else predicted_ids.len;

    // Count unique ground-truth IDs in `ground_truth_ids[0..gt_len]`.
    let unique_gt: usize = 0;
    let g: usize = 0;
    while (g < gt_len) : (g += 1) {
        let dup: bool = false;
        let prev: usize = 0;
        while (prev < g) : (prev += 1) {
            if (ground_truth_ids[prev] == ground_truth_ids[g]) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            unique_gt += 1;
        }
    }
    if (unique_gt == 0) return null;

    // Count deduplicated predicted IDs that match `ground_truth_ids[0..gt_len]`.
    let hits: usize = 0;
    let p: usize = 0;
    while (p < pred_len) : (p += 1) {
        const pid: u32 = predicted_ids[p];
        let seen: bool = false;
        let prev: usize = 0;
        while (prev < p) : (prev += 1) {
            if (predicted_ids[prev] == pid) {
                seen = true;
                break;
            }
        }
        if (seen) continue;

        let j: usize = 0;
        while (j < gt_len) : (j += 1) {
            if (ground_truth_ids[j] == pid) {
                hits += 1;
                break;
            }
        }
    }

    return (hits as f64) / (unique_gt as f64);
}

/// Evaluates a batch of `num_queries` row-major `[num_queries, k]` predicted and ground-truth ID
/// matrices, aggregating `mean_recall_at_k`, `min_recall_at_k`, and `exact_top1_rate`.
/// Returns `null` on zero queries/k, overflow, or undersized slices.
pub fn evaluateAnnBatch(
    pred_flat: []const u32,
    gt_flat: []const u32,
    num_queries: usize,
    k: usize,
): AnnEvalSummary? {
    if (num_queries == 0 || k == 0) return null;
    if (num_queries > std.math.maxInt(usize) / k) return null;
    const total: usize = num_queries * k;
    if (pred_flat.len < total || gt_flat.len < total) return null;

    let sum_recall: f64 = 0.0;
    let min_recall: f64 = 1.0;
    let top1_hits: usize = 0;

    let pred_rem: []const u32 = pred_flat[0..total];
    let gt_rem: []const u32 = gt_flat[0..total];

    let q: usize = 0;
    while (q < num_queries) : (q += 1) {
        const [pred_q, next_pred] = pred_rem.split(k);
        const [gt_q, next_gt] = gt_rem.split(k);
        pred_rem = next_pred;
        gt_rem = next_gt;

        if (r := annRecallAtK(pred_q, gt_q, k)) {
            sum_recall += r;
            if (r < min_recall) {
                min_recall = r;
            }
        } else {
            return null;
        }

        if (pred_q[0] == gt_q[0]) {
            top1_hits += 1;
        }
    }

    return AnnEvalSummary{
        queries: num_queries,
        k: k,
        mean_recall_at_k: sum_recall / (num_queries as f64),
        min_recall_at_k: min_recall,
        exact_top1_rate: (top1_hits as f64) / (num_queries as f64),
    };
}

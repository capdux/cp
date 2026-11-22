// Origin: Pure-CP Stdlib (AI Agent Eval: Pass@k, Token-F1, Tool Trajectory & Latency Percentiles) | Created: 2026-11-22

const FNV1A_OFFSET_BASIS: u64 = 0xCBF29CE484222325;
const FNV1A_PRIME: u64 = 0x00000100000001B3;

fn isNaN(x: f64): bool {
    return x != x;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn isTokenByte(b: u8): bool {
    if (b >= 0x80) {
        return true;
    }
    if (b >= 48 && b <= 57) {
        return true; // '0'..'9'
    }
    if (b >= 65 && b <= 90) {
        return true; // 'A'..'Z'
    }
    if (b >= 97 && b <= 122) {
        return true; // 'a'..'z'
    }
    return false;
}

fn toLowerAscii(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn siftDownU64(arr: *[]u64, start: usize, end: usize) {
    let root: usize = start;
    while (root * 2 + 1 < end) {
        const left = root * 2 + 1;
        let swap_idx = root;
        if (arr.*[swap_idx] < arr.*[left]) {
            swap_idx = left;
        }
        if (left + 1 < end && arr.*[swap_idx] < arr.*[left + 1]) {
            swap_idx = left + 1;
        }
        if (swap_idx == root) {
            return;
        }
        const tmp = arr.*[root];
        arr.*[root] = arr.*[swap_idx];
        arr.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn heapSortU64(arr: *[]u64, len: usize) {
    if (len < 2) {
        return;
    }
    let start: usize = len / 2;
    while (start > 0) {
        start -= 1;
        siftDownU64(arr, start, len);
    }
    let end: usize = len;
    while (end > 1) {
        end -= 1;
        const tmp = arr.*[end];
        arr.*[end] = arr.*[0];
        arr.*[0] = tmp;
        siftDownU64(arr, 0, end);
    }
}

fn siftDownF64(arr: *[]f64, start: usize, end: usize) {
    let root: usize = start;
    while (root * 2 + 1 < end) {
        const left = root * 2 + 1;
        let swap_idx = root;
        if (arr.*[swap_idx] < arr.*[left]) {
            swap_idx = left;
        }
        if (left + 1 < end && arr.*[swap_idx] < arr.*[left + 1]) {
            swap_idx = left + 1;
        }
        if (swap_idx == root) {
            return;
        }
        const tmp = arr.*[root];
        arr.*[root] = arr.*[swap_idx];
        arr.*[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn heapSortF64(arr: *[]f64, len: usize) {
    if (len < 2) {
        return;
    }
    let start: usize = len / 2;
    while (start > 0) {
        start -= 1;
        siftDownF64(arr, start, len);
    }
    let end: usize = len;
    while (end > 1) {
        end -= 1;
        const tmp = arr.*[end];
        arr.*[end] = arr.*[0];
        arr.*[0] = tmp;
        siftDownF64(arr, 0, end);
    }
}

fn tokenizeHashes(text: string, out_hashes: *[]u64): usize? {
    let count: usize = 0;
    let i: usize = 0;
    const n = text.len;

    while (i < n) {
        while (i < n && !isTokenByte(text[i])) {
            i += 1;
        }
        if (i >= n) {
            break;
        }
        if (count >= out_hashes.*.len) {
            return null;
        }
        let h: u64 = FNV1A_OFFSET_BASIS;
        while (i < n && isTokenByte(text[i])) {
            h = (h ^ (toLowerAscii(text[i]) as u64)) *% FNV1A_PRIME;
            i += 1;
        }
        out_hashes.*[count] = h;
        count += 1;
    }

    return count;
}

/// Unbiased HumanEval Pass@k estimator: `1 - C(n - c, k) / C(n, k)`.
/// Uses overflow-free multiplicative ratios over `i in 0..k`.
/// Returns `null` if `k == 0`, `n < k`, or `c > n`.
pub fn passAtK(n: usize, c: usize, k: usize): f64? {
    if (k == 0 || n < k || c > n) {
        return null;
    }
    if (c == 0) {
        return 0.0;
    }
    if (n - c < k) {
        return 1.0;
    }

    let prod: f64 = 1.0;
    let i: usize = 0;
    while (i < k) {
        const num = (n - c - i) as f64;
        const den = (n - i) as f64;
        prod *= num / den;
        i += 1;
    }

    const res = 1.0 - prod;
    if (res < 0.0) {
        return 0.0;
    }
    if (res > 1.0) {
        return 1.0;
    }
    return res;
}

/// Zero-allocation normalized exact-match comparator (ASCII case-insensitive,
/// ignoring ASCII punctuation and whitespace runs).
pub fn exactMatchNormalized(pred: string, gold: string): bool {
    let pi: usize = 0;
    let gi: usize = 0;
    const pn = pred.len;
    const gn = gold.len;

    while (true) {
        while (pi < pn && !isTokenByte(pred[pi])) {
            pi += 1;
        }
        while (gi < gn && !isTokenByte(gold[gi])) {
            gi += 1;
        }

        if (pi >= pn && gi >= gn) {
            return true;
        }
        if (pi >= pn || gi >= gn) {
            return false;
        }

        while (pi < pn && gi < gn && isTokenByte(pred[pi]) && isTokenByte(gold[gi])) {
            if (toLowerAscii(pred[pi]) != toLowerAscii(gold[gi])) {
                return false;
            }
            pi += 1;
            gi += 1;
        }

        const p_still = pi < pn && isTokenByte(pred[pi]);
        const g_still = gi < gn && isTokenByte(gold[gi]);
        if (p_still || g_still) {
            return false;
        }
    }
}

/// Token-level precision, recall, and F1 score returned by `tokenOverlapF1`.
pub struct TextF1Result {
    precision: f64,
    recall: f64,
    f1: f64,
}

/// Zero-allocation multiset token-overlap F1 (SQuAD semantics: ASCII case-insensitive,
/// punctuation/whitespace split, multiset frequency intersection).
/// Returns `null` if either caller scratch buffer is too small for the tokenized text.
pub fn tokenOverlapF1(
    pred: string,
    gold: string,
    pred_hashes_s: *[]u64,
    gold_hashes_s: *[]u64,
): TextF1Result? {
    const pred_len = tokenizeHashes(pred, pred_hashes_s) ?? return null;
    const gold_len = tokenizeHashes(gold, gold_hashes_s) ?? return null;

    if (pred_len == 0 && gold_len == 0) {
        return TextF1Result {
            precision: 1.0,
            recall: 1.0,
            f1: 1.0,
        };
    }
    if (pred_len == 0 || gold_len == 0) {
        return TextF1Result {
            precision: 0.0,
            recall: 0.0,
            f1: 0.0,
        };
    }

    heapSortU64(pred_hashes_s, pred_len);
    heapSortU64(gold_hashes_s, gold_len);

    let common: usize = 0;
    let i: usize = 0;
    let j: usize = 0;
    while (i < pred_len && j < gold_len) {
        const a = pred_hashes_s.*[i];
        const b = gold_hashes_s.*[j];
        if (a == b) {
            common += 1;
            i += 1;
            j += 1;
        } else if (a < b) {
            i += 1;
        } else {
            j += 1;
        }
    }

    if (common == 0) {
        return TextF1Result {
            precision: 0.0,
            recall: 0.0,
            f1: 0.0,
        };
    }

    const precision = (common as f64) / (pred_len as f64);
    const recall = (common as f64) / (gold_len as f64);
    const f1 = (2.0 * precision * recall) / (precision + recall);
    return TextF1Result {
        precision: precision,
        recall: recall,
        f1: f1,
    };
}

/// Tool-use trajectory evaluation metrics returned by `scoreToolTrajectory`.
pub struct TrajectoryScore {
    exact_order_match: bool,
    order_lcs_ratio: f64,
    set_precision: f64,
    set_recall: f64,
    set_f1: f64,
}

/// Evaluates an agent's actual tool-call ID sequence against an expected sequence:
/// - `exact_order_match`: exact length and element-wise order equality
/// - `order_lcs_ratio`: `LCS(actual, expected) / max(actual.len, expected.len)`
/// - `set_precision`, `set_recall`, `set_f1`: unordered unique-set precision/recall/F1
/// Requires `dp_prev_s.*.len >= expected_tools.len + 1` and `dp_curr_s.*.len >= expected_tools.len + 1`
/// when both slices are non-empty; returns `null` if scratch buffers are insufficient.
pub fn scoreToolTrajectory(
    actual_tools: []const u64,
    expected_tools: []const u64,
    dp_prev_s: *[]usize,
    dp_curr_s: *[]usize,
): TrajectoryScore? {
    const m = actual_tools.len;
    const n = expected_tools.len;

    if (m == 0 && n == 0) {
        return TrajectoryScore {
            exact_order_match: true,
            order_lcs_ratio: 1.0,
            set_precision: 1.0,
            set_recall: 1.0,
            set_f1: 1.0,
        };
    }
    if (m == 0 || n == 0) {
        return TrajectoryScore {
            exact_order_match: false,
            order_lcs_ratio: 0.0,
            set_precision: 0.0,
            set_recall: 0.0,
            set_f1: 0.0,
        };
    }

    if (dp_prev_s.*.len < n + 1 || dp_curr_s.*.len < n + 1) {
        return null;
    }

    // 1. Exact order match
    let exact: bool = m == n;
    if (exact) {
        let i: usize = 0;
        while (i < m) {
            if (actual_tools[i] != expected_tools[i]) {
                exact = false;
                break;
            }
            i += 1;
        }
    }

    // 2. Two-row LCS DP
    let j0: usize = 0;
    while (j0 <= n) {
        dp_prev_s.*[j0] = 0;
        j0 += 1;
    }

    let i: usize = 0;
    while (i < m) {
        dp_curr_s.*[0] = 0;
        let j: usize = 0;
        while (j < n) {
            if (actual_tools[i] == expected_tools[j]) {
                dp_curr_s.*[j + 1] = dp_prev_s.*[j] + 1;
            } else {
                const up = dp_prev_s.*[j + 1];
                const left = dp_curr_s.*[j];
                dp_curr_s.*[j + 1] = if (up >= left) up else left;
            }
            j += 1;
        }
        let k: usize = 0;
        while (k <= n) {
            dp_prev_s.*[k] = dp_curr_s.*[k];
            k += 1;
        }
        i += 1;
    }

    const lcs_len = dp_prev_s.*[n];
    const max_len = if (m >= n) m else n;
    const order_lcs_ratio = (lcs_len as f64) / (max_len as f64);

    // 3. Zero-alloc unique set precision / recall / F1
    let unique_actual: usize = 0;
    let intersection: usize = 0;
    let a: usize = 0;
    while (a < m) {
        let seen_before: bool = false;
        let p: usize = 0;
        while (p < a) {
            if (actual_tools[p] == actual_tools[a]) {
                seen_before = true;
                break;
            }
            p += 1;
        }
        if (!seen_before) {
            unique_actual += 1;
            let e: usize = 0;
            while (e < n) {
                if (expected_tools[e] == actual_tools[a]) {
                    intersection += 1;
                    break;
                }
                e += 1;
            }
        }
        a += 1;
    }

    let unique_expected: usize = 0;
    let b: usize = 0;
    while (b < n) {
        let seen_before: bool = false;
        let p: usize = 0;
        while (p < b) {
            if (expected_tools[p] == expected_tools[b]) {
                seen_before = true;
                break;
            }
            p += 1;
        }
        if (!seen_before) {
            unique_expected += 1;
        }
        b += 1;
    }

    const set_precision = (intersection as f64) / (unique_actual as f64);
    const set_recall = (intersection as f64) / (unique_expected as f64);
    const set_f1 = if (intersection == 0)
        0.0
    else
        (2.0 * set_precision * set_recall) / (set_precision + set_recall);

    return TrajectoryScore {
        exact_order_match: exact,
        order_lcs_ratio: order_lcs_ratio,
        set_precision: set_precision,
        set_recall: set_recall,
        set_f1: set_f1,
    };
}

/// Aggregate benchmark summary returned by `summarizeEvalRun`.
pub struct EvalSummary {
    total_cases: usize,
    passed_cases: usize,
    pass_rate: f64,
    mean_score: f64,
    p50_ms: f64,
    p95_ms: f64,
    p99_ms: f64,
    total_cost_micro_usd: u64,
}

fn percentileLinear(sorted: []const f64, len: usize, q: f64): f64 {
    if (len == 1) {
        return sorted[0];
    }
    const pos = q * ((len - 1) as f64);
    const lo = pos as usize;
    const hi = if (lo + 1 < len) (lo + 1) else lo;
    const frac = pos - (lo as f64);
    return sorted[lo] + (sorted[hi] - sorted[lo]) * frac;
}

/// Summarizes an evaluation run with pass rate, mean score, linear-interpolated P50/P95/P99 latencies,
/// and saturating total micro-USD cost.
/// - `passed`: if `passed.len == scores.len`, uses `passed[i] != 0`; if `passed.len == 0`, falls back to `scores[i] >= pass_threshold`.
/// - `costs_micro_usd`: may be empty (`len == 0`) or match `scores.len`.
/// Returns `null` on empty `scores`, length mismatch, insufficient `lat_scratch`, `NaN` values, or negative latencies.
pub fn summarizeEvalRun(
    scores: []const f64,
    passed: []const u8,
    latencies_ms: []const f64,
    costs_micro_usd: []const u64,
    pass_threshold: f64,
    lat_scratch: *[]f64,
): EvalSummary? {
    const n = scores.len;
    if (n == 0) {
        return null;
    }
    if (latencies_ms.len != n) {
        return null;
    }
    if (passed.len != 0 && passed.len != n) {
        return null;
    }
    if (costs_micro_usd.len != 0 && costs_micro_usd.len != n) {
        return null;
    }
    if (lat_scratch.*.len < n) {
        return null;
    }
    if (isNaN(pass_threshold)) {
        return null;
    }

    let score_sum: f64 = 0.0;
    let passed_cases: usize = 0;
    let total_cost: u64 = 0;

    let i: usize = 0;
    while (i < n) {
        const sc = scores[i];
        const lat = latencies_ms[i];
        if (isNaN(sc) || isNaN(lat) || lat < 0.0) {
            return null;
        }
        score_sum += sc;
        lat_scratch.*[i] = lat;

        if (passed.len == n) {
            if (passed[i] != 0) {
                passed_cases += 1;
            }
        } else {
            if (sc >= pass_threshold) {
                passed_cases += 1;
            }
        }

        if (costs_micro_usd.len == n) {
            total_cost = satAddU64(total_cost, costs_micro_usd[i]);
        }
        i += 1;
    }

    heapSortF64(lat_scratch, n);

    const sorted_view: []const f64 = lat_scratch.*[0..n];
    return EvalSummary {
        total_cases: n,
        passed_cases: passed_cases,
        pass_rate: (passed_cases as f64) / (n as f64),
        mean_score: score_sum / (n as f64),
        p50_ms: percentileLinear(sorted_view, n, 0.50),
        p95_ms: percentileLinear(sorted_view, n, 0.95),
        p99_ms: percentileLinear(sorted_view, n, 0.99),
        total_cost_micro_usd: total_cost,
    };
}

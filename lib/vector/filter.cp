// Origin: Pure-CP Stdlib (AI Vector Search Metadata Bitset Filter & Selectivity Planner) | Created: 2026-11-22
// Zero-heap-allocation Filtered ANN metadata bitset filtering, selectivity-driven execution planning,
// and bitset-masked Top-K cosine similarity search:
//   - `FilterBitset`: fixed 1024-bit (16 x u64) zero-heap bitset with hardware `@popCount` / `@ctz`
//   - `FilterOp` + `NumericFilter` + `evalInt64Filter` / `evalFloat64Filter`
//   - `FilterStrategy` + `FilterPlan` + `planFilterStrategy` (PreFilterBrute / InlineGraphFilter / PostFilterExpand)
//   - `maskedTopKCosine`: bit-jumping Top-K cosine search over masked subset

const std = @import("std");

/// Number of 64-bit words in `FilterBitset`.
pub const FILTER_BITSET_WORDS: usize = 16;

/// Maximum document capacity of `FilterBitset` (`16 * 64 = 1024` bits).
pub const FILTER_BITSET_MAX_BITS: usize = 1024;

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Fixed-capacity 1024-bit (`16 x u64`) zero-heap metadata filter bitset.
pub struct FilterBitset {
    words: [16]u64 = [0; 16];

    /// Returns an all-zero bitset.
    static fn empty(): FilterBitset {
        return FilterBitset{ words: [0; 16] };
    }

    /// Returns a bitset with bits `[0, min(n_bits, 1024))` set to `1`.
    static fn full(n_bits: usize): FilterBitset {
        let bs = FilterBitset{ words: [0; 16] };
        const clamped = if (n_bits > FILTER_BITSET_MAX_BITS) FILTER_BITSET_MAX_BITS else n_bits;
        const full_words = clamped / 64;
        let w: usize = 0;
        while (w < full_words) : (w += 1) {
            bs.words[w] = std.math.maxInt(u64);
        }
        const rem = clamped & 63;
        if (rem > 0 && full_words < FILTER_BITSET_WORDS) {
            const shift: u6 = @intCast(rem);
            bs.words[full_words] = ((1 as u64) << shift) - 1;
        }
        return bs;
    }

    /// Sets `bit` to `1`. Returns `false` if `bit >= 1024`.
    fn set(*_, bit: usize): bool {
        if (bit >= FILTER_BITSET_MAX_BITS) return false;
        const shift: u6 = @intCast(bit & 63);
        _.words[bit >> 6] |= (1 as u64) << shift;
        return true;
    }

    /// Clears `bit` to `0`. Returns `false` if `bit >= 1024`.
    fn clear(*_, bit: usize): bool {
        if (bit >= FILTER_BITSET_MAX_BITS) return false;
        const shift: u6 = @intCast(bit & 63);
        _.words[bit >> 6] &= ~((1 as u64) << shift);
        return true;
    }

    /// Returns `true` if `bit < 1024` and is set.
    fn testBit(bit: usize): bool {
        if (bit >= FILTER_BITSET_MAX_BITS) return false;
        const shift: u6 = @intCast(bit & 63);
        return ((_.words[bit >> 6] >> shift) & 1) != 0;
    }

    /// Returns the total number of set bits using hardware `@popCount`.
    fn popcount(): usize {
        let total: usize = 0;
        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            total += @popCount(_.words[w]) as usize;
        }
        return total;
    }

    /// In-place bitwise AND (`self &= other`).
    fn andWith(*_, other: FilterBitset) {
        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            _.words[w] &= other.words[w];
        }
    }

    /// In-place bitwise OR (`self |= other`).
    fn orWith(*_, other: FilterBitset) {
        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            _.words[w] |= other.words[w];
        }
    }

    /// In-place bitwise NOT masked to `[0, min(valid_bits, 1024))` so unused tail bits stay `0`.
    fn notInPlace(*_, valid_bits: usize) {
        const clamped = if (valid_bits > FILTER_BITSET_MAX_BITS) FILTER_BITSET_MAX_BITS else valid_bits;
        const full_words = clamped / 64;
        const rem = clamped & 63;

        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            if (w < full_words) {
                _.words[w] = ~_.words[w];
            } else if (w == full_words && rem > 0) {
                const shift: u6 = @intCast(rem);
                const mask: u64 = ((1 as u64) << shift) - 1;
                _.words[w] = (~_.words[w]) & mask;
            } else {
                _.words[w] = 0;
            }
        }
    }

    /// In-place bitwise difference (`self &= ~other`).
    fn andNotWith(*_, other: FilterBitset) {
        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            _.words[w] &= ~other.words[w];
        }
    }

    /// Returns the index of the lowest set bit, or `null` if the bitset is empty.
    fn firstSetBit(): usize? {
        let w: usize = 0;
        while (w < FILTER_BITSET_WORDS) : (w += 1) {
            const word = _.words[w];
            if (word != 0) {
                return w * 64 + (@ctz(word) as usize);
            }
        }
        return null;
    }
}

/// Predicate comparison operator for `NumericFilter`.
pub enum FilterOp {
    Eq,
    Neq,
    Gt,
    Gte,
    Lt,
    Lte,
    Range,
}

/// Numeric metadata predicate for `evalInt64Filter` and `evalFloat64Filter`.
/// - For `Eq`, `Neq`, `Gt`, `Gte`, `Lt`, `Lte`: compares against `i64_lo` / `f64_lo`.
/// - For `Range`: closed interval `[lo, hi]`.
pub struct NumericFilter {
    op: FilterOp = FilterOp.Eq;
    i64_lo: i64 = 0;
    i64_hi: i64 = 0;
    f64_lo: f64 = 0.0;
    f64_hi: f64 = 0.0;
}

/// Evaluates an `i64` column predicate into `out_mask` and returns the match count.
/// Returns `null` if `values.len > 1024` or `Range` has `i64_lo > i64_hi`.
pub fn evalInt64Filter(
    values: []const i64,
    filter: NumericFilter,
    out_mask: *FilterBitset,
): usize? {
    if (values.len > FILTER_BITSET_MAX_BITS) return null;
    if (filter.op == FilterOp.Range && filter.i64_lo > filter.i64_hi) return null;

    out_mask.* = FilterBitset.empty();
    let matches: usize = 0;

    let i: usize = 0;
    while (i < values.len) : (i += 1) {
        const v = values[i];
        const ok = switch (filter.op) {
            FilterOp.Eq => v == filter.i64_lo,
            FilterOp.Neq => v != filter.i64_lo,
            FilterOp.Gt => v > filter.i64_lo,
            FilterOp.Gte => v >= filter.i64_lo,
            FilterOp.Lt => v < filter.i64_lo,
            FilterOp.Lte => v <= filter.i64_lo,
            FilterOp.Range => v >= filter.i64_lo && v <= filter.i64_hi,
        };
        if (ok) {
            * = out_mask.*.set(i);
            matches += 1;
        }
    }

    return matches;
}

/// Evaluates an `f64` column predicate into `out_mask` and returns the match count.
/// NaN column values never match. Returns `null` if `values.len > 1024`, filter thresholds
/// are non-finite, or `Range` has `f64_lo > f64_hi`.
pub fn evalFloat64Filter(
    values: []const f64,
    filter: NumericFilter,
    out_mask: *FilterBitset,
): usize? {
    if (values.len > FILTER_BITSET_MAX_BITS) return null;
    if (!isFiniteF64(filter.f64_lo)) return null;
    if (filter.op == FilterOp.Range) {
        if (!isFiniteF64(filter.f64_hi) || filter.f64_lo > filter.f64_hi) return null;
    }

    out_mask.* = FilterBitset.empty();
    let matches: usize = 0;

    let i: usize = 0;
    while (i < values.len) : (i += 1) {
        const v = values[i];
        if (std.math.isNan(v)) continue;
        const ok = switch (filter.op) {
            FilterOp.Eq => v == filter.f64_lo,
            FilterOp.Neq => v != filter.f64_lo,
            FilterOp.Gt => v > filter.f64_lo,
            FilterOp.Gte => v >= filter.f64_lo,
            FilterOp.Lt => v < filter.f64_lo,
            FilterOp.Lte => v <= filter.f64_lo,
            FilterOp.Range => v >= filter.f64_lo && v <= filter.f64_hi,
        };
        if (ok) {
            * = out_mask.*.set(i);
            matches += 1;
        }
    }

    return matches;
}

/// Adaptive Filtered ANN execution strategy chosen by `planFilterStrategy`.
pub enum FilterStrategy {
    PreFilterBrute,
    InlineGraphFilter,
    PostFilterExpand,
}

/// Execution plan returned by `planFilterStrategy`.
pub struct FilterPlan {
    strategy: FilterStrategy = FilterStrategy.PreFilterBrute;
    selectivity: f64 = 0.0;
    oversample_factor: usize = 1;
}

/// Plans the optimal Filtered ANN execution path based on filter selectivity `matching_docs / total_docs`
/// and requested `top_k`:
/// - Low selectivity (`selectivity < 0.05` or `matching_docs <= top_k * 2`): `PreFilterBrute` (`oversample = 1`).
/// - Medium selectivity (`0.05 <= selectivity < 0.40`): `InlineGraphFilter` (`oversample = ceil(1 / selectivity)`).
/// - High selectivity (`selectivity >= 0.40`): `PostFilterExpand` (`oversample = ceil(1 / selectivity)`).
///
/// Returns `null` if `total_docs == 0`, `matching_docs > total_docs`, or `top_k == 0`.
pub fn planFilterStrategy(
    total_docs: usize,
    matching_docs: usize,
    top_k: usize,
): FilterPlan? {
    if (total_docs == 0 || matching_docs > total_docs || top_k == 0) return null;

    const sel = (matching_docs as f64) / (total_docs as f64);
    const brute_cap = if (top_k <= std.math.maxInt(usize) / 2) top_k * 2 else std.math.maxInt(usize);

    if (matching_docs == 0 || matching_docs <= brute_cap || sel < 0.05) {
        return FilterPlan{
            strategy: FilterStrategy.PreFilterBrute,
            selectivity: sel,
            oversample_factor: 1,
        };
    }

    const oversample: usize = (total_docs - 1) / matching_docs + 1;

    if (sel >= 0.40) {
        return FilterPlan{
            strategy: FilterStrategy.PostFilterExpand,
            selectivity: sel,
            oversample_factor: oversample,
        };
    }

    return FilterPlan{
        strategy: FilterStrategy.InlineGraphFilter,
        selectivity: sel,
        oversample_factor: oversample,
    };
}

/// Zero-heap-allocation bitset-masked Top-K cosine similarity search.
/// Uses hardware `@ctz` bit-jumping to visit only documents whose bit is set in `mask`.
/// Results are written to `out_ids[0..n]` and `out_scores[0..n]` in descending cosine score order
/// (breaking ties by ascending document index).
///
/// Returns the number of hits written (`<= min(k, mask.popcount())`), or `null` on invalid
/// dimensions (`n_docs == 0 || n_docs > 1024 || dim == 0 || k == 0`), undersized slices, or NaN/Inf.
pub fn maskedTopKCosine(
    query: []const f64,
    corpus_flat: []const f64,
    n_docs: usize,
    dim: usize,
    mask: FilterBitset,
    k: usize,
    out_ids: *[]usize,
    out_scores: *[]f64,
): usize? {
    if (n_docs == 0 || n_docs > FILTER_BITSET_MAX_BITS || dim == 0 || k == 0) return null;
    if (n_docs > std.math.maxInt(usize) / dim) return null;

    const target_k = if (k < n_docs) k else n_docs;
    if (query.len < dim || corpus_flat.len < n_docs * dim) return null;
    if (out_ids.len < target_k || out_scores.len < target_k) return null;

    let q_sq: f64 = 0.0;
    let d0: usize = 0;
    while (d0 < dim) : (d0 += 1) {
        const qv = query[d0];
        if (!isFiniteF64(qv)) return null;
        q_sq += qv * qv;
    }
    const q_norm = std.math.sqrt(q_sq);

    let filled: usize = 0;
    let w_idx: usize = 0;
    while (w_idx < FILTER_BITSET_WORDS) : (w_idx += 1) {
        let word = mask.words[w_idx];
        while (word != 0) {
            const tz = @ctz(word) as usize;
            const doc_id = w_idx * 64 + tz;
            word &= word - 1; // clear lowest set bit

            if (doc_id >= n_docs) continue;

            const base = doc_id * dim;
            let dot: f64 = 0.0;
            let d_sq: f64 = 0.0;
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                const dv = corpus_flat[base + d];
                if (!isFiniteF64(dv)) return null;
                dot += query[d] * dv;
                d_sq += dv * dv;
            }
            const d_norm = std.math.sqrt(d_sq);
            const sim: f64 = if (q_norm > 0.0 && d_norm > 0.0) dot / (q_norm * d_norm) else 0.0;

            // Insert descending by `sim` (ascending by `doc_id` on tie; doc_id increases monotonically)
            let pos: usize = filled;
            while (pos > 0 && sim > out_scores[pos - 1]) {
                pos -= 1;
            }
            if (pos < target_k) {
                const new_filled = if (filled < target_k) filled + 1 else target_k;
                let shift: usize = new_filled - 1;
                while (shift > pos) : (shift -= 1) {
                    out_ids[shift] = out_ids[shift - 1];
                    out_scores[shift] = out_scores[shift - 1];
                }
                out_ids[pos] = doc_id;
                out_scores[pos] = sim;
                filled = new_filled;
            }
        }
    }

    return filled;
}

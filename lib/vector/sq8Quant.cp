// Origin: Pure-CP Stdlib (Zero-Alloc 8-Bit Asymmetric Scalar Quantization SQ8 Vector Index) | Created: 2026-11-23
// Pure CP zero-heap-allocation 8-bit Scalar Quantization (SQ8) engine achieving 4x memory compression:
//   - Per-dimension Min/Max & optional quantile-clipped calibrator (streaming Sq8Calibrator + batch histogram)
//   - Safe zero-variance dimension handling (scale = 0, zero NaN/Inf)
//   - 4x compressed encodeF32ToU8 / decodeU8ToF32 + asymmetric (f32 vs u8) & symmetric (u8 vs u8) kernels
//   - Fixed-capacity Sq8VectorIndex supporting 1-stage SQ8 Top-K, 2-stage coarse-to-fine f32 rescoring,
//     and quantization fidelity evaluation (MSE / MAE / cosine fidelity)

const std = @import("std");

// ============================================================================
// 1. Constants & Numeric Helpers
// ============================================================================

/// Maximum vector dimensionality supported inline (zero heap).
pub const SQ8_MAX_DIM: usize = 64;

/// Maximum quantized vectors stored in a single zero-alloc `Sq8VectorIndex`.
pub const SQ8_MAX_DOCS: usize = 256;

const SQ8_HIST_BINS: usize = 64;
const SQ8_RESCORE_CAP: usize = 64;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn allFiniteF32(v: []const f32): bool {
    let i: usize = 0;
    while (i < v.len) : (i += 1) {
        if (!isFiniteF32(v[i])) return false;
    }
    return true;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

/// Approximates upper normal z-score for tail quantile `q ∈ (0, 0.5)` for streaming clipping.
fn tailZScore(q: f32): f64 {
    if (q <= 0.001) return 3.090;
    if (q <= 0.005) return 2.576;
    if (q <= 0.01) return 2.326;
    if (q <= 0.025) return 1.960;
    if (q <= 0.05) return 1.645;
    if (q <= 0.10) return 1.282;
    return 1.0;
}

fn hitBetter(a: Sq8SearchHit, b: Sq8SearchHit, metric: Sq8Metric): bool {
    if (metric == Sq8Metric.dot) {
        if (a.score > b.score) return true;
        if (a.score < b.score) return false;
        return a.doc_id < b.doc_id;
    }
    // l2_sq: smaller is better
    if (a.score < b.score) return true;
    if (a.score > b.score) return false;
    return a.doc_id < b.doc_id;
}

fn insertHitSorted(
    buf: *[]Sq8SearchHit,
    count: *usize,
    cap: usize,
    hit: Sq8SearchHit,
    metric: Sq8Metric,
) {
    if (cap == 0) return;
    const cur_n: usize = count.*;
    if (cur_n >= cap && !hitBetter(hit, buf[cap - 1], metric)) {
        return;
    }
    let pos: usize = if (cur_n < cap) cur_n else (cap - 1);
    while (pos > 0 && hitBetter(hit, buf[pos - 1], metric)) {
        buf[pos] = buf[pos - 1];
        pos -= 1;
    }
    buf[pos] = hit;
    if (cur_n < cap) {
        count.* = cur_n + 1;
    }
}

// ============================================================================
// 2. Public Data Models & Per-Dimension Calibrator
// ============================================================================

/// Distance/similarity metric supported by `Sq8VectorIndex`.
pub enum Sq8Metric {
    dot,
    l2_sq,
}

/// Per-dimension 8-bit asymmetric scalar quantization codebook.
pub struct Sq8Codebook {
    dim: usize = 0;
    min_vals: [64]f32 = [0.0; 64];
    scales: [64]f32 = [0.0; 64];
    inv_scales: [64]f32 = [0.0; 64];
    calibrated: bool = false;

    /// Validates codebook dimensions and finiteness of all per-dimension parameters.
    fn isValid(): bool {
        if (!_.calibrated || _.dim == 0 || _.dim > SQ8_MAX_DIM) return false;
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            if (!isFiniteF32(_.min_vals[d]) || !isFiniteF32(_.scales[d]) || !isFiniteF32(_.inv_scales[d])) {
                return false;
            }
            if (_.scales[d] < 0.0 || _.inv_scales[d] < 0.0) return false;
        }
        return true;
    }
}

/// Incremental zero-alloc per-dimension Min/Max + Welford calibrator.
/// Supports optional outlier clipping via `clip_quantile ∈ [0.0, 0.25]` (e.g., `0.01` for 1%~99%).
pub struct Sq8Calibrator {
    dim: usize = 0;
    clip_quantile: f32 = 0.0;
    sample_count: u64 = 0;
    min_vals: [64]f32 = [0.0; 64];
    max_vals: [64]f32 = [0.0; 64];
    means: [64]f64 = [0.0; 64];
    m2: [64]f64 = [0.0; 64];

    /// Initializes a streaming calibrator for `dim` (1..64) and optional `clip_quantile ∈ [0, 0.25]`.
    static fn init(dim: usize, clip_quantile: f32): Sq8Calibrator? {
        if (dim == 0 || dim > SQ8_MAX_DIM) return null;
        if (!isFiniteF32(clip_quantile) || clip_quantile < 0.0 || clip_quantile > 0.25) return null;
        return Sq8Calibrator{
            dim: dim,
            clip_quantile: clip_quantile,
        };
    }

    /// Observes one `f32` vector of length `dim`. Rejects length mismatch or NaN/Inf.
    fn observe(*_, vec: []const f32): bool {
        if (vec.len != _.dim) return false;
        if (!allFiniteF32(vec)) return false;

        _.sample_count += 1;
        const n_f: f64 = _.sample_count as f64;
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            const x: f32 = vec[d];
            if (_.sample_count == 1) {
                _.min_vals[d] = x;
                _.max_vals[d] = x;
                _.means[d] = x as f64;
                _.m2[d] = 0.0;
            } else {
                if (x < _.min_vals[d]) _.min_vals[d] = x;
                if (x > _.max_vals[d]) _.max_vals[d] = x;
                const xf: f64 = x as f64;
                const delta: f64 = xf - _.means[d];
                _.means[d] += delta / n_f;
                const delta2: f64 = xf - _.means[d];
                _.m2[d] += delta * delta2;
            }
        }
        return true;
    }

    /// Returns number of vectors observed so far.
    fn count(): u64 {
        return _.sample_count;
    }

    /// Finalizes per-dimension `min_vals`, `scales`, and `inv_scales` into an `Sq8Codebook`.
    /// Zero-variance dimensions safely degenerate to `scale = 0.0`, `inv_scale = 0.0`.
    fn finalize(): Sq8Codebook? {
        if (_.sample_count == 0 || _.dim == 0) return null;

        let cb = Sq8Codebook{
            dim: _.dim,
            calibrated: true,
        };
        const z: f64 = if (_.clip_quantile > 0.0) tailZScore(_.clip_quantile) else 0.0;

        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            let lo: f64 = _.min_vals[d] as f64;
            let hi: f64 = _.max_vals[d] as f64;

            if (_.clip_quantile > 0.0 && _.sample_count >= 4) {
                const variance: f64 = _.m2[d] / ((_.sample_count - 1) as f64);
                if (variance > 0.0) {
                    const std_dev: f64 = std.math.sqrt(variance);
                    const clip_lo: f64 = _.means[d] - z * std_dev;
                    const clip_hi: f64 = _.means[d] + z * std_dev;
                    if (clip_lo > lo) lo = clip_lo;
                    if (clip_hi < hi) hi = clip_hi;
                }
            }

            const span: f64 = hi - lo;
            cb.min_vals[d] = lo as f32;
            if (span <= 0.000000000001) {
                cb.scales[d] = 0.0;
                cb.inv_scales[d] = 0.0;
            } else {
                cb.scales[d] = (span / 255.0) as f32;
                cb.inv_scales[d] = (255.0 / span) as f32;
            }
        }
        return cb;
    }
}

/// Two-pass zero-alloc batch calibrator supporting exact histogram quantile clipping
/// (e.g., `clip_quantile = 0.01` trims 1% lower & upper extreme outlier tails per dimension).
pub fn sq8CalibrateBatch(
    vecs_flat: []const f32,
    n_vecs: usize,
    dim: usize,
    clip_quantile: f32,
): Sq8Codebook? {
    if (n_vecs == 0 || dim == 0 || dim > SQ8_MAX_DIM) return null;
    if (n_vecs > std.math.maxInt(usize) / dim) return null;
    if (vecs_flat.len < n_vecs * dim) return null;
    if (!isFiniteF32(clip_quantile) || clip_quantile < 0.0 || clip_quantile > 0.25) return null;

    let min_v: [64]f32 = [0.0; 64];
    let max_v: [64]f32 = [0.0; 64];

    // Pass 1: exact per-dimension min/max + finiteness check.
    let i: usize = 0;
    while (i < n_vecs) : (i += 1) {
        const base: usize = i * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const x: f32 = vecs_flat[base + d];
            if (!isFiniteF32(x)) return null;
            if (i == 0) {
                min_v[d] = x;
                max_v[d] = x;
            } else {
                if (x < min_v[d]) min_v[d] = x;
                if (x > max_v[d]) max_v[d] = x;
            }
        }
    }

    let cb = Sq8Codebook{
        dim: dim,
        calibrated: true,
    };

    // Pass 2: optional 64-bin histogram quantile clipping per dimension.
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        let lo: f64 = min_v[d] as f64;
        let hi: f64 = max_v[d] as f64;
        const full_span: f64 = hi - lo;

        if (clip_quantile > 0.0 && full_span > 0.000000000001 && n_vecs >= 4) {
            let hist: [64]usize = [0; 64];
            let r: usize = 0;
            while (r < n_vecs) : (r += 1) {
                const x: f64 = vecs_flat[r * dim + d] as f64;
                const norm: f64 = (x - lo) / full_span;
                let bin: usize = (norm * ((SQ8_HIST_BINS - 1) as f64)) as usize;
                if (bin >= SQ8_HIST_BINS) bin = SQ8_HIST_BINS - 1;
                hist[bin] += 1;
            }

            const trim_target: usize = ((n_vecs as f64) * (clip_quantile as f64)) as usize;
            let lo_bin: usize = 0;
            let acc_lo: usize = 0;
            while (lo_bin < SQ8_HIST_BINS - 1) : (lo_bin += 1) {
                acc_lo += hist[lo_bin];
                if (acc_lo > trim_target) break;
            }

            let hi_bin: usize = SQ8_HIST_BINS - 1;
            let acc_hi: usize = 0;
            while (hi_bin > lo_bin) : (hi_bin -= 1) {
                acc_hi += hist[hi_bin];
                if (acc_hi > trim_target) break;
            }

            const bin_width: f64 = full_span / (SQ8_HIST_BINS as f64);
            const new_lo: f64 = lo + (lo_bin as f64) * bin_width;
            const new_hi: f64 = lo + ((hi_bin + 1) as f64) * bin_width;
            if (new_hi > new_lo) {
                lo = new_lo;
                hi = if (new_hi < hi) new_hi else hi;
            }
        }

        const span: f64 = hi - lo;
        cb.min_vals[d] = lo as f32;
        if (span <= 0.000000000001) {
            cb.scales[d] = 0.0;
            cb.inv_scales[d] = 0.0;
        } else {
            cb.scales[d] = (span / 255.0) as f32;
            cb.inv_scales[d] = (255.0 / span) as f32;
        }
    }

    return cb;
}

// ============================================================================
// 3. 4x Compression Codec & Asymmetric / Symmetric Distance Kernels
// ============================================================================

/// Quantizes an `f32` vector into `u8` SQ8 codes (4x memory compression).
/// Clamps out-of-range values cleanly to `[0, 255]` and rejects NaN/Inf.
pub fn encodeF32ToU8(
    codebook: Sq8Codebook,
    src: []const f32,
    out_codes: *[]u8,
): bool {
    if (!codebook.isValid()) return false;
    const dim: usize = codebook.dim;
    if (src.len != dim || out_codes.len < dim) return false;
    if (!allFiniteF32(src)) return false;

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const inv_s: f32 = codebook.inv_scales[d];
        if (inv_s == 0.0) {
            out_codes[d] = 0;
        } else {
            const raw: f64 = ((src[d] as f64) - (codebook.min_vals[d] as f64)) * (inv_s as f64);
            if (raw <= 0.0) {
                out_codes[d] = 0;
            } else if (raw >= 255.0) {
                out_codes[d] = 255;
            } else {
                out_codes[d] = (raw + 0.5) as u8;
            }
        }
    }
    return true;
}

/// Dequantizes `u8` SQ8 codes back to approximate `f32` vector: `min[d] + code[d] * scale[d]`.
pub fn decodeU8ToF32(
    codebook: Sq8Codebook,
    codes: []const u8,
    out_vec: *[]f32,
): bool {
    if (!codebook.isValid()) return false;
    const dim: usize = codebook.dim;
    if (codes.len < dim || out_vec.len < dim) return false;

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        out_vec[d] = codebook.min_vals[d] + (codes[d] as f32) * codebook.scales[d];
    }
    return true;
}

/// Asymmetric inner product between full-precision `query_f32` and SQ8-quantized `doc_u8`.
pub fn sq8AsymDotF32(
    codebook: Sq8Codebook,
    query_f32: []const f32,
    doc_u8: []const u8,
): f32? {
    if (!codebook.isValid()) return null;
    const dim: usize = codebook.dim;
    if (query_f32.len != dim || doc_u8.len < dim) return null;
    if (!allFiniteF32(query_f32)) return null;

    let acc: f64 = 0.0;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const doc_val: f64 = (codebook.min_vals[d] as f64) + (doc_u8[d] as f64) * (codebook.scales[d] as f64);
        acc += (query_f32[d] as f64) * doc_val;
    }
    return acc as f32;
}

/// Asymmetric squared Euclidean distance between full-precision `query_f32` and SQ8-quantized `doc_u8`.
pub fn sq8AsymL2SqF32(
    codebook: Sq8Codebook,
    query_f32: []const f32,
    doc_u8: []const u8,
): f32? {
    if (!codebook.isValid()) return null;
    const dim: usize = codebook.dim;
    if (query_f32.len != dim || doc_u8.len < dim) return null;
    if (!allFiniteF32(query_f32)) return null;

    let acc: f64 = 0.0;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const doc_val: f64 = (codebook.min_vals[d] as f64) + (doc_u8[d] as f64) * (codebook.scales[d] as f64);
        const diff: f64 = (query_f32[d] as f64) - doc_val;
        acc += diff * diff;
    }
    return acc as f32;
}

/// Symmetric inner product between two SQ8-quantized `u8` vectors.
pub fn sq8SymDotU8(
    codebook: Sq8Codebook,
    a_u8: []const u8,
    b_u8: []const u8,
): f32? {
    if (!codebook.isValid()) return null;
    const dim: usize = codebook.dim;
    if (a_u8.len < dim || b_u8.len < dim) return null;

    let acc: f64 = 0.0;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const m: f64 = codebook.min_vals[d] as f64;
        const s: f64 = codebook.scales[d] as f64;
        const va: f64 = m + (a_u8[d] as f64) * s;
        const vb: f64 = m + (b_u8[d] as f64) * s;
        acc += va * vb;
    }
    return acc as f32;
}

/// Quantization reconstruction fidelity metrics returned by `evaluateSq8Error`.
pub struct Sq8ErrorStats {
    mse: f32 = 0.0;
    mae: f32 = 0.0;
    max_abs_err: f32 = 0.0;
    cosine_fidelity: f32 = 0.0;
    compression_ratio: f32 = 4.0;
}

/// Evaluates SQ8 reconstruction MSE, MAE, max absolute error, and mean cosine fidelity
/// across `n_vecs` original `f32` vectors.
pub fn evaluateSq8Error(
    codebook: Sq8Codebook,
    orig_vecs: []const f32,
    n_vecs: usize,
): Sq8ErrorStats? {
    if (!codebook.isValid() || n_vecs == 0) return null;
    const dim: usize = codebook.dim;
    if (n_vecs > std.math.maxInt(usize) / dim) return null;
    if (orig_vecs.len < n_vecs * dim) return null;

    let codes: [64]u8 = [0; 64];
    let codes_s: []u8 = codes[0..dim];
    let recon: [64]f32 = [0.0; 64];
    let recon_s: []f32 = recon[0..dim];

    let sum_sq: f64 = 0.0;
    let sum_abs: f64 = 0.0;
    let max_err: f64 = 0.0;
    let cos_sum: f64 = 0.0;
    let cos_count: usize = 0;

    let i: usize = 0;
    while (i < n_vecs) : (i += 1) {
        const row = orig_vecs[i * dim .. (i + 1) * dim];
        if (!encodeF32ToU8(codebook, row, *codes_s)) return null;
        if (!decodeU8ToF32(codebook, codes[0..dim], *recon_s)) return null;

        let dot_ab: f64 = 0.0;
        let norm_a: f64 = 0.0;
        let norm_b: f64 = 0.0;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const a: f64 = row[d] as f64;
            const b: f64 = recon[d] as f64;
            const err: f64 = absF64(a - b);
            sum_sq += err * err;
            sum_abs += err;
            if (err > max_err) max_err = err;
            dot_ab += a * b;
            norm_a += a * a;
            norm_b += b * b;
        }
        if (norm_a > 0.0 && norm_b > 0.0) {
            cos_sum += dot_ab / (std.math.sqrt(norm_a) * std.math.sqrt(norm_b));
            cos_count += 1;
        }
    }

    const total_elems: f64 = (n_vecs * dim) as f64;
    return Sq8ErrorStats{
        mse: (sum_sq / total_elems) as f32,
        mae: (sum_abs / total_elems) as f32,
        max_abs_err: max_err as f32,
        cosine_fidelity: if (cos_count > 0) ((cos_sum / (cos_count as f64)) as f32) else 1.0,
        compression_ratio: 4.0,
    };
}

// ============================================================================
// 4. Zero-Alloc SQ8 Vector Index (1-Stage Coarse + 2-Stage f32 Rescore)
// ============================================================================

/// Search hit returned by `Sq8VectorIndex.searchTopK` and `rescoreTopK`.
pub struct Sq8SearchHit {
    doc_id: u32 = 0;
    slot_idx: usize = 0;
    score: f32 = 0.0;
}

/// Memory footprint and capacity telemetry for `Sq8VectorIndex`.
pub struct Sq8IndexStats {
    dim: usize = 0;
    doc_count: usize = 0;
    capacity: usize = SQ8_MAX_DOCS;
    compressed_bytes: usize = 0;
    uncompressed_bytes: usize = 0;
}

/// Fixed-capacity zero-heap-allocation SQ8 vector index (up to 256 vectors × 64 dims).
pub struct Sq8VectorIndex {
    cb: Sq8Codebook = Sq8Codebook{},
    doc_ids: [256]u32 = [0; 256],
    codes: [16384]u8 = [0; 16384], // 256 * 64 = 16 KiB
    doc_count: usize = 0;

    /// Initializes an SQ8 index with a calibrated `Sq8Codebook`.
    static fn init(cb: Sq8Codebook): Sq8VectorIndex? {
        if (!cb.isValid()) return null;
        return Sq8VectorIndex{
            cb: cb,
        };
    }

    /// Returns number of indexed vectors.
    fn len(): usize {
        return _.doc_count;
    }

    /// Returns maximum vector capacity (`SQ8_MAX_DOCS`).
    fn capacity(): usize {
        return SQ8_MAX_DOCS;
    }

    /// Returns a copy of the active quantization codebook.
    fn codebook(): Sq8Codebook {
        return _.cb;
    }

    /// Clears all indexed vectors while retaining the calibrated codebook.
    fn clear(*_) {
        _.doc_count = 0;
    }

    /// Returns memory usage and document count telemetry.
    fn stats(): Sq8IndexStats {
        const comp: usize = _.doc_count * _.cb.dim;
        return Sq8IndexStats{
            dim: _.cb.dim,
            doc_count: _.doc_count,
            capacity: SQ8_MAX_DOCS,
            compressed_bytes: comp,
            uncompressed_bytes: comp * 4,
        };
    }

    /// Quantizes and appends an `f32` vector with `doc_id` (`doc_id != 0`).
    fn addVector(*_, doc_id: u32, vec: []const f32): bool {
        if (doc_id == 0 || _.doc_count >= SQ8_MAX_DOCS) return false;
        const dim: usize = _.cb.dim;
        const off: usize = _.doc_count * dim;
        let dst: []u8 = _.codes[off .. off + dim];
        if (!encodeF32ToU8(_.cb, vec, *dst)) return false;
        _.doc_ids[_.doc_count] = doc_id;
        _.doc_count += 1;
        return true;
    }

    /// Appends a pre-quantized `u8` vector slice with `doc_id` (`doc_id != 0`).
    fn addQuantized(*_, doc_id: u32, q_codes: []const u8): bool {
        const dim: usize = _.cb.dim;
        if (doc_id == 0 || _.doc_count >= SQ8_MAX_DOCS || q_codes.len != dim) return false;
        const off: usize = _.doc_count * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            _.codes[off + d] = q_codes[d];
        }
        _.doc_ids[_.doc_count] = doc_id;
        _.doc_count += 1;
        return true;
    }

    /// Stage-1 asymmetric SQ8 Top-K retrieval (`dot` descending, `l2_sq` ascending).
    /// Returns number of hits written into `out_hits`, or `null` on invalid query.
    fn searchTopK(
        query: []const f32,
        metric: Sq8Metric,
        top_k: usize,
        out_hits: *[]Sq8SearchHit,
    ): usize? {
        const dim: usize = _.cb.dim;
        if (query.len != dim || !allFiniteF32(query)) return null;
        if (top_k == 0 || out_hits.len == 0 || _.doc_count == 0) return 0;

        const cap: usize = if (top_k < out_hits.len) top_k else out_hits.len;
        let count: usize = 0;

        let i: usize = 0;
        while (i < _.doc_count) : (i += 1) {
            const code_slice = _.codes[i * dim .. (i + 1) * dim];
            const sc: f32 = if (metric == Sq8Metric.dot)
                (sq8AsymDotF32(_.cb, query, code_slice) orelse return null)
            else
                (sq8AsymL2SqF32(_.cb, query, code_slice) orelse return null);

            insertHitSorted(out_hits, *count, cap, Sq8SearchHit{
                doc_id: _.doc_ids[i],
                slot_idx: i,
                score: sc,
            }, metric);
        }
        return count;
    }

    /// Two-stage search: runs Stage-1 SQ8 coarse retrieval (`coarse_k` candidates) and
    /// Stage-2 exact `f32` rescoring against external `raw_vecs_flat` (`[doc_count, dim]`),
    /// returning top `final_k` exact-ranked hits.
    fn rescoreTopK(
        query: []const f32,
        raw_vecs_flat: []const f32,
        metric: Sq8Metric,
        coarse_k: usize,
        final_k: usize,
        out_hits: *[]Sq8SearchHit,
    ): usize? {
        const dim: usize = _.cb.dim;
        if (query.len != dim || !allFiniteF32(query)) return null;
        if (_.doc_count > 0) {
            if (_.doc_count > std.math.maxInt(usize) / dim) return null;
            if (raw_vecs_flat.len < _.doc_count * dim) return null;
        }
        if (coarse_k == 0 || final_k == 0 || out_hits.len == 0 || _.doc_count == 0) return 0;

        const eff_coarse: usize = if (coarse_k <= SQ8_RESCORE_CAP) coarse_k else SQ8_RESCORE_CAP;
        let coarse_buf: [64]Sq8SearchHit = [Sq8SearchHit{}; 64];
        let coarse_s: []Sq8SearchHit = coarse_buf[0..eff_coarse];
        const n_coarse = _.searchTopK(query, metric, eff_coarse, *coarse_s) orelse return null;

        const cap: usize = if (final_k < out_hits.len) final_k else out_hits.len;
        let out_count: usize = 0;

        let c: usize = 0;
        while (c < n_coarse) : (c += 1) {
            const cand = coarse_buf[c];
            const row = raw_vecs_flat[cand.slot_idx * dim .. (cand.slot_idx + 1) * dim];
            if (!allFiniteF32(row)) return null;

            let exact_sc: f64 = 0.0;
            let d: usize = 0;
            if (metric == Sq8Metric.dot) {
                while (d < dim) : (d += 1) {
                    exact_sc += (query[d] as f64) * (row[d] as f64);
                }
            } else {
                while (d < dim) : (d += 1) {
                    const diff: f64 = (query[d] as f64) - (row[d] as f64);
                    exact_sc += diff * diff;
                }
            }

            insertHitSorted(out_hits, *out_count, cap, Sq8SearchHit{
                doc_id: cand.doc_id,
                slot_idx: cand.slot_idx,
                score: exact_sc as f32,
            }, metric);
        }

        return out_count;
    }
}

// Origin: Pure-CP Stdlib (AI Inference Multi-Tenant LoRA Low-Rank Adapter & Weight Merge) | Created: 2026-11-22
// Zero-heap-allocation Multi-Tenant Low-Rank Adaptation (LoRA) inference & weight-merging primitives:
//   - Zero-copy `LoraAdapter` descriptor borrowing `A: [rank, in_dim]` and `B: [out_dim, rank]`
//   - Single-adapter incremental forward `y += (alpha / rank) * B * (A * x)` in O(r * (in_dim + out_dim))
//   - Multi-adapter weighted composition (`loraApplyWeightedSum`) for concurrent multi-tenant mixing
//   - Static weight hot-merge (`W += (alpha / rank) * B * A`) and exact hot-unmerge (`W -= (alpha / rank) * B * A`)

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Zero-copy LoRA low-rank adapter descriptor.
/// Borrows caller-owned row-major weight matrices:
/// - `a_mat`: `[rank, in_dim]` (down-projection)
/// - `b_mat`: `[out_dim, rank]` (up-projection)
pub struct LoraAdapter {
    id: u32 = 0;
    in_dim: usize = 0;
    out_dim: usize = 0;
    rank: usize = 0;
    alpha: f64 = 1.0;
    a_mat: []const f64;
    b_mat: []const f64;

    /// Constructs a validated `LoraAdapter` over caller-owned `a_mat` and `b_mat`.
    /// Returns `null` if any dimension/rank is zero, `alpha` or matrix entries are non-finite,
    /// or `a_mat`/`b_mat` slices are undersized.
    static fn init(
        id: u32,
        in_dim: usize,
        out_dim: usize,
        rank: usize,
        alpha: f64,
        a_mat: []const f64,
        b_mat: []const f64,
    ): LoraAdapter? {
        if (in_dim == 0 || out_dim == 0 || rank == 0) return null;
        if (!isFiniteF64(alpha)) return null;
        if (rank > std.math.maxInt(usize) / in_dim) return null;
        if (out_dim > std.math.maxInt(usize) / rank) return null;

        const a_elems = rank * in_dim;
        const b_elems = out_dim * rank;
        if (a_mat.len < a_elems || b_mat.len < b_elems) return null;

        let ia: usize = 0;
        while (ia < a_elems) : (ia += 1) {
            if (!isFiniteF64(a_mat[ia])) return null;
        }
        let ib: usize = 0;
        while (ib < b_elems) : (ib += 1) {
            if (!isFiniteF64(b_mat[ib])) return null;
        }

        return LoraAdapter{
            id: id,
            in_dim: in_dim,
            out_dim: out_dim,
            rank: rank,
            alpha: alpha,
            a_mat: a_mat[0..a_elems],
            b_mat: b_mat[0..b_elems],
        };
    }

    /// Returns the LoRA scaling factor `alpha / rank`.
    fn scaling(): f64 {
        if (_.rank == 0 || !isFiniteF64(_.alpha)) return 0.0;
        return _.alpha / (_.rank as f64);
    }
}

fn isValidAdapter(adapter: LoraAdapter): bool {
    if (adapter.in_dim == 0 || adapter.out_dim == 0 || adapter.rank == 0) return false;
    if (!isFiniteF64(adapter.alpha)) return false;
    if (adapter.rank > std.math.maxInt(usize) / adapter.in_dim) return false;
    if (adapter.out_dim > std.math.maxInt(usize) / adapter.rank) return false;

    const a_elems = adapter.rank * adapter.in_dim;
    const b_elems = adapter.out_dim * adapter.rank;
    if (adapter.a_mat.len < a_elems || adapter.b_mat.len < b_elems) return false;

    let ia: usize = 0;
    while (ia < a_elems) : (ia += 1) {
        if (!isFiniteF64(adapter.a_mat[ia])) return false;
    }
    let ib: usize = 0;
    while (ib < b_elems) : (ib += 1) {
        if (!isFiniteF64(adapter.b_mat[ib])) return false;
    }
    return true;
}

fn applySingleAdapterScaled(
    adapter: LoraAdapter,
    extra_scale: f64,
    x: []const f64,
    y_inout: *[]f64,
    hidden_scratch: *[]f64,
) {
    const in_dim = adapter.in_dim;
    const out_dim = adapter.out_dim;
    const rank = adapter.rank;
    const eff_scale = extra_scale * adapter.scaling();

    // Step 1: h = A * x  (length = rank)
    let r: usize = 0;
    while (r < rank) : (r += 1) {
        const a_base = r * in_dim;
        let acc: f64 = 0.0;
        let j: usize = 0;
        while (j < in_dim) : (j += 1) {
            acc += adapter.a_mat[a_base + j] * x[j];
        }
        hidden_scratch[r] = acc;
    }

    // Step 2: y_inout += eff_scale * (B * h)  (length = out_dim)
    let o: usize = 0;
    while (o < out_dim) : (o += 1) {
        const b_base = o * rank;
        let dot: f64 = 0.0;
        let r2: usize = 0;
        while (r2 < rank) : (r2 += 1) {
            dot += adapter.b_mat[b_base + r2] * hidden_scratch[r2];
        }
        y_inout[o] += eff_scale * dot;
    }
}

/// Applies a single LoRA adapter incrementally in-place:
/// `y_inout += (alpha / rank) * B * (A * x)` in `O(rank * (in_dim + out_dim))` time.
///
/// Caller must provide `hidden_scratch` of length `>= adapter.rank`.
/// Returns `false` on invalid adapter, undersized slices, or NaN/Inf inputs.
pub fn loraApplyAdd(
    adapter: LoraAdapter,
    x: []const f64,
    y_inout: *[]f64,
    hidden_scratch: *[]f64,
): bool {
    if (!isValidAdapter(adapter)) return false;
    if (x.len < adapter.in_dim || y_inout.len < adapter.out_dim || hidden_scratch.len < adapter.rank) {
        return false;
    }

    let ix: usize = 0;
    while (ix < adapter.in_dim) : (ix += 1) {
        if (!isFiniteF64(x[ix])) return false;
    }
    let iy: usize = 0;
    while (iy < adapter.out_dim) : (iy += 1) {
        if (!isFiniteF64(y_inout[iy])) return false;
    }

    applySingleAdapterScaled(adapter, 1.0, x, y_inout, hidden_scratch);
    return true;
}

/// Applies a weighted linear combination of multiple LoRA adapters in-place:
/// `y_inout += sum_k weights[k] * (alpha_k / rank_k) * B_k * (A_k * x)`.
///
/// All adapters must share the same `(in_dim, out_dim)`, and `hidden_scratch.len` must be
/// `>= max_k(adapters[k].rank)`. Validation is performed atomically before mutating `y_inout`.
/// Returns `false` on empty adapter list, dimension mismatch, undersized buffers, or NaN/Inf inputs.
pub fn loraApplyWeightedSum(
    adapters: []const LoraAdapter,
    weights: []const f64,
    x: []const f64,
    y_inout: *[]f64,
    hidden_scratch: *[]f64,
): bool {
    const n_adapters = adapters.len;
    if (n_adapters == 0 || weights.len < n_adapters) return false;

    const in_dim = adapters[0].in_dim;
    const out_dim = adapters[0].out_dim;

    let k_chk: usize = 0;
    while (k_chk < n_adapters) : (k_chk += 1) {
        if (!isFiniteF64(weights[k_chk])) return false;
        const ad = adapters[k_chk];
        if (!isValidAdapter(ad)) return false;
        if (ad.in_dim != in_dim || ad.out_dim != out_dim) return false;
        if (hidden_scratch.len < ad.rank) return false;
    }

    if (x.len < in_dim || y_inout.len < out_dim) return false;

    let ix: usize = 0;
    while (ix < in_dim) : (ix += 1) {
        if (!isFiniteF64(x[ix])) return false;
    }
    let iy: usize = 0;
    while (iy < out_dim) : (iy += 1) {
        if (!isFiniteF64(y_inout[iy])) return false;
    }

    let k: usize = 0;
    while (k < n_adapters) : (k += 1) {
        const w = weights[k];
        if (w == 0.0) continue;
        applySingleAdapterScaled(adapters[k], w, x, y_inout, hidden_scratch);
    }

    return true;
}

fn loraMergeScaled(
    base_weight_inout: *[]f64,
    adapter: LoraAdapter,
    sign: f64,
): bool {
    if (!isValidAdapter(adapter)) return false;
    const in_dim = adapter.in_dim;
    const out_dim = adapter.out_dim;
    const rank = adapter.rank;

    if (out_dim > std.math.maxInt(usize) / in_dim) return false;
    const w_elems = out_dim * in_dim;
    if (base_weight_inout.len < w_elems) return false;

    let iw: usize = 0;
    while (iw < w_elems) : (iw += 1) {
        if (!isFiniteF64(base_weight_inout[iw])) return false;
    }

    const scale = sign * adapter.scaling();
    let o: usize = 0;
    while (o < out_dim) : (o += 1) {
        const w_row = o * in_dim;
        const b_row = o * rank;
        let j: usize = 0;
        while (j < in_dim) : (j += 1) {
            let dot: f64 = 0.0;
            let r: usize = 0;
            while (r < rank) : (r += 1) {
                dot += adapter.b_mat[b_row + r] * adapter.a_mat[r * in_dim + j];
            }
            base_weight_inout[w_row + j] += scale * dot;
        }
    }

    return true;
}

/// Hot-merges a LoRA adapter into a row-major `[out_dim, in_dim]` base weight matrix:
/// `W += (alpha / rank) * B * A`, enabling zero-overhead static inference.
/// Returns `false` on invalid adapter, undersized `base_weight_inout`, or NaN/Inf inputs.
pub fn loraMergeIntoBase(base_weight_inout: *[]f64, adapter: LoraAdapter): bool {
    return loraMergeScaled(base_weight_inout, adapter, 1.0);
}

/// Hot-unmerges a LoRA adapter from a row-major `[out_dim, in_dim]` base weight matrix:
/// `W -= (alpha / rank) * B * A`, restoring the original base weights for online adapter swapping.
/// Returns `false` on invalid adapter, undersized `base_weight_inout`, or NaN/Inf inputs.
pub fn loraUnmergeFromBase(base_weight_inout: *[]f64, adapter: LoraAdapter): bool {
    return loraMergeScaled(base_weight_inout, adapter, -1.0);
}

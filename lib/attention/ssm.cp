// Origin: Pure-CP Stdlib (AI Inference Selective State Space Model Mamba S6 & Causal Depthwise Conv1D) | Created: 2026-11-22
// Zero-heap-allocation Mamba / S6 Selective State Space Model (SSM) O(N) linear-time sequence
// inference kernels & causal depthwise 1D convolution:
//   - Numerically stable `softplus(x)` with linear pass-through for large `x`
//   - Single-step ZOH selective SSM recurrence (`ssmSelectiveStep`) with negative-real A = -exp(a_log)
//   - Full-sequence selective scan (`ssmSelectiveScan`) bit-identical to step-by-step recurrence
//   - Depthwise causal 1D convolution step (`causalConv1dStep`) with fixed-size history buffer

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Numerically stable `softplus(x) = ln(1 + exp(x))`.
/// Uses linear pass-through for `x > 35.0` to prevent `exp` overflow, and `log1p(exp(x))`
/// for negative `x` so all finite inputs (including extreme magnitudes like `+500.0` / `-500.0`)
/// stay overflow-free and accurate to f64 precision.
pub fn softplus(x: f64): f64 {
    if (std.math.isNan(x)) return x;
    if (x > 35.0) return x;
    if (x >= 0.0) {
        return x + std.math.log1p(std.math.exp(0.0 - x));
    }
    return std.math.log1p(std.math.exp(x));
}

fn selectiveStepUnchecked(
    x: []const f64,
    dt: []const f64,
    a_log: []const f64,
    b_vec: []const f64,
    c_vec: []const f64,
    d_skip: []const f64,
    use_d: bool,
    d_inner: usize,
    d_state: usize,
    state_inout: *[]f64,
    y_out: *[]f64,
) {
    let i: usize = 0;
    while (i < d_inner) : (i += 1) {
        const x_i = x[i];
        const dt_i = dt[i];
        const row_base = i * d_state;

        let y_acc: f64 = 0.0;
        let n: usize = 0;
        while (n < d_state) : (n += 1) {
            const idx = row_base + n;
            const a_bar = if (dt_i == 0.0)
                1.0
            else
                std.math.exp(dt_i * (0.0 - std.math.exp(a_log[idx])));
            const b_bar = dt_i * b_vec[n];
            const h_next = a_bar * state_inout[idx] + b_bar * x_i;
            state_inout[idx] = h_next;
            y_acc += c_vec[n] * h_next;
        }

        if (use_d) {
            y_acc += d_skip[i] * x_i;
        }
        y_out[i] = y_acc;
    }
}

/// Zero-heap-allocation Mamba / S6 single-step Selective SSM recurrence with ZOH discretization:
/// - `A[i, n] = -exp(a_log[i, n]) < 0` (guaranteed exponential decay stability)
/// - `A_bar = exp(dt[i] * A[i, n])`, `B_bar = dt[i] * b_vec[n]`
/// - `h_t[i, n] = A_bar * h_{t-1}[i, n] + B_bar * x[i]`
/// - `y_t[i] = sum_n(c_vec[n] * h_t[i, n]) + d_skip[i] * x[i]` (`d_skip` may be empty to omit skip)
///
/// Shapes:
/// - `x`, `dt`, `y_out`: `[d_inner]` (`dt[i] >= 0.0`)
/// - `b_vec`, `c_vec`: `[d_state]`
/// - `a_log`, `state_inout`: `[d_inner, d_state]` row-major
/// - `d_skip`: `[]` (optional) or `[d_inner]`
///
/// Returns `false` on zero dimensions, negative `dt`, undersized slices, or NaN/Inf inputs.
pub fn ssmSelectiveStep(
    x: []const f64,
    dt: []const f64,
    a_log: []const f64,
    b_vec: []const f64,
    c_vec: []const f64,
    d_skip: []const f64,
    d_inner: usize,
    d_state: usize,
    state_inout: *[]f64,
    y_out: *[]f64,
): bool {
    if (d_inner == 0 || d_state == 0) return false;
    if (d_inner > std.math.maxInt(usize) / d_state) return false;
    const state_elems = d_inner * d_state;

    if (x.len < d_inner || dt.len < d_inner || y_out.len < d_inner) return false;
    if (b_vec.len < d_state || c_vec.len < d_state) return false;
    if (a_log.len < state_elems || state_inout.len < state_elems) return false;

    const use_d = d_skip.len > 0;
    if (use_d && d_skip.len < d_inner) return false;

    let i: usize = 0;
    while (i < d_inner) : (i += 1) {
        if (!isFiniteF64(x[i]) || !isFiniteF64(dt[i]) || dt[i] < 0.0) return false;
        if (use_d && !isFiniteF64(d_skip[i])) return false;
    }
    let n: usize = 0;
    while (n < d_state) : (n += 1) {
        if (!isFiniteF64(b_vec[n]) || !isFiniteF64(c_vec[n])) return false;
    }
    let s: usize = 0;
    while (s < state_elems) : (s += 1) {
        if (!isFiniteF64(a_log[s]) || !isFiniteF64(state_inout[s])) return false;
    }

    selectiveStepUnchecked(
        x,
        dt,
        a_log,
        b_vec,
        c_vec,
        d_skip,
        use_d,
        d_inner,
        d_state,
        state_inout,
        y_out,
    );
    return true;
}

/// Zero-heap-allocation full-sequence Selective SSM scan over `[seq_len, d_inner]`.
/// Updates `state_inout` in place across all `seq_len` steps, enabling chunked prefill followed
/// seamlessly by autoregressive single-token `ssmSelectiveStep` decoding with bit-identical results.
///
/// Shapes:
/// - `x_seq`, `dt_seq`, `y_seq_out`: `[seq_len, d_inner]` row-major (`dt_seq >= 0.0`)
/// - `b_seq`, `c_seq`: `[seq_len, d_state]` row-major
/// - `a_log`, `state_inout`: `[d_inner, d_state]` row-major
/// - `d_skip`: `[]` (optional) or `[d_inner]`
///
/// Validation is atomic: returns `false` without mutating `state_inout` on any invalid input.
pub fn ssmSelectiveScan(
    x_seq: []const f64,
    dt_seq: []const f64,
    a_log: []const f64,
    b_seq: []const f64,
    c_seq: []const f64,
    d_skip: []const f64,
    seq_len: usize,
    d_inner: usize,
    d_state: usize,
    state_inout: *[]f64,
    y_seq_out: *[]f64,
): bool {
    if (seq_len == 0 || d_inner == 0 || d_state == 0) return false;
    if (d_inner > std.math.maxInt(usize) / d_state) return false;
    if (seq_len > std.math.maxInt(usize) / d_inner) return false;
    if (seq_len > std.math.maxInt(usize) / d_state) return false;

    const state_elems = d_inner * d_state;
    const seq_inner_elems = seq_len * d_inner;
    const seq_state_elems = seq_len * d_state;

    if (x_seq.len < seq_inner_elems || dt_seq.len < seq_inner_elems || y_seq_out.len < seq_inner_elems) {
        return false;
    }
    if (b_seq.len < seq_state_elems || c_seq.len < seq_state_elems) return false;
    if (a_log.len < state_elems || state_inout.len < state_elems) return false;

    const use_d = d_skip.len > 0;
    if (use_d && d_skip.len < d_inner) return false;

    let idx0: usize = 0;
    while (idx0 < seq_inner_elems) : (idx0 += 1) {
        if (!isFiniteF64(x_seq[idx0]) || !isFiniteF64(dt_seq[idx0]) || dt_seq[idx0] < 0.0) return false;
    }
    let idx1: usize = 0;
    while (idx1 < seq_state_elems) : (idx1 += 1) {
        if (!isFiniteF64(b_seq[idx1]) || !isFiniteF64(c_seq[idx1])) return false;
    }
    let idx2: usize = 0;
    while (idx2 < state_elems) : (idx2 += 1) {
        if (!isFiniteF64(a_log[idx2]) || !isFiniteF64(state_inout[idx2])) return false;
    }
    if (use_d) {
        let id: usize = 0;
        while (id < d_inner) : (id += 1) {
            if (!isFiniteF64(d_skip[id])) return false;
        }
    }

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        const x_off = t * d_inner;
        const bc_off = t * d_state;
        let y_slice = y_seq_out[x_off .. x_off + d_inner];
        selectiveStepUnchecked(
            x_seq[x_off .. x_off + d_inner],
            dt_seq[x_off .. x_off + d_inner],
            a_log,
            b_seq[bc_off .. bc_off + d_state],
            c_seq[bc_off .. bc_off + d_state],
            d_skip,
            use_d,
            d_inner,
            d_state,
            state_inout,
            y_slice,
        );
    }

    return true;
}

/// Zero-heap-allocation Mamba causal depthwise 1D convolution single-step kernel.
///
/// For each channel `i in 0..d_inner`, computes:
/// `y_out[i] = bias[i] + sum_{k=0}^{K-2} conv_state[i, k] * weight[i, k] + x[i] * weight[i, K-1]`
/// and shifts `conv_state[i, :]` left by 1, appending `x[i]` at slot `K - 2`.
///
/// Shapes:
/// - `x`, `y_out`: `[d_inner]`
/// - `weight`: `[d_inner, kernel_size]` row-major (oldest tap at index `0`, newest tap at `kernel_size - 1`)
/// - `bias`: `[]` (optional) or `[d_inner]`
/// - `conv_state_inout`: `[d_inner, kernel_size - 1]` row-major (may be empty when `kernel_size == 1`)
///
/// Returns `false` on zero dimensions, undersized buffers, or NaN/Inf inputs.
pub fn causalConv1dStep(
    x: []const f64,
    weight: []const f64,
    bias: []const f64,
    d_inner: usize,
    kernel_size: usize,
    conv_state_inout: *[]f64,
    y_out: *[]f64,
): bool {
    if (d_inner == 0 || kernel_size == 0) return false;
    if (d_inner > std.math.maxInt(usize) / kernel_size) return false;

    const w_elems = d_inner * kernel_size;
    const hist_len = kernel_size - 1;
    const state_elems = d_inner * hist_len;

    if (x.len < d_inner || y_out.len < d_inner || weight.len < w_elems) return false;
    if (conv_state_inout.len < state_elems) return false;

    const use_bias = bias.len > 0;
    if (use_bias && bias.len < d_inner) return false;

    let ix: usize = 0;
    while (ix < d_inner) : (ix += 1) {
        if (!isFiniteF64(x[ix])) return false;
        if (use_bias && !isFiniteF64(bias[ix])) return false;
    }
    let iw: usize = 0;
    while (iw < w_elems) : (iw += 1) {
        if (!isFiniteF64(weight[iw])) return false;
    }
    let is: usize = 0;
    while (is < state_elems) : (is += 1) {
        if (!isFiniteF64(conv_state_inout[is])) return false;
    }

    let i: usize = 0;
    while (i < d_inner) : (i += 1) {
        const w_base = i * kernel_size;
        const s_base = i * hist_len;
        let acc: f64 = if (use_bias) bias[i] else 0.0;

        let k: usize = 0;
        while (k < hist_len) : (k += 1) {
            acc += conv_state_inout[s_base + k] * weight[w_base + k];
        }
        acc += x[i] * weight[w_base + hist_len];
        y_out[i] = acc;

        if (hist_len > 0) {
            let k_shift: usize = 0;
            while (k_shift + 1 < hist_len) : (k_shift += 1) {
                conv_state_inout[s_base + k_shift] = conv_state_inout[s_base + k_shift + 1];
            }
            conv_state_inout[s_base + hist_len - 1] = x[i];
        }
    }

    return true;
}

// Origin: Pure-CP Stdlib (Zero-Alloc SIMD @dot/@fma RMSNorm, LayerNorm, QK-Norm & SwiGLU/GeGLU) | Created: 2026-11-23
// Pure CP zero-heap-allocation, hardware-accelerated Transformer Normalization & Gated Activation Kernels:
//   - RMSNorm family powered by native `@dot` / `@fma`:
//       * `rmsNormF32`, `rmsNormInPlaceF32`, `rmsNormUnitOffsetF32` (Gemma-2/3 `1 + w` offset)
//       * `fusedAddRmsNormF32` (Llama/Qwen fused residual add + RMSNorm in minimal passes)
//       * `qkRmsNormHeadsInPlaceF32` (Qwen3/Gemma2 multi-head QK-Norm with full GQA support)
//       * `batchRmsNormF32` (row-major matrix batch normalization)
//   - LayerNorm family:
//       * `layerNormF32` (numerically stable centered variance + optional affine bias via `@fma`)
//   - Gated MLP Activation family:
//       * `siluF32`, `geluTanhF32` (Hendrycks tanh approximation, overflow-safe)
//       * `swigluF32`, `gegluF32`, `swigluInterleavedF32`, `swigluSplitF32`

// ============================================================================
// 1. Constants, Error Set & Configuration
// ============================================================================

/// Default numerical stability epsilon for RMSNorm / LayerNorm (`1e-6`).
pub const RMSNORM_DEFAULT_EPS: f32 = 1.0e-6;

/// Lightweight validation error set for normalization and activation kernels.
pub err NormKernelError {
    DimMismatch,
    EmptyInput,
    InvalidEps,
}

/// Configuration for RMSNorm / LayerNorm kernels.
pub struct NormConfig {
    /// Stability epsilon added to mean square / variance (must be finite and `> 0`).
    eps: f32 = RMSNORM_DEFAULT_EPS;
    /// When true, applies Gemma-2/3 unit-offset scaling `(1.0 + weight[i])`.
    unit_offset: bool = false;

    /// Return standard Llama/Qwen default configuration (`eps = 1e-6, unit_offset = false`).
    static fn defaults(): NormConfig {
        return NormConfig{};
    }

    /// Return Gemma-2/3 configuration (`eps = 1e-6, unit_offset = true`).
    static fn gemmaDefaults(): NormConfig {
        return NormConfig{ eps: RMSNORM_DEFAULT_EPS, unit_offset: true };
    }

    /// Return true if `eps` is finite and strictly positive.
    fn isValid(): bool {
        return !(eps != eps) && eps > 0.0 && eps < 1.0e30;
    }
}

fn validateEps(eps: f32): bool {
    return !(eps != eps) && eps > 0.0 && eps < 1.0e30;
}

// ============================================================================
// 2. RMSNorm Family (`@dot` / `@fma` Accelerated)
// ============================================================================

/// Compute `out[i] = (x[i] / sqrt(mean(x^2) + eps)) * weight[i]` using native `@dot`.
/// Returns the number of elements written (`x.len`).
pub fn rmsNormF32(
    x: []const f32,
    weight: []const f32,
    eps: f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (x.len == 0) return NormKernelError.EmptyInput;
    if (weight.len != x.len || out.len != x.len) return NormKernelError.DimMismatch;

    const sum_sq: f32 = @dot(x, x);
    const mean_sq: f32 = sum_sq / @as(f32, @floatFromInt(x.len));
    const inv_rms: f32 = 1.0 / @sqrt(mean_sq + eps);

    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        out[i] = (x[i] * inv_rms) * weight[i];
    }
    return x.len;
}

/// In-place RMSNorm: `x[i] = (x[i] / sqrt(mean(x^2) + eps)) * weight[i]`.
pub fn rmsNormInPlaceF32(
    x: *[]f32,
    weight: []const f32,
    eps: f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (x.len == 0) return NormKernelError.EmptyInput;
    if (weight.len != x.len) return NormKernelError.DimMismatch;

    const x_ro: []const f32 = x[0..];
    const sum_sq: f32 = @dot(x_ro, x_ro);
    const mean_sq: f32 = sum_sq / @as(f32, @floatFromInt(x.len));
    const inv_rms: f32 = 1.0 / @sqrt(mean_sq + eps);

    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        x[i] = (x[i] * inv_rms) * weight[i];
    }
    return x.len;
}

/// Gemma-2/3 unit-offset RMSNorm: `out[i] = (x[i] / sqrt(mean(x^2) + eps)) * (1.0 + weight[i])`.
pub fn rmsNormUnitOffsetF32(
    x: []const f32,
    weight: []const f32,
    eps: f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (x.len == 0) return NormKernelError.EmptyInput;
    if (weight.len != x.len || out.len != x.len) return NormKernelError.DimMismatch;

    const sum_sq: f32 = @dot(x, x);
    const mean_sq: f32 = sum_sq / @as(f32, @floatFromInt(x.len));
    const inv_rms: f32 = 1.0 / @sqrt(mean_sq + eps);

    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const scaled: f32 = x[i] * inv_rms;
        out[i] = @fma(scaled, weight[i], scaled);
    }
    return x.len;
}

/// Dispatch RMSNorm using `NormConfig` (`unit_offset` selects standard vs Gemma `1 + w`).
pub fn rmsNormWithConfigF32(
    x: []const f32,
    weight: []const f32,
    cfg: NormConfig,
    out: *[]f32,
): usize ! NormKernelError {
    if (cfg.unit_offset) {
        return rmsNormUnitOffsetF32(x, weight, cfg.eps, out);
    }
    return rmsNormF32(x, weight, cfg.eps, out);
}

/// Fused Residual Add + RMSNorm (Llama/Qwen architecture):
///   1. Updates `residual[i] += x[i]` in-place while accumulating `sum(residual[i]^2)` via `@fma`
///   2. Computes `out[i] = (residual[i] * inv_rms) * weight[i]`
/// Eliminates an extra global memory pass over the residual stream.
pub fn fusedAddRmsNormF32(
    residual: *[]f32,
    x: []const f32,
    weight: []const f32,
    eps: f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (x.len == 0) return NormKernelError.EmptyInput;
    if (residual.len != x.len || weight.len != x.len || out.len != x.len) {
        return NormKernelError.DimMismatch;
    }

    let sum_sq: f32 = 0.0;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const r: f32 = residual[i] + x[i];
        residual[i] = r;
        sum_sq = @fma(r, r, sum_sq);
    }

    const mean_sq: f32 = sum_sq / @as(f32, @floatFromInt(x.len));
    const inv_rms: f32 = 1.0 / @sqrt(mean_sq + eps);

    i = 0;
    while (i < x.len) : (i += 1) {
        out[i] = (residual[i] * inv_rms) * weight[i];
    }
    return x.len;
}

/// Multi-head QK-RMSNorm in-place for Qwen3 / Gemma2 with Grouped Query Attention (GQA).
/// Independently normalizes each of `num_q_heads` Q heads (`head_dim` floats each) with `q_weight`
/// and each of `num_kv_heads` K heads (`head_dim` floats each) with `k_weight`.
/// Returns total number of heads normalized (`num_q_heads + num_kv_heads`).
pub fn qkRmsNormHeadsInPlaceF32(
    q: *[]f32,
    k: *[]f32,
    q_weight: []const f32,
    k_weight: []const f32,
    num_q_heads: usize,
    num_kv_heads: usize,
    head_dim: usize,
    eps: f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (num_q_heads == 0 || num_kv_heads == 0 || head_dim == 0) {
        return NormKernelError.EmptyInput;
    }
    if (q_weight.len != head_dim || k_weight.len != head_dim) {
        return NormKernelError.DimMismatch;
    }
    if (q.len != num_q_heads * head_dim || k.len != num_kv_heads * head_dim) {
        return NormKernelError.DimMismatch;
    }

    let h: usize = 0;
    while (h < num_q_heads) : (h += 1) {
        const off = h * head_dim;
        let head_s: []f32 = q[off .. off + head_dim];
        * = rmsNormInPlaceF32(*head_s, q_weight, eps)?;
    }

    h = 0;
    while (h < num_kv_heads) : (h += 1) {
        const off = h * head_dim;
        let head_s: []f32 = k[off .. off + head_dim];
        * = rmsNormInPlaceF32(*head_s, k_weight, eps)?;
    }

    return num_q_heads + num_kv_heads;
}

/// Batch row-major RMSNorm over `rows` vectors of length `dim`.
/// Returns total rows normalized.
pub fn batchRmsNormF32(
    x_rows: []const f32,
    rows: usize,
    dim: usize,
    weight: []const f32,
    eps: f32,
    out_rows: *[]f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (rows == 0 || dim == 0) return NormKernelError.EmptyInput;
    if (weight.len != dim || x_rows.len != rows * dim || out_rows.len != rows * dim) {
        return NormKernelError.DimMismatch;
    }

    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const off = r * dim;
        let out_row: []f32 = out_rows[off .. off + dim];
        * = rmsNormF32(x_rows[off .. off + dim], weight, eps, *out_row)?;
    }
    return rows;
}

// ============================================================================
// 3. LayerNorm Family
// ============================================================================

/// Standard affine LayerNorm with numerically stable centered variance and optional `bias`:
///   `out[i] = ((x[i] - mean) / sqrt(var + eps)) * weight[i] + bias[i]`
pub fn layerNormF32(
    x: []const f32,
    weight: []const f32,
    bias: ([]const f32)?,
    eps: f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (!validateEps(eps)) return NormKernelError.InvalidEps;
    if (x.len == 0) return NormKernelError.EmptyInput;
    if (weight.len != x.len || out.len != x.len) return NormKernelError.DimMismatch;
    if (b := bias) {
        if (b.len != x.len) return NormKernelError.DimMismatch;
    }

    const n_f32: f32 = @as(f32, @floatFromInt(x.len));
    let sum: f32 = 0.0;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        sum += x[i];
    }
    const mean: f32 = sum / n_f32;

    let var_sum: f32 = 0.0;
    i = 0;
    while (i < x.len) : (i += 1) {
        const d: f32 = x[i] - mean;
        var_sum = @fma(d, d, var_sum);
    }
    const inv_std: f32 = 1.0 / @sqrt((var_sum / n_f32) + eps);

    if (b := bias) {
        i = 0;
        while (i < x.len) : (i += 1) {
            const norm: f32 = (x[i] - mean) * inv_std;
            out[i] = @fma(norm, weight[i], b[i]);
        }
    } else {
        i = 0;
        while (i < x.len) : (i += 1) {
            out[i] = ((x[i] - mean) * inv_std) * weight[i];
        }
    }
    return x.len;
}

// ============================================================================
// 4. Gated MLP Activation Family (SiLU, GeLU-Tanh, SwiGLU, GeGLU)
// ============================================================================

/// Overflow-safe SiLU (Swish-1) activation: `v / (1 + exp(-v))`.
pub fn siluF32(v: f32): f32 {
    if (v >= 0.0) {
        return v / (1.0 + @exp(-v));
    }
    const e: f32 = @exp(v);
    return (v * e) / (1.0 + e);
}

fn stableTanhF32(z: f32): f32 {
    if (z > 10.0) return 1.0;
    if (z < -10.0) return -1.0;
    if (z >= 0.0) {
        const e = @exp(-2.0 * z);
        return (1.0 - e) / (1.0 + e);
    }
    const e = @exp(2.0 * z);
    return (e - 1.0) / (e + 1.0);
}

/// Hendrycks-Gimpel tanh GeLU approximation:
/// `0.5 * v * (1 + tanh(sqrt(2 / pi) * (v + 0.044715 * v^3)))`.
pub fn geluTanhF32(v: f32): f32 {
    const SQRT_2_OVER_PI: f32 = 0.7978845608;
    const COEFF: f32 = 0.044715;
    const v3: f32 = v * v * v;
    const inner: f32 = SQRT_2_OVER_PI * @fma(COEFF, v3, v);
    return 0.5 * v * (1.0 + stableTanhF32(inner));
}

/// Elementwise SwiGLU gated activation: `out[i] = silu(gate[i]) * up[i]`.
pub fn swigluF32(
    gate: []const f32,
    up: []const f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (gate.len == 0) return NormKernelError.EmptyInput;
    if (up.len != gate.len || out.len != gate.len) return NormKernelError.DimMismatch;

    let i: usize = 0;
    while (i < gate.len) : (i += 1) {
        out[i] = siluF32(gate[i]) * up[i];
    }
    return gate.len;
}

/// Elementwise GeGLU gated activation: `out[i] = geluTanh(gate[i]) * up[i]`.
pub fn gegluF32(
    gate: []const f32,
    up: []const f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (gate.len == 0) return NormKernelError.EmptyInput;
    if (up.len != gate.len || out.len != gate.len) return NormKernelError.DimMismatch;

    let i: usize = 0;
    while (i < gate.len) : (i += 1) {
        out[i] = geluTanhF32(gate[i]) * up[i];
    }
    return gate.len;
}

/// Fused interleaved-projection SwiGLU: `gate_up` layout is `[gate0, up0, gate1, up1, ...]`.
/// Requires `gate_up.len == 2 * out.len`.
pub fn swigluInterleavedF32(
    gate_up: []const f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (out.len == 0) return NormKernelError.EmptyInput;
    if (gate_up.len != out.len * 2) return NormKernelError.DimMismatch;

    let i: usize = 0;
    while (i < out.len) : (i += 1) {
        const g: f32 = gate_up[i * 2];
        const u: f32 = gate_up[i * 2 + 1];
        out[i] = siluF32(g) * u;
    }
    return out.len;
}

/// Fused split-projection SwiGLU: `gate_up` layout is `[gate[0..d], up[0..d]]`.
/// Requires `gate_up.len == 2 * out.len`.
pub fn swigluSplitF32(
    gate_up: []const f32,
    out: *[]f32,
): usize ! NormKernelError {
    if (out.len == 0) return NormKernelError.EmptyInput;
    const d = out.len;
    if (gate_up.len != d * 2) return NormKernelError.DimMismatch;

    let i: usize = 0;
    while (i < d) : (i += 1) {
        out[i] = siluF32(gate_up[i]) * gate_up[d + i];
    }
    return d;
}

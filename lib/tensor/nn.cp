// Source: cp/lib/tensor/nn.cp (LLM Transformer Core Operators)
// Numerically-stable row-wise Softmax, RMSNorm, LayerNorm, SiLU, GELU, and Rotary Position Embedding (RoPE).

const std = @import("std");
import { Tensor, TensorError } from "./core.cp";

/// Numerically-stable Softmax along the last axis for 1D `[D]` or 2D `[R, D]` tensors.
pub fn softmaxRows(alloc: std.mem.Allocator, x: Tensor): Tensor! {
    if (x.ndim != 1 && x.ndim != 2) {
        return TensorError.DimensionMismatch;
    }
    let out = x.contiguous(alloc)?;
    const rows: usize = if (out.ndim == 1) 1 else out.shape[0];
    const cols: usize = if (out.ndim == 1) out.shape[0] else out.shape[1];
    if (cols == 0) {
        out.deinit(alloc);
        return TensorError.EmptyTensor;
    }

    let r: usize = 0;
    while (r < rows) {
        const base = r * cols;
        let max_val = out.data[base];
        let c: usize = 1;
        while (c < cols) {
            if (out.data[base + c] > max_val) {
                max_val = out.data[base + c];
            }
            c += 1;
        }

        let sum_exp: f32 = 0.0;
        c = 0;
        while (c < cols) {
            const e = @exp(out.data[base + c] - max_val);
            out.data[base + c] = e;
            sum_exp += e;
            c += 1;
        }

        if (sum_exp > 0.0) {
            const inv = 1.0 / sum_exp;
            c = 0;
            while (c < cols) {
                out.data[base + c] *= inv;
                c += 1;
            }
        }
        r += 1;
    }
    return out;
}

/// Root Mean Square Layer Normalization: `y = (x / sqrt(mean(x^2) + eps)) * weight`.
pub fn rmsNorm(alloc: std.mem.Allocator, x: Tensor, weight: Tensor, eps: f32): Tensor! {
    if (x.ndim != 1 && x.ndim != 2) {
        return TensorError.DimensionMismatch;
    }
    if (weight.ndim != 1) {
        return TensorError.DimensionMismatch;
    }
    const cols: usize = if (x.ndim == 1) x.shape[0] else x.shape[1];
    if (weight.shape[0] != cols) {
        return TensorError.DimensionMismatch;
    }

    let out = x.contiguous(alloc)?;
    const rows: usize = if (out.ndim == 1) 1 else out.shape[0];
    const w_off = weight.offset;
    const w_stride = weight.strides[0];
    const inv_cols: f32 = 1.0 / @as(f32, @floatFromInt(cols));

    let r: usize = 0;
    while (r < rows) {
        const base = r * cols;
        let sum_sq: f32 = 0.0;
        let c: usize = 0;
        while (c < cols) {
            const v = out.data[base + c];
            sum_sq += v * v;
            c += 1;
        }
        const rms = @sqrt(sum_sq * inv_cols + eps);
        const inv_rms = 1.0 / rms;
        c = 0;
        while (c < cols) {
            const w = weight.data[w_off + c * w_stride];
            out.data[base + c] = out.data[base + c] * inv_rms * w;
            c += 1;
        }
        r += 1;
    }
    return out;
}

/// Standard Layer Normalization: `y = ((x - mean) / sqrt(var + eps)) * weight + bias`.
pub fn layerNorm(alloc: std.mem.Allocator, x: Tensor, weight: Tensor, bias: Tensor, eps: f32): Tensor! {
    if (x.ndim != 1 && x.ndim != 2) {
        return TensorError.DimensionMismatch;
    }
    if (weight.ndim != 1 || bias.ndim != 1) {
        return TensorError.DimensionMismatch;
    }
    const cols: usize = if (x.ndim == 1) x.shape[0] else x.shape[1];
    if (weight.shape[0] != cols || bias.shape[0] != cols) {
        return TensorError.DimensionMismatch;
    }

    let out = x.contiguous(alloc)?;
    const rows: usize = if (out.ndim == 1) 1 else out.shape[0];
    const w_off = weight.offset;
    const w_stride = weight.strides[0];
    const b_off = bias.offset;
    const b_stride = bias.strides[0];
    const inv_cols: f32 = 1.0 / @as(f32, @floatFromInt(cols));

    let r: usize = 0;
    while (r < rows) {
        const base = r * cols;
        let sum: f32 = 0.0;
        let c: usize = 0;
        while (c < cols) {
            sum += out.data[base + c];
            c += 1;
        }
        const mean = sum * inv_cols;

        let var_sum: f32 = 0.0;
        c = 0;
        while (c < cols) {
            const diff = out.data[base + c] - mean;
            var_sum += diff * diff;
            c += 1;
        }
        const inv_std = 1.0 / @sqrt(var_sum * inv_cols + eps);

        c = 0;
        while (c < cols) {
            const w = weight.data[w_off + c * w_stride];
            const b = bias.data[b_off + c * b_stride];
            out.data[base + c] = ((out.data[base + c] - mean) * inv_std) * w + b;
            c += 1;
        }
        r += 1;
    }
    return out;
}

/// SiLU (Swish) activation: `x / (1.0 + exp(-x))`.
pub fn silu(alloc: std.mem.Allocator, x: Tensor): Tensor! {
    let out = x.contiguous(alloc)?;
    let i: usize = 0;
    while (i < out.data.len) {
        const v = out.data[i];
        out.data[i] = v / (1.0 + @exp(-v));
        i += 1;
    }
    return out;
}

/// GELU activation using the tanh approximation:
/// `0.5 * x * (1 + tanh(sqrt(2 / pi) * (x + 0.044715 * x^3)))`.
pub fn gelu(alloc: std.mem.Allocator, x: Tensor): Tensor! {
    let out = x.contiguous(alloc)?;
    const k_sqrt_2_over_pi: f32 = 0.7978845608;
    const k_coeff: f32 = 0.044715;
    let i: usize = 0;
    while (i < out.data.len) {
        const v = out.data[i];
        const inner = k_sqrt_2_over_pi * (v + k_coeff * v * v * v);
        out.data[i] = 0.5 * v * (1.0 + std.math.tanh(inner));
        i += 1;
    }
    return out;
}

/// Rotary Position Embedding (RoPE) applied to pairs `(2k, 2k+1)` along the last dimension.
/// Supports 1D `[HeadDim]` or 2D `[Rows, HeadDim]` where `HeadDim` is even.
pub fn applyRope(alloc: std.mem.Allocator, x: Tensor, pos: usize, theta_base: f32): Tensor! {
    if (x.ndim != 1 && x.ndim != 2) {
        return TensorError.DimensionMismatch;
    }
    const head_dim: usize = if (x.ndim == 1) x.shape[0] else x.shape[1];
    if (head_dim == 0 || (head_dim % 2) != 0) {
        return TensorError.DimensionMismatch;
    }

    let out = x.contiguous(alloc)?;
    const rows: usize = if (out.ndim == 1) 1 else out.shape[0];
    const half_dim = head_dim / 2;
    const pos_f: f32 = @as(f32, @floatFromInt(pos));
    const dim_f: f32 = @as(f32, @floatFromInt(head_dim));

    let r: usize = 0;
    while (r < rows) {
        const base = r * head_dim;
        let k: usize = 0;
        while (k < half_dim) {
            const exponent = @as(f32, @floatFromInt(2 * k)) / dim_f;
            const freq = 1.0 / std.math.pow(f32, theta_base, exponent);
            const angle = pos_f * freq;
            const cos_a = @cos(angle);
            const sin_a = @sin(angle);

            const x0 = out.data[base + 2 * k];
            const x1 = out.data[base + 2 * k + 1];
            out.data[base + 2 * k] = x0 * cos_a - x1 * sin_a;
            out.data[base + 2 * k + 1] = x0 * sin_a + x1 * cos_a;
            k += 1;
        }
        r += 1;
    }
    return out;
}

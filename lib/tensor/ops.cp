// Source: cp/lib/tensor/ops.cp (Broadcasting Elementwise Operations & Strided MatMul)
// Broadcasted add/mul, scalar scaling, and strided-aware 2D matrix multiplication.

const std = @import("std");
import { Tensor, TensorError, MAX_TENSOR_DIMS } from "./core.cp";

fn readStridedAt(t: Tensor, flat_coords: [4]usize, out_ndim: usize): f32 {
    if (t.ndim == 1 && t.shape[0] == 1) {
        return t.data[t.offset];
    }
    let idx: usize = t.offset;
    const pad = out_ndim - t.ndim;
    let d: usize = 0;
    while (d < t.ndim) {
        const coord = flat_coords[pad + d];
        const c = if (t.shape[d] == 1) 0 else coord;
        idx += c * t.strides[d];
        d += 1;
    }
    return t.data[idx];
}

fn resolveBroadcastShape(a: Tensor, b: Tensor, out_shape: *[4]usize): usize! {
    if (a.ndim == 0 || b.ndim == 0) {
        return TensorError.EmptyTensor;
    }
    const out_ndim = if (a.ndim >= b.ndim) a.ndim else b.ndim;
    if (out_ndim > MAX_TENSOR_DIMS) {
        return TensorError.InvalidShape;
    }
    const pad_a = out_ndim - a.ndim;
    const pad_b = out_ndim - b.ndim;
    let d: usize = 0;
    while (d < out_ndim) {
        const da = if (d < pad_a) 1 else a.shape[d - pad_a];
        const db = if (d < pad_b) 1 else b.shape[d - pad_b];
        if (da != db && da != 1 && db != 1) {
            return TensorError.DimensionMismatch;
        }
        out_shape[d] = if (da >= db) da else db;
        d += 1;
    }
    return out_ndim;
}

fn broadcastBinary(alloc: std.mem.Allocator, a: Tensor, b: Tensor, is_mul: bool): Tensor! {
    let out_shape: [4]usize = [0, 0, 0, 0];
    const out_ndim = resolveBroadcastShape(a, b, &out_shape)?;
    let out = Tensor.zeros(alloc, out_shape[0..out_ndim])?;
    const total = out.numel();

    let coords: [4]usize = [0, 0, 0, 0];
    let i: usize = 0;
    while (i < total) {
        const va = readStridedAt(a, coords, out_ndim);
        const vb = readStridedAt(b, coords, out_ndim);
        out.data[i] = if (is_mul) (va * vb) else (va + vb);
        i += 1;

        let carry = out_ndim;
        while (carry > 0) {
            carry -= 1;
            coords[carry] += 1;
            if (coords[carry] < out_shape[carry]) {
                break;
            }
            coords[carry] = 0;
        }
    }
    return out;
}

/// Broadcasted element-wise addition (supports scalar, row-vector on 2D matrix, and same-shape tensors).
pub fn add(alloc: std.mem.Allocator, a: Tensor, b: Tensor): Tensor! {
    return broadcastBinary(alloc, a, b, false);
}

/// Broadcasted element-wise multiplication (supports scalar, row-vector on 2D matrix, and same-shape tensors).
pub fn mul(alloc: std.mem.Allocator, a: Tensor, b: Tensor): Tensor! {
    return broadcastBinary(alloc, a, b, true);
}

/// Multiplies every element of `a` by `factor`, returning a new contiguous Tensor.
pub fn scale(alloc: std.mem.Allocator, a: Tensor, factor: f32): Tensor! {
    let out = a.contiguous(alloc)?;
    let i: usize = 0;
    while (i < out.data.len) {
        out.data[i] *= factor;
        i += 1;
    }
    return out;
}

/// 2D Matrix Multiplication `[M, K] x [K, N] -> [M, N]`.
/// Supports both contiguous and strided (e.g. transposed) 2D tensors without intermediate copies.
pub fn matmul(alloc: std.mem.Allocator, a: Tensor, b: Tensor): Tensor! {
    if (a.ndim != 2 || b.ndim != 2) {
        return TensorError.DimensionMismatch;
    }
    const m = a.shape[0];
    const k = a.shape[1];
    const kb = b.shape[0];
    const n = b.shape[1];
    if (k != kb) {
        return TensorError.DimensionMismatch;
    }
    if (m == 0 || k == 0 || n == 0) {
        return TensorError.EmptyTensor;
    }

    const out_shape: [2]usize = [m, n];
    let out = Tensor.zeros(alloc, &out_shape)?;

    const a_s0 = a.strides[0];
    const a_s1 = a.strides[1];
    const b_s0 = b.strides[0];
    const b_s1 = b.strides[1];
    const a_off = a.offset;
    const b_off = b.offset;

    let i: usize = 0;
    while (i < m) {
        const a_row = a_off + i * a_s0;
        const out_row = i * n;
        let kk: usize = 0;
        while (kk < k) {
            const a_val = a.data[a_row + kk * a_s1];
            const b_row = b_off + kk * b_s0;
            let j: usize = 0;
            while (j < n) {
                out.data[out_row + j] += a_val * b.data[b_row + j * b_s1];
                j += 1;
            }
            kk += 1;
        }
        i += 1;
    }
    return out;
}

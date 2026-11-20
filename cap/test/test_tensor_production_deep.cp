// Source: cp/cap/test/test_tensor_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:tensor & cap.tensor

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_TENSOR_DIMS,
    TensorError,
    Tensor,
    add,
    mul,
    scale,
    matmul,
    softmaxRows,
    rmsNorm,
    layerNorm,
    silu,
    gelu,
    applyRope,
} from "cap:tensor";

const EPS: f32 = 0.0001;

fn approxF32(a: f32, b: f32, eps: f32): bool {
    return @abs(a - b) <= eps;
}

fn testViewsTransposeSliceReshape(): void! {
    const alloc = std.testing.allocator;
    expect_eq(MAX_TENSOR_DIMS, 4);

    const shape_2x3: [2]usize = [2, 3];
    const raw: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let t = Tensor.fromSlice(alloc, &shape_2x3, &raw)?;
    defer t.deinit(alloc);

    expect_eq(t.numel(), 6);
    expect_eq(t.isContiguous(), true);

    const idx01: [2]usize = [0, 1];
    expect_eq(approxF32(t.at(&idx01)?, 2.0, EPS), true);

    // Zero-copy transpose2d: [2, 3] -> [3, 2]
    let t_t = t.transpose2d()?;
    expect_eq(t_t.owns_data, false);
    expect_eq(t_t.shape[0], 3);
    expect_eq(t_t.shape[1], 2);
    expect_eq(t_t.isContiguous(), false);

    const idx10: [2]usize = [1, 0];
    const idx21: [2]usize = [2, 1];
    expect_eq(approxF32(t_t.at(&idx10)?, 2.0, EPS), true);
    expect_eq(approxF32(t_t.at(&idx21)?, 6.0, EPS), true);

    // Non-contiguous reshape must return NonContiguous error
    const shape_6: [1]usize = [6];
    let caught_non_contig = false;
    zig {
        if (t_t.reshape(alloc, &shape_6)) |_| {
            caught_non_contig = false;
        } else |err| {
            caught_non_contig = (err == TensorError.NonContiguous);
        }
    }
    expect_eq(caught_non_contig, true);

    // Contiguous materialization makes it compact and reshapable
    let t_c = t_t.contiguous(alloc)?;
    defer t_c.deinit(alloc);
    expect_eq(t_c.isContiguous(), true);
    expect_eq(approxF32(t_c.data[0], 1.0, EPS), true);
    expect_eq(approxF32(t_c.data[1], 4.0, EPS), true);
    expect_eq(approxF32(t_c.data[2], 2.0, EPS), true);
    expect_eq(approxF32(t_c.data[3], 5.0, EPS), true);

    let r_6 = t_c.reshape(alloc, &shape_6)?;
    expect_eq(r_6.ndim, 1);
    expect_eq(r_6.shape[0], 6);

    // Zero-copy sliceAxis along axis 1: columns [1, 3) -> [2, 2]
    let sliced = t.sliceAxis(1, 1, 3)?;
    expect_eq(sliced.owns_data, false);
    expect_eq(sliced.shape[0], 2);
    expect_eq(sliced.shape[1], 2);
    const s00: [2]usize = [0, 0];
    const s11: [2]usize = [1, 1];
    expect_eq(approxF32(sliced.at(&s00)?, 2.0, EPS), true);
    expect_eq(approxF32(sliced.at(&s11)?, 6.0, EPS), true);
}

fn testMatmulAndDimensionMismatch(): void! {
    const alloc = std.testing.allocator;

    const s_a: [2]usize = [2, 3];
    const d_a: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let a = Tensor.fromSlice(alloc, &s_a, &d_a)?;
    defer a.deinit(alloc);

    // Multiply A [2, 3] by A^T [3, 2] (strided view) -> [2, 2]
    let a_t = a.transpose2d()?;
    let c = matmul(alloc, a, a_t)?;
    defer c.deinit(alloc);

    expect_eq(c.shape[0], 2);
    expect_eq(c.shape[1], 2);
    // [1*1+2*2+3*3=14, 1*4+2*5+3*6=32; 32, 4*4+5*5+6*6=77]
    expect_eq(approxF32(c.data[0], 14.0, EPS), true);
    expect_eq(approxF32(c.data[1], 32.0, EPS), true);
    expect_eq(approxF32(c.data[2], 32.0, EPS), true);
    expect_eq(approxF32(c.data[3], 77.0, EPS), true);

    // DimensionMismatch check: [2, 3] x [2, 3]
    let caught_mismatch = false;
    zig {
        if (matmul(alloc, a, a)) |*bad| {
            var tmp = bad.*;
            tmp.deinit(alloc);
            caught_mismatch = false;
        } else |err| {
            caught_mismatch = (err == TensorError.DimensionMismatch);
        }
    }
    expect_eq(caught_mismatch, true);
}

fn testSoftmaxStabilityAndBroadcast(): void! {
    const alloc = std.testing.allocator;

    // Large logits to verify max-subtraction stability (no NaN/Inf)
    const s_logits: [2]usize = [2, 3];
    const d_logits: [6]f32 = [1000.0, 1001.0, 1002.0, -50.0, -50.0, -50.0];
    let logits = Tensor.fromSlice(alloc, &s_logits, &d_logits)?;
    defer logits.deinit(alloc);

    let probs = softmaxRows(alloc, logits)?;
    defer probs.deinit(alloc);

    const row0_sum = probs.data[0] + probs.data[1] + probs.data[2];
    const row1_sum = probs.data[3] + probs.data[4] + probs.data[5];
    expect_eq(approxF32(row0_sum, 1.0, EPS), true);
    expect_eq(approxF32(row1_sum, 1.0, EPS), true);
    expect_eq(approxF32(probs.data[3], 1.0 / 3.0, EPS), true);
    expect_eq(probs.data[2] > probs.data[1] && probs.data[1] > probs.data[0], true);

    // Broadcast add: [2, 3] + [3] row vector
    const s_m: [2]usize = [2, 3];
    const d_m: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let m = Tensor.fromSlice(alloc, &s_m, &d_m)?;
    defer m.deinit(alloc);

    const s_v: [1]usize = [3];
    const d_v: [3]f32 = [10.0, 20.0, 30.0];
    let v = Tensor.fromSlice(alloc, &s_v, &d_v)?;
    defer v.deinit(alloc);

    let added = add(alloc, m, v)?;
    defer added.deinit(alloc);
    expect_eq(approxF32(added.data[0], 11.0, EPS), true);
    expect_eq(approxF32(added.data[5], 36.0, EPS), true);

    // Broadcast mul with scalar [1]
    const s_sc: [1]usize = [1];
    let sc = Tensor.full(alloc, &s_sc, 2.0)?;
    defer sc.deinit(alloc);

    let scaled = mul(alloc, added, sc)?;
    defer scaled.deinit(alloc);
    expect_eq(approxF32(scaled.data[0], 22.0, EPS), true);
    expect_eq(approxF32(scaled.data[5], 72.0, EPS), true);
}

fn testTransformerOperators(): void! {
    const alloc = std.testing.allocator;

    // 1. RMSNorm: x = [3.0, 4.0], weight = [1.0, 2.0], eps = 0.0 -> rms = sqrt((9+16)/2) = sqrt(12.5)
    const s_x: [1]usize = [2];
    const d_x: [2]f32 = [3.0, 4.0];
    let x = Tensor.fromSlice(alloc, &s_x, &d_x)?;
    defer x.deinit(alloc);

    const d_w: [2]f32 = [1.0, 2.0];
    let w = Tensor.fromSlice(alloc, &s_x, &d_w)?;
    defer w.deinit(alloc);

    let rms_out = rmsNorm(alloc, x, w, 0.0)?;
    defer rms_out.deinit(alloc);
    const expected_rms = @sqrt(12.5);
    expect_eq(approxF32(rms_out.data[0], (3.0 / expected_rms) * 1.0, EPS), true);
    expect_eq(approxF32(rms_out.data[1], (4.0 / expected_rms) * 2.0, EPS), true);

    // 2. LayerNorm: x = [1.0, 3.0], mean=2.0, var=1.0 -> norm=[-1.0, 1.0]
    const d_ln: [2]f32 = [1.0, 3.0];
    let ln_in = Tensor.fromSlice(alloc, &s_x, &d_ln)?;
    defer ln_in.deinit(alloc);

    const d_b: [2]f32 = [0.5, -0.5];
    let b = Tensor.fromSlice(alloc, &s_x, &d_b)?;
    defer b.deinit(alloc);

    let ln_out = layerNorm(alloc, ln_in, w, b, 0.0)?;
    defer ln_out.deinit(alloc);
    expect_eq(approxF32(ln_out.data[0], -1.0 * 1.0 + 0.5, EPS), true);
    expect_eq(approxF32(ln_out.data[1], 1.0 * 2.0 - 0.5, EPS), true);

    // 3. SiLU & GELU
    const s_act: [1]usize = [2];
    const d_act: [2]f32 = [0.0, 1.0];
    let act_in = Tensor.fromSlice(alloc, &s_act, &d_act)?;
    defer act_in.deinit(alloc);

    let silu_out = silu(alloc, act_in)?;
    defer silu_out.deinit(alloc);
    expect_eq(approxF32(silu_out.data[0], 0.0, EPS), true);
    expect_eq(approxF32(silu_out.data[1], 1.0 / (1.0 + @exp(-1.0)), EPS), true);

    let gelu_out = gelu(alloc, act_in)?;
    defer gelu_out.deinit(alloc);
    expect_eq(approxF32(gelu_out.data[0], 0.0, EPS), true);
    expect_eq(approxF32(gelu_out.data[1], 0.84119, 0.001), true);

    // 4. RoPE at pos=1, theta_base=10000.0, head_dim=2 -> angle=1.0 rad
    const d_rope: [2]f32 = [1.0, 0.0];
    let rope_in = Tensor.fromSlice(alloc, &s_x, &d_rope)?;
    defer rope_in.deinit(alloc);

    let rope_out = applyRope(alloc, rope_in, 1, 10000.0)?;
    defer rope_out.deinit(alloc);
    expect_eq(approxF32(rope_out.data[0], @cos(1.0), EPS), true);
    expect_eq(approxF32(rope_out.data[1], @sin(1.0), EPS), true);
}

fn testCapNamespaceAndZeroLeaks(): void! {
    const alloc = std.testing.allocator;

    const s: [2]usize = [2, 2];
    let a = cap.tensor.Tensor.ones(alloc, &s)?;
    defer a.deinit(alloc);

    let b = cap.tensor.scale(alloc, a, 3.0)?;
    defer b.deinit(alloc);

    let c = cap.tensor.matmul(alloc, a, b)?;
    defer c.deinit(alloc);

    expect_eq(approxF32(c.data[0], 6.0, EPS), true);
    expect_eq(approxF32(c.data[3], 6.0, EPS), true);
}

test("tensor views, transpose2d, sliceAxis, reshape, contiguous", testViewsTransposeSliceReshape);
test("tensor matmul and DimensionMismatch guard", testMatmulAndDimensionMismatch);
test("tensor softmaxRows numerical stability and broadcast add/mul", testSoftmaxStabilityAndBroadcast);
test("tensor Transformer ops: rmsNorm, layerNorm, silu, gelu, applyRope", testTransformerOperators);
test("cap.tensor dynamic namespace and zero-leak allocator", testCapNamespaceAndZeroLeaks);

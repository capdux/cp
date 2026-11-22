// Origin: Pure-CP Stdlib Test Suite (AI Training Optimizers: AdamW, Lion, Cosine Warmup & GradClip) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/optimizer.cp
// Covers:
//   1. cosineWarmupLr & linearWarmupDecayLr boundary invariants (step=0, warmup peak, midpoint, total_steps, >total_steps)
//   2. gradGlobalL2Norm & clipGradNormInPlace: below-threshold untouched, above-threshold exact norm == max_norm & direction preserved
//   3. adamWStepInPlace convex quadratic convergence to optimum + decoupled weight decay check
//   4. lionStepInPlace sign-momentum convergence with half state memory
//   5. Zero/invalid hyperparameters, step==0, length mismatch, negative v & NaN/Inf defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LrScheduleConfig,
    AdamWConfig,
    LionConfig,
    cosineWarmupLr,
    linearWarmupDecayLr,
    gradGlobalL2Norm,
    clipGradNormInPlace,
    adamWStepInPlace,
    lionStepInPlace,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testCosineAndLinearWarmupSchedules()! {
    const cfg = LrScheduleConfig{
        peak_lr: 0.001,
        min_lr: 0.0001,
        warmup_steps: 100,
        total_steps: 1100,
    };

    // Warmup boundaries
    expect_eq(approxF64(cosineWarmupLr(0, cfg) ?? -1.0, 0.0, EPS_F64), true);
    expect_eq(approxF64(cosineWarmupLr(50, cfg) ?? -1.0, 0.0005, EPS_F64), true);
    expect_eq(approxF64(cosineWarmupLr(100, cfg) ?? -1.0, 0.001, EPS_F64), true);

    // Cosine decay midpoint (step = 600 -> progress = 0.5 -> cos(pi/2) = 0 -> (peak + min) / 2 = 0.00055)
    expect_eq(approxF64(cosineWarmupLr(600, cfg) ?? -1.0, 0.00055, EPS_F64), true);

    // At and past total_steps -> clamped to min_lr
    expect_eq(approxF64(cosineWarmupLr(1100, cfg) ?? -1.0, 0.0001, EPS_F64), true);
    expect_eq(approxF64(cosineWarmupLr(5000, cfg) ?? -1.0, 0.0001, EPS_F64), true);

    // Linear decay midpoint also == 0.00055, quarter point (step = 350 -> progress = 0.25 -> 0.000775)
    expect_eq(approxF64(linearWarmupDecayLr(600, cfg) ?? -1.0, 0.00055, EPS_F64), true);
    expect_eq(approxF64(linearWarmupDecayLr(350, cfg) ?? -1.0, 0.000775, EPS_F64), true);
    expect_eq(approxF64(linearWarmupDecayLr(1100, cfg) ?? -1.0, 0.0001, EPS_F64), true);
}

fn testGradNormAndClipInvariants()! {
    // [3.0, 4.0] has L2 norm = 5.0
    let g_buf: [2]f64 = [3.0, 4.0];
    let g: []f64 = g_buf[0..];

    expect_eq(approxF64(gradGlobalL2Norm(g_buf[0..]) ?? -1.0, 5.0, EPS_F64), true);

    // Below threshold (max_norm = 10.0): untouched, returns 5.0
    expect_eq(approxF64(clipGradNormInPlace(g, 10.0) ?? -1.0, 5.0, EPS_F64), true);
    expect_eq(approxF64(g_buf[0], 3.0, EPS_F64), true);
    expect_eq(approxF64(g_buf[1], 4.0, EPS_F64), true);

    // Above threshold (max_norm = 2.0): returns pre-clip 5.0, post-clip norm == 2.0 & direction [1.2, 1.6]
    expect_eq(approxF64(clipGradNormInPlace(g, 2.0) ?? -1.0, 5.0, EPS_F64), true);
    expect_eq(approxF64(g_buf[0], 1.2, EPS_F64), true);
    expect_eq(approxF64(g_buf[1], 1.6, EPS_F64), true);
    expect_eq(approxF64(gradGlobalL2Norm(g_buf[0..]) ?? -1.0, 2.0, EPS_F64), true);
}

fn testAdamWConvexConvergenceAndWeightDecay()! {
    // Minimize f(x, y) = 0.5 * ((x - 3)^2 + (y + 2)^2), optimum at (3.0, -2.0)
    let p_buf: [2]f64 = [-4.0, 5.0];
    let m_buf: [2]f64 = [0.0; 2];
    let v_buf: [2]f64 = [0.0; 2];
    let p: []f64 = p_buf[0..];
    let m: []f64 = m_buf[0..];
    let v: []f64 = v_buf[0..];

    const cfg_no_wd = AdamWConfig{ beta1: 0.9, beta2: 0.999, eps: 0.00000001, weight_decay: 0.0 };
    let step: usize = 1;
    while (step <= 400) : (step += 1) {
        const grads: [2]f64 = [p_buf[0] - 3.0, p_buf[1] + 2.0];
        expect_eq(adamWStepInPlace(p, grads[0..], m, v, step, 0.1, cfg_no_wd), true);
    }
    expect_eq(@abs(p_buf[0] - 3.0) < 0.001, true);
    expect_eq(@abs(p_buf[1] + 2.0) < 0.001, true);

    // Decoupled weight decay check: with zero gradients, params decay strictly by (1 - lr * wd)
    let wd_p_buf: [2]f64 = [10.0, -20.0];
    let wd_m_buf: [2]f64 = [0.0; 2];
    let wd_v_buf: [2]f64 = [0.0; 2];
    let wd_p: []f64 = wd_p_buf[0..];
    let wd_m: []f64 = wd_m_buf[0..];
    let wd_v: []f64 = wd_v_buf[0..];
    const zero_g: [2]f64 = [0.0, 0.0];
    const cfg_wd = AdamWConfig{ beta1: 0.9, beta2: 0.999, eps: 0.00000001, weight_decay: 0.1 };
    expect_eq(adamWStepInPlace(wd_p, zero_g[0..], wd_m, wd_v, 1, 0.5, cfg_wd), true);
    expect_eq(approxF64(wd_p_buf[0], 9.5, EPS_F64), true);
    expect_eq(approxF64(wd_p_buf[1], -19.0, EPS_F64), true);
}

fn testLionSignMomentumConvergence()! {
    // 1. Exact single-step sign update check:
    // m0 = [0, 0], g = [2.0, -3.0] -> c = [0.2, -0.3] -> sign(c) = [1.0, -1.0]
    // p1 = p0 - lr * (sign(c) + wd * p0)
    let sp_buf: [2]f64 = [1.0, 2.0];
    let sm_buf: [2]f64 = [0.0, 0.0];
    let sp: []f64 = sp_buf[0..];
    let sm: []f64 = sm_buf[0..];
    const sg: [2]f64 = [2.0, -3.0];
    const cfg_exact = LionConfig{ beta1: 0.9, beta2: 0.99, weight_decay: 0.1 };
    expect_eq(lionStepInPlace(sp, sg[0..], sm, 0.1, cfg_exact), true);
    expect_eq(approxF64(sp_buf[0], 1.0 - 0.1 * (1.0 + 0.1 * 1.0), EPS_F64), true);
    expect_eq(approxF64(sp_buf[1], 2.0 - 0.1 * (-1.0 + 0.1 * 2.0), EPS_F64), true);
    expect_eq(approxF64(sm_buf[0], 0.01 * 2.0, EPS_F64), true);
    expect_eq(approxF64(sm_buf[1], 0.01 * (-3.0), EPS_F64), true);

    // 2. Convex quadratic convergence with cosine LR decay: f(x, y) = 0.5*((x-2)^2 + (y+3)^2)
    let p_buf: [2]f64 = [-3.0, 4.0];
    let m_buf: [2]f64 = [0.0; 2];
    let p: []f64 = p_buf[0..];
    let m: []f64 = m_buf[0..];

    const sched = LrScheduleConfig{ peak_lr: 0.05, min_lr: 0.0005, warmup_steps: 0, total_steps: 600 };
    const cfg = LionConfig{ beta1: 0.9, beta2: 0.95, weight_decay: 0.0 };
    let step: usize = 0;
    while (step < 600) : (step += 1) {
        const lr = cosineWarmupLr(step, sched) ?? 0.0005;
        const grads: [2]f64 = [p_buf[0] - 2.0, p_buf[1] + 3.0];
        expect_eq(lionStepInPlace(p, grads[0..], m, lr, cfg), true);
    }
    expect_eq(@abs(p_buf[0] - 2.0) < 0.01, true);
    expect_eq(@abs(p_buf[1] + 3.0) < 0.01, true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_val = std.math.nan(f64);

    // Invalid schedule configs
    expect_eq(cosineWarmupLr(0, LrScheduleConfig{ peak_lr: 0.001, min_lr: 0.002, warmup_steps: 10, total_steps: 100 }) == null, true);
    expect_eq(cosineWarmupLr(0, LrScheduleConfig{ peak_lr: 0.001, min_lr: 0.0, warmup_steps: 200, total_steps: 100 }) == null, true);
    expect_eq(linearWarmupDecayLr(0, LrScheduleConfig{ peak_lr: -0.1, min_lr: 0.0, warmup_steps: 0, total_steps: 10 }) == null, true);

    // Invalid grad norm / clip
    let g_nan_buf: [2]f64 = [1.0, nan_val];
    let g_nan: []f64 = g_nan_buf[0..];
    let g_ok_buf: [2]f64 = [1.0, 2.0];
    let g_ok: []f64 = g_ok_buf[0..];
    expect_eq(gradGlobalL2Norm(g_nan_buf[0..]) == null, true);
    expect_eq(clipGradNormInPlace(g_ok, 0.0) == null, true);
    expect_eq(clipGradNormInPlace(g_ok, -1.0) == null, true);
    expect_eq(clipGradNormInPlace(g_nan, 1.0) == null, true);

    // AdamW defenses: step == 0, length mismatch, negative v, bad beta/eps/lr, NaN
    let p_buf: [2]f64 = [1.0, 2.0];
    let m_buf: [2]f64 = [0.0, 0.0];
    let v_buf: [2]f64 = [0.0, 0.0];
    let v_neg_buf: [2]f64 = [0.0, -0.1];
    let p: []f64 = p_buf[0..];
    let m: []f64 = m_buf[0..];
    let v: []f64 = v_buf[0..];
    let v_neg: []f64 = v_neg_buf[0..];
    const def_adam = AdamWConfig.default();

    expect_eq(adamWStepInPlace(p, g_ok_buf[0..], m, v, 0, 0.01, def_adam), false); // step == 0
    expect_eq(adamWStepInPlace(p, g_ok_buf[0..1], m, v, 1, 0.01, def_adam), false); // len mismatch
    expect_eq(adamWStepInPlace(p, g_ok_buf[0..], m, v_neg, 1, 0.01, def_adam), false); // v < 0
    expect_eq(adamWStepInPlace(p, g_ok_buf[0..], m, v, 1, -0.01, def_adam), false); // lr < 0
    expect_eq(adamWStepInPlace(p, g_nan_buf[0..], m, v, 1, 0.01, def_adam), false); // NaN grad
    expect_eq(adamWStepInPlace(p, g_ok_buf[0..], m, v, 1, 0.01, AdamWConfig{ beta1: 1.0, beta2: 0.999, eps: 0.00000001, weight_decay: 0.0 }), false);
    // Confirm atomic no-mutation on failed calls
    expect_eq(approxF64(p_buf[0], 1.0, EPS_F64), true);

    // Lion defenses
    const def_lion = LionConfig.default();
    expect_eq(lionStepInPlace(p_buf[0..0], g_ok_buf[0..0], m_buf[0..0], 0.01, def_lion), false);
    expect_eq(lionStepInPlace(p, g_ok_buf[0..1], m, 0.01, def_lion), false);
    expect_eq(lionStepInPlace(p, g_nan_buf[0..], m, 0.01, def_lion), false);
    expect_eq(lionStepInPlace(p, g_ok_buf[0..], m, -0.01, def_lion), false);
    expect_eq(approxF64(p_buf[0], 1.0, EPS_F64), true);
}

fn main() {
    test("1. optimizer: cosineWarmupLr & linearWarmupDecayLr boundaries & midpoints", () => {
        testCosineAndLinearWarmupSchedules()?;
    });
    test("2. optimizer: gradGlobalL2Norm & clipGradNormInPlace norm & direction invariants", () => {
        testGradNormAndClipInvariants()?;
    });
    test("3. optimizer: adamWStepInPlace convex convergence & decoupled weight decay", () => {
        testAdamWConvexConvergenceAndWeightDecay()?;
    });
    test("4. optimizer: lionStepInPlace sign-momentum convergence", () => {
        testLionSignMomentumConvergence()?;
    });
    test("5. optimizer: invalid hyperparams, step==0, mismatch, neg v & NaN defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}

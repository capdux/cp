// Source: cp/cap/test/test_pid_production_deep.cp (Production deep tests for cap:pid)

import { test, expect_eq } from "cap:test";
import {
    PidConfig,
    PidTerms,
    PidController,
    zieglerNicholsPid,
    rateLimit,
} from "cap:pid";

fn absF(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    return absF(a - b) <= eps;
}

test("1. First-order inertia plant closed-loop simulation converges to setpoint=100.0 (< 0.05 error)", () => {
    const cfg = PidConfig {
        kp: 2.5,
        ki: 2.0,
        kd: 0.05,
        out_min: -500.0,
        out_max: 500.0,
        tau_d: 0.02,
        derivative_on_measurement: true,
        deadband: 0.0,
    };
    let ctrl = PidController.init(cfg) ?? unreachable;

    // Plant: tau * dy/dt + y = u  (tau = 1.0s, dt = 0.02s, 500 steps = 10s)
    const dt: f64 = 0.02;
    const tau: f64 = 1.0;
    const setpoint: f64 = 100.0;
    let y: f64 = 0.0;

    let step: usize = 0;
    while (step < 500) {
        const u = ctrl.update(setpoint, y, dt) ?? unreachable;
        y += ((u - y) / tau) * dt;
        step += 1;
    }

    expect_eq(absF(setpoint - y) < 0.05, true);
});

test("2. Conditional integration Anti-Windup prevents integrator runaway & enables immediate reversal", () => {
    const cfg = PidConfig {
        kp: 1.0,
        ki: 5.0,
        kd: 0.0,
        out_min: -10.0,
        out_max: 10.0,
        tau_d: 0.0,
        derivative_on_measurement: true,
        deadband: 0.0,
    };
    let ctrl = PidController.init(cfg) ?? unreachable;

    // Actuator stuck at measurement=0 while setpoint=100 for 200 steps (20 seconds)
    let step: usize = 0;
    while (step < 200) {
        const t = ctrl.updateDetailed(100.0, 0.0, 0.1) ?? unreachable;
        expect_eq(t.saturated, true);
        expect_eq(approxEq(t.output, 10.0, 1e-9), true);
        step += 1;
    }

    // Because of clamping anti-windup, integrator must remain frozen at 0.0 (not wound up to 10000!)
    expect_eq(approxEq(ctrl.integrator, 0.0, 1e-9), true);

    // Now reverse setpoint below measurement -> output drops immediately with zero lag
    const rev = ctrl.updateDetailed(-5.0, 0.0, 0.1) ?? unreachable;
    expect_eq(rev.output < 0.0, true);
    expect_eq(rev.saturated, false);
});

test("3. Derivative-on-measurement eliminates setpoint step kick vs error-derivative & LPF smoothing", () => {
    const cfg_dom = PidConfig {
        kp: 1.0,
        ki: 0.0,
        kd: 2.0,
        out_min: -1000.0,
        out_max: 1000.0,
        tau_d: 0.0,
        derivative_on_measurement: true,
        deadband: 0.0,
    };
    const cfg_doe = PidConfig {
        kp: 1.0,
        ki: 0.0,
        kd: 2.0,
        out_min: -1000.0,
        out_max: 1000.0,
        tau_d: 0.0,
        derivative_on_measurement: false,
        deadband: 0.0,
    };

    let c_dom = PidController.init(cfg_dom) ?? unreachable;
    let c_doe = PidController.init(cfg_doe) ?? unreachable;

    // Warm up at steady state (setpoint=10, measurement=10)
    * = c_dom.updateDetailed(10.0, 10.0, 0.1) ?? unreachable;
    * = c_doe.updateDetailed(10.0, 10.0, 0.1) ?? unreachable;

    // Step setpoint from 10 -> 20 while measurement stays at 10
    const t_dom = c_dom.updateDetailed(20.0, 10.0, 0.1) ?? unreachable;
    const t_doe = c_doe.updateDetailed(20.0, 10.0, 0.1) ?? unreachable;

    // Derivative-on-measurement has ZERO derivative kick; error-derivative kicks by kd * (10 / 0.1) = 200
    expect_eq(approxEq(t_dom.d, 0.0, 1e-9), true);
    expect_eq(approxEq(t_doe.d, 200.0, 1e-9), true);

    // Verify first-order LPF (tau_d = 0.1, dt = 0.1 -> alpha = 0.5) attenuates measurement step by half
    const cfg_lpf = PidConfig {
        kp: 0.0,
        ki: 0.0,
        kd: 1.0,
        out_min: -1000.0,
        out_max: 1000.0,
        tau_d: 0.1,
        derivative_on_measurement: true,
        deadband: 0.0,
    };
    let c_lpf = PidController.init(cfg_lpf) ?? unreachable;
    * = c_lpf.updateDetailed(0.0, 0.0, 0.1) ?? unreachable;
    const t_lpf = c_lpf.updateDetailed(0.0, 1.0, 0.1) ?? unreachable;
    // raw_deriv = -1 / 0.1 = -10; alpha = 0.5 -> filtered = -5.0
    expect_eq(approxEq(t_lpf.d, -5.0, 1e-9), true);
});

test("4. Bumpless manual-to-auto transfer (setManualOutput), reset & error deadband", () => {
    const cfg = PidConfig {
        kp: 2.0,
        ki: 1.0,
        kd: 0.5,
        out_min: 0.0,
        out_max: 100.0,
        tau_d: 0.0,
        derivative_on_measurement: true,
        deadband: 0.5,
    };
    let ctrl = PidController.init(cfg) ?? unreachable;

    // Manual operator holds valve at 42.5% while process sits at measurement=60.0
    ctrl.setManualOutput(42.5, 60.0);

    // Switch to automatic at setpoint=60.0 -> output must stay smoothly at 42.5 with zero bump
    const t0 = ctrl.updateDetailed(60.0, 60.0, 0.1) ?? unreachable;
    expect_eq(approxEq(t0.output, 42.5, 1e-9), true);
    expect_eq(approxEq(t0.p, 0.0, 1e-9), true);
    expect_eq(approxEq(t0.d, 0.0, 1e-9), true);

    // Small error within deadband (|60.3 - 60.0| = 0.3 <= 0.5) treated as zero error
    const t1 = ctrl.updateDetailed(60.3, 60.0, 0.1) ?? unreachable;
    expect_eq(approxEq(t1.p, 0.0, 1e-9), true);
    expect_eq(approxEq(t1.output, 42.5, 1e-9), true);

    // Reset clears integrator and initialized state
    ctrl.reset();
    expect_eq(approxEq(ctrl.integrator, 0.0, 1e-9), true);
    expect_eq(ctrl.initialized, false);
});

test("5. Ziegler-Nichols tuning formula & rateLimit slew limiting", () => {
    // ku = 10.0, tu = 2.0 -> kp = 6.0, ki = 6.0, kd = 1.5
    const zn = zieglerNicholsPid(10.0, 2.0) ?? unreachable;
    expect_eq(approxEq(zn.kp, 6.0, 1e-9), true);
    expect_eq(approxEq(zn.ki, 6.0, 1e-9), true);
    expect_eq(approxEq(zn.kd, 1.5, 1e-9), true);
    expect_eq(zn.derivative_on_measurement, true);

    // rateLimit rising, falling, within-limit, and invalid rate/dt
    expect_eq(approxEq(rateLimit(10.0, 50.0, 20.0, 0.5), 20.0, 1e-9), true);
    expect_eq(approxEq(rateLimit(50.0, 10.0, 20.0, 0.5), 40.0, 1e-9), true);
    expect_eq(approxEq(rateLimit(10.0, 15.0, 20.0, 0.5), 15.0, 1e-9), true);
    expect_eq(approxEq(rateLimit(10.0, 50.0, 0.0, 0.5), 10.0, 1e-9), true);
    expect_eq(approxEq(rateLimit(10.0, 50.0, 20.0, -0.1), 10.0, 1e-9), true);
});

test("6. Strict rejection of invalid configs, dt <= 0 & NaN/Inf inputs", () => {
    const zero_f: f64 = 0.0;
    const nan_val: f64 = zero_f / zero_f;
    const inf_val: f64 = 1.0 / zero_f;

    // out_min > out_max
    const bad_bounds = PidConfig {
        kp: 1.0, ki: 0.0, kd: 0.0,
        out_min: 10.0, out_max: 5.0,
        tau_d: 0.0, derivative_on_measurement: true, deadband: 0.0,
    };
    expect_eq(PidController.init(bad_bounds) == null, true);

    // Negative tau_d or deadband or NaN kp
    const bad_tau = PidConfig {
        kp: 1.0, ki: 0.0, kd: 0.0,
        out_min: 0.0, out_max: 10.0,
        tau_d: -0.1, derivative_on_measurement: true, deadband: 0.0,
    };
    expect_eq(PidController.init(bad_tau) == null, true);

    const bad_db = PidConfig {
        kp: 1.0, ki: 0.0, kd: 0.0,
        out_min: 0.0, out_max: 10.0,
        tau_d: 0.0, derivative_on_measurement: true, deadband: -1.0,
    };
    expect_eq(PidController.init(bad_db) == null, true);

    const bad_nan = PidConfig {
        kp: nan_val, ki: 0.0, kd: 0.0,
        out_min: 0.0, out_max: 10.0,
        tau_d: 0.0, derivative_on_measurement: true, deadband: 0.0,
    };
    expect_eq(PidController.init(bad_nan) == null, true);

    // Invalid dt <= 0 or NaN/Inf on update
    const good_cfg = PidConfig {
        kp: 1.0, ki: 1.0, kd: 0.1,
        out_min: -10.0, out_max: 10.0,
        tau_d: 0.0, derivative_on_measurement: true, deadband: 0.0,
    };
    let ctrl = PidController.init(good_cfg) ?? unreachable;
    expect_eq(ctrl.update(10.0, 0.0, 0.0) == null, true);
    expect_eq(ctrl.update(10.0, 0.0, -0.01) == null, true);
    expect_eq(ctrl.update(nan_val, 0.0, 0.1) == null, true);
    expect_eq(ctrl.update(10.0, inf_val, 0.1) == null, true);
    expect_eq(ctrl.updateDetailed(10.0, 0.0, nan_val) == null, true);

    // Invalid Ziegler-Nichols parameters
    expect_eq(zieglerNicholsPid(0.0, 1.0) == null, true);
    expect_eq(zieglerNicholsPid(1.0, -2.0) == null, true);
    expect_eq(zieglerNicholsPid(nan_val, 1.0) == null, true);
});

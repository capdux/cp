import { test, expect_eq } from "cap:test";
import { PidController } from "../../packages/@cap/pid/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/pid — Pure proportional control exact calculation", () => {
        let pid = PidController.init(100.0, 2.5, 0.0, 0.0);
        expect_eq(approxEq(pid.setpointValue(), 100.0, 0.0001), true);

        const out1 = pid.step(80.0, 0.1);
        expect_eq(approxEq(out1.p, 50.0, 0.0001), true);
        expect_eq(approxEq(out1.i, 0.0, 0.0001), true);
        expect_eq(approxEq(out1.d, 0.0, 0.0001), true);
        expect_eq(approxEq(out1.output, 50.0, 0.0001), true);

        const out2 = pid.step(110.0, 0.1);
        expect_eq(approxEq(out2.output, -25.0, 0.0001), true);
    });

    test("2. @cap/pid — Integral accumulation & i_limit anti-windup clamping", () => {
        let pid = PidController.init(10.0, 0.0, 2.0, 0.0);
        pid.setTermLimits(0.0, 15.0, 0.0);

        // err = 10, ki * err * dt = 2 * 10 * 0.5 = 10
        const o1 = pid.step(0.0, 0.5);
        expect_eq(approxEq(o1.i, 10.0, 0.0001), true);

        // Next step adds +10 -> 20, clamped to i_limit = 15.0
        const o2 = pid.step(0.0, 0.5);
        expect_eq(approxEq(o2.i, 15.0, 0.0001), true);
        expect_eq(approxEq(pid.integralValue(), 15.0, 0.0001), true);

        // Reversed error immediately unwinds without windup lag
        const o3 = pid.step(15.0, 0.5); // err = -5 -> -5
        expect_eq(approxEq(o3.i, 10.0, 0.0001), true);
    });

    test("3. @cap/pid — Derivative-on-measurement damping & zero setpoint-step kick", () => {
        let pid = PidController.init(50.0, 0.0, 0.0, 2.0);

        // First sample establishes baseline; derivative is 0
        const o1 = pid.step(10.0, 0.5);
        expect_eq(approxEq(o1.d, 0.0, 0.0001), true);

        // Step setpoint from 50 -> 500 while measurement stays at 10.0: NO derivative kick!
        pid.setSetpoint(500.0);
        const o2 = pid.step(10.0, 0.5);
        expect_eq(approxEq(o2.d, 0.0, 0.0001), true);

        // Measurement rises by +5 in dt=0.5 -> d = -2.0 * (5.0 / 0.5) = -20.0
        const o3 = pid.step(15.0, 0.5);
        expect_eq(approxEq(o3.d, -20.0, 0.0001), true);
    });

    test("4. @cap/pid — Total output saturation limits out_min / out_max", () => {
        let pid = PidController.init(100.0, 5.0, 0.0, 0.0);
        pid.setLimits(-30.0, 40.0);

        const high = pid.step(0.0, 0.1); // raw P = 500 -> clamped to 40
        expect_eq(approxEq(high.output, 40.0, 0.0001), true);

        const low = pid.step(200.0, 0.1); // raw P = -500 -> clamped to -30
        expect_eq(approxEq(low.output, -30.0, 0.0001), true);
    });

    test("5. @cap/pid — dt <= 0.0 division-by-zero protection", () => {
        let pid = PidController.init(20.0, 1.0, 3.0, 4.0);
        const first = pid.step(10.0, 0.5); // P=10, I=15, D=0
        expect_eq(approxEq(first.i, 15.0, 0.0001), true);

        // dt == 0.0 and dt < 0.0 must not divide by zero or alter integral
        const zero_dt = pid.step(12.0, 0.0);
        expect_eq(approxEq(zero_dt.p, 8.0, 0.0001), true);
        expect_eq(approxEq(zero_dt.i, 15.0, 0.0001), true);
        expect_eq(approxEq(zero_dt.d, 0.0, 0.0001), true);

        const neg_dt = pid.step(14.0, -0.2);
        expect_eq(approxEq(neg_dt.i, 15.0, 0.0001), true);
        expect_eq(approxEq(neg_dt.d, 0.0, 0.0001), true);
    });

    test("6. @cap/pid — First-order plant closed-loop convergence (50 steps) & reset", () => {
        // Plant: x[k+1] = x[k] + (u - 0.1 * x[k]) * dt, target = 100.0
        let pid = PidController.init(100.0, 2.0, 1.5, 0.1);
        pid.setTermLimits(200.0, 100.0, 50.0);
        pid.setLimits(-200.0, 200.0);

        let x: f64 = 0.0;
        const dt: f64 = 0.1;
        let step_i: usize = 0;
        while (step_i < 50) {
            const ctrl = pid.step(x, dt);
            x += (ctrl.output - 0.1 * x) * dt;
            step_i += 1;
        }

        // After 50 steps, plant converges closely to setpoint 100.0
        expect_eq(approxEq(x, 100.0, 1.0), true);

        // Reset clears integral and derivative history
        pid.reset();
        expect_eq(approxEq(pid.integralValue(), 0.0, 0.0001), true);
        const after_reset = pid.step(50.0, 0.1);
        expect_eq(approxEq(after_reset.d, 0.0, 0.0001), true);
    });
}

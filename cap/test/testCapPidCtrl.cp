import { test, expect_eq } from "cap:test";
import { PidConfig, PidController } from "../../packages/node-pid-controller/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d <= eps;
}

fn main() {
    test("1. @cap/node-pid-controller — P-only proportional response", () => {
        let pid = PidController.new(2.0, 0.0, 0.0, 10.0);
        expect_eq(near(pid.update(4.0, 0.1), 12.0, 0.000001), true);
        expect_eq(near(pid.update(10.0, 0.1), 0.0, 0.000001), true);
        expect_eq(near(pid.getError(7.5), 2.5, 0.000001), true);
    });

    test("2. @cap/node-pid-controller — PI integral accumulation & anti-windup clamp", () => {
        let pid = PidController.new(0.0, 1.0, 0.0, 10.0);
        pid.setIntegralLimits(-5.0, 5.0);
        expect_eq(near(pid.update(0.0, 0.2), 2.0, 0.000001), true);
        expect_eq(near(pid.update(0.0, 0.2), 4.0, 0.000001), true);
        expect_eq(near(pid.update(0.0, 0.2), 5.0, 0.000001), true);
    });

    test("3. @cap/node-pid-controller — derivative on measurement avoids setpoint kick", () => {
        let pid = PidController.new(0.0, 0.0, 1.0, 0.0);
        expect_eq(near(pid.update(5.0, 0.5), 0.0, 0.000001), true);
        pid.setTarget(100.0);
        expect_eq(near(pid.update(5.0, 0.5), 0.0, 0.000001), true);
        expect_eq(near(pid.update(7.0, 0.5), -4.0, 0.000001), true);
    });

    test("4. @cap/node-pid-controller — output clamping limits", () => {
        let pid = PidController.new(10.0, 0.0, 0.0, 100.0);
        pid.setOutputLimits(-20.0, 20.0);
        expect_eq(near(pid.update(0.0, 0.1), 20.0, 0.000001), true);
        expect_eq(near(pid.update(200.0, 0.1), -20.0, 0.000001), true);
    });

    test("5. @cap/node-pid-controller — closed-loop plant convergence to setpoint", () => {
        let pid = PidController.new(1.2, 0.8, 0.05, 50.0);
        pid.setOutputLimits(-100.0, 100.0);
        let pos: f64 = 0.0;
        let i: i32 = 0;
        while (i < 200) : (i += 1) {
            const u = pid.update(pos, 0.1);
            pos = pos + (u - 0.2 * pos) * 0.1;
        }
        expect_eq(near(pos, 50.0, 0.2), true);
    });

    test("6. @cap/node-pid-controller — reset & zero dt safety", () => {
        let pid = PidController.new(1.0, 1.0, 1.0, 10.0);
        * = pid.update(0.0, 1.0);
        pid.reset();
        expect_eq(near(pid.integral, 0.0, 0.000001), true);
        expect_eq(pid.has_prev, false);
        expect_eq(near(pid.update(4.0, 0.0), 6.0, 0.000001), true);
    });

    test("7. @cap/node-pid-controller — derivative low-pass filter & derivative-on-error mode", () => {
        let pid = PidController.new(0.0, 0.0, 1.0, 0.0);
        pid.setDerivativeFilter(0.5);
        * = pid.update(0.0, 1.0);
        // raw_rate = -10.0, alpha = 0.5 -> filtered = -5.0
        const d1 = pid.update(10.0, 1.0);
        expect_eq(near(d1, -5.0, 0.000001), true);
        // raw_rate = 0.0, filtered = 0.5*0 + 0.5*(-5.0) = -2.5
        const d2 = pid.update(10.0, 1.0);
        expect_eq(near(d2, -2.5, 0.000001), true);

        // Derivative-on-error mode reacts to setpoint change
        let pid_err = PidController.new(0.0, 0.0, 1.0, 0.0);
        pid_err.setDerivativeMode(false);
        * = pid_err.update(0.0, 1.0);
        pid_err.setTarget(10.0);
        expect_eq(near(pid_err.update(0.0, 1.0), 10.0, 0.000001), true);
    });

    test("8. @cap/node-pid-controller — fromConfig, updateWithDt, setTunings, clone & getLastTerms", () => {
        let cfg = PidConfig.defaults(2.0, 0.5, 0.1);
        cfg.dt = 0.5;
        cfg.setpoint = 10.0;
        cfg.out_min = -50.0;
        cfg.out_max = 50.0;
        let pid = PidController.fromConfig(cfg);
        const u1 = pid.updateWithDt(6.0); // err=4 -> P=8, I=0.5*4*0.5=1, D=0 -> 9
        expect_eq(near(u1, 9.0, 0.000001), true);
        const terms = pid.getLastTerms();
        expect_eq(near(terms.p, 8.0, 0.000001), true);
        expect_eq(near(terms.i, 1.0, 0.000001), true);
        expect_eq(near(terms.d, 0.0, 0.000001), true);
        expect_eq(near(terms.output, 9.0, 0.000001), true);

        let copy = pid.clone();
        pid.setTunings(0.0, 0.0, 0.0);
        pid.setDt(0.1);
        expect_eq(near(pid.updateWithDt(6.0), 1.0, 0.000001), true);
        // Cloned controller preserves original gains and state
        expect_eq(near(copy.kp, 2.0, 0.000001), true);
        expect_eq(near(copy.integral, 1.0, 0.000001), true);
    });
}

import { test, expect_eq } from "cap:test";
import { PidController } from "../../packages/@cap/pid-ctrl/src/index.cp";

fn near(a: f64, b: f64, eps: f64): bool {
    let d = a - b;
    if (d < 0.0) { d = -d; }
    return d <= eps;
}

fn main() {
    test("P-only proportional response", () => {
        let pid = PidController.new(2.0, 0.0, 0.0, 10.0);
        expect_eq(near(pid.update(4.0, 0.1), 12.0, 0.000001), true);
        expect_eq(near(pid.update(10.0, 0.1), 0.0, 0.000001), true);
    });

    test("PI integral accumulation & anti-windup clamp", () => {
        let pid = PidController.new(0.0, 1.0, 0.0, 10.0);
        pid.setIntegralLimits(-5.0, 5.0);
        expect_eq(near(pid.update(0.0, 0.2), 2.0, 0.000001), true);
        expect_eq(near(pid.update(0.0, 0.2), 4.0, 0.000001), true);
        expect_eq(near(pid.update(0.0, 0.2), 5.0, 0.000001), true);
    });

    test("derivative on measurement avoids setpoint kick", () => {
        let pid = PidController.new(0.0, 0.0, 1.0, 0.0);
        expect_eq(near(pid.update(5.0, 0.5), 0.0, 0.000001), true);
        pid.setTarget(100.0);
        // measurement did not change, so d_term == 0 even though setpoint jumped!
        expect_eq(near(pid.update(5.0, 0.5), 0.0, 0.000001), true);
        // measurement increased by 2.0 in 0.5s -> rate = 4.0 -> d_term = -4.0
        expect_eq(near(pid.update(7.0, 0.5), -4.0, 0.000001), true);
    });

    test("output clamping limits", () => {
        let pid = PidController.new(10.0, 0.0, 0.0, 100.0);
        pid.setOutputLimits(-20.0, 20.0);
        expect_eq(near(pid.update(0.0, 0.1), 20.0, 0.000001), true);
        expect_eq(near(pid.update(200.0, 0.1), -20.0, 0.000001), true);
    });

    test("closed-loop plant convergence to setpoint", () => {
        let pid = PidController.new(1.2, 0.8, 0.05, 50.0);
        pid.setOutputLimits(-100.0, 100.0);
        let pos: f64 = 0.0;
        let i: i32 = 0;
        while (i < 200) {
            let u = pid.update(pos, 0.1);
            pos = pos + (u - 0.2 * pos) * 0.1;
            i = i + 1;
        }
        expect_eq(near(pos, 50.0, 0.2), true);
    });

    test("reset & zero dt safety", () => {
        let pid = PidController.new(1.0, 1.0, 1.0, 10.0);
        * = pid.update(0.0, 1.0);
        pid.reset();
        expect_eq(near(pid.integral, 0.0, 0.000001), true);
        expect_eq(pid.has_prev, false);
        expect_eq(near(pid.update(4.0, 0.0), 6.0, 0.000001), true);
    });
}

# node-pid-controller — AI Agent Quick Ref

Zero-heap industrial PID controller with anti-windup clamping, derivative low-pass filter, and derivative-on-measurement (npm `node-pid-controller` / Rust `pid_control` parity).

## Import

```cp
import { PidConfig, PidTerms, PidController } from "node-pid-controller";
```

## API Signatures

```cp
export class PidConfig {
    kp: f64, ki: f64, kd: f64, dt: f64, setpoint: f64,
    i_min: f64, i_max: f64, out_min: f64, out_max: f64,
    d_filter_alpha: f64, derivative_on_measurement: bool,
    static fn defaults(kp: f64, ki: f64, kd: f64): PidConfig;
}
export class PidTerms { p: f64, i: f64, d: f64, output: f64 }
export class PidController {
    static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController;
    static fn fromConfig(cfg: PidConfig): PidController;
    fn setTunings(*self, kp: f64, ki: f64, kd: f64);
    fn setDt(*self, dt: f64);
    fn setOutputLimits(*self, lo: f64, hi: f64);
    fn setIntegralLimits(*self, lo: f64, hi: f64);
    fn setDerivativeFilter(*self, alpha: f64);
    fn setDerivativeMode(*self, on_measurement: bool);
    fn setTarget(*self, target: f64);
    fn getError(measurement: f64): f64;
    fn getLastTerms(): PidTerms;
    fn reset(*self);
    fn clone(): PidController;
    fn updateWithDt(*self, measurement: f64): f64;
    fn update(*self, measurement: f64, dt: f64): f64;
}
```

## Minimal Example

```cp
import { PidController } from "node-pid-controller";

fn demo(): f64 {
    let mut pid = PidController.new(1.0, 0.1, 0.05, 50.0);
    pid.setOutputLimits(-100.0, 100.0);
    const out = pid.update(40.0, 0.1);
    const t = pid.getLastTerms();
    return t.output;
}
```

## Caveats & Borrowing Rules

- **Mutable instance**: Declare `let mut pid = PidController.new(...)` because `update`, `updateWithDt`, `reset`, and all `set*` methods mutate `*self`.
- **Positive `dt`**: Pass `dt > 0.0` to `update` / `setDt`; non-positive `dt` disables integral/derivative accumulation safely without division by zero.
- **Derivative kick**: Keep `derivative_on_measurement = true` when changing `setpoint` dynamically to avoid output spikes.
- **Zero heap**: `PidController`, `PidConfig`, and `PidTerms` are plain value structs (`clone()` copies state cleanly).

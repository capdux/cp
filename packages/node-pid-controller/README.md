[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# node-pid-controller

Zero-heap industrial PID controller with anti-windup integral clamping, derivative low-pass filtering, and derivative-on-measurement mode in pure CP.

- **Upstream Alignment**: `npm` `node-pid-controller` & `crates.io` `pid_control` (MIT)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — pure stack-allocated `f64` state machine.

## Installation & Import

```bash
cap add node-pid-controller
```

```cp
import { PidConfig, PidTerms, PidController } from "node-pid-controller";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `PidConfig` | `class PidConfig { kp, ki, kd, dt, setpoint, i_min, i_max, out_min, out_max, d_filter_alpha: f64, derivative_on_measurement: bool }` | Full controller configuration. `PidConfig.defaults(kp, ki, kd)` returns sensible industrial defaults. |
| `PidTerms` | `class PidTerms { p: f64, i: f64, d: f64, output: f64 }` | Snapshot of the last computed P, I, D terms and clamped control output. |
| `PidController.new` | `static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController` | Creates a PID controller with gains and target setpoint. |
| `PidController.fromConfig` | `static fn fromConfig(cfg: PidConfig): PidController` | Creates a PID controller from a `PidConfig` struct. |
| `setTunings` / `setDt` | `fn setTunings(*self, kp, ki, kd: f64)` / `fn setDt(*self, dt: f64)` | Updates PID gains or default sample period `dt`. |
| `setOutputLimits` / `setIntegralLimits` | `fn setOutputLimits(*self, lo, hi: f64)` / `fn setIntegralLimits(*self, lo, hi: f64)` | Configures output clamping and anti-windup integral bounds. |
| `setDerivativeFilter` / `setDerivativeMode` | `fn setDerivativeFilter(*self, alpha: f64)` / `fn setDerivativeMode(*self, on_measurement: bool)` | Sets first-order low-pass filter factor `alpha` and switches between derivative-on-measurement (avoids derivative kick) and derivative-on-error. |
| `setTarget` | `fn setTarget(*self, target: f64)` | Updates the target `setpoint`. |
| `getError` / `getLastTerms` | `fn getError(measurement: f64): f64` / `fn getLastTerms(): PidTerms` | Computes `setpoint - measurement` or inspects the last step's `PidTerms`. |
| `reset` / `clone` | `fn reset(*self)` / `fn clone(): PidController` | Clears accumulated integral/derivative history or clones controller state. |
| `updateWithDt` / `update` | `fn updateWithDt(*self, measurement: f64): f64` / `fn update(*self, measurement: f64, dt: f64): f64` | Advances the PID loop using stored `dt` or an explicit step `dt` and returns clamped output. |

## Examples

### 1. Basic Closed-Loop Control with Anti-Windup

```cp
import { PidController } from "node-pid-controller";

fn controlLoop(): f64 {
    let mut pid = PidController.new(1.5, 0.2, 0.05, 100.0);
    pid.setOutputLimits(-50.0, 50.0);
    pid.setIntegralLimits(-20.0, 20.0);

    let mut temp: f64 = 25.0;
    let out = pid.update(temp, 0.1);
    return out;
}
```

### 2. Config-Driven Controller with Derivative-on-Measurement & Diagnostics

```cp
import { PidConfig, PidController } from "node-pid-controller";

fn configControl(): void {
    let mut cfg = PidConfig.defaults(2.0, 0.5, 0.1);
    cfg.setpoint = 60.0;
    cfg.dt = 0.02;
    cfg.d_filter_alpha = 0.2;
    cfg.derivative_on_measurement = true;

    let mut pid = PidController.fromConfig(cfg);
    const u = pid.updateWithDt(52.5);
    const terms = pid.getLastTerms(); // terms.p, terms.i, terms.d, terms.output
}
```

## Differences from Upstream & Highlights

- **Deterministic Time Steps**: Explicit `dt` parameter in `update(measurement, dt)` and fixed `dt` in `updateWithDt(measurement)` eliminate wall-clock jitter present in JS `Date.now()`-based loops.
- **Industrial Safeguards**: Built-in anti-windup integral clamping, output saturation limits, derivative low-pass filtering, and derivative-on-measurement to prevent setpoint-change spikes.
- **Zero Allocation**: Entire controller fits on the stack with zero GC pauses — ideal for real-time robotics and control loops.

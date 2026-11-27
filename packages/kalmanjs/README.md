[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kalmanjs

Zero-allocation 1D scalar Kalman filter for sensor denoising, signal smoothing, and linear state prediction in pure CP.

Aligned with npm [`kalmanjs`](https://github.com/wouterbulten/kalmanjs) (MIT).

## Installation & Import

```bash
cap add kalmanjs
```

```cp
import { Kalman1D, KalmanFilter } from "kalmanjs";
```

## Core API

### `KalmanFilter` (npm `kalmanjs` parity)

| Member | Signature | Description |
| --- | --- | --- |
| Fields | `R, Q, A, B, C, x, cov: f64`, `initialized: bool` | Process noise (`R`), measurement noise (`Q`), state/control/observation scalars (`A, B, C`), state estimate (`x`), covariance (`cov`). |
| `KalmanFilter.default` | `(): KalmanFilter` | Default filter (`R=1.0, Q=1.0, A=1.0, B=0.0, C=1.0`). |
| `KalmanFilter.new` | `(R: f64, Q: f64): KalmanFilter` | Creates filter with process noise `R` and measurement noise `Q`. |
| `KalmanFilter.withParams` | `(R: f64, Q: f64, A: f64, B: f64, C: f64): KalmanFilter` | Full linear model constructor. |
| `filter` | `(*self, z: f64): f64` | Filters measurement `z` with control input `u = 0.0` and returns updated state estimate. |
| `filterWithControl` | `(*self, z: f64, u: f64): f64` | Filters measurement `z` with control input `u`. |
| `predict` | `(u: f64): f64` | Predicts next state without mutating internal state. |
| `uncertainty` / `variance` | `(): f64` | Returns current covariance estimate (`cov`). |
| `lastMeasurement` | `(): f64` | Returns latest state estimate `x`. |
| `setProcessNoise` / `setMeasurementNoise` | `(*self, noise: f64)` | Dynamically updates process or measurement noise. |
| `reset` | `(*self)` | Resets `initialized`, `x`, and `cov` to uninitialized state. |

### `Kalman1D` (Standard Control-Theory Naming)

| Member | Signature | Description |
| --- | --- | --- |
| Fields | `q, r, a, b, c, x, cov: f64`, `initialized: bool` | Standard notation: `q` = process noise, `r` = measurement noise. |
| `Kalman1D.new` | `(process_noise: f64, measurement_noise: f64): Kalman1D` | Creates filter with `(q, r)` and identity model `a=1, b=0, c=1`. |
| `Kalman1D.withModel` | `(q: f64, r: f64, a: f64, b: f64, c: f64): Kalman1D` | Full linear model constructor. |
| `filter` | `(*self, z: f64, u: f64): f64` | Filters measurement `z` with control input `u`. |
| `filterOnly` | `(*self, z: f64): f64` | Filters measurement `z` with `u = 0.0`. |
| `predict` / `uncertainty` / `variance` / `lastMeasurement` | `(u: f64): f64` / `(): f64` | Read-only state queries. |
| `setProcessNoise` / `setMeasurementNoise` / `reset` | `(*self, ...)` | Mutating parameter updates and state reset. |

## Examples

### 1. Smoothing Noisy Sensor Readings (`KalmanFilter`)

```cp
import { print } from "cap:io";
import { KalmanFilter } from "kalmanjs";

pub fn main() {
    let kf = KalmanFilter.new(0.01, 3.0);
    let readings: [5]f64 = [20.1, 20.8, 19.7, 20.3, 20.0];
    let i: usize = 0;
    while (i < 5) {
        let est = kf.filter(readings[i]);
        print("step {}: est={}\n", i, est);
        i += 1;
    }
}
```

### 2. Constant-Growth System with Control Input (`Kalman1D`)

```cp
import { print } from "cap:io";
import { Kalman1D } from "kalmanjs";

pub fn main() {
    let kf = Kalman1D.withModel(0.01, 1.0, 1.0, 1.0, 1.0);
    let _ = kf.filter(10.0, 1.0);
    let est = kf.filter(11.4, 1.0);
    print("est={}, next_pred={}, cov={}\n", est, kf.predict(1.0), kf.uncertainty());
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Value-type stack state**: `KalmanFilter` and `Kalman1D` are plain stack structs (~64 bytes) with zero heap allocations and deterministic performance for real-time DSP / embedded loops.
- **Dual naming conventions**: Provides both `KalmanFilter` (npm `kalmanjs` `R`/`Q` convention) and `Kalman1D` (control-theory `q`/`r` convention) in one package.
- **Explicit mutability**: Read-only queries (`predict`, `uncertainty`, `variance`, `lastMeasurement`) never mutate filter state, while update methods require a mutable receiver.

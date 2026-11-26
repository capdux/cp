# @cap/kalman1d

Zero-allocation 1D scalar Kalman filter for sensor denoising, smoothing, and linear state prediction in CP.

## Features

- **Zero heap allocation**: lightweight `Kalman1D` value struct with configurable `(q, r, a, b, c)` linear system parameters.
- **Full filter lifecycle**: `Kalman1D.new`, `Kalman1D.withModel`, `filter(z, u)`, side-effect-free `predict(u)`, `lastMeasurement()`, and `reset()`.

## Usage

```cp
import { Kalman1D } from "@cap/kalman1d";

let kf = Kalman1D.new(0.01 as f64, 1.0 as f64);
const est1 = kf.filter(10.5 as f64, 0.0 as f64);
const est2 = kf.filter(9.6 as f64, 0.0 as f64);
```

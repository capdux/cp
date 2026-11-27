# kalmanjs — AI Agent Quick Ref

Zero-allocation 1D scalar Kalman filter for sensor denoising and linear state prediction in pure CP (npm `kalmanjs` parity).

## Import

```cp
import { Kalman1D, KalmanFilter } from "kalmanjs";
```

## API Signatures

```cp
class KalmanFilter {
    R: f64, Q: f64, A: f64, B: f64, C: f64, x: f64, cov: f64, initialized: bool,
    static fn default(): KalmanFilter;
    static fn new(R: f64, Q: f64): KalmanFilter;
    static fn withParams(R: f64, Q: f64, A: f64, B: f64, C: f64): KalmanFilter;
    fn filter(*self, z: f64): f64;
    fn filterWithControl(*self, z: f64, u: f64): f64;
    fn predict(u: f64): f64;
    fn uncertainty(): f64;
    fn variance(): f64;
    fn lastMeasurement(): f64;
    fn setMeasurementNoise(*self, noise: f64);
    fn setProcessNoise(*self, noise: f64);
    fn reset(*self);
}

class Kalman1D {
    q: f64, r: f64, a: f64, b: f64, c: f64, x: f64, cov: f64, initialized: bool,
    static fn new(process_noise: f64, measurement_noise: f64): Kalman1D;
    static fn withModel(q: f64, r: f64, a: f64, b: f64, c: f64): Kalman1D;
    fn filter(*self, z: f64, u: f64): f64;
    fn filterOnly(*self, z: f64): f64;
    fn predict(u: f64): f64;
    fn uncertainty(): f64;
    fn variance(): f64;
    fn lastMeasurement(): f64;
    fn setProcessNoise(*self, noise: f64);
    fn setMeasurementNoise(*self, noise: f64);
    fn reset(*self);
}
```

## Minimal Example

```cp
import { KalmanFilter } from "kalmanjs";

let kf = KalmanFilter.new(0.01, 3.0);
let s1 = kf.filter(20.1);
let s2 = kf.filter(20.6);
let next = kf.predict(0.0);
```

## Pitfalls & Borrowing Rules

- **Signature difference**: `KalmanFilter.filter(z)` takes 1 arg (use `filterWithControl(z, u)` for control input), whereas `Kalman1D.filter(z, u)` takes 2 args (use `filterOnly(z)` for `u = 0`).
- **`R`/`Q` vs `q`/`r` naming**: In `KalmanFilter` (npm parity), `R` is process noise and `Q` is measurement noise; in `Kalman1D` (control theory), `q` is process noise and `r` is measurement noise.
- **Mutable receiver**: `filter`, `filterOnly`, `filterWithControl`, `setProcessNoise`, `setMeasurementNoise`, and `reset` mutate the filter in place.
- **Uninitialized state**: Before the first `filter*` call, `initialized` is `false`; the first measurement seeds `x` and `cov` automatically.

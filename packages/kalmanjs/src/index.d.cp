// Type definitions for CP

export fn k1Abs(x: f64): f64;
export class Kalman1D {
    q: f64,
    r: f64,
    a: f64,
    b: f64,
    c: f64,
    x: f64,
    cov: f64,
    initialized: bool,
    fn new(process_noise: f64, measurement_noise: f64): Kalman1D;
    fn withModel(q: f64, r: f64, a: f64, b: f64, c: f64): Kalman1D;
    fn filter(_: auto, z: f64, u: f64): f64;
    fn predict(u: f64): f64;
    fn lastMeasurement(): f64;
    fn reset(_: auto);
}


// Type definitions for CP

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
    fn filterOnly(_: auto, z: f64): f64;
    fn predict(u: f64): f64;
    fn uncertainty(): f64;
    fn variance(): f64;
    fn lastMeasurement(): f64;
    fn setProcessNoise(_: auto, noise: f64);
    fn setMeasurementNoise(_: auto, noise: f64);
    fn reset(_: auto);
}

export class KalmanFilter {
    R: f64,
    Q: f64,
    A: f64,
    B: f64,
    C: f64,
    x: f64,
    cov: f64,
    initialized: bool,
    fn default(): KalmanFilter;
    fn new(R: f64, Q: f64): KalmanFilter;
    fn withParams(R: f64, Q: f64, A: f64, B: f64, C: f64): KalmanFilter;
    fn filterWithControl(_: auto, z: f64, u: f64): f64;
    fn filter(_: auto, z: f64): f64;
    fn predict(u: f64): f64;
    fn uncertainty(): f64;
    fn variance(): f64;
    fn lastMeasurement(): f64;
    fn setMeasurementNoise(_: auto, noise: f64);
    fn setProcessNoise(_: auto, noise: f64);
    fn reset(_: auto);
}

// kalmanjs — Zero-allocation 1D scalar Kalman filter (npm `kalmanjs` parity).
// MIT clean-room pure CP implementation.

fn k1Abs(x: f64): f64 {
    if (x < (0.0 as f64)) {
        return -x;
    }
    return x;
}

/// 1D scalar Kalman filter with configurable state transition `a`, control gain `b`,
/// observation gain `c`, process noise `q`, and measurement noise `r`.
pub struct Kalman1D {
    pub q: f64,
    pub r: f64,
    pub a: f64,
    pub b: f64,
    pub c: f64,
    pub x: f64,
    pub cov: f64,
    pub initialized: bool,

    /// Create a default constant-state 1D Kalman filter (`a = 1`, `b = 0`, `c = 1`).
    pub static fn new(process_noise: f64, measurement_noise: f64): Kalman1D {
        return Kalman1D.withModel(process_noise, measurement_noise, 1.0 as f64, 0.0 as f64, 1.0 as f64);
    }

    /// Create a 1D Kalman filter with explicit linear system model `(q, r, a, b, c)`.
    pub static fn withModel(q: f64, r: f64, a: f64, b: f64, c: f64): Kalman1D {
        return Kalman1D{
            q: q,
            r: r,
            a: a,
            b: b,
            c: c,
            x: 0.0 as f64,
            cov: 0.0 as f64,
            initialized: false,
        };
    }

    /// Incorporate measurement `z` with optional control input `u` and return the filtered estimate.
    /// The first call initializes the state directly from `z / c`.
    pub fn filter(*_, z: f64, u: f64): f64 {
        if (!_.initialized) {
            if (k1Abs(_.c) > (0.000000000000001 as f64)) {
                _.x = z / _.c;
                _.cov = _.r / (_.c * _.c);
            } else {
                _.x = z;
                _.cov = _.r;
            }
            _.initialized = true;
            return _.x;
        }

        // Prediction step
        const predX: f64 = (_.a * _.x) + (_.b * u);
        const predCov: f64 = (_.a * _.cov * _.a) + _.q;

        // Kalman gain
        const denom: f64 = (_.c * predCov * _.c) + _.r;
        let k: f64 = 0.0;
        if (k1Abs(denom) > (0.000000000000001 as f64)) {
            k = (predCov * _.c) / denom;
        }

        // Measurement update
        _.x = predX + k * (z - (_.c * predX));
        _.cov = predCov - (k * _.c * predCov);
        return _.x;
    }

    /// Incorporate measurement `z` with zero control input (`u = 0`).
    pub fn filterOnly(*_, z: f64): f64 {
        return _.filter(z, 0.0 as f64);
    }

    /// Predict the next state given control input `u` without mutating internal state.
    pub fn predict(u: f64): f64 {
        return (_.a * _.x) + (_.b * u);
    }

    /// Return predicted covariance `(a * cov * a) + q` without mutating internal state.
    pub fn uncertainty(): f64 {
        return (_.a * _.cov * _.a) + _.q;
    }

    /// Return the current posterior state variance (`cov`).
    pub fn variance(): f64 {
        return _.cov;
    }

    /// Return the current estimated state `x`.
    pub fn lastMeasurement(): f64 {
        return _.x;
    }

    /// Dynamically update process noise `q`.
    pub fn setProcessNoise(*_, noise: f64) {
        _.q = noise;
    }

    /// Dynamically update measurement noise `r`.
    pub fn setMeasurementNoise(*_, noise: f64) {
        _.r = noise;
    }

    /// Reset estimated state, covariance, and initialization flag.
    pub fn reset(*_) {
        _.x = 0.0 as f64;
        _.cov = 0.0 as f64;
        _.initialized = false;
    }
}

/// Upstream `kalmanjs` compatible 1D Kalman filter using npm `kalmanjs` parameter naming:
/// - `R`: process noise (default `1.0`)
/// - `Q`: measurement noise (default `1.0`)
/// - `A`: state transition scalar (default `1.0`)
/// - `B`: control input scalar (default `0.0`)
/// - `C`: measurement observation scalar (default `1.0`)
pub struct KalmanFilter {
    pub R: f64,
    pub Q: f64,
    pub A: f64,
    pub B: f64,
    pub C: f64,
    pub x: f64,
    pub cov: f64,
    pub initialized: bool,

    /// Create a default `KalmanFilter` with `R=1, Q=1, A=1, B=0, C=1`.
    pub static fn default(): KalmanFilter {
        return KalmanFilter.withParams(1.0, 1.0, 1.0, 0.0, 1.0);
    }

    /// Create a constant-model `KalmanFilter` with process noise `R` and measurement noise `Q`.
    pub static fn new(R: f64, Q: f64): KalmanFilter {
        return KalmanFilter.withParams(R, Q, 1.0, 0.0, 1.0);
    }

    /// Create a `KalmanFilter` with full linear parameters `(R, Q, A, B, C)`.
    pub static fn withParams(R: f64, Q: f64, A: f64, B: f64, C: f64): KalmanFilter {
        return KalmanFilter {
            R: R,
            Q: Q,
            A: A,
            B: B,
            C: C,
            x: 0.0,
            cov: 0.0,
            initialized: false,
        };
    }

    /// Filter a new measurement `z` with control input `u` and return the updated state estimate.
    pub fn filterWithControl(*_, z: f64, u: f64): f64 {
        if (!_.initialized) {
            if (k1Abs(_.C) > (0.000000000000001 as f64)) {
                _.x = z / _.C;
                _.cov = _.Q / (_.C * _.C);
            } else {
                _.x = z;
                _.cov = _.Q;
            }
            _.initialized = true;
            return _.x;
        }

        const predX: f64 = (_.A * _.x) + (_.B * u);
        const predCov: f64 = (_.A * _.cov * _.A) + _.R;

        const denom: f64 = (_.C * predCov * _.C) + _.Q;
        let k: f64 = 0.0;
        if (k1Abs(denom) > (0.000000000000001 as f64)) {
            k = (predCov * _.C) / denom;
        }

        _.x = predX + k * (z - (_.C * predX));
        _.cov = predCov - (k * _.C * predCov);
        return _.x;
    }

    /// Filter a new measurement `z` (zero control input `u = 0`).
    pub fn filter(*_, z: f64): f64 {
        return _.filterWithControl(z, 0.0);
    }

    /// Predict the next state given control input `u` without mutating internal state.
    pub fn predict(u: f64): f64 {
        return (_.A * _.x) + (_.B * u);
    }

    /// Return predicted uncertainty `(A * cov * A) + R` without mutating internal state.
    pub fn uncertainty(): f64 {
        return (_.A * _.cov * _.A) + _.R;
    }

    /// Return current posterior state variance (`cov`).
    pub fn variance(): f64 {
        return _.cov;
    }

    /// Return the last estimated state `x`.
    pub fn lastMeasurement(): f64 {
        return _.x;
    }

    /// Update measurement noise `Q`.
    pub fn setMeasurementNoise(*_, noise: f64) {
        _.Q = noise;
    }

    /// Update process noise `R`.
    pub fn setProcessNoise(*_, noise: f64) {
        _.R = noise;
    }

    /// Reset filter state, covariance, and initialization flag.
    pub fn reset(*_) {
        _.x = 0.0;
        _.cov = 0.0;
        _.initialized = false;
    }
}

export {
    Kalman1D,
    KalmanFilter,
};

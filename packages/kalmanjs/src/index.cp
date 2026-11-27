// packages/@cap/kalman1d/src/index.cp
// Zero-allocation 1D scalar Kalman filter (kalmanjs parity).
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

    /// Predict the next state given control input `u` without mutating internal state.
    pub fn predict(u: f64): f64 {
        return (_.a * _.x) + (_.b * u);
    }

    /// Return the current estimated state `x`.
    pub fn lastMeasurement(): f64 {
        return _.x;
    }

    /// Reset estimated state, covariance, and initialization flag.
    pub fn reset(*_) {
        _.x = 0.0 as f64;
        _.cov = 0.0 as f64;
        _.initialized = false;
    }
}

export {
    Kalman1D,
};

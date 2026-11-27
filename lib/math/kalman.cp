// Origin: Pure-CP Stdlib (Zero-Alloc 1D/2D Joseph-Form Kalman & Alpha-Beta Sensor Fusion Filters) | Created: 2026-11-22

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

/// Scalar 1D discrete Kalman filter with Joseph-form covariance update.
pub struct Kalman1D {
    x: f64;
    p: f64;
    q: f64;
    r: f64;
    last_gain: f64;
    last_innovation: f64;

    /// Initializes a 1D Kalman filter.
    /// Requires finite inputs, `p0 >= 0`, `q >= 0`, and `r > 0`.
    pub static fn init(x0: f64, p0: f64, q: f64, r: f64): Kalman1D? {
        if (!isFiniteF64(x0) || !isFiniteF64(p0) || !isFiniteF64(q) || !isFiniteF64(r)) {
            return null;
        }
        if (p0 < 0.0 || q < 0.0 || r <= 0.0) {
            return null;
        }
        return Kalman1D {
            x: x0,
            p: p0,
            q: q,
            r: r,
            last_gain: 0.0,
            last_innovation: 0.0,
        };
    }

    /// Time-update (prediction) step with optional control input (`x += b * control`, `p += q`).
    fn predict(*_, control: f64, b: f64) {
        _.x = _.x + b * control;
        _.p = _.p + _.q;
    }

    /// Measurement-update step using numerically stable Joseph-form variance update:
    /// `p = (1 - K)^2 * p + K^2 * r`. Returns updated state estimate `x`.
    fn update(*_, z: f64): f64 {
        const s = _.p + _.r;
        const k = _.p / s;
        const innov = z - _.x;
        _.x = _.x + k * innov;
        const omk = 1.0 - k;
        _.p = omk * omk * _.p + k * k * _.r;
        _.last_gain = k;
        _.last_innovation = innov;
        return _.x;
    }

    /// Convenience combined predict (zero control) + measurement update step.
    fn step(*_, z: f64): f64 {
        _.predict(0.0, 0.0);
        return _.update(z);
    }
}

/// 2D constant-velocity `[position, velocity]` Kalman filter with symmetric 2x2 Joseph covariance update.
pub struct Kalman2D {
    pos: f64;
    vel: f64;
    p00: f64;
    p01: f64;
    p11: f64;
    accel_var: f64;
    meas_var: f64;

    /// Initializes a 2D constant-velocity Kalman filter.
    /// Requires finite inputs, `p_pos0 >= 0`, `p_vel0 >= 0`, `accel_var >= 0`, and `meas_var > 0`.
    pub static fn init(pos0: f64, vel0: f64, p_pos0: f64, p_vel0: f64, accel_var: f64, meas_var: f64): Kalman2D? {
        if (!isFiniteF64(pos0) || !isFiniteF64(vel0) || !isFiniteF64(p_pos0) || !isFiniteF64(p_vel0) || !isFiniteF64(accel_var) || !isFiniteF64(meas_var)) {
            return null;
        }
        if (p_pos0 < 0.0 || p_vel0 < 0.0 || accel_var < 0.0 || meas_var <= 0.0) {
            return null;
        }
        return Kalman2D {
            pos: pos0,
            vel: vel0,
            p00: p_pos0,
            p01: 0.0,
            p11: p_vel0,
            accel_var: accel_var,
            meas_var: meas_var,
        };
    }

    /// Predicts state and covariance forward by `dt > 0` using continuous white-noise acceleration `Q`.
    /// Returns `false` if `dt` is non-positive or non-finite.
    fn predict(*_, dt: f64): bool {
        if (!isFiniteF64(dt) || dt <= 0.0) {
            return false;
        }
        _.pos = _.pos + _.vel * dt;

        const dt2 = dt * dt;
        const dt3 = dt2 * dt;
        const dt4 = dt2 * dt2;

        const fp00 = _.p00 + 2.0 * dt * _.p01 + dt2 * _.p11;
        const fp01 = _.p01 + dt * _.p11;
        const fp11 = _.p11;

        _.p00 = fp00 + _.accel_var * (0.25 * dt4);
        _.p01 = fp01 + _.accel_var * (0.5 * dt3);
        _.p11 = fp11 + _.accel_var * dt2;
        return true;
    }

    /// Updates state from a scalar position measurement `z_pos` (`H = [1, 0]`)
    /// using the 2x2 Joseph-form covariance update. Returns updated `pos`.
    fn updatePosition(*_, z_pos: f64): f64 {
        const s = _.p00 + _.meas_var;
        const k0 = _.p00 / s;
        const k1 = _.p01 / s;
        const y = z_pos - _.pos;

        _.pos = _.pos + k0 * y;
        _.vel = _.vel + k1 * y;

        // Joseph form: P' = (I - K H) P (I - K H)^T + K R K^T
        const omk0: f64 = 1.0 - k0;
        const nk1: f64 = 0.0 - k1;
        const m00: f64 = omk0 * _.p00;
        const m01: f64 = omk0 * _.p01;
        const m10: f64 = nk1 * _.p00 + _.p01;
        const m11: f64 = nk1 * _.p01 + _.p11;

        _.p00 = m00 * omk0 + k0 * k0 * _.meas_var;
        _.p01 = m00 * nk1 + m01 + k0 * k1 * _.meas_var;
        _.p11 = m10 * nk1 + m11 + k1 * k1 * _.meas_var;
        return _.pos;
    }

    /// Updates state from a scalar velocity measurement `z_vel` (`H = [0, 1]`) with variance `vel_var > 0`.
    /// Returns updated `vel`, or `null` if `z_vel` or `vel_var` is invalid.
    fn updateVelocity(*_, z_vel: f64, vel_var: f64): f64? {
        if (!isFiniteF64(z_vel) || !isFiniteF64(vel_var) || vel_var <= 0.0) {
            return null;
        }
        const s = _.p11 + vel_var;
        const k0 = _.p01 / s;
        const k1 = _.p11 / s;
        const y = z_vel - _.vel;

        _.pos = _.pos + k0 * y;
        _.vel = _.vel + k1 * y;

        // Joseph form for H = [0, 1]
        const omk1 = 1.0 - k1;
        const m00 = _.p00 - k0 * _.p01;
        const m01 = _.p01 - k0 * _.p11;
        const m11 = omk1 * _.p11;

        _.p00 = m00 - m01 * k0 + k0 * k0 * vel_var;
        _.p01 = m01 * omk1 + k0 * k1 * vel_var;
        _.p11 = m11 * omk1 + k1 * k1 * vel_var;
        return _.vel;
    }

    /// Returns squared Mahalanobis innovation distance `(z_pos - pos)^2 / (p00 + meas_var)`
    /// for chi-squared outlier gating.
    fn mahalanobisSq(z_pos: f64): f64 {
        const innov = z_pos - _.pos;
        return (innov * innov) / (_.p00 + _.meas_var);
    }
}

/// Lightweight constant-gain Alpha-Beta position/velocity tracking filter.
pub struct AlphaBetaFilter {
    pos: f64;
    vel: f64;
    alpha: f64;
    beta: f64;

    /// Initializes an `AlphaBetaFilter`.
    /// Requires finite inputs, `0 < alpha < 1`, and `0 < beta <= 2`.
    pub static fn init(pos0: f64, vel0: f64, alpha: f64, beta: f64): AlphaBetaFilter? {
        if (!isFiniteF64(pos0) || !isFiniteF64(vel0) || !isFiniteF64(alpha) || !isFiniteF64(beta)) {
            return null;
        }
        if (alpha <= 0.0 || alpha >= 1.0 || beta <= 0.0 || beta > 2.0) {
            return null;
        }
        return AlphaBetaFilter {
            pos: pos0,
            vel: vel0,
            alpha: alpha,
            beta: beta,
        };
    }

    /// Predicts and updates position/velocity from measurement `z_pos` over step `dt > 0`.
    /// Returns updated `pos`, or `null` if `z_pos` is non-finite or `dt <= 0`.
    fn update(*_, z_pos: f64, dt: f64): f64? {
        if (!isFiniteF64(z_pos) || !isFiniteF64(dt) || dt <= 0.0) {
            return null;
        }
        const pred_pos = _.pos + _.vel * dt;
        const residual = z_pos - pred_pos;
        _.pos = pred_pos + _.alpha * residual;
        _.vel = _.vel + (_.beta / dt) * residual;
        return _.pos;
    }
}

// @cap/pid — Zero-allocation Industrial PID Closed-Loop Feedback Controller
// Clean-room MIT implementation in pure CP.
// Features independent P/I/D term clamping, integral anti-windup, derivative-on-measurement,
// output saturation limits, and dt <= 0 division-by-zero protection.

pub struct PidOutput {
    pub output: f64,
    pub p: f64,
    pub i: f64,
    pub d: f64,
}

fn clampRange(v: f64, lo: f64, hi: f64) -> f64 {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

fn clampSymmetric(v: f64, lim: f64) -> f64 {
    if (lim <= 0.0) {
        return v;
    }
    if (v > lim) {
        return lim;
    }
    if (v < -lim) {
        return -lim;
    }
    return v;
}

pub struct PidController {
    setpoint: f64,
    kp: f64,
    ki: f64,
    kd: f64,
    p_limit: f64,
    i_limit: f64,
    d_limit: f64,
    out_min: f64,
    out_max: f64,
    has_out_limits: bool,
    integral: f64,
    prev_measurement: f64,
    has_prev: bool,

    /// Initializes a PID controller with the given setpoint and gains (kp, ki, kd).
    pub static fn init(setpoint: f64, kp: f64, ki: f64, kd: f64) -> PidController {
        return PidController {
            setpoint: setpoint,
            kp: kp,
            ki: ki,
            kd: kd,
            p_limit: 0.0,
            i_limit: 0.0,
            d_limit: 0.0,
            out_min: 0.0,
            out_max: 0.0,
            has_out_limits: false,
            integral: 0.0,
            prev_measurement: 0.0,
            has_prev: false,
        };
    }

    /// Sets total output saturation bounds `[out_min, out_max]`.
    pub fn setLimits(*_, out_min: f64, out_max: f64) {
        if (out_min <= out_max) {
            _.out_min = out_min;
            _.out_max = out_max;
        } else {
            _.out_min = out_max;
            _.out_max = out_min;
        }
        _.has_out_limits = true;
    }

    /// Sets symmetric clamping limits for P, I (anti-windup), and D terms.
    /// Pass `<= 0.0` for any term to leave it unbounded.
    pub fn setTermLimits(*_, p_limit: f64, i_limit: f64, d_limit: f64) {
        _.p_limit = p_limit;
        _.i_limit = i_limit;
        _.d_limit = d_limit;
        if (_.i_limit > 0.0) {
            _.integral = clampSymmetric(_.integral, _.i_limit);
        }
    }

    /// Updates the target setpoint without causing a derivative kick.
    pub fn setSetpoint(*_, setpoint: f64) {
        _.setpoint = setpoint;
    }

    /// Returns the current target setpoint.
    pub fn setpointValue() -> f64 {
        return _.setpoint;
    }

    /// Returns the current accumulated integral term.
    pub fn integralValue() -> f64 {
        return _.integral;
    }

    /// Advances the controller by one time step `dt` given `measurement`.
    /// Safely skips integral/derivative updates when `dt <= 0.0`.
    pub fn step(*_, measurement: f64, dt: f64) -> PidOutput {
        const err = _.setpoint - measurement;

        const p_term = clampSymmetric(_.kp * err, _.p_limit);

        if (dt > 0.0) {
            _.integral += _.ki * err * dt;
            _.integral = clampSymmetric(_.integral, _.i_limit);
        }
        const i_term = _.integral;

        let d_term: f64 = 0.0;
        if (_.has_prev && dt > 0.0) {
            const d_raw = -_.kd * (measurement - _.prev_measurement) / dt;
            d_term = clampSymmetric(d_raw, _.d_limit);
        }

        _.prev_measurement = measurement;
        _.has_prev = true;

        let out = p_term + i_term + d_term;
        if (_.has_out_limits) {
            out = clampRange(out, _.out_min, _.out_max);
        }

        return PidOutput {
            output: out,
            p: p_term,
            i: i_term,
            d: d_term,
        };
    }

    /// Clears accumulated integral and prior measurement history.
    pub fn reset(*_) {
        _.integral = 0.0;
        _.prev_measurement = 0.0;
        _.has_prev = false;
    }
}

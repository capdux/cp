// @cap/node-pid-controller — Zero-heap industrial PID controller with anti-windup,
// derivative-on-measurement, low-pass derivative filtering & output clamping in pure CP.
// Clean-room MIT implementation inspired by npm `node-pid-controller` & crates.io `pid_control`.

fn clampVal(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

/// Configuration struct for constructing a `PidController` with all parameters up front.
pub struct PidConfig {
    pub kp: f64,
    pub ki: f64,
    pub kd: f64,
    pub dt: f64,
    pub setpoint: f64,
    pub i_min: f64,
    pub i_max: f64,
    pub out_min: f64,
    pub out_max: f64,
    pub d_filter_alpha: f64,
    pub derivative_on_measurement: bool,

    pub static fn defaults(kp: f64, ki: f64, kd: f64): PidConfig {
        return PidConfig {
            kp: kp,
            ki: ki,
            kd: kd,
            dt: 1.0,
            setpoint: 0.0,
            i_min: -1000000000.0,
            i_max: 1000000000.0,
            out_min: -1000000000.0,
            out_max: 1000000000.0,
            d_filter_alpha: 1.0,
            derivative_on_measurement: true,
        };
    }
}

/// Diagnostic breakdown of the most recent PID computation.
pub struct PidTerms {
    pub p: f64,
    pub i: f64,
    pub d: f64,
    pub output: f64,
}

pub struct PidController {
    pub kp: f64,
    pub ki: f64,
    pub kd: f64,
    pub dt: f64,
    pub setpoint: f64,
    pub integral: f64,
    pub prev_measurement: f64,
    pub prev_error: f64,
    pub prev_derivative: f64,
    pub has_prev: bool,
    pub out_min: f64,
    pub out_max: f64,
    pub integral_min: f64,
    pub integral_max: f64,
    pub d_filter_alpha: f64,
    pub derivative_on_measurement: bool,
    pub last_p: f64,
    pub last_i: f64,
    pub last_d: f64,
    pub last_output: f64,

    pub static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController {
        return PidController {
            kp: kp,
            ki: ki,
            kd: kd,
            dt: 1.0,
            setpoint: setpoint,
            integral: 0.0,
            prev_measurement: 0.0,
            prev_error: 0.0,
            prev_derivative: 0.0,
            has_prev: false,
            out_min: -1000000000.0,
            out_max: 1000000000.0,
            integral_min: -1000000000.0,
            integral_max: 1000000000.0,
            d_filter_alpha: 1.0,
            derivative_on_measurement: true,
            last_p: 0.0,
            last_i: 0.0,
            last_d: 0.0,
            last_output: 0.0,
        };
    }

    pub static fn fromConfig(cfg: PidConfig): PidController {
        const eff_dt = if (cfg.dt > 0.0) cfg.dt else 1.0;
        const alpha = clampVal(cfg.d_filter_alpha, 0.0, 1.0);
        const lo_i = if (cfg.i_min <= cfg.i_max) cfg.i_min else cfg.i_max;
        const hi_i = if (cfg.i_min <= cfg.i_max) cfg.i_max else cfg.i_min;
        const lo_o = if (cfg.out_min <= cfg.out_max) cfg.out_min else cfg.out_max;
        const hi_o = if (cfg.out_min <= cfg.out_max) cfg.out_max else cfg.out_min;
        return PidController {
            kp: cfg.kp,
            ki: cfg.ki,
            kd: cfg.kd,
            dt: eff_dt,
            setpoint: cfg.setpoint,
            integral: 0.0,
            prev_measurement: 0.0,
            prev_error: 0.0,
            prev_derivative: 0.0,
            has_prev: false,
            out_min: lo_o,
            out_max: hi_o,
            integral_min: lo_i,
            integral_max: hi_i,
            d_filter_alpha: alpha,
            derivative_on_measurement: cfg.derivative_on_measurement,
            last_p: 0.0,
            last_i: 0.0,
            last_d: 0.0,
            last_output: 0.0,
        };
    }

    pub fn setTunings(*_, kp: f64, ki: f64, kd: f64) {
        _.kp = kp;
        _.ki = ki;
        _.kd = kd;
    }

    pub fn setDt(*_, dt: f64) {
        if (dt > 0.0) {
            _.dt = dt;
        }
    }

    pub fn setOutputLimits(*_, lo: f64, hi: f64) {
        if (lo <= hi) {
            _.out_min = lo;
            _.out_max = hi;
        } else {
            _.out_min = hi;
            _.out_max = lo;
        }
    }

    pub fn setIntegralLimits(*_, lo: f64, hi: f64) {
        if (lo <= hi) {
            _.integral_min = lo;
            _.integral_max = hi;
        } else {
            _.integral_min = hi;
            _.integral_max = lo;
        }
        _.integral = clampVal(_.integral, _.integral_min, _.integral_max);
    }

    /// Set low-pass exponential smoothing factor `alpha` in `[0.0, 1.0]` for the derivative term.
    /// `1.0` disables filtering (raw derivative); smaller values smooth high-frequency sensor noise.
    pub fn setDerivativeFilter(*_, alpha: f64) {
        _.d_filter_alpha = clampVal(alpha, 0.0, 1.0);
    }

    /// Configure whether derivative acts on measurement (`true`, avoids setpoint kick) or on error (`false`).
    pub fn setDerivativeMode(*_, on_measurement: bool) {
        _.derivative_on_measurement = on_measurement;
    }

    pub fn setTarget(*_, target: f64) {
        _.setpoint = target;
    }

    pub fn getError(measurement: f64): f64 {
        return _.setpoint - measurement;
    }

    pub fn getLastTerms(): PidTerms {
        return PidTerms {
            p: _.last_p,
            i: _.last_i,
            d: _.last_d,
            output: _.last_output,
        };
    }

    pub fn reset(*_) {
        _.integral = 0.0;
        _.prev_measurement = 0.0;
        _.prev_error = 0.0;
        _.prev_derivative = 0.0;
        _.has_prev = false;
        _.last_p = 0.0;
        _.last_i = 0.0;
        _.last_d = 0.0;
        _.last_output = 0.0;
    }

    pub fn clone(): PidController {
        return PidController {
            kp: _.kp,
            ki: _.ki,
            kd: _.kd,
            dt: _.dt,
            setpoint: _.setpoint,
            integral: _.integral,
            prev_measurement: _.prev_measurement,
            prev_error: _.prev_error,
            prev_derivative: _.prev_derivative,
            has_prev: _.has_prev,
            out_min: _.out_min,
            out_max: _.out_max,
            integral_min: _.integral_min,
            integral_max: _.integral_max,
            d_filter_alpha: _.d_filter_alpha,
            derivative_on_measurement: _.derivative_on_measurement,
            last_p: _.last_p,
            last_i: _.last_i,
            last_d: _.last_d,
            last_output: _.last_output,
        };
    }

    /// Compute PID control output using the controller's configured `dt`.
    pub fn updateWithDt(*_, measurement: f64): f64 {
        return _.update(measurement, _.dt);
    }

    /// Compute PID control output for `measurement` over time step `dt`.
    /// Uses derivative-on-measurement by default to avoid derivative kick on setpoint step changes.
    pub fn update(*_, measurement: f64, dt: f64): f64 {
        const err = _.setpoint - measurement;
        const p_term = _.kp * err;
        if (dt <= 0.0) {
            const out = clampVal(p_term + _.integral, _.out_min, _.out_max);
            _.last_p = p_term;
            _.last_i = _.integral;
            _.last_d = 0.0;
            _.last_output = out;
            return out;
        }

        _.integral = clampVal(_.integral + _.ki * err * dt, _.integral_min, _.integral_max);

        let d_term: f64 = 0.0;
        if (_.has_prev) {
            const raw_rate = if (_.derivative_on_measurement)
                -(measurement - _.prev_measurement) / dt
            else
                (err - _.prev_error) / dt;
            const alpha = clampVal(_.d_filter_alpha, 0.0, 1.0);
            const filtered_rate = alpha * raw_rate + (1.0 - alpha) * _.prev_derivative;
            _.prev_derivative = filtered_rate;
            d_term = _.kd * filtered_rate;
        } else {
            _.prev_derivative = 0.0;
        }
        _.prev_measurement = measurement;
        _.prev_error = err;
        _.has_prev = true;

        const out = clampVal(p_term + _.integral + d_term, _.out_min, _.out_max);
        _.last_p = p_term;
        _.last_i = _.integral;
        _.last_d = d_term;
        _.last_output = out;
        return out;
    }
}

export {
    PidConfig,
    PidTerms,
    PidController,
};

// @cap/pid-ctrl — Zero-heap industrial PID controller with anti-windup & output clamping
// Clean-room MIT implementation in pure CP.

fn clampVal(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) { return lo; }
    if (x > hi) { return hi; }
    return x;
}

pub struct PidController {
    pub kp: f64,
    pub ki: f64,
    pub kd: f64,
    pub setpoint: f64,
    pub integral: f64,
    pub prev_measurement: f64,
    pub has_prev: bool,
    pub out_min: f64,
    pub out_max: f64,
    pub integral_min: f64,
    pub integral_max: f64,

    pub static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController {
        return PidController {
            kp: kp,
            ki: ki,
            kd: kd,
            setpoint: setpoint,
            integral: 0.0,
            prev_measurement: 0.0,
            has_prev: false,
            out_min: -1000000000.0,
            out_max: 1000000000.0,
            integral_min: -1000000000.0,
            integral_max: 1000000000.0,
        };
    }

    pub fn setOutputLimits(*_, lo: f64, hi: f64) {
        _.out_min = lo;
        _.out_max = hi;
    }

    pub fn setIntegralLimits(*_, lo: f64, hi: f64) {
        _.integral_min = lo;
        _.integral_max = hi;
    }

    pub fn setTarget(*_, target: f64) {
        _.setpoint = target;
    }

    pub fn reset(*_) {
        _.integral = 0.0;
        _.prev_measurement = 0.0;
        _.has_prev = false;
    }

    // Derivative-on-measurement avoids derivative kick when setpoint changes
    pub fn update(*_, measurement: f64, dt: f64): f64 {
        if (dt <= 0.0) {
            return clampVal(_.kp * (_.setpoint - measurement) + _.integral, _.out_min, _.out_max);
        }
        let err = _.setpoint - measurement;
        let p_term = _.kp * err;

        _.integral = clampVal(_.integral + _.ki * err * dt, _.integral_min, _.integral_max);

        let d_term: f64 = 0.0;
        if (_.has_prev) {
            let d_meas = (measurement - _.prev_measurement) / dt;
            d_term = -_.kd * d_meas;
        }
        _.prev_measurement = measurement;
        _.has_prev = true;

        return clampVal(p_term + _.integral + d_term, _.out_min, _.out_max);
    }
}

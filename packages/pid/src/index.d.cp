// Type definitions for CP

export class PidOutput {
    output: f64,
    p: f64,
    i: f64,
    d: f64,
}

export fn clampRange(v: f64, lo: f64, hi: f64): f64;
export fn clampSymmetric(v: f64, lim: f64): f64;
export class PidController {
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
    fn init(setpoint: f64, kp: f64, ki: f64, kd: f64): PidController;
    fn setLimits(_: auto, out_min: f64, out_max: f64);
    fn setTermLimits(_: auto, p_limit: f64, i_limit: f64, d_limit: f64);
    fn setSetpoint(_: auto, setpoint: f64);
    fn setpointValue(): f64;
    fn integralValue(): f64;
    fn step(_: auto, measurement: f64, dt: f64): PidOutput;
    fn reset(_: auto);
}


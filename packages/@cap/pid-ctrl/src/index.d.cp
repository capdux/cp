// Type definitions for CP

export fn clampVal(x: f64, lo: f64, hi: f64): f64;
export class PidController {
    kp: f64,
    ki: f64,
    kd: f64,
    setpoint: f64,
    integral: f64,
    prev_measurement: f64,
    has_prev: bool,
    out_min: f64,
    out_max: f64,
    integral_min: f64,
    integral_max: f64,
    fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController;
    fn setOutputLimits(_: auto, lo: f64, hi: f64);
    fn setIntegralLimits(_: auto, lo: f64, hi: f64);
    fn setTarget(_: auto, target: f64);
    fn reset(_: auto);
    fn update(_: auto, measurement: f64, dt: f64): f64;
}


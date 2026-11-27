// Type definitions for CP

export class PidConfig {
    kp: f64,
    ki: f64,
    kd: f64,
    dt: f64,
    setpoint: f64,
    i_min: f64,
    i_max: f64,
    out_min: f64,
    out_max: f64,
    d_filter_alpha: f64,
    derivative_on_measurement: bool,
    static fn defaults(kp: f64, ki: f64, kd: f64): PidConfig;
}

export class PidTerms {
    p: f64,
    i: f64,
    d: f64,
    output: f64,
}

export class PidController {
    kp: f64,
    ki: f64,
    kd: f64,
    dt: f64,
    setpoint: f64,
    integral: f64,
    prev_measurement: f64,
    prev_error: f64,
    prev_derivative: f64,
    has_prev: bool,
    out_min: f64,
    out_max: f64,
    integral_min: f64,
    integral_max: f64,
    d_filter_alpha: f64,
    derivative_on_measurement: bool,
    last_p: f64,
    last_i: f64,
    last_d: f64,
    last_output: f64,
    static fn new(kp: f64, ki: f64, kd: f64, setpoint: f64): PidController;
    static fn fromConfig(cfg: PidConfig): PidController;
    fn setTunings(_: auto, kp: f64, ki: f64, kd: f64);
    fn setDt(_: auto, dt: f64);
    fn setOutputLimits(_: auto, lo: f64, hi: f64);
    fn setIntegralLimits(_: auto, lo: f64, hi: f64);
    fn setDerivativeFilter(_: auto, alpha: f64);
    fn setDerivativeMode(_: auto, on_measurement: bool);
    fn setTarget(_: auto, target: f64);
    fn getError(measurement: f64): f64;
    fn getLastTerms(): PidTerms;
    fn reset(_: auto);
    fn clone(): PidController;
    fn updateWithDt(_: auto, measurement: f64): f64;
    fn update(_: auto, measurement: f64, dt: f64): f64;
}

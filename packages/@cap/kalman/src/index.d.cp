// Type definitions for CP

export class Kalman1D {
    x: f64,
    p: f64,
    q: f64,
    r: f64,
    k: f64,
    fn init(initial_x: f64, initial_p: f64, process_noise_q: f64, measurement_noise_r: f64): Kalman1D;
    fn predict(_: auto);
    fn update(_: auto, measurement: f64): f64;
    fn step(_: auto, measurement: f64): f64;
    fn estimate(): f64;
    fn variance(): f64;
    fn gain(): f64;
    fn reset(_: auto, new_x: f64, new_p: f64);
}

export class Kalman2D {
    pos: f64,
    vel: f64,
    p00: f64,
    p01: f64,
    p10: f64,
    p11: f64,
    q_pos: f64,
    q_vel: f64,
    r_pos: f64,
    fn init(init_pos: f64, init_vel: f64, q_pos_in: f64, q_vel_in: f64, r_pos_in: f64): Kalman2D;
    fn predict(_: auto, dt: f64);
    fn updatePosition(_: auto, measured_pos: f64): f64;
    fn position(): f64;
    fn velocity(): f64;
}


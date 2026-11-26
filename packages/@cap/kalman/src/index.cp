// @cap/kalman — Zero-allocation 1D scalar & 2D constant-velocity Kalman filters for CP
// Clean-room MIT implementation inspired by Rust `kalman-filters` and `kfilter`.

pub struct Kalman1D {
    pub x: f64,
    pub p: f64,
    pub q: f64,
    pub r: f64,
    pub k: f64,

    pub static fn init(initial_x: f64, initial_p: f64, process_noise_q: f64, measurement_noise_r: f64) -> Kalman1D {
        return Kalman1D{
            x: initial_x,
            p: initial_p,
            q: process_noise_q,
            r: measurement_noise_r,
            k: 0.0,
        };
    }

    pub fn predict(*_) {
        _.p = _.p + _.q;
    }

    pub fn update(*_, measurement: f64) -> f64 {
        const denom = _.p + _.r;
        if (denom <= 0.0) {
            _.k = 1.0;
            _.x = measurement;
            _.p = 0.0;
            return _.x;
        }
        _.k = _.p / denom;
        _.x = _.x + (_.k * (measurement - _.x));
        _.p = (1.0 - _.k) * _.p;
        return _.x;
    }

    pub fn step(*_, measurement: f64) -> f64 {
        _.predict();
        return _.update(measurement);
    }

    pub fn estimate() -> f64 {
        return _.x;
    }

    pub fn variance() -> f64 {
        return _.p;
    }

    pub fn gain() -> f64 {
        return _.k;
    }

    pub fn reset(*_, new_x: f64, new_p: f64) {
        _.x = new_x;
        _.p = new_p;
        _.k = 0.0;
    }
}

pub struct Kalman2D {
    pub pos: f64,
    pub vel: f64,
    pub p00: f64,
    pub p01: f64,
    pub p10: f64,
    pub p11: f64,
    pub q_pos: f64,
    pub q_vel: f64,
    pub r_pos: f64,

    pub static fn init(init_pos: f64, init_vel: f64, q_pos_in: f64, q_vel_in: f64, r_pos_in: f64) -> Kalman2D {
        return Kalman2D{
            pos: init_pos,
            vel: init_vel,
            p00: 1.0,
            p01: 0.0,
            p10: 0.0,
            p11: 1.0,
            q_pos: q_pos_in,
            q_vel: q_vel_in,
            r_pos: r_pos_in,
        };
    }

    pub fn predict(*_, dt: f64) {
        _.pos = _.pos + (_.vel * dt);
        const dt2 = dt * dt;
        const np00 = _.p00 + (dt * (_.p10 + _.p01)) + (dt2 * _.p11) + _.q_pos;
        const np01 = _.p01 + (dt * _.p11);
        const np10 = _.p10 + (dt * _.p11);
        const np11 = _.p11 + _.q_vel;
        _.p00 = np00;
        _.p01 = np01;
        _.p10 = np10;
        _.p11 = np11;
    }

    // Joseph-form numerically stable covariance update: P' = (I - K H) P (I - K H)^T + K R K^T
    pub fn updatePosition(*_, measured_pos: f64) -> f64 {
        const s = _.p00 + _.r_pos;
        if (s <= 0.0) {
            _.pos = measured_pos;
            return _.pos;
        }
        const k0 = _.p00 / s;
        const k1 = _.p10 / s;
        const innov = measured_pos - _.pos;

        _.pos = _.pos + (k0 * innov);
        _.vel = _.vel + (k1 * innov);

        const oneMinusK0 = 1.0 - k0;
        const ap00 = oneMinusK0 * _.p00;
        const ap01 = oneMinusK0 * _.p01;
        const ap10 = (-k1 * _.p00) + _.p10;
        const ap11 = (-k1 * _.p01) + _.p11;

        const np00 = (ap00 * oneMinusK0) + (k0 * k0 * _.r_pos);
        const np01 = (ap00 * (-k1)) + ap01 + (k0 * k1 * _.r_pos);
        const np10 = (ap10 * oneMinusK0) + (k1 * k0 * _.r_pos);
        const np11 = (ap10 * (-k1)) + ap11 + (k1 * k1 * _.r_pos);

        _.p00 = np00;
        _.p01 = np01;
        _.p10 = np10;
        _.p11 = np11;
        return _.pos;
    }

    pub fn position() -> f64 {
        return _.pos;
    }

    pub fn velocity() -> f64 {
        return _.vel;
    }
}

export {
    Kalman1D,
    Kalman2D,
};

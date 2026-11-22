// Origin: Pure-CP Stdlib (Zero-Alloc Industrial Discrete PID Controller with Anti-Windup, Derivative-on-Measurement, LPF & Bumpless Transfer) | Created: 2026-11-22

fn isFiniteF64(x: f64): bool {
    return (x - x) == 0.0;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn clampF64(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

/// Configuration parameters for a discrete-time industrial PID controller.
pub struct PidConfig {
    kp: f64,
    ki: f64,
    kd: f64,
    out_min: f64,
    out_max: f64,
    tau_d: f64,
    derivative_on_measurement: bool,
    deadband: f64,
}

/// Detailed diagnostic breakdown returned by `PidController.updateDetailed`.
pub struct PidTerms {
    p: f64,
    i: f64,
    d: f64,
    output: f64,
    saturated: bool,
}

/// Zero-alloc discrete-time PID controller supporting:
/// - Conditional integration clamping (Anti-Windup)
/// - Derivative-on-measurement (avoids setpoint step kick)
/// - First-order low-pass derivative filtering (`tau_d`)
/// - Error deadband & output clamping
/// - Bumpless manual-to-auto transfer (`setManualOutput`)
pub struct PidController {
    cfg: PidConfig,
    integrator: f64,
    prev_error: f64,
    prev_measurement: f64,
    prev_deriv: f64,
    initialized: bool,

    /// Validates `cfg` (`out_min <= out_max`, `tau_d >= 0`, `deadband >= 0`, all finite)
    /// and returns a zero-initialized `PidController`, or `null` on invalid config.
    fn init(cfg: PidConfig): PidController? {
        if (
            !isFiniteF64(cfg.kp) ||
            !isFiniteF64(cfg.ki) ||
            !isFiniteF64(cfg.kd) ||
            !isFiniteF64(cfg.out_min) ||
            !isFiniteF64(cfg.out_max) ||
            !isFiniteF64(cfg.tau_d) ||
            !isFiniteF64(cfg.deadband)
        ) {
            return null;
        }
        if (cfg.out_min > cfg.out_max || cfg.tau_d < 0.0 || cfg.deadband < 0.0) {
            return null;
        }
        return PidController {
            cfg: cfg,
            integrator: 0.0,
            prev_error: 0.0,
            prev_measurement: 0.0,
            prev_deriv: 0.0,
            initialized: false,
        };
    }

    /// Resets internal integrator and derivative filter states.
    fn reset(*_) {
        _.integrator = 0.0;
        _.prev_error = 0.0;
        _.prev_measurement = 0.0;
        _.prev_deriv = 0.0;
        _.initialized = false;
    }

    /// Performs industrial bumpless transfer from manual to automatic mode by
    /// preloading `integrator` with clamped `output` and synchronizing `prev_measurement`.
    fn setManualOutput(*_, output: f64, measurement: f64) {
        if (!isFiniteF64(output) || !isFiniteF64(measurement)) {
            return;
        }
        _.integrator = clampF64(output, _.cfg.out_min, _.cfg.out_max);
        _.prev_measurement = measurement;
        _.prev_error = 0.0;
        _.prev_deriv = 0.0;
        _.initialized = true;
    }

    /// Advances the PID controller by one time step `dt` and returns full P/I/D terms
    /// and saturation status. Returns `null` if `dt <= 0` or any input is NaN/Inf.
    fn updateDetailed(*_, setpoint: f64, measurement: f64, dt: f64): PidTerms? {
        if (!isFiniteF64(setpoint) || !isFiniteF64(measurement) || !isFiniteF64(dt) || dt <= 0.0) {
            return null;
        }

        let err = setpoint - measurement;
        if (absF64(err) <= _.cfg.deadband) {
            err = 0.0;
        }

        // Proportional term
        const p_term = _.cfg.kp * err;

        // Derivative term with optional first-order LPF and derivative-on-measurement
        let filtered_deriv: f64 = 0.0;
        if (_.initialized) {
            const raw_deriv: f64 = if (_.cfg.derivative_on_measurement)
                -(measurement - _.prev_measurement) / dt
            else
                (err - _.prev_error) / dt;

            if (_.cfg.tau_d > 0.0) {
                const alpha = dt / (_.cfg.tau_d + dt);
                filtered_deriv = alpha * raw_deriv + (1.0 - alpha) * _.prev_deriv;
            } else {
                filtered_deriv = raw_deriv;
            }
        }
        const d_term = _.cfg.kd * filtered_deriv;

        // Integral term with conditional integration clamping (Anti-Windup)
        const di = _.cfg.ki * err * dt;
        const cand_i = _.integrator + di;
        const raw_out = p_term + cand_i + d_term;

        let out = raw_out;
        let saturated = false;
        if (raw_out > _.cfg.out_max) {
            out = _.cfg.out_max;
            saturated = true;
            // Only integrate if moving away from upper saturation
            if (di < 0.0) {
                _.integrator = clampF64(cand_i, _.cfg.out_min, _.cfg.out_max);
            }
        } else if (raw_out < _.cfg.out_min) {
            out = _.cfg.out_min;
            saturated = true;
            // Only integrate if moving away from lower saturation
            if (di > 0.0) {
                _.integrator = clampF64(cand_i, _.cfg.out_min, _.cfg.out_max);
            }
        } else {
            _.integrator = cand_i;
        }

        _.prev_error = err;
        _.prev_measurement = measurement;
        _.prev_deriv = filtered_deriv;
        _.initialized = true;

        return PidTerms {
            p: p_term,
            i: _.integrator,
            d: d_term,
            output: out,
            saturated: saturated,
        };
    }

    /// Advances the PID controller by one time step `dt` and returns the clamped control output.
    /// Returns `null` if `dt <= 0` or any input is NaN/Inf.
    fn update(*_, setpoint: f64, measurement: f64, dt: f64): f64? {
        const terms = _.updateDetailed(setpoint, measurement, dt) ?? return null;
        return terms.output;
    }
}

/// Computes classic Ziegler-Nichols closed-loop PID tuning gains from ultimate gain `ku`
/// and oscillation period `tu`: `kp = 0.6 * ku`, `ki = 1.2 * ku / tu`, `kd = 0.075 * ku * tu`.
/// Returns `null` if `ku <= 0`, `tu <= 0`, or either is non-finite.
pub fn zieglerNicholsPid(ku: f64, tu: f64): PidConfig? {
    if (!isFiniteF64(ku) || !isFiniteF64(tu) || ku <= 0.0 || tu <= 0.0) {
        return null;
    }
    return PidConfig {
        kp: 0.6 * ku,
        ki: 1.2 * ku / tu,
        kd: 0.075 * ku * tu,
        out_min: -1.0e9,
        out_max: 1.0e9,
        tau_d: 0.0,
        derivative_on_measurement: true,
        deadband: 0.0,
    };
}

/// Applies slew-rate / acceleration limiting to move `current` toward `target`
/// by at most `max_rate * dt` per step.
pub fn rateLimit(current: f64, target: f64, max_rate: f64, dt: f64): f64 {
    if (
        !isFiniteF64(current) ||
        !isFiniteF64(target) ||
        !isFiniteF64(max_rate) ||
        !isFiniteF64(dt) ||
        max_rate <= 0.0 ||
        dt <= 0.0
    ) {
        return current;
    }
    const max_step = max_rate * dt;
    const delta = target - current;
    if (delta > max_step) {
        return current + max_step;
    }
    if (delta < -max_step) {
        return current - max_step;
    }
    return target;
}

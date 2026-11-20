// Source: cp/lib/tween/tween.cp (Easing Curves, KeyframeTrack & SpringDamper Engine)
// Pure CP, zero-C-dependency kinematics easing, keyframe interpolation, and spring-damper physics engine.

const std = @import("std");

const SPRING_MAX_SUBSTEP: f64 = 1.0 / 240.0;
const SPRING_MIN_SUBSTEP: f64 = 0.00001;

/// Error set for tween, keyframe track, and spring-damper operations.
pub err TweenError {
    InvalidKeyframeOrder,
    EmptyTrack,
    InvalidSpringParams,
    InvalidDuration,
    OutOfMemory,
}

/// Classic Robert Penner easing curve family.
pub enum EaseKind {
    Linear,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    InQuart,
    OutQuart,
    InOutQuart,
    InQuint,
    OutQuint,
    InOutQuint,
    InSine,
    OutSine,
    InOutSine,
    InExpo,
    OutExpo,
    InOutExpo,
    InCirc,
    OutCirc,
    InOutCirc,
    InBack,
    OutBack,
    InOutBack,
    InElastic,
    OutElastic,
    InOutElastic,
    InBounce,
    OutBounce,
    InOutBounce,
}

fn bounceOut(x_in: f64): f64 {
    const n1: f64 = 7.5625;
    const d1: f64 = 2.75;
    let x = x_in;
    if (x < 1.0 / d1) {
        return n1 * x * x;
    } else if (x < 2.0 / d1) {
        x -= 1.5 / d1;
        return n1 * x * x + 0.75;
    } else if (x < 2.5 / d1) {
        x -= 2.25 / d1;
        return n1 * x * x + 0.9375;
    } else {
        x -= 2.625 / d1;
        return n1 * x * x + 0.984375;
    }
}

/// Pure static easing & interpolation utilities.
pub static class Easing {
    /// Clamps `t` to the closed unit interval `[0.0, 1.0]`.
    fn clamp01(t: f64): f64 {
        if (std.math.isNan(t)) {
            return 0.0;
        }
        if (t <= 0.0) {
            return 0.0;
        }
        if (t >= 1.0) {
            return 1.0;
        }
        return t;
    }

    /// Linearly interpolates between `a` and `b` by unclamped factor `t`.
    fn lerp(a: f64, b: f64, t: f64): f64 {
        return a + (b - a) * t;
    }

    /// Interpolates between `a` and `b` using easing curve `kind` at normalized time `t`.
    fn lerpEased(a: f64, b: f64, t: f64, kind: EaseKind): f64 {
        return Easing.lerp(a, b, Easing.sample(kind, t));
    }

    /// Hermite smoothstep interpolation in `[0.0, 1.0]` between `edge0` and `edge1`.
    fn smoothstep(edge0: f64, edge1: f64, x: f64): f64 {
        if (edge0 == edge1) {
            if (x < edge0) {
                return 0.0;
            }
            return 1.0;
        }
        const t = Easing.clamp01((x - edge0) / (edge1 - edge0));
        return t * t * (3.0 - 2.0 * t);
    }

    /// Samples an easing curve at `t`. Strictly returns `0.0` for `t <= 0.0` and `1.0` for `t >= 1.0`.
    fn sample(kind: EaseKind, t: f64): f64 {
        if (std.math.isNan(t)) {
            return 0.0;
        }
        if (t <= 0.0) {
            return 0.0;
        }
        if (t >= 1.0) {
            return 1.0;
        }

        switch (kind) {
            .Linear => return t,
            .InQuad => return t * t,
            .OutQuad => {
                const u = 1.0 - t;
                return 1.0 - u * u;
            },
            .InOutQuad => {
                if (t < 0.5) {
                    return 2.0 * t * t;
                }
                const u = -2.0 * t + 2.0;
                return 1.0 - (u * u) * 0.5;
            },
            .InCubic => return t * t * t,
            .OutCubic => {
                const u = 1.0 - t;
                return 1.0 - u * u * u;
            },
            .InOutCubic => {
                if (t < 0.5) {
                    return 4.0 * t * t * t;
                }
                const u = -2.0 * t + 2.0;
                return 1.0 - (u * u * u) * 0.5;
            },
            .InQuart => {
                const t2 = t * t;
                return t2 * t2;
            },
            .OutQuart => {
                const u = 1.0 - t;
                const u_sq = u * u;
                return 1.0 - u_sq * u_sq;
            },
            .InOutQuart => {
                if (t < 0.5) {
                    const t2 = t * t;
                    return 8.0 * t2 * t2;
                }
                const u = -2.0 * t + 2.0;
                const u_sq = u * u;
                return 1.0 - (u_sq * u_sq) * 0.5;
            },
            .InQuint => {
                const t2 = t * t;
                return t2 * t2 * t;
            },
            .OutQuint => {
                const u = 1.0 - t;
                const u_sq = u * u;
                return 1.0 - u_sq * u_sq * u;
            },
            .InOutQuint => {
                if (t < 0.5) {
                    const t2 = t * t;
                    return 16.0 * t2 * t2 * t;
                }
                const u = -2.0 * t + 2.0;
                const u_sq = u * u;
                return 1.0 - (u_sq * u_sq * u) * 0.5;
            },
            .InSine => return 1.0 - @cos((t * std.math.pi) * 0.5),
            .OutSine => return @sin((t * std.math.pi) * 0.5),
            .InOutSine => return -((@cos(std.math.pi * t) - 1.0) * 0.5),
            .InExpo => return std.math.pow(f64, 2.0, 10.0 * t - 10.0),
            .OutExpo => return 1.0 - std.math.pow(f64, 2.0, -10.0 * t),
            .InOutExpo => {
                if (t < 0.5) {
                    return std.math.pow(f64, 2.0, 20.0 * t - 10.0) * 0.5;
                }
                return (2.0 - std.math.pow(f64, 2.0, -20.0 * t + 10.0)) * 0.5;
            },
            .InCirc => return 1.0 - @sqrt(1.0 - t * t),
            .OutCirc => {
                const u = t - 1.0;
                return @sqrt(1.0 - u * u);
            },
            .InOutCirc => {
                if (t < 0.5) {
                    return (1.0 - @sqrt(1.0 - 4.0 * t * t)) * 0.5;
                }
                const u = -2.0 * t + 2.0;
                return (@sqrt(1.0 - u * u) + 1.0) * 0.5;
            },
            .InBack => {
                const c1: f64 = 1.70158;
                const c3: f64 = c1 + 1.0;
                return c3 * t * t * t - c1 * t * t;
            },
            .OutBack => {
                const c1: f64 = 1.70158;
                const c3: f64 = c1 + 1.0;
                const u = t - 1.0;
                return 1.0 + c3 * u * u * u + c1 * u * u;
            },
            .InOutBack => {
                const c1: f64 = 1.70158;
                const c2: f64 = c1 * 1.525;
                if (t < 0.5) {
                    const u = 2.0 * t;
                    return (u * u * ((c2 + 1.0) * u - c2)) * 0.5;
                }
                const u = 2.0 * t - 2.0;
                return (u * u * ((c2 + 1.0) * u + c2) + 2.0) * 0.5;
            },
            .InElastic => {
                const c4: f64 = (2.0 * std.math.pi) / 3.0;
                return (0.0 - std.math.pow(f64, 2.0, 10.0 * t - 10.0)) * @sin((t * 10.0 - 10.75) * c4);
            },
            .OutElastic => {
                const c4: f64 = (2.0 * std.math.pi) / 3.0;
                return std.math.pow(f64, 2.0, -10.0 * t) * @sin((t * 10.0 - 0.75) * c4) + 1.0;
            },
            .InOutElastic => {
                const c5: f64 = (2.0 * std.math.pi) / 4.5;
                if (t < 0.5) {
                    return (0.0 - (std.math.pow(f64, 2.0, 20.0 * t - 10.0) * @sin((20.0 * t - 11.125) * c5))) * 0.5;
                }
                return (std.math.pow(f64, 2.0, -20.0 * t + 10.0) * @sin((20.0 * t - 11.125) * c5)) * 0.5 + 1.0;
            },
            .InBounce => return 1.0 - bounceOut(1.0 - t),
            .OutBounce => return bounceOut(t),
            .InOutBounce => {
                if (t < 0.5) {
                    return (1.0 - bounceOut(1.0 - 2.0 * t)) * 0.5;
                }
                return (1.0 + bounceOut(2.0 * t - 1.0)) * 0.5;
            },
        }
    }
}

/// Single-segment tween controller supporting yoyo oscillation and finite repeats.
/// `repeat_count == 0` runs 1 pass; `repeat_count == N` runs `N + 1` passes total.
pub struct Tween {
    duration_ms: f64;
    kind: EaseKind;
    yoyo: bool;
    repeat_count: u32;
    elapsed_ms: f64 = 0.0;
    completed_passes: u32 = 0;
    finished: bool = false;

    /// Initializes a `Tween`. Returns `InvalidDuration` if `duration_ms <= 0.0` or non-finite.
    static fn init(duration_ms: f64, kind: EaseKind, yoyo: bool, repeat_count: u32): Tween ! TweenError {
        if (!std.math.isFinite(duration_ms) || duration_ms <= 0.0) {
            return TweenError.InvalidDuration;
        }
        return Tween{
            duration_ms: duration_ms,
            kind: kind,
            yoyo: yoyo,
            repeat_count: repeat_count,
            elapsed_ms: 0.0,
            completed_passes: 0,
            finished: false,
        };
    }

    /// Advances the tween by `dt_ms` milliseconds and returns the eased progress in `[0.0, 1.0]` (or overshot for Back/Elastic).
    fn update(*_, dt_ms: f64): f64 {
        if (_.finished) {
            return _.progress();
        }
        if (dt_ms > 0.0 && std.math.isFinite(dt_ms)) {
            _.elapsed_ms += dt_ms;
            while (_.elapsed_ms >= _.duration_ms) {
                if (_.completed_passes < _.repeat_count) {
                    _.elapsed_ms -= _.duration_ms;
                    _.completed_passes += 1;
                } else {
                    _.elapsed_ms = _.duration_ms;
                    _.finished = true;
                    break;
                }
            }
        }
        return _.progress();
    }

    /// Returns the current eased progress without advancing time.
    fn progress(): f64 {
        let t = Easing.clamp01(_.elapsed_ms / _.duration_ms);
        if (_.yoyo && (_.completed_passes % 2 == 1)) {
            t = 1.0 - t;
        }
        return Easing.sample(_.kind, t);
    }

    /// Returns `true` once all passes have completed.
    fn isFinished(): bool {
        return _.finished;
    }

    /// Resets elapsed time and pass counters to the initial state.
    fn reset(*_) {
        _.elapsed_ms = 0.0;
        _.completed_passes = 0;
        _.finished = false;
    }
}

/// A single timed keyframe with an outgoing easing curve toward the next keyframe.
pub struct Keyframe {
    time: f64;
    value: f64;
    ease: EaseKind;
}

/// Multi-segment keyframe track with monotonic time enforcement, out-of-range clamping, and O(log N) lookup.
pub struct KeyframeTrack {
    alloc: std.mem.Allocator;
    keyframes: std.ArrayList(Keyframe) = .empty;

    /// Initializes an empty `KeyframeTrack`.
    static fn init(alloc: std.mem.Allocator): KeyframeTrack {
        return KeyframeTrack{
            alloc: alloc,
            keyframes: .empty,
        };
    }

    /// Releases internal keyframe storage.
    fn deinit(*_) {
        _.keyframes.deinit(_.alloc);
    }

    /// Returns the number of keyframes in the track.
    fn len(): usize {
        return _.keyframes.items.len;
    }

    /// Appends a keyframe. Enforces strictly increasing `time`, returning `InvalidKeyframeOrder` otherwise.
    fn addKeyframe(*_, time: f64, value: f64, ease: EaseKind): ! TweenError {
        if (!std.math.isFinite(time) || !std.math.isFinite(value)) {
            return TweenError.InvalidKeyframeOrder;
        }
        if (_.keyframes.items.len > 0) {
            const prev_time = _.keyframes.items[_.keyframes.items.len - 1].time;
            if (time <= prev_time) {
                return TweenError.InvalidKeyframeOrder;
            }
        }
        _.keyframes.append(_.alloc, Keyframe{
            time: time,
            value: value,
            ease: ease,
        }) catch return TweenError.OutOfMemory;
    }

    /// Evaluates the track at `time` with endpoint clamping and piecewise easing interpolation.
    fn evaluate(time: f64): f64 ! TweenError {
        const count = _.keyframes.items.len;
        if (count == 0) {
            return TweenError.EmptyTrack;
        }
        if (count == 1) {
            return _.keyframes.items[0].value;
        }
        if (time <= _.keyframes.items[0].time) {
            return _.keyframes.items[0].value;
        }
        const last = count - 1;
        if (time >= _.keyframes.items[last].time) {
            return _.keyframes.items[last].value;
        }

        // Binary search for the segment [lo, lo + 1] containing `time`.
        let lo: usize = 0;
        let hi: usize = last;
        while (lo + 1 < hi) {
            const mid = lo + (hi - lo) / 2;
            if (_.keyframes.items[mid].time <= time) {
                lo = mid;
            } else {
                hi = mid;
            }
        }

        const k0 = _.keyframes.items[lo];
        const k1 = _.keyframes.items[lo + 1];
        const dt = k1.time - k0.time;
        const local_t = if (dt > 0.0) (time - k0.time) / dt else 1.0;
        return Easing.lerpEased(k0.value, k1.value, local_t, k0.ease);
    }
}

/// Continuous-time second-order spring-damper physical system (unit mass):
/// `a = -stiffness * (pos - target) - damping * vel`.
/// Uses fixed/adaptive internal sub-stepping with symplectic Euler to prevent divergence on large `dt`.
pub struct SpringDamper {
    stiffness: f64;
    damping: f64;
    pos: f64;
    vel: f64;
    target: f64;

    /// Initializes a `SpringDamper`. Requires `stiffness > 0.0` and `damping >= 0.0`.
    static fn init(stiffness: f64, damping: f64, initial_val: f64): SpringDamper ! TweenError {
        if (!std.math.isFinite(stiffness) || !std.math.isFinite(damping) || !std.math.isFinite(initial_val)) {
            return TweenError.InvalidSpringParams;
        }
        if (stiffness <= 0.0 || damping < 0.0) {
            return TweenError.InvalidSpringParams;
        }
        return SpringDamper{
            stiffness: stiffness,
            damping: damping,
            pos: initial_val,
            vel: 0.0,
            target: initial_val,
        };
    }

    /// Convenience constructor using natural angular frequency `omega` (rad/s) and `damping_ratio` (zeta).
    static fn initWithRatio(omega: f64, damping_ratio: f64, initial_val: f64): SpringDamper ! TweenError {
        if (!std.math.isFinite(omega) || !std.math.isFinite(damping_ratio) || !std.math.isFinite(initial_val)) {
            return TweenError.InvalidSpringParams;
        }
        if (omega <= 0.0 || damping_ratio < 0.0) {
            return TweenError.InvalidSpringParams;
        }
        const stiffness = omega * omega;
        const damping = 2.0 * damping_ratio * omega;
        return SpringDamper{
            stiffness: stiffness,
            damping: damping,
            pos: initial_val,
            vel: 0.0,
            target: initial_val,
        };
    }

    /// Sets a new equilibrium target position.
    fn setTarget(*_, target: f64) {
        if (std.math.isFinite(target)) {
            _.target = target;
        }
    }

    /// Advances the spring simulation by `dt_sec` seconds using stability-bounded internal sub-steps.
    fn step(*_, dt_sec: f64) {
        if (!std.math.isFinite(dt_sec) || dt_sec <= 0.0) {
            return;
        }
        const freq_step = 0.25 / @sqrt(_.stiffness);
        let max_h = if (freq_step < SPRING_MAX_SUBSTEP) freq_step else SPRING_MAX_SUBSTEP;
        if (max_h < SPRING_MIN_SUBSTEP) {
            max_h = SPRING_MIN_SUBSTEP;
        }

        let remaining = dt_sec;
        while (remaining > 0.0) {
            const h = if (remaining > max_h) max_h else remaining;
            const accel = -_.stiffness * (_.pos - _.target) - _.vel * _.damping;
            _.vel += accel * h;
            _.pos += _.vel * h;
            remaining -= h;
        }
    }

    /// Returns the current position value.
    fn value(): f64 {
        return _.pos;
    }

    /// Returns the current velocity.
    fn velocity(): f64 {
        return _.vel;
    }

    /// Returns the natural angular frequency `sqrt(stiffness)`.
    fn naturalFrequency(): f64 {
        return @sqrt(_.stiffness);
    }

    /// Returns the dimensionless damping ratio `damping / (2 * sqrt(stiffness))`.
    fn dampingRatio(): f64 {
        return _.damping / (2.0 * @sqrt(_.stiffness));
    }

    /// Returns `true` when both position error and velocity magnitude are within tolerance.
    fn isSettled(pos_eps: f64, vel_eps: f64): bool {
        return @abs(_.pos - _.target) <= pos_eps && @abs(_.vel) <= vel_eps;
    }
}

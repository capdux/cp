// @cap/d3-ease — Zero-allocation D3 v3 easing functions in pure CP
// Reference: npm:d3-ease (https://github.com/d3/d3-ease, ISC)

fn deasTpmt(x: f64): f64 {
    return (std.math.pow(f64, 2.0, -10.0 * x) - 0.0009765625) * 1.0009775171065494;
}

fn deasClampAmp(a: f64): f64 {
    if (a < 1.0) return 1.0;
    return a;
}

// --- Linear ---

pub fn easeLinear(t: f64): f64 {
    return t;
}

// --- Quad ---

pub fn easeQuadIn(t: f64): f64 {
    return t * t;
}

pub fn easeQuadOut(t: f64): f64 {
    return t * (2.0 - t);
}

pub fn easeQuadInOut(t: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return (t2 * t2) / 2.0;
    }
    const u = t2 - 1.0;
    return (u * (2.0 - u) + 1.0) / 2.0;
}

// --- Cubic ---

pub fn easeCubicIn(t: f64): f64 {
    return t * t * t;
}

pub fn easeCubicOut(t: f64): f64 {
    const u = t - 1.0;
    return u * u * u + 1.0;
}

pub fn easeCubicInOut(t: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return (t2 * t2 * t2) / 2.0;
    }
    const u = t2 - 2.0;
    return (u * u * u + 2.0) / 2.0;
}

// --- Poly ---

pub fn easePolyInExp(t: f64, e: f64): f64 {
    return std.math.pow(f64, t, e);
}

pub fn easePolyOutExp(t: f64, e: f64): f64 {
    return 1.0 - std.math.pow(f64, 1.0 - t, e);
}

pub fn easePolyInOutExp(t: f64, e: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return std.math.pow(f64, t2, e) / 2.0;
    }
    return (2.0 - std.math.pow(f64, 2.0 - t2, e)) / 2.0;
}

pub fn easePolyIn(t: f64): f64 {
    return easePolyInExp(t, 3.0);
}

pub fn easePolyOut(t: f64): f64 {
    return easePolyOutExp(t, 3.0);
}

pub fn easePolyInOut(t: f64): f64 {
    return easePolyInOutExp(t, 3.0);
}

// --- Sin ---

pub fn easeSinIn(t: f64): f64 {
    if (t == 1.0) return 1.0;
    return 1.0 - std.math.cos(t * (std.math.pi / 2.0));
}

pub fn easeSinOut(t: f64): f64 {
    return std.math.sin(t * (std.math.pi / 2.0));
}

pub fn easeSinInOut(t: f64): f64 {
    return (1.0 - std.math.cos(std.math.pi * t)) / 2.0;
}

// --- Exp ---

pub fn easeExpIn(t: f64): f64 {
    return deasTpmt(1.0 - t);
}

pub fn easeExpOut(t: f64): f64 {
    return 1.0 - deasTpmt(t);
}

pub fn easeExpInOut(t: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return deasTpmt(1.0 - t2) / 2.0;
    }
    return (2.0 - deasTpmt(t2 - 1.0)) / 2.0;
}

// --- Circle ---

pub fn easeCircleIn(t: f64): f64 {
    return 1.0 - std.math.sqrt(1.0 - t * t);
}

pub fn easeCircleOut(t: f64): f64 {
    const u = t - 1.0;
    return std.math.sqrt(1.0 - u * u);
}

pub fn easeCircleInOut(t: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return (1.0 - std.math.sqrt(1.0 - t2 * t2)) / 2.0;
    }
    const u = t2 - 2.0;
    return (std.math.sqrt(1.0 - u * u) + 1.0) / 2.0;
}

// --- Bounce ---

pub fn easeBounceOut(t: f64): f64 {
    const b1: f64 = 4.0 / 11.0;
    const b2: f64 = 6.0 / 11.0;
    const b3: f64 = 8.0 / 11.0;
    const b4: f64 = 3.0 / 4.0;
    const b5: f64 = 9.0 / 11.0;
    const b6: f64 = 10.0 / 11.0;
    const b7: f64 = 15.0 / 16.0;
    const b8: f64 = 21.0 / 22.0;
    const b9: f64 = 63.0 / 64.0;
    const b0: f64 = 1.0 / b1 / b1;

    if (t < b1) {
        return b0 * t * t;
    }
    if (t < b3) {
        const u = t - b2;
        return b0 * u * u + b4;
    }
    if (t < b6) {
        const u = t - b5;
        return b0 * u * u + b7;
    }
    const u = t - b8;
    return b0 * u * u + b9;
}

pub fn easeBounceIn(t: f64): f64 {
    return 1.0 - easeBounceOut(1.0 - t);
}

pub fn easeBounceInOut(t: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 <= 1.0) {
        return (1.0 - easeBounceOut(1.0 - t2)) / 2.0;
    }
    return (easeBounceOut(t2 - 1.0) + 1.0) / 2.0;
}

// --- Back ---

pub fn easeBackInOvershoot(t: f64, s: f64): f64 {
    return t * t * (s * (t - 1.0) + t);
}

pub fn easeBackOutOvershoot(t: f64, s: f64): f64 {
    const u = t - 1.0;
    return u * u * ((u + 1.0) * s + u) + 1.0;
}

pub fn easeBackInOutOvershoot(t: f64, s: f64): f64 {
    const t2 = 2.0 * t;
    if (t2 < 1.0) {
        return (t2 * t2 * ((s + 1.0) * t2 - s)) / 2.0;
    }
    const u = t2 - 2.0;
    return (u * u * ((s + 1.0) * u + s) + 2.0) / 2.0;
}

pub fn easeBackIn(t: f64): f64 {
    return easeBackInOvershoot(t, 1.70158);
}

pub fn easeBackOut(t: f64): f64 {
    return easeBackOutOvershoot(t, 1.70158);
}

pub fn easeBackInOut(t: f64): f64 {
    return easeBackInOutOvershoot(t, 1.70158);
}

// --- Elastic ---

pub fn easeElasticInParams(t: f64, a: f64, p: f64): f64 {
    const tau: f64 = 2.0 * std.math.pi;
    const amp = deasClampAmp(a);
    const per = p / tau;
    const s = std.math.asin(1.0 / amp) * per;
    const u = 1.0 - t;
    return amp * deasTpmt(u) * std.math.sin((s - u) / per);
}

pub fn easeElasticOutParams(t: f64, a: f64, p: f64): f64 {
    const tau: f64 = 2.0 * std.math.pi;
    const amp = deasClampAmp(a);
    const per = p / tau;
    const s = std.math.asin(1.0 / amp) * per;
    return 1.0 - amp * deasTpmt(t) * std.math.sin((t + s) / per);
}

pub fn easeElasticInOutParams(t: f64, a: f64, p: f64): f64 {
    const tau: f64 = 2.0 * std.math.pi;
    const amp = deasClampAmp(a);
    const per = p / tau;
    const s = std.math.asin(1.0 / amp) * per;
    const t2 = 2.0 * t - 1.0;
    if (t2 < 0.0) {
        return (amp * deasTpmt(0.0 - t2) * std.math.sin((s + t2) / per)) / 2.0;
    }
    return (2.0 - amp * deasTpmt(t2) * std.math.sin((s + t2) / per)) / 2.0;
}

pub fn easeElasticIn(t: f64): f64 {
    return easeElasticInParams(t, 1.0, 0.3);
}

pub fn easeElasticOut(t: f64): f64 {
    return easeElasticOutParams(t, 1.0, 0.3);
}

pub fn easeElasticInOut(t: f64): f64 {
    return easeElasticInOutParams(t, 1.0, 0.3);
}

// @cap/d3-scale — Zero-allocation D3 quantitative scales in pure CP
// Reference: npm:d3-scale (https://github.com/d3/d3-scale, ISC)

fn d3scClamp01(t: f64): f64 {
    if (t < 0.0) return 0.0;
    if (t > 1.0) return 1.0;
    return t;
}

fn d3scLerp(a: f64, b: f64, t: f64): f64 {
    return a + (b - a) * t;
}

fn d3scSignedPow(x: f64, exp: f64): f64 {
    if (x < 0.0) {
        return 0.0 - std.math.pow(f64, 0.0 - x, exp);
    }
    return std.math.pow(f64, x, exp);
}

fn d3scTickIncrement(start: f64, stop: f64, count: usize): f64 {
    const e10 = std.math.sqrt(50.0);
    const e5 = std.math.sqrt(10.0);
    const e2 = std.math.sqrt(2.0);
    const cnt_f: f64 = @floatFromInt(count);
    const step = (stop - start) / cnt_f;
    const power = std.math.floor(std.math.log10(step));
    const p10 = std.math.pow(f64, 10.0, power);
    const ratio = step / p10;
    let factor: f64 = 1.0;
    if (ratio >= e10) {
        factor = 10.0;
    } else if (ratio >= e5) {
        factor = 5.0;
    } else if (ratio >= e2) {
        factor = 2.0;
    }
    return factor * p10;
}

// --- LinearScale ---

pub struct LinearScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    clamped: bool,

    fn clamp(*_, enable: bool) {
        _.clamped = enable;
    }

    fn apply(x: f64): f64 {
        const span = _.d1 - _.d0;
        if (span == 0.0) return _.r0;
        let t = (x - _.d0) / span;
        if (_.clamped) t = d3scClamp01(t);
        return d3scLerp(_.r0, _.r1, t);
    }

    fn invert(y: f64): f64 {
        const span = _.r1 - _.r0;
        if (span == 0.0) return _.d0;
        let t = (y - _.r0) / span;
        if (_.clamped) t = d3scClamp01(t);
        return d3scLerp(_.d0, _.d1, t);
    }

    fn ticksCount(approx_count: usize): usize {
        if (approx_count == 0) return 0;
        let lo = _.d0;
        let hi = _.d1;
        if (lo == hi) return 1;
        if (lo > hi) {
            lo = _.d1;
            hi = _.d0;
        }
        const step = d3scTickIncrement(lo, hi, approx_count);
        if (step <= 0.0) return 0;
        const start_i = @as(i64, @intFromFloat(std.math.ceil(lo / step)));
        const stop_i = @as(i64, @intFromFloat(std.math.floor(hi / step)));
        if (stop_i < start_i) return 0;
        return (stop_i - start_i + 1) as usize;
    }

    fn ticks(approx_count: usize, out: []f64): usize {
        if (approx_count == 0 || out.len == 0) return 0;
        let lo = _.d0;
        let hi = _.d1;
        if (lo == hi) {
            out[0] = lo;
            return 1;
        }
        const reversed = lo > hi;
        if (reversed) {
            lo = _.d1;
            hi = _.d0;
        }
        const step = d3scTickIncrement(lo, hi, approx_count);
        if (step <= 0.0) return 0;
        const start_i = @as(i64, @intFromFloat(std.math.ceil(lo / step)));
        const stop_i = @as(i64, @intFromFloat(std.math.floor(hi / step)));
        if (stop_i < start_i) return 0;
        const total = (stop_i - start_i + 1) as usize;
        let n = total;
        if (n > out.len) n = out.len;
        let i: usize = 0;
        while (i < n) {
            const idx_i: i64 = if (reversed) stop_i - (i as i64) else start_i + (i as i64);
            const idx_f: f64 = @floatFromInt(idx_i);
            out[i] = idx_f * step;
            i += 1;
        }
        return n;
    }
}

pub fn linearScale(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale {
    return LinearScale{ d0: d0, d1: d1, r0: r0, r1: r1, clamped: false };
}

// --- PowScale ---

pub struct PowScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    exponent: f64,
    clamped: bool,

    fn clamp(*_, enable: bool) {
        _.clamped = enable;
    }

    fn apply(x: f64): f64 {
        const p0 = d3scSignedPow(_.d0, _.exponent);
        const p1 = d3scSignedPow(_.d1, _.exponent);
        const span = p1 - p0;
        if (span == 0.0) return _.r0;
        let t = (d3scSignedPow(x, _.exponent) - p0) / span;
        if (_.clamped) t = d3scClamp01(t);
        return d3scLerp(_.r0, _.r1, t);
    }

    fn invert(y: f64): f64 {
        const span = _.r1 - _.r0;
        if (span == 0.0 || _.exponent == 0.0) return _.d0;
        let t = (y - _.r0) / span;
        if (_.clamped) t = d3scClamp01(t);
        const p0 = d3scSignedPow(_.d0, _.exponent);
        const p1 = d3scSignedPow(_.d1, _.exponent);
        const px = d3scLerp(p0, p1, t);
        return d3scSignedPow(px, 1.0 / _.exponent);
    }
}

pub fn powScale(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale {
    return PowScale{ d0: d0, d1: d1, r0: r0, r1: r1, exponent: exponent, clamped: false };
}

// --- SqrtScale ---

pub struct SqrtScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    clamped: bool,

    fn clamp(*_, enable: bool) {
        _.clamped = enable;
    }

    fn apply(x: f64): f64 {
        const p0 = d3scSignedPow(_.d0, 0.5);
        const p1 = d3scSignedPow(_.d1, 0.5);
        const span = p1 - p0;
        if (span == 0.0) return _.r0;
        let t = (d3scSignedPow(x, 0.5) - p0) / span;
        if (_.clamped) t = d3scClamp01(t);
        return d3scLerp(_.r0, _.r1, t);
    }

    fn invert(y: f64): f64 {
        const span = _.r1 - _.r0;
        if (span == 0.0) return _.d0;
        let t = (y - _.r0) / span;
        if (_.clamped) t = d3scClamp01(t);
        const p0 = d3scSignedPow(_.d0, 0.5);
        const p1 = d3scSignedPow(_.d1, 0.5);
        const px = d3scLerp(p0, p1, t);
        return d3scSignedPow(px, 2.0);
    }
}

pub fn sqrtScale(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale {
    return SqrtScale{ d0: d0, d1: d1, r0: r0, r1: r1, clamped: false };
}

// --- LogScale ---

pub struct LogScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    base: f64,
    clamped: bool,

    fn clamp(*_, enable: bool) {
        _.clamped = enable;
    }

    fn apply(x: f64): f64 {
        const inv_ln_b = 1.0 / @log(_.base);
        const neg = _.d0 < 0.0;
        let l0: f64 = 0.0;
        let l1: f64 = 0.0;
        let lx: f64 = 0.0;
        if (neg) {
            l0 = 0.0 - @log(0.0 - _.d0) * inv_ln_b;
            l1 = 0.0 - @log(0.0 - _.d1) * inv_ln_b;
            lx = 0.0 - @log(0.0 - x) * inv_ln_b;
        } else {
            l0 = @log(_.d0) * inv_ln_b;
            l1 = @log(_.d1) * inv_ln_b;
            lx = @log(x) * inv_ln_b;
        }
        const span = l1 - l0;
        if (span == 0.0) return _.r0;
        let t = (lx - l0) / span;
        if (_.clamped) t = d3scClamp01(t);
        return d3scLerp(_.r0, _.r1, t);
    }

    fn invert(y: f64): f64 {
        const span = _.r1 - _.r0;
        if (span == 0.0) return _.d0;
        let t = (y - _.r0) / span;
        if (_.clamped) t = d3scClamp01(t);
        const inv_ln_b = 1.0 / @log(_.base);
        const neg = _.d0 < 0.0;
        if (neg) {
            const l0 = 0.0 - @log(0.0 - _.d0) * inv_ln_b;
            const l1 = 0.0 - @log(0.0 - _.d1) * inv_ln_b;
            const lx = d3scLerp(l0, l1, t);
            return 0.0 - std.math.pow(f64, _.base, 0.0 - lx);
        }
        const l0 = @log(_.d0) * inv_ln_b;
        const l1 = @log(_.d1) * inv_ln_b;
        const lx = d3scLerp(l0, l1, t);
        return std.math.pow(f64, _.base, lx);
    }
}

pub fn logScale(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale {
    let b = base;
    if (b <= 0.0 || b == 1.0) b = 10.0;
    return LogScale{ d0: d0, d1: d1, r0: r0, r1: r1, base: b, clamped: false };
}

// --- QuantizeScale ---

pub struct QuantizeScale {
    d0: f64,
    d1: f64,
    buckets: usize,

    fn applyIndex(x: f64): usize {
        if (_.buckets <= 1) return 0;
        const span = _.d1 - _.d0;
        if (span == 0.0) return 0;
        const t = (x - _.d0) / span;
        if (t <= 0.0) return 0;
        if (t >= 1.0) return _.buckets - 1;
        const bf: f64 = @floatFromInt(_.buckets);
        const idx = @as(usize, @intFromFloat(std.math.floor(t * bf)));
        if (idx >= _.buckets) return _.buckets - 1;
        return idx;
    }

    fn applyValue(x: f64, values: []const f64): f64 {
        if (values.len == 0) return 0.0;
        const idx = _.applyIndex(x);
        if (idx >= values.len) return values[values.len - 1];
        return values[idx];
    }

    fn invertExtentMin(idx: usize): f64 {
        if (_.buckets == 0) return _.d0;
        const bf: f64 = @floatFromInt(_.buckets);
        const if_: f64 = @floatFromInt(idx);
        return d3scLerp(_.d0, _.d1, if_ / bf);
    }

    fn invertExtentMax(idx: usize): f64 {
        if (_.buckets == 0) return _.d1;
        const bf: f64 = @floatFromInt(_.buckets);
        const if_: f64 = @floatFromInt(idx + 1);
        return d3scLerp(_.d0, _.d1, if_ / bf);
    }
}

pub fn quantizeScale(d0: f64, d1: f64, buckets: usize): QuantizeScale {
    return QuantizeScale{ d0: d0, d1: d1, buckets: buckets };
}

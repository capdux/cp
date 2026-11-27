// Type definitions for CP

export fn d3scClamp01(t: f64): f64;
export fn d3scLerp(a: f64, b: f64, t: f64): f64;
export fn d3scSignedPow(x: f64, exp: f64): f64;
export fn d3scTickIncrement(start: f64, stop: f64, count: usize): f64;
export class LinearScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    clamped: bool,
    fn clamp(_: auto, enable: bool);
    fn apply(x: f64): f64;
    fn invert(y: f64): f64;
    fn ticksCount(approx_count: usize): usize;
    fn ticks(approx_count: usize, out: []f64): usize;
}

export fn linearScale(d0: f64, d1: f64, r0: f64, r1: f64): LinearScale;
export class PowScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    exponent: f64,
    clamped: bool,
    fn clamp(_: auto, enable: bool);
    fn apply(x: f64): f64;
    fn invert(y: f64): f64;
}

export fn powScale(d0: f64, d1: f64, r0: f64, r1: f64, exponent: f64): PowScale;
export class SqrtScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    clamped: bool,
    fn clamp(_: auto, enable: bool);
    fn apply(x: f64): f64;
    fn invert(y: f64): f64;
}

export fn sqrtScale(d0: f64, d1: f64, r0: f64, r1: f64): SqrtScale;
export class LogScale {
    d0: f64,
    d1: f64,
    r0: f64,
    r1: f64,
    base: f64,
    clamped: bool,
    fn clamp(_: auto, enable: bool);
    fn apply(x: f64): f64;
    fn invert(y: f64): f64;
}

export fn logScale(d0: f64, d1: f64, r0: f64, r1: f64, base: f64): LogScale;
export class QuantizeScale {
    d0: f64,
    d1: f64,
    buckets: usize,
    fn applyIndex(x: f64): usize;
    fn applyValue(x: f64, values: &[]f64): f64;
    fn invertExtentMin(idx: usize): f64;
    fn invertExtentMax(idx: usize): f64;
}

export fn quantizeScale(d0: f64, d1: f64, buckets: usize): QuantizeScale;

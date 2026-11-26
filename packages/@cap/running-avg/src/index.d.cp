// Type definitions for CP

export class Ema {
    alpha: f64,
    value: f64,
    initialized: bool,
    fn new(alpha: f64): Ema;
    fn fromPeriod(period: u32): Ema;
    fn update(_: auto, sample: f64): f64;
    fn reset(_: auto);
}

export class Sma {
    head: usize,
    count: usize,
    sum: f64,
    fn new(): Sma;
    fn update(_: auto, window: []f64, sample: f64): f64;
    fn average(): f64;
    fn reset(_: auto);
}

export fn smaSlice(xs: []f64): f64;
export fn wmaSlice(xs: []f64): f64;

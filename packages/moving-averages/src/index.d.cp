// Type definitions for CP

export fn maNan(): f64;
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

export class Cma {
    count: usize,
    value: f64,
    fn new(): Cma;
    fn update(_: auto, sample: f64): f64;
    fn average(): f64;
    fn reset(_: auto);
}

export class Wma {
    head: usize,
    count: usize,
    fn new(): Wma;
    fn update(_: auto, window: []f64, sample: f64): f64;
    fn reset(_: auto);
}

export fn smaSlice(xs: []f64): f64;
export fn wmaSlice(xs: []f64): f64;
export fn ma(data: &[]f64, size: usize, out: []f64): usize;
export fn sma(data: &[]f64, size: usize, out: []f64): usize;
export fn dma(data: &[]f64, alpha: f64, out: []f64): usize;
export fn ema(data: &[]f64, size: usize, out: []f64): usize;
export fn smma(data: &[]f64, size: usize, out: []f64): usize;
export fn wma(data: &[]f64, size: usize, out: []f64): usize;
export fn cma(data: &[]f64, out: []f64): usize;

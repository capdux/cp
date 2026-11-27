// @cap/running-avg — Zero-heap Moving Averages (MA/SMA, EMA, WMA, DMA, CMA, SMMA)
// Clean-room MIT implementation in pure CP (modeled on npm `moving-averages`).

fn maNan(): f64 {
    let z: f64 = 0.0;
    return z / z;
}

pub struct Ema {
    pub alpha: f64,
    pub value: f64,
    pub initialized: bool,

    pub static fn new(alpha: f64): Ema {
        let a = alpha;
        if (a < 0.0) { a = 0.0; }
        if (a > 1.0) { a = 1.0; }
        return Ema { alpha: a, value: 0.0, initialized: false };
    }

    pub static fn fromPeriod(period: u32): Ema {
        if (period <= 1) {
            return Ema.new(1.0);
        }
        let a = 2.0 / ((period as f64) + 1.0);
        return Ema.new(a);
    }

    pub fn update(*_, sample: f64): f64 {
        if (!_.initialized) {
            _.value = sample;
            _.initialized = true;
            return _.value;
        }
        _.value = _.alpha * sample + (1.0 - _.alpha) * _.value;
        return _.value;
    }

    pub fn reset(*_) {
        _.value = 0.0;
        _.initialized = false;
    }
}

pub struct Sma {
    pub head: usize,
    pub count: usize,
    pub sum: f64,

    pub static fn new(): Sma {
        return Sma { head: 0, count: 0, sum: 0.0 };
    }

    pub fn update(*_, window: []f64, sample: f64): f64 {
        let cap = window.len;
        if (cap == 0) { return 0.0; }
        if (_.count < cap) {
            window[_.head] = sample;
            _.sum = _.sum + sample;
            _.count = _.count + 1;
            _.head = (_.head + 1) % cap;
        } else {
            let old = window[_.head];
            window[_.head] = sample;
            _.sum = _.sum - old + sample;
            _.head = (_.head + 1) % cap;
        }
        return _.sum / (_.count as f64);
    }

    pub fn average(): f64 {
        if (_.count == 0) { return 0.0; }
        return _.sum / (_.count as f64);
    }

    pub fn reset(*_) {
        _.head = 0;
        _.count = 0;
        _.sum = 0.0;
    }
}

/// Streaming Cumulative Moving Average (CMA).
pub struct Cma {
    pub count: usize,
    pub value: f64,

    pub static fn new(): Cma {
        return Cma { count: 0, value: 0.0 };
    }

    pub fn update(*_, sample: f64): f64 {
        _.count = _.count + 1;
        _.value = _.value + (sample - _.value) / (_.count as f64);
        return _.value;
    }

    pub fn average(): f64 {
        return _.value;
    }

    pub fn reset(*_) {
        _.count = 0;
        _.value = 0.0;
    }
}

/// Streaming Linearly-Weighted Moving Average (WMA) over a caller-provided ring buffer.
pub struct Wma {
    pub head: usize,
    pub count: usize,

    pub static fn new(): Wma {
        return Wma { head: 0, count: 0 };
    }

    pub fn update(*_, window: []f64, sample: f64): f64 {
        let cap = window.len;
        if (cap == 0) { return 0.0; }
        window[_.head] = sample;
        _.head = (_.head + 1) % cap;
        if (_.count < cap) {
            _.count = _.count + 1;
        }
        let n = _.count;
        let start = if (_.count < cap) { 0 as usize } else { _.head };
        let weighted_sum: f64 = 0.0;
        let weight_total: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            let idx = (start + i) % cap;
            let w = (i + 1) as f64;
            weighted_sum = weighted_sum + window[idx] * w;
            weight_total = weight_total + w;
            i = i + 1;
        }
        return weighted_sum / weight_total;
    }

    pub fn reset(*_) {
        _.head = 0;
        _.count = 0;
    }
}

pub fn smaSlice(xs: []f64): f64 {
    if (xs.len == 0) { return 0.0; }
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        s = s + xs[i];
        i = i + 1;
    }
    return s / (xs.len as f64);
}

pub fn wmaSlice(xs: []f64): f64 {
    let n = xs.len;
    if (n == 0) { return 0.0; }
    let weighted_sum: f64 = 0.0;
    let weight_total: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        let w = (i + 1) as f64;
        weighted_sum = weighted_sum + xs[i] * w;
        weight_total = weight_total + w;
        i = i + 1;
    }
    return weighted_sum / weight_total;
}

/// Simple Moving Average series (`npm: moving-averages` `ma(data, size)`).
/// Writes up to `min(data.len, out.len)` values into `out`.
/// Indices `< size - 1` are filled with `NaN`; returns number of elements written.
pub fn ma(data: []const f64, size: usize, out: []f64): usize {
    let n = if (data.len < out.len) { data.len } else { out.len };
    if (n == 0 || size == 0) { return 0; }
    let nan_val = maNan();
    let sum: f64 = 0.0;
    let inv: f64 = 1.0 / (size as f64);
    let i: usize = 0;
    while (i < n) {
        sum = sum + data[i];
        if (i + 1 < size) {
            out[i] = nan_val;
        } else {
            if (i >= size) {
                sum = sum - data[i - size];
            }
            out[i] = sum * inv;
        }
        i = i + 1;
    }
    return n;
}

/// Alias for `ma`: Simple Moving Average series (`sma(data, size, out)`).
pub fn sma(data: []const f64, size: usize, out: []f64): usize {
    return ma(data, size, out);
}

/// Dynamic Moving Average series (`npm: moving-averages` `dma(data, alpha)`):
/// `out[0] = data[0]`, `out[i] = alpha * data[i] + (1 - alpha) * out[i - 1]`.
pub fn dma(data: []const f64, alpha: f64, out: []f64): usize {
    let n = if (data.len < out.len) { data.len } else { out.len };
    if (n == 0) { return 0; }
    let a = alpha;
    if (a < 0.0) { a = 0.0; }
    if (a > 1.0) { a = 1.0; }
    let prev: f64 = data[0];
    out[0] = prev;
    let i: usize = 1;
    while (i < n) {
        prev = a * data[i] + (1.0 - a) * prev;
        out[i] = prev;
        i = i + 1;
    }
    return n;
}

/// Exponential Moving Average series (`npm: moving-averages` `ema(data, size)`),
/// using smoothing factor `alpha = 2 / (size + 1)`.
pub fn ema(data: []const f64, size: usize, out: []f64): usize {
    if (size == 0) { return 0; }
    let alpha: f64 = 2.0 / ((size as f64) + 1.0);
    return dma(data, alpha, out);
}

/// Smoothed Moving Average series (`npm: moving-averages` `smma(data, size)`),
/// using Wilder's smoothing factor `alpha = 1 / size`.
pub fn smma(data: []const f64, size: usize, out: []f64): usize {
    if (size == 0) { return 0; }
    let alpha: f64 = 1.0 / (size as f64);
    return dma(data, alpha, out);
}

/// Weighted Moving Average series (`npm: moving-averages` `wma(data, size)`).
/// Indices `< size - 1` are filled with `NaN`; returns number of elements written.
pub fn wma(data: []const f64, size: usize, out: []f64): usize {
    let n = if (data.len < out.len) { data.len } else { out.len };
    if (n == 0 || size == 0) { return 0; }
    let nan_val = maNan();
    let denom: f64 = ((size * (size + 1)) as f64) / 2.0;
    let i: usize = 0;
    while (i < n) {
        if (i + 1 < size) {
            out[i] = nan_val;
        } else {
            let weighted_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < size) {
                let w = (k + 1) as f64;
                weighted_sum = weighted_sum + data[i + 1 - size + k] * w;
                k = k + 1;
            }
            out[i] = weighted_sum / denom;
        }
        i = i + 1;
    }
    return n;
}

/// Cumulative Moving Average series (`npm: moving-averages` `cma(data)`):
/// `out[i] = (data[0] + ... + data[i]) / (i + 1)`.
pub fn cma(data: []const f64, out: []f64): usize {
    let n = if (data.len < out.len) { data.len } else { out.len };
    if (n == 0) { return 0; }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum = sum + data[i];
        out[i] = sum / ((i + 1) as f64);
        i = i + 1;
    }
    return n;
}

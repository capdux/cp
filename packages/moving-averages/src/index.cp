// @cap/running-avg — Zero-heap EMA, SMA & WMA streaming moving averages
// Clean-room MIT implementation in pure CP.

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

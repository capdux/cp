// @cap/stats-basic — Zero-heap Welford online statistics & descriptive statistics
// MIT License (clean-room CP implementation, zero external dependencies)

fn absF64(x: f64) -> f64 {
    if (x < 0.0) {
        return -x;
    }
    return x;
}

fn sqrtF64(x: f64) -> f64 {
    if (x <= 0.0) {
        return 0.0;
    }
    let g: f64 = x;
    if (g < 1.0) {
        g = 1.0;
    }
    let i: usize = 0;
    while (i < 48) {
        const nxt: f64 = 0.5 * (g + x / g);
        if (absF64(nxt - g) <= 0.0000000000000005 * g) {
            return nxt;
        }
        g = nxt;
        i += 1;
    }
    return g;
}

pub struct OnlineStats {
    pub count: u64,
    pub mean: f64,
    pub m2: f64,
    pub min_val: f64,
    pub max_val: f64,

    pub static fn new() -> OnlineStats {
        return OnlineStats {
            count: 0,
            mean: 0.0,
            m2: 0.0,
            min_val: 0.0,
            max_val: 0.0,
        };
    }

    pub fn push(*_, x: f64) {
        if (_.count == 0) {
            _.min_val = x;
            _.max_val = x;
        } else {
            if (x < _.min_val) {
                _.min_val = x;
            }
            if (x > _.max_val) {
                _.max_val = x;
            }
        }
        _.count += 1;
        const delta: f64 = x - _.mean;
        _.mean = _.mean + delta / (_.count as f64);
        const delta2: f64 = x - _.mean;
        _.m2 = _.m2 + delta * delta2;
    }

    pub fn merge(*_, other: OnlineStats) {
        if (other.count == 0) {
            return;
        }
        if (_.count == 0) {
            _.count = other.count;
            _.mean = other.mean;
            _.m2 = other.m2;
            _.min_val = other.min_val;
            _.max_val = other.max_val;
            return;
        }

        const n_a: f64 = _.count as f64;
        const n_b: f64 = other.count as f64;
        const n_ab: f64 = n_a + n_b;
        const delta: f64 = other.mean - _.mean;

        _.mean = _.mean + delta * (n_b / n_ab);
        _.m2 = _.m2 + other.m2 + delta * delta * (n_a * n_b / n_ab);
        _.count += other.count;

        if (other.min_val < _.min_val) {
            _.min_val = other.min_val;
        }
        if (other.max_val > _.max_val) {
            _.max_val = other.max_val;
        }
    }

    pub fn variance() -> f64 {
        if (_.count == 0) {
            return 0.0;
        }
        return _.m2 / (_.count as f64);
    }

    pub fn sampleVariance() -> f64 {
        if (_.count < 2) {
            return 0.0;
        }
        return _.m2 / ((_.count - 1) as f64);
    }

    pub fn stddev() -> f64 {
        return sqrtF64(_.variance());
    }

    pub fn sampleStddev() -> f64 {
        return sqrtF64(_.sampleVariance());
    }
}

pub fn sum(xs: []const f64) -> f64 {
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        total = total + xs[i];
        i += 1;
    }
    return total;
}

pub fn mean(xs: []const f64) -> f64 {
    if (xs.len == 0) {
        return 0.0;
    }
    return sum(xs) / (xs.len as f64);
}

pub fn variance(xs: []const f64) -> f64 {
    if (xs.len == 0) {
        return 0.0;
    }
    let st: OnlineStats = OnlineStats.new();
    let i: usize = 0;
    while (i < xs.len) {
        st.push(xs[i]);
        i += 1;
    }
    return st.variance();
}

pub fn stddev(xs: []const f64) -> f64 {
    return sqrtF64(variance(xs));
}

pub fn minSlice(xs: []const f64) -> f64 {
    if (xs.len == 0) {
        return 0.0;
    }
    let m: f64 = xs[0];
    let i: usize = 1;
    while (i < xs.len) {
        if (xs[i] < m) {
            m = xs[i];
        }
        i += 1;
    }
    return m;
}

pub fn maxSlice(xs: []const f64) -> f64 {
    if (xs.len == 0) {
        return 0.0;
    }
    let m: f64 = xs[0];
    let i: usize = 1;
    while (i < xs.len) {
        if (xs[i] > m) {
            m = xs[i];
        }
        i += 1;
    }
    return m;
}

pub fn covariance(xs: []const f64, ys: []const f64) -> f64 {
    if (xs.len == 0 || xs.len != ys.len) {
        return 0.0;
    }
    const mx: f64 = mean(xs);
    const my: f64 = mean(ys);
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        acc = acc + (xs[i] - mx) * (ys[i] - my);
        i += 1;
    }
    return acc / (xs.len as f64);
}

pub fn pearsonR(xs: []const f64, ys: []const f64) -> f64 {
    if (xs.len == 0 || xs.len != ys.len) {
        return 0.0;
    }
    const sx: f64 = stddev(xs);
    const sy: f64 = stddev(ys);
    const denom: f64 = sx * sy;
    if (denom <= 0.0) {
        return 0.0;
    }
    return covariance(xs, ys) / denom;
}

pub fn percentileSorted(sorted: []const f64, p: f64) -> f64 {
    if (sorted.len == 0) {
        return 0.0;
    }
    if (sorted.len == 1) {
        return sorted[0];
    }
    let pct: f64 = p;
    if (pct <= 0.0) {
        return sorted[0];
    }
    if (pct >= 100.0) {
        return sorted[sorted.len - 1];
    }

    const pos: f64 = (pct / 100.0) * ((sorted.len - 1) as f64);
    const lo_idx: usize = pos as usize;
    const hi_idx: usize = lo_idx + 1;
    if (hi_idx >= sorted.len) {
        return sorted[sorted.len - 1];
    }
    const frac: f64 = pos - (lo_idx as f64);
    return sorted[lo_idx] + (sorted[hi_idx] - sorted[lo_idx]) * frac;
}

pub fn medianSorted(sorted: []const f64) -> f64 {
    return percentileSorted(sorted, 50.0);
}

// @cap/sketches-ddsketch — Zero-heap DDSketch relative-accuracy quantile sketch in pure CP.

pub const DDSK_BUCKETS: usize = 512;

fn ddskZeroBuckets(): [DDSK_BUCKETS]u64 {
    let arr: [DDSK_BUCKETS]u64 = undefined;
    let i: usize = 0;
    while (i < DDSK_BUCKETS) : (i += 1) {
        arr[i] = 0;
    }
    return arr;
}

fn ddskClampAlpha(alpha: f64): f64 {
    if (alpha <= 0.0 || alpha >= 1.0) {
        return 0.01;
    }
    if (alpha < 1.0e-4) {
        return 1.0e-4;
    }
    if (alpha > 0.99) {
        return 0.99;
    }
    return alpha;
}

fn ddskAddKeyCount(sk: *DDSketch, key: i32, cnt: u64) {
    if (!sk.has_anchor) {
        sk.min_index = key;
        sk.has_anchor = true;
        sk.buckets[0] += cnt;
        return;
    }

    if (key < sk.min_index) {
        const diff: usize = (sk.min_index - key) as usize;
        if (diff >= DDSK_BUCKETS) {
            let total_old: u64 = 0;
            let i: usize = 0;
            while (i < DDSK_BUCKETS) : (i += 1) {
                total_old += sk.buckets[i];
                sk.buckets[i] = 0;
            }
            sk.buckets[DDSK_BUCKETS - 1] = total_old;
            sk.buckets[0] += cnt;
            sk.min_index = key;
            return;
        }
        let overflow: u64 = 0;
        let i: usize = DDSK_BUCKETS - diff;
        while (i < DDSK_BUCKETS) : (i += 1) {
            overflow += sk.buckets[i];
        }
        let j: usize = DDSK_BUCKETS - 1;
        while (j >= diff) : (j -= 1) {
            sk.buckets[j] = sk.buckets[j - diff];
            if (j == diff) {
                break;
            }
        }
        let z: usize = 0;
        while (z < diff) : (z += 1) {
            sk.buckets[z] = 0;
        }
        sk.buckets[DDSK_BUCKETS - 1] += overflow;
        sk.min_index = key;
        sk.buckets[0] += cnt;
        return;
    }

    const offset: usize = (key - sk.min_index) as usize;
    if (offset < DDSK_BUCKETS) {
        sk.buckets[offset] += cnt;
        return;
    }

    // CollapsingLowestDenseStore: collapse lowest buckets into bucket 0
    const shift: usize = offset - (DDSK_BUCKETS - 1);
    if (shift >= DDSK_BUCKETS) {
        let total_old: u64 = 0;
        let i: usize = 0;
        while (i < DDSK_BUCKETS) : (i += 1) {
            total_old += sk.buckets[i];
            sk.buckets[i] = 0;
        }
        sk.buckets[0] = total_old;
        sk.buckets[DDSK_BUCKETS - 1] += cnt;
        sk.min_index = key - ((DDSK_BUCKETS - 1) as i32);
        return;
    }

    let collapsed: u64 = 0;
    let i: usize = 0;
    while (i <= shift) : (i += 1) {
        collapsed += sk.buckets[i];
    }
    sk.buckets[0] = collapsed;
    let k: usize = 1;
    while (k + shift < DDSK_BUCKETS) : (k += 1) {
        sk.buckets[k] = sk.buckets[k + shift];
    }
    let clearIdx: usize = DDSK_BUCKETS - shift;
    while (clearIdx < DDSK_BUCKETS) : (clearIdx += 1) {
        sk.buckets[clearIdx] = 0;
    }
    sk.min_index += shift as i32;
    sk.buckets[DDSK_BUCKETS - 1] += cnt;
}

pub struct DDSketch {
    buckets: [DDSK_BUCKETS]u64,
    alpha: f64,
    gamma: f64,
    log_gamma: f64,
    min_index: i32,
    has_anchor: bool,
    zero_count: u64,
    total_count: u64,
    min_val: f64,
    max_val: f64,
    sum_val: f64,

    static fn new(relative_accuracy: f64): DDSketch {
        const a = ddskClampAlpha(relative_accuracy);
        const g = (1.0 + a) / (1.0 - a);
        const lg = @log(g);
        return DDSketch{
            buckets: ddskZeroBuckets(),
            alpha: a,
            gamma: g,
            log_gamma: lg,
            min_index: 0,
            has_anchor: false,
            zero_count: 0,
            total_count: 0,
            min_val: 0.0,
            max_val: 0.0,
            sum_val: 0.0,
        };
    }

    fn add(*_, v: f64) {
        if (_.total_count == 0) {
            _.min_val = v;
            _.max_val = v;
        } else {
            if (v < _.min_val) {
                _.min_val = v;
            }
            if (v > _.max_val) {
                _.max_val = v;
            }
        }
        _.sum_val += v;
        _.total_count += 1;

        if (v <= 1.0e-12) {
            _.zero_count += 1;
            return;
        }
        const key = @as(i32, @intFromFloat(@floor(@log(v) / _.log_gamma)));
        ddskAddKeyCount(_, key, 1);
    }

    fn count(): u64 {
        return _.total_count;
    }

    fn sum(): f64 {
        return _.sum_val;
    }

    fn min(): f64 {
        return _.min_val;
    }

    fn max(): f64 {
        return _.max_val;
    }

    fn quantile(q: f64): f64 {
        if (_.total_count == 0) {
            return 0.0;
        }
        if (q <= 0.0) {
            return _.min_val;
        }
        if (q >= 1.0) {
            return _.max_val;
        }

        const target = q * @as(f64, @floatFromInt(_.total_count));
        let acc: u64 = _.zero_count;
        if (@as(f64, @floatFromInt(acc)) >= target) {
            return 0.0;
        }

        let i: usize = 0;
        while (i < DDSK_BUCKETS) : (i += 1) {
            acc += _.buckets[i];
            if (@as(f64, @floatFromInt(acc)) >= target) {
                const key: i32 = _.min_index + (i as i32);
                const upper = @exp(@as(f64, @floatFromInt(key + 1)) * _.log_gamma);
                let est = (2.0 * upper) / (1.0 + _.gamma);
                if (est < _.min_val) {
                    est = _.min_val;
                }
                if (est > _.max_val) {
                    est = _.max_val;
                }
                return est;
            }
        }
        return _.max_val;
    }

    fn merge(*_, other: DDSketch) {
        if (other.total_count == 0) {
            return;
        }
        if (_.total_count == 0) {
            _.min_val = other.min_val;
            _.max_val = other.max_val;
        } else {
            if (other.min_val < _.min_val) {
                _.min_val = other.min_val;
            }
            if (other.max_val > _.max_val) {
                _.max_val = other.max_val;
            }
        }
        _.sum_val += other.sum_val;
        _.total_count += other.total_count;
        _.zero_count += other.zero_count;

        if (!other.has_anchor) {
            return;
        }
        let i: usize = 0;
        while (i < DDSK_BUCKETS) : (i += 1) {
            const c = other.buckets[i];
            if (c > 0) {
                const key: i32 = other.min_index + (i as i32);
                ddskAddKeyCount(_, key, c);
            }
        }
    }
}

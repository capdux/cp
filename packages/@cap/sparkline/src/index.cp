// @cap/sparkline — Zero-heap Unicode sparkline & horizontal bar chart renderer
// Clean-room MIT implementation in pure CP (replaces npm `sparkly` / `asciichart` / Rust `sparklines`).

pub struct SeriesStats {
    pub count: usize,
    pub minVal: f64,
    pub maxVal: f64,
    pub sum: f64,
    pub mean: f64,
    pub hasValues: bool,
}

pub struct SparkOptions {
    pub useCustomBounds: bool,
    pub minBound: f64,
    pub maxBound: f64,
    pub asciiOnly: bool,
}

pub struct SparkResult {
    pub buf: [512]u8,
    pub len: usize,
    pub truncated: bool,

    pub fn asSlice() -> string {
        return _.buf[0.._.len];
    }
}

pub fn defaultOptions() -> SparkOptions {
    return SparkOptions {
        useCustomBounds: false,
        minBound: 0.0,
        maxBound: 0.0,
        asciiOnly: false,
    };
}

pub fn computeStats(values: []const f64) -> SeriesStats {
    if (values.len == 0) {
        return SeriesStats {
            count: 0,
            minVal: 0.0,
            maxVal: 0.0,
            sum: 0.0,
            mean: 0.0,
            hasValues: false,
        };
    }

    let minV: f64 = values[0];
    let maxV: f64 = values[0];
    let total: f64 = 0.0;
    let i: usize = 0;
    while (i < values.len) {
        const v: f64 = values[i];
        if (v < minV) { minV = v; }
        if (v > maxV) { maxV = v; }
        total = total + v;
        i += 1;
    }

    return SeriesStats {
        count: values.len,
        minVal: minV,
        maxVal: maxV,
        sum: total,
        mean: total / (values.len as f64),
        hasValues: true,
    };
}

// Write a single Unicode block element U+2581..U+2588 (3 UTF-8 bytes: E2 96 81..88)
// Level is clamped to 0..7.
fn appendBlock(res: *SparkResult, level: u8, asciiOnly: bool) {
    let lv: u8 = level;
    if (lv > 7) { lv = 7; }

    if (asciiOnly) {
        if (res.len + 1 > 512) {
            res.truncated = true;
            return;
        }
        const chars: string = "._-:=+*#";
        res.buf[res.len] = chars[lv as usize];
        res.len += 1;
        return;
    }

    if (res.len + 3 > 512) {
        res.truncated = true;
        return;
    }
    res.buf[res.len] = 0xE2;
    res.buf[res.len + 1] = 0x96;
    res.buf[res.len + 2] = (0x81 as u8) + lv;
    res.len += 3;
}

fn appendByte(res: *SparkResult, b: u8) {
    if (res.len + 1 > 512) {
        res.truncated = true;
        return;
    }
    res.buf[res.len] = b;
    res.len += 1;
}

fn appendBytes(res: *SparkResult, s: string) {
    let i: usize = 0;
    while (i < s.len) {
        if (res.len >= 512) {
            res.truncated = true;
            return;
        }
        res.buf[res.len] = s[i];
        res.len += 1;
        i += 1;
    }
}

/// Map a value in [lo, hi] to a discrete sparkline level in 0..7.
pub fn quantizeLevel(value: f64, lo: f64, hi: f64) -> u8 {
    if (hi <= lo) {
        // Flat series renders at mid-height (level 3 = ▄)
        return 3;
    }
    let clamped: f64 = value;
    if (clamped < lo) { clamped = lo; }
    if (clamped > hi) { clamped = hi; }

    const norm: f64 = (clamped - lo) / (hi - lo);
    const scaled: f64 = norm * 7.0 + 0.5;
    if (scaled <= 0.0) { return 0; }
    if (scaled >= 7.0) { return 7; }
    return scaled as u8;
}

/// Render a Unicode sparkline (▁▂▃▄▅▆▇█) with custom options.
pub fn renderWithOptions(values: []const f64, opts: SparkOptions) -> SparkResult {
    let buf: [512]u8 = undefined;
    let res: SparkResult = SparkResult {
        buf: buf,
        len: 0,
        truncated: false,
    };

    if (values.len == 0) {
        return res;
    }

    let lo: f64 = opts.minBound;
    let hi: f64 = opts.maxBound;
    if (!opts.useCustomBounds) {
        const st: SeriesStats = computeStats(values);
        lo = st.minVal;
        hi = st.maxVal;
    }

    let i: usize = 0;
    while (i < values.len) {
        const lv: u8 = quantizeLevel(values[i], lo, hi);
        appendBlock(&res, lv, opts.asciiOnly);
        if (res.truncated) { break; }
        i += 1;
    }

    return res;
}

/// Render a Unicode sparkline (▁▂▃▄▅▆▇█) using automatic min/max scaling.
pub fn render(values: []const f64) -> SparkResult {
    return renderWithOptions(values, defaultOptions());
}

/// Render an ASCII-safe sparkline (._-:=+*#) using automatic min/max scaling.
pub fn renderAscii(values: []const f64) -> SparkResult {
    let opts: SparkOptions = defaultOptions();
    opts.asciiOnly = true;
    return renderWithOptions(values, opts);
}

/// Render a single horizontal bar with fractional eighth-block precision (▏▎▍▌▋▊▉█).
/// `ratio` is clamped to [0.0, 1.0]; `width` is total character cells.
pub fn renderBar(ratio: f64, width: usize, fillAscii: bool) -> SparkResult {
    let buf: [512]u8 = undefined;
    let res: SparkResult = SparkResult {
        buf: buf,
        len: 0,
        truncated: false,
    };

    if (width == 0) { return res; }

    let r: f64 = ratio;
    if (r < 0.0) { r = 0.0; }
    if (r > 1.0) { r = 1.0; }

    if (fillAscii) {
        const filledF: f64 = r * (width as f64) + 0.5;
        let filled: usize = filledF as usize;
        if (filled > width) { filled = width; }
        let i: usize = 0;
        while (i < width) {
            if (i < filled) {
                appendByte(&res, '#');
            } else {
                appendByte(&res, '-');
            }
            i += 1;
        }
        return res;
    }

    // Eighths precision: total eighths = round(r * width * 8)
    const totalEighthsF: f64 = r * ((width * 8) as f64) + 0.5;
    let totalEighths: usize = totalEighthsF as usize;
    const maxEighths: usize = width * 8;
    if (totalEighths > maxEighths) { totalEighths = maxEighths; }

    const fullCells: usize = totalEighths / 8;
    const remEighths: usize = totalEighths % 8;

    let cell: usize = 0;
    while (cell < fullCells) {
        // Full block U+2588 = E2 96 88
        if (res.len + 3 > 512) {
            res.truncated = true;
            return res;
        }
        res.buf[res.len] = 0xE2;
        res.buf[res.len + 1] = 0x96;
        res.buf[res.len + 2] = 0x88;
        res.len += 3;
        cell += 1;
    }

    if (remEighths > 0 && cell < width) {
        // Left partial blocks U+258F (1/8) down to U+2589 (7/8):
        // 1/8 = 0x8F, 2/8 = 0x8E, ..., 7/8 = 0x89 => 0x90 - remEighths
        if (res.len + 3 > 512) {
            res.truncated = true;
            return res;
        }
        res.buf[res.len] = 0xE2;
        res.buf[res.len + 1] = 0x96;
        res.buf[res.len + 2] = (0x90 - remEighths) as u8;
        res.len += 3;
        cell += 1;
    }

    while (cell < width) {
        appendByte(&res, ' ');
        cell += 1;
    }

    return res;
}

/// Downsample an arbitrary-length slice into `targetBuckets` buckets using bucket averages,
/// then render a Unicode sparkline of exact length `targetBuckets`.
pub fn renderDownsampled(values: []const f64, targetBuckets: usize, asciiOnly: bool) -> SparkResult {
    let buf: [512]u8 = undefined;
    let res: SparkResult = SparkResult {
        buf: buf,
        len: 0,
        truncated: false,
    };

    if (values.len == 0 || targetBuckets == 0) {
        return res;
    }

    let buckets: usize = targetBuckets;
    if (buckets > 128) { buckets = 128; }
    if (values.len <= buckets) {
        let opts: SparkOptions = defaultOptions();
        opts.asciiOnly = asciiOnly;
        return renderWithOptions(values, opts);
    }

    let avg: [128]f64 = undefined;
    let b: usize = 0;
    while (b < buckets) {
        const startIdx: usize = (b * values.len) / buckets;
        let endIdx: usize = ((b + 1) * values.len) / buckets;
        if (endIdx <= startIdx) { endIdx = startIdx + 1; }
        if (endIdx > values.len) { endIdx = values.len; }

        let sum: f64 = 0.0;
        let cnt: usize = 0;
        let j: usize = startIdx;
        while (j < endIdx) {
            sum = sum + values[j];
            cnt += 1;
            j += 1;
        }
        avg[b] = sum / (cnt as f64);
        b += 1;
    }

    let opts: SparkOptions = defaultOptions();
    opts.asciiOnly = asciiOnly;
    return renderWithOptions(avg[0..buckets], opts);
}

/// Render a labeled horizontal bar line: "label [bar]"
pub fn renderLabeledBar(label: string, ratio: f64, barWidth: usize, fillAscii: bool) -> SparkResult {
    let buf: [512]u8 = undefined;
    let res: SparkResult = SparkResult {
        buf: buf,
        len: 0,
        truncated: false,
    };
    appendBytes(&res, label);
    appendBytes(&res, " [");
    const bar: SparkResult = renderBar(ratio, barWidth, fillAscii);
    appendBytes(&res, bar.asSlice());
    appendByte(&res, ']');
    return res;
}

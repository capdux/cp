// @cap/d3-shape — Pure CP SVG shape and pie chart path generator
// Clean-room CP implementation inspired by d3-shape (ISC).

pub struct Point2D {
    x: f64,
    y: f64,
}

pub struct PieSlice {
    value: f64,
    index: usize,
    start_angle: f64,
    end_angle: f64,
    pad_angle: f64,
}

fn d3shWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) return pos;
    out[pos] = b;
    return pos + 1;
}

fn d3shWriteStr(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn d3shWriteU64(out: []u8, pos: usize, val: u64): usize {
    if (pos >= out.len) return pos;
    if (val == 0) {
        out[pos] = '0';
        return pos + 1;
    }
    let tmp: [24]u8 = [0 as u8; 24];
    let n: usize = 0;
    let v: u64 = val;
    while (v > 0) {
        const d: u64 = v % 10;
        tmp[n] = u8(d) + '0';
        n += 1;
        v = v / 10;
    }
    let p: usize = pos;
    while (n > 0 && p < out.len) {
        n -= 1;
        out[p] = tmp[n];
        p += 1;
    }
    return p;
}

fn d3shWriteFloat(out: []u8, pos: usize, val: f64): usize {
    let p: usize = pos;
    let neg: bool = val < 0.0;
    let abs_v: f64 = if (neg) -val else val;

    // Normalize near-zero to avoid "-0"
    if (abs_v < 0.0000005) {
        return d3shWriteByte(out, p, '0');
    }

    let int_part = @as(u64, @intFromFloat(abs_v));
    const rem: f64 = abs_v - (int_part as f64);
    let frac = @as(u64, @intFromFloat(rem * 1000000.0 + 0.5));
    if (frac >= 1000000) {
        int_part += 1;
        frac -= 1000000;
    }

    if (neg && (int_part > 0 || frac > 0)) {
        p = d3shWriteByte(out, p, '-');
    }

    p = d3shWriteU64(out, p, int_part);

    if (frac > 0) {
        p = d3shWriteByte(out, p, '.');
        let digits: [6]u8 = [0 as u8; 6];
        let f: u64 = frac;
        let idx: usize = 6;
        while (idx > 0) {
            idx -= 1;
            digits[idx] = u8(f % 10) + '0';
            f = f / 10;
        }
        let end: usize = 6;
        while (end > 0 && digits[end - 1] == '0') {
            end -= 1;
        }
        let j: usize = 0;
        while (j < end) : (j += 1) {
            p = d3shWriteByte(out, p, digits[j]);
        }
    }
    return p;
}

fn d3shWritePair(out: []u8, pos: usize, x: f64, y: f64): usize {
    let p: usize = d3shWriteFloat(out, pos, x);
    p = d3shWriteByte(out, p, ',');
    p = d3shWriteFloat(out, p, y);
    return p;
}

pub fn computePie(
    values: []const f64,
    start_angle: f64,
    end_angle: f64,
    pad_angle: f64,
    out: []PieSlice,
): usize {
    const n: usize = if (values.len < out.len) values.len else out.len;
    if (n == 0) return 0;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (values[i] > 0.0) {
            sum += values[i];
        }
    }

    const da: f64 = end_angle - start_angle;
    const k: f64 = if (sum > 0.0) (da / sum) else 0.0;

    let cur: f64 = start_angle;
    i = 0;
    while (i < n) : (i += 1) {
        const v: f64 = if (values[i] > 0.0) values[i] else 0.0;
        const next_angle: f64 = cur + v * k;
        out[i] = PieSlice{
            value: values[i],
            index: i,
            start_angle: cur,
            end_angle: next_angle,
            pad_angle: pad_angle,
        };
        cur = next_angle;
    }
    return n;
}

pub fn linePath(pts: []const Point2D, out: []u8): usize {
    if (pts.len == 0) return 0;
    let p: usize = 0;
    p = d3shWriteStr(out, p, "M ");
    p = d3shWritePair(out, p, pts[0].x, pts[0].y);
    let i: usize = 1;
    while (i < pts.len) : (i += 1) {
        p = d3shWriteStr(out, p, " L ");
        p = d3shWritePair(out, p, pts[i].x, pts[i].y);
    }
    return p;
}

pub fn stepLinePath(pts: []const Point2D, out: []u8): usize {
    if (pts.len == 0) return 0;
    let p: usize = 0;
    p = d3shWriteStr(out, p, "M ");
    p = d3shWritePair(out, p, pts[0].x, pts[0].y);
    let i: usize = 1;
    while (i < pts.len) : (i += 1) {
        p = d3shWriteStr(out, p, " H ");
        p = d3shWriteFloat(out, p, pts[i].x);
        p = d3shWriteStr(out, p, " V ");
        p = d3shWriteFloat(out, p, pts[i].y);
    }
    return p;
}

pub fn arcPath(
    inner_r: f64,
    outer_r: f64,
    start_angle: f64,
    end_angle: f64,
    out: []u8,
): usize {
    if (outer_r <= 0.0) return 0;
    const pi: f64 = std.math.pi;
    const tau: f64 = 2.0 * pi;
    const diff: f64 = end_angle - start_angle;
    const da: f64 = if (diff < 0.0) -diff else diff;

    let p: usize = 0;

    // Full circle / full annulus
    if (da >= tau - 1.0e-6) {
        p = d3shWriteStr(out, p, "M ");
        p = d3shWritePair(out, p, 0.0, -outer_r);
        p = d3shWriteStr(out, p, " A ");
        p = d3shWritePair(out, p, outer_r, outer_r);
        p = d3shWriteStr(out, p, " 0 1,1 ");
        p = d3shWritePair(out, p, 0.0, outer_r);
        p = d3shWriteStr(out, p, " A ");
        p = d3shWritePair(out, p, outer_r, outer_r);
        p = d3shWriteStr(out, p, " 0 1,1 ");
        p = d3shWritePair(out, p, 0.0, -outer_r);
        p = d3shWriteStr(out, p, " Z");

        if (inner_r > 0.0) {
            p = d3shWriteStr(out, p, " M ");
            p = d3shWritePair(out, p, 0.0, -inner_r);
            p = d3shWriteStr(out, p, " A ");
            p = d3shWritePair(out, p, inner_r, inner_r);
            p = d3shWriteStr(out, p, " 0 1,0 ");
            p = d3shWritePair(out, p, 0.0, inner_r);
            p = d3shWriteStr(out, p, " A ");
            p = d3shWritePair(out, p, inner_r, inner_r);
            p = d3shWriteStr(out, p, " 0 1,0 ");
            p = d3shWritePair(out, p, 0.0, -inner_r);
            p = d3shWriteStr(out, p, " Z");
        }
        return p;
    }

    const sweep: u8 = if (diff >= 0.0) 1 else 0;
    const inv_sweep: u8 = if (sweep == 1) 0 else 1;
    const large_arc: u8 = if (da > pi) 1 else 0;

    const s0: f64 = std.math.sin(start_angle);
    const c0: f64 = std.math.cos(start_angle);
    const s1: f64 = std.math.sin(end_angle);
    const c1: f64 = std.math.cos(end_angle);

    const x0: f64 = outer_r * s0;
    const y0: f64 = -outer_r * c0;
    const x1: f64 = outer_r * s1;
    const y1: f64 = -outer_r * c1;

    p = d3shWriteStr(out, p, "M ");
    p = d3shWritePair(out, p, x0, y0);
    p = d3shWriteStr(out, p, " A ");
    p = d3shWritePair(out, p, outer_r, outer_r);
    p = d3shWriteStr(out, p, " 0 ");
    p = d3shWriteByte(out, p, large_arc + '0');
    p = d3shWriteByte(out, p, ',');
    p = d3shWriteByte(out, p, sweep + '0');
    p = d3shWriteByte(out, p, ' ');
    p = d3shWritePair(out, p, x1, y1);

    if (inner_r > 0.0) {
        const x2: f64 = inner_r * s1;
        const y2: f64 = -inner_r * c1;
        const x3: f64 = inner_r * s0;
        const y3: f64 = -inner_r * c0;

        p = d3shWriteStr(out, p, " L ");
        p = d3shWritePair(out, p, x2, y2);
        p = d3shWriteStr(out, p, " A ");
        p = d3shWritePair(out, p, inner_r, inner_r);
        p = d3shWriteStr(out, p, " 0 ");
        p = d3shWriteByte(out, p, large_arc + '0');
        p = d3shWriteByte(out, p, ',');
        p = d3shWriteByte(out, p, inv_sweep + '0');
        p = d3shWriteByte(out, p, ' ');
        p = d3shWritePair(out, p, x3, y3);
        p = d3shWriteStr(out, p, " Z");
    } else {
        p = d3shWriteStr(out, p, " L 0,0 Z");
    }
    return p;
}

// @cap/d3-path — Zero-allocation SVG path `d` attribute builder in pure CP
// Clean-room MIT implementation of npm `d3-path` (#400, 134M/wk).

pub const D3P_BUF_CAP: usize = 2048;

fn d3pPushStr(buf: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < buf.len) {
        buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn d3pPushU64(buf: []u8, pos: usize, val: u64): usize {
    let rev: [20]u8 = undefined;
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        rev[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            rev[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < buf.len) {
        k -= 1;
        buf[p] = rev[k];
        p += 1;
    }
    return p;
}

/// Formats `val` with up to 3 decimal digits, stripping trailing fractional zeros (e.g. `10.0` -> `10`, `1.5` -> `1.5`).
fn d3pPushNum(buf: []u8, pos: usize, val: f64): usize {
    let p = pos;
    let v = val;
    if (v < 0.0) {
        if (v > -0.0005) {
            return d3pPushStr(buf, p, "0");
        }
        p = d3pPushStr(buf, p, "-");
        v = -v;
    }
    const scaled: u64 = (v * 1000.0 + 0.5) as u64;
    const whole: u64 = scaled / 1000;
    const frac: u64 = scaled % 1000;
    p = d3pPushU64(buf, p, whole);
    if (frac == 0) {
        return p;
    }
    p = d3pPushStr(buf, p, ".");
    const d1: u8 = (48 + (frac / 100)) as u8;
    const d2: u8 = (48 + ((frac / 10) % 10)) as u8;
    const d3: u8 = (48 + (frac % 10)) as u8;
    if (p < buf.len) { buf[p] = d1; p += 1; }
    if (d2 != 48 || d3 != 48) {
        if (p < buf.len) { buf[p] = d2; p += 1; }
    }
    if (d3 != 48) {
        if (p < buf.len) { buf[p] = d3; p += 1; }
    }
    return p;
}

/// Canvas2D-compatible SVG path builder storing serialized `d` string in a fixed stack buffer.
pub struct SvgPath {
    buf: [2048]u8,
    len: usize,
    has_subpath: bool,
    x0: f64,
    y0: f64,
    x1: f64,
    y1: f64,

    pub static fn new(): SvgPath {
        return SvgPath {
            buf: [0; 2048],
            len: 0,
            has_subpath: false,
            x0: 0.0,
            y0: 0.0,
            x1: 0.0,
            y1: 0.0,
        };
    }

    pub fn byteLen(): usize {
        return _.len;
    }

    pub fn reset(*_) {
        _.len = 0;
        _.has_subpath = false;
        _.x0 = 0.0;
        _.y0 = 0.0;
        _.x1 = 0.0;
        _.y1 = 0.0;
    }

    pub fn moveTo(*_, x: f64, y: f64) {
        _.x0 = x;
        _.y0 = y;
        _.x1 = x;
        _.y1 = y;
        _.has_subpath = true;
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "M");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, y);
    }

    pub fn lineTo(*_, x: f64, y: f64) {
        _.x1 = x;
        _.y1 = y;
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "L");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, y);
    }

    pub fn quadraticCurveTo(*_, cpx: f64, cpy: f64, x: f64, y: f64) {
        _.x1 = x;
        _.y1 = y;
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "Q");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cpx);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cpy);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, y);
    }

    pub fn bezierCurveTo(*_, cp1x: f64, cp1y: f64, cp2x: f64, cp2y: f64, x: f64, y: f64) {
        _.x1 = x;
        _.y1 = y;
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "C");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cp1x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cp1y);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cp2x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, cp2y);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, y);
    }

    pub fn rect(*_, x: f64, y: f64, w: f64, h: f64) {
        _.x0 = x;
        _.y0 = y;
        _.x1 = x;
        _.y1 = y;
        _.has_subpath = true;
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "M");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, x);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, ",");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, y);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "h");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, w);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "v");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, h);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "h");
        _.len = d3pPushNum(_.buf[0..D3P_BUF_CAP], _.len, -w);
        _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "Z");
    }

    pub fn closePath(*_) {
        if (_.has_subpath) {
            _.x1 = _.x0;
            _.y1 = _.y0;
            _.len = d3pPushStr(_.buf[0..D3P_BUF_CAP], _.len, "Z");
        }
    }

    /// Copies the serialized SVG path `d` string into `out`, returning bytes written.
    pub fn writeTo(out: []u8): usize {
        let n = _.len;
        if (out.len < n) { n = out.len; }
        let i: usize = 0;
        while (i < n) {
            out[i] = _.buf[i];
            i += 1;
        }
        return n;
    }
}

export {
    D3P_BUF_CAP,
    SvgPath,
};

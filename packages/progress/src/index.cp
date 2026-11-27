// @cap/progress — Zero-heap terminal progress bar renderer in pure CP
// Clean-room MIT implementation of npm `progress` (#726, 80.6M/wk).

import { startsWith } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub struct ProgressBar {
    pub total: u64,
    pub current: u64,
    pub width: usize,
    pub complete_char: u8,
    pub incomplete_char: u8,
    pub head_char: u8,
    pub elapsed_ms: u64,

    pub fn ratio(): f64 {
        if (_.total == 0) {
            return 1.0;
        }
        if (_.current >= _.total) {
            return 1.0;
        }
        return (_.current as f64) / (_.total as f64);
    }

    pub fn percent(): u32 {
        return (_.ratio() * 100.0 + 0.5) as u32;
    }

    pub fn isComplete(): bool {
        return _.current >= _.total;
    }

    pub fn tick(*_, delta: u64) {
        _.current += delta;
        if (_.current > _.total) {
            _.current = _.total;
        }
    }

    /// Estimated remaining time in seconds based on `elapsed_ms`.
    pub fn etaSeconds(): f64 {
        if (_.current == 0 || _.current >= _.total || _.elapsed_ms == 0) {
            return 0.0;
        }
        const rem = (_.total - _.current) as f64;
        const rate_per_ms = (_.current as f64) / (_.elapsed_ms as f64);
        if (rate_per_ms <= 0.0) {
            return 0.0;
        }
        return (rem / rate_per_ms) / 1000.0;
    }
}

pub fn newProgressBar(total: u64, width: usize): ProgressBar {
    let w = width;
    if (w == 0) { w = 20; }
    return ProgressBar {
        total: total,
        current: 0,
        width: w,
        complete_char: 61,   // '='
        incomplete_char: 45, // '-'
        head_char: 62,       // '>'
        elapsed_ms: 0,
    };
}

fn prgWriteBar(bar: ProgressBar, out: []u8, pos: usize): usize {
    let p = pos;
    const filled = (bar.ratio() * (bar.width as f64)) as usize;
    let i: usize = 0;
    while (i < bar.width && p < out.len) {
        if (i < filled) {
            if (i + 1 == filled && filled < bar.width && bar.head_char != 0) {
                out[p] = bar.head_char;
            } else {
                out[p] = bar.complete_char;
            }
        } else {
            out[p] = bar.incomplete_char;
        }
        p += 1;
        i += 1;
    }
    return p;
}

/// Renders a template string substituting `:bar`, `:percent`, `:current`, `:total`, `:etas`, `:elapseds`.
pub fn renderProgress(bar: ProgressBar, template: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < template.len) {
        if (template[i] == 58) { // ':'
            const tail = template[i..];
            if (startsWith(tail, ":bar")) {
                p = prgWriteBar(bar, out, p);
                i += 4;
                continue;
            }
            if (startsWith(tail, ":percent")) {
                p = writeU64(bar.percent() as u64, out, p);
                p = writeStr("%", out, p);
                i += 8;
                continue;
            }
            if (startsWith(tail, ":current")) {
                p = writeU64(bar.current, out, p);
                i += 8;
                continue;
            }
            if (startsWith(tail, ":total")) {
                p = writeU64(bar.total, out, p);
                i += 6;
                continue;
            }
            if (startsWith(tail, ":etas")) {
                const eta_tenths = (bar.etaSeconds() * 10.0 + 0.5) as u64;
                p = writeU64(eta_tenths / 10, out, p);
                p = writeStr(".", out, p);
                p = writeU64(eta_tenths % 10, out, p);
                p = writeStr("s", out, p);
                i += 5;
                continue;
            }
            if (startsWith(tail, ":elapseds")) {
                const el_tenths = (bar.elapsed_ms + 50) / 100;
                p = writeU64(el_tenths / 10, out, p);
                p = writeStr(".", out, p);
                p = writeU64(el_tenths % 10, out, p);
                p = writeStr("s", out, p);
                i += 9;
                continue;
            }
        }
        if (p < out.len) {
            out[p] = template[i];
            p += 1;
        }
        i += 1;
    }
    return p;
}

export {
    ProgressBar,
    newProgressBar,
    renderProgress,
};

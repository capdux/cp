// @cap/server-timing — Zero-allocation W3C Server-Timing Header Parser & Formatter
// Clean-room MIT implementation in pure CP.

import { isAlpha, isDigit, strEq, eqIgnoreCase, trim, findByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub struct TimingMetric {
    pub name: string,
    pub has_dur: bool,
    pub dur_ms: f64,
    pub desc: string,
}

struct ParsedFloat {
    val: f64,
    ok: bool,
}

fn parseFloatSimple(s: string) -> ParsedFloat {
    const t = trim(s);
    if (t.len == 0) {
        return ParsedFloat { val: 0.0, ok: false };
    }
    let i: usize = 0;
    let neg = false;
    if (t[0] == 45) {
        neg = true;
        i = 1;
    } else if (t[0] == 43) {
        i = 1;
    }
    if (i >= t.len) {
        return ParsedFloat { val: 0.0, ok: false };
    }

    let int_part: f64 = 0.0;
    let has_digit = false;
    while (i < t.len && t[i] >= 48 && t[i] <= 57) {
        int_part = int_part * 10.0 + ((t[i] - 48) as f64);
        has_digit = true;
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let scale: f64 = 0.1;
    if (i < t.len && t[i] == 46) {
        i += 1;
        while (i < t.len && t[i] >= 48 && t[i] <= 57) {
            frac_part += ((t[i] - 48) as f64) * scale;
            scale *= 0.1;
            has_digit = true;
            i += 1;
        }
    }

    if (!has_digit || i != t.len) {
        return ParsedFloat { val: 0.0, ok: false };
    }

    let result = int_part + frac_part;
    if (neg) {
        result = -result;
    }
    return ParsedFloat { val: result, ok: true };
}

fn parseSingleMetric(seg: string, out: *TimingMetric) -> bool {
    const n = seg.len;
    let pos: usize = 0;

    // Extract metric name up to first unquoted ';'
    const semi_opt = findByte(seg, 59, 0);
    const name_end = semi_opt ?? n;
    const name = trim(seg[0..name_end]);
    if (name.len == 0) {
        return false;
    }

    let has_dur = false;
    let dur_ms: f64 = 0.0;
    let desc: string = "";

    pos = name_end;
    if (pos < n && seg[pos] == 59) {
        pos += 1;
    }

    while (pos < n) {
        const p_start = pos;
        let in_quote = false;
        while (pos < n) {
            const ch = seg[pos];
            if (ch == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 59 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const p_end = pos;
        if (pos < n && seg[pos] == 59) {
            pos += 1;
        }

        const param = trim(seg[p_start..p_end]);
        if (param.len == 0) {
            continue;
        }

        // Find '='
        const eq_opt = findByte(param, 61, 0);
        if (eq_opt == null) {
            continue;
        }
        const eq_pos = eq_opt.?;
        const pname: string = trim(param[0..eq_pos]);
        const pval: string = trim(param[eq_pos + 1..param.len]);

        if (eqIgnoreCase(pname, "dur")) {
            const pf = parseFloatSimple(pval);
            if (pf.ok && !has_dur) {
                has_dur = true;
                dur_ms = pf.val;
            }
        } else if (eqIgnoreCase(pname, "desc")) {
            if (desc.len == 0 && pval.len > 0) {
                if (pval.len >= 2 && pval[0] == 34 && pval[pval.len - 1] == 34) {
                    desc = pval[1..pval.len - 1];
                } else {
                    desc = pval;
                }
            }
        }
    }

    out.name = name;
    out.has_dur = has_dur;
    out.dur_ms = dur_ms;
    out.desc = desc;
    return true;
}

/// Parses a comma-separated W3C `Server-Timing` header into `out`.
/// Returns the number of parsed metrics written into `out`.
pub fn parseHeader(header: string, out: []TimingMetric) -> usize {
    const n = header.len;
    let pos: usize = 0;
    let count: usize = 0;

    while (pos < n && count < out.len) {
        const start = pos;
        let in_quote = false;
        while (pos < n) {
            const ch = header[pos];
            if (ch == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const end = pos;
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }

        let m = TimingMetric { name: "", has_dur: false, dur_ms: 0.0, desc: "" };
        if (parseSingleMetric(header[start..end], &m)) {
            out[count] = m;
            count += 1;
        }
    }

    return count;
}

/// Finds the first metric with exact `name` in `header` and writes it into `out`.
pub fn findMetric(header: string, name: string, out: *TimingMetric) -> bool {
    const n = header.len;
    let pos: usize = 0;
    while (pos < n) {
        const start = pos;
        let in_quote = false;
        while (pos < n) {
            const ch = header[pos];
            if (ch == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const end = pos;
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }

        let m = TimingMetric { name: "", has_dur: false, dur_ms: 0.0, desc: "" };
        if (parseSingleMetric(header[start..end], &m)) {
            if (strEq(m.name, name)) {
                out.name = m.name;
                out.has_dur = m.has_dur;
                out.dur_ms = m.dur_ms;
                out.desc = m.desc;
                return true;
            }
        }
    }
    return false;
}

/// Sums the `dur` milliseconds of all metrics in `header` that specify a duration.
pub fn totalDurationMs(header: string) -> f64 {
    const n = header.len;
    let pos: usize = 0;
    let total: f64 = 0.0;
    while (pos < n) {
        const start = pos;
        let in_quote = false;
        while (pos < n) {
            const ch = header[pos];
            if (ch == 92 && in_quote && pos + 1 < n) {
                pos += 2;
                continue;
            }
            if (ch == 34) {
                in_quote = !in_quote;
            } else if (ch == 44 && !in_quote) {
                break;
            }
            pos += 1;
        }
        const end = pos;
        if (pos < n && header[pos] == 44) {
            pos += 1;
        }

        let m = TimingMetric { name: "", has_dur: false, dur_ms: 0.0, desc: "" };
        if (parseSingleMetric(header[start..end], &m)) {
            if (m.has_dur) {
                total += m.dur_ms;
            }
        }
    }
    return total;
}

fn isTokenChar(b: u8) -> bool {
    return isDigit(b) || isAlpha(b) || b == 45 || b == 95 || b == 46; // '-', '_', '.'
}

fn descNeedsQuotes(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        if (!isTokenChar(s[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    return writeStr(src, dst, pos) as i64;
}

fn pushU64(dst: []u8, pos: usize, val: u64) -> i64 {
    let tmp: [24]u8 = undefined;
    const n = writeU64(val, &tmp, 0);
    return pushSlice(dst, pos, tmp[0..n]);
}

fn pushDurMs(dst: []u8, pos: usize, dur_ms: f64) -> i64 {
    let p = pos as i64;
    let v = dur_ms;
    if (v < 0.0) {
        p = pushSlice(dst, p as usize, "-");
        if (p < 0) { return -1; }
        v = -v;
    }
    // Round to nearest thousandth (0.001 ms)
    const scaled = (v * 1000.0 + 0.5) as u64;
    const whole = scaled / 1000;
    const frac = scaled % 1000;

    p = pushU64(dst, p as usize, whole);
    if (p < 0) { return -1; }

    if (frac > 0) {
        p = pushSlice(dst, p as usize, ".");
        if (p < 0) { return -1; }
        const d1 = (frac / 100) % 10;
        const d2 = (frac / 10) % 10;
        const d3 = frac % 10;
        if ((p as usize) >= dst.len) { return -1; }
        dst[p as usize] = (48 + d1) as u8;
        p += 1;
        if (d2 != 0 || d3 != 0) {
            if ((p as usize) >= dst.len) { return -1; }
            dst[p as usize] = (48 + d2) as u8;
            p += 1;
        }
        if (d3 != 0) {
            if ((p as usize) >= dst.len) { return -1; }
            dst[p as usize] = (48 + d3) as u8;
            p += 1;
        }
    }
    return p;
}

/// Formats a single `TimingMetric` into `out`.
/// Automatically quotes and escapes `"` and `\` in `desc` when it contains spaces or special chars.
/// Returns bytes written, or `0` if invalid or `out` is too small.
pub fn formatMetric(metric: TimingMetric, out: []u8) -> usize {
    if (metric.name.len == 0) {
        return 0;
    }
    let p = pushSlice(out, 0, metric.name);
    if (p < 0) { return 0; }

    if (metric.has_dur) {
        p = pushSlice(out, p as usize, ";dur=");
        if (p < 0) { return 0; }
        p = pushDurMs(out, p as usize, metric.dur_ms);
        if (p < 0) { return 0; }
    }

    if (metric.desc.len > 0) {
        p = pushSlice(out, p as usize, ";desc=");
        if (p < 0) { return 0; }
        if (descNeedsQuotes(metric.desc)) {
            p = pushSlice(out, p as usize, "\"");
            if (p < 0) { return 0; }
            let i: usize = 0;
            while (i < metric.desc.len) {
                const ch = metric.desc[i];
                if (ch == 34 || ch == 92) {
                    if ((p as usize) + 2 > out.len) { return 0; }
                    out[p as usize] = 92;
                    out[(p as usize) + 1] = ch;
                    p += 2;
                } else {
                    if ((p as usize) >= out.len) { return 0; }
                    out[p as usize] = ch;
                    p += 1;
                }
                i += 1;
            }
            p = pushSlice(out, p as usize, "\"");
            if (p < 0) { return 0; }
        } else {
            p = pushSlice(out, p as usize, metric.desc);
            if (p < 0) { return 0; }
        }
    }

    return p as usize;
}

/// Formats multiple `TimingMetric` entries separated by `", "` into `out`.
/// Returns total bytes written, or `0` on empty input or buffer overflow.
pub fn formatHeader(metrics: []const TimingMetric, out: []u8) -> usize {
    if (metrics.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < metrics.len) {
        if (i > 0) {
            if (pos + 2 > out.len) {
                return 0;
            }
            out[pos] = 44;     // ','
            out[pos + 1] = 32; // ' '
            pos += 2;
        }
        const n = formatMetric(metrics[i], out[pos..out.len]);
        if (n == 0) {
            return 0;
        }
        pos += n;
        i += 1;
    }
    return pos;
}

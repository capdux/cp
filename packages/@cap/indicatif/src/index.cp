// @cap/indicatif — Zero-heap terminal progress bar, throughput & ETA renderer in pure CP
// MIT License — Clean-room implementation inspired by Rust `indicatif`,
// composing @cap/pretty-bytes and @cap/pretty-ms directly.

import { prettyBytes } from "../../pretty-bytes/src/index.cp";
import { prettyMs } from "../../pretty-ms/src/index.cp";

pub struct ProgressState {
    pub pos: u64,
    pub total: u64,
    pub elapsed_ms: u64,
    pub width: usize,
}

fn indWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn indWriteStr(out: []u8, pos: usize, s: []const u8): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn indWriteU64(out: []u8, pos: usize, val: u64): usize {
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    if (val == 0) {
        return indWriteByte(out, pos, 48);
    }
    let v: u64 = val;
    while (v > 0 && count < 20) {
        digits[count] = ((v % 10) as u8) + 48;
        count += 1;
        v = v / 10;
    }
    let p: usize = pos;
    let k: usize = count;
    while (k > 0 && p < out.len) {
        k -= 1;
        p = indWriteByte(out, p, digits[k]);
    }
    return p;
}

/// Completion percentage clamped to `[0.0, 100.0]`. Returns `0.0` if `total == 0`.
pub fn percent(s: ProgressState): f64 {
    if (s.total == 0) {
        return 0.0;
    }
    if (s.pos >= s.total) {
        return 100.0;
    }
    return ((s.pos as f64) * 100.0) / (s.total as f64);
}

/// Estimated remaining time in milliseconds based on current average throughput.
/// Returns `0` when `pos == 0`, `elapsed_ms == 0`, `total == 0`, or already complete.
pub fn etaMs(s: ProgressState): u64 {
    if (s.pos == 0 || s.total == 0 || s.pos >= s.total || s.elapsed_ms == 0) {
        return 0;
    }
    const remaining: u64 = s.total - s.pos;
    const est: f64 = ((remaining as f64) * (s.elapsed_ms as f64)) / (s.pos as f64) + 0.5;
    return est as u64;
}

/// Average throughput in bytes per second (`0.0` if `elapsed_ms == 0`).
pub fn bytesPerSec(s: ProgressState): f64 {
    if (s.elapsed_ms == 0 || s.pos == 0) {
        return 0.0;
    }
    return ((s.pos as f64) * 1000.0) / (s.elapsed_ms as f64);
}

/// Render a progress bar `[====>     ]` of inner width `s.width` into `out`.
/// Returns bytes written (safely truncated to `out.len`).
pub fn renderBar(s: ProgressState, fill_ch: u8, head_ch: u8, empty_ch: u8, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    let p: usize = indWriteByte(out, 0, 91); // '['

    if (s.width > 0) {
        let filled: usize = 0;
        if (s.total > 0 && s.pos > 0) {
            if (s.pos >= s.total) {
                filled = s.width;
            } else {
                filled = (((s.pos as f64) * (s.width as f64)) / (s.total as f64)) as usize;
                if (filled > s.width) {
                    filled = s.width;
                }
            }
        }

        const effective_head: u8 = if (head_ch != 0) head_ch else fill_ch;
        let i: usize = 0;
        while (i < s.width && p < out.len) {
            if (filled == 0) {
                p = indWriteByte(out, p, empty_ch);
            } else if (filled >= s.width) {
                p = indWriteByte(out, p, fill_ch);
            } else if (i + 1 < filled) {
                p = indWriteByte(out, p, fill_ch);
            } else if (i + 1 == filled) {
                p = indWriteByte(out, p, effective_head);
            } else {
                p = indWriteByte(out, p, empty_ch);
            }
            i += 1;
        }
    }

    p = indWriteByte(out, p, 93); // ']'
    return p;
}

/// Render a full status line: `<prefix> [====>     ] 50% 512 B/1.02 kB ETA 2s`
/// composing `@cap/pretty-bytes` and `@cap/pretty-ms` with zero heap allocation.
pub fn renderLine(s: ProgressState, prefix: string, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    let p: usize = 0;
    if (prefix.len > 0) {
        p = indWriteStr(out, p, prefix);
        p = indWriteByte(out, p, 32); // ' '
    }

    if (p < out.len) {
        const bar_n: usize = renderBar(s, 61, 62, 32, out[p..out.len]); // '=', '>', ' '
        p += bar_n;
    }

    p = indWriteByte(out, p, 32);
    const pct_int: u64 = (percent(s) + 0.5) as u64;
    p = indWriteU64(out, p, pct_int);
    p = indWriteStr(out, p, "% ");

    let pos_buf: [32]u8 = undefined;
    const pos_len: usize = prettyBytes(s.pos as f64, &pos_buf);
    p = indWriteStr(out, p, pos_buf[0..pos_len]);
    p = indWriteByte(out, p, 47); // '/'

    let tot_buf: [32]u8 = undefined;
    const tot_len: usize = prettyBytes(s.total as f64, &tot_buf);
    p = indWriteStr(out, p, tot_buf[0..tot_len]);

    p = indWriteStr(out, p, " ETA ");
    let eta_buf: [32]u8 = undefined;
    const eta_str: string = prettyMs(etaMs(s) as i64, &eta_buf);
    p = indWriteStr(out, p, eta_str);

    return p;
}

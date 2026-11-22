// Source: Absorbed from /data/dev/bun/src/bun_core/fmt.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation human-readable formatting utilities: byte sizes,
// nanosecond durations, URL credential/UUID/token redaction, and CLI table borders.

pub enum TableStyle: u8 {
    ascii = 0,
    unicode = 1,
}

/// Format a byte count into human-readable IEC/binary units (`B`, `KB`, `MB`, `GB`, `TB`) into `out`.
pub fn formatBytes(bytes: u64, out: []u8): ([]const u8)? {
    if (bytes < 1024) {
        let p: usize = 0;
        if (!appendU64(out, &p, bytes)) return null;
        if (!appendStr(out, &p, " B")) return null;
        return out[0..p];
    }
    const kb: f64 = @as(f64, @floatFromInt(bytes)) / 1024.0;
    if (kb < 1024.0) return formatScaledUnit(kb, " KB", out);
    const mb = kb / 1024.0;
    if (mb < 1024.0) return formatScaledUnit(mb, " MB", out);
    const gb = mb / 1024.0;
    if (gb < 1024.0) return formatScaledUnit(gb, " GB", out);
    const tb = gb / 1024.0;
    return formatScaledUnit(tb, " TB", out);
}

/// Format a duration in nanoseconds into compact human-readable form (`ns`, `us`, `ms`, `s`, `m`).
pub fn formatDurationNs(ns: u64, out: []u8): ([]const u8)? {
    if (ns < 1000) {
        let p: usize = 0;
        if (!appendU64(out, &p, ns)) return null;
        if (!appendStr(out, &p, "ns")) return null;
        return out[0..p];
    }
    if (ns < 1_000_000) {
        return formatScaledUnit(@as(f64, @floatFromInt(ns)) / 1000.0, "us", out);
    }
    if (ns < 1_000_000_000) {
        return formatScaledUnit(@as(f64, @floatFromInt(ns)) / 1_000_000.0, "ms", out);
    }
    if (ns < 60_000_000_000) {
        return formatScaledUnit(@as(f64, @floatFromInt(ns)) / 1_000_000_000.0, "s", out);
    }
    const total_secs = ns / 1_000_000_000;
    const mins = total_secs / 60;
    const rem_secs = total_secs % 60;
    let p: usize = 0;
    if (!appendU64(out, &p, mins)) return null;
    if (!appendStr(out, &p, "m")) return null;
    if (rem_secs > 0) {
        if (!appendU64(out, &p, rem_secs)) return null;
        if (!appendStr(out, &p, "s")) return null;
    }
    return out[0..p];
}

/// Redact sensitive segments in a URL/log string into `out`:
/// - `https://user:secret@host` -> `https://user:***@host`
/// - UUIDs (`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`) -> `***`
/// - npm tokens (`npm_<alnum>`) -> `***`
pub fn redactUrl(input: []const u8, out: []u8): ([]const u8)? {
    let p: usize = 0;
    let i: usize = 0;

    // Detect `scheme://user:pass@` password span
    let pass_start: usize? = null;
    let pass_end: usize = 0;
    if (scheme_end := findSubstr(input, "://")) {
        const auth_start = scheme_end + 3;
        let auth_end = auth_start;
        while (auth_end < input.len && input[auth_end] != '/' && input[auth_end] != '?') : (auth_end += 1) {}
        if (at := findCharInRange(input, auth_start, auth_end, '@')) {
            if (colon := findCharInRange(input, auth_start, at, ':')) {
                pass_start = colon + 1;
                pass_end = at;
            }
        }
    }

    while (i < input.len) {
        if (ps := pass_start) {
            if (i == ps) {
                if (!appendStr(out, &p, "***")) return null;
                i = pass_end;
                continue;
            }
        }
        if (startsWithUuid(input[i..])) {
            if (!appendStr(out, &p, "***")) return null;
            i += 36;
            continue;
        }
        const tok_len = npmTokenLen(input[i..]);
        if (tok_len > 0) {
            if (!appendStr(out, &p, "***")) return null;
            i += tok_len;
            continue;
        }
        if (p >= out.len) return null;
        out[p] = input[i];
        p += 1;
        i += 1;
    }
    return out[0..p];
}

/// Render a horizontal table separator line (`top`, `middle`, or `bottom`) into `out`.
/// `pos_kind`: 0 = top, 1 = middle, 2 = bottom.
pub fn formatTableBorder(widths: []const usize, style: TableStyle, pos_kind: u8, out: []u8): ([]const u8)? {
    let left: []const u8 = "+";
    let mid: []const u8 = "+";
    let right: []const u8 = "+";
    let horiz: []const u8 = "-";

    if (style == .unicode) {
        horiz = "─";
        if (pos_kind == 0) {
            left = "┌";
            mid = "┬";
            right = "┐";
        } else if (pos_kind == 1) {
            left = "├";
            mid = "┼";
            right = "┤";
        } else {
            left = "└";
            mid = "┴";
            right = "┘";
        }
    }

    let p: usize = 0;
    if (!appendStr(out, &p, left)) return null;
    let c: usize = 0;
    while (c < widths.len) : (c += 1) {
        if (c > 0) {
            if (!appendStr(out, &p, mid)) return null;
        }
        let k: usize = 0;
        const total_w = widths[c] + 2; // 1 space pad on each side
        while (k < total_w) : (k += 1) {
            if (!appendStr(out, &p, horiz)) return null;
        }
    }
    if (!appendStr(out, &p, right)) return null;
    return out[0..p];
}

/// Render a single padded table row into `out`.
pub fn formatTableRow(cells: []const []const u8, widths: []const usize, style: TableStyle, out: []u8): ([]const u8)? {
    if (cells.len != widths.len) return null;
    const vert: []const u8 = if (style == .unicode) "│" else "|";
    let p: usize = 0;
    let c: usize = 0;
    while (c < cells.len) : (c += 1) {
        if (!appendStr(out, &p, vert)) return null;
        if (!appendStr(out, &p, " ")) return null;
        const cell = cells[c];
        const w = widths[c];
        const print_len = @min(cell.len, w);
        if (!appendStr(out, &p, cell[0..print_len])) return null;
        let pad = w - print_len;
        while (pad > 0) : (pad -= 1) {
            if (!appendStr(out, &p, " ")) return null;
        }
        if (!appendStr(out, &p, " ")) return null;
    }
    if (!appendStr(out, &p, vert)) return null;
    return out[0..p];
}

// ---------- Helpers ----------

fn formatScaledUnit(val: f64, suffix: []const u8, out: []u8): ([]const u8)? {
    const scaled: u64 = @intFromFloat(val * 100.0 + 0.5);
    const whole = scaled / 100;
    const frac = scaled % 100;
    let p: usize = 0;
    if (!appendU64(out, &p, whole)) return null;
    if (frac != 0) {
        if (!appendStr(out, &p, ".")) return null;
        if (frac % 10 == 0) {
            if (!appendU64(out, &p, frac / 10)) return null;
        } else {
            if (frac < 10) {
                if (!appendStr(out, &p, "0")) return null;
            }
            if (!appendU64(out, &p, frac)) return null;
        }
    }
    if (!appendStr(out, &p, suffix)) return null;
    return out[0..p];
}

fn isHex(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isAlphaNum(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn startsWithUuid(s: []const u8): bool {
    if (s.len < 36) return false;
    if (s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-') return false;
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        if (i == 8 || i == 13 || i == 18 || i == 23) continue;
        if (!isHex(s[i])) return false;
    }
    return true;
}

fn npmTokenLen(s: []const u8): usize {
    if (s.len < 12) return 0;
    if (s[0] != 'n' || s[1] != 'p' || s[2] != 'm' || s[3] != '_') return 0;
    let i: usize = 4;
    while (i < s.len && isAlphaNum(s[i])) : (i += 1) {}
    return if (i >= 12) i else 0;
}

fn findSubstr(s: []const u8, needle: []const u8): usize? {
    if (needle.len == 0 || s.len < needle.len) return null;
    let i: usize = 0;
    while (i + needle.len <= s.len) : (i += 1) {
        let ok = true;
        let k: usize = 0;
        while (k < needle.len) : (k += 1) {
            if (s[i + k] != needle[k]) {
                ok = false;
                break;
            }
        }
        if (ok) return i;
    }
    return null;
}

fn findCharInRange(s: []const u8, start: usize, end: usize, ch: u8): usize? {
    let i = start;
    while (i < end) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

fn appendStr(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.*] = s[i];
        pos.* += 1;
    }
    return true;
}

fn appendU64(out: []u8, pos: *usize, v_in: u64): bool {
    if (v_in == 0) return appendStr(out, pos, "0");
    let buf: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + @as(u8, @intCast(v % 10)));
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    while (n > 0) {
        n -= 1;
        out[pos.*] = buf[n];
        pos.* += 1;
    }
    return true;
}

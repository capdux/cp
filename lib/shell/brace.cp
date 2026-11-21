// Source: Absorbed from /data/dev/bun/src/glob.zig | Synced: 2026-11-21T15:26:33Z (Bash-Style Brace Expansion Engine)
// Pure-CP zero-allocation shell brace expansion: comma lists {a,b,c}, integer ranges {1..5..2}, zero-padded {01..05}, alpha {a..e}.

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
        if (i >= s.len) return null;
    }
    let val: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        if (val > 922337203685477580) return null;
        val = val * 10 + @as(i64, c - '0');
        i += 1;
    }
    return if (neg) 0 - val else val;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writePaddedInt(out: []u8, pos: *usize, val: i64, pad_width: usize): bool {
    let v = val;
    let neg = false;
    if (v < 0) {
        neg = true;
        v = 0 - v;
    }
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, '0') + @as(u8, v % 10);
            v /= 10;
            n += 1;
        }
    }
    const neg_len: usize = if (neg) 1 else 0;
    const total_digits = if (n + neg_len < pad_width) pad_width - neg_len else n;
    if (pos.* + neg_len + total_digits > out.len) return false;
    if (neg) {
        out[pos.*] = '-';
        pos.* += 1;
    }
    let zeros = total_digits - n;
    while (zeros > 0) {
        out[pos.*] = '0';
        pos.* += 1;
        zeros -= 1;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

enum BraceMode {
    literal,
    comma_list,
    int_range,
    char_range,
}

export struct BraceExpandIter {
    prefix: []const u8,
    inner: []const u8,
    suffix: []const u8,
    mode: BraceMode,
    done: bool,
    // comma_list state
    list_pos: usize,
    // int_range / char_range state
    cur_val: i64,
    end_val: i64,
    step: i64,
    pad_width: usize,

    /// Initialize a zero-allocation iterator for a single-level brace expression (e.g. `"src/{a,b,c}.cp"`, `"img_{01..05}.png"`, `"{a..e}"`).
    /// Patterns without a valid expandable `{...}` group yield the original string once.
    pub fn init(pattern: []const u8): BraceExpandIter {
        let open_idx: usize? = null;
        let close_idx: usize? = null;
        let i: usize = 0;
        while (i < pattern.len) {
            if (pattern[i] == '\\' && i + 1 < pattern.len) {
                i += 2;
                continue;
            }
            if (pattern[i] == '{' && open_idx == null) {
                open_idx = i;
            } else if (pattern[i] == '}' && open_idx != null) {
                close_idx = i;
                break;
            }
            i += 1;
        }

        if (open_idx == null || close_idx == null) {
            return BraceExpandIter {
                prefix: pattern,
                inner: "",
                suffix: "",
                mode: BraceMode.literal,
                done: false,
                list_pos: 0,
                cur_val: 0,
                end_val: 0,
                step: 1,
                pad_width: 0,
            };
        }

        const ob = open_idx.?;
        const cb = close_idx.?;
        const prefix = pattern[0..ob];
        const inner = pattern[ob + 1..cb];
        const suffix = pattern[cb + 1..];

        // Check for comma list first
        let has_comma = false;
        let ci: usize = 0;
        while (ci < inner.len) {
            if (inner[ci] == ',') {
                has_comma = true;
                break;
            }
            ci += 1;
        }
        if (has_comma) {
            return BraceExpandIter {
                prefix: prefix,
                inner: inner,
                suffix: suffix,
                mode: BraceMode.comma_list,
                done: false,
                list_pos: 0,
                cur_val: 0,
                end_val: 0,
                step: 1,
                pad_width: 0,
            };
        }

        // Check for `start..end[..step]` sequence
        let dd1: usize? = null;
        let dd2: usize? = null;
        let di: usize = 0;
        while (di + 1 < inner.len) {
            if (inner[di] == '.' && inner[di + 1] == '.') {
                if (dd1 == null) {
                    dd1 = di;
                    di += 2;
                    continue;
                } else if (dd2 == null) {
                    dd2 = di;
                    di += 2;
                    continue;
                }
            }
            di += 1;
        }

        if (d1 := dd1) {
            const p1 = inner[0..d1];
            const p2 = if (d2 := dd2) inner[d1 + 2..d2] else inner[d1 + 2..];
            const p3 = if (d2 := dd2) inner[d2 + 2..] else "";

            // Integer range
            const s_opt = parseI64(p1);
            const e_opt = parseI64(p2);
            if (s_opt != null && e_opt != null) {
                let st: i64 = 1;
                let step_ok = true;
                if (dd2 != null) {
                    if (st_opt := parseI64(p3)) {
                        if (st_opt == 0) {
                            step_ok = false;
                        } else {
                            st = if (st_opt < 0) 0 - st_opt else st_opt;
                        }
                    } else {
                        step_ok = false;
                    }
                }
                if (step_ok) {
                    const sv = s_opt.?;
                    const ev = e_opt.?;
                    const signed_step = if (sv <= ev) st else 0 - st;
                    let pad: usize = 0;
                    const p1_zero = (p1.len > 1 && p1[0] == '0') || (p1.len > 2 && p1[0] == '-' && p1[1] == '0');
                    const p2_zero = (p2.len > 1 && p2[0] == '0') || (p2.len > 2 && p2[0] == '-' && p2[1] == '0');
                    if (p1_zero || p2_zero) {
                        pad = if (p1.len >= p2.len) p1.len else p2.len;
                    }
                    return BraceExpandIter {
                        prefix: prefix,
                        inner: inner,
                        suffix: suffix,
                        mode: BraceMode.int_range,
                        done: false,
                        list_pos: 0,
                        cur_val: sv,
                        end_val: ev,
                        step: signed_step,
                        pad_width: pad,
                    };
                }
            }

            // Single ASCII letter range (`a..z` or `Z..A`)
            if (p1.len == 1 && p2.len == 1 && dd2 == null) {
                const c1 = p1[0];
                const c2 = p2[0];
                const both_lower = (c1 >= 'a' && c1 <= 'z') && (c2 >= 'a' && c2 <= 'z');
                const both_upper = (c1 >= 'A' && c1 <= 'Z') && (c2 >= 'A' && c2 <= 'Z');
                if (both_lower || both_upper) {
                    const sv: i64 = @as(i64, c1);
                    const ev: i64 = @as(i64, c2);
                    return BraceExpandIter {
                        prefix: prefix,
                        inner: inner,
                        suffix: suffix,
                        mode: BraceMode.char_range,
                        done: false,
                        list_pos: 0,
                        cur_val: sv,
                        end_val: ev,
                        step: if (sv <= ev) 1 else -1,
                        pad_width: 0,
                    };
                }
            }
        }

        return BraceExpandIter {
            prefix: pattern,
            inner: "",
            suffix: "",
            mode: BraceMode.literal,
            done: false,
            list_pos: 0,
            cur_val: 0,
            end_val: 0,
            step: 1,
            pad_width: 0,
        };
    }

    pub fn next(*_, out: []u8): ([]const u8)? {
        if (_.done) return null;
        let pos: usize = 0;

        if (_.mode == BraceMode.literal) {
            _.done = true;
            if (!writeSlice(out, &pos, _.prefix)) return null;
            return out[0..pos];
        }

        if (_.mode == BraceMode.comma_list) {
            const start = _.list_pos;
            while (_.list_pos < _.inner.len && _.inner[_.list_pos] != ',') {
                _.list_pos += 1;
            }
            const item = _.inner[start.._.list_pos];
            if (_.list_pos < _.inner.len && _.inner[_.list_pos] == ',') {
                _.list_pos += 1;
            } else {
                _.done = true;
            }
            if (!writeSlice(out, &pos, _.prefix)) return null;
            if (!writeSlice(out, &pos, item)) return null;
            if (!writeSlice(out, &pos, _.suffix)) return null;
            return out[0..pos];
        }

        if (_.mode == BraceMode.int_range) {
            const val = _.cur_val;
            const next_val = val + _.step;
            if ((_.step > 0 && next_val > _.end_val) || (_.step < 0 && next_val < _.end_val)) {
                _.done = true;
            } else {
                _.cur_val = next_val;
            }
            if (!writeSlice(out, &pos, _.prefix)) return null;
            if (!writePaddedInt(out, &pos, val, _.pad_width)) return null;
            if (!writeSlice(out, &pos, _.suffix)) return null;
            return out[0..pos];
        }

        // char_range
        const val = _.cur_val;
        const next_val = val + _.step;
        if ((_.step > 0 && next_val > _.end_val) || (_.step < 0 && next_val < _.end_val)) {
            _.done = true;
        } else {
            _.cur_val = next_val;
        }
        if (!writeSlice(out, &pos, _.prefix)) return null;
        if (pos >= out.len) return null;
        out[pos] = @as(u8, val);
        pos += 1;
        if (!writeSlice(out, &pos, _.suffix)) return null;
        return out[0..pos];
    }
}

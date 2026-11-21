// Source: Absorbed from /data/dev/bun/src/cli.zig (Human Duration Parser & Formatter)

pub const NS_PER_US: u64 = 1000;
pub const NS_PER_MS: u64 = 1000000;
pub const NS_PER_SEC: u64 = 1000000000;
pub const NS_PER_MIN: u64 = 60000000000;
pub const NS_PER_HOUR: u64 = 3600000000000;
pub const NS_PER_DAY: u64 = 86400000000000;
pub const NS_PER_WEEK: u64 = 604800000000000;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn unitMultiplierNs(unit: []const u8): u64? {
    if (eqlBytes(unit, "ns")) { return 1; }
    if (eqlBytes(unit, "us") || eqlBytes(unit, "\xC2\xB5s") || eqlBytes(unit, "\xCE\xBCs")) {
        return NS_PER_US;
    }
    if (eqlBytes(unit, "ms")) { return NS_PER_MS; }
    if (eqlBytes(unit, "s")) { return NS_PER_SEC; }
    if (eqlBytes(unit, "m")) { return NS_PER_MIN; }
    if (eqlBytes(unit, "h")) { return NS_PER_HOUR; }
    if (eqlBytes(unit, "d")) { return NS_PER_DAY; }
    if (eqlBytes(unit, "w")) { return NS_PER_WEEK; }
    return null;
}

/// 解析人类可读时长字符串为纳秒 `u64`（纯整数 `u128` 算术，零浮点误差与溢出防护）
/// 支持 `ns`/`us`/`µs`/`ms`/`s`/`m`/`h`/`d`/`w`、复合片段（如 `"1h30m15s"`, `"2d 4h"`）、小数（如 `"1.5s"`）与 `"0"`
pub fn parseDurationNs(input: []const u8): u64? {
    const s = trimSpace(input);
    if (s.len == 0) { return null; }
    if (eqlBytes(s, "0")) { return 0; }
    if (s[0] == '-') { return null; }

    const max_u64: u128 = 18446744073709551615;
    let total: u128 = 0;
    let i: usize = 0;
    let seg_count: usize = 0;

    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len) { break; }

        if (s[i] == '-') { return null; }
        if (s[i] == '+') { i += 1; }

        let has_digits = false;
        let int_part: u128 = 0;
        while (i < s.len && isDigit(s[i])) {
            has_digits = true;
            const d: u128 = @as(u128, s[i] - '0');
            if (int_part > max_u64) { return null; }
            int_part = int_part * 10 + d;
            i += 1;
        }

        let frac_num: u128 = 0;
        let frac_div: u128 = 1;
        if (i < s.len && s[i] == '.') {
            i += 1;
            let frac_digits: usize = 0;
            while (i < s.len && isDigit(s[i])) {
                has_digits = true;
                if (frac_digits < 9) {
                    frac_num = frac_num * 10 + @as(u128, s[i] - '0');
                    frac_div *= 10;
                    frac_digits += 1;
                }
                i += 1;
            }
        }

        if (!has_digits) { return null; }

        // 解析紧随数字后的单位（允许数字与单位间无空格）
        const unit_start = i;
        while (i < s.len && !isDigit(s[i]) && !isSpace(s[i]) && s[i] != '.' && s[i] != '+' && s[i] != '-') {
            i += 1;
        }
        const unit_str = s[unit_start..i];
        if (unit_str.len == 0) {
            return null;
        }

        if (mult := unitMultiplierNs(unit_str)) {
            const mult128: u128 = @as(u128, mult);
            if (int_part > (max_u64 / mult128)) {
                return null;
            }
            const int_ns = int_part * mult128;
            const frac_ns = (frac_num * mult128) / frac_div;
            if (total + int_ns + frac_ns > max_u64) {
                return null;
            }
            total += int_ns + frac_ns;
            seg_count += 1;
        } else {
            return null;
        }
    }

    if (seg_count == 0) { return null; }
    return @as(u64, @intCast(total));
}

/// 解析人类可读时长字符串为毫秒 `u64`（不足 1ms 向下取整）
pub fn parseDurationMs(input: []const u8): u64? {
    if (ns := parseDurationNs(input)) {
        return ns / NS_PER_MS;
    }
    return null;
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeU64(*_, val: u64) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 规范化复合时长输出（精确无损，可与 `parseDurationNs` 往返）
/// 例：`0` -> `"0s"`、`1500_000_000` -> `"1s500ms"`、`5400_000_000_000` -> `"1h30m"`
pub fn formatDurationNs(ns: u64, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    if (ns == 0) {
        w.writeStr("0s");
        return w.finish();
    }

    let rem = ns;
    if (rem >= NS_PER_WEEK) {
        const v = rem / NS_PER_WEEK;
        rem %= NS_PER_WEEK;
        w.writeU64(v);
        w.writeByte('w');
    }
    if (rem >= NS_PER_DAY) {
        const v = rem / NS_PER_DAY;
        rem %= NS_PER_DAY;
        w.writeU64(v);
        w.writeByte('d');
    }
    if (rem >= NS_PER_HOUR) {
        const v = rem / NS_PER_HOUR;
        rem %= NS_PER_HOUR;
        w.writeU64(v);
        w.writeByte('h');
    }
    if (rem >= NS_PER_MIN) {
        const v = rem / NS_PER_MIN;
        rem %= NS_PER_MIN;
        w.writeU64(v);
        w.writeByte('m');
    }
    if (rem >= NS_PER_SEC) {
        const v = rem / NS_PER_SEC;
        rem %= NS_PER_SEC;
        w.writeU64(v);
        w.writeByte('s');
    }
    if (rem >= NS_PER_MS) {
        const v = rem / NS_PER_MS;
        rem %= NS_PER_MS;
        w.writeU64(v);
        w.writeStr("ms");
    }
    if (rem >= NS_PER_US) {
        const v = rem / NS_PER_US;
        rem %= NS_PER_US;
        w.writeU64(v);
        w.writeStr("us");
    }
    if (rem > 0) {
        w.writeU64(rem);
        w.writeStr("ns");
    }

    return w.finish();
}

fn writeCompactUnit(w: *OutWriter, ns: u64, unit_ns: u64, suffix: []const u8) {
    const whole = ns / unit_ns;
    const rem = ns % unit_ns;
    const hundredths: u64 = @intCast((@as(u128, rem) * 100) / @as(u128, unit_ns));
    w.writeU64(whole);
    if (hundredths > 0) {
        w.writeByte('.');
        w.writeByte(@intCast((hundredths / 10) + '0'));
        w.writeByte(@intCast((hundredths % 10) + '0'));
    }
    w.writeStr(suffix);
}

/// 单主单位紧凑输出（如 `"1.50s"` / `"250ms"` / `"42us"` / `"0s"`）
pub fn formatDurationCompact(ns: u64, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    if (ns == 0) {
        w.writeStr("0s");
        return w.finish();
    }
    if (ns < NS_PER_US) {
        w.writeU64(ns);
        w.writeStr("ns");
        return w.finish();
    }
    if (ns < NS_PER_MS) {
        writeCompactUnit(*w, ns, NS_PER_US, "us");
        return w.finish();
    }
    if (ns < NS_PER_SEC) {
        writeCompactUnit(*w, ns, NS_PER_MS, "ms");
        return w.finish();
    }
    if (ns < NS_PER_MIN) {
        writeCompactUnit(*w, ns, NS_PER_SEC, "s");
        return w.finish();
    }
    if (ns < NS_PER_HOUR) {
        writeCompactUnit(*w, ns, NS_PER_MIN, "m");
        return w.finish();
    }
    if (ns < NS_PER_DAY) {
        writeCompactUnit(*w, ns, NS_PER_HOUR, "h");
        return w.finish();
    }
    writeCompactUnit(*w, ns, NS_PER_DAY, "d");
    return w.finish();
}

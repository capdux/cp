// Source: Absorbed from /data/dev/bun/src/cli.zig (CLI Progress Bar & Spinner Renderer)
// Pure-CP zero-allocation deterministic CLI progress bar, percentage, rate/ETA estimator, and spinner renderer.

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
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

fn writeU64(out: []u8, pos: *usize, val: u64): bool {
    if (val == 0) return writeByte(out, pos, '0');
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = @as(u8, '0') + @as(u8, v % 10);
        v /= 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

export enum SpinnerStyle {
    ascii,   // | / - \
    dots,    // ⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏
    line,    // - \ | /
}

/// Return the spinner frame string for a given `tick` counter (wraps automatically).
export fn spinnerFrame(style: SpinnerStyle, tick: usize): []const u8 {
    if (style == SpinnerStyle.ascii) {
        const idx = tick % 4;
        if (idx == 0) return "|";
        if (idx == 1) return "/";
        if (idx == 2) return "-";
        return "\\";
    }
    if (style == SpinnerStyle.line) {
        const idx = tick % 4;
        if (idx == 0) return "-";
        if (idx == 1) return "\\";
        if (idx == 2) return "|";
        return "/";
    }
    const idx = tick % 10;
    if (idx == 0) return "⠋";
    if (idx == 1) return "⠙";
    if (idx == 2) return "⠹";
    if (idx == 3) return "⠸";
    if (idx == 4) return "⠼";
    if (idx == 5) return "⠴";
    if (idx == 6) return "⠦";
    if (idx == 7) return "⠧";
    if (idx == 8) return "⠇";
    return "⠏";
}

export struct ProgressBar {
    current: u64,
    total: u64,
    width: usize,
    fill_char: u8,
    empty_char: u8,

    pub fn init(total: u64, width: usize): ProgressBar {
        return ProgressBar {
            current: 0,
            total: total,
            width: if (width == 0) @as(usize, 20) else width,
            fill_char: '#',
            empty_char: '-',
        };
    }

    pub fn set(*_, value: u64) {
        _.current = if (value > _.total) _.total else value;
    }

    pub fn inc(*_, delta: u64) {
        if (delta > _.total - _.current) {
            _.current = _.total;
        } else {
            _.current += delta;
        }
    }

    /// Completion percentage in `0..=100` (pure integer, returns 100 when `total == 0`).
    pub fn percent(): u8 {
        if (_.total == 0) return 100;
        const pct: u64 = @as(u64, (@as(u128, _.current) * 100) / @as(u128, _.total));
        if (pct > 100) return 100;
        return @as(u8, pct);
    }

    pub fn isComplete(): bool {
        return _.current >= _.total;
    }

    /// Estimate remaining milliseconds given `elapsed_ms`. Returns null if `current == 0` or `elapsed_ms == 0`.
    pub fn etaMs(elapsed_ms: u64): u64? {
        if (_.current >= _.total) return 0;
        if (_.current == 0 || elapsed_ms == 0) return null;
        const rem = _.total - _.current;
        const eta: u128 = (@as(u128, rem) * @as(u128, elapsed_ms)) / @as(u128, _.current);
        if (eta > 18446744073709551615) return null;
        return @as(u64, eta);
    }

    /// Render `[#####-----] 50% (50/100)` into `out`.
    pub fn render(out: []u8): ([]const u8)? {
        let pos: usize = 0;
        if (!writeByte(out, &pos, '[')) return null;

        let filled: usize = _.width;
        if (_.total > 0) {
            filled = @as(usize, (@as(u128, _.current) * @as(u128, _.width)) / @as(u128, _.total));
            if (filled > _.width) filled = _.width;
        }
        let i: usize = 0;
        while (i < filled) {
            if (!writeByte(out, &pos, _.fill_char)) return null;
            i += 1;
        }
        while (i < _.width) {
            if (!writeByte(out, &pos, _.empty_char)) return null;
            i += 1;
        }

        if (!writeSlice(out, &pos, "] ")) return null;
        if (!writeU64(out, &pos, @as(u64, _.percent()))) return null;
        if (!writeSlice(out, &pos, "% (")) return null;
        if (!writeU64(out, &pos, _.current)) return null;
        if (!writeByte(out, &pos, '/')) return null;
        if (!writeU64(out, &pos, _.total)) return null;
        if (!writeByte(out, &pos, ')')) return null;
        return out[0..pos];
    }
}

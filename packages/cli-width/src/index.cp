// @cap/cli-width — Zero-allocation terminal column width resolver in pure CP
// Clean-room ISC implementation inspired by npm `cli-width`.

/// Options for `resolveCliWidth` (0 for `min_width` or `max_width` disables that bound).
pub struct CliWidthOptions {
    default_width: u32,
    min_width: u32,
    max_width: u32,
}

fn cliwIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// Default terminal width in columns (80).
pub fn defaultCliWidth(): u32 {
    return 80;
}

/// Parse a positive integer from a `COLUMNS` environment variable string.
/// Returns `null` if empty, non-numeric, zero, or overflowing.
pub fn parseColumnsEnv(env_val: string): u32? {
    let start: usize = 0;
    while (start < env_val.len && cliwIsSpace(env_val[start])) : (start += 1) {}
    let end: usize = env_val.len;
    while (end > start && cliwIsSpace(env_val[end - 1])) : (end -= 1) {}
    if (start >= end) {
        return null;
    }

    let acc: u64 = 0;
    let i: usize = start;
    while (i < end) : (i += 1) {
        const c: u8 = env_val[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 0xFFFF_FFFF) {
            return null;
        }
    }
    if (acc == 0) {
        return null;
    }
    return acc as u32;
}

/// Resolve terminal width with priority: `tty_cols > 0` > valid `COLUMNS` env > `default_width` (fallback 80),
/// then clamp to `[min_width, max_width]` when those bounds are non-zero.
pub fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32 {
    let w: u32 = 0;
    if (tty_cols > 0) {
        w = tty_cols;
    } else if (parsed := parseColumnsEnv(env_columns)) {
        w = parsed;
    } else if (opts.default_width > 0) {
        w = opts.default_width;
    } else {
        w = defaultCliWidth();
    }

    if (opts.min_width > 0 && w < opts.min_width) {
        w = opts.min_width;
    }
    if (opts.max_width > 0 && w > opts.max_width) {
        w = opts.max_width;
    }
    return w;
}

// @cap/cli-width — Zero-allocation terminal column width resolver in pure CP
// Clean-room ISC implementation inspired by npm `cli-width`.

import { isSpace, startsWith } from "cap:ascii";

/// Options for `resolveCliWidth` (0 for `min_width` or `max_width` disables that bound).
pub struct CliWidthOptions {
    pub default_width: u32,
    pub min_width: u32,
    pub max_width: u32,

    pub static fn defaults(): CliWidthOptions {
        return CliWidthOptions {
            default_width: 80,
            min_width: 0,
            max_width: 0,
        };
    }
}

/// Terminal stream descriptor matching Node `output` / `tty` stream dimensions.
pub struct TtyStreamInfo {
    pub is_tty: bool,
    pub columns: u32,
    pub rows: u32,

    pub static fn none(): TtyStreamInfo {
        return TtyStreamInfo {
            is_tty: false,
            columns: 0,
            rows: 0,
        };
    }

    pub static fn tty(columns: u32, rows: u32): TtyStreamInfo {
        return TtyStreamInfo {
            is_tty: true,
            columns: columns,
            rows: rows,
        };
    }
}

/// Full Node `cli-width` configuration supporting `defaultWidth`, `output`, `tty`,
/// `COLUMNS` environment value, and optional `[min_width, max_width]` clamping.
pub struct CliWidthConfig {
    pub default_width: u32,
    pub output: TtyStreamInfo,
    pub tty: TtyStreamInfo,
    pub env_columns: string,
    pub min_width: u32,
    pub max_width: u32,

    pub static fn defaults(): CliWidthConfig {
        return CliWidthConfig {
            default_width: 80,
            output: TtyStreamInfo.none(),
            tty: TtyStreamInfo.none(),
            env_columns: "",
            min_width: 0,
            max_width: 0,
        };
    }
}

/// Default terminal width in columns (80).
pub fn defaultCliWidth(): u32 {
    return 80;
}

/// Clamp `width` to `[min_width, max_width]`; a zero bound disables clamping on that side.
pub fn clampCliWidth(width: u32, min_width: u32, max_width: u32): u32 {
    let w: u32 = width;
    if (min_width > 0 && w < min_width) {
        w = min_width;
    }
    if (max_width > 0 && w > max_width) {
        w = max_width;
    }
    return w;
}

/// Parse a positive integer from a `COLUMNS` environment variable string.
/// Returns `null` if empty, non-numeric, zero, or overflowing `u32`.
pub fn parseColumnsEnv(env_val: string): u32? {
    let start: usize = 0;
    while (start < env_val.len && isSpace(env_val[start])) : (start += 1) {}
    let end: usize = env_val.len;
    while (end > start && isSpace(env_val[end - 1])) : (end -= 1) {}
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

/// Scan `"KEY=VALUE"` environment entries for `COLUMNS=...` and parse its positive column width.
/// Uses the last valid `COLUMNS=` entry when multiple appear.
pub fn parseColumnsFromEnvList(env_entries: []const string): u32? {
    let found: u32? = null;
    let i: usize = 0;
    while (i < env_entries.len) : (i += 1) {
        const entry = env_entries[i];
        if (startsWith(entry, "COLUMNS=")) {
            if (parsed := parseColumnsEnv(entry[8..entry.len])) {
                found = parsed;
            }
        }
    }
    return found;
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

    return clampCliWidth(w, opts.min_width, opts.max_width);
}

/// Full Node `cli-width` resolver following upstream precedence:
/// 1. `cfg.output.columns` (when `output.is_tty` and `columns > 0`)
/// 2. `cfg.tty.columns` (when `tty.is_tty` and `columns > 0`)
/// 3. Valid `COLUMNS` environment string (`cfg.env_columns`)
/// 4. `cfg.default_width` (fallback `80` if `0`)
/// Finally clamped to `[cfg.min_width, cfg.max_width]` when bounds are non-zero.
pub fn cliWidth(cfg: CliWidthConfig): u32 {
    let w: u32 = 0;
    if (cfg.output.is_tty && cfg.output.columns > 0) {
        w = cfg.output.columns;
    } else if (cfg.tty.is_tty && cfg.tty.columns > 0) {
        w = cfg.tty.columns;
    } else if (parsed := parseColumnsEnv(cfg.env_columns)) {
        w = parsed;
    } else if (cfg.default_width > 0) {
        w = cfg.default_width;
    } else {
        w = defaultCliWidth();
    }

    return clampCliWidth(w, cfg.min_width, cfg.max_width);
}

export {
    CliWidthOptions,
    TtyStreamInfo,
    CliWidthConfig,
    defaultCliWidth,
    clampCliWidth,
    parseColumnsEnv,
    parseColumnsFromEnvList,
    resolveCliWidth,
    cliWidth,
};

// @cap/supports-color — Zero-allocation terminal color capability detection in pure CP
// Clean-room MIT implementation inspired by npm `supports-color` v9.
// Reuses @cap/has-flag for CLI flag parsing (`--no-color`, `--color=16m`, `--color=256`, etc.).

import { hasFlag } from "../../has-flag/src/index.cp";

pub const COLOR_LEVEL_NONE: u8 = 0;
pub const COLOR_LEVEL_BASIC: u8 = 1;
pub const COLOR_LEVEL_256: u8 = 2;
pub const COLOR_LEVEL_16M: u8 = 3;

pub struct ColorSupport {
    pub level: u8,
    pub has_basic: bool,
    pub has_256: bool,
    pub has_16m: bool,
}

pub struct TerminalEnv {
    pub is_tty: bool,
    pub force_color: string,
    pub no_color: string,
    pub term: string,
    pub colorterm: string,
    pub term_program: string,
    pub ci: string,
    pub ci_name: string,
    pub is_windows: bool,
    pub win_build: u32,

    pub static fn empty(): TerminalEnv {
        return TerminalEnv {
            is_tty: false,
            force_color: "",
            no_color: "",
            term: "",
            colorterm: "",
            term_program: "",
            ci: "",
            ci_name: "",
            is_windows: false,
            win_build: 0,
        };
    }

    pub static fn tty(): TerminalEnv {
        return TerminalEnv {
            is_tty: true,
            force_color: "",
            no_color: "",
            term: "",
            colorterm: "",
            term_program: "",
            ci: "",
            ci_name: "",
            is_windows: false,
            win_build: 0,
        };
    }
}

fn scToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn scEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (scToLower(a[i]) != scToLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn scContainsIgnoreCase(haystack: string, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    const max_start: usize = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= max_start) {
        let matched: bool = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (scToLower(haystack[i + j]) != scToLower(needle[j])) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Convert a numeric color level (`0..=3`) into a `ColorSupport` struct with boolean flags.
pub fn translateLevel(level: u8): ColorSupport {
    let lv: u8 = level;
    if (lv > COLOR_LEVEL_16M) {
        lv = COLOR_LEVEL_16M;
    }
    return ColorSupport {
        level: lv,
        has_basic: lv >= COLOR_LEVEL_BASIC,
        has_256: lv >= COLOR_LEVEL_256,
        has_16m: lv >= COLOR_LEVEL_16M,
    };
}

/// Check CLI color override flags via `@cap/has-flag`. Returns `0..=3` if overridden, or `-1` if none.
fn scDetectCliOverride(argv: []const string): i8 {
    if (argv.len == 0) {
        return -1;
    }
    if (hasFlag("no-color", argv)
        || hasFlag("no-colors", argv)
        || hasFlag("color=false", argv)
        || hasFlag("color=never", argv))
    {
        return 0;
    }
    if (hasFlag("color=16m", argv)
        || hasFlag("color=full", argv)
        || hasFlag("color=truecolor", argv))
    {
        return 3;
    }
    if (hasFlag("color=256", argv)) {
        return 2;
    }
    if (hasFlag("color=true", argv)
        || hasFlag("color=always", argv)
        || hasFlag("color", argv)
        || hasFlag("colors", argv))
    {
        return 1;
    }
    return -1;
}

/// Parse `FORCE_COLOR` environment value. Returns `0..=3` if set, or `-1` if unset (empty).
fn scParseForceColor(val: string): i8 {
    if (val.len == 0) {
        return -1;
    }
    if (scEqIgnoreCase(val, "0") || scEqIgnoreCase(val, "false")) {
        return 0;
    }
    if (scEqIgnoreCase(val, "1") || scEqIgnoreCase(val, "true")) {
        return 1;
    }
    if (scEqIgnoreCase(val, "2")) {
        return 2;
    }
    if (scEqIgnoreCase(val, "3")) {
        return 3;
    }
    return 1;
}

/// Detect supported terminal color level (`0` = none, `1` = 16 colors, `2` = 256 colors, `3` = 16M Truecolor)
/// strictly following `supports-color` v9 precedence rules.
pub fn detectColorLevel(env: TerminalEnv, argv: []const string): u8 {
    // 1. CLI flags (highest priority, delegated to @cap/has-flag)
    const cli_lv: i8 = scDetectCliOverride(argv);
    if (cli_lv >= 0) {
        return cli_lv as u8;
    }

    // 2. FORCE_COLOR environment variable
    const force_lv: i8 = scParseForceColor(env.force_color);
    if (force_lv >= 0) {
        return force_lv as u8;
    }

    // 3. NO_COLOR disables color (per no-color.org)
    if (env.no_color.len > 0) {
        return COLOR_LEVEL_NONE;
    }

    // 4. Non-TTY streams default to no color
    if (!env.is_tty) {
        return COLOR_LEVEL_NONE;
    }

    // 5. Dumb terminal disables color
    if (scEqIgnoreCase(env.term, "dumb")) {
        return COLOR_LEVEL_NONE;
    }

    // 6. COLORTERM=truecolor / 24bit -> 16M
    if (scEqIgnoreCase(env.colorterm, "truecolor") || scEqIgnoreCase(env.colorterm, "24bit")) {
        return COLOR_LEVEL_16M;
    }

    // 7. Platform / CI / TERM_PROGRAM / TERM heuristics
    if (env.is_windows) {
        if (env.win_build >= 14931) {
            return COLOR_LEVEL_16M;
        }
        if (env.win_build >= 10586) {
            return COLOR_LEVEL_256;
        }
        return COLOR_LEVEL_BASIC;
    }

    if (env.ci.len > 0) {
        if (scContainsIgnoreCase(env.ci_name, "github")) {
            return COLOR_LEVEL_16M;
        }
        if (scContainsIgnoreCase(env.ci_name, "gitlab")
            || scContainsIgnoreCase(env.ci_name, "circle")
            || scContainsIgnoreCase(env.ci_name, "travis")
            || scContainsIgnoreCase(env.ci_name, "appveyor")
            || scContainsIgnoreCase(env.ci_name, "buildkite")
            || scContainsIgnoreCase(env.ci_name, "drone"))
        {
            return COLOR_LEVEL_BASIC;
        }
    }

    if (scEqIgnoreCase(env.term_program, "iTerm.app")) {
        return COLOR_LEVEL_16M;
    }
    if (scEqIgnoreCase(env.term_program, "Apple_Terminal")) {
        return COLOR_LEVEL_256;
    }

    if (scContainsIgnoreCase(env.term, "256color")) {
        return COLOR_LEVEL_256;
    }
    if (scContainsIgnoreCase(env.term, "xterm")
        || scContainsIgnoreCase(env.term, "screen")
        || scContainsIgnoreCase(env.term, "vt100")
        || scContainsIgnoreCase(env.term, "vt220")
        || scContainsIgnoreCase(env.term, "rxvt")
        || scContainsIgnoreCase(env.term, "color")
        || scContainsIgnoreCase(env.term, "ansi")
        || scContainsIgnoreCase(env.term, "cygwin")
        || scContainsIgnoreCase(env.term, "linux"))
    {
        return COLOR_LEVEL_BASIC;
    }

    return COLOR_LEVEL_NONE;
}

/// Detect terminal color support and return a structured `ColorSupport` result.
pub fn detectColorSupport(env: TerminalEnv, argv: []const string): ColorSupport {
    return translateLevel(detectColorLevel(env, argv));
}

export {
    COLOR_LEVEL_NONE,
    COLOR_LEVEL_BASIC,
    COLOR_LEVEL_256,
    COLOR_LEVEL_16M,
    ColorSupport,
    TerminalEnv,
    translateLevel,
    detectColorLevel,
    detectColorSupport,
};

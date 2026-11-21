// Source: Absorbed from /data/dev/bun/src/output.zig (Terminal ANSI Color Capability Detector)
// Pure-CP deterministic terminal color level detection (NO_COLOR, FORCE_COLOR, CLICOLOR, COLORTERM, TERM, CI).

fn toLowerAscii(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn eqIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn containsIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (eqIgnoreCase(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

export enum ColorLevel {
    none,
    basic16,
    ansi256,
    truecolor,
}

export struct ColorEnv {
    is_tty: bool,
    no_color: ([]const u8)?,
    force_color: ([]const u8)?,
    clicolor: ([]const u8)?,
    clicolor_force: ([]const u8)?,
    colorterm: ([]const u8)?,
    term: ([]const u8)?,
    term_program: ([]const u8)?,
    wt_session: ([]const u8)?,
    ci: ([]const u8)?,

    pub fn empty(is_tty: bool): ColorEnv {
        return ColorEnv {
            is_tty: is_tty,
            no_color: null,
            force_color: null,
            clicolor: null,
            clicolor_force: null,
            colorterm: null,
            term: null,
            term_program: null,
            wt_session: null,
            ci: null,
        };
    }
}

/// Parse `FORCE_COLOR` value ("0"/"false" -> none, "1"/"true"/"" -> basic16, "2" -> ansi256, "3" -> truecolor).
export fn parseForceColor(val: []const u8): ColorLevel? {
    if (val.len == 0) return ColorLevel.basic16;
    if (eqIgnoreCase(val, "0") || eqIgnoreCase(val, "false")) return ColorLevel.none;
    if (eqIgnoreCase(val, "1") || eqIgnoreCase(val, "true")) return ColorLevel.basic16;
    if (eqIgnoreCase(val, "2")) return ColorLevel.ansi256;
    if (eqIgnoreCase(val, "3")) return ColorLevel.truecolor;
    return ColorLevel.basic16;
}

/// Detect terminal color support level following no-color.org, force-color.org, CLICOLOR, and Bun conventions.
export fn detectColorLevel(env: ColorEnv): ColorLevel {
    // 1. FORCE_COLOR takes precedence when explicitly set
    if (fc := env.force_color) {
        if (lvl := parseForceColor(fc)) {
            return lvl;
        }
    }

    // 2. NO_COLOR (per no-color.org: any non-empty value disables color)
    if (nc := env.no_color) {
        if (nc.len > 0) return ColorLevel.none;
    }

    // 3. CLICOLOR_FORCE (non-empty and != "0" forces at least basic16)
    let forced = false;
    if (cf := env.clicolor_force) {
        if (cf.len > 0 && !eqIgnoreCase(cf, "0")) {
            forced = true;
        }
    }

    // 4. CLICOLOR=0 disables color unless CLICOLOR_FORCE is set
    if (!forced) {
        if (cc := env.clicolor) {
            if (eqIgnoreCase(cc, "0")) return ColorLevel.none;
        }
    }

    // 5. TERM=dumb disables color unless forced
    if (t := env.term) {
        if (!forced && eqIgnoreCase(t, "dumb")) return ColorLevel.none;
    }

    // 6. Non-TTY check (unless forced or known modern CI)
    if (!env.is_tty && !forced) {
        if (ci := env.ci) {
            if (ci.len == 0) return ColorLevel.none;
            // Modern CI environments generally support at least basic16 ANSI
            return ColorLevel.basic16;
        }
        return ColorLevel.none;
    }

    // 7. Truecolor detection via COLORTERM / WT_SESSION / iTerm / WezTerm / VSCode
    if (ct := env.colorterm) {
        if (eqIgnoreCase(ct, "truecolor") || eqIgnoreCase(ct, "24bit")) {
            return ColorLevel.truecolor;
        }
    }
    if (wt := env.wt_session) {
        if (wt.len > 0) return ColorLevel.truecolor;
    }
    if (tp := env.term_program) {
        if (eqIgnoreCase(tp, "iTerm.app") || eqIgnoreCase(tp, "WezTerm") ||
            eqIgnoreCase(tp, "vscode") || eqIgnoreCase(tp, "Hyper"))
        {
            return ColorLevel.truecolor;
        }
        if (eqIgnoreCase(tp, "Apple_Terminal")) {
            return ColorLevel.ansi256;
        }
    }

    // 8. TERM-based capability detection
    if (t := env.term) {
        if (containsIgnoreCase(t, "256color") || containsIgnoreCase(t, "256")) {
            return ColorLevel.ansi256;
        }
        if (containsIgnoreCase(t, "direct") || containsIgnoreCase(t, "truecolor") ||
            containsIgnoreCase(t, "kitty") || containsIgnoreCase(t, "alacritty") ||
            containsIgnoreCase(t, "ghostty"))
        {
            return ColorLevel.truecolor;
        }
        if (containsIgnoreCase(t, "color") || containsIgnoreCase(t, "ansi") ||
            containsIgnoreCase(t, "xterm") || containsIgnoreCase(t, "vt100") ||
            containsIgnoreCase(t, "screen") || containsIgnoreCase(t, "tmux") ||
            containsIgnoreCase(t, "rxvt"))
        {
            return ColorLevel.basic16;
        }
    }

    if (forced || env.is_tty) return ColorLevel.basic16;
    return ColorLevel.none;
}

export fn supportsColor(env: ColorEnv): bool {
    return detectColorLevel(env) != ColorLevel.none;
}

export fn supportsTruecolor(env: ColorEnv): bool {
    return detectColorLevel(env) == ColorLevel.truecolor;
}

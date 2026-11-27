// @cap/is-unicode-supported — Zero-allocation terminal Unicode support detector for CP
// Clean-room MIT implementation with parity to npm `is-unicode-supported` v2.

/// Terminal and platform environment snapshot used to determine Unicode support.
pub struct EnvInfo {
    is_windows: bool,
    ci: bool,
    wt_session: bool,
    terminus_sublime: bool,
    conemu_task: string,
    term_program: string,
    term: string,
    terminal_emulator: string,

    /// Default POSIX environment (`is_windows = false`, `TERM = "xterm-256color"`).
    static fn posix(): EnvInfo {
        return EnvInfo {
            is_windows: false,
            ci: false,
            wt_session: false,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "",
            term: "xterm-256color",
            terminal_emulator: "",
        };
    }

    /// POSIX environment with an explicit `TERM` value.
    static fn posixWithTerm(term: string): EnvInfo {
        return EnvInfo {
            is_windows: false,
            ci: false,
            wt_session: false,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "",
            term: term,
            terminal_emulator: "",
        };
    }

    /// Linux kernel virtual console (`is_windows = false`, `TERM = "linux"`).
    static fn linuxConsole(): EnvInfo {
        return EnvInfo {
            is_windows: false,
            ci: false,
            wt_session: false,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "",
            term: "linux",
            terminal_emulator: "",
        };
    }

    /// Bare legacy Windows console environment (`is_windows = true`, all flags unset).
    static fn windows(): EnvInfo {
        return EnvInfo {
            is_windows: true,
            ci: false,
            wt_session: false,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "",
            term: "",
            terminal_emulator: "",
        };
    }

    /// Windows Terminal environment (`is_windows = true`, `WT_SESSION` present).
    static fn windowsTerminal(): EnvInfo {
        return EnvInfo {
            is_windows: true,
            ci: false,
            wt_session: true,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "",
            term: "",
            terminal_emulator: "",
        };
    }

    /// VS Code integrated terminal on Windows (`is_windows = true`, `TERM_PROGRAM = "vscode"`).
    static fn vscodeWindows(): EnvInfo {
        return EnvInfo {
            is_windows: true,
            ci: false,
            wt_session: false,
            terminus_sublime: false,
            conemu_task: "",
            term_program: "vscode",
            term: "",
            terminal_emulator: "",
        };
    }
}

fn iusStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

/// Check whether a `TERM` value matches a known Unicode-capable terminal on Windows.
pub fn isUnicodeSupportedTerm(term: string): bool {
    return iusStrEq(term, "xterm-256color")
        || iusStrEq(term, "alacritty")
        || iusStrEq(term, "rxvt-unicode")
        || iusStrEq(term, "rxvt-unicode-256color");
}

/// Check whether a `TERM_PROGRAM` value matches a known Unicode-capable terminal on Windows.
pub fn isUnicodeSupportedTermProgram(term_program: string): bool {
    return iusStrEq(term_program, "Terminus-Sublime")
        || iusStrEq(term_program, "vscode");
}

/// Detect whether the terminal described by `env` supports Unicode output.
/// Matches `npm:is-unicode-supported` v2 semantics without OS or heap dependencies.
pub fn isUnicodeSupported(env: EnvInfo): bool {
    if (!env.is_windows) {
        return !iusStrEq(env.term, "linux");
    }

    return env.ci
        || env.wt_session
        || env.terminus_sublime
        || iusStrEq(env.conemu_task, "{cmd::Cmder}")
        || isUnicodeSupportedTermProgram(env.term_program)
        || isUnicodeSupportedTerm(env.term)
        || iusStrEq(env.terminal_emulator, "JetBrains-JediTerm");
}

/// Lightweight check using only platform flag and `TERM` string.
pub fn isUnicodeSupportedPlatform(is_windows: bool, term: string): bool {
    if (!is_windows) {
        return !iusStrEq(term, "linux");
    }
    return isUnicodeSupportedTerm(term);
}

/// Select `unicode_val` when the environment supports Unicode, otherwise `ascii_val`.
pub fn selectFallback(env: EnvInfo, unicode_val: string, ascii_val: string): string {
    if (isUnicodeSupported(env)) {
        return unicode_val;
    }
    return ascii_val;
}

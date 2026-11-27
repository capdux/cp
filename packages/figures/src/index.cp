// @cap/figures — Unicode terminal symbols with automatic fallbacks
// Inspired by npm:figures (https://github.com/sindresorhus/figures)
// Composes `@cap/is-unicode-supported`.

import { EnvInfo, isUnicodeSupported } from "../../is-unicode-supported/src/index.cp";

pub enum FigureKind {
    Tick,
    Cross,
    Star,
    Square,
    Circle,
    Info,
    Warning,
    Pointer,
    ArrowUp,
    ArrowDown,
    ArrowLeft,
    ArrowRight,
    Ellipsis,
    Bullet,
    Heart,
    RadioOn,
    RadioOff,
    CheckboxOn,
    CheckboxOff,
    Line,
}

fn figStartsAt(input: string, pos: usize, needle: string): bool {
    if (pos + needle.len > input.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) : (i += 1) {
        if (input[pos + i] != needle[i]) {
            return false;
        }
    }
    return true;
}

fn figWriteStr(out: []u8, pos: usize, s: string): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

/// Returns the Unicode or fallback symbol for `kind` depending on `unicode_ok`.
pub fn figureFor(kind: FigureKind, unicode_ok: bool): string {
    if (kind == FigureKind.Tick) {
        return if (unicode_ok) "✔" else "√";
    }
    if (kind == FigureKind.Cross) {
        return if (unicode_ok) "✖" else "×";
    }
    if (kind == FigureKind.Star) {
        return if (unicode_ok) "★" else "*";
    }
    if (kind == FigureKind.Square) {
        return if (unicode_ok) "▇" else "█";
    }
    if (kind == FigureKind.Circle) {
        return if (unicode_ok) "◯" else "( )";
    }
    if (kind == FigureKind.Info) {
        return if (unicode_ok) "ℹ" else "i";
    }
    if (kind == FigureKind.Warning) {
        return if (unicode_ok) "⚠" else "‼";
    }
    if (kind == FigureKind.Pointer) {
        return if (unicode_ok) "❯" else ">";
    }
    if (kind == FigureKind.ArrowUp) {
        return "↑";
    }
    if (kind == FigureKind.ArrowDown) {
        return "↓";
    }
    if (kind == FigureKind.ArrowLeft) {
        return "←";
    }
    if (kind == FigureKind.ArrowRight) {
        return "→";
    }
    if (kind == FigureKind.Ellipsis) {
        return if (unicode_ok) "…" else "...";
    }
    if (kind == FigureKind.Bullet) {
        return if (unicode_ok) "●" else "*";
    }
    if (kind == FigureKind.Heart) {
        return "♥";
    }
    if (kind == FigureKind.RadioOn) {
        return if (unicode_ok) "◉" else "(*)";
    }
    if (kind == FigureKind.RadioOff) {
        return if (unicode_ok) "◯" else "( )";
    }
    if (kind == FigureKind.CheckboxOn) {
        return if (unicode_ok) "☒" else "[×]";
    }
    if (kind == FigureKind.CheckboxOff) {
        return if (unicode_ok) "☐" else "[ ]";
    }
    return "─";
}

/// Returns the appropriate symbol for `kind` using `@cap/is-unicode-supported`.
pub fn figureForEnv(kind: FigureKind, env: EnvInfo): string {
    return figureFor(kind, isUnicodeSupported(env));
}

/// Replaces Unicode figure symbols in `input` with fallback strings when `unicode_ok` is `false`.
/// When `unicode_ok` is `true`, copies `input` unchanged into `out`.
/// Returns total bytes written into `out`.
pub fn replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize {
    if (unicode_ok) {
        return figWriteStr(out, 0, input);
    }

    const mains: [13]string = [
        "✔", "✖", "★", "▇", "◯", "ℹ", "⚠", "❯", "…", "●", "◉", "☒", "☐",
    ];
    const fallbacks: [13]string = [
        "√", "×", "*", "█", "( )", "i", "‼", ">", "...", "*", "(*)", "[×]", "[ ]",
    ];

    let i: usize = 0;
    let pos: usize = 0;
    while (i < input.len) {
        let matched: bool = false;
        let k: usize = 0;
        while (k < 13) : (k += 1) {
            const sym: string = mains[k];
            if (figStartsAt(input, i, sym)) {
                pos = figWriteStr(out, pos, fallbacks[k]);
                i += sym.len;
                matched = true;
                break;
            }
        }
        if (!matched) {
            if (pos < out.len) {
                out[pos] = input[i];
                pos += 1;
            }
            i += 1;
        }
    }
    return pos;
}

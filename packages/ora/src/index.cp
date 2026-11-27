// @cap/ora-spinner — Zero-heap CLI terminal spinner animation & status line renderer.
// Clean-room MIT CP implementation modeled on npm `ora` v8.
// Reuses `cursorHide`, `cursorShow`, `eraseLine`, `cursorLeft` from `@cap/ansi-escapes`.

import {
    cursorHide,
    cursorShow,
    eraseLine,
    cursorLeft,
} from "../../ansi-escapes/src/index.cp";

pub const SPINNER_DOTS: u8 = 0;
pub const SPINNER_LINE: u8 = 1;
pub const SPINNER_ARC: u8 = 2;
pub const SPINNER_BOUNCE: u8 = 3;
pub const SPINNER_ARROW: u8 = 4;

fn oraWriteSlice(out: []u8, pos: usize, s: string): usize {
    let w = pos;
    let i: usize = 0;
    while (i < s.len && w < out.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w;
}

fn oraFrameCount(style: u8): usize {
    if (style == SPINNER_LINE) { return 4; }
    if (style == SPINNER_ARC) { return 6; }
    if (style == SPINNER_BOUNCE) { return 4; }
    if (style == SPINNER_ARROW) { return 8; }
    return 10; // SPINNER_DOTS (default)
}

fn oraFrameAt(style: u8, index: usize): string {
    let idx = index % oraFrameCount(style);
    if (style == SPINNER_LINE) {
        if (idx == 0) { return "-"; }
        if (idx == 1) { return "\\"; }
        if (idx == 2) { return "|"; }
        return "/";
    }
    if (style == SPINNER_ARC) {
        if (idx == 0) { return "◜"; }
        if (idx == 1) { return "◠"; }
        if (idx == 2) { return "◝"; }
        if (idx == 3) { return "◞"; }
        if (idx == 4) { return "◡"; }
        return "◟";
    }
    if (style == SPINNER_BOUNCE) {
        if (idx == 0) { return "⠁"; }
        if (idx == 1) { return "⠂"; }
        if (idx == 2) { return "⠄"; }
        return "⠂";
    }
    if (style == SPINNER_ARROW) {
        if (idx == 0) { return "←"; }
        if (idx == 1) { return "↖"; }
        if (idx == 2) { return "↑"; }
        if (idx == 3) { return "↗"; }
        if (idx == 4) { return "→"; }
        if (idx == 5) { return "↘"; }
        if (idx == 6) { return "↓"; }
        return "↙";
    }
    // SPINNER_DOTS (default)
    if (idx == 0) { return "⠋"; }
    if (idx == 1) { return "⠙"; }
    if (idx == 2) { return "⠹"; }
    if (idx == 3) { return "⠸"; }
    if (idx == 4) { return "⠼"; }
    if (idx == 5) { return "⠴"; }
    if (idx == 6) { return "⠦"; }
    if (idx == 7) { return "⠧"; }
    if (idx == 8) { return "⠇"; }
    return "⠏";
}

fn oraRenderStatus(symbol: string, msg: string, fallback: string, out: []u8): usize {
    let text = if (msg.len > 0) { msg } else { fallback };
    let w = oraWriteSlice(out, 0, symbol);
    if (text.len > 0) {
        w = oraWriteSlice(out, w, " ");
        w = oraWriteSlice(out, w, text);
    }
    w = oraWriteSlice(out, w, "\n");
    return w;
}

/// Zero-heap CLI spinner state machine and status line renderer.
pub struct Spinner {
    pub style: u8,
    pub text: string,
    pub frame_index: usize,

    pub static fn new(style: u8, text: string): Spinner {
        return Spinner {
            style: style,
            text: text,
            frame_index: 0,
        };
    }

    pub fn frameCount(): usize {
        return oraFrameCount(_.style);
    }

    pub fn currentFrame(): string {
        return oraFrameAt(_.style, _.frame_index);
    }

    /// Advances the animation frame index (wrapping around) and returns the new frame glyph.
    pub fn tick(*_): string {
        _.frame_index = (_.frame_index + 1) % oraFrameCount(_.style);
        return oraFrameAt(_.style, _.frame_index);
    }

    /// Updates the spinner status label text.
    pub fn setText(*_, text: string) {
        _.text = text;
    }

    /// Renders the current spinner frame + text into `out`.
    /// When `with_ansi` is true, prepends `cursorLeft() + eraseLine()` from `@cap/ansi-escapes`.
    pub fn renderFrame(with_ansi: bool, out: []u8): usize {
        let w: usize = 0;
        if (with_ansi) {
            w = oraWriteSlice(out, w, cursorLeft());
            w = oraWriteSlice(out, w, eraseLine());
        }
        w = oraWriteSlice(out, w, oraFrameAt(_.style, _.frame_index));
        if (_.text.len > 0) {
            w = oraWriteSlice(out, w, " ");
            w = oraWriteSlice(out, w, _.text);
        }
        return w;
    }

    /// Renders a success line (`✔ <msg>\n`) into `out`. Uses `_.text` if `msg` is empty.
    pub fn renderSucceed(msg: string, out: []u8): usize {
        return oraRenderStatus("✔", msg, _.text, out);
    }

    /// Renders a failure line (`✖ <msg>\n`) into `out`. Uses `_.text` if `msg` is empty.
    pub fn renderFail(msg: string, out: []u8): usize {
        return oraRenderStatus("✖", msg, _.text, out);
    }

    /// Renders a warning line (`⚠ <msg>\n`) into `out`. Uses `_.text` if `msg` is empty.
    pub fn renderWarn(msg: string, out: []u8): usize {
        return oraRenderStatus("⚠", msg, _.text, out);
    }

    /// Renders an info line (`ℹ <msg>\n`) into `out`. Uses `_.text` if `msg` is empty.
    pub fn renderInfo(msg: string, out: []u8): usize {
        return oraRenderStatus("ℹ", msg, _.text, out);
    }

    /// Returns the ANSI sequence to hide the terminal cursor (`@cap/ansi-escapes::cursorHide()`).
    pub fn hideCursorSeq(): string {
        return cursorHide();
    }

    /// Returns the ANSI sequence to restore the terminal cursor (`@cap/ansi-escapes::cursorShow()`).
    pub fn showCursorSeq(): string {
        return cursorShow();
    }
}

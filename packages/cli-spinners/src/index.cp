// @cap/cli-spinners — Zero-allocation terminal spinner frame presets & status symbols for CP
// Clean-room MIT implementation inspired by npm `cli-spinners` (35M/wk), `ora` (35M/wk), and `log-symbols` (45M/wk).

import { strEq } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub enum SpinnerKind {
    Dots,
    Dots2,
    Line,
    Pipe,
    Arc,
    Circle,
    SquareCorners,
    Bounce,
    BouncingBar,
    Arrow,
    Clock,
    AsciiDots,
}

pub enum SymbolKind {
    Success,
    Error,
    Warning,
    Info,
}

pub fn intervalMs(kind: SpinnerKind) -> u32 {
    switch (kind) {
        SpinnerKind.Dots => { return 80 as u32; },
        SpinnerKind.Dots2 => { return 80 as u32; },
        SpinnerKind.Line => { return 130 as u32; },
        SpinnerKind.Pipe => { return 100 as u32; },
        SpinnerKind.Arc => { return 100 as u32; },
        SpinnerKind.Circle => { return 120 as u32; },
        SpinnerKind.SquareCorners => { return 180 as u32; },
        SpinnerKind.Bounce => { return 120 as u32; },
        SpinnerKind.BouncingBar => { return 80 as u32; },
        SpinnerKind.Arrow => { return 100 as u32; },
        SpinnerKind.Clock => { return 100 as u32; },
        SpinnerKind.AsciiDots => { return 200 as u32; },
    }
}

pub fn frameCount(kind: SpinnerKind) -> usize {
    switch (kind) {
        SpinnerKind.Dots => { return 10 as usize; },
        SpinnerKind.Dots2 => { return 8 as usize; },
        SpinnerKind.Line => { return 4 as usize; },
        SpinnerKind.Pipe => { return 8 as usize; },
        SpinnerKind.Arc => { return 6 as usize; },
        SpinnerKind.Circle => { return 4 as usize; },
        SpinnerKind.SquareCorners => { return 4 as usize; },
        SpinnerKind.Bounce => { return 4 as usize; },
        SpinnerKind.BouncingBar => { return 8 as usize; },
        SpinnerKind.Arrow => { return 8 as usize; },
        SpinnerKind.Clock => { return 12 as usize; },
        SpinnerKind.AsciiDots => { return 3 as usize; },
    }
}

pub fn frameAt(kind: SpinnerKind, tick: usize) -> string {
    const count = frameCount(kind);
    const idx = tick % count;
    switch (kind) {
        SpinnerKind.Dots => {
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
        },
        SpinnerKind.Dots2 => {
            if (idx == 0) { return "⣾"; }
            if (idx == 1) { return "⣽"; }
            if (idx == 2) { return "⣻"; }
            if (idx == 3) { return "⢿"; }
            if (idx == 4) { return "⡿"; }
            if (idx == 5) { return "⣟"; }
            if (idx == 6) { return "⣯"; }
            return "⣷";
        },
        SpinnerKind.Line => {
            if (idx == 0) { return "-"; }
            if (idx == 1) { return "\\"; }
            if (idx == 2) { return "|"; }
            return "/";
        },
        SpinnerKind.Pipe => {
            if (idx == 0) { return "┤"; }
            if (idx == 1) { return "┘"; }
            if (idx == 2) { return "┴"; }
            if (idx == 3) { return "└"; }
            if (idx == 4) { return "├"; }
            if (idx == 5) { return "┌"; }
            if (idx == 6) { return "┬"; }
            return "┐";
        },
        SpinnerKind.Arc => {
            if (idx == 0) { return "◜"; }
            if (idx == 1) { return "◠"; }
            if (idx == 2) { return "◝"; }
            if (idx == 3) { return "◞"; }
            if (idx == 4) { return "◡"; }
            return "◟";
        },
        SpinnerKind.Circle => {
            if (idx == 0) { return "◐"; }
            if (idx == 1) { return "◓"; }
            if (idx == 2) { return "◑"; }
            return "◒";
        },
        SpinnerKind.SquareCorners => {
            if (idx == 0) { return "◰"; }
            if (idx == 1) { return "◳"; }
            if (idx == 2) { return "◲"; }
            return "◱";
        },
        SpinnerKind.Bounce => {
            if (idx == 0) { return "⠁"; }
            if (idx == 1) { return "⠂"; }
            if (idx == 2) { return "⠄"; }
            return "⠂";
        },
        SpinnerKind.BouncingBar => {
            if (idx == 0) { return "[    ]"; }
            if (idx == 1) { return "[=   ]"; }
            if (idx == 2) { return "[==  ]"; }
            if (idx == 3) { return "[=== ]"; }
            if (idx == 4) { return "[ ===]"; }
            if (idx == 5) { return "[  ==]"; }
            if (idx == 6) { return "[   =]"; }
            return "[    ]";
        },
        SpinnerKind.Arrow => {
            if (idx == 0) { return "←"; }
            if (idx == 1) { return "↖"; }
            if (idx == 2) { return "↑"; }
            if (idx == 3) { return "↗"; }
            if (idx == 4) { return "→"; }
            if (idx == 5) { return "↘"; }
            if (idx == 6) { return "↓"; }
            return "↙";
        },
        SpinnerKind.Clock => {
            if (idx == 0) { return "🕛"; }
            if (idx == 1) { return "🕐"; }
            if (idx == 2) { return "🕑"; }
            if (idx == 3) { return "🕒"; }
            if (idx == 4) { return "🕓"; }
            if (idx == 5) { return "🕔"; }
            if (idx == 6) { return "🕕"; }
            if (idx == 7) { return "🕖"; }
            if (idx == 8) { return "🕗"; }
            if (idx == 9) { return "🕘"; }
            if (idx == 10) { return "🕙"; }
            return "🕚";
        },
        SpinnerKind.AsciiDots => {
            if (idx == 0) { return ".  "; }
            if (idx == 1) { return ".. "; }
            return "...";
        },
    }
}

pub fn frameAtTime(kind: SpinnerKind, elapsed_ms: u64) -> string {
    const iv = intervalMs(kind) as u64;
    const tick = (elapsed_ms / iv) as usize;
    return frameAt(kind, tick);
}

pub fn findSpinnerByName(name: string) -> SpinnerKind? {
    if (strEq(name, "dots") || strEq(name, "Dots")) { return SpinnerKind.Dots; }
    if (strEq(name, "dots2") || strEq(name, "Dots2")) { return SpinnerKind.Dots2; }
    if (strEq(name, "line") || strEq(name, "Line")) { return SpinnerKind.Line; }
    if (strEq(name, "pipe") || strEq(name, "Pipe")) { return SpinnerKind.Pipe; }
    if (strEq(name, "arc") || strEq(name, "Arc")) { return SpinnerKind.Arc; }
    if (strEq(name, "circle") || strEq(name, "Circle")) { return SpinnerKind.Circle; }
    if (strEq(name, "squareCorners") || strEq(name, "square-corners") || strEq(name, "square_corners") || strEq(name, "SquareCorners")) {
        return SpinnerKind.SquareCorners;
    }
    if (strEq(name, "bounce") || strEq(name, "Bounce")) { return SpinnerKind.Bounce; }
    if (strEq(name, "bouncingBar") || strEq(name, "bouncing-bar") || strEq(name, "bouncing_bar") || strEq(name, "BouncingBar")) {
        return SpinnerKind.BouncingBar;
    }
    if (strEq(name, "arrow") || strEq(name, "Arrow")) { return SpinnerKind.Arrow; }
    if (strEq(name, "clock") || strEq(name, "Clock")) { return SpinnerKind.Clock; }
    if (strEq(name, "asciiDots") || strEq(name, "ascii-dots") || strEq(name, "ascii_dots") || strEq(name, "AsciiDots")) {
        return SpinnerKind.AsciiDots;
    }
    return null;
}

pub fn logSymbol(kind: SymbolKind, ascii_only: bool) -> string {
    switch (kind) {
        SymbolKind.Success => {
            return if (ascii_only) { "√" } else { "✔" };
        },
        SymbolKind.Error => {
            return if (ascii_only) { "×" } else { "✖" };
        },
        SymbolKind.Warning => {
            return if (ascii_only) { "!" } else { "⚠" };
        },
        SymbolKind.Info => {
            return if (ascii_only) { "i" } else { "ℹ" };
        },
    }
}

pub fn renderSpinnerLine(kind: SpinnerKind, tick: usize, text: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = writeStr(frameAt(kind, tick), out, pos);
    if (text.len > 0) {
        pos = writeStr(" ", out, pos);
        pos = writeStr(text, out, pos);
    }
    return out[0..pos];
}

pub fn renderStatusLine(sym: SymbolKind, ascii_only: bool, text: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = writeStr(logSymbol(sym, ascii_only), out, pos);
    if (text.len > 0) {
        pos = writeStr(" ", out, pos);
        pos = writeStr(text, out, pos);
    }
    return out[0..pos];
}

export {
    SpinnerKind,
    SymbolKind,
    intervalMs,
    frameCount,
    frameAt,
    frameAtTime,
    findSpinnerByName,
    logSymbol,
    renderSpinnerLine,
    renderStatusLine,
};

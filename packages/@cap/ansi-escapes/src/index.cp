// @cap/ansi-escapes — Zero-allocation ANSI/VT100 terminal control & OSC-8 hyperlink engine for CP
// Clean-room MIT implementation inspired by npm `ansi-escapes` (~350M/wk).
//
// Features:
// - Zero-allocation static escape constants for fixed cursor/erase/scroll/bell sequences.
// - Parameterized cursor positioning, relative movement, multi-line `eraseLines`, OSC-8 hyperlinks,
//   and OSC-0 window titles written directly into caller-provided `out: []u8`.

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn pushUsize(out: []u8, pos: usize, val: usize) -> usize {
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    let v = val;
    if (v == 0) {
        return pushByte(out, pos, '0');
    }
    while (v > 0 && count < 20) {
        digits[count] = (48 as u8) + ((v % 10) as u8);
        count += 1;
        v = v / 10;
    }
    let p = pos;
    let k = count;
    while (k > 0) {
        k -= 1;
        p = pushByte(out, p, digits[k]);
    }
    return p;
}

// ============================================================================
// 1. Cursor Control
// ============================================================================

/// Moves the cursor to 0-based column `x` on the current line (`\x1b[<x+1>G`).
pub fn cursorTo(x: usize, out: []u8) -> string {
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, x + 1);
    pos = pushByte(out, pos, 'G');
    return out[0..pos];
}

/// Moves the cursor to 0-based column `x`, row `y` (`\x1b[<y+1>;<x+1>H`).
pub fn cursorToXY(x: usize, y: usize, out: []u8) -> string {
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, y + 1);
    pos = pushByte(out, pos, ';');
    pos = pushUsize(out, pos, x + 1);
    pos = pushByte(out, pos, 'H');
    return out[0..pos];
}

/// Moves the cursor relative to its current position by `(dx, dy)`.
pub fn cursorMove(dx: i32, dy: i32, out: []u8) -> string {
    let pos: usize = 0;
    if (dx < 0) {
        const steps = (0 - dx) as usize;
        pos = pushStr(out, pos, "\x1b[");
        pos = pushUsize(out, pos, steps);
        pos = pushByte(out, pos, 'D');
    } else if (dx > 0) {
        pos = pushStr(out, pos, "\x1b[");
        pos = pushUsize(out, pos, dx as usize);
        pos = pushByte(out, pos, 'C');
    }
    if (dy < 0) {
        const steps = (0 - dy) as usize;
        pos = pushStr(out, pos, "\x1b[");
        pos = pushUsize(out, pos, steps);
        pos = pushByte(out, pos, 'A');
    } else if (dy > 0) {
        pos = pushStr(out, pos, "\x1b[");
        pos = pushUsize(out, pos, dy as usize);
        pos = pushByte(out, pos, 'B');
    }
    return out[0..pos];
}

/// Moves the cursor up `n` rows (`\x1b[<n>A`). Returns `""` if `n == 0`.
pub fn cursorUp(n: usize, out: []u8) -> string {
    if (n == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, n);
    pos = pushByte(out, pos, 'A');
    return out[0..pos];
}

/// Moves the cursor down `n` rows (`\x1b[<n>B`). Returns `""` if `n == 0`.
pub fn cursorDown(n: usize, out: []u8) -> string {
    if (n == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, n);
    pos = pushByte(out, pos, 'B');
    return out[0..pos];
}

/// Moves the cursor forward (right) `n` columns (`\x1b[<n>C`). Returns `""` if `n == 0`.
pub fn cursorForward(n: usize, out: []u8) -> string {
    if (n == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, n);
    pos = pushByte(out, pos, 'C');
    return out[0..pos];
}

/// Moves the cursor backward (left) `n` columns (`\x1b[<n>D`). Returns `""` if `n == 0`.
pub fn cursorBackward(n: usize, out: []u8) -> string {
    if (n == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b[");
    pos = pushUsize(out, pos, n);
    pos = pushByte(out, pos, 'D');
    return out[0..pos];
}

/// Moves cursor to the left side of the current line (`\x1b[G`).
pub fn cursorLeft() -> string {
    return "\x1b[G";
}

/// Saves the current cursor position (`\x1b7`).
pub fn cursorSave() -> string {
    return "\x1b7";
}

/// Restores the saved cursor position (`\x1b8`).
pub fn cursorRestore() -> string {
    return "\x1b8";
}

/// Hides the terminal cursor (`\x1b[?25l`).
pub fn cursorHide() -> string {
    return "\x1b[?25l";
}

/// Shows the terminal cursor (`\x1b[?25h`).
pub fn cursorShow() -> string {
    return "\x1b[?25h";
}

// ============================================================================
// 2. Erasing & Scrolling
// ============================================================================

/// Erases the entire current line (`\x1b[2K`).
pub fn eraseLine() -> string {
    return "\x1b[2K";
}

/// Erases from the cursor to the end of the current line (`\x1b[K`).
pub fn eraseEndLine() -> string {
    return "\x1b[K";
}

/// Erases from the cursor to the start of the current line (`\x1b[1K`).
pub fn eraseStartLine() -> string {
    return "\x1b[1K";
}

/// Erases from the current line down to the bottom of the screen (`\x1b[J`).
pub fn eraseDown() -> string {
    return "\x1b[J";
}

/// Erases from the current line up to the top of the screen (`\x1b[1J`).
pub fn eraseUp() -> string {
    return "\x1b[1J";
}

/// Erases the visible screen (`\x1b[2J`).
pub fn eraseScreen() -> string {
    return "\x1b[2J";
}

/// Clears the terminal screen and scrollback buffer and moves cursor home (`\x1b[2J\x1b[3J\x1b[H`).
pub fn clearTerminal() -> string {
    return "\x1b[2J\x1b[3J\x1b[H";
}

/// Scrolls the display up by one line (`\x1b[S`).
pub fn scrollUp() -> string {
    return "\x1b[S";
}

/// Scrolls the display down by one line (`\x1b[T`).
pub fn scrollDown() -> string {
    return "\x1b[T";
}

/// Erases `count` lines moving upward from the current line and returns cursor to column 0.
/// Core primitive for flickering-free CLI spinners and progress bars.
pub fn eraseLines(count: usize, out: []u8) -> string {
    if (count == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < count) {
        pos = pushStr(out, pos, "\x1b[2K");
        if (i + 1 < count) {
            pos = pushStr(out, pos, "\x1b[1A");
        }
        i += 1;
    }
    pos = pushStr(out, pos, "\x1b[G");
    return out[0..pos];
}

// ============================================================================
// 3. Modern Terminal Features (OSC-8 Hyperlink, OSC-0 Title, Bell)
// ============================================================================

/// Creates a clickable OSC-8 terminal hyperlink (`\x1b]8;;<url>\x07<text>\x1b]8;;\x07`).
pub fn hyperlink(text: string, url: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b]8;;");
    pos = pushStr(out, pos, url);
    pos = pushByte(out, pos, 0x07);
    pos = pushStr(out, pos, text);
    pos = pushStr(out, pos, "\x1b]8;;");
    pos = pushByte(out, pos, 0x07);
    return out[0..pos];
}

/// Sets the terminal window / tab title using OSC-0 (`\x1b]0;<title>\x07`).
pub fn setWindowTitle(title: string, out: []u8) -> string {
    let pos: usize = 0;
    pos = pushStr(out, pos, "\x1b]0;");
    pos = pushStr(out, pos, title);
    pos = pushByte(out, pos, 0x07);
    return out[0..pos];
}

/// Emits an audible/visual terminal bell (`\x07`).
pub fn beep() -> string {
    return "\x07";
}

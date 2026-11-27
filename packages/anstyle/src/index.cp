// @cap/anstyle — Zero-heap ANSI terminal styling model in pure CP
// MIT License — Clean-room implementation matching Rust `anstyle` semantics

pub const COLOR_NONE: u8 = 0;
pub const COLOR_ANSI16: u8 = 1;
pub const COLOR_ANSI256: u8 = 2;
pub const COLOR_RGB: u8 = 3;

pub const EFFECT_BOLD: u16 = 1;
pub const EFFECT_DIMMED: u16 = 2;
pub const EFFECT_ITALIC: u16 = 4;
pub const EFFECT_UNDERLINE: u16 = 8;
pub const EFFECT_BLINK: u16 = 16;
pub const EFFECT_INVERT: u16 = 32;
pub const EFFECT_HIDDEN: u16 = 64;
pub const EFFECT_STRIKETHROUGH: u16 = 128;

const BYTE_ESC: u8 = 27;

pub struct Color {
    pub kind: u8,
    pub r: u8,
    pub g: u8,
    pub b: u8,

    /// No color (terminal default).
    pub static fn none(): Color {
        return Color { kind: COLOR_NONE, r: 0, g: 0, b: 0 };
    }

    /// Standard 16-color ANSI palette index (0..7 normal, 8..15 bright).
    pub static fn ansi(idx: u8): Color {
        return Color { kind: COLOR_ANSI16, r: idx & 15, g: 0, b: 0 };
    }

    /// 256-color xterm palette index (0..255).
    pub static fn ansi256(idx: u8): Color {
        return Color { kind: COLOR_ANSI256, r: idx, g: 0, b: 0 };
    }

    /// 24-bit TrueColor RGB.
    pub static fn rgb(r: u8, g: u8, b: u8): Color {
        return Color { kind: COLOR_RGB, r: r, g: g, b: b };
    }
}

pub struct Effects {
    pub bits: u16,

    /// Empty effects bitset.
    pub static fn new(): Effects {
        return Effects { bits: 0 };
    }

    /// Return true if no text effects are enabled.
    pub fn isEmpty(): bool {
        return _.bits == 0;
    }
}

fn writeByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeU8Dec(out: []u8, pos: usize, val: u8): usize {
    let p: usize = pos;
    if (val >= 100) {
        p = writeByte(out, p, (val / 100) + 48);
        p = writeByte(out, p, ((val / 10) % 10) + 48);
        p = writeByte(out, p, (val % 10) + 48);
    } else if (val >= 10) {
        p = writeByte(out, p, (val / 10) + 48);
        p = writeByte(out, p, (val % 10) + 48);
    } else {
        p = writeByte(out, p, val + 48);
    }
    return p;
}

fn writeParam(out: []u8, pos: usize, has_prev: bool, val: u8): usize {
    let p: usize = pos;
    if (has_prev) {
        p = writeByte(out, p, 59); // ';'
    }
    return writeU8Dec(out, p, val);
}

pub struct Style {
    pub fg: Color,
    pub bg: Color,
    pub effects: Effects,

    /// Construct an empty (plain) style with no colors or effects.
    pub static fn new(): Style {
        return Style {
            fg: Color.none(),
            bg: Color.none(),
            effects: Effects.new(),
        };
    }

    /// Return true if this style sets neither colors nor effects.
    pub fn isPlain(): bool {
        return _.fg.kind == COLOR_NONE &&
            _.bg.kind == COLOR_NONE &&
            _.effects.bits == 0;
    }

    /// Set foreground color.
    pub fn fgColor(c: Color): Style {
        return Style { fg: c, bg: _.bg, effects: _.effects };
    }

    /// Set background color.
    pub fn bgColor(c: Color): Style {
        return Style { fg: _.fg, bg: c, effects: _.effects };
    }

    /// Enable bold effect (SGR 1).
    pub fn bold(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_BOLD } };
    }

    /// Enable dimmed effect (SGR 2).
    pub fn dimmed(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_DIMMED } };
    }

    /// Enable italic effect (SGR 3).
    pub fn italic(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_ITALIC } };
    }

    /// Enable underline effect (SGR 4).
    pub fn underline(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_UNDERLINE } };
    }

    /// Enable blink effect (SGR 5).
    pub fn blink(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_BLINK } };
    }

    /// Enable reverse/invert effect (SGR 7).
    pub fn invert(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_INVERT } };
    }

    /// Enable hidden effect (SGR 8).
    pub fn hidden(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_HIDDEN } };
    }

    /// Enable strikethrough effect (SGR 9).
    pub fn strikethrough(): Style {
        return Style { fg: _.fg, bg: _.bg, effects: Effects { bits: _.effects.bits | EFFECT_STRIKETHROUGH } };
    }

    /// Render the opening ANSI SGR sequence (`ESC[...m`) into `out`.
    /// Writes 0 bytes for a plain style. Safely truncates if `out` is too small.
    pub fn renderStart(out: []u8): usize {
        if (_.isPlain() || out.len == 0) {
            return 0;
        }
        let p: usize = 0;
        p = writeByte(out, p, BYTE_ESC);
        p = writeByte(out, p, 91); // '['
        let has_prev: bool = false;

        // Effects
        const bits: u16 = _.effects.bits;
        if ((bits & EFFECT_BOLD) != 0) { p = writeParam(out, p, has_prev, 1); has_prev = true; }
        if ((bits & EFFECT_DIMMED) != 0) { p = writeParam(out, p, has_prev, 2); has_prev = true; }
        if ((bits & EFFECT_ITALIC) != 0) { p = writeParam(out, p, has_prev, 3); has_prev = true; }
        if ((bits & EFFECT_UNDERLINE) != 0) { p = writeParam(out, p, has_prev, 4); has_prev = true; }
        if ((bits & EFFECT_BLINK) != 0) { p = writeParam(out, p, has_prev, 5); has_prev = true; }
        if ((bits & EFFECT_INVERT) != 0) { p = writeParam(out, p, has_prev, 7); has_prev = true; }
        if ((bits & EFFECT_HIDDEN) != 0) { p = writeParam(out, p, has_prev, 8); has_prev = true; }
        if ((bits & EFFECT_STRIKETHROUGH) != 0) { p = writeParam(out, p, has_prev, 9); has_prev = true; }

        // Foreground color
        if (_.fg.kind == COLOR_ANSI16) {
            const code: u8 = if (_.fg.r < 8) _.fg.r + 30 else (_.fg.r - 8) + 90;
            p = writeParam(out, p, has_prev, code);
            has_prev = true;
        } else if (_.fg.kind == COLOR_ANSI256) {
            p = writeParam(out, p, has_prev, 38);
            p = writeParam(out, p, true, 5);
            p = writeParam(out, p, true, _.fg.r);
            has_prev = true;
        } else if (_.fg.kind == COLOR_RGB) {
            p = writeParam(out, p, has_prev, 38);
            p = writeParam(out, p, true, 2);
            p = writeParam(out, p, true, _.fg.r);
            p = writeParam(out, p, true, _.fg.g);
            p = writeParam(out, p, true, _.fg.b);
            has_prev = true;
        }

        // Background color
        if (_.bg.kind == COLOR_ANSI16) {
            const code: u8 = if (_.bg.r < 8) _.bg.r + 40 else (_.bg.r - 8) + 100;
            p = writeParam(out, p, has_prev, code);
        } else if (_.bg.kind == COLOR_ANSI256) {
            p = writeParam(out, p, has_prev, 48);
            p = writeParam(out, p, true, 5);
            p = writeParam(out, p, true, _.bg.r);
        } else if (_.bg.kind == COLOR_RGB) {
            p = writeParam(out, p, has_prev, 48);
            p = writeParam(out, p, true, 2);
            p = writeParam(out, p, true, _.bg.r);
            p = writeParam(out, p, true, _.bg.g);
            p = writeParam(out, p, true, _.bg.b);
        }

        p = writeByte(out, p, 109); // 'm'
        return p;
    }

    /// Render the ANSI SGR reset sequence (`ESC[0m`) into `out`.
    /// Writes 0 bytes for a plain style.
    pub fn renderReset(out: []u8): usize {
        if (_.isPlain() || out.len == 0) {
            return 0;
        }
        let p: usize = 0;
        p = writeByte(out, p, BYTE_ESC);
        p = writeByte(out, p, 91);  // '['
        p = writeByte(out, p, 48);  // '0'
        p = writeByte(out, p, 109); // 'm'
        return p;
    }

    /// Wrap `text` with this style's opening SGR and trailing `ESC[0m` reset.
    /// Plain styles pass `text` through unchanged without escape codes.
    pub fn paint(text: string, out: []u8): usize {
        if (out.len == 0) {
            return 0;
        }
        if (_.isPlain()) {
            let p: usize = 0;
            while (p < text.len && p < out.len) {
                out[p] = text[p];
                p += 1;
            }
            return p;
        }
        let p: usize = _.renderStart(out);
        let i: usize = 0;
        while (i < text.len && p < out.len) {
            out[p] = text[i];
            p += 1;
            i += 1;
        }
        if (p < out.len) {
            const r: usize = _.renderReset(out[p..out.len]);
            p += r;
        }
        return p;
    }
}

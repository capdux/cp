// Source: Absorbed from /data/dev/bun/src/output.zig (ANSI SGR Style & Color Codec)
// Pure-CP zero-allocation ANSI SGR style & 16-color / 256-color / 24-bit TrueColor RGB codec.

/// Color representation mode for ANSI SGR foreground/background colors.
pub enum SgrColorKind {
    none,
    ansi16,
    ansi256,
    rgb,
}

/// ANSI SGR color value supporting default, 16-color, 256-color palette, and 24-bit TrueColor RGB.
pub struct SgrColor {
    kind: SgrColorKind;
    code: u8;
    r: u8;
    g: u8;
    b: u8;

    static fn none(): SgrColor {
        return SgrColor { kind: SgrColorKind.none, code: 0, r: 0, g: 0, b: 0 };
    }

    static fn ansi16(code: u8): SgrColor {
        return SgrColor { kind: SgrColorKind.ansi16, code: code, r: 0, g: 0, b: 0 };
    }

    static fn ansi256(idx: u8): SgrColor {
        return SgrColor { kind: SgrColorKind.ansi256, code: idx, r: 0, g: 0, b: 0 };
    }

    static fn rgb(r: u8, g: u8, b: u8): SgrColor {
        return SgrColor { kind: SgrColorKind.rgb, code: 0, r: r, g: g, b: b };
    }

    fn isNone(): bool {
        return _.kind == SgrColorKind.none;
    }

    fn equals(other: SgrColor): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == SgrColorKind.none) return true;
        if (_.kind == SgrColorKind.ansi16 || _.kind == SgrColorKind.ansi256) {
            return _.code == other.code;
        }
        return _.r == other.r && _.g == other.g && _.b == other.b;
    }
}

/// ANSI SGR text attributes and foreground/background colors.
pub struct SgrStyle {
    bold: bool;
    dim: bool;
    italic: bool;
    underline: bool;
    inverse: bool;
    strikethrough: bool;
    fg: SgrColor;
    bg: SgrColor;

    static fn plain(): SgrStyle {
        return SgrStyle {
            bold: false,
            dim: false,
            italic: false,
            underline: false,
            inverse: false,
            strikethrough: false,
            fg: SgrColor.none(),
            bg: SgrColor.none(),
        };
    }

    fn reset(*_) {
        _.bold = false;
        _.dim = false;
        _.italic = false;
        _.underline = false;
        _.inverse = false;
        _.strikethrough = false;
        _.fg = SgrColor.none();
        _.bg = SgrColor.none();
    }

    fn isPlain(): bool {
        return !_.bold &&
            !_.dim &&
            !_.italic &&
            !_.underline &&
            !_.inverse &&
            !_.strikethrough &&
            _.fg.kind == SgrColorKind.none &&
            _.bg.kind == SgrColorKind.none;
    }

    /// Format the opening ANSI SGR sequence (`\x1b[...m`) into `out`.
    /// Returns an empty slice when the style is plain, or `null` if `out` is too small.
    fn formatOpen(out: []u8): ([]const u8)? {
        if (_.isPlain()) return out[0..0];

        let w: usize = 0;
        if (w + 2 > out.len) return null;
        out[w] = 0x1B;
        out[w + 1] = '[';
        w += 2;

        let first = true;
        if (_.bold) {
            w = appendCode(out, w, &first, 1) orelse return null;
        }
        if (_.dim) {
            w = appendCode(out, w, &first, 2) orelse return null;
        }
        if (_.italic) {
            w = appendCode(out, w, &first, 3) orelse return null;
        }
        if (_.underline) {
            w = appendCode(out, w, &first, 4) orelse return null;
        }
        if (_.inverse) {
            w = appendCode(out, w, &first, 7) orelse return null;
        }
        if (_.strikethrough) {
            w = appendCode(out, w, &first, 9) orelse return null;
        }

        w = appendColor(out, w, &first, _.fg, false) orelse return null;
        w = appendColor(out, w, &first, _.bg, true) orelse return null;

        if (w >= out.len) return null;
        out[w] = 'm';
        w += 1;
        return out[0..w];
    }
}

/// Write an unsigned 8-bit integer as decimal ASCII into `out` at `offset`.
fn writeU8Decimal(out: []u8, offset: usize, val: u8): usize? {
    let w = offset;
    if (val >= 100) {
        if (w + 3 > out.len) return null;
        out[w] = ('0' as u8) + (val / 100);
        out[w + 1] = ('0' as u8) + ((val / 10) % 10);
        out[w + 2] = ('0' as u8) + (val % 10);
        return w + 3;
    }
    if (val >= 10) {
        if (w + 2 > out.len) return null;
        out[w] = ('0' as u8) + (val / 10);
        out[w + 1] = ('0' as u8) + (val % 10);
        return w + 2;
    }
    if (w + 1 > out.len) return null;
    out[w] = ('0' as u8) + val;
    return w + 1;
}

/// Append a single SGR numeric code with leading `;` separator when not first.
fn appendCode(out: []u8, offset: usize, first: *bool, code: u8): usize? {
    let w = offset;
    if (!first) {
        if (w >= out.len) return null;
        out[w] = ';';
        w += 1;
    } else {
        first = false;
    }
    return writeU8Decimal(out, w, code);
}

/// Normalize a 16-color code (either 0..15 palette index or raw SGR 30..37/90..97/40..47/100..107)
/// into the appropriate foreground or background SGR code.
fn normalizeAnsi16Code(code: u8, is_bg: bool): u8 {
    if (code <= 7) {
        return if (is_bg) (40 + code) else (30 + code);
    }
    if (code <= 15) {
        return if (is_bg) (100 + (code - 8)) else (90 + (code - 8));
    }
    if (is_bg) {
        if ((code >= 30 && code <= 37) || (code >= 90 && code <= 97)) {
            return code + 10;
        }
        return code;
    }
    if ((code >= 40 && code <= 47) || (code >= 100 && code <= 107)) {
        return code - 10;
    }
    return code;
}

/// Append SGR color codes for `color` (foreground when `is_bg == false`, background when `true`).
fn appendColor(out: []u8, offset: usize, first: *bool, color: SgrColor, is_bg: bool): usize? {
    if (color.kind == SgrColorKind.none) return offset;
    let w = offset;
    if (color.kind == SgrColorKind.ansi16) {
        const sgr_code = normalizeAnsi16Code(color.code, is_bg);
        return appendCode(out, w, first, sgr_code);
    }
    const base_code: u8 = if (is_bg) 48 else 38;
    if (color.kind == SgrColorKind.ansi256) {
        w = appendCode(out, w, first, base_code) orelse return null;
        w = appendCode(out, w, first, 5) orelse return null;
        return appendCode(out, w, first, color.code);
    }
    // SgrColorKind.rgb
    w = appendCode(out, w, first, base_code) orelse return null;
    w = appendCode(out, w, first, 2) orelse return null;
    w = appendCode(out, w, first, color.r) orelse return null;
    w = appendCode(out, w, first, color.g) orelse return null;
    return appendCode(out, w, first, color.b);
}

/// Copy `src` into `dst` starting at `offset`.
fn copyInto(dst: []u8, offset: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Format `text` wrapped with `style` (`<open><text>\x1b[0m`) into `out`.
/// If `style` is plain, copies `text` directly without escape sequences.
/// Returns `null` if `out` is too small.
pub fn formatStyled(text: []const u8, style: SgrStyle, out: []u8): ([]const u8)? {
    if (style.isPlain()) {
        if (text.len > out.len) return null;
        const w = copyInto(out, 0, text);
        return out[0..w];
    }

    const open = style.formatOpen(out) orelse return null;
    const total = open.len + text.len + 4;
    if (total > out.len) return null;

    let w = open.len;
    w = copyInto(out, w, text);
    w = copyInto(out, w, "\x1b[0m");
    return out[0..w];
}

/// Result of reading one `;`-delimited numeric parameter from an SGR body.
struct ParamStep {
    valid: bool;
    has_val: bool;
    val: u32;
    next_pos: usize;
}

/// Read the next `;`-delimited non-negative integer from `body` starting at `pos`.
fn readNextParam(body: []const u8, pos: usize, expect_empty_tail: bool): ParamStep {
    if (pos > body.len) {
        return ParamStep { valid: true, has_val: false, val: 0, next_pos: pos };
    }
    if (pos == body.len) {
        if (expect_empty_tail) {
            return ParamStep { valid: true, has_val: true, val: 0, next_pos: body.len + 1 };
        }
        return ParamStep { valid: true, has_val: false, val: 0, next_pos: body.len + 1 };
    }

    let i = pos;
    let val: u32 = 0;
    while (i < body.len && body[i] != ';') {
        const b = body[i];
        if (b < '0' || b > '9') {
            return ParamStep { valid: false, has_val: false, val: 0, next_pos: i };
        }
        const digit: u32 = (b - '0') as u32;
        if (val > 429496729) {
            return ParamStep { valid: false, has_val: false, val: 0, next_pos: i };
        }
        val = val * 10 + digit;
        i += 1;
    }

    const next_pos = if (i < body.len) (i + 1) else (body.len + 1);
    return ParamStep { valid: true, has_val: true, val: val, next_pos: next_pos };
}

/// Parse a single ANSI SGR sequence (`\x1b[...m`) and incrementally update `style`.
/// Supports:
/// - `0` (or empty `\x1b[m`) reset
/// - `1/2/3/4/7/9` enable attributes, `22/23/24/27/29` disable attributes
/// - `30..37`, `90..97` foreground 16-color; `40..47`, `100..107` background 16-color
/// - `39` default foreground; `49` default background
/// - `38;5;n` / `48;5;n` 256-color palette
/// - `38;2;r;g;b` / `48;2;r;g;b` 24-bit TrueColor RGB
/// Returns `false` (leaving `style` unchanged) if `seq` is malformed or has invalid color params.
pub fn applySgrSequence(seq: []const u8, style: *SgrStyle): bool {
    if (seq.len < 3) return false;
    if (seq[0] != 0x1B || seq[1] != '[' || seq[seq.len - 1] != 'm') return false;

    const body = seq[2..(seq.len - 1)];
    if (body.len == 0) {
        style.reset();
        return true;
    }

    // Work on a local copy so malformed sequences never partially mutate `style`
    let next = SgrStyle {
        bold: style.bold,
        dim: style.dim,
        italic: style.italic,
        underline: style.underline,
        inverse: style.inverse,
        strikethrough: style.strikethrough,
        fg: style.fg,
        bg: style.bg,
    };

    let pos: usize = 0;
    let trailing_semi = false;
    while (pos <= body.len) {
        const step = readNextParam(body, pos, trailing_semi);
        if (!step.valid) return false;
        if (!step.has_val) break;

        trailing_semi = (step.next_pos <= body.len);
        pos = step.next_pos;
        const p = step.val;

        if (p == 0) {
            next.reset();
        } else if (p == 1) {
            next.bold = true;
        } else if (p == 2) {
            next.dim = true;
        } else if (p == 3) {
            next.italic = true;
        } else if (p == 4) {
            next.underline = true;
        } else if (p == 7) {
            next.inverse = true;
        } else if (p == 9) {
            next.strikethrough = true;
        } else if (p == 22) {
            next.bold = false;
            next.dim = false;
        } else if (p == 23) {
            next.italic = false;
        } else if (p == 24) {
            next.underline = false;
        } else if (p == 27) {
            next.inverse = false;
        } else if (p == 29) {
            next.strikethrough = false;
        } else if ((p >= 30 && p <= 37) || (p >= 90 && p <= 97)) {
            next.fg = SgrColor.ansi16(p as u8);
        } else if (p == 39) {
            next.fg = SgrColor.none();
        } else if ((p >= 40 && p <= 47) || (p >= 100 && p <= 107)) {
            next.bg = SgrColor.ansi16(p as u8);
        } else if (p == 49) {
            next.bg = SgrColor.none();
        } else if (p == 38 || p == 48) {
            const is_bg = (p == 48);
            const mode_step = readNextParam(body, pos, trailing_semi);
            if (!mode_step.valid || !mode_step.has_val) return false;
            trailing_semi = (mode_step.next_pos <= body.len);
            pos = mode_step.next_pos;

            if (mode_step.val == 5) {
                const idx_step = readNextParam(body, pos, trailing_semi);
                if (!idx_step.valid || !idx_step.has_val || idx_step.val > 255) return false;
                trailing_semi = (idx_step.next_pos <= body.len);
                pos = idx_step.next_pos;
                const c = SgrColor.ansi256(idx_step.val as u8);
                if (is_bg) {
                    next.bg = c;
                } else {
                    next.fg = c;
                }
            } else if (mode_step.val == 2) {
                const r_step = readNextParam(body, pos, trailing_semi);
                if (!r_step.valid || !r_step.has_val || r_step.val > 255) return false;
                trailing_semi = (r_step.next_pos <= body.len);
                pos = r_step.next_pos;

                const g_step = readNextParam(body, pos, trailing_semi);
                if (!g_step.valid || !g_step.has_val || g_step.val > 255) return false;
                trailing_semi = (g_step.next_pos <= body.len);
                pos = g_step.next_pos;

                const b_step = readNextParam(body, pos, trailing_semi);
                if (!b_step.valid || !b_step.has_val || b_step.val > 255) return false;
                trailing_semi = (b_step.next_pos <= body.len);
                pos = b_step.next_pos;

                const c = SgrColor.rgb(r_step.val as u8, g_step.val as u8, b_step.val as u8);
                if (is_bg) {
                    next.bg = c;
                } else {
                    next.fg = c;
                }
            } else {
                return false;
            }
        }
    }

    style.bold = next.bold;
    style.dim = next.dim;
    style.italic = next.italic;
    style.underline = next.underline;
    style.inverse = next.inverse;
    style.strikethrough = next.strikethrough;
    style.fg = next.fg;
    style.bg = next.bg;
    return true;
}

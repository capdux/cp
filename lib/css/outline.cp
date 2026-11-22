// Source: Absorbed from /data/dev/bun/src/css/properties/outline.cp | Synced: 2026-11-22T18:01:00Z
// ============================================================================
// lib/css/outline.cp — Zero-Alloc CSS UI Level 4 `outline` & `outline-style`
// ============================================================================
// Pure-CP zero-allocation parser, validator and minimal serializer for W3C
// CSS UI `outline-style` (`auto` | `LineStyle`), `outline-width` and order-
// independent `outline` shorthand property.

pub err CssOutlineError {
    EmptyInput,
    InvalidKeyword,
    DuplicateComponent,
    NegativeWidth,
    TooManyTokens,
    BufferTooSmall,
}

fn toLowerAscii(c: u8): u8 {
    return if (c >= 'A' && c <= 'Z') (c + 32) else c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isLengthToken(tok: []const u8): bool {
    if (tok.len == 0) return false;
    const c0 = tok[0];
    if (isDigit(c0)) return true;
    if ((c0 == '+' || c0 == '-' || c0 == '.') && tok.len >= 2 && isDigit(tok[1])) return true;
    if ((c0 == '+' || c0 == '-') && tok.len >= 3 && tok[1] == '.' && isDigit(tok[2])) return true;
    return false;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): CssOutlineError!usize {
    if (pos + src.len > out.len) return CssOutlineError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// W3C CSS UI `outline-style` property (`auto` or any `LineStyle` keyword).
pub enum CssOutlineStyle: u8 {
    none = 0,
    hidden = 1,
    inset = 2,
    groove = 3,
    outset = 4,
    ridge = 5,
    dotted = 6,
    dashed = 7,
    solid = 8,
    double = 9,
    auto = 10,

    static fn default(): CssOutlineStyle {
        return CssOutlineStyle.none;
    }

    static fn parse(raw: []const u8): CssOutlineStyle? {
        if (eqlIgnoreCase(raw, "none")) return CssOutlineStyle.none;
        if (eqlIgnoreCase(raw, "hidden")) return CssOutlineStyle.hidden;
        if (eqlIgnoreCase(raw, "inset")) return CssOutlineStyle.inset;
        if (eqlIgnoreCase(raw, "groove")) return CssOutlineStyle.groove;
        if (eqlIgnoreCase(raw, "outset")) return CssOutlineStyle.outset;
        if (eqlIgnoreCase(raw, "ridge")) return CssOutlineStyle.ridge;
        if (eqlIgnoreCase(raw, "dotted")) return CssOutlineStyle.dotted;
        if (eqlIgnoreCase(raw, "dashed")) return CssOutlineStyle.dashed;
        if (eqlIgnoreCase(raw, "solid")) return CssOutlineStyle.solid;
        if (eqlIgnoreCase(raw, "double")) return CssOutlineStyle.double;
        if (eqlIgnoreCase(raw, "auto")) return CssOutlineStyle.auto;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .none => "none",
            .hidden => "hidden",
            .inset => "inset",
            .groove => "groove",
            .outset => "outset",
            .ridge => "ridge",
            .dotted => "dotted",
            .dashed => "dashed",
            .solid => "solid",
            .double => "double",
            .auto => "auto",
        };
    }

    fn isVisible(): bool {
        return _ != CssOutlineStyle.none && _ != CssOutlineStyle.hidden;
    }

    fn isAuto(): bool {
        return _ == CssOutlineStyle.auto;
    }
}

pub enum CssOutlineWidthKind: u8 {
    thin = 0,
    medium = 1,
    thick = 2,
    length = 3,
}

/// W3C CSS UI `outline-width` value (`thin` | `medium` | `thick` | `<length>`).
pub struct CssOutlineWidth {
    kind: CssOutlineWidthKind = CssOutlineWidthKind.medium,
    raw_length: []const u8 = "",

    static fn default(): CssOutlineWidth {
        return CssOutlineWidth{
            kind: CssOutlineWidthKind.medium,
            raw_length: "",
        };
    }

    static fn tryParse(raw: []const u8): CssOutlineError!(CssOutlineWidth?) {
        if (eqlIgnoreCase(raw, "thin")) {
            return CssOutlineWidth{ kind: CssOutlineWidthKind.thin, raw_length: "" };
        }
        if (eqlIgnoreCase(raw, "medium")) {
            return CssOutlineWidth{ kind: CssOutlineWidthKind.medium, raw_length: "" };
        }
        if (eqlIgnoreCase(raw, "thick")) {
            return CssOutlineWidth{ kind: CssOutlineWidthKind.thick, raw_length: "" };
        }
        if (isLengthToken(raw)) {
            if (raw[0] == '-') return CssOutlineError.NegativeWidth;
            return CssOutlineWidth{ kind: CssOutlineWidthKind.length, raw_length: raw };
        }
        return null;
    }

    fn isDefault(): bool {
        return _.kind == CssOutlineWidthKind.medium;
    }

    fn asStr(): []const u8 {
        return switch (_.kind) {
            .thin => "thin",
            .medium => "medium",
            .thick => "thick",
            .length => _.raw_length,
        };
    }

    fn eql(other: CssOutlineWidth): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == CssOutlineWidthKind.length) {
            return eqlIgnoreCase(_.raw_length, other.raw_length);
        }
        return true;
    }
}

/// W3C CSS UI `outline` shorthand property (`[ <outline-color> || <outline-style> || <outline-width> ]`).
pub struct CssOutline {
    width: CssOutlineWidth = CssOutlineWidth.default(),
    style: CssOutlineStyle = CssOutlineStyle.default(),
    color: []const u8 = "currentcolor",
    has_width: bool = false,
    has_style: bool = false,
    has_color: bool = false,

    static fn default(): CssOutline {
        return CssOutline{};
    }

    /// Parses an order-independent CSS `outline` shorthand declaration value.
    /// Supports functional colors with inner whitespace (e.g. `rgb(0, 128, 255) solid 2px`).
    static fn parse(raw: []const u8): CssOutlineError!CssOutline {
        let out = CssOutline.default();
        let i: usize = 0;
        let tok_count: usize = 0;

        while (i < raw.len) {
            while (i < raw.len && isSpace(raw[i])) i += 1;
            if (i >= raw.len) break;

            const start = i;
            let paren_depth: usize = 0;
            while (i < raw.len) {
                const c = raw[i];
                if (c == '(') {
                    paren_depth += 1;
                } else if (c == ')' && paren_depth > 0) {
                    paren_depth -= 1;
                } else if (isSpace(c) && paren_depth == 0) {
                    break;
                }
                i += 1;
            }
            const tok = raw[start..i];
            tok_count += 1;
            if (tok_count > 3) return CssOutlineError.TooManyTokens;

            if (st := CssOutlineStyle.parse(tok)) {
                if (out.has_style) return CssOutlineError.DuplicateComponent;
                out.style = st;
                out.has_style = true;
                continue;
            }

            if (w := CssOutlineWidth.tryParse(tok)?) {
                if (out.has_width) return CssOutlineError.DuplicateComponent;
                out.width = w;
                out.has_width = true;
                continue;
            }

            // Otherwise treat as color token (named, hex, currentcolor, invert, rgb(...), etc.)
            if (out.has_color) return CssOutlineError.DuplicateComponent;
            out.color = tok;
            out.has_color = true;
        }

        if (tok_count == 0) return CssOutlineError.EmptyInput;
        return out;
    }

    fn isVisible(): bool {
        return _.style.isVisible();
    }

    fn isNone(): bool {
        return _.style == CssOutlineStyle.none;
    }

    fn isDefaultColor(): bool {
        return eqlIgnoreCase(_.color, "currentcolor");
    }

    /// Serializes the `outline` shorthand in canonical minimal CSS order (`width style color`),
    /// omitting default `medium` width and default `currentcolor` color when other parts exist.
    fn toCss(out: []u8): CssOutlineError!usize {
        let pos: usize = 0;
        let wrote = false;

        if (!_.width.isDefault()) {
            pos = appendBytes(out, pos, _.width.asStr())?;
            wrote = true;
        }

        if (_.style != CssOutlineStyle.none || (!wrote && _.isDefaultColor())) {
            if (wrote) pos = appendBytes(out, pos, " ")?;
            pos = appendBytes(out, pos, _.style.asStr())?;
            wrote = true;
        }

        if (!_.isDefaultColor()) {
            if (wrote) pos = appendBytes(out, pos, " ")?;
            pos = appendBytes(out, pos, _.color)?;
            wrote = true;
        }

        if (!wrote) {
            pos = appendBytes(out, pos, "none")?;
        }

        return pos;
    }

    fn eql(other: CssOutline): bool {
        return _.width.eql(other.width) &&
            _.style == other.style &&
            eqlIgnoreCase(_.color, other.color);
    }
}

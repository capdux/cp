// Source: Absorbed from /data/dev/bun/src/css/properties/overflow.cp | Synced: 2026-11-22T18:34:08Z
// ============================================================================
// lib/css/overflowProp.cp — Zero-Alloc CSS Overflow L3 `overflow` & `text-overflow`
// ============================================================================
// Implements W3C CSS Overflow Module Level 3:
//   - `CssOverflowAxis`: `visible`, `hidden`, `clip`, `scroll`, `auto`
//   - `CssOverflowProp`: 1-value & 2-value `overflow` shorthand parser, canonical
//     1-value collapse minifier (`hidden hidden` -> `hidden`), and browser-compat
//     2-value shorthand lowering (`overflow: hidden auto` -> `overflow-x:hidden;overflow-y:auto`)
//   - `CssTextOverflowProp`: `clip` / `ellipsis` parser & serializer.

pub err CssOverflowPropError {
    EmptyInput,
    InvalidKeyword,
    TrailingTokens,
    BufferTooSmall,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn eqlIgnoreAsciiCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn skipSpaces(s: []const u8, pos_in: usize): usize {
    let pos = pos_in;
    while (pos < s.len && isSpace(s[pos])) pos += 1;
    return pos;
}

fn readWordEnd(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len && !isSpace(s[i])) i += 1;
    return i;
}

fn writeSlice(out: []u8, pos: usize, src: []const u8): CssOverflowPropError!usize {
    if (pos + src.len > out.len) return CssOverflowPropError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// CSS Overflow Level 3 axis keyword (`overflow-x`, `overflow-y`, `overflow`).
pub enum CssOverflowAxis {
    visible,
    hidden,
    clip,
    scroll,
    auto_mode,

    static fn parse(raw: []const u8): CssOverflowPropError!CssOverflowAxis {
        const start = skipSpaces(raw, 0);
        if (start >= raw.len) return CssOverflowPropError.EmptyInput;
        const end = readWordEnd(raw, start);
        if (skipSpaces(raw, end) != raw.len) return CssOverflowPropError.TrailingTokens;
        return CssOverflowAxis.parseToken(raw[start..end]);
    }

    static fn parseToken(tok: []const u8): CssOverflowPropError!CssOverflowAxis {
        if (eqlIgnoreAsciiCase(tok, "visible")) return CssOverflowAxis.visible;
        if (eqlIgnoreAsciiCase(tok, "hidden")) return CssOverflowAxis.hidden;
        if (eqlIgnoreAsciiCase(tok, "clip")) return CssOverflowAxis.clip;
        if (eqlIgnoreAsciiCase(tok, "scroll")) return CssOverflowAxis.scroll;
        if (eqlIgnoreAsciiCase(tok, "auto")) return CssOverflowAxis.auto_mode;
        return CssOverflowPropError.InvalidKeyword;
    }

    fn asStr(): []const u8 {
        if (_ == CssOverflowAxis.visible) return "visible";
        if (_ == CssOverflowAxis.hidden) return "hidden";
        if (_ == CssOverflowAxis.clip) return "clip";
        if (_ == CssOverflowAxis.scroll) return "scroll";
        return "auto";
    }

    /// Returns true if programmatic scrolling (e.g. `element.scrollTo`) is allowed.
    /// Per CSS Overflow L3, `hidden`, `scroll`, and `auto` create scroll containers
    /// whereas `clip` forbids programmatic scrolling.
    fn allowsProgrammaticScroll(): bool {
        return _ == CssOverflowAxis.hidden ||
            _ == CssOverflowAxis.scroll ||
            _ == CssOverflowAxis.auto_mode;
    }

    fn isClipped(): bool {
        return _ == CssOverflowAxis.hidden || _ == CssOverflowAxis.clip;
    }

    fn toCss(out: []u8): CssOverflowPropError!usize {
        return writeSlice(out, 0, _.asStr());
    }
}

/// CSS Overflow Level 3 `overflow` shorthand (`<overflow-x> <overflow-y>?`).
pub struct CssOverflowProp {
    x: CssOverflowAxis = CssOverflowAxis.visible,
    y: CssOverflowAxis = CssOverflowAxis.visible,

    static fn default(): CssOverflowProp {
        return CssOverflowProp{
            x: CssOverflowAxis.visible,
            y: CssOverflowAxis.visible,
        };
    }

    static fn single(v: CssOverflowAxis): CssOverflowProp {
        return CssOverflowProp{ x: v, y: v };
    }

    /// Parses 1-value (`hidden`) or 2-value (`hidden auto`) `overflow` shorthand.
    static fn parse(raw: []const u8): CssOverflowPropError!CssOverflowProp {
        const p1 = skipSpaces(raw, 0);
        if (p1 >= raw.len) return CssOverflowPropError.EmptyInput;
        const e1 = readWordEnd(raw, p1);
        const x = CssOverflowAxis.parseToken(raw[p1..e1])?;

        const p2 = skipSpaces(raw, e1);
        if (p2 >= raw.len) {
            return CssOverflowProp{ x: x, y: x };
        }
        const e2 = readWordEnd(raw, p2);
        const y = CssOverflowAxis.parseToken(raw[p2..e2])?;

        if (skipSpaces(raw, e2) != raw.len) {
            return CssOverflowPropError.TrailingTokens;
        }
        return CssOverflowProp{ x: x, y: y };
    }

    fn isSymmetric(): bool {
        return _.x == _.y;
    }

    /// Returns true if `x != y` (requires `overflow-x`/`overflow-y` expansion for
    /// legacy browsers that do not support the 2-value `overflow` shorthand).
    fn needsTwoValueLowering(): bool {
        return _.x != _.y;
    }

    fn eql(other: CssOverflowProp): bool {
        return _.x == other.x && _.y == other.y;
    }

    /// Serializes the `overflow` value in minimal CSS form: collapses identical
    /// axes into a single keyword (aligned with Bun `Overflow.toCss`).
    fn toCss(out: []u8): CssOverflowPropError!usize {
        let pos = writeSlice(out, 0, _.x.asStr())?;
        if (_.y != _.x) {
            pos = writeSlice(out, pos, " ")?;
            pos = writeSlice(out, pos, _.y.asStr())?;
        }
        return pos;
    }

    /// Serializes complete CSS declaration(s) into `out`.
    /// When `x != y` and `supports_two_value == false`, lowers to longhands:
    ///   `overflow-x:<x>;overflow-y:<y>`
    /// Otherwise emits the minimal `overflow:<val>` shorthand.
    fn toCssDecls(supports_two_value: bool, minify: bool, out: []u8): CssOverflowPropError!usize {
        const colon = if (minify) ":" else ": ";
        const semi = if (minify) ";" else "; ";

        if (_.x != _.y && !supports_two_value) {
            let pos: usize = 0;
            pos = writeSlice(out, pos, "overflow-x")?;
            pos = writeSlice(out, pos, colon)?;
            pos = writeSlice(out, pos, _.x.asStr())?;
            pos = writeSlice(out, pos, semi)?;
            pos = writeSlice(out, pos, "overflow-y")?;
            pos = writeSlice(out, pos, colon)?;
            pos = writeSlice(out, pos, _.y.asStr())?;
            return pos;
        }

        let pos: usize = 0;
        pos = writeSlice(out, pos, "overflow")?;
        pos = writeSlice(out, pos, colon)?;
        const vlen = _.toCss(out[pos..out.len])?;
        return pos + vlen;
    }
}

/// CSS Overflow Level 3 `text-overflow` property (`clip` | `ellipsis`).
pub enum CssTextOverflowProp {
    clip,
    ellipsis,

    static fn default(): CssTextOverflowProp {
        return CssTextOverflowProp.clip;
    }

    static fn parse(raw: []const u8): CssOverflowPropError!CssTextOverflowProp {
        const start = skipSpaces(raw, 0);
        if (start >= raw.len) return CssOverflowPropError.EmptyInput;
        const end = readWordEnd(raw, start);
        if (skipSpaces(raw, end) != raw.len) return CssOverflowPropError.TrailingTokens;
        const tok = raw[start..end];
        if (eqlIgnoreAsciiCase(tok, "clip")) return CssTextOverflowProp.clip;
        if (eqlIgnoreAsciiCase(tok, "ellipsis")) return CssTextOverflowProp.ellipsis;
        return CssOverflowPropError.InvalidKeyword;
    }

    fn asStr(): []const u8 {
        if (_ == CssTextOverflowProp.clip) return "clip";
        return "ellipsis";
    }

    fn toCss(out: []u8): CssOverflowPropError!usize {
        return writeSlice(out, 0, _.asStr());
    }
}

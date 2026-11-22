// Source: Absorbed from /data/dev/bun/src/css/properties/position.cp | Synced: 2026-11-22T18:28:58Z
// ============================================================================
// lib/css/positionProp.cp — Zero-Alloc CSS Positioning L3 `position` Property
// ============================================================================
// Implements W3C CSS Positioned Layout Module Level 3 `position` property
// parsing (`static`, `relative`, `absolute`, `fixed`, `sticky`, `-webkit-sticky`),
// layout predicate helpers, Safari `-webkit-sticky` vendor-prefix lowering, and
// zero-allocation CSS value / declaration-list serialization.

pub const CSS_STICKY_PREFIX_NONE: u8 = 0x01;
pub const CSS_STICKY_PREFIX_WEBKIT: u8 = 0x02;
pub const CSS_STICKY_PREFIX_BOTH: u8 = 0x03;

pub err CssPositionPropError {
    EmptyInput,
    InvalidKeyword,
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

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
}

fn writeSlice(out: []u8, pos: usize, src: []const u8): CssPositionPropError!usize {
    if (pos + src.len > out.len) return CssPositionPropError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// CSS Positioning Level 3 `position` keyword variants.
pub enum CssPositionKind {
    static_pos,
    relative,
    absolute,
    fixed,
    sticky,
}

/// Zero-allocation representation of the CSS `position` property value,
/// including vendor-prefix tracking for `sticky` / `-webkit-sticky`.
pub struct CssPositionProp {
    kind: CssPositionKind = CssPositionKind.static_pos,
    sticky_prefixes: u8 = CSS_STICKY_PREFIX_NONE,

    static fn default(): CssPositionProp {
        return CssPositionProp{
            kind: CssPositionKind.static_pos,
            sticky_prefixes: CSS_STICKY_PREFIX_NONE,
        };
    }

    static fn sticky(prefixes: u8): CssPositionProp {
        const eff = if (prefixes == 0) CSS_STICKY_PREFIX_NONE else (prefixes & CSS_STICKY_PREFIX_BOTH);
        return CssPositionProp{
            kind: CssPositionKind.sticky,
            sticky_prefixes: eff,
        };
    }

    /// Parses a CSS `position` keyword (case-insensitive ASCII), matching Bun
    /// `Position.parse`: `static`, `relative`, `absolute`, `fixed`, `sticky`,
    /// and `-webkit-sticky`.
    static fn parse(raw: []const u8): CssPositionPropError!CssPositionProp {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssPositionPropError.EmptyInput;

        if (eqlIgnoreAsciiCase(s, "static")) {
            return CssPositionProp{ kind: CssPositionKind.static_pos, sticky_prefixes: CSS_STICKY_PREFIX_NONE };
        }
        if (eqlIgnoreAsciiCase(s, "relative")) {
            return CssPositionProp{ kind: CssPositionKind.relative, sticky_prefixes: CSS_STICKY_PREFIX_NONE };
        }
        if (eqlIgnoreAsciiCase(s, "absolute")) {
            return CssPositionProp{ kind: CssPositionKind.absolute, sticky_prefixes: CSS_STICKY_PREFIX_NONE };
        }
        if (eqlIgnoreAsciiCase(s, "fixed")) {
            return CssPositionProp{ kind: CssPositionKind.fixed, sticky_prefixes: CSS_STICKY_PREFIX_NONE };
        }
        if (eqlIgnoreAsciiCase(s, "sticky")) {
            return CssPositionProp{ kind: CssPositionKind.sticky, sticky_prefixes: CSS_STICKY_PREFIX_NONE };
        }
        if (eqlIgnoreAsciiCase(s, "-webkit-sticky")) {
            return CssPositionProp{ kind: CssPositionKind.sticky, sticky_prefixes: CSS_STICKY_PREFIX_WEBKIT };
        }
        return CssPositionPropError.InvalidKeyword;
    }

    /// Returns true if the element is positioned (anything other than `static`).
    fn isPositioned(): bool {
        return _.kind != CssPositionKind.static_pos;
    }

    /// Returns true if the box is taken out of normal document flow (`absolute` or `fixed`).
    fn isOutOfFlow(): bool {
        return _.kind == CssPositionKind.absolute || _.kind == CssPositionKind.fixed;
    }

    fn isSticky(): bool {
        return _.kind == CssPositionKind.sticky;
    }

    /// Returns true if this box establishes a containing block for `position: absolute` descendants.
    fn createsContainingBlockForAbsolute(): bool {
        return _.kind != CssPositionKind.static_pos;
    }

    /// Applies Safari compatibility vendor-prefix expansion for `position: sticky`.
    /// When `needs_webkit_sticky` is true, merges `-webkit-sticky` alongside standard `sticky`.
    fn withSafariCompat(needs_webkit_sticky: bool): CssPositionProp {
        if (_.kind != CssPositionKind.sticky) return _;
        if (!needs_webkit_sticky) return _;
        return CssPositionProp{
            kind: CssPositionKind.sticky,
            sticky_prefixes: _.sticky_prefixes | CSS_STICKY_PREFIX_WEBKIT | CSS_STICKY_PREFIX_NONE,
        };
    }

    fn eql(other: CssPositionProp): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == CssPositionKind.sticky) {
            return _.sticky_prefixes == other.sticky_prefixes;
        }
        return true;
    }

    /// Serializes a single `position` value into `out`.
    /// If `sticky` has only the WebKit prefix bit set, emits `"-webkit-sticky"`;
    /// otherwise emits the standard keyword.
    fn toCss(out: []u8): CssPositionPropError!usize {
        if (_.kind == CssPositionKind.static_pos) return writeSlice(out, 0, "static");
        if (_.kind == CssPositionKind.relative) return writeSlice(out, 0, "relative");
        if (_.kind == CssPositionKind.absolute) return writeSlice(out, 0, "absolute");
        if (_.kind == CssPositionKind.fixed) return writeSlice(out, 0, "fixed");
        if (_.sticky_prefixes == CSS_STICKY_PREFIX_WEBKIT) {
            return writeSlice(out, 0, "-webkit-sticky");
        }
        return writeSlice(out, 0, "sticky");
    }

    /// Serializes one or more `position: <val>` declarations into `out`.
    /// When `sticky` has both WebKit and standard bits set (`CSS_STICKY_PREFIX_BOTH`),
    /// emits the prefixed fallback first followed by the standard declaration:
    /// - `minify=true`:  `"position:-webkit-sticky;position:sticky"`
    /// - `minify=false`: `"position: -webkit-sticky; position: sticky"`
    fn toCssDecls(minify: bool, out: []u8): CssPositionPropError!usize {
        const prefix_str = if (minify) "position:" else "position: ";
        const sep_str = if (minify) ";" else "; ";

        if (_.kind == CssPositionKind.sticky &&
            (_.sticky_prefixes & CSS_STICKY_PREFIX_BOTH) == CSS_STICKY_PREFIX_BOTH)
        {
            let pos: usize = 0;
            pos = writeSlice(out, pos, prefix_str)?;
            pos = writeSlice(out, pos, "-webkit-sticky")?;
            pos = writeSlice(out, pos, sep_str)?;
            pos = writeSlice(out, pos, prefix_str)?;
            pos = writeSlice(out, pos, "sticky")?;
            return pos;
        }

        let pos: usize = 0;
        pos = writeSlice(out, pos, prefix_str)?;
        const val_len = _.toCss(out[pos..out.len])?;
        return pos + val_len;
    }
}

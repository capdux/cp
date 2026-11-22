// Source: Absorbed from /data/dev/bun/src/css/properties/contain.cp | Synced: 2026-11-22T18:09:54Z
// ============================================================================
// lib/css/containProp.cp — Zero-Alloc CSS Containment L3 `container` Properties
// ============================================================================
// Implements W3C CSS Containment Module Level 3 declaration properties:
// `container-type`, `container-name` (`none | <custom-ident>+`, max 16 names),
// and `container` shorthand (`<'container-name'> [ / <'container-type'> ]?`)
// with reserved-keyword validation and minimal CSS serialization.

pub const CSS_CONTAINER_MAX_NAMES: usize = 16;

pub err CssContainError {
    EmptyInput,
    InvalidName,
    ReservedKeyword,
    TooManyNames,
    InvalidType,
    InvalidShorthand,
    BufferTooSmall,
}

fn toLowerAscii(c: u8): u8 {
    return if (c >= 'A' && c <= 'Z') (c + 32) else c;
}

fn eqlExact(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
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

fn trimSpaces(raw: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = raw.len;
    while (start < end && isSpace(raw[start])) start += 1;
    while (end > start && isSpace(raw[end - 1])) end -= 1;
    return raw[start..end];
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isValidCustomIdentSyntax(s: []const u8): bool {
    if (s.len == 0) return false;
    if (!isIdentStart(s[0])) return false;
    // Per CSS Syntax L3, `--*` custom properties or single `-` followed by digit are not valid <custom-ident>
    if (s[0] == '-' && s.len >= 2 && s[1] == '-') return false;
    if (s[0] == '-' && s.len == 1) return false;
    if (s[0] == '-' && s.len >= 2 && s[1] >= '0' && s[1] <= '9') return false;
    let i: usize = 1;
    while (i < s.len) {
        if (!isIdentChar(s[i])) return false;
        i += 1;
    }
    return true;
}

/// W3C CSS Containment L3 forbids CSS-wide keywords and boolean query keywords as container names.
pub fn isReservedContainerName(s: []const u8): bool {
    return eqlIgnoreCase(s, "none") ||
        eqlIgnoreCase(s, "and") ||
        eqlIgnoreCase(s, "not") ||
        eqlIgnoreCase(s, "or") ||
        eqlIgnoreCase(s, "default") ||
        eqlIgnoreCase(s, "initial") ||
        eqlIgnoreCase(s, "inherit") ||
        eqlIgnoreCase(s, "unset") ||
        eqlIgnoreCase(s, "revert") ||
        eqlIgnoreCase(s, "revert-layer");
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): CssContainError!usize {
    if (pos + src.len > out.len) return CssContainError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// W3C CSS Containment L3 `container-type` property value.
pub enum CssContainType: u8 {
    normal = 0,
    size = 1,
    inline_size = 2,

    static fn default(): CssContainType {
        return CssContainType.normal;
    }

    static fn parse(raw: []const u8): CssContainType? {
        const s = trimSpaces(raw);
        if (eqlIgnoreCase(s, "normal")) return CssContainType.normal;
        if (eqlIgnoreCase(s, "size")) return CssContainType.size;
        if (eqlIgnoreCase(s, "inline-size")) return CssContainType.inline_size;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .normal => "normal",
            .size => "size",
            .inline_size => "inline-size",
        };
    }

    fn isDefault(): bool {
        return _ == CssContainType.normal;
    }

    fn establishesSizeContainer(): bool {
        return _ == CssContainType.size || _ == CssContainType.inline_size;
    }
}

/// W3C CSS Containment L3 `container-name` property (`none | <custom-ident>+`).
/// Zero-alloc fixed-capacity list of up to 16 container names; `len == 0` represents `none`.
pub struct CssContainerNameList {
    names: [CSS_CONTAINER_MAX_NAMES][]const u8 = [""; CSS_CONTAINER_MAX_NAMES],
    len: usize = 0,

    static fn none(): CssContainerNameList {
        return CssContainerNameList{};
    }

    static fn parse(raw: []const u8): CssContainError!CssContainerNameList {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssContainError.EmptyInput;
        if (eqlIgnoreCase(s, "none")) return CssContainerNameList.none();

        let list = CssContainerNameList.none();
        let i: usize = 0;
        while (i < s.len) {
            while (i < s.len && isSpace(s[i])) i += 1;
            if (i >= s.len) break;
            const start = i;
            while (i < s.len && !isSpace(s[i])) i += 1;
            const tok = s[start..i];

            if (isReservedContainerName(tok)) return CssContainError.ReservedKeyword;
            if (!isValidCustomIdentSyntax(tok)) return CssContainError.InvalidName;
            if (list.len >= CSS_CONTAINER_MAX_NAMES) return CssContainError.TooManyNames;
            list.names[list.len] = tok;
            list.len += 1;
        }
        if (list.len == 0) return CssContainError.EmptyInput;
        return list;
    }

    fn isNone(): bool {
        return _.len == 0;
    }

    /// Case-sensitive `<custom-ident>` membership check per CSS Containment L3.
    fn contains(target: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (eqlExact(_.names[i], target)) return true;
            i += 1;
        }
        return false;
    }

    fn get(index: usize): ([]const u8)? {
        if (index >= _.len) return null;
        return _.names[index];
    }

    fn toCss(out: []u8): CssContainError!usize {
        if (_.len == 0) return appendBytes(out, 0, "none");
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) pos = appendBytes(out, pos, " ")?;
            pos = appendBytes(out, pos, _.names[i])?;
            i += 1;
        }
        return pos;
    }

    fn eql(other: CssContainerNameList): bool {
        if (_.len != other.len) return false;
        let i: usize = 0;
        while (i < _.len) {
            if (!eqlExact(_.names[i], other.names[i])) return false;
            i += 1;
        }
        return true;
    }
}

/// W3C CSS Containment L3 `container` shorthand property (`<'container-name'> [ / <'container-type'> ]?`).
pub struct CssContainerShorthand {
    name: CssContainerNameList = CssContainerNameList.none(),
    container_type: CssContainType = CssContainType.normal,

    static fn default(): CssContainerShorthand {
        return CssContainerShorthand{};
    }

    static fn parse(raw: []const u8): CssContainError!CssContainerShorthand {
        const s = trimSpaces(raw);
        if (s.len == 0) return CssContainError.EmptyInput;

        // Find optional '/' separator (at most one '/')
        let slash_pos: usize = s.len;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '/') {
                if (slash_pos != s.len) return CssContainError.InvalidShorthand;
                slash_pos = i;
            }
            i += 1;
        }

        if (slash_pos == s.len) {
            const names = CssContainerNameList.parse(s)?;
            return CssContainerShorthand{
                name: names,
                container_type: CssContainType.normal,
            };
        }

        const name_part = trimSpaces(s[0..slash_pos]);
        const type_part = trimSpaces(s[(slash_pos + 1)..]);
        if (name_part.len == 0 || type_part.len == 0) return CssContainError.InvalidShorthand;

        const names = CssContainerNameList.parse(name_part)?;
        if (ct := CssContainType.parse(type_part)) {
            return CssContainerShorthand{
                name: names,
                container_type: ct,
            };
        }
        return CssContainError.InvalidType;
    }

    /// Serializes `container` shorthand in minimal form: omits `" / normal"` when type is default.
    fn toCss(out: []u8): CssContainError!usize {
        let pos = _.name.toCss(out)?;
        if (!_.container_type.isDefault()) {
            pos = appendBytes(out, pos, " / ")?;
            pos = appendBytes(out, pos, _.container_type.asStr())?;
        }
        return pos;
    }

    fn eql(other: CssContainerShorthand): bool {
        return _.name.eql(other.name) && _.container_type == other.container_type;
    }
}

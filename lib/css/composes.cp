// Source: Absorbed from /data/dev/bun/src/css/properties/css_modules.cp | Synced: 2026-11-22T18:17:06Z
// ============================================================================
// lib/css/composes.cp — Zero-Alloc CSS Modules `composes` Property Engine
// ============================================================================
// Implements CSS Modules `composes` declaration parsing, validation, and
// zero-allocation serialization:
//   - Local composition:    `composes: foo bar`
//   - Global composition:   `composes: foo bar from global`
//   - External module file: `composes: foo from "./base.module.css"` or `from url(...)`
// Also provides `isValidComposesSelector` enforcing the CSS Modules / Bun rule
// that `composes` is only allowed inside a simple single-class selector.

pub const CSS_COMPOSES_MAX_NAMES: usize = 16;

pub err CssComposesError {
    EmptyInput,
    MissingClassName,
    InvalidIdent,
    TooManyNames,
    MissingSpecifier,
    InvalidSpecifier,
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

fn eqlExact(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn isValidCustomIdent(s: []const u8): bool {
    if (s.len == 0) return false;
    // CSS custom-ident cannot be a CSS-wide keyword or `default`.
    if (eqlIgnoreAsciiCase(s, "initial") ||
        eqlIgnoreAsciiCase(s, "inherit") ||
        eqlIgnoreAsciiCase(s, "unset") ||
        eqlIgnoreAsciiCase(s, "revert") ||
        eqlIgnoreAsciiCase(s, "revert-layer") ||
        eqlIgnoreAsciiCase(s, "default"))
    {
        return false;
    }
    // Disallow `--` custom property prefix.
    if (s.len >= 2 && s[0] == '-' && s[1] == '-') return false;

    let i: usize = 0;
    if (s[0] == '-') {
        if (s.len == 1) return false;
        if (!isIdentStart(s[1])) return false;
        i = 2;
    } else {
        if (!isIdentStart(s[0])) return false;
        i = 1;
    }
    while (i < s.len) {
        if (!isIdentChar(s[i])) return false;
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

fn writeSlice(out: []u8, pos: usize, src: []const u8): CssComposesError!usize {
    if (pos + src.len > out.len) return CssComposesError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeQuotedCssString(out: []u8, pos_in: usize, s: []const u8): CssComposesError!usize {
    let pos = pos_in;
    if (pos >= out.len) return CssComposesError.BufferTooSmall;
    out[pos] = '"';
    pos += 1;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"' || c == '\\') {
            if (pos + 2 > out.len) return CssComposesError.BufferTooSmall;
            out[pos] = '\\';
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos >= out.len) return CssComposesError.BufferTooSmall;
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    if (pos >= out.len) return CssComposesError.BufferTooSmall;
    out[pos] = '"';
    return pos + 1;
}

/// Validates whether a CSS selector is eligible to contain a `composes` property
/// according to the CSS Modules / Bun rule: must be a simple selector with a
/// single class name (e.g. `.button`, `.card-header`).
pub fn isValidComposesSelector(selector: []const u8): bool {
    let start: usize = 0;
    let end: usize = selector.len;
    while (start < end && isSpace(selector[start])) start += 1;
    while (end > start && isSpace(selector[end - 1])) end -= 1;
    const s = selector[start..end];
    if (s.len < 2 || s[0] != '.') return false;
    return isValidCustomIdent(s[1..s.len]);
}

/// Origin kind for a CSS Modules `composes` target.
pub enum CssComposesFromKind {
    local,
    global,
    file,
}

/// Defines where the class names referenced in a `composes` declaration come from.
pub struct CssComposesSpecifier {
    kind: CssComposesFromKind = CssComposesFromKind.local,
    file_path: []const u8 = "",
    import_record_index: u32 = 0,

    static fn local(): CssComposesSpecifier {
        return CssComposesSpecifier{
            kind: CssComposesFromKind.local,
            file_path: "",
            import_record_index: 0,
        };
    }

    static fn global(): CssComposesSpecifier {
        return CssComposesSpecifier{
            kind: CssComposesFromKind.global,
            file_path: "",
            import_record_index: 0,
        };
    }

    static fn file(path: []const u8, record_idx: u32): CssComposesSpecifier {
        return CssComposesSpecifier{
            kind: CssComposesFromKind.file,
            file_path: path,
            import_record_index: record_idx,
        };
    }

    fn isLocal(): bool {
        return _.kind == CssComposesFromKind.local;
    }

    fn isGlobal(): bool {
        return _.kind == CssComposesFromKind.global;
    }

    fn isFile(): bool {
        return _.kind == CssComposesFromKind.file;
    }

    fn eql(other: CssComposesSpecifier): bool {
        if (_.kind != other.kind) return false;
        if (_.kind == CssComposesFromKind.file) {
            return eqlExact(_.file_path, other.file_path) &&
                _.import_record_index == other.import_record_index;
        }
        return true;
    }

    /// Serializes the specifier suffix (`"global"` or quoted file path).
    /// Returns 0 bytes written for `.local`.
    fn toCss(out: []u8): CssComposesError!usize {
        if (_.kind == CssComposesFromKind.local) return 0;
        if (_.kind == CssComposesFromKind.global) {
            return writeSlice(out, 0, "global");
        }
        return writeQuotedCssString(out, 0, _.file_path);
    }
}

fn parseQuotedPath(s: []const u8, start: usize): CssComposesError!struct { path: []const u8, next_pos: usize } {
    const quote = s[start];
    let i = start + 1;
    while (i < s.len) {
        if (s[i] == '\\') {
            i += 2;
            continue;
        }
        if (s[i] == quote) {
            const inner = s[(start + 1)..i];
            if (inner.len == 0) return CssComposesError.InvalidSpecifier;
            return { path: inner, next_pos: i + 1 };
        }
        i += 1;
    }
    return CssComposesError.InvalidSpecifier;
}

fn parseFromSpecifier(s: []const u8, start_in: usize): CssComposesError!CssComposesSpecifier {
    const pos = skipSpaces(s, start_in);
    if (pos >= s.len) return CssComposesError.MissingSpecifier;

    // 1. Quoted string: `"./base.module.css"` or `'./base.module.css'`
    if (s[pos] == '"' || s[pos] == '\'') {
        const res = parseQuotedPath(s, pos)?;
        const after = skipSpaces(s, res.next_pos);
        if (after != s.len) return CssComposesError.TrailingTokens;
        return CssComposesSpecifier.file(res.path, 0);
    }

    // 2. `url(...)` functional specifier (aligned with Bun `expectUrlOrString`)
    if (pos + 4 <= s.len &&
        toLowerAscii(s[pos]) == 'u' &&
        toLowerAscii(s[pos + 1]) == 'r' &&
        toLowerAscii(s[pos + 2]) == 'l' &&
        s[pos + 3] == '(')
    {
        let inner_start = skipSpaces(s, pos + 4);
        if (inner_start >= s.len) return CssComposesError.InvalidSpecifier;

        let path_slice: []const u8 = "";
        let close_scan = inner_start;
        if (s[inner_start] == '"' || s[inner_start] == '\'') {
            const qres = parseQuotedPath(s, inner_start)?;
            path_slice = qres.path;
            close_scan = skipSpaces(s, qres.next_pos);
        } else {
            let uend = inner_start;
            while (uend < s.len && s[uend] != ')' && !isSpace(s[uend])) uend += 1;
            path_slice = s[inner_start..uend];
            if (path_slice.len == 0) return CssComposesError.InvalidSpecifier;
            close_scan = skipSpaces(s, uend);
        }

        if (close_scan >= s.len || s[close_scan] != ')') return CssComposesError.InvalidSpecifier;
        const after = skipSpaces(s, close_scan + 1);
        if (after != s.len) return CssComposesError.TrailingTokens;
        return CssComposesSpecifier.file(path_slice, 0);
    }

    // 3. `global` keyword (case-insensitive)
    const wend = readWordEnd(s, pos);
    const word = s[pos..wend];
    if (eqlIgnoreAsciiCase(word, "global")) {
        const after = skipSpaces(s, wend);
        if (after != s.len) return CssComposesError.TrailingTokens;
        return CssComposesSpecifier.global();
    }

    return CssComposesError.InvalidSpecifier;
}

/// Zero-allocation representation of a CSS Modules `composes` property value.
pub struct CssComposesProp {
    names: [16][]const u8 = [""; 16],
    len: usize = 0,
    from_spec: CssComposesSpecifier = CssComposesSpecifier{},

    /// Parses a `composes` declaration value, e.g.:
    /// - `"primary bold"`
    /// - `"btn from global"`
    /// - `"card shadow from \"./base.module.css\""`
    static fn parse(raw: []const u8): CssComposesError!CssComposesProp {
        let pos = skipSpaces(raw, 0);
        if (pos >= raw.len) return CssComposesError.EmptyInput;

        let out = CssComposesProp{};
        let saw_from = false;

        while (pos < raw.len) {
            const wend = readWordEnd(raw, pos);
            const tok = raw[pos..wend];
            if (eqlIgnoreAsciiCase(tok, "from")) {
                saw_from = true;
                pos = wend;
                break;
            }
            if (!isValidCustomIdent(tok)) return CssComposesError.InvalidIdent;
            if (out.len >= CSS_COMPOSES_MAX_NAMES) return CssComposesError.TooManyNames;
            out.names[out.len] = tok;
            out.len += 1;
            pos = skipSpaces(raw, wend);
        }

        if (out.len == 0) return CssComposesError.MissingClassName;

        if (saw_from) {
            out.from_spec = parseFromSpecifier(raw, pos)?;
        }

        return out;
    }

    fn count(): usize {
        return _.len;
    }

    fn get(index: usize): ([]const u8)? {
        if (index >= _.len) return null;
        return _.names[index];
    }

    /// Case-sensitive class name lookup.
    fn contains(name: []const u8): bool {
        let i: usize = 0;
        while (i < _.len) {
            if (eqlExact(_.names[i], name)) return true;
            i += 1;
        }
        return false;
    }

    fn isLocal(): bool {
        return _.from_spec.isLocal();
    }

    fn isGlobal(): bool {
        return _.from_spec.isGlobal();
    }

    fn isExternalFile(): bool {
        return _.from_spec.isFile();
    }

    fn eql(other: CssComposesProp): bool {
        if (_.len != other.len) return false;
        if (!_.from_spec.eql(other.from_spec)) return false;
        let i: usize = 0;
        while (i < _.len) {
            if (!eqlExact(_.names[i], other.names[i])) return false;
            i += 1;
        }
        return true;
    }

    /// Serializes the `composes` value in canonical CSS Modules form into `out`.
    fn toCss(out: []u8): CssComposesError!usize {
        if (_.len == 0) return CssComposesError.MissingClassName;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                pos = writeSlice(out, pos, " ")?;
            }
            pos = writeSlice(out, pos, _.names[i])?;
            i += 1;
        }

        if (!_.from_spec.isLocal()) {
            pos = writeSlice(out, pos, " from ")?;
            const spec_len = _.from_spec.toCss(out[pos..out.len])?;
            pos += spec_len;
        }
        return pos;
    }
}

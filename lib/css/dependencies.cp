// Source: Absorbed from /data/dev/bun/src/css/dependencies.cp | Synced: 2026-11-22T01:33:00Z
// Pure-CP zero-allocation CSS `@import` & `url(...)` dependency analyzer,
// source range tracker, and deterministic placeholder hash generator.

/// Options controlling CSS dependency extraction.
pub struct CssDependencyOptions {
    /// Whether bundler/printer intends to strip `@import` rules.
    remove_imports: bool = false,
    /// Skip `data:` inline URIs when scanning `url(...)` dependencies.
    ignore_data_urls: bool = true,
    /// Skip external `http://` / `https://` / `//` URLs.
    ignore_external_urls: bool = false,
}

/// 1-based line and column position within a CSS source file.
pub struct CssDependencyLocation {
    line: u32 = 1,
    column: u32 = 1,

    pub static fn fromZeroBased(line0: u32, col0: u32): CssDependencyLocation {
        return CssDependencyLocation{
            line: line0 + 1,
            column: col0 + 1,
        };
    }

    pub fn eql(other: CssDependencyLocation): bool {
        return _.line == other.line && _.column == other.column;
    }
}

/// Source range where a CSS dependency was found (mirrors Bun `SourceRange`).
pub struct CssSourceRange {
    file_path: []const u8 = "",
    start: CssDependencyLocation = CssDependencyLocation{},
    end: CssDependencyLocation = CssDependencyLocation{},

    pub static fn init(
        file_path: []const u8,
        loc: CssDependencyLocation,
        offset: u32,
        len: usize,
    ): CssSourceRange {
        const start_col = loc.column + offset;
        const end_col: u32 = if (len == 0) start_col else start_col + @as(u32, @intCast(len)) - 1;
        return CssSourceRange{
            file_path: file_path,
            start: CssDependencyLocation{ line: loc.line, column: start_col },
            end: CssDependencyLocation{ line: loc.line, column: end_col },
        };
    }
}

/// Kind of extracted CSS dependency.
pub enum CssDependencyKind: u8 {
    import_rule = 0,
    url_ref = 1,
}

/// Extracted `@import` or `url(...)` dependency with deterministic placeholder hash.
pub struct CssDependency {
    kind: CssDependencyKind = .url_ref,
    url: []const u8 = "",
    placeholder_buf: [16]u8 = [0; 16],
    placeholder_len: u8 = 0,
    supports: ([]const u8)? = null,
    media: ([]const u8)? = null,
    loc: CssSourceRange = CssSourceRange{},

    pub fn placeholder(): []const u8 {
        return _.placeholder_buf[0..@as(usize, _.placeholder_len)];
    }

    pub fn isImport(): bool {
        return _.kind == CssDependencyKind.import_rule;
    }

    pub fn isUrl(): bool {
        return _.kind == CssDependencyKind.url_ref;
    }
}

const HEX_LOWER: []const u8 = "0123456789abcdef";

/// Compute a deterministic 16-char lowercase hex placeholder hash for `(file_path, url)`.
pub fn formatDependencyPlaceholder(file_path: []const u8, url: []const u8, out: *[16]u8): []const u8 {
    // 64-bit FNV-1a over `file_path + "_" + url` (0xcbf29ce484222325)
    let h: u64 = (@as(u64, 0xcbf29ce4) << 32) | @as(u64, 0x84222325);
    const prime: u64 = 1099511628211;
    let i: usize = 0;
    while (i < file_path.len) : (i += 1) {
        h = (h ^ @as(u64, file_path[i])) *% prime;
    }
    h = (h ^ @as(u64, '_')) *% prime;
    i = 0;
    while (i < url.len) : (i += 1) {
        h = (h ^ @as(u64, url[i])) *% prime;
    }
    let k: usize = 0;
    while (k < 16) : (k += 1) {
        const shift: u6 = @intCast((15 - k) * 4);
        const nib: usize = @intCast((h >> shift) & 0xF);
        out.*[k] = HEX_LOWER[nib];
    }
    return out.*[0..16];
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_';
}

fn startsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (asciiLower(s[i]) != asciiLower(prefix[i])) return false;
    }
    return true;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) : (start += 1) {}
    while (end > start && isSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn shouldSkipUrl(url: []const u8, opts: CssDependencyOptions): bool {
    if (url.len == 0) return true;
    // Fragment-only SVG/CSS references (e.g. `url(#clip)`) are local, not external dependencies.
    if (url[0] == '#') return true;
    if (opts.ignore_data_urls && startsWithIgnoreCase(url, "data:")) return true;
    if (opts.ignore_external_urls) {
        if (startsWithIgnoreCase(url, "http://") ||
            startsWithIgnoreCase(url, "https://") ||
            (url.len >= 2 && url[0] == '/' && url[1] == '/'))
        {
            return true;
        }
    }
    return false;
}

/// Zero-allocation streaming iterator extracting `@import` and `url(...)` dependencies from CSS.
pub struct CssDependencyIter {
    src: []const u8,
    file_path: []const u8,
    opts: CssDependencyOptions,
    pos: usize = 0,
    line: u32 = 1,
    col: u32 = 1,

    pub static fn init(
        source: []const u8,
        file_path: []const u8,
        opts: CssDependencyOptions,
    ): CssDependencyIter {
        return CssDependencyIter{
            src: source,
            file_path: file_path,
            opts: opts,
            pos: 0,
            line: 1,
            col: 1,
        };
    }

    fn advance(*_, count: usize) {
        let k: usize = 0;
        while (k < count && _.pos < _.src.len) : (k += 1) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '\n') {
                _.line += 1;
                _.col = 1;
            } else {
                _.col += 1;
            }
        }
    }

    fn skipComment(*_): bool {
        if (_.pos + 1 < _.src.len && _.src[_.pos] == '/' && _.src[_.pos + 1] == '*') {
            _.advance(2);
            while (_.pos < _.src.len) {
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '*' && _.src[_.pos + 1] == '/') {
                    _.advance(2);
                    return true;
                }
                _.advance(1);
            }
            return true;
        }
        return false;
    }

    fn skipQuotedString(*_): ([]const u8)? {
        if (_.pos >= _.src.len) return null;
        const quote = _.src[_.pos];
        if (quote != '"' && quote != '\'') return null;
        _.advance(1);
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\\' && _.pos + 1 < _.src.len) {
                _.advance(2);
                continue;
            }
            if (c == quote) {
                const content = _.src[start.._.pos];
                _.advance(1);
                return content;
            }
            if (c == '\n') return null;
            _.advance(1);
        }
        return null;
    }

    fn skipSpacesAndComments(*_) {
        while (_.pos < _.src.len) {
            if (isSpace(_.src[_.pos])) {
                _.advance(1);
            } else if (!_.skipComment()) {
                break;
            }
        }
    }

    fn parseUrlCall(*_, call_loc: CssDependencyLocation): CssDependency? {
        // Cursor is right after `url(`
        _.skipSpacesAndComments();
        if (_.pos >= _.src.len) return null;

        let url_slice: []const u8 = "";
        let url_offset: u32 = 4;
        if (_.src[_.pos] == '"' || _.src[_.pos] == '\'') {
            const q_col = _.col;
            const s = _.skipQuotedString() ?? return null;
            url_slice = s;
            if (q_col >= call_loc.column) {
                url_offset = q_col - call_loc.column;
            }
            _.skipSpacesAndComments();
            if (_.pos < _.src.len && _.src[_.pos] == ')') {
                _.advance(1);
            } else {
                return null;
            }
        } else {
            const u_col = _.col;
            const start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ')' && _.src[_.pos] != '\n') {
                _.advance(1);
            }
            if (_.pos >= _.src.len || _.src[_.pos] != ')') return null;
            url_slice = trimSpaces(_.src[start.._.pos]);
            _.advance(1);
            if (u_col >= call_loc.column) {
                url_offset = u_col - call_loc.column;
            }
        }

        if (shouldSkipUrl(url_slice, _.opts)) return null;
        let dep = CssDependency{
            kind: CssDependencyKind.url_ref,
            url: url_slice,
            loc: CssSourceRange.init(_.file_path, call_loc, url_offset, url_slice.len),
        };
        * = formatDependencyPlaceholder(_.file_path, url_slice, &dep.placeholder_buf);
        dep.placeholder_len = 16;
        return dep;
    }

    fn parseImportRule(*_, rule_loc: CssDependencyLocation): CssDependency? {
        // Cursor is right after `@import`
        _.skipSpacesAndComments();
        if (_.pos >= _.src.len) return null;

        let url_slice: []const u8 = "";
        let url_col: u32 = _.col;
        if (_.src[_.pos] == '"' || _.src[_.pos] == '\'') {
            url_slice = _.skipQuotedString() ?? return null;
        } else if (startsWithIgnoreCase(_.src[_.pos..], "url(")) {
            _.advance(4);
            _.skipSpacesAndComments();
            url_col = _.col;
            if (_.pos < _.src.len && (_.src[_.pos] == '"' || _.src[_.pos] == '\'')) {
                url_slice = _.skipQuotedString() ?? return null;
                _.skipSpacesAndComments();
                if (_.pos >= _.src.len || _.src[_.pos] != ')') return null;
                _.advance(1);
            } else {
                const start = _.pos;
                while (_.pos < _.src.len && _.src[_.pos] != ')' && _.src[_.pos] != '\n') {
                    _.advance(1);
                }
                if (_.pos >= _.src.len || _.src[_.pos] != ')') return null;
                url_slice = trimSpaces(_.src[start.._.pos]);
                _.advance(1);
            }
        } else {
            return null;
        }

        // Scan optional `supports(...)` and trailing media query up to `;` or newline/EOF
        _.skipSpacesAndComments();
        let supports_opt: ([]const u8)? = null;
        if (startsWithIgnoreCase(_.src[_.pos..], "supports(")) {
            _.advance(9);
            const sup_start = _.pos;
            let depth: u32 = 1;
            while (_.pos < _.src.len && depth > 0) {
                const c = _.src[_.pos];
                if (c == '(') depth += 1 else if (c == ')') depth -= 1;
                if (depth > 0) _.advance(1);
            }
            if (depth == 0) {
                supports_opt = trimSpaces(_.src[sup_start.._.pos]);
                _.advance(1); // consume ')'
            }
            _.skipSpacesAndComments();
        }

        const media_start = _.pos;
        while (_.pos < _.src.len && _.src[_.pos] != ';' && _.src[_.pos] != '{') {
            _.advance(1);
        }
        const raw_media = trimSpaces(_.src[media_start.._.pos]);
        if (_.pos < _.src.len && _.src[_.pos] == ';') {
            _.advance(1);
        }

        if (shouldSkipUrl(url_slice, _.opts)) return null;
        const url_off: u32 = if (url_col >= rule_loc.column) url_col - rule_loc.column else 8;
        let dep = CssDependency{
            kind: CssDependencyKind.import_rule,
            url: url_slice,
            supports: supports_opt,
            media: if (raw_media.len > 0) raw_media else null,
            loc: CssSourceRange.init(_.file_path, rule_loc, url_off, url_slice.len),
        };
        * = formatDependencyPlaceholder(_.file_path, url_slice, &dep.placeholder_buf);
        dep.placeholder_len = 16;
        return dep;
    }

    /// Yield the next `@import` or `url(...)` dependency, or `null` at EOF.
    pub fn next(*_): CssDependency? {
        while (_.pos < _.src.len) {
            if (_.skipComment()) continue;
            const c = _.src[_.pos];

            // Skip standalone string literals outside `@import` / `url(` so `content: "url(x)"` is not misparsed
            if (c == '"' || c == '\'') {
                if (_.skipQuotedString() == null) {
                    _.advance(1);
                }
                continue;
            }

            if (c == '@' && startsWithIgnoreCase(_.src[_.pos..], "@import")) {
                const after = _.pos + 7;
                if (after < _.src.len && (isSpace(_.src[after]) || _.src[after] == '"' || _.src[after] == '\'')) {
                    const loc = CssDependencyLocation{ line: _.line, column: _.col };
                    _.advance(7);
                    if (dep := _.parseImportRule(loc)) {
                        return dep;
                    }
                    continue;
                }
            }

            if ((c == 'u' || c == 'U') && startsWithIgnoreCase(_.src[_.pos..], "url(")) {
                const prev_ok = _.pos == 0 || !isIdentChar(_.src[_.pos - 1]);
                if (prev_ok) {
                    const loc = CssDependencyLocation{ line: _.line, column: _.col };
                    _.advance(4);
                    if (dep := _.parseUrlCall(loc)) {
                        return dep;
                    }
                    continue;
                }
            }

            _.advance(1);
        }
        return null;
    }
}

// Source: Absorbed from /data/dev/bun/src/css/rules/font_face.cp | Synced: 2026-11-23T21:39:27Z
// Ported from Bun: /data/dev/bun/src/css/rules/font_face.cp & /data/dev/bun/docs/bundler/css.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation CSS `@font-face` rule parser & serializer:
// W3C `<urange>` parser with `?` wildcard compression (`U+0000-00FF` -> `U+??`),
// `src` (`url(...) format(...) tech(...)` & `local(...)`) parser, `font-display`, and code-point matching.

/// W3C Unicode 最大合法码点
pub const CSS_UNICODE_MAX_CODEPOINT: u32 = 0x10FFFF;

/// 单条 `@font-face` 支持的最大 `src` 来源条目数
pub const CSS_FONT_FACE_MAX_SOURCES: usize = 16;

/// 单条 `@font-face` 支持的最大 `unicode-range` 区间数
pub const CSS_FONT_FACE_MAX_RANGES: usize = 32;

/// `@font-face` 解析与序列化错误集
pub err CssFontFaceError {
    InvalidRuleSyntax,
    InvalidUnicodeRange,
    CodePointOutOfRange,
    ReversedUnicodeRange,
    InvalidSource,
    EmptyFontFamily,
    InvalidFontDisplay,
    TooManySources,
    TooManyRanges,
    BufferTooSmall,
}

/// CSS 字体格式（`format(...)` 关键字，对齐 Bun `FontFormat`）
pub enum CssFontFormat: u8 {
    unknown = 0,
    collection = 1,
    embedded_opentype = 2,
    opentype = 3,
    svg = 4,
    truetype = 5,
    woff = 6,
    woff2 = 7,

    static fn parse(raw: []const u8): CssFontFormat {
        const s = stripQuotes(trimWs(raw));
        if (eqCi(s, "woff2")) { return CssFontFormat.woff2; }
        if (eqCi(s, "woff")) { return CssFontFormat.woff; }
        if (eqCi(s, "truetype")) { return CssFontFormat.truetype; }
        if (eqCi(s, "opentype")) { return CssFontFormat.opentype; }
        if (eqCi(s, "embedded-opentype")) { return CssFontFormat.embedded_opentype; }
        if (eqCi(s, "collection")) { return CssFontFormat.collection; }
        if (eqCi(s, "svg")) { return CssFontFormat.svg; }
        return CssFontFormat.unknown;
    }

    fn asKeyword(): []const u8 {
        return switch (_) {
            .woff2 => "woff2",
            .woff => "woff",
            .truetype => "truetype",
            .opentype => "opentype",
            .embedded_opentype => "embedded-opentype",
            .collection => "collection",
            .svg => "svg",
            .unknown => "",
        };
    }
}

/// `font-display` 描述符枚举
pub enum CssFontDisplay: u8 {
    auto = 0,
    block = 1,
    swap = 2,
    fallback = 3,
    optional = 4,

    static fn parse(raw: []const u8): CssFontDisplay! {
        const s = trimWs(raw);
        if (eqCi(s, "auto")) { return CssFontDisplay.auto; }
        if (eqCi(s, "block")) { return CssFontDisplay.block; }
        if (eqCi(s, "swap")) { return CssFontDisplay.swap; }
        if (eqCi(s, "fallback")) { return CssFontDisplay.fallback; }
        if (eqCi(s, "optional")) { return CssFontDisplay.optional; }
        return CssFontFaceError.InvalidFontDisplay;
    }

    fn asKeyword(): []const u8 {
        return switch (_) {
            .auto => "auto",
            .block => "block",
            .swap => "swap",
            .fallback => "fallback",
            .optional => "optional",
        };
    }
}

/// `src` 条目类型：`url(...)` 或 `local(...)`
pub enum CssFontSourceKind: u8 {
    url = 0,
    local = 1,
}

/// 连续 Unicode 码点区间 `[start, end]`（精准对齐 Bun `UnicodeRange` 含 `?` 通配压缩）
pub struct CssUnicodeRange {
    start: u32 = 0,
    end: u32 = 0,

    /// 解析 W3C `<urange>`：`U+0041`、`U+0025-00FF`、`U+4??`、`U+??`
    static fn parse(raw: []const u8): CssUnicodeRange! {
        const s = trimWs(raw);
        if (s.len < 3) {
            return CssFontFaceError.InvalidUnicodeRange;
        }
        if ((s[0] != 'U' && s[0] != 'u') || s[1] != '+') {
            return CssFontFaceError.InvalidUnicodeRange;
        }

        const body = s[2..];
        let start_val: u32 = 0;
        let end_val: u32 = 0;

        // 解析第一段（最多 6 位十六进制 + 可选尾随 `?`）
        let i: usize = 0;
        let hex_digits: usize = 0;
        let q_digits: usize = 0;
        while (i < body.len && body[i] != '-') {
            const c = body[i];
            if (c == '?') {
                q_digits += 1;
                start_val = (start_val << 4);
                end_val = (end_val << 4) | 0x0f;
            } else {
                if (q_digits > 0) {
                    // `?` 之后不允许再出现十六进制数字
                    return CssFontFaceError.InvalidUnicodeRange;
                }
                const d = hexDigit(c) ?? return CssFontFaceError.InvalidUnicodeRange;
                hex_digits += 1;
                start_val = (start_val << 4) | @as(u32, d);
                end_val = start_val;
            }
            if (hex_digits + q_digits > 6) {
                return CssFontFaceError.InvalidUnicodeRange;
            }
            i += 1;
        }

        if (hex_digits + q_digits == 0) {
            return CssFontFaceError.InvalidUnicodeRange;
        }

        if (i < body.len && body[i] == '-') {
            // 通配 `?` 与显式范围 `-` 不可混用
            if (q_digits > 0) {
                return CssFontFaceError.InvalidUnicodeRange;
            }
            const rest = body[i + 1..];
            if (rest.len == 0 || rest.len > 6) {
                return CssFontFaceError.InvalidUnicodeRange;
            }
            let ev: u32 = 0;
            let k: usize = 0;
            while (k < rest.len) {
                const d = hexDigit(rest[k]) ?? return CssFontFaceError.InvalidUnicodeRange;
                ev = (ev << 4) | @as(u32, d);
                k += 1;
            }
            end_val = ev;
        }

        if (start_val > CSS_UNICODE_MAX_CODEPOINT || end_val > CSS_UNICODE_MAX_CODEPOINT) {
            return CssFontFaceError.CodePointOutOfRange;
        }
        if (start_val > end_val) {
            return CssFontFaceError.ReversedUnicodeRange;
        }

        return CssUnicodeRange{ start: start_val, end: end_val };
    }

    fn contains(cp: u32): bool {
        return cp >= _.start && cp <= _.end;
    }

    fn eql(other: CssUnicodeRange): bool {
        return _.start == other.start && _.end == other.end;
    }

    /// 规范序列化为 CSS `<urange>`，自动将对齐的 16 进制区间压缩为 `?` 语法（如 `U+0000-00FF` -> `U+??`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "U+")?;

        if (_.start != _.end) {
            // 查找 start 与 end 首个相异的 4-bit 十六进制位
            let shift: u5 = 24;
            let mask: u32 = @as(u32, 0xf) << shift;
            while (shift > 0) {
                if ((_.start & mask) != (_.end & mask)) {
                    break;
                }
                mask = mask >> 4;
                shift -= 4;
            }

            const q_bits: u5 = shift + 4;
            if (q_bits <= 24) {
                const remainder_mask: u32 = (@as(u32, 1) << q_bits) - @as(u32, 1);
                if ((_.start & remainder_mask) == 0 && (_.end & remainder_mask) == remainder_mask) {
                    const prefix = (_.start & ~remainder_mask) >> q_bits;
                    if (prefix != 0) {
                        pos = appendHexLower(out, pos, prefix)?;
                    }
                    let rem = q_bits;
                    while (rem > 0) {
                        pos = appendByte(out, pos, '?')?;
                        rem -= 4;
                    }
                    return pos;
                }
            }
        }

        pos = appendHexLower(out, pos, _.start)?;
        if (_.end != _.start) {
            pos = appendByte(out, pos, '-')?;
            pos = appendHexLower(out, pos, _.end)?;
        }
        return pos;
    }
}

/// 单条 `@font-face` `src` 来源条目（`local(...)` 或 `url(...) [format(...)] [tech(...)]`）
pub struct CssFontFaceSource {
    kind: CssFontSourceKind = CssFontSourceKind.url,
    value: []const u8 = "",
    format: CssFontFormat = CssFontFormat.unknown,
    tech: []const u8 = "",

    static fn parse(raw: []const u8): CssFontFaceSource! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return CssFontFaceError.InvalidSource;
        }

        if (startsWithCi(s, "local(")) {
            const close = findMatchingParen(s, 5) ?? return CssFontFaceError.InvalidSource;
            const inner = stripQuotes(trimWs(s[6..close]));
            if (inner.len == 0) {
                return CssFontFaceError.InvalidSource;
            }
            return CssFontFaceSource{
                kind: CssFontSourceKind.local,
                value: inner,
            };
        }

        if (!startsWithCi(s, "url(")) {
            return CssFontFaceError.InvalidSource;
        }
        const url_close = findMatchingParen(s, 3) ?? return CssFontFaceError.InvalidSource;
        const url_val = stripQuotes(trimWs(s[4..url_close]));
        if (url_val.len == 0) {
            return CssFontFaceError.InvalidSource;
        }

        let src = CssFontFaceSource{
            kind: CssFontSourceKind.url,
            value: url_val,
        };

        let rest = trimWs(s[url_close + 1..]);
        while (rest.len > 0) {
            if (startsWithCi(rest, "format(")) {
                const close = findMatchingParen(rest, 6) ?? return CssFontFaceError.InvalidSource;
                src.format = CssFontFormat.parse(rest[7..close]);
                rest = trimWs(rest[close + 1..]);
            } else if (startsWithCi(rest, "tech(")) {
                const close = findMatchingParen(rest, 4) ?? return CssFontFaceError.InvalidSource;
                src.tech = trimWs(rest[5..close]);
                rest = trimWs(rest[close + 1..]);
            } else {
                return CssFontFaceError.InvalidSource;
            }
        }

        return src;
    }

    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        switch (_.kind) {
            .local => {
                pos = appendBytes(out, pos, "local(\"")?;
                pos = appendBytes(out, pos, _.value)?;
                pos = appendBytes(out, pos, "\")")?;
            },
            .url => {
                pos = appendBytes(out, pos, "url(\"")?;
                pos = appendBytes(out, pos, _.value)?;
                pos = appendBytes(out, pos, "\")")?;
                if (_.format != CssFontFormat.unknown) {
                    pos = appendBytes(out, pos, " format(\"")?;
                    pos = appendBytes(out, pos, _.format.asKeyword())?;
                    pos = appendBytes(out, pos, "\")")?;
                }
                if (_.tech.len > 0) {
                    pos = appendBytes(out, pos, " tech(")?;
                    pos = appendBytes(out, pos, _.tech)?;
                    pos = appendByte(out, pos, ')')?;
                }
            },
        }
        return pos;
    }
}

/// 零堆分配 `@font-face` 规则结构
pub struct CssFontFaceRule {
    font_family: []const u8 = "",
    font_display: CssFontDisplay = CssFontDisplay.auto,
    has_font_display: bool = false,
    font_weight: []const u8 = "",
    font_style: []const u8 = "",
    sources: [CSS_FONT_FACE_MAX_SOURCES]CssFontFaceSource = [CssFontFaceSource{}; CSS_FONT_FACE_MAX_SOURCES],
    source_count: usize = 0,
    unicode_ranges: [CSS_FONT_FACE_MAX_RANGES]CssUnicodeRange = [CssUnicodeRange{}; CSS_FONT_FACE_MAX_RANGES],
    range_count: usize = 0,

    /// 解析 `@font-face { ... }` 规则文本
    static fn parse(rule_css: []const u8): CssFontFaceRule! {
        const s = trimWs(rule_css);
        if (!startsWithCi(s, "@font-face")) {
            return CssFontFaceError.InvalidRuleSyntax;
        }
        const after_kw = trimWs(s[10..]);
        if (after_kw.len < 2 || after_kw[0] != '{' || after_kw[after_kw.len - 1] != '}') {
            return CssFontFaceError.InvalidRuleSyntax;
        }

        let rule = CssFontFaceRule{};
        const body = after_kw[1..after_kw.len - 1];

        let i: usize = 0;
        while (i < body.len) {
            const decl_start = i;
            let depth: usize = 0;
            let quote: u8 = 0;
            while (i < body.len) {
                const c = body[i];
                if (quote != 0) {
                    if (c == '\\' && i + 1 < body.len) {
                        i += 2;
                        continue;
                    }
                    if (c == quote) {
                        quote = 0;
                    }
                } else {
                    if (c == '"' || c == '\'') {
                        quote = c;
                    } else if (c == '(') {
                        depth += 1;
                    } else if (c == ')' && depth > 0) {
                        depth -= 1;
                    } else if (c == ';' && depth == 0) {
                        break;
                    }
                }
                i += 1;
            }
            const decl = trimWs(body[decl_start..i]);
            if (i < body.len && body[i] == ';') {
                i += 1;
            }
            if (decl.len == 0) {
                continue;
            }

            const colon = indexOfByte(decl, ':') ?? return CssFontFaceError.InvalidRuleSyntax;
            const name = trimWs(decl[0..colon]);
            const val = trimWs(decl[colon + 1..]);

            if (eqCi(name, "font-family")) {
                const fam = stripQuotes(val);
                if (fam.len == 0) {
                    return CssFontFaceError.EmptyFontFamily;
                }
                rule.font_family = fam;
            } else if (eqCi(name, "font-display")) {
                rule.font_display = CssFontDisplay.parse(val)?;
                rule.has_font_display = true;
            } else if (eqCi(name, "font-weight")) {
                rule.font_weight = val;
            } else if (eqCi(name, "font-style")) {
                rule.font_style = val;
            } else if (eqCi(name, "src")) {
                parseCommaSources(val, &rule)?;
            } else if (eqCi(name, "unicode-range")) {
                parseCommaRanges(val, &rule)?;
            }
        }

        return rule;
    }

    fn sourceAt(idx: usize): CssFontFaceSource? {
        if (idx >= _.source_count) {
            return null;
        }
        return _.sources[idx];
    }

    fn rangeAt(idx: usize): CssUnicodeRange? {
        if (idx >= _.range_count) {
            return null;
        }
        return _.unicode_ranges[idx];
    }

    /// 判断指定 Unicode 码点是否落入本 `@font-face` 的 `unicode-range` 覆盖范围内
    fn matchesCodePoint(cp: u32): bool {
        if (cp > CSS_UNICODE_MAX_CODEPOINT) {
            return false;
        }
        if (_.range_count == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.range_count) {
            if (_.unicode_ranges[i].contains(cp)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 规范序列化为 `@font-face { ... }` CSS 文本
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "@font-face { ")?;
        if (_.font_family.len > 0) {
            pos = appendBytes(out, pos, "font-family: \"")?;
            pos = appendBytes(out, pos, _.font_family)?;
            pos = appendBytes(out, pos, "\"; ")?;
        }
        if (_.source_count > 0) {
            pos = appendBytes(out, pos, "src: ")?;
            let i: usize = 0;
            while (i < _.source_count) {
                if (i > 0) {
                    pos = appendBytes(out, pos, ", ")?;
                }
                const sn = _.sources[i].toCss(out[pos..])?;
                pos += sn;
                i += 1;
            }
            pos = appendBytes(out, pos, "; ")?;
        }
        if (_.font_weight.len > 0) {
            pos = appendBytes(out, pos, "font-weight: ")?;
            pos = appendBytes(out, pos, _.font_weight)?;
            pos = appendBytes(out, pos, "; ")?;
        }
        if (_.font_style.len > 0) {
            pos = appendBytes(out, pos, "font-style: ")?;
            pos = appendBytes(out, pos, _.font_style)?;
            pos = appendBytes(out, pos, "; ")?;
        }
        if (_.has_font_display) {
            pos = appendBytes(out, pos, "font-display: ")?;
            pos = appendBytes(out, pos, _.font_display.asKeyword())?;
            pos = appendBytes(out, pos, "; ")?;
        }
        if (_.range_count > 0) {
            pos = appendBytes(out, pos, "unicode-range: ")?;
            let k: usize = 0;
            while (k < _.range_count) {
                if (k > 0) {
                    pos = appendBytes(out, pos, ", ")?;
                }
                const rn = _.unicode_ranges[k].toCss(out[pos..])?;
                pos += rn;
                k += 1;
            }
            pos = appendBytes(out, pos, "; ")?;
        }
        pos = appendByte(out, pos, '}')?;
        return pos;
    }
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

fn parseCommaSources(val: []const u8, rule: *CssFontFaceRule)! {
    let i: usize = 0;
    while (i < val.len) {
        const start = i;
        let depth: usize = 0;
        let quote: u8 = 0;
        while (i < val.len) {
            const c = val[i];
            if (quote != 0) {
                if (c == '\\' && i + 1 < val.len) {
                    i += 2;
                    continue;
                }
                if (c == quote) {
                    quote = 0;
                }
            } else {
                if (c == '"' || c == '\'') {
                    quote = c;
                } else if (c == '(') {
                    depth += 1;
                } else if (c == ')' && depth > 0) {
                    depth -= 1;
                } else if (c == ',' && depth == 0) {
                    break;
                }
            }
            i += 1;
        }
        const part = trimWs(val[start..i]);
        if (i < val.len && val[i] == ',') {
            i += 1;
        }
        if (part.len == 0) {
            return CssFontFaceError.InvalidSource;
        }
        if (rule.source_count >= CSS_FONT_FACE_MAX_SOURCES) {
            return CssFontFaceError.TooManySources;
        }
        rule.sources[rule.source_count] = CssFontFaceSource.parse(part)?;
        rule.source_count += 1;
    }
}

fn parseCommaRanges(val: []const u8, rule: *CssFontFaceRule)! {
    let i: usize = 0;
    while (i < val.len) {
        const start = i;
        while (i < val.len && val[i] != ',') {
            i += 1;
        }
        const part = trimWs(val[start..i]);
        if (i < val.len && val[i] == ',') {
            i += 1;
        }
        if (part.len == 0) {
            return CssFontFaceError.InvalidUnicodeRange;
        }
        if (rule.range_count >= CSS_FONT_FACE_MAX_RANGES) {
            return CssFontFaceError.TooManyRanges;
        }
        rule.unicode_ranges[rule.range_count] = CssUnicodeRange.parse(part)?;
        rule.range_count += 1;
    }
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn stripQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\''))) {
        return s[1..s.len - 1];
    }
    return s;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqCi(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn startsWithCi(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqCi(s[0..prefix.len], prefix);
}

fn indexOfByte(s: []const u8, needle: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findMatchingParen(s: []const u8, open_idx: usize): usize? {
    if (open_idx >= s.len || s[open_idx] != '(') {
        return null;
    }
    let depth: usize = 0;
    let quote: u8 = 0;
    let i = open_idx;
    while (i < s.len) {
        const c = s[i];
        if (quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                depth -= 1;
                if (depth == 0) {
                    return i;
                }
            }
        }
        i += 1;
    }
    return null;
}

fn hexDigit(c: u8): u8? {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return null;
}

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return CssFontFaceError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return CssFontFaceError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn appendHexLower(out: []u8, pos_: usize, val: u32): usize! {
    let pos = pos_;
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    const hex = "0123456789abcdef";
    let tmp: [8]u8 = [0; 8];
    let n: usize = 0;
    let v = val;
    while (v > 0) {
        tmp[n] = hex[@as(usize, @intCast(v & 0x0f))];
        v >>= 4;
        n += 1;
    }
    while (n > 0) {
        n -= 1;
        pos = appendByte(out, pos, tmp[n])?;
    }
    return pos;
}

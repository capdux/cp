// Source: Absorbed from /data/dev/bun/src/css/values/url.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation W3C CSS Values Level 4 `url(...)` parser, RFC 3986 / WHATWG
// URL classification (`isAbsolute` / `kind`), and shortest-form minified CSS serializer.

/// CSS `url(...)` 解析与序列化错误集
pub err CssUrlError {
    InvalidUrl,
    UnclosedQuote,
    UnclosedParen,
    UnescapedSpecialChar,
    BufferTooSmall,
}

/// CSS `url(...)` 路径分类
pub enum CssUrlKind: u8 {
    Relative = 0,
    RootRelative = 1,
    Fragment = 2,
    Data = 3,
    AbsoluteScheme = 4,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isSchemeChar(c: u8): bool {
    return isAsciiAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
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

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn needsUnquotedEscape(c: u8): bool {
    return isWs(c) || c == '"' || c == '\'' || c == '(' || c == ')' || c == '\\' || c <= 0x1F || c == 0x7F;
}

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 统计序列化为带双引号 `url("...")` 的精确字节长度
fn quotedSerializedLen(raw: []const u8): usize {
    // `url("` (5) + `")` (2) = 7
    let total: usize = 7;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == '"' || c == '\\' || c == '\n' || c == '\r') {
            total += 2;
        } else {
            total += 1;
        }
        i += 1;
    }
    return total;
}

/// 统计序列化为无引号 `url(...)`（对特殊字符反斜杠转义）的精确字节长度
fn unquotedSerializedLen(raw: []const u8): usize {
    // `url(` (4) + `)` (1) = 5
    let total: usize = 5;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (needsUnquotedEscape(c)) {
            total += 2;
        } else {
            total += 1;
        }
        i += 1;
    }
    return total;
}

/// W3C CSS Values Level 4 `url(...)` 值（对齐 Bun `src/css/values/url.cp`）
pub struct CssUrlValue {
    /// 去除外层 `url(...)` 及可选引号后的 URL 内容切片
    raw: []const u8 = "",
    /// 源文本中是否使用了引号包裹（`url("...")` 或 `url('...')`）
    was_quoted: bool = false,

    /// 解析 CSS `url(...)` 表达式（大小写不敏感，支持单/双引号与无引号形式）
    static fn parse(input: []const u8): CssUrlValue! {
        const s = trimWs(input);
        if (s.len < 5 || !eqlAsciiIgnoreCase(s[0..4], "url(")) {
            return err.InvalidUrl;
        }
        if (s[s.len - 1] != ')') {
            return err.UnclosedParen;
        }
        const inner = trimWs(s[4 .. s.len - 1]);
        if (inner.len == 0) {
            return CssUrlValue{ raw: "", was_quoted: false };
        }

        const first = inner[0];
        if (first == '"' || first == '\'') {
            if (inner.len < 2 || inner[inner.len - 1] != first) {
                return err.UnclosedQuote;
            }
            // 校验内部不存在未转义的同种引号
            let i: usize = 1;
            while (i + 1 < inner.len) {
                if (inner[i] == '\\' && i + 2 < inner.len) {
                    i += 2;
                    continue;
                }
                if (inner[i] == first) {
                    return err.UnescapedSpecialChar;
                }
                i += 1;
            }
            return CssUrlValue{
                raw: inner[1 .. inner.len - 1],
                was_quoted: true,
            };
        }

        // 无引号 URL 严禁包含未转义空白、引号、括号或控制字符
        let i: usize = 0;
        while (i < inner.len) {
            const c = inner[i];
            if (c == '\\' && i + 1 < inner.len) {
                i += 2;
                continue;
            }
            if (needsUnquotedEscape(c)) {
                return err.UnescapedSpecialChar;
            }
            i += 1;
        }
        return CssUrlValue{
            raw: inner,
            was_quoted: false,
        };
    }

    /// 对 URL 进行语义分类（对齐 WHATWG URL scheme 判定与 CSS Values Level 4 local URL 规则）
    fn kind(): CssUrlKind {
        const u = _.raw;
        if (u.len == 0) {
            return CssUrlKind.Relative;
        }
        if (u[0] == '.') {
            return CssUrlKind.Relative;
        }
        if (u[0] == '/') {
            return CssUrlKind.RootRelative;
        }
        if (u[0] == '#') {
            return CssUrlKind.Fragment;
        }
        if (u.len >= 5 && eqlAsciiIgnoreCase(u[0..5], "data:")) {
            return CssUrlKind.Data;
        }
        // RFC 3986 / WHATWG scheme-start-state: ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) ":"
        if (!isAsciiAlpha(u[0])) {
            return CssUrlKind.Relative;
        }
        let i: usize = 1;
        while (i < u.len) {
            if (u[i] == ':') {
                return CssUrlKind.AbsoluteScheme;
            }
            if (!isSchemeChar(u[i])) {
                break;
            }
            i += 1;
        }
        return CssUrlKind.Relative;
    }

    /// 是否为绝对 URL（对齐 Bun `Url.isAbsolute()`：`/`、`#`、`data:` 或任意合法 scheme 返回 `true`）
    fn isAbsolute(): bool {
        return _.kind() != CssUrlKind.Relative;
    }

    fn isRelative(): bool {
        return _.kind() == CssUrlKind.Relative;
    }

    fn isFragment(): bool {
        return _.kind() == CssUrlKind.Fragment;
    }

    fn isDataUrl(): bool {
        return _.kind() == CssUrlKind.Data;
    }

    /// 判断无引号输出时是否包含需转义字符
    fn needsQuoting(): bool {
        let i: usize = 0;
        while (i < _.raw.len) {
            if (needsUnquotedEscape(_.raw[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn eql(other: CssUrlValue): bool {
        return bytesEql(_.raw, other.raw);
    }

    /// 标准带双引号序列化 `url("...")`（适合依赖占位符安全替换，对齐 Bun `Url.toCss` 非压缩模式）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "url(\"")?;
        let i: usize = 0;
        while (i < _.raw.len) {
            const c = _.raw[i];
            if (c == '"' || c == '\\') {
                pos = appendChar(out, pos, '\\')?;
                pos = appendChar(out, pos, c)?;
            } else {
                pos = appendChar(out, pos, c)?;
            }
            i += 1;
        }
        pos = appendBytes(out, pos, "\")")?;
        return pos;
    }

    /// 最短化压缩序列化（对齐 Bun `Url.toCss` minify 模式：在无引号转义与带引号形式间自动择短）
    fn toCssMinified(out: []u8): usize! {
        const q_len = quotedSerializedLen(_.raw);
        const u_len = unquotedSerializedLen(_.raw);
        if (q_len < u_len) {
            return _.toCss(out);
        }
        let pos: usize = 0;
        pos = appendBytes(out, pos, "url(")?;
        let i: usize = 0;
        while (i < _.raw.len) {
            const c = _.raw[i];
            if (needsUnquotedEscape(c)) {
                pos = appendChar(out, pos, '\\')?;
                pos = appendChar(out, pos, c)?;
            } else {
                pos = appendChar(out, pos, c)?;
            }
            i += 1;
        }
        pos = appendChar(out, pos, ')')?;
        return pos;
    }
}

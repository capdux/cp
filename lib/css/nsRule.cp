// Source: Absorbed from /data/dev/bun/src/css/rules/namespace.cp | Synced: 2026-11-22T12:14:02Z
// Pure-CP zero-allocation W3C CSS Namespaces Module Level 3 `@namespace` rule parser,
// serializer, and fixed-capacity default/prefixed namespace registry.

/// `@namespace` 注册表最大具名前缀条目数
pub const CSS_NAMESPACE_MAX_RULES: usize = 32;
/// W3C HTML 标准命名空间 URI
pub const CSS_HTML_NS: []const u8 = "http://www.w3.org/1999/xhtml";
/// W3C SVG 标准命名空间 URI
pub const CSS_SVG_NS: []const u8 = "http://www.w3.org/2000/svg";
/// W3C MathML 标准命名空间 URI
pub const CSS_MATHML_NS: []const u8 = "http://www.w3.org/1998/Math/MathML";

/// CSS `@namespace` 规则解析、序列化与注册表错误集
pub err CssNamespaceError {
    InvalidPrelude,
    InvalidPrefix,
    InvalidUrl,
    BufferTooSmall,
    RegistryFull,
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

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_' || c == '-' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isValidNsPrefix(s: []const u8): bool {
    if (s.len == 0 || !isIdentStart(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isIdentChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseQuotedString(s: []const u8): ([]const u8)? {
    if (s.len < 2) {
        return null;
    }
    const q = s[0];
    if (q != '"' && q != '\'') {
        return null;
    }
    if (s[s.len - 1] != q) {
        return null;
    }
    return s[1 .. s.len - 1];
}

fn parseCssUrlOrString(raw: []const u8): ([]const u8)! {
    const s = trimWs(raw);
    if (s.len == 0) {
        return err.InvalidUrl;
    }
    if (quoted := parseQuotedString(s)) {
        return quoted;
    }
    if (s.len >= 5 && eqlAsciiIgnoreCase(s[0..4], "url(") && s[s.len - 1] == ')') {
        const inner = trimWs(s[4 .. s.len - 1]);
        if (inner.len == 0) {
            return err.InvalidUrl;
        }
        if (quoted := parseQuotedString(inner)) {
            return quoted;
        }
        // Unquoted url(...) must not contain unescaped whitespace or quotes
        let i: usize = 0;
        while (i < inner.len) {
            const c = inner[i];
            if (isWs(c) || c == '"' || c == '\'' || c == '(' || c == ')') {
                return err.InvalidUrl;
            }
            i += 1;
        }
        return inner;
    }
    return err.InvalidUrl;
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

/// CSS `@namespace` 规则（对齐 Bun `NamespaceRule`）
pub struct NamespaceRule {
    /// 可选命名空间前缀（`null` 表示声明默认命名空间）
    prefix: ([]const u8)? = null,
    /// 命名空间 URI（已剥离外层引号或 `url(...)`）
    url: []const u8 = "",

    /// 解析 `@namespace [<prefix>] (<string> | url(<url>));`（尾部 `;` 可选）
    static fn parse(rule_text: []const u8): NamespaceRule! {
        let s = trimWs(rule_text);
        if (s.len > 0 && s[s.len - 1] == ';') {
            s = trimWs(s[0 .. s.len - 1]);
        }
        const kw = "@namespace";
        if (s.len <= kw.len || !eqlAsciiIgnoreCase(s[0..kw.len], kw)) {
            return err.InvalidPrelude;
        }
        if (!isWs(s[kw.len])) {
            return err.InvalidPrelude;
        }
        const rest = trimWs(s[kw.len..]);
        if (rest.len == 0) {
            return err.InvalidUrl;
        }

        // Case 1: direct string or url(...) -> default namespace (no prefix)
        const first = rest[0];
        if (first == '"' || first == '\'' || (rest.len >= 4 && eqlAsciiIgnoreCase(rest[0..4], "url("))) {
            const url = parseCssUrlOrString(rest)?;
            return NamespaceRule{
                prefix: null,
                url: url,
            };
        }

        // Case 2: `<prefix> <string|url(...)>`
        let sp: usize = 0;
        while (sp < rest.len && !isWs(rest[sp])) {
            sp += 1;
        }
        const prefix = rest[0..sp];
        if (!isValidNsPrefix(prefix)) {
            return err.InvalidPrefix;
        }
        const url_part = trimWs(rest[sp..]);
        if (url_part.len == 0) {
            return err.InvalidUrl;
        }
        const url = parseCssUrlOrString(url_part)?;
        return NamespaceRule{
            prefix: prefix,
            url: url,
        };
    }

    /// 判断是否为默认命名空间声明（无前缀）
    fn isDefault(): bool {
        return _.prefix == null;
    }

    /// 序列化为规范 CSS 字符串 `@namespace [prefix ]"<url>";`（对齐 Bun `toCss`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "@namespace ")?;
        if (p := _.prefix) {
            pos = appendBytes(out, pos, p)?;
            pos = appendBytes(out, pos, " ")?;
        }
        pos = appendBytes(out, pos, "\"")?;
        pos = appendBytes(out, pos, _.url)?;
        pos = appendBytes(out, pos, "\";")?;
        return pos;
    }
}

/// 零堆分配 CSS `@namespace` 规则注册表（支持默认命名空间与具名前缀查找）
pub struct NamespaceRegistry {
    default_url: ([]const u8)? = null,
    entries: [CSS_NAMESPACE_MAX_RULES]NamespaceRule = [NamespaceRule{}; CSS_NAMESPACE_MAX_RULES],
    count: usize = 0,

    /// 初始化空注册表
    static fn init(): NamespaceRegistry {
        return NamespaceRegistry{
            default_url: null,
            entries: [NamespaceRule{}; CSS_NAMESPACE_MAX_RULES],
            count: 0,
        };
    }

    /// 注册或覆盖一条 `@namespace` 规则；默认命名空间返回 `0`，具名前缀返回槽位索引
    fn register(*_, rule: NamespaceRule): usize! {
        if (p := rule.prefix) {
            let i: usize = 0;
            while (i < _.count) {
                if (existing := _.entries[i].prefix) {
                    if (bytesEql(existing, p)) {
                        _.entries[i] = rule;
                        return i;
                    }
                }
                i += 1;
            }
            if (_.count >= CSS_NAMESPACE_MAX_RULES) {
                return err.RegistryFull;
            }
            const slot = _.count;
            _.entries[slot] = rule;
            _.count += 1;
            return slot;
        }
        _.default_url = rule.url;
        return 0;
    }

    /// 按具名前缀查找对应命名空间 URI
    fn lookupPrefix(prefix: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) {
            if (existing := _.entries[i].prefix) {
                if (bytesEql(existing, prefix)) {
                    return _.entries[i].url;
                }
            }
            i += 1;
        }
        return null;
    }

    /// 按 CSS 选择器命名空间前缀语义解析 URI：
    /// `""` -> 默认命名空间，` "*" ` -> `"*"`（通配），其余按具名前缀查表
    fn resolveSelectorPrefix(selector_ns: []const u8): ([]const u8)? {
        if (selector_ns.len == 0) {
            return _.default_url;
        }
        if (bytesEql(selector_ns, "*")) {
            return "*";
        }
        return _.lookupPrefix(selector_ns);
    }
}

// Source: Absorbed from /data/dev/bun/src/css/rules/document.cp | Synced: 2026-11-22T12:01:00Z
// Pure-CP zero-allocation W3C CSS Conditional Rules Level 3 (`@-moz-document`) rule parser,
// strict Firefox/Bun `url-prefix()` validator, URL/domain matcher (`url()`, `url-prefix()`,
// `domain()`, `media-document()`, `regexp()`), canonical & minified serializer, and
// stylesheet extractor.

/// 单条 `@-moz-document` 规则最多支持的逗号分隔条件函数数
pub const CSS_DOCUMENT_MAX_CONDITIONS: usize = 16;

/// 样式表批量提取 `@-moz-document` 规则的最大条目数
pub const CSS_DOCUMENT_MAX_RULES: usize = 16;

/// `@-moz-document` 规则解析、校验与序列化错误集
pub err CssDocumentRuleError {
    InvalidAtRule,
    UnsupportedPrefix,
    EmptyConditions,
    InvalidCondition,
    NonEmptyUrlPrefix,
    UnclosedBlock,
    TrailingContent,
    TooManyConditions,
    TooManyRules,
    BufferTooSmall,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0C;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c >= 0x80;
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

fn startsWithBytes(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn skipWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isWs(s[i])) {
            i += 1;
            continue;
        }
        if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i = s.len;
            }
            continue;
        }
        break;
    }
    return i;
}

fn trimWsAndComments(s: []const u8): []const u8 {
    let start = skipWsAndComments(s, 0);
    let end = s.len;
    while (end > start) {
        if (isWs(s[end - 1])) {
            end -= 1;
            continue;
        }
        if (end >= start + 4 && s[end - 2] == '*' && s[end - 1] == '/') {
            let j = end - 2;
            let found = false;
            while (j >= start + 2) {
                if (s[j - 2] == '/' && s[j - 1] == '*') {
                    end = j - 2;
                    found = true;
                    break;
                }
                j -= 1;
            }
            if (found) {
                continue;
            }
        }
        break;
    }
    return s[start..end];
}

fn findMatchingBrace(s: []const u8, open_pos: usize): usize? {
    if (open_pos >= s.len || s[open_pos] != '{') {
        return null;
    }
    let depth: usize = 0;
    let i = open_pos;
    let in_quote: u8 = 0;
    while (i < s.len) {
        const c = s[i];
        if (in_quote != 0) {
            if (c == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (c == in_quote) {
                in_quote = 0;
            }
            i += 1;
            continue;
        }
        if (c == '/' && i + 1 < s.len && s[i + 1] == '*') {
            i += 2;
            while (i + 1 < s.len && !(s[i] == '*' && s[i + 1] == '/')) {
                i += 1;
            }
            if (i + 1 < s.len) {
                i += 2;
            } else {
                return null;
            }
            continue;
        }
        if (c == '"' || c == '\'') {
            in_quote = c;
            i += 1;
            continue;
        }
        if (c == '{') {
            depth += 1;
        } else if (c == '}') {
            if (depth == 0) {
                return null;
            }
            depth -= 1;
            if (depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn writeByte(out: []u8, pos: *usize, b: u8): usize! {
    if (pos.* >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
    return 1;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

/// 从 URL 中提取不含端口的 host 切片（例如 `https://docs.example.com:8443/a` -> `docs.example.com`）
fn extractUrlHost(url: []const u8): ([]const u8)? {
    let start: usize = 0;
    let i: usize = 0;
    while (i + 2 < url.len) {
        if (url[i] == ':' && url[i + 1] == '/' && url[i + 2] == '/') {
            start = i + 3;
            break;
        }
        i += 1;
    }
    if (start == 0) {
        return null;
    }
    let end = start;
    while (end < url.len && url[end] != '/' && url[end] != '?' && url[end] != '#') {
        end += 1;
    }
    let authority = url[start..end];
    // 跳过 userinfo@
    let at_idx: usize? = null;
    let j: usize = 0;
    while (j < authority.len) {
        if (authority[j] == '@') {
            at_idx = j;
        }
        j += 1;
    }
    if (a := at_idx) {
        authority = authority[a + 1 ..];
    }
    if (authority.len == 0) {
        return null;
    }
    // IPv6 [::1]:8080
    if (authority[0] == '[') {
        let k: usize = 1;
        while (k < authority.len && authority[k] != ']') {
            k += 1;
        }
        if (k < authority.len) {
            return authority[0 .. k + 1];
        }
        return null;
    }
    // 剥离 :port
    let c_idx: usize = 0;
    while (c_idx < authority.len && authority[c_idx] != ':') {
        c_idx += 1;
    }
    const host = authority[0..c_idx];
    if (host.len == 0) {
        return null;
    }
    return host;
}

/// 域名精确或子域名边界匹配（ASCII 大小写无关）
fn domainMatchesHost(host: []const u8, domain: []const u8): bool {
    if (domain.len == 0 || host.len < domain.len) {
        return false;
    }
    if (host.len == domain.len) {
        return eqlAsciiIgnoreCase(host, domain);
    }
    const diff = host.len - domain.len;
    if (host[diff - 1] != '.') {
        return false;
    }
    return eqlAsciiIgnoreCase(host[diff..], domain);
}

/// `@-moz-document` 条件函数类型
pub enum CssDocumentMatchKind: u8 {
    Url = 0,
    UrlPrefix = 1,
    Domain = 2,
    MediaDocument = 3,
    Regexp = 4,

    static fn fromFuncName(fn_name: []const u8): CssDocumentMatchKind? {
        if (eqlAsciiIgnoreCase(fn_name, "url")) return .Url;
        if (eqlAsciiIgnoreCase(fn_name, "url-prefix")) return .UrlPrefix;
        if (eqlAsciiIgnoreCase(fn_name, "domain")) return .Domain;
        if (eqlAsciiIgnoreCase(fn_name, "media-document")) return .MediaDocument;
        if (eqlAsciiIgnoreCase(fn_name, "regexp")) return .Regexp;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Url => "url",
            .UrlPrefix => "url-prefix",
            .Domain => "domain",
            .MediaDocument => "media-document",
            .Regexp => "regexp",
        };
    }
}

/// 单条 `@-moz-document` URL 匹配条件
pub struct CssDocumentCondition {
    kind: CssDocumentMatchKind = .UrlPrefix,
    value: []const u8 = "",

    /// 判断是否为无参 `url-prefix()`（Firefox/Bun 唯一标准保留形态）
    fn isEmptyUrlPrefix(): bool {
        return _.kind == .UrlPrefix && _.value.len == 0;
    }

    /// 评估给定 URL 是否命中当前条件（`regexp` 返回 `false`，由外部正则引擎处理）
    fn matchesUrl(url: []const u8): bool {
        return switch (_.kind) {
            .Url => bytesEql(url, _.value),
            .UrlPrefix => startsWithBytes(url, _.value),
            .Domain => {
                if (host := extractUrlHost(url)) {
                    return domainMatchesHost(host, _.value);
                }
                return false;
            },
            .MediaDocument => eqlAsciiIgnoreCase(_.value, "all"),
            .Regexp => false,
        };
    }

    fn eql(other: CssDocumentCondition): bool {
        return _.kind == other.kind && bytesEql(_.value, other.value);
    }

    fn writeCss(out: []u8, pos: *usize): usize! {
        const start = pos.*;
        * = writeSlice(out, pos, _.kind.asStr())?;
        * = writeByte(out, pos, '(')?;
        if (_.value.len > 0) {
            * = writeByte(out, pos, '"')?;
            * = writeSlice(out, pos, _.value)?;
            * = writeByte(out, pos, '"')?;
        }
        * = writeByte(out, pos, ')')?;
        return pos.* - start;
    }
}

fn unquoteArg(raw: []const u8): ([]const u8)! {
    const s = trimWsAndComments(raw);
    if (s.len == 0) {
        return "";
    }
    const first = s[0];
    if (first == '"' || first == '\'') {
        if (s.len < 2 || s[s.len - 1] != first) {
            return err.InvalidCondition;
        }
        return s[1 .. s.len - 1];
    }
    // 无引号参数内不允许出现空白或未配对引号
    let i: usize = 0;
    while (i < s.len) {
        if (isWs(s[i]) || s[i] == '"' || s[i] == '\'') {
            return err.InvalidCondition;
        }
        i += 1;
    }
    return s;
}

fn parseSingleCondition(raw: []const u8): CssDocumentCondition! {
    const s = trimWsAndComments(raw);
    if (s.len == 0) {
        return err.InvalidCondition;
    }
    let open_paren: usize = 0;
    while (open_paren < s.len && isIdentChar(s[open_paren])) {
        open_paren += 1;
    }
    if (open_paren == 0 || open_paren >= s.len || s[open_paren] != '(' || s[s.len - 1] != ')') {
        return err.InvalidCondition;
    }
    const fn_name = s[0..open_paren];
    const kind = CssDocumentMatchKind.fromFuncName(fn_name) ?? return err.InvalidCondition;
    const arg_raw = s[open_paren + 1 .. s.len - 1];
    const val = unquoteArg(arg_raw)?;

    // `regexp(...)` 必须提供引号字符串参数且非空
    if (kind == .Regexp) {
        const trimmed_arg = trimWsAndComments(arg_raw);
        if (trimmed_arg.len < 2 || (trimmed_arg[0] != '"' && trimmed_arg[0] != '\'') || val.len == 0) {
            return err.InvalidCondition;
        }
    } else if (kind == .Url || kind == .Domain || kind == .MediaDocument) {
        if (val.len == 0) {
            return err.InvalidCondition;
        }
    }

    return CssDocumentCondition{
        kind: kind,
        value: val,
    };
}

/// `@-moz-document` 条件文档规则模型（对齐 Bun `MozDocumentRule`）
pub struct CssMozDocumentRule {
    conditions: [CSS_DOCUMENT_MAX_CONDITIONS]CssDocumentCondition = [CssDocumentCondition{}; CSS_DOCUMENT_MAX_CONDITIONS],
    cond_count: usize = 0,
    body: []const u8 = "",

    /// 构造标准无参 `@-moz-document url-prefix() { <body> }`（对齐 Bun `MozDocumentRule` 默认形态）
    static fn urlPrefixEmpty(body: []const u8): CssMozDocumentRule {
        let rule = CssMozDocumentRule{
            conditions: [CssDocumentCondition{}; CSS_DOCUMENT_MAX_CONDITIONS],
            cond_count: 1,
            body: trimWsAndComments(body),
        };
        rule.conditions[0] = CssDocumentCondition{
            kind: .UrlPrefix,
            value: "",
        };
        return rule;
    }

    /// 完整解析 `@-moz-document <cond1>, <cond2>, ... { <body> }`
    static fn parse(input: []const u8): CssMozDocumentRule! {
        let pos = skipWsAndComments(input, 0);
        if (pos >= input.len || input[pos] != '@') {
            return err.InvalidAtRule;
        }
        pos += 1;
        const kw_start = pos;
        while (pos < input.len && isIdentChar(input[pos])) {
            pos += 1;
        }
        const kw = input[kw_start..pos];
        if (!eqlAsciiIgnoreCase(kw, "-moz-document")) {
            if (eqlAsciiIgnoreCase(kw, "document") ||
                (kw.len > 9 && eqlAsciiIgnoreCase(kw[kw.len - 9 ..], "-document")))
            {
                return err.UnsupportedPrefix;
            }
            return err.InvalidAtRule;
        }

        // 查找顶层 '{'
        let open_brace: usize? = null;
        let scan = pos;
        let paren: usize = 0;
        let in_quote: u8 = 0;
        while (scan < input.len) {
            const c = input[scan];
            if (in_quote != 0) {
                if (c == '\\' && scan + 1 < input.len) {
                    scan += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                scan += 1;
                continue;
            }
            if (c == '/' && scan + 1 < input.len && input[scan + 1] == '*') {
                scan += 2;
                while (scan + 1 < input.len && !(input[scan] == '*' && input[scan + 1] == '/')) {
                    scan += 1;
                }
                if (scan + 1 < input.len) {
                    scan += 2;
                } else {
                    return err.UnclosedBlock;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '(') {
                paren += 1;
            } else if (c == ')') {
                if (paren == 0) {
                    return err.InvalidCondition;
                }
                paren -= 1;
            } else if (c == '{' && paren == 0) {
                open_brace = scan;
                break;
            }
            scan += 1;
        }
        if (in_quote != 0 || paren != 0) {
            return err.InvalidCondition;
        }
        const ob = open_brace ?? return err.UnclosedBlock;
        const cb = findMatchingBrace(input, ob) ?? return err.UnclosedBlock;
        const tail = skipWsAndComments(input, cb + 1);
        if (tail < input.len) {
            return err.TrailingContent;
        }

        const prelude = trimWsAndComments(input[pos..ob]);
        if (prelude.len == 0) {
            return err.EmptyConditions;
        }

        let rule = CssMozDocumentRule{
            conditions: [CssDocumentCondition{}; CSS_DOCUMENT_MAX_CONDITIONS],
            cond_count: 0,
            body: trimWsAndComments(input[ob + 1 .. cb]),
        };

        // 按顶层逗号切分条件函数
        let i: usize = 0;
        let seg_start: usize = 0;
        let p_depth: usize = 0;
        let q: u8 = 0;
        while (i <= prelude.len) {
            if (i < prelude.len) {
                const c = prelude[i];
                if (q != 0) {
                    if (c == '\\' && i + 1 < prelude.len) {
                        i += 2;
                        continue;
                    }
                    if (c == q) {
                        q = 0;
                    }
                    i += 1;
                    continue;
                }
                if (c == '"' || c == '\'') {
                    q = c;
                    i += 1;
                    continue;
                }
                if (c == '(') {
                    p_depth += 1;
                    i += 1;
                    continue;
                }
                if (c == ')') {
                    if (p_depth == 0) {
                        return err.InvalidCondition;
                    }
                    p_depth -= 1;
                    i += 1;
                    continue;
                }
                if (c != ',' || p_depth != 0) {
                    i += 1;
                    continue;
                }
            }
            const cond = parseSingleCondition(prelude[seg_start..i])?;
            if (rule.cond_count >= CSS_DOCUMENT_MAX_CONDITIONS) {
                return err.TooManyConditions;
            }
            rule.conditions[rule.cond_count] = cond;
            rule.cond_count += 1;
            seg_start = i + 1;
            i += 1;
        }

        if (rule.cond_count == 0) {
            return err.EmptyConditions;
        }
        return rule;
    }

    /// 严格对齐 Bun/LightningCSS 规范解析：仅允许单条无参 `@-moz-document url-prefix() { ... }`
    static fn parseStrict(input: []const u8): CssMozDocumentRule! {
        const rule = CssMozDocumentRule.parse(input)?;
        if (!rule.isFirefoxHack()) {
            return err.NonEmptyUrlPrefix;
        }
        return rule;
    }

    fn conditionCount(): usize {
        return _.cond_count;
    }

    fn conditionAt(idx: usize): CssDocumentCondition? {
        if (idx >= _.cond_count) {
            return null;
        }
        return _.conditions[idx];
    }

    /// 是否恰好为现代 Firefox/Bun 唯一允许的 `@-moz-document url-prefix()` 形态
    fn isFirefoxHack(): bool {
        return _.cond_count == 1 && _.conditions[0].isEmptyUrlPrefix();
    }

    /// 遵循 CSS 逗号 OR 语义：只要任意条件命中 `url` 即返回 `true`
    fn matchesUrl(url: []const u8): bool {
        let i: usize = 0;
        while (i < _.cond_count) {
            if (_.conditions[i].matchesUrl(url)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn eql(other: CssMozDocumentRule): bool {
        if (_.cond_count != other.cond_count || !bytesEql(_.body, other.body)) {
            return false;
        }
        let i: usize = 0;
        while (i < _.cond_count) {
            if (!_.conditions[i].eql(other.conditions[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 美化规范序列化（对齐 Bun `MozDocumentRule.toCss`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, "@-moz-document ")?;
        let i: usize = 0;
        while (i < _.cond_count) {
            if (i > 0) {
                * = writeSlice(out, *pos, ", ")?;
            }
            * = _.conditions[i].writeCss(out, *pos)?;
            i += 1;
        }
        if (_.body.len == 0) {
            * = writeSlice(out, *pos, " {}")?;
        } else {
            * = writeSlice(out, *pos, " {\n  ")?;
            * = writeSlice(out, *pos, _.body)?;
            * = writeSlice(out, *pos, "\n}")?;
        }
        return pos;
    }

    /// 压缩序列化
    fn toCssMinified(out: []u8): usize! {
        let pos: usize = 0;
        * = writeSlice(out, *pos, "@-moz-document ")?;
        let i: usize = 0;
        while (i < _.cond_count) {
            if (i > 0) {
                * = writeByte(out, *pos, ',')?;
            }
            * = _.conditions[i].writeCss(out, *pos)?;
            i += 1;
        }
        * = writeByte(out, *pos, '{')?;
        * = writeSlice(out, *pos, _.body)?;
        * = writeByte(out, *pos, '}')?;
        return pos;
    }
}

/// 从完整样式表批量提取的 `@-moz-document` 规则集合
pub struct CssMozDocumentRuleList {
    rules: [CSS_DOCUMENT_MAX_RULES]CssMozDocumentRule = [CssMozDocumentRule{}; CSS_DOCUMENT_MAX_RULES],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssMozDocumentRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.rules[idx];
    }

    /// 词法安全扫描完整样式表（跳过字符串、注释与普通规则块），提取全部顶层 `@-moz-document` 规则
    static fn extractFromStylesheet(css: []const u8): CssMozDocumentRuleList! {
        let list = CssMozDocumentRuleList{};
        let i: usize = 0;
        let in_quote: u8 = 0;
        let brace_depth: usize = 0;

        while (i < css.len) {
            const c = css[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < css.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '/' && i + 1 < css.len && css[i + 1] == '*') {
                i += 2;
                while (i + 1 < css.len && !(css[i] == '*' && css[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < css.len) {
                    i += 2;
                } else {
                    break;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }
            if (c == '{') {
                brace_depth += 1;
                i += 1;
                continue;
            }
            if (c == '}') {
                if (brace_depth > 0) {
                    brace_depth -= 1;
                }
                i += 1;
                continue;
            }
            if (c == '@' && brace_depth == 0) {
                let k = i + 1;
                while (k < css.len && isIdentChar(css[k])) {
                    k += 1;
                }
                const kw = css[i + 1 .. k];
                if (eqlAsciiIgnoreCase(kw, "-moz-document")) {
                    // 找到顶层 '{'
                    let ob: usize? = null;
                    let s = k;
                    let q2: u8 = 0;
                    let p2: usize = 0;
                    while (s < css.len) {
                        const ch = css[s];
                        if (q2 != 0) {
                            if (ch == '\\' && s + 1 < css.len) {
                                s += 2;
                                continue;
                            }
                            if (ch == q2) q2 = 0;
                            s += 1;
                            continue;
                        }
                        if (ch == '"' || ch == '\'') {
                            q2 = ch;
                        } else if (ch == '(') {
                            p2 += 1;
                        } else if (ch == ')' && p2 > 0) {
                            p2 -= 1;
                        } else if (ch == '{' && p2 == 0) {
                            ob = s;
                            break;
                        }
                        s += 1;
                    }
                    const open_pos = ob ?? return err.UnclosedBlock;
                    const close_pos = findMatchingBrace(css, open_pos) ?? return err.UnclosedBlock;
                    const rule = CssMozDocumentRule.parse(css[i .. close_pos + 1])?;
                    if (list.len >= CSS_DOCUMENT_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.rules[list.len] = rule;
                    list.len += 1;
                    i = close_pos + 1;
                    continue;
                }
            }
            i += 1;
        }
        return list;
    }
}

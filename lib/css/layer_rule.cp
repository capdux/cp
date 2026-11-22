// Source: Absorbed from /data/dev/bun/src/css/rules/layer.cp | Synced: 2026-11-22T12:14:05Z
// Pure-CP zero-allocation W3C CSS Cascading and Inheritance Level 5 `@layer` rule parser,
// dotted sublayer name validator, statement/block serializer, stylesheet extractor,
// and cascade layer precedence ordering registry.

/// 单个 `<layer-name>` 允许的最大点分嵌套层级数（如 `framework.theme.dark`）
pub const CSS_LAYER_MAX_SEGMENTS: usize = 16;

/// 单条 `@layer` 语句声明允许的最大层名称数（如 `@layer reset, base, utilities;`）
pub const CSS_LAYER_MAX_STATEMENT_NAMES: usize = 32;

/// 样式表批量提取 `@layer` 规则的最大条目数
pub const CSS_LAYER_MAX_RULES: usize = 32;

/// `CssLayerOrderRegistry` 支持的最大层优先级槽位数
pub const CSS_LAYER_MAX_REGISTRY: usize = 64;

/// CSS `@layer` 规则解析、序列化与优先级注册错误集
pub err CssLayerError {
    InvalidLayerName,
    InvalidPrelude,
    EmptyLayerName,
    TooManySegments,
    TooManyLayerNames,
    UnclosedBlock,
    UnexpectedToken,
    BufferTooSmall,
    TooManyRules,
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

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == '_' || c >= 0x80;
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9') || c == '-';
}

fn isValidIdentSegment(seg: []const u8): bool {
    if (seg.len == 0) {
        return false;
    }
    let idx: usize = 0;
    if (seg[0] == '-') {
        if (seg.len == 1) {
            return false;
        }
        if (seg[1] == '-') {
            idx = 2;
        } else if (isIdentStart(seg[1])) {
            idx = 2;
        } else {
            return false;
        }
    } else if (isIdentStart(seg[0])) {
        idx = 1;
    } else {
        return false;
    }

    while (idx < seg.len) {
        if (!isIdentChar(seg[idx])) {
            return false;
        }
        idx += 1;
    }
    return true;
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

fn appendChar(out: []u8, pos: usize, ch: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = ch;
    return pos + 1;
}

/// 跳过从 `start_pos` 开始的空白与 `/* ... */` 注释
fn skipWsAndComments(s: []const u8, start_pos: usize): usize {
    let i = start_pos;
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

/// 从 `open_pos`（`open_ch`）出发查找配对的 `close_ch`，跳过引号字符串与注释
fn findMatchingDelim(s: []const u8, open_pos: usize, open_ch: u8, close_ch: u8): usize? {
    if (open_pos >= s.len || s[open_pos] != open_ch) {
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
        if (c == open_ch) {
            depth += 1;
        } else if (c == close_ch) {
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

/// 校验并消耗 `@layer` 关键字前缀，返回关键字之后的游标位置
fn consumeAtLayerHeader(s: []const u8): usize! {
    let i = skipWsAndComments(s, 0);
    if (i >= s.len || s[i] != '@') {
        return err.InvalidPrelude;
    }
    i += 1;
    const kw_start = i;
    while (i < s.len && isIdentChar(s[i])) {
        i += 1;
    }
    if (!eqlAsciiIgnoreCase(s[kw_start..i], "layer")) {
        return err.InvalidPrelude;
    }
    // `@layer` 后必须紧跟空白、注释、`{` 或 `;`，防止 `@layerfoo` 误匹配
    if (i < s.len && !isWs(s[i]) && s[i] != '{' && s[i] != ';' && !(i + 1 < s.len && s[i] == '/' && s[i + 1] == '*')) {
        return err.InvalidPrelude;
    }
    return i;
}

/// 点分 CSS 级联层名称（`<layer-name> = <ident> [ '.' <ident> ]*`）
///
/// 严格遵循 W3C CSS Cascade Level 5 与 Bun `css/rules/layer.cp` 语义：
/// `.` 号两侧严禁插入空白，每个分段必须是合法 CSS `<ident>`。
pub struct CssLayerName {
    segments: [CSS_LAYER_MAX_SEGMENTS][]const u8 = [""; CSS_LAYER_MAX_SEGMENTS],
    len: usize = 0,

    /// 解析单个点分 `<layer-name>`（自动去除首尾空白，拒绝中间空白、空分段与非法标识符）
    static fn parse(raw: []const u8): CssLayerName! {
        const s = trimWs(raw);
        if (s.len == 0) {
            return err.EmptyLayerName;
        }

        let name = CssLayerName{};
        let seg_start: usize = 0;
        let i: usize = 0;
        while (i <= s.len) {
            if (i == s.len || s[i] == '.') {
                const seg = s[seg_start..i];
                if (!isValidIdentSegment(seg)) {
                    return err.InvalidLayerName;
                }
                if (name.len >= CSS_LAYER_MAX_SEGMENTS) {
                    return err.TooManySegments;
                }
                name.segments[name.len] = seg;
                name.len += 1;
                seg_start = i + 1;
            }
            i += 1;
        }
        return name;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): ([]const u8)? {
        if (idx >= _.len) {
            return null;
        }
        return _.segments[idx];
    }

    fn eql(other: CssLayerName): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!bytesEql(_.segments[i], other.segments[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 计算 64 位 FNV-1a 哈希（以 `\0` 分隔分段，防止 `a.b` 与 `ab` 碰撞）
    fn hash(): u64 {
        let h: u64 = 14695981039346656037;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                h ^= 0;
                h *%= 1099511628211;
            }
            const seg = _.segments[i];
            let j: usize = 0;
            while (j < seg.len) {
                h ^= seg[j] as u64;
                h *%= 1099511628211;
                j += 1;
            }
            i += 1;
        }
        return h;
    }

    /// 判断当前层是否为 `ancestor` 的严格子层（如 `theme.dark` 是 `theme` 的子层）
    fn isSublayerOf(ancestor: CssLayerName): bool {
        if (ancestor.len == 0 || ancestor.len >= _.len) {
            return false;
        }
        let i: usize = 0;
        while (i < ancestor.len) {
            if (!bytesEql(_.segments[i], ancestor.segments[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 判断当前层与 `ancestor` 相同或为其子层
    fn isSameOrSublayerOf(ancestor: CssLayerName): bool {
        return _.eql(ancestor) || _.isSublayerOf(ancestor);
    }

    /// 返回直接父层名称；若当前层为顶层（`count() <= 1`）则返回 `null`
    fn parent(): CssLayerName? {
        if (_.len <= 1) {
            return null;
        }
        let p = CssLayerName{};
        let i: usize = 0;
        while (i + 1 < _.len) {
            p.segments[i] = _.segments[i];
            i += 1;
        }
        p.len = _.len - 1;
        return p;
    }

    /// 将点分层名追加写入 `out[pos..]`，返回新的写入偏移
    fn writeCss(out: []u8, start_pos: usize): usize! {
        let pos = start_pos;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                pos = appendChar(out, pos, '.')?;
            }
            pos = appendBytes(out, pos, _.segments[i])?;
            i += 1;
        }
        return pos;
    }

    /// 将点分层名序列化为 CSS 文本写入 `out`
    fn toCss(out: []u8): usize! {
        return _.writeCss(out, 0);
    }
}

/// CSS `@layer` 语句规则（`@layer <layer-name># ;`）
pub struct CssLayerStatementRule {
    names: [CSS_LAYER_MAX_STATEMENT_NAMES]CssLayerName = [CssLayerName{}; CSS_LAYER_MAX_STATEMENT_NAMES],
    len: usize = 0,

    /// 解析 `@layer reset, base, theme.dark;` 语句规则
    static fn parse(raw: []const u8): CssLayerStatementRule! {
        let i = consumeAtLayerHeader(raw)?;
        const start = skipWsAndComments(raw, i);
        // 找到顶层分号 `;`（拒绝 `{`）
        let semi_pos: usize? = null;
        let cur = start;
        while (cur < raw.len) {
            if (raw[cur] == '/' && cur + 1 < raw.len && raw[cur + 1] == '*') {
                cur = skipWsAndComments(raw, cur);
                continue;
            }
            if (raw[cur] == '{') {
                return err.InvalidPrelude;
            }
            if (raw[cur] == ';') {
                semi_pos = cur;
                break;
            }
            cur += 1;
        }

        const end = semi_pos ?? return err.InvalidPrelude;
        const trailing = skipWsAndComments(raw, end + 1);
        if (trailing < raw.len) {
            return err.UnexpectedToken;
        }

        const prelude = trimWs(raw[start..end]);
        if (prelude.len == 0) {
            return err.EmptyLayerName;
        }

        let rule = CssLayerStatementRule{};
        let seg_start: usize = 0;
        let idx: usize = 0;
        while (idx <= prelude.len) {
            if (idx < prelude.len && prelude[idx] == '/' && idx + 1 < prelude.len && prelude[idx + 1] == '*') {
                // 不允许在层名中间夹杂注释后直接继续，仅允许段首/段尾注释；此处跳过注释便于按逗号切分
                const next = skipWsAndComments(prelude, idx);
                idx = next;
                continue;
            }
            if (idx == prelude.len || prelude[idx] == ',') {
                const part = trimWs(prelude[seg_start..idx]);
                if (part.len == 0) {
                    return err.EmptyLayerName;
                }
                if (rule.len >= CSS_LAYER_MAX_STATEMENT_NAMES) {
                    return err.TooManyLayerNames;
                }
                rule.names[rule.len] = CssLayerName.parse(part)?;
                rule.len += 1;
                seg_start = idx + 1;
            }
            idx += 1;
        }
        return rule;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssLayerName? {
        if (idx >= _.len) {
            return null;
        }
        return _.names[idx];
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn eql(other: CssLayerStatementRule): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!_.names[i].eql(other.names[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 格式化输出：`@layer reset, base, theme.dark;`（空列表输出 `@layer;`）
    fn toCss(out: []u8): usize! {
        if (_.len == 0) {
            return appendBytes(out, 0, "@layer;");
        }
        let pos = appendBytes(out, 0, "@layer ")?;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                pos = appendBytes(out, pos, ", ")?;
            }
            pos = _.names[i].writeCss(out, pos)?;
            i += 1;
        }
        return appendChar(out, pos, ';');
    }

    /// 压缩输出：`@layer reset,base,theme.dark;`（空列表输出 `@layer;`）
    fn toCssMinified(out: []u8): usize! {
        if (_.len == 0) {
            return appendBytes(out, 0, "@layer;");
        }
        let pos = appendBytes(out, 0, "@layer ")?;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                pos = appendChar(out, pos, ',')?;
            }
            pos = _.names[i].writeCss(out, pos)?;
            i += 1;
        }
        return appendChar(out, pos, ';');
    }
}

/// CSS `@layer` 块规则（`@layer [ <layer-name> ]? { <rule-list> }`）
pub struct CssLayerBlockRule {
    name: CssLayerName? = null,
    body: []const u8 = "",

    /// 解析命名或匿名 `@layer { ... }` 块规则
    static fn parse(raw: []const u8): CssLayerBlockRule! {
        let i = consumeAtLayerHeader(raw)?;
        const start = skipWsAndComments(raw, i);

        // 查找开括号 `{`（拒绝先出现 `;` 或 `,`）
        let brace_pos: usize? = null;
        let cur = start;
        while (cur < raw.len) {
            if (raw[cur] == '/' && cur + 1 < raw.len && raw[cur + 1] == '*') {
                cur = skipWsAndComments(raw, cur);
                continue;
            }
            if (raw[cur] == ';' || raw[cur] == ',') {
                return err.InvalidPrelude;
            }
            if (raw[cur] == '{') {
                brace_pos = cur;
                break;
            }
            cur += 1;
        }

        const open_brace = brace_pos ?? return err.UnclosedBlock;
        const close_brace = findMatchingDelim(raw, open_brace, '{', '}') ?? return err.UnclosedBlock;
        const trailing = skipWsAndComments(raw, close_brace + 1);
        if (trailing < raw.len) {
            return err.UnexpectedToken;
        }

        const prelude = trimWs(raw[start..open_brace]);
        let parsed_name: CssLayerName? = null;
        if (prelude.len > 0) {
            parsed_name = CssLayerName.parse(prelude)?;
        }

        return CssLayerBlockRule{
            name: parsed_name,
            body: trimWs(raw[open_brace + 1..close_brace]),
        };
    }

    fn isAnonymous(): bool {
        return _.name == null;
    }

    fn hasName(): bool {
        return _.name != null;
    }

    fn eql(other: CssLayerBlockRule): bool {
        if (n1 := _.name) {
            if (n2 := other.name) {
                if (!n1.eql(n2)) {
                    return false;
                }
            } else {
                return false;
            }
        } else if (other.name != null) {
            return false;
        }
        return bytesEql(_.body, other.body);
    }

    /// 格式化输出：`@layer theme.dark { ... }` 或匿名 `@layer { ... }`
    fn toCss(out: []u8): usize! {
        let pos = appendBytes(out, 0, "@layer")?;
        if (n := _.name) {
            pos = appendChar(out, pos, ' ')?;
            pos = n.writeCss(out, pos)?;
        }
        if (_.body.len == 0) {
            return appendBytes(out, pos, " {}");
        }
        pos = appendBytes(out, pos, " { ")?;
        pos = appendBytes(out, pos, _.body)?;
        return appendBytes(out, pos, " }");
    }

    /// 压缩输出：`@layer theme.dark{...}` 或匿名 `@layer{...}`
    fn toCssMinified(out: []u8): usize! {
        let pos = appendBytes(out, 0, "@layer")?;
        if (n := _.name) {
            pos = appendChar(out, pos, ' ')?;
            pos = n.writeCss(out, pos)?;
        }
        pos = appendChar(out, pos, '{')?;
        if (_.body.len > 0) {
            pos = appendBytes(out, pos, _.body)?;
        }
        return appendChar(out, pos, '}');
    }
}

/// `@layer` 规则类型标签
pub enum CssLayerRuleKind: u8 {
    Statement = 0,
    Block = 1,
}

/// 统一 `@layer` 规则容器（自动区分语句规则与块规则）
pub struct CssLayerRule {
    kind: CssLayerRuleKind = .Statement,
    statement: CssLayerStatementRule = CssLayerStatementRule{},
    block: CssLayerBlockRule = CssLayerBlockRule{},

    /// 自动识别并解析任意 `@layer` 语句或块规则
    static fn parse(raw: []const u8): CssLayerRule! {
        let i = consumeAtLayerHeader(raw)?;
        while (i < raw.len) {
            if (raw[i] == '/' && i + 1 < raw.len && raw[i + 1] == '*') {
                i = skipWsAndComments(raw, i);
                continue;
            }
            if (raw[i] == ';') {
                return CssLayerRule{
                    kind: .Statement,
                    statement: CssLayerStatementRule.parse(raw)?,
                };
            }
            if (raw[i] == '{') {
                return CssLayerRule{
                    kind: .Block,
                    block: CssLayerBlockRule.parse(raw)?,
                };
            }
            i += 1;
        }
        return err.InvalidPrelude;
    }

    fn isStatement(): bool {
        return _.kind == .Statement;
    }

    fn isBlock(): bool {
        return _.kind == .Block;
    }

    fn toCss(out: []u8): usize! {
        if (_.kind == .Statement) {
            return _.statement.toCss(out);
        }
        return _.block.toCss(out);
    }

    fn toCssMinified(out: []u8): usize! {
        if (_.kind == .Statement) {
            return _.statement.toCssMinified(out);
        }
        return _.block.toCssMinified(out);
    }
}

/// 样式表顶层 `@layer` 规则批量提取结果
pub struct CssLayerRuleList {
    items: [CSS_LAYER_MAX_RULES]CssLayerRule = [CssLayerRule{}; CSS_LAYER_MAX_RULES],
    len: usize = 0,

    /// 扫描完整样式表，提取所有顶层 `@layer` 语句与块规则
    static fn extractFromStylesheet(css_src: []const u8): CssLayerRuleList! {
        let list = CssLayerRuleList{};
        let i: usize = 0;
        let in_quote: u8 = 0;

        while (i < css_src.len) {
            i = skipWsAndComments(css_src, i);
            if (i >= css_src.len) {
                break;
            }

            const c = css_src[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < css_src.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }

            if (c == '"' || c == '\'') {
                in_quote = c;
                i += 1;
                continue;
            }

            if (c == '@') {
                let kw_end = i + 1;
                while (kw_end < css_src.len && isIdentChar(css_src[kw_end])) {
                    kw_end += 1;
                }
                const is_layer = eqlAsciiIgnoreCase(css_src[i + 1..kw_end], "layer") &&
                    (kw_end >= css_src.len || isWs(css_src[kw_end]) || css_src[kw_end] == '{' || css_src[kw_end] == ';' ||
                        (kw_end + 1 < css_src.len && css_src[kw_end] == '/' && css_src[kw_end + 1] == '*'));

                // 寻找该 at-rule 的结束位置（`;` 或配对 `{ ... }`）
                let cur = kw_end;
                let rule_end: usize? = null;
                let q: u8 = 0;
                while (cur < css_src.len) {
                    const ch = css_src[cur];
                    if (q != 0) {
                        if (ch == '\\' && cur + 1 < css_src.len) {
                            cur += 2;
                            continue;
                        }
                        if (ch == q) {
                            q = 0;
                        }
                        cur += 1;
                        continue;
                    }
                    if (ch == '/' && cur + 1 < css_src.len && css_src[cur + 1] == '*') {
                        cur = skipWsAndComments(css_src, cur);
                        continue;
                    }
                    if (ch == '"' || ch == '\'') {
                        q = ch;
                        cur += 1;
                        continue;
                    }
                    if (ch == ';') {
                        rule_end = cur + 1;
                        break;
                    }
                    if (ch == '{') {
                        const close_b = findMatchingDelim(css_src, cur, '{', '}') ?? return err.UnclosedBlock;
                        rule_end = close_b + 1;
                        break;
                    }
                    cur += 1;
                }

                const end_pos = rule_end ?? return err.InvalidPrelude;
                if (is_layer) {
                    if (list.len >= CSS_LAYER_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.items[list.len] = CssLayerRule.parse(css_src[i..end_pos])?;
                    list.len += 1;
                }
                i = end_pos;
                continue;
            }

            // 跳过普通样式规则块 `{ ... }`
            if (c == '{') {
                const close_b = findMatchingDelim(css_src, i, '{', '}') ?? return err.UnclosedBlock;
                i = close_b + 1;
                continue;
            }

            i += 1;
        }
        return list;
    }

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssLayerRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }
}

/// 级联层注册槽位（命名层或匿名层）
struct LayerRegistrySlot {
    is_anonymous: bool = false,
    name: CssLayerName = CssLayerName{},
}

/// W3C CSS Cascade Level 5 级联层优先级注册表
///
/// 遵循规范核心语义：
/// 1. 首次声明的层拥有更低优先级（索引越小优先级越低，索引越大优先级越高）；
/// 2. 重复声明已注册的层名不改变其原有优先级；
/// 3. 点分嵌套子层（如 `framework.theme.dark`）按父层到子层顺序依次建立注册槽位；
/// 4. 匿名 `@layer { ... }` 每次注册都会获得独立的新增优先级槽位。
pub struct CssLayerOrderRegistry {
    slots: [CSS_LAYER_MAX_REGISTRY]LayerRegistrySlot = [LayerRegistrySlot{}; CSS_LAYER_MAX_REGISTRY],
    len: usize = 0,

    static fn init(): CssLayerOrderRegistry {
        return CssLayerOrderRegistry{};
    }

    fn count(): usize {
        return _.len;
    }

    /// 查找已注册命名层的优先级索引（值越大优先级越高），未注册返回 `null`
    fn priorityOf(name: CssLayerName): usize? {
        let i: usize = 0;
        while (i < _.len) {
            if (!_.slots[i].is_anonymous && _.slots[i].name.eql(name)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 比较两个已注册命名层的级联优先级：
    /// 返回 `-1`（`a` 低于 `b`）、`0`（相等）、`1`（`a` 高于 `b`）；任一未注册返回 `null`
    fn comparePriority(a: CssLayerName, b: CssLayerName): i8? {
        const pa = _.priorityOf(a) ?? return null;
        const pb = _.priorityOf(b) ?? return null;
        if (pa < pb) {
            return -1;
        }
        if (pa > pb) {
            return 1;
        }
        return 0;
    }

    /// 注册单个命名层（自动展开父层前缀），返回本次新增的槽位数
    fn registerName(*_, name: CssLayerName): usize! {
        if (name.len == 0) {
            return err.EmptyLayerName;
        }
        let added: usize = 0;
        let depth: usize = 1;
        while (depth <= name.len) {
            let prefix = CssLayerName{};
            let k: usize = 0;
            while (k < depth) {
                prefix.segments[k] = name.segments[k];
                k += 1;
            }
            prefix.len = depth;

            if (_.priorityOf(prefix) == null) {
                if (_.len >= CSS_LAYER_MAX_REGISTRY) {
                    return err.RegistryFull;
                }
                _.slots[_.len] = LayerRegistrySlot{
                    is_anonymous: false,
                    name: prefix,
                };
                _.len += 1;
                added += 1;
            }
            depth += 1;
        }
        return added;
    }

    /// 注册一个匿名层并返回其分配的唯一优先级索引
    fn registerAnonymous(*_): usize! {
        if (_.len >= CSS_LAYER_MAX_REGISTRY) {
            return err.RegistryFull;
        }
        const idx = _.len;
        _.slots[idx] = LayerRegistrySlot{
            is_anonymous: true,
            name: CssLayerName{},
        };
        _.len += 1;
        return idx;
    }

    /// 注册单条 `@layer` 规则（语句或块），返回本次新增的槽位数
    fn registerRule(*_, rule: CssLayerRule): usize! {
        if (rule.kind == .Statement) {
            let added: usize = 0;
            let i: usize = 0;
            while (i < rule.statement.len) {
                added += _.registerName(rule.statement.names[i])?;
                i += 1;
            }
            return added;
        }
        if (n := rule.block.name) {
            return _.registerName(n);
        }
        * = _.registerAnonymous()?;
        return 1;
    }

    /// 按顺序注册从样式表提取的全部 `@layer` 规则，返回累计新增槽位数
    fn registerRuleList(*_, list: CssLayerRuleList): usize! {
        let added: usize = 0;
        let i: usize = 0;
        while (i < list.len) {
            added += _.registerRule(list.items[i])?;
            i += 1;
        }
        return added;
    }
}

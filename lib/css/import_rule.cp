// Source: Absorbed from /data/dev/bun/src/css/rules/import.cp | Synced: 2026-11-22T07:40:00Z
// Pure-CP zero-allocation W3C CSS Cascading and Inheritance Level 5 `@import` rule parser,
// condition model (`layer` / `supports(...)` / `<media-query-list>`), canonical serializer
// with bundler placeholder rewriting, and stylesheet leading `@import` extractor.

/// 样式表批量提取前置 `@import` 规则的最大条目数
pub const CSS_IMPORT_MAX_RULES: usize = 64;

/// 未绑定 bundler import record 时的哨兵索引（对应 `std.math.maxInt(u32)`）
pub const CSS_IMPORT_NO_RECORD: u32 = 0xFFFF_FFFF;

/// CSS `@import` 规则解析、校验与序列化错误集
pub err CssImportError {
    InvalidPrelude,
    InvalidUrl,
    InvalidLayer,
    InvalidSupports,
    DuplicateCondition,
    UnexpectedToken,
    BufferTooSmall,
    TooManyRules,
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
        if (seg[1] == '-' || isIdentStart(seg[1])) {
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

fn isValidDottedLayerName(name: []const u8): bool {
    if (name.len == 0) {
        return false;
    }
    let seg_start: usize = 0;
    let i: usize = 0;
    let seg_count: usize = 0;
    while (i <= name.len) {
        if (i == name.len || name[i] == '.') {
            const seg = name[seg_start..i];
            if (!isValidIdentSegment(seg)) {
                return false;
            }
            seg_count += 1;
            if (seg_count > 16) {
                return false;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return seg_count > 0;
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

fn startsWithAsciiIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return eqlAsciiIgnoreCase(s[0..prefix.len], prefix);
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

fn findMatchingParen(s: []const u8, open_pos: usize): usize? {
    if (open_pos >= s.len || s[open_pos] != '(') {
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
        if (c == '(') {
            depth += 1;
        } else if (c == ')') {
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

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (pos + src.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

fn writeByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn writeCssQuotedString(out: []u8, start: usize, value: []const u8): usize! {
    let pos = writeByte(out, start, '"')?;
    let i: usize = 0;
    while (i < value.len) {
        const c = value[i];
        if (c == '"' || c == '\\') {
            pos = writeByte(out, pos, '\\')?;
        }
        pos = writeByte(out, pos, c)?;
        i += 1;
    }
    pos = writeByte(out, pos, '"')?;
    return pos;
}

/// `@import` 规则的级联层类别
pub enum CssImportLayerKind: u8 {
    none = 0,
    anonymous = 1,
    named = 2,
}

/// `@import` 规则中的 `layer` / `layer(<layer-name>)` 条件（对应上游 `ImportLayer`）
pub struct CssImportLayer {
    kind: CssImportLayerKind,
    name: []const u8,

    /// 无层条件
    static fn none(): CssImportLayer {
        return CssImportLayer{
            kind: .none,
            name: "",
        };
    }

    /// 匿名层 `layer`
    static fn anonymous(): CssImportLayer {
        return CssImportLayer{
            kind: .anonymous,
            name: "",
        };
    }

    /// 命名层 `layer(<layer-name>)`，校验点分合法标识符
    static fn named(name: []const u8): CssImportLayer! {
        const trimmed = trimWs(name);
        if (!isValidDottedLayerName(trimmed)) {
            return err.InvalidLayer;
        }
        return CssImportLayer{
            kind: .named,
            name: trimmed,
        };
    }

    /// 是否声明了 `layer` 条件（匿名或命名）
    fn isPresent(): bool {
        return _.kind != .none;
    }

    /// 是否为匿名层
    fn isAnonymous(): bool {
        return _.kind == .anonymous;
    }

    /// 等价比较（对应上游 `ImportLayer.eql`）
    fn eql(other: CssImportLayer): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == .named) {
            return bytesEql(_.name, other.name);
        }
        return true;
    }

    /// 序列化为 `layer` 或 `layer(<name>)`；无层时写入 0 字节
    fn toCss(out: []u8): usize! {
        if (_.kind == .none) {
            return 0;
        }
        let pos = writeBytes(out, 0, "layer")?;
        if (_.kind == .named) {
            pos = writeByte(out, pos, '(')?;
            pos = writeBytes(out, pos, _.name)?;
            pos = writeByte(out, pos, ')')?;
        }
        return pos;
    }
}

/// `@import` 中 `supports(...)` 的内部语法形态
pub enum CssImportSupportsKind: u8 {
    none = 0,
    declaration = 1,
    condition = 2,
}

/// `@import` 中的 `supports(<supports-condition> | <declaration>)` 条件
pub struct CssImportSupports {
    kind: CssImportSupportsKind,
    raw: []const u8,

    /// 无 supports 条件
    static fn none(): CssImportSupports {
        return CssImportSupports{
            kind: .none,
            raw: "",
        };
    }

    /// 解析 `supports(...)` 括号内部表达式并识别 `declaration` vs `condition`
    static fn parse(inner: []const u8): CssImportSupports! {
        const trimmed = trimWs(inner);
        if (trimmed.len == 0) {
            return err.InvalidSupports;
        }
        // 校验括号平衡，防止畸形未闭合输入
        let depth: usize = 0;
        let in_quote: u8 = 0;
        let has_top_colon = false;
        let i: usize = 0;
        while (i < trimmed.len) {
            const c = trimmed[i];
            if (in_quote != 0) {
                if (c == '\\' && i + 1 < trimmed.len) {
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
            if (c == '(') {
                depth += 1;
            } else if (c == ')') {
                if (depth == 0) {
                    return err.InvalidSupports;
                }
                depth -= 1;
            } else if (c == ':' && depth == 0) {
                has_top_colon = true;
            }
            i += 1;
        }
        if (depth != 0 || in_quote != 0) {
            return err.InvalidSupports;
        }

        // 若以 `not ` / `selector(` / `font-tech(` / `font-format(` / `(` 开头，则归类为 condition
        const is_cond_prefix = trimmed[0] == '(' ||
            startsWithAsciiIgnoreCase(trimmed, "not ") ||
            startsWithAsciiIgnoreCase(trimmed, "not(") ||
            startsWithAsciiIgnoreCase(trimmed, "selector(") ||
            startsWithAsciiIgnoreCase(trimmed, "font-tech(") ||
            startsWithAsciiIgnoreCase(trimmed, "font-format(");

        const kind: CssImportSupportsKind = if (!is_cond_prefix && has_top_colon) .declaration else .condition;
        return CssImportSupports{
            kind: kind,
            raw: trimmed,
        };
    }

    /// 是否存在 `supports(...)` 条件
    fn isPresent(): bool {
        return _.kind != .none;
    }

    /// 等价比较
    fn eql(other: CssImportSupports): bool {
        if (_.kind != other.kind) {
            return false;
        }
        if (_.kind == .none) {
            return true;
        }
        return bytesEql(_.raw, other.raw);
    }

    /// 序列化为 `supports(<raw>)`；无条件时写入 0 字节
    fn toCss(out: []u8): usize! {
        if (_.kind == .none) {
            return 0;
        }
        let pos = writeBytes(out, 0, "supports(")?;
        pos = writeBytes(out, pos, _.raw)?;
        pos = writeByte(out, pos, ')')?;
        return pos;
    }
}

/// `@import` 规则附带的组合条件（对应上游 `ImportConditions`）
pub struct CssImportConditions {
    layer: CssImportLayer,
    supports: CssImportSupports,
    media: []const u8,

    /// 空条件
    static fn empty(): CssImportConditions {
        return CssImportConditions{
            layer: CssImportLayer.none(),
            supports: CssImportSupports.none(),
            media: "",
        };
    }

    /// 是否不包含任何 `layer`、`supports` 或 `media` 条件
    fn isEmpty(): bool {
        return !_.layer.isPresent() && !_.supports.isPresent() && _.media.len == 0;
    }

    /// 是否包含匿名 `layer`（对应上游 `ImportConditions.hasAnonymousLayer`）
    fn hasAnonymousLayer(): bool {
        return _.layer.isAnonymous();
    }

    /// 比较两者的 `layer` 条件是否相等（对应上游 `ImportConditions.layersEql`）
    fn layersEql(other: CssImportConditions): bool {
        return _.layer.eql(other.layer);
    }

    /// 比较两者的 `supports` 条件是否相等（对应上游 `ImportConditions.supportsEql`）
    fn supportsEql(other: CssImportConditions): bool {
        return _.supports.eql(other.supports);
    }

    /// 完整条件等价比较
    fn eql(other: CssImportConditions): bool {
        return _.layer.eql(other.layer) &&
            _.supports.eql(other.supports) &&
            bytesEql(_.media, other.media);
    }

    /// 序列化条件部分（不带前导空格，例如 `layer(base) supports(display: grid) screen`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        if (_.layer.isPresent()) {
            const n = _.layer.toCss(out[pos..])?;
            pos += n;
        }
        if (_.supports.isPresent()) {
            if (pos > 0) {
                pos = writeByte(out, pos, ' ')?;
            }
            const n = _.supports.toCss(out[pos..])?;
            pos += n;
        }
        if (_.media.len > 0) {
            if (pos > 0) {
                pos = writeByte(out, pos, ' ')?;
            }
            pos = writeBytes(out, pos, _.media)?;
        }
        return pos;
    }
}

struct ParsedUrlSlice {
    url: []const u8,
    next_pos: usize,
}

fn parseQuotedString(s: []const u8, start: usize): ParsedUrlSlice! {
    if (start >= s.len) {
        return err.InvalidUrl;
    }
    const quote = s[start];
    if (quote != '"' && quote != '\'') {
        return err.InvalidUrl;
    }
    let i = start + 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '\\') {
            if (i + 1 >= s.len) {
                return err.InvalidUrl;
            }
            i += 2;
            continue;
        }
        if (c == '\n' || c == '\r') {
            return err.InvalidUrl;
        }
        if (c == quote) {
            const val = s[start + 1..i];
            if (val.len == 0) {
                return err.InvalidUrl;
            }
            return ParsedUrlSlice{
                url: val,
                next_pos: i + 1,
            };
        }
        i += 1;
    }
    return err.InvalidUrl;
}

fn parseImportTargetUrl(s: []const u8, start: usize): ParsedUrlSlice! {
    let i = skipWsAndComments(s, start);
    if (i >= s.len) {
        return err.InvalidUrl;
    }
    if (s[i] == '"' || s[i] == '\'') {
        return parseQuotedString(s, i);
    }
    // 支持 url("..."), url('...'), url(...)
    if (startsWithAsciiIgnoreCase(s[i..], "url(")) {
        const open_pos = i + 3;
        const close_opt = findMatchingParen(s, open_pos);
        if (close_pos := close_opt) {
            const inner_start = skipWsAndComments(s, open_pos + 1);
            if (inner_start >= close_pos) {
                return err.InvalidUrl;
            }
            if (s[inner_start] == '"' || s[inner_start] == '\'') {
                const parsed = parseQuotedString(s, inner_start)?;
                const after = skipWsAndComments(s, parsed.next_pos);
                if (after != close_pos) {
                    return err.InvalidUrl;
                }
                return ParsedUrlSlice{
                    url: parsed.url,
                    next_pos: close_pos + 1,
                };
            }
            // 无引号 URL：不允许内部包含空白或引号
            const raw_url = trimWs(s[inner_start..close_pos]);
            if (raw_url.len == 0) {
                return err.InvalidUrl;
            }
            let k: usize = 0;
            while (k < raw_url.len) {
                const ch = raw_url[k];
                if (isWs(ch) || ch == '"' || ch == '\'' || ch == '(') {
                    return err.InvalidUrl;
                }
                k += 1;
            }
            return ParsedUrlSlice{
                url: raw_url,
                next_pos: close_pos + 1,
            };
        }
        return err.InvalidUrl;
    }
    return err.InvalidUrl;
}

/// 完整的 W3C CSS `@import` 规则节点（对应上游 `ImportRule`）
pub struct CssImportRule {
    url: []const u8,
    conditions: CssImportConditions,
    import_record_idx: u32,
    line: u32,
    column: u32,

    /// 仅凭 URL 构造无条件 `@import`（对应上游 `ImportRule.fromUrl`）
    static fn fromUrl(url: []const u8): CssImportRule {
        return CssImportRule{
            url: url,
            conditions: CssImportConditions.empty(),
            import_record_idx: CSS_IMPORT_NO_RECORD,
            line: 0,
            column: 0,
        };
    }

    /// 凭 URL 与 bundler import_record_idx 构造（对应上游 `ImportRule.fromUrlAndImportRecordIdx`）
    static fn fromUrlAndImportRecordIdx(url: []const u8, import_record_idx: u32): CssImportRule {
        return CssImportRule{
            url: url,
            conditions: CssImportConditions.empty(),
            import_record_idx: import_record_idx,
            line: 0,
            column: 0,
        };
    }

    /// 凭 URL 与组合条件构造（对应上游 `ImportRule.fromConditionsAndUrl`）
    static fn fromConditionsAndUrl(url: []const u8, conds: CssImportConditions): CssImportRule {
        return CssImportRule{
            url: url,
            conditions: conds,
            import_record_idx: CSS_IMPORT_NO_RECORD,
            line: 0,
            column: 0,
        };
    }

    /// 解析单条 `@import` 规则（支持 `"..."` / `'...'` / `url(...)`、`layer` / `layer(a.b)`、`supports(...)`、媒体查询列表）
    static fn parse(input: []const u8): CssImportRule! {
        let i = skipWsAndComments(input, 0);
        if (i >= input.len || input[i] != '@') {
            return err.InvalidPrelude;
        }
        i += 1;
        const kw_start = i;
        while (i < input.len && isIdentChar(input[i])) {
            i += 1;
        }
        if (!eqlAsciiIgnoreCase(input[kw_start..i], "import")) {
            return err.InvalidPrelude;
        }

        // 解析 URL 目标
        const target = parseImportTargetUrl(input, i)?;
        i = target.next_pos;

        // 找到末尾边界（允许可选 `;`，拦截 `{` 块）
        let end = input.len;
        let scan = i;
        let in_quote: u8 = 0;
        let paren_depth: usize = 0;
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
                    return err.UnexpectedToken;
                }
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                scan += 1;
                continue;
            }
            if (c == '(') {
                paren_depth += 1;
            } else if (c == ')') {
                if (paren_depth == 0) {
                    return err.UnexpectedToken;
                }
                paren_depth -= 1;
            } else if (paren_depth == 0 && c == '{') {
                return err.UnexpectedToken;
            } else if (paren_depth == 0 && c == ';') {
                end = scan;
                // `;` 之后只允许空白或注释
                const trailing = skipWsAndComments(input, scan + 1);
                if (trailing != input.len) {
                    return err.UnexpectedToken;
                }
                break;
            }
            scan += 1;
        }
        if (in_quote != 0 || paren_depth != 0) {
            return err.UnexpectedToken;
        }

        // 按 CSS Cascading Level 5 语法顺序解析条件：[ layer | layer(<layer-name>) ]? [ supports(...) ]? <media-query-list>?
        let layer = CssImportLayer.none();
        let supports = CssImportSupports.none();
        let cur = skipWsAndComments(input[0..end], i);

        // 1) 可选 layer / layer(<layer-name>)
        if (cur < end && startsWithAsciiIgnoreCase(input[cur..end], "layer")) {
            const after_kw = cur + 5;
            if (after_kw == end || isWs(input[after_kw]) || (after_kw + 1 < end && input[after_kw] == '/' && input[after_kw + 1] == '*')) {
                layer = CssImportLayer.anonymous();
                cur = skipWsAndComments(input[0..end], after_kw);
            } else if (input[after_kw] == '(') {
                const close_opt = findMatchingParen(input[0..end], after_kw);
                if (close_pos := close_opt) {
                    const inner = input[after_kw + 1..close_pos];
                    layer = CssImportLayer.named(inner)?;
                    cur = skipWsAndComments(input[0..end], close_pos + 1);
                } else {
                    return err.InvalidLayer;
                }
            }
        }

        // 2) 可选 supports(...)
        if (cur < end && startsWithAsciiIgnoreCase(input[cur..end], "supports")) {
            const after_kw = cur + 8;
            if (after_kw >= end || input[after_kw] != '(') {
                return err.InvalidSupports;
            }
            const close_opt = findMatchingParen(input[0..end], after_kw);
            if (close_pos := close_opt) {
                const inner = input[after_kw + 1..close_pos];
                supports = CssImportSupports.parse(inner)?;
                cur = skipWsAndComments(input[0..end], close_pos + 1);
            } else {
                return err.InvalidSupports;
            }
        }

        // 3) 检查是否违反规范顺序（如在 supports 之后又出现 layer 或重复 supports）
        if (cur < end) {
            if (startsWithAsciiIgnoreCase(input[cur..end], "layer")) {
                const after_kw = cur + 5;
                if (after_kw == end || isWs(input[after_kw]) || input[after_kw] == '(') {
                    return err.DuplicateCondition;
                }
            }
            if (startsWithAsciiIgnoreCase(input[cur..end], "supports(")) {
                return err.DuplicateCondition;
            }
        }

        // 4) 剩余内容作为 <media-query-list>
        const media = trimWs(input[cur..end]);

        return CssImportRule{
            url: target.url,
            conditions: CssImportConditions{
                layer: layer,
                supports: supports,
                media: media,
            },
            import_record_idx: CSS_IMPORT_NO_RECORD,
            line: 0,
            column: 0,
        };
    }

    /// 是否附带任意 `layer`、`supports` 或 `media` 条件（对应上游 `ImportRule.hasConditions`）
    fn hasConditions(): bool {
        return !_.conditions.isEmpty();
    }

    /// 是否已绑定 bundler import record
    fn hasImportRecord(): bool {
        return _.import_record_idx != CSS_IMPORT_NO_RECORD;
    }

    /// 判断目标 URL 是否为外部网络/数据资源（bundler 通常保留外链 `@import` 而仅内联本地路径）
    fn isExternalUrl(): bool {
        return startsWithAsciiIgnoreCase(_.url, "http://") ||
            startsWithAsciiIgnoreCase(_.url, "https://") ||
            startsWithAsciiIgnoreCase(_.url, "data:") ||
            (_.url.len >= 2 && _.url[0] == '/' && _.url[1] == '/');
    }

    /// 等价比较
    fn eql(other: CssImportRule): bool {
        return bytesEql(_.url, other.url) && _.conditions.eql(other.conditions);
    }

    /// 规范序列化为 `@import "<url>" [layer] [supports(...)] [<media>];`（对应上游 `ImportRule.toCss`）
    fn toCss(out: []u8): usize! {
        return _.toCssWithPlaceholder(_.url, out);
    }

    /// 使用 bundler 依赖占位符改写 URL 并序列化（对应上游 `ImportRule.toCss` 的 dependency placeholder 分支）
    fn toCssWithPlaceholder(placeholder: []const u8, out: []u8): usize! {
        let pos = writeBytes(out, 0, "@import ")?;
        pos = writeCssQuotedString(out, pos, placeholder)?;
        if (!_.conditions.isEmpty()) {
            pos = writeByte(out, pos, ' ')?;
            const cn = _.conditions.toCss(out[pos..])?;
            pos += cn;
        }
        pos = writeByte(out, pos, ';')?;
        return pos;
    }
}

/// 样式表前置 `@import` 规则批量提取结果
pub struct CssImportRuleList {
    items: [CSS_IMPORT_MAX_RULES]CssImportRule,
    len: usize,
    /// 出现在普通样式规则之后、依 CSS Syntax 规范必须被忽略的迟到 `@import` 计数
    ignored_after_rules: usize,

    /// 跳过注释、`@charset` 与前置 `@layer` 语句，从样式表头部提取全部有效 `@import` 规则，
    /// 并统计出现在普通规则之后的非法/被忽略 `@import` 数量。
    static fn extractFromStylesheet(css: []const u8): CssImportRuleList! {
        let list = CssImportRuleList{
            items: [CssImportRule.fromUrl(""); CSS_IMPORT_MAX_RULES],
            len: 0,
            ignored_after_rules: 0,
        };

        let i: usize = 0;
        let line: u32 = 1;
        let col: u32 = 1;
        let seen_regular_rule = false;

        while (i < css.len) {
            // 跳过空白与注释并维护行列号
            while (i < css.len) {
                if (isWs(css[i])) {
                    if (css[i] == '\n') {
                        line += 1;
                        col = 1;
                    } else {
                        col += 1;
                    }
                    i += 1;
                    continue;
                }
                if (i + 1 < css.len && css[i] == '/' && css[i + 1] == '*') {
                    i += 2;
                    col += 2;
                    while (i + 1 < css.len && !(css[i] == '*' && css[i + 1] == '/')) {
                        if (css[i] == '\n') {
                            line += 1;
                            col = 1;
                        } else {
                            col += 1;
                        }
                        i += 1;
                    }
                    if (i + 1 < css.len) {
                        i += 2;
                        col += 2;
                    } else {
                        i = css.len;
                    }
                    continue;
                }
                break;
            }
            if (i >= css.len) {
                break;
            }

            const stmt_start = i;
            const stmt_line = line;
            const stmt_col = col;

            // 扫描当前顶层语句或块规则的结束位置
            let in_quote: u8 = 0;
            let brace_depth: usize = 0;
            let is_block = false;
            while (i < css.len) {
                const c = css[i];
                if (c == '\n') {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }
                if (in_quote != 0) {
                    if (c == '\\' && i + 1 < css.len) {
                        i += 2;
                        col += 1;
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
                    col += 1;
                    while (i + 1 < css.len && !(css[i] == '*' && css[i + 1] == '/')) {
                        if (css[i] == '\n') {
                            line += 1;
                            col = 1;
                        } else {
                            col += 1;
                        }
                        i += 1;
                    }
                    if (i + 1 < css.len) {
                        i += 2;
                        col += 2;
                    } else {
                        i = css.len;
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
                    is_block = true;
                } else if (c == '}') {
                    if (brace_depth > 0) {
                        brace_depth -= 1;
                        if (brace_depth == 0) {
                            i += 1;
                            break;
                        }
                    }
                } else if (c == ';' && brace_depth == 0) {
                    i += 1;
                    break;
                }
                i += 1;
            }

            const slice = css[stmt_start..i];
            if (slice.len > 0 && slice[0] == '@') {
                let k: usize = 1;
                while (k < slice.len && isIdentChar(slice[k])) {
                    k += 1;
                }
                const kw = slice[1..k];
                if (eqlAsciiIgnoreCase(kw, "charset")) {
                    continue;
                }
                if (eqlAsciiIgnoreCase(kw, "layer") && !is_block) {
                    // 前置 `@layer a, b;` 语句允许位于 `@import` 之前
                    continue;
                }
                if (eqlAsciiIgnoreCase(kw, "import")) {
                    if (seen_regular_rule) {
                        list.ignored_after_rules += 1;
                        continue;
                    }
                    let rule = CssImportRule.parse(slice)?;
                    rule.line = stmt_line;
                    rule.column = stmt_col;
                    if (list.len >= CSS_IMPORT_MAX_RULES) {
                        return err.TooManyRules;
                    }
                    list.items[list.len] = rule;
                    list.len += 1;
                    continue;
                }
            }

            // 遇到普通规则或块级 at-rule（如 `@media` / `@layer { ... }` / `.btn { ... }`）后，
            // 后续所有 `@import` 依规范必须忽略
            seen_regular_rule = true;
        }

        return list;
    }

    /// 返回提取到的有效前置 `@import` 数量
    fn count(): usize {
        return _.len;
    }

    /// 按索引读取规则
    fn get(idx: usize): CssImportRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.items[idx];
    }
}

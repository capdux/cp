// Source: Absorbed from /data/dev/bun/src/css/rules/custom_media.cp | Synced: 2026-11-22T12:14:06Z
// Pure-CP zero-allocation CSS Media Queries Level 5 `@custom-media` rule parser,
// serializer, fixed-capacity cascade registry, and `(--custom-name)` media query expander.

/// `@custom-media` 注册表固定容量上限
pub const CUSTOM_MEDIA_MAX_RULES: usize = 64;

/// `@custom-media` 规则解析、注册与展开错误集
pub err CustomMediaError {
    InvalidPrelude,
    InvalidDashedIdent,
    EmptyMediaQuery,
    BufferTooSmall,
    RegistryFull,
}

/// `@custom-media` 规则值类型（W3C Media Queries Level 5: `<media-query-list> | true | false`）
pub enum CustomMediaValueKind: u8 {
    query = 0,
    always_true = 1,
    always_false = 2,
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

fn isIdentChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c >= 0x80;
}

fn isValidCustomMediaName(name: []const u8): bool {
    if (name.len < 3) {
        return false;
    }
    if (name[0] != '-' || name[1] != '-') {
        return false;
    }
    let i: usize = 2;
    while (i < name.len) {
        if (!isIdentChar(name[i])) {
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

/// CSS `@custom-media` 规则（对齐 Bun `CustomMediaRule`）
pub struct CustomMediaRule {
    /// 声明的自定义媒体名称（必须以 `--` 开头，例如 `--narrow-window`）
    name: []const u8 = "",
    /// 规则值类型（媒体查询文本 / `true` / `false`）
    kind: CustomMediaValueKind = .query,
    /// 媒体查询列表原文（当 `kind == .query` 时有效）
    query: []const u8 = "",

    /// 解析 `@custom-media --name <media-query-list | true | false>;`（尾部 `;` 可选）
    static fn parse(rule_text: []const u8): CustomMediaRule! {
        let s = trimWs(rule_text);
        if (s.len > 0 && s[s.len - 1] == ';') {
            s = trimWs(s[0 .. s.len - 1]);
        }
        const prefix = "@custom-media";
        if (s.len <= prefix.len || !eqlAsciiIgnoreCase(s[0..prefix.len], prefix)) {
            return err.InvalidPrelude;
        }
        if (!isWs(s[prefix.len])) {
            return err.InvalidPrelude;
        }
        const rest = trimWs(s[prefix.len..]);
        if (rest.len == 0) {
            return err.InvalidDashedIdent;
        }

        // Extract dashed-ident token
        let name_end: usize = 0;
        while (name_end < rest.len && !isWs(rest[name_end])) {
            name_end += 1;
        }
        const name = rest[0..name_end];
        if (!isValidCustomMediaName(name)) {
            return err.InvalidDashedIdent;
        }

        const val = trimWs(rest[name_end..]);
        if (val.len == 0) {
            return err.EmptyMediaQuery;
        }
        if (eqlAsciiIgnoreCase(val, "true")) {
            return CustomMediaRule{
                name: name,
                kind: CustomMediaValueKind.always_true,
                query: "",
            };
        }
        if (eqlAsciiIgnoreCase(val, "false")) {
            return CustomMediaRule{
                name: name,
                kind: CustomMediaValueKind.always_false,
                query: "",
            };
        }
        return CustomMediaRule{
            name: name,
            kind: CustomMediaValueKind.query,
            query: val,
        };
    }

    /// 序列化为规范 CSS 字符串 `@custom-media --name <query|true|false>;`（对齐 Bun `toCss`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        pos = appendBytes(out, pos, "@custom-media ")?;
        pos = appendBytes(out, pos, _.name)?;
        pos = appendBytes(out, pos, " ")?;
        const body = switch (_.kind) {
            .always_true => "true",
            .always_false => "false",
            .query => _.query,
        };
        pos = appendBytes(out, pos, body)?;
        pos = appendBytes(out, pos, ";")?;
        return pos;
    }
}

/// 零堆分配 `@custom-media` 规则表（遵循 CSS 级联后声明覆盖语义）
pub struct CustomMediaRegistry {
    entries: [CUSTOM_MEDIA_MAX_RULES]CustomMediaRule = [CustomMediaRule{}; CUSTOM_MEDIA_MAX_RULES],
    count: usize = 0,

    /// 初始化空注册表
    static fn init(): CustomMediaRegistry {
        return CustomMediaRegistry{
            entries: [CustomMediaRule{}; CUSTOM_MEDIA_MAX_RULES],
            count: 0,
        };
    }

    /// 注册或覆盖一条 `@custom-media` 规则，返回规则所在槽位索引
    fn register(*_, rule: CustomMediaRule): usize! {
        let i: usize = 0;
        while (i < _.count) {
            if (bytesEql(_.entries[i].name, rule.name)) {
                _.entries[i] = rule;
                return i;
            }
            i += 1;
        }
        if (_.count >= CUSTOM_MEDIA_MAX_RULES) {
            return err.RegistryFull;
        }
        const slot = _.count;
        _.entries[slot] = rule;
        _.count += 1;
        return slot;
    }

    /// 按自定义媒体名称（如 `"--dark-mode"`）查找规则
    fn lookup(name: []const u8): CustomMediaRule? {
        let i: usize = 0;
        while (i < _.count) {
            if (bytesEql(_.entries[i].name, name)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    /// 将媒体查询字符串中的 `(--custom-name)` 零堆分配展开为对应的注册定义
    /// （`true` -> `all`，`false` -> `not all`，未知名称原样保留），返回写入字节数
    fn resolveMediaQuery(input: []const u8, out: []u8): usize! {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            if (input[i] == '(') {
                // Find matching ')'
                let close = i + 1;
                while (close < input.len && input[close] != ')' && input[close] != '(') {
                    close += 1;
                }
                if (close < input.len && input[close] == ')') {
                    const inner = trimWs(input[i + 1 .. close]);
                    if (isValidCustomMediaName(inner)) {
                        if (rule := _.lookup(inner)) {
                            const replacement = switch (rule.kind) {
                                .always_true => "all",
                                .always_false => "not all",
                                .query => rule.query,
                            };
                            pos = appendBytes(out, pos, replacement)?;
                            i = close + 1;
                            continue;
                        }
                    }
                }
            }
            if (pos >= out.len) {
                return err.BufferTooSmall;
            }
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
        return pos;
    }
}

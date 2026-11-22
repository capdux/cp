// Source: Absorbed from /data/dev/bun/src/css/rules/viewport.cp | Synced: 2026-11-22T11:50:00Z
// Pure-CP zero-allocation W3C CSS Device Adaptation Level 1 (`@viewport` / `@-ms-viewport` /
// `@-webkit-viewport` / `@-o-viewport` / `@-moz-viewport`) rule parser, descriptor validator,
// width/height shorthand expander, cascade-safe deduplicator, multi-prefix emitter,
// canonical & minified serializer, and stylesheet extractor.

/// 单条 `@viewport` 规则内支持的最大描述符声明数
pub const CSS_VIEWPORT_MAX_DECLS: usize = 32;

/// 样式表批量提取 `@viewport` 规则的最大条目数
pub const CSS_VIEWPORT_MAX_RULES: usize = 16;

/// CSS `@viewport` 规则解析、校验与序列化错误集
pub err CssViewportError {
    InvalidAtRule,
    UnknownVendorPrefix,
    UnclosedBlock,
    InvalidDeclaration,
    InvalidDescriptorValue,
    TooManyDeclarations,
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

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
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

fn writeLowerSlice(out: []u8, pos: *usize, s: []const u8): usize! {
    if (s.len > out.len - pos.*) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = toLowerAscii(s[i]);
        i += 1;
    }
    pos.* += s.len;
    return s.len;
}

/// 校验 `<zoom-value>`：支持 `auto` 或非负 `<number>` / `<percentage>`（如 `1`, `1.5`, `100%`, `0.5`）
fn isValidZoomValue(raw: []const u8): bool {
    const s = trimWs(raw);
    if (s.len == 0) {
        return false;
    }
    if (eqlAsciiIgnoreCase(s, "auto")) {
        return true;
    }
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    } else if (s[0] == '-') {
        return false;
    }
    let has_digit = false;
    let has_dot = false;
    while (i < s.len) {
        const c = s[i];
        if (isDigit(c)) {
            has_digit = true;
            i += 1;
            continue;
        }
        if (c == '.' && !has_dot) {
            has_dot = true;
            i += 1;
            continue;
        }
        if (c == '%' && i + 1 == s.len) {
            i += 1;
            break;
        }
        return false;
    }
    return has_digit && i == s.len;
}

/// 校验单个 `<viewport-length>` token（支持 `auto` / `device-width` / `device-height` / `extend-to-zoom` / 非负长度或百分比 / `calc(...)`）
fn isValidViewportLengthToken(raw: []const u8): bool {
    const s = trimWs(raw);
    if (s.len == 0) {
        return false;
    }
    if (eqlAsciiIgnoreCase(s, "auto") ||
        eqlAsciiIgnoreCase(s, "device-width") ||
        eqlAsciiIgnoreCase(s, "device-height") ||
        eqlAsciiIgnoreCase(s, "extend-to-zoom"))
    {
        return true;
    }
    if (s.len >= 6 &&
        toLowerAscii(s[0]) == 'c' &&
        toLowerAscii(s[1]) == 'a' &&
        toLowerAscii(s[2]) == 'l' &&
        toLowerAscii(s[3]) == 'c' &&
        s[4] == '(' &&
        s[s.len - 1] == ')')
    {
        return true;
    }
    if (s[0] == '-') {
        return false;
    }
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    }
    let has_digit = false;
    let has_dot = false;
    while (i < s.len) {
        const c = s[i];
        if (isDigit(c)) {
            has_digit = true;
            i += 1;
            continue;
        }
        if (c == '.' && !has_dot) {
            has_dot = true;
            i += 1;
            continue;
        }
        break;
    }
    if (!has_digit) {
        return false;
    }
    if (i == s.len) {
        // 仅允许无单位的 0
        let k: usize = if (s[0] == '+') 1 else 0;
        while (k < s.len) {
            if (s[k] != '0' && s[k] != '.') {
                return false;
            }
            k += 1;
        }
        return true;
    }
    const unit = s[i..];
    if (bytesEql(unit, "%")) {
        return true;
    }
    let j: usize = 0;
    while (j < unit.len) {
        const uc = unit[j];
        if (!((uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z'))) {
            return false;
        }
        j += 1;
    }
    return unit.len > 0;
}

/// `@viewport` 供应商前缀枚举（对齐 Bun `ViewportRule.vendor_prefix`）
pub enum CssViewportVendorPrefix: u8 {
    None = 0,
    Ms = 1,
    WebKit = 2,
    O = 3,
    Moz = 4,

    /// 返回前缀字符串（无前缀为空串，其余如 `"-ms-"`）
    fn asStr(): []const u8 {
        return switch (_) {
            .None => "",
            .Ms => "-ms-",
            .WebKit => "-webkit-",
            .O => "-o-",
            .Moz => "-moz-",
        };
    }

    /// 返回完整 `@-rule` 关键字（含 `@`，如 `"@-ms-viewport"`）
    fn atRuleName(): []const u8 {
        return switch (_) {
            .None => "@viewport",
            .Ms => "@-ms-viewport",
            .WebKit => "@-webkit-viewport",
            .O => "@-o-viewport",
            .Moz => "@-moz-viewport",
        };
    }

    /// 是否带有供应商前缀
    fn isPrefixed(): bool {
        return _ != .None;
    }

    /// 从不含 `@` 的 at-keyword（ASCII 大小写无关）识别前缀
    static fn fromAtKeyword(kw: []const u8): CssViewportVendorPrefix? {
        if (eqlAsciiIgnoreCase(kw, "viewport")) {
            return .None;
        }
        if (eqlAsciiIgnoreCase(kw, "-ms-viewport")) {
            return .Ms;
        }
        if (eqlAsciiIgnoreCase(kw, "-webkit-viewport")) {
            return .WebKit;
        }
        if (eqlAsciiIgnoreCase(kw, "-o-viewport")) {
            return .O;
        }
        if (eqlAsciiIgnoreCase(kw, "-moz-viewport")) {
            return .Moz;
        }
        return null;
    }
}

/// W3C CSS Device Adaptation Level 1 标准描述符类型
pub enum CssViewportDescriptorKind: u8 {
    MinWidth = 0,
    MaxWidth = 1,
    Width = 2,
    MinHeight = 3,
    MaxHeight = 4,
    Height = 5,
    Zoom = 6,
    MinZoom = 7,
    MaxZoom = 8,
    UserZoom = 9,
    Orientation = 10,
    ViewportFit = 11,
    Custom = 12,

    static fn fromName(raw_name: []const u8): CssViewportDescriptorKind {
        if (eqlAsciiIgnoreCase(raw_name, "min-width")) return .MinWidth;
        if (eqlAsciiIgnoreCase(raw_name, "max-width")) return .MaxWidth;
        if (eqlAsciiIgnoreCase(raw_name, "width")) return .Width;
        if (eqlAsciiIgnoreCase(raw_name, "min-height")) return .MinHeight;
        if (eqlAsciiIgnoreCase(raw_name, "max-height")) return .MaxHeight;
        if (eqlAsciiIgnoreCase(raw_name, "height")) return .Height;
        if (eqlAsciiIgnoreCase(raw_name, "zoom")) return .Zoom;
        if (eqlAsciiIgnoreCase(raw_name, "min-zoom")) return .MinZoom;
        if (eqlAsciiIgnoreCase(raw_name, "max-zoom")) return .MaxZoom;
        if (eqlAsciiIgnoreCase(raw_name, "user-zoom")) return .UserZoom;
        if (eqlAsciiIgnoreCase(raw_name, "orientation")) return .Orientation;
        if (eqlAsciiIgnoreCase(raw_name, "viewport-fit")) return .ViewportFit;
        return .Custom;
    }

    fn canonicalName(): []const u8 {
        return switch (_) {
            .MinWidth => "min-width",
            .MaxWidth => "max-width",
            .Width => "width",
            .MinHeight => "min-height",
            .MaxHeight => "max-height",
            .Height => "height",
            .Zoom => "zoom",
            .MinZoom => "min-zoom",
            .MaxZoom => "max-zoom",
            .UserZoom => "user-zoom",
            .Orientation => "orientation",
            .ViewportFit => "viewport-fit",
            .Custom => "",
        };
    }

    fn isShorthand(): bool {
        return _ == .Width || _ == .Height;
    }
}

/// `user-zoom` 描述符强类型枚举
pub enum CssViewportUserZoom: u8 {
    Zoom = 0,
    Fixed = 1,

    static fn parse(raw: []const u8): CssViewportUserZoom? {
        const s = trimWs(raw);
        if (eqlAsciiIgnoreCase(s, "zoom")) return .Zoom;
        if (eqlAsciiIgnoreCase(s, "fixed")) return .Fixed;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Zoom => "zoom",
            .Fixed => "fixed",
        };
    }
}

/// `orientation` 描述符强类型枚举
pub enum CssViewportOrientation: u8 {
    Auto = 0,
    Portrait = 1,
    Landscape = 2,

    static fn parse(raw: []const u8): CssViewportOrientation? {
        const s = trimWs(raw);
        if (eqlAsciiIgnoreCase(s, "auto")) return .Auto;
        if (eqlAsciiIgnoreCase(s, "portrait")) return .Portrait;
        if (eqlAsciiIgnoreCase(s, "landscape")) return .Landscape;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Auto => "auto",
            .Portrait => "portrait",
            .Landscape => "landscape",
        };
    }
}

/// `viewport-fit` 描述符强类型枚举（CSS Round Display Level 1）
pub enum CssViewportFit: u8 {
    Auto = 0,
    Contain = 1,
    Cover = 2,

    static fn parse(raw: []const u8): CssViewportFit? {
        const s = trimWs(raw);
        if (eqlAsciiIgnoreCase(s, "auto")) return .Auto;
        if (eqlAsciiIgnoreCase(s, "contain")) return .Contain;
        if (eqlAsciiIgnoreCase(s, "cover")) return .Cover;
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Auto => "auto",
            .Contain => "contain",
            .Cover => "cover",
        };
    }
}

/// `width` / `height` 简写展开后的 `(min, max)` 对
pub struct CssViewportMinMaxPair {
    min_value: []const u8 = "",
    max_value: []const u8 = "",

    /// 解析 1~2 项空白分隔（括号函数安全）的简写值：
    /// - 单值 `V` => `{ min_value: V, max_value: V }`
    /// - 双值 `V1 V2` => `{ min_value: V1, max_value: V2 }`
    static fn parseShorthand(raw: []const u8): CssViewportMinMaxPair? {
        const s = trimWs(raw);
        if (s.len == 0) {
            return null;
        }
        let tokens: [3][]const u8 = [""; 3];
        let tok_count: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            while (i < s.len && isWs(s[i])) {
                i += 1;
            }
            if (i >= s.len) {
                break;
            }
            const start = i;
            let paren_depth: usize = 0;
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
                if (c == '"' || c == '\'') {
                    in_quote = c;
                    i += 1;
                    continue;
                }
                if (c == '(') {
                    paren_depth += 1;
                    i += 1;
                    continue;
                }
                if (c == ')') {
                    if (paren_depth == 0) {
                        return null;
                    }
                    paren_depth -= 1;
                    i += 1;
                    continue;
                }
                if (paren_depth == 0 && isWs(c)) {
                    break;
                }
                i += 1;
            }
            if (paren_depth != 0 || in_quote != 0) {
                return null;
            }
            if (tok_count >= 2) {
                return null;
            }
            tokens[tok_count] = s[start..i];
            tok_count += 1;
        }
        if (tok_count == 1) {
            if (!isValidViewportLengthToken(tokens[0])) {
                return null;
            }
            return CssViewportMinMaxPair{
                min_value: tokens[0],
                max_value: tokens[0],
            };
        }
        if (tok_count == 2) {
            if (!isValidViewportLengthToken(tokens[0]) || !isValidViewportLengthToken(tokens[1])) {
                return null;
            }
            return CssViewportMinMaxPair{
                min_value: tokens[0],
                max_value: tokens[1],
            };
        }
        return null;
    }

    fn eql(other: CssViewportMinMaxPair): bool {
        return bytesEql(_.min_value, other.min_value) && bytesEql(_.max_value, other.max_value);
    }
}

/// 单条 `@viewport` 描述符声明
pub struct CssViewportDeclaration {
    name: []const u8 = "",
    value: []const u8 = "",
    kind: CssViewportDescriptorKind = .Custom,
    important: bool = false,

    /// 校验该声明的值是否符合对应标准描述符的 W3C 语法约束
    fn validate(): bool {
        if (_.name.len == 0 || _.value.len == 0) {
            return false;
        }
        return switch (_.kind) {
            .MinWidth, .MaxWidth, .MinHeight, .MaxHeight => isValidViewportLengthToken(_.value),
            .Width, .Height => CssViewportMinMaxPair.parseShorthand(_.value) != null,
            .Zoom, .MinZoom, .MaxZoom => isValidZoomValue(_.value),
            .UserZoom => CssViewportUserZoom.parse(_.value) != null,
            .Orientation => CssViewportOrientation.parse(_.value) != null,
            .ViewportFit => CssViewportFit.parse(_.value) != null,
            .Custom => true,
        };
    }

    fn eql(other: CssViewportDeclaration): bool {
        return _.kind == other.kind &&
            _.important == other.important &&
            eqlAsciiIgnoreCase(_.name, other.name) &&
            bytesEql(_.value, other.value);
    }
}

fn stripImportantSuffix(raw: []const u8, out_important: *bool): []const u8 {
    out_important.* = false;
    const s = trimWsAndComments(raw);
    if (s.len < 10) {
        return s;
    }
    // 查找末尾是否以 `!` + 可选空白 + `important`（ASCII 大小写无关）结尾
    let end = s.len;
    if (end < 9) {
        return s;
    }
    const word = s[end - 9 .. end];
    if (!eqlAsciiIgnoreCase(word, "important")) {
        return s;
    }
    let j = end - 9;
    while (j > 0 && isWs(s[j - 1])) {
        j -= 1;
    }
    if (j > 0 && s[j - 1] == '!') {
        out_important.* = true;
        return trimWsAndComments(s[0 .. j - 1]);
    }
    return s;
}

fn parseSingleDeclaration(segment: []const u8): CssViewportDeclaration! {
    const trimmed = trimWsAndComments(segment);
    if (trimmed.len == 0) {
        return err.InvalidDeclaration;
    }
    let colon_pos: usize? = null;
    let i: usize = 0;
    let in_quote: u8 = 0;
    let paren_depth: usize = 0;
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
            paren_depth += 1;
        } else if (c == ')' && paren_depth > 0) {
            paren_depth -= 1;
        } else if (c == ':' && paren_depth == 0) {
            colon_pos = i;
            break;
        }
        i += 1;
    }
    const cpos = colon_pos ?? return err.InvalidDeclaration;
    const name = trimWsAndComments(trimmed[0..cpos]);
    const val_raw = trimWsAndComments(trimmed[cpos + 1 ..]);
    if (name.len == 0 || val_raw.len == 0) {
        return err.InvalidDeclaration;
    }
    // 校验描述符名合法性
    let k: usize = 0;
    while (k < name.len) {
        if (!isIdentChar(name[k])) {
            return err.InvalidDeclaration;
        }
        k += 1;
    }
    let is_imp = false;
    const clean_val = stripImportantSuffix(val_raw, *is_imp);
    if (clean_val.len == 0) {
        return err.InvalidDeclaration;
    }
    const kind = CssViewportDescriptorKind.fromName(name);
    const decl = CssViewportDeclaration{
        name: name,
        value: clean_val,
        kind: kind,
        important: is_imp,
    };
    if (!decl.validate()) {
        return err.InvalidDescriptorValue;
    }
    return decl;
}

/// W3C CSS Device Adaptation Level 1 `@viewport` 规则模型（对齐 Bun `ViewportRule`）
pub struct CssViewportRule {
    vendor_prefix: CssViewportVendorPrefix = .None,
    decls: [CSS_VIEWPORT_MAX_DECLS]CssViewportDeclaration = [CssViewportDeclaration{}; CSS_VIEWPORT_MAX_DECLS],
    decl_count: usize = 0,

    static fn init(prefix: CssViewportVendorPrefix): CssViewportRule {
        return CssViewportRule{
            vendor_prefix: prefix,
            decls: [CssViewportDeclaration{}; CSS_VIEWPORT_MAX_DECLS],
            decl_count: 0,
        };
    }

    /// 解析单条 `@viewport { ... }` 或 `@-ms-viewport { ... }` 规则
    static fn parse(input: []const u8): CssViewportRule! {
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
        if (kw.len == 0) {
            return err.InvalidAtRule;
        }
        const prefix = CssViewportVendorPrefix.fromAtKeyword(kw) ?? {
            // 若以 `-viewport` 结尾但非已知前缀，返回 UnknownVendorPrefix
            if (kw.len > 9 && eqlAsciiIgnoreCase(kw[kw.len - 9 ..], "-viewport")) {
                return err.UnknownVendorPrefix;
            }
            return err.InvalidAtRule;
        };

        pos = skipWsAndComments(input, pos);
        if (pos >= input.len || input[pos] != '{') {
            return err.InvalidAtRule;
        }
        const close_pos = findMatchingBrace(input, pos) ?? return err.UnclosedBlock;
        const tail = skipWsAndComments(input, close_pos + 1);
        if (tail < input.len) {
            return err.InvalidAtRule;
        }

        let rule = CssViewportRule.init(prefix);
        const body = input[pos + 1 .. close_pos];
        let i: usize = 0;
        let seg_start: usize = 0;
        let in_quote: u8 = 0;
        let paren_depth: usize = 0;
        while (i <= body.len) {
            if (i < body.len) {
                const c = body[i];
                if (in_quote != 0) {
                    if (c == '\\' && i + 1 < body.len) {
                        i += 2;
                        continue;
                    }
                    if (c == in_quote) {
                        in_quote = 0;
                    }
                    i += 1;
                    continue;
                }
                if (c == '/' && i + 1 < body.len && body[i + 1] == '*') {
                    i += 2;
                    while (i + 1 < body.len && !(body[i] == '*' && body[i + 1] == '/')) {
                        i += 1;
                    }
                    if (i + 1 < body.len) {
                        i += 2;
                    } else {
                        return err.UnclosedBlock;
                    }
                    continue;
                }
                if (c == '"' || c == '\'') {
                    in_quote = c;
                    i += 1;
                    continue;
                }
                if (c == '(') {
                    paren_depth += 1;
                    i += 1;
                    continue;
                }
                if (c == ')') {
                    if (paren_depth == 0) {
                        return err.InvalidDeclaration;
                    }
                    paren_depth -= 1;
                    i += 1;
                    continue;
                }
                if (c != ';' || paren_depth != 0) {
                    i += 1;
                    continue;
                }
            }
            const seg = trimWsAndComments(body[seg_start..i]);
            if (seg.len > 0) {
                const decl = parseSingleDeclaration(seg)?;
                * = rule.push(decl)?;
            }
            seg_start = i + 1;
            i += 1;
        }
        return rule;
    }

    fn push(*_, decl: CssViewportDeclaration): usize! {
        if (_.decl_count >= CSS_VIEWPORT_MAX_DECLS) {
            return err.TooManyDeclarations;
        }
        _.decls[_.decl_count] = decl;
        _.decl_count += 1;
        return _.decl_count;
    }

    fn count(): usize {
        return _.decl_count;
    }

    fn isEmpty(): bool {
        return _.decl_count == 0;
    }

    fn at(idx: usize): CssViewportDeclaration? {
        if (idx >= _.decl_count) {
            return null;
        }
        return _.decls[idx];
    }

    /// 遵循 CSS 级联优先级查找某描述符的最终生效声明（`!important` 优先于普通声明，同级后声明覆盖前声明）
    fn find(name: []const u8): CssViewportDeclaration? {
        let best: CssViewportDeclaration? = null;
        let i: usize = 0;
        while (i < _.decl_count) {
            const d = _.decls[i];
            if (eqlAsciiIgnoreCase(d.name, name)) {
                if (prev := best) {
                    if (d.important || !prev.important) {
                        best = d;
                    }
                } else {
                    best = d;
                }
            }
            i += 1;
        }
        return best;
    }

    fn effectiveOrientation(): CssViewportOrientation? {
        if (d := _.find("orientation")) {
            return CssViewportOrientation.parse(d.value);
        }
        return null;
    }

    fn effectiveUserZoom(): CssViewportUserZoom? {
        if (d := _.find("user-zoom")) {
            return CssViewportUserZoom.parse(d.value);
        }
        return null;
    }

    fn effectiveViewportFit(): CssViewportFit? {
        if (d := _.find("viewport-fit")) {
            return CssViewportFit.parse(d.value);
        }
        return null;
    }

    fn expandWidthShorthand(): CssViewportMinMaxPair? {
        if (d := _.find("width")) {
            return CssViewportMinMaxPair.parseShorthand(d.value);
        }
        return null;
    }

    fn expandHeightShorthand(): CssViewportMinMaxPair? {
        if (d := _.find("height")) {
            return CssViewportMinMaxPair.parseShorthand(d.value);
        }
        return null;
    }

    /// 级联安全去重：对同名描述符仅保留最终生效的那一条，返回移除的冗余声明数量
    fn deduplicate(*_): usize {
        let removed: usize = 0;
        let write_idx: usize = 0;
        let i: usize = 0;
        while (i < _.decl_count) {
            const cur = _.decls[i];
            let overridden = false;
            let j: usize = 0;
            while (j < _.decl_count) {
                if (j != i && eqlAsciiIgnoreCase(_.decls[j].name, cur.name)) {
                    const other = _.decls[j];
                    if (cur.important) {
                        if (other.important && j > i) {
                            overridden = true;
                            break;
                        }
                    } else {
                        if (other.important || j > i) {
                            overridden = true;
                            break;
                        }
                    }
                }
                j += 1;
            }
            if (overridden) {
                removed += 1;
            } else {
                _.decls[write_idx] = cur;
                write_idx += 1;
            }
            i += 1;
        }
        _.decl_count = write_idx;
        return removed;
    }

    fn eql(other: CssViewportRule): bool {
        if (_.vendor_prefix != other.vendor_prefix || _.decl_count != other.decl_count) {
            return false;
        }
        let i: usize = 0;
        while (i < _.decl_count) {
            if (!_.decls[i].eql(other.decls[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn writeWithPrefix(out: []u8, pos: *usize, prefix: CssViewportVendorPrefix, minify: bool): usize! {
        const start = pos.*;
        * = writeByte(out, pos, '@')?;
        * = writeSlice(out, pos, prefix.asStr())?;
        * = writeSlice(out, pos, "viewport")?;
        if (minify) {
            * = writeByte(out, pos, '{')?;
            let i: usize = 0;
            while (i < _.decl_count) {
                const d = _.decls[i];
                if (i > 0) {
                    * = writeByte(out, pos, ';')?;
                }
                * = writeLowerSlice(out, pos, d.name)?;
                * = writeByte(out, pos, ':')?;
                * = writeSlice(out, pos, d.value)?;
                if (d.important) {
                    * = writeSlice(out, pos, "!important")?;
                }
                i += 1;
            }
            * = writeByte(out, pos, '}')?;
        } else {
            * = writeSlice(out, pos, " {")?;
            if (_.decl_count > 0) {
                * = writeByte(out, pos, '\n')?;
                let i: usize = 0;
                while (i < _.decl_count) {
                    const d = _.decls[i];
                    * = writeSlice(out, pos, "  ")?;
                    * = writeLowerSlice(out, pos, d.name)?;
                    * = writeSlice(out, pos, ": ")?;
                    * = writeSlice(out, pos, d.value)?;
                    if (d.important) {
                        * = writeSlice(out, pos, " !important")?;
                    }
                    * = writeSlice(out, pos, ";\n")?;
                    i += 1;
                }
            }
            * = writeByte(out, pos, '}')?;
        }
        return pos.* - start;
    }

    /// 美化规范序列化（对齐 Bun `ViewportRule.toCss`）
    fn toCss(out: []u8): usize! {
        let pos: usize = 0;
        * = _.writeWithPrefix(out, *pos, _.vendor_prefix, false)?;
        return pos;
    }

    /// 压缩序列化（省略末条分号、冒号后空格与换行）
    fn toCssMinified(out: []u8): usize! {
        let pos: usize = 0;
        * = _.writeWithPrefix(out, *pos, _.vendor_prefix, true)?;
        return pos;
    }

    /// 多供应商前缀批量降级输出（对齐 Bun CSS bundler vendor prefixing 实践，如 `[.Ms, .None]`）
    fn toCssWithPrefixes(
        out: []u8,
        prefixes: []const CssViewportVendorPrefix,
        minify: bool,
    ): usize! {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < prefixes.len) {
            if (i > 0 && !minify) {
                * = writeByte(out, *pos, '\n')?;
            }
            * = _.writeWithPrefix(out, *pos, prefixes[i], minify)?;
            i += 1;
        }
        return pos;
    }
}

/// 从完整样式表批量提取的 `@viewport` 规则集合
pub struct CssViewportRuleList {
    rules: [CSS_VIEWPORT_MAX_RULES]CssViewportRule = [CssViewportRule{}; CSS_VIEWPORT_MAX_RULES],
    len: usize = 0,

    fn count(): usize {
        return _.len;
    }

    fn at(idx: usize): CssViewportRule? {
        if (idx >= _.len) {
            return null;
        }
        return _.rules[idx];
    }

    /// 词法安全扫描完整样式表（跳过字符串、注释与普通规则块），提取全部顶层 `@viewport` / `@-*-viewport` 规则
    static fn extractFromStylesheet(css: []const u8): CssViewportRuleList! {
        let list = CssViewportRuleList{};
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
                if (CssViewportVendorPrefix.fromAtKeyword(kw) != null) {
                    const open_pos = skipWsAndComments(css, k);
                    if (open_pos >= css.len || css[open_pos] != '{') {
                        return err.InvalidAtRule;
                    }
                    const close_pos = findMatchingBrace(css, open_pos) ?? return err.UnclosedBlock;
                    const rule = CssViewportRule.parse(css[i .. close_pos + 1])?;
                    if (list.len >= CSS_VIEWPORT_MAX_RULES) {
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

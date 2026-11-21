// Source: Absorbed from /data/dev/bun/src/css/rules/keyframes.cp | Synced: 2026-11-21T16:57:43Z

/// CSS At-Rules（`@keyframes` / `@layer` / `@counter-style` / `@property`）解析错误集
pub err CssAtRuleError {
    InvalidKeyframeSelector,
    InvalidKeyframeName,
    InvalidLayerName,
    TooManyLayerSegments,
    InvalidCounterSystem,
    InvalidPropertyName,
    InvalidPropertySyntax,
    MissingInitialValue,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '-';
}

fn isIdentChar(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isValidCssIdent(s: string): bool {
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

/// 校验 `@keyframes` 动画名标识符（拒绝空串与 CSS 全局关键字）
pub fn isValidKeyframesIdent(name: string): bool {
    if (!isValidCssIdent(name)) {
        return false;
    }
    if (std.ascii.eqlIgnoreCase(name, "none")
        || std.ascii.eqlIgnoreCase(name, "initial")
        || std.ascii.eqlIgnoreCase(name, "inherit")
        || std.ascii.eqlIgnoreCase(name, "unset")
        || std.ascii.eqlIgnoreCase(name, "default")
        || std.ascii.eqlIgnoreCase(name, "revert")
        || std.ascii.eqlIgnoreCase(name, "revert-layer"))
    {
        return false;
    }
    return true;
}

/// 滚动驱动动画时间线范围名称（Scroll-Driven Animations Level 1）
pub enum TimelineRangeName: u8 {
    cover = 0,
    contain = 1,
    entry = 2,
    exit = 3,
    entry_crossing = 4,
    exit_crossing = 5,

    static fn parse(input: string): TimelineRangeName? {
        if (std.ascii.eqlIgnoreCase(input, "cover")) { return .cover; }
        if (std.ascii.eqlIgnoreCase(input, "contain")) { return .contain; }
        if (std.ascii.eqlIgnoreCase(input, "entry")) { return .entry; }
        if (std.ascii.eqlIgnoreCase(input, "exit")) { return .exit; }
        if (std.ascii.eqlIgnoreCase(input, "entry-crossing")) { return .entry_crossing; }
        if (std.ascii.eqlIgnoreCase(input, "exit-crossing")) { return .exit_crossing; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .cover => "cover",
            .contain => "contain",
            .entry => "entry",
            .exit => "exit",
            .entry_crossing => "entry-crossing",
            .exit_crossing => "exit-crossing",
        };
    }
}

/// `@keyframes` 选择器类型
pub enum KeyframeSelectorKind: u8 {
    from_kw = 0,
    to_kw = 1,
    percentage = 2,
    timeline_range = 3,
}

fn parsePercentToken(tok: string): f64 ! CssAtRuleError {
    if (tok.len < 2 || tok[tok.len - 1] != '%') {
        return err.InvalidKeyframeSelector;
    }
    const num_str = tok[0 .. tok.len - 1];
    const val = std.fmt.parseFloat(f64, num_str) catch return err.InvalidKeyframeSelector;
    if (std.math.isNan(val) || std.math.isInf(val) || val < 0.0 || val > 100.0) {
        return err.InvalidKeyframeSelector;
    }
    return val;
}

/// `@keyframes` 关键帧选择器（支持 `from` / `to` / `0%..100%` / `<timeline-range> <percent>`）
pub struct KeyframeSelector {
    kind: KeyframeSelectorKind,
    percent: f64 = 0.0,
    range_name: TimelineRangeName? = null,

    static fn parse(input: string): KeyframeSelector ! CssAtRuleError {
        const s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidKeyframeSelector;
        }
        if (std.ascii.eqlIgnoreCase(s, "from")) {
            return KeyframeSelector{
                kind: KeyframeSelectorKind.from_kw,
                percent: 0.0,
            };
        }
        if (std.ascii.eqlIgnoreCase(s, "to")) {
            return KeyframeSelector{
                kind: KeyframeSelectorKind.to_kw,
                percent: 100.0,
            };
        }

        // 检查是否包含空白分隔的时间线范围（如 "entry 25%"）
        if (sp := std.mem.indexOfScalar(u8, s, ' ')) {
            const range_tok = trimAscii(s[0..sp]);
            const pct_tok = trimAscii(s[sp + 1 ..]);
            const rname = TimelineRangeName.parse(range_tok) ?? return err.InvalidKeyframeSelector;
            const pct = parsePercentToken(pct_tok)?;
            return KeyframeSelector{
                kind: KeyframeSelectorKind.timeline_range,
                percent: pct,
                range_name: rname,
            };
        }

        const pct = parsePercentToken(s)?;
        return KeyframeSelector{
            kind: KeyframeSelectorKind.percentage,
            percent: pct,
        };
    }

    /// 返回归一化后的百分比进度（`0.0..100.0`）
    fn normalizedPercent(): f64 {
        return switch (_.kind) {
            .from_kw => 0.0,
            .to_kw => 100.0,
            .percentage, .timeline_range => _.percent,
        };
    }

    /// 序列化为 CSS 文本（`minify=true` 时 `from` -> `0%`、`100%` -> `to`）
    fn toCss(buf: []u8, minify: bool): usize ! CssAtRuleError {
        switch (_.kind) {
            .from_kw => {
                const out = if (minify) "0%" else "from";
                if (out.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[0..out.len], out);
                return out.len;
            },
            .to_kw => {
                const out = "to";
                if (out.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[0..out.len], out);
                return out.len;
            },
            .percentage => {
                if (minify && _.percent == 100.0) {
                    const out = "to";
                    if (out.len > buf.len) { return err.BufferTooSmall; }
                    @memcpy(buf[0..out.len], out);
                    return out.len;
                }
                const formatted = std.fmt.bufPrint(buf, "{d}%", .{_.percent}) catch return err.BufferTooSmall;
                return formatted.len;
            },
            .timeline_range => {
                const rname = _.range_name.?.toCss();
                const formatted = std.fmt.bufPrint(buf, "{s} {d}%", .{ rname, _.percent }) catch return err.BufferTooSmall;
                return formatted.len;
            },
        }
    }
}

/// `@layer` 级联层点分多层名（最多 8 层，零堆分配内联存储，`len == 0` 表示匿名层）
pub struct LayerName {
    segments: [8]string = ["", "", "", "", "", "", "", ""],
    len: usize = 0,

    static fn anonymous(): LayerName {
        return LayerName{ len: 0 };
    }

    /// 解析点分层名（如 `"framework.theme.dark"`），拒绝空段、非法标识符或超深嵌套
    static fn parse(input: string): LayerName ! CssAtRuleError {
        if (input.len == 0) {
            return err.InvalidLayerName;
        }
        let layer = LayerName{};
        let start: usize = 0;
        let pos: usize = 0;
        while (pos <= input.len) {
            if (pos == input.len || input[pos] == '.') {
                const seg = input[start..pos];
                if (!isValidCssIdent(seg)) {
                    return err.InvalidLayerName;
                }
                if (layer.len >= 8) {
                    return err.TooManyLayerSegments;
                }
                layer.segments[layer.len] = seg;
                layer.len += 1;
                start = pos + 1;
            }
            pos += 1;
        }
        return layer;
    }

    fn isAnonymous(): bool {
        return _.len == 0;
    }

    fn depth(): usize {
        return _.len;
    }

    fn eql(other: LayerName): bool {
        if (_.len != other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!std.mem.eql(u8, _.segments[i], other.segments[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// 判断当前层是否为 `other` 的前缀父层（含相等情形；匿名层永不为命名层前缀）
    fn isPrefixOf(other: LayerName): bool {
        if (_.len == 0 || _.len > other.len) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            if (!std.mem.eql(u8, _.segments[i], other.segments[i])) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn toCss(buf: []u8): usize ! CssAtRuleError {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                if (pos + 1 > buf.len) { return err.BufferTooSmall; }
                buf[pos] = '.';
                pos += 1;
            }
            const seg = _.segments[i];
            if (pos + seg.len > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + seg.len], seg);
            pos += seg.len;
            i += 1;
        }
        return pos;
    }
}

/// `@counter-style` 计数系统（CSS Counter Styles Level 3 §3.1）
pub enum CounterSystem: u8 {
    cyclic = 0,
    numeric = 1,
    alphabetic = 2,
    symbolic = 3,
    additive = 4,
    fixed = 5,
    extends_style = 6,

    static fn default(): CounterSystem {
        return .symbolic;
    }

    static fn parse(input: string): CounterSystem? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "cyclic")) { return .cyclic; }
        if (std.ascii.eqlIgnoreCase(s, "numeric")) { return .numeric; }
        if (std.ascii.eqlIgnoreCase(s, "alphabetic")) { return .alphabetic; }
        if (std.ascii.eqlIgnoreCase(s, "symbolic")) { return .symbolic; }
        if (std.ascii.eqlIgnoreCase(s, "additive")) { return .additive; }
        if (std.ascii.eqlIgnoreCase(s, "fixed")) { return .fixed; }
        if (std.ascii.eqlIgnoreCase(s, "extends")) { return .extends_style; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .cyclic => "cyclic",
            .numeric => "numeric",
            .alphabetic => "alphabetic",
            .symbolic => "symbolic",
            .additive => "additive",
            .fixed => "fixed",
            .extends_style => "extends",
        };
    }

    /// 是否要求定义 `symbols` 描述符（`additive` 使用 `additive-symbols`，`extends` 继承父样式）
    fn requiresSymbols(): bool {
        return switch (_) {
            .cyclic, .numeric, .alphabetic, .symbolic, .fixed => true,
            else => false,
        };
    }

    /// 是否要求 `symbols` 至少包含 2 个符号（`numeric` 与 `alphabetic`）
    fn requiresMinTwoSymbols(): bool {
        return _ == .numeric || _ == .alphabetic;
    }
}

/// `@property` 注册自定义属性语法类型（CSS Properties and Values API Level 1）
pub enum PropertySyntaxKind: u8 {
    universal = 0,
    length = 1,
    number = 2,
    percentage = 3,
    length_percentage = 4,
    color = 5,
    image = 6,
    url = 7,
    integer = 8,
    angle = 9,
    time = 10,
    resolution = 11,
    transform_function = 12,
    transform_list = 13,
    custom_ident = 14,
    string_val = 15,

    /// 解析 `@property` 的 `syntax` 字符串（支持可选引号包裹与空白裁剪）
    static fn parse(input: string): PropertySyntaxKind? {
        let s = trimAscii(input);
        if (s.len >= 2 && ((s[0] == '"' && s[s.len - 1] == '"') || (s[0] == '\'' && s[s.len - 1] == '\''))) {
            s = trimAscii(s[1 .. s.len - 1]);
        }
        if (std.mem.eql(u8, s, "*")) { return .universal; }
        if (std.ascii.eqlIgnoreCase(s, "<length>")) { return .length; }
        if (std.ascii.eqlIgnoreCase(s, "<number>")) { return .number; }
        if (std.ascii.eqlIgnoreCase(s, "<percentage>")) { return .percentage; }
        if (std.ascii.eqlIgnoreCase(s, "<length-percentage>")) { return .length_percentage; }
        if (std.ascii.eqlIgnoreCase(s, "<color>")) { return .color; }
        if (std.ascii.eqlIgnoreCase(s, "<image>")) { return .image; }
        if (std.ascii.eqlIgnoreCase(s, "<url>")) { return .url; }
        if (std.ascii.eqlIgnoreCase(s, "<integer>")) { return .integer; }
        if (std.ascii.eqlIgnoreCase(s, "<angle>")) { return .angle; }
        if (std.ascii.eqlIgnoreCase(s, "<time>")) { return .time; }
        if (std.ascii.eqlIgnoreCase(s, "<resolution>")) { return .resolution; }
        if (std.ascii.eqlIgnoreCase(s, "<transform-function>")) { return .transform_function; }
        if (std.ascii.eqlIgnoreCase(s, "<transform-list>")) { return .transform_list; }
        if (std.ascii.eqlIgnoreCase(s, "<custom-ident>")) { return .custom_ident; }
        if (std.ascii.eqlIgnoreCase(s, "<string>")) { return .string_val; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .universal => "*",
            .length => "<length>",
            .number => "<number>",
            .percentage => "<percentage>",
            .length_percentage => "<length-percentage>",
            .color => "<color>",
            .image => "<image>",
            .url => "<url>",
            .integer => "<integer>",
            .angle => "<angle>",
            .time => "<time>",
            .resolution => "<resolution>",
            .transform_function => "<transform-function>",
            .transform_list => "<transform-list>",
            .custom_ident => "<custom-ident>",
            .string_val => "<string>",
        };
    }

    fn isUniversal(): bool {
        return _ == .universal;
    }
}

/// 校验 CSS 自定义属性名（必须以 `--` 开头且后续至少 1 个合法字符）
pub fn isValidCustomPropertyName(name: string): bool {
    if (name.len < 3 || name[0] != '-' || name[1] != '-') {
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

/// `@property` 注册自定义属性完整描述符（校验非 `*` 语法必须提供 `initial-value`）
pub struct PropertyDescriptor {
    name: string,
    syntax: PropertySyntaxKind,
    inherits: bool,
    initial_value: string? = null,

    static fn create(
        name: string,
        syntax_str: string,
        inherits: bool,
        initial_value: string?,
    ): PropertyDescriptor ! CssAtRuleError {
        if (!isValidCustomPropertyName(name)) {
            return err.InvalidPropertyName;
        }
        const syntax = PropertySyntaxKind.parse(syntax_str) ?? return err.InvalidPropertySyntax;
        if (!syntax.isUniversal()) {
            if (iv := initial_value) {
                if (trimAscii(iv).len == 0) {
                    return err.MissingInitialValue;
                }
            } else {
                return err.MissingInitialValue;
            }
        }
        return PropertyDescriptor{
            name: name,
            syntax: syntax,
            inherits: inherits,
            initial_value: initial_value,
        };
    }
}

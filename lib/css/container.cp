// Source: Absorbed from /data/dev/bun/src/css/rules/container.cp | Synced: 2026-11-22T12:49:17Z

/// CSS Container Queries Level 3 解析错误集
pub err CssContainerError {
    InvalidContainerName,
    InvalidContainerType,
    InvalidFeatureName,
    InvalidFeatureValue,
    InvalidQuerySyntax,
    TooManyConditions,
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

/// CSS `container-type` 属性枚举（`normal` / `size` / `inline-size`）
pub enum ContainerType: u8 {
    normal = 0,
    size = 1,
    inline_size = 2,

    static fn default(): ContainerType {
        return .normal;
    }

    static fn parse(input: string): ContainerType? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "size")) { return .size; }
        if (std.ascii.eqlIgnoreCase(s, "inline-size")) { return .inline_size; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .size => "size",
            .inline_size => "inline-size",
        };
    }

    /// 是否建立尺寸查询容器上下文（`size` / `inline-size`）
    fn supportsSizeQueries(): bool {
        return _ == .size || _ == .inline_size;
    }

    /// 是否支持块轴（高度 / `block-size` / `aspect-ratio`）查询（仅 `size`）
    fn supportsBlockQueries(): bool {
        return _ == .size;
    }
}

/// `@container` 容器名称（`<custom-ident>`，规范禁止 `none` / `and` / `not` / `or`）
pub struct ContainerName {
    value: string,

    static fn isValidName(input: string): bool {
        if (input.len == 0 || !isIdentStart(input[0])) {
            return false;
        }
        let i: usize = 1;
        while (i < input.len) {
            if (!isIdentChar(input[i])) {
                return false;
            }
            i += 1;
        }
        if (std.ascii.eqlIgnoreCase(input, "none")
            || std.ascii.eqlIgnoreCase(input, "and")
            || std.ascii.eqlIgnoreCase(input, "not")
            || std.ascii.eqlIgnoreCase(input, "or"))
        {
            return false;
        }
        return true;
    }

    static fn parse(input: string): ContainerName ! CssContainerError {
        const s = trimAscii(input);
        if (!ContainerName.isValidName(s)) {
            return err.InvalidContainerName;
        }
        return ContainerName{ value: s };
    }

    fn toCss(): string {
        return _.value;
    }
}

/// 容器尺寸特征标识（CSS Containment Level 3）
pub enum ContainerSizeFeatureId: u8 {
    width = 0,
    height = 1,
    inline_size = 2,
    block_size = 3,
    aspect_ratio = 4,
    orientation = 5,

    static fn parse(input: string): ContainerSizeFeatureId? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "width")) { return .width; }
        if (std.ascii.eqlIgnoreCase(s, "height")) { return .height; }
        if (std.ascii.eqlIgnoreCase(s, "inline-size")) { return .inline_size; }
        if (std.ascii.eqlIgnoreCase(s, "block-size")) { return .block_size; }
        if (std.ascii.eqlIgnoreCase(s, "aspect-ratio")) { return .aspect_ratio; }
        if (std.ascii.eqlIgnoreCase(s, "orientation")) { return .orientation; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .width => "width",
            .height => "height",
            .inline_size => "inline-size",
            .block_size => "block-size",
            .aspect_ratio => "aspect-ratio",
            .orientation => "orientation",
        };
    }
}

/// 容器方向枚举
pub enum Orientation: u8 {
    portrait = 0,
    landscape = 1,

    static fn parse(input: string): Orientation? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "portrait")) { return .portrait; }
        if (std.ascii.eqlIgnoreCase(s, "landscape")) { return .landscape; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .portrait => "portrait",
            .landscape => "landscape",
        };
    }
}

/// 比较运算符
pub enum CompareOp: u8 {
    eq = 0,
    ge = 1,
    le = 2,
    gt = 3,
    lt = 4,
}

fn parseLengthPx(raw: string): f64 ! CssContainerError {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return err.InvalidFeatureValue;
    }
    let scale: f64 = 1.0;
    let num_part = s;
    if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "px")) {
        num_part = s[0 .. s.len - 2];
        scale = 1.0;
    } else if (s.len > 3 && std.ascii.eqlIgnoreCase(s[s.len - 3 ..], "rem")) {
        num_part = s[0 .. s.len - 3];
        scale = 16.0;
    } else if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "em")) {
        num_part = s[0 .. s.len - 2];
        scale = 16.0;
    } else {
        // 仅允许无单位的 0
        const zero_v = std.fmt.parseFloat(f64, s) catch return err.InvalidFeatureValue;
        if (zero_v == 0.0) {
            return 0.0;
        }
        return err.InvalidFeatureValue;
    }
    const v = std.fmt.parseFloat(f64, num_part) catch return err.InvalidFeatureValue;
    if (std.math.isNan(v) || std.math.isInf(v) || v < 0.0) {
        return err.InvalidFeatureValue;
    }
    return v * scale;
}

fn parseRatio(raw: string): [2]f64 ! CssContainerError {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return err.InvalidFeatureValue;
    }
    if (slash := std.mem.indexOfScalar(u8, s, '/')) {
        const lhs = trimAscii(s[0..slash]);
        const rhs = trimAscii(s[slash + 1 ..]);
        const n = std.fmt.parseFloat(f64, lhs) catch return err.InvalidFeatureValue;
        const d = std.fmt.parseFloat(f64, rhs) catch return err.InvalidFeatureValue;
        if (std.math.isNan(n) || std.math.isNan(d) || n <= 0.0 || d <= 0.0) {
            return err.InvalidFeatureValue;
        }
        return [n, d];
    }
    const v = std.fmt.parseFloat(f64, s) catch return err.InvalidFeatureValue;
    if (std.math.isNan(v) || v <= 0.0) {
        return err.InvalidFeatureValue;
    }
    return [v, 1.0];
}

fn compareValues(actual: f64, expected: f64, op: CompareOp): bool {
    const eps: f64 = 0.000001;
    return switch (op) {
        .eq => {
            const diff = if (actual > expected) (actual - expected) else (expected - actual);
            return diff <= eps;
        },
        .ge => actual + eps >= expected,
        .le => actual <= expected + eps,
        .gt => actual > expected + eps,
        .lt => actual + eps < expected,
    };
}

/// 单个容器尺寸特征条件（支持 `min-`/`max-` 经典语法与 `>=`/`<=`/`>`/`<`/`=` 范围语法）
pub struct ContainerSizeFeature {
    feature: ContainerSizeFeatureId,
    op: CompareOp = CompareOp.eq,
    px_value: f64 = 0.0,
    ratio_num: f64 = 1.0,
    ratio_den: f64 = 1.0,
    orientation: Orientation = Orientation.landscape,

    static fn parse(input: string): ContainerSizeFeature ! CssContainerError {
        let s = trimAscii(input);
        if (s.len >= 2 && s[0] == '(' && s[s.len - 1] == ')') {
            s = trimAscii(s[1 .. s.len - 1]);
        }
        if (s.len == 0) {
            return err.InvalidQuerySyntax;
        }

        let name_raw: string = "";
        let val_raw: string = "";
        let op: CompareOp = CompareOp.eq;

        if (colon := std.mem.indexOfScalar(u8, s, ':')) {
            name_raw = trimAscii(s[0..colon]);
            val_raw = trimAscii(s[colon + 1 ..]);
            if (name_raw.len > 4 && std.ascii.eqlIgnoreCase(name_raw[0..4], "min-")) {
                op = CompareOp.ge;
                name_raw = name_raw[4..];
            } else if (name_raw.len > 4 && std.ascii.eqlIgnoreCase(name_raw[0..4], "max-")) {
                op = CompareOp.le;
                name_raw = name_raw[4..];
            }
        } else if (ge_idx := std.mem.indexOf(u8, s, ">=")) {
            name_raw = trimAscii(s[0..ge_idx]);
            val_raw = trimAscii(s[ge_idx + 2 ..]);
            op = CompareOp.ge;
        } else if (le_idx := std.mem.indexOf(u8, s, "<=")) {
            name_raw = trimAscii(s[0..le_idx]);
            val_raw = trimAscii(s[le_idx + 2 ..]);
            op = CompareOp.le;
        } else if (gt_idx := std.mem.indexOfScalar(u8, s, '>')) {
            name_raw = trimAscii(s[0..gt_idx]);
            val_raw = trimAscii(s[gt_idx + 1 ..]);
            op = CompareOp.gt;
        } else if (lt_idx := std.mem.indexOfScalar(u8, s, '<')) {
            name_raw = trimAscii(s[0..lt_idx]);
            val_raw = trimAscii(s[lt_idx + 1 ..]);
            op = CompareOp.lt;
        } else if (eq_idx := std.mem.indexOfScalar(u8, s, '=')) {
            name_raw = trimAscii(s[0..eq_idx]);
            val_raw = trimAscii(s[eq_idx + 1 ..]);
            op = CompareOp.eq;
        } else {
            return err.InvalidQuerySyntax;
        }

        const feat = ContainerSizeFeatureId.parse(name_raw) ?? return err.InvalidFeatureName;
        switch (feat) {
            .width, .height, .inline_size, .block_size => {
                const px = parseLengthPx(val_raw)?;
                return ContainerSizeFeature{
                    feature: feat,
                    op: op,
                    px_value: px,
                };
            },
            .aspect_ratio => {
                const pair = parseRatio(val_raw)?;
                return ContainerSizeFeature{
                    feature: feat,
                    op: op,
                    ratio_num: pair[0],
                    ratio_den: pair[1],
                };
            },
            .orientation => {
                if (op != CompareOp.eq) {
                    return err.InvalidFeatureValue;
                }
                const ori = Orientation.parse(val_raw) ?? return err.InvalidFeatureValue;
                return ContainerSizeFeature{
                    feature: feat,
                    op: CompareOp.eq,
                    orientation: ori,
                };
            },
        }
    }

    /// 在给定容器宽高（px）下求值该特征是否匹配
    fn evaluate(width: f64, height: f64): bool {
        return switch (_.feature) {
            .width, .inline_size => compareValues(width, _.px_value, _.op),
            .height, .block_size => compareValues(height, _.px_value, _.op),
            .aspect_ratio => {
                if (height <= 0.0) {
                    return false;
                }
                return compareValues(width / height, _.ratio_num / _.ratio_den, _.op);
            },
            .orientation => {
                const is_portrait = height >= width;
                return if (_.orientation == Orientation.portrait) is_portrait else !is_portrait;
            },
        };
    }

    /// 规范化序列化为 CSS 特征表达式（如 `(min-width: 400px)`、`(orientation: landscape)`）
    fn toCss(buf: []u8): usize ! CssContainerError {
        const fname = _.feature.toCss();
        const formatted = switch (_.feature) {
            .width, .height, .inline_size, .block_size => switch (_.op) {
                .eq => std.fmt.bufPrint(buf, "({s}: {d}px)", .{ fname, _.px_value }),
                .ge => std.fmt.bufPrint(buf, "(min-{s}: {d}px)", .{ fname, _.px_value }),
                .le => std.fmt.bufPrint(buf, "(max-{s}: {d}px)", .{ fname, _.px_value }),
                .gt => std.fmt.bufPrint(buf, "({s} > {d}px)", .{ fname, _.px_value }),
                .lt => std.fmt.bufPrint(buf, "({s} < {d}px)", .{ fname, _.px_value }),
            },
            .aspect_ratio => switch (_.op) {
                .eq => std.fmt.bufPrint(buf, "({s}: {d}/{d})", .{ fname, _.ratio_num, _.ratio_den }),
                .ge => std.fmt.bufPrint(buf, "(min-{s}: {d}/{d})", .{ fname, _.ratio_num, _.ratio_den }),
                .le => std.fmt.bufPrint(buf, "(max-{s}: {d}/{d})", .{ fname, _.ratio_num, _.ratio_den }),
                .gt => std.fmt.bufPrint(buf, "({s} > {d}/{d})", .{ fname, _.ratio_num, _.ratio_den }),
                .lt => std.fmt.bufPrint(buf, "({s} < {d}/{d})", .{ fname, _.ratio_num, _.ratio_den }),
            },
            .orientation => std.fmt.bufPrint(buf, "(orientation: {s})", .{_.orientation.toCss()}),
        } catch return err.BufferTooSmall;
        return formatted.len;
    }
}

/// 逻辑组合运算符
pub enum LogicOp: u8 {
    and_op = 0,
    or_op = 1,
}

/// 完整 `@container` 查询模型（可选容器名 + 可选 `not` 取反 + `and`/`or` 组合特征，零堆分配）
pub struct ContainerQuery {
    name: ContainerName? = null,
    negated: bool = false,
    op: LogicOp = LogicOp.and_op,
    features: [8]ContainerSizeFeature = [
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
        ContainerSizeFeature{ feature: ContainerSizeFeatureId.width },
    ],
    feature_count: usize = 0,

    /// 解析 `@container` 序言（如 `"sidebar (min-width: 300px) and (max-width: 800px)"` 或 `"not (orientation: portrait)"`）
    static fn parse(input: string): ContainerQuery ! CssContainerError {
        let s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidQuerySyntax;
        }
        // 允许可选的 "@container " 前缀
        if (s.len > 10 && std.ascii.eqlIgnoreCase(s[0..10], "@container")) {
            s = trimAscii(s[10..]);
        }

        let q = ContainerQuery{};

        // 1. 解析可选容器名或前导 `not`
        if (s.len > 0 && s[0] != '(') {
            const first_sp = std.mem.indexOfScalar(u8, s, ' ') ?? return err.InvalidQuerySyntax;
            const head = s[0..first_sp];
            const rest = trimAscii(s[first_sp + 1 ..]);
            if (std.ascii.eqlIgnoreCase(head, "not")) {
                q.negated = true;
                s = rest;
            } else {
                q.name = ContainerName.parse(head)?;
                s = rest;
                if (s.len > 4 && std.ascii.eqlIgnoreCase(s[0..4], "not ")) {
                    q.negated = true;
                    s = trimAscii(s[4..]);
                }
            }
        }

        if (s.len == 0 || s[0] != '(') {
            return err.InvalidQuerySyntax;
        }

        // 2. 依次解析 `(...)` 与 `and`/`or` 组合子句
        let pos: usize = 0;
        let has_op: bool = false;
        while (pos < s.len) {
            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= s.len) {
                break;
            }
            if (s[pos] != '(') {
                return err.InvalidQuerySyntax;
            }
            const close = std.mem.indexOfScalarPos(u8, s, pos + 1, ')') ?? return err.InvalidQuerySyntax;
            const feat = ContainerSizeFeature.parse(s[pos .. close + 1])?;
            if (q.feature_count >= 8) {
                return err.TooManyConditions;
            }
            q.features[q.feature_count] = feat;
            q.feature_count += 1;
            pos = close + 1;

            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= s.len) {
                break;
            }
            // 读取逻辑连接词 `and` 或 `or`
            const op_end = std.mem.indexOfScalarPos(u8, s, pos, ' ') ?? return err.InvalidQuerySyntax;
            const op_tok = s[pos..op_end];
            let next_op = LogicOp.and_op;
            if (std.ascii.eqlIgnoreCase(op_tok, "and")) {
                next_op = LogicOp.and_op;
            } else if (std.ascii.eqlIgnoreCase(op_tok, "or")) {
                next_op = LogicOp.or_op;
            } else {
                return err.InvalidQuerySyntax;
            }
            if (has_op && q.op != next_op) {
                return err.InvalidQuerySyntax;
            }
            q.op = next_op;
            has_op = true;
            pos = op_end + 1;
            // 连接词之后必须还有特征子句
            const tail = trimAscii(s[pos..]);
            if (tail.len == 0) {
                return err.InvalidQuerySyntax;
            }
        }

        if (q.feature_count == 0) {
            return err.InvalidQuerySyntax;
        }
        return q;
    }

    /// 在给定容器宽高（px）下求值整个 `@container` 逻辑表达式
    fn evaluate(width: f64, height: f64): bool {
        if (_.feature_count == 0) {
            return false;
        }
        let acc = _.features[0].evaluate(width, height);
        let i: usize = 1;
        while (i < _.feature_count) {
            const cur = _.features[i].evaluate(width, height);
            if (_.op == LogicOp.and_op) {
                acc = acc && cur;
            } else {
                acc = acc || cur;
            }
            i += 1;
        }
        return if (_.negated) !acc else acc;
    }

    /// 规范化序列化为 `@container ...` 文本
    fn toCss(buf: []u8): usize ! CssContainerError {
        let pos: usize = 0;
        const prefix = "@container ";
        if (prefix.len > buf.len) { return err.BufferTooSmall; }
        @memcpy(buf[0..prefix.len], prefix);
        pos = prefix.len;

        if (cname := _.name) {
            const nstr = cname.toCss();
            if (pos + nstr.len + 1 > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + nstr.len], nstr);
            pos += nstr.len;
            buf[pos] = ' ';
            pos += 1;
        }
        if (_.negated) {
            const nkw = "not ";
            if (pos + nkw.len > buf.len) { return err.BufferTooSmall; }
            @memcpy(buf[pos .. pos + nkw.len], nkw);
            pos += nkw.len;
        }
        let i: usize = 0;
        while (i < _.feature_count) {
            if (i > 0) {
                const sep = if (_.op == LogicOp.and_op) " and " else " or ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            const written = _.features[i].toCss(buf[pos..])?;
            pos += written;
            i += 1;
        }
        return pos;
    }
}

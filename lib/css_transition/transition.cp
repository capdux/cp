// Source: Absorbed from /data/dev/bun/src/css/properties/transition.cp

/// CSS Transitions 1/2 解析与序列化错误集
pub err CssTransitionError {
    InvalidTime,
    InvalidDuration,
    InvalidEasing,
    InvalidProperty,
    InvalidBehavior,
    InvalidTransition,
    TooManyTransitions,
    BufferTooSmall,
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= 0.000001;
}

fn isValidIdent(s: string): bool {
    if (s.len == 0) { return false; }
    const c0 = s[0];
    if (!((c0 >= 'a' && c0 <= 'z') || (c0 >= 'A' && c0 <= 'Z') || c0 == '_' || c0 == '-')) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        const c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// CSS `transition-behavior`（CSS Transitions Level 2：`normal` / `allow-discrete`）
pub enum TransitionBehavior: u8 {
    normal = 0,
    allow_discrete = 1,

    static fn default(): TransitionBehavior { return .normal; }

    static fn parse(input: string): TransitionBehavior? {
        const s = trimAscii(input);
        if (std.ascii.eqlIgnoreCase(s, "normal")) { return .normal; }
        if (std.ascii.eqlIgnoreCase(s, "allow-discrete")) { return .allow_discrete; }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .normal => "normal",
            .allow_discrete => "allow-discrete",
        };
    }
}

/// `transition-property` 类型
pub enum TransitionPropertyKind: u8 {
    none = 0,
    all = 1,
    custom_ident = 2,
}

/// `transition-property` 模型
pub struct TransitionProperty {
    kind: TransitionPropertyKind = TransitionPropertyKind.all,
    name: string = "all",

    static fn default(): TransitionProperty {
        return TransitionProperty{ kind: TransitionPropertyKind.all, name: "all" };
    }

    fn isAll(): bool { return _.kind == TransitionPropertyKind.all; }
    fn isNone(): bool { return _.kind == TransitionPropertyKind.none; }

    static fn parse(input: string): TransitionProperty ! CssTransitionError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidProperty; }
        if (std.ascii.eqlIgnoreCase(s, "none")) {
            return TransitionProperty{ kind: TransitionPropertyKind.none, name: "none" };
        }
        if (std.ascii.eqlIgnoreCase(s, "all")) {
            return TransitionProperty{ kind: TransitionPropertyKind.all, name: "all" };
        }
        if (!isValidIdent(s)) {
            return err.InvalidProperty;
        }
        return TransitionProperty{ kind: TransitionPropertyKind.custom_ident, name: s };
    }

    fn toCss(): string {
        return switch (_.kind) {
            .none => "none",
            .all => "all",
            .custom_ident => _.name,
        };
    }
}

/// CSS `<time>` 统一毫秒表示（支持 `s` / `ms` 解析与最短规范化序列化）
pub struct TransitionTime {
    ms: f64 = 0.0,

    static fn zero(): TransitionTime { return TransitionTime{ ms: 0.0 }; }
    static fn fromMs(v: f64): TransitionTime { return TransitionTime{ ms: v }; }
    static fn fromSeconds(v: f64): TransitionTime { return TransitionTime{ ms: v * 1000.0 }; }

    fn isZero(): bool { return approxEq(_.ms, 0.0); }
    fn seconds(): f64 { return _.ms / 1000.0; }

    static fn parse(input: string, allow_negative: bool): TransitionTime ! CssTransitionError {
        const s = trimAscii(input);
        if (s.len < 2) { return err.InvalidTime; }
        let ms_val: f64 = 0.0;
        if (s.len > 2 && std.ascii.eqlIgnoreCase(s[s.len - 2 ..], "ms")) {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 2]) catch return err.InvalidTime;
            if (std.math.isNan(v) || std.math.isInf(v)) { return err.InvalidTime; }
            ms_val = v;
        } else if (s[s.len - 1] == 's' || s[s.len - 1] == 'S') {
            const v = std.fmt.parseFloat(f64, s[0 .. s.len - 1]) catch return err.InvalidTime;
            if (std.math.isNan(v) || std.math.isInf(v)) { return err.InvalidTime; }
            ms_val = v * 1000.0;
        } else {
            return err.InvalidTime;
        }
        if (!allow_negative && ms_val < 0.0) {
            return err.InvalidDuration;
        }
        return TransitionTime{ ms: ms_val };
    }

    /// 规范化最短输出（`0` -> `0s`，`|ms| < 1000` -> `{d}ms`，否则 `{d}s`）
    fn toCss(buf: []u8): usize ! CssTransitionError {
        if (_.isZero()) {
            const s = std.fmt.bufPrint(buf, "0s", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        const abs_ms = if (_.ms < 0.0) -_.ms else _.ms;
        if (abs_ms < 1000.0) {
            const s = std.fmt.bufPrint(buf, "{d}ms", .{_.ms}) catch return err.BufferTooSmall;
            return s.len;
        }
        const s = std.fmt.bufPrint(buf, "{d}s", .{_.ms / 1000.0}) catch return err.BufferTooSmall;
        return s.len;
    }
}

/// 过渡缓动函数种类
pub enum TransitionEasingKind: u8 {
    ease = 0,
    linear = 1,
    ease_in = 2,
    ease_out = 3,
    ease_in_out = 4,
    step_start = 5,
    step_end = 6,
    cubic_bezier = 7,
    steps = 8,
}

/// 过渡缓动函数模型（关键字、`cubic-bezier(x1,y1,x2,y2)` 与 `steps(n, pos)`）
pub struct TransitionEasing {
    kind: TransitionEasingKind = TransitionEasingKind.ease,
    x1: f64 = 0.25,
    y1: f64 = 0.1,
    x2: f64 = 0.25,
    y2: f64 = 1.0,
    step_count: u32 = 1,
    step_pos: string = "end",

    static fn default(): TransitionEasing { return TransitionEasing{}; }

    fn isEase(): bool {
        if (_.kind == TransitionEasingKind.ease) { return true; }
        if (_.kind == TransitionEasingKind.cubic_bezier) {
            return approxEq(_.x1, 0.25) && approxEq(_.y1, 0.1) && approxEq(_.x2, 0.25) && approxEq(_.y2, 1.0);
        }
        return false;
    }

    static fn parse(input: string): TransitionEasing ! CssTransitionError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidEasing; }
        if (std.ascii.eqlIgnoreCase(s, "ease")) { return TransitionEasing{ kind: TransitionEasingKind.ease }; }
        if (std.ascii.eqlIgnoreCase(s, "linear")) { return TransitionEasing{ kind: TransitionEasingKind.linear }; }
        if (std.ascii.eqlIgnoreCase(s, "ease-in")) { return TransitionEasing{ kind: TransitionEasingKind.ease_in }; }
        if (std.ascii.eqlIgnoreCase(s, "ease-out")) { return TransitionEasing{ kind: TransitionEasingKind.ease_out }; }
        if (std.ascii.eqlIgnoreCase(s, "ease-in-out")) { return TransitionEasing{ kind: TransitionEasingKind.ease_in_out }; }
        if (std.ascii.eqlIgnoreCase(s, "step-start")) { return TransitionEasing{ kind: TransitionEasingKind.step_start, step_count: 1, step_pos: "start" }; }
        if (std.ascii.eqlIgnoreCase(s, "step-end")) { return TransitionEasing{ kind: TransitionEasingKind.step_end, step_count: 1, step_pos: "end" }; }

        const lparen = std.mem.indexOfScalar(u8, s, '(') ?? return err.InvalidEasing;
        if (s[s.len - 1] != ')') { return err.InvalidEasing; }
        const fname = trimAscii(s[0..lparen]);
        const args = trimAscii(s[lparen + 1 .. s.len - 1]);

        if (std.ascii.eqlIgnoreCase(fname, "cubic-bezier")) {
            let vals: [4]f64 = [0.0, 0.0, 0.0, 0.0];
            let count: usize = 0;
            let start: usize = 0;
            let pos: usize = 0;
            while (pos <= args.len) {
                if (pos == args.len || args[pos] == ',') {
                    if (count >= 4) { return err.InvalidEasing; }
                    const tok = trimAscii(args[start..pos]);
                    const v = std.fmt.parseFloat(f64, tok) catch return err.InvalidEasing;
                    if (std.math.isNan(v) || std.math.isInf(v)) { return err.InvalidEasing; }
                    vals[count] = v;
                    count += 1;
                    start = pos + 1;
                }
                pos += 1;
            }
            if (count != 4) { return err.InvalidEasing; }
            if (vals[0] < 0.0 || vals[0] > 1.0 || vals[2] < 0.0 || vals[2] > 1.0) {
                return err.InvalidEasing;
            }
            return TransitionEasing{
                kind: TransitionEasingKind.cubic_bezier,
                x1: vals[0],
                y1: vals[1],
                x2: vals[2],
                y2: vals[3],
            };
        }

        if (std.ascii.eqlIgnoreCase(fname, "steps")) {
            if (args.len == 0) { return err.InvalidEasing; }
            let cnt_str = args;
            let pos_norm: string = "end";
            if (comma := std.mem.indexOfScalar(u8, args, ',')) {
                cnt_str = trimAscii(args[0..comma]);
                const ptok = trimAscii(args[comma + 1 ..]);
                if (std.ascii.eqlIgnoreCase(ptok, "start") || std.ascii.eqlIgnoreCase(ptok, "jump-start")) {
                    pos_norm = "start";
                } else if (std.ascii.eqlIgnoreCase(ptok, "end") || std.ascii.eqlIgnoreCase(ptok, "jump-end")) {
                    pos_norm = "end";
                } else if (std.ascii.eqlIgnoreCase(ptok, "jump-none")) {
                    pos_norm = "jump-none";
                } else if (std.ascii.eqlIgnoreCase(ptok, "jump-both")) {
                    pos_norm = "jump-both";
                } else {
                    return err.InvalidEasing;
                }
            }
            const cnt = std.fmt.parseInt(u32, cnt_str, 10) catch return err.InvalidEasing;
            if (cnt == 0 || (std.mem.eql(u8, pos_norm, "jump-none") && cnt < 2)) {
                return err.InvalidEasing;
            }
            return TransitionEasing{
                kind: TransitionEasingKind.steps,
                step_count: cnt,
                step_pos: pos_norm,
            };
        }

        return err.InvalidEasing;
    }

    fn toCss(buf: []u8): usize ! CssTransitionError {
        if (_.isEase()) {
            const s = std.fmt.bufPrint(buf, "ease", .{}) catch return err.BufferTooSmall;
            return s.len;
        }
        const res = switch (_.kind) {
            .ease => std.fmt.bufPrint(buf, "ease", .{}),
            .linear => std.fmt.bufPrint(buf, "linear", .{}),
            .ease_in => std.fmt.bufPrint(buf, "ease-in", .{}),
            .ease_out => std.fmt.bufPrint(buf, "ease-out", .{}),
            .ease_in_out => std.fmt.bufPrint(buf, "ease-in-out", .{}),
            .step_start => std.fmt.bufPrint(buf, "step-start", .{}),
            .step_end => std.fmt.bufPrint(buf, "step-end", .{}),
            .cubic_bezier => std.fmt.bufPrint(buf, "cubic-bezier({d},{d},{d},{d})", .{ _.x1, _.y1, _.x2, _.y2 }),
            .steps => if (_.step_count == 1 && std.mem.eql(u8, _.step_pos, "start"))
                std.fmt.bufPrint(buf, "step-start", .{})
            else if (_.step_count == 1 && std.mem.eql(u8, _.step_pos, "end"))
                std.fmt.bufPrint(buf, "step-end", .{})
            else
                std.fmt.bufPrint(buf, "steps({d},{s})", .{ _.step_count, _.step_pos }),
        } catch return err.BufferTooSmall;
        return res.len;
    }
}

fn tokenizeTransition(input: string, out: *[8]string): usize ! CssTransitionError {
    const s = trimAscii(input);
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) { pos += 1; }
        if (pos >= s.len) { break; }
        const start = pos;
        let depth: usize = 0;
        while (pos < s.len) {
            const c = s[pos];
            if (c == '(') { depth += 1; }
            else if (c == ')') {
                if (depth == 0) { return err.InvalidTransition; }
                depth -= 1;
            } else if (depth == 0 && (c == ' ' || c == '\t')) {
                break;
            }
            pos += 1;
        }
        if (depth != 0) { return err.InvalidTransition; }
        if (count >= 8) { return err.InvalidTransition; }
        out.*[count] = s[start..pos];
        count += 1;
    }
    return count;
}

/// 单个 CSS `transition` 简写项（任意顺序解析，首 `<time>` 为 duration，次 `<time>` 为 delay）
pub struct Transition {
    property: TransitionProperty = TransitionProperty.default(),
    duration: TransitionTime = TransitionTime.zero(),
    timing_function: TransitionEasing = TransitionEasing.default(),
    delay: TransitionTime = TransitionTime.zero(),
    behavior: TransitionBehavior = TransitionBehavior.normal,

    static fn default(): Transition { return Transition{}; }

    static fn parse(input: string): Transition ! CssTransitionError {
        let toks: [8]string = ["", "", "", "", "", "", "", ""];
        const n = tokenizeTransition(input, &toks)?;
        if (n == 0) { return err.InvalidTransition; }

        let tr = Transition{};
        let has_prop = false;
        let has_dur = false;
        let has_delay = false;
        let has_easing = false;
        let has_beh = false;

        let i: usize = 0;
        while (i < n) {
            const tok = toks[i];

            // 1. 先检查是否为时间值（首 `<time>` 为 duration 且必须 >= 0，次 `<time>` 为 delay 允许负值）
            const looks_like_time = tok.len >= 2 && (tok[0] == '-' || tok[0] == '+' || tok[0] == '.' || (tok[0] >= '0' && tok[0] <= '9'));
            if (looks_like_time) {
                if (!has_dur) {
                    tr.duration = TransitionTime.parse(tok, false)?;
                    has_dur = true;
                    i += 1;
                    continue;
                }
                if (!has_delay) {
                    tr.delay = TransitionTime.parse(tok, true)?;
                    has_delay = true;
                    i += 1;
                    continue;
                }
                return err.InvalidTransition;
            }

            // 2. 检查 transition-behavior (`normal` / `allow-discrete`)
            if (!has_beh) {
                if (beh := TransitionBehavior.parse(tok)) {
                    tr.behavior = beh;
                    has_beh = true;
                    i += 1;
                    continue;
                }
            }

            // 3. 检查函数或关键字缓动函数
            if (std.mem.indexOfScalar(u8, tok, '(') != null) {
                if (has_easing) { return err.InvalidTransition; }
                tr.timing_function = TransitionEasing.parse(tok)?;
                has_easing = true;
                i += 1;
                continue;
            }
            if (!has_easing) {
                if (ef := TransitionEasing.parse(tok) catch null) {
                    tr.timing_function = ef;
                    has_easing = true;
                    i += 1;
                    continue;
                }
            }

            // 4. 其余合法标识符归为 transition-property
            if (!has_prop) {
                tr.property = TransitionProperty.parse(tok)?;
                has_prop = true;
                i += 1;
                continue;
            }

            return err.InvalidTransition;
        }

        return tr;
    }

    /// 规范化序列化：若 delay 非 0 则必须同时输出 duration；省略默认 `ease` 与 `normal` behavior
    fn toCss(buf: []u8): usize ! CssTransitionError {
        let pos: usize = 0;
        const pstr = _.property.toCss();
        if (pstr.len > buf.len) { return err.BufferTooSmall; }
        @memcpy(buf[0..pstr.len], pstr);
        pos = pstr.len;

        if (!_.duration.isZero() || !_.delay.isZero()) {
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
            pos += _.duration.toCss(buf[pos..])?;
        }

        if (!_.timing_function.isEase()) {
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
            pos += _.timing_function.toCss(buf[pos..])?;
        }

        if (!_.delay.isZero()) {
            if (pos >= buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
            pos += _.delay.toCss(buf[pos..])?;
        }

        if (_.behavior != TransitionBehavior.normal) {
            const bstr = _.behavior.toCss();
            if (pos + 1 + bstr.len > buf.len) { return err.BufferTooSmall; }
            buf[pos] = ' ';
            pos += 1;
            @memcpy(buf[pos .. pos + bstr.len], bstr);
            pos += bstr.len;
        }
        return pos;
    }
}

/// 逗号分隔的多过渡列表（最多 8 项，忽略函数括号内逗号，零堆分配）
pub struct TransitionList {
    items: [8]Transition = [
        Transition{},
        Transition{},
        Transition{},
        Transition{},
        Transition{},
        Transition{},
        Transition{},
        Transition{},
    ],
    len: usize = 0,

    static fn parse(input: string): TransitionList ! CssTransitionError {
        const s = trimAscii(input);
        if (s.len == 0) { return err.InvalidTransition; }

        let list = TransitionList{};
        let start: usize = 0;
        let depth: usize = 0;
        let pos: usize = 0;
        while (pos <= s.len) {
            const at_end = pos == s.len;
            if (!at_end) {
                if (s[pos] == '(') { depth += 1; }
                else if (s[pos] == ')') {
                    if (depth == 0) { return err.InvalidTransition; }
                    depth -= 1;
                }
            }
            if (at_end || (depth == 0 && s[pos] == ',')) {
                const part = trimAscii(s[start..pos]);
                if (part.len == 0) { return err.InvalidTransition; }
                if (list.len >= 8) { return err.TooManyTransitions; }
                list.items[list.len] = Transition.parse(part)?;
                list.len += 1;
                start = pos + 1;
            }
            pos += 1;
        }
        if (depth != 0) { return err.InvalidTransition; }
        return list;
    }

    fn toCss(buf: []u8): usize ! CssTransitionError {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            if (i > 0) {
                const sep = ", ";
                if (pos + sep.len > buf.len) { return err.BufferTooSmall; }
                @memcpy(buf[pos .. pos + sep.len], sep);
                pos += sep.len;
            }
            pos += _.items[i].toCss(buf[pos..])?;
            i += 1;
        }
        return pos;
    }
}

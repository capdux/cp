// Source: Absorbed from /data/dev/bun/src/css/values/easing.cp | Synced: 2026-11-21T16:57:43Z

/// CSS Easing Functions Level 1 解析与序列化错误集
pub err CssEasingError {
    InvalidEasing,
    InvalidBezierControlPoint,
    InvalidStepCount,
    InvalidStepPosition,
    BufferTooSmall,
}

/// CSS `steps()` 阶梯位置枚举（CSS Easing Functions Level 1 §2.3）
pub enum StepPosition: u8 {
    start = 0,
    end = 1,
    jump_none = 2,
    jump_both = 3,

    static fn default(): StepPosition {
        return .end;
    }

    static fn parse(input: string): StepPosition? {
        if (std.ascii.eqlIgnoreCase(input, "start") || std.ascii.eqlIgnoreCase(input, "jump-start")) {
            return .start;
        }
        if (std.ascii.eqlIgnoreCase(input, "end") || std.ascii.eqlIgnoreCase(input, "jump-end")) {
            return .end;
        }
        if (std.ascii.eqlIgnoreCase(input, "jump-none")) {
            return .jump_none;
        }
        if (std.ascii.eqlIgnoreCase(input, "jump-both")) {
            return .jump_both;
        }
        return null;
    }

    fn toCss(): string {
        return switch (_) {
            .start => "start",
            .end => "end",
            .jump_none => "jump-none",
            .jump_both => "jump-both",
        };
    }
}

/// 缓动函数种类枚举
pub enum EasingKind: u8 {
    linear = 0,
    ease = 1,
    ease_in = 2,
    ease_out = 3,
    ease_in_out = 4,
    cubic_bezier = 5,
    steps = 6,
}

fn approxEq(a: f64, b: f64): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= 0.000001;
}

/// 三次贝塞尔曲线控制点（`cubic-bezier(x1, y1, x2, y2)`，要求 `x1, x2 ∈ [0, 1]`）
pub struct CubicBezier {
    x1: f64 = 0.0,
    y1: f64 = 0.0,
    x2: f64 = 1.0,
    y2: f64 = 1.0,

    fn eql(other: CubicBezier): bool {
        return approxEq(_.x1, other.x1)
            && approxEq(_.y1, other.y1)
            && approxEq(_.x2, other.x2)
            && approxEq(_.y2, other.y2);
    }

    /// 按 W3C CSS Easing 规范对输入进度 `t` 进行贝塞尔曲线求值（Newton-Raphson + 二分法兜底）。
    fn sample(t: f64): f64 {
        if (t <= 0.0) {
            return 0.0;
        }
        if (t >= 1.0) {
            return 1.0;
        }
        if (approxEq(_.x1, _.y1) && approxEq(_.x2, _.y2)) {
            return t;
        }

        const cx = 3.0 * _.x1;
        const bx = 3.0 * (_.x2 - _.x1) - cx;
        const ax = 1.0 - cx - bx;

        const cy = 3.0 * _.y1;
        const by = 3.0 * (_.y2 - _.y1) - cy;
        const ay = 1.0 - cy - by;

        // Newton-Raphson 求解 B_x(u) = t
        let u: f64 = t;
        let iter: usize = 0;
        while (iter < 8) {
            const x_est = ((ax * u + bx) * u + cx) * u - t;
            const abs_err = if (x_est < 0.0) -x_est else x_est;
            if (abs_err < 0.000000001) {
                return ((ay * u + by) * u + cy) * u;
            }
            const dx = (3.0 * ax * u + 2.0 * bx) * u + cx;
            const abs_dx = if (dx < 0.0) -dx else dx;
            if (abs_dx < 0.000000001) {
                break;
            }
            u -= x_est / dx;
            if (u < 0.0 || u > 1.0) {
                break;
            }
            iter += 1;
        }

        // 二分法保底收敛
        let lo: f64 = 0.0;
        let hi: f64 = 1.0;
        u = t;
        let bi: usize = 0;
        while (bi < 40) {
            const x_val = ((ax * u + bx) * u + cx) * u;
            const diff = x_val - t;
            const abs_diff = if (diff < 0.0) -diff else diff;
            if (abs_diff < 0.000000001) {
                break;
            }
            if (t > x_val) {
                lo = u;
            } else {
                hi = u;
            }
            u = 0.5 * (lo + hi);
            bi += 1;
        }
        return ((ay * u + by) * u + cy) * u;
    }
}

/// 阶梯缓动函数参数（`steps(count, position)`）
pub struct EasingSteps {
    count: u32 = 1,
    position: StepPosition = StepPosition.end,

    /// 按 W3C CSS Easing Functions Level 1 §2.3.1 规范计算阶梯输出进度。
    fn sample(t: f64): f64 {
        const n_f: f64 = _.count as f64;
        if (t < 0.0) {
            return 0.0;
        }
        let step: i64 = @intFromFloat(@floor(t * n_f));
        if (_.position == StepPosition.start || _.position == StepPosition.jump_both) {
            step += 1;
        }
        const jumps: i64 = switch (_.position) {
            .start, .end => _.count as i64,
            .jump_none => (_.count - 1) as i64,
            .jump_both => (_.count + 1) as i64,
        };
        if (step < 0) {
            step = 0;
        }
        if (step > jumps) {
            step = jumps;
        }
        return (step as f64) / (jumps as f64);
    }
}

fn trimAscii(s: string): string {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn writeStr(buf: []u8, pos: usize, s: string): usize ! CssEasingError {
    if (pos + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos .. pos + s.len], s);
    return pos + s.len;
}

/// CSS 缓动函数统一模型（支持关键字、三次贝塞尔与阶梯函数的解析、归一化、序列化与数值采样）
pub struct EasingFunction {
    kind: EasingKind,
    bezier: CubicBezier = CubicBezier{},
    steps: EasingSteps = EasingSteps{},

    static fn linear(): EasingFunction {
        return EasingFunction{ kind: EasingKind.linear };
    }

    static fn ease(): EasingFunction {
        return EasingFunction{
            kind: EasingKind.ease,
            bezier: CubicBezier{ x1: 0.25, y1: 0.1, x2: 0.25, y2: 1.0 },
        };
    }

    static fn easeIn(): EasingFunction {
        return EasingFunction{
            kind: EasingKind.ease_in,
            bezier: CubicBezier{ x1: 0.42, y1: 0.0, x2: 1.0, y2: 1.0 },
        };
    }

    static fn easeOut(): EasingFunction {
        return EasingFunction{
            kind: EasingKind.ease_out,
            bezier: CubicBezier{ x1: 0.0, y1: 0.0, x2: 0.58, y2: 1.0 },
        };
    }

    static fn easeInOut(): EasingFunction {
        return EasingFunction{
            kind: EasingKind.ease_in_out,
            bezier: CubicBezier{ x1: 0.42, y1: 0.0, x2: 0.58, y2: 1.0 },
        };
    }

    static fn cubicBezier(x1: f64, y1: f64, x2: f64, y2: f64): EasingFunction ! CssEasingError {
        if (std.math.isNan(x1) || std.math.isNan(y1) || std.math.isNan(x2) || std.math.isNan(y2)
            || std.math.isInf(x1) || std.math.isInf(y1) || std.math.isInf(x2) || std.math.isInf(y2))
        {
            return err.InvalidBezierControlPoint;
        }
        if (x1 < 0.0 || x1 > 1.0 || x2 < 0.0 || x2 > 1.0) {
            return err.InvalidBezierControlPoint;
        }
        return EasingFunction{
            kind: EasingKind.cubic_bezier,
            bezier: CubicBezier{ x1: x1, y1: y1, x2: x2, y2: y2 },
        };
    }

    static fn makeSteps(count: u32, position: StepPosition): EasingFunction ! CssEasingError {
        if (count == 0) {
            return err.InvalidStepCount;
        }
        if (position == StepPosition.jump_none && count < 2) {
            return err.InvalidStepCount;
        }
        return EasingFunction{
            kind: EasingKind.steps,
            steps: EasingSteps{ count: count, position: position },
        };
    }

    /// 判断给定字符串是否为标准缓动关键字
    static fn isIdent(s: string): bool {
        return std.ascii.eqlIgnoreCase(s, "linear")
            || std.ascii.eqlIgnoreCase(s, "ease")
            || std.ascii.eqlIgnoreCase(s, "ease-in")
            || std.ascii.eqlIgnoreCase(s, "ease-out")
            || std.ascii.eqlIgnoreCase(s, "ease-in-out")
            || std.ascii.eqlIgnoreCase(s, "step-start")
            || std.ascii.eqlIgnoreCase(s, "step-end");
    }

    /// 解析 CSS 缓动函数表达式（关键字、`cubic-bezier(...)`、`steps(...)`）
    static fn parse(input: string): EasingFunction ! CssEasingError {
        const s = trimAscii(input);
        if (s.len == 0) {
            return err.InvalidEasing;
        }

        // 关键字匹配
        if (std.ascii.eqlIgnoreCase(s, "linear")) { return EasingFunction.linear(); }
        if (std.ascii.eqlIgnoreCase(s, "ease")) { return EasingFunction.ease(); }
        if (std.ascii.eqlIgnoreCase(s, "ease-in")) { return EasingFunction.easeIn(); }
        if (std.ascii.eqlIgnoreCase(s, "ease-out")) { return EasingFunction.easeOut(); }
        if (std.ascii.eqlIgnoreCase(s, "ease-in-out")) { return EasingFunction.easeInOut(); }
        if (std.ascii.eqlIgnoreCase(s, "step-start")) { return EasingFunction.makeSteps(1, StepPosition.start); }
        if (std.ascii.eqlIgnoreCase(s, "step-end")) { return EasingFunction.makeSteps(1, StepPosition.end); }

        // 函数形式：name(args)
        const lparen = std.mem.indexOfScalar(u8, s, '(') ?? return err.InvalidEasing;
        if (s[s.len - 1] != ')') {
            return err.InvalidEasing;
        }
        const fname = trimAscii(s[0..lparen]);
        const args_str = trimAscii(s[lparen + 1 .. s.len - 1]);

        if (std.ascii.eqlIgnoreCase(fname, "cubic-bezier")) {
            let parts: [4]f64 = [0.0, 0.0, 0.0, 0.0];
            let count: usize = 0;
            let start: usize = 0;
            let pos: usize = 0;
            while (pos <= args_str.len) {
                if (pos == args_str.len || args_str[pos] == ',') {
                    if (count >= 4) {
                        return err.InvalidEasing;
                    }
                    const tok = trimAscii(args_str[start..pos]);
                    if (tok.len == 0) {
                        return err.InvalidEasing;
                    }
                    parts[count] = std.fmt.parseFloat(f64, tok) catch return err.InvalidBezierControlPoint;
                    count += 1;
                    start = pos + 1;
                }
                pos += 1;
            }
            if (count != 4) {
                return err.InvalidEasing;
            }
            return EasingFunction.cubicBezier(parts[0], parts[1], parts[2], parts[3]);
        }

        if (std.ascii.eqlIgnoreCase(fname, "steps")) {
            if (args_str.len == 0) {
                return err.InvalidEasing;
            }
            if (comma := std.mem.indexOfScalar(u8, args_str, ',')) {
                const rest = args_str[comma + 1 ..];
                if (std.mem.indexOfScalar(u8, rest, ',') != null) {
                    return err.InvalidEasing;
                }
                const cnt_tok = trimAscii(args_str[0..comma]);
                const pos_tok = trimAscii(rest);
                const cnt = std.fmt.parseInt(u32, cnt_tok, 10) catch return err.InvalidStepCount;
                const step_pos = StepPosition.parse(pos_tok) ?? return err.InvalidStepPosition;
                return EasingFunction.makeSteps(cnt, step_pos);
            } else {
                const cnt = std.fmt.parseInt(u32, args_str, 10) catch return err.InvalidStepCount;
                return EasingFunction.makeSteps(cnt, StepPosition.default());
            }
        }

        return err.InvalidEasing;
    }

    /// 将等价的 `cubic-bezier(...)` 归一化为对应标准关键字（`linear`/`ease`/`ease-in`/`ease-out`/`ease-in-out`）。
    fn normalize(): EasingFunction {
        if (_.kind != EasingKind.cubic_bezier) {
            return _;
        }
        if (_.bezier.eql(CubicBezier{ x1: 0.0, y1: 0.0, x2: 1.0, y2: 1.0 })) {
            return EasingFunction.linear();
        }
        if (_.bezier.eql(CubicBezier{ x1: 0.25, y1: 0.1, x2: 0.25, y2: 1.0 })) {
            return EasingFunction.ease();
        }
        if (_.bezier.eql(CubicBezier{ x1: 0.42, y1: 0.0, x2: 1.0, y2: 1.0 })) {
            return EasingFunction.easeIn();
        }
        if (_.bezier.eql(CubicBezier{ x1: 0.0, y1: 0.0, x2: 0.58, y2: 1.0 })) {
            return EasingFunction.easeOut();
        }
        if (_.bezier.eql(CubicBezier{ x1: 0.42, y1: 0.0, x2: 0.58, y2: 1.0 })) {
            return EasingFunction.easeInOut();
        }
        return _;
    }

    /// 判断是否等价于标准 `ease` 关键字
    fn isEase(): bool {
        return _.normalize().kind == EasingKind.ease;
    }

    /// 对输入进度 `t` 求值（`t ∈ [0, 1]`）
    fn sample(t: f64): f64 {
        return switch (_.kind) {
            .linear => if (t <= 0.0) 0.0 else if (t >= 1.0) 1.0 else t,
            .ease, .ease_in, .ease_out, .ease_in_out, .cubic_bezier => _.bezier.sample(t),
            .steps => _.steps.sample(t),
        };
    }

    /// 规范化序列化为 CSS 文本（自动将等价贝塞尔曲线化简为关键字、`steps(1, start|end)` 化简为 `step-start|step-end`）。
    fn toCss(buf: []u8): usize ! CssEasingError {
        const norm = _.normalize();
        switch (norm.kind) {
            .linear => return writeStr(buf, 0, "linear"),
            .ease => return writeStr(buf, 0, "ease"),
            .ease_in => return writeStr(buf, 0, "ease-in"),
            .ease_out => return writeStr(buf, 0, "ease-out"),
            .ease_in_out => return writeStr(buf, 0, "ease-in-out"),
            .cubic_bezier => {
                const formatted = std.fmt.bufPrint(
                    buf,
                    "cubic-bezier({d},{d},{d},{d})",
                    .{ norm.bezier.x1, norm.bezier.y1, norm.bezier.x2, norm.bezier.y2 },
                ) catch return err.BufferTooSmall;
                return formatted.len;
            },
            .steps => {
                if (norm.steps.count == 1 && norm.steps.position == StepPosition.start) {
                    return writeStr(buf, 0, "step-start");
                }
                if (norm.steps.count == 1 && norm.steps.position == StepPosition.end) {
                    return writeStr(buf, 0, "step-end");
                }
                const formatted = std.fmt.bufPrint(
                    buf,
                    "steps({d},{s})",
                    .{ norm.steps.count, norm.steps.position.toCss() },
                ) catch return err.BufferTooSmall;
                return formatted.len;
            },
        }
    }
}

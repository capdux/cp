// Source: Absorbed from /data/dev/bun/src/css/values/ratio.cp, values/rect.cp, properties/overflow.cp

fn gcdU64(a_in: u64, b_in: u64): u64 {
    let a = a_in;
    let b = b_in;
    while (b != 0) {
        const t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/// CSS `<ratio>` 宽高比结构体（分子 / 分母）
pub struct Ratio {
    numerator: f64,
    denominator: f64 = 1.0,

    /// 构造指定分子与分母的宽高比
    static fn new(numerator: f64, denominator: f64): Ratio {
        return Ratio{
            numerator: numerator,
            denominator: denominator,
        };
    }

    /// 构造单值宽高比（分母默认为 1.0）
    static fn fromSingle(numerator: f64): Ratio {
        return Ratio{
            numerator: numerator,
            denominator: 1.0,
        };
    }

    /// 判断是否为退化比例（CSS Values 4：任一分量为非正数、零、NaN 或 Inf）
    fn isDegenerate(): bool {
        if (!std.math.isFinite(_.numerator) || !std.math.isFinite(_.denominator)) {
            return true;
        }
        return _.numerator <= 0.0 || _.denominator <= 0.0;
    }

    /// 计算浮点比值；零分母或非有限数值返回 null
    fn asF64(): f64? {
        if (!std.math.isFinite(_.numerator) || !std.math.isFinite(_.denominator)) {
            return null;
        }
        if (_.denominator == 0.0) {
            return null;
        }
        return _.numerator / _.denominator;
    }

    /// 化简为最简比例；退化比例原样返回以防除零或无效运算
    fn simplify(): Ratio {
        if (_.isDegenerate()) {
            return _;
        }
        const max_exact: f64 = 9007199254740992.0; // 2^53
        let scale: f64 = 1.0;
        let step: u32 = 0;
        while (step <= 6) {
            const sn = _.numerator * scale;
            const sd = _.denominator * scale;
            if (sn <= max_exact && sd <= max_exact) {
                const rn = @round(sn);
                const rd = @round(sd);
                if (@abs(sn - rn) <= 0.000000001 && @abs(sd - rd) <= 0.000000001 && rn >= 1.0 && rd >= 1.0) {
                    const ni = rn as u64;
                    const di = rd as u64;
                    const g = gcdU64(ni, di);
                    if (g > 0) {
                        return Ratio{
                            numerator: (ni / g) as f64,
                            denominator: (di / g) as f64,
                        };
                    }
                }
            }
            scale *= 10.0;
            step += 1;
        }
        return Ratio{
            numerator: _.numerator / _.denominator,
            denominator: 1.0,
        };
    }

    /// 返回倒数比例（交换分子与分母）
    fn invert(): Ratio {
        return Ratio{
            numerator: _.denominator,
            denominator: _.numerator,
        };
    }

    /// 字段级精确相等判断
    fn eql(other: Ratio): bool {
        return _.numerator == other.numerator && _.denominator == other.denominator;
    }

    /// 数学比值等价判断（如 16/9 与 1920/1080 视为等价；退化比例返回 false）
    fn equivalent(other: Ratio): bool {
        if (_.isDegenerate() || other.isDegenerate()) {
            return false;
        }
        const lhs = _.numerator * other.denominator;
        const rhs = other.numerator * _.denominator;
        const diff = @abs(lhs - rhs);
        const tol = 0.000000001 * (@abs(lhs) + @abs(rhs) + 1.0);
        return diff <= tol;
    }
}

/// 泛型四边盒模型矩形结构体（对应 CSS margin / padding / border-width 等四边值）
pub fn Rect(comptime T: type): type {
    return struct {
        top: T,
        right: T,
        bottom: T,
        left: T,

        const SelfRect = @This();

        /// 四边统一赋值（CSS 1-value shorthand）
        static fn all(v: T): SelfRect {
            return SelfRect{
                top: v,
                right: v,
                bottom: v,
                left: v,
            };
        }

        /// 上下对称、左右对称构造（CSS 2-value shorthand: vertical, horizontal）
        static fn symmetric(vertical: T, horizontal: T): SelfRect {
            return SelfRect{
                top: vertical,
                right: horizontal,
                bottom: vertical,
                left: horizontal,
            };
        }

        /// CSS 3-value shorthand 构造（top, horizontal, bottom）
        static fn fromThree(top_v: T, horizontal: T, bottom_v: T): SelfRect {
            return SelfRect{
                top: top_v,
                right: horizontal,
                bottom: bottom_v,
                left: horizontal,
            };
        }

        /// 判断四边值是否全部相等
        fn isUniform(): bool {
            return _.top == _.right && _.right == _.bottom && _.bottom == _.left;
        }

        /// 计算水平方向总和（left + right）
        fn horizontalSum(): T {
            return _.left + _.right;
        }

        /// 计算垂直方向总和（top + bottom）
        fn verticalSum(): T {
            return _.top + _.bottom;
        }

        /// 判断两个四边矩形是否逐边相等
        fn eql(other: SelfRect): bool {
            return _.top == other.top && _.right == other.right && _.bottom == other.bottom && _.left == other.left;
        }
    };
}

/// CSS Overflow 关键词枚举（CSS Overflow Module Level 3）
pub enum OverflowKeyword: u8 {
    visible = 0,
    hidden = 1,
    clip = 2,
    scroll = 3,
    auto = 4,

    /// 从标准 CSS 关键词字符串解析；未知或非法输入返回 null
    static fn fromStr(s: string): OverflowKeyword? {
        if (std.mem.eql(u8, s, "visible")) { return .visible; }
        if (std.mem.eql(u8, s, "hidden")) { return .hidden; }
        if (std.mem.eql(u8, s, "clip")) { return .clip; }
        if (std.mem.eql(u8, s, "scroll")) { return .scroll; }
        if (std.mem.eql(u8, s, "auto")) { return .auto; }
        return null;
    }

    /// 返回对应的标准 CSS 关键词字符串
    fn asStr(): string {
        return switch (_) {
            .visible => "visible",
            .hidden => "hidden",
            .clip => "clip",
            .scroll => "scroll",
            .auto => "auto",
        };
    }

    /// 该模式是否裁剪溢出内容（仅 visible 不裁剪）
    fn clipsContent(): bool {
        return _ != .visible;
    }

    /// 是否允许用户或程序滚动（hidden / scroll / auto 支持滚动容器，clip 禁止滚动）
    fn allowsScroll(): bool {
        return _ == .hidden || _ == .scroll || _ == .auto;
    }
}

/// CSS `overflow` 双轴简写属性结构体
pub struct Overflow {
    x: OverflowKeyword = .visible,
    y: OverflowKeyword = .visible,

    /// 双轴统一设置
    static fn all(kw: OverflowKeyword): Overflow {
        return Overflow{ x: kw, y: kw };
    }

    /// 分别指定 X 轴与 Y 轴
    static fn new(x_kw: OverflowKeyword, y_kw: OverflowKeyword): Overflow {
        return Overflow{ x: x_kw, y: y_kw };
    }

    /// 解析 CSS `overflow` 单值或双值简写字符串（如 "hidden" 或 "hidden scroll"）
    static fn parseShorthand(s: string): Overflow? {
        if (s.len == 0) {
            return null;
        }
        // 查找首个空白分隔符
        let split_idx: usize? = null;
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == ' ' || s[i] == '\t') {
                split_idx = i;
                break;
            }
            i += 1;
        }

        if (pos := split_idx) {
            const first_str = s[0..pos];
            let next_start = pos;
            while (next_start < s.len && (s[next_start] == ' ' || s[next_start] == '\t')) {
                next_start += 1;
            }
            if (next_start >= s.len) {
                return null;
            }
            const second_str = s[next_start..];
            if (x_kw := OverflowKeyword.fromStr(first_str)) {
                if (y_kw := OverflowKeyword.fromStr(second_str)) {
                    return Overflow{ x: x_kw, y: y_kw };
                }
            }
            return null;
        }

        if (kw := OverflowKeyword.fromStr(s)) {
            return Overflow{ x: kw, y: kw };
        }
        return null;
    }

    /// 判断双轴溢出模式是否一致
    fn isUniform(): bool {
        return _.x == _.y;
    }

    /// 判断任一轴是否裁剪溢出内容
    fn clipsAny(): bool {
        return _.x.clipsContent() || _.y.clipsContent();
    }

    /// 双轴相等比较
    fn eql(other: Overflow): bool {
        return _.x == other.x && _.y == other.y;
    }
}

/// CSS `text-overflow` 关键词枚举
pub enum TextOverflow: u8 {
    clip = 0,
    ellipsis = 1,

    static fn fromStr(s: string): TextOverflow? {
        if (std.mem.eql(u8, s, "clip")) { return .clip; }
        if (std.mem.eql(u8, s, "ellipsis")) { return .ellipsis; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .clip => "clip",
            .ellipsis => "ellipsis",
        };
    }
}

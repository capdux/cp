// Source: Absorbed from /data/dev/bun/src/css/values/angle.cp, /data/dev/bun/src/css/values/resolution.cp

pub err Error {
    EmptyInput,
    InvalidNumber,
    MissingUnit,
    UnknownUnit,
    NegativeResolution,
    BufferTooSmall,
}

const PI: f64 = 3.14159265358979323846;
const DPI_PER_DPPX: f64 = 96.0;
const CM_PER_INCH: f64 = 2.54;

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimAscii(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) {
            return false;
        }
        i += 1;
    }
    return true;
}

struct ParsedDim {
    value: f64,
    unit: string,
}

fn splitDimension(raw: string): ParsedDim! {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return err.EmptyInput;
    }

    let i: usize = 0;
    if (s[i] == '+' || s[i] == '-') {
        i += 1;
    }

    let has_digits = false;
    while (i < s.len && isAsciiDigit(s[i])) {
        has_digits = true;
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isAsciiDigit(s[i])) {
            has_digits = true;
            i += 1;
        }
    }

    if (!has_digits) {
        return err.InvalidNumber;
    }

    // 可选科学计数法指数部分（必须后接数字，防止把单位 'e...' 误吞）
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        let exp_idx = i + 1;
        if (exp_idx < s.len && (s[exp_idx] == '+' || s[exp_idx] == '-')) {
            exp_idx += 1;
        }
        if (exp_idx < s.len && isAsciiDigit(s[exp_idx])) {
            i = exp_idx;
            while (i < s.len && isAsciiDigit(s[i])) {
                i += 1;
            }
        }
    }

    const num_str = s[0..i];
    const unit_str = s[i..];

    const val = std.fmt.parseFloat(f64, num_str) catch return err.InvalidNumber;
    if (!std.math.isFinite(val)) {
        return err.InvalidNumber;
    }

    return ParsedDim{
        value: val,
        unit: unit_str,
    };
}

fn writeU64Dec(v_in: u64, dst: []u8): usize {
    if (v_in == 0) {
        dst[0] = '0';
        return 1;
    }
    let rev: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) {
        rev[n] = ('0' as u8) + ((v % 10) as u8);
        v /= 10;
        n += 1;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = rev[n - 1 - i];
        i += 1;
    }
    return n;
}

fn formatCssDim(val_in: f64, unit: string, buf: []u8): string! {
    let val = val_in;
    if (!std.math.isFinite(val)) {
        return err.InvalidNumber;
    }
    if (val == 0.0) {
        val = 0.0;
    }

    let tmp: [64]u8 = [0; 64];
    let pos: usize = 0;

    if (val < 0.0) {
        tmp[pos] = '-';
        pos += 1;
        val = -val;
    }

    // 四舍五入到 6 位小数精度（微度级，消除浮点尾差）
    const scale: f64 = 1000000.0;
    const scaled = @round(val * scale);
    const total_scaled: u64 = scaled as u64;
    const int_part: u64 = total_scaled / 1000000;
    const frac_part: u64 = total_scaled % 1000000;

    pos += writeU64Dec(int_part, tmp[pos..64]);

    if (frac_part > 0) {
        tmp[pos] = '.';
        pos += 1;
        let frac_buf: [6]u8 = ['0'; 6];
        let f = frac_part;
        let idx: usize = 6;
        while (idx > 0) {
            idx -= 1;
            frac_buf[idx] = ('0' as u8) + ((f % 10) as u8);
            f /= 10;
        }
        let frac_end: usize = 6;
        while (frac_end > 0 && frac_buf[frac_end - 1] == '0') {
            frac_end -= 1;
        }
        @memcpy(tmp[pos .. pos + frac_end], frac_buf[0..frac_end]);
        pos += frac_end;
    }

    const total = pos + unit.len;
    if (buf.len < total) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[0..pos], tmp[0..pos]);
    if (unit.len > 0) {
        @memcpy(buf[pos..total], unit);
    }
    return buf[0..total];
}

/// CSS `<angle>` 单位枚举
pub enum AngleUnit: u8 {
    deg = 0,
    rad = 1,
    grad = 2,
    turn = 3,

    fn asStr(): string {
        return switch (_) {
            .deg => "deg",
            .rad => "rad",
            .grad => "grad",
            .turn => "turn",
        };
    }
}

/// CSS `<angle>` 角度值（支持 deg / rad / grad / turn 四单位解析、换算、运算与规范序列化）
pub struct Angle {
    value: f64,
    unit: AngleUnit,

    static fn zero(): Angle {
        return Angle{ value: 0.0, unit: .deg };
    }

    static fn deg(v: f64): Angle {
        return Angle{ value: v, unit: .deg };
    }

    static fn rad(v: f64): Angle {
        return Angle{ value: v, unit: .rad };
    }

    static fn grad(v: f64): Angle {
        return Angle{ value: v, unit: .grad };
    }

    static fn turn(v: f64): Angle {
        return Angle{ value: v, unit: .turn };
    }

    /// 严格解析带单位的 CSS `<angle>`（不接受无单位 0）
    static fn parse(s: string): Angle! {
        return Angle.parseInternal(s, false);
    }

    /// 解析 CSS `<angle>`，允许无单位 `0` 解析为 `0deg`
    static fn parseWithUnitlessZero(s: string): Angle! {
        return Angle.parseInternal(s, true);
    }

    static fn parseInternal(s: string, allow_unitless_zero: bool): Angle! {
        const dim = splitDimension(s)?;
        if (dim.unit.len == 0) {
            if (allow_unitless_zero && dim.value == 0.0) {
                return Angle.zero();
            }
            return err.MissingUnit;
        }
        if (eqlIgnoreCaseAscii(dim.unit, "deg")) {
            return Angle{ value: dim.value, unit: .deg };
        }
        if (eqlIgnoreCaseAscii(dim.unit, "rad")) {
            return Angle{ value: dim.value, unit: .rad };
        }
        if (eqlIgnoreCaseAscii(dim.unit, "grad")) {
            return Angle{ value: dim.value, unit: .grad };
        }
        if (eqlIgnoreCaseAscii(dim.unit, "turn")) {
            return Angle{ value: dim.value, unit: .turn };
        }
        return err.UnknownUnit;
    }

    fn isZero(): bool {
        return _.value == 0.0;
    }

    fn toDegrees(): f64 {
        return switch (_.unit) {
            .deg => _.value,
            .rad => _.value * (180.0 / PI),
            .grad => _.value * 0.9,
            .turn => _.value * 360.0,
        };
    }

    fn toRadians(): f64 {
        return switch (_.unit) {
            .deg => _.value * (PI / 180.0),
            .rad => _.value,
            .grad => _.value * (PI / 200.0),
            .turn => _.value * (2.0 * PI),
        };
    }

    fn toGradians(): f64 {
        return switch (_.unit) {
            .deg => _.value * (200.0 / 180.0),
            .rad => _.value * (200.0 / PI),
            .grad => _.value,
            .turn => _.value * 400.0,
        };
    }

    fn toTurns(): f64 {
        return switch (_.unit) {
            .deg => _.value / 360.0,
            .rad => _.value / (2.0 * PI),
            .grad => _.value / 400.0,
            .turn => _.value,
        };
    }

    /// 归一化角度到 [0.0, 360.0) 区间
    fn normalizeDegrees(): f64 {
        let d = @rem(_.toDegrees(), 360.0);
        if (d < 0.0) {
            d += 360.0;
        }
        if (d >= 360.0 || d == 0.0) {
            return 0.0;
        }
        return d;
    }

    /// 角度相加（同单位保留原单位，跨单位换算为 deg）
    fn add(other: Angle): Angle {
        if (_.unit == other.unit) {
            return Angle{ value: _.value + other.value, unit: _.unit };
        }
        return Angle.deg(_.toDegrees() + other.toDegrees());
    }

    /// 角度相减（同单位保留原单位，跨单位换算为 deg）
    fn sub(other: Angle): Angle {
        if (_.unit == other.unit) {
            return Angle{ value: _.value - other.value, unit: _.unit };
        }
        return Angle.deg(_.toDegrees() - other.toDegrees());
    }

    /// 标量缩放
    fn scale(factor: f64): Angle {
        return Angle{ value: _.value * factor, unit: _.unit };
    }

    /// 角度数值等价比较（如 180deg == 0.5turn == 200grad）
    fn eql(other: Angle): bool {
        const a = _.toDegrees();
        const b = other.toDegrees();
        const diff = @abs(a - b);
        const tol = 0.000001 * (@abs(a) + @abs(b) + 1.0);
        return diff <= tol;
    }

    /// 规范 CSS 序列化写入目标缓冲区
    fn toCss(buf: []u8): string! {
        return formatCssDim(_.value, _.unit.asStr(), buf);
    }

    /// 规范 CSS 序列化（零值省略单位输出 "0"）
    fn toCssWithUnitlessZero(buf: []u8): string! {
        if (_.isZero()) {
            return formatCssDim(0.0, "", buf);
        }
        return _.toCss(buf);
    }
}

/// CSS `<resolution>` 单位枚举
pub enum ResolutionUnit: u8 {
    dpi = 0,
    dpcm = 1,
    dppx = 2,

    fn asStr(): string {
        return switch (_) {
            .dpi => "dpi",
            .dpcm => "dpcm",
            .dppx => "dppx",
        };
    }
}

/// CSS `<resolution>` 分辨率值（支持 dpi / dpcm / dppx / x 解析、换算与规范序列化）
pub struct Resolution {
    value: f64,
    unit: ResolutionUnit,

    static fn dpi(v: f64): Resolution {
        return Resolution{ value: v, unit: .dpi };
    }

    static fn dpcm(v: f64): Resolution {
        return Resolution{ value: v, unit: .dpcm };
    }

    static fn dppx(v: f64): Resolution {
        return Resolution{ value: v, unit: .dppx };
    }

    /// 解析 CSS `<resolution>`（支持 dpi / dpcm / dppx / x 别名，拒绝负值）
    static fn parse(s: string): Resolution! {
        const dim = splitDimension(s)?;
        if (dim.value < 0.0) {
            return err.NegativeResolution;
        }
        if (dim.unit.len == 0) {
            return err.MissingUnit;
        }
        if (eqlIgnoreCaseAscii(dim.unit, "dpi")) {
            return Resolution{ value: dim.value, unit: .dpi };
        }
        if (eqlIgnoreCaseAscii(dim.unit, "dpcm")) {
            return Resolution{ value: dim.value, unit: .dpcm };
        }
        if (eqlIgnoreCaseAscii(dim.unit, "dppx") || eqlIgnoreCaseAscii(dim.unit, "x")) {
            return Resolution{ value: dim.value, unit: .dppx };
        }
        return err.UnknownUnit;
    }

    fn toDpi(): f64 {
        return switch (_.unit) {
            .dpi => _.value,
            .dpcm => _.value * CM_PER_INCH,
            .dppx => _.value * DPI_PER_DPPX,
        };
    }

    fn toDppx(): f64 {
        return switch (_.unit) {
            .dpi => _.value / DPI_PER_DPPX,
            .dpcm => (_.value * CM_PER_INCH) / DPI_PER_DPPX,
            .dppx => _.value,
        };
    }

    fn toDpcm(): f64 {
        return switch (_.unit) {
            .dpi => _.value / CM_PER_INCH,
            .dpcm => _.value,
            .dppx => (_.value * DPI_PER_DPPX) / CM_PER_INCH,
        };
    }

    /// 分辨率等价比较（如 96dpi == 1dppx）
    fn eql(other: Resolution): bool {
        const a = _.toDpi();
        const b = other.toDpi();
        const diff = @abs(a - b);
        const tol = 0.000001 * (@abs(a) + @abs(b) + 1.0);
        return diff <= tol;
    }

    /// 规范 CSS 序列化（dppx 输出 "dppx"）
    fn toCss(buf: []u8): string! {
        return formatCssDim(_.value, _.unit.asStr(), buf);
    }

    /// 简短 CSS 序列化（dppx 输出 "x"）
    fn toCssShort(buf: []u8): string! {
        const u = if (_.unit == .dppx) "x" else _.unit.asStr();
        return formatCssDim(_.value, u, buf);
    }
}

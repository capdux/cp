// Source: Absorbed from /data/dev/bun/src/css/properties/transform.cp | Synced: 2026-11-22T10:14:47Z

pub err Error {
    EmptyInput,
    UnknownFunction,
    InvalidSyntax,
    InvalidArgumentCount,
    InvalidNumber,
    InvalidUnit,
    NegativePerspective,
    BufferTooSmall,
}

const PI: f64 = 3.14159265358979323846;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn trimSpaces(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn isNearZero(v: f64): bool {
    return @abs(v) <= 0.000000001;
}

fn isNear(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn writeU64(v_in: u64, dst: []u8): usize {
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

fn appendStr(buf: []u8, pos: *usize, s: string)! {
    if (pos.* + s.len > buf.len) {
        return err.BufferTooSmall;
    }
    @memcpy(buf[pos.* .. pos.* + s.len], s);
    pos.* += s.len;
}

fn appendNum(buf: []u8, pos: *usize, val_in: f64)! {
    let val = val_in;
    if (!std.math.isFinite(val)) {
        return err.InvalidNumber;
    }
    if (@abs(val) <= 0.0000005) {
        val = 0.0;
    }
    let tmp: [64]u8 = [0; 64];
    let p: usize = 0;
    if (val < 0.0) {
        tmp[p] = '-';
        p += 1;
        val = -val;
    }
    const scaled = @round(val * 1000000.0);
    const total_u64 = scaled as u64;
    const int_part = total_u64 / 1000000;
    const frac_part = total_u64 % 1000000;
    p += writeU64(int_part, tmp[p..64]);
    if (frac_part > 0) {
        tmp[p] = '.';
        p += 1;
        let fbuf: [6]u8 = ['0'; 6];
        let f = frac_part;
        let k: usize = 6;
        while (k > 0) {
            k -= 1;
            fbuf[k] = ('0' as u8) + ((f % 10) as u8);
            f /= 10;
        }
        let fend: usize = 6;
        while (fend > 0 && fbuf[fend - 1] == '0') { fend -= 1; }
        @memcpy(tmp[p .. p + fend], fbuf[0..fend]);
        p += fend;
    }
    appendStr(buf, pos, tmp[0..p])?;
}

fn appendLenPx(buf: []u8, pos: *usize, px: f64)! {
    if (@abs(px) <= 0.0000005) {
        appendStr(buf, pos, "0")?;
        return;
    }
    appendNum(buf, pos, px)?;
    appendStr(buf, pos, "px")?;
}

fn appendDeg(buf: []u8, pos: *usize, deg: f64)! {
    if (@abs(deg) <= 0.0000005) {
        appendStr(buf, pos, "0deg")?;
        return;
    }
    appendNum(buf, pos, deg)?;
    appendStr(buf, pos, "deg")?;
}

/// 2D 仿射变换矩阵 `matrix(a, b, c, d, e, f)`
pub struct Matrix2d {
    a: f64 = 1.0,
    b: f64 = 0.0,
    c: f64 = 0.0,
    d: f64 = 1.0,
    e: f64 = 0.0,
    f: f64 = 0.0,

    static fn identity(): Matrix2d {
        return Matrix2d{};
    }

    fn determinant(): f64 {
        return _.a * _.d - _.b * _.c;
    }

    fn isInvertible(): bool {
        return @abs(_.determinant()) > 0.000000000001;
    }

    fn isIdentity(): bool {
        return isNear(_.a, 1.0, 0.000000001) &&
            isNearZero(_.b) &&
            isNearZero(_.c) &&
            isNear(_.d, 1.0, 0.000000001) &&
            isNearZero(_.e) &&
            isNearZero(_.f);
    }

    fn multiply(rhs: Matrix2d): Matrix2d {
        return Matrix2d{
            a: _.a * rhs.a + _.c * rhs.b,
            b: _.b * rhs.a + _.d * rhs.b,
            c: _.a * rhs.c + _.c * rhs.d,
            d: _.b * rhs.c + _.d * rhs.d,
            e: _.a * rhs.e + _.c * rhs.f + _.e,
            f: _.b * rhs.e + _.d * rhs.f + _.f,
        };
    }

    fn toMatrix3d(): Matrix3d {
        return Matrix3d{
            m11: _.a, m12: _.b, m13: 0.0, m14: 0.0,
            m21: _.c, m22: _.d, m23: 0.0, m24: 0.0,
            m31: 0.0, m32: 0.0, m33: 1.0, m34: 0.0,
            m41: _.e, m42: _.f, m43: 0.0, m44: 1.0,
        };
    }

    fn approxEql(other: Matrix2d, eps: f64): bool {
        return isNear(_.a, other.a, eps) &&
            isNear(_.b, other.b, eps) &&
            isNear(_.c, other.c, eps) &&
            isNear(_.d, other.d, eps) &&
            isNear(_.e, other.e, eps) &&
            isNear(_.f, other.f, eps);
    }
}

/// 4x4 列主序 3D 齐次变换矩阵 `matrix3d(m11..m44)`
pub struct Matrix3d {
    m11: f64 = 1.0, m12: f64 = 0.0, m13: f64 = 0.0, m14: f64 = 0.0,
    m21: f64 = 0.0, m22: f64 = 1.0, m23: f64 = 0.0, m24: f64 = 0.0,
    m31: f64 = 0.0, m32: f64 = 0.0, m33: f64 = 1.0, m34: f64 = 0.0,
    m41: f64 = 0.0, m42: f64 = 0.0, m43: f64 = 0.0, m44: f64 = 1.0,

    static fn identity(): Matrix3d {
        return Matrix3d{};
    }

    static fn translation(tx: f64, ty: f64, tz: f64): Matrix3d {
        return Matrix3d{ m41: tx, m42: ty, m43: tz };
    }

    static fn scaling(sx: f64, sy: f64, sz: f64): Matrix3d {
        return Matrix3d{ m11: sx, m22: sy, m33: sz };
    }

    static fn rotationZ(rad: f64): Matrix3d {
        const c = @cos(rad);
        const s = @sin(rad);
        return Matrix3d{
            m11: c, m12: s,
            m21: -s, m22: c,
        };
    }

    static fn rotationX(rad: f64): Matrix3d {
        const c = @cos(rad);
        const s = @sin(rad);
        return Matrix3d{
            m22: c, m23: s,
            m32: -s, m33: c,
        };
    }

    static fn rotationY(rad: f64): Matrix3d {
        const c = @cos(rad);
        const s = @sin(rad);
        return Matrix3d{
            m11: c, m13: -s,
            m31: s, m33: c,
        };
    }

    static fn rotation3d(x: f64, y: f64, z: f64, rad: f64): Matrix3d {
        const len_sq = x * x + y * y + z * z;
        if (len_sq <= 0.000000000001) {
            return Matrix3d.identity();
        }
        const inv_len = 1.0 / @sqrt(len_sq);
        const nx = x * inv_len;
        const ny = y * inv_len;
        const nz = z * inv_len;

        const c = @cos(rad);
        const s = @sin(rad);
        const omc = 1.0 - c;

        return Matrix3d{
            m11: nx * nx * omc + c,
            m12: ny * nx * omc + nz * s,
            m13: nz * nx * omc - ny * s,
            m14: 0.0,
            m21: nx * ny * omc - nz * s,
            m22: ny * ny * omc + c,
            m23: nz * ny * omc + nx * s,
            m24: 0.0,
            m31: nx * nz * omc + ny * s,
            m32: ny * nz * omc - nx * s,
            m33: nz * nz * omc + c,
            m34: 0.0,
            m41: 0.0, m42: 0.0, m43: 0.0, m44: 1.0,
        };
    }

    static fn skewing(ax_rad: f64, ay_rad: f64): Matrix3d {
        return Matrix3d{
            m12: @tan(ay_rad),
            m21: @tan(ax_rad),
        };
    }

    static fn perspectiveDist(d: f64): Matrix3d {
        if (d <= 0.0) {
            return Matrix3d.identity();
        }
        return Matrix3d{ m34: -1.0 / d };
    }

    /// 列主序 4x4 矩阵乘法：A * B（先应用 B，再应用 A）
    fn multiply(b: Matrix3d): Matrix3d {
        return Matrix3d{
            m11: _.m11 * b.m11 + _.m21 * b.m12 + _.m31 * b.m13 + _.m41 * b.m14,
            m12: _.m12 * b.m11 + _.m22 * b.m12 + _.m32 * b.m13 + _.m42 * b.m14,
            m13: _.m13 * b.m11 + _.m23 * b.m12 + _.m33 * b.m13 + _.m43 * b.m14,
            m14: _.m14 * b.m11 + _.m24 * b.m12 + _.m34 * b.m13 + _.m44 * b.m14,

            m21: _.m11 * b.m21 + _.m21 * b.m22 + _.m31 * b.m23 + _.m41 * b.m24,
            m22: _.m12 * b.m21 + _.m22 * b.m22 + _.m32 * b.m23 + _.m42 * b.m24,
            m23: _.m13 * b.m21 + _.m23 * b.m22 + _.m33 * b.m23 + _.m43 * b.m24,
            m24: _.m14 * b.m21 + _.m24 * b.m22 + _.m34 * b.m23 + _.m44 * b.m24,

            m31: _.m11 * b.m31 + _.m21 * b.m32 + _.m31 * b.m33 + _.m41 * b.m34,
            m32: _.m12 * b.m31 + _.m22 * b.m32 + _.m32 * b.m33 + _.m42 * b.m34,
            m33: _.m13 * b.m31 + _.m23 * b.m32 + _.m33 * b.m33 + _.m43 * b.m34,
            m34: _.m14 * b.m31 + _.m24 * b.m32 + _.m34 * b.m33 + _.m44 * b.m34,

            m41: _.m11 * b.m41 + _.m21 * b.m42 + _.m31 * b.m43 + _.m41 * b.m44,
            m42: _.m12 * b.m41 + _.m22 * b.m42 + _.m32 * b.m43 + _.m42 * b.m44,
            m43: _.m13 * b.m41 + _.m23 * b.m42 + _.m33 * b.m43 + _.m43 * b.m44,
            m44: _.m14 * b.m41 + _.m24 * b.m42 + _.m34 * b.m43 + _.m44 * b.m44,
        };
    }

    fn is2d(): bool {
        return isNearZero(_.m13) && isNearZero(_.m14) &&
            isNearZero(_.m23) && isNearZero(_.m24) &&
            isNearZero(_.m31) && isNearZero(_.m32) &&
            isNear(_.m33, 1.0, 0.000000001) && isNearZero(_.m34) &&
            isNearZero(_.m43) && isNear(_.m44, 1.0, 0.000000001);
    }

    fn isIdentity(): bool {
        return _.approxEql(Matrix3d.identity(), 0.000000001);
    }

    fn toMatrix2d(): Matrix2d? {
        if (!_.is2d()) {
            return null;
        }
        return Matrix2d{
            a: _.m11, b: _.m12,
            c: _.m21, d: _.m22,
            e: _.m41, f: _.m42,
        };
    }

    /// 完整 4x4 矩阵行列式
    fn determinant(): f64 {
        const s0 = _.m11 * _.m22 - _.m12 * _.m21;
        const s1 = _.m11 * _.m23 - _.m13 * _.m21;
        const s2 = _.m11 * _.m24 - _.m14 * _.m21;
        const s3 = _.m12 * _.m23 - _.m13 * _.m22;
        const s4 = _.m12 * _.m24 - _.m14 * _.m22;
        const s5 = _.m13 * _.m24 - _.m14 * _.m23;

        const c5 = _.m33 * _.m44 - _.m34 * _.m43;
        const c4 = _.m32 * _.m44 - _.m34 * _.m42;
        const c3 = _.m32 * _.m43 - _.m33 * _.m42;
        const c2 = _.m31 * _.m44 - _.m34 * _.m41;
        const c1 = _.m31 * _.m43 - _.m33 * _.m41;
        const c0 = _.m31 * _.m42 - _.m32 * _.m41;

        return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    }

    fn isInvertible(): bool {
        return @abs(_.determinant()) > 0.000000000001;
    }

    fn approxEql(o: Matrix3d, eps: f64): bool {
        return isNear(_.m11, o.m11, eps) && isNear(_.m12, o.m12, eps) && isNear(_.m13, o.m13, eps) && isNear(_.m14, o.m14, eps) &&
            isNear(_.m21, o.m21, eps) && isNear(_.m22, o.m22, eps) && isNear(_.m23, o.m23, eps) && isNear(_.m24, o.m24, eps) &&
            isNear(_.m31, o.m31, eps) && isNear(_.m32, o.m32, eps) && isNear(_.m33, o.m33, eps) && isNear(_.m34, o.m34, eps) &&
            isNear(_.m41, o.m41, eps) && isNear(_.m42, o.m42, eps) && isNear(_.m43, o.m43, eps) && isNear(_.m44, o.m44, eps);
    }
}

/// CSS `transform-style` 枚举
pub enum TransformStyle: u8 {
    flat = 0,
    preserve_3d = 1,

    static fn fromStr(s: string): TransformStyle? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "flat")) { return .flat; }
        if (eqlIgnoreCaseAscii(t, "preserve-3d")) { return .preserve_3d; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .flat => "flat",
            .preserve_3d => "preserve-3d",
        };
    }

    fn is3d(): bool {
        return _ == .preserve_3d;
    }
}

/// CSS `backface-visibility` 枚举
pub enum BackfaceVisibility: u8 {
    visible = 0,
    hidden = 1,

    static fn fromStr(s: string): BackfaceVisibility? {
        const t = trimSpaces(s);
        if (eqlIgnoreCaseAscii(t, "visible")) { return .visible; }
        if (eqlIgnoreCaseAscii(t, "hidden")) { return .hidden; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .visible => "visible",
            .hidden => "hidden",
        };
    }

    fn isHidden(): bool {
        return _ == .hidden;
    }
}

/// CSS `transform-origin` 原点结构体（支持百分比/px/关键词与 Z 轴偏移）
pub struct TransformOrigin {
    x: f64 = 50.0,
    y: f64 = 50.0,
    z: f64 = 0.0,
    x_is_percent: bool = true,
    y_is_percent: bool = true,

    static fn center(): TransformOrigin {
        return TransformOrigin{};
    }

    static fn parse(input: string): TransformOrigin! {
        const s = trimSpaces(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        let toks: [4]string = ["", "", "", ""];
        let count: usize = 0;
        let i: usize = 0;
        while (i < s.len) {
            while (i < s.len && isSpace(s[i])) { i += 1; }
            if (i >= s.len) { break; }
            const start = i;
            while (i < s.len && !isSpace(s[i])) { i += 1; }
            if (count >= 3) {
                return err.InvalidArgumentCount;
            }
            toks[count] = s[start..i];
            count += 1;
        }
        if (count == 0) {
            return err.EmptyInput;
        }

        let origin = TransformOrigin{};
        if (count == 1) {
            const t0 = toks[0];
            if (eqlIgnoreCaseAscii(t0, "top")) {
                origin.x = 50.0; origin.y = 0.0;
            } else if (eqlIgnoreCaseAscii(t0, "bottom")) {
                origin.x = 50.0; origin.y = 100.0;
            } else if (eqlIgnoreCaseAscii(t0, "left")) {
                origin.x = 0.0; origin.y = 50.0;
            } else if (eqlIgnoreCaseAscii(t0, "right")) {
                origin.x = 100.0; origin.y = 50.0;
            } else if (eqlIgnoreCaseAscii(t0, "center")) {
                origin.x = 50.0; origin.y = 50.0;
            } else {
                const parsed_x = parseOriginCoord(t0)?;
                origin.x = parsed_x.val;
                origin.x_is_percent = parsed_x.is_pct;
                origin.y = 50.0;
                origin.y_is_percent = true;
            }
            return origin;
        }

        const px_v = parseOriginCoord(toks[0])?;
        const py_v = parseOriginCoord(toks[1])?;
        origin.x = px_v.val;
        origin.x_is_percent = px_v.is_pct;
        origin.y = py_v.val;
        origin.y_is_percent = py_v.is_pct;

        if (count == 3) {
            origin.z = parseLengthPx(toks[2])?;
        }
        return origin;
    }

    fn toCss(buf: []u8): string! {
        let pos: usize = 0;
        if (_.x_is_percent) {
            appendNum(buf, *pos, _.x)?;
            appendStr(buf, *pos, "%")?;
        } else {
            appendLenPx(buf, *pos, _.x)?;
        }
        appendStr(buf, *pos, " ")?;
        if (_.y_is_percent) {
            appendNum(buf, *pos, _.y)?;
            appendStr(buf, *pos, "%")?;
        } else {
            appendLenPx(buf, *pos, _.y)?;
        }
        if (!isNearZero(_.z)) {
            appendStr(buf, *pos, " ")?;
            appendLenPx(buf, *pos, _.z)?;
        }
        return buf[0..pos];
    }
}

struct OriginCoord {
    val: f64,
    is_pct: bool,
}

fn parseOriginCoord(tok: string): OriginCoord! {
    if (eqlIgnoreCaseAscii(tok, "left") || eqlIgnoreCaseAscii(tok, "top")) {
        return OriginCoord{ val: 0.0, is_pct: true };
    }
    if (eqlIgnoreCaseAscii(tok, "center")) {
        return OriginCoord{ val: 50.0, is_pct: true };
    }
    if (eqlIgnoreCaseAscii(tok, "right") || eqlIgnoreCaseAscii(tok, "bottom")) {
        return OriginCoord{ val: 100.0, is_pct: true };
    }
    if (tok.len > 1 && tok[tok.len - 1] == '%') {
        const v = std.fmt.parseFloat(f64, tok[0 .. tok.len - 1]) catch return err.InvalidNumber;
        if (!std.math.isFinite(v)) { return err.InvalidNumber; }
        return OriginCoord{ val: v, is_pct: true };
    }
    const px = parseLengthPx(tok)?;
    return OriginCoord{ val: px, is_pct: false };
}

fn parsePlainNumber(tok: string): f64! {
    const s = trimSpaces(tok);
    if (s.len == 0) { return err.InvalidNumber; }
    const v = std.fmt.parseFloat(f64, s) catch return err.InvalidNumber;
    if (!std.math.isFinite(v)) { return err.InvalidNumber; }
    return v;
}

fn parseScaleFactor(tok: string): f64! {
    const s = trimSpaces(tok);
    if (s.len == 0) { return err.InvalidNumber; }
    if (s[s.len - 1] == '%') {
        const pct = parsePlainNumber(s[0 .. s.len - 1])?;
        return pct / 100.0;
    }
    return parsePlainNumber(s);
}

fn parseLengthPx(tok: string): f64! {
    const s = trimSpaces(tok);
    if (s.len == 0) { return err.InvalidNumber; }
    if (s.len >= 2 && eqlIgnoreCaseAscii(s[s.len - 2 ..], "px")) {
        return parsePlainNumber(s[0 .. s.len - 2]);
    }
    const v = parsePlainNumber(s)?;
    if (v == 0.0) { return 0.0; }
    return err.InvalidUnit;
}

fn parseAngleDeg(tok: string): f64! {
    const s = trimSpaces(tok);
    if (s.len == 0) { return err.InvalidNumber; }
    if (s.len >= 4 && eqlIgnoreCaseAscii(s[s.len - 4 ..], "grad")) {
        return (parsePlainNumber(s[0 .. s.len - 4])?) * 0.9;
    }
    if (s.len >= 4 && eqlIgnoreCaseAscii(s[s.len - 4 ..], "turn")) {
        return (parsePlainNumber(s[0 .. s.len - 4])?) * 360.0;
    }
    if (s.len >= 3 && eqlIgnoreCaseAscii(s[s.len - 3 ..], "deg")) {
        return parsePlainNumber(s[0 .. s.len - 3]);
    }
    if (s.len >= 3 && eqlIgnoreCaseAscii(s[s.len - 3 ..], "rad")) {
        return (parsePlainNumber(s[0 .. s.len - 3])?) * (180.0 / PI);
    }
    const v = parsePlainNumber(s)?;
    if (v == 0.0) { return 0.0; }
    return err.InvalidUnit;
}

/// 21 种标准 CSS Transform 函数类型
pub enum TransformKind: u8 {
    translate = 0,
    translate_x = 1,
    translate_y = 2,
    translate_z = 3,
    translate_3d = 4,
    scale = 5,
    scale_x = 6,
    scale_y = 7,
    scale_z = 8,
    scale_3d = 9,
    rotate = 10,
    rotate_x = 11,
    rotate_y = 12,
    rotate_z = 13,
    rotate_3d = 14,
    skew = 15,
    skew_x = 16,
    skew_y = 17,
    perspective = 18,
    matrix = 19,
    matrix_3d = 20,
}

/// 单个 2D/3D CSS Transform 操作
pub struct Transform {
    kind: TransformKind,
    a: f64 = 0.0,
    b: f64 = 0.0,
    c: f64 = 0.0,
    d: f64 = 0.0,
    mat: Matrix3d = Matrix3d.identity(),

    static fn translate(x: f64, y: f64): Transform {
        return Transform{ kind: .translate, a: x, b: y };
    }
    static fn translateX(x: f64): Transform {
        return Transform{ kind: .translate_x, a: x };
    }
    static fn translateY(y: f64): Transform {
        return Transform{ kind: .translate_y, a: y };
    }
    static fn translateZ(z: f64): Transform {
        return Transform{ kind: .translate_z, a: z };
    }
    static fn translate3d(x: f64, y: f64, z: f64): Transform {
        return Transform{ kind: .translate_3d, a: x, b: y, c: z };
    }
    static fn scale(x: f64, y: f64): Transform {
        return Transform{ kind: .scale, a: x, b: y };
    }
    static fn scaleX(x: f64): Transform {
        return Transform{ kind: .scale_x, a: x };
    }
    static fn scaleY(y: f64): Transform {
        return Transform{ kind: .scale_y, a: y };
    }
    static fn scaleZ(z: f64): Transform {
        return Transform{ kind: .scale_z, a: z };
    }
    static fn scale3d(x: f64, y: f64, z: f64): Transform {
        return Transform{ kind: .scale_3d, a: x, b: y, c: z };
    }
    static fn rotate(deg: f64): Transform {
        return Transform{ kind: .rotate, a: deg };
    }
    static fn rotateX(deg: f64): Transform {
        return Transform{ kind: .rotate_x, a: deg };
    }
    static fn rotateY(deg: f64): Transform {
        return Transform{ kind: .rotate_y, a: deg };
    }
    static fn rotateZ(deg: f64): Transform {
        return Transform{ kind: .rotate_z, a: deg };
    }
    static fn rotate3d(x: f64, y: f64, z: f64, deg: f64): Transform {
        return Transform{ kind: .rotate_3d, a: x, b: y, c: z, d: deg };
    }
    static fn skew(ax_deg: f64, ay_deg: f64): Transform {
        return Transform{ kind: .skew, a: ax_deg, b: ay_deg };
    }
    static fn skewX(ax_deg: f64): Transform {
        return Transform{ kind: .skew_x, a: ax_deg };
    }
    static fn skewY(ay_deg: f64): Transform {
        return Transform{ kind: .skew_y, a: ay_deg };
    }
    static fn perspective(dist_px: f64): Transform {
        return Transform{ kind: .perspective, a: dist_px };
    }
    static fn fromMatrix2d(m: Matrix2d): Transform {
        return Transform{ kind: .matrix, mat: m.toMatrix3d() };
    }
    static fn fromMatrix3d(m: Matrix3d): Transform {
        return Transform{ kind: .matrix_3d, mat: m };
    }

    fn is3d(): bool {
        return switch (_.kind) {
            .translate_z | .translate_3d | .scale_z | .scale_3d | .rotate_x | .rotate_y | .rotate_z | .rotate_3d | .perspective | .matrix_3d => true,
            else => false,
        };
    }

    /// 精确转换为 4x4 齐次变换矩阵
    fn toMatrix3d(): Matrix3d {
        const deg2rad = PI / 180.0;
        return switch (_.kind) {
            .translate => Matrix3d.translation(_.a, _.b, 0.0),
            .translate_x => Matrix3d.translation(_.a, 0.0, 0.0),
            .translate_y => Matrix3d.translation(0.0, _.a, 0.0),
            .translate_z => Matrix3d.translation(0.0, 0.0, _.a),
            .translate_3d => Matrix3d.translation(_.a, _.b, _.c),
            .scale => Matrix3d.scaling(_.a, _.b, 1.0),
            .scale_x => Matrix3d.scaling(_.a, 1.0, 1.0),
            .scale_y => Matrix3d.scaling(1.0, _.a, 1.0),
            .scale_z => Matrix3d.scaling(1.0, 1.0, _.a),
            .scale_3d => Matrix3d.scaling(_.a, _.b, _.c),
            .rotate | .rotate_z => Matrix3d.rotationZ(_.a * deg2rad),
            .rotate_x => Matrix3d.rotationX(_.a * deg2rad),
            .rotate_y => Matrix3d.rotationY(_.a * deg2rad),
            .rotate_3d => Matrix3d.rotation3d(_.a, _.b, _.c, _.d * deg2rad),
            .skew => Matrix3d.skewing(_.a * deg2rad, _.b * deg2rad),
            .skew_x => Matrix3d.skewing(_.a * deg2rad, 0.0),
            .skew_y => Matrix3d.skewing(0.0, _.a * deg2rad),
            .perspective => Matrix3d.perspectiveDist(_.a),
            .matrix | .matrix_3d => _.mat,
        };
    }

    /// 规范 CSS 序列化（内置简写优化：translate(x, 0) -> translateX(x)、scale(s, s) -> scale(s) 等）
    fn toCss(buf: []u8): string! {
        let pos: usize = 0;
        switch (_.kind) {
            .translate => {
                if (isNearZero(_.b)) {
                    appendStr(buf, *pos, "translateX(")?;
                    appendLenPx(buf, *pos, _.a)?;
                    appendStr(buf, *pos, ")")?;
                } else {
                    appendStr(buf, *pos, "translate(")?;
                    appendLenPx(buf, *pos, _.a)?;
                    appendStr(buf, *pos, ", ")?;
                    appendLenPx(buf, *pos, _.b)?;
                    appendStr(buf, *pos, ")")?;
                }
            },
            .translate_x => {
                appendStr(buf, *pos, "translateX(")?;
                appendLenPx(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .translate_y => {
                appendStr(buf, *pos, "translateY(")?;
                appendLenPx(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .translate_z => {
                appendStr(buf, *pos, "translateZ(")?;
                appendLenPx(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .translate_3d => {
                appendStr(buf, *pos, "translate3d(")?;
                appendLenPx(buf, *pos, _.a)?;
                appendStr(buf, *pos, ", ")?;
                appendLenPx(buf, *pos, _.b)?;
                appendStr(buf, *pos, ", ")?;
                appendLenPx(buf, *pos, _.c)?;
                appendStr(buf, *pos, ")")?;
            },
            .scale => {
                appendStr(buf, *pos, "scale(")?;
                appendNum(buf, *pos, _.a)?;
                if (!isNear(_.a, _.b, 0.000000001)) {
                    appendStr(buf, *pos, ", ")?;
                    appendNum(buf, *pos, _.b)?;
                }
                appendStr(buf, *pos, ")")?;
            },
            .scale_x => {
                appendStr(buf, *pos, "scaleX(")?;
                appendNum(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .scale_y => {
                appendStr(buf, *pos, "scaleY(")?;
                appendNum(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .scale_z => {
                appendStr(buf, *pos, "scaleZ(")?;
                appendNum(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .scale_3d => {
                appendStr(buf, *pos, "scale3d(")?;
                appendNum(buf, *pos, _.a)?;
                appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.b)?;
                appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.c)?;
                appendStr(buf, *pos, ")")?;
            },
            .rotate => {
                appendStr(buf, *pos, "rotate(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .rotate_x => {
                appendStr(buf, *pos, "rotateX(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .rotate_y => {
                appendStr(buf, *pos, "rotateY(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .rotate_z => {
                appendStr(buf, *pos, "rotateZ(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .rotate_3d => {
                appendStr(buf, *pos, "rotate3d(")?;
                appendNum(buf, *pos, _.a)?;
                appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.b)?;
                appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.c)?;
                appendStr(buf, *pos, ", ")?;
                appendDeg(buf, *pos, _.d)?;
                appendStr(buf, *pos, ")")?;
            },
            .skew => {
                if (isNearZero(_.b)) {
                    appendStr(buf, *pos, "skewX(")?;
                    appendDeg(buf, *pos, _.a)?;
                    appendStr(buf, *pos, ")")?;
                } else {
                    appendStr(buf, *pos, "skew(")?;
                    appendDeg(buf, *pos, _.a)?;
                    appendStr(buf, *pos, ", ")?;
                    appendDeg(buf, *pos, _.b)?;
                    appendStr(buf, *pos, ")")?;
                }
            },
            .skew_x => {
                appendStr(buf, *pos, "skewX(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .skew_y => {
                appendStr(buf, *pos, "skewY(")?;
                appendDeg(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .perspective => {
                appendStr(buf, *pos, "perspective(")?;
                appendLenPx(buf, *pos, _.a)?;
                appendStr(buf, *pos, ")")?;
            },
            .matrix => {
                appendStr(buf, *pos, "matrix(")?;
                appendNum(buf, *pos, _.mat.m11)?; appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.mat.m12)?; appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.mat.m21)?; appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.mat.m22)?; appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.mat.m41)?; appendStr(buf, *pos, ", ")?;
                appendNum(buf, *pos, _.mat.m42)?;
                appendStr(buf, *pos, ")")?;
            },
            .matrix_3d => {
                appendStr(buf, *pos, "matrix3d(")?;
                const vals: [16]f64 = [
                    _.mat.m11, _.mat.m12, _.mat.m13, _.mat.m14,
                    _.mat.m21, _.mat.m22, _.mat.m23, _.mat.m24,
                    _.mat.m31, _.mat.m32, _.mat.m33, _.mat.m34,
                    _.mat.m41, _.mat.m42, _.mat.m43, _.mat.m44,
                ];
                let idx: usize = 0;
                while (idx < 16) {
                    if (idx > 0) { appendStr(buf, *pos, ", ")?; }
                    appendNum(buf, *pos, vals[idx])?;
                    idx += 1;
                }
                appendStr(buf, *pos, ")")?;
            },
        }
        return buf[0..pos];
    }
}

fn splitCommaArgs(args_str: string, out: []string): usize! {
    const s = trimSpaces(args_str);
    if (s.len == 0) {
        return err.InvalidArgumentCount;
    }
    let count: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i == s.len || s[i] == ',') {
            const seg = trimSpaces(s[start..i]);
            if (seg.len == 0) {
                return err.InvalidSyntax;
            }
            if (count >= out.len) {
                return err.InvalidArgumentCount;
            }
            out[count] = seg;
            count += 1;
            start = i + 1;
        }
        i += 1;
    }
    return count;
}

fn parseTransformCall(fn_name: string, args_str: string): Transform! {
    let args: [16]string = [""; 16];
    const argc = splitCommaArgs(args_str, args[0..16])?;

    if (eqlIgnoreCaseAscii(fn_name, "translate")) {
        if (argc == 1) { return Transform.translate(parseLengthPx(args[0])?, 0.0); }
        if (argc == 2) { return Transform.translate(parseLengthPx(args[0])?, parseLengthPx(args[1])?); }
        return err.InvalidArgumentCount;
    }
    if (eqlIgnoreCaseAscii(fn_name, "translateX")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.translateX(parseLengthPx(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "translateY")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.translateY(parseLengthPx(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "translateZ")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.translateZ(parseLengthPx(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "translate3d")) {
        if (argc != 3) { return err.InvalidArgumentCount; }
        return Transform.translate3d(parseLengthPx(args[0])?, parseLengthPx(args[1])?, parseLengthPx(args[2])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "scale")) {
        if (argc == 1) {
            const s = parseScaleFactor(args[0])?;
            return Transform.scale(s, s);
        }
        if (argc == 2) { return Transform.scale(parseScaleFactor(args[0])?, parseScaleFactor(args[1])?); }
        return err.InvalidArgumentCount;
    }
    if (eqlIgnoreCaseAscii(fn_name, "scaleX")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.scaleX(parseScaleFactor(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "scaleY")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.scaleY(parseScaleFactor(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "scaleZ")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.scaleZ(parseScaleFactor(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "scale3d")) {
        if (argc != 3) { return err.InvalidArgumentCount; }
        return Transform.scale3d(parseScaleFactor(args[0])?, parseScaleFactor(args[1])?, parseScaleFactor(args[2])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "rotate")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.rotate(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "rotateX")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.rotateX(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "rotateY")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.rotateY(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "rotateZ")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.rotateZ(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "rotate3d")) {
        if (argc != 4) { return err.InvalidArgumentCount; }
        return Transform.rotate3d(
            parsePlainNumber(args[0])?,
            parsePlainNumber(args[1])?,
            parsePlainNumber(args[2])?,
            parseAngleDeg(args[3])?,
        );
    }
    if (eqlIgnoreCaseAscii(fn_name, "skew")) {
        if (argc == 1) { return Transform.skew(parseAngleDeg(args[0])?, 0.0); }
        if (argc == 2) { return Transform.skew(parseAngleDeg(args[0])?, parseAngleDeg(args[1])?); }
        return err.InvalidArgumentCount;
    }
    if (eqlIgnoreCaseAscii(fn_name, "skewX")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.skewX(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "skewY")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        return Transform.skewY(parseAngleDeg(args[0])?);
    }
    if (eqlIgnoreCaseAscii(fn_name, "perspective")) {
        if (argc != 1) { return err.InvalidArgumentCount; }
        const d = parseLengthPx(args[0])?;
        if (d < 0.0) { return err.NegativePerspective; }
        return Transform.perspective(d);
    }
    if (eqlIgnoreCaseAscii(fn_name, "matrix")) {
        if (argc != 6) { return err.InvalidArgumentCount; }
        return Transform.fromMatrix2d(Matrix2d{
            a: parsePlainNumber(args[0])?,
            b: parsePlainNumber(args[1])?,
            c: parsePlainNumber(args[2])?,
            d: parsePlainNumber(args[3])?,
            e: parsePlainNumber(args[4])?,
            f: parsePlainNumber(args[5])?,
        });
    }
    if (eqlIgnoreCaseAscii(fn_name, "matrix3d")) {
        if (argc != 16) { return err.InvalidArgumentCount; }
        return Transform.fromMatrix3d(Matrix3d{
            m11: parsePlainNumber(args[0])?, m12: parsePlainNumber(args[1])?, m13: parsePlainNumber(args[2])?, m14: parsePlainNumber(args[3])?,
            m21: parsePlainNumber(args[4])?, m22: parsePlainNumber(args[5])?, m23: parsePlainNumber(args[6])?, m24: parsePlainNumber(args[7])?,
            m31: parsePlainNumber(args[8])?, m32: parsePlainNumber(args[9])?, m33: parsePlainNumber(args[10])?, m34: parsePlainNumber(args[11])?,
            m41: parsePlainNumber(args[12])?, m42: parsePlainNumber(args[13])?, m43: parsePlainNumber(args[14])?, m44: parsePlainNumber(args[15])?,
        });
    }

    return err.UnknownFunction;
}

/// CSS `transform` 属性列表（含 `~init()` 确定性 RAII 释放）
pub struct TransformList {
    allocator: std.mem.Allocator? = null,
    items_slice: ([]const Transform)? = null,

    ~init() {
        if (slice := _.items_slice) {
            if (alloc := _.allocator) {
                alloc.free(slice);
            }
            _.items_slice = null;
        }
    }

    static fn none(): TransformList {
        return TransformList{};
    }

    /// 解析 CSS `transform` 列表字符串（支持 `none` 或空格分隔的多变换函数）
    static fn parse(allocator: std.mem.Allocator, input: string): TransformList! {
        const s = trimSpaces(input);
        if (s.len == 0) {
            return err.EmptyInput;
        }
        if (eqlIgnoreCaseAscii(s, "none")) {
            return TransformList.none();
        }

        let list = std.array_list.Managed(Transform).init(allocator);
        defer list.deinit();

        let pos: usize = 0;
        while (pos < s.len) {
            while (pos < s.len && isSpace(s[pos])) { pos += 1; }
            if (pos >= s.len) { break; }

            const fn_start = pos;
            while (pos < s.len && s[pos] != '(' && !isSpace(s[pos])) { pos += 1; }
            if (pos >= s.len || s[pos] != '(') {
                return err.InvalidSyntax;
            }
            const fn_name = s[fn_start..pos];
            pos += 1; // skip '('

            const args_start = pos;
            while (pos < s.len && s[pos] != ')') { pos += 1; }
            if (pos >= s.len) {
                return err.InvalidSyntax;
            }
            const args_str = s[args_start..pos];
            pos += 1; // skip ')'

            const t = parseTransformCall(fn_name, args_str)?;
            list.append(t)?;
        }

        if (list.items.len == 0) {
            return err.InvalidSyntax;
        }

        const owned = list.toOwnedSlice()?;
        return TransformList{
            allocator: allocator,
            items_slice: owned,
        };
    }

    fn isNone(): bool {
        return _.len() == 0;
    }

    fn len(): usize {
        if (slice := _.items_slice) {
            return slice.len;
        }
        return 0;
    }

    fn items(): []const Transform {
        if (slice := _.items_slice) {
            return slice;
        }
        const empty: [0]Transform = [];
        return empty[0..0];
    }

    /// 按 CSS 规范从左到右合成最终 4x4 变换矩阵
    fn toMatrix3d(): Matrix3d {
        let acc = Matrix3d.identity();
        const arr = _.items();
        let i: usize = 0;
        while (i < arr.len) {
            acc = acc.multiply(arr[i].toMatrix3d());
            i += 1;
        }
        return acc;
    }

    fn is2d(): bool {
        return _.toMatrix3d().is2d();
    }

    /// 规范 CSS 序列化写入目标缓冲区
    fn toCss(buf: []u8): string! {
        const arr = _.items();
        if (arr.len == 0) {
            if (buf.len < 4) { return err.BufferTooSmall; }
            @memcpy(buf[0..4], "none");
            return buf[0..4];
        }
        let pos: usize = 0;
        let i: usize = 0;
        while (i < arr.len) {
            if (i > 0) {
                appendStr(buf, *pos, " ")?;
            }
            const seg = arr[i].toCss(buf[pos..])?;
            pos += seg.len;
            i += 1;
        }
        return buf[0..pos];
    }
}

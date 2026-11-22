// Source: Absorbed from /data/dev/bun/src/css/values/gradient.cp | Synced: 2026-11-22T05:04:38Z

pub const MAX_STOPS: usize = 16;
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

fn appendStr(buf: []u8, pos: *usize, s: string): bool {
    if (pos.* + s.len > buf.len) {
        return false;
    }
    if (s.len > 0) {
        @memcpy(buf[pos.* .. pos.* + s.len], s);
        pos.* += s.len;
    }
    return true;
}

fn appendDeg(buf: []u8, pos: *usize, deg_in: f64): bool {
    let val = deg_in;
    if (@abs(val) <= 0.0000005) {
        return appendStr(buf, pos, "0deg");
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
    if (!appendStr(buf, pos, tmp[0..p])) { return false; }
    return appendStr(buf, pos, "deg");
}

fn parseFloatStrict(s: string): f64? {
    const t = trimSpaces(s);
    if (t.len == 0) { return null; }
    const v = std.fmt.parseFloat(f64, t) catch return null;
    if (!std.math.isFinite(v)) { return null; }
    return v;
}

fn parseAngleDeg(s: string): f64? {
    const t = trimSpaces(s);
    if (t.len == 0) { return null; }
    if (t.len >= 4 && eqlIgnoreCaseAscii(t[t.len - 4 ..], "grad")) {
        if (v := parseFloatStrict(t[0 .. t.len - 4])) { return v * 0.9; }
        return null;
    }
    if (t.len >= 4 && eqlIgnoreCaseAscii(t[t.len - 4 ..], "turn")) {
        if (v := parseFloatStrict(t[0 .. t.len - 4])) { return v * 360.0; }
        return null;
    }
    if (t.len >= 3 && eqlIgnoreCaseAscii(t[t.len - 3 ..], "deg")) {
        return parseFloatStrict(t[0 .. t.len - 3]);
    }
    if (t.len >= 3 && eqlIgnoreCaseAscii(t[t.len - 3 ..], "rad")) {
        if (v := parseFloatStrict(t[0 .. t.len - 3])) { return v * (180.0 / PI); }
        return null;
    }
    if (v := parseFloatStrict(t)) {
        if (v == 0.0) { return 0.0; }
    }
    return null;
}

/// CSS 渐变类型枚举
pub enum GradientKind: u8 {
    linear = 0,
    repeating_linear = 1,
    radial = 2,
    repeating_radial = 3,
    conic = 4,
    repeating_conic = 5,

    fn fnName(): string {
        return switch (_) {
            .linear => "linear-gradient",
            .repeating_linear => "repeating-linear-gradient",
            .radial => "radial-gradient",
            .repeating_radial => "repeating-radial-gradient",
            .conic => "conic-gradient",
            .repeating_conic => "repeating-conic-gradient",
        };
    }

    fn isRepeating(): bool {
        return _ == .repeating_linear || _ == .repeating_radial || _ == .repeating_conic;
    }
}

/// 线性渐变 `to <side-or-corner>` 8 方向枚举
pub enum DirectionSide: u8 {
    to_top = 0,
    to_bottom = 1,
    to_left = 2,
    to_right = 3,
    to_top_left = 4,
    to_top_right = 5,
    to_bottom_left = 6,
    to_bottom_right = 7,

    fn asStr(): string {
        return switch (_) {
            .to_top => "to top",
            .to_bottom => "to bottom",
            .to_left => "to left",
            .to_right => "to right",
            .to_top_left => "to top left",
            .to_top_right => "to top right",
            .to_bottom_left => "to bottom left",
            .to_bottom_right => "to bottom right",
        };
    }
}

/// 线性渐变方向（角度或 `to <side-or-corner>`）
pub struct LineDirection {
    is_angle: bool = false,
    angle_deg: f64 = 180.0,
    side: DirectionSide = .to_bottom,

    static fn defaultDir(): LineDirection {
        return LineDirection{};
    }

    static fn fromAngle(deg: f64): LineDirection {
        return LineDirection{ is_angle: true, angle_deg: deg, side: .to_bottom };
    }

    static fn fromSide(side: DirectionSide): LineDirection {
        return LineDirection{ is_angle: false, angle_deg: 180.0, side: side };
    }

    static fn parse(raw: string): LineDirection? {
        const s = trimSpaces(raw);
        if (s.len == 0) { return null; }

        if (deg := parseAngleDeg(s)) {
            return LineDirection.fromAngle(deg);
        }

        // 匹配 "to <side> [<side>]"
        let toks: [4]string = ["", "", "", ""];
        const n = splitSpaceTokens(s, toks[0..4]);
        if (n < 2 || n > 3 || !eqlIgnoreCaseAscii(toks[0], "to")) {
            return null;
        }
        if (n == 2) {
            if (eqlIgnoreCaseAscii(toks[1], "top")) { return LineDirection.fromSide(.to_top); }
            if (eqlIgnoreCaseAscii(toks[1], "bottom")) { return LineDirection.fromSide(.to_bottom); }
            if (eqlIgnoreCaseAscii(toks[1], "left")) { return LineDirection.fromSide(.to_left); }
            if (eqlIgnoreCaseAscii(toks[1], "right")) { return LineDirection.fromSide(.to_right); }
            return null;
        }

        // n == 3: 垂直 + 水平任意顺序组合
        const a = toks[1];
        const b = toks[2];
        const is_top = eqlIgnoreCaseAscii(a, "top") || eqlIgnoreCaseAscii(b, "top");
        const is_bottom = eqlIgnoreCaseAscii(a, "bottom") || eqlIgnoreCaseAscii(b, "bottom");
        const is_left = eqlIgnoreCaseAscii(a, "left") || eqlIgnoreCaseAscii(b, "left");
        const is_right = eqlIgnoreCaseAscii(a, "right") || eqlIgnoreCaseAscii(b, "right");

        if (is_top && is_left && !is_bottom && !is_right) { return LineDirection.fromSide(.to_top_left); }
        if (is_top && is_right && !is_bottom && !is_left) { return LineDirection.fromSide(.to_top_right); }
        if (is_bottom && is_left && !is_top && !is_right) { return LineDirection.fromSide(.to_bottom_left); }
        if (is_bottom && is_right && !is_top && !is_left) { return LineDirection.fromSide(.to_bottom_right); }
        return null;
    }

    fn isDefault(): bool {
        if (!_.is_angle) {
            return _.side == .to_bottom;
        }
        return @abs(_.angle_deg - 180.0) <= 0.0001;
    }

    fn toDegrees(): f64 {
        if (_.is_angle) {
            return _.angle_deg;
        }
        return switch (_.side) {
            .to_top => 0.0,
            .to_right => 90.0,
            .to_bottom => 180.0,
            .to_left => 270.0,
            .to_top_right => 45.0,
            .to_bottom_right => 135.0,
            .to_bottom_left => 225.0,
            .to_top_left => 315.0,
        };
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!_.is_angle) {
            return appendStr(buf, pos, _.side.asStr());
        }
        return appendDeg(buf, pos, _.angle_deg);
    }
}

/// 径向渐变形状枚举
pub enum RadialShape: u8 {
    ellipse = 0,
    circle = 1,

    static fn fromStr(s: string): RadialShape? {
        if (eqlIgnoreCaseAscii(s, "ellipse")) { return .ellipse; }
        if (eqlIgnoreCaseAscii(s, "circle")) { return .circle; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .ellipse => "ellipse",
            .circle => "circle",
        };
    }
}

/// 径向渐变尺寸范围枚举
pub enum RadialExtent: u8 {
    farthest_corner = 0,
    closest_side = 1,
    farthest_side = 2,
    closest_corner = 3,

    static fn fromStr(s: string): RadialExtent? {
        if (eqlIgnoreCaseAscii(s, "farthest-corner")) { return .farthest_corner; }
        if (eqlIgnoreCaseAscii(s, "closest-side")) { return .closest_side; }
        if (eqlIgnoreCaseAscii(s, "farthest-side")) { return .farthest_side; }
        if (eqlIgnoreCaseAscii(s, "closest-corner")) { return .closest_corner; }
        return null;
    }

    fn asStr(): string {
        return switch (_) {
            .farthest_corner => "farthest-corner",
            .closest_side => "closest-side",
            .farthest_side => "farthest-side",
            .closest_corner => "closest-corner",
        };
    }
}

/// CSS 渐变色标（支持无位置、单位置及双位置色标，如 `red 10% 30%`）
pub struct ColorStop {
    color: string = "",
    pos1: string = "",
    pos2: string = "",

    fn hasPos1(): bool {
        return _.pos1.len > 0;
    }

    fn hasPos2(): bool {
        return _.pos2.len > 0;
    }

    fn isDoublePosition(): bool {
        return _.pos1.len > 0 && _.pos2.len > 0;
    }

    fn writeTo(buf: []u8, pos: *usize): bool {
        if (!appendStr(buf, pos, _.color)) { return false; }
        if (_.pos1.len > 0) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!appendStr(buf, pos, _.pos1)) { return false; }
        }
        if (_.pos2.len > 0) {
            if (!appendStr(buf, pos, " ")) { return false; }
            if (!appendStr(buf, pos, _.pos2)) { return false; }
        }
        return true;
    }
}

/// 线性渐变结构体
pub struct LinearGradient {
    repeating: bool = false,
    direction: LineDirection = LineDirection.defaultDir(),
    stops: [MAX_STOPS]ColorStop = [ColorStop{}; MAX_STOPS],
    stop_count: usize = 0,

    fn stopSlice(): []const ColorStop {
        return _.stops[0.._.stop_count];
    }
}

/// 径向渐变结构体
pub struct RadialGradient {
    repeating: bool = false,
    shape: RadialShape = .ellipse,
    extent: RadialExtent = .farthest_corner,
    position: string = "",
    stops: [MAX_STOPS]ColorStop = [ColorStop{}; MAX_STOPS],
    stop_count: usize = 0,

    fn stopSlice(): []const ColorStop {
        return _.stops[0.._.stop_count];
    }
}

/// 锥形渐变结构体
pub struct ConicGradient {
    repeating: bool = false,
    from_angle_deg: f64 = 0.0,
    has_from_angle: bool = false,
    position: string = "",
    stops: [MAX_STOPS]ColorStop = [ColorStop{}; MAX_STOPS],
    stop_count: usize = 0,

    fn stopSlice(): []const ColorStop {
        return _.stops[0.._.stop_count];
    }
}

fn splitSpaceTokens(s: string, out: []string): usize {
    let count: usize = 0;
    let i: usize = 0;
    let depth: usize = 0;
    while (i < s.len) {
        while (i < s.len && isSpace(s[i])) { i += 1; }
        if (i >= s.len) { break; }
        const start = i;
        while (i < s.len) {
            if (s[i] == '(') {
                depth += 1;
            } else if (s[i] == ')' && depth > 0) {
                depth -= 1;
            } else if (depth == 0 && isSpace(s[i])) {
                break;
            }
            i += 1;
        }
        if (count >= out.len) {
            return out.len + 1;
        }
        out[count] = s[start..i];
        count += 1;
    }
    return count;
}

fn splitTopLevelComma(s: string, out: []string): usize? {
    let count: usize = 0;
    let start: usize = 0;
    let depth: usize = 0;
    let i: usize = 0;
    while (i <= s.len) {
        if (i < s.len) {
            if (s[i] == '(') {
                depth += 1;
            } else if (s[i] == ')') {
                if (depth == 0) { return null; }
                depth -= 1;
            }
        }
        if (i == s.len || (depth == 0 && s[i] == ',')) {
            if (i == s.len && depth != 0) { return null; }
            const seg = trimSpaces(s[start..i]);
            if (seg.len == 0) { return null; }
            if (count >= out.len) { return null; }
            out[count] = seg;
            count += 1;
            start = i + 1;
        }
        i += 1;
    }
    return count;
}

fn parseColorStop(raw: string): ColorStop? {
    let toks: [4]string = ["", "", "", ""];
    const n = splitSpaceTokens(trimSpaces(raw), toks[0..4]);
    if (n == 0 || n > 3) {
        return null;
    }
    return ColorStop{
        color: toks[0],
        pos1: if (n >= 2) toks[1] else "",
        pos2: if (n == 3) toks[2] else "",
    };
}

fn parseRadialPrelude(raw: string, out: *RadialGradient): bool {
    const s = trimSpaces(raw);
    if (s.len == 0) { return false; }

    let toks: [8]string = ["", "", "", "", "", "", "", ""];
    const n = splitSpaceTokens(s, toks[0..8]);
    if (n == 0 || n > 8) { return false; }

    let seen_prelude = false;
    let i: usize = 0;
    while (i < n) {
        if (eqlIgnoreCaseAscii(toks[i], "at")) {
            if (i + 1 >= n) { return false; }
            // 在原始串中定位 "at" 之后的完整 position 切片
            let p_idx: usize = 0;
            let found_at = false;
            while (p_idx + 2 <= s.len) {
                if ((p_idx == 0 || isSpace(s[p_idx - 1])) &&
                    eqlIgnoreCaseAscii(s[p_idx .. p_idx + 2], "at") &&
                    (p_idx + 2 < s.len && isSpace(s[p_idx + 2])))
                {
                    out.position = trimSpaces(s[p_idx + 2 ..]);
                    found_at = out.position.len > 0;
                    break;
                }
                p_idx += 1;
            }
            if (!found_at) { return false; }
            seen_prelude = true;
            break;
        }
        if (sh := RadialShape.fromStr(toks[i])) {
            out.shape = sh;
            seen_prelude = true;
        } else if (ext := RadialExtent.fromStr(toks[i])) {
            out.extent = ext;
            seen_prelude = true;
        } else {
            return false;
        }
        i += 1;
    }
    return seen_prelude;
}

fn parseConicPrelude(raw: string, out: *ConicGradient): bool {
    const s = trimSpaces(raw);
    if (s.len == 0) { return false; }

    let toks: [8]string = ["", "", "", "", "", "", "", ""];
    const n = splitSpaceTokens(s, toks[0..8]);
    if (n == 0 || n > 8) { return false; }

    let seen_prelude = false;
    let i: usize = 0;
    while (i < n) {
        if (eqlIgnoreCaseAscii(toks[i], "from")) {
            if (i + 1 >= n) { return false; }
            if (deg := parseAngleDeg(toks[i + 1])) {
                out.from_angle_deg = deg;
                out.has_from_angle = true;
                seen_prelude = true;
                i += 2;
                continue;
            }
            return false;
        }
        if (eqlIgnoreCaseAscii(toks[i], "at")) {
            if (i + 1 >= n) { return false; }
            let p_idx: usize = 0;
            let found_at = false;
            while (p_idx + 2 <= s.len) {
                if ((p_idx == 0 || isSpace(s[p_idx - 1])) &&
                    eqlIgnoreCaseAscii(s[p_idx .. p_idx + 2], "at") &&
                    (p_idx + 2 < s.len && isSpace(s[p_idx + 2])))
                {
                    out.position = trimSpaces(s[p_idx + 2 ..]);
                    found_at = out.position.len > 0;
                    break;
                }
                p_idx += 1;
            }
            if (!found_at) { return false; }
            seen_prelude = true;
            break;
        }
        return false;
    }
    return seen_prelude;
}

/// 统一 CSS Images 4 渐变包装（零堆分配）
pub struct Gradient {
    kind: GradientKind,
    linear: LinearGradient = LinearGradient{},
    radial: RadialGradient = RadialGradient{},
    conic: ConicGradient = ConicGradient{},

    /// 解析任意 CSS linear/radial/conic (及 repeating-*) 渐变字符串；非法输入返回 null
    static fn parse(input: []const u8): Gradient? {
        const s = trimSpaces(input);
        if (s.len < 4 || s[s.len - 1] != ')') {
            return null;
        }
        let open_paren: usize? = null;
        let idx: usize = 0;
        while (idx < s.len) {
            if (s[idx] == '(') {
                open_paren = idx;
                break;
            }
            idx += 1;
        }
        const lp = open_paren ?? return null;
        const fn_name = trimSpaces(s[0..lp]);
        const inner = trimSpaces(s[lp + 1 .. s.len - 1]);
        if (inner.len == 0) {
            return null;
        }

        let kind: GradientKind = .linear;
        if (eqlIgnoreCaseAscii(fn_name, "linear-gradient")) {
            kind = .linear;
        } else if (eqlIgnoreCaseAscii(fn_name, "repeating-linear-gradient")) {
            kind = .repeating_linear;
        } else if (eqlIgnoreCaseAscii(fn_name, "radial-gradient")) {
            kind = .radial;
        } else if (eqlIgnoreCaseAscii(fn_name, "repeating-radial-gradient")) {
            kind = .repeating_radial;
        } else if (eqlIgnoreCaseAscii(fn_name, "conic-gradient")) {
            kind = .conic;
        } else if (eqlIgnoreCaseAscii(fn_name, "repeating-conic-gradient")) {
            kind = .repeating_conic;
        } else {
            return null;
        }

        let parts: [17]string = [""; 17];
        const part_count = splitTopLevelComma(inner, parts[0..17]) ?? return null;
        if (part_count < 2) {
            return null;
        }

        let grad = Gradient{ kind: kind };
        let stop_start: usize = 0;

        if (kind == .linear || kind == .repeating_linear) {
            grad.linear.repeating = kind.isRepeating();
            if (dir := LineDirection.parse(parts[0])) {
                grad.linear.direction = dir;
                stop_start = 1;
            }
            if (part_count - stop_start < 2 || part_count - stop_start > MAX_STOPS) {
                return null;
            }
            let i = stop_start;
            while (i < part_count) {
                const st = parseColorStop(parts[i]) ?? return null;
                grad.linear.stops[grad.linear.stop_count] = st;
                grad.linear.stop_count += 1;
                i += 1;
            }
            return grad;
        }

        if (kind == .radial || kind == .repeating_radial) {
            grad.radial.repeating = kind.isRepeating();
            if (parseRadialPrelude(parts[0], *grad.radial)) {
                stop_start = 1;
            }
            if (part_count - stop_start < 2 || part_count - stop_start > MAX_STOPS) {
                return null;
            }
            let i = stop_start;
            while (i < part_count) {
                const st = parseColorStop(parts[i]) ?? return null;
                grad.radial.stops[grad.radial.stop_count] = st;
                grad.radial.stop_count += 1;
                i += 1;
            }
            return grad;
        }

        // conic / repeating_conic
        grad.conic.repeating = kind.isRepeating();
        if (parseConicPrelude(parts[0], *grad.conic)) {
            stop_start = 1;
        }
        if (part_count - stop_start < 2 || part_count - stop_start > MAX_STOPS) {
            return null;
        }
        let i = stop_start;
        while (i < part_count) {
            const st = parseColorStop(parts[i]) ?? return null;
            grad.conic.stops[grad.conic.stop_count] = st;
            grad.conic.stop_count += 1;
            i += 1;
        }
        return grad;
    }

    fn isRepeating(): bool {
        return _.kind.isRepeating();
    }

    fn stopCount(): usize {
        return switch (_.kind) {
            .linear | .repeating_linear => _.linear.stop_count,
            .radial | .repeating_radial => _.radial.stop_count,
            .conic | .repeating_conic => _.conic.stop_count,
        };
    }

    /// 规范化 CSS 序列化（自动省略默认 `to bottom` / `180deg` / `ellipse farthest-corner` / `from 0deg` / `at center`）
    fn toCss(out: []u8): []const u8 {
        let pos: usize = 0;
        if (!appendStr(out, *pos, _.kind.fnName())) { return out[0..0]; }
        if (!appendStr(out, *pos, "(")) { return out[0..0]; }

        let need_comma = false;

        if (_.kind == .linear || _.kind == .repeating_linear) {
            if (!_.linear.direction.isDefault()) {
                if (!_.linear.direction.writeTo(out, *pos)) { return out[0..0]; }
                need_comma = true;
            }
            let i: usize = 0;
            while (i < _.linear.stop_count) {
                if (need_comma || i > 0) {
                    if (!appendStr(out, *pos, ", ")) { return out[0..0]; }
                }
                if (!_.linear.stops[i].writeTo(out, *pos)) { return out[0..0]; }
                i += 1;
            }
        } else if (_.kind == .radial || _.kind == .repeating_radial) {
            let wrote_prelude = false;
            if (_.radial.shape != .ellipse) {
                if (!appendStr(out, *pos, _.radial.shape.asStr())) { return out[0..0]; }
                wrote_prelude = true;
            }
            if (_.radial.extent != .farthest_corner) {
                if (wrote_prelude) {
                    if (!appendStr(out, *pos, " ")) { return out[0..0]; }
                }
                if (!appendStr(out, *pos, _.radial.extent.asStr())) { return out[0..0]; }
                wrote_prelude = true;
            }
            if (_.radial.position.len > 0 && !eqlIgnoreCaseAscii(_.radial.position, "center")) {
                if (wrote_prelude) {
                    if (!appendStr(out, *pos, " ")) { return out[0..0]; }
                }
                if (!appendStr(out, *pos, "at ")) { return out[0..0]; }
                if (!appendStr(out, *pos, _.radial.position)) { return out[0..0]; }
                wrote_prelude = true;
            }
            need_comma = wrote_prelude;
            let i: usize = 0;
            while (i < _.radial.stop_count) {
                if (need_comma || i > 0) {
                    if (!appendStr(out, *pos, ", ")) { return out[0..0]; }
                }
                if (!_.radial.stops[i].writeTo(out, *pos)) { return out[0..0]; }
                i += 1;
            }
        } else {
            let wrote_prelude = false;
            if (_.conic.has_from_angle && @abs(_.conic.from_angle_deg) > 0.0001) {
                if (!appendStr(out, *pos, "from ")) { return out[0..0]; }
                if (!appendDeg(out, *pos, _.conic.from_angle_deg)) { return out[0..0]; }
                wrote_prelude = true;
            }
            if (_.conic.position.len > 0 && !eqlIgnoreCaseAscii(_.conic.position, "center")) {
                if (wrote_prelude) {
                    if (!appendStr(out, *pos, " ")) { return out[0..0]; }
                }
                if (!appendStr(out, *pos, "at ")) { return out[0..0]; }
                if (!appendStr(out, *pos, _.conic.position)) { return out[0..0]; }
                wrote_prelude = true;
            }
            need_comma = wrote_prelude;
            let i: usize = 0;
            while (i < _.conic.stop_count) {
                if (need_comma || i > 0) {
                    if (!appendStr(out, *pos, ", ")) { return out[0..0]; }
                }
                if (!_.conic.stops[i].writeTo(out, *pos)) { return out[0..0]; }
                i += 1;
            }
        }

        if (!appendStr(out, *pos, ")")) { return out[0..0]; }
        return out[0..pos];
    }
}

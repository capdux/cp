// Source: Absorbed from /data/dev/bun/src/interchange/yaml.cp | Synced: 2026-11-23T21:46:13Z

/// YAML 1.2 Core Schema 标量类型枚举
pub enum YamlScalarKind: u8 {
    null_val = 0,
    bool_val = 1,
    int_val = 2,
    float_val = 3,
    string_val = 4,

    fn asStr(): string {
        return switch (_) {
            .null_val => "null",
            .bool_val => "bool",
            .int_val => "int",
            .float_val => "float",
            .string_val => "string",
        };
    }
}

/// YAML 1.2 Core Schema 解析后的标量值
pub struct YamlScalar {
    kind: YamlScalarKind = .null_val,
    bool_v: bool = false,
    int_v: i64 = 0,
    float_v: f64 = 0.0,
    str_v: []const u8 = "",
}

type YamlScVec16 = @Vector(16, u8);

fn makeYamlScHexLut(): [i8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [i8; 256] = [-1; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(i8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(i8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(i8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

fn makeYamlScFirstLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    const chars = " \t\n\r-?:,[]{}#&*!|>'\"%@`";
    let i: usize = 0;
    while (i < chars.len) : (i += 1) {
        lut[@as(usize, chars[i])] = 1;
    }
    return lut;
}

const YAML_SC_HEX_LUT: [i8; 256] = makeYamlScHexLut();
const YAML_SC_FIRST_LUT: [u8; 256] = makeYamlScFirstLut();

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: YamlScVec16 = a[i .. i + 16][0..16].*;
        const vb: YamlScVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isOctDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

fn hexVal(c: u8): i32 {
    return @as(i32, YAML_SC_HEX_LUT[@as(usize, c)]);
}

fn trimHorizSpace(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && (s[start] == ' ' || s[start] == '\t')) { start += 1; }
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) { end -= 1; }
    return s[start..end];
}

fn pow10(exp: i32): f64 {
    if (exp == 0) { return 1.0; }
    let neg = false;
    let e = exp;
    if (e < 0) {
        neg = true;
        e = 0 - e;
    }
    let result: f64 = 1.0;
    let base: f64 = 10.0;
    while (e > 0) {
        if ((e & 1) != 0) {
            result *= base;
        }
        base *= base;
        e >>= 1;
    }
    if (neg) {
        return 1.0 / result;
    }
    return result;
}

fn tryParseCoreInt(s: []const u8): i64? {
    if (s.len == 0) { return null; }

    // 八进制 0o[0-7]+
    if (s.len >= 3 && s[0] == '0' && s[1] == 'o') {
        let acc: u64 = 0;
        let i: usize = 2;
        while (i < s.len) {
            if (!isOctDigit(s[i])) { return null; }
            const d: u64 = @as(u64, s[i] - '0');
            if (acc > ((@as(u64, 9223372036854775807) - d) / 8)) { return null; }
            acc = acc * 8 + d;
            i += 1;
        }
        return @as(i64, @intCast(acc));
    }

    // 十六进制 0x[0-9a-fA-F]+
    if (s.len >= 3 && s[0] == '0' && s[1] == 'x') {
        let acc: u64 = 0;
        let i: usize = 2;
        while (i < s.len) {
            const hv = hexVal(s[i]);
            if (hv < 0) { return null; }
            const d: u64 = @as(u64, @intCast(hv));
            if (acc > ((@as(u64, 9223372036854775807) - d) / 16)) { return null; }
            acc = acc * 16 + d;
            i += 1;
        }
        return @as(i64, @intCast(acc));
    }

    // 十进制 [-+]?[0-9]+
    let i: usize = 0;
    let is_neg = false;
    if (s[i] == '+' || s[i] == '-') {
        is_neg = (s[i] == '-');
        i += 1;
    }
    if (i >= s.len) { return null; }

    const limit: u64 = if (is_neg) @as(u64, 9223372036854775808) else @as(u64, 9223372036854775807);
    let acc: u64 = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) { return null; }
        const d: u64 = @as(u64, s[i] - '0');
        if (acc > ((limit - d) / 10)) { return null; }
        acc = acc * 10 + d;
        i += 1;
    }

    if (is_neg) {
        if (acc == @as(u64, 9223372036854775808)) {
            return -9223372036854775807 - 1;
        }
        return 0 - @as(i64, @intCast(acc));
    }
    return @as(i64, @intCast(acc));
}

fn tryParseCoreFloat(s: []const u8): f64? {
    if (s.len == 0) { return null; }

    // .nan / .NaN / .NAN
    if (eqlBytes(s, ".nan") || eqlBytes(s, ".NaN") || eqlBytes(s, ".NAN")) {
        return std.math.nan(f64);
    }

    let i: usize = 0;
    let is_neg = false;
    if (s[i] == '+' || s[i] == '-') {
        is_neg = (s[i] == '-');
        i += 1;
    }
    const rest = s[i..s.len];
    if (eqlBytes(rest, ".inf") || eqlBytes(rest, ".Inf") || eqlBytes(rest, ".INF")) {
        return if (is_neg) (0.0 - std.math.inf(f64)) else std.math.inf(f64);
    }

    // 常规浮点：[-+]?(\.[0-9]+|[0-9]+(\.[0-9]*)?)([eE][-+]?[0-9]+)?
    let int_digits: usize = 0;
    let int_val: f64 = 0.0;
    while (i < s.len && isDigit(s[i])) {
        int_val = int_val * 10.0 + @as(f64, @floatFromInt(s[i] - '0'));
        int_digits += 1;
        i += 1;
    }

    let has_dot = false;
    let frac_digits: usize = 0;
    let frac_val: f64 = 0.0;
    let frac_scale: f64 = 0.1;
    if (i < s.len && s[i] == '.') {
        has_dot = true;
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_val += @as(f64, @floatFromInt(s[i] - '0')) * frac_scale;
            frac_scale *= 0.1;
            frac_digits += 1;
            i += 1;
        }
    }

    if (int_digits == 0 && frac_digits == 0) {
        return null;
    }

    let has_exp = false;
    let exp_val: i32 = 0;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        has_exp = true;
        i += 1;
        let exp_neg = false;
        if (i < s.len && (s[i] == '+' || s[i] == '-')) {
            exp_neg = (s[i] == '-');
            i += 1;
        }
        let exp_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + @as(i32, s[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0) { return null; }
        if (exp_neg) { exp_val = 0 - exp_val; }
    }

    if (i != s.len || (!has_dot && !has_exp)) {
        return null;
    }

    let total = (int_val + frac_val) * pow10(exp_val);
    if (is_neg) { total = 0.0 - total; }
    return total;
}

/// 严格遵循 YAML 1.2 Core Schema 解析纯标量并推断类型
pub fn parseCoreScalar(raw: []const u8): YamlScalar {
    const s = trimHorizSpace(raw);
    if (s.len == 0 || eqlBytes(s, "~") || eqlBytes(s, "null") || eqlBytes(s, "Null") || eqlBytes(s, "NULL")) {
        return YamlScalar{ kind: .null_val };
    }
    if (eqlBytes(s, "true") || eqlBytes(s, "True") || eqlBytes(s, "TRUE")) {
        return YamlScalar{ kind: .bool_val, bool_v: true };
    }
    if (eqlBytes(s, "false") || eqlBytes(s, "False") || eqlBytes(s, "FALSE")) {
        return YamlScalar{ kind: .bool_val, bool_v: false };
    }
    if (iv := tryParseCoreInt(s)) {
        return YamlScalar{ kind: .int_val, int_v: iv };
    }
    if (fv := tryParseCoreFloat(s)) {
        return YamlScalar{ kind: .float_val, float_v: fv };
    }
    return YamlScalar{ kind: .string_val, str_v: raw };
}

struct OutWriter {
    buf: []u8,
    pos: usize = 0,
    overflow: bool = false,

    fn writeByte(*_, c: u8) {
        if (_.pos < _.buf.len) {
            _.buf[_.pos] = c;
            _.pos += 1;
        } else {
            _.overflow = true;
        }
    }

    fn writeStr(*_, s: []const u8) {
        if (s.len == 0) return;
        if (_.pos + s.len <= _.buf.len) {
            @memcpy(_.buf[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
        } else {
            const avail = _.buf.len - _.pos;
            if (avail > 0) {
                @memcpy(_.buf[_.pos .. _.pos + avail], s[0..avail]);
                _.pos += avail;
            }
            _.overflow = true;
        }
    }

    fn finish(): ([]const u8)? {
        if (_.overflow) { return null; }
        return _.buf[0.._.pos];
    }
}

/// 解转义 YAML 双引号字符串（支持 `\n`, `\r`, `\t`, `\\`, `\"`, `\/`, `\0`, `\xHH` 等）
/// 自动兼容含外层 `"` 或纯内部内容；未闭合或非法转义返回 `null`
pub fn unescapeDoubleQuoted(raw: []const u8, out: []u8): ([]const u8)? {
    let body = raw;
    if (raw.len >= 1 && raw[0] == '"') {
        if (raw.len < 2 || raw[raw.len - 1] != '"') {
            return null;
        }
        body = raw[1..(raw.len - 1)];
    }

    let w = OutWriter{ buf: out };
    let i: usize = 0;
    const v_bs: YamlScVec16 = @splat('\\' as u8);
    while (i < body.len) {
        const run_start = i;
        while (i + 16 <= body.len) {
            const v: YamlScVec16 = body[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == v_bs)) break;
            i += 16;
        }
        if (i > run_start) {
            w.writeStr(body[run_start..i]);
            if (i >= body.len) break;
        }
        const c = body[i];
        if (c == '\\') {
            if (i + 1 >= body.len) { return null; }
            const esc = body[i + 1];
            switch (esc) {
                'n' => { w.writeByte('\n'); i += 2; },
                'r' => { w.writeByte('\r'); i += 2; },
                't' => { w.writeByte('\t'); i += 2; },
                '\\' => { w.writeByte('\\'); i += 2; },
                '"' => { w.writeByte('"'); i += 2; },
                '/' => { w.writeByte('/'); i += 2; },
                '0' => { w.writeByte(0); i += 2; },
                'a' => { w.writeByte(0x07); i += 2; },
                'b' => { w.writeByte(0x08); i += 2; },
                'v' => { w.writeByte(0x0B); i += 2; },
                'f' => { w.writeByte(0x0C); i += 2; },
                'e' => { w.writeByte(0x1B); i += 2; },
                ' ' => { w.writeByte(' '); i += 2; },
                'x' => {
                    if (i + 3 >= body.len) { return null; }
                    const hi = hexVal(body[i + 2]);
                    const lo = hexVal(body[i + 3]);
                    if (hi < 0 || lo < 0) { return null; }
                    const byte_val: u8 = @intCast((hi << 4) | lo);
                    w.writeByte(byte_val);
                    i += 4;
                },
                else => return null,
            }
        } else {
            w.writeByte(c);
            i += 1;
        }
    }
    return w.finish();
}

/// 解转义 YAML 单引号字符串（`''` -> `'`），兼容含外层 `'` 或纯内部内容
pub fn unescapeSingleQuoted(raw: []const u8, out: []u8): ([]const u8)? {
    let wrapped = false;
    let body = raw;
    if (raw.len >= 1 && raw[0] == '\'') {
        if (raw.len < 2 || raw[raw.len - 1] != '\'') {
            return null;
        }
        wrapped = true;
        body = raw[1..(raw.len - 1)];
    }

    let w = OutWriter{ buf: out };
    let i: usize = 0;
    const v_sq: YamlScVec16 = @splat('\'' as u8);
    while (i < body.len) {
        const run_start = i;
        while (i + 16 <= body.len) {
            const v: YamlScVec16 = body[i .. i + 16][0..16].*;
            if (@reduce(.Or, v == v_sq)) break;
            i += 16;
        }
        if (i > run_start) {
            w.writeStr(body[run_start..i]);
            if (i >= body.len) break;
        }
        if (body[i] == '\'') {
            if (i + 1 < body.len && body[i + 1] == '\'') {
                w.writeByte('\'');
                i += 2;
            } else if (wrapped) {
                return null;
            } else {
                w.writeByte('\'');
                i += 1;
            }
        } else {
            w.writeByte(body[i]);
            i += 1;
        }
    }
    return w.finish();
}

/// 判断字符串作为 YAML 纯标量输出时是否必须加引号（避免与关键字、数字或 YAML 语法指示符冲突）
pub fn needsQuoting(s: []const u8): bool {
    if (s.len == 0) { return true; }

    const first = s[0];
    const last = s[s.len - 1];
    if (YAML_SC_FIRST_LUT[@as(usize, first)] != 0 ||
        last == ' ' || last == '\t' || last == '\n' || last == '\r' || last == ':')
    {
        return true;
    }

    // 控制字符、映射分隔符 ": " 与注释前缀 " #" / "\t#"
    let i: usize = 0;
    const v_sp: YamlScVec16 = @splat(0x20 as u8);
    const v_del: YamlScVec16 = @splat(0x7F as u8);
    const v_cl: YamlScVec16 = @splat(':' as u8);
    const v_hs: YamlScVec16 = @splat('#' as u8);
    while (i + 16 <= s.len) {
        const v: YamlScVec16 = s[i .. i + 16][0..16].*;
        const hit = (v < v_sp) | (v == v_del) | (v == v_cl) | (v == v_hs);
        if (!@reduce(.Or, hit)) {
            i += 16;
            continue;
        }
        const chunk_end = i + 16;
        while (i < chunk_end) : (i += 1) {
            const c = s[i];
            if (c < 0x20 || c == 0x7F) return true;
            if (c == ':' && i + 1 < s.len && (s[i + 1] == ' ' || s[i + 1] == '\t')) return true;
            if (c == '#' && i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t')) return true;
        }
    }
    while (i < s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F) {
            return true;
        }
        if (c == ':' && i + 1 < s.len && (s[i + 1] == ' ' || s[i + 1] == '\t')) {
            return true;
        }
        if (c == '#' && i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t')) {
            return true;
        }
        i += 1;
    }

    // 若会被 YAML 1.2 Core Schema 误解析为非字符串（null/bool/int/float），必须加引号
    return parseCoreScalar(s).kind != .string_val;
}

/// 安全格式化 YAML 标量：无需引号时原样输出，否则自动双引号包裹并转义特殊字符
pub fn formatSafeScalar(s: []const u8, out: []u8): ([]const u8)? {
    let w = OutWriter{ buf: out };
    if (!needsQuoting(s)) {
        w.writeStr(s);
        return w.finish();
    }

    const hex = "0123456789abcdef";
    w.writeByte('"');
    let i: usize = 0;
    const v_sp: YamlScVec16 = @splat(0x20 as u8);
    const v_del: YamlScVec16 = @splat(0x7F as u8);
    const v_qt: YamlScVec16 = @splat('"' as u8);
    const v_bs: YamlScVec16 = @splat('\\' as u8);
    while (i < s.len) {
        const run_start = i;
        while (i + 16 <= s.len) {
            const v: YamlScVec16 = s[i .. i + 16][0..16].*;
            const hit = (v < v_sp) | (v == v_del) | (v == v_qt) | (v == v_bs);
            if (@reduce(.Or, hit)) break;
            i += 16;
        }
        if (i > run_start) {
            w.writeStr(s[run_start..i]);
            if (i >= s.len) break;
        }
        const c = s[i];
        switch (c) {
            '"' => w.writeStr("\\\""),
            '\\' => w.writeStr("\\\\"),
            '\n' => w.writeStr("\\n"),
            '\r' => w.writeStr("\\r"),
            '\t' => w.writeStr("\\t"),
            0 => w.writeStr("\\0"),
            else => {
                if (c < 0x20 || c == 0x7F) {
                    w.writeStr("\\x");
                    w.writeByte(hex[(c >> 4) & 0x0F]);
                    w.writeByte(hex[c & 0x0F]);
                } else {
                    w.writeByte(c);
                }
            },
        }
        i += 1;
    }
    w.writeByte('"');
    return w.finish();
}

// Source: Absorbed from /data/dev/bun/src/interchange/json5.cp | Synced: 2026-11-22T07:47:14Z

/// JSON5 词法 Token 类型枚举
pub enum Json5TokenKind: u8 {
    eof = 0,
    lbrace = 1,
    rbrace = 2,
    lbracket = 3,
    rbracket = 4,
    colon = 5,
    comma = 6,
    string = 7,
    number = 8,
    boolean = 9,
    null_lit = 10,
    identifier = 11,
    invalid = 12,

    fn asStr(): string {
        return switch (_) {
            .eof => "eof",
            .lbrace => "{",
            .rbrace => "}",
            .lbracket => "[",
            .rbracket => "]",
            .colon => ":",
            .comma => ",",
            .string => "string",
            .number => "number",
            .boolean => "boolean",
            .null_lit => "null",
            .identifier => "identifier",
            .invalid => "invalid",
        };
    }
}

/// JSON5 词法 Token（零拷贝切片 + 数值/布尔解析值）
pub struct Json5Token {
    kind: Json5TokenKind = .eof,
    raw: []const u8 = "",
    start: usize = 0,
    end: usize = 0,
    num_val: f64 = 0.0,
    bool_val: bool = false,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C;
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn hexVal(c: u8): i32 {
    if (c >= '0' && c <= '9') { return @as(i32, c - '0'); }
    if (c >= 'a' && c <= 'f') { return @as(i32, c - 'a') + 10; }
    if (c >= 'A' && c <= 'F') { return @as(i32, c - 'A') + 10; }
    return -1;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == '$' || c >= 0x80;
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || isDigit(c);
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
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

/// 零堆分配 JSON5 词法扫描器
pub struct Json5Lexer {
    src: []const u8,
    pos: usize = 0,

    static fn init(src: []const u8): Json5Lexer {
        let start_pos: usize = 0;
        // 跳过可选 UTF-8 BOM
        if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
            start_pos = 3;
        }
        return Json5Lexer{ src: src, pos: start_pos };
    }

    fn skipTrivia(*_): bool {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            // UTF-8 NBSP (U+00A0)
            if (c == 0xC2 && _.pos + 1 < _.src.len && _.src[_.pos + 1] == 0xA0) {
                _.pos += 2;
                continue;
            }
            // 注释
            if (c == '/' && _.pos + 1 < _.src.len) {
                const n = _.src[_.pos + 1];
                if (n == '/') {
                    _.pos += 2;
                    while (_.pos < _.src.len && _.src[_.pos] != '\n' && _.src[_.pos] != '\r') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (n == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < _.src.len) {
                        if (_.src[_.pos] == '*' && _.src[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) {
                        _.pos = _.src.len;
                        return false;
                    }
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn scanString(*_, start: usize): Json5Token {
        const quote = _.src[start];
        _.pos = start + 1;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '\\') {
                if (_.pos + 1 >= _.src.len) {
                    _.pos = _.src.len;
                    return Json5Token{ kind: .invalid, raw: _.src[start.._.pos], start: start, end: _.pos };
                }
                const esc = _.src[_.pos + 1];
                if (esc == '\r' && _.pos + 2 < _.src.len && _.src[_.pos + 2] == '\n') {
                    _.pos += 3;
                } else {
                    _.pos += 2;
                }
                continue;
            }
            if (c == '\n' || c == '\r') {
                return Json5Token{ kind: .invalid, raw: _.src[start.._.pos], start: start, end: _.pos };
            }
            if (c == quote) {
                _.pos += 1;
                return Json5Token{
                    kind: Json5TokenKind.string,
                    raw: _.src[start.._.pos],
                    start: start,
                    end: _.pos,
                };
            }
            _.pos += 1;
        }
        return Json5Token{ kind: .invalid, raw: _.src[start.._.pos], start: start, end: _.pos };
    }

    fn scanNumber(*_, start: usize): Json5Token {
        let i = start;
        let is_neg = false;
        if (i < _.src.len && (_.src[i] == '+' || _.src[i] == '-')) {
            is_neg = (_.src[i] == '-');
            i += 1;
        }

        // +Infinity / -Infinity / +NaN / -NaN
        if (i + 8 <= _.src.len && eqlBytes(_.src[i..(i + 8)], "Infinity")) {
            const end_pos = i + 8;
            if (end_pos == _.src.len || !isIdentContinue(_.src[end_pos])) {
                _.pos = end_pos;
                const inf_v: f64 = if (is_neg) (0.0 - std.math.inf(f64)) else std.math.inf(f64);
                return Json5Token{
                    kind: .number,
                    raw: _.src[start..end_pos],
                    start: start,
                    end: end_pos,
                    num_val: inf_v,
                };
            }
        }
        if (i + 3 <= _.src.len && eqlBytes(_.src[i..(i + 3)], "NaN")) {
            const end_pos = i + 3;
            if (end_pos == _.src.len || !isIdentContinue(_.src[end_pos])) {
                _.pos = end_pos;
                return Json5Token{
                    kind: .number,
                    raw: _.src[start..end_pos],
                    start: start,
                    end: end_pos,
                    num_val: std.math.nan(f64),
                };
            }
        }

        // 十六进制 0x... / 0X...
        if (i + 1 < _.src.len && _.src[i] == '0' && (_.src[i + 1] == 'x' || _.src[i + 1] == 'X')) {
            i += 2;
            let val: f64 = 0.0;
            let digits: usize = 0;
            while (i < _.src.len) {
                const hv = hexVal(_.src[i]);
                if (hv < 0) { break; }
                val = val * 16.0 + @as(f64, @floatFromInt(hv));
                digits += 1;
                i += 1;
            }
            _.pos = i;
            if (digits == 0) {
                return Json5Token{ kind: .invalid, raw: _.src[start..i], start: start, end: i };
            }
            if (is_neg) { val = 0.0 - val; }
            return Json5Token{
                kind: .number,
                raw: _.src[start..i],
                start: start,
                end: i,
                num_val: val,
            };
        }

        // 十进制（支持 .5、5.、1e-3、+42）
        let int_digits: usize = 0;
        let int_val: f64 = 0.0;
        if (i < _.src.len && _.src[i] == '0') {
            int_digits = 1;
            i += 1;
            if (i < _.src.len && isDigit(_.src[i])) {
                // 禁止前导零（如 01）
                while (i < _.src.len && isDigit(_.src[i])) { i += 1; }
                _.pos = i;
                return Json5Token{ kind: .invalid, raw: _.src[start..i], start: start, end: i };
            }
        } else {
            while (i < _.src.len && isDigit(_.src[i])) {
                int_val = int_val * 10.0 + @as(f64, @floatFromInt(_.src[i] - '0'));
                int_digits += 1;
                i += 1;
            }
        }

        let frac_digits: usize = 0;
        let frac_val: f64 = 0.0;
        let frac_scale: f64 = 0.1;
        if (i < _.src.len && _.src[i] == '.') {
            i += 1;
            while (i < _.src.len && isDigit(_.src[i])) {
                frac_val += @as(f64, @floatFromInt(_.src[i] - '0')) * frac_scale;
                frac_scale *= 0.1;
                frac_digits += 1;
                i += 1;
            }
        }

        if (int_digits == 0 && frac_digits == 0) {
            _.pos = if (i > start) i else start + 1;
            return Json5Token{ kind: .invalid, raw: _.src[start.._.pos], start: start, end: _.pos };
        }

        let exp_val: i32 = 0;
        if (i < _.src.len && (_.src[i] == 'e' || _.src[i] == 'E')) {
            i += 1;
            let exp_neg = false;
            if (i < _.src.len && (_.src[i] == '+' || _.src[i] == '-')) {
                exp_neg = (_.src[i] == '-');
                i += 1;
            }
            let exp_digits: usize = 0;
            while (i < _.src.len && isDigit(_.src[i])) {
                if (exp_val < 1000) {
                    exp_val = exp_val * 10 + @as(i32, _.src[i] - '0');
                }
                exp_digits += 1;
                i += 1;
            }
            if (exp_digits == 0) {
                _.pos = i;
                return Json5Token{ kind: .invalid, raw: _.src[start..i], start: start, end: i };
            }
            if (exp_neg) {
                exp_val = 0 - exp_val;
            }
        }

        let total = (int_val + frac_val) * pow10(exp_val);
        if (is_neg) {
            total = 0.0 - total;
        }
        _.pos = i;
        return Json5Token{
            kind: .number,
            raw: _.src[start..i],
            start: start,
            end: i,
            num_val: total,
        };
    }

    fn scanIdent(*_, start: usize): Json5Token {
        let i = start;
        while (i < _.src.len && isIdentContinue(_.src[i])) {
            i += 1;
        }
        _.pos = i;
        const word = _.src[start..i];
        if (eqlBytes(word, "true")) {
            return Json5Token{ kind: .boolean, raw: word, start: start, end: i, bool_val: true };
        }
        if (eqlBytes(word, "false")) {
            return Json5Token{ kind: .boolean, raw: word, start: start, end: i, bool_val: false };
        }
        if (eqlBytes(word, "null")) {
            return Json5Token{ kind: .null_lit, raw: word, start: start, end: i };
        }
        if (eqlBytes(word, "Infinity")) {
            return Json5Token{ kind: .number, raw: word, start: start, end: i, num_val: std.math.inf(f64) };
        }
        if (eqlBytes(word, "NaN")) {
            return Json5Token{ kind: .number, raw: word, start: start, end: i, num_val: std.math.nan(f64) };
        }
        return Json5Token{ kind: .identifier, raw: word, start: start, end: i };
    }

    /// 读取下一个 JSON5 Token
    fn next(*_): Json5Token {
        const trivia_start = _.pos;
        if (!_.skipTrivia()) {
            return Json5Token{
                kind: .invalid,
                raw: _.src[trivia_start.._.pos],
                start: trivia_start,
                end: _.pos,
            };
        }
        if (_.pos >= _.src.len) {
            return Json5Token{ kind: .eof, raw: "", start: _.pos, end: _.pos };
        }

        const start = _.pos;
        const c = _.src[start];

        if (c == '{') {
            _.pos += 1;
            return Json5Token{ kind: .lbrace, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == '}') {
            _.pos += 1;
            return Json5Token{ kind: .rbrace, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == '[') {
            _.pos += 1;
            return Json5Token{ kind: .lbracket, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == ']') {
            _.pos += 1;
            return Json5Token{ kind: .rbracket, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == ':') {
            _.pos += 1;
            return Json5Token{ kind: .colon, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == ',') {
            _.pos += 1;
            return Json5Token{ kind: .comma, raw: _.src[start.._.pos], start: start, end: _.pos };
        }
        if (c == '"' || c == '\'') {
            return _.scanString(start);
        }
        if (isDigit(c) || c == '.' || c == '+' || c == '-') {
            return _.scanNumber(start);
        }
        if (isIdentStart(c)) {
            return _.scanIdent(start);
        }

        _.pos += 1;
        return Json5Token{ kind: .invalid, raw: _.src[start.._.pos], start: start, end: _.pos };
    }
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
        let i: usize = 0;
        while (i < s.len) {
            _.writeByte(s[i]);
            i += 1;
        }
    }

    fn writeHex4(*_, v: u8) {
        const hex = "0123456789abcdef";
        _.writeStr("\\u00");
        _.writeByte(hex[(v >> 4) & 0x0F]);
        _.writeByte(hex[v & 0x0F]);
    }

    fn writeU64(*_, val: u64) {
        let digits: [24]u8 = [0; 24];
        let dlen: usize = 0;
        let tmp = val;
        if (tmp == 0) {
            _.writeByte('0');
            return;
        }
        while (tmp > 0 && dlen < 24) {
            digits[dlen] = @intCast((tmp % 10) + '0');
            tmp /= 10;
            dlen += 1;
        }
        let k = dlen;
        while (k > 0) {
            k -= 1;
            _.writeByte(digits[k]);
        }
    }
}

fn writeNormalizedString(w: *OutWriter, raw: []const u8): bool {
    if (raw.len < 2) { return false; }
    w.writeByte('"');
    let i: usize = 1;
    const end_idx = raw.len - 1;
    while (i < end_idx) {
        const c = raw[i];
        if (c == '\\') {
            if (i + 1 >= end_idx) { return false; }
            const esc = raw[i + 1];
            if (esc == '\n') {
                i += 2;
                continue;
            }
            if (esc == '\r') {
                if (i + 2 < end_idx && raw[i + 2] == '\n') {
                    i += 3;
                } else {
                    i += 2;
                }
                continue;
            }
            if (esc == '\'') {
                w.writeByte('\'');
                i += 2;
                continue;
            }
            if (esc == 'v') {
                w.writeStr("\\u000b");
                i += 2;
                continue;
            }
            if (esc == '0') {
                w.writeStr("\\u0000");
                i += 2;
                continue;
            }
            if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't' || esc == 'u') {
                w.writeByte('\\');
                w.writeByte(esc);
                i += 2;
                continue;
            }
            // 其他 ES5 非特殊转义字符（如 \a -> a）
            w.writeByte(esc);
            i += 2;
            continue;
        }
        if (c == '"') {
            w.writeStr("\\\"");
        } else if (c < 0x20) {
            w.writeHex4(c);
        } else {
            w.writeByte(c);
        }
        i += 1;
    }
    w.writeByte('"');
    return !w.overflow;
}

fn writeNormalizedNumber(w: *OutWriter, tok: Json5Token): bool {
    // 标准 RFC 8259 JSON 不支持 Infinity / NaN
    if (!std.math.isFinite(tok.num_val)) {
        return false;
    }

    const raw = tok.raw;
    if (raw.len == 0) { return false; }

    let i: usize = 0;
    let is_neg = false;
    if (raw[i] == '+' || raw[i] == '-') {
        is_neg = (raw[i] == '-');
        i += 1;
    }

    // 十六进制转为十进制整数
    if (i + 1 < raw.len && raw[i] == '0' && (raw[i + 1] == 'x' || raw[i + 1] == 'X')) {
        i += 2;
        let uval: u64 = 0;
        while (i < raw.len) {
            const hv = hexVal(raw[i]);
            if (hv < 0) { return false; }
            uval = (uval * 16) + @as(u64, @intCast(hv));
            i += 1;
        }
        if (is_neg && uval > 0) {
            w.writeByte('-');
        }
        w.writeU64(uval);
        return !w.overflow;
    }

    // 十进制规范化：去除前导 +、补全 .5 -> 0.5、去除 5. 尾随小数点
    if (is_neg) {
        w.writeByte('-');
    }
    if (i < raw.len && raw[i] == '.') {
        w.writeByte('0');
    }
    while (i < raw.len) {
        if (raw[i] == '.') {
            if (i + 1 == raw.len || !isDigit(raw[i + 1])) {
                // 5. 或 5.e2 -> 省略裸小数点
                i += 1;
                continue;
            }
        }
        w.writeByte(raw[i]);
        i += 1;
    }
    return !w.overflow;
}

struct Json5Converter {
    lexer: Json5Lexer,
    cur: Json5Token = Json5Token{},
    w: OutWriter,

    fn advance(*_) {
        _.cur = _.lexer.next();
    }

    fn emitValue(*_, depth: usize): bool {
        if (depth > 64 || _.w.overflow) {
            return false;
        }
        switch (_.cur.kind) {
            .lbrace => return _.emitObject(depth + 1),
            .lbracket => return _.emitArray(depth + 1),
            Json5TokenKind.string => {
                if (!writeNormalizedString(*_.w, _.cur.raw)) { return false; }
                _.advance();
                return true;
            },
            .number => {
                if (!writeNormalizedNumber(*_.w, _.cur)) { return false; }
                _.advance();
                return true;
            },
            .boolean => {
                _.w.writeStr(if (_.cur.bool_val) "true" else "false");
                _.advance();
                return !_.w.overflow;
            },
            .null_lit => {
                _.w.writeStr("null");
                _.advance();
                return !_.w.overflow;
            },
            else => return false,
        }
    }

    fn emitObject(*_, depth: usize): bool {
        _.w.writeByte('{');
        _.advance(); // consume '{'
        let first = true;
        while (_.cur.kind != .rbrace) {
            if (_.cur.kind == .eof || _.cur.kind == .invalid) {
                return false;
            }
            if (!first) {
                _.w.writeByte(',');
            }
            first = false;

            if (_.cur.kind == Json5TokenKind.string) {
                if (!writeNormalizedString(*_.w, _.cur.raw)) { return false; }
            } else if (_.cur.kind == .identifier || _.cur.kind == .boolean || _.cur.kind == .null_lit) {
                _.w.writeByte('"');
                _.w.writeStr(_.cur.raw);
                _.w.writeByte('"');
            } else {
                return false;
            }
            _.advance();

            if (_.cur.kind != .colon) {
                return false;
            }
            _.w.writeByte(':');
            _.advance();

            if (!_.emitValue(depth)) {
                return false;
            }

            if (_.cur.kind == .comma) {
                _.advance();
                // 允许尾随逗号
                if (_.cur.kind == .rbrace) {
                    break;
                }
            } else if (_.cur.kind != .rbrace) {
                return false;
            }
        }
        _.w.writeByte('}');
        _.advance(); // consume '}'
        return !_.w.overflow;
    }

    fn emitArray(*_, depth: usize): bool {
        _.w.writeByte('[');
        _.advance(); // consume '['
        let first = true;
        while (_.cur.kind != .rbracket) {
            if (_.cur.kind == .eof || _.cur.kind == .invalid) {
                return false;
            }
            if (!first) {
                _.w.writeByte(',');
            }
            first = false;

            if (!_.emitValue(depth)) {
                return false;
            }

            if (_.cur.kind == .comma) {
                _.advance();
                // 允许尾随逗号
                if (_.cur.kind == .rbracket) {
                    break;
                }
            } else if (_.cur.kind != .rbracket) {
                return false;
            }
        }
        _.w.writeByte(']');
        _.advance(); // consume ']'
        return !_.w.overflow;
    }
}

/// 将合法 JSON5 文本零堆分配转译为标准 RFC 8259 JSON 输出至 `out`
/// 去除注释与尾随逗号、单引号与无引号键转为双引号、十六进制与 `.5`/`+3` 规范化为标准 JSON 数字
pub fn toStandardJson(src: []const u8, out: []u8): ([]const u8)? {
    let conv = Json5Converter{
        lexer: Json5Lexer.init(src),
        w: OutWriter{ buf: out },
    };
    conv.advance();
    if (!conv.emitValue(0)) {
        return null;
    }
    if (conv.cur.kind != .eof || conv.w.overflow) {
        return null;
    }
    return conv.w.buf[0..conv.w.pos];
}

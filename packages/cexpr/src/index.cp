// @cap/cexpr — Zero-allocation C constant expression parser & evaluator
// Inspired by Rust `cexpr`
// MIT License

import { isSpace, isDigit, hexVal } from "cap:ascii";

pub struct EvalResult {
    pub value: i64,
    pub ok: bool,
}

struct Parser {
    pos: usize,
    ok: bool,
    depth: usize,
}

fn skipWs(p: *Parser, src: string) {
    while (p.*.pos < src.len && isSpace(src[p.*.pos])) {
        p.*.pos = p.*.pos + 1;
    }
}

fn peek(p: *Parser, src: string): u8 {
    if (p.*.pos >= src.len) {
        return 0;
    }
    return src[p.*.pos];
}

fn peek2(p: *Parser, src: string): u8 {
    if (p.*.pos + 1 >= src.len) {
        return 0;
    }
    return src[p.*.pos + 1];
}

fn absI64(x: i64): u64 {
    if (x >= 0) {
        return x as u64;
    }
    return ((0 - (x + 1)) as u64) + 1;
}

fn signedDivI64(a: i64, b: i64): i64 {
    let neg: bool = (a < 0) != (b < 0);
    let q: u64 = absI64(a) / absI64(b);
    if (neg) {
        return 0 - (q as i64);
    }
    return q as i64;
}

fn signedRemI64(a: i64, b: i64): i64 {
    let r: u64 = absI64(a) % absI64(b);
    if (a < 0) {
        return 0 - (r as i64);
    }
    return r as i64;
}

fn skipIntSuffix(p: *Parser, src: string) {
    let count: usize = 0;
    while (p.*.pos < src.len && count < 3) {
        let ch: u8 = src[p.*.pos];
        if (ch == 117 || ch == 85 || ch == 108 || ch == 76) {
            p.*.pos = p.*.pos + 1;
            count = count + 1;
        } else {
            break;
        }
    }
}

fn parseCharLit(p: *Parser, src: string): i64 {
    // Consume opening '\''
    p.*.pos = p.*.pos + 1;
    if (p.*.pos >= src.len) {
        p.*.ok = false;
        return 0;
    }
    let val: u8 = 0;
    if (src[p.*.pos] == 92) { // '\\'
        p.*.pos = p.*.pos + 1;
        if (p.*.pos >= src.len) {
            p.*.ok = false;
            return 0;
        }
        let esc: u8 = src[p.*.pos];
        p.*.pos = p.*.pos + 1;
        if (esc == 110) { val = 10; }       // \n
        else if (esc == 116) { val = 9; }   // \t
        else if (esc == 114) { val = 13; }  // \r
        else if (esc == 48) { val = 0; }    // \0
        else if (esc == 92) { val = 92; }   // \\
        else if (esc == 39) { val = 39; }   // \'
        else if (esc == 34) { val = 34; }   // \"
        else { val = esc; }
    } else {
        if (src[p.*.pos] == 39) {
            p.*.ok = false;
            return 0;
        }
        val = src[p.*.pos];
        p.*.pos = p.*.pos + 1;
    }
    if (p.*.pos >= src.len || src[p.*.pos] != 39) {
        p.*.ok = false;
        return 0;
    }
    p.*.pos = p.*.pos + 1;
    return val as i64;
}

fn parseIntLit(p: *Parser, src: string): i64 {
    let ch0: u8 = src[p.*.pos];
    let acc: u64 = 0;

    if (ch0 == 48 && p.*.pos + 1 < src.len) {
        let ch1: u8 = src[p.*.pos + 1];
        if (ch1 == 120 || ch1 == 88) {
            // Hex: 0x...
            p.*.pos = p.*.pos + 2;
            let digits: usize = 0;
            while (p.*.pos < src.len) {
                if (dv := hexVal(src[p.*.pos])) {
                    acc = (acc << 4) | (dv as u64);
                    p.*.pos = p.*.pos + 1;
                    digits = digits + 1;
                } else {
                    break;
                }
            }
            if (digits == 0) {
                p.*.ok = false;
                return 0;
            }
            skipIntSuffix(p, src);
            return acc as i64;
        }
        if (ch1 == 98 || ch1 == 66) {
            // Binary: 0b...
            p.*.pos = p.*.pos + 2;
            let digits: usize = 0;
            while (p.*.pos < src.len) {
                let c: u8 = src[p.*.pos];
                if (c != 48 && c != 49) {
                    break;
                }
                acc = (acc << 1) | ((c - 48) as u64);
                p.*.pos = p.*.pos + 1;
                digits = digits + 1;
            }
            if (digits == 0) {
                p.*.ok = false;
                return 0;
            }
            skipIntSuffix(p, src);
            return acc as i64;
        }
        if (isDigit(ch1)) {
            // Octal: 0...
            p.*.pos = p.*.pos + 1;
            while (p.*.pos < src.len) {
                let c: u8 = src[p.*.pos];
                if (c >= 48 && c <= 55) {
                    acc = (acc << 3) | ((c - 48) as u64);
                    p.*.pos = p.*.pos + 1;
                } else if (c == 56 || c == 57) {
                    // Invalid octal digit '8' or '9'
                    p.*.ok = false;
                    return 0;
                } else {
                    break;
                }
            }
            skipIntSuffix(p, src);
            return acc as i64;
        }
    }

    // Decimal
    let digits: usize = 0;
    while (p.*.pos < src.len) {
        let c: u8 = src[p.*.pos];
        if (!isDigit(c)) {
            break;
        }
        acc = acc * 10 + ((c - 48) as u64);
        p.*.pos = p.*.pos + 1;
        digits = digits + 1;
    }
    if (digits == 0) {
        p.*.ok = false;
        return 0;
    }
    skipIntSuffix(p, src);
    return acc as i64;
}

fn parsePrimary(p: *Parser, src: string): i64 {
    skipWs(p, src);
    if (!p.*.ok || p.*.pos >= src.len) {
        p.*.ok = false;
        return 0;
    }
    let ch: u8 = src[p.*.pos];
    if (ch == 40) { // '('
        if (p.*.depth >= 64) {
            p.*.ok = false;
            return 0;
        }
        p.*.pos = p.*.pos + 1;
        p.*.depth = p.*.depth + 1;
        let v: i64 = parseTernary(p, src);
        p.*.depth = p.*.depth - 1;
        skipWs(p, src);
        if (!p.*.ok || peek(p, src) != 41) {
            p.*.ok = false;
            return 0;
        }
        p.*.pos = p.*.pos + 1;
        return v;
    }
    if (ch == 39) { // '\''
        return parseCharLit(p, src);
    }
    if (isDigit(ch)) {
        return parseIntLit(p, src);
    }
    p.*.ok = false;
    return 0;
}

fn parseUnary(p: *Parser, src: string): i64 {
    skipWs(p, src);
    if (!p.*.ok || p.*.pos >= src.len) {
        p.*.ok = false;
        return 0;
    }
    let ch: u8 = src[p.*.pos];
    if (ch == 43) { // '+'
        p.*.pos = p.*.pos + 1;
        return parseUnary(p, src);
    }
    if (ch == 45) { // '-'
        p.*.pos = p.*.pos + 1;
        return 0 - parseUnary(p, src);
    }
    if (ch == 126) { // '~'
        p.*.pos = p.*.pos + 1;
        return ~parseUnary(p, src);
    }
    if (ch == 33) { // '!'
        p.*.pos = p.*.pos + 1;
        let v: i64 = parseUnary(p, src);
        if (v == 0) {
            return 1;
        }
        return 0;
    }
    return parsePrimary(p, src);
}

fn parseMul(p: *Parser, src: string): i64 {
    let lhs: i64 = parseUnary(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        let ch: u8 = peek(p, src);
        if (ch == 42) { // '*'
            p.*.pos = p.*.pos + 1;
            let rhs: i64 = parseUnary(p, src);
            lhs = lhs * rhs;
        } else if (ch == 47) { // '/'
            p.*.pos = p.*.pos + 1;
            let rhs: i64 = parseUnary(p, src);
            if (!p.*.ok || rhs == 0) {
                p.*.ok = false;
                return 0;
            }
            lhs = signedDivI64(lhs, rhs);
        } else if (ch == 37) { // '%'
            p.*.pos = p.*.pos + 1;
            let rhs: i64 = parseUnary(p, src);
            if (!p.*.ok || rhs == 0) {
                p.*.ok = false;
                return 0;
            }
            lhs = signedRemI64(lhs, rhs);
        } else {
            break;
        }
    }
    return lhs;
}

fn parseAdd(p: *Parser, src: string): i64 {
    let lhs: i64 = parseMul(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        let ch: u8 = peek(p, src);
        if (ch == 43) { // '+'
            p.*.pos = p.*.pos + 1;
            lhs = lhs + parseMul(p, src);
        } else if (ch == 45) { // '-'
            p.*.pos = p.*.pos + 1;
            lhs = lhs - parseMul(p, src);
        } else {
            break;
        }
    }
    return lhs;
}

fn parseShift(p: *Parser, src: string): i64 {
    let lhs: i64 = parseAdd(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        let c1: u8 = peek(p, src);
        let c2: u8 = peek2(p, src);
        if (c1 == 60 && c2 == 60) { // '<<'
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseAdd(p, src);
            if (!p.*.ok || rhs < 0 || rhs >= 64) {
                p.*.ok = false;
                return 0;
            }
            let sh: u32 = rhs as u32;
            lhs = ((lhs as u64) << sh) as i64;
        } else if (c1 == 62 && c2 == 62) { // '>>'
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseAdd(p, src);
            if (!p.*.ok || rhs < 0 || rhs >= 64) {
                p.*.ok = false;
                return 0;
            }
            let sh: u32 = rhs as u32;
            if (lhs >= 0) {
                lhs = ((lhs as u64) >> sh) as i64;
            } else {
                let inv: u64 = ((0 - 1 - lhs) as u64) >> sh;
                lhs = 0 - 1 - (inv as i64);
            }
        } else {
            break;
        }
    }
    return lhs;
}

fn parseRel(p: *Parser, src: string): i64 {
    let lhs: i64 = parseShift(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        let c1: u8 = peek(p, src);
        let c2: u8 = peek2(p, src);
        if (c1 == 60 && c2 == 61) { // '<='
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseShift(p, src);
            if (lhs <= rhs) { lhs = 1; } else { lhs = 0; }
        } else if (c1 == 62 && c2 == 61) { // '>='
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseShift(p, src);
            if (lhs >= rhs) { lhs = 1; } else { lhs = 0; }
        } else if (c1 == 60 && c2 != 60) { // '<'
            p.*.pos = p.*.pos + 1;
            let rhs: i64 = parseShift(p, src);
            if (lhs < rhs) { lhs = 1; } else { lhs = 0; }
        } else if (c1 == 62 && c2 != 62) { // '>'
            p.*.pos = p.*.pos + 1;
            let rhs: i64 = parseShift(p, src);
            if (lhs > rhs) { lhs = 1; } else { lhs = 0; }
        } else {
            break;
        }
    }
    return lhs;
}

fn parseEq(p: *Parser, src: string): i64 {
    let lhs: i64 = parseRel(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        let c1: u8 = peek(p, src);
        let c2: u8 = peek2(p, src);
        if (c1 == 61 && c2 == 61) { // '=='
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseRel(p, src);
            if (lhs == rhs) { lhs = 1; } else { lhs = 0; }
        } else if (c1 == 33 && c2 == 61) { // '!='
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseRel(p, src);
            if (lhs != rhs) { lhs = 1; } else { lhs = 0; }
        } else {
            break;
        }
    }
    return lhs;
}

fn parseBitAnd(p: *Parser, src: string): i64 {
    let lhs: i64 = parseEq(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        if (peek(p, src) == 38 && peek2(p, src) != 38) {
            p.*.pos = p.*.pos + 1;
            lhs = lhs & parseEq(p, src);
        } else {
            break;
        }
    }
    return lhs;
}

fn parseBitXor(p: *Parser, src: string): i64 {
    let lhs: i64 = parseBitAnd(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        if (peek(p, src) == 94) {
            p.*.pos = p.*.pos + 1;
            lhs = lhs ^ parseBitAnd(p, src);
        } else {
            break;
        }
    }
    return lhs;
}

fn parseBitOr(p: *Parser, src: string): i64 {
    let lhs: i64 = parseBitXor(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        if (peek(p, src) == 124 && peek2(p, src) != 124) {
            p.*.pos = p.*.pos + 1;
            lhs = lhs | parseBitXor(p, src);
        } else {
            break;
        }
    }
    return lhs;
}

fn parseLogAnd(p: *Parser, src: string): i64 {
    let lhs: i64 = parseBitOr(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        if (peek(p, src) == 38 && peek2(p, src) == 38) {
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseBitOr(p, src);
            if (lhs != 0 && rhs != 0) { lhs = 1; } else { lhs = 0; }
        } else {
            break;
        }
    }
    return lhs;
}

fn parseLogOr(p: *Parser, src: string): i64 {
    let lhs: i64 = parseLogAnd(p, src);
    while (p.*.ok) {
        skipWs(p, src);
        if (peek(p, src) == 124 && peek2(p, src) == 124) {
            p.*.pos = p.*.pos + 2;
            let rhs: i64 = parseLogAnd(p, src);
            if (lhs != 0 || rhs != 0) { lhs = 1; } else { lhs = 0; }
        } else {
            break;
        }
    }
    return lhs;
}

fn parseTernary(p: *Parser, src: string): i64 {
    let cond: i64 = parseLogOr(p, src);
    if (!p.*.ok) {
        return 0;
    }
    skipWs(p, src);
    if (peek(p, src) == 63) { // '?'
        p.*.pos = p.*.pos + 1;
        let then_val: i64 = parseTernary(p, src);
        skipWs(p, src);
        if (!p.*.ok || peek(p, src) != 58) { // ':'
            p.*.ok = false;
            return 0;
        }
        p.*.pos = p.*.pos + 1;
        let else_val: i64 = parseTernary(p, src);
        if (cond != 0) {
            return then_val;
        }
        return else_val;
    }
    return cond;
}

pub fn evalCExpr(expr: string): EvalResult {
    let p: Parser = Parser { pos: 0, ok: true, depth: 0 };
    let val: i64 = parseTernary(&p, expr);
    skipWs(&p, expr);
    if (!p.ok || p.pos != expr.len) {
        return EvalResult { value: 0, ok: false };
    }
    return EvalResult { value: val, ok: true };
}

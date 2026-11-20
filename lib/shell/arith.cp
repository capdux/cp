// Source: /data/dev/bun/src/shell/
// ============================================================================
// lib/shell/arith.cp
// POSIX Integer Arithmetic Expansion Evaluator `$(( ... ))` (cap:shell)
// Supports +, -, *, /, %, **, <<, >>, &, |, ^, ~, !, <, <=, >, >=, ==, !=,
// &&, ||, ?:, =, +=, -=, *=, /=, %=, ++, --, parentheses, and ShellEnv variables.

const std = @import("std");
import { ShellEnv } from "./env.cp";

pub struct ArithEval {
    src: []const u8 = "",
    pos: usize = 0,
    env: *ShellEnv,
    has_err: bool = false,

    pub static fn eval(expr: []const u8, env: *ShellEnv): i64 {
        let ev = ArithEval {
            src: expr,
            pos: 0,
            env: env,
            has_err: false,
        };
        const v = ev.parseExpr();
        if (ev.has_err) return 0;
        return v;
    }

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peekChar(*_): u8 {
        _.skipWs();
        if (_.pos >= _.src.len) return 0;
        return _.src[_.pos];
    }

    fn matchStr(*_, s: []const u8): bool {
        _.skipWs();
        if (_.pos + s.len <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + s.len], s)) {
            _.pos += s.len;
            return true;
        }
        return false;
    }

    fn isIdentStart(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }

    fn isIdentCont(c: u8): bool {
        return ArithEval.isIdentStart(c) || (c >= '0' && c <= '9');
    }

    fn readVar(env: *ShellEnv, name: []const u8): i64 {
        if (val := env.get(name)) {
            const trimmed = std.mem.trim(u8, val, " \t\r\n");
            if (trimmed.len == 0) return 0;
            return std.fmt.parseInt(i64, trimmed, 0) catch 0;
        }
        return 0;
    }

    fn writeVar(env: *ShellEnv, name: []const u8, val: i64) {
        let tmp: [32]u8 = [0; 32];
        const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{val}) catch "0";
        * = env.set(name, s, false);
    }

    fn parseExpr(*_): i64 {
        // Comma operator
        let val = _.parseAssign();
        while (_.peekChar() == ',') {
            _.pos += 1;
            val = _.parseAssign();
        }
        return val;
    }

    fn parseAssign(*_): i64 {
        _.skipWs();
        const saved_pos = _.pos;
        if (_.pos < _.src.len && ArithEval.isIdentStart(_.src[_.pos])) {
            const name_start = _.pos;
            _.pos += 1;
            while (_.pos < _.src.len && ArithEval.isIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
            const name = _.src[name_start.._.pos];
            _.skipWs();
            if (_.pos < _.src.len) {
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '+' && _.src[_.pos + 1] == '=') {
                    _.pos += 2;
                    const rhs = _.parseAssign();
                    const res = ArithEval.readVar(_.env, name) +% rhs;
                    ArithEval.writeVar(_.env, name, res);
                    return res;
                }
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '-' && _.src[_.pos + 1] == '=') {
                    _.pos += 2;
                    const rhs = _.parseAssign();
                    const res = ArithEval.readVar(_.env, name) -% rhs;
                    ArithEval.writeVar(_.env, name, res);
                    return res;
                }
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '*' && _.src[_.pos + 1] == '=') {
                    _.pos += 2;
                    const rhs = _.parseAssign();
                    const res = ArithEval.readVar(_.env, name) *% rhs;
                    ArithEval.writeVar(_.env, name, res);
                    return res;
                }
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '/' && _.src[_.pos + 1] == '=') {
                    _.pos += 2;
                    const rhs = _.parseAssign();
                    if (rhs == 0) { _.has_err = true; return 0; }
                    const res = @divTrunc(ArithEval.readVar(_.env, name), rhs);
                    ArithEval.writeVar(_.env, name, res);
                    return res;
                }
                if (_.pos + 1 < _.src.len && _.src[_.pos] == '%' && _.src[_.pos + 1] == '=') {
                    _.pos += 2;
                    const rhs = _.parseAssign();
                    if (rhs == 0) { _.has_err = true; return 0; }
                    const res = @rem(ArithEval.readVar(_.env, name), rhs);
                    ArithEval.writeVar(_.env, name, res);
                    return res;
                }
                if (_.src[_.pos] == '=' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '=')) {
                    _.pos += 1;
                    const rhs = _.parseAssign();
                    ArithEval.writeVar(_.env, name, rhs);
                    return rhs;
                }
            }
            _.pos = saved_pos;
        }
        return _.parseTernary();
    }

    fn parseTernary(*_): i64 {
        const cond = _.parseLogicalOr();
        if (_.peekChar() == '?') {
            _.pos += 1;
            const t_val = _.parseExpr();
            if (_.peekChar() == ':') {
                _.pos += 1;
                const f_val = _.parseTernary();
                return if (cond != 0) t_val else f_val;
            }
            return t_val;
        }
        return cond;
    }

    fn parseLogicalOr(*_): i64 {
        let val = _.parseLogicalAnd();
        while (_.matchStr("||")) {
            const rhs = _.parseLogicalAnd();
            val = if (val != 0 || rhs != 0) 1 else 0;
        }
        return val;
    }

    fn parseLogicalAnd(*_): i64 {
        let val = _.parseBitOr();
        while (_.matchStr("&&")) {
            const rhs = _.parseBitOr();
            val = if (val != 0 && rhs != 0) 1 else 0;
        }
        return val;
    }

    fn parseBitOr(*_): i64 {
        let val = _.parseBitXor();
        while (true) {
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == '|' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '|')) {
                _.pos += 1;
                val = val | _.parseBitXor();
            } else {
                break;
            }
        }
        return val;
    }

    fn parseBitXor(*_): i64 {
        let val = _.parseBitAnd();
        while (_.peekChar() == '^') {
            _.pos += 1;
            val = val ^ _.parseBitAnd();
        }
        return val;
    }

    fn parseBitAnd(*_): i64 {
        let val = _.parseEq();
        while (true) {
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == '&' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '&')) {
                _.pos += 1;
                val = val & _.parseEq();
            } else {
                break;
            }
        }
        return val;
    }

    fn parseEq(*_): i64 {
        let val = _.parseRel();
        while (true) {
            if (_.matchStr("==")) {
                const rhs = _.parseRel();
                val = if (val == rhs) 1 else 0;
            } else if (_.matchStr("!=")) {
                const rhs = _.parseRel();
                val = if (val != rhs) 1 else 0;
            } else {
                break;
            }
        }
        return val;
    }

    fn parseRel(*_): i64 {
        let val = _.parseShift();
        while (true) {
            if (_.matchStr("<=")) {
                const rhs = _.parseShift();
                val = if (val <= rhs) 1 else 0;
            } else if (_.matchStr(">=")) {
                const rhs = _.parseShift();
                val = if (val >= rhs) 1 else 0;
            } else {
                _.skipWs();
                if (_.pos < _.src.len && _.src[_.pos] == '<' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '<')) {
                    _.pos += 1;
                    const rhs = _.parseShift();
                    val = if (val < rhs) 1 else 0;
                } else if (_.pos < _.src.len && _.src[_.pos] == '>' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '>')) {
                    _.pos += 1;
                    const rhs = _.parseShift();
                    val = if (val > rhs) 1 else 0;
                } else {
                    break;
                }
            }
        }
        return val;
    }

    fn parseShift(*_): i64 {
        let val = _.parseAdd();
        while (true) {
            if (_.matchStr("<<")) {
                const rhs = _.parseAdd();
                const sh_amt: u6 = @truncate(@as(u64, @bitCast(rhs)));
                val = val << sh_amt;
            } else if (_.matchStr(">>")) {
                const rhs = _.parseAdd();
                const sh_amt: u6 = @truncate(@as(u64, @bitCast(rhs)));
                val = val >> sh_amt;
            } else {
                break;
            }
        }
        return val;
    }

    fn parseAdd(*_): i64 {
        let val = _.parseMul();
        while (true) {
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == '+' && (_.pos + 1 >= _.src.len || (_.src[_.pos + 1] != '+' && _.src[_.pos + 1] != '='))) {
                _.pos += 1;
                val = val +% _.parseMul();
            } else if (_.pos < _.src.len && _.src[_.pos] == '-' && (_.pos + 1 >= _.src.len || (_.src[_.pos + 1] != '-' && _.src[_.pos + 1] != '='))) {
                _.pos += 1;
                val = val -% _.parseMul();
            } else {
                break;
            }
        }
        return val;
    }

    fn parseMul(*_): i64 {
        let val = _.parsePow();
        while (true) {
            _.skipWs();
            if (_.pos < _.src.len && _.src[_.pos] == '*' && (_.pos + 1 >= _.src.len || (_.src[_.pos + 1] != '*' && _.src[_.pos + 1] != '='))) {
                _.pos += 1;
                val = val *% _.parsePow();
            } else if (_.pos < _.src.len && _.src[_.pos] == '/' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '=')) {
                _.pos += 1;
                const rhs = _.parsePow();
                if (rhs == 0) {
                    _.has_err = true;
                    return 0;
                }
                val = @divTrunc(val, rhs);
            } else if (_.pos < _.src.len && _.src[_.pos] == '%' && (_.pos + 1 >= _.src.len || _.src[_.pos + 1] != '=')) {
                _.pos += 1;
                const rhs = _.parsePow();
                if (rhs == 0) {
                    _.has_err = true;
                    return 0;
                }
                val = @rem(val, rhs);
            } else {
                break;
            }
        }
        return val;
    }

    fn parsePow(*_): i64 {
        const base = _.parseUnary();
        if (_.matchStr("**")) {
            const exp = _.parsePow();
            if (exp < 0) return 0;
            let res: i64 = 1;
            let k: i64 = 0;
            while (k < exp && k < 64) {
                res = res *% base;
                k += 1;
            }
            return res;
        }
        return base;
    }

    fn parseUnary(*_): i64 {
        _.skipWs();
        if (_.pos >= _.src.len) return 0;
        const ch = _.src[_.pos];
        if (_.pos + 1 < _.src.len && ch == '+' && _.src[_.pos + 1] == '+') {
            _.pos += 2;
            _.skipWs();
            if (_.pos < _.src.len && ArithEval.isIdentStart(_.src[_.pos])) {
                const start = _.pos;
                while (_.pos < _.src.len && ArithEval.isIdentCont(_.src[_.pos])) _.pos += 1;
                const name = _.src[start.._.pos];
                const v = ArithEval.readVar(_.env, name) +% 1;
                ArithEval.writeVar(_.env, name, v);
                return v;
            }
            return _.parseUnary();
        }
        if (_.pos + 1 < _.src.len && ch == '-' && _.src[_.pos + 1] == '-') {
            _.pos += 2;
            _.skipWs();
            if (_.pos < _.src.len && ArithEval.isIdentStart(_.src[_.pos])) {
                const start = _.pos;
                while (_.pos < _.src.len && ArithEval.isIdentCont(_.src[_.pos])) _.pos += 1;
                const name = _.src[start.._.pos];
                const v = ArithEval.readVar(_.env, name) -% 1;
                ArithEval.writeVar(_.env, name, v);
                return v;
            }
            return _.parseUnary();
        }
        if (ch == '+') {
            _.pos += 1;
            return _.parseUnary();
        }
        if (ch == '-') {
            _.pos += 1;
            return 0 -% _.parseUnary();
        }
        if (ch == '!') {
            _.pos += 1;
            return if (_.parseUnary() == 0) 1 else 0;
        }
        if (ch == '~') {
            _.pos += 1;
            return ~_.parseUnary();
        }
        return _.parsePrimary();
    }

    fn parsePrimary(*_): i64 {
        _.skipWs();
        if (_.pos >= _.src.len) return 0;

        const ch = _.src[_.pos];
        if (ch == '(') {
            _.pos += 1;
            const v = _.parseExpr();
            if (_.peekChar() == ')') {
                _.pos += 1;
            }
            return v;
        }

        // Optional leading '$' before variable name inside arithmetic
        if (ch == '$') {
            _.pos += 1;
        }

        if (_.pos < _.src.len && ArithEval.isIdentStart(_.src[_.pos])) {
            const start = _.pos;
            _.pos += 1;
            while (_.pos < _.src.len && ArithEval.isIdentCont(_.src[_.pos])) {
                _.pos += 1;
            }
            const name = _.src[start.._.pos];
            const cur = ArithEval.readVar(_.env, name);
            _.skipWs();
            if (_.pos + 1 < _.src.len && _.src[_.pos] == '+' && _.src[_.pos + 1] == '+') {
                _.pos += 2;
                ArithEval.writeVar(_.env, name, cur +% 1);
            } else if (_.pos + 1 < _.src.len && _.src[_.pos] == '-' && _.src[_.pos + 1] == '-') {
                _.pos += 2;
                ArithEval.writeVar(_.env, name, cur -% 1);
            }
            return cur;
        }

        // Integer literal (decimal, 0x hex, 0b bin, 0 octal)
        const num_start = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] >= '0' && _.src[_.pos] <= '9') {
            while (_.pos < _.src.len && (std.ascii.isAlphanumeric(_.src[_.pos]) || _.src[_.pos] == '_')) {
                _.pos += 1;
            }
            const num_str = _.src[num_start.._.pos];
            return std.fmt.parseInt(i64, num_str, 0) catch 0;
        }

        return 0;
    }
}

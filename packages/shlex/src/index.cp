// @cap/shlex — Zero-allocation POSIX shell quoting & lexical splitter for CP
// Clean-room MIT implementation inspired by npm `shell-quote` and Rust `shlex`.

import { isAlnum, isSpace } from "cap:ascii";
import { writeStr } from "cap:fmt";

const MAX_TOKENS: usize = 64;
const MAX_POOL_BYTES: usize = 4096;

pub enum ShlexError {
    None,
    UnclosedSingleQuote,
    UnclosedDoubleQuote,
    TrailingBackslash,
    TooManyTokens,
    BufferOverflow,
}

pub struct SplitResult {
    pub pool: [MAX_POOL_BYTES]u8,
    pub starts: [MAX_TOKENS]usize,
    pub lens: [MAX_TOKENS]usize,
    pub count: usize,
    pub ok: bool,
    pub err: ShlexError,

    pub static fn empty() -> SplitResult {
        let p: [MAX_POOL_BYTES]u8 = undefined;
        let s: [MAX_TOKENS]usize = undefined;
        let l: [MAX_TOKENS]usize = undefined;
        return SplitResult{
            pool: p,
            starts: s,
            lens: l,
            count: 0,
            ok: true,
            err: ShlexError.None,
        };
    }

    pub fn tokenAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        const st = _.starts[idx];
        const ln = _.lens[idx];
        return _.pool[st..st + ln];
    }
}

fn isSafeUnquotedChar(c: u8) -> bool {
    if (isAlnum(c)) {
        return true;
    }
    return c == '_' || c == '@' || c == '%' || c == '+' || c == '=' || c == ':' || c == ',' || c == '.' || c == '/' || c == '-';
}

pub fn needsQuote(arg: string) -> bool {
    if (arg.len == 0) {
        return true;
    }
    let i: usize = 0;
    while (i < arg.len) {
        if (!isSafeUnquotedChar(arg[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn hasShellMetachars(s: string) -> bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == ';' || c == '|' || c == '&' || c == '$' || c == '`' ||
            c == '<' || c == '>' || c == '(' || c == ')' || c == '\n' || c == '\r') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

pub fn quoteArg(arg: string, out: []u8) -> string {
    if (!needsQuote(arg)) {
        const n = writeStr(arg, out, 0 as usize);
        return out[0..n];
    }
    if (arg.len == 0) {
        const n = writeStr("''", out, 0 as usize);
        return out[0..n];
    }
    let pos: usize = 0;
    pos = appendByte(out, pos, '\'');
    let i: usize = 0;
    while (i < arg.len) {
        if (arg[i] == '\'') {
            pos = writeStr("'\\''", out, pos);
        } else {
            pos = appendByte(out, pos, arg[i]);
        }
        i += 1;
    }
    pos = appendByte(out, pos, '\'');
    return out[0..pos];
}

pub fn joinArgs(args: []string, out: []u8) -> string {
    let pos: usize = 0;
    let tmp: [512]u8 = undefined;
    let i: usize = 0;
    while (i < args.len) {
        if (i > 0) {
            pos = appendByte(out, pos, ' ');
        }
        const q = quoteArg(args[i], tmp[0..512]);
        pos = writeStr(q, out, pos);
        i += 1;
    }
    return out[0..pos];
}

pub fn splitShell(cmd: string) -> SplitResult {
    let res = SplitResult.empty();
    let pool_pos: usize = 0;
    let i: usize = 0;

    while (i < cmd.len) {
        while (i < cmd.len && isSpace(cmd[i])) {
            i += 1;
        }
        if (i >= cmd.len) {
            break;
        }
        // POSIX comment at start of word
        if (cmd[i] == '#') {
            break;
        }
        if (res.count >= MAX_TOKENS) {
            res.ok = false;
            res.err = ShlexError.TooManyTokens;
            return res;
        }

        const tok_start = pool_pos;
        while (i < cmd.len && !isSpace(cmd[i])) {
            const c = cmd[i];
            if (c == '\'') {
                i += 1;
                let closed = false;
                while (i < cmd.len) {
                    if (cmd[i] == '\'') {
                        closed = true;
                        i += 1;
                        break;
                    }
                    if (pool_pos >= MAX_POOL_BYTES) {
                        res.ok = false;
                        res.err = ShlexError.BufferOverflow;
                        return res;
                    }
                    res.pool[pool_pos] = cmd[i];
                    pool_pos += 1;
                    i += 1;
                }
                if (!closed) {
                    res.ok = false;
                    res.err = ShlexError.UnclosedSingleQuote;
                    return res;
                }
            } else if (c == '"') {
                i += 1;
                let closed = false;
                while (i < cmd.len) {
                    const dc = cmd[i];
                    if (dc == '"') {
                        closed = true;
                        i += 1;
                        break;
                    }
                    if (dc == '\\' && i + 1 < cmd.len) {
                        const nxt = cmd[i + 1];
                        if (nxt == '"' || nxt == '\\' || nxt == '$' || nxt == '`' || nxt == '\n') {
                            if (pool_pos >= MAX_POOL_BYTES) {
                                res.ok = false;
                                res.err = ShlexError.BufferOverflow;
                                return res;
                            }
                            res.pool[pool_pos] = nxt;
                            pool_pos += 1;
                            i += 2;
                            continue;
                        }
                    }
                    if (pool_pos >= MAX_POOL_BYTES) {
                        res.ok = false;
                        res.err = ShlexError.BufferOverflow;
                        return res;
                    }
                    res.pool[pool_pos] = dc;
                    pool_pos += 1;
                    i += 1;
                }
                if (!closed) {
                    res.ok = false;
                    res.err = ShlexError.UnclosedDoubleQuote;
                    return res;
                }
            } else if (c == '\\') {
                if (i + 1 >= cmd.len) {
                    res.ok = false;
                    res.err = ShlexError.TrailingBackslash;
                    return res;
                }
                if (pool_pos >= MAX_POOL_BYTES) {
                    res.ok = false;
                    res.err = ShlexError.BufferOverflow;
                    return res;
                }
                res.pool[pool_pos] = cmd[i + 1];
                pool_pos += 1;
                i += 2;
            } else {
                if (pool_pos >= MAX_POOL_BYTES) {
                    res.ok = false;
                    res.err = ShlexError.BufferOverflow;
                    return res;
                }
                res.pool[pool_pos] = c;
                pool_pos += 1;
                i += 1;
            }
        }

        res.starts[res.count] = tok_start;
        res.lens[res.count] = pool_pos - tok_start;
        res.count += 1;
    }

    return res;
}

export {
    ShlexError,
    SplitResult,
    needsQuote,
    hasShellMetachars,
    quoteArg,
    joinArgs,
    splitShell,
};

// @cap/shlex — Zero-allocation POSIX shell quoting & lexical splitter for CP
// Clean-room MIT implementation inspired by npm `shell-quote` and Rust `shlex`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUT, and @memcpy.

import { writeStr } from "cap:fmt";

type ShlexVec16 = @Vector(16, u8);
type ShlexBool16 = @Vector(16, bool);

const MAX_TOKENS: usize = 64;
const MAX_POOL_BYTES: usize = 4096;

const SHLEX_FLAG_SAFE: u8 = 1;
const SHLEX_FLAG_META: u8 = 2;
const SHLEX_FLAG_SPACE: u8 = 4;
const SHLEX_FLAG_DBL_ESC: u8 = 8;
const SHLEX_FLAG_UNQUOTED_STOP: u8 = 16;

fn shlexBuildCharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if ((c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) ||
            c == '_' || c == '@' || c == '%' || c == '+' || c == '=' ||
            c == ':' || c == ',' || c == '.' || c == '/' || c == '-') {
            f |= SHLEX_FLAG_SAFE;
        }
        if (c == ';' || c == '|' || c == '&' || c == '$' || c == '`' ||
            c == '<' || c == '>' || c == '(' || c == ')' || c == '\n' || c == '\r') {
            f |= SHLEX_FLAG_META;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C) {
            f |= SHLEX_FLAG_SPACE | SHLEX_FLAG_UNQUOTED_STOP;
        }
        if (c == '"' || c == '\\' || c == '$' || c == '`' || c == '\n') {
            f |= SHLEX_FLAG_DBL_ESC;
        }
        if (c == '\'' || c == '"' || c == '\\') {
            f |= SHLEX_FLAG_UNQUOTED_STOP;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const SHLEX_CHAR_LUT: [256]u8 = shlexBuildCharLut();

fn shlexFindByteSimd(s: string, target: u8, start: usize) -> usize {
    let i = start;
    const len = s.len;
    const vt: ShlexVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: ShlexVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn shlexFindTwoBytesSimd(s: string, a: u8, b: u8, start: usize) -> usize {
    let i = start;
    const len = s.len;
    const va: ShlexVec16 = @splat(a);
    const vb: ShlexVec16 = @splat(b);
    while (i + 16 <= len) {
        const v: ShlexVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == va) || @reduce(.Or, v == vb)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != a && s[i] != b) {
        i += 1;
    }
    return i;
}

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

pub fn needsQuote(arg: string) -> bool {
    if (arg.len == 0) {
        return true;
    }
    let i: usize = 0;
    const len = arg.len;
    const v_a: ShlexVec16 = @splat('a' as u8);
    const v_z: ShlexVec16 = @splat('z' as u8);
    const v_A: ShlexVec16 = @splat('A' as u8);
    const v_Z: ShlexVec16 = @splat('Z' as u8);
    const v_0: ShlexVec16 = @splat('0' as u8);
    const v_9: ShlexVec16 = @splat('9' as u8);
    const v_dash: ShlexVec16 = @splat('-' as u8);
    const v_dot: ShlexVec16 = @splat('.' as u8);
    const v_slash: ShlexVec16 = @splat('/' as u8);
    const v_under: ShlexVec16 = @splat('_' as u8);
    const v_false: ShlexBool16 = @splat(false);
    const v_true: ShlexBool16 = @splat(true);
    while (i + 16 <= len) {
        const v: ShlexVec16 = arg[i .. i + 16][0..16].*;
        const is_lo = @select(bool, v >= v_a, v <= v_z, v_false);
        const is_hi = @select(bool, v >= v_A, v <= v_Z, v_false);
        const is_dg = @select(bool, v >= v_0, v <= v_9, v_false);
        const is_sym1 = @select(bool, v == v_dash, v_true, v == v_dot);
        const is_sym2 = @select(bool, v == v_slash, v_true, v == v_under);
        const ok1 = @select(bool, is_lo, v_true, is_hi);
        const ok2 = @select(bool, is_dg, v_true, is_sym1);
        const ok3 = @select(bool, ok1, v_true, ok2);
        const ok_all = @select(bool, ok3, v_true, is_sym2);
        if (!@reduce(.And, ok_all)) {
            break;
        }
        i += 16;
    }
    while (i < len) {
        if ((SHLEX_CHAR_LUT[arg[i] as usize] & SHLEX_FLAG_SAFE) == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn hasShellMetachars(s: string) -> bool {
    let i: usize = 0;
    const len = s.len;
    const v_semi: ShlexVec16 = @splat(';' as u8);
    const v_pipe: ShlexVec16 = @splat('|' as u8);
    const v_amp: ShlexVec16 = @splat('&' as u8);
    const v_dol: ShlexVec16 = @splat('$' as u8);
    const v_bt: ShlexVec16 = @splat('`' as u8);
    const v_lt: ShlexVec16 = @splat('<' as u8);
    const v_gt: ShlexVec16 = @splat('>' as u8);
    const v_lp: ShlexVec16 = @splat('(' as u8);
    const v_rp: ShlexVec16 = @splat(')' as u8);
    const v_nl: ShlexVec16 = @splat('\n' as u8);
    const v_cr: ShlexVec16 = @splat('\r' as u8);
    while (i + 16 <= len) {
        const v: ShlexVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_semi) || @reduce(.Or, v == v_pipe) ||
            @reduce(.Or, v == v_amp) || @reduce(.Or, v == v_dol) ||
            @reduce(.Or, v == v_bt) || @reduce(.Or, v == v_lt) ||
            @reduce(.Or, v == v_gt) || @reduce(.Or, v == v_lp) ||
            @reduce(.Or, v == v_rp) || @reduce(.Or, v == v_nl) ||
            @reduce(.Or, v == v_cr)) {
            return true;
        }
        i += 16;
    }
    while (i < len) {
        if ((SHLEX_CHAR_LUT[s[i] as usize] & SHLEX_FLAG_META) != 0) {
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

fn appendSlice(out: []u8, pos: usize, src: string) -> usize {
    if (pos >= out.len || src.len == 0) {
        return pos;
    }
    const avail = out.len - pos;
    const n = if (src.len < avail) { src.len } else { avail };
    @memcpy(out[pos .. pos + n], src[0..n]);
    return pos + n;
}

pub fn quoteArg(arg: string, out: []u8) -> string {
    if (!needsQuote(arg)) {
        const n = appendSlice(out, 0, arg);
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
        const next_q = shlexFindByteSimd(arg, '\'', i);
        if (next_q > i) {
            pos = appendSlice(out, pos, arg[i..next_q]);
        }
        if (next_q < arg.len) {
            pos = writeStr("'\\''", out, pos);
            i = next_q + 1;
        } else {
            break;
        }
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
        pos = appendSlice(out, pos, q);
        i += 1;
    }
    return out[0..pos];
}

pub fn splitShell(cmd: string) -> SplitResult {
    let res = SplitResult.empty();
    let pool_pos: usize = 0;
    let i: usize = 0;

    while (i < cmd.len) {
        while (i < cmd.len && (SHLEX_CHAR_LUT[cmd[i] as usize] & SHLEX_FLAG_SPACE) != 0) {
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
        while (i < cmd.len && (SHLEX_CHAR_LUT[cmd[i] as usize] & SHLEX_FLAG_SPACE) == 0) {
            const c = cmd[i];
            if (c == '\'') {
                i += 1;
                const close_idx = shlexFindByteSimd(cmd, '\'', i);
                const run_len = close_idx - i;
                if (pool_pos + run_len > MAX_POOL_BYTES) {
                    res.ok = false;
                    res.err = ShlexError.BufferOverflow;
                    return res;
                }
                if (run_len > 0) {
                    @memcpy(res.pool[pool_pos .. pool_pos + run_len], cmd[i..close_idx]);
                    pool_pos += run_len;
                }
                if (close_idx >= cmd.len) {
                    res.ok = false;
                    res.err = ShlexError.UnclosedSingleQuote;
                    return res;
                }
                i = close_idx + 1;
            } else if (c == '"') {
                i += 1;
                let closed = false;
                while (i < cmd.len) {
                    const next_spec = shlexFindTwoBytesSimd(cmd, '"', '\\', i);
                    const run_len = next_spec - i;
                    if (run_len > 0) {
                        if (pool_pos + run_len > MAX_POOL_BYTES) {
                            res.ok = false;
                            res.err = ShlexError.BufferOverflow;
                            return res;
                        }
                        @memcpy(res.pool[pool_pos .. pool_pos + run_len], cmd[i..next_spec]);
                        pool_pos += run_len;
                        i = next_spec;
                    }
                    if (i >= cmd.len) {
                        break;
                    }
                    const dc = cmd[i];
                    if (dc == '"') {
                        closed = true;
                        i += 1;
                        break;
                    }
                    if (dc == '\\' && i + 1 < cmd.len) {
                        const nxt = cmd[i + 1];
                        if ((SHLEX_CHAR_LUT[nxt as usize] & SHLEX_FLAG_DBL_ESC) != 0) {
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
                const run_start = i;
                i += 1;
                while (i < cmd.len && (SHLEX_CHAR_LUT[cmd[i] as usize] & SHLEX_FLAG_UNQUOTED_STOP) == 0) {
                    i += 1;
                }
                const run_len = i - run_start;
                if (pool_pos + run_len > MAX_POOL_BYTES) {
                    res.ok = false;
                    res.err = ShlexError.BufferOverflow;
                    return res;
                }
                @memcpy(res.pool[pool_pos .. pool_pos + run_len], cmd[run_start..i]);
                pool_pos += run_len;
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

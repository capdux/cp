// @cap/regex-lite — Zero-allocation linear-time Thompson NFA regex engine for CP
// Clean-room MIT implementation (Rust `regex-lite` v0.1 parity).
// Reuses `@cap/regex-syntax` (`validateRegexSyntax`, `extractLiteralPrefix`, `isWordByte`)
// and `@cap/memchr` (`memmemFind`) for literal-prefix fast skipping.
// Executes via a PikeVM-style Thompson NFA (max 128 instructions) immune to catastrophic backtracking (ReDoS).

import {
    validateRegexSyntax,
    extractLiteralPrefix,
    isWordByte
} from "../../regex-syntax/src/index.cp";
import { memmemFind } from "../../memchr/src/index.cp";
import { hexVal } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const RL_MAX_INSTS: usize = 128;
const RL_MAX_CLASSES: usize = 16;
const RL_MAX_PREFIX: usize = 64;

const RL_OP_MATCH: u8 = 0;
const RL_OP_BYTE: u8 = 1;
const RL_OP_ANY: u8 = 2;
const RL_OP_CLASS: u8 = 3;
const RL_OP_SPLIT: u8 = 4;
const RL_OP_JMP: u8 = 5;
const RL_OP_ANCHOR_START: u8 = 6;
const RL_OP_ANCHOR_END: u8 = 7;
const RL_OP_WORD_BOUND: u8 = 8;

/// Result of a regex match operation (`[start, end)` half-open byte range).
pub struct RegexMatch {
    pub matched: bool,
    pub start: usize,
    pub end: usize,

    pub fn len(): usize {
        if (!_.matched || _.end <= _.start) {
            return 0;
        }
        return _.end - _.start;
    }
}

struct RlInst {
    op: u8,
    arg1: usize,
    arg2: usize
}

struct RlClass {
    bits: [4]u64
}

struct RlCompileCtx {
    insts: [128]RlInst,
    inst_len: usize,
    classes: [16]RlClass,
    class_len: usize,
    ok: bool
}

// ============================================================================
// Private helpers (`rl` prefix to avoid module-flattening symbol collisions)
// ============================================================================

fn rlEmptyInst(): RlInst {
    return RlInst { op: RL_OP_MATCH, arg1: 0, arg2: 0 };
}

fn rlEmptyClass(): RlClass {
    return RlClass { bits: [0; 4] };
}

fn rlNoMatch(): RegexMatch {
    return RegexMatch { matched: false, start: 0, end: 0 };
}

fn rlClassSet(c: *RlClass, b: u8) {
    let idx = (b >> 6) as usize;
    let shift = (b & 63) as u6;
    c.bits[idx] = c.bits[idx] | ((1 as u64) << shift);
}

fn rlClassTest(c: RlClass, b: u8): bool {
    let idx = (b >> 6) as usize;
    let shift = (b & 63) as u6;
    return (c.bits[idx] & ((1 as u64) << shift)) != 0;
}

fn rlClassSetRange(c: *RlClass, lo: u8, hi: u8) {
    let cur: usize = lo as usize;
    let end: usize = hi as usize;
    while (cur <= end) {
        rlClassSet(c, cur as u8);
        cur = cur + 1;
    }
}

fn rlClassInvert(c: *RlClass) {
    c.bits[0] = ~c.bits[0];
    c.bits[1] = ~c.bits[1];
    c.bits[2] = ~c.bits[2];
    c.bits[3] = ~c.bits[3];
}

fn rlClassAddShorthand(c: *RlClass, esc: u8) {
    if (esc == 0x64) { // \d
        rlClassSetRange(c, 48, 57);
    } else if (esc == 0x44) { // \D
        let b: usize = 0;
        while (b < 256) {
            if (b < 48 || b > 57) {
                rlClassSet(c, b as u8);
            }
            b = b + 1;
        }
    } else if (esc == 0x77) { // \w
        let b: usize = 0;
        while (b < 256) {
            if (isWordByte(b as u8)) {
                rlClassSet(c, b as u8);
            }
            b = b + 1;
        }
    } else if (esc == 0x57) { // \W
        let b: usize = 0;
        while (b < 256) {
            if (!isWordByte(b as u8)) {
                rlClassSet(c, b as u8);
            }
            b = b + 1;
        }
    } else if (esc == 0x73) { // \s
        rlClassSet(c, 0x20);
        rlClassSet(c, 0x09);
        rlClassSet(c, 0x0A);
        rlClassSet(c, 0x0B);
        rlClassSet(c, 0x0C);
        rlClassSet(c, 0x0D);
    } else if (esc == 0x53) { // \S
        let b: usize = 0;
        while (b < 256) {
            if (b != 0x20 && b != 0x09 && b != 0x0A && b != 0x0B && b != 0x0C && b != 0x0D) {
                rlClassSet(c, b as u8);
            }
            b = b + 1;
        }
    }
}

fn rlHexVal(b: u8): usize {
    return (hexVal(b) ?? 0) as usize;
}

fn rlEmit(ctx: *RlCompileCtx, op: u8, arg1: usize, arg2: usize): usize {
    if (!ctx.ok || ctx.inst_len >= RL_MAX_INSTS) {
        ctx.ok = false;
        return 0;
    }
    let pc = ctx.inst_len;
    ctx.insts[pc] = RlInst { op: op, arg1: arg1, arg2: arg2 };
    ctx.inst_len = pc + 1;
    return pc;
}

fn rlAllocClass(ctx: *RlCompileCtx): usize {
    if (!ctx.ok || ctx.class_len >= RL_MAX_CLASSES) {
        ctx.ok = false;
        return 0;
    }
    let id = ctx.class_len;
    ctx.classes[id] = rlEmptyClass();
    ctx.class_len = id + 1;
    return id;
}

/// Inserts a blank instruction slot at `at_pc`, shifting `[at_pc..inst_len)` right by 1
/// and fixing up internal jump/split targets `>= at_pc`.
fn rlInsertSlot(ctx: *RlCompileCtx, at_pc: usize) {
    if (!ctx.ok || ctx.inst_len >= RL_MAX_INSTS) {
        ctx.ok = false;
        return;
    }
    let k = ctx.inst_len;
    while (k > at_pc) {
        let inst = ctx.insts[k - 1];
        if (inst.op == RL_OP_JMP) {
            if (inst.arg1 >= at_pc) {
                inst.arg1 = inst.arg1 + 1;
            }
        } else if (inst.op == RL_OP_SPLIT) {
            if (inst.arg1 >= at_pc) {
                inst.arg1 = inst.arg1 + 1;
            }
            if (inst.arg2 >= at_pc) {
                inst.arg2 = inst.arg2 + 1;
            }
        }
        ctx.insts[k] = inst;
        k = k - 1;
    }
    ctx.insts[at_pc] = rlEmptyInst();
    ctx.inst_len = ctx.inst_len + 1;
}

fn rlDecodeEscapeByte(pat: string, pos: usize): u8 {
    let esc = pat[pos + 1];
    if (esc == 0x6E) { return 0x0A; }
    if (esc == 0x72) { return 0x0D; }
    if (esc == 0x74) { return 0x09; }
    if (esc == 0x66) { return 0x0C; }
    if (esc == 0x76) { return 0x0B; }
    if (esc == 0x61) { return 0x07; }
    if (esc == 0x30) { return 0x00; }
    if (esc == 0x78 && pos + 3 < pat.len) {
        return ((rlHexVal(pat[pos + 2]) << 4) | rlHexVal(pat[pos + 3])) as u8;
    }
    return esc;
}

fn rlEscapeLen(pat: string, pos: usize): usize {
    if (pos + 1 < pat.len && pat[pos + 1] == 0x78) {
        return 4;
    }
    return 2;
}

fn rlIsShorthand(esc: u8): bool {
    return esc == 0x64 || esc == 0x44 || esc == 0x77 || esc == 0x57 || esc == 0x73 || esc == 0x53;
}

fn rlCompileCharClass(ctx: *RlCompileCtx, pat: string, start_pos: usize): usize {
    let cid = rlAllocClass(ctx);
    if (!ctx.ok) {
        return pat.len;
    }
    let i = start_pos + 1;
    let negated = false;
    if (i < pat.len && pat[i] == 0x5E) {
        negated = true;
        i = i + 1;
    }

    let first = true;
    let has_prev = false;
    let prev_ch: u8 = 0;
    while (i < pat.len) {
        let b = pat[i];
        if (b == 0x5D && !first) {
            i = i + 1;
            break;
        }
        first = false;

        if (b == 0x2D && has_prev && i + 1 < pat.len && pat[i + 1] != 0x5D) {
            i = i + 1;
            let hi: u8 = pat[i];
            if (hi == 0x5C) {
                hi = rlDecodeEscapeByte(pat, i);
                i = i + rlEscapeLen(pat, i);
            } else {
                i = i + 1;
            }
            rlClassSetRange(&ctx.classes[cid], prev_ch, hi);
            has_prev = false;
            continue;
        }

        if (b == 0x5C && i + 1 < pat.len) {
            let esc = pat[i + 1];
            if (rlIsShorthand(esc)) {
                rlClassAddShorthand(&ctx.classes[cid], esc);
                has_prev = false;
                i = i + 2;
                continue;
            }
            let ch = rlDecodeEscapeByte(pat, i);
            rlClassSet(&ctx.classes[cid], ch);
            prev_ch = ch;
            has_prev = true;
            i = i + rlEscapeLen(pat, i);
            continue;
        }

        rlClassSet(&ctx.classes[cid], b);
        prev_ch = b;
        has_prev = true;
        i = i + 1;
    }

    if (negated) {
        rlClassInvert(&ctx.classes[cid]);
    }
    * = rlEmit(ctx, RL_OP_CLASS, cid, 0);
    return i;
}

/// Skips extended group header after `(?`. Returns next position (after `:` or `>` or `)`).
fn rlSkipGroupHeader(pat: string, pos: usize): usize {
    let i = pos;
    if (i < pat.len && pat[i] == 0x3A) { // (?:
        return i + 1;
    }
    if (i < pat.len && (pat[i] == 0x50 || pat[i] == 0x3C)) { // (?P<name> or (?<name>
        while (i < pat.len && pat[i] != 0x3E) {
            i = i + 1;
        }
        if (i < pat.len) {
            i = i + 1;
        }
        return i;
    }
    // Flags (?i:...) or (?i)
    while (i < pat.len && pat[i] != 0x3A && pat[i] != 0x29) {
        i = i + 1;
    }
    if (i < pat.len) {
        i = i + 1;
    }
    return i;
}

fn rlCompileAtom(ctx: *RlCompileCtx, pat: string, pos: usize): usize {
    if (!ctx.ok || pos >= pat.len) {
        return pos;
    }
    let b = pat[pos];

    if (b == 0x28) { // '('
        let inner = pos + 1;
        if (inner < pat.len && pat[inner] == 0x3F) {
            let afterHdr = rlSkipGroupHeader(pat, inner + 1);
            // Inline flags `(?i)` already closed with ')'
            if (afterHdr > inner + 1 && pat[afterHdr - 1] == 0x29) {
                return afterHdr;
            }
            inner = afterHdr;
        }
        let nextPos = rlCompileAlt(ctx, pat, inner);
        if (nextPos < pat.len && pat[nextPos] == 0x29) {
            return nextPos + 1;
        }
        return nextPos;
    }

    if (b == 0x5B) { // '['
        return rlCompileCharClass(ctx, pat, pos);
    }

    if (b == 0x5C && pos + 1 < pat.len) { // '\'
        let esc = pat[pos + 1];
        if (rlIsShorthand(esc)) {
            let cid = rlAllocClass(ctx);
            if (ctx.ok) {
                rlClassAddShorthand(&ctx.classes[cid], esc);
                * = rlEmit(ctx, RL_OP_CLASS, cid, 0);
            }
            return pos + 2;
        }
        if (esc == 0x41) { // \A
            * = rlEmit(ctx, RL_OP_ANCHOR_START, 0, 0);
            return pos + 2;
        }
        if (esc == 0x7A || esc == 0x5A) { // \z / \Z
            * = rlEmit(ctx, RL_OP_ANCHOR_END, 0, 0);
            return pos + 2;
        }
        if (esc == 0x62) { // \b
            * = rlEmit(ctx, RL_OP_WORD_BOUND, 0, 0);
            return pos + 2;
        }
        if (esc == 0x42) { // \B
            * = rlEmit(ctx, RL_OP_WORD_BOUND, 1, 0);
            return pos + 2;
        }
        let ch = rlDecodeEscapeByte(pat, pos);
        * = rlEmit(ctx, RL_OP_BYTE, ch as usize, 0);
        return pos + rlEscapeLen(pat, pos);
    }

    if (b == 0x2E) { // '.'
        * = rlEmit(ctx, RL_OP_ANY, 0, 0);
        return pos + 1;
    }

    if (b == 0x5E) { // '^'
        * = rlEmit(ctx, RL_OP_ANCHOR_START, 0, 0);
        return pos + 1;
    }

    if (b == 0x24) { // '$'
        * = rlEmit(ctx, RL_OP_ANCHOR_END, 0, 0);
        return pos + 1;
    }

    * = rlEmit(ctx, RL_OP_BYTE, b as usize, 0);
    return pos + 1;
}

fn rlCompileQuant(ctx: *RlCompileCtx, pat: string, pos: usize): usize {
    let atom_pat_start = pos;
    let atom_pc_start = ctx.inst_len;
    let i = rlCompileAtom(ctx, pat, pos);
    if (!ctx.ok || i >= pat.len) {
        return i;
    }

    let q = pat[i];
    if (q == 0x3F) { // '?' or '??'
        i = i + 1;
        let lazy = false;
        if (i < pat.len && pat[i] == 0x3F) {
            lazy = true;
            i = i + 1;
        }
        rlInsertSlot(ctx, atom_pc_start);
        if (!ctx.ok) { return i; }
        let after = ctx.inst_len;
        if (lazy) {
            ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: atom_pc_start + 1 };
        } else {
            ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: atom_pc_start + 1, arg2: after };
        }
        return i;
    }

    if (q == 0x2A) { // '*' or '*?'
        i = i + 1;
        let lazy = false;
        if (i < pat.len && pat[i] == 0x3F) {
            lazy = true;
            i = i + 1;
        }
        rlInsertSlot(ctx, atom_pc_start);
        let jmp_pc = rlEmit(ctx, RL_OP_JMP, atom_pc_start, 0);
        if (!ctx.ok) { return i; }
        let after = jmp_pc + 1;
        if (lazy) {
            ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: atom_pc_start + 1 };
        } else {
            ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: atom_pc_start + 1, arg2: after };
        }
        return i;
    }

    if (q == 0x2B) { // '+' or '+?'
        i = i + 1;
        let lazy = false;
        if (i < pat.len && pat[i] == 0x3F) {
            lazy = true;
            i = i + 1;
        }
        let sp = rlEmit(ctx, RL_OP_SPLIT, 0, 0);
        if (!ctx.ok) { return i; }
        let after = sp + 1;
        if (lazy) {
            ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: atom_pc_start };
        } else {
            ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: atom_pc_start, arg2: after };
        }
        return i;
    }

    if (q == 0x7B) { // '{m}', '{m,}', '{m,n}'
        i = i + 1;
        let min_rep: usize = 0;
        while (i < pat.len && pat[i] >= 48 && pat[i] <= 57) {
            min_rep = min_rep * 10 + ((pat[i] - 48) as usize);
            i = i + 1;
        }
        let has_max = true;
        let max_rep = min_rep;
        if (i < pat.len && pat[i] == 0x2C) {
            i = i + 1;
            if (i < pat.len && pat[i] == 0x7D) {
                has_max = false;
                max_rep = 0;
            } else {
                max_rep = 0;
                while (i < pat.len && pat[i] >= 48 && pat[i] <= 57) {
                    max_rep = max_rep * 10 + ((pat[i] - 48) as usize);
                    i = i + 1;
                }
            }
        }
        if (i < pat.len && pat[i] == 0x7D) {
            i = i + 1;
        }
        let lazy = false;
        if (i < pat.len && pat[i] == 0x3F) {
            lazy = true;
            i = i + 1;
        }

        // First copy is already emitted at [atom_pc_start..ctx.inst_len)
        if (min_rep == 0) {
            if (!has_max) {
                // {0,} == '*'
                rlInsertSlot(ctx, atom_pc_start);
                let jmp_pc = rlEmit(ctx, RL_OP_JMP, atom_pc_start, 0);
                if (!ctx.ok) { return i; }
                let after = jmp_pc + 1;
                if (lazy) {
                    ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: atom_pc_start + 1 };
                } else {
                    ctx.insts[atom_pc_start] = RlInst { op: RL_OP_SPLIT, arg1: atom_pc_start + 1, arg2: after };
                }
                return i;
            }
            // {0,0} or {0,max_rep}: rollback first mandatory copy and emit max_rep optional copies
            ctx.inst_len = atom_pc_start;
            let r: usize = 0;
            while (r < max_rep && ctx.ok) {
                let sp = rlEmit(ctx, RL_OP_SPLIT, 0, 0);
                let body_start = ctx.inst_len;
                * = rlCompileAtom(ctx, pat, atom_pat_start);
                let after = ctx.inst_len;
                if (lazy) {
                    ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: body_start };
                } else {
                    ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: body_start, arg2: after };
                }
                r = r + 1;
            }
            return i;
        }

        // min_rep >= 1: emit remaining (min_rep - 1) mandatory copies
        let last_copy_start = atom_pc_start;
        let m: usize = 1;
        while (m < min_rep && ctx.ok) {
            last_copy_start = ctx.inst_len;
            * = rlCompileAtom(ctx, pat, atom_pat_start);
            m = m + 1;
        }

        if (!has_max) {
            // {m,}: loop back on the last mandatory copy (behaves like '+')
            let sp = rlEmit(ctx, RL_OP_SPLIT, 0, 0);
            if (!ctx.ok) { return i; }
            let after = sp + 1;
            if (lazy) {
                ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: last_copy_start };
            } else {
                ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: last_copy_start, arg2: after };
            }
        } else {
            // {m,n}: emit (max_rep - min_rep) optional copies
            let opt_cnt = max_rep - min_rep;
            let r: usize = 0;
            while (r < opt_cnt && ctx.ok) {
                let sp = rlEmit(ctx, RL_OP_SPLIT, 0, 0);
                let body_start = ctx.inst_len;
                * = rlCompileAtom(ctx, pat, atom_pat_start);
                let after = ctx.inst_len;
                if (lazy) {
                    ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: after, arg2: body_start };
                } else {
                    ctx.insts[sp] = RlInst { op: RL_OP_SPLIT, arg1: body_start, arg2: after };
                }
                r = r + 1;
            }
        }
        return i;
    }

    return i;
}

fn rlCompileConcat(ctx: *RlCompileCtx, pat: string, pos: usize): usize {
    let i = pos;
    while (ctx.ok && i < pat.len) {
        let b = pat[i];
        if (b == 0x29 || b == 0x7C) { // ')' or '|'
            break;
        }
        i = rlCompileQuant(ctx, pat, i);
    }
    return i;
}

fn rlCompileAlt(ctx: *RlCompileCtx, pat: string, pos: usize): usize {
    let first_start = ctx.inst_len;
    let i = rlCompileConcat(ctx, pat, pos);
    if (!ctx.ok || i >= pat.len || pat[i] != 0x7C) {
        return i;
    }

    // Multiple branches separated by '|'
    let prev_branch_start = first_start;
    while (ctx.ok && i < pat.len && pat[i] == 0x7C) {
        i = i + 1; // skip '|'
        rlInsertSlot(ctx, prev_branch_start);
        let jmp_pc = rlEmit(ctx, RL_OP_JMP, 0, 0);
        if (!ctx.ok) {
            return i;
        }
        let rhs_start = ctx.inst_len;
        ctx.insts[prev_branch_start] = RlInst { op: RL_OP_SPLIT, arg1: prev_branch_start + 1, arg2: rhs_start };
        i = rlCompileConcat(ctx, pat, i);
        ctx.insts[jmp_pc] = RlInst { op: RL_OP_JMP, arg1: ctx.inst_len, arg2: 0 };
        prev_branch_start = rhs_start;
    }
    return i;
}

fn rlIsWordAt(text: string, pos: usize): bool {
    if (pos >= text.len) {
        return false;
    }
    return isWordByte(text[pos]);
}

// ============================================================================
// Public Struct — RegexLite (Thompson NFA VM, max 128 instructions)
// ============================================================================

/// Zero-allocation linear-time Thompson NFA regular expression engine.
pub struct RegexLite {
    insts: [128]RlInst,
    inst_len: usize,
    classes: [16]RlClass,
    class_len: usize,
    prefix: [64]u8,
    prefix_len: usize,
    anchored_start: bool,
    valid: bool,
    err_code: i32,

    /// Compiles `pattern` into a linear-time Thompson NFA.
    /// Validates syntax via `@cap/regex-syntax::validateRegexSyntax` and extracts a static
    /// literal prefix via `extractLiteralPrefix` for `@cap/memchr::memmemFind` acceleration.
    pub static fn compile(pattern: string): RegexLite {
        let info = validateRegexSyntax(pattern);
        let empty = RegexLite {
            insts: [rlEmptyInst(); 128],
            inst_len: 0,
            classes: [rlEmptyClass(); 16],
            class_len: 0,
            prefix: undefined,
            prefix_len: 0,
            anchored_start: false,
            valid: false,
            err_code: info.err_code
        };
        if (!info.valid) {
            return empty;
        }

        let ctx = RlCompileCtx {
            insts: [rlEmptyInst(); 128],
            inst_len: 0,
            classes: [rlEmptyClass(); 16],
            class_len: 0,
            ok: true
        };
        * = rlCompileAlt(&ctx, pattern, 0);
        * = rlEmit(&ctx, RL_OP_MATCH, 0, 0);
        if (!ctx.ok) {
            empty.err_code = -1;
            return empty;
        }

        let mut_res = RegexLite {
            insts: ctx.insts,
            inst_len: ctx.inst_len,
            classes: ctx.classes,
            class_len: ctx.class_len,
            prefix: undefined,
            prefix_len: 0,
            anchored_start: false,
            valid: true,
            err_code: 0
        };
        mut_res.prefix_len = extractLiteralPrefix(pattern, mut_res.prefix[0..RL_MAX_PREFIX]);
        if (mut_res.inst_len > 0 && mut_res.insts[0].op == RL_OP_ANCHOR_START) {
            mut_res.anchored_start = true;
        }
        return mut_res;
    }

    pub fn isOk(): bool {
        return _.valid;
    }

    pub fn errCode(): i32 {
        return _.err_code;
    }

    /// Runs an anchored PikeVM Thompson NFA search starting strictly at `start_pos`.
    fn matchAnchored(text: string, start_pos: usize): RegexMatch {
        let cur_list: [128]usize = undefined;
        let next_list: [128]usize = undefined;
        let visited: [128]usize = [0; 128];
        let stack: [128]usize = undefined;
        let cur_len: usize = 0;
        let gen: usize = 1;

        // Add initial epsilon-closure for pc = 0 at start_pos
        let sp: usize = 1;
        stack[0] = 0;
        while (sp > 0) {
            sp = sp - 1;
            let pc = stack[sp];
            if (pc >= _.inst_len || visited[pc] == gen) {
                continue;
            }
            visited[pc] = gen;
            let inst = _.insts[pc];
            if (inst.op == RL_OP_JMP) {
                stack[sp] = inst.arg1;
                sp = sp + 1;
            } else if (inst.op == RL_OP_SPLIT) {
                // Push arg2 first so arg1 (higher priority) is popped first
                stack[sp] = inst.arg2;
                sp = sp + 1;
                stack[sp] = inst.arg1;
                sp = sp + 1;
            } else if (inst.op == RL_OP_ANCHOR_START) {
                if (start_pos == 0) {
                    stack[sp] = pc + 1;
                    sp = sp + 1;
                }
            } else if (inst.op == RL_OP_ANCHOR_END) {
                if (start_pos == text.len) {
                    stack[sp] = pc + 1;
                    sp = sp + 1;
                }
            } else if (inst.op == RL_OP_WORD_BOUND) {
                let leftW = if (start_pos > 0) rlIsWordAt(text, start_pos - 1) else false;
                let rightW = rlIsWordAt(text, start_pos);
                let isBound = leftW != rightW;
                if ((inst.arg1 == 0 && isBound) || (inst.arg1 == 1 && !isBound)) {
                    stack[sp] = pc + 1;
                    sp = sp + 1;
                }
            } else {
                cur_list[cur_len] = pc;
                cur_len = cur_len + 1;
            }
        }

        let matched = false;
        let best_end: usize = start_pos;
        let pos = start_pos;

        while (cur_len > 0) {
            let next_len: usize = 0;
            gen = gen + 1;

            let ti: usize = 0;
            while (ti < cur_len) {
                let pc = cur_list[ti];
                let inst = _.insts[pc];
                if (inst.op == RL_OP_MATCH) {
                    matched = true;
                    best_end = pos;
                    // Priority cutoff: do not explore lower-priority threads after a match
                    break;
                }

                if (pos < text.len) {
                    let b = text[pos];
                    let ok_char = false;
                    if (inst.op == RL_OP_BYTE) {
                        ok_char = b == (inst.arg1 as u8);
                    } else if (inst.op == RL_OP_ANY) {
                        ok_char = b != 0x0A;
                    } else if (inst.op == RL_OP_CLASS) {
                        ok_char = rlClassTest(_.classes[inst.arg1], b);
                    }

                    if (ok_char) {
                        let next_pos = pos + 1;
                        let nsp: usize = 1;
                        stack[0] = pc + 1;
                        while (nsp > 0) {
                            nsp = nsp - 1;
                            let npc = stack[nsp];
                            if (npc >= _.inst_len || visited[npc] == gen) {
                                continue;
                            }
                            visited[npc] = gen;
                            let ninst = _.insts[npc];
                            if (ninst.op == RL_OP_JMP) {
                                stack[nsp] = ninst.arg1;
                                nsp = nsp + 1;
                            } else if (ninst.op == RL_OP_SPLIT) {
                                stack[nsp] = ninst.arg2;
                                nsp = nsp + 1;
                                stack[nsp] = ninst.arg1;
                                nsp = nsp + 1;
                            } else if (ninst.op == RL_OP_ANCHOR_START) {
                                if (next_pos == 0) {
                                    stack[nsp] = npc + 1;
                                    nsp = nsp + 1;
                                }
                            } else if (ninst.op == RL_OP_ANCHOR_END) {
                                if (next_pos == text.len) {
                                    stack[nsp] = npc + 1;
                                    nsp = nsp + 1;
                                }
                            } else if (ninst.op == RL_OP_WORD_BOUND) {
                                let leftW = if (next_pos > 0) rlIsWordAt(text, next_pos - 1) else false;
                                let rightW = rlIsWordAt(text, next_pos);
                                let isBound = leftW != rightW;
                                if ((ninst.arg1 == 0 && isBound) || (ninst.arg1 == 1 && !isBound)) {
                                    stack[nsp] = npc + 1;
                                    nsp = nsp + 1;
                                }
                            } else {
                                next_list[next_len] = npc;
                                next_len = next_len + 1;
                            }
                        }
                    }
                }
                ti = ti + 1;
            }

            if (next_len == 0) {
                break;
            }
            let k: usize = 0;
            while (k < next_len) {
                cur_list[k] = next_list[k];
                k = k + 1;
            }
            cur_len = next_len;
            pos = pos + 1;
        }

        if (matched) {
            return RegexMatch { matched: true, start: start_pos, end: best_end };
        }
        return rlNoMatch();
    }

    /// Finds the first match in `text` starting at or after `start_pos`.
    /// Uses `@cap/memchr::memmemFind` to jump directly to literal-prefix candidates when available.
    pub fn findAt(text: string, start_pos: usize): RegexMatch {
        if (!_.valid || start_pos > text.len) {
            return rlNoMatch();
        }
        if (_.anchored_start) {
            if (start_pos > 0) {
                return rlNoMatch();
            }
            return _.matchAnchored(text, 0);
        }

        let s = start_pos;
        if (_.prefix_len > 0) {
            while (s <= text.len) {
                let hit = memmemFind(text[s..text.len], _.prefix[0.._.prefix_len]);
                if (hit < 0) {
                    return rlNoMatch();
                }
                let cand = s + (hit as usize);
                let m = _.matchAnchored(text, cand);
                if (m.matched) {
                    return m;
                }
                s = cand + 1;
            }
            return rlNoMatch();
        }

        while (s <= text.len) {
            let m = _.matchAnchored(text, s);
            if (m.matched) {
                return m;
            }
            s = s + 1;
        }
        return rlNoMatch();
    }

    /// Finds the first match anywhere in `text`.
    pub fn find(text: string): RegexMatch {
        return _.findAt(text, 0);
    }

    /// Returns `true` if `text` contains a match.
    pub fn isMatch(text: string): bool {
        return _.findAt(text, 0).matched;
    }

    /// Finds all non-overlapping matches in `text`, writing them into `out_matches`.
    /// Returns the number of matches written.
    pub fn findAll(text: string, out_matches: []RegexMatch): usize {
        if (!_.valid || out_matches.len == 0) {
            return 0;
        }
        let found: usize = 0;
        let pos: usize = 0;
        while (pos <= text.len && found < out_matches.len) {
            let m = _.findAt(text, pos);
            if (!m.matched) {
                break;
            }
            out_matches[found] = m;
            found = found + 1;
            if (m.end > m.start) {
                pos = m.end;
            } else {
                pos = m.end + 1;
            }
        }
        return found;
    }

    /// Replaces all non-overlapping matches in `text` with `replacement`, writing into `out`.
    /// Returns the number of bytes written to `out`.
    pub fn replaceAll(text: string, replacement: string, out: []u8): usize {
        if (out.len == 0) {
            return 0;
        }
        if (!_.valid) {
            return writeStr(text, out, 0);
        }
        let written: usize = 0;
        let last_end: usize = 0;
        let search_pos: usize = 0;
        while (search_pos <= text.len && written < out.len) {
            let m = _.findAt(text, search_pos);
            if (!m.matched) {
                written = writeStr(text[last_end..text.len], out, written);
                break;
            }
            if (m.start > last_end) {
                written = writeStr(text[last_end..m.start], out, written);
            }
            if (replacement.len > 0 && written < out.len) {
                written = writeStr(replacement, out, written);
            }
            last_end = m.end;
            if (m.end > m.start) {
                search_pos = m.end;
            } else {
                if (m.end < text.len && written < out.len) {
                    out[written] = text[m.end];
                    written = written + 1;
                    last_end = m.end + 1;
                }
                search_pos = m.end + 1;
            }
        }
        return written;
    }

    /// Counts the number of substrings produced by splitting `text` on all non-overlapping matches.
    pub fn splitCount(text: string): usize {
        if (!_.valid) {
            return 1;
        }
        let parts: usize = 1;
        let pos: usize = 0;
        while (pos <= text.len) {
            let m = _.findAt(text, pos);
            if (!m.matched) {
                break;
            }
            parts = parts + 1;
            if (m.end > m.start) {
                pos = m.end;
            } else {
                pos = m.end + 1;
            }
        }
        return parts;
    }
}

export {
    RL_MAX_INSTS,
    RegexMatch,
    RegexLite
};

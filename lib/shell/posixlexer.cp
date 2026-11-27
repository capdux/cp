// Source: cp/lib/shell_parser/lexer.cp (pure CP POSIX shell lexer)
const std = @import("std");
const Allocator = std.mem.Allocator;

pub const ShellError = error{
    UnterminatedQuote,
    TrailingEscape,
    UnexpectedToken,
    OutOfMemory,
};

pub const TokenKind = enum {
    Word,
    Pipe,
    Or,
    And,
    Semi,
    Background,
    RedirectOut,
    RedirectAppend,
    RedirectErrOut,
    RedirectErrAppend,
    RedirectIn,
    RedirectErrToOut,
    RedirectBothOut,
};

pub const ShellToken = struct {
    kind: TokenKind,
    value: []const u8,
    quoted: bool,
    has_var: bool,
    start: usize,
    end: usize,

    pub fn deinit(*_, alloc: Allocator) {
        if (_.value.len > 0) {
            alloc.free(_.value);
        }
    }
};

pub fn deinitTokens(alloc: Allocator, tokens: []ShellToken) {
    for (tok of tokens) {
        if (tok.value.len > 0) {
            alloc.free(tok.value);
        }
    }
    if (tokens.len > 0) {
        alloc.free(tokens);
    }
}

type ShellLexVec16 = @Vector(16, u8);

const SH_WS: u8 = 1;
const SH_OP: u8 = 2;
const SH_VAR_START: u8 = 4;
const SH_VAR_CHAR: u8 = 8;
const SH_WORD_SPECIAL: u8 = 16;

fn makeShellCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i == ' ' || i == '\t' || i == '\r' || i == '\n') {
            lut[i] |= SH_WS | SH_WORD_SPECIAL;
        }
        if (i == '|' || i == '&' || i == ';' || i == '>' || i == '<') {
            lut[i] |= SH_OP | SH_WORD_SPECIAL;
        }
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_') {
            lut[i] |= SH_VAR_START | SH_VAR_CHAR;
        }
        if (i >= '0' && i <= '9') {
            lut[i] |= SH_VAR_CHAR;
        }
        if (i == '\'' || i == '"' || i == '\\' || i == '$') {
            lut[i] |= SH_WORD_SPECIAL;
        }
    }
    return lut;
}

const SHELL_CHAR_LUT: [u8; 256] = makeShellCharLut();

fn isWhitespace(c: u8): bool {
    return (SHELL_CHAR_LUT[usize(c)] & SH_WS) != 0;
}

fn isOperatorStart(c: u8): bool {
    return (SHELL_CHAR_LUT[usize(c)] & SH_OP) != 0;
}

fn isVarStart(c: u8): bool {
    return (SHELL_CHAR_LUT[usize(c)] & SH_VAR_START) != 0;
}

fn isVarChar(c: u8): bool {
    return (SHELL_CHAR_LUT[usize(c)] & SH_VAR_CHAR) != 0;
}

fn findByteSimd(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_needle: ShellLexVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ShellLexVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn findDoubleQuoteSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_dq: ShellLexVec16 = @splat('"' as u8);
    const v_bs: ShellLexVec16 = @splat('\\' as u8);
    const v_dl: ShellLexVec16 = @splat('$' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ShellLexVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((chunk == v_dq) | (chunk == v_bs) | (chunk == v_dl));
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '"' or c == '\\' or c == '$') return i;
    }
    return s.len;
}

fn findWordSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_sp: ShellLexVec16 = @splat(' ' as u8);
    const v_tb: ShellLexVec16 = @splat('\t' as u8);
    const v_cr: ShellLexVec16 = @splat('\r' as u8);
    const v_lf: ShellLexVec16 = @splat('\n' as u8);
    const v_pipe: ShellLexVec16 = @splat('|' as u8);
    const v_amp: ShellLexVec16 = @splat('&' as u8);
    const v_semi: ShellLexVec16 = @splat(';' as u8);
    const v_gt: ShellLexVec16 = @splat('>' as u8);
    const v_lt: ShellLexVec16 = @splat('<' as u8);
    const v_sq: ShellLexVec16 = @splat('\'' as u8);
    const v_dq: ShellLexVec16 = @splat('"' as u8);
    const v_bs: ShellLexVec16 = @splat('\\' as u8);
    const v_dl: ShellLexVec16 = @splat('$' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: ShellLexVec16 = s[i .. i + 16][0..16].*;
        const hit = (chunk == v_sp) | (chunk == v_tb) | (chunk == v_cr) | (chunk == v_lf) |
            (chunk == v_pipe) | (chunk == v_amp) | (chunk == v_semi) | (chunk == v_gt) | (chunk == v_lt) |
            (chunk == v_sq) | (chunk == v_dq) | (chunk == v_bs) | (chunk == v_dl);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if ((SHELL_CHAR_LUT[usize(s[i])] & SH_WORD_SPECIAL) != 0) return i;
    }
    return s.len;
}

fn startsWithAt(input: []const u8, idx: usize, prefix: []const u8): bool {
    if (idx + prefix.len > input.len) {
        return false;
    }
    return std.mem.eql(u8, input[idx .. idx + prefix.len], prefix);
}

fn detectVar(input: []const u8, idx: usize): bool {
    if (idx + 1 >= input.len) {
        return false;
    }
    const next = input[idx + 1];
    if (next == '{') {
        let j = idx + 2;
        if (j >= input.len or !isVarStart(input[j])) {
            return false;
        }
        j += 1;
        while (j < input.len and isVarChar(input[j])) {
            j += 1;
        }
        return j < input.len and input[j] == '}';
    }
    return isVarStart(next);
}

pub fn tokenize(alloc: Allocator, input: []const u8): ShellError![]ShellToken {
    let tokens = std.ArrayList(ShellToken).empty;
    errdefer {
        for (*tok of tokens.items) {
            tok.deinit(alloc);
        }
        tokens.deinit(alloc);
    }

    let i: usize = 0;
    while (i < input.len) {
        while (i < input.len and isWhitespace(input[i])) {
            i += 1;
        }
        if (i >= input.len) {
            break;
        }

        // Skip # comments until newline
        if (input[i] == '#') {
            i = findByteSimd(input, i + 1, '\n');
            continue;
        }

        // Line continuation outside quotes: \<newline>
        if (input[i] == '\\') {
            if (i + 1 < input.len and input[i + 1] == '\n') {
                i += 2;
                continue;
            }
            if (i + 2 < input.len and input[i + 1] == '\r' and input[i + 2] == '\n') {
                i += 3;
                continue;
            }
        }

        // Fd-prefixed stderr redirects at token boundary
        if (startsWithAt(input, i, "2>&1")) {
            const val = try alloc.dupe(u8, "2>&1");
            errdefer alloc.free(val);
            try tokens.append(alloc, ShellToken{
                .kind = .RedirectErrToOut,
                .value = val,
                .quoted = false,
                .has_var = false,
                .start = i,
                .end = i + 4,
            });
            i += 4;
            continue;
        }
        if (startsWithAt(input, i, "2>>")) {
            const val = try alloc.dupe(u8, "2>>");
            errdefer alloc.free(val);
            try tokens.append(alloc, ShellToken{
                .kind = .RedirectErrAppend,
                .value = val,
                .quoted = false,
                .has_var = false,
                .start = i,
                .end = i + 3,
            });
            i += 3;
            continue;
        }
        if (startsWithAt(input, i, "2>")) {
            const val = try alloc.dupe(u8, "2>");
            errdefer alloc.free(val);
            try tokens.append(alloc, ShellToken{
                .kind = .RedirectErrOut,
                .value = val,
                .quoted = false,
                .has_var = false,
                .start = i,
                .end = i + 2,
            });
            i += 2;
            continue;
        }

        // Control and redirect operators
        if (isOperatorStart(input[i])) {
            const start = i;
            let kind: TokenKind = .Pipe;
            let op_len: usize = 1;

            if (startsWithAt(input, i, "||")) {
                kind = .Or;
                op_len = 2;
            } else if (input[i] == '|') {
                kind = .Pipe;
                op_len = 1;
            } else if (startsWithAt(input, i, "&&")) {
                kind = .And;
                op_len = 2;
            } else if (startsWithAt(input, i, "&>")) {
                kind = .RedirectBothOut;
                op_len = 2;
            } else if (input[i] == '&') {
                kind = .Background;
                op_len = 1;
            } else if (input[i] == ';') {
                kind = .Semi;
                op_len = 1;
            } else if (startsWithAt(input, i, ">>")) {
                kind = .RedirectAppend;
                op_len = 2;
            } else if (input[i] == '>') {
                kind = .RedirectOut;
                op_len = 1;
            } else if (input[i] == '<') {
                kind = .RedirectIn;
                op_len = 1;
            }

            const val = try alloc.dupe(u8, input[start .. start + op_len]);
            errdefer alloc.free(val);
            try tokens.append(alloc, ShellToken{
                .kind = kind,
                .value = val,
                .quoted = false,
                .has_var = false,
                .start = start,
                .end = start + op_len,
            });
            i += op_len;
            continue;
        }

        // Word token (may combine unquoted, single-quoted, and double-quoted segments)
        const start = i;
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(alloc);
        let quoted = false;
        let has_var = false;

        while (i < input.len) {
            const ch = input[i];
            if (isWhitespace(ch) or isOperatorStart(ch)) {
                break;
            }

            if (ch == '\'') {
                quoted = true;
                i += 1;
                const sq_end = findByteSimd(input, i, '\'');
                if (sq_end >= input.len) {
                    return error.UnterminatedQuote;
                }
                if (sq_end > i) {
                    try buf.appendSlice(alloc, input[i..sq_end]);
                }
                i = sq_end + 1;
                continue;
            }

            if (ch == '"') {
                quoted = true;
                i += 1;
                let closed = false;
                while (i < input.len) {
                    const hit = findDoubleQuoteSpecialSimd(input, i);
                    if (hit > i) {
                        try buf.appendSlice(alloc, input[i..hit]);
                        i = hit;
                    }
                    if (i >= input.len) break;
                    const qc = input[i];
                    if (qc == '"') {
                        closed = true;
                        i += 1;
                        break;
                    }
                    if (qc == '\\') {
                        if (i + 1 >= input.len) {
                            return error.UnterminatedQuote;
                        }
                        const esc = input[i + 1];
                        if (esc == '\n') {
                            i += 2;
                            continue;
                        }
                        if (esc == '\r' and i + 2 < input.len and input[i + 2] == '\n') {
                            i += 3;
                            continue;
                        }
                        if (esc == '"' or esc == '\\' or esc == '$' or esc == '`') {
                            try buf.append(alloc, esc);
                            i += 2;
                            continue;
                        }
                        try buf.append(alloc, '\\');
                        try buf.append(alloc, esc);
                        i += 2;
                        continue;
                    }
                    if (qc == '$' and detectVar(input, i)) {
                        has_var = true;
                    }
                    try buf.append(alloc, qc);
                    i += 1;
                }
                if (!closed) {
                    return error.UnterminatedQuote;
                }
                continue;
            }

            if (ch == '\\') {
                if (i + 1 >= input.len) {
                    return error.TrailingEscape;
                }
                const esc = input[i + 1];
                if (esc == '\n') {
                    i += 2;
                    continue;
                }
                if (esc == '\r' and i + 2 < input.len and input[i + 2] == '\n') {
                    i += 3;
                    continue;
                }
                try buf.append(alloc, esc);
                i += 2;
                continue;
            }

            if (ch == '$') {
                if (detectVar(input, i)) {
                    has_var = true;
                }
                try buf.append(alloc, ch);
                i += 1;
                continue;
            }

            const plain_end = findWordSpecialSimd(input, i);
            if (plain_end > i) {
                try buf.appendSlice(alloc, input[i..plain_end]);
                i = plain_end;
            } else {
                try buf.append(alloc, ch);
                i += 1;
            }
        }

        const word_slice = try buf.toOwnedSlice(alloc);
        errdefer alloc.free(word_slice);
        try tokens.append(alloc, ShellToken{
            .kind = .Word,
            .value = word_slice,
            .quoted = quoted,
            .has_var = has_var,
            .start = start,
            .end = i,
        });
    }

    return try tokens.toOwnedSlice(alloc);
}

// @cap/js-tokens — Zero-allocation JavaScript/TypeScript lexer in pure CP
// Clean-room MIT implementation of npm `js-tokens` (#61, 381M/wk).
// Accelerated with comptime 256-entry character-class LUT and 16-lane @Vector(16, u8) SIMD scanning.

type JstVec16 = @Vector(16, u8);

const JST_CLS_IDENT_START: u8 = 1;
const JST_CLS_IDENT_PART: u8 = 2;
const JST_CLS_DIGIT: u8 = 4;
const JST_CLS_HEX: u8 = 8;
const JST_CLS_SPACE: u8 = 16;
const JST_CLS_OCTAL: u8 = 32;

fn jstBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        const is_alpha = (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
        const is_digit = c >= 48 && c <= 57;
        if (is_alpha || c == 95 || c == 36 || c >= 128) {
            f |= JST_CLS_IDENT_START | JST_CLS_IDENT_PART;
        }
        if (is_digit) {
            f |= JST_CLS_IDENT_PART | JST_CLS_DIGIT | JST_CLS_HEX;
        }
        if ((c >= 65 && c <= 70) || (c >= 97 && c <= 102)) {
            f |= JST_CLS_HEX;
        }
        if (c == 32 || c == 9 || c == 11 || c == 12) {
            f |= JST_CLS_SPACE;
        }
        if (c >= 48 && c <= 55) {
            f |= JST_CLS_OCTAL;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const JST_CHAR_LUT: [256]u8 = jstBuildCharLut();

pub enum JsTokenKind {
    IdentifierName,
    NumericLiteral,
    StringLiteral,
    NoSubstitutionTemplate,
    TemplateHead,
    Punctuator,
    SingleLineComment,
    MultiLineComment,
    WhiteSpace,
    LineTerminatorSequence,
    Invalid,
}

pub struct JsToken {
    pub kind: JsTokenKind,
    pub start: usize,
    pub end: usize,
    pub closed: bool,

    pub fn text(src: string): string {
        return src[_.start.._.end];
    }
}

fn jstIsIdentStart(b: u8): bool {
    return (JST_CHAR_LUT[b as usize] & JST_CLS_IDENT_START) != 0;
}

fn jstIsIdentPart(b: u8): bool {
    return (JST_CHAR_LUT[b as usize] & JST_CLS_IDENT_PART) != 0;
}

fn jstIsDigit(b: u8): bool {
    return (JST_CHAR_LUT[b as usize] & JST_CLS_DIGIT) != 0;
}

fn jstIsHex(b: u8): bool {
    return (JST_CHAR_LUT[b as usize] & JST_CLS_HEX) != 0;
}

fn jstIsSpace(b: u8): bool {
    return (JST_CHAR_LUT[b as usize] & JST_CLS_SPACE) != 0;
}

fn jstFindLineEnd(src: string, start: usize): usize {
    let i = start;
    const len = src.len;
    const v_lf: JstVec16 = @splat(10);
    const v_cr: JstVec16 = @splat(13);
    while (i + 16 <= len) {
        const v: JstVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == 10 || c == 13) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && src[i] != 10 && src[i] != 13) {
        i += 1;
    }
    return i;
}

fn jstFindStar(src: string, start: usize): usize {
    let i = start;
    const len = src.len;
    const v_star: JstVec16 = @splat(42);
    while (i + 16 <= len) {
        const v: JstVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_star)) {
            let k: usize = 0;
            while (k < 16) {
                if (src[i + k] == 42) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && src[i] != 42) {
        i += 1;
    }
    return i;
}

fn jstFindStringSpecial(src: string, start: usize, quote: u8): usize {
    let i = start;
    const len = src.len;
    const v_q: JstVec16 = @splat(quote);
    const v_bs: JstVec16 = @splat(92);
    const v_lf: JstVec16 = @splat(10);
    const v_cr: JstVec16 = @splat(13);
    while (i + 16 <= len) {
        const v: JstVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_q) || @reduce(.Or, v == v_bs) || @reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == quote || c == 92 || c == 10 || c == 13) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = src[i];
        if (c == quote || c == 92 || c == 10 || c == 13) { return i; }
        i += 1;
    }
    return len;
}

fn jstFindTemplateSpecial(src: string, start: usize): usize {
    let i = start;
    const len = src.len;
    const v_bt: JstVec16 = @splat(96);
    const v_bs: JstVec16 = @splat(92);
    const v_dl: JstVec16 = @splat(36);
    while (i + 16 <= len) {
        const v: JstVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_bt) || @reduce(.Or, v == v_bs) || @reduce(.Or, v == v_dl)) {
            let k: usize = 0;
            while (k < 16) {
                const c = src[i + k];
                if (c == 96 || c == 92 || c == 36) { return i + k; }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const c = src[i];
        if (c == 96 || c == 92 || c == 36) { return i; }
        i += 1;
    }
    return len;
}

fn jstSkipSpaces(src: string, start: usize): usize {
    let i = start;
    const len = src.len;
    const v_sp: JstVec16 = @splat(32);
    while (i + 16 <= len) {
        const v: JstVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.And, v == v_sp)) {
            i += 16;
        } else {
            break;
        }
    }
    while (i < len && jstIsSpace(src[i])) {
        i += 1;
    }
    return i;
}

fn jstStartsWith(src: string, pos: usize, lit: string): bool {
    if (pos + lit.len > src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (src[pos + i] != lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Scans the next JavaScript token starting at `pos`. Returns `null` at EOF.
pub fn nextJsToken(src: string, pos: usize): JsToken? {
    if (pos >= src.len) {
        return null;
    }
    const b = src[pos];

    // 1. LineTerminatorSequence (\r\n, \n, \r)
    if (b == 10) {
        return JsToken { kind: .LineTerminatorSequence, start: pos, end: pos + 1, closed: true };
    }
    if (b == 13) {
        let end = pos + 1;
        if (end < src.len && src[end] == 10) {
            end += 1;
        }
        return JsToken { kind: .LineTerminatorSequence, start: pos, end: end, closed: true };
    }

    // 2. WhiteSpace
    if (jstIsSpace(b)) {
        const end = jstSkipSpaces(src, pos + 1);
        return JsToken { kind: .WhiteSpace, start: pos, end: end, closed: true };
    }

    // 3. Comments: // ... and /* ... */
    if (b == 47 && pos + 1 < src.len) {
        if (src[pos + 1] == 47) {
            const end = jstFindLineEnd(src, pos + 2);
            return JsToken { kind: .SingleLineComment, start: pos, end: end, closed: true };
        }
        if (src[pos + 1] == 42) {
            let end = pos + 2;
            let closed = false;
            while (end + 1 < src.len) {
                end = jstFindStar(src, end);
                if (end + 1 >= src.len) {
                    break;
                }
                if (src[end + 1] == 47) {
                    end += 2;
                    closed = true;
                    break;
                }
                end += 1;
            }
            if (!closed) {
                end = src.len;
            }
            return JsToken { kind: .MultiLineComment, start: pos, end: end, closed: closed };
        }
    }

    // 4. StringLiteral ('...' or "...")
    if (b == 34 || b == 39) {
        let end = pos + 1;
        let closed = false;
        while (end < src.len) {
            end = jstFindStringSpecial(src, end, b);
            if (end >= src.len) {
                break;
            }
            const c = src[end];
            if (c == 92 && end + 1 < src.len) { // escape
                end += 2;
                continue;
            }
            if (c == b) {
                end += 1;
                closed = true;
                break;
            }
            if (c == 10 || c == 13) {
                break;
            }
            end += 1;
        }
        return JsToken { kind: .StringLiteral, start: pos, end: end, closed: closed };
    }

    // 5. Template literal (`...` or `...${)
    if (b == 96) {
        let end = pos + 1;
        while (end < src.len) {
            end = jstFindTemplateSpecial(src, end);
            if (end >= src.len) {
                break;
            }
            const c = src[end];
            if (c == 92 && end + 1 < src.len) {
                end += 2;
                continue;
            }
            if (c == 96) {
                return JsToken { kind: .NoSubstitutionTemplate, start: pos, end: end + 1, closed: true };
            }
            if (c == 36 && end + 1 < src.len && src[end + 1] == 123) {
                return JsToken { kind: .TemplateHead, start: pos, end: end + 2, closed: true };
            }
            end += 1;
        }
        return JsToken { kind: .NoSubstitutionTemplate, start: pos, end: src.len, closed: false };
    }

    // 6. NumericLiteral (0x.., 0b.., 0o.., decimal, float, exponent, bigint n)
    if (jstIsDigit(b) || (b == 46 && pos + 1 < src.len && jstIsDigit(src[pos + 1]))) {
        let end = pos;
        if (b == 48 && pos + 1 < src.len && (src[pos + 1] == 120 || src[pos + 1] == 88)) {
            end = pos + 2;
            while (end < src.len && (jstIsHex(src[end]) || src[end] == 95)) {
                end += 1;
            }
        } else if (b == 48 && pos + 1 < src.len && (src[pos + 1] == 98 || src[pos + 1] == 66)) {
            end = pos + 2;
            while (end < src.len && (src[end] == 48 || src[end] == 49 || src[end] == 95)) {
                end += 1;
            }
        } else if (b == 48 && pos + 1 < src.len && (src[pos + 1] == 111 || src[pos + 1] == 79)) {
            end = pos + 2;
            while (end < src.len && ((JST_CHAR_LUT[src[end] as usize] & JST_CLS_OCTAL) != 0 || src[end] == 95)) {
                end += 1;
            }
        } else {
            while (end < src.len && (jstIsDigit(src[end]) || src[end] == 95)) {
                end += 1;
            }
            if (end < src.len && src[end] == 46) {
                end += 1;
                while (end < src.len && (jstIsDigit(src[end]) || src[end] == 95)) {
                    end += 1;
                }
            }
            if (end < src.len && (src[end] == 101 || src[end] == 69)) {
                end += 1;
                if (end < src.len && (src[end] == 43 || src[end] == 45)) {
                    end += 1;
                }
                while (end < src.len && (jstIsDigit(src[end]) || src[end] == 95)) {
                    end += 1;
                }
            }
        }
        if (end < src.len && src[end] == 110) { // BigInt suffix 'n'
            end += 1;
        }
        return JsToken { kind: .NumericLiteral, start: pos, end: end, closed: true };
    }

    // 7. IdentifierName
    if (jstIsIdentStart(b)) {
        let end = pos + 1;
        while (end < src.len && jstIsIdentPart(src[end])) {
            end += 1;
        }
        return JsToken { kind: .IdentifierName, start: pos, end: end, closed: true };
    }

    // 8. Multi-character Punctuators (longest match)
    if (jstStartsWith(src, pos, ">>>=")) {
        return JsToken { kind: .Punctuator, start: pos, end: pos + 4, closed: true };
    }
    if (jstStartsWith(src, pos, "===") || jstStartsWith(src, pos, "!==") || jstStartsWith(src, pos, ">>>")
        || jstStartsWith(src, pos, "<<=") || jstStartsWith(src, pos, ">>=") || jstStartsWith(src, pos, "**=")
        || jstStartsWith(src, pos, "&&=") || jstStartsWith(src, pos, "||=") || jstStartsWith(src, pos, "??=")
        || jstStartsWith(src, pos, "...")) {
        return JsToken { kind: .Punctuator, start: pos, end: pos + 3, closed: true };
    }
    if (jstStartsWith(src, pos, "=>") || jstStartsWith(src, pos, "==") || jstStartsWith(src, pos, "!=")
        || jstStartsWith(src, pos, "<=") || jstStartsWith(src, pos, ">=") || jstStartsWith(src, pos, "&&")
        || jstStartsWith(src, pos, "||") || jstStartsWith(src, pos, "??") || jstStartsWith(src, pos, "?.")
        || jstStartsWith(src, pos, "++") || jstStartsWith(src, pos, "--") || jstStartsWith(src, pos, "**")
        || jstStartsWith(src, pos, "<<") || jstStartsWith(src, pos, ">>") || jstStartsWith(src, pos, "+=")
        || jstStartsWith(src, pos, "-=") || jstStartsWith(src, pos, "*=") || jstStartsWith(src, pos, "/=")
        || jstStartsWith(src, pos, "%=") || jstStartsWith(src, pos, "&=") || jstStartsWith(src, pos, "|=")
        || jstStartsWith(src, pos, "^=")) {
        return JsToken { kind: .Punctuator, start: pos, end: pos + 2, closed: true };
    }

    // 9. Single-character Punctuator
    return JsToken { kind: .Punctuator, start: pos, end: pos + 1, closed: true };
}

/// Tokenizes `src` into `out`, returning the number of tokens written (stops cleanly when `out` is full).
pub fn tokenizeJs(src: string, out: []JsToken): usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < src.len && count < out.len) {
        if (tok := nextJsToken(src, pos)) {
            out[count] = tok;
            count += 1;
            if (tok.end <= pos) {
                break;
            }
            pos = tok.end;
        } else {
            break;
        }
    }
    return count;
}

/// Counts total tokens in `src` without allocating storage.
pub fn countJsTokens(src: string): usize {
    let pos: usize = 0;
    let total: usize = 0;
    while (pos < src.len) {
        if (tok := nextJsToken(src, pos)) {
            total += 1;
            if (tok.end <= pos) {
                break;
            }
            pos = tok.end;
        } else {
            break;
        }
    }
    return total;
}

export {
    JsTokenKind,
    JsToken,
    nextJsToken,
    tokenizeJs,
    countJsTokens,
};

// @cap/js-tokens — Zero-allocation JavaScript/TypeScript lexer in pure CP
// Clean-room MIT implementation of npm `js-tokens` (#61, 381M/wk).

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
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122) || b == 95 || b == 36 || b >= 128;
}

fn jstIsIdentPart(b: u8): bool {
    return jstIsIdentStart(b) || (b >= 48 && b <= 57);
}

fn jstIsDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

fn jstIsHex(b: u8): bool {
    return jstIsDigit(b) || (b >= 65 && b <= 70) || (b >= 97 && b <= 102);
}

fn jstIsSpace(b: u8): bool {
    return b == 32 || b == 9 || b == 11 || b == 12;
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
        let end = pos + 1;
        while (end < src.len && jstIsSpace(src[end])) {
            end += 1;
        }
        return JsToken { kind: .WhiteSpace, start: pos, end: end, closed: true };
    }

    // 3. Comments: // ... and /* ... */
    if (b == 47 && pos + 1 < src.len) {
        if (src[pos + 1] == 47) {
            let end = pos + 2;
            while (end < src.len && src[end] != 10 && src[end] != 13) {
                end += 1;
            }
            return JsToken { kind: .SingleLineComment, start: pos, end: end, closed: true };
        }
        if (src[pos + 1] == 42) {
            let end = pos + 2;
            let closed = false;
            while (end + 1 < src.len) {
                if (src[end] == 42 && src[end + 1] == 47) {
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
            while (end < src.len && ((src[end] >= 48 && src[end] <= 55) || src[end] == 95)) {
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

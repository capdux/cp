// @cap/combine — Zero-copy streaming parser combinators with backtracking cursor
// Parity with Marwes/combine v4 (crates.io:combine, MIT)

pub enum CharPred: u8 {
    Alpha = 0,
    Digit = 1,
    AlphaNum = 2,
    Space = 3,
    HexDigit = 4,
    Ident = 5,
    NonSpace = 6,
}

pub struct CursorMark {
    pos: usize;
    line: usize;
    col: usize;
}

pub struct ParseCursor {
    input: string;
    pos: usize;
    line: usize;
    col: usize;

    static fn new(input: string): ParseCursor {
        return ParseCursor{
            input: input,
            pos: 0,
            line: 1,
            col: 1,
        };
    }

    fn remaining(): string {
        return _.input[_.pos.._.input.len];
    }

    fn isEof(): bool {
        return _.pos >= _.input.len;
    }

    fn peek(): u8? {
        if (_.pos >= _.input.len) {
            return null;
        }
        return _.input[_.pos];
    }

    fn advance(*_): u8? {
        if (_.pos >= _.input.len) {
            return null;
        }
        const ch = _.input[_.pos];
        _.pos += 1;
        if (ch == ('\n' as u8)) {
            _.line += 1;
            _.col = 1;
        } else {
            _.col += 1;
        }
        return ch;
    }

    fn save(): CursorMark {
        return CursorMark{
            pos: _.pos,
            line: _.line,
            col: _.col,
        };
    }

    fn restore(*_, mark: CursorMark) {
        _.pos = mark.pos;
        _.line = mark.line;
        _.col = mark.col;
    }
}

fn cmbIsAlpha(ch: u8): bool {
    return (ch >= ('a' as u8) && ch <= ('z' as u8)) || (ch >= ('A' as u8) && ch <= ('Z' as u8));
}

fn cmbIsDigit(ch: u8): bool {
    return ch >= ('0' as u8) && ch <= ('9' as u8);
}

fn cmbIsSpace(ch: u8): bool {
    return ch == (' ' as u8) || ch == ('\t' as u8) || ch == ('\r' as u8) || ch == ('\n' as u8);
}

fn cmbIsHex(ch: u8): bool {
    return cmbIsDigit(ch) || (ch >= ('a' as u8) && ch <= ('f' as u8)) || (ch >= ('A' as u8) && ch <= ('F' as u8));
}

fn cmbMatchPred(ch: u8, kind: CharPred): bool {
    if (kind == CharPred.Alpha) {
        return cmbIsAlpha(ch);
    }
    if (kind == CharPred.Digit) {
        return cmbIsDigit(ch);
    }
    if (kind == CharPred.AlphaNum) {
        return cmbIsAlpha(ch) || cmbIsDigit(ch);
    }
    if (kind == CharPred.Space) {
        return cmbIsSpace(ch);
    }
    if (kind == CharPred.HexDigit) {
        return cmbIsHex(ch);
    }
    if (kind == CharPred.Ident) {
        return cmbIsAlpha(ch) || cmbIsDigit(ch) || ch == ('_' as u8);
    }
    return !cmbIsSpace(ch);
}

fn cmbStartsWithAt(hay: string, pos: usize, needle: string): bool {
    if (pos + needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (hay[pos + i] != needle[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Matches exact literal string `lit` at cursor position. Advances on match; untouched on failure.
pub fn tag(cursor: *ParseCursor, lit: string): bool {
    if (!cmbStartsWithAt(cursor.input, cursor.pos, lit)) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        * = cursor.advance();
        i += 1;
    }
    return true;
}

/// Matches a single expected byte. Advances on match; untouched on failure.
pub fn charByte(cursor: *ParseCursor, expected: u8): bool {
    if (ch := cursor.peek()) {
        if (ch == expected) {
            * = cursor.advance();
            return true;
        }
    }
    return false;
}

/// Consumes zero or more bytes matching `pred_kind` and returns the zero-copy slice.
pub fn takeWhile(cursor: *ParseCursor, pred_kind: CharPred): string {
    const start = cursor.pos;
    while (ch := cursor.peek()) {
        if (!cmbMatchPred(ch, pred_kind)) {
            break;
        }
        * = cursor.advance();
    }
    return cursor.input[start..cursor.pos];
}

/// Consumes one or more bytes matching `pred_kind`, or returns `null` if none matched.
pub fn takeWhile1(cursor: *ParseCursor, pred_kind: CharPred): string? {
    const start = cursor.pos;
    const s = takeWhile(cursor, pred_kind);
    if (cursor.pos == start) {
        return null;
    }
    return s;
}

/// Consumes bytes until `delim` is encountered (not consuming `delim`).
/// Returns `null` and restores cursor if `delim` is not found.
pub fn takeUntil(cursor: *ParseCursor, delim: string): string? {
    if (delim.len == 0) {
        return "";
    }
    const mark = cursor.save();
    const start = cursor.pos;
    while (!cursor.isEof()) {
        if (cmbStartsWithAt(cursor.input, cursor.pos, delim)) {
            return cursor.input[start..cursor.pos];
        }
        * = cursor.advance();
    }
    cursor.restore(mark);
    return null;
}

/// Skips ASCII whitespace (` `, `\t`, `\r`, `\n`) and returns the count skipped.
pub fn skipSpaces(cursor: *ParseCursor): usize {
    const start = cursor.pos;
    * = takeWhile(cursor, CharPred.Space);
    return cursor.pos - start;
}

/// Parses an unsigned base-10 64-bit integer. Restores cursor and returns `null` on failure/overflow.
pub fn parseU64(cursor: *ParseCursor): u64? {
    const mark = cursor.save();
    let acc: u64 = 0;
    let digits: usize = 0;
    const maxDiv10: u64 = ((0x19999999 as u64) << 32) | (0x99999999 as u64);
    while (ch := cursor.peek()) {
        if (!cmbIsDigit(ch)) {
            break;
        }
        const d: u64 = (ch - ('0' as u8)) as u64;
        if (acc > maxDiv10 || (acc == maxDiv10 && d > (5 as u64))) {
            cursor.restore(mark);
            return null;
        }
        acc = acc * (10 as u64) + d;
        * = cursor.advance();
        digits += 1;
    }
    if (digits == 0) {
        cursor.restore(mark);
        return null;
    }
    return acc;
}

/// Parses a signed base-10 64-bit integer with optional `+` / `-` sign.
pub fn parseI64(cursor: *ParseCursor): i64? {
    const mark = cursor.save();
    let neg = false;
    if (charByte(cursor, '-' as u8)) {
        neg = true;
    } else {
        * = charByte(cursor, '+' as u8);
    }
    const magOpt = parseU64(cursor);
    if (magOpt == null) {
        cursor.restore(mark);
        return null;
    }
    const mag = magOpt.?;
    const maxPos: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    const maxNegMag: u64 = ((0x80000000 as u64) << 32);
    if (neg) {
        if (mag > maxNegMag) {
            cursor.restore(mark);
            return null;
        }
        if (mag == maxNegMag) {
            return (-(0x7FFFFFFF as i64) * (0x100000000 as i64)) - (0x100000000 as i64);
        }
        return -(mag as i64);
    }
    if (mag > maxPos) {
        cursor.restore(mark);
        return null;
    }
    return mag as i64;
}

/// Parses a double-quoted string `"..."` with `\"` / `\\` escape handling.
/// Returns the inner slice (without surrounding quotes) or `null` if malformed.
pub fn parseQuotedString(cursor: *ParseCursor): string? {
    const mark = cursor.save();
    if (!charByte(cursor, '"' as u8)) {
        return null;
    }
    const innerStart = cursor.pos;
    while (ch := cursor.peek()) {
        if (ch == ('\\' as u8)) {
            * = cursor.advance();
            if (cursor.isEof()) {
                cursor.restore(mark);
                return null;
            }
            * = cursor.advance();
            continue;
        }
        if (ch == ('"' as u8)) {
            const innerEnd = cursor.pos;
            * = cursor.advance();
            return cursor.input[innerStart..innerEnd];
        }
        * = cursor.advance();
    }
    cursor.restore(mark);
    return null;
}

/// Parses `open` + content + `close`, returning the inner slice. Restores cursor on failure.
pub fn delimited(cursor: *ParseCursor, open: string, close: string): string? {
    const mark = cursor.save();
    if (!tag(cursor, open)) {
        return null;
    }
    const innerOpt = takeUntil(cursor, close);
    if (innerOpt == null) {
        cursor.restore(mark);
        return null;
    }
    * = tag(cursor, close);
    return innerOpt.?;
}

/// Parses a list of trimmed items separated by `sep` into `out`. Returns number of items parsed.
pub fn separatedList(cursor: *ParseCursor, sep: string, out: []string): usize {
    if (sep.len == 0 || out.len == 0) {
        return 0;
    }
    let count: usize = 0;
    * = skipSpaces(cursor);
    if (cursor.isEof()) {
        return 0;
    }
    while (count < out.len && !cursor.isEof()) {
        const start = cursor.pos;
        while (!cursor.isEof() && !cmbStartsWithAt(cursor.input, cursor.pos, sep)) {
            * = cursor.advance();
        }
        let end = cursor.pos;
        while (end > start && cmbIsSpace(cursor.input[end - 1])) {
            end -= 1;
        }
        out[count] = cursor.input[start..end];
        count += 1;
        if (!tag(cursor, sep)) {
            break;
        }
        * = skipSpaces(cursor);
    }
    return count;
}

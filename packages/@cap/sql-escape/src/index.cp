// @cap/sql-escape — Zero-allocation ANSI SQL / PostgreSQL / MySQL Escaping & Identifier Guard
// Clean-room MIT implementation in pure CP.

fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8) -> usize {
    if (!allow_empty && input.len == 0) {
        return 0;
    }
    let needed: usize = 2;
    let i: usize = 0;
    while (i < input.len) {
        const ch = input[i];
        if (ch == 0) {
            // Reject embedded NUL bytes
            return 0;
        }
        if (ch == quote) {
            needed += 2;
        } else {
            needed += 1;
        }
        i += 1;
    }
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    out[w] = quote;
    w += 1;
    i = 0;
    while (i < input.len) {
        const ch = input[i];
        if (ch == quote) {
            out[w] = quote;
            out[w + 1] = quote;
            w += 2;
        } else {
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    out[w] = quote;
    w += 1;
    return w;
}

/// Escapes an ANSI SQL string literal as `'...'`, doubling internal `'` to `''`.
/// Rejects embedded NUL (`\0`) bytes or insufficient buffer by returning `0`.
pub fn escapeStringAnsi(input: string, out: []u8) -> usize {
    return escapeQuoted(input, 39, true, out); // '\''
}

/// Escapes an ANSI SQL / PostgreSQL / SQLite identifier as `"..."`, doubling internal `"` to `""`.
/// Rejects empty identifiers, embedded NUL bytes, or insufficient buffer by returning `0`.
pub fn escapeIdentifierAnsi(ident: string, out: []u8) -> usize {
    return escapeQuoted(ident, 34, false, out); // '"'
}

/// Escapes a MySQL identifier as `` `...` ``, doubling internal `` ` `` to ``` `` ```.
/// Rejects empty identifiers, embedded NUL bytes, or insufficient buffer by returning `0`.
pub fn escapeIdentifierMysql(ident: string, out: []u8) -> usize {
    return escapeQuoted(ident, 96, false, out); // '`'
}

/// Escapes SQL `LIKE` wildcard characters (`%`, `_`, and `escape_char` itself) using `escape_char`.
/// Returns bytes written, or `0` if `escape_char == 0` or `out` is too small.
pub fn escapeLike(pattern: string, escape_char: u8, out: []u8) -> usize {
    if (escape_char == 0 || pattern.len == 0) {
        return 0;
    }
    let needed: usize = 0;
    let i: usize = 0;
    while (i < pattern.len) {
        const ch = pattern[i];
        if (ch == 37 || ch == 95 || ch == escape_char) {
            needed += 2;
        } else {
            needed += 1;
        }
        i += 1;
    }
    if (needed > out.len) {
        return 0;
    }

    let w: usize = 0;
    i = 0;
    while (i < pattern.len) {
        const ch = pattern[i];
        if (ch == 37 || ch == 95 || ch == escape_char) {
            out[w] = escape_char;
            out[w + 1] = ch;
            w += 2;
        } else {
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    return w;
}

fn isAlphaOrUnder(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }
    if (b >= 97 && b <= 122) { return true; }
    return b == 95;
}

fn isIdentRest(b: u8) -> bool {
    if (isAlphaOrUnder(b)) { return true; }
    return b >= 48 && b <= 57;
}

/// Validates a safe unquoted SQL identifier (`[A-Za-z_][A-Za-z0-9_]*`, length 1..63).
pub fn isSafeUnquotedIdent(ident: string) -> bool {
    if (ident.len == 0 || ident.len > 63 || !isAlphaOrUnder(ident[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < ident.len) {
        if (!isIdentRest(ident[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Counts `?` parameter placeholders in `sql` that appear outside single-quoted string literals
/// and double-quoted identifiers (respecting doubled quote escapes `''` and `""`).
pub fn countPlaceholders(sql: string) -> usize {
    let count: usize = 0;
    let in_single = false;
    let in_double = false;
    let i: usize = 0;
    const n = sql.len;

    while (i < n) {
        const ch = sql[i];
        if (in_single) {
            if (ch == 39) {
                if (i + 1 < n && sql[i + 1] == 39) {
                    i += 2;
                    continue;
                }
                in_single = false;
            }
        } else if (in_double) {
            if (ch == 34) {
                if (i + 1 < n && sql[i + 1] == 34) {
                    i += 2;
                    continue;
                }
                in_double = false;
            }
        } else {
            if (ch == 39) {
                in_single = true;
            } else if (ch == 34) {
                in_double = true;
            } else if (ch == 63) {
                count += 1;
            }
        }
        i += 1;
    }

    return count;
}

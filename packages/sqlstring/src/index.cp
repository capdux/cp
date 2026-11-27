// @cap/sql-escape — Zero-allocation ANSI SQL / PostgreSQL / MySQL Escaping & Identifier Guard
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry identifier LUTs, and bulk @memcpy.

type SqlVec16 = @Vector(16, u8);

fn sqlBuildIdentStartLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        if ((i >= 65 && i <= 90) || (i >= 97 && i <= 122) || i == 95) {
            lut[i] = 1;
        } else {
            lut[i] = 0;
        }
        i += 1;
    }
    return lut;
}

fn sqlBuildIdentContLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        if ((i >= 48 && i <= 57) || (i >= 65 && i <= 90) || (i >= 97 && i <= 122) || i == 95) {
            lut[i] = 1;
        } else {
            lut[i] = 0;
        }
        i += 1;
    }
    return lut;
}

const SQL_IDENT_START_LUT: [256]u8 = sqlBuildIdentStartLut();
const SQL_IDENT_CONT_LUT: [256]u8 = sqlBuildIdentContLut();

fn escapeQuoted(input: string, quote: u8, allow_empty: bool, out: []u8) -> usize {
    const n = input.len;
    if (!allow_empty && n == 0) {
        return 0;
    }

    // Single-pass 16-lane SIMD scan: reject embedded NUL and count quote occurrences
    const v_nul: SqlVec16 = @splat(0 as u8);
    const v_q: SqlVec16 = @splat(quote);
    const v_one: SqlVec16 = @splat(1 as u8);
    const v_zero: SqlVec16 = @splat(0 as u8);

    let quote_count: usize = 0;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: SqlVec16 = input[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_nul)) {
            return 0;
        }
        const q_mask = v == v_q;
        if (@reduce(.Or, q_mask)) {
            const cnt_v: SqlVec16 = @select(u8, q_mask, v_one, v_zero);
            quote_count += @reduce(.Add, cnt_v) as usize;
        }
        i += 16;
    }
    while (i < n) {
        const ch = input[i];
        if (ch == 0) {
            return 0;
        }
        if (ch == quote) {
            quote_count += 1;
        }
        i += 1;
    }

    const needed: usize = n + quote_count + 2;
    if (needed > out.len) {
        return 0;
    }

    // Fast path: no internal quotes -> single @memcpy
    if (quote_count == 0) {
        out[0] = quote;
        if (n > 0) {
            @memcpy(out[1 .. n + 1], input[0..n]);
        }
        out[n + 1] = quote;
        return n + 2;
    }

    let w: usize = 0;
    out[w] = quote;
    w += 1;
    i = 0;
    while (i + 16 <= n) {
        const v: SqlVec16 = input[i .. i + 16][0..16].*;
        if (!@reduce(.Or, v == v_q)) {
            @memcpy(out[w .. w + 16], input[i .. i + 16]);
            w += 16;
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            const ch = input[i + k];
            if (ch == quote) {
                out[w] = quote;
                out[w + 1] = quote;
                w += 2;
            } else {
                out[w] = ch;
                w += 1;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < n) {
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
    const n = pattern.len;
    if (escape_char == 0 || n == 0) {
        return 0;
    }

    const v_pct: SqlVec16 = @splat(37 as u8);
    const v_und: SqlVec16 = @splat(95 as u8);
    const v_esc: SqlVec16 = @splat(escape_char);
    const v_one: SqlVec16 = @splat(1 as u8);
    const v_zero: SqlVec16 = @splat(0 as u8);

    let extra: usize = 0;
    let i: usize = 0;
    while (i + 16 <= n) {
        const v: SqlVec16 = pattern[i .. i + 16][0..16].*;
        const sp = (v == v_pct) | (v == v_und) | (v == v_esc);
        if (@reduce(.Or, sp)) {
            const cnt_v: SqlVec16 = @select(u8, sp, v_one, v_zero);
            extra += @reduce(.Add, cnt_v) as usize;
        }
        i += 16;
    }
    while (i < n) {
        const ch = pattern[i];
        if (ch == 37 || ch == 95 || ch == escape_char) {
            extra += 1;
        }
        i += 1;
    }

    const needed = n + extra;
    if (needed > out.len) {
        return 0;
    }

    if (extra == 0) {
        @memcpy(out[0..n], pattern[0..n]);
        return n;
    }

    let w: usize = 0;
    i = 0;
    while (i + 16 <= n) {
        const v: SqlVec16 = pattern[i .. i + 16][0..16].*;
        const sp = (v == v_pct) | (v == v_und) | (v == v_esc);
        if (!@reduce(.Or, sp)) {
            @memcpy(out[w .. w + 16], pattern[i .. i + 16]);
            w += 16;
            i += 16;
            continue;
        }
        let k: usize = 0;
        while (k < 16) {
            const ch = pattern[i + k];
            if (ch == 37 || ch == 95 || ch == escape_char) {
                out[w] = escape_char;
                out[w + 1] = ch;
                w += 2;
            } else {
                out[w] = ch;
                w += 1;
            }
            k += 1;
        }
        i += 16;
    }
    while (i < n) {
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

/// Validates a safe unquoted SQL identifier (`[A-Za-z_][A-Za-z0-9_]*`, length 1..63).
pub fn isSafeUnquotedIdent(ident: string) -> bool {
    const n = ident.len;
    if (n == 0 || n > 63 || SQL_IDENT_START_LUT[ident[0] as usize] == 0) {
        return false;
    }
    let i: usize = 1;
    const v_0: SqlVec16 = @splat(48 as u8);
    const v_9: SqlVec16 = @splat(57 as u8);
    const v_A: SqlVec16 = @splat(65 as u8);
    const v_Z: SqlVec16 = @splat(90 as u8);
    const v_a: SqlVec16 = @splat(97 as u8);
    const v_z: SqlVec16 = @splat(122 as u8);
    const v_und: SqlVec16 = @splat(95 as u8);
    while (i + 16 <= n) {
        const v: SqlVec16 = ident[i .. i + 16][0..16].*;
        const ok = ((v >= v_0) & (v <= v_9))
            | ((v >= v_A) & (v <= v_Z))
            | ((v >= v_a) & (v <= v_z))
            | (v == v_und);
        if (!@reduce(.And, ok)) {
            return false;
        }
        i += 16;
    }
    while (i < n) {
        if (SQL_IDENT_CONT_LUT[ident[i] as usize] == 0) {
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

    const v_sq: SqlVec16 = @splat(39 as u8);
    const v_dq: SqlVec16 = @splat(34 as u8);
    const v_qm: SqlVec16 = @splat(63 as u8);

    while (i < n) {
        if (in_single) {
            while (i + 16 <= n) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_sq)) {
                    break;
                }
                i += 16;
            }
            if (i >= n) {
                break;
            }
            const ch = sql[i];
            if (ch == 39) {
                if (i + 1 < n && sql[i + 1] == 39) {
                    i += 2;
                    continue;
                }
                in_single = false;
            }
            i += 1;
        } else if (in_double) {
            while (i + 16 <= n) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_dq)) {
                    break;
                }
                i += 16;
            }
            if (i >= n) {
                break;
            }
            const ch = sql[i];
            if (ch == 34) {
                if (i + 1 < n && sql[i + 1] == 34) {
                    i += 2;
                    continue;
                }
                in_double = false;
            }
            i += 1;
        } else {
            while (i + 16 <= n) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                const hit = (v == v_sq) | (v == v_dq) | (v == v_qm);
                if (@reduce(.Or, hit)) {
                    break;
                }
                i += 16;
            }
            if (i >= n) {
                break;
            }
            const ch = sql[i];
            if (ch == 39) {
                in_single = true;
            } else if (ch == 34) {
                in_double = true;
            } else if (ch == 63) {
                count += 1;
            }
            i += 1;
        }
    }

    return count;
}

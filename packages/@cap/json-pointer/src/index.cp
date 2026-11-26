// @cap/json-pointer — Zero-allocation RFC 6901 JSON Pointer Implementation
// Clean-room MIT implementation in pure CP.

/// Validates an RFC 6901 JSON Pointer.
/// Empty string `""` (root document) is valid; non-empty pointers must start with `'/'`
/// and every `'~'` must be followed by `'0'` or `'1'`.
pub fn isValid(ptr: string) -> bool {
    if (ptr.len == 0) {
        return true;
    }
    if (ptr[0] != 47) {
        // Must start with '/'
        return false;
    }
    let i: usize = 0;
    while (i < ptr.len) {
        if (ptr[i] == 126) {
            // '~'
            if (i + 1 >= ptr.len) {
                return false;
            }
            const nxt = ptr[i + 1];
            if (nxt != 48 && nxt != 49) {
                // Only ~0 and ~1 are valid
                return false;
            }
            i += 2;
        } else {
            i += 1;
        }
    }
    return true;
}

/// Escapes a reference token per RFC 6901 (`~` -> `~0`, `/` -> `~1`) into `out`.
/// Returns the number of bytes written.
pub fn escapeToken(token: string, out: []u8) -> usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < token.len) {
        const ch = token[i];
        if (ch == 126) {
            if (w + 2 > out.len) {
                break;
            }
            out[w] = 126;     // '~'
            out[w + 1] = 48;  // '0'
            w += 2;
        } else if (ch == 47) {
            if (w + 2 > out.len) {
                break;
            }
            out[w] = 126;     // '~'
            out[w + 1] = 49;  // '1'
            w += 2;
        } else {
            if (w >= out.len) {
                break;
            }
            out[w] = ch;
            w += 1;
        }
        i += 1;
    }
    return w;
}

/// Unescapes a reference token per RFC 6901 (`~1` -> `/`, `~0` -> `~`) into `out`.
/// Single-pass decoding preserves `~01` -> `~1`. Returns the number of bytes written.
pub fn unescapeToken(token: string, out: []u8) -> usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < token.len && w < out.len) {
        if (token[i] == 126 && i + 1 < token.len) {
            const nxt = token[i + 1];
            if (nxt == 49) {
                out[w] = 47;  // '/'
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 48) {
                out[w] = 126; // '~'
                w += 1;
                i += 2;
                continue;
            }
        }
        out[w] = token[i];
        w += 1;
        i += 1;
    }
    return w;
}

/// Returns the number of reference token segments in a valid JSON Pointer (`""` -> 0, `"/"` -> 1).
pub fn countSegments(ptr: string) -> usize {
    if (!isValid(ptr) || ptr.len == 0) {
        return 0;
    }
    let count: usize = 0;
    let i: usize = 0;
    while (i < ptr.len) {
        if (ptr[i] == 47) {
            count += 1;
        }
        i += 1;
    }
    return count;
}

/// Returns true if `ptr` is valid and has a segment at `index`.
pub fn hasSegment(ptr: string, index: usize) -> bool {
    return isValid(ptr) && index < countSegments(ptr);
}

/// Extracts and unescapes the `index`-th segment (0-based) into `out`.
/// Returns bytes written, or `0` if out-of-bounds or invalid.
pub fn getSegment(ptr: string, index: usize, out: []u8) -> usize {
    if (!isValid(ptr) || ptr.len == 0) {
        return 0;
    }
    let seg_idx: usize = 0;
    let i: usize = 0;
    while (i < ptr.len) {
        if (ptr[i] == 47) {
            const start = i + 1;
            let end = start;
            while (end < ptr.len && ptr[end] != 47) {
                end += 1;
            }
            if (seg_idx == index) {
                return unescapeToken(ptr[start..end], out);
            }
            seg_idx += 1;
            i = end;
        } else {
            i += 1;
        }
    }
    return 0;
}

/// Parses an RFC 6901 array index token:
/// - `"0"` or non-leading-zero decimal integer -> non-negative `i64`
/// - `"-"` (append after last element) -> `-2`
/// - invalid (empty, leading zero `"01"`, non-digit, overflow) -> `-1`
pub fn parseArrayIndex(token: string) -> i64 {
    if (token.len == 0) {
        return -1;
    }
    if (token.len == 1 && token[0] == 45) {
        // "-"
        return -2;
    }
    if (token.len == 1 && token[0] == 48) {
        return 0;
    }
    if (token[0] == 48) {
        // Leading zero forbidden by RFC 6901
        return -1;
    }

    const max_div10: u64 = 922337203685477580; // i64::MAX / 10
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < token.len) {
        const ch = token[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        const digit = (ch - 48) as u64;
        if (acc > max_div10 || (acc == max_div10 && digit > 7)) {
            return -1;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc as i64;
}

/// Appends an unescaped `token` onto `base` pointer (automatically escaping `~` and `/`) into `out`.
/// Returns total bytes written into `out`, or `0` if `base` is invalid or `out` cannot hold `base` + `'/'`.
pub fn appendSegment(base: string, token: string, out: []u8) -> usize {
    if (!isValid(base)) {
        return 0;
    }
    if (base.len + 1 > out.len) {
        return 0;
    }
    let w: usize = 0;
    let i: usize = 0;
    while (i < base.len) {
        out[w] = base[i];
        w += 1;
        i += 1;
    }
    out[w] = 47; // '/'
    w += 1;
    const esc_len = escapeToken(token, out[w..out.len]);
    return w + esc_len;
}

/// Returns the parent JSON Pointer slice (zero-copy).
/// Root `""` and top-level `"/a"` return `""`.
pub fn parentPointer(ptr: string) -> string {
    if (!isValid(ptr) || ptr.len == 0) {
        return "";
    }
    let i = ptr.len;
    while (i > 0) {
        i -= 1;
        if (ptr[i] == 47) {
            return ptr[0..i];
        }
    }
    return "";
}

pub struct PointerIter {
    ptr: string,
    pos: usize,
    valid: bool,

    /// Initializes a zero-allocation iterator over the unescaped segments of `ptr`.
    pub static fn init(ptr: string) -> PointerIter {
        return PointerIter {
            ptr: ptr,
            pos: 0,
            valid: isValid(ptr),
        };
    }

    /// Writes the next unescaped segment into `out` and returns its byte length (`>= 0`).
    /// Returns `-1` when iteration is complete or the pointer is invalid.
    pub fn next(*_, out: []u8) -> i64 {
        if (!_.valid || _.pos >= _.ptr.len) {
            return -1;
        }
        if (_.ptr[_.pos] != 47) {
            return -1;
        }
        const start = _.pos + 1;
        let end = start;
        while (end < _.ptr.len && _.ptr[end] != 47) {
            end += 1;
        }
        _.pos = end;
        const written = unescapeToken(_.ptr[start..end], out);
        return written as i64;
    }
}

// @cap/json-pointer — Zero-allocation RFC 6901 JSON Pointer Implementation
// Clean-room MIT implementation in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD delimiter/escape scanning,
// batched SIMD segment counting, and bulk @memcpy.

type JptrVec16 = @Vector(16, u8);

fn jptrFindByte(s: string, start: usize, target: u8) -> usize {
    let i: usize = start;
    const len = s.len;
    const vt: JptrVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: JptrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == target) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        if (s[i] == target) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn jptrFindEscape(s: string, start: usize) -> usize {
    let i: usize = start;
    const len = s.len;
    const v_tilde: JptrVec16 = @splat(126 as u8);
    const v_slash: JptrVec16 = @splat(47 as u8);
    while (i + 16 <= len) {
        const v: JptrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_tilde) || @reduce(.Or, v == v_slash)) {
            let k: usize = 0;
            while (k < 16) {
                const ch = s[i + k];
                if (ch == 126 || ch == 47) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const ch = s[i];
        if (ch == 126 || ch == 47) {
            return i;
        }
        i += 1;
    }
    return len;
}

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
    let i: usize = 1;
    while (i < ptr.len) {
        const tilde = jptrFindByte(ptr, i, 126);
        if (tilde >= ptr.len) {
            return true;
        }
        if (tilde + 1 >= ptr.len) {
            return false;
        }
        const nxt = ptr[tilde + 1];
        if (nxt != 48 && nxt != 49) {
            // Only ~0 and ~1 are valid
            return false;
        }
        i = tilde + 2;
    }
    return true;
}

/// Escapes a reference token per RFC 6901 (`~` -> `~0`, `/` -> `~1`) into `out`.
/// Returns the number of bytes written.
pub fn escapeToken(token: string, out: []u8) -> usize {
    let i: usize = 0;
    let w: usize = 0;
    while (i < token.len && w < out.len) {
        const esc = jptrFindEscape(token, i);
        const plain_len = esc - i;
        if (plain_len > 0) {
            const avail = out.len - w;
            const copy_len = if (plain_len < avail) { plain_len } else { avail };
            @memcpy(out[w .. w + copy_len], token[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (copy_len < plain_len) {
                break;
            }
        }
        if (i >= token.len) {
            break;
        }
        if (w + 2 > out.len) {
            break;
        }
        const ch = token[i];
        out[w] = 126; // '~'
        out[w + 1] = if (ch == 126) { 48 as u8 } else { 49 as u8 }; // '0' or '1'
        w += 2;
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
        const tilde = jptrFindByte(token, i, 126);
        const plain_len = tilde - i;
        if (plain_len > 0) {
            const avail = out.len - w;
            const copy_len = if (plain_len < avail) { plain_len } else { avail };
            @memcpy(out[w .. w + copy_len], token[i .. i + copy_len]);
            w += copy_len;
            i += copy_len;
            if (copy_len < plain_len) {
                break;
            }
        }
        if (i >= token.len || w >= out.len) {
            break;
        }
        if (i + 1 < token.len) {
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
    const len = ptr.len;
    const v_slash: JptrVec16 = @splat(47 as u8);
    const v_one: JptrVec16 = @splat(1 as u8);
    const v_zero: JptrVec16 = @splat(0 as u8);

    while (i + 16 <= len) {
        let acc: JptrVec16 = v_zero;
        let batch: usize = 0;
        while (batch < 255 && i + 16 <= len) {
            const v: JptrVec16 = ptr[i .. i + 16][0..16].*;
            acc = acc + @select(u8, v == v_slash, v_one, v_zero);
            i += 16;
            batch += 1;
        }
        const a: [16]u8 = acc;
        let k: usize = 0;
        while (k < 16) {
            count += a[k] as usize;
            k += 1;
        }
    }
    while (i < len) {
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
            const end = jptrFindByte(ptr, start, 47);
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
    if (base.len > 0) {
        @memcpy(out[0..base.len], base);
    }
    out[base.len] = 47; // '/'
    const w = base.len + 1;
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
    const v_slash: JptrVec16 = @splat(47 as u8);
    while (i >= 16) {
        const base = i - 16;
        const v: JptrVec16 = ptr[base .. base + 16][0..16].*;
        if (@reduce(.Or, v == v_slash)) {
            let k: usize = 16;
            while (k > 0) {
                k -= 1;
                if (ptr[base + k] == 47) {
                    return ptr[0 .. base + k];
                }
            }
        }
        i = base;
    }
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
        const end = jptrFindByte(_.ptr, start, 47);
        _.pos = end;
        const written = unescapeToken(_.ptr[start..end], out);
        return written as i64;
    }
}

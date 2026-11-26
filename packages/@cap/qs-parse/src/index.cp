// @cap/qs-parse — Zero-allocation Express/Node nested query string parser & serializer for CP
// Clean-room MIT implementation with parity to npm `qs` v6.
// Composes `@cap/serde-qs` (nested bracket key path parsing) and `@cap/percent-encoding`
// (WHATWG/RFC percent-encoding & form decoding) with zero duplication.

import {
    parseKeyPath,
    QsPair,
    QsPath,
} from "../../serde-qs/src/index.cp";
import {
    AsciiSet,
    componentSet,
    percentEncode,
    percentDecodePlus,
} from "../../percent-encoding/src/index.cp";

pub const QS_ARRAY_INDICES: u8 = 0;
pub const QS_ARRAY_BRACKETS: u8 = 1;
pub const QS_ARRAY_REPEAT: u8 = 2;
pub const QS_ARRAY_COMMA: u8 = 3;

/// Configuration options for `parseQuery` and `stringifyPairs`.
pub struct QsOptions {
    pub delimiter: u8,
    pub allow_dots: bool,
    pub max_depth: usize,
    pub array_format: u8,
    pub add_query_prefix: bool,

    /// Return default `qs` v6 options:
    /// `delimiter = '&'`, `allow_dots = false`, `max_depth = 5`,
    /// `array_format = QS_ARRAY_INDICES`, `add_query_prefix = false`.
    pub static fn defaultOpts(): QsOptions {
        return QsOptions {
            delimiter: 0x26 as u8, // '&'
            allow_dots: false,
            max_depth: 5,
            array_format: QS_ARRAY_INDICES,
            add_query_prefix: false,
        };
    }
}

fn qspFindByte(s: string, start: usize, end: usize, target: u8): usize {
    let i = start;
    while (i < end) {
        if (s[i] == target) {
            return i;
        }
        i = i + 1;
    }
    return end;
}

fn qspWriteByte(dst: []u8, pos: usize, ch: u8): usize {
    if (pos < dst.len) {
        dst[pos] = ch;
        return pos + 1;
    }
    return pos;
}

fn qspWriteUsize(dst: []u8, pos: usize, val: usize): usize {
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let n = val;
    if (n == 0) {
        return qspWriteByte(dst, pos, 48 as u8); // '0'
    }
    while (n > 0 && len < 24) {
        digits[len] = (48 + (n % 10)) as u8;
        n = n / 10;
        len = len + 1;
    }
    let w = pos;
    let i = len;
    while (i > 0 && w < dst.len) {
        i = i - 1;
        dst[w] = digits[i];
        w = w + 1;
    }
    return w;
}

fn qspKeyEncodeSet(): AsciiSet {
    let set = componentSet();
    // Preserve structural bracket delimiters `[` and `]` in nested keys.
    set.remove(0x5B as u8); // '['
    set.remove(0x5D as u8); // ']'
    return set;
}

fn qspCountDotSegments(s: string): usize {
    if (s.len == 0) {
        return 0;
    }
    let count: usize = 1;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == (0x2E as u8)) { // '.'
            count = count + 1;
        }
        i = i + 1;
    }
    return count;
}

/// Measure total logical nesting depth of `key`, delegating bracket parsing to `@cap/serde-qs`
/// and optionally counting dot-separated segments when `allow_dots` is enabled.
fn qspMeasureKeyDepth(key: string, allow_dots: bool): usize {
    if (key.len == 0) {
        return 0;
    }
    const base_path = parseKeyPath(key);
    // Also count extra bracket pairs beyond QsPath's 8-segment capacity for strict max_depth enforcement
    let bracket_pairs: usize = 0;
    let i: usize = 0;
    while (i < key.len) {
        if (key[i] == (0x5B as u8)) { // '['
            bracket_pairs = bracket_pairs + 1;
        }
        i = i + 1;
    }
    let total_depth = base_path.depth;
    if (bracket_pairs + 1 > total_depth) {
        total_depth = bracket_pairs + 1;
    }

    if (!allow_dots) {
        return total_depth;
    }

    // Count additional dot-separated levels inside each non-empty segment
    let dot_extra: usize = 0;
    let s_idx: usize = 0;
    while (s_idx < base_path.depth) {
        const seg = base_path.segments[s_idx];
        const seg_dots = qspCountDotSegments(seg);
        if (seg_dots > 1) {
            dot_extra = dot_extra + (seg_dots - 1);
        }
        s_idx = s_idx + 1;
    }
    return total_depth + dot_extra;
}

/// Split a key into up to 8 path segments, supporting both bracket notation (`a[b][c]`)
/// via `@cap/serde-qs` and optional dot notation (`a.b.c`) when `allow_dots` is `true`.
pub fn parseKeyPathWithDots(key: string, allow_dots: bool): QsPath {
    const base_path = parseKeyPath(key);
    if (!allow_dots) {
        return base_path;
    }

    let segs: [8]string = ["", "", "", "", "", "", "", ""];
    let out_depth: usize = 0;
    let s_idx: usize = 0;
    while (s_idx < base_path.depth && out_depth < 8) {
        const seg = base_path.segments[s_idx];
        if (seg.len == 0) {
            segs[out_depth] = "";
            out_depth = out_depth + 1;
        } else {
            let pos: usize = 0;
            while (pos <= seg.len && out_depth < 8) {
                const dot = qspFindByte(seg, pos, seg.len, 0x2E as u8); // '.'
                segs[out_depth] = seg[pos..dot];
                out_depth = out_depth + 1;
                if (dot >= seg.len) {
                    break;
                }
                pos = dot + 1;
            }
        }
        s_idx = s_idx + 1;
    }

    return QsPath { segments: segs, depth: out_depth };
}

/// Parse a query string into `out_pairs` with zero heap allocations.
/// Supports custom delimiters (e.g. `&` or `;`), skips a leading `?`,
/// supports `allow_dots` (`a.b=1`), and drops pairs whose nesting depth exceeds `opts.max_depth`.
/// Returns the number of pairs written into `out_pairs`.
pub fn parseQuery(query: string, opts: QsOptions, out_pairs: []QsPair): usize {
    if (query.len == 0 || out_pairs.len == 0) {
        return 0;
    }

    const delim: u8 = if (opts.delimiter == 0) (0x26 as u8) else opts.delimiter;

    let pos: usize = 0;
    if (query[0] == (0x3F as u8)) { // '?'
        pos = 1;
    }

    let count: usize = 0;
    while (pos < query.len && count < out_pairs.len) {
        const next_delim = qspFindByte(query, pos, query.len, delim);
        if (next_delim > pos) {
            const eq = qspFindByte(query, pos, next_delim, 0x3D as u8); // '='
            const raw_key = if (eq < next_delim) query[pos..eq] else query[pos..next_delim];
            const raw_val = if (eq < next_delim) query[(eq + 1)..next_delim] else "";

            let accept = true;
            if (opts.max_depth > 0) {
                const depth = qspMeasureKeyDepth(raw_key, opts.allow_dots);
                if (depth > opts.max_depth) {
                    accept = false;
                }
            }

            if (accept) {
                out_pairs[count] = QsPair {
                    key: raw_key,
                    value: raw_val,
                };
                count = count + 1;
            }
        }
        if (next_delim >= query.len) {
            break;
        }
        pos = next_delim + 1;
    }

    return count;
}

/// Serialize `pairs` into `out` using `opts.delimiter`, optional leading `?` (`opts.add_query_prefix`),
/// and `@cap/percent-encoding`. Returns total bytes written into `out`.
pub fn stringifyPairs(pairs: []const QsPair, opts: QsOptions, out: []u8): usize {
    if (pairs.len == 0 || out.len == 0) {
        return 0;
    }

    const delim: u8 = if (opts.delimiter == 0) (0x26 as u8) else opts.delimiter;
    const key_set = qspKeyEncodeSet();
    const val_set = componentSet();

    let pos: usize = 0;
    if (opts.add_query_prefix) {
        pos = qspWriteByte(out, pos, 0x3F as u8); // '?'
    }

    let i: usize = 0;
    while (i < pairs.len && pos < out.len) {
        if (i > 0) {
            pos = qspWriteByte(out, pos, delim);
            if (pos >= out.len) {
                break;
            }
        }

        const k_written = percentEncode(pairs[i].key, key_set, out[pos..]);
        pos = pos + k_written;

        if (pos >= out.len) {
            break;
        }
        pos = qspWriteByte(out, pos, 0x3D as u8); // '='
        if (pos >= out.len) {
            break;
        }

        const v_written = percentEncode(pairs[i].value, val_set, out[pos..]);
        pos = pos + v_written;

        i = i + 1;
    }

    return pos;
}

/// Serialize an array of values for `key` into `out` using one of the 4 npm `qs` array formats:
/// - `QS_ARRAY_INDICES (0)`: `key[0]=v0&key[1]=v1`
/// - `QS_ARRAY_BRACKETS (1)`: `key[]=v0&key[]=v1`
/// - `QS_ARRAY_REPEAT (2)`: `key=v0&key=v1`
/// - `QS_ARRAY_COMMA (3)`: `key=v0,v1`
/// Returns total bytes written into `out`.
pub fn stringifyArray(key: string, values: []const string, array_format: u8, out: []u8): usize {
    if (values.len == 0 || out.len == 0) {
        return 0;
    }

    const key_set = qspKeyEncodeSet();
    const val_set = componentSet();
    let pos: usize = 0;

    if (array_format == QS_ARRAY_COMMA) {
        pos = pos + percentEncode(key, key_set, out[pos..]);
        if (pos >= out.len) {
            return pos;
        }
        pos = qspWriteByte(out, pos, 0x3D as u8); // '='
        let i: usize = 0;
        while (i < values.len && pos < out.len) {
            if (i > 0) {
                pos = qspWriteByte(out, pos, 0x2C as u8); // ','
                if (pos >= out.len) {
                    break;
                }
            }
            pos = pos + percentEncode(values[i], val_set, out[pos..]);
            i = i + 1;
        }
        return pos;
    }

    let i: usize = 0;
    while (i < values.len && pos < out.len) {
        if (i > 0) {
            pos = qspWriteByte(out, pos, 0x26 as u8); // '&'
            if (pos >= out.len) {
                break;
            }
        }

        pos = pos + percentEncode(key, key_set, out[pos..]);
        if (pos >= out.len) {
            break;
        }

        if (array_format == QS_ARRAY_INDICES) {
            pos = qspWriteByte(out, pos, 0x5B as u8); // '['
            pos = qspWriteUsize(out, pos, i);
            pos = qspWriteByte(out, pos, 0x5D as u8); // ']'
        } else if (array_format == QS_ARRAY_BRACKETS) {
            pos = qspWriteByte(out, pos, 0x5B as u8); // '['
            pos = qspWriteByte(out, pos, 0x5D as u8); // ']'
        }

        if (pos >= out.len) {
            break;
        }
        pos = qspWriteByte(out, pos, 0x3D as u8); // '='
        if (pos >= out.len) {
            break;
        }

        pos = pos + percentEncode(values[i], val_set, out[pos..]);
        i = i + 1;
    }

    return pos;
}

/// Decode a query value (converting `+` to space and `%XX` sequences to bytes)
/// by delegating directly to `@cap/percent-encoding`.
pub fn decodeQueryValue(raw: string, out: []u8): usize {
    return percentDecodePlus(raw, out);
}

export {
    QS_ARRAY_INDICES,
    QS_ARRAY_BRACKETS,
    QS_ARRAY_REPEAT,
    QS_ARRAY_COMMA,
    QsOptions,
    QsPair,
    QsPath,
    parseKeyPath,
    parseKeyPathWithDots,
    parseQuery,
    stringifyPairs,
    stringifyArray,
    decodeQueryValue,
};

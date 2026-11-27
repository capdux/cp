// packages/@cap/serde-qs/src/index.cp
// Zero-allocation nested bracket query string parser & serializer.
// Clean-room MIT parity with crates.io `serde_qs` v0.13.
// Composes `@cap/percent-encoding` for RFC/WHATWG percent encoding & form decoding.

import { strEq, findByte } from "cap:ascii";
import {
    AsciiSet,
    componentSet,
    percentEncode,
    percentDecodePlus,
} from "../../percent-encoding/src/index.cp";

/// Raw key-value pair extracted from a query string.
pub struct QsPair {
    pub key: string,
    pub value: string,
}

/// Parsed nested bracket key path (up to 8 segments, zero heap allocation).
/// Examples:
/// - `user[address][city]` -> segments `["user", "address", "city"]`, depth `3`
/// - `items[0]`            -> segments `["items", "0"]`, depth `2`
/// - `tags[]`              -> segments `["tags", ""]`, depth `2`
pub struct QsPath {
    pub segments: [8]string,
    pub depth: usize,
}

fn sqsKeyEncodeSet(): AsciiSet {
    let set = componentSet();
    // Preserve structural bracket delimiters `[` and `]` in nested keys (serde_qs convention).
    set.remove(0x5B as u8); // '['
    set.remove(0x5D as u8); // ']'
    return set;
}

/// Split a nested bracket key into up to 8 path segments without heap allocation.
pub fn parseKeyPath(key: string): QsPath {
    let segs: [8]string = ["", "", "", "", "", "", "", ""];
    let depth: usize = 0;

    if (key.len == 0) {
        return QsPath { segments: segs, depth: 0 };
    }

    const first_open = findByte(key, 0x5B as u8, 0) ?? key.len; // '['
    if (first_open == key.len) {
        segs[0] = key;
        return QsPath { segments: segs, depth: 1 };
    }

    if (first_open > 0) {
        segs[0] = key[0..first_open];
        depth = 1;
    }

    let pos = first_open;
    while (pos < key.len && depth < 8) {
        if (key[pos] == (0x5B as u8)) {
            const close = findByte(key, 0x5D as u8, pos + 1) ?? key.len; // ']'
            if (close < key.len) {
                segs[depth] = key[(pos + 1)..close];
                depth += 1;
                pos = close + 1;
            } else {
                segs[depth] = key[(pos + 1)..key.len];
                depth += 1;
                break;
            }
        } else {
            const next_open = findByte(key, 0x5B as u8, pos) ?? key.len;
            segs[depth] = key[pos..next_open];
            depth += 1;
            pos = next_open;
        }
    }

    return QsPath { segments: segs, depth: depth };
}

/// Parse a query string (automatically skipping a leading `?`) into raw `QsPair` slices.
/// Splits on `&`, preserving raw key and value substrings. Returns the number of pairs written.
pub fn parseQs(query: string, out_pairs: []QsPair): usize {
    if (query.len == 0 || out_pairs.len == 0) {
        return 0;
    }

    let pos: usize = 0;
    if (query[0] == (0x3F as u8)) { // '?'
        pos = 1;
    }

    let count: usize = 0;
    while (pos < query.len && count < out_pairs.len) {
        const amp = findByte(query, 0x26 as u8, pos) ?? query.len; // '&'
        if (amp > pos) {
            const eq = findByte(query[0..amp], 0x3D as u8, pos) ?? amp; // '='
            if (eq < amp) {
                out_pairs[count] = QsPair {
                    key: query[pos..eq],
                    value: query[(eq + 1)..amp],
                };
            } else {
                out_pairs[count] = QsPair {
                    key: query[pos..amp],
                    value: "",
                };
            }
            count += 1;
        }
        if (amp >= query.len) {
            break;
        }
        pos = amp + 1;
    }

    return count;
}

/// Find `target_key` in `query` and decode its value into `out` via `@cap/percent-encoding`
/// (supporting `+` -> space and `%XX` sequences). Returns decoded byte length, or `null` if absent.
pub fn getQsValueDecoded(query: string, target_key: string, out: []u8): usize? {
    if (query.len == 0) {
        return null;
    }

    let pos: usize = 0;
    if (query[0] == (0x3F as u8)) { // '?'
        pos = 1;
    }

    let key_buf: [256]u8 = undefined;

    while (pos < query.len) {
        const amp = findByte(query, 0x26 as u8, pos) ?? query.len; // '&'
        if (amp > pos) {
            const eq = findByte(query[0..amp], 0x3D as u8, pos) ?? amp; // '='
            const raw_key = if (eq < amp) query[pos..eq] else query[pos..amp];
            const raw_val = if (eq < amp) query[(eq + 1)..amp] else "";

            let matched = strEq(raw_key, target_key);
            if (!matched && (findByte(raw_key, 0x25 as u8, 0) != null || findByte(raw_key, 0x2B as u8, 0) != null)) {
                const klen = percentDecodePlus(raw_key, key_buf[0..]);
                matched = strEq(key_buf[0..klen], target_key);
            }

            if (matched) {
                return percentDecodePlus(raw_val, out);
            }
        }
        if (amp >= query.len) {
            break;
        }
        pos = amp + 1;
    }

    return null;
}

/// Serialize `pairs` into `out` as `&`-delimited `key=value` entries using `@cap/percent-encoding`.
/// Preserves structural `[` and `]` brackets in keys while percent-encoding reserved bytes.
/// Returns the number of bytes written.
pub fn serializeQs(pairs: []const QsPair, out: []u8): usize {
    if (pairs.len == 0 || out.len == 0) {
        return 0;
    }

    const key_set = sqsKeyEncodeSet();
    const val_set = componentSet();

    let pos: usize = 0;
    let i: usize = 0;
    while (i < pairs.len && pos < out.len) {
        if (i > 0) {
            if (pos >= out.len) {
                break;
            }
            out[pos] = 0x26 as u8; // '&'
            pos += 1;
        }

        const k_written = percentEncode(pairs[i].key, key_set, out[pos..]);
        pos += k_written;

        if (pos >= out.len) {
            break;
        }
        out[pos] = 0x3D as u8; // '='
        pos += 1;

        const v_written = percentEncode(pairs[i].value, val_set, out[pos..]);
        pos += v_written;

        i += 1;
    }

    return pos;
}

export {
    QsPair,
    QsPath,
    parseKeyPath,
    parseQs,
    getQsValueDecoded,
    serializeQs,
};

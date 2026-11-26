// @cap/camelcase-keys — Zero-heap streaming JSON object key case converter in pure CP
// MIT License — Composes @cap/camelcase directly (npm camelcase-keys / decamelize-keys parity)

import {
    toCamelCase,
    toSnakeCase,
    toPascalCase,
} from "../../camelcase/src/index.cp";

const MODE_CAMEL: u8 = 0;
const MODE_SNAKE: u8 = 1;
const MODE_PASCAL: u8 = 2;

fn ckIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 13 || ch == 10;
}

fn ckWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn ckWriteSlice(out: []u8, pos: usize, src: []const u8): usize {
    let p: usize = pos;
    let i: usize = 0;
    while (i < src.len && p < out.len) {
        out[p] = src[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn ckConvertJsonKeys(json: string, mode: u8, out: []u8): usize {
    if (json.len == 0 || out.len == 0) {
        return 0;
    }
    let key_buf: [256]u8 = undefined;
    let w: usize = 0;
    let i: usize = 0;

    while (i < json.len && w < out.len) {
        if (json[i] != 34) { // not '"'
            out[w] = json[i];
            w += 1;
            i += 1;
            continue;
        }

        // Scan JSON string literal [i .. close_idx], respecting backslash escapes
        const open_idx: usize = i;
        let j: usize = i + 1;
        let has_escape: bool = false;
        while (j < json.len) {
            const ch: u8 = json[j];
            if (ch == 92) { // '\\'
                has_escape = true;
                j += 2;
                continue;
            }
            if (ch == 34) { // closing '"'
                break;
            }
            j += 1;
        }

        // Unclosed string literal: copy remaining bytes verbatim
        if (j >= json.len) {
            w = ckWriteSlice(out, w, json[open_idx..json.len]);
            break;
        }

        const close_idx: usize = j;

        // Look ahead past whitespace: in RFC 8259 JSON, a string is an object key iff followed by ':'
        let look: usize = close_idx + 1;
        while (look < json.len && ckIsSpace(json[look])) {
            look += 1;
        }
        const is_obj_key: bool = look < json.len && json[look] == 58; // ':'

        if (is_obj_key && !has_escape && (close_idx - open_idx - 1) > 0 && (close_idx - open_idx - 1) <= 256) {
            const raw_key: string = json[open_idx + 1 .. close_idx];
            let conv_len: usize = 0;
            if (mode == MODE_CAMEL) {
                conv_len = toCamelCase(raw_key, &key_buf);
            } else if (mode == MODE_SNAKE) {
                conv_len = toSnakeCase(raw_key, &key_buf);
            } else {
                conv_len = toPascalCase(raw_key, &key_buf);
            }
            w = ckWriteByte(out, w, 34); // '"'
            w = ckWriteSlice(out, w, key_buf[0..conv_len]);
            w = ckWriteByte(out, w, 34); // '"'
        } else {
            w = ckWriteSlice(out, w, json[open_idx .. close_idx + 1]);
        }

        i = close_idx + 1;
    }

    return w;
}

/// Stream-convert all JSON object keys (including nested objects & arrays) to `camelCase`.
/// String values, numbers, booleans, and structure are preserved verbatim.
pub fn convertJsonKeysToCamel(json: string, out: []u8): usize {
    return ckConvertJsonKeys(json, MODE_CAMEL, out);
}

/// Stream-convert all JSON object keys (including nested objects & arrays) to `snake_case`.
/// String values, numbers, booleans, and structure are preserved verbatim.
pub fn convertJsonKeysToSnake(json: string, out: []u8): usize {
    return ckConvertJsonKeys(json, MODE_SNAKE, out);
}

/// Stream-convert all JSON object keys (including nested objects & arrays) to `PascalCase`.
/// String values, numbers, booleans, and structure are preserved verbatim.
pub fn convertJsonKeysToPascal(json: string, out: []u8): usize {
    return ckConvertJsonKeys(json, MODE_PASCAL, out);
}

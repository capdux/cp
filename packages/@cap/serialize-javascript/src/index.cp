// @cap/serialize-javascript — Zero-heap XSS-safe JS/JSON serializer in pure CP
// Clean-room BSD-3-Clause implementation of npm `serialize-javascript` (#956, 59.0M/wk).

fn sjsWriteStr(s: string, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn sjsHex(n: u8): u8 {
    if (n < 10) { return 48 + n; }
    return 87 + n;
}

/// Escapes an already-serialized JSON string so it is safe to embed inside an HTML `<script>` tag
/// (escapes `<`, `>`, `/`, U+2028, and U+2029 as `\uXXXX` sequences).
pub fn escapeUnsafeJson(json_text: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < json_text.len) {
        const b = json_text[i];
        if (b == 60) { // '<'
            p = sjsWriteStr("\\u003C", out, p);
            i += 1;
        } else if (b == 62) { // '>'
            p = sjsWriteStr("\\u003E", out, p);
            i += 1;
        } else if (b == 47) { // '/'
            p = sjsWriteStr("\\u002F", out, p);
            i += 1;
        } else if (b == 0xE2 && i + 2 < json_text.len && json_text[i + 1] == 0x80 && json_text[i + 2] == 0xA8) {
            // U+2028 Line Separator
            p = sjsWriteStr("\\u2028", out, p);
            i += 3;
        } else if (b == 0xE2 && i + 2 < json_text.len && json_text[i + 1] == 0x80 && json_text[i + 2] == 0xA9) {
            // U+2029 Paragraph Separator
            p = sjsWriteStr("\\u2029", out, p);
            i += 3;
        } else {
            if (p < out.len) {
                out[p] = b;
                p += 1;
            }
            i += 1;
        }
    }
    return p;
}

/// Serializes a string as an XSS-safe double-quoted JS/JSON string literal into `out`.
pub fn serializeString(s: string, out: []u8): usize {
    let p: usize = 0;
    p = sjsWriteStr("\"", out, p);
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 34) {
            p = sjsWriteStr("\\\"", out, p);
            i += 1;
        } else if (b == 92) {
            p = sjsWriteStr("\\\\", out, p);
            i += 1;
        } else if (b == 10) {
            p = sjsWriteStr("\\n", out, p);
            i += 1;
        } else if (b == 13) {
            p = sjsWriteStr("\\r", out, p);
            i += 1;
        } else if (b == 9) {
            p = sjsWriteStr("\\t", out, p);
            i += 1;
        } else if (b == 60) {
            p = sjsWriteStr("\\u003C", out, p);
            i += 1;
        } else if (b == 62) {
            p = sjsWriteStr("\\u003E", out, p);
            i += 1;
        } else if (b == 47) {
            p = sjsWriteStr("\\u002F", out, p);
            i += 1;
        } else if (b == 0xE2 && i + 2 < s.len && s[i + 1] == 0x80 && s[i + 2] == 0xA8) {
            p = sjsWriteStr("\\u2028", out, p);
            i += 3;
        } else if (b == 0xE2 && i + 2 < s.len && s[i + 1] == 0x80 && s[i + 2] == 0xA9) {
            p = sjsWriteStr("\\u2029", out, p);
            i += 3;
        } else if (b < 32) {
            p = sjsWriteStr("\\u00", out, p);
            if (p + 1 < out.len) {
                out[p] = sjsHex((b >> 4) & 0x0F);
                out[p + 1] = sjsHex(b & 0x0F);
                p += 2;
            }
            i += 1;
        } else {
            if (p < out.len) {
                out[p] = b;
                p += 1;
            }
            i += 1;
        }
    }
    p = sjsWriteStr("\"", out, p);
    return p;
}

/// Returns `true` if `s` contains any character unsafe for raw `<script>` embedding (`<`, `>`, `/`, U+2028, U+2029).
pub fn hasUnsafeScriptChars(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 60 || b == 62 || b == 47) {
            return true;
        }
        if (b == 0xE2 && i + 2 < s.len && s[i + 1] == 0x80 && (s[i + 2] == 0xA8 || s[i + 2] == 0xA9)) {
            return true;
        }
        i += 1;
    }
    return false;
}

export {
    escapeUnsafeJson,
    serializeString,
    hasUnsafeScriptChars,
};

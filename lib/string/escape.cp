// Source: /data/dev/bun/src/string/escapeRegExp.cp
// lib/string/escape.cp
// High-speed HTML entity escaping, JS/JSON string escaping & RegExp pattern escaping
// Absorbed from bun/src/string/immutable/escapeHTML.zig & bun/src/string/escapeRegExp.cp

const std = @import("std");
const HEX_LOWER: []const u8 = "0123456789abcdef";

pub struct StringEscape {
    pub static fn needsHtmlEscape(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == 34 || c == 38 || c == 39 || c == 60 || c == 62) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    // Escapes `"`, `&`, `'`, `<`, `>` into `out`, returning number of bytes written
    pub static fn escapeHtml(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == 34) {
                // "&quot;" (6 bytes)
                if (pos + 6 > out.len) {
                    break;
                }
                const rep = "&quot;";
                let k: usize = 0;
                while (k < 6) {
                    out[pos + k] = rep[k];
                    k += 1;
                }
                pos += 6;
            } else if (c == 38) {
                // "&amp;" (5 bytes)
                if (pos + 5 > out.len) {
                    break;
                }
                const rep = "&amp;";
                let k: usize = 0;
                while (k < 5) {
                    out[pos + k] = rep[k];
                    k += 1;
                }
                pos += 5;
            } else if (c == 39) {
                // "&#x27;" (6 bytes)
                if (pos + 6 > out.len) {
                    break;
                }
                const rep = "&#x27;";
                let k: usize = 0;
                while (k < 6) {
                    out[pos + k] = rep[k];
                    k += 1;
                }
                pos += 6;
            } else if (c == 60) {
                // "&lt;" (4 bytes)
                if (pos + 4 > out.len) {
                    break;
                }
                const rep = "&lt;";
                let k: usize = 0;
                while (k < 4) {
                    out[pos + k] = rep[k];
                    k += 1;
                }
                pos += 4;
            } else if (c == 62) {
                // "&gt;" (4 bytes)
                if (pos + 4 > out.len) {
                    break;
                }
                const rep = "&gt;";
                let k: usize = 0;
                while (k < 4) {
                    out[pos + k] = rep[k];
                    k += 1;
                }
                pos += 4;
            } else {
                if (pos + 1 > out.len) {
                    break;
                }
                out[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return pos;
    }

    // Escapes RegExp metacharacters `|\{}()[]^$+*?.-` (absorbed from bun/src/string/escapeRegExp.cp)
    pub static fn escapeRegExp(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == 124 || c == 92 || c == 123 || c == 125 || c == 40 || c == 41 || c == 91 || c == 93 || c == 94 || c == 36 || c == 43 || c == 42 || c == 63 || c == 46) {
                if (pos + 2 > out.len) {
                    break;
                }
                out[pos] = 92; // '\'
                out[pos + 1] = c;
                pos += 2;
            } else if (c == 45) {
                // '-' -> "\x2d"
                if (pos + 4 > out.len) {
                    break;
                }
                out[pos] = 92;
                out[pos + 1] = 120;
                out[pos + 2] = 50;
                out[pos + 3] = 100;
                pos += 4;
            } else {
                if (pos + 1 > out.len) {
                    break;
                }
                out[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return pos;
    }

    pub static fn needsJsEscape(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == '"' || c == '\\' || c < 0x20) {
                return true;
            }
            // Check U+2028 (E2 80 A8) and U+2029 (E2 80 A9)
            if (c == 0xE2 && i + 2 < input.len && input[i + 1] == 0x80 && (input[i + 2] == 0xA8 || input[i + 2] == 0xA9)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    // Escapes JS/JSON string characters (`"`, `\`, `\b`, `\f`, `\n`, `\r`, `\t`, control < 0x20, U+2028, U+2029)
    // Writes into `out` and returns number of bytes written.
    pub static fn escapeJs(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (c == '"') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = '"';
                pos += 2;
            } else if (c == '\\') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = '\\';
                pos += 2;
            } else if (c == '\b') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'b';
                pos += 2;
            } else if (c == 0x0C) {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'f';
                pos += 2;
            } else if (c == '\n') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'n';
                pos += 2;
            } else if (c == '\r') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'r';
                pos += 2;
            } else if (c == '\t') {
                if (pos + 2 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 't';
                pos += 2;
            } else if (c < 0x20) {
                if (pos + 6 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'u';
                out[pos + 2] = '0';
                out[pos + 3] = '0';
                out[pos + 4] = HEX_LOWER[(c >> 4) as usize];
                out[pos + 5] = HEX_LOWER[(c & 0x0F) as usize];
                pos += 6;
            } else if (c == 0xE2 && i + 2 < input.len && input[i + 1] == 0x80 && (input[i + 2] == 0xA8 || input[i + 2] == 0xA9)) {
                if (pos + 6 > out.len) { break; }
                out[pos] = '\\';
                out[pos + 1] = 'u';
                out[pos + 2] = '2';
                out[pos + 3] = '0';
                out[pos + 4] = '2';
                out[pos + 5] = if (input[i + 2] == 0xA8) '8' else '9';
                pos += 6;
                i += 2;
            } else {
                if (pos + 1 > out.len) { break; }
                out[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return pos;
    }

    pub static fn escapeJsAlloc(allocator: std.mem.Allocator, input: []const u8): ![]u8 {
        if (!StringEscape.needsJsEscape(input)) {
            return allocator.dupe(u8, input);
        }
        // Worst case: every byte becomes `\u00XX` (6x)
        const max_len = input.len * 6;
        const buf = try allocator.alloc(u8, max_len);
        const written = StringEscape.escapeJs(input, buf);
        if (written == max_len) {
            return buf;
        }
        const shrunk = try allocator.realloc(buf, written);
        return shrunk;
    }
}

pub fn escapeJs(input: []const u8, out: []u8): usize {
    return StringEscape.escapeJs(input, out);
}

// @cap/content-disposition — Pure CP zero-allocation RFC 6266 / RFC 5987
// HTTP Content-Disposition header parser, formatter, filename* decoder & sanitizer.
// Clean-room MIT implementation inspired by npm `content-disposition`.

import { AsciiSet, componentSet, percentEncode, percentDecode } from "../../percent-encoding/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";

fn cdspIsOws(b: u8): bool {
    return b == 0x20 || b == 0x09;
}

fn cdspIsTokenChar(b: u8): bool {
    if (b >= 48 && b <= 57) { return true; }   // 0-9
    if (b >= 65 && b <= 90) { return true; }   // A-Z
    if (b >= 97 && b <= 122) { return true; }  // a-z
    if (b == 0x21 || b == 0x23 || b == 0x24 || b == 0x25 || b == 0x26 ||
        b == 0x27 || b == 0x2A || b == 0x2B || b == 0x2D || b == 0x2E ||
        b == 0x5E || b == 0x5F || b == 0x60 || b == 0x7C || b == 0x7E) {
        return true;
    }
    return false;
}

fn cdspSkipOws(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && cdspIsOws(s[i])) {
        i = i + 1;
    }
    return i;
}

fn cdspHasNonAscii(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] >= 0x80) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn cdspWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) {
        return pos;
    }
    out[pos] = b;
    return pos + 1;
}

fn cdspWriteStr(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn cdspWriteAsciiFallback(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) {
        let b = s[i];
        if (b < 0x80) {
            if (b == 0x22 || b == 0x5C) {
                if (p + 2 > out.len) { break; }
                out[p] = 0x5C; // '\'
                out[p + 1] = b;
                p = p + 2;
            } else if (b >= 0x20 && b <= 0x7E) {
                if (p >= out.len) { break; }
                out[p] = b;
                p = p + 1;
            }
            i = i + 1;
        } else {
            if (p >= out.len) { break; }
            out[p] = 0x3F; // '?'
            p = p + 1;
            i = i + 1;
            while (i < s.len && (s[i] & 0xC0) == 0x80) {
                i = i + 1;
            }
        }
    }
    return p;
}

fn cdspRfc5987Set(): AsciiSet {
    let s = componentSet();
    s.add(0x27); // '\''
    s.add(0x28); // '('
    s.add(0x29); // ')'
    s.add(0x2A); // '*'
    return s;
}

fn cdspIsControl(b: u8): bool {
    return b < 0x20 || b == 0x7F;
}

/// Parsed RFC 6266 HTTP `Content-Disposition` header value (zero-copy slices).
pub struct ContentDisposition {
    pub valid: bool,
    pub disp_type: string,
    pub filename: string,
    pub filename_star: string,

    pub static fn invalid(): ContentDisposition {
        return ContentDisposition {
            valid: false,
            disp_type: "",
            filename: "",
            filename_star: ""
        };
    }

    pub fn isAttachment(): bool {
        if (!_.valid) {
            return false;
        }
        return eqAscii(_.disp_type, "attachment");
    }

    pub fn isInline(): bool {
        if (!_.valid) {
            return false;
        }
        return eqAscii(_.disp_type, "inline");
    }

    pub fn hasFilename(): bool {
        return _.filename.len > 0 || _.filename_star.len > 0;
    }
}

/// Parses an RFC 6266 / RFC 5987 `Content-Disposition` header value with zero heap allocation.
/// Matches `attachment`, `inline`, `filename`, and `filename*` case-insensitively.
pub fn parseContentDisposition(header: string): ContentDisposition {
    let i = cdspSkipOws(header, 0);
    if (i >= header.len) {
        return ContentDisposition.invalid();
    }

    // Parse disposition-type token
    let type_start = i;
    while (i < header.len && cdspIsTokenChar(header[i])) {
        i = i + 1;
    }
    if (i == type_start) {
        return ContentDisposition.invalid();
    }
    let disp = header[type_start..i];
    let fname: string = "";
    let fstar: string = "";

    i = cdspSkipOws(header, i);
    while (i < header.len) {
        if (header[i] != 0x3B) { // ';'
            return ContentDisposition.invalid();
        }
        i = cdspSkipOws(header, i + 1);
        if (i >= header.len) {
            break;
        }

        // Parse parameter name
        let key_start = i;
        while (i < header.len && cdspIsTokenChar(header[i])) {
            i = i + 1;
        }
        if (i == key_start) {
            return ContentDisposition.invalid();
        }
        let key = header[key_start..i];

        i = cdspSkipOws(header, i);
        if (i >= header.len || header[i] != 0x3D) { // '='
            return ContentDisposition.invalid();
        }
        i = cdspSkipOws(header, i + 1);
        if (i >= header.len) {
            return ContentDisposition.invalid();
        }

        // Parse parameter value (quoted-string or token)
        let val: string = "";
        if (header[i] == 0x22) { // '"'
            i = i + 1;
            let val_start = i;
            let closed = false;
            while (i < header.len) {
                let ch = header[i];
                if (ch == 0x5C) { // '\'
                    if (i + 1 >= header.len) {
                        return ContentDisposition.invalid();
                    }
                    i = i + 2;
                    continue;
                }
                if (ch == 0x22) { // closing '"'
                    val = header[val_start..i];
                    i = i + 1;
                    closed = true;
                    break;
                }
                i = i + 1;
            }
            if (!closed) {
                return ContentDisposition.invalid();
            }
        } else {
            let val_start = i;
            while (i < header.len && header[i] != 0x3B && !cdspIsOws(header[i])) {
                if (cdspIsControl(header[i]) || header[i] == 0x22) {
                    return ContentDisposition.invalid();
                }
                i = i + 1;
            }
            if (i == val_start) {
                return ContentDisposition.invalid();
            }
            val = header[val_start..i];
        }

        if (eqAscii(key, "filename")) {
            fname = val;
        } else if (eqAscii(key, "filename*")) {
            fstar = val;
        }

        i = cdspSkipOws(header, i);
    }

    return ContentDisposition {
        valid: true,
        disp_type: disp,
        filename: fname,
        filename_star: fstar
    };
}

/// Strips path traversal (`/` and `\`) and control characters (`0x00..=0x1F`, `0x7F`)
/// to produce a safe download basename in `out`. Returns bytes written.
pub fn sanitizeFilename(raw: string, out: []u8): usize {
    if (raw.len == 0 || out.len == 0) {
        return 0;
    }

    // Strip trailing slashes
    let end = raw.len;
    while (end > 0 && (raw[end - 1] == 0x2F || raw[end - 1] == 0x5C)) {
        end = end - 1;
    }
    if (end == 0) {
        return 0;
    }

    // Find last '/' or '\' before `end`
    let start: usize = 0;
    let i: usize = 0;
    while (i < end) {
        if (raw[i] == 0x2F || raw[i] == 0x5C) {
            start = i + 1;
        }
        i = i + 1;
    }

    // Copy non-control bytes
    let pos: usize = 0;
    let j = start;
    while (j < end && pos < out.len) {
        let b = raw[j];
        if (!cdspIsControl(b)) {
            out[pos] = b;
            pos = pos + 1;
        }
        j = j + 1;
    }

    // Reject relative directory names "." and ".."
    if (pos == 1 && out[0] == 0x2E) {
        return 0;
    }
    if (pos == 2 && out[0] == 0x2E && out[1] == 0x2E) {
        return 0;
    }

    return pos;
}

/// Decodes an RFC 5987 `filename*` value (e.g. `UTF-8''%E6%8A%A5%E5%91%8A.pdf`)
/// into `out` via `@cap/percent-encoding`. Supports UTF-8, US-ASCII, and ISO-8859-1.
pub fn decodeFilenameStar(raw_star: string, out: []u8): usize {
    if (raw_star.len == 0 || out.len == 0) {
        return 0;
    }

    // Locate first '\''
    let q1: usize = 0;
    let found1 = false;
    while (q1 < raw_star.len) {
        if (raw_star[q1] == 0x27) {
            found1 = true;
            break;
        }
        q1 = q1 + 1;
    }
    if (!found1 || q1 == 0) {
        return 0;
    }

    // Locate second '\''
    let q2 = q1 + 1;
    let found2 = false;
    while (q2 < raw_star.len) {
        if (raw_star[q2] == 0x27) {
            found2 = true;
            break;
        }
        q2 = q2 + 1;
    }
    if (!found2) {
        return 0;
    }

    let charset = raw_star[0..q1];
    let encoded = raw_star[q2 + 1..raw_star.len];

    if (eqAscii(charset, "UTF-8") || eqAscii(charset, "UTF8") ||
        eqAscii(charset, "US-ASCII") || eqAscii(charset, "ASCII")) {
        return percentDecode(encoded, out);
    }

    if (eqAscii(charset, "ISO-8859-1") || eqAscii(charset, "LATIN1")) {
        let tmp: [512]u8 = undefined;
        let raw_len = percentDecode(encoded, tmp[0..512]);
        let pos: usize = 0;
        let k: usize = 0;
        while (k < raw_len) {
            let b = tmp[k];
            if (b < 0x80) {
                if (pos >= out.len) { break; }
                out[pos] = b;
                pos = pos + 1;
            } else {
                if (pos + 2 > out.len) { break; }
                out[pos] = (0xC0 | (b >> 6)) as u8;
                out[pos + 1] = (0x80 | (b & 0x3F)) as u8;
                pos = pos + 2;
            }
            k = k + 1;
        }
        return pos;
    }

    return 0;
}

/// Formats an RFC 6266 / RFC 5987 `Content-Disposition` header value into `out`.
/// - Pure ASCII filenames emit `<type>; filename="..."`.
/// - Non-ASCII filenames emit `<type>; filename="<ascii-fallback>"; filename*=UTF-8''<encoded>`.
/// - Automatically sanitizes `filename` against path traversal and control characters.
pub fn formatContentDisposition(disp_type: string, filename: string, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }

    let eff_type = if (disp_type.len > 0) { disp_type } else { "attachment" };
    let pos = cdspWriteStr(out, 0, eff_type);

    if (filename.len == 0) {
        return pos;
    }

    let safe_buf: [512]u8 = undefined;
    let safe_len = sanitizeFilename(filename, safe_buf[0..512]);
    if (safe_len == 0) {
        return pos;
    }
    let safe_name = safe_buf[0..safe_len];

    pos = cdspWriteStr(out, pos, "; filename=\"");
    pos = cdspWriteAsciiFallback(out, pos, safe_name);
    pos = cdspWriteByte(out, pos, 0x22); // '"'

    if (cdspHasNonAscii(safe_name)) {
        pos = cdspWriteStr(out, pos, "; filename*=UTF-8''");
        if (pos < out.len) {
            let enc_set = cdspRfc5987Set();
            let written = percentEncode(safe_name, enc_set, out[pos..out.len]);
            pos = pos + written;
        }
    }

    return pos;
}

export {
    ContentDisposition,
    parseContentDisposition,
    formatContentDisposition,
    decodeFilenameStar,
    sanitizeFilename
};

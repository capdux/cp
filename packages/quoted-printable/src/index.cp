// @cap/quoted-printable — Zero-allocation RFC 2045 §6.7 MIME Quoted-Printable Codec
// Clean-room MIT implementation in pure CP.

import { hexCharUpper, hexVal } from "cap:ascii";

fn isDirectPrintable(b: u8) -> bool {
    return (b >= 33 && b <= 60) || (b >= 62 && b <= 126);
}

/// Encodes `data` into RFC 2045 Quoted-Printable format in `out`:
/// - Direct printable ASCII (`33..60`, `62..126`) passes through verbatim.
/// - Space/Tab pass through mid-line and are encoded as `=20`/`=09` at end-of-line.
/// - `=`, controls, and non-ASCII bytes are encoded as uppercase `=XX`.
/// - Soft line breaks `=\r\n` are inserted automatically to keep lines <= 76 characters.
/// Returns bytes written, or `0` if `data` is empty or `out` is too small.
pub fn encode(data: []const u8, out: []u8) -> usize {
    const n = data.len;
    if (n == 0) {
        return 0;
    }

    let i: usize = 0;
    let w: usize = 0;
    let col: usize = 0;

    while (i < n) {
        const b = data[i];

        // Hard line break: CRLF or LF -> emit CRLF and reset column
        if (b == 13 && i + 1 < n && data[i + 1] == 10) {
            if (w + 2 > out.len) { return 0; }
            out[w] = 13;
            out[w + 1] = 10;
            w += 2;
            col = 0;
            i += 2;
            continue;
        }
        if (b == 10) {
            if (w + 2 > out.len) { return 0; }
            out[w] = 13;
            out[w + 1] = 10;
            w += 2;
            col = 0;
            i += 1;
            continue;
        }

        const is_eol = (i + 1 == n)
            || (data[i + 1] == 10)
            || (data[i + 1] == 13 && i + 2 < n && data[i + 2] == 10);

        let needs_esc = true;
        if (isDirectPrintable(b)) {
            needs_esc = false;
        } else if ((b == 32 || b == 9) && !is_eol) {
            needs_esc = false;
        }

        let tok_len: usize = 1;
        if (needs_esc) {
            tok_len = 3;
        }

        let max_col: usize = 75; // reserve 1 col for potential soft-break '='
        if (is_eol) {
            max_col = 76;
        }

        if (col + tok_len > max_col) {
            if (w + 3 > out.len) { return 0; }
            out[w] = 61;     // '='
            out[w + 1] = 13; // '\r'
            out[w + 2] = 10; // '\n'
            w += 3;
            col = 0;
        }

        if (needs_esc) {
            if (w + 3 > out.len) { return 0; }
            out[w] = 61; // '='
            out[w + 1] = hexCharUpper((b >> 4) & 0x0F);
            out[w + 2] = hexCharUpper(b & 0x0F);
            w += 3;
            col += 3;
        } else {
            if (w >= out.len) { return 0; }
            out[w] = b;
            w += 1;
            col += 1;
        }

        i += 1;
    }

    return w;
}

/// Decodes RFC 2045 Quoted-Printable `input` into `out`:
/// - Removes soft line breaks (`=\r\n` and `=\n`).
/// - Decodes `=XX` hex escapes (case-insensitive).
/// Returns bytes written, or `-1` on malformed `=` escape or insufficient buffer.
pub fn decode(input: string, out: []u8) -> i64 {
    const n = input.len;
    let i: usize = 0;
    let w: usize = 0;

    while (i < n) {
        const ch = input[i];
        if (ch == 61) {
            // '='
            if (i + 2 < n && input[i + 1] == 13 && input[i + 2] == 10) {
                // Soft break =\r\n
                i += 3;
                continue;
            }
            if (i + 1 < n && input[i + 1] == 10) {
                // Soft break =\n
                i += 2;
                continue;
            }
            if (i + 2 < n) {
                const hi = hexVal(input[i + 1]);
                const lo = hexVal(input[i + 2]);
                if (hi != null && lo != null) {
                    if (w >= out.len) {
                        return -1;
                    }
                    out[w] = (hi.? << 4) | lo.?;
                    w += 1;
                    i += 3;
                    continue;
                }
            }
            return -1;
        }

        if (w >= out.len) {
            return -1;
        }
        out[w] = ch;
        w += 1;
        i += 1;
    }

    return w as i64;
}

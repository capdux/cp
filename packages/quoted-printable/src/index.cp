// @cap/quoted-printable — Zero-allocation RFC 2045 §6.7 MIME Quoted-Printable Codec
// Clean-room MIT implementation in pure CP.
// Accelerated with comptime 256-entry LUTs, 16-lane @Vector(16, u8) SIMD scanning, and @memcpy.

type QpVec16 = @Vector(16, u8);

const QP_HEX_UPPER = "0123456789ABCDEF";

fn qpBuildDirectLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 33;
    while (c <= 60) { lut[c] = 1; c += 1; }
    c = 62;
    while (c <= 126) { lut[c] = 1; c += 1; }
    return lut;
}

fn qpBuildHexValLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [255; 256];
    let c: usize = 48;
    while (c <= 57) { lut[c] = (c - 48) as u8; c += 1; }
    c = 65;
    while (c <= 70) { lut[c] = (c - 55) as u8; c += 1; }
    c = 97;
    while (c <= 102) { lut[c] = (c - 87) as u8; c += 1; }
    return lut;
}

const QP_DIRECT_LUT: [256]u8 = qpBuildDirectLut();
const QP_HEX_VAL_LUT: [256]u8 = qpBuildHexValLut();

fn isDirectPrintable(b: u8) -> bool {
    return QP_DIRECT_LUT[b as usize] != 0;
}

fn qpFindEq(s: string, start: usize) -> usize {
    let i = start;
    const len = s.len;
    const v_eq: QpVec16 = @splat(61 as u8);
    while (i + 16 <= len) {
        const v: QpVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_eq)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 61) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 61) {
        i += 1;
    }
    return i;
}

fn qpScanDirectRun(data: []const u8, start: usize, max_end: usize) -> usize {
    let i = start;
    const v_33: QpVec16 = @splat(33 as u8);
    const v_126: QpVec16 = @splat(126 as u8);
    const v_eq: QpVec16 = @splat(61 as u8);
    while (i + 16 <= max_end) {
        const v: QpVec16 = data[i .. i + 16][0..16].*;
        if (@reduce(.Or, v < v_33) || @reduce(.Or, v > v_126) || @reduce(.Or, v == v_eq)) {
            break;
        }
        i += 16;
    }
    while (i < max_end && QP_DIRECT_LUT[data[i] as usize] != 0) {
        i += 1;
    }
    return i;
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
        // Fast path: bulk copy direct-printable ASCII run within current line budget (col < 75)
        if (col < 75 && QP_DIRECT_LUT[data[i] as usize] != 0) {
            const room_col = 75 - col;
            const rem = n - i;
            const limit = if (rem < room_col) rem else room_col;
            const run_end = qpScanDirectRun(data, i, i + limit);
            const run_len = run_end - i;
            if (run_len > 0) {
                if (w + run_len > out.len) { return 0; }
                @memcpy(out[w .. w + run_len], data[i..run_end]);
                w += run_len;
                col += run_len;
                i = run_end;
                continue;
            }
        }

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
            out[w + 1] = QP_HEX_UPPER[((b >> 4) & 0x0F) as usize];
            out[w + 2] = QP_HEX_UPPER[(b & 0x0F) as usize];
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
        const next_eq = qpFindEq(input, i);
        if (next_eq > i) {
            const chunk = next_eq - i;
            if (w + chunk > out.len) {
                return -1;
            }
            @memcpy(out[w .. w + chunk], input[i..next_eq]);
            w += chunk;
            i = next_eq;
            if (i >= n) {
                break;
            }
        }

        // input[i] == '='
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
            const hi = QP_HEX_VAL_LUT[input[i + 1] as usize];
            const lo = QP_HEX_VAL_LUT[input[i + 2] as usize];
            if (hi != 255 && lo != 255) {
                if (w >= out.len) {
                    return -1;
                }
                out[w] = (hi << 4) | lo;
                w += 1;
                i += 3;
                continue;
            }
        }
        return -1;
    }

    return w as i64;
}

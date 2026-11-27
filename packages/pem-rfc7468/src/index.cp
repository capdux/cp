// packages/@cap/pem-rfc7468/src/index.cp
// Zero-allocation RFC 7468 PEM encoder and strict decoder in pure CP.
// Reuses @cap/base64ct for constant-time RFC 4648 Base64 encoding and decoding.
// Clean-room MIT implementation (RustCrypto pem-rfc7468 parity).

import { encodeStd, decodeStd } from "../../base64ct/src/index.cp";
import { bytesEq, indexOfBytes } from "cap:bin";

type PemVec16 = @Vector(16, u8);

fn pemBuildLabelLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0x21;
    while (c <= 0x2C) {
        lut[c] = 1;
        c += 1;
    }
    c = 0x2E;
    while (c <= 0x7E) {
        lut[c] = 1;
        c += 1;
    }
    return lut;
}

fn pemBuildWsLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[9] = 1;  // '\t'
    lut[10] = 1; // '\n'
    lut[13] = 1; // '\r'
    lut[32] = 1; // ' '
    return lut;
}

const PEM_LABEL_LUT: [256]u8 = pemBuildLabelLut();
const PEM_WS_LUT: [256]u8 = pemBuildWsLut();

pub const PEM_LINE_WIDTH: usize = 64;
pub const PEM_OK: i64 = 0;
pub const PEM_ERR_INVALID_LABEL: i64 = -1;
pub const PEM_ERR_BUFFER_TOO_SMALL: i64 = -2;
pub const PEM_ERR_MISSING_BEGIN: i64 = -3;
pub const PEM_ERR_MISSING_END: i64 = -4;
pub const PEM_ERR_LABEL_MISMATCH: i64 = -5;
pub const PEM_ERR_INVALID_BASE64: i64 = -6;
pub const PEM_ERR_INVALID_HEADER: i64 = -7;
pub const PEM_ERR_INVALID_FOOTER: i64 = -8;

pub struct PemLabel {
    pub buf: [64]u8,
    pub len: usize,

    pub static fn empty(): PemLabel {
        return PemLabel {
            buf: undefined,
            len: 0,
        };
    }

    pub fn isEmpty(): bool {
        return _.len == 0;
    }
}

pub struct PemSlice {
    pub label_start: usize,
    pub label_len: usize,
    pub body_start: usize,
    pub body_len: usize,
    pub end_pos: usize,
    pub err: i64,

    pub fn isOk(): bool {
        return _.err == PEM_OK;
    }
}

fn pemIsLabelChar(ch: u8): bool {
    return PEM_LABEL_LUT[ch as usize] != 0;
}

fn pemIsLabelValidBytes(s: string, start: usize, len: usize): bool {
    if (len == 0 || start + len > s.len) {
        return false;
    }
    // First and last characters must be labelchar (cannot start/end with SP or '-')
    if (!pemIsLabelChar(s[start]) || !pemIsLabelChar(s[start + len - 1])) {
        return false;
    }
    let prev_sep = false;
    let i: usize = 0;
    while (i < len) {
        const ch = s[start + i];
        if (pemIsLabelChar(ch)) {
            prev_sep = false;
        } else if (ch == (' ' as u8) || ch == ('-' as u8)) {
            // RFC 7468 §2 forbids consecutive spaces/hyphens
            if (prev_sep) {
                return false;
            }
            prev_sep = true;
        } else {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Validates a PEM type label according to RFC 7468 §2.
pub fn isValidLabel(label: string): bool {
    return pemIsLabelValidBytes(label, 0, label.len);
}

fn pemStartsWithAt(s: string, pos: usize, prefix: string): bool {
    if (pos + prefix.len > s.len) {
        return false;
    }
    return bytesEq(s[pos..pos + prefix.len], prefix);
}

fn pemWriteBytes(out: []u8, pos: usize, src: string): usize {
    if (src.len > 0) {
        @memcpy(out[pos .. pos + src.len], src);
    }
    return pos + src.len;
}

fn pemWriteNewline(out: []u8, pos: usize, crlf: bool): usize {
    if (crlf) {
        out[pos] = '\r' as u8;
        out[pos + 1] = '\n' as u8;
        return pos + 2;
    }
    out[pos] = '\n' as u8;
    return pos + 1;
}

fn pemAdvanceLineEnd(s: string, pos: usize): usize {
    let i: usize = pos;
    const len = s.len;
    const v_lf: PemVec16 = @splat(0x0A as u8);
    const v_cr: PemVec16 = @splat(0x0D as u8);
    while (i + 16 <= len) {
        const v: PemVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_lf) || @reduce(.Or, v == v_cr)) {
            let k: usize = 0;
            while (k < 16) {
                const ch = s[i + k];
                if (ch == 0x0A || ch == 0x0D) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len) {
        const ch = s[i];
        if (ch == 0x0A || ch == 0x0D) {
            return i;
        }
        i += 1;
    }
    return len;
}

fn pemSkipLineEnding(s: string, pos: usize): usize {
    if (pos < s.len && s[pos] == ('\r' as u8)) {
        if (pos + 1 < s.len && s[pos + 1] == ('\n' as u8)) {
            return pos + 2;
        }
        return pos + 1;
    }
    if (pos < s.len && s[pos] == ('\n' as u8)) {
        return pos + 1;
    }
    return pos;
}

fn pemFindDashes(s: string, start: usize, line_end: usize): i64 {
    if (start + 5 > line_end) {
        return -1;
    }
    if (idx := indexOfBytes(s[start..line_end], "-----", 0)) {
        return (start + idx) as i64;
    }
    return -1;
}

fn pemOnlySpacesTabs(s: string, start: usize, end: usize): bool {
    let p = start;
    const v_sp: PemVec16 = @splat(32 as u8);
    const v_tab: PemVec16 = @splat(9 as u8);
    const v_one: PemVec16 = @splat(1 as u8);
    const v_zero: PemVec16 = @splat(0 as u8);
    while (p + 16 <= end) {
        const v: PemVec16 = s[p .. p + 16][0..16].*;
        const ok = @select(u8, v == v_sp, v_one, @select(u8, v == v_tab, v_one, v_zero));
        if (!@reduce(.And, ok != v_zero)) {
            return false;
        }
        p += 16;
    }
    while (p < end) {
        const ch = s[p];
        if (ch != (' ' as u8) && ch != ('\t' as u8)) {
            return false;
        }
        p = p + 1;
    }
    return true;
}

fn pemSliceEq(s: string, a_start: usize, a_len: usize, b_start: usize, b_len: usize): bool {
    if (a_len != b_len) {
        return false;
    }
    return bytesEq(s[a_start..a_start + a_len], s[b_start..b_start + b_len]);
}

/// Computes the exact byte length of a PEM-encoded document for a given
/// `label_len`, `der_len`, and line-ending style (`crlf` = `\r\n`, else `\n`).
pub fn pemEncodedLen(label_len: usize, der_len: usize, crlf: bool): usize {
    let nl_len: usize = 1;
    if (crlf) {
        nl_len = 2;
    }
    let b64_len: usize = 0;
    let body_lines: usize = 0;
    if (der_len > 0) {
        b64_len = ((der_len + 2) / 3) * 4;
        body_lines = (b64_len + PEM_LINE_WIDTH - 1) / PEM_LINE_WIDTH;
    }
    // "-----BEGIN " (11) + label + "-----" (5) + nl
    const header_len = (16 as usize) + label_len + nl_len;
    // base64 + line endings
    const body_len = b64_len + body_lines * nl_len;
    // "-----END " (9) + label + "-----" (5) + nl
    const footer_len = (14 as usize) + label_len + nl_len;
    return header_len + body_len + footer_len;
}

/// Encodes `der` bytes into RFC 7468 PEM format with 64-character wrapped Base64 lines.
/// Returns the number of bytes written on success, or a negative `PEM_ERR_*` code.
pub fn encodePem(label: string, der: []const u8, line_ending_crlf: bool, out: []u8): i64 {
    if (!isValidLabel(label)) {
        return PEM_ERR_INVALID_LABEL;
    }
    const need = pemEncodedLen(label.len, der.len, line_ending_crlf);
    if (out.len < need) {
        return PEM_ERR_BUFFER_TOO_SMALL;
    }

    let pos: usize = 0;
    pos = pemWriteBytes(out, pos, "-----BEGIN ");
    pos = pemWriteBytes(out, pos, label);
    pos = pemWriteBytes(out, pos, "-----");
    pos = pemWriteNewline(out, pos, line_ending_crlf);

    // Encode 48 DER bytes per line -> exactly 64 Base64 characters per line via @cap/base64ct
    let chunk: [48]u8 = undefined;
    let di: usize = 0;
    while (di < der.len) {
        let take: usize = der.len - di;
        if (take > (48 as usize)) {
            take = 48;
        }
        @memcpy(chunk[0..take], der[di .. di + take]);
        const b64_chunk_len = ((take + 2) / 3) * 4;
        const written = encodeStd(chunk[0..take], out[pos..pos + b64_chunk_len]);
        if (written != b64_chunk_len) {
            return PEM_ERR_BUFFER_TOO_SMALL;
        }
        pos = pos + written;
        pos = pemWriteNewline(out, pos, line_ending_crlf);
        di = di + take;
    }

    pos = pemWriteBytes(out, pos, "-----END ");
    pos = pemWriteBytes(out, pos, label);
    pos = pemWriteBytes(out, pos, "-----");
    pos = pemWriteNewline(out, pos, line_ending_crlf);

    return pos as i64;
}

/// Locates the first RFC 7468 PEM block in `input` (skipping leading explanatory text),
/// validating header, footer, and matching labels without decoding Base64.
pub fn locatePem(input: string): PemSlice {
    let pos: usize = 0;
    let found_begin = false;
    let label_start: usize = 0;
    let label_len: usize = 0;
    let body_start: usize = 0;

    while (pos < input.len) {
        const found = indexOfBytes(input, "-----BEGIN ", pos) ?? input.len;
        if (found >= input.len) {
            break;
        }
        if (found > 0 && input[found - 1] != ('\n' as u8) && input[found - 1] != ('\r' as u8)) {
            pos = found + 1;
            continue;
        }
        const line_start = found;
        const line_end = pemAdvanceLineEnd(input, line_start);
        const next_line = pemSkipLineEnding(input, line_end);
        const l_start = line_start + 11;
        const dash_pos = pemFindDashes(input, l_start, line_end);
        if (dash_pos < 0) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_INVALID_HEADER,
            };
        }
        const d_idx = dash_pos as usize;
        const l_len = d_idx - l_start;
        if (!pemOnlySpacesTabs(input, d_idx + 5, line_end)) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_INVALID_HEADER,
            };
        }
        if (!pemIsLabelValidBytes(input, l_start, l_len)) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_INVALID_LABEL,
            };
        }
        found_begin = true;
        label_start = l_start;
        label_len = l_len;
        body_start = next_line;
        pos = next_line;
        break;
    }

    if (!found_begin) {
        return PemSlice {
            label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
            err: PEM_ERR_MISSING_BEGIN,
        };
    }

    while (pos < input.len) {
        const found = indexOfBytes(input, "-----END ", pos) ?? input.len;
        if (found >= input.len) {
            break;
        }
        if (found > 0 && input[found - 1] != ('\n' as u8) && input[found - 1] != ('\r' as u8)) {
            pos = found + 1;
            continue;
        }
        const line_start = found;
        const line_end = pemAdvanceLineEnd(input, line_start);
        const next_line = pemSkipLineEnding(input, line_end);
        const e_start = line_start + 9;
        const dash_pos = pemFindDashes(input, e_start, line_end);
        if (dash_pos < 0) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_INVALID_FOOTER,
            };
        }
        const d_idx = dash_pos as usize;
        const e_len = d_idx - e_start;
        if (!pemOnlySpacesTabs(input, d_idx + 5, line_end)) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_INVALID_FOOTER,
            };
        }
        if (!pemSliceEq(input, label_start, label_len, e_start, e_len)) {
            return PemSlice {
                label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
                err: PEM_ERR_LABEL_MISMATCH,
            };
        }
        return PemSlice {
            label_start: label_start,
            label_len: label_len,
            body_start: body_start,
            body_len: line_start - body_start,
            end_pos: next_line,
            err: PEM_OK,
        };
    }

    return PemSlice {
        label_start: 0, label_len: 0, body_start: 0, body_len: 0, end_pos: 0,
        err: PEM_ERR_MISSING_END,
    };
}

/// Quickly extracts the PEM header label into `label_out` without decoding the body.
/// Returns the label length on success, or a negative `PEM_ERR_*` code.
pub fn decodeLabel(input: string, label_out: []u8): i64 {
    let pos: usize = 0;
    while (pos < input.len) {
        const found = indexOfBytes(input, "-----BEGIN ", pos) ?? input.len;
        if (found >= input.len) {
            break;
        }
        if (found > 0 && input[found - 1] != ('\n' as u8) && input[found - 1] != ('\r' as u8)) {
            pos = found + 1;
            continue;
        }
        const line_start = found;
        const line_end = pemAdvanceLineEnd(input, line_start);
        const l_start = line_start + 11;
        const dash_pos = pemFindDashes(input, l_start, line_end);
        if (dash_pos < 0) {
            return PEM_ERR_INVALID_HEADER;
        }
        const d_idx = dash_pos as usize;
        const l_len = d_idx - l_start;
        if (!pemOnlySpacesTabs(input, d_idx + 5, line_end)) {
            return PEM_ERR_INVALID_HEADER;
        }
        if (!pemIsLabelValidBytes(input, l_start, l_len)) {
            return PEM_ERR_INVALID_LABEL;
        }
        if (label_out.len < l_len) {
            return PEM_ERR_BUFFER_TOO_SMALL;
        }
        if (l_len > 0) {
            @memcpy(label_out[0..l_len], input[l_start .. l_start + l_len]);
        }
        return l_len as i64;
    }
    return PEM_ERR_MISSING_BEGIN;
}

/// Decodes an RFC 7468 PEM document into `label_out` and `der_out`.
/// Skips leading explanatory text/whitespace, verifies matching BEGIN/END labels,
/// and decodes Base64 payload via `@cap/base64ct` (`decodeStd`).
/// Returns the number of decoded DER bytes on success, or a negative `PEM_ERR_*` code.
pub fn decodePem(input: string, label_out: []u8, label_len_out: *usize, der_out: []u8): i64 {
    label_len_out.* = 0;
    const loc = locatePem(input);
    if (loc.err != PEM_OK) {
        return loc.err;
    }
    if (label_out.len < loc.label_len) {
        return PEM_ERR_BUFFER_TOO_SMALL;
    }
    if (loc.label_len > 0) {
        @memcpy(label_out[0..loc.label_len], input[loc.label_start .. loc.label_start + loc.label_len]);
    }
    label_len_out.* = loc.label_len;

    // Stream Base64 body in 64-byte chunks into @cap/base64ct decodeStd (zero heap allocation)
    let b64_buf: [64]u8 = undefined;
    let b64_len: usize = 0;
    let der_pos: usize = 0;
    let seen_pad = false;
    let pad_count: usize = 0;

    const v_sp: PemVec16 = @splat(32 as u8);
    const v_eq: PemVec16 = @splat(61 as u8);

    let i: usize = loc.body_start;
    const body_end = loc.body_start + loc.body_len;
    while (i < body_end) {
        if (!seen_pad && b64_len + 16 <= 64 && i + 16 <= body_end) {
            const v: PemVec16 = input[i .. i + 16][0..16].*;
            if (@reduce(.And, v > v_sp) && !@reduce(.Or, v == v_eq)) {
                @memcpy(b64_buf[b64_len .. b64_len + 16], input[i .. i + 16]);
                b64_len += 16;
                i += 16;
                if (b64_len == (64 as usize)) {
                    if (der_pos + 48 > der_out.len) {
                        return PEM_ERR_BUFFER_TOO_SMALL;
                    }
                    const res = decodeStd(b64_buf[0..64], der_out[der_pos..der_out.len]);
                    if (!res.ok) {
                        return PEM_ERR_INVALID_BASE64;
                    }
                    der_pos = der_pos + res.len;
                    b64_len = 0;
                }
                continue;
            }
        }
        const ch = input[i];
        if (PEM_WS_LUT[ch as usize] != 0) {
            i = i + 1;
            continue;
        }
        if (ch == ('=' as u8)) {
            seen_pad = true;
            pad_count = pad_count + 1;
            if (pad_count > 2) {
                return PEM_ERR_INVALID_BASE64;
            }
        } else if (seen_pad) {
            // Non-whitespace character after '=' padding is invalid
            return PEM_ERR_INVALID_BASE64;
        }

        b64_buf[b64_len] = ch;
        b64_len = b64_len + 1;

        // Flush full 64-byte block only if no padding has been seen yet
        if (b64_len == (64 as usize) && !seen_pad) {
            if (der_pos + 48 > der_out.len) {
                return PEM_ERR_BUFFER_TOO_SMALL;
            }
            const res = decodeStd(b64_buf[0..64], der_out[der_pos..der_out.len]);
            if (!res.ok) {
                return PEM_ERR_INVALID_BASE64;
            }
            der_pos = der_pos + res.len;
            b64_len = 0;
        } else if (b64_len == (64 as usize) && seen_pad) {
            // If a padded block filled 64 bytes, flush it now and require only whitespace afterward
            const expected_out = (48 as usize) - pad_count;
            if (der_pos + expected_out > der_out.len) {
                return PEM_ERR_BUFFER_TOO_SMALL;
            }
            const res = decodeStd(b64_buf[0..64], der_out[der_pos..der_out.len]);
            if (!res.ok) {
                return PEM_ERR_INVALID_BASE64;
            }
            der_pos = der_pos + res.len;
            b64_len = 0;
        }
        i = i + 1;
    }

    if (b64_len > 0) {
        if ((b64_len % 4) != 0) {
            return PEM_ERR_INVALID_BASE64;
        }
        const expected_out = (b64_len / 4) * 3 - pad_count;
        if (der_pos + expected_out > der_out.len) {
            return PEM_ERR_BUFFER_TOO_SMALL;
        }
        const res = decodeStd(b64_buf[0..b64_len], der_out[der_pos..der_out.len]);
        if (!res.ok) {
            return PEM_ERR_INVALID_BASE64;
        }
        der_pos = der_pos + res.len;
    }

    return der_pos as i64;
}

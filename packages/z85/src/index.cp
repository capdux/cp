// @cap/base85 — Zero-allocation ZeroMQ Z85 (RFC 32) & Adobe Ascii85 codec for CP
// Clean-room MIT implementation inspired by Rust `z85`, `ascii85`, and RFC 32.
//
// Features:
// - ZeroMQ Z85 (`encodeZ85`, `decodeZ85`): RFC 32 85-char safe alphabet, strict 4/5-byte framing,
//   u32 overflow rejection.
// - Adobe Ascii85 (`encodeAscii85`, `decodeAscii85`): arbitrary input length, `'z'` all-zero block
//   compression, optional `<~...~>` framing, whitespace tolerance, and strict overflow/char validation.
// - Zero heap allocation: writes directly into caller-provided `out: []u8`.

const Z85_ALPHABET: string = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#";

fn z85Index(c: u8) -> i8 {
    let i: usize = 0;
    while (i < 85) {
        if (Z85_ALPHABET[i] == c) {
            return i as i8;
        }
        i += 1;
    }
    return -1;
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

// ============================================================================
// 1. ZeroMQ Z85 (RFC 32)
// ============================================================================

/// Encodes `input` using ZeroMQ Z85 (RFC 32) into caller-provided `out`.
/// Strictly requires `input.len % 4 == 0` and sufficient `out` capacity (`input.len / 4 * 5`).
pub fn encodeZ85(input: string, out: []u8) -> string? {
    if ((input.len % 4) != 0) {
        return null;
    }
    const needed = (input.len / 4) * 5;
    if (needed > out.len) {
        return null;
    }

    let in_pos: usize = 0;
    let out_pos: usize = 0;
    while (in_pos < input.len) {
        let v: u32 = ((input[in_pos] as u32) << 24) |
            ((input[in_pos + 1] as u32) << 16) |
            ((input[in_pos + 2] as u32) << 8) |
            (input[in_pos + 3] as u32);
        in_pos += 4;

        out[out_pos + 4] = Z85_ALPHABET[(v % 85) as usize];
        v = v / 85;
        out[out_pos + 3] = Z85_ALPHABET[(v % 85) as usize];
        v = v / 85;
        out[out_pos + 2] = Z85_ALPHABET[(v % 85) as usize];
        v = v / 85;
        out[out_pos + 1] = Z85_ALPHABET[(v % 85) as usize];
        v = v / 85;
        out[out_pos] = Z85_ALPHABET[(v % 85) as usize];
        out_pos += 5;
    }

    return out[0..out_pos];
}

/// Decodes a ZeroMQ Z85 (RFC 32) string into caller-provided `out`.
/// Strictly requires `encoded.len % 5 == 0`, valid Z85 characters, and no `u32` block overflow.
pub fn decodeZ85(encoded: string, out: []u8) -> string? {
    if ((encoded.len % 5) != 0) {
        return null;
    }
    const needed = (encoded.len / 5) * 4;
    if (needed > out.len) {
        return null;
    }

    let in_pos: usize = 0;
    let out_pos: usize = 0;
    while (in_pos < encoded.len) {
        let acc: u64 = 0;
        let k: usize = 0;
        while (k < 5) {
            const d = z85Index(encoded[in_pos + k]);
            if (d < 0) {
                return null;
            }
            acc = (acc * 85) + (d as u64);
            k += 1;
        }
        if (acc > (0xFFFFFFFF as u64)) {
            return null;
        }
        const v: u32 = acc as u32;
        out[out_pos] = ((v >> 24) & 0xFF) as u8;
        out[out_pos + 1] = ((v >> 16) & 0xFF) as u8;
        out[out_pos + 2] = ((v >> 8) & 0xFF) as u8;
        out[out_pos + 3] = (v & 0xFF) as u8;
        in_pos += 5;
        out_pos += 4;
    }

    return out[0..out_pos];
}

// ============================================================================
// 2. Adobe Ascii85
// ============================================================================

/// Encodes arbitrary-length `input` using Adobe Ascii85 (`'!'..'u'`) into `out`.
/// Compresses full 4-byte zero blocks into `'z'` and emits `rem + 1` chars for partial trailing blocks.
pub fn encodeAscii85(input: string, out: []u8) -> string? {
    let in_pos: usize = 0;
    let out_pos: usize = 0;

    while (in_pos + 4 <= input.len) {
        let v: u32 = ((input[in_pos] as u32) << 24) |
            ((input[in_pos + 1] as u32) << 16) |
            ((input[in_pos + 2] as u32) << 8) |
            (input[in_pos + 3] as u32);
        in_pos += 4;

        if (v == 0) {
            if (out_pos >= out.len) {
                return null;
            }
            out[out_pos] = 'z';
            out_pos += 1;
            continue;
        }

        if (out_pos + 5 > out.len) {
            return null;
        }
        out[out_pos + 4] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        out[out_pos + 3] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        out[out_pos + 2] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        out[out_pos + 1] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        out[out_pos] = (33 as u8) + ((v % 85) as u8);
        out_pos += 5;
    }

    // Partial trailing block (1..3 bytes)
    const rem = input.len - in_pos;
    if (rem > 0) {
        const emit_len = rem + 1;
        if (out_pos + emit_len > out.len) {
            return null;
        }
        let v: u32 = 0;
        let k: usize = 0;
        while (k < rem) {
            v = v | ((input[in_pos + k] as u32) << (((3 - k) * 8) as u5));
            k += 1;
        }
        let tmp: [5]u8 = undefined;
        tmp[4] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        tmp[3] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        tmp[2] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        tmp[1] = (33 as u8) + ((v % 85) as u8);
        v = v / 85;
        tmp[0] = (33 as u8) + ((v % 85) as u8);

        let j: usize = 0;
        while (j < emit_len) {
            out[out_pos + j] = tmp[j];
            j += 1;
        }
        out_pos += emit_len;
    }

    return out[0..out_pos];
}

/// Decodes Adobe Ascii85 into `out`. Automatically skips whitespace and optional `<~` / `~>` framing,
/// expands `'z'` into 4 zero bytes, and rejects malformed characters, misplaced `'z'`, or `u32` overflows.
pub fn decodeAscii85(encoded: string, out: []u8) -> string? {
    let start: usize = 0;
    let end: usize = encoded.len;

    while (start < end && isSpace(encoded[start])) {
        start += 1;
    }
    while (end > start && isSpace(encoded[end - 1])) {
        end -= 1;
    }

    // Strip optional Adobe `<~` ... `~>` delimiters
    if (end - start >= 2 && encoded[start] == '<' && encoded[start + 1] == '~') {
        start += 2;
    }
    if (end - start >= 2 && encoded[end - 2] == '~' && encoded[end - 1] == '>') {
        end -= 2;
    }

    let out_pos: usize = 0;
    let acc: u64 = 0;
    let group_count: usize = 0;

    let i = start;
    while (i < end) {
        const c = encoded[i];
        i += 1;

        if (isSpace(c)) {
            continue;
        }

        if (c == 'z') {
            if (group_count != 0) {
                return null;
            }
            if (out_pos + 4 > out.len) {
                return null;
            }
            out[out_pos] = 0;
            out[out_pos + 1] = 0;
            out[out_pos + 2] = 0;
            out[out_pos + 3] = 0;
            out_pos += 4;
            continue;
        }

        if (c < 33 || c > 117) {
            return null;
        }

        acc = (acc * 85) + ((c - 33) as u64);
        group_count += 1;

        if (group_count == 5) {
            if (acc > (0xFFFFFFFF as u64)) {
                return null;
            }
            if (out_pos + 4 > out.len) {
                return null;
            }
            const v: u32 = acc as u32;
            out[out_pos] = ((v >> 24) & 0xFF) as u8;
            out[out_pos + 1] = ((v >> 16) & 0xFF) as u8;
            out[out_pos + 2] = ((v >> 8) & 0xFF) as u8;
            out[out_pos + 3] = (v & 0xFF) as u8;
            out_pos += 4;
            acc = 0;
            group_count = 0;
        }
    }

    if (group_count == 1) {
        return null;
    }
    if (group_count > 1) {
        // Pad remaining slots with 'u' (84)
        let pad = 5 - group_count;
        while (pad > 0) {
            acc = (acc * 85) + 84;
            pad -= 1;
        }
        if (acc > (0xFFFFFFFF as u64)) {
            return null;
        }
        const emit_bytes = group_count - 1;
        if (out_pos + emit_bytes > out.len) {
            return null;
        }
        const v: u32 = acc as u32;
        let k: usize = 0;
        while (k < emit_bytes) {
            out[out_pos + k] = ((v >> (((3 - k) * 8) as u5)) & 0xFF) as u8;
            k += 1;
        }
        out_pos += emit_bytes;
    }

    return out[0..out_pos];
}

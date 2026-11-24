// Source: Absorbed from /data/dev/bun/src/runtime/webcore/TextDecoder.cp | Synced: 2026-11-23T20:50:27Z

const UTF8_FFFD: [3]u8 = [0xEF, 0xBF, 0xBD];

/// Supported WHATWG text encodings for zero-alloc streaming `TextDecoder`.
pub enum TextEncoding {
    Utf8,
    Utf16Le,
    Utf16Be,
    Latin1,
}

/// Summary returned by `TextDecoder.decode`.
pub struct DecodeResult {
    bytes_written: usize,
    had_error: bool,
}

/// Sniffs a Unicode Byte Order Mark (BOM) prefix (`EF BB BF`, `FF FE`, or `FE FF`).
pub fn sniffBom(data: []const u8): TextEncoding? {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return TextEncoding.Utf8;
    }
    if (data.len >= 2 && data[0] == 0xFF && data[1] == 0xFE) {
        return TextEncoding.Utf16Le;
    }
    if (data.len >= 2 && data[0] == 0xFE && data[1] == 0xFF) {
        return TextEncoding.Utf16Be;
    }
    return null;
}

fn isLeadSurrogate(cu: u16): bool {
    return cu >= 0xD800 && cu <= 0xDBFF;
}

fn isTrailSurrogate(cu: u16): bool {
    return cu >= 0xDC00 && cu <= 0xDFFF;
}

fn writeUtf8CodePoint(cp: u32, out_utf8: *[]u8, pos: *usize): bool {
    const p = pos.*;
    if (cp < 0x80) {
        if (out_utf8.*.len - p < 1) return false;
        out_utf8.*[p] = cp as u8;
        pos.* = p + 1;
        return true;
    }
    if (cp < 0x800) {
        if (out_utf8.*.len - p < 2) return false;
        out_utf8.*[p] = (0xC0 as u8) | ((cp >> 6) as u8);
        out_utf8.*[p + 1] = (0x80 as u8) | ((cp & 0x3F) as u8);
        pos.* = p + 2;
        return true;
    }
    if (cp < 0x10000) {
        if (out_utf8.*.len - p < 3) return false;
        out_utf8.*[p] = (0xE0 as u8) | ((cp >> 12) as u8);
        out_utf8.*[p + 1] = (0x80 as u8) | (((cp >> 6) & 0x3F) as u8);
        out_utf8.*[p + 2] = (0x80 as u8) | ((cp & 0x3F) as u8);
        pos.* = p + 3;
        return true;
    }
    if (out_utf8.*.len - p < 4) return false;
    out_utf8.*[p] = (0xF0 as u8) | ((cp >> 18) as u8);
    out_utf8.*[p + 1] = (0x80 as u8) | (((cp >> 12) & 0x3F) as u8);
    out_utf8.*[p + 2] = (0x80 as u8) | (((cp >> 6) & 0x3F) as u8);
    out_utf8.*[p + 3] = (0x80 as u8) | ((cp & 0x3F) as u8);
    pos.* = p + 4;
    return true;
}

fn writeReplacement(out_utf8: *[]u8, pos: *usize): bool {
    const p = pos.*;
    if (out_utf8.*.len - p < 3) {
        return false;
    }
    out_utf8.*[p] = UTF8_FFFD[0];
    out_utf8.*[p + 1] = UTF8_FFFD[1];
    out_utf8.*[p + 2] = UTF8_FFFD[2];
    pos.* = p + 3;
    return true;
}

/// WHATWG Encoding Standard streaming `TextDecoder` (zero-alloc state machine,
/// absorbed from Bun `src/runtime/webcore/TextDecoder.cp`).
/// Decodes UTF-8, UTF-16LE, UTF-16BE, and Latin-1 byte chunks into canonical UTF-8.
pub struct TextDecoder {
    encoding: TextEncoding,
    fatal: bool,
    ignore_bom: bool,
    bom_checked: bool,
    utf8_needed: u8,
    utf8_seen: u8,
    utf8_lower: u8,
    utf8_upper: u8,
    utf8_cp: u32,
    lead_byte: u8?,
    lead_surrogate: u16?,

    /// Initializes a zero-alloc streaming `TextDecoder`.
    fn init(encoding: TextEncoding, fatal: bool, ignore_bom: bool): TextDecoder {
        return TextDecoder {
            encoding: encoding,
            fatal: fatal,
            ignore_bom: ignore_bom,
            bom_checked: false,
            utf8_needed: 0,
            utf8_seen: 0,
            utf8_lower: 0x80,
            utf8_upper: 0xBF,
            utf8_cp: 0,
            lead_byte: null,
            lead_surrogate: null,
        };
    }

    /// Resets all streaming state (pending bytes, surrogate state, and BOM flag).
    fn reset(*_) {
        _.bom_checked = false;
        _.utf8_needed = 0;
        _.utf8_seen = 0;
        _.utf8_lower = 0x80;
        _.utf8_upper = 0xBF;
        _.utf8_cp = 0;
        _.lead_byte = null;
        _.lead_surrogate = null;
    }

    /// Returns true if incomplete multi-byte/surrogate state is buffered across chunks.
    fn hasPendingBytes(): bool {
        return _.utf8_needed > 0 || _.lead_byte != null || _.lead_surrogate != null;
    }

    fn emitScalar(*_, cp: u32, out_utf8: *[]u8, pos: *usize): bool {
        if (!_.bom_checked) {
            _.bom_checked = true;
            if (!_.ignore_bom && cp == 0xFEFF) {
                return true;
            }
        }
        return writeUtf8CodePoint(cp, out_utf8, pos);
    }

    fn decodeUtf8Chunk(*_, input: []const u8, stream: bool, out_utf8: *[]u8): DecodeResult? {
        let pos: usize = 0;
        let had_error: bool = false;
        let i: usize = 0;

        while (i < input.len) {
            const b = input[i];
            if (_.utf8_needed == 0) {
                if (b <= 0x7F) {
                    if (!_.emitScalar(b as u32, out_utf8, &pos)) return null;
                    i += 1;
                } else if (b >= 0xC2 && b <= 0xDF) {
                    _.utf8_needed = 1;
                    _.utf8_seen = 0;
                    _.utf8_cp = (b & 0x1F) as u32;
                    _.utf8_lower = 0x80;
                    _.utf8_upper = 0xBF;
                    i += 1;
                } else if (b >= 0xE0 && b <= 0xEF) {
                    _.utf8_needed = 2;
                    _.utf8_seen = 0;
                    _.utf8_cp = (b & 0x0F) as u32;
                    _.utf8_lower = if (b == 0xE0) (0xA0 as u8) else (0x80 as u8);
                    _.utf8_upper = if (b == 0xED) (0x9F as u8) else (0xBF as u8);
                    i += 1;
                } else if (b >= 0xF0 && b <= 0xF4) {
                    _.utf8_needed = 3;
                    _.utf8_seen = 0;
                    _.utf8_cp = (b & 0x07) as u32;
                    _.utf8_lower = if (b == 0xF0) (0x90 as u8) else (0x80 as u8);
                    _.utf8_upper = if (b == 0xF4) (0x8F as u8) else (0xBF as u8);
                    i += 1;
                } else {
                    if (_.fatal) return null;
                    had_error = true;
                    _.bom_checked = true;
                    if (!writeReplacement(out_utf8, &pos)) return null;
                    i += 1;
                }
            } else {
                if (b < _.utf8_lower || b > _.utf8_upper) {
                    // Invalid continuation byte: emit U+FFFD and re-process `b` as a new lead byte.
                    _.utf8_needed = 0;
                    _.utf8_seen = 0;
                    _.utf8_cp = 0;
                    _.utf8_lower = 0x80;
                    _.utf8_upper = 0xBF;
                    if (_.fatal) return null;
                    had_error = true;
                    _.bom_checked = true;
                    if (!writeReplacement(out_utf8, &pos)) return null;
                    continue;
                }
                _.utf8_lower = 0x80;
                _.utf8_upper = 0xBF;
                _.utf8_cp = (_.utf8_cp << 6) | ((b & 0x3F) as u32);
                _.utf8_seen += 1;
                i += 1;
                if (_.utf8_seen == _.utf8_needed) {
                    const cp = _.utf8_cp;
                    _.utf8_needed = 0;
                    _.utf8_seen = 0;
                    _.utf8_cp = 0;
                    if (!_.emitScalar(cp, out_utf8, &pos)) return null;
                }
            }
        }

        if (!stream) {
            if (_.utf8_needed > 0) {
                _.utf8_needed = 0;
                _.utf8_seen = 0;
                _.utf8_cp = 0;
                _.utf8_lower = 0x80;
                _.utf8_upper = 0xBF;
                if (_.fatal) return null;
                had_error = true;
                _.bom_checked = true;
                if (!writeReplacement(out_utf8, &pos)) return null;
            }
            _.reset();
        }

        return DecodeResult {
            bytes_written: pos,
            had_error: had_error,
        };
    }

    fn processUtf16Unit(*_, cu: u16, out_utf8: *[]u8, pos: *usize, had_error: *bool): bool {
        if (lead := _.lead_surrogate) {
            _.lead_surrogate = null;
            if (isTrailSurrogate(cu)) {
                const hi: u32 = (lead - 0xD800) as u32;
                const lo: u32 = (cu - 0xDC00) as u32;
                const cp: u32 = (0x10000 as u32) + (hi << 10) + lo;
                return _.emitScalar(cp, out_utf8, pos);
            }
            if (_.fatal) return false;
            had_error.* = true;
            _.bom_checked = true;
            if (!writeReplacement(out_utf8, pos)) return false;
        }

        if (isLeadSurrogate(cu)) {
            _.lead_surrogate = cu;
            return true;
        }
        if (isTrailSurrogate(cu)) {
            if (_.fatal) return false;
            had_error.* = true;
            _.bom_checked = true;
            return writeReplacement(out_utf8, pos);
        }
        return _.emitScalar(cu as u32, out_utf8, pos);
    }

    fn decodeUtf16Chunk(*_, input: []const u8, big_endian: bool, stream: bool, out_utf8: *[]u8): DecodeResult? {
        let pos: usize = 0;
        let had_error: bool = false;
        let i: usize = 0;

        while (i < input.len) {
            if (first_b := _.lead_byte) {
                _.lead_byte = null;
                const b0: u16 = first_b as u16;
                const b1: u16 = input[i] as u16;
                const cu: u16 = if (big_endian) ((b0 << 8) | b1) else ((b1 << 8) | b0);
                if (!_.processUtf16Unit(cu, out_utf8, &pos, &had_error)) return null;
                i += 1;
            } else if (i + 1 < input.len) {
                const b0: u16 = input[i] as u16;
                const b1: u16 = input[i + 1] as u16;
                const cu: u16 = if (big_endian) ((b0 << 8) | b1) else ((b1 << 8) | b0);
                if (!_.processUtf16Unit(cu, out_utf8, &pos, &had_error)) return null;
                i += 2;
            } else {
                _.lead_byte = input[i];
                i += 1;
            }
        }

        if (!stream) {
            if (_.lead_byte != null || _.lead_surrogate != null) {
                const had_both = _.lead_byte != null && _.lead_surrogate != null;
                _.lead_byte = null;
                _.lead_surrogate = null;
                if (_.fatal) return null;
                had_error = true;
                _.bom_checked = true;
                if (!writeReplacement(out_utf8, &pos)) return null;
                if (had_both) {
                    if (!writeReplacement(out_utf8, &pos)) return null;
                }
            }
            _.reset();
        }

        return DecodeResult {
            bytes_written: pos,
            had_error: had_error,
        };
    }

    fn decodeLatin1Chunk(*_, input: []const u8, stream: bool, out_utf8: *[]u8): DecodeResult? {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            if (!writeUtf8CodePoint(input[i] as u32, out_utf8, &pos)) {
                return null;
            }
            i += 1;
        }
        if (!stream) {
            _.reset();
        }
        return DecodeResult {
            bytes_written: pos,
            had_error: false,
        };
    }

    /// Decodes a byte chunk into canonical UTF-8 in `out_utf8`.
    /// Set `stream = true` for intermediate chunks and `stream = false` for the final chunk / EOF flush.
    /// Returns `null` on `fatal` decoding error or insufficient `out_utf8` capacity.
    fn decode(*_, input: []const u8, stream: bool, out_utf8: *[]u8): DecodeResult? {
        match (_.encoding) {
            TextEncoding.Utf8 => return _.decodeUtf8Chunk(input, stream, out_utf8),
            TextEncoding.Utf16Le => return _.decodeUtf16Chunk(input, false, stream, out_utf8),
            TextEncoding.Utf16Be => return _.decodeUtf16Chunk(input, true, stream, out_utf8),
            TextEncoding.Latin1 => return _.decodeLatin1Chunk(input, stream, out_utf8),
        }
    }
}

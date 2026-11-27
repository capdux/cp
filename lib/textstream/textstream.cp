// Source: Absorbed from /data/dev/bun/src/runtime/webcore/TextEncoderStreamEncoder.cp | Synced: 2026-11-22T00:18:28Z

/// UTF-8 byte sequence for Unicode Replacement Character U+FFFD (`0xEF, 0xBF, 0xBD`).
pub const UTF8_REPLACEMENT: [3]u8 = [0xEF, 0xBF, 0xBD];

/// Returns true if UTF-16 code unit `cu` is a high (lead) surrogate (`U+D800..U+DBFF`).
pub fn isLeadSurrogate(cu: u16): bool {
    return cu >= 0xD800 && cu <= 0xDBFF;
}

/// Returns true if UTF-16 code unit `cu` is a low (trail) surrogate (`U+DC00..U+DFFF`).
pub fn isTrailSurrogate(cu: u16): bool {
    return cu >= 0xDC00 && cu <= 0xDFFF;
}

/// Combines a valid UTF-16 surrogate pair `(lead, trail)` into a supplementary Unicode scalar value (`0x10000..0x10FFFF`).
pub fn combineSurrogatePair(lead: u16, trail: u16): u32? {
    if (!isLeadSurrogate(lead) || !isTrailSurrogate(trail)) {
        return null;
    }
    const hi: u32 = (lead - 0xD800) as u32;
    const lo: u32 = (trail - 0xDC00) as u32;
    return 0x10000 + (hi << 10) + lo;
}

/// Encodes a valid Unicode scalar value `cp` into 1..4 UTF-8 bytes in `out_buf`.
/// Returns `null` if `cp` is a surrogate (`0xD800..0xDFFF`), `> 0x10FFFF`, or `out_buf` is too small.
pub fn encodeCodePointUtf8(cp: u32, out_buf: *[]u8): usize? {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return null;
    }
    if (cp < 0x80) {
        if (out_buf.*.len < 1) {
            return null;
        }
        out_buf.*[0] = cp as u8;
        return 1;
    }
    if (cp < 0x800) {
        if (out_buf.*.len < 2) {
            return null;
        }
        out_buf.*[0] = (0xC0 as u8) | ((cp >> 6) as u8);
        out_buf.*[1] = (0x80 as u8) | ((cp & 0x3F) as u8);
        return 2;
    }
    if (cp < 0x10000) {
        if (out_buf.*.len < 3) {
            return null;
        }
        out_buf.*[0] = (0xE0 as u8) | ((cp >> 12) as u8);
        out_buf.*[1] = (0x80 as u8) | (((cp >> 6) & 0x3F) as u8);
        out_buf.*[2] = (0x80 as u8) | ((cp & 0x3F) as u8);
        return 3;
    }
    if (out_buf.*.len < 4) {
        return null;
    }
    out_buf.*[0] = (0xF0 as u8) | ((cp >> 18) as u8);
    out_buf.*[1] = (0x80 as u8) | (((cp >> 12) & 0x3F) as u8);
    out_buf.*[2] = (0x80 as u8) | (((cp >> 6) & 0x3F) as u8);
    out_buf.*[3] = (0x80 as u8) | ((cp & 0x3F) as u8);
    return 4;
}

/// Computes exact UTF-8 byte length required to encode Latin-1 `input`.
pub fn utf8ByteLengthFromLatin1(input: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        total += if (input[i] < 0x80) (1 as usize) else (2 as usize);
        i += 1;
    }
    return total;
}

/// Computes exact UTF-8 byte length emitted by `TextEncoderStream.encodeUtf16` for `input`
/// given whether a lead surrogate is currently pending from the previous chunk.
pub fn utf8ByteLengthFromUtf16(input: []const u16, has_pending_lead: bool): usize {
    if (input.len == 0) {
        return 0;
    }
    let total: usize = 0;
    let i: usize = 0;
    if (has_pending_lead) {
        if (isTrailSurrogate(input[0])) {
            total += 4;
            i = 1;
        } else {
            total += 3;
        }
    }
    while (i < input.len) {
        const cu = input[i];
        if (isLeadSurrogate(cu)) {
            if (i + 1 < input.len) {
                if (isTrailSurrogate(input[i + 1])) {
                    total += 4;
                    i += 2;
                } else {
                    total += 3;
                    i += 1;
                }
            } else {
                // Trailing lead surrogate is held in pending_lead_surrogate (0 bytes emitted now).
                i += 1;
            }
        } else if (isTrailSurrogate(cu)) {
            total += 3;
            i += 1;
        } else if (cu < 0x80) {
            total += 1;
            i += 1;
        } else if (cu < 0x800) {
            total += 2;
            i += 1;
        } else {
            total += 3;
            i += 1;
        }
    }
    return total;
}

fn writeReplacement(out_buf: *[]u8, pos: usize) {
    out_buf.*[pos + 0] = UTF8_REPLACEMENT[0];
    out_buf.*[pos + 1] = UTF8_REPLACEMENT[1];
    out_buf.*[pos + 2] = UTF8_REPLACEMENT[2];
}

/// WHATWG Encoding Standard streaming UTF-8 encoder (zero-alloc state machine,
/// absorbed from Bun `src/runtime/webcore/TextEncoderStreamEncoder.cp`).
/// Preserves cross-chunk UTF-16 lead surrogates and emits U+FFFD for lone surrogates.
pub struct TextEncoderStream {
    pending_lead_surrogate: u16?,

    /// Initializes a clean `TextEncoderStream` with no pending surrogate.
    static fn init(): TextEncoderStream {
        return TextEncoderStream {
            pending_lead_surrogate: null,
        };
    }

    /// Returns true if a trailing UTF-16 high surrogate is buffered awaiting the next chunk.
    fn hasPendingSurrogate(): bool {
        return _.pending_lead_surrogate != null;
    }

    /// Clears any buffered pending lead surrogate.
    fn reset(*_) {
        _.pending_lead_surrogate = null;
    }

    /// Encodes a Latin-1 (`u8`) chunk into UTF-8 in `out_buf`.
    /// If a lead surrogate was pending and `input.len > 0`, emits `U+FFFD` first.
    /// Returns bytes written, or `null` if `out_buf` is too small (leaving state unchanged).
    fn encodeLatin1(*_, input: []const u8, out_buf: *[]u8): usize? {
        if (input.len == 0) {
            return 0;
        }
        const prepend_len: usize = if (_.pending_lead_surrogate != null) 3 else 0;
        const needed = prepend_len + utf8ByteLengthFromLatin1(input);
        if (out_buf.*.len < needed) {
            return null;
        }

        let pos: usize = 0;
        if (_.pending_lead_surrogate != null) {
            _.pending_lead_surrogate = null;
            writeReplacement(out_buf, pos);
            pos += 3;
        }

        let i: usize = 0;
        while (i < input.len) {
            const b = input[i];
            if (b < 0x80) {
                out_buf.*[pos] = b;
                pos += 1;
            } else {
                out_buf.*[pos] = (0xC0 as u8) | (b >> 6);
                out_buf.*[pos + 1] = (0x80 as u8) | (b & 0x3F);
                pos += 2;
            }
            i += 1;
        }
        return pos;
    }

    /// Encodes a UTF-16 (`u16`) chunk into UTF-8 in `out_buf`, handling cross-chunk surrogate pairs
    /// and lone-surrogate replacement (`U+FFFD`).
    /// Returns bytes written, or `null` if `out_buf` is too small (leaving state unchanged).
    fn encodeUtf16(*_, input: []const u16, out_buf: *[]u8): usize? {
        if (input.len == 0) {
            return 0;
        }
        const needed = utf8ByteLengthFromUtf16(input, _.pending_lead_surrogate != null);
        if (out_buf.*.len < needed) {
            return null;
        }

        let pos: usize = 0;
        let i: usize = 0;

        if (lead := _.pending_lead_surrogate) {
            _.pending_lead_surrogate = null;
            if (isTrailSurrogate(input[0])) {
                const cp = combineSurrogatePair(lead, input[0]) ?? 0xFFFD;
                let tail: []u8 = out_buf.*[pos..];
                const w = encodeCodePointUtf8(cp, &tail) ?? return null;
                pos += w;
                i = 1;
            } else {
                writeReplacement(out_buf, pos);
                pos += 3;
            }
        }

        while (i < input.len) {
            const cu = input[i];
            if (isLeadSurrogate(cu)) {
                if (i + 1 < input.len) {
                    if (isTrailSurrogate(input[i + 1])) {
                        const cp = combineSurrogatePair(cu, input[i + 1]) ?? 0xFFFD;
                        let tail: []u8 = out_buf.*[pos..];
                        const w = encodeCodePointUtf8(cp, &tail) ?? return null;
                        pos += w;
                        i += 2;
                    } else {
                        writeReplacement(out_buf, pos);
                        pos += 3;
                        i += 1;
                    }
                } else {
                    _.pending_lead_surrogate = cu;
                    i += 1;
                }
            } else if (isTrailSurrogate(cu)) {
                writeReplacement(out_buf, pos);
                pos += 3;
                i += 1;
            } else {
                let tail: []u8 = out_buf.*[pos..];
                const w = encodeCodePointUtf8(cu as u32, &tail) ?? return null;
                pos += w;
                i += 1;
            }
        }

        return pos;
    }

    /// Flushes the stream at EOF: emits 3-byte `U+FFFD` if a trailing lead surrogate remains pending.
    /// Returns `0` or `3`, or `null` if `out_buf` has fewer than 3 bytes when flushing a pending surrogate.
    fn flush(*_, out_buf: *[]u8): usize? {
        if (_.pending_lead_surrogate == null) {
            return 0;
        }
        if (out_buf.*.len < 3) {
            return null;
        }
        _.pending_lead_surrogate = null;
        writeReplacement(out_buf, 0);
        return 3;
    }
}

// @cap/string-decoder — Zero-allocation streaming UTF-8 / Latin1 / UTF-16LE StringDecoder in pure CP
// Clean-room MIT implementation of npm `string_decoder` (#62, 378M/wk).

pub enum DecoderEncoding {
    Utf8,
    Latin1,
    Utf16Le,
}

fn sdecUtf8ExpectedLen(lead: u8): usize {
    if (lead < 0x80) { return 1; }
    if ((lead & 0xE0) == 0xC0) { return 2; }
    if ((lead & 0xF0) == 0xE0) { return 3; }
    if ((lead & 0xF8) == 0xF0) { return 4; }
    return 1; // Invalid lead byte treated as single replacement
}

fn sdecPushReplacement(out: []u8, pos: usize): usize {
    if (pos + 3 > out.len) {
        return pos;
    }
    out[pos] = 0xEF;
    out[pos + 1] = 0xBF;
    out[pos + 2] = 0xBD;
    return pos + 3;
}

fn sdecPushCodePointUtf8(cp: u32, out: []u8, pos: usize): usize {
    if (cp < 0x80) {
        if (pos + 1 > out.len) { return pos; }
        out[pos] = cp as u8;
        return pos + 1;
    }
    if (cp < 0x800) {
        if (pos + 2 > out.len) { return pos; }
        out[pos] = (0xC0 | (cp >> 6)) as u8;
        out[pos + 1] = (0x80 | (cp & 0x3F)) as u8;
        return pos + 2;
    }
    if (cp < 0x10000) {
        if (pos + 3 > out.len) { return pos; }
        out[pos] = (0xE0 | (cp >> 12)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | (cp & 0x3F)) as u8;
        return pos + 3;
    }
    if (pos + 4 > out.len) { return pos; }
    out[pos] = (0xF0 | (cp >> 18)) as u8;
    out[pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
    out[pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
    out[pos + 3] = (0x80 | (cp & 0x3F)) as u8;
    return pos + 4;
}

/// Streaming byte-to-string decoder preserving incomplete multi-byte sequences across `write()` calls.
pub struct StringDecoder {
    pub encoding: DecoderEncoding,
    last_char: [4]u8,
    last_need: usize,
    last_total: usize,
    high_surrogate: u16,

    pub static fn new(encoding: DecoderEncoding): StringDecoder {
        return StringDecoder {
            encoding: encoding,
            last_char: [0; 4],
            last_need: 0,
            last_total: 0,
            high_surrogate: 0,
        };
    }

    pub static fn utf8(): StringDecoder {
        return StringDecoder.new(.Utf8);
    }

    pub fn pendingBytes(): usize {
        if (_.last_total >= _.last_need) {
            return _.last_total - _.last_need;
        }
        return 0;
    }

    pub fn write(*_, chunk: []const u8, out: []u8): usize {
        if (_.encoding == .Latin1) {
            let p: usize = 0;
            let i: usize = 0;
            while (i < chunk.len) {
                p = sdecPushCodePointUtf8(chunk[i] as u32, out, p);
                i += 1;
            }
            return p;
        }

        if (_.encoding == .Utf16Le) {
            return _.writeUtf16Le(chunk, out);
        }

        // UTF-8 streaming decode
        let p: usize = 0;
        let i: usize = 0;

        // 1. Complete any buffered incomplete sequence from previous chunk
        if (_.last_need > 0) {
            let have = _.last_total - _.last_need;
            while (i < chunk.len && _.last_need > 0) {
                const b = chunk[i];
                if ((b & 0xC0) != 0x80) {
                    // Broken continuation: emit replacement and reprocess `b`
                    p = sdecPushReplacement(out, p);
                    _.last_need = 0;
                    _.last_total = 0;
                    break;
                }
                _.last_char[have] = b;
                have += 1;
                _.last_need -= 1;
                i += 1;
            }
            if (_.last_need == 0 && _.last_total > 0) {
                let k: usize = 0;
                while (k < _.last_total && p < out.len) {
                    out[p] = _.last_char[k];
                    p += 1;
                    k += 1;
                }
                _.last_total = 0;
            } else if (_.last_need > 0) {
                return p; // Still waiting for more bytes
            }
        }

        // 2. Process remaining bytes in `chunk`
        while (i < chunk.len) {
            const b = chunk[i];
            if (b < 0x80) {
                if (p < out.len) {
                    out[p] = b;
                    p += 1;
                }
                i += 1;
                continue;
            }
            const expected = sdecUtf8ExpectedLen(b);
            if (expected == 1) {
                p = sdecPushReplacement(out, p);
                i += 1;
                continue;
            }
            const avail = chunk.len - i;
            if (avail < expected) {
                // Check if available continuation bytes are valid
                let valid_prefix = true;
                let c: usize = 1;
                while (c < avail) {
                    if ((chunk[i + c] & 0xC0) != 0x80) {
                        valid_prefix = false;
                        break;
                    }
                    c += 1;
                }
                if (valid_prefix) {
                    let c2: usize = 0;
                    while (c2 < avail) {
                        _.last_char[c2] = chunk[i + c2];
                        c2 += 1;
                    }
                    _.last_total = expected;
                    _.last_need = expected - avail;
                    break;
                } else {
                    p = sdecPushReplacement(out, p);
                    i += 1;
                    continue;
                }
            }
            // Enough bytes available: verify continuation bytes
            let ok = true;
            let k: usize = 1;
            while (k < expected) {
                if ((chunk[i + k] & 0xC0) != 0x80) {
                    ok = false;
                    break;
                }
                k += 1;
            }
            if (ok) {
                let c3: usize = 0;
                while (c3 < expected && p < out.len) {
                    out[p] = chunk[i + c3];
                    p += 1;
                    c3 += 1;
                }
                i += expected;
            } else {
                p = sdecPushReplacement(out, p);
                i += 1;
            }
        }
        return p;
    }

    fn writeUtf16Le(*_, chunk: []const u8, out: []u8): usize {
        let p: usize = 0;
        let i: usize = 0;
        if (_.last_need == 1 && chunk.len > 0) {
            const unit: u16 = (_.last_char[0] as u16) | ((chunk[0] as u16) << 8);
            _.last_need = 0;
            _.last_total = 0;
            i = 1;
            p = _.emitUtf16Unit(unit, out, p);
        }
        while (i + 1 < chunk.len) {
            const unit: u16 = (chunk[i] as u16) | ((chunk[i + 1] as u16) << 8);
            p = _.emitUtf16Unit(unit, out, p);
            i += 2;
        }
        if (i < chunk.len) {
            _.last_char[0] = chunk[i];
            _.last_total = 2;
            _.last_need = 1;
        }
        return p;
    }

    fn emitUtf16Unit(*_, unit: u16, out: []u8, pos: usize): usize {
        let p = pos;
        if (_.high_surrogate != 0) {
            if (unit >= 0xDC00 && unit <= 0xDFFF) {
                const hi = (_.high_surrogate - 0xD800) as u32;
                const lo = (unit - 0xDC00) as u32;
                _.high_surrogate = 0;
                return sdecPushCodePointUtf8(0x10000 + (hi << 10) + lo, out, p);
            }
            p = sdecPushReplacement(out, p);
            _.high_surrogate = 0;
        }
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            _.high_surrogate = unit;
            return p;
        }
        if (unit >= 0xDC00 && unit <= 0xDFFF) {
            return sdecPushReplacement(out, p);
        }
        return sdecPushCodePointUtf8(unit as u32, out, p);
    }

    /// Flushes any trailing incomplete byte sequence or dangling surrogate as U+FFFD (`\uFFFD`).
    pub fn end(*_, out: []u8): usize {
        let p: usize = 0;
        if (_.last_need > 0) {
            p = sdecPushReplacement(out, p);
            _.last_need = 0;
            _.last_total = 0;
        }
        if (_.high_surrogate != 0) {
            p = sdecPushReplacement(out, p);
            _.high_surrogate = 0;
        }
        return p;
    }
}

export {
    DecoderEncoding,
    StringDecoder,
};

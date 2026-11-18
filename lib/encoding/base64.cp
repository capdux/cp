// Source: bun/src/base64/base64.cp
// lib/encoding/base64.cp
// High-performance Base64 encoder, decoder, in-place decoder & codecs in pure CP

pub const standard_alphabet: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
pub const url_safe_alphabet: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

pub fn decodeByteStandard(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0xFF;
}

pub fn decodeByteURLSafe(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return 0xFF;
}

pub fn decodeByteUnified(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return 0xFF;
}

pub struct Base64Encoder {
    alphabet: string,
    pad_char: u8?,

    pub static fn init(alphabet: string, pad_char: u8?): Base64Encoder {
        return Base64Encoder {
            alphabet: alphabet,
            pad_char: pad_char,
        };
    }

    pub fn calcSize(source_len: usize): usize {
        if (_.pad_char != null) {
            return ((source_len + 2) / 3) * 4;
        } else {
            let leftover = source_len % 3;
            return (source_len / 3) * 4 + ((leftover * 4 + 2) / 3);
        }
    }

    pub fn encode(dest: []u8, source: []const u8): usize {
        let acc: u32 = 0;
        let acc_len: u32 = 0;
        let out_idx: usize = 0;

        for (byte in source) {
            acc = (acc << 8) | (byte as u32);
            acc_len += 8;
            while (acc_len >= 6) {
                acc_len -= 6;
                let idx = ((acc >> acc_len) & 0x3F) as usize;
                dest[out_idx] = _.alphabet[idx];
                out_idx += 1;
            }
        }

        if (acc_len > 0) {
            let shift = (6 as u32) - acc_len;
            let idx = ((acc << shift) & 0x3F) as usize;
            dest[out_idx] = _.alphabet[idx];
            out_idx += 1;
        }

        if (pad := _.pad_char) {
            while (out_idx % 4 != 0) {
                dest[out_idx] = pad;
                out_idx += 1;
            }
        }

        return out_idx;
    }
}

pub struct Base64Decoder {
    is_url_safe: bool,
    pad_char: u8?,

    pub static fn init(is_url_safe: bool, pad_char: u8?): Base64Decoder {
        return Base64Decoder {
            is_url_safe: is_url_safe,
            pad_char: pad_char,
        };
    }

    pub fn calcSizeUpperBound(source_len: usize): usize! {
        let result = (source_len / 4) * 3;
        let leftover = source_len % 4;
        if (_.pad_char != null) {
            if (leftover % 4 != 0) {
                return err.InvalidPadding;
            }
        } else {
            if (leftover % 4 == 1) {
                return err.InvalidPadding;
            }
            result += (leftover * 3) / 4;
        }
        return result;
    }

    pub fn calcSizeForSlice(source: []const u8): usize! {
        let source_len = source.len;
        let result = _.calcSizeUpperBound(source_len)?;
        if (pad := _.pad_char) {
            if (source_len >= 1 && source[source_len - 1] == pad) {
                result -= 1;
            }
            if (source_len >= 2 && source[source_len - 2] == pad) {
                result -= 1;
            }
        }
        return result;
    }

    pub fn decode(dest: []u8, source: []const u8): usize! {
        let acc: u32 = 0;
        let acc_len: u32 = 0;
        let dest_idx: usize = 0;
        let src_idx: usize = 0;

        while (src_idx < source.len) {
            let c = source[src_idx];
            if (c == '=') {
                break;
            }
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                src_idx += 1;
                continue;
            }
            let val = if (_.is_url_safe) decodeByteURLSafe(c) else decodeByteStandard(c);
            if (val == 0xFF) {
                return err.InvalidCharacter;
            }
            acc = (acc << 6) | (val as u32);
            acc_len += 6;
            if (acc_len >= 8) {
                if (dest_idx >= dest.len) {
                    return err.NoSpaceLeft;
                }
                acc_len -= 8;
                dest[dest_idx] = ((acc >> acc_len) & 0xFF) as u8;
                dest_idx += 1;
            }
            src_idx += 1;
        }

        if (acc_len > 4 || (acc & (((1 as u32) << acc_len) - 1)) != 0) {
            return err.InvalidPadding;
        }

        if (pad := _.pad_char) {
            while (src_idx < source.len) {
                let c = source[src_idx];
                if (c != pad && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                    return err.InvalidCharacter;
                }
                src_idx += 1;
            }
        }

        return dest_idx;
    }
}

pub struct Base64DecoderWithIgnore {
    decoder: Base64Decoder,
    ignore_whitespace: bool,

    pub static fn init(is_url_safe: bool, pad_char: u8?, ignore_whitespace: bool): Base64DecoderWithIgnore {
        return Base64DecoderWithIgnore {
            decoder: Base64Decoder.init(is_url_safe, pad_char),
            ignore_whitespace: ignore_whitespace,
        };
    }

    pub fn decode(dest: []u8, source: []const u8): usize! {
        return _.decoder.decode(dest, source);
    }
}

pub struct Codecs {
    encoder: Base64Encoder,
    decoder: Base64Decoder,

    pub static fn init(alphabet: string, is_url_safe: bool, pad_char: u8?): Codecs {
        return Codecs {
            encoder: Base64Encoder.init(alphabet, pad_char),
            decoder: Base64Decoder.init(is_url_safe, pad_char),
        };
    }
}

pub const standard = Codecs {
    encoder: Base64Encoder.init(standard_alphabet, '='),
    decoder: Base64Decoder.init(false, '='),
};

pub const standard_no_pad = Codecs {
    encoder: Base64Encoder.init(standard_alphabet, null),
    decoder: Base64Decoder.init(false, null),
};

pub const url_safe = Codecs {
    encoder: Base64Encoder.init(url_safe_alphabet, '='),
    decoder: Base64Decoder.init(true, '='),
};

pub const url_safe_no_pad = Codecs {
    encoder: Base64Encoder.init(url_safe_alphabet, null),
    decoder: Base64Decoder.init(true, null),
};

pub class Base64 {
    pub static fn encodeLen(len: usize): usize {
        return ((len + 2) / 3) * 4;
    }

    pub static fn encodeLenURLSafe(len: usize): usize {
        let leftover = len % 3;
        return (len / 3) * 4 + ((leftover * 4 + 2) / 3);
    }

    pub static fn decodeLen(len: usize): usize {
        return (len / 4) * 3 + 4;
    }

    pub static fn exactDecodedLen(source: []const u8): usize {
        let len = source.len;
        if (len == 0) return 0;
        let pad: usize = 0;
        if (len > 0 && source[len - 1] == '=') pad += 1;
        if (len > 1 && source[len - 2] == '=') pad += 1;
        let raw = (len / 4) * 3;
        if (raw >= pad) return raw - pad;
        return 0;
    }

    pub static fn encode(dest: []u8, source: []const u8): usize {
        return standard.encoder.encode(dest, source);
    }

    pub static fn encodeURLSafe(dest: []u8, source: []const u8): usize {
        return url_safe_no_pad.encoder.encode(dest, source);
    }

    pub static fn encodeURLSafePadded(dest: []u8, source: []const u8): usize {
        return url_safe.encoder.encode(dest, source);
    }

    pub static fn encodeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const out_len = encodeLen(source.len);
        let dest = allocator.alloc(u8, out_len)?;
        const actual_len = encode(dest, source);
        return dest[0..actual_len];
    }

    pub static fn encodeURLSafeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const out_len = encodeLenURLSafe(source.len);
        let dest = allocator.alloc(u8, out_len)?;
        const actual_len = encodeURLSafe(dest, source);
        return dest[0..actual_len];
    }

    pub static fn decode(dest: []u8, source: []const u8): usize! {
        return decodeUnified(dest, source);
    }

    pub static fn decodeURLSafe(dest: []u8, source: []const u8): usize! {
        return decodeUnified(dest, source);
    }

    pub static fn decodeInPlace(buf: []u8): usize! {
        let acc: u32 = 0;
        let acc_len: u32 = 0;
        let dest_idx: usize = 0;
        let src_idx: usize = 0;

        while (src_idx < buf.len) {
            let c = buf[src_idx];
            if (c == '=') {
                break;
            }
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                src_idx += 1;
                continue;
            }
            let val = decodeByteUnified(c);
            if (val == 0xFF) {
                return err.InvalidCharacter;
            }
            acc = (acc << 6) | (val as u32);
            acc_len += 6;
            if (acc_len >= 8) {
                acc_len -= 8;
                buf[dest_idx] = ((acc >> acc_len) & 0xFF) as u8;
                dest_idx += 1;
            }
            src_idx += 1;
        }

        if (acc_len > 4 || (acc & (((1 as u32) << acc_len) - 1)) != 0) {
            return err.InvalidPadding;
        }

        while (src_idx < buf.len) {
            let c = buf[src_idx];
            if (c != '=' && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return err.InvalidCharacter;
            }
            src_idx += 1;
        }

        return dest_idx;
    }

    pub static fn decodeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const max_len = decodeLen(source.len);
        let dest = allocator.alloc(u8, max_len)?;
        const actual_len = decode(dest, source)?;
        return dest[0..actual_len];
    }

    pub static fn decodeURLSafeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        return decodeAlloc(allocator, source);
    }

    pub static fn isValid(source: []const u8): bool {
        let pad_seen = false;
        let count: usize = 0;
        for (c in source) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '=') {
                pad_seen = true;
                count += 1;
                continue;
            }
            if (pad_seen) return false;
            if (decodeByteStandard(c) == 0xFF) return false;
            count += 1;
        }
        return count % 4 == 0 || count % 4 == 2 || count % 4 == 3;
    }

    pub static fn isValidURLSafe(source: []const u8): bool {
        for (c in source) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '=') continue;
            if (decodeByteURLSafe(c) == 0xFF) return false;
        }
        return true;
    }

    static fn decodeUnified(dest: []u8, source: []const u8): usize! {
        let acc: u32 = 0;
        let acc_len: u32 = 0;
        let dest_idx: usize = 0;
        let src_idx: usize = 0;

        while (src_idx < source.len) {
            let c = source[src_idx];
            if (c == '=') {
                break;
            }
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                src_idx += 1;
                continue;
            }
            let val = decodeByteUnified(c);
            if (val == 0xFF) {
                return err.InvalidCharacter;
            }
            acc = (acc << 6) | (val as u32);
            acc_len += 6;
            if (acc_len >= 8) {
                if (dest_idx >= dest.len) {
                    return err.NoSpaceLeft;
                }
                acc_len -= 8;
                dest[dest_idx] = ((acc >> acc_len) & 0xFF) as u8;
                dest_idx += 1;
            }
            src_idx += 1;
        }

        if (acc_len > 4 || (acc & (((1 as u32) << acc_len) - 1)) != 0) {
            return err.InvalidPadding;
        }

        while (src_idx < source.len) {
            let c = source[src_idx];
            if (c != '=' && c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return err.InvalidCharacter;
            }
            src_idx += 1;
        }

        return dest_idx;
    }
}

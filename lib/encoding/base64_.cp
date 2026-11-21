// Source: Absorbed from /data/dev/bun/src/base64/base64.cp | Synced: 2026-11-19T20:26:37Z
// lib/encoding/base64_.cp
// Low-level Base64 lookup tables, byte decoders, and codec engines in pure CP

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
    pub alphabet: string,
    pub pad_char: u8?,

    pub static fn init(alphabet: string, pad_char: u8?): Base64Encoder {
        return Base64Encoder{
            .alphabet = alphabet,
            .pad_char = pad_char,
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

    pub fn encodeWithoutSizeCheck(dest: []u8, source: []const u8): usize {
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

        return out_idx;
    }

    pub fn encode(dest: []u8, source: []const u8): usize {
        let out_idx = _.encodeWithoutSizeCheck(dest, source);
        if (pad := _.pad_char) {
            while (out_idx % 4 != 0) {
                dest[out_idx] = pad;
                out_idx += 1;
            }
        }
        return out_idx;
    }

    pub fn encodeSlice(dest: []u8, source: []const u8): []const u8 {
        let written = _.encode(dest, source);
        return dest[0..written];
    }
}

pub struct Base64Decoder {
    pub is_url_safe: bool,
    pub pad_char: u8?,

    pub static fn init(is_url_safe: bool, pad_char: u8?): Base64Decoder {
        return Base64Decoder{
            .is_url_safe = is_url_safe,
            .pad_char = pad_char,
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
                if (_.pad_char == null) {
                    return err.InvalidCharacter;
                }
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
    pub decoder: Base64Decoder,
    pub ignore_whitespace: bool,

    pub static fn init(is_url_safe: bool, pad_char: u8?, ignore_whitespace: bool): Base64DecoderWithIgnore {
        return Base64DecoderWithIgnore{
            .decoder = Base64Decoder.init(is_url_safe, pad_char),
            .ignore_whitespace = ignore_whitespace,
        };
    }

    pub fn calcSizeUpperBound(source_len: usize): usize {
        let result = (source_len / 4) * 3;
        if (_.decoder.pad_char == null) {
            let leftover = source_len % 4;
            result += (leftover * 3) / 4;
        }
        return result;
    }

    pub fn decode(dest: []u8, source: []const u8): usize! {
        return _.decoder.decode(dest, source);
    }
}

pub struct Codecs {
    pub encoder: Base64Encoder,
    pub decoder: Base64Decoder,
    pub Encoder: Base64Encoder,
    pub Decoder: Base64Decoder,

    pub static fn init(alphabet: string, is_url_safe: bool, pad_char: u8?): Codecs {
        let e = Base64Encoder.init(alphabet, pad_char);
        let d = Base64Decoder.init(is_url_safe, pad_char);
        return Codecs{
            .encoder = e,
            .decoder = d,
            .Encoder = e,
            .Decoder = d,
        };
    }

    pub fn encode(dest: []u8, source: []const u8): usize {
        return _.encoder.encode(dest, source);
    }

    pub fn decode(dest: []u8, source: []const u8): usize! {
        return _.decoder.decode(dest, source);
    }
}

pub const standard = Codecs{
    .encoder = Base64Encoder.init(standard_alphabet, '='),
    .decoder = Base64Decoder.init(false, '='),
    .Encoder = Base64Encoder.init(standard_alphabet, '='),
    .Decoder = Base64Decoder.init(false, '='),
};

pub const standard_no_pad = Codecs{
    .encoder = Base64Encoder.init(standard_alphabet, null),
    .decoder = Base64Decoder.init(false, null),
    .Encoder = Base64Encoder.init(standard_alphabet, null),
    .Decoder = Base64Decoder.init(false, null),
};

pub const url_safe = Codecs{
    .encoder = Base64Encoder.init(url_safe_alphabet, '='),
    .decoder = Base64Decoder.init(true, '='),
    .Encoder = Base64Encoder.init(url_safe_alphabet, '='),
    .Decoder = Base64Decoder.init(true, '='),
};

pub const url_safe_no_pad = Codecs{
    .encoder = Base64Encoder.init(url_safe_alphabet, null),
    .decoder = Base64Decoder.init(true, null),
    .Encoder = Base64Encoder.init(url_safe_alphabet, null),
    .Decoder = Base64Decoder.init(true, null),
};

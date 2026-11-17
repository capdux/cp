// Source: Bun src/string/
// lib/encoding/hex.cp
// Zero-heap table-driven Hexadecimal Encoder & Decoder
// Absorbed from Bun src/string/ & encoding utilities

const hex_lower_alphabet: []const u8 = "0123456789abcdef";
const hex_upper_alphabet: []const u8 = "0123456789ABCDEF";

pub struct Hex {
    pub static fn encodedLen(src_len: usize): usize {
        return src_len * 2;
    }

    pub static fn decodedLen(hex_len: usize): usize {
        return hex_len / 2;
    }

    pub static fn nibbleValue(c: u8): u8 {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return 0xFF;
    }

    /// Checks whether `hex_str` has an even length and consists solely of valid hexadecimal digits.
    pub static fn isValid(hex_str: []const u8): bool {
        if ((hex_str.len & 1) != 0) {
            return false;
        }
        let i: usize = 0;
        while (i < hex_str.len) {
            if (Hex.nibbleValue(hex_str[i]) == 0xFF) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    /// Encodes `src` into lowercase hexadecimal characters inside `out` and returns the number of bytes written.
    pub static fn encodeLower(src: []const u8, out: []u8): usize {
        const needed = src.len * 2;
        if (out.len < needed) {
            return 0;
        }
        let i: usize = 0;
        while (i < src.len) {
            const b = src[i];
            out[i * 2] = hex_lower_alphabet[(b >> 4) as usize];
            out[i * 2 + 1] = hex_lower_alphabet[(b & 0x0F) as usize];
            i += 1;
        }
        return needed;
    }

    /// Encodes `src` into uppercase hexadecimal characters inside `out` and returns the number of bytes written.
    pub static fn encodeUpper(src: []const u8, out: []u8): usize {
        const needed = src.len * 2;
        if (out.len < needed) {
            return 0;
        }
        let i: usize = 0;
        while (i < src.len) {
            const b = src[i];
            out[i * 2] = hex_upper_alphabet[(b >> 4) as usize];
            out[i * 2 + 1] = hex_upper_alphabet[(b & 0x0F) as usize];
            i += 1;
        }
        return needed;
    }

    /// Decodes `hex_str` into `out` and returns the number of decoded bytes, or an error on invalid length/characters.
    pub static fn decode(hex_str: []const u8, out: []u8): usize! {
        if ((hex_str.len & 1) != 0) {
            return error.InvalidHexLength;
        }
        const out_len = hex_str.len / 2;
        if (out.len < out_len) {
            return error.BufferTooSmall;
        }
        let i: usize = 0;
        while (i < out_len) {
            const hi = Hex.nibbleValue(hex_str[i * 2]);
            const lo = Hex.nibbleValue(hex_str[i * 2 + 1]);
            if (hi == 0xFF || lo == 0xFF) {
                return error.InvalidHexCharacter;
            }
            out[i] = (hi << 4) | lo;
            i += 1;
        }
        return out_len;
    }
}

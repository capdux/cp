// lib/url/percent.cp
// RFC 3986 URI Component Percent-Encoder & Decoder
// Absorbed from Bun src/url/percent-encoding.cp

const url_hex_upper: []const u8 = "0123456789ABCDEF";

pub struct PercentEncoding {
    /// Returns `true` if `c` is an RFC 3986 unreserved character (`ALPHA / DIGIT / "-" / "." / "_" / "~"`).
    pub static fn isUnreserved(c: u8): bool {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            return true;
        }
        return c == '-' || c == '.' || c == '_' || c == '~';
    }

    pub static fn hexValue(c: u8): u8 {
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

    /// Returns `true` if any byte in `input` requires RFC 3986 component percent-encoding.
    pub static fn needsEncodeComponent(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            if (!PercentEncoding.isUnreserved(input[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Percent-encodes `input` into `out` according to RFC 3986 URI component rules and returns the bytes written.
    pub static fn encodeComponentToBuf(input: []const u8, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const c = input[i];
            if (PercentEncoding.isUnreserved(c)) {
                if (pos + 1 > out.len) {
                    break;
                }
                out[pos] = c;
                pos += 1;
            } else {
                if (pos + 3 > out.len) {
                    break;
                }
                out[pos] = '%';
                out[pos + 1] = url_hex_upper[(c >> 4) as usize];
                out[pos + 2] = url_hex_upper[(c & 0x0F) as usize];
                pos += 3;
            }
            i += 1;
        }
        return pos;
    }

    /// Decodes `%XX` sequences (and optionally `+` as space when `plus_as_space` is true) into `out`.
    pub static fn decodeToBuf(input: []const u8, plus_as_space: bool, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len && pos < out.len) {
            const c = input[i];
            if (c == '%' && i + 2 < input.len) {
                const hi = PercentEncoding.hexValue(input[i + 1]);
                const lo = PercentEncoding.hexValue(input[i + 2]);
                if (hi != 0xFF && lo != 0xFF) {
                    out[pos] = (hi << 4) | lo;
                    pos += 1;
                    i += 3;
                    continue;
                }
            }
            if (plus_as_space && c == '+') {
                out[pos] = ' ';
                pos += 1;
                i += 1;
                continue;
            }
            out[pos] = c;
            pos += 1;
            i += 1;
        }
        return pos;
    }
}

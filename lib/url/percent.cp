// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T11:13:51Z
// ============================================================================
// lib/url/percent.cp — RFC 3986 URI Component Percent-Encoder & Decoder
// ============================================================================
const std = @import("std");

const url_hex_upper: []const u8 = "0123456789ABCDEF";

pub struct PercentEncoding {
    /// Returns `true` if `c` is an RFC 3986 unreserved character (`ALPHA / DIGIT / "-" / "." / "_" / "~"`).
    pub static fn isUnreserved(c: u8): bool {
        if ((c >= (65 as u8) and c <= (90 as u8)) or (c >= (97 as u8) and c <= (122 as u8)) or (c >= (48 as u8) and c <= (57 as u8))) {
            return true;
        }
        return c == (45 as u8) or c == (46 as u8) or c == (95 as u8) or c == (126 as u8);
    }

    pub static fn hexValue(c: u8): u8 {
        if (c >= (48 as u8) and c <= (57 as u8)) {
            return c - (48 as u8);
        }
        if (c >= (97 as u8) and c <= (102 as u8)) {
            return c - (97 as u8) + 10;
        }
        if (c >= (65 as u8) and c <= (70 as u8)) {
            return c - (65 as u8) + 10;
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
            let c = input[i];
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
                out[pos] = 37 as u8; // '%'
                out[pos + 1] = url_hex_upper[((c >> 4) & 0x0F) as usize];
                out[pos + 2] = url_hex_upper[(c & 0x0F) as usize];
                pos += 3;
            }
            i += 1;
        }
        return pos;
    }

    /// Convenience alias for `encodeComponentToBuf`.
    pub static fn encode(input: []const u8, out: []u8): usize {
        return PercentEncoding.encodeComponentToBuf(input, out);
    }

    /// Decodes `%XX` sequences (and optionally `+` as space when `plus_as_space` is true) into `out`.
    pub static fn decodeToBuf(input: []const u8, plus_as_space: bool, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len and pos < out.len) {
            let c = input[i];
            if (c == (37 as u8) and i + 2 < input.len) {
                let hi = PercentEncoding.hexValue(input[i + 1]);
                let lo = PercentEncoding.hexValue(input[i + 2]);
                if (hi != 0xFF and lo != 0xFF) {
                    out[pos] = (hi << 4) | lo;
                    pos += 1;
                    i += 3;
                    continue;
                }
            }
            if (plus_as_space and c == (43 as u8)) {
                out[pos] = 32 as u8; // ' '
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

    /// Decodes `%XX` sequences into `out` without converting `+` to space.
    pub static fn decode(input: []const u8, out: []u8): usize {
        return PercentEncoding.decodeToBuf(input, false, out);
    }

    /// Decodes `%XX` sequences and `+` as space (application/x-www-form-urlencoded) into `out`.
    pub static fn decodeQuery(input: []const u8, out: []u8): usize {
        return PercentEncoding.decodeToBuf(input, true, out);
    }

    /// Percent-encodes `input` into allocated string memory.
    pub static fn encodeComponent(input: []const u8, allocator: std.mem.Allocator): !string {
        let max_len = input.len * 3;
        let buf = allocator.alloc(u8, max_len)?;
        errdefer allocator.free(buf);
        let len = PercentEncoding.encodeComponentToBuf(input, buf);
        let res = allocator.realloc(buf, len)?;
        return res;
    }

    /// Decodes percent-encoded string into allocated memory.
    pub static fn decodeAlloc(input: []const u8, plus_as_space: bool, allocator: std.mem.Allocator): !string {
        let buf = allocator.alloc(u8, input.len)?;
        errdefer allocator.free(buf);
        let len = PercentEncoding.decodeToBuf(input, plus_as_space, buf);
        let res = allocator.realloc(buf, len)?;
        return res;
    }
}

pub const Percent = PercentEncoding;

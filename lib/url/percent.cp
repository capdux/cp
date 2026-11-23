// Reference-hardened against servo/rust-url (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/url/percent.cp — RFC 3986 / WHATWG Percent-Encoder & Strict Decoder
// ============================================================================
import { Simd } from "cap:simd";
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

    /// 16/32-lane SIMD fast scan for the next `'%'` byte via `cap:simd`.
    pub static fn findNextPercent(input: []const u8): usize? {
        return Simd.indexOfByte(input, 37 as u8);
    }

    /// Scalar reference scan for the next `'%'` byte (used for SIMD differential verification).
    pub static fn findNextPercentScalar(input: []const u8): usize? {
        let i: usize = 0;
        while (i < input.len) {
            if (input[i] == (37 as u8)) return i;
            i += 1;
        }
        return null;
    }

    /// Returns `true` if any byte in `input` requires RFC 3986 component percent-encoding.
    /// Uses `Simd.isAscii` fast-path to immediately detect non-ASCII bytes.
    pub static fn needsEncodeComponent(input: []const u8): bool {
        if (input.len >= 16 and !Simd.isAscii(input)) {
            return true;
        }
        let i: usize = 0;
        while (i < input.len) {
            if (!PercentEncoding.isUnreserved(input[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Zero-allocation strict validator: every `'%'` in `input` must be followed by two valid ASCII hex digits.
    pub static fn isValidEncoding(input: []const u8): bool {
        let i: usize = 0;
        while (i < input.len) {
            const rel = Simd.indexOfByte(input[i..], 37 as u8);
            if (off := rel) {
                const pct = i + off;
                if (pct + 2 >= input.len) return false;
                const hi = PercentEncoding.hexValue(input[pct + 1]);
                const lo = PercentEncoding.hexValue(input[pct + 2]);
                if (hi == 0xFF or lo == 0xFF) return false;
                i = pct + 3;
            } else {
                return true;
            }
        }
        return true;
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
    /// Uses `Simd.indexOfByte('%')` bulk-copy fast-path when `plus_as_space` is false.
    pub static fn decodeToBuf(input: []const u8, plus_as_space: bool, out: []u8): usize {
        let i: usize = 0;
        let pos: usize = 0;
        if (!plus_as_space) {
            while (i < input.len and pos < out.len) {
                const rel = Simd.indexOfByte(input[i..], 37 as u8);
                if (off := rel) {
                    if (off > 0) {
                        const avail = out.len - pos;
                        const copy_len = if (off <= avail) off else avail;
                        @memcpy(out[pos .. pos + copy_len], input[i .. i + copy_len]);
                        pos += copy_len;
                        i += copy_len;
                        if (copy_len < off or pos >= out.len) break;
                    }
                    if (i + 2 < input.len) {
                        const hi = PercentEncoding.hexValue(input[i + 1]);
                        const lo = PercentEncoding.hexValue(input[i + 2]);
                        if (hi != 0xFF and lo != 0xFF) {
                            out[pos] = (hi << 4) | lo;
                            pos += 1;
                            i += 3;
                            continue;
                        }
                    }
                    out[pos] = input[i];
                    pos += 1;
                    i += 1;
                } else {
                    const rem = input.len - i;
                    const avail = out.len - pos;
                    const copy_len = if (rem <= avail) rem else avail;
                    @memcpy(out[pos .. pos + copy_len], input[i .. i + copy_len]);
                    pos += copy_len;
                    break;
                }
            }
            return pos;
        }
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
            if (c == (43 as u8)) {
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

    /// Strict zero-allocation `%XX` decoder: rejects truncated `'%'` (e.g. `"%2"`, `"%"`),
    /// invalid hex sequences (e.g. `"%ZZ"`, `"%2G"`), and insufficient output buffer capacity.
    /// Uses `Simd.indexOfByte('%')` for fast bulk copy between escape sequences.
    pub static fn decodeStrict(input: []const u8, out: []u8): ([]const u8)? {
        let i: usize = 0;
        let pos: usize = 0;
        while (i < input.len) {
            const rel = Simd.indexOfByte(input[i..], 37 as u8);
            if (off := rel) {
                if (off > 0) {
                    if (pos + off > out.len) return null;
                    @memcpy(out[pos .. pos + off], input[i .. i + off]);
                    pos += off;
                    i += off;
                }
                if (i + 2 >= input.len) return null;
                const hi = PercentEncoding.hexValue(input[i + 1]);
                const lo = PercentEncoding.hexValue(input[i + 2]);
                if (hi == 0xFF or lo == 0xFF) return null;
                if (pos >= out.len) return null;
                out[pos] = (hi << 4) | lo;
                pos += 1;
                i += 3;
            } else {
                const rem = input.len - i;
                if (pos + rem > out.len) return null;
                @memcpy(out[pos .. pos + rem], input[i .. i + rem]);
                pos += rem;
                break;
            }
        }
        return out[0..pos];
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

/// Top-level strict percent-decoder: returns `null` on truncated `'%'`, invalid `%XX`, or small `out`.
pub fn percentDecodeStrict(input: []const u8, out: []u8): ([]const u8)? {
    return PercentEncoding.decodeStrict(input, out);
}

/// Top-level zero-allocation validator for percent-encoded sequences.
pub fn isValidPercentEncoding(input: []const u8): bool {
    return PercentEncoding.isValidEncoding(input);
}

pub const Percent = PercentEncoding;

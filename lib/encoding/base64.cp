// Source: Absorbed from /data/dev/bun/src/base64/base64.cp | Synced: 2026-11-22T12:49:17Z
// lib/encoding/base64.cp
// High-performance Base64 encoder, decoder, in-place decoder & codecs in pure CP

const std = @import("std");

export {
    standard_alphabet,
    url_safe_alphabet,
    decodeByteStandard,
    decodeByteURLSafe,
    decodeByteUnified,
    Base64Encoder,
    Base64Decoder,
    Base64DecoderWithIgnore,
    Codecs,
    standard,
    standard_no_pad,
    url_safe,
    url_safe_no_pad,
} from "./base64_.cp";

import {
    decodeByteStandard,
    decodeByteURLSafe,
    decodeByteUnified,
    standard,
    standard_no_pad,
    url_safe,
    url_safe_no_pad,
} from "./base64_.cp";

pub class Base64 {
    /// Calculates the padded Base64 encoded length (`((len + 2) / 3) * 4`).
    pub static fn encodeLen(len: usize): usize {
        return ((len + 2) / 3) * 4;
    }

    /// Calculates the unpadded URL-Safe Base64 encoded length (RFC 4648 Section 5).
    pub static fn encodeLenURLSafe(len: usize): usize {
        let leftover = len % 3;
        return (len / 3) * 4 + ((leftover * 4 + 2) / 3);
    }

    /// Calculates a safe upper-bound buffer size for decoding `len` Base64 bytes.
    pub static fn decodeLen(len: usize): usize {
        return (len / 4) * 3 + 4;
    }

    /// Calculates the upper-bound decoded byte length without extra margin.
    pub static fn decodeLenUpperBound(len: usize): usize {
        let leftover = len % 4;
        return (len / 4) * 3 + ((leftover * 3) / 4);
    }

    /// Computes the exact decoded byte length of a Base64 slice, accounting for `=` padding or unpadded remainder.
    pub static fn exactDecodedLen(source: []const u8): usize {
        let len = source.len;
        if (len == 0) return 0;
        let pad: usize = 0;
        if (len > 0 && source[len - 1] == '=') pad += 1;
        if (len > 1 && source[len - 2] == '=') pad += 1;
        if (pad > 0) {
            let raw = (len / 4) * 3;
            if (raw >= pad) return raw - pad;
            return 0;
        }
        let leftover = len % 4;
        return (len / 4) * 3 + ((leftover * 3) / 4);
    }

    /// Encodes `source` into `dest` using standard Base64 with `=` padding (RFC 4648 Section 4).
    pub static fn encode(dest: []u8, source: []const u8): usize {
        return standard.encoder.encode(dest, source);
    }

    /// Encodes `source` into `dest` using unpadded Base64.
    pub static fn encodeNoPad(dest: []u8, source: []const u8): usize {
        return standard_no_pad.encoder.encode(dest, source);
    }

    /// Encodes `source` into `dest` using URL-Safe Base64 without padding (`-` and `_`, no `=`).
    pub static fn encodeURLSafe(dest: []u8, source: []const u8): usize {
        return url_safe_no_pad.encoder.encode(dest, source);
    }

    /// Encodes `source` into `dest` using URL-Safe Base64 with `=` padding.
    pub static fn encodeURLSafePadded(dest: []u8, source: []const u8): usize {
        return url_safe.encoder.encode(dest, source);
    }

    /// Dynamically allocates and encodes `source` using standard padded Base64.
    pub static fn encodeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const out_len = encodeLen(source.len);
        let dest = allocator.alloc(u8, out_len)?;
        const actual_len = encode(dest, source);
        return dest[0..actual_len];
    }

    /// Dynamically allocates and encodes `source` using URL-Safe unpadded Base64.
    pub static fn encodeURLSafeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const out_len = encodeLenURLSafe(source.len);
        let dest = allocator.alloc(u8, out_len)?;
        const actual_len = encodeURLSafe(dest, source);
        return dest[0..actual_len];
    }

    /// Dynamically allocates and encodes `source` using URL-Safe padded Base64.
    pub static fn encodeURLSafePaddedAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const out_len = encodeLen(source.len);
        let dest = allocator.alloc(u8, out_len)?;
        const actual_len = encodeURLSafePadded(dest, source);
        return dest[0..actual_len];
    }

    /// Decodes standard or URL-Safe Base64 (forgiving whitespace & optional padding) into `dest`.
    pub static fn decode(dest: []u8, source: []const u8): usize! {
        return decodeUnified(dest, source);
    }

    /// Decodes URL-Safe Base64 (with or without padding) into `dest`.
    pub static fn decodeURLSafe(dest: []u8, source: []const u8): usize! {
        return decodeUnified(dest, source);
    }

    /// Decodes Base64 in-place within `buf`, overwriting it with decoded bytes and returning the decoded length.
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

        let pad_count: usize = 0;
        while (src_idx < buf.len) {
            let c = buf[src_idx];
            if (c == '=') {
                pad_count += 1;
                if (pad_count > 2) {
                    return err.InvalidPadding;
                }
            } else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return err.InvalidCharacter;
            }
            src_idx += 1;
        }

        return dest_idx;
    }

    /// Dynamically allocates and decodes standard or URL-Safe Base64.
    pub static fn decodeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        const max_len = decodeLen(source.len);
        let dest = allocator.alloc(u8, max_len)?;
        errdefer allocator.free(dest);
        const actual_len = decode(dest, source)?;
        return dest[0..actual_len];
    }

    /// Dynamically allocates and decodes URL-Safe Base64.
    pub static fn decodeURLSafeAlloc(allocator: std.mem.Allocator, source: []const u8): []u8! {
        return decodeAlloc(allocator, source);
    }

    /// Validates whether `source` is valid standard Base64.
    pub static fn isValid(source: []const u8): bool {
        let pad_seen = false;
        let pad_count: usize = 0;
        let count: usize = 0;
        for (c in source) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '=') {
                pad_seen = true;
                pad_count += 1;
                if (pad_count > 2) return false;
                count += 1;
                continue;
            }
            if (pad_seen) return false;
            if (decodeByteStandard(c) == 0xFF) return false;
            count += 1;
        }
        return count % 4 != 1;
    }

    /// Validates whether `source` is valid URL-Safe Base64.
    pub static fn isValidURLSafe(source: []const u8): bool {
        let pad_seen = false;
        let pad_count: usize = 0;
        let count: usize = 0;
        for (c in source) {
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') continue;
            if (c == '=') {
                pad_seen = true;
                pad_count += 1;
                if (pad_count > 2) return false;
                count += 1;
                continue;
            }
            if (pad_seen) return false;
            if (decodeByteURLSafe(c) == 0xFF) return false;
            count += 1;
        }
        return count % 4 != 1;
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

        let pad_count: usize = 0;
        while (src_idx < source.len) {
            let c = source[src_idx];
            if (c == '=') {
                pad_count += 1;
                if (pad_count > 2) {
                    return err.InvalidPadding;
                }
            } else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return err.InvalidCharacter;
            }
            src_idx += 1;
        }

        return dest_idx;
    }
}

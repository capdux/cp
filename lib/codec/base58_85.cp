// Source: cp/lib/codec/base58_85.cp (Pure CP Base58, Base58Check, Adobe ASCII85 & RFC 1924 Base85 Codec)
// Zero-C-dependency industrial multi-radix & checksum codec suite.

const std = @import("std");
import { sha256Raw } from "cap:crypto";

/// Unified error set returned by all `cap:codec` encoders and decoders.
pub err CodecError {
    InvalidCharacter,
    InvalidLength,
    InvalidChecksum,
    InvalidPrefix,
    MixedCase,
    Overflow,
    OutOfMemory,
}

const BASE58_ALPHABET: []const u8 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
const RFC1924_ALPHABET: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";

const POW85_4: u64 = 52200625; // 85^4
const POW85_3: u64 = 614125;   // 85^3
const POW85_2: u64 = 7225;     // 85^2
const POW85_1: u64 = 85;       // 85^1
const U32_MAX_U64: u64 = 0xFFFFFFFF;

fn base58Val(c: u8): u8 {
    if (c >= '1' && c <= '9') return c - '1';
    if (c >= 'A' && c <= 'H') return (c - 'A') + 9;
    if (c >= 'J' && c <= 'N') return (c - 'J') + 17;
    if (c >= 'P' && c <= 'Z') return (c - 'P') + 22;
    if (c >= 'a' && c <= 'k') return (c - 'a') + 33;
    if (c >= 'm' && c <= 'z') return (c - 'm') + 44;
    return 0xFF;
}

fn rfc1924Val(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return (c - 'A') + 10;
    if (c >= 'a' && c <= 'z') return (c - 'a') + 36;
    return switch (c) {
        '!' => 62,
        '#' => 63,
        '$' => 64,
        '%' => 65,
        '&' => 66,
        '(' => 67,
        ')' => 68,
        '*' => 69,
        '+' => 70,
        '-' => 71,
        ';' => 72,
        '<' => 73,
        '=' => 74,
        '>' => 75,
        '?' => 76,
        '@' => 77,
        '^' => 78,
        '_' => 79,
        '`' => 80,
        '{' => 81,
        '|' => 82,
        '}' => 83,
        '~' => 84,
        else => 0xFF,
    };
}

fn isAsciiWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C;
}

fn doubleSha256(data: []const u8): [32]u8 {
    let first: [32]u8 = [0; 32];
    let second: [32]u8 = [0; 32];
    sha256Raw(&first, data);
    sha256Raw(&second, first[0..32]);
    return second;
}

/// Pure CP Base58 and Base58Check (Bitcoin/IPFS/Solana alphabet + Double-SHA256) codec.
pub struct Base58 {
    /// Encodes `bytes` into Base58 using `std.heap.page_allocator`, preserving leading `0x00` bytes as `'1'`.
    static fn encode(bytes: []const u8): ([]u8) ! CodecError {
        return Base58.encodeAlloc(std.heap.page_allocator, bytes);
    }

    /// Encodes `bytes` into Base58 using the provided `alloc`.
    static fn encodeAlloc(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! CodecError {
        if (bytes.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }

        let zeros: usize = 0;
        while (zeros < bytes.len && bytes[zeros] == 0) {
            zeros += 1;
        }

        const rem_len = bytes.len - zeros;
        const cap = (rem_len * 138) / 100 + 2;
        let digits = alloc.alloc(u8, cap) catch return CodecError.OutOfMemory;
        defer alloc.free(digits);
        @memset(digits, 0);

        let size: usize = 0;
        let idx = zeros;
        while (idx < bytes.len) : (idx += 1) {
            let carry: u32 = bytes[idx] as u32;
            let j: usize = 0;
            while (j < size || carry != 0) : (j += 1) {
                if (j < size) {
                    carry += (digits[j] as u32) * 256;
                } else {
                    size += 1;
                }
                digits[j] = (carry % 58) as u8;
                carry /= 58;
            }
        }

        const out_len = zeros + size;
        let out = alloc.alloc(u8, out_len) catch return CodecError.OutOfMemory;
        let k: usize = 0;
        while (k < zeros) : (k += 1) {
            out[k] = '1';
        }
        let d_idx = size;
        while (d_idx > 0) {
            d_idx -= 1;
            out[k] = BASE58_ALPHABET[digits[d_idx] as usize];
            k += 1;
        }
        return out;
    }

    /// Decodes Base58 `encoded` text into raw bytes using `std.heap.page_allocator`.
    static fn decode(encoded: []const u8): ([]u8) ! CodecError {
        return Base58.decodeAlloc(std.heap.page_allocator, encoded);
    }

    /// Decodes Base58 `encoded` text into raw bytes using the provided `alloc`.
    static fn decodeAlloc(alloc: std.mem.Allocator, encoded: []const u8): ([]u8) ! CodecError {
        if (encoded.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }

        let zeros: usize = 0;
        while (zeros < encoded.len && encoded[zeros] == '1') {
            zeros += 1;
        }

        const rem_len = encoded.len - zeros;
        const cap = (rem_len * 733) / 1000 + 2;
        let bytes_rev = alloc.alloc(u8, cap) catch return CodecError.OutOfMemory;
        defer alloc.free(bytes_rev);
        @memset(bytes_rev, 0);

        let size: usize = 0;
        let idx = zeros;
        while (idx < encoded.len) : (idx += 1) {
            const val = base58Val(encoded[idx]);
            if (val == 0xFF) {
                return CodecError.InvalidCharacter;
            }
            let carry: u32 = val as u32;
            let j: usize = 0;
            while (j < size || carry != 0) : (j += 1) {
                if (j < size) {
                    carry += (bytes_rev[j] as u32) * 58;
                } else {
                    size += 1;
                }
                bytes_rev[j] = (carry & 0xFF) as u8;
                carry >>= 8;
            }
        }

        const out_len = zeros + size;
        let out = alloc.alloc(u8, out_len) catch return CodecError.OutOfMemory;
        let k: usize = 0;
        while (k < zeros) : (k += 1) {
            out[k] = 0x00;
        }
        let b_idx = size;
        while (b_idx > 0) {
            b_idx -= 1;
            out[k] = bytes_rev[b_idx];
            k += 1;
        }
        return out;
    }

    /// Encodes `payload` with a 4-byte Double-SHA256 checksum suffix (Base58Check).
    static fn encodeCheck(payload: []const u8): ([]u8) ! CodecError {
        return Base58.encodeCheckAlloc(std.heap.page_allocator, payload);
    }

    /// Encodes `payload` with a 4-byte Double-SHA256 checksum suffix using `alloc`.
    static fn encodeCheckAlloc(alloc: std.mem.Allocator, payload: []const u8): ([]u8) ! CodecError {
        const digest = doubleSha256(payload);
        let buf = alloc.alloc(u8, payload.len + 4) catch return CodecError.OutOfMemory;
        defer alloc.free(buf);
        if (payload.len > 0) {
            @memcpy(buf[0..payload.len], payload);
        }
        @memcpy(buf[payload.len .. payload.len + 4], digest[0..4]);
        return Base58.encodeAlloc(alloc, buf);
    }

    /// Decodes Base58Check `encoded` text, verifying the 4-byte Double-SHA256 checksum.
    static fn decodeCheck(encoded: []const u8): ([]u8) ! CodecError {
        return Base58.decodeCheckAlloc(std.heap.page_allocator, encoded);
    }

    /// Decodes Base58Check `encoded` text using `alloc`, verifying the 4-byte Double-SHA256 checksum.
    static fn decodeCheckAlloc(alloc: std.mem.Allocator, encoded: []const u8): ([]u8) ! CodecError {
        const raw = Base58.decodeAlloc(alloc, encoded)?;
        defer alloc.free(raw);

        if (raw.len < 4) {
            return CodecError.InvalidLength;
        }

        const payload_len = raw.len - 4;
        const payload = raw[0..payload_len];
        const chk = raw[payload_len..raw.len];
        const expected = doubleSha256(payload);

        let diff: u8 = 0;
        for (i of 0..4) {
            diff |= chk[i] ^ expected[i];
        }
        if (diff != 0) {
            return CodecError.InvalidChecksum;
        }

        let out = alloc.alloc(u8, payload_len) catch return CodecError.OutOfMemory;
        if (payload_len > 0) {
            @memcpy(out, payload);
        }
        return out;
    }
}

/// Pure CP Adobe ASCII85 (`<~...~>`, `'z'` zero compression) and RFC 1924 Base85 codec.
pub struct Base85 {
    /// Encodes `bytes` into Adobe ASCII85 wrapped with `<~...~>` delimiters.
    static fn encodeAscii85(bytes: []const u8): ([]u8) ! CodecError {
        return Base85.encodeAscii85Alloc(std.heap.page_allocator, bytes);
    }

    /// Encodes `bytes` into Adobe ASCII85 wrapped with `<~...~>` using `alloc`.
    static fn encodeAscii85Alloc(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! CodecError {
        return Base85.encodeAscii85Internal(alloc, bytes, true);
    }

    /// Encodes `bytes` into raw Adobe ASCII85 without `<~...~>` delimiters.
    static fn encodeAscii85Raw(bytes: []const u8): ([]u8) ! CodecError {
        return Base85.encodeAscii85Internal(std.heap.page_allocator, bytes, false);
    }

    static fn encodeAscii85Internal(
        alloc: std.mem.Allocator,
        bytes: []const u8,
        wrap_delimiters: bool,
    ): ([]u8) ! CodecError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (wrap_delimiters) {
            out.appendSlice(alloc, "<~") catch return CodecError.OutOfMemory;
        }

        let pos: usize = 0;
        while (pos + 4 <= bytes.len) : (pos += 4) {
            const val: u32 = ((bytes[pos] as u32) << 24) |
                ((bytes[pos + 1] as u32) << 16) |
                ((bytes[pos + 2] as u32) << 8) |
                (bytes[pos + 3] as u32);
            if (val == 0) {
                out.append(alloc, 'z') catch return CodecError.OutOfMemory;
            } else {
                let v = val;
                let digits: [5]u8 = [0; 5];
                let d: usize = 5;
                while (d > 0) {
                    d -= 1;
                    digits[d] = ('!' as u8) + ((v % 85) as u8);
                    v /= 85;
                }
                out.appendSlice(alloc, digits[0..5]) catch return CodecError.OutOfMemory;
            }
        }

        const rem = bytes.len - pos;
        if (rem > 0) {
            let padded: [4]u8 = [0; 4];
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                padded[i] = bytes[pos + i];
            }
            let v: u32 = ((padded[0] as u32) << 24) |
                ((padded[1] as u32) << 16) |
                ((padded[2] as u32) << 8) |
                (padded[3] as u32);
            let digits: [5]u8 = [0; 5];
            let d: usize = 5;
            while (d > 0) {
                d -= 1;
                digits[d] = ('!' as u8) + ((v % 85) as u8);
                v /= 85;
            }
            out.appendSlice(alloc, digits[0 .. rem + 1]) catch return CodecError.OutOfMemory;
        }

        if (wrap_delimiters) {
            out.appendSlice(alloc, "~>") catch return CodecError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return CodecError.OutOfMemory;
    }

    /// Decodes Adobe ASCII85 text (with or without `<~...~>`, ignoring whitespace, supporting `'z'`).
    static fn decodeAscii85(encoded: []const u8): ([]u8) ! CodecError {
        return Base85.decodeAscii85Alloc(std.heap.page_allocator, encoded);
    }

    /// Decodes Adobe ASCII85 text using `alloc`.
    static fn decodeAscii85Alloc(alloc: std.mem.Allocator, encoded: []const u8): ([]u8) ! CodecError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let pos: usize = 0;
        while (pos < encoded.len && isAsciiWhitespace(encoded[pos])) {
            pos += 1;
        }

        let has_prefix = false;
        if (pos + 2 <= encoded.len && encoded[pos] == '<' && encoded[pos + 1] == '~') {
            has_prefix = true;
            pos += 2;
        }

        let found_suffix = false;
        let group: [5]u8 = [0; 5];
        let count: usize = 0;

        while (pos < encoded.len) {
            const c = encoded[pos];
            if (isAsciiWhitespace(c)) {
                pos += 1;
                continue;
            }
            if (c == '~') {
                if (pos + 1 < encoded.len && encoded[pos + 1] == '>') {
                    if (!has_prefix) {
                        return CodecError.InvalidPrefix;
                    }
                    found_suffix = true;
                    pos += 2;
                    break;
                }
                if (has_prefix) {
                    return CodecError.InvalidPrefix;
                }
                return CodecError.InvalidCharacter;
            }
            if (c == 'z') {
                if (count != 0) {
                    return CodecError.InvalidCharacter;
                }
                const zeros: [4]u8 = [0, 0, 0, 0];
                out.appendSlice(alloc, zeros[0..4]) catch return CodecError.OutOfMemory;
                pos += 1;
                continue;
            }
            if (c < '!' || c > 'u') {
                return CodecError.InvalidCharacter;
            }
            group[count] = c - ('!' as u8);
            count += 1;
            pos += 1;

            if (count == 5) {
                const val: u64 = (group[0] as u64) * POW85_4 +
                    (group[1] as u64) * POW85_3 +
                    (group[2] as u64) * POW85_2 +
                    (group[3] as u64) * POW85_1 +
                    (group[4] as u64);
                if (val > U32_MAX_U64) {
                    return CodecError.Overflow;
                }
                const u = val as u32;
                const chunk: [4]u8 = [
                    ((u >> 24) & 0xFF) as u8,
                    ((u >> 16) & 0xFF) as u8,
                    ((u >> 8) & 0xFF) as u8,
                    (u & 0xFF) as u8,
                ];
                out.appendSlice(alloc, chunk[0..4]) catch return CodecError.OutOfMemory;
                count = 0;
            }
        }

        if (has_prefix) {
            if (!found_suffix) {
                return CodecError.InvalidPrefix;
            }
            while (pos < encoded.len) {
                if (!isAsciiWhitespace(encoded[pos])) {
                    return CodecError.InvalidCharacter;
                }
                pos += 1;
            }
        }

        if (count == 1) {
            return CodecError.InvalidLength;
        }
        if (count > 1) {
            let p = count;
            while (p < 5) : (p += 1) {
                group[p] = 84;
            }
            const val: u64 = (group[0] as u64) * POW85_4 +
                (group[1] as u64) * POW85_3 +
                (group[2] as u64) * POW85_2 +
                (group[3] as u64) * POW85_1 +
                (group[4] as u64);
            if (val > U32_MAX_U64) {
                return CodecError.Overflow;
            }
            const u = val as u32;
            const emit_bytes = count - 1;
            const chunk: [4]u8 = [
                ((u >> 24) & 0xFF) as u8,
                ((u >> 16) & 0xFF) as u8,
                ((u >> 8) & 0xFF) as u8,
                (u & 0xFF) as u8,
            ];
            out.appendSlice(alloc, chunk[0..emit_bytes]) catch return CodecError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return CodecError.OutOfMemory;
    }

    /// Encodes `bytes` using the RFC 1924 Base85 alphabet (4-byte chunking, arbitrary length).
    static fn encodeRfc1924(bytes: []const u8): ([]u8) ! CodecError {
        return Base85.encodeRfc1924Alloc(std.heap.page_allocator, bytes);
    }

    /// Encodes `bytes` using the RFC 1924 Base85 alphabet and `alloc`.
    static fn encodeRfc1924Alloc(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! CodecError {
        if (bytes.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }
        const full_chunks = bytes.len / 4;
        const rem = bytes.len % 4;
        const out_len = full_chunks * 5 + (if (rem > 0) (rem + 1) else 0);
        let out = alloc.alloc(u8, out_len) catch return CodecError.OutOfMemory;

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos + 4 <= bytes.len) : (in_pos += 4) {
            let v: u32 = ((bytes[in_pos] as u32) << 24) |
                ((bytes[in_pos + 1] as u32) << 16) |
                ((bytes[in_pos + 2] as u32) << 8) |
                (bytes[in_pos + 3] as u32);
            let d: usize = 5;
            while (d > 0) {
                d -= 1;
                out[out_pos + d] = RFC1924_ALPHABET[(v % 85) as usize];
                v /= 85;
            }
            out_pos += 5;
        }

        if (rem > 0) {
            let padded: [4]u8 = [0; 4];
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                padded[i] = bytes[in_pos + i];
            }
            let v: u32 = ((padded[0] as u32) << 24) |
                ((padded[1] as u32) << 16) |
                ((padded[2] as u32) << 8) |
                (padded[3] as u32);
            let digits: [5]u8 = [0; 5];
            let d: usize = 5;
            while (d > 0) {
                d -= 1;
                digits[d] = RFC1924_ALPHABET[(v % 85) as usize];
                v /= 85;
            }
            let j: usize = 0;
            while (j < rem + 1) : (j += 1) {
                out[out_pos + j] = digits[j];
            }
        }

        return out;
    }

    /// Decodes RFC 1924 Base85 text into raw bytes.
    static fn decodeRfc1924(encoded: []const u8): ([]u8) ! CodecError {
        return Base85.decodeRfc1924Alloc(std.heap.page_allocator, encoded);
    }

    /// Decodes RFC 1924 Base85 text into raw bytes using `alloc`.
    static fn decodeRfc1924Alloc(alloc: std.mem.Allocator, encoded: []const u8): ([]u8) ! CodecError {
        if (encoded.len == 0) {
            return alloc.alloc(u8, 0) catch return CodecError.OutOfMemory;
        }
        const rem = encoded.len % 5;
        if (rem == 1) {
            return CodecError.InvalidLength;
        }
        const full_groups = encoded.len / 5;
        const out_len = full_groups * 4 + (if (rem > 0) (rem - 1) else 0);
        let out = alloc.alloc(u8, out_len) catch return CodecError.OutOfMemory;
        errdefer alloc.free(out);

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos + 5 <= encoded.len) : (in_pos += 5) {
            let acc: u64 = 0;
            let i: usize = 0;
            while (i < 5) : (i += 1) {
                const d = rfc1924Val(encoded[in_pos + i]);
                if (d == 0xFF) {
                    return CodecError.InvalidCharacter;
                }
                acc = acc * 85 + (d as u64);
            }
            if (acc > U32_MAX_U64) {
                return CodecError.Overflow;
            }
            const u = acc as u32;
            out[out_pos] = ((u >> 24) & 0xFF) as u8;
            out[out_pos + 1] = ((u >> 16) & 0xFF) as u8;
            out[out_pos + 2] = ((u >> 8) & 0xFF) as u8;
            out[out_pos + 3] = (u & 0xFF) as u8;
            out_pos += 4;
        }

        if (rem > 0) {
            let group: [5]u8 = [84; 5];
            let i: usize = 0;
            while (i < rem) : (i += 1) {
                const d = rfc1924Val(encoded[in_pos + i]);
                if (d == 0xFF) {
                    return CodecError.InvalidCharacter;
                }
                group[i] = d;
            }
            const val: u64 = (group[0] as u64) * POW85_4 +
                (group[1] as u64) * POW85_3 +
                (group[2] as u64) * POW85_2 +
                (group[3] as u64) * POW85_1 +
                (group[4] as u64);
            if (val > U32_MAX_U64) {
                return CodecError.Overflow;
            }
            const u = val as u32;
            const emit_bytes = rem - 1;
            const chunk: [4]u8 = [
                ((u >> 24) & 0xFF) as u8,
                ((u >> 16) & 0xFF) as u8,
                ((u >> 8) & 0xFF) as u8,
                (u & 0xFF) as u8,
            ];
            let j: usize = 0;
            while (j < emit_bytes) : (j += 1) {
                out[out_pos + j] = chunk[j];
            }
        }

        return out;
    }
}

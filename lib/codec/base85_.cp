// Reference Only: Functional overlap with lib/codec/base58_85.cp | Kept for comparison/analysis
// Source: cp/lib/base85/base85.cp (Pure CP Z85, Adobe Ascii85 & RFC 1924 Base85 Encoding Engine)
// Zero-C-dependency industrial Base85 codec suite.

const std = @import("std");

/// Unified error set returned by all `cap:base85` encoders and decoders.
pub err Base85Error {
    InvalidLength,
    InvalidCharacter,
    ValueOverflow,
    MalformedFraming,
    OutOfMemory,
}

const Z85_ALPHABET: []const u8 = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#";
const RFC1924_ALPHABET: []const u8 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";

const POW85_4: u64 = 52200625; // 85^4
const POW85_3: u64 = 614125;   // 85^3
const POW85_2: u64 = 7225;     // 85^2
const POW85_1: u64 = 85;       // 85^1
const U32_MAX_U64: u64 = 0xFFFFFFFF;

fn z85Val(c: u8): u8 {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'z') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'Z') return (c - 'A') + 36;
    return switch (c) {
        '.' => 62,
        '-' => 63,
        ':' => 64,
        '+' => 65,
        '=' => 66,
        '^' => 67,
        '!' => 68,
        '/' => 69,
        '*' => 70,
        '?' => 71,
        '&' => 72,
        '<' => 73,
        '>' => 74,
        '(' => 75,
        ')' => 76,
        '[' => 77,
        ']' => 78,
        '{' => 79,
        '}' => 80,
        '@' => 81,
        '%' => 82,
        '$' => 83,
        '#' => 84,
        else => 0xFF,
    };
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

/// ZeroMQ Spec 32 Z85 codec (strict 4-byte / 5-char alignment).
pub struct Z85 {
    /// Encodes 4-byte aligned `bytes` into Z85 string using `alloc`.
    static fn encode(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Base85Error {
        if (bytes.len % 4 != 0) {
            return Base85Error.InvalidLength;
        }
        if (bytes.len == 0) {
            return alloc.alloc(u8, 0) catch return Base85Error.OutOfMemory;
        }

        const out_len = (bytes.len / 4) * 5;
        let out = alloc.alloc(u8, out_len) catch return Base85Error.OutOfMemory;

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < bytes.len) : (in_pos += 4) {
            let v: u32 = ((bytes[in_pos] as u32) << 24) |
                ((bytes[in_pos + 1] as u32) << 16) |
                ((bytes[in_pos + 2] as u32) << 8) |
                (bytes[in_pos + 3] as u32);
            let d: usize = 5;
            while (d > 0) {
                d -= 1;
                out[out_pos + d] = Z85_ALPHABET[(v % 85) as usize];
                v /= 85;
            }
            out_pos += 5;
        }

        return out;
    }

    /// Decodes 5-char aligned Z85 `str` into raw bytes using `alloc`.
    static fn decode(alloc: std.mem.Allocator, str: []const u8): ([]u8) ! Base85Error {
        if (str.len % 5 != 0) {
            return Base85Error.InvalidLength;
        }
        if (str.len == 0) {
            return alloc.alloc(u8, 0) catch return Base85Error.OutOfMemory;
        }

        const out_len = (str.len / 5) * 4;
        let out = alloc.alloc(u8, out_len) catch return Base85Error.OutOfMemory;
        errdefer alloc.free(out);

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < str.len) : (in_pos += 5) {
            let acc: u64 = 0;
            let i: usize = 0;
            while (i < 5) : (i += 1) {
                const d = z85Val(str[in_pos + i]);
                if (d == 0xFF) {
                    return Base85Error.InvalidCharacter;
                }
                acc = acc * 85 + (d as u64);
            }
            if (acc > U32_MAX_U64) {
                return Base85Error.ValueOverflow;
            }
            const u = acc as u32;
            out[out_pos] = ((u >> 24) & 0xFF) as u8;
            out[out_pos + 1] = ((u >> 16) & 0xFF) as u8;
            out[out_pos + 2] = ((u >> 8) & 0xFF) as u8;
            out[out_pos + 3] = (u & 0xFF) as u8;
            out_pos += 4;
        }

        return out;
    }
}

/// Adobe Ascii85 codec (arbitrary length, `'z'` all-zero compression, optional `<~...~>` framing).
pub struct Ascii85 {
    /// Encodes `bytes` into raw Adobe Ascii85 without `<~...~>` delimiters.
    static fn encode(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Base85Error {
        return Ascii85.encodeInternal(alloc, bytes, false);
    }

    /// Encodes `bytes` into Adobe Ascii85 wrapped with `<~...~>` delimiters.
    static fn encodeFramed(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Base85Error {
        return Ascii85.encodeInternal(alloc, bytes, true);
    }

    static fn encodeInternal(
        alloc: std.mem.Allocator,
        bytes: []const u8,
        wrap_delimiters: bool,
    ): ([]u8) ! Base85Error {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (wrap_delimiters) {
            out.appendSlice(alloc, "<~") catch return Base85Error.OutOfMemory;
        }

        let pos: usize = 0;
        while (pos + 4 <= bytes.len) : (pos += 4) {
            const val: u32 = ((bytes[pos] as u32) << 24) |
                ((bytes[pos + 1] as u32) << 16) |
                ((bytes[pos + 2] as u32) << 8) |
                (bytes[pos + 3] as u32);
            if (val == 0) {
                out.append(alloc, 'z') catch return Base85Error.OutOfMemory;
            } else {
                let v = val;
                let digits: [5]u8 = undefined;
                let d: usize = 5;
                while (d > 0) {
                    d -= 1;
                    digits[d] = ('!' as u8) + ((v % 85) as u8);
                    v /= 85;
                }
                out.appendSlice(alloc, digits[0..5]) catch return Base85Error.OutOfMemory;
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
            let digits: [5]u8 = undefined;
            let d: usize = 5;
            while (d > 0) {
                d -= 1;
                digits[d] = ('!' as u8) + ((v % 85) as u8);
                v /= 85;
            }
            out.appendSlice(alloc, digits[0 .. rem + 1]) catch return Base85Error.OutOfMemory;
        }

        if (wrap_delimiters) {
            out.appendSlice(alloc, "~>") catch return Base85Error.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return Base85Error.OutOfMemory;
    }

    /// Decodes raw Adobe Ascii85 `str` (ignoring ASCII whitespace, supporting `'z'`).
    static fn decode(alloc: std.mem.Allocator, str: []const u8): ([]u8) ! Base85Error {
        return Ascii85.decodeInternal(alloc, str, false);
    }

    /// Decodes `<~...~>` framed Adobe Ascii85 `str` (ignoring ASCII whitespace, supporting `'z'`).
    static fn decodeFramed(alloc: std.mem.Allocator, str: []const u8): ([]u8) ! Base85Error {
        return Ascii85.decodeInternal(alloc, str, true);
    }

    static fn decodeInternal(
        alloc: std.mem.Allocator,
        str: []const u8,
        require_framing: bool,
    ): ([]u8) ! Base85Error {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let pos: usize = 0;
        while (pos < str.len && isAsciiWhitespace(str[pos])) {
            pos += 1;
        }

        if (require_framing) {
            if (pos + 2 > str.len || str[pos] != '<' || str[pos + 1] != '~') {
                return Base85Error.MalformedFraming;
            }
            pos += 2;
        }

        let found_suffix = false;
        let group: [5]u8 = undefined;
        let count: usize = 0;

        while (pos < str.len) {
            const c = str[pos];
            if (isAsciiWhitespace(c)) {
                pos += 1;
                continue;
            }
            if (c == '~') {
                if (require_framing && pos + 1 < str.len && str[pos + 1] == '>') {
                    found_suffix = true;
                    pos += 2;
                    break;
                }
                if (require_framing) {
                    return Base85Error.MalformedFraming;
                }
                return Base85Error.InvalidCharacter;
            }
            if (c == 'z') {
                if (count != 0) {
                    return Base85Error.MalformedFraming;
                }
                const zeros: [4]u8 = [0, 0, 0, 0];
                out.appendSlice(alloc, zeros[0..4]) catch return Base85Error.OutOfMemory;
                pos += 1;
                continue;
            }
            if (c < '!' || c > 'u') {
                return Base85Error.InvalidCharacter;
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
                    return Base85Error.ValueOverflow;
                }
                const u = val as u32;
                const chunk: [4]u8 = [
                    ((u >> 24) & 0xFF) as u8,
                    ((u >> 16) & 0xFF) as u8,
                    ((u >> 8) & 0xFF) as u8,
                    (u & 0xFF) as u8,
                ];
                out.appendSlice(alloc, chunk[0..4]) catch return Base85Error.OutOfMemory;
                count = 0;
            }
        }

        if (require_framing) {
            if (!found_suffix) {
                return Base85Error.MalformedFraming;
            }
            while (pos < str.len) {
                if (!isAsciiWhitespace(str[pos])) {
                    return Base85Error.MalformedFraming;
                }
                pos += 1;
            }
        }

        if (count == 1) {
            return Base85Error.InvalidLength;
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
                return Base85Error.ValueOverflow;
            }
            const u = val as u32;
            const emit_bytes = count - 1;
            const chunk: [4]u8 = [
                ((u >> 24) & 0xFF) as u8,
                ((u >> 16) & 0xFF) as u8,
                ((u >> 8) & 0xFF) as u8,
                (u & 0xFF) as u8,
            ];
            out.appendSlice(alloc, chunk[0..emit_bytes]) catch return Base85Error.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return Base85Error.OutOfMemory;
    }
}

/// RFC 1924 IPv6 compact Base85 codec (arbitrary byte length).
pub struct Rfc1924 {
    /// Encodes `bytes` using the RFC 1924 Base85 alphabet and `alloc`.
    static fn encode(alloc: std.mem.Allocator, bytes: []const u8): ([]u8) ! Base85Error {
        if (bytes.len == 0) {
            return alloc.alloc(u8, 0) catch return Base85Error.OutOfMemory;
        }
        const full_chunks = bytes.len / 4;
        const rem = bytes.len % 4;
        const out_len = full_chunks * 5 + (if (rem > 0) (rem + 1) else 0);
        let out = alloc.alloc(u8, out_len) catch return Base85Error.OutOfMemory;

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
            let digits: [5]u8 = undefined;
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

    /// Decodes RFC 1924 Base85 `str` into raw bytes using `alloc`.
    static fn decode(alloc: std.mem.Allocator, str: []const u8): ([]u8) ! Base85Error {
        if (str.len == 0) {
            return alloc.alloc(u8, 0) catch return Base85Error.OutOfMemory;
        }
        const rem = str.len % 5;
        if (rem == 1) {
            return Base85Error.InvalidLength;
        }
        const full_groups = str.len / 5;
        const out_len = full_groups * 4 + (if (rem > 0) (rem - 1) else 0);
        let out = alloc.alloc(u8, out_len) catch return Base85Error.OutOfMemory;
        errdefer alloc.free(out);

        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos + 5 <= str.len) : (in_pos += 5) {
            let acc: u64 = 0;
            let i: usize = 0;
            while (i < 5) : (i += 1) {
                const d = rfc1924Val(str[in_pos + i]);
                if (d == 0xFF) {
                    return Base85Error.InvalidCharacter;
                }
                acc = acc * 85 + (d as u64);
            }
            if (acc > U32_MAX_U64) {
                return Base85Error.ValueOverflow;
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
                const d = rfc1924Val(str[in_pos + i]);
                if (d == 0xFF) {
                    return Base85Error.InvalidCharacter;
                }
                group[i] = d;
            }
            const val: u64 = (group[0] as u64) * POW85_4 +
                (group[1] as u64) * POW85_3 +
                (group[2] as u64) * POW85_2 +
                (group[3] as u64) * POW85_1 +
                (group[4] as u64);
            if (val > U32_MAX_U64) {
                return Base85Error.ValueOverflow;
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

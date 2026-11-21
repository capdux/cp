// Source: Absorbed from /data/dev/bun/src/string/immutable/exact_size_matcher.cp
// Constant-time fixed-width (1, 2, 4, 8, 12, 16 byte) integer-packed string matcher for O(1) switch dispatch.

/// Returns a matcher type that packs strings up to `max_bytes` (1, 2, 4, 8, 12, or 16)
/// into a single little-endian unsigned integer for branchless O(1) `switch` matching.
pub fn ExactSizeMatcher(comptime max_bytes: usize): type {
    switch (max_bytes) {
        1, 2, 4, 8, 12, 16 => {},
        * => {
            @compileError("max_bytes must be 1, 2, 4, 8, 12, or 16.");
        },
    }

    const T = std.meta.Int(.unsigned, max_bytes * 8);

    return struct {
        static fn match(str: string): T {
            if (str.len == 0) {
                return 0;
            }
            if (str.len < max_bytes) {
                let tmp: [max_bytes]u8 = undefined;
                @memcpy(tmp[0..str.len], str);
                @memset(tmp[str.len..], 0);
                return std.mem.readInt(T, &tmp, .little);
            }
            if (str.len == max_bytes) {
                return std.mem.readInt(T, str[0..max_bytes], .little);
            }
            return std.math.maxInt(T);
        }

        static fn matchLower(str: string): T {
            if (str.len == 0) {
                return 0;
            }
            if (str.len <= max_bytes) {
                let tmp: [max_bytes]u8 = undefined;
                let i: usize = 0;
                while (i < str.len) : (i += 1) {
                    tmp[i] = std.ascii.toLower(str[i]);
                }
                if (str.len < max_bytes) {
                    @memset(tmp[str.len..], 0);
                }
                return std.mem.readInt(T, &tmp, .little);
            }
            return std.math.maxInt(T);
        }

        static fn case(comptime str: string): T {
            if (str.len < max_bytes) {
                let bytes = std.mem.zeroes([max_bytes]u8);
                bytes[0..str.len].* = str[0..str.len].*;
                return std.mem.readInt(T, &bytes, .little);
            }
            if (str.len == max_bytes) {
                return std.mem.readInt(T, str[0..max_bytes], .little);
            }
            @compileError("str too long for ExactSizeMatcher");
        }
    };
}

/// Convenience 4-byte matcher (e.g., short HTTP methods, file extensions, protocol tags).
pub type Matcher4 = ExactSizeMatcher(4);
/// Convenience 8-byte matcher (e.g., HTTP methods, JS keywords, header tokens).
pub type Matcher8 = ExactSizeMatcher(8);
/// Convenience 16-byte matcher (e.g., content-types, MIME tokens, short commands).
pub type Matcher16 = ExactSizeMatcher(16);

pub fn packU32(str: string): u32 {
    return Matcher4.match(str);
}

pub fn packU32Lower(str: string): u32 {
    return Matcher4.matchLower(str);
}

pub fn packU64(str: string): u64 {
    return Matcher8.match(str);
}

pub fn packU64Lower(str: string): u64 {
    return Matcher8.matchLower(str);
}

pub fn packU128(str: string): u128 {
    return Matcher16.match(str);
}

pub fn packU128Lower(str: string): u128 {
    return Matcher16.matchLower(str);
}

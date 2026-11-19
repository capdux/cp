// Source: /data/dev/bun/src/highway/
// ============================================================================
// lib/simd/index.cp — Highway-grade SIMD Vector Acceleration & Byte Scanners (cap:simd)
// Absorbed from /data/dev/bun/src/highway/highway.cp & Zig std.simd
// ============================================================================

const std = @import("std");

type SimdVec16u8 = @Vector(16, u8);
type SimdVec32u8 = @Vector(32, u8);

pub struct Simd {
    /// Suggest the optimal vector length for a given type T on the target CPU.
    pub static fn suggestVectorLength(T: type): usize {
        zig {
            return std.simd.suggestVectorLength(T) orelse 4;
        }
    }

    /// Create an iota vector of length N: [0, 1, ..., N-1]
    pub static fn iotaI32(comptime N: usize): [N]i32 {
        zig {
            return std.simd.iota(i32, N);
        }
    }

    /// SIMD-accelerated reduction sum for i32 slice.
    pub static fn reduceAddI32(slice: []const i32): i32 {
        if (slice.len == 0) return 0;
        let sum: i32 = 0;
        zig {
            const len = slice.len;
            const vec_len = 8;
            var i: usize = 0;
            var v_sum: @Vector(vec_len, i32) = @splat(0);

            while (i + vec_len <= len) {
                var buf: [vec_len]i32 align(32) = undefined;
                @memcpy(&buf, slice[i .. i + vec_len]);
                const chunk: @Vector(vec_len, i32) = buf;
                v_sum += chunk;
                i += vec_len;
            }
            sum = @reduce(.Add, v_sum);
            while (i < len) {
                sum += slice[i];
                i += 1;
            }
        }
        return sum;
    }

    /// SIMD-accelerated reduction min for i32 slice.
    pub static fn reduceMinI32(slice: []const i32): i32 {
        if (slice.len == 0) return 0;
        let min_val: i32 = slice[0];
        zig {
            const len = slice.len;
            const vec_len = 8;
            var i: usize = 0;
            var v_min: @Vector(vec_len, i32) = @splat(slice[0]);

            while (i + vec_len <= len) {
                var buf: [vec_len]i32 align(32) = undefined;
                @memcpy(&buf, slice[i .. i + vec_len]);
                const chunk: @Vector(vec_len, i32) = buf;
                v_min = @min(v_min, chunk);
                i += vec_len;
            }
            min_val = @reduce(.Min, v_min);
            while (i < len) {
                if (slice[i] < min_val) min_val = slice[i];
                i += 1;
            }
        }
        return min_val;
    }

    /// SIMD-accelerated reduction max for i32 slice.
    pub static fn reduceMaxI32(slice: []const i32): i32 {
        if (slice.len == 0) return 0;
        let max_val: i32 = slice[0];
        zig {
            const len = slice.len;
            const vec_len = 8;
            var i: usize = 0;
            var v_max: @Vector(vec_len, i32) = @splat(slice[0]);

            while (i + vec_len <= len) {
                var buf: [vec_len]i32 align(32) = undefined;
                @memcpy(&buf, slice[i .. i + vec_len]);
                const chunk: @Vector(vec_len, i32) = buf;
                v_max = @max(v_max, chunk);
                i += vec_len;
            }
            max_val = @reduce(.Max, v_max);
            while (i < len) {
                if (slice[i] > max_val) max_val = slice[i];
                i += 1;
            }
        }
        return max_val;
    }

    /// SIMD-accelerated dot product for f32 slices.
    pub static fn dotProductF32(a: []const f32, b: []const f32): f32 {
        if (a.len != b.len || a.len == 0) return 0.0;
        let dot: f32 = 0.0;
        zig {
            const len = a.len;
            const vec_len = 8;
            var i: usize = 0;
            var v_dot: @Vector(vec_len, f32) = @splat(0.0);

            while (i + vec_len <= len) {
                var bufA: [vec_len]f32 align(32) = undefined;
                var bufB: [vec_len]f32 align(32) = undefined;
                @memcpy(&bufA, a[i .. i + vec_len]);
                @memcpy(&bufB, b[i .. i + vec_len]);
                const va: @Vector(vec_len, f32) = bufA;
                const vb: @Vector(vec_len, f32) = bufB;
                v_dot += va * vb;
                i += vec_len;
            }
            dot = @reduce(.Add, v_dot);
            while (i < len) {
                dot += a[i] * b[i];
                i += 1;
            }
        }
        return dot;
    }

    /// SIMD-accelerated prefix sum for i32 slice.
    pub static fn prefixSumI32(slice: []i32) {
        if (slice.len == 0) return;
        zig {
            var sum: i32 = 0;
            for (slice) |*v| {
                sum += v.*;
                v.* = sum;
            }
        }
    }

    // ========================================================================
    // Highway-grade 32-byte & 16-byte SIMD Byte Scanners & ASCII Transforms
    // ========================================================================

    /// 32-byte / 16-byte SIMD vector search for single byte `needle`.
    pub static fn indexOfByte(haystack: []const u8, needle: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const target32: SimdVec32u8 = @splat(needle);
        const target16: SimdVec16u8 = @splat(needle);

        while (i + 32 <= n) {
            let chunk32: SimdVec32u8 = @splat(0);
            zig {
                chunk32 = haystack[i..][0..32].*;
            }
            const mask32: u32 = @bitCast(chunk32 == target32);
            if (mask32 != 0) {
                return i + (@ctz(mask32) as usize);
            }
            i += 32;
        }

        while (i + 16 <= n) {
            let chunk16: SimdVec16u8 = @splat(0);
            zig {
                chunk16 = haystack[i..][0..16].*;
            }
            const mask16: u16 = @bitCast(chunk16 == target16);
            if (mask16 != 0) {
                return i + (@ctz(mask16) as usize);
            }
            i += 16;
        }

        while (i < n) {
            if (haystack[i] == needle) return i;
            i += 1;
        }
        return null;
    }

    /// Alias matching Bun `highway.indexOfChar`.
    pub static fn indexOfChar(haystack: []const u8, needle: u8): usize? {
        return Simd.indexOfByte(haystack, needle);
    }

    /// 16-byte SIMD search for either of two target bytes `b1` or `b2`.
    pub static fn indexOfAny2(haystack: []const u8, b1: u8, b2: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const t1: SimdVec16u8 = @splat(b1);
        const t2: SimdVec16u8 = @splat(b2);

        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = haystack[i..][0..16].*;
            }
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const mask: u16 = m1 | m2;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = haystack[i];
            if (c == b1 || c == b2) return i;
            i += 1;
        }
        return null;
    }

    /// 16-byte SIMD search for any of four target bytes `b1`, `b2`, `b3`, or `b4`.
    pub static fn indexOfAny4(haystack: []const u8, b1: u8, b2: u8, b3: u8, b4: u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const t1: SimdVec16u8 = @splat(b1);
        const t2: SimdVec16u8 = @splat(b2);
        const t3: SimdVec16u8 = @splat(b3);
        const t4: SimdVec16u8 = @splat(b4);

        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = haystack[i..][0..16].*;
            }
            const m1: u16 = @bitCast(chunk == t1);
            const m2: u16 = @bitCast(chunk == t2);
            const m3: u16 = @bitCast(chunk == t3);
            const m4: u16 = @bitCast(chunk == t4);
            const mask: u16 = m1 | m2 | m3 | m4;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = haystack[i];
            if (c == b1 || c == b2 || c == b3 || c == b4) return i;
            i += 1;
        }
        return null;
    }

    /// Multi-character SIMD search matching Bun `highway.indexOfAnyChar`.
    pub static fn indexOfAnyChar(haystack: []const u8, chars: []const u8): usize? {
        if (chars.len == 0 || haystack.len == 0) return null;
        if (chars.len == 1) return Simd.indexOfByte(haystack, chars[0]);
        if (chars.len == 2) return Simd.indexOfAny2(haystack, chars[0], chars[1]);
        if (chars.len == 3) return Simd.indexOfAny4(haystack, chars[0], chars[1], chars[2], chars[2]);
        if (chars.len == 4) return Simd.indexOfAny4(haystack, chars[0], chars[1], chars[2], chars[3]);

        let i: usize = 0;
        const n = haystack.len;
        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = haystack[i..][0..16].*;
            }
            let combined_mask: u16 = 0;
            let c_idx: usize = 0;
            while (c_idx < chars.len) {
                const target: SimdVec16u8 = @splat(chars[c_idx]);
                const m: u16 = @bitCast(chunk == target);
                combined_mask = combined_mask | m;
                c_idx += 1;
            }
            if (combined_mask != 0) {
                return i + (@ctz(combined_mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const b = haystack[i];
            let k: usize = 0;
            while (k < chars.len) {
                if (b == chars[k]) return i;
                k += 1;
            }
            i += 1;
        }
        return null;
    }

    /// 16-byte SIMD fast whitespace skipper (`' '`, `'\t'`, `'\r'`, `'\n'`).
    /// Returns the index of the first non-whitespace byte, or `input.len` if all whitespace.
    pub static fn skipWhitespace(input: []const u8): usize {
        let i: usize = 0;
        const n = input.len;
        const v_space: SimdVec16u8 = @splat(' ');
        const v_tab: SimdVec16u8 = @splat('\t');
        const v_cr: SimdVec16u8 = @splat('\r');
        const v_lf: SimdVec16u8 = @splat('\n');

        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = input[i..][0..16].*;
            }
            const m_sp: u16 = @bitCast(chunk == v_space);
            const m_tb: u16 = @bitCast(chunk == v_tab);
            const m_cr: u16 = @bitCast(chunk == v_cr);
            const m_lf: u16 = @bitCast(chunk == v_lf);
            const ws_mask: u16 = m_sp | m_tb | m_cr | m_lf;
            const non_ws_mask: u16 = ~ws_mask;
            if (non_ws_mask != 0) {
                return i + (@ctz(non_ws_mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = input[i];
            if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
                return i;
            }
            i += 1;
        }
        return n;
    }

    /// 32-byte / 16-byte SIMD scanner returning the first non-ASCII index (`>= 0x80`), or `null` if pure ASCII.
    pub static fn firstNonAscii(input: []const u8): usize? {
        let i: usize = 0;
        const n = input.len;
        const high32: SimdVec32u8 = @splat(0x80);
        const zero32: SimdVec32u8 = @splat(0);
        const high16: SimdVec16u8 = @splat(0x80);
        const zero16: SimdVec16u8 = @splat(0);

        while (i + 32 <= n) {
            let chunk32: SimdVec32u8 = @splat(0);
            zig {
                chunk32 = input[i..][0..32].*;
            }
            const mask32: u32 = @bitCast((chunk32 & high32) != zero32);
            if (mask32 != 0) {
                return i + (@ctz(mask32) as usize);
            }
            i += 32;
        }

        while (i + 16 <= n) {
            let chunk16: SimdVec16u8 = @splat(0);
            zig {
                chunk16 = input[i..][0..16].*;
            }
            const mask16: u16 = @bitCast((chunk16 & high16) != zero16);
            if (mask16 != 0) {
                return i + (@ctz(mask16) as usize);
            }
            i += 16;
        }

        while (i < n) {
            if (input[i] >= 0x80) return i;
            i += 1;
        }
        return null;
    }

    /// Returns true if all bytes in `input` are 7-bit ASCII (`< 0x80`).
    pub static fn isAscii(input: []const u8): bool {
        return Simd.firstNonAscii(input) == null;
    }

    /// 16-byte SIMD search for newline (`\n`, `\r`) or non-ASCII (`>= 0x80`), matching Bun `highway_index_of_newline_or_non_ascii`.
    pub static fn indexOfNewlineOrNonAscii(haystack: []const u8): usize? {
        let i: usize = 0;
        const n = haystack.len;
        const v_lf: SimdVec16u8 = @splat('\n');
        const v_cr: SimdVec16u8 = @splat('\r');
        const v_high: SimdVec16u8 = @splat(0x80);
        const v_zero: SimdVec16u8 = @splat(0);

        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = haystack[i..][0..16].*;
            }
            const m_lf: u16 = @bitCast(chunk == v_lf);
            const m_cr: u16 = @bitCast(chunk == v_cr);
            const m_hi: u16 = @bitCast((chunk & v_high) != v_zero);
            const mask: u16 = m_lf | m_cr | m_hi;
            if (mask != 0) {
                return i + (@ctz(mask) as usize);
            }
            i += 16;
        }

        while (i < n) {
            const c = haystack[i];
            if (c == '\n' || c == '\r' || c >= 0x80) return i;
            i += 1;
        }
        return null;
    }

    /// 16-byte SIMD byte frequency count using `@bitCast` + `@popCount`.
    pub static fn countByte(input: []const u8, needle: u8): usize {
        let i: usize = 0;
        let total: usize = 0;
        const n = input.len;
        const target: SimdVec16u8 = @splat(needle);

        while (i + 16 <= n) {
            let chunk: SimdVec16u8 = @splat(0);
            zig {
                chunk = input[i..][0..16].*;
            }
            const mask: u16 = @bitCast(chunk == target);
            total += (@popCount(mask) as usize);
            i += 16;
        }

        while (i < n) {
            if (input[i] == needle) total += 1;
            i += 1;
        }
        return total;
    }

    /// 16-byte SIMD batch ASCII lowercase conversion into `out`. Returns number of bytes written.
    pub static fn toLowerAscii(input: []const u8, out: []u8): usize {
        const limit = if (input.len <= out.len) input.len else out.len;
        let i: usize = 0;
        const v_A: SimdVec16u8 = @splat('A');
        const v_Z: SimdVec16u8 = @splat('Z');
        const v_diff: SimdVec16u8 = @splat(32);

        while (i + 16 <= limit) {
            zig {
                const chunk: SimdVec16u8 = input[i..][0..16].*;
                const is_upper = (chunk >= v_A) & (chunk <= v_Z);
                const lowered: SimdVec16u8 = @select(u8, is_upper, chunk + v_diff, chunk);
                out[i..][0..16].* = lowered;
            }
            i += 16;
        }

        while (i < limit) {
            const c = input[i];
            out[i] = if (c >= 'A' && c <= 'Z') (c + 32) else c;
            i += 1;
        }
        return limit;
    }

    /// 16-byte SIMD batch ASCII uppercase conversion into `out`. Returns number of bytes written.
    pub static fn toUpperAscii(input: []const u8, out: []u8): usize {
        const limit = if (input.len <= out.len) input.len else out.len;
        let i: usize = 0;
        const v_a: SimdVec16u8 = @splat('a');
        const v_z: SimdVec16u8 = @splat('z');
        const v_diff: SimdVec16u8 = @splat(32);

        while (i + 16 <= limit) {
            zig {
                const chunk: SimdVec16u8 = input[i..][0..16].*;
                const is_lower = (chunk >= v_a) & (chunk <= v_z);
                const uppered: SimdVec16u8 = @select(u8, is_lower, chunk - v_diff, chunk);
                out[i..][0..16].* = uppered;
            }
            i += 16;
        }

        while (i < limit) {
            const c = input[i];
            out[i] = if (c >= 'a' && c <= 'z') (c - 32) else c;
            i += 1;
        }
        return limit;
    }
}

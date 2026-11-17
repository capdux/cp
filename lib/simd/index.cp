// Source: Custom (agy-generated export index)
// ============================================================================
// lib/simd/index.cp — SIMD Acceleration & Vector Processing Primitives (cap:simd)
// ============================================================================

const std = @import("std");

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
            const vec_len = 8; // Defaulting to 8 for now, can be improved.
            var i: usize = 0;
            var v_sum: @Vector(vec_len, i32) = @splat(0);
            
            while (i + vec_len <= len) {
                var buf: [vec_len]i32 align(32) = undefined;
                @memcpy(&buf, slice[i..i+vec_len]);
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
                @memcpy(&buf, slice[i..i+vec_len]);
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
                @memcpy(&buf, slice[i..i+vec_len]);
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
                @memcpy(&bufA, a[i..i+vec_len]);
                @memcpy(&bufB, b[i..i+vec_len]);
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
            // Simple prefix sum implementation for demonstration.
            // A more optimized one would use tree-based parallel prefix scan.
            var sum: i32 = 0;
            for (slice) |*v| {
                sum += v.*;
                v.* = sum;
            }
        }
    }
}

// Source: cp/lib/math/embedding.cp (AI Embedding Similarity, Softmax & Int8 Quantization)
// High-performance SIMD vector similarity, softmax, top-k retrieval, and symmetric Int8 quantization.

const std = @import("std");

/// Symmetric Int8 quantized vector representation (`x_approx[i] = data[i] * scale`).
pub struct QuantizedVecI8 {
    scale: f32 = 0.0;
    data: []i8 = [];

    /// Releases the underlying quantized buffer using `alloc`.
    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.data.len > 0) {
            alloc.free(_.data);
            _.data = [];
        }
    }
}

/// SIMD-accelerated dot product of two f32 slices. Returns 0.0 on length mismatch or empty input.
pub fn dotF32(a: []const f32, b: []const f32): f32 {
    if (a.len != b.len || a.len == 0) return 0.0;
    let dot: f32 = 0.0;
    zig {
        const len = a.len;
        const vec_len = 8;
        var i: usize = 0;
        var v_sum: @Vector(vec_len, f32) = @splat(0.0);

        while (i + vec_len <= len) {
            var buf_a: [vec_len]f32 align(32) = undefined;
            var buf_b: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf_a, a[i .. i + vec_len]);
            @memcpy(&buf_b, b[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf_a;
            const vb: @Vector(vec_len, f32) = buf_b;
            v_sum += va * vb;
            i += vec_len;
        }
        dot = @reduce(.Add, v_sum);
        while (i < len) {
            dot += a[i] * b[i];
            i += 1;
        }
    }
    return dot;
}

/// SIMD-accelerated L2 (Euclidean) norm of an f32 vector.
pub fn l2NormF32(v: []const f32): f32 {
    if (v.len == 0) return 0.0;
    let sum_sq: f32 = 0.0;
    zig {
        const len = v.len;
        const vec_len = 8;
        var i: usize = 0;
        var v_sum: @Vector(vec_len, f32) = @splat(0.0);

        while (i + vec_len <= len) {
            var buf: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf, v[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf;
            v_sum += va * va;
            i += vec_len;
        }
        sum_sq = @reduce(.Add, v_sum);
        while (i < len) {
            sum_sq += v[i] * v[i];
            i += 1;
        }
    }
    return @sqrt(sum_sq);
}

/// Normalizes an f32 vector in-place to unit L2 norm (no-op if norm is zero).
pub fn normalizeL2(v: []f32) {
    const norm = l2NormF32(v);
    if (norm <= 0.0) return;
    const inv = 1.0 / norm;
    zig {
        const len = v.len;
        const vec_len = 8;
        var i: usize = 0;
        const v_inv: @Vector(vec_len, f32) = @splat(inv);

        while (i + vec_len <= len) {
            var buf: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf, v[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf;
            buf = va * v_inv;
            @memcpy(v[i .. i + vec_len], &buf);
            i += vec_len;
        }
        while (i < len) {
            v[i] *= inv;
            i += 1;
        }
    }
}

/// Single-pass SIMD cosine similarity between `a` and `b` in `[-1, 1]`.
pub fn cosineSimilarityF32(a: []const f32, b: []const f32): f32 {
    if (a.len != b.len || a.len == 0) return 0.0;
    let dot: f32 = 0.0;
    let norm_a_sq: f32 = 0.0;
    let norm_b_sq: f32 = 0.0;
    zig {
        const len = a.len;
        const vec_len = 8;
        var i: usize = 0;
        var v_dot: @Vector(vec_len, f32) = @splat(0.0);
        var v_na: @Vector(vec_len, f32) = @splat(0.0);
        var v_nb: @Vector(vec_len, f32) = @splat(0.0);

        while (i + vec_len <= len) {
            var buf_a: [vec_len]f32 align(32) = undefined;
            var buf_b: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf_a, a[i .. i + vec_len]);
            @memcpy(&buf_b, b[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf_a;
            const vb: @Vector(vec_len, f32) = buf_b;
            v_dot += va * vb;
            v_na += va * va;
            v_nb += vb * vb;
            i += vec_len;
        }
        dot = @reduce(.Add, v_dot);
        norm_a_sq = @reduce(.Add, v_na);
        norm_b_sq = @reduce(.Add, v_nb);
        while (i < len) {
            dot += a[i] * b[i];
            norm_a_sq += a[i] * a[i];
            norm_b_sq += b[i] * b[i];
            i += 1;
        }
    }
    const denom = @sqrt(norm_a_sq * norm_b_sq);
    if (denom <= 0.0) return 0.0;
    let sim = dot / denom;
    if (sim > 1.0) sim = 1.0;
    if (sim < -1.0) sim = -1.0;
    return sim;
}

/// SIMD-accelerated Euclidean (L2) distance between `a` and `b`.
pub fn euclideanDistanceF32(a: []const f32, b: []const f32): f32 {
    if (a.len != b.len || a.len == 0) return 0.0;
    let sum_sq: f32 = 0.0;
    zig {
        const len = a.len;
        const vec_len = 8;
        var i: usize = 0;
        var v_sum: @Vector(vec_len, f32) = @splat(0.0);

        while (i + vec_len <= len) {
            var buf_a: [vec_len]f32 align(32) = undefined;
            var buf_b: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf_a, a[i .. i + vec_len]);
            @memcpy(&buf_b, b[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf_a;
            const vb: @Vector(vec_len, f32) = buf_b;
            const diff = va - vb;
            v_sum += diff * diff;
            i += vec_len;
        }
        sum_sq = @reduce(.Add, v_sum);
        while (i < len) {
            const d = a[i] - b[i];
            sum_sq += d * d;
            i += 1;
        }
    }
    return @sqrt(sum_sq);
}

/// SIMD-accelerated Manhattan (L1) distance between `a` and `b`.
pub fn manhattanDistanceF32(a: []const f32, b: []const f32): f32 {
    if (a.len != b.len || a.len == 0) return 0.0;
    let sum_abs: f32 = 0.0;
    zig {
        const len = a.len;
        const vec_len = 8;
        var i: usize = 0;
        var v_sum: @Vector(vec_len, f32) = @splat(0.0);

        while (i + vec_len <= len) {
            var buf_a: [vec_len]f32 align(32) = undefined;
            var buf_b: [vec_len]f32 align(32) = undefined;
            @memcpy(&buf_a, a[i .. i + vec_len]);
            @memcpy(&buf_b, b[i .. i + vec_len]);
            const va: @Vector(vec_len, f32) = buf_a;
            const vb: @Vector(vec_len, f32) = buf_b;
            v_sum += @abs(va - vb);
            i += vec_len;
        }
        sum_abs = @reduce(.Add, v_sum);
        while (i < len) {
            sum_abs += @abs(a[i] - b[i]);
            i += 1;
        }
    }
    return sum_abs;
}

/// Numerically stable in-place softmax (`exp(x - max) / sum(exp(x - max))`).
pub fn softmaxInPlace(logits: []f32) {
    if (logits.len == 0) return;

    let max_val: f32 = logits[0];
    for (i of 1..logits.len) {
        if (logits[i] > max_val) max_val = logits[i];
    }

    let sum_exp: f32 = 0.0;
    for (i of 0..logits.len) {
        const e = @exp(logits[i] - max_val);
        logits[i] = e;
        sum_exp += e;
    }

    if (sum_exp <= 0.0) return;
    const inv = 1.0 / sum_exp;
    for (i of 0..logits.len) {
        logits[i] *= inv;
    }
}

/// Returns the index of the maximum element in `v`, or `null` if `v` is empty.
pub fn argMaxF32(v: []const f32): usize? {
    if (v.len == 0) return null;
    let best_idx: usize = 0;
    let best_val: f32 = v[0];
    let i: usize = 1;
    while (i < v.len) : (i += 1) {
        if (v[i] > best_val) {
            best_val = v[i];
            best_idx = i;
        }
    }
    return best_idx;
}

fn lessByScoreAsc(scores: []const f32, lhs_idx: usize, rhs_idx: usize): bool {
    const sl = scores[lhs_idx];
    const sr = scores[rhs_idx];
    if (sl != sr) return sl < sr;
    // Tie-break: larger index is considered "smaller" so smaller indices stay in top-K.
    return lhs_idx > rhs_idx;
}

fn siftDownMinHeap(heap: []usize, scores: []const f32, start: usize) {
    let idx = start;
    const n = heap.len;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= n) break;
        const right = left + 1;
        let smallest = left;
        if (right < n && lessByScoreAsc(scores, heap[right], heap[left])) {
            smallest = right;
        }
        if (!lessByScoreAsc(scores, heap[smallest], heap[idx])) break;
        const tmp = heap[idx];
        heap[idx] = heap[smallest];
        heap[smallest] = tmp;
        idx = smallest;
    }
}

/// Returns up to `k` indices ordered by descending `scores` using an O(N log k) min-heap.
pub fn topKIndices(alloc: std.mem.Allocator, scores: []const f32, k: usize): ([]usize)! {
    if (k == 0 || scores.len == 0) {
        return alloc.alloc(usize, 0)?;
    }

    const actual_k = if (k < scores.len) k else scores.len;
    let heap = alloc.alloc(usize, actual_k)?;
    errdefer alloc.free(heap);

    let k_idx: usize = 0;
    while (k_idx < actual_k) : (k_idx += 1) {
        heap[k_idx] = k_idx;
    }

    // Build min-heap in O(k)
    if (actual_k > 1) {
        let i: usize = actual_k / 2;
        while (i > 0) {
            i -= 1;
            siftDownMinHeap(heap, scores, i);
        }
    }

    // Stream remaining scores in O((N - k) log k)
    let s_idx: usize = actual_k;
    while (s_idx < scores.len) : (s_idx += 1) {
        if (lessByScoreAsc(scores, heap[0], s_idx)) {
            heap[0] = s_idx;
            siftDownMinHeap(heap, scores, 0);
        }
    }

    // Sort descending by score (tie-break smaller index first)
    let end: usize = actual_k;
    while (end > 1) {
        end -= 1;
        const tmp = heap[0];
        heap[0] = heap[end];
        heap[end] = tmp;
        siftDownMinHeap(heap[0..end], scores, 0);
    }

    return heap;
}

/// Quantizes an f32 vector into symmetric signed 8-bit integers (`[-127, 127]`).
pub fn quantizeSymmetricI8(alloc: std.mem.Allocator, v: []const f32): QuantizedVecI8! {
    let out = alloc.alloc(i8, v.len)?;
    errdefer alloc.free(out);

    if (v.len == 0) {
        return QuantizedVecI8 { scale: 0.0, data: out };
    }

    let max_abs: f32 = 0.0;
    for (x of v) {
        const ax = @abs(x);
        if (ax > max_abs) max_abs = ax;
    }

    if (max_abs <= 0.0) {
        @memset(out, 0);
        return QuantizedVecI8 { scale: 0.0, data: out };
    }

    const scale = max_abs / 127.0;
    const inv_scale = 127.0 / max_abs;
    let i: usize = 0;
    while (i < v.len) : (i += 1) {
        let q: f32 = @round(v[i] * inv_scale);
        if (q > 127.0) q = 127.0;
        if (q < -127.0) q = -127.0;
        const qi: i32 = @intFromFloat(q);
        out[i] = qi as i8;
    }

    return QuantizedVecI8 { scale: scale, data: out };
}

/// Dequantizes a `QuantizedVecI8` back to `[]f32` (`data[i] * scale`).
pub fn dequantizeSymmetricI8(alloc: std.mem.Allocator, q: QuantizedVecI8): ([]f32)! {
    let out = alloc.alloc(f32, q.data.len)?;
    for (i of 0..q.data.len) {
        out[i] = (q.data[i] as f32) * q.scale;
    }
    return out;
}

/// Computes the approximate dot product of two symmetric Int8 quantized vectors.
pub fn dotQuantizedI8(a: QuantizedVecI8, b: QuantizedVecI8): f32 {
    if (a.data.len != b.data.len || a.data.len == 0) return 0.0;
    let acc: i64 = 0;
    for (i of 0..a.data.len) {
        acc += (a.data[i] as i64) * (b.data[i] as i64);
    }
    return (acc as f32) * a.scale * b.scale;
}

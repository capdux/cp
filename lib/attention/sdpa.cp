// Source: cp/lib/attention/sdpa.cp (Scaled Dot-Product Attention & GQA)
const std = @import("std");

pub const AttnError = error{
    InvalidDimension,
    HeadMismatch,
    InvalidStartPos,
    EmptyInput,
    CacheFull,
    InvalidLayer,
    InvalidTemperature,
    InvalidTopK,
    InvalidTopP,
    OutOfMemory,
};

pub fn scaledDotProductAttention(
    alloc: std.mem.Allocator,
    q: []const f32,
    k: []const f32,
    v: []const f32,
    seq_q: usize,
    seq_k: usize,
    head_dim: usize,
    causal: bool,
    start_pos: usize,
): AttnError![]f32 {
    if (seq_q == 0 or seq_k == 0 or head_dim == 0) {
        return AttnError.InvalidDimension;
    }
    if (q.len != seq_q * head_dim or k.len != seq_k * head_dim or v.len != seq_k * head_dim) {
        return AttnError.InvalidDimension;
    }
    if (causal and start_pos + seq_q > seq_k) {
        return AttnError.InvalidStartPos;
    }

    let out = alloc.alloc(f32, seq_q * head_dim) catch return AttnError.OutOfMemory;
    errdefer alloc.free(out);

    let scores = alloc.alloc(f32, seq_k) catch return AttnError.OutOfMemory;
    defer alloc.free(scores);

    const scale: f32 = 1.0 / @sqrt(@as(f32, @floatFromInt(head_dim)));

    let qi: usize = 0;
    while (qi < seq_q) : (qi += 1) {
        const q_off = qi * head_dim;
        const max_k: usize = if (causal) @min(seq_k, start_pos + qi + 1) else seq_k;

        let max_score: f32 = -std.math.inf(f32);
        let kj: usize = 0;
        while (kj < max_k) : (kj += 1) {
            const k_off = kj * head_dim;
            let dot: f32 = 0.0;
            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                dot += q[q_off + d] * k[k_off + d];
            }
            const s = dot * scale;
            scores[kj] = s;
            if (s > max_score) {
                max_score = s;
            }
        }

        let exp_sum: f32 = 0.0;
        kj = 0;
        while (kj < max_k) : (kj += 1) {
            const w = @exp(scores[kj] - max_score);
            scores[kj] = w;
            exp_sum += w;
        }
        while (kj < seq_k) : (kj += 1) {
            scores[kj] = 0.0;
        }

        const inv_sum: f32 = if (exp_sum > 0.0) 1.0 / exp_sum else 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            out[q_off + d] = 0.0;
        }

        kj = 0;
        while (kj < max_k) : (kj += 1) {
            const prob = scores[kj] * inv_sum;
            const v_off = kj * head_dim;
            d = 0;
            while (d < head_dim) : (d += 1) {
                out[q_off + d] += prob * v[v_off + d];
            }
        }
    }

    return out;
}

pub fn groupedQueryAttention(
    alloc: std.mem.Allocator,
    q: []const f32,
    k_cache: []const f32,
    v_cache: []const f32,
    num_q_heads: usize,
    num_kv_heads: usize,
    seq_k: usize,
    head_dim: usize,
): AttnError![]f32 {
    if (num_q_heads == 0 or num_kv_heads == 0 or seq_k == 0 or head_dim == 0) {
        return AttnError.InvalidDimension;
    }
    if (num_q_heads % num_kv_heads != 0) {
        return AttnError.HeadMismatch;
    }
    if (q.len != num_q_heads * head_dim or
        k_cache.len != num_kv_heads * seq_k * head_dim or
        v_cache.len != num_kv_heads * seq_k * head_dim)
    {
        return AttnError.InvalidDimension;
    }

    let out = alloc.alloc(f32, num_q_heads * head_dim) catch return AttnError.OutOfMemory;
    errdefer alloc.free(out);

    let scores = alloc.alloc(f32, seq_k) catch return AttnError.OutOfMemory;
    defer alloc.free(scores);

    const group_size = num_q_heads / num_kv_heads;
    const kv_stride = num_kv_heads * head_dim;
    const scale: f32 = 1.0 / @sqrt(@as(f32, @floatFromInt(head_dim)));

    let qh: usize = 0;
    while (qh < num_q_heads) : (qh += 1) {
        const kvh = qh / group_size;
        const q_off = qh * head_dim;
        const kvh_off = kvh * head_dim;

        let max_score: f32 = -std.math.inf(f32);
        let kj: usize = 0;
        while (kj < seq_k) : (kj += 1) {
            const k_off = kj * kv_stride + kvh_off;
            let dot: f32 = 0.0;
            let d: usize = 0;
            while (d < head_dim) : (d += 1) {
                dot += q[q_off + d] * k_cache[k_off + d];
            }
            const s = dot * scale;
            scores[kj] = s;
            if (s > max_score) {
                max_score = s;
            }
        }

        let exp_sum: f32 = 0.0;
        kj = 0;
        while (kj < seq_k) : (kj += 1) {
            const w = @exp(scores[kj] - max_score);
            scores[kj] = w;
            exp_sum += w;
        }

        const inv_sum: f32 = if (exp_sum > 0.0) 1.0 / exp_sum else 0.0;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            out[q_off + d] = 0.0;
        }

        kj = 0;
        while (kj < seq_k) : (kj += 1) {
            const prob = scores[kj] * inv_sum;
            const v_off = kj * kv_stride + kvh_off;
            d = 0;
            while (d < head_dim) : (d += 1) {
                out[q_off + d] += prob * v_cache[v_off + d];
            }
        }
    }

    return out;
}

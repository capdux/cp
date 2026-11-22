// Origin: Pure-CP Stdlib (KIVI Asymmetric 2-Bit Per-Channel Key / Per-Token Value KV-Cache & Hybrid Residual Attention) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation KIVI 2-bit KV-Cache compression & attention engine:
//   - 2-bit quad bit-packing primitives (`pack2BitQuad`, `unpack2BitLane`) — 4 values per byte (16x vs FP32)
//   - Per-token asymmetric 2-bit Value quantization (`quantizeToken2BitPerToken`, `dequantizeToken2BitPerToken`)
//   - Per-channel asymmetric 2-bit Key quantization (`quantizeKeys2BitPerChannel`, `dequantizeKey2BitPerChannel`,
//     `kiviDotQueryKey2Bit`) preserving fixed-channel outlier precision without full dequantization buffers
//   - Hybrid `KiviKvCache` combining 2-bit quantized main storage + full-precision FP32 residual window
//     accelerated by native `@dot` with unified numerically stable Softmax single-query decoding

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & Affine Parameter Model
// ============================================================================

/// Maximum supported attention head dimension (`64`, must be a multiple of 4).
pub const KIVI_MAX_HEAD_DIM: usize = 64;

/// Maximum number of quantized + residual tokens stored inline in `KiviKvCache`.
pub const KIVI_MAX_TOKENS: usize = 128;

/// Maximum packed bytes per head vector (`64 / 4 = 16`, 4 x 2-bit lanes per byte).
pub const KIVI_PACKED_STRIDE: usize = 16;

const KIVI_MAX_RESIDUAL: usize = 16;
const KIVI_MAX_GROUPS: usize = 32;

/// Error set for KIVI 2-bit KV-Cache operations.
pub err KiviError {
    InvalidHeadDim,
    CapacityFull,
    DimMismatch,
}

/// Asymmetric affine quantization parameters: `x_hat = zero_point + q * scale` for `q in 0..=3`.
pub struct KiviAffineParams {
    scale: f32 = 0.0;
    zero_point: f32 = 0.0;
}

/// Memory footprint and compression telemetry for `KiviKvCache`.
pub struct KiviCacheStats {
    quantized_tokens: usize = 0;
    residual_tokens: usize = 0;
    total_tokens: usize = 0;
    compressed_bytes: usize = 0;
    fp32_equiv_bytes: usize = 0;
    compression_ratio_x100: u32 = 100;
}

// ============================================================================
// 2. 2-Bit Bit-Packing Primitives
// ============================================================================

/// Packs four 2-bit quantized integers (`0..=3`) into a single byte (`q0` in bits 0..1 .. `q3` in bits 6..7).
pub fn pack2BitQuad(q0: u8, q1: u8, q2: u8, q3: u8): u8 {
    return (q0 & 0x03) | ((q1 & 0x03) << 2) | ((q2 & 0x03) << 4) | ((q3 & 0x03) << 6);
}

/// Unpacks lane `0..=3` (2-bit value `0..=3`) from a packed byte.
pub fn unpack2BitLane(packed_byte: u8, lane: usize): u8 {
    const shift: u3 = @as(u3, @intCast((lane & 3) * 2));
    return (packed_byte >> shift) & 0x03;
}

fn quantizeScalar2Bit(val: f32, params: KiviAffineParams): u8 {
    if (params.scale <= 1.0e-12) return 0;
    const norm: f32 = (val - params.zero_point) / params.scale;
    if (norm <= 0.5) return 0;
    if (norm < 1.5) return 1;
    if (norm < 2.5) return 2;
    return 3;
}

fn computeAffineParams(min_v: f32, max_v: f32): KiviAffineParams {
    const span: f32 = max_v - min_v;
    if (span <= 1.0e-12) {
        return KiviAffineParams{ scale: 0.0, zero_point: min_v };
    }
    return KiviAffineParams{ scale: span / 3.0, zero_point: min_v };
}

// ============================================================================
// 3. Per-Token Value & Per-Channel Key 2-Bit Quantization Kernels
// ============================================================================

/// Quantizes a single Value vector `vec[0..dim]` along the channel dimension using asymmetric 2-bit
/// quantization, packing 4 lanes per byte into `out_packed`.
pub fn quantizeToken2BitPerToken(
    vec: []const f32,
    out_packed: *[]u8,
): KiviAffineParams {
    if (vec.len == 0) return KiviAffineParams{};
    let min_v: f32 = vec[0];
    let max_v: f32 = vec[0];
    let i: usize = 1;
    while (i < vec.len) : (i += 1) {
        if (vec[i] < min_v) min_v = vec[i];
        if (vec[i] > max_v) max_v = vec[i];
    }

    const params: KiviAffineParams = computeAffineParams(min_v, max_v);
    const n_bytes: usize = (vec.len + 3) / 4;
    let b: usize = 0;
    while (b < n_bytes && b < out_packed.len) : (b += 1) {
        const base: usize = b * 4;
        const q0: u8 = if (base < vec.len) quantizeScalar2Bit(vec[base], params) else 0;
        const q1: u8 = if (base + 1 < vec.len) quantizeScalar2Bit(vec[base + 1], params) else 0;
        const q2: u8 = if (base + 2 < vec.len) quantizeScalar2Bit(vec[base + 2], params) else 0;
        const q3: u8 = if (base + 3 < vec.len) quantizeScalar2Bit(vec[base + 3], params) else 0;
        out_packed[b] = pack2BitQuad(q0, q1, q2, q3);
    }
    return params;
}

/// Dequantizes a 2-bit per-token packed Value vector back into `out_vec[0..dim]`.
pub fn dequantizeToken2BitPerToken(
    packed_buf: []const u8,
    dim: usize,
    params: KiviAffineParams,
    out_vec: *[]f32,
) {
    let c: usize = 0;
    while (c < dim && c < out_vec.len) : (c += 1) {
        const byte_idx: usize = c / 4;
        const q: u8 = if (byte_idx < packed_buf.len) unpack2BitLane(packed_buf[byte_idx], c % 4) else 0;
        out_vec[c] = params.zero_point + (q as f32) * params.scale;
    }
}

/// Quantizes `num_tokens` Key vectors (`flat_keys[num_tokens * dim]`) along the token dimension
/// independently per channel, writing `[min_c, max_c]` affine parameters to `out_ch_params[0..dim]`
/// and packed 2-bit keys to `out_packed_keys[num_tokens * ((dim + 3) / 4)]`.
pub fn quantizeKeys2BitPerChannel(
    flat_keys: []const f32,
    num_tokens: usize,
    dim: usize,
    out_packed_keys: *[]u8,
    out_ch_params: *[]KiviAffineParams,
) {
    if (num_tokens == 0 || dim == 0) return;
    if (flat_keys.len < num_tokens * dim || out_ch_params.len < dim) return;
    const stride: usize = (dim + 3) / 4;
    if (out_packed_keys.len < num_tokens * stride) return;

    // Step 1: compute per-channel [min_c, max_c] across all tokens in the group
    let c: usize = 0;
    while (c < dim) : (c += 1) {
        let min_c: f32 = flat_keys[c];
        let max_c: f32 = flat_keys[c];
        let t: usize = 1;
        while (t < num_tokens) : (t += 1) {
            const v: f32 = flat_keys[t * dim + c];
            if (v < min_c) min_c = v;
            if (v > max_c) max_c = v;
        }
        out_ch_params[c] = computeAffineParams(min_c, max_c);
    }

    // Step 2: pack each token's quantized channels 4-per-byte
    let t: usize = 0;
    while (t < num_tokens) : (t += 1) {
        const key_off: usize = t * dim;
        const pack_off: usize = t * stride;
        let b: usize = 0;
        while (b < stride) : (b += 1) {
            const base: usize = b * 4;
            const q0: u8 = if (base < dim) quantizeScalar2Bit(flat_keys[key_off + base], out_ch_params[base]) else 0;
            const q1: u8 = if (base + 1 < dim) quantizeScalar2Bit(flat_keys[key_off + base + 1], out_ch_params[base + 1]) else 0;
            const q2: u8 = if (base + 2 < dim) quantizeScalar2Bit(flat_keys[key_off + base + 2], out_ch_params[base + 2]) else 0;
            const q3: u8 = if (base + 3 < dim) quantizeScalar2Bit(flat_keys[key_off + base + 3], out_ch_params[base + 3]) else 0;
            out_packed_keys[pack_off + b] = pack2BitQuad(q0, q1, q2, q3);
        }
    }
}

/// Dequantizes a single per-channel 2-bit packed Key vector into `out_vec[0..dim]`.
pub fn dequantizeKey2BitPerChannel(
    packed_key: []const u8,
    dim: usize,
    ch_params: []const KiviAffineParams,
    out_vec: *[]f32,
) {
    let c: usize = 0;
    while (c < dim && c < out_vec.len && c < ch_params.len) : (c += 1) {
        const byte_idx: usize = c / 4;
        const q: u8 = if (byte_idx < packed_key.len) unpack2BitLane(packed_key[byte_idx], c % 4) else 0;
        out_vec[c] = ch_params[c].zero_point + (q as f32) * ch_params[c].scale;
    }
}

/// Computes the asymmetric dot product `dot(q, dequant(packed_key))` directly from 2-bit packed lanes
/// without allocating or materializing a dequantized key buffer.
pub fn kiviDotQueryKey2Bit(
    q: []const f32,
    packed_key: []const u8,
    dim: usize,
    ch_params: []const KiviAffineParams,
): f32 {
    let acc: f64 = 0.0;
    let c: usize = 0;
    while (c < dim && c < q.len && c < ch_params.len) : (c += 1) {
        const byte_idx: usize = c / 4;
        const q_lane: u8 = if (byte_idx < packed_key.len) unpack2BitLane(packed_key[byte_idx], c % 4) else 0;
        const k_hat: f64 = (ch_params[c].zero_point as f64) + (q_lane as f64) * (ch_params[c].scale as f64);
        acc += (q[c] as f64) * k_hat;
    }
    return acc as f32;
}

// ============================================================================
// 4. Hybrid Residual + 2-Bit Quantized `KiviKvCache`
// ============================================================================

/// Zero-heap-allocation KIVI hybrid KV-Cache combining:
/// - 2-bit quantized main storage (per-channel Key groups + per-token Values)
/// - Recent full-precision FP32 residual window (automatically flushed & quantized in batches)
pub struct KiviKvCache {
    head_dim: usize = 16;
    packed_stride: usize = 4;
    residual_window: usize = 4;

    quantized_count: usize = 0;
    group_count: usize = 0;
    q_keys: [KIVI_MAX_TOKENS][KIVI_PACKED_STRIDE]u8 = [[0; KIVI_PACKED_STRIDE]; KIVI_MAX_TOKENS];
    q_vals: [KIVI_MAX_TOKENS][KIVI_PACKED_STRIDE]u8 = [[0; KIVI_PACKED_STRIDE]; KIVI_MAX_TOKENS];
    q_val_params: [KIVI_MAX_TOKENS]KiviAffineParams = [KiviAffineParams{}; KIVI_MAX_TOKENS];
    q_key_group_id: [KIVI_MAX_TOKENS]u8 = [0; KIVI_MAX_TOKENS];
    q_key_ch_params: [KIVI_MAX_GROUPS][KIVI_MAX_HEAD_DIM]KiviAffineParams = [[KiviAffineParams{}; KIVI_MAX_HEAD_DIM]; KIVI_MAX_GROUPS];

    residual_count: usize = 0;
    res_keys: [KIVI_MAX_RESIDUAL][KIVI_MAX_HEAD_DIM]f32 = [[0.0; KIVI_MAX_HEAD_DIM]; KIVI_MAX_RESIDUAL];
    res_vals: [KIVI_MAX_RESIDUAL][KIVI_MAX_HEAD_DIM]f32 = [[0.0; KIVI_MAX_HEAD_DIM]; KIVI_MAX_RESIDUAL];

    /// Initializes a `KiviKvCache` with even 4-aligned `head_dim <= 64` and `residual_window in 1..=16`.
    static fn init(head_dim: usize, residual_window: usize): KiviKvCache ! KiviError {
        if (head_dim == 0 || head_dim > KIVI_MAX_HEAD_DIM || (head_dim % 4) != 0) {
            return KiviError.InvalidHeadDim;
        }
        if (residual_window == 0 || residual_window > KIVI_MAX_RESIDUAL) {
            return KiviError.DimMismatch;
        }
        return KiviKvCache{
            head_dim: head_dim,
            packed_stride: head_dim / 4,
            residual_window: residual_window,
        };
    }

    fn flushResidualToQuantized(*_): void ! KiviError {
        if (residual_count == 0) return;
        if (quantized_count + residual_count > KIVI_MAX_TOKENS || group_count >= KIVI_MAX_GROUPS) {
            return KiviError.CapacityFull;
        }

        const gid: usize = group_count;
        // Per-channel Key affine params across the flushed residual batch
        let c: usize = 0;
        while (c < head_dim) : (c += 1) {
            let min_c: f32 = res_keys[0][c];
            let max_c: f32 = res_keys[0][c];
            let r: usize = 1;
            while (r < residual_count) : (r += 1) {
                const v: f32 = res_keys[r][c];
                if (v < min_c) min_c = v;
                if (v > max_c) max_c = v;
            }
            q_key_ch_params[gid][c] = computeAffineParams(min_c, max_c);
        }

        // Pack each residual token's Key (per-channel) and Value (per-token)
        let r: usize = 0;
        while (r < residual_count) : (r += 1) {
            const dst_tok: usize = quantized_count + r;
            q_key_group_id[dst_tok] = @as(u8, @intCast(gid));

            let b: usize = 0;
            while (b < packed_stride) : (b += 1) {
                const base: usize = b * 4;
                const q0: u8 = quantizeScalar2Bit(res_keys[r][base], q_key_ch_params[gid][base]);
                const q1: u8 = quantizeScalar2Bit(res_keys[r][base + 1], q_key_ch_params[gid][base + 1]);
                const q2: u8 = quantizeScalar2Bit(res_keys[r][base + 2], q_key_ch_params[gid][base + 2]);
                const q3: u8 = quantizeScalar2Bit(res_keys[r][base + 3], q_key_ch_params[gid][base + 3]);
                q_keys[dst_tok][b] = pack2BitQuad(q0, q1, q2, q3);
            }

            let vp_dst: []u8 = q_vals[dst_tok][0..packed_stride];
            q_val_params[dst_tok] = quantizeToken2BitPerToken(res_vals[r][0..head_dim], *vp_dst);
        }

        quantized_count += residual_count;
        group_count += 1;
        residual_count = 0;
    }

    /// Appends a single `(k_vec, v_vec)` token into the FP32 residual window.
    /// Automatically quantizes and archives the residual batch once it reaches `residual_window`.
    fn appendToken(*_, k_vec: []const f32, v_vec: []const f32): void ! KiviError {
        if (k_vec.len != head_dim || v_vec.len != head_dim) {
            return KiviError.DimMismatch;
        }
        if (residual_count == residual_window) {
            flushResidualToQuantized()?;
        }
        if (quantized_count + residual_count >= KIVI_MAX_TOKENS) {
            return KiviError.CapacityFull;
        }

        let c: usize = 0;
        while (c < head_dim) : (c += 1) {
            res_keys[residual_count][c] = k_vec[c];
            res_vals[residual_count][c] = v_vec[c];
        }
        residual_count += 1;
    }

    /// Performs unified numerically stable single-query attention over both the 2-bit quantized
    /// main storage and the FP32 residual window (accelerated by native `@dot`).
    fn decodeSingleQuery(q_vec: []const f32, out_vec: *[]f32): void ! KiviError {
        if (q_vec.len != head_dim || out_vec.len != head_dim) {
            return KiviError.DimMismatch;
        }
        const total: usize = quantized_count + residual_count;
        if (total == 0) {
            let z: usize = 0;
            while (z < head_dim) : (z += 1) {
                out_vec[z] = 0.0;
            }
            return;
        }

        const scale: f64 = 1.0 / std.math.sqrt(head_dim as f64);
        let scores: [KIVI_MAX_TOKENS]f64 = [0.0; KIVI_MAX_TOKENS];
        let max_score: f64 = -1.0e300;

        // Pass 1a: quantized keys via direct 2-bit per-channel dot product
        let qt: usize = 0;
        while (qt < quantized_count) : (qt += 1) {
            const gid: usize = @as(usize, q_key_group_id[qt]);
            const raw_dot: f32 = kiviDotQueryKey2Bit(
                q_vec,
                q_keys[qt][0..packed_stride],
                head_dim,
                q_key_ch_params[gid][0..head_dim],
            );
            const sc: f64 = (raw_dot as f64) * scale;
            scores[qt] = sc;
            if (sc > max_score) max_score = sc;
        }

        // Pass 1b: FP32 residual keys via native SIMD `@dot`
        let rt: usize = 0;
        while (rt < residual_count) : (rt += 1) {
            const raw_dot: f32 = @dot(q_vec, res_keys[rt][0..head_dim]);
            const sc: f64 = (raw_dot as f64) * scale;
            scores[quantized_count + rt] = sc;
            if (sc > max_score) max_score = sc;
        }

        // Pass 2: Softmax weighted aggregation over quantized & residual values
        let acc: [KIVI_MAX_HEAD_DIM]f64 = [0.0; KIVI_MAX_HEAD_DIM];
        let sum_w: f64 = 0.0;

        qt = 0;
        while (qt < quantized_count) : (qt += 1) {
            const w: f64 = std.math.exp(scores[qt] - max_score);
            sum_w += w;
            const vp: KiviAffineParams = q_val_params[qt];
            const zp: f64 = vp.zero_point as f64;
            const sc_v: f64 = vp.scale as f64;
            let c: usize = 0;
            while (c < head_dim) : (c += 1) {
                const q_lane: u8 = unpack2BitLane(q_vals[qt][c / 4], c % 4);
                const v_hat: f64 = zp + (q_lane as f64) * sc_v;
                acc[c] += w * v_hat;
            }
        }

        rt = 0;
        while (rt < residual_count) : (rt += 1) {
            const w: f64 = std.math.exp(scores[quantized_count + rt] - max_score);
            sum_w += w;
            let c: usize = 0;
            while (c < head_dim) : (c += 1) {
                acc[c] += w * (res_vals[rt][c] as f64);
            }
        }

        const inv_sum: f64 = if (sum_w > 0.0) (1.0 / sum_w) else 0.0;
        let oc: usize = 0;
        while (oc < head_dim) : (oc += 1) {
            out_vec[oc] = (acc[oc] * inv_sum) as f32;
        }
    }

    /// Returns memory usage and compression ratio telemetry.
    fn stats(): KiviCacheStats {
        const total: usize = quantized_count + residual_count;
        const fp32_bytes: usize = total * head_dim * 8; // K + V in FP32 (4B each)
        const quant_payload_bytes: usize = quantized_count * packed_stride * 2;
        const res_payload_bytes: usize = residual_count * head_dim * 8;
        const comp_bytes: usize = quant_payload_bytes + res_payload_bytes;
        const ratio_x100: u32 = if (comp_bytes > 0) @as(u32, @intCast((fp32_bytes * 100) / comp_bytes)) else 100;

        return KiviCacheStats{
            quantized_tokens: quantized_count,
            residual_tokens: residual_count,
            total_tokens: total,
            compressed_bytes: comp_bytes,
            fp32_equiv_bytes: fp32_bytes,
            compression_ratio_x100: ratio_x100,
        };
    }
}

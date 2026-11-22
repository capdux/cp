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

// ============================================================================
// 5. Unbounded External-Slice Multi-Head GQA KIVI 2-Bit Engine (KiviFlatKvCache)
//    + ICML 2024 Grouped Archival + Single-Pass Online Softmax + CRC32 Snapshot
// ============================================================================

/// Little-endian ASCII magic `"KIVI"` (`0x4956494B`) for binary KV-Cache snapshots.
pub const KIVI_SNAPSHOT_MAGIC: u32 = 0x4956494B;

/// Current binary KV-Cache snapshot format version (`1`).
pub const KIVI_SNAPSHOT_VERSION: u32 = 1;

const KIVI_SNAPSHOT_HEADER_BYTES: usize = 40;
const KIVI_SNAPSHOT_CRC_BYTES: usize = 4;

/// Error set for unbounded `KiviFlatKvCache` operations and snapshot persistence.
pub err KiviFlatError {
    InvalidConfig,
    DimMismatch,
    CapacityFull,
    BufferTooSmall,
    InvalidSnapshot,
    ChecksumMismatch,
    HeadOutOfBounds,
}

/// Configuration for the unbounded multi-head GQA `KiviFlatKvCache` engine.
/// Supports arbitrary `num_kv_heads`, `head_dim` (multiple of 4, e.g. 64/128/256),
/// `max_tokens` (e.g. 4K/32K+), `group_size` (e.g. 32/64), and `residual_window >= group_size`.
pub struct KiviFlatConfig {
    num_kv_heads: usize = 4;
    head_dim: usize = 128;
    max_tokens: usize = 4096;
    group_size: usize = 32;
    residual_window: usize = 64;

    /// Validates head count, 4-aligned head dimension, group size, and residual window constraints.
    fn isValid(): bool {
        if (num_kv_heads == 0 || head_dim == 0 || (head_dim % 4) != 0) return false;
        if (max_tokens == 0 || group_size == 0) return false;
        if (residual_window < group_size) return false;
        return true;
    }
}

fn kiviMaxGroups(cfg: KiviFlatConfig): usize {
    if (cfg.group_size == 0) return 0;
    return (cfg.max_tokens + cfg.group_size - 1) / cfg.group_size;
}

/// Exact `u8` buffer length required for `packed_keys` or `packed_vals` (`num_kv_heads * max_tokens * (head_dim / 4)`).
pub fn kiviPackedKvBufLen(cfg: KiviFlatConfig): usize {
    return cfg.num_kv_heads * cfg.max_tokens * (cfg.head_dim / 4);
}

/// Exact `KiviAffineParams` buffer length required for `key_group_params` (`num_kv_heads * max_groups * head_dim`).
pub fn kiviKeyGroupParamsBufLen(cfg: KiviFlatConfig): usize {
    return cfg.num_kv_heads * kiviMaxGroups(cfg) * cfg.head_dim;
}

/// Exact `KiviAffineParams` buffer length required for `val_tok_params` (`num_kv_heads * max_tokens`).
pub fn kiviValTokParamsBufLen(cfg: KiviFlatConfig): usize {
    return cfg.num_kv_heads * cfg.max_tokens;
}

/// Exact `f32` buffer length required for `res_keys` or `res_vals` (`num_kv_heads * residual_window * head_dim`).
pub fn kiviResidualBufLen(cfg: KiviFlatConfig): usize {
    return cfg.num_kv_heads * cfg.residual_window * cfg.head_dim;
}

fn kiviSnapshotExactBytes(cfg: KiviFlatConfig, q_count: usize, g_count: usize, r_count: usize): usize {
    const stride: usize = cfg.head_dim / 4;
    const packed_kv_bytes: usize = cfg.num_kv_heads * q_count * stride * 2;
    const key_gp_bytes: usize = cfg.num_kv_heads * g_count * cfg.head_dim * 8;
    const val_tp_bytes: usize = cfg.num_kv_heads * q_count * 8;
    const res_kv_bytes: usize = cfg.num_kv_heads * r_count * cfg.head_dim * 8;
    return KIVI_SNAPSHOT_HEADER_BYTES
        + packed_kv_bytes
        + key_gp_bytes
        + val_tp_bytes
        + res_kv_bytes
        + KIVI_SNAPSHOT_CRC_BYTES;
}

/// Upper-bound byte capacity required to serialize a `KiviFlatKvCache` snapshot holding `total_tokens`.
pub fn kiviSnapshotMaxBytes(cfg: KiviFlatConfig, total_tokens: usize): usize {
    const g_max: usize = if (cfg.group_size > 0) total_tokens / cfg.group_size else 0;
    const r_max: usize = if (total_tokens < cfg.residual_window) total_tokens else cfg.residual_window;
    return kiviSnapshotExactBytes(cfg, total_tokens, g_max, r_max);
}

// ---- Internal IEEE CRC32 & Little-Endian Helpers ----

fn kiviCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc ^= data[i] as u32;
        let bit: u32 = 0;
        while (bit < 8) : (bit += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    return crc ^ 0xFFFFFFFF;
}

fn kiviWriteLeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn kiviReadLeU32(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn kiviWriteAffine(buf: []u8, pos: usize, p: KiviAffineParams): usize {
    const sc_bits: u32 = @bitCast(p.scale);
    const zp_bits: u32 = @bitCast(p.zero_point);
    kiviWriteLeU32(buf, pos, sc_bits);
    kiviWriteLeU32(buf, pos + 4, zp_bits);
    return pos + 8;
}

fn kiviReadAffine(buf: []const u8, pos: usize): KiviAffineParams {
    const sc: f32 = @bitCast(kiviReadLeU32(buf, pos));
    const zp: f32 = @bitCast(kiviReadLeU32(buf, pos + 4));
    return KiviAffineParams{ scale: sc, zero_point: zp };
}

fn packValueTokenDirect(src: []const f32, out_packed: []u8): KiviAffineParams {
    if (src.len == 0) return KiviAffineParams{};
    let min_v: f32 = src[0];
    let max_v: f32 = src[0];
    let i: usize = 1;
    while (i < src.len) : (i += 1) {
        if (src[i] < min_v) min_v = src[i];
        if (src[i] > max_v) max_v = src[i];
    }
    const params: KiviAffineParams = computeAffineParams(min_v, max_v);
    const n_bytes: usize = src.len / 4;
    let b: usize = 0;
    while (b < n_bytes) : (b += 1) {
        const base: usize = b * 4;
        const q0: u8 = quantizeScalar2Bit(src[base], params);
        const q1: u8 = quantizeScalar2Bit(src[base + 1], params);
        const q2: u8 = quantizeScalar2Bit(src[base + 2], params);
        const q3: u8 = quantizeScalar2Bit(src[base + 3], params);
        out_packed[b] = pack2BitQuad(q0, q1, q2, q3);
    }
    return params;
}

/// Unbounded external-slice multi-head GQA KIVI 2-bit KV-Cache engine implementing ICML 2024
/// grouped per-channel Key + per-token Value quantization, single-pass Online Softmax decoding
/// (zero score/dequant buffers), and CRC32 binary snapshot persistence.
pub struct KiviFlatKvCache {
    cfg: KiviFlatConfig = KiviFlatConfig{},
    packed_stride: usize = 0;
    max_groups: usize = 0;
    quantized_count: usize = 0;
    group_count: usize = 0;
    residual_count: usize = 0;

    packed_keys: []u8 = [],
    packed_vals: []u8 = [],
    key_group_params: []KiviAffineParams = [],
    val_tok_params: []KiviAffineParams = [],
    res_keys: []f32 = [],
    res_vals: []f32 = [],

    /// Initializes an empty `KiviFlatKvCache` backed entirely by caller-provided slices.
    static fn init(
        cfg: KiviFlatConfig,
        packed_keys: []u8,
        packed_vals: []u8,
        key_group_params: []KiviAffineParams,
        val_tok_params: []KiviAffineParams,
        res_keys: []f32,
        res_vals: []f32,
    ): KiviFlatKvCache ! KiviFlatError {
        if (!cfg.isValid()) {
            return KiviFlatError.InvalidConfig;
        }
        const need_pk = kiviPackedKvBufLen(cfg);
        const need_kgp = kiviKeyGroupParamsBufLen(cfg);
        const need_vtp = kiviValTokParamsBufLen(cfg);
        const need_res = kiviResidualBufLen(cfg);
        if (packed_keys.len < need_pk
            || packed_vals.len < need_pk
            || key_group_params.len < need_kgp
            || val_tok_params.len < need_vtp
            || res_keys.len < need_res
            || res_vals.len < need_res)
        {
            return KiviFlatError.BufferTooSmall;
        }

        return KiviFlatKvCache{
            cfg: cfg,
            packed_stride: cfg.head_dim / 4,
            max_groups: kiviMaxGroups(cfg),
            quantized_count: 0,
            group_count: 0,
            residual_count: 0,
            packed_keys: packed_keys,
            packed_vals: packed_vals,
            key_group_params: key_group_params,
            val_tok_params: val_tok_params,
            res_keys: res_keys,
            res_vals: res_vals,
        };
    }

    fn packedOff(h: usize, tok: usize): usize {
        return (h * cfg.max_tokens + tok) * packed_stride;
    }

    fn keyGroupOff(h: usize, g: usize): usize {
        return (h * max_groups + g) * cfg.head_dim;
    }

    fn valParamOff(h: usize, tok: usize): usize {
        return h * cfg.max_tokens + tok;
    }

    fn resOff(h: usize, r: usize): usize {
        return (h * cfg.residual_window + r) * cfg.head_dim;
    }

    // Archives the oldest `cfg.group_size` tokens from the FP32 residual window into 2-bit grouped storage.
    fn archiveOldestGroup(*_): void ! KiviFlatError {
        const gs: usize = cfg.group_size;
        if (residual_count < gs) return;
        if (quantized_count + gs > cfg.max_tokens || group_count >= max_groups) {
            return KiviFlatError.CapacityFull;
        }

        const gid: usize = group_count;
        const dim: usize = cfg.head_dim;
        const rem: usize = residual_count - gs;

        let h: usize = 0;
        while (h < cfg.num_kv_heads) : (h += 1) {
            const kg_off: usize = keyGroupOff(h, gid);

            // 1. Per-channel Key [min_c, max_c] over the oldest `group_size` residual tokens
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                const r0_off: usize = resOff(h, 0);
                let min_c: f32 = res_keys[r0_off + c];
                let max_c: f32 = res_keys[r0_off + c];
                let r: usize = 1;
                while (r < gs) : (r += 1) {
                    const v: f32 = res_keys[resOff(h, r) + c];
                    if (v < min_c) min_c = v;
                    if (v > max_c) max_c = v;
                }
                key_group_params[kg_off + c] = computeAffineParams(min_c, max_c);
            }

            // 2. Pack oldest `group_size` Keys (per-channel) & Values (per-token)
            let r: usize = 0;
            while (r < gs) : (r += 1) {
                const dst_tok: usize = quantized_count + r;
                const rk_off: usize = resOff(h, r);
                const pk_off: usize = packedOff(h, dst_tok);

                let b: usize = 0;
                while (b < packed_stride) : (b += 1) {
                    const base: usize = b * 4;
                    const q0: u8 = quantizeScalar2Bit(res_keys[rk_off + base], key_group_params[kg_off + base]);
                    const q1: u8 = quantizeScalar2Bit(res_keys[rk_off + base + 1], key_group_params[kg_off + base + 1]);
                    const q2: u8 = quantizeScalar2Bit(res_keys[rk_off + base + 2], key_group_params[kg_off + base + 2]);
                    const q3: u8 = quantizeScalar2Bit(res_keys[rk_off + base + 3], key_group_params[kg_off + base + 3]);
                    packed_keys[pk_off + b] = pack2BitQuad(q0, q1, q2, q3);
                }

                const pv_slice: []u8 = packed_vals[pk_off .. pk_off + packed_stride];
                const rv_slice: []const f32 = res_vals[rk_off .. rk_off + dim];
                val_tok_params[valParamOff(h, dst_tok)] = packValueTokenDirect(rv_slice, pv_slice);
            }

            // 3. Shift remaining residual tokens left by `group_size`
            let s: usize = 0;
            while (s < rem) : (s += 1) {
                const dst_off: usize = resOff(h, s);
                const src_off: usize = resOff(h, s + gs);
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    res_keys[dst_off + d] = res_keys[src_off + d];
                    res_vals[dst_off + d] = res_vals[src_off + d];
                }
            }
        }

        quantized_count += gs;
        group_count += 1;
        residual_count = rem;
    }

    /// Appends one token across all KV heads (`k_all_heads.len == v_all_heads.len == num_kv_heads * head_dim`).
    /// When the FP32 residual window reaches `residual_window`, automatically archives the oldest
    /// `group_size` tokens into 2-bit per-channel Key + per-token Value storage.
    fn appendTokenAllHeads(*_, k_all_heads: []const f32, v_all_heads: []const f32): void ! KiviFlatError {
        const expected_len: usize = cfg.num_kv_heads * cfg.head_dim;
        if (k_all_heads.len != expected_len || v_all_heads.len != expected_len) {
            return KiviFlatError.DimMismatch;
        }
        if (residual_count == cfg.residual_window) {
            archiveOldestGroup()?;
        }
        if (quantized_count + residual_count >= cfg.max_tokens) {
            return KiviFlatError.CapacityFull;
        }

        const dim: usize = cfg.head_dim;
        let h: usize = 0;
        while (h < cfg.num_kv_heads) : (h += 1) {
            const dst_off: usize = resOff(h, residual_count);
            const src_off: usize = h * dim;
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                res_keys[dst_off + c] = k_all_heads[src_off + c];
                res_vals[dst_off + c] = v_all_heads[src_off + c];
            }
        }
        residual_count += 1;
    }

    /// Single-pass numerically stable Online Softmax attention decoding for KV head `kv_head_idx`
    /// over all 2-bit quantized groups plus the FP32 residual window (zero intermediate score/dequant buffers).
    fn decodeHeadQuery(
        kv_head_idx: usize,
        q_head: []const f32,
        out_head: []f32,
    ): void ! KiviFlatError {
        if (kv_head_idx >= cfg.num_kv_heads) {
            return KiviFlatError.HeadOutOfBounds;
        }
        const dim: usize = cfg.head_dim;
        if (q_head.len != dim || out_head.len != dim) {
            return KiviFlatError.DimMismatch;
        }

        let z: usize = 0;
        while (z < dim) : (z += 1) {
            out_head[z] = 0.0;
        }
        const total: usize = quantized_count + residual_count;
        if (total == 0) return;

        const inv_sqrt_d: f64 = 1.0 / std.math.sqrt(dim as f64);
        let running_max: f64 = -1.0e300;
        let running_sum: f64 = 0.0;

        // Pass A: all 2-bit quantized tokens in head-major contiguous storage
        let qt: usize = 0;
        while (qt < quantized_count) : (qt += 1) {
            const gid: usize = qt / cfg.group_size;
            const pk_off: usize = packedOff(kv_head_idx, qt);
            const kg_off: usize = keyGroupOff(kv_head_idx, gid);

            const raw_dot: f32 = kiviDotQueryKey2Bit(
                q_head,
                packed_keys[pk_off .. pk_off + packed_stride],
                dim,
                key_group_params[kg_off .. kg_off + dim],
            );
            const score: f64 = (raw_dot as f64) * inv_sqrt_d;
            const vp: KiviAffineParams = val_tok_params[valParamOff(kv_head_idx, qt)];
            const zp: f64 = vp.zero_point as f64;
            const sc_v: f64 = vp.scale as f64;

            if (score > running_max) {
                const rescale: f64 = if (running_sum > 0.0) std.math.exp(running_max - score) else 0.0;
                running_sum = running_sum * rescale + 1.0;
                running_max = score;
                let c: usize = 0;
                while (c < dim) : (c += 1) {
                    const q_lane: u8 = unpack2BitLane(packed_vals[pk_off + (c / 4)], c % 4);
                    const v_hat: f64 = zp + (q_lane as f64) * sc_v;
                    out_head[c] = ((out_head[c] as f64) * rescale + v_hat) as f32;
                }
            } else {
                const w: f64 = std.math.exp(score - running_max);
                running_sum += w;
                let c: usize = 0;
                while (c < dim) : (c += 1) {
                    const q_lane: u8 = unpack2BitLane(packed_vals[pk_off + (c / 4)], c % 4);
                    const v_hat: f64 = zp + (q_lane as f64) * sc_v;
                    out_head[c] = ((out_head[c] as f64) + w * v_hat) as f32;
                }
            }
        }

        // Pass B: FP32 residual window tokens accelerated by native `@dot`
        let rt: usize = 0;
        while (rt < residual_count) : (rt += 1) {
            const roff: usize = resOff(kv_head_idx, rt);
            const raw_dot: f32 = @dot(q_head, res_keys[roff .. roff + dim]);
            const score: f64 = (raw_dot as f64) * inv_sqrt_d;

            if (score > running_max) {
                const rescale: f64 = if (running_sum > 0.0) std.math.exp(running_max - score) else 0.0;
                running_sum = running_sum * rescale + 1.0;
                running_max = score;
                let c: usize = 0;
                while (c < dim) : (c += 1) {
                    out_head[c] = ((out_head[c] as f64) * rescale + (res_vals[roff + c] as f64)) as f32;
                }
            } else {
                const w: f64 = std.math.exp(score - running_max);
                running_sum += w;
                let c: usize = 0;
                while (c < dim) : (c += 1) {
                    out_head[c] = ((out_head[c] as f64) + w * (res_vals[roff + c] as f64)) as f32;
                }
            }
        }

        const inv_sum: f64 = if (running_sum > 0.0) (1.0 / running_sum) else 0.0;
        let oc: usize = 0;
        while (oc < dim) : (oc += 1) {
            out_head[oc] = ((out_head[oc] as f64) * inv_sum) as f32;
        }
    }

    /// Grouped-Query Attention (GQA) multi-head batch decode (`num_q_heads = R * num_kv_heads`).
    fn decodeGqaAllHeads(
        q_all_heads: []const f32,
        num_q_heads: usize,
        out_all_heads: []f32,
    ): void ! KiviFlatError {
        if (num_q_heads == 0 || (num_q_heads % cfg.num_kv_heads) != 0) {
            return KiviFlatError.InvalidConfig;
        }
        const total_dim: usize = num_q_heads * cfg.head_dim;
        if (q_all_heads.len != total_dim || out_all_heads.len != total_dim) {
            return KiviFlatError.DimMismatch;
        }

        const q_per_kv: usize = num_q_heads / cfg.num_kv_heads;
        const dim: usize = cfg.head_dim;
        let qh: usize = 0;
        while (qh < num_q_heads) : (qh += 1) {
            const kv_h: usize = qh / q_per_kv;
            const q_slice: []const f32 = q_all_heads[qh * dim .. (qh + 1) * dim];
            const out_slice: []f32 = out_all_heads[qh * dim .. (qh + 1) * dim];
            decodeHeadQuery(kv_h, q_slice, out_slice)?;
        }
    }

    /// Returns aggregate memory footprint and compression ratio telemetry across all KV heads.
    fn stats(): KiviCacheStats {
        const total: usize = quantized_count + residual_count;
        const fp32_bytes: usize = cfg.num_kv_heads * total * cfg.head_dim * 8;
        const quant_payload_bytes: usize = cfg.num_kv_heads * quantized_count * packed_stride * 2;
        const param_bytes: usize = cfg.num_kv_heads * (group_count * cfg.head_dim + quantized_count) * 8;
        const res_payload_bytes: usize = cfg.num_kv_heads * residual_count * cfg.head_dim * 8;
        const comp_bytes: usize = quant_payload_bytes + param_bytes + res_payload_bytes;
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

/// Serializes a `KiviFlatKvCache` (header, config, quantized 2-bit K/V, per-channel & per-token
/// affine params, FP32 residual window, and trailing IEEE CRC32) into `out_bytes` for zero-recompute
/// system-prompt prefix warm-loading.
pub fn serializeKiviSnapshot(cache: KiviFlatKvCache, out_bytes: []u8): usize ! KiviFlatError {
    const req: usize = kiviSnapshotExactBytes(cache.cfg, cache.quantized_count, cache.group_count, cache.residual_count);
    if (out_bytes.len < req) {
        return KiviFlatError.BufferTooSmall;
    }

    kiviWriteLeU32(out_bytes, 0, KIVI_SNAPSHOT_MAGIC);
    kiviWriteLeU32(out_bytes, 4, KIVI_SNAPSHOT_VERSION);
    kiviWriteLeU32(out_bytes, 8, @as(u32, @intCast(cache.cfg.num_kv_heads)));
    kiviWriteLeU32(out_bytes, 12, @as(u32, @intCast(cache.cfg.head_dim)));
    kiviWriteLeU32(out_bytes, 16, @as(u32, @intCast(cache.cfg.max_tokens)));
    kiviWriteLeU32(out_bytes, 20, @as(u32, @intCast(cache.cfg.group_size)));
    kiviWriteLeU32(out_bytes, 24, @as(u32, @intCast(cache.cfg.residual_window)));
    kiviWriteLeU32(out_bytes, 28, @as(u32, @intCast(cache.quantized_count)));
    kiviWriteLeU32(out_bytes, 32, @as(u32, @intCast(cache.residual_count)));
    kiviWriteLeU32(out_bytes, 36, @as(u32, @intCast(cache.group_count)));

    let pos: usize = KIVI_SNAPSHOT_HEADER_BYTES;
    const dim: usize = cache.cfg.head_dim;
    const stride: usize = cache.packed_stride;

    let h: usize = 0;
    while (h < cache.cfg.num_kv_heads) : (h += 1) {
        // 1. Quantized packed K and V
        let qt: usize = 0;
        while (qt < cache.quantized_count) : (qt += 1) {
            const pk_off: usize = cache.packedOff(h, qt);
            let b: usize = 0;
            while (b < stride) : (b += 1) {
                out_bytes[pos] = cache.packed_keys[pk_off + b];
                pos += 1;
            }
            b = 0;
            while (b < stride) : (b += 1) {
                out_bytes[pos] = cache.packed_vals[pk_off + b];
                pos += 1;
            }
            pos = kiviWriteAffine(out_bytes, pos, cache.val_tok_params[cache.valParamOff(h, qt)]);
        }

        // 2. Per-channel Key group params
        let g: usize = 0;
        while (g < cache.group_count) : (g += 1) {
            const kg_off: usize = cache.keyGroupOff(h, g);
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                pos = kiviWriteAffine(out_bytes, pos, cache.key_group_params[kg_off + c]);
            }
        }

        // 3. FP32 residual K and V
        let rt: usize = 0;
        while (rt < cache.residual_count) : (rt += 1) {
            const roff: usize = cache.resOff(h, rt);
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                const kb: u32 = @bitCast(cache.res_keys[roff + c]);
                kiviWriteLeU32(out_bytes, pos, kb);
                pos += 4;
            }
            c = 0;
            while (c < dim) : (c += 1) {
                const vb: u32 = @bitCast(cache.res_vals[roff + c]);
                kiviWriteLeU32(out_bytes, pos, vb);
                pos += 4;
            }
        }
    }

    const crc: u32 = kiviCrc32(out_bytes[0..pos]);
    kiviWriteLeU32(out_bytes, pos, crc);
    pos += KIVI_SNAPSHOT_CRC_BYTES;
    return pos;
}

/// Deserializes a `KiviFlatKvCache` snapshot with full magic, version, bounds, and IEEE CRC32
/// verification into caller-provided backing slices.
pub fn deserializeKiviSnapshot(
    bytes: []const u8,
    packed_keys: []u8,
    packed_vals: []u8,
    key_group_params: []KiviAffineParams,
    val_tok_params: []KiviAffineParams,
    res_keys: []f32,
    res_vals: []f32,
): KiviFlatKvCache ! KiviFlatError {
    if (bytes.len < KIVI_SNAPSHOT_HEADER_BYTES + KIVI_SNAPSHOT_CRC_BYTES) {
        return KiviFlatError.InvalidSnapshot;
    }
    const magic: u32 = kiviReadLeU32(bytes, 0);
    const version: u32 = kiviReadLeU32(bytes, 4);
    if (magic != KIVI_SNAPSHOT_MAGIC || version != KIVI_SNAPSHOT_VERSION) {
        return KiviFlatError.InvalidSnapshot;
    }

    const cfg = KiviFlatConfig{
        num_kv_heads: @as(usize, kiviReadLeU32(bytes, 8)),
        head_dim: @as(usize, kiviReadLeU32(bytes, 12)),
        max_tokens: @as(usize, kiviReadLeU32(bytes, 16)),
        group_size: @as(usize, kiviReadLeU32(bytes, 20)),
        residual_window: @as(usize, kiviReadLeU32(bytes, 24)),
    };
    const q_count: usize = @as(usize, kiviReadLeU32(bytes, 28));
    const r_count: usize = @as(usize, kiviReadLeU32(bytes, 32));
    const g_count: usize = @as(usize, kiviReadLeU32(bytes, 36));

    if (!cfg.isValid()
        || q_count + r_count > cfg.max_tokens
        || r_count > cfg.residual_window
        || g_count > kiviMaxGroups(cfg)
        || q_count != g_count * cfg.group_size)
    {
        return KiviFlatError.InvalidSnapshot;
    }

    const expected_len: usize = kiviSnapshotExactBytes(cfg, q_count, g_count, r_count);
    if (bytes.len < expected_len) {
        return KiviFlatError.InvalidSnapshot;
    }

    const payload_end: usize = expected_len - KIVI_SNAPSHOT_CRC_BYTES;
    const wire_crc: u32 = kiviReadLeU32(bytes, payload_end);
    const calc_crc: u32 = kiviCrc32(bytes[0..payload_end]);
    if (wire_crc != calc_crc) {
        return KiviFlatError.ChecksumMismatch;
    }

    let cache = KiviFlatKvCache.init(
        cfg,
        packed_keys,
        packed_vals,
        key_group_params,
        val_tok_params,
        res_keys,
        res_vals,
    )?;

    let pos: usize = KIVI_SNAPSHOT_HEADER_BYTES;
    const dim: usize = cfg.head_dim;
    const stride: usize = cache.packed_stride;

    let h: usize = 0;
    while (h < cfg.num_kv_heads) : (h += 1) {
        let qt: usize = 0;
        while (qt < q_count) : (qt += 1) {
            const pk_off: usize = cache.packedOff(h, qt);
            let b: usize = 0;
            while (b < stride) : (b += 1) {
                cache.packed_keys[pk_off + b] = bytes[pos];
                pos += 1;
            }
            b = 0;
            while (b < stride) : (b += 1) {
                cache.packed_vals[pk_off + b] = bytes[pos];
                pos += 1;
            }
            cache.val_tok_params[cache.valParamOff(h, qt)] = kiviReadAffine(bytes, pos);
            pos += 8;
        }

        let g: usize = 0;
        while (g < g_count) : (g += 1) {
            const kg_off: usize = cache.keyGroupOff(h, g);
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                cache.key_group_params[kg_off + c] = kiviReadAffine(bytes, pos);
                pos += 8;
            }
        }

        let rt: usize = 0;
        while (rt < r_count) : (rt += 1) {
            const roff: usize = cache.resOff(h, rt);
            let c: usize = 0;
            while (c < dim) : (c += 1) {
                const kf: f32 = @bitCast(kiviReadLeU32(bytes, pos));
                cache.res_keys[roff + c] = kf;
                pos += 4;
            }
            c = 0;
            while (c < dim) : (c += 1) {
                const vf: f32 = @bitCast(kiviReadLeU32(bytes, pos));
                cache.res_vals[roff + c] = vf;
                pos += 4;
            }
        }
    }

    cache.quantized_count = q_count;
    cache.residual_count = r_count;
    cache.group_count = g_count;
    return cache;
}

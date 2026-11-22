// Origin: Pure-CP Stdlib (3D Multimodal Rotary Position Embedding — M-RoPE for Qwen2-VL / Qwen2.5-VL) | Created: 2026-11-23
// Pure CP zero-heap-allocation 3D Multimodal RoPE (M-RoPE) engine:
//   - Unified 3D positional coordinates `(pos_t, pos_h, pos_w)` for text, 2D images, and 3D videos
//   - Zero-alloc `MropePosBuilder` generating contiguous 3D position IDs with Qwen2-VL offset continuity
//   - Both contiguous section layout (`[T..., H..., W...]`) and interleaved round-robin layout
//   - Single-head, multi-head (GQA-compatible), and batched sequence in-place F32 rotation kernels
//   - Exact 1D RoPE equivalence when `pos_t == pos_h == pos_w` and strict L2 norm preservation

const std = @import("std");

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// Maximum supported attention head dimension (`256`, i.e. 128 rotary pairs).
pub const MROPE_MAX_HEAD_DIM: usize = 256;

/// Maximum supported sequence length for zero-alloc `MropePosBuilder` and sequence kernels.
pub const MROPE_MAX_SEQ_LEN: usize = 512;

const MROPE_MAX_HALF_DIM: usize = MROPE_MAX_HEAD_DIM / 2;

/// Validation and runtime errors for 3D M-RoPE operations.
pub err MropeError {
    InvalidHeadDim,
    InvalidSections,
    DimMismatch,
    SequenceOverflow,
}

// ============================================================================
// 2. Configuration & 3D Positional Coordinate Model
// ============================================================================

/// Configuration for 3D Multimodal RoPE (M-RoPE).
///
/// The `head_dim / 2` rotary frequency pairs are partitioned across temporal (`section_t`),
/// height (`section_h`), and width (`section_w`) axes such that:
/// `(section_t + section_h + section_w) * 2 == head_dim`.
pub struct MropeConfig {
    /// Attention head dimension (must be even, `2 <= head_dim <= MROPE_MAX_HEAD_DIM`).
    head_dim: usize = 128;
    /// Number of rotary frequency pairs assigned to the temporal axis (`pos_t`).
    section_t: usize = 16;
    /// Number of rotary frequency pairs assigned to the spatial height axis (`pos_h`).
    section_h: usize = 24;
    /// Number of rotary frequency pairs assigned to the spatial width axis (`pos_w`).
    section_w: usize = 24;
    /// RoPE base frequency theta (Qwen2-VL / Qwen2.5-VL default `1000000.0`).
    theta: f32 = 1000000.0;
    /// When `false`, uses contiguous `[T..., H..., W...]` layout;
    /// when `true`, distributes T/H/W frequency pairs via round-robin interleaving across the spectrum.
    interleaved: bool = false;

    /// Returns Qwen2-VL / Qwen2.5-VL default configuration (`head_dim = 128`, sections `[16, 24, 24]`,
    /// `theta = 1000000.0`, contiguous layout).
    static fn qwen2VlDefaults(): MropeConfig {
        return MropeConfig{
            head_dim: 128,
            section_t: 16,
            section_h: 24,
            section_w: 24,
            theta: 1000000.0,
            interleaved: false,
        };
    }

    /// Returns `true` if `head_dim`, `(section_t, section_h, section_w)`, and `theta` are valid.
    fn isValid(): bool {
        if (head_dim == 0 || head_dim > MROPE_MAX_HEAD_DIM || (head_dim & 1) != 0) return false;
        const half_dim: usize = head_dim >> 1;
        if (section_t > half_dim || section_h > half_dim || section_w > half_dim) return false;
        if (section_t + section_h > half_dim) return false;
        if (section_t + section_h + section_w != half_dim) return false;
        if (std.math.isNan(theta) || std.math.isInf(theta) || theta <= 1.0) return false;
        return true;
    }
}

/// 3D multimodal positional coordinates `(pos_t, pos_h, pos_w)` for a single token.
pub struct MropePos3d {
    pos_t: i32 = 0;
    pos_h: i32 = 0;
    pos_w: i32 = 0;

    /// Creates a 1D text token coordinate `(p, p, p)` which degenerates identically to standard 1D RoPE.
    static fn textPos(p: i32): MropePos3d {
        return MropePos3d{ pos_t: p, pos_h: p, pos_w: p };
    }

    /// Creates an explicit 3D vision/video token coordinate `(t, h, w)`.
    static fn visionPos(t: i32, h: i32, w: i32): MropePos3d {
        return MropePos3d{ pos_t: t, pos_h: h, pos_w: w };
    }
}

/// Modality segment kind tracked by `MropePosBuilder`.
pub enum MropeSegmentKind {
    text,
    image,
    video,
}

/// Zero-heap-allocation 3D multimodal position ID builder following Qwen2-VL / Qwen2.5-VL conventions.
///
/// - Text tokens advance `(pos_t, pos_h, pos_w)` identically as `(cursor + i, cursor + i, cursor + i)`.
/// - 2D image grids `(grid_h, grid_w)` assign `pos_t = base`, `pos_h = base + h`, `pos_w = base + w`,
///   and advance `cursor` by `max(grid_h, grid_w)` so subsequent tokens never overlap.
/// - 3D video grids `(grid_t, grid_h, grid_w)` assign `pos_t = base + t`, `pos_h = base + h`,
///   `pos_w = base + w`, and advance `cursor` by `max(grid_t, max(grid_h, grid_w))`.
pub struct MropePosBuilder {
    positions: [MROPE_MAX_SEQ_LEN]MropePos3d = [MropePos3d{}; MROPE_MAX_SEQ_LEN];
    kinds: [MROPE_MAX_SEQ_LEN]MropeSegmentKind = [MropeSegmentKind.text; MROPE_MAX_SEQ_LEN];
    count: usize = 0;
    cursor: i32 = 0;

    /// Initializes an empty `MropePosBuilder` starting at position `0`.
    static fn init(): MropePosBuilder {
        return MropePosBuilder{};
    }

    /// Appends `token_count` contiguous text tokens starting at `nextTextPos()`.
    fn appendTextTokens(*_, token_count: usize): void ! MropeError {
        if (token_count > MROPE_MAX_SEQ_LEN - count) return MropeError.SequenceOverflow;
        let i: usize = 0;
        while (i < token_count) : (i += 1) {
            const p: i32 = cursor + @as(i32, @intCast(i));
            positions[count] = MropePos3d.textPos(p);
            kinds[count] = MropeSegmentKind.text;
            count += 1;
        }
        cursor += @as(i32, @intCast(token_count));
    }

    /// Appends a 2D image patch grid `(grid_h, grid_w)` in row-major `(h, w)` order.
    fn appendImageGrid(*_, grid_h: usize, grid_w: usize): void ! MropeError {
        if (grid_h == 0 || grid_w == 0) return MropeError.DimMismatch;
        const remaining: usize = MROPE_MAX_SEQ_LEN - count;
        if (grid_h > remaining || grid_w > remaining / grid_h) return MropeError.SequenceOverflow;

        const base: i32 = cursor;
        let h: usize = 0;
        while (h < grid_h) : (h += 1) {
            let w: usize = 0;
            while (w < grid_w) : (w += 1) {
                positions[count] = MropePos3d.visionPos(
                    base,
                    base + @as(i32, @intCast(h)),
                    base + @as(i32, @intCast(w)),
                );
                kinds[count] = MropeSegmentKind.image;
                count += 1;
            }
        }
        const span: usize = if (grid_h > grid_w) grid_h else grid_w;
        cursor += @as(i32, @intCast(span));
    }

    /// Appends a 3D video patch grid `(grid_t, grid_h, grid_w)` in `(t, h, w)` order.
    fn appendVideoGrid(*_, grid_t: usize, grid_h: usize, grid_w: usize): void ! MropeError {
        if (grid_t == 0 || grid_h == 0 || grid_w == 0) return MropeError.DimMismatch;
        const remaining: usize = MROPE_MAX_SEQ_LEN - count;
        if (grid_t > remaining || grid_h > remaining / grid_t) return MropeError.SequenceOverflow;
        const th: usize = grid_t * grid_h;
        if (grid_w > remaining / th) return MropeError.SequenceOverflow;

        const base: i32 = cursor;
        let t: usize = 0;
        while (t < grid_t) : (t += 1) {
            let h: usize = 0;
            while (h < grid_h) : (h += 1) {
                let w: usize = 0;
                while (w < grid_w) : (w += 1) {
                    positions[count] = MropePos3d.visionPos(
                        base + @as(i32, @intCast(t)),
                        base + @as(i32, @intCast(h)),
                        base + @as(i32, @intCast(w)),
                    );
                    kinds[count] = MropeSegmentKind.video;
                    count += 1;
                }
            }
        }
        const max_hw: usize = if (grid_h > grid_w) grid_h else grid_w;
        const span: usize = if (grid_t > max_hw) grid_t else max_hw;
        cursor += @as(i32, @intCast(span));
    }

    /// Returns the total number of generated token positions.
    fn len(): usize {
        return count;
    }

    /// Returns the 3D position at `idx`, or `null` if out of bounds.
    fn getPos(idx: usize): MropePos3d? {
        if (idx >= count) return null;
        return positions[idx];
    }

    /// Returns the modality segment kind at `idx`, or `null` if out of bounds.
    fn getKind(idx: usize): MropeSegmentKind? {
        if (idx >= count) return null;
        return kinds[idx];
    }

    /// Returns the next contiguous 1D text position ID following all appended segments.
    fn nextTextPos(): i32 {
        return cursor;
    }

    /// Returns a read-only slice view of all generated 3D positions `positions[0..len()]`.
    fn positionsSlice(): []const MropePos3d {
        return positions[0..count];
    }
}

// ============================================================================
// 3. Internal Validation & Rotary Table Helpers
// ============================================================================

fn validateConfig(cfg: MropeConfig): void ! MropeError {
    if (cfg.head_dim == 0 || cfg.head_dim > MROPE_MAX_HEAD_DIM || (cfg.head_dim & 1) != 0) {
        return MropeError.InvalidHeadDim;
    }
    if (std.math.isNan(cfg.theta) || std.math.isInf(cfg.theta) || cfg.theta <= 1.0) {
        return MropeError.InvalidHeadDim;
    }
    const half_dim: usize = cfg.head_dim >> 1;
    if (cfg.section_t > half_dim || cfg.section_h > half_dim || cfg.section_w > half_dim) {
        return MropeError.InvalidSections;
    }
    if (cfg.section_t + cfg.section_h > half_dim) {
        return MropeError.InvalidSections;
    }
    if (cfg.section_t + cfg.section_h + cfg.section_w != half_dim) {
        return MropeError.InvalidSections;
    }
}

/// Fills `cos_buf[0..half_dim]` and `sin_buf[0..half_dim]` for a given 3D position `pos`.
fn fillCosSinForPos(
    pos: MropePos3d,
    inv_freqs: []const f32,
    cfg: MropeConfig,
    cos_buf: *[MROPE_MAX_HALF_DIM]f64,
    sin_buf: *[MROPE_MAX_HALF_DIM]f64,
) {
    const half_dim: usize = cfg.head_dim >> 1;
    if (!cfg.interleaved) {
        const end_t: usize = cfg.section_t;
        const end_h: usize = cfg.section_t + cfg.section_h;
        let i: usize = 0;
        while (i < half_dim) : (i += 1) {
            const coord: i32 = if (i < end_t) pos.pos_t else if (i < end_h) pos.pos_h else pos.pos_w;
            const angle: f64 = (coord as f64) * (inv_freqs[i] as f64);
            cos_buf[i] = @cos(angle);
            sin_buf[i] = @sin(angle);
        }
        return;
    }

    // Interleaved layout: round-robin T -> H -> W across rotary frequency pairs
    // while strictly honoring the exact counts `(section_t, section_h, section_w)`.
    let rem_t: usize = cfg.section_t;
    let rem_h: usize = cfg.section_h;
    let rem_w: usize = cfg.section_w;
    let axis: usize = 0;
    let i: usize = 0;
    while (i < half_dim) : (i += 1) {
        let coord: i32 = pos.pos_w;
        let picked: bool = false;
        let tries: usize = 0;
        while (tries < 3 && !picked) : (tries += 1) {
            if (axis == 0 && rem_t > 0) {
                coord = pos.pos_t;
                rem_t -= 1;
                picked = true;
            } else if (axis == 1 && rem_h > 0) {
                coord = pos.pos_h;
                rem_h -= 1;
                picked = true;
            } else if (axis == 2 && rem_w > 0) {
                coord = pos.pos_w;
                rem_w -= 1;
                picked = true;
            }
            axis = (axis + 1) % 3;
        }
        const angle: f64 = (coord as f64) * (inv_freqs[i] as f64);
        cos_buf[i] = @cos(angle);
        sin_buf[i] = @sin(angle);
    }
}

fn rotateHeadSlice(
    vec: *[]f32,
    offset: usize,
    half_dim: usize,
    cos_buf: *const [MROPE_MAX_HALF_DIM]f64,
    sin_buf: *const [MROPE_MAX_HALF_DIM]f64,
) {
    let i: usize = 0;
    while (i < half_dim) : (i += 1) {
        const idx0: usize = offset + i;
        const idx1: usize = offset + i + half_dim;
        const x0: f64 = vec[idx0] as f64;
        const x1: f64 = vec[idx1] as f64;
        const c: f64 = cos_buf[i];
        const s: f64 = sin_buf[i];
        vec[idx0] = (x0 * c - x1 * s) as f32;
        vec[idx1] = (x0 * s + x1 * c) as f32;
    }
}

// ============================================================================
// 4. Core M-RoPE Kernels
// ============================================================================

/// Precomputes `head_dim / 2` inverse rotary frequencies `theta^(-2i / head_dim)` into `out_inv_freqs`.
pub fn computeMropeInvFreqs(
    cfg: MropeConfig,
    out_inv_freqs: *[]f32,
): void ! MropeError {
    validateConfig(cfg)?;
    const half_dim: usize = cfg.head_dim >> 1;
    if (out_inv_freqs.len < half_dim) return MropeError.DimMismatch;

    const d_f64: f64 = cfg.head_dim as f64;
    const theta_f64: f64 = cfg.theta as f64;
    let i: usize = 0;
    while (i < half_dim) : (i += 1) {
        const exp_i: f64 = ((i << 1) as f64) / d_f64;
        const inv_f: f64 = 1.0 / std.math.pow(f64, theta_f64, exp_i);
        out_inv_freqs[i] = inv_f as f32;
    }
}

/// Applies 3D M-RoPE half-split rotation in-place to a single head vector `head_vec[0..head_dim]`.
///
/// Strictly preserves L2 norm `||head_vec||_2` and degenerates identically to standard 1D RoPE
/// when `pos.pos_t == pos.pos_h == pos.pos_w`.
pub fn applyMropeHeadInPlaceF32(
    head_vec: *[]f32,
    pos: MropePos3d,
    inv_freqs: []const f32,
    cfg: MropeConfig,
): void ! MropeError {
    validateConfig(cfg)?;
    const half_dim: usize = cfg.head_dim >> 1;
    if (head_vec.len != cfg.head_dim || inv_freqs.len < half_dim) {
        return MropeError.DimMismatch;
    }

    let cos_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];
    let sin_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];
    fillCosSinForPos(pos, inv_freqs, cfg, *cos_buf, *sin_buf);
    rotateHeadSlice(head_vec, 0, half_dim, &cos_buf, &sin_buf);
}

/// Applies 3D M-RoPE rotation in-place to `num_heads` packed heads (`qk_flat.len == num_heads * head_dim`)
/// at the same 3D coordinate `pos`. Suitable for both Q and K heads under MHA / GQA.
pub fn applyMropeMultiHeadInPlaceF32(
    qk_flat: *[]f32,
    num_heads: usize,
    pos: MropePos3d,
    inv_freqs: []const f32,
    cfg: MropeConfig,
): void ! MropeError {
    validateConfig(cfg)?;
    const half_dim: usize = cfg.head_dim >> 1;
    if (num_heads == 0 || inv_freqs.len < half_dim) {
        return MropeError.DimMismatch;
    }
    if (num_heads > std.math.maxInt(usize) / cfg.head_dim) {
        return MropeError.DimMismatch;
    }
    const expected_len: usize = num_heads * cfg.head_dim;
    if (qk_flat.len != expected_len) {
        return MropeError.DimMismatch;
    }

    let cos_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];
    let sin_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];
    fillCosSinForPos(pos, inv_freqs, cfg, *cos_buf, *sin_buf);

    let h: usize = 0;
    while (h < num_heads) : (h += 1) {
        rotateHeadSlice(qk_flat, h * cfg.head_dim, half_dim, &cos_buf, &sin_buf);
    }
}

/// Applies 3D M-RoPE rotation in-place to a contiguous `[seq_len, num_heads, head_dim]` tensor
/// using per-token 3D coordinates `positions[0..seq_len]`.
pub fn applyMropeSequenceInPlaceF32(
    seq_flat: *[]f32,
    seq_len: usize,
    num_heads: usize,
    positions: []const MropePos3d,
    inv_freqs: []const f32,
    cfg: MropeConfig,
): void ! MropeError {
    validateConfig(cfg)?;
    if (seq_len > MROPE_MAX_SEQ_LEN) return MropeError.SequenceOverflow;

    const half_dim: usize = cfg.head_dim >> 1;
    if (num_heads == 0 || positions.len != seq_len || inv_freqs.len < half_dim) {
        return MropeError.DimMismatch;
    }
    if (seq_len == 0) {
        if (seq_flat.len != 0) return MropeError.DimMismatch;
        return;
    }
    if (num_heads > std.math.maxInt(usize) / cfg.head_dim) {
        return MropeError.DimMismatch;
    }
    const token_stride: usize = num_heads * cfg.head_dim;
    if (seq_len > std.math.maxInt(usize) / token_stride) {
        return MropeError.DimMismatch;
    }
    if (seq_flat.len != seq_len * token_stride) {
        return MropeError.DimMismatch;
    }

    let cos_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];
    let sin_buf: [MROPE_MAX_HALF_DIM]f64 = [0.0; MROPE_MAX_HALF_DIM];

    let t: usize = 0;
    while (t < seq_len) : (t += 1) {
        fillCosSinForPos(positions[t], inv_freqs, cfg, *cos_buf, *sin_buf);
        const token_base: usize = t * token_stride;
        let h: usize = 0;
        while (h < num_heads) : (h += 1) {
            rotateHeadSlice(seq_flat, token_base + h * cfg.head_dim, half_dim, &cos_buf, &sin_buf);
        }
    }
}

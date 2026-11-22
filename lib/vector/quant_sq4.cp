// Origin: Pure-CP Stdlib (AI Vector Search 4-bit Asymmetric Scalar Quantization SQ4) | Created: 2026-11-22
// Pure CP zero-heap-allocation 4-bit asymmetric scalar quantization (SQ4) providing 8x memory
// compression for f32 embeddings, nibble packing, direct @fma asymmetric L2 squared distance
// without temporary decoding buffers, and batch Top-K approximate retrieval.

const std = @import("std");

/// Per-dimension SQ4 quantization codebook referencing caller-owned `mins` and `scales` slices.
pub struct Sq4Codebook {
    mins: []f32;
    scales: []f32;
    dim: usize = 0;

    /// Initializes an `Sq4Codebook` view over caller-owned `mins` and `scales` slices of length `>= dim`.
    /// Returns `null` if `dim == 0` or either slice is shorter than `dim`.
    static fn init(mins: *[]f32, scales: *[]f32, dim: usize): Sq4Codebook? {
        if (dim == 0 || mins.len < dim || scales.len < dim) return null;
        return Sq4Codebook{
            mins: mins[0..dim],
            scales: scales[0..dim],
            dim: dim,
        };
    }
}

// ============================================================================
// Internal Numeric, Quantization & Top-K Heap Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn isValidCodebook(mins: []const f32, scales: []const f32, dim: usize): bool {
    if (dim == 0 || mins.len < dim || scales.len < dim) return false;
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        if (!isFiniteF32(mins[d]) || !isFiniteF32(scales[d]) || scales[d] < 0.0) {
            return false;
        }
    }
    return true;
}

fn quantizeNibble(v: f32, min_v: f32, scale: f32): u8 {
    if (scale <= 0.0) return 0;
    let norm: f32 = (v - min_v) / scale;
    if (norm <= 0.0) return 0;
    if (norm >= 15.0) return 15;
    const rounded: f32 = @floor(norm + 0.5);
    if (rounded <= 0.0) return 0;
    if (rounded >= 15.0) return 15;
    return @intFromFloat(rounded);
}

/// Returns `true` if `(a_dist, a_id)` is strictly worse (farther) than `(b_dist, b_id)`.
fn sq4Worse(a_dist: f32, a_id: usize, b_dist: f32, b_id: usize): bool {
    if (a_dist > b_dist) return true;
    if (a_dist < b_dist) return false;
    return a_id > b_id;
}

fn sq4HeapSiftUp(ids: *[]usize, dists: *[]f32, idx_in: usize) {
    let idx: usize = idx_in;
    while (idx > 0) {
        const parent: usize = (idx - 1) / 2;
        if (sq4Worse(dists[idx], ids[idx], dists[parent], ids[parent])) {
            const tmp_id: usize = ids[parent];
            const tmp_d: f32 = dists[parent];
            ids[parent] = ids[idx];
            dists[parent] = dists[idx];
            ids[idx] = tmp_id;
            dists[idx] = tmp_d;
            idx = parent;
        } else {
            break;
        }
    }
}

fn sq4HeapSiftDown(ids: *[]usize, dists: *[]f32, count: usize) {
    let root: usize = 0;
    while (true) {
        const left: usize = root * 2 + 1;
        if (left >= count) break;
        let worst: usize = root;
        if (sq4Worse(dists[left], ids[left], dists[worst], ids[worst])) {
            worst = left;
        }
        const right: usize = left + 1;
        if (right < count && sq4Worse(dists[right], ids[right], dists[worst], ids[worst])) {
            worst = right;
        }
        if (worst == root) break;
        const tmp_id: usize = ids[root];
        const tmp_d: f32 = dists[root];
        ids[root] = ids[worst];
        dists[root] = dists[worst];
        ids[worst] = tmp_id;
        dists[worst] = tmp_d;
        root = worst;
    }
}

// ============================================================================
// Public SQ4 4-Bit Asymmetric Scalar Quantization API
// ============================================================================

/// Returns the number of packed bytes required to store a `dim`-dimensional SQ4 vector
/// (`ceil(dim / 2)`, two 4-bit nibbles per byte).
pub fn sq4PackedBytes(dim: usize): usize {
    return (dim >> 1) + (dim & 1);
}

/// Fits per-dimension `[min, scale]` quantization parameters (`scale = (max - min) / 15.0`)
/// over a flat row-major `[n_docs, dim]` `f32` corpus.
/// Constant dimensions (`min == max`) safely receive `scale = 0.0` with zero-division protection.
/// Returns `false` on empty/mismatched/overflowing inputs or non-finite corpus values.
pub fn sq4FitCodebook(
    corpus_flat: []const f32,
    n_docs: usize,
    dim: usize,
    out_mins: *[]f32,
    out_scales: *[]f32,
): bool {
    if (dim == 0 || n_docs == 0) return false;
    if (out_mins.len < dim || out_scales.len < dim) return false;
    if (n_docs > std.math.maxInt(usize) / dim) return false;
    const total: usize = n_docs * dim;
    if (corpus_flat.len < total) return false;

    // Initialize per-dim min/max from document 0.
    let rem: []const f32 = corpus_flat[0..total];
    const [first_vec, rest0] = rem.split(dim);
    rem = rest0;

    let d0: usize = 0;
    while (d0 < dim) : (d0 += 1) {
        const v: f32 = first_vec[d0];
        if (!isFiniteF32(v)) return false;
        out_mins[d0] = v;
        out_scales[d0] = v; // Temporarily holds per-dim max.
    }

    let doc: usize = 1;
    while (doc < n_docs) : (doc += 1) {
        const [vec, next_rem] = rem.split(dim);
        rem = next_rem;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            const v: f32 = vec[d];
            if (!isFiniteF32(v)) return false;
            if (v < out_mins[d]) out_mins[d] = v;
            if (v > out_scales[d]) out_scales[d] = v;
        }
    }

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        const span: f32 = out_scales[d] - out_mins[d];
        if (!isFiniteF32(span) || span <= 0.0) {
            out_scales[d] = 0.0;
        } else {
            out_scales[d] = span / 15.0;
        }
    }

    return true;
}

/// Encodes a `dim`-dimensional `f32` vector into `sq4PackedBytes(dim)` bytes (low nibble = even dim,
/// high nibble = odd dim, each in `[0..15]`).
/// Returns `false` on empty/mismatched/non-finite inputs or undersized `out_packed`.
pub fn sq4EncodeVector(
    vec: []const f32,
    mins: []const f32,
    scales: []const f32,
    out_packed: *[]u8,
): bool {
    const dim: usize = vec.len;
    if (!isValidCodebook(mins, scales, dim)) return false;
    if (!isFiniteSliceF32(vec)) return false;
    const packed_len: usize = sq4PackedBytes(dim);
    if (out_packed.len < packed_len) return false;

    const pairs: usize = dim >> 1;
    let b: usize = 0;
    while (b < pairs) : (b += 1) {
        const idx0: usize = b << 1;
        const idx1: usize = idx0 + 1;
        const q0: u8 = quantizeNibble(vec[idx0], mins[idx0], scales[idx0]);
        const q1: u8 = quantizeNibble(vec[idx1], mins[idx1], scales[idx1]);
        out_packed[b] = q0 | (q1 << 4);
    }

    if ((dim & 1) == 1) {
        const last: usize = dim - 1;
        out_packed[pairs] = quantizeNibble(vec[last], mins[last], scales[last]);
    }

    return true;
}

/// Decodes a packed SQ4 byte slice back into `f32` values using hardware `@fma(q, scale, min)`.
/// Returns `false` on empty/mismatched/non-finite codebook or undersized buffers.
pub fn sq4DecodeVector(
    packed_code: []const u8,
    mins: []const f32,
    scales: []const f32,
    out_vec: *[]f32,
): bool {
    const dim: usize = mins.len;
    if (!isValidCodebook(mins, scales, dim)) return false;
    if (scales.len != dim) return false;
    if (out_vec.len < dim) return false;
    if (packed_code.len < sq4PackedBytes(dim)) return false;

    const pairs: usize = dim >> 1;
    let b: usize = 0;
    while (b < pairs) : (b += 1) {
        const byte: u8 = packed_code[b];
        const idx0: usize = b << 1;
        const idx1: usize = idx0 + 1;
        const q0: f32 = (byte & 0x0F) as f32;
        const q1: f32 = (byte >> 4) as f32;
        out_vec[idx0] = @fma(q0, scales[idx0], mins[idx0]);
        out_vec[idx1] = @fma(q1, scales[idx1], mins[idx1]);
    }

    if ((dim & 1) == 1) {
        const last: usize = dim - 1;
        const q_last: f32 = (packed_code[pairs] & 0x0F) as f32;
        out_vec[last] = @fma(q_last, scales[last], mins[last]);
    }

    return true;
}

/// Computes the asymmetric squared Euclidean (L2) distance directly between a full-precision
/// `query` vector and a 4-bit SQ4 `packed_code` without allocating or writing a decoded vector,
/// using hardware `@fma` for both inline reconstruction and squared-error accumulation.
/// Returns `null` on empty/mismatched/non-finite inputs or undersized `packed_code`.
pub fn sq4AsymmetricL2Sq(
    query: []const f32,
    packed_code: []const u8,
    mins: []const f32,
    scales: []const f32,
): f32? {
    const dim: usize = query.len;
    if (!isValidCodebook(mins, scales, dim)) return null;
    if (!isFiniteSliceF32(query)) return null;
    if (packed_code.len < sq4PackedBytes(dim)) return null;

    let acc: f32 = 0.0;
    const pairs: usize = dim >> 1;
    let b: usize = 0;
    while (b < pairs) : (b += 1) {
        const byte: u8 = packed_code[b];
        const idx0: usize = b << 1;
        const idx1: usize = idx0 + 1;

        const r0: f32 = @fma((byte & 0x0F) as f32, scales[idx0], mins[idx0]);
        const d0: f32 = query[idx0] - r0;
        acc = @fma(d0, d0, acc);

        const r1: f32 = @fma((byte >> 4) as f32, scales[idx1], mins[idx1]);
        const d1: f32 = query[idx1] - r1;
        acc = @fma(d1, d1, acc);
    }

    if ((dim & 1) == 1) {
        const last: usize = dim - 1;
        const rl: f32 = @fma((packed_code[pairs] & 0x0F) as f32, scales[last], mins[last]);
        const dl: f32 = query[last] - rl;
        acc = @fma(dl, dl, acc);
    }

    if (!isFiniteF32(acc)) return null;
    if (acc < 0.0) return 0.0;
    return acc;
}

/// Zero-heap-allocation Top-K approximate nearest neighbor search over an 8x-compressed
/// SQ4-encoded corpus (`packed_corpus` of length `>= n_docs * sq4PackedBytes(query.len)`).
///
/// Writes up to `min(k, n_docs)` document indices and asymmetric squared L2 distances into
/// `out_ids` and `out_dists` sorted ascending by `(dist_sq, doc_id)`.
/// Returns `null` on invalid dimensions, non-finite inputs, or undersized buffers.
pub fn sq4BatchTopK(
    query: []const f32,
    packed_corpus: []const u8,
    n_docs: usize,
    mins: []const f32,
    scales: []const f32,
    k: usize,
    out_ids: *[]usize,
    out_dists: *[]f32,
): usize? {
    const dim: usize = query.len;
    if (!isValidCodebook(mins, scales, dim)) return null;
    if (!isFiniteSliceF32(query)) return null;
    if (out_ids.len < k || out_dists.len < k) return null;
    if (n_docs == 0 || k == 0) return 0;

    const stride: usize = sq4PackedBytes(dim);
    if (n_docs > std.math.maxInt(usize) / stride) return null;
    const total_bytes: usize = n_docs * stride;
    if (packed_corpus.len < total_bytes) return null;

    const eff_k: usize = if (k < n_docs) k else n_docs;
    let count: usize = 0;
    let rem: []const u8 = packed_corpus[0..total_bytes];

    let doc: usize = 0;
    while (doc < n_docs) : (doc += 1) {
        const [code, next_rem] = rem.split(stride);
        rem = next_rem;

        if (dist_sq := sq4AsymmetricL2Sq(query, code, mins, scales)) {
            if (count < eff_k) {
                out_ids[count] = doc;
                out_dists[count] = dist_sq;
                sq4HeapSiftUp(out_ids, out_dists, count);
                count += 1;
            } else if (sq4Worse(out_dists[0], out_ids[0], dist_sq, doc)) {
                out_ids[0] = doc;
                out_dists[0] = dist_sq;
                sq4HeapSiftDown(out_ids, out_dists, count);
            }
        } else {
            return null;
        }
    }

    // Extract max-heap in reverse into ascending `(dist_sq, doc_id)` order.
    let end: usize = count;
    while (end > 1) {
        end -= 1;
        const tmp_id: usize = out_ids[0];
        const tmp_d: f32 = out_dists[0];
        out_ids[0] = out_ids[end];
        out_dists[0] = out_dists[end];
        out_ids[end] = tmp_id;
        out_dists[end] = tmp_d;
        sq4HeapSiftDown(out_ids, out_dists, end);
    }

    return count;
}

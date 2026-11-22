// Origin: Pure-CP Stdlib (Unbounded External-Buffer Multi-Layer HNSW Index & CRC32 Binary Snapshot Engine) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice HNSW ANN retrieval & persistence engine:
//   - Caller-owned external buffers (`vecs`, `nodes`, `l0_neighbors`, `upper_neighbors`, `visited_marks`)
//     supporting arbitrary `dim` and thousands-to-hundreds-of-thousands of vectors without static toy limits
//   - CP native SIMD `@l2sq` and `@dot` distance kernels (`l2_sq | cosine | neg_dot`) with cached `inv_norm`
//   - Full Malkov & Yashunin (2018) multi-layer HNSW: geometric level sampling, greedy upper descent,
//     beam `ef` search with O(1) stamp-based visited tracking, and Algorithm 4 `SELECT-NEIGHBORS-HEURISTIC`
//     diversity-preserving neighbor selection with bidirectional pruning
//   - Soft-deletion tombstones (`markDeleted` / `isDeleted`), online `computeRecallAtK` ground-truth calibrator,
//     and full IEEE CRC32-verified binary snapshot `serializeSnapshot` / `deserializeSnapshot` for instant reload

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set, Configuration & External Buffer Sizing Helpers
// ============================================================================

/// Little-endian magic tag `"HNSW"` (`0x57534E48`) for binary snapshot headers.
pub const HNSW_FLAT_MAGIC: u32 = 0x57534E48;

/// Binary snapshot wire format version.
pub const HNSW_FLAT_VERSION: u32 = 1;

/// Sentinel node index indicating an empty entry point or invalid neighbor slot.
pub const HNSW_FLAT_INVALID_ID: u32 = 0xFFFF_FFFF;

/// Maximum hierarchy levels supported per node (`0..=15`).
pub const HNSW_FLAT_MAX_LEVELS: usize = 16;

const HNSW_FLAT_MAX_EF: usize = 256;
const HNSW_FLAT_MAX_PRUNE_CANDS: usize = 140;
const SNAPSHOT_HEADER_BYTES: usize = 64;
const SNAPSHOT_NODE_BYTES: usize = 36;
const SNAPSHOT_CRC_BYTES: usize = 4;

/// Error set returned by HNSW index construction, query, and snapshot operations.
pub err HnswFlatError {
    InvalidConfig,
    DimMismatch,
    CapacityFull,
    BufferTooSmall,
    InvalidSnapshot,
    ChecksumMismatch,
    NodeNotFound,
}

/// Distance metric used by `HnswFlatIndex` (smaller distance = closer neighbor).
pub enum HnswFlatMetric {
    l2_sq,
    cosine,
    neg_dot,
}

/// Configuration for `HnswFlatIndex`.
pub struct HnswFlatConfig {
    dim: usize = 0;
    max_nodes: usize = 0;
    m: usize = 16;
    m_max0: usize = 32;
    ef_construction: usize = 64;
    metric: HnswFlatMetric = HnswFlatMetric.l2_sq;
    seed: u64 = 0x9E3779B97F4A7C15;

    /// Returns `true` iff all structural parameters are valid and consistent.
    fn isValid(): bool {
        return dim > 0 &&
            max_nodes > 0 &&
            m >= 2 &&
            m <= 64 &&
            m_max0 >= m &&
            m_max0 <= 128 &&
            ef_construction >= m;
    }
}

/// Returns the minimum `vecs.len` (`f32` elements) required for `cfg`.
pub fn hnswVecBufLen(cfg: HnswFlatConfig): usize {
    return cfg.max_nodes * cfg.dim;
}

/// Returns the minimum `l0_neighbors.len` (`u32` elements) required for `cfg`.
pub fn hnswL0AdjBufLen(cfg: HnswFlatConfig): usize {
    return cfg.max_nodes * cfg.m_max0;
}

/// Returns the recommended `upper_neighbors.len` (`u32` elements) required for `cfg`.
pub fn hnswUpperAdjBufLen(cfg: HnswFlatConfig): usize {
    return cfg.max_nodes * cfg.m * 2;
}

/// Returns an upper bound on snapshot byte size for `node_count` vectors under `cfg`.
pub fn hnswSnapshotMaxBytes(cfg: HnswFlatConfig, node_count: usize): usize {
    const n: usize = if (node_count > cfg.max_nodes) cfg.max_nodes else node_count;
    const node_meta: usize = n * SNAPSHOT_NODE_BYTES;
    const vec_bytes: usize = n * cfg.dim * 4;
    const l0_bytes: usize = n * cfg.m_max0 * 4;
    const upper_bytes: usize = hnswUpperAdjBufLen(cfg) * 4;
    return SNAPSHOT_HEADER_BYTES + node_meta + vec_bytes + l0_bytes + upper_bytes + SNAPSHOT_CRC_BYTES;
}

// ============================================================================
// 2. Graph Node, Search Hit & Telemetry Models
// ============================================================================

/// Per-node metadata stored in caller's `nodes: []HnswFlatNode` slice.
pub struct HnswFlatNode {
    doc_id: u64 = 0;
    level: u8 = 0;
    deleted: bool = false;
    l0_deg: u16 = 0;
    upper_offset: u32 = 0;
    upper_deg: [HNSW_FLAT_MAX_LEVELS]u8 = [0; HNSW_FLAT_MAX_LEVELS];
    inv_norm: f32 = 1.0;
}

/// Single approximate or exact nearest-neighbor search hit.
pub struct HnswFlatHit {
    doc_id: u64 = 0;
    node_idx: u32 = 0;
    distance: f32 = 0.0;
}

/// Aggregate graph topology and liveness telemetry.
pub struct HnswFlatStats {
    node_count: usize = 0;
    live_count: usize = 0;
    deleted_count: usize = 0;
    max_level: usize = 0;
    entry_point: u32 = HNSW_FLAT_INVALID_ID;
    total_l0_edges: usize = 0;
    total_upper_edges: usize = 0;
}

// ============================================================================
// 3. Internal Math, PRNG, Binary Encoding & IEEE CRC32 Helpers
// ============================================================================

fn splitmix64Next(state: *u64): u64 {
    state.* +%= 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn uniformOpen01(state: *u64): f64 {
    const bits: u64 = splitmix64Next(state) >> 11;
    let u: f64 = (@as(f64, @floatFromInt(bits)) + 0.5) / 9007199254740992.0;
    if (u <= 1.0e-12) u = 1.0e-12;
    if (u >= 1.0) u = 1.0 - 1.0e-12;
    return u;
}

fn computeInvNormF32(v: []const f32): f32 {
    const n2: f32 = @dot(v, v);
    if (n2 <= 1.0e-20 || std.math.isNan(n2) || std.math.isInf(n2)) {
        return 0.0;
    }
    return (1.0 as f32) / std.math.sqrt(n2);
}

fn metricToU32(m: HnswFlatMetric): u32 {
    if (m == HnswFlatMetric.l2_sq) return 0;
    if (m == HnswFlatMetric.cosine) return 1;
    return 2;
}

fn metricFromU32(tag: u32): HnswFlatMetric? {
    if (tag == 0) return HnswFlatMetric.l2_sq;
    if (tag == 1) return HnswFlatMetric.cosine;
    if (tag == 2) return HnswFlatMetric.neg_dot;
    return null;
}

fn crc32Ieee(bytes: []const u8): u32 {
    let crc: u32 = 0xFFFF_FFFF;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        crc ^= @as(u32, bytes[i]);
        let b: usize = 0;
        while (b < 8) : (b += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
    }
    return ~crc;
}

fn writeU32Le(dst: *[]u8, off: usize, v: u32) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
    dst[off + 2] = @as(u8, @intCast((v >> 16) & 0xFF));
    dst[off + 3] = @as(u8, @intCast((v >> 24) & 0xFF));
}

fn writeU16Le(dst: *[]u8, off: usize, v: u16) {
    dst[off + 0] = @as(u8, @intCast(v & 0xFF));
    dst[off + 1] = @as(u8, @intCast((v >> 8) & 0xFF));
}

fn writeU64Le(dst: *[]u8, off: usize, v: u64) {
    writeU32Le(dst, off, @as(u32, @intCast(v & 0xFFFF_FFFF)));
    writeU32Le(dst, off + 4, @as(u32, @intCast((v >> 32) & 0xFFFF_FFFF)));
}

fn readU32Le(src: []const u8, off: usize): u32 {
    return @as(u32, src[off + 0]) |
        (@as(u32, src[off + 1]) << 8) |
        (@as(u32, src[off + 2]) << 16) |
        (@as(u32, src[off + 3]) << 24);
}

fn readU16Le(src: []const u8, off: usize): u16 {
    return @as(u16, src[off + 0]) | (@as(u16, src[off + 1]) << 8);
}

fn readU64Le(src: []const u8, off: usize): u64 {
    const lo: u64 = @as(u64, readU32Le(src, off));
    const hi: u64 = @as(u64, readU32Le(src, off + 4));
    return lo | (hi << 32);
}

fn f32ToBits(v: f32): u32 {
    return @as(u32, @bitCast(v));
}

fn bitsToF32(b: u32): f32 {
    return @as(f32, @bitCast(b));
}

/// Inserts `(node_id, dist)` into ascending-distance sorted array bounded by `max_len`.
/// Returns updated length.
fn sortedInsertBounded(
    ids: *[]u32,
    dists: *[]f32,
    cur_len: usize,
    max_len: usize,
    node_id: u32,
    dist: f32,
): usize {
    if (max_len == 0) return 0;
    if (cur_len >= max_len && dist >= dists[cur_len - 1]) {
        return cur_len;
    }
    let pos: usize = cur_len;
    if (pos == max_len) {
        pos = max_len - 1;
    }
    while (pos > 0 && dist < dists[pos - 1]) {
        ids[pos] = ids[pos - 1];
        dists[pos] = dists[pos - 1];
        pos -= 1;
    }
    ids[pos] = node_id;
    dists[pos] = dist;
    return if (cur_len < max_len) cur_len + 1 else max_len;
}

// ============================================================================
// 4. Core Engine: HnswFlatIndex (Unbounded Caller-Slice View)
// ============================================================================

/// Multi-layer HNSW approximate nearest-neighbor index operating directly over caller-owned slices.
pub struct HnswFlatIndex {
    cfg: HnswFlatConfig = HnswFlatConfig{},
    vecs: []f32 = [],
    nodes: []HnswFlatNode = [],
    l0_neighbors: []u32 = [],
    upper_neighbors: []u32 = [],
    visited_marks: []u32 = [],

    node_count: usize = 0;
    deleted_count: usize = 0;
    max_level: usize = 0;
    entry_point: u32 = HNSW_FLAT_INVALID_ID;
    upper_used: usize = 0;
    visit_stamp: u32 = 0;
    rng_state: u64 = 0x9E3779B97F4A7C15;

    /// Initializes an empty `HnswFlatIndex` bound to caller-provided external slices.
    static fn init(
        cfg: HnswFlatConfig,
        vecs: *[]f32,
        nodes: *[]HnswFlatNode,
        l0_neighbors: *[]u32,
        upper_neighbors: *[]u32,
        visited_marks: *[]u32,
    ): HnswFlatIndex ! HnswFlatError {
        if (!cfg.isValid()) {
            return HnswFlatError.InvalidConfig;
        }
        if (vecs.len < hnswVecBufLen(cfg) ||
            nodes.len < cfg.max_nodes ||
            l0_neighbors.len < hnswL0AdjBufLen(cfg) ||
            upper_neighbors.len < cfg.m ||
            visited_marks.len < cfg.max_nodes)
        {
            return HnswFlatError.BufferTooSmall;
        }

        let i: usize = 0;
        while (i < cfg.max_nodes) : (i += 1) {
            visited_marks[i] = 0;
        }

        return HnswFlatIndex{
            cfg: cfg,
            vecs: vecs.*,
            nodes: nodes.*,
            l0_neighbors: l0_neighbors.*,
            upper_neighbors: upper_neighbors.*,
            visited_marks: visited_marks.*,
            node_count: 0,
            deleted_count: 0,
            max_level: 0,
            entry_point: HNSW_FLAT_INVALID_ID,
            upper_used: 0,
            visit_stamp: 0,
            rng_state: if (cfg.seed == 0) 0x9E3779B97F4A7C15 else cfg.seed,
        };
    }

    fn nextVisitStamp(*_): u32 {
        visit_stamp +%= 1;
        if (visit_stamp == 0) {
            let i: usize = 0;
            while (i < cfg.max_nodes) : (i += 1) {
                visited_marks[i] = 0;
            }
            visit_stamp = 1;
        }
        return visit_stamp;
    }

    fn vecSliceOf(node_idx: usize): []const f32 {
        const off: usize = node_idx * cfg.dim;
        return vecs[off..(off + cfg.dim)];
    }

    fn distQueryToNode(query: []const f32, q_inv_norm: f32, node_idx: usize): f32 {
        const v = vecSliceOf(node_idx);
        if (cfg.metric == HnswFlatMetric.l2_sq) {
            return @l2sq(query, v);
        }
        if (cfg.metric == HnswFlatMetric.neg_dot) {
            return -@dot(query, v);
        }
        const denom_inv: f32 = q_inv_norm * nodes[node_idx].inv_norm;
        if (denom_inv <= 0.0) {
            return 1.0;
        }
        return 1.0 - (@dot(query, v) * denom_inv);
    }

    fn distNodeToNode(a_idx: usize, b_idx: usize): f32 {
        const va = vecSliceOf(a_idx);
        const vb = vecSliceOf(b_idx);
        if (cfg.metric == HnswFlatMetric.l2_sq) {
            return @l2sq(va, vb);
        }
        if (cfg.metric == HnswFlatMetric.neg_dot) {
            return -@dot(va, vb);
        }
        const denom_inv: f32 = nodes[a_idx].inv_norm * nodes[b_idx].inv_norm;
        if (denom_inv <= 0.0) {
            return 1.0;
        }
        return 1.0 - (@dot(va, vb) * denom_inv);
    }

    fn neighborDegree(node_idx: usize, level: usize): usize {
        if (level == 0) {
            return @as(usize, nodes[node_idx].l0_deg);
        }
        return @as(usize, nodes[node_idx].upper_deg[level]);
    }

    fn neighborAt(node_idx: usize, level: usize, slot: usize): u32 {
        if (level == 0) {
            return l0_neighbors[node_idx * cfg.m_max0 + slot];
        }
        const base: usize = @as(usize, nodes[node_idx].upper_offset) + (level - 1) * cfg.m;
        return upper_neighbors[base + slot];
    }

    fn setNeighbors(*_, node_idx: usize, level: usize, src_ids: []const u32, count: usize) {
        if (level == 0) {
            const base: usize = node_idx * cfg.m_max0;
            let i: usize = 0;
            while (i < count) : (i += 1) {
                l0_neighbors[base + i] = src_ids[i];
            }
            nodes[node_idx].l0_deg = @as(u16, @intCast(count));
        } else {
            const base: usize = @as(usize, nodes[node_idx].upper_offset) + (level - 1) * cfg.m;
            let i: usize = 0;
            while (i < count) : (i += 1) {
                upper_neighbors[base + i] = src_ids[i];
            }
            nodes[node_idx].upper_deg[level] = @as(u8, @intCast(count));
        }
    }

    fn sampleLevel(*_): usize {
        const u: f64 = uniformOpen01(*rng_state);
        const m_f: f64 = @as(f64, @floatFromInt(cfg.m));
        const inv_ln_m: f64 = 1.0 / std.math.log(f64, std.math.e, m_f);
        const raw: f64 = -std.math.log(f64, std.math.e, u) * inv_ln_m;
        let lvl: usize = @as(usize, @intFromFloat(std.math.floor(raw)));
        if (lvl >= HNSW_FLAT_MAX_LEVELS) {
            lvl = HNSW_FLAT_MAX_LEVELS - 1;
        }
        // Gracefully cap level if caller's upper_neighbors pool is nearly full.
        while (lvl > 0 && upper_used + lvl * cfg.m > upper_neighbors.len) {
            lvl -= 1;
        }
        return lvl;
    }

    /// Greedy single-beam descent on `level` starting from `entry`.
    fn searchLayerGreedy(query: []const f32, q_inv_norm: f32, entry: u32, level: usize): u32 {
        let cur: u32 = entry;
        let cur_dist: f32 = distQueryToNode(query, q_inv_norm, @as(usize, cur));
        let improved: bool = true;
        while (improved) {
            improved = false;
            const deg: usize = neighborDegree(@as(usize, cur), level);
            let i: usize = 0;
            while (i < deg) : (i += 1) {
                const nb: u32 = neighborAt(@as(usize, cur), level, i);
                if (nb == HNSW_FLAT_INVALID_ID) continue;
                const d: f32 = distQueryToNode(query, q_inv_norm, @as(usize, nb));
                if (d < cur_dist) {
                    cur = nb;
                    cur_dist = d;
                    improved = true;
                }
            }
        }
        return cur;
    }

    /// Beam search on `level` returning up to `ef` closest candidates sorted ascending by distance.
    fn searchLayerEf(
        *_,
        query: []const f32,
        q_inv_norm: f32,
        entry: u32,
        ef_in: usize,
        level: usize,
        out_ids: *[]u32,
        out_dists: *[]f32,
    ): usize {
        let ef: usize = ef_in;
        if (ef == 0) ef = @as(usize, 1);
        if (ef > HNSW_FLAT_MAX_EF) ef = HNSW_FLAT_MAX_EF;
        if (ef > out_ids.len) ef = out_ids.len;

        let cand_ids: [HNSW_FLAT_MAX_EF]u32 = [0; HNSW_FLAT_MAX_EF];
        let cand_dists: [HNSW_FLAT_MAX_EF]f32 = [0.0; HNSW_FLAT_MAX_EF];
        let cand_ids_s: []u32 = cand_ids[0..];
        let cand_dists_s: []f32 = cand_dists[0..];

        const stamp: u32 = nextVisitStamp();
        const entry_dist: f32 = distQueryToNode(query, q_inv_norm, @as(usize, entry));
        visited_marks[@as(usize, entry)] = stamp;

        cand_ids[0] = entry;
        cand_dists[0] = entry_dist;
        let cand_len: usize = 1;

        out_ids[0] = entry;
        out_dists[0] = entry_dist;
        let res_len: usize = 1;

        while (cand_len > 0) {
            // Pop closest candidate (index 0) and shift left.
            const cur_id: u32 = cand_ids[0];
            const cur_dist: f32 = cand_dists[0];
            let s: usize = 1;
            while (s < cand_len) : (s += 1) {
                cand_ids[s - 1] = cand_ids[s];
                cand_dists[s - 1] = cand_dists[s];
            }
            cand_len -= 1;

            const worst_res_dist: f32 = out_dists[res_len - 1];
            if (cur_dist > worst_res_dist && res_len >= ef) {
                break;
            }

            const deg: usize = neighborDegree(@as(usize, cur_id), level);
            let i: usize = 0;
            while (i < deg) : (i += 1) {
                const nb: u32 = neighborAt(@as(usize, cur_id), level, i);
                if (nb == HNSW_FLAT_INVALID_ID) continue;
                const nb_idx: usize = @as(usize, nb);
                if (visited_marks[nb_idx] == stamp) continue;
                visited_marks[nb_idx] = stamp;

                const d: f32 = distQueryToNode(query, q_inv_norm, nb_idx);
                if (res_len < ef || d < out_dists[res_len - 1]) {
                    cand_len = sortedInsertBounded(*cand_ids_s, *cand_dists_s, cand_len, HNSW_FLAT_MAX_EF, nb, d);
                    res_len = sortedInsertBounded(out_ids, out_dists, res_len, ef, nb, d);
                }
            }
        }

        return res_len;
    }

    /// Malkov & Yashunin Algorithm 4 (`SELECT-NEIGHBORS-HEURISTIC` with `keepPrunedConnections = true`).
    /// Selects up to `max_conn` diverse neighbors from ascending-sorted `cand_ids`/`cand_dists`.
    fn selectNeighborsHeuristic(
        cand_ids: []const u32,
        cand_dists: []const f32,
        cand_len: usize,
        max_conn: usize,
        out_selected: *[]u32,
    ): usize {
        let sel_len: usize = 0;
        let disc_ids: [HNSW_FLAT_MAX_EF]u32 = [0; HNSW_FLAT_MAX_EF];
        let disc_len: usize = 0;

        let i: usize = 0;
        while (i < cand_len && sel_len < max_conn) : (i += 1) {
            const e_id: u32 = cand_ids[i];
            const e_dist: f32 = cand_dists[i];

            let good: bool = true;
            let r: usize = 0;
            while (r < sel_len) : (r += 1) {
                const d_er: f32 = distNodeToNode(@as(usize, e_id), @as(usize, out_selected[r]));
                if (d_er < e_dist) {
                    good = false;
                    break;
                }
            }

            if (good) {
                out_selected[sel_len] = e_id;
                sel_len += 1;
            } else if (disc_len < HNSW_FLAT_MAX_EF) {
                disc_ids[disc_len] = e_id;
                disc_len += 1;
            }
        }

        // Backfill from pruned connections in ascending distance order so degree targets stay healthy.
        let d_i: usize = 0;
        while (sel_len < max_conn && d_i < disc_len) : (d_i += 1) {
            out_selected[sel_len] = disc_ids[d_i];
            sel_len += 1;
        }

        return sel_len;
    }

    fn connectBidirectional(*_, new_id: u32, nb_id: u32, level: usize) {
        const nb_idx: usize = @as(usize, nb_id);
        const max_deg: usize = if (level == 0) cfg.m_max0 else cfg.m;
        const cur_deg: usize = neighborDegree(nb_idx, level);

        // Avoid duplicate edge.
        let dup_i: usize = 0;
        while (dup_i < cur_deg) : (dup_i += 1) {
            if (neighborAt(nb_idx, level, dup_i) == new_id) {
                return;
            }
        }

        if (cur_deg < max_deg) {
            if (level == 0) {
                l0_neighbors[nb_idx * cfg.m_max0 + cur_deg] = new_id;
                nodes[nb_idx].l0_deg = @as(u16, @intCast(cur_deg + 1));
            } else {
                const base: usize = @as(usize, nodes[nb_idx].upper_offset) + (level - 1) * cfg.m;
                upper_neighbors[base + cur_deg] = new_id;
                nodes[nb_idx].upper_deg[level] = @as(u8, @intCast(cur_deg + 1));
            }
            return;
        }

        // Overflow: gather existing neighbors + new_id, sort by distance to nb_id, and heuristic-prune.
        let c_ids: [HNSW_FLAT_MAX_PRUNE_CANDS]u32 = [0; HNSW_FLAT_MAX_PRUNE_CANDS];
        let c_dists: [HNSW_FLAT_MAX_PRUNE_CANDS]f32 = [0.0; HNSW_FLAT_MAX_PRUNE_CANDS];
        let c_ids_s: []u32 = c_ids[0..];
        let c_dists_s: []f32 = c_dists[0..];
        let c_len: usize = 0;

        let i: usize = 0;
        while (i < cur_deg) : (i += 1) {
            const existing: u32 = neighborAt(nb_idx, level, i);
            const d: f32 = distNodeToNode(nb_idx, @as(usize, existing));
            c_len = sortedInsertBounded(*c_ids_s, *c_dists_s, c_len, HNSW_FLAT_MAX_PRUNE_CANDS, existing, d);
        }
        const d_new: f32 = distNodeToNode(nb_idx, @as(usize, new_id));
        c_len = sortedInsertBounded(*c_ids_s, *c_dists_s, c_len, HNSW_FLAT_MAX_PRUNE_CANDS, new_id, d_new);

        let pruned: [HNSW_FLAT_MAX_PRUNE_CANDS]u32 = [0; HNSW_FLAT_MAX_PRUNE_CANDS];
        let pruned_s: []u32 = pruned[0..];
        const kept: usize = selectNeighborsHeuristic(c_ids[0..], c_dists[0..], c_len, max_deg, *pruned_s);
        setNeighbors(nb_idx, level, pruned[0..], kept);
    }

    /// Inserts a vector with `doc_id` into the HNSW graph and returns its internal `node_idx`.
    fn insert(*_, doc_id: u64, vec: []const f32): u32 ! HnswFlatError {
        if (vec.len != cfg.dim) {
            return HnswFlatError.DimMismatch;
        }
        if (node_count >= cfg.max_nodes) {
            return HnswFlatError.CapacityFull;
        }

        const new_idx: usize = node_count;
        const new_id: u32 = @as(u32, @intCast(new_idx));
        const lvl: usize = sampleLevel();

        // Store vector inline in external vecs pool.
        const v_off: usize = new_idx * cfg.dim;
        let k: usize = 0;
        while (k < cfg.dim) : (k += 1) {
            vecs[v_off + k] = vec[k];
        }

        const q_inv_norm: f32 = computeInvNormF32(vec);
        let node = HnswFlatNode{
            doc_id: doc_id,
            level: @as(u8, @intCast(lvl)),
            deleted: false,
            l0_deg: 0,
            upper_offset: @as(u32, @intCast(upper_used)),
            inv_norm: q_inv_norm,
        };
        if (lvl > 0) {
            upper_used += lvl * cfg.m;
        }
        nodes[new_idx] = node;
        node_count += 1;

        // First node becomes entry point.
        if (entry_point == HNSW_FLAT_INVALID_ID) {
            entry_point = new_id;
            max_level = lvl;
            return new_id;
        }

        // 1. Greedy descent on upper layers strictly above `lvl`.
        let curr_ep: u32 = entry_point;
        if (max_level > lvl) {
            let lc: usize = max_level;
            while (lc > lvl) {
                curr_ep = searchLayerGreedy(vec, q_inv_norm, curr_ep, lc);
                lc -= 1;
            }
        }

        // 2. From min(lvl, max_level) down to 0: beam search + Algorithm 4 Heuristic linking.
        const start_lvl: usize = if (lvl < max_level) lvl else max_level;
        let w_ids: [HNSW_FLAT_MAX_EF]u32 = [0; HNSW_FLAT_MAX_EF];
        let w_dists: [HNSW_FLAT_MAX_EF]f32 = [0.0; HNSW_FLAT_MAX_EF];
        let sel_ids: [HNSW_FLAT_MAX_PRUNE_CANDS]u32 = [0; HNSW_FLAT_MAX_PRUNE_CANDS];
        let w_ids_s: []u32 = w_ids[0..];
        let w_dists_s: []f32 = w_dists[0..];
        let sel_ids_s: []u32 = sel_ids[0..];

        let lc_plus1: usize = start_lvl + 1;
        while (lc_plus1 > 0) {
            lc_plus1 -= 1;
            const lc: usize = lc_plus1;

            const w_len: usize = searchLayerEf(
                vec,
                q_inv_norm,
                curr_ep,
                cfg.ef_construction,
                lc,
                *w_ids_s,
                *w_dists_s,
            );

            const max_conn: usize = if (lc == 0) cfg.m_max0 else cfg.m;
            const n_sel: usize = selectNeighborsHeuristic(w_ids[0..], w_dists[0..], w_len, max_conn, *sel_ids_s);
            setNeighbors(new_idx, lc, sel_ids[0..], n_sel);

            let s_i: usize = 0;
            while (s_i < n_sel) : (s_i += 1) {
                connectBidirectional(new_id, sel_ids[s_i], lc);
            }

            if (w_len > 0) {
                curr_ep = w_ids[0];
            }
        }

        if (lvl > max_level) {
            max_level = lvl;
            entry_point = new_id;
        }

        return new_id;
    }

    /// Searches for up to `top_k` live (non-deleted) nearest neighbors to `query_vec`.
    /// Writes hits in strictly ascending distance order into `out_hits` and returns hit count.
    fn search(
        *_,
        query_vec: []const f32,
        top_k: usize,
        ef_search: usize,
        out_hits: *[]HnswFlatHit,
    ): usize ! HnswFlatError {
        if (query_vec.len != cfg.dim) {
            return HnswFlatError.DimMismatch;
        }
        if (node_count == 0 || entry_point == HNSW_FLAT_INVALID_ID || top_k == 0 || out_hits.len == 0) {
            return 0;
        }

        const q_inv_norm: f32 = computeInvNormF32(query_vec);
        let curr_ep: u32 = entry_point;

        if (max_level > 0) {
            let lc: usize = max_level;
            while (lc > 0) {
                curr_ep = searchLayerGreedy(query_vec, q_inv_norm, curr_ep, lc);
                lc -= 1;
            }
        }

        const k_lim: usize = if (top_k < out_hits.len) top_k else out_hits.len;
        let eff_ef: usize = if (ef_search > k_lim) ef_search else k_lim;
        // Expand beam slightly when tombstones exist so enough live results survive filtering.
        if (deleted_count > 0 && eff_ef + deleted_count <= HNSW_FLAT_MAX_EF) {
            const boost: usize = if (deleted_count < 32) deleted_count else 32;
            eff_ef += boost;
        }

        let w_ids: [HNSW_FLAT_MAX_EF]u32 = [0; HNSW_FLAT_MAX_EF];
        let w_dists: [HNSW_FLAT_MAX_EF]f32 = [0.0; HNSW_FLAT_MAX_EF];
        let w_ids_s: []u32 = w_ids[0..];
        let w_dists_s: []f32 = w_dists[0..];

        const w_len: usize = searchLayerEf(query_vec, q_inv_norm, curr_ep, eff_ef, 0, *w_ids_s, *w_dists_s);

        let written: usize = 0;
        let i: usize = 0;
        while (i < w_len && written < k_lim) : (i += 1) {
            const nid: u32 = w_ids[i];
            const n_idx: usize = @as(usize, nid);
            if (nodes[n_idx].deleted) continue;
            out_hits[written] = HnswFlatHit{
                doc_id: nodes[n_idx].doc_id,
                node_idx: nid,
                distance: w_dists[i],
            };
            written += 1;
        }

        return written;
    }

    /// Marks the first live node matching `doc_id` as deleted (tombstone).
    fn markDeleted(*_, doc_id: u64): bool {
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            if (nodes[i].doc_id == doc_id && !nodes[i].deleted) {
                nodes[i].deleted = true;
                deleted_count += 1;
                return true;
            }
        }
        return false;
    }

    /// Returns `true` if `node_idx` is out of range or marked deleted.
    fn isDeleted(node_idx: u32): bool {
        const idx: usize = @as(usize, node_idx);
        if (idx >= node_count) return true;
        return nodes[idx].deleted;
    }

    /// Computes exact brute-force Top-K vs ANN `search` Recall@K in `[0.0, 1.0]` for online calibration.
    fn computeRecallAtK(
        *_,
        query_vec: []const f32,
        top_k: usize,
        ef_search: usize,
        gt_hits_buf: *[]HnswFlatHit,
        ann_hits_buf: *[]HnswFlatHit,
    ): f32 {
        if (query_vec.len != cfg.dim || top_k == 0 || node_count == 0) {
            return 0.0;
        }
        let k_lim: usize = top_k;
        if (k_lim > gt_hits_buf.len) k_lim = gt_hits_buf.len;
        if (k_lim > ann_hits_buf.len) k_lim = ann_hits_buf.len;
        if (k_lim == 0) return 0.0;

        // 1. Exact brute-force Top-K across all live nodes.
        const q_inv_norm: f32 = computeInvNormF32(query_vec);
        let gt_len: usize = 0;
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            if (nodes[i].deleted) continue;
            const d: f32 = distQueryToNode(query_vec, q_inv_norm, i);
            if (gt_len < k_lim || d < gt_hits_buf[gt_len - 1].distance) {
                let pos: usize = if (gt_len < k_lim) gt_len else k_lim - 1;
                while (pos > 0 && d < gt_hits_buf[pos - 1].distance) {
                    gt_hits_buf[pos] = gt_hits_buf[pos - 1];
                    pos -= 1;
                }
                gt_hits_buf[pos] = HnswFlatHit{
                    doc_id: nodes[i].doc_id,
                    node_idx: @as(u32, @intCast(i)),
                    distance: d,
                };
                if (gt_len < k_lim) gt_len += 1;
            }
        }
        if (gt_len == 0) return 1.0;

        // 2. ANN search Top-K.
        const ann_len: usize = search(query_vec, k_lim, ef_search, ann_hits_buf) catch 0;
        let matches: usize = 0;
        let a: usize = 0;
        while (a < ann_len) : (a += 1) {
            let g: usize = 0;
            while (g < gt_len) : (g += 1) {
                if (ann_hits_buf[a].node_idx == gt_hits_buf[g].node_idx) {
                    matches += 1;
                    break;
                }
            }
        }
        return (@as(f32, @floatFromInt(matches))) / (@as(f32, @floatFromInt(gt_len)));
    }

    /// Returns aggregate topology and liveness statistics for this index.
    fn stats(): HnswFlatStats {
        let l0_edges: usize = 0;
        let up_edges: usize = 0;
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            l0_edges += @as(usize, nodes[i].l0_deg);
            let lvl: usize = 1;
            while (lvl <= @as(usize, nodes[i].level)) : (lvl += 1) {
                up_edges += @as(usize, nodes[i].upper_deg[lvl]);
            }
        }
        return HnswFlatStats{
            node_count: node_count,
            live_count: node_count - deleted_count,
            deleted_count: deleted_count,
            max_level: max_level,
            entry_point: entry_point,
            total_l0_edges: l0_edges,
            total_upper_edges: up_edges,
        };
    }

    // ========================================================================
    // 5. Binary Snapshot Serialization & CRC32-Verified Hot Reload
    // ========================================================================

    /// Serializes the full graph state (config, node metadata, vectors, L0 & upper adjacency,
    /// PRNG state, and trailing IEEE CRC32) into `out_bytes`. Returns exact bytes written.
    fn serializeSnapshot(out_bytes: *[]u8): usize ! HnswFlatError {
        const node_bytes: usize = node_count * SNAPSHOT_NODE_BYTES;
        const vec_bytes: usize = node_count * cfg.dim * 4;
        const l0_bytes: usize = node_count * cfg.m_max0 * 4;
        const up_bytes: usize = upper_used * 4;
        const total_bytes: usize = SNAPSHOT_HEADER_BYTES + node_bytes + vec_bytes + l0_bytes + up_bytes + SNAPSHOT_CRC_BYTES;

        if (out_bytes.len < total_bytes) {
            return HnswFlatError.BufferTooSmall;
        }

        // 1. 64-byte little-endian header.
        writeU32Le(out_bytes, 0, HNSW_FLAT_MAGIC);
        writeU32Le(out_bytes, 4, HNSW_FLAT_VERSION);
        writeU32Le(out_bytes, 8, @as(u32, @intCast(cfg.dim)));
        writeU32Le(out_bytes, 12, @as(u32, @intCast(cfg.max_nodes)));
        writeU32Le(out_bytes, 16, @as(u32, @intCast(cfg.m)));
        writeU32Le(out_bytes, 20, @as(u32, @intCast(cfg.m_max0)));
        writeU32Le(out_bytes, 24, @as(u32, @intCast(cfg.ef_construction)));
        writeU32Le(out_bytes, 28, metricToU32(cfg.metric));
        writeU64Le(out_bytes, 32, cfg.seed);
        writeU32Le(out_bytes, 40, @as(u32, @intCast(node_count)));
        writeU32Le(out_bytes, 44, @as(u32, @intCast(deleted_count)));
        writeU32Le(out_bytes, 48, @as(u32, @intCast(max_level)));
        writeU32Le(out_bytes, 52, entry_point);
        writeU32Le(out_bytes, 56, @as(u32, @intCast(upper_used)));
        writeU32Le(out_bytes, 60, @as(u32, @intCast(rng_state & 0xFFFF_FFFF)));

        let off: usize = SNAPSHOT_HEADER_BYTES;

        // 2. Per-node metadata (36 bytes each).
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            const nd = nodes[i];
            writeU64Le(out_bytes, off, nd.doc_id);
            out_bytes[off + 8] = nd.level;
            out_bytes[off + 9] = if (nd.deleted) 1 else 0;
            writeU16Le(out_bytes, off + 10, nd.l0_deg);
            writeU32Le(out_bytes, off + 12, nd.upper_offset);
            writeU32Le(out_bytes, off + 16, f32ToBits(nd.inv_norm));
            let lv: usize = 0;
            while (lv < HNSW_FLAT_MAX_LEVELS) : (lv += 1) {
                out_bytes[off + 20 + lv] = nd.upper_deg[lv];
            }
            off += SNAPSHOT_NODE_BYTES;
        }

        // 3. Vector payload.
        const total_floats: usize = node_count * cfg.dim;
        let vf: usize = 0;
        while (vf < total_floats) : (vf += 1) {
            writeU32Le(out_bytes, off, f32ToBits(vecs[vf]));
            off += 4;
        }

        // 4. Level-0 adjacency payload.
        const total_l0: usize = node_count * cfg.m_max0;
        let l0_i: usize = 0;
        while (l0_i < total_l0) : (l0_i += 1) {
            writeU32Le(out_bytes, off, l0_neighbors[l0_i]);
            off += 4;
        }

        // 5. Upper-layer adjacency payload.
        let up_i: usize = 0;
        while (up_i < upper_used) : (up_i += 1) {
            writeU32Le(out_bytes, off, upper_neighbors[up_i]);
            off += 4;
        }

        // 6. Trailing IEEE CRC32 checksum.
        const crc: u32 = crc32Ieee(out_bytes.*[0..off]);
        writeU32Le(out_bytes, off, crc);
        off += SNAPSHOT_CRC_BYTES;
        return off;
    }

    /// Validates magic, version, bounds, and IEEE CRC32 checksum, then reconstructs a ready-to-query
    /// `HnswFlatIndex` directly into caller-provided slices with zero retraining.
    static fn deserializeSnapshot(
        bytes: []const u8,
        vecs: *[]f32,
        nodes: *[]HnswFlatNode,
        l0_neighbors: *[]u32,
        upper_neighbors: *[]u32,
        visited_marks: *[]u32,
    ): HnswFlatIndex ! HnswFlatError {
        if (bytes.len < SNAPSHOT_HEADER_BYTES + SNAPSHOT_CRC_BYTES) {
            return HnswFlatError.InvalidSnapshot;
        }

        const magic: u32 = readU32Le(bytes, 0);
        const version: u32 = readU32Le(bytes, 4);
        if (magic != HNSW_FLAT_MAGIC || version != HNSW_FLAT_VERSION) {
            return HnswFlatError.InvalidSnapshot;
        }

        const payload_len: usize = bytes.len - SNAPSHOT_CRC_BYTES;
        const stored_crc: u32 = readU32Le(bytes, payload_len);
        const calc_crc: u32 = crc32Ieee(bytes[0..payload_len]);
        if (stored_crc != calc_crc) {
            return HnswFlatError.ChecksumMismatch;
        }

        const metric_opt = metricFromU32(readU32Le(bytes, 28));
        let metric = HnswFlatMetric.l2_sq;
        if (m_val := metric_opt) {
            metric = m_val;
        } else {
            return HnswFlatError.InvalidSnapshot;
        }

        const cfg = HnswFlatConfig{
            dim: @as(usize, readU32Le(bytes, 8)),
            max_nodes: @as(usize, readU32Le(bytes, 12)),
            m: @as(usize, readU32Le(bytes, 16)),
            m_max0: @as(usize, readU32Le(bytes, 20)),
            ef_construction: @as(usize, readU32Le(bytes, 24)),
            metric: metric,
            seed: readU64Le(bytes, 32),
        };
        if (!cfg.isValid()) {
            return HnswFlatError.InvalidSnapshot;
        }

        const node_count: usize = @as(usize, readU32Le(bytes, 40));
        const deleted_count: usize = @as(usize, readU32Le(bytes, 44));
        const max_level: usize = @as(usize, readU32Le(bytes, 48));
        const entry_point: u32 = readU32Le(bytes, 52);
        const upper_used: usize = @as(usize, readU32Le(bytes, 56));
        const rng_lo: u64 = @as(u64, readU32Le(bytes, 60));

        if (node_count > cfg.max_nodes || deleted_count > node_count || max_level >= HNSW_FLAT_MAX_LEVELS) {
            return HnswFlatError.InvalidSnapshot;
        }

        const expected_payload: usize = SNAPSHOT_HEADER_BYTES +
            node_count * SNAPSHOT_NODE_BYTES +
            node_count * cfg.dim * 4 +
            node_count * cfg.m_max0 * 4 +
            upper_used * 4;
        if (payload_len != expected_payload) {
            return HnswFlatError.InvalidSnapshot;
        }

        if (vecs.len < hnswVecBufLen(cfg) ||
            nodes.len < cfg.max_nodes ||
            l0_neighbors.len < hnswL0AdjBufLen(cfg) ||
            upper_neighbors.len < upper_used ||
            visited_marks.len < cfg.max_nodes)
        {
            return HnswFlatError.BufferTooSmall;
        }

        let idx = HnswFlatIndex.init(cfg, vecs, nodes, l0_neighbors, upper_neighbors, visited_marks)?;
        idx.node_count = node_count;
        idx.deleted_count = deleted_count;
        idx.max_level = max_level;
        idx.entry_point = entry_point;
        idx.upper_used = upper_used;
        idx.rng_state = if (rng_lo == 0) cfg.seed else rng_lo;

        let off: usize = SNAPSHOT_HEADER_BYTES;
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            let nd = HnswFlatNode{
                doc_id: readU64Le(bytes, off),
                level: bytes[off + 8],
                deleted: bytes[off + 9] != 0,
                l0_deg: readU16Le(bytes, off + 10),
                upper_offset: readU32Le(bytes, off + 12),
                inv_norm: bitsToF32(readU32Le(bytes, off + 16)),
            };
            let lv: usize = 0;
            while (lv < HNSW_FLAT_MAX_LEVELS) : (lv += 1) {
                nd.upper_deg[lv] = bytes[off + 20 + lv];
            }
            nodes[i] = nd;
            off += SNAPSHOT_NODE_BYTES;
        }

        const total_floats: usize = node_count * cfg.dim;
        let vf: usize = 0;
        while (vf < total_floats) : (vf += 1) {
            vecs[vf] = bitsToF32(readU32Le(bytes, off));
            off += 4;
        }

        const total_l0: usize = node_count * cfg.m_max0;
        let l0_i: usize = 0;
        while (l0_i < total_l0) : (l0_i += 1) {
            l0_neighbors[l0_i] = readU32Le(bytes, off);
            off += 4;
        }

        let up_i: usize = 0;
        while (up_i < upper_used) : (up_i += 1) {
            upper_neighbors[up_i] = readU32Le(bytes, off);
            off += 4;
        }

        return idx;
    }
}

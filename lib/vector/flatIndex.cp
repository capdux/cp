// Source: cp/lib/vector/flat_index.cp (In-Memory Vector Index with Cosine/Dot/L2 Metrics)
// High-performance in-memory dense vector index powered by SIMD similarity kernels from cap:math.

const std = @import("std");
import { cosineSimilarityF32, dotF32, euclideanDistanceF32 } from "cap:math";

/// Error set for dense vector index operations.
pub err VectorError {
    DimensionMismatch,
    InvalidDimension,
    OutOfMemory,
}

/// Similarity / distance metric used by `VectorIndex`.
pub enum DistanceMetric {
    Cosine,
    Dot,
    Euclidean,
}

/// Unified search result entry returned by `VectorIndex`, `Bm25Index`, and `RrfFusion`.
pub struct SearchHit {
    id: string = "";
    score: f32 = 0.0;
    meta: string = "";
}

struct VectorEntry {
    id: []u8 = [];
    vec: []f32 = [];
    meta: []u8 = [];
}

fn hitBetterDesc(a: SearchHit, b: SearchHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn sortHitsDesc(hits: []SearchHit) {
    if (hits.len <= 1) return;
    // Max-heapify then extract to sort descending using hitBetterDesc.
    // Using a min-heap of "worse" elements at the root gives ascending order of worst->best;
    // popping to the end yields descending best->worst in O(N log N) in-place.
    let n = hits.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstRoot(hits, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        siftDownWorstRoot(hits, 0, end);
    }
}

fn siftDownWorstRoot(hits: []SearchHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        // Pick the strictly worse element between left and right (i.e. NOT hitBetterDesc).
        if (right < len && hitBetterDesc(hits[worst], hits[right])) {
            worst = right;
        }
        if (!hitBetterDesc(hits[idx], hits[worst])) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

/// In-memory flat vector index supporting O(1) upsert/remove and SIMD top-k search.
/// Returned `SearchHit.id` and `SearchHit.meta` slices borrow from the index until
/// the corresponding entry is updated, removed, or `deinit()` is called.
pub struct VectorIndex {
    alloc: std.mem.Allocator = undefined;
    dim: usize = 0;
    metric: DistanceMetric = .Cosine;
    entries: std.ArrayList(VectorEntry) = .empty;
    id_to_index: std.StringHashMapUnmanaged(usize) = .empty;

    /// Initializes a `VectorIndex` for vectors of dimension `dim` and metric `metric`.
    pub static fn init(
        alloc: std.mem.Allocator,
        dim: usize,
        metric: DistanceMetric,
    ): VectorIndex ! VectorError {
        if (dim == 0) return VectorError.InvalidDimension;
        return VectorIndex{
            alloc: alloc,
            dim: dim,
            metric: metric,
            entries: .empty,
            id_to_index: .empty,
        };
    }

    /// Releases all memory owned by the index.
    pub fn deinit(*_) {
        for (e of _.entries.items) {
            if (e.id.len > 0) _.alloc.free(e.id);
            if (e.vec.len > 0) _.alloc.free(e.vec);
            if (e.meta.len > 0) _.alloc.free(e.meta);
        }
        _.entries.deinit(_.alloc);
        _.id_to_index.deinit(_.alloc);
    }

    /// Returns the number of indexed vectors.
    pub fn len(): usize {
        return _.entries.items.len;
    }

    /// Inserts a new vector or overwrites the existing vector and metadata for `id`.
    /// Returns `VectorError.DimensionMismatch` if `vec.len != dim`.
    pub fn upsert(*_, id: string, vec: []const f32, meta: string = ""): void ! VectorError {
        if (vec.len != _.dim) return VectorError.DimensionMismatch;

        if (existing_idx := _.id_to_index.get(id)) {
            @memcpy(_.entries.items[existing_idx].vec, vec);
            const new_meta: []u8 = if (meta.len > 0) _.alloc.dupe(u8, meta)? else [];
            if (_.entries.items[existing_idx].meta.len > 0) {
                _.alloc.free(_.entries.items[existing_idx].meta);
            }
            _.entries.items[existing_idx].meta = new_meta;
        } else {
            const owned_id: []u8 = if (id.len > 0) _.alloc.dupe(u8, id)? else [];
            errdefer if (owned_id.len > 0) _.alloc.free(owned_id);

            const owned_vec = _.alloc.dupe(f32, vec)?;
            errdefer _.alloc.free(owned_vec);

            const owned_meta: []u8 = if (meta.len > 0) _.alloc.dupe(u8, meta)? else [];
            errdefer if (owned_meta.len > 0) _.alloc.free(owned_meta);

            const new_idx = _.entries.items.len;
            _.entries.append(_.alloc, VectorEntry{
                id: owned_id,
                vec: owned_vec,
                meta: owned_meta,
            })?;
            errdefer * = _.entries.pop();

            _.id_to_index.put(_.alloc, owned_id, new_idx)?;
        }
    }

    /// Removes the vector with `id` in O(1) time. Returns `true` if removed, `false` if absent.
    pub fn remove(*_, id: string): bool {
        const idx = _.id_to_index.get(id) ?? return false;
        * = _.id_to_index.remove(id);

        const removed = _.entries.items[idx];
        if (removed.id.len > 0) _.alloc.free(removed.id);
        if (removed.vec.len > 0) _.alloc.free(removed.vec);
        if (removed.meta.len > 0) _.alloc.free(removed.meta);

        const last_idx = _.entries.items.len - 1;
        if (idx != last_idx) {
            const last_entry = _.entries.items[last_idx];
            _.entries.items[idx] = last_entry;
            if (ptr := _.id_to_index.getPtr(last_entry.id)) {
                ptr.* = idx;
            }
        }
        * = _.entries.pop();
        return true;
    }

    /// Searches for up to `top_k` vectors with relevance score `>= min_score`,
    /// sorted in descending order of relevance.
    /// For `DistanceMetric.Euclidean`, distance `d` is converted to similarity `1.0 / (1.0 + d)`.
    /// Caller owns the returned slice (`alloc.free(hits)`).
    pub fn search(
        alloc: std.mem.Allocator,
        query: []const f32,
        top_k: usize,
        min_score: f32 = -3.4028234e38,
    ): ([]SearchHit) ! VectorError {
        if (query.len != _.dim) return VectorError.DimensionMismatch;
        if (top_k == 0 || _.entries.items.len == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let candidates: std.ArrayList(SearchHit) = .empty;
        defer candidates.deinit(alloc);

        for (e of _.entries.items) {
            const score: f32 = switch (_.metric) {
                .Cosine => cosineSimilarityF32(query, e.vec),
                .Dot => dotF32(query, e.vec),
                .Euclidean => 1.0 / (1.0 + euclideanDistanceF32(query, e.vec)),
            };
            if (!std.math.isNan(score) && score >= min_score) {
                candidates.append(alloc, SearchHit{
                    id: e.id,
                    score: score,
                    meta: e.meta,
                })?;
            }
        }

        if (candidates.items.len == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        sortHitsDesc(candidates.items);
        const out_len = if (top_k < candidates.items.len) top_k else candidates.items.len;
        let out = alloc.alloc(SearchHit, out_len)?;
        @memcpy(out, candidates.items[0..out_len]);
        return out;
    }
}

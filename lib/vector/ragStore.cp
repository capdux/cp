// Origin: Pure-CP Stdlib (Zero-Heap Local RAG Store, 64-Bit Tag Filter & SIMD Cosine Top-K Facade) | Created: 2026-11-22
// Pure CP zero-heap-allocation all-in-one local RAG storage and retrieval facade:
//   - Caller-owned metadata + flat L2-normalized embedding slab (`InMemoryRagStore`)
//   - In-place `upsert` by `doc_id` with automatic tombstone slot reuse on `remove`
//   - Namespace + 64-bit tag bitmask pre-filtering + 8-lane SIMD `@dot` cosine Top-K recall (`searchFiltered`)
//   - Token-budgeted LLM prompt context packing with `[doc:<id>]` source attribution (`packSearchContext`)

const std = @import("std");

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn appendU64(out: *[]u8, pos: *usize, v: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let cur: u64 = v;
    if (cur == 0) {
        tmp[0] = 48; // '0'
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

/// Metadata slot for a single stored document chunk in `InMemoryRagStore`.
pub struct RagDocMeta {
    doc_id: u64 = 0;
    namespace_hash: u64 = 0;
    tag_mask: u64 = 0;
    token_count: u32 = 0;
    timestamp_ms: i64 = 0;
    text: string = "";
    occupied: bool = false;
    deleted: bool = false;
}

/// Single Top-K search match returned by `InMemoryRagStore.searchFiltered`.
pub struct RagSearchHit {
    doc_id: u64 = 0;
    slot_idx: usize = 0;
    score: f32 = 0.0;
    token_count: u32 = 0;
    timestamp_ms: i64 = 0;
    text: string = "";
}

/// Aggregate capacity and operational statistics for `InMemoryRagStore`.
pub struct RagStoreStats {
    capacity: usize = 0;
    dim: usize = 0;
    active_docs: usize = 0;
    deleted_slots: usize = 0;
    free_slots: usize = 0;
    total_upserts: u64 = 0;
    total_updates: u64 = 0;
    total_removals: u64 = 0;
}

/// Zero-heap-allocation local RAG store with L2-normalized embeddings, 64-bit tag bitmask filtering,
/// 8-lane SIMD `@dot` cosine Top-K recall, and token-budgeted context packing.
pub struct InMemoryRagStore {
    metas: []RagDocMeta;
    vecs_flat: []f32;
    dim: usize = 0;
    active_count: usize = 0;
    deleted_count: usize = 0;
    total_upserts: u64 = 0;
    total_updates: u64 = 0;
    total_removals: u64 = 0;

    /// Initializes a zero-alloc RAG store over caller-owned `metas_buf` and `vecs_flat_buf`.
    /// Requires `metas_buf.len > 0`, `dim > 0`, and `vecs_flat_buf.len >= metas_buf.len * dim`.
    static fn init(
        metas_buf: *[]RagDocMeta,
        vecs_flat_buf: *[]f32,
        dim: usize,
    ): InMemoryRagStore? {
        if (metas_buf.len == 0 || dim == 0) return null;
        if (metas_buf.len > 0xFFFFFFFFFFFFFFFF / dim) return null;
        const needed = metas_buf.len * dim;
        if (vecs_flat_buf.len < needed) return null;

        let i: usize = 0;
        while (i < metas_buf.len) : (i += 1) {
            metas_buf[i] = RagDocMeta{};
        }

        return InMemoryRagStore{
            metas: metas_buf,
            vecs_flat: vecs_flat_buf,
            dim: dim,
            active_count: 0,
            deleted_count: 0,
            total_upserts: 0,
            total_updates: 0,
            total_removals: 0,
        };
    }

    /// Inserts or updates `doc_id` with an automatically L2-normalized `embedding`.
    /// Reuses tombstone slots freed by `remove`. Returns `false` on `doc_id == 0`,
    /// dimension mismatch, zero/NaN embedding, or full store.
    fn upsert(
        *_,
        doc_id: u64,
        namespace_hash: u64,
        tag_mask: u64,
        token_count: u32,
        timestamp_ms: i64,
        text: string,
        embedding: []const f32,
    ): bool {
        if (doc_id == 0 || embedding.len != _.dim) return false;

        let k: usize = 0;
        while (k < _.dim) : (k += 1) {
            if (!isFiniteF32(embedding[k])) return false;
        }
        const norm_sq: f32 = @dot(embedding, embedding);
        if (!isFiniteF32(norm_sq) || norm_sq <= 0.000000000001) return false;
        const inv_norm: f32 = 1.0 / std.math.sqrt(norm_sq);

        let existing_idx: usize? = null;
        let deleted_idx: usize? = null;
        let free_idx: usize? = null;

        let i: usize = 0;
        while (i < _.metas.len) : (i += 1) {
            if (_.metas[i].occupied && !_.metas[i].deleted) {
                if (_.metas[i].doc_id == doc_id) {
                    existing_idx = i;
                    break;
                }
            } else if (_.metas[i].occupied && _.metas[i].deleted) {
                if (deleted_idx == null) deleted_idx = i;
            } else {
                if (free_idx == null) free_idx = i;
            }
        }

        let target_idx: usize = 0;
        let is_update = false;
        let reused_deleted = false;
        if (idx := existing_idx) {
            target_idx = idx;
            is_update = true;
        } else if (d_idx := deleted_idx) {
            target_idx = d_idx;
            reused_deleted = true;
        } else if (f_idx := free_idx) {
            target_idx = f_idx;
        } else {
            return false;
        }

        const base = target_idx * _.dim;
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            _.vecs_flat[base + d] = embedding[d] * inv_norm;
        }

        _.metas[target_idx] = RagDocMeta{
            doc_id: doc_id,
            namespace_hash: namespace_hash,
            tag_mask: tag_mask,
            token_count: token_count,
            timestamp_ms: timestamp_ms,
            text: text,
            occupied: true,
            deleted: false,
        };

        _.total_upserts += 1;
        if (is_update) {
            _.total_updates += 1;
        } else {
            _.active_count += 1;
            if (reused_deleted && _.deleted_count > 0) {
                _.deleted_count -= 1;
            }
        }
        return true;
    }

    /// Logically deletes `doc_id`, marking its slot as a reusable tombstone.
    fn remove(*_, doc_id: u64): bool {
        if (doc_id == 0) return false;
        let i: usize = 0;
        while (i < _.metas.len) : (i += 1) {
            if (_.metas[i].occupied && !_.metas[i].deleted && _.metas[i].doc_id == doc_id) {
                _.metas[i].deleted = true;
                if (_.active_count > 0) _.active_count -= 1;
                _.deleted_count += 1;
                _.total_removals += 1;
                return true;
            }
        }
        return false;
    }

    /// Returns a read-only copy of the active metadata for `doc_id`, or `null` if absent/deleted.
    fn get(doc_id: u64): RagDocMeta? {
        if (doc_id == 0) return null;
        let i: usize = 0;
        while (i < _.metas.len) : (i += 1) {
            if (_.metas[i].occupied && !_.metas[i].deleted && _.metas[i].doc_id == doc_id) {
                return _.metas[i];
            }
        }
        return null;
    }

    /// Pre-filters by `namespace_hash` (`0` = all namespaces) and 64-bit `required_tag_mask`,
    /// then performs 8-lane SIMD `@dot` cosine similarity Top-K retrieval sorted by `(score desc, doc_id asc)`.
    /// Returns number of hits written to `out_hits`, or `null` on dimension mismatch, zero/NaN query, or invalid `top_k`.
    fn searchFiltered(
        query_vec: []const f32,
        namespace_hash: u64,
        required_tag_mask: u64,
        min_score: f32,
        top_k: usize,
        out_hits: *[]RagSearchHit,
    ): usize? {
        if (query_vec.len != _.dim || top_k == 0 || out_hits.len < top_k) return null;
        if (!isFiniteF32(min_score)) return null;

        let qk: usize = 0;
        while (qk < _.dim) : (qk += 1) {
            if (!isFiniteF32(query_vec[qk])) return null;
        }
        const q_norm_sq: f32 = @dot(query_vec, query_vec);
        if (!isFiniteF32(q_norm_sq) || q_norm_sq <= 0.000000000001) return null;
        const q_inv_norm: f32 = 1.0 / std.math.sqrt(q_norm_sq);

        let hit_count: usize = 0;
        let i: usize = 0;
        while (i < _.metas.len) : (i += 1) {
            if (!_.metas[i].occupied || _.metas[i].deleted) continue;
            if (namespace_hash != 0 && _.metas[i].namespace_hash != namespace_hash) continue;
            if ((_.metas[i].tag_mask & required_tag_mask) != required_tag_mask) continue;

            const base = i * _.dim;
            const unit_vec: []const f32 = _.vecs_flat[base..(base + _.dim)];
            const score: f32 = @dot(query_vec, unit_vec) * q_inv_norm;
            if (!isFiniteF32(score) || score < min_score) continue;

            const cand = RagSearchHit{
                doc_id: _.metas[i].doc_id,
                slot_idx: i,
                score: score,
                token_count: _.metas[i].token_count,
                timestamp_ms: _.metas[i].timestamp_ms,
                text: _.metas[i].text,
            };

            // Determine insertion position in sorted Top-K array (score desc, doc_id asc).
            let pos: usize = hit_count;
            while (pos > 0) {
                const prev = out_hits[pos - 1];
                const better = (cand.score > prev.score) ||
                    (cand.score == prev.score && cand.doc_id < prev.doc_id);
                if (!better) break;
                if (pos < top_k) {
                    out_hits[pos] = prev;
                }
                pos -= 1;
            }
            if (pos < top_k) {
                out_hits[pos] = cand;
                if (hit_count < top_k) {
                    hit_count += 1;
                }
            }
        }
        return hit_count;
    }

    /// Packs retrieved `hits` in order within `max_tokens` budget into `out_buf` with `[doc:<id>]\n<text>\n\n` attribution.
    /// Skips hits that exceed remaining token budget. Returns bytes written, or `null` if `out_buf` overflows.
    fn packSearchContext(
        hits: []const RagSearchHit,
        max_tokens: u32,
        out_buf: *[]u8,
    ): usize? {
        if (max_tokens == 0 || hits.len == 0) return 0;

        let used_tokens: u32 = 0;
        let pos: usize = 0;
        let i: usize = 0;
        while (i < hits.len) : (i += 1) {
            const tc = hits[i].token_count;
            if (tc > max_tokens - used_tokens) {
                continue;
            }
            if (!appendBytes(out_buf, &pos, "[doc:")) return null;
            if (!appendU64(out_buf, &pos, hits[i].doc_id)) return null;
            if (!appendBytes(out_buf, &pos, "]\n")) return null;
            if (!appendBytes(out_buf, &pos, hits[i].text)) return null;
            if (!appendBytes(out_buf, &pos, "\n\n")) return null;
            used_tokens += tc;
        }
        return pos;
    }

    /// Returns current number of active (non-deleted) documents.
    fn len(): usize {
        return _.active_count;
    }

    /// Returns maximum number of document slots in this store.
    fn capacity(): usize {
        return _.metas.len;
    }

    /// Returns embedding dimensionality.
    fn embeddingDim(): usize {
        return _.dim;
    }

    /// Returns live store capacity and operational statistics.
    fn stats(): RagStoreStats {
        const cap = _.metas.len;
        const used = _.active_count + _.deleted_count;
        const free = if (cap >= used) cap - used else 0;
        return RagStoreStats{
            capacity: cap,
            dim: _.dim,
            active_docs: _.active_count,
            deleted_slots: _.deleted_count,
            free_slots: free,
            total_upserts: _.total_upserts,
            total_updates: _.total_updates,
            total_removals: _.total_removals,
        };
    }
}

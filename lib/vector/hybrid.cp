// Source: cp/lib/vector/hybrid.cp (Reciprocal Rank Fusion Hybrid RAG Ranker)
// Industrial-standard Reciprocal Rank Fusion (RRF) ranker combining dense semantic vector
// hits with sparse BM25 lexical hits into a deduplicated, descending relevance ranking.

const std = @import("std");
import { SearchHit } from "./flat_index.cp";

struct FusedAcc {
    score: f32 = 0.0;
    meta: string = "";
}

fn rrfHitBetterDesc(a: SearchHit, b: SearchHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn rrfSiftDownWorstRoot(hits: []SearchHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && rrfHitBetterDesc(hits[worst], hits[right])) {
            worst = right;
        }
        if (!rrfHitBetterDesc(hits[idx], hits[worst])) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

fn rrfSortHitsDesc(hits: []SearchHit) {
    if (hits.len <= 1) return;
    let n = hits.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        rrfSiftDownWorstRoot(hits, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        rrfSiftDownWorstRoot(hits, 0, end);
    }
}

/// Reciprocal Rank Fusion (RRF) ranker for hybrid RAG retrieval.
pub static struct RrfFusion {
    /// Fuses ranked `vector_hits` and `text_hits` using Reciprocal Rank Fusion:
    /// `score(doc) = sum(1.0 / (rrf_k + rank))` with 1-based rank (`1, 2, ...`).
    /// Deduplicates results by `id`, preserves non-empty `meta` (preferring `vector_hits`),
    /// and returns up to `top_k` results sorted descending by fused RRF score.
    static fn fuse(
        alloc: std.mem.Allocator,
        vector_hits: []const SearchHit,
        text_hits: []const SearchHit,
        rrf_k: f32 = 60.0,
        top_k: usize = 10,
    ): ([]SearchHit)! {
        if (top_k == 0 || (vector_hits.len == 0 && text_hits.len == 0)) {
            return alloc.alloc(SearchHit, 0)?;
        }

        const k: f32 = if (rrf_k >= 0.0) rrf_k else 60.0;

        let fused: std.StringHashMapUnmanaged(FusedAcc) = .empty;
        defer fused.deinit(alloc);

        let seen_vec: std.StringHashMapUnmanaged(u8) = .empty;
        defer seen_vec.deinit(alloc);

        let seen_text: std.StringHashMapUnmanaged(u8) = .empty;
        defer seen_text.deinit(alloc);

        let i: usize = 0;
        while (i < vector_hits.len) : (i += 1) {
            const hit = vector_hits[i];
            if (seen_vec.contains(hit.id)) continue;
            seen_vec.put(alloc, hit.id, 1)?;

            const rank = (i + 1) as f32;
            const rrf_contrib = 1.0 / (k + rank);
            if (entry := fused.getEntry(hit.id)) {
                entry.value_ptr.score += rrf_contrib;
                if (entry.value_ptr.meta.len == 0 && hit.meta.len > 0) {
                    entry.value_ptr.meta = hit.meta;
                }
            } else {
                fused.put(alloc, hit.id, FusedAcc{
                    score: rrf_contrib,
                    meta: hit.meta,
                })?;
            }
        }

        let j: usize = 0;
        while (j < text_hits.len) : (j += 1) {
            const hit = text_hits[j];
            if (seen_text.contains(hit.id)) continue;
            seen_text.put(alloc, hit.id, 1)?;

            const rank = (j + 1) as f32;
            const rrf_contrib = 1.0 / (k + rank);
            if (entry := fused.getEntry(hit.id)) {
                entry.value_ptr.score += rrf_contrib;
                if (entry.value_ptr.meta.len == 0 && hit.meta.len > 0) {
                    entry.value_ptr.meta = hit.meta;
                }
            } else {
                fused.put(alloc, hit.id, FusedAcc{
                    score: rrf_contrib,
                    meta: hit.meta,
                })?;
            }
        }

        if (fused.count() == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let candidates: std.ArrayList(SearchHit) = .empty;
        defer candidates.deinit(alloc);

        let f_iter = fused.iterator();
        while (f_entry := f_iter.next()) {
            candidates.append(alloc, SearchHit{
                id: f_entry.key_ptr.*,
                score: f_entry.value_ptr.score,
                meta: f_entry.value_ptr.meta,
            })?;
        }

        rrfSortHitsDesc(candidates.items);
        const out_len = if (top_k < candidates.items.len) top_k else candidates.items.len;
        let out = alloc.alloc(SearchHit, out_len)?;
        @memcpy(out, candidates.items[0..out_len]);
        return out;
    }
}

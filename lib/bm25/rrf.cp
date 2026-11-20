// Source: cp/lib/bm25/rrf.cp (Reciprocal Rank Fusion for Hybrid Search)
const std = @import("std");
import { Bm25Hit, sortAndTruncateHits } from "./bm25.cp";

pub struct RankedDoc {
    doc_id: u64,
    rank: usize,
}

pub fn rrfFuse(
    alloc: std.mem.Allocator,
    lists: []const []const RankedDoc,
    weights: []const f64,
    k: f64,
    top_k: usize,
): ![]Bm25Hit {
    if (top_k == 0 or lists.len == 0) {
        return alloc.alloc(Bm25Hit, 0);
    }

    let scores = std.AutoHashMapUnmanaged(u64, f64).empty;
    defer scores.deinit(alloc);

    let idx: usize = 0;
    for (list of lists) {
        const w = if (idx < weights.len) weights[idx] else 1.0;
        for (item of list) {
            const denom = k + @as(f64, @floatFromInt(item.rank));
            if (denom <= 0.0) {
                continue;
            }
            const contrib = w / denom;
            const gop = try scores.getOrPut(alloc, item.doc_id);
            if (!gop.found_existing) {
                gop.value_ptr.* = 0.0;
            }
            gop.value_ptr.* += contrib;
        }
        idx += 1;
    }

    let hits = std.ArrayList(Bm25Hit).empty;
    defer hits.deinit(alloc);

    let it = scores.iterator();
    while (entry := it.next()) {
        try hits.append(alloc, Bm25Hit{
            .doc_id = entry.key_ptr.*,
            .score = entry.value_ptr.*,
        });
    }

    return sortAndTruncateHits(alloc, &hits, top_k);
}

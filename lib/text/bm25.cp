// Source: cp/lib/bm25/bm25.cp (Okapi BM25 Inverted Index & Phrase Search)
const std = @import("std");
import { TokenizerConfig, tokenizeTerms, freeTerms, lowercaseTerm } from "./bm25Tokenizer.cp";

pub const Bm25Error = error {
    InvalidConfig,
    OutOfMemory,
};

pub struct Bm25Config {
    k1: f64 = 1.2,
    b: f64 = 0.75,
    tokenizer: TokenizerConfig = TokenizerConfig{},
}

pub struct Posting {
    doc_id: u64,
    tf: u32,
    positions: []u32,
}

pub struct Bm25Hit {
    doc_id: u64,
    score: f64,
}

fn hitLessThan(_: void, a: Bm25Hit, b: Bm25Hit): bool {
    if (a.score > b.score) {
        return true;
    }
    if (a.score < b.score) {
        return false;
    }
    return a.doc_id < b.doc_id;
}

pub fn sortAndTruncateHits(alloc: std.mem.Allocator, list: *std.ArrayList(Bm25Hit), top_k: usize): ![]Bm25Hit {
    std.mem.sort(Bm25Hit, list.items, {}, hitLessThan);
    const keep = if (top_k < list.items.len) top_k else list.items.len;
    const out = try alloc.alloc(Bm25Hit, keep);
    @memcpy(out, list.items[0..keep]);
    return out;
}

fn containsPosition(positions: []const u32, target: u32): bool {
    let lo: usize = 0;
    let hi: usize = positions.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        if (positions[mid] == target) {
            return true;
        }
        if (positions[mid] < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return false;
}

fn findPosting(postings: []const Posting, doc_id: u64): Posting? {
    for (p of postings) {
        if (p.doc_id == doc_id) {
            return p;
        }
    }
    return null;
}

pub struct Bm25Index {
    alloc: std.mem.Allocator,
    config: Bm25Config,
    postings: std.StringHashMapUnmanaged(std.ArrayList(Posting)),
    doc_lengths: std.AutoHashMapUnmanaged(u64, u32),
    doc_terms: std.AutoHashMapUnmanaged(u64, std.ArrayList([]u8)),
    total_tokens: u64,

    pub fn init(alloc: std.mem.Allocator, config: Bm25Config): !Bm25Index {
        if (config.k1 < 0.0 or config.b < 0.0 or config.b > 1.0) {
            return Bm25Error.InvalidConfig;
        }
        return Bm25Index{
            .alloc = alloc,
            .config = config,
            .postings = std.StringHashMapUnmanaged(std.ArrayList(Posting)).empty,
            .doc_lengths = std.AutoHashMapUnmanaged(u64, u32).empty,
            .doc_terms = std.AutoHashMapUnmanaged(u64, std.ArrayList([]u8)).empty,
            .total_tokens = 0,
        };
    }

    pub fn deinit(*_) {
        let p_it = _.postings.iterator();
        while (entry := p_it.next()) {
            _.alloc.free(entry.key_ptr.*);
            for (p of entry.value_ptr.items) {
                _.alloc.free(p.positions);
            }
            entry.value_ptr.deinit(_.alloc);
        }
        _.postings.deinit(_.alloc);

        let dt_it = _.doc_terms.iterator();
        while (entry := dt_it.next()) {
            for (t of entry.value_ptr.items) {
                _.alloc.free(t);
            }
            entry.value_ptr.deinit(_.alloc);
        }
        _.doc_terms.deinit(_.alloc);
        _.doc_lengths.deinit(_.alloc);
    }

    pub fn docCount(): usize {
        return _.doc_lengths.count();
    }

    pub fn avgDocLen(): f64 {
        const count = _.doc_lengths.count();
        if (count == 0) {
            return 0.0;
        }
        return @as(f64, @floatFromInt(_.total_tokens)) / @as(f64, @floatFromInt(count));
    }

    pub fn removeDocument(*_, doc_id: u64): bool {
        const dl_opt = _.doc_lengths.get(doc_id);
        if (dl_opt == null) {
            return false;
        }
        const dl = dl_opt.?;
        _.total_tokens -= @as(u64, @intCast(dl));
        * = _.doc_lengths.remove(doc_id);

        if (term_list := _.doc_terms.getPtr(doc_id)) {
            for (term of term_list.items) {
                if (plist := _.postings.getPtr(term)) {
                    let idx: usize = 0;
                    while (idx < plist.items.len) {
                        if (plist.items[idx].doc_id == doc_id) {
                            _.alloc.free(plist.items[idx].positions);
                            * = plist.swapRemove(idx);
                            break;
                        }
                        idx += 1;
                    }
                    if (plist.items.len == 0) {
                        if (removed := _.postings.fetchRemove(term)) {
                            _.alloc.free(removed.key);
                            let val = removed.value;
                            val.deinit(_.alloc);
                        }
                    }
                }
                _.alloc.free(term);
            }
            term_list.deinit(_.alloc);
            * = _.doc_terms.remove(doc_id);
        }
        return true;
    }

    pub fn addDocument(*_, doc_id: u64, text: []const u8): !void {
        if (_.doc_lengths.contains(doc_id)) {
            * = _.removeDocument(doc_id);
        }

        const terms = try tokenizeTerms(_.alloc, text, _.config.tokenizer);
        defer freeTerms(_.alloc, terms);

        const doc_len: u32 = @as(u32, @intCast(terms.len));
        try _.doc_lengths.put(_.alloc, doc_id, doc_len);
        _.total_tokens += @as(u64, @intCast(doc_len));

        let term_pos = std.StringHashMapUnmanaged(std.ArrayList(u32)).empty;
        defer {
            let it = term_pos.iterator();
            while (entry := it.next()) {
                entry.value_ptr.deinit(_.alloc);
            }
            term_pos.deinit(_.alloc);
        }

        let pos: usize = 0;
        for (term of terms) {
            const gop = try term_pos.getOrPut(_.alloc, term);
            if (!gop.found_existing) {
                gop.value_ptr.* = std.ArrayList(u32).empty;
            }
            try gop.value_ptr.append(_.alloc, @as(u32, @intCast(pos)));
            pos += 1;
        }

        let doc_term_list = std.ArrayList([]u8).empty;
        errdefer {
            for (t of doc_term_list.items) {
                _.alloc.free(t);
            }
            doc_term_list.deinit(_.alloc);
        }

        let it = term_pos.iterator();
        while (entry := it.next()) {
            const term = entry.key_ptr.*;
            const pos_slice = try entry.value_ptr.toOwnedSlice(_.alloc);
            const tf: u32 = @as(u32, @intCast(pos_slice.len));

            const posting = Posting{
                .doc_id = doc_id,
                .tf = tf,
                .positions = pos_slice,
            };

            const p_gop = try _.postings.getOrPut(_.alloc, term);
            if (!p_gop.found_existing) {
                p_gop.key_ptr.* = try _.alloc.dupe(u8, term);
                p_gop.value_ptr.* = std.ArrayList(Posting).empty;
            }
            try p_gop.value_ptr.append(_.alloc, posting);

            const dt_copy = try _.alloc.dupe(u8, term);
            try doc_term_list.append(_.alloc, dt_copy);
        }

        try _.doc_terms.put(_.alloc, doc_id, doc_term_list);
    }

    fn computeIdf(df_usize: usize): f64 {
        const n = @as(f64, @floatFromInt(_.doc_lengths.count()));
        const df = @as(f64, @floatFromInt(df_usize));
        return @log((n - df + 0.5) / (df + 0.5) + 1.0);
    }

    fn bm25TermScore(idf: f64, tf_u32: u32, dl_u32: u32, avgdl: f64): f64 {
        if (avgdl <= 0.0) {
            return 0.0;
        }
        const tf = @as(f64, @floatFromInt(tf_u32));
        const dl = @as(f64, @floatFromInt(dl_u32));
        const k1 = _.config.k1;
        const b = _.config.b;
        const denom = tf + k1 * (1.0 - b + b * dl / avgdl);
        if (denom <= 0.0) {
            return 0.0;
        }
        return idf * (tf * (k1 + 1.0)) / denom;
    }

    pub fn search(alloc: std.mem.Allocator, query: []const u8, top_k: usize): ![]Bm25Hit {
        if (top_k == 0 or _.doc_lengths.count() == 0) {
            return alloc.alloc(Bm25Hit, 0);
        }

        const q_terms = try tokenizeTerms(alloc, query, _.config.tokenizer);
        defer freeTerms(alloc, q_terms);

        if (q_terms.len == 0) {
            return alloc.alloc(Bm25Hit, 0);
        }

        const avgdl = _.avgDocLen();
        let scores = std.AutoHashMapUnmanaged(u64, f64).empty;
        defer scores.deinit(alloc);

        for (qt of q_terms) {
            if (plist := _.postings.get(qt)) {
                const idf = _.computeIdf(plist.items.len);
                for (posting of plist.items) {
                    const dl = _.doc_lengths.get(posting.doc_id) orelse continue;
                    const s = _.bm25TermScore(idf, posting.tf, dl, avgdl);
                    const gop = try scores.getOrPut(alloc, posting.doc_id);
                    if (!gop.found_existing) {
                        gop.value_ptr.* = 0.0;
                    }
                    gop.value_ptr.* += s;
                }
            }
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

    pub fn searchPhrase(alloc: std.mem.Allocator, phrase_terms: []const []const u8, top_k: usize): ![]Bm25Hit {
        if (phrase_terms.len == 0 or top_k == 0 or _.doc_lengths.count() == 0) {
            return alloc.alloc(Bm25Hit, 0);
        }

        let norm_terms = try alloc.alloc([]u8, phrase_terms.len);
        let built: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < built) {
                alloc.free(norm_terms[j]);
                j += 1;
            }
            alloc.free(norm_terms);
        }
        let idx: usize = 0;
        for (raw of phrase_terms) {
            norm_terms[idx] = try lowercaseTerm(alloc, raw, _.config.tokenizer.lowercase);
            built += 1;
            idx += 1;
        }
        defer {
            for (nt of norm_terms) {
                alloc.free(nt);
            }
            alloc.free(norm_terms);
        }

        const first_plist = _.postings.get(norm_terms[0]) orelse {
            return alloc.alloc(Bm25Hit, 0);
        };

        let idf_sum: f64 = 0.0;
        for (nt of norm_terms) {
            const plist = _.postings.get(nt) orelse {
                return alloc.alloc(Bm25Hit, 0);
            };
            idf_sum += _.computeIdf(plist.items.len);
        }

        const avgdl = _.avgDocLen();
        let hits = std.ArrayList(Bm25Hit).empty;
        defer hits.deinit(alloc);

        for (p0 of first_plist.items) {
            let match_count: u32 = 0;
            for (start_pos of p0.positions) {
                let all_match = true;
                let offset: usize = 1;
                while (offset < norm_terms.len) {
                    const next_plist = _.postings.get(norm_terms[offset]).?;
                    const next_p = findPosting(next_plist.items, p0.doc_id);
                    if (next_p == null) {
                        all_match = false;
                        break;
                    }
                    const target_pos = start_pos + @as(u32, @intCast(offset));
                    if (!containsPosition(next_p.?.positions, target_pos)) {
                        all_match = false;
                        break;
                    }
                    offset += 1;
                }
                if (all_match) {
                    match_count += 1;
                }
            }

            if (match_count > 0) {
                const dl = _.doc_lengths.get(p0.doc_id) orelse continue;
                const score = _.bm25TermScore(idf_sum, match_count, dl, avgdl);
                try hits.append(alloc, Bm25Hit{
                    .doc_id = p0.doc_id,
                    .score = score,
                });
            }
        }

        return sortAndTruncateHits(alloc, &hits, top_k);
    }
}

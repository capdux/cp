// Source: cp/lib/vector/bm25.cp (Okapi BM25 Full-Text Inverted Index)
// Pure CP Okapi BM25 inverted index with ASCII alphanumeric tokenizer, stopword filtering,
// term deduplication, document deletion, and exact IDF/TF-saturation scoring.

const std = @import("std");
import { SearchHit } from "./flatIndex.cp";

/// Configuration parameters for Okapi BM25 scoring.
pub struct Bm25Config {
    k1: f64 = 1.5;
    b: f64 = 0.75;
    use_stopwords: bool = true;
}

struct Posting {
    doc_slot: usize = 0;
    tf: u32 = 0;
}

struct DocSlot {
    active: bool = false;
    id: []u8 = [];
    doc_len: usize = 0;
    terms: []string = [];
}

fn isAsciiAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn toAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isStopword(tok: []const u8): bool {
    if (tok.len == 0 || tok.len > 5) return false;
    if (std.mem.eql(u8, tok, "a")) return true;
    if (std.mem.eql(u8, tok, "an")) return true;
    if (std.mem.eql(u8, tok, "and")) return true;
    if (std.mem.eql(u8, tok, "are")) return true;
    if (std.mem.eql(u8, tok, "as")) return true;
    if (std.mem.eql(u8, tok, "at")) return true;
    if (std.mem.eql(u8, tok, "be")) return true;
    if (std.mem.eql(u8, tok, "but")) return true;
    if (std.mem.eql(u8, tok, "by")) return true;
    if (std.mem.eql(u8, tok, "for")) return true;
    if (std.mem.eql(u8, tok, "if")) return true;
    if (std.mem.eql(u8, tok, "in")) return true;
    if (std.mem.eql(u8, tok, "into")) return true;
    if (std.mem.eql(u8, tok, "is")) return true;
    if (std.mem.eql(u8, tok, "it")) return true;
    if (std.mem.eql(u8, tok, "no")) return true;
    if (std.mem.eql(u8, tok, "not")) return true;
    if (std.mem.eql(u8, tok, "of")) return true;
    if (std.mem.eql(u8, tok, "on")) return true;
    if (std.mem.eql(u8, tok, "or")) return true;
    if (std.mem.eql(u8, tok, "such")) return true;
    if (std.mem.eql(u8, tok, "that")) return true;
    if (std.mem.eql(u8, tok, "the")) return true;
    if (std.mem.eql(u8, tok, "their")) return true;
    if (std.mem.eql(u8, tok, "then")) return true;
    if (std.mem.eql(u8, tok, "there")) return true;
    if (std.mem.eql(u8, tok, "these")) return true;
    if (std.mem.eql(u8, tok, "they")) return true;
    if (std.mem.eql(u8, tok, "this")) return true;
    if (std.mem.eql(u8, tok, "to")) return true;
    if (std.mem.eql(u8, tok, "was")) return true;
    if (std.mem.eql(u8, tok, "will")) return true;
    if (std.mem.eql(u8, tok, "with")) return true;
    return false;
}

fn bm25HitBetterDesc(a: SearchHit, b: SearchHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.id, b.id) == .lt;
}

fn bm25SiftDownWorstRoot(hits: []SearchHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && bm25HitBetterDesc(hits[worst], hits[right])) {
            worst = right;
        }
        if (!bm25HitBetterDesc(hits[idx], hits[worst])) break;
        const tmp = hits[idx];
        hits[idx] = hits[worst];
        hits[worst] = tmp;
        idx = worst;
    }
}

fn bm25SortHitsDesc(hits: []SearchHit) {
    if (hits.len <= 1) return;
    let n = hits.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        bm25SiftDownWorstRoot(hits, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = hits[0];
        hits[0] = hits[end];
        hits[end] = tmp;
        bm25SiftDownWorstRoot(hits, 0, end);
    }
}

/// Tokenizes `text` into lowercased ASCII alphanumeric tokens, counting frequencies
/// in `term_counts` (deduplicating repeated occurrences) and returning total indexed token count.
fn tokenizeCount(
    alloc: std.mem.Allocator,
    text: string,
    use_stopwords: bool,
    term_counts: *std.StringHashMapUnmanaged(u32),
): usize! {
    let total_tokens: usize = 0;
    let scratch: std.ArrayList(u8) = .empty;
    defer scratch.deinit(alloc);

    let i: usize = 0;
    while (i <= text.len) : (i += 1) {
        const at_end = (i == text.len);
        if (!at_end && isAsciiAlnum(text[i])) {
            scratch.append(alloc, toAsciiLower(text[i]))?;
            continue;
        }
        if (scratch.items.len > 0) {
            const tok = scratch.items;
            if (!use_stopwords || !isStopword(tok)) {
                total_tokens += 1;
                if (entry := term_counts.getEntry(tok)) {
                    entry.value_ptr.* += 1;
                } else {
                    const owned_tok = alloc.dupe(u8, tok)?;
                    errdefer alloc.free(owned_tok);
                    term_counts.put(alloc, owned_tok, 1)?;
                }
            }
            scratch.clearRetainingCapacity();
        }
    }
    return total_tokens;
}

fn freeTokenMap(alloc: std.mem.Allocator, map: *std.StringHashMapUnmanaged(u32)) {
    let iter = map.iterator();
    while (entry := iter.next()) {
        if (entry.key_ptr.*.len > 0) {
            alloc.free(entry.key_ptr.*);
        }
    }
    map.deinit(alloc);
}

/// In-memory Okapi BM25 inverted index for lexical keyword search.
/// Returned `SearchHit.id` slices borrow from the index until the document is
/// updated, removed, or `deinit()` is called.
pub struct Bm25Index {
    alloc: std.mem.Allocator = undefined;
    config: Bm25Config = Bm25Config{};
    slots: std.ArrayList(DocSlot) = .empty;
    free_slots: std.ArrayList(usize) = .empty;
    id_to_slot: std.StringHashMapUnmanaged(usize) = .empty;
    postings: std.StringHashMapUnmanaged(std.ArrayList(Posting)) = .empty;
    active_docs: usize = 0;
    total_doc_len: usize = 0;

    /// Initializes a `Bm25Index` with optional configuration (`k1 = 1.5, b = 0.75`).
    pub static fn init(
        alloc: std.mem.Allocator,
        config: Bm25Config = Bm25Config{},
    ): Bm25Index {
        return Bm25Index{
            alloc: alloc,
            config: config,
            slots: .empty,
            free_slots: .empty,
            id_to_slot: .empty,
            postings: .empty,
            active_docs: 0,
            total_doc_len: 0,
        };
    }

    /// Releases all memory owned by the BM25 index.
    pub fn deinit(*_) {
        _.id_to_slot.deinit(_.alloc);

        for (slot of _.slots.items) {
            if (slot.active) {
                if (slot.id.len > 0) _.alloc.free(slot.id);
                if (slot.terms.len > 0) _.alloc.free(slot.terms);
            }
        }
        _.slots.deinit(_.alloc);
        _.free_slots.deinit(_.alloc);

        let p_iter = _.postings.iterator();
        while (entry := p_iter.next()) {
            entry.value_ptr.deinit(_.alloc);
            if (entry.key_ptr.*.len > 0) _.alloc.free(entry.key_ptr.*);
        }
        _.postings.deinit(_.alloc);
        _.active_docs = 0;
        _.total_doc_len = 0;
    }

    /// Returns the number of active documents in the index.
    pub fn docCount(): usize {
        return _.active_docs;
    }

    /// Indexes or replaces document `id` with `text`.
    pub fn addDocument(*_, id: string, text: string): ! {
        if (_.id_to_slot.contains(id)) {
            * = _.removeDocument(id);
        }

        let term_counts: std.StringHashMapUnmanaged(u32) = .empty;
        defer freeTokenMap(_.alloc, *term_counts);

        const doc_len = tokenizeCount(_.alloc, text, _.config.use_stopwords, *term_counts)?;

        const owned_id: []u8 = if (id.len > 0) _.alloc.dupe(u8, id)? else [];
        errdefer if (owned_id.len > 0) _.alloc.free(owned_id);

        const unique_count = term_counts.count();
        let doc_terms = _.alloc.alloc(string, unique_count)?;
        errdefer _.alloc.free(doc_terms);

        let slot_idx: usize = 0;
        let reused_slot = false;
        if (_.free_slots.items.len > 0) {
            slot_idx = _.free_slots.pop().?;
            reused_slot = true;
        } else {
            slot_idx = _.slots.items.len;
            _.slots.append(_.alloc, DocSlot{})?;
        }
        errdefer {
            if (reused_slot) {
                _.free_slots.append(_.alloc, slot_idx) catch {};
            } else {
                * = _.slots.pop();
            }
        }

        let filled_terms: usize = 0;
        errdefer {
            for (k of 0..filled_terms) {
                const t = doc_terms[k];
                if (p_entry := _.postings.getEntry(t)) {
                    let list = p_entry.value_ptr;
                    let idx: usize = 0;
                    while (idx < list.items.len) {
                        if (list.items[idx].doc_slot == slot_idx) {
                            * = list.swapRemove(idx);
                            break;
                        }
                        idx += 1;
                    }
                    if (list.items.len == 0) {
                        const key = p_entry.key_ptr.*;
                        list.deinit(_.alloc);
                        * = _.postings.remove(key);
                        _.alloc.free(key);
                    }
                }
            }
        }

        let tc_iter = term_counts.iterator();
        while (entry := tc_iter.next()) {
            const term = entry.key_ptr.*;
            const tf = entry.value_ptr.*;
            if (p_entry := _.postings.getEntry(term)) {
                p_entry.value_ptr.append(_.alloc, Posting{
                    doc_slot: slot_idx,
                    tf: tf,
                })?;
                doc_terms[filled_terms] = p_entry.key_ptr.*;
                filled_terms += 1;
            } else {
                const owned_term = _.alloc.dupe(u8, term)?;
                errdefer _.alloc.free(owned_term);

                let new_list: std.ArrayList(Posting) = .empty;
                errdefer new_list.deinit(_.alloc);

                new_list.append(_.alloc, Posting{
                    doc_slot: slot_idx,
                    tf: tf,
                })?;
                _.postings.put(_.alloc, owned_term, new_list)?;
                doc_terms[filled_terms] = owned_term;
                filled_terms += 1;
            }
        }

        _.id_to_slot.put(_.alloc, owned_id, slot_idx)?;
        _.slots.items[slot_idx] = DocSlot{
            active: true,
            id: owned_id,
            doc_len: doc_len,
            terms: doc_terms,
        };
        _.active_docs += 1;
        _.total_doc_len += doc_len;
    }

    /// Removes document `id` from the index and cleans up empty posting lists.
    /// Returns `true` if removed, `false` if not found.
    pub fn removeDocument(*_, id: string): bool {
        const slot_idx = _.id_to_slot.get(id) ?? return false;
        * = _.id_to_slot.remove(id);

        const slot = _.slots.items[slot_idx];
        for (term of slot.terms) {
            if (p_entry := _.postings.getEntry(term)) {
                let list = p_entry.value_ptr;
                let idx: usize = 0;
                while (idx < list.items.len) {
                    if (list.items[idx].doc_slot == slot_idx) {
                        * = list.swapRemove(idx);
                        break;
                    }
                    idx += 1;
                }
                if (list.items.len == 0) {
                    const key = p_entry.key_ptr.*;
                    list.deinit(_.alloc);
                    * = _.postings.remove(key);
                    _.alloc.free(key);
                }
            }
        }

        if (slot.id.len > 0) _.alloc.free(slot.id);
        if (slot.terms.len > 0) _.alloc.free(slot.terms);
        _.slots.items[slot_idx] = DocSlot{};
        _.free_slots.append(_.alloc, slot_idx) catch {};

        _.active_docs -= 1;
        _.total_doc_len -= slot.doc_len;
        return true;
    }

    /// Searches for up to `top_k` documents matching `query` ranked by descending Okapi BM25 score.
    /// Caller owns the returned slice (`alloc.free(hits)`).
    pub fn search(
        alloc: std.mem.Allocator,
        query: string,
        top_k: usize,
    ): ([]SearchHit)! {
        if (top_k == 0 || _.active_docs == 0 || query.len == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let query_terms: std.StringHashMapUnmanaged(u32) = .empty;
        defer freeTokenMap(alloc, *query_terms);

        const q_len = tokenizeCount(alloc, query, _.config.use_stopwords, *query_terms)?;
        if (q_len == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let scores: std.AutoHashMapUnmanaged(usize, f64) = .empty;
        defer scores.deinit(alloc);

        const n_f = _.active_docs as f64;
        const avgdl: f64 = if (_.total_doc_len > 0) (_.total_doc_len as f64) / n_f else 1.0;
        const k1 = _.config.k1;
        const b = _.config.b;

        let q_iter = query_terms.iterator();
        while (q_entry := q_iter.next()) {
            const term = q_entry.key_ptr.*;
            const list_ptr = _.postings.getPtr(term) ?? continue;
            const df = list_ptr.items.len;
            if (df == 0) continue;

            const df_f = df as f64;
            const idf = @log(1.0 + (n_f - df_f + 0.5) / (df_f + 0.5));

            for (posting of list_ptr.items) {
                const slot = _.slots.items[posting.doc_slot];
                if (!slot.active) continue;

                const tf_f: f64 = @floatFromInt(posting.tf);
                const dl_f: f64 = @floatFromInt(slot.doc_len);
                const norm = 1.0 - b + b * (dl_f / avgdl);
                const tf_sat = (tf_f * (k1 + 1.0)) / (tf_f + k1 * norm);
                const contrib = idf * tf_sat;

                if (s_entry := scores.getEntry(posting.doc_slot)) {
                    s_entry.value_ptr.* += contrib;
                } else {
                    scores.put(alloc, posting.doc_slot, contrib)?;
                }
            }
        }

        if (scores.count() == 0) {
            return alloc.alloc(SearchHit, 0)?;
        }

        let candidates: std.ArrayList(SearchHit) = .empty;
        defer candidates.deinit(alloc);

        let sc_iter = scores.iterator();
        while (sc_entry := sc_iter.next()) {
            const slot_idx = sc_entry.key_ptr.*;
            const slot = _.slots.items[slot_idx];
            candidates.append(alloc, SearchHit{
                id: slot.id,
                score: sc_entry.value_ptr.* as f32,
                meta: "",
            })?;
        }

        bm25SortHitsDesc(candidates.items);
        const out_len = if (top_k < candidates.items.len) top_k else candidates.items.len;
        let out = alloc.alloc(SearchHit, out_len)?;
        @memcpy(out, candidates.items[0..out_len]);
        return out;
    }
}

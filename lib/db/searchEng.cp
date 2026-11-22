// Source: cp/lib/search/engine.cp (Multi-Field BM25F, Boolean/Phrase Query & Snippet Highlighting Engine)
// Pure CP full-text search engine with multi-field BM25F weighting (title/body),
// boolean operators (+must, -exclude, AND, OR, NOT), exact phrase matching ("..."),
// dense-window keyword snippet highlighting, and integrated PrefixTrie autocomplete.

const std = @import("std");
import { SearchError, PrefixSuggestion, PrefixTrie } from "./searchTrie.cp";

/// Search result entry returned by `SearchIndex.search`.
/// `doc_id` borrows from the index; `snippet` is owned by the caller's allocator
/// (use `SearchIndex.freeHits(alloc, hits)` to release both snippets and slice).
pub struct SearchHit {
    doc_id: string = "";
    score: f64 = 0.0;
    snippet: string = "";
}

struct FieldPosting {
    doc_slot: usize = 0;
    title_tf: u32 = 0;
    body_tf: u32 = 0;
}

struct TokenSpan {
    offset: usize = 0;
    len: usize = 0;
    is_title: bool = false;
}

struct DocSlot {
    active: bool = false;
    doc_id: []u8 = [];
    title: []u8 = [];
    body: []u8 = [];
    title_len: usize = 0;
    body_len: usize = 0;
    tokens_buf: []u8 = [];
    token_spans: []TokenSpan = [];
    unique_terms: []string = [];
    unique_tfs: []u32 = [];
}

enum ClauseKind {
    Should,
    Must,
    MustNot,
}

struct QueryClause {
    kind: ClauseKind = .Should;
    is_phrase: bool = false;
    terms: std.ArrayList(string) = .empty;
}

fn isAsciiAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
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

fn hitBetterDesc(a: SearchHit, b: SearchHit): bool {
    if (a.score != b.score) return a.score > b.score;
    return std.mem.order(u8, a.doc_id, b.doc_id) == .lt;
}

fn siftDownWorstHit(items: []SearchHit, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && hitBetterDesc(items[worst], items[right])) {
            worst = right;
        }
        if (!hitBetterDesc(items[idx], items[worst])) break;
        const tmp = items[idx];
        items[idx] = items[worst];
        items[worst] = tmp;
        idx = worst;
    }
}

fn sortHitsDesc(items: []SearchHit) {
    if (items.len <= 1) return;
    const n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstHit(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownWorstHit(items, 0, end);
    }
}

struct FieldTfPair {
    title_tf: u32 = 0;
    body_tf: u32 = 0;
}

fn freeFieldTfMap(alloc: std.mem.Allocator, map: *std.StringHashMapUnmanaged(FieldTfPair)) {
    let iter = map.iterator();
    while (entry := iter.next()) {
        if (entry.key_ptr.*.len > 0) {
            alloc.free(entry.key_ptr.*);
        }
    }
    map.deinit(alloc);
}

fn freeClauses(alloc: std.mem.Allocator, clauses: *std.ArrayList(QueryClause)) {
    for (*c of clauses.items) {
        for (t of c.terms.items) {
            if (t.len > 0) alloc.free(t);
        }
        c.terms.deinit(alloc);
    }
    clauses.deinit(alloc);
}

fn tokenizeField(
    alloc: std.mem.Allocator,
    text: string,
    is_title: bool,
    use_stopwords: bool,
    tokens_buf: *std.ArrayList(u8),
    spans: *std.ArrayList(TokenSpan),
    tf_map: *std.StringHashMapUnmanaged(FieldTfPair),
): usize! {
    let count: usize = 0;
    let scratch: std.ArrayList(u8) = .empty;
    defer scratch.deinit(alloc);

    let i: usize = 0;
    while (i <= text.len) : (i += 1) {
        const at_end = (i == text.len);
        if (!at_end && isAsciiAlnum(text[i])) {
            scratch.append(alloc, toLowerAscii(text[i]))?;
            continue;
        }
        if (scratch.items.len > 0) {
            const tok = scratch.items;
            if (!use_stopwords || !isStopword(tok)) {
                count += 1;
                const off = tokens_buf.items.len;
                tokens_buf.appendSlice(alloc, tok)?;
                spans.append(alloc, TokenSpan{
                    offset: off,
                    len: tok.len,
                    is_title: is_title,
                })?;

                if (entry := tf_map.getEntry(tok)) {
                    if (is_title) {
                        entry.value_ptr.title_tf += 1;
                    } else {
                        entry.value_ptr.body_tf += 1;
                    }
                } else {
                    const owned_tok = alloc.dupe(u8, tok)?;
                    errdefer alloc.free(owned_tok);
                    const pair = if (is_title)
                        FieldTfPair{ title_tf: 1, body_tf: 0 }
                    else
                        FieldTfPair{ title_tf: 0, body_tf: 1 };
                    tf_map.put(alloc, owned_tok, pair)?;
                }
            }
            scratch.clearRetainingCapacity();
        }
    }
    return count;
}

fn tokenizeQuerySlice(
    alloc: std.mem.Allocator,
    raw: string,
    out_terms: *std.ArrayList(string),
): ! {
    let scratch: std.ArrayList(u8) = .empty;
    defer scratch.deinit(alloc);

    let i: usize = 0;
    while (i <= raw.len) : (i += 1) {
        const at_end = (i == raw.len);
        if (!at_end && isAsciiAlnum(raw[i])) {
            scratch.append(alloc, toLowerAscii(raw[i]))?;
            continue;
        }
        if (scratch.items.len > 0) {
            const owned = alloc.dupe(u8, scratch.items)?;
            errdefer alloc.free(owned);
            out_terms.append(alloc, owned)?;
            scratch.clearRetainingCapacity();
        }
    }
}

fn parseQuery(
    alloc: std.mem.Allocator,
    query: string,
    clauses: *std.ArrayList(QueryClause),
): ! {
    let i: usize = 0;
    const n = query.len;
    let pending_and = false;
    let pending_not = false;
    let saw_operator = false;

    while (i < n) {
        while (i < n && isWhitespace(query[i])) {
            i += 1;
        }
        if (i >= n) break;

        let prefix_kind: ClauseKind? = null;
        if (query[i] == '+') {
            prefix_kind = .Must;
            i += 1;
            saw_operator = true;
        } else if (query[i] == '-') {
            prefix_kind = .MustNot;
            i += 1;
            saw_operator = true;
        }

        if (i >= n || isWhitespace(query[i])) {
            return SearchError.InvalidQuery;
        }

        if (query[i] == '"') {
            i += 1;
            const start = i;
            while (i < n && query[i] != '"') {
                i += 1;
            }
            if (i >= n) {
                return SearchError.InvalidQuery;
            }
            const phrase_raw = query[start..i];
            i += 1; // consume closing quote

            let phrase_terms: std.ArrayList(string) = .empty;
            errdefer {
                for (t of phrase_terms.items) alloc.free(t);
                phrase_terms.deinit(alloc);
            }
            tokenizeQuerySlice(alloc, phrase_raw, *phrase_terms)?;
            if (phrase_terms.items.len == 0) {
                return SearchError.InvalidQuery;
            }

            let kind: ClauseKind = .Must;
            if (prefix_kind != null) {
                kind = prefix_kind.?;
            } else if (pending_not) {
                kind = .MustNot;
            }
            pending_and = false;
            pending_not = false;

            clauses.append(alloc, QueryClause{
                kind: kind,
                is_phrase: true,
                terms: phrase_terms,
            })?;
            continue;
        }

        const tok_start = i;
        while (i < n && !isWhitespace(query[i]) && query[i] != '"') {
            i += 1;
        }
        const raw_word = query[tok_start..i];

        if (prefix_kind == null) {
            if (std.mem.eql(u8, raw_word, "AND")) {
                if (clauses.items.len == 0 || pending_and || pending_not) {
                    return SearchError.InvalidQuery;
                }
                const last_idx = clauses.items.len - 1;
                if (clauses.items[last_idx].kind == .Should) {
                    clauses.items[last_idx].kind = .Must;
                }
                pending_and = true;
                saw_operator = true;
                continue;
            }
            if (std.mem.eql(u8, raw_word, "OR")) {
                if (clauses.items.len == 0 || pending_and || pending_not) {
                    return SearchError.InvalidQuery;
                }
                saw_operator = true;
                continue;
            }
            if (std.mem.eql(u8, raw_word, "NOT")) {
                if (pending_not) return SearchError.InvalidQuery;
                pending_not = true;
                saw_operator = true;
                continue;
            }
        }

        let sub_terms: std.ArrayList(string) = .empty;
        defer {
            for (t of sub_terms.items) alloc.free(t);
            sub_terms.deinit(alloc);
        }
        tokenizeQuerySlice(alloc, raw_word, *sub_terms)?;
        if (sub_terms.items.len == 0) {
            if (prefix_kind != null) return SearchError.InvalidQuery;
            continue;
        }

        let base_kind: ClauseKind = .Should;
        if (prefix_kind != null) {
            base_kind = prefix_kind.?;
        } else if (pending_not) {
            base_kind = .MustNot;
        } else if (pending_and) {
            base_kind = .Must;
        }
        pending_and = false;
        pending_not = false;

        for (st of sub_terms.items) {
            let single: std.ArrayList(string) = .empty;
            errdefer {
                for (t of single.items) alloc.free(t);
                single.deinit(alloc);
            }
            const duped = alloc.dupe(u8, st)?;
            single.append(alloc, duped)?;
            clauses.append(alloc, QueryClause{
                kind: base_kind,
                is_phrase: false,
                terms: single,
            })?;
        }
    }

    if (pending_and || pending_not) {
        return SearchError.InvalidQuery;
    }
    if (saw_operator && clauses.items.len == 0) {
        return SearchError.InvalidQuery;
    }
}

fn slotContainsPhrase(slot: DocSlot, phrase_terms: []const string): bool {
    if (phrase_terms.len == 0 || slot.token_spans.len < phrase_terms.len) return false;
    const max_start = slot.token_spans.len - phrase_terms.len;
    let i: usize = 0;
    while (i <= max_start) : (i += 1) {
        const first_is_title = slot.token_spans[i].is_title;
        let matched = true;
        for (j of 0..phrase_terms.len) {
            const sp = slot.token_spans[i + j];
            if (sp.is_title != first_is_title) {
                matched = false;
                break;
            }
            const tok = slot.tokens_buf[sp.offset..(sp.offset + sp.len)];
            if (!std.mem.eql(u8, tok, phrase_terms[j])) {
                matched = false;
                break;
            }
        }
        if (matched) return true;
    }
    return false;
}

fn slotContainsTerm(slot: DocSlot, term: string): bool {
    for (t of slot.unique_terms) {
        if (std.mem.eql(u8, t, term)) return true;
    }
    return false;
}

struct TextTokenPos {
    start: usize = 0;
    end: usize = 0;
    matched_term_idx: usize? = null;
}

fn collectTextTokens(
    alloc: std.mem.Allocator,
    text: string,
    lower_terms: []const []u8,
    out_tokens: *std.ArrayList(TextTokenPos),
): ! {
    let scratch: std.ArrayList(u8) = .empty;
    defer scratch.deinit(alloc);

    let tok_start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) : (i += 1) {
        const at_end = (i == text.len);
        if (!at_end && isAsciiAlnum(text[i])) {
            if (scratch.items.len == 0) {
                tok_start = i;
            }
            scratch.append(alloc, toLowerAscii(text[i]))?;
            continue;
        }
        if (scratch.items.len > 0) {
            let match_idx: usize? = null;
            for (ti of 0..lower_terms.len) {
                if (std.mem.eql(u8, scratch.items, lower_terms[ti])) {
                    match_idx = ti;
                    break;
                }
            }
            out_tokens.append(alloc, TextTokenPos{
                start: tok_start,
                end: i,
                matched_term_idx: match_idx,
            })?;
            scratch.clearRetainingCapacity();
        }
    }
}

/// Multi-field BM25F inverted index with boolean/phrase query parsing,
/// snippet highlighting, and integrated `PrefixTrie` autocomplete.
pub struct SearchIndex {
    alloc: std.mem.Allocator = undefined;
    title_weight: f64 = 3.0;
    body_weight: f64 = 1.0;
    k1: f64 = 1.5;
    b: f64 = 0.75;
    use_stopwords: bool = false;
    slots: std.ArrayList(DocSlot) = .empty;
    free_slots: std.ArrayList(usize) = .empty;
    id_to_slot: std.StringHashMapUnmanaged(usize) = .empty;
    postings: std.StringHashMapUnmanaged(std.ArrayList(FieldPosting)) = .empty;
    trie: PrefixTrie = undefined;
    active_docs: usize = 0;
    total_title_len: usize = 0;
    total_body_len: usize = 0;

    /// Initializes an empty `SearchIndex` with default field weights (`title = 3.0`, `body = 1.0`).
    pub static fn init(alloc: std.mem.Allocator): SearchIndex {
        return SearchIndex{
            alloc: alloc,
            title_weight: 3.0,
            body_weight: 1.0,
            k1: 1.5,
            b: 0.75,
            use_stopwords: false,
            slots: .empty,
            free_slots: .empty,
            id_to_slot: .empty,
            postings: .empty,
            trie: PrefixTrie.init(alloc),
            active_docs: 0,
            total_title_len: 0,
            total_body_len: 0,
        };
    }

    /// Releases all documents, postings, and the internal `PrefixTrie`.
    pub fn deinit(*_) {
        _.id_to_slot.deinit(_.alloc);

        for (slot of _.slots.items) {
            if (slot.active) {
                if (slot.doc_id.len > 0) _.alloc.free(slot.doc_id);
                if (slot.title.len > 0) _.alloc.free(slot.title);
                if (slot.body.len > 0) _.alloc.free(slot.body);
                if (slot.tokens_buf.len > 0) _.alloc.free(slot.tokens_buf);
                if (slot.token_spans.len > 0) _.alloc.free(slot.token_spans);
                if (slot.unique_terms.len > 0) _.alloc.free(slot.unique_terms);
                if (slot.unique_tfs.len > 0) _.alloc.free(slot.unique_tfs);
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
        _.trie.deinit();
        _.active_docs = 0;
        _.total_title_len = 0;
        _.total_body_len = 0;
    }

    /// Frees a `[]SearchHit` slice and all owned `snippet` strings returned by `search`.
    pub static fn freeHits(alloc: std.mem.Allocator, hits: []SearchHit) {
        for (h of hits) {
            if (h.snippet.len > 0) alloc.free(h.snippet);
        }
        if (hits.len > 0) alloc.free(hits);
    }

    /// Configures field weights for BM25F scoring (defaults: `title_weight = 3.0`, `body_weight = 1.0`).
    pub fn setFieldWeights(*_, title_weight: f64, body_weight: f64) {
        _.title_weight = title_weight;
        _.body_weight = body_weight;
    }

    /// Enables or disables English stopword filtering during indexing.
    pub fn setUseStopwords(*_, enabled: bool) {
        _.use_stopwords = enabled;
    }

    /// Returns the number of active indexed documents.
    pub fn docCount(): usize {
        return _.active_docs;
    }

    /// Indexes a new document `(doc_id, title, body)`.
    /// Returns `SearchError.DuplicateDocId` if `doc_id` already exists.
    pub fn addDocument(*_, doc_id: string, title: string, body: string): ! {
        if (_.id_to_slot.contains(doc_id)) {
            return SearchError.DuplicateDocId;
        }

        let tf_map: std.StringHashMapUnmanaged(FieldTfPair) = .empty;
        defer freeFieldTfMap(_.alloc, *tf_map);

        let tokens_buf: std.ArrayList(u8) = .empty;
        errdefer tokens_buf.deinit(_.alloc);

        let spans: std.ArrayList(TokenSpan) = .empty;
        errdefer spans.deinit(_.alloc);

        const title_len = tokenizeField(_.alloc, title, true, _.use_stopwords, *tokens_buf, *spans, *tf_map)?;
        const body_len = tokenizeField(_.alloc, body, false, _.use_stopwords, *tokens_buf, *spans, *tf_map)?;

        const owned_id: []u8 = if (doc_id.len > 0) _.alloc.dupe(u8, doc_id)? else [];
        errdefer if (owned_id.len > 0) _.alloc.free(owned_id);

        const owned_title: []u8 = if (title.len > 0) _.alloc.dupe(u8, title)? else [];
        errdefer if (owned_title.len > 0) _.alloc.free(owned_title);

        const owned_body: []u8 = if (body.len > 0) _.alloc.dupe(u8, body)? else [];
        errdefer if (owned_body.len > 0) _.alloc.free(owned_body);

        const owned_tokens_buf = tokens_buf.toOwnedSlice(_.alloc)?;
        errdefer if (owned_tokens_buf.len > 0) _.alloc.free(owned_tokens_buf);

        const owned_spans = spans.toOwnedSlice(_.alloc)?;
        errdefer if (owned_spans.len > 0) _.alloc.free(owned_spans);

        const uniq_n = tf_map.count();
        let uniq_terms = _.alloc.alloc(string, uniq_n)?;
        errdefer _.alloc.free(uniq_terms);

        let uniq_tfs = _.alloc.alloc(u32, uniq_n)?;
        errdefer _.alloc.free(uniq_tfs);

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

        let filled: usize = 0;
        errdefer {
            for (k of 0..filled) {
                const t = uniq_terms[k];
                _.trie.decrement(t, uniq_tfs[k]);
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
                        const k_ptr = p_entry.key_ptr.*;
                        list.deinit(_.alloc);
                        * = _.postings.remove(k_ptr);
                        if (k_ptr.len > 0) _.alloc.free(k_ptr);
                    }
                }
            }
        }

        let iter = tf_map.iterator();
        while (entry := iter.next()) {
            const term_key = entry.key_ptr.*;
            const pair = entry.value_ptr.*;
            const total_tf = pair.title_tf + pair.body_tf;

            if (p_entry := _.postings.getEntry(term_key)) {
                p_entry.value_ptr.append(_.alloc, FieldPosting{
                    doc_slot: slot_idx,
                    title_tf: pair.title_tf,
                    body_tf: pair.body_tf,
                })?;
                uniq_terms[filled] = p_entry.key_ptr.*;
            } else {
                const owned_term = _.alloc.dupe(u8, term_key)?;
                errdefer _.alloc.free(owned_term);
                let plist: std.ArrayList(FieldPosting) = .empty;
                errdefer plist.deinit(_.alloc);
                plist.append(_.alloc, FieldPosting{
                    doc_slot: slot_idx,
                    title_tf: pair.title_tf,
                    body_tf: pair.body_tf,
                })?;
                _.postings.put(_.alloc, owned_term, plist)?;
                uniq_terms[filled] = owned_term;
            }

            uniq_tfs[filled] = total_tf;
            _.trie.insert(uniq_terms[filled], total_tf)?;
            filled += 1;
        }

        _.id_to_slot.put(_.alloc, owned_id, slot_idx)?;

        _.slots.items[slot_idx] = DocSlot{
            active: true,
            doc_id: owned_id,
            title: owned_title,
            body: owned_body,
            title_len: title_len,
            body_len: body_len,
            tokens_buf: owned_tokens_buf,
            token_spans: owned_spans,
            unique_terms: uniq_terms,
            unique_tfs: uniq_tfs,
        };
        _.active_docs += 1;
        _.total_title_len += title_len;
        _.total_body_len += body_len;
    }

    /// Removes document `doc_id` from the inverted index and decrements its terms in `PrefixTrie`.
    /// Returns `true` if the document existed and was removed.
    pub fn removeDocument(*_, doc_id: string): bool {
        const kv = _.id_to_slot.fetchRemove(doc_id) ?? return false;
        const slot_idx = kv.value;
        const slot = _.slots.items[slot_idx];

        for (i of 0..slot.unique_terms.len) {
            const term = slot.unique_terms[i];
            const tf = slot.unique_tfs[i];
            _.trie.decrement(term, tf);

            if (p_entry := _.postings.getEntry(term)) {
                let list = p_entry.value_ptr;
                let j: usize = 0;
                while (j < list.items.len) {
                    if (list.items[j].doc_slot == slot_idx) {
                        * = list.swapRemove(j);
                        break;
                    }
                    j += 1;
                }
                if (list.items.len == 0) {
                    const key_slice = p_entry.key_ptr.*;
                    list.deinit(_.alloc);
                    * = _.postings.remove(key_slice);
                    if (key_slice.len > 0) _.alloc.free(key_slice);
                }
            }
        }

        if (_.active_docs > 0) _.active_docs -= 1;
        _.total_title_len -= slot.title_len;
        _.total_body_len -= slot.body_len;

        if (slot.doc_id.len > 0) _.alloc.free(slot.doc_id);
        if (slot.title.len > 0) _.alloc.free(slot.title);
        if (slot.body.len > 0) _.alloc.free(slot.body);
        if (slot.tokens_buf.len > 0) _.alloc.free(slot.tokens_buf);
        if (slot.token_spans.len > 0) _.alloc.free(slot.token_spans);
        if (slot.unique_terms.len > 0) _.alloc.free(slot.unique_terms);
        if (slot.unique_tfs.len > 0) _.alloc.free(slot.unique_tfs);

        _.slots.items[slot_idx] = DocSlot{};
        _.free_slots.append(_.alloc, slot_idx) catch {};
        return true;
    }

    /// Inserts or replaces document `doc_id` with updated `title` and `body`.
    pub fn upsertDocument(*_, doc_id: string, title: string, body: string): ! {
        if (_.id_to_slot.contains(doc_id)) {
            * = _.removeDocument(doc_id);
        }
        _.addDocument(doc_id, title, body)?;
    }

    /// Delegates prefix autocomplete to the internal `PrefixTrie`.
    pub fn autocomplete(
        alloc: std.mem.Allocator,
        prefix: string,
        limit: usize,
    ): ([]PrefixSuggestion)! {
        return _.trie.suggest(alloc, prefix, limit);
    }

    /// Extracts a dense keyword-rich window of up to `max_len` bytes from `text`,
    /// wrapping matched terms with `open_tag` and `close_tag`.
    /// Caller owns the returned `[]u8` slice and must free it with `alloc.free(snippet)`.
    pub fn highlightSnippet(
        alloc: std.mem.Allocator,
        text: string,
        terms: []const string,
        max_len: usize,
        open_tag: string,
        close_tag: string,
    ): ([]u8)! {
        if (text.len == 0 || max_len == 0) return [];

        let lower_terms: std.ArrayList([]u8) = .empty;
        defer {
            for (lt of lower_terms.items) alloc.free(lt);
            lower_terms.deinit(alloc);
        }

        for (t of terms) {
            if (t.len == 0) continue;
            let buf = alloc.alloc(u8, t.len)?;
            for (k of 0..t.len) {
                buf[k] = toLowerAscii(t[k]);
            }
            let dup = false;
            for (existing of lower_terms.items) {
                if (std.mem.eql(u8, existing, buf)) {
                    dup = true;
                    break;
                }
            }
            if (dup) {
                alloc.free(buf);
            } else {
                lower_terms.append(alloc, buf)?;
            }
        }

        let tokens: std.ArrayList(TextTokenPos) = .empty;
        defer tokens.deinit(alloc);
        collectTextTokens(alloc, text, lower_terms.items, *tokens)?;

        let win_start: usize = 0;
        let win_end: usize = text.len;

        if (text.len > max_len) {
            let best_score: usize = 0;
            let best_start: usize = 0;
            let best_end: usize = max_len;
            let found_match = false;

            const term_count = lower_terms.items.len;
            let seen = alloc.alloc(bool, if (term_count > 0) term_count else 1)?;
            defer alloc.free(seen);

            for (idx of 0..tokens.items.len) {
                const tok = tokens.items[idx];
                if (tok.matched_term_idx == null) continue;
                found_match = true;

                // Center window slightly before the matched token for natural context
                const pad_left: usize = max_len / 4;
                let cand_start: usize = if (tok.start > pad_left) tok.start - pad_left else 0;
                // Align cand_start forward to a whitespace boundary if inside a word
                if (cand_start > 0 && isAsciiAlnum(text[cand_start]) && isAsciiAlnum(text[cand_start - 1])) {
                    while (cand_start < tok.start && !isWhitespace(text[cand_start])) {
                        cand_start += 1;
                    }
                    while (cand_start < tok.start && isWhitespace(text[cand_start])) {
                        cand_start += 1;
                    }
                }
                let cand_end: usize = @min(text.len, cand_start + max_len);
                if (cand_end < text.len && isAsciiAlnum(text[cand_end]) && isAsciiAlnum(text[cand_end - 1])) {
                    while (cand_end > tok.end && isAsciiAlnum(text[cand_end - 1])) {
                        cand_end -= 1;
                    }
                }

                @memset(seen, false);
                let distinct_hits: usize = 0;
                let total_hits: usize = 0;
                let j = idx;
                while (j < tokens.items.len) : (j += 1) {
                    const t2 = tokens.items[j];
                    if (t2.start >= cand_end) break;
                    if (t2.start >= cand_start && t2.end <= cand_end) {
                        if (tidx := t2.matched_term_idx) {
                            total_hits += 1;
                            if (!seen[tidx]) {
                                seen[tidx] = true;
                                distinct_hits += 1;
                            }
                        }
                    }
                }

                const score = distinct_hits * 16 + total_hits;
                if (score > best_score) {
                    best_score = score;
                    best_start = cand_start;
                    best_end = cand_end;
                }
            }

            if (found_match) {
                win_start = best_start;
                win_end = best_end;
            } else {
                win_start = 0;
                win_end = max_len;
            }
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        if (win_start > 0) {
            out.appendSlice(alloc, "...")?;
        }

        let cursor = win_start;
        for (tok of tokens.items) {
            if (tok.end <= win_start) continue;
            if (tok.start >= win_end) break;
            if (tok.matched_term_idx == null) continue;
            if (tok.start < win_start || tok.end > win_end) continue;

            if (tok.start > cursor) {
                out.appendSlice(alloc, text[cursor..tok.start])?;
            }
            out.appendSlice(alloc, open_tag)?;
            out.appendSlice(alloc, text[tok.start..tok.end])?;
            out.appendSlice(alloc, close_tag)?;
            cursor = tok.end;
        }

        if (cursor < win_end) {
            out.appendSlice(alloc, text[cursor..win_end])?;
        }
        if (win_end < text.len) {
            out.appendSlice(alloc, "...")?;
        }

        return out.toOwnedSlice(alloc);
    }

    /// Searches the index using BM25F scoring, boolean operators (`+`, `-`, `AND`, `OR`, `NOT`),
    /// and exact phrase matching (`"exact phrase"`).
    /// Returns up to `limit` hits sorted by score descending.
    /// Caller must release hits via `SearchIndex.freeHits(alloc, hits)`.
    pub fn search(
        alloc: std.mem.Allocator,
        query: string,
        limit: usize,
    ): ([]SearchHit)! {
        let clauses: std.ArrayList(QueryClause) = .empty;
        defer freeClauses(alloc, *clauses);
        parseQuery(alloc, query, *clauses)?;

        if (limit == 0 || _.active_docs == 0 || clauses.items.len == 0) {
            return [];
        }

        let has_must = false;
        let has_should = false;
        for (c of clauses.items) {
            if (c.kind == .Must) has_must = true;
            if (c.kind == .Should) has_should = true;
        }
        // Only negative clauses without any positive condition cannot select documents
        if (!has_must && !has_should) {
            return [];
        }

        // Collect unique positive scoring terms for BM25F & snippet highlighting
        let pos_terms: std.ArrayList(string) = .empty;
        defer pos_terms.deinit(alloc);
        for (c of clauses.items) {
            if (c.kind == .MustNot) continue;
            for (t of c.terms.items) {
                let dup = false;
                for (pt of pos_terms.items) {
                    if (std.mem.eql(u8, pt, t)) {
                        dup = true;
                        break;
                    }
                }
                if (!dup) {
                    pos_terms.append(alloc, t)?;
                }
            }
        }

        const n_docs = f64(_.active_docs);
        const avg_title = if (_.active_docs > 0) f64(_.total_title_len) / n_docs else 0.0;
        const avg_body = if (_.active_docs > 0) f64(_.total_body_len) / n_docs else 0.0;

        let scores = alloc.alloc(f64, _.slots.items.len)?;
        defer alloc.free(scores);
        @memset(scores, 0.0);

        let has_positive_hit = alloc.alloc(bool, _.slots.items.len)?;
        defer alloc.free(has_positive_hit);
        @memset(has_positive_hit, false);

        for (term of pos_terms.items) {
            const p_list_opt = _.postings.get(term);
            if (p_list_opt == null) continue;
            const p_list = p_list_opt.?;
            if (p_list.items.len == 0) continue;

            const df = f64(p_list.items.len);
            const idf = @log(1.0 + (n_docs - df + 0.5) / (df + 0.5));

            for (post of p_list.items) {
                const slot = _.slots.items[post.doc_slot];
                if (!slot.active) continue;

                let tf_title_norm: f64 = 0.0;
                if (post.title_tf > 0) {
                    const dl_t = f64(slot.title_len);
                    const denom_t = if (avg_title > 0.0)
                        (1.0 - _.b + _.b * (dl_t / avg_title))
                    else
                        1.0;
                    tf_title_norm = (f64(post.title_tf) * _.title_weight) / denom_t;
                }

                let tf_body_norm: f64 = 0.0;
                if (post.body_tf > 0) {
                    const dl_b = f64(slot.body_len);
                    const denom_b = if (avg_body > 0.0)
                        (1.0 - _.b + _.b * (dl_b / avg_body))
                    else
                        1.0;
                    tf_body_norm = (f64(post.body_tf) * _.body_weight) / denom_b;
                }

                const pseudo_tf = tf_title_norm + tf_body_norm;
                if (pseudo_tf > 0.0) {
                    const term_score = idf * ((pseudo_tf * (_.k1 + 1.0)) / (pseudo_tf + _.k1));
                    scores[post.doc_slot] += term_score;
                    has_positive_hit[post.doc_slot] = true;
                }
            }
        }

        let candidates: std.ArrayList(SearchHit) = .empty;
        errdefer {
            for (h of candidates.items) {
                if (h.snippet.len > 0) alloc.free(h.snippet);
            }
            candidates.deinit(alloc);
        }

        for (slot_idx of 0.._.slots.items.len) {
            if (!has_positive_hit[slot_idx]) continue;
            const slot = _.slots.items[slot_idx];
            if (!slot.active) continue;

            // Evaluate boolean & phrase constraints
            let ok = true;
            for (c of clauses.items) {
                if (c.kind == .Should) continue;
                let matched = false;
                if (c.is_phrase) {
                    matched = slotContainsPhrase(slot, c.terms.items);
                } else {
                    matched = slotContainsTerm(slot, c.terms.items[0]);
                }

                if (c.kind == .Must && !matched) {
                    ok = false;
                    break;
                }
                if (c.kind == .MustNot && matched) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            candidates.append(alloc, SearchHit{
                doc_id: slot.doc_id,
                score: scores[slot_idx],
                snippet: "",
            })?;
        }

        if (candidates.items.len == 0) {
            candidates.deinit(alloc);
            return [];
        }

        sortHitsDesc(candidates.items);
        const out_len = @min(limit, candidates.items.len);
        let out = alloc.alloc(SearchHit, out_len)?;
        let built: usize = 0;
        errdefer {
            for (k of 0..built) {
                if (out[k].snippet.len > 0) alloc.free(out[k].snippet);
            }
            alloc.free(out);
        }

        for (i of 0..out_len) {
            const c = candidates.items[i];
            const slot_idx = _.id_to_slot.get(c.doc_id).?;
            const slot = _.slots.items[slot_idx];
            const source_text = if (slot.body.len > 0) slot.body else slot.title;
            const snip = _.highlightSnippet(
                alloc,
                source_text,
                pos_terms.items,
                160,
                "<mark>",
                "</mark>",
            )?;
            out[i] = SearchHit{
                doc_id: c.doc_id,
                score: c.score,
                snippet: snip,
            };
            built += 1;
        }

        candidates.deinit(alloc);
        return out;
    }
}

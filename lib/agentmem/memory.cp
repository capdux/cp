// Source: cp/lib/agentmem/memory.cp (Hierarchical Agent Memory Engine)
// Pure CP, zero-C-dependency three-tier long-term memory engine (Working / Episodic / Semantic)
// with Stanford Generative Agents three-factor recall (Recency + Importance + Relevance),
// memory consolidation, forgetting/pruning, and JSONL persistence roundtrip.

const std = @import("std");

/// Error set for hierarchical agent memory operations.
pub err AgentMemError {
    MemoryNotFound,
    InvalidImportance,
    InvalidCapacity,
    InvalidJsonl,
    OutOfMemory,
}

/// Three-tier memory hierarchy model.
pub enum MemoryTier {
    Working,
    Episodic,
    Semantic,

    fn toStr(): string {
        return switch (_) {
            .Working => "Working",
            .Episodic => "Episodic",
            .Semantic => "Semantic",
        };
    }

    static fn fromStr(s: string): MemoryTier? {
        if (std.mem.eql(u8, s, "Working")) return .Working;
        if (std.mem.eql(u8, s, "Episodic")) return .Episodic;
        if (std.mem.eql(u8, s, "Semantic")) return .Semantic;
        return null;
    }
}

/// Weighting and exponential decay configuration for three-factor recall.
pub struct RecallWeights {
    recency_weight: f64 = 1.0;
    importance_weight: f64 = 1.0;
    relevance_weight: f64 = 1.0;
    decay_half_life_ms: f64 = 3600000.0;
}

/// Read-only view of a stored memory item.
pub struct MemoryItem {
    id: u64 = 0;
    tier: MemoryTier = .Working;
    content: string = "";
    tags: []string = [];
    importance: f64 = 0.0;
    created_at_ms: u64 = 0;
    last_accessed_ms: u64 = 0;
    access_count: u32 = 0;
}

/// Scored recall result returned by `recall` and `recallByTier`.
pub struct RecallHit {
    id: u64 = 0;
    tier: MemoryTier = .Working;
    content: string = "";
    total_score: f64 = 0.0;
    recency_score: f64 = 0.0;
    importance_score: f64 = 0.0;
    relevance_score: f64 = 0.0;
}

struct StoredMemory {
    id: u64 = 0;
    tier: MemoryTier = .Working;
    content: []u8 = [];
    tags: [][]u8 = [];
    tag_views: []string = [];
    importance: f64 = 0.0;
    created_at_ms: u64 = 0;
    last_accessed_ms: u64 = 0;
    access_count: u32 = 0;

    fn freeOwned(*_, alloc: std.mem.Allocator) {
        if (_.content.len > 0) alloc.free(_.content);
        for (t of _.tags) {
            if (t.len > 0) alloc.free(t);
        }
        if (_.tags.len > 0) alloc.free(_.tags);
        if (_.tag_views.len > 0) alloc.free(_.tag_views);
    }

    fn toView(): MemoryItem {
        return MemoryItem{
            id: _.id,
            tier: _.tier,
            content: _.content,
            tags: _.tag_views,
            importance: _.importance,
            created_at_ms: _.created_at_ms,
            last_accessed_ms: _.last_accessed_ms,
            access_count: _.access_count,
        };
    }
}

struct ScoredCandidate {
    item_idx: usize = 0;
    last_accessed_ms: u64 = 0;
    hit: RecallHit = RecallHit{},
}

const LN2: f64 = 0.6931471805599453;

fn isAsciiAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

fn toAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn computeRecencyScore(now_ms: u64, last_accessed_ms: u64, half_life_ms: f64): f64 {
    const dt: f64 = if (now_ms > last_accessed_ms) f64(now_ms - last_accessed_ms) else 0.0;
    if (dt == 0.0) return 1.0;
    if (half_life_ms <= 0.0) return 0.0;
    return std.math.exp(-LN2 * dt / half_life_ms);
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

fn tokenizeIntoMap(
    alloc: std.mem.Allocator,
    text: string,
    map: *std.StringHashMapUnmanaged(u32),
): u32! {
    let total: u32 = 0;
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
            total += 1;
            const tok = scratch.items;
            if (entry := map.getEntry(tok)) {
                entry.value_ptr.* += 1;
            } else {
                const owned_tok = alloc.dupe(u8, tok)?;
                errdefer alloc.free(owned_tok);
                map.put(alloc, owned_tok, 1)?;
            }
            scratch.clearRetainingCapacity();
        }
    }
    return total;
}

fn computeRelevanceScore(
    alloc: std.mem.Allocator,
    query_tokens: *const std.StringHashMapUnmanaged(u32),
    content: string,
    tags: []const string,
): f64! {
    const q_unique = query_tokens.count();
    if (q_unique == 0) return 0.0;

    let doc_tokens: std.StringHashMapUnmanaged(u32) = .empty;
    defer freeTokenMap(alloc, *doc_tokens);

    * = tokenizeIntoMap(alloc, content, *doc_tokens)?;

    let tag_tokens: std.StringHashMapUnmanaged(u32) = .empty;
    defer freeTokenMap(alloc, *tag_tokens);

    for (t of tags) {
        * = tokenizeIntoMap(alloc, t, *doc_tokens)?;
        * = tokenizeIntoMap(alloc, t, *tag_tokens)?;
    }

    const d_unique = doc_tokens.count();
    if (d_unique == 0) return 0.0;

    let intersection: usize = 0;
    let tf_hits: u32 = 0;
    let tag_hits: usize = 0;

    let q_iter = query_tokens.iterator();
    while (q_entry := q_iter.next()) {
        const q_term = q_entry.key_ptr.*;
        if (d_tf := doc_tokens.get(q_term)) {
            intersection += 1;
            tf_hits += d_tf;
        }
        if (tag_tokens.contains(q_term)) {
            tag_hits += 1;
        }
    }

    if (intersection == 0) return 0.0;

    const union_count: usize = q_unique + d_unique - intersection;
    const jaccard: f64 = @as(f64, @floatFromInt(intersection)) / @as(f64, @floatFromInt(union_count));
    const coverage: f64 = @as(f64, @floatFromInt(intersection)) / @as(f64, @floatFromInt(q_unique));
    const tf_sat: f64 = @as(f64, @floatFromInt(tf_hits)) / (@as(f64, @floatFromInt(tf_hits)) + 1.0);
    const tag_bonus: f64 = (@as(f64, @floatFromInt(tag_hits)) / @as(f64, @floatFromInt(q_unique))) * 0.15;

    let combined: f64 = 0.45 * jaccard + 0.35 * coverage + 0.20 * tf_sat + tag_bonus;
    if (combined > 1.0) combined = 1.0;
    return combined;
}

fn candidateBetterDesc(a: ScoredCandidate, b: ScoredCandidate): bool {
    if (a.hit.total_score != b.hit.total_score) return a.hit.total_score > b.hit.total_score;
    if (a.last_accessed_ms != b.last_accessed_ms) return a.last_accessed_ms > b.last_accessed_ms;
    return a.hit.id < b.hit.id;
}

fn siftDownWorstCandidate(items: []ScoredCandidate, start: usize, len: usize) {
    let idx = start;
    while (true) {
        const left = idx * 2 + 1;
        if (left >= len) break;
        const right = left + 1;
        let worst = left;
        if (right < len && candidateBetterDesc(items[worst], items[right])) {
            worst = right;
        }
        if (!candidateBetterDesc(items[idx], items[worst])) break;
        const tmp = items[idx];
        items[idx] = items[worst];
        items[worst] = tmp;
        idx = worst;
    }
}

fn sortCandidatesDesc(items: []ScoredCandidate) {
    if (items.len <= 1) return;
    let n = items.len;
    let i: usize = n / 2;
    while (i > 0) {
        i -= 1;
        siftDownWorstCandidate(items, i, n);
    }
    let end: usize = n;
    while (end > 1) {
        end -= 1;
        const tmp = items[0];
        items[0] = items[end];
        items[end] = tmp;
        siftDownWorstCandidate(items, 0, end);
    }
}

fn appendJsonEscaped(alloc: std.mem.Allocator, out: *std.ArrayList(u8), s: string)! {
    out.append(alloc, '"')?;
    for (c of s) {
        switch (c) {
            '"' => out.appendSlice(alloc, "\\\"")?,
            '\\' => out.appendSlice(alloc, "\\\\")?,
            '\n' => out.appendSlice(alloc, "\\n")?,
            '\r' => out.appendSlice(alloc, "\\r")?,
            '\t' => out.appendSlice(alloc, "\\t")?,
            else => {
                if (c < 0x20) {
                    const hex = "0123456789abcdef";
                    let esc: [6]u8 = ['\\', 'u', '0', '0', hex[(c >> 4) & 0x0f], hex[c & 0x0f]];
                    out.appendSlice(alloc, &esc)?;
                } else {
                    out.append(alloc, c)?;
                }
            },
        }
    }
    out.append(alloc, '"')?;
}

struct JsonCursor {
    src: string = "";
    pos: usize = 0;

    fn skipWs(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                _.pos += 1;
            } else {
                break;
            }
        }
    }

    fn peek(): u8? {
        if (_.pos >= _.src.len) return null;
        return _.src[_.pos];
    }

    fn expectChar(*_, ch: u8): void ! AgentMemError {
        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != ch) {
            return AgentMemError.InvalidJsonl;
        }
        _.pos += 1;
    }

    fn parseString(*_, alloc: std.mem.Allocator): ([]u8) ! AgentMemError {
        _.skipWs();
        if (_.pos >= _.src.len || _.src[_.pos] != '"') {
            return AgentMemError.InvalidJsonl;
        }
        _.pos += 1;

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            _.pos += 1;
            if (c == '"') {
                return buf.toOwnedSlice(alloc) catch return AgentMemError.OutOfMemory;
            }
            if (c == '\\') {
                if (_.pos >= _.src.len) return AgentMemError.InvalidJsonl;
                const esc = _.src[_.pos];
                _.pos += 1;
                switch (esc) {
                    '"' => buf.append(alloc, '"') catch return AgentMemError.OutOfMemory,
                    '\\' => buf.append(alloc, '\\') catch return AgentMemError.OutOfMemory,
                    '/' => buf.append(alloc, '/') catch return AgentMemError.OutOfMemory,
                    'b' => buf.append(alloc, 0x08) catch return AgentMemError.OutOfMemory,
                    'f' => buf.append(alloc, 0x0c) catch return AgentMemError.OutOfMemory,
                    'n' => buf.append(alloc, '\n') catch return AgentMemError.OutOfMemory,
                    'r' => buf.append(alloc, '\r') catch return AgentMemError.OutOfMemory,
                    't' => buf.append(alloc, '\t') catch return AgentMemError.OutOfMemory,
                    'u' => {
                        if (_.pos + 4 > _.src.len) return AgentMemError.InvalidJsonl;
                        const cp = std.fmt.parseInt(u21, _.src[_.pos .. _.pos + 4], 16) catch {
                            return AgentMemError.InvalidJsonl;
                        };
                        _.pos += 4;
                        let utf8_buf: [4]u8 = [0; 4];
                        const utf8_len = std.unicode.utf8Encode(cp, &utf8_buf) catch {
                            return AgentMemError.InvalidJsonl;
                        };
                        buf.appendSlice(alloc, utf8_buf[0..utf8_len]) catch return AgentMemError.OutOfMemory;
                    },
                    else => return AgentMemError.InvalidJsonl,
                }
            } else {
                buf.append(alloc, c) catch return AgentMemError.OutOfMemory;
            }
        }
        return AgentMemError.InvalidJsonl;
    }

    fn readNumberSlice(*_): string ! AgentMemError {
        _.skipWs();
        const start = _.pos;
        if (_.pos < _.src.len && _.src[_.pos] == '-') {
            _.pos += 1;
        }
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-') {
                _.pos += 1;
            } else {
                break;
            }
        }
        if (_.pos == start) return AgentMemError.InvalidJsonl;
        return _.src[start.._.pos];
    }

    fn parseU64(*_): u64 ! AgentMemError {
        const raw = _.readNumberSlice()?;
        return std.fmt.parseInt(u64, raw, 10) catch return AgentMemError.InvalidJsonl;
    }

    fn parseU32(*_): u32 ! AgentMemError {
        const raw = _.readNumberSlice()?;
        return std.fmt.parseInt(u32, raw, 10) catch return AgentMemError.InvalidJsonl;
    }

    fn parseF64(*_): f64 ! AgentMemError {
        const raw = _.readNumberSlice()?;
        const val = std.fmt.parseFloat(f64, raw) catch return AgentMemError.InvalidJsonl;
        if (std.math.isNan(val) || std.math.isInf(val)) return AgentMemError.InvalidJsonl;
        return val;
    }

    fn skipValue(*_): bool ! AgentMemError {
        _.skipWs();
        let ch: u8 = 0;
        if (p := _.peek()) {
            ch = p;
        } else {
            return AgentMemError.InvalidJsonl;
        }
        switch (ch) {
            '"' => {
                _.pos += 1;
                while (_.pos < _.src.len) {
                    const c = _.src[_.pos];
                    _.pos += 1;
                    if (c == '\\') {
                        if (_.pos >= _.src.len) return AgentMemError.InvalidJsonl;
                        _.pos += 1;
                    } else if (c == '"') {
                        return true;
                    }
                }
                return AgentMemError.InvalidJsonl;
            },
            '{' => {
                _.pos += 1;
                _.skipWs();
                if (_.peek() == '}') {
                    _.pos += 1;
                    return true;
                }
                while (true) {
                    _.skipWs();
                    if (_.peek() != '"') return AgentMemError.InvalidJsonl;
                    * = _.skipValue()?;
                    _.expectChar(':')?;
                    * = _.skipValue()?;
                    _.skipWs();
                    if (next_ch := _.peek()) {
                        if (next_ch == ',') {
                            _.pos += 1;
                        } else if (next_ch == '}') {
                            _.pos += 1;
                            return true;
                        } else {
                            return AgentMemError.InvalidJsonl;
                        }
                    } else {
                        return AgentMemError.InvalidJsonl;
                    }
                }
            },
            '[' => {
                _.pos += 1;
                _.skipWs();
                if (_.peek() == ']') {
                    _.pos += 1;
                    return true;
                }
                while (true) {
                    * = _.skipValue()?;
                    _.skipWs();
                    if (next_ch := _.peek()) {
                        if (next_ch == ',') {
                            _.pos += 1;
                        } else if (next_ch == ']') {
                            _.pos += 1;
                            return true;
                        } else {
                            return AgentMemError.InvalidJsonl;
                        }
                    } else {
                        return AgentMemError.InvalidJsonl;
                    }
                }
            },
            't' => {
                if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "true")) {
                    _.pos += 4;
                    return true;
                }
                return AgentMemError.InvalidJsonl;
            },
            'f' => {
                if (_.pos + 5 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 5], "false")) {
                    _.pos += 5;
                    return true;
                }
                return AgentMemError.InvalidJsonl;
            },
            'n' => {
                if (_.pos + 4 <= _.src.len && std.mem.eql(u8, _.src[_.pos .. _.pos + 4], "null")) {
                    _.pos += 4;
                    return true;
                }
                return AgentMemError.InvalidJsonl;
            },
            else => {
                * = _.readNumberSlice()?;
                return true;
            },
        }
    }
}

/// Hierarchical three-tier Agent Memory Engine (Working / Episodic / Semantic).
pub struct AgentMemory {
    alloc: std.mem.Allocator = undefined;
    max_working_items: usize = 64;
    next_id: u64 = 1;
    items: std.ArrayList(StoredMemory) = .empty;

    /// Initializes a new `AgentMemory` instance with a capacity limit for `Working` memory.
    pub static fn init(
        alloc: std.mem.Allocator,
        max_working_items: usize,
    ): AgentMemory ! AgentMemError {
        if (max_working_items == 0) return AgentMemError.InvalidCapacity;
        return AgentMemory{
            alloc: alloc,
            max_working_items: max_working_items,
            next_id: 1,
            items: .empty,
        };
    }

    /// Releases all memory owned by this `AgentMemory` instance.
    pub fn deinit(*_) {
        for (*item of _.items.items) {
            item.freeOwned(_.alloc);
        }
        _.items.deinit(_.alloc);
    }

    /// Returns total number of stored memory items across all tiers.
    pub fn len(): usize {
        return _.items.items.len;
    }

    /// Returns number of stored memory items in the specified `tier`.
    pub fn countByTier(tier: MemoryTier): usize {
        let count: usize = 0;
        for (item of _.items.items) {
            if (item.tier == tier) count += 1;
        }
        return count;
    }

    /// Looks up a memory item by `id`, returning a read-only `MemoryItem` view if found.
    pub fn get(id: u64): MemoryItem? {
        for (item of _.items.items) {
            if (item.id == id) return item.toView();
        }
        return null;
    }

    /// Removes a memory item by `id`. Returns `true` if found and removed, `false` otherwise.
    pub fn remove(*_, id: u64): bool {
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (_.items.items[i].id == id) {
                let removed = _.items.orderedRemove(i);
                removed.freeOwned(_.alloc);
                return true;
            }
        }
        return false;
    }

    fn evictOldestWorkingIfFull(*_) {
        let working_count: usize = 0;
        let victim_idx: usize? = null;
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            const cur = _.items.items[i];
            if (cur.tier != .Working) continue;
            working_count += 1;
            if (best := victim_idx) {
                const prev = _.items.items[best];
                if (cur.last_accessed_ms < prev.last_accessed_ms ||
                    (cur.last_accessed_ms == prev.last_accessed_ms && cur.id < prev.id))
                {
                    victim_idx = i;
                }
            } else {
                victim_idx = i;
            }
        }
        if (working_count >= _.max_working_items) {
            if (idx := victim_idx) {
                let evicted = _.items.orderedRemove(idx);
                evicted.freeOwned(_.alloc);
            }
        }
    }

    /// Adds a memory item to `tier`. Validates `0.0 <= importance <= 1.0`.
    /// When `tier == .Working` and capacity is full, evicts the least-recently-accessed `Working` item.
    pub fn add(
        *_,
        tier: MemoryTier,
        content: string,
        tags: []const string,
        importance: f64,
        now_ms: u64,
    ): u64 ! AgentMemError {
        if (std.math.isNan(importance) || importance < 0.0 || importance > 1.0) {
            return AgentMemError.InvalidImportance;
        }

        if (tier == .Working) {
            _.evictOldestWorkingIfFull();
        }

        const owned_content: []u8 = if (content.len > 0)
            (_.alloc.dupe(u8, content) catch return AgentMemError.OutOfMemory)
        else
            [];
        errdefer if (owned_content.len > 0) _.alloc.free(owned_content);

        let owned_tags: [][]u8 = [];
        let owned_views: []string = [];
        if (tags.len > 0) {
            let tags_list: std.ArrayList([]u8) = .empty;
            errdefer {
                for (t of tags_list.items) {
                    if (t.len > 0) _.alloc.free(t);
                }
                tags_list.deinit(_.alloc);
            }
            for (t of tags) {
                const duped: []u8 = if (t.len > 0)
                    (_.alloc.dupe(u8, t) catch return AgentMemError.OutOfMemory)
                else
                    [];
                tags_list.append(_.alloc, duped) catch {
                    if (duped.len > 0) _.alloc.free(duped);
                    return AgentMemError.OutOfMemory;
                };
            }
            owned_tags = tags_list.toOwnedSlice(_.alloc) catch return AgentMemError.OutOfMemory;
            errdefer {
                for (t of owned_tags) {
                    if (t.len > 0) _.alloc.free(t);
                }
                _.alloc.free(owned_tags);
            }

            owned_views = _.alloc.alloc(string, tags.len) catch return AgentMemError.OutOfMemory;
            let k: usize = 0;
            while (k < tags.len) : (k += 1) {
                owned_views[k] = owned_tags[k];
            }
        }
        errdefer if (owned_views.len > 0) _.alloc.free(owned_views);

        const assigned_id = _.next_id;
        _.items.append(_.alloc, StoredMemory{
            id: assigned_id,
            tier: tier,
            content: owned_content,
            tags: owned_tags,
            tag_views: owned_views,
            importance: importance,
            created_at_ms: now_ms,
            last_accessed_ms: now_ms,
            access_count: 0,
        }) catch return AgentMemError.OutOfMemory;

        _.next_id += 1;
        return assigned_id;
    }

    fn recallInternal(
        *_,
        alloc: std.mem.Allocator,
        tier_filter: MemoryTier?,
        query: string,
        now_ms: u64,
        limit: usize,
        weights: RecallWeights,
    ): ([]RecallHit) ! AgentMemError {
        if (limit == 0 || _.items.items.len == 0) {
            return alloc.alloc(RecallHit, 0) catch return AgentMemError.OutOfMemory;
        }

        let query_tokens: std.StringHashMapUnmanaged(u32) = .empty;
        defer freeTokenMap(alloc, *query_tokens);
        * = tokenizeIntoMap(alloc, query, *query_tokens) catch return AgentMemError.OutOfMemory;

        let candidates: std.ArrayList(ScoredCandidate) = .empty;
        defer candidates.deinit(alloc);

        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            const item = _.items.items[i];
            if (tf := tier_filter) {
                if (item.tier != tf) continue;
            }

            const recency = computeRecencyScore(now_ms, item.last_accessed_ms, weights.decay_half_life_ms);
            const importance = item.importance;
            const relevance = computeRelevanceScore(alloc, &query_tokens, item.content, item.tag_views) catch {
                return AgentMemError.OutOfMemory;
            };
            const total = weights.recency_weight * recency +
                weights.importance_weight * importance +
                weights.relevance_weight * relevance;

            candidates.append(alloc, ScoredCandidate{
                item_idx: i,
                last_accessed_ms: item.last_accessed_ms,
                hit: RecallHit{
                    id: item.id,
                    tier: item.tier,
                    content: item.content,
                    total_score: total,
                    recency_score: recency,
                    importance_score: importance,
                    relevance_score: relevance,
                },
            }) catch return AgentMemError.OutOfMemory;
        }

        sortCandidatesDesc(candidates.items);

        const out_len = if (candidates.items.len < limit) candidates.items.len else limit;
        let out = alloc.alloc(RecallHit, out_len) catch return AgentMemError.OutOfMemory;

        let k: usize = 0;
        while (k < out_len) : (k += 1) {
            const cand = candidates.items[k];
            out[k] = cand.hit;
            _.items.items[cand.item_idx].last_accessed_ms = now_ms;
            _.items.items[cand.item_idx].access_count += 1;
        }
        return out;
    }

    /// Recalls up to `limit` memories across all tiers ranked by Recency + Importance + Relevance.
    /// Automatically updates `last_accessed_ms` and increments `access_count` on returned hits.
    pub fn recall(
        *_,
        alloc: std.mem.Allocator,
        query: string,
        now_ms: u64,
        limit: usize,
        weights: RecallWeights = RecallWeights{},
    ): ([]RecallHit) ! AgentMemError {
        return _.recallInternal(alloc, null, query, now_ms, limit, weights);
    }

    /// Recalls up to `limit` memories from a specific `tier`.
    /// Automatically updates `last_accessed_ms` and increments `access_count` on returned hits.
    pub fn recallByTier(
        *_,
        alloc: std.mem.Allocator,
        tier: MemoryTier,
        query: string,
        now_ms: u64,
        limit: usize,
        weights: RecallWeights = RecallWeights{},
    ): ([]RecallHit) ! AgentMemError {
        return _.recallInternal(alloc, tier, query, now_ms, limit, weights);
    }

    /// Promotes `Working` and `Episodic` memories meeting `min_access_count` and `min_importance`
    /// into `Semantic` long-term memory. Returns the number of promoted items.
    pub fn consolidate(*_, min_access_count: u32, min_importance: f64): usize {
        let promoted: usize = 0;
        for (*item of _.items.items) {
            if (item.tier == .Working || item.tier == .Episodic) {
                if (item.access_count >= min_access_count && item.importance >= min_importance) {
                    item.tier = .Semantic;
                    promoted += 1;
                }
            }
        }
        return promoted;
    }

    /// Removes stale, low-value `Episodic` memories whose `recency + importance < min_recency_plus_importance`.
    /// `Semantic` facts (and `Working` items) are preserved. Returns the number of pruned items.
    pub fn prune(
        *_,
        now_ms: u64,
        min_recency_plus_importance: f64,
    ): usize {
        return _.pruneWithHalfLife(now_ms, min_recency_plus_importance, 3600000.0);
    }

    /// Same as `prune`, with an explicit `decay_half_life_ms`.
    pub fn pruneWithHalfLife(
        *_,
        now_ms: u64,
        min_recency_plus_importance: f64,
        decay_half_life_ms: f64,
    ): usize {
        let pruned: usize = 0;
        let i: usize = 0;
        while (i < _.items.items.len) {
            const item = _.items.items[i];
            if (item.tier == .Episodic) {
                const recency = computeRecencyScore(now_ms, item.last_accessed_ms, decay_half_life_ms);
                if (recency + item.importance < min_recency_plus_importance) {
                    let removed = _.items.orderedRemove(i);
                    removed.freeOwned(_.alloc);
                    pruned += 1;
                    continue;
                }
            }
            i += 1;
        }
        return pruned;
    }

    /// Serializes all stored memory items to JSON Lines (JSONL) format.
    pub fn exportJsonl(alloc: std.mem.Allocator): ([]u8) ! AgentMemError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let num_buf: [64]u8 = [0; 64];
        for (item of _.items.items) {
            out.appendSlice(alloc, "{\"id\":") catch return AgentMemError.OutOfMemory;
            const id_s = std.fmt.bufPrint(&num_buf, "{d}", .{item.id}) catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, id_s) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, ",\"tier\":\"") catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, item.tier.toStr()) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, "\",\"content\":") catch return AgentMemError.OutOfMemory;
            appendJsonEscaped(alloc, *out, item.content) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, ",\"tags\":[") catch return AgentMemError.OutOfMemory;
            let t_idx: usize = 0;
            while (t_idx < item.tag_views.len) : (t_idx += 1) {
                if (t_idx > 0) out.append(alloc, ',') catch return AgentMemError.OutOfMemory;
                appendJsonEscaped(alloc, *out, item.tag_views[t_idx]) catch return AgentMemError.OutOfMemory;
            }

            out.appendSlice(alloc, "],\"importance\":") catch return AgentMemError.OutOfMemory;
            const imp_s = std.fmt.bufPrint(&num_buf, "{d}", .{item.importance}) catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, imp_s) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, ",\"created_at_ms\":") catch return AgentMemError.OutOfMemory;
            const cr_s = std.fmt.bufPrint(&num_buf, "{d}", .{item.created_at_ms}) catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, cr_s) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, ",\"last_accessed_ms\":") catch return AgentMemError.OutOfMemory;
            const la_s = std.fmt.bufPrint(&num_buf, "{d}", .{item.last_accessed_ms}) catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, la_s) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, ",\"access_count\":") catch return AgentMemError.OutOfMemory;
            const ac_s = std.fmt.bufPrint(&num_buf, "{d}", .{item.access_count}) catch return AgentMemError.OutOfMemory;
            out.appendSlice(alloc, ac_s) catch return AgentMemError.OutOfMemory;

            out.appendSlice(alloc, "}\n") catch return AgentMemError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return AgentMemError.OutOfMemory;
    }

    /// Reconstructs an `AgentMemory` instance from JSON Lines (JSONL) text (default working capacity 64).
    pub static fn importJsonl(
        alloc: std.mem.Allocator,
        jsonl_text: string,
    ): AgentMemory ! AgentMemError {
        return AgentMemory.importJsonlWithCapacity(alloc, jsonl_text, 64);
    }

    /// Reconstructs an `AgentMemory` instance from JSON Lines (JSONL) text with explicit `max_working_items`.
    pub static fn importJsonlWithCapacity(
        alloc: std.mem.Allocator,
        jsonl_text: string,
        max_working_items: usize,
    ): AgentMemory ! AgentMemError {
        let mem = AgentMemory.init(alloc, max_working_items)?;
        errdefer mem.deinit();

        let max_id: u64 = 0;
        let start: usize = 0;
        while (start < jsonl_text.len) {
            let end = start;
            while (end < jsonl_text.len && jsonl_text[end] != '\n') : (end += 1) {}
            const raw_line = jsonl_text[start..end];
            start = if (end < jsonl_text.len) end + 1 else end;

            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0) continue;

            let cur = JsonCursor{ src: line, pos: 0 };
            cur.expectChar('{')?;

            let has_id = false;
            let has_tier = false;
            let has_content = false;
            let has_importance = false;
            let has_created = false;
            let has_accessed = false;
            let has_count = false;

            let parsed_id: u64 = 0;
            let parsed_tier: MemoryTier = .Working;
            let parsed_content: []u8 = [];
            errdefer if (parsed_content.len > 0) alloc.free(parsed_content);

            let parsed_tags: std.ArrayList([]u8) = .empty;
            errdefer {
                for (t of parsed_tags.items) {
                    if (t.len > 0) alloc.free(t);
                }
                parsed_tags.deinit(alloc);
            }

            let parsed_importance: f64 = 0.0;
            let parsed_created: u64 = 0;
            let parsed_accessed: u64 = 0;
            let parsed_count: u32 = 0;

            cur.skipWs();
            if (cur.peek() != '}') {
                while (true) {
                    const key = cur.parseString(alloc)?;
                    defer alloc.free(key);
                    cur.expectChar(':')?;

                    if (std.mem.eql(u8, key, "id")) {
                        parsed_id = cur.parseU64()?;
                        has_id = true;
                    } else if (std.mem.eql(u8, key, "tier")) {
                        const tier_s = cur.parseString(alloc)?;
                        defer alloc.free(tier_s);
                        if (t := MemoryTier.fromStr(tier_s)) {
                            parsed_tier = t;
                        } else {
                            return AgentMemError.InvalidJsonl;
                        }
                        has_tier = true;
                    } else if (std.mem.eql(u8, key, "content")) {
                        if (parsed_content.len > 0) alloc.free(parsed_content);
                        parsed_content = cur.parseString(alloc)?;
                        has_content = true;
                    } else if (std.mem.eql(u8, key, "tags")) {
                        cur.expectChar('[')?;
                        cur.skipWs();
                        if (cur.peek() != ']') {
                            while (true) {
                                const tag_s = cur.parseString(alloc)?;
                                parsed_tags.append(alloc, tag_s) catch {
                                    alloc.free(tag_s);
                                    return AgentMemError.OutOfMemory;
                                };
                                cur.skipWs();
                                if (arr_next := cur.peek()) {
                                    if (arr_next == ',') {
                                        cur.pos += 1;
                                    } else if (arr_next == ']') {
                                        cur.pos += 1;
                                        break;
                                    } else {
                                        return AgentMemError.InvalidJsonl;
                                    }
                                } else {
                                    return AgentMemError.InvalidJsonl;
                                }
                            }
                        } else {
                            cur.pos += 1;
                        }
                    } else if (std.mem.eql(u8, key, "importance")) {
                        parsed_importance = cur.parseF64()?;
                        if (parsed_importance < 0.0 || parsed_importance > 1.0) {
                            return AgentMemError.InvalidJsonl;
                        }
                        has_importance = true;
                    } else if (std.mem.eql(u8, key, "created_at_ms")) {
                        parsed_created = cur.parseU64()?;
                        has_created = true;
                    } else if (std.mem.eql(u8, key, "last_accessed_ms")) {
                        parsed_accessed = cur.parseU64()?;
                        has_accessed = true;
                    } else if (std.mem.eql(u8, key, "access_count")) {
                        parsed_count = cur.parseU32()?;
                        has_count = true;
                    } else {
                        * = cur.skipValue()?;
                    }

                    cur.skipWs();
                    if (obj_next := cur.peek()) {
                        if (obj_next == ',') {
                            cur.pos += 1;
                        } else if (obj_next == '}') {
                            cur.pos += 1;
                            break;
                        } else {
                            return AgentMemError.InvalidJsonl;
                        }
                    } else {
                        return AgentMemError.InvalidJsonl;
                    }
                }
            } else {
                cur.pos += 1;
            }

            cur.skipWs();
            if (cur.pos != line.len) return AgentMemError.InvalidJsonl;

            if (!has_id || !has_tier || !has_content || !has_importance || !has_created || !has_accessed || !has_count) {
                return AgentMemError.InvalidJsonl;
            }

            const owned_tags = parsed_tags.toOwnedSlice(alloc) catch return AgentMemError.OutOfMemory;
            errdefer {
                for (t of owned_tags) {
                    if (t.len > 0) alloc.free(t);
                }
                if (owned_tags.len > 0) alloc.free(owned_tags);
            }

            let tag_views: []string = [];
            if (owned_tags.len > 0) {
                tag_views = alloc.alloc(string, owned_tags.len) catch return AgentMemError.OutOfMemory;
                let k: usize = 0;
                while (k < owned_tags.len) : (k += 1) {
                    tag_views[k] = owned_tags[k];
                }
            }
            errdefer if (tag_views.len > 0) alloc.free(tag_views);

            mem.items.append(alloc, StoredMemory{
                id: parsed_id,
                tier: parsed_tier,
                content: parsed_content,
                tags: owned_tags,
                tag_views: tag_views,
                importance: parsed_importance,
                created_at_ms: parsed_created,
                last_accessed_ms: parsed_accessed,
                access_count: parsed_count,
            }) catch return AgentMemError.OutOfMemory;

            // Ownership transferred to mem.items; clear local slices so errdefer won't double-free
            parsed_content = [];
            if (parsed_id > max_id) max_id = parsed_id;
        }

        mem.next_id = max_id + 1;
        return mem;
    }
}

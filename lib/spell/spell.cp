// Source: cp/lib/spell/spell.cp (SymSpell, BK-Tree, Damerau-Levenshtein & Soundex)
// Pure CP zero-C-dependency high-speed spell checking and fuzzy correction engine.
// Combines SymSpell symmetric-delete indexing, BK-Tree metric pruning, OSA
// Damerau-Levenshtein distance, and American Soundex phonetic encoding.

const std = @import("std");

/// Error set for spell checking, dictionary indexing, and distance computation.
pub err SpellError {
    InvalidMaxDistance,
    EmptyWord,
    OutOfMemory,
}

/// Suggestion filtering mode for `SymSpellIndex.lookup`.
pub enum SuggestVerbosity {
    Top,
    Closest,
    All,
}

/// Ranked spelling correction candidate returned by `SymSpellIndex.lookup` and `BkTree.search`.
pub struct SpellSuggestion {
    term: string;
    distance: u32;
    frequency: u64;
}

const WordEntry = struct {
    term: []u8;
    freq: u64;
};

const BkNode = struct {
    term: []u8;
    freq: u64;
    children: std.AutoHashMapUnmanaged(u32, usize) = .empty;
};

fn toLowerAsciiAlloc(alloc: std.mem.Allocator, s: string): []u8 ! SpellError {
    let buf = alloc.alloc(u8, s.len) catch return SpellError.OutOfMemory;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        buf[i] = std.ascii.toLower(s[i]);
    }
    return buf;
}

fn suggestionLessThan(ctx: void, a: SpellSuggestion, b: SpellSuggestion): bool {
    * = ctx;
    if (a.distance != b.distance) {
        return a.distance < b.distance;
    }
    if (a.frequency != b.frequency) {
        return a.frequency > b.frequency;
    }
    return std.mem.order(u8, a.term, b.term) == .lt;
}

/// Compute Optimal String Alignment (restricted Damerau-Levenshtein) edit distance
/// between byte slices `a` and `b`. Adjacent transpositions (e.g. "teh" -> "the")
/// count as distance 1.
pub fn damerauLevenshtein(alloc: std.mem.Allocator, a: string, b: string): u32 ! SpellError {
    if (a.len == 0) {
        return b.len as u32;
    }
    if (b.len == 0) {
        return a.len as u32;
    }
    if (std.mem.eql(u8, a, b)) {
        return 0;
    }

    // Ensure `s2` is the shorter string to minimize DP row memory.
    let s1 = a;
    let s2 = b;
    if (s1.len < s2.len) {
        s1 = b;
        s2 = a;
    }

    const m = s1.len;
    const n = s2.len;
    const stride = n + 1;

    const rows = alloc.alloc(u32, 3 * stride) catch return SpellError.OutOfMemory;
    defer alloc.free(rows);

    let prev2 = rows[0..stride];
    let prev1 = rows[stride .. 2 * stride];
    let curr = rows[2 * stride .. 3 * stride];

    let j: usize = 0;
    while (j <= n) : (j += 1) {
        prev1[j] = j as u32;
        prev2[j] = 0;
    }

    let i: usize = 1;
    while (i <= m) : (i += 1) {
        curr[0] = i as u32;
        j = 1;
        while (j <= n) : (j += 1) {
            const cost: u32 = if (s1[i - 1] == s2[j - 1]) 0 else 1;
            let best = prev1[j] + 1; // deletion
            const ins = curr[j - 1] + 1; // insertion
            if (ins < best) {
                best = ins;
            }
            const sub = prev1[j - 1] + cost; // substitution
            if (sub < best) {
                best = sub;
            }
            if (i > 1 && j > 1 && s1[i - 1] == s2[j - 2] && s1[i - 2] == s2[j - 1]) {
                const trans = prev2[j - 2] + 1;
                if (trans < best) {
                    best = trans;
                }
            }
            curr[j] = best;
        }
        const tmp = prev2;
        prev2 = prev1;
        prev1 = curr;
        curr = tmp;
    }

    return prev1[n];
}

fn soundexDigit(ch: u8): u8 {
    return switch (std.ascii.toUpper(ch)) {
        'B', 'F', 'P', 'V' => '1',
        'C', 'G', 'J', 'K', 'Q', 'S', 'X', 'Z' => '2',
        'D', 'T' => '3',
        'L' => '4',
        'M', 'N' => '5',
        'R' => '6',
        else => '0',
    };
}

/// Compute the classic 4-character American Soundex phonetic code for `word`.
/// Example: `"Robert"` and `"Rupert"` both encode to `"R163"`.
pub fn soundex(word: string): [4]u8 {
    let code: [4]u8 = ['0', '0', '0', '0'];
    let start: usize = 0;
    while (start < word.len && !std.ascii.isAlphabetic(word[start])) : (start += 1) {}
    if (start >= word.len) {
        return code;
    }

    code[0] = std.ascii.toUpper(word[start]);
    let prev_digit = soundexDigit(word[start]);
    let out_idx: usize = 1;
    let i: usize = start + 1;

    while (i < word.len && out_idx < 4) : (i += 1) {
        const ch = word[i];
        if (!std.ascii.isAlphabetic(ch)) {
            continue;
        }
        const up = std.ascii.toUpper(ch);
        if (up == 'H' || up == 'W') {
            // H and W are transparent in American Soundex (do not reset prev_digit).
            continue;
        }
        const d = soundexDigit(up);
        if (d != '0' && d != prev_digit) {
            code[out_idx] = d;
            out_idx += 1;
        }
        prev_digit = d;
    }

    return code;
}

fn generateDeletesRecursive(
    alloc: std.mem.Allocator,
    word: []const u8,
    edit_dist: u32,
    max_dist: u32,
    seen: *std.StringHashMapUnmanaged(void),
    out_list: *std.ArrayList([]u8),
): void ! SpellError {
    if (edit_dist >= max_dist || word.len == 0) {
        return;
    }
    let i: usize = 0;
    while (i < word.len) : (i += 1) {
        const del = alloc.alloc(u8, word.len - 1) catch return SpellError.OutOfMemory;
        if (i > 0) {
            @memcpy(del[0..i], word[0..i]);
        }
        if (i + 1 < word.len) {
            @memcpy(del[i..], word[i + 1 ..]);
        }

        if (seen.contains(del)) {
            alloc.free(del);
            continue;
        }

        seen.put(alloc, del, {}) catch {
            alloc.free(del);
            return SpellError.OutOfMemory;
        };
        out_list.append(alloc, del) catch {
            return SpellError.OutOfMemory;
        };

        if (edit_dist + 1 < max_dist) {
            generateDeletesRecursive(alloc, del, edit_dist + 1, max_dist, seen, out_list)?;
        }
    }
}

/// High-speed SymSpell symmetric-delete spelling correction index.
/// Pre-computes delete variants on dictionary insertion for O(1)-like lookup.
pub struct SymSpellIndex {
    alloc: std.mem.Allocator;
    max_edit_distance: u32;
    prefix_length: usize;
    words: std.ArrayList(WordEntry) = .empty;
    word_map: std.StringHashMapUnmanaged(u32) = .empty;
    deletes: std.StringHashMapUnmanaged(std.ArrayList(u32)) = .empty;
    owned_delete_keys: std.ArrayList([]u8) = .empty;

    /// Initialize an empty `SymSpellIndex` with maximum edit distance and prefix length.
    pub static fn init(
        alloc: std.mem.Allocator,
        max_edit_distance: u32,
        prefix_length: usize,
    ): SymSpellIndex ! SpellError {
        if (max_edit_distance > 8) {
            return SpellError.InvalidMaxDistance;
        }
        return SymSpellIndex{
            alloc: alloc,
            max_edit_distance: max_edit_distance,
            prefix_length: prefix_length,
            words: .empty,
            word_map: .empty,
            deletes: .empty,
            owned_delete_keys: .empty,
        };
    }

    /// Release all dictionary terms, posting lists, and delete-key buffers.
    pub fn deinit(*_) {
        let it = _.deletes.valueIterator();
        while (list_ptr := it.next()) {
            list_ptr.deinit(_.alloc);
        }
        _.deletes.deinit(_.alloc);
        for (k of _.owned_delete_keys.items) {
            _.alloc.free(k);
        }
        _.owned_delete_keys.deinit(_.alloc);
        _.word_map.deinit(_.alloc);
        for (w of _.words.items) {
            _.alloc.free(w.term);
        }
        _.words.deinit(_.alloc);
    }

    /// Return the number of unique normalized terms in the dictionary.
    pub fn wordCount(): usize {
        return _.words.items.len;
    }

    /// Add or accumulate a dictionary term with its corpus frequency.
    pub fn addWord(*_, word: string, freq: u64): void ! SpellError {
        if (word.len == 0) {
            return SpellError.EmptyWord;
        }

        const norm = toLowerAsciiAlloc(_.alloc, word)?;
        if (existing_idx := _.word_map.get(norm)) {
            _.alloc.free(norm);
            _.words.items[existing_idx].freq = _.words.items[existing_idx].freq +| freq;
            return;
        }

        const word_idx: u32 = _.words.items.len as u32;
        _.words.append(_.alloc, WordEntry{ term: norm, freq: freq }) catch {
            _.alloc.free(norm);
            return SpellError.OutOfMemory;
        };
        _.word_map.put(_.alloc, norm, word_idx) catch {
            * = _.words.pop();
            _.alloc.free(norm);
            return SpellError.OutOfMemory;
        };

        if (_.max_edit_distance == 0) {
            return;
        }

        const eff_prefix_len: usize = if (_.prefix_length > 0 && norm.len > _.prefix_length)
            _.prefix_length
        else
            norm.len;
        const prefix = norm[0..eff_prefix_len];

        let seen: std.StringHashMapUnmanaged(void) = .empty;
        defer seen.deinit(_.alloc);
        let del_list: std.ArrayList([]u8) = .empty;
        defer del_list.deinit(_.alloc);

        // If the word was truncated to `prefix_length`, index the prefix itself too.
        if (norm.len > eff_prefix_len) {
            const p_copy = _.alloc.alloc(u8, prefix.len) catch return SpellError.OutOfMemory;
            @memcpy(p_copy, prefix);
            seen.put(_.alloc, p_copy, {}) catch {
                _.alloc.free(p_copy);
                return SpellError.OutOfMemory;
            };
            del_list.append(_.alloc, p_copy) catch {
                return SpellError.OutOfMemory;
            };
        }

        generateDeletesRecursive(_.alloc, prefix, 0, _.max_edit_distance, *seen, *del_list) catch e => {
            for (d of del_list.items) {
                _.alloc.free(d);
            }
            return e;
        };

        for (del_key of del_list.items) {
            if (posting := _.deletes.getPtr(del_key)) {
                posting.append(_.alloc, word_idx) catch {
                    _.alloc.free(del_key);
                    return SpellError.OutOfMemory;
                };
                _.alloc.free(del_key);
            } else {
                _.owned_delete_keys.append(_.alloc, del_key) catch {
                    _.alloc.free(del_key);
                    return SpellError.OutOfMemory;
                };
                let new_posting: std.ArrayList(u32) = .empty;
                new_posting.append(_.alloc, word_idx) catch {
                    new_posting.deinit(_.alloc);
                    return SpellError.OutOfMemory;
                };
                _.deletes.put(_.alloc, del_key, new_posting) catch {
                    new_posting.deinit(_.alloc);
                    return SpellError.OutOfMemory;
                };
            }
        }
    }

    fn considerCandidate(
        alloc: std.mem.Allocator,
        norm_input: string,
        word_idx: u32,
        max_distance: u32,
        considered: *std.AutoHashMapUnmanaged(u32, void),
        candidates: *std.ArrayList(SpellSuggestion),
    ): void ! SpellError {
        if (considered.contains(word_idx)) {
            return;
        }
        considered.put(alloc, word_idx, {}) catch return SpellError.OutOfMemory;

        const entry = _.words.items[word_idx];
        const len_diff: usize = if (entry.term.len >= norm_input.len)
            entry.term.len - norm_input.len
        else
            norm_input.len - entry.term.len;
        if (len_diff > usize(max_distance)) {
            return;
        }

        const dist = damerauLevenshtein(alloc, norm_input, entry.term)?;
        if (dist <= max_distance) {
            candidates.append(alloc, SpellSuggestion{
                term: entry.term,
                distance: dist,
                frequency: entry.freq,
            }) catch return SpellError.OutOfMemory;
        }
    }

    /// Look up spelling suggestions for `input` within `max_distance`.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    pub fn lookup(
        alloc: std.mem.Allocator,
        input: string,
        verbosity: SuggestVerbosity,
        max_distance: u32,
    ): []SpellSuggestion ! SpellError {
        if (input.len == 0) {
            return SpellError.EmptyWord;
        }
        if (max_distance > _.max_edit_distance) {
            return SpellError.InvalidMaxDistance;
        }

        const norm_input = toLowerAsciiAlloc(alloc, input)?;
        defer alloc.free(norm_input);

        let considered: std.AutoHashMapUnmanaged(u32, void) = .empty;
        defer considered.deinit(alloc);
        let candidates: std.ArrayList(SpellSuggestion) = .empty;
        defer candidates.deinit(alloc);

        if (exact_idx := _.word_map.get(norm_input)) {
            considered.put(alloc, exact_idx, {}) catch return SpellError.OutOfMemory;
            const entry = _.words.items[exact_idx];
            candidates.append(alloc, SpellSuggestion{
                term: entry.term,
                distance: 0,
                frequency: entry.freq,
            }) catch return SpellError.OutOfMemory;

            if (verbosity == .Top) {
                let res = alloc.alloc(SpellSuggestion, 1) catch return SpellError.OutOfMemory;
                res[0] = candidates.items[0];
                return res;
            }
        }

        if (max_distance > 0) {
            // Check direct match in delete index (input may be a delete of a longer word).
            if (direct_list := _.deletes.get(norm_input)) {
                for (widx of direct_list.items) {
                    _.considerCandidate(alloc, norm_input, widx, max_distance, *considered, *candidates)?;
                }
            }

            const eff_q_len: usize = if (_.prefix_length > 0 && norm_input.len > _.prefix_length)
                _.prefix_length
            else
                norm_input.len;
            const q_prefix = norm_input[0..eff_q_len];

            if (!std.mem.eql(u8, q_prefix, norm_input)) {
                if (prefix_list := _.deletes.get(q_prefix)) {
                    for (widx of prefix_list.items) {
                        _.considerCandidate(alloc, norm_input, widx, max_distance, *considered, *candidates)?;
                    }
                }
            }

            let seen_q: std.StringHashMapUnmanaged(void) = .empty;
            defer seen_q.deinit(alloc);
            let q_deletes: std.ArrayList([]u8) = .empty;
            defer {
                for (d of q_deletes.items) {
                    alloc.free(d);
                }
                q_deletes.deinit(alloc);
            }

            generateDeletesRecursive(alloc, q_prefix, 0, max_distance, *seen_q, *q_deletes)?;

            for (q_del of q_deletes.items) {
                if (widx := _.word_map.get(q_del)) {
                    _.considerCandidate(alloc, norm_input, widx, max_distance, *considered, *candidates)?;
                }
                if (del_list := _.deletes.get(q_del)) {
                    for (widx of del_list.items) {
                        _.considerCandidate(alloc, norm_input, widx, max_distance, *considered, *candidates)?;
                    }
                }
            }
        }

        if (candidates.items.len == 0) {
            return alloc.alloc(SpellSuggestion, 0) catch return SpellError.OutOfMemory;
        }

        std.mem.sort(SpellSuggestion, candidates.items, {}, suggestionLessThan);

        let keep_count: usize = candidates.items.len;
        switch (verbosity) {
            .Top => keep_count = 1,
            .Closest => {
                const min_dist = candidates.items[0].distance;
                keep_count = 1;
                while (keep_count < candidates.items.len && candidates.items[keep_count].distance == min_dist) : (keep_count += 1) {}
            },
            .All => {},
        }

        const out = alloc.alloc(SpellSuggestion, keep_count) catch return SpellError.OutOfMemory;
        @memcpy(out, candidates.items[0..keep_count]);
        return out;
    }
}

/// Burkhard-Keller Tree (BK-Tree) for arbitrary-distance metric-space fuzzy search
/// using triangle inequality pruning `[d - max_dist, d + max_dist]`.
pub struct BkTree {
    alloc: std.mem.Allocator;
    nodes: std.ArrayList(BkNode) = .empty;
    word_map: std.StringHashMapUnmanaged(usize) = .empty;

    /// Initialize an empty `BkTree`.
    pub static fn init(alloc: std.mem.Allocator): BkTree {
        return BkTree{
            alloc: alloc,
            nodes: .empty,
            word_map: .empty,
        };
    }

    /// Release all tree nodes and owned term buffers.
    pub fn deinit(*_) {
        for (*node of _.nodes.items) {
            node.children.deinit(_.alloc);
            _.alloc.free(node.term);
        }
        _.nodes.deinit(_.alloc);
        _.word_map.deinit(_.alloc);
    }

    /// Return the number of unique normalized terms stored in the tree.
    pub fn wordCount(): usize {
        return _.nodes.items.len;
    }

    /// Insert a term and its frequency into the BK-Tree.
    pub fn insert(*_, word: string, freq: u64): void ! SpellError {
        if (word.len == 0) {
            return SpellError.EmptyWord;
        }

        const norm = toLowerAsciiAlloc(_.alloc, word)?;
        if (existing_idx := _.word_map.get(norm)) {
            _.alloc.free(norm);
            _.nodes.items[existing_idx].freq = _.nodes.items[existing_idx].freq +| freq;
            return;
        }

        if (_.nodes.items.len == 0) {
            _.nodes.append(_.alloc, BkNode{
                term: norm,
                freq: freq,
                children: .empty,
            }) catch {
                _.alloc.free(norm);
                return SpellError.OutOfMemory;
            };
            _.word_map.put(_.alloc, norm, 0) catch {
                * = _.nodes.pop();
                _.alloc.free(norm);
                return SpellError.OutOfMemory;
            };
            return;
        }

        let curr_idx: usize = 0;
        while (true) {
            const dist = damerauLevenshtein(_.alloc, norm, _.nodes.items[curr_idx].term) catch e => {
                _.alloc.free(norm);
                return e;
            };
            if (dist == 0) {
                _.alloc.free(norm);
                _.nodes.items[curr_idx].freq = _.nodes.items[curr_idx].freq +| freq;
                return;
            }
            if (next_idx := _.nodes.items[curr_idx].children.get(dist)) {
                curr_idx = next_idx;
            } else {
                const new_idx = _.nodes.items.len;
                _.nodes.append(_.alloc, BkNode{
                    term: norm,
                    freq: freq,
                    children: .empty,
                }) catch {
                    _.alloc.free(norm);
                    return SpellError.OutOfMemory;
                };
                _.word_map.put(_.alloc, norm, new_idx) catch {
                    * = _.nodes.pop();
                    _.alloc.free(norm);
                    return SpellError.OutOfMemory;
                };
                _.nodes.items[curr_idx].children.put(_.alloc, dist, new_idx) catch {
                    * = _.word_map.remove(norm);
                    * = _.nodes.pop();
                    _.alloc.free(norm);
                    return SpellError.OutOfMemory;
                };
                return;
            }
        }
    }

    /// Search for all terms within `max_dist` edit distance of `query`, sorted by
    /// `(distance ASC, frequency DESC, term ASC)`. Caller must `alloc.free` the result.
    pub fn search(alloc: std.mem.Allocator, query: string, max_dist: u32): []SpellSuggestion ! SpellError {
        if (query.len == 0) {
            return SpellError.EmptyWord;
        }
        if (_.nodes.items.len == 0) {
            return alloc.alloc(SpellSuggestion, 0) catch return SpellError.OutOfMemory;
        }

        const norm_q = toLowerAsciiAlloc(alloc, query)?;
        defer alloc.free(norm_q);

        let results: std.ArrayList(SpellSuggestion) = .empty;
        defer results.deinit(alloc);

        let stack: std.ArrayList(usize) = .empty;
        defer stack.deinit(alloc);
        stack.append(alloc, 0) catch return SpellError.OutOfMemory;

        while (stack.items.len > 0) {
            const idx = stack.pop().?;
            const node = _.nodes.items[idx];
            const dist = damerauLevenshtein(alloc, norm_q, node.term)?;

            if (dist <= max_dist) {
                results.append(alloc, SpellSuggestion{
                    term: node.term,
                    distance: dist,
                    frequency: node.freq,
                }) catch return SpellError.OutOfMemory;
            }

            const min_d: u32 = if (dist > max_dist) dist - max_dist else 0;
            const max_d: u32 = dist +| max_dist;

            let it = node.children.iterator();
            while (entry := it.next()) {
                const child_d = entry.key_ptr.*;
                if (child_d >= min_d && child_d <= max_d) {
                    stack.append(alloc, entry.value_ptr.*) catch return SpellError.OutOfMemory;
                }
            }
        }

        std.mem.sort(SpellSuggestion, results.items, {}, suggestionLessThan);
        return results.toOwnedSlice(alloc) catch return SpellError.OutOfMemory;
    }
}

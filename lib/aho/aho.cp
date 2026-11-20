// Source: cp/lib/aho/aho.cp (Aho-Corasick Multi-Pattern String Matching Automaton)
// Pure CP zero-C-dependency Aho-Corasick automaton with Trie + BFS failure links,
// dictionary suffix links, O(N) overlapping matching, LeftmostFirst / LeftmostLongest
// non-overlapping disambiguation, zero-allocation containsAny, and replaceAll.

const std = @import("std");

const NONE_STATE: u32 = std.math.maxInt(u32);
const NONE_OUT: u32 = std.math.maxInt(u32);

/// Error set for Aho-Corasick construction, compilation, and matching.
pub err AhoError {
    EmptyPattern,
    NoPatterns,
    PatternNotCompiled,
    InvalidOption,
    OutOfMemory,
}

/// Match disambiguation strategy.
pub enum MatchKind {
    AllOverlapping,
    LeftmostFirst,
    LeftmostLongest,
}

/// A single pattern match span `[start, end)` in the scanned text.
pub struct AhoMatch {
    pattern_id: usize;
    start: usize;
    end: usize;
}

const OutputNode = struct {
    pattern_id: u32,
    prev: u32,
};

const AhoState = struct {
    trie_next: [256]u32,
    dfa_next: [256]u32,
    fail: u32,
    dict_link: u32,
    out_head: u32,
};

fn makeEmptyState(): AhoState {
    let s: AhoState = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        s.trie_next[i] = NONE_STATE;
        s.dfa_next[i] = 0;
    }
    s.fail = 0;
    s.dict_link = NONE_STATE;
    s.out_head = NONE_OUT;
    return s;
}

@inline
fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn lessMatch(a: AhoMatch, b: AhoMatch, kind: MatchKind): bool {
    if (a.start != b.start) {
        return a.start < b.start;
    }
    if (kind == MatchKind.LeftmostLongest) {
        const len_a = a.end - a.start;
        const len_b = b.end - b.start;
        if (len_a != len_b) {
            return len_a > len_b;
        }
    }
    return a.pattern_id < b.pattern_id;
}

fn sortMatches(items: []AhoMatch, kind: MatchKind) {
    if (items.len <= 1) {
        return;
    }
    quickSortMatches(items, 0, items.len - 1, kind);
}

fn quickSortMatches(items: []AhoMatch, lo: usize, hi: usize, kind: MatchKind) {
    if (lo >= hi) {
        return;
    }
    const pivot = items[lo + (hi - lo) / 2];
    let i = lo;
    let j = hi;
    while (true) {
        while (lessMatch(items[i], pivot, kind)) : (i += 1) {}
        while (lessMatch(pivot, items[j], kind)) : (j -= 1) {}
        if (i >= j) {
            break;
        }
        const tmp = items[i];
        items[i] = items[j];
        items[j] = tmp;
        i += 1;
        j -= 1;
    }
    if (lo < j) {
        quickSortMatches(items, lo, j, kind);
    }
    if (j + 1 < hi) {
        quickSortMatches(items, j + 1, hi, kind);
    }
}

/// Multi-pattern Aho-Corasick automaton with O(N) DFA scanning.
pub struct AhoCorasick {
    alloc: std.mem.Allocator;
    case_insensitive: bool = false;
    built: bool = false;
    states: std.ArrayList(AhoState) = .empty;
    outputs: std.ArrayList(OutputNode) = .empty;
    pattern_lens: std.ArrayList(usize) = .empty;

    /// Initialize a case-sensitive Aho-Corasick automaton.
    pub static fn init(alloc: std.mem.Allocator): AhoCorasick {
        return AhoCorasick{
            alloc: alloc,
            case_insensitive: false,
            built: false,
            states: .empty,
            outputs: .empty,
            pattern_lens: .empty,
        };
    }

    /// Initialize an ASCII case-insensitive Aho-Corasick automaton.
    pub static fn initCaseInsensitive(alloc: std.mem.Allocator): AhoCorasick {
        return AhoCorasick{
            alloc: alloc,
            case_insensitive: true,
            built: false,
            states: .empty,
            outputs: .empty,
            pattern_lens: .empty,
        };
    }

    /// Release all automaton states, output chains, and pattern metadata.
    pub fn deinit(*_) {
        _.pattern_lens.deinit(_.alloc);
        _.outputs.deinit(_.alloc);
        _.states.deinit(_.alloc);
    }

    /// Return the number of registered patterns.
    pub fn patternCount(): usize {
        return _.pattern_lens.items.len;
    }

    /// Return the number of states in the automaton.
    pub fn stateCount(): usize {
        return _.states.items.len;
    }

    fn ensureRoot(*_): void ! AhoError {
        if (_.states.items.len == 0) {
            _.states.append(_.alloc, makeEmptyState()) catch return AhoError.OutOfMemory;
        }
    }

    /// Register a single pattern and return its assigned 0-based `pattern_id`.
    pub fn addPattern(*_, pattern: []const u8): usize ! AhoError {
        if (pattern.len == 0) {
            return AhoError.EmptyPattern;
        }
        try _.ensureRoot();

        let cur: u32 = 0;
        for (raw_b of pattern) {
            const b = if (_.case_insensitive) asciiLower(raw_b) else raw_b;
            let next_state = _.states.items[usize(cur)].trie_next[usize(b)];
            if (next_state == NONE_STATE) {
                next_state = _.states.items.len as u32;
                _.states.append(_.alloc, makeEmptyState()) catch return AhoError.OutOfMemory;
                _.states.items[usize(cur)].trie_next[usize(b)] = next_state;
            }
            cur = next_state;
        }

        const pid = _.pattern_lens.items.len;
        _.pattern_lens.append(_.alloc, pattern.len) catch return AhoError.OutOfMemory;

        const out_idx = _.outputs.items.len as u32;
        _.outputs.append(_.alloc, OutputNode{
            pattern_id: pid as u32,
            prev: _.states.items[usize(cur)].out_head,
        }) catch {
            * = _.pattern_lens.pop();
            return AhoError.OutOfMemory;
        };
        _.states.items[usize(cur)].out_head = out_idx;
        _.built = false;

        return pid;
    }

    /// Register multiple patterns in order.
    pub fn addPatterns(*_, patterns: []const []const u8): void ! AhoError {
        for (p of patterns) {
            * = try _.addPattern(p);
        }
    }

    /// Build BFS failure links, dictionary suffix links, and DFA transition tables.
    pub fn build(*_): void ! AhoError {
        if (_.pattern_lens.items.len == 0) {
            return AhoError.NoPatterns;
        }
        try _.ensureRoot();

        let queue: std.ArrayList(u32) = .empty;
        defer queue.deinit(_.alloc);

        // Initialize root state transitions and depth-1 failure links.
        _.states.items[0].fail = 0;
        _.states.items[0].dict_link = NONE_STATE;
        let c: usize = 0;
        while (c < 256) : (c += 1) {
            const child = _.states.items[0].trie_next[c];
            if (child != NONE_STATE) {
                _.states.items[0].dfa_next[c] = child;
                _.states.items[usize(child)].fail = 0;
                _.states.items[usize(child)].dict_link = if (_.states.items[0].out_head != NONE_OUT) 0 else NONE_STATE;
                queue.append(_.alloc, child) catch return AhoError.OutOfMemory;
            } else {
                _.states.items[0].dfa_next[c] = 0;
            }
        }

        // BFS over Trie to compute failure links, dictionary suffix links, and full DFA.
        let q_head: usize = 0;
        while (q_head < queue.items.len) {
            const u = queue.items[q_head];
            q_head += 1;
            const f = _.states.items[usize(u)].fail;

            let ch: usize = 0;
            while (ch < 256) : (ch += 1) {
                const v = _.states.items[usize(u)].trie_next[ch];
                if (v != NONE_STATE) {
                    _.states.items[usize(u)].dfa_next[ch] = v;
                    const vf = _.states.items[usize(f)].dfa_next[ch];
                    _.states.items[usize(v)].fail = vf;
                    if (_.states.items[usize(vf)].out_head != NONE_OUT) {
                        _.states.items[usize(v)].dict_link = vf;
                    } else {
                        _.states.items[usize(v)].dict_link = _.states.items[usize(vf)].dict_link;
                    }
                    queue.append(_.alloc, v) catch return AhoError.OutOfMemory;
                } else {
                    _.states.items[usize(u)].dfa_next[ch] = _.states.items[usize(f)].dfa_next[ch];
                }
            }
        }

        _.built = true;
    }

    /// Scan `text` in O(N) time and return all overlapping matches.
    pub fn findAll(alloc: std.mem.Allocator, text: []const u8): ([]AhoMatch) ! AhoError {
        if (!_.built) {
            return AhoError.PatternNotCompiled;
        }
        if (text.len == 0) {
            return alloc.alloc(AhoMatch, 0) catch return AhoError.OutOfMemory;
        }

        let matches: std.ArrayList(AhoMatch) = .empty;
        errdefer matches.deinit(alloc);

        let state: u32 = 0;
        let i: usize = 0;
        while (i < text.len) : (i += 1) {
            const b = if (_.case_insensitive) asciiLower(text[i]) else text[i];
            state = _.states.items[usize(state)].dfa_next[usize(b)];

            let cur_state = state;
            while (cur_state != NONE_STATE) {
                let out_idx = _.states.items[usize(cur_state)].out_head;
                while (out_idx != NONE_OUT) {
                    const node = _.outputs.items[usize(out_idx)];
                    const pid = usize(node.pattern_id);
                    const plen = _.pattern_lens.items[pid];
                    const end_pos = i + 1;
                    matches.append(alloc, AhoMatch{
                        pattern_id: pid,
                        start: end_pos - plen,
                        end: end_pos,
                    }) catch return AhoError.OutOfMemory;
                    out_idx = node.prev;
                }
                cur_state = _.states.items[usize(cur_state)].dict_link;
            }
        }

        return matches.toOwnedSlice(alloc) catch return AhoError.OutOfMemory;
    }

    /// Find non-overlapping matches using `LeftmostFirst` or `LeftmostLongest` semantics.
    pub fn findLeftmost(
        alloc: std.mem.Allocator,
        text: []const u8,
        kind: MatchKind,
    ): ([]AhoMatch) ! AhoError {
        if (!_.built) {
            return AhoError.PatternNotCompiled;
        }
        if (kind == MatchKind.AllOverlapping) {
            return AhoError.InvalidOption;
        }
        if (text.len == 0) {
            return alloc.alloc(AhoMatch, 0) catch return AhoError.OutOfMemory;
        }

        const all = try _.findAll(alloc, text);
        defer alloc.free(all);

        if (all.len == 0) {
            return alloc.alloc(AhoMatch, 0) catch return AhoError.OutOfMemory;
        }

        sortMatches(all, kind);

        let selected: std.ArrayList(AhoMatch) = .empty;
        errdefer selected.deinit(alloc);

        let cursor: usize = 0;
        for (m of all) {
            if (m.start >= cursor) {
                selected.append(alloc, m) catch return AhoError.OutOfMemory;
                cursor = m.end;
            }
        }

        return selected.toOwnedSlice(alloc) catch return AhoError.OutOfMemory;
    }

    /// Zero-allocation check returning `true` immediately on the first match.
    pub fn containsAny(text: []const u8): bool {
        if (!_.built || text.len == 0) {
            return false;
        }
        let state: u32 = 0;
        for (raw_b of text) {
            const b = if (_.case_insensitive) asciiLower(raw_b) else raw_b;
            state = _.states.items[usize(state)].dfa_next[usize(b)];
            if (_.states.items[usize(state)].out_head != NONE_OUT ||
                _.states.items[usize(state)].dict_link != NONE_STATE)
            {
                return true;
            }
        }
        return false;
    }

    /// Replace all non-overlapping matches (using `LeftmostLongest`) with `replacement`.
    pub fn replaceAll(
        alloc: std.mem.Allocator,
        text: []const u8,
        replacement: []const u8,
    ): ([]u8) ! AhoError {
        if (!_.built) {
            return AhoError.PatternNotCompiled;
        }
        if (text.len == 0) {
            return alloc.dupe(u8, text) catch return AhoError.OutOfMemory;
        }

        const matches = try _.findLeftmost(alloc, text, MatchKind.LeftmostLongest);
        defer alloc.free(matches);

        if (matches.len == 0) {
            return alloc.dupe(u8, text) catch return AhoError.OutOfMemory;
        }

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let pos: usize = 0;
        for (m of matches) {
            if (m.start > pos) {
                out.appendSlice(alloc, text[pos..m.start]) catch return AhoError.OutOfMemory;
            }
            out.appendSlice(alloc, replacement) catch return AhoError.OutOfMemory;
            pos = m.end;
        }
        if (pos < text.len) {
            out.appendSlice(alloc, text[pos..text.len]) catch return AhoError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return AhoError.OutOfMemory;
    }
}

/// Builder helper for constructing a compiled `AhoCorasick` automaton.
pub struct AhoBuilder {
    alloc: std.mem.Allocator;
    case_insensitive: bool = false;
    patterns: std.ArrayList([]u8) = .empty;

    /// Create a case-sensitive `AhoBuilder`.
    pub static fn init(alloc: std.mem.Allocator): AhoBuilder {
        return AhoBuilder{
            alloc: alloc,
            case_insensitive: false,
            patterns: .empty,
        };
    }

    /// Create an ASCII case-insensitive `AhoBuilder`.
    pub static fn initCaseInsensitive(alloc: std.mem.Allocator): AhoBuilder {
        return AhoBuilder{
            alloc: alloc,
            case_insensitive: true,
            patterns: .empty,
        };
    }

    /// Free all buffered pattern copies.
    pub fn deinit(*_) {
        for (p of _.patterns.items) {
            _.alloc.free(p);
        }
        _.patterns.deinit(_.alloc);
    }

    /// Add a single pattern to the builder and return its 0-based `pattern_id`.
    pub fn addPattern(*_, pattern: []const u8): usize ! AhoError {
        if (pattern.len == 0) {
            return AhoError.EmptyPattern;
        }
        const copy = _.alloc.dupe(u8, pattern) catch return AhoError.OutOfMemory;
        errdefer _.alloc.free(copy);
        const pid = _.patterns.items.len;
        _.patterns.append(_.alloc, copy) catch return AhoError.OutOfMemory;
        return pid;
    }

    /// Add multiple patterns to the builder.
    pub fn addPatterns(*_, patterns: []const []const u8): void ! AhoError {
        for (p of patterns) {
            * = try _.addPattern(p);
        }
    }

    /// Return the number of patterns currently staged in the builder.
    pub fn patternCount(): usize {
        return _.patterns.items.len;
    }

    /// Compile and return an `AhoCorasick` automaton from the staged patterns.
    pub fn build(*_): AhoCorasick ! AhoError {
        if (_.patterns.items.len == 0) {
            return AhoError.NoPatterns;
        }
        let ac = if (_.case_insensitive)
            AhoCorasick.initCaseInsensitive(_.alloc)
        else
            AhoCorasick.init(_.alloc);
        errdefer ac.deinit();

        for (p of _.patterns.items) {
            * = try ac.addPattern(p);
        }
        try ac.build();
        return ac;
    }
}

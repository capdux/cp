// Reference-hardened against BurntSushi/aho-corasick (MIT/Unlicense)
// Source: cp/lib/ds/ahoCorasick.cp (Pure CP Aho-Corasick Multi-Pattern Automaton + cap:simd Prefilter)
// Features:
//   - 3 MatchKind semantics: Standard, LeftmostFirst, LeftmostLongest
//   - Overlapping & Non-Overlapping zero-heap external-slice scans (`findAllNonOverlapping`, `findAllOverlappingInto`)
//   - Single-pass zero-heap multi-pattern replacement (`replaceAllInto`)
//   - ASCII case-insensitive matching (`ascii_case_insensitive`)
//   - Empty-pattern, duplicate-pattern, single-byte & prefix-chain exact handling (zero infinite loops)
//   - Hot-path `cap:simd` root-state prefiltering (`Simd.indexOfByte` / `indexOfAny2` / `indexOfAny4`)

const std = @import("std");
import { Simd } from "cap:simd";

const NO_PAT: u32 = 0xFFFF_FFFF;

/// Match semantics controlling how overlapping or ambiguous pattern matches are resolved.
pub enum MatchKind {
    /// Standard Aho-Corasick: reports the earliest-ending match (and among same-end, first registered).
    Standard,
    /// Leftmost-first (Perl/regex-like): reports the earliest-starting match; ties broken by lowest `pattern_id`.
    LeftmostFirst,
    /// Leftmost-longest (POSIX/lexer-like): reports the earliest-starting match; ties broken by longest length, then lowest `pattern_id`.
    LeftmostLongest,
}

/// Error set returned by zero-heap replacement and automaton construction helpers.
pub err AcError {
    BufferTooSmall,
    ReplacementCountMismatch,
}

/// Configuration options for `AhoCorasick.initWithOptions`.
pub struct AhoCorasickOptions {
    match_kind: MatchKind = MatchKind.Standard;
    ascii_case_insensitive: bool = false;
    enable_prefilter: bool = true;
    ignore_empty_patterns: bool = false;
}

/// Single match span `[start, end)` for pattern `pattern_id`.
pub const Match = struct {
    pattern_id: usize,
    start: usize,
    end: usize,
};

const DupEntry = struct {
    pattern_id: u32,
    pattern_len: usize,
    next: u32,
};

const Node = struct {
    next: [256]u32,
    fail: u32,
    out_link: u32,
    pattern_id: u32,
    pattern_len: usize,
    depth: usize,
    dup_head: u32,

    static fn create(depth: usize): Node {
        let n: Node = undefined;
        @memset(&n.next, 0);
        n.fail = 0;
        n.out_link = 0;
        n.pattern_id = NO_PAT;
        n.pattern_len = 0;
        n.depth = depth;
        n.dup_head = NO_PAT;
        return n;
    }
};

fn asciiLower(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b + 32;
    return b;
}

fn copyBytes(dst: *[]u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

/// Multi-pattern Aho-Corasick automaton with `Standard` / `LeftmostFirst` / `LeftmostLongest`
/// semantics, ASCII case-insensitivity, zero-heap scan/replace APIs, and `cap:simd` prefiltering.
pub const AhoCorasick = struct {
    nodes: std.ArrayList(Node),
    dups: std.ArrayList(DupEntry),
    pattern_count: usize,
    match_kind: MatchKind,
    ascii_case_insensitive: bool,
    has_empty_pattern: bool,
    empty_pattern_id: u32,
    prefilter_bytes: [4]u8,
    prefilter_len: u8,

    /// Backward-compatible constructor (Standard semantics, case-sensitive, ignores empty patterns).
    pub fn init(alloc: std.mem.Allocator, patterns: []const []const u8): !AhoCorasick {
        return AhoCorasick.initWithOptions(alloc, patterns, AhoCorasickOptions{
            match_kind: MatchKind.Standard,
            ascii_case_insensitive: false,
            enable_prefilter: true,
            ignore_empty_patterns: true,
        });
    }

    /// Full-featured constructor supporting `MatchKind`, `ascii_case_insensitive`, `enable_prefilter`,
    /// empty patterns, and duplicate patterns.
    pub static fn initWithOptions(
        alloc: std.mem.Allocator,
        patterns: []const []const u8,
        opts: AhoCorasickOptions,
    ): !AhoCorasick {
        let nodes = std.ArrayList(Node).empty;
        errdefer nodes.deinit(alloc);
        let dups = std.ArrayList(DupEntry).empty;
        errdefer dups.deinit(alloc);

        try nodes.append(alloc, Node.create(0));

        let has_empty: bool = false;
        let empty_pid: u32 = NO_PAT;

        let pid: usize = 0;
        while (pid < patterns.len) : (pid += 1) {
            const pat = patterns[pid];
            if (pat.len == 0) {
                if (opts.ignore_empty_patterns) continue;
                const pid32: u32 = @intCast(pid);
                if (!has_empty) {
                    has_empty = true;
                    empty_pid = pid32;
                }
                if (nodes.items[0].pattern_id == NO_PAT) {
                    nodes.items[0].pattern_id = pid32;
                    nodes.items[0].pattern_len = 0;
                } else {
                    const dup_idx: u32 = @intCast(dups.items.len);
                    try dups.append(alloc, DupEntry{
                        .pattern_id = pid32,
                        .pattern_len = 0,
                        .next = NO_PAT,
                    });
                    if (nodes.items[0].dup_head == NO_PAT) {
                        nodes.items[0].dup_head = dup_idx;
                    } else {
                        let tail: usize = @intCast(nodes.items[0].dup_head);
                        while (dups.items[tail].next != NO_PAT) {
                            tail = @intCast(dups.items[tail].next);
                        }
                        dups.items[tail].next = dup_idx;
                    }
                }
                continue;
            }

            let cur: usize = 0;
            let depth: usize = 0;
            for (raw_b of pat) {
                const b: u8 = if (opts.ascii_case_insensitive) asciiLower(raw_b) else raw_b;
                const b_idx: usize = @intCast(b);
                depth += 1;
                if (nodes.items[cur].next[b_idx] == 0) {
                    const new_idx: u32 = @intCast(nodes.items.len);
                    try nodes.append(alloc, Node.create(depth));
                    nodes.items[cur].next[b_idx] = new_idx;
                }
                cur = @intCast(nodes.items[cur].next[b_idx]);
            }

            const pid32: u32 = @intCast(pid);
            if (nodes.items[cur].pattern_id == NO_PAT) {
                nodes.items[cur].pattern_id = pid32;
                nodes.items[cur].pattern_len = pat.len;
            } else {
                const dup_idx: u32 = @intCast(dups.items.len);
                try dups.append(alloc, DupEntry{
                    .pattern_id = pid32,
                    .pattern_len = pat.len,
                    .next = NO_PAT,
                });
                if (nodes.items[cur].dup_head == NO_PAT) {
                    nodes.items[cur].dup_head = dup_idx;
                } else {
                    let tail: usize = @intCast(nodes.items[cur].dup_head);
                    while (dups.items[tail].next != NO_PAT) {
                        tail = @intCast(dups.items[tail].next);
                    }
                    dups.items[tail].next = dup_idx;
                }
            }
        }

        // Build failure & dictionary suffix (`out_link`) chains via BFS
        let queue = std.ArrayList(u32).empty;
        defer queue.deinit(alloc);

        let c: usize = 0;
        while (c < 256) : (c += 1) {
            const child = nodes.items[0].next[c];
            if (child != 0) {
                const ch_idx: usize = @intCast(child);
                nodes.items[ch_idx].fail = 0;
                try queue.append(alloc, child);
            }
        }

        let head: usize = 0;
        while (head < queue.items.len) : (head += 1) {
            const u_idx: usize = @intCast(queue.items[head]);
            let ch: usize = 0;
            while (ch < 256) : (ch += 1) {
                const v = nodes.items[u_idx].next[ch];
                if (v == 0) continue;
                const v_idx: usize = @intCast(v);
                let f = nodes.items[u_idx].fail;
                while (f != 0) {
                    const fi: usize = @intCast(f);
                    if (nodes.items[fi].next[ch] != 0) break;
                    f = nodes.items[fi].fail;
                }
                const f_idx: usize = @intCast(f);
                const target = nodes.items[f_idx].next[ch];
                nodes.items[v_idx].fail = target;
                const t_idx: usize = @intCast(target);
                if (target != 0 && nodes.items[t_idx].pattern_id != NO_PAT) {
                    nodes.items[v_idx].out_link = target;
                } else {
                    nodes.items[v_idx].out_link = nodes.items[t_idx].out_link;
                }
                try queue.append(alloc, v);
            }
        }

        // Build cap:simd prefilter byte set when root transitions cover 1..4 distinct raw bytes
        let pf_bytes: [4]u8 = [0; 4];
        let pf_count: usize = 0;
        let pf_overflow: bool = false;
        if (opts.enable_prefilter && !has_empty) {
            let rc: usize = 0;
            while (rc < 256) : (rc += 1) {
                if (nodes.items[0].next[rc] == 0) continue;
                const rb: u8 = @intCast(rc);
                if (pf_count < 4) {
                    pf_bytes[pf_count] = rb;
                    pf_count += 1;
                } else {
                    pf_overflow = true;
                    break;
                }
                if (opts.ascii_case_insensitive && rb >= 'a' && rb <= 'z') {
                    if (pf_count < 4) {
                        pf_bytes[pf_count] = rb - 32;
                        pf_count += 1;
                    } else {
                        pf_overflow = true;
                        break;
                    }
                }
            }
        }
        const pf_len: u8 = if (pf_overflow || pf_count == 0 || pf_count > 4) 0 else @intCast(pf_count);

        return AhoCorasick{
            .nodes = nodes,
            .dups = dups,
            .pattern_count = patterns.len,
            .match_kind = opts.match_kind,
            .ascii_case_insensitive = opts.ascii_case_insensitive,
            .has_empty_pattern = has_empty,
            .empty_pattern_id = empty_pid,
            .prefilter_bytes = pf_bytes,
            .prefilter_len = pf_len,
        };
    }

    pub fn deinit(self: *AhoCorasick, alloc: std.mem.Allocator) {
        self.nodes.deinit(alloc);
        self.dups.deinit(alloc);
    }

    /// Returns `true` if `cap:simd` root-state prefiltering is active for this automaton.
    pub fn prefilterActive(self: AhoCorasick): bool {
        return self.prefilter_len > 0;
    }

    /// Returns the number of target start bytes accelerated by `cap:simd` prefiltering (`0..4`).
    pub fn prefilterByteCount(self: AhoCorasick): usize {
        return @intCast(self.prefilter_len);
    }

    fn prefilterNext(self: AhoCorasick, rem: []const u8): usize? {
        if (self.prefilter_len == 1) {
            return Simd.indexOfByte(rem, self.prefilter_bytes[0]);
        }
        if (self.prefilter_len == 2) {
            return Simd.indexOfAny2(rem, self.prefilter_bytes[0], self.prefilter_bytes[1]);
        }
        if (self.prefilter_len == 3) {
            return Simd.indexOfAny4(
                rem,
                self.prefilter_bytes[0],
                self.prefilter_bytes[1],
                self.prefilter_bytes[2],
                self.prefilter_bytes[2],
            );
        }
        if (self.prefilter_len == 4) {
            return Simd.indexOfAny4(
                rem,
                self.prefilter_bytes[0],
                self.prefilter_bytes[1],
                self.prefilter_bytes[2],
                self.prefilter_bytes[3],
            );
        }
        return 0;
    }

    fn stepState(self: AhoCorasick, state_in: usize, raw_byte: u8): usize {
        const b: u8 = if (self.ascii_case_insensitive) asciiLower(raw_byte) else raw_byte;
        const b_idx: usize = @intCast(b);
        let state: usize = state_in;
        while (state != 0 && self.nodes.items[state].next[b_idx] == 0) {
            state = @intCast(self.nodes.items[state].fail);
        }
        return @intCast(self.nodes.items[state].next[b_idx]);
    }

    /// Finds the next match starting at or after `search_start` under `self.match_kind`.
    /// When `allow_empty_at_start == false`, zero-width matches at `search_start` are skipped
    /// (preventing an empty match immediately following a non-empty match, per BurntSushi/aho-corasick).
    pub fn findNextAt(
        self: AhoCorasick,
        text: []const u8,
        search_start: usize,
        allow_empty_at_start: bool,
    ): Match? {
        if (search_start > text.len) return null;
        if (self.nodes.items.len == 1 && !self.has_empty_pattern) return null;

        if (self.match_kind == MatchKind.Standard) {
            if (self.has_empty_pattern && allow_empty_at_start) {
                return Match{
                    .pattern_id = @intCast(self.empty_pattern_id),
                    .start = search_start,
                    .end = search_start,
                };
            }
            let state: usize = 0;
            let i: usize = search_start;
            while (i < text.len) {
                if (state == 0 && self.prefilter_len > 0) {
                    const off_opt: usize? = self.prefilterNext(text[i..]);
                    if (off_opt == null) return null;
                    if (off := off_opt) {
                        i += off;
                    }
                }
                state = self.stepState(state, text[i]);
                // Pick best match ending at i + 1 (lowest pattern_id along node + out_link)
                let found: bool = false;
                let best_m: Match = Match{ .pattern_id = 0, .start = 0, .end = 0 };
                let cur: usize = state;
                while (cur != 0) {
                    const nd = self.nodes.items[cur];
                    if (nd.pattern_id != NO_PAT) {
                        const m_start: usize = i + 1 - nd.pattern_len;
                        if (m_start >= search_start) {
                            const cand = Match{
                                .pattern_id = @intCast(nd.pattern_id),
                                .start = m_start,
                                .end = i + 1,
                            };
                            if (!found || cand.pattern_id < best_m.pattern_id) {
                                best_m = cand;
                                found = true;
                            }
                        }
                    }
                    cur = @intCast(nd.out_link);
                }
                if (found) return best_m;
                if (self.has_empty_pattern) {
                    return Match{
                        .pattern_id = @intCast(self.empty_pattern_id),
                        .start = i + 1,
                        .end = i + 1,
                    };
                }
                i += 1;
            }
            return null;
        }

        // LeftmostFirst & LeftmostLongest: exact linear-time failure-depth bounded scan
        let has_best: bool = false;
        let best: Match = Match{ .pattern_id = 0, .start = 0, .end = 0 };

        if (self.has_empty_pattern && allow_empty_at_start) {
            has_best = true;
            best = Match{
                .pattern_id = @intCast(self.empty_pattern_id),
                .start = search_start,
                .end = search_start,
            };
            if (self.match_kind == MatchKind.LeftmostFirst && best.pattern_id == 0) {
                return best;
            }
        }

        let state: usize = 0;
        let i: usize = search_start;
        while (i < text.len) {
            if (state == 0 && !has_best && self.prefilter_len > 0) {
                const off_opt: usize? = self.prefilterNext(text[i..]);
                if (off_opt == null) return null;
                if (off := off_opt) {
                    i += off;
                }
            }
            state = self.stepState(state, text[i]);

            // Evaluate all matches terminating at i + 1 whose start >= search_start
            let cur: usize = state;
            while (cur != 0) {
                const nd = self.nodes.items[cur];
                if (nd.pattern_id != NO_PAT) {
                    const m_start: usize = i + 1 - nd.pattern_len;
                    if (m_start >= search_start) {
                        const cand = Match{
                            .pattern_id = @intCast(nd.pattern_id),
                            .start = m_start,
                            .end = i + 1,
                        };
                        if (!has_best) {
                            best = cand;
                            has_best = true;
                        } else if (cand.start < best.start) {
                            best = cand;
                        } else if (cand.start == best.start) {
                            if (self.match_kind == MatchKind.LeftmostFirst) {
                                if (cand.pattern_id < best.pattern_id) {
                                    best = cand;
                                }
                            } else {
                                // LeftmostLongest
                                if (cand.end > best.end || (cand.end == best.end && cand.pattern_id < best.pattern_id)) {
                                    best = cand;
                                }
                            }
                        }
                    }
                }
                cur = @intCast(nd.out_link);
            }

            // Consider empty pattern at boundary i + 1 (only if strictly after search_start when !allow_empty_at_start)
            if (self.has_empty_pattern) {
                const emp_pos: usize = i + 1;
                if (!has_best) {
                    best = Match{
                        .pattern_id = @intCast(self.empty_pattern_id),
                        .start = emp_pos,
                        .end = emp_pos,
                    };
                    has_best = true;
                } else if (emp_pos < best.start) {
                    best = Match{
                        .pattern_id = @intCast(self.empty_pattern_id),
                        .start = emp_pos,
                        .end = emp_pos,
                    };
                } else if (emp_pos == best.start && self.match_kind == MatchKind.LeftmostFirst) {
                    if (@as(usize, @intCast(self.empty_pattern_id)) < best.pattern_id) {
                        best = Match{
                            .pattern_id = @intCast(self.empty_pattern_id),
                            .start = emp_pos,
                            .end = emp_pos,
                        };
                    }
                }
            }

            // Early termination: earliest possible start of any future match is (i + 1) - nodes[state].depth
            if (has_best) {
                const min_future_start: usize = (i + 1) - self.nodes.items[state].depth;
                if (min_future_start > best.start) {
                    return best;
                }
                if (self.match_kind == MatchKind.LeftmostFirst && best.start == search_start && best.pattern_id == 0) {
                    return best;
                }
            }
            i += 1;
        }

        if (has_best) return best;
        return null;
    }

    /// Finds the first match in `text` under `self.match_kind`.
    pub fn findFirst(self: AhoCorasick, text: []const u8): Match? {
        return self.findNextAt(text, 0, true);
    }

    /// Zero-heap non-overlapping scan populating matches into caller-owned `out_matches`.
    /// Returns the number of matches written (stops cleanly when `out_matches` is full).
    pub fn findAllNonOverlapping(
        self: AhoCorasick,
        text: []const u8,
        out_matches: *[]Match,
    ): usize {
        if (out_matches.len == 0) return 0;
        let count: usize = 0;
        let pos: usize = 0;
        let allow_empty: bool = true;
        while (pos <= text.len && count < out_matches.len) {
            const m_opt: Match? = self.findNextAt(text, pos, allow_empty);
            if (m_opt == null) break;
            if (m := m_opt) {
                out_matches[count] = m;
                count += 1;
                if (m.end > m.start) {
                    pos = m.end;
                    allow_empty = false;
                } else {
                    if (m.end >= text.len) break;
                    pos = m.end + 1;
                    allow_empty = true;
                }
            }
        }
        return count;
    }

    /// Zero-heap overlapping scan populating all overlapping matches (including duplicate patterns
    /// and full `out_link` dictionary suffix chains) into caller-owned `out_matches`.
    pub fn findAllOverlappingInto(
        self: AhoCorasick,
        text: []const u8,
        out_matches: *[]Match,
    ): usize {
        if (out_matches.len == 0) return 0;
        if (self.nodes.items.len == 1 && !self.has_empty_pattern) return 0;
        let count: usize = 0;

        // Emit empty-pattern matches at position 0 if present
        if (self.has_empty_pattern && self.nodes.items[0].pattern_id != NO_PAT) {
            if (count < out_matches.len) {
                out_matches[count] = Match{
                    .pattern_id = @intCast(self.nodes.items[0].pattern_id),
                    .start = 0,
                    .end = 0,
                };
                count += 1;
            }
            let d: u32 = self.nodes.items[0].dup_head;
            while (d != NO_PAT && count < out_matches.len) {
                const de = self.dups.items[@as(usize, @intCast(d))];
                out_matches[count] = Match{
                    .pattern_id = @intCast(de.pattern_id),
                    .start = 0,
                    .end = 0,
                };
                count += 1;
                d = de.next;
            }
        }

        let state: usize = 0;
        let i: usize = 0;
        while (i < text.len && count < out_matches.len) {
            if (state == 0 && self.prefilter_len > 0) {
                const off_opt: usize? = self.prefilterNext(text[i..]);
                if (off_opt == null) break;
                if (off := off_opt) {
                    i += off;
                }
            }
            state = self.stepState(state, text[i]);

            let cur: usize = state;
            while (cur != 0 && count < out_matches.len) {
                const nd = self.nodes.items[cur];
                if (nd.pattern_id != NO_PAT) {
                    out_matches[count] = Match{
                        .pattern_id = @intCast(nd.pattern_id),
                        .start = i + 1 - nd.pattern_len,
                        .end = i + 1,
                    };
                    count += 1;
                    let d: u32 = nd.dup_head;
                    while (d != NO_PAT && count < out_matches.len) {
                        const de = self.dups.items[@as(usize, @intCast(d))];
                        out_matches[count] = Match{
                            .pattern_id = @intCast(de.pattern_id),
                            .start = i + 1 - de.pattern_len,
                            .end = i + 1,
                        };
                        count += 1;
                        d = de.next;
                    }
                }
                cur = @intCast(nd.out_link);
            }

            if (self.has_empty_pattern && self.nodes.items[0].pattern_id != NO_PAT && count < out_matches.len) {
                out_matches[count] = Match{
                    .pattern_id = @intCast(self.nodes.items[0].pattern_id),
                    .start = i + 1,
                    .end = i + 1,
                };
                count += 1;
                let d: u32 = self.nodes.items[0].dup_head;
                while (d != NO_PAT && count < out_matches.len) {
                    const de = self.dups.items[@as(usize, @intCast(d))];
                    out_matches[count] = Match{
                        .pattern_id = @intCast(de.pattern_id),
                        .start = i + 1,
                        .end = i + 1,
                    };
                    count += 1;
                    d = de.next;
                }
            }
            i += 1;
        }
        return count;
    }

    /// Zero-heap single-pass multi-pattern replacement into caller-owned `out_buf`.
    /// Uses `self.match_kind` non-overlapping semantics and returns total bytes written.
    pub fn replaceAllInto(
        self: AhoCorasick,
        text: []const u8,
        replacements: []const []const u8,
        out_buf: *[]u8,
    ): usize ! AcError {
        if (replacements.len < self.pattern_count) return AcError.ReplacementCountMismatch;
        let out_pos: usize = 0;
        let last_copied: usize = 0;
        let search_pos: usize = 0;
        let allow_empty: bool = true;

        while (search_pos <= text.len) {
            const m_opt: Match? = self.findNextAt(text, search_pos, allow_empty);
            if (m_opt == null) break;
            if (m := m_opt) {
                const gap_len: usize = m.start - last_copied;
                if (out_pos + gap_len > out_buf.len) return AcError.BufferTooSmall;
                if (gap_len > 0) {
                    copyBytes(out_buf, out_pos, text[last_copied..m.start]);
                    out_pos += gap_len;
                }
                const rep: []const u8 = replacements[m.pattern_id];
                if (out_pos + rep.len > out_buf.len) return AcError.BufferTooSmall;
                if (rep.len > 0) {
                    copyBytes(out_buf, out_pos, rep);
                    out_pos += rep.len;
                }
                last_copied = m.end;
                if (m.end > m.start) {
                    search_pos = m.end;
                    allow_empty = false;
                } else {
                    if (m.end >= text.len) break;
                    search_pos = m.end + 1;
                    allow_empty = true;
                }
            }
        }

        const tail_len: usize = text.len - last_copied;
        if (out_pos + tail_len > out_buf.len) return AcError.BufferTooSmall;
        if (tail_len > 0) {
            copyBytes(out_buf, out_pos, text[last_copied..text.len]);
            out_pos += tail_len;
        }
        return out_pos;
    }

    /// Backward-compatible allocator-backed overlapping scan.
    pub fn findAllOverlapping(self: AhoCorasick, alloc: std.mem.Allocator, text: []const u8): ![]Match {
        let out = std.ArrayList(Match).empty;
        errdefer out.deinit(alloc);

        if (self.has_empty_pattern && self.nodes.items[0].pattern_id != NO_PAT) {
            try out.append(alloc, Match{
                .pattern_id = @intCast(self.nodes.items[0].pattern_id),
                .start = 0,
                .end = 0,
            });
            let d: u32 = self.nodes.items[0].dup_head;
            while (d != NO_PAT) {
                const de = self.dups.items[@as(usize, @intCast(d))];
                try out.append(alloc, Match{
                    .pattern_id = @intCast(de.pattern_id),
                    .start = 0,
                    .end = 0,
                });
                d = de.next;
            }
        }

        let state: usize = 0;
        let i: usize = 0;
        while (i < text.len) {
            if (state == 0 && self.prefilter_len > 0) {
                const off_opt: usize? = self.prefilterNext(text[i..]);
                if (off_opt == null) break;
                if (off := off_opt) {
                    i += off;
                }
            }
            state = self.stepState(state, text[i]);

            let cur: usize = state;
            while (cur != 0) {
                const nd = self.nodes.items[cur];
                if (nd.pattern_id != NO_PAT) {
                    try out.append(alloc, Match{
                        .pattern_id = @intCast(nd.pattern_id),
                        .start = i + 1 - nd.pattern_len,
                        .end = i + 1,
                    });
                    let d: u32 = nd.dup_head;
                    while (d != NO_PAT) {
                        const de = self.dups.items[@as(usize, @intCast(d))];
                        try out.append(alloc, Match{
                            .pattern_id = @intCast(de.pattern_id),
                            .start = i + 1 - de.pattern_len,
                            .end = i + 1,
                        });
                        d = de.next;
                    }
                }
                cur = @intCast(nd.out_link);
            }

            if (self.has_empty_pattern && self.nodes.items[0].pattern_id != NO_PAT) {
                try out.append(alloc, Match{
                    .pattern_id = @intCast(self.nodes.items[0].pattern_id),
                    .start = i + 1,
                    .end = i + 1,
                });
                let d: u32 = self.nodes.items[0].dup_head;
                while (d != NO_PAT) {
                    const de = self.dups.items[@as(usize, @intCast(d))];
                    try out.append(alloc, Match{
                        .pattern_id = @intCast(de.pattern_id),
                        .start = i + 1,
                        .end = i + 1,
                    });
                    d = de.next;
                }
            }
            i += 1;
        }
        return out.toOwnedSlice(alloc);
    }

    /// Fast zero-alloc existence check (accelerated by `cap:simd` prefilter when active).
    pub fn containsAny(self: AhoCorasick, text: []const u8): bool {
        if (self.has_empty_pattern) return true;
        if (self.nodes.items.len == 1) return false;
        let state: usize = 0;
        let i: usize = 0;
        while (i < text.len) {
            if (state == 0 && self.prefilter_len > 0) {
                const off_opt: usize? = self.prefilterNext(text[i..]);
                if (off_opt == null) return false;
                if (off := off_opt) {
                    i += off;
                }
            }
            state = self.stepState(state, text[i]);
            if (self.nodes.items[state].pattern_id != NO_PAT || self.nodes.items[state].out_link != 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }
};

// packages/@cap/aho-corasick-dfa/src/index.cp
// Zero-allocation multi-pattern Aho-Corasick automaton engine (256 states / 32 patterns).
// Clean-room MIT pure CP implementation inspired by `aho-corasick` v1.

import { foldAsciiByte } from "../../unicase/src/index.cp";

const ACDFA_MAX_STATES: usize = 256;
const ACDFA_MAX_PATTERNS: usize = 32;
const ACDFA_NO_EDGE: u16 = 0xFFFF;
const ACDFA_NO_STATE: u16 = 0xFFFF;

/// A single pattern match span `[start, end)` in the haystack.
pub struct AcMatch {
    pub pattern_id: usize,
    pub start: usize,
    pub end: usize,
}

fn acdfaNormByte(b: u8, ignore_case: bool): u8 {
    if (ignore_case) {
        return foldAsciiByte(b);
    }
    return b;
}

fn acdfaPopcount32(mask: u32): usize {
    let x: u32 = mask;
    let c: usize = 0;
    while (x != 0) {
        x = x & (x - 1);
        c += 1;
    }
    return c;
}

/// Fixed-pool, zero-heap Aho-Corasick multi-pattern automaton (up to 256 states, 32 patterns).
pub struct AhoCorasick {
    pub edge_head: [256]u16,
    pub edge_ch: [256]u8,
    pub edge_to: [256]u16,
    pub edge_next: [256]u16,
    pub fail: [256]u16,
    pub out_link: [256]u16,
    pub output_mask: [256]u32,
    pub pat_len: [32]usize,
    pub state_count: usize,
    pub edge_count: usize,
    pub pat_count: usize,
    pub ignore_case: bool,
    pub built: bool,

    /// Create an empty Aho-Corasick automaton with root state 0.
    pub static fn new(ignore_case: bool): AhoCorasick {
        let ac = AhoCorasick {
            edge_head: undefined,
            edge_ch: undefined,
            edge_to: undefined,
            edge_next: undefined,
            fail: undefined,
            out_link: undefined,
            output_mask: undefined,
            pat_len: undefined,
            state_count: 1,
            edge_count: 0,
            pat_count: 0,
            ignore_case: ignore_case,
            built: false,
        };
        let i: usize = 0;
        while (i < ACDFA_MAX_STATES) {
            ac.edge_head[i] = ACDFA_NO_EDGE;
            ac.edge_ch[i] = 0;
            ac.edge_to[i] = 0;
            ac.edge_next[i] = ACDFA_NO_EDGE;
            ac.fail[i] = 0;
            ac.out_link[i] = 0;
            ac.output_mask[i] = 0;
            i += 1;
        }
        let p: usize = 0;
        while (p < ACDFA_MAX_PATTERNS) {
            ac.pat_len[p] = 0;
            p += 1;
        }
        return ac;
    }

    /// Lookup direct trie edge from `state` labeled `ch`. Returns `ACDFA_NO_STATE` if absent.
    fn acdfaFindEdge(state: u16, ch: u8): u16 {
        let e: u16 = _.edge_head[state as usize];
        while (e != ACDFA_NO_EDGE) {
            if (_.edge_ch[e as usize] == ch) {
                return _.edge_to[e as usize];
            }
            e = _.edge_next[e as usize];
        }
        return ACDFA_NO_STATE;
    }

    /// Advance automaton one normalized byte using failure links.
    fn acdfaStep(state: u16, ch: u8): u16 {
        let cur: u16 = state;
        while (true) {
            const nxt: u16 = _.acdfaFindEdge(cur, ch);
            if (nxt != ACDFA_NO_STATE) {
                return nxt;
            }
            if (cur == 0) {
                return 0;
            }
            cur = _.fail[cur as usize];
        }
        return 0;
    }

    /// Insert a non-empty pattern into the trie. Returns false if empty or capacity is exceeded.
    pub fn addPattern(*_, pat: string): bool {
        if (pat.len == 0 || _.pat_count >= ACDFA_MAX_PATTERNS) {
            return false;
        }

        // Pre-check how many new states/edges would be required to preserve atomicity.
        let sim_state: u16 = 0;
        let needed: usize = 0;
        let i: usize = 0;
        while (i < pat.len) {
            const ch: u8 = acdfaNormByte(pat[i], _.ignore_case);
            if (needed == 0) {
                const nxt: u16 = _.acdfaFindEdge(sim_state, ch);
                if (nxt != ACDFA_NO_STATE) {
                    sim_state = nxt;
                } else {
                    needed = pat.len - i;
                    break;
                }
            }
            i += 1;
        }
        if (_.state_count + needed > ACDFA_MAX_STATES || _.edge_count + needed > ACDFA_MAX_STATES) {
            return false;
        }

        let cur: u16 = 0;
        let j: usize = 0;
        while (j < pat.len) {
            const ch: u8 = acdfaNormByte(pat[j], _.ignore_case);
            const nxt: u16 = _.acdfaFindEdge(cur, ch);
            if (nxt != ACDFA_NO_STATE) {
                cur = nxt;
            } else {
                const new_state: u16 = _.state_count as u16;
                _.state_count += 1;
                const e_idx: u16 = _.edge_count as u16;
                _.edge_count += 1;
                _.edge_ch[e_idx as usize] = ch;
                _.edge_to[e_idx as usize] = new_state;
                _.edge_next[e_idx as usize] = _.edge_head[cur as usize];
                _.edge_head[cur as usize] = e_idx;
                cur = new_state;
            }
            j += 1;
        }

        const pid: usize = _.pat_count;
        _.pat_len[pid] = pat.len;
        _.output_mask[cur as usize] = _.output_mask[cur as usize] | ((1 as u32) << (pid as u5));
        _.pat_count += 1;
        _.built = false;
        return true;
    }

    /// Build all BFS failure links and dictionary output links. Must be called before searching.
    pub fn build(*_) {
        let queue: [256]u16 = undefined;
        let q_head: usize = 0;
        let q_tail: usize = 0;

        _.fail[0] = 0;
        _.out_link[0] = 0;

        // Depth-1 states fail back to root 0.
        let e0: u16 = _.edge_head[0];
        while (e0 != ACDFA_NO_EDGE) {
            const child: u16 = _.edge_to[e0 as usize];
            _.fail[child as usize] = 0;
            _.out_link[child as usize] = 0;
            queue[q_tail] = child;
            q_tail += 1;
            e0 = _.edge_next[e0 as usize];
        }

        // BFS over remaining trie states.
        while (q_head < q_tail) {
            const u: u16 = queue[q_head];
            q_head += 1;

            let e: u16 = _.edge_head[u as usize];
            while (e != ACDFA_NO_EDGE) {
                const ch: u8 = _.edge_ch[e as usize];
                const v: u16 = _.edge_to[e as usize];

                let f: u16 = _.fail[u as usize];
                let target: u16 = 0;
                while (true) {
                    const cand: u16 = _.acdfaFindEdge(f, ch);
                    if (cand != ACDFA_NO_STATE) {
                        target = cand;
                        break;
                    }
                    if (f == 0) {
                        target = 0;
                        break;
                    }
                    f = _.fail[f as usize];
                }

                _.fail[v as usize] = target;
                if (_.output_mask[target as usize] != 0) {
                    _.out_link[v as usize] = target;
                } else {
                    _.out_link[v as usize] = _.out_link[target as usize];
                }
                _.output_mask[v as usize] = _.output_mask[v as usize] | _.output_mask[target as usize];

                queue[q_tail] = v;
                q_tail += 1;
                e = _.edge_next[e as usize];
            }
        }

        _.built = true;
    }

    /// Return total number of registered patterns.
    pub fn patternCount(): usize {
        return _.pat_count;
    }

    /// Return total number of allocated trie states (including root state 0).
    pub fn stateCount(): usize {
        return _.state_count;
    }

    /// Return true if any pattern matches anywhere in `haystack`.
    pub fn isMatch(haystack: string): bool {
        if (!_.built || _.pat_count == 0 || haystack.len == 0) {
            return false;
        }
        let state: u16 = 0;
        let i: usize = 0;
        while (i < haystack.len) {
            const ch: u8 = acdfaNormByte(haystack[i], _.ignore_case);
            state = _.acdfaStep(state, ch);
            if (_.output_mask[state as usize] != 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Return the earliest-ending match in `haystack`, or `null` if none match.
    /// When multiple patterns end at the same index, selects the leftmost-start (longest) match,
    /// breaking ties by lowest `pattern_id`.
    pub fn findFirst(haystack: string): AcMatch? {
        if (!_.built || _.pat_count == 0 || haystack.len == 0) {
            return null;
        }
        let state: u16 = 0;
        let i: usize = 0;
        while (i < haystack.len) {
            const ch: u8 = acdfaNormByte(haystack[i], _.ignore_case);
            state = _.acdfaStep(state, ch);
            const mask: u32 = _.output_mask[state as usize];
            if (mask != 0) {
                const end_pos: usize = i + 1;
                let best_pid: usize = 0;
                let best_start: usize = end_pos + 1;
                let pid: usize = 0;
                while (pid < _.pat_count) {
                    if ((mask & ((1 as u32) << (pid as u5))) != 0) {
                        const st: usize = end_pos - _.pat_len[pid];
                        if (st < best_start) {
                            best_start = st;
                            best_pid = pid;
                        }
                    }
                    pid += 1;
                }
                return AcMatch {
                    pattern_id: best_pid,
                    start: best_start,
                    end: end_pos,
                };
            }
            i += 1;
        }
        return null;
    }

    /// Find all (including overlapping) matches in `haystack`, writing up to `out_matches.len` results
    /// in ascending `end` order (and ascending `pattern_id` for ties). Returns the number written.
    pub fn findAll(haystack: string, out_matches: []AcMatch): usize {
        if (!_.built || _.pat_count == 0 || haystack.len == 0 || out_matches.len == 0) {
            return 0;
        }
        let state: u16 = 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < haystack.len) {
            const ch: u8 = acdfaNormByte(haystack[i], _.ignore_case);
            state = _.acdfaStep(state, ch);
            const mask: u32 = _.output_mask[state as usize];
            if (mask != 0) {
                const end_pos: usize = i + 1;
                let pid: usize = 0;
                while (pid < _.pat_count) {
                    if ((mask & ((1 as u32) << (pid as u5))) != 0) {
                        if (written < out_matches.len) {
                            out_matches[written] = AcMatch {
                                pattern_id: pid,
                                start: end_pos - _.pat_len[pid],
                                end: end_pos,
                            };
                            written += 1;
                        } else {
                            return written;
                        }
                    }
                    pid += 1;
                }
            }
            i += 1;
        }
        return written;
    }

    /// Count all (including overlapping) pattern matches in `haystack` in O(N) time.
    pub fn countMatches(haystack: string): usize {
        if (!_.built || _.pat_count == 0 || haystack.len == 0) {
            return 0;
        }
        let state: u16 = 0;
        let total: usize = 0;
        let i: usize = 0;
        while (i < haystack.len) {
            const ch: u8 = acdfaNormByte(haystack[i], _.ignore_case);
            state = _.acdfaStep(state, ch);
            const mask: u32 = _.output_mask[state as usize];
            if (mask != 0) {
                total += acdfaPopcount32(mask);
            }
            i += 1;
        }
        return total;
    }
}

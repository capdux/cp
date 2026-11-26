// @cap/aho-corasick — Zero-allocation Aho-Corasick O(N + Z) multi-pattern search automaton for CP
// Clean-room MIT implementation inspired by Rust `aho-corasick` (1.45B downloads).

const MAX_NODES: usize = 256;
const MAX_PATTERNS: usize = 64;
const NO_NODE: u16 = 0;
const NO_PAT: i16 = -1;

pub struct AcMatch {
    pub pattern_id: u16,
    pub start: usize,
    pub end: usize,
    pub found: bool,
}

fn noMatch() -> AcMatch {
    return AcMatch{
        pattern_id: 0 as u16,
        start: 0 as usize,
        end: 0 as usize,
        found: false,
    };
}

pub struct AhoCorasick {
    pub edge_char: [MAX_NODES]u8,
    pub first_child: [MAX_NODES]u16,
    pub next_sibling: [MAX_NODES]u16,
    pub fail: [MAX_NODES]u16,
    pub dict_link: [MAX_NODES]u16,
    pub out_pat: [MAX_NODES]i16,
    pub pat_lens: [MAX_PATTERNS]usize,
    pub node_count: usize,
    pub pat_count: usize,
    pub built: bool,

    pub static fn init() -> AhoCorasick {
        let ec: [MAX_NODES]u8 = undefined;
        let fc: [MAX_NODES]u16 = undefined;
        let ns: [MAX_NODES]u16 = undefined;
        let fl: [MAX_NODES]u16 = undefined;
        let dl: [MAX_NODES]u16 = undefined;
        let op: [MAX_NODES]i16 = undefined;
        let i: usize = 0;
        while (i < MAX_NODES) {
            ec[i] = 0 as u8;
            fc[i] = NO_NODE;
            ns[i] = NO_NODE;
            fl[i] = NO_NODE;
            dl[i] = NO_NODE;
            op[i] = NO_PAT;
            i += 1;
        }
        let pl: [MAX_PATTERNS]usize = undefined;
        let p: usize = 0;
        while (p < MAX_PATTERNS) {
            pl[p] = 0 as usize;
            p += 1;
        }
        return AhoCorasick{
            edge_char: ec,
            first_child: fc,
            next_sibling: ns,
            fail: fl,
            dict_link: dl,
            out_pat: op,
            pat_lens: pl,
            node_count: 1 as usize,
            pat_count: 0 as usize,
            built: false,
        };
    }

    fn findChild(node: u16, ch: u8) -> u16 {
        let cur = _.first_child[node as usize];
        while (cur != NO_NODE) {
            if (_.edge_char[cur as usize] == ch) {
                return cur;
            }
            cur = _.next_sibling[cur as usize];
        }
        return NO_NODE;
    }

    pub fn addPattern(*_, pat: []const u8) -> i32 {
        if (pat.len == (0 as usize) || _.pat_count >= MAX_PATTERNS) {
            return -1 as i32;
        }
        // Pre-check whether enough nodes exist to avoid partial insertion on overflow
        let cur: u16 = 0;
        let needed: usize = 0;
        let checkIdx: usize = 0;
        while (checkIdx < pat.len) {
            const nxt = _.findChild(cur, pat[checkIdx]);
            if (nxt != NO_NODE) {
                cur = nxt;
            } else {
                needed = pat.len - checkIdx;
                break;
            }
            checkIdx += 1;
        }
        if (_.node_count + needed > MAX_NODES) {
            return -1 as i32;
        }

        cur = 0 as u16;
        let i: usize = 0;
        while (i < pat.len) {
            const ch = pat[i];
            const nxt = _.findChild(cur, ch);
            if (nxt != NO_NODE) {
                cur = nxt;
            } else {
                const newNode = _.node_count as u16;
                _.node_count += 1;
                _.edge_char[newNode as usize] = ch;
                _.first_child[newNode as usize] = NO_NODE;
                _.next_sibling[newNode as usize] = _.first_child[cur as usize];
                _.first_child[cur as usize] = newNode;
                cur = newNode;
            }
            i += 1;
        }

        const pid = _.pat_count;
        _.pat_lens[pid] = pat.len;
        _.out_pat[cur as usize] = pid as i16;
        _.pat_count += 1;
        _.built = false;
        return pid as i32;
    }

    pub fn build(*_) -> bool {
        let queue: [MAX_NODES]u16 = undefined;
        let qHead: usize = 0;
        let qTail: usize = 0;

        // Reset failure & dictionary links
        let r: usize = 0;
        while (r < _.node_count) {
            _.fail[r] = NO_NODE;
            _.dict_link[r] = NO_NODE;
            r += 1;
        }

        // Depth-1 nodes fail to root (0)
        let child = _.first_child[0];
        while (child != NO_NODE) {
            _.fail[child as usize] = 0 as u16;
            _.dict_link[child as usize] = NO_NODE;
            queue[qTail] = child;
            qTail += 1;
            child = _.next_sibling[child as usize];
        }

        while (qHead < qTail) {
            const u = queue[qHead];
            qHead += 1;

            let v = _.first_child[u as usize];
            while (v != NO_NODE) {
                const ch = _.edge_char[v as usize];
                let f = _.fail[u as usize];
                while (f != (0 as u16) && _.findChild(f, ch) == NO_NODE) {
                    f = _.fail[f as usize];
                }
                const target = _.findChild(f, ch);
                if (target != NO_NODE && target != v) {
                    _.fail[v as usize] = target;
                } else {
                    _.fail[v as usize] = 0 as u16;
                }

                const fv = _.fail[v as usize];
                if (fv != (0 as u16) && _.out_pat[fv as usize] >= (0 as i16)) {
                    _.dict_link[v as usize] = fv;
                } else {
                    _.dict_link[v as usize] = _.dict_link[fv as usize];
                }

                queue[qTail] = v;
                qTail += 1;
                v = _.next_sibling[v as usize];
            }
        }

        _.built = true;
        return true;
    }

    pub fn patternCount() -> usize {
        return _.pat_count;
    }

    fn stepState(state_in: u16, ch: u8) -> u16 {
        let state = state_in;
        while (state != (0 as u16) && _.findChild(state, ch) == NO_NODE) {
            state = _.fail[state as usize];
        }
        const nxt = _.findChild(state, ch);
        if (nxt != NO_NODE) {
            return nxt;
        }
        return 0 as u16;
    }

    pub fn isMatch(text: []const u8) -> bool {
        if (!_.built || _.pat_count == (0 as usize)) {
            return false;
        }
        let state: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            state = _.stepState(state, text[i]);
            if (_.out_pat[state as usize] >= (0 as i16) || _.dict_link[state as usize] != NO_NODE) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn findFirst(text: []const u8) -> AcMatch {
        if (!_.built || _.pat_count == (0 as usize)) {
            return noMatch();
        }
        let state: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            state = _.stepState(state, text[i]);
            const endPos = i + (1 as usize);
            if (_.out_pat[state as usize] >= (0 as i16)) {
                const pid = _.out_pat[state as usize] as usize;
                return AcMatch{
                    pattern_id: pid as u16,
                    start: endPos - _.pat_lens[pid],
                    end: endPos,
                    found: true,
                };
            }
            if (_.dict_link[state as usize] != NO_NODE) {
                const dl = _.dict_link[state as usize];
                const pid = _.out_pat[dl as usize] as usize;
                return AcMatch{
                    pattern_id: pid as u16,
                    start: endPos - _.pat_lens[pid],
                    end: endPos,
                    found: true,
                };
            }
            i += 1;
        }
        return noMatch();
    }

    pub fn findAll(text: []const u8, out: []AcMatch) -> usize {
        if (!_.built || _.pat_count == (0 as usize)) {
            return 0 as usize;
        }
        let total: usize = 0;
        let state: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            state = _.stepState(state, text[i]);
            const endPos = i + (1 as usize);

            if (_.out_pat[state as usize] >= (0 as i16)) {
                const pid = _.out_pat[state as usize] as usize;
                if (total < out.len) {
                    out[total] = AcMatch{
                        pattern_id: pid as u16,
                        start: endPos - _.pat_lens[pid],
                        end: endPos,
                        found: true,
                    };
                }
                total += 1;
            }
            let dl = _.dict_link[state as usize];
            while (dl != NO_NODE) {
                const pid = _.out_pat[dl as usize] as usize;
                if (total < out.len) {
                    out[total] = AcMatch{
                        pattern_id: pid as u16,
                        start: endPos - _.pat_lens[pid],
                        end: endPos,
                        found: true,
                    };
                }
                total += 1;
                dl = _.dict_link[dl as usize];
            }
            i += 1;
        }
        return total;
    }

    pub fn countMatches(text: []const u8) -> usize {
        if (!_.built || _.pat_count == (0 as usize)) {
            return 0 as usize;
        }
        let total: usize = 0;
        let state: u16 = 0;
        let i: usize = 0;
        while (i < text.len) {
            state = _.stepState(state, text[i]);
            if (_.out_pat[state as usize] >= (0 as i16)) {
                total += 1;
            }
            let dl = _.dict_link[state as usize];
            while (dl != NO_NODE) {
                total += 1;
                dl = _.dict_link[dl as usize];
            }
            i += 1;
        }
        return total;
    }
}

export {
    AcMatch,
    AhoCorasick,
};

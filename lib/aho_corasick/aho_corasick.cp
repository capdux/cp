// Source: cp/lib/aho_corasick/aho_corasick.cp (Pure CP Aho-Corasick Multi-Pattern Automaton)
const std = @import("std");

pub const Match = struct {
    pattern_id: usize,
    start: usize,
    end: usize,
};

const Node = struct {
    next: [256]u32,
    fail: u32,
    out_link: u32,
    pattern_id: u32,
    pattern_len: usize,

    fn create(): Node {
        let n: Node = undefined;
        @memset(&n.next, 0);
        n.fail = 0;
        n.out_link = 0;
        n.pattern_id = std.math.maxInt(u32);
        n.pattern_len = 0;
        return n;
    }
};

pub const AhoCorasick = struct {
    nodes: std.ArrayList(Node),
    pattern_count: usize,

    pub fn init(alloc: std.mem.Allocator, patterns: []const []const u8): !AhoCorasick {
        let nodes = std.ArrayList(Node).empty;
        errdefer nodes.deinit(alloc);
        try nodes.append(alloc, Node.create());

        let pid: usize = 0;
        while (pid < patterns.len) : (pid += 1) {
            const pat = patterns[pid];
            if (pat.len == 0) { continue; }
            let cur: usize = 0;
            for (b of pat) {
                const b_idx: usize = @intCast(b);
                if (nodes.items[cur].next[b_idx] == 0) {
                    const new_idx: u32 = @intCast(nodes.items.len);
                    try nodes.append(alloc, Node.create());
                    nodes.items[cur].next[b_idx] = new_idx;
                }
                cur = @intCast(nodes.items[cur].next[b_idx]);
            }
            if (nodes.items[cur].pattern_id == std.math.maxInt(u32)) {
                nodes.items[cur].pattern_id = @intCast(pid);
                nodes.items[cur].pattern_len = pat.len;
            }
        }

        // Build failure & output links via BFS
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
                if (v == 0) { continue; }
                const v_idx: usize = @intCast(v);
                let f = nodes.items[u_idx].fail;
                while (f != 0) {
                    const fi: usize = @intCast(f);
                    if (nodes.items[fi].next[ch] != 0) { break; }
                    f = nodes.items[fi].fail;
                }
                const f_idx: usize = @intCast(f);
                const target = nodes.items[f_idx].next[ch];
                nodes.items[v_idx].fail = target;
                const t_idx: usize = @intCast(target);
                if (nodes.items[t_idx].pattern_id != std.math.maxInt(u32)) {
                    nodes.items[v_idx].out_link = target;
                } else {
                    nodes.items[v_idx].out_link = nodes.items[t_idx].out_link;
                }
                try queue.append(alloc, v);
            }
        }

        return AhoCorasick{
            .nodes = nodes,
            .pattern_count = patterns.len,
        };
    }

    pub fn deinit(self: *AhoCorasick, alloc: std.mem.Allocator) {
        self.nodes.deinit(alloc);
    }

    pub fn findAllOverlapping(self: AhoCorasick, alloc: std.mem.Allocator, text: []const u8): ![]Match {
        let out = std.ArrayList(Match).empty;
        errdefer out.deinit(alloc);

        let state: usize = 0;
        let i: usize = 0;
        while (i < text.len) : (i += 1) {
            const b_idx: usize = @intCast(text[i]);
            while (state != 0 and self.nodes.items[state].next[b_idx] == 0) {
                state = @intCast(self.nodes.items[state].fail);
            }
            state = @intCast(self.nodes.items[state].next[b_idx]);

            if (self.nodes.items[state].pattern_id != std.math.maxInt(u32)) {
                const plen = self.nodes.items[state].pattern_len;
                try out.append(alloc, Match{
                    .pattern_id = @intCast(self.nodes.items[state].pattern_id),
                    .start = i + 1 - plen,
                    .end = i + 1,
                });
            }
            let link = self.nodes.items[state].out_link;
            while (link != 0) {
                const l_idx: usize = @intCast(link);
                const l_plen = self.nodes.items[l_idx].pattern_len;
                try out.append(alloc, Match{
                    .pattern_id = @intCast(self.nodes.items[l_idx].pattern_id),
                    .start = i + 1 - l_plen,
                    .end = i + 1,
                });
                link = self.nodes.items[l_idx].out_link;
            }
        }
        return out.toOwnedSlice(alloc);
    }

    pub fn containsAny(self: AhoCorasick, text: []const u8): bool {
        let state: usize = 0;
        for (b of text) {
            const b_idx: usize = @intCast(b);
            while (state != 0 and self.nodes.items[state].next[b_idx] == 0) {
                state = @intCast(self.nodes.items[state].fail);
            }
            state = @intCast(self.nodes.items[state].next[b_idx]);
            if (self.nodes.items[state].pattern_id != std.math.maxInt(u32) or self.nodes.items[state].out_link != 0) {
                return true;
            }
        }
        return false;
    }
};

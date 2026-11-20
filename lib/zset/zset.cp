// Source: cp/lib/zset/zset.cp (SkipList + HashMap Dual-Index SortedSet Engine)
// Pure CP zero-C-dependency Redis-style ZSet combining a span-augmented SkipList
// with a StringHashMap(f64) for O(1) score lookup and O(log N) rank/range queries.

const std = @import("std");

/// Error set for SortedSet operations.
pub err ZSetError {
    InvalidScore,
    InvalidRange,
    OutOfMemory,
}

/// Read-only or popped entry representing a (member, score) pair in a SortedSet.
pub struct ZSetEntry {
    member: string = "";
    score: f64 = 0.0;
}

/// Aggregation strategy for `SortedSet.unionInto`.
pub enum ZSetAgg {
    Sum,
    Min,
    Max,
}

const SKIPLIST_MAXLEVEL: usize = 32;
const NULL_IDX: usize = std.math.maxInt(usize);

struct SkipLevel {
    forward: usize = NULL_IDX;
    span: usize = 0;
}

struct SkipNode {
    member: []u8 = [];
    score: f64 = 0.0;
    backward: usize = NULL_IDX;
    levels: []SkipLevel = [];
}

fn compareScoreMember(a_score: f64, a_member: string, b_score: f64, b_member: string): i32 {
    if (a_score < b_score) return -1;
    if (a_score > b_score) return 1;
    const ord = std.mem.order(u8, a_member, b_member);
    if (ord == .lt) return -1;
    if (ord == .gt) return 1;
    return 0;
}

/// Dual-indexed ordered set (SkipList with span + StringHashMapUnmanaged(f64)),
/// ordered ascending by `(score, member)` with lexicographical tie-breaking.
pub struct SortedSet {
    alloc: std.mem.Allocator;
    nodes: std.ArrayList(SkipNode) = .empty;
    free_slots: std.ArrayList(usize) = .empty;
    header: usize = NULL_IDX;
    tail: usize = NULL_IDX;
    length: usize = 0;
    level: usize = 1;
    dict: std.StringHashMapUnmanaged(f64) = .empty;
    rng_state: u64 = 0x9e3779b97f4a7c15;

    /// Initialize an empty SortedSet (zero heap allocation until first insert).
    pub static fn init(alloc: std.mem.Allocator): SortedSet {
        return SortedSet{
            alloc: alloc,
            nodes: .empty,
            free_slots: .empty,
            header: NULL_IDX,
            tail: NULL_IDX,
            length: 0,
            level: 1,
            dict: .empty,
            rng_state: 0x9e3779b97f4a7c15,
        };
    }

    /// Release all SkipList nodes, owned member strings, and hash index storage.
    pub fn deinit(*_) {
        _.dict.deinit(_.alloc);
        for (node of _.nodes.items) {
            if (node.member.len > 0) {
                _.alloc.free(node.member);
            }
            if (node.levels.len > 0) {
                _.alloc.free(node.levels);
            }
        }
        _.nodes.deinit(_.alloc);
        _.free_slots.deinit(_.alloc);
        _.header = NULL_IDX;
        _.tail = NULL_IDX;
        _.length = 0;
        _.level = 1;
    }

    /// Free entries returned by `zpopMin` / `zpopMax` (which own duplicated member strings).
    pub static fn freeEntries(alloc: std.mem.Allocator, entries: []ZSetEntry) {
        for (entry of entries) {
            if (entry.member.len > 0) {
                alloc.free(entry.member);
            }
        }
        if (entries.len > 0) {
            alloc.free(entries);
        }
    }

    fn nextRand(*_): u64 {
        let x = _.rng_state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.rng_state = x;
        return x *% 0x2545F4914F6CDD1D;
    }

    fn randomLevel(*_): usize {
        let lvl: usize = 1;
        while ((_.nextRand() & 0xFFFF) < 0x4000 && lvl < SKIPLIST_MAXLEVEL) {
            lvl += 1;
        }
        return lvl;
    }

    fn allocNode(*_, lvl: usize, score: f64, owned_member: []u8): usize ! ZSetError {
        let levels = _.alloc.alloc(SkipLevel, lvl) catch return ZSetError.OutOfMemory;
        for (i of 0..lvl) {
            levels[i] = SkipLevel{ forward: NULL_IDX, span: 0 };
        }
        const entry = SkipNode{
            member: owned_member,
            score: score,
            backward: NULL_IDX,
            levels: levels,
        };
        if (_.free_slots.items.len > 0) {
            const idx = _.free_slots.pop().?;
            _.nodes.items[idx] = entry;
            return idx;
        }
        const idx = _.nodes.items.len;
        _.nodes.append(_.alloc, entry) catch {
            _.alloc.free(levels);
            return ZSetError.OutOfMemory;
        };
        return idx;
    }

    fn releaseNode(*_, idx: usize, free_member: bool) {
        if (free_member && _.nodes.items[idx].member.len > 0) {
            _.alloc.free(_.nodes.items[idx].member);
        }
        if (_.nodes.items[idx].levels.len > 0) {
            _.alloc.free(_.nodes.items[idx].levels);
        }
        _.nodes.items[idx].member = [];
        _.nodes.items[idx].levels = [];
        _.nodes.items[idx].backward = NULL_IDX;
        _.free_slots.append(_.alloc, idx) catch {};
    }

    fn ensureHeader(*_): usize ! ZSetError {
        if (_.header != NULL_IDX) {
            return _.header;
        }
        const hdr = _.allocNode(SKIPLIST_MAXLEVEL, 0.0, [])?;
        _.header = hdr;
        _.level = 1;
        return hdr;
    }

    fn insertNode(*_, score: f64, owned_member: []u8): usize ! ZSetError {
        const hdr = _.ensureHeader()?;
        let update: [32]usize = [NULL_IDX; 32];
        let rank: [32]usize = [0; 32];

        let x: usize = hdr;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            rank[i] = if (i == _.level - 1) 0 else rank[i + 1];
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                if (compareScoreMember(_.nodes.items[fwd].score, _.nodes.items[fwd].member, score, owned_member) < 0) {
                    rank[i] += _.nodes.items[x].levels[i].span;
                    x = fwd;
                } else {
                    break;
                }
            }
            update[i] = x;
        }

        const lvl = _.randomLevel();
        if (lvl > _.level) {
            let k: usize = _.level;
            while (k < lvl) : (k += 1) {
                rank[k] = 0;
                update[k] = hdr;
                _.nodes.items[hdr].levels[k].span = _.length;
            }
            _.level = lvl;
        }

        const node_idx = _.allocNode(lvl, score, owned_member)?;
        let j: usize = 0;
        while (j < lvl) : (j += 1) {
            const u = update[j];
            _.nodes.items[node_idx].levels[j].forward = _.nodes.items[u].levels[j].forward;
            _.nodes.items[u].levels[j].forward = node_idx;
            _.nodes.items[node_idx].levels[j].span = _.nodes.items[u].levels[j].span - (rank[0] - rank[j]);
            _.nodes.items[u].levels[j].span = (rank[0] - rank[j]) + 1;
        }

        let m: usize = lvl;
        while (m < _.level) : (m += 1) {
            const u = update[m];
            _.nodes.items[u].levels[m].span += 1;
        }

        const prev0 = update[0];
        _.nodes.items[node_idx].backward = if (prev0 == hdr) NULL_IDX else prev0;
        const fwd0 = _.nodes.items[node_idx].levels[0].forward;
        if (fwd0 != NULL_IDX) {
            _.nodes.items[fwd0].backward = node_idx;
        } else {
            _.tail = node_idx;
        }

        _.length += 1;
        return node_idx;
    }

    fn unlinkNode(*_, x: usize, update: [32]usize) {
        const hdr = _.header;
        let i: usize = 0;
        while (i < _.level) : (i += 1) {
            const u = update[i];
            if (_.nodes.items[u].levels[i].forward == x) {
                _.nodes.items[u].levels[i].span += _.nodes.items[x].levels[i].span - 1;
                _.nodes.items[u].levels[i].forward = _.nodes.items[x].levels[i].forward;
            } else {
                _.nodes.items[u].levels[i].span -= 1;
            }
        }
        const fwd = _.nodes.items[x].levels[0].forward;
        if (fwd != NULL_IDX) {
            _.nodes.items[fwd].backward = _.nodes.items[x].backward;
        } else {
            _.tail = _.nodes.items[x].backward;
        }
        while (_.level > 1 && _.nodes.items[hdr].levels[_.level - 1].forward == NULL_IDX) {
            _.level -= 1;
        }
        _.length -= 1;
    }

    fn findAndUnlink(*_, score: f64, member: string): usize? {
        if (_.header == NULL_IDX) return null;
        let update: [32]usize = [NULL_IDX; 32];
        let x: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                if (compareScoreMember(_.nodes.items[fwd].score, _.nodes.items[fwd].member, score, member) < 0) {
                    x = fwd;
                } else {
                    break;
                }
            }
            update[i] = x;
        }
        const cand = _.nodes.items[x].levels[0].forward;
        if (cand != NULL_IDX) {
            if (_.nodes.items[cand].score == score && std.mem.eql(u8, _.nodes.items[cand].member, member)) {
                _.unlinkNode(cand, update);
                return cand;
            }
        }
        return null;
    }

    fn getNodeByRank(rank_1based: usize): usize {
        if (rank_1based == 0 || rank_1based > _.length || _.header == NULL_IDX) return NULL_IDX;
        let traversed: usize = 0;
        let x: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                if (traversed + _.nodes.items[x].levels[i].span <= rank_1based) {
                    traversed += _.nodes.items[x].levels[i].span;
                    x = fwd;
                    if (traversed == rank_1based) {
                        return x;
                    }
                } else {
                    break;
                }
            }
        }
        return NULL_IDX;
    }

    /// Add or update `member` with `score`. Returns `true` if newly inserted, `false` if updated.
    /// Rejects `NaN` with `ZSetError.InvalidScore`.
    pub fn zadd(*_, member: string, score: f64): bool ! ZSetError {
        if (std.math.isNan(score)) return ZSetError.InvalidScore;

        if (score_ptr := _.dict.getPtr(member)) {
            const old_score = score_ptr.*;
            if (old_score == score) {
                return false;
            }
            if (old_idx := _.findAndUnlink(old_score, member)) {
                const kept_member = _.nodes.items[old_idx].member;
                _.releaseNode(old_idx, false);
                * = _.insertNode(score, kept_member)?;
                score_ptr.* = score;
                return false;
            }
            return false;
        }

        const owned_member = _.alloc.dupe(u8, member) catch return ZSetError.OutOfMemory;
        errdefer _.alloc.free(owned_member);

        _.dict.put(_.alloc, owned_member, score) catch return ZSetError.OutOfMemory;
        errdefer * = _.dict.remove(owned_member);

        * = _.insertNode(score, owned_member)?;
        return true;
    }

    /// Increment `member`'s score by `delta` (inserting with `delta` if absent).
    /// Returns the resulting score.
    pub fn zincrby(*_, member: string, delta: f64): f64 ! ZSetError {
        if (std.math.isNan(delta)) return ZSetError.InvalidScore;
        const cur = _.dict.get(member) ?? 0.0;
        const next_score = cur + delta;
        if (std.math.isNan(next_score)) return ZSetError.InvalidScore;
        * = _.zadd(member, next_score)?;
        return next_score;
    }

    /// Remove `member` from the SortedSet. Returns `true` if it existed.
    pub fn zrem(*_, member: string): bool {
        const old_score = _.dict.get(member) ?? return false;
        * = _.dict.remove(member);
        if (idx := _.findAndUnlink(old_score, member)) {
            _.releaseNode(idx, true);
            return true;
        }
        return false;
    }

    /// O(1) lookup of `member`'s score, or `null` if absent.
    pub fn zscore(member: string): f64? {
        return _.dict.get(member);
    }

    /// Total number of members in the SortedSet.
    pub fn zcard(): usize {
        return _.length;
    }

    /// 0-based ascending rank of `member` computed in O(log N) via SkipList `span`.
    pub fn zrank(member: string): usize? {
        const score = _.dict.get(member) ?? return null;
        if (_.header == NULL_IDX) return null;
        let rank: usize = 0;
        let x: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                const cmp = compareScoreMember(_.nodes.items[fwd].score, _.nodes.items[fwd].member, score, member);
                if (cmp <= 0) {
                    rank += _.nodes.items[x].levels[i].span;
                    x = fwd;
                    if (cmp == 0) {
                        return rank - 1;
                    }
                } else {
                    break;
                }
            }
        }
        return null;
    }

    /// 0-based descending rank of `member` computed in O(log N).
    pub fn zrevrank(member: string): usize? {
        const r = _.zrank(member) ?? return null;
        return _.length - 1 - r;
    }

    /// Return entries in ascending `[start, stop]` index range (supports negative indices like `0, -1`).
    /// Caller owns the returned slice and frees it with `alloc.free(entries)`.
    pub fn zrange(alloc: std.mem.Allocator, start: isize, stop: isize): []ZSetEntry ! ZSetError {
        if (_.length == 0) return [];
        const len_i = _.length as isize;
        let s = if (start < 0) len_i + start else start;
        let e = if (stop < 0) len_i + stop else stop;
        if (s < 0) s = 0;
        if (e >= len_i) e = len_i - 1;
        if (s > e || s >= len_i) return [];

        const count = (e - s + 1) as usize;
        let res = alloc.alloc(ZSetEntry, count) catch return ZSetError.OutOfMemory;
        let cur = _.getNodeByRank((s as usize) + 1);
        let idx: usize = 0;
        while (idx < count) : (idx += 1) {
            res[idx] = ZSetEntry{
                member: _.nodes.items[cur].member,
                score: _.nodes.items[cur].score,
            };
            cur = _.nodes.items[cur].levels[0].forward;
        }
        return res;
    }

    /// Return entries in descending `[start, stop]` index range (supports negative indices like `0, -1`).
    /// Caller owns the returned slice and frees it with `alloc.free(entries)`.
    pub fn zrevrange(alloc: std.mem.Allocator, start: isize, stop: isize): []ZSetEntry ! ZSetError {
        if (_.length == 0) return [];
        const len_i = _.length as isize;
        let s = if (start < 0) len_i + start else start;
        let e = if (stop < 0) len_i + stop else stop;
        if (s < 0) s = 0;
        if (e >= len_i) e = len_i - 1;
        if (s > e || s >= len_i) return [];

        const count = (e - s + 1) as usize;
        let res = alloc.alloc(ZSetEntry, count) catch return ZSetError.OutOfMemory;
        let cur = _.getNodeByRank(_.length - (s as usize));
        let idx: usize = 0;
        while (idx < count) : (idx += 1) {
            res[idx] = ZSetEntry{
                member: _.nodes.items[cur].member,
                score: _.nodes.items[cur].score,
            };
            cur = _.nodes.items[cur].backward;
        }
        return res;
    }

    fn firstNodeAtLeast(min_score: f64): usize {
        if (_.header == NULL_IDX) return NULL_IDX;
        let x: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                if (_.nodes.items[fwd].score < min_score) {
                    x = fwd;
                } else {
                    break;
                }
            }
        }
        return _.nodes.items[x].levels[0].forward;
    }

    /// Return all entries with `min_score <= score <= max_score` in ascending order.
    /// Caller frees the returned slice with `alloc.free(entries)`.
    pub fn zrangeByScore(alloc: std.mem.Allocator, min_score: f64, max_score: f64): []ZSetEntry ! ZSetError {
        if (std.math.isNan(min_score) || std.math.isNan(max_score)) {
            return ZSetError.InvalidScore;
        }
        if (min_score > max_score || _.length == 0) return [];

        let list: std.ArrayList(ZSetEntry) = .empty;
        errdefer list.deinit(alloc);

        let cur = _.firstNodeAtLeast(min_score);
        while (cur != NULL_IDX) {
            const sc = _.nodes.items[cur].score;
            if (sc > max_score) break;
            list.append(alloc, ZSetEntry{
                member: _.nodes.items[cur].member,
                score: sc,
            }) catch return ZSetError.OutOfMemory;
            cur = _.nodes.items[cur].levels[0].forward;
        }
        return list.toOwnedSlice(alloc) catch return ZSetError.OutOfMemory;
    }

    /// Count entries with `min_score <= score <= max_score` in O(log N) using SkipList `span`.
    pub fn zcount(min_score: f64, max_score: f64): usize {
        if (std.math.isNan(min_score) || std.math.isNan(max_score) || min_score > max_score || _.length == 0 || _.header == NULL_IDX) {
            return 0;
        }

        // Find 1-based rank of first node with score >= min_score
        let r1: usize = 0;
        let x1: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x1].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x1].levels[i].forward;
                if (_.nodes.items[fwd].score < min_score) {
                    r1 += _.nodes.items[x1].levels[i].span;
                    x1 = fwd;
                } else {
                    break;
                }
            }
        }
        const first_idx = _.nodes.items[x1].levels[0].forward;
        if (first_idx == NULL_IDX || _.nodes.items[first_idx].score > max_score) {
            return 0;
        }
        r1 += 1;

        // Find 1-based rank of last node with score <= max_score
        let r2: usize = 0;
        let x2: usize = _.header;
        let j: usize = _.level;
        while (j > 0) {
            j -= 1;
            while (_.nodes.items[x2].levels[j].forward != NULL_IDX) {
                const fwd = _.nodes.items[x2].levels[j].forward;
                if (_.nodes.items[fwd].score <= max_score) {
                    r2 += _.nodes.items[x2].levels[j].span;
                    x2 = fwd;
                } else {
                    break;
                }
            }
        }
        if (r2 < r1) return 0;
        return r2 - r1 + 1;
    }

    /// Remove all entries with `min_score <= score <= max_score`. Returns the number removed.
    pub fn zremRangeByScore(*_, min_score: f64, max_score: f64): usize {
        if (std.math.isNan(min_score) || std.math.isNan(max_score) || min_score > max_score || _.length == 0 || _.header == NULL_IDX) {
            return 0;
        }
        let update: [32]usize = [NULL_IDX; 32];
        let x: usize = _.header;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (_.nodes.items[x].levels[i].forward != NULL_IDX) {
                const fwd = _.nodes.items[x].levels[i].forward;
                if (_.nodes.items[fwd].score < min_score) {
                    x = fwd;
                } else {
                    break;
                }
            }
            update[i] = x;
        }

        let cur = _.nodes.items[x].levels[0].forward;
        let removed: usize = 0;
        while (cur != NULL_IDX) {
            if (_.nodes.items[cur].score > max_score) break;
            const next_cur = _.nodes.items[cur].levels[0].forward;
            _.unlinkNode(cur, update);
            * = _.dict.remove(_.nodes.items[cur].member);
            _.releaseNode(cur, true);
            removed += 1;
            cur = next_cur;
        }
        return removed;
    }

    /// Pop up to `count` lowest-scoring entries. Caller frees via `SortedSet.freeEntries(alloc, res)`.
    pub fn zpopMin(*_, alloc: std.mem.Allocator, count: usize): []ZSetEntry ! ZSetError {
        if (count == 0 || _.length == 0 || _.header == NULL_IDX) return [];
        const take = if (count < _.length) count else _.length;
        let res = alloc.alloc(ZSetEntry, take) catch return ZSetError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < filled) : (k += 1) {
                alloc.free(res[k].member);
            }
            alloc.free(res);
        }

        const update: [32]usize = [_.header; 32];
        while (filled < take) {
            const node_idx = _.nodes.items[_.header].levels[0].forward;
            const dup_member = alloc.dupe(u8, _.nodes.items[node_idx].member) catch return ZSetError.OutOfMemory;
            res[filled] = ZSetEntry{
                member: dup_member,
                score: _.nodes.items[node_idx].score,
            };
            filled += 1;

            _.unlinkNode(node_idx, update);
            * = _.dict.remove(_.nodes.items[node_idx].member);
            _.releaseNode(node_idx, true);
        }
        return res;
    }

    /// Pop up to `count` highest-scoring entries. Caller frees via `SortedSet.freeEntries(alloc, res)`.
    pub fn zpopMax(*_, alloc: std.mem.Allocator, count: usize): []ZSetEntry ! ZSetError {
        if (count == 0 || _.length == 0 || _.tail == NULL_IDX) return [];
        const take = if (count < _.length) count else _.length;
        let res = alloc.alloc(ZSetEntry, take) catch return ZSetError.OutOfMemory;
        let filled: usize = 0;
        errdefer {
            let k: usize = 0;
            while (k < filled) : (k += 1) {
                alloc.free(res[k].member);
            }
            alloc.free(res);
        }

        while (filled < take) {
            const tail_idx = _.tail;
            const dup_member = alloc.dupe(u8, _.nodes.items[tail_idx].member) catch return ZSetError.OutOfMemory;
            const tail_score = _.nodes.items[tail_idx].score;
            res[filled] = ZSetEntry{
                member: dup_member,
                score: tail_score,
            };
            filled += 1;

            const unlinked = _.findAndUnlink(tail_score, _.nodes.items[tail_idx].member).?;
            * = _.dict.remove(_.nodes.items[unlinked].member);
            _.releaseNode(unlinked, true);
        }
        return res;
    }

    /// Merge multiple SortedSets into a newly allocated SortedSet using `agg` (`Sum` / `Min` / `Max`).
    pub static fn unionInto(alloc: std.mem.Allocator, sets: []const *SortedSet, agg: ZSetAgg): SortedSet ! ZSetError {
        let out = SortedSet.init(alloc);
        errdefer out.deinit();

        for (set_ptr of sets) {
            if (set_ptr.header != NULL_IDX) {
                let cur = set_ptr.nodes.items[set_ptr.header].levels[0].forward;
                while (cur != NULL_IDX) {
                    const mem = set_ptr.nodes.items[cur].member;
                    const sc = set_ptr.nodes.items[cur].score;
                    if (existing := out.zscore(mem)) {
                        let merged: f64 = existing;
                        switch (agg) {
                            .Sum => { merged = existing + sc; },
                            .Min => { if (sc < existing) merged = sc; },
                            .Max => { if (sc > existing) merged = sc; },
                        }
                        let * = out.zadd(mem, merged)?;
                    } else {
                        let * = out.zadd(mem, sc)?;
                    }
                    cur = set_ptr.nodes.items[cur].levels[0].forward;
                }
            }
        }
        return out;
    }
}

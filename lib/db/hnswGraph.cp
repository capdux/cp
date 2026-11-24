// Source: cp/lib/hnsw/graph.cp (Pure CP HNSW multi-layer navigable small-world graph index)
const std = @import("std");
import { HnswError, DistanceMetric, HnswConfig, SearchHit, computeDistance } from "./hnswMetric.cp";

const NO_NODE: usize = std.math.maxInt(usize);
const SNAPSHOT_VERSION: u32 = 1;

struct Candidate {
    node_idx: usize,
    distance: f32,
}

struct HnswNode {
    id: u64,
    deleted: bool,
    level: usize,
    neighbors: []std.ArrayList(usize),
}

fn sortCandidatesAsc(items: []Candidate) {
    let i: usize = 1;
    while (i < items.len) {
        const key = items[i];
        let j: usize = i;
        while (j > 0 and items[j - 1].distance > key.distance) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

fn sortHitsAsc(items: []SearchHit) {
    let i: usize = 1;
    while (i < items.len) {
        const key = items[i];
        let j: usize = i;
        while (j > 0 and items[j - 1].distance > key.distance) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

fn appendU32LE(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): !void {
    let bytes: [4]u8 = undefined;
    std.mem.writeInt(u32, &bytes, val, .little);
    try buf.appendSlice(alloc, &bytes);
}

fn appendU64LE(buf: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u64): !void {
    let bytes: [8]u8 = undefined;
    std.mem.writeInt(u64, &bytes, val, .little);
    try buf.appendSlice(alloc, &bytes);
}

fn readU8(bytes: []const u8, pos: *usize): HnswError!u8 {
    if (pos.* + 1 > bytes.len) {
        return HnswError.CorruptSnapshot;
    }
    const v = bytes[pos.*];
    pos.* += 1;
    return v;
}

fn readU32LE(bytes: []const u8, pos: *usize): HnswError!u32 {
    if (pos.* + 4 > bytes.len) {
        return HnswError.CorruptSnapshot;
    }
    const slice = bytes[pos.* .. pos.* + 4];
    pos.* += 4;
    return std.mem.readInt(u32, slice[0..4], .little);
}

fn readU64LE(bytes: []const u8, pos: *usize): HnswError!u64 {
    if (pos.* + 8 > bytes.len) {
        return HnswError.CorruptSnapshot;
    }
    const slice = bytes[pos.* .. pos.* + 8];
    pos.* += 8;
    return std.mem.readInt(u64, slice[0..8], .little);
}

pub struct HnswIndex {
    alloc: std.mem.Allocator,
    dim: usize,
    metric: DistanceMetric,
    config: HnswConfig,
    rng_state: u64,
    entry_point: usize,
    max_level: usize,
    active_count: usize,
    nodes: std.ArrayList(HnswNode),
    vectors: std.ArrayList(f32),
    id_map: std.AutoHashMapUnmanaged(u64, usize),

    pub static fn init(alloc: std.mem.Allocator, dim: usize, metric: DistanceMetric, config: HnswConfig): !HnswIndex {
        if (dim == 0) {
            return HnswError.EmptyVector;
        }
        try config.validate();
        return HnswIndex{
            .alloc = alloc,
            .dim = dim,
            .metric = metric,
            .config = config,
            .rng_state = if (config.seed == 0) 0x9E3779B97F4A7C15 else config.seed,
            .entry_point = NO_NODE,
            .max_level = 0,
            .active_count = 0,
            .nodes = std.ArrayList(HnswNode).empty,
            .vectors = std.ArrayList(f32).empty,
            .id_map = std.AutoHashMapUnmanaged(u64, usize).empty,
        };
    }

    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) {
            let lvl: usize = 0;
            while (lvl < _.nodes.items[i].neighbors.len) {
                _.nodes.items[i].neighbors[lvl].deinit(_.alloc);
                lvl += 1;
            }
            _.alloc.free(_.nodes.items[i].neighbors);
            i += 1;
        }
        _.nodes.deinit(_.alloc);
        _.vectors.deinit(_.alloc);
        _.id_map.deinit(_.alloc);
    }

    pub fn len(): usize {
        return _.active_count;
    }

    pub fn totalNodes(): usize {
        return _.nodes.items.len;
    }

    pub fn maxLevel(): usize {
        return _.max_level;
    }

    fn nodeVector(idx: usize): []const f32 {
        const start = idx * _.dim;
        return _.vectors.items[start .. start + _.dim];
    }

    fn distToNode(query: []const f32, idx: usize): HnswError!f32 {
        return computeDistance(_.metric, query, _.nodeVector(idx));
    }

    fn nextUniform01(*_): f64 {
        // SplitMix64 deterministic PRNG
        _.rng_state = _.rng_state +% 0x9E3779B97F4A7C15;
        let z: u64 = _.rng_state;
        z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
        z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
        z = z ^ (z >> 31);
        const mantissa: u64 = z >> 11;
        let u: f64 = @as(f64, @floatFromInt(mantissa)) / 9007199254740992.0;
        if (u <= 1.0e-12) {
            u = 1.0e-12;
        }
        return u;
    }

    fn sampleLevel(*_): usize {
        const m_f: f64 = @floatFromInt(_.config.m);
        const ml: f64 = 1.0 / std.math.ln(m_f);
        const u: f64 = _.nextUniform01();
        const raw: f64 = -std.math.ln(u) * ml;
        if (raw <= 0.0) {
            return 0;
        }
        let lvl: usize = @intFromFloat(raw);
        if (lvl > 32) {
            lvl = 32;
        }
        return lvl;
    }

    fn greedySearchLayer(query: []const f32, entry: usize, level: usize): !usize {
        let curr = entry;
        let curr_dist = try _.distToNode(query, curr);
        let improved = true;
        while (improved) {
            improved = false;
            const nbrs = _.nodes.items[curr].neighbors[level].items;
            let i: usize = 0;
            while (i < nbrs.len) {
                const nb = nbrs[i];
                const d = try _.distToNode(query, nb);
                if (d < curr_dist) {
                    curr = nb;
                    curr_dist = d;
                    improved = true;
                }
                i += 1;
            }
        }
        return curr;
    }

    fn searchLayer(
        alloc: std.mem.Allocator,
        query: []const f32,
        entry: usize,
        ef: usize,
        level: usize,
    ): ![]Candidate {
        let visited = std.AutoHashMapUnmanaged(usize, void).empty;
        defer visited.deinit(alloc);

        let candidates = std.ArrayList(Candidate).empty;
        defer candidates.deinit(alloc);

        let results = std.ArrayList(Candidate).empty;
        errdefer results.deinit(alloc);

        const entry_dist = try _.distToNode(query, entry);
        try visited.put(alloc, entry, {});
        try candidates.append(alloc, Candidate{ .node_idx = entry, .distance = entry_dist });
        try results.append(alloc, Candidate{ .node_idx = entry, .distance = entry_dist });

        while (candidates.items.len > 0) {
            // Pop closest candidate
            let best_pos: usize = 0;
            let ci: usize = 1;
            while (ci < candidates.items.len) {
                if (candidates.items[ci].distance < candidates.items[best_pos].distance) {
                    best_pos = ci;
                }
                ci += 1;
            }
            const curr = candidates.orderedRemove(best_pos);

            // Find worst in results
            let worst_dist: f32 = results.items[0].distance;
            let ri: usize = 1;
            while (ri < results.items.len) {
                if (results.items[ri].distance > worst_dist) {
                    worst_dist = results.items[ri].distance;
                }
                ri += 1;
            }

            if (curr.distance > worst_dist and results.items.len >= ef) {
                break;
            }

            const nbrs = _.nodes.items[curr.node_idx].neighbors[level].items;
            let ni: usize = 0;
            while (ni < nbrs.len) {
                const nb = nbrs[ni];
                if (!visited.contains(nb)) {
                    try visited.put(alloc, nb, {});
                    const d = try _.distToNode(query, nb);

                    let curr_worst_idx: usize = 0;
                    let rj: usize = 1;
                    while (rj < results.items.len) {
                        if (results.items[rj].distance > results.items[curr_worst_idx].distance) {
                            curr_worst_idx = rj;
                        }
                        rj += 1;
                    }

                    if (results.items.len < ef or d < results.items[curr_worst_idx].distance) {
                        try candidates.append(alloc, Candidate{ .node_idx = nb, .distance = d });
                        if (results.items.len < ef) {
                            try results.append(alloc, Candidate{ .node_idx = nb, .distance = d });
                        } else {
                            results.items[curr_worst_idx] = Candidate{ .node_idx = nb, .distance = d };
                        }
                    }
                }
                ni += 1;
            }
        }

        sortCandidatesAsc(results.items);
        return results.toOwnedSlice(alloc);
    }

    fn selectNeighbors(candidates: []const Candidate, max_conn: usize, out: *std.ArrayList(usize), alloc: std.mem.Allocator): !void {
        const count = if (candidates.len < max_conn) candidates.len else max_conn;
        let i: usize = 0;
        while (i < count) {
            try out.append(alloc, candidates[i].node_idx);
            i += 1;
        }
    }

    fn pruneNeighbors(*_, node_idx: usize, level: usize, max_conn: usize): !void {
        const nbrs = &_.nodes.items[node_idx].neighbors[level];
        if (nbrs.items.len <= max_conn) {
            return;
        }
        const base_vec = _.nodeVector(node_idx);
        let scored = std.ArrayList(Candidate).empty;
        defer scored.deinit(_.alloc);

        let i: usize = 0;
        while (i < nbrs.items.len) {
            const nb = nbrs.items[i];
            const d = try computeDistance(_.metric, base_vec, _.nodeVector(nb));
            try scored.append(_.alloc, Candidate{ .node_idx = nb, .distance = d });
            i += 1;
        }
        sortCandidatesAsc(scored.items);
        nbrs.clearRetainingCapacity();
        try selectNeighbors(scored.items, max_conn, nbrs, _.alloc);
    }

    pub fn insert(*_, id: u64, vector: []const f32): !void {
        if (vector.len == 0) {
            return HnswError.EmptyVector;
        }
        if (vector.len != _.dim) {
            return HnswError.DimensionMismatch;
        }
        if (_.id_map.contains(id)) {
            return HnswError.DuplicateId;
        }

        const new_idx = _.nodes.items.len;
        const level = _.sampleLevel();

        let nbr_slices = try _.alloc.alloc(std.ArrayList(usize), level + 1);
        let li: usize = 0;
        while (li <= level) {
            nbr_slices[li] = std.ArrayList(usize).empty;
            li += 1;
        }

        try _.vectors.appendSlice(_.alloc, vector);
        try _.nodes.append(_.alloc, HnswNode{
            .id = id,
            .deleted = false,
            .level = level,
            .neighbors = nbr_slices,
        });
        try _.id_map.put(_.alloc, id, new_idx);
        _.active_count += 1;

        if (_.entry_point == NO_NODE) {
            _.entry_point = new_idx;
            _.max_level = level;
            return;
        }

        let curr_entry = _.entry_point;
        let curr_lvl = _.max_level;
        while (curr_lvl > level) {
            curr_entry = try _.greedySearchLayer(vector, curr_entry, curr_lvl);
            if (curr_lvl == 0) {
                break;
            }
            curr_lvl -= 1;
        }

        const top_connect = if (level < _.max_level) level else _.max_level;
        let lc: usize = top_connect;
        while (true) {
            const cands = try _.searchLayer(_.alloc, vector, curr_entry, _.config.ef_construction, lc);
            defer _.alloc.free(cands);

            const max_conn = if (lc == 0) _.config.m0 else _.config.m;
            try selectNeighbors(cands, max_conn, &_.nodes.items[new_idx].neighbors[lc], _.alloc);

            const added = _.nodes.items[new_idx].neighbors[lc].items;
            let ai: usize = 0;
            while (ai < added.len) {
                const nb = added[ai];
                try _.nodes.items[nb].neighbors[lc].append(_.alloc, new_idx);
                try _.pruneNeighbors(nb, lc, max_conn);
                ai += 1;
            }

            if (cands.len > 0) {
                curr_entry = cands[0].node_idx;
            }
            if (lc == 0) {
                break;
            }
            lc -= 1;
        }

        if (level > _.max_level) {
            _.max_level = level;
            _.entry_point = new_idx;
        }
    }

    pub fn markDeleted(*_, id: u64): HnswError!void {
        const idx = _.id_map.get(id) orelse return HnswError.IdNotFound;
        if (!_.nodes.items[idx].deleted) {
            _.nodes.items[idx].deleted = true;
            if (_.active_count > 0) {
                _.active_count -= 1;
            }
        }
    }

    pub fn search(alloc: std.mem.Allocator, query: []const f32, top_k: usize, ef_search: usize): ![]SearchHit {
        if (query.len == 0) {
            return HnswError.EmptyVector;
        }
        if (query.len != _.dim) {
            return HnswError.DimensionMismatch;
        }
        if (top_k == 0 or _.entry_point == NO_NODE or _.active_count == 0) {
            return try alloc.alloc(SearchHit, 0);
        }

        let curr_entry = _.entry_point;
        let lvl = _.max_level;
        while (lvl > 0) {
            curr_entry = try _.greedySearchLayer(query, curr_entry, lvl);
            lvl -= 1;
        }

        const effective_ef = if (ef_search > top_k) ef_search else top_k;
        const cands = try _.searchLayer(alloc, query, curr_entry, effective_ef, 0);
        defer alloc.free(cands);

        let hits = std.ArrayList(SearchHit).empty;
        errdefer hits.deinit(alloc);

        let i: usize = 0;
        while (i < cands.len and hits.items.len < top_k) {
            const node = _.nodes.items[cands[i].node_idx];
            if (!node.deleted) {
                try hits.append(alloc, SearchHit{
                    .id = node.id,
                    .distance = cands[i].distance,
                });
            }
            i += 1;
        }

        // If soft-deleted nodes caused fewer than top_k hits, fallback scan remaining active nodes
        if (hits.items.len < top_k and hits.items.len < _.active_count) {
            let extra = std.ArrayList(SearchHit).empty;
            defer extra.deinit(alloc);

            let ni: usize = 0;
            while (ni < _.nodes.items.len) {
                const node = _.nodes.items[ni];
                if (!node.deleted) {
                    const d = try _.distToNode(query, ni);
                    try extra.append(alloc, SearchHit{ .id = node.id, .distance = d });
                }
                ni += 1;
            }
            sortHitsAsc(extra.items);
            hits.clearRetainingCapacity();
            const take = if (extra.items.len < top_k) extra.items.len else top_k;
            let ti: usize = 0;
            while (ti < take) {
                try hits.append(alloc, extra.items[ti]);
                ti += 1;
            }
        }

        return hits.toOwnedSlice(alloc);
    }

    pub fn saveBytes(alloc: std.mem.Allocator): ![]u8 {
        let buf = std.ArrayList(u8).empty;
        errdefer buf.deinit(alloc);

        try buf.appendSlice(alloc, "HNSW");
        try appendU32LE(&buf, alloc, SNAPSHOT_VERSION);
        try appendU64LE(&buf, alloc, @intCast(_.dim));

        const metric_u8: u8 = switch (_.metric) {
            .L2Squared => 0,
            .Cosine => 1,
            .InnerProduct => 2,
        };
        try buf.append(alloc, metric_u8);
        try appendU64LE(&buf, alloc, @intCast(_.config.m));
        try appendU64LE(&buf, alloc, @intCast(_.config.m0));
        try appendU64LE(&buf, alloc, @intCast(_.config.ef_construction));
        try appendU64LE(&buf, alloc, _.config.seed);
        try appendU64LE(&buf, alloc, _.rng_state);

        const ep_u64: u64 = if (_.entry_point == NO_NODE) std.math.maxInt(u64) else @intCast(_.entry_point);
        try appendU64LE(&buf, alloc, ep_u64);
        try appendU64LE(&buf, alloc, @intCast(_.max_level));
        try appendU64LE(&buf, alloc, @intCast(_.nodes.items.len));

        let ni: usize = 0;
        while (ni < _.nodes.items.len) {
            const node = _.nodes.items[ni];
            try appendU64LE(&buf, alloc, node.id);
            try buf.append(alloc, if (node.deleted) 1 else 0);
            try appendU64LE(&buf, alloc, @intCast(node.level));

            const vec = _.nodeVector(ni);
            let di: usize = 0;
            while (di < _.dim) {
                const bits: u32 = @bitCast(vec[di]);
                try appendU32LE(&buf, alloc, bits);
                di += 1;
            }

            let lvl: usize = 0;
            while (lvl <= node.level) {
                const nbrs = node.neighbors[lvl].items;
                try appendU64LE(&buf, alloc, @intCast(nbrs.len));
                let ki: usize = 0;
                while (ki < nbrs.len) {
                    try appendU64LE(&buf, alloc, @intCast(nbrs[ki]));
                    ki += 1;
                }
                lvl += 1;
            }
            ni += 1;
        }

        return buf.toOwnedSlice(alloc);
    }

    pub fn loadBytes(alloc: std.mem.Allocator, bytes: []const u8): !HnswIndex {
        if (bytes.len < 8) {
            return HnswError.CorruptSnapshot;
        }
        if (bytes[0] != 'H' or bytes[1] != 'N' or bytes[2] != 'S' or bytes[3] != 'W') {
            return HnswError.CorruptSnapshot;
        }
        let pos: usize = 4;
        const ver = try readU32LE(bytes, &pos);
        if (ver != SNAPSHOT_VERSION) {
            return HnswError.CorruptSnapshot;
        }
        const dim_u64 = try readU64LE(bytes, &pos);
        if (dim_u64 == 0) {
            return HnswError.CorruptSnapshot;
        }
        const dim: usize = @intCast(dim_u64);
        const metric_u8 = try readU8(bytes, &pos);
        const metric: DistanceMetric = switch (metric_u8) {
            0 => .L2Squared,
            1 => .Cosine,
            2 => .InnerProduct,
            else => return HnswError.CorruptSnapshot,
        };
        const m: usize = @intCast(try readU64LE(bytes, &pos));
        const m0: usize = @intCast(try readU64LE(bytes, &pos));
        const ef_construction: usize = @intCast(try readU64LE(bytes, &pos));
        const seed = try readU64LE(bytes, &pos);
        const rng_state = try readU64LE(bytes, &pos);
        const ep_u64 = try readU64LE(bytes, &pos);
        const max_level: usize = @intCast(try readU64LE(bytes, &pos));
        const node_count: usize = @intCast(try readU64LE(bytes, &pos));

        const cfg = HnswConfig{
            .m = m,
            .m0 = m0,
            .ef_construction = ef_construction,
            .seed = seed,
        };
        try cfg.validate();

        let idx = try HnswIndex.init(alloc, dim, metric, cfg);
        errdefer idx.deinit();

        idx.rng_state = rng_state;
        idx.max_level = max_level;
        idx.entry_point = if (ep_u64 == std.math.maxInt(u64)) NO_NODE else @intCast(ep_u64);
        if (idx.entry_point != NO_NODE and idx.entry_point >= node_count) {
            return HnswError.CorruptSnapshot;
        }

        let ni: usize = 0;
        while (ni < node_count) {
            const id = try readU64LE(bytes, &pos);
            const del_u8 = try readU8(bytes, &pos);
            const deleted = del_u8 != 0;
            const level: usize = @intCast(try readU64LE(bytes, &pos));
            if (level > 64) {
                return HnswError.CorruptSnapshot;
            }

            let di: usize = 0;
            while (di < dim) {
                const bits = try readU32LE(bytes, &pos);
                const val: f32 = @bitCast(bits);
                try idx.vectors.append(alloc, val);
                di += 1;
            }

            let nbr_slices = try alloc.alloc(std.ArrayList(usize), level + 1);
            let li: usize = 0;
            while (li <= level) {
                nbr_slices[li] = std.ArrayList(usize).empty;
                li += 1;
            }
            try idx.nodes.append(alloc, HnswNode{
                .id = id,
                .deleted = deleted,
                .level = level,
                .neighbors = nbr_slices,
            });
            try idx.id_map.put(alloc, id, ni);
            if (!deleted) {
                idx.active_count += 1;
            }

            let lvl: usize = 0;
            while (lvl <= level) {
                const deg: usize = @intCast(try readU64LE(bytes, &pos));
                let ki: usize = 0;
                while (ki < deg) {
                    const nb: usize = @intCast(try readU64LE(bytes, &pos));
                    if (nb >= node_count) {
                        return HnswError.CorruptSnapshot;
                    }
                    try idx.nodes.items[ni].neighbors[lvl].append(alloc, nb);
                    ki += 1;
                }
                lvl += 1;
            }
            ni += 1;
        }

        return idx;
    }
}

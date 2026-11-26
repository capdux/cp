// @cap/dijkstra — Zero-allocation weighted shortest-path engine (Dijkstra + Bellman-Ford) for CP
// Clean-room MIT implementation inspired by Rust `pathfinding` and `petgraph::algo::dijkstra`.

const MAX_NODES: usize = 64;
const MAX_EDGES: usize = 256;
const INF_DIST: i64 = 1000000000000;
const NO_EDGE: i16 = -1;
const NO_PREV: i16 = -1;

pub struct ShortestPaths {
    pub ok: bool,
    pub node_count: usize,
    pub source: usize,
    pub dist: [MAX_NODES]i64,
    pub prev: [MAX_NODES]i16,

    pub fn isReachable(target: usize) -> bool {
        if (!_.ok || target >= _.node_count) {
            return false;
        }
        return _.dist[target] < INF_DIST;
    }

    pub fn distanceTo(target: usize) -> i64 {
        if (!_.isReachable(target)) {
            return -1 as i64;
        }
        return _.dist[target];
    }

    pub fn reconstructPath(target: usize, out: []u16) -> usize {
        if (!_.isReachable(target)) {
            return 0 as usize;
        }
        let rev: [MAX_NODES]u16 = undefined;
        let count: usize = 0;
        let cur: i16 = target as i16;
        while (cur >= (0 as i16) && count < MAX_NODES) {
            rev[count] = cur as u16;
            count += 1;
            if ((cur as usize) == _.source) {
                break;
            }
            cur = _.prev[cur as usize];
        }
        if (count == (0 as usize) || (rev[count - (1 as usize)] as usize) != _.source) {
            return 0 as usize;
        }
        if (out.len < count) {
            return 0 as usize;
        }
        let i: usize = 0;
        while (i < count) {
            out[i] = rev[count - (1 as usize) - i];
            i += 1;
        }
        return count;
    }
}

fn emptyShortestPaths(n_nodes: usize, src: usize, ok_val: bool) -> ShortestPaths {
    let dist: [MAX_NODES]i64 = undefined;
    let prev: [MAX_NODES]i16 = undefined;
    let i: usize = 0;
    while (i < MAX_NODES) {
        dist[i] = INF_DIST;
        prev[i] = NO_PREV;
        i += 1;
    }
    return ShortestPaths{
        ok: ok_val,
        node_count: n_nodes,
        source: src,
        dist: dist,
        prev: prev,
    };
}

pub struct WeightedGraph {
    pub node_count: usize,
    pub edge_count: usize,
    pub head: [MAX_NODES]i16,
    pub edge_src: [MAX_EDGES]u16,
    pub edge_dst: [MAX_EDGES]u16,
    pub edge_weight: [MAX_EDGES]i64,
    pub edge_next: [MAX_EDGES]i16,

    pub static fn init(n_nodes: usize) -> WeightedGraph {
        let clamped = n_nodes;
        if (clamped > MAX_NODES) {
            clamped = MAX_NODES;
        }
        let head: [MAX_NODES]i16 = undefined;
        let i: usize = 0;
        while (i < MAX_NODES) {
            head[i] = NO_EDGE;
            i += 1;
        }
        let esrc: [MAX_EDGES]u16 = undefined;
        let edst: [MAX_EDGES]u16 = undefined;
        let ew: [MAX_EDGES]i64 = undefined;
        let enext: [MAX_EDGES]i16 = undefined;
        let e: usize = 0;
        while (e < MAX_EDGES) {
            esrc[e] = 0 as u16;
            edst[e] = 0 as u16;
            ew[e] = 0 as i64;
            enext[e] = NO_EDGE;
            e += 1;
        }
        return WeightedGraph{
            node_count: clamped,
            edge_count: 0 as usize,
            head: head,
            edge_src: esrc,
            edge_dst: edst,
            edge_weight: ew,
            edge_next: enext,
        };
    }

    pub fn nodeCount() -> usize {
        return _.node_count;
    }

    pub fn edgeCount() -> usize {
        return _.edge_count;
    }

    pub fn addEdge(*_, src: usize, dst: usize, weight: i64) -> bool {
        if (src >= _.node_count || dst >= _.node_count || _.edge_count >= MAX_EDGES) {
            return false;
        }
        const eidx = _.edge_count;
        _.edge_src[eidx] = src as u16;
        _.edge_dst[eidx] = dst as u16;
        _.edge_weight[eidx] = weight;
        _.edge_next[eidx] = _.head[src];
        _.head[src] = eidx as i16;
        _.edge_count += 1;
        return true;
    }

    pub fn addUndirectedEdge(*_, u: usize, v: usize, weight: i64) -> bool {
        if (u >= _.node_count || v >= _.node_count || _.edge_count + (2 as usize) > MAX_EDGES) {
            return false;
        }
        const ok1 = _.addEdge(u, v, weight);
        const ok2 = _.addEdge(v, u, weight);
        return ok1 && ok2;
    }

    pub fn reset(*_, n_nodes: usize) {
        let clamped = n_nodes;
        if (clamped > MAX_NODES) {
            clamped = MAX_NODES;
        }
        _.node_count = clamped;
        _.edge_count = 0 as usize;
        let i: usize = 0;
        while (i < MAX_NODES) {
            _.head[i] = NO_EDGE;
            i += 1;
        }
    }

    pub fn dijkstra(source: usize) -> ShortestPaths {
        if (source >= _.node_count) {
            return emptyShortestPaths(_.node_count, source, false);
        }
        let sp = emptyShortestPaths(_.node_count, source, true);
        sp.dist[source] = 0 as i64;

        let visited: [MAX_NODES]bool = undefined;
        let v: usize = 0;
        while (v < MAX_NODES) {
            visited[v] = false;
            v += 1;
        }

        let iter: usize = 0;
        while (iter < _.node_count) {
            let bestNode: usize = MAX_NODES;
            let bestDist: i64 = INF_DIST;
            let u: usize = 0;
            while (u < _.node_count) {
                if (!visited[u] && sp.dist[u] < bestDist) {
                    bestDist = sp.dist[u];
                    bestNode = u;
                }
                u += 1;
            }
            if (bestNode >= _.node_count) {
                break;
            }
            visited[bestNode] = true;

            let e = _.head[bestNode];
            while (e >= (0 as i16)) {
                const eidx = e as usize;
                const w = _.edge_weight[eidx];
                if (w >= (0 as i64)) {
                    const nxt = _.edge_dst[eidx] as usize;
                    const cand = bestDist + w;
                    if (cand < sp.dist[nxt]) {
                        sp.dist[nxt] = cand;
                        sp.prev[nxt] = bestNode as i16;
                    }
                }
                e = _.edge_next[eidx];
            }
            iter += 1;
        }
        return sp;
    }

    pub fn bellmanFord(source: usize) -> ShortestPaths {
        if (source >= _.node_count) {
            return emptyShortestPaths(_.node_count, source, false);
        }
        let sp = emptyShortestPaths(_.node_count, source, true);
        sp.dist[source] = 0 as i64;

        let round: usize = 1;
        while (round < _.node_count) {
            let changed = false;
            let e: usize = 0;
            while (e < _.edge_count) {
                const u = _.edge_src[e] as usize;
                const v = _.edge_dst[e] as usize;
                const w = _.edge_weight[e];
                if (sp.dist[u] < INF_DIST) {
                    const cand = sp.dist[u] + w;
                    if (cand < sp.dist[v]) {
                        sp.dist[v] = cand;
                        sp.prev[v] = u as i16;
                        changed = true;
                    }
                }
                e += 1;
            }
            if (!changed) {
                break;
            }
            round += 1;
        }

        // Detect negative-weight cycle reachable from source
        let e2: usize = 0;
        while (e2 < _.edge_count) {
            const u = _.edge_src[e2] as usize;
            const v = _.edge_dst[e2] as usize;
            const w = _.edge_weight[e2];
            if (sp.dist[u] < INF_DIST && (sp.dist[u] + w) < sp.dist[v]) {
                sp.ok = false;
                return sp;
            }
            e2 += 1;
        }

        return sp;
    }
}

export {
    ShortestPaths,
    WeightedGraph,
};

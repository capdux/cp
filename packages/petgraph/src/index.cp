// @cap/petgraph-core — Zero-allocation directed/undirected graph & classic algorithms for CP
// Clean-room MIT implementation inspired by Rust `petgraph` v0.6 (950M+ downloads).
// Fixed pool: 64 nodes / 256 weighted edges with BFS, iterative DFS, Kahn topoSort,
// cycle detection, non-negative Dijkstra shortest paths, and reachability.

pub const PG_MAX_NODES: usize = 64;
pub const PG_MAX_EDGES: usize = 256;
pub const PG_INF: u64 = 0x3fffffffffffffff;

fn pgcSatAdd(a: u64, b: u64) -> u64 {
    if (a >= PG_INF || b >= PG_INF) {
        return PG_INF;
    }
    if (b >= PG_INF - a) {
        return PG_INF;
    }
    return a + b;
}

/// Zero-allocation weighted directed graph (max 64 nodes, 256 edges).
/// Supports undirected edges via symmetric directed pairs.
pub struct DiGraph {
    pub node_count: usize,
    pub edge_count: usize,
    pub edge_from: [256]usize,
    pub edge_to: [256]usize,
    pub edge_weight: [256]u64,
    pub head: [64]usize,
    pub tail: [64]usize,
    pub next_edge: [256]usize,
    pub out_deg: [64]usize,
    pub in_deg: [64]usize,

    pub static fn new() -> DiGraph {
        let ef: [256]usize = undefined;
        let et: [256]usize = undefined;
        let ew: [256]u64 = undefined;
        let ne: [256]usize = undefined;
        let hd: [64]usize = undefined;
        let tl: [64]usize = undefined;
        let od: [64]usize = undefined;
        let id: [64]usize = undefined;

        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            hd[i] = PG_MAX_EDGES;
            tl[i] = PG_MAX_EDGES;
            od[i] = 0 as usize;
            id[i] = 0 as usize;
            i += 1;
        }
        let j: usize = 0;
        while (j < PG_MAX_EDGES) {
            ef[j] = 0 as usize;
            et[j] = 0 as usize;
            ew[j] = 0 as u64;
            ne[j] = PG_MAX_EDGES;
            j += 1;
        }

        return DiGraph{
            node_count: 0 as usize,
            edge_count: 0 as usize,
            edge_from: ef,
            edge_to: et,
            edge_weight: ew,
            head: hd,
            tail: tl,
            next_edge: ne,
            out_deg: od,
            in_deg: id,
        };
    }

    pub fn nodeCount() -> usize {
        return _.node_count;
    }

    pub fn edgeCount() -> usize {
        return _.edge_count;
    }

    pub fn addNode(*_) -> usize? {
        if (_.node_count >= PG_MAX_NODES) {
            return null;
        }
        let id = _.node_count;
        _.node_count += 1;
        return id;
    }

    pub fn addEdge(*_, from_node: usize, to_node: usize, weight: u64) -> bool {
        if (from_node >= _.node_count || to_node >= _.node_count) {
            return false;
        }
        if (_.edge_count >= PG_MAX_EDGES) {
            return false;
        }
        let e = _.edge_count;
        _.edge_from[e] = from_node;
        _.edge_to[e] = to_node;
        _.edge_weight[e] = weight;
        _.next_edge[e] = PG_MAX_EDGES;

        if (_.head[from_node] == PG_MAX_EDGES) {
            _.head[from_node] = e;
            _.tail[from_node] = e;
        } else {
            _.next_edge[_.tail[from_node]] = e;
            _.tail[from_node] = e;
        }

        _.out_deg[from_node] += 1;
        _.in_deg[to_node] += 1;
        _.edge_count += 1;
        return true;
    }

    pub fn addUndirectedEdge(*_, a: usize, b: usize, weight: u64) -> bool {
        if (a >= _.node_count || b >= _.node_count) {
            return false;
        }
        if (_.edge_count + (2 as usize) > PG_MAX_EDGES) {
            return false;
        }
        let ok1 = _.addEdge(a, b, weight);
        let ok2 = _.addEdge(b, a, weight);
        return ok1 && ok2;
    }

    pub fn hasEdge(u: usize, v: usize) -> bool {
        if (u >= _.node_count || v >= _.node_count) {
            return false;
        }
        let e = _.head[u];
        while (e != PG_MAX_EDGES) {
            if (_.edge_to[e] == v) {
                return true;
            }
            e = _.next_edge[e];
        }
        return false;
    }

    pub fn outDegree(u: usize) -> usize {
        if (u >= _.node_count) {
            return 0 as usize;
        }
        return _.out_deg[u];
    }

    pub fn inDegree(u: usize) -> usize {
        if (u >= _.node_count) {
            return 0 as usize;
        }
        return _.in_deg[u];
    }

    pub fn bfs(start: usize, order_out: []usize) -> usize {
        if (start >= _.node_count) {
            return 0 as usize;
        }
        let visited: [64]bool = undefined;
        let queue: [64]usize = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            visited[i] = false;
            queue[i] = 0 as usize;
            i += 1;
        }

        let q_head: usize = 0;
        let q_tail: usize = 0;
        let count: usize = 0;

        visited[start] = true;
        queue[q_tail] = start;
        q_tail += 1;

        while (q_head < q_tail) {
            let u = queue[q_head];
            q_head += 1;
            if (count < order_out.len) {
                order_out[count] = u;
            }
            count += 1;

            let e = _.head[u];
            while (e != PG_MAX_EDGES) {
                let v = _.edge_to[e];
                if (!visited[v]) {
                    visited[v] = true;
                    queue[q_tail] = v;
                    q_tail += 1;
                }
                e = _.next_edge[e];
            }
        }
        return count;
    }

    pub fn dfs(start: usize, order_out: []usize) -> usize {
        if (start >= _.node_count) {
            return 0 as usize;
        }
        let visited: [64]bool = undefined;
        let stack: [256]usize = undefined;
        let nbrs: [256]usize = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            visited[i] = false;
            i += 1;
        }
        let j: usize = 0;
        while (j < PG_MAX_EDGES) {
            stack[j] = 0 as usize;
            nbrs[j] = 0 as usize;
            j += 1;
        }

        let sp: usize = 0;
        let count: usize = 0;
        stack[sp] = start;
        sp += 1;

        while (sp > (0 as usize)) {
            sp -= 1;
            let u = stack[sp];
            if (visited[u]) {
                continue;
            }
            visited[u] = true;
            if (count < order_out.len) {
                order_out[count] = u;
            }
            count += 1;

            // Collect outgoing neighbors and push in reverse so earlier edges visit first.
            let n_cnt: usize = 0;
            let e = _.head[u];
            while (e != PG_MAX_EDGES) {
                let v = _.edge_to[e];
                if (!visited[v] && n_cnt < PG_MAX_EDGES) {
                    nbrs[n_cnt] = v;
                    n_cnt += 1;
                }
                e = _.next_edge[e];
            }
            let k = n_cnt;
            while (k > (0 as usize)) {
                k -= 1;
                if (sp < PG_MAX_EDGES) {
                    stack[sp] = nbrs[k];
                    sp += 1;
                }
            }
        }
        return count;
    }

    pub fn topoSort(order_out: []usize) -> usize? {
        let in_rem: [64]usize = undefined;
        let queue: [64]usize = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            in_rem[i] = 0 as usize;
            queue[i] = 0 as usize;
            i += 1;
        }

        let q_head: usize = 0;
        let q_tail: usize = 0;
        let n: usize = 0;
        while (n < _.node_count) {
            in_rem[n] = _.in_deg[n];
            if (in_rem[n] == (0 as usize)) {
                queue[q_tail] = n;
                q_tail += 1;
            }
            n += 1;
        }

        let count: usize = 0;
        while (q_head < q_tail) {
            let u = queue[q_head];
            q_head += 1;
            if (count < order_out.len) {
                order_out[count] = u;
            }
            count += 1;

            let e = _.head[u];
            while (e != PG_MAX_EDGES) {
                let v = _.edge_to[e];
                in_rem[v] -= 1;
                if (in_rem[v] == (0 as usize)) {
                    queue[q_tail] = v;
                    q_tail += 1;
                }
                e = _.next_edge[e];
            }
        }

        if (count != _.node_count) {
            return null;
        }
        return count;
    }

    pub fn isCyclic() -> bool {
        let scratch: [64]usize = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            scratch[i] = 0 as usize;
            i += 1;
        }
        return _.topoSort(scratch[0..64]) == null;
    }

    pub fn dijkstra(start: usize, dist_out: []u64) -> bool {
        if (start >= _.node_count) {
            return false;
        }
        if (dist_out.len < _.node_count) {
            return false;
        }
        let dist: [64]u64 = undefined;
        let settled: [64]bool = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            dist[i] = PG_INF;
            settled[i] = false;
            i += 1;
        }
        dist[start] = 0 as u64;

        let step: usize = 0;
        while (step < _.node_count) {
            let best: usize = PG_MAX_NODES;
            let best_dist: u64 = PG_INF;
            let u: usize = 0;
            while (u < _.node_count) {
                if (!settled[u] && dist[u] < best_dist) {
                    best_dist = dist[u];
                    best = u;
                }
                u += 1;
            }
            if (best == PG_MAX_NODES) {
                break;
            }
            settled[best] = true;

            let e = _.head[best];
            while (e != PG_MAX_EDGES) {
                let v = _.edge_to[e];
                let cand = pgcSatAdd(dist[best], _.edge_weight[e]);
                if (cand < dist[v]) {
                    dist[v] = cand;
                }
                e = _.next_edge[e];
            }
            step += 1;
        }

        let k: usize = 0;
        while (k < _.node_count) {
            dist_out[k] = dist[k];
            k += 1;
        }
        return true;
    }

    pub fn hasPath(src: usize, dst: usize) -> bool {
        if (src >= _.node_count || dst >= _.node_count) {
            return false;
        }
        if (src == dst) {
            return true;
        }
        let visited: [64]bool = undefined;
        let queue: [64]usize = undefined;
        let i: usize = 0;
        while (i < PG_MAX_NODES) {
            visited[i] = false;
            queue[i] = 0 as usize;
            i += 1;
        }

        let q_head: usize = 0;
        let q_tail: usize = 0;
        visited[src] = true;
        queue[q_tail] = src;
        q_tail += 1;

        while (q_head < q_tail) {
            let u = queue[q_head];
            q_head += 1;
            let e = _.head[u];
            while (e != PG_MAX_EDGES) {
                let v = _.edge_to[e];
                if (v == dst) {
                    return true;
                }
                if (!visited[v]) {
                    visited[v] = true;
                    queue[q_tail] = v;
                    q_tail += 1;
                }
                e = _.next_edge[e];
            }
        }
        return false;
    }
}

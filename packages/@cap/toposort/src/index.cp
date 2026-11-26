// @cap/toposort — Zero-allocation deterministic Kahn topological sort & DAG level engine for CP
// Clean-room MIT implementation inspired by Rust `topological-sort`, `petgraph::algo::toposort`, and npm `toposort`.
//
// Features:
// - Fixed stack capacity: up to `MAX_NODES = 128` vertices and `MAX_EDGES = 512` directed edges
//   stored in a compressed adjacency list (forward-star representation).
// - Deterministic Kahn's algorithm with a binary min-heap priority queue guaranteeing the
//   lexicographically smallest topological ordering on every run.
// - Automatic cycle detection (`has_cycle`, `TopoResult.ok`) and parallel execution level
//   stratification (`TopoResult.levels[node]`, with in-degree 0 sources at level 0).
// - Zero heap allocation.

pub const MAX_NODES: usize = 128;
pub const MAX_EDGES: usize = 512;

pub struct TopoResult {
    pub ok: bool,
    pub count: usize,
    pub order: [MAX_NODES]u16,
    pub levels: [MAX_NODES]u16,

    pub static fn empty() -> TopoResult {
        let ord: [MAX_NODES]u16 = undefined;
        let lvl: [MAX_NODES]u16 = undefined;
        let i: usize = 0;
        while (i < MAX_NODES) {
            ord[i] = 0;
            lvl[i] = 0;
            i += 1;
        }
        return TopoResult{
            ok: true,
            count: 0,
            order: ord,
            levels: lvl,
        };
    }
}

fn clampNodes(n: usize) -> usize {
    if (n > MAX_NODES) {
        return MAX_NODES;
    }
    return n;
}

/// Directed graph for zero-allocation topological sorting, cycle detection, and DAG level scheduling.
pub struct TopoGraph {
    pub node_count: usize,
    pub edge_count: usize,
    pub head: [MAX_NODES]i16,
    pub edge_to: [MAX_EDGES]u16,
    pub edge_next: [MAX_EDGES]i16,
    pub in_deg: [MAX_NODES]u16,
    pub out_deg: [MAX_NODES]u16,

    /// Initializes a directed graph with `node_count` vertices (clamped to `MAX_NODES = 128`).
    pub static fn init(node_count: usize) -> TopoGraph {
        const n = clampNodes(node_count);
        let hd: [MAX_NODES]i16 = undefined;
        let to: [MAX_EDGES]u16 = undefined;
        let nxt: [MAX_EDGES]i16 = undefined;
        let ind: [MAX_NODES]u16 = undefined;
        let outd: [MAX_NODES]u16 = undefined;

        let i: usize = 0;
        while (i < MAX_NODES) {
            hd[i] = -1;
            ind[i] = 0;
            outd[i] = 0;
            i += 1;
        }
        let e: usize = 0;
        while (e < MAX_EDGES) {
            to[e] = 0;
            nxt[e] = -1;
            e += 1;
        }

        return TopoGraph{
            node_count: n,
            edge_count: 0,
            head: hd,
            edge_to: to,
            edge_next: nxt,
            in_deg: ind,
            out_deg: outd,
        };
    }

    /// Resets the graph in place with `node_count` vertices and zero edges.
    pub fn reset(*_, node_count: usize) {
        const n = clampNodes(node_count);
        _.node_count = n;
        _.edge_count = 0;
        let i: usize = 0;
        while (i < n) {
            _.head[i] = -1;
            _.in_deg[i] = 0;
            _.out_deg[i] = 0;
            i += 1;
        }
    }

    /// Adds a directed dependency edge `src -> dst` (`src` must execute before `dst`).
    /// Returns `false` if either vertex is out of range or `MAX_EDGES` (512) is reached.
    pub fn add_edge(*_, src: usize, dst: usize) -> bool {
        if (src >= _.node_count || dst >= _.node_count || _.edge_count >= MAX_EDGES) {
            return false;
        }
        const idx = _.edge_count;
        _.edge_to[idx] = dst as u16;
        _.edge_next[idx] = _.head[src];
        _.head[src] = idx as i16;
        _.edge_count += 1;
        _.out_deg[src] += 1;
        _.in_deg[dst] += 1;
        return true;
    }

    pub fn in_degree(node: usize) -> u16 {
        if (node >= _.node_count) {
            return 0;
        }
        return _.in_deg[node];
    }

    pub fn out_degree(node: usize) -> u16 {
        if (node >= _.node_count) {
            return 0;
        }
        return _.out_deg[node];
    }

    /// Runs deterministic Kahn's topological sort using a min-heap over ready node IDs,
    /// producing the lexicographically smallest valid order and parallel DAG execution levels.
    pub fn sort() -> TopoResult {
        let res = TopoResult.empty();
        const n = _.node_count;
        if (n == 0) {
            res.ok = true;
            res.count = 0;
            return res;
        }

        let rem_in: [MAX_NODES]u16 = undefined;
        let heap: [MAX_NODES]u16 = undefined;
        let heap_len: usize = 0;

        let i: usize = 0;
        while (i < n) {
            rem_in[i] = _.in_deg[i];
            res.levels[i] = 0;
            if (rem_in[i] == 0) {
                // Pushing in ascending node ID order already satisfies min-heap invariant
                heap[heap_len] = i as u16;
                heap_len += 1;
            }
            i += 1;
        }

        while (heap_len > 0) {
            // Pop smallest ready vertex from min-heap
            const u = heap[0] as usize;
            heap_len -= 1;
            if (heap_len > 0) {
                heap[0] = heap[heap_len];
                let pos: usize = 0;
                while (true) {
                    const left = pos * 2 + 1;
                    if (left >= heap_len) {
                        break;
                    }
                    let smallest = left;
                    const right = left + 1;
                    if (right < heap_len && heap[right] < heap[left]) {
                        smallest = right;
                    }
                    if (heap[smallest] >= heap[pos]) {
                        break;
                    }
                    const tmp = heap[pos];
                    heap[pos] = heap[smallest];
                    heap[smallest] = tmp;
                    pos = smallest;
                }
            }

            res.order[res.count] = u as u16;
            res.count += 1;

            // Relax all outgoing edges u -> v
            let e = _.head[u];
            while (e >= 0) {
                const e_idx = e as usize;
                const v = _.edge_to[e_idx] as usize;
                const cand_level: u16 = res.levels[u] + 1;
                if (cand_level > res.levels[v]) {
                    res.levels[v] = cand_level;
                }
                rem_in[v] -= 1;
                if (rem_in[v] == 0) {
                    // Push v into min-heap & sift up
                    let pos = heap_len;
                    heap[pos] = v as u16;
                    heap_len += 1;
                    while (pos > 0) {
                        const parent = (pos - 1) / 2;
                        if (heap[parent] <= heap[pos]) {
                            break;
                        }
                        const tmp = heap[parent];
                        heap[parent] = heap[pos];
                        heap[pos] = tmp;
                        pos = parent;
                    }
                }
                e = _.edge_next[e_idx];
            }
        }

        res.ok = (res.count == n);
        return res;
    }

    /// Returns `true` if the directed graph contains at least one cycle.
    pub fn has_cycle() -> bool {
        return !_.sort().ok;
    }
}

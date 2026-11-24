// Origin: Pure-CP Stdlib (Fused Memoirist + Matchit Zero-Heap Radix Router: cap:web)
// ============================================================================
// High-Performance Zero-Heap HTTP Radix Router combining:
//   - Elysia's Memoirist O(1) Static Path Hash Table (FNV-1a + linear probing)
//   - Axum's Matchit Compressed Segment Radix Trie (:param, {param}, *wildcard)
//   - Per-Method Handler Matrix + 405 Allow Bitmask + Axum HEAD->GET Fallback
// ============================================================================

pub const RADIX_STATIC_BUCKETS: usize = 512;
pub const RADIX_MAX_NODES: usize = 512;
pub const RADIX_MAX_CHILDREN: usize = 16;
pub const RADIX_MAX_PARAMS: usize = 16;
pub const RADIX_MAX_SEG_LEN: usize = 64;
pub const RADIX_MAX_PATH_LEN: usize = 256;
pub const RADIX_METHOD_SLOTS: usize = 7;

fn rxBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn rxFnv1a64(s: string): u64 {
    let h: u64 = ((0xcbf29ce4 as u64) << 32) | (0x84222325 as u64);
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        h = h ^ (s[i] as u64);
        h = h *% 1099511628211;
    }
    return h;
}

fn rxCopyInto(dst: []u8, src: string): usize {
    let n = src.len;
    if (n > dst.len) n = dst.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        dst[i] = src[i];
    }
    return n;
}

pub struct RadixParam {
    name: string = "";
    value: string = "";
}

pub struct RadixMatchResult {
    matched: bool = false;
    handler_id: i32 = -1;
    allowed_mask: u8 = 0;
    head_fallback_to_get: bool = false;
    params: [RADIX_MAX_PARAMS]RadixParam = undefined;
    param_len: usize = 0;
}

struct StaticBucket {
    occupied: bool = false;
    hash: u64 = 0;
    path_buf: [RADIX_MAX_PATH_LEN]u8 = [0; RADIX_MAX_PATH_LEN];
    path_len: usize = 0;
    handlers: [RADIX_METHOD_SLOTS]i32 = [-1; RADIX_METHOD_SLOTS];
    allowed_mask: u8 = 0;
}

struct StaticChildEdge {
    seg_buf: [RADIX_MAX_SEG_LEN]u8 = [0; RADIX_MAX_SEG_LEN];
    seg_len: usize = 0;
    child_node: i32 = -1;
}

struct RadixNode {
    static_edges: [RADIX_MAX_CHILDREN]StaticChildEdge = [StaticChildEdge{}; RADIX_MAX_CHILDREN];
    static_edge_len: usize = 0;

    param_child: i32 = -1;
    param_name_buf: [RADIX_MAX_SEG_LEN]u8 = [0; RADIX_MAX_SEG_LEN];
    param_name_len: usize = 0;

    wildcard_child: i32 = -1;
    wildcard_name_buf: [RADIX_MAX_SEG_LEN]u8 = [0; RADIX_MAX_SEG_LEN];
    wildcard_name_len: usize = 0;

    handlers: [RADIX_METHOD_SLOTS]i32 = [-1; RADIX_METHOD_SLOTS];
    allowed_mask: u8 = 0;
}

fn isDynamicPath(path: string): bool {
    let i: usize = 0;
    while (i < path.len) : (i += 1) {
        const c = path[i];
        if (c == ':' || c == '*' || c == '{') return true;
    }
    return false;
}

pub struct RadixRouter {
    static_table: [RADIX_STATIC_BUCKETS]StaticBucket = [StaticBucket{}; RADIX_STATIC_BUCKETS];
    static_count: usize = 0;
    nodes: [RADIX_MAX_NODES]RadixNode = [RadixNode{}; RADIX_MAX_NODES];
    node_count: usize = 1; // index 0 is root

    pub static fn init(): RadixRouter {
        return RadixRouter{
            static_table: [StaticBucket{}; RADIX_STATIC_BUCKETS],
            static_count: 0,
            nodes: [RadixNode{}; RADIX_MAX_NODES],
            node_count: 1,
        };
    }

    /// Insert a route into either the O(1) static hash table or the segment radix trie.
    /// `method_idx`: 0=GET, 1=POST, 2=PUT, 3=DELETE, 4=PATCH, 5=HEAD, 6=OPTIONS.
    pub fn insert(*_, method_idx: usize, path: string, handler_id: i32): bool {
        if (method_idx >= RADIX_METHOD_SLOTS) return false;
        const m_mask: u8 = (1 as u8) << (method_idx as u3);

        // Fast-path: pure static routes go into O(1) hash table
        if (!isDynamicPath(path) && path.len <= RADIX_MAX_PATH_LEN) {
            const h = rxFnv1a64(path);
            const mask = RADIX_STATIC_BUCKETS - 1;
            let idx = (h as usize) & mask;
            let probe: usize = 0;
            while (probe < RADIX_STATIC_BUCKETS) : (probe += 1) {
                if (!_.static_table[idx].occupied) {
                    _.static_table[idx].occupied = true;
                    _.static_table[idx].hash = h;
                    _.static_table[idx].path_len = rxCopyInto(_.static_table[idx].path_buf[0..], path);
                    _.static_table[idx].handlers[method_idx] = handler_id;
                    _.static_table[idx].allowed_mask = _.static_table[idx].allowed_mask | m_mask;
                    _.static_count += 1;
                    return true;
                }
                if (_.static_table[idx].hash == h
                    && rxBytesEql(_.static_table[idx].path_buf[0.._.static_table[idx].path_len], path))
                {
                    _.static_table[idx].handlers[method_idx] = handler_id;
                    _.static_table[idx].allowed_mask = _.static_table[idx].allowed_mask | m_mask;
                    return true;
                }
                idx = (idx + 1) & mask;
            }
            return false;
        }

        // Dynamic path: insert into compressed segment Radix Trie
        let cur: usize = 0;
        let i: usize = 0;
        while (i < path.len) {
            while (i < path.len && path[i] == '/') : (i += 1) {}
            if (i >= path.len) break;
            const seg_start = i;
            while (i < path.len && path[i] != '/') : (i += 1) {}
            const seg = path[seg_start..i];

            if (seg[0] == '*') {
                // Wildcard catch-all terminal segment
                const wname = if (seg.len > 1) seg[1..] else "wildcard";
                if (_.nodes[cur].wildcard_child < 0) {
                    if (_.node_count >= RADIX_MAX_NODES) return false;
                    const nid = _.node_count as i32;
                    _.node_count += 1;
                    _.nodes[cur].wildcard_child = nid;
                    _.nodes[cur].wildcard_name_len = rxCopyInto(_.nodes[cur].wildcard_name_buf[0..], wname);
                }
                cur = _.nodes[cur].wildcard_child as usize;
                break;
            } else if (seg[0] == ':' || (seg.len >= 3 && seg[0] == '{' && seg[seg.len - 1] == '}')) {
                // Parametric segment: supports both Elysia `:id` and Axum `{id}`
                const pname = if (seg[0] == ':')
                    (if (seg.len > 1) seg[1..] else "param")
                else
                    seg[1 .. seg.len - 1];
                if (_.nodes[cur].param_child < 0) {
                    if (_.node_count >= RADIX_MAX_NODES) return false;
                    const nid = _.node_count as i32;
                    _.node_count += 1;
                    _.nodes[cur].param_child = nid;
                    _.nodes[cur].param_name_len = rxCopyInto(_.nodes[cur].param_name_buf[0..], pname);
                }
                cur = _.nodes[cur].param_child as usize;
            } else {
                // Static segment edge
                let found_child: i32 = -1;
                let e_idx: usize = 0;
                while (e_idx < _.nodes[cur].static_edge_len) : (e_idx += 1) {
                    const elen = _.nodes[cur].static_edges[e_idx].seg_len;
                    if (rxBytesEql(_.nodes[cur].static_edges[e_idx].seg_buf[0..elen], seg)) {
                        found_child = _.nodes[cur].static_edges[e_idx].child_node;
                        break;
                    }
                }
                if (found_child < 0) {
                    if (_.node_count >= RADIX_MAX_NODES || _.nodes[cur].static_edge_len >= RADIX_MAX_CHILDREN) {
                        return false;
                    }
                    const nid = _.node_count as i32;
                    _.node_count += 1;
                    const slot = _.nodes[cur].static_edge_len;
                    _.nodes[cur].static_edges[slot].seg_len = rxCopyInto(_.nodes[cur].static_edges[slot].seg_buf[0..], seg);
                    _.nodes[cur].static_edges[slot].child_node = nid;
                    _.nodes[cur].static_edge_len += 1;
                    found_child = nid;
                }
                cur = found_child as usize;
            }
        }

        _.nodes[cur].handlers[method_idx] = handler_id;
        _.nodes[cur].allowed_mask = _.nodes[cur].allowed_mask | m_mask;
        return true;
    }

    fn lookupStatic(*_, method_idx: usize, path: string, out: *RadixMatchResult): bool {
        if (_.static_count == 0 || path.len > RADIX_MAX_PATH_LEN) return false;
        const h = rxFnv1a64(path);
        const mask = RADIX_STATIC_BUCKETS - 1;
        let idx = (h as usize) & mask;
        let probe: usize = 0;
        while (probe < RADIX_STATIC_BUCKETS) : (probe += 1) {
            if (!_.static_table[idx].occupied) return false;
            if (_.static_table[idx].hash == h
                && rxBytesEql(_.static_table[idx].path_buf[0.._.static_table[idx].path_len], path))
            {
                out.allowed_mask = out.allowed_mask | _.static_table[idx].allowed_mask;
                const hid = _.static_table[idx].handlers[method_idx];
                if (hid >= 0) {
                    out.matched = true;
                    out.handler_id = hid;
                    return true;
                }
                // Axum-style HEAD -> GET automatic fallback
                if (method_idx == 5 && _.static_table[idx].handlers[0] >= 0) {
                    out.matched = true;
                    out.handler_id = _.static_table[idx].handlers[0];
                    out.head_fallback_to_get = true;
                    return true;
                }
                return false;
            }
            idx = (idx + 1) & mask;
        }
        return false;
    }

    fn matchTrieNode(
        *_,
        node_idx: usize,
        method_idx: usize,
        path: string,
        pos: usize,
        out: *RadixMatchResult,
    ): bool {
        let i = pos;
        while (i < path.len && path[i] == '/') : (i += 1) {}

        // Reached end of path
        if (i >= path.len) {
            out.allowed_mask = out.allowed_mask | _.nodes[node_idx].allowed_mask;
            const hid = _.nodes[node_idx].handlers[method_idx];
            if (hid >= 0) {
                out.matched = true;
                out.handler_id = hid;
                return true;
            }
            if (method_idx == 5 && _.nodes[node_idx].handlers[0] >= 0) {
                out.matched = true;
                out.handler_id = _.nodes[node_idx].handlers[0];
                out.head_fallback_to_get = true;
                return true;
            }
            // Empty wildcard match (e.g. `/files/*path` matching `/files`)
            if (_.nodes[node_idx].wildcard_child >= 0) {
                const wnode = _.nodes[node_idx].wildcard_child as usize;
                out.allowed_mask = out.allowed_mask | _.nodes[wnode].allowed_mask;
                const whid = _.nodes[wnode].handlers[method_idx];
                if (whid >= 0) {
                    if (out.param_len < RADIX_MAX_PARAMS) {
                        const wlen = _.nodes[node_idx].wildcard_name_len;
                        out.params[out.param_len] = RadixParam{
                            name: _.nodes[node_idx].wildcard_name_buf[0..wlen],
                            value: "",
                        };
                        out.param_len += 1;
                    }
                    out.matched = true;
                    out.handler_id = whid;
                    return true;
                }
            }
            return false;
        }

        const seg_start = i;
        while (i < path.len && path[i] != '/') : (i += 1) {}
        const seg = path[seg_start..i];
        const saved_plen = out.param_len;

        // Priority 1: Static segment edge
        let e_idx: usize = 0;
        while (e_idx < _.nodes[node_idx].static_edge_len) : (e_idx += 1) {
            const elen = _.nodes[node_idx].static_edges[e_idx].seg_len;
            if (rxBytesEql(_.nodes[node_idx].static_edges[e_idx].seg_buf[0..elen], seg)) {
                const child = _.nodes[node_idx].static_edges[e_idx].child_node as usize;
                if (_.matchTrieNode(child, method_idx, path, i, out)) {
                    return true;
                }
                out.param_len = saved_plen;
                break;
            }
        }

        // Priority 2: Parametric segment edge (:param / {param})
        if (_.nodes[node_idx].param_child >= 0) {
            const pchild = _.nodes[node_idx].param_child as usize;
            if (out.param_len < RADIX_MAX_PARAMS) {
                const plen = _.nodes[node_idx].param_name_len;
                out.params[out.param_len] = RadixParam{
                    name: _.nodes[node_idx].param_name_buf[0..plen],
                    value: seg,
                };
                out.param_len += 1;
            }
            if (_.matchTrieNode(pchild, method_idx, path, i, out)) {
                return true;
            }
            out.param_len = saved_plen;
        }

        // Priority 3: Wildcard catch-all edge (*wildcard)
        if (_.nodes[node_idx].wildcard_child >= 0) {
            const wnode = _.nodes[node_idx].wildcard_child as usize;
            out.allowed_mask = out.allowed_mask | _.nodes[wnode].allowed_mask;
            let whid = _.nodes[wnode].handlers[method_idx];
            let is_head_fb = false;
            if (whid < 0 && method_idx == 5 && _.nodes[wnode].handlers[0] >= 0) {
                whid = _.nodes[wnode].handlers[0];
                is_head_fb = true;
            }
            if (whid >= 0) {
                if (out.param_len < RADIX_MAX_PARAMS) {
                    const wlen = _.nodes[node_idx].wildcard_name_len;
                    out.params[out.param_len] = RadixParam{
                        name: _.nodes[node_idx].wildcard_name_buf[0..wlen],
                        value: path[seg_start..],
                    };
                    out.param_len += 1;
                }
                out.matched = true;
                out.handler_id = whid;
                out.head_fallback_to_get = is_head_fb;
                return true;
            }
        }

        return false;
    }

    /// Match an incoming HTTP method & normalized path.
    /// Zero heap allocations; O(1) for static routes, O(depth) for parametric/wildcard routes.
    pub fn lookup(*_, method_idx: usize, path: string): RadixMatchResult {
        let res = RadixMatchResult{};
        if (method_idx >= RADIX_METHOD_SLOTS) return res;

        // 1. O(1) static fast-path
        if (_.lookupStatic(method_idx, path, *res)) {
            return res;
        }

        // 2. Dynamic Radix Trie fallback (also collects allowed_mask for 405)
        if (_.node_count > 1) {
            * = _.matchTrieNode(0, method_idx, path, 0, *res);
        }
        return res;
    }

    pub fn staticRouteCount(): usize {
        return _.static_count;
    }

    pub fn trieNodeCount(): usize {
        return _.node_count;
    }
}

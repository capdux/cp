// @cap/flatted — Zero-allocation circular-reference-safe flattened JSON codec
// Clean-room implementation modeled on npm `flatted` (WebReflection).

pub const FLAT_MAX_NODES: usize = 64;
pub const FLAT_MAX_EDGES: usize = 256;
pub const FLAT_NONE: i32 = -1;

pub enum FlatNodeKind {
    Null,
    Bool,
    Int,
    Str,
    Array,
    Object,
}

pub struct FlatNode {
    kind: FlatNodeKind,
    bool_val: bool,
    int_val: i64,
    str_val: string,
    first_edge: i32,
    last_edge: i32,
    edge_count: usize,
}

struct FlatEdge {
    key: string,
    child_id: usize,
    next: i32,
}

fn fltEmptyNode(): FlatNode {
    return FlatNode{
        kind: .Null,
        bool_val: false,
        int_val: 0,
        str_val: "",
        first_edge: FLAT_NONE,
        last_edge: FLAT_NONE,
        edge_count: 0,
    };
}

fn fltEmptyEdge(): FlatEdge {
    return FlatEdge{
        key: "",
        child_id: 0,
        next: FLAT_NONE,
    };
}

fn fltStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

pub struct FlatArena {
    nodes: [64]FlatNode,
    node_count: usize,
    edges: [256]FlatEdge,
    edge_count: usize,

    static fn init(): FlatArena {
        let a = FlatArena{
            nodes: undefined,
            node_count: 0,
            edges: undefined,
            edge_count: 0,
        };
        let i: usize = 0;
        while (i < FLAT_MAX_NODES) : (i += 1) {
            a.nodes[i] = fltEmptyNode();
        }
        let j: usize = 0;
        while (j < FLAT_MAX_EDGES) : (j += 1) {
            a.edges[j] = fltEmptyEdge();
        }
        return a;
    }

    fn reset(*_) {
        _.node_count = 0;
        _.edge_count = 0;
    }

    fn nodeCount(): usize {
        return _.node_count;
    }

    fn addNull(*_): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Null;
        _.node_count += 1;
        return id;
    }

    fn addBool(*_, v: bool): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Bool;
        _.nodes[id].bool_val = v;
        _.node_count += 1;
        return id;
    }

    fn addInt(*_, v: i64): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Int;
        _.nodes[id].int_val = v;
        _.node_count += 1;
        return id;
    }

    fn addStr(*_, s: string): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Str;
        _.nodes[id].str_val = s;
        _.node_count += 1;
        return id;
    }

    fn addArray(*_): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Array;
        _.node_count += 1;
        return id;
    }

    fn addObject(*_): usize {
        if (_.node_count >= FLAT_MAX_NODES) return FLAT_MAX_NODES;
        const id = _.node_count;
        _.nodes[id] = fltEmptyNode();
        _.nodes[id].kind = .Object;
        _.node_count += 1;
        return id;
    }

    fn setField(*_, obj_id: usize, key: string, child_id: usize): bool {
        if (obj_id >= _.node_count || child_id >= _.node_count) return false;
        if (_.nodes[obj_id].kind != .Object) return false;

        // Update existing key in-place if present
        let cur = _.nodes[obj_id].first_edge;
        while (cur != FLAT_NONE) {
            const idx = usize(cur);
            if (fltStrEq(_.edges[idx].key, key)) {
                _.edges[idx].child_id = child_id;
                return true;
            }
            cur = _.edges[idx].next;
        }

        if (_.edge_count >= FLAT_MAX_EDGES) return false;
        const eid = _.edge_count;
        _.edges[eid] = FlatEdge{
            key: key,
            child_id: child_id,
            next: FLAT_NONE,
        };
        _.edge_count += 1;

        if (_.nodes[obj_id].first_edge == FLAT_NONE) {
            _.nodes[obj_id].first_edge = i32(eid);
            _.nodes[obj_id].last_edge = i32(eid);
        } else {
            const prev = usize(_.nodes[obj_id].last_edge);
            _.edges[prev].next = i32(eid);
            _.nodes[obj_id].last_edge = i32(eid);
        }
        _.nodes[obj_id].edge_count += 1;
        return true;
    }

    fn pushElem(*_, arr_id: usize, child_id: usize): bool {
        if (arr_id >= _.node_count || child_id >= _.node_count) return false;
        if (_.nodes[arr_id].kind != .Array) return false;
        if (_.edge_count >= FLAT_MAX_EDGES) return false;

        const eid = _.edge_count;
        _.edges[eid] = FlatEdge{
            key: "",
            child_id: child_id,
            next: FLAT_NONE,
        };
        _.edge_count += 1;

        if (_.nodes[arr_id].first_edge == FLAT_NONE) {
            _.nodes[arr_id].first_edge = i32(eid);
            _.nodes[arr_id].last_edge = i32(eid);
        } else {
            const prev = usize(_.nodes[arr_id].last_edge);
            _.edges[prev].next = i32(eid);
            _.nodes[arr_id].last_edge = i32(eid);
        }
        _.nodes[arr_id].edge_count += 1;
        return true;
    }

    fn nodeKind(id: usize): FlatNodeKind {
        if (id >= _.node_count) return .Null;
        return _.nodes[id].kind;
    }

    fn nodeBool(id: usize): bool {
        if (id >= _.node_count) return false;
        return _.nodes[id].bool_val;
    }

    fn nodeInt(id: usize): i64 {
        if (id >= _.node_count) return 0;
        return _.nodes[id].int_val;
    }

    fn nodeStr(id: usize): string {
        if (id >= _.node_count) return "";
        return _.nodes[id].str_val;
    }

    fn elemCount(arr_id: usize): usize {
        if (arr_id >= _.node_count) return 0;
        return _.nodes[arr_id].edge_count;
    }

    fn getField(obj_id: usize, key: string): i64 {
        if (obj_id >= _.node_count) return -1;
        let cur = _.nodes[obj_id].first_edge;
        while (cur != FLAT_NONE) {
            const idx = usize(cur);
            if (fltStrEq(_.edges[idx].key, key)) {
                return i64(_.edges[idx].child_id);
            }
            cur = _.edges[idx].next;
        }
        return -1;
    }

    fn getElem(arr_id: usize, idx: usize): i64 {
        if (arr_id >= _.node_count) return -1;
        let cur = _.nodes[arr_id].first_edge;
        let k: usize = 0;
        while (cur != FLAT_NONE) {
            const eidx = usize(cur);
            if (k == idx) {
                return i64(_.edges[eidx].child_id);
            }
            cur = _.edges[eidx].next;
            k += 1;
        }
        return -1;
    }
}

fn fltPush(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn fltPushSlice(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        p = fltPush(out, p, s[i]);
    }
    return p;
}

fn fltPushUsize(out: []u8, pos: usize, val: usize): usize {
    let digits: [20]u8 = undefined;
    let count: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (v > 0 && count < 20) {
            digits[count] = u8('0') + u8(v % 10);
            count += 1;
            v = v / 10;
        }
    }
    let p = pos;
    let k = count;
    while (k > 0) {
        k -= 1;
        p = fltPush(out, p, digits[k]);
    }
    return p;
}

fn fltPushI64(out: []u8, pos: usize, val: i64): usize {
    let p = pos;
    if (val < 0) {
        p = fltPush(out, p, '-');
        // Avoid overflow for i64 min by computing in u64
        const mag: u64 = @bitCast(-val);
        let digits: [20]u8 = undefined;
        let count: usize = 0;
        let v = mag;
        while (v > 0 && count < 20) {
            digits[count] = u8('0') + u8(v % 10);
            count += 1;
            v = v / 10;
        }
        let k = count;
        while (k > 0) {
            k -= 1;
            p = fltPush(out, p, digits[k]);
        }
        return p;
    }
    return fltPushUsize(out, p, usize(val));
}

fn fltPushJsonString(out: []u8, pos: usize, s: string): usize {
    let p = fltPush(out, pos, '"');
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b == '"') {
            p = fltPush(out, p, '\\');
            p = fltPush(out, p, '"');
        } else if (b == '\\') {
            p = fltPush(out, p, '\\');
            p = fltPush(out, p, '\\');
        } else if (b == '\n') {
            p = fltPush(out, p, '\\');
            p = fltPush(out, p, 'n');
        } else if (b == '\r') {
            p = fltPush(out, p, '\\');
            p = fltPush(out, p, 'r');
        } else if (b == '\t') {
            p = fltPush(out, p, '\\');
            p = fltPush(out, p, 't');
        } else {
            p = fltPush(out, p, b);
        }
    }
    p = fltPush(out, p, '"');
    return p;
}

fn fltNeedsSlot(kind: FlatNodeKind): bool {
    return kind == .Str || kind == .Array || kind == .Object;
}

/// Serialize the graph rooted at `root_id` into standard `flatted` JSON array format.
/// Cycles and repeated strings are deduplicated via stringified slot indices (e.g. `"0"`).
pub fn stringifyFlatted(arena: FlatArena, root_id: usize, out: []u8): usize {
    if (root_id >= arena.node_count) return 0;

    let node_to_slot: [64]i32 = undefined;
    let slot_to_node: [64]usize = undefined;
    let i: usize = 0;
    while (i < FLAT_MAX_NODES) : (i += 1) {
        node_to_slot[i] = FLAT_NONE;
        slot_to_node[i] = 0;
    }

    // Slot 0 is always root_id
    node_to_slot[root_id] = 0;
    slot_to_node[0] = root_id;
    let slot_count: usize = 1;

    // BFS discovery: assign slot indices to all reachable Str/Array/Object nodes
    let head: usize = 0;
    while (head < slot_count) : (head += 1) {
        const nid = slot_to_node[head];
        const kind = arena.nodes[nid].kind;
        if (kind == .Array || kind == .Object) {
            let cur = arena.nodes[nid].first_edge;
            while (cur != FLAT_NONE) {
                const eidx = usize(cur);
                const cid = arena.edges[eidx].child_id;
                const ckind = arena.nodes[cid].kind;
                if (fltNeedsSlot(ckind) && node_to_slot[cid] == FLAT_NONE) {
                    // Deduplicate identical strings against existing Str slots
                    let dup_slot: i32 = FLAT_NONE;
                    if (ckind == .Str) {
                        let s_idx: usize = 0;
                        while (s_idx < slot_count) : (s_idx += 1) {
                            const snid = slot_to_node[s_idx];
                            if (arena.nodes[snid].kind == .Str && fltStrEq(arena.nodes[snid].str_val, arena.nodes[cid].str_val)) {
                                dup_slot = i32(s_idx);
                                break;
                            }
                        }
                    }
                    if (dup_slot != FLAT_NONE) {
                        node_to_slot[cid] = dup_slot;
                    } else if (slot_count < FLAT_MAX_NODES) {
                        node_to_slot[cid] = i32(slot_count);
                        slot_to_node[slot_count] = cid;
                        slot_count += 1;
                    }
                }
                cur = arena.edges[eidx].next;
            }
        }
    }

    // Emit JSON array: [slot0, slot1, ...]
    let pos: usize = 0;
    pos = fltPush(out, pos, '[');
    let s: usize = 0;
    while (s < slot_count) : (s += 1) {
        if (s > 0) pos = fltPush(out, pos, ',');
        const nid = slot_to_node[s];
        const node = arena.nodes[nid];
        if (node.kind == .Null) {
            pos = fltPushSlice(out, pos, "null");
        } else if (node.kind == .Bool) {
            if (node.bool_val) {
                pos = fltPushSlice(out, pos, "true");
            } else {
                pos = fltPushSlice(out, pos, "false");
            }
        } else if (node.kind == .Int) {
            pos = fltPushI64(out, pos, node.int_val);
        } else if (node.kind == .Str) {
            pos = fltPushJsonString(out, pos, node.str_val);
        } else if (node.kind == .Array) {
            pos = fltPush(out, pos, '[');
            let cur = node.first_edge;
            let first = true;
            while (cur != FLAT_NONE) {
                if (!first) pos = fltPush(out, pos, ',');
                first = false;
                const eidx = usize(cur);
                const cid = arena.edges[eidx].child_id;
                const cnode = arena.nodes[cid];
                if (fltNeedsSlot(cnode.kind)) {
                    pos = fltPush(out, pos, '"');
                    pos = fltPushUsize(out, pos, usize(node_to_slot[cid]));
                    pos = fltPush(out, pos, '"');
                } else if (cnode.kind == .Null) {
                    pos = fltPushSlice(out, pos, "null");
                } else if (cnode.kind == .Bool) {
                    if (cnode.bool_val) {
                        pos = fltPushSlice(out, pos, "true");
                    } else {
                        pos = fltPushSlice(out, pos, "false");
                    }
                } else if (cnode.kind == .Int) {
                    pos = fltPushI64(out, pos, cnode.int_val);
                }
                cur = arena.edges[eidx].next;
            }
            pos = fltPush(out, pos, ']');
        } else if (node.kind == .Object) {
            pos = fltPush(out, pos, '{');
            let cur = node.first_edge;
            let first = true;
            while (cur != FLAT_NONE) {
                if (!first) pos = fltPush(out, pos, ',');
                first = false;
                const eidx = usize(cur);
                pos = fltPushJsonString(out, pos, arena.edges[eidx].key);
                pos = fltPush(out, pos, ':');
                const cid = arena.edges[eidx].child_id;
                const cnode = arena.nodes[cid];
                if (fltNeedsSlot(cnode.kind)) {
                    pos = fltPush(out, pos, '"');
                    pos = fltPushUsize(out, pos, usize(node_to_slot[cid]));
                    pos = fltPush(out, pos, '"');
                } else if (cnode.kind == .Null) {
                    pos = fltPushSlice(out, pos, "null");
                } else if (cnode.kind == .Bool) {
                    if (cnode.bool_val) {
                        pos = fltPushSlice(out, pos, "true");
                    } else {
                        pos = fltPushSlice(out, pos, "false");
                    }
                } else if (cnode.kind == .Int) {
                    pos = fltPushI64(out, pos, cnode.int_val);
                }
                cur = arena.edges[eidx].next;
            }
            pos = fltPush(out, pos, '}');
        }
    }
    pos = fltPush(out, pos, ']');
    return pos;
}

// --- Zero-allocation Flatted JSON Parser ---

struct FltSpan {
    ok: bool,
    start: usize,
    end: usize,
    next: usize,
}

fn fltSkipWs(s: string, pos: usize): usize {
    let i = pos;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b != ' ' && b != '\t' && b != '\n' && b != '\r') break;
    }
    return i;
}

fn fltParseStringSpan(s: string, pos: usize): FltSpan {
    if (pos >= s.len || s[pos] != '"') {
        return FltSpan{ ok: false, start: 0, end: 0, next: pos };
    }
    const content_start = pos + 1;
    let i = content_start;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\\') {
            i += 1;
            if (i >= s.len) return FltSpan{ ok: false, start: 0, end: 0, next: pos };
        } else if (s[i] == '"') {
            return FltSpan{ ok: true, start: content_start, end: i, next: i + 1 };
        }
    }
    return FltSpan{ ok: false, start: 0, end: 0, next: pos };
}

fn fltSkipValue(s: string, pos: usize): i64 {
    let i = fltSkipWs(s, pos);
    if (i >= s.len) return -1;
    const ch = s[i];
    if (ch == '"') {
        const sp = fltParseStringSpan(s, i);
        if (!sp.ok) return -1;
        return i64(sp.next);
    }
    if (ch == '{' || ch == '[') {
        let depth: usize = 0;
        let in_str = false;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            if (in_str) {
                if (c == '\\') {
                    i += 1;
                } else if (c == '"') {
                    in_str = false;
                }
            } else {
                if (c == '"') {
                    in_str = true;
                } else if (c == '{' || c == '[') {
                    depth += 1;
                } else if (c == '}' || c == ']') {
                    if (depth == 0) return -1;
                    depth -= 1;
                    if (depth == 0) return i64(i + 1);
                }
            }
        }
        return -1;
    }
    // Primitive token (null, true, false, number)
    const start = i;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == ',' || c == ']' || c == '}' || c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            break;
        }
    }
    if (i == start) return -1;
    return i64(i);
}

fn fltParseUsizeStr(s: string): i64 {
    if (s.len == 0) return -1;
    let val: i64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b < '0' || b > '9') return -1;
        val = val * 10 + i64(b - '0');
    }
    return val;
}

fn fltParseI64Token(s: string): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
        if (i >= s.len) return null;
    }
    let val: i64 = 0;
    while (i < s.len) : (i += 1) {
        const b = s[i];
        if (b < '0' || b > '9') return null;
        val = val * 10 + i64(b - '0');
    }
    if (neg) return -val;
    return val;
}

fn fltParseInlineValue(json: string, start: usize, end: usize, slot_to_node: []usize, slot_count: usize, arena: *FlatArena): i64 {
    const tok = json[start..end];
    if (tok.len == 0) return -1;
    if (tok[0] == '"') {
        const sp = fltParseStringSpan(json, start);
        if (!sp.ok || sp.next != end) return -1;
        const ref_idx = fltParseUsizeStr(json[sp.start..sp.end]);
        if (ref_idx < 0 || usize(ref_idx) >= slot_count) return -1;
        return i64(slot_to_node[usize(ref_idx)]);
    }
    if (fltStrEq(tok, "null")) {
        const id = arena.addNull();
        if (id >= FLAT_MAX_NODES) return -1;
        return i64(id);
    }
    if (fltStrEq(tok, "true")) {
        const id = arena.addBool(true);
        if (id >= FLAT_MAX_NODES) return -1;
        return i64(id);
    }
    if (fltStrEq(tok, "false")) {
        const id = arena.addBool(false);
        if (id >= FLAT_MAX_NODES) return -1;
        return i64(id);
    }
    if (iv := fltParseI64Token(tok)) {
        const id = arena.addInt(iv);
        if (id >= FLAT_MAX_NODES) return -1;
        return i64(id);
    }
    return -1;
}

/// Parse a `flatted` JSON array string into `out_arena`, restoring all object/array references and cycles.
/// Returns the root node ID on success, or `-1` on invalid input.
pub fn parseFlatted(json: string, out_arena: *FlatArena): i64 {
    out_arena.reset();
    let pos = fltSkipWs(json, 0);
    if (pos >= json.len || json[pos] != '[') return -1;
    pos += 1;
    pos = fltSkipWs(json, pos);
    if (pos >= json.len || json[pos] == ']') return -1;

    let slot_starts: [64]usize = undefined;
    let slot_ends: [64]usize = undefined;
    let slot_to_node: [64]usize = undefined;
    let slot_count: usize = 0;

    // Pass 1: locate each top-level slot span and allocate its node shell
    while (pos < json.len) {
        pos = fltSkipWs(json, pos);
        if (pos >= json.len) return -1;
        const elem_start = pos;
        const next_i = fltSkipValue(json, elem_start);
        if (next_i < 0) return -1;
        const elem_end = usize(next_i);

        if (slot_count >= FLAT_MAX_NODES) return -1;
        slot_starts[slot_count] = elem_start;
        slot_ends[slot_count] = elem_end;

        const ch = json[elem_start];
        let nid: usize = FLAT_MAX_NODES;
        if (ch == '{') {
            nid = out_arena.addObject();
        } else if (ch == '[') {
            nid = out_arena.addArray();
        } else if (ch == '"') {
            const sp = fltParseStringSpan(json, elem_start);
            if (!sp.ok || sp.next != elem_end) return -1;
            nid = out_arena.addStr(json[sp.start..sp.end]);
        } else {
            const tok = json[elem_start..elem_end];
            if (fltStrEq(tok, "null")) {
                nid = out_arena.addNull();
            } else if (fltStrEq(tok, "true")) {
                nid = out_arena.addBool(true);
            } else if (fltStrEq(tok, "false")) {
                nid = out_arena.addBool(false);
            } else if (iv := fltParseI64Token(tok)) {
                nid = out_arena.addInt(iv);
            } else {
                return -1;
            }
        }
        if (nid >= FLAT_MAX_NODES) return -1;
        slot_to_node[slot_count] = nid;
        slot_count += 1;

        pos = fltSkipWs(json, elem_end);
        if (pos >= json.len) return -1;
        if (json[pos] == ',') {
            pos += 1;
            continue;
        }
        if (json[pos] == ']') {
            pos += 1;
            break;
        }
        return -1;
    }

    pos = fltSkipWs(json, pos);
    if (pos != json.len || slot_count == 0) return -1;

    // Pass 2: populate Array and Object edges (resolving slot references & cycles)
    let s: usize = 0;
    while (s < slot_count) : (s += 1) {
        const nid = slot_to_node[s];
        const s_start = slot_starts[s];
        const s_end = slot_ends[s];
        const ch = json[s_start];

        if (ch == '[') {
            let p = fltSkipWs(json, s_start + 1);
            if (p < s_end && json[p] == ']') continue;
            while (p < s_end) {
                p = fltSkipWs(json, p);
                const vs = p;
                const vn = fltSkipValue(json, vs);
                if (vn < 0 || usize(vn) > s_end) return -1;
                const ve = usize(vn);
                const cid = fltParseInlineValue(json, vs, ve, slot_to_node[0..], slot_count, out_arena);
                if (cid < 0) return -1;
                if (!out_arena.pushElem(nid, usize(cid))) return -1;

                p = fltSkipWs(json, ve);
                if (p < s_end && json[p] == ',') {
                    p += 1;
                    continue;
                }
                if (p < s_end && json[p] == ']') break;
                return -1;
            }
        } else if (ch == '{') {
            let p = fltSkipWs(json, s_start + 1);
            if (p < s_end && json[p] == '}') continue;
            while (p < s_end) {
                p = fltSkipWs(json, p);
                const ksp = fltParseStringSpan(json, p);
                if (!ksp.ok || ksp.next >= s_end) return -1;
                const key = json[ksp.start..ksp.end];
                p = fltSkipWs(json, ksp.next);
                if (p >= s_end || json[p] != ':') return -1;
                p = fltSkipWs(json, p + 1);
                const vs = p;
                const vn = fltSkipValue(json, vs);
                if (vn < 0 || usize(vn) > s_end) return -1;
                const ve = usize(vn);
                const cid = fltParseInlineValue(json, vs, ve, slot_to_node[0..], slot_count, out_arena);
                if (cid < 0) return -1;
                if (!out_arena.setField(nid, key, usize(cid))) return -1;

                p = fltSkipWs(json, ve);
                if (p < s_end && json[p] == ',') {
                    p += 1;
                    continue;
                }
                if (p < s_end && json[p] == '}') break;
                return -1;
            }
        }
    }

    return i64(slot_to_node[0]);
}

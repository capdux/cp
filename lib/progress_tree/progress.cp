// Source: Absorbed from /data/dev/bun/src/bun_core/Progress.cp | Synced: 2026-11-21T13:33:00Z
// Pure-CP zero-allocation hierarchical ProgressTree with rate-limiting and
// deterministic status-line formatting (no OS terminal syscall dependency).

pub enum ProgressUnit: u8 {
    none = 0,
    files = 1,
    bytes = 2,
}

pub const INVALID_NODE: u16 = 0xFFFF;

pub struct ProgressNode {
    name: []const u8,
    unit: ProgressUnit,
    completed: usize,
    estimated_total: usize,
    parent: u16,
    recently_updated_child: u16,
    active: bool,
}

/// Zero-allocation hierarchical progress tracker (up to 32 active nodes).
pub struct ProgressTree {
    nodes: [32]ProgressNode,
    len: u16,
    root_idx: u16,
    done: bool,
    initial_delay_ns: u64,
    refresh_rate_ns: u64,
    start_ns: u64,
    last_refresh_ns: u64,
    has_refreshed: bool,

    pub static fn init(start_ns: u64): ProgressTree {
        return ProgressTree{
            nodes: [ProgressNode{
                name: "",
                unit: .none,
                completed: 0,
                estimated_total: 0,
                parent: INVALID_NODE,
                recently_updated_child: INVALID_NODE,
                active: false,
            }; 32],
            len: 0,
            root_idx: INVALID_NODE,
            done: true,
            initial_delay_ns: 500_000_000, // 500ms default
            refresh_rate_ns: 50_000_000,   // 50ms default
            start_ns: start_ns,
            last_refresh_ns: start_ns,
            has_refreshed: false,
        };
    }

    /// Start the root progress node. Returns root node handle (0), or null if full.
    pub fn startRoot(*_, name: []const u8, estimated_total: usize): u16? {
        _.len = 0;
        _.done = false;
        _.has_refreshed = false;
        _.nodes[0] = ProgressNode{
            name: name,
            unit: .none,
            completed: 0,
            estimated_total: estimated_total,
            parent: INVALID_NODE,
            recently_updated_child: INVALID_NODE,
            active: true,
        };
        _.len = 1;
        _.root_idx = 0;
        return 0;
    }

    /// Start a child progress node under `parent_idx`. Returns child handle, or null if full/invalid.
    pub fn startChild(*_, parent_idx: u16, name: []const u8, estimated_total: usize, unit: ProgressUnit): u16? {
        if (parent_idx >= _.len || !_.nodes[parent_idx].active) return null;
        if (_.len >= 32) return null;
        const idx = _.len;
        _.nodes[idx] = ProgressNode{
            name: name,
            unit: unit,
            completed: 0,
            estimated_total: estimated_total,
            parent: parent_idx,
            recently_updated_child: INVALID_NODE,
            active: true,
        };
        _.len += 1;
        _.nodes[parent_idx].recently_updated_child = idx;
        return idx;
    }

    /// Increment completed count by `delta` and mark node as recently active on its parent.
    pub fn addCompleted(*_, node_idx: u16, delta: usize) {
        if (node_idx >= _.len || !_.nodes[node_idx].active) return;
        _.nodes[node_idx].completed += delta;
        const p = _.nodes[node_idx].parent;
        if (p != INVALID_NODE && p < _.len) {
            _.nodes[p].recently_updated_child = node_idx;
        }
    }

    pub fn completeOne(*_, node_idx: u16) {
        _.addCompleted(node_idx, 1);
    }

    pub fn setName(*_, node_idx: u16, name: []const u8) {
        if (node_idx >= _.len) return;
        _.nodes[node_idx].name = name;
        const p = _.nodes[node_idx].parent;
        if (p != INVALID_NODE && p < _.len) {
            _.nodes[p].recently_updated_child = node_idx;
        }
    }

    pub fn setEstimatedTotal(*_, node_idx: u16, total: usize) {
        if (node_idx >= _.len) return;
        _.nodes[node_idx].estimated_total = total;
    }

    /// Finish a node: if child, clears parent's active child pointer and increments parent's completed count.
    /// If root, marks the entire tree done.
    pub fn end(*_, node_idx: u16) {
        if (node_idx >= _.len || !_.nodes[node_idx].active) return;
        _.nodes[node_idx].active = false;
        const p = _.nodes[node_idx].parent;
        if (p != INVALID_NODE && p < _.len) {
            if (_.nodes[p].recently_updated_child == node_idx) {
                _.nodes[p].recently_updated_child = INVALID_NODE;
            }
            _.completeOne(p);
        } else if (node_idx == _.root_idx) {
            _.done = true;
        }
    }

    /// Return whether a UI refresh should happen at `now_ns` according to `initial_delay_ns` and `refresh_rate_ns`.
    pub fn shouldRefresh(*_, now_ns: u64): bool {
        if (_.done) return false;
        if (now_ns < _.start_ns) return false;
        if (!_.has_refreshed) {
            if (now_ns - _.start_ns >= _.initial_delay_ns) {
                _.has_refreshed = true;
                _.last_refresh_ns = now_ns;
                return true;
            }
            return false;
        }
        if (now_ns >= _.last_refresh_ns && (now_ns - _.last_refresh_ns) >= _.refresh_rate_ns) {
            _.last_refresh_ns = now_ns;
            return true;
        }
        return false;
    }

    /// Percentage [0..100] for a given node (returns null if estimated_total == 0).
    pub fn percent(node_idx: u16): u8? {
        if (node_idx >= _.len) return null;
        const n = _.nodes[node_idx];
        if (n.estimated_total == 0) return null;
        if (n.completed >= n.estimated_total) return 100;
        const pct: usize = (n.completed * 100) / n.estimated_total;
        return @intCast(pct);
    }

    /// Render the active root -> recently_updated_child chain into `out`.
    /// Example: `[1/3] Installing > Downloading (512KB/1MB)`
    pub fn formatLine(out: []u8): []const u8 {
        if (_.root_idx == INVALID_NODE || _.root_idx >= _.len) return out[0..0];
        let pos: usize = 0;
        let cur = _.root_idx;
        let depth: usize = 0;
        while (cur != INVALID_NODE && cur < _.len && depth < 32) : (depth += 1) {
            const n = _.nodes[cur];
            if (depth > 0) {
                pos = appendStr(out, pos, " > ");
            }
            if (n.estimated_total > 0 && depth == 0) {
                pos = appendStr(out, pos, "[");
                pos = appendCount(out, pos, n.completed, n.unit);
                pos = appendStr(out, pos, "/");
                pos = appendCount(out, pos, n.estimated_total, n.unit);
                pos = appendStr(out, pos, "] ");
            }
            pos = appendStr(out, pos, n.name);
            if (depth > 0 && (n.completed > 0 || n.estimated_total > 0)) {
                pos = appendStr(out, pos, " (");
                pos = appendCount(out, pos, n.completed, n.unit);
                if (n.estimated_total > 0) {
                    pos = appendStr(out, pos, "/");
                    pos = appendCount(out, pos, n.estimated_total, n.unit);
                }
                if (n.unit == .files) {
                    pos = appendStr(out, pos, " files");
                }
                pos = appendStr(out, pos, ")");
            }
            cur = n.recently_updated_child;
        }
        return out[0..pos];
    }
}

fn appendStr(out: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn appendUsize(out: []u8, pos: usize, v_in: usize): usize {
    if (v_in == 0) return appendStr(out, pos, "0");
    let buf: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = v_in;
    while (v > 0) : (v /= 10) {
        buf[n] = @intCast('0' + (v % 10));
        n += 1;
    }
    let p = pos;
    while (n > 0 && p < out.len) {
        n -= 1;
        out[p] = buf[n];
        p += 1;
    }
    return p;
}

fn appendCount(out: []u8, pos: usize, val: usize, unit: ProgressUnit): usize {
    if (unit != .bytes) return appendUsize(out, pos, val);
    if (val >= 1024 * 1024 * 1024) {
        const p = appendUsize(out, pos, val / (1024 * 1024 * 1024));
        return appendStr(out, p, "GB");
    }
    if (val >= 1024 * 1024) {
        const p = appendUsize(out, pos, val / (1024 * 1024));
        return appendStr(out, p, "MB");
    }
    if (val >= 1024) {
        const p = appendUsize(out, pos, val / 1024);
        return appendStr(out, p, "KB");
    }
    const p = appendUsize(out, pos, val);
    return appendStr(out, p, "B");
}

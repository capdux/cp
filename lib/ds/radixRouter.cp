// Source: cp/lib/radix/router.cp (Pure CP Segment-Aware Radix Path Router)
// Zero-C-dependency path router supporting:
//   - Static segments (e.g. `/users/profile`)
//   - Named parameter segments `:param` (e.g. `/users/:id/posts`)
//   - Catch-all wildcard segments `*wildcard` (e.g. `/static/*filepath`)
//   - Priority resolution: Static > :param > *wildcard

const std = @import("std");
import { RadixError } from "./radix.cp";

/// Captured route parameter (`name` -> `value`).
/// Both slices borrow from the route pattern and matched path respectively.
pub struct RouteParam {
    pub name: []const u8,
    pub value: []const u8,
}

/// Result of `RadixRouter.matchRoute()`.
/// Call `deinit()` to release the `params` slice.
pub struct RouteMatch<T> {
    alloc: std.mem.Allocator,
    pub value: T,
    pub params: []RouteParam,

    /// Release the allocated `params` slice.
    pub fn deinit(*_) {
        if (_.params.len > 0) {
            _.alloc.free(_.params);
            _.params = [];
        }
    }

    /// Look up a captured parameter by name.
    pub fn getParam(name: []const u8): ([]const u8)? {
        for (p of _.params) {
            if (std.mem.eql(u8, p.name, name)) return p.value;
        }
        return null;
    }
}

struct RouterNode<T> {
    segment: []u8,
    param_child: (*RouterNode<T>)? = null,
    param_name: []u8 = [],
    wildcard_child: (*RouterNode<T>)? = null,
    wildcard_name: []u8 = [],
    static_children: std.ArrayList(*@This()) = .empty,
    value: T? = null,
    is_terminal: bool = false,
}

fn dupeSeg(alloc: std.mem.Allocator, s: []const u8): []u8 ! RadixError {
    if (s.len == 0) return [];
    return alloc.dupe(u8, s) catch return RadixError.OutOfMemory;
}

fn freeSeg(alloc: std.mem.Allocator, s: []u8) {
    if (s.len > 0) alloc.free(s);
}

/// Segment-aware Radix Router supporting static, `:param`, and `*wildcard` routes.
pub struct RadixRouter<T> {
    const NodeType = RouterNode(T);
    alloc: std.mem.Allocator,
    root: (*RouterNode<T>)? = null,
    len: usize = 0,

    /// Initialize an empty RadixRouter.
    pub static fn init(allocator: std.mem.Allocator): RadixRouter<T> {
        return RadixRouter<T>{
            alloc: allocator,
            root: null,
            len: 0,
        };
    }

    /// Release all router nodes and stored segment strings.
    pub fn deinit(*_) {
        if (r := _.root) {
            _.destroyNode(r);
            _.root = null;
        }
        _.len = 0;
    }

    /// Remove all registered routes.
    pub fn clear(*_) {
        _.deinit();
    }

    /// Return the number of registered routes.
    pub fn count(): usize {
        return _.len;
    }

    /// Return true if no routes are registered.
    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    /// Register a route pattern (e.g. `/users/:id/posts`, `/static/*filepath`) with `value`.
    /// Returns `RadixError.DuplicateKey` if an identical route pattern is already registered.
    pub fn addRoute(*_, pattern: []const u8, value: T): void ! RadixError {
        let curr = _.ensureRoot()?;

        let it = std.mem.tokenizeScalar(u8, pattern, '/');
        while (seg := it.next()) {
            if (seg[0] == '*') {
                const wname = seg[1..];
                if (wc := curr.wildcard_child) {
                    if (wc.is_terminal) return RadixError.DuplicateKey;
                    wc.value = value;
                    wc.is_terminal = true;
                    _.len += 1;
                    return;
                }
                const wn_copy = dupeSeg(_.alloc, wname)?;
                errdefer freeSeg(_.alloc, wn_copy);
                const wc = _.createNode([])?;
                wc.wildcard_name = wn_copy;
                wc.value = value;
                wc.is_terminal = true;
                curr.wildcard_child = wc;
                _.len += 1;
                return;
            } else if (seg[0] == ':') {
                const pname = seg[1..];
                if (pc := curr.param_child) {
                    curr = pc;
                } else {
                    const pn_copy = dupeSeg(_.alloc, pname)?;
                    errdefer freeSeg(_.alloc, pn_copy);
                    const pc = _.createNode([])?;
                    pc.param_name = pn_copy;
                    curr.param_child = pc;
                    curr = curr.param_child.?;
                }
            } else {
                let found_idx: usize? = null;
                let si: usize = 0;
                while (si < curr.static_children.items.len) : (si += 1) {
                    if (std.mem.eql(u8, curr.static_children.items[si].segment, seg)) {
                        found_idx = si;
                        break;
                    }
                }
                if (idx := found_idx) {
                    curr = curr.static_children.items[idx];
                } else {
                    const seg_copy = dupeSeg(_.alloc, seg)?;
                    errdefer freeSeg(_.alloc, seg_copy);
                    const sc = _.createNode(seg_copy)?;
                    curr.static_children.append(_.alloc, sc) catch {
                        _.destroyNode(sc);
                        return RadixError.OutOfMemory;
                    };
                    curr = curr.static_children.items[curr.static_children.items.len - 1];
                }
            }
        }

        if (curr.is_terminal) return RadixError.DuplicateKey;
        curr.value = value;
        curr.is_terminal = true;
        _.len += 1;
    }

    /// Match a request `path` against registered routes with priority: static > `:param` > `*wildcard`.
    /// Returns `null` if no route matches. Caller must call `match.deinit()` when non-null.
    pub fn matchRoute(allocator: std.mem.Allocator, path: []const u8): RouteMatch<T>? ! RadixError {
        if (_.root == null) return null;
        const r = _.root.?;

        // Collect path segments and their byte start offsets (for wildcard slice extraction)
        let segs: std.ArrayList(string) = .empty;
        defer segs.deinit(allocator);
        let starts: std.ArrayList(usize) = .empty;
        defer starts.deinit(allocator);

        let i: usize = 0;
        while (i < path.len) {
            while (i < path.len && path[i] == '/') : (i += 1) {}
            if (i >= path.len) break;
            const start = i;
            while (i < path.len && path[i] != '/') : (i += 1) {}
            segs.append(allocator, path[start..i]) catch return RadixError.OutOfMemory;
            starts.append(allocator, start) catch return RadixError.OutOfMemory;
        }

        let params: std.ArrayList(RouteParam) = .empty;
        errdefer params.deinit(allocator);

        const matched_val = matchRecursive(allocator, r, path, segs.items, starts.items, 0, &params)?;
        if (val := matched_val) {
            const owned_params = params.toOwnedSlice(allocator) catch return RadixError.OutOfMemory;
            return RouteMatch<T>{
                alloc: allocator,
                value: val,
                params: owned_params,
            };
        }
        params.deinit(allocator);
        return null;
    }

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    fn createNode(*_, segment: []u8): (*RouterNode<T>) ! RadixError {
        let node = _.alloc.create(@This().NodeType) catch return RadixError.OutOfMemory;
        node.* = RouterNode<T>{
            segment: segment,
            param_child: null,
            param_name: [],
            wildcard_child: null,
            wildcard_name: [],
            static_children: .empty,
            value: null,
            is_terminal: false,
        };
        return node;
    }

    fn ensureRoot(*_): (*RouterNode<T>) ! RadixError {
        if (r := _.root) return r;
        _.root = _.createNode([])?;
        return _.root.?;
    }

    fn destroyNode(*_, node: *RouterNode<T>) {
        for (sc of node.static_children.items) {
            _.destroyNode(sc);
        }
        node.static_children.deinit(_.alloc);
        if (pc := node.param_child) _.destroyNode(pc);
        if (wc := node.wildcard_child) _.destroyNode(wc);
        freeSeg(_.alloc, node.segment);
        freeSeg(_.alloc, node.param_name);
        freeSeg(_.alloc, node.wildcard_name);
        _.alloc.destroy(node);
    }

    fn matchRecursive(
        alloc: std.mem.Allocator,
        node: *RouterNode<T>,
        full_path: []const u8,
        segs: []const []const u8,
        starts: []const usize,
        depth: usize,
        params: *std.ArrayList(RouteParam),
    ): T? ! RadixError {
        if (depth == segs.len) {
            if (node.is_terminal) return node.value;
            // Also allow `*wildcard` to match empty remainder if registered at this level
            if (wc := node.wildcard_child) {
                if (wc.is_terminal) {
                    params.append(alloc, RouteParam{
                        name: wc.wildcard_name,
                        value: "",
                    }) catch return RadixError.OutOfMemory;
                    return wc.value;
                }
            }
            return null;
        }

        const seg = segs[depth];

        // 1. Static children (highest priority)
        for (sc of node.static_children.items) {
            if (std.mem.eql(u8, sc.segment, seg)) {
                const res = matchRecursive(alloc, sc, full_path, segs, starts, depth + 1, params)?;
                if (res != null) return res;
            }
        }

        // 2. `:param` child (medium priority)
        if (pc := node.param_child) {
            const saved_len = params.items.len;
            params.append(alloc, RouteParam{
                name: pc.param_name,
                value: seg,
            }) catch return RadixError.OutOfMemory;
            const res = matchRecursive(alloc, pc, full_path, segs, starts, depth + 1, params)?;
            if (res != null) return res;
            params.items.len = saved_len;
        }

        // 3. `*wildcard` child (lowest priority, captures rest of path)
        if (wc := node.wildcard_child) {
            if (wc.is_terminal) {
                const rest_start = starts[depth];
                params.append(alloc, RouteParam{
                    name: wc.wildcard_name,
                    value: full_path[rest_start..],
                }) catch return RadixError.OutOfMemory;
                return wc.value;
            }
        }

        return null;
    }
}

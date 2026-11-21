// Source: Absorbed from /data/dev/bun/src/router/Router.cp | Synced: 2026-11-20T05:04:52Z
// lib/router/radix.cp
// High-performance 5-tier Radix-Tree / Trie router for CP Web applications
import { ParamMap } from "./param_map";
import { RouteMatch } from "./route";

const std = @import("std");

pub enum TrailingSlashMode: u8 {
    Ignore = 0,   // /users/ and /users match equivalently (default)
    Strict = 1,   // /users/ != /users
    Canonical = 2, // Normalize trailing slash on insert & lookup
}

pub struct RouterOptions {
    pub trailing_slash: TrailingSlashMode = TrailingSlashMode.Ignore,
    pub detect_conflicts: bool = true,
}

pub struct HybridSegment {
    pub prefix: []const u8 = "",
    pub param_name: []const u8 = "",
    pub suffix: []const u8 = "",

    /// Parses segments like `prefix-:id`, `:name.json`, `file-[id].tar.gz`, `v:ver-beta`.
    /// Returns null if the segment is purely static, pure `:param`, pure `[param]`, or wildcard.
    pub static fn parse(seg: []const u8): HybridSegment? {
        if (seg.len == 0) return null;
        if (seg[0] == '*' || std.mem.startsWith(u8, seg, "[...")) return null;
        if (std.mem.startsWith(u8, seg, "[[...")) return null;

        // Case 1: `[param]` embedded with prefix and/or suffix (e.g. `img-[id].png`)
        if (open_br := std.mem.indexOfScalar(u8, seg, '[')) {
            if (close_br := std.mem.indexOfScalarPos(u8, seg, open_br + 1, ']')) {
                const prefix = seg[0..open_br];
                const pname = seg[open_br + 1 .. close_br];
                const suffix = seg[close_br + 1 ..];
                if (pname.len > 0 && (prefix.len > 0 || suffix.len > 0)) {
                    return HybridSegment{
                        prefix: prefix,
                        param_name: pname,
                        suffix: suffix,
                    };
                }
            }
            return null;
        }

        // Case 2: `:param` embedded with prefix and/or suffix (e.g. `user-:id`, `:file.json`, `v:ver-rc1`)
        if (colon := std.mem.indexOfScalar(u8, seg, ':')) {
            const prefix = seg[0..colon];
            // Param name spans alphanumeric / underscore characters after ':'
            let end: usize = colon + 1;
            while (end < seg.len) : (end += 1) {
                const ch = seg[end];
                const is_ident = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_';
                if (!is_ident) break;
            }
            const pname = seg[colon + 1 .. end];
            const suffix = seg[end..];
            if (pname.len > 0 && (prefix.len > 0 || suffix.len > 0)) {
                return HybridSegment{
                    prefix: prefix,
                    param_name: pname,
                    suffix: suffix,
                };
            }
        }
        return null;
    }

    pub fn matchSegment(seg: []const u8): ([]const u8)? {
        if (seg.len <= _.prefix.len + _.suffix.len) return null;
        if (_.prefix.len > 0 && !std.mem.startsWith(u8, seg, _.prefix)) return null;
        if (_.suffix.len > 0 && !std.mem.endsWith(u8, seg, _.suffix)) return null;
        return seg[_.prefix.len .. seg.len - _.suffix.len];
    }

    pub fn specificity(): usize {
        return _.prefix.len + _.suffix.len;
    }
}

pub fn RadixNode(comptime T: type): type {
    return struct {
        const Self = @This();

        segment: []const u8 = "",
        is_param: bool = false,
        is_wildcard: bool = false,
        is_optional_catch_all: bool = false,
        is_hybrid: bool = false,
        param_name: []const u8 = "",
        wildcard_name: []const u8 = "",
        hybrid: HybridSegment = HybridSegment{},
        is_terminal: bool = false,
        has_trailing_slash: bool = false,
        handler: T? = null,

        children: [16](*Self)? = undefined,
        children_len: usize = 0,
        hybrid_children: [16](*Self)? = undefined,
        hybrid_children_len: usize = 0,
        param_child: (*Self)? = null,
        wildcard_child: (*Self)? = null,
        optional_catch_all_child: (*Self)? = null,

        pub fn create(allocator: std.mem.Allocator, segment: []const u8) *Self {
            let node = allocator.create(Self) catch unreachable;
            node.segment = segment;
            node.is_param = false;
            node.is_wildcard = false;
            node.is_optional_catch_all = false;
            node.is_hybrid = false;
            node.param_name = "";
            node.wildcard_name = "";
            node.hybrid = HybridSegment{};
            node.is_terminal = false;
            node.has_trailing_slash = false;
            node.handler = null;
            node.children_len = 0;
            node.hybrid_children_len = 0;
            node.param_child = null;
            node.wildcard_child = null;
            node.optional_catch_all_child = null;
            let i: usize = 0;
            while (i < 16) : (i += 1) {
                node.children[i] = null;
                node.hybrid_children[i] = null;
            }
            return node;
        }

        pub fn findStaticChild(seg: []const u8): (*Self)? {
            let i: usize = 0;
            while (i < _.children_len) : (i += 1) {
                if (c := _.children[i]) {
                    if (std.mem.eql(u8, c.segment, seg)) {
                        return c;
                    }
                }
            }
            return null;
        }

        pub fn addStaticChild(*_, child: *Self) {
            if (_.children_len < 16) {
                _.children[_.children_len] = child;
                _.children_len += 1;
            }
        }

        pub fn findHybridChild(h: HybridSegment): (*Self)? {
            let i: usize = 0;
            while (i < _.hybrid_children_len) : (i += 1) {
                if (c := _.hybrid_children[i]) {
                    if (std.mem.eql(u8, c.hybrid.prefix, h.prefix) &&
                        std.mem.eql(u8, c.hybrid.suffix, h.suffix))
                    {
                        return c;
                    }
                }
            }
            return null;
        }

        pub fn addHybridChild(*_, child: *Self) {
            if (_.hybrid_children_len >= 16) return;
            // Insert sorted descending by (prefix.len + suffix.len) so more specific hybrid patterns match first
            const spec = child.hybrid.specificity();
            let pos: usize = _.hybrid_children_len;
            while (pos > 0) : (pos -= 1) {
                if (prev := _.hybrid_children[pos - 1]) {
                    if (prev.hybrid.specificity() >= spec) break;
                    _.hybrid_children[pos] = prev;
                }
            }
            _.hybrid_children[pos] = child;
            _.hybrid_children_len += 1;
        }
    };
}

pub fn Router(comptime T: type): type {
    return struct {
        const Self = @This();
        const Node = RadixNode(T);
        const Match = RouteMatch(T);

        trees: [8](*Node)? = undefined,
        allocator: std.mem.Allocator,
        options: RouterOptions = RouterOptions{},
        total_routes: usize = 0,

        pub fn init(allocator: std.mem.Allocator) Self {
            return Self.initWithOptions(allocator, RouterOptions{});
        }

        pub fn initWithOptions(allocator: std.mem.Allocator, options: RouterOptions) Self {
            let r = Self{
                trees: undefined,
                allocator: allocator,
                options: options,
                total_routes: 0,
            };
            let i: usize = 0;
            while (i < 8) : (i += 1) {
                r.trees[i] = Node.create(allocator, "");
            }
            return r;
        }

        pub fn methodIndex(method: []const u8) usize {
            if (std.ascii.eqlIgnoreCase(method, "GET")) return 0;
            if (std.ascii.eqlIgnoreCase(method, "POST")) return 1;
            if (std.ascii.eqlIgnoreCase(method, "PUT")) return 2;
            if (std.ascii.eqlIgnoreCase(method, "DELETE")) return 3;
            if (std.ascii.eqlIgnoreCase(method, "PATCH")) return 4;
            if (std.ascii.eqlIgnoreCase(method, "HEAD")) return 5;
            if (std.ascii.eqlIgnoreCase(method, "OPTIONS")) return 6;
            return 7; // ALL
        }

        pub fn routeCount(): usize {
            return _.total_routes;
        }

        fn normalizeTrimPath(path: []const u8, mode: TrailingSlashMode): []const u8 {
            let p = path;
            if (p.len > 0 && p[0] == '/') p = p[1..];
            if (mode != TrailingSlashMode.Strict) {
                while (p.len > 0 && p[p.len - 1] == '/') {
                    p = p[0 .. p.len - 1];
                }
            }
            return p;
        }

        fn splitSegments(p: []const u8, out: [][]const u8): usize {
            if (p.len == 0) return 0;
            let count: usize = 0;
            let start: usize = 0;
            let i: usize = 0;
            while (i <= p.len) : (i += 1) {
                if (i == p.len || p[i] == '/') {
                    if (i > start) {
                        if (count < out.len) {
                            out[count] = p[start..i];
                            count += 1;
                        }
                    } else if (i == p.len && i > 0 && p[i - 1] == '/') {
                        // Trailing empty segment preserved only when Strict mode leaves trailing '/'
                        if (count < out.len) {
                            out[count] = "";
                            count += 1;
                        }
                    }
                    start = i + 1;
                }
            }
            return count;
        }

        /// Checked route insertion returning errors on duplicate routes, param name conflicts, or misplaced catch-alls.
        pub fn tryInsert(*_, method: []const u8, path: []const u8, handler: T): !void {
            const m_idx = methodIndex(method);
            if (_.trees[m_idx] == null) {
                _.trees[m_idx] = Node.create(_.allocator, "");
            }
            let curr = _.trees[m_idx].?;

            const p = normalizeTrimPath(path, _.options.trailing_slash);
            if (p.len == 0) {
                if (_.options.detect_conflicts && curr.is_terminal) {
                    return error.DuplicateRoute;
                }
                curr.is_terminal = true;
                curr.handler = handler;
                _.total_routes += 1;
                return;
            }

            let segs: [32][]const u8 = undefined;
            const seg_count = splitSegments(p, &segs);
            let idx: usize = 0;
            while (idx < seg_count) : (idx += 1) {
                const seg = segs[idx];
                const is_last = (idx + 1 == seg_count);

                // Check catch-all placement rule
                const is_opt_catch = std.mem.startsWith(u8, seg, "[[...") && std.mem.endsWith(u8, seg, "]]") && seg.len > 7;
                const is_catch = seg.len > 0 && (seg[0] == '*' || std.mem.eql(u8, seg, "**") ||
                    (std.mem.startsWith(u8, seg, "[...") && std.mem.endsWith(u8, seg, "]") && seg.len > 5));

                if ((is_opt_catch || is_catch) && !is_last) {
                    return error.CatchAllMustBeAtEnd;
                }

                curr = _.insertSegmentChecked(curr, seg)?;
            }

            if (_.options.detect_conflicts && curr.is_terminal) {
                return error.DuplicateRoute;
            }
            curr.is_terminal = true;
            curr.handler = handler;
            _.total_routes += 1;
        }

        pub fn insert(*_, method: []const u8, path: []const u8, handler: T) {
            const m_idx = methodIndex(method);
            if (_.trees[m_idx] == null) {
                _.trees[m_idx] = Node.create(_.allocator, "");
            }
            let curr = _.trees[m_idx].?;

            const p = normalizeTrimPath(path, _.options.trailing_slash);
            if (p.len == 0) {
                if (!curr.is_terminal) _.total_routes += 1;
                curr.is_terminal = true;
                curr.handler = handler;
                return;
            }

            let segs: [32][]const u8 = undefined;
            const seg_count = splitSegments(p, &segs);
            let idx: usize = 0;
            while (idx < seg_count) : (idx += 1) {
                curr = _.insertSegment(curr, segs[idx]);
            }

            if (!curr.is_terminal) _.total_routes += 1;
            curr.is_terminal = true;
            curr.handler = handler;
        }

        fn insertSegmentChecked(*_, parent: *Node, seg: []const u8): (*Node)! {
            // 1. Optional Catch-All: [[...slug]]
            if (std.mem.startsWith(u8, seg, "[[...") && std.mem.endsWith(u8, seg, "]]")) {
                if (seg.len <= 7) return error.InvalidPattern;
                const name = seg[5 .. seg.len - 2];
                if (existing := parent.optional_catch_all_child) {
                    if (_.options.detect_conflicts && !std.mem.eql(u8, existing.wildcard_name, name)) {
                        return error.RouteConflict;
                    }
                    return existing;
                }
                let child = Node.create(_.allocator, seg);
                child.is_optional_catch_all = true;
                child.wildcard_name = name;
                parent.optional_catch_all_child = child;
                return child;
            }

            // 2. Catch-All / Wildcard: *, **, *name, [...name]
            if (seg.len > 0 && (seg[0] == '*' || (std.mem.startsWith(u8, seg, "[...") && std.mem.endsWith(u8, seg, "]")))) {
                const name = if (std.mem.eql(u8, seg, "**"))
                    "**"
                else if (seg[0] == '*')
                    (if (seg.len > 1) seg[1..] else "*")
                else
                    seg[4 .. seg.len - 1];
                if (name.len == 0) return error.InvalidPattern;
                if (existing := parent.wildcard_child) {
                    if (_.options.detect_conflicts && !std.mem.eql(u8, existing.wildcard_name, name)) {
                        return error.RouteConflict;
                    }
                    return existing;
                }
                let child = Node.create(_.allocator, seg);
                child.is_wildcard = true;
                child.wildcard_name = name;
                parent.wildcard_child = child;
                return child;
            }

            // 3. Hybrid Prefix/Suffix segment: e.g. `user-:id`, `:name.json`, `file-[id].cp`
            if (h := HybridSegment.parse(seg)) {
                if (existing := parent.findHybridChild(h)) {
                    if (_.options.detect_conflicts && !std.mem.eql(u8, existing.hybrid.param_name, h.param_name)) {
                        return error.RouteConflict;
                    }
                    return existing;
                }
                let child = Node.create(_.allocator, seg);
                child.is_hybrid = true;
                child.hybrid = h;
                child.param_name = h.param_name;
                parent.addHybridChild(child);
                return child;
            }

            // 4. Pure dynamic param: :id or [id]
            if (seg.len > 1 && (seg[0] == ':' || (seg[0] == '[' && seg[seg.len - 1] == ']'))) {
                const name = if (seg[0] == ':') seg[1..] else seg[1 .. seg.len - 1];
                if (name.len == 0) return error.InvalidPattern;
                if (existing := parent.param_child) {
                    if (_.options.detect_conflicts && !std.mem.eql(u8, existing.param_name, name)) {
                        return error.RouteConflict;
                    }
                    return existing;
                }
                let child = Node.create(_.allocator, seg);
                child.is_param = true;
                child.param_name = name;
                parent.param_child = child;
                return child;
            }

            // 5. Static segment
            if (existing := parent.findStaticChild(seg)) {
                return existing;
            }
            const child = Node.create(_.allocator, seg);
            parent.addStaticChild(child);
            return child;
        }

        fn insertSegment(*_, parent: *Node, seg: []const u8) *Node {
            return _.insertSegmentChecked(parent, seg) catch parent;
        }

        pub fn lookup(*_, method: []const u8, path: []const u8): Match? {
            const m_idx = methodIndex(method);
            if (root := _.trees[m_idx]) {
                if (res := _.lookupInTree(root, path)) {
                    return res;
                }
            }
            if (m_idx != 7) {
                if (all_root := _.trees[7]) {
                    return _.lookupInTree(all_root, path);
                }
            }
            return null;
        }

        pub fn hasRoute(*_, method: []const u8, path: []const u8): bool {
            return _.lookup(method, path) != null;
        }

        fn lookupInTree(*_, root: *Node, path: []const u8): Match? {
            let params = ParamMap.init();
            const p = normalizeTrimPath(path, _.options.trailing_slash);
            if (p.len == 0) {
                if (root.is_terminal && root.handler != null) {
                    return Match{ handler: root.handler.?, params: params };
                }
                if (opt_wc := root.optional_catch_all_child) {
                    if (opt_wc.is_terminal && opt_wc.handler != null) {
                        return Match{ handler: opt_wc.handler.?, params: params };
                    }
                }
                return null;
            }

            let segs: [32][]const u8 = undefined;
            const seg_count = splitSegments(p, &segs);
            return matchNode(root, segs[0..seg_count], p, &params);
        }

        fn joinRemaining(segs: [][]const u8, idx: usize, full_trimmed_path: []const u8): []const u8 {
            if (idx >= segs.len) return "";
            // Reconstruct remaining slice directly from full_trimmed_path offsets
            let offset: usize = 0;
            let k: usize = 0;
            while (k < idx) : (k += 1) {
                offset += segs[k].len;
                if (offset < full_trimmed_path.len && full_trimmed_path[offset] == '/') {
                    offset += 1;
                }
            }
            if (offset <= full_trimmed_path.len) {
                return full_trimmed_path[offset..];
            }
            return segs[idx];
        }

        fn matchNode(node: *Node, segs: [][]const u8, full_path: []const u8, params: *ParamMap): Match? {
            return matchNodeAt(node, segs, 0, full_path, params);
        }

        fn matchNodeAt(node: *Node, segs: [][]const u8, idx: usize, full_path: []const u8, params: *ParamMap): Match? {
            if (idx >= segs.len) {
                if (node.is_terminal && node.handler != null) {
                    return Match{ handler: node.handler.?, params: params.* };
                }
                if (opt_wc := node.optional_catch_all_child) {
                    if (opt_wc.is_terminal && opt_wc.handler != null) {
                        return Match{ handler: opt_wc.handler.?, params: params.* };
                    }
                }
                return null;
            }

            const seg = segs[idx];
            const saved_len = params.len;

            // Tier 1: Static segment (highest priority)
            if (sc := node.findStaticChild(seg)) {
                if (m := matchNodeAt(sc, segs, idx + 1, full_path, params)) {
                    return m;
                }
                params.len = saved_len;
            }

            // Tier 2: Hybrid prefix/suffix segment (`prefix-:param` / `:param.ext`)
            let hi: usize = 0;
            while (hi < node.hybrid_children_len) : (hi += 1) {
                if (hc := node.hybrid_children[hi]) {
                    if (captured := hc.hybrid.matchSegment(seg)) {
                        params.append(hc.hybrid.param_name, captured);
                        if (m := matchNodeAt(hc, segs, idx + 1, full_path, params)) {
                            return m;
                        }
                        params.len = saved_len;
                    }
                }
            }

            // Tier 3: Pure dynamic parameter segment (`:id` / `[id]`)
            if (seg.len > 0) {
                if (pc := node.param_child) {
                    params.append(pc.param_name, seg);
                    if (m := matchNodeAt(pc, segs, idx + 1, full_path, params)) {
                        return m;
                    }
                    params.len = saved_len;
                }
            }

            // Tier 4: Wildcard / Catch-All (`*path`, `**`, `[...path]`) — requires at least 1 segment
            if (seg.len > 0) {
                if (wc := node.wildcard_child) {
                    if (wc.is_terminal && wc.handler != null) {
                        const rem = joinRemaining(segs, idx, full_path);
                        params.append(wc.wildcard_name, rem);
                        return Match{ handler: wc.handler.?, params: params.* };
                    }
                }
            }

            // Tier 5: Optional Catch-All (`[[...path]]`)
            if (opt_wc := node.optional_catch_all_child) {
                if (opt_wc.is_terminal && opt_wc.handler != null) {
                    const rem = joinRemaining(segs, idx, full_path);
                    if (rem.len > 0) {
                        params.append(opt_wc.wildcard_name, rem);
                    }
                    return Match{ handler: opt_wc.handler.?, params: params.* };
                }
            }

            return null;
        }

        pub fn get(*_, path: []const u8, handler: T) {
            _.insert("GET", path, handler);
        }

        pub fn post(*_, path: []const u8, handler: T) {
            _.insert("POST", path, handler);
        }

        pub fn put(*_, path: []const u8, handler: T) {
            _.insert("PUT", path, handler);
        }

        pub fn delete(*_, path: []const u8, handler: T) {
            _.insert("DELETE", path, handler);
        }

        pub fn patch(*_, path: []const u8, handler: T) {
            _.insert("PATCH", path, handler);
        }

        pub fn head(*_, path: []const u8, handler: T) {
            _.insert("HEAD", path, handler);
        }

        pub fn optionsMethod(*_, path: []const u8, handler: T) {
            _.insert("OPTIONS", path, handler);
        }

        pub fn all(*_, path: []const u8, handler: T) {
            _.insert("ALL", path, handler);
        }
    };
}

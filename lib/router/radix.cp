// lib/router/radix.cp
// High-performance Radix-Tree / Trie router for CP Web applications
import { ParamMap } from "./param_map";
import { RouteMatch } from "./route";

pub fn RadixNode(comptime T: type): type {
    return struct {
        const Self = @This();

        segment: []const u8 = "",
        is_param: bool = false,
        is_wildcard: bool = false,
        param_name: []const u8 = "",
        wildcard_name: []const u8 = "",
        is_terminal: bool = false,
        handler: T? = null,

        children: [16](*Self)? = undefined,
        children_len: usize = 0,
        param_child: (*Self)? = null,
        wildcard_child: (*Self)? = null,

        pub fn create(allocator: std.mem.Allocator, segment: []const u8) *Self {
            const node = allocator.create(Self) catch unreachable;
            node.segment = segment;
            node.is_param = false;
            node.is_wildcard = false;
            node.param_name = "";
            node.wildcard_name = "";
            node.is_terminal = false;
            node.handler = null;
            node.children_len = 0;
            node.param_child = null;
            node.wildcard_child = null;
            let i: usize = 0;
            while (i < 16) : (i += 1) {
                node.children[i] = null;
            }
            return node;
        }

        pub fn findStaticChild(*_, seg: []const u8): (*Self)? {
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
    };
}

pub fn Router(comptime T: type): type {
    return struct {
        const Self = @This();
        const Node = RadixNode(T);
        const Match = RouteMatch(T);

        trees: [8](*Node)? = undefined,
        allocator: std.mem.Allocator,

        pub fn init(allocator: std.mem.Allocator) Self {
            let r = Self{
                .trees = undefined,
                .allocator = allocator,
            };
            let i: usize = 0;
            while (i < 8) : (i += 1) {
                r.trees[i] = Node.create(allocator, "");
            }
            return r;
        }

        pub fn methodIndex(method: []const u8) usize {
            if (std.mem.eql(u8, method, "GET")) return 0;
            if (std.mem.eql(u8, method, "POST")) return 1;
            if (std.mem.eql(u8, method, "PUT")) return 2;
            if (std.mem.eql(u8, method, "DELETE")) return 3;
            if (std.mem.eql(u8, method, "PATCH")) return 4;
            if (std.mem.eql(u8, method, "HEAD")) return 5;
            if (std.mem.eql(u8, method, "OPTIONS")) return 6;
            return 7; // ALL
        }

        pub fn insert(*_, method: []const u8, path: []const u8, handler: T) {
            const m_idx = methodIndex(method);
            if (_.trees[m_idx] == null) {
                _.trees[m_idx] = Node.create(_.allocator, "");
            }
            let curr = _.trees[m_idx].?;

            let p = path;
            if (p.len > 0 and p[0] == '/') p = p[1..];
            if (p.len > 0 and p[p.len - 1] == '/') p = p[0 .. p.len - 1];

            if (p.len == 0) {
                curr.is_terminal = true;
                curr.handler = handler;
                return;
            }

            let start: usize = 0;
            let i: usize = 0;
            while (i <= p.len) : (i += 1) {
                if (i == p.len or p[i] == '/') {
                    if (i > start) {
                        const seg = p[start..i];
                        curr = _.insertSegment(curr, seg);
                    }
                    start = i + 1;
                }
            }

            curr.is_terminal = true;
            curr.handler = handler;
        }

        fn insertSegment(*_, parent: *Node, seg: []const u8) *Node {
            // Dynamic param :id or [id]
            if (seg.len > 1 and (seg[0] == ':' or (seg[0] == '[' and seg[seg.len - 1] == ']' and !std.mem.startsWith(u8, seg, "[...")))) {
                const name = if (seg[0] == ':') seg[1..] else seg[1 .. seg.len - 1];
                if (existing := parent.param_child) {
                    return existing;
                }
                const child = Node.create(_.allocator, seg);
                child.is_param = true;
                child.param_name = name;
                parent.param_child = child;
                return child;
            }

            // Wildcard catch-all *path or [...path]
            if (seg.len > 0 and (seg[0] == '*' or (std.mem.startsWith(u8, seg, "[...") and seg[seg.len - 1] == ']'))) {
                const name = if (seg[0] == '*') (if (seg.len > 1) seg[1..] else "*") else seg[4 .. seg.len - 1];
                if (existing := parent.wildcard_child) {
                    return existing;
                }
                const child = Node.create(_.allocator, seg);
                child.is_wildcard = true;
                child.wildcard_name = name;
                parent.wildcard_child = child;
                return child;
            }

            // Static segment
            if (parent.findStaticChild(seg)) |existing| {
                return existing;
            }
            const child = Node.create(_.allocator, seg);
            parent.addStaticChild(child);
            return child;
        }

        pub fn lookup(*_, method: []const u8, path: []const u8): Match? {
            const m_idx = methodIndex(method);
            if (root := _.trees[m_idx]) {
                if (_.lookupInTree(root, path)) |res| {
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

        fn lookupInTree(*_, root: *Node, path: []const u8): Match? {
            * = _;
            let params = ParamMap.init();
            let p = path;
            if (p.len > 0 and p[0] == '/') p = p[1..];
            if (p.len > 0 and p[p.len - 1] == '/') p = p[0 .. p.len - 1];

            if (p.len == 0) {
                if (root.is_terminal and root.handler != null) {
                    return Match{ .handler = root.handler.?, .params = params };
                }
                return null;
            }

            let curr = root;
            let start: usize = 0;
            let i: usize = 0;

            while (i <= p.len) : (i += 1) {
                if (i == p.len or p[i] == '/') {
                    if (i > start) {
                        const seg = p[start..i];

                        if (curr.findStaticChild(seg)) |sc| {
                            curr = sc;
                        } else if (pc := curr.param_child) {
                            params.append(pc.param_name, seg);
                            curr = pc;
                        } else if (wc := curr.wildcard_child) {
                            const remaining = p[start..];
                            params.append(wc.wildcard_name, remaining);
                            if (wc.is_terminal and wc.handler != null) {
                                return Match{ .handler = wc.handler.?, .params = params };
                            }
                            return null;
                        } else {
                            return null;
                        }
                    }
                    start = i + 1;
                }
            }

            if (curr.is_terminal and curr.handler != null) {
                return Match{ .handler = curr.handler.?, .params = params };
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

        pub fn all(*_, path: []const u8, handler: T) {
            _.insert("ALL", path, handler);
        }
    };
}

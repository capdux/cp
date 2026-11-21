// Source: Absorbed from /data/dev/bun/src/router/router.cp | Synced: 2026-11-20T13:13:25Z
// lib/router/fs_router.cp
// Next.js / Bun-compatible FileSystemRouter for CP (cap:router)
// Supports [slug].cp, [...catchall].cp, [[...optional]].cp, (group) route groups, and priority sorting.

const std = @import("std");
import { ParamMap } from "./param_map";
import { GlobWalker } from "../glob/walker.cp";

pub const MAX_FS_ROUTES: usize = 256;
pub const MAX_FS_SEGMENTS: usize = 32;

pub enum FsRouteKind: u8 {
    Static = 0,
    Dynamic = 1,
    CatchAll = 2,
    OptionalCatchAll = 3,
}

pub enum FsSegmentKind: u8 {
    Static = 0,
    Dynamic = 1,
    CatchAll = 2,
    OptionalCatchAll = 3,
}

pub struct FsSegment {
    pub kind: FsSegmentKind = FsSegmentKind.Static,
    pub raw: []const u8 = "",
    pub param_name: []const u8 = "",
}

pub struct FsRouteEntry {
    pub file_path: []const u8 = "",
    pub route_pattern: []const u8 = "/",
    pub kind: FsRouteKind = FsRouteKind.Static,
    pub param_count: u16 = 0,
    pub static_count: u16 = 0,
    pub segments: [MAX_FS_SEGMENTS]FsSegment = undefined,
    pub segment_count: usize = 0,
    pub is_index: bool = false,
}

pub struct FsRouteMatch {
    pub file_path: []const u8 = "",
    pub route_pattern: []const u8 = "/",
    pub pathname: []const u8 = "/",
    pub kind: FsRouteKind = FsRouteKind.Static,
    pub params: ParamMap = ParamMap.init(),
}

pub struct FileSystemRouter {
    pub routes: [MAX_FS_ROUTES]FsRouteEntry = undefined,
    pub route_count: usize = 0,
    pub is_sorted: bool = true,
    pub allocator: std.mem.Allocator,

    pub static fn init(allocator: std.mem.Allocator): FileSystemRouter {
        let r = FileSystemRouter{
            routes: undefined,
            route_count: 0,
            is_sorted: true,
            allocator: allocator,
        };
        let i: usize = 0;
        while (i < MAX_FS_ROUTES) : (i += 1) {
            r.routes[i] = FsRouteEntry{};
        }
        return r;
    }

    fn isKnownExtension(ext: []const u8): bool {
        return std.mem.eql(u8, ext, ".cp") ||
            std.mem.eql(u8, ext, ".ts") ||
            std.mem.eql(u8, ext, ".tsx") ||
            std.mem.eql(u8, ext, ".js") ||
            std.mem.eql(u8, ext, ".jsx");
    }

    fn stripExtension(path: []const u8): []const u8 {
        if (dot := std.mem.lastIndexOfScalar(u8, path, '.')) {
            const slash = std.mem.lastIndexOfScalar(u8, path, '/');
            if (slash == null || dot > slash.?) {
                const ext = path[dot..];
                if (isKnownExtension(ext)) {
                    return path[0..dot];
                }
            }
        }
        return path;
    }

    fn isRouteGroup(seg: []const u8): bool {
        return seg.len >= 2 && seg[0] == '(' && seg[seg.len - 1] == ')';
    }

    fn parseSegment(seg: []const u8): FsSegment! {
        if (seg.len == 0) return error.InvalidRoutePattern;
        if (seg[0] == '[') {
            if (std.mem.startsWith(u8, seg, "[[...")) {
                if (!std.mem.endsWith(u8, seg, "]]") || seg.len <= 7) {
                    return error.InvalidOptionalCatchAllRoute;
                }
                const pname = seg[5 .. seg.len - 2];
                if (pname.len == 0) return error.MissingParamName;
                return FsSegment{
                    kind: FsSegmentKind.OptionalCatchAll,
                    raw: seg,
                    param_name: pname,
                };
            }
            if (std.mem.startsWith(u8, seg, "[...")) {
                if (!std.mem.endsWith(u8, seg, "]") || seg.len <= 5) {
                    return error.InvalidCatchAllRoute;
                }
                const pname = seg[4 .. seg.len - 1];
                if (pname.len == 0) return error.MissingParamName;
                return FsSegment{
                    kind: FsSegmentKind.CatchAll,
                    raw: seg,
                    param_name: pname,
                };
            }
            if (!std.mem.endsWith(u8, seg, "]")) {
                return error.PatternMissingClosingBracket;
            }
            if (seg.len <= 2) return error.MissingParamName;
            const pname = seg[1 .. seg.len - 1];
            return FsSegment{
                kind: FsSegmentKind.Dynamic,
                raw: seg,
                param_name: pname,
            };
        }
        return FsSegment{
            kind: FsSegmentKind.Static,
            raw: seg,
            param_name: "",
        };
    }

    fn parseFileEntry(allocator: std.mem.Allocator, file_path: []const u8): FsRouteEntry! {
        let p = file_path;
        while (p.len > 0 && p[0] == '/') p = p[1..];
        const no_ext = stripExtension(p);

        let entry = FsRouteEntry{
            file_path: allocator.dupe(u8, file_path) catch file_path,
            route_pattern: "/",
            kind: FsRouteKind.Static,
            param_count: 0,
            static_count: 0,
            segments: undefined,
            segment_count: 0,
            is_index: false,
        };
        let si: usize = 0;
        while (si < MAX_FS_SEGMENTS) : (si += 1) {
            entry.segments[si] = FsSegment{};
        }

        let iter = std.mem.splitScalar(u8, no_ext, '/');
        let raw_segs: [MAX_FS_SEGMENTS][]const u8 = undefined;
        let raw_count: usize = 0;
        while (s := iter.next()) {
            if (s.len == 0) continue;
            // Ignore Next.js / Bun route groups `(marketing)` in URL mapping
            if (isRouteGroup(s)) continue;
            if (raw_count >= MAX_FS_SEGMENTS) return error.TooManySegments;
            raw_segs[raw_count] = s;
            raw_count += 1;
        }

        // Strip trailing `index` segment
        if (raw_count > 0 && std.mem.eql(u8, raw_segs[raw_count - 1], "index")) {
            entry.is_index = true;
            raw_count -= 1;
        }

        let pat_buf: [512]u8 = undefined;
        let pat_pos: usize = 0;

        let idx: usize = 0;
        while (idx < raw_count) : (idx += 1) {
            const parsed = parseSegment(raw_segs[idx])?;
            const is_last = (idx + 1 == raw_count);
            if ((parsed.kind == FsSegmentKind.CatchAll || parsed.kind == FsSegmentKind.OptionalCatchAll) && !is_last) {
                return error.CatchAllMustBeAtTheEnd;
            }
            entry.segments[entry.segment_count] = FsSegment{
                kind: parsed.kind,
                raw: allocator.dupe(u8, parsed.raw) catch parsed.raw,
                param_name: allocator.dupe(u8, parsed.param_name) catch parsed.param_name,
            };
            entry.segment_count += 1;

            if (parsed.kind == FsSegmentKind.Static) {
                entry.static_count += 1;
            } else {
                entry.param_count += 1;
                if ((parsed.kind as u8) > (entry.kind as u8)) {
                    if (parsed.kind == FsSegmentKind.Dynamic) entry.kind = FsRouteKind.Dynamic;
                    if (parsed.kind == FsSegmentKind.CatchAll) entry.kind = FsRouteKind.CatchAll;
                    if (parsed.kind == FsSegmentKind.OptionalCatchAll) entry.kind = FsRouteKind.OptionalCatchAll;
                }
            }

            if (pat_pos + 1 + parsed.raw.len <= pat_buf.len) {
                pat_buf[pat_pos] = '/';
                pat_pos += 1;
                @memcpy(pat_buf[pat_pos .. pat_pos + parsed.raw.len], parsed.raw);
                pat_pos += parsed.raw.len;
            }
        }

        if (pat_pos == 0) {
            entry.route_pattern = "/";
        } else {
            entry.route_pattern = allocator.dupe(u8, pat_buf[0..pat_pos]) catch "/";
        }
        return entry;
    }

    pub fn addFile(*_, file_path: []const u8): !void {
        if (_.route_count >= MAX_FS_ROUTES) return error.RouteTableFull;
        const entry = parseFileEntry(_.allocator, file_path)?;

        // Check duplicate normalized route pattern (e.g. `(a)/about.cp` vs `(b)/about.cp`)
        let i: usize = 0;
        while (i < _.route_count) : (i += 1) {
            if (std.mem.eql(u8, _.routes[i].route_pattern, entry.route_pattern)) {
                return error.DuplicateRoute;
            }
        }

        _.routes[_.route_count] = entry;
        _.route_count += 1;
        _.is_sorted = false;
    }

    fn compareRoutes(a: FsRouteEntry, b: FsRouteEntry): bool {
        // Return true if `a` has strictly higher priority than `b`:
        // 1. Kind priority: Static (0) < Dynamic (1) < CatchAll (2) < OptionalCatchAll (3)
        const ak = a.kind as u8;
        const bk = b.kind as u8;
        if (ak != bk) return ak < bk;

        // 2. Compare segment-by-segment: Static segment beats Dynamic/CatchAll at earliest diverging position
        const min_segs = @min(a.segment_count, b.segment_count);
        let i: usize = 0;
        while (i < min_segs) : (i += 1) {
            const sa = a.segments[i].kind as u8;
            const sb = b.segments[i].kind as u8;
            if (sa != sb) return sa < sb;
        }

        // 3. More static segments wins
        if (a.static_count != b.static_count) return a.static_count > b.static_count;

        // 4. More total segments wins for specific routes
        if (a.segment_count != b.segment_count) return a.segment_count > b.segment_count;

        // 5. Lexicographical tie-break
        return std.mem.lessThan(u8, a.route_pattern, b.route_pattern);
    }

    pub fn sortRoutes(*_) {
        if (_.is_sorted || _.route_count <= 1) {
            _.is_sorted = true;
            return;
        }
        let i: usize = 1;
        while (i < _.route_count) : (i += 1) {
            const key = _.routes[i];
            let j: usize = i;
            while (j > 0 && compareRoutes(key, _.routes[j - 1])) : (j -= 1) {
                _.routes[j] = _.routes[j - 1];
            }
            _.routes[j] = key;
        }
        _.is_sorted = true;
    }

    fn matchEntry(entry: FsRouteEntry, req_segs: [][]const u8, trimmed_path: []const u8): ParamMap? {
        let params = ParamMap.init();
        if (entry.segment_count == 0) {
            if (req_segs.len == 0) return params;
            return null;
        }

        let r_idx: usize = 0;
        let e_idx: usize = 0;
        while (e_idx < entry.segment_count) : (e_idx += 1) {
            const seg = entry.segments[e_idx];
            switch (seg.kind) {
                .Static => {
                    if (r_idx >= req_segs.len) return null;
                    if (!std.mem.eql(u8, req_segs[r_idx], seg.raw)) return null;
                    r_idx += 1;
                },
                .Dynamic => {
                    if (r_idx >= req_segs.len || req_segs[r_idx].len == 0) return null;
                    params.append(seg.param_name, req_segs[r_idx]);
                    r_idx += 1;
                },
                .CatchAll => {
                    if (r_idx >= req_segs.len) return null;
                    // Capture remaining path slice
                    let offset: usize = 0;
                    let k: usize = 0;
                    while (k < r_idx) : (k += 1) {
                        offset += req_segs[k].len;
                        if (offset < trimmed_path.len && trimmed_path[offset] == '/') offset += 1;
                    }
                    params.append(seg.param_name, trimmed_path[offset..]);
                    return params;
                },
                .OptionalCatchAll => {
                    if (r_idx < req_segs.len) {
                        let offset: usize = 0;
                        let k: usize = 0;
                        while (k < r_idx) : (k += 1) {
                            offset += req_segs[k].len;
                            if (offset < trimmed_path.len && trimmed_path[offset] == '/') offset += 1;
                        }
                        params.append(seg.param_name, trimmed_path[offset..]);
                    }
                    return params;
                },
            }
        }

        if (r_idx != req_segs.len) return null;
        return params;
    }

    pub fn matchRoute(*_, pathname: []const u8): FsRouteMatch? {
        if (!_.is_sorted) _.sortRoutes();

        let p = pathname;
        if (q := std.mem.indexOfAny(u8, p, "?#")) {
            p = p[0..q];
        }
        while (p.len > 0 && p[0] == '/') p = p[1..];
        while (p.len > 0 && p[p.len - 1] == '/') p = p[0 .. p.len - 1];

        let req_segs: [MAX_FS_SEGMENTS][]const u8 = undefined;
        let req_count: usize = 0;
        if (p.len > 0) {
            let iter = std.mem.splitScalar(u8, p, '/');
            while (s := iter.next()) {
                if (s.len == 0) continue;
                if (req_count < MAX_FS_SEGMENTS) {
                    req_segs[req_count] = s;
                    req_count += 1;
                }
            }
        }

        let i: usize = 0;
        while (i < _.route_count) : (i += 1) {
            const entry = _.routes[i];
            if (params := matchEntry(entry, req_segs[0..req_count], p)) {
                return FsRouteMatch{
                    file_path: entry.file_path,
                    route_pattern: entry.route_pattern,
                    pathname: pathname,
                    kind: entry.kind,
                    params: params,
                };
            }
        }
        return null;
    }

    pub fn count(): usize {
        return _.route_count;
    }

    pub fn getRoute(idx: usize): FsRouteEntry? {
        if (idx >= _.route_count) return null;
        return _.routes[idx];
    }

    /// Scans `dir_path` recursively on disk and registers all matching route files, sorted by priority.
    pub fn scanDir(*_, dir_path: []const u8): !usize {
        const files = try GlobWalker.scan(_.allocator, "**/*", dir_path);
        let added: usize = 0;
        for (rel_path of files) {
            if (std.mem.indexOf(u8, rel_path, "node_modules") != null) continue;
            if (dot := std.mem.lastIndexOfScalar(u8, rel_path, '.')) {
                if (isKnownExtension(rel_path[dot..])) {
                    try _.addFile(rel_path);
                    added += 1;
                }
            }
        }
        _.sortRoutes();
        return added;
    }
}

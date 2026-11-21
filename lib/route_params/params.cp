// Source: Absorbed from /data/dev/bun/src/router/ParamMap.cp | Synced: 2026-11-21T12:47:31Z
//! HTTP route parameter map (`ParamMap` with `~init()` RAII) and
//! zero-allocation `URLPath` parser (extname, first segment, query string, `.map` source map detection).

import std from "std";

export struct Param {
    name: []const u8,
    value: []const u8,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    return eqlBytes(s[s.len - suffix.len ..], suffix);
}

/// Dynamic HTTP route parameter map with deterministic `~init()` RAII cleanup.
export struct ParamMap {
    allocator: std.mem.Allocator,
    items: std.ArrayList(Param),

    pub static fn init(allocator: std.mem.Allocator): ParamMap {
        return ParamMap{
            allocator: allocator,
            items: std.ArrayList(Param).empty,
        };
    }

    ~init() {
        _.items.deinit(_.allocator);
    }

    pub fn count(): usize {
        return _.items.items.len;
    }

    pub fn isEmpty(): bool {
        return _.items.items.len == 0;
    }

    pub fn append(*_, name: []const u8, value: []const u8): void! {
        _.items.append(_.allocator, Param{ name: name, value: value })?;
    }

    pub fn set(*_, name: []const u8, value: []const u8): void! {
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (eqlBytes(_.items.items[i].name, name)) {
                _.items.items[i].value = value;
                return;
            }
        }
        _.append(name, value)?;
    }

    pub fn find(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.items.items.len) : (i += 1) {
            if (eqlBytes(_.items.items[i].name, name)) {
                return _.items.items[i].value;
            }
        }
        return null;
    }

    pub fn has(name: []const u8): bool {
        return _.find(name) != null;
    }

    pub fn get(index: usize): Param? {
        if (index >= _.items.items.len) return null;
        return _.items.items[index];
    }

    pub fn clear(*_) {
        _.items.shrinkRetainingCapacity(0);
    }

    pub fn shrinkRetainingCapacity(*_, new_len: usize) {
        _.items.shrinkRetainingCapacity(new_len);
    }
}

/// Parsed URL path components (zero heap allocation, slices reference input).
export struct URLPath {
    extname: []const u8 = "",
    path: []const u8 = "",
    pathname: []const u8 = "",
    first_segment: []const u8 = "",
    query_string: []const u8 = "",
    is_source_map: bool = false,

    pub fn isRoot(asset_prefix: []const u8): bool {
        const without = _.pathWithoutAssetPrefix(asset_prefix);
        if (without.len == 1 && without[0] == '.') return true;
        return eqlBytes(without, "index");
    }

    pub fn pathWithoutAssetPrefix(asset_prefix: []const u8): []const u8 {
        if (asset_prefix.len == 0) return _.path;
        const lead: usize = if (asset_prefix[0] == '/') 1 else 0;
        const origin = asset_prefix[lead..];
        let out = _.path;
        if (_.path.len >= origin.len && eqlBytes(_.path[0..origin.len], origin)) {
            out = _.path[origin.len..];
        }
        if (_.is_source_map && endsWith(out, ".map")) {
            return out[0 .. out.len - 4];
        }
        return out;
    }
}

/// Parse a URL pathname (e.g. `"/assets/app.js.map?v=1"`) into structured `URLPath` components.
export fn parseURLPath(pathname: []const u8): URLPath {
    if (pathname.len == 0) {
        return URLPath{};
    }

    let question_idx: usize? = null;
    let period_idx: usize? = null;
    let first_seg_end: usize = pathname.len;
    let last_slash: usize? = null;

    let idx: usize = pathname.len;
    while (idx > 0) {
        idx -= 1;
        const c = pathname[idx];
        if (c == '?') {
            if (question_idx == null || idx > question_idx.?) {
                question_idx = idx;
            }
            if (period_idx != null && question_idx.? < period_idx.?) {
                period_idx = null;
            }
            if (last_slash != null && last_slash.? > question_idx.?) {
                last_slash = null;
            }
        } else if (c == '.') {
            if (period_idx == null || idx > period_idx.?) {
                period_idx = idx;
            }
        } else if (c == '/') {
            if (last_slash == null || idx > last_slash.?) {
                last_slash = idx;
            }
            if (idx > 0 && idx < first_seg_end) {
                first_seg_end = idx;
            }
        }
    }

    if (last_slash != null && period_idx != null && last_slash.? > period_idx.?) {
        period_idx = null;
    }

    const path_end = question_idx ?? pathname.len;
    const extname: []const u8 = if (period_idx != null)
        pathname[period_idx.? + 1 .. path_end]
    else
        "";

    const start_off: usize = if (pathname[0] == '/') 1 else 0;
    let path: []const u8 = if (path_end >= start_off) pathname[start_off..path_end] else "";
    const first_seg_limit = if (first_seg_end < path_end) first_seg_end else path_end;
    const first_segment: []const u8 = if (first_seg_limit >= start_off) pathname[start_off..first_seg_limit] else "";

    const is_source_map = eqlBytes(extname, "map");
    let final_ext = extname;
    if (is_source_map && path.len > 4) {
        const stem = path[0 .. path.len - 4];
        let dot_pos: usize? = null;
        let k = stem.len;
        while (k > 0) {
            k -= 1;
            if (stem[k] == '.') {
                dot_pos = k;
                break;
            }
        }
        if (j := dot_pos) {
            final_ext = stem[j + 1 ..];
            path = stem;
        }
    }

    const query_string: []const u8 = if (question_idx != null) pathname[question_idx.?..] else "";
    const final_path: []const u8 = if (pathname.len == 1 && pathname[0] == '/') "." else path;

    return URLPath{
        extname: final_ext,
        path: final_path,
        pathname: pathname,
        first_segment: first_segment,
        query_string: query_string,
        is_source_map: is_source_map,
    };
}

// @cap/path-to-regexp — Zero-allocation URL route pattern matcher & reverse URL builder for CP
// Clean-room MIT implementation inspired by npm `path-to-regexp` (65M/wk).

const MAX_PARAMS: usize = 16;

pub struct ParamPair {
    pub name: string,
    pub value: string,
}

pub struct RouteMatch {
    pub matched: bool,
    pub params: [MAX_PARAMS]ParamPair,
    pub param_count: usize,

    pub static fn miss() -> RouteMatch {
        let p: [MAX_PARAMS]ParamPair = undefined;
        return RouteMatch{
            matched: false,
            params: p,
            param_count: 0,
        };
    }

    pub static fn okEmpty() -> RouteMatch {
        let p: [MAX_PARAMS]ParamPair = undefined;
        return RouteMatch{
            matched: true,
            params: p,
            param_count: 0,
        };
    }

    pub fn pushParam(*_, name: string, value: string) {
        if (_.param_count < MAX_PARAMS) {
            _.params[_.param_count] = ParamPair{ name: name, value: value };
            _.param_count += 1;
        }
    }

    pub fn get(name: string) -> string? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (strEq(_.params[i].name, name)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn has(name: string) -> bool {
        return _.get(name) != null;
    }
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn nextSlash(s: string, start: usize) -> usize {
    let i = start;
    while (i < s.len && s[i] != '/') {
        i += 1;
    }
    return i;
}

fn stripQueryAndTrailingSlash(path: string) -> string {
    let end = path.len;
    let i: usize = 0;
    while (i < path.len) {
        if (path[i] == '?' || path[i] == '#') {
            end = i;
            break;
        }
        i += 1;
    }
    while (end > 1 && path[end - 1] == '/') {
        end -= 1;
    }
    return path[0..end];
}

fn stripTrailingSlashOnly(s: string) -> string {
    let end = s.len;
    while (end > 1 && s[end - 1] == '/') {
        end -= 1;
    }
    return s[0..end];
}

pub fn matchRoute(pattern_raw: string, path_raw: string) -> RouteMatch {
    const pattern = stripTrailingSlashOnly(pattern_raw);
    const path = stripQueryAndTrailingSlash(path_raw);

    let res = RouteMatch.okEmpty();
    let pi: usize = 0;
    let si: usize = 0;

    // Skip leading '/'
    if (pi < pattern.len && pattern[pi] == '/') {
        pi += 1;
    }
    if (si < path.len && path[si] == '/') {
        si += 1;
    }

    while (pi < pattern.len) {
        const p_end = nextSlash(pattern, pi);
        const p_seg = pattern[pi..p_end];

        // Wildcard catch-all: "*name" or "*"
        if (p_seg.len > 0 && p_seg[0] == '*') {
            const wname = if (p_seg.len > 1) { p_seg[1..p_seg.len] } else { "wildcard" };
            const rest = if (si <= path.len) { path[si..path.len] } else { "" };
            res.pushParam(wname, rest);
            return res;
        }

        // Optional named parameter: ":id?"
        if (p_seg.len >= 3 && p_seg[0] == ':' && p_seg[p_seg.len - 1] == '?') {
            const pname = p_seg[1..p_seg.len - 1];
            if (si < path.len) {
                const s_end = nextSlash(path, si);
                res.pushParam(pname, path[si..s_end]);
                si = if (s_end < path.len) { s_end + 1 } else { path.len };
            }
            pi = if (p_end < pattern.len) { p_end + 1 } else { pattern.len };
            continue;
        }

        // Path exhausted before required pattern segment
        if (si >= path.len) {
            return RouteMatch.miss();
        }
        const s_end = nextSlash(path, si);
        const s_seg = path[si..s_end];

        if (p_seg.len >= 2 && p_seg[0] == ':') {
            // Required named parameter ":id"
            if (s_seg.len == 0) {
                return RouteMatch.miss();
            }
            res.pushParam(p_seg[1..p_seg.len], s_seg);
        } else {
            // Static segment exact match
            if (!strEq(p_seg, s_seg)) {
                return RouteMatch.miss();
            }
        }

        pi = if (p_end < pattern.len) { p_end + 1 } else { pattern.len };
        si = if (s_end < path.len) { s_end + 1 } else { path.len };
    }

    if (si < path.len) {
        return RouteMatch.miss();
    }
    return res;
}

pub fn isRouteMatch(pattern: string, path: string) -> bool {
    return matchRoute(pattern, path).matched;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn findParam(params: []ParamPair, name: string) -> string? {
    let i: usize = 0;
    while (i < params.len) {
        if (strEq(params[i].name, name)) {
            return params[i].value;
        }
        i += 1;
    }
    return null;
}

pub fn buildRoute(pattern_raw: string, params: []ParamPair, out: []u8) -> string? {
    const pattern = stripTrailingSlashOnly(pattern_raw);
    if (strEq(pattern, "/")) {
        const n = appendStr(out, 0 as usize, "/");
        return out[0..n];
    }

    let pos: usize = 0;
    let pi: usize = 0;
    if (pi < pattern.len && pattern[pi] == '/') {
        pi += 1;
    }

    while (pi < pattern.len) {
        const p_end = nextSlash(pattern, pi);
        const p_seg = pattern[pi..p_end];

        if (p_seg.len >= 3 && p_seg[0] == ':' && p_seg[p_seg.len - 1] == '?') {
            const pname = p_seg[1..p_seg.len - 1];
            const val = findParam(params, pname);
            if (val != null) {
                pos = appendStr(out, pos, "/");
                pos = appendStr(out, pos, val ?? "");
            }
        } else if (p_seg.len >= 2 && (p_seg[0] == ':' || p_seg[0] == '*')) {
            const pname = p_seg[1..p_seg.len];
            const val = findParam(params, pname);
            if (val == null) {
                return null; // missing required param
            }
            pos = appendStr(out, pos, "/");
            pos = appendStr(out, pos, val ?? "");
        } else {
            pos = appendStr(out, pos, "/");
            pos = appendStr(out, pos, p_seg);
        }

        pi = if (p_end < pattern.len) { p_end + 1 } else { pattern.len };
    }

    if (pos == 0) {
        pos = appendStr(out, pos, "/");
    }
    return out[0..pos];
}

export {
    ParamPair,
    RouteMatch,
    matchRoute,
    isRouteMatch,
    buildRoute,
};

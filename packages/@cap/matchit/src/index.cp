// @cap/matchit — Zero-allocation high-priority URL router for CP
// Clean-room MIT implementation inspired by Rust `matchit` (595M downloads, used by axum).

const MAX_ROUTES: usize = 64;
const MAX_PARAMS: usize = 8;
const MAX_SEGS: usize = 16;

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

pub struct RouteParam {
    pub key: string,
    pub value: string,
}

pub struct RouteMatch {
    pub matched: bool,
    pub handler_id: u32,
    pub params: [MAX_PARAMS]RouteParam,
    pub param_count: usize,

    pub fn param(key: string) -> string? {
        let i: usize = 0;
        while (i < _.param_count) {
            if (strEq(_.params[i].key, key)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }
}

fn emptyParam() -> RouteParam {
    return RouteParam{ key: "", value: "" };
}

fn noMatch() -> RouteMatch {
    let params: [MAX_PARAMS]RouteParam = undefined;
    let i: usize = 0;
    while (i < MAX_PARAMS) {
        params[i] = emptyParam();
        i += 1;
    }
    return RouteMatch{
        matched: false,
        handler_id: 0 as u32,
        params: params,
        param_count: 0 as usize,
    };
}

enum SegKind {
    Static,
    Param,
    CatchAll,
}

struct RouteSeg {
    kind: SegKind,
    text: string,
}

struct RouteEntry {
    active: bool,
    handler_id: u32,
    segs: [MAX_SEGS]RouteSeg,
    seg_count: usize,
}

fn emptySeg() -> RouteSeg {
    return RouteSeg{ kind: SegKind.Static, text: "" };
}

fn emptyEntry() -> RouteEntry {
    let segs: [MAX_SEGS]RouteSeg = undefined;
    let i: usize = 0;
    while (i < MAX_SEGS) {
        segs[i] = emptySeg();
        i += 1;
    }
    return RouteEntry{
        active: false,
        handler_id: 0 as u32,
        segs: segs,
        seg_count: 0 as usize,
    };
}

fn segPriority(kind: SegKind) -> u8 {
    switch (kind) {
        SegKind.Static => { return 3 as u8; },
        SegKind.Param => { return 2 as u8; },
        SegKind.CatchAll => { return 1 as u8; },
    }
}

fn parseRoutePattern(pattern: string, handler_id: u32) -> RouteEntry? {
    if (pattern.len == 0 || pattern[0] != '/') {
        return null;
    }
    let entry = emptyEntry();
    entry.active = true;
    entry.handler_id = handler_id;

    // Root route "/"
    if (pattern.len == 1) {
        entry.seg_count = 0 as usize;
        return entry;
    }

    let i: usize = 1;
    let end = pattern.len;
    // Trim single trailing slash if present on non-root pattern
    if (end > 1 && pattern[end - 1] == '/') {
        end -= 1;
    }

    while (i < end) {
        if (entry.seg_count >= MAX_SEGS) {
            return null;
        }
        const segStart = i;
        while (i < end && pattern[i] != '/') {
            i += 1;
        }
        const raw = pattern[segStart..i];
        if (raw.len == 0) {
            return null;
        }

        if (raw[0] == ':') {
            if (raw.len < 2) {
                return null;
            }
            entry.segs[entry.seg_count] = RouteSeg{
                kind: SegKind.Param,
                text: raw[1..raw.len],
            };
        } else if (raw[0] == '*') {
            if (raw.len < 2 || i < end) {
                // Catch-all must have a name and be the final segment
                return null;
            }
            entry.segs[entry.seg_count] = RouteSeg{
                kind: SegKind.CatchAll,
                text: raw[1..raw.len],
            };
        } else if (raw[0] == '{') {
            if (raw.len < 3 || raw[raw.len - 1] != '}') {
                return null;
            }
            const inner = raw[1..(raw.len - 1)];
            if (inner[0] == '*') {
                if (inner.len < 2 || i < end) {
                    return null;
                }
                entry.segs[entry.seg_count] = RouteSeg{
                    kind: SegKind.CatchAll,
                    text: inner[1..inner.len],
                };
            } else {
                entry.segs[entry.seg_count] = RouteSeg{
                    kind: SegKind.Param,
                    text: inner,
                };
            }
        } else {
            entry.segs[entry.seg_count] = RouteSeg{
                kind: SegKind.Static,
                text: raw,
            };
        }
        entry.seg_count += 1;

        if (i < end && pattern[i] == '/') {
            i += 1;
        }
    }

    return entry;
}

fn routesConflict(a: RouteEntry, b: RouteEntry) -> bool {
    if (a.seg_count != b.seg_count) {
        return false;
    }
    let i: usize = 0;
    while (i < a.seg_count) {
        if (a.segs[i].kind != b.segs[i].kind) {
            return false;
        }
        if (a.segs[i].kind == SegKind.Static && !strEq(a.segs[i].text, b.segs[i].text)) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn routeBetterThan(cand: RouteEntry, best: RouteEntry) -> bool {
    const minLen = if (cand.seg_count < best.seg_count) { cand.seg_count } else { best.seg_count };
    let i: usize = 0;
    while (i < minLen) {
        const pc = segPriority(cand.segs[i].kind);
        const pb = segPriority(best.segs[i].kind);
        if (pc != pb) {
            return pc > pb;
        }
        i += 1;
    }
    return cand.seg_count > best.seg_count;
}

fn matchEntry(entry: RouteEntry, path: string) -> RouteMatch? {
    if (path.len == 0 || path[0] != '/') {
        return null;
    }
    let res = noMatch();
    res.handler_id = entry.handler_id;

    // Root path "/"
    if (path.len == 1) {
        if (entry.seg_count == 0) {
            res.matched = true;
            return res;
        }
        return null;
    }

    let end = path.len;
    if (end > 1 && path[end - 1] == '/') {
        end -= 1;
    }

    let pos: usize = 1;
    let segIdx: usize = 0;
    while (segIdx < entry.seg_count) {
        const seg = entry.segs[segIdx];
        if (seg.kind == SegKind.CatchAll) {
            if (pos > end) {
                return null;
            }
            const rest = path[pos..end];
            if (rest.len == 0) {
                return null;
            }
            if (res.param_count < MAX_PARAMS) {
                res.params[res.param_count] = RouteParam{ key: seg.text, value: rest };
                res.param_count += 1;
            }
            res.matched = true;
            return res;
        }

        if (pos >= end) {
            return null;
        }
        const segStart = pos;
        while (pos < end && path[pos] != '/') {
            pos += 1;
        }
        const part = path[segStart..pos];
        if (part.len == 0) {
            return null;
        }

        if (seg.kind == SegKind.Static) {
            if (!strEq(part, seg.text)) {
                return null;
            }
        } else {
            if (res.param_count < MAX_PARAMS) {
                res.params[res.param_count] = RouteParam{ key: seg.text, value: part };
                res.param_count += 1;
            }
        }

        segIdx += 1;
        if (pos < end && path[pos] == '/') {
            pos += 1;
        }
    }

    if (pos != end) {
        return null;
    }
    res.matched = true;
    return res;
}

pub struct Router {
    pub routes: [MAX_ROUTES]RouteEntry,
    pub count: usize,

    pub static fn init() -> Router {
        let arr: [MAX_ROUTES]RouteEntry = undefined;
        let i: usize = 0;
        while (i < MAX_ROUTES) {
            arr[i] = emptyEntry();
            i += 1;
        }
        return Router{
            routes: arr,
            count: 0 as usize,
        };
    }

    pub fn insert(*_, pattern: string, handler_id: u32) -> bool {
        if (_.count >= MAX_ROUTES) {
            return false;
        }
        const parsedOpt = parseRoutePattern(pattern, handler_id);
        if (parsedOpt == null) {
            return false;
        }
        const entry = parsedOpt ?? emptyEntry();
        let i: usize = 0;
        while (i < _.count) {
            if (routesConflict(_.routes[i], entry)) {
                return false;
            }
            i += 1;
        }
        _.routes[_.count] = entry;
        _.count += 1;
        return true;
    }

    pub fn at(path: string) -> RouteMatch {
        let hasBest = false;
        let bestIdx: usize = 0;
        let bestMatch = noMatch();

        let i: usize = 0;
        while (i < _.count) {
            const mOpt = matchEntry(_.routes[i], path);
            if (mOpt != null) {
                const m = mOpt ?? noMatch();
                if (!hasBest || routeBetterThan(_.routes[i], _.routes[bestIdx])) {
                    hasBest = true;
                    bestIdx = i;
                    bestMatch = m;
                }
            }
            i += 1;
        }
        return bestMatch;
    }

    pub fn routeCount() -> usize {
        return _.count;
    }
}

export {
    RouteParam,
    RouteMatch,
    Router,
};

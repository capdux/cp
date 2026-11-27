// @cap/path-to-regexp — Zero-heap Express/Router URL route template matching,
// parameter extraction, and reverse route compilation engine.
// Clean-room MIT CP implementation modeled on npm `path-to-regexp` v8.
// Reuses `eqAscii` from `@cap/unicase` for optional case-insensitive route matching.
// Accelerated with 16-lane @Vector(16, u8) SIMD equality/slash scanning and @memcpy bulk copy.

import { eqAscii } from "../../unicase/src/index.cp";

type PtrVec16 = @Vector(16, u8);

fn ptrStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    const len = a.len;
    while (i + 16 <= len) {
        const va: PtrVec16 = a[i .. i + 16][0..16].*;
        const vb: PtrVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) {
            return false;
        }
        i += 16;
    }
    while (i < len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn ptrSegEq(a: string, b: string, case_sensitive: bool): bool {
    if (case_sensitive) { return ptrStrEq(a, b); }
    return eqAscii(a, b);
}

// Strips leading and trailing '/' characters (except keeps empty when root "/").
fn ptrTrimSlashes(s: string): string {
    let start: usize = 0;
    const v_sl: PtrVec16 = @splat(47 as u8);
    while (start + 16 <= s.len) {
        const v: PtrVec16 = s[start .. start + 16][0..16].*;
        if (!@reduce(.And, v == v_sl)) {
            break;
        }
        start += 16;
    }
    while (start < s.len && s[start] == 47) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && s[end - 1] == 47) {
        end -= 1;
    }
    return s[start..end];
}

// Finds the end index of the next '/' delimited segment starting at `pos`.
fn ptrFindSegEnd(s: string, pos: usize): usize {
    let i: usize = pos;
    const len = s.len;
    const v_sl: PtrVec16 = @splat(47 as u8);
    while (i + 16 <= len) {
        const v: PtrVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_sl)) {
            let k: usize = 0;
            while (k < 16) {
                if (s[i + k] == 47) {
                    return i + k;
                }
                k += 1;
            }
        }
        i += 16;
    }
    while (i < len && s[i] != 47) {
        i += 1;
    }
    return i;
}

fn ptrSkipSlashes(s: string, pos: usize): usize {
    let i: usize = pos;
    const len = s.len;
    const v_sl: PtrVec16 = @splat(47 as u8);
    while (i + 16 <= len) {
        const v: PtrVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.And, v == v_sl)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] == 47) {
        i += 1;
    }
    return i;
}

fn ptrWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn ptrWriteSlice(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len || s.len == 0) {
        return pos;
    }
    const rem: usize = out.len - pos;
    const n: usize = if (s.len < rem) s.len else rem;
    @memcpy(out[pos .. pos + n], s[0 .. n]);
    return pos + n;
}

fn ptrFindParam(params: []const RouteParam, name: string): string? {
    let i: usize = 0;
    while (i < params.len) {
        if (ptrStrEq(params[i].name, name)) {
            return params[i].value;
        }
        i += 1;
    }
    return null;
}

/// Extracted or supplied route parameter key/value pair.
pub struct RouteParam {
    pub name: string,
    pub value: string,

    pub static fn new(name: string, value: string): RouteParam {
        return RouteParam { name: name, value: value };
    }
}

/// Zero-heap route match result holding up to 16 extracted parameters.
pub struct RouteMatch {
    pub matched: bool,
    pub params: [16]RouteParam,
    pub param_count: usize,

    pub static fn noMatch(): RouteMatch {
        let empty = RouteParam { name: "", value: "" };
        return RouteMatch {
            matched: false,
            params: [
                empty, empty, empty, empty,
                empty, empty, empty, empty,
                empty, empty, empty, empty,
                empty, empty, empty, empty,
            ],
            param_count: 0,
        };
    }

    /// Retrieves an extracted parameter value by exact parameter name.
    pub fn getParam(name: string): string? {
        if (!_.matched) { return null; }
        let i: usize = 0;
        while (i < _.param_count) {
            if (ptrStrEq(_.params[i].name, name)) {
                return _.params[i].value;
            }
            i += 1;
        }
        return null;
    }
}

/// Matches `path` against a route `pattern` (supporting static segments, `:param`,
/// optional `:param?`, wildcard `*rest` / `*rest?`, and trailing-slash tolerance).
pub fn matchPath(pattern: string, path: string, case_sensitive: bool): RouteMatch {
    let res = RouteMatch.noMatch();
    if (pattern.len == 0 || path.len == 0) {
        return res;
    }
    // Require leading '/' consistency
    if ((pattern[0] == 47) != (path[0] == 47)) {
        return res;
    }

    const pat = ptrTrimSlashes(pattern);
    const pth = ptrTrimSlashes(path);

    let pi: usize = 0;
    let si: usize = 0;

    while (pi < pat.len) {
        pi = ptrSkipSlashes(pat, pi);
        if (pi >= pat.len) { break; }
        const p_end = ptrFindSegEnd(pat, pi);
        const p_seg = pat[pi..p_end];
        pi = p_end;

        // Wildcard segment: *name or *name? or *
        if (p_seg[0] == 42) {
            const is_opt = p_seg.len > 2 && p_seg[p_seg.len - 1] == 63;
            const w_name = if (is_opt) {
                p_seg[1..p_seg.len - 1]
            } else if (p_seg.len > 1) {
                p_seg[1..p_seg.len]
            } else {
                "*"
            };

            si = ptrSkipSlashes(pth, si);
            if (si >= pth.len) {
                if (is_opt) {
                    continue;
                }
                return RouteMatch.noMatch();
            }

            const rest = pth[si..pth.len];
            si = pth.len;
            if (res.param_count < 16) {
                res.params[res.param_count] = RouteParam { name: w_name, value: rest };
                res.param_count += 1;
            }
            // Wildcard consumes the remainder of the path
            continue;
        }

        // Named parameter segment: :name or :name?
        if (p_seg[0] == 58) {
            const is_opt = p_seg.len > 2 && p_seg[p_seg.len - 1] == 63;
            const k_name = if (is_opt) {
                p_seg[1..p_seg.len - 1]
            } else if (p_seg.len > 1) {
                p_seg[1..p_seg.len]
            } else {
                ""
            };
            if (k_name.len == 0) {
                return RouteMatch.noMatch();
            }

            si = ptrSkipSlashes(pth, si);
            if (si >= pth.len) {
                if (is_opt) {
                    continue;
                }
                return RouteMatch.noMatch();
            }
            const s_end = ptrFindSegEnd(pth, si);
            const s_seg = pth[si..s_end];
            si = s_end;

            if (res.param_count < 16) {
                res.params[res.param_count] = RouteParam { name: k_name, value: s_seg };
                res.param_count += 1;
            }
            continue;
        }

        // Static segment
        si = ptrSkipSlashes(pth, si);
        if (si >= pth.len) {
            return RouteMatch.noMatch();
        }
        const s_end = ptrFindSegEnd(pth, si);
        const s_seg = pth[si..s_end];
        si = s_end;

        if (!ptrSegEq(p_seg, s_seg, case_sensitive)) {
            return RouteMatch.noMatch();
        }
    }

    si = ptrSkipSlashes(pth, si);
    if (si < pth.len) {
        return RouteMatch.noMatch();
    }

    res.matched = true;
    return res;
}

/// Reverse-compiles a route `pattern` by substituting `:param`, `:param?`, and `*wildcard`
/// slots from `params` into `out`. Returns bytes written, or `0` if a required parameter is missing.
pub fn compilePath(pattern: string, params: []const RouteParam, out: []u8): usize {
    if (pattern.len == 0) { return 0; }
    const has_leading_slash = pattern[0] == 47;
    const pat = ptrTrimSlashes(pattern);

    if (pat.len == 0) {
        if (has_leading_slash) {
            return ptrWriteByte(out, 0, 47);
        }
        return 0;
    }

    let w: usize = 0;
    let pi: usize = 0;
    let first = true;

    while (pi < pat.len) {
        pi = ptrSkipSlashes(pat, pi);
        if (pi >= pat.len) { break; }
        const p_end = ptrFindSegEnd(pat, pi);
        const p_seg = pat[pi..p_end];
        pi = p_end;

        if (p_seg[0] == 42) {
            // Wildcard *name or *name?
            const is_opt = p_seg.len > 2 && p_seg[p_seg.len - 1] == 63;
            const w_name = if (is_opt) {
                p_seg[1..p_seg.len - 1]
            } else if (p_seg.len > 1) {
                p_seg[1..p_seg.len]
            } else {
                "*"
            };
            const val_opt = ptrFindParam(params, w_name);
            if (val_opt == null) {
                if (is_opt) { continue; }
                return 0;
            }
            const val = val_opt ?? "";
            if (val.len == 0) {
                if (is_opt) { continue; }
                return 0;
            }
            const clean_val = ptrTrimSlashes(val);
            if (!first || has_leading_slash) {
                w = ptrWriteByte(out, w, 47);
            }
            w = ptrWriteSlice(out, w, clean_val);
            first = false;
            continue;
        }

        if (p_seg[0] == 58) {
            // Named param :name or :name?
            const is_opt = p_seg.len > 2 && p_seg[p_seg.len - 1] == 63;
            const k_name = if (is_opt) {
                p_seg[1..p_seg.len - 1]
            } else if (p_seg.len > 1) {
                p_seg[1..p_seg.len]
            } else {
                ""
            };
            if (k_name.len == 0) { return 0; }

            const val_opt = ptrFindParam(params, k_name);
            if (val_opt == null) {
                if (is_opt) { continue; }
                return 0;
            }
            const val = val_opt ?? "";
            if (val.len == 0) {
                if (is_opt) { continue; }
                return 0;
            }
            if (!first || has_leading_slash) {
                w = ptrWriteByte(out, w, 47);
            }
            w = ptrWriteSlice(out, w, val);
            first = false;
            continue;
        }

        // Static segment
        if (!first || has_leading_slash) {
            w = ptrWriteByte(out, w, 47);
        }
        w = ptrWriteSlice(out, w, p_seg);
        first = false;
    }

    if (w == 0 && has_leading_slash) {
        return ptrWriteByte(out, 0, 47);
    }
    return w;
}

/// Counts the number of dynamic parameter slots (`:name`, `:name?`, `*wildcard`) in `pattern`.
pub fn countRouteParams(pattern: string): usize {
    const pat = ptrTrimSlashes(pattern);
    let count: usize = 0;
    let pi: usize = 0;

    while (pi < pat.len) {
        pi = ptrSkipSlashes(pat, pi);
        if (pi >= pat.len) { break; }
        const p_end = ptrFindSegEnd(pat, pi);
        const p_seg = pat[pi..p_end];
        pi = p_end;

        if (p_seg[0] == 58 && p_seg.len > 1) {
            count += 1;
        } else if (p_seg[0] == 42) {
            count += 1;
        }
    }

    return count;
}

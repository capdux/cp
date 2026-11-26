// @cap/path-to-regexp — Zero-allocation URL Route Pattern Matcher & Builder
// Clean-room MIT implementation in pure CP.
// Supports static segments, named `:param`, optional `:param?`, and `*wildcard` segments.

pub const MAX_SEGMENTS: usize = 16;
pub const MAX_PARAMS: usize = 8;
const MAX_PATTERN_LEN: usize = 256;

const SEG_LITERAL: u8 = 0;
const SEG_PARAM: u8 = 1;
const SEG_OPT_PARAM: u8 = 2;
const SEG_WILDCARD: u8 = 3;

pub struct ParamSlice {
    pub name_start: usize,
    pub name_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

pub struct MatchResult {
    pub matched: bool,
    pub param_count: usize,
    pub params: [8]ParamSlice,
}

struct Segment {
    kind: u8,
    start: usize,
    len: usize,
}

fn emptyMatchResult() -> MatchResult {
    let params: [8]ParamSlice = undefined;
    let i: usize = 0;
    while (i < MAX_PARAMS) {
        params[i] = ParamSlice {
            name_start: 0,
            name_len: 0,
            val_start: 0,
            val_len: 0,
        };
        i += 1;
    }
    return MatchResult {
        matched: false,
        param_count: 0,
        params: params,
    };
}

pub struct RoutePattern {
    pat_buf: [256]u8,
    pat_len: usize,
    segments: [16]Segment,
    seg_count: usize,
    param_count: usize,

    /// Compiles a route pattern such as `/users/:id/posts/:postId?` or `/files/*path`.
    pub static fn compile(pattern: []const u8) -> RoutePattern {
        let pat_buf: [256]u8 = undefined;
        let plen = pattern.len;
        if (plen > MAX_PATTERN_LEN) {
            plen = MAX_PATTERN_LEN;
        }
        let c: usize = 0;
        while (c < plen) {
            pat_buf[c] = pattern[c];
            c += 1;
        }

        let segments: [16]Segment = undefined;
        let seg_count: usize = 0;
        let param_count: usize = 0;

        let i: usize = 0;
        while (i < plen && seg_count < MAX_SEGMENTS) {
            while (i < plen && pat_buf[i] == 47) {
                i += 1; // skip '/'
            }
            if (i >= plen) {
                break;
            }
            const seg_start = i;
            while (i < plen && pat_buf[i] != 47) {
                i += 1;
            }
            const seg_end = i;
            const seg_len = seg_end - seg_start;

            if (pat_buf[seg_start] == 58) {
                // ':' named parameter
                const nstart = seg_start + 1;
                let nlen = seg_len - 1;
                let kind: u8 = SEG_PARAM;
                if (nlen > 0 && pat_buf[seg_end - 1] == 63) {
                    // '?' optional parameter
                    kind = SEG_OPT_PARAM;
                    nlen -= 1;
                }
                segments[seg_count] = Segment {
                    kind: kind,
                    start: nstart,
                    len: nlen,
                };
                seg_count += 1;
                param_count += 1;
            } else if (pat_buf[seg_start] == 42) {
                // '*' wildcard parameter
                segments[seg_count] = Segment {
                    kind: SEG_WILDCARD,
                    start: seg_start + 1,
                    len: seg_len - 1,
                };
                seg_count += 1;
                param_count += 1;
                break;
            } else {
                segments[seg_count] = Segment {
                    kind: SEG_LITERAL,
                    start: seg_start,
                    len: seg_len,
                };
                seg_count += 1;
            }
        }

        return RoutePattern {
            pat_buf: pat_buf,
            pat_len: plen,
            segments: segments,
            seg_count: seg_count,
            param_count: param_count,
        };
    }

    /// Returns the number of parsed route segments.
    pub fn segmentCount() -> usize {
        return _.seg_count;
    }

    /// Returns the number of parameter/wildcard segments in the compiled pattern.
    pub fn paramCount() -> usize {
        return _.param_count;
    }

    /// Matches `path` against this compiled route pattern (zero-copy offset extraction).
    /// Trailing slashes are normalized.
    pub fn matchPath(path: []const u8) -> MatchResult {
        let res = emptyMatchResult();

        // Trim trailing '/'
        let eff_len = path.len;
        while (eff_len > 0 && path[eff_len - 1] == 47) {
            eff_len -= 1;
        }

        let pos: usize = 0;
        let si: usize = 0;
        let pcount: usize = 0;

        while (si < _.seg_count) {
            while (pos < eff_len && path[pos] == 47) {
                pos += 1;
            }

            const seg = _.segments[si];

            if (seg.kind == SEG_WILDCARD) {
                if (pcount < MAX_PARAMS) {
                    res.params[pcount] = ParamSlice {
                        name_start: seg.start,
                        name_len: seg.len,
                        val_start: pos,
                        val_len: eff_len - pos,
                    };
                    pcount += 1;
                }
                pos = eff_len;
                si += 1;
                break;
            }

            if (pos >= eff_len) {
                if (seg.kind == SEG_OPT_PARAM) {
                    si += 1;
                    continue;
                }
                return res;
            }

            const val_start = pos;
            while (pos < eff_len && path[pos] != 47) {
                pos += 1;
            }
            const val_len = pos - val_start;

            if (seg.kind == SEG_LITERAL) {
                if (val_len != seg.len) {
                    return res;
                }
                let k: usize = 0;
                while (k < seg.len) {
                    if (_.pat_buf[seg.start + k] != path[val_start + k]) {
                        return res;
                    }
                    k += 1;
                }
            } else {
                // SEG_PARAM or SEG_OPT_PARAM
                if (pcount < MAX_PARAMS) {
                    res.params[pcount] = ParamSlice {
                        name_start: seg.start,
                        name_len: seg.len,
                        val_start: val_start,
                        val_len: val_len,
                    };
                    pcount += 1;
                }
            }

            si += 1;
        }

        while (pos < eff_len && path[pos] == 47) {
            pos += 1;
        }
        if (pos < eff_len) {
            return res;
        }

        res.matched = true;
        res.param_count = pcount;
        return res;
    }
}

/// Builds a concrete URL path by substituting `param_values` into `pattern` in order.
/// Returns the written byte length, or `-1` if a required parameter is missing or `dst` is too small.
pub fn buildPath(pattern: []const u8, param_values: []const []const u8, dst: []u8) -> i64 {
    const compiled = RoutePattern.compile(pattern);
    if (compiled.seg_count == 0) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = 47; // '/'
        return 1;
    }

    let out_pos: usize = 0;
    let val_idx: usize = 0;
    let si: usize = 0;

    while (si < compiled.seg_count) {
        const seg = compiled.segments[si];
        if (seg.kind == SEG_LITERAL) {
            if (out_pos + 1 + seg.len > dst.len) {
                return -1;
            }
            dst[out_pos] = 47;
            out_pos += 1;
            let k: usize = 0;
            while (k < seg.len) {
                dst[out_pos] = compiled.pat_buf[seg.start + k];
                out_pos += 1;
                k += 1;
            }
        } else if (seg.kind == SEG_PARAM || seg.kind == SEG_WILDCARD) {
            if (val_idx >= param_values.len) {
                return -1;
            }
            const val = param_values[val_idx];
            val_idx += 1;
            if (seg.kind == SEG_PARAM && val.len == 0) {
                return -1;
            }
            if (out_pos + 1 + val.len > dst.len) {
                return -1;
            }
            dst[out_pos] = 47;
            out_pos += 1;
            let k: usize = 0;
            while (k < val.len) {
                dst[out_pos] = val[k];
                out_pos += 1;
                k += 1;
            }
        } else {
            // SEG_OPT_PARAM
            if (val_idx < param_values.len) {
                const val = param_values[val_idx];
                val_idx += 1;
                if (val.len > 0) {
                    if (out_pos + 1 + val.len > dst.len) {
                        return -1;
                    }
                    dst[out_pos] = 47;
                    out_pos += 1;
                    let k: usize = 0;
                    while (k < val.len) {
                        dst[out_pos] = val[k];
                        out_pos += 1;
                        k += 1;
                    }
                }
            }
        }
        si += 1;
    }

    if (out_pos == 0) {
        if (dst.len < 1) {
            return -1;
        }
        dst[0] = 47;
        return 1;
    }

    return out_pos as i64;
}

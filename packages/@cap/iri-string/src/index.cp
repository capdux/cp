// @cap/iri-string — Zero-allocation RFC 3986 / RFC 3987 URI/IRI parser & resolver
// Inspired by Rust `iri-string`
// MIT License

pub struct IriParts {
    pub scheme_start: usize,
    pub scheme_len: usize,
    pub auth_start: usize,
    pub auth_len: usize,
    pub path_start: usize,
    pub path_len: usize,
    pub query_start: usize,
    pub query_len: usize,
    pub frag_start: usize,
    pub frag_len: usize,
    pub has_scheme: bool,
    pub has_auth: bool,
    pub has_query: bool,
    pub has_frag: bool,
    pub valid: bool,
}

fn invalidIri(): IriParts {
    return IriParts {
        scheme_start: 0,
        scheme_len: 0,
        auth_start: 0,
        auth_len: 0,
        path_start: 0,
        path_len: 0,
        query_start: 0,
        query_len: 0,
        frag_start: 0,
        frag_len: 0,
        has_scheme: false,
        has_auth: false,
        has_query: false,
        has_frag: false,
        valid: false,
    };
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122);
}

fn isSchemeChar(c: u8): bool {
    return isAsciiAlpha(c) || (c >= 48 && c <= 57) || c == 43 || c == 45 || c == 46;
}

pub fn parseIri(s: string): IriParts {
    // Reject ASCII control characters (0x00..0x1F, 0x7F) and space (0x20)
    let k: usize = 0;
    while (k < s.len) {
        let ch: u8 = s[k];
        if (ch <= 0x20 || ch == 0x7F) {
            return invalidIri();
        }
        k = k + 1;
    }

    let res: IriParts = invalidIri();
    res.valid = true;

    let pos: usize = 0;
    // Detect scheme: scan up to ':', '/', '?', '#'
    let scan: usize = 0;
    while (scan < s.len && s[scan] != 58 && s[scan] != 47 && s[scan] != 63 && s[scan] != 35) {
        scan = scan + 1;
    }
    if (scan < s.len && s[scan] == 58) { // ':'
        if (scan == 0 || !isAsciiAlpha(s[0])) {
            return invalidIri();
        }
        let si: usize = 1;
        while (si < scan) {
            if (!isSchemeChar(s[si])) {
                return invalidIri();
            }
            si = si + 1;
        }
        res.has_scheme = true;
        res.scheme_start = 0;
        res.scheme_len = scan;
        pos = scan + 1;
    }

    // Detect optional //authority
    if (pos + 1 < s.len && s[pos] == 47 && s[pos + 1] == 47) {
        pos = pos + 2;
        res.has_auth = true;
        res.auth_start = pos;
        while (pos < s.len && s[pos] != 47 && s[pos] != 63 && s[pos] != 35) {
            pos = pos + 1;
        }
        res.auth_len = pos - res.auth_start;
    }

    // Path
    res.path_start = pos;
    while (pos < s.len && s[pos] != 63 && s[pos] != 35) {
        pos = pos + 1;
    }
    res.path_len = pos - res.path_start;

    // Optional ?query
    if (pos < s.len && s[pos] == 63) {
        pos = pos + 1;
        res.has_query = true;
        res.query_start = pos;
        while (pos < s.len && s[pos] != 35) {
            pos = pos + 1;
        }
        res.query_len = pos - res.query_start;
    }

    // Optional #fragment
    if (pos < s.len && s[pos] == 35) {
        pos = pos + 1;
        res.has_frag = true;
        res.frag_start = pos;
        res.frag_len = s.len - pos;
    }

    return res;
}

struct RemCursor {
    pos: usize,
    prepend_slash: bool,
}

fn remLen(path: string, cur: RemCursor): usize {
    let base: usize = path.len - cur.pos;
    if (cur.prepend_slash) {
        return base + 1;
    }
    return base;
}

fn remAt(path: string, cur: RemCursor, idx: usize): u8 {
    if (cur.prepend_slash) {
        if (idx == 0) {
            return 47; // '/'
        }
        return path[cur.pos + (idx - 1)];
    }
    return path[cur.pos + idx];
}

fn remAdvance(cur: *RemCursor, n: usize) {
    let left: usize = n;
    if (cur.*.prepend_slash && left > 0) {
        cur.*.prepend_slash = false;
        left = left - 1;
    }
    cur.*.pos = cur.*.pos + left;
}

fn popLastSegment(out: []u8, out_len: usize): usize {
    let n: usize = out_len;
    while (n > 0) {
        n = n - 1;
        if (out[n] == 47) { // '/'
            return n;
        }
    }
    return 0;
}

// RFC 3986 Section 5.2.4 Remove Dot Segments
pub fn removeDotSegments(path: string, out: []u8): usize {
    if (path.len == 0) {
        return 0;
    }
    let cur: RemCursor = RemCursor { pos: 0, prepend_slash: false };
    let out_len: usize = 0;

    while (remLen(path, cur) > 0) {
        let rlen: usize = remLen(path, cur);

        // Rule A: prefix "../" or "./"
        if (rlen >= 3 && remAt(path, cur, 0) == 46 && remAt(path, cur, 1) == 46 && remAt(path, cur, 2) == 47) {
            remAdvance(&cur, 3);
            continue;
        }
        if (rlen >= 2 && remAt(path, cur, 0) == 46 && remAt(path, cur, 1) == 47) {
            remAdvance(&cur, 2);
            continue;
        }

        // Rule B: prefix "/./" or exact "/."
        if (rlen >= 3 && remAt(path, cur, 0) == 47 && remAt(path, cur, 1) == 46 && remAt(path, cur, 2) == 47) {
            remAdvance(&cur, 2);
            continue;
        }
        if (rlen == 2 && remAt(path, cur, 0) == 47 && remAt(path, cur, 1) == 46) {
            remAdvance(&cur, 2);
            cur.prepend_slash = true;
            continue;
        }

        // Rule C: prefix "/../" or exact "/.."
        if (rlen >= 4 && remAt(path, cur, 0) == 47 && remAt(path, cur, 1) == 46 &&
            remAt(path, cur, 2) == 46 && remAt(path, cur, 3) == 47) {
            remAdvance(&cur, 3);
            out_len = popLastSegment(out, out_len);
            continue;
        }
        if (rlen == 3 && remAt(path, cur, 0) == 47 && remAt(path, cur, 1) == 46 && remAt(path, cur, 2) == 46) {
            remAdvance(&cur, 3);
            cur.prepend_slash = true;
            out_len = popLastSegment(out, out_len);
            continue;
        }

        // Rule D: exact "." or ".."
        if (rlen == 1 && remAt(path, cur, 0) == 46) {
            remAdvance(&cur, 1);
            continue;
        }
        if (rlen == 2 && remAt(path, cur, 0) == 46 && remAt(path, cur, 1) == 46) {
            remAdvance(&cur, 2);
            continue;
        }

        // Rule E: move first path segment (including initial '/' if any) to output
        let seg_len: usize = 0;
        if (remAt(path, cur, 0) == 47) {
            seg_len = 1;
        }
        while (seg_len < rlen && remAt(path, cur, seg_len) != 47) {
            seg_len = seg_len + 1;
        }
        if (out_len + seg_len > out.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < seg_len) {
            out[out_len] = remAt(path, cur, i);
            out_len = out_len + 1;
            i = i + 1;
        }
        remAdvance(&cur, seg_len);
    }

    return out_len;
}

struct OutBuf {
    pos: usize,
    overflow: bool,
}

fn obPush(ob: *OutBuf, out: []u8, ch: u8) {
    if (ob.*.overflow || ob.*.pos >= out.len) {
        ob.*.overflow = true;
        return;
    }
    out[ob.*.pos] = ch;
    ob.*.pos = ob.*.pos + 1;
}

fn obWrite(ob: *OutBuf, out: []u8, src: string) {
    if (ob.*.overflow || src.len > out.len - ob.*.pos) {
        ob.*.overflow = true;
        return;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[ob.*.pos] = src[i];
        ob.*.pos = ob.*.pos + 1;
        i = i + 1;
    }
}

// RFC 3986 Section 5.2.2 Transform References
pub fn resolveReference(base: string, rel: string, out: []u8): usize {
    let b: IriParts = parseIri(base);
    let r: IriParts = parseIri(rel);
    if (!b.valid || !r.valid || !b.has_scheme) {
        return 0;
    }

    let path_buf: [512]u8 = [0; 512];
    let merge_buf: [512]u8 = [0; 512];

    let t_scheme: string = base[b.scheme_start..(b.scheme_start + b.scheme_len)];
    let t_has_auth: bool = false;
    let t_auth: string = "";
    let t_path_len: usize = 0;
    let t_has_query: bool = false;
    let t_query: string = "";
    let t_has_frag: bool = r.has_frag;
    let t_frag: string = rel[r.frag_start..(r.frag_start + r.frag_len)];

    if (r.has_scheme) {
        t_scheme = rel[r.scheme_start..(r.scheme_start + r.scheme_len)];
        t_has_auth = r.has_auth;
        t_auth = rel[r.auth_start..(r.auth_start + r.auth_len)];
        let rp: string = rel[r.path_start..(r.path_start + r.path_len)];
        t_path_len = removeDotSegments(rp, path_buf[0..512]);
        t_has_query = r.has_query;
        t_query = rel[r.query_start..(r.query_start + r.query_len)];
    } else {
        if (r.has_auth) {
            t_has_auth = true;
            t_auth = rel[r.auth_start..(r.auth_start + r.auth_len)];
            let rp: string = rel[r.path_start..(r.path_start + r.path_len)];
            t_path_len = removeDotSegments(rp, path_buf[0..512]);
            t_has_query = r.has_query;
            t_query = rel[r.query_start..(r.query_start + r.query_len)];
        } else {
            t_has_auth = b.has_auth;
            t_auth = base[b.auth_start..(b.auth_start + b.auth_len)];
            if (r.path_len == 0) {
                let bp: string = base[b.path_start..(b.path_start + b.path_len)];
                if (bp.len > 512) {
                    return 0;
                }
                let bi: usize = 0;
                while (bi < bp.len) {
                    path_buf[bi] = bp[bi];
                    bi = bi + 1;
                }
                t_path_len = bp.len;
                if (r.has_query) {
                    t_has_query = true;
                    t_query = rel[r.query_start..(r.query_start + r.query_len)];
                } else {
                    t_has_query = b.has_query;
                    t_query = base[b.query_start..(b.query_start + b.query_len)];
                }
            } else {
                let rp: string = rel[r.path_start..(r.path_start + r.path_len)];
                if (rp[0] == 47) { // '/'
                    t_path_len = removeDotSegments(rp, path_buf[0..512]);
                } else {
                    // RFC 3986 Section 5.2.3 Merge Paths
                    let bp: string = base[b.path_start..(b.path_start + b.path_len)];
                    let mlen: usize = 0;
                    if (b.has_auth && bp.len == 0) {
                        merge_buf[0] = 47;
                        mlen = 1;
                    } else {
                        // Copy bp up to and including last '/'
                        let last_slash: usize = 0;
                        let found_slash: bool = false;
                        let si: usize = 0;
                        while (si < bp.len) {
                            if (bp[si] == 47) {
                                last_slash = si;
                                found_slash = true;
                            }
                            si = si + 1;
                        }
                        if (found_slash) {
                            let prefix_len: usize = last_slash + 1;
                            if (prefix_len > 512) {
                                return 0;
                            }
                            let pi: usize = 0;
                            while (pi < prefix_len) {
                                merge_buf[pi] = bp[pi];
                                pi = pi + 1;
                            }
                            mlen = prefix_len;
                        }
                    }
                    if (mlen + rp.len > 512) {
                        return 0;
                    }
                    let ri: usize = 0;
                    while (ri < rp.len) {
                        merge_buf[mlen] = rp[ri];
                        mlen = mlen + 1;
                        ri = ri + 1;
                    }
                    t_path_len = removeDotSegments(merge_buf[0..mlen], path_buf[0..512]);
                }
                t_has_query = r.has_query;
                t_query = rel[r.query_start..(r.query_start + r.query_len)];
            }
        }
    }

    // Recompose per RFC 3986 Section 5.3
    let ob: OutBuf = OutBuf { pos: 0, overflow: false };
    obWrite(&ob, out, t_scheme);
    obPush(&ob, out, 58); // ':'
    if (t_has_auth) {
        obPush(&ob, out, 47);
        obPush(&ob, out, 47);
        obWrite(&ob, out, t_auth);
    }
    obWrite(&ob, out, path_buf[0..t_path_len]);
    if (t_has_query) {
        obPush(&ob, out, 63); // '?'
        obWrite(&ob, out, t_query);
    }
    if (t_has_frag) {
        obPush(&ob, out, 35); // '#'
        obWrite(&ob, out, t_frag);
    }
    if (ob.overflow) {
        return 0;
    }
    return ob.pos;
}

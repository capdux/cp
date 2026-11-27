// @cap/normalize-url — Zero-allocation URL normalization utility for CP
// Clean-room MIT implementation with parity to npm `normalize-url`.
// Composes `@cap/prepend-http` with zero duplication.

import {
    prependHttp,
    hasScheme,
    isProtocolRelative,
} from "../../prepend-http/src/index.cp";

const NURL_SCRATCH_CAP: usize = 8192;
const NURL_MAX_SEGMENTS: usize = 256;

/// Configuration options for `normalizeUrlWithOptions`.
pub struct NormalizeUrlOptions {
    default_https: bool = false,
    strip_www: bool = true,
    strip_hash: bool = false,
    remove_trailing_slash: bool = true,
    remove_default_port: bool = true,
    normalize_protocol: bool = true,
}

struct NurlSegment {
    start: usize,
    end: usize,
}

/// Returns the default `NormalizeUrlOptions` matching npm `normalize-url`.
pub fn defaultNormalizeUrlOptions(): NormalizeUrlOptions {
    return NormalizeUrlOptions{
        default_https: false,
        strip_www: true,
        strip_hash: false,
        remove_trailing_slash: true,
        remove_default_port: true,
        normalize_protocol: true,
    };
}

fn nurlIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0b || c == 0x0c;
}

fn nurlTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && nurlIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && nurlIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn nurlToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn nurlEqStr(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn nurlEqLower(a: string, lower_b: string): bool {
    if (a.len != lower_b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (nurlToLowerAscii(a[i]) != lower_b[i]) {
            return false;
        }
    }
    return true;
}

fn nurlStartsWithLower(s: string, lower_prefix: string): bool {
    if (s.len < lower_prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lower_prefix.len) : (i += 1) {
        if (nurlToLowerAscii(s[i]) != lower_prefix[i]) {
            return false;
        }
    }
    return true;
}

fn nurlHasDot(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') {
            return true;
        }
    }
    return false;
}

fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn nurlWriteSlice(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn nurlWriteLower(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) : (i += 1) {
        out[p] = nurlToLowerAscii(s[i]);
        p += 1;
    }
    return p;
}

fn nurlFindSchemeSep(s: string): usize {
    // Returns index of ':' in "scheme://", or s.len if not found
    let i: usize = 0;
    while (i + 2 < s.len) : (i += 1) {
        if (s[i] == ':' && s[i + 1] == '/' && s[i + 2] == '/') {
            return i;
        }
        if (s[i] == '/' || s[i] == '?' || s[i] == '#') {
            return s.len;
        }
    }
    return s.len;
}

fn nurlIsDefaultPort(scheme: string, port: string): bool {
    if (nurlEqLower(scheme, "http") && nurlEqStr(port, "80")) {
        return true;
    }
    if (nurlEqLower(scheme, "https") && nurlEqStr(port, "443")) {
        return true;
    }
    if (nurlEqLower(scheme, "ftp") && nurlEqStr(port, "21")) {
        return true;
    }
    return false;
}

fn nurlNormalizePath(
    raw_path: string,
    remove_trailing_slash: bool,
    has_query_or_hash: bool,
    out: []u8,
    start_pos: usize,
): usize {
    if (raw_path.len == 0) {
        return start_pos;
    }

    // Track whether the raw path ends with '/', "/.", or "/.." to preserve directory intent
    let had_trailing_slash = raw_path[raw_path.len - 1] == '/';

    let segs: [NURL_MAX_SEGMENTS]NurlSegment = undefined;
    let seg_count: usize = 0;

    let i: usize = 0;
    while (i < raw_path.len) {
        // Collapse repeated '/'
        while (i < raw_path.len && raw_path[i] == '/') {
            i += 1;
        }
        if (i >= raw_path.len) {
            break;
        }
        const seg_start = i;
        while (i < raw_path.len && raw_path[i] != '/') {
            i += 1;
        }
        const seg_end = i;
        const seg = raw_path[seg_start..seg_end];

        if (nurlEqStr(seg, ".")) {
            if (i >= raw_path.len) {
                had_trailing_slash = true;
            }
        } else if (nurlEqStr(seg, "..")) {
            if (seg_count > 0) {
                seg_count -= 1;
            }
            if (i >= raw_path.len) {
                had_trailing_slash = true;
            }
        } else {
            if (seg_count < NURL_MAX_SEGMENTS) {
                segs[seg_count] = NurlSegment{ start: seg_start, end: seg_end };
                seg_count += 1;
            }
        }
    }

    let pos = start_pos;
    if (seg_count == 0) {
        if (!remove_trailing_slash) {
            pos = nurlWriteByte(out, pos, '/');
        }
        return pos;
    }

    let k: usize = 0;
    while (k < seg_count) : (k += 1) {
        pos = nurlWriteByte(out, pos, '/');
        pos = nurlWriteSlice(out, pos, raw_path[segs[k].start..segs[k].end]);
    }

    if (had_trailing_slash && !remove_trailing_slash) {
        pos = nurlWriteByte(out, pos, '/');
    }

    return pos;
}

/// Normalize `url` according to `opts`, writing the result into `out` and returning bytes written.
pub fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize {
    const trimmed = nurlTrim(url);
    if (trimmed.len == 0) {
        return 0;
    }

    let prep_buf: [NURL_SCRATCH_CAP]u8 = undefined;
    let work_len: usize = 0;

    if (isProtocolRelative(trimmed)) {
        if (opts.normalize_protocol) {
            // Strip leading "//" and delegate protocol prepending to @cap/prepend-http
            work_len = prependHttp(trimmed[2..trimmed.len], opts.default_https, prep_buf[0..NURL_SCRATCH_CAP]);
        } else {
            work_len = nurlWriteSlice(prep_buf[0..NURL_SCRATCH_CAP], 0, trimmed);
        }
    } else if (!hasScheme(trimmed)) {
        work_len = prependHttp(trimmed, opts.default_https, prep_buf[0..NURL_SCRATCH_CAP]);
    } else {
        work_len = prependHttp(trimmed, opts.default_https, prep_buf[0..NURL_SCRATCH_CAP]);
    }

    const work = prep_buf[0..work_len];
    let pos: usize = 0;

    // Determine scheme & authority start
    let scheme: string = "";
    let auth_start: usize = 0;
    let is_proto_rel = false;

    const sep_idx = nurlFindSchemeSep(work);
    if (sep_idx < work.len) {
        scheme = work[0..sep_idx];
        pos = nurlWriteLower(out, pos, scheme);
        pos = nurlWriteSlice(out, pos, "://");
        auth_start = sep_idx + 3;
    } else if (work.len >= 2 && work[0] == '/' && work[1] == '/') {
        is_proto_rel = true;
        pos = nurlWriteSlice(out, pos, "//");
        auth_start = 2;
    } else {
        // Non-hierarchical scheme (e.g. mailto:) or relative path
        let hash_idx: usize = work.len;
        let hi: usize = 0;
        while (hi < work.len) : (hi += 1) {
            if (work[hi] == '#') {
                hash_idx = hi;
                break;
            }
        }
        const effective = if (opts.strip_hash) work[0..hash_idx] else work;
        // Lowercase scheme before ':' if present
        let colon_idx: usize = effective.len;
        let ci: usize = 0;
        while (ci < effective.len) : (ci += 1) {
            if (effective[ci] == ':') {
                colon_idx = ci;
                break;
            }
            if (effective[ci] == '/' || effective[ci] == '?' || effective[ci] == '#') {
                break;
            }
        }
        if (colon_idx < effective.len) {
            pos = nurlWriteLower(out, pos, effective[0..colon_idx]);
            pos = nurlWriteSlice(out, pos, effective[colon_idx..effective.len]);
        } else {
            pos = nurlWriteSlice(out, pos, effective);
        }
        return pos;
    }

    // Find end of authority ('/', '?', or '#')
    let auth_end: usize = auth_start;
    while (auth_end < work.len && work[auth_end] != '/' && work[auth_end] != '?' && work[auth_end] != '#') {
        auth_end += 1;
    }
    const authority = work[auth_start..auth_end];

    // Split userinfo@hostport (use last '@' in authority)
    let at_idx: usize = authority.len;
    let ai: usize = 0;
    while (ai < authority.len) : (ai += 1) {
        if (authority[ai] == '@') {
            at_idx = ai;
        }
    }

    let hostport = authority;
    if (at_idx < authority.len) {
        pos = nurlWriteSlice(out, pos, authority[0..at_idx + 1]);
        hostport = authority[at_idx + 1..authority.len];
    }

    // Split host and optional :port (IPv6-aware)
    let host = hostport;
    let port: string = "";
    if (hostport.len > 0 && hostport[0] == '[') {
        let close_bracket: usize = hostport.len;
        let bi: usize = 1;
        while (bi < hostport.len) : (bi += 1) {
            if (hostport[bi] == ']') {
                close_bracket = bi;
                break;
            }
        }
        if (close_bracket + 1 < hostport.len && hostport[close_bracket + 1] == ':') {
            host = hostport[0..close_bracket + 1];
            port = hostport[close_bracket + 2..hostport.len];
        } else {
            host = hostport;
        }
    } else {
        let colon_pos: usize = hostport.len;
        let ci: usize = 0;
        while (ci < hostport.len) : (ci += 1) {
            if (hostport[ci] == ':') {
                colon_pos = ci;
                break;
            }
        }
        if (colon_pos < hostport.len) {
            host = hostport[0..colon_pos];
            port = hostport[colon_pos + 1..hostport.len];
        }
    }

    // Strip leading "www." if requested and remainder still contains a dot (e.g. www.example.com)
    if (opts.strip_www && nurlStartsWithLower(host, "www.")) {
        const after_www = host[4..host.len];
        if (nurlHasDot(after_www)) {
            host = after_www;
        }
    }

    // Write lowercased host
    pos = nurlWriteLower(out, pos, host);

    // Write port unless empty or default port
    if (port.len > 0) {
        const drop_port = opts.remove_default_port && !is_proto_rel && nurlIsDefaultPort(scheme, port);
        if (!drop_port) {
            pos = nurlWriteByte(out, pos, ':');
            pos = nurlWriteSlice(out, pos, port);
        }
    }

    // Split remainder into path, query, hash
    const rest = work[auth_end..work.len];
    let q_idx: usize = rest.len;
    let h_idx: usize = rest.len;
    let ri: usize = 0;
    while (ri < rest.len) : (ri += 1) {
        if (rest[ri] == '?' && q_idx == rest.len && h_idx == rest.len) {
            q_idx = ri;
        } else if (rest[ri] == '#' && h_idx == rest.len) {
            h_idx = ri;
            break;
        }
    }

    const path_end = if (q_idx < h_idx) q_idx else h_idx;
    const raw_path = rest[0..path_end];
    const query = if (q_idx < h_idx) rest[q_idx..h_idx] else "";
    const hash = if (h_idx < rest.len && !opts.strip_hash) rest[h_idx..rest.len] else "";

    const has_qh = query.len > 0 || hash.len > 0;
    pos = nurlNormalizePath(raw_path, opts.remove_trailing_slash, has_qh, out, pos);
    pos = nurlWriteSlice(out, pos, query);
    pos = nurlWriteSlice(out, pos, hash);

    return pos;
}

/// Normalize `url` using default options (`defaultNormalizeUrlOptions()`),
/// writing the result into `out` and returning bytes written.
pub fn normalizeUrl(url: string, out: []u8): usize {
    return normalizeUrlWithOptions(url, defaultNormalizeUrlOptions(), out);
}

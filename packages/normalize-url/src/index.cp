// @cap/normalize-url — Zero-allocation URL normalization utility for CP
// Clean-room MIT implementation with parity to npm `normalize-url`.
// Composes `@cap/prepend-http` with zero duplication.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning/lowercasing, comptime 256-entry LUT, and @memcpy.

import {
    prependHttp,
    hasScheme,
    isProtocolRelative,
} from "../../prepend-http/src/index.cp";
import {
    trim,
    strEq,
    eqIgnoreCase,
    findByte,
    rfindByte,
} from "cap:ascii";
import { writeStr } from "cap:fmt";

type NurlVec16 = @Vector(16, u8);

fn nurlBuildLowerLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        lut[c] = if (c >= 65 && c <= 90) (c + 32) as u8 else c as u8;
        c += 1;
    }
    return lut;
}

const NURL_LOWER_LUT: [256]u8 = nurlBuildLowerLut();

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

fn nurlWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn nurlWriteLower(out: []u8, pos: usize, s: string): usize {
    if (pos >= out.len || s.len == 0) { return pos; }
    const avail = out.len - pos;
    const limit = if (s.len < avail) s.len else avail;
    const v_zero: NurlVec16 = @splat(0 as u8);
    const v_one: NurlVec16 = @splat(1 as u8);
    const v_up_a: NurlVec16 = @splat(65 as u8);
    const v_up_z: NurlVec16 = @splat(90 as u8);
    const v_diff: NurlVec16 = @splat(32 as u8);
    let i: usize = 0;
    while (i + 16 <= limit) {
        const v: NurlVec16 = s[i .. i + 16][0..16].*;
        const is_up = @select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero);
        const low: NurlVec16 = v + @select(u8, is_up != v_zero, v_diff, v_zero);
        const arr: [16]u8 = low;
        @memcpy(out[pos + i .. pos + i + 16], arr[0..16]);
        i += 16;
    }
    while (i < limit) {
        out[pos + i] = NURL_LOWER_LUT[s[i] as usize];
        i += 1;
    }
    return pos + limit;
}

fn nurlFindAuthEnd(s: string, start: usize): usize {
    let i = start;
    const v_sl: NurlVec16 = @splat(47 as u8); // '/'
    const v_q: NurlVec16 = @splat(63 as u8);  // '?'
    const v_h: NurlVec16 = @splat(35 as u8);  // '#'
    while (i + 16 <= s.len) {
        const v: NurlVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.Or, v == v_sl) && !@reduce(.Or, v == v_q) && !@reduce(.Or, v == v_h)) {
            i += 16;
            continue;
        }
        break;
    }
    while (i < s.len && s[i] != '/' && s[i] != '?' && s[i] != '#') {
        i += 1;
    }
    return i;
}

fn nurlFindFirstQH(s: string, start: usize): usize {
    let i = start;
    const v_q: NurlVec16 = @splat(63 as u8); // '?'
    const v_h: NurlVec16 = @splat(35 as u8); // '#'
    while (i + 16 <= s.len) {
        const v: NurlVec16 = s[i .. i + 16][0..16].*;
        if (!@reduce(.Or, v == v_q) && !@reduce(.Or, v == v_h)) {
            i += 16;
            continue;
        }
        break;
    }
    while (i < s.len && s[i] != '?' && s[i] != '#') {
        i += 1;
    }
    return i;
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
    if (eqIgnoreCase(scheme, "http") && strEq(port, "80")) { return true; }
    if (eqIgnoreCase(scheme, "https") && strEq(port, "443")) { return true; }
    if (eqIgnoreCase(scheme, "ftp") && strEq(port, "21")) { return true; }
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
        while (i < raw_path.len && raw_path[i] == '/') {
            i += 1;
        }
        if (i >= raw_path.len) { break; }
        const seg_start = i;
        const seg_end = findByte(raw_path, '/', seg_start) ?? raw_path.len;
        i = seg_end;
        const seg = raw_path[seg_start..seg_end];

        if (strEq(seg, ".")) {
            if (i >= raw_path.len) {
                had_trailing_slash = true;
            }
        } else if (strEq(seg, "..")) {
            if (seg_count > 0) {
                seg_count -= 1;
            }
            if (i >= raw_path.len) {
                had_trailing_slash = true;
            }
        } else if (seg_count < NURL_MAX_SEGMENTS) {
            segs[seg_count] = NurlSegment{ start: seg_start, end: seg_end };
            seg_count += 1;
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
        pos = writeStr(raw_path[segs[k].start..segs[k].end], out, pos);
    }

    if (had_trailing_slash && !remove_trailing_slash) {
        pos = nurlWriteByte(out, pos, '/');
    }

    return pos;
}

/// Normalize `url` according to `opts`, writing the result into `out` and returning bytes written.
pub fn normalizeUrlWithOptions(url: string, opts: NormalizeUrlOptions, out: []u8): usize {
    const trimmed = trim(url);
    if (trimmed.len == 0) {
        return 0;
    }

    let prep_buf: [NURL_SCRATCH_CAP]u8 = undefined;
    let work_len: usize = 0;

    if (isProtocolRelative(trimmed)) {
        if (opts.normalize_protocol) {
            work_len = prependHttp(trimmed[2..trimmed.len], opts.default_https, prep_buf[0..NURL_SCRATCH_CAP]);
        } else {
            work_len = writeStr(trimmed, prep_buf[0..NURL_SCRATCH_CAP], 0);
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
        pos = writeStr("://", out, pos);
        auth_start = sep_idx + 3;
    } else if (work.len >= 2 && work[0] == '/' && work[1] == '/') {
        is_proto_rel = true;
        pos = writeStr("//", out, pos);
        auth_start = 2;
    } else {
        // Non-hierarchical scheme (e.g. mailto:) or relative path
        const hash_idx = findByte(work, '#', 0) ?? work.len;
        const effective = if (opts.strip_hash) work[0..hash_idx] else work;
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
            pos = writeStr(effective[colon_idx..effective.len], out, pos);
        } else {
            pos = writeStr(effective, out, pos);
        }
        return pos;
    }

    // Find end of authority ('/', '?', or '#') via 16-lane SIMD
    const auth_end = nurlFindAuthEnd(work, auth_start);
    const authority = work[auth_start..auth_end];

    // Split userinfo@hostport (use last '@' in authority)
    let hostport = authority;
    if (at_idx := rfindByte(authority, '@')) {
        pos = writeStr(authority[0..at_idx + 1], out, pos);
        hostport = authority[at_idx + 1..authority.len];
    }

    // Split host and optional :port (IPv6-aware)
    let host = hostport;
    let port: string = "";
    if (hostport.len > 0 && hostport[0] == '[') {
        const close_bracket = findByte(hostport, ']', 1) ?? hostport.len;
        if (close_bracket + 1 < hostport.len && hostport[close_bracket + 1] == ':') {
            host = hostport[0..close_bracket + 1];
            port = hostport[close_bracket + 2..hostport.len];
        } else {
            host = hostport;
        }
    } else if (colon_pos := findByte(hostport, ':', 0)) {
        host = hostport[0..colon_pos];
        port = hostport[colon_pos + 1..hostport.len];
    }

    // Strip leading "www." if requested and remainder still contains a dot (e.g. www.example.com)
    if (opts.strip_www && host.len >= 4 && eqIgnoreCase(host[0..4], "www.")) {
        const after_www = host[4..host.len];
        if (findByte(after_www, '.', 0) != null) {
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
            pos = writeStr(port, out, pos);
        }
    }

    // Split remainder into path, query, hash via 16-lane SIMD
    const rest = work[auth_end..work.len];
    const first_qh = nurlFindFirstQH(rest, 0);
    let q_idx: usize = rest.len;
    let h_idx: usize = rest.len;
    if (first_qh < rest.len) {
        if (rest[first_qh] == '?') {
            q_idx = first_qh;
            h_idx = findByte(rest, '#', first_qh + 1) ?? rest.len;
        } else {
            h_idx = first_qh;
        }
    }

    const path_end = if (q_idx < h_idx) q_idx else h_idx;
    const raw_path = rest[0..path_end];
    const query = if (q_idx < h_idx) rest[q_idx..h_idx] else "";
    const hash = if (h_idx < rest.len && !opts.strip_hash) rest[h_idx..rest.len] else "";

    const has_qh = query.len > 0 || hash.len > 0;
    pos = nurlNormalizePath(raw_path, opts.remove_trailing_slash, has_qh, out, pos);
    pos = writeStr(query, out, pos);
    pos = writeStr(hash, out, pos);

    return pos;
}

/// Normalize `url` using default options (`defaultNormalizeUrlOptions()`),
/// writing the result into `out` and returning bytes written.
pub fn normalizeUrl(url: string, out: []u8): usize {
    return normalizeUrlWithOptions(url, defaultNormalizeUrlOptions(), out);
}

// @cap/send — Pure CP zero-allocation static file streaming planner.
// Clean-room MIT implementation inspired by npm `send` v1.
// Reuses `@cap/encodeurl`, `@cap/etag`, `@cap/range-parser`, and `@cap/mime-types`.

import { encodeUrl } from "../../encodeurl/src/index.cp";
import { generateStatEtag, compareWeak } from "../../etag/src/index.cp";
import {
    RANGE_OK,
    parseRange,
    rangeByteLength
} from "../../range-parser/src/index.cp";
import { lookup } from "../../mime-types/src/index.cp";

pub const DOTFILES_IGNORE: u8 = 0;
pub const DOTFILES_ALLOW: u8 = 1;
pub const DOTFILES_DENY: u8 = 2;

fn sndStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn sndIsWs(b: u8): bool {
    return b == (' ' as u8) || b == ('\t' as u8);
}

fn sndTrimWs(s: string): string {
    let start: usize = 0;
    while (start < s.len && sndIsWs(s[start])) {
        start = start + 1;
    }
    let end: usize = s.len;
    while (end > start && sndIsWs(s[end - 1])) {
        end = end - 1;
    }
    return s[start..end];
}

// Rejects NULL bytes and any `..` path traversal segment.
fn sndHasMaliciousPath(req_path: string): bool {
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= req_path.len) {
        if (i < req_path.len && req_path[i] == (0 as u8)) {
            return true;
        }
        let is_sep: bool = i == req_path.len
            || req_path[i] == ('/' as u8)
            || req_path[i] == ('\\' as u8);
        if (is_sep) {
            let seg_len: usize = i - seg_start;
            if (seg_len == 2
                && req_path[seg_start] == ('.' as u8)
                && req_path[seg_start + 1] == ('.' as u8))
            {
                return true;
            }
            seg_start = i + 1;
        }
        i = i + 1;
    }
    return false;
}

// Returns `true` if any path segment starts with `.` (excluding `.` and `..`).
fn sndContainsDotfile(req_path: string): bool {
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= req_path.len) {
        let is_sep: bool = i == req_path.len
            || req_path[i] == ('/' as u8)
            || req_path[i] == ('\\' as u8);
        if (is_sep) {
            let seg_len: usize = i - seg_start;
            if (seg_len >= 2 && req_path[seg_start] == ('.' as u8)) {
                if (!(seg_len == 2 && req_path[seg_start + 1] == ('.' as u8))) {
                    return true;
                }
            }
            seg_start = i + 1;
        }
        i = i + 1;
    }
    return false;
}

// Checks whether an `If-None-Match` header matches `current_etag` (supports `*` and comma-separated weak tags).
fn sndIfNoneMatchMatches(if_none_match: string, current_etag: string): bool {
    if (if_none_match.len == 0 || current_etag.len == 0) {
        return false;
    }
    let trimmed_all = sndTrimWs(if_none_match);
    if (sndStrEq(trimmed_all, "*")) {
        return true;
    }
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= if_none_match.len) {
        if (i == if_none_match.len || if_none_match[i] == (',' as u8)) {
            let token = sndTrimWs(if_none_match[seg_start..i]);
            if (token.len > 0) {
                if (sndStrEq(token, "*") || compareWeak(token, current_etag)) {
                    return true;
                }
            }
            seg_start = i + 1;
        }
        i = i + 1;
    }
    return false;
}

/// Configuration options for static file response planning.
pub struct SendOptions {
    pub root: string,
    pub dotfiles: u8,
    pub etag: bool,
    pub accept_ranges: bool,
    pub max_age_secs: u64,
    pub immutable: bool,

    pub static fn defaults(root: string): SendOptions {
        return SendOptions {
            root: root,
            dotfiles: DOTFILES_IGNORE,
            etag: true,
            accept_ranges: true,
            max_age_secs: 0 as u64,
            immutable: false
        };
    }
}

/// Computed HTTP transfer plan for serving a static file.
pub struct SendPlan {
    pub status: u16,
    pub content_type: string,
    pub is_partial: bool,
    pub range_start: u64,
    pub range_end: u64,
    pub content_length: u64,
    pub redirect_location_len: usize,

    pub static fn errorPlan(status: u16): SendPlan {
        return SendPlan {
            status: status,
            content_type: "",
            is_partial: false,
            range_start: 0 as u64,
            range_end: 0 as u64,
            content_length: 0 as u64,
            redirect_location_len: 0
        };
    }
}

/// Plans static file delivery: validates path safety & dotfiles policy, derives MIME type,
/// computes ETag + 304 negotiation, and evaluates HTTP Range (206 / 416).
pub fn planSendFile(
    req_path: string,
    file_size: u64,
    mtime_ms: u64,
    range_header: string,
    if_none_match: string,
    opts: SendOptions,
    etag_out: []u8
): SendPlan {
    // 1. Path traversal & NULL-byte protection
    if (sndHasMaliciousPath(req_path)) {
        return SendPlan.errorPlan(403 as u16);
    }

    // 2. Dotfiles policy check
    if (sndContainsDotfile(req_path)) {
        if (opts.dotfiles == DOTFILES_DENY) {
            return SendPlan.errorPlan(403 as u16);
        }
        if (opts.dotfiles == DOTFILES_IGNORE) {
            return SendPlan.errorPlan(404 as u16);
        }
    }

    // 3. MIME type derivation via `@cap/mime-types`
    let mime_opt = lookup(req_path);
    let mime: string = if (mime_opt != null) { mime_opt.? } else { "application/octet-stream" };

    // 4. ETag generation & If-None-Match 304 negotiation via `@cap/etag`
    if (opts.etag) {
        let etag_len: usize = generateStatEtag(file_size, mtime_ms, etag_out);
        if (etag_len > 0 && if_none_match.len > 0) {
            if (sndIfNoneMatchMatches(if_none_match, etag_out[0..etag_len])) {
                return SendPlan {
                    status: 304 as u16,
                    content_type: mime,
                    is_partial: false,
                    range_start: 0 as u64,
                    range_end: 0 as u64,
                    content_length: 0 as u64,
                    redirect_location_len: 0
                };
            }
        }
    }

    // 5. HTTP Range negotiation via `@cap/range-parser`
    if (opts.accept_ranges && range_header.len > 0) {
        let parsed = parseRange(file_size, range_header, true);
        if (parsed.status < (0 as i32)) {
            return SendPlan {
                status: 416 as u16,
                content_type: mime,
                is_partial: false,
                range_start: 0 as u64,
                range_end: 0 as u64,
                content_length: 0 as u64,
                redirect_location_len: 0
            };
        }
        if (parsed.status == RANGE_OK && parsed.count > 0 && sndStrEq(parsed.unit, "bytes")) {
            let r = parsed.ranges[0];
            let part_len: u64 = rangeByteLength(r);
            return SendPlan {
                status: 206 as u16,
                content_type: mime,
                is_partial: true,
                range_start: r.start,
                range_end: r.end,
                content_length: part_len,
                redirect_location_len: 0
            };
        }
    }

    // 6. Full 200 OK file response
    let full_end: u64 = if (file_size > (0 as u64)) { file_size - (1 as u64) } else { 0 as u64 };
    return SendPlan {
        status: 200 as u16,
        content_type: mime,
        is_partial: false,
        range_start: 0 as u64,
        range_end: full_end,
        content_length: file_size,
        redirect_location_len: 0
    };
}

/// Encodes a safe directory redirect URL (collapsing leading duplicate slashes and ensuring trailing `/`)
/// using `@cap/encodeurl`.
pub fn formatRedirectUrl(path: string, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    // Collapse multiple leading slashes (prevent protocol-relative open redirect like `//evil.com`)
    let start: usize = 0;
    while (start + 1 < path.len && path[start] == ('/' as u8) && path[start + 1] == ('/' as u8)) {
        start = start + 1;
    }
    let clean_path = path[start..path.len];
    let n: usize = encodeUrl(clean_path, out);
    if (n == 0) {
        out[0] = '/' as u8;
        return 1;
    }
    if (out[n - 1] != ('/' as u8) && n < out.len) {
        out[n] = '/' as u8;
        return n + 1;
    }
    return n;
}

export {
    DOTFILES_IGNORE,
    DOTFILES_ALLOW,
    DOTFILES_DENY,
    SendOptions,
    SendPlan,
    planSendFile,
    formatRedirectUrl
};

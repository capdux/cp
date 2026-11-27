// @cap/serve-static — Zero-heap static file HTTP request policy & response helper.
// Clean-room MIT port of npm `serve-static` v2 (130M+/week);
// composes `@cap/parseurl`, `@cap/mime-types`, `@cap/etag`, and `@cap/itoa`.

import { parseRequestUrl, isSafeRequestPath } from "../../parseurl/src/index.cp";
import { lookup, contentType } from "../../mime-types/src/index.cp";
import { generateStatEtag } from "../../etag/src/index.cp";
import { formatU64 } from "../../itoa/src/index.cp";

pub const DOTFILE_IGNORE: u8 = 0; // 404 Not Found
pub const DOTFILE_DENY: u8 = 1;   // 403 Forbidden
pub const DOTFILE_ALLOW: u8 = 2;  // 200 Allowed

pub const STATIC_ACTION_SERVE: u16 = 200;
pub const STATIC_ACTION_REDIRECT: u16 = 301;
pub const STATIC_ACTION_FORBIDDEN: u16 = 403;
pub const STATIC_ACTION_NOT_FOUND: u16 = 404;
pub const STATIC_ACTION_METHOD_NOT_ALLOWED: u16 = 405;

pub struct ServeStaticOptions {
    pub index_file: string,
    pub dotfiles: u8,
    pub max_age_secs: u64,
    pub immutable: bool,
    pub redirect_dirs: bool,

    pub static fn defaultOpts(): ServeStaticOptions {
        return ServeStaticOptions {
            index_file: "index.html",
            dotfiles: DOTFILE_IGNORE,
            max_age_secs: 0,
            immutable: false,
            redirect_dirs: true,
        };
    }
}

pub struct StaticResolution {
    pub status: u16,
    pub rel_path_len: usize,
    pub mime_type: string,
}

fn sstPush(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn sstIsGetOrHead(method: string): bool {
    if (method.len == 3 && method[0] == 71 && method[1] == 69 && method[2] == 84) { return true; } // GET
    if (method.len == 4 && method[0] == 72 && method[1] == 69 && method[2] == 65 && method[3] == 68) { return true; } // HEAD
    return false;
}

/// Checks if any path segment starts with `.` (e.g., `/.env`, `/a/.git/config`).
pub fn hasDotfileSegment(pathname: string): bool {
    let i: usize = 0;
    while (i < pathname.len) {
        if (pathname[i] == 46 && (i == 0 || pathname[i - 1] == 47)) {
            // Skip single "." segment; treat ".env" or ".." as dotfile/unsafe
            if (i + 1 < pathname.len && pathname[i + 1] != 47) {
                return true;
            }
        }
        i = i + 1;
    }
    return false;
}

/// Resolves an HTTP request `(method, raw_url)` against `opts`, writing the normalized
/// relative file path into `path_out` and returning the action status & guessed MIME type.
pub fn resolveStaticRequest(method: string, raw_url: string, opts: ServeStaticOptions, path_out: []u8): StaticResolution {
    if (!sstIsGetOrHead(method)) {
        return StaticResolution { status: STATIC_ACTION_METHOD_NOT_ALLOWED, rel_path_len: 0, mime_type: "" };
    }
    let parsed = parseRequestUrl(raw_url);
    let pathname = if (parsed.pathname.len == 0) { "/" } else { parsed.pathname };
    if (!isSafeRequestPath(pathname)) {
        return StaticResolution { status: STATIC_ACTION_FORBIDDEN, rel_path_len: 0, mime_type: "" };
    }
    if (hasDotfileSegment(pathname)) {
        if (opts.dotfiles == DOTFILE_DENY) {
            return StaticResolution { status: STATIC_ACTION_FORBIDDEN, rel_path_len: 0, mime_type: "" };
        }
        if (opts.dotfiles == DOTFILE_IGNORE) {
            return StaticResolution { status: STATIC_ACTION_NOT_FOUND, rel_path_len: 0, mime_type: "" };
        }
    }

    // Strip leading '/' to form relative path; append index_file on trailing '/'
    let rel = if (pathname.len > 0 && pathname[0] == 47) { pathname[1..pathname.len] } else { pathname };
    let p: usize = 0;
    p = sstPush(path_out, p, rel);
    if (pathname[pathname.len - 1] == 47 && opts.index_file.len > 0) {
        p = sstPush(path_out, p, opts.index_file);
    }

    let resolved_str = path_out[0..p] as string;
    let mime = lookup(resolved_str) ?? "application/octet-stream";
    return StaticResolution { status: STATIC_ACTION_SERVE, rel_path_len: p, mime_type: mime };
}

/// Formats a `Cache-Control` header value (`"public, max-age=3600, immutable"`) into `out`.
pub fn formatCacheControl(max_age_secs: u64, immutable: bool, out: []u8): usize {
    let p: usize = 0;
    p = sstPush(out, p, "public, max-age=");
    let tmp: [24]u8 = undefined;
    let n = formatU64(max_age_secs, tmp[0..24]);
    let i: usize = 0;
    while (i < n && p < out.len) {
        out[p] = tmp[i];
        p = p + 1;
        i = i + 1;
    }
    if (immutable) {
        p = sstPush(out, p, ", immutable");
    }
    return p;
}

/// Builds a weak stat ETag via `@cap/etag` and full Content-Type via `@cap/mime-types`.
pub fn formatStaticEtag(size_bytes: u64, mtime_ms: u64, out: []u8): usize {
    return generateStatEtag(size_bytes, mtime_ms, out);
}

pub fn formatStaticContentType(file_path: string, out: []u8): usize {
    let mime = lookup(file_path) ?? "application/octet-stream";
    return contentType(mime, out);
}

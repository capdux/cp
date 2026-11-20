// Source: /data/dev/bun/src/http/http.cp
// lib/http/redirect.cp
// WHATWG Fetch / RFC 9110 HTTP Redirect Policy Engine (cap:http)

const std = @import("std");
import { Method } from "./method";
import { Headers } from "./headers";
import { FetchRedirect } from "./fetch_modes";

pub const MAX_REDIRECT_HOPS: u8 = 20;
pub const MAX_REDIRECT_HISTORY: usize = 32;

pub enum RedirectAction: u8 {
    Follow = 0,
    Stop = 1,
    Error = 2,
}

pub struct ParsedHttpOrigin {
    pub scheme: []const u8 = "http",
    pub host: []const u8 = "",
    pub port: u16 = 80,
    pub path: []const u8 = "/",

    pub static fn defaultPort(scheme: []const u8): u16 {
        if (std.ascii.eqlIgnoreCase(scheme, "https")) return 443;
        return 80;
    }

    pub static fn parse(url: []const u8): ParsedHttpOrigin? {
        let rest = url;
        let scheme: []const u8 = "http";
        if (std.ascii.startsWithIgnoreCase(rest, "https://")) {
            scheme = "https";
            rest = rest[8..];
        } else if (std.ascii.startsWithIgnoreCase(rest, "http://")) {
            scheme = "http";
            rest = rest[7..];
        } else {
            return null;
        }

        let path_start: usize = rest.len;
        let i: usize = 0;
        while (i < rest.len) : (i += 1) {
            if (rest[i] == '/' || rest[i] == '?' || rest[i] == '#') {
                path_start = i;
                break;
            }
        }
        const authority = rest[0..path_start];
        if (authority.len == 0) return null;
        const path = if (path_start < rest.len) rest[path_start..] else "/";

        // Handle IPv6 [::1]:8080 or host:port
        let host = authority;
        let port = defaultPort(scheme);
        if (authority[0] == '[') {
            if (close_br := std.mem.indexOfScalar(u8, authority, ']')) {
                host = authority[0 .. close_br + 1];
                if (close_br + 1 < authority.len && authority[close_br + 1] == ':') {
                    port = std.fmt.parseInt(u16, authority[close_br + 2 ..], 10) catch return null;
                }
            } else {
                return null;
            }
        } else if (colon := std.mem.lastIndexOfScalar(u8, authority, ':')) {
            host = authority[0..colon];
            port = std.fmt.parseInt(u16, authority[colon + 1 ..], 10) catch return null;
        }

        return ParsedHttpOrigin{
            scheme: scheme,
            host: host,
            port: port,
            path: path,
        };
    }

    pub fn isSameOrigin(other: ParsedHttpOrigin): bool {
        return std.ascii.eqlIgnoreCase(_.scheme, other.scheme) &&
            std.ascii.eqlIgnoreCase(_.host, other.host) &&
            _.port == other.port;
    }
}

pub struct RedirectDecision {
    pub action: RedirectAction = RedirectAction.Stop,
    pub next_method: Method = Method.GET,
    pub next_url: []const u8 = "",
    pub keep_body: bool = true,
    pub strip_auth_headers: bool = false,
    pub strip_body_headers: bool = false,
    pub is_same_origin: bool = true,
    pub err_reason: ([]const u8)? = null,
}

pub struct RedirectPolicy {
    pub mode: FetchRedirect = FetchRedirect.Follow,
    pub max_redirects: u8 = MAX_REDIRECT_HOPS,
    pub redirect_count: u8 = 0,
    pub history: [MAX_REDIRECT_HISTORY]u64 = undefined,
    pub history_len: usize = 0,

    pub static fn init(mode: FetchRedirect, max_redirects: u8 = MAX_REDIRECT_HOPS): RedirectPolicy {
        return RedirectPolicy{
            mode: mode,
            max_redirects: max_redirects,
            redirect_count: 0,
            history: undefined,
            history_len: 0,
        };
    }

    pub static fn initDefault(): RedirectPolicy {
        return RedirectPolicy.init(FetchRedirect.Follow, MAX_REDIRECT_HOPS);
    }

    pub static fn isRedirectStatus(status: u16): bool {
        return status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
    }

    pub fn reset(*_) {
        _.redirect_count = 0;
        _.history_len = 0;
    }

    fn hashUrl(url: []const u8): u64 {
        return std.hash.Wyhash.hash(0, url);
    }

    fn hasVisited(url: []const u8): bool {
        const h = hashUrl(url);
        let i: usize = 0;
        while (i < _.history_len) : (i += 1) {
            if (_.history[i] == h) return true;
        }
        return false;
    }

    fn recordVisited(*_, url: []const u8) {
        if (_.history_len < MAX_REDIRECT_HISTORY) {
            _.history[_.history_len] = hashUrl(url);
            _.history_len += 1;
        }
    }

    /// Resolves `location` against `base_url` into `out_buf` (handles absolute, protocol-relative `//`, root `/`, relative paths, `.`/`..`, and `?query`).
    pub static fn resolveLocation(base_url: []const u8, location: []const u8, out_buf: []u8): ([]const u8)! {
        if (location.len == 0) return error.InvalidRedirectURL;
        const base = ParsedHttpOrigin.parse(base_url) ?? return error.InvalidBaseURL;

        // 1. Absolute URL with scheme
        if (scheme_sep := std.mem.indexOf(u8, location, "://")) {
            const scheme = location[0..scheme_sep];
            if (!std.ascii.eqlIgnoreCase(scheme, "http") && !std.ascii.eqlIgnoreCase(scheme, "https")) {
                return error.UnsupportedRedirectProtocol;
            }
            if (location.len > out_buf.len) return error.BufferTooSmall;
            @memcpy(out_buf[0..location.len], location);
            return out_buf[0..location.len];
        }

        // 2. Protocol-relative //host/path
        if (std.mem.startsWith(u8, location, "//")) {
            const total = base.scheme.len + 1 + location.len;
            if (total > out_buf.len) return error.BufferTooSmall;
            @memcpy(out_buf[0..base.scheme.len], base.scheme);
            out_buf[base.scheme.len] = ':';
            @memcpy(out_buf[base.scheme.len + 1 .. total], location);
            return out_buf[0..total];
        }

        // Build origin prefix: scheme://host[:port]
        let pos: usize = 0;
        const has_explicit_port = base.port != ParsedHttpOrigin.defaultPort(base.scheme);
        const origin_str = if (has_explicit_port)
            std.fmt.bufPrint(out_buf[pos..], "{s}://{s}:{d}", .{ base.scheme, base.host, base.port }) catch return error.BufferTooSmall
        else
            std.fmt.bufPrint(out_buf[pos..], "{s}://{s}", .{ base.scheme, base.host }) catch return error.BufferTooSmall;
        pos += origin_str.len;

        // Strip query/fragment from base.path for path resolution
        let base_path_only = base.path;
        if (q := std.mem.indexOfAny(u8, base_path_only, "?#")) {
            base_path_only = base_path_only[0..q];
        }
        if (base_path_only.len == 0) base_path_only = "/";

        // 3. Root-relative /path
        if (location[0] == '/') {
            if (pos + location.len > out_buf.len) return error.BufferTooSmall;
            @memcpy(out_buf[pos .. pos + location.len], location);
            return out_buf[0 .. pos + location.len];
        }

        // 4. Query-only ?foo=bar
        if (location[0] == '?') {
            if (pos + base_path_only.len + location.len > out_buf.len) return error.BufferTooSmall;
            @memcpy(out_buf[pos .. pos + base_path_only.len], base_path_only);
            pos += base_path_only.len;
            @memcpy(out_buf[pos .. pos + location.len], location);
            return out_buf[0 .. pos + location.len];
        }

        // 5. Relative path (e.g. "next", "./a/b", "../up")
        let dir_end: usize = 0;
        if (last_slash := std.mem.lastIndexOfScalar(u8, base_path_only, '/')) {
            dir_end = last_slash + 1;
        }
        const base_dir = base_path_only[0..dir_end];

        // Split location into path and optional ?query/#fragment
        let loc_path = location;
        let loc_suffix: []const u8 = "";
        if (q2 := std.mem.indexOfAny(u8, location, "?#")) {
            loc_path = location[0..q2];
            loc_suffix = location[q2..];
        }

        // Normalize dot segments using stack segment array
        let segs: [32][]const u8 = undefined;
        let seg_count: usize = 0;

        let base_iter = std.mem.splitScalar(u8, base_dir, '/');
        while (seg := base_iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
            if (std.mem.eql(u8, seg, "..")) {
                if (seg_count > 0) seg_count -= 1;
            } else if (seg_count < 32) {
                segs[seg_count] = seg;
                seg_count += 1;
            }
        }

        let loc_iter = std.mem.splitScalar(u8, loc_path, '/');
        while (seg := loc_iter.next()) {
            if (seg.len == 0 || std.mem.eql(u8, seg, ".")) continue;
            if (std.mem.eql(u8, seg, "..")) {
                if (seg_count > 0) seg_count -= 1;
            } else if (seg_count < 32) {
                segs[seg_count] = seg;
                seg_count += 1;
            }
        }

        if (seg_count == 0) {
            if (pos + 1 > out_buf.len) return error.BufferTooSmall;
            out_buf[pos] = '/';
            pos += 1;
        } else {
            let si: usize = 0;
            while (si < seg_count) : (si += 1) {
                const s = segs[si];
                if (pos + 1 + s.len > out_buf.len) return error.BufferTooSmall;
                out_buf[pos] = '/';
                pos += 1;
                @memcpy(out_buf[pos .. pos + s.len], s);
                pos += s.len;
            }
            if (loc_path.len > 0 && loc_path[loc_path.len - 1] == '/') {
                if (pos + 1 > out_buf.len) return error.BufferTooSmall;
                out_buf[pos] = '/';
                pos += 1;
            }
        }

        if (loc_suffix.len > 0) {
            if (pos + loc_suffix.len > out_buf.len) return error.BufferTooSmall;
            @memcpy(out_buf[pos .. pos + loc_suffix.len], loc_suffix);
            pos += loc_suffix.len;
        }

        return out_buf[0..pos];
    }

    /// Evaluates a redirect response and returns the next method, resolved URL, body retention, and header stripping flags.
    pub fn evaluate(
        *_,
        status: u16,
        current_method: Method,
        current_url: []const u8,
        location: []const u8,
        out_url_buf: []u8,
    ): RedirectDecision {
        if (!isRedirectStatus(status)) {
            return RedirectDecision{
                action: RedirectAction.Stop,
                next_method: current_method,
                next_url: current_url,
            };
        }

        if (_.mode == FetchRedirect.Manual) {
            return RedirectDecision{
                action: RedirectAction.Stop,
                next_method: current_method,
                next_url: current_url,
            };
        }

        if (_.mode == FetchRedirect.Error) {
            return RedirectDecision{
                action: RedirectAction.Error,
                next_method: current_method,
                next_url: current_url,
                err_reason: "Redirect disallowed by FetchRedirect.Error",
            };
        }

        if (_.redirect_count >= _.max_redirects) {
            return RedirectDecision{
                action: RedirectAction.Error,
                next_method: current_method,
                next_url: current_url,
                err_reason: "Max redirect count exceeded",
            };
        }

        if (_.history_len == 0) {
            _.recordVisited(current_url);
        }

        const resolved_url = resolveLocation(current_url, location, out_url_buf) catch {
            return RedirectDecision{
                action: RedirectAction.Error,
                next_method: current_method,
                next_url: current_url,
                err_reason: "Invalid or unsupported redirect Location URL",
            };
        };

        if (_.hasVisited(resolved_url)) {
            return RedirectDecision{
                action: RedirectAction.Error,
                next_method: current_method,
                next_url: resolved_url,
                err_reason: "Redirect loop detected",
            };
        }

        _.recordVisited(resolved_url);
        _.redirect_count += 1;

        let same_origin = true;
        if (orig := ParsedHttpOrigin.parse(current_url)) {
            if (next_orig := ParsedHttpOrigin.parse(resolved_url)) {
                same_origin = orig.isSameOrigin(next_orig);
            }
        }

        // Method rewrite & body preservation rules per WHATWG Fetch / RFC 9110:
        // - 301 / 302: POST -> GET (drop body); other methods stay unchanged with body
        // - 303: anything except GET/HEAD -> GET (drop body)
        // - 307 / 308: method & body strictly preserved
        let next_method = current_method;
        let keep_body = current_method.hasRequestBody();
        let strip_body_headers = false;

        if ((status == 301 || status == 302) && current_method == Method.POST) {
            next_method = Method.GET;
            keep_body = false;
            strip_body_headers = true;
        } else if (status == 303 && current_method != Method.GET && current_method != Method.HEAD) {
            next_method = Method.GET;
            keep_body = false;
            strip_body_headers = true;
        }

        return RedirectDecision{
            action: RedirectAction.Follow,
            next_method: next_method,
            next_url: resolved_url,
            keep_body: keep_body,
            strip_auth_headers: !same_origin,
            strip_body_headers: strip_body_headers,
            is_same_origin: same_origin,
            err_reason: null,
        };
    }

    /// Applies cross-origin credential stripping and body-header stripping to `headers`.
    pub static fn applyHeaderPolicy(headers: *Headers, decision: RedirectDecision) {
        if (decision.strip_auth_headers) {
            headers.delete("Authorization");
            headers.delete("Cookie");
            headers.delete("Proxy-Authorization");
        }
        if (decision.strip_body_headers) {
            headers.delete("Content-Type");
            headers.delete("Content-Length");
            headers.delete("Content-Encoding");
            headers.delete("Content-Language");
            headers.delete("Content-Location");
        }
    }
}

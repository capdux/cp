// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T10:56:52Z
// ============================================================================
// lib/url/url.cp — WHATWG URL Parser & Representation
// ============================================================================
import { URLSearchParams, QueryParam } from "./search_params";
import { PercentEncoding } from "./percent";
import {
    findSubstr,
    urlIndexOfChar,
    urlLastIndexOfChar,
    isIPHostname,
    isLocalhostName,
    parsePortNumber,
} from "./url_";
const std = @import("std");

pub struct URL {
    pub href: string = "",
    pub protocol: string = "",
    pub scheme: string = "",
    pub username: string = "",
    pub password: string = "",
    pub host: string = "",
    pub hostname: string = "",
    pub port: string = "",
    pub pathname: string = "/",
    pub path: string = "/",
    pub search: string = "",
    pub hash: string = "",
    pub origin: string = "",
    pub searchParams: URLSearchParams = URLSearchParams{},
    pub allocated_origin: string? = null,
    pub allocator: std.mem.Allocator? = null,

    pub fn isFile(): bool {
        return std.mem.eql(u8, _.protocol, "file:") or std.mem.eql(u8, _.protocol, "file");
    }

    pub fn isBlob(): bool {
        return std.mem.eql(u8, _.protocol, "blob:") or std.mem.eql(u8, _.protocol, "blob");
    }

    pub fn isHTTPS(): bool {
        return std.mem.eql(u8, _.protocol, "https:") or std.mem.eql(u8, _.protocol, "https");
    }

    pub fn isHTTP(): bool {
        return std.mem.eql(u8, _.protocol, "http:") or std.mem.eql(u8, _.protocol, "http");
    }

    pub fn isS3(): bool {
        return std.mem.eql(u8, _.protocol, "s3:") or std.mem.eql(u8, _.protocol, "s3");
    }

    pub fn isUnix(): bool {
        return _.protocol.len >= 4 and _.protocol[0] == (117 as u8) and _.protocol[1] == (110 as u8) and
            _.protocol[2] == (105 as u8) and _.protocol[3] == (120 as u8); // "unix"
    }

    pub fn hasHTTPLikeProtocol(): bool {
        return _.isHTTP() or _.isHTTPS();
    }

    pub fn isLocalhost(): bool {
        return isLocalhostName(_.hostname);
    }

    pub fn isIPAddress(): bool {
        return isIPHostname(_.hostname);
    }

    pub fn hasValidPort(): bool {
        if (p := _.getPort()) {
            return p > 0;
        }
        return false;
    }

    pub fn isEmpty(): bool {
        return _.href.len == 0;
    }

    pub fn isAbsolute(): bool {
        return _.hostname.len > 0 and _.pathname.len > 0;
    }

    pub fn displayProtocol(): string {
        if (_.protocol.len > 0) {
            if (_.protocol[_.protocol.len - 1] == (58 as u8)) { // ':'
                return _.protocol[0 .. _.protocol.len - 1];
            }
            return _.protocol;
        }
        if (p := _.getPort()) {
            if (p == 443) return "https";
        }
        return "http";
    }

    pub fn getScheme(): string {
        if (_.scheme.len > 0) return _.scheme;
        return _.displayProtocol();
    }

    pub fn displayHostname(): string {
        if (_.hostname.len > 0) return _.hostname;
        return "localhost";
    }

    pub fn displayHost(): string {
        if (_.host.len > 0) return _.host;
        return _.displayHostname();
    }

    pub fn getPort(): u16? {
        return parsePortNumber(_.port);
    }

    pub fn getPortAuto(): u16 {
        if (p := _.getPort()) {
            return p;
        }
        return _.getDefaultPort();
    }

    pub fn getDefaultPort(): u16 {
        if (_.isHTTPS()) return 443 as u16;
        return 80 as u16;
    }

    /// Returns `host + pathname` without trailing slash, protocol, search, or hash.
    pub fn hostWithPath(): string {
        if (_.host.len == 0) return "";
        if (_.pathname.len <= 1) return _.host;
        if (host_idx := findSubstr(_.href, _.host)) {
            let end_idx = host_idx + _.host.len + _.pathname.len;
            if (end_idx <= _.href.len) {
                if (_.href[end_idx - 1] == (47 as u8)) { // '/'
                    end_idx -= 1;
                }
                return _.href[host_idx..end_idx];
            }
        }
        return _.host;
    }

    pub fn s3Path(): string {
        let h = _.href;
        if (_.protocol.len > 0 and h.len > _.protocol.len + 2) {
            h = h[_.protocol.len + 2 ..];
        }
        let end = h.len;
        if (_.search.len > 0) {
            end -= _.search.len;
        }
        if (_.hash.len > 0) {
            end -= _.hash.len;
        }
        return h[0..end];
    }

    pub static fn joinNormalize(
        out: []u8,
        prefix: string,
        dirname: string,
        basename: string,
        extname: string
    ): string {
        let pos: usize = 0;
        if (prefix.len > 0 and pos + prefix.len <= out.len) {
            if (prefix[0] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            @memcpy(out[pos .. pos + prefix.len], prefix);
            pos += prefix.len;
        }
        if (dirname.len > 0) {
            if (pos > 0 and out[pos - 1] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            let d = dirname;
            if (d.len > 0 and d[0] == (47 as u8)) d = d[1..];
            if (pos + d.len <= out.len) {
                @memcpy(out[pos .. pos + d.len], d);
                pos += d.len;
            }
        }
        if (basename.len > 0) {
            if (pos > 0 and out[pos - 1] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            let b = basename;
            if (b.len > 0 and b[0] == (47 as u8)) b = b[1..];
            if (pos + b.len <= out.len) {
                @memcpy(out[pos .. pos + b.len], b);
                pos += b.len;
            }
        }
        if (extname.len > 0 and pos + extname.len <= out.len) {
            @memcpy(out[pos .. pos + extname.len], extname);
            pos += extname.len;
        }
        if (pos == 0) {
            out[0] = 47 as u8;
            return out[0..1];
        }
        return out[0..pos];
    }

    /// Zero-allocation slice parser (equivalent to `URL.parse(input, null)`).
    pub static fn parseSlice(input: string): URL {
        return URL.parse(input, null) catch URL{};
    }

    /// Full WHATWG URL parsing engine
    pub static fn parse(input: string, alloc: std.mem.Allocator?): !URL {
        let url = URL{
            .href = input,
            .searchParams = URLSearchParams.init(alloc),
            .allocator = alloc,
        };

        if (input.len == 0) return url;

        let rem = input;

        // 1. Protocol scheme (e.g. "https://", "http://", "file://", "s3://", "blob:", "mailto:")
        let c_opt = urlIndexOfChar(rem, 58 as u8); // ':'
        if (c_idx := c_opt) {
            let is_scheme = true;
            let k: usize = 0;
            while (k < c_idx) {
                let ch = rem[k];
                if (ch == (47 as u8) or ch == (63 as u8) or ch == (35 as u8) or ch == (64 as u8)) { // '/', '?', '#', '@'
                    is_scheme = false;
                    break;
                }
                k += 1;
            }
            if (is_scheme and c_idx > 0) {
                url.protocol = input[0 .. c_idx + 1];
                url.scheme = input[0..c_idx];
                let after_colon = rem[c_idx + 1 ..];
                if (after_colon.len >= 2 and after_colon[0] == (47 as u8) and after_colon[1] == (47 as u8)) {
                    rem = after_colon[2..]; // skip "://"
                } else {
                    rem = after_colon; // e.g. "blob:", "mailto:"
                }
            }
        }

        // 2. Hash fragment ("#...")
        if (h_idx := urlIndexOfChar(rem, 35 as u8)) { // '#'
            url.hash = rem[h_idx..];
            rem = rem[0..h_idx];
        }

        // 3. Search query string ("?...")
        if (q_idx := urlIndexOfChar(rem, 63 as u8)) { // '?'
            url.search = rem[q_idx..];
            url.searchParams = URLSearchParams.parse(url.search, alloc);
            rem = rem[0..q_idx];
        }

        // 4. Pathname separation ("/")
        let path_start = urlIndexOfChar(rem, 47 as u8); // '/'
        let authority = rem;
        if (p_start := path_start) {
            url.pathname = rem[p_start..];
            authority = rem[0..p_start];
        } else {
            url.pathname = "/";
            authority = rem;
        }
        url.path = url.pathname;

        // 5. Userinfo ("user:pass@host")
        if (at_idx := urlIndexOfChar(authority, 64 as u8)) { // '@'
            let userinfo = authority[0..at_idx];
            authority = authority[at_idx + 1 ..];

            if (colon_idx := urlIndexOfChar(userinfo, 58 as u8)) { // ':'
                url.username = userinfo[0..colon_idx];
                url.password = userinfo[colon_idx + 1 ..];
            } else {
                url.username = userinfo;
                url.password = "";
            }
        }

        // 6. Host & Port (handles IPv6 "[::1]:8080" and IPv4/domain "host:port")
        url.host = authority;
        if (authority.len > 0 and authority[0] == (91 as u8)) { // '[' for IPv6
            if (close_idx := urlIndexOfChar(authority, 93 as u8)) { // ']'
                url.hostname = authority[0 .. close_idx + 1];
                let after_bracket = authority[close_idx + 1 ..];
                if (after_bracket.len > 1 and after_bracket[0] == (58 as u8)) { // ':'
                    url.port = after_bracket[1..];
                } else {
                    url.port = "";
                }
            } else {
                url.hostname = authority;
                url.port = "";
            }
        } else {
            if (port_idx := urlLastIndexOfChar(authority, 58 as u8)) { // ':'
                url.hostname = authority[0..port_idx];
                url.port = authority[port_idx + 1 ..];
            } else {
                url.hostname = authority;
                url.port = "";
            }
        }

        // 7. Origin formatting
        if (url.isFile()) {
            url.origin = "null";
        } else if (url.protocol.len > 0 and url.host.len > 0) {
            if (url.username.len == 0 and url.password.len == 0) {
                // Slice directly from input: "protocol//host"
                let proto_host_end = url.protocol.len + 2 + url.host.len;
                if (proto_host_end <= input.len and std.mem.eql(u8, input[url.protocol.len + 2 .. proto_host_end], url.host)) {
                    url.origin = input[0..proto_host_end];
                } else if (a := alloc) {
                    url.origin = std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host })?;
                    url.allocated_origin = url.origin;
                }
            } else if (a := alloc) {
                url.origin = std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host })?;
                url.allocated_origin = url.origin;
            }
        }

        return url;
    }

    pub fn deinit(*_) {
        _.searchParams.deinit();
        if (orig := _.allocated_origin) {
            if (alloc := _.allocator) {
                alloc.free(orig);
            }
            _.allocated_origin = null;
        }
    }

    /// Formats the URL origin (`scheme://host` or `"null"` for file://) into `out` without heap allocation.
    pub fn formatOrigin(out: []u8): string {
        if (_.isFile()) {
            const null_str = "null";
            if (out.len < null_str.len) return "";
            @memcpy(out[0..null_str.len], null_str);
            return out[0..null_str.len];
        }
        if (_.protocol.len == 0 or _.host.len == 0) return "";
        const needed = _.protocol.len + 2 + _.host.len;
        if (out.len < needed) return "";
        @memcpy(out[0.._.protocol.len], _.protocol);
        out[_.protocol.len] = 47 as u8;
        out[_.protocol.len + 1] = 47 as u8;
        @memcpy(out[_.protocol.len + 2 .. needed], _.host);
        return out[0..needed];
    }

    /// Serializes the full URL back into `out` without heap allocation.
    pub fn format(out: []u8): string {
        let pos: usize = 0;
        if (_.protocol.len > 0) {
            if (pos + _.protocol.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.protocol.len], _.protocol);
            pos += _.protocol.len;
            if (_.host.len > 0 or _.isFile()) {
                if (pos + 2 > out.len) return out[0..pos];
                out[pos] = 47 as u8;
                out[pos + 1] = 47 as u8;
                pos += 2;
            }
        }
        if (_.username.len > 0) {
            if (pos + _.username.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.username.len], _.username);
            pos += _.username.len;
            if (_.password.len > 0) {
                if (pos + 1 + _.password.len > out.len) return out[0..pos];
                out[pos] = 58 as u8; // ':'
                pos += 1;
                @memcpy(out[pos .. pos + _.password.len], _.password);
                pos += _.password.len;
            }
            if (pos + 1 > out.len) return out[0..pos];
            out[pos] = 64 as u8; // '@'
            pos += 1;
        }
        if (_.host.len > 0) {
            if (pos + _.host.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.host.len], _.host);
            pos += _.host.len;
        }
        if (_.pathname.len > 0) {
            if (pos + _.pathname.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.pathname.len], _.pathname);
            pos += _.pathname.len;
        }
        if (_.search.len > 0) {
            if (pos + _.search.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.search.len], _.search);
            pos += _.search.len;
        }
        if (_.hash.len > 0) {
            if (pos + _.hash.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.hash.len], _.hash);
            pos += _.hash.len;
        }
        return out[0..pos];
    }

    /// RFC 3986 Section 5.2.4 dot-segment normalization (resolves `.` and `..` in `path`).
    pub static fn normalizePath(path: string, out: []u8): string {
        if (out.len == 0) return "";
        if (path.len == 0) {
            out[0] = 47 as u8;
            return out[0..1];
        }
        let seg_starts: [64]usize = [0; 64];
        let seg_lens: [64]usize = [0; 64];
        let seg_count: usize = 0;

        let trailing_slash = path[path.len - 1] == (47 as u8);
        let i: usize = 0;
        while (i <= path.len) {
            let start = i;
            while (i < path.len and path[i] != (47 as u8)) {
                i += 1;
            }
            const seg = path[start..i];
            if (seg.len == 0 or std.mem.eql(u8, seg, ".")) {
                // skip empty or "."
            } else if (std.mem.eql(u8, seg, "..")) {
                if (seg_count > 0) {
                    seg_count -= 1;
                }
            } else {
                if (seg_count < 64) {
                    seg_starts[seg_count] = start;
                    seg_lens[seg_count] = seg.len;
                    seg_count += 1;
                }
            }
            if (i < path.len) {
                i += 1; // skip '/'
            } else {
                break;
            }
        }

        let pos: usize = 0;
        if (seg_count == 0) {
            out[0] = 47 as u8;
            return out[0..1];
        }
        let s: usize = 0;
        while (s < seg_count) {
            if (pos >= out.len) break;
            out[pos] = 47 as u8;
            pos += 1;
            const slen = seg_lens[s];
            const sstart = seg_starts[s];
            if (pos + slen > out.len) break;
            @memcpy(out[pos .. pos + slen], path[sstart .. sstart + slen]);
            pos += slen;
            s += 1;
        }
        if (trailing_slash and pos < out.len and (pos == 0 or out[pos - 1] != (47 as u8))) {
            out[pos] = 47 as u8;
            pos += 1;
        }
        return out[0..pos];
    }

    /// Resolves a relative or absolute URL reference `relative_str` against `base_str` into `out` (RFC 3986 §5.2).
    pub static fn resolve(base_str: string, relative_str: string, out: []u8): string? {
        if (relative_str.len == 0) {
            if (base_str.len > out.len) return null;
            @memcpy(out[0..base_str.len], base_str);
            return out[0..base_str.len];
        }

        // Check if relative_str already has a scheme (e.g. "https://...")
        if (c_idx := urlIndexOfChar(relative_str, 58 as u8)) {
            let is_scheme = c_idx > 0;
            let k: usize = 0;
            while (k < c_idx) {
                const ch = relative_str[k];
                if (ch == (47 as u8) or ch == (63 as u8) or ch == (35 as u8) or ch == (64 as u8)) {
                    is_scheme = false;
                    break;
                }
                k += 1;
            }
            if (is_scheme) {
                if (relative_str.len > out.len) return null;
                @memcpy(out[0..relative_str.len], relative_str);
                return out[0..relative_str.len];
            }
        }

        const base = URL.parseSlice(base_str);
        if (base.protocol.len == 0) return null;

        // Protocol-relative "//host/path..."
        if (relative_str.len >= 2 and relative_str[0] == (47 as u8) and relative_str[1] == (47 as u8)) {
            const needed = base.protocol.len + relative_str.len;
            if (needed > out.len) return null;
            @memcpy(out[0..base.protocol.len], base.protocol);
            @memcpy(out[base.protocol.len .. needed], relative_str);
            return out[0..needed];
        }

        // Split relative_str into path, ?search, #hash
        let rel = relative_str;
        let rel_hash: string = "";
        if (h_idx := urlIndexOfChar(rel, 35 as u8)) {
            rel_hash = rel[h_idx..];
            rel = rel[0..h_idx];
        }
        let rel_search: string = "";
        if (q_idx := urlIndexOfChar(rel, 63 as u8)) {
            rel_search = rel[q_idx..];
            rel = rel[0..q_idx];
        }

        let norm_path_buf: [1024]u8 = undefined;
        let final_path: string = base.pathname;
        let final_search: string = rel_search;

        if (rel.len == 0) {
            if (rel_search.len == 0) {
                final_search = base.search;
            }
        } else if (rel[0] == (47 as u8)) {
            final_path = URL.normalizePath(rel, &norm_path_buf);
        } else {
            // Merge relative path with base directory
            let merged_buf: [1024]u8 = undefined;
            let m_pos: usize = 0;
            if (last_sl := urlLastIndexOfChar(base.pathname, 47 as u8)) {
                const dir_len = last_sl + 1;
                if (dir_len <= merged_buf.len) {
                    @memcpy(merged_buf[0..dir_len], base.pathname[0..dir_len]);
                    m_pos = dir_len;
                }
            } else {
                merged_buf[0] = 47 as u8;
                m_pos = 1;
            }
            if (m_pos + rel.len > merged_buf.len) return null;
            @memcpy(merged_buf[m_pos .. m_pos + rel.len], rel);
            m_pos += rel.len;
            final_path = URL.normalizePath(merged_buf[0..m_pos], &norm_path_buf);
        }

        let resolved = URL{
            .protocol = base.protocol,
            .scheme = base.scheme,
            .username = base.username,
            .password = base.password,
            .host = base.host,
            .hostname = base.hostname,
            .port = base.port,
            .pathname = final_path,
            .path = final_path,
            .search = final_search,
            .hash = rel_hash,
        };
        return resolved.format(out);
    }

    /// Resolves `input` against `base_str` into `out_buf` and parses the resulting `URL`.
    pub static fn parseWithBase(input: string, base_str: string, alloc: std.mem.Allocator?, out_buf: []u8): !URL {
        const resolved = URL.resolve(base_str, input, out_buf) orelse return err.InvalidUrl;
        return URL.parse(resolved, alloc);
    }
}

pub const Url = URL;

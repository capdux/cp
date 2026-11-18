// Source: bun/src/url/url.cp
// ============================================================================
// lib/url/url.cp — WHATWG URL Parser & Representation
// ============================================================================
import { URLSearchParams, QueryParam } from "./search_params";
import { PercentEncoding } from "./percent";
const std = @import("std");

fn findSubstr(haystack: string, needle: []const u8): usize? {
    if (needle.len == 0) return 0 as usize;
    if (haystack.len < needle.len) return null;
    let max_idx = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= max_idx) {
        let is_match = true;
        let j: usize = 0;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                is_match = false;
                break;
            }
            j += 1;
        }
        if (is_match) return i;
        i += 1;
    }
    return null;
}

fn indexOfChar(s: string, c: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == c) return i;
        i += 1;
    }
    return null;
}

fn lastIndexOfChar(s: string, c: u8): usize? {
    let i = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == c) return i;
    }
    return null;
}

pub struct URL {
    pub href: string = "",
    pub protocol: string = "",
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
        let h = _.hostname;
        return h.len == 0 or
            std.mem.eql(u8, h, "localhost") or
            std.mem.eql(u8, h, "127.0.0.1") or
            std.mem.eql(u8, h, "0.0.0.0") or
            std.mem.eql(u8, h, "[::1]");
    }

    pub fn isIPAddress(): bool {
        let h = _.hostname;
        if (h.len >= 2 and h[0] == (91 as u8) and h[h.len - 1] == (93 as u8)) { // '[' and ']'
            return true;
        }
        let dots: usize = 0;
        let i: usize = 0;
        let has_digits = false;
        while (i < h.len) {
            let c = h[i];
            if (c == (46 as u8)) { // '.'
                if (!has_digits) return false;
                dots += 1;
                has_digits = false;
            } else if (c >= (48 as u8) and c <= (57 as u8)) {
                has_digits = true;
            } else {
                return false;
            }
            i += 1;
        }
        return dots == 3 and has_digits;
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

    pub fn displayHostname(): string {
        if (_.hostname.len > 0) return _.hostname;
        return "localhost";
    }

    pub fn displayHost(): string {
        if (_.host.len > 0) return _.host;
        return _.displayHostname();
    }

    pub fn getPort(): u16? {
        if (_.port.len == 0) return null;
        let p: u32 = 0;
        let i: usize = 0;
        while (i < _.port.len) {
            let c = _.port[i];
            if (c < (48 as u8) or c > (57 as u8)) return null;
            p = p * 10 + ((c - (48 as u8)) as u32);
            if (p > 65535) return null;
            i += 1;
        }
        return (p as u16);
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

    pub fn hostWithPath(): string {
        if (_.host.len == 0) return "";
        let path_clean = _.pathname;
        if (path_clean.len > 1 and path_clean[path_clean.len - 1] == (47 as u8)) {
            path_clean = path_clean[0 .. path_clean.len - 1];
        }
        return path_clean;
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
        let c_opt = indexOfChar(rem, 58 as u8); // ':'
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
                let after_colon = rem[c_idx + 1 ..];
                if (after_colon.len >= 2 and after_colon[0] == (47 as u8) and after_colon[1] == (47 as u8)) {
                    rem = after_colon[2..]; // skip "://"
                } else {
                    rem = after_colon; // e.g. "blob:", "mailto:"
                }
            }
        }

        // 2. Hash fragment ("#...")
        if (h_idx := indexOfChar(rem, 35 as u8)) { // '#'
            url.hash = rem[h_idx..];
            rem = rem[0..h_idx];
        }

        // 3. Search query string ("?...")
        if (q_idx := indexOfChar(rem, 63 as u8)) { // '?'
            url.search = rem[q_idx..];
            url.searchParams = URLSearchParams.parse(url.search, alloc);
            rem = rem[0..q_idx];
        }

        // 4. Pathname separation ("/")
        let path_start = indexOfChar(rem, 47 as u8); // '/'
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
        if (at_idx := indexOfChar(authority, 64 as u8)) { // '@'
            let userinfo = authority[0..at_idx];
            authority = authority[at_idx + 1 ..];

            if (colon_idx := indexOfChar(userinfo, 58 as u8)) { // ':'
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
            if (close_idx := indexOfChar(authority, 93 as u8)) { // ']'
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
            if (port_idx := lastIndexOfChar(authority, 58 as u8)) { // ':'
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
                    url.origin = try std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host });
                    url.allocated_origin = url.origin;
                }
            } else if (a := alloc) {
                url.origin = try std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host });
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
}

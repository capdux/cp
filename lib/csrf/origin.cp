// Source: cp/lib/csrf/origin.cp (Origin, Referer, and Sec-Fetch-Site verification)
// Pure CP CSRF origin, referer, sec-fetch-site, and safe HTTP method validation.

const std = @import("std");
import { CsrfError } from "./types.cp";

pub struct ParsedOrigin {
    scheme: []const u8,
    host: []const u8,
    port: u16,
}

pub fn isSafeMethod(method: []const u8): bool {
    return std.ascii.eqlIgnoreCase(method, "GET") or
        std.ascii.eqlIgnoreCase(method, "HEAD") or
        std.ascii.eqlIgnoreCase(method, "OPTIONS") or
        std.ascii.eqlIgnoreCase(method, "TRACE");
}

fn defaultPortForScheme(scheme: []const u8): u16? {
    if (std.ascii.eqlIgnoreCase(scheme, "http")) {
        return 80;
    }
    if (std.ascii.eqlIgnoreCase(scheme, "https")) {
        return 443;
    }
    return null;
}

fn parsePort(port_str: []const u8): u16? {
    if (port_str.len == 0 or port_str.len > 5) {
        return null;
    }
    const port = std.fmt.parseInt(u16, port_str, 10) catch return null;
    if (port == 0) {
        return null;
    }
    return port;
}

pub fn parseOrigin(url_str: []const u8): ParsedOrigin? {
    if (url_str.len == 0) {
        return null;
    }

    const scheme_sep = std.mem.indexOf(u8, url_str, "://") ?? return null;
    const scheme = url_str[0..scheme_sep];
    const def_port = defaultPortForScheme(scheme) ?? return null;

    const rest = url_str[scheme_sep + 3 ..];
    if (rest.len == 0) {
        return null;
    }

    let auth_end: usize = rest.len;
    let i: usize = 0;
    while (i < rest.len) {
        const c = rest[i];
        if (c == '/' or c == '?' or c == '#') {
            auth_end = i;
            break;
        }
        if (c <= ' ' or c == '\\') {
            return null;
        }
        i += 1;
    }

    const authority = rest[0..auth_end];
    if (authority.len == 0) {
        return null;
    }
    if (std.mem.indexOfScalar(u8, authority, '@') != null) {
        return null;
    }

    if (authority[0] == '[') {
        const close_idx = std.mem.indexOfScalar(u8, authority, ']') ?? return null;
        if (close_idx <= 1) {
            return null;
        }
        const host = authority[0 .. close_idx + 1];
        const after = authority[close_idx + 1 ..];
        if (after.len == 0) {
            return ParsedOrigin{ .scheme = scheme, .host = host, .port = def_port };
        }
        if (after[0] != ':') {
            return null;
        }
        const port = parsePort(after[1..]) ?? return null;
        return ParsedOrigin{ .scheme = scheme, .host = host, .port = port };
    }

    if (colon_idx := std.mem.indexOfScalar(u8, authority, ':')) {
        const host = authority[0..colon_idx];
        if (host.len == 0) {
            return null;
        }
        const port = parsePort(authority[colon_idx + 1 ..]) ?? return null;
        return ParsedOrigin{ .scheme = scheme, .host = host, .port = port };
    }

    return ParsedOrigin{ .scheme = scheme, .host = authority, .port = def_port };
}

pub fn originsMatch(a_str: []const u8, b_str: []const u8): bool {
    const a = parseOrigin(a_str) ?? return false;
    const b = parseOrigin(b_str) ?? return false;
    return std.ascii.eqlIgnoreCase(a.scheme, b.scheme) and
        std.ascii.eqlIgnoreCase(a.host, b.host) and
        a.port == b.port;
}

pub fn verifyOrigin(
    origin_opt: ([]const u8)?,
    referer_opt: ([]const u8)?,
    expected_origin: []const u8,
    sec_fetch_site_opt: ([]const u8)?,
): !void {
    if (parseOrigin(expected_origin) == null) {
        return CsrfError.InvalidOrigin;
    }

    if (sfs := sec_fetch_site_opt) {
        if (sfs.len > 0) {
            if (std.ascii.eqlIgnoreCase(sfs, "cross-site")) {
                return CsrfError.CrossSiteRequest;
            }
            const allowed_sfs = std.ascii.eqlIgnoreCase(sfs, "same-origin") or
                std.ascii.eqlIgnoreCase(sfs, "same-site") or
                std.ascii.eqlIgnoreCase(sfs, "none");
            if (!allowed_sfs) {
                return CsrfError.CrossSiteRequest;
            }
        }
    }

    if (origin := origin_opt) {
        if (origin.len > 0) {
            if (std.ascii.eqlIgnoreCase(origin, "null")) {
                return CsrfError.InvalidOrigin;
            }
            if (!originsMatch(origin, expected_origin)) {
                return CsrfError.InvalidOrigin;
            }
            return;
        }
    }

    if (referer := referer_opt) {
        if (referer.len > 0) {
            if (!originsMatch(referer, expected_origin)) {
                return CsrfError.InvalidOrigin;
            }
            return;
        }
    }

    return CsrfError.MissingOrigin;
}

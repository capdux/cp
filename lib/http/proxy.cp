// Source: Absorbed from /data/dev/bun/src/http/ProxyTunnel.cp | Synced: 2026-11-20T05:01:43Z
// lib/http/proxy.cp
// CP HTTP Proxy Tunnel (cap:http)
// RFC 7231 HTTP CONNECT Tunneling

const std = @import("std");
import { Base64 } from "cap:encoding";

pub enum ProxyTunnelSocketType {
    Tcp,
    Ssl,
    None,
}

pub struct ProxyConnectResponse {
    pub status_code: u16 = 0,
    pub reason: []const u8 = "",
    pub established: bool = false,
    pub requires_auth: bool = false,
    pub proxy_authenticate: ([]const u8)? = null,
}

pub struct NoProxyMatcher {
    /// Parses an IPv4 dotted quad string into big-endian u32.
    fn parseIpv4(s: []const u8): u32? {
        let octets: [4]u8 = [0 as u8; 4];
        let idx: usize = 0;
        let iter = std.mem.splitScalar(u8, s, '.');
        while (part := iter.next()) {
            if (idx >= 4 || part.len == 0) return null;
            const val = std.fmt.parseInt(u8, part, 10) catch return null;
            octets[idx] = val;
            idx += 1;
        }
        if (idx != 4) return null;
        return ((octets[0] as u32) << 24) |
            ((octets[1] as u32) << 16) |
            ((octets[2] as u32) << 8) |
            (octets[3] as u32);
    }

    fn trimWs(s: []const u8): []const u8 {
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
        return s[start..end];
    }

    fn stripBrackets(host: []const u8): []const u8 {
        if (host.len >= 2 && host[0] == '[' && host[host.len - 1] == ']') {
            return host[1 .. host.len - 1];
        }
        return host;
    }

    /// Evaluates whether `target_host:target_port` bypasses proxy according to `NO_PROXY` rules.
    /// Supports `*`, exact host, `.domain` / `domain` suffix matching, `:port` filter, and IPv4 CIDR (`10.0.0.0/8`).
    pub static fn matches(target_host: []const u8, target_port: u16, no_proxy_list: []const u8): bool {
        if (no_proxy_list.len == 0 || target_host.len == 0) return false;
        const clean_target = stripBrackets(trimWs(target_host));
        const target_ip = parseIpv4(clean_target);

        let iter = std.mem.splitScalar(u8, no_proxy_list, ',');
        while (raw_entry := iter.next()) {
            let entry = trimWs(raw_entry);
            if (entry.len == 0) continue;
            if (std.mem.eql(u8, entry, "*")) return true;

            // Check optional :port suffix (only if not IPv6 literal without brackets and not CIDR)
            let rule_host = entry;
            let rule_port: u16? = null;
            if (entry[0] == '[') {
                if (close_br := std.mem.indexOfScalar(u8, entry, ']')) {
                    rule_host = entry[1..close_br];
                    if (close_br + 1 < entry.len && entry[close_br + 1] == ':') {
                        rule_port = std.fmt.parseInt(u16, entry[close_br + 2 ..], 10) catch null;
                    }
                }
            } else if (colon := std.mem.lastIndexOfScalar(u8, entry, ':')) {
                if (std.mem.indexOfScalar(u8, entry[0..colon], ':') == null) {
                    if (p := std.fmt.parseInt(u16, entry[colon + 1 ..], 10) catch null) {
                        rule_port = p;
                        rule_host = entry[0..colon];
                    }
                }
            }

            if (rp := rule_port) {
                if (rp != target_port) continue;
            }

            // CIDR match (e.g. 192.168.0.0/16)
            if (slash := std.mem.indexOfScalar(u8, rule_host, '/')) {
                if (t_ip := target_ip) {
                    const net_str = rule_host[0..slash];
                    const prefix_str = rule_host[slash + 1 ..];
                    if (net_ip := parseIpv4(net_str)) {
                        const prefix_len = std.fmt.parseInt(u8, prefix_str, 10) catch 255;
                        if (prefix_len <= 32) {
                            if (prefix_len == 0) return true;
                            const shift: u5 = @truncate(32 - prefix_len);
                            const mask: u32 = (~(0 as u32)) << shift;
                            if ((t_ip & mask) == (net_ip & mask)) return true;
                        }
                    }
                }
                continue;
            }

            // Leading dot ".example.com" or bare "example.com"
            if (rule_host.len > 0 && rule_host[0] == '.') {
                const bare = rule_host[1..];
                if (std.ascii.eqlIgnoreCase(clean_target, bare)) return true;
                if (clean_target.len > rule_host.len && std.ascii.endsWithIgnoreCase(clean_target, rule_host)) {
                    return true;
                }
            } else {
                if (std.ascii.eqlIgnoreCase(clean_target, rule_host)) return true;
                if (clean_target.len > rule_host.len + 1 &&
                    clean_target[clean_target.len - rule_host.len - 1] == '.' &&
                    std.ascii.endsWithIgnoreCase(clean_target, rule_host))
                {
                    return true;
                }
            }
        }
        return false;
    }
}

pub struct ProxyTunnel {
    pub proxy_host: []const u8 = "",
    pub proxy_port: u16 = 8080;
    pub target_host: []const u8 = "";
    pub target_port: u16 = 443;
    pub socket_fd: i32 = -1;
    pub socket_type: ProxyTunnelSocketType = ProxyTunnelSocketType.None;
    pub is_established: bool = false;
    pub auth_header: []const u8 = "";
    pub auth_buf: [256]u8 = undefined;
    pub auth_len: usize = 0;

    pub static fn init(proxy_host: []const u8, proxy_port: u16, target_host: []const u8, target_port: u16): ProxyTunnel {
        return ProxyTunnel{
            proxy_host: proxy_host,
            proxy_port: proxy_port,
            target_host: target_host,
            target_port: target_port,
            socket_fd: -1,
            socket_type: ProxyTunnelSocketType.None,
            is_established: false,
            auth_header: "",
            auth_buf: undefined,
            auth_len: 0,
        };
    }

    /// Encodes `Basic <base64(username:password)>` into `out` and returns the header value slice.
    pub static fn encodeBasicAuth(username: []const u8, password: []const u8, out: []u8): ([]const u8)! {
        let cred_buf: [256]u8 = undefined;
        const cred_len = username.len + 1 + password.len;
        if (cred_len > cred_buf.len) return error.CredentialsTooLong;
        @memcpy(cred_buf[0..username.len], username);
        cred_buf[username.len] = ':';
        @memcpy(cred_buf[username.len + 1 .. cred_len], password);

        const b64_len = Base64.encodeLen(cred_len);
        if (6 + b64_len > out.len) return error.BufferTooSmall;
        @memcpy(out[0..6], "Basic ");
        const written = Base64.encode(out[6..], cred_buf[0..cred_len]);
        return out[0 .. 6 + written];
    }

    pub fn setBasicAuth(*_, username: []const u8, password: []const u8): bool {
        const encoded = encodeBasicAuth(username, password, &_.auth_buf) catch return false;
        _.auth_len = encoded.len;
        _.auth_header = _.auth_buf[0.._.auth_len];
        return true;
    }

    pub fn formatConnectRequest(buf: []u8): ![]const u8 {
        if (_.auth_len > 0) {
            return std.fmt.bufPrint(
                buf,
                "CONNECT {s}:{d} HTTP/1.1\r\nHost: {s}:{d}\r\nProxy-Authorization: {s}\r\nProxy-Connection: keep-alive\r\nUser-Agent: CP/1.0\r\n\r\n",
                .{ _.target_host, _.target_port, _.target_host, _.target_port, _.auth_buf[0.._.auth_len] },
            )?;
        }
        if (_.auth_header.len > 0) {
            return std.fmt.bufPrint(
                buf,
                "CONNECT {s}:{d} HTTP/1.1\r\nHost: {s}:{d}\r\nProxy-Authorization: {s}\r\nProxy-Connection: keep-alive\r\nUser-Agent: CP/1.0\r\n\r\n",
                .{ _.target_host, _.target_port, _.target_host, _.target_port, _.auth_header },
            )?;
        }
        return std.fmt.bufPrint(
            buf,
            "CONNECT {s}:{d} HTTP/1.1\r\nHost: {s}:{d}\r\nProxy-Connection: keep-alive\r\nUser-Agent: CP/1.0\r\n\r\n",
            .{ _.target_host, _.target_port, _.target_host, _.target_port },
        )?;
    }

    pub static fn parseConnectResponse(response: []const u8): ProxyConnectResponse? {
        if (response.len < 12) return null;
        if (!std.mem.startsWith(u8, response, "HTTP/1.1 ") && !std.mem.startsWith(u8, response, "HTTP/1.0 ")) {
            return null;
        }
        const line_end = std.mem.indexOf(u8, response, "\r\n") ?? response.len;
        const status_line = response[0..line_end];
        if (status_line.len < 12) return null;

        const code = std.fmt.parseInt(u16, status_line[9..12], 10) catch return null;
        const reason = if (status_line.len > 13 && status_line[12] == ' ') status_line[13..] else "";

        let proxy_auth: ([]const u8)? = null;
        let pos: usize = if (line_end + 2 <= response.len) line_end + 2 else response.len;
        while (pos < response.len) {
            const next_eol = std.mem.indexOfPos(u8, response, pos, "\r\n") ?? response.len;
            if (next_eol == pos) break;
            const hdr_line = response[pos..next_eol];
            if (colon := std.mem.indexOfScalar(u8, hdr_line, ':')) {
                const name = NoProxyMatcher.trimWs(hdr_line[0..colon]);
                const val = NoProxyMatcher.trimWs(hdr_line[colon + 1 ..]);
                if (std.ascii.eqlIgnoreCase(name, "Proxy-Authenticate")) {
                    proxy_auth = val;
                }
            }
            pos = if (next_eol + 2 <= response.len) next_eol + 2 else response.len;
        }

        return ProxyConnectResponse{
            status_code: code,
            reason: reason,
            established: code >= 200 && code < 300,
            requires_auth: code == 407,
            proxy_authenticate: proxy_auth,
        };
    }

    pub static fn verifyConnectResponse(response: []const u8): bool {
        if (parsed := parseConnectResponse(response)) {
            return parsed.established;
        }
        return false;
    }

    pub static fn shouldBypassProxy(target_host: []const u8, target_port: u16, no_proxy: []const u8): bool {
        return NoProxyMatcher.matches(target_host, target_port, no_proxy);
    }

    pub fn close(*_) {
        if (_.socket_fd >= 0) {
            * = std.os.linux.close(_.socket_fd);
            _.socket_fd = -1;
        }
        _.is_established = false;
    }
}

pub type HttpProxyTunnel = ProxyTunnel;

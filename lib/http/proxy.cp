// Source: /data/dev/bun/src/http/ProxyTunnel.cp
// lib/http/proxy.cp
// CP HTTP Proxy Tunnel (cap:http)
// RFC 7231 HTTP CONNECT Tunneling

const std = @import("std");

pub enum ProxyTunnelSocketType {
    Tcp,
    Ssl,
    None,
}

pub struct ProxyTunnel {
    pub proxy_host: []const u8 = "";
    pub proxy_port: u16 = 8080;
    pub target_host: []const u8 = "";
    pub target_port: u16 = 443;
    pub socket_fd: i32 = -1;
    pub socket_type: ProxyTunnelSocketType = ProxyTunnelSocketType.None;
    pub is_established: bool = false;
    pub auth_header: []const u8 = "";

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
        };
    }

    pub fn formatConnectRequest(buf: []u8): ![]const u8 {
        return std.fmt.bufPrint(
            buf,
            "CONNECT {s}:{d} HTTP/1.1\r\nHost: {s}:{d}\r\nProxy-Connection: keep-alive\r\nUser-Agent: CP/1.0\r\n\r\n",
            .{ _.target_host, _.target_port, _.target_host, _.target_port },
        )?;
    }

    pub static fn verifyConnectResponse(response: []const u8): bool {
        if (response.len < 12) return false;
        if (std.mem.startsWith(u8, response, "HTTP/1.1 200") || std.mem.startsWith(u8, response, "HTTP/1.0 200")) {
            return true;
        }
        return false;
    }

    pub fn close(*_) {
        if (_.socket_fd >= 0) {
            * = std.os.linux.close(_.socket_fd);
            _.socket_fd = -1;
        }
        _.is_established = false;
    }
}

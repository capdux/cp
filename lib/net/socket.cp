// Source: Absorbed from /data/dev/bun/src/uws_sys/socket.cp | Synced: 2026-11-22T13:13:07Z
// lib/net/socket.cp
// SafeSocket with RAII, socket options, long-timeout splitting, and legacy Socket compatibility
// Absorbed from Bun src/uws_sys/socket.cp and src/uws_sys/us_socket_t.cp

import { SocketKind, SocketCloseCode, SendStatus } from "./_usockets";

pub enum SocketState: u8 {
    Connecting = 0,
    Connected = 1,
    Closing = 2,
    Closed = 3,
    Detached = 4,
}

pub struct SocketStats {
    bytes_read: u64 = 0;
    bytes_written: u64 = 0;
    write_calls: u64 = 0;
    read_calls: u64 = 0;
}

pub class SafeSocket {
    pub fd: i32 = -1;
    pub kind: SocketKind = SocketKind.closed;
    pub state: SocketState = SocketState.Closed;
    pub is_tls: bool = false;
    pub nodelay: bool = false;
    pub keepalive_enabled: bool = false;
    pub keepalive_delay: u32 = 0;
    pub timeout_sec: u32 = 0;
    pub long_timeout_min: u32 = 0;
    pub paused: bool = false;
    pub read_shutdown: bool = false;
    pub write_shutdown: bool = false;
    pub close_code: SocketCloseCode = SocketCloseCode.normal;
    pub stats: SocketStats = SocketStats {};

    pub static fn create(fd: i32, kind: SocketKind): *SafeSocket {
        let ptr = std.heap.page_allocator.create(SafeSocket) catch unreachable;
        ptr.fd = fd;
        ptr.kind = kind;
        if (kind == SocketKind.established_tcp || kind == SocketKind.established_tls || kind == SocketKind.udp) {
            ptr.state = SocketState.Connected;
        } else if (kind == SocketKind.connecting_tcp || kind == SocketKind.connecting_tls) {
            ptr.state = SocketState.Connecting;
        } else {
            ptr.state = SocketState.Closed;
        }
        ptr.is_tls = (kind == SocketKind.established_tls || kind == SocketKind.connecting_tls || kind == SocketKind.listen_tls);
        ptr.nodelay = false;
        ptr.keepalive_enabled = false;
        ptr.keepalive_delay = 0;
        ptr.timeout_sec = 0;
        ptr.long_timeout_min = 0;
        ptr.paused = false;
        ptr.read_shutdown = false;
        ptr.write_shutdown = false;
        ptr.close_code = SocketCloseCode.normal;
        ptr.stats = SocketStats {};
        return ptr;
    }

    pub fn setNoDelay(*_, enabled: bool) {
        if (_.isClosed()) {
            return;
        }
        _.nodelay = enabled;
    }

    pub fn setKeepAlive(*_, enabled: bool, delay: u32): bool {
        if (_.isClosed()) {
            return false;
        }
        _.keepalive_enabled = enabled;
        if (enabled) {
            _.keepalive_delay = delay;
        } else {
            _.keepalive_delay = 0;
        }
        return true;
    }

    // Absorbed from Bun SocketHandler.setTimeout:
    // uSockets short timeout supports <= 240s (4-second granularity);
    // timeouts > 240s automatically promote to minute-level long_timeout!
    pub fn setTimeout(*_, seconds: u32) {
        if (_.isClosed()) {
            return;
        }
        if (seconds > 240) {
            _.timeout_sec = 0;
            _.long_timeout_min = seconds / 60;
        } else {
            _.timeout_sec = seconds;
            _.long_timeout_min = 0;
        }
    }

    pub fn setLongTimeout(*_, minutes: u32) {
        if (_.isClosed()) {
            return;
        }
        _.long_timeout_min = minutes;
    }

    pub fn pause(*_) {
        if (_.isClosed()) {
            return;
        }
        _.paused = true;
    }

    pub fn resumeSocket(*_) {
        if (_.isClosed()) {
            return;
        }
        _.paused = false;
    }

    pub fn detach(*_) {
        _.state = SocketState.Detached;
        _.kind = SocketKind.closed;
        _.fd = -1;
    }

    pub fn isDetached(*_): bool {
        return _.state == SocketState.Detached || _.fd == -1;
    }

    pub fn isClosed(*_): bool {
        return _.state == SocketState.Closed || _.state == SocketState.Detached || _.kind == SocketKind.closed;
    }

    pub fn isEstablished(*_): bool {
        return _.state == SocketState.Connected &&
            (_.kind == SocketKind.established_tcp || _.kind == SocketKind.established_tls);
    }

    pub fn recordWrite(*_, n: usize): SendStatus {
        if (_.isClosed() || _.write_shutdown) {
            return SendStatus.dropped;
        }
        _.stats.bytes_written += n as u64;
        _.stats.write_calls += 1;
        return SendStatus.success;
    }

    pub fn recordRead(*_, n: usize) {
        if (_.isClosed() || _.read_shutdown) {
            return;
        }
        _.stats.bytes_read += n as u64;
        _.stats.read_calls += 1;
    }

    pub fn shutdown(*_) {
        if (_.isClosed()) {
            return;
        }
        _.write_shutdown = true;
    }

    pub fn shutdownRead(*_) {
        if (_.isClosed()) {
            return;
        }
        _.read_shutdown = true;
    }

    pub fn close(*_, code: SocketCloseCode) {
        if (_.state == SocketState.Closed || _.state == SocketState.Detached) {
            return;
        }
        _.close_code = code;
        _.state = SocketState.Closed;
        _.kind = SocketKind.closed;
        _.fd = -1;
    }

    ~init() {
        if (!_.isClosed()) {
            _.close(SocketCloseCode.normal);
        }
        std.heap.page_allocator.destroy(_);
    }
}

// ============================================================================
// Backward-Compatible Actor / Async Socket (from legacy lib/net.cp)
// ============================================================================
pub class Socket {
    pub fd: i32;

    pub fn init(fd: i32) {
        this.fd = fd;
    }

    pub async fn read(): string {
        return await @net_read_async(this.fd);
    }

    pub async fn write(data: string): i32 {
        return await @net_write_async(this.fd, data);
    }

    pub fn close() {
        @net_close(this.fd);
    }
}

// Source: cp/lib/net/sockopts.cp (POSIX Socket Options & Linux Poll)
// lib/net/sockopts.cp
// Production Socket Options (std.os.linux / std.c) & Lightweight Poll

pub struct PollEvent {
    fd: i32 = -1;
    events: u16 = 0;
    revents: u16 = 0;
    ready: bool = false;

    pub const NONE: u16 = 0;
    pub const IN: u16 = 0x001;
    pub const PRI: u16 = 0x002;
    pub const OUT: u16 = 0x004;
    pub const ERR: u16 = 0x008;
    pub const HUP: u16 = 0x010;
    pub const NVAL: u16 = 0x020;

    fn isReadable(): bool {
        return (_.revents & PollEvent.IN) != 0;
    }

    fn isWritable(): bool {
        return (_.revents & PollEvent.OUT) != 0;
    }

    fn hasError(): bool {
        return (_.revents & PollEvent.ERR) != 0;
    }

    fn isHup(): bool {
        return (_.revents & PollEvent.HUP) != 0;
    }

    fn isInvalid(): bool {
        return (_.revents & PollEvent.NVAL) != 0;
    }
}

fn implSetIntSockOpt(fd: i32, level: i32, optname: u32, val: i32): bool {
    if (fd < 0) { return false; }
    let raw_bytes: [4]u8 = @bitCast(val);
    const rc = std.os.linux.setsockopt(
        fd,
        level,
        optname,
        raw_bytes[0..4].ptr,
        4 as u32
    );
    return std.os.linux.errno(rc) == .SUCCESS;
}

fn implGetIntSockOpt(fd: i32, level: i32, optname: u32): i32? {
    if (fd < 0) { return null; }
    let raw_bytes: [4]u8 = [0; 4];
    let optlen: u32 = 4;
    let ok = false;
    unsafe {
        const rc = std.os.linux.getsockopt(
            fd,
            level,
            optname,
            raw_bytes[0..4].ptr,
            *optlen!
        );
        ok = std.os.linux.errno(rc) == .SUCCESS;
    }
    if (!ok || optlen < 4) { return null; }
    const val: i32 = @bitCast(raw_bytes);
    return val;
}

fn implSetTimevalSockOpt(fd: i32, optname: u32, ms: u32): bool {
    if (fd < 0) { return false; }
    let tv: std.os.linux.timeval = undefined;
    tv.sec = (ms / 1000) as isize;
    tv.usec = ((ms % 1000) * 1000) as i64;
    let ok = false;
    unsafe {
        const tv_ptr = (*tv!) as [*]const u8!;
        const rc = std.os.linux.setsockopt(
            fd,
            std.os.linux.SOL.SOCKET,
            optname,
            tv_ptr,
            @sizeOf(std.os.linux.timeval) as u32
        );
        ok = std.os.linux.errno(rc) == .SUCCESS;
    }
    return ok;
}

fn implSetNoDelay(fd: i32, enable: bool): bool {
    const v: i32 = if (enable) 1 else 0;
    return implSetIntSockOpt(fd, std.os.linux.IPPROTO.TCP, std.os.linux.TCP.NODELAY, v);
}

fn implSetKeepAlive(fd: i32, enable: bool): bool {
    const v: i32 = if (enable) 1 else 0;
    return implSetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.KEEPALIVE, v);
}

fn implSetReuseAddr(fd: i32, enable: bool): bool {
    const v: i32 = if (enable) 1 else 0;
    return implSetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.REUSEADDR, v);
}

fn implSetReusePort(fd: i32, enable: bool): bool {
    const v: i32 = if (enable) 1 else 0;
    return implSetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.REUSEPORT, v);
}

fn implSetRecvBufSize(fd: i32, bytes: u32): bool {
    return implSetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.RCVBUF, bytes as i32);
}

fn implSetSendBufSize(fd: i32, bytes: u32): bool {
    return implSetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.SNDBUF, bytes as i32);
}

fn implSetRecvTimeoutMs(fd: i32, ms: u32): bool {
    return implSetTimevalSockOpt(fd, std.os.linux.SO.RCVTIMEO, ms);
}

fn implSetSendTimeoutMs(fd: i32, ms: u32): bool {
    return implSetTimevalSockOpt(fd, std.os.linux.SO.SNDTIMEO, ms);
}

fn implSetNonBlocking(fd: i32, enable: bool): bool {
    if (fd < 0) { return false; }
    const cur_rc = std.os.linux.fcntl(fd, std.os.linux.F.GETFL, 0);
    if (std.os.linux.errno(cur_rc) != .SUCCESS) { return false; }
    const o_nonblock: usize = @as(u32, @bitCast(std.os.linux.O{ .NONBLOCK = true }));
    const next_flags: usize = if (enable) (cur_rc | o_nonblock) else (cur_rc & (~o_nonblock));
    const set_rc = std.os.linux.fcntl(fd, std.os.linux.F.SETFL, next_flags);
    return std.os.linux.errno(set_rc) == .SUCCESS;
}

fn implGetSocketError(fd: i32): i32 {
    if (err_val := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.ERROR)) {
        return err_val;
    }
    return -1;
}

pub fn setNoDelay(fd: i32, enable: bool): bool {
    return implSetNoDelay(fd, enable);
}

pub fn getNoDelay(fd: i32): bool? {
    if (v := implGetIntSockOpt(fd, std.os.linux.IPPROTO.TCP, std.os.linux.TCP.NODELAY)) {
        return v != 0;
    }
    return null;
}

pub fn setKeepAlive(fd: i32, enable: bool): bool {
    return implSetKeepAlive(fd, enable);
}

pub fn getKeepAlive(fd: i32): bool? {
    if (v := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.KEEPALIVE)) {
        return v != 0;
    }
    return null;
}

pub fn setReuseAddr(fd: i32, enable: bool): bool {
    return implSetReuseAddr(fd, enable);
}

pub fn getReuseAddr(fd: i32): bool? {
    if (v := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.REUSEADDR)) {
        return v != 0;
    }
    return null;
}

pub fn setReusePort(fd: i32, enable: bool): bool {
    return implSetReusePort(fd, enable);
}

pub fn getReusePort(fd: i32): bool? {
    if (v := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.REUSEPORT)) {
        return v != 0;
    }
    return null;
}

pub fn setRecvBufSize(fd: i32, bytes: u32): bool {
    return implSetRecvBufSize(fd, bytes);
}

pub fn getRecvBufSize(fd: i32): u32? {
    if (v := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.RCVBUF)) {
        if (v >= 0) { return v as u32; }
    }
    return null;
}

pub fn setSendBufSize(fd: i32, bytes: u32): bool {
    return implSetSendBufSize(fd, bytes);
}

pub fn getSendBufSize(fd: i32): u32? {
    if (v := implGetIntSockOpt(fd, std.os.linux.SOL.SOCKET, std.os.linux.SO.SNDBUF)) {
        if (v >= 0) { return v as u32; }
    }
    return null;
}

pub fn setRecvTimeoutMs(fd: i32, ms: u32): bool {
    return implSetRecvTimeoutMs(fd, ms);
}

pub fn setSendTimeoutMs(fd: i32, ms: u32): bool {
    return implSetSendTimeoutMs(fd, ms);
}

pub fn setNonBlocking(fd: i32, enable: bool): bool {
    return implSetNonBlocking(fd, enable);
}

pub fn isNonBlocking(fd: i32): bool? {
    if (fd < 0) { return null; }
    const cur_rc = std.os.linux.fcntl(fd, std.os.linux.F.GETFL, 0);
    if (std.os.linux.errno(cur_rc) != .SUCCESS) { return null; }
    const o_nonblock: usize = @as(u32, @bitCast(std.os.linux.O{ .NONBLOCK = true }));
    return (cur_rc & o_nonblock) != 0;
}

pub fn getSocketError(fd: i32): i32 {
    return implGetSocketError(fd);
}

pub fn pollSingleFd(fd: i32, events: u16, timeout_ms: i32): PollEvent {
    if (fd < 0) {
        return PollEvent { fd: fd, events: events, revents: PollEvent.NVAL, ready: false };
    }
    let pfd: std.os.linux.pollfd = undefined;
    pfd.fd = fd;
    pfd.events = @bitCast(events);
    pfd.revents = 0;
    let pfds: [1]std.os.linux.pollfd = [pfd];
    while (true) {
        const rc = std.os.linux.poll(pfds[0..1].ptr, 1, timeout_ms);
        const err = std.os.linux.errno(rc);
        if (err == .INTR) { continue; }
        if (err != .SUCCESS) {
            return PollEvent { fd: fd, events: events, revents: PollEvent.ERR, ready: false };
        }
        const rev: u16 = @bitCast(pfds[0].revents);
        return PollEvent {
            fd: fd,
            events: events,
            revents: rev,
            ready: rc > 0 && rev != 0,
        };
    }
}

pub fn pollManyFds(pfds: []std.os.linux.pollfd, timeout_ms: i32): i32 {
    if (pfds.len == 0) { return 0; }
    while (true) {
        const rc = std.os.linux.poll(pfds.ptr, pfds.len, timeout_ms);
        const err = std.os.linux.errno(rc);
        if (err == .INTR) { continue; }
        if (err != .SUCCESS) { return -1; }
        return rc as i32;
    }
}

pub fn createTcpSocket(nonblocking: bool): i32? {
    let sock_type: u32 = std.os.linux.SOCK.STREAM | std.os.linux.SOCK.CLOEXEC;
    if (nonblocking) {
        sock_type |= std.os.linux.SOCK.NONBLOCK;
    }
    const rc = std.os.linux.socket(std.os.linux.AF.INET, sock_type, 0);
    if (std.os.linux.errno(rc) != .SUCCESS) { return null; }
    return rc as i32;
}

pub fn createSocketPair(nonblocking: bool): ([2]i32)? {
    let sock_type: u32 = std.os.linux.SOCK.STREAM | std.os.linux.SOCK.CLOEXEC;
    if (nonblocking) {
        sock_type |= std.os.linux.SOCK.NONBLOCK;
    }
    let fds: [2]i32 = [-1, -1];
    let ok = false;
    unsafe {
        const rc = std.os.linux.socketpair(std.os.linux.AF.UNIX, sock_type, 0, *fds!);
        ok = std.os.linux.errno(rc) == .SUCCESS;
    }
    if (!ok) { return null; }
    return fds;
}

pub fn closeSocketFd(fd: i32) {
    if (fd >= 0) {
        * = std.os.linux.close(fd);
    }
}

pub fn writeFd(fd: i32, data: []const u8): isize {
    if (fd < 0 || data.len == 0) { return 0; }
    const rc = std.os.linux.write(fd, data.ptr, data.len);
    if (std.os.linux.errno(rc) != .SUCCESS) { return -1; }
    return @bitCast(rc);
}

pub fn readFd(fd: i32, buf: []u8): isize {
    if (fd < 0 || buf.len == 0) { return 0; }
    const rc = std.os.linux.read(fd, buf.ptr, buf.len);
    if (std.os.linux.errno(rc) != .SUCCESS) { return -1; }
    return @bitCast(rc);
}

pub struct SocketOptions {
    no_delay: bool? = null;
    keep_alive: bool? = null;
    reuse_addr: bool? = null;
    reuse_port: bool? = null;
    non_blocking: bool? = null;
    recv_buf_size: u32? = null;
    send_buf_size: u32? = null;
    recv_timeout_ms: u32? = null;
    send_timeout_ms: u32? = null;

    fn apply(fd: i32): bool {
        if (fd < 0) { return false; }
        if (v := _.no_delay) {
            if (!implSetNoDelay(fd, v)) { return false; }
        }
        if (v := _.keep_alive) {
            if (!implSetKeepAlive(fd, v)) { return false; }
        }
        if (v := _.reuse_addr) {
            if (!implSetReuseAddr(fd, v)) { return false; }
        }
        if (v := _.reuse_port) {
            if (!implSetReusePort(fd, v)) { return false; }
        }
        if (v := _.recv_buf_size) {
            if (!implSetRecvBufSize(fd, v)) { return false; }
        }
        if (v := _.send_buf_size) {
            if (!implSetSendBufSize(fd, v)) { return false; }
        }
        if (v := _.recv_timeout_ms) {
            if (!implSetRecvTimeoutMs(fd, v)) { return false; }
        }
        if (v := _.send_timeout_ms) {
            if (!implSetSendTimeoutMs(fd, v)) { return false; }
        }
        if (v := _.non_blocking) {
            if (!implSetNonBlocking(fd, v)) { return false; }
        }
        return true;
    }

    static fn setNoDelay(fd: i32, enable: bool): bool { return implSetNoDelay(fd, enable); }
    static fn setKeepAlive(fd: i32, enable: bool): bool { return implSetKeepAlive(fd, enable); }
    static fn setReuseAddr(fd: i32, enable: bool): bool { return implSetReuseAddr(fd, enable); }
    static fn setReusePort(fd: i32, enable: bool): bool { return implSetReusePort(fd, enable); }
    static fn setRecvBufSize(fd: i32, bytes: u32): bool { return implSetRecvBufSize(fd, bytes); }
    static fn setSendBufSize(fd: i32, bytes: u32): bool { return implSetSendBufSize(fd, bytes); }
    static fn setRecvTimeoutMs(fd: i32, ms: u32): bool { return implSetRecvTimeoutMs(fd, ms); }
    static fn setSendTimeoutMs(fd: i32, ms: u32): bool { return implSetSendTimeoutMs(fd, ms); }
    static fn setNonBlocking(fd: i32, enable: bool): bool { return implSetNonBlocking(fd, enable); }
    static fn getSocketError(fd: i32): i32 { return implGetSocketError(fd); }
}

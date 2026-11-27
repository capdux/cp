// Origin: Pure-CP Stdlib (Multi-Core SO_REUSEPORT + epoll + 32B SIMD WebSocket Server: cap:web)
// ============================================================================
// High-Performance Native RFC 6455 WebSocket Server:
//   - Multi-worker `SO_REUSEPORT` (1 epoll event loop per CPU worker, 0 lock contention)
//   - Zero heap allocation per WebSocket frame (pre-allocated flat connection slab)
//   - 32-byte SIMD (`@Vector(32, u8)`) mask XOR unmasking + single-write zero-copy echo
//   - Built-in pure-CP SHA-1 + Base64 for RFC 6455 `Sec-WebSocket-Accept` handshake
//   - Automatic Ping -> Pong reply and Close frame graceful teardown
// ============================================================================

pub const WS_MAX_WORKERS: usize = 64;
pub const WS_READ_BUF_LEN: usize = 2048;
pub const WS_WRITE_BUF_LEN: usize = 2048;
pub const WS_EPOLL_BATCH: usize = 128;

// Linux x86_64 POSIX / socket / epoll constants
const AF_INET: i32 = 2;
const SOCK_STREAM: i32 = 1;
const SOCK_NONBLOCK: i32 = 2048;
const SOCK_CLOEXEC: i32 = 524288;
const SOL_SOCKET: i32 = 1;
const SO_REUSEADDR: i32 = 2;
const SO_REUSEPORT: i32 = 15;
const IPPROTO_TCP: i32 = 6;
const TCP_NODELAY: i32 = 1;

const EPOLL_CLOEXEC: i32 = 524288;
const EPOLL_CTL_ADD: i32 = 1;
const EPOLL_CTL_DEL: i32 = 2;
const EPOLLIN: u32 = 0x001;
const EPOLLERR: u32 = 0x008;
const EPOLLHUP: u32 = 0x010;
const EPOLLRDHUP: u32 = 0x2000;

const WS_GUID: string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const B64_TABLE: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

@c struct WsSockAddrIn {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [8]u8,
}

// Linux x86_64 struct epoll_event is 12 bytes (__attribute__((__packed__)): u32 events + u64 data).
// Using three u32 fields guarantees exact 12-byte stride with zero padding!
@c struct WsEpollEventC {
    events: u32,
    data_lo: u32,
    data_hi: u32,
}

extern fn socket(domain: i32, sock_type: i32, protocol: i32): i32;
extern fn setsockopt(fd: i32, level: i32, optname: i32, optval: *const u8, optlen: u32): i32;
extern fn bind(fd: i32, addr: *const WsSockAddrIn, addrlen: u32): i32;
extern fn listen(fd: i32, backlog: i32): i32;
extern fn accept4(fd: i32, addr: *u8?, addrlen: *u32?, flags: i32): i32;
extern fn epoll_create1(flags: i32): i32;
extern fn epoll_ctl(epfd: i32, op: i32, fd: i32, event: *WsEpollEventC?): i32;
extern fn epoll_wait(epfd: i32, events: *WsEpollEventC, maxevents: i32, timeout: i32): i32;
extern fn read(fd: i32, buf: *u8, count: usize): isize;
extern fn write(fd: i32, buf: [*]const u8, count: usize): isize;
extern fn close(fd: i32): i32;
extern fn malloc(size: usize): *u8?;
extern fn free(ptr: *u8?);
extern fn __errno_location(): *i32;
extern fn sched_yield(): i32;
extern fn sched_setaffinity(pid: i32, cpusetsize: usize, mask: *const u8): i32;
extern fn pthread_create(
    thread: *usize,
    attr: *const u8?,
    start_routine: *const fn (*u8?) callconv(.c) *u8?,
    arg: *u8?,
): i32;
extern fn pthread_join(thread: usize, retval: *u8?): i32;

fn wsHtons(v: u16): u16 {
    return ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);
}

fn wsAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn wsAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (wsAsciiLower(a[i]) != wsAsciiLower(b[i])) return false;
    }
    return true;
}

fn wsFindCrlf(buf: string, start: usize): usize? {
    if (start + 1 >= buf.len) return null;
    let i = start;
    while (i + 1 < buf.len) : (i += 1) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') return i;
    }
    return null;
}

fn wsWriteBytes(dst: []u8, pos: *usize, s: string): bool {
    if (s.len > dst.len || pos.* > dst.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

// ============================================================================
// 32-Byte SIMD Mask XOR (`@Vector(32, u8)`)
// ============================================================================

/// Broadcasts 4-byte WebSocket mask key into a 32-byte SIMD vector and XORs
/// `src` into `dst` 32 bytes at a time, with scalar tail handling.
pub fn simdMaskXorCopy(dst: []u8, src: string, mask: [4]u8) {
    const n = src.len;
    if (n == 0) return;
    let i: usize = 0;
    if (n >= 32) {
        let mask_arr: [32]u8 = [
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
        ];
        const mask_vec: @Vector(32, u8) = mask_arr;
        while (i + 32 <= n) : (i += 32) {
            const chunk: @Vector(32, u8) = src[i..][0..32].*;
            const xored: @Vector(32, u8) = chunk ^ mask_vec;
            const out32: [32]u8 = xored;
            dst[i..][0..32].* = out32;
        }
    }
    while (i < n) : (i += 1) {
        dst[i] = src[i] ^ mask[i & 3];
    }
}

/// In-place 32-byte SIMD mask XOR using `@Vector(32, u8)`.
pub fn simdMaskXorInPlace(buf: []u8, mask: [4]u8) {
    const n = buf.len;
    if (n == 0) return;
    let i: usize = 0;
    if (n >= 32) {
        let mask_arr: [32]u8 = [
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
            mask[0], mask[1], mask[2], mask[3],
        ];
        const mask_vec: @Vector(32, u8) = mask_arr;
        while (i + 32 <= n) : (i += 32) {
            const chunk: @Vector(32, u8) = buf[i..][0..32].*;
            const xored: @Vector(32, u8) = chunk ^ mask_vec;
            const out32: [32]u8 = xored;
            buf[i..][0..32].* = out32;
        }
    }
    while (i < n) : (i += 1) {
        buf[i] = buf[i] ^ mask[i & 3];
    }
}

// ============================================================================
// Pure-CP SHA-1 (RFC 3174) + Base64 for RFC 6455 Sec-WebSocket-Accept
// ============================================================================

fn rotl1(x: u32): u32 {
    return (x << 1) | (x >> 31);
}

fn rotl5(x: u32): u32 {
    return (x << 5) | (x >> 27);
}

fn rotl30(x: u32): u32 {
    return (x << 30) | (x >> 2);
}

fn sha1ProcessBlock(state: *[5]u32, block: *const [64]u8) {
    let w: [80]u32 = undefined;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const b = i * 4;
        w[i] = ((block.*[b] as u32) << 24)
            | ((block.*[b + 1] as u32) << 16)
            | ((block.*[b + 2] as u32) << 8)
            | (block.*[b + 3] as u32);
    }
    while (i < 80) : (i += 1) {
        w[i] = rotl1(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]);
    }

    let a = state.*[0];
    let b = state.*[1];
    let c = state.*[2];
    let d = state.*[3];
    let e = state.*[4];

    i = 0;
    while (i < 80) : (i += 1) {
        let f: u32 = 0;
        let k: u32 = 0;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        const temp = rotl5(a) +% f +% e +% k +% w[i];
        e = d;
        d = c;
        c = rotl30(b);
        b = a;
        a = temp;
    }

    state.*[0] = state.*[0] +% a;
    state.*[1] = state.*[1] +% b;
    state.*[2] = state.*[2] +% c;
    state.*[3] = state.*[3] +% d;
    state.*[4] = state.*[4] +% e;
}

pub fn sha1Digest(data: string): [20]u8 {
    let state: [5]u32 = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];
    let block: [64]u8 = undefined;
    let offset: usize = 0;
    while (offset + 64 <= data.len) : (offset += 64) {
        @memcpy(block[0..64], data[offset .. offset + 64]);
        sha1ProcessBlock(*state, *block);
    }

    // Final padded block(s)
    let rem = data.len - offset;
    let j: usize = 0;
    while (j < 64) : (j += 1) {
        block[j] = 0;
    }
    j = 0;
    while (j < rem) : (j += 1) {
        block[j] = data[offset + j];
    }
    block[rem] = 0x80;
    if (rem >= 56) {
        sha1ProcessBlock(*state, *block);
        j = 0;
        while (j < 64) : (j += 1) {
            block[j] = 0;
        }
    }
    const bit_len: u64 = (data.len as u64) * 8;
    block[56] = ((bit_len >> 56) & 0xFF) as u8;
    block[57] = ((bit_len >> 48) & 0xFF) as u8;
    block[58] = ((bit_len >> 40) & 0xFF) as u8;
    block[59] = ((bit_len >> 32) & 0xFF) as u8;
    block[60] = ((bit_len >> 24) & 0xFF) as u8;
    block[61] = ((bit_len >> 16) & 0xFF) as u8;
    block[62] = ((bit_len >> 8) & 0xFF) as u8;
    block[63] = (bit_len & 0xFF) as u8;
    sha1ProcessBlock(*state, *block);

    let out: [20]u8 = [0; 20];
    let k: usize = 0;
    while (k < 5) : (k += 1) {
        out[k * 4] = ((state[k] >> 24) & 0xFF) as u8;
        out[k * 4 + 1] = ((state[k] >> 16) & 0xFF) as u8;
        out[k * 4 + 2] = ((state[k] >> 8) & 0xFF) as u8;
        out[k * 4 + 3] = (state[k] & 0xFF) as u8;
    }
    return out;
}

/// Base64-encode a 20-byte SHA-1 digest into a 28-byte RFC 6455 accept token.
pub fn base64Encode20(src: [20]u8): [28]u8 {
    let out: [28]u8 = [0; 28];
    let i: usize = 0;
    let o: usize = 0;
    while (i + 3 <= 20) : (i += 3) {
        const v: u32 = ((src[i] as u32) << 16) | ((src[i + 1] as u32) << 8) | (src[i + 2] as u32);
        out[o] = B64_TABLE[((v >> 18) & 0x3F) as usize];
        out[o + 1] = B64_TABLE[((v >> 12) & 0x3F) as usize];
        out[o + 2] = B64_TABLE[((v >> 6) & 0x3F) as usize];
        out[o + 3] = B64_TABLE[(v & 0x3F) as usize];
        o += 4;
    }
    // Remaining 2 bytes (indices 18, 19)
    const v_rem: u32 = ((src[18] as u32) << 16) | ((src[19] as u32) << 8);
    out[24] = B64_TABLE[((v_rem >> 18) & 0x3F) as usize];
    out[25] = B64_TABLE[((v_rem >> 12) & 0x3F) as usize];
    out[26] = B64_TABLE[((v_rem >> 6) & 0x3F) as usize];
    out[27] = '=';
    return out;
}

/// Compute RFC 6455 `Sec-WebSocket-Accept` (28 bytes) from `Sec-WebSocket-Key`.
pub fn computeWebSocketAccept(client_key: string): [28]u8 {
    let combined: [128]u8 = undefined;
    let klen = client_key.len;
    if (klen > 92) klen = 92;
    @memcpy(combined[0..klen], client_key[0..klen]);
    @memcpy(combined[klen .. klen + WS_GUID.len], WS_GUID);
    const digest = sha1Digest(combined[0 .. klen + WS_GUID.len]);
    return base64Encode20(digest);
}

// ============================================================================
// RFC 6455 Handshake Parsing & Frame Processing
// ============================================================================

/// Parses an HTTP/1.1 WebSocket Upgrade request.
/// Returns:
///   > 0: consumed handshake bytes, and writes 101 Switching Protocols response into `out_resp` (`out_resp_len`)
///     0: incomplete HTTP headers (wait for more bytes)
///    -1: malformed or missing `Sec-WebSocket-Key`
fn tryHandleWsUpgrade(raw: string, out_resp: []u8, out_resp_len: *usize): isize {
    const line_end = wsFindCrlf(raw, 0) ?? return 0;
    let pos = line_end + 2;
    let headers_done = false;
    let key_start: usize = 0;
    let key_len: usize = 0;

    while (pos < raw.len) {
        const next_crlf = wsFindCrlf(raw, pos) ?? return 0;
        if (next_crlf == pos) {
            pos = next_crlf + 2;
            headers_done = true;
            break;
        }
        const hline = raw[pos..next_crlf];
        let colon: usize = 0;
        while (colon < hline.len && hline[colon] != ':') : (colon += 1) {}
        if (colon > 0 && colon < hline.len) {
            const hname = hline[0..colon];
            if (wsAsciiEqlIgnoreCase(hname, "Sec-WebSocket-Key")) {
                let vstart = colon + 1;
                while (vstart < hline.len && (hline[vstart] == ' ' || hline[vstart] == '\t')) : (vstart += 1) {}
                let vend = hline.len;
                while (vend > vstart && (hline[vend - 1] == ' ' || hline[vend - 1] == '\t')) : (vend -= 1) {}
                key_start = pos + vstart;
                key_len = vend - vstart;
            }
        }
        pos = next_crlf + 2;
    }

    if (!headers_done) return 0;
    if (key_len == 0) return -1;

    const accept = computeWebSocketAccept(raw[key_start .. key_start + key_len]);
    let wpos: usize = 0;
    * = wsWriteBytes(out_resp, *wpos, "HTTP/1.1 101 Switching Protocols\r\n");
    * = wsWriteBytes(out_resp, *wpos, "Upgrade: websocket\r\n");
    * = wsWriteBytes(out_resp, *wpos, "Connection: Upgrade\r\n");
    * = wsWriteBytes(out_resp, *wpos, "Sec-WebSocket-Accept: ");
    * = wsWriteBytes(out_resp, *wpos, accept[0..28]);
    * = wsWriteBytes(out_resp, *wpos, "\r\n\r\n");
    out_resp_len.* = wpos;
    return pos as isize;
}

/// Encodes an unmasked server-to-client WebSocket frame header into `out_buf` and returns header length.
fn writeServerFrameHeader(out_buf: []u8, opcode: u8, payload_len: usize): usize {
    out_buf[0] = (0x80 | (opcode & 0x0F)) as u8;
    if (payload_len < 126) {
        out_buf[1] = payload_len as u8;
        return 2;
    } else if (payload_len <= 65535) {
        out_buf[1] = 126;
        out_buf[2] = ((payload_len >> 8) & 0xFF) as u8;
        out_buf[3] = (payload_len & 0xFF) as u8;
        return 4;
    } else {
        out_buf[1] = 127;
        const p64 = payload_len as u64;
        out_buf[2] = ((p64 >> 56) & 0xFF) as u8;
        out_buf[3] = ((p64 >> 48) & 0xFF) as u8;
        out_buf[4] = ((p64 >> 40) & 0xFF) as u8;
        out_buf[5] = ((p64 >> 32) & 0xFF) as u8;
        out_buf[6] = ((p64 >> 24) & 0xFF) as u8;
        out_buf[7] = ((p64 >> 16) & 0xFF) as u8;
        out_buf[8] = ((p64 >> 8) & 0xFF) as u8;
        out_buf[9] = (p64 & 0xFF) as u8;
        return 10;
    }
}

// ============================================================================
// Multi-Core SO_REUSEPORT + epoll Worker Loop
// ============================================================================

struct WsConnBuf {
    fd: i32 = -1;
    upgraded: bool = false;
    read_buf: [WS_READ_BUF_LEN]u8 = undefined;
    read_len: usize = 0;
}

struct WsWorkerCtx {
    port: u16,
    backlog: i32,
    worker_id: usize,
    pin_cpu: bool,
    broadcast_mode: bool,
    stop_flag: *i32,
    ready_count: *i32,
}

fn wsRemoveSub(subs: *[512]i32, sub_len: *usize, fd: i32) {
    let i: usize = 0;
    while (i < sub_len.*) : (i += 1) {
        if (subs[i] == fd) {
            sub_len.* -= 1;
            subs[i] = subs[sub_len.*];
            return;
        }
    }
}

fn wsFlushBroadcast(subs: *[512]i32, sub_len: usize, payload: string) {
    if (payload.len == 0 || sub_len == 0) return;
    let si: usize = 0;
    while (si < sub_len) : (si += 1) {
        * = wsWriteAllFd(subs[si], payload);
    }
}

fn wsPinCurrentThreadToCpu(cpu_id: usize) {
    let mask: [16]u64 = [0; 16]; // up to 1024 CPUs
    const word_idx = (cpu_id / 64) % 16;
    const bit_idx = (cpu_id % 64) as u6;
    mask[word_idx] = (1 as u64) << bit_idx;
    unsafe {
        * = sched_setaffinity(0, 128, @ptrCast(&mask[0]));
    }
}

fn wsCreateReusePortListener(port: u16, backlog: i32): i32 {
    const fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;

    let one: i32 = 1;
    unsafe {
        * = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, @ptrCast(*one), 4);
        * = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, @ptrCast(*one), 4);
    }

    let addr = WsSockAddrIn{
        sin_family: AF_INET as u16,
        sin_port: wsHtons(port),
        sin_addr: 0, // INADDR_ANY
        sin_zero: [0; 8],
    };
    if (bind(fd, *addr, 16) < 0) {
        * = close(fd);
        return -1;
    }
    if (listen(fd, backlog) < 0) {
        * = close(fd);
        return -1;
    }
    return fd;
}

fn wsWriteAllFd(fd: i32, data: string): bool {
    let sent: usize = 0;
    let retries: usize = 0;
    while (sent < data.len) {
        const n = write(fd, data.ptr + sent, data.len - sent);
        if (n > 0) {
            sent += n as usize;
            retries = 0;
        } else {
            const err = __errno_location().*;
            if (err == 11 || err == 4) { // EAGAIN or EINTR
                retries += 1;
                if (retries > 4096) return false;
                * = sched_yield();
            } else {
                return false;
            }
        }
    }
    return true;
}

@c fn wsWorkerThreadMain(arg: *u8?): *u8? {
    if (arg == null) return null;
    unsafe {
        const wctx: *WsWorkerCtx = @ptrCast(@alignCast(arg.?));
        if (wctx.pin_cpu) {
            wsPinCurrentThreadToCpu(wctx.worker_id);
        }
        const lfd = wsCreateReusePortListener(wctx.port, wctx.backlog);
        if (lfd < 0) return null;

        const epfd = epoll_create1(EPOLL_CLOEXEC);
        if (epfd < 0) {
            * = close(lfd);
            return null;
        }

        let lev = WsEpollEventC{
            events: EPOLLIN,
            data_lo: lfd as u32,
            data_hi: 0,
        };
        * = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, *lev);

        // Signal worker ready
        * = @atomicRmw(i32, wctx.ready_count, .Add, 1, .seq_cst);

        // Pre-allocate flat per-worker connection table (0 heap alloc per frame)
        const max_fds: usize = 512;
        const conn_raw = malloc(max_fds * @sizeOf(WsConnBuf));
        if (conn_raw == null) {
            * = close(epfd);
            * = close(lfd);
            return null;
        }
        const conns: [*]WsConnBuf = @ptrCast(@alignCast(conn_raw.?));
        let ci: usize = 0;
        while (ci < max_fds) : (ci += 1) {
            conns[ci].fd = -1;
            conns[ci].upgraded = false;
            conns[ci].read_len = 0;
        }

        let events: [WS_EPOLL_BATCH]WsEpollEventC = undefined;
        let write_buf: [WS_WRITE_BUF_LEN]u8 = undefined;
        let pub_buf: [65536]u8 = undefined;
        let subs: [512]i32 = undefined;
        let sub_len: usize = 0;
        let one: i32 = 1;

        while (@atomicLoad(i32, wctx.stop_flag, .acquire) == 0) {
            const nev = epoll_wait(epfd, &events[0], WS_EPOLL_BATCH as i32, 50);
            if (nev <= 0) continue;

            let pub_len: usize = 0;
            let ei: usize = 0;
            while (ei < (nev as usize)) : (ei += 1) {
                const ev = events[ei];
                const fd = ev.data_lo as i32;

                if (fd == lfd) {
                    while (true) {
                        const cfd = accept4(lfd, null, null, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (cfd < 0) break;
                        if ((cfd as usize) >= max_fds) {
                            * = close(cfd);
                            continue;
                        }
                        * = setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, @ptrCast(*one), 4);
                        conns[cfd as usize].fd = cfd;
                        conns[cfd as usize].upgraded = false;
                        conns[cfd as usize].read_len = 0;
                        let cev = WsEpollEventC{
                            events: EPOLLIN | EPOLLRDHUP,
                            data_lo: cfd as u32,
                            data_hi: 0,
                        };
                        * = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, *cev);
                    }
                } else {
                    const idx = fd as usize;
                    if (idx >= max_fds) {
                        * = close(fd);
                        continue;
                    }
                    if ((ev.events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) != 0) {
                        if (wctx.broadcast_mode && conns[idx].upgraded) {
                            wsRemoveSub(*subs, *sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].upgraded = false;
                        conns[idx].read_len = 0;
                        continue;
                    }

                    const rem = WS_READ_BUF_LEN - conns[idx].read_len;
                    if (rem == 0) {
                        if (wctx.broadcast_mode && conns[idx].upgraded) {
                            wsRemoveSub(*subs, *sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].upgraded = false;
                        conns[idx].read_len = 0;
                        continue;
                    }

                    const nread = read(fd, &conns[idx].read_buf[conns[idx].read_len], rem);
                    if (nread < 0) {
                        const err = __errno_location().*;
                        if (err == 11 || err == 4) continue;
                        if (wctx.broadcast_mode && conns[idx].upgraded) {
                            wsRemoveSub(*subs, *sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].upgraded = false;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    if (nread == 0) {
                        if (wctx.broadcast_mode && conns[idx].upgraded) {
                            wsRemoveSub(*subs, *sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].upgraded = false;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    conns[idx].read_len += nread as usize;

                    let offset: usize = 0;
                    let should_close = false;

                    // Stage 1: RFC 6455 HTTP Upgrade Handshake
                    if (!conns[idx].upgraded) {
                        let resp_len: usize = 0;
                        const slice = conns[idx].read_buf[0..conns[idx].read_len];
                        const consumed = tryHandleWsUpgrade(slice, write_buf[0..], *resp_len);
                        if (consumed < 0) {
                            should_close = true;
                        } else if (consumed > 0) {
                            if (!wsWriteAllFd(fd, write_buf[0..resp_len])) {
                                should_close = true;
                            } else {
                                conns[idx].upgraded = true;
                                if (wctx.broadcast_mode && sub_len < 512) {
                                    subs[sub_len] = fd;
                                    sub_len += 1;
                                }
                                offset = consumed as usize;
                            }
                        }
                    }

                    // Stage 2: Process WebSocket frames
                    if (!should_close && conns[idx].upgraded) {
                        // Zero-copy single-frame fast path:
                        // Client masked frames carry a 4-byte mask key, so server unmasked header
                        // is always 4 bytes shorter and fits directly before the payload in read_buf.
                        let handled_fast = false;
                        if (!wctx.broadcast_mode && offset == 0 && conns[idx].read_len >= 6) {
                            const fb0 = conns[idx].read_buf[0];
                            const fb1 = conns[idx].read_buf[1];
                            const fop = fb0 & 0x0F;
                            if ((fb1 & 0x80) != 0 && (fop == 0x1 || fop == 0x2 || fop == 0x9)) {
                                const flen7 = (fb1 & 0x7F) as usize;
                                const fout_op: u8 = if (fop == 0x9) 0xA else fop;
                                if (flen7 < 126 && conns[idx].read_len == 6 + flen7) {
                                    const fmask: [4]u8 = [
                                        conns[idx].read_buf[2],
                                        conns[idx].read_buf[3],
                                        conns[idx].read_buf[4],
                                        conns[idx].read_buf[5],
                                    ];
                                    simdMaskXorInPlace(conns[idx].read_buf[6 .. 6 + flen7], fmask);
                                    conns[idx].read_buf[4] = (0x80 | fout_op) as u8;
                                    conns[idx].read_buf[5] = flen7 as u8;
                                    if (!wsWriteAllFd(fd, conns[idx].read_buf[4 .. 6 + flen7])) {
                                        should_close = true;
                                    }
                                    conns[idx].read_len = 0;
                                    handled_fast = true;
                                } else if (flen7 == 126 && conns[idx].read_len >= 8) {
                                    const fplen = ((conns[idx].read_buf[2] as usize) << 8)
                                        | (conns[idx].read_buf[3] as usize);
                                    if (conns[idx].read_len == 8 + fplen) {
                                        const fmask: [4]u8 = [
                                            conns[idx].read_buf[4],
                                            conns[idx].read_buf[5],
                                            conns[idx].read_buf[6],
                                            conns[idx].read_buf[7],
                                        ];
                                        simdMaskXorInPlace(conns[idx].read_buf[8 .. 8 + fplen], fmask);
                                        conns[idx].read_buf[4] = (0x80 | fout_op) as u8;
                                        conns[idx].read_buf[5] = 126;
                                        conns[idx].read_buf[6] = ((fplen >> 8) & 0xFF) as u8;
                                        conns[idx].read_buf[7] = (fplen & 0xFF) as u8;
                                        if (!wsWriteAllFd(fd, conns[idx].read_buf[4 .. 8 + fplen])) {
                                            should_close = true;
                                        }
                                        conns[idx].read_len = 0;
                                        handled_fast = true;
                                    }
                                }
                            }
                        }

                        if (!handled_fast) {
                        let write_pos: usize = 0;
                        while (offset < conns[idx].read_len) {
                            const avail = conns[idx].read_len - offset;
                            if (avail < 2) break;

                            const b0 = conns[idx].read_buf[offset];
                            const b1 = conns[idx].read_buf[offset + 1];
                            const opcode = b0 & 0x0F;
                            const masked = (b1 & 0x80) != 0;
                            const len7 = (b1 & 0x7F) as usize;

                            let hdr_len: usize = 2;
                            let payload_len: usize = len7;
                            if (len7 == 126) {
                                if (avail < 4) break;
                                payload_len = ((conns[idx].read_buf[offset + 2] as usize) << 8)
                                    | (conns[idx].read_buf[offset + 3] as usize);
                                hdr_len = 4;
                            } else if (len7 == 127) {
                                if (avail < 10) break;
                                const p64: u64 = ((conns[idx].read_buf[offset + 2] as u64) << 56)
                                    | ((conns[idx].read_buf[offset + 3] as u64) << 48)
                                    | ((conns[idx].read_buf[offset + 4] as u64) << 40)
                                    | ((conns[idx].read_buf[offset + 5] as u64) << 32)
                                    | ((conns[idx].read_buf[offset + 6] as u64) << 24)
                                    | ((conns[idx].read_buf[offset + 7] as u64) << 16)
                                    | ((conns[idx].read_buf[offset + 8] as u64) << 8)
                                    | (conns[idx].read_buf[offset + 9] as u64);
                                payload_len = p64 as usize;
                                hdr_len = 10;
                            }

                            let mask_key: [4]u8 = undefined;
                            if (masked) {
                                if (avail < hdr_len + 4) break;
                                mask_key[0] = conns[idx].read_buf[offset + hdr_len];
                                mask_key[1] = conns[idx].read_buf[offset + hdr_len + 1];
                                mask_key[2] = conns[idx].read_buf[offset + hdr_len + 2];
                                mask_key[3] = conns[idx].read_buf[offset + hdr_len + 3];
                                hdr_len += 4;
                            }

                            if (payload_len > WS_WRITE_BUF_LEN - 16) {
                                should_close = true;
                                break;
                            }
                            const frame_len = hdr_len + payload_len;
                            if (avail < frame_len) break;

                            const payload_start = offset + hdr_len;
                            const payload_slice = conns[idx].read_buf[payload_start .. payload_start + payload_len];

                            if (opcode == 0x1 || opcode == 0x2 || opcode == 0x9 || opcode == 0x8) {
                                let out_opcode: u8 = opcode;
                                if (opcode == 0x9) {
                                    out_opcode = 0xA; // Ping -> Pong
                                }
                                let need_hdr: usize = 2;
                                if (payload_len >= 126 && payload_len <= 65535) {
                                    need_hdr = 4;
                                } else if (payload_len > 65535) {
                                    need_hdr = 10;
                                }
                                if (wctx.broadcast_mode && (opcode == 0x1 || opcode == 0x2)) {
                                    if (pub_len + need_hdr + payload_len > 65536) {
                                        wsFlushBroadcast(*subs, sub_len, pub_buf[0..pub_len]);
                                        pub_len = 0;
                                    }
                                    const phdr = writeServerFrameHeader(pub_buf[pub_len..], out_opcode, payload_len);
                                    if (masked) {
                                        simdMaskXorCopy(
                                            pub_buf[pub_len + phdr .. pub_len + phdr + payload_len],
                                            payload_slice,
                                            mask_key,
                                        );
                                    } else {
                                        let pi: usize = 0;
                                        while (pi < payload_len) : (pi += 1) {
                                            pub_buf[pub_len + phdr + pi] = payload_slice[pi];
                                        }
                                    }
                                    pub_len += phdr + payload_len;
                                } else {
                                    if (write_pos + need_hdr + payload_len > WS_WRITE_BUF_LEN) {
                                        if (!wsWriteAllFd(fd, write_buf[0..write_pos])) {
                                            write_pos = 0;
                                            should_close = true;
                                            break;
                                        }
                                        write_pos = 0;
                                    }
                                    const out_hdr = writeServerFrameHeader(write_buf[write_pos..], out_opcode, payload_len);
                                    if (masked) {
                                        simdMaskXorCopy(
                                            write_buf[write_pos + out_hdr .. write_pos + out_hdr + payload_len],
                                            payload_slice,
                                            mask_key,
                                        );
                                    } else {
                                        let pi: usize = 0;
                                        while (pi < payload_len) : (pi += 1) {
                                            write_buf[write_pos + out_hdr + pi] = payload_slice[pi];
                                        }
                                    }
                                    write_pos += out_hdr + payload_len;
                                }

                                if (opcode == 0x8) {
                                    offset += frame_len;
                                    should_close = true;
                                    break;
                                }
                            }
                            // opcode 0xA (Pong) or continuation: consume silently
                            offset += frame_len;
                        }

                        // Flush all aggregated response frames in a single write() syscall
                        if (write_pos > 0) {
                            if (!wsWriteAllFd(fd, write_buf[0..write_pos])) {
                                should_close = true;
                            }
                        }
                        }
                    }

                    if (should_close) {
                        if (wctx.broadcast_mode && conns[idx].upgraded) {
                            wsRemoveSub(*subs, *sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].upgraded = false;
                        conns[idx].read_len = 0;
                    } else if (offset > 0) {
                        const leftover = conns[idx].read_len - offset;
                        let k: usize = 0;
                        while (k < leftover) : (k += 1) {
                            conns[idx].read_buf[k] = conns[idx].read_buf[offset + k];
                        }
                        conns[idx].read_len = leftover;
                    }
                }
            }

            // Flush all aggregated topic-broadcast frames across the entire epoll batch in 1 write per subscriber!
            if (wctx.broadcast_mode && pub_len > 0) {
                wsFlushBroadcast(*subs, sub_len, pub_buf[0..pub_len]);
            }
        }

        // Cleanup all open sockets
        ci = 0;
        while (ci < max_fds) : (ci += 1) {
            if (conns[ci].fd >= 0) {
                * = close(conns[ci].fd);
            }
        }
        free(conn_raw);
        * = close(epfd);
        * = close(lfd);
    }
    return null;
}

pub struct WsServerConfig {
    port: u16 = 3000;
    workers: usize = 1;
    backlog: i32 = 4096;
    pin_cpu: bool = true;
    broadcast_mode: bool = false;
}

pub struct WsServer {
    config: WsServerConfig = WsServerConfig{},
    threads: [WS_MAX_WORKERS]usize = [0; WS_MAX_WORKERS];
    worker_ctxs: [WS_MAX_WORKERS]WsWorkerCtx = undefined;
    worker_count: usize = 0;
    stop_flag: i32 = 0;
    ready_count: i32 = 0;

    pub static fn init(config: WsServerConfig): WsServer {
        return WsServer{
            config: config,
            threads: [0; WS_MAX_WORKERS],
            worker_ctxs: undefined,
            worker_count: 0,
            stop_flag: 0,
            ready_count: 0,
        };
    }

    /// Start N `SO_REUSEPORT` epoll worker threads and wait until all are listening.
    pub fn start(*_): bool {
        let num_workers: usize = _.config.workers;
        if (num_workers == 0) {
            num_workers = 1 as usize;
        }
        if (num_workers > WS_MAX_WORKERS) {
            num_workers = WS_MAX_WORKERS;
        }
        _.worker_count = num_workers;
        _.stop_flag = 0;
        _.ready_count = 0;

        let i: usize = 0;
        while (i < num_workers) : (i += 1) {
            _.worker_ctxs[i] = WsWorkerCtx{
                port: _.config.port,
                backlog: _.config.backlog,
                worker_id: i,
                pin_cpu: _.config.pin_cpu,
                broadcast_mode: _.config.broadcast_mode,
                stop_flag: *_.stop_flag,
                ready_count: *_.ready_count,
            };
            unsafe {
                const rc = pthread_create(
                    *_.threads[i],
                    null,
                    wsWorkerThreadMain,
                    @ptrCast(*_.worker_ctxs[i]),
                );
                if (rc != 0) return false;
            }
        }

        let spins: usize = 0;
        while (@atomicLoad(i32, *_.ready_count, .acquire) < (num_workers as i32) && spins < 10000000) : (spins += 1) {}
        return @atomicLoad(i32, *_.ready_count, .acquire) == (num_workers as i32);
    }

    /// Block until all worker threads exit.
    pub fn join(*_) {
        let i: usize = 0;
        while (i < _.worker_count) : (i += 1) {
            if (_.threads[i] != 0) {
                * = pthread_join(_.threads[i], null);
                _.threads[i] = 0;
            }
        }
    }

    /// Gracefully stop all worker threads and wait for clean socket teardown.
    pub fn stop(*_) {
        @atomicStore(i32, *_.stop_flag, 1, .release);
        _.join();
    }
}

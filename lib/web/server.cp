// Origin: Pure-CP Stdlib (Multi-Core SO_REUSEPORT + epoll Non-Blocking HTTP/1.1 Server: cap:web)
// ============================================================================
// High-Performance Native HTTP/1.1 Keep-Alive Server for `cap:web`:
//   - Multi-worker `SO_REUSEPORT` (1 epoll event loop per CPU worker, 0 lock contention)
//   - Zero heap allocation per HTTP request (pre-allocated flat connection slab)
//   - Zero-copy HTTP/1.1 request line, header & Content-Length body parsing
//   - Direct dispatch into `App.dispatch(req)` + single-syscall response serialization
//   - Graceful shutdown via atomic stop flag
// ============================================================================

const std = @import("std");
import { App, AppMethod, Request, Response, HeaderKV } from "./app.cp";

pub const SERVER_MAX_WORKERS: usize = 64;
pub const SERVER_READ_BUF_LEN: usize = 2048;
pub const SERVER_WRITE_BUF_LEN: usize = 4096;
pub const SERVER_EPOLL_BATCH: usize = 128;

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
const F_GETFL: i32 = 3;
const F_SETFL: i32 = 4;
const O_NONBLOCK: i32 = 2048;

const EPOLL_CLOEXEC: i32 = 524288;
const EPOLL_CTL_ADD: i32 = 1;
const EPOLL_CTL_DEL: i32 = 2;
const EPOLLIN: u32 = 0x001;
const EPOLLERR: u32 = 0x008;
const EPOLLHUP: u32 = 0x010;
const EPOLLRDHUP: u32 = 0x2000;

@c struct SockAddrIn {
    sin_family: u16,
    sin_port: u16,
    sin_addr: u32,
    sin_zero: [8]u8,
}

// Linux x86_64 struct epoll_event is 12 bytes (__attribute__((__packed__)): u32 events + u64 data).
// Using three u32 fields guarantees exact 12-byte stride with zero padding!
@c struct EpollEventC {
    events: u32,
    data_lo: u32,
    data_hi: u32,
}

extern fn socket(domain: i32, sock_type: i32, protocol: i32): i32;
extern fn setsockopt(fd: i32, level: i32, optname: i32, optval: *const u8, optlen: u32): i32;
extern fn bind(fd: i32, addr: *const SockAddrIn, addrlen: u32): i32;
extern fn listen(fd: i32, backlog: i32): i32;
extern fn accept4(fd: i32, addr: *u8?, addrlen: *u32?, flags: i32): i32;
extern fn epoll_create1(flags: i32): i32;
extern fn epoll_ctl(epfd: i32, op: i32, fd: i32, event: *EpollEventC?): i32;
extern fn epoll_wait(epfd: i32, events: *EpollEventC, maxevents: i32, timeout: i32): i32;
extern fn read(fd: i32, buf: *u8, count: usize): isize;
extern fn write(fd: i32, buf: [*]const u8, count: usize): isize;
extern fn close(fd: i32): i32;
extern fn fcntl(fd: i32, cmd: i32, arg: i32): i32;
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

fn htons(v: u16): u16 {
    return ((v & 0x00FF) << 8) | ((v >> 8) & 0x00FF);
}

fn srvAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn srvAsciiEqlIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (srvAsciiLower(a[i]) != srvAsciiLower(b[i])) return false;
    }
    return true;
}

fn srvParseUsize(s: string): usize? {
    if (s.len == 0) return null;
    let acc: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        acc = acc * 10 + ((c - '0') as usize);
    }
    return acc;
}

fn srvFindCrlf(buf: string, start: usize): usize? {
    if (start + 1 >= buf.len) return null;
    let i = start;
    while (i + 1 < buf.len) : (i += 1) {
        if (buf[i] == '\r' && buf[i + 1] == '\n') return i;
    }
    return null;
}

fn srvWriteBytes(dst: []u8, pos: *usize, s: string): bool {
    if (s.len > dst.len || pos.* > dst.len - s.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        dst[pos.* + i] = s[i];
    }
    pos.* += s.len;
    return true;
}

fn srvWriteUsize(dst: []u8, pos: *usize, val: usize): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        if (pos.* >= dst.len) return false;
        dst[pos.*] = '0';
        pos.* += 1;
        return true;
    }
    while (v > 0) {
        tmp[n] = (48 + ((v % 10) as u8)) as u8;
        n += 1;
        v = v / 10;
    }
    if (pos.* + n > dst.len) return false;
    while (n > 0) {
        n -= 1;
        dst[pos.*] = tmp[n];
        pos.* += 1;
    }
    return true;
}

fn statusReasonPhrase(code: u16): string {
    if (code == 200) return "OK";
    if (code == 201) return "Created";
    if (code == 204) return "No Content";
    if (code == 301) return "Moved Permanently";
    if (code == 302) return "Found";
    if (code == 304) return "Not Modified";
    if (code == 400) return "Bad Request";
    if (code == 401) return "Unauthorized";
    if (code == 403) return "Forbidden";
    if (code == 404) return "Not Found";
    if (code == 405) return "Method Not Allowed";
    if (code == 422) return "Unprocessable Entity";
    if (code == 429) return "Too Many Requests";
    if (code == 500) return "Internal Server Error";
    return "OK";
}

/// Zero-copy HTTP/1.1 request parser.
/// Returns `consumed_bytes` if a full request (headers + Content-Length body) is available,
/// or `0` if more bytes are needed, or `-1` on malformed HTTP.
fn parseHttp1Request(raw: string, out_req: *Request, out_keep_alive: *bool): isize {
    const line_end = srvFindCrlf(raw, 0) ?? return 0;
    const req_line = raw[0..line_end];

    // Parse METHOD SP TARGET SP HTTP/1.x
    let sp1: usize = 0;
    while (sp1 < req_line.len && req_line[sp1] != ' ') : (sp1 += 1) {}
    if (sp1 == 0 || sp1 >= req_line.len) return -1;
    const method_str = req_line[0..sp1];

    let sp2 = sp1 + 1;
    while (sp2 < req_line.len && req_line[sp2] != ' ') : (sp2 += 1) {}
    if (sp2 <= sp1 + 1 || sp2 >= req_line.len) return -1;
    const target_str = req_line[sp1 + 1 .. sp2];

    const method = AppMethod.fromString(method_str) ?? return -1;
    let req = Request.init(method, target_str);

    // Default HTTP/1.1 keep-alive = true
    let keep_alive = true;
    let content_len: usize = 0;

    let pos = line_end + 2;
    let headers_done = false;
    while (pos < raw.len) {
        const next_crlf = srvFindCrlf(raw, pos) ?? return 0;
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
            let vstart = colon + 1;
            while (vstart < hline.len && (hline[vstart] == ' ' || hline[vstart] == '\t')) : (vstart += 1) {}
            const hval = hline[vstart..];
            * = req.withHeader(hname, hval);
            if (srvAsciiEqlIgnoreCase(hname, "Content-Length")) {
                content_len = srvParseUsize(hval) ?? return -1;
            } else if (srvAsciiEqlIgnoreCase(hname, "Connection")) {
                if (srvAsciiEqlIgnoreCase(hval, "close")) {
                    keep_alive = false;
                }
            }
        }
        pos = next_crlf + 2;
    }

    if (!headers_done) return 0;
    if (pos + content_len > raw.len) return 0;

    if (content_len > 0) {
        * = req.withBody(raw[pos .. pos + content_len]);
    }
    out_req.* = req;
    out_keep_alive.* = keep_alive;
    return (pos + content_len) as isize;
}

/// Serialize `Response` into `out_buf` as a single contiguous HTTP/1.1 buffer.
fn serializeHttp1Response(resp: *const Response, keep_alive: bool, out_buf: []u8): usize {
    let pos: usize = 0;
    * = srvWriteBytes(out_buf, *pos, "HTTP/1.1 ");
    * = srvWriteUsize(out_buf, *pos, resp.status as usize);
    * = srvWriteBytes(out_buf, *pos, " ");
    * = srvWriteBytes(out_buf, *pos, statusReasonPhrase(resp.status));
    * = srvWriteBytes(out_buf, *pos, "\r\n");

    if (resp.content_type.len > 0) {
        * = srvWriteBytes(out_buf, *pos, "Content-Type: ");
        * = srvWriteBytes(out_buf, *pos, resp.content_type);
        * = srvWriteBytes(out_buf, *pos, "\r\n");
    }
    const actual_body = if (resp.body_buf_len > 0)
        resp.body_buf[0..resp.body_buf_len]
    else
        resp.body;

    * = srvWriteBytes(out_buf, *pos, "Content-Length: ");
    * = srvWriteUsize(out_buf, *pos, actual_body.len);
    * = srvWriteBytes(out_buf, *pos, "\r\n");

    if (keep_alive) {
        * = srvWriteBytes(out_buf, *pos, "Connection: keep-alive\r\n");
    } else {
        * = srvWriteBytes(out_buf, *pos, "Connection: close\r\n");
    }

    let i: usize = 0;
    while (i < resp.header_len) : (i += 1) {
        * = srvWriteBytes(out_buf, *pos, resp.headers[i].name);
        * = srvWriteBytes(out_buf, *pos, ": ");
        * = srvWriteBytes(out_buf, *pos, resp.headers[i].value);
        * = srvWriteBytes(out_buf, *pos, "\r\n");
    }

    * = srvWriteBytes(out_buf, *pos, "\r\n");
    if (actual_body.len > 0) {
        * = srvWriteBytes(out_buf, *pos, actual_body);
    }
    return pos;
}

struct ConnBuf {
    fd: i32 = -1;
    read_buf: [SERVER_READ_BUF_LEN]u8 = [0; SERVER_READ_BUF_LEN];
    read_len: usize = 0;
}

struct WorkerCtx {
    app: *App,
    port: u16,
    backlog: i32,
    worker_id: usize,
    pin_cpu: bool,
    stop_flag: *i32,
    ready_count: *i32,
}

fn pinCurrentThreadToCpu(cpu_id: usize) {
    let mask: [16]u64 = [0; 16]; // up to 1024 CPUs
    const word_idx = (cpu_id / 64) % 16;
    const bit_idx = (cpu_id % 64) as u6;
    mask[word_idx] = (1 as u64) << bit_idx;
    unsafe {
        * = sched_setaffinity(0, 128, @ptrCast(&mask[0]));
    }
}

fn createReusePortListener(port: u16, backlog: i32): i32 {
    const fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;

    let one: i32 = 1;
    unsafe {
        * = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, @ptrCast(*one), 4);
        * = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, @ptrCast(*one), 4);
    }

    let addr = SockAddrIn{
        sin_family: AF_INET as u16,
        sin_port: htons(port),
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

fn writeAllFd(fd: i32, data: string): bool {
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

@c fn workerThreadMain(arg: *u8?): *u8? {
    if (arg == null) return null;
    unsafe {
        const wctx: *WorkerCtx = @ptrCast(@alignCast(arg.?));
        if (wctx.pin_cpu) {
            pinCurrentThreadToCpu(wctx.worker_id);
        }
        const lfd = createReusePortListener(wctx.port, wctx.backlog);
        if (lfd < 0) return null;

        const epfd = epoll_create1(EPOLL_CLOEXEC);
        if (epfd < 0) {
            * = close(lfd);
            return null;
        }

        let lev = EpollEventC{
            events: EPOLLIN,
            data_lo: lfd as u32,
            data_hi: 0,
        };
        * = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, *lev);

        // Signal ready
        * = @atomicRmw(i32, wctx.ready_count, .Add, 1, .seq_cst);

        // Pre-allocate per-worker flat connection buffers (zero alloc per request)
        const max_fds: usize = 512;
        const conn_raw = malloc(max_fds * @sizeOf(ConnBuf));
        if (conn_raw == null) {
            * = close(epfd);
            * = close(lfd);
            return null;
        }
        const conns: [*]ConnBuf = @ptrCast(@alignCast(conn_raw.?));
        let ci: usize = 0;
        while (ci < max_fds) : (ci += 1) {
            conns[ci].fd = -1;
            conns[ci].read_len = 0;
        }

        let events: [SERVER_EPOLL_BATCH]EpollEventC = [EpollEventC{ events: 0, data_lo: 0, data_hi: 0 }; SERVER_EPOLL_BATCH];
        let write_buf: [SERVER_WRITE_BUF_LEN]u8 = [0; SERVER_WRITE_BUF_LEN];
        let one: i32 = 1;

        while (@atomicLoad(i32, wctx.stop_flag, .acquire) == 0) {
            const nev = epoll_wait(epfd, &events[0], SERVER_EPOLL_BATCH as i32, 50);
            if (nev <= 0) continue;

            let ei: usize = 0;
            while (ei < (nev as usize)) : (ei += 1) {
                const ev = events[ei];
                const fd = ev.data_lo as i32;

                if (fd == lfd) {
                    // Drain accept queue
                    while (true) {
                        const cfd = accept4(lfd, null, null, SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if (cfd < 0) break;
                        if ((cfd as usize) >= max_fds) {
                            * = close(cfd);
                            continue;
                        }
                        * = setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, @ptrCast(*one), 4);
                        conns[cfd as usize].fd = cfd;
                        conns[cfd as usize].read_len = 0;
                        let cev = EpollEventC{
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
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].read_len = 0;
                        continue;
                    }

                    // Non-blocking read into connection buffer
                    const rem = SERVER_READ_BUF_LEN - conns[idx].read_len;
                    if (rem == 0) {
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    const nread = read(fd, &conns[idx].read_buf[conns[idx].read_len], rem);
                    if (nread < 0) {
                        const err = __errno_location().*;
                        if (err == 11 || err == 4) { // EAGAIN or EINTR
                            continue;
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    if (nread == 0) {
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    conns[idx].read_len += nread as usize;

                    // Process all complete pipelined HTTP/1.1 requests in buffer
                    let offset: usize = 0;
                    let should_close = false;
                    while (offset < conns[idx].read_len) {
                        let req = Request{};
                        let keep_alive = true;
                        const slice = conns[idx].read_buf[offset..conns[idx].read_len];
                        const consumed = parseHttp1Request(slice, *req, *keep_alive);
                        if (consumed < 0) {
                            should_close = true;
                            break;
                        }
                        if (consumed == 0) break;

                        const resp = wctx.app.dispatch(req);
                        const wlen = serializeHttp1Response(*resp, keep_alive, write_buf[0..]);
                        if (!writeAllFd(fd, write_buf[0..wlen])) {
                            should_close = true;
                            break;
                        }
                        offset += consumed as usize;
                        if (!keep_alive) {
                            should_close = true;
                            break;
                        }
                    }

                    if (should_close) {
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
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

pub struct ServerConfig {
    port: u16 = 3000;
    workers: usize = 1;
    backlog: i32 = 4096;
    pin_cpu: bool = true;
}

pub struct WebServer {
    config: ServerConfig = ServerConfig{},
    threads: [SERVER_MAX_WORKERS]usize = [0; SERVER_MAX_WORKERS];
    worker_ctxs: [SERVER_MAX_WORKERS]WorkerCtx = undefined;
    worker_count: usize = 0;
    stop_flag: i32 = 0;
    ready_count: i32 = 0;

    pub static fn init(config: ServerConfig): WebServer {
        return WebServer{
            config: config,
            threads: [0; SERVER_MAX_WORKERS],
            worker_ctxs: undefined,
            worker_count: 0,
            stop_flag: 0,
            ready_count: 0,
        };
    }

    /// Start N `SO_REUSEPORT` epoll worker threads in the background and wait until all are listening.
    pub fn start(*_, app: *App): bool {
        let num_workers: usize = _.config.workers;
        if (num_workers == 0) {
            num_workers = 1 as usize;
        }
        if (num_workers > SERVER_MAX_WORKERS) {
            num_workers = SERVER_MAX_WORKERS;
        }
        _.worker_count = num_workers;
        _.stop_flag = 0;
        _.ready_count = 0;

        let i: usize = 0;
        while (i < num_workers) : (i += 1) {
            _.worker_ctxs[i] = WorkerCtx{
                app: app,
                port: _.config.port,
                backlog: _.config.backlog,
                worker_id: i,
                pin_cpu: _.config.pin_cpu,
                stop_flag: *_.stop_flag,
                ready_count: *_.ready_count,
            };
            unsafe {
                const rc = pthread_create(
                    *_.threads[i],
                    null,
                    workerThreadMain,
                    @ptrCast(*_.worker_ctxs[i]),
                );
                if (rc != 0) return false;
            }
        }

        // Wait until all workers have bound & entered epoll loop
        let spins: usize = 0;
        while (@atomicLoad(i32, *_.ready_count, .acquire) < (num_workers as i32) && spins < 10000000) : (spins += 1) {}
        return @atomicLoad(i32, *_.ready_count, .acquire) == (num_workers as i32);
    }

    /// Block until all worker threads exit (e.g. after `stop()` or signal).
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

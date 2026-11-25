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
import { App, AppMethod, Context, Request, Response, HeaderKV } from "./app.cp";
import { ComptimeRouteTable } from "./comptimeRouter.cp";

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
    const cr_vec: @Vector(16, u8) = @splat('\r');
    while (i + 16 < buf.len) : (i += 16) {
        const chunk: @Vector(16, u8) = buf[i .. i + 16][0..16].*;
        const eq = chunk == cr_vec;
        if (@reduce(.Or, eq)) {
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                if (buf[i + k] == '\r' && buf[i + k + 1] == '\n') return i + k;
            }
        }
    }
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

/// Zero-copy HTTP/1.1 request parser (writes directly in-place into `out_req`).
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
    out_req.method = method;
    let qpos: usize = target_str.len;
    let qi: usize = 0;
    while (qi < target_str.len) : (qi += 1) {
        if (target_str[qi] == '?') {
            qpos = qi;
            break;
        }
    }
    const pslice = target_str[0..qpos];
    out_req.path = if (pslice.len == 0) "/" else pslice;
    out_req.raw_query = if (qpos + 1 < target_str.len) target_str[qpos + 1 ..] else "";
    out_req.body = "";
    out_req.header_len = 0;

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
            * = out_req.withHeader(hname, hval);
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
        out_req.body = raw[pos .. pos + content_len];
    }
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
    is_ws: bool = false;
    read_buf: [SERVER_READ_BUF_LEN]u8 = undefined;
    read_len: usize = 0;
}

struct WorkerCtx {
    app: *App,
    comptime_table: *const ComptimeRouteTable?,
    port: u16,
    backlog: i32,
    worker_id: usize,
    pin_cpu: bool,
    ws_path: string,
    ws_broadcast_mode: bool,
    shutdown_grace_ms: u32,
    stop_flag: *i32,
    ready_count: *i32,
}

fn srvBytesEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn srvPathMatchesWs(target: string, ws_path: string): bool {
    if (ws_path.len == 0) return false;
    let qpos: usize = target.len;
    let i: usize = 0;
    while (i < target.len) : (i += 1) {
        if (target[i] == '?') {
            qpos = i;
            break;
        }
    }
    return srvBytesEql(target[0..qpos], ws_path);
}

const SRV_WS_GUID: string = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const SRV_B64_TABLE: string = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

fn srvRotl32(x: u32, comptime n: u5): u32 {
    return (x << n) | (x >> (32 - (n as u6)));
}

fn srvSha1ProcessBlock(state: *[5]u32, block: *const [64]u8) {
    let w: [80]u32 = undefined;
    let i: usize = 0;
    while (i < 16) : (i += 1) {
        w[i] = ((block.*[i * 4] as u32) << 24)
            | ((block.*[i * 4 + 1] as u32) << 16)
            | ((block.*[i * 4 + 2] as u32) << 8)
            | (block.*[i * 4 + 3] as u32);
    }
    while (i < 80) : (i += 1) {
        w[i] = srvRotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
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
        const temp = srvRotl32(a, 5) +% f +% e +% k +% w[i];
        e = d;
        d = c;
        c = srvRotl32(b, 30);
        b = a;
        a = temp;
    }
    state.*[0] = state.*[0] +% a;
    state.*[1] = state.*[1] +% b;
    state.*[2] = state.*[2] +% c;
    state.*[3] = state.*[3] +% d;
    state.*[4] = state.*[4] +% e;
}

fn srvComputeWsAccept(client_key: string): [28]u8 {
    let combined: [128]u8 = undefined;
    let klen = client_key.len;
    if (klen > 92) klen = 92;
    let i: usize = 0;
    while (i < klen) : (i += 1) {
        combined[i] = client_key[i];
    }
    let j: usize = 0;
    while (j < SRV_WS_GUID.len) : (j += 1) {
        combined[klen + j] = SRV_WS_GUID[j];
    }
    const data = combined[0 .. klen + SRV_WS_GUID.len];
    let state: [5]u32 = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0];
    let block: [64]u8 = [0; 64];
    let offset: usize = 0;
    while (offset + 64 <= data.len) : (offset += 64) {
        let b: usize = 0;
        while (b < 64) : (b += 1) {
            block[b] = data[offset + b];
        }
        srvSha1ProcessBlock(*state, *block);
    }
    const rem = data.len - offset;
    let b: usize = 0;
    while (b < 64) : (b += 1) {
        block[b] = 0;
    }
    b = 0;
    while (b < rem) : (b += 1) {
        block[b] = data[offset + b];
    }
    block[rem] = 0x80;
    if (rem >= 56) {
        srvSha1ProcessBlock(*state, *block);
        b = 0;
        while (b < 64) : (b += 1) {
            block[b] = 0;
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
    srvSha1ProcessBlock(*state, *block);

    let dig: [20]u8 = undefined;
    let k: usize = 0;
    while (k < 5) : (k += 1) {
        dig[k * 4] = ((state[k] >> 24) & 0xFF) as u8;
        dig[k * 4 + 1] = ((state[k] >> 16) & 0xFF) as u8;
        dig[k * 4 + 2] = ((state[k] >> 8) & 0xFF) as u8;
        dig[k * 4 + 3] = (state[k] & 0xFF) as u8;
    }
    let out: [28]u8 = undefined;
    let o: usize = 0;
    k = 0;
    while (k + 3 <= 20) : (k += 3) {
        const v: u32 = ((dig[k] as u32) << 16) | ((dig[k + 1] as u32) << 8) | (dig[k + 2] as u32);
        out[o] = SRV_B64_TABLE[((v >> 18) & 0x3F) as usize];
        out[o + 1] = SRV_B64_TABLE[((v >> 12) & 0x3F) as usize];
        out[o + 2] = SRV_B64_TABLE[((v >> 6) & 0x3F) as usize];
        out[o + 3] = SRV_B64_TABLE[(v & 0x3F) as usize];
        o += 4;
    }
    const vr: u32 = ((dig[18] as u32) << 16) | ((dig[19] as u32) << 8);
    out[24] = SRV_B64_TABLE[((vr >> 18) & 0x3F) as usize];
    out[25] = SRV_B64_TABLE[((vr >> 12) & 0x3F) as usize];
    out[26] = SRV_B64_TABLE[((vr >> 6) & 0x3F) as usize];
    out[27] = '=';
    return out;
}

fn srvSimdMaskXorCopy(dst: []u8, src: string, mask: [4]u8) {
    let i: usize = 0;
    const len = src.len;
    if (len >= 32) {
        let mv_arr: [32]u8 = undefined;
        let k: usize = 0;
        while (k < 32) : (k += 1) {
            mv_arr[k] = mask[k & 3];
        }
        const mvec: @Vector(32, u8) = mv_arr;
        while (i + 32 <= len) : (i += 32) {
            const svec: @Vector(32, u8) = src[i .. i + 32][0..32].*;
            const res: [32]u8 = svec ^ mvec;
            let c: usize = 0;
            while (c < 32) : (c += 1) {
                dst[i + c] = res[c];
            }
        }
    }
    while (i < len) : (i += 1) {
        dst[i] = src[i] ^ mask[i & 3];
    }
}

fn srvWriteWsUpgrade101(ws_key: string, out_buf: []u8): usize {
    const accept = srvComputeWsAccept(ws_key);
    let pos: usize = 0;
    * = srvWriteBytes(out_buf, *pos, "HTTP/1.1 101 Switching Protocols\r\n");
    * = srvWriteBytes(out_buf, *pos, "Upgrade: websocket\r\n");
    * = srvWriteBytes(out_buf, *pos, "Connection: Upgrade\r\n");
    * = srvWriteBytes(out_buf, *pos, "Sec-WebSocket-Accept: ");
    * = srvWriteBytes(out_buf, *pos, accept[0..28]);
    * = srvWriteBytes(out_buf, *pos, "\r\n\r\n");
    return pos;
}

fn srvWriteWsFrameHeader(out_buf: []u8, opcode: u8, payload_len: usize): usize {
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

fn srvAddWsSub(subs: *[512]i32, sub_len: *usize, fd: i32) {
    if (sub_len.* < 512) {
        subs.*[sub_len.*] = fd;
        sub_len.* += 1;
    }
}

fn srvRemoveWsSub(subs: *[512]i32, sub_len: *usize, fd: i32) {
    let i: usize = 0;
    while (i < sub_len.*) : (i += 1) {
        if (subs.*[i] == fd) {
            sub_len.* -= 1;
            subs.*[i] = subs.*[sub_len.*];
            return;
        }
    }
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
            conns[ci].is_ws = false;
            conns[ci].read_len = 0;
        }

        let events: [SERVER_EPOLL_BATCH]EpollEventC = undefined;
        let write_buf: [SERVER_WRITE_BUF_LEN]u8 = undefined;
        let ws_subs: [512]i32 = undefined;
        let ws_sub_len: usize = 0;
        let reusable_req: Request = undefined;
        let reusable_ctx: Context = undefined;
        let reusable_resp: Response = undefined;
        let one: i32 = 1;
        let listener_active = true;
        let drain_ticks: u32 = 0;
        const max_drain_ticks: u32 = if (wctx.shutdown_grace_ms > 0) ((wctx.shutdown_grace_ms + 9) / 10) else 0;

        while (true) {
            const stopping = @atomicLoad(i32, wctx.stop_flag, .acquire) != 0;
            if (stopping) {
                if (listener_active) {
                    * = epoll_ctl(epfd, EPOLL_CTL_DEL, lfd, null);
                    listener_active = false;
                }
                if (drain_ticks >= max_drain_ticks) break;
                drain_ticks += 1;
            }
            const wait_ms: i32 = if (stopping) 10 else 50;
            const nev = epoll_wait(epfd, &events[0], SERVER_EPOLL_BATCH as i32, wait_ms);
            if (nev <= 0) {
                if (stopping) break;
                continue;
            }

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
                        conns[cfd as usize].is_ws = false;
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
                        if (conns[idx].is_ws) {
                            srvRemoveWsSub(*ws_subs, *ws_sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].is_ws = false;
                        conns[idx].read_len = 0;
                        continue;
                    }

                    // Non-blocking read into connection buffer
                    const rem = SERVER_READ_BUF_LEN - conns[idx].read_len;
                    if (rem == 0) {
                        if (conns[idx].is_ws) {
                            srvRemoveWsSub(*ws_subs, *ws_sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].is_ws = false;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    const nread = read(fd, &conns[idx].read_buf[conns[idx].read_len], rem);
                    if (nread < 0) {
                        const err = __errno_location().*;
                        if (err == 11 || err == 4) { // EAGAIN or EINTR
                            continue;
                        }
                        if (conns[idx].is_ws) {
                            srvRemoveWsSub(*ws_subs, *ws_sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].is_ws = false;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    if (nread == 0) {
                        if (conns[idx].is_ws) {
                            srvRemoveWsSub(*ws_subs, *ws_sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].is_ws = false;
                        conns[idx].read_len = 0;
                        continue;
                    }
                    conns[idx].read_len += nread as usize;

                    let offset: usize = 0;
                    let should_close = false;

                    if (conns[idx].is_ws) {
                        // Same-port upgraded WebSocket connection (RFC 6455 + 32B SIMD unmask)
                        let wpos: usize = 0;
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
                                payload_len = ((conns[idx].read_buf[offset + 8] as usize) << 8)
                                    | (conns[idx].read_buf[offset + 9] as usize);
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
                            if (payload_len > SERVER_WRITE_BUF_LEN - 16) {
                                should_close = true;
                                break;
                            }
                            const frame_len = hdr_len + payload_len;
                            if (avail < frame_len) break;

                            if (opcode == 0x1 || opcode == 0x2 || opcode == 0x9 || opcode == 0x8) {
                                const out_op: u8 = if (opcode == 0x9) 0xA else opcode;
                                if (wpos + 12 + payload_len > SERVER_WRITE_BUF_LEN) {
                                    if (wctx.ws_broadcast_mode && (opcode == 0x1 || opcode == 0x2)) {
                                        let si: usize = 0;
                                        while (si < ws_sub_len) : (si += 1) {
                                            * = writeAllFd(ws_subs[si], write_buf[0..wpos]);
                                        }
                                    } else if (!writeAllFd(fd, write_buf[0..wpos])) {
                                        should_close = true;
                                        break;
                                    }
                                    wpos = 0;
                                }
                                const oh = srvWriteWsFrameHeader(write_buf[wpos..], out_op, payload_len);
                                const pslice = conns[idx].read_buf[offset + hdr_len .. offset + frame_len];
                                if (masked) {
                                    srvSimdMaskXorCopy(write_buf[wpos + oh .. wpos + oh + payload_len], pslice, mask_key);
                                } else {
                                    let pi: usize = 0;
                                    while (pi < payload_len) : (pi += 1) {
                                        write_buf[wpos + oh + pi] = pslice[pi];
                                    }
                                }
                                wpos += oh + payload_len;
                                if (opcode == 0x8) {
                                    offset += frame_len;
                                    should_close = true;
                                    break;
                                }
                            }
                            offset += frame_len;
                        }
                        if (wpos > 0) {
                            if (wctx.ws_broadcast_mode && !should_close) {
                                let si: usize = 0;
                                while (si < ws_sub_len) : (si += 1) {
                                    * = writeAllFd(ws_subs[si], write_buf[0..wpos]);
                                }
                            } else if (!writeAllFd(fd, write_buf[0..wpos])) {
                                should_close = true;
                            }
                        }
                    } else {
                        // Process all complete pipelined HTTP/1.1 requests in buffer with Cork write coalescing
                        let wtotal: usize = 0;
                        while (offset < conns[idx].read_len) {
                            let keep_alive = true;
                            const slice = conns[idx].read_buf[offset..conns[idx].read_len];
                            const consumed = parseHttp1Request(slice, *reusable_req, *keep_alive);
                            if (consumed < 0) {
                                should_close = true;
                                break;
                            }
                            if (consumed == 0) break;

                            // Same-port HTTP -> WebSocket Upgrade check
                            if (wctx.ws_path.len > 0 && srvBytesEql(reusable_req.path, wctx.ws_path)) {
                                if (ws_key := reusable_req.header("Sec-WebSocket-Key")) {
                                    if (wtotal > 0) {
                                        if (!writeAllFd(fd, write_buf[0..wtotal])) {
                                            should_close = true;
                                            break;
                                        }
                                        wtotal = 0;
                                    }
                                    const ulen = srvWriteWsUpgrade101(ws_key, write_buf[0..]);
                                    if (!writeAllFd(fd, write_buf[0..ulen])) {
                                        should_close = true;
                                        break;
                                    }
                                    conns[idx].is_ws = true;
                                    srvAddWsSub(*ws_subs, *ws_sub_len, fd);
                                    offset += consumed as usize;
                                    break;
                                }
                            }

                            // ComptimeRouteTable O(1) direct fast-path -> fallback to App.dispatchInto
                            let dispatched = false;
                            if (ct := wctx.comptime_table) {
                                dispatched = ct.dispatchInto(&reusable_req, *reusable_ctx, *reusable_resp);
                            }
                            if (!dispatched) {
                                wctx.app.dispatchInto(&reusable_req, *reusable_ctx, *reusable_resp);
                            }

                            if (wtotal + 1024 > SERVER_WRITE_BUF_LEN) {
                                if (!writeAllFd(fd, write_buf[0..wtotal])) {
                                    should_close = true;
                                    break;
                                }
                                wtotal = 0;
                            }
                            const wlen = serializeHttp1Response(*reusable_resp, keep_alive, write_buf[wtotal..]);
                            wtotal += wlen;
                            offset += consumed as usize;
                            if (!keep_alive) {
                                should_close = true;
                                break;
                            }
                        }
                        if (wtotal > 0) {
                            if (!writeAllFd(fd, write_buf[0..wtotal])) {
                                should_close = true;
                            }
                        }
                    }

                    if (should_close) {
                        if (conns[idx].is_ws) {
                            srvRemoveWsSub(*ws_subs, *ws_sub_len, fd);
                        }
                        * = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, null);
                        * = close(fd);
                        conns[idx].fd = -1;
                        conns[idx].is_ws = false;
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
    ws_path: string = "";
    ws_broadcast_mode: bool = false;
    shutdown_grace_ms: u32 = 200;
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
        return _.startInternal(app, null);
    }

    /// Start N `SO_REUSEPORT` epoll worker threads with an optional `ComptimeRouteTable` O(1) fast-path.
    pub fn startWithComptime(*_, app: *App, ct_table: *const ComptimeRouteTable): bool {
        return _.startInternal(app, ct_table);
    }

    fn startInternal(*_, app: *App, ct_table: *const ComptimeRouteTable?): bool {
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
                comptime_table: ct_table,
                port: _.config.port,
                backlog: _.config.backlog,
                worker_id: i,
                pin_cpu: _.config.pin_cpu,
                ws_path: _.config.ws_path,
                ws_broadcast_mode: _.config.ws_broadcast_mode,
                shutdown_grace_ms: _.config.shutdown_grace_ms,
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

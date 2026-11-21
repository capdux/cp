// Source: Absorbed from /data/dev/bun/src/uws/uws.cp | Synced: 2026-11-21T11:59:39Z
// lib/net/tcp_stream.cp
// High-throughput TCP Stream with Corking & StreamBuffer
// Absorbed from Bun us_socket_stream_buffer_t and SocketHandler

import { SocketKind, SocketCloseCode, SendStatus } from "./_usockets";
import { SocketAddress } from "./address";
import { SafeSocket } from "./socket";

pub const STREAM_CORK_BUFFER_SIZE: usize = 16384;

pub class StreamBuffer {
    pub buf: [16384]u8 = undefined;
    pub head: usize = 0;
    pub tail: usize = 0;

    pub static fn create(): *StreamBuffer {
        let ptr = std.heap.page_allocator.create(StreamBuffer) catch unreachable;
        ptr.head = 0;
        ptr.tail = 0;
        return ptr;
    }

    pub fn pendingLen(*_): usize {
        return _.tail - _.head;
    }

    pub fn availableSpace(*_): usize {
        return STREAM_CORK_BUFFER_SIZE - _.tail;
    }

    pub fn write(*_, data: []const u8): usize {
        if (_.head == _.tail) {
            _.head = 0;
            _.tail = 0;
        }
        const avail = _.availableSpace();
        const to_copy = if (data.len < avail) data.len else avail;
        let i: usize = 0;
        while (i < to_copy) {
            _.buf[_.tail + i] = data[i];
            i += 1;
        }
        _.tail += to_copy;
        return to_copy;
    }

    pub fn flushAll(*_): usize {
        const flushed = _.pendingLen();
        _.head = 0;
        _.tail = 0;
        return flushed;
    }

    pub fn reset(*_) {
        _.head = 0;
        _.tail = 0;
    }

    pub fn destroy(*_) {
        std.heap.page_allocator.destroy(_);
    }
}

pub class TcpStream {
    pub sock: *SafeSocket;
    pub local_addr: SocketAddress;
    pub remote_addr: SocketAddress;
    pub corked: bool = false;
    pub send_buf: *StreamBuffer;
    pub recv_buf: [16384]u8 = undefined;
    pub recv_len: usize = 0;

    pub static fn init(fd: i32, local_addr: SocketAddress, remote_addr: SocketAddress): *TcpStream {
        let ptr = std.heap.page_allocator.create(TcpStream) catch unreachable;
        ptr.sock = SafeSocket.create(fd, SocketKind.established_tcp);
        ptr.local_addr = local_addr;
        ptr.remote_addr = remote_addr;
        ptr.corked = false;
        ptr.send_buf = StreamBuffer.create();
        ptr.recv_len = 0;
        return ptr;
    }

    pub fn cork(*_) {
        if (_.sock.isClosed()) {
            return;
        }
        _.corked = true;
    }

    pub fn uncork(*_): usize {
        if (!_.corked) {
            return 0;
        }
        _.corked = false;
        return _.flush();
    }

    pub fn write(*_, data: []const u8): usize {
        if (_.sock.isClosed() || _.sock.write_shutdown) {
            return 0;
        }
        if (_.corked) {
            const buffered = _.send_buf.write(data);
            if (buffered == data.len) {
                return buffered;
            }
            let * = _.flush();
            const rem = data.len - buffered;
            let * = _.sock.recordWrite(rem);
            return data.len;
        }
        const status = _.sock.recordWrite(data.len);
        if (status == SendStatus.success) {
            return data.len;
        }
        return 0;
    }

    pub fn flush(*_): usize {
        const pending = _.send_buf.flushAll();
        if (pending > 0) {
            let * = _.sock.recordWrite(pending);
        }
        return pending;
    }

    pub fn feedIncoming(*_, data: []const u8): usize {
        if (_.sock.isClosed() || _.sock.read_shutdown) {
            return 0;
        }
        const space = STREAM_CORK_BUFFER_SIZE - _.recv_len;
        const count = if (data.len < space) data.len else space;
        let i: usize = 0;
        while (i < count) {
            _.recv_buf[_.recv_len + i] = data[i];
            i += 1;
        }
        _.recv_len += count;
        _.sock.recordRead(count);
        return count;
    }

    pub fn readAvailable(*_, out: []u8): usize {
        const count = if (out.len < _.recv_len) out.len else _.recv_len;
        let i: usize = 0;
        while (i < count) {
            out[i] = _.recv_buf[i];
            i += 1;
        }
        const rem = _.recv_len - count;
        let j: usize = 0;
        while (j < rem) {
            _.recv_buf[j] = _.recv_buf[count + j];
            j += 1;
        }
        _.recv_len = rem;
        return count;
    }

    pub fn setNoDelay(*_, enabled: bool) {
        _.sock.setNoDelay(enabled);
    }

    pub fn setKeepAlive(*_, enabled: bool, delay: u32): bool {
        return _.sock.setKeepAlive(enabled, delay);
    }

    pub fn setTimeout(*_, seconds: u32) {
        _.sock.setTimeout(seconds);
    }

    pub fn close(*_) {
        let * = _.flush();
        _.sock.close(SocketCloseCode.normal);
    }

    ~init() {
        _.close();
        _.send_buf.destroy();
        _.sock.~init();
        std.heap.page_allocator.destroy(_);
    }
}

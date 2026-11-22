// Source: Absorbed from /data/dev/bun/src/threading/channel.cp | Synced: 2026-11-22T11:56:52Z
// Thread-safe bounded channel in CP
// High-throughput CSP communication primitive
import { Mutex } from "./mutex";
import { Condition } from "./condition";

pub fn Channel(comptime T: type): type {
    return struct {
        const Self = @This();

        mutex: Mutex = .{},
        not_empty: Condition = .{},
        not_full: Condition = .{},
        buf: []T = undefined,
        head: usize = 0,
        tail: usize = 0,
        count: usize = 0,
        capacity: usize = 0,
        is_closed: bool = false,
        allocator: std.mem.Allocator = undefined,

        pub fn init(allocator: std.mem.Allocator, capacity: usize): !Self {
            const cap = if (capacity == 0) 1 else capacity;
            const buf = try allocator.alloc(T, cap);
            return Self{
                .mutex = .{},
                .not_empty = .{},
                .not_full = .{},
                .buf = buf,
                .head = 0,
                .tail = 0,
                .count = 0,
                .capacity = cap,
                .is_closed = false,
                .allocator = allocator,
            };
        }

        pub fn deinit(*_) {
            _.close();
            if (_.capacity > 0) {
                _.allocator.free(_.buf);
                _.buf = [];
                _.capacity = 0;
            }
            _.mutex.deinit();
            _.not_empty.deinit();
            _.not_full.deinit();
        }

        pub fn close(*_) {
            _.mutex.lock();
            defer _.mutex.unlock();

            if (_.is_closed) return;
            _.is_closed = true;
            _.not_full.broadcast();
            _.not_empty.broadcast();
        }

        pub fn isClosed(*_): bool {
            _.mutex.lock();
            defer _.mutex.unlock();
            return _.is_closed;
        }

        pub fn len(*_): usize {
            _.mutex.lock();
            defer _.mutex.unlock();
            return _.count;
        }

        pub fn send(*_, item: T): !void {
            _.mutex.lock();
            defer _.mutex.unlock();

            while (_.count == _.capacity and !_.is_closed) {
                _.not_full.wait(&_.mutex);
            }

            if (_.is_closed) {
                return error.Closed;
            }

            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % _.capacity;
            _.count += 1;
            _.not_empty.signal();
        }

        pub fn trySend(*_, item: T): !bool {
            _.mutex.lock();
            defer _.mutex.unlock();

            if (_.is_closed) return error.Closed;
            if (_.count == _.capacity) return false;

            _.buf[_.tail] = item;
            _.tail = (_.tail + 1) % _.capacity;
            _.count += 1;
            _.not_empty.signal();
            return true;
        }

        pub fn recv(*_): !?T {
            _.mutex.lock();
            defer _.mutex.unlock();

            while (_.count == 0 and !_.is_closed) {
                _.not_empty.wait(&_.mutex);
            }

            if (_.count == 0 and _.is_closed) {
                return null;
            }

            const item = _.buf[_.head];
            _.head = (_.head + 1) % _.capacity;
            _.count -= 1;
            _.not_full.signal();
            return item;
        }

        pub fn tryRecv(*_): T? {
            _.mutex.lock();
            defer _.mutex.unlock();

            if (_.count == 0) return null;

            const item = _.buf[_.head];
            _.head = (_.head + 1) % _.capacity;
            _.count -= 1;
            _.not_full.signal();
            return item;
        }
    };
}

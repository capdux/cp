// Source: cp/lib/lockfree_queue/channel.cp (Dynamic Capacity Bounded Channel)

const std = @import("std");

pub const ChannelError = error{
    ChannelClosed,
    QueueFull,
    InvalidCapacity,
    OutOfMemory,
};

fn lockMutex(m: *std.atomic.Mutex) {
    while (!m.tryLock()) {
        std.Thread.yield() catch {};
    }
}

pub fn BoundedChannel(comptime T: type): type {
    return struct {
        alloc: std.mem.Allocator,
        buffer: []T,
        cap: usize,
        head: usize,
        tail: usize,
        count: usize,
        closed: bool,
        mutex: std.atomic.Mutex,

        pub static fn init(alloc: std.mem.Allocator, cap: usize): ChannelError!@This() {
            if (cap == 0) {
                return Err(ChannelError.InvalidCapacity);
            }
            const buf = alloc.alloc(T, cap) catch return Err(ChannelError.OutOfMemory);
            return .{
                .alloc = alloc,
                .buffer = buf,
                .cap = cap,
                .head = 0,
                .tail = 0,
                .count = 0,
                .closed = false,
                .mutex = .unlocked,
            };
        }

        pub fn deinit(*_) {
            _.alloc.free(_.buffer);
        }

        pub fn send(*_, item: T): ChannelError!void {
            while (true) {
                lockMutex(&_.mutex);
                if (_.closed) {
                    _.mutex.unlock();
                    return Err(ChannelError.ChannelClosed);
                }
                if (_.count < _.cap) {
                    _.buffer[_.tail] = item;
                    _.tail = (_.tail + 1) % _.cap;
                    _.count += 1;
                    _.mutex.unlock();
                    return;
                }
                _.mutex.unlock();
                std.Thread.yield() catch {};
            }
        }

        pub fn trySend(*_, item: T): ChannelError!void {
            lockMutex(&_.mutex);
            defer _.mutex.unlock();

            if (_.closed) {
                return Err(ChannelError.ChannelClosed);
            }
            if (_.count == _.cap) {
                return Err(ChannelError.QueueFull);
            }

            _.buffer[_.tail] = item;
            _.tail = (_.tail + 1) % _.cap;
            _.count += 1;
        }

        pub fn recv(*_): T? {
            while (true) {
                lockMutex(&_.mutex);
                if (_.count > 0) {
                    const item = _.buffer[_.head];
                    _.head = (_.head + 1) % _.cap;
                    _.count -= 1;
                    _.mutex.unlock();
                    return item;
                }
                if (_.closed) {
                    _.mutex.unlock();
                    return null;
                }
                _.mutex.unlock();
                std.Thread.yield() catch {};
            }
        }

        pub fn tryRecv(*_): T? {
            lockMutex(&_.mutex);
            defer _.mutex.unlock();

            if (_.count == 0) {
                return null;
            }

            const item = _.buffer[_.head];
            _.head = (_.head + 1) % _.cap;
            _.count -= 1;
            return item;
        }

        pub fn close(*_) {
            lockMutex(&_.mutex);
            defer _.mutex.unlock();
            _.closed = true;
        }

        pub fn len(*_): usize {
            lockMutex(&_.mutex);
            defer _.mutex.unlock();
            return _.count;
        }

        pub fn isClosed(*_): bool {
            lockMutex(&_.mutex);
            defer _.mutex.unlock();
            return _.closed;
        }

        pub fn capacity(): usize {
            return _.cap;
        }
    };
}

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

        pub fn init(alloc: std.mem.Allocator, cap: usize): ChannelError!@This() {
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

        pub fn deinit(self: *@This()) {
            self.alloc.free(self.buffer);
        }

        pub fn send(self: *@This(), item: T): ChannelError!void {
            while (true) {
                lockMutex(&self.mutex);
                if (self.closed) {
                    self.mutex.unlock();
                    return Err(ChannelError.ChannelClosed);
                }
                if (self.count < self.cap) {
                    self.buffer[self.tail] = item;
                    self.tail = (self.tail + 1) % self.cap;
                    self.count += 1;
                    self.mutex.unlock();
                    return;
                }
                self.mutex.unlock();
                std.Thread.yield() catch {};
            }
        }

        pub fn trySend(self: *@This(), item: T): ChannelError!void {
            lockMutex(&self.mutex);
            defer self.mutex.unlock();

            if (self.closed) {
                return Err(ChannelError.ChannelClosed);
            }
            if (self.count == self.cap) {
                return Err(ChannelError.QueueFull);
            }

            self.buffer[self.tail] = item;
            self.tail = (self.tail + 1) % self.cap;
            self.count += 1;
        }

        pub fn recv(self: *@This()): T? {
            while (true) {
                lockMutex(&self.mutex);
                if (self.count > 0) {
                    const item = self.buffer[self.head];
                    self.head = (self.head + 1) % self.cap;
                    self.count -= 1;
                    self.mutex.unlock();
                    return item;
                }
                if (self.closed) {
                    self.mutex.unlock();
                    return null;
                }
                self.mutex.unlock();
                std.Thread.yield() catch {};
            }
        }

        pub fn tryRecv(self: *@This()): T? {
            lockMutex(&self.mutex);
            defer self.mutex.unlock();

            if (self.count == 0) {
                return null;
            }

            const item = self.buffer[self.head];
            self.head = (self.head + 1) % self.cap;
            self.count -= 1;
            return item;
        }

        pub fn close(self: *@This()) {
            lockMutex(&self.mutex);
            defer self.mutex.unlock();
            self.closed = true;
        }

        pub fn len(self: *@This()): usize {
            lockMutex(&self.mutex);
            defer self.mutex.unlock();
            return self.count;
        }

        pub fn isClosed(self: *@This()): bool {
            lockMutex(&self.mutex);
            defer self.mutex.unlock();
            return self.closed;
        }

        pub fn capacity(self: *const @This()): usize {
            return self.cap;
        }
    };
}

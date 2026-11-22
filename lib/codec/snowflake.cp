// Source: cp/lib/snowflake/snowflake.cp (Pure CP Distributed 64-bit Snowflake ID Generator)
const std = @import("std");

pub const DEFAULT_EPOCH_MS: i64 = 1704067200000; // 2024-01-01T00:00:00Z
pub const MAX_DATACENTER_ID: u16 = 31;
pub const MAX_WORKER_ID: u16 = 31;
pub const MAX_SEQUENCE: u16 = 4095;

pub const SnowflakeError = error{
    InvalidNodeId,
    ClockMovedBackwards,
    InvalidIdString,
};

pub const SnowflakeParts = struct {
    timestamp_ms: i64,
    datacenter_id: u5,
    worker_id: u5,
    sequence: u12,
};

const BASE62_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

pub const SnowflakeGenerator = struct {
    epoch_ms: i64,
    datacenter_id: u5,
    worker_id: u5,
    last_ts: i64,
    sequence: u12,
    mutex: std.Thread.Mutex,

    pub fn init(datacenter_id: u16, worker_id: u16, epoch_ms: i64): SnowflakeError!SnowflakeGenerator {
        if (datacenter_id > MAX_DATACENTER_ID or worker_id > MAX_WORKER_ID) {
            return SnowflakeError.InvalidNodeId;
        }
        return SnowflakeGenerator{
            .epoch_ms = epoch_ms,
            .datacenter_id = @intCast(datacenter_id),
            .worker_id = @intCast(worker_id),
            .last_ts = -1,
            .sequence = 0,
            .mutex = .{},
        };
    }

    pub fn nextIdAt(self: *SnowflakeGenerator, now_ms: i64): SnowflakeError!u64 {
        self.mutex.lock();
        defer self.mutex.unlock();

        if (now_ms < self.last_ts) {
            return SnowflakeError.ClockMovedBackwards;
        }
        let ts = now_ms;
        if (ts == self.last_ts) {
            if (self.sequence == MAX_SEQUENCE) {
                // Virtual deterministic advance to next millisecond for explicit-clock callers
                ts = self.last_ts + 1;
                self.sequence = 0;
            } else {
                self.sequence += 1;
            }
        } else {
            self.sequence = 0;
        }
        self.last_ts = ts;

        const rel_ts: u64 = @intCast(ts - self.epoch_ms);
        return (rel_ts << 22) |
            (@as(u64, self.datacenter_id) << 17) |
            (@as(u64, self.worker_id) << 12) |
            @as(u64, self.sequence);
    }

    pub fn nextId(self: *SnowflakeGenerator): SnowflakeError!u64 {
        self.mutex.lock();
        defer self.mutex.unlock();

        let now_ms = std.time.milliTimestamp();
        if (now_ms < self.last_ts) {
            const drift = self.last_ts - now_ms;
            if (drift > 5) {
                return SnowflakeError.ClockMovedBackwards;
            }
            while (now_ms < self.last_ts) {
                std.Thread.yield() catch {};
                now_ms = std.time.milliTimestamp();
            }
        }

        if (now_ms == self.last_ts) {
            if (self.sequence == MAX_SEQUENCE) {
                while (now_ms <= self.last_ts) {
                    std.Thread.yield() catch {};
                    now_ms = std.time.milliTimestamp();
                }
                self.sequence = 0;
            } else {
                self.sequence += 1;
            }
        } else {
            self.sequence = 0;
        }
        self.last_ts = now_ms;

        const rel_ts: u64 = @intCast(now_ms - self.epoch_ms);
        return (rel_ts << 22) |
            (@as(u64, self.datacenter_id) << 17) |
            (@as(u64, self.worker_id) << 12) |
            @as(u64, self.sequence);
    }
};

pub fn decomposeId(id: u64, epoch_ms: i64): SnowflakeParts {
    const rel_ts: i64 = @intCast(id >> 22);
    return SnowflakeParts{
        .timestamp_ms = epoch_ms + rel_ts,
        .datacenter_id = @truncate((id >> 17) & 0x1F),
        .worker_id = @truncate((id >> 12) & 0x1F),
        .sequence = @truncate(id & 0xFFF),
    };
}

pub fn encodeBase62(id: u64, buf: *[11]u8): []const u8 {
    if (id == 0) {
        buf[10] = '0';
        return buf[10..11];
    }
    let val = id;
    let pos: usize = 11;
    while (val > 0) {
        pos -= 1;
        const rem: usize = @intCast(val % 62);
        buf[pos] = BASE62_CHARS[rem];
        val /= 62;
    }
    return buf[pos..11];
}

pub fn decodeBase62(s: []const u8): SnowflakeError!u64 {
    if (s.len == 0 or s.len > 11) { return SnowflakeError.InvalidIdString; }
    let acc: u64 = 0;
    for (c of s) {
        let digit: u64 = 0;
        if (c >= '0' and c <= '9') {
            digit = c - '0';
        } else if (c >= 'A' and c <= 'Z') {
            digit = @as(u64, 10) + @as(u64, c - 'A');
        } else if (c >= 'a' and c <= 'z') {
            digit = @as(u64, 36) + @as(u64, c - 'a');
        } else {
            return SnowflakeError.InvalidIdString;
        }
        acc = std.math.mul(u64, acc, 62) catch return SnowflakeError.InvalidIdString;
        acc = std.math.add(u64, acc, digit) catch return SnowflakeError.InvalidIdString;
    }
    return acc;
}

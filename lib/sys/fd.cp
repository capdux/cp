// Source: bun/src/sys/fd.cp
// ============================================================================
// lib/sys/fd.cp — Cross-platform File Descriptor Abstraction & Flags
// ============================================================================

const std = @import("std");

pub const O_RDONLY: i32 = 0;
pub const O_WRONLY: i32 = 1;
pub const O_RDWR: i32 = 2;
pub const O_CREAT: i32 = 64;
pub const O_EXCL: i32 = 128;
pub const O_TRUNC: i32 = 512;
pub const O_APPEND: i32 = 1024;
pub const O_NONBLOCK: i32 = 2048;
pub const O_CLOEXEC: i32 = 524288;

pub const FD = struct {
    raw: i32 = -1,

    pub const invalid: FD = FD{ raw: -1 };
    pub const stdin: FD = FD{ raw: 0 };
    pub const stdout: FD = FD{ raw: 1 };
    pub const stderr: FD = FD{ raw: 2 };

    pub fn fromNative(raw_fd: i32): FD {
        return FD{ raw: raw_fd };
    }

    pub fn isValid(self: FD): bool {
        return self.raw >= 0;
    }

    pub fn isStandardStream(self: FD): bool {
        return self.raw >= 0 and self.raw <= 2;
    }

    pub fn read(self: FD, buf: []u8): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.read(self.raw, buf);
    }

    pub fn write(self: FD, data: []const u8): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.write(self.raw, data);
    }

    pub fn writeAll(self: FD, data: []const u8): !void {
        if (!self.isValid()) return error.BadFileDescriptor;
        let remaining = data;
        while (remaining.len > 0) {
            let written = try std.posix.write(self.raw, remaining);
            if (written == 0) return error.WriteZero;
            remaining = remaining[written..];
        }
    }

    pub fn pread(self: FD, buf: []u8, offset: u64): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.pread(self.raw, buf, offset);
    }

    pub fn pwrite(self: FD, data: []const u8, offset: u64): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.pwrite(self.raw, data, offset);
    }

    pub fn seekTo(self: FD, offset: i64): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.lseek_SET(self.raw, offset);
    }

    pub fn seekFromEnd(self: FD, offset: i64): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        return try std.posix.lseek_END(self.raw, offset);
    }

    pub fn close(self: FD): void {
        if (self.isValid() and !self.isStandardStream()) {
            std.posix.close(self.raw);
        }
    }

    pub fn sync(self: FD): !void {
        if (!self.isValid()) return error.BadFileDescriptor;
        try std.posix.fsync(self.raw);
    }
};

// Source: /data/dev/bun/src/sys/fd.cp
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

pub const F_OK: i32 = 0;
pub const X_OK: i32 = 1;
pub const W_OK: i32 = 2;
pub const R_OK: i32 = 4;

pub const SEEK_SET: i32 = 0;
pub const SEEK_CUR: i32 = 1;
pub const SEEK_END: i32 = 2;

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
        if (buf.len == 0) return 0;
        let rc = std.c.read(self.raw, buf.ptr, buf.len);
        if (rc < 0) return error.ReadFailed;
        return @intCast(rc);
    }

    pub fn write(self: FD, data: []const u8): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        if (data.len == 0) return 0;
        let rc = std.c.write(self.raw, data.ptr, data.len);
        if (rc < 0) return error.WriteFailed;
        return @intCast(rc);
    }

    pub fn writeAll(self: FD, data: []const u8): !void {
        if (!self.isValid()) return error.BadFileDescriptor;
        let remaining = data;
        while (remaining.len > 0) {
            let rc = std.c.write(self.raw, remaining.ptr, remaining.len);
            if (rc < 0) return error.WriteFailed;
            if (rc == 0) return error.WriteZero;
            let written: usize = @intCast(rc);
            remaining = remaining[written..];
        }
    }

    pub fn pread(self: FD, buf: []u8, offset: u64): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        if (buf.len == 0) return 0;
        let rc = std.c.pread(self.raw, buf.ptr, buf.len, @intCast(offset));
        if (rc < 0) return error.ReadFailed;
        return @intCast(rc);
    }

    pub fn pwrite(self: FD, data: []const u8, offset: u64): !usize {
        if (!self.isValid()) return error.BadFileDescriptor;
        if (data.len == 0) return 0;
        let rc = std.c.pwrite(self.raw, data.ptr, data.len, @intCast(offset));
        if (rc < 0) return error.WriteFailed;
        return @intCast(rc);
    }

    pub fn lseek(self: FD, offset: i64, whence: i32): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        let rc = std.c.lseek(self.raw, @intCast(offset), @intCast(whence));
        if (rc < 0) return error.SeekFailed;
        return @intCast(rc);
    }

    pub fn seekTo(self: FD, offset: i64): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        let rc = std.c.lseek(self.raw, @intCast(offset), std.c.SEEK.SET);
        if (rc < 0) return error.SeekFailed;
        return @intCast(rc);
    }

    pub fn seekBy(self: FD, offset: i64): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        let rc = std.c.lseek(self.raw, @intCast(offset), std.c.SEEK.CUR);
        if (rc < 0) return error.SeekFailed;
        return @intCast(rc);
    }

    pub fn seekFromEnd(self: FD, offset: i64): !u64 {
        if (!self.isValid()) return error.BadFileDescriptor;
        let rc = std.c.lseek(self.raw, @intCast(offset), std.c.SEEK.END);
        if (rc < 0) return error.SeekFailed;
        return @intCast(rc);
    }

    pub fn ftruncate(self: FD, length: u64): !void {
        if (!self.isValid()) return error.BadFileDescriptor;
        let rc = std.c.ftruncate(self.raw, @intCast(length));
        if (rc != 0) return error.TruncateFailed;
    }

    pub fn dup(self: FD): !FD {
        if (!self.isValid()) return error.BadFileDescriptor;
        let new_raw = std.c.dup(self.raw);
        if (new_raw < 0) return error.DupFailed;
        return FD.fromNative(new_raw);
    }

    pub fn dup2(self: FD, new_fd: FD): !FD {
        if (!self.isValid() or new_fd.raw < 0) return error.BadFileDescriptor;
        let rc = std.c.dup2(self.raw, new_fd.raw);
        if (rc < 0) return error.DupFailed;
        return FD.fromNative(rc);
    }

    pub fn close(self: FD): void {
        if (self.isValid() and !self.isStandardStream()) {
            * = std.c.close(self.raw);
        }
    }

    pub fn sync(self: FD): !void {
        if (!self.isValid()) return error.BadFileDescriptor;
        if (std.c.fsync(self.raw) != 0) return error.SyncFailed;
    }
};

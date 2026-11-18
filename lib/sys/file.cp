// Source: bun/src/sys/File.cp
// ============================================================================
// lib/sys/file.cp — RAII File Handle Wrapper & High-level File Operations
// ============================================================================

const std = @import("std");
import { FD, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_EXCL, O_TRUNC, O_APPEND, O_NONBLOCK, O_CLOEXEC } from "./fd";
import { Stat, fstat } from "./stat";

pub const File = struct {
    fd: FD = FD.invalid,

    pub fn open(path: []const u8, flags: i32, mode: u32): !File {
        let path_buf: [1024]u8 = undefined;
        if (path.len >= path_buf.len) return error.NameTooLong;
        @memcpy(path_buf[0 .. path.len], path);
        path_buf[path.len] = 0;
        let path_z: [:0]const u8 = path_buf[0 .. path.len :0];

        let acc: std.posix.ACCMODE = .RDONLY;
        if ((flags & O_RDWR) != 0) {
            acc = .RDWR;
        } else if ((flags & O_WRONLY) != 0) {
            acc = .WRONLY;
        }

        let z_flags = std.posix.O{
            .ACCMODE = acc,
            .CREAT = (flags & O_CREAT) != 0,
            .EXCL = (flags & O_EXCL) != 0,
            .TRUNC = (flags & O_TRUNC) != 0,
            .APPEND = (flags & O_APPEND) != 0,
            .NONBLOCK = (flags & O_NONBLOCK) != 0,
            .CLOEXEC = (flags & O_CLOEXEC) != 0,
        };

        let raw = try std.posix.open(path_z, z_flags, std.posix.mode_t(@intCast(mode)));
        return File{ fd: FD.fromNative(raw) };
    }

    pub fn create(path: []const u8): !File {
        return open(path, O_CREAT | O_TRUNC | O_WRONLY | O_CLOEXEC, 0o644);
    }

    pub fn openReadOnly(path: []const u8): !File {
        return open(path, O_RDONLY | O_CLOEXEC, 0);
    }

    pub fn close(*_): void {
        _.fd.close();
        _.fd = FD.invalid;
    }

    pub fn read(*_, buf: []u8): !usize {
        return _.fd.read(buf);
    }

    pub fn write(*_, data: []const u8): !usize {
        return _.fd.write(data);
    }

    pub fn writeAll(*_, data: []const u8): !void {
        return _.fd.writeAll(data);
    }

    pub fn pread(*_, buf: []u8, offset: u64): !usize {
        return _.fd.pread(buf, offset);
    }

    pub fn pwrite(*_, data: []const u8, offset: u64): !usize {
        return _.fd.pwrite(data, offset);
    }

    pub fn seekTo(*_, offset: i64): !u64 {
        return _.fd.seekTo(offset);
    }

    pub fn seekFromEnd(*_, offset: i64): !u64 {
        return _.fd.seekFromEnd(offset);
    }

    pub fn stat(*_): !Stat {
        return fstat(_.fd);
    }

    pub fn size(*_): !u64 {
        let st = try fstat(_.fd);
        return st.size;
    }

    pub fn readToEndAlloc(*_, allocator: std.mem.Allocator): ![]u8 {
        let s = try _.size();
        let buf = try allocator.alloc(u8, s);
        let bytes_read = try _.pread(buf, 0);
        return buf[0 .. bytes_read];
    }
};

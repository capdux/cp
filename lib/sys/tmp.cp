// Source: bun/src/sys/tmp.cp
// ============================================================================
// lib/sys/tmp.cp — Cross-platform Temporary Files & Directory Utilities
// ============================================================================

const std = @import("std");
import { FD, O_RDWR, O_CREAT, O_EXCL, O_CLOEXEC } from "./fd";
import { File } from "./file";

let tmp_sequence: u64 = 1000;

pub fn getTempDir(): []const u8 {
    if (val := std.c.getenv("TMPDIR")) {
        return std.mem.span(val);
    }
    if (val := std.c.getenv("TMP")) {
        return std.mem.span(val);
    }
    if (val := std.c.getenv("TEMP")) {
        return std.mem.span(val);
    }
    return "/tmp";
}

pub fn removeFile(path: []const u8): !void {
    let buf: [1024]u8 = undefined;
    if (path.len >= buf.len) return error.NameTooLong;
    @memcpy(buf[0 .. path.len], path);
    buf[path.len] = 0;
    let path_z: [*:0]const u8 = buf[0 .. path.len :0].ptr;

    let rc = std.c.unlink(path_z);
    if (rc != 0) return error.UnlinkFailed;
}

pub fn removeDir(path: []const u8): !void {
    let buf: [1024]u8 = undefined;
    if (path.len >= buf.len) return error.NameTooLong;
    @memcpy(buf[0 .. path.len], path);
    buf[path.len] = 0;
    let path_z: [*:0]const u8 = buf[0 .. path.len :0].ptr;

    let rc = std.c.rmdir(path_z);
    if (rc != 0) return error.RmdirFailed;
}

pub fn makeTempFile(prefix: []const u8, suffix: []const u8, path_buf: []u8): !File {
    let tmp_dir = getTempDir();
    let max_retries: usize = 100;
    let attempt: usize = 0;

    while (attempt < max_retries) {
        attempt += 1;
        tmp_sequence += 1;
        let ts: i64 = std.time.milliTimestamp();

        let formatted = std.fmt.bufPrint(
            path_buf,
            "{s}/{s}_{d}_{d}{s}",
            .{ tmp_dir, prefix, ts, tmp_sequence },
        ) catch return error.BufferTooSmall;

        if (formatted.len + 1 > path_buf.len) return error.BufferTooSmall;
        path_buf[formatted.len] = 0;

        let f = File.open(formatted, O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC, 0o600) catch e => {
            if (e == error.PathAlreadyExists) continue;
            return e;
        };
        return f;
    }

    return error.TooManyRetries;
}

pub fn makeTempDir(prefix: []const u8, path_buf: []u8): ![]const u8 {
    let tmp_dir = getTempDir();
    let max_retries: usize = 100;
    let attempt: usize = 0;

    while (attempt < max_retries) {
        attempt += 1;
        tmp_sequence += 1;
        let ts: i64 = std.time.milliTimestamp();

        let formatted = std.fmt.bufPrint(
            path_buf,
            "{s}/{s}_{d}_{d}",
            .{ tmp_dir, prefix, ts, tmp_sequence },
        ) catch return error.BufferTooSmall;

        if (formatted.len + 1 > path_buf.len) return error.BufferTooSmall;
        path_buf[formatted.len] = 0;
        let path_z: [*:0]const u8 = path_buf[0 .. formatted.len :0].ptr;

        let rc = std.c.mkdir(path_z, 0o700);
        if (rc == 0) {
            return formatted;
        }
    }

    return error.TooManyRetries;
}

// cap/test/test_sys_complete.cp
// ============================================================================
// Comprehensive Test Suite for cap:sys Standard Library
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    FD,
    File,
    Stat,
    stat,
    fstat,
    copyFile,
    copyFileByFd,
    Signal,
    fromNumber,
    fromName,
    getTempDir,
    makeTempFile,
    makeTempDir,
    removeFile,
    removeDir,
} from "cap:sys";

const std = @import("std");

fn testFileLifecycle(): !void {
    let path_buf: [1024]u8 = undefined;
    let file = try makeTempFile("test_life", ".txt", &path_buf);
    let path_len = std.mem.indexOfScalar(u8, &path_buf, 0) orelse path_buf.len;
    let path = path_buf[0..path_len];

    let msg = "Hello CP Sys Subsystem!";
    try file.writeAll(msg);

    let sz = try file.size();
    expect_eq(sz, msg.len);

    let read_buf: [128]u8 = undefined;
    let bytes_read = try file.pread(&read_buf, 0);
    expect_eq(bytes_read, msg.len);
    expect_eq(std.mem.eql(u8, read_buf[0..bytes_read], msg), true);

    file.close();
    try removeFile(path);
}

fn testPositionalIOAndSeek(): !void {
    let path_buf: [1024]u8 = undefined;
    let file = try makeTempFile("test_seek", ".bin", &path_buf);
    let path_len = std.mem.indexOfScalar(u8, &path_buf, 0) orelse path_buf.len;
    let path = path_buf[0..path_len];

    let initial = "0123456789ABCDEF";
    try file.writeAll(initial);

    // pwrite at offset 4
    let patch = "WXYZ";
    let written = try file.pwrite(patch, 4);
    expect_eq(written, 4);

    // pread at offset 4
    let read_buf: [16]u8 = undefined;
    let r1 = try file.pread(read_buf[0..4], 4);
    expect_eq(r1, 4);
    expect_eq(std.mem.eql(u8, read_buf[0..4], "WXYZ"), true);

    // seekTo offset 10 and read
    let pos = try file.seekTo(10);
    expect_eq(pos, 10);
    let r2 = try file.read(read_buf[0..2]);
    expect_eq(r2, 2);
    expect_eq(std.mem.eql(u8, read_buf[0..2], "9A"), true);

    // seekFromEnd
    let end_pos = try file.seekFromEnd(0);
    expect_eq(end_pos, 16);

    file.close();
    try removeFile(path);
}

fn testStatMetadata(): !void {
    let path_buf: [1024]u8 = undefined;
    let file = try makeTempFile("test_stat", ".dat", &path_buf);
    let path_len = std.mem.indexOfScalar(u8, &path_buf, 0) orelse path_buf.len;
    let path = path_buf[0..path_len];

    let content = "Forty-two bytes of payload test data string!";
    try file.writeAll(content);

    // Test fstat
    let st_fd = try file.stat();
    expect_eq(st_fd.size, content.len);
    expect_eq(st_fd.isFile(), true);
    expect_eq(st_fd.isDir(), false);

    file.close();

    // Test stat by path
    let st_path = try stat(path);
    expect_eq(st_path.size, content.len);
    expect_eq(st_path.isFile(), true);
    expect_eq(st_path.isDir(), false);

    // Test directory stat
    let tmp_dir = getTempDir();
    let st_dir = try stat(tmp_dir);
    expect_eq(st_dir.isDir(), true);
    expect_eq(st_dir.isFile(), false);

    try removeFile(path);
}

fn testCopyFile(): !void {
    let src_buf: [1024]u8 = undefined;
    let dst_buf: [1024]u8 = undefined;

    let src_file = try makeTempFile("test_copy_src", ".dat", &src_buf);
    let src_len = std.mem.indexOfScalar(u8, &src_buf, 0) orelse src_buf.len;
    let src_path = src_buf[0..src_len];

    let payload = "High-Performance Zero-Copy Syscall File Engine for Agent Runtime";
    try src_file.writeAll(payload);
    src_file.close();

    // Destination path
    let tmp_dir = getTempDir();
    let dst_formatted = try std.fmt.bufPrint(&dst_buf, "{s}/test_copy_dst_{d}.dat", .{ tmp_dir, std.time.milliTimestamp() });

    try copyFile(src_path, dst_formatted);

    // Verify destination stat and contents
    let dst_st = try stat(dst_formatted);
    expect_eq(dst_st.size, payload.len);

    let dst_file = try File.openReadOnly(dst_formatted);
    let read_buf: [128]u8 = undefined;
    let bytes_read = try dst_file.read(&read_buf);
    expect_eq(bytes_read, payload.len);
    expect_eq(std.mem.eql(u8, read_buf[0..bytes_read], payload), true);
    dst_file.close();

    try removeFile(src_path);
    try removeFile(dst_formatted);
}

fn testTempDirLifecycle(): !void {
    let dir_buf: [1024]u8 = undefined;
    let created_dir = try makeTempDir("test_sys_dir", &dir_buf);

    let st = try stat(created_dir);
    expect_eq(st.isDir(), true);

    // Create a file inside created directory
    let file_buf: [1024]u8 = undefined;
    let file_path = try std.fmt.bufPrint(&file_buf, "{s}/subfile.txt", .{created_dir});
    let file = try File.create(file_path);
    try file.writeAll("hello inside dir");
    file.close();

    // Remove file and dir
    try removeFile(file_path);
    try removeDir(created_dir);
}

fn testSignalCodesAndLookup(): !void {
    // 1. Enum names and descriptions
    expect_eq(std.mem.eql(u8, Signal.SIGTERM.name(), "SIGTERM"), true);
    expect_eq(std.mem.eql(u8, Signal.SIGKILL.name(), "SIGKILL"), true);
    expect_eq(std.mem.eql(u8, Signal.SIGINT.name(), "SIGINT"), true);

    expect_eq(Signal.SIGINT.toExitCode(), 130);
    expect_eq(Signal.SIGKILL.toExitCode(), 137);

    expect_eq(std.mem.eql(u8, Signal.SIGSEGV.description(), "Address boundary error"), true);

    // 2. Lookup by number
    if (s1 := fromNumber(15)) {
        expect_eq(s1 == Signal.SIGTERM, true);
    } else {
        expect_eq(false, true);
    }

    if (s2 := fromNumber(9)) {
        expect_eq(s2 == Signal.SIGKILL, true);
    } else {
        expect_eq(false, true);
    }

    let inv_num = fromNumber(99);
    expect_eq(inv_num == null, true);

    // 3. Lookup by name
    if (s3 := fromName("SIGQUIT")) {
        expect_eq(s3 == Signal.SIGQUIT, true);
    } else {
        expect_eq(false, true);
    }

    if (s4 := fromName("HUP")) {
        expect_eq(s4 == Signal.SIGHUP, true);
    } else {
        expect_eq(false, true);
    }

    let inv_name = fromName("NONEXISTENT_SIGNAL");
    expect_eq(inv_name == null, true);
}

fn main() {
    test("1. File Lifecycle & RAII: create, writeAll, pread, size, close, removeFile", () => {
        try testFileLifecycle();
    });
    test("2. File Positional I/O & Seek: pwrite, pread, seekTo, seekFromEnd", () => {
        try testPositionalIOAndSeek();
    });
    test("3. Posix Stat & File Metadata: fstat, stat, isFile, isDir", () => {
        try testStatMetadata();
    });
    test("4. High-Performance File Copy: copyFile, zero-copy kernel & fallback", () => {
        try testCopyFile();
    });
    test("5. Temporary Directory Lifecycle: makeTempDir, stat isDir, removeDir", () => {
        try testTempDirLifecycle();
    });
    test("6. POSIX Signal Codes & Name Resolution: Signal, toExitCode, fromNumber, fromName", () => {
        try testSignalCodesAndLookup();
    });
}

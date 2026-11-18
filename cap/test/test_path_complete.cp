// cap/test/test_path_complete.cp
// ============================================================================
// Comprehensive Test Suite for cap:path Standard Library
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    Path,
    PosixPath,
    WindowsPath,
    ParsedPath,
    EnvPath,
    EnvPathIterator,
    PathBufferPool,
    PathBuffer,
    MAX_PATH_BYTES,
    longestCommonPathPosix,
    longestCommonPathWindows,
} from "cap:path";

const std = @import("std");

fn testPosixAlgorithms(): !void {
    let buf: [1024]u8 = undefined;

    // 1. normalize
    let n1 = PosixPath.normalize("/foo/bar//baz/asdf/quux/..", &buf);
    expect_eq(std.mem.eql(u8, n1, "/foo/bar/baz/asdf"), true);

    let n2 = PosixPath.normalize("a/b/../c/./d", &buf);
    expect_eq(std.mem.eql(u8, n2, "a/c/d"), true);

    let n3 = PosixPath.normalize("/..", &buf);
    expect_eq(std.mem.eql(u8, n3, "/"), true);

    let n4 = PosixPath.normalize("../../a", &buf);
    expect_eq(std.mem.eql(u8, n4, "../../a"), true);

    let n5 = PosixPath.normalize("", &buf);
    expect_eq(std.mem.eql(u8, n5, "."), true);

    // 2. join
    let join_parts: [5][]const u8 = [ "/foo", "bar", "baz/asdf", "quux", ".." ];
    let j1 = PosixPath.join(join_parts[0..5], &buf);
    expect_eq(std.mem.eql(u8, j1, "/foo/bar/baz/asdf"), true);

    let join_parts2: [3][]const u8 = [ "a", "b", "c" ];
    let j2 = PosixPath.join(join_parts2[0..3], &buf);
    expect_eq(std.mem.eql(u8, j2, "a/b/c"), true);

    // 3. resolve
    let res_parts1: [2][]const u8 = [ "/foo/bar", "/baz" ];
    let r1 = PosixPath.resolve(res_parts1[0..2], &buf);
    expect_eq(std.mem.eql(u8, r1, "/baz"), true);

    let res_parts2: [3][]const u8 = [ "/a/b", "c", "../d" ];
    let r2 = PosixPath.resolve(res_parts2[0..3], &buf);
    expect_eq(std.mem.eql(u8, r2, "/a/b/d"), true);

    let res_parts3: [2][]const u8 = [ "a", "b" ];
    let r3 = PosixPath.resolve(res_parts3[0..2], &buf);
    expect_eq(std.mem.eql(u8, r3, "/a/b"), true);

    // 4. relative
    let rel1 = PosixPath.relative("/data/dev/cp", "/data/dev/bun/src", &buf);
    expect_eq(std.mem.eql(u8, rel1, "../bun/src"), true);

    let rel2 = PosixPath.relative("/a/b/c", "/a", &buf);
    expect_eq(std.mem.eql(u8, rel2, "../.."), true);

    let rel3 = PosixPath.relative("/a/b", "/a/b/c/d", &buf);
    expect_eq(std.mem.eql(u8, rel3, "c/d"), true);

    let rel4 = PosixPath.relative("/same/path", "/same/path", &buf);
    expect_eq(std.mem.eql(u8, rel4, ""), true);

    // 5. longestCommonPath
    let lcp_list1: [3][]const u8 = [ "/usr/local/bin", "/usr/local/share", "/usr/local/etc" ];
    let l1 = PosixPath.longestCommonPath(lcp_list1[0..3]);
    expect_eq(std.mem.eql(u8, l1, "/usr/local"), true);

    let lcp_list2: [2][]const u8 = [ "/usr/bin", "/var/log" ];
    let l2 = PosixPath.longestCommonPath(lcp_list2[0..2]);
    expect_eq(std.mem.eql(u8, l2, "/"), true);
}

fn testWindowsAlgorithms(): !void {
    let buf: [1024]u8 = undefined;

    // 1. isAbsolute & volume
    expect_eq(WindowsPath.isAbsolute("C:\\Users\\Admin"), true);
    expect_eq(WindowsPath.isAbsolute("\\\\server\\share\\path"), true);
    expect_eq(WindowsPath.isAbsolute("foo\\bar"), false);
    expect_eq(std.mem.eql(u8, WindowsPath.volumeName("C:\\dir\\file"), "C:"), true);

    // 2. normalize
    let n1 = WindowsPath.normalize("C:\\foo\\bar\\..\\baz", &buf);
    expect_eq(std.mem.eql(u8, n1, "C:\\foo\\baz"), true);

    let n2 = WindowsPath.normalize("\\\\server\\share\\a\\..\\b", &buf);
    expect_eq(std.mem.eql(u8, n2, "\\\\server\\share\\b"), true);

    let n3 = WindowsPath.normalize("foo\\bar\\.\\baz", &buf);
    expect_eq(std.mem.eql(u8, n3, "foo\\bar\\baz"), true);

    // 3. join
    let parts1: [3][]const u8 = [ "C:\\dir", "subdir", "file.txt" ];
    let j1 = WindowsPath.join(parts1[0..3], &buf);
    expect_eq(std.mem.eql(u8, j1, "C:\\dir\\subdir\\file.txt"), true);

    // 4. resolve
    let res_parts1: [2][]const u8 = [ "C:\\dir", "D:\\other\\file.txt" ];
    let r1 = WindowsPath.resolve(res_parts1[0..2], &buf);
    expect_eq(std.mem.eql(u8, r1, "D:\\other\\file.txt"), true);

    let res_parts2: [2][]const u8 = [ "C:\\dir", "subdir" ];
    let r2 = WindowsPath.resolve(res_parts2[0..2], &buf);
    expect_eq(std.mem.eql(u8, r2, "C:\\dir\\subdir"), true);

    // 5. relative
    let rel1 = WindowsPath.relative("C:\\a\\b\\c", "C:\\a\\d", &buf);
    expect_eq(std.mem.eql(u8, rel1, "..\\..\\d"), true);

    let rel2 = WindowsPath.relative("C:\\foo", "D:\\bar", &buf);
    expect_eq(std.mem.eql(u8, rel2, "D:\\bar"), true);

    // 6. longestCommonPath
    let lcp_win: [2][]const u8 = [ "C:\\Users\\Alice\\Documents", "C:\\Users\\Alice\\Downloads" ];
    let l1 = WindowsPath.longestCommonPath(lcp_win[0..2]);
    expect_eq(std.mem.eql(u8, l1, "C:\\Users\\Alice"), true);
}

fn testPathParseFormat(): !void {
    let p = Path.parse("/home/user/document.pdf");
    expect_eq(std.mem.eql(u8, p.root, "/"), true);
    expect_eq(std.mem.eql(u8, p.dir, "/home/user"), true);
    expect_eq(std.mem.eql(u8, p.base, "document.pdf"), true);
    expect_eq(std.mem.eql(u8, p.ext, ".pdf"), true);
    expect_eq(std.mem.eql(u8, p.name, "document"), true);

    let buf: [1024]u8 = undefined;
    let formatted = Path.format(p, &buf);
    expect_eq(std.mem.eql(u8, formatted, "/home/user/document.pdf"), true);
}

fn testEnvPath(): !void {
    let env = EnvPath.init(std.heap.page_allocator);
    defer env.deinit();

    try env.append("/usr/bin", ':');
    try env.append("/usr/local/bin", ':');
    try env.append("/bin", ':');

    expect_eq(std.mem.eql(u8, env.slice(), "/usr/bin:/usr/local/bin:/bin"), true);
    expect_eq(env.count(), 3);

    let iter = EnvPath.split("C:\\Windows;C:\\Windows\\System32;C:\\Program Files", ';');
    let item1 = iter.next();
    expect_eq(std.mem.eql(u8, item1.?, "C:\\Windows"), true);

    let item2 = iter.next();
    expect_eq(std.mem.eql(u8, item2.?, "C:\\Windows\\System32"), true);

    let item3 = iter.next();
    expect_eq(std.mem.eql(u8, item3.?, "C:\\Program Files"), true);

    let item4 = iter.next();
    expect_eq(item4 == null, true);
}

fn testPathBufferPool(): !void {
    expect_eq(MAX_PATH_BYTES, 4096);

    let b1 = PathBufferPool.get();
    let b2 = PathBufferPool.get();

    // Two distinct buffers obtained from pool (distinct addresses)
    expect_eq(@intFromPtr(b1) != @intFromPtr(b2), true);

    b1[0] = 'a';
    b2[0] = 'b';
    expect_eq(b1[0], 'a');
    expect_eq(b2[0], 'b');

    // Return b1 to pool
    PathBufferPool.put(b1);

    // Next get returns recycled b1
    let b3 = PathBufferPool.get();
    expect_eq(@intFromPtr(b1) == @intFromPtr(b3), true);

    PathBufferPool.put(b2);
    PathBufferPool.put(b3);
}

fn main() {
    test("1. POSIX path algorithms: normalize, join, resolve, relative, longestCommonPath", () => {
        try testPosixAlgorithms();
    });
    test("2. Windows path algorithms: drive letter, UNC, normalize, join, resolve, relative", () => {
        try testWindowsAlgorithms();
    });
    test("3. Path.parse and Path.format", () => {
        try testPathParseFormat();
    });
    test("4. EnvPath: PATH environment variable builder & delimiter-aware iterator", () => {
        try testEnvPath();
    });
    test("5. PathBufferPool: zero-allocation preallocated path buffers", () => {
        try testPathBufferPool();
    });
}

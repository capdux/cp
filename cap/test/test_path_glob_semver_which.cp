// Source: /data/dev/bun/src/paths/ & /data/dev/bun/src/glob/ & /data/dev/bun/src/semver/ & /data/dev/bun/src/which/
// ============================================================================
// cap/test/test_path_glob_semver_which.cp
// Unified Industrial-grade Audit Test Suite for cap:path, cap:glob, cap:semver, cap:which
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Path,
    PosixPath,
    WindowsPath,
    ParsedPath,
    EnvPath,
    EnvPathIterator,
    PathBufferPool,
    MAX_PATH_BYTES,
} from "cap:path";
import {
    Glob,
    Matcher,
    GlobMatcher,
    GlobWalker,
    GlobWalkerOptions,
} from "cap:glob";
import {
    Semver,
    Version,
    ReleaseType,
    Op,
    Comparator,
    Query,
    SemverRange,
} from "cap:semver";
import {
    Which,
    WhichOptions,
    which,
    isExecutable,
    endsWithExtension,
} from "cap:which";

fn testCapPathPosixAndWindows(): !void {
    let buf: [1024]u8 = undefined;

    // 1. POSIX: isAbsolute, dirname, basename, extname
    expect_eq(PosixPath.isAbsolute("/usr/local/bin"), true);
    expect_eq(PosixPath.isAbsolute("usr/local/bin"), false);
    expect_eq(std.mem.eql(u8, PosixPath.dirname("/usr/local/bin/cpc"), "/usr/local/bin"), true);
    expect_eq(std.mem.eql(u8, PosixPath.dirname("/"), "/"), true);
    expect_eq(std.mem.eql(u8, PosixPath.dirname("README.md"), "."), true);
    expect_eq(std.mem.eql(u8, PosixPath.basename("/usr/local/bin/cpc.tar.gz", null), "cpc.tar.gz"), true);
    expect_eq(std.mem.eql(u8, PosixPath.basename("/usr/local/bin/cpc.tar.gz", ".gz"), "cpc.tar"), true);
    expect_eq(std.mem.eql(u8, PosixPath.extname("index.cp"), ".cp"), true);
    expect_eq(std.mem.eql(u8, PosixPath.extname(".bashrc"), ""), true);
    expect_eq(std.mem.eql(u8, PosixPath.extname(".."), ""), true);
    expect_eq(std.mem.eql(u8, PosixPath.extname("."), ""), true);

    // 2. POSIX: normalize, join, resolve, relative, longestCommonPath
    let norm_p = PosixPath.normalize("/data/dev//cp/../bun/./src", &buf);
    expect_eq(std.mem.eql(u8, norm_p, "/data/dev/bun/src"), true);

    let join_parts: [4][]const u8 = [ "/data/dev", "cp", "lib/path", "../glob" ];
    let joined_p = PosixPath.join(join_parts[0..4], &buf);
    expect_eq(std.mem.eql(u8, joined_p, "/data/dev/cp/lib/glob"), true);

    let res_parts: [3][]const u8 = [ "/var/log", "/data/dev/cp", "lib/semver" ];
    let resolved_p = PosixPath.resolve(res_parts[0..3], &buf);
    expect_eq(std.mem.eql(u8, resolved_p, "/data/dev/cp/lib/semver"), true);

    let rel_p = PosixPath.relative("/data/dev/cp/lib/path", "/data/dev/cp/docs/path", &buf);
    expect_eq(std.mem.eql(u8, rel_p, "../../docs/path"), true);

    let lcp_paths: [3][]const u8 = [ "/data/dev/cp/lib/path", "/data/dev/cp/lib/glob", "/data/dev/cp/lib/semver" ];
    expect_eq(std.mem.eql(u8, PosixPath.longestCommonPath(lcp_paths[0..3]), "/data/dev/cp/lib"), true);

    // 3. Windows: isAbsolute, volumeName, dirname, basename, extname
    expect_eq(WindowsPath.isAbsolute("C:\\Program Files\\Bun"), true);
    expect_eq(WindowsPath.isAbsolute("\\\\server\\share\\dir"), true);
    expect_eq(WindowsPath.isAbsolute("relative\\dir"), false);
    expect_eq(std.mem.eql(u8, WindowsPath.volumeName("C:\\Windows\\System32"), "C:"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.dirname("C:\\Windows\\System32\\cmd.exe"), "C:\\Windows\\System32"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.basename("C:\\Windows\\System32\\CMD.EXE", ".exe"), "CMD"), true);
    expect_eq(std.mem.eql(u8, WindowsPath.extname("C:\\dir\\.."), ""), true);
    expect_eq(std.mem.eql(u8, WindowsPath.extname("C:\\dir\\script.bat"), ".bat"), true);

    // 4. Windows: normalize, join, resolve, relative
    let win_norm = WindowsPath.normalize("C:\\Users\\Admin\\..\\Public\\.\\Music", &buf);
    expect_eq(std.mem.eql(u8, win_norm, "C:\\Users\\Public\\Music"), true);

    let win_join_parts: [3][]const u8 = [ "C:\\dev", "cp", "lib\\which" ];
    let win_joined = WindowsPath.join(win_join_parts[0..3], &buf);
    expect_eq(std.mem.eql(u8, win_joined, "C:\\dev\\cp\\lib\\which"), true);

    let win_rel = WindowsPath.relative("C:\\dev\\cp\\lib", "C:\\dev\\bun\\src", &buf);
    expect_eq(std.mem.eql(u8, win_rel, "..\\..\\bun\\src"), true);

    // 5. Path.parse & Path.format, EnvPath & PathBufferPool
    let parsed = Path.parse("/data/dev/cp/lib/path/index.cp");
    expect_eq(std.mem.eql(u8, parsed.root, "/"), true);
    expect_eq(std.mem.eql(u8, parsed.dir, "/data/dev/cp/lib/path"), true);
    expect_eq(std.mem.eql(u8, parsed.base, "index.cp"), true);
    expect_eq(std.mem.eql(u8, parsed.ext, ".cp"), true);
    expect_eq(std.mem.eql(u8, parsed.name, "index"), true);

    let formatted = Path.format(parsed, &buf);
    expect_eq(std.mem.eql(u8, formatted, "/data/dev/cp/lib/path/index.cp"), true);

    let env = EnvPath.init(std.heap.page_allocator);
    defer env.deinit();
    try env.append("/usr/local/bin", ':');
    try env.append("/usr/bin", ':');
    expect_eq(env.count(), 2);
    expect_eq(std.mem.eql(u8, env.slice(), "/usr/local/bin:/usr/bin"), true);

    let pool_b1 = PathBufferPool.get();
    let pool_b2 = PathBufferPool.get();
    expect_eq(unsafe { @intFromPtr(pool_b1) != @intFromPtr(pool_b2) }, true);
    PathBufferPool.put(pool_b1);
    PathBufferPool.put(pool_b2);
}

fn testCapGlobMatcherAndWalker(): !void {
    // 1. hasMagic
    expect_eq(Glob.hasMagic("src/main.cp"), false);
    expect_eq(Glob.hasMagic("src/*.cp"), true);
    expect_eq(Glob.hasMagic("src/file?.cp"), true);
    expect_eq(Glob.hasMagic("src/[a-z].cp"), true);
    expect_eq(Glob.hasMagic("src/{a,b}.cp"), true);
    expect_eq(Glob.hasMagic("!src/main.cp"), true);

    // 2. Wildcards: *, **, ?, [abc], [!0-9], {a,b}
    expect_eq(Glob.match("*.cp", "index.cp"), true);
    expect_eq(Glob.match("*.cp", "lib/index.cp"), false);
    expect_eq(Glob.match("**/*.cp", "index.cp"), true);
    expect_eq(Glob.match("**/*.cp", "lib/path/posix.cp"), true);
    expect_eq(Glob.match("lib/**/index.cp", "lib/index.cp"), true);
    expect_eq(Glob.match("lib/**/index.cp", "lib/a/b/c/index.cp"), true);
    expect_eq(Glob.match("file?.cp", "file1.cp"), true);
    expect_eq(Glob.match("file?.cp", "file12.cp"), false);
    expect_eq(Glob.match("test_[abc].cp", "test_b.cp"), true);
    expect_eq(Glob.match("test_[abc].cp", "test_d.cp"), false);
    expect_eq(Glob.match("test_[a-z0-9].cp", "test_7.cp"), true);
    expect_eq(Glob.match("test_[!0-9].cp", "test_x.cp"), true);
    expect_eq(Glob.match("test_[!0-9].cp", "test_5.cp"), false);
    expect_eq(Glob.match("lib/{path,glob,semver,which}/*.cp", "lib/semver/version.cp"), true);
    expect_eq(Glob.match("lib/{path,glob,semver,which}/*.cp", "lib/crypto/hash.cp"), false);
    expect_eq(Glob.match("!*.zig", "main.cp"), true);
    expect_eq(Glob.match("!*.zig", "main.zig"), false);
    expect_eq(Glob.match("foo**bar.cp", "fooxyzbar.cp"), true);

    // 3. GlobWalker filesystem scan
    const allocator = std.heap.page_allocator;
    const files = try Glob.scan(allocator, "*.cp", "lib/which");
    defer {
        for (f in files) {
            allocator.free(f);
        }
        allocator.free(files);
    }
    expect_eq(files.len, 2);
}

fn testCapSemverEngine(): !void {
    // 1. Version parsing & formatting
    let v1 = try Semver.parse("v1.2.3-alpha.1+build.42");
    expect_eq(v1.major, 1);
    expect_eq(v1.minor, 2);
    expect_eq(v1.patch, 3);
    expect_eq(std.mem.eql(u8, v1.prerelease, "alpha.1"), true);
    expect_eq(std.mem.eql(u8, v1.build, "build.42"), true);

    let fmt_buf: [128]u8 = undefined;
    expect_eq(std.mem.eql(u8, v1.format(&fmt_buf), "1.2.3-alpha.1+build.42"), true);

    // 2. Comparison & Prerelease ordering
    let v_a = try Semver.parse("1.0.0-alpha.1");
    let v_b = try Semver.parse("1.0.0-alpha.2");
    let v_c = try Semver.parse("1.0.0-beta");
    let v_r = try Semver.parse("1.0.0");
    expect_eq(v_a.lt(v_b), true);
    expect_eq(v_b.lt(v_c), true);
    expect_eq(v_c.lt(v_r), true);
    expect_eq(Semver.order(v_r, v_c) == .gt, true);

    // 3. Range evaluation (^, ~, >=, *, ||, hyphen, spaced operators)
    expect_eq(Semver.satisfiesStr("1.5.2", "^1.2.0"), true);
    expect_eq(Semver.satisfiesStr("2.0.0", "^1.2.0"), false);
    expect_eq(Semver.satisfiesStr("0.2.5", "^0.2.1"), true);
    expect_eq(Semver.satisfiesStr("0.3.0", "^0.2.1"), false);
    expect_eq(Semver.satisfiesStr("1.2.9", "~1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "~1.2.3"), false);
    expect_eq(Semver.satisfiesStr("2.1.0", ">=1.5.0 <3.0.0"), true);
    expect_eq(Semver.satisfiesStr("2.1.0", ">= 1.5.0 < 3.0.0"), true);
    expect_eq(Semver.satisfiesStr("3.0.0", ">= 1.5.0 < 3.0.0"), false);
    expect_eq(Semver.satisfiesStr("99.0.0", "*"), true);
    expect_eq(Semver.satisfiesStr("1.4.0", "1.2.0 - 1.6.0"), true);
    expect_eq(Semver.satisfiesStr("3.1.0", "^1.0.0 || ^3.0.0"), true);

    // 4. maxSatisfyingStr & minSatisfyingStr
    let candidates: [4][]const u8 = [ "1.0.0", "1.4.2", "1.9.0", "2.0.0" ];
    let max_s = Semver.maxSatisfyingStr(candidates[0..4], "^1.0.0");
    let min_s = Semver.minSatisfyingStr(candidates[0..4], "^1.0.0");
    expect_eq(std.mem.eql(u8, max_s.?, "1.9.0"), true);
    expect_eq(std.mem.eql(u8, min_s.?, "1.0.0"), true);

    // 5. Increment
    let v_inc = v_r.increment(.minor, null);
    expect_eq(v_inc.major, 1);
    expect_eq(v_inc.minor, 1);
    expect_eq(v_inc.patch, 0);
}

fn testCapWhichLocator(): !void {
    let out_buf: [512]u8 = undefined;

    // 1. Windows extension check helper
    expect_eq(endsWithExtension("bun.exe"), true);
    expect_eq(endsWithExtension("SCRIPT.CMD"), true);
    expect_eq(endsWithExtension("build.bat"), true);
    expect_eq(endsWithExtension("binary.sh"), false);

    // 2. Directory is not treated as an executable binary
    expect_eq(isExecutable("/bin"), false);
    expect_eq(isExecutable("/usr/bin"), false);

    // 3. System binary lookup via PATH
    let sh_res = which("sh", .{}, &out_buf);
    expect_eq(sh_res != null, true);
    if (p := sh_res) {
        expect_eq(std.mem.endsWith(u8, p, "/sh"), true);
        expect_eq(isExecutable(p), true);
    }

    // 4. Absolute path lookup
    let abs_res = which("/bin/sh", .{}, &out_buf);
    expect_eq(abs_res != null, true);
    if (p := abs_res) {
        expect_eq(std.mem.eql(u8, p, "/bin/sh"), true);
    }

    // 5. Relative path with custom cwd
    let rel_res = which("./sh", .{ .cwd = "/bin" }, &out_buf);
    expect_eq(rel_res != null, true);
    if (p := rel_res) {
        expect_eq(std.mem.eql(u8, p, "/bin/sh"), true);
    }

    // 6. Custom PATH lookup and missing binary
    let custom_res = which("sh", .{ .path = "/nonexistent:/bin:/usr/bin" }, &out_buf);
    expect_eq(custom_res != null, true);

    let missing_res = which("nonexistent_binary_999999", .{}, &out_buf);
    expect_eq(missing_res == null, true);
}

fn main() {
    test("1. cap:path — POSIX & Windows dual-track path resolution, EnvPath & PathBufferPool", () => {
        try testCapPathPosixAndWindows();
    });
    test("2. cap:glob — Wildcard matcher (*, **, ?, [abc], {a,b}, !) & GlobWalker", () => {
        try testCapGlobMatcherAndWalker();
    });
    test("3. cap:semver — Semver 2.0.0 parser, precedence comparator & range engine", () => {
        try testCapSemverEngine();
    });
    test("4. cap:which — Cross-platform executable binary locator & permission verifier", () => {
        try testCapWhichLocator();
    });
}

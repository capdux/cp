// Source: cp/cap/test/test_paths_production_deep.cp
// Deep Production Verification Suite for cap:paths & cap.paths
// Covers:
//   1. POSIX / Windows drive letter / UNC `parse`, `dirname`, `basename`, `extname`, `isAbsolute`
//   2. Multi-segment `normalize`, `join`, `resolve`, `relative` folding & root boundary guards
//   3. `safeJoin` & `isSubpath` path traversal guards, prefix trap immunity & null-byte injection rejection
//   4. Case sensitivity, slash conversion (`toPosixSlash` / `toWindowsSlash`), trailing slash & `matchGlobSimple`
//   5. `import cap from "cap"; cap.paths` dynamic mounting & `std.testing.allocator` zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PathError,
    PathStyle,
    parse,
    dirname,
    basename,
    extname,
    isAbsolute,
    containsNullByte,
    hasTrailingSlash,
    trimTrailingSlashes,
    toPosixSlash,
    toWindowsSlash,
    normalize,
    join,
    resolve,
    relative,
    isSubpath,
    safeJoin,
    matchGlobSimple,
} from "cap:paths";

test("cap:paths - POSIX, Windows drive and UNC parse/dirname/basename/extname/isAbsolute") {
    // POSIX standard & hidden file
    const p1 = parse("/home/user/dir/archive.tar.gz", PathStyle.Posix);
    expect_eq(std.mem.eql(u8, p1.root, "/"), true);
    expect_eq(std.mem.eql(u8, p1.dir, "/home/user/dir"), true);
    expect_eq(std.mem.eql(u8, p1.base, "archive.tar.gz"), true);
    expect_eq(std.mem.eql(u8, p1.ext, ".gz"), true);
    expect_eq(std.mem.eql(u8, p1.name, "archive.tar"), true);
    expect_eq(isAbsolute("/home/user", PathStyle.Posix), true);
    expect_eq(isAbsolute("rel/path", PathStyle.Posix), false);

    const hidden = parse("/app/.gitignore", PathStyle.Posix);
    expect_eq(std.mem.eql(u8, hidden.base, ".gitignore"), true);
    expect_eq(std.mem.eql(u8, hidden.ext, ""), true);
    expect_eq(std.mem.eql(u8, hidden.name, ".gitignore"), true);

    // Windows drive letter
    const w1 = parse("C:\\Users\\dev\\main.cp", PathStyle.Windows);
    expect_eq(std.mem.eql(u8, w1.root, "C:\\"), true);
    expect_eq(std.mem.eql(u8, w1.dir, "C:\\Users\\dev"), true);
    expect_eq(std.mem.eql(u8, w1.base, "main.cp"), true);
    expect_eq(std.mem.eql(u8, w1.ext, ".cp"), true);
    expect_eq(std.mem.eql(u8, w1.name, "main"), true);
    expect_eq(isAbsolute("C:\\Users", PathStyle.Windows), true);
    expect_eq(isAbsolute("C:rel\\file", PathStyle.Windows), false);

    // Windows UNC
    const unc = parse("\\\\filesrv\\share\\docs\\spec.md", PathStyle.Windows);
    expect_eq(std.mem.eql(u8, unc.root, "\\\\filesrv\\share\\"), true);
    expect_eq(std.mem.eql(u8, unc.dir, "\\\\filesrv\\share\\docs"), true);
    expect_eq(std.mem.eql(u8, unc.base, "spec.md"), true);
    expect_eq(std.mem.eql(u8, unc.ext, ".md"), true);
    expect_eq(std.mem.eql(u8, unc.name, "spec"), true);
    expect_eq(isAbsolute("\\\\filesrv\\share\\docs", PathStyle.Windows), true);

    // Direct slice helpers
    expect_eq(std.mem.eql(u8, dirname("/a/b/c.txt", PathStyle.Posix), "/a/b"), true);
    expect_eq(std.mem.eql(u8, basename("/a/b/c.txt/", PathStyle.Posix), "c.txt"), true);
    expect_eq(std.mem.eql(u8, extname("readme.md", PathStyle.Posix), ".md"), true);
}

test("cap:paths - normalize, join, resolve, relative folding and root boundaries") {
    const alloc = std.testing.allocator;

    // POSIX root boundary guard
    const n1 = normalize(alloc, "/../../var/./www//../lib/", PathStyle.Posix) catch unreachable;
    defer alloc.free(n1);
    expect_eq(std.mem.eql(u8, n1, "/var/lib/"), true);

    // Relative .. preservation
    const n2 = normalize(alloc, "../a/b/../../c/..", PathStyle.Posix) catch unreachable;
    defer alloc.free(n2);
    expect_eq(std.mem.eql(u8, n2, "../"), false);
    expect_eq(std.mem.eql(u8, n2, ".."), true);

    // Windows drive & UNC boundary guard
    const n3 = normalize(alloc, "C:\\..\\..\\foo\\bar\\..\\baz", PathStyle.Windows) catch unreachable;
    defer alloc.free(n3);
    expect_eq(std.mem.eql(u8, n3, "C:\\foo\\baz"), true);

    const n4 = normalize(alloc, "\\\\srv\\shr\\..\\..\\sub\\file.txt", PathStyle.Windows) catch unreachable;
    defer alloc.free(n4);
    expect_eq(std.mem.eql(u8, n4, "\\\\srv\\shr\\sub\\file.txt"), true);

    // Join
    const j1 = join(alloc, &[_]string{ "/srv", "app", "..", "static", "app.js" }, PathStyle.Posix) catch unreachable;
    defer alloc.free(j1);
    expect_eq(std.mem.eql(u8, j1, "/srv/static/app.js"), true);

    // Resolve right-to-left
    const r1 = resolve(alloc, &[_]string{ "/first", "/second/dir", "../target" }, PathStyle.Posix) catch unreachable;
    defer alloc.free(r1);
    expect_eq(std.mem.eql(u8, r1, "/second/target"), true);

    // Relative
    const rel1 = relative(alloc, "/data/dev/cp/lib", "/data/dev/cp/cap/test", PathStyle.Posix) catch unreachable;
    defer alloc.free(rel1);
    expect_eq(std.mem.eql(u8, rel1, "../cap/test"), true);

    const rel_same = relative(alloc, "/data/dev/cp", "/data/dev/cp/", PathStyle.Posix) catch unreachable;
    defer alloc.free(rel_same);
    expect_eq(std.mem.eql(u8, rel_same, ""), true);
}

test("cap:paths - safeJoin, isSubpath traversal guards and null-byte rejection") {
    const alloc = std.testing.allocator;

    // Valid safeJoin with internal .. that stays inside base_dir
    const sj_ok = safeJoin(alloc, "/var/www/site", &[_]string{ "assets", "sub/../img.png" }, PathStyle.Posix) catch unreachable;
    defer alloc.free(sj_ok);
    expect_eq(std.mem.eql(u8, sj_ok, "/var/www/site/assets/img.png"), true);

    // Prefix trap: /var/www vs /var/www2
    const sub_trap = isSubpath(alloc, "/var/www", "/var/www2/secret", PathStyle.Posix) catch unreachable;
    expect_eq(sub_trap, false);

    const sub_ok = isSubpath(alloc, "/var/www/", "/var/www/index.html", PathStyle.Posix) catch unreachable;
    expect_eq(sub_ok, true);

    // Traversal escape via ../..
    let got_traversal = false;
    * = safeJoin(alloc, "/var/www/site", &[_]string{ "assets", "../../etc/passwd" }, PathStyle.Posix) catch |e| {
        if (e == PathError.PathTraversal) {
            got_traversal = true;
        }
    };
    expect_eq(got_traversal, true);

    // Absolute injection inside rel_parts
    let got_abs_inject = false;
    * = safeJoin(alloc, "/var/www/site", &[_]string{ "/etc/passwd" }, PathStyle.Posix) catch |e| {
        if (e == PathError.PathTraversal) {
            got_abs_inject = true;
        }
    };
    expect_eq(got_abs_inject, true);

    // Windows drive injection inside rel_parts
    let got_drive_inject = false;
    * = safeJoin(alloc, "C:\\sandbox", &[_]string{ "D:secret.txt" }, PathStyle.Windows) catch |e| {
        if (e == PathError.PathTraversal) {
            got_drive_inject = true;
        }
    };
    expect_eq(got_drive_inject, true);

    // Null byte injection
    expect_eq(containsNullByte("abc\x00def"), true);
    expect_eq(containsNullByte("abcdef"), false);
    let got_null = false;
    * = safeJoin(alloc, "/var/www", &[_]string{ "file\x00.txt" }, PathStyle.Posix) catch |e| {
        if (e == PathError.NullByteFound) {
            got_null = true;
        }
    };
    expect_eq(got_null, true);
}

test("cap:paths - case sensitivity, slash conversion, trailing slash and matchGlobSimple") {
    const alloc = std.testing.allocator;

    // Windows case-insensitive subpath & relative
    const win_sub = isSubpath(alloc, "C:\\Users\\Admin", "c:/users/admin/Docs/a.txt", PathStyle.Windows) catch unreachable;
    expect_eq(win_sub, true);

    const win_diff_drive = isSubpath(alloc, "C:\\Users", "D:\\Users\\Admin", PathStyle.Windows) catch unreachable;
    expect_eq(win_diff_drive, false);

    // POSIX case-sensitive subpath
    const posix_sub = isSubpath(alloc, "/var/Data", "/var/data/file", PathStyle.Posix) catch unreachable;
    expect_eq(posix_sub, false);

    // Slash conversion
    const ps = toPosixSlash(alloc, "C:\\a\\b\\c.cp") catch unreachable;
    defer alloc.free(ps);
    expect_eq(std.mem.eql(u8, ps, "C:/a/b/c.cp"), true);

    const ws = toWindowsSlash(alloc, "/usr/local/bin") catch unreachable;
    defer alloc.free(ws);
    expect_eq(std.mem.eql(u8, ws, "\\usr\\local\\bin"), true);

    // Trailing slash helpers
    expect_eq(hasTrailingSlash("/var/log/", PathStyle.Posix), true);
    expect_eq(hasTrailingSlash("/var/log", PathStyle.Posix), false);
    expect_eq(std.mem.eql(u8, trimTrailingSlashes("/var/log///", PathStyle.Posix), "/var/log"), true);
    expect_eq(std.mem.eql(u8, trimTrailingSlashes("///", PathStyle.Posix), "/"), true);
    expect_eq(std.mem.eql(u8, trimTrailingSlashes("C:\\\\", PathStyle.Windows), "C:\\"), true);

    // matchGlobSimple
    expect_eq(matchGlobSimple("lib/paths/index.cp", "lib/*/index.cp"), true);
    expect_eq(matchGlobSimple("test_a1.cp", "test_??.cp"), true);
    expect_eq(matchGlobSimple("test_a12.cp", "test_??.cp"), false);
}

test("cap:paths - cap.paths dynamic mounting and std.testing.allocator zero-leak") {
    const alloc = std.testing.allocator;

    const p = cap.paths.parse("/opt/cp/lib/paths/index.cp", cap.paths.PathStyle.Posix);
    expect_eq(std.mem.eql(u8, p.base, "index.cp"), true);

    const norm = cap.paths.normalize(alloc, "/opt/cp/./lib/../lib/paths", cap.paths.PathStyle.Posix) catch unreachable;
    defer alloc.free(norm);
    expect_eq(std.mem.eql(u8, norm, "/opt/cp/lib/paths"), true);

    const joined = cap.paths.safeJoin(alloc, "/opt/cp", &[_]string{ "lib", "paths", "index.cp" }, cap.paths.PathStyle.Posix) catch unreachable;
    defer alloc.free(joined);
    expect_eq(std.mem.eql(u8, joined, "/opt/cp/lib/paths/index.cp"), true);

    const rel = cap.paths.relative(alloc, "/opt/cp/lib/paths", "/opt/cp/doc/lib/paths.md", cap.paths.PathStyle.Posix) catch unreachable;
    defer alloc.free(rel);
    expect_eq(std.mem.eql(u8, rel, "../../doc/lib/paths.md"), true);
}

// Source: /data/dev/bun/src/glob/ & /data/dev/bun/src/semver/ & /data/dev/bun/src/dotenv/ & /data/dev/bun/src/patch/
// ============================================================================
// cap/test/test_glob_semver_dotenv_patch_deep.cp
// Comprehensive Deep Audit Test Suite for cap:glob, cap:semver, cap:dotenv, cap:patch
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Glob,
    Matcher,
    GlobMatchOptions,
    GlobWalker,
    GlobWalkerOptions,
} from "cap:glob";
import {
    Semver,
    Version,
    ReleaseType,
    SemverRange,
    Query,
} from "cap:semver";
import {
    parse,
    parseWithOptions,
    expandString,
    EnvMap,
    DotenvLoader,
    DotenvOptions,
    config,
} from "cap:dotenv";
import {
    DiffOpKind,
    DiffOp,
    LineDiffer,
    UnifiedDiff,
    UnifiedHunkHeader,
    PatchFileKind,
    PatchErrorKind,
    PatchApplyOptions,
} from "cap:patch";

fn ensureDir(path: [:0]const u8) {
    * = std.c.mkdir(path, 0o755);
}

fn writeTestFile(path: [:0]const u8, content: []const u8) !void {
    const fd = try std.posix.openat(
        std.posix.AT.FDCWD,
        path,
        .{ .ACCMODE = .WRONLY, .CREAT = true, .TRUNC = true },
        0o644,
    );
    defer * = std.c.close(fd);
    let written: usize = 0;
    while (written < content.len) {
        const n = std.c.write(fd, content[written..].ptr, content.len - written);
        if (n <= 0) break;
        written += n as usize;
    }
}

fn testGlobDeep(): !void {
    // 1. Recursive ** and consecutive **/** collapsing
    expect_eq(Glob.match("**", "a/b/c.cp"), true);
    expect_eq(Glob.match("**/**/**/*.cp", "x/y/z.cp"), true);
    expect_eq(Glob.match("a/**/b/**/c.cp", "a/b/c.cp"), true);
    expect_eq(Glob.match("a/**/b/**/c.cp", "a/x/y/b/z/c.cp"), true);
    expect_eq(Glob.match("a/**/b/**/c.cp", "a/x/c.cp"), false);

    // 2. Nested braces, empty branch, brackets inside braces, escaped braces
    expect_eq(Glob.match("{src,{lib,test}/unit}/*.{cp,zig}", "lib/unit/a.cp"), true);
    expect_eq(Glob.match("{src,{lib,test}/unit}/*.{cp,zig}", "test/unit/b.zig"), true);
    expect_eq(Glob.match("{src,{lib,test}/unit}/*.{cp,zig}", "src/main.cp"), true);
    expect_eq(Glob.match("{src,{lib,test}/unit}/*.{cp,zig}", "docs/main.cp"), false);
    expect_eq(Glob.match("config{,.local}.json", "config.json"), true);
    expect_eq(Glob.match("config{,.local}.json", "config.local.json"), true);
    expect_eq(Glob.match("{[a,b],c}.txt", ",.txt"), true);
    expect_eq(Glob.match("{[a,b],c}.txt", "c.txt"), true);
    expect_eq(Glob.match("\\{a,b\\}.txt", "{a,b}.txt"), true);
    expect_eq(Glob.match("\\{a,b\\}.txt", "a.txt"), false);

    // 3. Character classes [a-z], [!a-z], [^a-z], escaped \], literal ] at start, UTF-8 ?
    expect_eq(Glob.match("[]a].txt", "].txt"), true);
    expect_eq(Glob.match("[]a].txt", "a.txt"), true);
    expect_eq(Glob.match("[a\\]b].txt", "].txt"), true);
    expect_eq(Glob.match("file[\\!a].txt", "file!.txt"), true);
    expect_eq(Glob.match("v[!0-9].cp", "va.cp"), true);
    expect_eq(Glob.match("v[!0-9].cp", "v5.cp"), false);
    expect_eq(Glob.match("v[^a-z].cp", "v9.cp"), true);
    expect_eq(Glob.match("v[^a-z].cp", "vz.cp"), false);
    expect_eq(Glob.match("??.txt", "中文.txt"), true);
    expect_eq(Glob.match("?.txt", "中文.txt"), false);
    expect_eq(Glob.match("literal\\*\\?.txt", "literal*?.txt"), true);
    expect_eq(Glob.match("literal\\*\\?.txt", "literalab.txt"), false);

    // 4. Negation with braces & double negation
    expect_eq(Glob.match("!*.{tmp,bak}", "main.cp"), true);
    expect_eq(Glob.match("!*.{tmp,bak}", "cache.bak"), false);
    expect_eq(Glob.match("!!*.cp", "app.cp"), true);
    expect_eq(Glob.match("!!*.cp", "app.zig"), false);

    // 5. Path normalization & absolute vs relative
    let norm_buf: [256]u8 = undefined;
    const normed = Glob.normalizePath("./src//sub/../core/./main.cp", &norm_buf, false);
    expect_eq(std.mem.eql(u8, normed, "src/core/main.cp"), true);
    expect_eq(Glob.match("src/core/*.cp", "./src//sub/../core/./main.cp"), true);
    expect_eq(Glob.match("/src/*.cp", "/src/main.cp"), true);
    expect_eq(Glob.match("/src/*.cp", "src/main.cp"), false);
    expect_eq(Glob.match("src/*.cp", "/src/main.cp"), false);

    const win_opts = GlobMatchOptions{
        case_sensitive: false,
        dot: true,
        normalize: true,
        windows_paths: true,
    };
    expect_eq(Glob.matchWithOptions("src/**/Main.CP", "SRC\\sub\\main.cp", win_opts), true);

    // 6. Case sensitivity & dot options in matchWithOptions
    const nocase_opts = GlobMatchOptions{ case_sensitive: false, dot: false };
    expect_eq(Glob.matchWithOptions("SRC/[a-z]*.CP", "src/App.cp", nocase_opts), true);
    expect_eq(Glob.matchWithOptions("*.cp", ".hidden.cp", nocase_opts), false);
    expect_eq(Glob.matchWithOptions(".*.cp", ".hidden.cp", nocase_opts), true);

    // 7. Real directory traversal via Glob.scanSync (dot, onlyFiles, absolute)
    const allocator = std.heap.page_allocator;
    ensureDir("temp");
    ensureDir("temp/glob_deep_test");
    ensureDir("temp/glob_deep_test/sub");
    ensureDir("temp/glob_deep_test/.hidden_dir");
    try writeTestFile("temp/glob_deep_test/alpha.cp", "// a");
    try writeTestFile("temp/glob_deep_test/sub/BETA.CP", "// b");
    try writeTestFile("temp/glob_deep_test/.env_secret", "SECRET=1");
    try writeTestFile("temp/glob_deep_test/.hidden_dir/inner.cp", "// h");

    // 7a. Default: dot=false, onlyFiles=true -> only alpha.cp and sub/BETA.CP (case_sensitive=false)
    const f1 = try Glob.scanSync(allocator, "**/*", "temp/glob_deep_test", GlobWalkerOptions{
        dot: false,
        onlyFiles: true,
        absolute: false,
    });
    defer {
        for (item of f1) allocator.free(item);
        allocator.free(f1);
    }
    expect_eq(f1.len, 2);

    // 7b. dot=true -> includes .env_secret and .hidden_dir/inner.cp (total 4 files)
    const f2 = try Glob.scanSync(allocator, "**/*", "temp/glob_deep_test", GlobWalkerOptions{
        dot: true,
        onlyFiles: true,
        absolute: false,
    });
    defer {
        for (item of f2) allocator.free(item);
        allocator.free(f2);
    }
    expect_eq(f2.len, 4);

    // 7c. onlyFiles=false -> includes directories ("sub") as well as files
    const f3 = try Glob.scanSync(allocator, "*", "temp/glob_deep_test", GlobWalkerOptions{
        dot: false,
        onlyFiles: false,
        absolute: false,
    });
    defer {
        for (item of f3) allocator.free(item);
        allocator.free(f3);
    }
    let saw_sub_dir = false;
    for (item of f3) {
        if (std.mem.eql(u8, item, "sub")) saw_sub_dir = true;
    }
    expect_eq(saw_sub_dir, true);

    // 7d. absolute=true & case_sensitive=false
    const f4 = try Glob.scanSync(allocator, "**/*.cp", "temp/glob_deep_test", GlobWalkerOptions{
        dot: false,
        onlyFiles: true,
        absolute: true,
        case_sensitive: false,
    });
    defer {
        for (item of f4) allocator.free(item);
        allocator.free(f4);
    }
    expect_eq(f4.len, 2);
    for (item of f4) {
        expect_eq(item.len > 0 && item[0] == '/', true);
    }
}

fn testSemverDeep(): !void {
    // 1. Strict vs Loose parsing
    const s_ok = try Semver.parseStrict("1.2.3-alpha.1+build.42");
    expect_eq(s_ok.major, 1);
    expect_eq(std.mem.eql(u8, s_ok.prerelease, "alpha.1"), true);

    let strict_fail_v = false;
    * = Semver.parseStrict("v1.2.3") catch { strict_fail_v = true; };
    expect_eq(strict_fail_v, true);

    let strict_fail_lz = false;
    * = Semver.parseStrict("01.2.3") catch { strict_fail_lz = true; };
    expect_eq(strict_fail_lz, true);

    let strict_fail_pre_lz = false;
    * = Semver.parseStrict("1.0.0-01") catch { strict_fail_pre_lz = true; };
    expect_eq(strict_fail_pre_lz, true);

    let strict_fail_empty_pre = false;
    * = Semver.parseStrict("1.0.0-") catch { strict_fail_empty_pre = true; };
    expect_eq(strict_fail_empty_pre, true);

    // Loose parsing: v=, 1.2, 1, 1.0.0rc.1
    const l1 = try Semver.parseLoose("v=2.1");
    expect_eq(l1.major, 2);
    expect_eq(l1.minor, 1);
    expect_eq(l1.patch, 0);

    const l2 = try Semver.parseLoose("1.0.0rc.1");
    expect_eq(std.mem.eql(u8, l2.prerelease, "rc.1"), true);

    let clean_buf: [64]u8 = undefined;
    expect_eq(std.mem.eql(u8, Semver.clean("  =v1.2.3-beta.1+exp  ", &clean_buf).?, "1.2.3-beta.1+exp"), true);
    expect_eq(Semver.valid("not-a-ver", &clean_buf) == null, true);

    // 2. Strict Semver 2.0.0 prerelease ordering (including huge numeric ids)
    const big1 = try Semver.parse("1.0.0-999999999999999999991");
    const big2 = try Semver.parse("1.0.0-999999999999999999992");
    expect_eq(Semver.order(big1, big2) == .lt, true);

    // 3. Prerelease range gating (Bun / npm Semver 2.0 spec)
    expect_eq(Semver.satisfiesStr("2.0.0-alpha.1", "^1.0.0"), false);
    expect_eq(Semver.satisfiesStr("1.2.3-beta.4", "^1.2.3-beta.2"), true);
    expect_eq(Semver.satisfiesStr("1.2.3-beta.1", "^1.2.3-beta.2"), false);
    expect_eq(Semver.satisfiesStr("1.2.4-alpha.1", "^1.2.3-beta.2"), false);
    expect_eq(Semver.satisfiesStr("1.0.0-alpha.1", "*"), false);
    expect_eq(Semver.satisfiesStrWithOptions("2.0.0-alpha.1", ">=1.0.0 <2.0.0", true), true);

    // 4. Extra range forms: ~>, ==, partial bounds, hyphen with *, X-range
    expect_eq(Semver.satisfiesStr("1.2.5", "~> 1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "~> 1.2.3"), false);
    expect_eq(Semver.satisfiesStr("1.2.3", "== 1.2.3"), true);
    expect_eq(Semver.satisfiesStr("1.2.9", "<=1.2.x"), true);
    expect_eq(Semver.satisfiesStr("1.3.0", "<=1.2.x"), false);
    expect_eq(Semver.satisfiesStr("2.0.0", ">1.x"), true);
    expect_eq(Semver.satisfiesStr("1.9.9", ">1.x"), false);
    expect_eq(Semver.satisfiesStr("9.0.0", "1.0.0 - *"), true);
    expect_eq(Semver.satisfiesStr("0.9.9", "1.0.0 - *"), false);
    expect_eq(Semver.satisfiesStr("1.5.0", "* - 2.0.0"), true);
    expect_eq(Semver.satisfiesStr("2.0.1", "* - 2.0.0"), false);

    // 5. validRange / isValidRange / validRangeBuf
    expect_eq(Semver.isValidRange("^1.2.3 || ~2.0.0"), true);
    expect_eq(Semver.isValidRange(">=1.0.0 <2.0.0"), true);
    expect_eq(Semver.validRange("  ^1.2.3  ") != null, true);
    expect_eq(Semver.validRange("invalid-range") == null, true);
    expect_eq(Semver.validRange("^1.0.0 || bad") == null, true);
    expect_eq(Semver.validRange("1.0.0 - ") == null, true);

    let r_buf: [128]u8 = undefined;
    const canon = Semver.validRangeBuf("^1.2.3", &r_buf);
    expect_eq(canon != null, true);
    expect_eq(std.mem.eql(u8, canon.?, ">=1.2.3 <2.0.0"), true);

    // 6. sort / rsort / sortStr
    let vers: [5]Version = [
        try Version.parse("2.0.0"),
        try Version.parse("1.0.0-alpha.1"),
        try Version.parse("1.0.0"),
        try Version.parse("1.0.0-alpha"),
        try Version.parse("1.2.0"),
    ];
    Semver.sort(&vers);
    expect_eq(std.mem.eql(u8, vers[0].prerelease, "alpha"), true);
    expect_eq(std.mem.eql(u8, vers[1].prerelease, "alpha.1"), true);
    expect_eq(vers[2].major == 1 && vers[2].minor == 0 && vers[2].prerelease.len == 0, true);
    expect_eq(vers[3].minor == 2, true);
    expect_eq(vers[4].major == 2, true);

    Semver.rsort(&vers);
    expect_eq(vers[0].major == 2, true);
    expect_eq(std.mem.eql(u8, vers[4].prerelease, "alpha"), true);

    let ver_strs: [4][]const u8 = [ "1.10.0", "1.2.0", "1.0.0-rc.1", "1.0.0" ];
    Semver.sortStr(&ver_strs);
    expect_eq(std.mem.eql(u8, ver_strs[0], "1.0.0-rc.1"), true);
    expect_eq(std.mem.eql(u8, ver_strs[1], "1.0.0"), true);
    expect_eq(std.mem.eql(u8, ver_strs[2], "1.2.0"), true);
    expect_eq(std.mem.eql(u8, ver_strs[3], "1.10.0"), true);
}

fn testDotenvDeep(): !void {
    // 1. UTF-8 BOM + CRLF + UTF-8 NBSP blank lines + export prefix
    const bom_crlf_input =
        "\xEF\xBB\xBF" ++
        "# Header comment\r\n" ++
        "\xC2\xA0\xC2\xA0\r\n" ++
        "export APP_NAME=cp-runtime\r\n" ++
        "export\tAPP_PORT = 8443 # inline comment\r\n" ++
        "EMPTY_COMMENT=#ignored\r\n" ++
        "URL_WITH_HASH=http://example.com#section\r\n";

    let env1 = parse(bom_crlf_input);
    expect_eq(std.mem.eql(u8, env1.get("APP_NAME").?, "cp-runtime"), true);
    expect_eq(std.mem.eql(u8, env1.get("APP_PORT").?, "8443"), true);
    expect_eq(std.mem.eql(u8, env1.get("EMPTY_COMMENT").?, ""), true);
    expect_eq(std.mem.eql(u8, env1.get("URL_WITH_HASH").?, "http://example.com#section"), true);

    // 2. Multiline double, single, and backtick quotes with CRLF normalization & escapes
    const multiline_input =
        "MULTI_DOUBLE=\"line1\\nline2\\r\\t\\\"q\\\"\\\\end\r\nnext_raw_line\" # after quote\r\n" ++
        "MULTI_SINGLE='raw1\\nraw2\r\nline2_actual' # after single\r\n" ++
        "MULTI_BACKTICK=`has 'single' and \"double\"\r\nand # hash inside` # trailing\r\n";

    let env2 = parse(multiline_input);
    expect_eq(std.mem.eql(u8, env2.get("MULTI_DOUBLE").?, "line1\nline2\r\t\"q\"\\end\nnext_raw_line"), true);
    expect_eq(std.mem.eql(u8, env2.get("MULTI_SINGLE").?, "raw1\\nraw2\nline2_actual"), true);
    expect_eq(std.mem.eql(u8, env2.get("MULTI_BACKTICK").?, "has 'single' and \"double\"\nand # hash inside"), true);

    // 3. Variable expansion: $VAR, ${VAR}, ${VAR:-default}, ${VAR-default}, nested default, \$ escape
    const exp_input =
        "HOST=127.0.0.1\n" ++
        "PORT=9000\n" ++
        "EMPTY_SET=\n" ++
        "UNQUOTED_BOUNDARY=$HOST:$PORT/api\n" ++
        "DASH_SUFFIX=$HOST-prod\n" ++
        "COLON_DASH_EMPTY=${EMPTY_SET:-fallback_empty}\n" ++
        "DASH_ONLY_EMPTY=${EMPTY_SET-fallback_unset}\n" ++
        "DASH_ONLY_UNSET=${NO_SUCH_VAR-fallback_unset}\n" ++
        "NESTED_DEF=${NO_SUCH_A:-${NO_SUCH_B:-deep_val}}\n" ++
        "CHAINED=$HOST\n" ++
        "CHAINED_2=${CHAINED}:${PORT}\n" ++
        "DBL_ESC_DOLLAR=\"\\${PORT}_literal\"\n";

    let env3 = parse(exp_input);
    expect_eq(std.mem.eql(u8, env3.get("UNQUOTED_BOUNDARY").?, "127.0.0.1:9000/api"), true);
    expect_eq(std.mem.eql(u8, env3.get("DASH_SUFFIX").?, "127.0.0.1-prod"), true);
    expect_eq(std.mem.eql(u8, env3.get("COLON_DASH_EMPTY").?, "fallback_empty"), true);
    expect_eq(std.mem.eql(u8, env3.get("DASH_ONLY_EMPTY").?, ""), true);
    expect_eq(std.mem.eql(u8, env3.get("DASH_ONLY_UNSET").?, "fallback_unset"), true);
    expect_eq(std.mem.eql(u8, env3.get("NESTED_DEF").?, "deep_val"), true);
    expect_eq(std.mem.eql(u8, env3.get("CHAINED_2").?, "127.0.0.1:9000"), true);
    expect_eq(std.mem.eql(u8, env3.get("DBL_ESC_DOLLAR").?, "${PORT}_literal"), true);

    // 4. Real file load with BOM + CRLF
    ensureDir("temp");
    try writeTestFile("temp/dotenv_deep_test.env", "\xEF\xBB\xBFexport FILE_KEY=\"hello\\nworld\"\r\n");
    let loader = config(DotenvOptions{
        path: "temp/dotenv_deep_test.env",
        override: true,
        expand: true,
    });
    expect_eq(std.mem.eql(u8, loader.get("FILE_KEY").?, "hello\nworld"), true);
}

fn testPatchDeep(): !void {
    const allocator = std.heap.page_allocator;

    // 1. Multi-file Unified Diff parsing (modify, create, delete, rename, no-newline)
    const multi_patch =
        "diff --git a/src/app.cp b/src/app.cp\n" ++
        "index 1111111..2222222 100644\n" ++
        "--- a/src/app.cp\n" ++
        "+++ b/src/app.cp\n" ++
        "@@ -1,3 +1,3 @@ fn main()\n" ++
        " line1\n" ++
        "-line2\n" ++
        "+line2_updated\n" ++
        " line3\n" ++
        "@@ -7,2 +7,3 @@\n" ++
        " line7\n" ++
        "+line7b\n" ++
        " line8\n" ++
        "\\ No newline at end of file\n" ++
        "diff --git a/old_name.cp b/new_name.cp\n" ++
        "rename from old_name.cp\n" ++
        "rename to new_name.cp\n" ++
        "diff --git a/added.txt b/added.txt\n" ++
        "new file mode 100644\n" ++
        "--- /dev/null\n" ++
        "+++ b/added.txt\n" ++
        "@@ -0,0 +1,2 @@\n" ++
        "+hello\n" ++
        "+world\n" ++
        "diff --git a/removed.txt b/removed.txt\n" ++
        "deleted file mode 100644\n" ++
        "--- a/removed.txt\n" ++
        "+++ /dev/null\n" ++
        "@@ -1,1 +0,0 @@\n" ++
        "-bye\n";

    let doc = try UnifiedDiff.parse(allocator, multi_patch);
    defer doc.deinit(allocator);

    expect_eq(doc.fileCount(), 4);
    expect_eq(doc.totalHunkCount(), 4);

    expect_eq(doc.files.items[0].kind == PatchFileKind.Modify, true);
    expect_eq(std.mem.eql(u8, doc.files.items[0].targetPath(), "src/app.cp"), true);
    expect_eq(doc.files.items[0].hunks.items.len, 2);
    expect_eq(std.mem.eql(u8, doc.files.items[0].hunks.items[0].header.section, "fn main()"), true);
    expect_eq(doc.files.items[0].hunks.items[1].old_no_newline, true);
    expect_eq(doc.files.items[0].hunks.items[1].new_no_newline, true);

    expect_eq(doc.files.items[1].kind == PatchFileKind.Rename, true);
    expect_eq(std.mem.eql(u8, doc.files.items[1].old_path, "old_name.cp"), true);
    expect_eq(std.mem.eql(u8, doc.files.items[1].new_path, "new_name.cp"), true);

    expect_eq(doc.files.items[2].kind == PatchFileKind.Create, true);
    expect_eq(doc.files.items[3].kind == PatchFileKind.Delete, true);

    // Apply file 0 (multi-hunk + no trailing newline)
    const orig_app = "line1\nline2\nline3\nline4\nline5\nline6\nline7\nline8";
    const app_res = try UnifiedDiff.applyFilePatch(allocator, orig_app, doc.files.items[0], PatchApplyOptions{});
    defer if (app_res.ok) allocator.free(app_res.text);
    expect_eq(app_res.ok, true);
    expect_eq(app_res.exact_hunks, 2);
    expect_eq(std.mem.eql(u8, app_res.text, "line1\nline2_updated\nline3\nline4\nline5\nline6\nline7\nline7b\nline8"), true);

    // Apply file 2 (file creation)
    const create_res = try UnifiedDiff.applyFilePatch(allocator, "", doc.files.items[2], PatchApplyOptions{});
    defer if (create_res.ok) allocator.free(create_res.text);
    expect_eq(create_res.ok, true);
    expect_eq(std.mem.eql(u8, create_res.text, "hello\nworld\n"), true);

    // 2. Fuzzy offset matching (hunk shifted down by +3 lines)
    const shifted_orig = "header1\nheader2\nheader3\nctx_a\nold_target\nctx_b\ntail\n";
    const offset_patch =
        "--- a/f.txt\n" ++
        "+++ b/f.txt\n" ++
        "@@ -1,3 +1,3 @@\n" ++
        " ctx_a\n" ++
        "-old_target\n" ++
        "+new_target\n" ++
        " ctx_b\n";
    const off_res = try UnifiedDiff.apply(allocator, shifted_orig, offset_patch);
    defer if (off_res.ok) allocator.free(off_res.text);
    expect_eq(off_res.ok, true);
    expect_eq(off_res.offset_hunks, 1);
    expect_eq(off_res.applied_offset, 3);
    expect_eq(std.mem.eql(u8, off_res.text, "header1\nheader2\nheader3\nctx_a\nnew_target\nctx_b\ntail\n"), true);

    // 3. Context fuzz matching (outer leading & trailing context lines altered)
    const fuzzed_orig = "different_top\ninner_a\ntarget_line\ninner_b\ndifferent_bot\n";
    const fuzz_patch =
        "--- a/f.txt\n" ++
        "+++ b/f.txt\n" ++
        "@@ -1,5 +1,5 @@\n" ++
        " expected_top\n" ++
        " inner_a\n" ++
        "-target_line\n" ++
        "+patched_line\n" ++
        " inner_b\n" ++
        " expected_bot\n";
    const fuzz_res = try UnifiedDiff.applyWithOptions(allocator, fuzzed_orig, fuzz_patch, PatchApplyOptions{ max_offset: 10, max_fuzz: 1 });
    defer if (fuzz_res.ok) allocator.free(fuzz_res.text);
    expect_eq(fuzz_res.ok, true);
    expect_eq(fuzz_res.fuzz_hunks, 1);
    expect_eq(std.mem.eql(u8, fuzz_res.text, "different_top\ninner_a\npatched_line\ninner_b\ndifferent_bot\n"), true);

    // 4. Error localization on mismatching hunk
    const bad_target = "completely\nunrelated\ncontent\n";
    const err_res = try UnifiedDiff.apply(allocator, bad_target, offset_patch);
    expect_eq(err_res.ok, false);
    expect_eq(err_res.error_kind == PatchErrorKind.HunkMismatch, true);
    expect_eq(err_res.failed_hunk_index, 0);
    expect_eq(err_res.expected_line, 1);

    // 5. Real Myers O(ND) multi-hunk diff + UnifiedDiff.diffLines + roundtrip apply
    const src_a = "L1\nL2_old\nL3\nL4\nL5\nL6\nL7\nL8\nL9\nL10\nL11_old\nL12";
    const src_b = "L1\nL2_new\nL3\nL4\nL5\nL6\nL7\nL8\nL9\nL10\nL11_new\nL12_added\n";

    // Stack LineDiffer.diffLines must find L3..L10 as Equal (not deleted+inserted)
    let ops: [32]DiffOp = undefined;
    const op_count = LineDiffer.diffLines(src_a, src_b, ops[0..32]);
    // Edit distance: 1 del + 1 ins at L2, 1 del + 1 ins at L11, 1 del + 1 ins at L12 = 6
    expect_eq(LineDiffer.editDistance(ops[0..op_count]), 6);

    // UnifiedDiff.diffLines generates standard multi-hunk Unified Diff text & roundtrips cleanly
    const gen_patch = try UnifiedDiff.diffLines(allocator, src_a, src_b, "demo.cp", "demo.cp", 1);
    defer allocator.free(gen_patch);
    expect_eq(std.mem.indexOf(u8, gen_patch, "--- a/demo.cp\n+++ b/demo.cp\n") != null, true);
    expect_eq(std.mem.indexOf(u8, gen_patch, "\\ No newline at end of file") != null, true);

    const rt_res = try UnifiedDiff.apply(allocator, src_a, gen_patch);
    defer if (rt_res.ok) allocator.free(rt_res.text);
    expect_eq(rt_res.ok, true);
    expect_eq(std.mem.eql(u8, rt_res.text, src_b), true);
}

fn main() {
    test("1. cap:glob deep — **, nested braces, char classes, normalize, nocase, dot, scanSync(dot/onlyFiles/absolute)", () => {
        try testGlobDeep();
    });
    test("2. cap:semver deep — strict/loose, Section 11 order, prerelease gating, ~>/==, sort, validRange", () => {
        try testSemverDeep();
    });
    test("3. cap:dotenv deep — BOM, CRLF, UTF-8 spaces, multiline quotes, ${VAR:-def} vs ${VAR-def}, nested, file", () => {
        try testDotenvDeep();
    });
    test("4. cap:patch deep — multi-file/multi-hunk parse, \\ No newline, exact/offset/fuzz apply, Myers Unified Diff roundtrip", () => {
        try testPatchDeep();
    });
}

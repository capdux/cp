// cap/test/test_phase_c3_resolver_testrunner_port.cp
// ============================================================================
// Deep Regression Suite for Phase C-3: cap:resolver (Cache/Advanced) & cap:test_runner
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { tmpPath, writeFileSync, readFileSync, exists, mkdir, remove, rmdir, isDirectory } from "cap:fs";

fn makeDirTree(path: []const u8) {
    if (path.len == 0 || exists(path)) return;
    let i: usize = if (path[0] == '/') 1 else 0;
    while (i <= path.len) : (i += 1) {
        if (i == path.len || path[i] == '/') {
            if (i > 0) {
                const sub = path[0..i];
                if (!exists(sub)) {
                    mkdir(sub);
                }
            }
        }
    }
}

fn makeSymlink(target: []const u8, link_path: []const u8): ! {
    let tbuf: [1024]u8 = undefined;
    let lbuf: [1024]u8 = undefined;
    if (target.len >= tbuf.len || link_path.len >= lbuf.len) return error.NameTooLong;
    @memcpy(tbuf[0..target.len], target);
    tbuf[target.len] = 0;
    @memcpy(lbuf[0..link_path.len], link_path);
    lbuf[link_path.len] = 0;
    const tz: [*:0]const u8 = tbuf[0..target.len :0].ptr;
    const lz: [*:0]const u8 = lbuf[0..link_path.len :0].ptr;
    const rc = std.os.linux.symlinkat(tz, std.os.linux.AT.FDCWD, lz);
    const signed: isize = @bitCast(rc);
    if (signed < 0) return error.SymlinkFailed;
}

fn removeTree(alloc: std.mem.Allocator, path: []const u8) {
    if (!exists(path)) return;
    if (!isDirectory(path)) {
        remove(path);
        return;
    }
    let cache = DirEntryCache.init(alloc);
    defer cache.deinit();
    const info = cache.getOrLoad(path) catch {
        remove(path);
        return;
    };
    for (e of info.entries) {
        const child = std.fmt.allocPrint(alloc, "{s}/{s}", .{ path, e.name }) catch continue;
        if (e.kind == .directory) {
            removeTree(alloc, child);
        } else {
            remove(child);
        }
    }
    rmdir(path);
}

import {
    DirEntryCache,
    RealpathResolver,
    MAX_SYMLINK_DEPTH,
    TsConfigChainLoader,
    BrowserMapAction,
    BrowserFieldMap,
    SideEffectsMode,
    SideEffectsMatcher,
    SelfReferenceResolver,
    ExportResolveStatus,
    ResolveKind,
    ModuleResolver,
    PackageJson,
} from "cap:resolver";

import {
    TestOrderMode,
    Prng64,
    TestOrder,
    TestStatus,
    TestFilter,
    TestContext,
    TestRunner,
    SnapshotUpdateMode,
    SnapshotMatchStatus,
    SnapshotStore,
    ReportFormat,
    PrettyFormatOptions,
    PrettyField,
    PrettyNode,
    PrettyFormat,
    TestReporter,
} from "cap:test_runner";

// ============================================================================
// 1. cap:resolver — DirEntryCache & RealpathResolver (Symlinks + ELOOP)
// ============================================================================
fn testDirCacheAndRealpath(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    let root_buf: [256]u8 = undefined;
    const root = tmpPath("cap_c3_dircache", "", &root_buf);
    const sub_a = try std.fmt.allocPrint(alloc, "{s}/sub_a", .{root});
    const nm_dir = try std.fmt.allocPrint(alloc, "{s}/node_modules", .{root});
    const nm_pkg = try std.fmt.allocPrint(alloc, "{s}/node_modules/pkg-x", .{root});
    const f_pkg = try std.fmt.allocPrint(alloc, "{s}/package.json", .{root});
    const f_tsc = try std.fmt.allocPrint(alloc, "{s}/tsconfig.json", .{root});
    const f_target = try std.fmt.allocPrint(alloc, "{s}/sub_a/real_target.cp", .{root});
    const f_extra = try std.fmt.allocPrint(alloc, "{s}/sub_a/added_later.cp", .{root});
    const l_rel = try std.fmt.allocPrint(alloc, "{s}/link_rel.cp", .{root});
    const l_chain = try std.fmt.allocPrint(alloc, "{s}/link_chain.cp", .{root});
    const l_dir = try std.fmt.allocPrint(alloc, "{s}/link_dir", .{root});
    const l_loop_a = try std.fmt.allocPrint(alloc, "{s}/loop_a", .{root});
    const l_loop_b = try std.fmt.allocPrint(alloc, "{s}/loop_b", .{root});

    makeDirTree(sub_a);
    makeDirTree(nm_pkg);
    writeFileSync(f_pkg, "{\"name\":\"root-app\"}");
    writeFileSync(f_tsc, "{}");
    writeFileSync(f_target, "pub const v = 1;");

    // Multi-level relative + directory symlinks + ELOOP cycle
    try makeSymlink("sub_a/real_target.cp", l_rel);
    try makeSymlink(l_rel, l_chain);
    try makeSymlink("sub_a", l_dir);
    try makeSymlink("loop_b", l_loop_a);
    try makeSymlink("loop_a", l_loop_b);

    defer {
        removeTree(alloc, root);
    }

    // 1.1 DirEntryCache load, hit/miss stats, entry flags & nearest lookup
    let cache = DirEntryCache.init(alloc);
    defer cache.deinit();

    const info1 = try cache.getOrLoad(root);
    expect_eq(info1.hasPackageJson(), true);
    expect_eq(info1.hasTsConfigJson(), true);
    expect_eq(info1.hasNodeModules(), true);
    expect_eq(info1.hasEntry("sub_a"), true);
    expect_eq(info1.isStale(), false);

    const info2 = try cache.getOrLoad(root);
    expect_eq(info2.entryCount(), info1.entryCount());
    const st1 = cache.stats();
    expect_eq(st1.misses, 1);
    expect_eq(st1.hits, 1);

    const nearest_pkg = cache.findNearestPackageDir(sub_a).?;
    expect_eq(std.mem.eql(u8, nearest_pkg, info1.abs_path), true);
    const nearest_nm = cache.findNearestModulesDir(sub_a, false).?;
    expect_eq(std.mem.eql(u8, nearest_nm, nm_dir), true);

    const nm_info = try cache.getOrLoad(nm_pkg);
    expect_eq(nm_info.isInsideNodeModules(), true);

    // 1.2 isStale() + getFresh() after directory mutation
    const sub_info1 = try cache.getOrLoad(sub_a);
    expect_eq(sub_info1.hasEntry("added_later.cp"), false);
    writeFileSync(f_extra, "pub const v = 2;");
    const was_inv = cache.invalidate(sub_a);
    expect_eq(was_inv, true);
    const sub_info2 = try cache.getFresh(sub_a);
    expect_eq(sub_info2.hasEntry("added_later.cp"), true);

    // 1.3 RealpathResolver multi-level symlink traversal
    const real_target = try RealpathResolver.resolve(alloc, f_target);
    const real_via_chain = try RealpathResolver.resolveDetailed(alloc, l_chain, MAX_SYMLINK_DEPTH);
    expect_eq(std.mem.eql(u8, real_via_chain.path, real_target), true);
    expect_eq(real_via_chain.symlinks_followed, 2);

    const through_dir = try std.fmt.allocPrint(alloc, "{s}/real_target.cp", .{l_dir});
    const real_through_dir = try RealpathResolver.resolve(alloc, through_dir);
    expect_eq(std.mem.eql(u8, real_through_dir, real_target), true);

    // 1.4 Symlink loop interception (ELOOP at MAX_SYMLINK_DEPTH = 40)
    let saw_eloop = false;
    * = RealpathResolver.resolve(alloc, l_loop_a) catch |e| {
        saw_eloop = std.mem.eql(u8, @errorName(e), "ELOOP");
    };
    expect_eq(saw_eloop, true);
}

// ============================================================================
// 2. cap:resolver — TsConfigChainLoader, BrowserFieldMap, SideEffects, SelfRef
// ============================================================================
fn testResolverAdvanced(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    let root_buf: [256]u8 = undefined;
    const root = tmpPath("cap_c3_adv", "", &root_buf);
    const cfg_dir = try std.fmt.allocPrint(alloc, "{s}/configs", .{root});
    const app_dir = try std.fmt.allocPrint(alloc, "{s}/apps/web", .{root});
    const app_src = try std.fmt.allocPrint(alloc, "{s}/apps/web/src", .{root});
    const self_dir = try std.fmt.allocPrint(alloc, "{s}/selfpkg", .{root});
    const self_dist = try std.fmt.allocPrint(alloc, "{s}/selfpkg/dist", .{root});
    const self_src = try std.fmt.allocPrint(alloc, "{s}/selfpkg/src", .{root});

    makeDirTree(cfg_dir);
    makeDirTree(app_src);
    makeDirTree(self_dist);
    makeDirTree(self_src);

    defer {
        removeTree(alloc, root);
    }

    // 2.1 Multi-level TsConfig extends chain + baseUrl relocation + paths merge
    const f_grand = try std.fmt.allocPrint(alloc, "{s}/configs/tsconfig.grand.json", .{root});
    const f_base = try std.fmt.allocPrint(alloc, "{s}/configs/tsconfig.base.json", .{root});
    const f_child = try std.fmt.allocPrint(alloc, "{s}/apps/web/tsconfig.json", .{root});

    writeFileSync(f_grand,
        "{\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"target\": \"ES2020\",\n" ++
        "    \"jsx\": \"react\",\n" ++
        "    \"paths\": { \"@shared/*\": [\"shared/*\"], \"@ui/*\": [\"old_ui/*\"] }\n" ++
        "  }\n" ++
        "}\n");
    writeFileSync(f_base,
        "{\n" ++
        "  \"extends\": \"./tsconfig.grand.json\",\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"baseUrl\": \"../apps/web\",\n" ++
        "    \"target\": \"ES2022\",\n" ++
        "    \"paths\": { \"@ui/*\": [\"src/ui/*\"] }\n" ++
        "  }\n" ++
        "}\n");
    writeFileSync(f_child,
        "{\n" ++
        "  \"extends\": \"../../configs/tsconfig.base.json\",\n" ++
        "  \"compilerOptions\": {\n" ++
        "    \"jsx\": \"react-jsx\",\n" ++
        "    \"paths\": { \"@app/*\": [\"src/app/*\"] }\n" ++
        "  }\n" ++
        "}\n");

    const chain_res = try TsConfigChainLoader.loadFile(alloc, f_child);
    expect_eq(chain_res.chain_paths.len, 3);
    // Child overrides target/jsx, inherits & relocates baseUrl "../apps/web" from configs/ -> "." relative to apps/web
    expect_eq(std.mem.eql(u8, chain_res.config.target, "ES2022"), true);
    expect_eq(std.mem.eql(u8, chain_res.config.jsx, "react-jsx"), true);
    expect_eq(std.mem.eql(u8, chain_res.config.base_url, "."), true);
    // Merged paths: @shared/* (grand), @ui/* (overridden by base), @app/* (child)
    expect_eq(std.mem.eql(u8, chain_res.config.resolveAlias(alloc, "@shared/id").?, "shared/id"), true);
    expect_eq(std.mem.eql(u8, chain_res.config.resolveAlias(alloc, "@ui/btn").?, "src/ui/btn"), true);
    expect_eq(std.mem.eql(u8, chain_res.config.resolveAlias(alloc, "@app/main").?, "src/app/main"), true);

    // Circular extends detection
    const f_cyc_a = try std.fmt.allocPrint(alloc, "{s}/configs/cyc_a.json", .{root});
    const f_cyc_b = try std.fmt.allocPrint(alloc, "{s}/configs/cyc_b.json", .{root});
    writeFileSync(f_cyc_a, "{\"extends\": \"./cyc_b.json\"}");
    writeFileSync(f_cyc_b, "{\"extends\": \"./cyc_a.json\"}");
    let saw_cyc = false;
    * = TsConfigChainLoader.loadFile(alloc, f_cyc_a) catch |e| {
        saw_cyc = std.mem.eql(u8, @errorName(e), "CircularExtends");
    };
    expect_eq(saw_cyc, true);

    // 2.2 BrowserFieldMap
    const bmap = try BrowserFieldMap.parse(alloc,
        "{\"name\":\"web-lib\",\"browser\":{\"./server.js\":\"./browser.js\",\"fs\":false,\"./sub/node.cp\":\"./sub/web.cp\"}}");
    expect_eq(bmap.count(), 3);
    const b_srv = bmap.lookup("./server.js");
    expect_eq(b_srv.action, BrowserMapAction.Remap);
    expect_eq(std.mem.eql(u8, b_srv.target, "./browser.js"), true);
    const b_extless = bmap.lookup("./server");
    expect_eq(b_extless.action, BrowserMapAction.Remap);
    expect_eq(std.mem.eql(u8, b_extless.target, "./browser.js"), true);
    expect_eq(bmap.isDisabled("fs"), true);
    expect_eq(std.mem.eql(u8, bmap.remap("fs").?, ""), true);
    expect_eq(bmap.lookup("./other.js").action, BrowserMapAction.Unmapped);

    // 2.3 SideEffectsMatcher (false / true / mixed glob + exact via cap:glob)
    const se_false = try SideEffectsMatcher.parse(alloc, "{\"sideEffects\": false}", "/pkg");
    expect_eq(se_false.mode, SideEffectsMode.None);
    expect_eq(se_false.hasSideEffects("/pkg/src/index.js"), false);

    const se_true = try SideEffectsMatcher.parse(alloc, "{\"sideEffects\": true}", "/pkg");
    expect_eq(se_true.mode, SideEffectsMode.All);
    expect_eq(se_true.hasSideEffects("/pkg/src/anything.js"), true);

    const se_pats = try SideEffectsMatcher.parse(alloc,
        "{\"sideEffects\": [\"*.css\", \"./src/polyfill.js\"]}", "/pkg");
    expect_eq(se_pats.mode, SideEffectsMode.Patterns);
    expect_eq(se_pats.hasSideEffects("/pkg/src/styles/button.css"), true);
    expect_eq(se_pats.hasSideEffects("/pkg/src/polyfill.js"), true);
    expect_eq(se_pats.hasSideEffects("/pkg/src/pure_math.js"), false);

    // 2.4 SelfReferenceResolver
    const f_self_pkg = try std.fmt.allocPrint(alloc, "{s}/package.json", .{self_dir});
    const f_self_sub = try std.fmt.allocPrint(alloc, "{s}/utils.cp", .{self_dist});
    const f_self_idx = try std.fmt.allocPrint(alloc, "{s}/index.cp", .{self_dist});
    writeFileSync(f_self_pkg,
        "{\"name\":\"my-lib\",\"type\":\"module\",\"exports\":{\".\":\"./dist/index.cp\",\"./utils\":\"./dist/utils.cp\"}}");
    writeFileSync(f_self_sub, "pub fn util() {}");
    writeFileSync(f_self_idx, "pub fn main() {}");

    const self_res = SelfReferenceResolver.init(true);
    const conds: [2][]const u8 = ["import", "default"];
    const m_sub = (try self_res.resolveFromDir(alloc, self_src, "my-lib/utils", &conds)).?;
    expect_eq(m_sub.status, ExportResolveStatus.Matched);
    expect_eq(std.mem.eql(u8, m_sub.resolved_path, f_self_sub), true);

    const m_root = (try self_res.resolveFromDir(alloc, self_src, "my-lib", &conds)).?;
    expect_eq(m_root.status, ExportResolveStatus.Matched);
    expect_eq(std.mem.eql(u8, m_root.resolved_path, f_self_idx), true);

    // Non-matching package name returns null
    const m_other = try self_res.resolveFromDir(alloc, self_src, "other-lib/utils", &conds);
    expect_eq(m_other == null, true);

    // Sanity check existing ModuleResolver compatibility
    let mr = ModuleResolver.init(root);
    const r_builtin = try mr.resolve(alloc, root, "cap:io", ResolveKind.ImportStmt);
    expect_eq(std.mem.eql(u8, r_builtin.protocol, "cap"), true);
}

// ============================================================================
// 3. cap:test_runner — TestOrder & TestRunner (Hooks, Describe, .only/.skip/.todo)
// ============================================================================
struct HookTrace {
    events: [32][]const u8 = undefined,
    len: usize = 0,

    fn push(*_, tag: []const u8) {
        if (_.len < 32) {
            _.events[_.len] = tag;
            _.len += 1;
        }
    }
}

fn hookOuterBeforeAll(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("outer:beforeAll");
}
fn hookOuterBeforeEach(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("outer:beforeEach");
}
fn hookOuterAfterEach(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("outer:afterEach");
}
fn hookOuterAfterAll(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("outer:afterAll");
}
fn hookInnerBeforeAll(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("inner:beforeAll");
}
fn hookInnerBeforeEach(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("inner:beforeEach");
}
fn hookInnerAfterEach(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("inner:afterEach");
}
fn hookInnerAfterAll(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("inner:afterAll");
}

fn fnPassingTest(ctx: *TestContext) {
    if (tr := ctx.getUserData(HookTrace)) tr.push("test:inner");
    * = ctx.expectEq(10 + 20, 30);
    * = ctx.expectStrEq("cp", "cp");
}

fn fnFailingTest(ctx: *TestContext) {
    * = ctx.expectStrEq("received_val", "expected_val");
}

fn buildInnerGroup(r: *TestRunner) {
    r.beforeAll(hookInnerBeforeAll) catch {};
    r.beforeEach(hookInnerBeforeEach) catch {};
    r.afterEach(hookInnerAfterEach) catch {};
    r.afterAll(hookInnerAfterAll) catch {};
    * = r.addTest("inner works", fnPassingTest) catch 0;
    * = r.testSkip("skipped inner", fnFailingTest) catch 0;
    * = r.testTodo("todo inner") catch 0;
}

fn buildOuterGroup(r: *TestRunner) {
    r.beforeAll(hookOuterBeforeAll) catch {};
    r.beforeEach(hookOuterBeforeEach) catch {};
    r.afterEach(hookOuterAfterEach) catch {};
    r.afterAll(hookOuterAfterAll) catch {};
    r.describe("inner", buildInnerGroup) catch {};
}

fn fnOnlyPass(ctx: *TestContext) {
    * = ctx.expectTrue(true, "only ok");
}

fn fnShouldBeSkippedByOnly(ctx: *TestContext) {
    ctx.fail("should not execute when .only is active");
}

fn testOrderAndRunner(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 3.1 Deterministic Fisher-Yates shuffle
    let a: [6]usize = [0, 1, 2, 3, 4, 5];
    let b: [6]usize = [0, 1, 2, 3, 4, 5];
    let c: [6]usize = [0, 1, 2, 3, 4, 5];
    TestOrder.shuffleIndices(&a, 987654321);
    TestOrder.shuffleIndices(&b, 987654321);
    TestOrder.shuffleIndices(&c, 123456789);

    let i: usize = 0;
    let diff_count: usize = 0;
    while (i < 6) : (i += 1) {
        expect_eq(a[i], b[i]);
        if (a[i] != c[i]) diff_count += 1;
    }
    expect_eq(diff_count > 0, true);

    const banner = try TestOrder.formatSeedBanner(alloc, 987654321);
    expect_eq(std.mem.indexOf(u8, banner, "987654321") != null, true);

    // 3.2 Nested describe + strict hook nesting order + skip/todo
    let trace = HookTrace{};
    let runner = TestRunner.init(alloc);
    defer runner.deinit();
    runner.setUserData(&trace);
    try runner.describe("outer", buildOuterGroup);

    const summary = try runner.run();
    expect_eq(summary.total, 3);
    expect_eq(summary.passed, 1);
    expect_eq(summary.skipped, 1);
    expect_eq(summary.todo, 1);
    expect_eq(summary.failed, 0);
    expect_eq(summary.isSuccess(), true);

    // Verify exact hook sequence:
    // outer:beforeAll -> inner:beforeAll -> outer:beforeEach -> inner:beforeEach ->
    // test:inner -> inner:afterEach -> outer:afterEach -> inner:afterAll -> outer:afterAll
    expect_eq(trace.len, 9);
    expect_eq(std.mem.eql(u8, trace.events[0], "outer:beforeAll"), true);
    expect_eq(std.mem.eql(u8, trace.events[1], "inner:beforeAll"), true);
    expect_eq(std.mem.eql(u8, trace.events[2], "outer:beforeEach"), true);
    expect_eq(std.mem.eql(u8, trace.events[3], "inner:beforeEach"), true);
    expect_eq(std.mem.eql(u8, trace.events[4], "test:inner"), true);
    expect_eq(std.mem.eql(u8, trace.events[5], "inner:afterEach"), true);
    expect_eq(std.mem.eql(u8, trace.events[6], "outer:afterEach"), true);
    expect_eq(std.mem.eql(u8, trace.events[7], "inner:afterAll"), true);
    expect_eq(std.mem.eql(u8, trace.events[8], "outer:afterAll"), true);

    // 3.3 .only exclusive mode + name filter
    let only_runner = TestRunner.init(alloc);
    defer only_runner.deinit();
    * = try only_runner.addTest("normal excluded", fnShouldBeSkippedByOnly);
    * = try only_runner.testOnly("exclusive test", fnOnlyPass);
    const only_sum = try only_runner.run();
    expect_eq(only_sum.total, 2);
    expect_eq(only_sum.passed, 1);
    expect_eq(only_sum.skipped, 1);
    expect_eq(only_sum.failed, 0);

    let filter_runner = TestRunner.init(alloc);
    defer filter_runner.deinit();
    * = try filter_runner.addTest("alpha match", fnOnlyPass);
    * = try filter_runner.addTest("beta excluded", fnShouldBeSkippedByOnly);
    filter_runner.setFilter(TestFilter{ name_pattern: "alpha" });
    const f_sum = try filter_runner.run();
    expect_eq(f_sum.passed, 1);
    expect_eq(f_sum.skipped, 1);
}

// ============================================================================
// 4. cap:test_runner — SnapshotStore (.snap Disk Roundtrip + Colored Diff)
// ============================================================================
fn testSnapshotStore(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    let dir_buf: [256]u8 = undefined;
    const tmp_dir = tmpPath("cap_c3_snap", "", &dir_buf);
    const snap_path = try std.fmt.allocPrint(alloc, "{s}/__snapshots__/suite.test.snap", .{tmp_dir});
    defer {
        removeTree(alloc, tmp_dir);
    }

    // 4.1 CreateNew + save to disk (including special backtick/${}/backslash chars)
    let store1 = SnapshotStore.init(alloc);
    defer store1.deinit();
    const payload = "line 1 `backtick` ${interp} \\slash\nline 2";
    const r_add = try store1.matchSnapshot("complex 1", payload, SnapshotUpdateMode.CreateNew);
    expect_eq(r_add.status, SnapshotMatchStatus.Added);
    expect_eq(r_add.matched, true);
    try store1.save(snap_path);
    expect_eq(exists(snap_path), true);

    const disk_text = readFileSync(snap_path);
    expect_eq(std.mem.startsWith(u8, disk_text, "// Cap Snapshot v1"), true);
    expect_eq(std.mem.indexOf(u8, disk_text, "exports[`complex 1`] = `") != null, true);

    // 4.2 Load from disk & Check exact match
    let store2 = SnapshotStore.init(alloc);
    defer store2.deinit();
    try store2.load(snap_path);
    const r_ok = try store2.matchSnapshot("complex 1", payload, SnapshotUpdateMode.Check);
    expect_eq(r_ok.status, SnapshotMatchStatus.Passed);
    expect_eq(r_ok.matched, true);

    // 4.3 Mismatch generates colored diff via cap:patch DiffFormatter
    const r_bad = try store2.matchSnapshot("complex 1", "line 1 `backtick` ${interp} \\slash\nline CHANGED", SnapshotUpdateMode.Check);
    expect_eq(r_bad.status, SnapshotMatchStatus.Mismatched);
    expect_eq(r_bad.matched, false);
    expect_eq(r_bad.diff_report.len > 0, true);
    expect_eq(std.mem.indexOf(u8, r_bad.diff_report, "Snapshot") != null, true);
    expect_eq(std.mem.indexOf(u8, r_bad.diff_report, "Received") != null, true);
    expect_eq(std.mem.indexOf(u8, r_bad.diff_report, "\x1b[") != null, true);

    // 4.4 Update mode updates snapshot & persists
    const r_upd = try store2.matchSnapshot("complex 1", "updated value", SnapshotUpdateMode.Update);
    expect_eq(r_upd.status, SnapshotMatchStatus.Updated);
    expect_eq(r_upd.matched, true);
    try store2.save(snap_path);

    let store3 = SnapshotStore.init(alloc);
    defer store3.deinit();
    try store3.load(snap_path);
    const r_recheck = try store3.matchSnapshot("complex 1", "updated value", SnapshotUpdateMode.Check);
    expect_eq(r_recheck.matched, true);
}

// ============================================================================
// 5. cap:test_runner — PrettyFormat & TestReporter (TAP / JUnit XML / JSON)
// ============================================================================
fn fnXmlFail(ctx: *TestContext) {
    ctx.fail("expected <tag attr=\"1\"> & '2' > 0");
}

fn buildReportSuite(r: *TestRunner) {
    * = r.addTest("passes cleanly", fnOnlyPass) catch 0;
    * = r.addTest("escapes <xml> & \"quotes\"", fnXmlFail) catch 0;
    * = r.testSkip("skipped case", null) catch 0;
    * = r.testTodo("todo case") catch 0;
}

fn testPrettyFormatAndReporters(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 5.1 PrettyFormat
    const arr_items: [2]PrettyNode = [PrettyNode.intNode(10), PrettyNode.boolNode(true)];
    const obj_fields: [3]PrettyField = [
        PrettyField{ key: "name", value: PrettyNode.strNode("cp") },
        PrettyField{ key: "score", value: PrettyNode.floatNode(99.5) },
        PrettyField{ key: "tags", value: PrettyNode.arrayNode(&arr_items) },
    ];
    const pretty_obj = try PrettyFormat.formatObject(alloc, &obj_fields, PrettyFormatOptions{});
    expect_eq(std.mem.indexOf(u8, pretty_obj, "name: \"cp\"") != null, true);
    expect_eq(std.mem.indexOf(u8, pretty_obj, "  tags: [\n    10,\n    true\n  ]") != null, true);

    const pretty_val = try PrettyFormat.formatValue(alloc, 42);
    expect_eq(std.mem.eql(u8, pretty_val, "42"), true);

    // 5.2 Run suite with pass, fail (with XML special chars), skip, todo
    let runner = TestRunner.init(alloc);
    defer runner.deinit();
    runner.setOrderMode(TestOrderMode.Declaration, 777);
    try runner.describe("api & core", buildReportSuite);
    const summary = try runner.run();

    // TAP v13
    const tap = try TestReporter.format(alloc, summary, ReportFormat.Tap, "c3-suite");
    expect_eq(std.mem.startsWith(u8, tap, "TAP version 13\n1..4\n"), true);
    expect_eq(std.mem.indexOf(u8, tap, "ok 1 - ") != null, true);
    expect_eq(std.mem.indexOf(u8, tap, "not ok 2 - ") != null, true);
    expect_eq(std.mem.indexOf(u8, tap, "# SKIP") != null, true);
    expect_eq(std.mem.indexOf(u8, tap, "# TODO") != null, true);

    // JUnit XML (verify XML entity escaping)
    const junit = try TestReporter.format(alloc, summary, ReportFormat.JUnit, "suite & \"1\"");
    expect_eq(std.mem.indexOf(u8, junit, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>") != null, true);
    expect_eq(std.mem.indexOf(u8, junit, "<testsuites name=\"suite &amp; &quot;1&quot;\"") != null, true);
    expect_eq(std.mem.indexOf(u8, junit, "&lt;xml&gt; &amp; &quot;quotes&quot;") != null, true);
    expect_eq(std.mem.indexOf(u8, junit, "&lt;tag attr=&quot;1&quot;&gt; &amp; &apos;2&apos; &gt; 0") != null, true);
    expect_eq(std.mem.indexOf(u8, junit, "<skipped />") != null, true);

    // JSON Report (parseable via built-in JSON.parse)
    const json_rep = try TestReporter.format(alloc, summary, ReportFormat.Json, "c3-suite");
    const parsed = try JSON.parse(alloc, json_rep);
    expect_eq(parsed.isObject(), true);
    expect_eq(parsed.get("seed").?.asNumber().?, 777.0);
    const sum_obj = parsed.get("summary").?;
    expect_eq(sum_obj.get("total").?.asNumber().?, 4.0);
    expect_eq(sum_obj.get("passed").?.asNumber().?, 1.0);
    expect_eq(sum_obj.get("failed").?.asNumber().?, 1.0);
    expect_eq(sum_obj.get("skipped").?.asNumber().?, 1.0);
    expect_eq(sum_obj.get("todo").?.asNumber().?, 1.0);
    expect_eq(sum_obj.get("success").?.asBool().?, false);
}

pub fn main() {
    test("cap:resolver DirEntryCache & RealpathResolver (symlinks + ELOOP)", () => {
        try testDirCacheAndRealpath();
    });

    test("cap:resolver TsConfigChainLoader, BrowserFieldMap, SideEffectsMatcher & SelfReferenceResolver", () => {
        try testResolverAdvanced();
    });

    test("cap:test_runner TestOrder & TestRunner (nested describe, hooks, .only/skip/todo)", () => {
        try testOrderAndRunner();
    });

    test("cap:test_runner SnapshotStore (.snap load/save/update & colored diff)", () => {
        try testSnapshotStore();
    });

    test("cap:test_runner PrettyFormat & TestReporter (TAP v13 / JUnit XML / JSON)", () => {
        try testPrettyFormatAndReporters();
    });
}

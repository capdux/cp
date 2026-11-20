// Source: /data/dev/bun/src/bun_alloc/ & /data/dev/bun/src/cli/ & /data/dev/bun/src/bundler/
// cap/test/test_phase_e1_mem_cli_bundler_port.cp
// Deep Verification Suite for Phase E-1: cap:mem, cap:cli, cap:bundler

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    ScopeError,
    AllocationScope,
    BufferFallbackAllocator,
    MaybeOwned,
    MemFdBuffer,
    Arena,
    SharedTensor,
    Cow,
    TaggedPointer,
    MaxHeapAllocator,
    SlabPool,
} from "cap:mem";
import {
    CiVendor,
    CiEnvEntry,
    CiInfo,
    isCI,
    detectCIName,
    SubcommandSpec,
    SubcommandFlag,
    RouteStatus,
    SubcommandRouter,
    SpinnerStyle,
    SpinnerState,
    Spinner,
    ProgressUnit,
    ProgressTaskState,
    NO_PARENT_TASK,
    MultiProgress,
    CliArgs,
    TtyMode,
    AnsiColor,
    ProgressBar,
    CliTable,
    CliTableAlign,
} from "cap:cli";
import {
    ChunkKind,
    ChunkOrder,
    BundleConfig,
    BundleV2,
    CompileTarget,
    StandaloneModuleEntry,
    StandaloneModuleGraph,
    ImportKind,
    ModuleGraph,
    MetafileImport,
    Metafile,
    CodeSplitter,
} from "cap:bundler";

// ============================================================================
// 1. cap:mem — AllocationScope Leak, Peak, Double-Free & Dangling-Free Detection
// ============================================================================
fn testAllocationScope(): ! {
    let scope = AllocationScope.initDefault()?;
    defer scope.deinit();

    const a1 = scope.allocBytes(128, "header")?;
    const a2 = scope.allocBytes(512, "payload")?;
    expect_eq(scope.liveAllocations(), 2 as usize);
    expect_eq(scope.liveBytes(), 640 as usize);
    expect_eq(scope.totalAllocatedBytes(), 640 as usize);
    expect_eq(scope.peakBytes(), 640 as usize);
    expect_eq(scope.isOwned(a1), true);
    expect_eq(scope.hasLeaks(), true);

    // Free a1 -> live drops, peak remains 640
    scope.freeBytes(a1)?;
    expect_eq(scope.liveAllocations(), 1 as usize);
    expect_eq(scope.liveBytes(), 512 as usize);
    expect_eq(scope.peakBytes(), 640 as usize);
    expect_eq(scope.totalFrees(), 1 as usize);
    expect_eq(scope.isOwned(a1), false);

    // Double-free detection on a1
    let saw_double_free = false;
    scope.freeBytes(a1) catch e => {
        if (std.mem.eql(u8, @errorName(e), "DoubleFree")) {
            saw_double_free = true;
        }
    };
    expect_eq(saw_double_free, true);
    expect_eq(scope.doubleFrees(), 1 as usize);

    // Dangling / unowned free detection
    let foreign: [16]u8 = [0xAA; 16];
    let saw_invalid_free = false;
    scope.trackExternalFree(foreign[0..16]) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidFree")) {
            saw_invalid_free = true;
        }
    };
    expect_eq(saw_invalid_free, true);
    expect_eq(scope.invalidFrees(), 1 as usize);

    // assertNoLeaks must fail while a2 is still live, and report must show leak details
    let saw_leak_err = false;
    scope.assertNoLeaks() catch e => {
        if (std.mem.eql(u8, @errorName(e), "MemoryLeak")) {
            saw_leak_err = true;
        }
    };
    expect_eq(saw_leak_err, true);

    let rep_buf: [2048]u8 = undefined;
    const rep = scope.formatReport(&rep_buf);
    expect_eq(std.mem.indexOf(u8, rep, "live_allocations: 1") != null, true);
    expect_eq(std.mem.indexOf(u8, rep, "peak_bytes: 640") != null, true);
    expect_eq(std.mem.indexOf(u8, rep, "double_frees: 1") != null, true);
    expect_eq(std.mem.indexOf(u8, rep, "tag=payload") != null, true);

    // Free a2 and also exercise std.mem.Allocator VTable
    scope.freeBytes(a2)?;
    scope.assertNoLeaks()?;

    const v_alloc = scope.allocator();
    const v_buf = v_alloc.alloc(u8, 256)?;
    expect_eq(scope.liveAllocations(), 1 as usize);
    expect_eq(scope.totalAllocatedBytes(), 896 as usize);
    v_alloc.free(v_buf);
    expect_eq(scope.totalFrees(), 3 as usize);
    scope.assertNoLeaks()?;
}

// ============================================================================
// 2. cap:mem — BufferFallbackAllocator<N>, MaybeOwned<T> & Real Linux MemFdBuffer
// ============================================================================
fn testFallbackMaybeOwnedAndMemFd(): ! {
    // 2.1 BufferFallbackAllocator<64>: stack hot path + heap fallback
    let fba = BufferFallbackAllocator<64>.initDefault();
    let s1 = fba.allocBytes(32)?;
    s1[0] = 0x11;
    s1[31] = 0x22;
    expect_eq(fba.ownsSlice(s1), true);
    expect_eq(fba.stackUsed(), 32 as usize);
    expect_eq(fba.heapAllocs(), 0 as usize);

    const s2 = fba.allocBytes(16)?;
    expect_eq(fba.ownsSlice(s2), true);
    expect_eq(fba.stackUsed(), 48 as usize);

    // Exceeds remaining 16 bytes -> falls back seamlessly to heap
    let h1 = fba.allocBytes(40)?;
    h1[0] = 0x99;
    expect_eq(fba.ownsSlice(h1), false);
    expect_eq(fba.heapAllocs(), 1 as usize);
    expect_eq(fba.heapBytes(), 40 as usize);

    fba.freeBytes(h1);
    fba.freeBytes(s2);
    expect_eq(fba.stackUsed(), 32 as usize);
    fba.reset();
    expect_eq(fba.stackUsed(), 0 as usize);

    // Also test via std.mem.Allocator VTable
    const vt = fba.allocator();
    const vt_small = vt.alloc(u8, 24)?;
    expect_eq(fba.ownsSlice(vt_small), true);
    const vt_big = vt.alloc(u8, 128)?;
    expect_eq(fba.ownsSlice(vt_big), false);
    expect_eq(fba.heapAllocs(), 2 as usize);
    vt.free(vt_big);
    vt.free(vt_small);

    // 2.2 MaybeOwned<u8>
    let mo = MaybeOwned<u8>.initBorrowed("cp-borrowed-slice");
    expect_eq(mo.isOwned(), false);
    expect_eq(std.mem.eql(u8, mo.asSlice(), "cp-borrowed-slice"), true);
    let owned_mut = mo.toOwned()?;
    expect_eq(mo.isOwned(), true);
    owned_mut[0] = 'C';
    owned_mut[1] = 'P';
    expect_eq(std.mem.eql(u8, mo.asSlice(), "CP-borrowed-slice"), true);
    mo.deinit();
    expect_eq(mo.isOwned(), false);
    expect_eq(mo.len(), 0 as usize);

    let mo_copy = MaybeOwned<u8>.initOwnedCopyDefault("owned-copy")?;
    defer mo_copy.deinit();
    expect_eq(mo_copy.isOwned(), true);
    expect_eq(std.mem.eql(u8, mo_copy.asSlice(), "owned-copy"), true);

    // 2.3 Real Linux MemFdBuffer (memfd_create + pwrite + pread + mmap + cloneView)
    const init_payload = "CP-MemFd-ZeroCopy-Payload-2026";
    let mfd = MemFdBuffer.create(init_payload, "cp-e1-test")?;
    defer mfd.deinit();

    expect_eq(mfd.isValid(), true);
    expect_eq(mfd.fd() >= 0, true);
    expect_eq(mfd.len(), init_payload.len);
    expect_eq(std.mem.eql(u8, mfd.asSlice(), init_payload), true);

    // Verify real kernel storage via pread syscall
    let fd_read_buf: [64]u8 = [0; 64];
    const n_read = mfd.readFd(0, fd_read_buf[0..init_payload.len])?;
    expect_eq(n_read, init_payload.len);
    expect_eq(std.mem.eql(u8, fd_read_buf[0..n_read], init_payload), true);

    // Shared mmap cloneView: mutation through clone is visible in primary view
    let cloned = mfd.cloneView()?;
    defer cloned.deinit();
    expect_eq(cloned.fd() != mfd.fd(), true);
    let cmut = cloned.asMutSlice();
    cmut[0] = 'Z';
    cmut[1] = 'X';
    expect_eq(mfd.asSlice()[0], 'Z');
    expect_eq(mfd.asSlice()[1], 'X');
}

// ============================================================================
// 3. cap:cli — CiInfo Vendor/PR/TTY Detection, SubcommandRouter & MultiProgress
// ============================================================================
fn testCliCiSubcommandAndProgress(): ! {
    // 3.1 CiInfo deterministic vendor & PR matrix
    const gh_env: [3]CiEnvEntry = [
        CiEnvEntry{ key: "GITHUB_ACTIONS", value: "true" },
        CiEnvEntry{ key: "GITHUB_EVENT_NAME", value: "pull_request" },
        CiEnvEntry{ key: "COLUMNS", value: "120" },
    ];
    const gh = CiInfo.detectWithEnv(&gh_env, false, 80, 24);
    expect_eq(gh.is_ci, true);
    expect_eq(gh.vendor == CiVendor.GitHubActions, true);
    expect_eq(std.mem.eql(u8, gh.vendor_name, "GitHub Actions"), true);
    expect_eq(gh.is_pr, true);
    expect_eq(gh.supports_ansi, true);
    expect_eq(gh.supports_hyperlink, true);
    expect_eq(gh.terminal_width, 120 as u16);

    const gl_env: [2]CiEnvEntry = [
        CiEnvEntry{ key: "GITLAB_CI", value: "true" },
        CiEnvEntry{ key: "CI_MERGE_REQUEST_ID", value: "99" },
    ];
    const gl = CiInfo.detectWithEnv(&gl_env, false, 80, 24);
    expect_eq(gl.vendor == CiVendor.GitLabCi, true);
    expect_eq(gl.is_pr, true);

    const tr_env: [2]CiEnvEntry = [
        CiEnvEntry{ key: "TRAVIS", value: "true" },
        CiEnvEntry{ key: "TRAVIS_PULL_REQUEST", value: "false" },
    ];
    const tr = CiInfo.detectWithEnv(&tr_env, false, 80, 24);
    expect_eq(tr.vendor == CiVendor.Travis, true);
    expect_eq(tr.is_pr, false);

    const jk_env: [3]CiEnvEntry = [
        CiEnvEntry{ key: "JENKINS_URL", value: "https://ci.local" },
        CiEnvEntry{ key: "BUILD_ID", value: "42" },
        CiEnvEntry{ key: "CHANGE_ID", value: "17" },
    ];
    const jk = CiInfo.detectWithEnv(&jk_env, false, 80, 24);
    expect_eq(jk.vendor == CiVendor.Jenkins, true);
    expect_eq(jk.is_pr, true);

    const az_env: [2]CiEnvEntry = [
        CiEnvEntry{ key: "TF_BUILD", value: "True" },
        CiEnvEntry{ key: "BUILD_REASON", value: "PullRequest" },
    ];
    expect_eq(CiInfo.detectWithEnv(&az_env, false, 80, 24).vendor == CiVendor.AzurePipelines, true);
    expect_eq(CiInfo.detectWithEnv(&az_env, false, 80, 24).is_pr, true);

    const cf_env: [2]CiEnvEntry = [
        CiEnvEntry{ key: "CF_PAGES", value: "1" },
        CiEnvEntry{ key: "NO_COLOR", value: "1" },
    ];
    const cf = CiInfo.detectWithEnv(&cf_env, true, 100, 40);
    expect_eq(cf.vendor == CiVendor.CloudflarePages, true);
    expect_eq(cf.supports_ansi, false);
    expect_eq(cf.supports_hyperlink, false);

    // Live process ioctl + env probe must succeed safely
    const live = CiInfo.detect(1);
    expect_eq(live.terminal_width > 0, true);
    expect_eq(live.terminal_height > 0, true);
    * = isCI();
    * = detectCIName();

    // 3.2 SubcommandRouter: nested commands, aliases, Levenshtein didYouMean & --help
    let router = SubcommandRouter.init("cap", "Fast CP Systems Toolchain", "Docs: https://cp-lang.org");
    expect_eq(router.addCommand(SubcommandSpec{
        name: "build",
        alias: "b",
        usage_args: "<entry>",
        description: "Compile CP project",
    }), true);
    expect_eq(router.addCommand(SubcommandSpec{
        name: "install",
        alias: "i",
        usage_args: "[pkg]",
        description: "Install dependencies",
    }), true);
    expect_eq(router.addCommand(SubcommandSpec{
        name: "pm",
        description: "Package manager utilities",
    }), true);
    expect_eq(router.addCommand(SubcommandSpec{
        name: "cache",
        parent: "pm",
        description: "Inspect or clean cache",
    }), true);
    expect_eq(router.addFlag(SubcommandFlag{
        long_name: "help",
        short_name: "h",
        description: "Print help text",
    }), true);
    expect_eq(router.addFlag(SubcommandFlag{
        long_name: "release",
        short_name: "r",
        description: "Build release artifacts",
    }), true);

    const argv_alias: [2][]const u8 = ["i", "lodash"];
    const m_alias = router.resolve(&argv_alias);
    expect_eq(m_alias.status == RouteStatus.Matched, true);
    expect_eq(std.mem.eql(u8, m_alias.command_name, "install"), true);

    const argv_nested: [3][]const u8 = ["pm", "cache", "--help"];
    const m_nested = router.resolve(&argv_nested);
    expect_eq(m_nested.status == RouteStatus.HelpRequested, true);
    expect_eq(std.mem.eql(u8, m_nested.command_name, "cache"), true);
    expect_eq(std.mem.eql(u8, m_nested.parent_name, "pm"), true);

    const argv_typo: [1][]const u8 = ["isntall"];
    const m_typo = router.resolve(&argv_typo);
    expect_eq(m_typo.status == RouteStatus.UnknownCommand, true);
    expect_eq(std.mem.eql(u8, m_typo.suggestion.?, "install"), true);

    expect_eq(SubcommandRouter.levenshteinDistance("build", "buld"), 1 as usize);
    expect_eq(std.mem.eql(u8, router.didYouMean("chace", "pm").?, "cache"), true);

    let help_buf: [2048]u8 = undefined;
    const help_txt = router.formatHelp(&help_buf, true, "");
    expect_eq(std.mem.indexOf(u8, help_txt, "Usage:") != null, true);
    expect_eq(std.mem.indexOf(u8, help_txt, "install") != null, true);
    expect_eq(std.mem.indexOf(u8, help_txt, "--release") != null, true);

    // 3.3 MultiProgress & Spinner
    let sp = Spinner.init("Linking", SpinnerStyle.Dots);
    const f0 = sp.tick();
    const f1 = sp.tick();
    expect_eq(std.mem.eql(u8, f0, f1), false);
    sp.succeed("Linked");
    expect_eq(sp.state == SpinnerState.Succeeded, true);

    let mp = MultiProgress.init("Build Pipeline", 10);
    const t_root = mp.addChildTaskAt(NO_PARENT_TASK, "Compile", 100, ProgressUnit.Files, SpinnerStyle.Dots, 1000000000);
    const t_sub = mp.addChildTaskAt(t_root, "Emit Bytes", 4096, ProgressUnit.Bytes, SpinnerStyle.Line, 1000000000);
    mp.advanceTaskAt(t_root, 50, 2000000000); // 50 files in 1s -> 50 files/s, ETA 1s
    mp.advanceTaskAt(t_sub, 2048, 2000000000); // 2048 B in 1s -> 2.0 KB/s, ETA 1s
    mp.completeTask(t_root, "Compiled");

    expect_eq(mp.taskCount(), 2 as usize);
    expect_eq(mp.getTask(t_root).?.state == ProgressTaskState.Succeeded, true);

    let tree_buf: [2048]u8 = undefined;
    const tree = mp.renderTree(&tree_buf, false);
    expect_eq(std.mem.indexOf(u8, tree, "Build Pipeline") != null, true);
    expect_eq(std.mem.indexOf(u8, tree, "├─") != null, true);
    expect_eq(std.mem.indexOf(u8, tree, "2.0 KB/s") != null, true);
    expect_eq(std.mem.indexOf(u8, tree, "ETA 1s") != null, true);
}

// ============================================================================
// 4. cap:bundler — Metafile JSON + Analysis Tree & CodeSplitter Multi-Entry Graph
// ============================================================================
fn testBundlerMetafileAndSplitting(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // 4.1 Metafile JSON & colored size breakdown tree
    let mf = Metafile.init(alloc);
    defer mf.deinit();

    const in_main = mf.addInput("src/main.ts", 2048, "esm")?;
    * = mf.addInput("src/shared.ts", 4096, "esm")?;
    mf.addInputImport(in_main, MetafileImport{
        path: "src/shared.ts",
        kind: ImportKind.ImportStmt,
        original: "./shared",
    })?;
    mf.addInputImport(in_main, MetafileImport{
        path: "fs",
        kind: ImportKind.ImportStmt,
        external: true,
    })?;

    const out_main = mf.addOutput("dist/main.js", 5120, "src/main.ts")?;
    mf.addOutputInput(out_main, "src/main.ts", 1536)?;
    mf.addOutputInput(out_main, "src/shared.ts", 3584)?;
    mf.addOutputImport(out_main, MetafileImport{
        path: "dist/chunk-deadbeef.js",
        kind: ImportKind.DynamicImport,
    })?;
    mf.addOutputExport(out_main, "default")?;
    mf.setOutputCssBundle(out_main, "dist/main.css")?;

    const json = mf.toJsonAlloc(alloc)?;
    expect_eq(std.mem.indexOf(u8, json, "\"inputs\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"outputs\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"bytesInOutput\": 3584") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"entryPoint\": \"src/main.ts\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"cssBundle\": \"dist/main.css\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json, "\"external\": true") != null, true);

    const table = mf.formatAnalysisTable(alloc, true)?;
    expect_eq(std.mem.indexOf(u8, table, "dist/main.js") != null, true);
    expect_eq(std.mem.indexOf(u8, table, "src/shared.ts") != null, true);
    expect_eq(std.mem.indexOf(u8, table, "└─") != null, true);

    // 4.2 CodeSplitter multi-entry bitmask chunking + AssetManifest
    let mg = ModuleGraph.init(alloc);
    defer mg.deinit();

    const id_app = mg.addModule("src/app.cp", true, 1200)?;
    const id_admin = mg.addModule("src/admin.cp", true, 1500)?;
    const id_app_only = mg.addModule("src/app_only.cp", false, 400)?;
    const id_shared = mg.addModule("src/shared.cp", false, 2000)?;
    const id_core = mg.addModule("src/core.cp", false, 800)?;
    * = mg.addModule("src/dead_unused.cp", false, 999)?;

    mg.addDependency(id_app, id_app_only, "./app_only", ImportKind.ImportStmt)?;
    mg.addDependency(id_app, id_shared, "./shared", ImportKind.ImportStmt)?;
    mg.addDependency(id_admin, id_shared, "./shared", ImportKind.ImportStmt)?;
    mg.addDependency(id_shared, id_core, "./core", ImportKind.ImportStmt)?;

    let split_res = CodeSplitter.split(alloc, *mg, "cp")?;
    defer split_res.deinit();

    expect_eq(split_res.dead_module_count, 1 as usize);
    expect_eq(split_res.entry_chunk_count, 2 as usize);
    expect_eq(split_res.shared_chunk_count, 1 as usize);
    expect_eq(split_res.chunks.len, 3 as usize);

    const shared_chunks = split_res.findSharedChunks(alloc)?;
    expect_eq(shared_chunks.len, 1 as usize);
    const sc = shared_chunks[0];
    expect_eq(sc.kind == ChunkKind.SharedChunk, true);
    expect_eq(sc.entry_mask, 3 as u64); // reachable from entry 0 and entry 1 (0b11)
    expect_eq(sc.module_ids.len, 2 as usize); // shared + core
    expect_eq(sc.total_bytes, 2800 as usize);
    expect_eq(std.mem.startsWith(u8, sc.name, "chunk-"), true);
    expect_eq(std.mem.endsWith(u8, sc.name, ".cp"), true);

    // Entry chunk for app must import the shared chunk
    let app_chunk_imports_shared = false;
    for (ch of split_res.chunks) {
        if (ch.kind == ChunkKind.EntryPoint && std.mem.startsWith(u8, ch.name, "app-")) {
            expect_eq(ch.module_ids.len, 2 as usize); // app + app_only
            for (dep_idx of ch.imports_chunks) {
                if (dep_idx == sc.id) {
                    app_chunk_imports_shared = true;
                }
            }
        }
    }
    expect_eq(app_chunk_imports_shared, true);

    // AssetManifest lookup & JSON
    expect_eq(split_res.manifest.findByLogical("src/dead_unused.cp") == null, true);
    const shared_manifest = split_res.manifest.findByLogical("src/shared.cp").?;
    expect_eq(std.mem.eql(u8, shared_manifest.output_path, sc.name), true);
    const mf_json = split_res.manifest.toJsonAlloc(alloc)?;
    expect_eq(std.mem.indexOf(u8, mf_json, "src/app.cp") != null, true);
    expect_eq(std.mem.indexOf(u8, mf_json, sc.name) != null, true);
}

fn testBackwardCompatExports(): ! {
    let arena_alloc = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena_alloc.deinit();
    const alloc = arena_alloc.allocator();

    // cap:mem existing exports
    let mha = MaxHeapAllocator.init(512);
    defer mha.deinit();
    expect_eq(mha.alloc(128) != null, true);
    let cow = Cow.initBorrowed("compat");
    defer cow.release();
    expect_eq(std.mem.eql(u8, cow.slice(), "compat"), true);
    let pool = SlabPool.init(32, 4);
    defer pool.release();
    const sl = pool.alloc().?;
    expect_eq(pool.free(sl), true);
    const tp = TaggedPointer.pack(0x123400, 7);
    expect_eq(tp.tag(), 7 as u16);
    * = Arena;
    * = SharedTensor;

    // cap:cli existing exports
    const raw_cli: [2][]const u8 = ["--mode=fast", "main.cp"];
    const ca = CliArgs.parse(&raw_cli);
    expect_eq(std.mem.eql(u8, ca.getOption("mode").?, "fast"), true);
    expect_eq(TtyMode.Normal as i32, 0);
    expect_eq(AnsiColor.RESET.len > 0, true);
    let pb = ProgressBar.init(10, 5);
    pb.tick(5);
    expect_eq(pb.percent(), 50 as u8);
    let tbl = CliTable.init(2);
    tbl.setHeader(0, "K", CliTableAlign.Left);
    expect_eq(tbl.addRow2("a", "b"), true);

    // cap:bundler existing exports
    const cfg = BundleConfig.productionBrowser("dist");
    let bv2 = BundleV2.init(cfg);
    expect_eq(bv2.chunkCount(), 0 as usize);
    const tgt = CompileTarget.parse("linux-x64-musl")?;
    expect_eq(tgt.isUnix(), true);
    let sg = StandaloneModuleGraph.init(alloc);
    * = sg.addModule(StandaloneModuleEntry{ path: "/$cpfs/a.js", contents: "1", is_entry: true })?;
    const blob = sg.serialize(alloc)?;
    let decoded_sg = StandaloneModuleGraph.deserialize(alloc, blob)?;
    expect_eq(std.mem.eql(u8, decoded_sg.findContent("/$cpfs/a.js").?, "1"), true);
}

fn main() {
    test("1. cap:mem AllocationScope leak, peak, double-free & dangling-free detection", () => {
        try testAllocationScope();
    });
    test("2. cap:mem BufferFallbackAllocator<N>, MaybeOwned<T> & real Linux MemFdBuffer", () => {
        try testFallbackMaybeOwnedAndMemFd();
    });
    test("3. cap:cli CiInfo vendor/PR/TTY detection, SubcommandRouter & MultiProgress", () => {
        try testCliCiSubcommandAndProgress();
    });
    test("4. cap:bundler Metafile JSON/tree report & CodeSplitter multi-entry bitmask", () => {
        try testBundlerMetafileAndSplitting();
    });
    test("5. cap:mem / cap:cli / cap:bundler 100% backward-compatible exports", () => {
        try testBackwardCompatExports();
    });
}

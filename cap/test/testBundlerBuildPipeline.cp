import { test, assert, assertEq } from "cap:test";
import {
    BuildPipelineError,
    BuildTargetKind,
    BuildLoaderKind,
    BuildCacheStatus,
    BuildPipelineStage,
    BuildLoaderOverride,
    BuildDefinePair,
    BuildPipelineFlags,
    BuildCacheUnit,
    BuildDepEdge,
    IncrementalBuildCache,
    BuildPipelineSession,
    parseBuildTargetKind,
    buildTargetName,
    parseBuildLoaderKind,
    buildLoaderName,
    resolveBuildLoader,
    isCapBuiltinSpecifier,
    matchExternalPattern,
    isExternalSpecifier,
    lookupBuildDefine,
    computeBuildFlagsFingerprint,
    formatBuildPipelineReport,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("Target/Loader parsing and extension routing with user overrides", () => {
    assert(parseBuildTargetKind("native-exe").? == BuildTargetKind.NativeExe);
    assert(parseBuildTargetKind("nest").? == BuildTargetKind.NestContainer);
    assert(parseBuildTargetKind("wasm").? == BuildTargetKind.Wasm);
    assert(strEql(buildTargetName(BuildTargetKind.NestContainer), "nest"));

    assert(parseBuildLoaderKind("cp").? == BuildLoaderKind.Cp);
    assert(parseBuildLoaderKind("dataurl").? == BuildLoaderKind.DataUrl);
    assert(strEql(buildLoaderName(BuildLoaderKind.Tsx), "tsx"));

    const overrides: [2]BuildLoaderOverride = [
        BuildLoaderOverride{ extension: ".svg", loader: BuildLoaderKind.DataUrl },
        BuildLoaderOverride{ extension: ".sql", loader: BuildLoaderKind.Text },
    ];
    assert(resolveBuildLoader("src/main.cp", overrides[0..]) == BuildLoaderKind.Cp);
    assert(resolveBuildLoader("src/native.ZIG", overrides[0..]) == BuildLoaderKind.Zig);
    assert(resolveBuildLoader("assets/icon.SVG", overrides[0..]) == BuildLoaderKind.DataUrl);
    assert(resolveBuildLoader("db/schema.sql", overrides[0..]) == BuildLoaderKind.Text);
    assert(resolveBuildLoader("README.md", overrides[0..]) == BuildLoaderKind.Text);
    assert(resolveBuildLoader("dist/mod.wasm", overrides[0..]) == BuildLoaderKind.Wasm);
    assert(resolveBuildLoader("assets/blob.bin", overrides[0..]) == BuildLoaderKind.File);
});

test("External specifier matching: cap:* builtins, exact subpaths, and wildcards", () => {
    assert(isCapBuiltinSpecifier("cap:io"));
    assert(isCapBuiltinSpecifier("cap:pkg"));
    assert(!isCapBuiltinSpecifier("cap:"));
    assert(!isCapBuiltinSpecifier("lodash"));

    assert(matchExternalPattern("react", "react"));
    assert(matchExternalPattern("react", "react/jsx-runtime"));
    assert(!matchExternalPattern("react", "react-dom"));
    assert(matchExternalPattern("@acme/*", "@acme/core"));
    assert(matchExternalPattern("*.node", "native/addon.node"));

    const externals: [2][]const u8 = ["react", "@native/*"];
    assert(isExternalSpecifier("cap:http", externals[0..], true));
    assert(!isExternalSpecifier("cap:http", externals[0..], false));
    assert(isExternalSpecifier("react/client", externals[0..], true));
    assert(isExternalSpecifier("@native/sqlite", externals[0..], true));
    assert(!isExternalSpecifier("./local.cp", externals[0..], true));
});

test("Build defines lookup and deterministic 64-bit flags fingerprint sensitivity", () => {
    const defs1: [2]BuildDefinePair = [
        BuildDefinePair{ key: "NODE_ENV", value: "\"development\"" },
        BuildDefinePair{ key: "NODE_ENV", value: "\"production\"" },
    ];
    assert(strEql(lookupBuildDefine(defs1[0..], "NODE_ENV").?, "\"production\""));
    assert(lookupBuildDefine(defs1[0..], "MISSING") == null);

    const ov: [0]BuildLoaderOverride = [];
    const f1 = BuildPipelineFlags.defaults();
    let f2 = BuildPipelineFlags.defaults();
    f2.minify = true;

    const h1 = computeBuildFlagsFingerprint(f1, defs1[0..], ov[0..]);
    const h1_repeat = computeBuildFlagsFingerprint(f1, defs1[0..], ov[0..]);
    const h2 = computeBuildFlagsFingerprint(f2, defs1[0..], ov[0..]);
    assertEq(h1, h1_repeat);
    assert(h1 != h2);
});

test("IncrementalBuildCache reverse-dependency BFS invalidates minimal affected closure", () => {
    // Graph: 0 (app) -> 1 (auth) -> 2 (crypto); 3 (cli) -> 4 (fmt) [independent branch]
    let backing: [8]BuildCacheUnit = [BuildCacheUnit.empty(); 8];
    let cache = IncrementalBuildCache.init(backing[0..]);
    const flags_hash: u64 = 0xABCDEF01;

    * = cache.registerUnit("src/app.cp", BuildLoaderKind.Cp, 0x100, flags_hash, 0, 0xA0, 1000, true) catch unreachable;
    * = cache.registerUnit("src/auth.cp", BuildLoaderKind.Cp, 0x200, flags_hash, 0, 0xA1, 800, true) catch unreachable;
    * = cache.registerUnit("src/crypto.cp", BuildLoaderKind.Cp, 0x300, flags_hash, 0, 0xA2, 600, true) catch unreachable;
    * = cache.registerUnit("src/cli.cp", BuildLoaderKind.Cp, 0x400, flags_hash, 0, 0xA3, 500, true) catch unreachable;
    * = cache.registerUnit("src/fmt.cp", BuildLoaderKind.Cp, 0x500, flags_hash, 0, 0xA4, 400, true) catch unreachable;

    const edges: [3]BuildDepEdge = [
        BuildDepEdge{ from_unit: 0, to_unit: 1 },
        BuildDepEdge{ from_unit: 1, to_unit: 2 },
        BuildDepEdge{ from_unit: 3, to_unit: 4 },
    ];
    let queue: [8]u32 = [0; 8];

    // 1. All hashes unchanged -> 100% cache hit
    const same_hashes: [5]u64 = [0x100, 0x200, 0x300, 0x400, 0x500];
    const sum1 = cache.evaluateAndPropagate(same_hashes[0..], flags_hash, edges[0..], queue[0..]) catch unreachable;
    assertEq(sum1.fresh_hits, @as(u32, 5));
    assertEq(sum1.units_to_rebuild, @as(u32, 0));
    assertEq(sum1.hit_rate_permille, @as(u32, 1000));

    // 2. Modify only unit 2 (crypto.cp) -> invalidates 2 (StaleSource), 1 (StaleDep), 0 (StaleDep);
    //    independent units 3 & 4 remain FreshHit!
    const changed_hashes: [5]u64 = [0x100, 0x200, 0x399, 0x400, 0x500];
    const sum2 = cache.evaluateAndPropagate(changed_hashes[0..], flags_hash, edges[0..], queue[0..]) catch unreachable;
    assertEq(sum2.fresh_hits, @as(u32, 2));
    assertEq(sum2.stale_source, @as(u32, 1));
    assertEq(sum2.stale_dep, @as(u32, 2));
    assertEq(sum2.units_to_rebuild, @as(u32, 3));
    assertEq(sum2.hit_rate_permille, @as(u32, 400));

    assert(backing[2].status == BuildCacheStatus.StaleSource);
    assert(backing[1].status == BuildCacheStatus.StaleDep);
    assert(backing[0].status == BuildCacheStatus.StaleDep);
    assert(backing[3].status == BuildCacheStatus.FreshHit);
    assert(backing[4].status == BuildCacheStatus.FreshHit);
});

test("Global flags change triggers StaleFlags and commitCompiledUnit restores FreshHit", () => {
    let backing: [4]BuildCacheUnit = [BuildCacheUnit.empty(); 4];
    let cache = IncrementalBuildCache.init(backing[0..]);
    * = cache.registerUnit("src/a.cp", BuildLoaderKind.Cp, 0x11, 0xF1, 0, 0xAA, 512, true) catch unreachable;
    * = cache.registerUnit("src/b.cp", BuildLoaderKind.Cp, 0x22, 0xF1, 0, 0xBB, 256, false) catch unreachable;

    const empty_edges: [0]BuildDepEdge = [];
    const cur_hashes: [2]u64 = [0x11, 0x22];
    let queue: [4]u32 = [0; 4];

    const inv = cache.evaluateAndPropagate(cur_hashes[0..], 0xF2, empty_edges[0..], queue[0..]) catch unreachable;
    assertEq(inv.stale_flags, @as(u32, 1));
    assertEq(inv.missing_units, @as(u32, 1));
    assertEq(inv.units_to_rebuild, @as(u32, 2));

    cache.commitCompiledUnit(0, 0x11, 0xF2, 0, 0xCC, 600) catch unreachable;
    cache.commitCompiledUnit(1, 0x22, 0xF2, 0, 0xDD, 300) catch unreachable;

    const inv2 = cache.evaluateAndPropagate(cur_hashes[0..], 0xF2, empty_edges[0..], queue[0..]) catch unreachable;
    assertEq(inv2.fresh_hits, @as(u32, 2));
    assertEq(inv2.units_to_rebuild, @as(u32, 0));
});

test("Six-stage BuildPipelineSession state machine and formatted report", () => {
    let sess = BuildPipelineSession.init(BuildTargetKind.NativeExe, 0x1234);
    assert(sess.stage == BuildPipelineStage.Init);

    // Out-of-order transition rejected
    let bad_trans = false;
    sess.beginLink() catch |e| {
        bad_trans = (e == BuildPipelineError.InvalidStageTransition);
    };
    assert(bad_trans);

    sess.beginResolve() catch unreachable;
    sess.beginCacheCheck(3) catch unreachable;

    // Simulate 4 units, 3 cached hits, 1 rebuilt (75% hit rate)
    let backing: [4]BuildCacheUnit = [BuildCacheUnit.empty(); 4];
    let cache = IncrementalBuildCache.init(backing[0..]);
    * = cache.registerUnit("a.cp", BuildLoaderKind.Cp, 1, 9, 0, 1, 100, true) catch unreachable;
    * = cache.registerUnit("b.cp", BuildLoaderKind.Cp, 2, 9, 0, 2, 100, true) catch unreachable;
    * = cache.registerUnit("c.cp", BuildLoaderKind.Cp, 3, 9, 0, 3, 100, true) catch unreachable;
    * = cache.registerUnit("d.cp", BuildLoaderKind.Cp, 4, 9, 0, 4, 100, true) catch unreachable;
    const hashes: [4]u64 = [1, 2, 3, 99];
    const no_edges: [0]BuildDepEdge = [];
    let q: [4]u32 = [0; 4];
    const inv = cache.evaluateAndPropagate(hashes[0..], 9, no_edges[0..], q[0..]) catch unreachable;

    sess.beginCompile(inv) catch unreachable;
    sess.beginLink() catch unreachable;
    sess.beginEmit(65536) catch unreachable;
    sess.markDone(42) catch unreachable;
    assert(sess.isDone());

    let buf: [256]u8 = [0; 256];
    const n = formatBuildPipelineReport(sess, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n], "build target=native-exe units=4 rebuilt=1 cached=3 hit=75% out=65536B time=42ms"));
});

test("Boundary guards: invalid unit ID and cache slice capacity exceeded", () => {
    let backing: [1]BuildCacheUnit = [BuildCacheUnit.empty(); 1];
    let cache = IncrementalBuildCache.init(backing[0..]);
    * = cache.registerUnit("only.cp", BuildLoaderKind.Cp, 1, 1, 0, 1, 10, true) catch unreachable;

    let cap_err = false;
    * = cache.registerUnit("overflow.cp", BuildLoaderKind.Cp, 2, 1, 0, 2, 10, true) catch |e| {
        cap_err = (e == BuildPipelineError.SliceCapacityExceeded);
    };
    assert(cap_err);

    let id_err = false;
    cache.commitCompiledUnit(99, 1, 1, 0, 1, 10) catch |e| {
        id_err = (e == BuildPipelineError.InvalidUnitId);
    };
    assert(id_err);
});

import { test, assert, assertEq } from "cap:test";
import {
    BundleCoordinatorError,
    encodeVlqI32,
    decodeVlqI32,
    SourceMapSegment,
    SourceMapV3Input,
    encodeSourceMapV3Json,
    PluginFilterKind,
    PluginResolveHook,
    PluginLoadHook,
    BundlePluginRegistry,
    BundleCoordStage,
    BundleDiagSeverity,
    BundleDiagnostic,
    BundleEmittedChunk,
    BundleCoordOptions,
    BundleCoordinator,
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

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("encodeVlqI32 and decodeVlqI32 match official SourceMap V3 Base64-VLQ vectors and round-trip", () => {
    let buf: [16]u8 = [0; 16];

    const check_vec = [
        0, 1, -1, 15, -15, 16, -16, 1000, -1000, 123456, -123456,
    ];
    let i: usize = 0;
    while (i < check_vec.len) {
        let pos: usize = 0;
        const v: i32 = check_vec[i];
        encodeVlqI32(v, buf[0..], &pos) catch unreachable;
        const decoded = decodeVlqI32(buf[0..pos]) catch unreachable;
        assertEq(decoded.value, v);
        assertEq(decoded.consumed, pos);
        i += 1;
    }

    // Verify exact canonical encodings
    let pos: usize = 0;
    encodeVlqI32(0, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "A"));

    pos = 0;
    encodeVlqI32(1, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "C"));

    pos = 0;
    encodeVlqI32(-1, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "D"));

    pos = 0;
    encodeVlqI32(16, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "gB"));

    pos = 0;
    encodeVlqI32(-16, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "hB"));

    pos = 0;
    encodeVlqI32(1000, buf[0..], &pos) catch unreachable;
    assert(strEql(buf[0..pos], "w+B"));
});

test("decodeVlqI32 rejects invalid Base64 digits, truncated continuations, and empty input", () => {
    let empty_err = false;
    * = decodeVlqI32("") catch |e| {
        empty_err = (e == BundleCoordinatorError.TruncatedVlq);
    };
    assert(empty_err);

    // 'g' has continuation bit 0x20 set (index 32) with no following digit
    let trunc_err = false;
    * = decodeVlqI32("g") catch |e| {
        trunc_err = (e == BundleCoordinatorError.TruncatedVlq);
    };
    assert(trunc_err);

    let bad_err = false;
    * = decodeVlqI32("!@#") catch |e| {
        bad_err = (e == BundleCoordinatorError.InvalidVlqDigit);
    };
    assert(bad_err);

    // 8 continuation bytes -> overflow
    let ovf_err = false;
    * = decodeVlqI32("ggggggggA") catch |e| {
        ovf_err = (e == BundleCoordinatorError.VlqOverflow);
    };
    assert(ovf_err);
});

test("encodeSourceMapV3Json emits valid RFC 8259 SourceMap V3 JSON with multi-line relative VLQ mappings", () => {
    const sources: [2][]const u8 = ["src/app.cp", "src/util.cp"];
    const contents: [2][]const u8 = ["const x = 1;\n", "pub fn add(a: i32, b: i32): i32 { return a + b; }\n"];
    const names: [1][]const u8 = ["add"];
    const segs: [3]SourceMapSegment = [
        // Line 0, col 0 -> src 0, line 0, col 0 (AAAA)
        SourceMapSegment{ gen_line: 0, gen_col: 0, src_idx: 0, src_line: 0, src_col: 0 },
        // Line 0, col 6 -> src 0, line 0, col 6 (MAAA -> delta gen_col=+6='M', src_idx=0='A', src_line=0='A', src_col=+6='M')
        SourceMapSegment{ gen_line: 0, gen_col: 6, src_idx: 0, src_line: 0, src_col: 6 },
        // Line 2, col 4 -> src 1, line 1, col 0, name 0 (;;IACA -> skip 2 lines ';;', gen_col=+4='I', src_idx=+1='C', src_line=+1='C', src_col=-6='N', name=0='A')
        SourceMapSegment{ gen_line: 2, gen_col: 4, src_idx: 1, src_line: 1, src_col: 0, name_idx: 0 },
    ];

    let out: [1024]u8 = [0; 1024];
    const n = encodeSourceMapV3Json(SourceMapV3Input{
        file: "dist/app.js",
        source_root: "/",
        sources: sources[0..],
        sources_content: contents[0..],
        names: names[0..],
        segments: segs[0..],
    }, out[0..]) catch unreachable;

    const json = out[0..n];
    assert(strContains(json, "\"version\":3"));
    assert(strContains(json, "\"file\":\"dist/app.js\""));
    assert(strContains(json, "\"sourceRoot\":\"/\""));
    assert(strContains(json, "\"sources\":[\"src/app.cp\",\"src/util.cp\"]"));
    assert(strContains(json, "\"names\":[\"add\"]"));
    assert(strContains(json, "\"mappings\":\"AAAA,MAAM;;ICCNA\""));
});

test("BundlePluginRegistry executes priority-ordered onResolve and onLoad virtual module hooks", () => {
    let rhooks: [4]PluginResolveHook = [PluginResolveHook.empty(); 4];
    let lhooks: [4]PluginLoadHook = [PluginLoadHook.empty(); 4];
    let reg = BundlePluginRegistry.init(rhooks[0..], lhooks[0..], 8);

    // Register lower priority first, then higher priority -> higher priority must win!
    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "fallback-virtual",
        filter_kind: PluginFilterKind.Prefix,
        pattern: "virtual:",
        priority: 1,
        target_namespace: "virtual",
    }) catch unreachable;

    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "env-virtual",
        filter_kind: PluginFilterKind.Exact,
        pattern: "virtual:env",
        priority: 100,
        target_namespace: "env-ns",
        resolved_override: "env-config",
    }) catch unreachable;

    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "cdn-external",
        filter_kind: PluginFilterKind.Prefix,
        pattern: "https://",
        priority: 50,
        mark_external: true,
    }) catch unreachable;

    reg.registerOnLoad(PluginLoadHook{
        plugin_name: "env-loader",
        namespace_filter: "env-ns",
        filter_kind: PluginFilterKind.Exact,
        pattern: "env-config",
        priority: 10,
        contents: "pub const MODE = \"prod\";",
        loader_name: "cp",
    }) catch unreachable;

    const r1 = reg.runOnResolve("virtual:env", "file") catch unreachable;
    assert(r1.matched);
    assert(strEql(r1.plugin_name, "env-virtual"));
    assert(strEql(r1.resolved_path, "env-config"));
    assert(strEql(r1.namespace, "env-ns"));

    const l1 = reg.runOnLoad(r1.resolved_path, r1.namespace);
    assert(l1.matched);
    assert(strEql(l1.plugin_name, "env-loader"));
    assert(strEql(l1.contents, "pub const MODE = \"prod\";"));

    const r2 = reg.runOnResolve("https://cdn.example.com/lib.js", "file") catch unreachable;
    assert(r2.matched && r2.is_external);
});

test("BundlePluginRegistry chains redirects and blocks infinite resolution cycles via max_resolve_depth", () => {
    let rhooks: [4]PluginResolveHook = [PluginResolveHook.empty(); 4];
    let lhooks: [2]PluginLoadHook = [PluginLoadHook.empty(); 2];
    let reg = BundlePluginRegistry.init(rhooks[0..], lhooks[0..], 4);

    // Cycle: "cycle:a" -> "cycle:b" -> "cycle:a"
    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "cycle-a",
        filter_kind: PluginFilterKind.Exact,
        pattern: "cycle:a",
        redirect_specifier: "cycle:b",
        target_namespace: "file",
    }) catch unreachable;
    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "cycle-b",
        filter_kind: PluginFilterKind.Exact,
        pattern: "cycle:b",
        redirect_specifier: "cycle:a",
        target_namespace: "file",
    }) catch unreachable;

    // Finite chain: "alias:old" -> "alias:new" (resolved)
    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "alias-hop",
        filter_kind: PluginFilterKind.Exact,
        pattern: "alias:old",
        redirect_specifier: "alias:new",
        target_namespace: "file",
    }) catch unreachable;
    reg.registerOnResolve(PluginResolveHook{
        plugin_name: "alias-final",
        filter_kind: PluginFilterKind.Exact,
        pattern: "alias:new",
        resolved_override: "src/new.cp",
        target_namespace: "file",
    }) catch unreachable;

    const ok = reg.runOnResolve("alias:old", "file") catch unreachable;
    assert(ok.matched);
    assert(strEql(ok.resolved_path, "src/new.cp"));
    assertEq(ok.hops, @as(u32, 2));

    let depth_err = false;
    * = reg.runOnResolve("cycle:a", "file") catch |e| {
        depth_err = (e == BundleCoordinatorError.PluginResolveDepthExceeded);
    };
    assert(depth_err);
});

test("BundleCoordinator orchestrates 6 stages monotonically and emits deterministic Metafile JSON", () => {
    let diags: [8]BundleDiagnostic = [BundleDiagnostic.empty(); 8];
    let chunks: [4]BundleEmittedChunk = [BundleEmittedChunk.empty(); 4];
    let coord = BundleCoordinator.init(
        BundleCoordOptions{ max_chunk_bytes: 100000, emit_sourcemaps: true },
        diags[0..],
        chunks[0..],
    );

    coord.advanceStage(BundleCoordStage.ResolveEntries, 0) catch unreachable;
    coord.recordEntriesResolved(2);

    // Reject backwards stage jump
    let bad_stage = false;
    coord.advanceStage(BundleCoordStage.Idle, 1) catch |e| {
        bad_stage = (e == BundleCoordinatorError.InvalidStageTransition);
    };
    assert(bad_stage);

    coord.advanceStage(BundleCoordStage.ScanGraph, 5) catch unreachable;
    coord.recordGraphScanned(18);

    coord.advanceStage(BundleCoordStage.RunPlugins, 12) catch unreachable;
    coord.recordPluginHit();

    coord.advanceStage(BundleCoordStage.GenerateChunks, 8) catch unreachable;
    coord.recordChunkEmitted(BundleEmittedChunk{
        output_path: "dist/app.js",
        entry_point: "src/app.cp",
        byte_size: 42000,
        input_count: 14,
        has_sourcemap: true,
        content_hash64: 0x0123456789ABCDEF,
    }) catch unreachable;

    coord.advanceStage(BundleCoordStage.EmitSourceMaps, 25) catch unreachable;
    coord.recordSourceMapEmitted();

    coord.advanceStage(BundleCoordStage.WriteManifest, 6) catch unreachable;
    const sum = coord.finish(4);

    assert(sum.succeeded);
    assert(sum.stage == BundleCoordStage.Completed);
    assertEq(sum.entry_count, @as(u32, 2));
    assertEq(sum.scanned_module_count, @as(u32, 18));
    assertEq(sum.plugin_hits, @as(u32, 1));
    assertEq(sum.chunk_count, @as(u32, 1));
    assertEq(sum.sourcemap_count, @as(u32, 1));
    assertEq(sum.total_output_bytes, @as(u64, 42000));
    assertEq(sum.total_elapsed_ms, @as(u64, 5 + 12 + 8 + 25 + 6 + 4));

    let mbuf: [1024]u8 = [0; 1024];
    const mlen = coord.formatMetafileJson(mbuf[0..]) catch unreachable;
    const meta = mbuf[0..mlen];
    assert(strContains(meta, "\"stage\":\"completed\""));
    assert(strContains(meta, "\"dist/app.js\":{\"bytes\":42000,\"inputs\":14,\"hash\":\"0123456789abcdef\",\"budgetExceeded\":false"));
});

test("BundleCoordinator enforces max_chunk_bytes budget warnings and strict fail_on_budget_exceeded mode", () => {
    // 1. Soft budget warning mode
    let diags1: [4]BundleDiagnostic = [BundleDiagnostic.empty(); 4];
    let chunks1: [4]BundleEmittedChunk = [BundleEmittedChunk.empty(); 4];
    let c1 = BundleCoordinator.init(
        BundleCoordOptions{ max_chunk_bytes: 10000, fail_on_budget_exceeded: false },
        diags1[0..],
        chunks1[0..],
    );
    c1.advanceStage(BundleCoordStage.GenerateChunks, 0) catch unreachable;
    c1.recordChunkEmitted(BundleEmittedChunk{ output_path: "dist/huge.js", byte_size: 25000 }) catch unreachable;
    const s1 = c1.finish(10);
    assert(s1.succeeded);
    assertEq(s1.budget_warning_count, @as(u32, 1));
    assertEq(s1.warning_count, @as(u32, 1));
    assert(chunks1[0].budget_exceeded);
    assert(diags1[0].severity == BundleDiagSeverity.Warning);
    assert(strEql(diags1[0].code, "W_BUDGET_EXCEEDED"));

    // 2. Strict fail_on_budget_exceeded upgrades to error and fails build
    let diags2: [4]BundleDiagnostic = [BundleDiagnostic.empty(); 4];
    let chunks2: [4]BundleEmittedChunk = [BundleEmittedChunk.empty(); 4];
    let c2 = BundleCoordinator.init(
        BundleCoordOptions{ max_chunk_bytes: 10000, fail_on_budget_exceeded: true },
        diags2[0..],
        chunks2[0..],
    );
    c2.advanceStage(BundleCoordStage.GenerateChunks, 0) catch unreachable;
    c2.recordChunkEmitted(BundleEmittedChunk{ output_path: "dist/huge.js", byte_size: 25000 }) catch unreachable;
    const s2 = c2.finish(10);
    assert(!s2.succeeded);
    assert(s2.stage == BundleCoordStage.Failed);
    assertEq(s2.error_count, @as(u32, 1));
    assert(diags2[0].severity == BundleDiagSeverity.Error);
});

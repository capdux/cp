import { test, assert, assertEq } from "cap:test";
import {
    CHUNK_GRAPH_INVALID_ID,
    ChunkGraphError,
    BundleEdgeKind,
    SplitChunkRole,
    BundleModuleNode,
    BundleImportEdge,
    ChunkSplitOptions,
    ChunkGraphNode,
    ChunkDepEdge,
    CompileWaveEntry,
    computeLiveModules,
    formatChunkFilename,
    computeCodeSplitChunks,
    planParallelCompileWaves,
    formatChunkGraphReport,
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j += 1;
        }
        if (ok) return true;
        i += 1;
    }
    return false;
}

test("Tree-Shaking liveness prunes unreachable and pure unused modules while preserving side effects", () => {
    const mods: [5]BundleModuleNode = [
        BundleModuleNode{ id: 0, path: "src/entry.cp", byte_size: 1000, has_side_effects: true, is_entry: true, content_hash64: 0x10 },
        BundleModuleNode{ id: 1, path: "src/used.cp", byte_size: 500, has_side_effects: false, is_entry: false, content_hash64: 0x20 },
        BundleModuleNode{ id: 2, path: "src/pure_unused.cp", byte_size: 800, has_side_effects: false, is_entry: false, content_hash64: 0x30 },
        BundleModuleNode{ id: 3, path: "src/polyfill.cp", byte_size: 300, has_side_effects: true, is_entry: false, content_hash64: 0x40 },
        BundleModuleNode{ id: 4, path: "src/orphan.cp", byte_size: 1200, has_side_effects: true, is_entry: false, content_hash64: 0x50 },
    ];
    const edges: [3]BundleImportEdge = [
        BundleImportEdge{ from_module: 0, to_module: 1, kind: BundleEdgeKind.StaticImport, is_used_export: true },
        BundleImportEdge{ from_module: 0, to_module: 2, kind: BundleEdgeKind.StaticImport, is_used_export: false },
        BundleImportEdge{ from_module: 0, to_module: 3, kind: BundleEdgeKind.SideEffectOnly, is_used_export: false },
    ];

    let live: [5]bool = [false; 5];
    let queue: [5]u32 = [0; 5];
    const sum = computeLiveModules(mods[0..], edges[0..], live[0..], queue[0..]) catch unreachable;

    assertEq(sum.total_modules, @as(u32, 5));
    assertEq(sum.live_modules, @as(u32, 3));
    assertEq(sum.pruned_modules, @as(u32, 2));
    assertEq(sum.live_bytes, @as(usize, 1800));
    assertEq(sum.saved_bytes, @as(usize, 2000));
    assert(live[0] && live[1] && !live[2] && live[3] && !live[4]);
});

test("Multi-entry + dynamic import() Code-Splitting produces Entry, Shared Common, and Dynamic Async chunks", () => {
    const mods: [5]BundleModuleNode = [
        BundleModuleNode{ id: 0, path: "src/pageA.cp", byte_size: 400, is_entry: true, content_hash64: 0xAA11 },
        BundleModuleNode{ id: 1, path: "src/pageB.cp", byte_size: 450, is_entry: true, content_hash64: 0xBB22 },
        BundleModuleNode{ id: 2, path: "src/shared.cp", byte_size: 600, is_entry: false, content_hash64: 0xCC33 },
        BundleModuleNode{ id: 3, path: "src/lazyModal.cp", byte_size: 700, is_entry: false, content_hash64: 0xDD44 },
        BundleModuleNode{ id: 4, path: "src/lazyHelper.cp", byte_size: 200, is_entry: false, content_hash64: 0xEE55 },
    ];
    const edges: [4]BundleImportEdge = [
        BundleImportEdge{ from_module: 0, to_module: 2, kind: BundleEdgeKind.StaticImport },
        BundleImportEdge{ from_module: 1, to_module: 2, kind: BundleEdgeKind.StaticImport },
        BundleImportEdge{ from_module: 0, to_module: 3, kind: BundleEdgeKind.DynamicImport },
        BundleImportEdge{ from_module: 3, to_module: 4, kind: BundleEdgeKind.StaticImport },
    ];

    let live: [5]bool = [false; 5];
    let queue: [5]u32 = [0; 5];
    * = computeLiveModules(mods[0..], edges[0..], live[0..], queue[0..]) catch unreachable;

    let bitset: [16]u64 = [0; 16];
    let mod_to_chunk: [5]u32 = [0; 5];
    let chunks: [8]ChunkGraphNode = [ChunkGraphNode.empty(); 8];
    let cedges: [16]ChunkDepEdge = [ChunkDepEdge.empty(); 16];
    let names: [512]u8 = [0; 512];
    let apos: usize = 0;

    const split = computeCodeSplitChunks(
        mods[0..],
        edges[0..],
        live[0..],
        ChunkSplitOptions.defaults(),
        bitset[0..],
        queue[0..],
        mod_to_chunk[0..],
        chunks[0..],
        cedges[0..],
        names[0..],
        &apos,
    ) catch unreachable;

    assertEq(split.chunks_len, @as(usize, 4));
    assertEq(split.entry_chunks, @as(u32, 2));
    assertEq(split.shared_chunks, @as(u32, 1));
    assertEq(split.async_chunks, @as(u32, 1));

    // lazyModal (3) and lazyHelper (4) share the same async chunk
    assertEq(mod_to_chunk[3], mod_to_chunk[4]);
    const async_cid: usize = @intCast(mod_to_chunk[3]);
    assert(chunks[async_cid].role == SplitChunkRole.DynamicAsyncChunk);
    assertEq(chunks[async_cid].module_count, @as(u32, 2));
    assertEq(chunks[async_cid].total_bytes, @as(usize, 900));

    // shared.cp (2) is in SharedCommonChunk referenced by 2 roots
    const shared_cid: usize = @intCast(mod_to_chunk[2]);
    assert(chunks[shared_cid].role == SplitChunkRole.SharedCommonChunk);
    assertEq(chunks[shared_cid].entry_root_count, @as(u32, 2));

    // Cross-chunk edges include 2 static imports to shared + 1 dynamic import to lazy
    assertEq(split.chunk_edges_len, @as(usize, 3));
    let dyn_edges: u32 = 0;
    let i: usize = 0;
    while (i < split.chunk_edges_len) {
        if (cedges[i].is_dynamic) dyn_edges += 1;
        i += 1;
    }
    assertEq(dyn_edges, @as(u32, 1));
});

test("Arbitrary entry-count external bitset supports >64 entries without truncation", () => {
    // 70 static entries (0..69) all statically importing 1 shared module (70)
    let mods: [71]BundleModuleNode = [BundleModuleNode{}; 71];
    let edges: [70]BundleImportEdge = [BundleImportEdge{}; 70];
    let i: usize = 0;
    while (i < 70) {
        mods[i] = BundleModuleNode{
            id: @intCast(i),
            path: "src/e.cp",
            byte_size: 100,
            is_entry: true,
            content_hash64: @as(u64, i + 1),
        };
        edges[i] = BundleImportEdge{
            from_module: @intCast(i),
            to_module: 70,
            kind: BundleEdgeKind.StaticImport,
        };
        i += 1;
    }
    mods[70] = BundleModuleNode{ id: 70, path: "src/core.cp", byte_size: 500, is_entry: false, content_hash64: 0x9999 };

    let live: [71]bool = [false; 71];
    let queue: [71]u32 = [0; 71];
    * = computeLiveModules(mods[0..], edges[0..], live[0..], queue[0..]) catch unreachable;

    // 70 roots -> words_per_module = 2 -> 71 * 2 = 142 u64 words
    let bitset: [256]u64 = [0; 256];
    let mod_to_chunk: [71]u32 = [0; 71];
    let chunks: [80]ChunkGraphNode = [ChunkGraphNode.empty(); 80];
    let cedges: [80]ChunkDepEdge = [ChunkDepEdge.empty(); 80];
    let names: [4096]u8 = [0; 4096];
    let apos: usize = 0;

    const split = computeCodeSplitChunks(
        mods[0..],
        edges[0..],
        live[0..],
        ChunkSplitOptions.defaults(),
        bitset[0..],
        queue[0..],
        mod_to_chunk[0..],
        chunks[0..],
        cedges[0..],
        names[0..],
        &apos,
    ) catch unreachable;

    assertEq(split.words_per_module, @as(usize, 2));
    assertEq(split.entry_chunks, @as(u32, 70));
    assertEq(split.shared_chunks, @as(u32, 1));
    const core_cid: usize = @intCast(mod_to_chunk[70]);
    assertEq(chunks[core_cid].entry_root_count, @as(u32, 70));
});

test("Deterministic content-hashed chunk filenames for .js, .wasm, and .nest targets", () => {
    let buf: [128]u8 = [0; 128];
    const n1 = formatChunkFilename("src/app.cp", SplitChunkRole.EntryChunk, 0x123456789abcdef0, ".js", true, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "app-12345678.js"));

    const n2 = formatChunkFilename("src/shared.cp", SplitChunkRole.SharedCommonChunk, 0xdeadbeef00000000, ".wasm", true, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "chunk-deadbeef.wasm"));

    const n3 = formatChunkFilename("src/worker.cp", SplitChunkRole.DynamicAsyncChunk, 0xabcdef0123456789, ".nest", false, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n3], "worker.nest"));
});

test("Parallel topological compile wave scheduling on diamond DAG", () => {
    // 0 (entry) -> 1, 2; 1 -> 3; 2 -> 3  (compile order: wave0=[3], wave1=[1,2], wave2=[0])
    const mods: [4]BundleModuleNode = [
        BundleModuleNode{ id: 0, path: "a.cp", is_entry: true },
        BundleModuleNode{ id: 1, path: "b.cp" },
        BundleModuleNode{ id: 2, path: "c.cp" },
        BundleModuleNode{ id: 3, path: "d.cp" },
    ];
    const edges: [4]BundleImportEdge = [
        BundleImportEdge{ from_module: 0, to_module: 1 },
        BundleImportEdge{ from_module: 0, to_module: 2 },
        BundleImportEdge{ from_module: 1, to_module: 3 },
        BundleImportEdge{ from_module: 2, to_module: 3 },
    ];
    const live: [4]bool = [true, true, true, true];
    let indeg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];
    let waves: [4]CompileWaveEntry = [CompileWaveEntry.empty(); 4];

    const plan = planParallelCompileWaves(mods[0..], edges[0..], live[0..], indeg[0..], queue[0..], waves[0..]) catch unreachable;
    assertEq(plan.scheduled_count, @as(usize, 4));
    assertEq(plan.wave_count, @as(u32, 3));
    assertEq(plan.max_wave_width, @as(u32, 2));
    assertEq(plan.cycles.cycle_count, @as(u32, 0));
    assertEq(waves[0].module_id, @as(u32, 3));
    assertEq(waves[0].wave_index, @as(u32, 0));
    assertEq(waves[3].module_id, @as(u32, 0));
    assertEq(waves[3].wave_index, @as(u32, 2));
});

test("Cycle detection isolates cyclic modules into final convergence wave without hanging", () => {
    // 0 (entry) -> 1 -> 2 -> 1 (cycle between 1 and 2, blocking 0 as well); 3 is leaf
    const mods: [4]BundleModuleNode = [
        BundleModuleNode{ id: 0, path: "entry.cp", is_entry: true },
        BundleModuleNode{ id: 1, path: "cycA.cp" },
        BundleModuleNode{ id: 2, path: "cycB.cp" },
        BundleModuleNode{ id: 3, path: "leaf.cp" },
    ];
    const edges: [4]BundleImportEdge = [
        BundleImportEdge{ from_module: 0, to_module: 1 },
        BundleImportEdge{ from_module: 1, to_module: 2 },
        BundleImportEdge{ from_module: 2, to_module: 1 },
        BundleImportEdge{ from_module: 1, to_module: 3 },
    ];
    const live: [4]bool = [true, true, true, true];
    let indeg: [4]u32 = [0; 4];
    let queue: [4]u32 = [0; 4];
    let waves: [4]CompileWaveEntry = [CompileWaveEntry.empty(); 4];

    const plan = planParallelCompileWaves(mods[0..], edges[0..], live[0..], indeg[0..], queue[0..], waves[0..]) catch unreachable;
    assertEq(plan.scheduled_count, @as(usize, 4));
    assertEq(plan.cycles.cycle_count, @as(u32, 1));
    assertEq(plan.cycles.modules_in_cycles, @as(u32, 3)); // 0, 1, 2
    assert(plan.cycles.first_cycle_module != CHUNK_GRAPH_INVALID_ID);
    assertEq(waves[0].module_id, @as(u32, 3));
    assert(!waves[0].in_cycle);
    assert(waves[1].in_cycle);
});

test("Metafile JSON report rendering and graph validation error guards", () => {
    const mods: [2]BundleModuleNode = [
        BundleModuleNode{ id: 0, path: "src/main.cp", byte_size: 1024, is_entry: true, content_hash64: 0x1111 },
        BundleModuleNode{ id: 1, path: "src/dead.cp", byte_size: 256, has_side_effects: false, is_entry: false, content_hash64: 0x2222 },
    ];
    const empty_edges: [0]BundleImportEdge = [];
    let live: [2]bool = [false; 2];
    let queue: [2]u32 = [0; 2];
    const lsum = computeLiveModules(mods[0..], empty_edges[0..], live[0..], queue[0..]) catch unreachable;

    let bitset: [4]u64 = [0; 4];
    let mod_to_chunk: [2]u32 = [0; 2];
    let chunks: [4]ChunkGraphNode = [ChunkGraphNode.empty(); 4];
    let cedges: [4]ChunkDepEdge = [ChunkDepEdge.empty(); 4];
    let names: [128]u8 = [0; 128];
    let apos: usize = 0;
    const split = computeCodeSplitChunks(mods[0..], empty_edges[0..], live[0..], ChunkSplitOptions.defaults(), bitset[0..], queue[0..], mod_to_chunk[0..], chunks[0..], cedges[0..], names[0..], &apos) catch unreachable;

    let indeg: [2]u32 = [0; 2];
    let waves: [2]CompileWaveEntry = [CompileWaveEntry.empty(); 2];
    const wplan = planParallelCompileWaves(mods[0..], empty_edges[0..], live[0..], indeg[0..], queue[0..], waves[0..]) catch unreachable;

    let report_buf: [1024]u8 = [0; 1024];
    const rlen = formatChunkGraphReport(lsum, split, chunks[0..split.chunks_len], cedges[0..split.chunk_edges_len], wplan, report_buf[0..]) catch unreachable;
    const json = report_buf[0..rlen];
    assert(containsSubstr(json, "\"total\":2,\"live\":1,\"pruned\":1,\"savedBytes\":256"));
    assert(containsSubstr(json, "\"role\":\"entry\",\"modules\":1,\"bytes\":1024"));

    // Empty graph error guard
    const no_mods: [0]BundleModuleNode = [];
    let empty_err = false;
    * = computeLiveModules(no_mods[0..], empty_edges[0..], live[0..], queue[0..]) catch |e| {
        empty_err = (e == ChunkGraphError.EmptyModuleGraph);
    };
    assert(empty_err);
});

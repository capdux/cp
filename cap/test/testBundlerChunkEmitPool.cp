import { test, assert, assertEq } from "cap:test";
import {
    CHUNK_EMIT_INVALID_ID,
    ChunkEmitPoolError,
    computeChunkContentHash64,
    ChunkPlaceholderSpan,
    scanChunkPlaceholdersFast,
    substituteChunkPlaceholders,
    ChunkEmitKind,
    ChunkEmitState,
    ChunkEmitJob,
    ChunkEmitDispatchItem,
    ChunkEmitPool,
    ChunkHashInput,
    ResolvedChunkOutput,
    ChunkHashResolveOptions,
    formatChunkHashedFilename,
    getResolvedFilename,
    getResolvedCode,
    resolveChunkContentHashes,
    ManifestChunkEntry,
    formatAssetManifestJson,
    formatChunkEmitPoolReport,
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

fn findIndexOf(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return i;
        i += 1;
    }
    return null;
}

test("ChunkEmitPool dispatches jobs in LPT descending size order with max_workers throttling", () => {
    let jobs: [6]ChunkEmitJob = [ChunkEmitJob.empty(); 6];
    let pool = ChunkEmitPool.init(jobs[0..], 2, true);

    const t0 = pool.enqueue(ChunkEmitJob{ chunk_id: 10, base_name: "small", estimated_bytes: 1024 }) catch unreachable;
    const t1 = pool.enqueue(ChunkEmitJob{ chunk_id: 20, base_name: "vendor", estimated_bytes: 65536 }) catch unreachable;
    const t2 = pool.enqueue(ChunkEmitJob{ chunk_id: 30, base_name: "app", estimated_bytes: 16384 }) catch unreachable;
    const t3 = pool.enqueue(ChunkEmitJob{ chunk_id: 40, base_name: "styles", kind: ChunkEmitKind.Css, extension: ".css", estimated_bytes: 16384, priority_boost: 5 }) catch unreachable;
    assertEq(t0, @as(usize, 0));
    assertEq(t1, @as(usize, 1));
    assertEq(t2, @as(usize, 2));
    assertEq(t3, @as(usize, 3));

    // Duplicate chunk_id must be rejected
    let dup_err = false;
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 20, base_name: "dup", estimated_bytes: 100 }) catch {
        dup_err = true;
    };
    assert(dup_err);

    // First wave: max_workers=2 -> vendor (65536), then styles (16384, boost=5)
    let batch: [4]ChunkEmitDispatchItem = [ChunkEmitDispatchItem.empty(); 4];
    const n1 = pool.dispatchNext(batch[0..]);
    assertEq(n1, @as(usize, 2));
    assertEq(batch[0].chunk_id, @as(u32, 20));
    assertEq(batch[1].chunk_id, @as(u32, 40));
    assert(batch[0].worker_slot != batch[1].worker_slot);

    // Pool is saturated: additional dispatch returns 0
    assertEq(pool.dispatchNext(batch[0..]), @as(usize, 0));

    // Invalid state transition: cannot markWritten before markHashed
    let bad_transition = false;
    pool.markWritten(batch[0].task_index, 60000) catch {
        bad_transition = true;
    };
    assert(bad_transition);

    // Complete vendor: Emitting -> Hashed -> Written
    pool.markHashed(batch[0].task_index, 0x1122334455667788) catch unreachable;
    assert(jobs[batch[0].task_index].state == ChunkEmitState.Hashed);
    pool.markWritten(batch[0].task_index, 64000) catch unreachable;
    assert(jobs[batch[0].task_index].state == ChunkEmitState.Written);
    assertEq(jobs[batch[0].task_index].worker_slot, CHUNK_EMIT_INVALID_ID);

    // Next wave frees 1 slot -> picks app (16384)
    const n2 = pool.dispatchNext(batch[0..]);
    assertEq(n2, @as(usize, 1));
    assertEq(batch[0].chunk_id, @as(u32, 30));

    // Finish styles, app, and small
    pool.markHashed(t3, 0xaaaa) catch unreachable;
    pool.markWritten(t3, 15000) catch unreachable;
    pool.markHashed(t2, 0xbbbb) catch unreachable;
    pool.markWritten(t2, 16000) catch unreachable;

    const n3 = pool.dispatchNext(batch[0..]);
    assertEq(n3, @as(usize, 1));
    assertEq(batch[0].chunk_id, @as(u32, 10));
    pool.markHashed(t0, 0xcccc) catch unreachable;
    pool.markWritten(t0, 900) catch unreachable;

    assert(pool.isDrained());
    const st = pool.stats();
    assertEq(st.written, @as(usize, 4));
    assertEq(st.failed, @as(usize, 0));
    assertEq(st.peak_active_workers, @as(u32, 2));
    assertEq(st.critical_path_bytes, @as(u64, 65536));
    assertEq(st.total_written_bytes, @as(usize, 95900));
});

test("ChunkEmitPool fail-fast cancels pending jobs immediately on failure", () => {
    let jobs: [4]ChunkEmitJob = [ChunkEmitJob.empty(); 4];
    let pool = ChunkEmitPool.init(jobs[0..], 2, true);

    * = pool.enqueue(ChunkEmitJob{ chunk_id: 1, base_name: "a", estimated_bytes: 5000 }) catch unreachable;
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 2, base_name: "b", estimated_bytes: 4000 }) catch unreachable;
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 3, base_name: "c", estimated_bytes: 3000 }) catch unreachable;
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 4, base_name: "d", estimated_bytes: 2000 }) catch unreachable;

    let batch: [2]ChunkEmitDispatchItem = [ChunkEmitDispatchItem.empty(); 2];
    assertEq(pool.dispatchNext(batch[0..]), @as(usize, 2));

    // First active job fails -> remaining 2 pending jobs must immediately become Cancelled
    pool.markFailed(batch[0].task_index, 503) catch unreachable;
    const st1 = pool.stats();
    assert(st1.aborted);
    assertEq(st1.failed, @as(usize, 1));
    assertEq(st1.cancelled, @as(usize, 2));
    assertEq(st1.emitting, @as(usize, 1));
    assertEq(pool.dispatchNext(batch[0..]), @as(usize, 0));

    // In-flight second job can still finish cleanly
    pool.markHashed(batch[1].task_index, 0xdeadbeef) catch unreachable;
    pool.markWritten(batch[1].task_index, 3800) catch unreachable;
    assert(pool.isDrained());
    assertEq(pool.stats().written, @as(usize, 1));
});

test("SIMD scanChunkPlaceholdersFast and substituteChunkPlaceholders handle valid, unknown, and malformed markers", () => {
    const src =
        "import a from \"[chunk_placeholder:1]\";\n" ++
        "const broken1 = \"[chunk_placeholder:]\";\n" ++
        "const broken2 = \"[chunk_placeholder:999999999999999]\";\n" ++
        "const broken3 = \"[chunk_placeholder:12abc]\";\n" ++
        "import b from \"[chunk_placeholder:2]\";\n" ++
        "import u from \"[chunk_placeholder:99]\";\n" ++
        "const tail = \"[chunk_place";

    let spans: [8]ChunkPlaceholderSpan = [ChunkPlaceholderSpan.empty(); 8];
    const scan = scanChunkPlaceholdersFast(src, spans[0..]) catch unreachable;

    assertEq(scan.valid_count, @as(usize, 3));
    assertEq(scan.malformed_count, @as(usize, 4));
    assert(scan.simd_jumps >= 3);
    assertEq(spans[0].target_chunk_id, @as(u32, 1));
    assertEq(spans[1].target_chunk_id, @as(u32, 2));
    assertEq(spans[2].target_chunk_id, @as(u32, 99));

    const ids: [2]u32 = [1, 2];
    const names: [2][]const u8 = ["assets/vendor-11223344.js", "assets/shared-aabbccdd.js"];
    let out: [512]u8 = [0; 512];

    // Non-strict mode preserves unknown target [chunk_placeholder:99]
    const rw = substituteChunkPlaceholders(src, ids[0..], names[0..], false, out[0..]) catch unreachable;
    assertEq(rw.replaced_count, @as(usize, 2));
    assertEq(rw.unresolved_count, @as(usize, 1));
    assertEq(rw.malformed_count, @as(usize, 4));
    const rendered = out[0..rw.bytes_written];
    assert(strContains(rendered, "assets/vendor-11223344.js"));
    assert(strContains(rendered, "assets/shared-aabbccdd.js"));
    assert(strContains(rendered, "[chunk_placeholder:99]"));

    // Strict mode rejects unknown target [chunk_placeholder:99]
    let strict_failed = false;
    * = substituteChunkPlaceholders(src, ids[0..], names[0..], true, out[0..]) catch {
        strict_failed = true;
    };
    assert(strict_failed);
});

test("resolveChunkContentHashes converges multi-hop DAG placeholders to deterministic content-addressed names", () => {
    // 3-level DAG: entry(0) -> shared(1) -> vendor(2)
    const inputs: [3]ChunkHashInput = [
        ChunkHashInput{
            chunk_id: 0,
            base_name: "entry",
            extension: ".js",
            public_prefix: "assets/",
            raw_source: "import { s } from \"[chunk_placeholder:1]\"; console.log(s);",
        },
        ChunkHashInput{
            chunk_id: 1,
            base_name: "shared",
            extension: ".js",
            public_prefix: "assets/",
            raw_source: "import { v } from \"[chunk_placeholder:2]\"; export const s = v + 1;",
        },
        ChunkHashInput{
            chunk_id: 2,
            base_name: "vendor",
            extension: ".js",
            public_prefix: "assets/",
            raw_source: "export const v = 41;",
        },
    ];

    let outs: [3]ResolvedChunkOutput = [ResolvedChunkOutput.empty(); 3];
    let ids: [3]u32 = [0; 3];
    let names: [3][]const u8 = [""; 3];
    let fn_arena: [512]u8 = [0; 512];
    let code_arena: [1024]u8 = [0; 1024];

    const sum = resolveChunkContentHashes(
        inputs[0..],
        ChunkHashResolveOptions{ strict_placeholders: true },
        outs[0..],
        ids[0..],
        names[0..],
        fn_arena[0..],
        code_arena[0..],
    ) catch unreachable;

    assert(sum.converged);
    assert(sum.rounds_executed >= 2 && sum.rounds_executed <= 5);
    assertEq(sum.total_placeholders_replaced, @as(usize, 2));

    const entry_name = getResolvedFilename(outs[0], fn_arena[0..]);
    const shared_name = getResolvedFilename(outs[1], fn_arena[0..]);
    const vendor_name = getResolvedFilename(outs[2], fn_arena[0..]);

    const entry_code = getResolvedCode(outs[0], code_arena[0..]);
    const shared_code = getResolvedCode(outs[1], code_arena[0..]);
    const vendor_code = getResolvedCode(outs[2], code_arena[0..]);

    // Final code must embed the exact final hashed filename of its dependency
    assert(strContains(entry_code, shared_name));
    assert(strContains(shared_code, vendor_name));
    assert(!strContains(entry_code, "[chunk_placeholder:"));
    assert(!strContains(shared_code, "[chunk_placeholder:"));

    // Recomputing hash of final code must match `outs[i].content_hash`
    assertEq(computeChunkContentHash64(entry_code), outs[0].content_hash);
    assertEq(computeChunkContentHash64(shared_code), outs[1].content_hash);
    assertEq(computeChunkContentHash64(vendor_code), outs[2].content_hash);
    assert(strContains(entry_name, "assets/entry-"));
    assert(strContains(vendor_name, "assets/vendor-"));
});

test("resolveChunkContentHashes detects duplicate output paths and non-converging cyclic hashes", () => {
    // 1. Duplicate output paths when include_hash_in_name=false
    const dup_inputs: [2]ChunkHashInput = [
        ChunkHashInput{ chunk_id: 1, base_name: "bundle", extension: ".js", raw_source: "const a = 1;", include_hash_in_name: false },
        ChunkHashInput{ chunk_id: 2, base_name: "bundle", extension: ".js", raw_source: "const b = 2;", include_hash_in_name: false },
    ];
    let outs: [2]ResolvedChunkOutput = [ResolvedChunkOutput.empty(); 2];
    let ids: [2]u32 = [0; 2];
    let names: [2][]const u8 = [""; 2];
    let fn_arena: [256]u8 = [0; 256];
    let code_arena: [256]u8 = [0; 256];

    let dup_detected = false;
    * = resolveChunkContentHashes(
        dup_inputs[0..],
        ChunkHashResolveOptions{},
        outs[0..],
        ids[0..],
        names[0..],
        fn_arena[0..],
        code_arena[0..],
    ) catch {
        dup_detected = true;
    };
    assert(dup_detected);

    // 2. Mutual cyclic reference A <-> B causes hashes to oscillate -> HashNotConverged
    const cyc_inputs: [2]ChunkHashInput = [
        ChunkHashInput{ chunk_id: 1, base_name: "a", extension: ".js", raw_source: "import \"[chunk_placeholder:2]\";" },
        ChunkHashInput{ chunk_id: 2, base_name: "b", extension: ".js", raw_source: "import \"[chunk_placeholder:1]\";" },
    ];
    let non_converged = false;
    * = resolveChunkContentHashes(
        cyc_inputs[0..],
        ChunkHashResolveOptions{ max_rounds: 4, require_convergence: true },
        outs[0..],
        ids[0..],
        names[0..],
        fn_arena[0..],
        code_arena[0..],
    ) catch {
        non_converged = true;
    };
    assert(non_converged);
});

test("formatAssetManifestJson emits deterministic lexicographically sorted JSON with proper escaping", () => {
    const all_imports: [2][]const u8 = ["assets/vendor-a1b2c3d4.js", "assets/shared-deadbeef.js"];
    const all_css: [1][]const u8 = ["assets/app-12345678.css"];

    // Intentionally unsorted input order
    const entries: [3]ManifestChunkEntry = [
        ManifestChunkEntry{
            entry_key: "src/z-admin.ts",
            output_file: "assets/z-admin-cafebabe.js",
            is_entry: true,
            size_bytes: 2048,
            integrity_hash: 0x0123456789abcdef,
            imports_start: 0,
            imports_len: 1,
            css_start: 0,
            css_len: 0,
        },
        ManifestChunkEntry{
            entry_key: "src/app.ts",
            output_file: "assets/app-11112222.js",
            is_entry: true,
            size_bytes: 4096,
            integrity_hash: 0xfedcba9876543210,
            imports_start: 0,
            imports_len: 2,
            css_start: 0,
            css_len: 1,
        },
        ManifestChunkEntry{
            entry_key: "src/chunk\"quoted\".ts",
            output_file: "assets/quoted-33334444.js",
            is_entry: false,
            size_bytes: 512,
            integrity_hash: 0x00000000000000ff,
            imports_start: 0,
            imports_len: 0,
            css_start: 0,
            css_len: 0,
        },
    ];

    let scratch: [3]usize = [0; 3];
    let json_buf: [2048]u8 = [0; 2048];
    const written = formatAssetManifestJson(
        entries[0..],
        all_imports[0..],
        all_css[0..],
        scratch[0..],
        json_buf[0..],
    ) catch unreachable;

    const json = json_buf[0..written];
    const idx_app = findIndexOf(json, "\"src/app.ts\"") orelse unreachable;
    const idx_quoted = findIndexOf(json, "\"src/chunk\\\"quoted\\\".ts\"") orelse unreachable;
    const idx_z = findIndexOf(json, "\"src/z-admin.ts\"") orelse unreachable;

    // Strictly lexicographical order: app < chunk"quoted" < z-admin
    assert(idx_app < idx_quoted);
    assert(idx_quoted < idx_z);
    assert(strContains(json, "\"integrity\": \"h64-fedcba9876543210\""));
    assert(strContains(json, "\"integrity\": \"h64-00000000000000ff\""));
    assert(strContains(json, "\"css\": [\"assets/app-12345678.css\"]"));
    assert(strContains(json, "\"imports\": [\"assets/vendor-a1b2c3d4.js\", \"assets/shared-deadbeef.js\"]"));
});

test("deterministic 500-iteration fuzz over malformed and truncated chunk placeholders causes 0 panics", () => {
    let seed: u64 = 0x9e3779b97f4a7c15;
    let buf: [160]u8 = [0; 160];
    let out_buf: [512]u8 = [0; 512];
    let spans: [32]ChunkPlaceholderSpan = [ChunkPlaceholderSpan.empty(); 32];
    const ids: [3]u32 = [0, 1, 42];
    const names: [3][]const u8 = ["c0.js", "c1.js", "c42.js"];

    const fragments: [10][]const u8 = [
        "[chunk_placeholder:0]",
        "[chunk_placeholder:42]",
        "[chunk_placeholder:]",
        "[chunk_placeholder:99999999999999999]",
        "[chunk_placeholder:12",
        "[chunk_place",
        "[[chunk_placeholder:1]]",
        "[chunk_placeholder:-5]",
        "const x = [1, 2, 3];",
        "[chunk_placeholder:777]",
    ];

    let iter: u32 = 0;
    let total_valid: usize = 0;
    let total_malformed: usize = 0;
    while (iter < 500) {
        // Xorshift64* PRNG
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        const r = seed *% 2685821657736338717;

        // Build deterministic adversarial payload of varying length (0..150 bytes)
        let len: usize = 0;
        const pieces: usize = @as(usize, @intCast(r % 6)) + 1;
        let p: usize = 0;
        let local_r = r;
        while (p < pieces && len < 120) {
            local_r = local_r *% 6364136223846793005 +% 1442695040888963407;
            const frag = fragments[@as(usize, @intCast((local_r >> 16) % 10))];
            const cut_max = frag.len;
            const take = if ((local_r & 3) == 0 && cut_max > 1)
                @as(usize, @intCast((local_r >> 8) % cut_max)) + 1
            else
                cut_max;
            let j: usize = 0;
            while (j < take && len < buf.len) {
                buf[len] = frag[j];
                len += 1;
                j += 1;
            }
            p += 1;
        }

        const payload = buf[0..len];
        const scan = scanChunkPlaceholdersFast(payload, spans[0..]) catch unreachable;
        const rw = substituteChunkPlaceholders(payload, ids[0..], names[0..], false, out_buf[0..]) catch unreachable;

        // Invariant: scanner and substituter must agree on valid count == replaced + unresolved
        assertEq(scan.valid_count, rw.replaced_count + rw.unresolved_count);
        assertEq(scan.malformed_count, rw.malformed_count);
        total_valid += scan.valid_count;
        total_malformed += scan.malformed_count;
        iter += 1;
    }

    assert(total_valid > 100);
    assert(total_malformed > 100);
});

test("end-to-end ChunkEmitPool -> resolveChunkContentHashes -> formatAssetManifestJson -> report", () => {
    const raw_inputs: [2]ChunkHashInput = [
        ChunkHashInput{
            chunk_id: 1,
            base_name: "main",
            extension: ".js",
            public_prefix: "dist/",
            raw_source: "import \"[chunk_placeholder:2]\"; export const ok = 1;",
        },
        ChunkHashInput{
            chunk_id: 2,
            base_name: "runtime",
            extension: ".js",
            public_prefix: "dist/",
            raw_source: "export const rt = true;",
        },
    ];

    let outs: [2]ResolvedChunkOutput = [ResolvedChunkOutput.empty(); 2];
    let ids: [2]u32 = [0; 2];
    let names: [2][]const u8 = [""; 2];
    let fn_arena: [256]u8 = [0; 256];
    let code_arena: [512]u8 = [0; 512];

    const rsum = resolveChunkContentHashes(
        raw_inputs[0..],
        ChunkHashResolveOptions{},
        outs[0..],
        ids[0..],
        names[0..],
        fn_arena[0..],
        code_arena[0..],
    ) catch unreachable;
    assert(rsum.converged);

    let jobs: [2]ChunkEmitJob = [ChunkEmitJob.empty(); 2];
    let pool = ChunkEmitPool.init(jobs[0..], 2, true);
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 1, base_name: "main", entry_key: "src/main.cp", is_entry: true, estimated_bytes: 512 }) catch unreachable;
    * = pool.enqueue(ChunkEmitJob{ chunk_id: 2, base_name: "runtime", estimated_bytes: 256 }) catch unreachable;

    let batch: [2]ChunkEmitDispatchItem = [ChunkEmitDispatchItem.empty(); 2];
    assertEq(pool.dispatchNext(batch[0..]), @as(usize, 2));
    pool.markHashed(0, outs[0].content_hash) catch unreachable;
    pool.markWritten(0, outs[0].code_len) catch unreachable;
    pool.markHashed(1, outs[1].content_hash) catch unreachable;
    pool.markWritten(1, outs[1].code_len) catch unreachable;

    let report_buf: [512]u8 = [0; 512];
    const rlen = formatChunkEmitPoolReport(pool.stats(), report_buf[0..]) catch unreachable;
    const report = report_buf[0..rlen];
    assert(strContains(report, "ChunkEmitPool Summary"));
    assert(strContains(report, "written=2"));
    assert(strContains(report, "aborted=false"));

    const imps: [1][]const u8 = [getResolvedFilename(outs[1], fn_arena[0..])];
    const no_css: [0][]const u8 = [];
    const manifest_entries: [1]ManifestChunkEntry = [
        ManifestChunkEntry{
            entry_key: "src/main.cp",
            output_file: getResolvedFilename(outs[0], fn_arena[0..]),
            is_entry: true,
            size_bytes: outs[0].code_len,
            integrity_hash: outs[0].content_hash,
            imports_start: 0,
            imports_len: 1,
            css_start: 0,
            css_len: 0,
        },
    ];
    let scratch: [1]usize = [0; 1];
    let json_buf: [1024]u8 = [0; 1024];
    const mlen = formatAssetManifestJson(manifest_entries[0..], imps[0..], no_css[0..], scratch[0..], json_buf[0..]) catch unreachable;
    assert(strContains(json_buf[0..mlen], getResolvedFilename(outs[0], fn_arena[0..])));
    assert(strContains(json_buf[0..mlen], getResolvedFilename(outs[1], fn_arena[0..])));
});

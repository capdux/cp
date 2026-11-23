import { test, assert, assertEq } from "cap:test";
import {
    PmRouterError,
    PmSubcommandKind,
    parsePmSubcommandArgs,
    formatPmCommandHelp,
    PmLsNode,
    PmLsEdge,
    PmLsRenderOptions,
    renderPmLsTree,
    CacheEntryMeta,
    CachePrunePolicy,
    CachePruneDecisionKind,
    CachePruneDecision,
    planCachePrune,
    PmLockHashEntry,
    computePmLockfileHash,
    PmWhoamiAuthKind,
    diagnosePmWhoami,
} from "cap:pkg";

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

test("parsePmSubcommandArgs routes all cap pm subcommands including two-word cache rm and typed flags", () => {
    let pos: [8][]const u8 = [""; 8];

    const a1: [3][]const u8 = ["pm", "bin", "-g"];
    const c1 = parsePmSubcommandArgs(a1[0..], pos[0..]) catch unreachable;
    assert(c1.kind == PmSubcommandKind.Bin && c1.global);

    const a2: [3][]const u8 = ["pm", "ls", "--all"];
    const c2 = parsePmSubcommandArgs(a2[0..], pos[0..]) catch unreachable;
    assert(c2.kind == PmSubcommandKind.Ls && c2.all);

    const a3: [3][]const u8 = ["pm", "cache", "rm"];
    const c3 = parsePmSubcommandArgs(a3[0..], pos[0..]) catch unreachable;
    assert(c3.kind == PmSubcommandKind.CacheRm);

    const a4: [7][]const u8 = ["pack", "--dry-run", "--gzip-level=6", "--destination", "dist", "--filename", "pkg.tgz"];
    const c4 = parsePmSubcommandArgs(a4[0..], pos[0..]) catch unreachable;
    assert(c4.kind == PmSubcommandKind.Pack && c4.dry_run);
    assertEq(c4.gzip_level, @as(u8, 6));
    assert(strEql(c4.destination, "dist") && strEql(c4.filename, "pkg.tgz"));

    const a5: [4][]const u8 = ["pm", "trust", "esbuild", "sharp"];
    const c5 = parsePmSubcommandArgs(a5[0..], pos[0..]) catch unreachable;
    assert(c5.kind == PmSubcommandKind.Trust);
    assertEq(c5.positionals.len, @as(usize, 2));
    assert(strEql(c5.positionals[0], "esbuild"));

    const a6: [3][]const u8 = ["why", "zod", "--top"];
    const c6 = parsePmSubcommandArgs(a6[0..], pos[0..]) catch unreachable;
    assert(c6.kind == PmSubcommandKind.Why && c6.top_only);
    assert(strEql(c6.positionals[0], "zod"));

    const a7: [3][]const u8 = ["audit", "--json", "--audit-level=high"];
    const c7 = parsePmSubcommandArgs(a7[0..], pos[0..]) catch unreachable;
    assert(c7.kind == PmSubcommandKind.Audit && c7.json_output && strEql(c7.audit_level, "high"));
});

test("parsePmSubcommandArgs rejects unknown subcommands, flags, and invalid gzip levels; formatPmCommandHelp renders usage", () => {
    let pos: [4][]const u8 = [""; 4];

    const bad_cmd: [1][]const u8 = ["nonexistent"];
    let unk_cmd = false;
    * = parsePmSubcommandArgs(bad_cmd[0..], pos[0..]) catch |e| {
        unk_cmd = (e == PmRouterError.UnknownSubcommand);
    };
    assert(unk_cmd);

    const bad_gz: [2][]const u8 = ["pack", "--gzip-level=10"];
    let gz_err = false;
    * = parsePmSubcommandArgs(bad_gz[0..], pos[0..]) catch |e| {
        gz_err = (e == PmRouterError.InvalidGzipLevel);
    };
    assert(gz_err);

    let hbuf: [512]u8 = [0; 512];
    const h1 = formatPmCommandHelp(PmSubcommandKind.Help, hbuf[0..]) catch unreachable;
    assert(strContains(hbuf[0..h1], "Usage: cap pm <command> [flags]"));

    const h2 = formatPmCommandHelp(PmSubcommandKind.CacheRm, hbuf[0..]) catch unreachable;
    assert(strContains(hbuf[0..h2], "Usage: cap pm cache rm [flags]"));
});

test("renderPmLsTree renders direct view vs --all nested Unicode/ASCII tree with disk sizes", () => {
    const nodes: [3]PmLsNode = [
        PmLsNode{ id: 0, name: "zod", version: "3.23.8", byte_size: 20480, is_direct: true },
        PmLsNode{ id: 1, name: "axios", version: "1.7.2", byte_size: 40960, is_direct: true },
        PmLsNode{ id: 2, name: "follow-redirects", version: "1.15.6", byte_size: 10240, is_direct: false },
    ];
    const edges: [1]PmLsEdge = [
        PmLsEdge{ parent_idx: 1, child_idx: 2 },
    ];
    let visited: [3]bool = [false; 3];
    let out: [512]u8 = [0; 512];

    // Direct-only view
    const r1 = renderPmLsTree("my-app@1.0.0", nodes[0..], edges[0..], PmLsRenderOptions.defaults(), visited[0..], out[0..]) catch unreachable;
    assertEq(r1.nodes_rendered, @as(u32, 2));
    assert(strContains(out[0..r1.bytes_written], "├── zod@3.23.8 [20.0 KB]\n"));
    assert(strContains(out[0..r1.bytes_written], "└── axios@1.7.2 [40.0 KB]\n"));
    assert(!strContains(out[0..r1.bytes_written], "follow-redirects"));

    // --all full nested ASCII view
    const r2 = renderPmLsTree(
        "my-app@1.0.0",
        nodes[0..],
        edges[0..],
        PmLsRenderOptions{ show_all: true, use_unicode: false, show_sizes: true },
        visited[0..],
        out[0..],
    ) catch unreachable;
    assertEq(r2.nodes_rendered, @as(u32, 3));
    assertEq(r2.total_disk_bytes, @as(u64, 71680));
    assert(strContains(out[0..r2.bytes_written], "    `-- follow-redirects@1.15.6 [10.0 KB]\n"));
});

test("renderPmLsTree annotates circular references and deduped subtrees without infinite recursion", () => {
    // Direct: pkg-a (0), pkg-c (2); Edges: 0->1, 1->0 (circular!), 2->1 (deduped!)
    const nodes: [3]PmLsNode = [
        PmLsNode{ id: 0, name: "pkg-a", version: "1.0.0", is_direct: true },
        PmLsNode{ id: 1, name: "pkg-b", version: "2.0.0", is_direct: false },
        PmLsNode{ id: 2, name: "pkg-c", version: "3.0.0", is_direct: true },
    ];
    const edges: [3]PmLsEdge = [
        PmLsEdge{ parent_idx: 0, child_idx: 1 },
        PmLsEdge{ parent_idx: 1, child_idx: 0 },
        PmLsEdge{ parent_idx: 2, child_idx: 1 },
    ];
    let visited: [3]bool = [false; 3];
    let out: [512]u8 = [0; 512];

    const res = renderPmLsTree(
        "cycle-app",
        nodes[0..],
        edges[0..],
        PmLsRenderOptions{ show_all: true, use_unicode: true, show_sizes: false },
        visited[0..],
        out[0..],
    ) catch unreachable;
    assertEq(res.circular_refs, @as(u32, 1));
    assertEq(res.deduped_refs, @as(u32, 1));
    assert(strContains(out[0..res.bytes_written], "pkg-a@1.0.0 (circular)"));
    assert(strContains(out[0..res.bytes_written], "pkg-b@2.0.0 (deduped)"));
});

test("planCachePrune evicts TTL-expired and LRU over-quota entries while protecting lockfile-referenced packages", () => {
    const entries: [4]CacheEntryMeta = [
        CacheEntryMeta{ key: "active-locked", byte_size: 500, last_access_secs: 100, referenced_by_lockfile: true },
        CacheEntryMeta{ key: "expired-old", byte_size: 400, last_access_secs: 200, referenced_by_lockfile: false },
        CacheEntryMeta{ key: "lru-victim", byte_size: 400, last_access_secs: 900, referenced_by_lockfile: false },
        CacheEntryMeta{ key: "recent-keep", byte_size: 300, last_access_secs: 990, referenced_by_lockfile: false },
    ];
    let decs: [4]CachePruneDecision = [CachePruneDecision.empty(); 4];
    const sum = planCachePrune(
        entries[0..],
        CachePrunePolicy{ now_secs: 1000, max_bytes: 900, max_age_secs: 500, protect_referenced: true },
        decs[0..],
    ) catch unreachable;

    assertEq(sum.kept_count, @as(u32, 2));
    assertEq(sum.evicted_ttl_count, @as(u32, 1));
    assertEq(sum.evicted_lru_count, @as(u32, 1));
    assertEq(sum.bytes_kept, @as(u64, 800));
    assertEq(sum.bytes_reclaimed, @as(u64, 800));

    assert(decs[0].decision == CachePruneDecisionKind.KeepProtected);
    assert(decs[1].decision == CachePruneDecisionKind.EvictExpiredTtl);
    assert(decs[2].decision == CachePruneDecisionKind.EvictLruQuota);
    assert(decs[3].decision == CachePruneDecisionKind.Keep);
});

test("computePmLockfileHash produces deterministic 32-hex digests sensitive to package mutations", () => {
    const e1: [2]PmLockHashEntry = [
        PmLockHashEntry{ name: "zod", version: "3.23.8", resolved: "https://r/zod.tgz", integrity: "sha512-aaa" },
        PmLockHashEntry{ name: "axios", version: "1.7.2", resolved: "https://r/axios.tgz", integrity: "sha512-bbb" },
    ];
    const e2: [2]PmLockHashEntry = [
        PmLockHashEntry{ name: "zod", version: "3.23.9", resolved: "https://r/zod.tgz", integrity: "sha512-aaa" },
        PmLockHashEntry{ name: "axios", version: "1.7.2", resolved: "https://r/axios.tgz", integrity: "sha512-bbb" },
    ];

    let h1: [32]u8 = [0; 32];
    let h1_repeat: [32]u8 = [0; 32];
    let h2: [32]u8 = [0; 32];

    assertEq(computePmLockfileHash(e1[0..], h1[0..]) catch unreachable, @as(usize, 32));
    assertEq(computePmLockfileHash(e1[0..], h1_repeat[0..]) catch unreachable, @as(usize, 32));
    assertEq(computePmLockfileHash(e2[0..], h2[0..]) catch unreachable, @as(usize, 32));

    assert(strEql(h1[0..], h1_repeat[0..]));
    assert(!strEql(h1[0..], h2[0..]));
});

test("diagnosePmWhoami resolves bearer, env, basic, and unauthenticated credential states with token redaction", () => {
    let rbuf: [32]u8 = [0; 32];

    const d1 = diagnosePmWhoami("https://registry.cap.dev", "cap_live_secret_987654", "", "", rbuf[0..]);
    assert(d1.authenticated && d1.auth_kind == PmWhoamiAuthKind.BearerToken);
    assert(strEql(d1.redacted_token, "cap_***"));

    const d2 = diagnosePmWhoami("", "", "", "npm_env_token_123", rbuf[0..]);
    assert(d2.authenticated && d2.auth_kind == PmWhoamiAuthKind.EnvToken);
    assert(strEql(d2.registry_url, "https://registry.npmjs.org"));

    const d3 = diagnosePmWhoami("https://reg.internal", "", "alice", "", rbuf[0..]);
    assert(d3.authenticated && d3.auth_kind == PmWhoamiAuthKind.BasicAuth);
    assert(strEql(d3.username_hint, "alice"));

    const d4 = diagnosePmWhoami("", "", "", "", rbuf[0..]);
    assert(!d4.authenticated && d4.auth_kind == PmWhoamiAuthKind.None);
});

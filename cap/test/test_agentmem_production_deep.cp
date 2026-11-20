// Source: cp/cap/test/test_agentmem_production_deep.cp
// Deep Production Verification Suite for cap:agentmem & cap.agentmem
// Covers:
//   1. Three-tier memory writes, Working capacity LRU/FIFO eviction & InvalidImportance/InvalidCapacity
//   2. Three-factor recall scoring (Recency + Importance + Relevance), recallByTier & access refresh
//   3. consolidate() Working/Episodic -> Semantic promotion & prune() Episodic forgetting (Semantic immune)
//   4. exportJsonl() -> importJsonl() persistence roundtrip, escaping & recall consistency
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.agentmem`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AgentMemError,
    MemoryTier,
    RecallWeights,
    MemoryItem,
    RecallHit,
    AgentMemory,
} from "../../lib/agentmem/index.cp";

const EPS_F64: f64 = 0.0001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testTiersWorkingEvictionAndErrors()! {
    const alloc = std.heap.page_allocator;

    // InvalidCapacity when max_working_items == 0
    let got_invalid_cap = false;
    * = AgentMemory.init(alloc, 0) catch e => {
        if (e == AgentMemError.InvalidCapacity) got_invalid_cap = true;
    };
    expect_eq(got_invalid_cap, true);

    let mem = AgentMemory.init(alloc, 2)?;
    defer mem.deinit();

    // InvalidImportance interception (< 0.0 and > 1.0)
    const empty_tags: []const string = [];
    let got_neg_imp = false;
    * = mem.add(.Working, "bad neg", empty_tags, -0.1, 1000) catch e => {
        if (e == AgentMemError.InvalidImportance) got_neg_imp = true;
    };
    expect_eq(got_neg_imp, true);

    let got_high_imp = false;
    * = mem.add(.Episodic, "bad high", empty_tags, 1.05, 1000) catch e => {
        if (e == AgentMemError.InvalidImportance) got_high_imp = true;
    };
    expect_eq(got_high_imp, true);

    // Add 3 Working items with max_working_items = 2 -> oldest (w1) should be evicted
    const w1 = mem.add(.Working, "working step one", ["w1"], 0.4, 1000)?;
    const w2 = mem.add(.Working, "working step two", ["w2"], 0.5, 2000)?;
    const w3 = mem.add(.Working, "working step three", ["w3"], 0.6, 3000)?;

    expect_eq(mem.countByTier(.Working), 2);
    expect_eq(mem.get(w1) == null, true);
    expect_eq(mem.get(w2) != null, true);
    expect_eq(mem.get(w3) != null, true);

    // Episodic and Semantic are not capped by max_working_items
    const e1 = mem.add(.Episodic, "observed user prefers dark mode", ["ui", "pref"], 0.7, 3100)?;
    const s1 = mem.add(.Semantic, "CP uses postfix T? and T! types", ["cp", "syntax"], 0.95, 3200)?;
    expect_eq(mem.countByTier(.Episodic), 1);
    expect_eq(mem.countByTier(.Semantic), 1);
    expect_eq(mem.len(), 4);
    expect_eq(mem.get(e1) != null, true);
    expect_eq(mem.get(s1) != null, true);
}

fn testThreeFactorRecallAndAccessRefresh()! {
    const alloc = std.heap.page_allocator;
    let mem = AgentMemory.init(alloc, 8)?;
    defer mem.deinit();

    const weights = RecallWeights{
        recency_weight: 1.0,
        importance_weight: 1.0,
        relevance_weight: 2.0,
        decay_half_life_ms: 1000.0,
    };

    // id1: old, low importance, unrelated
    const id1 = mem.add(.Episodic, "had coffee in the morning", ["daily"], 0.2, 1000)?;
    // id2: stale, medium importance, partially relevant
    const id2 = mem.add(.Episodic, "compiler error in parser", ["compiler"], 0.5, 2000)?;
    // id3: recent, high importance, highly relevant (matches both content & tags)
    const id3 = mem.add(.Working, "zig compiler memory safety borrow checker", ["zig", "compiler", "memory"], 0.9, 5000)?;

    const now_ms: u64 = 5000;
    const hits = mem.recall(alloc, "zig compiler memory", now_ms, 2, weights)?;
    defer alloc.free(hits);

    expect_eq(hits.len, 2);
    expect_eq(hits[0].id, id3);
    expect_eq(hits[0].tier == .Working, true);
    expect_eq(approxF64(hits[0].recency_score, 1.0, EPS_F64), true);
    expect_eq(approxF64(hits[0].importance_score, 0.9, EPS_F64), true);
    expect_eq(hits[0].relevance_score > 0.7, true);
    expect_eq(hits[1].id, id2);
    expect_eq(hits[0].total_score > hits[1].total_score, true);

    // Verify recall() refreshed access_count and last_accessed_ms for returned hits only
    const item3 = mem.get(id3).?;
    expect_eq(item3.access_count, 1);
    expect_eq(item3.last_accessed_ms, now_ms);

    const item2 = mem.get(id2).?;
    expect_eq(item2.access_count, 1);
    expect_eq(item2.last_accessed_ms, now_ms);

    const item1 = mem.get(id1).?;
    expect_eq(item1.access_count, 0);
    expect_eq(item1.last_accessed_ms, 1000);

    // Verify recallByTier filters strictly to Episodic
    const ep_hits = mem.recallByTier(alloc, .Episodic, "zig compiler memory", 6000, 5, weights)?;
    defer alloc.free(ep_hits);
    expect_eq(ep_hits.len, 2);
    expect_eq(ep_hits[0].id, id2);
    expect_eq(ep_hits[0].tier == .Episodic, true);
    expect_eq(ep_hits[1].id, id1);
}

fn testConsolidateAndPruneLifecycle()! {
    const alloc = std.heap.page_allocator;
    let mem = AgentMemory.init(alloc, 8)?;
    defer mem.deinit();

    // w_hot: Working, high importance, will be recalled twice -> promoted to Semantic
    const w_hot = mem.add(.Working, "critical database connection pool limit is 64", ["db", "config"], 0.9, 1000)?;
    // e_hot: Episodic, high importance, will be recalled twice -> promoted to Semantic
    const e_hot = mem.add(.Episodic, "production deploy requires tls cert verification", ["deploy", "tls"], 0.85, 1000)?;
    // e_stale_low: Episodic, low importance, never recalled -> will be pruned
    const e_stale_low = mem.add(.Episodic, "temporary debug log line 42", ["debug"], 0.1, 1000)?;
    // e_fresh_mid: Episodic, recent, moderate importance -> survives prune
    const e_fresh_mid = mem.add(.Episodic, "recent user query about billing", ["billing"], 0.6, 10000000)?;

    // Recall twice at t = 1100 and t = 1200 to boost access_count of w_hot and e_hot
    const h1 = mem.recall(alloc, "database pool tls deploy", 1100, 2, RecallWeights{})?;
    alloc.free(h1);
    const h2 = mem.recall(alloc, "database pool tls deploy", 1200, 2, RecallWeights{})?;
    alloc.free(h2);

    expect_eq(mem.get(w_hot).?.access_count, 2);
    expect_eq(mem.get(e_hot).?.access_count, 2);

    // Consolidate items with access_count >= 2 and importance >= 0.8
    const promoted = mem.consolidate(2, 0.8);
    expect_eq(promoted, 2);
    expect_eq(mem.get(w_hot).?.tier == .Semantic, true);
    expect_eq(mem.get(e_hot).?.tier == .Semantic, true);
    expect_eq(mem.countByTier(.Semantic), 2);

    // Prune at t = 10000000 with threshold 0.5:
    // - e_stale_low: dt ~ 10,000,000ms (recency ~ 0.0) + importance 0.1 < 0.5 -> PRUNED
    // - e_fresh_mid: dt = 0 (recency = 1.0) + importance 0.6 >= 0.5 -> KEPT
    // - w_hot & e_hot: Semantic -> IMMUNE even at huge dt
    const pruned = mem.prune(10000000, 0.5);
    expect_eq(pruned, 1);
    expect_eq(mem.get(e_stale_low) == null, true);
    expect_eq(mem.get(e_fresh_mid) != null, true);
    expect_eq(mem.get(w_hot) != null, true);
    expect_eq(mem.get(e_hot) != null, true);
    expect_eq(mem.len(), 3);
}

fn testJsonlRoundtripAndInvalidJsonl()! {
    const alloc = std.heap.page_allocator;
    let mem = AgentMemory.init(alloc, 4)?;
    defer mem.deinit();

    const tags1: [2]string = ["quote\"tag", "line\ntag"];
    const id1 = mem.add(.Semantic, "Line1\nLine2 with \"quotes\" and \\slash\\", &tags1, 0.92, 1000)?;
    const id2 = mem.add(.Episodic, "Agent completed benchmark suite", ["bench", "perf"], 0.75, 2000)?;

    const r1 = mem.recall(alloc, "benchmark", 2500, 1, RecallWeights{})?;
    alloc.free(r1);

    const jsonl = mem.exportJsonl(alloc)?;
    defer alloc.free(jsonl);

    let restored = AgentMemory.importJsonl(alloc, jsonl)?;
    defer restored.deinit();

    expect_eq(restored.len(), 2);

    const rest1 = restored.get(id1).?;
    expect_eq(rest1.tier == .Semantic, true);
    expect_eq(std.mem.eql(u8, rest1.content, "Line1\nLine2 with \"quotes\" and \\slash\\"), true);
    expect_eq(rest1.tags.len, 2);
    expect_eq(std.mem.eql(u8, rest1.tags[0], "quote\"tag"), true);
    expect_eq(std.mem.eql(u8, rest1.tags[1], "line\ntag"), true);
    expect_eq(approxF64(rest1.importance, 0.92, EPS_F64), true);

    const rest2 = restored.get(id2).?;
    expect_eq(rest2.tier == .Episodic, true);
    expect_eq(rest2.access_count, 1);
    expect_eq(rest2.last_accessed_ms, 2500);

    // Recall consistency after roundtrip
    const orig_hits = mem.recall(alloc, "benchmark perf", 3000, 2, RecallWeights{})?;
    defer alloc.free(orig_hits);
    const rest_hits = restored.recall(alloc, "benchmark perf", 3000, 2, RecallWeights{})?;
    defer alloc.free(rest_hits);

    expect_eq(orig_hits.len, rest_hits.len);
    expect_eq(orig_hits[0].id, rest_hits[0].id);
    expect_eq(approxF64(orig_hits[0].total_score, rest_hits[0].total_score, EPS_F64), true);

    // Invalid JSONL interception
    let got_invalid_jsonl = false;
    * = AgentMemory.importJsonl(alloc, "{\"id\":\"not_a_number\"}\n") catch e => {
        if (e == AgentMemError.InvalidJsonl) got_invalid_jsonl = true;
    };
    expect_eq(got_invalid_jsonl, true);
}

fn testCapAgentMemDynamicMount()! {
    const alloc = std.heap.page_allocator;
    let mem = cap.agentmem.AgentMemory.init(alloc, 3)?;
    defer mem.deinit();

    const id1 = mem.add(cap.agentmem.MemoryTier.Working, "load user profile from cache", ["cache", "profile"], 0.7, 1000)?;
    const id2 = mem.add(cap.agentmem.MemoryTier.Semantic, "user timezone is UTC+8", ["timezone", "profile"], 0.95, 1000)?;
    * = id1;

    const hits = mem.recall(alloc, "timezone profile", 1500, 2, cap.agentmem.RecallWeights{})?;
    defer alloc.free(hits);

    expect_eq(hits.len, 2);
    expect_eq(hits[0].id, id2);
    expect_eq(hits[0].tier == cap.agentmem.MemoryTier.Semantic, true);
}

fn main() {
    test("1. Three-Tier Writes, Working Capacity Eviction & Error Guards", () => {
        testTiersWorkingEvictionAndErrors()?;
    });
    test("2. Three-Factor Recall Scoring, recallByTier & Access Refresh", () => {
        testThreeFactorRecallAndAccessRefresh()?;
    });
    test("3. Consolidate Promotion & Prune Forgetting Lifecycle", () => {
        testConsolidateAndPruneLifecycle()?;
    });
    test("4. JSONL Export/Import Roundtrip & Recall Consistency", () => {
        testJsonlRoundtripAndInvalidJsonl()?;
    });
    test("5. import cap from \"cap\"; cap.agentmem Dynamic Lazy Mounting", () => {
        testCapAgentMemDynamicMount()?;
    });
}

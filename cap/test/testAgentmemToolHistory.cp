// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/toolHistory.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Agent Tool-Output Compactor & Reference Store:
//   1. Record outputs, FNV-1a content hashing, token estimation, preview truncation & lookup
//   2. Budget-driven oldest-first compaction protecting `keep_recent_full` & `pinned` entries
//   3. Error-output protection toggle (`keep_errors_full = true` vs `false`) & `within_budget` reporting
//   4. Dynamic pinning lifecycle (`pinRef` / `unpinRef` / `removeRef`) & compacted-ref pin guard
//   5. Zero-alloc context rendering (`renderEntry` & `renderAll`) + insufficient-buffer guard
//   6. 64-entry capacity overflow eviction (compacted first -> oldest full -> all-pinned refusal)
//   7. Global telemetry stats (`savings_bps`, token accounting) & `cap.agentmem` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TOOL_HISTORY_MAX_ENTRIES,
    TOOL_HISTORY_NAME_MAX_LEN,
    TOOL_HISTORY_PREVIEW_MAX_LEN,
    TOOL_HISTORY_NO_REF,
    TOOL_HISTORY_DEFAULT_STUB_TOKENS,
    TOOL_HISTORY_BPS_SCALE,
    ToolOutputStatus,
    ToolHistoryConfig,
    ToolHistoryStore,
    hashToolPayload,
    estimateToolTokens,
    formatToolStatus,
} from "cap:agentmem";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (bytesEq(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

test("ToolHistory: record outputs, FNV-1a hashing, preview truncation & lookup") {
    let store = ToolHistoryStore.init();
    expect_eq(store.len(), @as(usize, 0));
    expect_eq(store.activeTokens(), @as(u64, 0));

    const payload1 = "file contents: hello world";
    const r1 = store.recordOutput("read_file", payload1, 120, false, false) ?? 0;
    expect_eq(r1, @as(u64, 1));

    // Oversized payload (> 256B) should set preview_truncated = true and preserve original_bytes
    let big: [300]u8 = ['A'; 300];
    const r2 = store.recordOutput("dump_logs", big[0..], 400, false, true) ?? 0;
    expect_eq(r2, @as(u64, 2));

    if (e1 := store.getEntry(r1)) {
        expect_eq(e1.ref_id, @as(u64, 1));
        expect_eq(e1.status, ToolOutputStatus.full);
        expect_eq(e1.original_bytes, @as(u32, @intCast(payload1.len)));
        expect_eq(e1.original_tokens, @as(u32, 120));
        expect_eq(e1.active_tokens, @as(u32, 120));
        expect_eq(e1.content_hash, hashToolPayload(payload1));
        expect_true(!e1.is_error);
        expect_true(!e1.preview_truncated);
        expect_true(bytesEq(e1.toolName(), "read_file"));
        expect_true(bytesEq(e1.preview(), payload1));
    } else {
        expect_true(false);
    }

    if (e2 := store.getEntry(r2)) {
        expect_eq(e2.status, ToolOutputStatus.pinned);
        expect_true(e2.isPinned());
        expect_eq(e2.original_bytes, @as(u32, 300));
        expect_true(e2.preview_truncated);
        expect_eq(e2.preview().len, TOOL_HISTORY_PREVIEW_MAX_LEN);
        expect_eq(e2.content_hash, hashToolPayload(big[0..]));
    } else {
        expect_true(false);
    }

    // Helper pure functions
    expect_eq(estimateToolTokens(""), @as(u32, 0));
    expect_eq(estimateToolTokens("abcd"), @as(u32, 1));
    expect_eq(estimateToolTokens("abcde"), @as(u32, 2));
    expect_true(bytesEq(formatToolStatus(ToolOutputStatus.full), "full"));
    expect_true(bytesEq(formatToolStatus(ToolOutputStatus.compacted), "compacted"));
    expect_true(bytesEq(formatToolStatus(ToolOutputStatus.pinned), "pinned"));

    // Missing ref returns null
    const missing = store.getEntry(999);
    expect_true(missing == null);
    const zero_ref = store.getEntry(TOOL_HISTORY_NO_REF);
    expect_true(zero_ref == null);
}

test("ToolHistory: budget-driven oldest-first compaction protects pinned & recent calls") {
    let cfg = ToolHistoryConfig.defaults();
    cfg.keep_recent_full = 2;
    cfg.keep_errors_full = true;
    cfg.stub_tokens = 10;
    let store = ToolHistoryStore.initWithConfig(cfg);

    // 5 calls: r1(full 500), r2(pinned 400), r3(full 300), r4(recent full 200), r5(recent full 200)
    // Total active = 1600
    const r1 = store.recordOutput("read_file", "alpha content", 500, false, false) ?? 0;
    const r2 = store.recordOutput("schema_doc", "critical spec", 400, false, true) ?? 0;
    const r3 = store.recordOutput("grep_code", "match lines", 300, false, false) ?? 0;
    const r4 = store.recordOutput("run_build", "build ok", 200, false, false) ?? 0;
    const r5 = store.recordOutput("run_tests", "all pass", 200, false, false) ?? 0;

    // Compact to budget 1200 -> only r1 needs compaction (freed = 490, after = 1110 <= 1200)
    const rep1 = store.compactToBudget(1200);
    expect_eq(rep1.compacted_count, @as(u32, 1));
    expect_eq(rep1.tokens_freed, @as(u64, 490));
    expect_eq(rep1.active_tokens_before, @as(u64, 1600));
    expect_eq(rep1.active_tokens_after, @as(u64, 1110));
    expect_true(rep1.within_budget);

    if (e1 := store.getEntry(r1)) {
        expect_true(e1.isCompacted());
        expect_eq(e1.active_tokens, @as(u32, 10));
        expect_eq(e1.savedTokens(), @as(u32, 490));
        expect_true(bytesEq(e1.preview(), "[ref#1: read_file, 13B, ok]"));
    } else {
        expect_true(false);
    }
    if (e3 := store.getEntry(r3)) {
        expect_eq(e3.status, ToolOutputStatus.full);
    } else {
        expect_true(false);
    }

    // Compact further to budget 500 -> r2 is pinned (protected), r3 gets compacted (freed = 290, after = 820),
    // r4 & r5 are protected by keep_recent_full = 2 -> cannot reach 500, within_budget = false
    const rep2 = store.compactToBudget(500);
    expect_eq(rep2.compacted_count, @as(u32, 1));
    expect_eq(rep2.tokens_freed, @as(u64, 290));
    expect_eq(rep2.active_tokens_after, @as(u64, 820));
    expect_true(!rep2.within_budget);

    if (e2 := store.getEntry(r2)) {
        expect_true(e2.isPinned());
        expect_eq(e2.active_tokens, @as(u32, 400));
    } else {
        expect_true(false);
    }
    if (e3 := store.getEntry(r3)) {
        expect_true(e3.isCompacted());
        expect_eq(e3.active_tokens, @as(u32, 10));
    } else {
        expect_true(false);
    }
    if (e4 := store.getEntry(r4)) {
        expect_eq(e4.status, ToolOutputStatus.full);
    } else {
        expect_true(false);
    }
    if (e5 := store.getEntry(r5)) {
        expect_eq(e5.status, ToolOutputStatus.full);
    } else {
        expect_true(false);
    }
}

test("ToolHistory: keep_errors_full protection toggle (protected vs compactable)") {
    // 1. keep_errors_full = true: error entry is skipped during compaction
    let cfg_keep = ToolHistoryConfig.defaults();
    cfg_keep.keep_recent_full = 1;
    cfg_keep.keep_errors_full = true;
    cfg_keep.stub_tokens = 12;

    let s1 = ToolHistoryStore.initWithConfig(cfg_keep);
    const err_ref = s1.recordOutput("compile", "error[E0105]: syntax", 300, true, false) ?? 0;
    const ok_ref = s1.recordOutput("read_file", "normal output", 300, false, false) ?? 0;
    * = s1.recordOutput("status", "latest", 100, false, false);

    const rep_keep = s1.compactToBudget(450);
    // err_ref is protected; ok_ref is compacted (300 -> 12, freed = 288, after = 412 <= 450)
    expect_eq(rep_keep.compacted_count, @as(u32, 1));
    expect_eq(rep_keep.tokens_freed, @as(u64, 288));
    expect_true(rep_keep.within_budget);
    if (ee := s1.getEntry(err_ref)) {
        expect_eq(ee.status, ToolOutputStatus.full);
        expect_true(ee.is_error);
    } else {
        expect_true(false);
    }
    if (eo := s1.getEntry(ok_ref)) {
        expect_true(eo.isCompacted());
    } else {
        expect_true(false);
    }

    // 2. keep_errors_full = false: error entry CAN be compacted and renders with `, err]`
    let cfg_drop = ToolHistoryConfig.defaults();
    cfg_drop.keep_recent_full = 1;
    cfg_drop.keep_errors_full = false;
    cfg_drop.stub_tokens = 12;

    let s2 = ToolHistoryStore.initWithConfig(cfg_drop);
    const err_ref2 = s2.recordOutput("compile", "error[E0105]: syntax", 300, true, false) ?? 0;
    * = s2.recordOutput("status", "latest", 100, false, false);

    const rep_drop = s2.compactToBudget(150);
    expect_eq(rep_drop.compacted_count, @as(u32, 1));
    expect_eq(rep_drop.tokens_freed, @as(u64, 288));
    expect_true(rep_drop.within_budget);
    if (ee2 := s2.getEntry(err_ref2)) {
        expect_true(ee2.isCompacted());
        expect_true(bytesEq(ee2.preview(), "[ref#1: compile, 20B, err]"));
    } else {
        expect_true(false);
    }
}

test("ToolHistory: dynamic pinRef / unpinRef / removeRef lifecycle & guards") {
    let cfg = ToolHistoryConfig.defaults();
    cfg.keep_recent_full = 0;
    cfg.stub_tokens = 10;
    let store = ToolHistoryStore.initWithConfig(cfg);

    const r1 = store.recordOutput("t1", "payload_one", 200, false, false) ?? 0;
    const r2 = store.recordOutput("t2", "payload_two", 200, false, false) ?? 0;

    // Pin r1 dynamically -> compaction skips r1 and compacts r2 instead
    expect_true(store.pinRef(r1));
    expect_true(!store.unpinRef(r2)); // r2 is not pinned
    const rep1 = store.compactToBudget(250);
    expect_eq(rep1.compacted_count, @as(u32, 1));
    if (e1 := store.getEntry(r1)) {
        expect_true(e1.isPinned());
    } else {
        expect_true(false);
    }
    if (e2 := store.getEntry(r2)) {
        expect_true(e2.isCompacted());
    } else {
        expect_true(false);
    }

    // Compacted ref cannot be pinned back
    expect_true(!store.pinRef(r2));

    // Unpin r1 -> now r1 becomes compactable
    expect_true(store.unpinRef(r1));
    const rep2 = store.compactToBudget(50);
    expect_eq(rep2.compacted_count, @as(u32, 1));
    expect_eq(rep2.active_tokens_after, @as(u64, 20));
    expect_true(rep2.within_budget);

    // Remove r1
    expect_true(store.removeRef(r1));
    expect_true(!store.removeRef(r1));
    expect_eq(store.len(), @as(usize, 1));
}

test("ToolHistory: zero-alloc renderEntry / renderAll & small-buffer guard") {
    let cfg = ToolHistoryConfig.defaults();
    cfg.keep_recent_full = 1;
    cfg.stub_tokens = 12;
    let store = ToolHistoryStore.initWithConfig(cfg);

    const r1 = store.recordOutput("read_file", "first file data", 400, false, false) ?? 0;
    const r2 = store.recordOutput("spec_doc", "pinned content", 200, false, true) ?? 0;
    const r3 = store.recordOutput("run_cmd", "exit 0", 100, false, false) ?? 0;

    * = store.compactToBudget(350); // compacts r1

    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];

    // 1. Compacted entry renders exact stub
    const n1 = store.renderEntry(r1, *buf_s) ?? 0;
    expect_true(n1 > 0);
    expect_true(bytesEq(buf[0..n1], "[ref#1: read_file, 15B, ok]"));

    // 2. Pinned entry renders with `, pinned]` + preview
    const n2 = store.renderEntry(r2, *buf_s) ?? 0;
    expect_true(n2 > 0);
    expect_true(bytesEq(buf[0..n2], "[ref#2: spec_doc, 14B, ok, pinned] pinned content"));

    // 3. Full entry renders with preview
    const n3 = store.renderEntry(r3, *buf_s) ?? 0;
    expect_true(n3 > 0);
    expect_true(bytesEq(buf[0..n3], "[ref#3: run_cmd, 6B, ok] exit 0"));

    // 4. renderAll renders all 3 in chronological order separated by newlines
    const n_all = store.renderAll(*buf_s) ?? 0;
    expect_true(n_all > 0);
    expect_true(containsSubstr(buf[0..n_all], "[ref#1: read_file, 15B, ok]\n"));
    expect_true(containsSubstr(buf[0..n_all], "[ref#2: spec_doc, 14B, ok, pinned] pinned content\n"));
    expect_true(containsSubstr(buf[0..n_all], "[ref#3: run_cmd, 6B, ok] exit 0"));

    // 5. Undersized buffer cleanly returns null
    let tiny: [8]u8 = [0; 8];
    let tiny_s: []u8 = tiny[0..];
    expect_true(store.renderEntry(r1, *tiny_s) == null);
    expect_true(store.renderAll(*tiny_s) == null);
}

test("ToolHistory: 64-entry capacity overflow eviction & all-pinned refusal") {
    let cfg = ToolHistoryConfig.defaults();
    cfg.keep_recent_full = 2;
    cfg.stub_tokens = 10;
    let store = ToolHistoryStore.initWithConfig(cfg);

    // Fill all 64 slots; pin ref#1, leave others full
    let i: usize = 0;
    while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
        const ref_id = store.recordOutput("tool", "data", 100, false, i == 0) ?? 0;
        expect_eq(ref_id, @as(u64, @intCast(i + 1)));
    }
    expect_eq(store.len(), TOOL_HISTORY_MAX_ENTRIES);

    // Compact ref#2 so it becomes the preferred eviction victim
    * = store.compactToBudget(6310);
    if (e2 := store.getEntry(2)) {
        expect_true(e2.isCompacted());
    } else {
        expect_true(false);
    }

    // 65th insert should evict compacted ref#2 first (keeping pinned ref#1 intact)
    const r65 = store.recordOutput("tool_new", "fresh", 50, false, false) ?? 0;
    expect_eq(r65, @as(u64, 65));
    expect_eq(store.len(), TOOL_HISTORY_MAX_ENTRIES);
    expect_true(store.getEntry(2) == null);
    expect_true(store.getEntry(1) != null);
    expect_eq(store.stats().evicted_total, @as(u64, 1));

    // All-pinned store refuses 65th insert without side effects
    let pinned_store = ToolHistoryStore.init();
    i = 0;
    while (i < TOOL_HISTORY_MAX_ENTRIES) : (i += 1) {
        expect_true(pinned_store.recordOutput("pin", "x", 10, false, true) != null);
    }
    expect_true(pinned_store.recordOutput("overflow", "y", 10, false, false) == null);
    expect_eq(pinned_store.len(), TOOL_HISTORY_MAX_ENTRIES);
    expect_eq(pinned_store.stats().evicted_total, @as(u64, 0));
}

test("ToolHistory: global telemetry stats (savings_bps) & cap.agentmem facade parity") {
    let cfg = cap.agentmem.ToolHistoryConfig.defaults();
    cfg.keep_recent_full = 1;
    cfg.stub_tokens = 20;

    let store = cap.agentmem.ToolHistoryStore.initWithConfig(cfg);
    * = store.recordOutput("read_a", "aaaa", 400, false, false);
    * = store.recordOutput("read_b", "bbbb", 400, false, false);
    * = store.recordOutput("read_c", "cccc", 200, false, true);

    // Total original = 1000. Compact to budget 300 -> r1 & r2 compacted (400->20 each), saved = 760
    const rep = store.compactToBudget(300);
    expect_eq(rep.compacted_count, @as(u32, 2));
    expect_eq(rep.tokens_freed, @as(u64, 760));
    expect_true(rep.within_budget);

    const st = store.stats();
    expect_eq(st.total_entries, @as(usize, 3));
    expect_eq(st.capacity, TOOL_HISTORY_MAX_ENTRIES);
    expect_eq(st.full_count, @as(usize, 0));
    expect_eq(st.compacted_count, @as(usize, 2));
    expect_eq(st.pinned_count, @as(usize, 1));
    expect_eq(st.active_tokens, @as(u64, 240));
    expect_eq(st.original_tokens_total, @as(u64, 1000));
    expect_eq(st.saved_tokens, @as(u64, 760));
    // 760 / 1000 = 76.00% = 7600 bps
    expect_eq(st.savings_bps, @as(u32, 7600));
    expect_eq(st.recorded_total, @as(u64, 3));
    expect_eq(st.evicted_total, @as(u64, 0));
    expect_eq(TOOL_HISTORY_DEFAULT_STUB_TOKENS, @as(u32, 12));
    expect_eq(TOOL_HISTORY_BPS_SCALE, @as(u32, 10_000));
    expect_eq(TOOL_HISTORY_NAME_MAX_LEN, @as(usize, 32));

    // Reset clears everything cleanly
    store.reset();
    const st_after = store.stats();
    expect_eq(st_after.total_entries, @as(usize, 0));
    expect_eq(st_after.active_tokens, @as(u64, 0));
    expect_eq(st_after.savings_bps, @as(u32, 0));
}

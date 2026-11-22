// Origin: Pure-CP Stdlib Test Suite (lib/mcp/taskAsync.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc MCP Long-Running Async Tool Task Manager:
//   1. Full 6-state lifecycle (queued -> running -> input_required -> running -> completed) & progress/poll updates
//   2. Irreversible terminal states & idempotent cancellation (`already_cancelled` without duplicate events)
//   3. Automatic TTL expiration reaping (`reapExpired`), immortal tasks (`ttl_ms == 0`) & slot reuse
//   4. Incremental cursor polling (`pollEvents`): `since_seq` pagination (`has_more`) & per-task filtering
//   5. Event ring wrap-around & `lagged` detection when consumer falls behind ring capacity (128)
//   6. Zero-alloc RFC 8259 JSON-RPC task status formatting (`formatTaskStatusJson`) with escaping & buffer guard
//   7. 64-task capacity/invalid-arg guards, global `stats()` snapshot & `cap.mcp` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MCP_TASK_MAX_TASKS,
    MCP_TASK_EVENT_RING_CAP,
    MCP_TASK_DEFAULT_POLL_MS,
    MCP_TASK_MAX_PROGRESS_BPS,
    McpAsyncTaskState,
    mcpTaskStateIsTerminal,
    mcpTaskStateName,
    McpAsyncTaskOpStatus,
    McpAsyncTaskEventKind,
    McpAsyncTask,
    McpAsyncTaskEvent,
    formatTaskStatusJson,
    McpAsyncTaskManager,
} from "cap:mcp";

fn strEq(a: []const u8, b: []const u8): bool {
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
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEq(haystack[i..(i + needle.len)], needle)) return true;
    }
    return false;
}

test("McpTaskAsync: full 6-state lifecycle, progress_bps & dynamic poll_interval_ms") {
    let mgr = McpAsyncTaskManager.init();

    expect_eq(mgr.submitTask(101, 0xCAFE, 0, 60_000, 1000), McpAsyncTaskOpStatus.ok);
    const t0 = mgr.getTask(101) ?? McpAsyncTask{};
    expect_eq(t0.state, McpAsyncTaskState.queued);
    expect_eq(t0.poll_interval_ms, MCP_TASK_DEFAULT_POLL_MS);
    expect_eq(t0.expires_at_ms, 61_000 as u64);

    // Cannot update progress or require input while queued
    expect_eq(mgr.updateProgress(101, 1000, 500, "premature", 1050), McpAsyncTaskOpStatus.invalid_transition);
    expect_eq(mgr.requireInput(101, "need key", 1050), McpAsyncTaskOpStatus.invalid_transition);
    expect_eq(mgr.completeTask(101, "premature", 1050), McpAsyncTaskOpStatus.invalid_transition);

    // queued -> running
    expect_eq(mgr.startTask(101, 1100), McpAsyncTaskOpStatus.ok);
    expect_eq(mgr.updateProgress(101, 3500, 250, "scanning files", 1500), McpAsyncTaskOpStatus.ok);
    const t1 = mgr.getTask(101) ?? McpAsyncTask{};
    expect_eq(t1.progress_bps, 3500 as u16);
    expect_eq(t1.poll_interval_ms, 250 as u32);
    expect_true(strEq(t1.summarySlice(), "scanning files"));

    // running -> input_required -> running
    expect_eq(mgr.requireInput(101, "confirm production deploy?", 2000), McpAsyncTaskOpStatus.ok);
    expect_eq((mgr.getTask(101) ?? McpAsyncTask{}).state, McpAsyncTaskState.input_required);
    expect_eq(mgr.resumeFromInput(101, "approved by admin", 2500), McpAsyncTaskOpStatus.ok);
    expect_eq((mgr.getTask(101) ?? McpAsyncTask{}).state, McpAsyncTaskState.running);

    // running -> completed (auto-sets progress_bps = 10000)
    expect_eq(mgr.completeTask(101, "deployed v2.4.0", 3000), McpAsyncTaskOpStatus.ok);
    const t_done = mgr.getTask(101) ?? McpAsyncTask{};
    expect_eq(t_done.state, McpAsyncTaskState.completed);
    expect_eq(t_done.progress_bps, MCP_TASK_MAX_PROGRESS_BPS);
    expect_true(mcpTaskStateIsTerminal(t_done.state));
    expect_true(strEq(mcpTaskStateName(t_done.state), "completed"));
    expect_true(strEq(t_done.summarySlice(), "deployed v2.4.0"));
}

test("McpTaskAsync: irreversible terminal states & idempotent cancellation") {
    let mgr = McpAsyncTaskManager.init();
    * = mgr.submitTask(1, 0x11, 500, 0, 1000);
    * = mgr.submitTask(2, 0x22, 500, 0, 1000);
    * = mgr.submitTask(3, 0x33, 500, 0, 1000);

    // Task 1: cancel while queued -> ok; second cancel -> idempotent already_cancelled!
    expect_eq(mgr.cancelTask(1, "user aborted", 1100), McpAsyncTaskOpStatus.ok);
    const seq_after_first_cancel = mgr.stats().latest_event_seq;
    expect_eq(mgr.cancelTask(1, "retry cancel", 1200), McpAsyncTaskOpStatus.already_cancelled);
    // Idempotent cancel must not emit duplicate events or bump total_cancelled
    expect_eq(mgr.stats().latest_event_seq, seq_after_first_cancel);
    expect_eq(mgr.stats().total_cancelled, 1 as u64);
    // Cancelled task cannot be started or failed
    expect_eq(mgr.startTask(1, 1300), McpAsyncTaskOpStatus.invalid_transition);
    expect_eq(mgr.failTask(1, -32000, "err", 1300), McpAsyncTaskOpStatus.invalid_transition);

    // Task 2: completed cannot be cancelled or failed
    * = mgr.startTask(2, 1100);
    expect_eq(mgr.completeTask(2, "ok", 1200), McpAsyncTaskOpStatus.ok);
    expect_eq(mgr.cancelTask(2, "too late", 1300), McpAsyncTaskOpStatus.invalid_transition);
    expect_eq(mgr.failTask(2, -1, "too late", 1300), McpAsyncTaskOpStatus.invalid_transition);

    // Task 3: failed cannot be cancelled or completed
    * = mgr.startTask(3, 1100);
    expect_eq(mgr.failTask(3, -32603, "upstream timeout", 1200), McpAsyncTaskOpStatus.ok);
    expect_eq((mgr.getTask(3) ?? McpAsyncTask{}).error_code, -32603 as i32);
    expect_eq(mgr.cancelTask(3, "too late", 1300), McpAsyncTaskOpStatus.invalid_transition);
    expect_eq(mgr.completeTask(3, "too late", 1300), McpAsyncTaskOpStatus.invalid_transition);
}

test("McpTaskAsync: TTL expiration reaping, immortal tasks & slot reuse") {
    let mgr = McpAsyncTaskManager.init();
    // Task 10: expires at 5000; Task 20: expires at 9000; Task 30: immortal (ttl == 0)
    * = mgr.submitTask(10, 0xA, 500, 4000, 1000);
    * = mgr.submitTask(20, 0xB, 500, 8000, 1000);
    * = mgr.submitTask(30, 0xC, 500, 0, 1000);

    expect_eq(mgr.reapExpired(4999), 0 as usize);
    expect_eq(mgr.reapExpired(5000), 1 as usize);
    expect_true(mgr.getTask(10) == null);
    expect_true(mgr.getTask(20) != null);
    expect_true(mgr.getTask(30) != null);
    expect_eq(mgr.stats().active_slots, 2 as usize);
    expect_eq(mgr.stats().total_reaped_expired, 1 as u64);

    // Reaped task_id can be cleanly resubmitted into reclaimed slot
    expect_eq(mgr.submitTask(10, 0xA2, 500, 1000, 5100), McpAsyncTaskOpStatus.ok);
    expect_eq(mgr.reapExpired(10_000), 2 as usize); // reaps 20 and new 10; keeps immortal 30
    expect_eq(mgr.stats().active_slots, 1 as usize);
    expect_true(mgr.getTask(30) != null);
}

test("McpTaskAsync: incremental cursor polling (since_seq), has_more pagination & task_filter") {
    let mgr = McpAsyncTaskManager.init();
    * = mgr.submitTask(10, 0x1, 500, 0, 1000);  // seq 1 (task 10)
    * = mgr.submitTask(20, 0x2, 500, 0, 1000);  // seq 2 (task 20)
    * = mgr.startTask(10, 1100);                // seq 3 (task 10)
    * = mgr.updateProgress(10, 5000, 0, "half", 1200); // seq 4 (task 10)
    * = mgr.startTask(20, 1300);                // seq 5 (task 20)
    * = mgr.completeTask(10, "done", 1400);     // seq 6 (task 10)

    // Page 1: fetch at most 2 global events since_seq = 0
    let page1_buf: [2]McpAsyncTaskEvent = [McpAsyncTaskEvent{}; 2];
    let page1_s: []McpAsyncTaskEvent = page1_buf[0..];
    const r1 = mgr.pollEvents(0, 0, page1_s);
    expect_eq(r1.count, 2 as usize);
    expect_eq(r1.next_seq, 2 as u64);
    expect_eq(r1.latest_seq, 6 as u64);
    expect_true(!r1.lagged);
    expect_true(r1.has_more);

    // Page 2: continue from r1.next_seq == 2, filtered to task 10 only
    let page2_buf: [4]McpAsyncTaskEvent = [McpAsyncTaskEvent{}; 4];
    let page2_s: []McpAsyncTaskEvent = page2_buf[0..];
    const r2 = mgr.pollEvents(r1.next_seq, 10, page2_s);
    expect_eq(r2.count, 3 as usize); // seq 3 (started), 4 (progress), 6 (completed)
    expect_eq(page2_buf[0].kind, McpAsyncTaskEventKind.started);
    expect_eq(page2_buf[1].kind, McpAsyncTaskEventKind.progress);
    expect_eq(page2_buf[2].kind, McpAsyncTaskEventKind.completed);
    expect_eq(r2.next_seq, 6 as u64);
    expect_true(!r2.has_more);
}

test("McpTaskAsync: event ring overflow & lagged cursor detection") {
    let mgr = McpAsyncTaskManager.init();
    * = mgr.submitTask(77, 0x77, 100, 0, 1000); // seq 1
    * = mgr.startTask(77, 1001);                // seq 2

    // Emit 140 progress events (> MCP_TASK_EVENT_RING_CAP == 128) -> total 142 events
    let k: usize = 0;
    while (k < 140) : (k += 1) {
        const bps: u16 = ((k * 50) % 10000) as u16;
        * = mgr.updateProgress(77, bps, 0, "step", 1100 + (k as u64));
    }

    // Ring retains seq 15..=142; polling from since_seq = 5 must report lagged == true
    let ev_buf: [8]McpAsyncTaskEvent = [McpAsyncTaskEvent{}; 8];
    let ev_s: []McpAsyncTaskEvent = ev_buf[0..];
    const lagged_res = mgr.pollEvents(5, 0, ev_s);
    expect_true(lagged_res.lagged);
    expect_eq(lagged_res.count, 8 as usize);
    expect_eq(ev_buf[0].seq, (142 - MCP_TASK_EVENT_RING_CAP + 1) as u64);

    // Polling from since_seq = 140 is within retained window -> lagged == false, returns seq 141 & 142
    const fresh_res = mgr.pollEvents(140, 0, ev_s);
    expect_true(!fresh_res.lagged);
    expect_eq(fresh_res.count, 2 as usize);
    expect_eq(fresh_res.next_seq, 142 as u64);
}

test("McpTaskAsync: zero-alloc RFC 8259 JSON-RPC status formatter & escaping") {
    let mgr = McpAsyncTaskManager.init();
    * = mgr.submitTask(404, 0x404, 750, 10_000, 2000);
    * = mgr.startTask(404, 2100);
    * = mgr.failTask(404, -32005, "bad \"arg\"\\line\nnext", 2500);

    const task = mgr.getTask(404) ?? McpAsyncTask{};

    // Small buffer returns null safely
    let tiny: [32]u8 = [0; 32];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatTaskStatusJson(task, tiny_s) == null);

    let json_buf: [512]u8 = [0; 512];
    let json_s: []u8 = json_buf[0..];
    const n = formatTaskStatusJson(task, json_s) ?? 0;
    expect_true(n > 64);
    const out = json_buf[0..n];

    expect_true(containsSubstr(out, "\"taskId\":404"));
    expect_true(containsSubstr(out, "\"status\":\"failed\""));
    expect_true(containsSubstr(out, "\"pollIntervalMs\":750"));
    expect_true(containsSubstr(out, "\"errorCode\":-32005"));
    expect_true(containsSubstr(out, "\"summary\":\"bad \\\"arg\\\"\\\\line\\nnext\""));
}

test("McpTaskAsync: 64-task capacity, invalid-arg guards & cap.mcp facade parity") {
    let mgr = cap.mcp.McpAsyncTaskManager.init();

    // Invalid task_id == 0, duplicate task_id, and progress_bps > 10000
    expect_eq(mgr.submitTask(0, 0x1, 500, 0, 1000), cap.mcp.McpAsyncTaskOpStatus.invalid_arg);
    expect_eq(mgr.submitTask(1, 0x1, 500, 0, 1000), cap.mcp.McpAsyncTaskOpStatus.ok);
    expect_eq(mgr.submitTask(1, 0x1, 500, 0, 1000), cap.mcp.McpAsyncTaskOpStatus.duplicate_task_id);
    * = mgr.startTask(1, 1050);
    expect_eq(mgr.updateProgress(1, 10001, 0, "overflow", 1100), cap.mcp.McpAsyncTaskOpStatus.invalid_arg);

    // Fill up to MCP_TASK_MAX_TASKS (64)
    let id: u64 = 2;
    while (id <= MCP_TASK_MAX_TASKS as u64) : (id += 1) {
        expect_eq(mgr.submitTask(id, id * 7, 1000, 0, 1200), cap.mcp.McpAsyncTaskOpStatus.ok);
    }
    expect_eq(mgr.submitTask(999, 0x999, 1000, 0, 1300), cap.mcp.McpAsyncTaskOpStatus.capacity_full);

    const st = mgr.stats();
    expect_eq(st.active_slots, MCP_TASK_MAX_TASKS);
    expect_eq(st.capacity, MCP_TASK_MAX_TASKS);
    expect_eq(st.running_count, 1 as usize);
    expect_eq(st.queued_count, MCP_TASK_MAX_TASKS - 1);
    expect_eq(st.total_submitted, MCP_TASK_MAX_TASKS as u64);
}

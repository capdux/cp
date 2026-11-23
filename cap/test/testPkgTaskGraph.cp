import { test, assert, assertEq } from "cap:test";
import {
    PmTaskGraphError,
    PmTaskKind,
    pmTaskKindName,
    PmTaskChannelLimits,
    computePmTaskDedupHash,
    PmTaskFailureKind,
    classifyPmHttpFailure,
    isPmFailureRetryable,
    PmRetryPolicy,
    decidePmTaskRetry,
    PmTaskState,
    PmTaskDepEdge,
    PmTaskNode,
    PmTaskDispatchItem,
    PmTaskGraph,
    formatPmTaskGraphReport,
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

test("computePmTaskDedupHash separates channels and normalizes leading whitespace", () => {
    const h1 = computePmTaskDedupHash(PmTaskKind.ManifestFetch, "zod", "3.23.8");
    const h2 = computePmTaskDedupHash(PmTaskKind.ManifestFetch, "   zod", "  3.23.8");
    const h3 = computePmTaskDedupHash(PmTaskKind.TarballDownload, "zod", "3.23.8");
    assertEq(h1, h2);
    assert(h1 != h3);
    assert(strEql(pmTaskKindName(PmTaskKind.TarballExtract), "tarball_extract"));
});

test("PmTaskGraph enforces independent per-channel concurrency quotas across heterogeneous kinds", () => {
    let nodes: [8]PmTaskNode = [PmTaskNode.empty(); 8];
    let edges: [8]PmTaskDepEdge = [PmTaskDepEdge.empty(); 8];
    const limits = PmTaskChannelLimits.defaults()
        .withLimit(PmTaskKind.TarballDownload, 2)
        .withLimit(PmTaskKind.LifecycleScript, 1);

    let g = PmTaskGraph.init(nodes[0..], edges[0..], limits, PmRetryPolicy.defaults());

    // Enqueue 3 downloads + 2 lifecycle scripts
    * = g.addTask(PmTaskKind.TarballDownload, 101, "dl-a", 10) catch unreachable;
    * = g.addTask(PmTaskKind.TarballDownload, 102, "dl-b", 20) catch unreachable;
    * = g.addTask(PmTaskKind.TarballDownload, 103, "dl-c", 5) catch unreachable;
    * = g.addTask(PmTaskKind.LifecycleScript, 201, "postinstall-a", 15) catch unreachable;
    * = g.addTask(PmTaskKind.LifecycleScript, 202, "postinstall-b", 8) catch unreachable;

    // Duplicate (TarballDownload, 102) must deduplicate and boost priority
    const dup = g.addTask(PmTaskKind.TarballDownload, 102, "dl-b-dup", 50) catch unreachable;
    assert(dup.deduplicated);
    assertEq(dup.task_index, @as(usize, 1));

    let batch: [8]PmTaskDispatchItem = [PmTaskDispatchItem.empty(); 8];
    const n1 = g.dispatchReady(100, batch[0..]);
    // At most 2 downloads (dl-b priority 50, dl-a priority 10) + 1 script (postinstall-a priority 15) = 3
    assertEq(n1, @as(usize, 3));
    assertEq(batch[0].task_index, @as(usize, 1)); // dl-b (prio 50)
    assertEq(batch[1].task_index, @as(usize, 3)); // postinstall-a (prio 15)
    assertEq(batch[2].task_index, @as(usize, 0)); // dl-a (prio 10)

    // Saturated channels return 0
    assertEq(g.dispatchReady(105, batch[0..]), @as(usize, 0));

    // Completing 1 download frees slot for dl-c (index 2)
    g.completeSuccess(1, 150) catch unreachable;
    const n2 = g.dispatchReady(150, batch[0..]);
    assertEq(n2, @as(usize, 1));
    assertEq(batch[0].task_index, @as(usize, 2));
});

test("PmTaskGraph executes full 5-stage package pipeline and tracks critical-path latency", () => {
    let nodes: [8]PmTaskNode = [PmTaskNode.empty(); 8];
    let edges: [8]PmTaskDepEdge = [PmTaskDepEdge.empty(); 8];
    let g = PmTaskGraph.init(nodes[0..], edges[0..], PmTaskChannelLimits.defaults(), PmRetryPolicy.defaults());

    // Chain: ManifestFetch(0) -> TarballDownload(1) -> TarballExtract(2) -> PatchApply(3) -> LifecycleScript(4)
    const t0 = (g.addTask(PmTaskKind.ManifestFetch, 1, "manifest", 10) catch unreachable).task_index;
    const t1 = (g.addTask(PmTaskKind.TarballDownload, 2, "download", 10) catch unreachable).task_index;
    const t2 = (g.addTask(PmTaskKind.TarballExtract, 3, "extract", 10) catch unreachable).task_index;
    const t3 = (g.addTask(PmTaskKind.PatchApply, 4, "patch", 10) catch unreachable).task_index;
    const t4 = (g.addTask(PmTaskKind.LifecycleScript, 5, "script", 10) catch unreachable).task_index;

    g.addDependency(t0, t1, true) catch unreachable;
    g.addDependency(t1, t2, true) catch unreachable;
    g.addDependency(t2, t3, true) catch unreachable;
    g.addDependency(t3, t4, true) catch unreachable;

    // Cycle check: t4 -> t0 must be rejected
    let cycle_err = false;
    g.addDependency(t4, t0, true) catch {
        cycle_err = true;
    };
    assert(cycle_err);

    // Execute step-by-step with durations: 30 + 120 + 40 + 15 + 80 = 285ms
    const durations: [5]u64 = [30, 120, 40, 15, 80];
    let batch: [4]PmTaskDispatchItem = [PmTaskDispatchItem.empty(); 4];
    let now: u64 = 1000;
    let step: usize = 0;
    while (step < 5) {
        const n = g.dispatchReady(now, batch[0..]);
        assertEq(n, @as(usize, 1));
        assertEq(batch[0].task_index, step);
        now += durations[step];
        g.completeSuccess(batch[0].task_index, now) catch unreachable;
        step += 1;
    }

    assert(g.isDrained());
    const st = g.stats();
    assertEq(st.succeeded, @as(usize, 5));
    assertEq(st.critical_path_ms, @as(u64, 285));
    assertEq(st.total_exec_ms, @as(u64, 285));

    let rep: [1024]u8 = [0; 1024];
    const rlen = formatPmTaskGraphReport(st, PmTaskChannelLimits.defaults(), rep[0..]) catch unreachable;
    const text = rep[0..rlen];
    assert(strContains(text, "PmTaskGraph Summary"));
    assert(strContains(text, "critical_path=285ms"));
    assert(strContains(text, "channel[patch_apply]: done=1"));
});

test("decidePmTaskRetry classifies HTTP 429/5xx vs permanent 404/SRI and honors Retry-After", () => {
    assert(classifyPmHttpFailure(429) == PmTaskFailureKind.Http429RateLimit);
    assert(classifyPmHttpFailure(503) == PmTaskFailureKind.Http5xxServer);
    assert(classifyPmHttpFailure(404) == PmTaskFailureKind.Http404NotFound);
    assert(classifyPmHttpFailure(401) == PmTaskFailureKind.Http401Unauthorized);

    assert(isPmFailureRetryable(PmTaskFailureKind.Http429RateLimit));
    assert(isPmFailureRetryable(PmTaskFailureKind.Http5xxServer));
    assert(isPmFailureRetryable(PmTaskFailureKind.NetworkTimeout));
    assert(!isPmFailureRetryable(PmTaskFailureKind.Http404NotFound));
    assert(!isPmFailureRetryable(PmTaskFailureKind.IntegrityMismatch));
    assert(!isPmFailureRetryable(PmTaskFailureKind.ScriptNonZeroExit));

    const policy = PmRetryPolicy{
        max_retries: 3,
        base_backoff_ms: 200,
        max_backoff_ms: 5000,
        jitter_permille: 0,
    };

    // Exponential backoff without jitter: 200, 400, 800, then exhausted
    const d0 = decidePmTaskRetry(PmTaskFailureKind.Http5xxServer, 0, null, 1, policy);
    assert(d0.should_retry && d0.delay_ms == 200 && d0.next_attempt == 1 && !d0.used_retry_after);

    const d1 = decidePmTaskRetry(PmTaskFailureKind.Http5xxServer, 1, null, 1, policy);
    assert(d1.should_retry && d1.delay_ms == 400 && d1.next_attempt == 2);

    const d3 = decidePmTaskRetry(PmTaskFailureKind.Http5xxServer, 3, null, 1, policy);
    assert(!d3.should_retry);

    // Retry-After override (clamped to max_backoff_ms=5000)
    const d_ra = decidePmTaskRetry(PmTaskFailureKind.Http429RateLimit, 0, 12000, 1, policy);
    assert(d_ra.should_retry && d_ra.used_retry_after && d_ra.delay_ms == 5000);

    // Permanent 404 never retries
    const d_404 = decidePmTaskRetry(PmTaskFailureKind.Http404NotFound, 0, null, 1, policy);
    assert(!d_404.should_retry);
});

test("PmTaskGraph retries transient HTTP 429 with backoff timer before succeeding", () => {
    let nodes: [4]PmTaskNode = [PmTaskNode.empty(); 4];
    let edges: [4]PmTaskDepEdge = [PmTaskDepEdge.empty(); 4];
    const policy = PmRetryPolicy{ max_retries: 2, base_backoff_ms: 500, max_backoff_ms: 5000, jitter_permille: 0 };
    let g = PmTaskGraph.init(nodes[0..], edges[0..], PmTaskChannelLimits.defaults(), policy);

    const t0 = (g.addTask(PmTaskKind.ManifestFetch, 77, "lodash", 1) catch unreachable).task_index;
    let batch: [2]PmTaskDispatchItem = [PmTaskDispatchItem.empty(); 2];

    assertEq(g.dispatchReady(1000, batch[0..]), @as(usize, 1));
    const retry_dec = g.completeFailure(t0, PmTaskFailureKind.Http429RateLimit, 1500, 1050) catch unreachable;
    assert(retry_dec.should_retry && retry_dec.used_retry_after && retry_dec.delay_ms == 1500);
    assert(nodes[t0].state == PmTaskState.WaitingRetry);

    // Not eligible before 1050 + 1500 = 2550
    assertEq(g.dispatchReady(2000, batch[0..]), @as(usize, 0));
    const next_ts = g.nextRetryTimestampMs() orelse unreachable;
    assertEq(next_ts, @as(u64, 2550));

    // Eligible at 2550 -> succeeds on retry attempt 1
    assertEq(g.dispatchReady(2550, batch[0..]), @as(usize, 1));
    assertEq(batch[0].attempt, @as(u8, 1));
    g.completeSuccess(t0, 2600) catch unreachable;
    assert(g.isDrained());
    assertEq(g.stats().total_retries, @as(u32, 1));
    assertEq(g.stats().succeeded, @as(usize, 1));
});

test("PmTaskGraph cascades cancellation on required upstream failure while sparing optional dependents", () => {
    let nodes: [6]PmTaskNode = [PmTaskNode.empty(); 6];
    let edges: [6]PmTaskDepEdge = [PmTaskDepEdge.empty(); 6];
    let g = PmTaskGraph.init(nodes[0..], edges[0..], PmTaskChannelLimits.defaults(), PmRetryPolicy.defaults());

    // 0 (download) --required--> 1 (extract) --required--> 2 (script)
    // 0 (download) --optional--> 3 (optional-audit)
    const t0 = (g.addTask(PmTaskKind.TarballDownload, 10, "pkg", 1) catch unreachable).task_index;
    const t1 = (g.addTask(PmTaskKind.TarballExtract, 11, "extract", 1) catch unreachable).task_index;
    const t2 = (g.addTask(PmTaskKind.LifecycleScript, 12, "script", 1) catch unreachable).task_index;
    const t3 = (g.addTask(PmTaskKind.PatchApply, 13, "opt-step", 1) catch unreachable).task_index;

    g.addDependency(t0, t1, true) catch unreachable;
    g.addDependency(t1, t2, true) catch unreachable;
    g.addDependency(t0, t3, false) catch unreachable;

    let batch: [4]PmTaskDispatchItem = [PmTaskDispatchItem.empty(); 4];
    assertEq(g.dispatchReady(0, batch[0..]), @as(usize, 1));

    // Permanent IntegrityMismatch -> t0 Failed, t1 & t2 Cancelled, t3 becomes ready!
    const retry_dec = g.completeFailure(t0, PmTaskFailureKind.IntegrityMismatch, null, 50) catch unreachable;
    assert(!retry_dec.should_retry);
    assert(nodes[t0].state == PmTaskState.Failed);
    assert(nodes[t1].state == PmTaskState.Cancelled);
    assert(nodes[t2].state == PmTaskState.Cancelled);
    assert(nodes[t3].state == PmTaskState.Pending);

    assertEq(g.dispatchReady(60, batch[0..]), @as(usize, 1));
    assertEq(batch[0].task_index, t3);
    g.completeSuccess(t3, 80) catch unreachable;

    assert(g.isDrained());
    const st = g.stats();
    assertEq(st.failed, @as(usize, 1));
    assertEq(st.cancelled, @as(usize, 2));
    assertEq(st.succeeded, @as(usize, 1));
});

test("deterministic 500-iteration fuzz over random task transitions and adversarial edges causes 0 panics or deadlocks", () => {
    let seed: u64 = 0x517cc1b727220a95;
    let nodes: [16]PmTaskNode = [PmTaskNode.empty(); 16];
    let edges: [48]PmTaskDepEdge = [PmTaskDepEdge.empty(); 48];
    let batch: [4]PmTaskDispatchItem = [PmTaskDispatchItem.empty(); 4];

    const kinds: [6]PmTaskKind = [
        PmTaskKind.ManifestFetch,
        PmTaskKind.TarballDownload,
        PmTaskKind.TarballExtract,
        PmTaskKind.GitCheckout,
        PmTaskKind.PatchApply,
        PmTaskKind.LifecycleScript,
    ];
    const failures: [5]PmTaskFailureKind = [
        PmTaskFailureKind.NetworkTimeout,
        PmTaskFailureKind.Http429RateLimit,
        PmTaskFailureKind.Http5xxServer,
        PmTaskFailureKind.Http404NotFound,
        PmTaskFailureKind.IntegrityMismatch,
    ];

    let iter: u32 = 0;
    let total_drained: u32 = 0;
    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        const lim = PmTaskChannelLimits.defaults().withLimit(PmTaskKind.TarballDownload, 2);
        const pol = PmRetryPolicy{ max_retries: 2, base_backoff_ms: 10, max_backoff_ms: 100, jitter_permille: 200 };
        let g = PmTaskGraph.init(nodes[0..], edges[0..], lim, pol);

        const task_n: usize = @as(usize, @intCast(r % 8)) + 2;
        let t: usize = 0;
        while (t < task_n) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            const k = kinds[@as(usize, @intCast((r >> 16) % 6))];
            * = g.addTask(k, @as(u64, t) + 1, "fuzz", @as(u32, @intCast((r >> 8) % 10))) catch unreachable;
            t += 1;
        }

        // Attempt random edges including self-loops, out-of-bounds, and potential cycles
        let e: usize = 0;
        while (e < 12) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            const u = @as(usize, @intCast((r >> 8) % (task_n + 2)));
            const v = @as(usize, @intCast((r >> 24) % (task_n + 2)));
            const req = (r & 1) == 0;
            g.addDependency(u, v, req) catch {};
            e += 1;
        }

        // Drive scheduler to completion with bounded step guard (must never deadlock)
        let now: u64 = 0;
        let guard: u32 = 0;
        while (!g.isDrained() && guard < 128) {
            const n = g.dispatchReady(now, batch[0..]);
            if (n == 0) {
                if (next_ts := g.nextRetryTimestampMs()) {
                    now = next_ts;
                } else {
                    break;
                }
            } else {
                let b: usize = 0;
                while (b < n) {
                    r = r *% 6364136223846793005 +% 1442695040888963407;
                    now += 5;
                    if ((r & 3) == 0) {
                        const fk = failures[@as(usize, @intCast((r >> 16) % 5))];
                        * = g.completeFailure(batch[b].task_index, fk, null, now) catch unreachable;
                    } else {
                        g.completeSuccess(batch[b].task_index, now) catch unreachable;
                    }
                    b += 1;
                }
            }
            guard += 1;
        }

        assert(g.isDrained());
        const st = g.stats();
        assertEq(st.succeeded + st.failed + st.cancelled, st.total_tasks);
        total_drained += 1;
        iter += 1;
    }

    assertEq(total_drained, @as(u32, 500));
});

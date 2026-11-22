import { test, assert, assertEq } from "cap:test";
import {
    RESOLVE_QUEUE_NO_PARENT,
    ResolveQueueError,
    RqDepPriority,
    RqTaskStatus,
    RqPlacementKind,
    RqSemver,
    RqTaskEntry,
    RqVersionCandidate,
    RqDemandSpec,
    RqReconciledDep,
    satisfiesSemverRange,
    PkgResolveQueue,
    selectBestMatchingVersion,
    reconcileResolvedGraph,
    formatResolveQueueReport,
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

test("SemVer 2.0 parsing and prerelease ordering rules", () => {
    const v1 = RqSemver.parse("1.0.0-alpha.1") catch unreachable;
    const v2 = RqSemver.parse("1.0.0-alpha.2") catch unreachable;
    const v3 = RqSemver.parse("1.0.0-alpha.beta") catch unreachable;
    const v4 = RqSemver.parse("1.0.0-beta.1") catch unreachable;
    const v5 = RqSemver.parse("v1.0.0+build.123") catch unreachable;

    assertEq(RqSemver.compare(v1, v2), @as(i32, -1));
    assertEq(RqSemver.compare(v2, v3), @as(i32, -1)); // numeric < alpha
    assertEq(RqSemver.compare(v3, v4), @as(i32, -1));
    assertEq(RqSemver.compare(v4, v5), @as(i32, -1)); // prerelease < stable
    assertEq(RqSemver.compare(v5, RqSemver.parse("1.0.0") catch unreachable), @as(i32, 0));
});

test("SemVer range evaluation: caret (0.x/0.0.x), tilde, AND/OR clauses, and prerelease isolation", () => {
    const v150 = RqSemver.parse("1.5.0") catch unreachable;
    const v200 = RqSemver.parse("2.0.0") catch unreachable;
    const v025 = RqSemver.parse("0.2.5") catch unreachable;
    const v030 = RqSemver.parse("0.3.0") catch unreachable;
    const v004 = RqSemver.parse("0.0.4") catch unreachable;
    const v150_rc = RqSemver.parse("1.5.0-rc.1") catch unreachable;

    assert(satisfiesSemverRange(v150, "^1.2.3") catch unreachable);
    assert(!(satisfiesSemverRange(v200, "^1.2.3") catch unreachable));

    // Caret 0.x and 0.0.x rules
    assert(satisfiesSemverRange(v025, "^0.2.3") catch unreachable);
    assert(!(satisfiesSemverRange(v030, "^0.2.3") catch unreachable));
    assert(!(satisfiesSemverRange(v004, "^0.0.3") catch unreachable));

    // Tilde
    assert(satisfiesSemverRange(RqSemver.parse("1.2.9") catch unreachable, "~1.2.3") catch unreachable);
    assert(!(satisfiesSemverRange(RqSemver.parse("1.3.0") catch unreachable, "~1.2.3") catch unreachable));

    // Compound AND + OR union
    assert(satisfiesSemverRange(v150, ">=1.2.0 <1.6.0 || ^3.0.0") catch unreachable);
    assert(satisfiesSemverRange(RqSemver.parse("3.4.1") catch unreachable, ">=1.2.0 <1.6.0 || ^3.0.0") catch unreachable);
    assert(!(satisfiesSemverRange(v200, ">=1.2.0 <1.6.0 || ^3.0.0") catch unreachable));

    // Prerelease isolation: ^1.2.3 must NOT match 1.5.0-rc.1, but ^1.5.0-alpha.1 DOES match
    assert(!(satisfiesSemverRange(v150_rc, "^1.2.3") catch unreachable));
    assert(satisfiesSemverRange(v150_rc, "^1.5.0-alpha.1") catch unreachable);
});

test("PkgResolveQueue deduplication, priority promotion, and concurrency throttling", () => {
    let backing: [8]RqTaskEntry = [RqTaskEntry.empty(); 8];
    let q = PkgResolveQueue.init(backing[0..], 2, 2);

    const id_opt = q.enqueue("fsevents", "^2.3.0", RqDepPriority.Optional, RESOLVE_QUEUE_NO_PARENT) catch unreachable;
    const id_trans = q.enqueue("chalk", "^5.0.0", RqDepPriority.Transitive, RESOLVE_QUEUE_NO_PARENT) catch unreachable;
    const id_root = q.enqueue("react", "^18.2.0", RqDepPriority.RootDirect, RESOLVE_QUEUE_NO_PARENT) catch unreachable;

    // Duplicate enqueue of chalk with Peer priority deduplicates and promotes priority
    const id_dup = q.enqueue("chalk", "^5.0.0", RqDepPriority.Peer, id_root) catch unreachable;
    assertEq(id_dup, id_trans);
    assert(backing[@as(usize, id_trans)].priority == RqDepPriority.Peer);

    // Pop respects priority order: RootDirect (react) -> Peer (chalk), then throttled at max_in_flight=2
    const p1 = q.popNextReady(0).?;
    const p2 = q.popNextReady(0).?;
    assertEq(p1, id_root);
    assertEq(p2, id_trans);
    assert(q.popNextReady(0) == null); // max_in_flight == 2 reached

    q.markResolved(p1, "18.3.1") catch unreachable;
    const p3 = q.popNextReady(0).?;
    assertEq(p3, id_opt);
    q.markResolved(p2, "5.3.0") catch unreachable;
    q.markResolved(p3, "2.3.3") catch unreachable;

    assert(q.isDrained());
    const st = q.stats();
    assertEq(st.total_enqueued, @as(u32, 3));
    assertEq(st.deduplicated_hits, @as(u32, 1));
    assertEq(st.resolved, @as(u32, 3));
});

test("Exponential backoff retry scheduling and optional dependency fallback to SkippedOptional", () => {
    let backing: [4]RqTaskEntry = [RqTaskEntry.empty(); 4];
    let q = PkgResolveQueue.init(backing[0..], 4, 2);

    const id_opt = q.enqueue("opt-native", "^1.0.0", RqDepPriority.Optional, RESOLVE_QUEUE_NO_PARENT) catch unreachable;
    const id_req = q.enqueue("core-lib", "^1.0.0", RqDepPriority.RootDirect, RESOLVE_QUEUE_NO_PARENT) catch unreachable;

    // Attempt 1 fails retryably at t=1000 -> backoff 100ms (ready at t=1100)
    const p_req = q.popNextReady(1000).?;
    assertEq(p_req, id_req);
    q.markFailed(p_req, 1000, true) catch unreachable;
    assertEq(backing[@as(usize, id_req)].next_retry_after_ms, @as(u64, 1100));

    // At t=1050, id_req is not yet ready; id_opt pops instead
    const p_opt = q.popNextReady(1050).?;
    assertEq(p_opt, id_opt);
    // Non-retryable failure on Optional -> SkippedOptional
    q.markFailed(p_opt, 1050, false) catch unreachable;
    assert(backing[@as(usize, id_opt)].status == RqTaskStatus.SkippedOptional);

    // At t=1100, id_req becomes ready again, attempt 2 succeeds
    const p_req2 = q.popNextReady(1100).?;
    assertEq(p_req2, id_req);
    q.markResolved(p_req2, "1.4.2") catch unreachable;

    const st = q.stats();
    assertEq(st.retries_scheduled, @as(u32, 1));
    assertEq(st.skipped_optional, @as(u32, 1));
    assertEq(st.resolved, @as(u32, 1));
});

test("selectBestMatchingVersion prefers dedup reuse, skips yanked, and avoids deprecated", () => {
    const cands: [4]RqVersionCandidate = [
        RqVersionCandidate{ version_str: "1.2.0", is_deprecated: false, is_yanked: false },
        RqVersionCandidate{ version_str: "1.4.0", is_deprecated: false, is_yanked: false },
        RqVersionCandidate{ version_str: "1.5.0", is_deprecated: true, is_yanked: false },
        RqVersionCandidate{ version_str: "1.6.0", is_deprecated: false, is_yanked: true },
    ];

    // Without existing version: picks 1.4.0 (skips yanked 1.6.0 and deprecated 1.5.0)
    const s1 = selectBestMatchingVersion("^1.0.0", cands[0..], null) catch unreachable;
    assert(s1.found && !s1.reused_existing);
    assert(strEql(s1.version_str, "1.4.0"));

    // With compatible existing version 1.2.0: reuses 1.2.0 to avoid duplicate install
    const s2 = selectBestMatchingVersion("^1.0.0", cands[0..], "1.2.0") catch unreachable;
    assert(s2.found && s2.reused_existing);
    assert(strEql(s2.version_str, "1.2.0"));
});

test("Diamond dependency reconciliation hoists max-coverage version and isolates incompatible demands", () => {
    const cands: [4]RqVersionCandidate = [
        RqVersionCandidate{ version_str: "1.8.0" },
        RqVersionCandidate{ version_str: "2.1.0" },
        RqVersionCandidate{ version_str: "2.3.4" },
        RqVersionCandidate{ version_str: "2.5.0" },
    ];
    const demands: [5]RqDemandSpec = [
        RqDemandSpec{ requester_name: "app", pkg_name: "zod", range_expr: "^2.0.0" },
        RqDemandSpec{ requester_name: "sdk", pkg_name: "zod", range_expr: "^2.1.0" },
        RqDemandSpec{ requester_name: "cli", pkg_name: "zod", range_expr: ">=2.0.0 <2.4.0" },
        RqDemandSpec{ requester_name: "legacy", pkg_name: "zod", range_expr: "^1.0.0" },
        RqDemandSpec{ requester_name: "future", pkg_name: "zod", range_expr: "^9.0.0" },
    ];

    let out: [8]RqReconciledDep = [RqReconciledDep.empty(); 8];
    const sum = reconcileResolvedGraph("zod", demands[0..], cands[0..], out[0..]) catch unreachable;

    // 2.3.4 satisfies 3 demands (app, sdk, cli) -> chosen as hoisted shared version!
    assert(strEql(sum.hoisted_version, "2.3.4"));
    assertEq(sum.hoisted_shared, @as(u32, 3));
    assertEq(sum.nested_isolated, @as(u32, 1)); // legacy -> 1.8.0
    assertEq(sum.unresolved_conflicts, @as(u32, 1)); // future -> ^9.0.0
    assertEq(sum.dedup_saved_installs, @as(u32, 2));

    assert(out[0].placement == RqPlacementKind.HoistedShared);
    assert(out[3].placement == RqPlacementKind.NestedIsolated);
    assert(strEql(out[3].selected_version, "1.8.0"));
    assert(out[4].placement == RqPlacementKind.UnresolvedConflict);
});

test("formatResolveQueueReport and invalid semver/task guard checks", () => {
    let backing: [2]RqTaskEntry = [RqTaskEntry.empty(); 2];
    let q = PkgResolveQueue.init(backing[0..], 2, 1);
    const tid = q.enqueue("zod", "^2.0.0", RqDepPriority.RootDirect, RESOLVE_QUEUE_NO_PARENT) catch unreachable;
    * = q.popNextReady(0);
    q.markResolved(tid, "2.3.4") catch unreachable;

    const cands: [1]RqVersionCandidate = [RqVersionCandidate{ version_str: "2.3.4" }];
    const demands: [2]RqDemandSpec = [
        RqDemandSpec{ requester_name: "a", pkg_name: "zod", range_expr: "^2.0.0" },
        RqDemandSpec{ requester_name: "b", pkg_name: "zod", range_expr: "^2.1.0" },
    ];
    let out: [2]RqReconciledDep = [RqReconciledDep.empty(); 2];
    const rec = reconcileResolvedGraph("zod", demands[0..], cands[0..], out[0..]) catch unreachable;

    let buf: [256]u8 = [0; 256];
    const n = formatResolveQueueReport(q.stats(), rec, buf[0..]) catch unreachable;
    assert(strEql(buf[0..n], "resolve tasks=1 resolved=1 dedup=0 retries=0 failed=0 | zod hoisted=2.3.4 shared=2 nested=0 saved=1"));

    // Invalid SemVer rejected
    let bad_sv = false;
    * = RqSemver.parse("not-a-version") catch |e| {
        bad_sv = (e == ResolveQueueError.InvalidSemver);
    };
    assert(bad_sv);

    // Invalid task ID rejected
    let bad_id = false;
    q.markResolved(99, "1.0.0") catch |e| {
        bad_id = (e == ResolveQueueError.InvalidTaskId);
    };
    assert(bad_id);
});

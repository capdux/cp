// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/playbook.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Procedural Playbook / SOP Memory Engine:
//   1. Playbook registration, multi-step SOP authoring, version bumping & 12-step capacity boundary
//   2. Hardware `@popCount` intent bitmask coverage & hard precondition bitmask filtering
//   3. Laplace-smoothed Bayesian reliability `(s+1)/(s+f+2)` & deterministic Top-K ranking
//   4. Automatic degradation (`active -> degraded -> deprecated`) & recovery (`degraded -> active`)
//   5. EWMA latency tracking, `max_latency_ms` gating & `min_success_rate_bps` filtering
//   6. Explicit `deprecate`, `include_degraded` toggle & `PlaybookStoreStats` telemetry
//   7. Full 32-entry capacity boundary, invalid input guards & `cap.agentmem` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    PLAYBOOK_MAX_ENTRIES,
    PLAYBOOK_MAX_STEPS,
    PlaybookStatus,
    PlaybookQuery,
    PlaybookMatch,
    PlaybookMemoryStore,
} from "cap:agentmem";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

const INT_BUILD: u64 = 1 << 0;
const INT_TEST: u64 = 1 << 1;
const INT_DEPLOY: u64 = 1 << 2;
const INT_ROLLBACK: u64 = 1 << 3;

const PRE_GIT: u64 = 1 << 0;
const PRE_DOCKER: u64 = 1 << 1;
const PRE_K8S: u64 = 1 << 2;

test("Playbook: registration, multi-step SOP authoring & 12-step boundary") {
    let store = PlaybookMemoryStore.init();

    // Reject empty name or zero intent_mask
    expect_true(store.registerPlaybook("", INT_BUILD, 0, 1000) == null);
    expect_true(store.registerPlaybook("noop", 0, 0, 1000) == null);

    const id = store.registerPlaybook("ci_release_sop", INT_BUILD | INT_TEST | INT_DEPLOY, PRE_GIT | PRE_DOCKER, 1000) ?? 0;
    expect_eq(id, @as(u32, 1));

    expect_true(store.addStep(id, "git_pull", "working tree clean", false, 5000));
    expect_true(store.addStep(id, "cap_test", "all unit tests green", false, 30000));
    expect_true(store.addStep(id, "notify_slack", "release posted", true, 3000));
    expect_true(!store.addStep(id, "", "empty tool rejected", false, 1000));
    expect_true(!store.addStep(999, "git_pull", "missing playbook", false, 1000));

    if (pb := store.getPlaybook(id)) {
        expect_true(strEq(pb.name(), "ci_release_sop"));
        expect_eq(pb.step_count, @as(usize, 3));
        expect_eq(pb.version, @as(u16, 4)); // initial 1 + 3 steps
        expect_eq(pb.successRateBps(), @as(u16, 5000)); // Laplace neutral (0+1)/(0+2) = 50%
        expect_true(strEq(pb.steps[0].tool_name(), "git_pull"));
        expect_true(strEq(pb.steps[1].expected_outcome(), "all unit tests green"));
        expect_true(pb.steps[2].is_optional);
        expect_eq(pb.steps[2].timeout_ms, @as(u32, 3000));
    } else {
        expect_true(false);
    }

    // Fill up to PLAYBOOK_MAX_STEPS (12); 13th step must return false
    let s: usize = 3;
    while (s < PLAYBOOK_MAX_STEPS) : (s += 1) {
        expect_true(store.addStep(id, "extra_step", "ok", true, 1000));
    }
    expect_true(!store.addStep(id, "overflow_step", "denied", true, 1000));
}

test("Playbook: @popCount intent coverage & hard precondition gating") {
    let store = PlaybookMemoryStore.init();

    // Playbook 1: covers BUILD | TEST | DEPLOY, requires GIT | DOCKER | K8S
    const p_k8s = store.registerPlaybook("k8s_full", INT_BUILD | INT_TEST | INT_DEPLOY, PRE_GIT | PRE_DOCKER | PRE_K8S, 1000) ?? 0;
    // Playbook 2: covers BUILD | TEST | DEPLOY, requires GIT | DOCKER
    const p_docker = store.registerPlaybook("docker_full", INT_BUILD | INT_TEST | INT_DEPLOY, PRE_GIT | PRE_DOCKER, 1000) ?? 0;
    // Playbook 3: covers BUILD only, requires GIT
    const p_build = store.registerPlaybook("build_only", INT_BUILD, PRE_GIT, 1000) ?? 0;

    let out: [4]PlaybookMatch = [PlaybookMatch{}; 4];
    let out_s: []PlaybookMatch = out[0..];

    // Environment only has GIT | DOCKER (no K8S) -> p_k8s must be hard-filtered out
    const q = PlaybookQuery{
        required_intent_mask: INT_BUILD | INT_TEST | INT_DEPLOY,
        available_preconditions: PRE_GIT | PRE_DOCKER,
    };
    const n = store.matchTopK(q, *out_s);
    expect_eq(n, @as(usize, 2));
    // p_docker has 3/3 intent overlap vs p_build 1/3 -> p_docker ranks first
    expect_eq(out[0].playbook_id, p_docker);
    expect_eq(out[0].intent_overlap, @as(u32, 3));
    expect_eq(out[1].playbook_id, p_build);
    expect_eq(out[1].intent_overlap, @as(u32, 1));
    * = p_k8s;
}

test("Playbook: Bayesian Laplace-smoothed reliability scoring & Top-K ranking") {
    let store = PlaybookMemoryStore.init();
    const p_proven = store.registerPlaybook("proven_sop", INT_BUILD | INT_TEST, PRE_GIT, 1000) ?? 0;
    const p_fresh = store.registerPlaybook("fresh_sop", INT_BUILD | INT_TEST, PRE_GIT, 1000) ?? 0;
    const p_shaky = store.registerPlaybook("shaky_sop", INT_BUILD | INT_TEST, PRE_GIT, 1000) ?? 0;

    // Proven: 8/8 successes -> (8+1)/(8+2) = 9000 bps
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        * = store.recordExecution(p_proven, true, 200, 2000 + (i as u64));
    }
    // Shaky: 1 success, 1 failure -> (1+1)/(2+2) = 5000 bps, higher latency 900ms
    * = store.recordExecution(p_shaky, true, 900, 2100);
    * = store.recordExecution(p_shaky, false, 900, 2200);

    if (pb := store.getPlaybook(p_proven)) {
        expect_eq(pb.successRateBps(), @as(u16, 9000));
    } else {
        expect_true(false);
    }

    let out: [3]PlaybookMatch = [PlaybookMatch{}; 3];
    let out_s: []PlaybookMatch = out[0..];
    const q = PlaybookQuery{
        required_intent_mask: INT_BUILD | INT_TEST,
        available_preconditions: PRE_GIT,
    };
    const n = store.matchTopK(q, *out_s);
    expect_eq(n, @as(usize, 3));
    expect_eq(out[0].playbook_id, p_proven);
    expect_eq(out[0].success_rate_bps, @as(u16, 9000));
    expect_eq(out[1].playbook_id, p_fresh); // same 5000 bps as shaky, lower latency penalty
    expect_eq(out[2].playbook_id, p_shaky);
    expect_true(out[0].score > out[1].score);
    expect_true(out[1].score > out[2].score);
}

test("Playbook: automatic active -> degraded -> deprecated & recovery back to active") {
    let store = PlaybookMemoryStore.init();
    const id = store.registerPlaybook("flaky_deploy", INT_DEPLOY, PRE_DOCKER, 1000) ?? 0;

    // 1 success + 2 failures -> 3 trials, rate = (1+1)/(3+2) = 4000 bps < 4500 -> degraded
    const s1 = store.recordExecution(id, true, 100, 1100) ?? PlaybookStatus.draft;
    expect_true(s1 == PlaybookStatus.active);
    const s2 = store.recordExecution(id, false, 120, 1200) ?? PlaybookStatus.draft;
    expect_true(s2 == PlaybookStatus.active); // only 2 trials (< 3), still active
    const s3 = store.recordExecution(id, false, 140, 1300) ?? PlaybookStatus.draft;
    expect_true(s3 == PlaybookStatus.degraded);

    // Consecutive successes recover back to active:
    // +1 success -> (2+1)/(4+2)=5000 bps (still degraded)
    // +2 success -> (3+1)/(5+2)=5714 bps >= 5500 & consecutive_successes=2 -> active!
    const r1 = store.recordExecution(id, true, 100, 1400) ?? PlaybookStatus.draft;
    expect_true(r1 == PlaybookStatus.degraded);
    const r2 = store.recordExecution(id, true, 100, 1500) ?? PlaybookStatus.draft;
    expect_true(r2 == PlaybookStatus.active);

    // Second playbook: sustained failures -> degraded -> deprecated
    const id2 = store.registerPlaybook("broken_sop", INT_ROLLBACK, PRE_DOCKER, 1000) ?? 0;
    * = store.recordExecution(id2, false, 200, 2000);
    * = store.recordExecution(id2, false, 200, 2100);
    const d1 = store.recordExecution(id2, false, 200, 2200) ?? PlaybookStatus.draft;
    // 0/3 -> (0+1)/(3+2) = 2000 bps < 2500 -> immediately deprecated
    expect_true(d1 == PlaybookStatus.deprecated);
}

test("Playbook: EWMA latency gating & min_success_rate_bps filter") {
    let store = PlaybookMemoryStore.init();
    const fast_id = store.registerPlaybook("fast_sop", INT_TEST, 0, 1000) ?? 0;
    const slow_id = store.registerPlaybook("slow_sop", INT_TEST, 0, 1000) ?? 0;

    // Fast: initial 100ms, then 200ms -> EWMA = (100*3 + 200)/4 = 125ms
    * = store.recordExecution(fast_id, true, 100, 1100);
    * = store.recordExecution(fast_id, true, 200, 1200);
    if (pb := store.getPlaybook(fast_id)) {
        expect_eq(pb.avg_latency_ms, @as(u32, 125));
    } else {
        expect_true(false);
    }

    // Slow: 1500ms
    * = store.recordExecution(slow_id, true, 1500, 1100);
    * = store.recordExecution(slow_id, true, 1500, 1200);

    let out: [4]PlaybookMatch = [PlaybookMatch{}; 4];
    let out_s: []PlaybookMatch = out[0..];
    const q = PlaybookQuery{
        required_intent_mask: INT_TEST,
        available_preconditions: 0,
        max_latency_ms: 500,
        min_success_rate_bps: 7000, // (2+1)/(2+2) = 7500 bps passes
    };
    const n = store.matchTopK(q, *out_s);
    expect_eq(n, @as(usize, 1));
    expect_eq(out[0].playbook_id, fast_id);
}

test("Playbook: explicit deprecate, include_degraded toggle & stats telemetry") {
    let store = PlaybookMemoryStore.init();
    const id_a = store.registerPlaybook("sop_a", INT_DEPLOY, 0, 1000) ?? 0;
    const id_b = store.registerPlaybook("sop_b", INT_DEPLOY, 0, 1000) ?? 0;
    const id_c = store.registerPlaybook("sop_c", INT_DEPLOY, 0, 1000) ?? 0;

    // Make id_b degraded (1 success + 2 failures -> 4000 bps)
    * = store.recordExecution(id_b, true, 100, 1100);
    * = store.recordExecution(id_b, false, 100, 1200);
    * = store.recordExecution(id_b, false, 100, 1300);

    // Explicitly deprecate id_c
    expect_true(store.deprecate(id_c));
    expect_true(!store.deprecate(9999));

    let out: [4]PlaybookMatch = [PlaybookMatch{}; 4];
    let out_s: []PlaybookMatch = out[0..];

    // Default include_degraded = false -> only id_a matched
    const q_strict = PlaybookQuery{ required_intent_mask: INT_DEPLOY, include_degraded: false };
    expect_eq(store.matchTopK(q_strict, *out_s), @as(usize, 1));
    expect_eq(out[0].playbook_id, id_a);

    // include_degraded = true -> id_a and id_b matched (id_c still excluded)
    const q_relaxed = PlaybookQuery{ required_intent_mask: INT_DEPLOY, include_degraded: true };
    expect_eq(store.matchTopK(q_relaxed, *out_s), @as(usize, 2));

    const st = store.stats();
    expect_eq(st.total_playbooks, @as(usize, 3));
    expect_eq(st.active_count, @as(usize, 1));
    expect_eq(st.degraded_count, @as(usize, 1));
    expect_eq(st.deprecated_count, @as(usize, 1));
    expect_eq(st.total_executions, @as(u64, 3));
    expect_eq(st.total_successes, @as(u64, 1));
}

test("Playbook: 32-entry full capacity boundary & cap.agentmem facade parity") {
    let store = PlaybookMemoryStore.init();
    let i: usize = 0;
    while (i < PLAYBOOK_MAX_ENTRIES) : (i += 1) {
        const maybe_id = store.registerPlaybook("sop_item", INT_BUILD, 0, 1000 + (i as u64));
        expect_true(maybe_id != null);
    }
    // 33rd registration must safely return null
    expect_true(store.registerPlaybook("sop_overflow", INT_BUILD, 0, 9999) == null);
    expect_eq(store.stats().total_playbooks, PLAYBOOK_MAX_ENTRIES);

    // `cap.agentmem` facade parity
    expect_eq(cap.agentmem.PLAYBOOK_MAX_ENTRIES, @as(usize, 32));
    expect_eq(cap.agentmem.PLAYBOOK_MAX_STEPS, @as(usize, 12));
}

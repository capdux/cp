// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/reflexion.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Agent Reflexion Verbal Reinforcement Lesson Bank:
//   1. Multi-signal Top-K recall (@popCount domain overlap + error_kind + action_hash + posterior + recency)
//   2. Closed-loop Bayesian reinforcement feedback, auto-deprecation on low efficiency & recovery
//   3. 64-slot capacity saturation & priority eviction (retired > deprecated > lowest-eff / oldest active)
//   4. Recency half-life decay & monotonic clock rollback safety
//   5. Zero-alloc LLM Prompt injection block formatting & tiny-buffer protection
//   6. Invalid input/config guards, manual retirement & `cap.agentmem` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    REFLEXION_MAX_LESSONS,
    REFLEXION_TEXT_MAX_LEN,
    REFLEXION_BPS_SCALE,
    ReflexionErrorKind,
    ReflexionStatus,
    ReflexionConfig,
    ReflexionLesson,
    ReflexionRecallHit,
    ReflexionBank,
    formatLessonsPromptBlock,
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

test("Reflexion: multi-signal Top-K recall ranking (domain + error + action + posterior)") {
    const DOM_SQL: u64 = 1 << 0;
    const DOM_FS: u64 = 1 << 1;
    const DOM_NET: u64 = 1 << 2;

    let bank = ReflexionBank.init();
    // Lesson 1: exact match on domain, error_kind, and action_hash
    const id1 = bank.recordLesson(
        DOM_SQL | DOM_FS,
        ReflexionErrorKind.schema_mismatch,
        0xAA11,
        "Verify column names before executing SQL migration.",
        1000,
    ) ?? return;
    // Lesson 2: partial domain + same error_kind, different action
    const id2 = bank.recordLesson(
        DOM_SQL,
        ReflexionErrorKind.schema_mismatch,
        0xBB22,
        "Quote reserved SQL identifiers cleanly.",
        1000,
    ) ?? return;
    // Lesson 3: unrelated domain, error_kind, and action
    const id3 = bank.recordLesson(
        DOM_NET,
        ReflexionErrorKind.timeout,
        0xCC33,
        "Add jitter before retrying HTTP 503.",
        1000,
    ) ?? return;

    // Boost lesson 1 posterior efficiency
    * = bank.recordFeedback(id1, true, 1100);
    * = bank.recordFeedback(id1, true, 1200);

    let hits: [4]ReflexionRecallHit = [ReflexionRecallHit{}; 4];
    let hits_s: []ReflexionRecallHit = hits[0..];
    const n = bank.recallLessons(
        DOM_SQL | DOM_FS,
        ReflexionErrorKind.schema_mismatch,
        0xAA11,
        2000,
        2,
        *hits_s,
    );

    expect_eq(n, 2 as usize);
    expect_eq(hits[0].lesson_id, id1);
    expect_true(hits[0].exact_error_match && hits[0].exact_action_match);
    expect_eq(hits[0].domain_overlap_bits, 2 as u32);
    expect_eq(hits[0].posterior_bps, 7500 as u32); // (2 + 1) / (2 + 0 + 2) = 0.75
    expect_eq(hits[1].lesson_id, id2);
    expect_true(hits[0].score > hits[1].score);
    * = id3;
}

test("Reflexion: closed-loop Bayesian feedback, auto-deprecation & recovery") {
    let bank = ReflexionBank.init();
    const id = bank.recordLesson(1, ReflexionErrorKind.tool_failure, 0x10, "Retry with smaller batch.", 1000) ?? return;

    // Initial Laplace prior: (0 + 1) / (0 + 0 + 2) = 5000 bps
    const init_l = bank.getLesson(id) ?? ReflexionLesson{};
    expect_eq(init_l.posteriorBps(), 5000 as u32);
    expect_eq(init_l.status, ReflexionStatus.active);

    // 3 consecutive failures -> posterior = 1/5 = 2000 bps < 3500 -> auto-deprecated!
    expect_eq(bank.recordFeedback(id, false, 1100) ?? ReflexionStatus.retired, ReflexionStatus.active);
    expect_eq(bank.recordFeedback(id, false, 1200) ?? ReflexionStatus.retired, ReflexionStatus.active);
    expect_eq(bank.recordFeedback(id, false, 1300) ?? ReflexionStatus.retired, ReflexionStatus.deprecated);

    // Deprecated lesson is excluded from active recall
    let hits: [4]ReflexionRecallHit = [ReflexionRecallHit{}; 4];
    let hits_s: []ReflexionRecallHit = hits[0..];
    expect_eq(bank.recallLessons(1, ReflexionErrorKind.tool_failure, 0x10, 1400, 4, *hits_s), 0 as usize);

    // Positive feedback raises posterior back >= 3500 bps -> auto-reactivated!
    // After 2 helps: (2+1)/(2+3+2) = 3/7 = 4285 bps >= 3500
    * = bank.recordFeedback(id, true, 1500);
    expect_eq(bank.recordFeedback(id, true, 1600) ?? ReflexionStatus.retired, ReflexionStatus.active);
    expect_eq(bank.recallLessons(1, ReflexionErrorKind.tool_failure, 0x10, 1700, 4, *hits_s), 1 as usize);

    const st = bank.stats();
    expect_eq(st.total_auto_deprecations, 1 as u64);
    expect_eq(st.total_helped_feedback, 2 as u64);
    expect_eq(st.total_failed_feedback, 3 as u64);
}

test("Reflexion: 64-slot capacity saturation & priority eviction (retired > deprecated > weak active)") {
    let bank = ReflexionBank.init();
    let ids: [REFLEXION_MAX_LESSONS]u32 = [0; REFLEXION_MAX_LESSONS];

    let i: usize = 0;
    while (i < REFLEXION_MAX_LESSONS) : (i += 1) {
        ids[i] = bank.recordLesson(1, ReflexionErrorKind.other, i as u64, "Base lesson", (1000 + i) as u64) ?? return;
    }
    expect_eq(bank.stats().total_slots_used, REFLEXION_MAX_LESSONS);

    // Retire slot #20 and deprecate slot #10
    expect_true(bank.retireLesson(ids[20]));
    * = bank.recordFeedback(ids[10], false, 2000);
    * = bank.recordFeedback(ids[10], false, 2001);
    * = bank.recordFeedback(ids[10], false, 2002);

    // 65th lesson MUST evict the retired lesson (ids[20]) first
    const new1 = bank.recordLesson(1, ReflexionErrorKind.logic_error, 999, "Fresh lesson 1", 3000) ?? return;
    expect_true(bank.getLesson(ids[20]) == null);
    expect_true(bank.getLesson(ids[10]) != null);
    expect_true(bank.getLesson(new1) != null);

    // 66th lesson MUST evict the deprecated lesson (ids[10]) next
    const new2 = bank.recordLesson(1, ReflexionErrorKind.logic_error, 1000, "Fresh lesson 2", 3001) ?? return;
    expect_true(bank.getLesson(ids[10]) == null);
    expect_true(bank.getLesson(new2) != null);

    // 67th lesson evicts oldest/lowest-efficiency active lesson (ids[0], oldest last_used_ms=1000)
    * = bank.recordLesson(1, ReflexionErrorKind.logic_error, 1001, "Fresh lesson 3", 3002);
    expect_true(bank.getLesson(ids[0]) == null);
    expect_eq(bank.stats().total_evictions, 3 as u64);
}

test("Reflexion: recency half-life decay & monotonic clock rollback safety") {
    let cfg = ReflexionConfig.defaults();
    cfg.half_life_ms = 1000;
    let bank = ReflexionBank.initWithConfig(cfg) ?? return;

    const old_id = bank.recordLesson(1, ReflexionErrorKind.timeout, 0, "Old timeout lesson", 1000) ?? return;
    const fresh_id = bank.recordLesson(1, ReflexionErrorKind.timeout, 0, "Fresh timeout lesson", 10000) ?? return;

    let hits: [2]ReflexionRecallHit = [ReflexionRecallHit{}; 2];
    let hits_s: []ReflexionRecallHit = hits[0..];
    const n = bank.recallLessons(1, ReflexionErrorKind.timeout, 0, 10000, 2, *hits_s);
    expect_eq(n, 2 as usize);
    expect_eq(hits[0].lesson_id, fresh_id);
    expect_eq(hits[1].lesson_id, old_id);
    expect_true(hits[0].score > hits[1].score);

    // Clock rollback (now_ms < last_used_ms) must not underflow or produce NaN
    const n_back = bank.recallLessons(1, ReflexionErrorKind.timeout, 0, 500, 2, *hits_s);
    expect_eq(n_back, 2 as usize);
    expect_true(hits[0].score > 0.0);
}

test("Reflexion: zero-alloc LLM Prompt injection block formatting & tiny buffer guard") {
    let bank = ReflexionBank.init();
    const id1 = bank.recordLesson(1, ReflexionErrorKind.permission_denied, 0x55, "Check file ACL before writing.", 1000) ?? return;
    * = bank.recordFeedback(id1, true, 1100); // eff = 2/3 = 66%

    let hits: [2]ReflexionRecallHit = [ReflexionRecallHit{}; 2];
    let hits_s: []ReflexionRecallHit = hits[0..];
    const n = bank.recallLessons(1, ReflexionErrorKind.permission_denied, 0x55, 1200, 2, *hits_s);
    expect_eq(n, 1 as usize);

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const written = formatLessonsPromptBlock(hits[0..n], *buf_s) ?? 0;
    expect_true(written > 0);
    expect_true(containsSubstr(buf[0..written], "[Reflexion Lessons]\n"));
    expect_true(containsSubstr(buf[0..written], "1. (#1, eff=66%) Check file ACL before writing.\n"));

    // Empty hits slice writes 0 bytes cleanly
    expect_eq(formatLessonsPromptBlock(hits[0..0], *buf_s) ?? 99, 0 as usize);

    // Tiny buffer returns null safely
    let tiny: [12]u8 = [0; 12];
    let tiny_s: []u8 = tiny[0..];
    expect_true(formatLessonsPromptBlock(hits[0..n], *tiny_s) == null);
}

test("Reflexion: invalid config/text guards & cap.agentmem namespace facade parity") {
    let bad_cfg = ReflexionConfig.defaults();
    bad_cfg.half_life_ms = 0;
    expect_true(ReflexionBank.initWithConfig(bad_cfg) == null);

    let bank = cap.agentmem.ReflexionBank.init();
    expect_true(bank.recordLesson(1, ReflexionErrorKind.other, 0, "", 100) == null);

    let long_txt: [REFLEXION_TEXT_MAX_LEN + 2]u8 = [0x61; REFLEXION_TEXT_MAX_LEN + 2];
    expect_true(bank.recordLesson(1, ReflexionErrorKind.other, 0, long_txt[0..], 100) == null);

    let nul_txt: [4]u8 = [0x61, 0x00, 0x62, 0x63];
    expect_true(bank.recordLesson(1, ReflexionErrorKind.other, 0, nul_txt[0..], 100) == null);

    const ok_id = bank.recordLesson(1, cap.agentmem.ReflexionErrorKind.hallucination, 0x77, "Cite verified tool output only.", 100) ?? return;
    expect_true(bank.recordFeedback(9999, true, 200) == null);
    expect_eq(bank.retireLesson(9999), false);
    expect_true(bank.retireLesson(ok_id));
    expect_eq(bank.recordFeedback(ok_id, true, 300) ?? ReflexionStatus.active, ReflexionStatus.retired);
    expect_eq(REFLEXION_BPS_SCALE, 10000 as u32);
}

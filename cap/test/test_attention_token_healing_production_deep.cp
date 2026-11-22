// Origin: Pure-CP Stdlib Test Suite (lib/attention/token_healing.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Prompt Boundary Token Healing & Byte-Prefix Logits Masking:
//   1. Classic BPE boundary artifact healing (StrictSuperset: "http:" -> "http://") + -Inf logits mask
//   2. Multi-token suffix backtracking ("http" + ":" -> "http:") with automatic depth fallback
//   3. Multi-step continuous healing state machine (MultiStepPrefix: "ht" -> "tp" -> "://") + mask release
//   4. require_strict_extension flag: triggers only when strictly longer candidate exists
//   5. Cross-word (>128 tokens) HealCandidateMask bitset & hardware @popCount verification
//   6. Session state machine rejection guards (incompatible token, unknown ID, partial token in StrictSuperset)
//   7. Full boundary guards (empty prompt, unknown/OOB tokens, empty vocab, sparse setToken)
//   8. Logits undersized guard (no mutation + null return), NaN/Inf sanitization, and oversized tail masking

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HEAL_MAX_VOCAB_TOKENS,
    HEAL_MASK_WORDS,
    HEAL_MAX_BYTE_POOL,
    HEAL_MAX_PREFIX_BYTES,
    HEAL_MAX_BACKTRACK_TOKENS,
    HealMatchMode,
    HealSessionState,
    TokenHealConfig,
    HealVocabTable,
    HealCandidateMask,
    TokenHealPlan,
    HealStepOutcome,
    TokenHealingSession,
    buildHealingCandidateMask,
    applyHealingLogitsMaskInPlace,
    planTokenHealing,
    planTokenHealingWithConfig,
} from "cap:attention";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn isNegInfF32(x: f32): bool {
    return std.math.isInf(x) && x < 0.0;
}

fn argmaxF32(xs: []const f32): usize {
    let best_idx: usize = 0;
    let best_val: f32 = xs[0];
    let i: usize = 1;
    while (i < xs.len) : (i += 1) {
        if (xs[i] > best_val) {
            best_val = xs[i];
            best_idx = i;
        }
    }
    return best_idx;
}

// ============================================================================
// Test 1: Classic BPE Boundary Artifact Healing ("http:" -> "http://")
// ============================================================================

fn testStrictSupersetHttpBoundaryHealing()! {
    let vocab = HealVocabTable.init();
    const id_visit: u32 = vocab.addToken("Visit ") ?? 999;     // 0
    const id_http: u32 = vocab.addToken("http") ?? 999;        // 1
    const id_http_colon: u32 = vocab.addToken("http:") ?? 999; // 2
    const id_http_url: u32 = vocab.addToken("http://") ?? 999; // 3
    const id_https: u32 = vocab.addToken("https://") ?? 999;   // 4
    const id_slash: u32 = vocab.addToken("//") ?? 999;         // 5
    const id_ftp: u32 = vocab.addToken("ftp://") ?? 999;       // 6

    expect_eq(vocab.vocabSize(), 7);
    expect_eq(id_http_colon, 2);
    expect_eq(id_http_url, 3);

    // Prompt ends with "http:" (token 2)
    const prompt: [2]u32 = [id_visit, id_http_colon];
    const plan: TokenHealPlan = cap.attention.planTokenHealing(prompt[0..], vocab);

    expect_eq(plan.no_heal_needed, false);
    expect_eq(plan.trimmed_prompt_len, 1);
    expect_eq(plan.backtrack_count, 1);
    expect_eq(bytesEq(plan.prefixSlice(), "http:"), true);
    // StrictSuperset candidates starting with "http:" are {"http:", "http://"} -> 2 candidates, 1 strict extension
    expect_eq(plan.candidate_count, 2);
    expect_eq(plan.strict_extension_count, 1);

    // Build candidate mask & verify @popCount
    const mask = buildHealingCandidateMask(vocab, plan.prefixSlice(), HealMatchMode.StrictSuperset);
    expect_eq(mask.allowedCount(), 2);
    expect_eq(mask.isAllowed(id_http_colon as usize), true);
    expect_eq(mask.isAllowed(id_http_url as usize), true);
    expect_eq(mask.isAllowed(id_http as usize), false);
    expect_eq(mask.isAllowed(id_https as usize), false);
    expect_eq(mask.isAllowed(id_slash as usize), false);

    // Logits strongly favor "https://" (token 4) before masking, and "http://" (token 3) > "http:" (token 2)
    let logits_buf: [7]f32 = [1.0, 2.0, 3.5, 4.2, 9.9, 5.0, 0.5];
    let logits_s: []f32 = logits_buf[0..];
    const kept_opt = applyHealingLogitsMaskInPlace(*logits_s, mask);
    expect_eq(kept_opt != null, true);
    if (kept := kept_opt) {
        expect_eq(kept, 2);
    }

    // Forbidden tokens must be -inf; allowed tokens preserve original logits
    expect_eq(isNegInfF32(logits_buf[0]), true);
    expect_eq(isNegInfF32(logits_buf[1]), true);
    expect_eq(isNegInfF32(logits_buf[4]), true);
    expect_eq(isNegInfF32(logits_buf[5]), true);
    expect_eq(isNegInfF32(logits_buf[6]), true);
    expect_eq(logits_buf[2] == 3.5, true);
    expect_eq(logits_buf[3] == 4.2, true);
    expect_eq(argmaxF32(logits_buf[0..]), id_http_url as usize);

    // Advance session with healed token "http://" -> completes in 1 step and releases mask
    let session = TokenHealingSession.fromPlan(plan, HealMatchMode.StrictSuperset);
    expect_eq(session.isActive(), true);
    const step = session.advance(id_http_url, vocab);
    expect_eq(step.accepted, true);
    expect_eq(step.completed, true);
    expect_eq(step.consumed_prefix_bytes, 5);
    expect_eq(step.remaining_prefix_len, 0);
    expect_eq(step.extra_healed_bytes, 2);
    expect_eq(session.isActive(), false);
    expect_eq(session.isCompleted(), true);

    // Subsequent step must leave logits completely untouched
    let next_logits: [7]f32 = [0.1, 0.2, 0.3, 0.4, 8.8, 0.6, 0.7];
    let next_s: []f32 = next_logits[0..];
    const next_kept = session.applyStepLogitsMaskInPlace(vocab, *next_s);
    expect_eq(next_kept != null, true);
    if (nk := next_kept) {
        expect_eq(nk, 7);
    }
    expect_eq(next_logits[4] == 8.8, true);
    expect_eq(isNegInfF32(next_logits[0]), false);
}

// ============================================================================
// Test 2: Multi-Token Suffix Backtracking & Depth Fallback
// ============================================================================

fn testMultiTokenBacktrackingAndFallback()! {
    let vocab = HealVocabTable.init();
    const t_the: u32 = vocab.addToken("The ") ?? 999;          // 0
    const t_url: u32 = vocab.addToken("url ") ?? 999;          // 1
    const t_http: u32 = vocab.addToken("http") ?? 999;         // 2
    const t_colon: u32 = vocab.addToken(":") ?? 999;           // 3
    const t_http_url: u32 = vocab.addToken("http://") ?? 999;  // 4
    const t_http_ex: u32 = vocab.addToken("http://ex") ?? 999; // 5
    const t_colon_sl: u32 = vocab.addToken("://") ?? 999;      // 6

    const prompt: [4]u32 = [t_the, t_url, t_http, t_colon];

    // Single-token backtrack only sees ":"
    const plan1 = planTokenHealing(prompt[0..], vocab);
    expect_eq(plan1.no_heal_needed, false);
    expect_eq(plan1.backtrack_count, 1);
    expect_eq(plan1.trimmed_prompt_len, 3);
    expect_eq(bytesEq(plan1.prefixSlice(), ":"), true);
    expect_eq(plan1.candidate_count, 2); // ":" and "://"
    * = t_colon_sl;

    // Multi-token backtrack (max=3): k=2 ("http:") matches {"http://", "http://ex"},
    // while k=3 ("url http:") has 0 matches -> keeps best valid depth k=2!
    const cfg3 = TokenHealConfig{
        max_backtrack_tokens: 3,
        mode: HealMatchMode.StrictSuperset,
        require_strict_extension: true,
    };
    const plan3 = planTokenHealingWithConfig(prompt[0..], vocab, cfg3);
    expect_eq(plan3.no_heal_needed, false);
    expect_eq(plan3.backtrack_count, 2);
    expect_eq(plan3.trimmed_prompt_len, 2);
    expect_eq(bytesEq(plan3.prefixSlice(), "http:"), true);
    expect_eq(plan3.candidate_count, 2);
    expect_eq(plan3.strict_extension_count, 2);

    const mask3 = buildHealingCandidateMask(vocab, plan3.prefixSlice(), HealMatchMode.StrictSuperset);
    expect_eq(mask3.isAllowed(t_http_url as usize), true);
    expect_eq(mask3.isAllowed(t_http_ex as usize), true);
    expect_eq(mask3.isAllowed(t_http as usize), false);
    expect_eq(mask3.isAllowed(t_colon as usize), false);
}

// ============================================================================
// Test 3: Multi-Step Continuous Healing State Machine ("ht" -> "tp" -> "://")
// ============================================================================

fn testMultiStepContinuousHealingSession()! {
    let vocab = HealVocabTable.init();
    const t_go: u32 = vocab.addToken("go ") ?? 999;           // 0
    const t_http_c: u32 = vocab.addToken("http:") ?? 999;     // 1
    const t_ht: u32 = vocab.addToken("ht") ?? 999;            // 2 (partial prefix 1)
    const t_tp: u32 = vocab.addToken("tp") ?? 999;            // 3 (partial prefix 2)
    const t_colon_sl: u32 = vocab.addToken("://") ?? 999;     // 4 (completes ":" with "+//")
    const t_tp_sl: u32 = vocab.addToken("tp://") ?? 999;      // 5 (alternate 2-step completion)
    const t_bad: u32 = vocab.addToken("ftp://") ?? 999;       // 6 (never allowed)

    const prompt: [2]u32 = [t_go, t_http_c];
    let session = TokenHealingSession.init(prompt[0..], vocab, TokenHealConfig.multiStep(1));

    expect_eq(session.isActive(), true);
    expect_eq(session.trimmedPromptLen(), 1);
    expect_eq(bytesEq(session.remainingPrefix(), "http:"), true);

    // Step 1 mask: allows "http:" (superset) and "ht" (strict prefix of "http:")
    const m1 = session.buildStepMask(vocab);
    expect_eq(m1.allowedCount(), 2);
    expect_eq(m1.isAllowed(t_http_c as usize), true);
    expect_eq(m1.isAllowed(t_ht as usize), true);
    expect_eq(m1.isAllowed(t_tp as usize), false);
    expect_eq(m1.isAllowed(t_bad as usize), false);

    // Emit partial token "ht" -> remaining prefix advances to "tp:"
    const s1 = session.advance(t_ht, vocab);
    expect_eq(s1.accepted, true);
    expect_eq(s1.completed, false);
    expect_eq(s1.consumed_prefix_bytes, 2);
    expect_eq(s1.remaining_prefix_len, 3);
    expect_eq(s1.extra_healed_bytes, 0);
    expect_eq(session.isActive(), true);
    expect_eq(bytesEq(session.remainingPrefix(), "tp:"), true);

    // Step 2 mask: allows "tp" and "tp://", forbids "ht" and "http:"
    let l2: [7]f32 = [1.0, 9.0, 8.0, 5.0, 7.0, 4.0, 9.5];
    let l2_s: []f32 = l2[0..];
    const k2 = session.applyStepLogitsMaskInPlace(vocab, *l2_s);
    expect_eq(k2 != null, true);
    if (kept2 := k2) {
        expect_eq(kept2, 2);
    }
    expect_eq(isNegInfF32(l2[t_ht as usize]), true);
    expect_eq(isNegInfF32(l2[t_http_c as usize]), true);
    expect_eq(l2[t_tp as usize] == 5.0, true);
    expect_eq(l2[t_tp_sl as usize] == 4.0, true);
    expect_eq(argmaxF32(l2[0..]), t_tp as usize);

    // Emit partial token "tp" -> remaining prefix advances to ":"
    const s2 = session.advance(t_tp, vocab);
    expect_eq(s2.accepted, true);
    expect_eq(s2.completed, false);
    expect_eq(s2.consumed_prefix_bytes, 2);
    expect_eq(s2.remaining_prefix_len, 1);
    expect_eq(bytesEq(session.remainingPrefix(), ":"), true);

    // Step 3 mask: only "://" matches ":"
    const m3 = session.buildStepMask(vocab);
    expect_eq(m3.allowedCount(), 1);
    expect_eq(m3.isAllowed(t_colon_sl as usize), true);

    // Emit "://" -> completes healing with 2 extra healed bytes ("//")
    const s3 = session.advance(t_colon_sl, vocab);
    expect_eq(s3.accepted, true);
    expect_eq(s3.completed, true);
    expect_eq(s3.consumed_prefix_bytes, 1);
    expect_eq(s3.remaining_prefix_len, 0);
    expect_eq(s3.extra_healed_bytes, 2);
    expect_eq(session.isActive(), false);
    expect_eq(session.isCompleted(), true);
    expect_eq(session.steps_taken, 3);
    expect_eq(session.remainingPrefix().len, 0);

    // Step 4: mask is released, all tokens allowed
    const m4 = session.buildStepMask(vocab);
    expect_eq(m4.allowedCount(), 7);
}

// ============================================================================
// Test 4: require_strict_extension Flag & Safe Degradation
// ============================================================================

fn testRequireStrictExtensionDegradesSafely()! {
    let vocab = HealVocabTable.init();
    const t_hello: u32 = vocab.addToken("hello") ?? 999; // 0
    const t_xyz: u32 = vocab.addToken("xyz") ?? 999;     // 1 (no longer token starts with "xyz")

    const prompt: [2]u32 = [t_hello, t_xyz];

    // Without require_strict_extension: exact self-match is allowed
    const plan_loose = planTokenHealing(prompt[0..], vocab);
    expect_eq(plan_loose.no_heal_needed, false);
    expect_eq(plan_loose.candidate_count, 1);
    expect_eq(plan_loose.strict_extension_count, 0);

    // With require_strict_extension: degrades safely to no_heal_needed
    const cfg_strict = TokenHealConfig{
        max_backtrack_tokens: 1,
        mode: HealMatchMode.StrictSuperset,
        require_strict_extension: true,
    };
    const plan_strict = planTokenHealingWithConfig(prompt[0..], vocab, cfg_strict);
    expect_eq(plan_strict.no_heal_needed, true);
    expect_eq(plan_strict.backtrack_count, 0);
    expect_eq(plan_strict.trimmed_prompt_len, 2);
    expect_eq(plan_strict.heal_prefix_len, 0);

    let session = TokenHealingSession.fromPlan(plan_strict, HealMatchMode.StrictSuperset);
    expect_eq(session.state == HealSessionState.Inactive, true);
    expect_eq(session.isActive(), false);
}

// ============================================================================
// Test 5: Cross-64-bit-Word Candidate Bitset & Hardware @popCount (>128 tokens)
// ============================================================================

fn testCrossWordBitsetAndPopCount()! {
    let vocab = HealVocabTable.init();
    let i: usize = 0;
    while (i < 135) : (i += 1) {
        // Place matching "pre_" tokens across word 0 (0, 63), word 1 (64, 127), word 2 (128, 134)
        if (i == 0 || i == 63 || i == 64 || i == 127 || i == 128 || i == 134) {
            * = vocab.addToken("pre_match");
        } else {
            * = vocab.addToken("other");
        }
    }
    expect_eq(vocab.vocabSize(), 135);

    const mask = buildHealingCandidateMask(vocab, "pre_", HealMatchMode.StrictSuperset);
    expect_eq(mask.allowedCount(), 6);
    expect_eq(mask.firstAllowedToken() ?? 999, 0);
    expect_eq(mask.isAllowed(0), true);
    expect_eq(mask.isAllowed(63), true);
    expect_eq(mask.isAllowed(64), true);
    expect_eq(mask.isAllowed(127), true);
    expect_eq(mask.isAllowed(128), true);
    expect_eq(mask.isAllowed(134), true);
    expect_eq(mask.isAllowed(1), false);
    expect_eq(mask.isAllowed(65), false);
    expect_eq(mask.isAllowed(135), false);

    // Apply mask to 135 logits and verify exact kept count
    let logits: [135]f32 = [1.5; 135];
    let logits_s: []f32 = logits[0..];
    const kept = applyHealingLogitsMaskInPlace(*logits_s, mask);
    expect_eq(kept != null, true);
    if (k := kept) {
        expect_eq(k, 6);
    }
    expect_eq(logits[63] == 1.5, true);
    expect_eq(logits[64] == 1.5, true);
    expect_eq(logits[128] == 1.5, true);
    expect_eq(isNegInfF32(logits[62]), true);
    expect_eq(isNegInfF32(logits[129]), true);

    // Mutate mask: forbid 0 -> firstAllowedToken becomes 63, allowedCount becomes 5
    let m_mut = mask;
    expect_eq(m_mut.forbidToken(0), true);
    expect_eq(m_mut.allowedCount(), 5);
    expect_eq(m_mut.firstAllowedToken() ?? 999, 63);
    expect_eq(m_mut.forbidToken(500), false);
}

// ============================================================================
// Test 6: Session State Machine Rejection Guards
// ============================================================================

fn testSessionRejectionGuards()! {
    let vocab = HealVocabTable.init();
    const t_http_c: u32 = vocab.addToken("http:") ?? 999; // 0
    const t_ht: u32 = vocab.addToken("ht") ?? 999;        // 1
    const t_ftp: u32 = vocab.addToken("ftp:") ?? 999;     // 2

    const prompt: [1]u32 = [t_http_c];

    // StrictSuperset rejects partial prefix token "ht"
    let s_strict = TokenHealingSession.init(prompt[0..], vocab, TokenHealConfig.defaults());
    expect_eq(s_strict.isActive(), true);
    const bad1 = s_strict.advance(t_ht, vocab);
    expect_eq(bad1.accepted, false);
    expect_eq(bad1.completed, false);
    expect_eq(s_strict.state == HealSessionState.Failed, true);

    // MultiStepPrefix rejects non-matching token "ftp:"
    let s_multi = TokenHealingSession.init(prompt[0..], vocab, TokenHealConfig.multiStep(1));
    const bad2 = s_multi.advance(t_ftp, vocab);
    expect_eq(bad2.accepted, false);
    expect_eq(s_multi.state == HealSessionState.Failed, true);

    // Unknown token ID also transitions safely to Failed
    let s_unk = TokenHealingSession.init(prompt[0..], vocab, TokenHealConfig.multiStep(1));
    const bad3 = s_unk.advance(999, vocab);
    expect_eq(bad3.accepted, false);
    expect_eq(s_unk.state == HealSessionState.Failed, true);
}

// ============================================================================
// Test 7: Full Boundary Guards (Empty Prompt, Unknown Token, Sparse Vocab)
// ============================================================================

fn testBoundaryGuardsEmptyAndUnknownTokens()! {
    let vocab = HealVocabTable.init();

    // Empty vocab & empty prompt
    const empty_prompt: [0]u32 = [];
    const p_empty = planTokenHealing(empty_prompt[0..], vocab);
    expect_eq(p_empty.no_heal_needed, true);
    expect_eq(p_empty.trimmed_prompt_len, 0);
    expect_eq(p_empty.backtrack_count, 0);

    // Sparse registration via setToken (skipping ID 1)
    expect_eq(vocab.setToken(0, "abc"), true);
    expect_eq(vocab.setToken(2, "abcdef"), true);
    expect_eq(vocab.setToken(HEAL_MAX_VOCAB_TOKENS as u32, "oob"), false);
    expect_eq(vocab.isValidToken(0), true);
    expect_eq(vocab.isValidToken(1), false);
    expect_eq(vocab.isValidToken(2), true);
    expect_eq(vocab.tokenBytes(1) == null, true);
    expect_eq(vocab.tokenBytes(9999) == null, true);

    // Trailing unknown token (ID 1) -> safe degradation to no_heal_needed
    const p_unk_tail_arr: [2]u32 = [0, 1];
    const p_unk_tail = planTokenHealing(p_unk_tail_arr[0..], vocab);
    expect_eq(p_unk_tail.no_heal_needed, true);
    expect_eq(p_unk_tail.trimmed_prompt_len, 2);
    expect_eq(p_unk_tail.backtrack_count, 0);

    // Unknown token before valid trailing token: multi-token backtrack stops at k=1 safely
    const p_unk_mid_arr: [2]u32 = [1, 0];
    const p_unk_mid = planTokenHealingWithConfig(
        p_unk_mid_arr[0..],
        vocab,
        TokenHealConfig{
            max_backtrack_tokens: 2,
            mode: HealMatchMode.StrictSuperset,
            require_strict_extension: true,
        },
    );
    expect_eq(p_unk_mid.no_heal_needed, false);
    expect_eq(p_unk_mid.backtrack_count, 1);
    expect_eq(p_unk_mid.trimmed_prompt_len, 1);
    expect_eq(bytesEq(p_unk_mid.prefixSlice(), "abc"), true);

    // Sanity check exported constants
    expect_eq(HEAL_MASK_WORDS * 64, HEAL_MAX_VOCAB_TOKENS);
    expect_eq(HEAL_MAX_BYTE_POOL >= 4096, true);
    expect_eq(HEAL_MAX_PREFIX_BYTES >= 32, true);
    expect_eq(HEAL_MAX_BACKTRACK_TOKENS >= 4, true);
}

// ============================================================================
// Test 8: Logits Undersized Guard, NaN/Inf Sanitization & Tail Masking
// ============================================================================

fn testLogitsGuardsAndSanitization()! {
    let vocab = HealVocabTable.init();
    * = vocab.addToken("a");   // 0
    * = vocab.addToken("ab");  // 1
    * = vocab.addToken("abc"); // 2
    * = vocab.addToken("xyz"); // 3

    const mask = buildHealingCandidateMask(vocab, "ab", HealMatchMode.StrictSuperset);
    expect_eq(mask.allowedCount(), 2); // tokens 1 and 2

    // Undersized logits (< vocab_size): returns null without mutating logits
    let short_logits: [2]f32 = [1.25, 2.5];
    let short_s: []f32 = short_logits[0..];
    expect_eq(applyHealingLogitsMaskInPlace(*short_s, mask) == null, true);
    expect_eq(short_logits[0] == 1.25, true);
    expect_eq(short_logits[1] == 2.5, true);

    // Session also rejects undersized logits cleanly
    const prompt: [1]u32 = [1];
    const session = TokenHealingSession.init(prompt[0..], vocab, TokenHealConfig.defaults());
    expect_eq(session.applyStepLogitsMaskInPlace(vocab, *short_s) == null, true);

    // Oversized logits (len 6 > vocab_size 4) + NaN on an allowed token (token 1):
    // token 1 (NaN) -> sanitized to -inf, token 2 (3.0) -> kept, tail [4, 5] -> masked to -inf
    let ext_logits: [6]f32 = [5.0, std.math.nan(f32), 3.0, 7.0, 9.0, 10.0];
    let ext_s: []f32 = ext_logits[0..];
    const kept = applyHealingLogitsMaskInPlace(*ext_s, mask);
    expect_eq(kept != null, true);
    if (k := kept) {
        expect_eq(k, 1);
    }
    expect_eq(isNegInfF32(ext_logits[0]), true);
    expect_eq(isNegInfF32(ext_logits[1]), true);
    expect_eq(ext_logits[2] == 3.0, true);
    expect_eq(isNegInfF32(ext_logits[3]), true);
    expect_eq(isNegInfF32(ext_logits[4]), true);
    expect_eq(isNegInfF32(ext_logits[5]), true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("attention.token_healing: strict superset BPE boundary healing (http: -> http://)", testStrictSupersetHttpBoundaryHealing);
test("attention.token_healing: multi-token suffix backtracking & depth fallback", testMultiTokenBacktrackingAndFallback);
test("attention.token_healing: multi-step continuous healing state machine & mask release", testMultiStepContinuousHealingSession);
test("attention.token_healing: require_strict_extension safe degradation", testRequireStrictExtensionDegradesSafely);
test("attention.token_healing: cross-word (>128 tokens) bitset & hardware @popCount", testCrossWordBitsetAndPopCount);
test("attention.token_healing: session state machine rejection guards", testSessionRejectionGuards);
test("attention.token_healing: empty prompt, unknown token & sparse vocab guards", testBoundaryGuardsEmptyAndUnknownTokens);
test("attention.token_healing: logits undersized guard, NaN sanitization & tail masking", testLogitsGuardsAndSanitization);

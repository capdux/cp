// Source: cp/cap/test/test_feature_production_deep.cp
// Deep Production Verification Suite for cap:feature & cap.feature
// Covers:
//   1. Deterministic FNV-1a consistent bucketing uniformity (1000 users @ 25%) & strict monotonicity (25% -> 50%)
//   2. TargetingRule multi-operator matching (Equals, InList, Contains, NumericGt) priority & setKillSwitch circuit breaker
//   3. Multi-variant A/B/n experimentation distribution (50% / 30% / 20% = 10000 bps) & invalid weight/percentage rejection
//   4. Missing flag fallback (FlagNotFound) & toJsonSnapshot() validated via cap:json
//   5. `import cap from "cap"; cap.feature` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON, JsonTag } from "cap:json";
import cap from "cap";
import {
    BUCKET_SCALE,
    FeatureError,
    MatchOp,
    EvalReason,
    EvalContext,
    TargetingRule,
    VariantWeight,
    FeatureEngine,
} from "cap:feature";

// ============================================================================
// Test 1: Consistent Bucketing Uniformity & Monotonicity (25% -> 50%)
// ============================================================================

fn testBucketingUniformityAndMonotonicity(): !void {
    const alloc = std.heap.page_allocator;
    let engine = FeatureEngine.init(alloc);
    defer engine.deinit();

    engine.defineBoolFlag("new_checkout", false, 25)?;

    let ctx = EvalContext.init(alloc, "init")?;
    defer ctx.deinit();

    let included_at_25: [1000]bool = [false; 1000];
    let count_25: usize = 0;

    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        const uid = `user-${i}`;
        ctx.setUserId(uid)?;

        const d1 = engine.evaluateBool("new_checkout", ctx);
        // Determinism check: repeated evaluation yields identical result & bucket
        const d2 = engine.evaluateBool("new_checkout", ctx);
        expect_eq(d1.enabled, d2.enabled);
        expect_eq(d1.bucket, d2.bucket);
        expect_eq(d1.bucket < BUCKET_SCALE, true);

        if (d1.enabled) {
            expect_eq(d1.reason == .RolloutIncluded, true);
            included_at_25[i] = true;
            count_25 += 1;
        } else {
            expect_eq(d1.reason == .RolloutExcluded, true);
        }
    }

    // Statistical uniformity: 25% of 1000 should be comfortably within [190, 310]
    expect_eq(count_25 >= 190 && count_25 <= 310, true);

    // Increase rollout to 50%: every user included at 25% MUST remain included
    engine.setRollout("new_checkout", 50)?;
    let count_50: usize = 0;
    let j: usize = 0;
    while (j < 1000) : (j += 1) {
        const uid = `user-${j}`;
        ctx.setUserId(uid)?;

        const d = engine.evaluateBool("new_checkout", ctx);
        if (included_at_25[j]) {
            expect_eq(d.enabled, true);
            expect_eq(d.reason == .RolloutIncluded, true);
        }
        if (d.enabled) {
            count_50 += 1;
        }
    }
    expect_eq(count_50 >= 430 && count_50 <= 570, true);
}

// ============================================================================
// Test 2: TargetingRule Multi-Operators Priority & KillSwitch Circuit Breaker
// ============================================================================

fn testTargetingRulesAndKillSwitch(): !void {
    const alloc = std.heap.page_allocator;
    let engine = FeatureEngine.init(alloc);
    defer engine.deinit();

    // 0% static disabled flag, overridden by targeting rules
    engine.defineBoolFlag("ai_copilot", false, 0)?;

    engine.addRule("ai_copilot", TargetingRule{
        attr_key: "role",
        op: .Equals,
        value: "admin",
        enabled_result: true,
    })?;
    engine.addRule("ai_copilot", TargetingRule{
        attr_key: "region",
        op: .InList,
        value: "us-east, eu-west, ap-south",
        enabled_result: true,
    })?;
    engine.addRule("ai_copilot", TargetingRule{
        attr_key: "email",
        op: .Contains,
        value: "@cp-lang.org",
        enabled_result: true,
    })?;
    engine.addRule("ai_copilot", TargetingRule{
        attr_key: "score",
        op: .NumericGt,
        value: "90.5",
        enabled_result: true,
    })?;

    // 1. Equals match
    let c1 = EvalContext.init(alloc, "u-1")?;
    defer c1.deinit();
    c1.setAttr("role", "admin")?;
    const r1 = engine.evaluateBool("ai_copilot", c1);
    expect_eq(r1.enabled, true);
    expect_eq(r1.reason == .RuleMatch, true);

    // 2. InList match (with whitespace trimming)
    let c2 = EvalContext.init(alloc, "u-2")?;
    defer c2.deinit();
    c2.setAttr("region", "eu-west")?;
    const r2 = engine.evaluateBool("ai_copilot", c2);
    expect_eq(r2.enabled, true);
    expect_eq(r2.reason == .RuleMatch, true);

    // 3. Contains match
    let c3 = EvalContext.init(alloc, "u-3")?;
    defer c3.deinit();
    c3.setAttr("email", "alice@cp-lang.org")?;
    const r3 = engine.evaluateBool("ai_copilot", c3);
    expect_eq(r3.enabled, true);
    expect_eq(r3.reason == .RuleMatch, true);

    // 4. NumericGt match
    let c4 = EvalContext.init(alloc, "u-4")?;
    defer c4.deinit();
    c4.setAttr("score", "95.2")?;
    const r4 = engine.evaluateBool("ai_copilot", c4);
    expect_eq(r4.enabled, true);
    expect_eq(r4.reason == .RuleMatch, true);

    // Non-matching user falls back to StaticDisabled (0%, default_enabled = false)
    let c5 = EvalContext.init(alloc, "u-5")?;
    defer c5.deinit();
    c5.setAttr("score", "80.0")?;
    const r5 = engine.evaluateBool("ai_copilot", c5);
    expect_eq(r5.enabled, false);
    expect_eq(r5.reason == .StaticDisabled, true);

    // Activate KillSwitch: immediately overrides ALL targeting rules (even admin!)
    engine.setKillSwitch("ai_copilot", true)?;
    const killed_res = engine.evaluateBool("ai_copilot", c1);
    expect_eq(killed_res.enabled, false);
    expect_eq(killed_res.reason == .KillSwitch, true);

    // Deactivate KillSwitch: targeting rule resumes
    engine.setKillSwitch("ai_copilot", false)?;
    const resumed_res = engine.evaluateBool("ai_copilot", c1);
    expect_eq(resumed_res.enabled, true);
    expect_eq(resumed_res.reason == .RuleMatch, true);
}

// ============================================================================
// Test 3: Multi-Variant A/B/n Experimentation & Invalid Input Guards
// ============================================================================

fn testExperimentsAndValidation(): !void {
    const alloc = std.heap.page_allocator;
    let engine = FeatureEngine.init(alloc);
    defer engine.deinit();

    // Guard 1: Invalid percentage (> 100)
    let pct_err = false;
    engine.defineBoolFlag("bad_pct", false, 101) catch e => {
        if (e == FeatureError.InvalidPercentage) pct_err = true;
    };
    expect_eq(pct_err, true);

    // Guard 2: Invalid weights (sum != 10000)
    const bad_variants: [2]VariantWeight = [
        VariantWeight{ name: "control", weight_bps: 5000 },
        VariantWeight{ name: "treatment", weight_bps: 4000 },
    ];
    let weight_err = false;
    engine.defineExperiment("bad_exp", bad_variants[0..], "control") catch e => {
        if (e == FeatureError.InvalidWeights) weight_err = true;
    };
    expect_eq(weight_err, true);

    // Valid 3-way AI prompt experiment: 50% / 30% / 20% = 10000 bps
    const variants: [3]VariantWeight = [
        VariantWeight{ name: "control", weight_bps: 5000 },
        VariantWeight{ name: "prompt_v2", weight_bps: 3000 },
        VariantWeight{ name: "prompt_v3", weight_bps: 2000 },
    ];
    engine.defineExperiment("llm_prompt_exp", variants[0..], "control")?;

    let ctx = EvalContext.init(alloc, "u")?;
    defer ctx.deinit();

    let c_ctrl: usize = 0;
    let c_v2: usize = 0;
    let c_v3: usize = 0;

    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        const uid = `exp-user-${i}`;
        ctx.setUserId(uid)?;

        const decision = engine.evaluateVariant("llm_prompt_exp", ctx);
        expect_eq(decision.enabled, true);
        expect_eq(decision.reason == .RolloutIncluded, true);
        if (std.mem.eql(u8, decision.variant, "control")) {
            c_ctrl += 1;
        } else if (std.mem.eql(u8, decision.variant, "prompt_v2")) {
            c_v2 += 1;
        } else if (std.mem.eql(u8, decision.variant, "prompt_v3")) {
            c_v3 += 1;
        } else {
            expect_eq(false, true);
        }
    }

    expect_eq(c_ctrl + c_v2 + c_v3, 1000);
    expect_eq(c_ctrl >= 430 && c_ctrl <= 570, true);
    expect_eq(c_v2 >= 240 && c_v2 <= 360, true);
    expect_eq(c_v3 >= 150 && c_v3 <= 250, true);
}

// ============================================================================
// Test 4: Missing Flag Fallback (FlagNotFound) & toJsonSnapshot via cap:json
// ============================================================================

fn testMissingFlagAndJsonSnapshot(): !void {
    const alloc = std.heap.page_allocator;
    let engine = FeatureEngine.init(alloc);
    defer engine.deinit();

    let ctx = EvalContext.init(alloc, "anon-1")?;
    defer ctx.deinit();

    const missing_bool = engine.evaluateBoolWithDefault("does_not_exist", ctx, true);
    expect_eq(missing_bool.enabled, true);
    expect_eq(missing_bool.reason == .FlagNotFound, true);

    const missing_var = engine.evaluateVariantWithDefault("missing_exp", ctx, "fallback_v1");
    expect_eq(missing_var.enabled, false);
    expect_eq(std.mem.eql(u8, missing_var.variant, "fallback_v1"), true);
    expect_eq(missing_var.reason == .FlagNotFound, true);

    // Populate flags and export JSON snapshot
    engine.defineBoolFlag("dark_mode", true, 75)?;
    const variants: [2]VariantWeight = [
        VariantWeight{ name: "a", weight_bps: 6000 },
        VariantWeight{ name: "b", weight_bps: 4000 },
    ];
    engine.defineExperiment("ranker_ab", variants[0..], "a")?;

    const snap = engine.toJsonSnapshot()?;
    defer alloc.free(snap);

    let json_arena = std.heap.ArenaAllocator.init(alloc);
    defer json_arena.deinit();
    const parsed = JSON.parse(json_arena.allocator(), snap)?;
    expect_eq(parsed.tag == JsonTag.Object, true);

    const flags_val = parsed.get("flags") ?? return error.InvalidSnapshot;
    expect_eq(flags_val.tag == JsonTag.Array, true);
    expect_eq(flags_val.len(), 2);

    const first_flag = flags_val.getIndex(0) ?? return error.InvalidSnapshot;
    const first_key = first_flag.get("key") ?? return error.InvalidSnapshot;
    expect_eq(std.mem.eql(u8, first_key.asString() ?? "", "dark_mode"), true);
}

// ============================================================================
// Test 5: Dynamic Lazy Namespace Mounting via `import cap from "cap"; cap.feature`
// ============================================================================

fn testCapFeatureDynamicMount(): !void {
    const alloc = std.heap.page_allocator;
    let engine = cap.feature.FeatureEngine.init(alloc);
    defer engine.deinit();

    expect_eq(cap.feature.BUCKET_SCALE, 10000);

    engine.defineBoolFlag("edge_cache", false, 100)?;
    let ctx = cap.feature.EvalContext.init(alloc, "edge-user-99")?;
    defer ctx.deinit();

    const decision = engine.evaluateBool("edge_cache", ctx);
    expect_eq(decision.enabled, true);
    expect_eq(decision.reason == cap.feature.EvalReason.RolloutIncluded, true);
    expect_eq(engine.flagCount(), 1);
}

fn main() {
    test("1. Deterministic FNV-1a bucketing uniformity & monotonicity (25% -> 50%)", () => {
        testBucketingUniformityAndMonotonicity()?;
    });
    test("2. TargetingRule multi-operators priority & setKillSwitch circuit breaker", () => {
        testTargetingRulesAndKillSwitch()?;
    });
    test("3. Multi-variant A/B/n experimentation & invalid weight/percentage guards", () => {
        testExperimentsAndValidation()?;
    });
    test("4. Missing flag fallback (FlagNotFound) & toJsonSnapshot validated via cap:json", () => {
        testMissingFlagAndJsonSnapshot()?;
    });
    test("5. import cap from \"cap\"; cap.feature Automatic Dynamic Mounting", () => {
        testCapFeatureDynamicMount()?;
    });
}

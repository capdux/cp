// Origin: Pure-CP Stdlib Test Suite (lib/agent/model_catalog.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc LLM Model Capability Catalog & Cost Optimizer:
//   1. Registration, lookup, duplicate/invalid-spec & full-capacity guards
//   2. Prompt-cache + 50% batch discount exact fixed-point micro-USD accounting
//   3. Capability bitmask, context window, max_output_tokens & batch-aware cheapest selection
//   4. Deterministic tie-breaking by larger context_window when costs are equal
//   5. Extreme token saturation overflow defense (no u64 wraparound)
//   6. Empty catalog, zero-capacity catalog & clear() reuse guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MODEL_CAP_TOOLS,
    MODEL_CAP_VISION,
    MODEL_CAP_THINKING,
    MODEL_CAP_STRUCTURED_JSON,
    MODEL_CAP_PROMPT_CACHE,
    ModelSpec,
    ModelCostEstimate,
    estimateCallCost,
    ModelCatalog,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Registration, Lookup & Validation Guards
// ============================================================================

fn testRegistrationAndGuards()! {
    let slots: [2]ModelSpec = [ModelSpec{}; 2];
    let catalog = cap.agent.ModelCatalog.init(slots[0..]);

    // Reject empty model_id, zero context_window, batch_discount_bps > 10000
    expect_eq(catalog.registerModel(ModelSpec{ model_id: "", context_window: 128000 }), false);
    expect_eq(catalog.registerModel(ModelSpec{ model_id: "m0", context_window: 0 }), false);
    expect_eq(catalog.registerModel(ModelSpec{ model_id: "m0", context_window: 128000, batch_discount_bps: 10001 }), false);

    const m1 = ModelSpec{
        model_id: "gpt-4o-mini",
        provider: "openai",
        context_window: 128000,
        max_output_tokens: 16384,
        input_micro_usd_per_mtok: 150000,
        output_micro_usd_per_mtok: 600000,
        cached_input_micro_usd_per_mtok: 75000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_VISION | MODEL_CAP_STRUCTURED_JSON | MODEL_CAP_PROMPT_CACHE,
    };
    expect_eq(catalog.registerModel(m1), true);
    // Reject duplicate model_id
    expect_eq(catalog.registerModel(m1), false);

    const m2 = ModelSpec{
        model_id: "claude-sonnet",
        provider: "anthropic",
        context_window: 200000,
        max_output_tokens: 16384,
        input_micro_usd_per_mtok: 3000000,
        output_micro_usd_per_mtok: 15000000,
        cached_input_micro_usd_per_mtok: 300000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_VISION | MODEL_CAP_THINKING | MODEL_CAP_PROMPT_CACHE,
    };
    expect_eq(catalog.registerModel(m2), true);
    // Full capacity rejection
    expect_eq(catalog.registerModel(ModelSpec{ model_id: "m3", context_window: 64000 }), false);

    expect_eq(catalog.count(), 2);
    expect_eq(catalog.capacity(), 2);

    const found = catalog.findById("claude-sonnet") ?? return error.NotFound;
    expect_eq(strEq(found.provider, "anthropic"), true);
    expect_eq(found.context_window, 200000);
    expect_eq(catalog.findById("unknown-model") == null, true);
}

// ============================================================================
// Test 2: Prompt Cache & Batch 50% Discount Exact Accounting
// ============================================================================

fn testPromptCacheAndBatchDiscountPricing()! {
    // $3.00/1M input, $15.00/1M output, $0.30/1M cached input, 50% batch discount
    const spec = ModelSpec{
        model_id: "claude-sonnet",
        provider: "anthropic",
        context_window: 200000,
        max_output_tokens: 16384,
        input_micro_usd_per_mtok: 3000000,
        output_micro_usd_per_mtok: 15000000,
        cached_input_micro_usd_per_mtok: 300000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_PROMPT_CACHE,
    };

    // 100k regular input ($0.30 = 300,000), 500k cached input ($0.15 = 150,000 vs $1.50 = 1,500,000),
    // 20k output ($0.30 = 300,000) -> subtotal = 750,000 micro-USD, cache_savings = 1,350,000
    const live: ModelCostEstimate = estimateCallCost(spec, 100000, 500000, 20000, false);
    expect_eq(live.regular_input_micro_usd, 300000);
    expect_eq(live.cached_input_micro_usd, 150000);
    expect_eq(live.output_micro_usd, 300000);
    expect_eq(live.subtotal_micro_usd, 750000);
    expect_eq(live.total_micro_usd, 750000);
    expect_eq(live.cache_savings_micro_usd, 1350000);
    expect_eq(live.batch_savings_micro_usd, 0);
    expect_eq(live.total_savings_micro_usd, 1350000);

    // Same call in Batch mode (50% off subtotal -> 375,000 micro-USD, batch_savings = 375,000)
    const batch: ModelCostEstimate = ModelCatalog.estimateCallCost(spec, 100000, 500000, 20000, true);
    expect_eq(batch.subtotal_micro_usd, 750000);
    expect_eq(batch.total_micro_usd, 375000);
    expect_eq(batch.batch_savings_micro_usd, 375000);
    expect_eq(batch.total_savings_micro_usd, 1725000);

    // Model without MODEL_CAP_PROMPT_CACHE pays standard input rate for cached_input_tokens
    const no_cache_spec = ModelSpec{
        model_id: "legacy-llm",
        context_window: 32000,
        input_micro_usd_per_mtok: 2000000,
        output_micro_usd_per_mtok: 4000000,
        cached_input_micro_usd_per_mtok: 200000,
        batch_discount_bps: 0,
        cap_mask: MODEL_CAP_TOOLS,
    };
    const nc = estimateCallCost(no_cache_spec, 0, 500000, 0, false);
    expect_eq(nc.cached_input_micro_usd, 1000000);
    expect_eq(nc.cache_savings_micro_usd, 0);
}

// ============================================================================
// Test 3: Capability + Context + Max Output + Batch Cheapest Selection
// ============================================================================

fn testSelectCheapestCapableAndBatchFlip()! {
    let slots: [4]ModelSpec = [ModelSpec{}; 4];
    let catalog = ModelCatalog.init(slots[0..]);

    // Model A: ultra cheap, small context (32k), no THINKING
    * = catalog.registerModel(ModelSpec{
        model_id: "mini-fast",
        provider: "p1",
        context_window: 32000,
        max_output_tokens: 4096,
        input_micro_usd_per_mtok: 100000,
        output_micro_usd_per_mtok: 400000,
        batch_discount_bps: 0,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_STRUCTURED_JSON,
    });

    // Model B: mid price, 128k context, supports THINKING, NO batch discount
    * = catalog.registerModel(ModelSpec{
        model_id: "reasoner-live",
        provider: "p2",
        context_window: 128000,
        max_output_tokens: 16384,
        input_micro_usd_per_mtok: 1200000,
        output_micro_usd_per_mtok: 4000000,
        batch_discount_bps: 0,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_THINKING,
    });

    // Model C: slightly pricier live, 200k context, supports THINKING + 50% batch discount!
    * = catalog.registerModel(ModelSpec{
        model_id: "reasoner-batch",
        provider: "p3",
        context_window: 200000,
        max_output_tokens: 32768,
        input_micro_usd_per_mtok: 1800000,
        output_micro_usd_per_mtok: 6000000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_TOOLS | MODEL_CAP_THINKING | MODEL_CAP_VISION,
    });

    // 3a. Live mode requiring TOOLS + THINKING + 100k context -> Model B wins
    const best_live = catalog.selectCheapestCapable(MODEL_CAP_TOOLS | MODEL_CAP_THINKING, 100000, 10000, 2000, false) ?? return error.NoMatch;
    expect_eq(strEq(best_live.model_id, "reasoner-live"), true);

    // 3b. Batch mode with same requirements -> Model C (50% off -> cheaper than B!) wins
    const best_batch = catalog.selectCheapestCapable(MODEL_CAP_TOOLS | MODEL_CAP_THINKING, 100000, 10000, 2000, true) ?? return error.NoMatch;
    expect_eq(strEq(best_batch.model_id, "reasoner-batch"), true);

    // 3c. Requiring 24000 output tokens filters out Model B (max_output_tokens=16384) -> Model C wins even live
    const best_long_out = catalog.selectCheapestCapable(MODEL_CAP_TOOLS | MODEL_CAP_THINKING, 100000, 10000, 24000, false) ?? return error.NoMatch;
    expect_eq(strEq(best_long_out.model_id, "reasoner-batch"), true);

    // 3d. Impossible context requirement -> null
    expect_eq(catalog.selectCheapestCapable(MODEL_CAP_TOOLS, 500000, 1000, 100, false) == null, true);
}

// ============================================================================
// Test 4: Equal-Cost Tie-Breaking Prefers Larger Context Window
// ============================================================================

fn testEqualCostTieBreakByContextWindow()! {
    let slots: [2]ModelSpec = [ModelSpec{}; 2];
    let catalog = ModelCatalog.init(slots[0..]);

    * = catalog.registerModel(ModelSpec{
        model_id: "same-cost-64k",
        context_window: 64000,
        input_micro_usd_per_mtok: 500000,
        output_micro_usd_per_mtok: 1000000,
        cap_mask: MODEL_CAP_TOOLS,
    });
    * = catalog.registerModel(ModelSpec{
        model_id: "same-cost-256k",
        context_window: 256000,
        input_micro_usd_per_mtok: 500000,
        output_micro_usd_per_mtok: 1000000,
        cap_mask: MODEL_CAP_TOOLS,
    });

    const best = catalog.selectCheapestCapable(MODEL_CAP_TOOLS, 32000, 5000, 1000, false) ?? return error.NoMatch;
    expect_eq(strEq(best.model_id, "same-cost-256k"), true);
}

// ============================================================================
// Test 5: Extreme Token Saturation Overflow Defense
// ============================================================================

fn testExtremeTokenSaturationDefense()! {
    const spec = ModelSpec{
        model_id: "huge-rate",
        context_window: 1000000,
        input_micro_usd_per_mtok: 0xFFFFFFFFFFFFFFFF,
        output_micro_usd_per_mtok: 0xFFFFFFFFFFFFFFFF,
        cached_input_micro_usd_per_mtok: 1000000,
        batch_discount_bps: 5000,
        cap_mask: MODEL_CAP_PROMPT_CACHE,
    };
    const est = estimateCallCost(spec, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF, false);
    expect_eq(est.regular_input_micro_usd, 0xFFFFFFFFFFFFFFFF);
    expect_eq(est.output_micro_usd, 0xFFFFFFFFFFFFFFFF);
    expect_eq(est.total_micro_usd, 0xFFFFFFFFFFFFFFFF);
}

// ============================================================================
// Test 6: Empty Catalog, Zero-Capacity & Clear Reuse
// ============================================================================

fn testEmptyCatalogAndClearReuse()! {
    let empty_slots: [0]ModelSpec = [];
    let empty_cat = ModelCatalog.init(empty_slots[0..]);
    expect_eq(empty_cat.registerModel(ModelSpec{ model_id: "m1", context_window: 1000 }), false);
    expect_eq(empty_cat.selectCheapestCapable(0, 0, 100, 100, false) == null, true);

    let slots: [2]ModelSpec = [ModelSpec{}; 2];
    let cat = ModelCatalog.init(slots[0..]);
    expect_eq(cat.selectCheapestCapable(0, 0, 100, 100, false) == null, true);
    expect_eq(cat.registerModel(ModelSpec{ model_id: "m1", context_window: 64000, cap_mask: MODEL_CAP_TOOLS }), true);
    cat.clear();
    expect_eq(cat.count(), 0);
    expect_eq(cat.findById("m1") == null, true);
    expect_eq(cat.registerModel(ModelSpec{ model_id: "m1", context_window: 128000, cap_mask: MODEL_CAP_TOOLS }), true);
    expect_eq(cat.count(), 1);
}

test("agent.model_catalog: registration, lookup & invalid spec guards", testRegistrationAndGuards);
test("agent.model_catalog: prompt-cache & 50% batch discount exact micro-USD accounting", testPromptCacheAndBatchDiscountPricing);
test("agent.model_catalog: capability, context, max_output & batch cheapest selection", testSelectCheapestCapableAndBatchFlip);
test("agent.model_catalog: equal-cost deterministic tie-break by larger context_window", testEqualCostTieBreakByContextWindow);
test("agent.model_catalog: extreme token u128->u64 saturation overflow defense", testExtremeTokenSaturationDefense);
test("agent.model_catalog: empty catalog, zero-capacity & clear reuse guards", testEmptyCatalogAndClearReuse);

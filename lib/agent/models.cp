// Origin: Pure-CP Stdlib (Zero-Alloc LLM Model Capability Catalog & Cost Optimizer) | Created: 2026-11-22
// Zero-heap-allocation multi-model capability catalog, prompt-cache/batch discount fixed-point
// cost estimator (u128 overflow-safe micro-USD), and lowest-cost capable model selector.

const MTOK_DIV: u128 = 1000000;
const MTOK_HALF: u128 = 500000;
const BPS_DENOM: u128 = 10000;
const BPS_HALF: u128 = 5000;
const U64_MAX_U128: u128 = 0xFFFFFFFFFFFFFFFF;

/// Model supports function / tool calling.
pub const MODEL_CAP_TOOLS: u64 = 1 << 0;
/// Model supports image / multimodal vision input.
pub const MODEL_CAP_VISION: u64 = 1 << 1;
/// Model supports extended reasoning / thinking tokens.
pub const MODEL_CAP_THINKING: u64 = 1 << 2;
/// Model supports schema-constrained structured JSON output.
pub const MODEL_CAP_STRUCTURED_JSON: u64 = 1 << 3;
/// Model supports prompt-cache discounted input pricing.
pub const MODEL_CAP_PROMPT_CACHE: u64 = 1 << 4;

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn satU128ToU64(v: u128): u64 {
    if (v >= U64_MAX_U128) return 0xFFFFFFFFFFFFFFFF;
    return v as u64;
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) return 0xFFFFFFFFFFFFFFFF;
    return a + b;
}

fn rateTokensMicroUsd(tokens: u64, rate_per_mtok: u64): u64 {
    if (tokens == 0 || rate_per_mtok == 0) return 0;
    // Max product of two u64 values is (2^64 - 1)^2 = 2^128 - 2^65 + 1,
    // so adding MTOK_HALF (500,000) can never overflow u128.
    const prod: u128 = (tokens as u128) * (rate_per_mtok as u128);
    return satU128ToU64((prod + MTOK_HALF) / MTOK_DIV);
}

/// Static specification and fixed-point pricing card of an LLM model.
/// All rates are expressed in micro-USD per 1,000,000 tokens (1 USD = 1,000,000 micro-USD).
/// `batch_discount_bps` is expressed in basis points `0..10000` (e.g., `5000` = 50% off).
pub struct ModelSpec {
    model_id: string = "";
    provider: string = "";
    context_window: u32 = 0;
    max_output_tokens: u32 = 0;
    input_micro_usd_per_mtok: u64 = 0;
    output_micro_usd_per_mtok: u64 = 0;
    cached_input_micro_usd_per_mtok: u64 = 0;
    batch_discount_bps: u16 = 0;
    cap_mask: u64 = 0;

    /// Returns `true` if this model satisfies every capability bit in `required_cap_mask`.
    fn supports(required_cap_mask: u64): bool {
        return (_.cap_mask & required_cap_mask) == required_cap_mask;
    }
}

/// Detailed fixed-point micro-USD cost breakdown for a single LLM invocation.
pub struct ModelCostEstimate {
    regular_input_micro_usd: u64 = 0;
    cached_input_micro_usd: u64 = 0;
    output_micro_usd: u64 = 0;
    subtotal_micro_usd: u64 = 0;
    total_micro_usd: u64 = 0;
    cache_savings_micro_usd: u64 = 0;
    batch_savings_micro_usd: u64 = 0;
    total_savings_micro_usd: u64 = 0;
}

fn calcCallCost(
    spec: ModelSpec,
    input_tokens: u64,
    cached_input_tokens: u64,
    output_tokens: u64,
    is_batch: bool,
): ModelCostEstimate {
    const reg_in = rateTokensMicroUsd(input_tokens, spec.input_micro_usd_per_mtok);
    const has_cache_cap = (spec.cap_mask & MODEL_CAP_PROMPT_CACHE) != 0;
    const eff_cached_rate = if (has_cache_cap) spec.cached_input_micro_usd_per_mtok else spec.input_micro_usd_per_mtok;
    const cached_in = rateTokensMicroUsd(cached_input_tokens, eff_cached_rate);
    const uncached_equiv = rateTokensMicroUsd(cached_input_tokens, spec.input_micro_usd_per_mtok);
    const cache_savings = if (uncached_equiv > cached_in) (uncached_equiv - cached_in) else 0;

    const out_cost = rateTokensMicroUsd(output_tokens, spec.output_micro_usd_per_mtok);
    const subtotal = satAddU64(satAddU64(reg_in, cached_in), out_cost);

    let total = subtotal;
    let batch_savings: u64 = 0;
    if (is_batch && spec.batch_discount_bps > 0) {
        const bps: u128 = if (spec.batch_discount_bps > 10000) 10000 else (spec.batch_discount_bps as u128);
        const pay_bps: u128 = BPS_DENOM - bps;
        const discounted = satU128ToU64(((subtotal as u128) * pay_bps + BPS_HALF) / BPS_DENOM);
        total = discounted;
        batch_savings = if (subtotal > discounted) (subtotal - discounted) else 0;
    }

    return ModelCostEstimate{
        regular_input_micro_usd: reg_in,
        cached_input_micro_usd: cached_in,
        output_micro_usd: out_cost,
        subtotal_micro_usd: subtotal,
        total_micro_usd: total,
        cache_savings_micro_usd: cache_savings,
        batch_savings_micro_usd: batch_savings,
        total_savings_micro_usd: satAddU64(cache_savings, batch_savings),
    };
}

/// Estimates fixed-point micro-USD cost for a single call on `spec`, including prompt-cache
/// pricing (when `MODEL_CAP_PROMPT_CACHE` is set) and batch API discount (`batch_discount_bps`).
/// Uses `u128` intermediate arithmetic and saturates cleanly at `u64` max on extreme inputs.
pub fn estimateCallCost(
    spec: ModelSpec,
    input_tokens: u64,
    cached_input_tokens: u64,
    output_tokens: u64,
    is_batch: bool,
): ModelCostEstimate {
    return calcCallCost(spec, input_tokens, cached_input_tokens, output_tokens, is_batch);
}

/// Zero-heap-allocation LLM model capability catalog and lowest-cost selector.
pub struct ModelCatalog {
    slots: []ModelSpec;
    len: usize = 0;

    /// Initializes a `ModelCatalog` backed by caller-provided `slots`.
    static fn init(slots: []ModelSpec): ModelCatalog {
        return ModelCatalog{
            slots: slots,
            len: 0,
        };
    }

    /// Registers `spec` in the catalog. Rejects empty `model_id`, zero `context_window`,
    /// `batch_discount_bps > 10000`, duplicate `model_id`, or full capacity.
    fn registerModel(*_, spec: ModelSpec): bool {
        if (_.len >= _.slots.len) return false;
        if (spec.model_id.len == 0 || spec.context_window == 0) return false;
        if (spec.batch_discount_bps > 10000) return false;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (strEq(_.slots[i].model_id, spec.model_id)) {
                return false;
            }
        }
        _.slots[_.len] = spec;
        _.len += 1;
        return true;
    }

    /// Looks up a registered model specification by exact `model_id`.
    fn findById(model_id: string): ModelSpec? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (strEq(_.slots[i].model_id, model_id)) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns the number of registered models.
    fn count(): usize {
        return _.len;
    }

    /// Returns the maximum capacity of the catalog.
    fn capacity(): usize {
        return _.slots.len;
    }

    /// Clears all registered models for buffer reuse.
    fn clear(*_) {
        _.len = 0;
    }

    /// Convenience wrapper delegating to `calcCallCost`.
    static fn estimateCallCost(
        spec: ModelSpec,
        input_tokens: u64,
        cached_input_tokens: u64,
        output_tokens: u64,
        is_batch: bool,
    ): ModelCostEstimate {
        return calcCallCost(spec, input_tokens, cached_input_tokens, output_tokens, is_batch);
    }

    /// Selects the lowest-cost registered model that satisfies `required_cap_mask`,
    /// `context_window >= min_context_tokens`, and `max_output_tokens >= est_output_tokens`
    /// (when `max_output_tokens > 0`). Ties break deterministically by larger `context_window`.
    fn selectCheapestCapable(
        required_cap_mask: u64,
        min_context_tokens: u32,
        est_input_tokens: u64,
        est_output_tokens: u64,
        is_batch: bool,
    ): ModelSpec? {
        let found = false;
        let best_idx: usize = 0;
        let best_cost: u64 = 0xFFFFFFFFFFFFFFFF;
        let best_ctx: u32 = 0;

        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const spec = _.slots[i];
            if (!spec.supports(required_cap_mask)) continue;
            if (spec.context_window < min_context_tokens) continue;
            if (spec.max_output_tokens > 0 && est_output_tokens > (spec.max_output_tokens as u64)) continue;

            const est = calcCallCost(spec, est_input_tokens, 0, est_output_tokens, is_batch);
            if (!found || est.total_micro_usd < best_cost || (est.total_micro_usd == best_cost && spec.context_window > best_ctx)) {
                found = true;
                best_idx = i;
                best_cost = est.total_micro_usd;
                best_ctx = spec.context_window;
            }
        }

        if (!found) return null;
        return _.slots[best_idx];
    }
}

// Origin: Pure-CP Stdlib (AI Multi-Agent Cost Attribution Ledger & Prompt-Cache Accounting) | Created: 2026-11-22
// Zero-heap-allocation fixed-point (micro-USD) token cost attribution ledger with
// prompt-cache read/write discount accounting and cycle-safe hierarchical subtree aggregation.

const MTOK_DIVISOR: u128 = 1000000;
const MTOK_HALF: u128 = 500000;

/// Per-model fixed-point pricing card expressed in micro-USD per 1,000,000 tokens (1 USD = 1,000,000 micro-USD).
pub struct ModelRateCard {
    model_id: u16 = 0;
    input_per_mtok_micro: u64 = 0;
    output_per_mtok_micro: u64 = 0;
    cache_read_per_mtok_micro: u64 = 0;
    cache_write_per_mtok_micro: u64 = 0;
}

/// Single LLM invocation token accounting event attributed to `agent_id` under `parent_agent_id` (`0` = root).
pub struct UsageEvent {
    agent_id: u32 = 0;
    parent_agent_id: u32 = 0;
    model_id: u16 = 0;
    input_tokens: u64 = 0;
    output_tokens: u64 = 0;
    cache_read_tokens: u64 = 0;
    cache_write_tokens: u64 = 0;
}

/// Aggregated direct and hierarchical subtree cost/token metrics for an agent.
pub struct AgentCostSummary {
    direct_micro_usd: u64 = 0;
    subtree_micro_usd: u64 = 0;
    total_input_tokens: u64 = 0;
    total_output_tokens: u64 = 0;
    cache_read_tokens: u64 = 0;
    cache_savings_micro_usd: u64 = 0;
    call_count: u64 = 0;
}

/// Highest direct-spending agent record returned by `CostLedger.topSpendingAgent`.
pub struct TopSpender {
    agent_id: u32 = 0;
    direct_micro_usd: u64 = 0;
    subtree_micro_usd: u64 = 0;
}

/// Computes exact fixed-point micro-USD cost for `ev` using `u128` intermediate arithmetic
/// to prevent overflow and floating-point drift.
pub fn computeUsageCostMicroUsd(rate: ModelRateCard, ev: UsageEvent): u64 {
    const in_num: u128 = u128(ev.input_tokens) * u128(rate.input_per_mtok_micro);
    const out_num: u128 = u128(ev.output_tokens) * u128(rate.output_per_mtok_micro);
    const cr_num: u128 = u128(ev.cache_read_tokens) * u128(rate.cache_read_per_mtok_micro);
    const cw_num: u128 = u128(ev.cache_write_tokens) * u128(rate.cache_write_per_mtok_micro);
    const total_num: u128 = in_num + out_num + cr_num + cw_num;
    return u64((total_num + MTOK_HALF) / MTOK_DIVISOR);
}

/// Computes fixed-point micro-USD saved by prompt-cache read hits compared against standard input pricing.
pub fn computeCacheSavingsMicroUsd(rate: ModelRateCard, cache_read_tokens: u64): u64 {
    if (cache_read_tokens == 0 || rate.input_per_mtok_micro <= rate.cache_read_per_mtok_micro) {
        return 0;
    }
    const discount = rate.input_per_mtok_micro - rate.cache_read_per_mtok_micro;
    const num: u128 = u128(cache_read_tokens) * u128(discount);
    return u64((num + MTOK_HALF) / MTOK_DIVISOR);
}

/// Zero-heap-allocation multi-agent token and micro-USD cost attribution ledger.
pub struct CostLedger {
    rates: []ModelRateCard;
    rate_count: usize = 0;
    events: []UsageEvent;
    event_count: usize = 0;

    /// Initializes a ledger backed by caller-owned `rates` and `events` buffers.
    static fn init(rates: *[]ModelRateCard, events: *[]UsageEvent): CostLedger {
        return CostLedger{
            rates: rates,
            rate_count: 0,
            events: events,
            event_count: 0,
        };
    }

    /// Returns the number of registered model rate cards.
    fn rateCount(): usize {
        return _.rate_count;
    }

    /// Returns the number of recorded usage events.
    fn eventCount(): usize {
        return _.event_count;
    }

    /// Looks up a registered `ModelRateCard` by `model_id`.
    fn findRate(model_id: u16): ModelRateCard? {
        if (model_id == 0) return null;
        let i: usize = 0;
        while (i < _.rate_count) : (i += 1) {
            if (_.rates[i].model_id == model_id) {
                return _.rates[i];
            }
        }
        return null;
    }

    /// Registers or updates a model rate card. Rejects `model_id == 0` or full rate buffer.
    fn registerRate(*_, rate: ModelRateCard): bool {
        if (rate.model_id == 0) return false;
        let i: usize = 0;
        while (i < _.rate_count) : (i += 1) {
            if (_.rates[i].model_id == rate.model_id) {
                _.rates[i] = rate;
                return true;
            }
        }
        if (_.rate_count >= _.rates.len) return false;
        _.rates[_.rate_count] = rate;
        _.rate_count += 1;
        return true;
    }

    /// Records a token usage event. Rejects `agent_id == 0`, unregistered `model_id`, or full event buffer.
    fn recordUsage(*_, ev: UsageEvent): bool {
        if (ev.agent_id == 0) return false;
        if (_.findRate(ev.model_id) == null) return false;
        if (_.event_count >= _.events.len) return false;
        _.events[_.event_count] = ev;
        _.event_count += 1;
        return true;
    }

    fn parentOf(agent_id: u32): u32 {
        let i: usize = 0;
        while (i < _.event_count) : (i += 1) {
            if (_.events[i].agent_id == agent_id) {
                return _.events[i].parent_agent_id;
            }
        }
        return 0;
    }

    /// Cycle-safe iterative ancestor check: returns `true` if `candidate_id` is a descendant of `root_id`.
    fn isDescendantOf(candidate_id: u32, root_id: u32): bool {
        if (candidate_id == 0 || root_id == 0 || candidate_id == root_id) {
            return false;
        }
        let cur = candidate_id;
        let steps: usize = 0;
        const max_steps = _.event_count + 1;
        while (steps < max_steps) : (steps += 1) {
            const p = _.parentOf(cur);
            if (p == 0 || p == cur) return false;
            if (p == root_id) return true;
            if (p == candidate_id) return false; // closed cycle guard
            cur = p;
        }
        return false;
    }

    /// Computes direct and hierarchical subtree cost/token metrics for `agent_id` (cycle-safe).
    fn agentSummary(agent_id: u32): AgentCostSummary {
        let sum = AgentCostSummary{};
        if (agent_id == 0) return sum;

        let i: usize = 0;
        while (i < _.event_count) : (i += 1) {
            const ev = _.events[i];
            if (rate := _.findRate(ev.model_id)) {
                const cost = computeUsageCostMicroUsd(rate, ev);
                if (ev.agent_id == agent_id) {
                    sum.direct_micro_usd += cost;
                    sum.subtree_micro_usd += cost;
                    sum.total_input_tokens += ev.input_tokens;
                    sum.total_output_tokens += ev.output_tokens;
                    sum.cache_read_tokens += ev.cache_read_tokens;
                    sum.cache_savings_micro_usd += computeCacheSavingsMicroUsd(rate, ev.cache_read_tokens);
                    sum.call_count += 1;
                } else if (_.isDescendantOf(ev.agent_id, agent_id)) {
                    sum.subtree_micro_usd += cost;
                }
            }
        }
        return sum;
    }

    /// Returns total micro-USD spent across all recorded events in the ledger.
    fn totalMicroUsd(): u64 {
        let total: u64 = 0;
        let i: usize = 0;
        while (i < _.event_count) : (i += 1) {
            const ev = _.events[i];
            if (rate := _.findRate(ev.model_id)) {
                total += computeUsageCostMicroUsd(rate, ev);
            }
        }
        return total;
    }

    /// Identifies the agent with the highest `direct_micro_usd` spend (tie-broken by smaller `agent_id`).
    /// Returns `null` if no usage events have been recorded.
    fn topSpendingAgent(): TopSpender? {
        if (_.event_count == 0) return null;

        let best: TopSpender? = null;
        let i: usize = 0;
        while (i < _.event_count) : (i += 1) {
            const aid = _.events[i].agent_id;
            // Deduplicate: skip if we already evaluated this agent_id at an earlier index
            let seen = false;
            let prev: usize = 0;
            while (prev < i) : (prev += 1) {
                if (_.events[prev].agent_id == aid) {
                    seen = true;
                    break;
                }
            }
            if (seen) continue;

            const s = _.agentSummary(aid);
            if (b := best) {
                if (s.direct_micro_usd > b.direct_micro_usd ||
                    (s.direct_micro_usd == b.direct_micro_usd && aid < b.agent_id))
                {
                    best = TopSpender{
                        agent_id: aid,
                        direct_micro_usd: s.direct_micro_usd,
                        subtree_micro_usd: s.subtree_micro_usd,
                    };
                }
            } else {
                best = TopSpender{
                    agent_id: aid,
                    direct_micro_usd: s.direct_micro_usd,
                    subtree_micro_usd: s.subtree_micro_usd,
                };
            }
        }
        return best;
    }
}

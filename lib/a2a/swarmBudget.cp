// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Agent Hierarchical Swarm Budget Ledger & Cascading Circuit Breaker) | Created: 2026-11-23
// Pure CP zero-heap-allocation hierarchical multi-agent token/cost/tool budget ledger:
//   - Fixed 64-node parent-child agent budget tree with root global caps (`max_tokens`,
//     `max_cost_micros`, `max_tool_calls`, `deadline_ms`) and per-child sub-quotas
//   - Optional strict parent reservation (`reserve_from_parent`) with automatic conversion from
//     reserved->used on charge and unused-quota refund on `completeAgent`
//   - Atomic ancestor-chain charging (`charge`): validates entire parent chain before committing;
//     on quota/deadline breach, cascades `halted_budget` / `halted_deadline` across the offending
//     subtree via 100% iterative traversal (zero recursion)
//   - Effective remaining budget calculation along the full ancestor chain & cluster telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of agent nodes tracked inline in a swarm budget tree (zero dynamic heap).
pub const SWARM_BUDGET_MAX_NODES: usize = 64;

/// Sentinel parent slot for the root agent node.
pub const SWARM_BUDGET_NO_NODE: u16 = 0xFFFF;

/// Lifecycle state of an agent node in the budget tree.
pub enum SwarmAgentState {
    active,
    completed,
    halted_budget,
    halted_deadline,
}

/// Violation reason returned by `charge` when a request is rejected.
pub enum SwarmBudgetViolation {
    none,
    agent_not_found,
    agent_not_active,
    ancestor_halted,
    token_limit_exceeded,
    cost_limit_exceeded,
    tool_limit_exceeded,
    deadline_exceeded,
}

/// Outcome status of `spawnChild`.
pub enum SwarmSpawnStatus {
    ok,
    parent_not_found,
    parent_not_active,
    duplicate_agent_id,
    invalid_agent_id,
    capacity_full,
    exceeds_parent_remaining,
    deadline_exceeds_parent,
}

/// Quota specification for a root or child agent.
pub struct SwarmBudgetLimits {
    max_tokens: u64 = 0,
    max_cost_micros: u64 = 0,
    max_tool_calls: u32 = 0,
    /// Absolute deadline in epoch milliseconds (`0` means no deadline, or inherit parent's).
    deadline_ms: u64 = 0,
    /// When `true`, `spawnChild` reserves the child's full sub-quota from the parent's free pool
    /// and refunds any unused portion when the child completes or halts.
    reserve_from_parent: bool = false,
}

/// Inline agent budget ledger node.
pub struct SwarmAgentNode {
    occupied: bool = false;
    agent_id: u64 = 0;
    parent_slot: u16 = SWARM_BUDGET_NO_NODE;
    depth: u16 = 0;
    state: SwarmAgentState = SwarmAgentState.active;
    limits: SwarmBudgetLimits = SwarmBudgetLimits{};

    used_tokens: u64 = 0;
    used_cost_micros: u64 = 0;
    used_tool_calls: u32 = 0;

    reserved_tokens: u64 = 0;
    reserved_cost_micros: u64 = 0;
    reserved_tool_calls: u32 = 0;

    reservation_active: bool = false;
}

/// Detailed verdict returned by `SwarmBudgetLedger.charge`.
pub struct SwarmChargeOutcome {
    allowed: bool = false;
    violation: SwarmBudgetViolation = SwarmBudgetViolation.none;
    violated_agent_id: u64 = 0;
    cascaded_halts: usize = 0;
}

/// Effective minimum remaining budget across an agent and its entire ancestor chain.
pub struct SwarmRemainingBudget {
    found: bool = false;
    can_continue: bool = false;
    remaining_tokens: u64 = 0;
    remaining_cost_micros: u64 = 0;
    remaining_tool_calls: u32 = 0;
    has_deadline: bool = false;
    remaining_ms: u64 = 0;
}

/// Global telemetry snapshot for `SwarmBudgetLedger`.
pub struct SwarmBudgetStats {
    node_count: usize = 0;
    capacity: usize = SWARM_BUDGET_MAX_NODES;
    active_agents: usize = 0;
    completed_agents: usize = 0;
    halted_budget_agents: usize = 0;
    halted_deadline_agents: usize = 0;
    root_used_tokens: u64 = 0;
    root_used_cost_micros: u64 = 0;
    root_used_tool_calls: u32 = 0;
    total_charges_allowed: u64 = 0;
    total_charges_denied: u64 = 0;
    total_cascaded_halts: u64 = 0;
}

// ============================================================================
// 2. Core Engine (`SwarmBudgetLedger`)
// ============================================================================

/// Zero-heap hierarchical multi-agent budget ledger with atomic ancestor-chain
/// charging, parent quota reservation/refund, and iterative cascading circuit breaking.
pub struct SwarmBudgetLedger {
    nodes: [SWARM_BUDGET_MAX_NODES]SwarmAgentNode = [SwarmAgentNode{}; SWARM_BUDGET_MAX_NODES];
    node_count: usize = 0;
    total_charges_allowed: u64 = 0;
    total_charges_denied: u64 = 0;
    total_cascaded_halts: u64 = 0;

    /// Initializes a swarm budget ledger with a single root agent (`root_id > 0`).
    /// Returns `null` if `root_id == 0`.
    pub fn init(root_id: u64, root_limits: SwarmBudgetLimits): SwarmBudgetLedger? {
        if (root_id == 0) return null;
        let ledger = SwarmBudgetLedger{};
        const clean_limits = SwarmBudgetLimits{
            max_tokens: root_limits.max_tokens,
            max_cost_micros: root_limits.max_cost_micros,
            max_tool_calls: root_limits.max_tool_calls,
            deadline_ms: root_limits.deadline_ms,
            reserve_from_parent: false,
        };
        ledger.nodes[0] = SwarmAgentNode{
            occupied: true,
            agent_id: root_id,
            parent_slot: SWARM_BUDGET_NO_NODE,
            depth: 0,
            state: SwarmAgentState.active,
            limits: clean_limits,
        };
        ledger.node_count = 1;
        return ledger;
    }

    fn findSlot(agent_id: u64): usize? {
        if (agent_id == 0) return null;
        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            if (nodes[i].occupied && nodes[i].agent_id == agent_id) return i;
        }
        return null;
    }

    fn freeTokens(slot: usize): u64 {
        const used_plus_res = nodes[slot].used_tokens + nodes[slot].reserved_tokens;
        if (used_plus_res >= nodes[slot].limits.max_tokens) return 0;
        return nodes[slot].limits.max_tokens - used_plus_res;
    }

    fn freeCost(slot: usize): u64 {
        const used_plus_res = nodes[slot].used_cost_micros + nodes[slot].reserved_cost_micros;
        if (used_plus_res >= nodes[slot].limits.max_cost_micros) return 0;
        return nodes[slot].limits.max_cost_micros - used_plus_res;
    }

    fn freeTools(slot: usize): u32 {
        const used_plus_res = nodes[slot].used_tool_calls + nodes[slot].reserved_tool_calls;
        if (used_plus_res >= nodes[slot].limits.max_tool_calls) return 0;
        return nodes[slot].limits.max_tool_calls - used_plus_res;
    }

    fn releaseReservationToParent(*_, slot: usize) {
        if (!nodes[slot].reservation_active) return;
        const p = nodes[slot].parent_slot;
        if (p != SWARM_BUDGET_NO_NODE) {
            const p_idx: usize = p as usize;
            const unused_tok: u64 = if (nodes[slot].limits.max_tokens > nodes[slot].used_tokens)
                (nodes[slot].limits.max_tokens - nodes[slot].used_tokens)
            else
                0;
            const unused_cost: u64 = if (nodes[slot].limits.max_cost_micros > nodes[slot].used_cost_micros)
                (nodes[slot].limits.max_cost_micros - nodes[slot].used_cost_micros)
            else
                0;
            const unused_tools: u32 = if (nodes[slot].limits.max_tool_calls > nodes[slot].used_tool_calls)
                (nodes[slot].limits.max_tool_calls - nodes[slot].used_tool_calls)
            else
                0;

            nodes[p_idx].reserved_tokens = if (nodes[p_idx].reserved_tokens > unused_tok)
                (nodes[p_idx].reserved_tokens - unused_tok)
            else
                0;
            nodes[p_idx].reserved_cost_micros = if (nodes[p_idx].reserved_cost_micros > unused_cost)
                (nodes[p_idx].reserved_cost_micros - unused_cost)
            else
                0;
            nodes[p_idx].reserved_tool_calls = if (nodes[p_idx].reserved_tool_calls > unused_tools)
                (nodes[p_idx].reserved_tool_calls - unused_tools)
            else
                0;
        }
        nodes[slot].reservation_active = false;
    }

    /// 100% iterative BFS halting `root_slot` and all of its active descendants.
    /// Returns the number of newly halted agent nodes.
    fn haltSubtreeIterative(*_, root_slot: usize, target_state: SwarmAgentState): usize {
        let queue: [SWARM_BUDGET_MAX_NODES]usize = [0; SWARM_BUDGET_MAX_NODES];
        let head_q: usize = 0;
        let tail_q: usize = 0;

        queue[tail_q] = root_slot;
        tail_q += 1;

        let halted: usize = 0;
        while (head_q < tail_q) {
            const cur = queue[head_q];
            head_q += 1;

            if (nodes[cur].state == SwarmAgentState.active) {
                nodes[cur].state = target_state;
                halted += 1;
                releaseReservationToParent(cur);
            }

            let c: usize = 0;
            while (c < node_count) : (c += 1) {
                if (nodes[c].occupied && nodes[c].parent_slot == (cur as u16)) {
                    if (tail_q < SWARM_BUDGET_MAX_NODES) {
                        queue[tail_q] = c;
                        tail_q += 1;
                    }
                }
            }
        }
        total_cascaded_halts += halted as u64;
        return halted;
    }

    /// Spawns a child agent under `parent_id` with `sub_limits`.
    /// Enforces parent active status, deadline monotonic tightening, and parent remaining budget.
    pub fn spawnChild(
        *_,
        parent_id: u64,
        child_id: u64,
        sub_limits: SwarmBudgetLimits,
    ): SwarmSpawnStatus {
        if (child_id == 0) return SwarmSpawnStatus.invalid_agent_id;
        if (node_count >= SWARM_BUDGET_MAX_NODES) return SwarmSpawnStatus.capacity_full;
        const p_slot = findSlot(parent_id) ?? return SwarmSpawnStatus.parent_not_found;
        if (nodes[p_slot].state != SwarmAgentState.active) return SwarmSpawnStatus.parent_not_active;
        if (findSlot(child_id) != null) return SwarmSpawnStatus.duplicate_agent_id;

        // Deadline validation & inheritance
        const p_deadline = nodes[p_slot].limits.deadline_ms;
        let eff_deadline = sub_limits.deadline_ms;
        if (eff_deadline == 0) {
            eff_deadline = p_deadline;
        } else if (p_deadline > 0 && eff_deadline > p_deadline) {
            return SwarmSpawnStatus.deadline_exceeds_parent;
        }
        const eff_limits = SwarmBudgetLimits{
            max_tokens: sub_limits.max_tokens,
            max_cost_micros: sub_limits.max_cost_micros,
            max_tool_calls: sub_limits.max_tool_calls,
            deadline_ms: eff_deadline,
            reserve_from_parent: sub_limits.reserve_from_parent,
        };

        // Remaining pool validation
        const p_free_tok = freeTokens(p_slot);
        const p_free_cost = freeCost(p_slot);
        const p_free_tools = freeTools(p_slot);
        if (eff_limits.max_tokens > p_free_tok ||
            eff_limits.max_cost_micros > p_free_cost ||
            eff_limits.max_tool_calls > p_free_tools)
        {
            return SwarmSpawnStatus.exceeds_parent_remaining;
        }

        if (eff_limits.reserve_from_parent) {
            nodes[p_slot].reserved_tokens += eff_limits.max_tokens;
            nodes[p_slot].reserved_cost_micros += eff_limits.max_cost_micros;
            nodes[p_slot].reserved_tool_calls += eff_limits.max_tool_calls;
        }

        const slot = node_count;
        nodes[slot] = SwarmAgentNode{
            occupied: true,
            agent_id: child_id,
            parent_slot: p_slot as u16,
            depth: nodes[p_slot].depth + 1,
            state: SwarmAgentState.active,
            limits: eff_limits,
            reservation_active: eff_limits.reserve_from_parent,
        };
        node_count += 1;
        return SwarmSpawnStatus.ok;
    }

    /// Atomically validates and charges `(tokens, cost_micros, tool_calls)` up the entire ancestor
    /// chain from `agent_id` to root. If any node or ancestor breaches its quota or deadline, no
    /// partial charge is committed and the offending node's entire subtree is cascaded to
    /// `halted_budget` or `halted_deadline`.
    pub fn charge(
        *_,
        agent_id: u64,
        tokens: u64,
        cost_micros: u64,
        tool_calls: u32,
        now_ms: u64,
    ): SwarmChargeOutcome {
        let start_slot: usize = 0;
        if (s := findSlot(agent_id)) {
            start_slot = s;
        } else {
            total_charges_denied += 1;
            return SwarmChargeOutcome{
                allowed: false,
                violation: SwarmBudgetViolation.agent_not_found,
                violated_agent_id: agent_id,
            };
        }

        // Pass 1: iterative validation from start_slot up to root
        let cur: usize = start_slot;
        let covered_by_reservation: bool = false;
        let steps: usize = 0;

        while (steps < SWARM_BUDGET_MAX_NODES) : (steps += 1) {
            const node = nodes[cur];
            if (node.state != SwarmAgentState.active) {
                total_charges_denied += 1;
                return SwarmChargeOutcome{
                    allowed: false,
                    violation: if (cur == start_slot) SwarmBudgetViolation.agent_not_active else SwarmBudgetViolation.ancestor_halted,
                    violated_agent_id: node.agent_id,
                };
            }

            if (node.limits.deadline_ms > 0 && now_ms > node.limits.deadline_ms) {
                const halts = haltSubtreeIterative(cur, SwarmAgentState.halted_deadline);
                total_charges_denied += 1;
                return SwarmChargeOutcome{
                    allowed: false,
                    violation: SwarmBudgetViolation.deadline_exceeded,
                    violated_agent_id: node.agent_id,
                    cascaded_halts: halts,
                };
            }

            // If the child directly below `cur` reserved its quota from `cur`, this charge
            // converts reserved->used without consuming additional free pool on `cur`.
            if (!covered_by_reservation) {
                if (tokens > freeTokens(cur)) {
                    const halts = haltSubtreeIterative(cur, SwarmAgentState.halted_budget);
                    total_charges_denied += 1;
                    return SwarmChargeOutcome{
                        allowed: false,
                        violation: SwarmBudgetViolation.token_limit_exceeded,
                        violated_agent_id: node.agent_id,
                        cascaded_halts: halts,
                    };
                }
                if (cost_micros > freeCost(cur)) {
                    const halts = haltSubtreeIterative(cur, SwarmAgentState.halted_budget);
                    total_charges_denied += 1;
                    return SwarmChargeOutcome{
                        allowed: false,
                        violation: SwarmBudgetViolation.cost_limit_exceeded,
                        violated_agent_id: node.agent_id,
                        cascaded_halts: halts,
                    };
                }
                if (tool_calls > freeTools(cur)) {
                    const halts = haltSubtreeIterative(cur, SwarmAgentState.halted_budget);
                    total_charges_denied += 1;
                    return SwarmChargeOutcome{
                        allowed: false,
                        violation: SwarmBudgetViolation.tool_limit_exceeded,
                        violated_agent_id: node.agent_id,
                        cascaded_halts: halts,
                    };
                }
            }

            if (node.parent_slot == SWARM_BUDGET_NO_NODE) break;
            covered_by_reservation = node.reservation_active;
            cur = node.parent_slot as usize;
        }

        // Pass 2: atomic commit up the ancestor chain
        cur = start_slot;
        covered_by_reservation = false;
        steps = 0;
        while (steps < SWARM_BUDGET_MAX_NODES) : (steps += 1) {
            nodes[cur].used_tokens += tokens;
            nodes[cur].used_cost_micros += cost_micros;
            nodes[cur].used_tool_calls += tool_calls;

            if (covered_by_reservation) {
                nodes[cur].reserved_tokens = if (nodes[cur].reserved_tokens > tokens)
                    (nodes[cur].reserved_tokens - tokens)
                else
                    0;
                nodes[cur].reserved_cost_micros = if (nodes[cur].reserved_cost_micros > cost_micros)
                    (nodes[cur].reserved_cost_micros - cost_micros)
                else
                    0;
                nodes[cur].reserved_tool_calls = if (nodes[cur].reserved_tool_calls > tool_calls)
                    (nodes[cur].reserved_tool_calls - tool_calls)
                else
                    0;
            }

            if (nodes[cur].parent_slot == SWARM_BUDGET_NO_NODE) break;
            covered_by_reservation = nodes[cur].reservation_active;
            cur = nodes[cur].parent_slot as usize;
        }

        total_charges_allowed += 1;
        return SwarmChargeOutcome{
            allowed: true,
            violation: SwarmBudgetViolation.none,
        };
    }

    /// Marks `agent_id` (and any still-active descendants) as `completed` and refunds any
    /// unused reserved quota back to its parent's free pool.
    pub fn completeAgent(*_, agent_id: u64): bool {
        const root_slot = findSlot(agent_id) ?? return false;
        if (nodes[root_slot].state != SwarmAgentState.active) return false;

        let queue: [SWARM_BUDGET_MAX_NODES]usize = [0; SWARM_BUDGET_MAX_NODES];
        let head_q: usize = 0;
        let tail_q: usize = 0;
        queue[tail_q] = root_slot;
        tail_q += 1;

        while (head_q < tail_q) {
            const cur = queue[head_q];
            head_q += 1;
            if (nodes[cur].state == SwarmAgentState.active) {
                nodes[cur].state = SwarmAgentState.completed;
                releaseReservationToParent(cur);
            }
            let c: usize = 0;
            while (c < node_count) : (c += 1) {
                if (nodes[c].occupied && nodes[c].parent_slot == (cur as u16)) {
                    if (tail_q < SWARM_BUDGET_MAX_NODES) {
                        queue[tail_q] = c;
                        tail_q += 1;
                    }
                }
            }
        }
        return true;
    }

    /// Computes the effective minimum remaining tokens, cost, tool calls, and deadline window
    /// across `agent_id` and its entire ancestor chain.
    pub fn remainingBudget(agent_id: u64, now_ms: u64): SwarmRemainingBudget {
        let out = SwarmRemainingBudget{};
        const start_slot = findSlot(agent_id) ?? return out;

        out.found = true;
        out.can_continue = true;
        out.remaining_tokens = 0xFFFFFFFFFFFFFFFF;
        out.remaining_cost_micros = 0xFFFFFFFFFFFFFFFF;
        out.remaining_tool_calls = 0xFFFFFFFF;

        let cur: usize = start_slot;
        let covered_by_reservation: bool = false;
        let steps: usize = 0;

        while (steps < SWARM_BUDGET_MAX_NODES) : (steps += 1) {
            const node = nodes[cur];
            if (node.state != SwarmAgentState.active) {
                out.can_continue = false;
            }

            if (node.limits.deadline_ms > 0) {
                const rem_ms: u64 = if (node.limits.deadline_ms > now_ms)
                    (node.limits.deadline_ms - now_ms)
                else
                    0;
                if (now_ms > node.limits.deadline_ms) {
                    out.can_continue = false;
                }
                if (!out.has_deadline || rem_ms < out.remaining_ms) {
                    out.has_deadline = true;
                    out.remaining_ms = rem_ms;
                }
            }

            if (!covered_by_reservation) {
                const ft = freeTokens(cur);
                const fc = freeCost(cur);
                const fl = freeTools(cur);
                if (ft < out.remaining_tokens) out.remaining_tokens = ft;
                if (fc < out.remaining_cost_micros) out.remaining_cost_micros = fc;
                if (fl < out.remaining_tool_calls) out.remaining_tool_calls = fl;
            }

            if (node.parent_slot == SWARM_BUDGET_NO_NODE) break;
            covered_by_reservation = node.reservation_active;
            cur = node.parent_slot as usize;
        }

        return out;
    }

    /// Returns a read-only copy of `agent_id`, or `null` if absent.
    pub fn getAgent(agent_id: u64): SwarmAgentNode? {
        const slot = findSlot(agent_id) ?? return null;
        return nodes[slot];
    }

    /// Returns a global telemetry snapshot.
    pub fn stats(): SwarmBudgetStats {
        let st = SwarmBudgetStats{
            node_count: node_count,
            capacity: SWARM_BUDGET_MAX_NODES,
            root_used_tokens: if (node_count > 0) nodes[0].used_tokens else 0,
            root_used_cost_micros: if (node_count > 0) nodes[0].used_cost_micros else 0,
            root_used_tool_calls: if (node_count > 0) nodes[0].used_tool_calls else 0,
            total_charges_allowed: total_charges_allowed,
            total_charges_denied: total_charges_denied,
            total_cascaded_halts: total_cascaded_halts,
        };

        let i: usize = 0;
        while (i < node_count) : (i += 1) {
            if (!nodes[i].occupied) continue;
            if (nodes[i].state == SwarmAgentState.active) {
                st.active_agents += 1;
            } else if (nodes[i].state == SwarmAgentState.completed) {
                st.completed_agents += 1;
            } else if (nodes[i].state == SwarmAgentState.halted_budget) {
                st.halted_budget_agents += 1;
            } else {
                st.halted_deadline_agents += 1;
            }
        }
        return st;
    }
}

// Origin: Pure-CP Stdlib Test Suite (lib/agent/skill_registry.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent Skill / Plugin Registry:
// registration deduplication (ID & name) & lifecycle management, capability bitmask + tag +
// priority/cost Top-K ranking, deterministic Kahn topological dependency activation order,
// cycle & missing-dependency rejection, per-skill concurrency quota acquire/release & stats.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SKILL_NO_DEP,
    SkillStatus,
    SkillDescriptor,
    SkillQuery,
    SkillMatch,
    SkillRegistryStats,
    skillTagHash,
    SkillRegistry,
} from "cap:agent";

fn testRegistrationDedupLifecycleAndLookup()! {
    let slots_arr: [3]SkillDescriptor = undefined;
    let slots: []SkillDescriptor = slots_arr[0..];
    let reg = SkillRegistry.init(slots);

    // 1. Valid registration
    const s1 = reg.register(SkillDescriptor{ id: 10, name: "web_search", version: 2, capability_mask: 0x01 });
    expect_eq(s1 != null, true);

    // 2. Duplicate ID, duplicate name, empty name, sentinel ID -> all rejected (null)
    expect_eq(reg.register(SkillDescriptor{ id: 10, name: "other" }) == null, true);
    expect_eq(reg.register(SkillDescriptor{ id: 11, name: "web_search" }) == null, true);
    expect_eq(reg.register(SkillDescriptor{ id: 12, name: "" }) == null, true);
    expect_eq(reg.register(SkillDescriptor{ id: SKILL_NO_DEP, name: "bad_id" }) == null, true);

    // 3. Fill remaining slots and verify pool-full rejection
    expect_eq(reg.register(SkillDescriptor{ id: 20, name: "code_exec" }) != null, true);
    expect_eq(reg.register(SkillDescriptor{ id: 30, name: "sql_read" }) != null, true);
    expect_eq(reg.register(SkillDescriptor{ id: 40, name: "overflow" }) == null, true);

    // 4. Lookup & lifecycle transitions
    if (found := reg.findByName("web_search")) {
        expect_eq(found.id, 10);
        expect_eq(found.version, 2);
    } else {
        expect_eq(true, false);
    }
    expect_eq(reg.disable(20), true);
    expect_eq(reg.setStatus(30, SkillStatus.Deprecated), true);

    const st = reg.stats();
    expect_eq(st.total_registered, 3);
    expect_eq(st.active_count, 1);
    expect_eq(st.disabled_count, 1);
    expect_eq(st.deprecated_count, 1);

    // 5. Unregister frees a slot for reuse
    expect_eq(reg.unregister(20), true);
    expect_eq(reg.findById(20) == null, true);
    expect_eq(reg.register(SkillDescriptor{ id: 40, name: "reused_slot" }) != null, true);
}

fn testCapabilityBitmaskTagAndCompositeTopK()! {
    let slots_arr: [6]SkillDescriptor = undefined;
    let slots: []SkillDescriptor = slots_arr[0..];
    let reg = SkillRegistry.init(slots);

    const cap_read: u64 = 1 << 0;
    const cap_write: u64 = 1 << 1;
    const cap_ast: u64 = 1 << 2;
    const cap_gpu: u64 = 1 << 3;

    const tag_code = skillTagHash("Coding");
    const tag_data = skillTagHash("Data");
    expect_eq(tag_code, skillTagHash("coding")); // Case-insensitive hash parity

    // Skill 1: has read+write, priority 5, cost 10 -> pref_hits=0, score = 50 - 10 = 40
    * = reg.register(SkillDescriptor{
        id: 1, name: "basic_rw", capability_mask: cap_read | cap_write,
        priority: 5, cost_units: 10, tag_hash_0: tag_code,
    });
    // Skill 2: has read+write+ast, priority 2, cost 20 -> pref_hits=1 (ast), score = 1000 + 20 - 20 = 1000
    * = reg.register(SkillDescriptor{
        id: 2, name: "ast_editor", capability_mask: cap_read | cap_write | cap_ast,
        priority: 2, cost_units: 20, tag_hash_0: tag_code,
    });
    // Skill 3: has read+write+ast+gpu, priority 10, cost 15 -> pref_hits=2, score = 2000 + 100 - 15 = 2085
    * = reg.register(SkillDescriptor{
        id: 3, name: "gpu_ast_pro", capability_mask: cap_read | cap_write | cap_ast | cap_gpu,
        priority: 10, cost_units: 15, tag_hash_0: tag_code,
    });
    // Skill 4: disabled -> must be excluded
    * = reg.register(SkillDescriptor{
        id: 4, name: "disabled_pro", capability_mask: cap_read | cap_write | cap_ast | cap_gpu,
        priority: 99, cost_units: 1, tag_hash_0: tag_code, status: SkillStatus.Disabled,
    });
    // Skill 5: wrong tag -> must be excluded when required_tag_hash == tag_code
    * = reg.register(SkillDescriptor{
        id: 5, name: "data_tool", capability_mask: cap_read | cap_write | cap_ast,
        priority: 50, cost_units: 1, tag_hash_0: tag_data,
    });

    let matches_arr: [4]SkillMatch = undefined;
    let matches: []SkillMatch = matches_arr[0..];

    const q = SkillQuery{
        required_caps: cap_read | cap_write,
        preferred_caps: cap_ast | cap_gpu,
        required_tag_hash: tag_code,
        max_cost_units: 25,
        require_available_slot: false,
        include_deprecated: false,
    };

    const n = reg.matchTopK(q, matches, 2);
    expect_eq(n, 2);
    expect_eq(matches[0].skill_id, 3); // score 2085
    expect_eq(matches[0].score, 2085);
    expect_eq(matches[1].skill_id, 2); // score 1000
    expect_eq(matches[1].score, 1000);
}

fn testDependencyTopologicalActivationOrder()! {
    let slots_arr: [5]SkillDescriptor = undefined;
    let slots: []SkillDescriptor = slots_arr[0..];
    let reg = SkillRegistry.init(slots);

    // Diamond DAG:
    //   1 (auth) -> no deps
    //   2 (db)   -> depends on 1
    //   3 (cache)-> depends on 1
    //   4 (rag)  -> depends on 2 and 3
    // Deterministic order (smallest skill_id tie-break): [1, 2, 3, 4]
    * = reg.register(SkillDescriptor{ id: 4, name: "rag", dep_0: 2, dep_1: 3 });
    * = reg.register(SkillDescriptor{ id: 3, name: "cache", dep_0: 1 });
    * = reg.register(SkillDescriptor{ id: 2, name: "db", dep_0: 1 });
    * = reg.register(SkillDescriptor{ id: 1, name: "auth" });

    let order_arr: [5]u32 = undefined;
    let indeg_arr: [5]u32 = undefined;
    let flags_arr: [5]usize = undefined;
    let order: []u32 = order_arr[0..];
    let indeg: []u32 = indeg_arr[0..];
    let flags: []usize = flags_arr[0..];

    if (n := reg.resolveActivationOrder(false, order, indeg, flags)) {
        expect_eq(n, 4);
        expect_eq(order[0], 1);
        expect_eq(order[1], 2);
        expect_eq(order[2], 3);
        expect_eq(order[3], 4);
    } else {
        expect_eq(true, false);
    }
}

fn testCycleSelfCycleAndMissingDependencyRejection()! {
    let slots_arr: [4]SkillDescriptor = undefined;
    let slots: []SkillDescriptor = slots_arr[0..];
    let reg = SkillRegistry.init(slots);

    let order_arr: [4]u32 = undefined;
    let indeg_arr: [4]u32 = undefined;
    let flags_arr: [4]usize = undefined;
    let order: []u32 = order_arr[0..];
    let indeg: []u32 = indeg_arr[0..];
    let flags: []usize = flags_arr[0..];

    // 1. 2-node cycle: 10 <-> 20 -> returns null
    * = reg.register(SkillDescriptor{ id: 10, name: "a", dep_0: 20 });
    * = reg.register(SkillDescriptor{ id: 20, name: "b", dep_0: 10 });
    expect_eq(reg.resolveActivationOrder(false, order, indeg, flags) == null, true);

    // 2. Self-dependency: 30 -> 30 -> returns null
    * = reg.unregister(10);
    * = reg.unregister(20);
    * = reg.register(SkillDescriptor{ id: 30, name: "self_dep", dep_0: 30 });
    expect_eq(reg.resolveActivationOrder(false, order, indeg, flags) == null, true);

    // 3. Missing dependency: 40 -> 999 (unregistered) -> returns null
    * = reg.unregister(30);
    * = reg.register(SkillDescriptor{ id: 40, name: "missing_dep", dep_0: 999 });
    expect_eq(reg.resolveActivationOrder(false, order, indeg, flags) == null, true);
}

fn testConcurrencyQuotaAcquireReleaseAndNamespace()! {
    let slots_arr: [4]cap.agent.SkillDescriptor = undefined;
    let slots: []cap.agent.SkillDescriptor = slots_arr[0..];
    let reg = cap.agent.SkillRegistry.init(slots);

    * = reg.register(cap.agent.SkillDescriptor{
        id: 7, name: "browser_nav", capability_mask: 0x01, max_concurrency: 2,
    });

    // Acquire up to max_concurrency=2, 3rd rejected
    expect_eq(reg.tryAcquire(7), true);
    expect_eq(reg.tryAcquire(7), true);
    expect_eq(reg.tryAcquire(7), false);

    // Active in-flight skill cannot be unregistered
    expect_eq(reg.unregister(7), false);

    // Match with require_available_slot = true skips saturated skill
    let matches_arr: [2]cap.agent.SkillMatch = undefined;
    let matches: []cap.agent.SkillMatch = matches_arr[0..];
    const q = cap.agent.SkillQuery{ required_caps: 0x01, require_available_slot: true };
    expect_eq(reg.matchTopK(q, matches, 2), 0);

    // Stats report 2 in-flight
    expect_eq(reg.stats().total_in_flight, 2);

    // Release frees slot and allows re-matching & safe unregister at 0
    expect_eq(reg.release(7), true);
    expect_eq(reg.matchTopK(q, matches, 2), 1);
    expect_eq(reg.release(7), true);
    // Extra release prevented (no underflow)
    expect_eq(reg.release(7), false);
    expect_eq(reg.unregister(7), true);
}

pub fn main()! {
    test("1. agent/skill_registry: registration dedup, pool limit & lifecycle", () => {
        testRegistrationDedupLifecycleAndLookup()?;
    });
    test("2. agent/skill_registry: capability bitmask, tag filter & composite Top-K ranking", () => {
        testCapabilityBitmaskTagAndCompositeTopK()?;
    });
    test("3. agent/skill_registry: deterministic Kahn topological activation order", () => {
        testDependencyTopologicalActivationOrder()?;
    });
    test("4. agent/skill_registry: cycle, self-dependency & missing-dependency rejection", () => {
        testCycleSelfCycleAndMissingDependencyRejection()?;
    });
    test("5. agent/skill_registry: concurrency quota acquire/release, stats & cap.agent", () => {
        testConcurrencyQuotaAcquireReleaseAndNamespace()?;
    });
}

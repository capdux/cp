// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/kg.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent Temporal Knowledge Graph Memory:
// entity deduplication, SPO triples, temporal fact superseding with point-in-time recall,
// 2-hop GraphRAG neighborhood expansion, and defensive boundary guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KgEntity,
    KgTriple,
    KgStore,
} from "cap:agentmem";

fn containsU32(slice: []const u32, val: u32): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (slice[i] == val) return true;
    }
    return false;
}

fn testEntityDeduplicationAndMultiPredicateFacts()! {
    let ent_buf: [8]KgEntity = undefined;
    let tri_buf: [16]KgTriple = undefined;
    let ent_slice: []KgEntity = ent_buf[0..];
    let tri_slice: []KgTriple = tri_buf[0..];

    let kg = KgStore.init(ent_slice, tri_slice);

    const alice_opt = kg.upsertEntity("Alice", "person");
    const alice_dup = kg.upsertEntity("Alice", "person");
    const cp_lang = kg.upsertEntity("CP", "language");
    const sydney = kg.upsertEntity("Sydney", "city");

    expect_eq(alice_opt != null, true);
    expect_eq(alice_dup != null, true);
    if (a1 := alice_opt) {
        if (a2 := alice_dup) {
            expect_eq(a1, a2);
            expect_eq(kg.findEntity("Alice") != null, true);
            expect_eq(kg.findEntity("Unknown") == null, true);
            if (cp_id := cp_lang) {
                if (syd_id := sydney) {
                    expect_eq(kg.addFact(a1, "uses", cp_id, 0.95, 100) != null, true);
                    expect_eq(kg.addFact(a1, "lives_in", syd_id, 0.90, 100) != null, true);

                    // Query all predicates (predicate == "") at current time (at_ms == 0)
                    let idx_buf: [8]usize = undefined;
                    let idx_slice: []usize = idx_buf[0..];
                    const n_all = kg.queryFactsAt(a1, "", 0, idx_slice);
                    expect_eq(n_all, 2);

                    // Query specific predicate
                    const n_uses = kg.queryFactsAt(a1, "uses", 0, idx_slice);
                    expect_eq(n_uses, 1);
                    expect_eq(tri_slice[idx_slice[0]].object, cp_id);
                } else {
                    expect_eq(true, false);
                }
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    }
}

fn testTemporalSupersedeAndPointInTimeRecall()! {
    let ent_buf: [8]KgEntity = undefined;
    let tri_buf: [16]KgTriple = undefined;
    let ent_slice: []KgEntity = ent_buf[0..];
    let tri_slice: []KgTriple = tri_buf[0..];

    let kg = KgStore.init(ent_slice, tri_slice);

    const alice: u32 = kg.upsertEntity("Alice", "person") ?? 999;
    const comp_a: u32 = kg.upsertEntity("CompanyA", "org") ?? 999;
    const comp_b: u32 = kg.upsertEntity("CompanyB", "org") ?? 999;
    const comp_c: u32 = kg.upsertEntity("CompanyC", "org") ?? 999;

    // t=100: Alice works_at CompanyA
    const f1_opt = kg.addFact(alice, "works_at", comp_a, 0.9, 100);
    expect_eq(f1_opt != null, true);

    // t=200: Alice supersedes works_at -> CompanyB (CompanyA invalidated at 200, preserved in history)
    const f2_opt = kg.supersedeFact(alice, "works_at", comp_b, 0.95, 200);
    expect_eq(f2_opt != null, true);

    // t=300: Alice supersedes works_at -> CompanyC
    const f3_opt = kg.supersedeFact(alice, "works_at", comp_c, 0.99, 300);
    expect_eq(f3_opt != null, true);

    let out_buf: [4]usize = undefined;
    let out_slice: []usize = out_buf[0..];

    // Before t=100 (t=50): no fact valid yet
    expect_eq(kg.queryFactsAt(alice, "works_at", 50, out_slice), 0);

    // At t=100 and t=150: works_at CompanyA
    expect_eq(kg.queryFactsAt(alice, "works_at", 100, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_a);
    expect_eq(tri_slice[out_slice[0]].active, false);
    expect_eq(tri_slice[out_slice[0]].valid_until_ms, 200);

    expect_eq(kg.queryFactsAt(alice, "works_at", 150, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_a);

    // At t=200 and t=250: works_at CompanyB
    expect_eq(kg.queryFactsAt(alice, "works_at", 200, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_b);
    expect_eq(kg.queryFactsAt(alice, "works_at", 250, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_b);

    // At t=300 and current (at_ms=0): works_at CompanyC
    expect_eq(kg.queryFactsAt(alice, "works_at", 300, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_c);
    expect_eq(kg.queryFactsAt(alice, "works_at", 0, out_slice), 1);
    expect_eq(tri_slice[out_slice[0]].object, comp_c);
    expect_eq(tri_slice[out_slice[0]].active, true);
    expect_eq(tri_slice[out_slice[0]].valid_until_ms, 0);
}

fn testTwoHopNeighborhoodAndInvalidatedEdgeExclusion()! {
    let ent_buf: [8]KgEntity = undefined;
    let tri_buf: [16]KgTriple = undefined;
    let ent_slice: []KgEntity = ent_buf[0..];
    let tri_slice: []KgTriple = tri_buf[0..];

    let kg = KgStore.init(ent_slice, tri_slice);

    const alice: u32 = kg.upsertEntity("Alice", "person") ?? 999;
    const old_org: u32 = kg.upsertEntity("OldOrg", "org") ?? 999;
    const new_org: u32 = kg.upsertEntity("NewOrg", "org") ?? 999;
    const proj_x: u32 = kg.upsertEntity("ProjectX", "project") ?? 999;
    const old_proj: u32 = kg.upsertEntity("OldProject", "project") ?? 999;
    const hop3_node: u32 = kg.upsertEntity("Hop3Node", "leaf") ?? 999;

    // t=100: Alice -> works_at -> OldOrg -> owns -> OldProject
    * = kg.addFact(alice, "works_at", old_org, 0.9, 100);
    * = kg.addFact(old_org, "owns", old_proj, 0.9, 100);

    // t=200: Alice supersedes works_at -> NewOrg; NewOrg <- part_of <- ProjectX; ProjectX -> depends -> Hop3Node
    * = kg.supersedeFact(alice, "works_at", new_org, 0.95, 200);
    * = kg.addFact(proj_x, "part_of", new_org, 0.9, 200); // reverse direction edge to verify undirected 2-hop
    * = kg.addFact(proj_x, "depends", hop3_node, 0.9, 200);

    let out_arr: [8]u32 = undefined;
    let out_slice: []u32 = out_arr[0..];
    let visited_arr: [8]u8 = undefined;
    let visited_slice: []u8 = visited_arr[0..];

    // Current (at_ms == 0): 1-hop = {NewOrg}, 2-hop = {ProjectX}.
    // Must exclude Alice (start), OldOrg & OldProject (invalidated edge), and Hop3Node (3 hops away).
    const n_now = kg.queryTwoHopNeighbors(alice, 0, out_slice, visited_slice);
    expect_eq(n_now, 2);
    expect_eq(containsU32(out_slice[0..n_now], new_org), true);
    expect_eq(containsU32(out_slice[0..n_now], proj_x), true);
    expect_eq(containsU32(out_slice[0..n_now], alice), false);
    expect_eq(containsU32(out_slice[0..n_now], old_org), false);
    expect_eq(containsU32(out_slice[0..n_now], old_proj), false);
    expect_eq(containsU32(out_slice[0..n_now], hop3_node), false);

    // Point-in-time t=150: 1-hop = {OldOrg}, 2-hop = {OldProject}.
    const n_past = kg.queryTwoHopNeighbors(alice, 150, out_slice, visited_slice);
    expect_eq(n_past, 2);
    expect_eq(containsU32(out_slice[0..n_past], old_org), true);
    expect_eq(containsU32(out_slice[0..n_past], old_proj), true);
    expect_eq(containsU32(out_slice[0..n_past], new_org), false);
}

fn testDefensiveGuardsPoolLimitsAndCapNamespace()! {
    let ent_buf: [2]KgEntity = undefined;
    let tri_buf: [1]KgTriple = undefined;
    let ent_slice: []KgEntity = ent_buf[0..];
    let tri_slice: []KgTriple = tri_buf[0..];

    let kg = cap.agentmem.KgStore.init(ent_slice, tri_slice);

    // 1. Empty entity name rejected; pool capacity=2 enforced
    expect_eq(kg.upsertEntity("", "kind") == null, true);
    const e0: u32 = kg.upsertEntity("E0", "node") ?? 999;
    const e1: u32 = kg.upsertEntity("E1", "node") ?? 999;
    expect_eq(kg.upsertEntity("E2", "node") == null, true); // entity pool full

    // 2. Invalid confidence (NaN, < 0, > 1), invalid entity IDs, empty predicate
    const nan_val: f64 = std.math.nan(f64);
    expect_eq(kg.addFact(e0, "rel", e1, nan_val, 100) == null, true);
    expect_eq(kg.addFact(e0, "rel", e1, -0.1, 100) == null, true);
    expect_eq(kg.addFact(e0, "rel", e1, 1.1, 100) == null, true);
    expect_eq(kg.addFact(e0, "", e1, 0.8, 100) == null, true);
    expect_eq(kg.addFact(99, "rel", e1, 0.8, 100) == null, true);
    expect_eq(kg.addFact(e0, "rel", 99, 0.8, 100) == null, true);

    // 3. Fill triple pool (capacity=1) and verify supersedeFact atomically rejects WITHOUT invalidating e0->e1
    expect_eq(kg.addFact(e0, "rel", e1, 0.8, 100) != null, true);
    expect_eq(kg.addFact(e1, "rel", e0, 0.8, 150) == null, true); // pool full
    expect_eq(kg.supersedeFact(e0, "rel", e0, 0.9, 200) == null, true); // pool full -> atomic reject
    expect_eq(tri_slice[0].active, true);
    expect_eq(tri_slice[0].valid_until_ms, 0);

    // 4. Short visited_s scratch buffer & invalid start_entity return 0 safely
    let out_arr: [4]u32 = undefined;
    let out_slice: []u32 = out_arr[0..];
    let tiny_vis: [1]u8 = undefined;
    let tiny_vis_slice: []u8 = tiny_vis[0..];
    expect_eq(kg.queryTwoHopNeighbors(e0, 0, out_slice, tiny_vis_slice), 0);
    let ok_vis: [4]u8 = undefined;
    let ok_vis_slice: []u8 = ok_vis[0..];
    expect_eq(kg.queryTwoHopNeighbors(99, 0, out_slice, ok_vis_slice), 0);
}

fn testMultiFactSupersedeIsolationAndOutputTruncation()! {
    let ent_buf: [8]KgEntity = undefined;
    let tri_buf: [16]KgTriple = undefined;
    let ent_slice: []KgEntity = ent_buf[0..];
    let tri_slice: []KgTriple = tri_buf[0..];

    let kg = KgStore.init(ent_slice, tri_slice);

    const bob: u32 = kg.upsertEntity("Bob", "person") ?? 999;
    const r1: u32 = kg.upsertEntity("RoleA", "role") ?? 999;
    const r2: u32 = kg.upsertEntity("RoleB", "role") ?? 999;
    const r3: u32 = kg.upsertEntity("RoleC", "role") ?? 999;
    const city: u32 = kg.upsertEntity("Tokyo", "city") ?? 999;

    // Two active facts on same (Bob, "role") + one unrelated fact (Bob, "city")
    * = kg.addFact(bob, "role", r1, 0.8, 100);
    * = kg.addFact(bob, "role", r2, 0.85, 110);
    * = kg.addFact(bob, "city", city, 0.99, 100);

    // Supersede (Bob, "role") -> RoleC at t=200: BOTH RoleA and RoleB invalidated, "city" untouched!
    * = kg.supersedeFact(bob, "role", r3, 0.95, 200);

    let idx_buf: [4]usize = undefined;
    let idx_slice: []usize = idx_buf[0..];
    expect_eq(kg.queryFactsAt(bob, "role", 0, idx_slice), 1);
    expect_eq(tri_slice[idx_slice[0]].object, r3);
    expect_eq(kg.queryFactsAt(bob, "city", 0, idx_slice), 1);
    expect_eq(tri_slice[idx_slice[0]].object, city);

    // Truncated out_indices slice (len=1) safely caps output at 1 even when 2 active facts exist
    let cap1_slice: []usize = idx_buf[0..1];
    expect_eq(kg.queryFactsAt(bob, "", 0, cap1_slice), 1);
}

fn main() {
    test("1. agentmem/kg: entity deduplication & multi-predicate SPO triple queries", () => {
        testEntityDeduplicationAndMultiPredicateFacts()?;
    });

    test("2. agentmem/kg: temporal supersedeFact invalidation & point-in-time history recall", () => {
        testTemporalSupersedeAndPointInTimeRecall()?;
    });

    test("3. agentmem/kg: 2-hop GraphRAG neighborhood recall excluding invalidated edges & 3-hop nodes", () => {
        testTwoHopNeighborhoodAndInvalidatedEdgeExclusion()?;
    });

    test("4. agentmem/kg: pool capacity atomicity, NaN/out-of-range confidence & cap.agentmem namespace", () => {
        testDefensiveGuardsPoolLimitsAndCapNamespace()?;
    });

    test("5. agentmem/kg: multi-fact supersede predicate isolation & truncated output slice safety", () => {
        testMultiFactSupersedeIsolationAndOutputTruncation()?;
    });
}

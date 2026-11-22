// Origin: Pure-CP Stdlib Test Suite (lib/sys/drfSched.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Heterogeneous Cluster Weighted DRF, Vector Bin-Packing, Preemption & EDF:
//   1. Classic Ghodsi et al. (NSDI 2011) DRF benchmark (<9 CPU, 18 GB>, <1,4> vs <3,1>) & Jain Fairness == 1.0
//   2. Weighted DRF weights, `min_guarantee` priority tier & `max_quota` capping
//   3. 4 multi-dimensional placement strategies (`first_fit`, `best_fit_remaining`, `dot_product_align`, `worst_fit_spread`)
//   4. Kubernetes-style Taint/Toleration bitmask & Cordon maintenance isolation
//   5. Minimum-cost priority preemption eviction planner (`preemptible` & priority guards)
//   6. Event-driven preemptive EDF real-time scheduling & Liu-Layland schedulability test
//   7. Scale test (128 nodes, 64 tasks), `DrfError` validation guards & `cap.sys` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    DRF_MAX_RES_DIMS,
    DrfError,
    ResourceVec,
    validateClusterCapacity,
    DrfTenant,
    DrfAllocDecision,
    drfScheduleNext,
    drfScheduleBatch,
    computeJainFairnessIndex,
    PlacementStrategy,
    ClusterNode,
    PlacementOutcome,
    placeTaskOnNodes,
    placeTasksBatch,
    RunningWorkload,
    planNodePreemption,
    EdfTask,
    checkLiuLaylandSchedulable,
    simulateEdfSchedule,
} from "cap:sys";

fn absDiff(a: f64, b: f64): f64 {
    const d: f64 = a - b;
    return if (d < 0.0) -d else d;
}

test("DRF: Ghodsi NSDI 2011 classic benchmark & Jain fairness == 1.0") {
    // Cluster: 9 CPU, 18 GB RAM
    // Tenant A: <1 CPU, 4 GB> per task (dominant: RAM = 4/18)
    // Tenant B: <3 CPU, 1 GB> per task (dominant: CPU = 3/9)
    // Optimal DRF allocation: A gets 3 tasks (<3, 12>, share=2/3), B gets 2 tasks (<6, 2>, share=2/3)
    const cluster = ResourceVec.from2(9.0, 18.0);
    let rem = cluster;

    let tenants: [2]DrfTenant = [
        DrfTenant{
            tenant_id: 1,
            weight: 1.0,
            allocated: ResourceVec.zero(2),
            pending_demand: ResourceVec.from2(1.0, 4.0),
        },
        DrfTenant{
            tenant_id: 2,
            weight: 1.0,
            allocated: ResourceVec.zero(2),
            pending_demand: ResourceVec.from2(3.0, 1.0),
        },
    ];
    let decisions: [16]DrfAllocDecision = [DrfAllocDecision{}; 16];
    let t_s: []DrfTenant = tenants[0..];
    let d_s: []DrfAllocDecision = decisions[0..];

    const total_alloc = cap.sys.drfScheduleBatch(*t_s, cluster, *rem, *d_s);
    expect_eq(total_alloc, @as(usize, 5));
    expect_eq(tenants[0].tasks_assigned, @as(usize, 3));
    expect_eq(tenants[1].tasks_assigned, @as(usize, 2));

    expect_true(absDiff(tenants[0].allocated.values[0], 3.0) < 1.0e-9);
    expect_true(absDiff(tenants[0].allocated.values[1], 12.0) < 1.0e-9);
    expect_true(absDiff(tenants[1].allocated.values[0], 6.0) < 1.0e-9);
    expect_true(absDiff(tenants[1].allocated.values[1], 2.0) < 1.0e-9);

    expect_true(absDiff(tenants[0].effectiveDominantShare(cluster), 12.0 / 18.0) < 1.0e-9);
    expect_true(absDiff(tenants[1].effectiveDominantShare(cluster), 6.0 / 9.0) < 1.0e-9);
    expect_true(absDiff(rem.values[0], 0.0) < 1.0e-9);
    expect_true(absDiff(rem.values[1], 4.0) < 1.0e-9);

    const jain = computeJainFairnessIndex(tenants[0..], cluster);
    expect_true(absDiff(jain, 1.0) < 1.0e-9);
}

test("DRF: weighted fairness, min_guarantee precedence & max_quota enforcement") {
    const cluster = ResourceVec.from2(20.0, 20.0);
    let rem = cluster;

    let tenants: [3]DrfTenant = [
        // Tenant 1: weight=2.0, capped at max_quota=<6, 6>
        DrfTenant{
            tenant_id: 10,
            weight: 2.0,
            max_quota: ResourceVec.from2(6.0, 6.0),
            allocated: ResourceVec.zero(2),
            pending_demand: ResourceVec.from2(2.0, 2.0),
        },
        // Tenant 2: weight=1.0, uncapped
        DrfTenant{
            tenant_id: 20,
            weight: 1.0,
            allocated: ResourceVec.zero(2),
            pending_demand: ResourceVec.from2(2.0, 2.0),
        },
        // Tenant 3: weight=1.0, strict min_guarantee=<4, 4>
        DrfTenant{
            tenant_id: 30,
            weight: 1.0,
            min_guarantee: ResourceVec.from2(4.0, 4.0),
            allocated: ResourceVec.zero(2),
            pending_demand: ResourceVec.from2(2.0, 2.0),
        },
    ];
    let t_s: []DrfTenant = tenants[0..];

    // First two allocations MUST go to Tenant 30 to satisfy min_guarantee=<4, 4>
    if (d0 := drfScheduleNext(*t_s, cluster, *rem)) {
        expect_eq(d0.tenant_id, @as(u32, 30));
    } else {
        expect_true(false);
    }
    if (d1 := drfScheduleNext(*t_s, cluster, *rem)) {
        expect_eq(d1.tenant_id, @as(u32, 30));
    } else {
        expect_true(false);
    }
    expect_true(!tenants[2].isBelowMinGuarantee());

    // Drain remaining cluster capacity (8 more units of <2,2>)
    let decs: [16]DrfAllocDecision = [DrfAllocDecision{}; 16];
    let d_s: []DrfAllocDecision = decs[0..];
    const more = drfScheduleBatch(*t_s, cluster, *rem, *d_s);
    expect_eq(more, @as(usize, 8));

    // Tenant 10 strictly capped at max_quota=<6,6> (3 tasks)
    expect_eq(tenants[0].tasks_assigned, @as(usize, 3));
    expect_true(tenants[0].allocated.fitsWithin(tenants[0].max_quota));
    // Remaining 7 tasks split cleanly across Tenant 20 and Tenant 30
    expect_eq(tenants[1].tasks_assigned + tenants[2].tasks_assigned, @as(usize, 7));
}

test("Placement: dot_product_align vs first_fit, best_fit_remaining & worst_fit_spread") {
    // Node 0: high free RAM (56/64), scarce CPU/GPU (4/16, 1/4)
    // Node 1: high free CPU/GPU (14/16, 4/4), tighter RAM (16/64)
    const base_nodes: [2]ClusterNode = [
        ClusterNode{
            node_id: 100,
            capacity: ResourceVec.from3(16.0, 64.0, 4.0),
            used: ResourceVec.from3(12.0, 8.0, 3.0),
        },
        ClusterNode{
            node_id: 200,
            capacity: ResourceVec.from3(16.0, 64.0, 4.0),
            used: ResourceVec.from3(2.0, 48.0, 0.0),
        },
    ];
    // Incoming task: CPU+GPU heavy, low RAM -> aligns strongly with Node 1 (id 200)
    const gpu_req = ResourceVec.from3(4.0, 4.0, 1.0);

    // 1. first_fit picks Node 0 (id 100)
    let n_ff: [2]ClusterNode = base_nodes;
    let n_ff_s: []ClusterNode = n_ff[0..];
    const o_ff = placeTaskOnNodes(*n_ff_s, gpu_req, 0, PlacementStrategy.first_fit);
    expect_true(o_ff.placed);
    expect_eq(o_ff.node_id, @as(u32, 100));

    // 2. dot_product_align picks Node 1 (id 200) to avoid stranding Node 0's last GPU/CPU
    let n_dp: [2]ClusterNode = base_nodes;
    let n_dp_s: []ClusterNode = n_dp[0..];
    const o_dp = placeTaskOnNodes(*n_dp_s, gpu_req, 0, PlacementStrategy.dot_product_align);
    expect_true(o_dp.placed);
    expect_eq(o_dp.node_id, @as(u32, 200));
    expect_true(o_dp.score > 0.8);

    // 3. best_fit_remaining picks tighter node (Node 0, lower mean remaining after placement)
    let n_bf: [2]ClusterNode = base_nodes;
    let n_bf_s: []ClusterNode = n_bf[0..];
    const o_bf = placeTaskOnNodes(*n_bf_s, gpu_req, 0, PlacementStrategy.best_fit_remaining);
    expect_true(o_bf.placed);
    expect_eq(o_bf.node_id, @as(u32, 100));

    // 4. worst_fit_spread picks roomier node (Node 1, higher mean remaining after placement)
    let n_wf: [2]ClusterNode = base_nodes;
    let n_wf_s: []ClusterNode = n_wf[0..];
    const o_wf = placeTaskOnNodes(*n_wf_s, gpu_req, 0, PlacementStrategy.worst_fit_spread);
    expect_true(o_wf.placed);
    expect_eq(o_wf.node_id, @as(u32, 200));
}

test("Placement: taint/toleration bitmasks & cordon isolation") {
    const TAINT_GPU: u64 = 1 << 0;
    const TAINT_SPOT: u64 = 1 << 1;

    let nodes: [3]ClusterNode = [
        ClusterNode{
            node_id: 1,
            capacity: ResourceVec.from2(16.0, 32.0),
            used: ResourceVec.zero(2),
            cordoned: true, // Maintenance mode
        },
        ClusterNode{
            node_id: 2,
            capacity: ResourceVec.from2(16.0, 32.0),
            used: ResourceVec.zero(2),
            taint_mask: TAINT_GPU | TAINT_SPOT,
        },
        ClusterNode{
            node_id: 3,
            capacity: ResourceVec.from2(4.0, 8.0),
            used: ResourceVec.zero(2),
        },
    ];
    let n_s: []ClusterNode = nodes[0..];

    const big_req = ResourceVec.from2(8.0, 16.0);
    // Missing TAINT_SPOT -> cannot fit Node 1 (cordoned), Node 2 (taint), or Node 3 (too small)
    const denied = placeTaskOnNodes(*n_s, big_req, TAINT_GPU, PlacementStrategy.first_fit);
    expect_true(!denied.placed);

    // Tolerating both taints succeeds on Node 2
    const allowed = placeTaskOnNodes(*n_s, big_req, TAINT_GPU | TAINT_SPOT, PlacementStrategy.first_fit);
    expect_true(allowed.placed);
    expect_eq(allowed.node_id, @as(u32, 2));
}

test("Preemption: minimum-cost priority eviction respecting non-preemptible & high-priority tasks") {
    const node = ClusterNode{
        node_id: 7,
        capacity: ResourceVec.from2(16.0, 32.0),
        used: ResourceVec.from2(14.0, 28.0), // remaining: <2, 4>
    };
    const running: [4]RunningWorkload = [
        RunningWorkload{ task_id: 101, tenant_id: 1, node_id: 7, priority: 10, usage: ResourceVec.from2(4.0, 8.0), preemptible: true },
        RunningWorkload{ task_id: 102, tenant_id: 1, node_id: 7, priority: 20, usage: ResourceVec.from2(4.0, 8.0), preemptible: true },
        RunningWorkload{ task_id: 103, tenant_id: 2, node_id: 7, priority: 5, usage: ResourceVec.from2(4.0, 8.0), preemptible: false },
        RunningWorkload{ task_id: 104, tenant_id: 3, node_id: 7, priority: 90, usage: ResourceVec.from2(2.0, 4.0), preemptible: true },
    ];

    let victims: [8]u64 = [0; 8];
    let v_s: []u64 = victims[0..];

    // Incoming prio=50 needs <8, 16> (deficit <6, 12>): must evict 101 (prio 10) then 102 (prio 20), skipping 103 & 104
    const plan = planNodePreemption(node, ResourceVec.from2(8.0, 16.0), 50, running[0..], *v_s);
    expect_true(plan.feasible);
    expect_eq(plan.victim_count, @as(usize, 2));
    expect_eq(victims[0], @as(u64, 101));
    expect_eq(victims[1], @as(u64, 102));
    expect_eq(plan.min_victim_priority, @as(i32, 10));
    expect_true(absDiff(plan.freed_resources.values[0], 8.0) < 1.0e-9);

    // Incoming prio=50 needing <12, 24> cannot succeed (only <8, 16> preemptible -> max avail <10, 20>)
    const infeasible = planNodePreemption(node, ResourceVec.from2(12.0, 24.0), 50, running[0..], *v_s);
    expect_true(!infeasible.feasible);
}

test("EDF: Liu-Layland schedulability & event-driven preemptive deadline simulation") {
    const exec_ok: [3]u64 = [1, 2, 2];
    const period_ok: [3]u64 = [4, 5, 10]; // util = 0.85 <= 1.0
    expect_true(checkLiuLaylandSchedulable(exec_ok[0..], period_ok[0..]));

    const exec_over: [2]u64 = [2, 3];
    const period_over: [2]u64 = [4, 5]; // util = 1.1 > 1.0
    expect_true(!checkLiuLaylandSchedulable(exec_over[0..], period_over[0..]));

    // Preemptive simulation:
    // Task 1 released at t=0, exec=10, deadline=30
    // Task 2 released at t=3, exec=4, deadline=10 (preempts Task 1 at t=3, finishes at t=7 <= 10!)
    // Task 1 resumes at t=7, finishes at t=14 <= 30!
    let tasks: [2]EdfTask = [
        EdfTask{ task_id: 1, release_ms: 0, deadline_ms: 30, exec_ms: 10 },
        EdfTask{ task_id: 2, release_ms: 3, deadline_ms: 10, exec_ms: 4 },
    ];
    let tk_s: []EdfTask = tasks[0..];
    const rep = simulateEdfSchedule(*tk_s, 20);
    expect_eq(rep.total_tasks, @as(usize, 2));
    expect_eq(rep.completed_on_time, @as(usize, 2));
    expect_eq(rep.missed_deadlines, @as(usize, 0));
    expect_eq(rep.total_busy_ms, @as(u64, 14));
    expect_eq(rep.utilization_bps, @as(u32, 7000));

    // Overload scenario: 2 tasks needing 8ms each before deadline 10ms -> 1 misses
    let overload: [2]EdfTask = [
        EdfTask{ task_id: 10, release_ms: 0, deadline_ms: 10, exec_ms: 8 },
        EdfTask{ task_id: 20, release_ms: 0, deadline_ms: 10, exec_ms: 8 },
    ];
    let ov_s: []EdfTask = overload[0..];
    const rep_ov = simulateEdfSchedule(*ov_s, 20);
    expect_eq(rep_ov.completed_on_time, @as(usize, 1));
    expect_eq(rep_ov.missed_deadlines, @as(usize, 1));
}

test("Scale & Guards: 128-node batch placement, DrfError validation & cap.sys facade parity") {
    expect_eq(DRF_MAX_RES_DIMS, @as(usize, 8));

    // Validate cluster capacity guards
    let got_bad_dim: bool = false;
    validateClusterCapacity(ResourceVec.zero(0)) catch |e| {
        got_bad_dim = (e == DrfError.InvalidDimensions);
    };
    expect_true(got_bad_dim);

    let got_zero_cap: bool = false;
    cap.sys.validateClusterCapacity(ResourceVec.zero(3)) catch |e| {
        got_zero_cap = (e == DrfError.ZeroClusterCapacity);
    };
    expect_true(got_zero_cap);

    // 128 heterogeneous nodes, 64 tasks placed via external caller slices
    let nodes: [128]ClusterNode = [ClusterNode{}; 128];
    let i: usize = 0;
    while (i < 128) : (i += 1) {
        nodes[i] = ClusterNode{
            node_id: @as(u32, @intCast(i + 1)),
            capacity: ResourceVec.from4(16.0, 64.0, 4.0, 10.0),
            used: ResourceVec.zero(4),
        };
    }
    let reqs: [64]ResourceVec = [ResourceVec.from4(4.0, 16.0, 1.0, 2.5); 64];
    let tols: [64]u64 = [0; 64];
    let outcomes: [64]PlacementOutcome = [PlacementOutcome{}; 64];
    let n_s: []ClusterNode = nodes[0..];
    let o_s: []PlacementOutcome = outcomes[0..];

    const placed_n = placeTasksBatch(*n_s, reqs[0..], tols[0..], PlacementStrategy.worst_fit_spread, *o_s);
    expect_eq(placed_n, @as(usize, 64));
    expect_true(outcomes[0].placed);
    expect_true(outcomes[63].placed);
}

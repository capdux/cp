// Origin: Pure-CP Stdlib Test Suite (lib/agent/hitl.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation Human-in-the-Loop (HITL) approval queue:
// risk-tiered auto-approval, single-approval Medium/High workflows, Critical dual-sign quorum,
// mid-review rejection & cancellation, SLA expiration via approve/sweepExpired with irreversible
// terminal states, audit timestamps, and duplicate ID / empty action / full pool / unknown ID guards.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ApprovalState,
    ActionRisk,
    ApprovalRequest,
    HitlPolicy,
    HitlQueue,
} from "cap:agent";

fn testAutoApproveAndSingleApprovalWorkflow()! {
    let pool_arr: [8]ApprovalRequest = undefined;
    let pool: []ApprovalRequest = pool_arr[0..];
    const policy = HitlPolicy{
        auto_approve_max_risk: ActionRisk.Low,
        default_sla_ms: 60000,
        critical_quorum: 2,
    };
    let q = HitlQueue.init(pool, policy);

    // 1. Low risk -> immediate auto-approval
    if (st := q.submit(100, 1, "read_config", 0xAAAA, ActionRisk.Low, 1000)) {
        expect_eq(st == ApprovalState.Approved, true);
    } else {
        expect_eq(true, false);
    }
    if (rec := q.findRequest(100)) {
        expect_eq(rec.state == ApprovalState.Approved, true);
        expect_eq(rec.approvals_received, 1);
        expect_eq(rec.resolved_ms, 1000);
    } else {
        expect_eq(true, false);
    }

    // 2. Medium & High risk -> Pending, approved on 1st human signature
    if (st_m := q.submit(101, 2, "update_cache", 0xBBBB, ActionRisk.Medium, 2000)) {
        expect_eq(st_m == ApprovalState.Pending, true);
    } else {
        expect_eq(true, false);
    }
    if (st_h := q.submit(102, 2, "restart_worker", 0xCCCC, ActionRisk.High, 2000)) {
        expect_eq(st_h == ApprovalState.Pending, true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(q.pendingCount(2500), 2);

    if (st_m2 := q.approve(101, 3000)) {
        expect_eq(st_m2 == ApprovalState.Approved, true);
    } else {
        expect_eq(true, false);
    }
    if (st_h2 := q.approve(102, 4000)) {
        expect_eq(st_h2 == ApprovalState.Approved, true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(q.pendingCount(4500), 0);

    if (rec_h := q.findRequest(102)) {
        expect_eq(rec_h.resolved_ms, 4000);
        expect_eq(rec_h.deadline_ms, 62000);
    } else {
        expect_eq(true, false);
    }
}

fn testCriticalDualSignQuorumAndMidReviewRejection()! {
    let pool_arr: [8]ApprovalRequest = undefined;
    let pool: []ApprovalRequest = pool_arr[0..];
    let q = HitlQueue.init(pool, HitlPolicy.default());

    // 1. Critical action requires 2 approvals (quorum = 2)
    if (st0 := q.submit(200, 7, "drop_staging_table", 0x1111, ActionRisk.Critical, 10000)) {
        expect_eq(st0 == ApprovalState.Pending, true);
    } else {
        expect_eq(true, false);
    }

    // First approval -> still Pending (1/2)
    if (st1 := q.approve(200, 11000)) {
        expect_eq(st1 == ApprovalState.Pending, true);
    } else {
        expect_eq(true, false);
    }
    if (r1 := q.findRequest(200)) {
        expect_eq(r1.required_approvals, 2);
        expect_eq(r1.approvals_received, 1);
        expect_eq(r1.resolved_ms, 0);
    } else {
        expect_eq(true, false);
    }

    // Second approval -> Approved (2/2)
    if (st2 := q.approve(200, 12000)) {
        expect_eq(st2 == ApprovalState.Approved, true);
    } else {
        expect_eq(true, false);
    }
    if (r2 := q.findRequest(200)) {
        expect_eq(r2.approvals_received, 2);
        expect_eq(r2.resolved_ms, 12000);
    } else {
        expect_eq(true, false);
    }

    // 2. Mid-review rejection after 1/2 approvals immediately transitions to Rejected
    if (st_r := q.submit(201, 7, "rotate_prod_key", 0x2222, ActionRisk.Critical, 10000)) {
        expect_eq(st_r == ApprovalState.Pending, true);
    } else {
        expect_eq(true, false);
    }
    * = q.approve(201, 10500);
    expect_eq(q.reject(201, 10600), true);
    // Irreversible: subsequent approve or cancel cannot alter Rejected state
    if (st_after := q.approve(201, 10700)) {
        expect_eq(st_after == ApprovalState.Rejected, true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(q.cancel(201, 10800), false);
    if (rr := q.findRequest(201)) {
        expect_eq(rr.state == ApprovalState.Rejected, true);
        expect_eq(rr.resolved_ms, 10600);
    } else {
        expect_eq(true, false);
    }
}

fn testSlaExpirationSweepAndIrreversibility()! {
    let pool_arr: [8]ApprovalRequest = undefined;
    let pool: []ApprovalRequest = pool_arr[0..];
    const policy = HitlPolicy{
        auto_approve_max_risk: ActionRisk.Low,
        default_sla_ms: 5000,
        critical_quorum: 2,
    };
    let q = HitlQueue.init(pool, policy);

    * = q.submit(301, 3, "deploy_a", 0x1, ActionRisk.High, 1000); // deadline 6000
    * = q.submit(302, 3, "deploy_b", 0x2, ActionRisk.High, 2000); // deadline 7000
    * = q.submit(303, 3, "deploy_c", 0x3, ActionRisk.High, 5000); // deadline 10000

    expect_eq(q.pendingCount(5500), 3);

    // 1. Calling approve past deadline on 301 transitions directly to Expired
    if (st_exp := q.approve(301, 6000)) {
        expect_eq(st_exp == ApprovalState.Expired, true);
    } else {
        expect_eq(true, false);
    }
    // Irreversible: cannot reject or approve again
    expect_eq(q.reject(301, 6100), false);
    if (st_exp2 := q.approve(301, 6200)) {
        expect_eq(st_exp2 == ApprovalState.Expired, true);
    } else {
        expect_eq(true, false);
    }

    // 2. Batch sweep at t=8000 expires 302 (deadline 7000) but leaves 303 (deadline 10000) Pending
    const swept = q.sweepExpired(8000);
    expect_eq(swept, 1);
    expect_eq(q.pendingCount(8000), 1);

    if (r302 := q.findRequest(302)) {
        expect_eq(r302.state == ApprovalState.Expired, true);
        expect_eq(r302.resolved_ms, 8000);
    } else {
        expect_eq(true, false);
    }

    // 3. Cancel active 303 before deadline succeeds
    expect_eq(q.cancel(303, 9000), true);
    expect_eq(q.pendingCount(9000), 0);
    if (r303 := q.findRequest(303)) {
        expect_eq(r303.state == ApprovalState.Cancelled, true);
        expect_eq(r303.resolved_ms, 9000);
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsDuplicatesPoolFullAndUnknownId()! {
    let pool_arr: [2]ApprovalRequest = undefined;
    let pool: []ApprovalRequest = pool_arr[0..];
    let q = HitlQueue.init(pool, HitlPolicy.default());

    // 1. Empty action name rejected
    expect_eq(q.submit(1, 1, "", 0x0, ActionRisk.Medium, 100) == null, true);

    // 2. Fill pool of capacity 2
    expect_eq(q.submit(1, 1, "op_one", 0x1, ActionRisk.Medium, 100) != null, true);
    // Duplicate ID rejected
    expect_eq(q.submit(1, 1, "op_dup", 0x2, ActionRisk.Medium, 100) == null, true);
    expect_eq(q.submit(2, 1, "op_two", 0x3, ActionRisk.Medium, 100) != null, true);
    // Pool full rejected
    expect_eq(q.submit(3, 1, "op_three", 0x4, ActionRisk.Medium, 100) == null, true);

    // 3. Unknown ID guards
    expect_eq(q.approve(999, 200) == null, true);
    expect_eq(q.reject(999, 200), false);
    expect_eq(q.cancel(999, 200), false);
    expect_eq(q.findRequest(999) == null, true);
}

fn testCapAgentNamespaceIntegration()! {
    let pool_arr: [4]cap.agent.ApprovalRequest = undefined;
    let pool: []cap.agent.ApprovalRequest = pool_arr[0..];
    let q = cap.agent.HitlQueue.init(pool, cap.agent.HitlPolicy.default());
    if (st := q.submit(42, 9, "send_email", 0xBEEF, cap.agent.ActionRisk.Medium, 500)) {
        expect_eq(st == cap.agent.ApprovalState.Pending, true);
        if (st2 := q.approve(42, 600)) {
            expect_eq(st2 == cap.agent.ApprovalState.Approved, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. agent/hitl: Low auto-approve & Medium/High single-approval workflow", () => {
        testAutoApproveAndSingleApprovalWorkflow()?;
    });
    test("2. agent/hitl: Critical dual-sign quorum & irreversible mid-review rejection", () => {
        testCriticalDualSignQuorumAndMidReviewRejection()?;
    });
    test("3. agent/hitl: SLA expiration on approve, batch sweepExpired & cancel", () => {
        testSlaExpirationSweepAndIrreversibility()?;
    });
    test("4. agent/hitl: empty action, duplicate ID, full pool & unknown ID guards", () => {
        testDefensiveGuardsDuplicatesPoolFullAndUnknownId()?;
    });
    test("5. agent/hitl: cap.agent namespace integration", () => {
        testCapAgentNamespaceIntegration()?;
    });
}

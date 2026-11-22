// Origin: Pure-CP Stdlib Test Suite (lib/mcp/batch_dispatcher.cp) | Created: 2026-11-22
// Production Deep Verification Suite for JSON-RPC 2.0 / MCP Batch Request Correlator:
//   1. Mixed RPC + Notification batch: out-of-order recording + submission/completion reordering + latency stats
//   2. All-notification batch: JSON-RPC 2.0 wire-suppression (`requiresWireResponse() == false`)
//   3. Partial deadline timeout sweep (`expirePending`) injecting `-32001` & rejecting late results
//   4. Empty batch guard (`invalid_empty_batch`) per JSON-RPC 2.0 §6
//   5. Duplicate request_id rejection, unknown request_id rejection & double-record rejection
//   6. Full 64-item capacity boundary (`MCP_BATCH_MAX_ITEMS = 64`)
//   7. Post-seal mutation guards & undersized output buffer truncation safety
//   8. `getItemByRequestId` lookup & error_code normalization when `is_error == false`

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MCP_BATCH_MAX_ITEMS,
    MCP_BATCH_ERR_TIMEOUT,
    MCP_BATCH_ERR_INVALID_REQUEST,
    McpBatchEntryKind,
    McpBatchItemState,
    McpBatchSubmitStatus,
    McpBatchRecordStatus,
    McpBatchOrderMode,
    McpBatchItem,
    McpBatchResponseItem,
    McpBatchSummary,
    McpBatchDispatcher,
} from "cap:mcp";

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: Mixed RPC + Notification Out-of-Order Reassembly & Latency Summary
// ============================================================================

fn testMixedBatchOutOfOrderReassembly()! {
    let batch = cap.mcp.McpBatchDispatcher.init(9001);

    // Submit: [req(10), notif, req(20), req(30), notif]
    expect_eq(batch.addRequest(10, 0xA1, 1000) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addNotification(0xB1, 1000) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addRequest(20, 0xA2, 1000) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addRequest(30, 0xA3, 1000) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addNotification(0xB2, 1000) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.ok, true);

    expect_eq(batch.requiresWireResponse(), true);
    expect_eq(batch.isComplete(), false);

    // Out-of-order completion: 30 (ok, 400us) -> 10 (ok, 100us) -> 20 (err -32602, 700us)
    expect_eq(batch.recordResult(30, 0x3333, false, 0, 400) == McpBatchRecordStatus.ok, true);
    expect_eq(batch.recordResult(10, 0x1111, false, 0, 100) == McpBatchRecordStatus.ok, true);
    expect_eq(batch.recordResult(20, 0x2222, true, -32602, 700) == McpBatchRecordStatus.ok, true);

    expect_eq(batch.isComplete(), true);

    // 1) Collect in original submission order -> [10, 20, 30]
    let sub_out: [8]McpBatchResponseItem = [McpBatchResponseItem{}; 8];
    let sub_s: []McpBatchResponseItem = sub_out[0..];
    const n_sub = batch.collectResponses(McpBatchOrderMode.submission_order, *sub_s);
    expect_eq(n_sub, 3);
    expect_eq(sub_out[0].request_id, 10);
    expect_eq(sub_out[0].payload_hash, 0x1111);
    expect_eq(sub_out[0].is_error, false);
    expect_eq(sub_out[1].request_id, 20);
    expect_eq(sub_out[1].is_error, true);
    expect_eq(sub_out[1].error_code, -32602);
    expect_eq(sub_out[2].request_id, 30);
    expect_eq(sub_out[2].payload_hash, 0x3333);

    // 2) Collect in completion order -> [30, 10, 20]
    let comp_out: [8]McpBatchResponseItem = [McpBatchResponseItem{}; 8];
    let comp_s: []McpBatchResponseItem = comp_out[0..];
    const n_comp = batch.collectResponses(McpBatchOrderMode.completion_order, *comp_s);
    expect_eq(n_comp, 3);
    expect_eq(comp_out[0].request_id, 30);
    expect_eq(comp_out[1].request_id, 10);
    expect_eq(comp_out[2].request_id, 20);

    // 3) Verify aggregated summary
    const sum: McpBatchSummary = batch.summary();
    expect_eq(sum.batch_id, 9001);
    expect_eq(sum.total_items, 5);
    expect_eq(sum.rpc_count, 3);
    expect_eq(sum.notification_count, 2);
    expect_eq(sum.pending_count, 0);
    expect_eq(sum.succeeded_count, 2);
    expect_eq(sum.failed_count, 1);
    expect_eq(sum.timed_out_count, 0);
    expect_eq(sum.is_finalized, true);
    expect_eq(sum.total_latency_us, 1200);
    expect_eq(sum.min_latency_us, 100);
    expect_eq(sum.max_latency_us, 700);
    expect_eq(absF64(sum.avg_latency_us - 400.0) < 1.0e-6, true);
    expect_eq(absF64(sum.success_rate - (2.0 / 3.0)) < 1.0e-6, true);
}

// ============================================================================
// Test 2: All-Notification Batch (JSON-RPC 2.0 §6 Wire Suppression)
// ============================================================================

fn testAllNotificationBatchSuppressesWireResponse()! {
    let batch = McpBatchDispatcher.init(102);
    expect_eq(batch.addNotification(0x10, 500) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addNotification(0x20, 500) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addNotification(0x30, 500) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.ok, true);

    expect_eq(batch.requiresWireResponse(), false);
    expect_eq(batch.isComplete(), true);

    let out: [4]McpBatchResponseItem = [McpBatchResponseItem{}; 4];
    let out_s: []McpBatchResponseItem = out[0..];
    expect_eq(batch.collectResponses(McpBatchOrderMode.submission_order, *out_s), 0);

    const sum = batch.summary();
    expect_eq(sum.requires_wire_response, false);
    expect_eq(sum.is_finalized, true);
    expect_eq(sum.notification_count, 3);
    expect_eq(sum.rpc_count, 0);
    expect_eq(absF64(sum.success_rate - 1.0) < 1.0e-6, true);
}

// ============================================================================
// Test 3: Partial Timeout Sweep (`expirePending`) & Late Result Rejection
// ============================================================================

fn testPartialTimeoutSweepInjectsMinus32001()! {
    let batch = McpBatchDispatcher.init(303);
    * = batch.addRequest(101, 0x1, 1000);
    * = batch.addRequest(102, 0x2, 1000);
    * = batch.addRequest(103, 0x3, 1000);
    * = batch.sealBatch();

    // Only 102 completes before deadline
    expect_eq(batch.recordResult(102, 0xBEEF, false, 0, 250) == McpBatchRecordStatus.ok, true);

    // Before deadline (now=1999 < deadline=2000): 0 expired
    expect_eq(batch.expirePending(1999, 2000), 0);
    expect_eq(batch.isComplete(), false);

    // At/past deadline (now=2500 >= deadline=2000): 101 & 103 timed out with -32001
    expect_eq(batch.expirePending(2500, 2000), 2);
    expect_eq(batch.isComplete(), true);

    // Late arrival for 101 must be rejected as batch_finalized
    expect_eq(batch.recordResult(101, 0xCAFE, false, 0, 1600) == McpBatchRecordStatus.batch_finalized, true);

    let out: [4]McpBatchResponseItem = [McpBatchResponseItem{}; 4];
    let out_s: []McpBatchResponseItem = out[0..];
    expect_eq(batch.collectResponses(McpBatchOrderMode.submission_order, *out_s), 3);

    expect_eq(out[0].request_id, 101);
    expect_eq(out[0].state == McpBatchItemState.timed_out, true);
    expect_eq(out[0].is_error, true);
    expect_eq(out[0].error_code, MCP_BATCH_ERR_TIMEOUT);
    expect_eq(out[0].latency_us, 1_500_000);

    expect_eq(out[1].request_id, 102);
    expect_eq(out[1].state == McpBatchItemState.succeeded, true);
    expect_eq(out[1].error_code, 0);

    expect_eq(out[2].request_id, 103);
    expect_eq(out[2].state == McpBatchItemState.timed_out, true);
    expect_eq(out[2].error_code, MCP_BATCH_ERR_TIMEOUT);

    const sum = batch.summary();
    expect_eq(sum.succeeded_count, 1);
    expect_eq(sum.timed_out_count, 2);
    expect_eq(sum.late_result_rejects, 1);
}

// ============================================================================
// Test 4: Empty Batch JSON-RPC 2.0 Guard (`invalid_empty_batch`)
// ============================================================================

fn testEmptyBatchRejectedPerJsonRpcSpec()! {
    let batch = McpBatchDispatcher.init(404);
    expect_eq(batch.requiresWireResponse(), false);
    expect_eq(batch.isComplete(), false);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.invalid_empty_batch, true);
    expect_eq(MCP_BATCH_ERR_INVALID_REQUEST, -32600);

    // Batch remains unsealed after rejected empty seal, so valid request can still be added
    expect_eq(batch.addRequest(1, 0xAA, 10) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.ok, true);
}

// ============================================================================
// Test 5: Duplicate Request ID, Unknown ID & Double-Record Rejection
// ============================================================================

fn testDuplicateUnknownAndDoubleRecordGuards()! {
    let batch = McpBatchDispatcher.init(505);
    expect_eq(batch.addRequest(42, 0x1, 100) == McpBatchSubmitStatus.ok, true);
    expect_eq(batch.addRequest(42, 0x2, 100) == McpBatchSubmitStatus.duplicate_request_id, true);
    expect_eq(batch.addRequest(43, 0x3, 100) == McpBatchSubmitStatus.ok, true);
    * = batch.sealBatch();

    // Unknown ID rejected
    expect_eq(batch.recordResult(999, 0, false, 0, 50) == McpBatchRecordStatus.unknown_request_id, true);

    // First record for 42 succeeds, second record while 43 is still pending returns already_completed
    expect_eq(batch.recordResult(42, 0xAA, false, 0, 50) == McpBatchRecordStatus.ok, true);
    expect_eq(batch.recordResult(42, 0xBB, false, 0, 60) == McpBatchRecordStatus.already_completed, true);

    const sum = batch.summary();
    expect_eq(sum.duplicate_rejects, 1);
    expect_eq(sum.unknown_result_rejects, 1);
    expect_eq(sum.late_result_rejects, 1);
    expect_eq(sum.pending_count, 1);
}

// ============================================================================
// Test 6: Full 64-Item Capacity Boundary (`MCP_BATCH_MAX_ITEMS = 64`)
// ============================================================================

fn testFull64ItemCapacityBoundary()! {
    let batch = McpBatchDispatcher.init(606);
    expect_eq(MCP_BATCH_MAX_ITEMS, 64);

    let i: usize = 0;
    while (i < 32) : (i += 1) {
        expect_eq(batch.addRequest((i as u64) + 1, 0x100 + (i as u64), 1000) == McpBatchSubmitStatus.ok, true);
        expect_eq(batch.addNotification(0x200 + (i as u64), 1000) == McpBatchSubmitStatus.ok, true);
    }

    // 65th item (RPC or Notification) must be rejected with capacity_full
    expect_eq(batch.addRequest(999, 0x300, 1000) == McpBatchSubmitStatus.capacity_full, true);
    expect_eq(batch.addNotification(0x301, 1000) == McpBatchSubmitStatus.capacity_full, true);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.ok, true);

    // Complete all 32 RPCs in reverse order (32 down to 1)
    let r: usize = 32;
    while (r > 0) : (r -= 1) {
        expect_eq(batch.recordResult(r as u64, (r as u64) * 10, false, 0, (r as u64) * 5) == McpBatchRecordStatus.ok, true);
    }
    expect_eq(batch.isComplete(), true);

    // Verify submission-order reassembly restores 1..32
    let out: [32]McpBatchResponseItem = [McpBatchResponseItem{}; 32];
    let out_s: []McpBatchResponseItem = out[0..];
    expect_eq(batch.collectResponses(McpBatchOrderMode.submission_order, *out_s), 32);
    let k: usize = 0;
    while (k < 32) : (k += 1) {
        expect_eq(out[k].request_id, (k as u64) + 1);
    }
}

// ============================================================================
// Test 7: Post-Seal Mutation Guards & Output Buffer Truncation Safety
// ============================================================================

fn testPostSealGuardsAndBufferTruncation()! {
    let batch = McpBatchDispatcher.init(707);
    * = batch.addRequest(1, 0x1, 10);
    * = batch.addRequest(2, 0x2, 10);
    * = batch.addRequest(3, 0x3, 10);
    * = batch.addRequest(4, 0x4, 10);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.ok, true);

    // Sealed batch rejects further additions and re-sealing
    expect_eq(batch.addRequest(5, 0x5, 10) == McpBatchSubmitStatus.batch_sealed, true);
    expect_eq(batch.addNotification(0x6, 10) == McpBatchSubmitStatus.batch_sealed, true);
    expect_eq(batch.sealBatch() == McpBatchSubmitStatus.batch_sealed, true);

    * = batch.recordResult(1, 10, false, 0, 10);
    * = batch.recordResult(2, 20, false, 0, 20);
    * = batch.recordResult(3, 30, false, 0, 30);
    * = batch.recordResult(4, 40, false, 0, 40);

    // Undersized buffer (len 2) safely truncates without OOB
    let small: [2]McpBatchResponseItem = [McpBatchResponseItem{}; 2];
    let small_s: []McpBatchResponseItem = small[0..];
    expect_eq(batch.collectResponses(McpBatchOrderMode.submission_order, *small_s), 2);
    expect_eq(small[0].request_id, 1);
    expect_eq(small[1].request_id, 2);

    // Zero-length buffer returns 0 safely
    let empty_buf: [0]McpBatchResponseItem = [];
    let empty_s: []McpBatchResponseItem = empty_buf[0..];
    expect_eq(batch.collectResponses(McpBatchOrderMode.submission_order, *empty_s), 0);
}

// ============================================================================
// Test 8: `getItemByRequestId` Lookup & Error Code Normalization
// ============================================================================

fn testItemLookupAndErrorCodeNormalization()! {
    let batch = McpBatchDispatcher.init(808);
    * = batch.addRequest(55, 0x555, 200);
    * = batch.addNotification(0x666, 200);
    * = batch.sealBatch();

    // Pass non-zero error_code with is_error=false -> normalized to 0
    expect_eq(batch.recordResult(55, 0x9999, false, -32603, 123) == McpBatchRecordStatus.ok, true);

    if (it := batch.getItemByRequestId(55)) {
        expect_eq(it.kind == McpBatchEntryKind.rpc_request, true);
        expect_eq(it.state == McpBatchItemState.succeeded, true);
        expect_eq(it.is_error, false);
        expect_eq(it.error_code, 0);
        expect_eq(it.payload_hash, 0x9999);
        expect_eq(it.latency_us, 123);
    } else {
        expect_eq(true, false);
    }

    // Notifications (request_id == 0) and unknown IDs return null
    expect_eq(batch.getItemByRequestId(0) == null, true);
    expect_eq(batch.getItemByRequestId(999) == null, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("mcp.batch_dispatcher: mixed RPC + notification out-of-order reassembly & latency summary", testMixedBatchOutOfOrderReassembly);
test("mcp.batch_dispatcher: all-notification batch suppresses wire response (JSON-RPC 2.0)", testAllNotificationBatchSuppressesWireResponse);
test("mcp.batch_dispatcher: partial timeout sweep injects -32001 & rejects late results", testPartialTimeoutSweepInjectsMinus32001);
test("mcp.batch_dispatcher: empty batch rejected with invalid_empty_batch", testEmptyBatchRejectedPerJsonRpcSpec);
test("mcp.batch_dispatcher: duplicate request_id, unknown ID & double-record guards", testDuplicateUnknownAndDoubleRecordGuards);
test("mcp.batch_dispatcher: full 64-item capacity boundary & reverse completion reassembly", testFull64ItemCapacityBoundary);
test("mcp.batch_dispatcher: post-seal mutation guards & output buffer truncation safety", testPostSealGuardsAndBufferTruncation);
test("mcp.batch_dispatcher: getItemByRequestId lookup & error_code normalization", testItemLookupAndErrorCodeNormalization);

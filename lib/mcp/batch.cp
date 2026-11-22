// Origin: Pure-CP Stdlib (JSON-RPC 2.0 / MCP Batch Request Correlator & Out-of-Order Response Reassembler) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation JSON-RPC 2.0 / MCP batch dispatcher:
//   - Fixed 64-item batch capacity supporting mixed RPC requests & Notifications
//   - Strict duplicate request_id rejection within a batch & empty-batch JSON-RPC 2.0 guard
//   - Out-of-order async result recording with submission-order or completion-order reassembly
//   - All-notification batch wire-suppression (`requiresWireResponse() == false`)
//   - Batch deadline timeout sweep (`expirePending`) injecting `-32001` error codes
//   - Full latency & success-rate aggregation (`McpBatchSummary`)

// ============================================================================
// Constants
// ============================================================================

/// Maximum number of sub-entries (RPC requests + notifications) per JSON-RPC 2.0 batch.
pub const MCP_BATCH_MAX_ITEMS: usize = 64;

/// JSON-RPC 2.0 server-defined error code injected for timed-out batch sub-requests.
pub const MCP_BATCH_ERR_TIMEOUT: i32 = -32001;

/// JSON-RPC 2.0 Invalid Request error code (e.g. empty batch `[]`).
pub const MCP_BATCH_ERR_INVALID_REQUEST: i32 = -32600;

// ============================================================================
// 1. Enums
// ============================================================================

/// Kind of sub-entry inside a JSON-RPC 2.0 / MCP batch.
pub enum McpBatchEntryKind: u8 {
    /// Standard JSON-RPC request carrying a `request_id` and expecting a response.
    rpc_request = 0,
    /// JSON-RPC notification carrying no `request_id` and producing no wire response.
    notification = 1,
}

/// Lifecycle state of an individual batch sub-entry.
pub enum McpBatchItemState: u8 {
    /// RPC request awaiting async execution result.
    pending = 0,
    /// RPC request completed successfully.
    succeeded = 1,
    /// RPC request completed with a JSON-RPC error.
    failed = 2,
    /// RPC request expired past batch deadline (`-32001`).
    timed_out = 3,
    /// Notification accepted (no response expected).
    notification_done = 4,
}

/// Outcome of submitting a sub-entry or sealing a batch.
pub enum McpBatchSubmitStatus: u8 {
    ok = 0,
    /// JSON-RPC 2.0 §6 forbids empty batches (`[]`).
    invalid_empty_batch = 1,
    /// A request with the same `request_id` already exists in this batch.
    duplicate_request_id = 2,
    /// Batch already contains `MCP_BATCH_MAX_ITEMS` (64) entries.
    capacity_full = 3,
    /// Batch has already been sealed or finalized.
    batch_sealed = 4,
}

/// Outcome of recording an async execution result for a batch sub-request.
pub enum McpBatchRecordStatus: u8 {
    ok = 0,
    /// No RPC request with this `request_id` exists in the batch.
    unknown_request_id = 1,
    /// The target request already succeeded, failed, or timed out.
    already_completed = 2,
    /// The entire batch is already finalized and closed to further writes.
    batch_finalized = 3,
}

/// Output ordering mode for `McpBatchDispatcher.collectResponses(...)`.
pub enum McpBatchOrderMode: u8 {
    /// Reorder responses strictly by original batch submission index.
    submission_order = 0,
    /// Order responses by actual async completion sequence.
    completion_order = 1,
}

// ============================================================================
// 2. Data Models
// ============================================================================

/// Stored state of a single sub-entry inside `McpBatchDispatcher`.
pub struct McpBatchItem {
    slot_index: u16 = 0;
    kind: McpBatchEntryKind = McpBatchEntryKind.rpc_request;
    state: McpBatchItemState = McpBatchItemState.pending;
    request_id: u64 = 0;
    method_hash: u64 = 0;
    submitted_at_ms: u64 = 0;
    completion_seq: u32 = 0;
    payload_hash: u64 = 0;
    is_error: bool = false;
    error_code: i32 = 0;
    latency_us: u64 = 0;
}

/// Reassembled wire-response descriptor emitted by `collectResponses(...)`.
pub struct McpBatchResponseItem {
    request_id: u64 = 0;
    slot_index: u16 = 0;
    completion_seq: u32 = 0;
    state: McpBatchItemState = McpBatchItemState.pending;
    payload_hash: u64 = 0;
    is_error: bool = false;
    error_code: i32 = 0;
    latency_us: u64 = 0;
}

/// Aggregated latency, success-rate, and protocol summary of a batch.
pub struct McpBatchSummary {
    batch_id: u64 = 0;
    total_items: usize = 0;
    rpc_count: usize = 0;
    notification_count: usize = 0;
    pending_count: usize = 0;
    succeeded_count: usize = 0;
    failed_count: usize = 0;
    timed_out_count: usize = 0;
    requires_wire_response: bool = false;
    is_complete: bool = false;
    is_sealed: bool = false;
    is_finalized: bool = false;
    total_latency_us: u64 = 0;
    min_latency_us: u64 = 0;
    max_latency_us: u64 = 0;
    avg_latency_us: f64 = 0.0;
    success_rate: f64 = 0.0;
    duplicate_rejects: u64 = 0;
    unknown_result_rejects: u64 = 0;
    late_result_rejects: u64 = 0;
}

// ============================================================================
// 3. McpBatchDispatcher (Zero-Alloc Batch Correlator & Response Reassembler)
// ============================================================================

/// Zero-heap-allocation JSON-RPC 2.0 / MCP batch request correlator and response reassembler.
pub struct McpBatchDispatcher {
    batch_id: u64 = 0;
    items: [MCP_BATCH_MAX_ITEMS]McpBatchItem = [McpBatchItem{}; MCP_BATCH_MAX_ITEMS];
    item_count: usize = 0;
    rpc_count: usize = 0;
    notification_count: usize = 0;
    pending_count: usize = 0;
    succeeded_count: usize = 0;
    failed_count: usize = 0;
    timed_out_count: usize = 0;
    next_completion_seq: u32 = 0;
    sealed: bool = false;
    finalized: bool = false;
    duplicate_rejects: u64 = 0;
    unknown_result_rejects: u64 = 0;
    late_result_rejects: u64 = 0;

    /// Creates a fresh empty batch dispatcher for `batch_id`.
    static fn init(batch_id: u64): McpBatchDispatcher {
        return McpBatchDispatcher{
            batch_id: batch_id,
        };
    }

    fn findRequestSlot(request_id: u64): usize? {
        let i: usize = 0;
        while (i < _.item_count) : (i += 1) {
            if (_.items[i].kind == McpBatchEntryKind.rpc_request &&
                _.items[i].request_id == request_id)
            {
                return i;
            }
        }
        return null;
    }

    /// Appends an RPC request carrying `request_id` to this batch.
    /// Rejects sealed/finalized batches, duplicate `request_id`, and batches exceeding 64 items.
    fn addRequest(
        *_,
        request_id: u64,
        method_hash: u64,
        submitted_at_ms: u64,
    ): McpBatchSubmitStatus {
        if (_.sealed || _.finalized) return McpBatchSubmitStatus.batch_sealed;
        if (_.item_count >= MCP_BATCH_MAX_ITEMS) return McpBatchSubmitStatus.capacity_full;
        if (_.findRequestSlot(request_id) != null) {
            _.duplicate_rejects += 1;
            return McpBatchSubmitStatus.duplicate_request_id;
        }

        const idx: usize = _.item_count;
        _.items[idx] = McpBatchItem{
            slot_index: idx as u16,
            kind: McpBatchEntryKind.rpc_request,
            state: McpBatchItemState.pending,
            request_id: request_id,
            method_hash: method_hash,
            submitted_at_ms: submitted_at_ms,
            completion_seq: 0,
            payload_hash: 0,
            is_error: false,
            error_code: 0,
            latency_us: 0,
        };
        _.item_count += 1;
        _.rpc_count += 1;
        _.pending_count += 1;
        return McpBatchSubmitStatus.ok;
    }

    /// Appends a fire-and-forget JSON-RPC Notification (produces no wire response).
    fn addNotification(
        *_,
        method_hash: u64,
        submitted_at_ms: u64,
    ): McpBatchSubmitStatus {
        if (_.sealed || _.finalized) return McpBatchSubmitStatus.batch_sealed;
        if (_.item_count >= MCP_BATCH_MAX_ITEMS) return McpBatchSubmitStatus.capacity_full;

        const idx: usize = _.item_count;
        _.items[idx] = McpBatchItem{
            slot_index: idx as u16,
            kind: McpBatchEntryKind.notification,
            state: McpBatchItemState.notification_done,
            request_id: 0,
            method_hash: method_hash,
            submitted_at_ms: submitted_at_ms,
            completion_seq: 0,
            payload_hash: 0,
            is_error: false,
            error_code: 0,
            latency_us: 0,
        };
        _.item_count += 1;
        _.notification_count += 1;
        return McpBatchSubmitStatus.ok;
    }

    /// Seals the batch after all incoming JSON-RPC batch elements have been parsed.
    ///
    /// - Returns `invalid_empty_batch` if no entries were added (JSON-RPC 2.0 §6).
    /// - Automatically marks the batch `finalized` if `pending_count == 0` (e.g. all-notification batch).
    fn sealBatch(*_): McpBatchSubmitStatus {
        if (_.item_count == 0) return McpBatchSubmitStatus.invalid_empty_batch;
        if (_.sealed) return McpBatchSubmitStatus.batch_sealed;
        _.sealed = true;
        if (_.pending_count == 0) {
            _.finalized = true;
        }
        return McpBatchSubmitStatus.ok;
    }

    /// Records an asynchronous execution result (possibly out-of-order) for `request_id`.
    /// Rejects unknown IDs, already-completed requests, and already-finalized batches.
    fn recordResult(
        *_,
        request_id: u64,
        payload_hash: u64,
        is_error: bool,
        error_code: i32,
        latency_us: u64,
    ): McpBatchRecordStatus {
        if (_.finalized) {
            _.late_result_rejects += 1;
            return McpBatchRecordStatus.batch_finalized;
        }

        const slot_opt = _.findRequestSlot(request_id);
        if (slot_opt == null) {
            _.unknown_result_rejects += 1;
            return McpBatchRecordStatus.unknown_request_id;
        }
        const idx: usize = slot_opt ?? 0;

        if (_.items[idx].state != McpBatchItemState.pending) {
            _.late_result_rejects += 1;
            return McpBatchRecordStatus.already_completed;
        }

        _.next_completion_seq += 1;
        _.items[idx].completion_seq = _.next_completion_seq;
        _.items[idx].payload_hash = payload_hash;
        _.items[idx].is_error = is_error;
        _.items[idx].error_code = if (is_error) error_code else 0;
        _.items[idx].latency_us = latency_us;

        if (is_error) {
            _.items[idx].state = McpBatchItemState.failed;
            _.failed_count += 1;
        } else {
            _.items[idx].state = McpBatchItemState.succeeded;
            _.succeeded_count += 1;
        }

        if (_.pending_count > 0) {
            _.pending_count -= 1;
        }
        if (_.sealed && _.pending_count == 0) {
            _.finalized = true;
        }
        return McpBatchRecordStatus.ok;
    }

    /// Sweeps all still-pending RPC requests once `now_ms >= deadline_ms`, injecting
    /// `MCP_BATCH_ERR_TIMEOUT` (`-32001`) errors and finalizing the batch if sealed.
    /// Returns the number of requests timed out by this sweep.
    fn expirePending(*_, now_ms: u64, deadline_ms: u64): usize {
        if (now_ms < deadline_ms || _.pending_count == 0) return 0;

        let expired: usize = 0;
        let i: usize = 0;
        while (i < _.item_count) : (i += 1) {
            if (_.items[i].kind == McpBatchEntryKind.rpc_request &&
                _.items[i].state == McpBatchItemState.pending)
            {
                const elapsed_ms: u64 = if (now_ms > _.items[i].submitted_at_ms)
                    (now_ms - _.items[i].submitted_at_ms)
                else
                    0;
                _.next_completion_seq += 1;
                _.items[i].completion_seq = _.next_completion_seq;
                _.items[i].state = McpBatchItemState.timed_out;
                _.items[i].is_error = true;
                _.items[i].error_code = MCP_BATCH_ERR_TIMEOUT;
                _.items[i].payload_hash = 0;
                _.items[i].latency_us = elapsed_ms * 1000;
                _.timed_out_count += 1;
                if (_.pending_count > 0) {
                    _.pending_count -= 1;
                }
                expired += 1;
            }
        }

        if (_.sealed && _.pending_count == 0) {
            _.finalized = true;
        }
        return expired;
    }

    /// Returns `true` iff this batch must emit a JSON-RPC response array on the wire.
    /// Per JSON-RPC 2.0 §6, an all-notification batch returns `false` (server MUST NOT return `[]`).
    fn requiresWireResponse(): bool {
        return _.item_count > 0 && _.rpc_count > 0;
    }

    /// Returns `true` when the batch is non-empty and all RPC requests have reached a terminal state.
    fn isComplete(): bool {
        return _.item_count > 0 && _.pending_count == 0;
    }

    /// Looks up a batch item by `request_id`. Returns `null` if not found.
    fn getItemByRequestId(request_id: u64): McpBatchItem? {
        if (idx := _.findRequestSlot(request_id)) {
            return _.items[idx];
        }
        return null;
    }

    /// Collects completed RPC responses into `out_buf` ordered by either original submission order
    /// (`submission_order`) or async completion order (`completion_order`).
    /// Notifications and still-pending items are excluded. Returns the number of responses written.
    fn collectResponses(
        order: McpBatchOrderMode,
        out_buf: *[]McpBatchResponseItem,
    ): usize {
        if (out_buf.len == 0 || _.rpc_count == 0) return 0;

        let written: usize = 0;
        let i: usize = 0;
        while (i < _.item_count && written < out_buf.len) : (i += 1) {
            const it = _.items[i];
            if (it.kind == McpBatchEntryKind.rpc_request && it.state != McpBatchItemState.pending) {
                out_buf[written] = McpBatchResponseItem{
                    request_id: it.request_id,
                    slot_index: it.slot_index,
                    completion_seq: it.completion_seq,
                    state: it.state,
                    payload_hash: it.payload_hash,
                    is_error: it.is_error,
                    error_code: it.error_code,
                    latency_us: it.latency_us,
                };
                written += 1;
            }
        }

        if (order == McpBatchOrderMode.completion_order && written > 1) {
            // In-place insertion sort by `completion_seq` ascending (<= 64 elements).
            let k: usize = 1;
            while (k < written) : (k += 1) {
                const key = out_buf[k];
                let j: usize = k;
                while (j > 0 && out_buf[j - 1].completion_seq > key.completion_seq) : (j -= 1) {
                    out_buf[j] = out_buf[j - 1];
                }
                out_buf[j] = key;
            }
        }

        return written;
    }

    /// Computes aggregated latency, success-rate, and protocol telemetry summary for this batch.
    fn summary(): McpBatchSummary {
        let total_lat: u64 = 0;
        let min_lat: u64 = 0;
        let max_lat: u64 = 0;
        let completed_rpc: usize = 0;

        let i: usize = 0;
        while (i < _.item_count) : (i += 1) {
            const it = _.items[i];
            if (it.kind == McpBatchEntryKind.rpc_request && it.state != McpBatchItemState.pending) {
                total_lat += it.latency_us;
                if (completed_rpc == 0 || it.latency_us < min_lat) {
                    min_lat = it.latency_us;
                }
                if (it.latency_us > max_lat) {
                    max_lat = it.latency_us;
                }
                completed_rpc += 1;
            }
        }

        const avg_lat: f64 = if (completed_rpc > 0)
            (total_lat as f64) / (completed_rpc as f64)
        else
            0.0;
        const succ_rate: f64 = if (_.rpc_count > 0)
            (_.succeeded_count as f64) / (_.rpc_count as f64)
        else
            1.0;

        return McpBatchSummary{
            batch_id: _.batch_id,
            total_items: _.item_count,
            rpc_count: _.rpc_count,
            notification_count: _.notification_count,
            pending_count: _.pending_count,
            succeeded_count: _.succeeded_count,
            failed_count: _.failed_count,
            timed_out_count: _.timed_out_count,
            requires_wire_response: _.requiresWireResponse(),
            is_complete: _.isComplete(),
            is_sealed: _.sealed,
            is_finalized: _.finalized,
            total_latency_us: total_lat,
            min_latency_us: min_lat,
            max_latency_us: max_lat,
            avg_latency_us: avg_lat,
            success_rate: succ_rate,
            duplicate_rejects: _.duplicate_rejects,
            unknown_result_rejects: _.unknown_result_rejects,
            late_result_rejects: _.late_result_rejects,
        };
    }
}

// Origin: Pure-CP Stdlib (Zero-Alloc Bi-Temporal Agent Fact Ledger: Valid-Time + Transaction-Time) | Created: 2026-11-22
// Pure CP zero-heap-allocation Bi-Temporal Agent Fact Ledger (Zep / Graphiti memory architecture):
//   - Tracks every fact `(subject_id, predicate_id, object_hash)` along two orthogonal time axes:
//       * Valid-Time       `[valid_from_ms, valid_to_ms)` : when the fact is true in the world
//       * Transaction-Time `[tx_from_ms, tx_to_ms)`       : when the agent believed/recorded the fact
//   - Automatic conflict superseding (`assertFact` / `assertFactWindow`): closes conflicting active
//     facts' `tx_to_ms` (and `valid_to_ms` when advancing chronologically) with `superseded_by` links
//   - Current truth queries (`queryCurrentFacts`), arbitrary point-in-time bi-temporal time-travel
//     (`queryAsOf`), explicit retraction (`retractFact`), and predicate evolution audit (`tracePredicateHistory`)

// ============================================================================
// Constants & Models
// ============================================================================

pub const FACT_LEDGER_MAX_RECORDS: usize = 256;
pub const FACT_TIME_INF: i64 = 0x7FFFFFFFFFFFFFFF;
pub const FACT_NO_ID: u64 = 0;
pub const FACT_MAX_CONFIDENCE_BPS: u16 = 10000;

/// Lifecycle state of a bi-temporal fact record.
pub enum FactStatus {
    active,
    superseded,
    retracted,
}

/// Operation result status for fact assertions and retractions.
pub enum FactLedgerOpStatus {
    ok,
    invalid_time_range,
    invalid_confidence,
    capacity_full,
    fact_not_found,
    already_closed,
    invalid_tx_time,
}

/// Single bi-temporal fact entry stored inline in `BiTemporalFactLedger`.
pub struct BiTemporalFact {
    fact_id: u64 = FACT_NO_ID;
    subject_id: u64 = 0;
    predicate_id: u64 = 0;
    object_hash: u64 = 0;
    confidence_bps: u16 = FACT_MAX_CONFIDENCE_BPS;
    valid_from_ms: i64 = 0;
    valid_to_ms: i64 = FACT_TIME_INF;
    tx_from_ms: i64 = 0;
    tx_to_ms: i64 = FACT_TIME_INF;
    source_turn_id: u64 = 0;
    superseded_by: u64 = FACT_NO_ID;
    status: FactStatus = FactStatus.active;

    /// Returns `true` if `valid_at_ms ∈ [valid_from_ms, valid_to_ms)`.
    fn isValidAt(valid_at_ms: i64): bool {
        return valid_at_ms >= _.valid_from_ms && valid_at_ms < _.valid_to_ms;
    }

    /// Returns `true` if `tx_at_ms ∈ [tx_from_ms, tx_to_ms)`.
    fn isRecordedAt(tx_at_ms: i64): bool {
        return tx_at_ms >= _.tx_from_ms && tx_at_ms < _.tx_to_ms;
    }

    /// Returns `true` if the fact was both valid at `valid_at_ms` and believed at `tx_at_ms`.
    fn isActiveAt(valid_at_ms: i64, tx_at_ms: i64): bool {
        return _.isValidAt(valid_at_ms) && _.isRecordedAt(tx_at_ms);
    }
}

/// Result returned by `assertFact` / `assertFactWindow`.
pub struct FactAssertResult {
    status: FactLedgerOpStatus = FactLedgerOpStatus.ok;
    fact_id: u64 = FACT_NO_ID;
    superseded_count: usize = 0;
    first_superseded_id: u64 = FACT_NO_ID;
    reinforced_existing: bool = false;
}

/// Aggregate occupancy and lifecycle counts for `BiTemporalFactLedger`.
pub struct FactLedgerStats {
    total_records: usize = 0;
    active_records: usize = 0;
    superseded_records: usize = 0;
    retracted_records: usize = 0;
    capacity: usize = FACT_LEDGER_MAX_RECORDS;
}

// ============================================================================
// Bi-Temporal Agent Fact Ledger Engine
// ============================================================================

/// Zero-heap-allocation Bi-Temporal Agent Fact Ledger backed by `FACT_LEDGER_MAX_RECORDS` (256)
/// inline records.
pub struct BiTemporalFactLedger {
    records: [FACT_LEDGER_MAX_RECORDS]BiTemporalFact = [BiTemporalFact {}; FACT_LEDGER_MAX_RECORDS];
    record_count: usize = 0;
    next_fact_id: u64 = 1;

    static fn init(): BiTemporalFactLedger {
        return BiTemporalFactLedger {};
    }

    fn count(): usize {
        return _.record_count;
    }

    /// Asserts an open-ended fact `(subject_id, predicate_id, object_hash)` valid from
    /// `valid_from_ms` onward, recorded at `tx_now_ms` with `confidence_bps ∈ [0, 10000]`.
    fn assertFact(
        *_,
        subject_id: u64,
        predicate_id: u64,
        object_hash: u64,
        valid_from_ms: i64,
        tx_now_ms: i64,
        confidence_bps: u16,
    ): FactAssertResult {
        return _.assertFactWindow(
            subject_id,
            predicate_id,
            object_hash,
            valid_from_ms,
            FACT_TIME_INF,
            tx_now_ms,
            confidence_bps,
            0,
        );
    }

    /// Asserts a bi-temporal fact over explicit valid-time `[valid_from_ms, valid_to_ms)`
    /// recorded at `tx_now_ms` from `source_turn_id`.
    /// - Automatically closes conflicting active facts sharing `(subject_id, predicate_id)` with
    ///   a different `object_hash` whose valid intervals overlap, linking `superseded_by`.
    /// - Reinforces an identical active fact `(subject, predicate, object_hash)` with overlapping
    ///   valid interval in-place without consuming an extra slot.
    fn assertFactWindow(
        *_,
        subject_id: u64,
        predicate_id: u64,
        object_hash: u64,
        valid_from_ms: i64,
        valid_to_ms: i64,
        tx_now_ms: i64,
        confidence_bps: u16,
        source_turn_id: u64,
    ): FactAssertResult {
        if (valid_from_ms >= valid_to_ms) {
            return FactAssertResult { status: FactLedgerOpStatus.invalid_time_range };
        }
        if (confidence_bps > FACT_MAX_CONFIDENCE_BPS) {
            return FactAssertResult { status: FactLedgerOpStatus.invalid_confidence };
        }

        // 1. Check if an identical active fact with overlapping valid window already exists -> reinforce.
        let i: usize = 0;
        while (i < _.record_count) : (i += 1) {
            const r = _.records[i];
            if (r.status == FactStatus.active && r.tx_to_ms == FACT_TIME_INF
                && r.subject_id == subject_id && r.predicate_id == predicate_id
                && r.object_hash == object_hash
                && r.valid_to_ms > valid_from_ms && r.valid_from_ms < valid_to_ms)
            {
                if (confidence_bps > _.records[i].confidence_bps) {
                    _.records[i].confidence_bps = confidence_bps;
                }
                if (valid_to_ms > _.records[i].valid_to_ms) {
                    _.records[i].valid_to_ms = valid_to_ms;
                }
                if (source_turn_id != 0) {
                    _.records[i].source_turn_id = source_turn_id;
                }
                return FactAssertResult {
                    status: FactLedgerOpStatus.ok,
                    fact_id: r.fact_id,
                    reinforced_existing: true,
                };
            }
        }

        // 2. Ensure capacity before mutating any conflicting records (atomicity).
        if (_.record_count >= FACT_LEDGER_MAX_RECORDS) {
            return FactAssertResult { status: FactLedgerOpStatus.capacity_full };
        }

        const new_id: u64 = _.next_fact_id;

        // 3. Supersede conflicting active facts for the same (subject_id, predicate_id).
        let sup_count: usize = 0;
        let first_sup: u64 = FACT_NO_ID;
        i = 0;
        while (i < _.record_count) : (i += 1) {
            const r = _.records[i];
            if (r.status == FactStatus.active && r.tx_to_ms == FACT_TIME_INF
                && r.subject_id == subject_id && r.predicate_id == predicate_id
                && r.object_hash != object_hash
                && r.valid_to_ms > valid_from_ms && r.valid_from_ms < valid_to_ms)
            {
                // Close transaction-time interval (clamped >= tx_from_ms for monotonicity).
                _.records[i].tx_to_ms = if (tx_now_ms >= r.tx_from_ms) tx_now_ms else r.tx_from_ms;
                // Close valid-time upper bound only when advancing strictly past old valid_from_ms.
                if (valid_from_ms > r.valid_from_ms && valid_from_ms < r.valid_to_ms) {
                    _.records[i].valid_to_ms = valid_from_ms;
                }
                _.records[i].status = FactStatus.superseded;
                _.records[i].superseded_by = new_id;
                if (sup_count == 0) {
                    first_sup = r.fact_id;
                }
                sup_count += 1;
            }
        }

        // 4. Append new active fact record.
        _.records[_.record_count] = BiTemporalFact {
            fact_id: new_id,
            subject_id: subject_id,
            predicate_id: predicate_id,
            object_hash: object_hash,
            confidence_bps: confidence_bps,
            valid_from_ms: valid_from_ms,
            valid_to_ms: valid_to_ms,
            tx_from_ms: tx_now_ms,
            tx_to_ms: FACT_TIME_INF,
            source_turn_id: source_turn_id,
            superseded_by: FACT_NO_ID,
            status: FactStatus.active,
        };
        _.record_count += 1;
        _.next_fact_id += 1;

        return FactAssertResult {
            status: FactLedgerOpStatus.ok,
            fact_id: new_id,
            superseded_count: sup_count,
            first_superseded_id: first_sup,
            reinforced_existing: false,
        };
    }

    /// Explicitly retracts an active fact at `tx_now_ms` by closing its transaction-time interval.
    /// Returns `fact_not_found`, `already_closed`, or `invalid_tx_time` on boundary violations.
    fn retractFact(*_, fact_id: u64, tx_now_ms: i64): FactLedgerOpStatus {
        let i: usize = 0;
        while (i < _.record_count) : (i += 1) {
            if (_.records[i].fact_id == fact_id) {
                if (_.records[i].status != FactStatus.active) {
                    return FactLedgerOpStatus.already_closed;
                }
                if (tx_now_ms < _.records[i].tx_from_ms) {
                    return FactLedgerOpStatus.invalid_tx_time;
                }
                _.records[i].tx_to_ms = tx_now_ms;
                _.records[i].status = FactStatus.retracted;
                return FactLedgerOpStatus.ok;
            }
        }
        return FactLedgerOpStatus.fact_not_found;
    }

    /// Looks up a fact record by unique `fact_id`.
    fn getFact(fact_id: u64): BiTemporalFact? {
        let i: usize = 0;
        while (i < _.record_count) : (i += 1) {
            if (_.records[i].fact_id == fact_id) {
                return _.records[i];
            }
        }
        return null;
    }

    /// Queries all currently believed (`status == active`) facts for `subject_id` that are
    /// valid at `now_ms`. Writes up to `out_facts.len` results and returns count written.
    fn queryCurrentFacts(subject_id: u64, now_ms: i64, out_facts: *[]BiTemporalFact): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.record_count && written < out_facts.len) : (i += 1) {
            const r = _.records[i];
            if (r.subject_id == subject_id && r.status == FactStatus.active
                && r.tx_to_ms == FACT_TIME_INF && r.isValidAt(now_ms))
            {
                out_facts[written] = r;
                written += 1;
            }
        }
        return written;
    }

    /// Bi-temporal time-travel query: reconstructs what the agent believed at transaction time
    /// `tx_at_ms` was valid in the world at `valid_at_ms` for `subject_id`.
    fn queryAsOf(
        subject_id: u64,
        valid_at_ms: i64,
        tx_at_ms: i64,
        out_facts: *[]BiTemporalFact,
    ): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.record_count && written < out_facts.len) : (i += 1) {
            const r = _.records[i];
            if (r.subject_id == subject_id && r.isActiveAt(valid_at_ms, tx_at_ms)) {
                out_facts[written] = r;
                written += 1;
            }
        }
        return written;
    }

    /// Returns the complete chronological audit trail (all active, superseded, and retracted
    /// versions) for `(subject_id, predicate_id)`.
    fn tracePredicateHistory(
        subject_id: u64,
        predicate_id: u64,
        out_facts: *[]BiTemporalFact,
    ): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.record_count && written < out_facts.len) : (i += 1) {
            const r = _.records[i];
            if (r.subject_id == subject_id && r.predicate_id == predicate_id) {
                out_facts[written] = r;
                written += 1;
            }
        }
        return written;
    }

    /// Returns occupancy and lifecycle status counts.
    fn stats(): FactLedgerStats {
        let active_c: usize = 0;
        let sup_c: usize = 0;
        let ret_c: usize = 0;
        let i: usize = 0;
        while (i < _.record_count) : (i += 1) {
            const st = _.records[i].status;
            if (st == FactStatus.active) {
                active_c += 1;
            } else if (st == FactStatus.superseded) {
                sup_c += 1;
            } else {
                ret_c += 1;
            }
        }
        return FactLedgerStats {
            total_records: _.record_count,
            active_records: active_c,
            superseded_records: sup_c,
            retracted_records: ret_c,
            capacity: FACT_LEDGER_MAX_RECORDS,
        };
    }

    /// Clears all records and resets `next_fact_id` to 1.
    fn clear(*_) {
        let i: usize = 0;
        while (i < _.record_count) : (i += 1) {
            _.records[i] = BiTemporalFact {};
        }
        _.record_count = 0;
        _.next_fact_id = 1;
    }
}

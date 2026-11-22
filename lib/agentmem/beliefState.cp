// Origin: Pure-CP Stdlib (AI Agent Belief State Revision & Contradiction Resolver) | Created: 2026-11-23
// Pure CP zero-heap-allocation AGM / Bayesian agent belief state engine:
//   - Fixed-capacity belief table storing (subject_id, predicate_id, object_hash), confidence [0,1],
//     source_trust [0,1], observation_count, timestamps, status & superseded_by lineage
//   - Smooth log-odds Bayesian reinforcement for corroborating observations
//   - Contradiction detection & weighted trust + half-life decay arbitration (retain / supersede / contest)
//   - Explicit retraction with automatic contested resolution, time-decay sweep, zero-heap conflict audit log,
//     and epistemic entropy / contradiction ratio telemetry

const std = @import("std");

// ============================================================================
// 1. Constants & Numeric Helpers
// ============================================================================

/// Maximum belief records stored in a zero-alloc `BeliefStateStore`.
pub const BELIEF_MAX_RECORDS: usize = 128;

/// Maximum conflict audit records kept in the zero-alloc ring buffer.
pub const BELIEF_MAX_AUDIT_ENTRIES: usize = 64;

/// Sentinel ID representing no belief / no superseding successor.
pub const BELIEF_NO_ID: u32 = 0;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampF64(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

// ============================================================================
// 2. Public Bayesian / Decay Scoring Primitives
// ============================================================================

/// Smooth log-odds Bayesian confidence reinforcement for a corroborating observation.
/// Combines prior `prior_conf` with trust-tempered `evidence_conf` via log-odds addition
/// and maps back to `(0, 1)` through the logistic sigmoid (never overflows `> 1.0`).
pub fn beliefLogOddsReinforce(
    prior_conf: f32,
    evidence_conf: f32,
    source_trust: f32,
): f32 {
    const eps: f64 = 0.0001;
    const p: f64 = clampF64(prior_conf as f64, eps, 1.0 - eps);
    const ev: f64 = clampF64(evidence_conf as f64, 0.0, 1.0);
    const tr: f64 = clampF64(source_trust as f64, 0.0, 1.0);

    // Trust-tempered evidence probability: at tr=0 -> neutral 0.5 (delta log-odds = 0).
    const q: f64 = clampF64(0.5 + (ev - 0.5) * tr, eps, 1.0 - eps);

    const l0: f64 = std.math.log(f64, std.math.e, p / (1.0 - p));
    const dl: f64 = std.math.log(f64, std.math.e, q / (1.0 - q));
    const l1: f64 = clampF64(l0 + dl, -12.0, 12.0);
    const updated: f64 = 1.0 / (1.0 + std.math.exp(0.0 - l1));
    return clampF64(updated, 0.0, 1.0) as f32;
}

/// Applies exponential half-life decay `confidence * 2^(-elapsed_ms / half_life_ms)`,
/// floored at `min_confidence` (unless `confidence` is already below `min_confidence`).
pub fn beliefHalfLifeDecay(
    confidence: f32,
    elapsed_ms: u64,
    half_life_ms: u64,
    min_confidence: f32,
): f32 {
    if (!isFiniteF32(confidence) || confidence <= 0.0) return 0.0;
    if (half_life_ms == 0 || elapsed_ms == 0) return confidence;

    const exponent: f64 = (0.0 - (elapsed_ms as f64)) / (half_life_ms as f64);
    const factor: f64 = std.math.pow(f64, 2.0, exponent);
    const decayed: f32 = ((confidence as f64) * factor) as f32;
    const floor_val: f32 = if (confidence < min_confidence) confidence else min_confidence;
    return if (decayed < floor_val) floor_val else decayed;
}

/// Computes effective arbitration weight combining half-life decayed confidence and source trust:
/// `decayed_confidence * (0.25 + 0.75 * source_trust)`.
pub fn beliefEffectiveWeight(
    confidence: f32,
    source_trust: f32,
    elapsed_ms: u64,
    half_life_ms: u64,
    min_confidence: f32,
): f32 {
    const decayed: f32 = beliefHalfLifeDecay(confidence, elapsed_ms, half_life_ms, min_confidence);
    const tr: f32 = clampF64(source_trust as f64, 0.0, 1.0) as f32;
    return decayed * (0.25 + 0.75 * tr);
}

// ============================================================================
// 3. Public Data Models
// ============================================================================

/// Lifecycle state of an individual belief record.
pub enum BeliefStatus {
    active,
    contested,
    superseded,
    retracted,
}

/// Outcome of a single `BeliefStateStore.observe` revision call.
pub enum BeliefRevisionAction {
    inserted,
    reinforced,
    superseded_old,
    retained_old,
    marked_contested,
    rejected_invalid,
    rejected_full,
}

/// Policy parameters controlling decay and contradiction arbitration.
pub struct BeliefRevisionConfig {
    half_life_ms: u64 = 3600000;
    contest_margin: f32 = 0.12;
    min_confidence: f32 = 0.05;

    /// Returns production defaults (`half_life_ms=3600000`, `contest_margin=0.12`, `min_confidence=0.05`).
    static fn defaults(): BeliefRevisionConfig {
        return BeliefRevisionConfig{
            half_life_ms: 3600000,
            contest_margin: 0.12,
            min_confidence: 0.05,
        };
    }

    /// Validates numeric bounds and finiteness invariants.
    fn isValid(): bool {
        if (!isFiniteF32(_.contest_margin) || _.contest_margin < 0.0 || _.contest_margin > 1.0) return false;
        if (!isFiniteF32(_.min_confidence) || _.min_confidence < 0.0 || _.min_confidence > 1.0) return false;
        return true;
    }
}

/// Single belief triple record with confidence, trust, provenance & lineage.
pub struct BeliefRecord {
    belief_id: u32 = BELIEF_NO_ID;
    subject_id: u64 = 0;
    predicate_id: u32 = 0;
    object_hash: u64 = 0;
    confidence: f32 = 0.0;
    source_trust: f32 = 0.0;
    observation_count: u32 = 0;
    created_ms: u64 = 0;
    updated_ms: u64 = 0;
    status: BeliefStatus = BeliefStatus.active;
    superseded_by: u32 = BELIEF_NO_ID;
    occupied: bool = false;
}

/// Structured result returned by `BeliefStateStore.observe`.
pub struct BeliefRevisionResult {
    action: BeliefRevisionAction = BeliefRevisionAction.inserted;
    belief_id: u32 = BELIEF_NO_ID;
    conflict_with_id: u32 = BELIEF_NO_ID;
    new_confidence: f32 = 0.0;
    incumbent_effective_weight: f32 = 0.0;
    incoming_effective_weight: f32 = 0.0;
}

/// Zero-heap conflict audit record capturing contradiction arbitration decisions.
pub struct BeliefConflictAudit {
    subject_id: u64 = 0;
    predicate_id: u32 = 0;
    incumbent_id: u32 = BELIEF_NO_ID;
    incoming_id: u32 = BELIEF_NO_ID;
    incumbent_object_hash: u64 = 0;
    incoming_object_hash: u64 = 0;
    action: BeliefRevisionAction = BeliefRevisionAction.marked_contested;
    timestamp_ms: u64 = 0;
}

/// Epistemic uncertainty and lifecycle telemetry snapshot.
pub struct BeliefStateStats {
    total_records: usize = 0;
    active_count: usize = 0;
    contested_count: usize = 0;
    superseded_count: usize = 0;
    retracted_count: usize = 0;
    conflict_events: u64 = 0;
    reinforce_events: u64 = 0;
    epistemic_entropy: f32 = 0.0;
    contradiction_ratio: f32 = 0.0;
}

// ============================================================================
// 4. Zero-Alloc Belief State Store & Contradiction Resolver
// ============================================================================

/// Zero-heap-allocation agent belief revision & contradiction resolution engine.
pub struct BeliefStateStore {
    cfg: BeliefRevisionConfig = BeliefRevisionConfig{},
    records: [128]BeliefRecord = [BeliefRecord{}; 128],
    record_count: usize = 0,
    next_id: u32 = 1,
    audits: [64]BeliefConflictAudit = [BeliefConflictAudit{}; 64],
    audit_head: usize = 0,
    audit_count: usize = 0,
    conflict_events: u64 = 0,
    reinforce_events: u64 = 0;

    /// Initializes a belief state store from `cfg`. Returns `null` if `cfg` is invalid.
    static fn init(cfg: BeliefRevisionConfig): BeliefStateStore? {
        if (!cfg.isValid()) return null;
        return BeliefStateStore{
            cfg: cfg,
        };
    }

    /// Clears all beliefs, conflict audits, and counters while preserving `cfg`.
    fn clear(*_) {
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            _.records[i] = BeliefRecord{};
        }
        _.record_count = 0;
        _.next_id = 1;
        _.audit_head = 0;
        _.audit_count = 0;
        _.conflict_events = 0;
        _.reinforce_events = 0;
    }

    fn recordAudit(*_, entry: BeliefConflictAudit) {
        _.audits[_.audit_head] = entry;
        _.audit_head = (_.audit_head + 1) % BELIEF_MAX_AUDIT_ENTRIES;
        if (_.audit_count < BELIEF_MAX_AUDIT_ENTRIES) {
            _.audit_count += 1;
        }
    }

    fn allocateSlot(*_): usize? {
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            if (!_.records[i].occupied) return i;
        }
        return null;
    }

    fn isLiveStatus(st: BeliefStatus): bool {
        return st == BeliefStatus.active || st == BeliefStatus.contested;
    }

    fn recordWeightAt(rec: BeliefRecord, now_ms: u64): f32 {
        const elapsed: u64 = if (now_ms > rec.updated_ms) (now_ms - rec.updated_ms) else 0;
        return beliefEffectiveWeight(
            rec.confidence,
            rec.source_trust,
            elapsed,
            _.cfg.half_life_ms,
            _.cfg.min_confidence,
        );
    }

    /// Observes a new belief evidence tuple `(subject_id, predicate_id, object_hash)` with
    /// `confidence ∈ [0, 1]` and `source_trust ∈ [0, 1]` at timestamp `now_ms`.
    ///
    /// - Corroborating `object_hash`: applies half-life decay then log-odds reinforcement;
    ///   if reinforcement breaks a `contested` tie beyond `contest_margin`, automatically promotes
    ///   the winner to `active` and supersedes weaker rivals.
    /// - Contradicting `object_hash`: compares effective weights (decayed confidence × source trust)
    ///   to either `superseded_old`, `retained_old`, or `marked_contested`.
    fn observe(
        *_,
        subject_id: u64,
        predicate_id: u32,
        object_hash: u64,
        confidence: f32,
        source_trust: f32,
        now_ms: u64,
    ): BeliefRevisionResult {
        if (subject_id == 0 || predicate_id == 0) {
            return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_invalid };
        }
        if (!isFiniteF32(confidence) || confidence < 0.0 || confidence > 1.0) {
            return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_invalid };
        }
        if (!isFiniteF32(source_trust) || source_trust < 0.0 || source_trust > 1.0) {
            return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_invalid };
        }

        // Scan live (active or contested) beliefs for this (subject_id, predicate_id).
        let exact_idx: usize = BELIEF_MAX_RECORDS;
        let best_rival_idx: usize = BELIEF_MAX_RECORDS;
        let best_rival_weight: f32 = -1.0;

        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            const rec = _.records[i];
            if (!rec.occupied) continue;
            if (rec.subject_id != subject_id || rec.predicate_id != predicate_id) continue;
            if (!_.isLiveStatus(rec.status)) continue;

            if (rec.object_hash == object_hash) {
                exact_idx = i;
            } else {
                const w: f32 = _.recordWeightAt(rec, now_ms);
                if (best_rival_idx == BELIEF_MAX_RECORDS || w > best_rival_weight) {
                    best_rival_idx = i;
                    best_rival_weight = w;
                }
            }
        }

        // Case A: Corroborating observation on an existing live belief -> log-odds reinforcement.
        if (exact_idx < BELIEF_MAX_RECORDS) {
            let cur = _.records[exact_idx];
            const elapsed: u64 = if (now_ms > cur.updated_ms) (now_ms - cur.updated_ms) else 0;
            const decayed_prior: f32 = beliefHalfLifeDecay(
                cur.confidence,
                elapsed,
                _.cfg.half_life_ms,
                _.cfg.min_confidence,
            );
            const new_conf: f32 = beliefLogOddsReinforce(decayed_prior, confidence, source_trust);
            const prev_n: f32 = cur.observation_count as f32;
            const new_trust: f32 = (cur.source_trust * prev_n + source_trust) / (prev_n + 1.0);

            cur.confidence = new_conf;
            cur.source_trust = new_trust;
            cur.observation_count += 1;
            if (now_ms >= cur.updated_ms) {
                cur.updated_ms = now_ms;
            }

            // If this belief was contested and now decisively beats the strongest rival, resolve tie.
            const self_w: f32 = beliefEffectiveWeight(new_conf, new_trust, 0, _.cfg.half_life_ms, _.cfg.min_confidence);
            if (cur.status == BeliefStatus.contested && best_rival_idx < BELIEF_MAX_RECORDS) {
                if (self_w > best_rival_weight + _.cfg.contest_margin) {
                    cur.status = BeliefStatus.active;
                    let r: usize = 0;
                    while (r < BELIEF_MAX_RECORDS) : (r += 1) {
                        if (r == exact_idx || !_.records[r].occupied) continue;
                        if (_.records[r].subject_id == subject_id &&
                            _.records[r].predicate_id == predicate_id &&
                            _.isLiveStatus(_.records[r].status))
                        {
                            _.records[r].status = BeliefStatus.superseded;
                            _.records[r].superseded_by = cur.belief_id;
                        }
                    }
                }
            }

            _.records[exact_idx] = cur;
            _.reinforce_events += 1;
            return BeliefRevisionResult{
                action: BeliefRevisionAction.reinforced,
                belief_id: cur.belief_id,
                conflict_with_id: if (best_rival_idx < BELIEF_MAX_RECORDS) _.records[best_rival_idx].belief_id else BELIEF_NO_ID,
                new_confidence: new_conf,
                incumbent_effective_weight: if (best_rival_idx < BELIEF_MAX_RECORDS) best_rival_weight else self_w,
                incoming_effective_weight: self_w,
            };
        }

        const w_in: f32 = beliefEffectiveWeight(confidence, source_trust, 0, _.cfg.half_life_ms, _.cfg.min_confidence);

        // Case B: Contradiction with existing live rival(s) for same (subject_id, predicate_id).
        if (best_rival_idx < BELIEF_MAX_RECORDS) {
            const inc = _.records[best_rival_idx];
            const w_inc: f32 = best_rival_weight;
            _.conflict_events += 1;

            // B1: Incoming evidence decisively wins -> supersede existing live beliefs.
            if (w_in > w_inc + _.cfg.contest_margin) {
                const slot = _.allocateSlot() orelse {
                    return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_full };
                };
                const new_id: u32 = _.next_id;
                _.next_id += 1;

                _.records[slot] = BeliefRecord{
                    belief_id: new_id,
                    subject_id: subject_id,
                    predicate_id: predicate_id,
                    object_hash: object_hash,
                    confidence: confidence,
                    source_trust: source_trust,
                    observation_count: 1,
                    created_ms: now_ms,
                    updated_ms: now_ms,
                    status: BeliefStatus.active,
                    superseded_by: BELIEF_NO_ID,
                    occupied: true,
                };
                _.record_count += 1;

                let r: usize = 0;
                while (r < BELIEF_MAX_RECORDS) : (r += 1) {
                    if (r == slot || !_.records[r].occupied) continue;
                    if (_.records[r].subject_id == subject_id &&
                        _.records[r].predicate_id == predicate_id &&
                        _.isLiveStatus(_.records[r].status))
                    {
                        _.records[r].status = BeliefStatus.superseded;
                        _.records[r].superseded_by = new_id;
                    }
                }

                _.recordAudit(BeliefConflictAudit{
                    subject_id: subject_id,
                    predicate_id: predicate_id,
                    incumbent_id: inc.belief_id,
                    incoming_id: new_id,
                    incumbent_object_hash: inc.object_hash,
                    incoming_object_hash: object_hash,
                    action: BeliefRevisionAction.superseded_old,
                    timestamp_ms: now_ms,
                });

                return BeliefRevisionResult{
                    action: BeliefRevisionAction.superseded_old,
                    belief_id: new_id,
                    conflict_with_id: inc.belief_id,
                    new_confidence: confidence,
                    incumbent_effective_weight: w_inc,
                    incoming_effective_weight: w_in,
                };
            }

            // B2: Incumbent belief decisively stronger -> retain incumbent, archive weak incoming.
            if (w_inc > w_in + _.cfg.contest_margin) {
                let recorded_in_id: u32 = BELIEF_NO_ID;
                if (slot := _.allocateSlot()) {
                    recorded_in_id = _.next_id;
                    _.next_id += 1;
                    _.records[slot] = BeliefRecord{
                        belief_id: recorded_in_id,
                        subject_id: subject_id,
                        predicate_id: predicate_id,
                        object_hash: object_hash,
                        confidence: confidence,
                        source_trust: source_trust,
                        observation_count: 1,
                        created_ms: now_ms,
                        updated_ms: now_ms,
                        status: BeliefStatus.superseded,
                        superseded_by: inc.belief_id,
                        occupied: true,
                    };
                    _.record_count += 1;
                }

                _.recordAudit(BeliefConflictAudit{
                    subject_id: subject_id,
                    predicate_id: predicate_id,
                    incumbent_id: inc.belief_id,
                    incoming_id: recorded_in_id,
                    incumbent_object_hash: inc.object_hash,
                    incoming_object_hash: object_hash,
                    action: BeliefRevisionAction.retained_old,
                    timestamp_ms: now_ms,
                });

                return BeliefRevisionResult{
                    action: BeliefRevisionAction.retained_old,
                    belief_id: inc.belief_id,
                    conflict_with_id: recorded_in_id,
                    new_confidence: inc.confidence,
                    incumbent_effective_weight: w_inc,
                    incoming_effective_weight: w_in,
                };
            }

            // B3: Close weights (|w_in - w_inc| <= contest_margin) -> mark both contested.
            const slot = _.allocateSlot() orelse {
                return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_full };
            };
            const new_id: u32 = _.next_id;
            _.next_id += 1;

            _.records[slot] = BeliefRecord{
                belief_id: new_id,
                subject_id: subject_id,
                predicate_id: predicate_id,
                object_hash: object_hash,
                confidence: confidence,
                source_trust: source_trust,
                observation_count: 1,
                created_ms: now_ms,
                updated_ms: now_ms,
                status: BeliefStatus.contested,
                superseded_by: BELIEF_NO_ID,
                occupied: true,
            };
            _.record_count += 1;

            let r: usize = 0;
            while (r < BELIEF_MAX_RECORDS) : (r += 1) {
                if (r == slot || !_.records[r].occupied) continue;
                if (_.records[r].subject_id == subject_id &&
                    _.records[r].predicate_id == predicate_id &&
                    _.records[r].status == BeliefStatus.active)
                {
                    _.records[r].status = BeliefStatus.contested;
                }
            }

            _.recordAudit(BeliefConflictAudit{
                subject_id: subject_id,
                predicate_id: predicate_id,
                incumbent_id: inc.belief_id,
                incoming_id: new_id,
                incumbent_object_hash: inc.object_hash,
                incoming_object_hash: object_hash,
                action: BeliefRevisionAction.marked_contested,
                timestamp_ms: now_ms,
            });

            return BeliefRevisionResult{
                action: BeliefRevisionAction.marked_contested,
                belief_id: new_id,
                conflict_with_id: inc.belief_id,
                new_confidence: confidence,
                incumbent_effective_weight: w_inc,
                incoming_effective_weight: w_in,
            };
        }

        // Case C: Brand new (subject_id, predicate_id) belief.
        const slot = _.allocateSlot() orelse {
            return BeliefRevisionResult{ action: BeliefRevisionAction.rejected_full };
        };
        const new_id: u32 = _.next_id;
        _.next_id += 1;
        _.records[slot] = BeliefRecord{
            belief_id: new_id,
            subject_id: subject_id,
            predicate_id: predicate_id,
            object_hash: object_hash,
            confidence: confidence,
            source_trust: source_trust,
            observation_count: 1,
            created_ms: now_ms,
            updated_ms: now_ms,
            status: BeliefStatus.active,
            superseded_by: BELIEF_NO_ID,
            occupied: true,
        };
        _.record_count += 1;

        return BeliefRevisionResult{
            action: BeliefRevisionAction.inserted,
            belief_id: new_id,
            conflict_with_id: BELIEF_NO_ID,
            new_confidence: confidence,
            incumbent_effective_weight: 0.0,
            incoming_effective_weight: w_in,
        };
    }

    /// Retracts a belief by `belief_id`. If retracting leaves a single `contested` rival
    /// for the same `(subject_id, predicate_id)`, automatically promotes that survivor to `active`.
    fn retract(*_, belief_id: u32, now_ms: u64): bool {
        if (belief_id == BELIEF_NO_ID) return false;
        let found_idx: usize = BELIEF_MAX_RECORDS;
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            if (_.records[i].occupied && _.records[i].belief_id == belief_id) {
                found_idx = i;
                break;
            }
        }
        if (found_idx == BELIEF_MAX_RECORDS) return false;
        if (_.records[found_idx].status == BeliefStatus.retracted) return false;

        const subj: u64 = _.records[found_idx].subject_id;
        const pred: u32 = _.records[found_idx].predicate_id;
        _.records[found_idx].status = BeliefStatus.retracted;
        if (now_ms >= _.records[found_idx].updated_ms) {
            _.records[found_idx].updated_ms = now_ms;
        }

        // If exactly one contested belief remains for (subj, pred), promote it back to active.
        let contested_survivor: usize = BELIEF_MAX_RECORDS;
        let contested_n: usize = 0;
        let j: usize = 0;
        while (j < BELIEF_MAX_RECORDS) : (j += 1) {
            if (!_.records[j].occupied) continue;
            if (_.records[j].subject_id == subj &&
                _.records[j].predicate_id == pred &&
                _.records[j].status == BeliefStatus.contested)
            {
                contested_survivor = j;
                contested_n += 1;
            }
        }
        if (contested_n == 1 && contested_survivor < BELIEF_MAX_RECORDS) {
            _.records[contested_survivor].status = BeliefStatus.active;
        }
        return true;
    }

    /// Sweeps all live (`active` / `contested`) beliefs and applies half-life confidence decay
    /// up to `now_ms`. Returns the number of beliefs updated.
    fn applyDecay(*_, now_ms: u64): usize {
        let updated: usize = 0;
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            if (!_.records[i].occupied) continue;
            if (!_.isLiveStatus(_.records[i].status)) continue;
            if (now_ms <= _.records[i].updated_ms) continue;

            const elapsed: u64 = now_ms - _.records[i].updated_ms;
            const decayed: f32 = beliefHalfLifeDecay(
                _.records[i].confidence,
                elapsed,
                _.cfg.half_life_ms,
                _.cfg.min_confidence,
            );
            _.records[i].confidence = decayed;
            _.records[i].updated_ms = now_ms;
            updated += 1;
        }
        return updated;
    }

    /// Looks up a belief record by `belief_id`.
    fn getBelief(belief_id: u32): BeliefRecord? {
        if (belief_id == BELIEF_NO_ID) return null;
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            if (_.records[i].occupied && _.records[i].belief_id == belief_id) {
                return _.records[i];
            }
        }
        return null;
    }

    /// Queries the single most credible live belief for `(subject_id, predicate_id)`.
    /// Prefers `active`; if in `contested` state, returns the highest effective-weight candidate.
    fn queryBestBelief(subject_id: u64, predicate_id: u32): BeliefRecord? {
        if (subject_id == 0 || predicate_id == 0) return null;
        let best_idx: usize = BELIEF_MAX_RECORDS;
        let best_score: f32 = -1.0;

        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            const rec = _.records[i];
            if (!rec.occupied) continue;
            if (rec.subject_id != subject_id || rec.predicate_id != predicate_id) continue;
            if (!_.isLiveStatus(rec.status)) continue;

            const score: f32 = beliefEffectiveWeight(rec.confidence, rec.source_trust, 0, _.cfg.half_life_ms, _.cfg.min_confidence);
            if (best_idx == BELIEF_MAX_RECORDS ||
                (rec.status == BeliefStatus.active && _.records[best_idx].status != BeliefStatus.active) ||
                (rec.status == _.records[best_idx].status && score > best_score))
            {
                best_idx = i;
                best_score = score;
            }
        }

        if (best_idx == BELIEF_MAX_RECORDS) return null;
        return _.records[best_idx];
    }

    /// Queries all `contested` candidate beliefs for `subject_id` (and `predicate_id`, or any
    /// predicate if `predicate_id == 0`). Writes up to `out_records.len` records and returns count.
    fn queryContested(
        subject_id: u64,
        predicate_id: u32,
        out_records: *[]BeliefRecord,
    ): usize {
        if (subject_id == 0 || out_records.len == 0) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS && written < out_records.len) : (i += 1) {
            const rec = _.records[i];
            if (!rec.occupied || rec.status != BeliefStatus.contested) continue;
            if (rec.subject_id != subject_id) continue;
            if (predicate_id != 0 && rec.predicate_id != predicate_id) continue;
            out_records[written] = rec;
            written += 1;
        }
        return written;
    }

    /// Queries beliefs for `subject_id`, optionally including inactive (`superseded`/`retracted`) records.
    fn queryBySubject(
        subject_id: u64,
        include_inactive: bool,
        out_records: *[]BeliefRecord,
    ): usize {
        if (subject_id == 0 || out_records.len == 0) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS && written < out_records.len) : (i += 1) {
            const rec = _.records[i];
            if (!rec.occupied || rec.subject_id != subject_id) continue;
            if (!include_inactive && !_.isLiveStatus(rec.status)) continue;
            out_records[written] = rec;
            written += 1;
        }
        return written;
    }

    /// Copies up to `out_audits.len` conflict audit entries (chronological oldest-to-newest)
    /// into `out_audits` and returns the number of entries written.
    fn recentConflicts(out_audits: *[]BeliefConflictAudit): usize {
        if (out_audits.len == 0 || _.audit_count == 0) return 0;
        const start: usize = if (_.audit_count < BELIEF_MAX_AUDIT_ENTRIES) 0 else _.audit_head;
        const n: usize = if (_.audit_count < out_audits.len) _.audit_count else out_audits.len;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const idx: usize = (start + i) % BELIEF_MAX_AUDIT_ENTRIES;
            out_audits[i] = _.audits[idx];
        }
        return n;
    }

    /// Computes live epistemic telemetry including average binary Shannon entropy (nats)
    /// and contradiction ratio `contested / (active + contested)`.
    fn stats(): BeliefStateStats {
        let active_n: usize = 0;
        let contested_n: usize = 0;
        let superseded_n: usize = 0;
        let retracted_n: usize = 0;
        let entropy_sum: f64 = 0.0;
        const eps: f64 = 0.000001;

        let i: usize = 0;
        while (i < BELIEF_MAX_RECORDS) : (i += 1) {
            const rec = _.records[i];
            if (!rec.occupied) continue;
            if (rec.status == BeliefStatus.active) {
                active_n += 1;
            } else if (rec.status == BeliefStatus.contested) {
                contested_n += 1;
            } else if (rec.status == BeliefStatus.superseded) {
                superseded_n += 1;
            } else {
                retracted_n += 1;
            }

            if (_.isLiveStatus(rec.status)) {
                const p: f64 = clampF64(rec.confidence as f64, eps, 1.0 - eps);
                entropy_sum += 0.0 - (p * std.math.log(f64, std.math.e, p) + (1.0 - p) * std.math.log(f64, std.math.e, 1.0 - p));
            }
        }

        const live_n: usize = active_n + contested_n;
        const mean_entropy: f32 = if (live_n > 0) ((entropy_sum / (live_n as f64)) as f32) else 0.0;
        const c_ratio: f32 = if (live_n > 0) (((contested_n as f64) / (live_n as f64)) as f32) else 0.0;

        return BeliefStateStats{
            total_records: _.record_count,
            active_count: active_n,
            contested_count: contested_n,
            superseded_count: superseded_n,
            retracted_count: retracted_n,
            conflict_events: _.conflict_events,
            reinforce_events: _.reinforce_events,
            epistemic_entropy: mean_entropy,
            contradiction_ratio: c_ratio,
        };
    }
}

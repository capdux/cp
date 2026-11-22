// Origin: Pure-CP Stdlib (Zero-Alloc SWIM-Style Multi-Agent Gossip Membership & Failure Detector) | Created: 2026-11-23
// Pure CP zero-heap-allocation SWIM-style multi-agent cluster membership & failure detection state machine:
//   - Fixed 64-member table tracking `agent_id`, `state` (`alive | suspect | dead`), monotonic `incarnation`,
//     64-bit `capabilities` bitmask, and state-change / last-seen timestamps
//   - Strict SWIM incarnation conflict resolution & self-refutation (`applyUpdate`):
//       * Higher `incarnation` overrides lower `incarnation`
//       * Equal `incarnation` converges strictly by `dead > suspect > alive`
//       * Receiving a `suspect` (or `dead`) rumor about the local node with `incarnation >= local.incarnation`
//         automatically bumps `local.incarnation = update.incarnation + 1` and broadcasts an `alive` refutation
//   - Bounded piggyback dissemination queue (`buildGossipBatch`): per-member deduplicated updates with
//     `retransmits_left` budget, prioritizing lowest `tx_count` and newest `seq` changes
//   - Automatic suspicion-timeout promotion to `dead` (`reapSuspects`), Fisher-Yates shuffled round-robin
//     probe target selection (`selectProbeTarget`), capability filtering & health telemetry snapshot

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of agent members tracked inline (zero dynamic heap allocation).
pub const GOSSIP_MAX_MEMBERS: usize = 64;

/// Maximum number of pending piggyback dissemination entries tracked inline.
pub const GOSSIP_QUEUE_CAP: usize = 64;

/// Default suspicion timeout (in milliseconds) before a `suspect` member is promoted to `dead`.
pub const GOSSIP_DEFAULT_SUSPECT_TIMEOUT_MS: u64 = 5000;

/// Default number of piggyback transmissions per membership state change.
pub const GOSSIP_DEFAULT_RETRANSMIT_LIMIT: u16 = 4;

/// Default non-zero PRNG seed for deterministic shuffled round-robin probe target selection.
pub const GOSSIP_DEFAULT_PRNG_SEED: u64 = 0x9E37_79B9_7F4A_7C15;

/// Membership health state in the SWIM state machine (`dead > suspect > alive` at equal incarnation).
pub enum GossipMemberState {
    alive,
    suspect,
    dead,
}

/// Returns the strict SWIM precedence rank for equal-incarnation tie-breaking: `dead (2) > suspect (1) > alive (0)`.
pub fn gossipStateRank(state: GossipMemberState): u8 {
    return match (state) {
        GossipMemberState.alive => 0,
        GossipMemberState.suspect => 1,
        GossipMemberState.dead => 2,
    };
}

/// Outcome returned by `applyUpdate` / `upsertPeer`.
pub enum GossipApplyOutcome {
    ignored_stale,
    applied_new,
    applied_updated,
    refuted_self,
    capacity_full,
}

/// Configuration parameters for `GossipHealthMachine`.
pub struct GossipConfig {
    /// Duration in milliseconds after which a `suspect` node is promoted to `dead` by `reapSuspects`.
    suspect_timeout_ms: u64 = GOSSIP_DEFAULT_SUSPECT_TIMEOUT_MS,
    /// Number of piggyback transmissions for each newly enqueued state change (0 falls back to default 4).
    retransmit_limit: u16 = GOSSIP_DEFAULT_RETRANSMIT_LIMIT,
    /// Deterministic PRNG seed used to shuffle round-robin probe schedules (0 falls back to default seed).
    prng_seed: u64 = GOSSIP_DEFAULT_PRNG_SEED,
}

/// Inline membership record stored in the fixed 64-slot member table.
pub struct GossipMember {
    occupied: bool = false,
    agent_id: u64 = 0,
    state: GossipMemberState = GossipMemberState.alive,
    incarnation: u64 = 0,
    capabilities: u64 = 0,
    state_changed_ms: u64 = 0,
    last_seen_ms: u64 = 0,
}

/// Compact wire/piggyback state-change update exchanged between agents on heartbeats.
pub struct GossipUpdate {
    agent_id: u64 = 0,
    state: GossipMemberState = GossipMemberState.alive,
    incarnation: u64 = 0,
    capabilities: u64 = 0,
}

/// Bounded dissemination queue entry tracking remaining piggyback retransmissions.
pub struct GossipDisseminationEntry {
    active: bool = false,
    update: GossipUpdate = GossipUpdate{},
    retransmits_left: u16 = 0,
    tx_count: u16 = 0,
    seq: u64 = 0,
}

/// Cluster health & dissemination telemetry snapshot.
pub struct GossipHealthStats {
    local_agent_id: u64 = 0,
    local_incarnation: u64 = 0,
    member_count: usize = 0,
    capacity: usize = GOSSIP_MAX_MEMBERS,
    alive_count: usize = 0,
    suspect_count: usize = 0,
    dead_count: usize = 0,
    pending_disseminations: usize = 0,
    total_updates_applied: u64 = 0,
    total_updates_ignored: u64 = 0,
    total_self_refutations: u64 = 0,
    total_suspects_reaped: u64 = 0,
    total_gossip_sent: u64 = 0,
}

// ============================================================================
// 2. Core Zero-Alloc SWIM Gossip Health State Machine
// ============================================================================

/// Zero-heap-allocation SWIM-style multi-agent membership & failure detection state machine.
pub struct GossipHealthMachine {
    local_agent_id: u64 = 0,
    local_slot: usize = 0,
    config: GossipConfig = GossipConfig{},
    members: [GOSSIP_MAX_MEMBERS]GossipMember = [GossipMember{}; GOSSIP_MAX_MEMBERS],
    member_count: usize = 0,
    queue: [GOSSIP_QUEUE_CAP]GossipDisseminationEntry = [GossipDisseminationEntry{}; GOSSIP_QUEUE_CAP],
    queue_count: usize = 0,
    next_seq: u64 = 1,
    probe_order: [GOSSIP_MAX_MEMBERS]u64 = [0; GOSSIP_MAX_MEMBERS],
    probe_order_len: usize = 0,
    probe_cursor: usize = 0,
    prng_state: u64 = GOSSIP_DEFAULT_PRNG_SEED,
    total_updates_applied: u64 = 0,
    total_updates_ignored: u64 = 0,
    total_self_refutations: u64 = 0,
    total_suspects_reaped: u64 = 0,
    total_gossip_sent: u64 = 0,

    /// Initializes the state machine with the local agent node (`agent_id` must be non-zero).
    /// Automatically enqueues the local node's initial `alive` announcement for piggyback gossip.
    pub fn init(
        local_agent_id: u64,
        capabilities: u64,
        now_ms: u64,
        config: GossipConfig,
    ): GossipHealthMachine? {
        if (local_agent_id == 0) return null;

        const normalized_cfg = GossipConfig{
            suspect_timeout_ms: config.suspect_timeout_ms,
            retransmit_limit: if (config.retransmit_limit == 0) GOSSIP_DEFAULT_RETRANSMIT_LIMIT else config.retransmit_limit,
            prng_seed: if (config.prng_seed == 0) GOSSIP_DEFAULT_PRNG_SEED else config.prng_seed,
        };

        let m = GossipHealthMachine{
            local_agent_id: local_agent_id,
            local_slot: 0,
            config: normalized_cfg,
            member_count: 1,
            prng_state: normalized_cfg.prng_seed,
        };

        m.members[0] = GossipMember{
            occupied: true,
            agent_id: local_agent_id,
            state: GossipMemberState.alive,
            incarnation: 1,
            capabilities: capabilities,
            state_changed_ms: now_ms,
            last_seen_ms: now_ms,
        };

        m.enqueueDissemination(GossipUpdate{
            agent_id: local_agent_id,
            state: GossipMemberState.alive,
            incarnation: 1,
            capabilities: capabilities,
        });

        return m;
    }

    // ---- Internal Helpers ----

    fn findSlot(agent_id: u64): usize? {
        if (agent_id == 0) return null;
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (_.members[i].occupied && _.members[i].agent_id == agent_id) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (!_.members[i].occupied) return i;
        }
        return null;
    }

    /// Deterministic xorshift64* PRNG for probe schedule shuffling (zero OS/syscall dependency).
    fn nextRand(*_): u64 {
        let x: u64 = _.prng_state;
        if (x == 0) x = GOSSIP_DEFAULT_PRNG_SEED;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.prng_state = x;
        return x *% 0x2545_F491_4F6C_DD1D;
    }

    /// Inserts or deduplicates a state-change update in the bounded piggyback dissemination queue.
    /// If full, evicts the entry with the fewest remaining retransmits (breaking ties by oldest `seq`).
    fn enqueueDissemination(*_, update: GossipUpdate) {
        const seq = _.next_seq;
        _.next_seq += 1;
        const limit = _.config.retransmit_limit;

        // 1. Deduplicate by agent_id: replace existing active entry in place with latest update
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < GOSSIP_QUEUE_CAP) : (i += 1) {
            if (_.queue[i].active) {
                if (_.queue[i].update.agent_id == update.agent_id) {
                    _.queue[i] = GossipDisseminationEntry{
                        active: true,
                        update: update,
                        retransmits_left: limit,
                        tx_count: 0,
                        seq: seq,
                    };
                    return;
                }
            } else if (free_idx == null) {
                free_idx = i;
            }
        }

        // 2. Write into free slot if available
        if (slot := free_idx) {
            _.queue[slot] = GossipDisseminationEntry{
                active: true,
                update: update,
                retransmits_left: limit,
                tx_count: 0,
                seq: seq,
            };
            _.queue_count += 1;
            return;
        }

        // 3. Queue full: evict lowest `retransmits_left`, breaking ties by oldest `seq`
        let evict_idx: usize = 0;
        let j: usize = 1;
        while (j < GOSSIP_QUEUE_CAP) : (j += 1) {
            const cur = _.queue[j];
            const best = _.queue[evict_idx];
            if (cur.retransmits_left < best.retransmits_left ||
                (cur.retransmits_left == best.retransmits_left && cur.seq < best.seq))
            {
                evict_idx = j;
            }
        }

        _.queue[evict_idx] = GossipDisseminationEntry{
            active: true,
            update: update,
            retransmits_left: limit,
            tx_count: 0,
            seq: seq,
        };
    }

    // ---- Public Membership & SWIM Protocol API ----

    /// Adds a newly discovered `alive` peer (at incarnation 1) or refreshes an existing `alive` peer's `last_seen_ms`.
    pub fn upsertPeer(*_, agent_id: u64, capabilities: u64, now_ms: u64): GossipApplyOutcome {
        if (agent_id == 0) {
            _.total_updates_ignored += 1;
            return GossipApplyOutcome.ignored_stale;
        }
        if (idx := _.findSlot(agent_id)) {
            if (_.members[idx].state == GossipMemberState.alive) {
                _.members[idx].last_seen_ms = now_ms;
                if (capabilities != 0 && _.members[idx].capabilities != capabilities && agent_id != _.local_agent_id) {
                    _.members[idx].capabilities = capabilities;
                }
            }
            return GossipApplyOutcome.ignored_stale;
        }
        return _.applyUpdate(GossipUpdate{
            agent_id: agent_id,
            state: GossipMemberState.alive,
            incarnation: 1,
            capabilities: capabilities,
        }, now_ms);
    }

    /// Applies an incoming SWIM gossip update with strict incarnation precedence and automatic self-refutation:
    ///   - Higher `incarnation` overrides lower `incarnation`.
    ///   - Equal `incarnation` converges strictly by `dead > suspect > alive`.
    ///   - A `suspect` or `dead` rumor targeting the local node with `incarnation >= local.incarnation`
    ///     triggers automatic refutation: `local.incarnation = update.incarnation + 1` and broadcasts `alive`.
    pub fn applyUpdate(*_, update: GossipUpdate, now_ms: u64): GossipApplyOutcome {
        if (update.agent_id == 0) {
            _.total_updates_ignored += 1;
            return GossipApplyOutcome.ignored_stale;
        }

        // Special handling for updates targeting the local node (SWIM self-refutation rule)
        if (update.agent_id == _.local_agent_id) {
            const local_inc = _.members[_.local_slot].incarnation;

            if (update.state == GossipMemberState.suspect || update.state == GossipMemberState.dead) {
                if (update.incarnation >= local_inc) {
                    const next_inc = update.incarnation + 1;
                    _.members[_.local_slot].incarnation = next_inc;
                    _.members[_.local_slot].state = GossipMemberState.alive;
                    _.members[_.local_slot].state_changed_ms = now_ms;
                    _.members[_.local_slot].last_seen_ms = now_ms;

                    _.enqueueDissemination(GossipUpdate{
                        agent_id: _.local_agent_id,
                        state: GossipMemberState.alive,
                        incarnation: next_inc,
                        capabilities: _.members[_.local_slot].capabilities,
                    });
                    _.total_self_refutations += 1;
                    return GossipApplyOutcome.refuted_self;
                }
                _.total_updates_ignored += 1;
                return GossipApplyOutcome.ignored_stale;
            }

            // `alive` update for local node: only adopt if higher incarnation (e.g. post-restart catch-up)
            if (update.incarnation > local_inc) {
                _.members[_.local_slot].incarnation = update.incarnation;
                _.members[_.local_slot].last_seen_ms = now_ms;
                _.enqueueDissemination(GossipUpdate{
                    agent_id: _.local_agent_id,
                    state: GossipMemberState.alive,
                    incarnation: update.incarnation,
                    capabilities: _.members[_.local_slot].capabilities,
                });
                _.total_updates_applied += 1;
                return GossipApplyOutcome.applied_updated;
            }

            _.total_updates_ignored += 1;
            return GossipApplyOutcome.ignored_stale;
        }

        // Remote member update
        if (idx := _.findSlot(update.agent_id)) {
            const cur = _.members[idx];

            if (update.incarnation < cur.incarnation) {
                _.total_updates_ignored += 1;
                return GossipApplyOutcome.ignored_stale;
            }

            if (update.incarnation == cur.incarnation) {
                const incoming_rank = gossipStateRank(update.state);
                const current_rank = gossipStateRank(cur.state);
                if (incoming_rank <= current_rank) {
                    // Refresh `last_seen_ms` if equal-incarnation alive heartbeat arrives
                    if (update.state == GossipMemberState.alive && cur.state == GossipMemberState.alive) {
                        _.members[idx].last_seen_ms = now_ms;
                    }
                    _.total_updates_ignored += 1;
                    return GossipApplyOutcome.ignored_stale;
                }
            }

            // Accepted: either higher incarnation, or same incarnation with stricter state (`dead > suspect > alive`)
            const caps = if (update.capabilities != 0) update.capabilities else cur.capabilities;
            _.members[idx].state = update.state;
            _.members[idx].incarnation = update.incarnation;
            _.members[idx].capabilities = caps;
            _.members[idx].state_changed_ms = now_ms;
            if (update.state == GossipMemberState.alive) {
                _.members[idx].last_seen_ms = now_ms;
            }

            _.enqueueDissemination(GossipUpdate{
                agent_id: update.agent_id,
                state: update.state,
                incarnation: update.incarnation,
                capabilities: caps,
            });
            _.total_updates_applied += 1;
            return GossipApplyOutcome.applied_updated;
        }

        // New remote member discovery
        if (slot := _.findFreeSlot()) {
            _.members[slot] = GossipMember{
                occupied: true,
                agent_id: update.agent_id,
                state: update.state,
                incarnation: update.incarnation,
                capabilities: update.capabilities,
                state_changed_ms: now_ms,
                last_seen_ms: now_ms,
            };
            _.member_count += 1;

            _.enqueueDissemination(update);
            _.total_updates_applied += 1;
            return GossipApplyOutcome.applied_new;
        }

        _.total_updates_ignored += 1;
        return GossipApplyOutcome.capacity_full;
    }

    /// Applies a batch of piggybacked gossip updates and returns the count of accepted/refuted updates.
    pub fn applyBatch(*_, updates: []const GossipUpdate, now_ms: u64): usize {
        let accepted: usize = 0;
        let i: usize = 0;
        while (i < updates.len) : (i += 1) {
            const outcome = _.applyUpdate(updates[i], now_ms);
            if (outcome == GossipApplyOutcome.applied_new ||
                outcome == GossipApplyOutcome.applied_updated ||
                outcome == GossipApplyOutcome.refuted_self)
            {
                accepted += 1;
            }
        }
        return accepted;
    }

    /// Locally marks an `alive` remote peer as `suspect` (e.g. after direct & indirect probe timeout)
    /// and enqueues the suspicion rumor for piggyback dissemination.
    pub fn markSuspect(*_, agent_id: u64, now_ms: u64): bool {
        if (agent_id == 0 || agent_id == _.local_agent_id) return false;
        const idx = _.findSlot(agent_id) ?? return false;
        if (_.members[idx].state != GossipMemberState.alive) return false;

        _.members[idx].state = GossipMemberState.suspect;
        _.members[idx].state_changed_ms = now_ms;

        _.enqueueDissemination(GossipUpdate{
            agent_id: agent_id,
            state: GossipMemberState.suspect,
            incarnation: _.members[idx].incarnation,
            capabilities: _.members[idx].capabilities,
        });
        _.total_updates_applied += 1;
        return true;
    }

    /// Records a direct probe ACK / heartbeat from an `alive` remote peer, refreshing `last_seen_ms`.
    pub fn recordDirectAck(*_, agent_id: u64, now_ms: u64): bool {
        if (agent_id == 0) return false;
        const idx = _.findSlot(agent_id) ?? return false;
        if (_.members[idx].state == GossipMemberState.dead) return false;
        _.members[idx].last_seen_ms = now_ms;
        return _.members[idx].state == GossipMemberState.alive;
    }

    /// Updates the local agent's 64-bit capability bitmask, bumps `local.incarnation += 1`,
    /// and enqueues an `alive` broadcast so peers converge on the new capability set.
    pub fn updateLocalCapabilities(*_, new_capabilities: u64, now_ms: u64): u64 {
        const next_inc = _.members[_.local_slot].incarnation + 1;
        _.members[_.local_slot].incarnation = next_inc;
        _.members[_.local_slot].capabilities = new_capabilities;
        _.members[_.local_slot].state_changed_ms = now_ms;
        _.members[_.local_slot].last_seen_ms = now_ms;

        _.enqueueDissemination(GossipUpdate{
            agent_id: _.local_agent_id,
            state: GossipMemberState.alive,
            incarnation: next_inc,
            capabilities: new_capabilities,
        });
        return next_inc;
    }

    /// Promotes any `suspect` remote members whose suspicion duration has reached `suspect_timeout_ms`
    /// to `dead` and enqueues `dead` notifications. Returns the number of newly reaped members.
    pub fn reapSuspects(*_, now_ms: u64): usize {
        let reaped: usize = 0;
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (!_.members[i].occupied) continue;
            if (i == _.local_slot) continue;
            if (_.members[i].state != GossipMemberState.suspect) continue;

            if (now_ms >= _.members[i].state_changed_ms &&
                (now_ms - _.members[i].state_changed_ms) >= _.config.suspect_timeout_ms)
            {
                _.members[i].state = GossipMemberState.dead;
                _.members[i].state_changed_ms = now_ms;
                _.enqueueDissemination(GossipUpdate{
                    agent_id: _.members[i].agent_id,
                    state: GossipMemberState.dead,
                    incarnation: _.members[i].incarnation,
                    capabilities: _.members[i].capabilities,
                });
                reaped += 1;
                _.total_suspects_reaped += 1;
            }
        }
        return reaped;
    }

    /// Evicts `dead` remote members that have remained dead for at least `min_dead_age_ms`
    /// to reclaim member-table slots. Returns the number of purged slots.
    pub fn purgeDead(*_, now_ms: u64, min_dead_age_ms: u64): usize {
        let purged: usize = 0;
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (!_.members[i].occupied || i == _.local_slot) continue;
            if (_.members[i].state != GossipMemberState.dead) continue;

            if (now_ms >= _.members[i].state_changed_ms &&
                (now_ms - _.members[i].state_changed_ms) >= min_dead_age_ms)
            {
                _.members[i] = GossipMember{};
                if (_.member_count > 0) _.member_count -= 1;
                purged += 1;
            }
        }
        return purged;
    }

    /// Builds a piggyback gossip batch of at most `min(max_items, out_updates.len)` updates.
    /// Strictly prioritizes entries with the fewest transmissions so far (`tx_count`),
    /// breaking ties in favor of the newest state change (`seq`).
    /// Decrements `retransmits_left` for each selected entry and retires exhausted entries.
    pub fn buildGossipBatch(*_, max_items: usize, out_updates: *[]GossipUpdate): usize {
        const limit = if (max_items < out_updates.len) max_items else out_updates.len;
        if (limit == 0 || _.queue_count == 0) return 0;

        let picked: [GOSSIP_QUEUE_CAP]bool = [false; GOSSIP_QUEUE_CAP];
        let written: usize = 0;

        while (written < limit) {
            let best_idx: usize? = null;
            let best_tx: u16 = 0xFFFF;
            let best_seq: u64 = 0;

            let i: usize = 0;
            while (i < GOSSIP_QUEUE_CAP) : (i += 1) {
                if (picked[i] || !_.queue[i].active || _.queue[i].retransmits_left == 0) continue;
                const entry = _.queue[i];
                if (best_idx == null ||
                    entry.tx_count < best_tx ||
                    (entry.tx_count == best_tx && entry.seq > best_seq))
                {
                    best_idx = i;
                    best_tx = entry.tx_count;
                    best_seq = entry.seq;
                }
            }

            const chosen = best_idx ?? break;
            picked[chosen] = true;
            out_updates[written] = _.queue[chosen].update;
            written += 1;

            _.queue[chosen].tx_count += 1;
            _.queue[chosen].retransmits_left -= 1;
            if (_.queue[chosen].retransmits_left == 0) {
                _.queue[chosen].active = false;
                if (_.queue_count > 0) _.queue_count -= 1;
            }
        }

        _.total_gossip_sent += written as u64;
        return written;
    }

    /// Selects the next non-local probe target (`alive` or `suspect`) using SWIM-style
    /// Fisher-Yates shuffled round-robin scheduling, guaranteeing O(N) uniform coverage
    /// without consecutive duplicates across a full probe round.
    pub fn selectProbeTarget(*_): u64? {
        // Try remaining entries in current shuffled round
        while (_.probe_cursor < _.probe_order_len) {
            const candidate_id = _.probe_order[_.probe_cursor];
            _.probe_cursor += 1;
            if (idx := _.findSlot(candidate_id)) {
                if (idx != _.local_slot && _.members[idx].state != GossipMemberState.dead) {
                    return candidate_id;
                }
            }
        }

        // Rebuild probe round with all currently probeable (non-local, non-dead) members
        let count: usize = 0;
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (!_.members[i].occupied || i == _.local_slot) continue;
            if (_.members[i].state == GossipMemberState.dead) continue;
            _.probe_order[count] = _.members[i].agent_id;
            count += 1;
        }

        _.probe_order_len = count;
        _.probe_cursor = 0;
        if (count == 0) return null;

        // Fisher-Yates in-place shuffle for randomized uniform probe order
        if (count > 1) {
            let k: usize = count - 1;
            while (k > 0) : (k -= 1) {
                const r = (_.nextRand() % ((k + 1) as u64)) as usize;
                const tmp = _.probe_order[k];
                _.probe_order[k] = _.probe_order[r];
                _.probe_order[r] = tmp;
            }
        }

        const chosen = _.probe_order[0];
        _.probe_cursor = 1;
        return chosen;
    }

    /// Looks up a member record by `agent_id`.
    pub fn getMember(agent_id: u64): GossipMember? {
        const idx = _.findSlot(agent_id) ?? return null;
        return _.members[idx];
    }

    /// Returns a copy of the local node's membership record.
    pub fn localMember(): GossipMember {
        return _.members[_.local_slot];
    }

    /// Lists all `alive` members whose `capabilities` satisfy `(m.capabilities & required_caps) == required_caps`.
    /// When `include_local` is `false`, omits the local node. Returns the number of IDs written to `out_ids`.
    pub fn listCapableAlive(
        required_caps: u64,
        include_local: bool,
        out_ids: *[]u64,
    ): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS && written < out_ids.len) : (i += 1) {
            if (!_.members[i].occupied) continue;
            if (!include_local && i == _.local_slot) continue;
            if (_.members[i].state != GossipMemberState.alive) continue;
            if ((_.members[i].capabilities & required_caps) != required_caps) continue;
            out_ids[written] = _.members[i].agent_id;
            written += 1;
        }
        return written;
    }

    /// Returns a cluster health & gossip dissemination telemetry snapshot.
    pub fn stats(): GossipHealthStats {
        let alive: usize = 0;
        let suspect: usize = 0;
        let dead: usize = 0;

        let i: usize = 0;
        while (i < GOSSIP_MAX_MEMBERS) : (i += 1) {
            if (!_.members[i].occupied) continue;
            match (_.members[i].state) {
                GossipMemberState.alive => { alive += 1; },
                GossipMemberState.suspect => { suspect += 1; },
                GossipMemberState.dead => { dead += 1; },
            }
        }

        return GossipHealthStats{
            local_agent_id: _.local_agent_id,
            local_incarnation: _.members[_.local_slot].incarnation,
            member_count: _.member_count,
            capacity: GOSSIP_MAX_MEMBERS,
            alive_count: alive,
            suspect_count: suspect,
            dead_count: dead,
            pending_disseminations: _.queue_count,
            total_updates_applied: _.total_updates_applied,
            total_updates_ignored: _.total_updates_ignored,
            total_self_refutations: _.total_self_refutations,
            total_suspects_reaped: _.total_suspects_reaped,
            total_gossip_sent: _.total_gossip_sent,
        };
    }
}

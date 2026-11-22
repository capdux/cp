// Origin: Pure-CP Stdlib (Zero-Alloc RFC 8305 Happy Eyeballs v2 Dual-Stack Connection Racing State Machine) | Created: 2026-11-22
// Pure CP zero-heap-allocation RFC 8305 Happy Eyeballs v2 dual-stack IPv6/IPv4 connection racing engine:
//   - First Address Family Preference & RFC 8305 §4 interleaving (default IPv6-first 1:1 alternation)
//   - Asynchronous incremental DNS arrival (`addResolvedAddresses` / `markDnsComplete`) with configurable
//     RFC 8305 §3 Resolution Delay (default 50ms) when non-preferred family arrives first
//   - Pure time-driven state machine (`nextAction(now_ms)`): `start_attempt` / `wait_until` / `connected` / `all_failed`
//   - RFC 8305 §5 Connection Attempt Delay (default 250ms) with immediate early wake-up upon attempt failure
//   - First-winner completion (`markAttemptSucceeded`) automatically flagging all other in-flight attempts for cancellation

/// Maximum total resolved IPv6 + IPv4 candidate addresses tracked (zero dynamic heap).
pub const HE_MAX_CANDIDATES: usize = 32;

/// RFC 8305 §3 default Resolution Delay (ms) waiting for preferred family (AAAA) after A arrives first.
pub const HE_DEFAULT_RESOLUTION_DELAY_MS: u64 = 50;

/// RFC 8305 §5 default Connection Attempt Delay (ms) between staggered connection attempts.
pub const HE_DEFAULT_ATTEMPT_DELAY_MS: u64 = 250;

/// RFC 8305 §4 default First Address Family Count (number of preferred-family addresses tried before interleaving).
pub const HE_DEFAULT_FIRST_FAMILY_COUNT: u8 = 1;

/// Sentinel invalid attempt identifier.
pub const HE_INVALID_ATTEMPT_ID: u16 = 0xFFFF;

const HE_MAX_DELAY_MS: u64 = 60_000;

fn ipBytesEqual(a: [16]u8, b: [16]u8, family: HeAddressFamily): bool {
    const n: usize = if (family == HeAddressFamily.ipv4) 4 else 16;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// 1. Public Enums, Candidate & Action Models
// ============================================================================

/// IP address family for dual-stack Happy Eyeballs racing.
pub enum HeAddressFamily {
    ipv6,
    ipv4,
}

/// Lifecycle state of an individual candidate connection attempt.
pub enum HeAttemptState {
    pending,
    in_flight,
    succeeded,
    failed,
    cancel_pending,
    canceled,
}

/// Next scheduling directive emitted by `HappyEyeballsEngine.nextAction`.
pub enum HeActionKind {
    start_attempt,
    wait_until,
    connected,
    all_failed,
}

/// Zero-alloc POD candidate endpoint (IPv4 stored in `ip_bytes[0..4]`, IPv6 in `ip_bytes[0..16]`).
pub struct HeCandidate {
    family: HeAddressFamily = HeAddressFamily.ipv6;
    ip_bytes: [16]u8 = [0; 16];
    port: u16 = 0;
    addr_id: u32 = 0;

    /// Constructs an IPv4 candidate endpoint.
    static fn fromIpv4(ip4: [4]u8, port: u16, addr_id: u32): HeCandidate {
        let bytes: [16]u8 = [0; 16];
        bytes[0] = ip4[0];
        bytes[1] = ip4[1];
        bytes[2] = ip4[2];
        bytes[3] = ip4[3];
        return HeCandidate{
            family: HeAddressFamily.ipv4,
            ip_bytes: bytes,
            port: port,
            addr_id: addr_id,
        };
    }

    /// Constructs an IPv6 candidate endpoint.
    static fn fromIpv6(ip6: [16]u8, port: u16, addr_id: u32): HeCandidate {
        return HeCandidate{
            family: HeAddressFamily.ipv6,
            ip_bytes: ip6,
            port: port,
            addr_id: addr_id,
        };
    }
}

/// Configuration parameters for `HappyEyeballsEngine` (RFC 8305 defaults).
pub struct HeConfig {
    preferred_family: HeAddressFamily = HeAddressFamily.ipv6;
    first_family_count: u8 = HE_DEFAULT_FIRST_FAMILY_COUNT;
    resolution_delay_ms: u64 = HE_DEFAULT_RESOLUTION_DELAY_MS;
    connection_attempt_delay_ms: u64 = HE_DEFAULT_ATTEMPT_DELAY_MS;

    /// Returns RFC 8305 recommended defaults (IPv6 preferred, 50ms resolution delay, 250ms attempt delay).
    static fn defaults(): HeConfig {
        return HeConfig{
            preferred_family: HeAddressFamily.ipv6,
            first_family_count: HE_DEFAULT_FIRST_FAMILY_COUNT,
            resolution_delay_ms: HE_DEFAULT_RESOLUTION_DELAY_MS,
            connection_attempt_delay_ms: HE_DEFAULT_ATTEMPT_DELAY_MS,
        };
    }

    /// Validates `first_family_count >= 1`, `resolution_delay_ms <= 60_000`, and `1 <= connection_attempt_delay_ms <= 60_000`.
    fn isValid(): bool {
        if (_.first_family_count == 0) return false;
        if (_.resolution_delay_ms > HE_MAX_DELAY_MS) return false;
        if (_.connection_attempt_delay_ms == 0 || _.connection_attempt_delay_ms > HE_MAX_DELAY_MS) return false;
        return true;
    }
}

/// Action returned by `HappyEyeballsEngine.nextAction`.
pub struct HeAction {
    kind: HeActionKind = HeActionKind.wait_until;
    attempt_id: u16 = HE_INVALID_ATTEMPT_ID;
    candidate: HeCandidate = HeCandidate{};
    deadline_ms: u64 = 0;
}

/// Aggregate operational telemetry snapshot for `HappyEyeballsEngine`.
pub struct HeStats {
    total_candidates: usize = 0;
    ipv6_candidates: usize = 0;
    ipv4_candidates: usize = 0;
    started_attempts: usize = 0;
    in_flight_attempts: usize = 0;
    failed_attempts: usize = 0;
    cancel_pending_attempts: usize = 0;
    winner_attempt_id: u16 = HE_INVALID_ATTEMPT_ID;
    dns_ipv6_done: bool = false;
    dns_ipv4_done: bool = false;
}

struct HeAttemptSlot {
    candidate: HeCandidate = HeCandidate{};
    state: HeAttemptState = HeAttemptState.pending;
    started_at_ms: u64 = 0;
    occupied: bool = false;
}

// ============================================================================
// 2. Zero-Alloc RFC 8305 Happy Eyeballs v2 State Machine
// ============================================================================

/// Zero-heap-allocation RFC 8305 Happy Eyeballs v2 dual-stack connection racing state machine.
pub struct HappyEyeballsEngine {
    cfg: HeConfig = HeConfig{},
    slots: [HE_MAX_CANDIDATES]HeAttemptSlot = [HeAttemptSlot{}; HE_MAX_CANDIDATES];
    slot_count: usize = 0;
    ipv6_count: usize = 0;
    ipv4_count: usize = 0;
    dns_ipv6_done: bool = false;
    dns_ipv4_done: bool = false;
    resolution_timer_active: bool = false;
    resolution_deadline_ms: u64 = 0;
    next_attempt_deadline_ms: u64 = 0;
    has_started_any: bool = false;
    pref_initial_remaining: u8 = HE_DEFAULT_FIRST_FAMILY_COUNT;
    next_turn_family: HeAddressFamily = HeAddressFamily.ipv6;
    winner_attempt_id: u16 = HE_INVALID_ATTEMPT_ID;
    last_now_ms: u64 = 0;

    /// Initializes a `HappyEyeballsEngine` at monotonic timestamp `start_ms`.
    static fn init(cfg: HeConfig, start_ms: u64): HappyEyeballsEngine? {
        if (!cfg.isValid()) return null;
        return HappyEyeballsEngine{
            cfg: cfg,
            slots: [HeAttemptSlot{}; HE_MAX_CANDIDATES],
            slot_count: 0,
            ipv6_count: 0,
            ipv4_count: 0,
            dns_ipv6_done: false,
            dns_ipv4_done: false,
            resolution_timer_active: false,
            resolution_deadline_ms: 0,
            next_attempt_deadline_ms: start_ms,
            has_started_any: false,
            pref_initial_remaining: cfg.first_family_count,
            next_turn_family: cfg.preferred_family,
            winner_attempt_id: HE_INVALID_ATTEMPT_ID,
            last_now_ms: start_ms,
        };
    }

    fn effectiveNow(*_, now_ms: u64): u64 {
        if (now_ms < _.last_now_ms) {
            return _.last_now_ms;
        }
        _.last_now_ms = now_ms;
        return now_ms;
    }

    fn otherFamily(fam: HeAddressFamily): HeAddressFamily {
        if (fam == HeAddressFamily.ipv6) return HeAddressFamily.ipv4;
        return HeAddressFamily.ipv6;
    }

    fn isDnsDoneFor(fam: HeAddressFamily): bool {
        if (fam == HeAddressFamily.ipv6) return _.dns_ipv6_done;
        return _.dns_ipv4_done;
    }

    fn hasDuplicateCandidate(cand: HeCandidate): bool {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            const existing = _.slots[i].candidate;
            if (existing.family == cand.family && existing.port == cand.port &&
                ipBytesEqual(existing.ip_bytes, cand.ip_bytes, cand.family)) {
                return true;
            }
        }
        return false;
    }

    fn hasPendingOfFamily(fam: HeAddressFamily): bool {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == HeAttemptState.pending &&
                _.slots[i].candidate.family == fam) {
                return true;
            }
        }
        return false;
    }

    fn findNextPendingOfFamily(fam: HeAddressFamily): u16 {
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].state == HeAttemptState.pending &&
                _.slots[i].candidate.family == fam) {
                return i as u16;
            }
        }
        return HE_INVALID_ATTEMPT_ID;
    }

    /// Marks DNS resolution complete for `family` (e.g. empty AAAA response), cancelling any pending resolution delay.
    fn markDnsComplete(*_, family: HeAddressFamily, now_ms: u64) {
        const eff_now: u64 = _.effectiveNow(now_ms);
        if (family == HeAddressFamily.ipv6) {
            _.dns_ipv6_done = true;
        } else {
            _.dns_ipv4_done = true;
        }
        if (family == _.cfg.preferred_family && _.resolution_timer_active) {
            _.resolution_timer_active = false;
            _.resolution_deadline_ms = eff_now;
        }
    }

    /// Appends a batch of asynchronously resolved addresses for `family`, deduplicating against existing entries
    /// and enforcing `HE_MAX_CANDIDATES`. Starts RFC 8305 §3 Resolution Delay if non-preferred family arrives first.
    /// Returns the number of newly added candidates.
    fn addResolvedAddresses(
        *_,
        family: HeAddressFamily,
        candidates: []const HeCandidate,
        dns_complete: bool,
        now_ms: u64,
    ): usize {
        const eff_now: u64 = _.effectiveNow(now_ms);
        let added: usize = 0;

        let i: usize = 0;
        while (i < candidates.len && _.slot_count < HE_MAX_CANDIDATES) : (i += 1) {
            let cand = candidates[i];
            if (cand.family != family) continue;
            if (_.hasDuplicateCandidate(cand)) continue;

            const init_state = if (_.winner_attempt_id != HE_INVALID_ATTEMPT_ID)
                HeAttemptState.canceled
            else
                HeAttemptState.pending;

            _.slots[_.slot_count] = HeAttemptSlot{
                candidate: cand,
                state: init_state,
                started_at_ms: 0,
                occupied: true,
            };
            _.slot_count += 1;
            if (family == HeAddressFamily.ipv6) {
                _.ipv6_count += 1;
            } else {
                _.ipv4_count += 1;
            }
            added += 1;
        }

        if (dns_complete) {
            if (family == HeAddressFamily.ipv6) {
                _.dns_ipv6_done = true;
            } else {
                _.dns_ipv4_done = true;
            }
        }

        // RFC 8305 §3 Resolution Delay handling:
        // If preferred family arrived or completed, clear any resolution delay immediately.
        if (family == _.cfg.preferred_family && (added > 0 || dns_complete)) {
            _.resolution_timer_active = false;
            _.resolution_deadline_ms = eff_now;
        } else if (family != _.cfg.preferred_family && added > 0 && !_.has_started_any &&
            !_.isDnsDoneFor(_.cfg.preferred_family) && !_.hasPendingOfFamily(_.cfg.preferred_family)) {
            if (!_.resolution_timer_active && _.cfg.resolution_delay_ms > 0) {
                _.resolution_timer_active = true;
                _.resolution_deadline_ms = eff_now + _.cfg.resolution_delay_ms;
            }
        }

        return added;
    }

    /// Outputs the RFC 8305 §4 interleaved preference order of all currently pending candidates into `out_order`.
    fn previewInterleavedOrder(out_order: *[]HeCandidate): usize {
        if (out_order.len == 0 || _.slot_count == 0) return 0;

        let used: [HE_MAX_CANDIDATES]bool = [false; HE_MAX_CANDIDATES];
        let pref_rem: u8 = _.pref_initial_remaining;
        let turn: HeAddressFamily = _.next_turn_family;
        let written: usize = 0;

        while (written < out_order.len) {
            const want: HeAddressFamily = if (pref_rem > 0) _.cfg.preferred_family else turn;
            let picked: usize = HE_MAX_CANDIDATES;

            // Try wanted family first
            let i: usize = 0;
            while (i < _.slot_count) : (i += 1) {
                if (!used[i] && _.slots[i].occupied && _.slots[i].state == HeAttemptState.pending &&
                    _.slots[i].candidate.family == want) {
                    picked = i;
                    break;
                }
            }

            // Fallback to other family if wanted family has no pending candidates
            if (picked >= _.slot_count) {
                const alt: HeAddressFamily = _.otherFamily(want);
                let j: usize = 0;
                while (j < _.slot_count) : (j += 1) {
                    if (!used[j] && _.slots[j].occupied && _.slots[j].state == HeAttemptState.pending &&
                        _.slots[j].candidate.family == alt) {
                        picked = j;
                        break;
                    }
                }
            }

            if (picked >= _.slot_count) break;

            used[picked] = true;
            out_order[written] = _.slots[picked].candidate;
            written += 1;

            const chosen_fam = _.slots[picked].candidate.family;
            if (pref_rem > 0 && chosen_fam == _.cfg.preferred_family) {
                pref_rem -= 1;
                if (pref_rem == 0) {
                    turn = _.otherFamily(_.cfg.preferred_family);
                }
            } else {
                turn = _.otherFamily(chosen_fam);
            }
        }
        return written;
    }

    fn pickNextPendingSlot(*_): u16 {
        const want: HeAddressFamily = if (_.pref_initial_remaining > 0) _.cfg.preferred_family else _.next_turn_family;
        let slot: u16 = _.findNextPendingOfFamily(want);
        if (slot == HE_INVALID_ATTEMPT_ID) {
            slot = _.findNextPendingOfFamily(_.otherFamily(want));
        }
        if (slot == HE_INVALID_ATTEMPT_ID) return HE_INVALID_ATTEMPT_ID;

        const chosen_fam = _.slots[slot as usize].candidate.family;
        if (_.pref_initial_remaining > 0 && chosen_fam == _.cfg.preferred_family) {
            _.pref_initial_remaining -= 1;
            if (_.pref_initial_remaining == 0) {
                _.next_turn_family = _.otherFamily(_.cfg.preferred_family);
            }
        } else {
            _.next_turn_family = _.otherFamily(chosen_fam);
        }
        return slot;
    }

    /// Advances the state machine to monotonic timestamp `now_ms` and returns the next scheduling action:
    /// `start_attempt`, `wait_until`, `connected`, or `all_failed`.
    fn nextAction(*_, now_ms: u64): HeAction {
        const eff_now: u64 = _.effectiveNow(now_ms);

        // Terminal success
        if (_.winner_attempt_id != HE_INVALID_ATTEMPT_ID) {
            return HeAction{
                kind: HeActionKind.connected,
                attempt_id: _.winner_attempt_id,
                candidate: _.slots[_.winner_attempt_id as usize].candidate,
                deadline_ms: eff_now,
            };
        }

        // Check if any pending candidate exists
        const has_pending: bool = _.hasPendingOfFamily(HeAddressFamily.ipv6) || _.hasPendingOfFamily(HeAddressFamily.ipv4);
        if (has_pending) {
            // Respect RFC 8305 §3 Resolution Delay if waiting for preferred family before first attempt
            if (!_.has_started_any && _.resolution_timer_active) {
                if (eff_now < _.resolution_deadline_ms) {
                    return HeAction{
                        kind: HeActionKind.wait_until,
                        attempt_id: HE_INVALID_ATTEMPT_ID,
                        candidate: HeCandidate{},
                        deadline_ms: _.resolution_deadline_ms,
                    };
                }
                _.resolution_timer_active = false;
            }

            // Respect RFC 8305 §5 Connection Attempt Delay between staggered attempts
            if (_.has_started_any && eff_now < _.next_attempt_deadline_ms) {
                return HeAction{
                    kind: HeActionKind.wait_until,
                    attempt_id: HE_INVALID_ATTEMPT_ID,
                    candidate: HeCandidate{},
                    deadline_ms: _.next_attempt_deadline_ms,
                };
            }

            const slot: u16 = _.pickNextPendingSlot();
            if (slot != HE_INVALID_ATTEMPT_ID) {
                _.slots[slot as usize].state = HeAttemptState.in_flight;
                _.slots[slot as usize].started_at_ms = eff_now;
                _.has_started_any = true;
                _.next_attempt_deadline_ms = eff_now + _.cfg.connection_attempt_delay_ms;
                return HeAction{
                    kind: HeActionKind.start_attempt,
                    attempt_id: slot,
                    candidate: _.slots[slot as usize].candidate,
                    deadline_ms: _.next_attempt_deadline_ms,
                };
            }
        }

        // No pending candidates: check if any in-flight attempts remain or DNS is still pending
        let has_in_flight: bool = false;
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (_.slots[i].state == HeAttemptState.in_flight) {
                has_in_flight = true;
                break;
            }
        }

        if (has_in_flight || !_.dns_ipv6_done || !_.dns_ipv4_done) {
            return HeAction{
                kind: HeActionKind.wait_until,
                attempt_id: HE_INVALID_ATTEMPT_ID,
                candidate: HeCandidate{},
                deadline_ms: 0,
            };
        }

        return HeAction{
            kind: HeActionKind.all_failed,
            attempt_id: HE_INVALID_ATTEMPT_ID,
            candidate: HeCandidate{},
            deadline_ms: eff_now,
        };
    }

    /// Reports that `attempt_id` succeeded, electing it as the winner and flagging all other
    /// in-flight attempts as `cancel_pending` (and remaining pending attempts as `canceled`).
    fn markAttemptSucceeded(*_, attempt_id: u16): bool {
        if (_.winner_attempt_id != HE_INVALID_ATTEMPT_ID) return false;
        const idx: usize = attempt_id as usize;
        if (idx >= _.slot_count || !_.slots[idx].occupied) return false;
        if (_.slots[idx].state != HeAttemptState.in_flight) return false;

        _.slots[idx].state = HeAttemptState.succeeded;
        _.winner_attempt_id = attempt_id;

        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            if (i == idx) continue;
            if (_.slots[i].state == HeAttemptState.in_flight) {
                _.slots[i].state = HeAttemptState.cancel_pending;
            } else if (_.slots[i].state == HeAttemptState.pending) {
                _.slots[i].state = HeAttemptState.canceled;
            }
        }
        return true;
    }

    /// Reports that `attempt_id` failed at `now_ms`, immediately clearing the stagger timer
    /// so the next `nextAction(now_ms)` launches the next candidate without waiting out 250ms.
    fn markAttemptFailed(*_, attempt_id: u16, now_ms: u64): bool {
        const eff_now: u64 = _.effectiveNow(now_ms);
        const idx: usize = attempt_id as usize;
        if (idx >= _.slot_count || !_.slots[idx].occupied) return false;
        if (_.slots[idx].state != HeAttemptState.in_flight) return false;

        _.slots[idx].state = HeAttemptState.failed;
        // RFC 8305 §5: failure immediately triggers the next connection attempt without waiting.
        _.next_attempt_deadline_ms = eff_now;
        return true;
    }

    /// Collects all `cancel_pending` attempt IDs into `out_attempt_ids` and transitions them to `canceled`.
    /// Supports batched collection when `out_attempt_ids` is smaller than pending cancels.
    fn collectAttemptsToCancel(*_, out_attempt_ids: *[]u16): usize {
        if (out_attempt_ids.len == 0) return 0;
        let written: usize = 0;
        let i: usize = 0;
        while (i < _.slot_count && written < out_attempt_ids.len) : (i += 1) {
            if (_.slots[i].state == HeAttemptState.cancel_pending) {
                out_attempt_ids[written] = i as u16;
                _.slots[i].state = HeAttemptState.canceled;
                written += 1;
            }
        }
        return written;
    }

    /// Returns current operational statistics for the racing session.
    fn stats(): HeStats {
        let st = HeStats{
            total_candidates: _.slot_count,
            ipv6_candidates: _.ipv6_count,
            ipv4_candidates: _.ipv4_count,
            winner_attempt_id: _.winner_attempt_id,
            dns_ipv6_done: _.dns_ipv6_done,
            dns_ipv4_done: _.dns_ipv4_done,
        };
        let i: usize = 0;
        while (i < _.slot_count) : (i += 1) {
            const s = _.slots[i].state;
            if (s != HeAttemptState.pending && s != HeAttemptState.canceled) {
                st.started_attempts += 1;
            }
            if (s == HeAttemptState.in_flight) {
                st.in_flight_attempts += 1;
            } else if (s == HeAttemptState.failed) {
                st.failed_attempts += 1;
            } else if (s == HeAttemptState.cancel_pending) {
                st.cancel_pending_attempts += 1;
            }
        }
        return st;
    }
}

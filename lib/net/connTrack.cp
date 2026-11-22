// Origin: Pure-CP Stdlib (Zero-Alloc L4 Bidirectional 5-Tuple Conntrack Table & Half-Open Quota Guard) | Created: 2026-11-23
// 100% pure CP zero-heap-allocation L4 stateful connection tracking table (Conntrack):
//   - Fixed 128-slot table (`CONNTRACK_MAX_FLOWS = 128`) with normalized 16-byte IPv4/IPv6 addresses
//     and symmetric bidirectional 5-tuple hashing so original & reply packets hit the exact same slot
//   - Full TCP (`syn_sent | syn_recv | established | fin_wait | time_wait | closed`) & UDP
//     (`udp_unreplied | udp_assured`) state machines with per-state idle timeout sweeping (`reapExpired`)
//   - Half-open (`syn_sent` / `syn_recv`) quota enforcement for resource exhaustion protection,
//     bidirectional packet/byte counters, and full-table priority reclamation of `closed` / `time_wait` slots

/// Maximum number of tracked L4 flows stored inline in `ConnTrackTable`.
pub const CONNTRACK_MAX_FLOWS: usize = 128;

/// IANA IP protocol number for TCP.
pub const CONNTRACK_PROTO_TCP: u8 = 6;

/// IANA IP protocol number for UDP.
pub const CONNTRACK_PROTO_UDP: u8 = 17;

/// TCP header flag bits used to drive conntrack state transitions.
pub const CT_TCP_FIN: u8 = 0x01;
pub const CT_TCP_SYN: u8 = 0x02;
pub const CT_TCP_RST: u8 = 0x04;
pub const CT_TCP_ACK: u8 = 0x10;

// ============================================================================
// 1. Enums, 5-Tuple & Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// L4 connection tracking state for TCP and UDP flows.
pub enum ConnTrackState: u8 {
    syn_sent = 0,
    syn_recv = 1,
    established = 2,
    fin_wait = 3,
    time_wait = 4,
    udp_unreplied = 5,
    udp_assured = 6,
    closed = 7,
}

/// Packet direction relative to the flow's initiating (original) 5-tuple.
pub enum ConnTrackDir: u8 {
    original = 0,
    reply = 1,
}

/// Outcome classification returned by `ConnTrackTable.observePacket`.
pub enum ConnTrackAction: u8 {
    created = 0,
    updated = 1,
    rejected_half_open_quota = 2,
    rejected_table_full = 3,
    rejected_invalid = 4,
}

fn splitMix64(x_in: u64): u64 {
    let z: u64 = x_in +% 0x9E3779B97F4A7C15;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    return z ^ (z >> 31);
}

fn hashEndpoint(ip: [16]u8, port: u16): u64 {
    let h: u64 = 0xCBF29CE484222325;
    let idx: usize = 0;
    while (idx < 16) {
        h ^= @as(u64, ip[idx]);
        h = h *% 0x100000001B3;
        idx += 1;
    }
    h ^= @as(u64, port & 0xFF);
    h = h *% 0x100000001B3;
    h ^= @as(u64, (port >> 8) & 0xFF);
    h = h *% 0x100000001B3;
    return splitMix64(h);
}

fn ipEq16(a: [16]u8, b: [16]u8): bool {
    let idx: usize = 0;
    while (idx < 16) {
        if (a[idx] != b[idx]) {
            return false;
        }
        idx += 1;
    }
    return true;
}

/// Normalized IPv4/IPv6 L4 5-tuple (`IPv4` is stored as IPv4-mapped IPv6 `::ffff:a.b.c.d`).
pub struct ConnTrackTuple {
    proto: u8 = CONNTRACK_PROTO_TCP;
    src_ip: [16]u8 = [0; 16];
    dst_ip: [16]u8 = [0; 16];
    src_port: u16 = 0;
    dst_port: u16 = 0;

    /// Constructs a normalized 5-tuple from IPv4 addresses (mapped to `::ffff:a.b.c.d`).
    static fn fromIpv4(
        proto: u8,
        src_v4: [4]u8,
        src_port: u16,
        dst_v4: [4]u8,
        dst_port: u16,
    ): ConnTrackTuple {
        let s: [16]u8 = [0; 16];
        s[10] = 0xFF;
        s[11] = 0xFF;
        s[12] = src_v4[0];
        s[13] = src_v4[1];
        s[14] = src_v4[2];
        s[15] = src_v4[3];

        let d: [16]u8 = [0; 16];
        d[10] = 0xFF;
        d[11] = 0xFF;
        d[12] = dst_v4[0];
        d[13] = dst_v4[1];
        d[14] = dst_v4[2];
        d[15] = dst_v4[3];

        return ConnTrackTuple{
            proto: proto,
            src_ip: s,
            dst_ip: d,
            src_port: src_port,
            dst_port: dst_port,
        };
    }

    /// Constructs a normalized 5-tuple from 16-byte IPv6 addresses.
    static fn fromIpv6(
        proto: u8,
        src_v6: [16]u8,
        src_port: u16,
        dst_v6: [16]u8,
        dst_port: u16,
    ): ConnTrackTuple {
        return ConnTrackTuple{
            proto: proto,
            src_ip: src_v6,
            dst_ip: dst_v6,
            src_port: src_port,
            dst_port: dst_port,
        };
    }

    /// Returns the reversed (reply-direction) 5-tuple.
    fn reverse(): ConnTrackTuple {
        return ConnTrackTuple{
            proto: _.proto,
            src_ip: _.dst_ip,
            dst_ip: _.src_ip,
            src_port: _.dst_port,
            dst_port: _.src_port,
        };
    }

    /// Exact directional equality check.
    fn equals(other: ConnTrackTuple): bool {
        return _.proto == other.proto
            && _.src_port == other.src_port
            && _.dst_port == other.dst_port
            && ipEq16(_.src_ip, other.src_ip)
            && ipEq16(_.dst_ip, other.dst_ip);
    }

    /// Commutative bidirectional hash: `tuple.symmetricHash() == tuple.reverse().symmetricHash()`.
    fn symmetricHash(): u64 {
        const ha: u64 = hashEndpoint(_.src_ip, _.src_port);
        const hb: u64 = hashEndpoint(_.dst_ip, _.dst_port);
        const mixed: u64 = (ha ^ hb) ^ ((ha +% hb) *% 0x9E3779B97F4A7C15) ^ (@as(u64, _.proto) << 56);
        return splitMix64(mixed);
    }
}

/// Per-state idle timeouts and half-open quota configuration.
pub struct ConnTrackConfig {
    /// Maximum simultaneous half-open (`syn_sent` + `syn_recv`) TCP flows allowed.
    max_half_open: usize = 32;
    syn_timeout_ms: u64 = 10_000;
    established_timeout_ms: u64 = 300_000;
    fin_wait_timeout_ms: u64 = 30_000;
    time_wait_timeout_ms: u64 = 15_000;
    udp_unreplied_timeout_ms: u64 = 30_000;
    udp_assured_timeout_ms: u64 = 120_000;

    /// Returns production default conntrack timeouts and half-open quota.
    static fn defaults(): ConnTrackConfig {
        return ConnTrackConfig{};
    }

    /// Validates quota and timeout bounds.
    fn isValid(): bool {
        return _.max_half_open > 0
            && _.max_half_open <= CONNTRACK_MAX_FLOWS
            && _.syn_timeout_ms > 0
            && _.established_timeout_ms > 0
            && _.fin_wait_timeout_ms > 0
            && _.time_wait_timeout_ms > 0
            && _.udp_unreplied_timeout_ms > 0
            && _.udp_assured_timeout_ms > 0;
    }
}

/// Single tracked bidirectional L4 flow entry.
pub struct ConnTrackEntry {
    occupied: bool = false;
    flow_id: u64 = 0;
    sym_hash: u64 = 0;
    orig_tuple: ConnTrackTuple = ConnTrackTuple{};
    state: ConnTrackState = ConnTrackState.closed;
    orig_fin_seen: bool = false;
    reply_fin_seen: bool = false;
    orig_packets: u64 = 0;
    orig_bytes: u64 = 0;
    reply_packets: u64 = 0;
    reply_bytes: u64 = 0;
    created_ms: i64 = 0;
    last_seen_ms: i64 = 0;
}

/// Result returned by `ConnTrackTable.observePacket`.
pub struct ConnTrackOutcome {
    action: ConnTrackAction = ConnTrackAction.rejected_invalid;
    direction: ConnTrackDir = ConnTrackDir.original;
    state: ConnTrackState = ConnTrackState.closed;
    flow_id: u64 = 0;
    slot_idx: usize = 0;
    reclaimed_slot: bool = false;
}

/// Zero-heap global telemetry snapshot of `ConnTrackTable`.
pub struct ConnTrackStats {
    capacity: usize = CONNTRACK_MAX_FLOWS;
    active_flows: usize = 0;
    half_open_flows: usize = 0;
    total_created: u64 = 0;
    total_hits: u64 = 0;
    total_expired_reaped: u64 = 0;
    total_half_open_rejected: u64 = 0;
    total_table_full_dropped: u64 = 0;
    total_reclaimed_closed_tw: u64 = 0;
}

fn isHalfOpenState(st: ConnTrackState): bool {
    return st == ConnTrackState.syn_sent || st == ConnTrackState.syn_recv;
}

// ============================================================================
// 2. ConnTrackTable Core Engine
// ============================================================================

/// Zero-heap 128-slot L4 bidirectional connection tracking table with half-open quota protection.
pub struct ConnTrackTable {
    cfg: ConnTrackConfig = ConnTrackConfig.defaults();
    entries: [CONNTRACK_MAX_FLOWS]ConnTrackEntry = [ConnTrackEntry{}; CONNTRACK_MAX_FLOWS];
    active_flows: usize = 0;
    half_open_flows: usize = 0;
    next_flow_id: u64 = 1;
    total_created: u64 = 0;
    total_hits: u64 = 0;
    total_expired_reaped: u64 = 0;
    total_half_open_rejected: u64 = 0;
    total_table_full_dropped: u64 = 0;
    total_reclaimed_closed_tw: u64 = 0;

    /// Initializes an empty `ConnTrackTable` with validated `cfg`, or `null` if invalid.
    static fn init(cfg: ConnTrackConfig): ConnTrackTable? {
        if (!cfg.isValid()) {
            return null;
        }
        return ConnTrackTable{
            cfg: cfg,
        };
    }

    /// Clears all tracked flows and resets telemetry counters.
    fn reset(*_) {
        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            _.entries[idx] = ConnTrackEntry{};
            idx += 1;
        }
        _.active_flows = 0;
        _.half_open_flows = 0;
        _.next_flow_id = 1;
        _.total_created = 0;
        _.total_hits = 0;
        _.total_expired_reaped = 0;
        _.total_half_open_rejected = 0;
        _.total_table_full_dropped = 0;
        _.total_reclaimed_closed_tw = 0;
    }

    fn timeoutForState(st: ConnTrackState): u64 {
        if (st == ConnTrackState.syn_sent || st == ConnTrackState.syn_recv) {
            return _.cfg.syn_timeout_ms;
        }
        if (st == ConnTrackState.established) {
            return _.cfg.established_timeout_ms;
        }
        if (st == ConnTrackState.fin_wait) {
            return _.cfg.fin_wait_timeout_ms;
        }
        if (st == ConnTrackState.time_wait || st == ConnTrackState.closed) {
            return _.cfg.time_wait_timeout_ms;
        }
        if (st == ConnTrackState.udp_unreplied) {
            return _.cfg.udp_unreplied_timeout_ms;
        }
        return _.cfg.udp_assured_timeout_ms;
    }

    fn setSlotState(*_, slot_idx: usize, new_state: ConnTrackState) {
        const old_half: bool = isHalfOpenState(_.entries[slot_idx].state);
        const new_half: bool = isHalfOpenState(new_state);
        _.entries[slot_idx].state = new_state;
        if (!old_half && new_half) {
            _.half_open_flows += 1;
        } else if (old_half && !new_half && _.half_open_flows > 0) {
            _.half_open_flows -= 1;
        }
    }

    /// Read-only bidirectional lookup: matches either original or reply 5-tuple.
    fn lookup(tuple: ConnTrackTuple): ConnTrackEntry? {
        const h: u64 = tuple.symmetricHash();
        const rev = tuple.reverse();
        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            if (_.entries[idx].occupied && _.entries[idx].sym_hash == h) {
                if (_.entries[idx].orig_tuple.equals(tuple) || _.entries[idx].orig_tuple.equals(rev)) {
                    return _.entries[idx];
                }
            }
            idx += 1;
        }
        return null;
    }

    /// Removes a tracked flow by `flow_id`. Returns `true` if found and removed.
    fn removeFlow(*_, flow_id: u64): bool {
        if (flow_id == 0) {
            return false;
        }
        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            if (_.entries[idx].occupied && _.entries[idx].flow_id == flow_id) {
                if (isHalfOpenState(_.entries[idx].state) && _.half_open_flows > 0) {
                    _.half_open_flows -= 1;
                }
                _.entries[idx] = ConnTrackEntry{};
                if (_.active_flows > 0) {
                    _.active_flows -= 1;
                }
                return true;
            }
            idx += 1;
        }
        return false;
    }

    /// Sweeps all flows whose idle time `now_ms - last_seen_ms` exceeds their per-state timeout.
    fn reapExpired(*_, now_ms: i64): usize {
        let reaped: usize = 0;
        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            if (_.entries[idx].occupied && now_ms > _.entries[idx].last_seen_ms) {
                const idle_ms: u64 = @as(u64, @intCast(now_ms - _.entries[idx].last_seen_ms));
                const limit_ms: u64 = _.timeoutForState(_.entries[idx].state);
                if (idle_ms >= limit_ms) {
                    if (isHalfOpenState(_.entries[idx].state) && _.half_open_flows > 0) {
                        _.half_open_flows -= 1;
                    }
                    _.entries[idx] = ConnTrackEntry{};
                    if (_.active_flows > 0) {
                        _.active_flows -= 1;
                    }
                    reaped += 1;
                }
            }
            idx += 1;
        }
        _.total_expired_reaped += @as(u64, reaped);
        return reaped;
    }

    fn findReclaimableSlot(): usize? {
        // Priority 1: oldest `closed` slot; Priority 2: oldest `time_wait` slot.
        let closed_found: bool = false;
        let closed_idx: usize = 0;
        let tw_found: bool = false;
        let tw_idx: usize = 0;

        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            if (_.entries[idx].occupied) {
                const st = _.entries[idx].state;
                if (st == ConnTrackState.closed) {
                    if (!closed_found || _.entries[idx].last_seen_ms < _.entries[closed_idx].last_seen_ms) {
                        closed_found = true;
                        closed_idx = idx;
                    }
                } else if (st == ConnTrackState.time_wait) {
                    if (!tw_found || _.entries[idx].last_seen_ms < _.entries[tw_idx].last_seen_ms) {
                        tw_found = true;
                        tw_idx = idx;
                    }
                }
            }
            idx += 1;
        }

        if (closed_found) {
            return closed_idx;
        }
        if (tw_found) {
            return tw_idx;
        }
        return null;
    }

    /// Processes an observed L4 packet, matching existing bidirectional flows or creating a new flow
    /// under half-open quota and full-table reclamation policies.
    fn observePacket(
        *_,
        tuple: ConnTrackTuple,
        tcp_flags: u8,
        packet_bytes: u32,
        now_ms: i64,
    ): ConnTrackOutcome {
        if (tuple.proto != CONNTRACK_PROTO_TCP && tuple.proto != CONNTRACK_PROTO_UDP) {
            return ConnTrackOutcome{
                action: ConnTrackAction.rejected_invalid,
            };
        }

        const h: u64 = tuple.symmetricHash();
        const rev = tuple.reverse();

        // 1. Search existing bidirectional flow.
        let idx: usize = 0;
        while (idx < CONNTRACK_MAX_FLOWS) {
            if (_.entries[idx].occupied && _.entries[idx].sym_hash == h) {
                const is_orig: bool = _.entries[idx].orig_tuple.equals(tuple);
                const is_reply: bool = if (is_orig) false else _.entries[idx].orig_tuple.equals(rev);
                if (is_orig || is_reply) {
                    const dir: ConnTrackDir = if (is_orig) ConnTrackDir.original else ConnTrackDir.reply;
                    if (is_orig) {
                        _.entries[idx].orig_packets += 1;
                        _.entries[idx].orig_bytes += @as(u64, packet_bytes);
                    } else {
                        _.entries[idx].reply_packets += 1;
                        _.entries[idx].reply_bytes += @as(u64, packet_bytes);
                    }
                    if (now_ms > _.entries[idx].last_seen_ms) {
                        _.entries[idx].last_seen_ms = now_ms;
                    }

                    // Advance protocol state machine.
                    if (tuple.proto == CONNTRACK_PROTO_TCP) {
                        const has_rst: bool = (tcp_flags & CT_TCP_RST) != 0;
                        const has_fin: bool = (tcp_flags & CT_TCP_FIN) != 0;
                        const has_syn: bool = (tcp_flags & CT_TCP_SYN) != 0;
                        const has_ack: bool = (tcp_flags & CT_TCP_ACK) != 0;

                        if (has_rst) {
                            _.setSlotState(idx, ConnTrackState.closed);
                        } else if (has_fin) {
                            if (is_orig) {
                                _.entries[idx].orig_fin_seen = true;
                            } else {
                                _.entries[idx].reply_fin_seen = true;
                            }
                            if (_.entries[idx].orig_fin_seen && _.entries[idx].reply_fin_seen) {
                                _.setSlotState(idx, ConnTrackState.time_wait);
                            } else {
                                _.setSlotState(idx, ConnTrackState.fin_wait);
                            }
                        } else {
                            const cur_st = _.entries[idx].state;
                            if (cur_st == ConnTrackState.syn_sent && is_reply && has_syn && has_ack) {
                                _.setSlotState(idx, ConnTrackState.syn_recv);
                            } else if (cur_st == ConnTrackState.syn_recv && is_orig && has_ack && !has_syn) {
                                _.setSlotState(idx, ConnTrackState.established);
                            }
                        }
                    } else {
                        // UDP: first reply promotes `udp_unreplied` -> `udp_assured`.
                        if (is_reply && _.entries[idx].state == ConnTrackState.udp_unreplied) {
                            _.setSlotState(idx, ConnTrackState.udp_assured);
                        }
                    }

                    _.total_hits += 1;
                    return ConnTrackOutcome{
                        action: ConnTrackAction.updated,
                        direction: dir,
                        state: _.entries[idx].state,
                        flow_id: _.entries[idx].flow_id,
                        slot_idx: idx,
                        reclaimed_slot: false,
                    };
                }
            }
            idx += 1;
        }

        // 2. New flow: determine initial state.
        let init_state: ConnTrackState = ConnTrackState.udp_unreplied;
        if (tuple.proto == CONNTRACK_PROTO_TCP) {
            if ((tcp_flags & CT_TCP_RST) != 0) {
                init_state = ConnTrackState.closed;
            } else if ((tcp_flags & CT_TCP_SYN) != 0 && (tcp_flags & CT_TCP_ACK) == 0) {
                init_state = ConnTrackState.syn_sent;
            } else if ((tcp_flags & CT_TCP_SYN) != 0 && (tcp_flags & CT_TCP_ACK) != 0) {
                init_state = ConnTrackState.syn_recv;
            } else {
                init_state = ConnTrackState.established;
            }
        }

        // Enforce half-open quota guard before allocating slot.
        if (isHalfOpenState(init_state) && _.half_open_flows >= _.cfg.max_half_open) {
            _.total_half_open_rejected += 1;
            return ConnTrackOutcome{
                action: ConnTrackAction.rejected_half_open_quota,
                direction: ConnTrackDir.original,
                state: init_state,
            };
        }

        // Locate free slot, or reclaim oldest `closed` / `time_wait` slot if table is full.
        let target_idx: usize = CONNTRACK_MAX_FLOWS;
        let reclaimed: bool = false;
        let scan: usize = 0;
        while (scan < CONNTRACK_MAX_FLOWS) {
            if (!_.entries[scan].occupied) {
                target_idx = scan;
                break;
            }
            scan += 1;
        }

        if (target_idx >= CONNTRACK_MAX_FLOWS) {
            if (rec_idx := _.findReclaimableSlot()) {
                target_idx = rec_idx;
                reclaimed = true;
                _.total_reclaimed_closed_tw += 1;
            } else {
                _.total_table_full_dropped += 1;
                return ConnTrackOutcome{
                    action: ConnTrackAction.rejected_table_full,
                    direction: ConnTrackDir.original,
                    state: init_state,
                };
            }
        } else {
            _.active_flows += 1;
        }

        const fid: u64 = _.next_flow_id;
        _.next_flow_id += 1;

        _.entries[target_idx] = ConnTrackEntry{
            occupied: true,
            flow_id: fid,
            sym_hash: h,
            orig_tuple: tuple,
            state: init_state,
            orig_packets: 1,
            orig_bytes: @as(u64, packet_bytes),
            reply_packets: 0,
            reply_bytes: 0,
            created_ms: now_ms,
            last_seen_ms: now_ms,
        };
        if (isHalfOpenState(init_state)) {
            _.half_open_flows += 1;
        }
        _.total_created += 1;

        return ConnTrackOutcome{
            action: ConnTrackAction.created,
            direction: ConnTrackDir.original,
            state: init_state,
            flow_id: fid,
            slot_idx: target_idx,
            reclaimed_slot: reclaimed,
        };
    }

    /// Returns a zero-heap global telemetry snapshot of the connection tracking table.
    fn stats(): ConnTrackStats {
        return ConnTrackStats{
            capacity: CONNTRACK_MAX_FLOWS,
            active_flows: _.active_flows,
            half_open_flows: _.half_open_flows,
            total_created: _.total_created,
            total_hits: _.total_hits,
            total_expired_reaped: _.total_expired_reaped,
            total_half_open_rejected: _.total_half_open_rejected,
            total_table_full_dropped: _.total_table_full_dropped,
            total_reclaimed_closed_tw: _.total_reclaimed_closed_tw,
        };
    }
}

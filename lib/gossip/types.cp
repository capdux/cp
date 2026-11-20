// Source: cp/lib/gossip/types.cp (SWIM member states, incarnation conflict resolution, config & errors)

const std = @import("std");

pub const GossipError = error {
    OutOfMemory,
    InvalidMessage,
    TruncatedMessage,
    UnknownTag,
    InvalidNodeId,
    BufferTooSmall,
};

pub enum MemberState {
    Active = 0,
    Suspect = 1,
    Offline = 2,
    Left = 3,

    pub fn priority() -> u8 {
        switch (_) {
            .Active => return 0,
            .Suspect => return 1,
            .Offline => return 2,
            .Left => return 3,
        }
    }

    pub fn fromU8(v: u8) -> GossipError!MemberState {
        switch (v) {
            0 => return MemberState.Active,
            1 => return MemberState.Suspect,
            2 => return MemberState.Offline,
            3 => return MemberState.Left,
            else => return GossipError.UnknownTag,
        }
    }
}

pub struct MemberInfo {
    id: string,
    addr: string,
    state: MemberState,
    incarnation: u64,
    state_changed_ms: u64,
}

pub struct GossipUpdate {
    node_id: string,
    addr: string,
    state: MemberState,
    incarnation: u64,
}

pub struct SwimConfig {
    protocol_period_ms: u64,
    ack_timeout_ms: u64,
    suspect_timeout_ms: u64,
    indirect_ping_count: usize,
    max_piggyback: usize,
    retransmit_mult: u32,

    pub fn default() -> SwimConfig {
        return SwimConfig{
            .protocol_period_ms = 1000,
            .ack_timeout_ms = 500,
            .suspect_timeout_ms = 3000,
            .indirect_ping_count = 3,
            .max_piggyback = 8,
            .retransmit_mult = 4,
        };
    }
}

/// SWIM version precedence:
/// 1. Higher incarnation always wins.
/// 2. At equal incarnation: Left > Offline > Suspect > Active.
pub fn shouldSupersede(
    existing_state: MemberState,
    existing_inc: u64,
    new_state: MemberState,
    new_inc: u64,
) -> bool {
    if (new_inc > existing_inc) {
        return true;
    }
    if (new_inc < existing_inc) {
        return false;
    }
    return new_state.priority() > existing_state.priority();
}

/// Compute SWIM retransmit limit: lambda * ceil(log2(N + 1)), minimum 1.
pub fn retransmitLimit(cluster_size: usize, mult: u32) -> u32 {
    const n: usize = if (cluster_size < 1) 1 else cluster_size;
    let bits: u32 = 0;
    let v: usize = n;
    while (v > 0) {
        bits += 1;
        v = v >> 1;
    }
    const m: u32 = if (mult == 0) 1 else mult;
    const total: u32 = m * bits;
    return if (total == 0) 1 else total;
}

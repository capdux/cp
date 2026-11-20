// Source: cp/lib/gossip/node.cp (Sans-IO SWIM cluster membership & health probe engine)

const std = @import("std");
import {
    GossipError,
    MemberState,
    MemberInfo,
    GossipUpdate,
    SwimConfig,
    shouldSupersede,
    retransmitLimit,
} from "./types.cp";
import { MessageKind, GossipMessage, OutboundMessage, freeMessage } from "./wire.cp";

struct OwnedMember {
    id: []u8,
    addr: []u8,
    state: MemberState,
    incarnation: u64,
    state_changed_ms: u64,
}

struct OwnedUpdate {
    node_id: []u8,
    addr: []u8,
    state: MemberState,
    incarnation: u64,
    tx_count: u32,
}

struct PendingPing {
    seq: u64,
    target_id: []u8,
    target_addr: []u8,
    sent_ms: u64,
    indirect_sent: bool,
    is_relay: bool,
    relay_for_id: []u8,
    relay_for_addr: []u8,
    relay_seq: u64,
}

pub struct SwimClusterNode {
    alloc: std.mem.Allocator,
    self_id: []u8,
    self_addr: []u8,
    incarnation: u64,
    config: SwimConfig,
    members: std.ArrayList(OwnedMember),
    piggyback: std.ArrayList(OwnedUpdate),
    pending_pings: std.ArrayList(PendingPing),
    next_seq: u64,
    probe_cursor: usize,
    last_probe_ms: u64,

    pub fn init(
        alloc: std.mem.Allocator,
        self_id: string,
        addr: string,
        config: SwimConfig,
    ) -> GossipError!SwimClusterNode {
        if (self_id.len == 0) {
            return GossipError.InvalidNodeId;
        }
        const sid = alloc.dupe(u8, self_id) catch return GossipError.OutOfMemory;
        errdefer alloc.free(sid);
        const saddr = alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
        errdefer alloc.free(saddr);

        let members = std.ArrayList(OwnedMember).empty;
        errdefer {
            for (m of members.items) {
                alloc.free(m.id);
                alloc.free(m.addr);
            }
            members.deinit(alloc);
        }

        const mid = alloc.dupe(u8, self_id) catch return GossipError.OutOfMemory;
        errdefer alloc.free(mid);
        const maddr = alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
        errdefer alloc.free(maddr);

        members.append(alloc, OwnedMember{
            .id = mid,
            .addr = maddr,
            .state = MemberState.Active,
            .incarnation = 0,
            .state_changed_ms = 0,
        }) catch return GossipError.OutOfMemory;

        return SwimClusterNode{
            .alloc = alloc,
            .self_id = sid,
            .self_addr = saddr,
            .incarnation = 0,
            .config = config,
            .members = members,
            .piggyback = std.ArrayList(OwnedUpdate).empty,
            .pending_pings = std.ArrayList(PendingPing).empty,
            .next_seq = 1,
            .probe_cursor = 0,
            .last_probe_ms = 0,
        };
    }

    pub fn deinit(*_) {
        _.alloc.free(_.self_id);
        _.alloc.free(_.self_addr);
        for (m of _.members.items) {
            _.alloc.free(m.id);
            _.alloc.free(m.addr);
        }
        _.members.deinit(_.alloc);

        for (u of _.piggyback.items) {
            _.alloc.free(u.node_id);
            _.alloc.free(u.addr);
        }
        _.piggyback.deinit(_.alloc);

        for (p of _.pending_pings.items) {
            _.alloc.free(p.target_id);
            _.alloc.free(p.target_addr);
            _.alloc.free(p.relay_for_id);
            _.alloc.free(p.relay_for_addr);
        }
        _.pending_pings.deinit(_.alloc);
    }

    fn findMemberIndex(id: string) -> usize? {
        let i: usize = 0;
        while (i < _.members.items.len) {
            if (std.mem.eql(u8, _.members.items[i].id, id)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn enqueuePiggyback(
        *_,
        node_id: string,
        addr: string,
        state: MemberState,
        incarnation: u64,
    ) -> GossipError!void {
        let i: usize = 0;
        while (i < _.piggyback.items.len) {
            if (std.mem.eql(u8, _.piggyback.items[i].node_id, node_id)) {
                if (shouldSupersede(
                    _.piggyback.items[i].state,
                    _.piggyback.items[i].incarnation,
                    state,
                    incarnation,
                )) {
                    const new_addr = _.alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
                    _.alloc.free(_.piggyback.items[i].addr);
                    _.piggyback.items[i].addr = new_addr;
                    _.piggyback.items[i].state = state;
                    _.piggyback.items[i].incarnation = incarnation;
                    _.piggyback.items[i].tx_count = 0;
                }
                return;
            }
            i += 1;
        }

        const nid = _.alloc.dupe(u8, node_id) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(nid);
        const naddr = _.alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(naddr);

        _.piggyback.append(_.alloc, OwnedUpdate{
            .node_id = nid,
            .addr = naddr,
            .state = state,
            .incarnation = incarnation,
            .tx_count = 0,
        }) catch return GossipError.OutOfMemory;
    }

    fn takePiggybackUpdates(*_) -> GossipError![]GossipUpdate {
        const limit = retransmitLimit(_.members.items.len, _.config.retransmit_mult);
        let out = std.ArrayList(GossipUpdate).empty;
        errdefer {
            for (u of out.items) {
                _.alloc.free(u.node_id);
                _.alloc.free(u.addr);
            }
            out.deinit(_.alloc);
        }

        const max_take = _.config.max_piggyback;
        let i: usize = 0;
        while (i < _.piggyback.items.len and out.items.len < max_take) {
            const nid = _.alloc.dupe(u8, _.piggyback.items[i].node_id) catch return GossipError.OutOfMemory;
            errdefer _.alloc.free(nid);
            const naddr = _.alloc.dupe(u8, _.piggyback.items[i].addr) catch return GossipError.OutOfMemory;
            errdefer _.alloc.free(naddr);

            out.append(_.alloc, GossipUpdate{
                .node_id = nid,
                .addr = naddr,
                .state = _.piggyback.items[i].state,
                .incarnation = _.piggyback.items[i].incarnation,
            }) catch return GossipError.OutOfMemory;

            _.piggyback.items[i].tx_count += 1;
            if (_.piggyback.items[i].tx_count >= limit) {
                const removed = _.piggyback.orderedRemove(i);
                _.alloc.free(removed.node_id);
                _.alloc.free(removed.addr);
            } else {
                i += 1;
            }
        }

        return out.toOwnedSlice(_.alloc) catch return GossipError.OutOfMemory;
    }

    fn takeAllMembersAsUpdates(*_) -> GossipError![]GossipUpdate {
        let out = std.ArrayList(GossipUpdate).empty;
        errdefer {
            for (u of out.items) {
                _.alloc.free(u.node_id);
                _.alloc.free(u.addr);
            }
            out.deinit(_.alloc);
        }

        for (m of _.members.items) {
            const nid = _.alloc.dupe(u8, m.id) catch return GossipError.OutOfMemory;
            errdefer _.alloc.free(nid);
            const naddr = _.alloc.dupe(u8, m.addr) catch return GossipError.OutOfMemory;
            errdefer _.alloc.free(naddr);

            out.append(_.alloc, GossipUpdate{
                .node_id = nid,
                .addr = naddr,
                .state = m.state,
                .incarnation = m.incarnation,
            }) catch return GossipError.OutOfMemory;
        }

        return out.toOwnedSlice(_.alloc) catch return GossipError.OutOfMemory;
    }

    fn buildOutbound(
        kind: MessageKind,
        seq: u64,
        msg_target_id: string,
        msg_target_addr: string,
        dest_id: string,
        dest_addr: string,
        updates: []GossipUpdate,
    ) -> GossipError!OutboundMessage {
        const d_id = _.alloc.dupe(u8, dest_id) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(d_id);
        const d_addr = _.alloc.dupe(u8, dest_addr) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(d_addr);
        const s_id = _.alloc.dupe(u8, _.self_id) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(s_id);
        const s_addr = _.alloc.dupe(u8, _.self_addr) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(s_addr);
        const t_id = _.alloc.dupe(u8, msg_target_id) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(t_id);
        const t_addr = _.alloc.dupe(u8, msg_target_addr) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(t_addr);

        return OutboundMessage{
            .target_id = d_id,
            .target_addr = d_addr,
            .msg = GossipMessage{
                .kind = kind,
                .seq = seq,
                .sender_id = s_id,
                .sender_addr = s_addr,
                .target_id = t_id,
                .target_addr = t_addr,
                .updates = updates,
            },
        };
    }

    fn upsertMember(
        *_,
        node_id: string,
        addr: string,
        state: MemberState,
        incarnation: u64,
        now_ms: u64,
    ) -> GossipError!void {
        if (std.mem.eql(u8, node_id, _.self_id)) {
            if (state == MemberState.Suspect or state == MemberState.Offline) {
                if (incarnation >= _.incarnation) {
                    _.incarnation = incarnation + 1;
                } else {
                    _.incarnation += 1;
                }
                if (idx := _.findMemberIndex(_.self_id)) {
                    _.members.items[idx].state = MemberState.Active;
                    _.members.items[idx].incarnation = _.incarnation;
                    _.members.items[idx].state_changed_ms = now_ms;
                }
                try _.enqueuePiggyback(_.self_id, _.self_addr, MemberState.Active, _.incarnation);
            }
            return;
        }

        if (idx := _.findMemberIndex(node_id)) {
            const cur = _.members.items[idx];
            if (shouldSupersede(cur.state, cur.incarnation, state, incarnation)) {
                if (!std.mem.eql(u8, cur.addr, addr) and addr.len > 0) {
                    const new_addr = _.alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
                    _.alloc.free(_.members.items[idx].addr);
                    _.members.items[idx].addr = new_addr;
                }
                _.members.items[idx].state = state;
                _.members.items[idx].incarnation = incarnation;
                _.members.items[idx].state_changed_ms = now_ms;
                try _.enqueuePiggyback(node_id, _.members.items[idx].addr, state, incarnation);
            }
            return;
        }

        const nid = _.alloc.dupe(u8, node_id) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(nid);
        const naddr = _.alloc.dupe(u8, addr) catch return GossipError.OutOfMemory;
        errdefer _.alloc.free(naddr);

        _.members.append(_.alloc, OwnedMember{
            .id = nid,
            .addr = naddr,
            .state = state,
            .incarnation = incarnation,
            .state_changed_ms = now_ms,
        }) catch return GossipError.OutOfMemory;
        try _.enqueuePiggyback(node_id, addr, state, incarnation);
    }

    pub fn joinSeed(
        *_,
        peer_id: string,
        addr: string,
        now_ms: u64,
    ) -> GossipError![]OutboundMessage {
        try _.upsertMember(peer_id, addr, MemberState.Active, 0, now_ms);
        try _.enqueuePiggyback(_.self_id, _.self_addr, MemberState.Active, _.incarnation);

        let out = std.ArrayList(OutboundMessage).empty;
        errdefer freeOutboundSlice(_.alloc, out.items);

        const updates = try _.takeAllMembersAsUpdates();
        const seq = _.next_seq;
        _.next_seq += 1;

        const ob = try _.buildOutbound(
            MessageKind.Sync,
            seq,
            peer_id,
            addr,
            peer_id,
            addr,
            updates,
        );
        out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
        return out.toOwnedSlice(_.alloc) catch return GossipError.OutOfMemory;
    }

    pub fn leave(*_, now_ms: u64) -> GossipError!void {
        _.incarnation += 1;
        if (idx := _.findMemberIndex(_.self_id)) {
            _.members.items[idx].state = MemberState.Left;
            _.members.items[idx].incarnation = _.incarnation;
            _.members.items[idx].state_changed_ms = now_ms;
        }
        try _.enqueuePiggyback(_.self_id, _.self_addr, MemberState.Left, _.incarnation);
    }

    pub fn tick(*_, now_ms: u64) -> GossipError![]OutboundMessage {
        let out = std.ArrayList(OutboundMessage).empty;
        errdefer freeOutboundSlice(_.alloc, out.items);

        // 1. Check pending pings for ack timeout / indirect PingReq / Suspect transition
        let pi: usize = 0;
        while (pi < _.pending_pings.items.len) {
            const p = _.pending_pings.items[pi];
            if (p.is_relay) {
                if (now_ms >= p.sent_ms + _.config.ack_timeout_ms) {
                    const removed = _.pending_pings.orderedRemove(pi);
                    _.alloc.free(removed.target_id);
                    _.alloc.free(removed.target_addr);
                    _.alloc.free(removed.relay_for_id);
                    _.alloc.free(removed.relay_for_addr);
                } else {
                    pi += 1;
                }
                continue;
            }

            if (!p.indirect_sent and now_ms >= p.sent_ms + _.config.ack_timeout_ms) {
                _.pending_pings.items[pi].indirect_sent = true;
                let sent_indirect: usize = 0;
                for (m of _.members.items) {
                    if (sent_indirect >= _.config.indirect_ping_count) {
                        break;
                    }
                    if (std.mem.eql(u8, m.id, _.self_id) or std.mem.eql(u8, m.id, p.target_id)) {
                        continue;
                    }
                    if (m.state != MemberState.Active) {
                        continue;
                    }
                    const updates = try _.takePiggybackUpdates();
                    const ob = try _.buildOutbound(
                        MessageKind.PingReq,
                        p.seq,
                        p.target_id,
                        p.target_addr,
                        m.id,
                        m.addr,
                        updates,
                    );
                    out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
                    sent_indirect += 1;
                }
                pi += 1;
            } else if (p.indirect_sent and now_ms >= p.sent_ms + _.config.ack_timeout_ms * 2) {
                if (midx := _.findMemberIndex(p.target_id)) {
                    const cur = _.members.items[midx];
                    if (cur.state == MemberState.Active) {
                        try _.upsertMember(cur.id, cur.addr, MemberState.Suspect, cur.incarnation, now_ms);
                    }
                }
                const removed = _.pending_pings.orderedRemove(pi);
                _.alloc.free(removed.target_id);
                _.alloc.free(removed.target_addr);
                _.alloc.free(removed.relay_for_id);
                _.alloc.free(removed.relay_for_addr);
            } else {
                pi += 1;
            }
        }

        // 2. Check Suspect -> Offline timeouts
        let mi: usize = 0;
        while (mi < _.members.items.len) {
            const m = _.members.items[mi];
            if (!std.mem.eql(u8, m.id, _.self_id) and m.state == MemberState.Suspect) {
                if (now_ms >= m.state_changed_ms + _.config.suspect_timeout_ms) {
                    try _.upsertMember(m.id, m.addr, MemberState.Offline, m.incarnation, now_ms);
                }
            }
            mi += 1;
        }

        // 3. Periodic Ping probe
        if (now_ms >= _.last_probe_ms + _.config.protocol_period_ms) {
            _.last_probe_ms = now_ms;
            if (_.members.items.len > 1) {
                let checked: usize = 0;
                while (checked < _.members.items.len) {
                    const idx = _.probe_cursor % _.members.items.len;
                    _.probe_cursor = (_.probe_cursor + 1) % _.members.items.len;
                    checked += 1;

                    const target = _.members.items[idx];
                    if (std.mem.eql(u8, target.id, _.self_id)) {
                        continue;
                    }
                    if (target.state == MemberState.Offline or target.state == MemberState.Left) {
                        continue;
                    }

                    const seq = _.next_seq;
                    _.next_seq += 1;

                    const tid = _.alloc.dupe(u8, target.id) catch return GossipError.OutOfMemory;
                    errdefer _.alloc.free(tid);
                    const taddr = _.alloc.dupe(u8, target.addr) catch return GossipError.OutOfMemory;
                    errdefer _.alloc.free(taddr);
                    const rid = _.alloc.dupe(u8, "") catch return GossipError.OutOfMemory;
                    errdefer _.alloc.free(rid);
                    const raddr = _.alloc.dupe(u8, "") catch return GossipError.OutOfMemory;
                    errdefer _.alloc.free(raddr);

                    _.pending_pings.append(_.alloc, PendingPing{
                        .seq = seq,
                        .target_id = tid,
                        .target_addr = taddr,
                        .sent_ms = now_ms,
                        .indirect_sent = false,
                        .is_relay = false,
                        .relay_for_id = rid,
                        .relay_for_addr = raddr,
                        .relay_seq = 0,
                    }) catch return GossipError.OutOfMemory;

                    const updates = try _.takePiggybackUpdates();
                    const ob = try _.buildOutbound(
                        MessageKind.Ping,
                        seq,
                        target.id,
                        target.addr,
                        target.id,
                        target.addr,
                        updates,
                    );
                    out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
                    break;
                }
            }
        }

        return out.toOwnedSlice(_.alloc) catch return GossipError.OutOfMemory;
    }

    pub fn handleMessage(
        *_,
        from_id: string,
        msg: GossipMessage,
        now_ms: u64,
    ) -> GossipError![]OutboundMessage {
        // 1. Apply piggybacked updates
        for (u of msg.updates) {
            try _.upsertMember(u.node_id, u.addr, u.state, u.incarnation, now_ms);
        }

        // Ensure sender is known if not self
        if (from_id.len > 0 and !std.mem.eql(u8, from_id, _.self_id)) {
            if (_.findMemberIndex(from_id) == null) {
                try _.upsertMember(from_id, msg.sender_addr, MemberState.Active, 0, now_ms);
            }
        }

        let out = std.ArrayList(OutboundMessage).empty;
        errdefer freeOutboundSlice(_.alloc, out.items);

        switch (msg.kind) {
            .Ping => {
                const updates = try _.takePiggybackUpdates();
                const ob = try _.buildOutbound(
                    MessageKind.Ack,
                    msg.seq,
                    msg.sender_id,
                    msg.sender_addr,
                    msg.sender_id,
                    msg.sender_addr,
                    updates,
                );
                out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
            },
            .Ack => {
                let i: usize = 0;
                while (i < _.pending_pings.items.len) {
                    const p = _.pending_pings.items[i];
                    if (p.seq == msg.seq or std.mem.eql(u8, p.target_id, msg.sender_id)) {
                        if (p.is_relay) {
                            const updates = try _.takePiggybackUpdates();
                            const ob = try _.buildOutbound(
                                MessageKind.Ack,
                                p.relay_seq,
                                p.relay_for_id,
                                p.relay_for_addr,
                                p.relay_for_id,
                                p.relay_for_addr,
                                updates,
                            );
                            out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
                        }
                        const removed = _.pending_pings.orderedRemove(i);
                        _.alloc.free(removed.target_id);
                        _.alloc.free(removed.target_addr);
                        _.alloc.free(removed.relay_for_id);
                        _.alloc.free(removed.relay_for_addr);
                    } else {
                        i += 1;
                    }
                }
            },
            .PingReq => {
                const relay_seq = _.next_seq;
                _.next_seq += 1;

                const tid = _.alloc.dupe(u8, msg.target_id) catch return GossipError.OutOfMemory;
                errdefer _.alloc.free(tid);
                const taddr = _.alloc.dupe(u8, msg.target_addr) catch return GossipError.OutOfMemory;
                errdefer _.alloc.free(taddr);
                const rid = _.alloc.dupe(u8, msg.sender_id) catch return GossipError.OutOfMemory;
                errdefer _.alloc.free(rid);
                const raddr = _.alloc.dupe(u8, msg.sender_addr) catch return GossipError.OutOfMemory;
                errdefer _.alloc.free(raddr);

                _.pending_pings.append(_.alloc, PendingPing{
                    .seq = relay_seq,
                    .target_id = tid,
                    .target_addr = taddr,
                    .sent_ms = now_ms,
                    .indirect_sent = false,
                    .is_relay = true,
                    .relay_for_id = rid,
                    .relay_for_addr = raddr,
                    .relay_seq = msg.seq,
                }) catch return GossipError.OutOfMemory;

                const updates = try _.takePiggybackUpdates();
                const ob = try _.buildOutbound(
                    MessageKind.Ping,
                    relay_seq,
                    msg.target_id,
                    msg.target_addr,
                    msg.target_id,
                    msg.target_addr,
                    updates,
                );
                out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
            },
            .Sync => {
                const updates = try _.takeAllMembersAsUpdates();
                const ob = try _.buildOutbound(
                    MessageKind.Ack,
                    msg.seq,
                    msg.sender_id,
                    msg.sender_addr,
                    msg.sender_id,
                    msg.sender_addr,
                    updates,
                );
                out.append(_.alloc, ob) catch return GossipError.OutOfMemory;
            },
        }

        return out.toOwnedSlice(_.alloc) catch return GossipError.OutOfMemory;
    }

    pub fn activeMembers(alloc: std.mem.Allocator) -> GossipError![]MemberInfo {
        let out = std.ArrayList(MemberInfo).empty;
        errdefer out.deinit(alloc);

        for (m of _.members.items) {
            if (m.state == MemberState.Active) {
                out.append(alloc, MemberInfo{
                    .id = m.id,
                    .addr = m.addr,
                    .state = m.state,
                    .incarnation = m.incarnation,
                    .state_changed_ms = m.state_changed_ms,
                }) catch return GossipError.OutOfMemory;
            }
        }
        return out.toOwnedSlice(alloc) catch return GossipError.OutOfMemory;
    }

    pub fn getMember(id: string) -> MemberInfo? {
        if (idx := _.findMemberIndex(id)) {
            const m = _.members.items[idx];
            return MemberInfo{
                .id = m.id,
                .addr = m.addr,
                .state = m.state,
                .incarnation = m.incarnation,
                .state_changed_ms = m.state_changed_ms,
            };
        }
        return null;
    }

    pub fn memberCount() -> usize {
        return _.members.items.len;
    }

    pub fn selfIncarnation() -> u64 {
        return _.incarnation;
    }
}

fn freeOutboundSlice(alloc: std.mem.Allocator, items: []OutboundMessage) {
    let i: usize = 0;
    while (i < items.len) {
        alloc.free(items[i].target_id);
        alloc.free(items[i].target_addr);
        let msg_mut = items[i].msg;
        freeMessage(alloc, &msg_mut);
        i += 1;
    }
}

pub fn freeOutbound(alloc: std.mem.Allocator, list: []OutboundMessage) {
    freeOutboundSlice(alloc, list);
    alloc.free(list);
}

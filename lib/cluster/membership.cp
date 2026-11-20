// Source: cp/lib/cluster/membership.cp (SWIM-Style Gossip Membership & Incarnation CRDT)
// Deterministic SWIM-style cluster membership table with incarnation-ordered CRDT merge
// and explicit now_ms timeout progression (Alive -> Suspect -> Dead).

const std = @import("std");
import { ClusterErr } from "./hash_ring.cp";

/// Lifecycle state of a cluster member in SWIM failure detection.
pub enum MemberState: u8 {
    Alive = 0,
    Suspect = 1,
    Dead = 2,

    /// Returns the CRDT tie-break severity rank (`Dead > Suspect > Alive`).
    fn severity(): u8 {
        return _ as u8;
    }

    /// Returns `true` when the member is actively healthy (`Alive`).
    fn isAlive(): bool {
        return _ == .Alive;
    }
}

/// Public snapshot of a cluster member's metadata and gossip state.
pub struct MemberInfo {
    node_id: string = "",
    address: string = "",
    incarnation: u64 = 0,
    state: MemberState = MemberState.Alive,
    last_seen_ms: i64 = 0,
}

struct StoredMember {
    node_id_buf: []u8 = "",
    address_buf: []u8 = "",
    incarnation: u64 = 0,
    state: MemberState = MemberState.Alive,
    last_seen_ms: i64 = 0,

    fn toInfo(): MemberInfo {
        return MemberInfo{
            node_id: _.node_id_buf,
            address: _.address_buf,
            incarnation: _.incarnation,
            state: _.state,
            last_seen_ms: _.last_seen_ms,
        };
    }
}

fn infoNodeLessThan(a: MemberInfo, b: MemberInfo): bool {
    return std.mem.order(u8, a.node_id, b.node_id) == .lt;
}

fn sortMemberInfos(items: []MemberInfo) {
    zig {
        std.sort.pdq(MemberInfo, items, {}, struct {
            fn lessThan(_: void, a: MemberInfo, b: MemberInfo) bool {
                return infoNodeLessThan(a, b);
            }
        }.lessThan);
    }
}

/// SWIM-style membership table enforcing monotonic incarnation + state-severity CRDT convergence.
pub struct MembershipTable {
    allocator: std.mem.Allocator = undefined,
    members: std.StringHashMapUnmanaged(StoredMember) = .empty,

    /// Initializes an empty `MembershipTable`.
    static fn init(alloc: std.mem.Allocator): MembershipTable {
        return MembershipTable{
            allocator: alloc,
            members: .empty,
        };
    }

    /// Releases all owned member IDs, addresses, and internal map memory.
    fn deinit(*_) {
        let iter = _.members.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.value_ptr.node_id_buf);
            _.allocator.free(entry.value_ptr.address_buf);
        }
        _.members.deinit(_.allocator);
    }

    /// Returns the total number of tracked members across all states.
    fn len(): usize {
        return _.members.count();
    }

    /// Inserts or refreshes the local/self node heartbeat as `Alive` at `incarnation` and `now_ms`.
    fn upsertSelf(*_, node_id: string, address: string, incarnation: u64, now_ms: i64): ! {
        const info = MemberInfo{
            node_id: node_id,
            address: address,
            incarnation: incarnation,
            state: MemberState.Alive,
            last_seen_ms: now_ms,
        };
        _.mergeUpdate(info)?;
    }

    /// Merges a gossip `MemberInfo` update using SWIM CRDT precedence:
    /// 1. Higher `incarnation` always wins.
    /// 2. Equal `incarnation` converges by `Dead > Suspect > Alive`.
    /// 3. Equal `incarnation` and equal state refreshes `last_seen_ms` / `address` when newer (`Alive`).
    fn mergeUpdate(*_, info: MemberInfo): ! {
        if (info.node_id.len == 0) {
            return ClusterErr.InvalidParameter;
        }

        if (existing := _.members.getPtr(info.node_id)) {
            if (info.incarnation > existing.incarnation) {
                if (!std.mem.eql(u8, existing.address_buf, info.address)) {
                    const new_addr = _.allocator.dupe(u8, info.address) catch return ClusterErr.OutOfMemory;
                    _.allocator.free(existing.address_buf);
                    existing.address_buf = new_addr;
                }
                existing.incarnation = info.incarnation;
                existing.state = info.state;
                existing.last_seen_ms = info.last_seen_ms;
                return;
            }

            if (info.incarnation < existing.incarnation) {
                return;
            }

            // Equal incarnation: Dead > Suspect > Alive
            const in_sev = info.state.severity();
            const cur_sev = existing.state.severity();

            if (in_sev > cur_sev) {
                if (!std.mem.eql(u8, existing.address_buf, info.address)) {
                    const new_addr = _.allocator.dupe(u8, info.address) catch return ClusterErr.OutOfMemory;
                    _.allocator.free(existing.address_buf);
                    existing.address_buf = new_addr;
                }
                existing.state = info.state;
                if (info.last_seen_ms > existing.last_seen_ms && info.state == MemberState.Alive) {
                    existing.last_seen_ms = info.last_seen_ms;
                }
                return;
            }

            if (in_sev == cur_sev && info.state == MemberState.Alive) {
                if (info.last_seen_ms >= existing.last_seen_ms) {
                    if (!std.mem.eql(u8, existing.address_buf, info.address)) {
                        const new_addr = _.allocator.dupe(u8, info.address) catch return ClusterErr.OutOfMemory;
                        _.allocator.free(existing.address_buf);
                        existing.address_buf = new_addr;
                    }
                    existing.last_seen_ms = info.last_seen_ms;
                }
            }
            return;
        }

        const owned_id = _.allocator.dupe(u8, info.node_id) catch return ClusterErr.OutOfMemory;
        errdefer _.allocator.free(owned_id);

        const owned_addr = _.allocator.dupe(u8, info.address) catch return ClusterErr.OutOfMemory;
        errdefer _.allocator.free(owned_addr);

        const stored = StoredMember{
            node_id_buf: owned_id,
            address_buf: owned_addr,
            incarnation: info.incarnation,
            state: info.state,
            last_seen_ms: info.last_seen_ms,
        };
        _.members.put(_.allocator, owned_id, stored) catch return ClusterErr.OutOfMemory;
    }

    /// Deterministically advances stale members based on `now_ms - last_seen_ms`:
    /// `Alive -> Suspect` at `>= suspect_after_ms`, and `Alive/Suspect -> Dead` at `>= dead_after_ms`.
    fn tick(*_, now_ms: i64, suspect_after_ms: i64, dead_after_ms: i64) {
        const eff_suspect: i64 = if (suspect_after_ms < 0) 0 else suspect_after_ms;
        const eff_dead: i64 = if (dead_after_ms < eff_suspect) eff_suspect else dead_after_ms;

        let iter = _.members.iterator();
        while (entry := iter.next()) {
            if (entry.value_ptr.state == MemberState.Dead) continue;
            if (now_ms <= entry.value_ptr.last_seen_ms) continue;

            const elapsed = now_ms - entry.value_ptr.last_seen_ms;
            if (elapsed >= eff_dead) {
                entry.value_ptr.state = MemberState.Dead;
            } else if (elapsed >= eff_suspect && entry.value_ptr.state == MemberState.Alive) {
                entry.value_ptr.state = MemberState.Suspect;
            }
        }
    }

    /// Returns a snapshot of `node_id`'s membership info, or `null` if unknown.
    fn get(node_id: string): MemberInfo? {
        if (stored := _.members.get(node_id)) {
            return stored.toInfo();
        }
        return null;
    }

    /// Counts how many tracked members currently match `state`.
    fn countByState(state: MemberState): usize {
        let count: usize = 0;
        let iter = _.members.iterator();
        while (entry := iter.next()) {
            if (entry.value_ptr.state == state) {
                count += 1;
            }
        }
        return count;
    }

    /// Returns a deterministically sorted slice of all `Alive` members.
    /// Caller owns the returned slice and must free it with `alloc.free`.
    fn aliveMembers(alloc: std.mem.Allocator): ([]MemberInfo)! {
        const alive_cnt = _.countByState(MemberState.Alive);
        let out = alloc.alloc(MemberInfo, alive_cnt) catch return ClusterErr.OutOfMemory;
        if (alive_cnt == 0) return out;

        let idx: usize = 0;
        let iter = _.members.iterator();
        while (entry := iter.next()) {
            if (entry.value_ptr.state == MemberState.Alive) {
                out[idx] = entry.value_ptr.toInfo();
                idx += 1;
            }
        }
        sortMemberInfos(out);
        return out;
    }

    /// Removes `node_id` from the membership table and frees its owned buffers.
    fn remove(*_, node_id: string): bool {
        if (kv := _.members.fetchRemove(node_id)) {
            _.allocator.free(kv.value.node_id_buf);
            _.allocator.free(kv.value.address_buf);
            return true;
        }
        return false;
    }
}

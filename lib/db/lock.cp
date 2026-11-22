// Source: cp/lib/lock/lock.cp (Named RW Lock Table with Wait-For Deadlock Detection & Fencing-Token Lease Manager)
// Pure CP logical read-write lock table with wait-for graph deadlock detection
// and monotonic fencing-token TTL lease lock manager for multi-agent coordination.

const std = @import("std");
import { WaitForGraph } from "./lock_wait_graph.cp";

/// Error set returned by `NamedLockTable` and `LeaseLockManager` operations.
pub err LockError {
    LockBusy,
    NotOwner,
    DeadlockDetected,
    LeaseExpired,
    InvalidTtl,
    OutOfMemory,
}

/// Lock acquisition mode for `NamedLockTable`.
pub enum LockMode {
    Shared,
    Exclusive,
}

struct HolderEntry {
    owner: []u8,
    mode: LockMode,
    count: usize = 1,

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.owner);
    }
}

struct WaitRecord {
    waiter: []u8,
    blocker: []u8,

    fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.waiter);
        alloc.free(_.blocker);
    }
}

struct ResourceEntry {
    name: []u8,
    holders: std.ArrayList(HolderEntry) = .empty,
    waiters: std.ArrayList(WaitRecord) = .empty,

    fn deinit(*_, alloc: std.mem.Allocator) {
        for (*h of _.holders.items) {
            h.deinit(alloc);
        }
        _.holders.deinit(alloc);

        for (*w of _.waiters.items) {
            w.deinit(alloc);
        }
        _.waiters.deinit(alloc);

        alloc.free(_.name);
    }
}

/// Non-blocking cooperative named read-write lock table with integrated wait-for
/// graph deadlock detection.
pub struct NamedLockTable {
    alloc: std.mem.Allocator = undefined,
    resources: std.ArrayList(ResourceEntry) = .empty,
    resource_index: std.StringHashMapUnmanaged(usize) = .empty,
    wait_graph: WaitForGraph = undefined,

    /// Initializes an empty `NamedLockTable` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): NamedLockTable {
        return NamedLockTable{
            alloc: alloc,
            resources: .empty,
            resource_index: .empty,
            wait_graph: WaitForGraph.init(alloc),
        };
    }

    /// Releases all lock entries, wait records, and internal wait-for graph state.
    pub fn deinit(*_) {
        for (*r of _.resources.items) {
            r.deinit(_.alloc);
        }
        _.resources.deinit(_.alloc);
        _.resource_index.deinit(_.alloc);
        _.wait_graph.deinit();
    }

    fn ensureResource(*_, resource: string): usize ! LockError {
        if (idx := _.resource_index.get(resource)) {
            return idx;
        }
        const owned = _.alloc.dupe(u8, resource) catch return LockError.OutOfMemory;
        errdefer _.alloc.free(owned);

        const idx = _.resources.items.len;
        _.resources.append(_.alloc, ResourceEntry{
            name: owned,
            holders: .empty,
            waiters: .empty,
        }) catch return LockError.OutOfMemory;
        errdefer * = _.resources.pop();

        _.resource_index.put(_.alloc, owned, idx) catch return LockError.OutOfMemory;
        return idx;
    }

    fn clearWaiterForResource(*_, res_idx: usize, waiter: string) {
        let waiters = *_.resources.items[res_idx].waiters;
        let i: usize = 0;
        while (i < waiters.items.len) {
            if (std.mem.eql(u8, waiters.items[i].waiter, waiter)) {
                _.wait_graph.removeEdge(waiters.items[i].waiter, waiters.items[i].blocker);
                waiters.items[i].deinit(_.alloc);
                * = waiters.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    fn clearBlockerForResource(*_, res_idx: usize, blocker: string) {
        let waiters = *_.resources.items[res_idx].waiters;
        let i: usize = 0;
        while (i < waiters.items.len) {
            if (std.mem.eql(u8, waiters.items[i].blocker, blocker)) {
                _.wait_graph.removeEdge(waiters.items[i].waiter, waiters.items[i].blocker);
                waiters.items[i].deinit(_.alloc);
                * = waiters.swapRemove(i);
            } else {
                i += 1;
            }
        }
    }

    fn addWaitRecord(*_, res_idx: usize, waiter: string, blocker: string): void ! LockError {
        let exists = false;
        for (rec of _.resources.items[res_idx].waiters.items) {
            if (std.mem.eql(u8, rec.waiter, waiter) && std.mem.eql(u8, rec.blocker, blocker)) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            const w_owned = _.alloc.dupe(u8, waiter) catch return LockError.OutOfMemory;
            errdefer _.alloc.free(w_owned);
            const b_owned = _.alloc.dupe(u8, blocker) catch return LockError.OutOfMemory;
            errdefer _.alloc.free(b_owned);

            _.resources.items[res_idx].waiters.append(_.alloc, WaitRecord{
                waiter: w_owned,
                blocker: b_owned,
            }) catch return LockError.OutOfMemory;

            _.wait_graph.addEdge(waiter, blocker) catch {
                let popped = _.resources.items[res_idx].waiters.pop();
                if (p := popped) {
                    let mut_p = p;
                    mut_p.deinit(_.alloc);
                }
                return LockError.OutOfMemory;
            };
        }
    }

    /// Attempts to acquire `resource` for `owner` in `mode` (`Shared` or `Exclusive`).
    ///
    /// Supports concurrent `Shared` readers, same-`owner` reentrancy (including
    /// sole-reader upgrade to `Exclusive`), and mutual exclusion for `Exclusive`.
    /// When blocked by existing holders, records `owner -> blocker` edges in the
    /// internal wait-for graph; if this creates a cycle, rolls back the edges and
    /// returns `LockError.DeadlockDetected`.
    pub fn tryAcquire(*_, resource: string, owner: string, mode: LockMode): bool ! LockError {
        const res_idx = _.ensureResource(resource)?;
        let holders = *_.resources.items[res_idx].holders;

        // Check if `owner` already holds a lock on `resource` (reentrancy / upgrade).
        let self_pos: usize? = null;
        let idx_i: usize = 0;
        while (idx_i < holders.items.len) : (idx_i += 1) {
            if (std.mem.eql(u8, holders.items[idx_i].owner, owner)) {
                self_pos = idx_i;
                break;
            }
        }

        if (pos := self_pos) {
            const cur_mode = holders.items[pos].mode;
            if (cur_mode == .Exclusive || mode == .Shared) {
                holders.items[pos].count += 1;
                _.clearWaiterForResource(res_idx, owner);
                return true;
            }
            // Upgrade from Shared -> Exclusive: allowed only if `owner` is the sole holder.
            if (holders.items.len == 1) {
                holders.items[pos].mode = .Exclusive;
                holders.items[pos].count += 1;
                _.clearWaiterForResource(res_idx, owner);
                return true;
            }
            // Blocked by other shared readers.
            for (h of holders.items) {
                if (!std.mem.eql(u8, h.owner, owner)) {
                    _.addWaitRecord(res_idx, owner, h.owner)?;
                }
            }
            const has_cycle = _.wait_graph.hasCycle(_.alloc) catch return LockError.OutOfMemory;
            if (has_cycle) {
                _.clearWaiterForResource(res_idx, owner);
                return LockError.DeadlockDetected;
            }
            return false;
        }

        // `owner` does not currently hold `resource`.
        if (holders.items.len == 0) {
            const owned_owner = _.alloc.dupe(u8, owner) catch return LockError.OutOfMemory;
            errdefer _.alloc.free(owned_owner);
            holders.append(_.alloc, HolderEntry{
                owner: owned_owner,
                mode: mode,
                count: 1,
            }) catch return LockError.OutOfMemory;
            _.clearWaiterForResource(res_idx, owner);
            return true;
        }

        // Shared acquisition is compatible if all existing holders are Shared.
        if (mode == .Shared) {
            let all_shared = true;
            for (h of holders.items) {
                if (h.mode == .Exclusive) {
                    all_shared = false;
                    break;
                }
            }
            if (all_shared) {
                const owned_owner = _.alloc.dupe(u8, owner) catch return LockError.OutOfMemory;
                errdefer _.alloc.free(owned_owner);
                holders.append(_.alloc, HolderEntry{
                    owner: owned_owner,
                    mode: .Shared,
                    count: 1,
                }) catch return LockError.OutOfMemory;
                _.clearWaiterForResource(res_idx, owner);
                return true;
            }
        }

        // Blocked by incompatible holders: register wait edges and check for cycle.
        for (h of holders.items) {
            if (mode == .Exclusive || h.mode == .Exclusive) {
                _.addWaitRecord(res_idx, owner, h.owner)?;
            }
        }
        const has_cycle = _.wait_graph.hasCycle(_.alloc) catch return LockError.OutOfMemory;
        if (has_cycle) {
            _.clearWaiterForResource(res_idx, owner);
            return LockError.DeadlockDetected;
        }
        return false;
    }

    /// Releases one hold count of `resource` by `owner` and clears `owner`'s
    /// wait records for `resource`. Returns `LockError.NotOwner` if `owner` does
    /// not currently hold `resource`.
    pub fn unlock(*_, resource: string, owner: string): void ! LockError {
        const res_idx_opt = _.resource_index.get(resource);
        if (res_idx_opt == null) {
            return LockError.NotOwner;
        }
        const res_idx = res_idx_opt.?;
        let holders = *_.resources.items[res_idx].holders;

        let found = false;
        let i: usize = 0;
        while (i < holders.items.len) : (i += 1) {
            if (std.mem.eql(u8, holders.items[i].owner, owner)) {
                _.clearWaiterForResource(res_idx, owner);
                if (holders.items[i].count > 1) {
                    holders.items[i].count -= 1;
                } else {
                    holders.items[i].deinit(_.alloc);
                    * = holders.swapRemove(i);
                    _.clearBlockerForResource(res_idx, owner);
                }
                found = true;
                break;
            }
        }

        if (!found) {
            return LockError.NotOwner;
        }
    }

    /// Returns the number of distinct owners currently holding `resource`.
    pub fn holderCount(resource: string): usize {
        const res_idx_opt = _.resource_index.get(resource);
        if (res_idx_opt == null) {
            return 0;
        }
        return _.resources.items[res_idx_opt.?].holders.items.len;
    }

    /// Returns `true` if `resource` currently has at least one lock holder.
    pub fn isResourceLocked(resource: string): bool {
        return _.holderCount(resource) > 0;
    }

    /// Runs cycle detection on the internal wait-for graph.
    pub fn detectCycle(alloc: std.mem.Allocator): bool ! LockError {
        return _.wait_graph.hasCycle(alloc) catch return LockError.OutOfMemory;
    }
}

/// Lease token issued by `LeaseLockManager` with a monotonic `fencing_token`.
pub struct LeaseToken {
    resource: string = "",
    owner: string = "",
    fencing_token: u64 = 0,
    expires_at_ms: u64 = 0,
}

struct LeaseEntry {
    active: bool = false,
    owner: string = "",
    fencing_token: u64 = 0,
    expires_at_ms: u64 = 0,
}

/// Lease lock manager with TTL expiration and strictly monotonic `fencing_token`
/// generation for split-brain prevention.
pub struct LeaseLockManager {
    alloc: std.mem.Allocator = undefined,
    next_fencing_token: u64 = 0,
    leases: std.StringHashMapUnmanaged(LeaseEntry) = .empty,
    intern_pool: std.ArrayList([]u8) = .empty,
    intern_map: std.StringHashMapUnmanaged(string) = .empty,

    /// Initializes an empty `LeaseLockManager` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): LeaseLockManager {
        return LeaseLockManager{
            alloc: alloc,
            next_fencing_token: 0,
            leases: .empty,
            intern_pool: .empty,
            intern_map: .empty,
        };
    }

    /// Releases all interned strings and lease table storage.
    pub fn deinit(*_) {
        _.leases.deinit(_.alloc);
        _.intern_map.deinit(_.alloc);
        for (s of _.intern_pool.items) {
            _.alloc.free(s);
        }
        _.intern_pool.deinit(_.alloc);
    }

    fn intern(*_, s: string): string ! LockError {
        if (existing := _.intern_map.get(s)) {
            return existing;
        }
        const owned = _.alloc.dupe(u8, s) catch return LockError.OutOfMemory;
        errdefer _.alloc.free(owned);

        _.intern_pool.append(_.alloc, owned) catch return LockError.OutOfMemory;
        errdefer * = _.intern_pool.pop();

        const view: string = owned;
        _.intern_map.put(_.alloc, view, view) catch return LockError.OutOfMemory;
        return view;
    }

    /// Acquires a TTL lease on `resource` for `owner` at `now_ms`.
    ///
    /// Succeeds if `resource` is free, its prior lease has expired (`now_ms >= expires_at_ms`),
    /// or `owner` already holds the active lease; increments the global monotonic
    /// `fencing_token`. Returns `LockError.LockBusy` if another owner holds an active lease.
    pub fn acquireLease(*_, resource: string, owner: string, ttl_ms: u64, now_ms: u64): LeaseToken ! LockError {
        if (ttl_ms == 0) {
            return LockError.InvalidTtl;
        }
        const res_key = _.intern(resource)?;
        const owner_key = _.intern(owner)?;
        const expires_at = now_ms +| ttl_ms;

        if (*entry := _.leases.getPtr(res_key)) {
            if (entry.active && now_ms < entry.expires_at_ms && !std.mem.eql(u8, entry.owner, owner_key)) {
                return LockError.LockBusy;
            }
            _.next_fencing_token += 1;
            entry.active = true;
            entry.owner = owner_key;
            entry.fencing_token = _.next_fencing_token;
            entry.expires_at_ms = expires_at;
            return LeaseToken{
                resource: res_key,
                owner: owner_key,
                fencing_token: entry.fencing_token,
                expires_at_ms: entry.expires_at_ms,
            };
        }

        _.next_fencing_token += 1;
        const tok = _.next_fencing_token;
        _.leases.put(_.alloc, res_key, LeaseEntry{
            active: true,
            owner: owner_key,
            fencing_token: tok,
            expires_at_ms: expires_at,
        }) catch return LockError.OutOfMemory;

        return LeaseToken{
            resource: res_key,
            owner: owner_key,
            fencing_token: tok,
            expires_at_ms: expires_at,
        };
    }

    /// Renews an active lease if `owner` and `fencing_token` match and the lease
    /// has not expired (`now_ms < expires_at_ms`).
    pub fn renewLease(*_, resource: string, owner: string, fencing_token: u64, ttl_ms: u64, now_ms: u64): LeaseToken ! LockError {
        if (ttl_ms == 0) {
            return LockError.InvalidTtl;
        }
        const entry_opt = _.leases.getPtr(resource);
        if (entry_opt == null) {
            return LockError.LeaseExpired;
        }
        let entry = entry_opt.?;
        if (!entry.active) {
            return LockError.LeaseExpired;
        }
        if (!std.mem.eql(u8, entry.owner, owner)) {
            return LockError.NotOwner;
        }
        if (entry.fencing_token != fencing_token || now_ms >= entry.expires_at_ms) {
            return LockError.LeaseExpired;
        }

        const res_key = _.intern(resource)?;
        entry.expires_at_ms = now_ms +| ttl_ms;
        return LeaseToken{
            resource: res_key,
            owner: entry.owner,
            fencing_token: entry.fencing_token,
            expires_at_ms: entry.expires_at_ms,
        };
    }

    /// Explicitly releases an active lease if `owner` and `fencing_token` match.
    pub fn releaseLease(*_, resource: string, owner: string, fencing_token: u64): void ! LockError {
        const entry_opt = _.leases.getPtr(resource);
        if (entry_opt == null) {
            return LockError.LeaseExpired;
        }
        let entry = entry_opt.?;
        if (!entry.active) {
            return LockError.LeaseExpired;
        }
        if (!std.mem.eql(u8, entry.owner, owner)) {
            return LockError.NotOwner;
        }
        if (entry.fencing_token != fencing_token) {
            return LockError.LeaseExpired;
        }
        entry.active = false;
    }

    /// Validates whether `fencing_token` is the current active, non-expired token
    /// for `resource` at `now_ms`. Used by storage layers to reject stale writers.
    pub fn validateToken(resource: string, fencing_token: u64, now_ms: u64): bool {
        const entry_opt = _.leases.get(resource);
        if (entry_opt == null) {
            return false;
        }
        const entry = entry_opt.?;
        if (!entry.active) {
            return false;
        }
        if (entry.fencing_token != fencing_token) {
            return false;
        }
        if (now_ms >= entry.expires_at_ms) {
            return false;
        }
        return true;
    }
}

// Source: cp/lib/mvcc/store.cp (MVCC timestamp oracle, snapshot isolation store, txn handle, and GC)

const std = @import("std");
import { MvccError, TxnState, VersionValue, WriteEntry, ActiveTxnInfo } from "./mvccTypes.cp";

pub struct MvccStore {
    pub alloc: std.mem.Allocator,
    pub current_ts: u64,
    pub next_txn_id: u64,
    pub versions: std.StringHashMapUnmanaged(std.ArrayList(VersionValue)),
    pub active_txns: std.ArrayList(ActiveTxnInfo),

    pub fn init(alloc: std.mem.Allocator): MvccStore {
        return MvccStore{
            .alloc = alloc,
            .current_ts = 0,
            .next_txn_id = 1,
            .versions = std.StringHashMapUnmanaged(std.ArrayList(VersionValue)).empty,
            .active_txns = std.ArrayList(ActiveTxnInfo).empty,
        };
    }

    pub fn deinit(*_) {
        let it = _.versions.iterator();
        while (entry := it.next()) {
            _.alloc.free(entry.key_ptr.*);
            let i: usize = 0;
            while (i < entry.value_ptr.items.len) {
                if (v := entry.value_ptr.items[i].value) {
                    _.alloc.free(v);
                }
                i += 1;
            }
            entry.value_ptr.deinit(_.alloc);
        }
        _.versions.deinit(_.alloc);
        _.active_txns.deinit(_.alloc);
    }

    pub fn beginTxn(*_, read_only: bool): MvccError!MvccTxn {
        const txn_id = _.next_txn_id;
        _.next_txn_id += 1;
        const read_ts = _.current_ts;

        _.active_txns.append(_.alloc, ActiveTxnInfo{
            .txn_id = txn_id,
            .read_ts = read_ts,
        }) catch return MvccError.OutOfMemory;

        return MvccTxn{
            .store = _,
            .txn_id = txn_id,
            .read_ts = read_ts,
            .commit_ts = 0,
            .read_only = read_only,
            .state = TxnState.Active,
            .write_set = std.ArrayList(WriteEntry).empty,
        };
    }

    pub fn removeActiveTxn(*_, txn_id: u64) {
        let i: usize = 0;
        while (i < _.active_txns.items.len) {
            if (_.active_txns.items[i].txn_id == txn_id) {
                * = _.active_txns.swapRemove(i);
                return;
            }
            i += 1;
        }
    }

    pub fn minActiveReadTs(): u64 {
        if (_.active_txns.items.len == 0) {
            return _.current_ts;
        }
        let min_ts = _.active_txns.items[0].read_ts;
        let i: usize = 1;
        while (i < _.active_txns.items.len) {
            if (_.active_txns.items[i].read_ts < min_ts) {
                min_ts = _.active_txns.items[i].read_ts;
            }
            i += 1;
        }
        return min_ts;
    }

    pub fn gcVersions(*_, safe_watermark_ts: u64): usize {
        let pruned: usize = 0;
        let it = _.versions.iterator();
        while (entry := it.next()) {
            let chain = entry.value_ptr;
            if (chain.items.len <= 1) {
                continue;
            }

            // Versions are stored in descending commit_ts order.
            // Keep the newest version with commit_ts <= safe_watermark_ts as baseline,
            // and prune all older overwritten versions after it.
            let keep_until: usize? = null;
            let i: usize = 0;
            while (i < chain.items.len) {
                if (chain.items[i].commit_ts <= safe_watermark_ts) {
                    keep_until = i;
                    break;
                }
                i += 1;
            }

            if (baseline_idx := keep_until) {
                const new_len = baseline_idx + 1;
                if (new_len < chain.items.len) {
                    let j: usize = new_len;
                    while (j < chain.items.len) {
                        if (v := chain.items[j].value) {
                            _.alloc.free(v);
                        }
                        pruned += 1;
                        j += 1;
                    }
                    chain.shrinkRetainingCapacity(new_len);
                }
            }
        }
        return pruned;
    }

    pub fn versionCount(key: []const u8): usize {
        if (chain := _.versions.getPtr(key)) {
            return chain.items.len;
        }
        return 0;
    }
}

pub struct MvccTxn {
    pub store: *MvccStore,
    pub txn_id: u64,
    pub read_ts: u64,
    pub commit_ts: u64,
    pub read_only: bool,
    pub state: TxnState,
    pub write_set: std.ArrayList(WriteEntry),

    pub fn deinit(*_) {
        _.freeWriteSet();
        if (_.state == TxnState.Active) {
            _.store.removeActiveTxn(_.txn_id);
            _.state = TxnState.Aborted;
        }
    }

    fn freeWriteSet(*_) {
        let i: usize = 0;
        while (i < _.write_set.items.len) {
            _.store.alloc.free(_.write_set.items[i].key);
            if (v := _.write_set.items[i].value) {
                _.store.alloc.free(v);
            }
            i += 1;
        }
        _.write_set.deinit(_.store.alloc);
        _.write_set = std.ArrayList(WriteEntry).empty;
    }

    pub fn get(key: []const u8): ([]const u8)? {
        if (_.state != TxnState.Active) {
            return null;
        }

        // 1. Read-Your-Own-Writes: check local write_set first
        let i: usize = 0;
        while (i < _.write_set.items.len) {
            if (std.mem.eql(u8, _.write_set.items[i].key, key)) {
                return _.write_set.items[i].value;
            }
            i += 1;
        }

        // 2. Scan committed version chain for newest commit_ts <= read_ts
        if (chain := _.store.versions.getPtr(key)) {
            let j: usize = 0;
            while (j < chain.items.len) {
                if (chain.items[j].commit_ts <= _.read_ts) {
                    return chain.items[j].value;
                }
                j += 1;
            }
        }

        return null;
    }

    pub fn put(*_, key: []const u8, val: []const u8): MvccError!void {
        try _.writeInternal(key, val);
    }

    pub fn delete(*_, key: []const u8): MvccError!void {
        try _.writeInternal(key, null);
    }

    fn writeInternal(*_, key: []const u8, val: ([]const u8)?): MvccError!void {
        if (_.state == TxnState.Aborted) {
            return MvccError.TxnAborted;
        }
        if (_.state != TxnState.Active) {
            return MvccError.TxnNotActive;
        }
        if (_.read_only) {
            return MvccError.ReadOnlyTxn;
        }

        let dup_val: ([]const u8)? = null;
        if (v := val) {
            dup_val = _.store.alloc.dupe(u8, v) catch return MvccError.OutOfMemory;
        }

        let i: usize = 0;
        while (i < _.write_set.items.len) {
            if (std.mem.eql(u8, _.write_set.items[i].key, key)) {
                if (old_v := _.write_set.items[i].value) {
                    _.store.alloc.free(old_v);
                }
                _.write_set.items[i].value = dup_val;
                return;
            }
            i += 1;
        }

        const dup_key = _.store.alloc.dupe(u8, key) catch {
            if (dv := dup_val) {
                _.store.alloc.free(dv);
            }
            return MvccError.OutOfMemory;
        };

        _.write_set.append(_.store.alloc, WriteEntry{
            .key = dup_key,
            .value = dup_val,
        }) catch {
            _.store.alloc.free(dup_key);
            if (dv := dup_val) {
                _.store.alloc.free(dv);
            }
            return MvccError.OutOfMemory;
        };
    }

    pub fn rollback(*_) {
        _.freeWriteSet();
        if (_.state == TxnState.Active) {
            _.store.removeActiveTxn(_.txn_id);
            _.state = TxnState.Aborted;
        }
    }

    pub fn commit(*_): MvccError!u64 {
        if (_.state == TxnState.Aborted) {
            return MvccError.TxnAborted;
        }
        if (_.state != TxnState.Active) {
            return MvccError.TxnNotActive;
        }

        if (_.read_only or _.write_set.items.len == 0) {
            _.state = TxnState.Committed;
            _.commit_ts = _.read_ts;
            _.store.removeActiveTxn(_.txn_id);
            return _.commit_ts;
        }

        // First-Committer-Wins conflict detection
        let i: usize = 0;
        while (i < _.write_set.items.len) {
            if (chain := _.store.versions.getPtr(_.write_set.items[i].key)) {
                if (chain.items.len > 0) {
                    const latest_commit_ts = chain.items[0].commit_ts;
                    if (latest_commit_ts > _.read_ts) {
                        _.rollback();
                        return MvccError.WriteConflict;
                    }
                }
            }
            i += 1;
        }

        // Allocate new commit timestamp atomically
        _.store.current_ts += 1;
        const new_commit_ts = _.store.current_ts;

        // Apply write_set to version chains (newest at index 0)
        let j: usize = 0;
        while (j < _.write_set.items.len) {
            const entry = _.write_set.items[j];
            if (chain := _.store.versions.getPtr(entry.key)) {
                chain.insert(_.store.alloc, 0, VersionValue{
                    .commit_ts = new_commit_ts,
                    .txn_id = _.txn_id,
                    .value = entry.value,
                }) catch return MvccError.OutOfMemory;
                _.store.alloc.free(entry.key);
            } else {
                let new_chain = std.ArrayList(VersionValue).empty;
                new_chain.append(_.store.alloc, VersionValue{
                    .commit_ts = new_commit_ts,
                    .txn_id = _.txn_id,
                    .value = entry.value,
                }) catch return MvccError.OutOfMemory;
                _.store.versions.put(_.store.alloc, entry.key, new_chain) catch {
                    new_chain.deinit(_.store.alloc);
                    return MvccError.OutOfMemory;
                };
            }
            j += 1;
        }

        // Ownership of keys/values has been transferred/freed
        _.write_set.deinit(_.store.alloc);
        _.write_set = std.ArrayList(WriteEntry).empty;
        _.state = TxnState.Committed;
        _.commit_ts = new_commit_ts;
        _.store.removeActiveTxn(_.txn_id);
        return new_commit_ts;
    }
}

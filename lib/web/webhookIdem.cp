// Source: cp/lib/webhook/idempotency.cp (TTL-Bounded Webhook Idempotency Deduplication Store)
// Pure CP deterministic TTL-bounded deduplication store for webhook event IDs,
// preventing duplicate delivery processing within a configurable expiration window.

const std = @import("std");

struct IdempotencyEntry {
    key_buf: []u8,
    expires_at_ms: u64,
}

/// TTL-bounded webhook event deduplication store keyed by owned event ID strings.
pub struct IdempotencyStore {
    allocator: std.mem.Allocator,
    default_ttl_ms: u64,
    entries: std.StringHashMapUnmanaged(IdempotencyEntry) = .empty,

    /// Initializes an empty `IdempotencyStore` with `default_ttl_ms` expiration window.
    pub static fn init(alloc: std.mem.Allocator, default_ttl_ms: u64): IdempotencyStore {
        return IdempotencyStore{
            allocator: alloc,
            default_ttl_ms: default_ttl_ms,
            entries: .empty,
        };
    }

    /// Releases all owned event ID buffers and underlying map storage.
    pub fn deinit(*_) {
        let iter = _.entries.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.value_ptr.key_buf);
        }
        _.entries.deinit(_.allocator);
    }

    /// Checks whether `event_id` is currently active within its TTL window at `now_ms`.
    /// If unseen or expired, records `event_id` with `now_ms +| default_ttl_ms` and returns `true`.
    /// If already seen and not expired, returns `false` to block duplicate processing.
    pub fn checkAndRecord(*_, event_id: []const u8, now_ms: u64): bool {
        return _.checkAndRecordWithTtl(event_id, _.default_ttl_ms, now_ms);
    }

    /// Same as `checkAndRecord`, using an explicit per-event `ttl_ms` override.
    pub fn checkAndRecordWithTtl(*_, event_id: []const u8, ttl_ms: u64, now_ms: u64): bool {
        const expires_at = now_ms +| ttl_ms;
        if (existing := _.entries.getPtr(event_id)) {
            if (now_ms < existing.expires_at_ms) {
                return false;
            }
            existing.expires_at_ms = expires_at;
            return true;
        }

        const owned_key = _.allocator.dupe(u8, event_id) catch return false;
        const entry = IdempotencyEntry{
            key_buf: owned_key,
            expires_at_ms: expires_at,
        };
        _.entries.put(_.allocator, owned_key, entry) catch {
            _.allocator.free(owned_key);
            return false;
        };
        return true;
    }

    /// Returns `true` if `event_id` exists in the store and has not expired at `now_ms`.
    /// Does not mutate store state or insert missing keys.
    pub fn isSeen(event_id: []const u8, now_ms: u64): bool {
        if (entry := _.entries.get(event_id)) {
            return now_ms < entry.expires_at_ms;
        }
        return false;
    }

    /// Removes all entries whose expiration timestamp satisfies `now_ms >= expires_at_ms`,
    /// freeing their owned keys. Returns the number of purged entries.
    pub fn purgeExpired(*_, now_ms: u64): usize {
        let removed: usize = 0;
        while (true) {
            let batch: [64][]const u8 = undefined;
            let batch_len: usize = 0;
            let iter = _.entries.iterator();
            while (entry := iter.next()) {
                if (now_ms >= entry.value_ptr.expires_at_ms) {
                    batch[batch_len] = entry.key_ptr.*;
                    batch_len += 1;
                    if (batch_len == 64) break;
                }
            }
            if (batch_len == 0) break;
            for (i of 0..batch_len) {
                if (kv := _.entries.fetchRemove(batch[i])) {
                    _.allocator.free(kv.value.key_buf);
                    removed += 1;
                }
            }
        }
        return removed;
    }

    /// Removes a specific `event_id` from the store, freeing its owned key. Returns `true` if removed.
    pub fn remove(*_, event_id: []const u8): bool {
        if (kv := _.entries.fetchRemove(event_id)) {
            _.allocator.free(kv.value.key_buf);
            return true;
        }
        return false;
    }

    /// Returns the total number of entries currently stored (including unpurged expired entries).
    pub fn len(): usize {
        return _.entries.count();
    }
}

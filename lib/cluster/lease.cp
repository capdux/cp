// Source: cp/lib/cluster/lease.cp (Monotonic Fencing-Token Leader Lease Coordinator)
// Split-brain-safe leader lease coordinator issuing strictly monotonic fencing tokens
// to reject stale-leader writes after lease expiration or network partition.

const std = @import("std");

/// Snapshot of an active leader lease and its monotonic fencing token.
pub struct LeaseInfo {
    holder_id: string = "",
    fencing_token: u64 = 0,
    expires_at_ms: i64 = 0,
}

/// Monotonic fencing-token leader lease coordinator with deterministic `now_ms` clock injection.
pub struct LeaderLease {
    allocator: std.mem.Allocator = undefined,
    holder_buf: ([]u8)? = null,
    current_token: u64 = 0,
    expires_at_ms: i64 = 0,
    active: bool = false,

    /// Initializes a free `LeaderLease` with `fencing_token = 0`.
    static fn init(alloc: std.mem.Allocator): LeaderLease {
        return LeaderLease{
            allocator: alloc,
            holder_buf: null,
            current_token: 0,
            expires_at_ms: 0,
            active: false,
        };
    }

    /// Releases any owned holder identifier buffer.
    fn deinit(*_) {
        if (buf := _.holder_buf) {
            _.allocator.free(buf);
            _.holder_buf = null;
        }
        _.active = false;
    }

    /// Attempts to acquire the leader lease for `candidate_id` with duration `ttl_ms > 0`.
    /// Succeeds only when no active unexpired lease exists, strictly incrementing `fencing_token`.
    fn tryAcquire(*_, candidate_id: string, ttl_ms: i64, now_ms: i64): LeaseInfo? {
        if (candidate_id.len == 0 || ttl_ms <= 0) return null;

        if (_.active && now_ms < _.expires_at_ms) {
            return null;
        }

        const owned_holder = _.allocator.dupe(u8, candidate_id) catch return null;
        if (old_buf := _.holder_buf) {
            _.allocator.free(old_buf);
        }
        _.holder_buf = owned_holder;
        _.current_token += 1;
        _.expires_at_ms = now_ms + ttl_ms;
        _.active = true;

        return LeaseInfo{
            holder_id: owned_holder,
            fencing_token: _.current_token,
            expires_at_ms: _.expires_at_ms,
        };
    }

    /// Extends an unexpired lease currently held by `holder_id` without changing its `fencing_token`.
    /// Returns `null` if the lease is expired, free, or held by a different node.
    fn renew(*_, holder_id: string, ttl_ms: i64, now_ms: i64): LeaseInfo? {
        if (holder_id.len == 0 || ttl_ms <= 0) return null;
        if (!_.active) return null;

        if (now_ms >= _.expires_at_ms) {
            _.active = false;
            return null;
        }

        if (buf := _.holder_buf) {
            if (!std.mem.eql(u8, buf, holder_id)) {
                return null;
            }
            _.expires_at_ms = now_ms + ttl_ms;
            return LeaseInfo{
                holder_id: buf,
                fencing_token: _.current_token,
                expires_at_ms: _.expires_at_ms,
            };
        }
        return null;
    }

    /// Voluntarily releases the lease if currently active and held by `holder_id`.
    /// Preserves `current_token` monotonicity for subsequent acquisitions.
    fn releaseLease(*_, holder_id: string): bool {
        if (!_.active || holder_id.len == 0) return false;
        if (buf := _.holder_buf) {
            if (!std.mem.eql(u8, buf, holder_id)) {
                return false;
            }
            _.allocator.free(buf);
            _.holder_buf = null;
            _.active = false;
            return true;
        }
        _.active = false;
        return false;
    }

    zig {
        /// Alias `release(holder_id)` exposed via `zig` block to avoid CP auto-drop
        /// `release()` hook collision on zero-argument destructor detection.
        pub fn release(self: *LeaderLease, holder_id: []const u8) bool {
            return self.releaseLease(holder_id);
        }
    }

    /// Returns the active leader lease snapshot at `now_ms`, or `null` if free or expired.
    fn currentLeader(now_ms: i64): LeaseInfo? {
        if (!_.active || now_ms >= _.expires_at_ms) return null;
        if (buf := _.holder_buf) {
            return LeaseInfo{
                holder_id: buf,
                fencing_token: _.current_token,
                expires_at_ms: _.expires_at_ms,
            };
        }
        return null;
    }

    /// Validates a fencing token against split-brain stale leaders (`token >= current_fencing_token`).
    /// Rejects `0` or any token older than the latest issued fencing token.
    fn validateToken(token: u64): bool {
        if (_.current_token == 0) return false;
        return token >= _.current_token;
    }

    /// Returns the highest fencing token issued so far (`0` if never acquired).
    fn currentFencingToken(): u64 {
        return _.current_token;
    }
}

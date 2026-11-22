// Source: cp/lib/pool/pool.cp (Generic Bounded Resource & Connection Pool Engine)
// Pure CP zero-C-dependency bounded resource pool with min_idle/max_size bounds,
// LIFO/FIFO borrow order, idle_timeout_ms & max_lifetime_ms eviction,
// pre-acquire health validation, discard, and full PoolStats observability.

const std = @import("std");

/// Error set for ResourcePool lifecycle and borrow operations.
pub err PoolError {
    PoolExhausted,
    InvalidConfig,
    CreateFailed,
    PoolClosed,
    InvalidHandle,
    OutOfMemory,
}

/// Strategy for selecting an idle resource from the pool.
pub enum BorrowOrder {
    Lifo,
    Fifo,
}

/// Configuration for ResourcePool capacity, expiration, and borrow policy.
pub struct PoolConfig {
    min_idle: usize = 0;
    max_size: usize = 10;
    idle_timeout_ms: u64 = 0;
    max_lifetime_ms: u64 = 0;
    borrow_order: BorrowOrder = .Lifo;
}

/// Callback invoked to create a new pooled resource instance.
pub type CreateFn = *const fn (*anyopaque, std.mem.Allocator) *anyopaque ! PoolError;

/// Callback invoked to destroy a pooled resource instance.
pub type DestroyFn = *const fn (*anyopaque, *anyopaque, std.mem.Allocator) void;

/// Callback invoked before lending an idle resource to verify health.
pub type ValidateFn = *const fn (*anyopaque, *anyopaque) bool;

/// Handle representing a borrowed resource from ResourcePool.
pub struct PooledHandle {
    slot_id: usize;
    ptr: *anyopaque;
    created_at_ms: u64;
    borrow_count: u64;
}

/// Observable runtime counters and gauges for ResourcePool.
pub struct PoolStats {
    total_created: u64 = 0;
    total_destroyed: u64 = 0;
    active_count: usize = 0;
    idle_count: usize = 0;
    exhausted_rejects: u64 = 0;
    evicted_idle: u64 = 0;
    evicted_lifetime: u64 = 0;
    failed_validations: u64 = 0;
}

enum SlotState {
    Empty,
    Idle,
    Active,
}

struct PoolSlot {
    state: SlotState = .Empty;
    ptr: *anyopaque = undefined;
    created_at_ms: u64 = 0;
    last_used_ms: u64 = 0;
    borrow_count: u64 = 0;
}

/// Bounded, zero-C-dependency resource and connection pool.
pub struct ResourcePool {
    alloc: std.mem.Allocator;
    cfg: PoolConfig;
    user_ctx: *anyopaque;
    create_fn: CreateFn;
    destroy_fn: DestroyFn;
    validate_fn: ValidateFn?;

    slots: []PoolSlot = [];
    idle_buf: []usize = [];
    idle_head: usize = 0;
    idle_len: usize = 0;
    active_count: usize = 0;
    closed: bool = false;

    total_created: u64 = 0;
    total_destroyed: u64 = 0;
    exhausted_rejects: u64 = 0;
    evicted_idle: u64 = 0;
    evicted_lifetime: u64 = 0;
    failed_validations: u64 = 0;

    /// Initialize a bounded resource pool with configuration and lifecycle callbacks.
    pub fn init(
        alloc: std.mem.Allocator,
        cfg: PoolConfig,
        user_ctx: *anyopaque,
        create_fn: CreateFn,
        destroy_fn: DestroyFn,
        validate_fn: ValidateFn?,
    ): ResourcePool ! PoolError {
        if (cfg.max_size == 0 || cfg.min_idle > cfg.max_size) {
            return PoolError.InvalidConfig;
        }
        let slots = alloc.alloc(PoolSlot, cfg.max_size) catch return PoolError.OutOfMemory;
        let idle_buf = alloc.alloc(usize, cfg.max_size) catch {
            alloc.free(slots);
            return PoolError.OutOfMemory;
        };

        let i: usize = 0;
        while (i < cfg.max_size) : (i += 1) {
            slots[i] = PoolSlot{
                state: .Empty,
                ptr: undefined,
                created_at_ms: 0,
                last_used_ms: 0,
                borrow_count: 0,
            };
            idle_buf[i] = 0;
        }

        return ResourcePool{
            alloc: alloc,
            cfg: cfg,
            user_ctx: user_ctx,
            create_fn: create_fn,
            destroy_fn: destroy_fn,
            validate_fn: validate_fn,
            slots: slots,
            idle_buf: idle_buf,
            idle_head: 0,
            idle_len: 0,
            active_count: 0,
            closed: false,
            total_created: 0,
            total_destroyed: 0,
            exhausted_rejects: 0,
            evicted_idle: 0,
            evicted_lifetime: 0,
            failed_validations: 0,
        };
    }

    /// Destroy all idle and active resources and release internal pool buffers.
    pub fn deinit(*_) {
        if (_.closed) return;

        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].state == .Idle || _.slots[i].state == .Active) {
                _.destroy_fn(_.user_ctx, _.slots[i].ptr, _.alloc);
                _.total_destroyed += 1;
                _.slots[i].state = .Empty;
            }
        }

        if (_.slots.len > 0) {
            _.alloc.free(_.slots);
            _.slots = [];
        }
        if (_.idle_buf.len > 0) {
            _.alloc.free(_.idle_buf);
            _.idle_buf = [];
        }

        _.active_count = 0;
        _.idle_head = 0;
        _.idle_len = 0;
        _.closed = true;
    }

    /// Pre-create idle resources up to cfg.min_idle.
    pub fn warmup(*_, now_ms: u64): void ! PoolError {
        if (_.closed) return PoolError.PoolClosed;

        while (_.active_count + _.idle_len < _.cfg.min_idle) {
            const slot_id = _.createSlot(now_ms)?;
            _.slots[slot_id].state = .Idle;
            _.slots[slot_id].last_used_ms = now_ms;
            _.pushIdle(slot_id);
        }
    }

    /// Acquire a resource from the pool, validating and evicting expired idle slots as needed.
    pub fn acquire(*_, now_ms: u64): PooledHandle ! PoolError {
        if (_.closed) return PoolError.PoolClosed;

        while (_.idle_len > 0) {
            const slot_id = _.popIdle();
            if (_.isLifetimeExpired(slot_id, now_ms)) {
                _.destroySlot(slot_id);
                _.evicted_lifetime += 1;
                continue;
            }
            if (_.isIdleExpired(slot_id, now_ms)) {
                _.destroySlot(slot_id);
                _.evicted_idle += 1;
                continue;
            }
            if (vf := _.validate_fn) {
                if (!vf(_.user_ctx, _.slots[slot_id].ptr)) {
                    _.destroySlot(slot_id);
                    _.failed_validations += 1;
                    continue;
                }
            }

            _.slots[slot_id].state = .Active;
            _.slots[slot_id].borrow_count += 1;
            _.active_count += 1;
            return PooledHandle{
                slot_id: slot_id,
                ptr: _.slots[slot_id].ptr,
                created_at_ms: _.slots[slot_id].created_at_ms,
                borrow_count: _.slots[slot_id].borrow_count,
            };
        }

        if (_.active_count + _.idle_len < _.cfg.max_size) {
            const slot_id = _.createSlot(now_ms)?;
            _.slots[slot_id].state = .Active;
            _.slots[slot_id].borrow_count = 1;
            _.active_count += 1;
            return PooledHandle{
                slot_id: slot_id,
                ptr: _.slots[slot_id].ptr,
                created_at_ms: _.slots[slot_id].created_at_ms,
                borrow_count: 1,
            };
        }

        _.exhausted_rejects += 1;
        return PoolError.PoolExhausted;
    }

    /// Return a borrowed handle to the pool, or destroy it if max_lifetime_ms has elapsed.
    pub fn checkin(*_, handle: PooledHandle, now_ms: u64): void ! PoolError {
        if (_.closed) return PoolError.PoolClosed;
        if (!_.isValidActiveHandle(handle)) {
            return PoolError.InvalidHandle;
        }

        _.active_count -= 1;

        if (_.isLifetimeExpired(handle.slot_id, now_ms)) {
            _.destroySlot(handle.slot_id);
            _.evicted_lifetime += 1;
        } else {
            _.slots[handle.slot_id].state = .Idle;
            _.slots[handle.slot_id].last_used_ms = now_ms;
            _.pushIdle(handle.slot_id);
        }
    }

    /// Mark a borrowed handle as broken, destroy its underlying resource, and free its slot.
    pub fn discard(*_, handle: PooledHandle): void ! PoolError {
        if (_.closed) return PoolError.PoolClosed;
        if (!_.isValidActiveHandle(handle)) {
            return PoolError.InvalidHandle;
        }

        _.active_count -= 1;
        _.destroySlot(handle.slot_id);
    }

    /// Proactively evict expired idle resources while replenishing up to cfg.min_idle.
    pub fn evictExpired(*_, now_ms: u64): usize ! PoolError {
        if (_.closed) return PoolError.PoolClosed;

        const orig_len = _.idle_len;
        let evicted: usize = 0;

        // Pop each currently-idle slot from the front; keep non-expired ones by pushing back.
        let i: usize = 0;
        while (i < orig_len) : (i += 1) {
            const slot_id = _.popFrontIdle();
            if (_.isLifetimeExpired(slot_id, now_ms)) {
                _.destroySlot(slot_id);
                _.evicted_lifetime += 1;
                evicted += 1;
            } else if (_.isIdleExpired(slot_id, now_ms)) {
                _.destroySlot(slot_id);
                _.evicted_idle += 1;
                evicted += 1;
            } else {
                _.pushIdle(slot_id);
            }
        }

        // Maintain min_idle floor by replenishing freshly created resources.
        while (_.active_count + _.idle_len < _.cfg.min_idle) {
            const slot_id = _.createSlot(now_ms)?;
            _.slots[slot_id].state = .Idle;
            _.slots[slot_id].last_used_ms = now_ms;
            _.pushIdle(slot_id);
        }

        return evicted;
    }

    /// Return a snapshot of pool statistics.
    pub fn stats(): PoolStats {
        return PoolStats{
            total_created: _.total_created,
            total_destroyed: _.total_destroyed,
            active_count: _.active_count,
            idle_count: _.idle_len,
            exhausted_rejects: _.exhausted_rejects,
            evicted_idle: _.evicted_idle,
            evicted_lifetime: _.evicted_lifetime,
            failed_validations: _.failed_validations,
        };
    }

    fn isValidActiveHandle(handle: PooledHandle): bool {
        if (handle.slot_id >= _.slots.len) return false;
        const slot = _.slots[handle.slot_id];
        return slot.state == .Active && slot.ptr == handle.ptr;
    }

    fn isLifetimeExpired(slot_id: usize, now_ms: u64): bool {
        if (_.cfg.max_lifetime_ms == 0) return false;
        const created = _.slots[slot_id].created_at_ms;
        return now_ms >= created && (now_ms - created) >= _.cfg.max_lifetime_ms;
    }

    fn isIdleExpired(slot_id: usize, now_ms: u64): bool {
        if (_.cfg.idle_timeout_ms == 0) return false;
        const last_used = _.slots[slot_id].last_used_ms;
        return now_ms >= last_used && (now_ms - last_used) >= _.cfg.idle_timeout_ms;
    }

    fn createSlot(*_, now_ms: u64): usize ! PoolError {
        let free_idx: usize? = null;
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            if (_.slots[i].state == .Empty) {
                free_idx = i;
                break;
            }
        }
        if (free_idx == null) return PoolError.PoolExhausted;
        const slot_id = free_idx.?;
        const ptr = _.create_fn(_.user_ctx, _.alloc) catch return PoolError.CreateFailed;
        _.slots[slot_id] = PoolSlot{
            state: .Empty,
            ptr: ptr,
            created_at_ms: now_ms,
            last_used_ms: now_ms,
            borrow_count: 0,
        };
        _.total_created += 1;
        return slot_id;
    }

    fn destroySlot(*_, slot_id: usize) {
        if (_.slots[slot_id].state == .Empty) return;
        _.destroy_fn(_.user_ctx, _.slots[slot_id].ptr, _.alloc);
        _.total_destroyed += 1;
        _.slots[slot_id].state = .Empty;
        _.slots[slot_id].ptr = undefined;
        _.slots[slot_id].created_at_ms = 0;
        _.slots[slot_id].last_used_ms = 0;
        _.slots[slot_id].borrow_count = 0;
    }

    fn pushIdle(*_, slot_id: usize) {
        const cap = _.idle_buf.len;
        const idx = (_.idle_head + _.idle_len) % cap;
        _.idle_buf[idx] = slot_id;
        _.idle_len += 1;
    }

    fn popIdle(*_): usize {
        const cap = _.idle_buf.len;
        if (_.cfg.borrow_order == .Lifo) {
            _.idle_len -= 1;
            const idx = (_.idle_head + _.idle_len) % cap;
            return _.idle_buf[idx];
        } else {
            return _.popFrontIdle();
        }
    }

    fn popFrontIdle(*_): usize {
        const cap = _.idle_buf.len;
        const slot_id = _.idle_buf[_.idle_head];
        _.idle_head = (_.idle_head + 1) % cap;
        _.idle_len -= 1;
        return slot_id;
    }
}

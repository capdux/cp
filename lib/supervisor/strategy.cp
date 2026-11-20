// Source: cp/lib/supervisor/strategy.cp (OTP Supervision Tree & Restart Storm Guard)
// Pure CP OTP-style Supervision Tree state machine with OneForOne/OneForAll/RestForOne
// topology ordering, Permanent/Transient/Temporary policies, and sliding-window restart storm protection.

const std = @import("std");

/// Error set returned by `cap:supervisor` supervision tree and resilience primitives.
pub err SupervisorError {
    ChildNotFound,
    DuplicateChildId,
    InvalidConfig,
    MaxRestartsExceeded,
    BulkheadFull,
    OutOfMemory,
}

/// OTP supervision restart strategy governing sibling impact when a child exits.
pub enum RestartStrategy: u8 {
    OneForOne = 0,
    OneForAll = 1,
    RestForOne = 2,
}

/// Per-child restart policy evaluated against the child's exit reason.
pub enum ChildRestartPolicy: u8 {
    Permanent = 0,
    Transient = 1,
    Temporary = 2,

    /// Returns `true` when `reason` warrants restarting a child with this policy.
    fn shouldRestart(reason: ExitReason): bool {
        switch (_) {
            .Permanent => return true,
            .Transient => return reason == ExitReason.Abnormal,
            .Temporary => return false,
        }
    }
}

/// Reason a supervised child exited.
pub enum ExitReason: u8 {
    Normal = 0,
    Shutdown = 1,
    Abnormal = 2,
}

/// Lifecycle state of a registered child inside a `SupervisorTree`.
pub enum ChildState: u8 {
    Running = 0,
    Stopped = 1,
    Restarting = 2,
    Terminated = 3,

    /// Returns `true` if the child is currently active (`Running` or `Restarting`).
    fn isActive(): bool {
        return _ == .Running || _ == .Restarting;
    }
}

/// Action kind produced by `SupervisorTree.handleChildExit`.
pub enum SupervisionDecisionKind: u8 {
    Ignore = 0,
    RestartChildren = 1,
    EscalateShutdown = 2,
}

/// Specification for registering a child actor/worker in a `SupervisorTree`.
pub struct ChildSpec {
    id: string = "",
    restart_policy: ChildRestartPolicy = ChildRestartPolicy.Permanent,
    shutdown_timeout_ms: u64 = 5000,
}

/// Supervisor intensity and topology configuration.
pub struct SupervisorConfig {
    strategy: RestartStrategy = RestartStrategy.OneForOne,
    max_restarts: u32 = 3,
    period_ms: u64 = 5000,
}

/// Owned supervision plan returned by `SupervisorTree.handleChildExit`.
/// Call `deinit()` when finished inspecting `stop_order` and `restart_order`.
pub struct SupervisionDecision {
    kind: SupervisionDecisionKind = SupervisionDecisionKind.Ignore,
    stop_order: []string = [],
    restart_order: []string = [],
    window_restarts: u32 = 0,
    allocator: std.mem.Allocator = undefined,
    owns_memory: bool = false,

    /// Releases owned `stop_order` and `restart_order` slices and strings.
    fn deinit(*_) {
        if (!_.owns_memory) return;
        let i: usize = 0;
        while (i < _.stop_order.len) : (i += 1) {
            _.allocator.free(_.stop_order[i]);
        }
        if (_.stop_order.len > 0) {
            _.allocator.free(_.stop_order);
        }
        let j: usize = 0;
        while (j < _.restart_order.len) : (j += 1) {
            _.allocator.free(_.restart_order[j]);
        }
        if (_.restart_order.len > 0) {
            _.allocator.free(_.restart_order);
        }
        _.stop_order = [];
        _.restart_order = [];
        _.owns_memory = false;
    }
}

struct ChildEntry {
    id: []u8 = "",
    restart_policy: ChildRestartPolicy = ChildRestartPolicy.Permanent,
    shutdown_timeout_ms: u64 = 5000,
    state: ChildState = ChildState.Running,
    restart_count: u64 = 0,
}

fn freeStringSlice(alloc: std.mem.Allocator, list: []string) {
    let i: usize = 0;
    while (i < list.len) : (i += 1) {
        alloc.free(list[i]);
    }
    if (list.len > 0) {
        alloc.free(list);
    }
}

fn dupeStringSlice(alloc: std.mem.Allocator, src: []const string): ([]string) ! SupervisorError {
    if (src.len == 0) {
        return [];
    }
    let out = alloc.alloc(string, src.len) catch return SupervisorError.OutOfMemory;
    let filled: usize = 0;
    errdefer {
        let k: usize = 0;
        while (k < filled) : (k += 1) {
            alloc.free(out[k]);
        }
        alloc.free(out);
    }
    while (filled < src.len) : (filled += 1) {
        out[filled] = alloc.dupe(u8, src[filled]) catch return SupervisorError.OutOfMemory;
    }
    return out;
}

/// Deterministic Erlang/OTP-style supervision tree state machine with restart-storm protection.
pub struct SupervisorTree {
    allocator: std.mem.Allocator = undefined,
    config: SupervisorConfig = SupervisorConfig{},
    children: std.ArrayList(ChildEntry) = .empty,
    restart_timestamps: std.ArrayList(u64) = .empty,
    escalated: bool = false,

    /// Initializes a `SupervisorTree` with `config`. Rejects `period_ms == 0` with `InvalidConfig`.
    static fn init(alloc: std.mem.Allocator, config: SupervisorConfig): SupervisorTree ! SupervisorError {
        if (config.period_ms == 0) {
            return SupervisorError.InvalidConfig;
        }
        return SupervisorTree{
            allocator: alloc,
            config: config,
            children: .empty,
            restart_timestamps: .empty,
            escalated: false,
        };
    }

    /// Releases all owned child entries and restart window history.
    fn deinit(*_) {
        let i: usize = 0;
        while (i < _.children.items.len) : (i += 1) {
            _.allocator.free(_.children.items[i].id);
        }
        _.children.deinit(_.allocator);
        _.restart_timestamps.deinit(_.allocator);
    }

    fn findIndex(id: string): usize? {
        let i: usize = 0;
        while (i < _.children.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.children.items[i].id, id)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a child in startup dependency order (`Running` initially).
    /// Returns `InvalidConfig` on empty `spec.id` or `DuplicateChildId` if `spec.id` already exists.
    fn addChild(*_, spec: ChildSpec): void ! SupervisorError {
        if (spec.id.len == 0) {
            return SupervisorError.InvalidConfig;
        }
        if (_.findIndex(spec.id) != null) {
            return SupervisorError.DuplicateChildId;
        }
        const owned_id = _.allocator.dupe(u8, spec.id) catch return SupervisorError.OutOfMemory;
        _.children.append(_.allocator, ChildEntry{
            id: owned_id,
            restart_policy: spec.restart_policy,
            shutdown_timeout_ms: spec.shutdown_timeout_ms,
            state: ChildState.Running,
            restart_count: 0,
        }) catch {
            _.allocator.free(owned_id);
            return SupervisorError.OutOfMemory;
        };
    }

    /// Removes a child by `id` while preserving the relative dependency order of remaining children.
    fn removeChild(*_, id: string): void ! SupervisorError {
        const idx_opt = _.findIndex(id);
        if (idx_opt == null) {
            return SupervisorError.ChildNotFound;
        }
        const idx = idx_opt.?;
        const removed = _.children.orderedRemove(idx);
        _.allocator.free(removed.id);
    }

    /// Returns the current lifecycle state of `id`, or `null` if not registered.
    fn childState(id: string): ChildState? {
        const idx = _.findIndex(id) ?? return null;
        return _.children.items[idx].state;
    }

    /// Returns the cumulative number of restarts applied to child `id`, or `null` if not registered.
    fn childRestartCount(id: string): u64? {
        const idx = _.findIndex(id) ?? return null;
        return _.children.items[idx].restart_count;
    }

    /// Returns the number of registered children currently in `ChildState.Running`.
    fn runningCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.children.items.len) : (i += 1) {
            if (_.children.items[i].state == ChildState.Running) {
                count += 1;
            }
        }
        return count;
    }

    /// Returns total registered children count.
    fn childCount(): usize {
        return _.children.items.len;
    }

    /// Returns `true` if restart intensity was exceeded and the supervisor escalated shutdown.
    fn isEscalated(): bool {
        return _.escalated;
    }

    fn pruneWindow(*_, now_ms: u64) {
        const window_start: u64 = if (now_ms >= _.config.period_ms) now_ms - _.config.period_ms else 0;
        let write_idx: usize = 0;
        let read_idx: usize = 0;
        while (read_idx < _.restart_timestamps.items.len) : (read_idx += 1) {
            const ts = _.restart_timestamps.items[read_idx];
            if (ts >= window_start && ts <= now_ms) {
                _.restart_timestamps.items[write_idx] = ts;
                write_idx += 1;
            }
        }
        _.restart_timestamps.items.len = write_idx;
    }

    /// Returns the number of restarts still within `[now_ms - period_ms, now_ms]`.
    fn windowRestartCount(*_, now_ms: u64): u32 {
        _.pruneWindow(now_ms);
        return _.restart_timestamps.items.len as u32;
    }

    /// Evaluates child exit `(id, reason)` at `now_ms`, updates child states,
    /// enforces the `[now_ms - period_ms, now_ms]` restart-intensity window, and returns the exact
    /// reverse stop order and forward restart order.
    fn handleChildExit(*_, id: string, reason: ExitReason, now_ms: u64): SupervisionDecision ! SupervisorError {
        if (_.escalated) {
            return SupervisorError.MaxRestartsExceeded;
        }
        const idx_opt = _.findIndex(id);
        if (idx_opt == null) {
            return SupervisorError.ChildNotFound;
        }
        const exit_idx = idx_opt.?;

        _.pruneWindow(now_ms);

        const should_restart = _.children.items[exit_idx].restart_policy.shouldRestart(reason);
        if (!should_restart) {
            _.children.items[exit_idx].state = ChildState.Terminated;
            return SupervisionDecision{
                kind: SupervisionDecisionKind.Ignore,
                stop_order: [],
                restart_order: [],
                window_restarts: _.restart_timestamps.items.len as u32,
                allocator: _.allocator,
                owns_memory: false,
            };
        }

        const cur_restarts: u32 = _.restart_timestamps.items.len as u32;
        if (cur_restarts >= _.config.max_restarts) {
            // Restart storm threshold exceeded: collect all active siblings in reverse order and escalate.
            let stop_tmp: std.ArrayList(string) = .empty;
            defer stop_tmp.deinit(_.allocator);

            let rev: usize = _.children.items.len;
            while (rev > 0) {
                rev -= 1;
                if (rev != exit_idx && _.children.items[rev].state.isActive()) {
                    stop_tmp.append(_.allocator, _.children.items[rev].id) catch return SupervisorError.OutOfMemory;
                }
            }
            const owned_stops = dupeStringSlice(_.allocator, stop_tmp.items)?;

            _.children.items[exit_idx].state = ChildState.Terminated;
            let k: usize = 0;
            while (k < _.children.items.len) : (k += 1) {
                if (k != exit_idx && _.children.items[k].state.isActive()) {
                    _.children.items[k].state = ChildState.Stopped;
                }
            }
            _.escalated = true;

            return SupervisionDecision{
                kind: SupervisionDecisionKind.EscalateShutdown,
                stop_order: owned_stops,
                restart_order: [],
                window_restarts: cur_restarts + 1,
                allocator: _.allocator,
                owns_memory: owned_stops.len > 0,
            };
        }

        // Plan stop_order (reverse registration) and restart_order (forward registration).
        let stop_tmp: std.ArrayList(string) = .empty;
        defer stop_tmp.deinit(_.allocator);
        let restart_tmp: std.ArrayList(string) = .empty;
        defer restart_tmp.deinit(_.allocator);

        switch (_.config.strategy) {
            .OneForOne => {
                restart_tmp.append(_.allocator, _.children.items[exit_idx].id) catch return SupervisorError.OutOfMemory;
            },
            .OneForAll => {
                let rev: usize = _.children.items.len;
                while (rev > 0) {
                    rev -= 1;
                    if (rev != exit_idx && _.children.items[rev].state.isActive()) {
                        stop_tmp.append(_.allocator, _.children.items[rev].id) catch return SupervisorError.OutOfMemory;
                    }
                }
                let fwd: usize = 0;
                while (fwd < _.children.items.len) : (fwd += 1) {
                    if (fwd == exit_idx) {
                        restart_tmp.append(_.allocator, _.children.items[fwd].id) catch return SupervisorError.OutOfMemory;
                    } else if (_.children.items[fwd].state.isActive() && _.children.items[fwd].restart_policy != ChildRestartPolicy.Temporary) {
                        restart_tmp.append(_.allocator, _.children.items[fwd].id) catch return SupervisorError.OutOfMemory;
                    }
                }
            },
            .RestForOne => {
                let rev: usize = _.children.items.len;
                while (rev > exit_idx + 1) {
                    rev -= 1;
                    if (_.children.items[rev].state.isActive()) {
                        stop_tmp.append(_.allocator, _.children.items[rev].id) catch return SupervisorError.OutOfMemory;
                    }
                }
                let fwd: usize = exit_idx;
                while (fwd < _.children.items.len) : (fwd += 1) {
                    if (fwd == exit_idx) {
                        restart_tmp.append(_.allocator, _.children.items[fwd].id) catch return SupervisorError.OutOfMemory;
                    } else if (_.children.items[fwd].state.isActive() && _.children.items[fwd].restart_policy != ChildRestartPolicy.Temporary) {
                        restart_tmp.append(_.allocator, _.children.items[fwd].id) catch return SupervisorError.OutOfMemory;
                    }
                }
            },
        }

        const owned_stops = dupeStringSlice(_.allocator, stop_tmp.items)?;
        errdefer freeStringSlice(_.allocator, owned_stops);

        const owned_restarts = dupeStringSlice(_.allocator, restart_tmp.items)?;
        errdefer freeStringSlice(_.allocator, owned_restarts);

        _.restart_timestamps.append(_.allocator, now_ms) catch return SupervisorError.OutOfMemory;

        // Apply state transitions now that all allocations succeeded.
        switch (_.config.strategy) {
            .OneForOne => {
                _.children.items[exit_idx].state = ChildState.Running;
                _.children.items[exit_idx].restart_count += 1;
            },
            .OneForAll => {
                let fwd: usize = 0;
                while (fwd < _.children.items.len) : (fwd += 1) {
                    if (fwd == exit_idx) {
                        _.children.items[fwd].state = ChildState.Running;
                        _.children.items[fwd].restart_count += 1;
                    } else if (_.children.items[fwd].state.isActive()) {
                        if (_.children.items[fwd].restart_policy == ChildRestartPolicy.Temporary) {
                            _.children.items[fwd].state = ChildState.Terminated;
                        } else {
                            _.children.items[fwd].state = ChildState.Running;
                            _.children.items[fwd].restart_count += 1;
                        }
                    }
                }
            },
            .RestForOne => {
                let fwd: usize = exit_idx;
                while (fwd < _.children.items.len) : (fwd += 1) {
                    if (fwd == exit_idx) {
                        _.children.items[fwd].state = ChildState.Running;
                        _.children.items[fwd].restart_count += 1;
                    } else if (_.children.items[fwd].state.isActive()) {
                        if (_.children.items[fwd].restart_policy == ChildRestartPolicy.Temporary) {
                            _.children.items[fwd].state = ChildState.Terminated;
                        } else {
                            _.children.items[fwd].state = ChildState.Running;
                            _.children.items[fwd].restart_count += 1;
                        }
                    }
                }
            },
        }

        return SupervisionDecision{
            kind: SupervisionDecisionKind.RestartChildren,
            stop_order: owned_stops,
            restart_order: owned_restarts,
            window_restarts: _.restart_timestamps.items.len as u32,
            allocator: _.allocator,
            owns_memory: owned_stops.len > 0 || owned_restarts.len > 0,
        };
    }
}

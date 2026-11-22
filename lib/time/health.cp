// Source: cp/lib/health/probe.cp (Health Check Probes & Graceful Shutdown Coordinator)
// Pure CP zero-C-dependency Kubernetes-grade Liveness/Readiness/Startup probe aggregator
// and priority-ordered idempotent ShutdownCoordinator.

const std = @import("std");

/// Error set for HealthRegistry and ShutdownCoordinator operations.
pub err HealthError {
    DuplicateCheck,
    CheckNotFound,
    DuplicateHook,
    AlreadyShuttingDown,
    OutOfMemory,
}

/// Probe classification aligned with Kubernetes container lifecycle probes.
pub enum ProbeKind {
    Liveness,
    Readiness,
    Startup,
    All,

    fn asStr(): string {
        switch (_) {
            .Liveness => return "liveness",
            .Readiness => return "readiness",
            .Startup => return "startup",
            .All => return "all",
        }
    }
}

/// Tri-state health status of an individual check or aggregated probe report.
pub enum HealthStatus {
    Up,
    Degraded,
    Down,

    fn asStr(): string {
        switch (_) {
            .Up => return "up",
            .Degraded => return "degraded",
            .Down => return "down",
        }
    }
}

/// Result returned by an individual health check callback.
pub struct CheckOutcome {
    status: HealthStatus = .Up;
    message: string = "";
    latency_ms: u64 = 0;
}

/// Aggregated probe report with HTTP status mapping and RFC 8259 JSON details.
pub struct ProbeReport {
    alloc: std.mem.Allocator;
    overall: HealthStatus;
    http_status: u16;
    passing_count: usize = 0;
    degraded_count: usize = 0;
    failing_count: usize = 0;
    details_json: string;

    /// Free the owned JSON report buffer.
    pub fn deinit(*_) {
        if (_.details_json.len > 0) {
            _.alloc.free(_.details_json);
            _.details_json = "";
        }
    }
}

/// Callback signature for a health check probe.
pub type CheckFn = *const fn (*anyopaque) CheckOutcome;

/// Callback signature for a graceful shutdown hook.
pub type ShutdownHookFn = *const fn (*anyopaque) void;

struct CheckEntry {
    name: string;
    kind: ProbeKind;
    critical: bool;
    check_fn: CheckFn;
    ctx: *anyopaque;
}

struct HookEntry {
    name: string;
    priority: i32;
    seq: usize;
    hook_fn: ShutdownHookFn;
    ctx: *anyopaque;
}

struct JsonBuf {
    alloc: std.mem.Allocator;
    buf: []u8 = [];
    len: usize = 0;

    fn deinit(*_) {
        if (_.buf.len > 0) {
            _.alloc.free(_.buf);
            _.buf = [];
            _.len = 0;
        }
    }

    fn ensureCap(*_, extra: usize): void ! HealthError {
        const needed = _.len + extra;
        if (needed > _.buf.len) {
            let new_cap: usize = if (_.buf.len == 0) 256 else _.buf.len * 2;
            while (new_cap < needed) {
                new_cap *= 2;
            }
            const new_buf = _.alloc.alloc(u8, new_cap) catch return HealthError.OutOfMemory;
            if (_.len > 0) {
                std.mem.copyForwards(u8, new_buf[0.._.len], _.buf[0.._.len]);
            }
            if (_.buf.len > 0) {
                _.alloc.free(_.buf);
            }
            _.buf = new_buf;
        }
    }

    fn appendSlice(*_, s: []const u8): void ! HealthError {
        if (s.len > 0) {
            _.ensureCap(s.len)?;
            std.mem.copyForwards(u8, _.buf[_.len..(_.len + s.len)], s);
            _.len += s.len;
        }
    }

    fn appendByte(*_, b: u8): void ! HealthError {
        _.ensureCap(1)?;
        _.buf[_.len] = b;
        _.len += 1;
    }

    fn appendBool(*_, v: bool): void ! HealthError {
        if (v) {
            _.appendSlice("true")?;
        } else {
            _.appendSlice("false")?;
        }
    }

    fn appendU64(*_, val: u64): void ! HealthError {
        if (val == 0) {
            _.appendByte('0')?;
        } else {
            let tmp: [24]u8 = [0; 24];
            let v = val;
            let idx: usize = 24;
            while (v > 0) {
                idx -= 1;
                const digit = (v % 10) as u8;
                tmp[idx] = ('0' as u8) + digit;
                v /= 10;
            }
            _.appendSlice(tmp[idx..24])?;
        }
    }

    fn appendJsonString(*_, s: string): void ! HealthError {
        const hex_digits = "0123456789abcdef";
        _.appendByte('"')?;
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const c = s[i];
            switch (c) {
                '"' => _.appendSlice("\\\"")?,
                '\\' => _.appendSlice("\\\\")?,
                '\n' => _.appendSlice("\\n")?,
                '\r' => _.appendSlice("\\r")?,
                '\t' => _.appendSlice("\\t")?,
                0x08 => _.appendSlice("\\b")?,
                0x0c => _.appendSlice("\\f")?,
                else => {
                    if (c < 0x20) {
                        _.appendSlice("\\u00")?;
                        _.appendByte(hex_digits[(c >> 4) & 0x0f])?;
                        _.appendByte(hex_digits[c & 0x0f])?;
                    } else {
                        _.appendByte(c)?;
                    }
                },
            }
        }
        _.appendByte('"')?;
    }

    fn toOwnedSlice(*_): string ! HealthError {
        const out = _.alloc.alloc(u8, _.len) catch return HealthError.OutOfMemory;
        if (_.len > 0) {
            std.mem.copyForwards(u8, out, _.buf[0.._.len]);
        }
        _.deinit();
        return out;
    }
}

/// Registry for Liveness, Readiness, and Startup health check probes with traffic draining.
pub struct HealthRegistry {
    alloc: std.mem.Allocator;
    checks: []CheckEntry = [];
    checks_len: usize = 0;
    draining: bool = false;

    /// Initialize an empty HealthRegistry.
    pub fn init(alloc: std.mem.Allocator): HealthRegistry {
        return HealthRegistry{
            alloc: alloc,
            checks: [],
            checks_len: 0,
            draining: false,
        };
    }

    /// Release all registered check names and internal storage.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.checks_len) : (i += 1) {
            if (_.checks[i].name.len > 0) {
                _.alloc.free(_.checks[i].name);
            }
        }
        if (_.checks.len > 0) {
            _.alloc.free(_.checks);
        }
        _.checks = [];
        _.checks_len = 0;
    }

    /// Register a named health check for a specific probe kind (or ProbeKind.All).
    pub fn registerCheck(
        *_,
        name: string,
        kind: ProbeKind,
        critical: bool,
        check_fn: CheckFn,
        ctx: *anyopaque,
    ): void ! HealthError {
        let i: usize = 0;
        while (i < _.checks_len) : (i += 1) {
            if (std.mem.eql(u8, _.checks[i].name, name)) {
                return HealthError.DuplicateCheck;
            }
        }

        if (_.checks_len == _.checks.len) {
            const new_cap: usize = if (_.checks.len == 0) 8 else _.checks.len * 2;
            const new_arr = _.alloc.alloc(CheckEntry, new_cap) catch return HealthError.OutOfMemory;
            if (_.checks_len > 0) {
                std.mem.copyForwards(CheckEntry, new_arr[0.._.checks_len], _.checks[0.._.checks_len]);
                _.alloc.free(_.checks);
            }
            _.checks = new_arr;
        }

        const owned_name = _.alloc.dupe(u8, name) catch return HealthError.OutOfMemory;
        _.checks[_.checks_len] = CheckEntry{
            name: owned_name,
            kind: kind,
            critical: critical,
            check_fn: check_fn,
            ctx: ctx,
        };
        _.checks_len += 1;
    }

    /// Remove a previously registered check by name.
    pub fn unregisterCheck(*_, name: string): void ! HealthError {
        let i: usize = 0;
        let found = false;
        while (i < _.checks_len) : (i += 1) {
            if (std.mem.eql(u8, _.checks[i].name, name)) {
                if (_.checks[i].name.len > 0) {
                    _.alloc.free(_.checks[i].name);
                }
                let j = i;
                while (j + 1 < _.checks_len) : (j += 1) {
                    _.checks[j] = _.checks[j + 1];
                }
                _.checks_len -= 1;
                found = true;
                break;
            }
        }
        if (!found) {
            return HealthError.CheckNotFound;
        }
    }

    /// Toggle traffic draining mode. Readiness immediately reports Down (503) while Liveness stays unaffected.
    pub fn setDraining(*_, draining: bool) {
        _.draining = draining;
    }

    /// Return whether the registry is currently in traffic-draining mode.
    pub fn isDraining(): bool {
        return _.draining;
    }

    /// Return total number of registered checks.
    pub fn checkCount(): usize {
        return _.checks_len;
    }

    /// Evaluate all checks matching `kind` (plus ProbeKind.All wildcards) and emit a ProbeReport.
    pub fn evaluate(kind: ProbeKind): ProbeReport ! HealthError {
        let passing: usize = 0;
        let degraded: usize = 0;
        let failing: usize = 0;
        let has_critical_down = false;
        let has_non_critical_down = false;
        let has_degraded = false;

        let jb = JsonBuf{ alloc: _.alloc };
        errdefer jb.deinit();

        jb.appendSlice("{\"probe\":")?;
        jb.appendJsonString(kind.asStr())?;
        jb.appendSlice(",\"draining\":")?;
        jb.appendBool(_.draining)?;
        jb.appendSlice(",\"checks\":[")?;

        let first = true;
        let i: usize = 0;
        while (i < _.checks_len) : (i += 1) {
            const entry = _.checks[i];
            const matches = (entry.kind == kind) || (entry.kind == .All) || (kind == .All);
            if (!matches) continue;

            const outcome = entry.check_fn(entry.ctx);
            switch (outcome.status) {
                .Up => {
                    passing += 1;
                },
                .Degraded => {
                    degraded += 1;
                    has_degraded = true;
                },
                .Down => {
                    failing += 1;
                    if (entry.critical) {
                        has_critical_down = true;
                    } else {
                        has_non_critical_down = true;
                    }
                },
            }

            if (!first) {
                jb.appendByte(',')?;
            }
            first = false;

            jb.appendSlice("{\"name\":")?;
            jb.appendJsonString(entry.name)?;
            jb.appendSlice(",\"kind\":")?;
            jb.appendJsonString(entry.kind.asStr())?;
            jb.appendSlice(",\"critical\":")?;
            jb.appendBool(entry.critical)?;
            jb.appendSlice(",\"status\":")?;
            jb.appendJsonString(outcome.status.asStr())?;
            jb.appendSlice(",\"latency_ms\":")?;
            jb.appendU64(outcome.latency_ms)?;
            jb.appendSlice(",\"message\":")?;
            jb.appendJsonString(outcome.message)?;
            jb.appendByte('}')?;
        }

        const is_draining_readiness = _.draining && (kind == .Readiness);
        let overall: HealthStatus = .Up;
        let http_status: u16 = 200;
        if (is_draining_readiness || has_critical_down) {
            overall = .Down;
            http_status = 503;
        } else if (has_non_critical_down || has_degraded) {
            overall = .Degraded;
            http_status = 200;
        }

        jb.appendSlice("],\"status\":")?;
        jb.appendJsonString(overall.asStr())?;
        jb.appendSlice(",\"http_status\":")?;
        jb.appendU64(http_status)?;
        jb.appendSlice(",\"passing\":")?;
        jb.appendU64(passing)?;
        jb.appendSlice(",\"degraded\":")?;
        jb.appendU64(degraded)?;
        jb.appendSlice(",\"failing\":")?;
        jb.appendU64(failing)?;
        jb.appendByte('}')?;

        const json_out = jb.toOwnedSlice()?;
        return ProbeReport{
            alloc: _.alloc,
            overall: overall,
            http_status: http_status,
            passing_count: passing,
            degraded_count: degraded,
            failing_count: failing,
            details_json: json_out,
        };
    }
}

/// Priority-ordered, strictly idempotent graceful shutdown hook coordinator.
pub struct ShutdownCoordinator {
    alloc: std.mem.Allocator;
    hooks: []HookEntry = [];
    hooks_len: usize = 0;
    executed: []string = [];
    executed_len: usize = 0;
    shutting_down: bool = false;
    next_seq: usize = 0;

    /// Initialize an empty ShutdownCoordinator.
    pub fn init(alloc: std.mem.Allocator): ShutdownCoordinator {
        return ShutdownCoordinator{
            alloc: alloc,
            hooks: [],
            hooks_len: 0,
            executed: [],
            executed_len: 0,
            shutting_down: false,
            next_seq: 0,
        };
    }

    /// Release all hook names and internal buffers.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.hooks_len) : (i += 1) {
            if (_.hooks[i].name.len > 0) {
                _.alloc.free(_.hooks[i].name);
            }
        }
        if (_.hooks.len > 0) {
            _.alloc.free(_.hooks);
        }
        if (_.executed.len > 0) {
            _.alloc.free(_.executed);
        }
        _.hooks = [];
        _.hooks_len = 0;
        _.executed = [];
        _.executed_len = 0;
    }

    /// Register a named shutdown hook with a priority (higher priority runs first).
    pub fn registerHook(
        *_,
        name: string,
        priority: i32,
        hook_fn: ShutdownHookFn,
        ctx: *anyopaque,
    ): void ! HealthError {
        if (_.shutting_down) {
            return HealthError.AlreadyShuttingDown;
        }
        let i: usize = 0;
        while (i < _.hooks_len) : (i += 1) {
            if (std.mem.eql(u8, _.hooks[i].name, name)) {
                return HealthError.DuplicateHook;
            }
        }

        if (_.hooks_len == _.hooks.len) {
            const new_cap: usize = if (_.hooks.len == 0) 8 else _.hooks.len * 2;
            const new_arr = _.alloc.alloc(HookEntry, new_cap) catch return HealthError.OutOfMemory;
            if (_.hooks_len > 0) {
                std.mem.copyForwards(HookEntry, new_arr[0.._.hooks_len], _.hooks[0.._.hooks_len]);
                _.alloc.free(_.hooks);
            }
            _.hooks = new_arr;
        }

        const owned_name = _.alloc.dupe(u8, name) catch return HealthError.OutOfMemory;
        _.hooks[_.hooks_len] = HookEntry{
            name: owned_name,
            priority: priority,
            seq: _.next_seq,
            hook_fn: hook_fn,
            ctx: ctx,
        };
        _.hooks_len += 1;
        _.next_seq += 1;
    }

    /// Trigger graceful shutdown once, executing all hooks in descending priority order.
    /// Subsequent calls are strict no-ops returning 0.
    pub fn triggerShutdown(*_): usize {
        if (_.shutting_down) {
            return 0;
        }
        _.shutting_down = true;
        if (_.hooks_len == 0) {
            return 0;
        }

        // Insertion sort by priority descending, then seq ascending for determinism
        let i: usize = 1;
        while (i < _.hooks_len) : (i += 1) {
            const key = _.hooks[i];
            let j: usize = i;
            while (j > 0) {
                const prev = _.hooks[j - 1];
                const should_swap = (prev.priority < key.priority) ||
                    (prev.priority == key.priority && prev.seq > key.seq);
                if (!should_swap) break;
                _.hooks[j] = prev;
                j -= 1;
            }
            _.hooks[j] = key;
        }

        if (_.executed.len < _.hooks_len) {
            const exec_buf = _.alloc.alloc(string, _.hooks_len) catch {
                // Fallback: still execute all hooks even if tracking buffer fails
                let k: usize = 0;
                while (k < _.hooks_len) : (k += 1) {
                    _.hooks[k].hook_fn(_.hooks[k].ctx);
                }
                return _.hooks_len;
            };
            if (_.executed.len > 0) {
                _.alloc.free(_.executed);
            }
            _.executed = exec_buf;
        }

        let k: usize = 0;
        while (k < _.hooks_len) : (k += 1) {
            _.hooks[k].hook_fn(_.hooks[k].ctx);
            _.executed[k] = _.hooks[k].name;
        }
        _.executed_len = _.hooks_len;
        return _.hooks_len;
    }

    /// Return the slice of hook names in the exact order they were executed.
    pub fn executedOrder(): []const string {
        return _.executed[0.._.executed_len];
    }

    /// Return whether shutdown has already been triggered.
    pub fn isShuttingDown(): bool {
        return _.shutting_down;
    }

    /// Return total number of registered shutdown hooks.
    pub fn hookCount(): usize {
        return _.hooks_len;
    }
}

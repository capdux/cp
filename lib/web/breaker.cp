// Source: cp/lib/circuit/breaker.cp (Circuit Breaker State Machine & Sliding Window)
// Pure CP resilience circuit breaker with ring-buffer sliding failure-rate window,
// deterministic monotonic millisecond clock injection, and HalfOpen probe gating.

const std = @import("std");

/// Error set for resilience circuit breaker, backoff policy, and bulkhead operations.
pub err CircuitError {
    CircuitOpen,
    BulkheadFull,
    InvalidConfig,
    OutOfMemory,
}

/// Lifecycle state of a `CircuitBreaker`.
pub enum CircuitState: u8 {
    Closed = 0,
    Open = 1,
    HalfOpen = 2,
}

/// Configuration parameters for `CircuitBreaker`.
pub struct CircuitConfig {
    failure_threshold_rate: f64 = 0.5,
    minimum_calls: usize = 5,
    window_size: usize = 10,
    open_duration_ms: i64 = 30000,
    half_open_max_calls: usize = 3,
    half_open_success_threshold: usize = 2,
}

/// Point-in-time telemetry snapshot returned by `CircuitBreaker.stats`.
pub struct CircuitStats {
    state: CircuitState,
    total_calls: u64,
    window_calls: usize,
    window_failures: usize,
    failure_rate: f64,
    opened_count: u64,
    rejected_count: u64,
}

/// Ring-buffer sliding-window Circuit Breaker with deterministic time injection.
pub struct CircuitBreaker {
    alloc: std.mem.Allocator,
    config: CircuitConfig,
    state: CircuitState,
    outcomes: []u8,
    window_head: usize,
    window_calls: usize,
    window_failures: usize,
    opened_at_ms: i64,
    half_open_calls: usize,
    half_open_successes: usize,
    total_calls: u64,
    opened_count: u64,
    rejected_count: u64,

    /// Validates `config` and allocates the sliding-window ring buffer.
    pub static fn init(alloc: std.mem.Allocator, config: CircuitConfig): CircuitBreaker ! CircuitError {
        if (config.window_size == 0 ||
            config.minimum_calls == 0 ||
            config.minimum_calls > config.window_size ||
            config.open_duration_ms < 0 ||
            config.half_open_max_calls == 0 ||
            config.half_open_success_threshold == 0 ||
            std.math.isNan(config.failure_threshold_rate) ||
            config.failure_threshold_rate <= 0.0 ||
            config.failure_threshold_rate > 1.0)
        {
            return CircuitError.InvalidConfig;
        }
        const outcomes = alloc.alloc(u8, config.window_size) catch return CircuitError.OutOfMemory;
        @memset(outcomes, 0);
        return CircuitBreaker{
            alloc: alloc,
            config: config,
            state: .Closed,
            outcomes: outcomes,
            window_head: 0,
            window_calls: 0,
            window_failures: 0,
            opened_at_ms: 0,
            half_open_calls: 0,
            half_open_successes: 0,
            total_calls: 0,
            opened_count: 0,
            rejected_count: 0,
        };
    }

    /// Releases the sliding-window ring buffer.
    pub fn deinit(*_) {
        if (_.outcomes.len > 0) {
            _.alloc.free(_.outcomes);
            _.outcomes = [];
        }
        _.window_head = 0;
        _.window_calls = 0;
        _.window_failures = 0;
    }

    fn maybeAdvanceFromOpen(*_, now_ms: i64) {
        if (_.state == .Open && (now_ms - _.opened_at_ms) >= _.config.open_duration_ms) {
            _.state = .HalfOpen;
            _.half_open_calls = 0;
            _.half_open_successes = 0;
        }
    }

    fn pushWindowOutcome(*_, failed: u8) {
        const cap = _.config.window_size;
        if (_.window_calls == cap) {
            _.window_failures -= _.outcomes[_.window_head] as usize;
        } else {
            _.window_calls += 1;
        }
        _.outcomes[_.window_head] = failed;
        _.window_failures += failed as usize;
        _.window_head = (_.window_head + 1) % cap;
    }

    fn clearWindow(*_) {
        if (_.outcomes.len > 0) {
            @memset(_.outcomes, 0);
        }
        _.window_head = 0;
        _.window_calls = 0;
        _.window_failures = 0;
    }

    fn tripToOpen(*_, now_ms: i64) {
        if (_.state != .Open) {
            _.opened_count += 1;
        }
        _.state = .Open;
        _.opened_at_ms = now_ms;
        _.half_open_calls = 0;
        _.half_open_successes = 0;
    }

    /// Checks whether a request is permitted at `now_ms`, advancing `Open -> HalfOpen` when elapsed.
    pub fn allowRequest(*_, now_ms: i64): bool {
        _.maybeAdvanceFromOpen(now_ms);
        if (_.state == .Closed) {
            return true;
        }
        if (_.state == .HalfOpen) {
            if (_.half_open_calls < _.config.half_open_max_calls) {
                _.half_open_calls += 1;
                return true;
            }
            _.rejected_count += 1;
            return false;
        }
        _.rejected_count += 1;
        return false;
    }

    /// Records a successful call at `now_ms`.
    pub fn recordSuccess(*_, now_ms: i64) {
        _.maybeAdvanceFromOpen(now_ms);
        _.total_calls += 1;
        if (_.state == .Closed) {
            _.pushWindowOutcome(0);
            return;
        }
        if (_.state == .HalfOpen) {
            _.half_open_successes += 1;
            if (_.half_open_successes >= _.config.half_open_success_threshold) {
                _.state = .Closed;
                _.clearWindow();
                _.half_open_calls = 0;
                _.half_open_successes = 0;
            }
        }
    }

    /// Records a failed call at `now_ms`, tripping to `Open` when threshold is reached.
    pub fn recordFailure(*_, now_ms: i64) {
        _.maybeAdvanceFromOpen(now_ms);
        _.total_calls += 1;
        if (_.state == .Closed) {
            _.pushWindowOutcome(1);
            if (_.window_calls >= _.config.minimum_calls) {
                const rate = (_.window_failures as f64) / (_.window_calls as f64);
                if (rate >= _.config.failure_threshold_rate) {
                    _.tripToOpen(now_ms);
                }
            }
            return;
        }
        if (_.state == .HalfOpen) {
            _.tripToOpen(now_ms);
        }
    }

    /// Manually forces the breaker into `Open` state at `now_ms`.
    pub fn forceOpen(*_, now_ms: i64) {
        _.tripToOpen(now_ms);
    }

    /// Manually forces the breaker into `Closed` state and clears the sliding window.
    pub fn forceClose(*_) {
        _.state = .Closed;
        _.clearWindow();
        _.half_open_calls = 0;
        _.half_open_successes = 0;
    }

    /// Resets all state, sliding-window history, and telemetry counters to initial values.
    pub fn reset(*_) {
        _.state = .Closed;
        _.clearWindow();
        _.opened_at_ms = 0;
        _.half_open_calls = 0;
        _.half_open_successes = 0;
        _.total_calls = 0;
        _.opened_count = 0;
        _.rejected_count = 0;
    }

    /// Returns a point-in-time telemetry snapshot at `now_ms`.
    pub fn stats(*_, now_ms: i64): CircuitStats {
        _.maybeAdvanceFromOpen(now_ms);
        const rate: f64 = if (_.window_calls == 0)
            0.0
        else
            (_.window_failures as f64) / (_.window_calls as f64);
        return CircuitStats{
            state: _.state,
            total_calls: _.total_calls,
            window_calls: _.window_calls,
            window_failures: _.window_failures,
            failure_rate: rate,
            opened_count: _.opened_count,
            rejected_count: _.rejected_count,
        };
    }
}

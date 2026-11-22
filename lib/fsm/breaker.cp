// Source: cp/lib/fsm/circuit_breaker.cp (Resilience Circuit Breaker: Closed/Open/HalfOpen)
// Pure CP Deterministic Circuit Breaker for fault-tolerant AI Agent tool & RPC calls.

pub enum CircuitState: u8 {
    Closed = 0,
    Open = 1,
    HalfOpen = 2,
}

pub struct CircuitBreakerConfig {
    failure_threshold: u32 = 5,
    success_threshold_half_open: u32 = 2,
    open_timeout_ms: i64 = 30000,
    half_open_max_probes: u32 = 1,
}

pub struct CircuitStats {
    state: CircuitState = CircuitState.Closed,
    consecutive_failures: u32 = 0,
    consecutive_successes: u32 = 0,
    half_open_in_flight: u32 = 0,
    total_requests: u64 = 0,
    total_allowed: u64 = 0,
    total_rejected: u64 = 0,
    total_successes: u64 = 0,
    total_failures: u64 = 0,
    open_count: u64 = 0,
    last_failure_ms: i64 = -1,
    opened_at_ms: i64 = -1,
}

pub struct CircuitBreaker {
    config: CircuitBreakerConfig = CircuitBreakerConfig{},
    cur_state: CircuitState = CircuitState.Closed,
    consecutive_failures: u32 = 0,
    consecutive_successes: u32 = 0,
    half_open_in_flight: u32 = 0,
    total_requests: u64 = 0,
    total_allowed: u64 = 0,
    total_rejected: u64 = 0,
    total_successes: u64 = 0,
    total_failures: u64 = 0,
    open_count: u64 = 0,
    last_failure_ms: i64 = -1,
    opened_at_ms: i64 = -1,

    static fn init(config: CircuitBreakerConfig): CircuitBreaker {
        return CircuitBreaker{
            config: config,
        };
    }

    static fn initDefault(): CircuitBreaker {
        return CircuitBreaker{
            config: CircuitBreakerConfig{},
        };
    }

    fn refreshState(*_, now_ms: i64) {
        if (_.cur_state == CircuitState.Open) {
            const timeout = if (_.config.open_timeout_ms < 0) 0 else _.config.open_timeout_ms;
            if (_.opened_at_ms >= 0 && now_ms >= _.opened_at_ms && (now_ms - _.opened_at_ms) >= timeout) {
                _.cur_state = CircuitState.HalfOpen;
                _.consecutive_failures = 0;
                _.consecutive_successes = 0;
                _.half_open_in_flight = 0;
            }
        }
    }

    fn tripOpen(*_, now_ms: i64) {
        _.cur_state = CircuitState.Open;
        _.opened_at_ms = now_ms;
        _.consecutive_failures = 0;
        _.consecutive_successes = 0;
        _.half_open_in_flight = 0;
        _.open_count += 1;
    }

    fn state(*_, now_ms: i64): CircuitState {
        _.refreshState(now_ms);
        return _.cur_state;
    }

    fn currentState(): CircuitState {
        return _.cur_state;
    }

    fn allowRequest(*_, now_ms: i64): bool {
        _.total_requests += 1;
        _.refreshState(now_ms);

        switch (_.cur_state) {
            .Closed => {
                _.total_allowed += 1;
                return true;
            },
            .Open => {
                _.total_rejected += 1;
                return false;
            },
            .HalfOpen => {
                const max_probes: u32 = if (_.config.half_open_max_probes == 0) 1 else _.config.half_open_max_probes;
                if (_.half_open_in_flight < max_probes) {
                    _.half_open_in_flight += 1;
                    _.total_allowed += 1;
                    return true;
                }
                _.total_rejected += 1;
                return false;
            },
        }
    }

    fn recordSuccess(*_, now_ms: i64) {
        _.refreshState(now_ms);
        _.total_successes += 1;

        switch (_.cur_state) {
            .Closed => {
                _.consecutive_failures = 0;
                _.consecutive_successes += 1;
            },
            .HalfOpen => {
                if (_.half_open_in_flight > 0) {
                    _.half_open_in_flight -= 1;
                }
                _.consecutive_successes += 1;
                const req_ok: u32 = if (_.config.success_threshold_half_open == 0) 1 else _.config.success_threshold_half_open;
                if (_.consecutive_successes >= req_ok) {
                    _.cur_state = CircuitState.Closed;
                    _.consecutive_failures = 0;
                    _.consecutive_successes = 0;
                    _.half_open_in_flight = 0;
                }
            },
            .Open => {},
        }
    }

    fn recordFailure(*_, now_ms: i64) {
        _.refreshState(now_ms);
        _.total_failures += 1;
        _.last_failure_ms = now_ms;

        switch (_.cur_state) {
            .Closed => {
                _.consecutive_successes = 0;
                _.consecutive_failures += 1;
                const threshold: u32 = if (_.config.failure_threshold == 0) 1 else _.config.failure_threshold;
                if (_.consecutive_failures >= threshold) {
                    _.tripOpen(now_ms);
                }
            },
            .HalfOpen => {
                if (_.half_open_in_flight > 0) {
                    _.half_open_in_flight -= 1;
                }
                _.tripOpen(now_ms);
            },
            .Open => {},
        }
    }

    fn forceOpen(*_, now_ms: i64) {
        _.tripOpen(now_ms);
    }

    fn reset(*_) {
        _.cur_state = CircuitState.Closed;
        _.consecutive_failures = 0;
        _.consecutive_successes = 0;
        _.half_open_in_flight = 0;
        _.opened_at_ms = -1;
    }

    fn stats(): CircuitStats {
        return CircuitStats{
            state: _.cur_state,
            consecutive_failures: _.consecutive_failures,
            consecutive_successes: _.consecutive_successes,
            half_open_in_flight: _.half_open_in_flight,
            total_requests: _.total_requests,
            total_allowed: _.total_allowed,
            total_rejected: _.total_rejected,
            total_successes: _.total_successes,
            total_failures: _.total_failures,
            open_count: _.open_count,
            last_failure_ms: _.last_failure_ms,
            opened_at_ms: _.opened_at_ms,
        };
    }
}

// Source: Absorbed from /data/dev/bun/src/test_runner/DoneCallback.cp | Synced: 2026-11-22T18:36:22Z
// ============================================================================
// lib/test_runner/doneCallback.cp — Zero-Alloc Async Test `done()` State Machine
// ============================================================================
// Implements Bun test runner's `DoneCallback` lifecycle for callback-style
// asynchronous tests (`test("async", (done) => { ... done(); })`):
//   - `createUnbound()` -> `bind(test_id, started_at_ms, timeout_ms)`
//   - Single-settlement enforcement: detects accidental double `done()` calls
//     (`AlreadySettled`) while making late callbacks after timeout or `finalize()`
//     safe no-ops.
//   - Inline 128-byte error message buffer for `done(err)` with zero heap allocation.
//   - Default 5000ms per-test timeout aligned with `bun:test`.

/// Default per-test timeout in milliseconds (aligned with `bun:test`).
pub const DONE_CALLBACK_DEFAULT_TIMEOUT_MS: u64 = 5000;
/// Maximum inline error message bytes captured by `fail()`.
pub const DONE_CALLBACK_MAX_ERR_MSG: usize = 128;

pub err DoneCallbackError {
    NotBound,
    AlreadyBound,
    AlreadySettled,
}

pub enum DoneCallbackState {
    Unbound,
    Pending,
    Passed,
    Failed,
    TimedOut,
    Detached,
}

/// Zero-allocation async test completion callback state machine.
pub struct DoneCallback {
    state: DoneCallbackState = DoneCallbackState.Unbound,
    test_id: u64 = 0,
    called: bool = false,
    call_count: u32 = 0,
    timeout_ms: u64 = DONE_CALLBACK_DEFAULT_TIMEOUT_MS,
    started_at_ms: i64 = 0,
    settled_at_ms: i64 = 0,
    error_code: i32 = 0,
    error_msg: [128]u8 = [0; 128],
    error_msg_len: usize = 0,

    /// Creates an unbound callback instance (aligned with Bun `DoneCallback.createUnbound`).
    static fn createUnbound(): DoneCallback {
        return DoneCallback{};
    }

    /// Binds the callback to a concrete test execution (aligned with Bun `DoneCallback.bind`).
    /// Passing `timeout_ms == 0` uses `DONE_CALLBACK_DEFAULT_TIMEOUT_MS` (5000ms).
    fn bind(*_, test_id: u64, started_at_ms: i64, timeout_ms: u64): DoneCallbackError!void {
        if (_.state != DoneCallbackState.Unbound) return DoneCallbackError.AlreadyBound;
        _.test_id = test_id;
        _.started_at_ms = started_at_ms;
        _.settled_at_ms = started_at_ms;
        _.timeout_ms = if (timeout_ms == 0) DONE_CALLBACK_DEFAULT_TIMEOUT_MS else timeout_ms;
        _.state = DoneCallbackState.Pending;
    }

    fn copyErrMsg(*_, msg: []const u8) {
        const n = if (msg.len > DONE_CALLBACK_MAX_ERR_MSG) DONE_CALLBACK_MAX_ERR_MSG else msg.len;
        let i: usize = 0;
        while (i < n) {
            _.error_msg[i] = msg[i];
            i += 1;
        }
        _.error_msg_len = n;
    }

    fn isExpiredAt(now_ms: i64): bool {
        if (now_ms <= _.started_at_ms) return false;
        const elapsed: u64 = @as(u64, @intCast(now_ms - _.started_at_ms));
        return elapsed >= _.timeout_ms;
    }

    /// Signals successful completion (`done()`).
    /// - Returns `true` when transitioning `Pending -> Passed`.
    /// - Returns `false` as a safe no-op if already `TimedOut` or `Detached` (or expired at `now_ms`).
    /// - Returns `AlreadySettled` if `done()` was already invoked while still attached.
    fn complete(*_, now_ms: i64): DoneCallbackError!bool {
        if (_.state == DoneCallbackState.Unbound) return DoneCallbackError.NotBound;
        if (_.state == DoneCallbackState.Detached || _.state == DoneCallbackState.TimedOut) {
            _.call_count += 1;
            return false;
        }
        if (_.state == DoneCallbackState.Passed || _.state == DoneCallbackState.Failed) {
            _.call_count += 1;
            return DoneCallbackError.AlreadySettled;
        }

        if (_.isExpiredAt(now_ms)) {
            _.state = DoneCallbackState.TimedOut;
            _.settled_at_ms = now_ms;
            _.call_count += 1;
            return false;
        }

        _.state = DoneCallbackState.Passed;
        _.called = true;
        _.call_count += 1;
        _.settled_at_ms = now_ms;
        return true;
    }

    /// Signals failure with an error code and message (`done(err)`).
    fn fail(*_, code: i32, msg: []const u8, now_ms: i64): DoneCallbackError!bool {
        if (_.state == DoneCallbackState.Unbound) return DoneCallbackError.NotBound;
        if (_.state == DoneCallbackState.Detached || _.state == DoneCallbackState.TimedOut) {
            _.call_count += 1;
            return false;
        }
        if (_.state == DoneCallbackState.Passed || _.state == DoneCallbackState.Failed) {
            _.call_count += 1;
            return DoneCallbackError.AlreadySettled;
        }

        if (_.isExpiredAt(now_ms)) {
            _.state = DoneCallbackState.TimedOut;
            _.settled_at_ms = now_ms;
            _.call_count += 1;
            return false;
        }

        _.state = DoneCallbackState.Failed;
        _.called = true;
        _.call_count += 1;
        _.settled_at_ms = now_ms;
        _.error_code = code;
        _.copyErrMsg(msg);
        return true;
    }

    /// Fires the per-test timeout watchdog. Returns `true` if `Pending -> TimedOut`.
    fn onTimeout(*_, now_ms: i64): bool {
        if (_.state != DoneCallbackState.Pending) return false;
        _.state = DoneCallbackState.TimedOut;
        _.settled_at_ms = now_ms;
        return true;
    }

    /// Detaches the callback when the test harness tears down (aligned with Bun
    /// `DoneCallback.finalize`). Returns `true` if the test had already settled,
    /// or `false` if it was still `Pending` (dangling/uncalled `done`).
    fn finalize(*_): bool {
        const was_settled = (_.state == DoneCallbackState.Passed ||
            _.state == DoneCallbackState.Failed ||
            _.state == DoneCallbackState.TimedOut);
        _.state = DoneCallbackState.Detached;
        return was_settled;
    }

    fn isPending(): bool {
        return _.state == DoneCallbackState.Pending;
    }

    fn isSettled(): bool {
        return _.state == DoneCallbackState.Passed ||
            _.state == DoneCallbackState.Failed ||
            _.state == DoneCallbackState.TimedOut;
    }

    fn didPass(): bool {
        return _.state == DoneCallbackState.Passed;
    }

    fn didFail(): bool {
        return _.state == DoneCallbackState.Failed;
    }

    fn didTimeout(): bool {
        return _.state == DoneCallbackState.TimedOut;
    }

    fn elapsedMs(): i64 {
        if (_.settled_at_ms <= _.started_at_ms) return 0;
        return _.settled_at_ms - _.started_at_ms;
    }

    fn deadlineMs(): i64 {
        return _.started_at_ms + @as(i64, @intCast(_.timeout_ms));
    }

    fn errorMessage(): []const u8 {
        return _.error_msg[0.._.error_msg_len];
    }
}

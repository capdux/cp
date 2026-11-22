// Source: Absorbed from /data/dev/bun/src/test_runner/expect/toHaveBeenCalledTimes.cp | Synced: 2026-11-22T18:38:51Z
// ============================================================================
// lib/test_runner/mockSpy.cp — Zero-Alloc Mock / Spy Call History & Matchers
// ============================================================================
// Implements Bun `bun:test` mock/spy call tracking and Jest-compatible call
// assertions with zero dynamic heap allocation:
//   - Call & return assertions: `toHaveBeenCalled`, `toHaveBeenCalledOnce`,
//     `toHaveBeenCalledTimes`, `toHaveBeenCalledWith`, `toHaveBeenLastCalledWith`,
//     `toHaveBeenNthCalledWith` (1-based), `toHaveReturned`, `toHaveReturnedTimes`,
//     `toHaveReturnedWith`, `toHaveLastReturnedWith`, `toHaveNthReturnedWith`.
//   - Lifecycle controls aligned with Bun `mocks.mdx`:
//     `mockClear()` (clear history), `mockReset()` (clear + drop custom impl),
//     `mockRestore()` (clear + restore original implementation).

pub const MOCK_SPY_MAX_CALLS: usize = 64;

pub err MockSpyError {
    HistoryFull,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

pub enum MockCallOutcome {
    Returned,
    Threw,
}

pub struct MockCallRecord {
    call_seq: u64 = 0,
    arg_tag: i64 = 0,
    arg_str: []const u8 = "",
    outcome: MockCallOutcome = MockCallOutcome.Returned,
    ret_tag: i64 = 0,
    ret_str: []const u8 = "",
}

/// Fixed-capacity zero-allocation mock/spy recorder and assertion matcher.
pub struct MockSpy {
    calls: [64]MockCallRecord = [MockCallRecord{}; 64],
    recorded_len: usize = 0,
    total_calls: u64 = 0,
    returned_calls: u64 = 0,
    has_custom_impl: bool = false,
    restored: bool = false,

    static fn init(has_custom_impl: bool): MockSpy {
        return MockSpy{
            has_custom_impl: has_custom_impl,
            restored: false,
        };
    }

    /// Records a normal call invocation that returned a value.
    /// Increments `total_calls` and `returned_calls` even if the 64-slot ring is full.
    fn recordReturn(*_, arg_tag: i64, arg_str: []const u8, ret_tag: i64, ret_str: []const u8): MockSpyError!usize {
        _.total_calls += 1;
        _.returned_calls += 1;
        if (_.recorded_len >= MOCK_SPY_MAX_CALLS) return MockSpyError.HistoryFull;
        const idx = _.recorded_len;
        _.calls[idx] = MockCallRecord{
            call_seq: _.total_calls,
            arg_tag: arg_tag,
            arg_str: arg_str,
            outcome: MockCallOutcome.Returned,
            ret_tag: ret_tag,
            ret_str: ret_str,
        };
        _.recorded_len += 1;
        return idx;
    }

    /// Records a call invocation that threw an error.
    fn recordThrow(*_, arg_tag: i64, arg_str: []const u8, err_code: i64, err_msg: []const u8): MockSpyError!usize {
        _.total_calls += 1;
        if (_.recorded_len >= MOCK_SPY_MAX_CALLS) return MockSpyError.HistoryFull;
        const idx = _.recorded_len;
        _.calls[idx] = MockCallRecord{
            call_seq: _.total_calls,
            arg_tag: arg_tag,
            arg_str: arg_str,
            outcome: MockCallOutcome.Threw,
            ret_tag: err_code,
            ret_str: err_msg,
        };
        _.recorded_len += 1;
        return idx;
    }

    /// Clears call history and counters while keeping the custom mock implementation.
    fn mockClear(*_) {
        _.recorded_len = 0;
        _.total_calls = 0;
        _.returned_calls = 0;
    }

    /// Clears call history and removes any custom mock implementation.
    fn mockReset(*_) {
        _.mockClear();
        _.has_custom_impl = false;
    }

    /// Clears call history, removes custom mock implementation, and restores original.
    fn mockRestore(*_) {
        _.mockReset();
        _.restored = true;
    }

    fn callCount(): u64 {
        return _.total_calls;
    }

    /// Returns the 1-based `nth` recorded call (1 <= nth <= recorded_len), or `null`.
    fn getCall(nth_1based: usize): MockCallRecord? {
        if (nth_1based == 0 || nth_1based > _.recorded_len) return null;
        return _.calls[nth_1based - 1];
    }

    fn toHaveBeenCalled(): bool {
        return _.total_calls > 0;
    }

    fn toHaveBeenCalledOnce(): bool {
        return _.total_calls == 1;
    }

    fn toHaveBeenCalledTimes(expected: u64): bool {
        return _.total_calls == expected;
    }

    fn toHaveBeenCalledWith(arg_tag: i64, arg_str: []const u8): bool {
        let i: usize = 0;
        while (i < _.recorded_len) {
            if (_.calls[i].arg_tag == arg_tag && eqlBytes(_.calls[i].arg_str, arg_str)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn toHaveBeenLastCalledWith(arg_tag: i64, arg_str: []const u8): bool {
        if (_.recorded_len == 0) return false;
        const last = _.calls[_.recorded_len - 1];
        return last.arg_tag == arg_tag && eqlBytes(last.arg_str, arg_str);
    }

    /// 1-based index check aligned with Jest/Bun `toHaveBeenNthCalledWith(nth, ...)`.
    fn toHaveBeenNthCalledWith(nth_1based: usize, arg_tag: i64, arg_str: []const u8): bool {
        if (nth_1based == 0 || nth_1based > _.recorded_len) return false;
        const rec = _.calls[nth_1based - 1];
        return rec.arg_tag == arg_tag && eqlBytes(rec.arg_str, arg_str);
    }

    fn toHaveReturned(): bool {
        return _.returned_calls > 0;
    }

    fn toHaveReturnedTimes(expected: u64): bool {
        return _.returned_calls == expected;
    }

    fn toHaveReturnedWith(ret_tag: i64, ret_str: []const u8): bool {
        let i: usize = 0;
        while (i < _.recorded_len) {
            if (_.calls[i].outcome == MockCallOutcome.Returned &&
                _.calls[i].ret_tag == ret_tag &&
                eqlBytes(_.calls[i].ret_str, ret_str))
            {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn toHaveLastReturnedWith(ret_tag: i64, ret_str: []const u8): bool {
        if (_.recorded_len == 0) return false;
        const last = _.calls[_.recorded_len - 1];
        return last.outcome == MockCallOutcome.Returned &&
            last.ret_tag == ret_tag &&
            eqlBytes(last.ret_str, ret_str);
    }

    fn toHaveNthReturnedWith(nth_1based: usize, ret_tag: i64, ret_str: []const u8): bool {
        if (nth_1based == 0 || nth_1based > _.recorded_len) return false;
        const rec = _.calls[nth_1based - 1];
        return rec.outcome == MockCallOutcome.Returned &&
            rec.ret_tag == ret_tag &&
            eqlBytes(rec.ret_str, ret_str);
    }
}

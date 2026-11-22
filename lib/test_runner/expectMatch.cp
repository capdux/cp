// Source: Absorbed from /data/dev/bun/src/test_runner/expect/toEqualIgnoringWhitespace.cp | Synced: 2026-11-22T16:31:13Z
// Pure-CP zero-allocation Bun/Jest-compatible expect() matchers:
// whitespace-insensitive string equality, substring/prefix/suffix/repeat matchers,
// floating-point `toBeCloseTo`, half-open `toBeWithin`, and parity matchers.

pub fn isAsciiWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0B || c == 0x0C;
}

pub struct ExpectOutcome {
    passed: bool = false,
    negated: bool = false,
    mismatch_pos: usize = 0,

    fn effectivePass(): bool {
        return if (_.negated) !_.passed else _.passed;
    }
}

/// Compares `actual` and `expected` ignoring all ASCII whitespace characters.
/// Strictly mirrors Bun's two-pointer algorithm in `toEqualIgnoringWhitespace.cp`
/// with zero heap allocation.
pub fn equalIgnoringWhitespace(actual: []const u8, expected: []const u8): bool {
    let left: usize = 0;
    let right: usize = 0;

    while (left < actual.len && isAsciiWhitespace(actual[left])) left += 1;
    while (right < expected.len && isAsciiWhitespace(expected[right])) right += 1;

    while (left < actual.len && right < expected.len) {
        if (actual[left] != expected[right]) {
            return false;
        }
        left += 1;
        right += 1;
        while (left < actual.len && isAsciiWhitespace(actual[left])) left += 1;
        while (right < expected.len && isAsciiWhitespace(expected[right])) right += 1;
    }

    return left == actual.len && right == expected.len;
}

/// Variant of `equalIgnoringWhitespace` returning the byte offset in `actual`
/// where the first mismatch occurred (for diagnostic reporting).
pub fn equalIgnoringWhitespacePos(actual: []const u8, expected: []const u8): ExpectOutcome {
    let left: usize = 0;
    let right: usize = 0;

    while (left < actual.len && isAsciiWhitespace(actual[left])) left += 1;
    while (right < expected.len && isAsciiWhitespace(expected[right])) right += 1;

    while (left < actual.len && right < expected.len) {
        if (actual[left] != expected[right]) {
            return ExpectOutcome { passed: false, negated: false, mismatch_pos: left };
        }
        left += 1;
        right += 1;
        while (left < actual.len && isAsciiWhitespace(actual[left])) left += 1;
        while (right < expected.len && isAsciiWhitespace(expected[right])) right += 1;
    }

    if (left == actual.len && right == expected.len) {
        return ExpectOutcome { passed: true, negated: false, mismatch_pos: 0 };
    }
    return ExpectOutcome { passed: false, negated: false, mismatch_pos: left };
}

/// Trims leading/trailing ASCII whitespace and collapses interior whitespace runs
/// into single ASCII spaces into caller-provided `out`.
pub fn collapseWhitespace(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    while (i < input.len && isAsciiWhitespace(input[i])) i += 1;

    let pos: usize = 0;
    let pending_space = false;

    while (i < input.len) {
        const c = input[i];
        if (isAsciiWhitespace(c)) {
            pending_space = true;
        } else {
            if (pending_space && pos > 0) {
                if (pos >= out.len) return null;
                out[pos] = ' ';
                pos += 1;
            }
            pending_space = false;
            if (pos >= out.len) return null;
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }

    return out[0..pos];
}

pub fn startsWith(actual: []const u8, prefix: []const u8): bool {
    if (prefix.len > actual.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (actual[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

pub fn endsWith(actual: []const u8, suffix: []const u8): bool {
    if (suffix.len > actual.len) return false;
    const offset = actual.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (actual[offset + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

pub fn includesSubstring(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        while (j < needle.len && haystack[i + j] == needle[j]) {
            j += 1;
        }
        if (j == needle.len) return true;
        i += 1;
    }
    return false;
}

/// Counts non-overlapping occurrences of `needle` in `haystack`.
pub fn countOccurrences(haystack: []const u8, needle: []const u8): usize {
    if (needle.len == 0 || needle.len > haystack.len) return 0;
    let count: usize = 0;
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        let j: usize = 0;
        while (j < needle.len && haystack[i + j] == needle[j]) {
            j += 1;
        }
        if (j == needle.len) {
            count += 1;
            i += needle.len;
        } else {
            i += 1;
        }
    }
    return count;
}

/// Aligns with Bun `expect(haystack).toIncludeRepeated(needle, expected_times)`.
pub fn includesRepeated(haystack: []const u8, needle: []const u8, expected_times: usize): bool {
    if (needle.len == 0) return expected_times == 0;
    return countOccurrences(haystack, needle) == expected_times;
}

fn isNaNF64(v: f64): bool {
    return v != v;
}

fn pow10NegHalf(num_digits: i32): f64 {
    let clamped: i32 = num_digits;
    if (clamped < -15) clamped = -15;
    if (clamped > 15) clamped = 15;
    let scale: f64 = 1.0;
    if (clamped >= 0) {
        let k: i32 = 0;
        while (k < clamped) {
            scale *= 10.0;
            k += 1;
        }
        return 0.5 / scale;
    } else {
        let k: i32 = 0;
        const n: i32 = -clamped;
        while (k < n) {
            scale *= 10.0;
            k += 1;
        }
        return 0.5 * scale;
    }
}

/// Aligns with Jest / Bun `expect(actual).toBeCloseTo(expected, num_digits)`:
/// passes iff `|actual - expected| < (10^-num_digits) / 2` (default `num_digits = 2`).
pub fn isCloseTo(actual: f64, expected: f64, num_digits: i32): bool {
    if (isNaNF64(actual) || isNaNF64(expected)) return false;
    if (actual == expected) return true;
    let diff = actual - expected;
    if (diff < 0.0) diff = -diff;
    return diff < pow10NegHalf(num_digits);
}

/// Aligns with Bun `expect(actual).toBeWithin(start, end)` — half-open `[start, end)`.
pub fn isWithinI64(actual: i64, start: i64, end: i64): bool {
    return actual >= start && actual < end;
}

/// Aligns with Bun `expect(actual).toBeWithin(start, end)` for f64 — half-open `[start, end)`.
pub fn isWithinF64(actual: f64, start: f64, end: f64): bool {
    if (isNaNF64(actual) || isNaNF64(start) || isNaNF64(end)) return false;
    return actual >= start && actual < end;
}

pub fn isEvenI64(v: i64): bool {
    return (v & 1) == 0;
}

pub fn isOddI64(v: i64): bool {
    return (v & 1) != 0;
}

/// Stateful expect() matcher evaluator tracking assertion counts and `.not` negation.
pub struct ExpectMatcher {
    negated: bool = false,
    assertions: usize = 0,
    failures: usize = 0,

    static fn init(): ExpectMatcher {
        return ExpectMatcher {};
    }

    static fn negatedMatcher(): ExpectMatcher {
        return ExpectMatcher { negated: true };
    }

    fn setNegated(*_, flag: bool) {
        _.negated = flag;
    }

    fn assertionCount(): usize {
        return _.assertions;
    }

    fn failureCount(): usize {
        return _.failures;
    }

    fn record(*_, raw_pass: bool): bool {
        _.assertions += 1;
        const ok = if (_.negated) !raw_pass else raw_pass;
        if (!ok) _.failures += 1;
        return ok;
    }

    fn toEqualIgnoringWhitespace(*_, actual: []const u8, expected: []const u8): bool {
        return _.record(equalIgnoringWhitespace(actual, expected));
    }

    fn toStartWith(*_, actual: []const u8, prefix: []const u8): bool {
        return _.record(startsWith(actual, prefix));
    }

    fn toEndWith(*_, actual: []const u8, suffix: []const u8): bool {
        return _.record(endsWith(actual, suffix));
    }

    fn toInclude(*_, haystack: []const u8, needle: []const u8): bool {
        return _.record(includesSubstring(haystack, needle));
    }

    fn toIncludeRepeated(*_, haystack: []const u8, needle: []const u8, times: usize): bool {
        return _.record(includesRepeated(haystack, needle, times));
    }

    fn toBeCloseTo(*_, actual: f64, expected: f64, num_digits: i32): bool {
        return _.record(isCloseTo(actual, expected, num_digits));
    }

    fn toBeWithinI64(*_, actual: i64, start: i64, end: i64): bool {
        return _.record(isWithinI64(actual, start, end));
    }

    fn toBeWithinF64(*_, actual: f64, start: f64, end: f64): bool {
        return _.record(isWithinF64(actual, start, end));
    }

    fn toBeEven(*_, v: i64): bool {
        return _.record(isEvenI64(v));
    }

    fn toBeOdd(*_, v: i64): bool {
        return _.record(isOddI64(v));
    }
}

// Source: Absorbed from /data/dev/bun/src/test_runner/expect/toContainAllKeys.cp | Synced: 2026-11-22T18:40:52Z
// ============================================================================
// lib/test_runner/expectKeys.cp — Zero-Alloc Key/Value & Membership Matchers
// ============================================================================
// Implements Bun `bun:test` / `jest-extended` object key/value set matchers and
// membership checks with zero dynamic heap allocation:
//   - `toContainKey` / `toContainKeys` / `toContainAllKeys` / `toContainAnyKeys`
//   - `toContainValue` / `toContainValues` / `toContainAllValues` (multiset-aware)
//     / `toContainAnyValues`
//   - `toBeOneOfStr` / `toBeOneOfI64`
//   - `ExpectKeysMatcher` with one-shot `.not()` negation and assertion counters.

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// A single key/value pair representing an object property for zero-alloc matching.
pub struct ExpectKvEntry {
    key: []const u8 = "",
    val: []const u8 = "",
}

/// Returns true if `value` equals any element in `candidates` (`toBeOneOf`).
pub fn toBeOneOfStr(value: []const u8, candidates: []const []const u8): bool {
    let i: usize = 0;
    while (i < candidates.len) {
        if (eqlBytes(value, candidates[i])) return true;
        i += 1;
    }
    return false;
}

/// Returns true if `value` equals any integer in `candidates` (`toBeOneOf`).
pub fn toBeOneOfI64(value: i64, candidates: []const i64): bool {
    let i: usize = 0;
    while (i < candidates.len) {
        if (value == candidates[i]) return true;
        i += 1;
    }
    return false;
}

/// Returns true if `entries` contains an entry with exact key `key` (`toContainKey`).
pub fn toContainKey(entries: []const ExpectKvEntry, key: []const u8): bool {
    let i: usize = 0;
    while (i < entries.len) {
        if (eqlBytes(entries[i].key, key)) return true;
        i += 1;
    }
    return false;
}

/// Returns true if `entries` contains every key in `expected_keys` (extra keys allowed).
pub fn toContainKeys(entries: []const ExpectKvEntry, expected_keys: []const []const u8): bool {
    let i: usize = 0;
    while (i < expected_keys.len) {
        if (!toContainKey(entries, expected_keys[i])) return false;
        i += 1;
    }
    return true;
}

/// Returns true if `entries` contains all and only the keys in `expected_keys`
/// (order-independent, aligned with Bun `toContainAllKeys`).
pub fn toContainAllKeys(entries: []const ExpectKvEntry, expected_keys: []const []const u8): bool {
    if (entries.len != expected_keys.len) return false;
    let i: usize = 0;
    while (i < entries.len) {
        if (!toBeOneOfStr(entries[i].key, expected_keys)) return false;
        i += 1;
    }
    let j: usize = 0;
    while (j < expected_keys.len) {
        if (!toContainKey(entries, expected_keys[j])) return false;
        j += 1;
    }
    return true;
}

/// Returns true if `entries` contains at least one key from `candidate_keys`.
pub fn toContainAnyKeys(entries: []const ExpectKvEntry, candidate_keys: []const []const u8): bool {
    let i: usize = 0;
    while (i < candidate_keys.len) {
        if (toContainKey(entries, candidate_keys[i])) return true;
        i += 1;
    }
    return false;
}

/// Returns true if `entries` contains an entry with exact value `val` (`toContainValue`).
pub fn toContainValue(entries: []const ExpectKvEntry, val: []const u8): bool {
    let i: usize = 0;
    while (i < entries.len) {
        if (eqlBytes(entries[i].val, val)) return true;
        i += 1;
    }
    return false;
}

/// Returns true if `entries` contains every value in `expected_vals` (extra values allowed).
pub fn toContainValues(entries: []const ExpectKvEntry, expected_vals: []const []const u8): bool {
    let i: usize = 0;
    while (i < expected_vals.len) {
        if (!toContainValue(entries, expected_vals[i])) return false;
        i += 1;
    }
    return true;
}

fn countValInEntries(entries: []const ExpectKvEntry, val: []const u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < entries.len) {
        if (eqlBytes(entries[i].val, val)) n += 1;
        i += 1;
    }
    return n;
}

fn countValInSlice(vals: []const []const u8, val: []const u8): usize {
    let n: usize = 0;
    let i: usize = 0;
    while (i < vals.len) {
        if (eqlBytes(vals[i], val)) n += 1;
        i += 1;
    }
    return n;
}

/// Returns true if the values of `entries` form the exact same multiset as
/// `expected_vals` (order-independent, duplicate multiplicities must match).
pub fn toContainAllValues(entries: []const ExpectKvEntry, expected_vals: []const []const u8): bool {
    if (entries.len != expected_vals.len) return false;
    let i: usize = 0;
    while (i < expected_vals.len) {
        const v = expected_vals[i];
        if (countValInEntries(entries, v) != countValInSlice(expected_vals, v)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns true if `entries` contains at least one value from `candidate_vals`.
pub fn toContainAnyValues(entries: []const ExpectKvEntry, candidate_vals: []const []const u8): bool {
    let i: usize = 0;
    while (i < candidate_vals.len) {
        if (toContainValue(entries, candidate_vals[i])) return true;
        i += 1;
    }
    return false;
}

/// Stateful assertion matcher with one-shot `.not()` negation and pass/fail counters.
pub struct ExpectKeysMatcher {
    assertions: u64 = 0,
    passed: u64 = 0,
    failed: u64 = 0,
    negated: bool = false,

    static fn init(): ExpectKeysMatcher {
        return ExpectKeysMatcher{};
    }

    /// Toggles one-shot negation for the immediately following `check*` call.
    fn negate(*_) {
        _.negated = !_.negated;
    }

    fn record(*_, raw_pass: bool): bool {
        const pass = if (_.negated) !raw_pass else raw_pass;
        _.negated = false;
        _.assertions += 1;
        if (pass) {
            _.passed += 1;
        } else {
            _.failed += 1;
        }
        return pass;
    }

    fn checkContainKey(*_, entries: []const ExpectKvEntry, key: []const u8): bool {
        return _.record(toContainKey(entries, key));
    }

    fn checkContainKeys(*_, entries: []const ExpectKvEntry, expected_keys: []const []const u8): bool {
        return _.record(toContainKeys(entries, expected_keys));
    }

    fn checkContainAllKeys(*_, entries: []const ExpectKvEntry, expected_keys: []const []const u8): bool {
        return _.record(toContainAllKeys(entries, expected_keys));
    }

    fn checkContainAnyKeys(*_, entries: []const ExpectKvEntry, candidate_keys: []const []const u8): bool {
        return _.record(toContainAnyKeys(entries, candidate_keys));
    }

    fn checkContainAllValues(*_, entries: []const ExpectKvEntry, expected_vals: []const []const u8): bool {
        return _.record(toContainAllValues(entries, expected_vals));
    }

    fn checkBeOneOfStr(*_, value: []const u8, candidates: []const []const u8): bool {
        return _.record(toBeOneOfStr(value, candidates));
    }
}

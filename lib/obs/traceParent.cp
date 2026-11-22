// Origin: Pure-CP Stdlib (W3C Trace Context Level 1/2: traceparent & tracestate Propagation Engine) | Created: 2026-11-23
// Pure CP zero-heap-allocation W3C Trace Context Level 1/2 codec and propagation engine:
//   - `TraceParent`: strict W3C `traceparent` parser/formatter (`00-<32hex>-<16hex>-<2hex>`),
//     rejecting version `ff`, uppercase hex, all-zero `trace_id`/`parent_id`, and CRLF/NUL injection,
//     while supporting forward-compatible future versions (`version > 0x00` trailing `-<ext>` fields)
//     and child span derivation (`deriveChild`)
//   - `TraceState`: order-preserving 32-entry W3C `tracestate` vendor list supporting single-segment
//     `vendor` and multi-tenant `tenant@vendor` keys, OWS trimming, duplicate-key deduplication,
//     MRU `setFront` promotion with tail eviction at capacity 32, and zero-alloc formatting

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// Fixed canonical length of a W3C `traceparent` v00 header (`00-<32hex>-<16hex>-<2hex>`).
pub const TRACEPARENT_HEADER_LEN: usize = 55;

/// W3C `trace-flags` bit 0: caller sampled the trace (`0x01`).
pub const TRACE_FLAG_SAMPLED: u8 = 0x01;

/// W3C Trace Context Level 2 `trace-flags` bit 1: `trace_id` has >= 56 random bits (`0x02`).
pub const TRACE_FLAG_RANDOM: u8 = 0x02;

/// W3C `tracestate` maximum number of list-member entries (`32`).
pub const TRACESTATE_MAX_ENTRIES: usize = 32;

/// Maximum inline byte length for a single `tracestate` key (`64`).
pub const TRACESTATE_KEY_MAX_LEN: usize = 64;

/// Maximum inline byte length for a single `tracestate` value (`128`).
pub const TRACESTATE_VAL_MAX_LEN: usize = 128;

/// Validation, parse, and formatting errors for W3C Trace Context headers.
pub err TraceContextError {
    InvalidLength,
    InvalidVersion,
    InvalidHex,
    AllZeroTraceId,
    AllZeroParentId,
    InvalidSeparator,
    InvalidKey,
    InvalidValue,
    CrlfRejected,
    BufferTooSmall,
}

// ============================================================================
// 2. Internal Byte, Hex & Validation Helpers
// ============================================================================

const HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

fn hasCrlfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n' || s[i] == 0) return true;
    }
    return false;
}

fn hexLowerNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    return null;
}

fn parseHexByteLower(hi_c: u8, lo_c: u8): u8 ! TraceContextError {
    const hi: u8 = hexLowerNibble(hi_c) ?? return TraceContextError.InvalidHex;
    const lo: u8 = hexLowerNibble(lo_c) ?? return TraceContextError.InvalidHex;
    return (hi << 4) | lo;
}

fn writeHexByteLower(buf: *[]u8, pos: usize, b: u8) {
    buf[pos] = HEX_LOWER[(b >> 4) as usize];
    buf[pos + 1] = HEX_LOWER[(b & 0x0F) as usize];
}

fn isAllZeroBytes(bytes: []const u8): bool {
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        if (bytes[i] != 0) return false;
    }
    return true;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
    return s[start..end];
}

fn isLowerAlpha(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn isLowerAlnum(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}

fn isKeyBodyChar(c: u8): bool {
    return isLowerAlnum(c) || c == '_' || c == '-' || c == '*' || c == '/';
}

/// Validates a W3C `tracestate` key per Level 1/2 specification:
/// - Simple key (`vendor`): `1..=64` bytes, starts with `[a-z]`, body `[a-z0-9_\-*/]`.
/// - Multi-tenant key (`tenant@vendor`): `tenant` starts with `[a-z0-9]` (`>= 1` byte),
///   `vendor` starts with `[a-z]` (`1..=14` bytes), both body `[a-z0-9_\-*/]`.
pub fn isValidTraceStateKey(key: []const u8): bool {
    if (key.len == 0 || key.len > TRACESTATE_KEY_MAX_LEN) return false;

    let at_pos: usize = key.len;
    let at_count: usize = 0;
    let i: usize = 0;
    while (i < key.len) : (i += 1) {
        if (key[i] == '@') {
            at_pos = i;
            at_count += 1;
        }
    }
    if (at_count > 1) return false;

    if (at_count == 0) {
        if (!isLowerAlpha(key[0])) return false;
        let j: usize = 1;
        while (j < key.len) : (j += 1) {
            if (!isKeyBodyChar(key[j])) return false;
        }
        return true;
    }

    // Multi-tenant: tenant@vendor
    const tenant_len: usize = at_pos;
    const vendor_start: usize = at_pos + 1;
    const vendor_len: usize = key.len - vendor_start;
    if (tenant_len == 0 || vendor_len == 0 || vendor_len > 14) return false;

    if (!isLowerAlnum(key[0])) return false;
    let ti: usize = 1;
    while (ti < tenant_len) : (ti += 1) {
        if (!isKeyBodyChar(key[ti])) return false;
    }

    if (!isLowerAlpha(key[vendor_start])) return false;
    let vi: usize = vendor_start + 1;
    while (vi < key.len) : (vi += 1) {
        if (!isKeyBodyChar(key[vi])) return false;
    }
    return true;
}

fn isValidTraceStateValue(val: []const u8): bool {
    if (val.len == 0 || val.len > TRACESTATE_VAL_MAX_LEN) return false;
    // W3C tracestate value: 0x20..=0x7E excluding ',' (0x2C) and '=' (0x3D); last byte must not be space.
    if (val[val.len - 1] == 0x20) return false;
    let i: usize = 0;
    while (i < val.len) : (i += 1) {
        const c: u8 = val[i];
        if (c < 0x20 || c > 0x7E || c == ',' || c == '=') return false;
    }
    return true;
}

// ============================================================================
// 3. W3C `TraceParent` Codec & Propagation
// ============================================================================

/// W3C Trace Context `traceparent` header representation (`version`, `trace_id`, `parent_id`, `trace_flags`).
pub struct TraceParent {
    version: u8 = 0;
    trace_id: [16]u8 = [0; 16];
    parent_id: [8]u8 = [0; 8];
    trace_flags: u8 = 0;

    /// Parses a W3C `traceparent` header string strictly per W3C Trace Context Level 1/2:
    /// - Rejects CRLF/NUL injection, uppercase hex, version `0xff`, all-zero `trace_id` or `parent_id`.
    /// - Version `0x00` requires exact length `55`.
    /// - Future versions (`0x01..0xfe`) accept trailing `-<extension>` fields for forward compatibility.
    static fn parse(header: []const u8): TraceParent ! TraceContextError {
        if (hasCrlfOrNul(header)) return TraceContextError.CrlfRejected;
        if (header.len < TRACEPARENT_HEADER_LEN) return TraceContextError.InvalidLength;

        if (header[2] != '-' || header[35] != '-' || header[52] != '-') {
            return TraceContextError.InvalidSeparator;
        }

        const version: u8 = parseHexByteLower(header[0], header[1])?;
        if (version == 0xFF) return TraceContextError.InvalidVersion;

        if (version == 0x00) {
            if (header.len != TRACEPARENT_HEADER_LEN) return TraceContextError.InvalidLength;
        } else if (header.len > TRACEPARENT_HEADER_LEN) {
            // Forward-compatible extension must be preceded by '-' and contain >= 1 printable byte
            if (header[TRACEPARENT_HEADER_LEN] != '-') return TraceContextError.InvalidSeparator;
            if (header.len == TRACEPARENT_HEADER_LEN + 1) return TraceContextError.InvalidLength;
            let ei: usize = TRACEPARENT_HEADER_LEN + 1;
            while (ei < header.len) : (ei += 1) {
                if (header[ei] < 0x20 || header[ei] > 0x7E) return TraceContextError.InvalidValue;
            }
        }

        let tid: [16]u8 = [0; 16];
        let ti: usize = 0;
        while (ti < 16) : (ti += 1) {
            const off: usize = @as(usize, 3) + ti * @as(usize, 2);
            tid[ti] = parseHexByteLower(header[off], header[off + 1])?;
        }
        if (isAllZeroBytes(tid[0..])) return TraceContextError.AllZeroTraceId;

        let pid: [8]u8 = [0; 8];
        let pi: usize = 0;
        while (pi < 8) : (pi += 1) {
            const off: usize = @as(usize, 36) + pi * @as(usize, 2);
            pid[pi] = parseHexByteLower(header[off], header[off + 1])?;
        }
        if (isAllZeroBytes(pid[0..])) return TraceContextError.AllZeroParentId;

        const flags: u8 = parseHexByteLower(header[53], header[54])?;

        return TraceParent{
            version: version,
            trace_id: tid,
            parent_id: pid,
            trace_flags: flags,
        };
    }

    /// Formats this `TraceParent` into `out_buf` as canonical 55-byte lowercase hex
    /// (`vv-<32hex>-<16hex>-<2hex>`). Returns `55` on success.
    fn format(out_buf: *[]u8): usize ! TraceContextError {
        if (out_buf.len < TRACEPARENT_HEADER_LEN) return TraceContextError.BufferTooSmall;
        writeHexByteLower(out_buf, 0, version);
        out_buf[2] = '-';

        let ti: usize = 0;
        while (ti < 16) : (ti += 1) {
            writeHexByteLower(out_buf, 3 + ti * 2, trace_id[ti]);
        }
        out_buf[35] = '-';

        let pi: usize = 0;
        while (pi < 8) : (pi += 1) {
            writeHexByteLower(out_buf, 36 + pi * 2, parent_id[pi]);
        }
        out_buf[52] = '-';

        writeHexByteLower(out_buf, 53, trace_flags);
        return TRACEPARENT_HEADER_LEN;
    }

    /// Returns `true` if the `sampled` flag (`0x01`) is set.
    fn isSampled(): bool {
        return (trace_flags & TRACE_FLAG_SAMPLED) != 0;
    }

    /// Returns `true` if the W3C Level 2 `random` flag (`0x02`) is set.
    fn isRandom(): bool {
        return (trace_flags & TRACE_FLAG_RANDOM) != 0;
    }

    /// Returns a copy of this `TraceParent` with the `sampled` bit updated.
    fn withSampled(sampled: bool): TraceParent {
        const next_flags: u8 = if (sampled) (trace_flags | TRACE_FLAG_SAMPLED) else (trace_flags & (~TRACE_FLAG_SAMPLED));
        return TraceParent{
            version: version,
            trace_id: trace_id,
            parent_id: parent_id,
            trace_flags: next_flags,
        };
    }

    /// Derives a child span `TraceParent` keeping `version`, `trace_id`, and `trace_flags`,
    /// replacing `parent_id` with `new_parent_id`. Rejects an all-zero `new_parent_id`.
    fn deriveChild(new_parent_id: [8]u8): TraceParent ! TraceContextError {
        if (isAllZeroBytes(new_parent_id[0..])) return TraceContextError.AllZeroParentId;
        return TraceParent{
            version: version,
            trace_id: trace_id,
            parent_id: new_parent_id,
            trace_flags: trace_flags,
        };
    }
}

// ============================================================================
// 4. W3C `TraceState` Ordered Vendor List
// ============================================================================

/// Single vendor key-value entry inside `TraceState`.
pub struct TraceStateEntry {
    key_buf: [TRACESTATE_KEY_MAX_LEN]u8 = [0; TRACESTATE_KEY_MAX_LEN];
    key_len: usize = 0;
    val_buf: [TRACESTATE_VAL_MAX_LEN]u8 = [0; TRACESTATE_VAL_MAX_LEN];
    val_len: usize = 0;

    /// Returns the vendor key slice (`vendor` or `tenant@vendor`).
    fn key(): []const u8 {
        return key_buf[0..key_len];
    }

    /// Returns the vendor value slice.
    fn value(): []const u8 {
        return val_buf[0..val_len];
    }
}

fn makeTraceStateEntry(k: []const u8, v: []const u8): TraceStateEntry {
    let e = TraceStateEntry{ key_len: k.len, val_len: v.len };
    let i: usize = 0;
    while (i < k.len) : (i += 1) {
        e.key_buf[i] = k[i];
    }
    let j: usize = 0;
    while (j < v.len) : (j += 1) {
        e.val_buf[j] = v[j];
    }
    return e;
}

/// Zero-heap-allocation W3C `tracestate` list (up to 32 order-preserved vendor entries).
pub struct TraceState {
    entries: [TRACESTATE_MAX_ENTRIES]TraceStateEntry = [TraceStateEntry{}; TRACESTATE_MAX_ENTRIES];
    count: usize = 0;

    /// Initializes an empty `TraceState`.
    static fn init(): TraceState {
        return TraceState{};
    }

    /// Parses a W3C `tracestate` header:
    /// - Rejects CRLF/NUL injection.
    /// - Splits on `,`, trims OWS, skips empty list members, validates keys/values,
    ///   deduplicates repeated keys (keeping the leftmost/most-recent occurrence),
    ///   and retains at most `TRACESTATE_MAX_ENTRIES` (`32`) entries from the front.
    static fn parse(header: []const u8): TraceState ! TraceContextError {
        if (hasCrlfOrNul(header)) return TraceContextError.CrlfRejected;

        let ts = TraceState.init();
        let pos: usize = 0;
        while (pos <= header.len) {
            let next_comma: usize = pos;
            while (next_comma < header.len && header[next_comma] != ',') : (next_comma += 1) {}

            const raw_member: []const u8 = header[pos..next_comma];
            const member: []const u8 = trimOws(raw_member);
            if (member.len > 0) {
                let eq_pos: usize = member.len;
                let k: usize = 0;
                while (k < member.len) : (k += 1) {
                    if (member[k] == '=') {
                        eq_pos = k;
                        break;
                    }
                }
                if (eq_pos == member.len) return TraceContextError.InvalidKey;

                const key_s: []const u8 = member[0..eq_pos];
                const val_s: []const u8 = member[eq_pos + 1 .. member.len];
                if (!isValidTraceStateKey(key_s)) return TraceContextError.InvalidKey;
                if (!isValidTraceStateValue(val_s)) return TraceContextError.InvalidValue;

                // Leftmost occurrence wins for duplicate keys
                if (ts.get(key_s) == null && ts.count < TRACESTATE_MAX_ENTRIES) {
                    ts.entries[ts.count] = makeTraceStateEntry(key_s, val_s);
                    ts.count += 1;
                }
            }

            if (next_comma == header.len) break;
            pos = next_comma + 1;
        }
        return ts;
    }

    /// Returns the number of stored vendor entries (`0..=32`).
    fn len(): usize {
        return count;
    }

    /// Returns the value associated with `key`, or `null` if absent.
    fn get(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (bytesEqual(entries[i].key(), key)) {
                return entries[i].value();
            }
        }
        return null;
    }

    /// Returns the entry at index `idx` (`0` is most recent), or `null` if out of bounds.
    fn entryAt(idx: usize): TraceStateEntry? {
        if (idx >= count) return null;
        return entries[idx];
    }

    /// Inserts or updates `(key, value)` at the front (index `0`) per W3C `tracestate` mutation rules.
    /// If already present, promotes the entry to index `0`. If full (`32` entries) and inserting a new key,
    /// evicts the oldest entry at the tail (`index 31`).
    fn setFront(*_, key: []const u8, value: []const u8): void ! TraceContextError {
        if (hasCrlfOrNul(key) || hasCrlfOrNul(value)) return TraceContextError.CrlfRejected;
        if (!isValidTraceStateKey(key)) return TraceContextError.InvalidKey;
        if (!isValidTraceStateValue(value)) return TraceContextError.InvalidValue;

        const new_entry = makeTraceStateEntry(key, value);

        // Check if key already exists
        let found: usize = count;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (bytesEqual(entries[i].key(), key)) {
                found = i;
                break;
            }
        }

        if (found < count) {
            // Shift entries[0..found] right by 1
            let j: usize = found;
            while (j > 0) : (j -= 1) {
                entries[j] = entries[j - 1];
            }
            entries[0] = new_entry;
            return;
        }

        // New key: shift right, evicting tail if already at TRACESTATE_MAX_ENTRIES
        const limit: usize = if (count < TRACESTATE_MAX_ENTRIES) count else (TRACESTATE_MAX_ENTRIES - 1);
        let j: usize = limit;
        while (j > 0) : (j -= 1) {
            entries[j] = entries[j - 1];
        }
        entries[0] = new_entry;
        if (count < TRACESTATE_MAX_ENTRIES) {
            count += 1;
        }
    }

    /// Removes `key` from the list while preserving relative order of remaining entries.
    /// Returns `true` if removed, `false` if not found.
    fn remove(*_, key: []const u8): bool {
        let i: usize = 0;
        while (i < count) : (i += 1) {
            if (bytesEqual(entries[i].key(), key)) {
                let j: usize = i;
                while (j + 1 < count) : (j += 1) {
                    entries[j] = entries[j + 1];
                }
                count -= 1;
                return true;
            }
        }
        return false;
    }

    /// Serializes all entries in order as `k0=v0,k1=v1,...` into `out_buf`.
    /// Returns the number of bytes written (`0` if empty).
    fn format(out_buf: *[]u8): usize ! TraceContextError {
        let written: usize = 0;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const k: []const u8 = entries[i].key();
            const v: []const u8 = entries[i].value();
            const sep_len: usize = if (i == 0) 0 else 1;
            const need: usize = sep_len + k.len + 1 + v.len;
            if (need > out_buf.len - written) return TraceContextError.BufferTooSmall;

            if (i > 0) {
                out_buf[written] = ',';
                written += 1;
            }
            let ki: usize = 0;
            while (ki < k.len) : (ki += 1) {
                out_buf[written] = k[ki];
                written += 1;
            }
            out_buf[written] = '=';
            written += 1;
            let vi: usize = 0;
            while (vi < v.len) : (vi += 1) {
                out_buf[written] = v[vi];
                written += 1;
            }
        }
        return written;
    }
}

// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Kulkarni Hybrid Logical Clock - HLC)

/// Maximum 48-bit physical millisecond timestamp (0x0000FFFFFFFFFFFF, valid through year 10889).
pub const HLC_MAX_PHYSICAL_MS: u64 = 0x0000FFFFFFFFFFFF;

/// Maximum 16-bit logical counter value (0xFFFF).
pub const HLC_MAX_LOGICAL: u16 = 0xFFFF;

/// Default maximum tolerated clock drift in milliseconds (60 seconds).
pub const HLC_DEFAULT_MAX_DRIFT_MS: u64 = 60000;

/// Canonical lexicographically sortable string length: `PPPPPPPPPPPP-LLLL-NNNNNNNN` (12+1+4+1+8 = 26 bytes).
pub const HLC_STR_LEN: usize = 26;

const HEX_UPPER: []const u8 = "0123456789ABCDEF";

/// Hybrid Logical Clock timestamp carrying 48-bit physical ms, 16-bit logical counter, and 32-bit node ID.
pub struct HlcTimestamp {
    physical_ms: u64,
    logical: u16,
    node_id: u32,

    /// Packs the 48-bit physical timestamp and 16-bit logical counter into a single monotonic `u64`.
    fn packU64(): u64 {
        return ((_.physical_ms & HLC_MAX_PHYSICAL_MS) << 16) | (_.logical as u64);
    }

    /// Unpacks a 64-bit compact HLC value paired with `node_id` into an `HlcTimestamp`.
    static fn unpackU64(raw: u64, node_id: u32): HlcTimestamp {
        return HlcTimestamp {
            physical_ms: (raw >> 16) & HLC_MAX_PHYSICAL_MS,
            logical: (raw & 0xFFFF) as u16,
            node_id: node_id,
        };
    }

    /// Total order comparator: compares `physical_ms`, then `logical`, then `node_id`.
    /// Returns `-1` if `this < other`, `0` if equal, `1` if `this > other`.
    fn compare(other: HlcTimestamp): i8 {
        if (_.physical_ms < other.physical_ms) {
            return -1;
        }
        if (_.physical_ms > other.physical_ms) {
            return 1;
        }
        if (_.logical < other.logical) {
            return -1;
        }
        if (_.logical > other.logical) {
            return 1;
        }
        if (_.node_id < other.node_id) {
            return -1;
        }
        if (_.node_id > other.node_id) {
            return 1;
        }
        return 0;
    }

    /// Returns true if all fields (`physical_ms`, `logical`, `node_id`) are identical.
    fn equals(other: HlcTimestamp): bool {
        return _.compare(other) == 0;
    }

    /// Returns true if this timestamp strictly precedes `other` in total order.
    fn isBefore(other: HlcTimestamp): bool {
        return _.compare(other) < 0;
    }

    /// Returns true if this timestamp strictly follows `other` in total order.
    fn isAfter(other: HlcTimestamp): bool {
        return _.compare(other) > 0;
    }
}

/// Stateful Hybrid Logical Clock (Kulkarni et al.) with drift protection and logical overflow guards.
pub struct HlcClock {
    last: HlcTimestamp,
    max_drift_ms: u64,

    /// Initializes a new HLC clock bound to `node_id` with maximum tolerated drift `max_drift_ms`.
    static fn init(node_id: u32, max_drift_ms: u64): HlcClock {
        return HlcClock {
            last: HlcTimestamp {
                physical_ms: 0,
                logical: 0,
                node_id: node_id,
            },
            max_drift_ms: max_drift_ms,
        };
    }

    /// Advances the clock for a local or send event at physical wall time `wall_ms`.
    /// Guarantees strict monotonicity even under clock regression, while rejecting drift > `max_drift_ms`
    /// or logical counter overflow (`0xFFFF`).
    fn now(*_, wall_ms: u64): HlcTimestamp? {
        if (wall_ms > HLC_MAX_PHYSICAL_MS) {
            return null;
        }

        if (wall_ms > _.last.physical_ms) {
            _.last.physical_ms = wall_ms;
            _.last.logical = 0;
            return _.last;
        }

        // wall_ms <= _.last.physical_ms: check drift bound and logical overflow before mutating state.
        if ((_.last.physical_ms - wall_ms) > _.max_drift_ms) {
            return null;
        }
        if (_.last.logical == HLC_MAX_LOGICAL) {
            return null;
        }

        _.last.logical += 1;
        return _.last;
    }

    /// Merges a received `remote` HLC timestamp with local `wall_ms` following Kulkarni's 4-branch algorithm.
    /// Rejects remote or local timestamps exceeding `max_drift_ms` ahead of `wall_ms`, or logical overflow.
    /// Always preserves this clock's local `node_id`.
    fn receive(*_, wall_ms: u64, remote: HlcTimestamp): HlcTimestamp? {
        if (wall_ms > HLC_MAX_PHYSICAL_MS || remote.physical_ms > HLC_MAX_PHYSICAL_MS) {
            return null;
        }

        // Guard against remote node clock running too far ahead of local wall clock.
        if (remote.physical_ms > wall_ms && (remote.physical_ms - wall_ms) > _.max_drift_ms) {
            return null;
        }
        // Guard against local clock having regressed beyond max_drift_ms relative to last issued timestamp.
        if (_.last.physical_ms > wall_ms && (_.last.physical_ms - wall_ms) > _.max_drift_ms) {
            return null;
        }

        const l_prev: u64 = _.last.physical_ms;
        const r_phys: u64 = remote.physical_ms;

        let l_new: u64 = wall_ms;
        if (l_prev > l_new) {
            l_new = l_prev;
        }
        if (r_phys > l_new) {
            l_new = r_phys;
        }

        let c_new: u16 = 0;
        if (l_new == l_prev && l_new == r_phys) {
            // Branch 1: local and remote physical clocks tie at max (>= wall_ms).
            let max_c: u16 = _.last.logical;
            if (remote.logical > max_c) {
                max_c = remote.logical;
            }
            if (max_c == HLC_MAX_LOGICAL) {
                return null;
            }
            c_new = max_c + 1;
        } else if (l_new == l_prev) {
            // Branch 2: local logical/physical clock strictly leads remote and >= wall_ms.
            if (_.last.logical == HLC_MAX_LOGICAL) {
                return null;
            }
            c_new = _.last.logical + 1;
        } else if (l_new == r_phys) {
            // Branch 3: remote physical clock strictly leads local and >= wall_ms.
            if (remote.logical == HLC_MAX_LOGICAL) {
                return null;
            }
            c_new = remote.logical + 1;
        } else {
            // Branch 4: local physical wall_ms strictly leads both local.last and remote.
            c_new = 0;
        }

        _.last.physical_ms = l_new;
        _.last.logical = c_new;
        return _.last;
    }
}

fn writeHexBe(val: u64, digits: usize, out: *[]u8, offset: usize) {
    let i: usize = 0;
    while (i < digits) {
        const shift_bits: u6 = ((digits - 1 - i) * 4) as u6;
        const nibble: usize = ((val >> shift_bits) & 0x0F) as usize;
        out.*[offset + i] = HEX_UPPER[nibble];
        i += 1;
    }
}

fn decodeHexDigit(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }
    return null;
}

fn parseHexBe(text: []const u8, offset: usize, digits: usize): u64? {
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < digits) {
        if (d := decodeHexDigit(text[offset + i])) {
            acc = (acc << 4) | (d as u64);
        } else {
            return null;
        }
        i += 1;
    }
    return acc;
}

/// Formats `ts` into a fixed 26-byte uppercase big-endian hex string `PPPPPPPPPPPP-LLLL-NNNNNNNN`.
/// Byte-wise lexicographical order matches `HlcTimestamp.compare()` 100%.
/// Returns `HLC_STR_LEN` (26), or `null` if `ts.physical_ms > HLC_MAX_PHYSICAL_MS` or `out` is too small.
pub fn formatHlc(ts: HlcTimestamp, out: *[]u8): usize? {
    if (ts.physical_ms > HLC_MAX_PHYSICAL_MS) {
        return null;
    }
    if (out.*.len < HLC_STR_LEN) {
        return null;
    }

    writeHexBe(ts.physical_ms, 12, out, 0);
    out.*[12] = '-';
    writeHexBe(ts.logical as u64, 4, out, 13);
    out.*[17] = '-';
    writeHexBe(ts.node_id as u64, 8, out, 18);
    return HLC_STR_LEN;
}

/// Strictly parses a 26-byte canonical HLC string `PPPPPPPPPPPP-LLLL-NNNNNNNN` (accepting upper/lower hex).
/// Returns `null` on invalid length, missing `'-'` delimiters, or non-hex characters.
pub fn parseHlc(text: []const u8): HlcTimestamp? {
    if (text.len != HLC_STR_LEN) {
        return null;
    }
    if (text[12] != '-' || text[17] != '-') {
        return null;
    }

    if (phys := parseHexBe(text, 0, 12)) {
        if (logi := parseHexBe(text, 13, 4)) {
            if (node := parseHexBe(text, 18, 8)) {
                return HlcTimestamp {
                    physical_ms: phys,
                    logical: logi as u16,
                    node_id: node as u32,
                };
            }
        }
    }
    return null;
}

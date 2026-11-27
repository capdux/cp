// @cap/snowflake-id — Zero-heap 64-bit Snowflake distributed ID generator & decoder.
// Clean-room MIT implementation (41-bit ms timestamp | 5-bit datacenter | 5-bit worker | 12-bit seq).
// Reuses `@cap/itoa` for zero-heap decimal ID formatting.

import { formatU64 } from "../../itoa/src/index.cp";

pub const TWITTER_EPOCH_MS: u64 = 1288834974657;
pub const DISCORD_EPOCH_MS: u64 = 1420070400000;
pub const SF_MAX_DATACENTER: u16 = 31;
pub const SF_MAX_WORKER: u16 = 31;
pub const SF_MAX_SEQUENCE: u16 = 4095;
pub const SF_TIMESTAMP_SHIFT: u6 = 22;
pub const SF_DATACENTER_SHIFT: u6 = 17;
pub const SF_WORKER_SHIFT: u6 = 12;
pub const SF_MAX_TIMESTAMP: u64 = ((1 as u64) << 41) - 1;

pub struct SnowflakeParts {
    pub timestamp_ms: u64,
    pub datacenter_id: u16,
    pub worker_id: u16,
    pub sequence: u16,
}

/// Composes a 64-bit Snowflake ID from its 4 bitfields relative to `epoch_ms`.
pub fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64 {
    let ts: u64 = if (timestamp_ms >= epoch_ms) { timestamp_ms - epoch_ms } else { 0 };
    let dc: u64 = (datacenter_id & SF_MAX_DATACENTER) as u64;
    let wk: u64 = (worker_id & SF_MAX_WORKER) as u64;
    let sq: u64 = (seq & SF_MAX_SEQUENCE) as u64;
    return (ts << 22) | (dc << 17) | (wk << 12) | sq;
}

/// Alias for `encodeSnowflake`: composes a 64-bit Snowflake ID from its 4 bitfields.
pub fn compose(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, seq: u16): u64 {
    return encodeSnowflake(epoch_ms, timestamp_ms, datacenter_id, worker_id, seq);
}

/// Decodes a 64-bit Snowflake ID back into `(timestamp_ms, datacenter_id, worker_id, sequence)`.
pub fn decodeSnowflake(id: u64, epoch_ms: u64): SnowflakeParts {
    let ts = (id >> 22) + epoch_ms;
    let dc = ((id >> 17) & 31) as u16;
    let wk = ((id >> 12) & 31) as u16;
    let sq = (id & 4095) as u16;
    return SnowflakeParts {
        timestamp_ms: ts,
        datacenter_id: dc,
        worker_id: wk,
        sequence: sq,
    };
}

/// Alias for `decodeSnowflake`: decomposes a 64-bit Snowflake ID into its constituent fields.
pub fn decompose(id: u64, epoch_ms: u64): SnowflakeParts {
    return decodeSnowflake(id, epoch_ms);
}

/// Extracts the absolute millisecond timestamp from a Snowflake ID relative to `epoch_ms`.
pub fn timestamp(id: u64, epoch_ms: u64): u64 {
    return (id >> 22) + epoch_ms;
}

/// Extracts the 5-bit datacenter ID from a Snowflake ID.
pub fn datacenterId(id: u64): u16 {
    return ((id >> 17) & 31) as u16;
}

/// Extracts the 5-bit worker ID from a Snowflake ID.
pub fn workerId(id: u64): u16 {
    return ((id >> 12) & 31) as u16;
}

/// Extracts the 12-bit sequence number from a Snowflake ID.
pub fn sequence(id: u64): u16 {
    return (id & 4095) as u16;
}

/// Returns the minimum possible Snowflake ID for `timestamp_ms` (for range queries).
pub fn minIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64 {
    return encodeSnowflake(epoch_ms, timestamp_ms, 0, 0, 0);
}

/// Returns the maximum possible Snowflake ID for `timestamp_ms` (for range queries).
pub fn maxIdForTimestamp(epoch_ms: u64, timestamp_ms: u64): u64 {
    return encodeSnowflake(epoch_ms, timestamp_ms, SF_MAX_DATACENTER, SF_MAX_WORKER, SF_MAX_SEQUENCE);
}

/// Formats a 64-bit Snowflake ID as decimal ASCII into `out` via `@cap/itoa`.
pub fn formatSnowflake(id: u64, out: []u8): usize {
    return formatU64(id, out);
}

/// Parses a decimal ASCII Snowflake ID string into `u64`, returning `null` if invalid.
pub fn parseSnowflake(s: string): u64? {
    if (s.len == 0 || s.len > 20) { return null; }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let b = s[i];
        if (b < 48 || b > 57) { return null; }
        let d: u64 = (b - 48) as u64;
        if (acc > 1844674407370955161 || (acc == 1844674407370955161 && d > 5)) { return null; }
        acc = acc * 10 + d;
        i = i + 1;
    }
    return acc;
}

/// Alias for `parseSnowflake`: parses a decimal ASCII Snowflake ID string into `u64`.
pub fn parse(s: string): u64? {
    return parseSnowflake(s);
}

/// Returns `true` if `s` is a valid unsigned 64-bit decimal Snowflake ID string.
pub fn isValidSnowflakeString(s: string): bool {
    return parseSnowflake(s) != null;
}

/// Monotonic zero-heap Snowflake ID generator (`SnowflakeGenerator`).
pub struct SnowflakeGenerator {
    pub epoch_ms: u64,
    pub datacenter_id: u16,
    pub worker_id: u16,
    pub last_ts_ms: u64,
    pub sequence: u16,

    pub static fn new(epoch_ms: u64, datacenter_id: u16, worker_id: u16): SnowflakeGenerator {
        return SnowflakeGenerator {
            epoch_ms: epoch_ms,
            datacenter_id: datacenter_id & SF_MAX_DATACENTER,
            worker_id: worker_id & SF_MAX_WORKER,
            last_ts_ms: 0,
            sequence: 0,
        };
    }

    /// Creates a generator configured with the Twitter epoch (`1288834974657`).
    pub static fn withTwitterEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator {
        return SnowflakeGenerator.new(TWITTER_EPOCH_MS, datacenter_id, worker_id);
    }

    /// Creates a generator configured with the Discord epoch (`1420070400000`).
    pub static fn withDiscordEpoch(datacenter_id: u16, worker_id: u16): SnowflakeGenerator {
        return SnowflakeGenerator.new(DISCORD_EPOCH_MS, datacenter_id, worker_id);
    }

    /// Generates the next monotonically increasing Snowflake ID for `now_ms`.
    /// Returns `null` if clock moved backwards (`now_ms < last_ts_ms`) or sequence overflowed in same ms.
    pub fn nextId(*_, now_ms: u64): u64? {
        if (now_ms < _.last_ts_ms || now_ms < _.epoch_ms) {
            return null;
        }
        if (now_ms == _.last_ts_ms) {
            if (_.sequence >= SF_MAX_SEQUENCE) {
                return null;
            }
            _.sequence = _.sequence + 1;
        } else {
            _.last_ts_ms = now_ms;
            _.sequence = 0;
        }
        return encodeSnowflake(_.epoch_ms, now_ms, _.datacenter_id, _.worker_id, _.sequence);
    }

    /// Generates up to `out.len` IDs at `now_ms`, stopping early if sequence overflows or clock regressed.
    pub fn nextBatch(*_, now_ms: u64, out: []u64): usize {
        let count: usize = 0;
        while (count < out.len) {
            let id = _.nextId(now_ms);
            if (id == null) {
                return count;
            }
            out[count] = id ?? 0;
            count = count + 1;
        }
        return count;
    }

    /// Composes a Snowflake ID using this generator's epoch, datacenter_id, and worker_id.
    pub fn compose(timestamp_ms: u64, seq: u16): u64 {
        return encodeSnowflake(_.epoch_ms, timestamp_ms, _.datacenter_id, _.worker_id, seq);
    }

    /// Decomposes a Snowflake ID using this generator's configured epoch.
    pub fn decompose(id: u64): SnowflakeParts {
        return decodeSnowflake(id, _.epoch_ms);
    }

    /// Extracts the absolute timestamp of `id` using this generator's configured epoch.
    pub fn timestamp(id: u64): u64 {
        return (id >> 22) + _.epoch_ms;
    }

    /// Resets the internal monotonic timestamp and sequence counters.
    pub fn reset(*_) {
        _.last_ts_ms = 0;
        _.sequence = 0;
    }
}

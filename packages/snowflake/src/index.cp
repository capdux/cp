// @cap/snowflake-id — Zero-heap 64-bit Snowflake distributed ID generator & decoder.
// Clean-room MIT implementation (41-bit ms timestamp | 5-bit datacenter | 5-bit worker | 12-bit seq).
// Reuses `@cap/itoa` for zero-heap decimal ID formatting.

import { formatU64 } from "../../itoa/src/index.cp";

pub const TWITTER_EPOCH_MS: u64 = 1288834974657;
pub const DISCORD_EPOCH_MS: u64 = 1420070400000;
pub const SF_MAX_DATACENTER: u16 = 31;
pub const SF_MAX_WORKER: u16 = 31;
pub const SF_MAX_SEQUENCE: u16 = 4095;

pub struct SnowflakeParts {
    pub timestamp_ms: u64,
    pub datacenter_id: u16,
    pub worker_id: u16,
    pub sequence: u16,
}

/// Composes a 64-bit Snowflake ID from its 4 bitfields relative to `epoch_ms`.
pub fn encodeSnowflake(epoch_ms: u64, timestamp_ms: u64, datacenter_id: u16, worker_id: u16, sequence: u16): u64 {
    let ts: u64 = if (timestamp_ms >= epoch_ms) { timestamp_ms - epoch_ms } else { 0 };
    let dc: u64 = (datacenter_id & SF_MAX_DATACENTER) as u64;
    let wk: u64 = (worker_id & SF_MAX_WORKER) as u64;
    let sq: u64 = (sequence & SF_MAX_SEQUENCE) as u64;
    return (ts << 22) | (dc << 17) | (wk << 12) | sq;
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
        if (acc > 1844674407370955161) { return null; }
        acc = acc * 10 + d;
        i = i + 1;
    }
    return acc;
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
}

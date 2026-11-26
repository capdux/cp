// @cap/snowflake — Zero-heap 64-bit Twitter / Discord Snowflake distributed ID engine
// MIT License (clean-room CP implementation)

pub const TWITTER_EPOCH_MS: u64 = 1288834974657;
pub const DISCORD_EPOCH_MS: u64 = 1420070400000;

const MAX_DELTA_MS: u64 = 2199023255551; // (1 << 41) - 1

pub struct SnowflakeParts {
    pub timestamp_ms: u64,
    pub datacenter_id: u8,
    pub worker_id: u8,
    pub sequence: u16,
}

pub struct SnowflakeGenerator {
    pub epoch_ms: u64,
    pub datacenter_id: u8,
    pub worker_id: u8,
    pub last_ts_ms: u64,
    pub sequence: u16,
}

pub fn compose(
    epoch_ms: u64,
    timestamp_ms: u64,
    datacenter_id: u8,
    worker_id: u8,
    sequence: u16,
) -> u64 {
    if (timestamp_ms < epoch_ms) {
        return 0;
    }
    const delta: u64 = timestamp_ms - epoch_ms;
    if (delta > MAX_DELTA_MS || datacenter_id > 31 || worker_id > 31 || sequence > 4095) {
        return 0;
    }
    return (delta << 22)
        | ((datacenter_id as u64) << 17)
        | ((worker_id as u64) << 12)
        | (sequence as u64);
}

pub fn decompose(id: u64, epoch_ms: u64) -> SnowflakeParts {
    return SnowflakeParts {
        timestamp_ms: epoch_ms + (id >> 22),
        datacenter_id: ((id >> 17) & 0x1F) as u8,
        worker_id: ((id >> 12) & 0x1F) as u8,
        sequence: (id & 0xFFF) as u16,
    };
}

pub fn initGenerator(epoch_ms: u64, datacenter_id: u8, worker_id: u8) -> SnowflakeGenerator {
    return SnowflakeGenerator {
        epoch_ms: epoch_ms,
        datacenter_id: datacenter_id & 31,
        worker_id: worker_id & 31,
        last_ts_ms: 0,
        sequence: 0,
    };
}

pub fn nextId(gen: *SnowflakeGenerator, now_ms: u64) -> i64 {
    if (now_ms < gen.*.epoch_ms) {
        return -1;
    }
    if (gen.*.last_ts_ms > 0 && now_ms < gen.*.last_ts_ms) {
        return -1;
    }

    if (gen.*.last_ts_ms > 0 && now_ms == gen.*.last_ts_ms) {
        if (gen.*.sequence >= 4095) {
            return -2;
        }
        gen.*.sequence += 1;
    } else {
        gen.*.last_ts_ms = now_ms;
        gen.*.sequence = 0;
    }

    const delta: u64 = now_ms - gen.*.epoch_ms;
    if (delta > MAX_DELTA_MS) {
        return -1;
    }
    const id: u64 = compose(
        gen.*.epoch_ms,
        now_ms,
        gen.*.datacenter_id,
        gen.*.worker_id,
        gen.*.sequence,
    );
    return id as i64;
}

pub fn formatId(id: u64, out: []u8) -> usize {
    let digits: [24]u8 = undefined;
    let count: usize = 0;
    if (id == 0) {
        digits[0] = '0' as u8;
        count = 1;
    } else {
        let cur: u64 = id;
        while (cur > 0) {
            digits[count] = ('0' as u8) + ((cur % 10) as u8);
            count += 1;
            cur = cur / 10;
        }
    }
    if (count > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let k: usize = count;
    while (k > 0) {
        k -= 1;
        out[pos] = digits[k];
        pos += 1;
    }
    return pos;
}

pub fn parseId(s: string) -> i64 {
    if (s.len == 0 || s.len > 19) {
        return -1;
    }
    const max_i64: u64 = ((0x7FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c < '0' as u8 || c > '9' as u8) {
            return -1;
        }
        const d: u64 = (c - ('0' as u8)) as u64;
        if (acc > (max_i64 - d) / 10) {
            return -1;
        }
        acc = acc * 10 + d;
        i += 1;
    }
    return acc as i64;
}

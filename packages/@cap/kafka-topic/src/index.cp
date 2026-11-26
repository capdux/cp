// @cap/kafka-topic — Zero-allocation Apache Kafka Topic Validator, Official Murmur2 Partitioner & Spec Parser
// Clean-room MIT implementation in pure CP.

pub struct TopicPartition {
    pub topic: string,
    pub partition: i32,
    pub valid: bool,
}

fn isTopicChar(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    return b == 46 || b == 95 || b == 45;     // '.', '_', '-'
}

/// Validates an Apache Kafka topic name:
/// length 1..249, not `"."` or `".."`, allowed characters `A-Za-z0-9._-`.
pub fn isValidTopicName(name: string) -> bool {
    if (name.len == 0 || name.len > 249) {
        return false;
    }
    if (name.len == 1 && name[0] == 46) {
        return false;
    }
    if (name.len == 2 && name[0] == 46 && name[1] == 46) {
        return false;
    }
    let i: usize = 0;
    while (i < name.len) {
        if (!isTopicChar(name[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns true if `name` contains both `.` and `_`, which Kafka warns can collide in JMX metric names.
pub fn hasMetricCollisionRisk(name: string) -> bool {
    let has_dot = false;
    let has_under = false;
    let i: usize = 0;
    while (i < name.len) {
        if (name[i] == 46) {
            has_dot = true;
        } else if (name[i] == 95) {
            has_under = true;
        }
        i += 1;
    }
    return has_dot && has_under;
}

/// Exact Apache Kafka `Utils.murmur2` 32-bit hash (seed `0x9747b28c`, m `0x5bd1e995`, r `24`).
pub fn murmur2(data: []const u8) -> u32 {
    const seed: u32 = 0x9747b28c;
    const m: u32 = 0x5bd1e995;
    const n = data.len;

    let h: u32 = seed ^ (n as u32);
    const blocks = n / 4;

    let i: usize = 0;
    while (i < blocks) {
        const idx = i * 4;
        let k: u32 = (data[idx] as u32)
            | ((data[idx + 1] as u32) << 8)
            | ((data[idx + 2] as u32) << 16)
            | ((data[idx + 3] as u32) << 24);
        k = k *% m;
        k ^= k >> 24;
        k = k *% m;
        h = h *% m;
        h ^= k;
        i += 1;
    }

    const tail = blocks * 4;
    const rem = n - tail;
    if (rem == 3) {
        h ^= (data[tail + 2] as u32) << 16;
    }
    if (rem >= 2) {
        h ^= (data[tail + 1] as u32) << 8;
    }
    if (rem >= 1) {
        h ^= data[tail] as u32;
        h = h *% m;
    }

    h ^= h >> 13;
    h = h *% m;
    h ^= h >> 15;
    return h;
}

/// Clears the sign bit of a 32-bit hash (`hash & 0x7fffffff`), matching Kafka's `Utils.toPositive`.
pub fn toPositive(hash: u32) -> u32 {
    return hash & 0x7FFFFFFF;
}

/// Computes the deterministic Kafka `DefaultPartitioner` partition index for `key`.
/// Returns `0` if `num_partitions == 0`.
pub fn partitionForKey(key: []const u8, num_partitions: u32) -> u32 {
    if (num_partitions == 0) {
        return 0;
    }
    return toPositive(murmur2(key)) % num_partitions;
}

fn parseNonNegI32(s: string) -> i32 {
    if (s.len == 0 || s.len > 10) {
        return -1;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < 48 || ch > 57) {
            return -1;
        }
        acc = acc * 10 + ((ch - 48) as u64);
        if (acc > 2147483647) {
            return -1;
        }
        i += 1;
    }
    return acc as i32;
}

/// Parses a topic-partition spec in `"topic:partition"` or `"topic-partition"` form.
/// Validates the topic name and non-negative 32-bit partition ID.
pub fn parseTopicPartition(spec: string) -> TopicPartition {
    if (spec.len < 3) {
        return TopicPartition { topic: "", partition: -1, valid: false };
    }

    // Prefer last ':', otherwise fall back to last '-'
    let sep: usize = spec.len;
    let i = spec.len;
    while (i > 0) {
        i -= 1;
        if (spec[i] == 58) {
            sep = i;
            break;
        }
    }
    if (sep == spec.len) {
        let j = spec.len;
        while (j > 0) {
            j -= 1;
            if (spec[j] == 45) {
                sep = j;
                break;
            }
        }
    }

    if (sep == spec.len || sep == 0 || sep + 1 >= spec.len) {
        return TopicPartition { topic: "", partition: -1, valid: false };
    }

    const topic = spec[0..sep];
    const part_str = spec[sep + 1..spec.len];
    if (!isValidTopicName(topic)) {
        return TopicPartition { topic: "", partition: -1, valid: false };
    }

    const part = parseNonNegI32(part_str);
    if (part < 0) {
        return TopicPartition { topic: "", partition: -1, valid: false };
    }

    return TopicPartition {
        topic: topic,
        partition: part,
        valid: true,
    };
}

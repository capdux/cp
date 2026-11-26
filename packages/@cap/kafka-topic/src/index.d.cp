// Type definitions for CP

export class TopicPartition {
    topic: string,
    partition: i32,
    valid: bool,
}

export fn isTopicChar(b: u8): bool;
export fn isValidTopicName(name: string): bool;
export fn hasMetricCollisionRisk(name: string): bool;
export fn murmur2(data: &[]u8): u32;
export fn toPositive(hash: u32): u32;
export fn partitionForKey(key: &[]u8, num_partitions: u32): u32;
export fn parseNonNegI32(s: string): i32;
export fn parseTopicPartition(spec: string): TopicPartition;

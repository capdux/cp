# kafkajs@1.0.0 (Agent Quickref)

> Zero-allocation Apache Kafka / Redpanda topic validator, official Murmur2 DefaultPartitioner hash, and Topic-Partition parser in pure CP | License: MIT
> Import: `import { ... } from "kafkajs";` | Full types: `src/index.d.cp`

## Functions
- `export fn isTopicChar(b: u8): bool;`
- `export fn isValidTopicName(name: string): bool;`
- `export fn hasMetricCollisionRisk(name: string): bool;`
- `export fn murmur2(data: &[]u8): u32;`
- `export fn toPositive(hash: u32): u32;`
- `export fn partitionForKey(key: &[]u8, num_partitions: u32): u32;`
- `export fn parseNonNegI32(s: string): i32;`
- `export fn parseTopicPartition(spec: string): TopicPartition;`

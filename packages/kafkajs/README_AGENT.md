# kafkajs — AI Agent Quick Ref

Zero-allocation Apache Kafka / Redpanda topic validator, official Murmur2 DefaultPartitioner hash, and Topic-Partition parser in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    TopicPartition,
    isTopicChar,
    isValidTopicName,
    hasMetricCollisionRisk,
    murmur2,
    toPositive,
    partitionForKey,
    parseNonNegI32,
} from "kafkajs";
```

## Core Types & Constants

```cp
class TopicPartition { topic: string, partition: i32, valid: bool }
```

## API Signatures

```cp
fn isTopicChar(b: u8): bool;
fn isValidTopicName(name: string): bool;
fn hasMetricCollisionRisk(name: string): bool;
fn murmur2(data: &[]u8): u32;
fn toPositive(hash: u32): u32;
fn partitionForKey(key: &[]u8, num_partitions: u32): u32;
fn parseNonNegI32(s: string): i32;
fn parseTopicPartition(spec: string): TopicPartition;
```

## Minimal Usage

```cp
import { isValidTopicName } from "kafkajs";

pub fn main() {
    let result = isValidTopicName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"kafkajs"`; install via `cap add kafkajs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.

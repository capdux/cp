[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# kafkajs

Zero-allocation Apache Kafka / Redpanda topic validator, official Murmur2 DefaultPartitioner hash, and Topic-Partition parser in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kafkajs` ([https://github.com/tulios/kafkajs](https://github.com/tulios/kafkajs)) / `crates.io:rdkafka` ([https://github.com/fede1024/rust-rdkafka](https://github.com/fede1024/rust-rdkafka))（原库名 `kafkajs` → CP 包名 `kafkajs`）

| Metadata | Value |
| --- | --- |
| Package | `kafkajs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add kafkajs
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `TopicPartition` | `class` | Fields: `topic: string, partition: i32, valid: bool` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `isTopicChar` | `(b: u8): bool` | Checks predicate `isTopicChar` and returns `bool`. |
| `isValidTopicName` | `(name: string): bool` | Checks predicate `isValidTopicName` and returns `bool`. |
| `hasMetricCollisionRisk` | `(name: string): bool` | Checks predicate `hasMetricCollisionRisk` and returns `bool`. |
| `murmur2` | `(data: &[]u8): u32` | Invokes `murmur2` and returns `u32`. |
| `toPositive` | `(hash: u32): u32` | Invokes `toPositive` and returns `u32`. |
| `partitionForKey` | `(key: &[]u8, num_partitions: u32): u32` | Invokes `partitionForKey` and returns `u32`. |
| `parseNonNegI32` | `(s: string): i32` | Parses input via `parseNonNegI32` and returns `i32`. |
| `parseTopicPartition` | `(spec: string): TopicPartition` | Parses input via `parseTopicPartition` and returns `TopicPartition`. |

## Minimal Example

```cp
import { isValidTopicName } from "kafkajs";

pub fn main() {
    let result = isValidTopicName("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).

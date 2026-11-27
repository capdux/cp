[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `kafkajs`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kafkajs` ([https://github.com/tulios/kafkajs](https://github.com/tulios/kafkajs)) / `crates.io:rdkafka` ([https://github.com/fede1024/rust-rdkafka](https://github.com/fede1024/rust-rdkafka))（原库名 `kafkajs` → CP 包名 `kafkajs`）

Zero-heap-allocation Apache Kafka / Redpanda topic validator, official Murmur2 `DefaultPartitioner` hash, and Topic-Partition parser in pure CP.

## Features

- **`isValidTopicName` & `hasMetricCollisionRisk`**: Enforces Kafka's 1..249 character topic naming rules and flags `.`+`_` metric name collisions.
- **`murmur2`, `toPositive`, `partitionForKey`**: Bit-exact match to Apache Kafka `Utils.murmur2` (seed `0x9747b28c`) and `DefaultPartitioner`.
- **`parseTopicPartition`**: Zero-copy parser for `"orders.events:12"` and `"orders.events-12"` specs.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    TopicPartition,
    isValidTopicName,
    hasMetricCollisionRisk,
    murmur2,
    toPositive,
    partitionForKey,
    parseTopicPartition,
} from "kafkajs";
```

## License

MIT

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# kafkajs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Apache Kafka / Redpanda topic validator, official Murmur2 DefaultPartitioner hash, and Topic-Partition parser in pure CP

## 安装与导入

```bash
cap add kafkajs
```

```cp
import { isTopicChar } from "kafkajs";
```

## 核心导出 API

- `export fn isTopicChar(b: u8): bool;`
- `export fn isValidTopicName(name: string): bool;`
- `export fn hasMetricCollisionRisk(name: string): bool;`
- `export fn murmur2(data: &[]u8): u32;`
- `export fn toPositive(hash: u32): u32;`
- `export fn partitionForKey(key: &[]u8, num_partitions: u32): u32;`
- `export fn parseNonNegI32(s: string): i32;`
- `export fn parseTopicPartition(spec: string): TopicPartition;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`kafkajs@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`rdkafka@1.0.0`)，许可证：MIT

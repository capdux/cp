[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# kafkajs

Zero-allocation Apache Kafka / Redpanda topic validator, official Murmur2 DefaultPartitioner hash, and Topic-Partition parser in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kafkajs` ([https://github.com/tulios/kafkajs](https://github.com/tulios/kafkajs)) / `crates.io:rdkafka` ([https://github.com/fede1024/rust-rdkafka](https://github.com/fede1024/rust-rdkafka))（原库名 `kafkajs` → CP 包名 `kafkajs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `kafkajs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

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

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TopicPartition` | `class` | 字段: `topic: string, partition: i32, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isTopicChar` | `(b: u8): bool` | 执行 `isTopicChar` 条件判定，返回 `bool`。 |
| `isValidTopicName` | `(name: string): bool` | 执行 `isValidTopicName` 条件判定，返回 `bool`。 |
| `hasMetricCollisionRisk` | `(name: string): bool` | 执行 `hasMetricCollisionRisk` 条件判定，返回 `bool`。 |
| `murmur2` | `(data: &[]u8): u32` | 调用 `murmur2` 并返回 `u32`。 |
| `toPositive` | `(hash: u32): u32` | 调用 `toPositive` 并返回 `u32`。 |
| `partitionForKey` | `(key: &[]u8, num_partitions: u32): u32` | 调用 `partitionForKey` 并返回 `u32`。 |
| `parseNonNegI32` | `(s: string): i32` | 通过 `parseNonNegI32` 解析输入数据，返回 `i32`。 |
| `parseTopicPartition` | `(spec: string): TopicPartition` | 通过 `parseTopicPartition` 解析输入数据，返回 `TopicPartition`。 |

## 最小调用示例

```cp
import { isValidTopicName } from "kafkajs";

pub fn main() {
    let result = isValidTopicName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

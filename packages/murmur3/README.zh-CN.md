[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# murmur3

Zero-heap MurmurHash3 (32-bit x86 & 128-bit x64) deterministic non-cryptographic hash functions for sharding, Kafka/Cassandra/ClickHouse partitioning

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:murmur3` ([https://github.com/stusmall/murmur3](https://github.com/stusmall/murmur3))

| 元信息 | 值 |
| --- | --- |
| 包名 | `murmur3` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add murmur3
```

```cp
import {
    Hash128,
    rotl32,
    rotl64,
    fmix32,
    fmix64,
    murmur3_32,
    murmur3_128,
} from "murmur3";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Hash128` | `class` | 字段: `lo: u64, hi: u64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rotl32` | `(x: u32, r: u5): u32` | 调用 `rotl32` 并返回 `u32`。 |
| `rotl64` | `(x: u64, r: u6): u64` | 调用 `rotl64` 并返回 `u64`。 |
| `fmix32` | `(h_in: u32): u32` | 调用 `fmix32` 并返回 `u32`。 |
| `fmix64` | `(k_in: u64): u64` | 调用 `fmix64` 并返回 `u64`。 |
| `murmur3_32` | `(data: string, seed: u32): u32` | 调用 `murmur3_32` 并返回 `u32`。 |
| `murmur3_128` | `(data: string, seed: u64): Hash128` | 调用 `murmur3_128` 并返回 `Hash128`。 |

## 最小调用示例

```cp
import { murmur3_32 } from "murmur3";

pub fn main() {
    let result = murmur3_32("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

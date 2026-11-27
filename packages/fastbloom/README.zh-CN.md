[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# fastbloom

Zero-allocation Kirsch-Mitzenmacher double-hashing Bloom filters BloomFilter1024/BloomFilter4096 (Rust fastbloom / bloomfilter parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:fastbloom` ([https://github.com/tomtomwombat/fastbloom](https://github.com/tomtomwombat/fastbloom))（原库名 `fastbloom` → CP 包名 `fastbloom`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `fastbloom` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add fastbloom
```

```cp
import {
    HashPair,
    BloomFilter1024,
    BloomFilter4096,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    hashBytesPair,
} from "fastbloom";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HashPair` | `class` | 字段: `h1: u64, h2: u64` |
| `BloomFilter1024` | `class` | 字段: `words: [16]u64, num_hashes: u8, items: usize` · 方法: `init, insert, mightContain, insertU64, mightContainU64, bitCount, +5 项` |
| `BloomFilter4096` | `class` | 字段: `words: [64]u64, num_hashes: u8, items: usize` · 方法: `init, insert, mightContain, insertU64, mightContainU64, bitCount, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `splitMix64` | `(x_in: u64): u64` | 调用 `splitMix64` 并返回 `u64`。 |
| `hashBytesPair` | `(s: string): HashPair` | 执行 `hashBytesPair` 条件判定，返回 `bool`。 |
| `hashU64Pair` | `(val: u64): HashPair` | 执行 `hashU64Pair` 条件判定，返回 `bool`。 |
| `bitMask64` | `(bit: usize): u64` | 调用 `bitMask64` 并返回 `u64`。 |
| `popcount64` | `(x_in: u64): usize` | 调用 `popcount64` 并返回 `usize`。 |
| `clampHashes` | `(k: u8): u8` | 调用 `clampHashes` 并返回 `u8`。 |
| `expNeg` | `(x_in: f64): f64` | 调用 `expNeg` 并返回 `f64`。 |
| `computeFpr` | `(k: u8, n: usize, m: usize): f64` | 调用 `computeFpr` 并返回 `f64`。 |

## 最小调用示例

```cp
import { hashBytesPair } from "fastbloom";

pub fn main() {
    let result = hashBytesPair("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

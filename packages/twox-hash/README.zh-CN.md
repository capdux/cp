[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# twox-hash

Zero-heap pure-CP XXHash32 and XXHash64 fast non-cryptographic streaming hashers (crates.io twox-hash equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:twox-hash` ([https://github.com/shepmaster/twox-hash](https://github.com/shepmaster/twox-hash))

| 元信息 | 值 |
| --- | --- |
| 包名 | `twox-hash` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add twox-hash
```

```cp
import {
    XxHash32,
    XxHash64,
    xxhU64,
    xxhAdd32,
    xxhSub32,
    xxhMul32,
    xxhRol32,
    xxhAdd64,
} from "twox-hash";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `XxHash32` | `class` | 字段: `seed: u32, v1: u32, v2: u32, v3: u32, v4: u32, buf: [16]u8, +2 项` · 方法: `withSeed, oneshot, write, writeStr, finish` |
| `XxHash64` | `class` | 字段: `seed: u64, v1: u64, v2: u64, v3: u64, v4: u64, buf: [32]u8, +2 项` · 方法: `withSeed, oneshot, write, writeStr, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `xxhU64` | `(hi: u32, lo: u32): u64` | 调用 `xxhU64` 并返回 `u64`。 |
| `xxhAdd32` | `(a: u32, b: u32): u32` | 调用 `xxhAdd32` 并返回 `u32`。 |
| `xxhSub32` | `(a: u32, b: u32): u32` | 调用 `xxhSub32` 并返回 `u32`。 |
| `xxhMul32` | `(a: u32, b: u32): u32` | 调用 `xxhMul32` 并返回 `u32`。 |
| `xxhRol32` | `(x: u32, r: u32): u32` | 调用 `xxhRol32` 并返回 `u32`。 |
| `xxhAdd64` | `(a: u64, b: u64): u64` | 调用 `xxhAdd64` 并返回 `u64`。 |
| `xxhSub64` | `(a: u64, b: u64): u64` | 调用 `xxhSub64` 并返回 `u64`。 |
| `xxhMul64` | `(a: u64, b: u64): u64` | 调用 `xxhMul64` 并返回 `u64`。 |
| `xxhRol64` | `(x: u64, r: u64): u64` | 调用 `xxhRol64` 并返回 `u64`。 |
| `xxh32Round` | `(acc: u32, lane: u32): u32` | 调用 `xxh32Round` 并返回 `u32`。 |
| `xxh64Prime1` | `(): u64` | 调用 `xxh64Prime1` 并返回 `u64`。 |
| `xxh64Prime2` | `(): u64` | 调用 `xxh64Prime2` 并返回 `u64`。 |
| `xxh64Prime3` | `(): u64` | 调用 `xxh64Prime3` 并返回 `u64`。 |
| `xxh64Prime4` | `(): u64` | 调用 `xxh64Prime4` 并返回 `u64`。 |
| `xxh64Prime5` | `(): u64` | 调用 `xxh64Prime5` 并返回 `u64`。 |
| `xxh64Round` | `(acc: u64, lane: u64): u64` | 调用 `xxh64Round` 并返回 `u64`。 |
| `xxh64MergeRound` | `(acc: u64, val: u64): u64` | 调用 `xxh64MergeRound` 并返回 `u64`。 |

## 最小调用示例

```cp
import { xxhU64 } from "twox-hash";

pub fn main() {
    let result = xxhU64(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

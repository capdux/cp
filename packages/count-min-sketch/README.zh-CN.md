[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# count-min-sketch

Zero-allocation Count-Min Sketch streaming frequency estimator and heavy-hitter detector (Rust count-min-sketch / Redis CMS parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:count-min-sketch` ([https://github.com/amansarda/count-min-sketch](https://github.com/amansarda/count-min-sketch))（原库名 `count-min-sketch` → CP 包名 `count-min-sketch`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `count-min-sketch` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add count-min-sketch
```

```cp
import {
    CountMinSketch,
    makeU64,
    addWrap64,
    mulWrap64,
    splitMix64,
    rowSeed,
    hashBytesRow,
    hashU64Row,
} from "count-min-sketch";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CountMinSketch` | `class` | 字段: `counters: [CMS_CELLS]u32, total: u64` · 方法: `init, clear, width, depth, totalCount, add, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `makeU64` | `(hi: u32, lo: u32): u64` | 调用 `makeU64` 并返回 `u64`。 |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `splitMix64` | `(x_in: u64): u64` | 调用 `splitMix64` 并返回 `u64`。 |
| `rowSeed` | `(row: usize): u64` | 调用 `rowSeed` 并返回 `u64`。 |
| `hashBytesRow` | `(s: string, row: usize): usize` | 执行 `hashBytesRow` 条件判定，返回 `bool`。 |
| `hashU64Row` | `(val: u64, row: usize): usize` | 执行 `hashU64Row` 条件判定，返回 `bool`。 |
| `satAddU32` | `(a: u32, b: u32): u32` | 调用 `satAddU32` 并返回 `u32`。 |

## 最小调用示例

```cp
import { hashBytesRow } from "count-min-sketch";

pub fn main() {
    let result = hashBytesRow("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

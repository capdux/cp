[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pcg-random

Zero-heap Melissa O'Neill PCG-XSH-RR 32-bit deterministic PRNG with stream separation, unbiased bounded sampling, O(log n) advance, and byte filling.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pcg-random` ([https://github.com/ThomWright/pcg-random](https://github.com/ThomWright/pcg-random))（原库名 `pcg-random` → CP 包名 `pcg-random`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `pcg-random` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pcg-random
```

```cp
import {
    Pcg32,
    pcgMult,
    defaultStream,
    add64,
    mul64,
    rotr32,
} from "pcg-random";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Pcg32` | `class` | 字段: `state: u64, inc: u64` · 方法: `new, fromSeed, nextU32, nextU64, nextF64, nextBounded, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pcgMult` | `(): u64` | 调用 `pcgMult` 并返回 `u64`。 |
| `defaultStream` | `(): u64` | 调用 `defaultStream` 并返回 `u64`。 |
| `add64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mul64` | `(a: u64, b: u64): u64` | 调用 `mul64` 并返回 `u64`。 |
| `rotr32` | `(x: u32, rot: u32): u32` | 调用 `rotr32` 并返回 `u32`。 |

## 最小调用示例

```cp
import { pcgMult } from "pcg-random";

pub fn main() {
    let result = pcgMult();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
